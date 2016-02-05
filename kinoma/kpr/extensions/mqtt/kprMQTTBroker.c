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
#include "kprMQTTBroker.h"

#define kHeaderBufferSize 512
#define kMQTTBrokerIdentifier "Kinoma MQTT Server/1.0"

static FskErr KprMQTTBrokerAcceptNewConnection(KprSocketServer server, FskSocket skt, const char *interfaceName, int ip, void *refcon);

static KprMQTTSubscription KprMQTTBrokerFindSubscription(KprMQTTBroker self, const char *topicPattern);
static KprMQTTSubscriber KprMQTTBrokerFindSubscriber(KprMQTTSubscription subscription, const char *client);
static KprMQTTBrokerClient KprMQTTBrokerFindClient(KprMQTTBroker self, const char *identifier);

static FskErr KprMQTTBrokerRememberMessage(KprMQTTBroker self, const char *topic, KprMemoryBuffer payload, UInt8 qos);
static FskErr KprMQTTBrokerForgetMessage(KprMQTTBroker self, const char *topic);
static KprMQTTRetainedMessage KprMQTTBrokerRemindMessage(KprMQTTBroker self, const char *topicPattern, KprMQTTRetainedMessage after);

static FskErr KprMQTTBrokerClientNew(KprMQTTBrokerClient *it, KprMQTTBroker server);
static void KprMQTTBrokerClientDispose(KprMQTTBrokerClient client);

static FskErr KprMQTTBrokerClientOpen(KprMQTTBrokerClient self, FskSocket skt);
static FskErr KprMQTTBrokerClientReopen(KprMQTTBrokerClient self, KprMQTTEndpoint endpoint);
static void KprMQTTBrokerClientClose(KprMQTTBrokerClient self);

static void KprMQTTBrokerClientTakeOver(KprMQTTBrokerClient self, KprMQTTBrokerClient oldClient);
static void KprMQTTBrokerClientRequestClose(KprMQTTBrokerClient self, Boolean graceful);

static FskErr KprMQTTBrokerClientSendPublish(KprMQTTBrokerClient self, const char *topic, KprMemoryBuffer payload, UInt8 qos, Boolean retain);

static void KprMQTTBrokerHandleError(KprMQTTBroker self, FskErr err, char *reason);

#define CALLBACK(x) if (self->x) self->x

//--------------------------------------------------
// MQTTBroker
//--------------------------------------------------

FskErr KprMQTTBrokerNew(KprMQTTBroker* it, void *refCon)
{
	FskErr err = kFskErrNone;
	KprMQTTBroker self = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(KprMQTTBrokerRecord), &self));

	self->refCon = refCon;
	self->owner = FskThreadGetCurrent();

	*it = self;
	self = NULL;

bail:
	KprMQTTBrokerDispose(self);
	return err;
}

void KprMQTTBrokerDispose(KprMQTTBroker self)
{
	if (self) {
		KprMQTTSubscription subscription = self->subscriptions;
		while (subscription) {
			KprMQTTSubscription next = subscription->next;

			KprMQTTSubscriber subscriber = subscription->subscribers;
			while (subscriber) {
				KprMQTTSubscriber next = subscriber->next;

				FskMemPtrDispose(subscriber->client);
				FskMemPtrDispose(subscriber);
				subscriber = next;
			}

			FskMemPtrDispose(subscription->topic);
			FskMemPtrDispose(subscription);
			subscription = next;
		}

		while (self->retainedMessages) {
			KprMQTTBrokerForgetMessage(self, self->retainedMessages->topic);
		}

		while (self->clients) {
			KprMQTTBrokerClient client = self->clients;
			KprMQTTBrokerClientDispose(client);
		}

		KprSocketServerDispose(self->server);

		FskMemPtrDispose(self);
		FskDebugStr("BROKER: DISPOSE BROKER(0x%lx)", (UInt32)self);
	}
}

FskErr KprMQTTBrokerListen(KprMQTTBroker self, int port, char *interfaceName)
{
	FskErr err = kFskErrNone;
	KprSocketServer server = NULL;

	if (self->server) return kFskErrBadState;

	bailIfError(KprSocketServerNew(&server, self));
	server->debugName = "MQTT Server";
	server->acceptCallback = KprMQTTBrokerAcceptNewConnection;

	bailIfError(KprSocketServerListen(server, port, interfaceName));

	self->server = server;
	server = NULL;

	CALLBACK(launchCallback)(self, self->refCon);

bail:
	KprSocketServerDispose(server);

	if (err) {
		CALLBACK(errorCallback)(self, err, "failed to launch", self->refCon);
	}

	return err;
}

static void KprMQTTBrokerHandleError(KprMQTTBroker self, FskErr err, char *reason)
{
	CALLBACK(errorCallback)(self, err, reason, self->refCon);
}

