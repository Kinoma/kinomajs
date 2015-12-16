/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#define __FSKNETUTILS_PRIV__
#include "FskHTTPClient.h"
#include "FskResolver.h"
#include "FskNetInterface.h"

#include "FskHTTPAuth.h"

#if OPEN_SSL || CLOSED_SSL
#include "FskSSL.h"
#endif

#define CHECK_AGAINST_TOO_MANY_AUTH_REQUESTS 5	// after authentication fails, should we limit how many times we retry?
#define INSTR_PACKET_CONTENTS	0		// 1 to emit packet data instrumentation messages (binary data causes problems)

#if TARGET_OS_WIN32
#include "Wininet.h"
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
#include <SystemConfiguration/SystemConfiguration.h>
#endif

enum {
// idle
	kHTTPClientIdle = 0,		// 0
	kHTTPClientSuspended,
	kHTTPClientInitialize,		// not set up yet
	kHTTPClientDone,

// request
	kHTTPClientConnecting,		// waiting for connection
	kHTTPClientConnectingSSL,	// 5 // waiting for SSL connection (thru proxy)
	kHTTPClientSendRequest,
	kHTTPClientPrepareBody,
	kHTTPClientSendRequestBody,
	kHTTPClientRequestError,

// response
	kHTTPClientPrepareForResponse,	// 10
	kHTTPClientReadResponseHeaders,
	kHTTPClientProcessResponseHeaders,
	kHTTPClientPrepareForResponseBody,
	kHTTPClientReadResponseBody,
	kHTTPClientProcessResponseBody,	// 15
	kHTTPClientReadRemainder,
	kHTTPClientRequestDone,

	kHTTPClientConnectFailed,
	kHTTPClientSocketError,
	kHTTPClientError,				// 20
	kHTTPClientLostInterface,

	kHTTPClientCancel,

	kHTTPClientNotSuspended		// used to indicate suspend state
};

#define kFskHTTPClientKillTimerSecs	0	// default to off, user can set on if he wants

#define kMaxAutoRedirectTimes	5		// RFC 2616 - 10.3

#define kFskHTTPClientEndOfChunk	(-2)
#define kFskHTTPNoContentLength		(-1)
typedef struct {
	Boolean	useconfig;		// if enabled, it forces kconfig to override system proxies
	// from kconfig.xml
	int gHTTPProxyAddr;
	int gHTTPProxyPort;
	int gHTTPSProxyAddr;
	int gHTTPSProxyPort;

	// obtained via OS controls on Windows and Mac (or environment on Linux?)
	int gHTTPSystemProxyAddr;
	int gHTTPSystemProxyPort;
	int gHTTPSSystemProxyAddr;
	int gHTTPSSystemProxyPort;

	char* proxyAuthentication;

	FskNetInterfaceNotifier HTTPClientInterfaceNotifierRef;

	FskInstrumentedItemDeclaration
} FskHTTPClientSystemRecord, *FskHTTPClientSystem;
static FskHTTPClientSystemRecord gFskHTTPClient;
static Boolean gFskHTTPClientInitialized = false;

static FskErr sFskHTTPClientDispose(FskHTTPClient client);
static void sFskHTTPClientUpUse(FskHTTPClient client);
static void sFskHTTPClientDownUse(FskHTTPClient client);
static FskErr sFskHTTPClientRequestDispose(FskHTTPClientRequest request);
static void sFskHTTPClientRequestUpUse(FskHTTPClientRequest request);
static void sFskHTTPClientRequestDownUse(FskHTTPClientRequest request);

static FskErr sHTTPClientGotSocket(FskSocket skt, void *refCon);
static FskErr sHTTPClientMakeRequest(FskHTTPClient client);
static FskErr sPrepareHTTPClientRequestBlock(FskHTTPClient client);
static FskErr sHTTPClientRequestBlob(FskHTTPClient client, FskHTTPClientRequest request, char *buffer, int bufferSizeMax, int *bufferSizeUsed);
static int sHTTPClientGetRequestLength(FskHTTPClientRequest request);
static void sClientCycle(void *param);
static void sCanReadData(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);
static void sCanSendData(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);
static void sClientRunCycle(FskTimeCallBack cb, FskTime when, void *param);
static FskErr clientReceivedData(FskHTTPClientRequest req, char *buffer, int amt);
static void sReissueRequest(FskHTTPClient client, FskHTTPClientRequest request);
#if OPEN_SSL
static FskErr sHTTPClientConnectedSSLThruProxy(struct FskSocketRecord *skt, void *refCon);
#endif

static int clientInterfaceChangedCB(struct FskNetInterfaceRecord *iface, UInt32 status, void *param);

FskErr FskHTTPAuthNew(FskHTTPAuth *authOut);
FskErr FskHTTPAuthDispose(FskHTTPAuth auth);


#if SUPPORT_INSTRUMENTATION

#include <stddef.h>
static Boolean doFormatMessageHTTPClient(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);
	static FskInstrumentedValueRecord gInstrumentationHTTPClientValues[] = {
		{ "host",				offsetof(FskHTTPClientRecord, host),			kFskInstrumentationKindString},
		{ "hostPort",			offsetof(FskHTTPClientRecord, hostPort),		kFskInstrumentationKindInteger},
		{ "priority",			offsetof(FskHTTPClientRecord, priority),		kFskInstrumentationKindInteger},
		{ "idleKillSecs",		offsetof(FskHTTPClientRecord, idleKillSecs),	kFskInstrumentationKindInteger},
		{ "isLocal",			offsetof(FskHTTPClientRecord, isLocal),			kFskInstrumentationKindBoolean},
		{ NULL,					0,												kFskInstrumentationKindUndefined}
	};

	static FskInstrumentedValueRecord gInstrumentationHTTPClientRequestValues[] = {
		{ "method",				offsetof(FskHTTPClientRequestRecord, method),	kFskInstrumentationKindString},
		{ "scheme",				offsetof(FskHTTPClientRequestRecord, parsedUrl),kFskInstrumentationKindString | kFskInstrumentationKindHasOffset2,	NULL, offsetof(FskStrParsedUrlRecord, scheme)},
		{ "host",				offsetof(FskHTTPClientRequestRecord, parsedUrl),kFskInstrumentationKindString | kFskInstrumentationKindHasOffset2,	NULL, offsetof(FskStrParsedUrlRecord, host)},
		{ "port",				offsetof(FskHTTPClientRequestRecord, parsedUrl),kFskInstrumentationKindInteger | kFskInstrumentationKindHasOffset2,	NULL, offsetof(FskStrParsedUrlRecord, port)},
		{ "path",				offsetof(FskHTTPClientRequestRecord, parsedUrl),kFskInstrumentationKindString | kFskInstrumentationKindHasOffset2,	NULL, offsetof(FskStrParsedUrlRecord, path)},
		{ "params",				offsetof(FskHTTPClientRequestRecord, parsedUrl),kFskInstrumentationKindString | kFskInstrumentationKindHasOffset2,	NULL, offsetof(FskStrParsedUrlRecord, params)},
		{ NULL,					0,												kFskInstrumentationKindUndefined}
	};

static FskInstrumentedTypeRecord gFskHTTPClientTypeSystem = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"httpsystem",
	FskInstrumentationOffset(FskHTTPClientSystemRecord),
	NULL,
	0,
	NULL,
	doFormatMessageHTTPClient
};

static FskInstrumentedTypeRecord gFskHTTPClientTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"httpclient",
	FskInstrumentationOffset(FskHTTPClientRecord),
	NULL,
	0,
	NULL,
	doFormatMessageHTTPClient,
	gInstrumentationHTTPClientValues
};

static FskInstrumentedTypeRecord gFskHTTPClientRequestTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"httpclientrequest",
	FskInstrumentationOffset(FskHTTPClientRequestRecord),
	NULL,
	0,
	NULL,
	doFormatMessageHTTPClient,
	gInstrumentationHTTPClientRequestValues
};

static FskInstrumentedTypeRecord gFskHTTPClientAuthTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"httpclientauth",
	FskInstrumentationOffset(FskHTTPAuthRecord),
	NULL,
	0,
	NULL,
	doFormatMessageHTTPClient
};

#endif

FskErr sFskHTTPClientGetSystemProxy(int type, int *outAddr, int *outPort)
{
	FskErr err = kFskErrUnimplemented;
	int port = 0, addr = 0;

#if TARGET_OS_WIN32
	// configure HTTP proxy
	DWORD dwSize = 0;
	INTERNET_PROXY_INFO *pi;

	if ((kFskHTTPClientProxy != type) && (kFskHTTPSClientProxy != type))
		return kFskErrInvalidParameter;

	err = kFskErrNone;
	InternetQueryOption(NULL, INTERNET_OPTION_PROXY, NULL, &dwSize);
	if (kFskErrNone == FskMemPtrNewClear(dwSize, (FskMemPtr *)&pi)) {
		BOOL result = InternetQueryOption(NULL, INTERNET_OPTION_PROXY, pi, &dwSize);
		if (result && (INTERNET_OPEN_TYPE_PROXY == pi->dwAccessType)) {
			char *p = (char *)pi->lpszProxy;
			char *proxy = NULL;

			while (NULL != p) {
				char *scheme = p;
				char *value = FskStrChr(scheme, '=');
				if (NULL == value) { // use same proxy for all schemes
					proxy = p;
					break;
				}
				*value++ = 0;
				p = FskStrChr(value, ' ');
				if (p)
					*p++ = 0;

				if ((kFskHTTPClientProxy == type)
					&& (0 == FskStrCompare(scheme, "http"))) {
					proxy = value;
					break;
				}
				else if ((kFskHTTPSClientProxy == type)
					&& (0 == FskStrCompare(scheme, "https"))) {
					proxy = value;
					break;
				}
			}

			if (NULL != proxy) {
				FskStrParsedUrl parsedUrl = NULL;

				if (kFskErrNone == FskStrParseUrl(proxy, &parsedUrl)) {
					if (NULL != parsedUrl->host)
						FskNetHostnameResolve(parsedUrl->host, &addr);
					port = parsedUrl->port;
				}
				FskStrParsedUrlDispose(parsedUrl);
			}
		}
		FskMemPtrDispose(pi);
	}
#elif TARGET_OS_IPHONE
#elif TARGET_OS_MAC
	CFDictionaryRef proxyDict;
	CFNumberRef enableNum = NULL;
    int enable;
	CFStringRef hostStr = NULL;
	CFNumberRef portNum;
	Boolean result;

	if ((kFskHTTPClientProxy != type) && (kFskHTTPSClientProxy != type))
		return kFskErrInvalidParameter;

	err = kFskErrNone;

	// Get the dictionary.
	proxyDict = SCDynamicStoreCopyProxies(NULL);
	result = (proxyDict != NULL);
	if (result) {
		// Get the enable flag.
		if (type == kFskHTTPClientProxy)
            enableNum = (CFNumberRef) CFDictionaryGetValue(proxyDict, kSCPropNetProxiesHTTPEnable);
		else if (type == kFskHTTPSClientProxy)
			enableNum = (CFNumberRef) CFDictionaryGetValue(proxyDict, kSCPropNetProxiesHTTPSEnable);
		result = (enableNum != NULL) && (CFGetTypeID(enableNum) == CFNumberGetTypeID());
	}
	if (result) {
		result = CFNumberGetValue(enableNum, kCFNumberIntType, &enable) && (enable != 0);
	}
	if (result) {
		// Get the proxy host.
		if (type == kFskHTTPClientProxy)
            hostStr = (CFStringRef) CFDictionaryGetValue(proxyDict, kSCPropNetProxiesHTTPProxy);
		else if (type == kFskHTTPSClientProxy)
			hostStr = (CFStringRef) CFDictionaryGetValue(proxyDict, kSCPropNetProxiesHTTPSProxy);
		result = (hostStr != NULL) && (CFGetTypeID(hostStr) == CFStringGetTypeID());
	}
	if (result) {
		char host[4096];
		result = CFStringGetCString(hostStr, host, sizeof(host), kCFStringEncodingASCII);
		if (FskStrLen(host))
			FskNetHostnameResolve(host, &addr);
	}
	if (result) {
		// Get the proxy port.
		portNum = (CFNumberRef) CFDictionaryGetValue(proxyDict, kSCPropNetProxiesHTTPPort);
		if (type == kFskHTTPClientProxy)
			portNum = (CFNumberRef) CFDictionaryGetValue(proxyDict, kSCPropNetProxiesHTTPPort);
		else if (type == kFskHTTPSClientProxy)
			portNum = (CFNumberRef) CFDictionaryGetValue(proxyDict, kSCPropNetProxiesHTTPSPort);
		result = (portNum != NULL) && (CFGetTypeID(portNum) == CFNumberGetTypeID());
	}
	if (result) {
		result = CFNumberGetValue(portNum, kCFNumberIntType, &port);
	}

	if (proxyDict != NULL) {
        CFRelease(proxyDict);
	}
#endif
	*outAddr = addr;
	*outPort = port;

	return err;
}

FskErr FskHTTPGetProxy(int type, int *addr, int *port)
{
	FskErr err = kFskErrNone;
	int paddr = 0, pport = 0;

	switch (type) {
		case kFskHTTPClientProxy:
			if (!gFskHTTPClient.useconfig && gFskHTTPClient.gHTTPSystemProxyAddr) {
				paddr = gFskHTTPClient.gHTTPSystemProxyAddr;
				pport = gFskHTTPClient.gHTTPSystemProxyPort;
			}
			else {
				paddr = gFskHTTPClient.gHTTPProxyAddr;
				pport = gFskHTTPClient.gHTTPProxyPort;
			}
			break;
		case kFskHTTPSClientProxy:
			if (!gFskHTTPClient.useconfig && gFskHTTPClient.gHTTPSSystemProxyAddr) {
				paddr = gFskHTTPClient.gHTTPSSystemProxyAddr;
				pport = gFskHTTPClient.gHTTPSSystemProxyPort;
			}
			else {
				paddr = gFskHTTPClient.gHTTPSProxyAddr;
				pport = gFskHTTPClient.gHTTPSProxyPort;
			}
			break;
		default:
			err = kFskErrInvalidParameter;
			break;
	}
	if (addr) *addr = paddr;
	if (port) *port = pport;
	return err;
}

void FskHTTPSetDefaultProxyAuth( const char* authString )
{
	if( gFskHTTPClient.proxyAuthentication )
	{
		FskMemPtrDispose( gFskHTTPClient.proxyAuthentication );
		gFskHTTPClient.proxyAuthentication = NULL;
	}
	if( authString && FskStrLen( authString) )
		gFskHTTPClient.proxyAuthentication = FskStrDoCopy( authString );
}

FskErr FskHTTPSetDefaultProxy(int type, int addr, int port)
{
	FskErr err = kFskErrNone;

	switch (type) {
		case kFskHTTPClientProxy:
			gFskHTTPClient.gHTTPProxyAddr = addr;
			gFskHTTPClient.gHTTPProxyPort = port;
			FskInstrumentedItemSendMessage(&gFskHTTPClient, kFskHTTPClientInstrMsgHTTPProxy, &gFskHTTPClient);
			break;
		case kFskHTTPSClientProxy:
			gFskHTTPClient.gHTTPSProxyAddr = addr;
			gFskHTTPClient.gHTTPSProxyPort = port;
			FskInstrumentedItemSendMessage(&gFskHTTPClient, kFskHTTPClientInstrMsgHTTPSProxy, &gFskHTTPClient);
			break;
		default:
			err = kFskErrInvalidParameter;
			break;
	}
	return err;
}

FskErr FskHTTPClientGetProxy(FskHTTPClient client, int type, int *addr, int *port)
{
	FskErr err = kFskErrNone;

	if (!client)
		return kFskErrInvalidParameter;

	switch (type) {
		case kFskHTTPClientProxy:
			if (addr) *addr = client->proxyAddr;
			if (port) *port = client->proxyPort;
			break;
		case kFskHTTPSClientProxy:
			if (addr) *addr = client->proxyAddrS;
			if (port) *port = client->proxyPortS;
			break;
		default:
			err = kFskErrInvalidParameter;
			break;
	}
	return err;
}

FskErr FskHTTPSetProxyOverride(Boolean proxyOverride)
{
	gFskHTTPClient.useconfig = proxyOverride;
	return kFskErrNone;
}

FskErr FskHTTPClientSetProxy(FskHTTPClient client, int type, int addr, int port)
{
	FskErr err = kFskErrNone;

	if (!client)
		return kFskErrInvalidParameter;

	switch (type) {
		case kFskHTTPClientProxy:
			client->proxyAddr = addr;
			client->proxyPort = port;
			break;
		case kFskHTTPSClientProxy:
			client->proxyAddrS = addr;
			client->proxyPortS = port;
			break;
		default:
			err = kFskErrInvalidParameter;
			break;
	}
	return err;
}

