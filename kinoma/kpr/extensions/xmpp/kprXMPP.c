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
//#include "fips180.h"
#define __FSKNETUTILS_PRIV__
#include "FskNetUtils.h"
#include "FskSSL.h"

#include "kpr.h"
#include "kprAuthentication.h"
#include "kprHTTPClient.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprUtilities.h"
#include "kprXMPP.h"

typedef struct KprXMPPStruct KprXMPPRecord, *KprXMPP;
typedef struct KprXMPPWaiterStruct KprXMPPWaiterRecord, *KprXMPPWaiter;

//--------------------------------------------------
// XMPP
//--------------------------------------------------

enum {
	ELEMENT_PROTOTYPE,
	ATTRIBUTE_PROTOTYPE,
	DOCUMENT,
	ELEMENT,
	PARENT,
	ATTRIBUTES,
	ATTRIBUTE,
	CHILDREN,
	CHILD,
	PREFIX,
	VALUE,
	COUNT
};

typedef FskErr (*KprXMPPProcessElement)(KprXMPP self, KprXMLElement element);

struct KprXMPPStruct {
	KprXMPP next;
    FskList waiters;
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	Boolean bind;
	Boolean connected;
	char* connectionHost;
	UInt32 connectionPort;
	KprXMLElement element;
	XML_Parser expat;
	char* fjid;
	UInt32 id;
	char* jid;
	Boolean registered;
	char* resource;
	char* domain;
	char* username;
	char* password;
	UInt32 ping;
	FskTimeCallBack pingTimeCallback;
	KprXMPPProcessElement process;
	char* proxyAuthentication;
	char* proxyHost;
	UInt32 proxyPort;
	KprXMLElement queue;
	Boolean secured;
	Boolean session;
	FskSocket socket;
	KprXMLElement stream;
	FskThreadDataHandler socketHandler;
	char* scramMessage;
	char* scramVerifier;
	char* verification;
	void* certificate;
	UInt32 certificateSize;
	char *policies;
	FskInstrumentedItemDeclaration
};

struct KprXMPPWaiterStruct {
	KprXMPPWaiter next;
	UInt32 id;
	KprMessage message;
};

#define kPingPeriod 300
#define kPacketBufSize 4095
#define kXMPPBufferSize (128 * 1024)

#define kXMPPStartTLSMessage "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>"
#define kXMPPAuthPlainStartMessage "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'>"
#define kXMPPAuthPlainStopMessage "</auth>"
#define kXMPPAuthDigestMD5Message "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='DIGEST-MD5'/>"
#define kXMPPAuthSCRAMSHA1StartMessage "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='SCRAM-SHA-1'>"
#define kXMPPAuthSCRAMSHA1StopMessage "</auth>"
#define kXMPPAuthDigestMD5ResponseStartMessage "<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
#define kXMPPAuthDigestMD5ResponseStopMessage "</response>"
#define kXMPPAuthSCRAMSHA1ResponseStartMessage "<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
#define kXMPPAuthSCRAMSHA1ResponseStopMessage "</response>"


static FskErr KprXMPPNew(KprXMPP* it, char* domain, char* username, char* password, char* resource);
static void KprXMPPDispose(KprXMPP self);
static void KprXMPPApplicationCallback(KprXMPP self, char* id, void* param);
static void KprXMPPCancelRegistration(KprXMPP self);
static void KprXMPPChangePassword(KprXMPP self, char* password);
static void KprXMPPConnect(KprXMPP self, char* host, UInt32 port);
static FskErr KprXMPPConnectCallback(FskSocket skt, void *refCon);
static FskErr KprXMPPConnectProxyCallback(FskSocket skt, void *refCon);
static void KprXMPPConnectedProxyCallback(FskThreadDataHandler handler UNUSED, FskThreadDataSource source, void *refCon);
static void KprXMPPConnected(KprXMPP self);
static void KprXMPPDisconnect(KprXMPP self);
static void KprXMPPDisposeStream(KprXMPP self);
static void KprXMPPError(KprXMPP self, FskErr err);
static char* KprXMPPGetNextPingID(KprXMPP self);
static void KprXMPPParse(FskThreadDataHandler handler UNUSED, FskThreadDataSource source, void *refCon);
static void KprXMPPParserDefault(void *data, const char *theText, int theSize);
static void KprXMPPParserStartTag(void *data, const char *name, const char **attributes);
static void KprXMPPParserStopTag(void *data, const char *name);
static void KprXMPPPingTimeCallback(FskTimeCallBack callback, const FskTime time, void *param);
static FskErr KprXMPPProcessStreamChallenge(KprXMPP self, KprXMLElement challenge);
static FskErr KprXMPPProcessStreamFeatures(KprXMPP self, KprXMLElement features);
static FskErr KprXMPPProcessStreamIQ(KprXMPP self, KprXMLElement iq);
static FskErr KprXMPPProcessStreamSASLFailure(KprXMPP self, KprXMLElement failure);
static FskErr KprXMPPProcessStreamSASLSuccess(KprXMPP self, KprXMLElement success);
static FskErr KprXMPPProcessStreamTLSFailure(KprXMPP self, KprXMLElement failure);
static FskErr KprXMPPProcessStreamTLSProceed(KprXMPP self, KprXMLElement proceed);
static FskErr KprXMPPProcessStreamTLSProceedCallback(FskSocket skt, void *refCon);
static FskErr KprXMPPReceiveStanza(KprXMPP self, KprXMLElement element);
static void KprXMPPReceiveStanzaAux(xsMachine* the, KprXMLElement element, xsSlot parent, xsIntegerValue index);
static void KprXMPPReceiveStanzaCallback(KprXMPP self, void* stanza);
static void KprXMPPPasswordChanged(KprXMPP self);
static void KprXMPPPasswordChangeError(KprXMPP self, FskErr err);
static void KprXMPPRegistered(KprXMPP self);
static void KprXMPPRegisterError(KprXMPP self, FskErr err);
static void KprXMPPRegistrationCanceled(KprXMPP self);
static void KprXMPPRegistrationCancelError(KprXMPP self, FskErr err);
static FskErr KprXMPPSend(KprXMPP self, char* string, UInt32 size);
static FskErr FskXMPPSendBind(KprXMPP self);
static FskErr FskXMPPSendRegister(KprXMPP self, char* password);
static FskErr FskXMPPSendRegisterRemove(KprXMPP self);
static FskErr FskXMPPSendSession(KprXMPP self);
static void KprXMPPSendStanza(KprXMPP self, void* stanza);
static void KprXMPPSendStanzaAux(xsMachine* the, KprXMPP self);
static FskErr KprXMPPSendXMLCDATA(KprXMPP self, char* string, UInt32 size);
static FskErr KprXMPPSendXMLEntity(KprXMPP self, char* string, UInt32 size, int flag);
static FskErr KprXMPPSendXMLEntityAttribute(KprXMPP self, char* string, UInt32 size);
static FskErr KprXMPPSendXMLEntityData(KprXMPP self, char* string, UInt32 size);
static FskErr KprXMPPStreamStart(KprXMPP self);
static UInt32 KprXMPPUnescapeCDATABufferMunger(char* string, UInt32 size);
static FskErr KprXMPPSetCertificate(KprXMPP self, void* certificate, UInt32 size, char *policies);

#define kprXMPPNamespace "http://www.kinoma.com/kpr/1"

static void KprXMPPServiceStart(KprService self, FskThread thread, xsMachine* the);
static void KprXMPPServiceStop(KprService self);
static void KprXMPPServiceCancel(KprService self, KprMessage message);
static void KprXMPPServiceInvoke(KprService self, KprMessage message);
static FskErr KprXMPPWaiterNew(KprXMPPWaiter* it, UInt32 id, KprMessage message);
static void KprXMPPWaiterDispose(KprXMPPWaiter self);

static FskList gXMPPList = NULL;

KprServiceRecord gXMPPService = {
	NULL,
	kprServicesThread,
	"xmpp:",
	NULL,
	NULL,
	KprServiceAccept,
	KprXMPPServiceCancel,
	KprXMPPServiceInvoke,
	KprXMPPServiceStart,
	KprXMPPServiceStop,
	NULL,
	NULL,
	NULL
};

//--------------------------------------------------
// EXTENSION
//--------------------------------------------------

FskExport(FskErr) kprXMPP_fskLoad(FskLibrary library)
{
	KprServiceRegister(&gXMPPService);
	return kFskErrNone;
}

FskExport(FskErr) kprXMPP_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

void KprXMPPServiceStart(KprService self, FskThread thread, xsMachine* the)
{
	self->machine = the;
	self->thread = thread;
}

void KprXMPPServiceStop(KprService self UNUSED)
{
}

//--------------------------------------------------
// INSTRUMENTATION
//--------------------------------------------------

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord gKprXMPPInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprXMPP", FskInstrumentationOffset(KprXMPPRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

//--------------------------------------------------
// XMPP
//--------------------------------------------------

static FskErr KprXMPPNew(KprXMPP* it, char* domain, char* username, char* password, char* resource)
{
	FskErr err = kFskErrNone;
	KprXMPP self = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprXMPPRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprXMPPInstrumentation);
	self->domain = FskStrDoCopy(domain);
	bailIfNULL(self->domain);
	self->username = FskStrDoCopy(username);
	bailIfNULL(self->username);
	self->password = FskStrDoCopy(password);
	bailIfNULL(self->password);
	if (resource) {
		self->resource = FskStrDoCopy(resource);
		bailIfNULL(self->resource);
	}
	bailIfError(FskMemPtrNewClear(FskStrLen(username) + 1 + FskStrLen(domain) + 1, &self->jid));
	FskStrCopy(self->jid, username);
	FskStrCat(self->jid, "@");
	FskStrCat(self->jid, domain);
	if (resource) {
		bailIfError(FskMemPtrNewClear(FskStrLen(self->jid) + 1 + FskStrLen(resource) + 1, &self->fjid));
		FskStrCopy(self->fjid, self->jid);
		FskStrCat(self->fjid, "/");
		FskStrCat(self->fjid, resource);
	}
	bailIfNULL(self->jid);
#ifdef mxDebug
	self->policies = FskStrDoCopy("allowOrphan, allowSelfSigned");
#else
	self->policies = FskStrDoCopy("allowOrphan");
#endif
	bailIfNULL(self->policies);
	FskTimeCallbackNew(&self->pingTimeCallback);
	return err;
bail:
	if (self)
		KprXMPPDispose(self);
	return err;
}

