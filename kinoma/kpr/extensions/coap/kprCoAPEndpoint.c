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
#include "kprCoAPEndpoint.h"

static FskErr KprCoAPEndpointMessageQueueAppend(KprCoAPEndpoint self, KprCoAPMessage message);
static FskErr KprCoAPEndpointMessageQueueRemove(KprCoAPEndpoint self, KprCoAPEndpointMessageQueue entry);
static void KprCoAPEndpointMessageQueueDequeue(KprCoAPEndpoint self, UInt16 messageId);

FskErr KprCoAPEndpointNew(KprCoAPEndpoint *it, FskSocket skt, UInt32 ip, UInt16 port, KprCoAPEndpointCallbacks *callbacks, void *refcon)
{
	FskErr err = kFskErrNone;
	KprCoAPEndpoint self = NULL;

	bailIfError(KprMemPtrNewClear(sizeof(KprCoAPEndpointRecord), &self));
	bailIfError(KprRetainableNew(&self->retainable));

	FskTimeCallbackNew(&self->resendCallback);
	bailIfNULL(self->resendCallback);

	self->ipaddr = ip;
	self->port = port;

	self->socket = skt;
	self->callbacks = *callbacks;
	self->refcon = refcon;

	self->timeout = kKprCoAP_ACK_TIMEOUT * 1000;
	self->timeoutFactor = kKprCoAP_ACK_RANDOM_FACTOR;
	self->maxRetryCount = kKprCoAP_MAX_RETRANSMIT;

	*it = self;

bail:
	if (err) {
		KprCoAPEndpointDispose(self);
	}
	return err;
}

FskErr KprCoAPEndpointDispose(KprCoAPEndpoint self)
{
	if (self && KprRetainableRelease(self->retainable)) {
		FskTimeCallbackDispose(self->resendCallback);

		while (self->resendQueue) {
			KprCoAPEndpointMessageQueueRemove(self, self->resendQueue);
		}

		KprRetainableDispose(self->retainable);
		KprMemPtrDispose(self);
	}
	return kFskErrNone;
}

FskErr KprCoAPEndpointDisposeAt(KprCoAPEndpoint *it)
{
	FskErr err = kFskErrNone;

	if (it) {
		err = KprCoAPEndpointDispose(*it);
		*it = NULL;
	}

	return err;
}

KprCoAPEndpoint KprCoAPEndpointRetain(KprCoAPEndpoint self)
{
	KprRetainableRetain(self->retainable);
	return self;
}

static FskErr KprCoAPEndpointSendMessage_(KprCoAPEndpoint self, KprCoAPMessage message)
{
	FskErr err = kFskErrNone;
	KprMemoryChunk chunk = NULL;
	int sent = 0;

	bailIfError(KprCoAPMessageSerialize(message, &chunk));

	err = FskNetSocketSendUDP(self->socket, KprMemoryChunkStart(chunk), chunk->size, &sent, self->ipaddr, self->port);
	FskTimeGetNow(&self->timestamp);

bail:
	KprMemoryChunkDispose(chunk);
	return err;
}

static Boolean KprCoAPEndpointShouldQueueMessage(KprCoAPMessage message) {
	return message->type == kKprCoAPMessageTypeConfirmable;
}

FskErr KprCoAPEndpointSendMessage(KprCoAPEndpoint self, KprCoAPMessage message)
{
	FskErr err = kFskErrNone;

	bailIfError(KprCoAPEndpointSendMessage_(self, message));
	if (KprCoAPEndpointShouldQueueMessage(message)) {
		bailIfError(KprCoAPEndpointMessageQueueAppend(self, message));
	}

bail:
	return err;
}

Boolean KprCoAPEndpointHandleMessage(KprCoAPEndpoint self, KprCoAPMessage message)
{
	FskTimeGetNow(&self->timestamp);

	if (message->type == kKprCoAPMessageTypeAcknowledgement || message->type == kKprCoAPMessageTypeReset) {
		KprCoAPEndpointMessageQueueDequeue(self, message->messageId);
	}

	if (message->type == kKprCoAPMessageTypeReset) {
		if (self->callbacks.deliveryFailureCallback) {
			self->callbacks.deliveryFailureCallback(self, message, kKprCoAPEndpointDeliveryFailureReset, self->refcon);
		}
		return false;
	}

	return true;
}

void KprCoAPEndpointGetExpireTime(KprCoAPEndpoint self, FskTime time)
{
	FskTimeGetNow(time);
	FskTimeAddMS(time, KprCoAPEndpointGetExchangeLifetime(self) * 1000.0f);
}