void sSetClientProxy(FskHTTPClient client) {
	if (!gFskHTTPClient.useconfig && gFskHTTPClient.gHTTPSystemProxyAddr) {
		client->proxyAddr = gFskHTTPClient.gHTTPSystemProxyAddr;
		client->proxyPort = gFskHTTPClient.gHTTPSystemProxyPort;
	}
	else {
		client->proxyAddr = gFskHTTPClient.gHTTPProxyAddr;
		client->proxyPort = gFskHTTPClient.gHTTPProxyPort;
	}
	if (!gFskHTTPClient.useconfig && gFskHTTPClient.gHTTPSSystemProxyAddr) {
		client->proxyAddrS = gFskHTTPClient.gHTTPSSystemProxyAddr;
		client->proxyPortS = gFskHTTPClient.gHTTPSSystemProxyPort;
	}
	else {
		client->proxyAddrS = gFskHTTPClient.gHTTPSProxyAddr;
		client->proxyPortS = gFskHTTPClient.gHTTPSProxyPort;
	}
	if( gFskHTTPClient.proxyAuthentication )
		client->proxyAuth = FskStrDoCopy( gFskHTTPClient.proxyAuthentication );
}

void FskHTTPSyncSystemProxy(void) {
	sFskHTTPClientGetSystemProxy(kFskHTTPClientProxy, &gFskHTTPClient.gHTTPSystemProxyAddr, &gFskHTTPClient.gHTTPSystemProxyPort);
	FskInstrumentedItemSendMessage(&gFskHTTPClient, kFskHTTPClientInstrMsgSystemHTTPProxy, &gFskHTTPClient);

	sFskHTTPClientGetSystemProxy(kFskHTTPSClientProxy, &gFskHTTPClient.gHTTPSSystemProxyAddr, &gFskHTTPClient.gHTTPSSystemProxyPort);
	FskInstrumentedItemSendMessage(&gFskHTTPClient, kFskHTTPClientInstrMsgSystemHTTPSProxy, &gFskHTTPClient);
}

static int sHTTPClientInterfacesChangedCallback(struct FskNetInterfaceRecord *iface, UInt32 status, void *param)
{
	switch (status) {
		case kFskNetInterfaceStatusChanged:
		case kFskNetInterfaceStatusNew:
			FskHTTPSyncSystemProxy();
			break;
	}
	return 0;
}

Boolean FskHTTPClientIsIdle(FskHTTPClient client) {
	if (!client)
		return true;
	if ((client->requestState == kHTTPClientIdle)
		&& (client->responseState == kHTTPClientIdle)
		&& (NULL == client->httpRequests)
		&& (NULL == client->httpResponses))
		return true;
	return false;
}

FskErr FskHTTPClientInitialize(void)
{
	if (false == gFskHTTPClientInitialized) {
		FskMemSet(&gFskHTTPClient, 0, sizeof(FskHTTPClientSystemRecord));
		FskInstrumentedItemNew(&gFskHTTPClient, "FskHTTPClient-system", &gFskHTTPClientTypeSystem);

		sFskHTTPClientGetSystemProxy(kFskHTTPClientProxy, &gFskHTTPClient.gHTTPSystemProxyAddr, &gFskHTTPClient.gHTTPSystemProxyPort);
		FskInstrumentedItemSendMessage(&gFskHTTPClient, kFskHTTPClientInstrMsgSystemHTTPProxy, &gFskHTTPClient);

		sFskHTTPClientGetSystemProxy(kFskHTTPSClientProxy, &gFskHTTPClient.gHTTPSSystemProxyAddr, &gFskHTTPClient.gHTTPSSystemProxyPort);
		FskInstrumentedItemSendMessage(&gFskHTTPClient, kFskHTTPClientInstrMsgSystemHTTPSProxy, &gFskHTTPClient);

		gFskHTTPClient.HTTPClientInterfaceNotifierRef = FskNetInterfaceAddNotifier(sHTTPClientInterfacesChangedCallback, NULL, "FskHTTPClient-global proxy tracking");
		gFskHTTPClientInitialized = true;
	}
	return kFskErrNone;
}

FskErr FskHTTPClientTerminate(void)
{
	if (gFskHTTPClientInitialized) {
		FskNetInterfaceRemoveNotifier(gFskHTTPClient.HTTPClientInterfaceNotifierRef);
		gFskHTTPClient.HTTPClientInterfaceNotifierRef = NULL;
		FskInstrumentedItemDispose(&gFskHTTPClient);
		FskMemSet(&gFskHTTPClient, 0, sizeof(FskHTTPClientSystemRecord));
	}
	return kFskErrNone;
}

/* ---------------------------------------------------------------------- */
void sFskHTTPClientUpUse(FskHTTPClient client) {
	client->useCount++;
}
void sFskHTTPClientDownUse(FskHTTPClient client) {
	if (NULL != client) {
		client->useCount--;
		if (client->useCount < 1) {
			sFskHTTPClientDispose(client);
		}
	}
}

/* ---------------------------------------------------------------------- */
static int sFskHTTPClientSeq = 0;
FskErr FskHTTPClientNewPrioritized(FskHTTPClient *clientOut, int priority, char *name)
{
	FskErr	err;
	FskHTTPClient client;

	client = NULL;

	err = FskMemPtrNewClear(sizeof(FskHTTPClientRecord), &client);
	BAIL_IF_ERR(err);
	client->seqNum = sFskHTTPClientSeq++;

	FskNetInitialize();

	sFskHTTPClientUpUse(client);

	client->priority = priority;

	client->requestState = kHTTPClientInitialize;
	client->responseState = kHTTPClientInitialize;

	BAIL_IF_ERR(err = FskMemPtrNew(kFskHTTPClientTransferBufferSize, (FskMemPtr*)(void*)&client->inputBuf));
	client->inputBufNeedsDisposal = true;

	BAIL_IF_ERR(err = FskMemPtrNew(kFskHTTPClientTransferBufferSize, (FskMemPtr*)(void*)&client->outputBuf));
	client->outputBufNeedsDisposal = true;

	FskTimeCallbackNew(&client->cycleCallback);

	sSetClientProxy(client);

	client->requestSuspendedState = kHTTPClientNotSuspended;
	client->responseSuspendedState = kHTTPClientNotSuspended;
	client->idleKillSecs = kFskHTTPClientKillTimerSecs;

	client->interfaceChangeNotifier = FskNetInterfaceAddNotifier(clientInterfaceChangedCB, client, "http client");

	client->name = FskStrDoCopy(name);
	FskInstrumentedItemNew(client, client->name, &gFskHTTPClientTypeInstrumentation);
	*clientOut = client;

	FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgNew, client);

bail:
	if (kFskErrNone != err) {
		sFskHTTPClientDispose(client);
		*clientOut = NULL;
	}

	return err;
}

/* ---------------------------------------------------------------------- */
static FskErr sFskHTTPClientDispose(FskHTTPClient client)
{
	if (NULL == client) goto bail;

	if (client->waitingForSocket) {
		FskInstrumentedItemSendMessageDebug(client, kFskHTTPClientRequestInstrMsgErrString,
					"Someone disposed of the client while we were waiting for the socket. Mark it as disposed so it'll get cleaned up later");
		client->disposed = true;
		goto bail;
	}

	if (client->callingFinish)
		goto bail;

	FskNetInterfaceRemoveNotifier(client->interfaceChangeNotifier);
	client->interfaceChangeNotifier = NULL;

	FskTimeCallbackDispose(client->cycleCallback);
	client->cycleCallback = NULL;
	FskTimeCallbackDispose(client->killTimer);
	client->killTimer = NULL;
	FskThreadRemoveDataHandler(&client->writeDataHandler);
	FskThreadRemoveDataHandler(&client->readDataHandler);

	FskNetSocketClose(client->skt);
	if (client->inputBufNeedsDisposal) {
		client->inputBufNeedsDisposal = false;
		FskMemPtrDispose(client->inputBuf);
		client->inputBuf = NULL;
	}
	if (client->outputBufNeedsDisposal) {
		client->outputBufNeedsDisposal = false;
		FskMemPtrDispose(client->outputBuf);
		client->outputBuf = NULL;
	}

	while (client->httpRequests) {
		FskHTTPClientRequestDispose(client->httpRequests);
	}
	while (client->httpResponses) {
		FskHTTPClientRequestDispose(client->httpResponses);
	}

	if (!client->calledFinish && client->finishedCB) {
		client->callingFinish = true;
		(*client->finishedCB)(client, client->refCon);
		client->calledFinish = true;
	}

	FskHTTPAuthDispose(client->auth);
	FskMemPtrDispose(client->cert.certificates);
	FskMemPtrDispose(client->cert.policies);
	FskMemPtrDispose(client->proxyAuth);

	FskInstrumentedItemDispose(client);
	FskMemPtrDispose(client->host);
	FskMemPtrDispose(client->name);
	FskMemPtrDispose(client);
	FskNetTerminate();

bail:
	return kFskErrNone;
}

FskErr FskHTTPClientDispose(FskHTTPClient client)
{
	FskHTTPClientRequest req;

	if (client) {
		// clear out callbacks to prevent unexpected surprises
		req = client->httpRequests;
		if (req) {
			req->reqFinishedCB = NULL;
			req->receivedDataCB = NULL;
			req->responseHeadersCB = NULL;
			req->sendDataCB = NULL;
		}
		req = client->httpResponses;
		if (req) {
			req->reqFinishedCB = NULL;
			req->receivedDataCB = NULL;
			req->responseHeadersCB = NULL;
			req->sendDataCB = NULL;
		}
		client->authCB = NULL;
		client->finishedCB = NULL;
		sFskHTTPClientDownUse(client);
	}
	return kFskErrNone;
}

/* ---------------------------------------------------------------------- */
static void sClientKillMe(FskTimeCallBack cb, FskTime when, void *param) {
	FskHTTPClient client = (FskHTTPClient)param;
	if (!client)
		return;

	FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgKillIdle, client);
	client->responseState = kHTTPClientDone;
	client->status.lastErr = kFskErrTimedOut;
	sClientCycle(client);
}

static void sResetKillTimer(FskHTTPClient client) {
	if (!client)
		return;

	if (client->idleKillSecs && !client->killTimer) {
		FskTimeCallbackNew(&client->killTimer);
	}
	if (client->killTimer) {
		if (!client->idleKillSecs)
			FskTimeCallbackRemove(client->killTimer);
		else
			FskTimeCallbackScheduleFuture(client->killTimer, client->idleKillSecs, 0, sClientKillMe, client);
	}
}

void FskHTTPClientSetIdleTimeout(FskHTTPClient client, int secs) {
	if (!client)
		return;
	client->idleKillSecs = secs;
	sResetKillTimer(client);
}


static int sProtocolToNum(char *protocol) {
	int ret;
	if (0 == FskStrCompare(protocol, "http"))
		ret = kFskHTTPProtocolHTTP;
	else if (0 == FskStrCompare(protocol, "https"))
		ret = kFskHTTPProtocolHTTPS;
	else
		ret = kFskHTTPProtocolUnknown;
	return ret;
}

void FskHTTPClientSetRefCon(FskHTTPClient client, void *refCon)
{
	client->refCon = refCon;
}

void FskHTTPClientSetFinishedCallback(FskHTTPClient client, FskHTTPClientFinishedCallback callback)
{
	client->finishedCB = callback;
}

FskErr FskHTTPClientBeginRequest(FskHTTPClient client, FskHTTPClientRequest req)
{
	FskErr err;

	if (!client || !req)
		return kFskErrInvalidParameter;

	if (kFskErrNone != (err = FskHTTPClientAddRequest(client, req)))
		return err;

	return FskHTTPClientBegin(client);
}

FskErr FskHTTPClientBegin(FskHTTPClient client)
{
	FskErr	err;
	int connectionFlags = 0;
	int connectToAddr = 0, connectToPort = 0;

	if (client->calledFinish)
		client->calledFinish = false;

	if ((kHTTPClientIdle != client->requestState)
		&& (kHTTPClientInitialize != client->requestState)) {
		FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgBeginCurrentlyProcessing, client);
		return kFskErrNone;
	}

	if (client->skt && client->skt->pendingClose) {
		FskNetSocketClose(client->skt);
		client->skt = NULL;
		client->sslProxied = false;
	}

	if (client->skt && FskNetSocketIsWritable(client->skt)) {
#if OPEN_SSL
		if (client->sslProxied) {
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgBeginSSLThruProxy, client);
			client->requestState = kHTTPClientConnectingSSL;
			return FskNetSocketDoSSL(client->host, client->skt, (FskNetSocketCreatedCallback)sHTTPClientGotSocket, client);
		}
#else /* if CLOSED_SSL */
		if( client->sslProxied && ( client->protocolNum==kFskHTTPProtocolHTTPS && !client->skt->isSSL) )
		{
			void* ssl = 0;
			FskSocketCertificateRecord cert = {
				NULL, 0,
				client->cert.policies ? client->cert.policies : "allowOrphan",
				client->host,
				NULL, 0,
			};

			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgBeginSSLThruProxy, client);
			client->requestState = kHTTPClientConnectingSSL;

			err = FskSSLAttach( &ssl, client->skt );
			if( kFskErrNone == err )
			{
				FskSSLLoadCerts( ssl, &cert );
				err = FskSSLHandshake( ssl, sHTTPClientGotSocket, client, true );
				if( kFskErrNone == err )
					client->skt->isSSL = true;
			}
			return err;
		}
#endif
		FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgBeginClientAlreadyConnected, client);
		sClientCycle(client);
		return kFskErrNone;
	}

	// Connect to the host (asynchronous)
	client->requestState = kHTTPClientConnecting;
	client->waitingForSocket = true;
	client->interfaceLost = false;
	client->interfaceSeedPreConnect = client->interfaceSeed;
	client->status.lastErr = 0;

#if OPEN_SSL || CLOSED_SSL
	if (client->protocolNum == kFskHTTPProtocolHTTPS) {
		if (client->proxyAddrS) {
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgBeginConnectToSecureProxy, client);
			connectToAddr = client->proxyAddrS;
			connectToPort = client->proxyPortS;
		}
		else {
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgBeginConnectToSecure, client);
			connectionFlags = kConnectFlagsSSLConnection;
		}
	}
	else // not ssl
#endif
	{
		if( client->proxyAddr && !client->isLocal ) {
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgBeginConnectToProxy, client);
			connectToAddr = client->proxyAddr;
			connectToPort = client->proxyPort;
		}
		else {
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgBeginConnect, client);
		}
	}
	client->eligibleForReconnect = false;
	if (0 != connectToAddr) {
		char name[64];
		FskNetIPandPortToString(connectToAddr, 0, name);
		err = FskNetConnectToHostPrioritized(name, connectToPort, false,
			(FskNetSocketCreatedCallback)sHTTPClientGotSocket, client, connectionFlags,
			client->priority, &client->cert, "HTTP Client - proxied");
	}
	else {
		err = FskNetConnectToHostPrioritized(client->host, client->hostPort, false,
			(FskNetSocketCreatedCallback)sHTTPClientGotSocket, client, connectionFlags,
			client->priority, &client->cert, "HTTP Client");
	}

	if (kFskErrWaitingForSocket == err)
		err = kFskErrNone;

	return err;
}


// --------------------------------------------
// will suspend on next cycle
void FskHTTPClientSuspend(FskHTTPClient client)
{
	if (!client || (client->requestSuspendedState != kHTTPClientNotSuspended))
		return;
	client->pendingSuspend = true;
	FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgSuspend, client);
  	FskTimeCallbackScheduleNextRun(client->cycleCallback, sClientRunCycle, client);
}

void FskHTTPClientResume(FskHTTPClient client)
{
	if (!client || (client->requestSuspendedState == kHTTPClientNotSuspended)) {
		if (client)
			client->pendingSuspend = false;
		return;
	}
	sFskHTTPClientUpUse(client);
	client->pendingSuspend = false;
	client->requestState = client->requestSuspendedState;
	client->requestSuspendedState = kHTTPClientNotSuspended;

	client->responseState = client->responseSuspendedState;
	client->responseSuspendedState = kHTTPClientNotSuspended;
	FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResume, client);
  	FskTimeCallbackScheduleNextRun(client->cycleCallback, sClientRunCycle, client);
	sResetKillTimer(client);
	sFskHTTPClientDownUse(client);
}

void FskHTTPClientCancel(FskHTTPClient client)
{
	if (!client || (!client->httpRequests && !client->httpResponses) || client->pendingCancel)
		return;

	sFskHTTPClientUpUse(client);
	if (kHTTPClientConnecting == client->requestState) {
//		FskNetCancelConnection();
	}
	else if (kHTTPClientConnectingSSL == client->requestState) {
	}
	client->pendingCancel = true;
	client->flushUnsatisfiedResponses = true;
 	FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgCancel, client);
 	FskTimeCallbackScheduleNextRun(client->cycleCallback, sClientRunCycle, client);
	sFskHTTPClientDownUse(client);
}


