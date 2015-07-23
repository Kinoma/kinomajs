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
#define __FSKECMASCRIPT_PRIV__

#include "FskNetUtils.h"
#include "FskSSL.h"
#include "FskEndian.h"
#include "FskECMAScript.h"
#include "xs.h"

#include "kpr.h"
#include "kprAuthentication.h"
#include "kprHTTPClient.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprUtilities.h"
#include "kprWebSocketEndpoint.h"
#include "kprWebSocketServer.h"

static void KPR_WebSocketClient_onOpen(KprWebSocketEndpoint endpoint, void *refcon);
static void KPR_WebSocketClient_onClose(KprWebSocketEndpoint endpoint, UInt16 code, char *reason, Boolean wasClean, void *refcon);
static void KPR_WebSocketClient_onTextMessage(KprWebSocketEndpoint endpoint, char *text, void *refcon);
static void KPR_WebSocketClient_onBinaryMessage(KprWebSocketEndpoint endpoint, void *data, UInt32 length, void *refcon);
static void KPR_WebSocketClient_onError(KprWebSocketEndpoint endpoint, FskErr err, char *message, void *refcon);

typedef struct KPR_WebSocketClientStruct KPR_WebSocketClientRecord;
typedef struct KPR_WebSocketServerStruct KPR_WebSocketServerRecord;

struct KPR_WebSocketClientStruct {
	KPR_WebSocketClientRecord *next;
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	KprWebSocketEndpoint endpoint;
	KPR_WebSocketServerRecord *server;
};

static void KPR_WebSocketServerClientWillDispose(KPR_WebSocketServerRecord *self, KPR_WebSocketClientRecord *client);
static void KPR_websocketserver_setCallback(KPR_WebSocketServerRecord *self);
static void KPR_websocketserver_timeToPing(FskTimeCallBack callback, const FskTime time, void *it);
static void KPR_WebSocketServer_onLaunch(KprWebSocketServer server, void *refcon);
static void KPR_WebSocketServer_onConnect(KprWebSocketServer server, FskSocket skt, void *refcon);
static void KPR_WebSocketServer_onError(KprWebSocketServer server, FskErr err, char *message, void *refcon);
static void KPR_WebSocketServer_onClientClosing(KprWebSocketEndpoint endpoint, UInt16 code, char *reason, Boolean wasClean, void *refcon);
static void KPR_WebSocketServer_onClientClose(KprWebSocketEndpoint endpoint, UInt16 code, char *reason, Boolean wasClean, void *refcon);

struct KPR_WebSocketServerStruct {
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	KprWebSocketServer server;
	KPR_WebSocketClientRecord *clients;

	double pingInterval;
	double pingTimeout;
	FskTimeCallBack pingCallback;
};

#define isObject(x) xsIsInstanceOf(x, xsObjectPrototype)
#define isString(x) xsIsInstanceOf(x, xsStringPrototype)
#define isChunk(x) xsIsInstanceOf(x, xsChunkPrototype)
#define isFunc(x) xsIsInstanceOf(x, xsFunctionPrototype)
#define hasCallback(x) xsFindResult(self->slot, xsID(x)) && isFunc(xsResult)

//--------------------------------------------------
// WebSocket Client Script Interface
//--------------------------------------------------

void KPR_websocketclient(void *it)
{
	KPR_WebSocketClientRecord *self = it;
	if (self) {
		if (self->server) {
			KPR_WebSocketServerClientWillDispose(self->server, self);
		}

		self->endpoint->openCallback = NULL;
		self->endpoint->closeCallback = NULL;
		self->endpoint->textCallback = NULL;
		self->endpoint->binaryCallback = NULL;
		self->endpoint->errorCallback = NULL;
		INVOKE_AFTER1(KprWebSocketEndpointDispose, self->endpoint);
		FskMemPtrDispose(self);
	}
}

