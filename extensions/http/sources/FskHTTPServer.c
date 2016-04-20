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
/*
	** ToDo:
	Interface change notification (done)
	Flush on shutdown
*/

#define __FSKTHREAD_PRIV__
#define __FSKHTTPSERVER_PRIV__

#include "FskHTTPServer.h"
#include "FskSSL.h"

#define kHTTPServerIdentifier "Kinoma HTTP Server/0.5"

#define doCallCondition(cb, item, cond, ref) \
	((cb) ? (*cb)(item, cond, ref) : kFskErrUnimplemented)
#define doDataCallback(cb, item, data, size, ret, ref) \
	((cb) ? (*cb)(item, data, size, ret, ref) : kFskErrUnimplemented)


#define shoveBuffer(x) if (x.max && x.pos && (x.max >= x.pos)) { \
	x.max = x.max - x.pos; \
	FskMemMove(x.buf, &x.buf[x.pos], x.max); \
	x.pos = 0; \
}

#define kFskHTTPKeepAliveTimeout	30

static void httpServerEngineCycle(void *param);
static void httpServerTimeCycle(FskTimeCallBack cb, const FskTime when, void *param);


#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageHTTPServer(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

static FskInstrumentedTypeRecord gFskHTTPServerTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"httpserver",
	FskInstrumentationOffset(FskHTTPServerRecord),
	NULL,
	0,
	NULL,
	doFormatMessageHTTPServer
};

static FskInstrumentedTypeRecord gFskHTTPServerRequestTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"httpserverrequest",
	FskInstrumentationOffset(FskHTTPServerRequestRecord),
	NULL,
	0,
	NULL,
	doFormatMessageHTTPServer
};

#endif
static FskErr sFskHTTPServerDispose(FskHTTPServer http);
static void sFskHTTPServerRequestDispose(FskHTTPServerRequest request);

void sFskHTTPServerRequestUpUse(FskHTTPServerRequest request) {
	request->useCount++;
}

void sFskHTTPServerRequestDownUse(FskHTTPServerRequest request) {
	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "%p ServerRequestDownUse to %d\n", request, request ? request->useCount : 0);
	if (NULL != request) {
		request->useCount--;
		if (request->useCount == 0) {
			sFskHTTPServerRequestDispose(request);
		}
	}
}

void sFskHTTPServerUpUse(FskHTTPServer http) {
	http->useCount++;
	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "%p ServerUpUse to %d\n", http, http->useCount);
}

void sFskHTTPServerDownUse(FskHTTPServer http) {
	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "%p ServerDownUse to %d\n", http, http ? http->useCount : 0);
	if (NULL != http) {
		http->useCount--;
		if (http->useCount == 0) {
			sFskHTTPServerDispose(http);
		}
	}
}


static void KprHTTPServerTimerCallback(FskTimeCallBack callback, const FskTime time, void *it)
{
	FskHTTPServerRequest self = it;
	FskTimeCallbackScheduleFuture(self->timer, 0, 200, KprHTTPServerTimerCallback, self);
	FskHTTPServerRequestCycle(self);
}

static FskErr httpServerListenerStart(FskHTTPServerListener listener, FskSocket skt) {
	FskErr err = kFskErrNone;
	FskHTTPServerRequest request;

	if (listener->http->stopped) {
		FskInstrumentedItemSendMessage(listener->http, kFskHTTPInstrMsgConnectionRefusedStopped, listener);
		listener->http->stats.connectionsRefused++;
		FskNetSocketClose(skt);
		goto bail;
	}
		
	err = FskMemPtrNewClear(sizeof(FskHTTPServerRequestRecord), (FskMemPtr*)&request);
	BAIL_IF_ERR(err);

	sFskHTTPServerRequestUpUse(request);

	request->http = listener->http;
	request->skt = skt;	
	FskNetSocketGetRemoteAddress(skt, (UInt32 *)&request->requesterAddress, &request->requesterPort);
	FskNetSocketMakeNonblocking(request->skt);
	err = FskHeaderStructNew(&request->requestHeaders);
	BAIL_IF_ERR(err);
	err = FskHeaderStructNew(&request->responseHeaders);
	BAIL_IF_ERR(err);
	request->in.bufferSize = request->http->defaultBufferSize;
	request->out.bufferSize = request->http->defaultBufferSize;
	err = FskMemPtrNew(request->in.bufferSize, (FskMemPtr*)&request->in.buf);
	BAIL_IF_ERR(err);
	err = FskMemPtrNew(request->out.bufferSize, (FskMemPtr*)&request->out.buf);
	BAIL_IF_ERR(err);

	FskListAppend((FskList*)&request->http->activeRequests, request);
	FskTimeCallbackNew(&request->cycleCallback);
	FskTimeCallbackNew(&request->keepAliveKillCallback);

	listener->http->stats.connectionsMade++;

	request->state = kHTTPNewSession;

	FskInstrumentedItemNew(request, NULL, &gFskHTTPServerRequestTypeInstrumentation);
	FskInstrumentedItemSetOwner(request, request->http);
	
	FskTimeCallbackScheduleNextRun(request->cycleCallback, httpServerTimeCycle, request);
	doCallCondition(request->http->callbacks->requestCondition, request, kFskHTTPConditionConnectionInitialized, request->refCon);
	FskTimeCallbackNew(&request->timer);
	FskTimeCallbackScheduleFuture(request->timer, 1, 0, KprHTTPServerTimerCallback, request);
bail:
	if (err)
		FskHTTPServerRequestDispose(request);
	return err;
}

static FskErr sHTTPServerGotSocket(struct FskSocketRecord *skt, void *refCon) {
	FskHTTPServerListener listener = (FskHTTPServerListener)refCon;
	listener->handshaking = false;
	if (listener->http && FskNetSocketGetLastError(skt) == kFskErrNone)
		return httpServerListenerStart(listener, skt);
	else {
		FskHTTPServerListenerDispose(listener);
		return kFskErrOperationFailed;
	}
}

static FskErr httpServerListenerAcceptNewConnection(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon) {
	FskErr err = kFskErrNone;
	FskHTTPServerListener listener = (FskHTTPServerListener)handler->refCon;
	FskSocket skt;

	err = FskNetAcceptConnection((FskSocket)source, &skt, "HTTP Acceptor");
	if (err != kFskErrNone) return err;
	FskNetSocketMakeNonblocking(skt);
#if CLOSED_SSL
	if (listener->http->ssl) {
		void *ssl;
		err = FskSSLAttach(&ssl, skt);
		if (err != kFskErrNone) {
			FskNetSocketClose(skt);
			return err;
		}
		if (listener->http->certs != NULL)
			FskSSLLoadCerts(ssl, listener->http->certs);
		listener->handshaking = true;
		err = FskSSLHandshake(ssl, sHTTPServerGotSocket, listener, false);
		if (err != kFskErrNone) {
			FskSSLDispose(ssl);
			FskNetSocketClose(skt);
			return err;
		}
		return err;
	}
	else
#endif
		return httpServerListenerStart(listener, skt);
}


