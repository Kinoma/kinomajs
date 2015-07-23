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
#include "kprMQTTCommon.h"
#include "kprShell.h"


FskErr KprMQTTMessageNew(KprMQTTMessage *it)
{
	FskErr err = kFskErrNone;
	KprMQTTMessage message;

	bailIfError(FskMemPtrNewClear(sizeof(KprMQTTMessageRecord), &message));

	*it = message;

bail:
	return err;
}

FskErr KprMQTTMessageNewWithType(KprMQTTMessage *it, UInt8 type)
{
	FskErr err = kFskErrNone;
	KprMQTTMessage message;

	bailIfError(KprMQTTMessageNew(it));
	message = *it;
	message->type = type;

	switch (type) {
		case kKprMQTTMessageTypePUBREL:
		case kKprMQTTMessageTypeSUBSCRIBE:
		case kKprMQTTMessageTypeUNSUBSCRIBE:
			message->qualityOfService = 1;
			break;

		case kKprMQTTMessageTypeCONNECT:
		case kKprMQTTMessageTypeCONNACK:
		case kKprMQTTMessageTypePUBLISH:
		case kKprMQTTMessageTypePUBACK:
		case kKprMQTTMessageTypePUBREC:
		case kKprMQTTMessageTypePUBCOMP:
		case kKprMQTTMessageTypeSUBACK:
		case kKprMQTTMessageTypeUNSUBACK:
		case kKprMQTTMessageTypePINGREQ:
		case kKprMQTTMessageTypePINGRESP:
		case kKprMQTTMessageTypeDISCONNECT:
			break;

		default:
			break;
	}

bail:
	return err;
}

FskErr KprMQTTMessageDispose(KprMQTTMessage message)
{
	if (message) {
		KprMQTTSubscribeTopic next;

		switch (message->type) {
			case kKprMQTTMessageTypeCONNECT:
				FskMemPtrDispose(message->t.connect.clientIdentifier);
				FskMemPtrDispose(message->t.connect.willTopic);
				KprMemoryBufferDispose(message->t.connect.willPayload);
				FskMemPtrDispose(message->t.connect.username);
				FskMemPtrDispose(message->t.connect.password);
				break;

			case kKprMQTTMessageTypePUBLISH:
				FskMemPtrDispose(message->t.publish.topic);
				KprMemoryBufferDispose(message->t.publish.payload);
				break;

			case kKprMQTTMessageTypeSUBSCRIBE:
			case kKprMQTTMessageTypeSUBACK:
			case kKprMQTTMessageTypeUNSUBSCRIBE:
				next = message->t.other.topics;
				while (next) {
					KprMQTTSubscribeTopic topic = next;
					next = topic->next;

					FskMemPtrDispose(topic->topic);
					FskMemPtrDispose(topic);
				}
				break;

			default:
				break;
		}
		FskMemPtrDispose(message);
	}
	return kFskErrNone;
}

FskErr KprMQTTMessageDisposeAt(KprMQTTMessage *message)
{
	FskErr err = kFskErrNone;
	if (message && *message != NULL) {
		err = KprMQTTMessageDispose(*message);
		*message = NULL;
	}
	return err;
}

// Deferred Utilities

void KprMQTTInvokeAfter(void *func, void *param1, void *param2, void *param3, void *param4)
{
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)func, param1, param2, param3, param4);
}

FskErr KprMQTTMessageAddSubscribeTopic(KprMQTTMessage message)
{
	KprMQTTSubscribeTopic topic;
	FskErr err;

	err = FskMemPtrNewClear(sizeof(KprMQTTSubscribeTopicRecord), &topic);
	if (err) return err;

	FskListAppend(&message->t.other.topics, topic);
	return kFskErrNone;
}

KprMQTTSubscribeTopic KprMQTTMessageLastSubscribeTopic(KprMQTTMessage message)
{
	KprMQTTSubscribeTopic topic = message->t.other.topics;
	while (topic) {
		if (topic->next == NULL) return topic;
		topic = topic->next;
	}
	return NULL;
}

