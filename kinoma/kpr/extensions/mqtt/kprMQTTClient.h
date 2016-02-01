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
#ifndef __KPRMQTTCLIENT__
#define __KPRMQTTCLIENT__

#include "kprMQTTEndpoint.h"

//--------------------------------------------------
// MQTT Client
//--------------------------------------------------

enum {
	kKprMQTTStateDisconnected = 0,
	kKprMQTTStateConnecting = 1,
	kKprMQTTStateHandshaking = 2,
	kKprMQTTStateEstablished = 3,
};

typedef struct KprMQTTClientRecord KprMQTTClientRecord, *KprMQTTClient;
typedef struct KprMQTTClientConnectOptions KprMQTTClientConnectOptions;

typedef void (*KprMQTTClientConnectCallback)(KprMQTTClient self, UInt8 returnCode, Boolean sessionPresent, void *refcon);
typedef void (*KprMQTTClientSubscribeCallback)(KprMQTTClient self, UInt16 token, KprMQTTSubscribeTopic request, KprMQTTSubscribeTopic result, void *refcon);
typedef void (*KprMQTTClientUnsubscribeCallback)(KprMQTTClient self, UInt16 token, KprMQTTSubscribeTopic request, void *refcon);
typedef void (*KprMQTTClientPublishCallback)(KprMQTTClient self, UInt16 token, void *refcon);
typedef void (*KprMQTTClientMessageCallback)(KprMQTTClient self, char *topic, KprMemoryBuffer payload, void *refcon);
typedef void (*KprMQTTClientDisconnectCallback)(KprMQTTClient self, Boolean cleanClose, void *refcon);
typedef void (*KprMQTTClientErrorCallback)(KprMQTTClient self, FskErr err, char *reason, void *refcon);

struct KprMQTTClientRecord {
	char *clientIdentifier;
	Boolean cleanSession;
	KprMQTTProtocolVersion protocolVersion;

	char	*host;
	UInt16 port;
	Boolean isSecure;

	UInt8 state;
	KprMQTTEndpoint endpoint;

	Boolean cancelConnection;
	Boolean disconnectWasSent;
	KprMQTTMessage connectMessage;

	KprMQTTQueue queue;

	UInt16 keepAlive;
	FskTimeCallBack pingRequestCallaback;
	FskTimeCallBack pingResponseCallaback;

	KprMQTTClientConnectCallback connectCallback;
	KprMQTTClientSubscribeCallback subscribeCallback;
	KprMQTTClientUnsubscribeCallback unsubscribeCallback;
	KprMQTTClientPublishCallback publishCallback;
	KprMQTTClientMessageCallback messageCallback;
	KprMQTTClientDisconnectCallback disconnectCallback;
	KprMQTTClientErrorCallback errorCallback;

	void *refcon;
};

struct KprMQTTClientConnectOptions {
	Boolean isSecure;

	char *willTopic;
	void *willPayload;
	UInt32 willPayloadLength;
	UInt8 willQualityOfService;
	Boolean willIsRetained;

	char *username;
	char *password;

	UInt16 keepAlive;
};


FskErr KprMQTTClientNew(KprMQTTClient *it, char *clientIdentifier, Boolean cleanSession, KprMQTTProtocolVersion protocolVersion, void *refcon);
FskErr KprMQTTClientDispose(KprMQTTClient self);

FskErr KprMQTTClientConnect(KprMQTTClient self, char *host, UInt16 port, KprMQTTClientConnectOptions *options);
FskErr KprMQTTClientReconnect(KprMQTTClient self);
FskErr KprMQTTClientDisconnect(KprMQTTClient self);

FskErr KprMQTTClientPublish(KprMQTTClient self, char *topic, void *payload, UInt32 payloadLength, UInt8 qos, Boolean retain, UInt16 *token);
FskErr KprMQTTClientSubscribeTopic(KprMQTTClient self, char *topic, UInt8 qos, UInt16 *token);
FskErr KprMQTTClientSubscribeTopics(KprMQTTClient self, char **topics, UInt8 *qoss, int count, UInt16 *token);
FskErr KprMQTTClientUnsubscribeTopic(KprMQTTClient self, char *topic, UInt16 *token);
FskErr KprMQTTClientUnsubscribeTopics(KprMQTTClient self, char **topics, int count, UInt16 *token);

const char *KprMQTTClientGetProtocolVersion(KprMQTTClient self);

#endif