static void KprXMPPDispose(KprXMPP self)
{
	if (self) {
		KprXMPPDisconnect(self);
		FskTimeCallbackDispose(self->pingTimeCallback);
		FskMemPtrDispose(self->fjid);
		FskMemPtrDispose(self->jid);
		FskMemPtrDispose(self->resource);
		FskMemPtrDispose(self->password);
		FskMemPtrDispose(self->username);
		FskMemPtrDispose(self->domain);
		FskMemPtrDispose(self->connectionHost);
		FskMemPtrDispose(self->proxyAuthentication);
		FskMemPtrDispose(self->proxyHost);
		FskMemPtrDispose(self->certificate);
		FskMemPtrDispose(self->policies);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

static void KprXMPPApplicationCallback(KprXMPP self, char* id, void* param)
{
	FskErr err = (FskErr)param;
	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	xsVar(0) = xsGet(self->slot, xsID_behavior);
	if (xsFindResult(xsVar(0), xsID(id))) {
		if (err)
			(void)xsCallFunction2(xsResult, xsVar(0), self->slot, xsInteger(err));
		else
			(void)xsCallFunction1(xsResult, xsVar(0), self->slot);
	}
	xsEndHostSandboxCode();
}

static void KprXMPPCancelRegistration(KprXMPP self)
{
	FskErr err = kFskErrNone;
	
	bailIfError(self->connected && self->secured ? FskXMPPSendRegisterRemove(self) : kFskErrUnimplemented);
bail:
	return;
}

static void KprXMPPChangePassword(KprXMPP self, char* password)
{
	FskErr err = kFskErrNone;
	
	bailIfError(self->connected && self->secured ? FskXMPPSendRegister(self, password) : kFskErrUnimplemented);
bail:
	FskMemPtrDispose(password);
	return;
}

static void KprXMPPConnect(KprXMPP self, char* domain, UInt32 port)
{
	FskErr err = kFskErrNone;
	UInt32 size;
	
	if (self->connected) return;
	FskListAppend(&gXMPPList, self);
	if (self->proxyHost) {
		if (domain) {
			self->connectionHost = domain;
			self->connectionPort = port;
			domain = NULL;
		}
		else {
			UInt16 connectionPort = 0;
			char* connectionHost = NULL;

			size = FskStrLen(self->domain) + 19;
			bailIfError(FskMemPtrNewClear(size, &domain));
			FskStrCopy(domain, "_xmpp-client._tcp.");
			FskStrCat(domain, self->domain);
			
			bailIfError(FskNetServerSelection(domain, &connectionHost, &connectionPort));
			self->connectionHost = connectionHost;
			self->connectionPort = connectionPort;
		}
		bailIfError(FskNetConnectToHost(self->proxyHost, self->proxyPort, false, KprXMPPConnectProxyCallback, self, 0, NULL, "XMPP"));
	}
	else {
		if (domain) {
			bailIfError(FskNetConnectToHost(domain, port, false, KprXMPPConnectCallback, self, 0, NULL, "XMPP"));
		}
		else {
			size = FskStrLen(self->domain) + 19;
			bailIfError(FskMemPtrNewClear(size, &domain));
			FskStrCopy(domain, "_xmpp-client._tcp.");
			FskStrCat(domain, self->domain);
			bailIfError(FskNetConnectToHost(domain, port, false, KprXMPPConnectCallback, self, kConnectFlagsServerSelection, NULL, "XMPP"));
		}
	}
bail:
	FskMemPtrDispose(domain);
	return;
}

static FskErr KprXMPPConnectCallback(FskSocket skt, void *refCon)
{
	FskErr err = kFskErrNone;
	KprXMPP self = refCon;
	
	if (!skt || 0 == skt->ipaddrRemote) {
		BAIL(kFskErrSocketNotConnected);
	}
	self->socket = skt;
	FskNetSocketReceiveBufferSetSize(self->socket, kXMPPBufferSize);
	bailIfError(KprXMPPStreamStart(self));
bail:
	if (err)
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPApplicationCallback, self, "onXMPPError", (void*)err, NULL);
	return err;
}

#define kProxyConnectTemplate "CONNECT %s:%lu HTTP/1.0\r\n"

#define kProxyConnectAuthenticationTemplate \
	"Proxy-authorization: basic %s\r\n"

static FskErr KprXMPPConnectProxyCallback(FskSocket skt, void *refCon)
{
	FskErr err = kFskErrNone;
	KprXMPP self = refCon;
	int size = 0;
	char* string = NULL;
	
	if (!skt || 0 == skt->ipaddrRemote) {
		BAIL(kFskErrSocketNotConnected);
	}
	self->socket = skt;
	size = sizeof(kProxyConnectTemplate) - 4 + FskStrLen(self->connectionHost) + 16 + 1;
	bailIfError(FskMemPtrNewClear(size, &string));
	snprintf(string, size, kProxyConnectTemplate, self->connectionHost, self->connectionPort );
	bailIfError(FskNetSocketSendTCP(self->socket, string, FskStrLen(string), &size));
	if (self->proxyAuthentication) {
		size = sizeof(kProxyConnectAuthenticationTemplate) - 2 + FskStrLen(self->proxyAuthentication) + 1;
		bailIfError(FskMemPtrRealloc(size, &string));
		snprintf(string, size, kProxyConnectAuthenticationTemplate, self->proxyAuthentication);
		bailIfError(FskNetSocketSendTCP(self->socket, string, FskStrLen(string), &size));
	}
	bailIfError(FskNetSocketSendTCP(self->socket, "\r\n", 2, &size));
	FskThreadAddDataHandler(&self->socketHandler, (FskThreadDataSource)self->socket, KprXMPPConnectedProxyCallback, true, false, self);
bail:
	FskMemPtrDispose(string);
	if (err)
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPApplicationCallback, self, "onXMPPError", (void*)err, NULL);
	FskMemPtrDisposeAt(&self->connectionHost);
	return err;
}

static FskErr KprXMPPReadline(FskSocket socket, char *buf, int bufsize, int *pRead )
{
	FskErr e = kFskErrNone;
	int	x, back = 1, EOL = 0;
	
	for (x = 0; x < bufsize && (1 == back) && (0 == EOL); x++) {
		e = FskNetSocketRecvTCP(socket, buf + x, 1, &back );
		if ((kFskErrNoData != e) && (kFskErrNone != e)) {
			return e;
		}
		EOL = buf[x]=='\n' ? 1 : 0;
	}
	*pRead = x;
	return kFskErrNone;
}

static void KprXMPPConnectedProxyCallback(FskThreadDataHandler handler UNUSED, FskThreadDataSource source, void *refCon)
{
	FskErr err = kFskErrNone;
	KprXMPP self = refCon;
	int size = -1, prevSize = 0;
	FskSocket socket = (FskSocket)source;
	char response[64];
	
	bailIfError(KprXMPPReadline(socket, response, sizeof(response), &size)); 
	
	if (FskStrCompareWithLength(response, "HTTP/1.0 200", 12) && FskStrCompareWithLength(response, "HTTP/1.1 200", 12)) {
		BAIL(kFskErrSocketNotConnected);
	}
	else {
		prevSize = size;
		//  We have to make sure we keep reading until we get an empty line
		while (kFskErrNone == err) {
			err = KprXMPPReadline(socket,response, sizeof(response), &size);
			if ((response[0] == '\n') || ((response[0]=='\r') && (response[1] == '\n'))) {
				if (prevSize != sizeof(response))
					break; // Found empty line
			}
			prevSize = size;
		}
	}
	FskThreadRemoveDataHandler(&self->socketHandler);
	self->socketHandler = NULL;
	FskNetSocketReceiveBufferSetSize(self->socket, kXMPPBufferSize);
	bailIfError(KprXMPPStreamStart(self));
bail:
	if (err) {
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPApplicationCallback, self, "onXMPPError", (void*)err, NULL);
		FskThreadRemoveDataHandler(&self->socketHandler);
		self->socketHandler = NULL;
	}
	return;
}

static void KprXMPPConnected(KprXMPP self)
{
	FskTimeCallbackScheduleFuture(self->pingTimeCallback, kPingPeriod, 0, KprXMPPPingTimeCallback, self);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPApplicationCallback, self, "onXMPPConnected", NULL, NULL);
}

static void KprXMPPDisconnect(KprXMPP self)
{
	if (self->socket) {
		if (self->expat) {
			XML_ParserFree(self->expat);
			self->expat = NULL;
		}
		if (self->connected)
			KprXMPPSend(self, "</stream:stream>", 16);
		FskListRemove(&gXMPPList, self);
		self->connected = false;
		KprXMPPDisposeStream(self);
		FskThreadRemoveDataHandler(&self->socketHandler);
		FskNetSocketClose(self->socket);
		self->socket = NULL;
		self->bind = false;
		self->process = NULL;
		self->secured = false;
		self->session = false;
		FskMemPtrDisposeAt(&self->verification);
	}
}

static void KprXMPPDisposeStream(KprXMPP self)
{
	KprXMLElement element;
	while ((element = FskListRemoveFirst(&self->queue))) {
		KprXMLElementDispose(element);
	}
	KprXMLElementDispose(self->stream);
	self->element = NULL;
	self->stream = NULL;
}

static void KprXMPPError(KprXMPP self, FskErr err)
{
	KprXMPPDisconnect(self);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPApplicationCallback, self, "onXMPPError", (void*)err, NULL);
	return;
}

static char* KprXMPPGetNextPingID(KprXMPP self)
{
	char number[16];
	FskStrNumToStr(self->ping++, number, 15);
    return FskStrDoCat("ping-", number);
}

static void KprXMPPParse(FskThreadDataHandler handler UNUSED, FskThreadDataSource source, void *refCon)
{
	FskErr err = kFskErrNone;
	KprXMPP self = refCon;
	int size = -1;
	char* buffer;
	KprXMLElement element;
	
	FskSocket socket = (FskSocket)source;
	
	while (self->expat) {
		buffer = XML_GetBuffer(self->expat, kPacketBufSize);
		bailIfError(FskNetSocketRecvTCP(socket, buffer, kPacketBufSize + 1, &size));
		if (size == 0)
			continue;
		buffer[size] = 0;
		FskInstrumentedItemPrintfVerbose(self, "PARSE %d\n%s", size, buffer);
		if (!XML_ParseBuffer(self->expat, size, 0)) {
			BAIL(kFskErrBadData);
		}
		while ((element = FskListRemoveFirst(&self->queue))) {
			if (!self->connected) {
				if (KprXMLElementIsEqual(element, "http://etherx.jabber.org/streams", "features")) {
					bailIfError(KprXMPPProcessStreamFeatures(self, element));
				}
				else if (KprXMLElementIsEqual(element, "urn:ietf:params:xml:ns:xmpp-tls", "proceed")) {
					bailIfError(KprXMPPProcessStreamTLSProceed(self, element));
					element = NULL;
				}
				else if (KprXMLElementIsEqual(element, "urn:ietf:params:xml:ns:xmpp-tls", "failure")) {
					bailIfError(KprXMPPProcessStreamTLSFailure(self, element));
				}
				else if (KprXMLElementIsEqual(element, "urn:ietf:params:xml:ns:xmpp-sasl", "failure")) {
					bailIfError(KprXMPPProcessStreamSASLFailure(self, element));
				}
				else if (KprXMLElementIsEqual(element, "urn:ietf:params:xml:ns:xmpp-sasl", "success")) {
					bailIfError(KprXMPPProcessStreamSASLSuccess(self, element));
					element = NULL;
				}
				else if (KprXMLElementIsEqual(element, "jabber:client", "iq")) {
					bailIfError(KprXMPPProcessStreamIQ(self, element));
				}
				else if (KprXMLElementIsEqual(element, "urn:ietf:params:xml:ns:xmpp-sasl", "challenge")) {
					bailIfError(KprXMPPProcessStreamChallenge(self, element));
				}
			}
			else {
				bailIfError(KprXMPPReceiveStanza(self, element));
			}
			KprXMLElementDispose(element);
		}
	}
	return;
bail:
	if (err != kFskErrNoData)
		KprXMPPError(self, err);
	return;
}