FskHTTPServerListener FskHTTPServerListenerNew(FskHTTPServer http, int port, char *interfaceName) {
	FskHTTPServerListener	listener;
	FskErr			err;
	FskSocket		skt;
	FskNetInterfaceRecord *ifc = NULL;

	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerListenerNew - interfaceName: %s\n", interfaceName);
	err = FskMemPtrNewClear(sizeof(FskHTTPServerListenerRecord), (FskMemPtr*)&listener);
	BAIL_IF_ERR(err);
	listener->http = http;
	listener->port = port;
	listener->ifcName = FskStrDoCopy(interfaceName);
	err = FskNetSocketNewTCP(&skt, true, "HTTP Server");
	if (err) {
		FskInstrumentedItemSendMessage(listener->http, kFskHTTPInstrMsgErrString, "listener socket create failed.");
		FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerListenerNew -  creating socket failed: %d\n", err);
        BAIL(kFskErrNoMoreSockets);
	}
	FskNetSocketReuseAddress(skt);
	ifc = FskNetInterfaceFindByName(listener->ifcName);
	if ((NULL == ifc) ||
		(kFskErrNone != (err = FskNetSocketBind(skt, ifc->ip, listener->port)))) {
		FskNetSocketClose(skt);
		FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerListenerNew - bind failed: %d port: %d\n", err, listener->port);
		listener->http->stats.connectionsAborted++;
		if (listener->http->callbacks)
			err = doCallCondition(listener->http->callbacks->serverCondition, listener->http, kFskHTTPConditionNoSocket, listener->http->refCon);
		goto bail;
	}

	listener->skt = skt;	
	FskNetSocketMakeNonblocking(skt);
	FskListAppend((FskList*)&listener->http->listeners, listener);
	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerListenerNew -  about to listen\n");

	FskNetSocketListen(skt);
	FskThreadAddDataHandler(&listener->dataHandler, (FskThreadDataSource)skt, (FskThreadDataReadyCallback)httpServerListenerAcceptNewConnection, true, false, listener);

	FskInstrumentedItemSendMessage(listener->http, kFskHTTPInstrMsgNowListening, listener);
bail:
	FskNetInterfaceDescriptionDispose(ifc);
	if (err) {
		FskInstrumentedItemSendMessage(listener->http, kFskHTTPInstrMsgFailedListener, listener);
		FskMemPtrDisposeAt((void**)&listener);
	}
	return listener;
}

FskErr FskHTTPServerListenerDispose(FskHTTPServerListener listener) {
	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerListenerDispose - listener: %p\n", listener);
	if (listener) {
		if (listener->http && listener->http->listeners)
			FskListRemove((FskList*)&listener->http->listeners, listener);
		if (listener->handshaking) {
			FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerListenerDispose - listener: %p - wait for handshaking\n", listener);
			listener->http = NULL;
		}
		else {
			FskThreadRemoveDataHandler(&listener->dataHandler);
			FskNetSocketClose(listener->skt);
			FskMemPtrDispose(listener->ifcName);
			FskMemPtrDispose(listener);
		}
	}
	return kFskErrNone;
}

FskErr FskHTTPServerListenerAdd(FskHTTPServer http, int port, char *ifcName, FskHTTPServerListener *newListener) {
	FskHTTPServerListener listener;
	FskErr err = kFskErrNone;

	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerListenerAdd - ifcName: %s\n", ifcName);
	listener = FskHTTPServerListenerNew(http, port, ifcName);
	if (!listener) {
		FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerListenerAdd - no listener created\n");
		err = kFskErrOperationFailed;
	}

	if (newListener)
		*newListener = listener;

		FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerListenerAdd - %s:%d returns %d\n", ifcName, port, err);
	return err;
}

void httpServerInterfaceDown(FskHTTPServer http, char *ifcName) {
	FskHTTPServerListener cur = http->listeners, next = NULL;

	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceDown - ifcName: %p\n", ifcName);
	if (ifcName == NULL) return;

	while (cur) {
		next = cur->next;
		if (0 == FskStrCompare(cur->ifcName, ifcName)) {
			FskHTTPServerListenerDispose(cur);
		}
		cur = next;
	}
}

int httpServerInterfaceUp(FskHTTPServer http, char *ifcName) {
	return FskHTTPServerListenerAdd(http, http->port, ifcName, NULL);
}

int httpServerInterfaceChanged(FskNetInterfaceRecord *ifc, UInt32 status, void *param)
{
	FskHTTPServer http = (FskHTTPServer)param;
	FskNetInterfaceRecord *newIface = NULL;
	FskErr err;

	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged - status: %u\n", (unsigned int)status);
	if (status == kFskNetInterfaceStatusChanged) {
		newIface = FskNetInterfaceFindByName(ifc->name);
		FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged - newIface is: %p\n", newIface);
	}

	switch (status) {
		case kFskNetInterfaceStatusChanged:
			err = doCallCondition(http->callbacks->serverCondition, http, kFskHTTPConditionInterfaceChanged, ifc->name);
			if (kFskErrUnimplemented == err || kFskErrNone == err) {
				FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged - about to call httpServerInterfaceDown - %p: %p\n", http, ifc);
				httpServerInterfaceDown(http, ifc->name);
				if (newIface->status == 1) {
					FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged - about to call httpServerInterfaceUp - %p: %p\n", http, newIface);
					httpServerInterfaceUp(http, newIface->name);
				}
				FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged - done\n");
			}
			break;
    	case kFskNetInterfaceStatusRemoved:
			FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged - InterfaceRemoved %p: %p\n", http, ifc);
			err = doCallCondition(http->callbacks->serverCondition, http, kFskHTTPConditionInterfaceRemoved, ifc->name);
			if (kFskErrUnimplemented == err || kFskErrNone == err) {
				FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged - about to call httpServerInterfaceDown - %p: %p\n", http, ifc);
				httpServerInterfaceDown(http, ifc->name);
				FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged -  done\n");
			}
			break;
		case kFskNetInterfaceStatusNew:
			FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged - InterfaceNew %p: %p\n", http, ifc);
			err = doCallCondition(http->callbacks->serverCondition, http, kFskHTTPConditionInterfaceAdded, ifc->name);
			if (ifc->status == 0) {
				FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, " -- new interface is down\n");
			}
			else
			if (kFskErrUnimplemented == err || kFskErrNone == err) {
				if (http->all) {
					FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged - about to call httpServerInterfaceUp - %p: %p\n", http, ifc);
					httpServerInterfaceUp(http, ifc->name);
				}
			}
			break;
	}

	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged - about to call DescriptionDispose\n");
	FskNetInterfaceDescriptionDispose(newIface);
	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerInterfaceChanged - Done.\n");
	return kFskErrNone;
}