// --------------------------------------------
FskErr FskHTTPClientAddRequest(FskHTTPClient client, FskHTTPClientRequest request)
{
	FskErr err = kFskErrNone;

	if (client->protocolNum) {
		if (request->protocolNum != client->protocolNum)
            BAIL(kFskErrProtocolDoesntMatch);
	}
	if (client->host) {
		if ((0 != FskStrCompareCaseInsensitive(client->host, request->parsedUrl->host))
			|| (client->hostPort != request->parsedUrl->port))
            BAIL(kFskErrHostDoesntMatch);
	}
	else {
		client->host = FskStrDoCopy(request->parsedUrl->host);
        BAIL_IF_NULL(client->host, err, kFskErrMemFull);

		client->hostPort = request->parsedUrl->port;
		if ((0 == FskStrCompare(client->host, "localhost")) ||
			(FskStrIsDottedQuad(client->host, &client->hostIP) && (FskNetIsLocalNetwork(client->hostIP) || FskNetIsLocalAddress(client->hostIP))))
				client->isLocal = true;
	}

	if (!client->protocolNum)
		client->protocolNum = request->protocolNum;

	sFskHTTPClientRequestUpUse(request);
	request->owner = client;
	FskListAppend((FskList*)(void*)&client->httpRequests, request);

	FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgAdd, request);

bail:
	request->status.lastErr = err;
	return err;
}

FskErr FskHTTPClientRemoveRequest(FskHTTPClientRequest request)
{
	FskHTTPClient client;

	if (NULL == request)
		return kFskErrNone;
	FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgRemove, request);
	client = request->owner;
	if (client) {
		FskListRemove((FskList*)(void*)&client->httpRequests, request);
		FskListRemove((FskList*)(void*)&client->httpResponses, request);
		request->owner = NULL;
//@@		sFskHTTPClientRequestDownUse(request);
	}
	return kFskErrNone;
}

FskErr FskHTTPClientFlushRequests(FskHTTPClient client)
{
 	FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgFlushRequests, client);
	return kFskErrNone;
}

FskHTTPClientRequest FskHTTPClientGetActiveRequest(FskHTTPClient client)
{
	if (client)
		if (client->httpRequests)
			return client->httpRequests;
	return NULL;
}

void sFskHTTPClientRequestUpUse(FskHTTPClientRequest req) {
	req->useCount++;
}
void sFskHTTPClientRequestDownUse(FskHTTPClientRequest req) {
	if (NULL != req) {
		req->useCount--;
		if (req->useCount < 1) {
			sFskHTTPClientRequestDispose(req);
		}
	}
}

FskErr FskHTTPClientRequestNew(FskHTTPClientRequest *reqOut, char *url)
{
	FskErr	err;
	FskHTTPClientRequest	req = NULL;

	err = FskMemPtrNewClear(sizeof(FskHTTPClientRequestRecord), &req);
    BAIL_IF_ERR(err);

	FskInstrumentedItemNew(req, NULL, &gFskHTTPClientRequestTypeInstrumentation);

    BAIL_IF_NULL(url, err, kFskErrInvalidParameter);

	err = FskStrParseUrl(url, &req->parsedUrl);
	if ((kFskErrNone != err) || (NULL == req->parsedUrl->scheme) || (NULL == req->parsedUrl->host))
        BAIL(kFskErrInvalidParameter);
	req->respContentLength = kFskHTTPNoContentLength;		// haven't seen content length header
	req->protocolNum = sProtocolToNum(req->parsedUrl->scheme);

	req->requestParameters = FskAssociativeArrayNew();
	err = FskHeaderStructNew(&req->requestHeaders);
	BAIL_IF_ERR(err);
	err = FskHeaderStructNew(&req->responseHeaders);
	BAIL_IF_ERR(err);
	req->eligibleForReconnect = true;
bail:
	if (kFskErrNone == err) {
		if (reqOut)
			*reqOut = req;
	}
	else {
		FskHTTPClientRequestDispose(req);
	}
	return err;
}

FskErr sFskHTTPClientRequestDispose(FskHTTPClientRequest request)
{
	if (NULL == request)
		return kFskErrNone;

	if (request->owner)
		FskHTTPClientRemoveRequest(request);

	FskAssociativeArrayDispose(request->requestParameters);
	FskHeaderStructDispose(request->requestHeaders);
	FskHeaderStructDispose(request->responseHeaders);

	if (request->userBuffer == false)
		FskMemPtrDispose(request->buffer);

	FskInstrumentedItemDispose(request);

	FskStrParsedUrlDispose(request->parsedUrl);
	if (request->methodNeedsDispose)
		FskMemPtrDispose(request->method);

	FskMemPtrDispose(request);

	return kFskErrNone;
}

FskErr FskHTTPClientRequestDispose(FskHTTPClientRequest request)
{
	if (request) {
		request->reqFinishedCB = NULL;
		request->receivedDataCB = NULL;
		request->responseHeadersCB = NULL;
		request->sendDataCB = NULL;
		sFskHTTPClientRequestDownUse(request);
	}
	return kFskErrNone;
}

void FskHTTPClientRequestSetRefCon(FskHTTPClientRequest request, void *refCon)
{
	request->refCon = refCon;
}

FskErr FskHTTPClientRequestSetURL(FskHTTPClientRequest req, char *url)
{
	FskErr	err;

	FskInstrumentedItemSendMessage(req, kFskHTTPClientRequestInstrMsgChangeURL, url);
	err = FskStrParseUrl(url, &req->parsedUrl);
	if ((kFskErrNone != err) || (NULL == req->parsedUrl->scheme) || (NULL == req->parsedUrl->host)) {
		err = kFskErrInvalidParameter;
		return err;
	}
	req->protocolNum = sProtocolToNum(req->parsedUrl->scheme);

	return err;
}

/* ---------------------------------------------------------------------- */
FskErr FskHTTPClientRequestSetURLParts(FskHTTPClientRequest req, char *protocol, char *host, int port, char *target)
{
	FskErr err;
	char *url;
	int size;

	size = FskStrLen(protocol) + 7 + FskStrLen(host) + 6 + FskStrLen(target) + 1;
	if (kFskErrNone != (err = FskMemPtrNew(size, &url)))
		return err;

	if (protocol) {
		if (0 == FskStrCompare("https", protocol)) {
			if (port == 443 || port == 0)
				snprintf(url, size, "https://%s/%s", host, target);
			else
				snprintf(url, size, "https://%s:%d/%s", host, port, target);
		}
		else if (0 == FskStrCompare("http", protocol)) {
			if (port == 80 || port == 0)
				snprintf(url, size, "http://%s/%s", host, target);
			else
				snprintf(url, size, "http://%s:%d/%s", host, port, target);
		}
		else {
			if (port == 0)
				snprintf(url, size, "%s://%s/%s", protocol, host, target);
			else
				snprintf(url, size, "%s://%s:%d/%s", protocol, host, port, target);
		}
	}
	else {
		if (port == 80 || port == 0)
			snprintf(url, size, "http://%s/%s", host, target);
		else
			snprintf(url, size, "http://%s:%d/%s", host, port, target);
	}

	FskHTTPClientRequestSetURL(req, url);
	FskMemPtrDispose(url);
	return kFskErrNone;
}

FskErr FskHTTPClientRequestAddParameter(FskHTTPClientRequest req, char *paramName, char *paramValue)
{
	if (NULL == paramName || NULL == paramValue)
		return kFskErrInvalidParameter;

	FskAssociativeArrayElementSetString(req->requestParameters, paramName, paramValue);
	return kFskErrNone;
}

FskErr FskHTTPClientRequestAddHeader(FskHTTPClientRequest request, char *headerName, char *headerValue)
{
	if (NULL == headerName || NULL == headerValue)
		return kFskErrInvalidParameter;

	FskHeaderAddString(headerName, headerValue, request->requestHeaders);

	return kFskErrNone;
}

FskErr FskHTTPClientRequestRemoveHeader(FskHTTPClientRequest request, char *headerName)
{
	if (NULL == headerName)
		return kFskErrInvalidParameter;

	FskHeaderRemove(headerName, request->requestHeaders);

	return kFskErrNone;
}

void FskHTTPClientRequestSetMethod(FskHTTPClientRequest request, char *method)
{
	if (request->methodNeedsDispose)
		FskMemPtrDispose(request->method);
	request->method = FskStrDoCopy(method);
	request->methodNeedsDispose = true;
}

FskErr FskHTTPClientRequestPost(FskHTTPClientRequest request, char *messageBody, int messageBodyLen)
{
	FskHTTPClientRequestSetRequestBody(request, messageBody, messageBodyLen);
	FskHTTPClientRequestSetMethod(request, "POST");
	return kFskErrNone;
}

void FskHTTPClientRequestSetRequestBody(FskHTTPClientRequest request, char *buffer, int bufferSize)
{
	request->requestBodySize = bufferSize;
	request->requestBodyBuffer = buffer;
	if (buffer && bufferSize > 0) {
		request->staticRequestBuffer = true;
		FskHeaderAddInteger(kFskStrContentLength, bufferSize, request->requestHeaders);
	}
	else {
		request->staticRequestBuffer = false;
		FskHeaderRemove(kFskStrContentLength, request->requestHeaders);
	}
}

void FskHTTPClientRequestSetSendDataCallback(FskHTTPClientRequest request,
							  FskHTTPClientRequestSendDataCallback callback)
{
	request->sendDataCB = callback;
}


void FskHTTPClientRequestSetReceivedResponseHeadersCallback(FskHTTPClientRequest request,
						FskHTTPClientRequestReceivedResponseHeadersCallback callback,
						UInt32 flags)
{
	request->responseHeadersCB = callback;
	request->responseHeadersFlags = flags;
}

FskErr FskHTTPClientRequestSetReceivedDataCallback(FskHTTPClientRequest request,
								 FskHTTPClientRequestReceivedDataCallback callback,
								 char *buffer, int bufferSize, FskHTTPClientReadEnum mode)
{
	FskErr err = kFskErrNone;

	switch (mode) {
		case kFskHTTPClientReadAllData:		// library manages buffer
			request->userBuffer = false;
			BAIL_IF_ERR(err = FskMemPtrNew(kFskHTTPClientTransferBufferSize, (FskMemPtr*)(void*)&request->buffer));
			request->bufferSize = kFskHTTPClientTransferBufferSize;
			request->bufferPos = 0;
			request->bufferAmt = 0;
			break;
		case kFskHTTPClientReadAnyData:		// user manages buffer
            BAIL_IF_ZERO(bufferSize, err, kFskErrInvalidParameter); // need to know how big a buffer we're using
			if (NULL == buffer) {
				request->userBuffer = false;
                BAIL_IF_ERR(err = FskMemPtrNew(bufferSize, (FskMemPtr*)(void*)&request->buffer));
				request->bufferSize = bufferSize;
			}
			else {
				request->userBuffer = true;
				request->buffer = buffer;
				request->bufferSize = bufferSize;
			}
			request->bufferAmt = 0;
			request->bufferPos = 0;
			break;
		default:
            BAIL(kFskErrInvalidParameter);
	}

	request->receivedDataCB = callback;
	request->receivedDataCBMode = mode;

bail:
	return err;
}

void FskHTTPClientRequestSetFinishedCallback(FskHTTPClientRequest request,
								  FskHTTPClientRequestFinishedCallback callback)
{
	request->reqFinishedCB = callback;
}

void FskHTTPClientSetAuthCallback(FskHTTPClient client,
								FskHTTPClientAuthCallback callback)
{
	client->authCB = callback;
}


/* ---------------------------------------------------------------------- */
FskErr FskHTTPClientGetAuth(FskHTTPClient client, FskHTTPClientRequest req, char *url, char *realm, FskHTTPAuth auth)
{
	// replace the existing realm with the new one
	FskMemPtrDispose(auth->realm);
	auth->realm = FskStrDoCopy(realm);
	if (NULL == auth->realm)
		return kFskErrMemFull;
	auth->waitingForPassword = true;

	// if this is the first attempt, use the name/pw from the original URL
	if (1 == auth->authAttempts) {
		if ((kFskHTTPAuthCredentialsTypeNone == auth->credentialsType)
			&& ((0 != FskStrLen(req->parsedUrl->username))
			&&  (0 != FskStrLen(req->parsedUrl->password))) ) {
			FskInstrumentedItemSendMessage(client->auth, kFskHTTPClientAuthInstrMsgCredFromURL, req);
			FskMemPtrDispose(auth->username);
			auth->username = FskStrDoCopy(req->parsedUrl->username);
			FskMemPtrDispose(auth->credentials);
			auth->credentials = FskStrDoCopy(req->parsedUrl->password);
			auth->waitingForPassword = false;
			auth->credentialsType = kFskHTTPAuthCredentialsTypeString;
			return kFskErrNone;
		}
        else if (((auth->authType == kFskHTTPAuthBasic)
			|| (auth->authType == kFskHTTPAuthDigest))
            && (auth->credentialsType == kFskHTTPAuthCredentialsTypeString)) {
			auth->waitingForPassword = false;
			return kFskErrNone;
		}
		else if ((auth->authType == kFskHTTPAuthDigest)
			&& (auth->credentialsType == kFskHTTPAuthCredentialsTypeDigest)) {
			auth->waitingForPassword = false;
			return kFskErrNone;
		}
	}

	// or call up to the app
	if (NULL != client->authCB) {
		FskErr err = kFskErrNone;
		err = client->authCB(client, req, url, auth->realm, auth, req->refCon);
		FskInstrumentedItemSendMessage(client->auth, kFskHTTPClientAuthInstrMsgCredFromCallback, req);
		return err;
	}

	// or check a database, or ....

	return kFskErrAuthFailed;
}

/* ---------------------------------------------------------------------- */
FskErr FskHTTPAuthNew(FskHTTPAuth *authOut) {
	FskErr err = kFskErrNone;
	FskHTTPAuth	auth = NULL;

	err = FskMemPtrNewClear(sizeof(FskHTTPAuthRecord), &auth);
	BAIL_IF_ERR(err);

	auth->nc = 13;
	FskStrNumToHex(FskRandom(), auth->cnonce, 8);

	FskInstrumentedItemNew(auth, NULL, &gFskHTTPClientAuthTypeInstrumentation);

bail:
	*authOut = auth;
	return err;
}

FskErr FskHTTPAuthDispose(FskHTTPAuth auth) {
	if (auth) {
		FskInstrumentedItemDispose(auth);
		FskMemPtrDispose(auth->realm);
		FskMemPtrDispose(auth->username);
		FskMemPtrDispose(auth->credentials);
		FskMemPtrDispose(auth->nonce);
		FskMemPtrDispose(auth->qop);
		FskMemPtrDispose(auth->alg);
		FskMemPtrDispose(auth->opaque);
		FskMemPtrDispose(auth);
	}
	return kFskErrNone;
}

FskErr FskHTTPClientSetCredentials(FskHTTPClient client, char *username, char *credentials, int credentialsSize, int credentialsType) {
	FskErr err = kFskErrInvalidParameter;
	FskHTTPAuth auth ;

	if (kFskHTTPAuthCredentialsTypeNone == credentialsType) {
		FskHTTPAuthDispose(client->auth);
		client->auth = NULL;
		return kFskErrNone;
	}

	if (NULL == client->auth) {
		err = FskHTTPAuthNew(&client->auth);
		if (err) return err;
	}
	auth = client->auth;

	auth->credentialsType = credentialsType;
	auth->credentialsSize = credentialsSize;

	FskMemPtrDispose(auth->username);
	auth->username = FskStrDoCopy(username);
	FskMemPtrDispose(auth->credentials);
	switch (credentialsType) {
		case kFskHTTPAuthCredentialsTypeString:
            BAIL_IF_ERR(err = FskMemPtrNew(FskStrLen(credentials) + 1, (FskMemPtr*)(void*)&auth->credentials));
			FskStrCopy(auth->credentials, credentials);
			FskInstrumentedItemSendMessage(client, kFskHTTPClientAuthInstrMsgCredFromAPI, client);
			break;
		case kFskHTTPAuthCredentialsTypeDigest:
		default:
            BAIL_IF_ERR(err = FskMemPtrNew(credentialsSize, (FskMemPtr*)(void*)&auth->credentials));
			FskMemCopy(auth->credentials, credentials, credentialsSize);
			break;
	}
	auth->tryPassword = true;
	err = kFskErrNone;

bail:
	auth->waitingForPassword = false;
	return err;
}

FskErr FskHTTPClientSetCertificates(FskHTTPClient client, const void *data, int dataSize, const char *policies)
{
	FskErr err;

	if (client->cert.certificates != NULL)
		FskMemPtrDispose(client->cert.certificates);
	client->cert.certificates = NULL;
	client->cert.certificatesSize = dataSize;
	if (data != NULL) {
		if ((err = FskMemPtrNewFromData(dataSize, data, &client->cert.certificates)) != kFskErrNone)
			return err;
	}
	if (policies != NULL) {
		if ((client->cert.policies = FskStrDoCopy(policies)) == NULL)
			return kFskErrMemFull;
	}
	else
		client->cert.policies = NULL;
	client->cert.hostname = NULL;

	return kFskErrNone;
}