static void KprXMPPParserDefault(void *data, const char *text, int size)
{
	FskErr err = kFskErrNone;
	KprXMPP self = data;
	KprXMLElement element = self->element;
	const char* attributes = NULL;
	if (element) {
		if (element->name) {
			bailIfError(KprXMLElementNew(&element, self->element, NULL, &attributes));
			self->element = element;
		}
		bailIfError(FskMemPtrRealloc(element->valueSize + size + 1, &element->value));
		FskMemCopy(element->value + element->valueSize, text, size);
		element->valueSize += size;
		element->value[element->valueSize] = 0;
	}
bail:
	return;
}

static void KprXMPPParserStartTag(void *data, const char *name, const char **attributes)
{
	FskErr err = kFskErrNone;
	KprXMPP self = data;
	KprXMLElement element;
	
	if (!self->element || self->element->name) {
		bailIfError(KprXMLElementNew(&element, self->element, name, attributes));
	}
	else {
		bailIfError(KprXMLElementNew(&element, self->element->owner, name, attributes));
	}
	if (!self->element)
		self->stream = element;
	self->element = element;
bail:
	return;
}

static void KprXMPPParserStopTag(void *data, const char *name UNUSED)
{
	KprXMPP self = data;
	if (!self->element->name) {
		self->element = self->element->owner;
	}
	if (self->element && self->stream) {
		if (self->element->owner == self->stream) {
			FskListAppend(&self->queue, self->element);
			self->stream->element = NULL;
		}
	}
	self->element = self->element->owner;
	return;
}

static void KprXMPPPingTimeCallback(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *param)
{
	FskErr err = kFskErrNone;
	KprXMPP self = param;
	char* id = KprXMPPGetNextPingID(self);
	bailIfError(KprXMPPSend(self, "<iq from='", 0));
	if (self->fjid) {
		bailIfError(KprXMPPSendXMLEntityAttribute(self, self->fjid, 0));
	}
	else {
		bailIfError(KprXMPPSendXMLEntityAttribute(self, self->jid, 0));
	}
	bailIfError(KprXMPPSend(self, "' to='", 0));
	bailIfError(KprXMPPSendXMLEntityAttribute(self, self->domain, 0));
	bailIfError(KprXMPPSend(self, "' id='", 0));
	bailIfError(KprXMPPSendXMLEntityAttribute(self, id, 0));
	bailIfError(KprXMPPSend(self, "' type='get'>", 0));
	bailIfError(KprXMPPSend(self, "<ping xmlns='urn:xmpp:ping'/>", 0));
	bailIfError(KprXMPPSend(self, "</iq>", 0));
	FskMemPtrDispose(id);
	FskTimeCallbackScheduleFuture(self->pingTimeCallback, kPingPeriod, 0, KprXMPPPingTimeCallback, self);
bail:
	return;
}

static FskErr KprXMPPProcessStreamChallenge(KprXMPP self, KprXMLElement challenge)
{
	FskErr err = kFskErrNone;
	char* base64Challenge = NULL;
	char* uri = NULL;
	char* sasl = NULL;
	char* response = NULL;
	char* base64Response = NULL;
	
	base64Challenge = KprXMLElementGetValue(challenge);
	if (self->scramMessage) {
		bailIfError(KprAuthenticationSCRAMSHA1Response(base64Challenge, 0, self->scramMessage, 0, self->password, 0, &response, &self->scramVerifier));
		bailIfError(KprXMPPSend(self, kXMPPAuthSCRAMSHA1ResponseStartMessage, FskStrLen(kXMPPAuthSCRAMSHA1ResponseStartMessage)));
		bailIfError(KprXMPPSend(self, response, FskStrLen(response)));
		bailIfError(KprXMPPSend(self, kXMPPAuthSCRAMSHA1ResponseStopMessage, FskStrLen(kXMPPAuthSCRAMSHA1ResponseStopMessage)));
		FskMemPtrDispose(self->scramMessage);
		self->scramMessage = NULL;
	}
	else {
		FskStrB64Decode(base64Challenge, FskStrLen(base64Challenge), &sasl, NULL);
		if (!self->verification) {
			int size = FskStrLen(self->domain);
			bailIfError(FskMemPtrNewClear(5 + size + 1, &uri));
			FskStrCopy(uri, "xmpp/");
			FskStrCat(uri, self->domain);
			bailIfError(KprAuthenticationDigestMD5(self->username, 0, self->password, 0, uri, 0, sasl, &response, &self->verification));

			FskStrB64Encode(response, FskStrLen(response), &base64Response, NULL, false);

			bailIfError(KprXMPPSend(self, kXMPPAuthDigestMD5ResponseStartMessage, FskStrLen(kXMPPAuthDigestMD5ResponseStartMessage)));
			bailIfError(KprXMPPSend(self, base64Response, FskStrLen(base64Response)));
			bailIfError(KprXMPPSend(self, kXMPPAuthDigestMD5ResponseStopMessage, FskStrLen(kXMPPAuthDigestMD5ResponseStopMessage)));
		}
		else {
			bailIfError(FskStrCompare(self->verification, sasl) != 0);
			bailIfError(KprXMPPSend(self, kXMPPAuthDigestMD5ResponseStartMessage, FskStrLen(kXMPPAuthDigestMD5ResponseStartMessage)));
			bailIfError(KprXMPPSend(self, kXMPPAuthDigestMD5ResponseStopMessage, FskStrLen(kXMPPAuthDigestMD5ResponseStopMessage)));
		}
	}
bail:	
	FskMemPtrDispose(base64Response);
	FskMemPtrDispose(response);
	FskMemPtrDispose(sasl);
	FskMemPtrDispose(uri);
	if (err)
		FskMemPtrDisposeAt(&self->verification);
	return err;
}

static FskErr KprXMPPProcessStreamFeatures(KprXMPP self, KprXMLElement features)
{
	FskErr err = kFskErrNone;
	KprXMLElement element;
	int size;
	char* base64 = NULL;
	char* string = NULL;
	
	if (KprXMLElementGetFirstElement(features, "urn:ietf:params:xml:ns:xmpp-tls", "starttls")) {
		bailIfError(KprXMPPSend(self, kXMPPStartTLSMessage, FskStrLen(kXMPPStartTLSMessage)));
	}
	else if (!self->registered) {
		if (KprXMLElementGetFirstElement(features, "http://jabber.org/features/iq-register", "register")) {
			bailIfError(self->secured ? FskXMPPSendRegister(self, NULL) : kFskErrUnimplemented); // do not register if not secured
		}
		else {
			BAIL(kFskErrUnimplemented); // not supported
		}
	}
	else if ((element = KprXMLElementGetFirstElement(features, "urn:ietf:params:xml:ns:xmpp-sasl", "mechanisms"))) {
		UInt32 selectedMechanism = 0x0;
		for (element = KprXMLElementGetFirstElement(element, "urn:ietf:params:xml:ns:xmpp-sasl", "mechanism"); element; element = KprXMLElementGetNextElement(element, "urn:ietf:params:xml:ns:xmpp-sasl", "mechanism")) {
			if (!FskStrCompare(KprXMLElementGetValue(element), "PLAIN")) {
				if (self->secured) // happy with PLAIN if secured
					selectedMechanism |= 0x1;
			}
			else if (!FskStrCompare(KprXMLElementGetValue(element), "DIGEST-MD5")) {
				selectedMechanism |= 0x2;
			}
			else if (!FskStrCompare(KprXMLElementGetValue(element), "SCRAM-SHA-1")) {
#if 0
				selectedMechanism |= 0x4;
#endif
			}
		}
		if (selectedMechanism & 0x4) { // SCRAM-SHA-1
			bailIfError(KprAuthenticationSCRAMSHA1Message(self->username, 0, &self->scramMessage));
			bailIfError(KprAuthenticationSCRAMSHA1MessageToken(self->scramMessage, 0, &string));
			bailIfError(KprXMPPSend(self, kXMPPAuthSCRAMSHA1StartMessage, FskStrLen(kXMPPAuthSCRAMSHA1StartMessage)));
			bailIfError(KprXMPPSend(self, string, FskStrLen(string)));
			bailIfError(KprXMPPSend(self, kXMPPAuthSCRAMSHA1StopMessage, FskStrLen(kXMPPAuthSCRAMSHA1StopMessage)));
		}
		else if (selectedMechanism & 0x2) { // DIGEST-MD5
			bailIfError(KprXMPPSend(self, kXMPPAuthDigestMD5Message, FskStrLen(kXMPPAuthDigestMD5Message)));
		}
		else if (selectedMechanism & 0x1) { // PLAIN
			char* username = self->username;
			UInt32 usernameSize = FskStrLen(username);
			char* password = self->password;
			UInt32 passwordSize = FskStrLen(password);
			UInt32 base64Size;
			
			size = 0 + 1/*\0*/ + usernameSize + 1/*\0*/ + passwordSize;
			bailIfError(FskMemPtrNewClear(size, &string));
			FskStrCopy(string + 0 + 1, username);
			FskStrCopy(string + 0 + 1 + usernameSize + 1, password);
			FskStrB64Encode(string, size, &base64, &base64Size, false);
			if (base64Size == 0)
				BAIL(kFskErrMemFull);
			base64Size--; // do not count NULL termination!
			bailIfError(KprXMPPSend(self, kXMPPAuthPlainStartMessage, FskStrLen(kXMPPAuthPlainStartMessage)));
			bailIfError(KprXMPPSend(self, base64, base64Size));
			bailIfError(KprXMPPSend(self, kXMPPAuthPlainStopMessage, FskStrLen(kXMPPAuthPlainStopMessage)));
		}
		else {
			BAIL(kFskErrUnimplemented); // not supported
		}
	}
	else {
		if (KprXMLElementGetFirstElement(features, "urn:ietf:params:xml:ns:xmpp-bind", "bind")) {
			self->bind = true;
			bailIfError(FskXMPPSendBind(self));
		}
		if (KprXMLElementGetFirstElement(features, "urn:ietf:params:xml:ns:xmpp-session", "session")) {
			bailIfError(FskXMPPSendSession(self));
			self->session = true;
		}
		self->connected = (self->registered && !self->session && ! self->session);
		if (self->connected)
			KprXMPPConnected(self);
	}
bail:	
	if (base64)
		FskMemPtrDispose(base64);
	if (string)
		FskMemPtrDispose(string);
	return err;
}