/*
 	interface is optional. If NULL, then all interfaces will be used
*/
FskErr FskHTTPServerCreate(int port, char *interfaceName, FskHTTPServer *server, void *refCon, Boolean ssl) {
	FskHTTPServer	http;
	FskErr err;

	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpServerCreate\n");
	err = FskMemPtrNewClear(sizeof(FskHTTPServerRecord), (FskMemPtr*)&http);
	BAIL_IF_ERR(err);

	sFskHTTPServerUpUse(http);

	http->stopped = true;
	http->refCon = refCon;
	http->port = port;
	http->keepAliveTimeout = kFskHTTPKeepAliveTimeout;
	http->defaultBufferSize = kFskHTTPServerDefaultBufferSize;
	http->owner = FskThreadGetCurrent();
	http->ssl = ssl;

	snprintf(http->name, 64, "%s:%d", interfaceName ? interfaceName : "all", port);
	FskInstrumentedItemNew(http, http->name, &gFskHTTPServerTypeInstrumentation);

	if (interfaceName) {
		err = FskHTTPServerListenerAdd(http, port, interfaceName, NULL);
	}
	else {
		FskNetInterfaceRecord *ifc;
		int i, numI;
		http->all = true;
		numI = FskNetInterfaceEnumerate();
		for (i=0; i<numI; i++) {
			FskErr notErr = FskNetInterfaceDescribe(i, &ifc);
			if (notErr) continue;
			if (ifc->status) {
				notErr = FskHTTPServerListenerAdd(http, port, ifc->name, NULL);
				if (notErr) err = notErr;
			}
			FskNetInterfaceDescriptionDispose(ifc);
		}
	}

	http->interfaceNotifier = FskNetInterfaceAddNotifier(httpServerInterfaceChanged, http, "http server");

bail:
	*server = http;
	return err;
}

FskErr FskHTTPServerDispose(FskHTTPServer http) {
	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "FskHTTPServerDispose %p  - useCount: %d\n", http, http ? http->useCount : 0);
	if (http) {
		// remove existing requests
		while (http->activeRequests) {
			FskHTTPServerRequest request = http->activeRequests;
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestRemainsOnClose, request);
			if (kFskErrNone != doCallCondition(http->callbacks->requestCondition, request, kFskHTTPConditionConnectionTerminating, request->refCon))
				FskHTTPServerRequestDispose(request);
		}

		FskNetInterfaceRemoveNotifier(http->interfaceNotifier);
		while (http->listeners)
			FskHTTPServerListenerDispose(http->listeners);

		sFskHTTPServerDownUse(http);
	}
	return kFskErrNone;
}

FskErr sFskHTTPServerDispose(FskHTTPServer http) {
	FskErr		err = kFskErrNone;

	if (!http)
		goto bail;

	if (http->certs != NULL)
		FskNetUtilDisposeCertificate(http->certs);
	FskInstrumentedItemDispose(http);
	FskMemPtrDispose(http);
bail:
	return err;
}

FskErr FskHTTPServerGetStats(FskHTTPServer http, FskHTTPServerStats *stats) {

	if (http)
		FskMemCopy(stats, &http->stats, sizeof(http->stats));
	else
		return kFskErrParameterError;
	return kFskErrNone;
}

FskErr FskHTTPServerStart(FskHTTPServer http) {
	if (!http)
		return kFskErrInvalidParameter;

	if (http->stopped) {
		FskInstrumentedItemSendMessage(http, kFskHTTPInstrMsgServerStart, http);
		http->stopped = false;
		FskTimeGetNow(&http->stats.serverStarted);
	}
	else {
		FskInstrumentedItemSendMessage(http, kFskHTTPInstrMsgServerStartedAlready, http);
	}
	return kFskErrNone;
}

FskErr FskHTTPServerStop(FskHTTPServer http, Boolean flush) {

	if (!http)
		return kFskErrInvalidParameter;

	if (http->stopped == false) {
		FskHTTPServerRequest request = http->activeRequests;
		http->stopped = true;
		FskTimeGetNow(&http->stats.serverStopped);
		// -- should we kill off live requests here?	no. can restart.
		// -- should we kill off active sessions here?	yes when waiting on a keep alive.
		while (request) {
			if (request->state == kHTTPReadRequestHeaders) {
				request->state = kHTTPClose;
				FskHTTPServerRequestCycle(request);
				request = http->activeRequests;
			}
			else
				request = request->next;
		}
	}
	else {
		FskInstrumentedItemSendMessage(http, kFskHTTPInstrMsgServerStoppedAlready, http);
	}
	return kFskErrNone;
}

void FskHTTPServerSetBufferSize(FskHTTPServer http, int bufferSize) {
	if (http) http->defaultBufferSize = bufferSize;
}

void FskHTTPServerSetKeepAliveTimeout(FskHTTPServer http, int timeout) {
	if (http) http->keepAliveTimeout = timeout;
}


void* FskHTTPServerGetRefcon(FskHTTPServer http) {
	if (http) return http->refCon;
	else return NULL;
}

void FskHTTPServerSetRefcon(FskHTTPServer http, void *refCon) {
	if (http) http->refCon = refCon;
}

FskErr FskHTTPServerSetCertificates(FskHTTPServer http, FskSocketCertificateRecord *certs)
{
	if (certs == NULL)
		return kFskErrNone;
	http->certs = FskNetUtilCopyCertificate(certs);
	if (http->certs == NULL)
		return kFskErrMemFull;
	return kFskErrNone;
}

void FskHTTPServerRequestSetRefcon(FskHTTPServerRequest request, void *ref) {
	if (request) request->refCon = ref;
}

void *FskHTTPServerRequestGetRefcon(FskHTTPServerRequest request) {
	if (request) return request->refCon;
	return NULL;
}

void FskHTTPServerRequestSetKeepAliveTimeout(FskHTTPServerRequest request, int timeout) {
	if (request) request->keepAliveTimeout = timeout;
}

int FskHTTPServerRequestGetKeepAliveTimeout(FskHTTPServerRequest request) {
	if (request) 
		return request->keepAliveTimeout ? request->keepAliveTimeout : request->http->keepAliveTimeout;
	return 0;
}