// Topic Utilities

Boolean KprMQTTMatchTopic(const char *topic, const char *pattern)
{
	char *p;
	int len;

	do {
		if (FskStrCompare(topic, pattern) == 0) return true;

		p = FskStrChr(pattern, kMQTTSingleLevelSeparator);
		if (p) {
			len = p - pattern;
			if (FskStrCompareWithLength(topic, pattern, len) != 0) return false;
			topic += len;
			pattern += len + 1;

			topic = FskStrChr(topic, kMQTTTopicLevelSeparator);
			if (topic != NULL) {
				topic += 1;
				if (*pattern == 0) return false;
				pattern += 1;
			} else {
				if (*pattern != 0) return false;
				return true;
			}
		} else {
			break;
		}
	} while (true);

	p = FskStrChr(pattern, kMQTTMultiLevelSeparator);
	if (p == NULL) return false;

	len = p - pattern;
	if (FskStrCompareWithLength(topic, pattern, len) != 0) return false;

	return true;
}

Boolean KprMQTTIsValidTopic(const char *topic, Boolean allowWildcard)
{
	UInt32 len = FskStrLen(topic), len2, pos;

	// A topic must be at least one character long.
	if (len == 0) return false;

	// The length is limited to 64k
	if (len > 65535LU) return false;

	if (allowWildcard) {
		char *multi = FskStrChr(topic, kMQTTMultiLevelSeparator), *single;
		if (multi != NULL) {
			len2 = FskStrLen(multi);

			// The multi-level wildcard must be the last character used within the topic tree.
			if (len2 > 1 /* not the last character */) return false;

			if (len > len2 /* not the 1st character */) {
				pos = len - len2 - 1;
				if (topic[pos] != kMQTTTopicLevelSeparator) return false;
			}
		}

		single = FskStrChr(topic, kMQTTSingleLevelSeparator);
		while (single != NULL) {
			len2 = FskStrLen(single);

			// It must be used next to the topic level separator, except when it is specified on its own.
			if (len2 > 1 /* not the last character */) {
				if (single[1] != kMQTTTopicLevelSeparator) return false;
			}

			if (len > len2 /* not the 1st character */) {
				pos = len - len2 - 1;
				if (topic[pos] != kMQTTTopicLevelSeparator) return false;
			}

			single = FskStrChr(single + 1, kMQTTSingleLevelSeparator);
		}

	} else {
		// The multi-level wildcard and single-level wildcard can be used for subscriptions,
		// but they cannot be used within a topic by the publisher of a message.
		if (FskStrChr(topic, kMQTTMultiLevelSeparator) != NULL || FskStrChr(topic, kMQTTSingleLevelSeparator) != NULL) return false;
	}

	return true;
}

FskErr KprMemoryBufferNew(UInt32 size, KprMemoryBuffer *it)
{
	FskErr err = kFskErrNone;
	UInt32 recSize = sizeof(KprMemoryBufferRecord) + size + 1;
	KprMemoryBuffer self;

	bailIfError(FskMemPtrNew(recSize, &self));
	FskMemSet(self, 0, sizeof(KprMemoryBufferRecord));
	self->size = size;
	self->buffer = (char *)self + sizeof(KprMemoryBufferRecord);
	((char *)self->buffer)[size] = 0;
	*it = self;

bail:
	return err;
}

FskErr KprMemoryBufferNewClear(UInt32 size, KprMemoryBuffer *it)
{
	FskErr err;
	KprMemoryBuffer self;

	bailIfError(KprMemoryBufferNew(size, &self));
	FskMemSet(self->buffer, 0, size);
	*it = self;

bail:
	return err;
}

FskErr KprMemoryBufferNewFromData(UInt32 size, void *buffer, KprMemoryBuffer *it)
{
	FskErr err;
	KprMemoryBuffer self;

	bailIfError(KprMemoryBufferNew(size, &self));
	FskMemCopy(self->buffer, buffer, size);
	*it = self;

bail:
	return err;
}