static FskErr KprXMPPProcessStreamIQ(KprXMPP self, KprXMLElement iq)
{
	FskErr err = kFskErrNone;
	KprXMLElement element;
	char* id = KprXMLElementGetAttribute(iq, "id");
	char* type = KprXMLElementGetAttribute(iq, "type");
	
	if ((element = KprXMLElementGetFirstElement(iq, "urn:ietf:params:xml:ns:xmpp-bind", "bind"))) {
		if ((element = KprXMLElementGetFirstElement(element, "urn:ietf:params:xml:ns:xmpp-bind", "jid"))) {
			char* jid = (element && element->element && element->element->value) ? element->element->value : NULL;
			if (jid) {
				char* slash = FskStrChr(jid, '/');
				if (self->resource && slash) {
					if (FskStrCompare(self->resource, slash + 1)) {
						FskMemPtrDispose(self->resource);
						self->resource = FskStrDoCopy(slash + 1);
						bailIfNULL(self->resource);
					}
				}
				else if (!self->resource && slash) {
					self->resource = FskStrDoCopy(slash + 1);
					bailIfNULL(self->resource);
				}
				FskMemPtrDispose(self->fjid);
				bailIfError(FskMemPtrNewClear(FskStrLen(self->jid) + 1 + FskStrLen(self->resource) + 1, &self->fjid));
				FskStrCopy(self->fjid, self->jid);
				FskStrCat(self->fjid, "/");
				FskStrCat(self->fjid, self->resource);
			}
		}
		self->bind = false;
	}
	else if (!FskStrCompare("error", type) && !FskStrCompare("change-password-0", id)) {
		KprXMPPPasswordChangeError(self, kFskErrNone); // placeholder kFskErrNone
	}
	else if (!FskStrCompare("result", type) && !FskStrCompare("change-password-0", id)) {
		KprXMPPPasswordChanged(self);
	}
	else if (!FskStrCompare("error", type) && !FskStrCompare("register-0", id)) {
		KprXMPPRegisterError(self, kFskErrNone); // placeholder kFskErrNone
	}
	else if (!FskStrCompare("result", type) && !FskStrCompare("register-0", id)) {
		KprXMPPRegistered(self);
	}
	else if (!FskStrCompare("error", type) && !FskStrCompare("register-remove-0", id)) {
		KprXMPPRegistrationCancelError(self, kFskErrNone); // placeholder kFskErrNone
	}
	else if (!FskStrCompare("result", type) && !FskStrCompare("register-remove-0", id)) {
		KprXMPPRegistrationCanceled(self);
	}
	else if (KprXMLElementGetFirstElement(iq, "urn:ietf:params:xml:ns:xmpp-session", "session")) {
		self->session = false;
	}
	else if (!FskStrCompare("result", type) && !FskStrCompare("session-0", id)) {
		self->session = false;
	}
	self->connected = (self->registered && !self->session && ! self->session);
	if (self->connected)
		KprXMPPConnected(self);
bail:	
	return err;
}

static FskErr KprXMPPProcessStreamSASLFailure(KprXMPP self UNUSED, KprXMLElement failure UNUSED)
{
	FskErr err = kFskErrAuthFailed;
	return err;
}

static FskErr KprXMPPProcessStreamSASLSuccess(KprXMPP self, KprXMLElement success)
{
	FskErr err = kFskErrNone;
	if (self->scramVerifier) {
		char* value = (success && success->element && success->element->value) ? success->element->value : NULL;
		if (!(value && !FskStrCompare(self->scramVerifier, value)))
			err = kFskErrAuthFailed;
		FskMemPtrDispose(self->scramVerifier);
		self->scramVerifier = NULL;
	}
	KprXMLElementDispose(success);
	if (err == kFskErrNone) {
		bailIfError(KprXMPPStreamStart(self));
	}
	else
		KprXMPPError(self, err);
bail:
	return err;
}

static FskErr KprXMPPProcessStreamTLSFailure(KprXMPP self UNUSED, KprXMLElement failure UNUSED)
{
	FskErr err = kFskErrSSLHandshakeFailed;
	return err;
}

static FskErr KprXMPPProcessStreamTLSProceed(KprXMPP self UNUSED, KprXMLElement proceed UNUSED)
{
	FskErr err = kFskErrNone;
	void* ssl;
	FskSocketCertificateRecord cert = {
		NULL, 0,
		self->policies,
		self->connectionHost,
		NULL, 0,
	};
	KprXMLElementDispose(proceed);

	FskThreadRemoveDataHandler(&self->socketHandler);
	bailIfError(FskSSLAttach(&ssl, self->socket));
	bailIfError(FskSSLLoadCerts(ssl, &cert));
	bailIfError(FskSSLHandshake(ssl, KprXMPPProcessStreamTLSProceedCallback, self, true));
bail:
	return err;
}

static FskErr KprXMPPProcessStreamTLSProceedCallback(FskSocket skt UNUSED, void *refCon)
{
	KprXMPP self = refCon;
	self->secured = true;
	return KprXMPPStreamStart(self);
}

static void KprXMPPMessageCallback(KprMessage message, void* it)
{
	FskErr err = kFskErrNone;
	KprXMPP self = it;
	FskAssociativeArrayIterator iterate = NULL;
	char* from = KprMessageGetRequestHeader(message, "from");
	char* id = KprMessageGetRequestHeader(message, "id");
	char* to = KprMessageGetRequestHeader(message, "to");
	bailIfError(KprXMPPSend(self, "<iq from='", 0));
	bailIfError(KprXMPPSendXMLEntityAttribute(self, to, 0));
	bailIfError(KprXMPPSend(self, "' id='", 0));
	bailIfError(KprXMPPSendXMLEntityAttribute(self, id, 0));
	bailIfError(KprXMPPSend(self, "' to='", 0));
	bailIfError(KprXMPPSendXMLEntityAttribute(self, from, 0));
	bailIfError(KprXMPPSend(self, "' type='result'><message xmlns='", 0));
	bailIfError(KprXMPPSend(self, kprXMPPNamespace, 0));
	bailIfError(KprXMPPSend(self, "'>", 0));
	if (message->response.body) {
		if (1) {
			xsStringValue body;
			xsBeginHostSandboxCode(self->the, self->code);
			xsVars(1);
			xsVar(0) = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(message->response.size));
			FskMemCopy(xsGetHostData(xsVar(0)), message->response.body, message->response.size);
			body = xsToString(xsVar(0));
			bailIfError(KprXMPPSend(self, "<body><chunk>", 0));
			bailIfError(KprXMPPSend(self, body, FskStrLen(body)));
			bailIfError(KprXMPPSend(self, "</chunk></body>", 0));
			xsEndHostSandboxCode();
		}
		else {
			bailIfError(KprXMPPSend(self, "<body><text><![CDATA[", 0));
			bailIfError(KprXMPPSendXMLCDATA(self, message->response.body, message->response.size));
			bailIfError(KprXMPPSend(self, "]]></text></body>", 0));
		}
	}
	if (message->response.headers) {
		iterate = FskAssociativeArrayIteratorNew(message->response.headers);
		bailIfError(KprXMPPSend(self, "<headers>", 0));
		while (iterate) {
			bailIfError(KprXMPPSend(self, "<header name='", 0));
			bailIfError(KprXMPPSendXMLEntityAttribute(self, iterate->name, 0));
			bailIfError(KprXMPPSend(self, "' value='", 0));
			bailIfError(KprXMPPSendXMLEntityAttribute(self, iterate->value, 0));
			bailIfError(KprXMPPSend(self, "'/>", 0));
			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		bailIfError(KprXMPPSend(self, "</headers>", 0));
		FskAssociativeArrayIteratorDispose(iterate);
		iterate = NULL;
	}
	bailIfError(KprXMPPSend(self, "</message></iq>", 0));
bail:
	if (iterate)
		FskAssociativeArrayIteratorDispose(iterate);
	return;
}

static void KprXMPPMessageMainCallback(KprMessage message, void* it)
{
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprXMPPMessageCallback, message, it, NULL, NULL);
}

static void KprXMPPMessageMainDispose(void* it)
{
}