FskHTTPServer FskHTTPServerRequestGetServer(FskHTTPServerRequest request) {
	if (request) return request->http;
	return NULL;
}
FskErr FskHTTPServerRequestGetStats(FskHTTPServerRequest request, FskHTTPServerRequestStatsRecord *stats) {
	if (request)
		FskMemCopy(stats, &request->stats, sizeof(request->stats));
	else
		return kFskErrParameterError;
	return kFskErrNone;
}

FskHeaders *FskHTTPServerRequestGetRequestHeaders(FskHTTPServerRequest request) {
	if (request) return request->requestHeaders;
	return NULL;
}
FskHeaders *FskHTTPServerRequestGetResponseHeaders(FskHTTPServerRequest request) {
	if (request) return request->responseHeaders;
	return NULL;
}

FskErr FskHTTPServerRequestGetLocalAddress(FskHTTPServerRequest request, int *addr, int *port) {
	if (request && request->skt) {
		FskNetSocketGetLocalAddress(request->skt, (UInt32 *)addr, port);
		return kFskErrNone;
	}
	else
		return kFskErrInvalidParameter;
}

void FskHTTPServerRequestDispose(FskHTTPServerRequest request) {
	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "FskHTTPServerRequestDispose %p  - useCount: %d\n", request, request ? request->useCount : 0);
	if (request) {
		FskTimeCallbackDispose(request->timer);
		FskListRemove((FskList*)&request->http->activeRequests, request);
		FskThreadRemoveDataHandler(&request->dataHandler);
		FskTimeCallbackDispose(request->cycleCallback);
		request->cycleCallback = NULL;
		FskTimeCallbackDispose(request->keepAliveKillCallback);
		request->keepAliveKillCallback = NULL;
		request->keepAliveTimeout = 0;
		request->http->stats.connectionsCompleted++;
		sFskHTTPServerRequestDownUse(request);
	}
}

void FskHTTPServerRequestCycle(FskHTTPServerRequest request) {
	httpServerEngineCycle(request);
}

void sFskHTTPServerRequestDispose(FskHTTPServerRequest request) {
	FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "sFskHTTPServerRequestDispose %p  - useCount: %d\n", request, request ? request->useCount : 0);
	if (!request)
		return;

	FskInstrumentedItemDispose(request);
	FskNetSocketClose(request->skt);
	FskHeaderStructDispose(request->requestHeaders);
	FskHeaderStructDispose(request->responseHeaders);
	FskMemPtrDispose(request->in.buf);
	FskMemPtrDispose(request->out.buf);
	FskMemPtrDispose(request);
}

static void httpProcessRequestHeaders(FskHTTPServerRequest request) {
	char 		*str;
	FskHeaders* headers = request->requestHeaders;
	UInt32 version = FskHeaderHTTPVersion(headers);
	char* host = FskHeaderFind(kFskStrHost, headers);
	char* uri = FskHeaderURI(headers);
	char* filename = FskHeaderFilename(headers);
	
	request->state = kHTTPReadRequestBody;

	if (FskStrCompareWithLength(uri, "http://", 7) == 0) {
		// remove host from filename
		char* p = FskStrStr(filename, "://") + 3;
		p = FskStrChr(p, '/') + 1;
		FskMemMove(filename, p, FskStrLen(p) + 1);
	}
	else {
		if (host) {
			if (FskMemPtrNewClear(FskStrLen(host) + FskStrLen(uri) + 9, &str) != kFskErrNone)
				headers->responseCode = 500;
			else {
				FskStrCat(str, "http://");
				FskStrCat(str, host);
				FskStrCat(str, "/");
				FskStrCat(str, headers->URI);
				FskMemPtrDispose(headers->URI);
				headers->URI = str;
			}
		}
		else if (version >= kFskHTTPVersion1dot1)
			headers->responseCode = 400;
		else if (version == kFskHTTPVersion1dot0) {
			if (FskMemPtrNewClear(FskStrLen(uri) + 9, &str) != kFskErrNone)
				headers->responseCode = 500;
			else {
				FskStrCat(str, "http:///");
				FskStrCat(str, headers->URI);
				FskMemPtrDispose(headers->URI);
				headers->URI = str;
			}
		}
	}
	
	str = FskHeaderFind(kFskStrConnection, request->requestHeaders);
	if (str && FskStrCompareCaseInsensitiveWithLength(str, kFskStrClose, 5) == 0) 
		request->keepAlive = false;
	else
		request->keepAlive = true;

	str = FskHeaderFind(kFskStrContentLength, request->requestHeaders);
	if (str) {
		request->requestBodyContentLength = FskStrToNum(str);
		request->stats.expectedBytesToReceive = FskStrToNum(str);
	}
	else
		request->stats.expectedBytesToReceive = 0;

	str = FskHeaderFind(kFskStrTransferEncoding, request->requestHeaders);
	if (str && (FskStrCompareCaseInsensitiveWithLength(str, kFskStrChunked, FskStrLen(kFskStrChunked)) == 0))
		request->requestBodyChunked = true;
	else
		request->requestBodyChunked = false;
	
	doCallCondition(request->http->callbacks->requestCondition, request, kFskHTTPConditionRequestReceivedRequestHeaders, request->refCon);

	if (NULL != (str = FskHeaderFind(kFskStrExpect, request->requestHeaders))) {
		if (0 == FskStrCompareCaseInsensitive(kFskStr100Continue, str))
			request->state = kHTTPFulfillExpectation;
		else
			request->state = kHTTPDenyExpectation;
	}
}

static char *httpProtocolVersionString(FskHTTPServerRequest request) {
	if (request->responseHeaders->protocol)
		return request->responseHeaders->protocol;
	if (FskHeaderHTTPVersion(request->requestHeaders) > kFskHTTPVersion1dot0)
		return "HTTP/1.1";
	else
		return "HTTP/1.0";
}