/* ---------------------------------------------------------------------- */
FskErr FskHTTPClientRequestAddAuthorization(FskHTTPClient client, FskHTTPClientRequest req)
{
	FskErr err = kFskErrNone;
	int amt;
	char authString[1024];

	if (!client->auth) {
		return kFskErrOperationFailed;
	}

	if (kFskHTTPAuthDigest == client->auth->authType) {
		char *method;
		HASHHEX HA1, HA2, response;
		char nonceCount[9];
		int ret;

		FskInstrumentedItemSendMessage(client->auth, kFskHTTPClientAuthInstrMsgAddToReq, client->auth);
		if (kFskHTTPAuthCredentialsTypeString == client->auth->credentialsType) {
			char *pw = client->auth->credentials;
			if (kFskErrNone == (err = FskHTTPAuthCalculateUsernamePasswordHash(client->auth->username, client->auth->realm, pw, &client->auth->credentials))) {
				client->auth->credentialsType = kFskHTTPAuthCredentialsTypeDigest;
				client->auth->credentialsSize = HASHLEN;
			}
			FskMemPtrDispose(pw);
			if (err) return err;
		}
		if (kFskHTTPAuthCredentialsTypeDigest != client->auth->credentialsType)
            BAIL(kFskErrAuthFailed);

		if (client->auth->useCnonce) {
			FskStrNumToHex(client->auth->nc, nonceCount, 8);
		}

		method = FskHeaderMethod(req->requestHeaders);
		if (NULL == method) method = req->method;
		if (NULL == method) method = "GET";
		FskHTTPAuthDigestCalcHA1(client->auth->alg,
				client->auth->credentials, client->auth->nonce,
				client->auth->cnonce, HA1);
		FskHTTPAuthDigestCalcResponse(HA1,
				client->auth->nonce, nonceCount,
				client->auth->cnonce,
				client->auth->qop, method,
				req->parsedUrl->url, HA2, response);

		ret = snprintf(authString, 1024, "Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\",",
				client->auth->username,
				client->auth->realm,
				client->auth->nonce, req->parsedUrl->url);
		BAIL_IF_NEGATIVE(ret, err, kFskErrOperationFailed);
		amt = ret;
			
		if (client->auth->useCnonce) {
			ret = snprintf(&authString[amt], 1024-amt, " qop=\"%s\", nc=%s, cnonce=\"%s\",", client->auth->qop, nonceCount, client->auth->cnonce);
			BAIL_IF_NEGATIVE(ret, err, kFskErrOperationFailed);
			amt += ret;	
		}
		ret = snprintf(&authString[amt], 1024-amt, " response=\"%s\"", response);
		BAIL_IF_NEGATIVE(ret, err, kFskErrOperationFailed);
		amt += ret;

		if (NULL != client->auth->opaque) {
			ret = snprintf(&authString[amt], 1024-amt, " , opaque=\"%s\"", client->auth->opaque);
			BAIL_IF_NEGATIVE(ret, err, kFskErrOperationFailed);
			amt += ret;
		}

		FskHeaderRemove("Authorization", req->requestHeaders);
		FskHTTPClientRequestAddHeader(req, "Authorization", authString);
	}
	else if ( ((kFskHTTPAuthNone == client->auth->authType) && (client->auth->tryPassword))
		|| (kFskHTTPAuthBasic == client->auth->authType)) {
		char *val = NULL;

		if (!client->auth->sessionKeyValid) {
			amt = FskStrLen(client->auth->username) + FskStrLen(client->auth->credentials) + 2;
			err = FskMemPtrNew(amt, &val);
			BAIL_IF_ERR(err);
			snprintf(val, amt, "%s:%s", client->auth->username, client->auth->credentials);
			FskMemPtrDispose(client->auth->opaque);
			FskStrB64Encode(val, amt - 1, &client->auth->opaque, NULL, false);
			FskMemPtrDispose(val);
		}
		if (client->auth->opaque) {
			FskInstrumentedItemSendMessage(client->auth, kFskHTTPClientAuthInstrMsgAddToReq, client->auth);
			snprintf(authString, 1024, "Basic %s", client->auth->opaque);
			FskHeaderRemove("Authorization", req->requestHeaders);
			FskHTTPClientRequestAddHeader(req, "Authorization", authString);
		}
	}
bail:
	return err;
}

/* ---------------------------------------------------------------------- */
FskErr sProcessAuthResponse(FskHTTPClient client, FskHTTPClientRequest req)
{
	char	*hdr, *val;
	char	*realm = NULL;
	int		authType;
	FskErr	err = kFskErrNone;
	FskAssociativeArray	authParams = NULL;

	hdr = FskHeaderFind("WWW-Authenticate", req->responseHeaders);
	if (NULL == hdr)
		goto bail;
	err = FskHTTPAuthParseWWWAuthenticate(hdr, &authType, &authParams);
	BAIL_IF_ERR(err);

	if (!client->auth)
		FskHTTPAuthNew(&client->auth);

	client->auth->authType = authType;
	client->auth->authAttempts++;
	FskInstrumentedItemSendMessage(client->auth, kFskHTTPClientAuthInstrMsgChallenge, req);

#if CHECK_AGAINST_TOO_MANY_AUTH_REQUESTS
	if (client->auth->authAttempts > kHTTPAuthAttemptsMax) {
		FskInstrumentedItemSendMessage(client->auth, kFskHTTPClientAuthInstrMsgTooManyFailures, req);
        BAIL(kFskErrAuthFailed);
	}
#endif

	if (client->auth->authType == kFskHTTPAuthDigest) {
		char *nonce, *alg;
		char *qop;
		char nonceCount[9];

		client->auth->useCnonce = false;

		nonce = realm = alg = qop = NULL;
		val = FskAssociativeArrayElementGetString(authParams, "realm");
		if (val) {
			realm = FskStrDoCopy(val);
			FskStrStripQuotes(realm);
		}

		val = FskAssociativeArrayElementGetString(authParams, "algorithm");
		if (val) {
			alg = FskStrDoCopy(val);
			FskMemPtrDispose(client->auth->alg);
			client->auth->alg = alg;
		}

		val = FskAssociativeArrayElementGetString(authParams, "qop");
		if (val) {
			qop = FskStrDoCopy(val);
			FskStrStripQuotes(qop);
			FskMemPtrDispose(client->auth->qop);
			client->auth->qop = qop;
		}

		if (0 != FskStrLen(client->auth->qop)) {
			client->auth->useCnonce = true;
			client->auth->nc++;
			FskStrNumToHex(client->auth->nc, nonceCount, 8);
		}
		else {
			client->auth->useCnonce = false;
			client->auth->nc = 0;
			client->auth->cnonce[0] = 0;
		}

		if (client->auth->opaque)
			FskMemPtrDispose(client->auth->opaque);
		client->auth->opaque = FskAssociativeArrayElementGetString(authParams, "opaque");
		if (client->auth->opaque) {
			client->auth->opaque = FskStrDoCopy(client->auth->opaque);
			FskStrStripQuotes(client->auth->opaque);
		}

		val = FskAssociativeArrayElementGetString(authParams, "nonce");
		nonce = FskStrDoCopy(val);
		FskStrStripQuotes(nonce);
		if (0 != FskStrCompare(nonce, client->auth->nonce)) {
			client->auth->sessionKeyValid = false;
			FskMemPtrDispose(client->auth->nonce);
			client->auth->nonce = nonce;
			nonce = NULL;
			err = FskHTTPClientGetAuth(client, req, req->parsedUrl->url, realm, client->auth);
			BAIL_IF_ERR(err);
		}

		if (!client->auth->sessionKeyValid) {
			if (client->auth->credentialsType != kFskHTTPAuthCredentialsTypeDigest) {
				if (kFskHTTPAuthCredentialsTypeString == client->auth->credentialsType) {
					char *pw = client->auth->credentials;
					if (kFskErrNone == (err = FskHTTPAuthCalculateUsernamePasswordHash(client->auth->username, client->auth->realm, pw, &client->auth->credentials))) {
						client->auth->credentialsType = kFskHTTPAuthCredentialsTypeDigest;
						client->auth->credentialsSize = HASHLEN;
					}
					FskMemPtrDispose(pw);
					BAIL_IF_ERR(err);
				}
			}
			if (!client->auth->credentials)
				err = kFskErrAuthFailed;
			else {
				FskHTTPAuthDigestCalcHA1(client->auth->alg, client->auth->credentials,
				client->auth->nonce, client->auth->cnonce,
				client->auth->sessionKey);
			FskInstrumentedItemSendMessage(client->auth, kFskHTTPClientAuthInstrMsgGeneratingSessionKey, req);
			}
		}

		client->auth->challenged = true;
		client->auth->tryPassword = true;
		req->flushAndReissue = true;

		FskMemPtrDispose(nonce);
	}
	else if (client->auth->authType == kFskHTTPAuthBasic) {
		val = FskAssociativeArrayElementGetString(authParams, "realm");
		realm = FskStrDoCopy(val);
		if (!realm) goto bail;
		FskStrStripQuotes(realm);

		if (!client->auth->tryPassword) {
			err = FskHTTPClientGetAuth(client, req, req->parsedUrl->url, realm, client->auth);
			BAIL_IF_ERR(err);
			client->auth->tryPassword = true;
		}

		req->flushAndReissue = true;
	}
	else {
		err = kFskErrAuthFailed;
	}

bail:
	if (kFskErrAuthPending == err) {
		req->flushAndReissue = true;
		req->suspend = true;
	}

	if (authParams)
		FskAssociativeArrayDispose(authParams);
	FskMemPtrDispose(realm);
	return err;
}

/* ---------------------------------------------------------------------- */
static FskErr sProcessResponseHeaders(FskHTTPClient client, FskHTTPClientRequest req)
{
	char	*hdr;
	int		responseCode;
	Boolean	skipHeaderCallback = false;
	FskErr	err;

	responseCode = FskHeaderResponseCode(req->responseHeaders);
	FskInstrumentedItemSendMessage(req, kFskHTTPClientRequestInstrMsgServerResponse, (void *)responseCode);

	switch (responseCode) {
		case 154:
			responseCode = 100;
			break;
		default:
			break;
	}
	switch (responseCode) {
		case 304:
			client->responseState = kHTTPClientRequestDone;
			break;
		case 401:
			err = sProcessAuthResponse(client, req);
			if (req->flushAndReissue)
				skipHeaderCallback = true;
			break;
		case 301:
		case 302:
		case 303:
		case 305:
		case 307:	// the HTTP 1.1 RFC states that we MUST NOT automatically redirect (well, not quite...)
			{
			if (++req->redirectTimes > kMaxAutoRedirectTimes) {
				responseCode = 503;
				break;
			}

			if ((302 == responseCode)
				&& (NULL != req->method)		// no method implies GET
				&& (0 != FskStrCompareCaseInsensitive(req->method, "HEAD"))
				&& (0 != FskStrCompareCaseInsensitive(req->method, "GET"))) {
				break;		// don't auto-redirect a 302 that's not HEAD or GET
			}

			hdr = FskHeaderFind(kFskStrLocation, req->responseHeaders);
			if (hdr) {
				FskStrParsedUrl comp = NULL;

				if ((kHTTPClientResponseHeadersOnRedirect & req->responseHeadersFlags) && (NULL != req->responseHeadersCB)) {
					skipHeaderCallback = true;
					if ((err = (*req->responseHeadersCB)(req, req->responseHeaders, (void*)req->refCon))) {
						if (kFskErrOperationCancelled == err) {
							// some redirects can contain data. the request has been canceled, so don't issue any callbacks about that data. just finish.
							client->responseState = kHTTPClientRequestDone;
							req->receivedDataCB = NULL;
						}
						break;
					}
				}
				// we can't just add a new request to the client if the
				// redirected location is on a different server, if so
				// just punt and let the client's user deal with it.
//				FskStrParseUrl(('/' == hdr[0]) ? hdr + 1 : hdr, &comp);
				FskStrParseUrl(hdr, &comp);
				if (comp->valid) {
					if (NULL == comp->host)
						FskStrParsedUrlSetHost(comp, req->parsedUrl->host);

					if (0 == FskStrCompare(comp->host, req->parsedUrl->host)) {
						FskInstrumentedItemSendMessage(req, kFskHTTPClientRequestInstrMsgChangeURL, comp->path);
						req->flushAndReissue = true;
						FskStrParsedUrlDispose(req->parsedUrl);
						req->parsedUrl = comp;
						comp = NULL;
						skipHeaderCallback = true;
					}
				}
				if (comp)
					FskStrParsedUrlDispose(comp);
			}
			break;
		}
		break;
	}

	if( (!client->sslProxied) && (!client->isLocal) &&
		( ((client->protocolNum == kFskHTTPProtocolHTTPS) && client->proxyAddrS) ||
				((client->protocolNum == kFskHTTPProtocolHTTP) && client->proxyAddr ) ) )
	{
		if (responseCode == 200) {
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgSecureProxyConnected, client);

			client->sslProxied = true;
			client->waitingForSocket = true;		// make sure we make SSL connect through proxy
			req->flushAndReissue = true;
			skipHeaderCallback = true;
			client->responseState = kHTTPClientRequestDone;
			FskMemSet(&req->status, 0, sizeof(FskHTTPClientRequestStatus));
			return kFskErrNone;
		}
	}

	if ( !skipHeaderCallback ) {
		if (req->responseHeadersCB) {
			err = (*req->responseHeadersCB)(req, req->responseHeaders, req->refCon);
			if (kFskErrOperationCancelled == err)
				err = kFskErrNone;
		}
	}

	switch (responseCode) {
		case 304:		// not modified
			return responseCode;
		default:
			if ((responseCode / 100) == 1) {
				if (req->expectContinue)
					client->responseState = kHTTPClientPrepareBody;
				else
					client->responseState = kHTTPClientReadResponseHeaders;
				req->expectContinue = false;
				client->inputBufPos = 0;
				FskHeaderStructDispose(req->responseHeaders);
				FskHeaderStructNew(&req->responseHeaders);
				return kFskErrNone;
			}
	}

	if (kHTTPResponseSuccessfulType == FskHeaderResponseCategory(responseCode)) {
		if (client->auth) {
			client->auth->tryPassword = false;
			client->auth->sessionKeyValid = true;
			if (client->auth->challenged) {
				FskInstrumentedItemSendMessage(client->auth, kFskHTTPClientAuthInstrMsgSessionKeyAccepted, req);
				client->auth->challenged = false;
			}

			hdr = FskHeaderFind("Authentication-Info", req->responseHeaders);
			if (hdr) {
				FskAssociativeArray faa;
				if (kFskErrNone == FskHTTPAuthParseAuthenticationInfo(hdr, &faa)) {
					// Authentication-Info: rspauth="0f037737047b3bb19671b7d97257db8d", cnonce="11560EBD", nc=00000001, qop=auth
					// compare cnonce here?
					// what do we do with rspauth
					hdr = FskAssociativeArrayElementGetString(faa, "nc");
					if (hdr) {
						client->auth->nc = FskStrHexToNum(hdr, 8) + 1;
					}
				}
				FskAssociativeArrayDispose(faa);
			}
		}
	}

	client->responseState = kHTTPClientPrepareForResponseBody;

	// if the response has a content-length, then read the data
	req->respContentLength = kFskHTTPNoContentLength;
	hdr = FskHeaderFind(kFskStrContentLength, req->responseHeaders);
	if (hdr) {
		req->respContentLength = FskStrToFskInt64(hdr);
		req->transferEncoding = kFskTransferEncodingNone;
	}

	if (responseCode == 204)
		req->respContentLength = 0;

	// If there's a transfer encoding header, there wont be a content
	// length -- but there will be a message body
	hdr = FskHeaderFind(kFskStrTransferEncoding, req->responseHeaders);
	if (hdr && (FskStrCompareCaseInsensitiveWithLength(hdr, kFskStrChunked, FskStrLen(kFskStrChunked)) == 0)) {
		req->transferEncoding = kFskTransferEncodingChunked;
		req->currentChunkSize = 0;
		req->chunksFinished = false;
	}
	else if (hdr) {
		FskInstrumentedItemSendMessage(req, kFskHTTPClientRequestInstrMsgBadTE, hdr);
		client->responseNextState = kHTTPClientDone;	// don't know how to do the other TE
	}

	hdr = FskHeaderFind(kFskStrProxyConnection, req->responseHeaders);
	if (!hdr)
		hdr = FskHeaderFind(kFskStrConnection, req->responseHeaders);
	if (hdr && (0 == FskStrCompareCaseInsensitiveWithLength(hdr, kFskStrClose, FskStrLen(kFskStrClose)))) {
		client->dontPipeline = true;
		req->closeOnDone = true;
		if ((req->transferEncoding != kFskTransferEncodingChunked) && (req->protocolNum != kFskHTTPProtocolHTTPS))
			;
		else
			client->readUntilServerCloses = true;
	}

	if ((req->transferEncoding == kFskTransferEncodingNone)
		&& (req->respContentLength == kFskHTTPNoContentLength)) {
		client->readUntilServerCloses = true;
	}

	if (0 == FskStrCompare("HEAD", req->method)) {
		// bypass the body processing content
		client->responseState = kHTTPClientRequestDone;
	}

	return kFskErrNone;
}


