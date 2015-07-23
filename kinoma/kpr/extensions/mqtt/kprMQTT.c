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
#include "kprMQTTClient.h"
#include "kprMQTTBroker.h"
#include "kprMQTT.h"

#define kKprMQTTClientIdentifierMaxLength 23
#define kKprMQTTClientIdentifierPrefix "KprMQTTClient-"
#define isObject(x) xsIsInstanceOf(x, xsObjectPrototype)
#define isString(x) xsIsInstanceOf(x, xsStringPrototype)
#define isChunk(x) xsIsInstanceOf(x, xsChunkPrototype)
#define isFunc(x) xsIsInstanceOf(x, xsFunctionPrototype)
#define hasCallback(x) xsFindResult(self->slot, xsID(x)) && isFunc(xsResult)

//--------------------------------------------------
// MQTT Client Script Interface
//--------------------------------------------------

struct KPR_MQTTClientRecord {
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	KprMQTTClient client;
};

typedef struct KPR_MQTTClientRecord KPR_MQTTClientRecord;

static void KPR_mqttclient_onConnect(KprMQTTClient client, UInt8 returnCode, void *refcon);
static void KPR_mqttclient_onSubscribe(KprMQTTClient client, char *topic, UInt8 qos, void *refcon);
static void KPR_mqttclient_onUnsubscribe(KprMQTTClient client, char *topic, void *refcon);
static void KPR_mqttclient_onPublish(KprMQTTClient client, UInt16 token, void *refcon);
static void KPR_mqttclient_onMessage(KprMQTTClient client, char *topic, KprMemoryBuffer payload, void *refcon);
static void KPR_mqttclient_onDisconnect(KprMQTTClient client, Boolean cleanClose, void *refcon);
static void KPR_mqttclient_onError(KprMQTTClient client, FskErr err, char *reason, void *refcon);

static void KPR_mqttmessage_newFromBuffer(xsMachine *the, xsSlot *object, KprMemoryBuffer buffer);

void KPR_MQTTClient(xsMachine* the)
{
	FskErr err;
	KPR_MQTTClientRecord *self = NULL;
	xsIntegerValue c = xsToInteger(xsArgc);
	KprMQTTClient client = NULL;
	char clientIdentifier[kKprMQTTClientIdentifierMaxLength + 1];
	Boolean cleanSession = true;

	if (c >= 1) {
		char *arg = xsToString(xsArg(0));
		UInt32 len = FskStrLen(arg);

		if (len < 1 || len > kKprMQTTClientIdentifierMaxLength) xsThrowIfFskErr(kFskErrBadData);
		FskStrCopy(clientIdentifier, arg);
	} else {
		char hex[9];
		FskStrNumToHex(FskRandom(), hex, 8);
		FskStrCopy(clientIdentifier, kKprMQTTClientIdentifierPrefix);
		FskStrCat(clientIdentifier, hex);

	}

	if (c >= 2) {
		cleanSession = xsToBoolean(xsArg(1));
	}

	bailIfError(FskMemPtrNewClear(sizeof(KPR_MQTTClientRecord), &self));

	bailIfError(KprMQTTClientNew(&client, clientIdentifier, cleanSession, self));

	client->connectCallback = KPR_mqttclient_onConnect;
	client->subscribeCallback = KPR_mqttclient_onSubscribe;
	client->unsubscribeCallback = KPR_mqttclient_onUnsubscribe;
	client->publishCallback = KPR_mqttclient_onPublish;
	client->messageCallback = KPR_mqttclient_onMessage;
	client->disconnectCallback = KPR_mqttclient_onDisconnect;
	client->errorCallback = KPR_mqttclient_onError;

	self->client = client;
	self->the = the;
	self->slot = xsThis;
	self->code = the->code;
	xsSetHostData(self->slot, self);
	// xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, self->slot);

	xsRemember(self->slot);

bail:
	if (err) {
		KprMQTTClientDispose(client);
		FskMemPtrDispose(self);
		xsThrowIfFskErr(err);
	}
}

void KPR_mqttclient_destructor(void *it)
{
	KPR_MQTTClientRecord *self = it;

	if (self) {
		INVOKE_AFTER1(KprMQTTClientDispose, self->client);

		FskMemPtrDispose(self);
	}
}

