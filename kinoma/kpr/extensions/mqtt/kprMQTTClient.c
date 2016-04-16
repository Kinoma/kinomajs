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
#include "FskNetUtils.h"
#include "FskSSL.h"
#include "FskEndian.h"

#include "kpr.h"
#include "kprShell.h"
#include "kprMQTTClient.h"

//--------------------------------------------------
// MQTT Client
//--------------------------------------------------

#define kKprMQTTClientIdentifierPrefix "KprMQTTClient-"
#define CALLBACK(x) if (self->x) self->x

static FskErr KprMQTTClientOnConnect(FskSocket skt, void *refCon);
static FskErr KprMQTTClientForceClose(KprMQTTClient self);
static FskErr KprMQTTClientSendMessage(KprMQTTClient self, KprMQTTMessage *message);
static void KprMQTTClientSendMessageViaDelivery(KprMQTTQueue queue, KprMQTTMessage *message, void *refcon);

static FskErr KprMQTTClientCreateConnectMessage(KprMQTTClient self, KprMQTTClientConnectOptions *options, KprMQTTMessage *message);
static FskErr KprMQTTClientCreatePublishMessage(KprMQTTClient self, char *topic, void *payload, UInt32 payloadLength, UInt8 qos, Boolean retain, KprMQTTMessage *it);
static FskErr KprMQTTClientCreateSubscribeMessages(KprMQTTClient self, char **topic, UInt8 *qos, int count, KprMQTTMessage *it);
static FskErr KprMQTTClientCreateUnsubscribeMessages(KprMQTTClient self, char **topic, int count, KprMQTTMessage *it);
static void KprMQTTClientHandleMessage(KprMQTTEndpoint endpoint, KprMQTTMessage message, void *refcon);
static void KprMQTTClient_onEndpointError(KprMQTTEndpoint endpoint, FskErr err, char *reason, void *refcon);
static void KprMQTTClientHandleError(KprMQTTClient self, FskErr err, char *reason);

static void KprMQTTClientResetKeepAliveTimer(KprMQTTClient self);
static void KprMQTTClientIdle(FskTimeCallBack callback, const FskTime time, void *it);
static void KprMQTTClientCheckPingResponse(FskTimeCallBack callback, const FskTime time, void *it);

FskErr KprMQTTClientNew(KprMQTTClient* it, char *clientId, Boolean cleanSession, KprMQTTProtocolVersion protocolVersion, void *refcon)
{
	FskErr err = kFskErrNone;
	KprMQTTClient self = NULL;
	char generatedId[23];

	bailIfError(FskMemPtrNewClear(sizeof(KprMQTTClientRecord), &self));

	bailIfError(KprMQTTQueueNew(&self->queue, 15, KprMQTTClientSendMessageViaDelivery, self));

	FskTimeCallbackNew(&self->pingRequestCallaback);
	bailIfNULL(self->pingRequestCallaback);

	FskTimeCallbackNew(&self->pingResponseCallaback);
	bailIfNULL(self->pingResponseCallaback);

	if (protocolVersion == kKprMQTTProtocol31) {
		if (clientId == NULL || FskStrLen(clientId)) {
			char hex[9];

			FskStrCopy(generatedId, kKprMQTTClientIdentifierPrefix);

			FskStrNumToHex(FskRandom(), hex, 8);
			FskStrCat(generatedId, hex);

			clientId = generatedId;
		}
	} else {
		if (clientId == NULL) clientId = "";
	}

	self->clientIdentifier = FskStrDoCopy(clientId);
	bailIfNULL(self->clientIdentifier);

	self->cleanSession = FskStrLen(self->clientIdentifier) > 0 ? cleanSession : true;
	self->refcon = refcon;
	self->state = kKprMQTTStateDisconnected;
	self->protocolVersion = protocolVersion ? protocolVersion : kKprMQTTProtocol311;

	KprMQTTQueueStart(self->queue);

	*it = self;
bail:
	if (err) KprMQTTClientDispose(self);
	return err;
}

FskErr KprMQTTClientDispose(KprMQTTClient self)
{
	if (self) {
		KprMQTTClientForceClose(self);

		FskTimeCallbackDispose(self->pingRequestCallaback);
		FskTimeCallbackDispose(self->pingResponseCallaback);

		KprMQTTMessageDispose(self->connectMessage);
		self->connectMessage = NULL;

		KprMQTTQueueDispose(self->queue);

		FskMemPtrDispose(self->clientIdentifier);
		FskMemPtrDispose(self->host);

		FskMemPtrDispose(self);
	}
	return kFskErrNone;
}