static FskErr KprXMPPReceiveStanza(KprXMPP self, KprXMLElement element)
{
	FskErr err = kFskErrNone;
	// handle ping iq
	if (KprXMLElementGetFirstElement(element, "urn:xmpp:ping", "ping")) {
		if (!FskStrCompare("get", KprXMLElementGetAttribute(element, "type"))) {
			bailIfError(KprXMPPSend(self, "<iq from='", 0));
			bailIfError(KprXMPPSend(self, KprXMLElementGetAttribute(element, "to"), 0));
			bailIfError(KprXMPPSend(self, "' to='", 0));
			bailIfError(KprXMPPSend(self, KprXMLElementGetAttribute(element, "from"), 0));
			bailIfError(KprXMPPSend(self, "' id='", 0));
			bailIfError(KprXMPPSend(self, KprXMLElementGetAttribute(element, "id"), 0));
			bailIfError(KprXMPPSend(self, "' type='result'/>", 0));
		}
		return err;
	}
	if (!FskStrCompare(element->name, "iq")) {
		char* type = KprXMLElementGetAttribute(element, "type");
		if (!FskStrCompare("result", type)) {
			KprXMLElement messageElement = KprXMLElementGetFirstElement(element, kprXMPPNamespace, "message");
			if (messageElement) {
				UInt32 id = FskStrToNum(KprXMLElementGetAttribute(element, "id"));
				KprXMPPWaiter waiter = NULL;
				while ((waiter = FskListGetNext(self->waiters, waiter))) {
					if (waiter->id == id) {
						KprMessage message = waiter->message;
						FskListRemove(&self->waiters, waiter);
						KprXMPPWaiterDispose(waiter);
						if (messageElement->element) {
							KprXMLElement bodyElement = KprXMLElementGetFirstElement(messageElement, kprXMPPNamespace, "body");
							KprXMLElement headersElement = KprXMLElementGetFirstElement(messageElement, kprXMPPNamespace, "headers");
							if (bodyElement && bodyElement->element) {
								KprXMLElement chunk = KprXMLElementGetFirstElement(bodyElement, kprXMPPNamespace, "chunk");
								KprXMLElement text = KprXMLElementGetFirstElement(bodyElement, kprXMPPNamespace, "text");
								if (chunk && chunk->element) {
									void* data;
									xsIntegerValue size;
									xsBeginHostSandboxCode(self->the, self->code);
									xsVars(1);
									xsVar(0) = xsNew1(xsGlobal, xsID("Chunk"), xsStringBuffer(chunk->element->value, chunk->element->valueSize));
									data = xsGetHostData(xsVar(0));
									size = xsToInteger(xsGet(xsVar(0), xsID("length")));
									KprMessageSetResponseBody(message, data, size);
									xsEndHostSandboxCode();
								}
								else if (text && text->element) {
									UInt32 size = KprXMPPUnescapeCDATABufferMunger(text->element->value, text->element->valueSize);
									KprMessageSetResponseBody(message, text->element->value, size);
								}
							}
							if (headersElement && headersElement->element) {
								KprXMLElement header = KprXMLElementGetFirstElement(headersElement, kprXMPPNamespace, "header");
								for (; header; header = KprXMLElementGetNextElement(header, kprXMPPNamespace, "header")) {
									KprMessageSetResponseHeader(message, KprXMLElementGetAttribute(header, "name"), KprXMLElementGetAttribute(header, "value"));
								}
							}
						}
						KprMessageTransform(message, gXMPPService.machine);
						FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
						return err;
					}
				}
			}
		}
		else if (!FskStrCompare("get", type) || !FskStrCompare("set", type)) {
			KprXMLElement messageElement = KprXMLElementGetFirstElement(element, kprXMPPNamespace, "message");
			if (messageElement) {
				KprMessage message = NULL;
				char *url = KprXMLElementGetAttribute(messageElement, "url");
				if (url) {
					bailIfError(KprMessageNew(&message, url));
					if (!FskStrCompare("set", type))
						KprMessageSetMethod(message, "PUT");
					KprMessageSetRequestHeader(message, "from", KprXMLElementGetAttribute(element, "from"));
					KprMessageSetRequestHeader(message, "id", KprXMLElementGetAttribute(element, "id"));
					KprMessageSetRequestHeader(message, "to", KprXMLElementGetAttribute(element, "to"));
					if (messageElement->element) {
						KprXMLElement bodyElement = KprXMLElementGetFirstElement(messageElement, kprXMPPNamespace, "body");
						KprXMLElement headersElement = KprXMLElementGetFirstElement(messageElement, kprXMPPNamespace, "headers");
						if (bodyElement && bodyElement->element) {
							KprXMLElement chunk = KprXMLElementGetFirstElement(bodyElement, kprXMPPNamespace, "chunk");
							KprXMLElement text = KprXMLElementGetFirstElement(bodyElement, kprXMPPNamespace, "text");
							if (chunk && chunk->element) {
								void* data;
								xsIntegerValue size;
								xsBeginHostSandboxCode(self->the, self->code);
								xsVars(1);
								xsVar(0) = xsNew1(xsGlobal, xsID("Chunk"), xsStringBuffer(chunk->element->value, chunk->element->valueSize));
								data = xsGetHostData(xsVar(0));
								size = xsToInteger(xsGet(xsVar(0), xsID("length")));
								KprMessageSetRequestBody(message, data, size);
								xsEndHostSandboxCode();
							}
							else if (text && text->element) {
								UInt32 size = KprXMPPUnescapeCDATABufferMunger(text->element->value, text->element->valueSize);
								KprMessageSetRequestBody(message, text->element->value, size);
							}
						}
						if (headersElement && headersElement->element) {
							KprXMLElement header = KprXMLElementGetFirstElement(headersElement, kprXMPPNamespace, "header");
							for (; header; header = KprXMLElementGetNextElement(header, kprXMPPNamespace, "header")) {
								KprMessageSetRequestHeader(message, KprXMLElementGetAttribute(header, "name"), KprXMLElementGetAttribute(header, "value"));
							}
						}
					}
					KprMessageInvoke(message, KprXMPPMessageMainCallback, KprXMPPMessageMainDispose, self);
					return err;
				}
			}
		}
	}
	{
		void* stanza;
		xsBeginHost(gXMPPService.machine);
		xsVars(COUNT);
		xsVar(ELEMENT_PROTOTYPE) = xsGet(xsGet(xsGlobal, xsID("DOM")), xsID("element"));	
		xsVar(ATTRIBUTE_PROTOTYPE) = xsGet(xsGet(xsGlobal, xsID("DOM")), xsID("attribute"));	
		KprXMPPReceiveStanzaAux(the, element, xsUndefined, 0);
		stanza = xsMarshall(xsResult);
		xsEndHost(gXMPPService.machine);
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPReceiveStanzaCallback, self, stanza, NULL, NULL);
	}
bail:
	return err;
}

static void KprXMPPReceiveStanzaAux(xsMachine* the, KprXMLElement element, xsSlot parent, xsIntegerValue index)
{
	SInt32 i;
	xsResult = xsNewInstanceOf(xsVar(ELEMENT_PROTOTYPE));
	xsSet(xsResult, xsID("parent"), parent);
	if (element->name)
		xsSet(xsResult, xsID("name"), xsString(element->name));
	if (element->value)
		xsSet(xsResult, xsID("value"), xsString(element->value));
	if (element->nameSpace) {
		if (element->nameSpace->name)
			xsSet(xsResult, xsID("prefix"), xsString(element->nameSpace->name));
		xsSet(xsResult, xsID("namespace"), xsString(element->nameSpace->value));
	}
	if (element->attribute) {
		KprXMLAttribute attribute;
		SInt32 attributeCount = 0;
		SInt32 xmlnsAttributeCount = 0;
		for (attribute = element->attribute; attribute; attribute = attribute->next) {
			if (attribute->isNamespace)
				xmlnsAttributeCount++;
			else
				attributeCount++;
		}
		if (xmlnsAttributeCount) {
			xsVar(ATTRIBUTES) = xsNew1(xsGlobal, xsID("Array"), xsInteger(xmlnsAttributeCount));
			for (attribute = element->attribute, i = 0; attribute; attribute = attribute->next) {
				if (attribute->isNamespace) {
					xsVar(ATTRIBUTE) = xsNewInstanceOf(xsVar(ATTRIBUTE_PROTOTYPE));
					xsSet(xsVar(ATTRIBUTE), xsID("parent"), xsResult);
					if (attribute->name) {
						xsSet(xsVar(ATTRIBUTE), xsID("name"), xsString(attribute->name));
						xsSet(xsVar(ATTRIBUTE), xsID("prefix"), xsString("xmlns"));
					}
					else
						xsSet(xsVar(ATTRIBUTE), xsID("name"), xsString("xmlns"));
					xsSet(xsVar(ATTRIBUTE), xsID("namespace"), xsString("http://www.w3.org/XML/1998/namespace"));
					xsSet(xsVar(ATTRIBUTE), xsID("value"), xsString(attribute->value));
					xsSetAt(xsVar(ATTRIBUTES), xsInteger(i), xsVar(ATTRIBUTE));
					i++;
				}
			}
			xsSet(xsResult, xsID("xmlnsAttributes"), xsVar(ATTRIBUTES));
		}
		if (attributeCount) {
			xsVar(ATTRIBUTES) = xsNew1(xsGlobal, xsID("Array"), xsInteger(attributeCount));
			for (attribute = element->attribute, i = 0; attribute; attribute = attribute->next) {
				if (!attribute->isNamespace) {
					xsVar(ATTRIBUTE) = xsNewInstanceOf(xsVar(ATTRIBUTE_PROTOTYPE));
					xsSet(xsVar(ATTRIBUTE), xsID("parent"), xsResult);
					xsSet(xsVar(ATTRIBUTE), xsID("name"), xsString(attribute->name));
					if (attribute->nameSpace) {
						if (attribute->nameSpace->name)
							xsSet(xsVar(ATTRIBUTE), xsID("prefix"), xsString(attribute->nameSpace->name));
						xsSet(xsVar(ATTRIBUTE), xsID("namespace"), xsString(element->nameSpace->value));
					}
					xsSet(xsVar(ATTRIBUTE), xsID("value"), xsString(attribute->value));
					xsSetAt(xsVar(ATTRIBUTES), xsInteger(i), xsVar(ATTRIBUTE));
					i++;
				}
			}
			xsSet(xsResult, xsID("_attributes"), xsVar(ATTRIBUTES));
		}
	}
	if (xsTest(parent))
		xsSetAt(xsGet(parent, xsID("children")), xsInteger(index), xsResult);
	if (element->element) {
		KprXMLElement child;
		SInt32 elementCount = 0;
		for (child = element->element; child; child = child->next)
			elementCount++;
		if (elementCount) {
			xsSlot slot = xsResult;
			xsRemember(slot);
			xsSet(xsResult, xsID("children"), xsNew1(xsGlobal, xsID("Array"), xsInteger(elementCount)));
			for (child = element->element, i = 0; child; child = child->next) {
				KprXMPPReceiveStanzaAux(the, child, slot, i);
				i++;
			}
 			xsForget(slot);
           	xsResult = slot;
		}
	}
}

static void KprXMPPReceiveStanzaCallback(KprXMPP self, void* stanza)
{
	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(2);
	xsVar(0) = xsGet(self->slot, xsID_behavior);
	if (xsFindResult(xsVar(0), xsID("onXMPPReceived"))) {
		xsVar(1) = xsDemarshall(stanza);
		(void)xsCallFunction2(xsResult, xsVar(0), self->slot, xsVar(1));
	}
	xsEndHostSandboxCode();
	FskMemPtrDispose(stanza);
}

static void KprXMPPPasswordChanged(KprXMPP self)
{
	KprXMPPDisconnect(self);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPApplicationCallback, self, "onXMPPPasswordChanged", NULL, NULL);
	return;
}

static void KprXMPPPasswordChangeError(KprXMPP self, FskErr err)
{
	KprXMPPDisconnect(self);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPApplicationCallback, self, "onXMPPPasswordChangeError", (void*)err, NULL);
	return;
}

static void KprXMPPRegistered(KprXMPP self)
{
	KprXMPPDisconnect(self);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPApplicationCallback, self, "onXMPPRegistered", NULL, NULL);
	return;
}

static void KprXMPPRegisterError(KprXMPP self, FskErr err)
{
	KprXMPPDisconnect(self);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPApplicationCallback, self, "onXMPPRegisterError", (void*)err, NULL);
	return;
}

static void KprXMPPRegistrationCanceled(KprXMPP self)
{
	KprXMPPDisconnect(self);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPApplicationCallback, self, "onXMPPRegistrationCanceled", NULL, NULL);
	return;
}

static void KprXMPPRegistrationCancelError(KprXMPP self, FskErr err)
{
	KprXMPPDisconnect(self);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprXMPPApplicationCallback, self, "onXMPPRegistrationCancelError", (void*)err, NULL);
	return;
}