static void httpPrepareResponseHeaders(FskHTTPServerRequest request) {
	char	*str;
	int		requestProtocolVersion;

	requestProtocolVersion = FskHeaderHTTPVersion(request->requestHeaders);

	if (NULL == FskHeaderFind(kFskStrServer, request->responseHeaders))
		FskHeaderAddString(kFskStrServer, kHTTPServerIdentifier, request->responseHeaders);

	str = FskHeaderFind(kFskStrConnection, request->responseHeaders);
	if (str && FskStrCompareCaseInsensitiveWithLength(str, kFskStrClose, 5) == 0)
		request->keepAlive = false;

	str = FskHeaderFind(kFskStrTransferEncoding, request->responseHeaders);
	if (str && (FskStrCompareCaseInsensitiveWithLength(str, kFskStrChunked, FskStrLen(kFskStrChunked)) == 0)) {
		request->transferEncoding = kFskTransferEncodingChunked;
	}
	else {
		request->transferEncoding = kFskTransferEncodingNone;
		str = FskHeaderFind(kFskStrContentLength, request->responseHeaders);
		if (str) {
			request->stats.expectedBodyToSend = FskStrToNum(str);
		}
		else if (requestProtocolVersion >= kFskHTTPVersion1dot1) {
			// DHWG 7.8.1 - if http server is responding to 1.1 requests,
			// it must use connection:close if there's no content length
			// or chunked encoding
			FskHeaderRemove(kFskStrConnection, request->responseHeaders);
			FskHeaderAddString(kFskStrConnection, kFskStrClose, request->responseHeaders);
		}
	}
	
	if (requestProtocolVersion <= kFskHTTPVersion1dot0) {
		request->keepAlive = false;		// DHWG 7.8.21 (must ignore keepalive)
		FskHeaderRemove(kFskStrConnection, request->responseHeaders);
		FskHeaderAddString(kFskStrConnection, kFskStrClose, request->responseHeaders);
		FskMemPtrDispose(request->requestHeaders->protocol);

		request->requestHeaders->protocol = FskStrDoCopy("HTTP/1.0");

		if (0 == FskStrCompareCaseInsensitiveWithLength(FskHeaderFind(kFskStrTransferEncoding, request->responseHeaders), kFskStrChunked, FskStrLen(kFskStrChunked))) {
			FskHeaderRemove(kFskStrTransferEncoding, request->responseHeaders);
			request->transferEncoding = kFskTransferEncodingNone;
		}
		if (request->stats.expectedBodyToSend == 0) {
			FskHeaderRemove(kFskStrConnection, request->responseHeaders);
			FskHeaderAddString(kFskStrConnection, kFskStrClose, request->responseHeaders);
		}
	}

	if (request->requestHeaders->responseCode >= 400) {
		request->out.max = snprintf(request->out.buf, request->out.bufferSize, "%s %d %s\r\n", httpProtocolVersionString(request), request->requestHeaders->responseCode, FskFindResponse(request->requestHeaders->responseCode));
	}
	else if (request->responseHeaders->responseCode > 0) {
		str = FskFindResponse(request->responseHeaders->responseCode);
		request->out.max = snprintf(request->out.buf, request->out.bufferSize, "%s %d %s\r\n", httpProtocolVersionString(request), request->responseHeaders->responseCode, str ? str : "");
		if (request->keepAlive) {
			FskHeaderRemove(kFskStrConnection, request->responseHeaders);
			FskHeaderAddString(kFskStrConnection, kFskStrKeepAlive, request->responseHeaders);
		}
	}
	else {
		request->responseHeaders->responseCode = 500;
		request->out.max = snprintf(request->out.buf, request->out.bufferSize, "%s 500 Internal Sever Error\r\n", httpProtocolVersionString(request));
	}

	if (NULL == FskHeaderFind(kFskStrDate, request->responseHeaders)) {
		char dateString[32];
		FskTimeMakeDate(dateString, 31);
		FskHeaderAddString(kFskStrDate, dateString, request->responseHeaders);
	}

	// *** generate the response
	request->out.max += FskHeaderGenerateOutputBlob(&request->out.buf[request->out.max], request->out.bufferSize - request->out.max, true, request->responseHeaders);

	if ((!request->keepAlive) || (request->responseHeaders->responseCode >= 400))
		request->nextState = kHTTPDone;
}

static void httpKillKeepAlive(FskTimeCallBack cb, const FskTime when, void *param) {
	FskHTTPServerRequest request = (FskHTTPServerRequest)param;
	FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestKillIdle, request);
	request->state = kHTTPClose;
	httpServerEngineCycle(param);
}

static void httpServerDataHandler(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon) {
	httpServerEngineCycle((FskHTTPServerRequest)refCon);
}

static void httpServerTimeCycle(FskTimeCallBack cb, const FskTime when, void *param) {
	httpServerEngineCycle(param);
}