/* ---------------------------------------------------------------------- */
static int clientInterfaceChangedCB(struct FskNetInterfaceRecord *iface, UInt32 status, void *param)
{
	FskHTTPClient client = (FskHTTPClient)param;

// is this for me?
	if (client->skt && (client->skt->ipaddrLocal != 0) && (iface->ip & iface->netmask) != (client->skt->ipaddrLocal & iface->netmask)) {
		FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString, "clientInterfaceChangedCB - not for our IP");
		return 0;
	}

	switch (status) {
		case kFskNetInterfaceStatusChanged:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString, "clientInterfaceChangedCB - kFskNetInterfaceStatusChanged");
			client->interfaceSeed++;
			break;
		case kFskNetInterfaceStatusNew:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString, "clientInterfaceChangedCB - kFskNetInterfaceStatusNew");
			client->interfaceSeed++;
			break;
		case kFskNetInterfaceStatusRemoved:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString, "clientInterfaceChangedCB - kFskNetInterfaceStatusRemoved");
			client->interfaceSeed++;
			client->interfaceLost = true;
			if (client->skt)
				client->skt->pendingClose = true;

			if (client->responseState == kHTTPClientIdle) {
				if (client->requestState == kHTTPClientIdle) {
					FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString, " interface removed, client in a request state");
					FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRequestState, client);
				}
				else {
					FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString, " interface removed, client in a idle state");
				}
			}
			else {
				FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString, " interface removed, client in a response state");
				FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
				// error causes state to transition to failed
			}
			FskTimeCallbackScheduleNextRun(client->cycleCallback, sClientRunCycle, client);
			break;
	}
	return 0;
}


/* ---------------------------------------------------------------------- */
static FskErr clientReceivedData(FskHTTPClientRequest req, char *buffer, int amt)
{
	FskErr err = kFskErrNone;
#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListeners(req)) {
		FskHTTPClientRequestInstrMsgDataRecord msg;
		msg.buffer = buffer;
		msg.amt = amt;
		FskInstrumentedItemSendMessage(req, kFskHTTPClientRequestInstrMsgRecvData, &msg);
	}
#endif
	if (req->receivedDataCB) {
		err = (*req->receivedDataCB)(req, buffer, amt, req->refCon);
	}
	if (req->owner && req->owner->pendingCancel)
		err = kFskErrOperationCancelled;

	return err;
}

static FskErr sClientCycleRequest(FskHTTPClient client)
{
	FskErr err = kFskErrNone;
	FskErr retVal = kFskErrNeedMoreTime;
	int amt, ret;
	FskHTTPClientRequest request, req;

	request = client->httpRequests;
	if (NULL == request) {
		client->requestState = kHTTPClientIdle;
		if (client->interfaceLost)
			client->status.lastErr = kFskErrNetworkInterfaceRemoved;
		return kFskErrNone;
	}
	sFskHTTPClientRequestUpUse(request);

	if (client->interfaceLost) {
		FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString,
			"in CycleRequest - interface is lost. current state:");
		FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRequestState, client);
		request->closeOnDone = true;
		client->status.lastErr = kFskErrNetworkInterfaceRemoved;
		goto doNetworkError;
	}

	switch (client->requestState) {
		case kHTTPClientSendRequest:	// Write to the connection
sendRequest:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRequestState, client);
			amt = client->outputBufAmt - client->outputBufPos;
 			if (amt < 0) {
				FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgErrString,
					"SendDataChunk -- failed amt went negative");
				err = kFskErrOperationFailed;
				goto doError;
			}
			else if (amt == 0) {
				client->requestState = client->requestNextState;
			}
			else {
				if (client->needsWritable) {
					retVal = kFskErrNone;		// bail out until it's writable
					goto done;
				}
				err = FskNetSocketSendTCP(client->skt, client->outputBuf + client->outputBufPos, amt, &ret);
#if SUPPORT_INSTRUMENTATION
				if (FskInstrumentedItemHasListeners(request)) {
					FskHTTPClientRequestInstrMsgDataRecord msg;
					msg.buffer = client->outputBuf + client->outputBufPos;
					msg.amt = ret;
					msg.err = err;
					FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgSentRequest, &msg);
				}
#endif
				if (kFskErrNone == err) {
					sResetKillTimer(client);
					client->outputBufPos += ret;
					client->outputBufAmt -= ret;
					request->status.bytesSent += ret;
					client->status.bytesSent += ret;
					if (client->outputBufAmt == 0) {
						client->requestState = client->requestNextState;
						client->outputBufPos = 0;
					}
				}
				else if (kFskErrNoData == err) {
					client->needsWritable = true;
					retVal = kFskErrSocketFull;
				}
				else {
					FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgErrString,
						"SendDataChunk -- send_tcp failed");
					client->status.lastErr = err;
					client->responseState = kHTTPClientSocketError;
					goto doNetworkError;
				}
			}
			if (client->pendingCancel)
				break;
			if (client->requestState == kHTTPClientPrepareForResponse)
				goto prepareForResponse;
			if (client->requestState != kHTTPClientPrepareBody)
				break;

		case kHTTPClientPrepareBody:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRequestState, client);
			if (request->staticRequestBuffer) {
				client->requestState = kHTTPClientPrepareForResponse;
			}
			else if (request->sendDataCB) {
				if (request->expectContinue) {
					FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgErrString, " -- in PrepareBody - expecting a CONTINUE\n");
					client->requestState = kHTTPClientPrepareForResponse;
				}
				else {
					err = (*request->sendDataCB)(request, &request->requestBodyBuffer, &request->requestBodyBufferAmt, request->requestBodyPos, request->refCon);
					if (err == kFskErrNone) {
						request->requestBodyBufferPos = 0;
						request->requestBodyPos += request->requestBodyBufferAmt;
						client->requestState = kHTTPClientSendRequestBody;
						client->requestNextState = kHTTPClientPrepareBody;
					}
					else if (err == kFskErrEndOfFile) {
						FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgErrString, " -- in PrepareBody - sendDataCB returns EOF\n");
						client->requestState = kHTTPClientPrepareForResponse;
					}
					else if (err != kFskErrNone) {
						FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgErrString, " -- in PrepareBody - sendDataCB returns Error - failing\n");
						client->requestState = kHTTPClientRequestError;
					}
				}
			}
			else {
				FskInstrumentedItemSendMessageDebug(request, kFskHTTPClientRequestInstrMsgErrString, " -- in PrepareBody - no body was specified\n");
				client->requestState = kHTTPClientPrepareForResponse;
			}

			if (client->pendingCancel)
				break;
			if (client->requestState == kHTTPClientPrepareForResponse)
				goto prepareForResponse;
			if (client->requestState != kHTTPClientSendRequestBody)
				break;

		case kHTTPClientSendRequestBody:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRequestState, client);
//			amt = request->requestBodyBufferAmt - request->requestBodyBufferPos;
			amt = request->requestBodyBufferAmt;
			if (amt == 0) {
				client->requestState = client->requestNextState;
			}
 			else if (amt < 0) {
				FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgErrString,
					"SendRequest -- failed amt went negative");
				err = kFskErrOperationFailed;
				goto doError;
			}
			else {
				if (client->needsWritable) {
					retVal = kFskErrNone;		// bail out until it's writable
					goto done;
				}
				err = FskNetSocketSendTCP(client->skt, request->requestBodyBuffer + request->requestBodyBufferPos, amt, &ret);
#if SUPPORT_INSTRUMENTATION
				if (FskInstrumentedItemHasListeners(request)) {
					FskHTTPClientRequestInstrMsgDataRecord msg;
					msg.buffer = request->requestBodyBuffer + request->requestBodyBufferPos;
					msg.amt = ret;
					msg.err = err;
					FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgSentBody, &msg);
				}
#endif
				if (kFskErrNone == err) {
					sResetKillTimer(client);
					request->requestBodyBufferAmt -= ret;
					request->requestBodyBufferPos += ret;
					request->status.bytesSent += ret;
					client->status.bytesSent += ret;
					client->eligibleForReconnect = request->eligibleForReconnect;
					if (0 == request->requestBodyBufferAmt)
						client->requestState = client->requestNextState;
				}
				else if (kFskErrNoData == err) {
					client->needsWritable = true;
					retVal = kFskErrSocketFull;
				}
				else {
					client->status.lastErr = err;
					client->responseState = kHTTPClientSocketError;
					FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgErrString, "SendRequest -- send_tcp failed");
					goto doNetworkError;
				}
			}
			if (client->requestState != kHTTPClientPrepareForResponse)
				break;

		case  kHTTPClientPrepareForResponse:
prepareForResponse:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRequestState, client);

			err = FskNetSocketFlushTCP(client->skt);
			if (kFskErrNoData == err) {
				client->needsWritable = true;
				retVal = kFskErrSocketFull;
				goto done;
			}

			if (NULL == client->httpResponses) {
				if (client->responseSuspendedState == kHTTPClientNotSuspended) {
					client->responseState = kHTTPClientIdle;	// reset the responseState if necessary
				}
				else {
					client->responseSuspendedState = kHTTPClientIdle;
				}
			}
			err = kFskErrNone;
			retVal = kFskErrNone;
			goto transition;
			break;

		case kHTTPClientConnecting:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRequestState, client);
			err = kFskErrNone;
			retVal = kFskErrNone;
			break;
		case kHTTPClientConnectingSSL:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRequestState, client);
			err = kFskErrNone;
			retVal = kFskErrNone;
			break;
		case kHTTPClientConnectFailed:
			err = client->status.lastErr;
			goto doNetworkError;
		case kHTTPClientSuspended:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRequestState, client);
			err = kFskErrNone;
			retVal = kFskErrNone;
			break;

		case kHTTPClientIdle:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRequestState, client);
			if (request && request->flushAndReissue) {
				FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgTransitionRequest, request);
				FskListRemove((FskList*)(void*)&client->httpRequests, request);
				FskListAppend((FskList*)(void*)&client->httpResponses, request);
				break;
			}
			// fall through
		default:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRequestState, client);
transition:
			while ((req = client->httpRequests)
					&& (client->httpRequests->beenRequested)) {
				FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgTransitionRequest, req);
				FskListRemove((FskList*)(void*)&client->httpRequests, req);
				FskListAppend((FskList*)(void*)&client->httpResponses, req);
			}
			client->requestState = kHTTPClientIdle;

			// start up the next request
			if (client->httpRequests) {
#if OPEN_SSL || CLOSED_SSL
				if( client->sslProxied && client->waitingForSocket &&
							( client->protocolNum==kFskHTTPProtocolHTTPS && !client->skt->isSSL ) )
				{
					void* ssl;
					FskSocketCertificateRecord cert = {
						NULL, 0,
						client->cert.policies ? client->cert.policies : "allowOrphan",
						client->host,
						NULL, 0,
					};

					FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgBeginSSLThruProxy, client);
					client->requestState = kHTTPClientConnectingSSL;
#if OPEN_SSL
					err = FskNetSocketDoSSL(client->host, client->skt, (FskNetSocketCreatedCallback)sHTTPClientConnectedSSLThruProxy, client);
#else
					err = FskSSLAttach( &ssl, client->skt );
					if( kFskErrNone == err ) {
						FskSSLLoadCerts( ssl, &cert );
						err = FskSSLHandshake( ssl, sHTTPClientGotSocket, client, true );
						if( kFskErrNone == err )
							client->skt->isSSL = true;
					}
#endif
				}
				else
#endif
					err = sHTTPClientMakeRequest(client);
				if (kFskErrNone == err) {
					if (client->requestState == kHTTPClientSendRequest)
						goto sendRequest;
				}
			}
			else
				retVal = kFskErrNone;
			break;
	}
	goto done;

doNetworkError:
	while ((req = client->httpRequests)) {
		req->status.lastErr = client->status.lastErr;
		FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgErrString, "  send_tcp err, flushing/reissuing requests - recv_tcp err");
		FskListRemove((FskList*)(void*)&client->httpRequests, req);
		FskListAppend((FskList*)(void*)&client->httpResponses, req);
	}
	client->flushUnsatisfiedResponses = true;
	client->requestState = kHTTPClientIdle;
	client->requestState = 0;
	//@@ avoid rewriting the content already buffered
	client->outputBufPos = 0;
	client->outputBufAmt = 0;
	goto done;

doError:
	client->requestState = kHTTPClientError;
	client->status.lastErr = err;

done:
	sFskHTTPClientRequestDownUse(request);
	return retVal;
}

#if OPEN_SSL

FskErr sHTTPClientConnectedSSLThruProxy(struct FskSocketRecord *skt, void *refCon)
{
	FskHTTPClient client = (FskHTTPClient)refCon;

//	client->skt = skt;
	client->waitingForSocket = false;

	// someone cancelled the connection
	if (kHTTPClientCancel == client->requestState) {
		sClientCycle(client);
		return kFskErrNone;
	}

	if (client->disposed) {
		FskHTTPClientDispose(client);
		return kFskErrNone;
	}

	if (!skt) {
		if (kFskHTTPProtocolHTTPS == client->protocolNum) {
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgSSLConnectFailed, client);
			client->status.lastErr = kFskErrSSLHandshakeFailed;
			client->requestState = kHTTPClientSocketError;
			sClientCycle(client);
		}
		return kFskErrNoMoreSockets;
	}

	client->requestState = kHTTPClientIdle;
	sClientCycle(client);
	return kFskErrNone;
}
#endif

static FskErr sClientCycleResponse(FskHTTPClient client)
{
	FskErr err = kFskErrNone;
	FskErr retVal = kFskErrNeedMoreTime;
	int amt, ret;
	Boolean doRequestFinishedCallback = false;
	FskErr finishAllWithError = kFskErrNone;
	FskHTTPClientRequest request;
	int readCycleIterator = 0;

	request = client->httpResponses;
	if (NULL == request) {
		if (client->interfaceLost && (client->responseState != kHTTPClientIdle)) {
			client->interfaceLost = false;
			client->status.lastErr = kFskErrNetworkInterfaceRemoved;
			client->responseState = kHTTPClientDone;
		}
		else
			client->responseState = kHTTPClientIdle;
	}
	else {
		sFskHTTPClientRequestUpUse(request);
		if (client->responseState != kHTTPClientSuspended) {
			if ((client->interfaceLost)
				&& (client->responseState != kHTTPClientIdle)
				&& (client->responseState != kHTTPClientLostInterface)) {
				client->status.lastErr = kFskErrNetworkInterfaceRemoved;
				client->responseState = kHTTPClientLostInterface;
			}
			else {
				if (client->status.lastErr == kFskErrNameLookupFailed)
					client->responseState = kHTTPClientConnectFailed;
				if (client->status.lastErr == kFskErrConnectionRefused)
					client->responseState = kHTTPClientConnectFailed;
				if (client->status.lastErr == kFskErrSocketNotConnected)	// network unavailable
					client->responseState = kHTTPClientConnectFailed;
				if (client->status.lastErr == kFskErrNoNetworkInterfaces)
					client->responseState = kHTTPClientConnectFailed;
				if (client->status.lastErr == kFskErrNeedConnectionSelection)
					client->responseState = kHTTPClientConnectFailed;
			}
		}
	}

	switch (client->responseState) {
		case kHTTPClientInitialize:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			err = kFskErrNone;
			retVal = kFskErrNone;
			client->responseState = kHTTPClientIdle;
// fall thru			break;

		case kHTTPClientIdle:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			if (!request) {
				err = kFskErrNone;
				retVal = kFskErrNone;
				break;
			}
			else {
				if (request->flushAndReissue)
					break;
				client->responseState = kHTTPClientPrepareForResponse;
				client->responseNextState = kHTTPClientPrepareForResponseBody;
			}
			// fall through

		case kHTTPClientPrepareForResponse:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			client->responseState = kHTTPClientReadResponseHeaders;
			client->responseNextState = kHTTPClientPrepareForResponseBody;
//			break;		// fall through

		case kHTTPClientReadResponseHeaders:
			// fall through from PrepareForResponse
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			if (0 == client->inputBufAmt) {
				client->inputBufPos = 0;
				amt = kFskHTTPClientTransferBufferSize;
				if (client->needsReadable) {
					retVal = kFskErrNone;
					goto done;
				}
				err = FskNetSocketRecvTCP(client->skt, client->inputBuf, amt, &ret);
			}
			else {
				err = kFskErrNone;
				ret = 0;
			}
			if (0 == client->inputBufAmt && (kFskErrConnectionClosed == err || kFskErrSocketNotConnected == err)) {
				FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgConnectionClosed, request);
//				client->responseState = kHTTPClientSocketError;
				client->status.lastErr = request->status.lastErr = err;
				request->flushAndReissue = request->eligibleForReconnect;
				if (!request->eligibleForReconnect)
					client->responseState = kHTTPClientSocketError;
                else
                    request->eligibleForReconnect = false;      // only retry once
				request->closeOnDone = true;
			}
			else if (kFskErrNone == err || kFskErrConnectionClosed == err) {
				sResetKillTimer(client);
				client->inputBufAmt += ret;
				client->responseState = kHTTPClientProcessResponseHeaders;
				request->status.bytesReceived += ret;
				client->status.bytesReceived += ret;
				if (kFskErrConnectionClosed != err)
					client->eligibleForReconnect = request->eligibleForReconnect;
			}
			else if (kFskErrNoData == err) {
				client->needsReadable = true;
//				client->eligibleForReconnect = request->eligibleForReconnect;
				retVal = err;
			}
			else {
				FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgErrString,
					"kReadResponseHeaders - recv_tcp err");
				goto doError;
			}

			if (client->responseState != kHTTPClientProcessResponseHeaders)
				break;

		case kHTTPClientProcessResponseHeaders:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			ret = FskHeadersParseChunk(&client->inputBuf[client->inputBufPos], client->inputBufAmt, kFskHeaderTypeResponse, request->responseHeaders);
			if (ret > 0) {
#if SUPPORT_INSTRUMENTATION
				if (FskInstrumentedItemHasListeners(request)) {
					FskHTTPClientRequestInstrMsgDataRecord msg;
					msg.buffer = &client->inputBuf[client->inputBufPos];
					msg.amt = ret;
					msg.err = err;
					FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgRecvHeaders, &msg);
				}