static FskErr KprXMPPSend(KprXMPP self, char* string, UInt32 size)
{
	FskErr err = kFskErrNone;
	int sent = 0;
	if (!size)
		size = FskStrLen(string);
    while (size > 0) {
        bailIfError(FskNetSocketSendTCP(self->socket, string, size < 1024 ? size : 1024, &sent));
		string += sent;
		size -= sent;
    }
	FskInstrumentedItemPrintfDebug(self, "SEND: %.*s", size, string);
bail:
	return err;
}

static FskErr FskXMPPSendBind(KprXMPP self)
{
	FskErr err = kFskErrNone;
	
	bailIfError(KprXMPPSend(self, "<iq id='bind-0' type='set'><bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'><resource>", 0));
	if (self->resource)
		bailIfError(KprXMPPSendXMLEntityData(self, self->resource, 0));
	bailIfError(KprXMPPSend(self, "</resource></bind></iq>", 0));
bail:
	return err;
}

static FskErr FskXMPPSendRegister(KprXMPP self, char* password)
{
	FskErr err = kFskErrNone;
	
	bailIfError(KprXMPPSend(self, "<iq type='set' to='", 0));
	bailIfError(KprXMPPSendXMLEntityAttribute(self, self->domain, 0));
	bailIfError(KprXMPPSend(self, "' id='", 0));
	bailIfError(KprXMPPSend(self, password ? "change-password-0" : "register-0", 0));
	bailIfError(KprXMPPSend(self, "'><query xmlns='jabber:iq:register'><username><![CDATA[", 0));
	bailIfError(KprXMPPSendXMLCDATA(self, self->username, FskStrLen(self->username)));
	bailIfError(KprXMPPSend(self, "]]></username><password><![CDATA[", 0));
	bailIfError(KprXMPPSendXMLCDATA(self, password ? password : self->password, FskStrLen(password ? password : self->password)));
	bailIfError(KprXMPPSend(self, "]]></password></query></iq>", 0));
bail:
	return err;
}

static FskErr FskXMPPSendRegisterRemove(KprXMPP self)
{
	FskErr err = kFskErrNone;
	
	bailIfError(KprXMPPSend(self, "<iq type='set' to='", 0));
	bailIfError(KprXMPPSendXMLEntityAttribute(self, self->domain, 0));
	bailIfError(KprXMPPSend(self, "' id='register-remove-0'><query xmlns='jabber:iq:register'><remove/></query></iq>", 0));
bail:
	return err;
}

static FskErr FskXMPPSendSession(KprXMPP self)
{
	FskErr err = kFskErrNone;	
	bailIfError(KprXMPPSend(self, "<iq id='session-0' type='set'><session xmlns='urn:ietf:params:xml:ns:xmpp-session'/></iq>", 0));
bail:
	return err;
}

static void KprXMPPSendStanza(KprXMPP self, void* stanza)
{
	xsBeginHost(gXMPPService.machine);
	xsVars(COUNT);
	xsVar(ELEMENT_PROTOTYPE) = xsGet(xsGet(xsGlobal, xsID("DOM")), xsID("element"));	
	xsVar(ATTRIBUTE_PROTOTYPE) = xsGet(xsGet(xsGlobal, xsID("DOM")), xsID("attribute"));
	xsResult = xsDemarshall(stanza);
	KprXMPPSendStanzaAux(the, self);
	xsEndHost(gXMPPService.machine);
	FskMemPtrDispose(stanza);
}

static void KprXMPPSendStanzaAux(xsMachine* the, KprXMPP self)
{
	char* name;
	char* prefix = NULL;
	
	xsVar(VALUE) = xsGet(xsResult, xsID("value"));
	if (xsTest(xsVar(VALUE))) {
		KprXMPPSend(self, xsToString(xsVar(VALUE)), 0);
	}
	else {
		name = xsToString(xsGet(xsResult, xsID("name")));
		xsVar(PREFIX) = xsGet(xsResult, xsID("prefix"));
		if (xsTest(xsVar(PREFIX))) 
			prefix = xsToString(xsVar(PREFIX));
		KprXMPPSend(self, "<", 1);
		if (prefix) {
			KprXMPPSend(self, prefix, 0);
			KprXMPPSend(self, ":", 1);
		}
		KprXMPPSend(self, name, 0);
		xsVar(ATTRIBUTES) = xsGet(xsResult, xsID("_attributes"));
		if (xsTest(xsVar(ATTRIBUTES))) {
			xsIntegerValue c = xsToInteger(xsGet(xsVar(ATTRIBUTES), xsID("length"))), i;
			for (i = 0; i < c; i++) {
				KprXMPPSend(self, " ", 1);
				xsVar(ATTRIBUTE) = xsGetAt(xsVar(ATTRIBUTES), xsInteger(i));
				xsVar(PREFIX) = xsGet(xsVar(ATTRIBUTE), xsID("prefix"));
				if (xsTest(xsVar(PREFIX))) {
					KprXMPPSend(self, xsToString(xsVar(PREFIX)), 0);
					KprXMPPSend(self, ":", 1);
				}
				KprXMPPSend(self, xsToString(xsGet(xsVar(ATTRIBUTE), xsID("name"))), 0);
				KprXMPPSend(self, "='", 2);
				KprXMPPSend(self, xsToString(xsGet(xsVar(ATTRIBUTE), xsID("value"))), 0);
				KprXMPPSend(self, "'", 1);
			}
		}
		xsVar(ATTRIBUTES) = xsGet(xsResult, xsID("xmlnsAttributes"));
		if (xsTest(xsVar(ATTRIBUTES))) {
			xsIntegerValue c = xsToInteger(xsGet(xsVar(ATTRIBUTES), xsID("length"))), i;
			for (i = 0; i < c; i++) {
				KprXMPPSend(self, " ", 1);
				xsVar(ATTRIBUTE) = xsGetAt(xsVar(ATTRIBUTES), xsInteger(i));
				xsVar(PREFIX) = xsGet(xsVar(ATTRIBUTE), xsID("prefix"));
				if (xsTest(xsVar(PREFIX))) {
					KprXMPPSend(self, xsToString(xsVar(PREFIX)), 0);
					KprXMPPSend(self, ":", 1);
				}
				KprXMPPSend(self, xsToString(xsGet(xsVar(ATTRIBUTE), xsID("name"))), 0);
				KprXMPPSend(self, "='", 2);
				KprXMPPSend(self, xsToString(xsGet(xsVar(ATTRIBUTE), xsID("value"))), 0);
				KprXMPPSend(self, "'", 1);
			}
		}
		xsVar(CHILDREN) = xsGet(xsResult, xsID("children"));
		if (xsTest(xsVar(CHILDREN))) {
			xsIntegerValue c = xsToInteger(xsGet(xsVar(CHILDREN), xsID("length"))), i;
			KprXMPPSend(self, ">", 1);
			for (i = 0; i < c; i++) {
				xsResult = xsGetAt(xsVar(CHILDREN), xsInteger(i));
				KprXMPPSendStanzaAux(the, self);
				xsResult = xsGet(xsResult, xsID("parent"));
				xsVar(CHILDREN) = xsGet(xsResult, xsID("children"));
			}	
			KprXMPPSend(self, "</", 2);
			if (prefix) {
				KprXMPPSend(self, prefix, 0);
				KprXMPPSend(self, ":", 1);
			}
			KprXMPPSend(self, name, 0);
			KprXMPPSend(self, ">", 1);
		}
		else {
			KprXMPPSend(self, "/>", 2);
		}
	}
}

static FskErr KprXMPPSendXMLCDATA(KprXMPP self, char* string, UInt32 size)
{
	FskErr err = kFskErrNone;
	char *aStart, *aStop, *theText = string;
	UInt32 aSize, aSpan, theSize = size;
	aStart = (char *)theText;
	aSize = theSize;
	while ((aStop = FskStrNStr(aStart, aSize, "]]")) != NULL) { // partial match
		aStop += 1;
		aSpan = aStop - aStart;
		if ((err = KprXMPPSend(self, aStart, aSpan)))
			break;
		aSize -= aSpan;
		if ((aSize > 1) && (aStop[1] == '&')) { // match = ]] + &
			if ((err = KprXMPPSend(self, "]&amp;", 6)))
				break;
			aSize -= 2;
			aStop += 2;
		}
		else
		if ((aSize > 1) && (aStop[1] == '>')) { // match = ]] + >
			if ((err = KprXMPPSend(self, "]&gt;", 5)))
				break;
			aSize -= 2;
			aStop += 2;
		}
		aStart = aStop;
	}
	if (!err && (aSize > 0)) {
		err = KprXMPPSend(self, aStart, aSize);
		aSize = 0;
	}
	return err;
}

static FskErr KprXMPPSendXMLEntity(KprXMPP self, char* string, UInt32 size, int flag)
{
	FskErr err = kFskErrNone;
	char *aStart, *aStop, *theText = string;
	UInt32 theFlag = flag, theSize = size;
	char aBuffer[7];
	unsigned char aChar;
	static char sEscape[256] = {
	/*  0 1 2 3 4 5 6 7 8 9 A B C D E F */
		0,0,0,0,0,0,0,0,0,2,2,0,0,2,0,0,	/* 0x                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
		1,1,2,1,1,1,0,2,1,1,1,1,1,1,1,1,	/* 2x   !"#$%&'()*+,-./  */
		1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 	/* 7X  pqrstuvwxyz{|}~   */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 8X                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 9X                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* AX                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* BX                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* CX                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* FX                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* EX                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 	/* FX                    */
	};
	aStart = aStop = (char *)theText;
	if (!theSize)
		theSize = (UInt32)FskStrLen(theText);
	while (!err && (theSize > 0) && (aChar = *((unsigned char *)theText))) {
		theText++;
		theSize--;
		switch (sEscape[aChar]) {
		case 1:
			aStop++;
			break;
		case 2:
			if (!theFlag) {
				aStop++;
				break;
			}
			/* continue */
		default:
			if (aStop > aStart)
				err = KprXMPPSend(self, aStart, aStop - aStart);
			switch (aChar) {
			case '"':
				err = KprXMPPSend(self, "&quot;", 6);
				break;
			case '&':
				err = KprXMPPSend(self, "&amp;", 5);
				break;
			case '\'':
				err = KprXMPPSend(self, "&apos;", 6);
				break;
			case '<':
				err = KprXMPPSend(self, "&lt;", 4);
				break;
			case '>':
				err = KprXMPPSend(self, "&gt;", 4);
				break;
			default:
				aStart = aBuffer;
				*(aStart++) = '&';
				*(aStart++) = '#';
				if (aChar >= 10)
					*(aStart++) = '0' + (aChar / 10);
				aChar %= 10;
				*(aStart++) = '0' + aChar;
				*(aStart++) = ';';
				err = KprXMPPSend(self, aBuffer, aStart - aBuffer);
				break;
			}
			aStart = ++aStop;
			break;
		}
	}
	if (!err && (aStop > aStart)) {
		err = KprXMPPSend(self, aStart, aStop - aStart);
	}
	return err;
}