void KPR_WebSocketClient(xsMachine* the)
{
	FskErr err;
	xsIntegerValue c = xsToInteger(xsArgc);
	KPR_WebSocketClientRecord *self = NULL;
	KprWebSocketEndpoint endpoint = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KPR_WebSocketClientRecord), &self));
	
	xsThrowIfFskErr(KprWebSocketEndpointNew(&endpoint, self));
	
	endpoint->openCallback = KPR_WebSocketClient_onOpen;
	endpoint->closeCallback = KPR_WebSocketClient_onClose;
	endpoint->textCallback = KPR_WebSocketClient_onTextMessage;
	endpoint->binaryCallback = KPR_WebSocketClient_onBinaryMessage;
	endpoint->errorCallback = KPR_WebSocketClient_onError;
	
	self->endpoint = endpoint;
	self->the = the;
	self->slot = xsThis;
	self->code = the->code;
	xsSetHostData(self->slot, self);
	// xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, self->slot);
	
	if (c >= 1) {
		bailIfError(KprWebSocketEndpointConnect(endpoint, xsToString(xsArg(0)), NULL));
	}

	xsRemember(self->slot);

	return;
	
bail:
	KprWebSocketEndpointDispose(endpoint);
	FskMemPtrDispose(self);
	xsThrowIfFskErr(err);
}

void KPR_websocketclient_send(xsMachine* the)
{
	KPR_WebSocketClientRecord *self = xsGetHostData(xsThis);

	if (isChunk(xsArg(0))) {
		void *data = xsGetHostData(xsArg(0));
		UInt32 length = xsToInteger(xsGet(xsArg(0), xsID_length));

		xsThrowIfFskErr(KprWebSocketEndpointSendBinary(self->endpoint, data, length));
	} else {
		xsThrowIfFskErr(KprWebSocketEndpointSendString(self->endpoint, xsToString(xsArg(0))));
	}
}

void KPR_websocketclient_close(xsMachine* the)
{
	KPR_WebSocketClientRecord *self = xsGetHostData(xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	UInt16 code = 1000;
	char *reason = "closed by script";

	if (c >= 1) {
		code = xsToInteger(xsArg(0));
		if (code != 1000 && !(code >= 3000 && code <= 4999)) xsThrowIfFskErr(kFskErrInvalidParameter);
	}

	if (c >= 2) {
		reason = xsToString(xsArg(1));
		if (FskStrLen(reason) > 123) xsThrowIfFskErr(kFskErrInvalidParameter);
	}

	xsThrowIfFskErr(KprWebSocketEndpointClose(self->endpoint, code, reason));
}

static void KPR_WebSocketClient_OpenDeferred(void *a, void *b UNUSED, void *c UNUSED, void *d UNUSED)
{
	KPR_WebSocketClientRecord *self = a;
	
	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onopen")) {
		xsTry {
			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
		
			(void)xsCallFunction1(xsResult, self->slot, xsVar(0));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();
}

static void KPR_WebSocketClient_onOpen(KprWebSocketEndpoint endpoint UNUSED, void *refcon)
{
	KPR_WebSocketClientRecord *self = refcon;
	
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KPR_WebSocketClient_OpenDeferred, self, NULL, NULL, NULL);
}

static void KPR_WebSocketClient_buildCloseSlots(xsMachine *the, UInt16 code, char *reason, Boolean wasClean)
{
	xsVar(0) = xsNewInstanceOf(xsObjectPrototype);

	xsNewHostProperty(xsVar(0), xsID("code"), xsInteger(code), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("wasClean"), xsBoolean(wasClean), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("reason"), xsString(reason ? reason : ""), xsDefault, xsDontScript);
}

static void KPR_WebSocketClient_onClose(KprWebSocketEndpoint endpoint UNUSED, UInt16 code, char *reason, Boolean wasClean, void *refcon)
{
	KPR_WebSocketClientRecord *self = refcon;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onclose")) {
		xsTry {
			KPR_WebSocketClient_buildCloseSlots(the, code, reason, wasClean);

			(void)xsCallFunction1(xsResult, self->slot, xsVar(0));
		}
		xsCatch {
		}
	}

	xsForget(self->slot);

	xsEndHostSandboxCode();
}

static void KPR_WebSocketClient_onTextMessage(KprWebSocketEndpoint endpoint, char *text, void *refcon)
{
	KPR_WebSocketClientRecord *self = refcon;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onmessage")) {
		xsTry {
			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);

			xsNewHostProperty(xsVar(0), xsID("data"), xsString(text), xsDefault, xsDontScript);

			(void)xsCallFunction1(xsResult, self->slot, xsVar(0));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();
}

