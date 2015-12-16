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
#include "kprCoAPClientRequest.h"
#include "kprCoAPMessage.h"
#include "kprCoAPEndpoint.h"
#include "kprCoAPReceiver.h"

FskErr KprCoAPClientRequestNew(KprCoAPClientRequest *it, KprCoAPClient client, KprCoAPMessage message, KprCoAPEndpoint endpoint)
{
	FskErr err = kFskErrNone;
	KprCoAPClientRequest self = NULL;
	KprCoAPMessageOptionRecord *option;

	bailIfError(KprMemPtrNewClear(sizeof(KprCoAPClientRequestRecord), &self));

	self->client = client;
	self->message= KprRetain(message);
	self->endpoint = KprRetain(endpoint);

	option = KprCoAPMessageFindOption(message, kKprCoAPMessageOptionObserve);
	self->observeRequested = (option != NULL && option->value.uint == kKprCoAPMessageObserveRegister);

	*it = self;
	FskDebugStr("CoAP ClientRequest created: %p", self);

bail:
	if (err) {
		KprCoAPClientRequestDispose(self);
	}
	return err;
}

FskErr KprCoAPClientRequestDispose(KprCoAPClientRequest self)
{
	FskErr err = kFskErrNone;
	if (self) {
		FskDebugStr("CoAP ClientRequest Disposed: %p", self);

		KprCoAPMessageDispose(self->message);
		KprCoAPEndpointDispose(self->endpoint);

		KprMemPtrDispose(self);
	}
	return err;
}

FskErr KprCoAPClientRequestSendMessage(KprCoAPClientRequest self, KprCoAPMessage message)
{
	return KprCoAPEndpointSendMessage(self->endpoint, message);
}

Boolean KprCoAPClientRequestMatchResponse(KprCoAPClientRequest self, KprCoAPMessage response)
{
	if (response->type == kKprCoAPMessageTypeAcknowledgement || response->type == kKprCoAPMessageTypeReset) {
		return (response->messageId == self->message->messageId);
	}

	if (response->token && response->token->size > 0) {
		if (self->message->token && self->message->token->size == response->token->size) {
			if (KprMemoryChunkIsSame(response->token, self->message->token)) return true;
		}
	}

	return false;
}

Boolean KPrCoAPClientRequestExpectsMoreResponse(KprCoAPClientRequest self)
{
	Boolean hasToken = self->message->token && self->message->token->size > 0;
	Boolean confirmable = self->message->type == kKprCoAPMessageTypeConfirmable;

	if (!confirmable && !hasToken) return false;
	if (!self->responseReceived) return true;
	if (self->observeAccepted) return true;
	return false;
}

FskErr KprCoAPClientRequestHandleResponse(KprCoAPClientRequest self, KprCoAPMessage response)
{
	FskErr err = kFskErrNone;

	if (!KprCoAPEndpointHandleMessage(self->endpoint, response)) goto bail;

	switch (response->type) {
		case kKprCoAPMessageTypeAcknowledgement:
			self->ackReceived = true;
			if (response->code != kKprCoAPMessageCodeEmpty) {
				self->responseReceived = true;
			}
			break;

		case kKprCoAPMessageTypeReset:
			KprCoAPClientEndRequest(self->client, self, kKprCoAPClientRequestEndReasonReset);
			return kFskErrNone;

		default:
			self->responseReceived = true;
			break;
	}

	if (self->observeRequested && !self->observeAccepted) {
		KprCoAPMessageOptionRecord *option = KprCoAPMessageFindOption(response, kKprCoAPMessageOptionObserve);
		self->observeAccepted = (option != NULL && option->value.uint > kKprCoAPMessageObserveDeregister);
	}

	bailIfError(KprCoAPClientHandleResponse(self->client, self->message, response, self->endpoint));

bail:
	return err;
}