static FskErr KprXMPPSendXMLEntityAttribute(KprXMPP self, char* string, UInt32 size)
{
	return KprXMPPSendXMLEntity(self, string, size, 1); // XS_ATTRIBUTE_LITERAL = 1
}

static FskErr KprXMPPSendXMLEntityData(KprXMPP self, char* string, UInt32 size)
{
	return KprXMPPSendXMLEntity(self, string, size, 0); // XS_DATA_LITERAL = 0
}

static FskErr KprXMPPStreamStart(KprXMPP self)
{
	FskErr err = kFskErrNone;
	
	KprXMPPDisposeStream(self);
	if (!self->socketHandler)
		FskThreadAddDataHandler(&self->socketHandler, (FskThreadDataSource)self->socket, KprXMPPParse, true, false, self);
	if (self->expat)
		XML_ParserReset(self->expat, NULL);
	else
		self->expat = XML_ParserCreate(NULL);
	bailIfNULL(self->expat);

	XML_SetUserData(self->expat, self);
	XML_SetElementHandler(self->expat, KprXMPPParserStartTag, KprXMPPParserStopTag);
	XML_SetCharacterDataHandler(self->expat, KprXMPPParserDefault);

	bailIfError(KprXMPPSend(self, "<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='", 0));
	bailIfError(KprXMPPSendXMLEntityAttribute(self, self->domain, 0));
	bailIfError(KprXMPPSend(self, "' version='1.0'>", 0));
bail:
	if (err && self->socketHandler)
		FskThreadRemoveDataHandler(&self->socketHandler);
	return err;
}

UInt32 KprXMPPUnescapeCDATABufferMunger(char* string, UInt32 size)
{
	char *aPtr, *aStart, *aStop, *theText = string;
	UInt32 aSize, aSpan, theSize = size;
	aPtr = aStart = (char *)theText;
	aSize = theSize;
	while ((aStop = FskStrNStr(aStart, aSize, "]]&")) != NULL) { // partial match
		aStop += 1;
		aSpan = aStop - aStart;
		FskMemMove(aPtr, aStart, aSpan);
		aPtr += aSpan;
		aSize -= aSpan;
	//	if ((aSize > 5) && (FskStrCompareWithLength(aStop + 2, "amp;", 4) == 0)) { // match = ]]& + amp;
		if ((aSize > 5) && (aStop[2] == 'a') && (aStop[3] == 'm') && (aStop[4] == 'p') && (aStop[5] == ';')) { // match = ]]& + amp;
	//
			FskStrNCopy(aPtr, "]&", 2);
			aPtr += 2;
			aSize -= 6;
			aStop += 6;
		}
		else
	//	if ((aSize > 4) && (FskStrCompareWithLength(aStop + 2, "gt;", 3) == 0)) { // match = ]]& + gt
		if ((aSize > 4) && (aStop[2] == 'g') && (aStop[3] == 't') && (aStop[4] == ';')) { // match = ]]& + gt;
	//
			FskStrNCopy(aPtr, "]>", 2);
			aPtr += 2;
			aSize -= 5;
			aStop += 5;
		}
		aStart = aStop;
	}
	if (aStart == theText) {
		return theSize; // no change
	}
	if (aSize > 0) {
		FskMemMove(aPtr, aStart, aSize);
		aPtr += aSize;
		aSize = 0;
	}
	return aPtr - theText;
}

FskErr KprXMPPSetCertificate(KprXMPP self, void* certificate, UInt32 size, char *policies)
{
	FskErr err = kFskErrNone;
	FskMemPtrDisposeAt(&self->certificate);
	FskMemPtrDisposeAt(&self->policies);
	self->certificateSize = 0;
	if (certificate && size) {
		bailIfError(FskMemPtrNew(size, &self->certificate));
		FskMemCopy(self->certificate, certificate, size);
		self->certificateSize = size;
	}
	self->policies = FskStrDoCopy(policies);
bail:
	return err;
}

void KPR_xmpp(void *it)
{
	if (it)
		FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprXMPPDispose, it, NULL, NULL, NULL);
}

void KPR_XMPP(xsMachine* the)
{
	KprXMPP self;
	char* domain = xsToString(xsArg(1));
	char* username = xsToString(xsArg(2));
	char* password = xsToString(xsArg(3));
	xsIntegerValue c = xsToInteger(xsArgc);
	
	if (c > 4)
		KprXMPPNew(&self, domain, username, password, xsToString(xsArg(4)));
	else {
		char* resource = KprMachineName();
		KprXMPPNew(&self, domain, username, password, resource);
		FskMemPtrDispose(resource);
	}
		
	xsVars(COUNT);

	xsSetHostData(xsResult, self);
	xsNewHostProperty(xsResult, xsID_behavior, xsArg(0), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	
	xsVar(ELEMENT_PROTOTYPE) = xsGet(xsGet(xsGlobal, xsID("DOM")), xsID("element"));	
	xsVar(ATTRIBUTE_PROTOTYPE) = xsGet(xsGet(xsGlobal, xsID("DOM")), xsID("attribute"));
	xsVar(DOCUMENT) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("DOM")), xsID("document")));
	xsVar(ELEMENT) = xsNewInstanceOf(xsVar(ELEMENT_PROTOTYPE));
	xsVar(ATTRIBUTES) = xsNew1(xsGlobal, xsID("Array"), xsInteger(2));
	xsVar(ATTRIBUTE) = xsNewInstanceOf(xsVar(ATTRIBUTE_PROTOTYPE));
	xsSet(xsVar(ATTRIBUTE), xsID("name"), xsString("xmlns"));
	xsSet(xsVar(ATTRIBUTE), xsID("value"), xsString("jabber:client"));
	xsSetAt(xsVar(ATTRIBUTES), xsInteger(0), xsVar(ATTRIBUTE));
	xsVar(ATTRIBUTE) = xsNewInstanceOf(xsVar(ATTRIBUTE_PROTOTYPE));
	xsSet(xsVar(ATTRIBUTE), xsID("name"), xsString("stream"));
	xsSet(xsVar(ATTRIBUTE), xsID("prefix"), xsString("xmlns"));
	xsSet(xsVar(ATTRIBUTE), xsID("value"), xsString("http://etherx.jabber.org/streams"));
	xsSetAt(xsVar(ATTRIBUTES), xsInteger(1), xsVar(ATTRIBUTE));
	xsSet(xsVar(ELEMENT), xsID("xmlnsAttributes"), xsVar(ATTRIBUTES));
	xsVar(ATTRIBUTES) = xsNew1(xsGlobal, xsID("Array"), xsInteger(2));
	xsVar(ATTRIBUTE) = xsNewInstanceOf(xsVar(ATTRIBUTE_PROTOTYPE));
	xsSet(xsVar(ATTRIBUTE), xsID("name"), xsString("to"));
	xsSet(xsVar(ATTRIBUTE), xsID("value"), xsString(self->domain));
	xsSetAt(xsVar(ATTRIBUTES), xsInteger(0), xsVar(ATTRIBUTE));
	xsVar(ATTRIBUTE) = xsNewInstanceOf(xsVar(ATTRIBUTE_PROTOTYPE));
	xsSet(xsVar(ATTRIBUTE), xsID("name"), xsString("version"));
	xsSet(xsVar(ATTRIBUTE), xsID("value"), xsString("1.0"));
	xsSetAt(xsVar(ATTRIBUTES), xsInteger(1), xsVar(ATTRIBUTE));
	xsSet(xsVar(ELEMENT), xsID("_attributes"), xsVar(ATTRIBUTES));
	xsSet(xsVar(DOCUMENT), xsID("element"), xsVar(ELEMENT));
	xsNewHostProperty(xsResult, xsID("document"), xsVar(DOCUMENT), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);

	self->the = the;
	self->slot = xsResult;
	self->code = the->code;
	xsRemember(self->slot);
}

void KPR_xmpp_connect(xsMachine* the)
{
	KprXMPP self = xsGetHostData(xsThis);
	char* domain = NULL;
	char* colon = NULL;
	UInt32 port = 5222;
	xsIntegerValue c = xsToInteger(xsArgc);
	if ((c > 0) && xsTest(xsArg(0))) {
		domain = FskStrDoCopy(xsToString(xsArg(0)));
		colon = FskStrChr(domain, ':');
		if (colon) {
			*colon = 0;
			port = FskStrToNum(colon + 1);
		}
	}
	if ((c > 1) && (xsTypeOf(xsArg(1)) == xsBooleanType)) {
		self->registered = xsToBoolean(xsArg(1)) ? true : false;
	}
	else {
		self->registered = true; // default
	}
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprXMPPConnect, self, domain, (void*)port, NULL);
}

void KPR_xmpp_disconnect(xsMachine* the)
{
	KprXMPP self = xsGetHostData(xsThis);
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprXMPPDisconnect, self, NULL, NULL, NULL);
}

void KPR_xmpp_get_bareJID(xsMachine* the)
{
	KprXMPP self = xsGetHostData(xsThis);
	xsResult = xsString(self->jid);
}

void KPR_xmpp_get_fullJID(xsMachine* the)
{
	KprXMPP self = xsGetHostData(xsThis);
	if (self->fjid)
		xsResult = xsString(self->fjid);
	else
		xsResult = xsString(self->jid);
}

void KPR_xmpp_get_nextID(xsMachine* the)
{
	KprXMPP self = xsGetHostData(xsThis);
	char number[16];
	FskStrNumToStr(self->id++, number, 15);
	xsResult = xsString(number);
}

void KPR_xmpp_get_resource(xsMachine* the)
{
	KprXMPP self = xsGetHostData(xsThis);
	xsResult = xsString(self->resource);
}

void KPR_xmpp_set_certificate(xsMachine* the)
{
	KprXMPP self = xsGetHostData(xsThis);
	void *data = NULL;
	xsStringValue policies = NULL;
	xsIntegerValue size = 0;
	if (xsTest(xsArg(0))) {
		data = xsGetHostData(xsArg(0));
		size = xsToInteger(xsGet(xsArg(0), xsID_length));
	}
	if (xsTest(xsArg(1)))
		policies = xsToString(xsArg(1));
	xsThrowIfFskErr(KprXMPPSetCertificate(self, data, size, policies));
}

void KPR_xmpp_send(xsMachine* the)
{
	KprXMPP self = xsGetHostData(xsThis);
	void* stanza = xsMarshall(xsArg(0));
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprXMPPSendStanza, self, stanza, NULL, NULL);
}