static void httpServerEngineCycle(void *param) {
	FskErr err = kFskErrNone, retVal = kFskErrNeedMoreTime;
	FskHTTPServerRequest request = (FskHTTPServerRequest)param;
	int amt, ret, chunkSize = 0;
	UInt32 chunkSizeL;
	char *s, *p;
	Boolean readSomeMore, needsDispose = false;

	FskThreadRemoveDataHandler(&request->dataHandler);
	
	switch (request->state) {
		case kHTTPNewSession:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			FskTimeGetNow(&request->stats.requestStarted);
			request->state = kHTTPReadRequestHeaders;
			
		case kHTTPReadRequestHeaders:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			shoveBuffer(request->in);
			amt = request->in.bufferSize - request->in.max;
			if (amt) {
				err = FskNetSocketRecvTCP(request->skt, &request->in.buf[request->in.max], amt, &ret);
				switch (err) {
					case kFskErrNone:
#if SUPPORT_INSTRUMENTATION
						if (FskInstrumentedItemHasListeners(request)) {
							FskHTTPInstrMsgDataRecord msg;
							msg.buffer = &request->in.buf[request->in.max];
							msg.amt = ret;
							FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestRecvData, &msg);
						}
#endif
						request->http->stats.requestsStarted += 1;
						request->in.max += ret;
						request->state = kHTTPProcessRequestHeaders;
						request->stats.bytesReceived += ret;
						request->http->stats.bytesReceived += ret;
						break;
					case kFskErrNoData:
						retVal = kFskErrNoData;
						break;
					case kFskErrConnectionClosed:
						if (request->stats.bytesReceived) {
							request->state = kHTTPSocketError;
						}
						else {
							request->state = kHTTPDone;
						}
						break;
					default:
						FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgErrString, "kHTTPReadRequestHeaders: RecvTCP - error");
						request->state = kHTTPSocketError;
						break;
				}
			}
			else
				request->state = kHTTPProcessRequestHeaders;

			if (request->state != kHTTPProcessRequestHeaders)
				break;

		case kHTTPProcessRequestHeaders:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			amt = request->in.max - request->in.pos;
			if (amt) {
				ret = FskHeadersParseChunk(&request->in.buf[request->in.pos], amt, kFskHeaderTypeRequest, request->requestHeaders);
				if (ret < 0) {
					err = kFskErrBadData;
					request->state = kHTTPSocketError;
					break;
				}
				request->in.pos += ret;
				
				if (request->requestHeaders->headersParsed) {
					httpProcessRequestHeaders(request);
				}
				else if (ret != amt) {
					// odd case - we didn't consume all the data, but
					// the header parsing isn't complete.
					request->state = kHTTPServerError;
				}
			}
			else 
				request->state = kHTTPReadRequestHeaders;

			if (request->state != kHTTPReadRequestBody)
				break;

		case kHTTPReadRequestBody:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			readSomeMore = false;
			amt = request->in.max - request->in.pos;
			if (request->requestBodyContentLength > 0) {
				if (amt) {
					request->state = kHTTPProcessRequestBody;
				}
				else {
					request->in.max = 0;
					request->in.pos = 0;
					readSomeMore = true;
				}
			}
			else if (request->requestBodyChunked) {		// chunked
				if (amt == 0) {
					readSomeMore = true;
				}
				else {
					p = &request->in.buf[request->in.pos];
					while ((amt > 1) && lineEnd(p)) {
						// consume line-ends
						amt -= 2;
						request->in.pos += 2;
						p += 2;
					}
					while ((amt > 1) && !lineEnd(p)) {
						// scan for chunk size
						amt--;
						p++;
					}
					if ((amt > 1) && lineEnd(p)) {
						// convert the chunksize
						s = &request->in.buf[request->in.pos];
						chunkSize = FskStrHexToNum(s, p-s);
						p += 2;		//lineend
						request->requestBodyContentLength = chunkSize;
						request->in.pos += (p-s);
						request->state = kHTTPReadRequestBody;
						if (0 == chunkSize)	{
							// we've read the end indicator (0)
							if ((amt > 1) && lineEnd(p))
								request->in.pos += 2;			// consume last cr/lf							}
							request->requestBodyChunked = false;
						}
					}
					else {
						readSomeMore = true;
					}
				}
			}
			else {
				// we're done reading chunks
				// we're done reading the request
				request->state = kHTTPPrepareResponse;
				doCallCondition(request->http->callbacks->requestCondition, request, kFskHTTPConditionRequestRequestFinished, request->refCon);
			}

			if (readSomeMore) {
				shoveBuffer(request->in);
				err = FskNetSocketRecvTCP(request->skt, &request->in.buf[request->in.max], request->in.bufferSize - request->in.max, &ret);
				switch (err) {
					case kFskErrNone:
#if SUPPORT_INSTRUMENTATION
						if (FskInstrumentedItemHasListeners(request)) {
							FskHTTPInstrMsgDataRecord msg;
							msg.buffer = &request->in.buf[request->in.max];
							msg.amt = ret;
							FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestRecvData, &msg);
						}
#endif
						request->in.max += ret;
						if (request->requestBodyChunked) 		// chunked?
							request->state = kHTTPReadRequestBody;
						else
							request->state = kHTTPProcessRequestBody;

						request->stats.bytesReceived += ret;
						request->http->stats.bytesReceived += ret;
						break;
					case kFskErrNoData:
						retVal = kFskErrNoData;
						break;
					default:
						FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgErrString, "kHTTPReadRequestBody: RecvTCP - error");
						request->http->stats.requestsFailed += 1;
						request->state = kHTTPSocketError;
						break;
				}
			}

			if (request->state != kHTTPProcessRequestBody)
				break;

		case kHTTPProcessRequestBody:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			amt = request->in.max - request->in.pos;
			if (amt > request->requestBodyContentLength) {
				if (false == request->requestBodyChunked)
					request->requestBodyContentLength = amt;
				else
					amt = request->requestBodyContentLength;
			}
			chunkSizeL = (UInt32)chunkSize;
			err = doDataCallback(request->http->callbacks->requestReceiveRequest, request, &request->in.buf[request->in.pos], amt, &chunkSizeL, request->refCon);
			chunkSize = (int)chunkSizeL;
			if (kFskErrNone == err) {
				if (chunkSize) {
					request->in.pos += chunkSize;
					request->requestBodyContentLength -= chunkSize;
					request->stats.requestBodyReceived += chunkSize;
					if (false == request->requestBodyChunked) {
						if (0 == request->requestBodyContentLength)
							request->state = kHTTPPrepareResponse;
						else
							request->state = kHTTPReadRequestBody;
					}
					else
						request->state = kHTTPReadRequestBody;
				}
				else {
					// data callback wants to suspend the session and not
					// consume the chunk, it can do so
					if (request->state != kHTTPSessionSuspend)
						request->state = kHTTPServerError;
				}
			}
			else {
				// the data callback returned an error.
				if (request->state != kHTTPSessionSuspend)
					request->state = kHTTPServerError;
			}

			if (request->state != kHTTPPrepareResponse)
				break;
			doCallCondition(request->http->callbacks->requestCondition, request, kFskHTTPConditionRequestRequestFinished, request->refCon);
			if (request->state != kHTTPPrepareResponse)
				break;

		case kHTTPPrepareResponse:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			request->state = kHTTPProcessResponse;
			err = doCallCondition(request->http->callbacks->requestCondition, request, kFskHTTPConditionRequestGenerateResponseHeaders, request->refCon);
			if (err == kFskErrNeedMoreTime)
				request->state = kHTTPSessionSuspend;
			else if (err)
				request->responseHeaders->responseCode = 500;
				
			if (request->state != kHTTPProcessResponse)
				break;
				
		case kHTTPProcessResponse:
			request->state = kHTTPGetDataChunk;
			httpPrepareResponseHeaders(request);
			if (request->state != kHTTPGetDataChunk)
				break;

		case kHTTPGetDataChunk:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			if (0 == FskStrCompare(kFskStrHEAD, FskHeaderMethod(request->requestHeaders))) {
				request->state = kHTTPSendDataChunk;
				request->nextState = kHTTPSetupNextRequest;
				break;
			}
			p = &request->out.buf[request->out.max];
			if (request->transferEncoding == kFskTransferEncodingChunked) {
				request->out.max += 6;
			}
			chunkSize = 0;
			amt = (request->out.bufferSize - request->out.max) - 2;
			// fetch response data from callback
			chunkSizeL = (UInt32)chunkSize;
			err = doDataCallback(request->http->callbacks->requestGenerateResponseBody, request, &request->out.buf[request->out.max], amt, &chunkSizeL, request->refCon);
			chunkSize = (int)chunkSizeL;
			FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "doDataCallback returns err: %d, chunkSize: %d\n", err, chunkSize);
			if ((kFskErrNone != err) && (kFskErrEndOfFile != err)) {
				FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "about to doCallCondition -requestResponseFinished - FAIL\n");
				doCallCondition(request->http->callbacks->requestCondition, request, kFskHTTPConditionRequestResponseFinished, request->refCon);
				request->http->stats.requestsFailed += 1;
				request->state = kHTTPServerError;
				break;
			}
			request->out.max += chunkSize;
			if ((0 == chunkSize) && (request->state == kHTTPSessionSuspend)) {
                if (kFskTransferEncodingChunked == request->transferEncoding)
                    request->out.max -= 6;
				break;
            }

			request->state = kHTTPSendDataChunk;
			if ((chunkSize == 0) || (kFskErrEndOfFile == err)) {
				request->nextState = kHTTPSetupNextRequest;
			}
			else
				request->nextState = kHTTPGetDataChunk;
			if (request->transferEncoding == kFskTransferEncodingChunked) {
				FskStrNumToHex(chunkSize, p, 4);
				p += 4;
				*p++ = kFskCR;
				*p++ = kFskLF;
				if (chunkSize)
					p += chunkSize;
				*p++ = kFskCR;
				*p++ = kFskLF;
				request->out.max += 2;
			}
			request->stats.bodyBytesSent += chunkSize;

			if (request->state != kHTTPSendDataChunk)
				break;

		case kHTTPSendDataChunk:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, " - request->out.max %d - request->out.pos %d\n", request->out.max, request->out.pos);
			amt = request->out.max - request->out.pos;
			if (0 == amt) {
				FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "amt is zero now - transition to next state.\n");
				request->state = request->nextState;
				break;
			}
			err = FskNetSocketSendTCP(request->skt, &request->out.buf[request->out.pos], amt, &ret);
			switch (err) {
				case kFskErrNone:
#if SUPPORT_INSTRUMENTATION
					if (FskInstrumentedItemHasListeners(request)) {
						FskHTTPInstrMsgDataRecord msg;
						msg.buffer = &request->out.buf[request->out.pos];
						msg.amt = ret;
						FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestSendData, &msg);
					}