#endif
				if (client->inputBufAmt == ret)
					client->inputBufAmt = 0;
				else {
					client->inputBufAmt -= ret;
					FskMemMove(client->inputBuf, &client->inputBuf[client->inputBufPos+ret], client->inputBufAmt);
				}
				client->inputBufPos = 0;
			}
			else if (ret < 0) {
				client->responseState = kHTTPClientError;
				client->status.lastErr = kFskErrOperationFailed;
				doRequestFinishedCallback = true;
				break;
			}

			client->responseState = kHTTPClientReadResponseHeaders;

			if (request->responseHeaders->headersParsed) {
				int response;
				response = sProcessResponseHeaders(client, request);
				if (kHTTPResponseRedirectionType == FskHeaderResponseCategory(response)) {
					// continue processing the body of request
				}
			}

			if (client->pendingCancel)
				break;
			if (client->responseState == kHTTPClientRequestDone)
				goto requestDone;
			if (client->responseState != kHTTPClientPrepareForResponseBody)
				break;

		case kHTTPClientPrepareForResponseBody:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			if (0 == request->respContentLength) {	// no body
				client->responseState = kHTTPClientRequestDone;
				goto requestDone;
			}
			if (kFskHTTPClientReadAllData == request->receivedDataCBMode) {
				if (request->respContentLength > 0)
					request->bufferSize = (UInt32)request->respContentLength;
				else
					request->bufferSize = kFskHTTPClientTransferBufferSize;

				if (kFskErrNone != (err = FskMemPtrNew(request->bufferSize, (FskMemPtr*)(void*)&request->buffer)))
					goto doError;
				request->userBuffer = false;
			}
			else if (kFskHTTPClientReadAnyData == request->receivedDataCBMode) {
				if (NULL == request->buffer) {
					request->buffer = client->outputBuf;
					request->userBuffer = true;			// ie. don't delete
					request->bufferSize = kFskHTTPClientTransferBufferSize;
				}
			}
			request->bufferAmt = 0;
			request->bufferPos = 0;

			client->responseState = kHTTPClientProcessResponseBody;
//			break;	// fall through to kHTTPClientProcessResponseBody

		case kHTTPClientProcessResponseBody:
processNow:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			// fall through from PrepareForResponseBody
			if (client->inputBufAmt == 0) {
				if (0 == request->respContentLength)
					client->responseState = kHTTPClientRequestDone;
				else
					client->responseState = kHTTPClientReadResponseBody;
			}
			else {
				if (request->transferEncoding == kFskTransferEncodingChunked) {
anotherChunk:
					if (request->currentChunkSize > 0) {
						amt = request->bufferSize - request->bufferAmt;
						if (amt > request->currentChunkSize)
							amt = request->currentChunkSize;
						if (amt > client->inputBufAmt)
							amt = client->inputBufAmt;
						if (amt) {
							FskMemCopy(&request->buffer[request->bufferAmt],
								&client->inputBuf[client->inputBufPos], amt);
							request->bufferAmt += amt;
							client->inputBufAmt -= amt;
							client->inputBufPos += amt;
							request->status.bytesReceived += amt;
							request->status.dataReceived += amt;
							request->currentChunkSize -= amt;
							if (client->inputBufAmt < 0) {
								FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgErrString,
									"client->inputBufAmt is negative");
							}
							else if (client->inputBufAmt > 0)
								FskMemCopy(client->inputBuf, &client->inputBuf[client->inputBufPos], client->inputBufAmt);
							else
								client->responseState = kHTTPClientReadResponseBody;
							client->inputBufPos = 0;
						}
					}

					// are we done with this chunk?
					if (request->currentChunkSize == 0) {
						char *p, *s;
						p = &client->inputBuf[client->inputBufPos];
						// read in next chunk (or trailer) to end of line
						while ((client->inputBufAmt > 1) && lineEnd(p)) {
		 					p += 2;
							client->inputBufAmt -= 2;
							client->inputBufPos += 2;
						}
						// if there's more data, then we read to end-of-line
						if (client->inputBufAmt >= 1) {
							s = &client->inputBuf[client->inputBufPos];
							p = s;
							while ((client->inputBufAmt > 1) && !lineEnd(p)) {
								p++;
								client->inputBufAmt--;
							}
							// if there's an end of line (and then the next chunk or EOL)
							if ((client->inputBufAmt > 1) && lineEnd(p)) {
								request->currentChunkSize = FskStrHexToNum(s, p-s);
								if (0 == request->currentChunkSize && '0' == s[0] ) {
									request->currentChunkSize = kFskHTTPClientEndOfChunk;
								}
								// consume line's EOL
								p += 2;
								client->inputBufAmt -= 2;
								client->inputBufPos += (p - s);

								if (request->currentChunkSize == kFskHTTPClientEndOfChunk) {
									request->currentChunkSize = 0;
									client->responseState = kHTTPClientRequestDone;
									request->chunksFinished = true;
									while ((client->inputBufAmt > 1) && (lineEnd(p))) {
										client->inputBufAmt -= 2;
										client->inputBufPos += 2;
										p += 2;
									}
								}
								else if (request->currentChunkSize == 0) {
									// this is probably a trailer or directives
									goto anotherChunk;
								}
								else {
									goto anotherChunk;
								}
							}
							else {
								// not a complete chunk size
								// return bits back for parsing when we read more.
								client->inputBufAmt += (p-s);
								client->responseState = kHTTPClientReadResponseBody;
							}
						}
						else {
							// no data after EOL
						}
					}
				}
				else {
					amt = client->inputBufAmt > request->bufferSize ? request->bufferSize : client->inputBufAmt;
					if ((request->respContentLength > 0) && (request->respContentLength < amt + request->respContentRead))
						amt = (UInt32)(request->respContentLength - request->respContentRead);
					FskMemCopy(request->buffer, &client->inputBuf[client->inputBufPos], amt);
					client->inputBufAmt -= amt;
					client->inputBufPos += amt;
					request->respContentRead += amt;
					request->status.bytesReceived += amt;
					request->status.dataReceived += amt;

					request->bufferAmt = amt;

					if ((request->respContentLength > 0) && (request->respContentRead == request->respContentLength))
						client->responseState = kHTTPClientRequestDone;
					else if (client->inputBufAmt == 0) {
						client->responseState = kHTTPClientReadResponseBody;
					}
					else
						client->responseState = kHTTPClientProcessResponseBody;
				}
			}

			{
				Boolean doCallback = false;

				if (kFskHTTPClientReadAnyData == request->receivedDataCBMode) {
					if (request->bufferAmt > 0)
						doCallback = true;
				}
				else {
					if (request->respContentLength == 0) {
						client->responseState = kHTTPClientRequestDone;
						doCallback = true;
					}
					else if (request->respContentRead == request->respContentLength) {
						client->responseState = kHTTPClientRequestDone;
						doCallback = true;
					}
					else {
						if ((request->bufferSize - request->bufferAmt) < kFskHTTPClientTransferBufferSize) {
							err = FskMemPtrRealloc(request->bufferSize + kFskHTTPClientTransferBufferSize, (FskMemPtr*)(void*)&request->buffer);
							if (kFskErrNone != err)
								goto doError;

							request->bufferSize += kFskHTTPClientTransferBufferSize;
						}
					}
				}

				if (request->flushAndReissue) {
					request->bufferAmt = 0;
					request->bufferPos = 0;
				}
				else if (doCallback)	 {
					err = clientReceivedData(request, request->buffer, request->bufferAmt);
					if (kFskErrNone != err)
						goto doError;

					request->bufferAmt = 0;
					request->bufferPos = 0;
				}
			}

			if (client->pendingCancel)
				break;
			if (client->responseState == kHTTPClientRequestDone)
				goto requestDone;

			if ((client->responseState != kHTTPClientReadResponseBody) || client->pendingSuspend)
				break;

		case kHTTPClientReadResponseBody:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			if (client->inputBufAmt && (client->inputBufPos != 0))
				FskMemMove(client->inputBuf, &client->inputBuf[client->inputBufPos], client->inputBufAmt);
			client->inputBufPos = 0;
			amt = kFskHTTPClientTransferBufferSize - client->inputBufAmt;

			if (client->needsReadable) {
				retVal = kFskErrNone;		// bail out until it's readable
				goto done;
			}
			err = FskNetSocketRecvTCP(client->skt,
						&client->inputBuf[client->inputBufAmt], amt, &ret);
			if (kFskErrNone == err) {
				sResetKillTimer(client);
				client->inputBufAmt += ret;
				client->responseState = kHTTPClientProcessResponseBody;
				client->responseNextState = kHTTPClientReadResponseBody;
				client->status.bytesReceived += ret;
				if (!client->pendingCancel && readCycleIterator++ < 5)
					goto processNow;
			}
			else if (kFskErrNoData == err) {
				client->responseState = kHTTPClientReadResponseBody;
				client->needsReadable = true;
				retVal = kFskErrNoData;	// no data ready to be read yet
			}
			else if (kFskErrConnectionClosed == err || kFskErrSocketNotConnected == err) {
				if (client->inputBufAmt > 0) {
					client->responseState = kHTTPClientProcessResponseBody;
					client->responseNextState = kHTTPClientRequestDone;
				}
				else
					client->responseState = kHTTPClientRequestDone;
				client->status.lastErr = err;
				request->closeOnDone = true;
			}
			else if (kFskErrNone != err) {
				FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgErrString,
					"kHTTPClientReadResponseBody - recv_tcp err");
				goto doError;
			}

			if (client->responseState != kHTTPClientRequestDone)
				break;

		case kHTTPClientRequestDone:
requestDone:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			if (client->inputBufAmt) {
				// we've still got data, deal with it...
				if (client->httpResponses->next)
					client->responseState = kHTTPClientProcessResponseHeaders;
				else {
#if SUPPORT_INSTRUMENTATION
					if (FskInstrumentedItemHasListeners(client)) {
						FskHTTPClientRequestInstrMsgDataRecord msg;
						msg.buffer = &client->inputBuf[client->inputBufPos];
						msg.amt = client->inputBufAmt;
						FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgRemainder, &msg);
					}
#endif
					client->inputBufAmt = 0;
					client->inputBufPos = 0;
					client->responseState = kHTTPClientDone;
				}
			}
			else {
				client->responseState = kHTTPClientDone;
			}

			if (request)
				doRequestFinishedCallback = true;
			break;

		case kHTTPClientReadRemainder:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			if (request) {
				if (!request->buffer) {
					request->bufferSize = kFskHTTPClientTransferBufferSize;
					if (kFskErrNone != (err = FskMemPtrNew(request->bufferSize, (FskMemPtr*)(void*)&request->buffer)))
						goto doError;
					request->userBuffer = false;
				}

				err = FskNetSocketRecvTCP(client->skt, request->buffer,
							request->bufferSize, &ret);
#if SUPPORT_INSTRUMENTATION
				if (FskInstrumentedItemHasListeners(request)) {
					FskHTTPClientRequestInstrMsgDataRecord msg;
					msg.buffer = request->buffer;
					msg.amt = ret;
					msg.err = err;
					FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgReadRemainder, &msg);
				}
#endif
				client->status.bytesReceived += ret;
				if ((kFskErrConnectionClosed == err)
					|| (kFskErrSocketNotConnected == err)) {
					request->closeOnDone = true;
					client->readUntilServerCloses = false;
					client->responseState = kHTTPClientDone;
					client->status.lastErr = kFskErrNone;
				}
			}
			else
				client->responseState = kHTTPClientDone;
			client->readUntilServerCloses = false;
			break;

		case kHTTPClientSuspended:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			err = kFskErrNone;
			retVal = kFskErrNone;
			break;

		case kHTTPClientDone:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			if (client->status.lastErr == kFskErrTimedOut) {
				client->responseState = kHTTPClientIdle;
				if (request) {
					doRequestFinishedCallback = true;
					finishAllWithError = kFskErrTimedOut;
				}
				err = kFskErrNone;
				break;
			}
			if (client->readUntilServerCloses) {
				client->responseState = kHTTPClientReadRemainder;
				break;
			}
			FskTimeCallbackRemove(client->cycleCallback);
			client->responseState = kHTTPClientIdle;
			err = kFskErrNone;
			break;

		case kHTTPClientCancel:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			client->responseState = kHTTPClientDone;
			client->status.lastErr = kFskErrOperationCancelled;
			request->status.lastErr = kFskErrOperationCancelled;
			client->readUntilServerCloses = false;
			request->closeOnDone = true;
			finishAllWithError = client->status.lastErr;
			doRequestFinishedCallback = true;
			break;

		case kHTTPClientLostInterface:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			client->responseState = kHTTPClientDone;
			client->status.lastErr = kFskErrNetworkInterfaceRemoved;
			client->readUntilServerCloses = false;
			request->status.lastErr = kFskErrNetworkInterfaceRemoved;
			request->closeOnDone = true;
			finishAllWithError = client->status.lastErr;
			doRequestFinishedCallback = true;
			break;

		case kHTTPClientRequestError:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			client->responseState = kHTTPClientRequestDone;
			client->readUntilServerCloses = false;
			request->closeOnDone = true;
			break;
		case kHTTPClientError:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			client->responseState = kHTTPClientRequestDone;
			client->readUntilServerCloses = false;
			request->status.lastErr = client->status.lastErr;
			request->closeOnDone = true;
			break;
		case kHTTPClientSocketError:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			if (client->eligibleForReconnect) {
				client->responseState = kHTTPClientRequestDone;
				client->readUntilServerCloses = false;
				request->closeOnDone = true;
			}
			else {
				client->responseState = kHTTPClientIdle;
				doRequestFinishedCallback = true;
				request->closeOnDone = true;
				finishAllWithError = client->status.lastErr;
			}
			break;
		case kHTTPClientConnectFailed:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			client->responseState = kHTTPClientIdle;
			doRequestFinishedCallback = true;
			request->closeOnDone = true;
			finishAllWithError = client->status.lastErr;
			break;
		default:
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgResponseState, client);
			err = kFskErrNone;
			retVal = kFskErrNone;
			break;
	}

	if ((request && request->flushAndReissue) || doRequestFinishedCallback) {
		FskHTTPClientRemoveRequest(request);

		if (request->closeOnDone) {
			FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgCloseOnDone, request);
			FskNetSocketClose(client->skt);

			client->skt = NULL;
			client->sslProxied = false;
			request->closeOnDone = false;
			// if connection dies, clear unsatisfied requests and responses so they can be reissued
			client->flushUnsatisfiedResponses = true;
		}

		if (client->flushUnsatisfiedResponses) {
			FskHTTPClientRequest req;
			client->flushUnsatisfiedResponses = false;
			while ((req = client->httpResponses)) {
				FskListRemove((FskList*)(void*)&client->httpResponses, req);
				if (finishAllWithError) {
					req->status.lastErr = finishAllWithError;
					if (req->reqFinishedCB) {
						err = (*req->reqFinishedCB)(client, req, req->refCon);	// request can be disposed of in this routine.
					}
				}
				else
					sReissueRequest(client, req);
			}
		}

		if (finishAllWithError) {
			if (request->reqFinishedCB) {
				request->status.lastErr = finishAllWithError;
				err = (*request->reqFinishedCB)(client, request, request->refCon);	// request can be disposed of in this routine.
			}
			FskHTTPClientRequestDispose(request);	// get rid of it regardless of callback
		}
		else if (request->flushAndReissue) {
			client->requestState = kHTTPClientIdle;
			client->responseState = kHTTPClientIdle;
			sReissueRequest(client, request);
			if (request->suspend) {
				request->suspend = false;
				if (client->auth && client->auth->waitingForPassword) {
					client->requestSuspendedState = kHTTPClientDone;
					client->requestState = kHTTPClientSuspended;
					client->responseSuspendedState = kHTTPClientDone;
					client->responseState = kHTTPClientSuspended;
				}
			}
		}
		else {
			if (request->bufferAmt) {
				err = clientReceivedData(request, request->buffer, request->bufferAmt);
			}
			if (request->reqFinishedCB) {
				if ((kFskErrNone == client->status.lastErr)
					|| (kFskErrConnectionClosed == client->status.lastErr)
					|| (kFskErrSocketNotConnected == client->status.lastErr)) {
					client->status.lastErr = kFskErrNone;
					if (!request->responseHeaders->headersParsed) {
						if (request->status.bytesReceived)
							request->status.lastErr = kFskErrConnectionClosed;
						else
							request->status.lastErr = kFskErrConnectionDropped;
					}
					else if (request->method && !FskStrCompare(request->method, "HEAD"))
						request->status.lastErr = kFskErrNone;
					else if ((request->transferEncoding == kFskTransferEncodingChunked) &&
						((0 != request->currentChunkSize) || (false == request->chunksFinished))) {
						request->status.lastErr = kFskErrConnectionClosed;
					}
					else if ((request->respContentLength > 0)
						&& (request->status.dataReceived != request->respContentLength)) {
						request->status.lastErr = kFskErrConnectionClosed;
					}
					if (request->status.lastErr && (client->status.lastErr != request->status.lastErr))
						client->status.lastErr = request->status.lastErr;
				}
				FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgFinished, request);
				if (request->reqFinishedCB) {
					err = (*request->reqFinishedCB)(client, request, request->refCon);	// request can be disposed of in this routine.
				}
			}
			FskHTTPClientRequestDispose(request);	// get rid of it regardless of callback
		}
	}