static FskErr KprMQTTBrokerAcceptNewConnection(KprSocketServer server UNUSED, FskSocket skt, const char *interfaceName UNUSED, int ip UNUSED, void *refcon) {
	KprMQTTBroker self = refcon;
	FskErr err = kFskErrNone;
	KprMQTTBrokerClient client = NULL;

	bailIfError(KprMQTTBrokerClientNew(&client, self));
	bailIfError(KprMQTTBrokerClientOpen(client, skt));

bail:
	if (err) {
		FskNetSocketClose(skt);
	}

	return err;
}

static UInt8 KprMQTTBrokerAuthorize(KprMQTTBroker self, KprMQTTBrokerClient client, KprMQTTMessage message, Boolean *sesstionPresent)
{
	struct KprMQTTConnectMessageRecord *mc = &message->t.connect;
	KprMQTTBrokerClient oldClient;

	if (!mc->clientIdentifier || FskStrLen(mc->clientIdentifier) == 0) {
		char generatedId[10];

		if (!mc->cleanSession) {
			return kKprMQTTMessageReturnCodeIdentifierRejected;
		}

		generatedId[0] = 0xfe; // invalid unicode character which isolates these generated number from unicode name space.

		do {
			FskStrNumToHex(FskRandom(), &generatedId[1], 4);
			FskStrNumToHex(FskRandom(), &generatedId[5], 4);
		} while (KprMQTTBrokerFindClient(self, generatedId) != NULL);


		FskMemPtrDispose(mc->clientIdentifier);
		mc->clientIdentifier = FskStrDoCopy(generatedId);
	}

	oldClient = KprMQTTBrokerFindClient(self, mc->clientIdentifier);

	FskDebugStr(">CONNECT(0x%lx): %s, %s", (UInt32)client, mc->clientIdentifier, mc->cleanSession ? "clean" : "dirty");
	FskMemPtrDispose(client->clientIdentifier);
	client->clientIdentifier = mc->clientIdentifier;
	mc->clientIdentifier = NULL;
	client->cleanSession = mc->cleanSession;

	if (client->clientIdentifier == NULL) {
		return kKprMQTTMessageReturnCodeIdentifierRejected;
	}

	if (oldClient) {
		if (!client->cleanSession) {
			KprMQTTBrokerClientTakeOver(client, oldClient);
			*sesstionPresent = true;
		}

		KprMQTTBrokerClientDispose(oldClient);
	}

	if (mc->username) {
		FskDebugStr("  USERNAME: %s", mc->username);
		client->username = mc->username;
		mc->username = NULL;
	}

	if (mc->password) {
		FskDebugStr("  PASSWORD: %s", mc->password);
		client->password = mc->password;
		mc->password = NULL;
	}

	if (mc->willTopic) {
		FskDebugStr("  WILL TOPIC: %s", mc->willTopic);
		client->willTopic = mc->willTopic;
		mc->willTopic = NULL;
	}

	if (mc->willPayload) {
		FskDebugStr("  WILL PAYLOAD: %ld byte (%s)", mc->willPayload->size, mc->willPayload->buffer);
		client->willPayload = mc->willPayload;
		mc->willPayload = NULL;
	}

	client->willQoS = mc->willQualityOfService;
	client->willRetain = mc->willIsRetained;
	client->keepAlive = mc->keepAlive;

	return kKprMQTTMessageReturnCodeAccepted;
}

static FskErr KprMQTTBrokerSubscriptionNew(KprMQTTBroker self, const char *topicPattern, KprMQTTSubscription *it)
{
	FskErr err;
	KprMQTTSubscription subscription = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(KprMQTTSubscriptionRecord), &subscription));

	subscription->topic = FskStrDoCopy(topicPattern);
	bailIfNULL(subscription->topic);

	FskListAppend(&self->subscriptions, subscription);
	*it = subscription;
	subscription = NULL;

bail:
	if (err) {
		FskMemPtrDispose(subscription->topic);
		FskMemPtrDispose(subscription);
	}
	return err;
}

static void KprMQTTBrokerSubscriptionDispose(KprMQTTBroker self, KprMQTTSubscription subscription)
{
	FskListRemove(&self->subscriptions, subscription);
	FskMemPtrDispose(subscription->topic);
	FskMemPtrDispose(subscription);
}

static FskErr KprMQTTBrokerSubscriberNew(KprMQTTSubscription subscription, const char *client, KprMQTTSubscriber *it)
{
	FskErr err;
	KprMQTTSubscriber subscriber = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(KprMQTTSubscriberRecord), &subscriber));

	subscriber->client = FskStrDoCopy(client);
	bailIfNULL(subscriber->client);

	FskListAppend(&subscription->subscribers, subscriber);
	*it = subscriber;
	subscriber = NULL;