FskErr KprMemoryBufferNewFromString(const char *str, KprMemoryBuffer *it)
{
	FskErr err;
	KprMemoryBuffer self;
	UInt32 size = FskStrLen(str);

	bailIfError(KprMemoryBufferNew(size, &self));
	FskMemCopy(self->buffer, str, size);
	*it = self;

bail:
	return err;
}

FskErr KprMemoryBufferDispose(KprMemoryBuffer self)
{
	if (self) {
		FskMemPtrDispose(self);
	}
	return kFskErrNone;
}

FskErr KprMemoryBufferDisposeAt(KprMemoryBuffer *self)
{
	FskErr err = kFskErrNone;

	if (self && *self != NULL) {
		bailIfError(KprMemoryBufferDispose(*self));
		*self = NULL;
	}
bail:
	return err;
}

FskErr KprMemoryBufferDuplicate(KprMemoryBuffer self, KprMemoryBuffer *it)
{
	return KprMemoryBufferNewFromData(self->size, self->buffer, it);
}

FskErr KprMemoryBufferCopyBuffer(KprMemoryBuffer self, FskMemPtr *it)
{
	return FskMemPtrNewFromData(self->size, self->buffer, it);
}

UInt32 KprMemoryBufferCopyTo(KprMemoryBuffer self, void *dest)
{
	FskMemCopy(dest, self->buffer, self->size);
	return self->size;
}

//--------------------------------------------------
// MQTT Message Delivery
//--------------------------------------------------

static KprMQTTMessage KprMQTTQueueInboxFind(KprMQTTQueue self, UInt16 messageId);

static KprMQTTMessage KprMQTTQueueOutboxFind(KprMQTTQueue self, UInt16 messageId);
static void KprMQTTQueueReschedule(KprMQTTQueue self);
static void KprMQTTQueueTimeToResendMessage(FskTimeCallBack callback, const FskTime time, void *it);
static void KprMQTTQueueResendMessage(KprMQTTQueue self, KprMQTTMessage message);

FskErr KprMQTTQueueNew(KprMQTTQueue *it, UInt16 resendInterval, KprMQTTQueueCallback callback, void *refcon)
{
	FskErr err = kFskErrNone;
	KprMQTTQueue self;

	bailIfError(FskMemPtrNewClear(sizeof(KprMQTTQueueRecord), &self));

	FskTimeCallbackNew(&self->resendCallback);
	bailIfNULL(self->resendCallback);

	self->refcon = refcon;
	self->callback = callback;
	self->resendInterval = resendInterval;
	self->messageId = 1;
	self->pause = true;

	*it = self;
	self = NULL;

bail:
	KprMQTTQueueDispose(self);
	return err;
}

FskErr KprMQTTQueueDispose(KprMQTTQueue self)
{
	if (self) {
		KprMQTTMessage message;

		FskTimeCallbackDispose(self->resendCallback);

		message = self->inbox;
		while (message) {
			KprMQTTMessage tmp = message->next;

			KprMQTTMessageDispose(message);
			message = tmp;
		}

		message = self->outbox;
		while (message) {
			KprMQTTMessage tmp = message->next;

			KprMQTTMessageDispose(message);
			message = tmp;
		}

		FskMemPtrDispose(self);
	}
	return kFskErrNone;
}

void KprMQTTQueueExchange(KprMQTTQueue queue1, KprMQTTQueue queue2)
{
	KprMQTTQueueRecord temp;
	temp.messageId = queue1->messageId;
	temp.inbox = queue1->inbox;
	temp.outbox = queue1->outbox;
	temp.pause = queue1->pause;

	queue1->messageId = queue2->messageId;
	queue1->inbox = queue2->inbox;
	queue1->outbox = queue2->outbox;
	queue1->pause = queue2->pause;

	queue2->messageId = temp.messageId;
	queue2->inbox = temp.inbox;
	queue2->outbox = temp.outbox;
	queue2->pause = temp.pause;
}

// --- MESSAGE QUEUE --------------------------------