done:
	sFskHTTPClientRequestDownUse(request);

	return retVal;

doError:
	client->eligibleForReconnect = false;
	client->responseState = kHTTPClientRequestError;
	client->status.lastErr = err;
	goto done;
}

static void sReissueRequest(FskHTTPClient client, FskHTTPClientRequest request)
{
	request->beenRequested = false;
	request->flushAndReissue = false;
	request->respContentLength = kFskHTTPNoContentLength;
	request->respContentRead = 0;
	request->owner = client;
	request->requestBodyPos = 0;
	client->status.lastErr = 0;
	request->status.lastErr = 0;
	FskHeaderStructDispose(request->responseHeaders);
	FskHeaderStructNew(&request->responseHeaders);
	FskListAppend((FskList*)(void*)&client->httpRequests, request);
	FskInstrumentedItemSendMessage(request, kFskHTTPClientRequestInstrMsgReissue, request);
}

static void sClientRunCycle(FskTimeCallBack cb, FskTime when, void *param)
{
	sClientCycle(param);
}

static void sCanReadData(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon)
{
	FskHTTPClient client = (FskHTTPClient)refCon;
	FskThreadRemoveDataHandler(&client->readDataHandler);
	client->needsReadable = false;
	sClientCycle(client);
}

static void sCanSendData(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon)
{
	FskHTTPClient client = (FskHTTPClient)refCon;
	FskThreadRemoveDataHandler(&client->writeDataHandler);
	client->needsWritable = false;
	sClientCycle(client);
}

static void sClientCycle(void *param)
{
	FskHTTPClient client = (FskHTTPClient)param;
	int retReq, retResp;
	Boolean scheduleNextRun = false;

	client->cycleNum++;
	sFskHTTPClientUpUse(client);
	FskInstrumentedItemSendMessageDebug(client, kFskHTTPClientInstrMsgCycleBegin, client);

	if (client->pendingSuspend) {
		client->pendingSuspend = false;
		client->requestSuspendedState = client->requestState;
		client->requestState = kHTTPClientSuspended;
		client->responseSuspendedState = client->responseState;
		client->responseState = kHTTPClientSuspended;
		FskTimeCallbackRemove(client->cycleCallback);
		sResetKillTimer(client);
		goto finish;
	}

	if (client->pendingCancel) {
		client->pendingCancel = false;
		client->requestState = kHTTPClientCancel;
		client->responseState = kHTTPClientCancel;
	}

	retReq = sClientCycleRequest(client);
	if (retReq == kFskErrSocketFull) {
		FskThreadAddDataHandler(&client->writeDataHandler, (FskThreadDataSource)client->skt, sCanSendData, false, true, client);
	}
	else if (retReq == kFskErrNeedMoreTime)
		scheduleNextRun = true;

	retResp = sClientCycleResponse(client);
	if (retResp == kFskErrNoData) {
		FskThreadAddDataHandler(&client->readDataHandler, (FskThreadDataSource)client->skt, sCanReadData, true, false, client);
	}
	else if (retResp == kFskErrNeedMoreTime)
		scheduleNextRun = true;

	if ((client->httpRequests && (client->requestState == kHTTPClientIdle))
		|| (client->httpResponses && (client->responseState == kHTTPClientIdle)))
		scheduleNextRun = true;

	if (scheduleNextRun)
		FskTimeCallbackScheduleNextRun(client->cycleCallback, sClientRunCycle, client);
	else if (FskHTTPClientIsIdle(client)) {
		if (client->finishedCB) {
			client->calledFinish = true;	// have to do this before, since finishedCB can dispose
			FskInstrumentedItemSendMessage(client, kFskHTTPClientInstrMsgFinished, client);
			(*client->finishedCB)(client, client->refCon);
		}
	}

finish:
	FskInstrumentedItemSendMessageDebug(client, kFskHTTPClientInstrMsgCycleEnd, client);
	sFskHTTPClientDownUse(client);
}


static int sHTTPClientGetRequestLength(FskHTTPClientRequest request)
{
	FskAssociativeArrayIterator	iter;
	int	totalSize = 0;

	if (request->method)
		totalSize += FskStrLen(request->method) + 1;
	else
		totalSize += 4;
	totalSize += 9;		// http://
	totalSize += FskStrLen(request->parsedUrl->path);
	iter = FskAssociativeArrayIteratorNew(request->requestParameters);
	while (iter) {
		totalSize += 2;		// ? or & and =
		totalSize += FskStrLen(iter->name);
		totalSize += FskStrLen(iter->value);
		iter = FskAssociativeArrayIteratorNext(iter);
	}
	totalSize += FskStrLen(request->parsedUrl->params);
	totalSize += 11;	// <sp>HTTP/1.1/r/n
	totalSize += 8;		// Host:<sp>(host)/r/n
	totalSize += FskStrLen(request->parsedUrl->host);

	if (request->staticRequestBuffer) {
		totalSize += request->requestBodySize;
	}
	totalSize += 2;	// final crlf

	if (totalSize > kFskHTTPClientTransferBufferSize) {
		return -1;
	}
	FskAssociativeArrayIteratorDispose(iter);

	return totalSize;
}

#define kProxyConnectTemplate "CONNECT %s:%d HTTP/1.0\r\n"

#define kProxyConnectAuthenticationTemplate \
	"Proxy-Authorization: Basic %s\r\n"

static FskErr sHTTPClientRequestBlob(FskHTTPClient client, FskHTTPClientRequest request, char *buffer, int bufferSizeMax, int *bufferSizeUsed)
{
	FskAssociativeArrayIterator	iter;
	Boolean				first = true;
	char				*hostString = NULL;
	int					hostStringSize, totalSize;
	FskErr				err;
	int bufferPos;

	totalSize = sHTTPClientGetRequestLength(request);
	if ((totalSize == -1)
		|| (totalSize > bufferSizeMax))
		return kFskErrRequestTooLarge;

	hostStringSize = FskStrLen(request->parsedUrl->host) + 10;
	if (kFskErrNone != (err = FskMemPtrNew(hostStringSize, &hostString))) return err;

	if (request->parsedUrl->port != 80)	{
		char portNum[12];
		FskStrCopy(hostString, request->parsedUrl->host);
		FskStrCat(hostString, ":");
		FskStrNumToStr(request->parsedUrl->port, portNum, sizeof(portNum));
		FskStrCat(hostString, portNum);
	}
	else
		FskStrCopy(hostString, request->parsedUrl->host);

	if( (!client->isLocal) && (!client->sslProxied) &&
			( ((client->protocolNum == kFskHTTPProtocolHTTPS) && client->proxyAddrS) ||
				((client->protocolNum == kFskHTTPProtocolHTTP) && client->proxyAddr ) ) )
	{
		totalSize = sizeof(kProxyConnectTemplate) - 4 + FskStrLen( request->parsedUrl->host ) + 16 + 1;
		if( client->proxyAuth )
			totalSize += (sizeof(kProxyConnectAuthenticationTemplate) + FskStrLen( client->proxyAuth ) + 1);
		if( totalSize > bufferSizeMax )
			return kFskErrRequestTooLarge;
		snprintf( buffer, bufferSizeMax, kProxyConnectTemplate,
				request->parsedUrl->host, request->parsedUrl->port );
		totalSize = FskStrLen( buffer );
		if( client->proxyAuth )
			snprintf( buffer+totalSize, bufferSizeMax-totalSize,
							kProxyConnectAuthenticationTemplate, client->proxyAuth );

		FskStrCat( buffer, "\r\n" );
		bufferPos = FskStrLen(buffer);
		*bufferSizeUsed = bufferPos;
		goto bail;
	}

	if (request->method)
		FskStrCopy(buffer, request->method);
	else
		FskStrCopy(buffer, "GET");

	if (client->protocolNum == kFskHTTPProtocolHTTPS && client->proxyAddrS) {
		FskStrCat(buffer, " https://");
		FskStrCat(buffer, hostString);
		FskStrCat(buffer, "/");
	}
	else if (client->proxyAddr && !client->isLocal) {
		FskStrCat(buffer, " http://");
		FskStrCat(buffer, hostString);
		FskStrCat(buffer, "/");
	}
	else {
		FskStrCat(buffer, " /");
	}

	if (request->parsedUrl->path)
		FskStrCat(buffer, request->parsedUrl->path);

	iter = FskAssociativeArrayIteratorNew(request->requestParameters);
	while (iter) {
		if (first) {
			first = false;
			FskStrCat(buffer, "?");
		}
		else
			FskStrCat(buffer, "&");
		FskStrCat(buffer, iter->name);
		FskStrCat(buffer, "=");
		FskStrCat(buffer, iter->value);
		iter = FskAssociativeArrayIteratorNext(iter);
	}
	if (request->parsedUrl->params) {
		if (first) {
			first = false;
			FskStrCat(buffer, "?");
		}
		else
			FskStrCat(buffer, "&");
		FskStrCat(buffer, request->parsedUrl->params);
	}

	FskStrCat(buffer, " HTTP/1.1\r\n");
	FskHeaderRemove(kFskStrHost, request->requestHeaders);
	FskHeaderAddString(kFskStrHost, hostString, request->requestHeaders);

	// Format the headers appropriately and dump into the send buffer
	bufferPos = FskStrLen(buffer);
	bufferPos += FskHeaderGenerateOutputBlob(&buffer[bufferPos], bufferSizeMax - bufferPos,
		true, request->requestHeaders);

	if (request->staticRequestBuffer) {
		FskMemCopy(&buffer[bufferPos], request->requestBodyBuffer, request->requestBodySize);
		bufferPos += request->requestBodySize;
	}

	*bufferSizeUsed = bufferPos;

#if OPEN_SSL || CLOSED_SSL
bail:
#endif
	FskMemPtrDispose(hostString);

	return err;
}


static FskErr sPrepareHTTPClientRequestBlock(FskHTTPClient client)
{
	FskHTTPClientRequest req;
	char *hdr;
	int size, gotOne, pipelinable = 0;

	gotOne = 0;
	if (!client->dontPipeline)
		pipelinable = 1;
	req = client->httpRequests;
	while (req) {
		if (req->beenRequested) {
			break;
		}
		if (client->auth) {
			if (client->auth->sessionKeyValid || client->auth->tryPassword) {
				FskHTTPClientRequestAddAuthorization(client, req);
			}
		}

		size = sHTTPClientGetRequestLength(req);
		if (client->outputBufAmt + size > kFskHTTPClientTransferBufferSize)
			break;	// stop - requests wont fit anymore
		if (req->sendDataCB) {	// if there's a send callback, don't pipeline
			if (gotOne)
			break;
			pipelinable = 0;
		}
		if ((NULL != req->method)
			&& (0 != FskStrCompare(req->method, "GET"))
			&& (0 != FskStrCompare(req->method, "HEAD"))) {
			if (gotOne)
				break;	// only GET and HEAD can be pipelined
			pipelinable = 0;
		}
		if (req->method && (0 == FskStrCompare(req->method, "POST"))) {
			char *foo;
			foo = FskHeaderFind(kFskStrContentLength, req->requestHeaders);
			if (!foo || 0 == FskStrToNum(foo)) {
				FskHeaderRemove(kFskStrContentLength, req->requestHeaders);
				FskHeaderAddInteger(kFskStrContentLength, 0, req->requestHeaders);
			}
		}
		if (client->protocolNum == kFskHTTPProtocolHTTPS && !client->sslProxied && client->proxyAddrS) {
//			FskInstrumentedItemSendMessage(req, kFskHTTPClientRequestInstrMsgNotConnect, req->method);
			pipelinable = 0;
		}
		hdr = FskHeaderFind(kFskStrExpect, req->requestHeaders);
		if (hdr && (0 == FskStrCompareCaseInsensitiveWithLength(hdr, kFskStr100Continue, FskStrLen(kFskStr100Continue))))
			req->expectContinue = true;

		sHTTPClientRequestBlob(client, req, &client->outputBuf[client->outputBufAmt], kFskHTTPClientTransferBufferSize - client->outputBufAmt, &size);
		client->outputBufAmt += size;
		req->beenRequested = true;
		if (! pipelinable)
			break;

		gotOne = 1;
		req = req->next;
	}

	return kFskErrNone;
}


// MakeRequest is only called when client requests are idle and connected
static FskErr sHTTPClientMakeRequest(FskHTTPClient client)
{
	FskErr err = kFskErrNone;

	if (!client->skt) {
		return FskHTTPClientBegin(client);	// this should start the socket
	}

	// prepare request buffers
	err = sPrepareHTTPClientRequestBlock(client);
	BAIL_IF_ERR(err);

	client->requestState = kHTTPClientSendRequest;
#if OPEN_SSL || CLOSED_SSL
	if (client->protocolNum == kFskHTTPProtocolHTTPS && (client->proxyPort && !client->sslProxied)) {
		client->requestNextState = kHTTPClientPrepareForResponse;
	}
	else
#endif
		client->requestNextState = kHTTPClientPrepareBody;

bail:
	return err;
}

/* ---------------------------------------------------------------------- */
FskErr sHTTPClientGotSocket(struct FskSocketRecord *skt, void *refCon)
{
	FskHTTPClient client = (FskHTTPClient)refCon;

	client->skt = skt;
	client->waitingForSocket = false;

	// someone cancelled the connection
	if (kHTTPClientCancel == client->requestState) {
		FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString,
			"Someone cancelled this http connection");
		sClientCycle(client);
		return kFskErrNone;
	}

	if (client->disposed) {
		FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString,
			"Someone disposed of this http connection while we were trying to connect");
		sFskHTTPClientDownUse(client);
		return kFskErrNone;
	}

	if (!skt || 0 == skt->ipaddrRemote) {
#if TARGET_OS_ANDROID
		if (client->interfaceSeed != client->interfaceSeedPreConnect) {
			FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString,
				"skt is null or ipaddRemote but intefaceSeed changed (ie. proxy changed) so retry");
			client->eligibleForReconnect = true;
			client->interfaceLost = false;
			client->status.lastErr = 0;
			if (client->httpRequests) {
				client->httpRequests->closeOnDone = true;
				client->httpRequests->flushAndReissue = true;
			}
			goto bail;
		}
#endif
		FskInstrumentedItemSendMessage(client, kFskHTTPClientRequestInstrMsgErrString,
			"skt is null or ipaddRemote is null meaning that the connect failed");
		client->status.lastErr = skt ? (skt->lastErr ? skt->lastErr : kFskErrNameLookupFailed) : kFskErrNameLookupFailed;
		client->requestState = kHTTPClientConnectFailed;
		sClientCycle(client);
		return kFskErrNone;
	}

	// get the local address so we can manage interfaces going away
	FskNetSocketGetLocalAddress(client->skt, NULL, NULL);

#if TARGET_OS_ANDROID
bail:
#endif

	sResetKillTimer(client);
	client->requestState = kHTTPClientIdle;
	sClientCycle(client);
	return kFskErrNone;
}

#if SUPPORT_INSTRUMENTATION
const char *state2str(UInt32 state) {
switch (state) {
	case kHTTPClientIdle: return "kHTTPClientIdle";
	case kHTTPClientSuspended: return "kHTTPClientSuspended";
	case kHTTPClientInitialize: return "kHTTPClientInitialize";
	case kHTTPClientConnecting: return "kHTTPClientConnecting";
	case kHTTPClientConnectingSSL: return "kHTTPClientConnectingSSL";
	case kHTTPClientSendRequest: return "kHTTPClientSendRequest";
	case kHTTPClientPrepareBody: return "kHTTPClientPrepareBody";
	case kHTTPClientSendRequestBody: return "kHTTPClientSendRequestBody";
	case kHTTPClientPrepareForResponse: return "kHTTPClientPrepareForResponse";
	case kHTTPClientReadResponseHeaders: return "kHTTPClientReadResponseHeaders";
	case kHTTPClientProcessResponseHeaders: return "kHTTPClientProcessResponseHeaders";
	case kHTTPClientPrepareForResponseBody: return "kHTTPClientPrepareForResponseBody";
	case kHTTPClientReadResponseBody: return "kHTTPClientReadResponseBody";
	case kHTTPClientProcessResponseBody: return "kHTTPClientProcessResponseBody";
	case kHTTPClientReadRemainder: return "kHTTPClientReadRemainder";
	case kHTTPClientRequestDone: return "kHTTPClientRequestDone";
	case kHTTPClientDone: return "kHTTPClientDone";
	case kHTTPClientRequestError: return "kHTTPClientRequestError";
	case kHTTPClientSocketError: return "kHTTPClientSocketError";
	case kHTTPClientConnectFailed: return "kHTTPClientConnectFailed";
	case kHTTPClientError: return "kHTTPClientError";
	case kHTTPClientLostInterface: return "kHTTPClientLostInterface";
	case kHTTPClientCancel: return "kHTTPClientCancel";
	case kHTTPClientNotSuspended: return "kHTTPClientNotSuspended";
	}
	return "";
}