bail:
	if (err) {
		FskMemPtrDispose(subscriber->client);
		FskMemPtrDispose(subscriber);
	}
	return err;
}

static void KprMQTTBrokerSubscriberDispose(KprMQTTSubscription subscription, KprMQTTSubscriber subscriber)
{
	FskListRemove(&subscription->subscribers, subscriber);
	FskMemPtrDispose(subscriber->client);
	FskMemPtrDispose(subscriber);
}

static FskErr KprMQTTBrokerSubscribe(KprMQTTBroker self, const char *clientIdentifier, const char *topicPattern, UInt8 qos)
{
	FskErr err = kFskErrNone;
	KprMQTTSubscription subscription;
	KprMQTTSubscriber subscriber;
	KprMQTTRetainedMessage rm = NULL;
	KprMQTTBrokerClient client = NULL;

	if (!KprMQTTIsValidTopic(topicPattern, true)) return kFskErrBadData;

	subscription = KprMQTTBrokerFindSubscription(self, topicPattern);
	if (subscription == NULL) {
		bailIfError(KprMQTTBrokerSubscriptionNew(self, topicPattern, &subscription));
	}

	subscriber = KprMQTTBrokerFindSubscriber(subscription, clientIdentifier);
	if (subscriber == NULL) {
		bailIfError(KprMQTTBrokerSubscriberNew(subscription, clientIdentifier, &subscriber));
	}
	subscriber->qos = qos;

	while ((rm = KprMQTTBrokerRemindMessage(self, topicPattern, rm)) != NULL) {
		if (client == NULL) {
			client = KprMQTTBrokerFindClient(self, clientIdentifier);
			if (client == NULL) break;
		}

		qos = (qos < rm->qos ? qos : rm->qos);
		err = KprMQTTBrokerClientSendPublish(client, rm->topic, rm->payload, qos, true);
	}

bail:
	return err;
}

static FskErr KprMQTTBrokerUnsubscribe(KprMQTTBroker self, const char *client, const char *topicPattern)
{
	FskErr err = kFskErrNone;
	KprMQTTSubscription subscription;
	KprMQTTSubscriber subscriber;

	if (!KprMQTTIsValidTopic(topicPattern, true)) return kFskErrBadData;

	subscription = KprMQTTBrokerFindSubscription(self, topicPattern);
	if (subscription == NULL) return kFskErrNotFound;

	subscriber = KprMQTTBrokerFindSubscriber(subscription, client);
	if (subscriber == NULL) return kFskErrNotFound;

	KprMQTTBrokerSubscriberDispose(subscription, subscriber);

	if (subscription->subscribers == NULL) {
		KprMQTTBrokerSubscriptionDispose(self, subscription);
	}

	return err;
}

static FskErr KprMQTTBrokerUnsubscribeClient(KprMQTTBroker self, const char *client)
{
	FskErr err = kFskErrNone;
	KprMQTTSubscription subscription;

	subscription = self->subscriptions;
	while (subscription) {
		KprMQTTSubscription next = subscription->next;
		KprMQTTSubscriber subscriber;

		subscriber = KprMQTTBrokerFindSubscriber(subscription, client);
		if (subscriber) {
			KprMQTTBrokerSubscriberDispose(subscription, subscriber);

			if (subscription->subscribers == NULL) {
				KprMQTTBrokerSubscriptionDispose(self, subscription);
			}
		}

		subscription = next;
	}

	return err;
}

static Boolean KprMQTTBrokerIsSentToClient(KprMemoryBuffer clients, const char *client)
{
	while (clients) {
		if (FskStrCompare(client, clients->buffer) == 0) return true;
		clients = clients->next;
	}
	return false;
}

static FskErr KprMQTTBrokerPublish(KprMQTTBroker self, const char *topic, KprMemoryBuffer payload, UInt8 qos, Boolean retain)
{
	KprMQTTSubscription subscription = self->subscriptions;
	KprMemoryBuffer sentClients = NULL, clientIdentifier;
	FskErr err;

	if (!KprMQTTIsValidTopic(topic, false)) return kFskErrBadData;

	while (subscription) {
		if (KprMQTTMatchTopic(topic, subscription->topic)) {
			KprMQTTSubscriber subscriber = subscription->subscribers;
			while (subscriber) {
				KprMQTTBrokerClient client;

				if (!KprMQTTBrokerIsSentToClient(sentClients, subscriber->client)) {
					bailIfError(KprMemoryBufferNewFromString(subscriber->client, &clientIdentifier));
					FskListAppend(&sentClients, clientIdentifier);

					client = KprMQTTBrokerFindClient(self, subscriber->client);
					if (client) {
						/* err = */ KprMQTTBrokerClientSendPublish(client, topic, payload, (qos < subscriber->qos ? qos : subscriber->qos), false);
						// @TODO report error
					}
				}
				subscriber = subscriber->next;
			}
		}
		subscription = subscription->next;
	}

	if (retain) {
		if (payload && payload->size > 0) {
			bailIfError(KprMQTTBrokerRememberMessage(self, topic, payload, qos));
		} else {
			bailIfError(KprMQTTBrokerForgetMessage(self, topic));
		}
	}

bail:
	clientIdentifier = sentClients;
	while (clientIdentifier) {
		KprMemoryBuffer next = clientIdentifier->next;
		KprMemoryBufferDispose(clientIdentifier);
		clientIdentifier = next;
	}
	return kFskErrNone;
}