/*
 * EXCHANGE_LIFETIME is the time from starting to send a Confirmable
 * message to the time when an acknowledgement is no longer expected,
 * i.e., message-layer information about the message exchange can be
 * purged.  EXCHANGE_LIFETIME includes a MAX_TRANSMIT_SPAN, a
 * MAX_LATENCY forward, PROCESSING_DELAY, and a MAX_LATENCY for the
 * way back.  Note that there is no need to consider
 * MAX_TRANSMIT_WAIT if the configuration is chosen such that the
 * last waiting period (ACK_TIMEOUT * (2 ** MAX_RETRANSMIT) or the
 * difference between MAX_TRANSMIT_SPAN and MAX_TRANSMIT_WAIT) is
 * less than MAX_LATENCY -- which is a likely choice, as MAX_LATENCY
 * is a worst-case value unlikely to be met in the real world.  In
 * this case, EXCHANGE_LIFETIME simplifies to:
 *
 *   MAX_TRANSMIT_SPAN + (2 * MAX_LATENCY) + PROCESSING_DELAY
 *
 * or 247 seconds with the default transmission parameters.
 */
float KprCoAPEndpointGetExchangeLifetime(KprCoAPEndpoint self)
{
	return KprCoAPEndpointGetMaxTransmitSpan(self)
	+ (2 * KprCoAPEndpointGetMaxLatency(self))
	+ KprCoAPEndpointGetProcessingDelay(self);
}

/* MAX_TRANSMIT_SPAN is the maximum time from the first transmission
 * of a Confirmable message to its last retransmission.  For the
 * default transmission parameters, the value is (2+4+8+16)*1.5 = 45
 * seconds, or more generally:
 *
 *   ACK_TIMEOUT * ((2 ** MAX_RETRANSMIT) - 1) * ACK_RANDOM_FACTOR
 */
float KprCoAPEndpointGetMaxTransmitSpan(KprCoAPEndpoint self)
{
	return KprCoAPEndpointGetAckTimeout(self)
	* (powf(2.0f, KprCoAPEndpointGetMaxRetransmit(self)) - 1)
	* KprCoAPEndpointGetAckRandomFactor(self);
}

/* MAX_LATENCY is the maximum time a datagram is expected to take
 * from the start of its transmission to the completion of its
 * reception.  This constant is related to the MSL (Maximum Segment
 * Lifetime) of [RFC0793], which is "arbitrarily defined to be 2
 * minutes" ([RFC0793] glossary, page 81).  Note that this is not
 * necessarily smaller than MAX_TRANSMIT_WAIT, as MAX_LATENCY is not
 * intended to describe a situation when the protocol works well, but
 * the worst-case situation against which the protocol has to guard.
 * We, also arbitrarily, define MAX_LATENCY to be 100 seconds.  Apart
 * from being reasonably realistic for the bulk of configurations as
 * well as close to the historic choice for TCP, this value also
 * allows Message ID lifetime timers to be represented in 8 bits
 * (when measured in seconds).  In these calculations, there is no
 * assumption that the direction of the transmission is irrelevant
 * (i.e., that the network is symmetric); there is just the
 * assumption that the same value can reasonably be used as a maximum
 * value for both directions.  If that is not the case, the following
 * calculations become only slightly more complex.
 */
float KprCoAPEndpointGetMaxLatency(KprCoAPEndpoint self)
{
	return kKprCoAP_MAX_LATENCY;
}

/*
 * PROCESSING_DELAY is the time a node takes to turn around a
 * Confirmable message into an acknowledgement.  We assume the node
 * will attempt to send an ACK before having the sender time out, so
 * as a conservative assumption we set it equal to ACK_TIMEOUT.
 */
float KprCoAPEndpointGetProcessingDelay(KprCoAPEndpoint self)
{
	return KprCoAPEndpointGetAckTimeout(self);
}

float KprCoAPEndpointGetAckTimeout(KprCoAPEndpoint self)
{
	return kKprCoAP_ACK_TIMEOUT;
}

float KprCoAPEndpointGetMaxRetransmit(KprCoAPEndpoint self)
{
	return kKprCoAP_MAX_RETRANSMIT;
}

float KprCoAPEndpointGetAckRandomFactor(KprCoAPEndpoint self)
{
	return kKprCoAP_ACK_RANDOM_FACTOR;
}

// =============================================

static void KprCoAPEndpointMessageQueueReschedule(KprCoAPEndpoint self);
static void KprCoAPEndpointMessageQueueResend(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *it);
static int KprCoAPEndpointMessageQueueCompare(const void *a, const void *b);
static UInt32 KprCoAPEndpointMessageQueueInitialTimeout(KprCoAPEndpoint self);

