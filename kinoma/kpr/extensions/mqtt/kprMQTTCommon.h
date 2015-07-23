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
#ifndef __KPRMQTTCOMMON__
#define __KPRMQTTCOMMON__

#include "xs.h"
#include "FskTime.h"
#include "FskMemory.h"

//--------------------------------------------------
// MQTT Common
//--------------------------------------------------

/*

			QoS		DUP		RETAIN	VHEAD				PAYLOAD
CONNECT		-		-		-		12					ClientIdentifier,
														WillTopic,
														WillMessage,
														UserName,
														Password
CONNACK		-		-		-		2					-
PUBLISH		0/1/2	0/1		0/1		Topic, MessageID	Data
PUBACK		-		-		-		MessageID			-
PUBREC		-		-		-		MessageID			-
PUBREL		1		0/1		-		MessageID			-
PUBCOMP		-		-		-		MessageID			-
SUBSCRIBE	1		0/1		-		MessageID			[Topic, QoS]*
SUBACK		-		-		-		MessageID			[QoS]*
UNSUBSCRIBE	1		0/1		-		MessageID			[Topoc]*
UNSUBACK	-		-		-		MessageID			-
PINGREQ		-		-		-		-					-
PINGRESP	-		-		-		-					-
DISCONNECT	-		-		-		-					-

 */

// Memory Buffer

typedef struct KprMemoryBufferRecord KprMemoryBufferRecord, *KprMemoryBuffer;

struct KprMemoryBufferRecord {
	KprMemoryBuffer next;
	UInt32 size;
	void *buffer;
};

FskErr KprMemoryBufferNew(UInt32 size, KprMemoryBuffer *it);
FskErr KprMemoryBufferNewClear(UInt32 size, KprMemoryBuffer *it);
FskErr KprMemoryBufferNewFromData(UInt32 size, void *buffer, KprMemoryBuffer *it);
FskErr KprMemoryBufferNewFromString(const char *str, KprMemoryBuffer *it);
FskErr KprMemoryBufferDispose(KprMemoryBuffer self);
FskErr KprMemoryBufferDisposeAt(KprMemoryBuffer *self);

FskErr KprMemoryBufferDuplicate(KprMemoryBuffer self, KprMemoryBuffer *it);
FskErr KprMemoryBufferCopyBuffer(KprMemoryBuffer self, FskMemPtr *it);
UInt32 KprMemoryBufferCopyTo(KprMemoryBuffer self, void *dest);

#define kMQTTTopicLevelSeparator '/'
#define kMQTTMultiLevelSeparator '#'
#define kMQTTSingleLevelSeparator '+'

enum {
	kKprMQTTInvalidMessageId = 0,
};

enum {
	kKprMQTTMessageTypeCONNECT = 1,
	kKprMQTTMessageTypeCONNACK,
	kKprMQTTMessageTypePUBLISH,
	kKprMQTTMessageTypePUBACK,
	kKprMQTTMessageTypePUBREC,
	kKprMQTTMessageTypePUBREL,
	kKprMQTTMessageTypePUBCOMP,
	kKprMQTTMessageTypeSUBSCRIBE,
	kKprMQTTMessageTypeSUBACK,
	kKprMQTTMessageTypeUNSUBSCRIBE,
	kKprMQTTMessageTypeUNSUBACK,
	kKprMQTTMessageTypePINGREQ,
	kKprMQTTMessageTypePINGRESP,
	kKprMQTTMessageTypeDISCONNECT,
};

enum {
	kKprMQTTMessageQoSAtMostOnce = 0,
	kKprMQTTMessageQoSAtLeastOnce,
	kKprMQTTMessageQoSExactlyOnce,
};

enum {
	kKprMQTTMessageReturnCodeAccepted = 0,
	kKprMQTTMessageReturnCodeUnacceptableProtocolVersion,
	kKprMQTTMessageReturnCodeIdentifierRejected,
	kKprMQTTMessageReturnCodeServerUnavailable,
	kKprMQTTMessageReturnCodeBadUserNameOrPassword,
	kKprMQTTMessageReturnCodeNotAuthorized,
};

typedef struct KprMQTTMessageRecord KprMQTTMessageRecord, *KprMQTTMessage;
typedef struct KprMQTTSubscribeTopicRecord KprMQTTSubscribeTopicRecord, *KprMQTTSubscribeTopic;