void KPR_xmpp_tunnel(xsMachine* the)
{
	KprXMPP self = xsGetHostData(xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	char* colon = NULL;
	self->proxyHost = FskStrDoCopy(xsToString(xsArg(0)));
	colon = FskStrChr(self->proxyHost, ':');
	if (colon) {
		*colon = 0;
		self->proxyPort = FskStrToNum(colon + 1);
	}
	else
		self->proxyPort = 80;
	if (c > 1) {
		self->proxyAuthentication = FskStrDoCopy(xsToString(xsArg(1)));
	}
}

// XEP-0077: In-Band Registration
// http://xmpp.org/extensions/xep-0077.html#usecases-cancel
// http://xmpp.org/extensions/xep-0077.html#usecases-changepw
// http://xmpp.org/extensions/xep-0077.html#usecases-register

void KPR_xmpp_cancelRegistration(xsMachine* the)
{
	KprXMPP self = xsGetHostData(xsThis);
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprXMPPCancelRegistration, self, NULL, NULL, NULL);
}

void KPR_xmpp_changePassword(xsMachine* the)
{
	KprXMPP self = xsGetHostData(xsThis);
	xsStringValue password = NULL;
	if (xsTest(xsArg(0))) {
		password = FskStrDoCopy(xsToString(xsArg(0)));
	}
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprXMPPChangePassword, self, password, NULL, NULL);
}

void KPR_xmpp_register(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsCall2_noResult(xsThis, xsID_connect, (c > 0) ? xsArg(0) : xsUndefined, xsBoolean(false));
}

// XEP-0106: JID Escaping
// http://xmpp.org/extensions/xep-0106.html#escaping
// http://xmpp.org/extensions/xep-0106.html#unescaping

void KPR_xmpp_escapeNode(xsMachine* the)
{
	static unsigned char sHexa[] = "0123456789ABCDEF";
	xsStringValue aString = xsToString(xsArg(0));
	char *aStart, *aStop;
	char aBuffer[4] = "\\##\0";
	unsigned char aChar;
	aStart = aStop = (char *)aString;
	xsResult = xsString("");
	while ((aChar = *((unsigned char *)aString++))) {
		switch (aChar) {
			case ' ': case '"': case '&': case '\'': case '/': case ':': case '<': case '>': case '@': case '\\':
				if (aStop > aStart) {
					xsResult = xsCall1(xsResult, xsID_concat, xsStringBuffer(aStart, aStop - aStart));
				}
				aStop++;
				aBuffer[1] = sHexa[aChar / 16];
				aBuffer[2] = sHexa[aChar % 16];
				xsResult = xsCall1(xsResult, xsID_concat, xsString(aBuffer));
				aStart = aStop;
				break;
			default:
				aStop++;
				break;
		}
	}
	if (aStop > aStart) {
		xsResult = xsCall1(xsResult, xsID_concat, xsStringBuffer(aStart, aStop - aStart));
	}
}

void KPR_xmpp_unescapeNode(xsMachine* the)
{
#define unescapeHexa(X) \
	((('0' <= (X)) && ((X) <= '9')) \
		? ((X) - '0') \
		: ((('a' <= (X)) && ((X) <= 'f')) \
			? (10 + (X) - 'a') \
			: (10 + (X) - 'A')))
	xsStringValue aString = xsToString(xsArg(0));
	char *aStart, *aStop;
	char aBuffer[2] = "#\0";
	unsigned char aChar;
	aStart = aStop = (char *)aString;
	xsResult = xsString("");
	while ((aChar = *((unsigned char *)aString++))) {
		switch (aChar) {
			case '\\':
				if (aStop > aStart) {
					xsResult = xsCall1(xsResult, xsID_concat, xsStringBuffer(aStart, aStop - aStart));
				}
				aStop++;
				aChar = *((unsigned char *)aString++);
				if (aChar) {
					aStop++;
					aBuffer[0] = unescapeHexa(aChar);
					aChar = *((unsigned char *)aString++);
					if (aChar) {
						aStop++;
						aBuffer[0] = (aBuffer[0] << 4) | unescapeHexa(aChar);
						xsResult = xsCall1(xsResult, xsID_concat, xsString(aBuffer));
					}
				}
				aStart = aStop;
				break;
			default:
				aStop++;
				break;
		}
	}
	if (aStop > aStart) {
		xsResult = xsCall1(xsResult, xsID_concat, xsStringBuffer(aStart, aStop - aStart));
	}
}

void KPR_XMPP_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("XMPP"));
	xsNewHostProperty(xsResult, xsID("SocketNotConnectedError"), xsInteger(kFskErrSocketNotConnected), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("SSLHandshakeFailedError"), xsInteger(kFskErrSSLHandshakeFailed), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("AuthFailedError"), xsInteger(kFskErrAuthFailed), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("UnimplementedError"), xsInteger(kFskErrUnimplemented), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
}

void KprXMPPServiceCancel(KprService service UNUSED, KprMessage message UNUSED)
{
}

void KprXMPPServiceInvoke(KprService service UNUSED, KprMessage message)
{
	FskErr err = kFskErrNone;
	if (KprMessageContinue(message)) {
		FskAssociativeArrayIterator iterate = NULL;
		char* from = KprMessageGetRequestHeader(message, "from");
		char* fromPtr = NULL;
		char* to = KprMessageGetRequestHeader(message, "to");
		char* toPtr = NULL;
		char* url = message->url + 7;
		KprXMPP self = NULL;
		char id[16];
		KprXMPPWaiter waiter = NULL;
		if (!from || !to) {
			UInt32 len;
			char* jid = url;
			char* ptr = FskStrStr(jid, "//");
			if (ptr) {
				len = ptr - jid;
				bailIfError(FskMemPtrNewFromData(len + 1, jid, &fromPtr));
				fromPtr[len] = '\0';
				jid = ptr + 2;
			}
			ptr = FskStrChr(jid, '@');
			if (ptr) {
				ptr = FskStrChr(ptr + 1, '/');
				if (ptr) {
					if (!fromPtr) {
						len = ptr - jid;
						bailIfError(FskMemPtrNewFromData(len + 1, jid, &fromPtr));
						fromPtr[len] = '\0';
					}
					ptr = FskStrChr(ptr + 1, '/');
					if (ptr) {
						len = ptr - jid;
						bailIfError(FskMemPtrNewFromData(len + 1, jid, &toPtr));
						toPtr[len] = '\0';
						url = ptr + 1;
					}
				}
			}
			from = from ? from : fromPtr;
			to = to ? to : toPtr;
			bailIfNULL(from);
			bailIfNULL(to);
		}
		// fullJID == from
		while ((self = FskListGetNext(gXMPPList, self))) {
			if (self->fjid && !FskStrCompare(self->fjid, from))
				break;
		}
		if (!self) {
			// bareJID == from && fullJID != to
			while ((self = FskListGetNext(gXMPPList, self))) {
				if (self->jid && !FskStrCompare(self->jid, from) && self->fjid && FskStrCompare(self->fjid, to))
					break;
			}
		}
		if (!self) {
			err = kFskErrNameLookupFailed;
			goto bail;
		}
		self->id++;
		FskStrNumToStr(self->id, id, 15);
		bailIfError(KprXMPPWaiterNew(&waiter, self->id, message));
	//	KprMessageSetRequestHeader(message, "id", id); // XXX
		
		bailIfError(KprXMPPSend(self, "<iq from='", 0));
		if (self->fjid) {
			bailIfError(KprXMPPSendXMLEntityAttribute(self, self->fjid, 0));
		}
		else {
			bailIfError(KprXMPPSendXMLEntityAttribute(self, self->jid, 0));
		}
		bailIfError(KprXMPPSend(self, "' id='", 0));
		bailIfError(KprXMPPSendXMLEntityAttribute(self, id, 0));
		bailIfError(KprXMPPSend(self, "' to='", 0));
		bailIfError(KprXMPPSendXMLEntityAttribute(self, to, 0));
		bailIfError(KprXMPPSend(self, "' type='", 0));
		if (message->method && !FskStrCompare(message->method, "PUT")) {
			bailIfError(KprXMPPSend(self, "set", 0));
        }
		else {
			bailIfError(KprXMPPSend(self, "get", 0));
        }
		bailIfError(KprXMPPSend(self, "'><message xmlns='", 0));
		bailIfError(KprXMPPSend(self, kprXMPPNamespace, 0));
		bailIfError(KprXMPPSend(self, "' url='xkpr://", 0));
		bailIfError(KprXMPPSendXMLEntityAttribute(self, url, 0));
		bailIfError(KprXMPPSend(self, "'>", 0));
		if (message->request.body) {
			if (1) {
				xsStringValue body;
				xsBeginHostSandboxCode(self->the, self->code);
				xsVars(1);
				xsVar(0) = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(message->request.size));
				FskMemCopy(xsGetHostData(xsVar(0)), message->request.body, message->request.size);
				body = xsToString(xsVar(0));
				bailIfError(KprXMPPSend(self, "<body><chunk>", 0));
				bailIfError(KprXMPPSend(self, body, FskStrLen(body)));
				bailIfError(KprXMPPSend(self, "</chunk></body>", 0));
				xsEndHostSandboxCode();
			}
			else {
				bailIfError(KprXMPPSend(self, "<body><text><![CDATA[", 0));
				bailIfError(KprXMPPSendXMLCDATA(self, message->request.body, message->request.size));
				bailIfError(KprXMPPSend(self, "]]></text></body>", 0));
			}
		}
		if (message->request.headers) {
			iterate = FskAssociativeArrayIteratorNew(message->request.headers);
			bailIfError(KprXMPPSend(self, "<headers>", 0));
			while (iterate) {
				if (FskStrCompareWithLength(iterate->name, "from", 4) && FskStrCompareWithLength(iterate->name, "to", 2)) {
					bailIfError(KprXMPPSend(self, "<header name='", 0));
					bailIfError(KprXMPPSendXMLEntityAttribute(self, iterate->name, 0));
					bailIfError(KprXMPPSend(self, "' value='", 0));
					bailIfError(KprXMPPSendXMLEntityAttribute(self, iterate->value, 0));
					bailIfError(KprXMPPSend(self, "'/>", 0));
				}
				iterate = FskAssociativeArrayIteratorNext(iterate);
			}
			bailIfError(KprXMPPSend(self, "</headers>", 0));
			FskAssociativeArrayIteratorDispose(iterate);
			iterate = NULL;
		}
		bailIfError(KprXMPPSend(self, "</message></iq>", 0));
		
		FskListAppend(&self->waiters, waiter);
	bail:
		if (iterate)
			FskAssociativeArrayIteratorDispose(iterate);
		if (fromPtr)
			FskMemPtrDispose(fromPtr);
		if (toPtr)
			FskMemPtrDispose(toPtr);
		if (err) {
			if (waiter)
				KprXMPPWaiterDispose(waiter);
			message->error = err;
			FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
		}
	}
}

FskErr KprXMPPWaiterNew(KprXMPPWaiter* it, UInt32 id, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprXMPPWaiter self = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprXMPPWaiterRecord), it));
	self = *it;
	self->id = id;
	self->message = message;
bail:
    return err;
}

void KprXMPPWaiterDispose(KprXMPPWaiter self)
{
	FskMemPtrDispose(self);
}