// --- CONNECT & DISCONNECT -------------------------

FskErr KprMQTTClientConnect(KprMQTTClient self, char *host, UInt16 port, KprMQTTClientConnectOptions *options)
{
	FskErr err = kFskErrNone;

	if (self->state != kKprMQTTStateDisconnected) {
		return kFskErrBadState;
	}

	FskMemPtrDisposeAt(&self->host);
	self->host = FskStrDoCopy(host);
	bailIfNULL(self->host);
	self->port = port;
	self->cancelConnection = false;

	KprMQTTMessageDispose(self->connectMessage);
	self->connectMessage = NULL;
	bailIfError(KprMQTTClientCreateConnectMessage(self, options, &self->connectMessage));

	self->keepAlive = options->keepAlive;

	bailIfError(KprMQTTClientReconnect(self));

bail:
	return err;
}

FskErr KprMQTTClientReconnect(KprMQTTClient self)
{
	FskErr err = kFskErrNone;
	long flags = 0;

	if (self->state != kKprMQTTStateDisconnected || self->host == NULL) {
		return kFskErrBadState;
	}

	self->state = kKprMQTTStateConnecting;
	bailIfError(FskNetConnectToHost(self->host, self->port, false, KprMQTTClientOnConnect, self, flags, NULL, "MQTT"));

bail:
	if (err) KprMQTTClientForceClose(self);
	return err;
}

static FskErr KprMQTTClientOnConnect(FskSocket skt, void *refCon)
{
	FskErr err = kFskErrNone;
	KprMQTTClient self = refCon;

	if (!skt || 0 == skt->ipaddrRemote) {
		bailIfError(kFskErrSocketNotConnected);
	}

	if (self->cancelConnection) {
		FskNetSocketClose(skt);
		KprMQTTClientForceClose(self);
		return kFskErrNone;
	}

	bailIfError(KprMQTTEndpointNew(&self->endpoint, skt, self));

	self->endpoint->protocolVersion = self->protocolVersion;
	self->endpoint->messageCallback = KprMQTTClientHandleMessage;
	self->endpoint->errorCallback = KprMQTTClient_onEndpointError;

	self->state = kKprMQTTStateHandshaking;
	bailIfError(KprMQTTClientSendMessage(self, &self->connectMessage));

bail:
	if (err) {
		CALLBACK(errorCallback)(self, err, "connection failed", self->refcon);

		KprMQTTClientForceClose(self);
	}
	return err;
}

FskErr KprMQTTClientDisconnect(KprMQTTClient self)
{
	FskErr err;
	KprMQTTMessage message = NULL;

	if (self->state == kKprMQTTStateDisconnected) {
		return kFskErrBadState;
	}

	if (self->state == kKprMQTTStateConnecting) {
		self->cancelConnection = true;
		return kFskErrNone;
	}

	bailIfError(KprMQTTMessageNewWithType(&message, kKprMQTTMessageTypeDISCONNECT));
	bailIfError(KprMQTTClientSendMessage(self, &message));
	self->disconnectWasSent = true;

bail:
	KprMQTTMessageDispose(message);
	return KprMQTTClientForceClose(self);
}

static FskErr KprMQTTClientForceClose(KprMQTTClient self)
{
	if (self->state != kKprMQTTStateDisconnected) {
		Boolean wasEstablished = (self->state == kKprMQTTStateEstablished);

		KprMQTTEndpointDispose(self->endpoint);
		self->endpoint = NULL;

		KprMQTTQueueStop(self->queue);

		FskTimeCallbackRemove(self->pingRequestCallaback);
		FskTimeCallbackRemove(self->pingResponseCallaback);

		self->state = kKprMQTTStateDisconnected;

		if (wasEstablished) {
			CALLBACK(disconnectCallback)(self, self->disconnectWasSent, self->refcon);
		}
	}

	return kFskErrNone;
}

const char *KprMQTTClientGetProtocolVersion(KprMQTTClient self)
{
	switch (self->protocolVersion) {
		case kKprMQTTProtocol31:
			return "3.1";
		case kKprMQTTProtocol311:
			return "3.1.1";
		default:
			return "unknown";
	}
}

// --- PUBLISH --------------------------------------