static void KPR_WebSocketClient_onBinaryMessage(KprWebSocketEndpoint endpoint, void *data, UInt32 length, void *refcon)
{
	KPR_WebSocketClientRecord *self = refcon;
	void *data2 = NULL;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(2);
	if (hasCallback("onmessage")) {
		xsTry {
			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);

			FskMemPtrNewFromData(length, data, &data2);
			xsMemPtrToChunk(the, &xsVar(1), (FskMemPtr)data2, length, false);
			data2 = NULL; // data is taking over by ECMASCript Object

			xsNewHostProperty(xsVar(0), xsID("data"), xsVar(1), xsDefault, xsDontScript);

			(void)xsCallFunction1(xsResult, self->slot, xsVar(0));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();
}

static void KPR_WebSocketClient_onError(KprWebSocketEndpoint endpoint UNUSED, FskErr err, char *message, void *refcon)
{
	KPR_WebSocketClientRecord *self = refcon;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onerror")) {
		xsTry {
			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);

			xsNewHostProperty(xsVar(0), xsID("code"), xsInteger(err), xsDefault, xsDontScript);
			xsNewHostProperty(xsVar(0), xsID("message"), xsString(message ? message : ""), xsDefault, xsDontScript);

			(void)xsCallFunction1(xsResult, self->slot, xsVar(0));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();
}

void KPR_websocketclient_get_readyState(xsMachine* the)
{
	KPR_WebSocketClientRecord *self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->endpoint->state);
}

void KPR_websocketclient_get_bufferedAmount(xsMachine* the)
{
	KPR_WebSocketClientRecord *self = xsGetHostData(xsThis);
	xsResult = xsInteger(KprWebSocketEndpointGetPendingData(self->endpoint));
}

void KPR_websocketclient_get_protocol(xsMachine* the)
{
	xsResult = xsString("");
}

void KPR_websocketclient_get_url(xsMachine* the)
{
	KPR_WebSocketClientRecord *self = xsGetHostData(xsThis);
	xsResult = xsString(self->endpoint->url);
}

//--------------------------------------------------
// WebSocket Server Script Interface
//--------------------------------------------------

void KPR_websocketserver(void *it)
{
	KPR_WebSocketServerRecord *self = it;

	if (self) {
		KPR_WebSocketClientRecord *client;

		if (self->pingCallback) {
			FskTimeCallbackDispose(self->pingCallback);
		}

		client = self->clients;
		while (client) {
			client->endpoint->closingCallback = NULL;
			client->endpoint->closeCallback = KPR_WebSocketClient_onClose;
			KprWebSocketEndpointClose(client->endpoint, 1001, "server was disposed");

			client = client->next;
		}

		INVOKE_AFTER1(KprWebSocketServerDispose, self->server);
		FskMemPtrDispose(self);
	}
}

void KPR_WebSocketServer(xsMachine* the)
{
	FskErr err;
	xsIntegerValue c = xsToInteger(xsArgc);
	KPR_WebSocketServerRecord *self = NULL;
	KprWebSocketServer server = NULL;
	int port = (c < 1) ? 0 : xsToInteger(xsArg(0));

	bailIfError(FskMemPtrNewClear(sizeof(KPR_WebSocketServerRecord), &self));

	xsThrowIfFskErr(KprWebSocketServerNew(&server, self));

	server->launchCallback = KPR_WebSocketServer_onLaunch;
	server->connectCallback = KPR_WebSocketServer_onConnect;
	server->errorCallback = KPR_WebSocketServer_onError;

	FskTimeCallbackNew(&self->pingCallback);

	self->server = server;
	self->the = the;
	self->slot = xsThis;
	self->code = the->code;
	xsSetHostData(xsThis, self);
	// xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, xsThis);

	xsThrowIfFskErr(KprWebSocketServerListen(server, port, NULL));
	return;

bail:
	KprWebSocketServerDispose(server);
	FskMemPtrDispose(self);
	xsThrowIfFskErr(err);
}

