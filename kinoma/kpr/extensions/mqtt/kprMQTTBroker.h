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
#ifndef __KPRMQTTBROKER__
#define __KPRMQTTBROKER__

#include "kpr.h"
#include "kprURL.h"
#include "FskThread.h"
#include "FskNetInterface.h"

#include "kprMQTTEndpoint.h"

//--------------------------------------------------
// MQTTServer
//--------------------------------------------------

#define kKprMQTTBrokerConnectTimeout 60

typedef struct KprMQTTBrokerStruct KprMQTTBrokerRecord, *KprMQTTBroker;
typedef struct KprMQTTBrokerListenerStruct KprMQTTBrokerListenerRecord, *KprMQTTBrokerListener;
typedef struct KprMQTTBrokerClientStruct KprMQTTBrokerClientRecord, *KprMQTTBrokerClient;
typedef struct KprMQTTSubscriptionRecord KprMQTTSubscriptionRecord, *KprMQTTSubscription;
typedef struct KprMQTTSubscriberRecord KprMQTTSubscriberRecord, *KprMQTTSubscriber;
typedef struct KprMQTTRetainedMessageRecord KprMQTTRetainedMessageRecord, *KprMQTTRetainedMessage;

// Callbacks

typedef void (*KprMQTTBrokerLaunchCallback)(KprMQTTBroker self, void *refcon);
typedef void (*KprMQTTBrokerConnectCallback)(KprMQTTBroker self, FskSocket sock, void *refcon);
typedef void (*KprMQTTBrokerErrorCallback)(KprMQTTBroker self, FskErr err, char *message, void *refcon);

struct KprMQTTBrokerStruct {
	KprMQTTBrokerClient clients;

	void *refCon;

//	FskNetInterfaceNotifier interfaceNotifier;
	FskThread owner;

	KprSocketServer server;

	KprMQTTBrokerLaunchCallback launchCallback;
	KprMQTTBrokerConnectCallback connectCallback;
	KprMQTTBrokerErrorCallback errorCallback;

	KprMQTTSubscription subscriptions;
	KprMQTTRetainedMessage retainedMessages;
};

struct KprMQTTBrokerClientStruct {
	KprMQTTBrokerClient next;
	KprMQTTBroker /* @weak */ broker;
	KprMQTTEndpoint endpoint;

	KprMQTTQueue queue;

	char *clientIdentifier;
	char *username;
	char *password;
	char *willTopic;
	KprMemoryBuffer willPayload;
	UInt8 willQoS;
	Boolean willRetain;

	UInt16 keepAlive;
	FskTimeCallBack idleCallaback;

	Boolean open;
	Boolean cleanSession;
};

// Subscription

struct KprMQTTSubscriptionRecord {
	KprMQTTSubscription next;
	char *topic;
	KprMQTTSubscriber subscribers;
};

struct KprMQTTSubscriberRecord {
	KprMQTTSubscriber next;
	char *client;
	UInt8 qos;
};

struct KprMQTTRetainedMessageRecord {
	KprMQTTRetainedMessage next;
	char *topic;
	KprMemoryBuffer payload;
	UInt8 qos;
};

FskErr KprMQTTBrokerNew(KprMQTTBroker* it, void *refCon);
void KprMQTTBrokerDispose(KprMQTTBroker self);
FskErr KprMQTTBrokerListen(KprMQTTBroker self, int port, char *interfaceName);

#endif