static Boolean doFormatMessageHTTPClient(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize) {
	FskHTTPClient client = (FskHTTPClient)msgData;
	FskHTTPClientRequest request = (FskHTTPClientRequest)msgData;
	FskHTTPAuth auth = (FskHTTPAuth)msgData;
	FskHTTPClientRequestInstrMsgDataRecord *data = (FskHTTPClientRequestInstrMsgDataRecord *)msgData;

	char tmp[64];
	UInt32 s;
	const UInt32 kMessageTextSize = 128;

	switch (msg) {
    	case kFskHTTPClientInstrMsgKillIdle:
			snprintf(buffer, bufferSize, "[%d:%s] killing connection - idle too long", client->seqNum, client->host);
			return true;
    	case kFskHTTPClientInstrMsgBegin:
			snprintf(buffer, bufferSize, "[%d:%s] begin session - current request State: %s", client->seqNum, client->host, state2str(client->requestState));
			return true;
    	case kFskHTTPClientInstrMsgBeginCurrentlyProcessing:
			snprintf(buffer, bufferSize, "[%d:%s] append to session - already processing another request", client->seqNum, client->host);
			return true;
    	case kFskHTTPClientInstrMsgBeginSSLThruProxy:
			snprintf(buffer, bufferSize, "[%d:%s] begin session - connected to proxy, starting ssl connection", client->seqNum, client->host);
			return true;
    	case kFskHTTPClientInstrMsgBeginClientAlreadyConnected:
			snprintf(buffer, bufferSize, "[%d:%s] begin session - already connected -- start request ", client->seqNum, client->host);
			return true;
    	case kFskHTTPClientInstrMsgBeginConnectToSecureProxy:
			FskNetIPandPortToString(client->proxyAddrS, client->proxyPortS, tmp);
			snprintf(buffer, bufferSize, "[%d:%s] begin session - connect through secure proxy %s", client->seqNum, client->host, tmp);
			return true;
    	case kFskHTTPClientInstrMsgSecureProxyConnected:
			FskNetIPandPortToString(client->proxyAddrS, client->proxyPortS, tmp);
			snprintf(buffer, bufferSize, "[%d:%s] connected through ssl proxy %s", client->seqNum, client->host, tmp);
			return true;
    	case kFskHTTPClientInstrMsgBeginConnectToSecure:
			snprintf(buffer, bufferSize, "[%d:%s] begin session - connect to secure server", client->seqNum, client->host);
			return true;
    	case kFskHTTPClientInstrMsgSSLConnectFailed:
			FskNetIPandPortToString(client->proxyAddrS, client->proxyPortS, tmp);
			snprintf(buffer, bufferSize, "[%d:%s] failed to connect through ssl proxy %s", client->seqNum, client->host, tmp);
			return true;
    	case kFskHTTPClientInstrMsgBeginConnectToProxy:
			FskNetIPandPortToString(client->proxyAddr, client->proxyPort, tmp);
			snprintf(buffer, bufferSize, "[%d:%s] begin session - connect through proxy %s", client->seqNum, client->host, tmp);
			return true;
    	case kFskHTTPClientInstrMsgBeginConnect:
			snprintf(buffer, bufferSize, "[%d:%s] begin session - connect to server", client->seqNum, client->host);
			return true;
    	case kFskHTTPClientInstrMsgSuspend:
			snprintf(buffer, bufferSize, "[%d:%s] suspending", client->seqNum, client->host);
			return true;
    	case kFskHTTPClientInstrMsgResume:
			snprintf(buffer, bufferSize, "[%d:%s] resuming", client->seqNum, client->host);
			return true;
    	case kFskHTTPClientInstrMsgRequestState:
			{
			request = client->httpRequests;
			if (request) {
				if (request->flushAndReissue) {
					snprintf(buffer, bufferSize, "[%d:%s] requestState: %s -- errs(skt:%d, http:%ld) (flushing)", client->seqNum, client->host, state2str(client->requestState), client->skt ? client->skt->lastErr : 0, (long)client->status.lastErr);
					return true;
				}
			}
			snprintf(buffer, bufferSize, "[%d:%s] requestState: %s -- errs(skt:%d, http:%ld)", client->seqNum, client->host, state2str(client->requestState), client->skt ? client->skt->lastErr : 0, (long)client->status.lastErr);
			return true;
			}
    	case kFskHTTPClientInstrMsgResponseState:
			{
			request = client->httpResponses;
			if (request) {
				if (request->flushAndReissue) {
					snprintf(buffer, bufferSize, "[%d:%s] responseState: %s -- errs(skt:%d, http:%ld) (flushing)", client->seqNum, client->host, state2str(client->responseState), client->skt ? client->skt->lastErr : 0, (long)client->status.lastErr);
					return true;
				}
			}
			snprintf(buffer, bufferSize, "[%d:%s] responseState: %s -- errs(skt:%d, http:%ld)", client->seqNum, client->host, state2str(client->responseState), client->skt ? client->skt->lastErr : 0, (long)client->status.lastErr);
			return true;
			}
		case kFskHTTPClientInstrMsgTransitionRequest:
			snprintf(buffer, bufferSize, "[%d:%s] transition [/%s] to response queue state (response current state is %s) ", request->owner->seqNum, request->owner->host, request->parsedUrl->path, state2str(client->responseState));
			return true;
		case kFskHTTPClientInstrMsgConnectionClosed:
			snprintf(buffer, bufferSize, "[%d:%s] Connection closed by remote server ", request->owner->seqNum, request->owner->host);
			return true;
    	case kFskHTTPClientInstrMsgFinished:
			snprintf(buffer, bufferSize, "[%d:%s] client finished", client->seqNum, client->host);
			return true;
		case kFskHTTPClientInstrMsgFinishedDoneCB:
			snprintf(buffer, bufferSize, "[%d:%s] client finished - post callback", client->seqNum, client->host);
			return true;
    	case kFskHTTPClientRequestInstrMsgAdd:
			snprintf(buffer, bufferSize, "[%d:%s] adding (/%s) to client session (#%u in request queue)", request->owner->seqNum, request->owner->host, request->parsedUrl->path, (unsigned)(FskListCount(request->owner->httpRequests)));
			return true;
    	case kFskHTTPClientRequestInstrMsgRemove:
			if (request) {
				if (request->owner)
					snprintf(buffer, bufferSize, "[%s] removing request (/%s)", request->owner->host, request->parsedUrl->path);
				else
					snprintf(buffer, bufferSize, "[%s] removing request (/%s)", request->parsedUrl->host, request->parsedUrl->path);
			}
			else
				snprintf(buffer, bufferSize, "[none] removing NULL request");
			return true;
    	case kFskHTTPClientRequestInstrMsgChangeURL:
			snprintf(buffer, bufferSize, "changing request (/%s)", (char*)msgData);
			return true;
    	case kFskHTTPClientRequestInstrMsgServerResponse:
			snprintf(buffer, bufferSize, "server response (%d)", (int)msgData);
			return true;
    	case kFskHTTPClientRequestInstrMsgSentRequest:
			snprintf(buffer, bufferSize, "sent request %u bytes", (unsigned)data->amt);
			return true;
    	case kFskHTTPClientRequestInstrMsgSentBody:
#if INSTR_PACKET_CONTENTS
			s = data->amt - 1;
			if ((s + kMessageTextSize) > bufferSize)
				s = bufferSize - kMessageTextSize;
			tmp[0] = data->buffer[s];
			data->buffer[s] = '\0';
			snprintf(buffer, bufferSize, "sent request body\n%s%c", data->buffer, tmp[0]);
			data->buffer[s] = tmp[0];
#else
			snprintf(buffer, bufferSize, "sent request body (%u bytes)", (unsigned)data->amt);
#endif
			return true;
    	case kFskHTTPClientRequestInstrMsgRecvHeaders:
			s = data->amt - 1;
			if ((s + kMessageTextSize) > bufferSize)
				s = bufferSize - kMessageTextSize;
			tmp[0] = data->buffer[s];
			data->buffer[s] = '\0';
			snprintf(buffer, bufferSize, "received headers\n%s%c", data->buffer, tmp[0]);
			data->buffer[s] = tmp[0];
			return true;
    	case kFskHTTPClientRequestInstrMsgRecvData:
#if INSTR_PACKET_CONTENTS
			s = data->amt - 1;
			if ((s + kMessageTextSize) > bufferSize)
				s = bufferSize - kMessageTextSize;
			tmp[0] = data->buffer[s];
			data->buffer[s] = '\0';
			snprintf(buffer, bufferSize, "received body\n%s%c", data->buffer, tmp[0]);
			data->buffer[s] = tmp[0];
#else
			snprintf(buffer, bufferSize, "received body (%u bytes)", (unsigned)data->amt);
#endif
			return true;
    	case kFskHTTPClientInstrMsgRemainder:
#if INSTR_PACKET_CONTENTS
			s = data->amt - 1;
			if ((s + kMessageTextSize) > bufferSize)
				s = bufferSize - kMessageTextSize;
			tmp[0] = data->buffer[s];
			data->buffer[s] = '\0';
			snprintf(buffer, bufferSize, "data remains(%d bytes)\n%s%c", data->amt, data->buffer, tmp[0]);
			data->buffer[s] = tmp[0];
#else
			snprintf(buffer, bufferSize, "data remains after requests satisfied (%u bytes)", (unsigned)data->amt);
#endif
			return true;
    	case kFskHTTPClientRequestInstrMsgReadRemainder:
#if INSTR_PACKET_CONTENTS
			s = data->amt - 1;
			if ((s + kMessageTextSize) > bufferSize)
				s = bufferSize - kMessageTextSize;
			tmp[0] = data->buffer[s];
			data->buffer[s] = '\0';
			snprintf(buffer, bufferSize, "read remainder\n%s%c", data->buffer, tmp[0]);
			data->buffer[s] = tmp[0];
#else
			snprintf(buffer, bufferSize, "read remainder of request (%u bytes)", (unsigned)data->amt);
#endif
			return true;
    	case kFskHTTPClientRequestInstrMsgFinished:
			snprintf(buffer, bufferSize, "[%s] request finished (%s)", request->parsedUrl->host, request->parsedUrl->path);
			return true;
		case kFskHTTPClientRequestInstrMsgBadTE:
			snprintf(buffer, bufferSize, "unknown transfer encoding (%s)", (char*)msgData);
			return true;
		case kFskHTTPClientRequestInstrMsgErrString:
			snprintf(buffer, bufferSize, "%s", (char*)msgData);
			return true;
		case kFskHTTPClientRequestInstrMsgNotConnect:
			snprintf(buffer, bufferSize, "method is %s - should be CONNECT? if so, wouldn't be pipelinable since it's not GET or HEAD", (char*)msgData);
			return true;
		case kFskHTTPClientRequestInstrMsgReissue:
			snprintf(buffer, bufferSize, "[%d:%s] reissuing request (/%s)", request->owner->seqNum, request->parsedUrl->host, request->parsedUrl->path);
			return true;
		case kFskHTTPClientRequestInstrMsgCloseOnDone:
			snprintf(buffer, bufferSize, "[%s] close on done (/%s)", request->parsedUrl->host, request->parsedUrl->path);
			return true;
		case kFskHTTPClientAuthInstrMsgAddToReq:
		{
			char *authString;
			switch (auth->authType) {
				case kFskHTTPAuthDigest:	authString = "digest"; break;
				case kFskHTTPAuthNone:	authString = "none"; break;
				case kFskHTTPAuthBasic:	authString = "basic"; break;
				default:	authString = "unknown"; break;
			}
			snprintf(buffer, bufferSize, "adding (%s) auth for realm (%s)", authString, auth->realm);
			return true;
		}
		case kFskHTTPClientAuthInstrMsgCredFromURL:
			snprintf(buffer, bufferSize, "[%s] using credentials from URL (%s:%s)", request->parsedUrl->host, request->parsedUrl->username, request->parsedUrl->password);
			return true;
		case kFskHTTPClientAuthInstrMsgCredFromAPI:
			snprintf(buffer, bufferSize, "[%s] using supplied credentials from API (%s:%s)", client->host, client->auth->username, client->auth->credentials);
			return true;
    	case kFskHTTPClientAuthInstrMsgCredFromCallback:
		{
			char *credString;
			switch (request->owner->auth->credentialsType) {
				case kFskHTTPAuthCredentialsTypeString:	credString = "string"; break;
				case kFskHTTPAuthCredentialsTypeDigest:	credString = "digest"; break;
				default:	credString = "unknown"; break;
			}
			snprintf(buffer, bufferSize, "[%s] using credentials from callback (%s:%s:%s)", request->owner->host, credString, request->owner->auth->username, request->owner->auth->credentials);
			return true;
		}
    	case kFskHTTPClientAuthInstrMsgTooManyFailures:
			snprintf(buffer, bufferSize, "[%s] failed credentials too many times %s", request->owner->host, request->parsedUrl->path);
			return true;
    	case kFskHTTPClientAuthInstrMsgChallenge:
			snprintf(buffer, bufferSize, "authorization challenge: %s", FskHeaderFind("WWW-Authenticate", request->responseHeaders));
			return true;
		case kFskHTTPClientAuthInstrMsgGeneratingSessionKey:
			snprintf(buffer, bufferSize, "[%s] trying a session key: %s", request->owner->host, request->owner->auth->sessionKey);
			return true;
		case kFskHTTPClientAuthInstrMsgSessionKeyAccepted:
			snprintf(buffer, bufferSize, "[%s] session key accepted: %s", request->owner->host, request->owner->auth->sessionKey);
			return true;
		case kFskHTTPClientInstrMsgSystemHTTPProxy:
			if (gFskHTTPClient.gHTTPSystemProxyAddr) {
				FskNetIPandPortToString(gFskHTTPClient.gHTTPSystemProxyAddr, gFskHTTPClient.gHTTPSystemProxyPort, tmp);
				snprintf(buffer, bufferSize, "System proxy at [%s]", tmp);
			}
			else
				snprintf(buffer, bufferSize, "System proxy cleared");
			return true;
		case kFskHTTPClientInstrMsgSystemHTTPSProxy:
			if (gFskHTTPClient.gHTTPSSystemProxyAddr) {
				FskNetIPandPortToString(gFskHTTPClient.gHTTPSSystemProxyAddr, gFskHTTPClient.gHTTPSSystemProxyPort, tmp);
				snprintf(buffer, bufferSize, "Secure proxy at [%s]", tmp);
			}
			else
				snprintf(buffer, bufferSize, "System secure proxy cleared");
			return true;
		case kFskHTTPClientInstrMsgHTTPSProxy:
			if (gFskHTTPClient.gHTTPSProxyAddr) {
				FskNetIPandPortToString(gFskHTTPClient.gHTTPSProxyAddr, gFskHTTPClient.gHTTPSProxyPort, tmp);
				snprintf(buffer, bufferSize, "kconfig secure proxy set [%s]", tmp);
			}
			else
				snprintf(buffer, bufferSize, "kconfig secure proxy cleared");
			return true;
		case kFskHTTPClientInstrMsgHTTPProxy:
			if (gFskHTTPClient.gHTTPProxyAddr) {
				FskNetIPandPortToString(gFskHTTPClient.gHTTPProxyAddr, gFskHTTPClient.gHTTPProxyPort, tmp);
				snprintf(buffer, bufferSize, "kconfig proxy set [%s]", tmp);
			}
			else
				snprintf(buffer, bufferSize, "kconfig proxy cleared");
			return true;
		case kFskHTTPClientInstrMsgNew:
			if (!gFskHTTPClient.gHTTPSystemProxyAddr && !gFskHTTPClient.gHTTPSSystemProxyAddr && !gFskHTTPClient.gHTTPProxyAddr && !gFskHTTPClient.gHTTPSProxyAddr)
				snprintf(buffer, bufferSize, "[%d] new: no proxy", client->seqNum);
			else {
				snprintf(buffer, bufferSize, "[%d] new: a proxy is configured", client->seqNum);
			}
			return true;
 		case  kFskHTTPClientInstrMsgCancel:
			snprintf(buffer, bufferSize, "[%d:%s] cancel request", client->seqNum, client->host);
			return true;
		case kFskHTTPClientInstrMsgFlushRequests:
			snprintf(buffer, bufferSize, "flush requests - not implemented yet");
			return true;

		case kFskHTTPClientInstrMsgCycleBegin:
			snprintf(buffer, bufferSize, "[%d] begin cycle %d", client->seqNum, client->cycleNum);
			return true;
		case kFskHTTPClientInstrMsgCycleEnd:
			snprintf(buffer, bufferSize, "[%d] end cycle %d", client->seqNum, client->cycleNum);
			return true;

		default:
			return false;
	}
	return false;
}
#endif


