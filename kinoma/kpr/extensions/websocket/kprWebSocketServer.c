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
#define __FSKHTTPSERVER_PRIV__
#include "FskNetUtils.h"
#include "FskSSL.h"
#include "FskEndian.h"

#include "kpr.h"
#include "kprAuthentication.h"
#include "kprHTTPClient.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprUtilities.h"
#include "kprWebSocketServer.h"
#include "kprWebSocketCommon.h"

//--------------------------------------------------
// INSTRUMENTATION
//--------------------------------------------------

#define kHeaderBufferSize 512
#define kWebSocketServerIdentifier "Kinoma WebSocket Server/0.1"

static FskErr KprWebSocketServerAcceptNewConnection(KprSocketServer server, FskSocket skt, const char *interfaceName, int ip, void *refcon);
static void KprWebSocketServerInterfaceDropped(KprSocketServer server, const char *interface, int ip, void *refcon);
static FskErr KprWebSocketServerRequestNew(KprWebSocketServerRequest *it, KprWebSocketServer server, FskSocket skt, const char *interface, int ip);
static void KprWebSocketServerRequestDispose(KprWebSocketServerRequest request);
static FskErr KprWebSocketServerRequestDoRead(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);

static FskErr KprWebSocketServerHandleRequest(KprWebSocketServerRequest request);
static int KprWebSocketServerValidateRequest(FskHeaders *requestHeaders, FskHeaders *responseHeaders);
static FskErr KprWebSocketServerSendResponseHeader(KprWebSocketServerRequest request);

static void KprWebSocketServerPrepareResponseHeaders(KprWebSocketServerRequest request);
static void KprWebSocketServerGenerateErrorResponse(KprWebSocketServerRequest request, int statusCode);
static void KprWebSocketServerGenerateUpgradeResponse(KprWebSocketServerRequest request);

//static int KprWebSocketServerInterfaceChanged(FskNetInterfaceRecord *ifc, UInt32 status, void *param);
// Callbacks

//--------------------------------------------------
// WebSocketServer
//--------------------------------------------------

FskErr KprWebSocketServerNew(KprWebSocketServer* it, void *refCon)
{
	FskErr err = kFskErrNone;
	KprWebSocketServer self = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(KprWebSocketServerRecord), it));
	self = *it;

	self->stopped = false;
	self->refCon = refCon;
	self->owner = FskThreadGetCurrent();

//	self->interfaceNotifier = FskNetInterfaceAddNotifier(KprWebSocketServerInterfaceChanged, self, "websocket server");
	return err;
bail:
	KprWebSocketServerDispose(self);
	return err;
}

void KprWebSocketServerDispose(KprWebSocketServer self)
{
	if (self) {
		while (self->activeRequests) {
			KprWebSocketServerRequest request = self->activeRequests;
			KprWebSocketServerRequestDispose(request);
		}

		KprSocketServerDispose(self->server);
		FskMemPtrDispose(self);
	}
}

FskErr KprWebSocketServerListen(KprWebSocketServer self, int port, char *interfaceName)
{
	FskErr err = kFskErrNone;

	if (!self->server) {
		KprSocketServer server = NULL;
		
		bailIfError(KprSocketServerNew(&server, self));
		server->acceptCallback = KprWebSocketServerAcceptNewConnection;
		server->interfaceDroppedCallback = KprWebSocketServerInterfaceDropped;

		server->debugName = "WebSocket Server";

		self->server = server;
	}

	bailIfError(KprSocketServerListen(self->server, port, interfaceName));

	if (self->launchCallback) {
		self->launchCallback(self, self->refCon);
	}

bail:
	if (err && self->errorCallback) {
		self->errorCallback(self, err, "failed to launch", self->refCon);
	}

	return err;
}

UInt16 KprWebSocketServerGetPort(KprWebSocketServer self)
{
	return self->server->port;
}

KprPortListener KprWebSocketServerGetInterface(KprWebSocketServer self)
{
	return self->server->listeners;
}

