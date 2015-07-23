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
#include "kprCoAPReceiver.h"

static void KprCoAPReceiverGetStates(KprSocketReaderState **states, UInt32 *count);

static void KprCoAPReceiver_readError(KprSocketErrorContext context, FskErr err, void *refcon);

enum {
	kKprCoAPReceiver_parseMessage,
};

FskErr KprCoAPReceiverNew(KprCoAPReceiver *it, FskSocket skt, KprCoAPReceiverCallbacks *callbacks, void *refcon)
{
	FskErr err = kFskErrNone;
	KprCoAPReceiver self = NULL;
	KprSocketReaderState *states;
	UInt32 stateCount;

	bailIfError(KprMemPtrNewClear(sizeof(KprCoAPReceiverRecord), &self));

	FskNetSocketReceiveBufferSetSize(skt, kKprCoAPReceiverBufferSize);

	KprCoAPReceiverGetStates(&states, &stateCount);
	bailIfError(KprSocketReaderNew(&self->reader, skt, states, stateCount, self));

	self->reader->errorCallback = KprCoAPReceiver_readError;

	KprSocketReaderSetState(self->reader, kKprCoAPReceiver_parseMessage);

	self->socket = skt;
	self->callbacks = *callbacks;
	self->refcon = refcon;

	*it = self;

bail:
	if (err) {
		KprCoAPReceiverDispose(self);
	}
	return err;
}

FskErr KprCoAPReceiverDispose(KprCoAPReceiver self)
{
	if (self) {
		KprSocketReaderDispose(self->reader);

		KprMemPtrDispose(self);
	}
	return kFskErrNone;
}

static void KprCoAPReceiver_readError(KprSocketErrorContext context, FskErr err, void *refcon)
{
	KprCoAPReceiver self = refcon;
	self->callbacks.errorCallback(err, "receive error", self->refcon);
}

// ==============================================

static FskErr KprCoAPReceiver_parseMessage(KprSocketReader reader, void *refcon)
{
	FskErr err = kFskErrNone;
	KprCoAPReceiver self = (KprCoAPReceiver) refcon;
	UInt32 size = kKprCoAPReceiverBufferSize;
	KprCoAPMessage message = NULL;
	UInt32 remoteAddr;
	UInt16 remotePort;

	err = KprSocketReaderReadDataFrom(reader, self->buffer, &size, &remoteAddr, &remotePort);
	if (err) return err;

	err = KprCoAPMessageDeserialize(self->buffer, size, &message);
	if (err) return err;

	self->callbacks.receiveCallback(message, remoteAddr, remotePort, self->refcon);

	KprCoAPMessageDispose(message);
	return err;
}

static void KprCoAPReceiverGetStates(KprSocketReaderState **states, UInt32 *count) {
	static KprSocketReaderState kStates[] = {
		{ kKprCoAPReceiver_parseMessage, KprCoAPReceiver_parseMessage },
	};

	*states = kStates;
	*count = sizeof(kStates) / sizeof(KprSocketReaderState);
}