FskErr KprMQTTClientPublish(KprMQTTClient self, char *topic, void *payload, UInt32 payloadLength, UInt8 qos, Boolean retain, UInt16 *token)
{
	FskErr err;
	KprMQTTMessage message = NULL;

	bailIfError(KprMQTTClientCreatePublishMessage(self, topic, payload, payloadLength, qos, retain, &message));
	if (token) *token = message->messageId;

	bailIfError(KprMQTTClientSendMessage(self, &message));

bail:
	KprMQTTMessageDispose(message);

	return err;
}

// --- SUBSCRIBE TOPIC ------------------------------

FskErr KprMQTTClientSubscribeTopic(KprMQTTClient self, char *topic, UInt8 qos, UInt16 *token)
{
	return KprMQTTClientSubscribeTopics(self, &topic, &qos, 1, token);
}

FskErr KprMQTTClientSubscribeTopics(KprMQTTClient self, char **topics, UInt8 *qoss, int count, UInt16 *token)
{
	FskErr err;
	KprMQTTMessage message = NULL;

	bailIfError(KprMQTTClientCreateSubscribeMessages(self, topics, qoss, count, &message));
	if (token) *token = message->messageId;

	bailIfError(KprMQTTClientSendMessage(self, &message));

bail:
	KprMQTTMessageDispose(message);

	return err;
}

FskErr KprMQTTClientUnsubscribeTopic(KprMQTTClient self, char *topic, UInt16 *token)
{
	return KprMQTTClientUnsubscribeTopics(self, &topic, 1, token);
}

FskErr KprMQTTClientUnsubscribeTopics(KprMQTTClient self, char **topics, int count, UInt16 *token)
{
	FskErr err;
	KprMQTTMessage message = NULL;

	bailIfError(KprMQTTClientCreateUnsubscribeMessages(self, topics, count, &message));
	if (token) *token = message->messageId;

	bailIfError(KprMQTTClientSendMessage(self, &message));

bail:
	KprMQTTMessageDispose(message);

	return err;
}

// --- SEND MESSAGE ---------------------------------

static Boolean KprMQTTClientOKtoSendMessage(KprMQTTClient self, KprMQTTMessage message)
{
	if (self->state == kKprMQTTStateHandshaking) {
		return (message->type == kKprMQTTMessageTypeCONNECT);
	}

	return (self->state == kKprMQTTStateEstablished);
}

static FskErr KprMQTTClientSendMessage(KprMQTTClient self, KprMQTTMessage *message)
{
	FskErr err = kFskErrNone;

	if (!KprMQTTClientOKtoSendMessage(self, *message)) bailIfError(kFskErrBadState);

	bailIfError(KprMQTTEndpointSendMessage(self->endpoint, *message));

	KprMQTTClientResetKeepAliveTimer(self);

	if ((*message)->qualityOfService > 0) {
		(*message)->duplicateDelivery = true;
		bailIfError(KprMQTTQueueOutboxPut(self->queue, message));
	}

bail:
	return err;
}

static void KprMQTTClientSendMessageViaDelivery(KprMQTTQueue queue, KprMQTTMessage *message, void *refcon)
{
	KprMQTTClient self = refcon;
	FskErr err = KprMQTTClientSendMessage(self, message);

	if (err) {
		CALLBACK(errorCallback)(self, err, "resend fail", self->refcon);
	}
}

static FskErr KprMQTTClientCopyString(const char *src, char **dest)
{
	if (src != NULL) {
		*dest = FskStrDoCopy(src);
		if (*dest == NULL) return kFskErrMemFull;
	}
	return kFskErrNone;
}

static FskErr KprMQTTClientCreateConnectMessage(KprMQTTClient self, KprMQTTClientConnectOptions *options, KprMQTTMessage *it)
{
	FskErr err;
	KprMQTTMessage message = NULL;
	struct KprMQTTConnectMessageRecord *p;

	bailIfError(KprMQTTMessageNewWithType(&message, kKprMQTTMessageTypeCONNECT));
	p = &message->t.connect;

	p->keepAlive = options->keepAlive;
	p->cleanSession = self->cleanSession;

	if (options->willTopic) {
		p->willQualityOfService = options->willQualityOfService;
		p->willIsRetained = options->willIsRetained;

		bailIfError(KprMQTTClientCopyString(options->willTopic, &p->willTopic));

		if (options->willPayload) {
			bailIfError(KprMemoryBufferNewFromData(options->willPayloadLength, options->willPayload, &p->willPayload));
		}
	}

	bailIfError(KprMQTTClientCopyString(self->clientIdentifier, &p->clientIdentifier));
	bailIfError(KprMQTTClientCopyString(options->username, &p->username));
	bailIfError(KprMQTTClientCopyString(options->password, &p->password));

	*it = message;
	message = NULL;

bail:
	KprMQTTMessageDispose(message);
	return err;
}