static FskErr KprWebSocketServerAcceptNewConnection(KprSocketServer server, FskSocket skt, const char *interfaceName, int ip, void *refcon) {
	KprWebSocketServer self = refcon;
	FskErr err = kFskErrNone;
	KprWebSocketServerRequest request;

	if (self->stopped) {
		FskNetSocketClose(skt);
		goto bail;
	}

	bailIfError(KprWebSocketServerRequestNew(&request, self, skt, interfaceName, ip));
	FskListAppend((FskList*)&self->activeRequests, request);

bail:
	return err;
}

static void KprWebSocketServerInterfaceDropped(KprSocketServer server, const char *interface, int ip, void *refcon)
{
	KprWebSocketServer self = refcon;
	printf("INTERFACE %s DROPPED.\n", interface);

	if (self->interfaceDropCallback) {
		self->interfaceDropCallback(self, interface, ip, self->refCon);
	}
}

static FskErr KprWebSocketServerRequestNew(KprWebSocketServerRequest *it, KprWebSocketServer server, FskSocket skt, const char *interface, int ip) {
	FskErr err = kFskErrNone;
	KprWebSocketServerRequest request = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(KprWebSocketServerRequestRecord), &request));

	request->server = server;
	request->skt = skt;
	skt = NULL;

	request->interface = FskStrDoCopy(interface);
	bailIfNULL(request->interface);
	request->ip = ip;

	FskNetSocketGetRemoteAddress(request->skt, (UInt32 *)&request->requesterAddress, &request->requesterPort);
	FskNetSocketMakeNonblocking(request->skt);
	bailIfError(FskHeaderStructNew(&request->requestHeaders));
	bailIfError(FskHeaderStructNew(&request->responseHeaders));

	request->out.size = 512;
	bailIfError(FskMemPtrNew(request->out.size, &request->out.buffer));

	FskThreadAddDataHandler(&request->dataHandler, (FskThreadDataSource)request->skt, (FskThreadDataReadyCallback)KprWebSocketServerRequestDoRead, true, false, request);
	//	request->state = kHTTPNewSession;
	
	*it = request;

bail:
	if (err) {
		KprWebSocketServerRequestDispose(request);
		FskNetSocketClose(skt);
	}

	return err;
}

static void KprWebSocketServerRequestDispose(KprWebSocketServerRequest request) {
	if (request) {
		FskListRemove((FskList*)&request->server->activeRequests, request);
		FskThreadRemoveDataHandler(&request->dataHandler);

		FskMemPtrDispose((void *) request->interface);
		FskNetSocketClose(request->skt);
		FskHeaderStructDispose(request->requestHeaders);
		FskHeaderStructDispose(request->responseHeaders);
		FskStrParsedUrlDispose(request->parts);
		FskMemPtrDispose(request->out.buffer);
		FskMemPtrDispose(request);
	}
}

static FskErr KprWebSocketServerRequestDoRead(FskThreadDataHandler handler UNUSED, FskThreadDataSource source UNUSED, void *refCon)
{
	FskErr err = kFskErrNone;
	KprWebSocketServerRequest self = refCon;
	int size = -1;
	char buffer[kHeaderBufferSize];

	while (err == kFskErrNone) {
		err = FskNetSocketRecvTCP(self->skt, buffer, kHeaderBufferSize, &size);
		if (err == kFskErrNoData) return kFskErrNone;

		FskHeadersParseChunk(buffer, size, kFskHeaderTypeRequest, self->requestHeaders);

		if (self->requestHeaders->headersParsed) {
			KprWebSocketServerHandleRequest(self);

			KprWebSocketServerRequestDispose(self);
			break;
		}
	}

	return err;
}

static FskErr KprWebSocketServerHandleRequest(KprWebSocketServerRequest request)
{
	FskErr err = kFskErrNone;
	int statusCode = 101;
	Boolean doUpgrade = false;

	statusCode = KprWebSocketServerValidateRequest(request->requestHeaders, request->responseHeaders);
	doUpgrade = (statusCode == 101);

	if (doUpgrade) {
		KprWebSocketServerGenerateUpgradeResponse(request);
	} else {
		KprWebSocketServerGenerateErrorResponse(request, statusCode);
	}

	bailIfError(KprWebSocketServerSendResponseHeader(request));

	if (doUpgrade) {
		KprWebSocketServer self = request->server;
		FskSocket skt = request->skt;

		FskThreadRemoveDataHandler(&request->dataHandler);
		request->skt = NULL;

		if (self->connectCallback) {
			self->connectCallback(self, skt, request->interface, request->ip, self->refCon);
		} else {
			FskNetSocketClose(skt);
		}
	}

bail:
	return err;
}

