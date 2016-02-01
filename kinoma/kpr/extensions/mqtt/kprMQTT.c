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

#define isObject(x) xsIsInstanceOf(x, xsObjectPrototype)
#define isString(x) xsIsInstanceOf(x, xsStringPrototype)
#define isFunc(x) xsIsInstanceOf(x, xsFunctionPrototype)
#define isArrayBuffer(x) xsIsInstanceOf(x, xsArrayBufferPrototype)
#define hasCallback(x) xsFindResult(self->slot, xsID(x)) && isFunc(xsResult)

//--------------------------------------------------
// MQTT Client Script Interface
//--------------------------------------------------

struct KPR_MQTTClientRecord {
	xsMachine* the;
	xsSlot slot;
	KprMQTTClient client;
	Boolean pending;
};

typedef struct KPR_MQTTClientRecord KPR_MQTTClientRecord;

static void KPR_mqttclient_onConnect(KprMQTTClient client, UInt8 returnCode, Boolean sessionPresent, void *refcon);
static void KPR_mqttclient_onSubscribe(KprMQTTClient client, UInt16 token, KprMQTTSubscribeTopic request, KprMQTTSubscribeTopic result, void *refcon);
static void KPR_mqttclient_onUnsubscribe(KprMQTTClient client, UInt16 token, KprMQTTSubscribeTopic request, void *refcon);
static void KPR_mqttclient_onPublish(KprMQTTClient client, UInt16 token, void *refcon);
static void KPR_mqttclient_onMessage(KprMQTTClient client, char *topic, KprMemoryBuffer payload, void *refcon);
static void KPR_mqttclient_onDisconnect(KprMQTTClient client, Boolean cleanClose, void *refcon);
static void KPR_mqttclient_onError(KprMQTTClient client, FskErr err, char *reason, void *refcon);

static void KPR_mqttmessage_newFromBuffer(xsMachine *the, xsSlot *object, KprMemoryBuffer buffer);

void KPR_MQTTClient(xsMachine* the)
{
	FskErr err;
	KPR_MQTTClientRecord *self = NULL;
	KprMQTTClient client = NULL;
	char *clientIdentifier;
	Boolean cleanSession;

	if (xsToInteger(xsArgc) != 2) xsThrowIfFskErr(kFskErrParameterError);

	clientIdentifier = xsToString(xsArg(0));
	cleanSession = xsToBoolean(xsArg(1));

	bailIfError(FskMemPtrNewClear(sizeof(KPR_MQTTClientRecord), &self));
	bailIfError(KprMQTTClientNew(&client, clientIdentifier, cleanSession, kKprMQTTProtocol311, self));

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
	xsSetHostData(self->slot, self);

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
		self->client->connectCallback = NULL;
		self->client->subscribeCallback = NULL;
		self->client->unsubscribeCallback = NULL;
		self->client->publishCallback = NULL;
		self->client->messageCallback = NULL;
		self->client->disconnectCallback = NULL;
		self->client->errorCallback = NULL;

		INVOKE_AFTER1(KprMQTTClientDispose, self->client);

		FskMemPtrDispose(self);
	}
}