FskErr KprMQTTQueueInboxPut(KprMQTTQueue self, KprMQTTMessage *it)
{
	KprMQTTMessage message = *it;

	if (KprMQTTQueueInboxFind(self, message->messageId) != NULL) return kFskErrDuplicateElement;

	FskListAppend(&self->inbox, message);

	*it = NULL;
	return kFskErrNone;
}

static KprMQTTMessage KprMQTTQueueInboxFind(KprMQTTQueue self, UInt16 messageId)
{
	KprMQTTMessage message = self->inbox;

	while (message) {
		if (message->messageId == messageId) return message;
		message = message->next;
	}

	return NULL;
}

KprMQTTMessage KprMQTTQueueInboxGet(KprMQTTQueue self, UInt16 messageId)
{
	KprMQTTMessage message = KprMQTTQueueInboxFind(self, messageId);
	if (!message) return NULL;

	FskListRemove(&self->inbox, message);

	return message;
}

FskErr KprMQTTQueueOutboxPut(KprMQTTQueue self, KprMQTTMessage *it)
{
	KprMQTTMessage message = *it;
	int i;
	double m;

	if (KprMQTTQueueOutboxFind(self, message->messageId) != NULL) return kFskErrDuplicateElement;

	FskTimeGetNow(&message->nextTime);
	m = 1.0;
	for (i = 0; i < message->retryCount; i++) {
		m *= 1.1;
	}
	FskTimeAddSecs(&message->nextTime, self->resendInterval * m);
	message->retryCount += 1;

	FskListAppend(&self->outbox, message);

	KprMQTTQueueReschedule(self);

	*it = NULL;
	return kFskErrNone;
}

static KprMQTTMessage KprMQTTQueueOutboxFind(KprMQTTQueue self, UInt16 messageId)
{
	KprMQTTMessage message = self->outbox;

	while (message) {
		if (message->messageId == messageId) return message;
		message = message->next;
	}

	return NULL;
}

KprMQTTMessage KprMQTTQueueOutboxGet(KprMQTTQueue self, UInt16 messageId)
{
	KprMQTTMessage message = KprMQTTQueueOutboxFind(self, messageId);
	if (!message) return NULL;

	FskListRemove(&self->outbox, message);

	KprMQTTQueueReschedule(self);

	return message;
}

static KprMQTTMessage KprMQTTQueueNextMessageToResend(KprMQTTQueue self)
{
	KprMQTTMessage first, message;

	message = first = self->outbox;
	while (message) {
		if (message != first && FskTimeCompare(&message->nextTime, &first->nextTime) > 0) {
			first = message;
		}
		message = message->next;
	}

	return first;
}

static KprMQTTMessage KprMQTTQueueYoungestUnsentMessageInOutbox(KprMQTTQueue self)
{
	KprMQTTMessage first = NULL, message;

	message = self->outbox;
	while (message) {
		if (!message->duplicateDelivery) {
			if (first == NULL || (message->messageId < first->messageId || (message->messageId == first->messageId && message->type < first->type))) {
				first = message;
			}
		}
		message = message->next;
	}

	return first;
}

static void KprMQTTQueueReschedule(KprMQTTQueue self)
{
	KprMQTTMessage message;

	if (self->pause) return;

	message = KprMQTTQueueNextMessageToResend(self);
	if (message) {
		if (self->outbox != message) {
			FskListRemove(&self->outbox, message);
			FskListPrepend(&self->outbox, message);
		}

		FskTimeCallbackSet(self->resendCallback, &message->nextTime, KprMQTTQueueTimeToResendMessage, self);
	} else {
		FskTimeCallbackRemove(self->resendCallback);
	}
}

static void KprMQTTQueueTimeToResendMessage(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *it)
{
	KprMQTTQueue self = it;
	KprMQTTMessage message = self->outbox;

	if (message) {
		KprMQTTQueueResendMessage(self, message);
	}
}

static void KprMQTTQueueResendMessage(KprMQTTQueue self, KprMQTTMessage message)
{
	FskListRemove(&self->outbox, message);

	self->callback(self, &message, self->refcon);

	KprMQTTMessageDispose(message);
}