// subscriptions

static KprMQTTSubscription KprMQTTBrokerFindSubscription(KprMQTTBroker self, const char *topicPattern)
{
	KprMQTTSubscription subscription = self->subscriptions;
	while (subscription) {
		if (FskStrCompare(topicPattern, subscription->topic) == 0) return subscription;
		subscription = subscription->next;
	}
	return NULL;
}

static KprMQTTSubscriber KprMQTTBrokerFindSubscriber(KprMQTTSubscription subscription, const char *client)
{
	KprMQTTSubscriber subscriber = subscription->subscribers;
	while (subscriber) {
		if (FskStrCompare(subscriber->client, client) == 0) return subscriber;
		subscriber = subscriber->next;
	}
	return NULL;
}

static KprMQTTBrokerClient KprMQTTBrokerFindClient(KprMQTTBroker self, const char *identifier)
{
	KprMQTTBrokerClient client = self->clients;

	while (client) {
		if (FskStrCompare(identifier, client->clientIdentifier) == 0) return client;
		client = client->next;
	}
	return NULL;
}

// retained message

static FskErr KprMQTTBrokerRememberMessage(KprMQTTBroker self, const char *topic, KprMemoryBuffer payload, UInt8 qos)
{
	FskErr err = kFskErrNone;
	KprMQTTRetainedMessage rm = NULL;

	KprMQTTBrokerForgetMessage(self, topic);

	bailIfError(FskMemPtrNewClear(sizeof(KprMQTTRetainedMessageRecord), &rm));
	bailIfError(KprMemoryBufferDuplicate(payload, &rm->payload));
	rm->topic = FskStrDoCopy(topic);
	bailIfNULL(rm->topic);
	rm->qos = qos;

	FskListAppend(&self->retainedMessages, rm);

bail:
	if (err) {
		FskMemPtrDispose(rm);
	}
	return err;
}

static FskErr KprMQTTBrokerForgetMessage(KprMQTTBroker self, const char *topic)
{
	KprMQTTRetainedMessage rm = self->retainedMessages;
	while (rm) {
		if (FskStrCompare(topic, rm->topic) == 0) {
			FskListRemove(&self->retainedMessages, rm);

			FskMemPtrDispose(rm->topic);
			KprMemoryBufferDispose(rm->payload);
			FskMemPtrDispose(rm);
			return kFskErrNone;
		}
		rm = rm->next;
	}
	return kFskErrNotFound;
}

static KprMQTTRetainedMessage KprMQTTBrokerRemindMessage(KprMQTTBroker self, const char *topicPattern, KprMQTTRetainedMessage after)
{
	KprMQTTRetainedMessage rm = (after ? after->next : self->retainedMessages);
	while (rm) {
		if (KprMQTTMatchTopic(rm->topic, topicPattern)) return rm;
		rm = rm->next;
	}
	return NULL;
}

//--------------------------------------------------
// MQTT Broker Client -- client object in broker
//--------------------------------------------------

static void KprMQTTBrokerClient_beforeConnectMessage(KprMQTTEndpoint endpoint, KprMQTTMessage message, void *refcon);
static void KprMQTTBrokerClient_onMessage(KprMQTTEndpoint endpoint, KprMQTTMessage message, void *refcon);
static void KprMQTTBrokerClient_onEndpointError(KprMQTTEndpoint endpoint, FskErr err, char *reason, void *refcon);

static void KprMQTTBrokerClientHandleError(KprMQTTBrokerClient self, FskErr err, char *reason);

static FskErr KprMQTTBrokerClientSendMessage(KprMQTTBrokerClient self, KprMQTTMessage *message);
static void KprMQTTClientDeliverMessage(KprMQTTQueue queue, KprMQTTMessage *message, void *refcon);

//static void KprMQTTBrokerClientSetConnectTimer(KprMQTTBrokerClient self);
static void KprMQTTBrokerClientResetKeepAliveTimer(KprMQTTBrokerClient self);
static void KprMQTTBrokerClientIdleDisconnect(FskTimeCallBack callback, const FskTime time, void *it);