struct KprMQTTConnectMessageRecord {
	char *clientIdentifier;
	char *willTopic;
	KprMemoryBuffer willPayload;
	UInt8 willQualityOfService;
	Boolean willIsRetained;
	Boolean cleanSession;
	char *username;
	char *password;
	UInt16 keepAlive;
};

struct KprMQTTConnackMessageRecord {
	UInt8 returnCode;
};

struct KprMQTTPublishMessageRecord {
	char *topic;
	KprMemoryBuffer payload;
};

struct KprMQTTOtherMessageRecord {
	KprMQTTSubscribeTopic topics;
};

struct KprMQTTMessageRecord {
	KprMQTTMessage next;

	UInt8 type;
	UInt8 qualityOfService;
	Boolean duplicateDelivery;
	Boolean isRetained;
	FskTimeRecord nextTime;
	UInt16 retryCount;
	UInt16 messageId;

	union {
		struct KprMQTTConnectMessageRecord connect;
		struct KprMQTTConnackMessageRecord connack;
		struct KprMQTTPublishMessageRecord publish;
		struct KprMQTTOtherMessageRecord other;
	} t;
};

struct KprMQTTSubscribeTopicRecord {
	KprMQTTSubscribeTopic next;
	UInt8 qualityOfService;
	char *topic;
};

FskErr KprMQTTMessageNew(KprMQTTMessage *it);
FskErr KprMQTTMessageNewWithType(KprMQTTMessage *it, UInt8 type);
FskErr KprMQTTMessageDispose(KprMQTTMessage message);
FskErr KprMQTTMessageDisposeAt(KprMQTTMessage *message);

FskErr KprMQTTMessageAddSubscribeTopic(KprMQTTMessage message);
KprMQTTSubscribeTopic KprMQTTMessageLastSubscribeTopic(KprMQTTMessage message);

Boolean KprMQTTMatchTopic(const char *topic, const char *pattern);
Boolean KprMQTTIsValidTopic(const char *topic, Boolean allowWildcard);

// Deferred Utilities

void KprMQTTInvokeAfter(void *func, void *param1, void *param2, void *param3, void *param4);
#define INVOKE_AFTER0(f) KprMQTTInvokeAfter(f, NULL, NULL, NULL, NULL)
#define INVOKE_AFTER1(f, p1) KprMQTTInvokeAfter(f, p1, NULL, NULL, NULL)
#define INVOKE_AFTER2(f, p1, p2) KprMQTTInvokeAfter(f, p1, p2, NULL, NULL)
#define INVOKE_AFTER3(f, p1, p2, p3) KprMQTTInvokeAfter(f, p1, p2, p3, NULL)
#define INVOKE_AFTER4(f, p1, p2, p3, p4) KprMQTTInvokeAfter(f, p1, p2, p3, p4)


//--------------------------------------------------
// MQTT Message Queue
//--------------------------------------------------

typedef struct KprMQTTQueueRecord KprMQTTQueueRecord, *KprMQTTQueue;
typedef void (*KprMQTTQueueCallback)(KprMQTTQueue queue, KprMQTTMessage *message, void *refcon);

struct KprMQTTQueueRecord {
	void *refcon;
	KprMQTTQueueCallback callback;
	KprMQTTMessage inbox;
	KprMQTTMessage outbox;
	FskTimeCallBack resendCallback;
	UInt16 resendInterval;
	UInt16 messageId;
	Boolean pause;
};

FskErr KprMQTTQueueNew(KprMQTTQueue *it, UInt16 resendInterval, KprMQTTQueueCallback callback, void *refcon);
FskErr KprMQTTQueueDispose(KprMQTTQueue queue);

void KprMQTTQueueExchange(KprMQTTQueue queue1, KprMQTTQueue queue2);

FskErr KprMQTTQueueInboxPut(KprMQTTQueue queue, KprMQTTMessage *message);
KprMQTTMessage KprMQTTQueueInboxGet(KprMQTTQueue queue, UInt16 messageId);

FskErr KprMQTTQueueOutboxPut(KprMQTTQueue queue, KprMQTTMessage *message);
KprMQTTMessage KprMQTTQueueOutboxGet(KprMQTTQueue queue, UInt16 messageId);

UInt16 KprMQTTQueueNextId(KprMQTTQueue queue);

void KprMQTTQueueStop(KprMQTTQueue queue);
void KprMQTTQueueStart(KprMQTTQueue queue);

#endif