UInt16 KprMQTTQueueNextId(KprMQTTQueue self)
{
	UInt16 mid = self->messageId++;
	if (self->messageId == 0) self->messageId++;
	return mid;
}

void KprMQTTQueueStop(KprMQTTQueue self)
{
	if (!self->pause) {
		self->pause = true;
		FskTimeCallbackRemove(self->resendCallback);
	}
}

void KprMQTTQueueStart(KprMQTTQueue self)
{
	KprMQTTMessage message;

	if (self->pause) {
		self->pause = false;

		while (true) {
			message = KprMQTTQueueYoungestUnsentMessageInOutbox(self);
			if (!message) break;

			KprMQTTQueueResendMessage(self, message);
		}
	}
}

#if defined(RUN_UNITTEST) && RUN_UNITTEST

#include "kunit.h"

ku_test(MQTT_topicMatch)
{
	char *ok_patterns[] = {
		"a/b", "a/b",
		"a/b", "a/#",
		"a/b", "+/+",
		"a/b", "#",
		"a/b/c", "a/#",
		"a/b/c", "a/+/c",
		"a/b/c/", "a/+/c/#",
		"a/b/c/d/e/f", "a/+/c/#",
		"/b", "/b",
		"/b", "/#",
		"/b", "+/+",
		"/b", "#",
		"a/", "a/",
		"a/", "a/#",
		"a/", "+/+",
		"a/", "#",
		NULL, NULL,
	};

	char *ng_patterns[] = {
		"a/c", "a/b",
		"a/b/c", "a/+/c/#",
		NULL, NULL,
	};

	char *p1, *p2, **p;

	p = ok_patterns;
	while (p1 = *p++, p2 = *p++, p1) {
		ku_assert(KprMQTTMatchTopic(p1, p2), "Must match %s and %s", p1, p2);
	}

	p = ng_patterns;
	while (p1 = *p++, p2 = *p++, p1) {
		ku_assert(KprMQTTMatchTopic(p1, p2) == false, "Must not match %s and %s", p1, p2);
	}
}

ku_test(MQTT_topicValidCheck)
{
	char *ok_with_wildcard[] = {
		"finance/stock/ibm/#",
		"#",
		"finance/#",

		"+",
		"finance/stock/+",
		"finance/+",
		"finance/+/ibm",
		"+/+",
		"/+",

		NULL,
	};

	char *ng_with_wildcard[] = {
		"finance#",
		"#finance",
		"hello#finance",
		"finance/#/closingprice",
		"finance+",
		"+finance",
		"hello+finance",
		NULL,
	};

	char *ok_without_wildcard[] = {
		"/",
		"//////a",
		"finance/stock/ibm",
		"finance/stock/xyz",
		"finance/stock/ibm/closingprice",
		"finance",
		"/finance",
		NULL,
	};

	char *ng_without_wildcard[] = {
		"",
		NULL,
	};

	char *p, **p0;

	p0 = ok_with_wildcard;
	while (p = *p0++, p) {
		ku_assert(KprMQTTIsValidTopic(p, true), "Valid topic %s, with wildcard", p);
		ku_assert(!KprMQTTIsValidTopic(p, false), "Not valid topic %s, without wildcard", p);
	}

	p0 = ng_with_wildcard;
	while (p = *p0++, p) {
		ku_assert(!KprMQTTIsValidTopic(p, true), "Not valid %s, with wildcard", p);
	}

	p0 = ok_without_wildcard;
	while (p = *p0++, p) {
		ku_assert(KprMQTTIsValidTopic(p, false), "Valid topic %s, without wildcard", p);
	}

	p0 = ng_without_wildcard;
	while (p = *p0++, p) {
		ku_assert(!KprMQTTIsValidTopic(p, false), "Not valid topic %s, without wildcard", p);
	}
}

ku_test(MQTT_common)
{
	ku_run(MQTT_topicMatch);
	ku_run(MQTT_topicValidCheck);
}

#endif