static FskErr KprMQTTBrokerClientNew(KprMQTTBrokerClient *it, KprMQTTBroker broker) {
	FskErr err = kFskErrNone;
	KprMQTTBrokerClient self = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(KprMQTTBrokerClientRecord), &self));

	self->broker = broker;

	FskTimeCallbackNew(&self->idleCallaback);
	bailIfNULL(self->idleCallaback);

	bailIfError(KprMQTTQueueNew(&self->queue, 15, KprMQTTClientDeliverMessage, self));

	FskListAppend((FskList*)&self->broker->clients, self);

	*it = self;

bail:
	if (err) {
		KprMQTTBrokerClientDispose(self);
	}

	return err;
}

static void KprMQTTBrokerClientDispose(KprMQTTBrokerClient self) {
	if (self) {
		FskListRemove((FskList*)&self->broker->clients, self);
		if (self->clientIdentifier) {
			KprMQTTBrokerUnsubscribeClient(self->broker, self->clientIdentifier);
		}

		FskTimeCallbackDispose(self->idleCallaback);

		KprMQTTBrokerClientClose(self);

		KprMQTTQueueDispose(self->queue);

		FskMemPtrDispose(self->clientIdentifier);

		FskMemPtrDispose(self);
		FskDebugStr("BROKER: DISPOSE CLIENT (0x%lx)", (UInt32)self);
	}
}

static FskErr KprMQTTBrokerClientOpen(KprMQTTBrokerClient self, FskSocket skt)
{
	FskErr err = kFskErrNone;
	KprMQTTEndpoint endpoint = NULL;

	if (self->open) return kFskErrBadState;

	FskNetSocketMakeNonblocking(skt);

	bailIfError(KprMQTTEndpointNew(&endpoint, skt, self));
	endpoint->errorCallback = KprMQTTBrokerClient_onEndpointError;

	KprMQTTBrokerClientReopen(self, endpoint);

bail:
	return err;
}

static FskErr KprMQTTBrokerClientReopen(KprMQTTBrokerClient self, KprMQTTEndpoint endpoint)
{
	FskErr err = kFskErrNone;

	if (self->endpoint) {
		KprMQTTEndpointDispose(self->endpoint);
		self->endpoint = NULL;
	}

	self->endpoint = endpoint;
	KprMQTTQueueStart(self->queue);

	self->open = true;
	self->endpoint->messageCallback = KprMQTTBrokerClient_beforeConnectMessage;

	return err;
}

static void KprMQTTBrokerClientClose(KprMQTTBrokerClient self)
{
	if (self->endpoint) {
		KprMQTTEndpointDispose(self->endpoint);
		self->endpoint = NULL;
	}

	KprMQTTQueueStop(self->queue);

	FskMemPtrDisposeAt(&self->username);
	FskMemPtrDisposeAt(&self->password);
	FskMemPtrDisposeAt(&self->willTopic);
	KprMemoryBufferDisposeAt(&self->willPayload);

	self->open = false;
}

static void KprMQTTBrokerClientTakeOver(KprMQTTBrokerClient self, KprMQTTBrokerClient oldClient)
{
	KprMQTTQueueStop(self->queue);
	KprMQTTQueueExchange(self->queue, oldClient->queue);
	KprMQTTQueueStart(self->queue);
}

static void KprMQTTBrokerClientRequestClose(KprMQTTBrokerClient self, Boolean graceful)
{
	if (self->open) {
		void *action = (self->cleanSession ? KprMQTTBrokerClientDispose : KprMQTTBrokerClientClose);

		INVOKE_AFTER1(action, self);
		self->open = false;

		if (!graceful && self->willTopic) {
			KprMQTTBrokerPublish(self->broker, self->willTopic, self->willPayload, self->willQoS, self->willRetain);
		}
	}
}

static FskErr KprMQTTBrokerClientCreatePublishMessage(KprMQTTBrokerClient self, const char *topic, KprMemoryBuffer payload, UInt8 qos, Boolean retain, KprMQTTMessage *it)
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

	if (payload) {
		bailIfError(KprMemoryBufferDuplicate(payload, &message->t.publish.payload));
	}

	*it = message;
	message = NULL;

bail:
	KprMQTTMessageDispose(message);
	return err;
}

static FskErr KprMQTTBrokerClientSendPublish(KprMQTTBrokerClient self, const char *topic, KprMemoryBuffer payload, UInt8 qos, Boolean retain)
{
	FskErr err;
	KprMQTTMessage message = NULL;

	bailIfError(KprMQTTBrokerClientCreatePublishMessage(self, topic, payload, qos, retain, &message));

	FskDebugStr("BROKER: send PUBLISH(0x%lx): qos:%d %s : %s", (UInt32)self, message->qualityOfService, topic, (char *)payload->buffer);
	if (!self->open) {
		bailIfError(KprMQTTQueueOutboxPut(self->queue, &message));
	} else {
		bailIfError(KprMQTTBrokerClientSendMessage(self, &message));
	}

bail:
	KprMQTTMessageDispose(message);

	return err;
}