static FskErr KprCoAPEndpointMessageQueueAppend(KprCoAPEndpoint self, KprCoAPMessage message)
{
	FskErr err = kFskErrNone;
	KprCoAPEndpointMessageQueue entry = NULL;

	bailIfError(KprMemPtrNewClear(sizeof(KprCoAPEndpointMessageQueueRecord), &entry));

	entry->message = KprCoAPMessageRetain(message);
	entry->retryCount = 0;
	entry->interval = KprCoAPEndpointMessageQueueInitialTimeout(self);
	FskTimeGetNow(&entry->nextDelivery);
	FskTimeAddMS(&entry->nextDelivery, entry->interval);

	FskListInsertSorted(&self->resendQueue, entry, KprCoAPEndpointMessageQueueCompare);

	KprCoAPEndpointMessageQueueReschedule(self);

bail:
	if (err) {
		KprMemPtrDispose(entry);
	}

	return err;
}

static void KprCoAPEndpointMessageQueueReschedule(KprCoAPEndpoint self)
{
	if (self->resendQueue) {
		FskDebugStr("next resend(%d) message is %x in %.3f secs", (int) self->resendQueue->retryCount + 1, (int) self->resendQueue->message->messageId, KprCoAPFromNow(&self->resendQueue->nextDelivery));

		FskTimeCallbackSet(self->resendCallback, &self->resendQueue->nextDelivery, KprCoAPEndpointMessageQueueResend, self);
	} else {
		FskTimeCallbackRemove(self->resendCallback);
	}
}

static FskErr KprCoAPEndpointMessageQueueRemove(KprCoAPEndpoint self, KprCoAPEndpointMessageQueue entry)
{
	if (entry) {
		FskListRemove(&self->resendQueue, entry);
		KprCoAPMessageDispose(entry->message);
		KprMemPtrDispose(entry);
	}

	return kFskErrNone;
}

static void KprCoAPEndpointMessageQueueDequeue(KprCoAPEndpoint self, UInt16 messageId)
{
	KprCoAPEndpointMessageQueue entry = self->resendQueue;

	while (entry) {
		if (entry->message->messageId == messageId) {
			KprCoAPEndpointMessageQueueRemove(self, entry);

			KprCoAPEndpointMessageQueueReschedule(self);
			return;
		}

		entry = entry->next;
	}
}

static void KprCoAPEndpointMessageQueueResend(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *it)
{
	FskErr err = kFskErrNone;
	KprCoAPEndpoint self = it;
	KprCoAPEndpointMessageQueue entry = self->resendQueue;

	if (entry) {
		FskListRemove(&self->resendQueue, entry);

		entry->retryCount += 1;

		if (self->callbacks.retryCallback) {
			self->callbacks.retryCallback(self, entry->message, entry->retryCount, self->refcon);
		}

		bailIfError(KprCoAPEndpointSendMessage_(self, entry->message));

		if (entry->retryCount >= self->maxRetryCount) {
			if (self->callbacks.deliveryFailureCallback) {
				self->callbacks.deliveryFailureCallback(self, entry->message, kKprCoAPEndpointDeliveryFailureMaxRetry, self->refcon);
			}
			goto bail;
		}

		entry->interval *= 2;
		FskTimeAddMS(&entry->nextDelivery, entry->interval);

		FskListInsertSorted(&self->resendQueue, entry, KprCoAPEndpointMessageQueueCompare);
		entry = NULL;
	}

bail:
	if (entry) KprCoAPEndpointMessageQueueRemove(self, entry);

	KprCoAPEndpointMessageQueueReschedule(self);

	if (err) {
		if (self->callbacks.errorCallback) {
			self->callbacks.errorCallback(err, "resend error", self->refcon);
		}
	}
}

static int KprCoAPEndpointMessageQueueCompare(const void *a, const void *b)
{
	FskTime t1 = &((KprCoAPEndpointMessageQueue) a)->nextDelivery;
	FskTime t2 = &((KprCoAPEndpointMessageQueue) b)->nextDelivery;
	return FskTimeCompare(t1, t2) * -1;
}

static UInt32 KprCoAPEndpointMessageQueueInitialTimeout(KprCoAPEndpoint self)
{
	/* For a new Confirmable message, the initial timeout is set
	   to a random duration (often not an integral number of seconds)
	   between ACK_TIMEOUT and (ACK_TIMEOUT * ACK_RANDOM_FACTOR)
	 */
	UInt32 timeout = self->timeout;
	UInt32 maxTimeout = self->timeout * self->timeoutFactor;
	return timeout + (FskRandom() % (maxTimeout - timeout));
}