static FskErr KprMQTTClientCreatePublishMessage(KprMQTTClient self, char *topic, void *payload, UInt32 payloadLength, UInt8 qos, Boolean retain, KprMQTTMessage *it)
{
	FskErr err;
	KprMQTTMessage message = NULL;

	bailIfError(KprMQTTMessageNewWithType(&message, kKprMQTTMessageTypePUBLISH));

	message->qualityOfService = qos;
	message->isRetained = retain;

	if (qos > 0) {
		message->messageId = KprMQTTQueueNextId(self->queue);
	}

	message->t.publish.topic = FskStrDoCopy(topic);
	bailIfNULL(message->t.publish.topic);

	bailIfError(KprMemoryBufferNewFromData(payloadLength, payload, &message->t.publish.payload));

	*it = message;
	message = NULL;

bail:
	KprMQTTMessageDispose(message);
	return err;
}

static FskErr KprMQTTClientCreateSubscribeMessages(KprMQTTClient self, char **topic, UInt8 *qos, int count, KprMQTTMessage *it)
{
	FskErr err;
	KprMQTTMessage message = NULL;
	KprMQTTSubscribeTopic st;
	int i;

	bailIfError(KprMQTTMessageNewWithType(&message, kKprMQTTMessageTypeSUBSCRIBE));

	message->messageId = KprMQTTQueueNextId(self->queue);

	for (i = 0; i < count; i++) {
		bailIfError(KprMQTTMessageAddSubscribeTopic(message, &st));

		st->topic = FskStrDoCopy(*topic++);
		bailIfNULL(st->topic);
		st->qualityOfService = *qos++;
	}

	*it = message;
	message = NULL;

bail:
	KprMQTTMessageDispose(message);
	return err;
}

static FskErr KprMQTTClientCreateUnsubscribeMessages(KprMQTTClient self, char **topic, int count, KprMQTTMessage *it)
{
	FskErr err;
	KprMQTTMessage message = NULL;
	KprMQTTSubscribeTopic st;
	int i;

	bailIfError(KprMQTTMessageNewWithType(&message, kKprMQTTMessageTypeUNSUBSCRIBE));

	message->messageId = KprMQTTQueueNextId(self->queue);

	for (i = 0; i < count; i++) {
		bailIfError(KprMQTTMessageAddSubscribeTopic(message, &st));

		st->topic = FskStrDoCopy(*topic++);
		bailIfNULL(st->topic);
	}

	*it = message;
	message = NULL;

bail:
	KprMQTTMessageDispose(message);
	return err;
}