static void KprMQTTBrokerClient_beforeConnectMessage(KprMQTTEndpoint endpoint UNUSED, KprMQTTMessage message, void *refcon)
{
	KprMQTTBrokerClient self = refcon;
	FskErr err = kFskErrNone;
	KprMQTTMessage message2 = NULL;

	if (!self->open) return;

	switch (message->type) {
		case kKprMQTTMessageTypeCONNECT: {
			UInt8 returnCode;
			Boolean sesstionPresent = false;

			returnCode = KprMQTTBrokerAuthorize(self->broker, self, message, &sesstionPresent);

			KprMQTTBrokerClientResetKeepAliveTimer(self);

			bailIfError(KprMQTTMessageNewWithType(&message2, kKprMQTTMessageTypeCONNACK));
			message2->t.connack.returnCode = returnCode;
			message2->t.connack.sesstionPresent = sesstionPresent;

			bailIfError(KprMQTTBrokerClientSendMessage(self, &message2));
			if (returnCode != kKprMQTTMessageReturnCodeAccepted) {
				KprMQTTBrokerClientRequestClose(self, true);
			} else {
				self->endpoint->messageCallback = KprMQTTBrokerClient_onMessage;
			}
			break;
		}
		default:
			err = kFskErrBadState;
			break;
	}

bail:
	KprMQTTMessageDispose(message);
	KprMQTTMessageDispose(message2);

	if (err) {
		KprMQTTBrokerClientHandleError(self, err, "authorization");
	}
}