#endif
					request->out.pos += ret;
					request->stats.bytesSent += ret;
					request->http->stats.bytesSent += ret;
					if (request->transferEncoding == kFskTransferEncodingChunked) {
						request->stats.bytesSent -= 8;
					}
					if (request->out.pos == request->out.max) {
						request->out.pos = 0;
						request->out.max = 0;
						request->state = request->nextState;
					}
					break;
				case kFskErrNoData:
					retVal = kFskErrSocketFull;
					break;
				default:
					FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgErrString, "kHTTPSendDataChunk: SendTCP - error");
					request->state = kHTTPSocketError;
					request->http->stats.requestsFailed += 1;
					break;
			}			

			if (request->state != kHTTPSetupNextRequest)
				break;

		case kHTTPSetupNextRequest:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			FskTimeGetNow(&request->stats.requestStopped);
			request->http->stats.requestsFinished += 1;
			doCallCondition(request->http->callbacks->requestCondition, request, kFskHTTPConditionRequestResponseFinished, request->refCon);
			request->state = kHTTPDone;
			if (request->keepAlive && !request->http->stopped) {
				if ((request->in.max - request->in.pos) > 0) {
					FskHeaderStructDispose(request->requestHeaders);
					FskHeaderStructDispose(request->responseHeaders);
					FskHeaderStructNew(&request->requestHeaders);
					FskHeaderStructNew(&request->responseHeaders);
					request->keepAlive = false;
					request->out.max = 0;
					request->out.pos = 0;
					FskMemSet(&request->stats, 0, sizeof(request->stats));
					FskTimeGetNow(&request->stats.requestStarted);
					request->state = kHTTPProcessRequestHeaders;
				}
			}

			if (request->state != kHTTPDone)
				break;

		case kHTTPDone:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			if (request->keepAlive && !request->http->stopped) {
				FskHeaderStructDispose(request->requestHeaders);
				FskHeaderStructDispose(request->responseHeaders);
				FskHeaderStructNew(&request->requestHeaders);
				FskHeaderStructNew(&request->responseHeaders);
				request->keepAlive = false;
				request->in.max = 0;
				request->in.pos = 0;
				request->out.max = 0;
				request->out.pos = 0;
				FskMemSet(&request->stats, 0, sizeof(request->stats));
				retVal = kFskErrNoData;		// will cause data handler to be installed 
				request->state = kHTTPNewSession;
			}
			else {
				request->state = kHTTPClose;
				retVal = kFskErrNeedMoreTime;
			}
			break;

		case kHTTPFulfillExpectation:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			request->state = kHTTPSendDataChunk;
			request->nextState = kHTTPReadRequestBody;
			request->out.max += snprintf(request->out.buf, request->out.bufferSize, "%s %d %s\r\n\r\n", httpProtocolVersionString(request), 100, FskFindResponse(100));
			break;

		case kHTTPDenyExpectation:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			request->state = kHTTPSendDataChunk;
			request->nextState = kHTTPDone;
			request->out.max += snprintf(request->out.buf, request->out.bufferSize, "%s %d %s\r\n\r\n", httpProtocolVersionString(request), 417, FskFindResponse(417));
			break;

		case kHTTPServerError:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			request->state = kHTTPSocketError;
			// fall through
		case kHTTPSocketError:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			request->http->stats.connectionsAborted++;
			doCallCondition(request->http->callbacks->requestCondition, request, kFskHTTPConditionRequestErrorAbort, request->refCon);
			request->state = kHTTPClose;
			// fall through
		case kHTTPClose:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			request->state = kHTTPRequestComplete;
			needsDispose = true;
			break;

		case kHTTPRequestComplete:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			retVal = kFskErrNone;				// request is finished, don't call back
			break;

		case kHTTPSessionSuspend:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			retVal = kFskErrNone;				// do nothing in suspend state
			break;
		
		default:
			FskInstrumentedItemSendMessage(request, kFskHTTPInstrMsgRequestState, request);
			request->state = kHTTPClose;		// unknown state
			break;
			;
	}

	if ((request->state == kHTTPServerError)
		|| (request->state == kHTTPSocketError)
		|| (request->state == kHTTPDone)
		|| (request->state == kHTTPClose))
		retVal = kFskErrNeedMoreTime;

	if (retVal == kFskErrNoData) {
		FskThreadAddDataHandler(&request->dataHandler, (FskThreadDataSource)request->skt, httpServerDataHandler, true, false, request);
	}
	else if (retVal == kFskErrSocketFull) {
		FskThreadAddDataHandler(&request->dataHandler, (FskThreadDataSource)request->skt, httpServerDataHandler, false, true, request);
	}
	else if (retVal == kFskErrNeedMoreTime) {
		FskTimeCallbackScheduleNextRun(request->cycleCallback, httpServerTimeCycle, request);
	}
	else if (retVal == kFskErrNone) {
		// nothing doin
	}
	else {
		FskInstrumentedTypePrintfDebug(&gFskHTTPServerTypeInstrumentation, "httpCycle - weird retVal %d\n", retVal);
	}

	if (needsDispose) {
		FskListRemove((FskList*)&request->http->activeRequests, request);
		FskTimeCallbackDispose(request->keepAliveKillCallback);
		request->keepAliveKillCallback = NULL;
		if (kFskErrNone != doCallCondition(request->http->callbacks->requestCondition, request, kFskHTTPConditionConnectionTerminating, request->refCon))
			FskHTTPServerRequestDispose(request);
	}
	else {
		if ((retVal == kFskErrNeedMoreTime) || ((retVal != kFskErrNone) && request->keepAlive)) {
			FskTimeCallbackScheduleFuture(request->keepAliveKillCallback, FskHTTPServerRequestGetKeepAliveTimeout(request), 0, httpKillKeepAlive, request);
		}
	}

}