void KPR_mqttclient_connect(xsMachine* the)
{
	KPR_MQTTClientRecord *self = xsGetHostData(xsThis);
	char *host;
	UInt16 port;
	KprMQTTClientConnectOptions options;

	if (xsToInteger(xsArgc) != 10) xsThrowIfFskErr(kFskErrParameterError);

	host = xsToString(xsArg(0));
	port = xsToInteger(xsArg(1));

	options.isSecure = xsToBoolean(xsArg(2));
	options.keepAlive = xsToInteger(xsArg(3));
	options.username = xsTest(xsArg(4)) ? xsToString(xsArg(4)) : NULL;
	options.password = xsTest(xsArg(5)) ? xsToString(xsArg(5)) : NULL;
	options.willIsRetained = false;
	options.willQualityOfService = 0;
	options.willTopic = NULL;
	options.willPayload = NULL;
	options.willPayloadLength = 0;

	if (xsTest(xsArg(6))) {
		options.willTopic = xsToString(xsArg(6));
		options.willQualityOfService = xsToInteger(xsArg(7));
		options.willIsRetained = xsToBoolean(xsArg(8));

		if (xsTest(xsArg(9))) {
			if (isArrayBuffer(xsArg(9))) {
				options.willPayload = xsToArrayBuffer(xsArg(9));
				options.willPayloadLength = xsGetArrayBufferLength(xsArg(9));
			} else {
				options.willPayload = xsToString(xsArg(9));
				options.willPayloadLength = FskStrLen(options.willPayload);
			}
		}
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
	char *topic;
	void *payload = NULL;
	UInt32 payloadLength = 0;
	UInt8 qos;
	Boolean retain;
	UInt16 token;

	if (xsToInteger(xsArgc) != 4) xsThrowIfFskErr(kFskErrParameterError);

	topic = xsToString(xsArg(0));

	if (xsTest(xsArg(1))) {
		if (isArrayBuffer(xsArg(1))) {
			payload = xsToArrayBuffer(xsArg(1));
			payloadLength = xsGetArrayBufferLength(xsArg(1));
		} else {
			payload = xsToString(xsArg(1));
			payloadLength = FskStrLen(payload);
		}
	}

	qos = xsToInteger(xsArg(2));
	retain = xsToBoolean(xsArg(3));

	bailIfError(KprMQTTClientPublish(self->client, topic, payload, payloadLength, qos, retain, &token));
	xsResult = xsInteger(token);

bail:
	xsThrowIfFskErr(err);
}

void KPR_mqttclient_subscribe(xsMachine* the)
{
	KPR_MQTTClientRecord *self = xsGetHostData(xsThis);
	FskErr err = kFskErrNone;
	xsIntegerValue c = xsToInteger(xsArgc);
	char **topics = NULL;
	UInt8 *qoss = NULL;
	int i, count;
	UInt16 token;

	if (c == 0 || (c % 2) != 0) xsThrowIfFskErr(kFskErrParameterError);

	count = c / 2;

	bailIfError(FskMemPtrNew(sizeof(char*) * count, &topics));
	bailIfError(FskMemPtrNew(sizeof(UInt8*) * count, &qoss));

	for (i = 0; i < count; i++) {
		topics[i] = xsToString(xsArg(i * 2));
		qoss[i] = xsToInteger(xsArg(i * 2 + 1));
	}

	bailIfError(KprMQTTClientSubscribeTopics(self->client, topics, qoss, count, &token));
	xsResult = xsInteger(token);

bail:
	FskMemPtrDispose(topics);
	FskMemPtrDispose(qoss);
	xsThrowIfFskErr(err);
}

void KPR_mqttclient_unsubscribe(xsMachine* the)
{
	KPR_MQTTClientRecord *self = xsGetHostData(xsThis);
	FskErr err = kFskErrNone;
	xsIntegerValue c = xsToInteger(xsArgc);
	char **topics = NULL;
	int i, count;
	UInt16 token;

	if (c == 0) xsThrowIfFskErr(kFskErrParameterError);

	count = c;

	bailIfError(FskMemPtrNew(sizeof(char*) * count, &topics));

	for (i = 0; i < count; i++) {
		topics[i] = xsToString(xsArg(i));
	}

	bailIfError(KprMQTTClientUnsubscribeTopics(self->client, topics, count, &token));
	xsResult = xsInteger(token);

bail:
	FskMemPtrDispose(topics);
	xsThrowIfFskErr(err);
}

void KPR_mqttclient_get_protocolVersion(xsMachine *the)
{
	KPR_MQTTClientRecord *self = xsGetHostData(xsThis);
	xsResult = xsString((char *) KprMQTTClientGetProtocolVersion(self->client));
}

#define DEFER3(xxx, a, b, c) FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)xxx, self, (void *)(a), (void *)(b), (void *)(c))
#define DEFER2(xxx, a, b) DEFER3(xxx, a, b, NULL)
#define DEFER1(xxx, a) DEFER3(xxx, a, NULL, NULL)
#define DEFER0(xxx) DEFER3(xxx, NULL, NULL, NULL)

static void KPR_mqttclient_deferredReleaseCallbacks(void *a, void *b UNUSED, void *c UNUSED, void *d UNUSED)
{
	KPR_MQTTClientRecord *self = a;

	xsBeginHost(self->the);
	xsThis = self->slot;

	if (self->pending) {
		self->pending = false;

		if (xsToBoolean(xsCall0(xsThis, xsID("_callbackPending")))) {
			xsCall0(xsThis, xsID("_releaseCallbacks"));
		}
	}
	xsForget(self->slot);
	xsEndHost();
}

static void KPR_mqttclient_releaseCallbacksIfPending(xsMachine *the)
{
	KPR_MQTTClientRecord *self = xsGetHostData(xsThis);
	if (!self->pending && xsToBoolean(xsCall0(xsThis, xsID("_callbackPending")))) {
		self->pending = true;

		DEFER0(KPR_mqttclient_deferredReleaseCallbacks);
		xsBeginHost(self->the);
		xsRemember(self->slot);
		xsEndHost();
	}
}

static void KPR_mqttclient_onConnect(KprMQTTClient client UNUSED, UInt8 returnCode, Boolean sessionPresent, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;
	xsBeginHost(self->the);
	xsThis = self->slot;
	if (hasCallback("_onConnect")) {
		xsTry {
			(void)xsCallFunction2(xsResult, xsThis, xsInteger(returnCode), xsBoolean(sessionPresent));
			KPR_mqttclient_releaseCallbacksIfPending(the);
		}
		xsCatch {
		}
	}
	xsEndHost();
}

static void KPR_mqttclient_onSubscribe(KprMQTTClient client UNUSED, UInt16 token, KprMQTTSubscribeTopic request, KprMQTTSubscribeTopic result, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;
	KprMQTTSubscribeTopic st = result;

	xsBeginHost(self->the);
	xsThis = self->slot;
	xsVars(1);
	if (hasCallback("_onSubscribe")) {
		xsTry {
			xsVar(0) = xsNew0(xsGlobal, xsID_Array);

			while (st) {
				(void)xsCall1(xsVar(0), xsID_push, xsInteger(st->qualityOfService));
				st = st->next;
			}

			(void)xsCallFunction2(xsResult, xsThis, xsInteger(token), xsVar(0));
			KPR_mqttclient_releaseCallbacksIfPending(the);
		}
		xsCatch {
		}
	}
	xsEndHost();
}

static void KPR_mqttclient_onUnsubscribe(KprMQTTClient client UNUSED, UInt16 token, KprMQTTSubscribeTopic request, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;

	xsBeginHost(self->the);
	xsThis = self->slot;
	if (hasCallback("_onUnsubscribe")) {
		xsTry {
			(void)xsCallFunction1(xsResult, xsThis, xsInteger(token));
			KPR_mqttclient_releaseCallbacksIfPending(the);
		}
		xsCatch {
		}
	}
	xsEndHost();
}

static void KPR_mqttclient_onPublish(KprMQTTClient client UNUSED, UInt16 token, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;

	xsBeginHost(self->the);
	xsThis = self->slot;
	if (hasCallback("_onPublish")) {
		xsTry {
			(void)xsCallFunction1(xsResult, xsThis, xsInteger(token));
			KPR_mqttclient_releaseCallbacksIfPending(the);
		}
		xsCatch {
		}
	}
	xsEndHost();
}

static void KPR_mqttclient_onMessage(KprMQTTClient client UNUSED, char *topic, KprMemoryBuffer payload, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;

	xsBeginHost(self->the);
	xsThis = self->slot;
	xsVars(1);
	if (hasCallback("_onMessage")) {
		xsTry {
			KPR_mqttmessage_newFromBuffer(the, &xsVar(0), payload);

			(void)xsCallFunction2(xsResult, xsThis, xsString(topic), xsVar(0));
			KPR_mqttclient_releaseCallbacksIfPending(the);
		}
		xsCatch {
		}
	}
	xsEndHost();
}

static void KPR_mqttclient_onDisconnectDeferred(void *a, void *b, void *c UNUSED, void *d UNUSED)
{
	KPR_MQTTClientRecord *self = a;
	Boolean cleanClose = b;

	xsBeginHost(self->the);
	xsThis = self->slot;
	if (hasCallback("_onDisconnect")) {
		xsTry {
			(void)xsCallFunction1(xsResult, xsThis, xsBoolean(cleanClose));
		}
		xsCatch {
		}
	}
	xsForget(self->slot);
	xsEndHost();
}

static void KPR_mqttclient_onDisconnect(KprMQTTClient client UNUSED, Boolean cleanClose, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;

	DEFER1(KPR_mqttclient_onDisconnectDeferred, (int) cleanClose);

	xsBeginHost(self->the);
	xsRemember(self->slot);
	xsEndHost();
}

static void KPR_mqttclient_onError(KprMQTTClient client UNUSED, FskErr err, char *reason, void *refcon)
{
	KPR_MQTTClientRecord *self = refcon;

	xsBeginHost(self->the);
	xsThis = self->slot;
	if (hasCallback("_onError")) {
		xsTry {
			(void)xsCallFunction2(xsResult, xsThis, xsInteger(err), xsString(reason));
			KPR_mqttclient_releaseCallbacksIfPending(the);
		}
		xsCatch {
		}
	}
	xsEndHost();
}

//--------------------------------------------------
// MQTT Message Script Interface
//--------------------------------------------------

void KPR_mqttmessage_get_data(xsMachine* the)
{
	xsVars(2);
	xsVar(0) = xsGet(xsGlobal, xsID_String);
	xsVar(1) = xsGet(xsThis, xsID("buffer"));
	xsResult = xsCallFunction1(xsGet(xsVar(0), xsID_fromArrayBuffer), xsVar(0), xsVar(1));
}

void KPR_mqttmessage_get_binaryData(xsMachine* the)
{
	xsVars(1);
	xsVar(0) = xsGet(xsThis, xsID_length);
	xsResult = xsCall2(xsGet(xsThis, xsID("buffer")), xsID_slice, xsInteger(0), xsVar(0));
}

static void KPR_mqttmessage_newFromBuffer(xsMachine *the, xsSlot *object, KprMemoryBuffer buffer)
{
	*object = xsNewInstanceOf(xsObjectPrototype);

	xsSet(*object, xsID("buffer"), xsArrayBuffer(buffer->buffer, buffer->size + 1));

	xsNewHostProperty(*object, xsID("length"), xsInteger(buffer->size), xsDefault, xsDontScript);
	xsNewHostProperty(*object, xsID("data"), xsNewHostFunction(KPR_mqttmessage_get_data, 0), xsIsGetter, xsDontScript | xsIsGetter);
	xsNewHostProperty(*object, xsID("binaryData"), xsNewHostFunction(KPR_mqttmessage_get_binaryData, 0), xsIsGetter, xsDontScript | xsIsGetter);
}

//--------------------------------------------------
// MQTT Broker Script Interface
//--------------------------------------------------

struct KPR_MQTTBrokerRecord {
	xsMachine* the;
	xsSlot slot;
	KprMQTTBroker broker;
	int port;
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
	xsSetHostData(self->slot, self);

	self->port = port;
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

void KPR_mqttbroker_get_port(xsMachine *the)
{
	KPR_MQTTBrokerRecord *self = xsGetHostData(xsThis);

	xsResult = xsInteger(self->port);
}

void KPR_mqttbroker_get_clients(xsMachine* the)
{

}

void KPR_mqttbroker_disconnect(xsMachine* the)
{

}

//--------------------------------------------------
// MQTT Function
//--------------------------------------------------

void KPR_mqtt_isValidTopic(xsMachine *the)
{
	Boolean isTopic = xsToBoolean(xsArg(1));
	char *topic = xsToString(xsArg(0));

	xsResult = xsBoolean(KprMQTTIsValidTopic(topic, isTopic));
}

//--------------------------------------------------
// DLL
//--------------------------------------------------

FskExport(FskErr) kprMQTT_fskLoad(FskLibrary library UNUSED)
{
	return kFskErrNone;
}

FskExport(FskErr) kprMQTT_fskUnload(FskLibrary library UNUSED)
{
	return kFskErrNone;
}