static int KprWebSocketServerValidateRequest(FskHeaders *requestHeaders, FskHeaders *responseHeaders)
{
	FskErr err;
	char *value;
	char *decoded = NULL;
	UInt32 len;
	int statusCode = 400;

	value = FskHeaderFind("Upgrade", requestHeaders);
	if (!value || FskStrCompareCaseInsensitive(value, "websocket") != 0) goto bail;

	value = FskHeaderFind("Connection", requestHeaders);
	if (!value || FskStrCompareCaseInsensitive(value, "Upgrade") != 0) goto bail;

	value = FskHeaderFind("Sec-WebSocket-Version", requestHeaders);
	if (!value || FskStrCompare(value, "13") != 0) {
		statusCode = 426;
		FskHeaderAddString("Sec-WebSocket-Version", "13", responseHeaders);
		goto bail;
	}

	value = FskHeaderFind("Sec-WebSocket-Key", requestHeaders);
	if (!value) goto bail;

	bailIfError(FskStrB64Decode(value, FskStrLen(value), &decoded, &len));
	if (len != 16) goto bail;

	statusCode = 101;

bail:
	FskMemPtrDispose(decoded);

	return statusCode;
}

static FskErr KprWebSocketServerSendResponseHeader(KprWebSocketServerRequest request) {
	FskErr err = kFskErrNone;
	int sent;

	KprWebSocketServerPrepareResponseHeaders(request);

	request->out.length = snprintf((char *) request->out.buffer, request->out.size, "%s %d %s\r\n", "HTTP/1.1", request->responseHeaders->responseCode, FskFindResponse(request->responseHeaders->responseCode));
	request->out.length += FskHeaderGenerateOutputBlob((char *) &request->out.buffer[request->out.length], request->out.size - request->out.length, true, request->responseHeaders);

	err = FskNetSocketSendTCP(request->skt, request->out.buffer, request->out.length, &sent);

	return err;
}

static void KprWebSocketServerPrepareResponseHeaders(KprWebSocketServerRequest request) {
	if (NULL == FskHeaderFind(kFskStrServer, request->responseHeaders))
		FskHeaderAddString(kFskStrServer, kWebSocketServerIdentifier, request->responseHeaders);

	if (NULL == FskHeaderFind(kFskStrContentLength, request->responseHeaders))
		FskHeaderAddString(kFskStrContentLength, "0", request->responseHeaders);

	if (NULL == FskHeaderFind(kFskStrDate, request->responseHeaders)) {
		char dateString[32];
		FskTimeMakeDate(dateString, 31);
		FskHeaderAddString(kFskStrDate, dateString, request->responseHeaders);
	}
}

static void KprWebSocketServerGenerateErrorResponse(KprWebSocketServerRequest request, int statusCode UNUSED)
{
//	FskHeaders *responseHeaders = request->responseHeaders;

	// TODO
}

static void KprWebSocketServerGenerateUpgradeResponse(KprWebSocketServerRequest request)
{
	FskHeaders *responseHeaders = request->responseHeaders;
	char *encoded = NULL;
	char *value;

	responseHeaders->responseCode = 101;

	FskHeaderAddString("Upgrade", "websocket", responseHeaders);
	FskHeaderAddString("Connection", "Upgrade", responseHeaders);

	value = FskHeaderFind("Sec-WebSocket-Key", request->requestHeaders);
	if (value) {
		KprWebSocketCalculateHash(value, &encoded);
		if (encoded) {
			FskHeaderAddString("Sec-WebSocket-Accept", encoded, responseHeaders);
			FskMemPtrDispose(encoded);
		}
	}
}