static FskErr KPR_WebSocketServerClientNew(KPR_WebSocketServerRecord *self, KPR_WebSocketClientRecord *client, FskSocket skt)
{
	FskErr err;

	client->server = self;
	client->endpoint->closingCallback = KPR_WebSocketServer_onClientClosing;
	client->endpoint->closeCallback = KPR_WebSocketServer_onClientClose;

	bailIfError(KprWebSocketEndpointOpenWithSocket(client->endpoint, skt));

	FskListAppend((FskList *)&self->clients, client);

bail:
	return err;
}

static void KPR_WebSocketServerClientWillDispose(KPR_WebSocketServerRecord *self, KPR_WebSocketClientRecord *client)
{
	FskListRemove((FskList *)&self->clients, client);
}

void KPR_websocketserver_get_port(xsMachine* the)
{
	KPR_WebSocketServerRecord *self = xsGetHostData(xsThis);
	UInt16 port = KprWebSocketServerGetPort(self->server);
	xsResult = xsInteger(port);
}

void KPR_websocketserver_get_clientCount(xsMachine* the)
{
	KPR_WebSocketServerRecord *self = xsGetHostData(xsThis);
	UInt32 count = FskListCount(self->clients);
	xsResult = xsInteger(count);
}

void KPR_websocketserver_get_pingInterval(xsMachine* the)
{
	KPR_WebSocketServerRecord *self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->pingInterval);
}

void KPR_websocketserver_set_pingInterval(xsMachine* the)
{
	KPR_WebSocketServerRecord *self = xsGetHostData(xsThis);
	xsNumberValue interval = xsToNumber(xsArg(0));

	if (self->pingInterval > 0) {
		FskTimeCallbackRemove(self->pingCallback);
	}

	self->pingInterval = interval;

	KPR_websocketserver_setCallback(self);
}

void KPR_websocketserver_get_pingTimeout(xsMachine* the)
{
	KPR_WebSocketServerRecord *self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->pingTimeout);
}

void KPR_websocketserver_set_pingTimeout(xsMachine* the)
{
	KPR_WebSocketServerRecord *self = xsGetHostData(xsThis);
	self->pingTimeout = xsToNumber(xsArg(0));
}

static void KPR_websocketserver_setCallback(KPR_WebSocketServerRecord *self)
{
	if (self->pingInterval > 0) {
		FskTimeRecord when;
		FskTimeGetNow(&when);
		FskTimeAddMS(&when, self->pingInterval);
		FskTimeCallbackSet(self->pingCallback, &when, KPR_websocketserver_timeToPing, self);
	}
}

static void KPR_websocketserver_timeToPing(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *it)
{
	KPR_WebSocketServerRecord *self = it;
	KPR_WebSocketClientRecord *client;

	client = self->clients;
	while (client) {
		KprWebSocketEndpointSendPing(client->endpoint);
		client = client->next;
	}
	
	KPR_websocketserver_setCallback(self);
}

static void KPR_WebSocketServer_onLaunchDeferred(void *a, void *b UNUSED, void *c UNUSED, void *d UNUSED)
{
	KPR_WebSocketServerRecord *self = a;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onlaunch")) {
		xsTry {
			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);

			(void)xsCallFunction1(xsResult, self->slot, xsVar(0));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();
}

static void KPR_WebSocketServer_onLaunch(KprWebSocketServer server UNUSED, void *refcon)
{
	KPR_WebSocketServerRecord *self = refcon;

	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KPR_WebSocketServer_onLaunchDeferred, self, NULL, NULL, NULL);
}