static void KprMQTTClientHandleMessage(KprMQTTEndpoint endpoint UNUSED, KprMQTTMessage message, void *refcon)
{
	KprMQTTClient self = refcon;
	FskErr err = kFskErrNone;
	KprMQTTMessage message2 = NULL, message3 = NULL;

	switch (message->type) {
		case kKprMQTTMessageTypeCONNACK: {
			UInt8 returnCode = message->t.connack.returnCode;
			Boolean sessionPersist = message->t.connack.sesstionPresent;

			if (returnCode == kKprMQTTMessageReturnCodeAccepted) {
				self->state = kKprMQTTStateEstablished;
			}
			CALLBACK(connectCallback)(self, returnCode, sessionPersist, self->refcon);
			if (returnCode != kKprMQTTMessageReturnCodeAccepted) {
				KprMQTTClientForceClose(self);
			}
			break;
		}

		case kKprMQTTMessageTypePUBACK:
		case kKprMQTTMessageTypePUBCOMP: {
			message2 = KprMQTTQueueOutboxGet(self->queue, message->messageId);
			if (!message2) break;

			CALLBACK(publishCallback)(self, message2->messageId, self->refcon);
			break;
		}

		case kKprMQTTMessageTypePUBREC: {
			message2 = KprMQTTQueueOutboxGet(self->queue, message->messageId);
			if (!message2) break;

			bailIfError(KprMQTTMessageNewWithType(&message3, kKprMQTTMessageTypePUBREL));
			message3->messageId = message2->messageId;

			bailIfError(KprMQTTClientSendMessage(self, &message3));
			break;
		}

		case kKprMQTTMessageTypeSUBACK: {
			message2 = KprMQTTQueueOutboxGet(self->queue, message->messageId);
			if (!message2) break;

			CALLBACK(subscribeCallback)(self, message->messageId, KprMQTTMessageFirstSubscribeTopic(message2), KprMQTTMessageFirstSubscribeTopic(message), self->refcon);
			break;
		}

		case kKprMQTTMessageTypeUNSUBACK: {
			message2 = KprMQTTQueueOutboxGet(self->queue, message->messageId);
			if (!message2) break;

			CALLBACK(unsubscribeCallback)(self, message->messageId, KprMQTTMessageFirstSubscribeTopic(message2), self->refcon);
			break;
		}

		case kKprMQTTMessageTypePINGRESP: {
			FskTimeCallbackRemove(self->pingResponseCallaback);
			break;
		}

		case kKprMQTTMessageTypePUBLISH: {
			struct KprMQTTPublishMessageRecord *p = &message->t.publish;

			switch (message->qualityOfService) {
				case 1:
					bailIfError(KprMQTTMessageNewWithType(&message2, kKprMQTTMessageTypePUBACK));
					message2->messageId = message->messageId;

					bailIfError(KprMQTTClientSendMessage(self, &message2));
					// fall thru

				case 0:
					CALLBACK(messageCallback)(self, p->topic, p->payload, self->refcon);
					break;

				case 2:
					bailIfError(KprMQTTMessageNewWithType(&message2, kKprMQTTMessageTypePUBREC));
					message2->messageId = message->messageId;

					bailIfError(KprMQTTClientSendMessage(self, &message2));
					bailIfError(KprMQTTQueueInboxPut(self->queue, &message));
					break;

				default:
					err = kFskErrBadData;
					break;

			}

			break;
		}

		case kKprMQTTMessageTypePUBREL: {
			struct KprMQTTPublishMessageRecord *p;

			message2 = KprMQTTQueueInboxGet(self->queue, message->messageId);
			if (!message2) break;

			bailIfError(KprMQTTMessageNewWithType(&message3, kKprMQTTMessageTypePUBCOMP));
			message3->messageId = message->messageId;

			bailIfError(KprMQTTClientSendMessage(self, &message3));

			p = &message2->t.publish;
			CALLBACK(messageCallback)(self, p->topic, p->payload, self->refcon);
			p->payload = NULL;
			break;
		}

		default:
			err = kFskErrBadState;
			break;
	}

bail:
	KprMQTTMessageDispose(message);
	KprMQTTMessageDispose(message2);
	KprMQTTMessageDispose(message3);

	if (err) {
		KprMQTTClientHandleError(self, err, "handling message");
	}
}

static void KprMQTTClient_onEndpointError(KprMQTTEndpoint endpoint UNUSED, FskErr err, char *reason, void *refcon)
{
	KprMQTTClient self = refcon;

	KprMQTTClientHandleError(self, err, reason);
}

static void KprMQTTClientHandleError(KprMQTTClient self, FskErr err, char *reason)
{
	CALLBACK(errorCallback)(self, err, reason, self->refcon);

	switch (err) {
		case kFskErrConnectionClosed:
			KprMQTTClientForceClose(self);
			break;

		default:
			break;
	}
}

// --- KEEP ALIVE -----------------------------------

static void KprMQTTClientResetKeepAliveTimer(KprMQTTClient self)
{
	FskTimeCallbackRemove(self->pingRequestCallaback);

	if (self->keepAlive > 0) {
		FskTimeRecord when;
		FskTimeGetNow(&when);
		FskTimeAddSecs(&when, self->keepAlive);
		FskTimeCallbackSet(self->pingRequestCallaback, &when, KprMQTTClientIdle, self);
	}

}

static void KprMQTTClientIdle(FskTimeCallBack callback UNUSED, const FskTime time, void *it)
{
	KprMQTTClient self = it;
	FskErr err;
	KprMQTTMessage message = NULL;
	FskTimeRecord when;

	bailIfError(KprMQTTMessageNewWithType(&message, kKprMQTTMessageTypePINGREQ));
	bailIfError(KprMQTTClientSendMessage(self, &message));

	when = *time;
	FskTimeAddSecs(&when, self->keepAlive);
	FskTimeCallbackSet(self->pingResponseCallaback, &when, KprMQTTClientCheckPingResponse, self);

bail:
	if (err) {
		KprMQTTClientHandleError(self, err, "sending ping");
	}

	KprMQTTMessageDispose(message);
}

static void KprMQTTClientCheckPingResponse(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *it)
{
	KprMQTTClient self = it;

	KprMQTTClientForceClose(self);
}