void KPR_mqttclient_connect(xsMachine* the)
{
	KPR_MQTTClientRecord *self = xsGetHostData(xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	char *host;
	UInt16 port;
	KprMQTTClientConnectOptions options;

	if (c >= 1) {
		host = xsToString(xsArg(0));
	} else {
		host = "localhost";
	}

	if (c >= 2) {
		port = xsToInteger(xsArg(1));
	} else {
		port = 1883;
	}

	options.isSecure = (port == 1884);
	options.keepAlive = 60;
	options.password = NULL;
	options.username = NULL;
	options.willIsRetained = false;
	options.willQualityOfService = 0;
	options.willTopic = NULL;
	options.willPayload = NULL;
	options.willPayloadLength = 0;

	if (c >= 3) {
		xsVars(1);
		xsEnterSandbox();
		{
			if (xsHas(xsArg(2), xsID("secure"))) options.isSecure = xsToBoolean(the->scratch);
			if (xsHas(xsArg(2), xsID("keepAlive"))) options.keepAlive = xsToInteger(the->scratch);
			if (xsHas(xsArg(2), xsID("username"))) options.username = xsToString(the->scratch);
			if (xsHas(xsArg(2), xsID("password"))) options.password = xsToString(the->scratch);

			if (xsHas(xsArg(2), xsID("will"))) {
				xsVar(0) = the->scratch;

				if (xsHas(xsVar(0), xsID("topic"))) options.willTopic = xsToString(the->scratch);
				if (xsHas(xsVar(0), xsID("qos"))) options.willQualityOfService = xsToInteger(the->scratch);
				if (xsHas(xsVar(0), xsID("retain"))) options.willIsRetained = xsToBoolean(the->scratch);

				if (xsHas(xsVar(0), xsID("data"))) {
					xsVar(0) = the->scratch;

					if (isChunk(xsVar(0))) {
						options.willPayload = xsGetHostData(xsVar(0));
						options.willPayloadLength = xsToInteger(xsGet(xsVar(0), xsID_length));
					} else {
						options.willPayload = xsToString(xsVar(0));
						options.willPayloadLength = FskStrLen(options.willPayload);
					}
				}
			}
		}
		xsLeaveSandbox();
	}

	if (options.willQualityOfService > 2 || (options.willTopic && !KprMQTTIsValidTopic(options.willTopic, false))) {
		xsThrowIfFskErr(kFskErrBadData);
	}

	xsThrowIfFskErr(KprMQTTClientConnect(self->client, host, port, &options));
}

void KPR_mqttclient_reconnect(xsMachine* the)
{
	KPR_MQTTClientRecord *self = xsGetHostData(xsThis);

	xsThrowIfFskErr(KprMQTTClientReconnect(self->client));
}

void KPR_mqttclient_disconnect(xsMachine* the)
{
	KPR_MQTTClientRecord *self = xsGetHostData(xsThis);

	xsThrowIfFskErr(KprMQTTClientDisconnect(self->client));
}

void KPR_mqttclient_publish(xsMachine* the)
{
	KPR_MQTTClientRecord *self = xsGetHostData(xsThis);
	FskErr err = kFskErrNone;
	xsIntegerValue c = xsToInteger(xsArgc);
	char *topic = NULL;
	void *payload = NULL;
	UInt32 payloadLength = 0;
	UInt8 qos = 0;
	Boolean retain = false, hasPayload = false, hasQos = false, hasRetain = false;
	UInt16 token;

	/*
	 Case 1.
		topic, string_or_chunk, qos, retain
	 Case 2
		topic, { payload: string_or_chunk}, qos, retain
	 Case 3
		topic, { payload: string_or_chunk, qos: 0, retain: true }
	 */
	if (c < 1) goto invalidParams;

	topic = xsToString(xsArg(0));

	if (c >= 2) {
		if (isChunk(xsArg(1))) {
			payload = xsGetHostData(xsArg(1));
			payloadLength = xsToInteger(xsGet(xsArg(1), xsID_length));
		} else if (isObject(xsArg(1))) {
			xsVars(1);

			xsEnterSandbox();
			{
				hasPayload = xsHas(xsArg(1), xsID("data"));
				if (hasPayload) xsVar(0) = the->scratch;

				hasQos = xsHas(xsArg(1), xsID("qos"));
				if (hasQos) qos = xsToInteger(the->scratch);

				hasRetain = xsHas(xsArg(1), xsID("retain"));
				if (hasRetain) retain = xsToInteger(the->scratch);
			}
			xsLeaveSandbox();

			if (hasPayload) {
				if (isChunk(xsVar(0))) {
					payload = xsGetHostData(xsVar(0));
					payloadLength = xsToInteger(xsGet(xsVar(0), xsID_length));
				} else {
					payload = xsToString(xsVar(0));
					payloadLength = FskStrLen(payload);
				}
			}
		} else {
			payload = xsToString(xsArg(1));
			payloadLength = FskStrLen(payload);
		}
	}

	if (c >= 3 && !hasQos) {
		qos = xsToInteger(xsArg(2));
	}

	if (c >= 4 && !hasRetain) {
		retain = xsToBoolean(xsArg(3));
	}

	if (!KprMQTTIsValidTopic(topic, false)) goto badParam;
	if (qos > 2) goto badParam;

	bailIfError(KprMQTTClientPublish(self->client, topic, payload, payloadLength, qos, retain, &token));
	xsResult = xsInteger(token);
	goto bail;

invalidParams:
	err = kFskErrInvalidParameter;
	goto bail;

badParam:
	err = kFskErrBadData;
	goto bail;

bail:
	xsThrowIfFskErr(err);
}

void KPR_mqttclient_subscribe(xsMachine* the)
{
	KPR_MQTTClientRecord *self = xsGetHostData(xsThis);
	FskErr err = kFskErrNone;
	xsIntegerValue c = xsToInteger(xsArgc);
	char *topic = NULL;
	UInt8 qos = 0;

	if (c < 1) goto invalidParams;

	topic = xsToString(xsArg(0));
	if (!KprMQTTIsValidTopic(topic, true)) goto badParam;

	if (c >= 2) {
		qos = xsToInteger(xsArg(1));
	}

	if (qos > 2) goto badParam;

	bailIfError(KprMQTTClientSubscribeTopic(self->client, topic, qos));
	goto bail;

invalidParams:
	err = kFskErrInvalidParameter;
	goto bail;

badParam:
	err = kFskErrBadData;
	goto bail;

bail:
	xsThrowIfFskErr(err);
}

void KPR_mqttclient_unsubscribe(xsMachine* the)
{
	KPR_MQTTClientRecord *self = xsGetHostData(xsThis);
	FskErr err = kFskErrNone;
	xsIntegerValue c = xsToInteger(xsArgc);
	char *topic = NULL;

	if (c < 1) goto invalidParams;

	topic = xsToString(xsArg(0));
	if (!KprMQTTIsValidTopic(topic, true)) goto badParam;

	bailIfError(KprMQTTClientUnsubscribeTopic(self->client, topic));
	goto bail;

invalidParams:
	err = kFskErrInvalidParameter;
	goto bail;

badParam:
	err = kFskErrBadData;
	goto bail;

bail:
	xsThrowIfFskErr(err);
}

static void KPR_mqttclient_deferredConnect(void *a, void *b, void *c UNUSED, void *d UNUSED)
{
	KPR_MQTTClientRecord *self = a;
	UInt8 returnCode = (UInt8) b;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onConnect")) {
		xsTry {
//			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);

			(void)xsCallFunction1(xsResult, self->slot, xsInteger(returnCode));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();
}

static void KPR_mqttclient_deferredSubscribe(void *a, void *b, void *c, void *d UNUSED)
{
	KPR_MQTTClientRecord *self = a;
	char *topic = (char *) b;
	UInt8 qos = (UInt32) c;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onSubscribe")) {
		xsTry {
			(void)xsCallFunction2(xsResult, self->slot, xsString(topic), xsInteger(qos));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();

	FskMemPtrDispose(topic);
}

static void KPR_mqttclient_deferredUnsubscribe(void *a, void *b, void *c UNUSED, void *d UNUSED)
{
	KPR_MQTTClientRecord *self = a;
	char *topic = (char *) b;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onUnsubscribe")) {
		xsTry {
			(void)xsCallFunction1(xsResult, self->slot, xsString(topic));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();

	FskMemPtrDispose(topic);
}

static void KPR_mqttclient_deferredPublish(void *a, void *b, void *c UNUSED, void *d UNUSED)
{
	KPR_MQTTClientRecord *self = a;
	UInt16 token = (char *) b;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onPublish")) {
		xsTry {
			(void)xsCallFunction1(xsResult, self->slot, xsInteger(token));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();
}

static void KPR_mqttclient_deferredMessage(void *a, void *b, void *c, void *d UNUSED)
{
	KPR_MQTTClientRecord *self = a;
	char *topic = (char *) b;
	KprMemoryBuffer payload = (KprMemoryBuffer) c;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onMessage")) {
		xsTry {
			KPR_mqttmessage_newFromBuffer(the, &xsVar(0), payload);

			(void)xsCallFunction2(xsResult, self->slot, xsString(topic), xsVar(0));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();

	FskMemPtrDispose(topic);
}

static void KPR_mqttclient_deferredDisconnect(void *a, void *b, void *c UNUSED, void *d UNUSED)
{
	KPR_MQTTClientRecord *self = a;
	Boolean cleanClose = (Boolean) b;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onDisconnect")) {
		xsTry {
			(void)xsCallFunction1(xsResult, self->slot, xsBoolean(cleanClose));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();
}


static void KPR_mqttclient_deferredError(void *a, void *b, void *c, void *d UNUSED)
{
	KPR_MQTTClientRecord *self = a;
	FskErr err = (FskErr) b;
	char *reason = (char *) c;

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(1);
	if (hasCallback("onError")) {
		xsTry {
			(void)xsCallFunction2(xsResult, self->slot, xsInteger(err), xsString(reason));
		}
		xsCatch {
		}
	}
	xsEndHostSandboxCode();

	FskMemPtrDispose(reason);
}

#define DEFER3(xxx, a, b, c) FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)xxx, self, (void *)(a), (void *)(b), (void *)(c))
#define DEFER2(xxx, a, b) DEFER3(xxx, a, b, NULL)
#define DEFER1(xxx, a) DEFER3(xxx, a, NULL, NULL)

static void KPR_mqttclient_onConnect(KprMQTTClient client UNUSED, UInt8 returnCode, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;
	DEFER1(KPR_mqttclient_deferredConnect, returnCode);
}

static void KPR_mqttclient_onSubscribe(KprMQTTClient client UNUSED, char *topic, UInt8 qos, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;
	char *topic2 = FskStrDoCopy(topic);
	if (topic2) {
		DEFER2(KPR_mqttclient_deferredSubscribe, topic2, qos);
	}
}

static void KPR_mqttclient_onUnsubscribe(KprMQTTClient client UNUSED, char *topic, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;
	char *topic2 = FskStrDoCopy(topic);
	if (topic2) {
		DEFER1(KPR_mqttclient_deferredUnsubscribe, topic2);
	}
}

static void KPR_mqttclient_onPublish(KprMQTTClient client UNUSED, UInt16 token, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;
	DEFER1(KPR_mqttclient_deferredPublish, token);
}

static void KPR_mqttclient_onMessage(KprMQTTClient client UNUSED, char *topic, KprMemoryBuffer payload, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;
	FskErr err;
	char *topic2 = NULL;
	KprMemoryBuffer payload2 = NULL;

	topic2 = FskStrDoCopy(topic);
	bailIfNULL(topic2);

	bailIfError(KprMemoryBufferDuplicate(payload, &payload2))
	DEFER2(KPR_mqttclient_deferredMessage, topic2, payload2);

	topic2 = NULL;

bail:
	FskMemPtrDispose(topic2);
}

static void KPR_mqttclient_onDisconnect(KprMQTTClient client UNUSED, Boolean cleanClose, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;
	DEFER1(KPR_mqttclient_deferredDisconnect, cleanClose);
}

static void KPR_mqttclient_onError(KprMQTTClient client UNUSED, FskErr err, char *reason, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;
	DEFER2(KPR_mqttclient_deferredError, err, FskStrDoCopy(reason));
}

//--------------------------------------------------
// MQTT Message Script Interface
//--------------------------------------------------

static void KPR_mqttmessage_newFromBuffer(xsMachine *the, xsSlot *object, KprMemoryBuffer buffer)
{
	*object = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("MQTT")), xsID("_message")));
	xsSetHostData(*object, buffer);
}

void KPR_mqttmessage_destructor(void *it)
{
	KprMemoryBuffer self = it;
	if (self) {
		KprMemoryBufferDispose(self);
	}
}

void KPR_mqttmessage_get_length(xsMachine* the)
{
	KprMemoryBuffer self = xsGetHostData(xsThis);

	if (self) {
		xsResult = xsInteger(self->size);
	} else {
		xsThrowIfFskErr(kFskErrNoData);
	}
}

void KPR_mqttmessage_get_data(xsMachine* the)
{
	KprMemoryBuffer self = xsGetHostData(xsThis);

	if (self) {
		xsResult = xsString(self->buffer);
	} else {
		xsThrowIfFskErr(kFskErrNoData);
	}
}

void KPR_mqttmessage_get_binaryData(xsMachine* the)
{
	KprMemoryBuffer self = xsGetHostData(xsThis);

	if (self) {
		FskMemPtr buffer;
		xsThrowIfFskErr(KprMemoryBufferCopyBuffer(self, &buffer));
		xsMemPtrToChunk(the, &xsResult, buffer, self->size, false);
	} else {
		xsThrowIfFskErr(kFskErrNoData);
	}
}

//--------------------------------------------------
// MQTT Broker Script Interface
//--------------------------------------------------

struct KPR_MQTTBrokerRecord {
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	KprMQTTBroker broker;
};

typedef struct KPR_MQTTBrokerRecord KPR_MQTTBrokerRecord;

void KPR_MQTTBroker(xsMachine* the)
{
	FskErr err;
	KPR_MQTTBrokerRecord *self = NULL;
	xsIntegerValue c = xsToInteger(xsArgc);
	KprMQTTBroker broker = NULL;
	UInt16 port = 1883;

	if (c >= 1) {
		port = xsToInteger(xsArg(0));
	}

	if (!port) xsThrowIfFskErr(kFskErrBadData);

	bailIfError(FskMemPtrNewClear(sizeof(KPR_MQTTBrokerRecord), &self));

	bailIfError(KprMQTTBrokerNew(&broker, self));

//	broker->connectCallback = KPR_mqttclient_onConnect;
//	broker->errorCallback = KPR_mqttclient_onError;

	self->broker = broker;
	self->the = the;
	self->slot = xsThis;
	self->code = the->code;
	xsSetHostData(self->slot, self);

	xsRemember(self->slot);

	xsThrowIfFskErr(KprMQTTBrokerListen(self->broker, port, NULL));

bail:
	if (err) {
		KprMQTTBrokerDispose(broker);
		FskMemPtrDispose(self);
		xsThrowIfFskErr(err);
	}
}

void KPR_mqttbroker_destructor(void *it)
{
	KPR_MQTTBrokerRecord *self = it;

	if (self) {
		INVOKE_AFTER1(KprMQTTBrokerDispose, self->broker);

		FskMemPtrDispose(self);
	}
}

void KPR_mqttbroker_get_clients(xsMachine* the)
{

}

void KPR_mqttbroker_disconnect(xsMachine* the)
{

}

//--------------------------------------------------
// MQTT Function Test
//--------------------------------------------------

#if defined(RUN_UNITTEST) && RUN_UNITTEST

//#include "kunit.h"
//
//ku_main();
//ku_test(MQTT_common);
//ku_test(MQTT_broker);
//
//void KPR_MQTT_test()
//{
//	ku_begin();
//	ku_run(MQTT_common);
//	ku_run(MQTT_broker);
//	ku_finish();
//}

#endif

//--------------------------------------------------
// MQTT Script Patch
//--------------------------------------------------

void KPR_MQTT_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("MQTT"));
	xsNewHostProperty(xsResult, xsID("DISCONNECTED"), xsInteger(kKprMQTTStateDisconnected), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("CONNECTING"), xsInteger(kKprMQTTStateConnecting), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("HANDSHAKING"), xsInteger(kKprMQTTStateHandshaking), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("ESTABLISHED"), xsInteger(kKprMQTTStateEstablished), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
}

//--------------------------------------------------
// Service
//--------------------------------------------------

static void KprMQTTServiceStart(KprService self, FskThread thread, xsMachine* the);
static void KprMQTTServiceStop(KprService self);
static void KprMQTTServiceCancel(KprService self, KprMessage message);
static void KprMQTTServiceInvoke(KprService self, KprMessage message);

KprServiceRecord gMQTTService = {
	NULL,
	kprServicesThread,
	"ws:",
	NULL,
	NULL,
	KprServiceAccept,
	KprMQTTServiceCancel,
	KprMQTTServiceInvoke,
	KprMQTTServiceStart,
	KprMQTTServiceStop,
	NULL,
	NULL,
	NULL
};

FskExport(FskErr) kprMQTT_fskLoad(FskLibrary library UNUSED)
{
	KprServiceRegister(&gMQTTService);
#if defined(RUN_UNITTEST) && RUN_UNITTEST
//	KPR_MQTT_test();
#endif
	return kFskErrNone;
}

FskExport(FskErr) kprMQTT_fskUnload(FskLibrary library UNUSED)
{
	return kFskErrNone;
}

void KprMQTTServiceCancel(KprService service UNUSED, KprMessage message UNUSED)
{
}

void KprMQTTServiceInvoke(KprService service UNUSED, KprMessage message)
{
	FskErr err = kFskErrNone;
	if (KprMessageContinue(message)) {
		if (err) {
			message->error = err;
			FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
		}
	}
}

void KprMQTTServiceStart(KprService self, FskThread thread, xsMachine* the)
{
	self->machine = the;
	self->thread = thread;
}

void KprMQTTServiceStop(KprService self UNUSED)
{
}