FskErr FskHTTPServerSetCallbacks(FskHTTPServer http, FskHTTPServerCallbacks callbacks) {
	if (http) {
		http->callbacks = callbacks;
		return kFskErrNone;
	}
	return kFskErrInvalidParameter;
}

FskErr FskHTTPServerGetCallbacks(FskHTTPServer http, FskHTTPServerCallbacks *callbacks) {
	if (http && callbacks) {
		*callbacks = http->callbacks;
		return kFskErrNone;
	}
	return kFskErrInvalidParameter;
}

FskErr FskHTTPServerRequestSuspend(FskHTTPServerRequest request)
{
    if (kHTTPSessionSuspend != request->state) {
        request->suspendedState = request->state;
        request->state = kHTTPSessionSuspend;
    }
    return kFskErrNone;
}

FskErr FskHTTPServerRequestResume(FskHTTPServerRequest request)
{
    request->state = request->suspendedState;
	FskTimeCallbackScheduleNextRun(request->cycleCallback, httpServerTimeCycle, request);
    return kFskErrNone;
}

#if SUPPORT_INSTRUMENTATION
const char *httpserverstate2str(UInt32 state) {
switch (state) {
	case kHTTPNewSession: return "kHTTPNewSession";
	case kHTTPReadRequestHeaders: return "kHTTPReadRequestHeaders";
	case kHTTPProcessRequestHeaders: return "kHTTPProcessRequestHeaders";
	case kHTTPReadRequestBody: return "kHTTPReadRequestBody";
	case kHTTPProcessRequestBody: return "kHTTPProcessRequestBody";
	case kHTTPPrepareResponse: return "kHTTPPrepareResponse";
	case kHTTPProcessResponse: return "kHTTPProcessResponse";
	case kHTTPGetDataChunk: return "kHTTPGetDataChunk";
	case kHTTPSendDataChunk: return "kHTTPSendDataChunk";
	case kHTTPSetupNextRequest: return "kHTTPSetupNextRequest";
	case kHTTPDone: return "kHTTPDone";
	case kHTTPFulfillExpectation: return "kHTTPFulfillExpectation";
	case kHTTPDenyExpectation: return "kHTTPDenyExpectation";
	case kHTTPServerError: return "kHTTPServerError";
	case kHTTPSocketError: return "kHTTPSocketError";
	case kHTTPClose: return "kHTTPClose";
	case kHTTPSessionSuspend: return "kHTTPSessionSuspend";
	case kHTTPRequestComplete: return "kHTTPRequestComplete";
	}
	return "unknown state";
}

static Boolean doFormatMessageHTTPServer(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize) {
	FskHTTPServerListener listener = (FskHTTPServerListener)msgData;
	FskHTTPServerRequest request = (FskHTTPServerRequest)msgData;
	FskHTTPServer http = (FskHTTPServer)msgData;
	FskHTTPInstrMsgDataRecord *data = (FskHTTPInstrMsgDataRecord*)msgData;
	FskThread thread;
		
	char tmp[64];
	UInt32 s;
	const UInt32 kMessageTextSize = 512;

	switch (msg) {
    	case kFskHTTPInstrMsgRequestState:
			thread = FskThreadGetCurrent();
			if (request->http->owner != thread)
				snprintf(buffer, bufferSize, "RUNNING IN (%p:%s) SHOULD BE (%p:%s) %s - %s",
					thread, thread->name, request->http->owner, request->http->owner->name,
					request->http->name, httpserverstate2str(request->state));
			else
				snprintf(buffer, bufferSize, "%s - %s", request->http->name, httpserverstate2str(request->state));
			return true;
    	case kFskHTTPInstrMsgErrString:
			snprintf(buffer, bufferSize, "%s", (char*)msgData);
			return true;
    	case kFskHTTPInstrMsgNowListening:
			snprintf(buffer, bufferSize, "listening to %s:%d", listener->ifcName, listener->port);
			return true;
    	case kFskHTTPInstrMsgFailedListener:
			snprintf(buffer, bufferSize, "no longer listening to %s", listener->http->name);
			return true;
    	case kFskHTTPInstrMsgConnectionRefusedStopped:
			snprintf(buffer, bufferSize, "refused connection - server stopped %s", listener->http->name);
			return true;
    	case kFskHTTPInstrMsgServerStart:
			snprintf(buffer, bufferSize, "%s starting", http->name);
			return true;
    	case kFskHTTPInstrMsgServerStartedAlready:
			snprintf(buffer, bufferSize, "%s already started", http->name);
			return true;
    	case kFskHTTPInstrMsgServerStop:
			snprintf(buffer, bufferSize, "%s stopping", http->name);
			return true;
    	case kFskHTTPInstrMsgServerStoppedAlready:
			snprintf(buffer, bufferSize, "%s already stopped", http->name);
			return true;
    	case kFskHTTPInstrMsgRequestKillIdle:
			FskNetIPandPortToString(request->requesterAddress, request->requesterPort, tmp);
			snprintf(buffer, bufferSize, "%s - killing idle request connection from %s", request->http->name, tmp);
			return true;
		case kFskHTTPInstrMsgRequestRemainsOnClose:
			FskNetIPandPortToString(request->requesterAddress, request->requesterPort, tmp);
			snprintf(buffer, bufferSize, "closing %s - killing request from %s", request->http->name, tmp);
			return true;
		case kFskHTTPInstrMsgRequestRecvData:
		case kFskHTTPInstrMsgRequestSendData:
			s = data->amt -1;
			if ((s + kMessageTextSize) > bufferSize)
				s = bufferSize - kMessageTextSize;
			tmp[0] = data->buffer[s];
			data->buffer[s] = '\0';
			if (msg == kFskHTTPInstrMsgRequestSendData)
				snprintf(buffer, bufferSize, "send data (%u bytes): %s%c", (unsigned)data->amt, data->buffer, tmp[0]);
			else
				snprintf(buffer, bufferSize, "recv data (%u bytes): %s%c", (unsigned)data->amt, data->buffer, tmp[0]);
			data->buffer[s] = tmp[0];
			return true;
		default:
			return false;
	}
	return false;
}
#endif