static void KprMQTTBrokerClient_onMessage(KprMQTTEndpoint endpoint UNUSED, KprMQTTMessage message, void *refcon)
{
	KprMQTTBrokerClient self = refcon;
	FskErr err = kFskErrNone;
	KprMQTTMessage message2 = NULL, message3 = NULL;

	if (!self->open) return;

	KprMQTTBrokerClientResetKeepAliveTimer(self);

	switch (message->type) {
		case kKprMQTTMessageTypeSUBSCRIBE: {
			KprMQTTSubscribeTopic t1, t2;

			FskDebugStr(">SUBSCRIBE(0x%lx): %d", (UInt32)self, message->messageId);
			bailIfError(KprMQTTMessageNewWithType(&message2, kKprMQTTMessageTypeSUBACK));
			message2->messageId = message->messageId;

			t1 = message->t.other.topics;
			while (t1) {
				bailIfError(KprMQTTBrokerSubscribe(self->broker, self->clientIdentifier, t1->topic, t1->qualityOfService));

				bailIfError(KprMQTTMessageAddSubscribeTopic(message2, &t2));
				t2->qualityOfService = t1->qualityOfService;

				t1 = t1->next;
			}

			FskDebugStr("<SUBACK(0x%lx): %d", (UInt32)self, message2->messageId);
			bailIfError(KprMQTTBrokerClientSendMessage(self, &message2));
			break;
		}

		case kKprMQTTMessageTypeUNSUBSCRIBE: {
			KprMQTTSubscribeTopic t = message->t.other.topics;

			FskDebugStr(">UNSUBSCRIBE(0x%lx): %d", (UInt32)self, message->messageId);
			while (t) {
				KprMQTTBrokerUnsubscribe(self->broker, self->clientIdentifier, t->topic);
				t = t->next;
			}

			bailIfError(KprMQTTMessageNewWithType(&message2, kKprMQTTMessageTypeUNSUBACK));
			message2->messageId = message->messageId;

			FskDebugStr("<UNSUBACK(0x%lx): %d", (UInt32)self, message2->messageId);
			bailIfError(KprMQTTBrokerClientSendMessage(self, &message2));
			break;
		}

		case kKprMQTTMessageTypePUBLISH: {
			UInt8 qos = message->qualityOfService;
			UInt16 messageId = message->messageId;

			FskDebugStr(">PUBLISH(0x%lx): %d, QoS %d%s", (UInt32)self, message->qualityOfService, message->messageId, message->isRetained ? " retain" : "");
			if (message->qualityOfService == 2) {
				bailIfError(KprMQTTQueueInboxPut(self->queue, &message));
			} else {
				char *topic = message->t.publish.topic;
				KprMemoryBuffer payload = message->t.publish.payload;
				Boolean retain = message->isRetained;
				UInt8 qos = message->qualityOfService;
				bailIfError(KprMQTTBrokerPublish(self->broker, topic, payload, qos, retain));
			}

			if (qos > 0) {
				UInt8 type = (qos == 1 ? kKprMQTTMessageTypePUBACK : kKprMQTTMessageTypePUBREC);
				bailIfError(KprMQTTMessageNewWithType(&message2, type));
				message2->messageId = messageId;

				FskDebugStr("<%s(0x%lx): %d", (qos == 1 ? "PUBACK" : "PUBREC"), (UInt32)self, message2->messageId);
				bailIfError(KprMQTTBrokerClientSendMessage(self, &message2));
			}
			break;
		}

		case kKprMQTTMessageTypePUBREC: {
			FskDebugStr(">PUBREC(0x%lx): %d", (UInt32)self, message->messageId);
			message2 = KprMQTTQueueOutboxGet(self->queue, message->messageId);
			if (!message2) break;

			bailIfError(KprMQTTMessageNewWithType(&message3, kKprMQTTMessageTypePUBREL));
			message3->messageId = message->messageId;

			FskDebugStr("<PUBREL(0x%lx): %d", (UInt32)self, message3->messageId);
			bailIfError(KprMQTTBrokerClientSendMessage(self, &message3));
			break;
		}

		case kKprMQTTMessageTypePUBREL: {
			FskDebugStr(">PUBREL(0x%lx): %d", (UInt32)self, message->messageId);
			message2 = KprMQTTQueueInboxGet(self->queue, message->messageId);
			if (!message2) break;

			{
				char *topic = message2->t.publish.topic;
				KprMemoryBuffer payload = message2->t.publish.payload;
				Boolean retain = message2->isRetained;
				UInt8 qos = message2->qualityOfService;
				bailIfError(KprMQTTBrokerPublish(self->broker, topic, payload, qos, retain));
			}

			bailIfError(KprMQTTMessageNewWithType(&message3, kKprMQTTMessageTypePUBCOMP));
			message3->messageId = message->messageId;

			FskDebugStr("<PUBCOMP(0x%lx): %d", (UInt32)self, message3->messageId);
			bailIfError(KprMQTTBrokerClientSendMessage(self, &message3));
			break;
		}

		case kKprMQTTMessageTypePUBACK:
		case kKprMQTTMessageTypePUBCOMP: {
			FskDebugStr(">%s(0x%lx): %d", (message->type == kKprMQTTMessageTypePUBACK ? "PUBACK" : "PUBCOMP"), (UInt32)self, message->messageId);
			message2 = KprMQTTQueueOutboxGet(self->queue, message->messageId);
			break;
		}

		case kKprMQTTMessageTypePINGREQ: {
			FskDebugStr(">PINGREQ(0x%lx)", (UInt32)self);
			bailIfError(KprMQTTMessageNewWithType(&message2, kKprMQTTMessageTypePINGRESP));

			FskDebugStr("<PINGRESP(0x%lx)", (UInt32)self);
			bailIfError(KprMQTTBrokerClientSendMessage(self, &message2));
			break;
		}

		case kKprMQTTMessageTypeDISCONNECT: {
			FskDebugStr(">DISCONNECT(0x%lx)", (UInt32)self);
			KprMQTTBrokerClientRequestClose(self, true);
			return;
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
		KprMQTTBrokerClientHandleError(self, err, "handling message");
	}
}

static void KprMQTTBrokerClient_onEndpointError(KprMQTTEndpoint endpoint UNUSED, FskErr err, char *reason, void *refcon)
{
	KprMQTTBrokerClient self = refcon;

	KprMQTTBrokerClientHandleError(self, err, reason);
}

static void KprMQTTBrokerClientHandleError(KprMQTTBrokerClient self, FskErr err, char *reason)
{
	KprMQTTBrokerHandleError(self->broker, err, reason);

	switch (err) {
		case kFskErrConnectionClosed:
			KprMQTTBrokerClientRequestClose(self, false);
			break;

		default:
			break;
	}
}

static FskErr KprMQTTBrokerClientSendMessage(KprMQTTBrokerClient self, KprMQTTMessage *message)
{
	FskErr err = kFskErrNone;

	if (!self->open) bailIfError(kFskErrConnectionClosed);

	bailIfError(KprMQTTEndpointSendMessage(self->endpoint, *message));

	if ((*message)->qualityOfService > 0) {
		(*message)->duplicateDelivery = true;
		bailIfError(KprMQTTQueueOutboxPut(self->queue, message));
	}

bail:
	return err;
}

static void KprMQTTClientDeliverMessage(KprMQTTQueue queue, KprMQTTMessage *message, void *refcon)
{
	KprMQTTBrokerClient self = refcon;
	FskErr err = KprMQTTBrokerClientSendMessage(self, message);

	if (err) {
		KprMQTTBrokerHandleError(self->broker, err, "resend fail");
	}
}

// --- KEEP ALIVE -----------------------------------

#if 0
static void KprMQTTBrokerClientSetConnectTimer(KprMQTTBrokerClient self)
{
	FskTimeRecord when;

	FskTimeCallbackRemove(self->idleCallaback);

	FskTimeGetNow(&when);
	FskTimeAddSecs(&when, kKprMQTTBrokerConnectTimeout);
	FskTimeCallbackSet(self->idleCallaback, &when, KprMQTTBrokerClientIdleDisconnect, self);
}
#endif

static void KprMQTTBrokerClientResetKeepAliveTimer(KprMQTTBrokerClient self)
{
	FskTimeCallbackRemove(self->idleCallaback);

	if (self->keepAlive > 0) {
		FskTimeRecord when;
		double secs = self->keepAlive;
		secs *= 1.5;

		FskTimeGetNow(&when);
		FskTimeAddSecs(&when, secs);
		FskTimeCallbackSet(self->idleCallaback, &when, KprMQTTBrokerClientIdleDisconnect, self);
	}

}

static void KprMQTTBrokerClientIdleDisconnect(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *it)
{
	KprMQTTBrokerClient self = it;

	KprMQTTBrokerClientRequestClose(self, false);
}


#if defined(RUN_UNITTEST) && RUN_UNITTEST

#include "kunit.h"

int KprMQTTBrokerClientTopicQoS(KprMQTTBroker broker, const char *client, const char *topic)
{
	KprMQTTSubscription subscription;
	KprMQTTSubscriber subscriber;

	subscription = KprMQTTBrokerFindSubscription(broker, topic);
	if (!subscription) return -1;

	subscriber = KprMQTTBrokerFindSubscriber(subscription, client);
	if (!subscriber) return -1;

	return subscriber->qos;
}

#define qosOfTopic(C,T) KprMQTTBrokerClientTopicQoS(broker, C, T)

ku_test(MQTT_broker_Subscription)
{
	FskErr err = kFskErrNone;
	KprMQTTBroker broker;
	KprMQTTSubscription subscription;
	int c;

	// setup
	bailIfError(KprMQTTBrokerNew(&broker, NULL));

	bailIfError(KprMQTTBrokerSubscribe(broker, "A", "#", 0));
	bailIfError(KprMQTTBrokerSubscribe(broker, "B", "#", 1));
	bailIfError(KprMQTTBrokerSubscribe(broker, "C", "#", 2));
	bailIfError(KprMQTTBrokerSubscribe(broker, "C", "kinoma/#", 0));
	bailIfError(KprMQTTBrokerSubscribe(broker, "B", "kinoma/+/demo", 1));

	// test
	ku_assert(FskListCount(broker->subscriptions) == 3, "");

	subscription = KprMQTTBrokerFindSubscription(broker, "#");
	c = FskListCount(subscription->subscribers);
	ku_assert(c == 3, "3 clients is subscribing");

	subscription = KprMQTTBrokerFindSubscription(broker, "kinoma/#");
	c = FskListCount(subscription->subscribers);
	ku_assert(c == 1, "a client is subscribing");
	ku_assert(subscription->subscribers->qos == 0, "qos is 0");
	ku_assert(subscription->subscribers->client[0] == 'C', "client is C");

	subscription = KprMQTTBrokerFindSubscription(broker, "kinoma");
	ku_assert(subscription == NULL, "No subscription will find.");

	ku_assert(qosOfTopic("A", "#") == 0, "A * #");
	ku_assert(qosOfTopic("B", "#") == 1, "A * #");
	ku_assert(qosOfTopic("C", "#") == 2, "A * #");
	ku_assert(qosOfTopic("A", "kinoma/#") == -1, "A !* kinoma/#");
	ku_assert(qosOfTopic("B", "kinoma/#") == -1, "B !* kinoma/#");
	ku_assert(qosOfTopic("C", "kinoma/#") == 0, "C * kinoma/#");

	bailIfError(KprMQTTBrokerUnsubscribe(broker, "B", "#"));
	bailIfError(KprMQTTBrokerUnsubscribe(broker, "A", "#"));
	bailIfError(KprMQTTBrokerUnsubscribe(broker, "C", "#"));

	ku_assert(FskListCount(broker->subscriptions) == 2, "");
	ku_assert(qosOfTopic("A", "#") == -1, "A !* #");
	ku_assert(qosOfTopic("B", "#") == -1, "A !* #");
	ku_assert(qosOfTopic("C", "#") == -1, "A !* #");

	KprMQTTBrokerDispose(broker);
	ku_ok();

bail:
	ku_fail("error : %d", err);
}

ku_test(MQTT_broker)
{
	ku_run(MQTT_broker_Subscription);
}

#endif

