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
#include "kprCoAPServerSession.h"
#include "kprCoAPServer.h"
#include "kprCoAPReceiver.h"
#include "kprCoAPEndpoint.h"


FskErr KprCoAPServerSessionNew(KprCoAPServer server, KprCoAPMessage request, KprCoAPEndpoint endpoint, KprCoAPServerSession *it)
{
	FskErr err = kFskErrNone;
	KprCoAPServerSession self = NULL;
	KprCoAPMessageOptionRecord *option;

	bailIfError(KprMemPtrNewClear(sizeof(KprCoAPServerSessionRecord), &self));
	bailIfError(KprRetainableNew(&self->retainable));

	self->server = server;
	self->sessionId = KprCoAPServerNextSessionId(server);
	self->endpoint = KprRetain(endpoint);
	self->request = KprRetain(request);
	self->autoAck = true;
	self->confiramableRequest = KprCoAPMessageIsConfirmable(request);

	KprCoAPEndpointGetExpireTime(endpoint, &self->expireAt);

	option = KprCoAPMessageFindOption(request, kKprCoAPMessageOptionObserve);
	self->observeRequested = (option != NULL && option->value.uint == kKprCoAPMessageObserveRegister);

	bailIfError(KprCoAPMessageBuildUri(self->request));

	*it = self;

bail:
	if (err) {
		KprCoAPServerSessionDispose(self);
	}
	return err;
}

FskErr KprCoAPServerSessionDispose(KprCoAPServerSession self)
{
	FskErr err = kFskErrNone;
	if (self && KprRetainableRelease(self->retainable)) {
		FskDebugStr("CoAP ServerSession Disposed: %x", (int) self);

		KprCoAPMessageDispose(self->request);
		KprCoAPMessageDispose(self->lastResponse);
		KprCoAPEndpointDispose(self->endpoint);

		KprRetainableDispose(self->retainable);

		KprMemPtrDispose(self);
	}
	return err;
}

KprCoAPServerSession KprCoAPServerSessionRetain(KprCoAPServerSession self)
{
	KprRetainableRetain(self->retainable);
	return self;
}

Boolean KprCoAPServerSessionCompareWith(KprCoAPServerSession self, KprCoAPMessage request, KprCoAPEndpoint endpoint)
{
	if (self->endpoint != endpoint) return false;
	if (self->request->messageId != request->messageId) return false;
	return true;
}

FskErr KprCoAPServerSessionCreateResponse(KprCoAPServerSession self, KprCoAPMessage *it)
{
	FskErr err = kFskErrNone;
	KprCoAPMessage response = NULL;

	bailIfError(KprCoAPMessageCopy(self->request, &response));

	response->type = self->request->type;
	response->code = kKprCoAPMessageCodeCreated;
	response->messageId = 0;

	*it = response;
bail:
	return err;
}

FskErr KprCoAPServerSessionSendAck(KprCoAPServerSession self)
{
	FskErr err = kFskErrNone;
	KprCoAPMessage response;
	KprCoAPServer server = self->server;

	if (!server || self->responded || self->emptyAckSent || !self->confiramableRequest) {
		return kFskErrBadState;
	}

	bailIfError(KprCoAPMessageNew(&response));
	response->type = kKprCoAPMessageTypeAcknowledgement;
	response->messageId = self->request->messageId;

	bailIfError(KprCoAPEndpointSendMessage(self->endpoint, response));

	self->emptyAckSent = true;

bail:
	KprCoAPMessageDispose(response);
	return err;
}

FskErr KprCoAPServerSessionSendResponse(KprCoAPServerSession self, KprCoAPMessage response)
{
	FskErr err = kFskErrNone;
	KprCoAPServer server = self->server;

	if (!server) {
		return kFskErrBadState;
	}

	if (self->responded && !self->observeAccepted) {
		return kFskErrBadState;
	}

	if (self->confiramableRequest) {
		if (!self->emptyAckSent && !self->responded) {
			response->messageId = self->request->messageId;
			response->type = kKprCoAPMessageTypeAcknowledgement;
		}
	}

	if (!response->messageId) {
		response->messageId = KprCoAPServerGenerateMessageId(server);
	}

	if (self->observeAccepted) {
		bailIfError(KprCoAPMessageAppendUintOption(response, kKprCoAPMessageOptionObserve, self->observeId));
		self->observeId += 1;
	}

	bailIfError(KprCoAPEndpointSendMessage(self->endpoint, response));

	KprCoAPMessageRetain(response);
	KprCoAPMessageDispose(self->lastResponse);
	self->lastResponse = response;

	self->responded = true;

	if (!self->observeAccepted) {
		KprCoAPServerFinishSession(server, self);
	}

bail:
	return err;
}

FskErr KprCoAPServerSessionAcceptObserve(KprCoAPServerSession self)
{
	if (!self->observeRequested || self->observeAccepted) return kFskErrBadState;
	self->observeAccepted = true;
	self->observeId = 2;
	return kFskErrNone;
}

FskErr KprCoAPServerSessionEndObserve(KprCoAPServerSession self)
{
	if (!self->observeAccepted) return kFskErrBadState;
	self->observeRequested = self->observeAccepted = false;
	return kFskErrNone;
}

static FskErr KprCoAPServerSessionSendErrorResponse(KprCoAPServerSession self, KprCoAPMessageCode code, const char *message)
{
	FskErr err = kFskErrNone;
	KprCoAPMessage response = NULL;

	bailIfError(KprCoAPServerSessionCreateResponse(self, &response));

	response->code = code;

	if (message) {
		int len = FskStrLen(message);
		bailIfError(KprCoAPMessageSetPayload(response, message, len));
	}

	bailIfError(KprCoAPServerSessionSendResponse(self, response));

bail:
	KprCoAPMessageDispose(response);
	return err;
}

FskErr KprCoAPServerSessionRun(KprCoAPServerSession self)
{
	FskErr err = kFskErrNone;
	KprCoAPServer server = self->server;

	err = server->callbacks.resourceCallback(self, server->refcon);
	if (err != kFskErrNone) {
		KprCoAPMessageCode code = (err == kFskErrNotFound) ? kKprCoAPMessageCodeNotFound : kKprCoAPMessageCodeInternalServerError;

		KprCoAPServerSessionEndObserve(self);

		KprCoAPServerSessionSendErrorResponse(self, code, NULL);
		err = kFskErrNone;
	}

	if (!self->responded && self->confiramableRequest && self->autoAck) {
		bailIfError(KprCoAPServerSessionSendAck(self));
	}

bail:
	return err;
}

FskErr KprCoAPServerSessionRunAgain(KprCoAPServerSession self)
{
	FskErr err = kFskErrNone;

	if (self->lastResponse) {
		if (self->confiramableRequest && self->emptyAckSent) {
			bailIfError(KprCoAPServerSessionSendAck(self));
		}

		bailIfError(KprCoAPEndpointSendMessage(self->endpoint, self->lastResponse));
	} else {
		if (self->confiramableRequest) {
			bailIfError(KprCoAPServerSessionSendAck(self));
		}
	}

bail:
	return err;
}