static void KPR_WebSocketServer_onConnect(KprWebSocketServer server UNUSED, FskSocket skt, void *refcon)
{
	KPR_WebSocketServerRecord *self = refcon;
	FskErr err = kFskErrNone;
	Boolean handled = false;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onconnect")) {
		xsTry {
			xsVar(0) = xsNew0(xsGlobal, xsID("WebSocket"));

			(void)xsCallFunction1(xsResult, self->slot, xsVar(0));

			handled = xsFindResult(xsVar(0), xsID("onopen"))
			       || xsFindResult(xsVar(0), xsID("onmessage"))
				   || xsFindResult(xsVar(0), xsID("onclose"))
				   || xsFindResult(xsVar(0), xsID("onerror"));
			if (handled) {
				KPR_WebSocketClientRecord *rec = xsGetHostData(xsVar(0));

				err = KPR_WebSocketServerClientNew(self, rec, skt);
				if (err == kFskErrNone) {
					skt = NULL;
				}
			}
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();

	FskNetSocketClose(skt);

}

static void KPR_WebSocketServer_onError(KprWebSocketServer server UNUSED, FskErr err, char *message, void *refcon)
{
	KPR_WebSocketServerRecord *self = refcon;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onerror")) {
		xsTry {
			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);

			xsNewHostProperty(xsVar(0), xsID("code"), xsInteger(err), xsDefault, xsDontScript);
			xsNewHostProperty(xsVar(0), xsID("message"), xsString(message ? message : ""), xsDefault, xsDontScript);

			(void)xsCallFunction1(xsResult, self->slot, xsVar(0));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();
}

static void KPR_WebSocketServer_onClientClosing(KprWebSocketEndpoint endpoint UNUSED, UInt16 code, char *reason, Boolean wasClean, void *refcon)
{
	KPR_WebSocketClientRecord *client = refcon;
	KPR_WebSocketServerRecord *self = client->server;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("ondisconnecting")) {
		xsTry {
			KPR_WebSocketClient_buildCloseSlots(the, code, reason, wasClean);

			(void)xsCallFunction2(xsResult, self->slot, client->slot, xsVar(0));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();
}

static void KPR_WebSocketServer_onClientClose(KprWebSocketEndpoint endpoint UNUSED, UInt16 code, char *reason, Boolean wasClean, void *refcon)
{
	KPR_WebSocketClientRecord *client = refcon;
	KPR_WebSocketServerRecord *self = client->server;

	FskListRemove((FskList *)&self->clients, client);

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);

	if (xsFindResult(client->slot, xsID("onclose"))) {
		xsTry {
			KPR_WebSocketClient_buildCloseSlots(the, code, reason, wasClean);

			(void)xsCallFunction1(xsResult, client->slot, xsVar(0));
		}
		xsCatch {
		}
	}

	if (hasCallback("ondisconnect")) {
		xsTry {
			KPR_WebSocketClient_buildCloseSlots(the, code, reason, wasClean);

			(void)xsCallFunction2(xsResult, self->slot, client->slot, xsVar(0));
		}
		xsCatch {
		}
	}

	xsForget(client->slot);

	xsEndHostSandboxCode();
}

//--------------------------------------------------
// WebSocket Script Patch
//--------------------------------------------------

void KPR_WebSocket_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("WebSocket"));
	xsNewHostProperty(xsResult, xsID("CONNECTING"), xsInteger(kKprWebSocketStateConnecting), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("OPEN"), xsInteger(kKprWebSocketStateOpen), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("CLOSING"), xsInteger(kKprWebSocketStateClosing), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("CLOSED"), xsInteger(kKprWebSocketStateClosed), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
}

//--------------------------------------------------
// Service
//--------------------------------------------------

static void KprWebSocketServiceStart(KprService self, FskThread thread, xsMachine* the);
static void KprWebSocketServiceStop(KprService self);
static void KprWebSocketServiceCancel(KprService self, KprMessage message);
static void KprWebSocketServiceInvoke(KprService self, KprMessage message);

KprServiceRecord gWebSocketService = {
	NULL,
	kprServicesThread,
	"ws:",
	NULL,
	NULL,
	KprServiceAccept,
	KprWebSocketServiceCancel,
	KprWebSocketServiceInvoke,
	KprWebSocketServiceStart,
	KprWebSocketServiceStop,
	NULL,
	NULL,
	NULL
};

FskExport(FskErr) kprWebSocket_fskLoad(FskLibrary library)
{
	KprServiceRegister(&gWebSocketService);
	return kFskErrNone;
}

FskExport(FskErr) kprWebSocket_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

void KprWebSocketServiceCancel(KprService service UNUSED, KprMessage message UNUSED)
{
}

void KprWebSocketServiceInvoke(KprService service UNUSED, KprMessage message)
{
	FskErr err = kFskErrNone;
	if (KprMessageContinue(message)) {
		if (err) {
			message->error = err;
			FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
		}
	}
}

void KprWebSocketServiceStart(KprService self, FskThread thread, xsMachine* the)
{
	self->machine = the;
	self->thread = thread;
}

void KprWebSocketServiceStop(KprService self UNUSED)
{
}


