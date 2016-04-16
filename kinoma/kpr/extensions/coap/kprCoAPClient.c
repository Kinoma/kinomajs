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
#include "kprCoAPClient.h"
#include "kprCoAPMessage.h"
#include "kprCoAPEndpoint.h"
#include "kprCoAPReceiver.h"
#include "kprCoAPClientResolver.h"
#include "kprCoAPClientRequest.h"
#include "FskEndian.h"

const char *kKprCoAPClientRequestEndReasonSuccess = "success";
const char *kKprCoAPClientRequestEndReasonReset = "reset";


static UInt16 KprCoAPClientGenerateMessageId(KprCoAPClient self);

static KprCoAPEndpoint KprCoAPClientFindEndpoint(KprCoAPClient self, UInt32 ipaddr, UInt16 port);
static FskErr KprCoAPClientCreateEndpoint(KprCoAPClient self, UInt32 ipaddr, UInt16 port, KprCoAPEndpoint *it);
static FskErr KprCoAPClientGetEndpoint(KprCoAPClient self, UInt32 ipaddr, UInt16 port, KprCoAPEndpoint *it);
static void KprCoAPClientDisposeUnusedEndpoint(KprCoAPClient self);


static void KprCoAPClient_receiveCallback(KprCoAPMessage message, UInt32 remoteAddr, UInt16 remotePort, void *refcon);
static void KprCoAPClient_errorCallback(FskErr err, const char *reason, void *refcon);

static void KprCoAPClient_retryCallback(KprCoAPEndpoint endpoint, KprCoAPMessage message, UInt32 retryCount, void *refcon);
static void KprCoAPClient_deliveryErrorCallback(KprCoAPEndpoint endpoint, KprCoAPMessage message, KprCoAPEndpointDeliveryFailure failure, void *refcon);


FskErr KprCoAPClientNew(KprCoAPClient *it, KprCoAPClientCallbacks *callbacks, void *refcon)
{
	FskErr err = kFskErrNone;
	KprCoAPClient self = NULL;
	FskSocket socket = NULL;
	KprCoAPReceiver	receiver = NULL;
	KprCoAPReceiverCallbacks receiverCallback = {
		KprCoAPClient_receiveCallback,
		KprCoAPClient_errorCallback,
	};

	bailIfError(KprMemPtrNewClear(sizeof(KprCoAPClientRecord), &self));

	bailIfError(FskNetSocketNewUDP(&socket, "KprCoAPClient"));
	bailIfError(FskNetSocketBind(socket, -1, 0));

	bailIfError(KprCoAPReceiverNew(&receiver, socket, &receiverCallback, self));

	self->socket = socket;
	self->receiver = receiver;
	self->messageId = FskRandom();
	self->autoToken = false;
	self->nextTokenBytes = 1;
	self->callbacks = *callbacks;
	self->refcon = refcon;

	*it = self;

bail:
	if (err) {
		FskNetSocketClose(socket);
		KprCoAPReceiverDispose(receiver);
		KprCoAPClientDispose(self);
	}

	return err;
}

FskErr KprCoAPClientDispose(KprCoAPClient self)
{
	FskErr err = kFskErrNone;
	if (self) {
		KprCoAPClientResolver resolver;
		KprCoAPClientRequest request;
		KprCoAPEndpoint endpoint;

		resolver = self->resolvers;
		while (resolver) {
			KprCoAPClientResolver next = resolver->next;
			KprCoAPClientResolverDispose(resolver);
			resolver = next;
		}

		request = self->requests;
		while (request) {
			KprCoAPClientRequest next = request->next;
			KprCoAPClientRequestDispose(request);
			request = next;
		}

		endpoint = self->endpoints;
		while (endpoint) {
			KprCoAPEndpoint next = endpoint->next;
			KprCoAPEndpointDispose(endpoint);
			endpoint = next;
		}

		KprMemoryBlockDispose(self->recycleTokens);

		KprCoAPReceiverDispose(self->receiver);
		FskNetSocketClose(self->socket);
		KprMemPtrDispose(self);
	}
	return err;
}

FskErr KprCoAPClientCreateRequestMessage(KprCoAPClient self, const char *uri, KprCoAPRequestMethod method, Boolean confirmable, KprCoAPMessage *requestOut)
{
	FskErr err = kFskErrNone;
	KprCoAPMessage request = NULL;

	bailIfError(KprCoAPMessageNew(&request));
	bailIfError(KprCoAPMessageParseUri(request, uri));

	request->code = KprCoAPMessageCodeWith(0, method);
	request->messageId = KprCoAPClientGenerateMessageId(self);
	if (confirmable) {
		request->type = kKprCoAPMessageTypeConfirmable;
	} else {
		request->type = kKprCoAPMessageTypeNonConfirmable;
	}

	*requestOut = request;

bail:
	if (err) {
		KprCoAPMessageDispose(request);
	}

	return err;
}

FskErr KprCoAPClientSendRequest(KprCoAPClient self, KprCoAPMessage message)
{
	FskErr err = kFskErrNone;
	const char *host = message->host;
	UInt16 port = message->port;
	KprCoAPClientResolver dest = self->resolvers;

	while (dest) {
		if (port == dest->port && FskStrCompareCaseInsensitive(host, dest->host) == 0) break;

		dest = dest->next;
	}

	if (dest) {
		if (KprCoAPClientResolverIsResolved(dest)) {
			err = KprCoAPClientStartRequest(self, dest->ipaddr, port, message);
		} else {
			err = KprCoAPClientResolverQueueMessage(dest, message);
		}
	} else {
		bailIfError(KprCoAPClientResolverNew(&dest, self, host, port, message));
		FskListAppend(&self->resolvers, dest);
	}

bail:
	return err;
}

FskErr KprCoAPClientHandleResponse(KprCoAPClient self, KprCoAPMessage request, KprCoAPMessage response, KprCoAPEndpoint endpoint)
{
	KprCoAPMessage ack = NULL;
	FskErr err = kFskErrNone;

	if (request && response->type == kKprCoAPMessageTypeAcknowledgement) {
		bailIfError(self->callbacks.acknowledgementCallback(request, self->refcon));
	}

	if (response->code != kKprCoAPMessageCodeEmpty) {
		bailIfError(self->callbacks.responseCallback(request, response, self->refcon));
		if (response->type == kKprCoAPMessageTypeConfirmable) {
			bailIfError(KprCoAPMessageNew(&ack));

			ack->type = kKprCoAPMessageTypeAcknowledgement;
			ack->messageId = response->messageId;

			err = KprCoAPEndpointSendMessage(endpoint, ack);
		}
	}

bail:
	KprCoAPMessageDispose(ack);
	return err;
}

static FskErr KprCoAPClientDispatchResponse(KprCoAPClient self, UInt32 ipaddr, UInt16 port, KprCoAPMessage response)
{
	FskErr err = kFskErrNone;
	KprCoAPClientRequest request = self->requests;
	KprCoAPEndpoint endpoint = NULL;

	while (request) {
		if (KprCoAPClientRequestMatchResponse(request, response)) {
			err = KprCoAPClientRequestHandleResponse(request, response);
			if (KPrCoAPClientRequestExpectsMoreResponse(request) == false) {
				KprCoAPClientEndRequest(self, request, kKprCoAPClientRequestEndReasonSuccess);
			}
			return err;
		}

		request = request->next;
	}

	endpoint = KprCoAPClientFindEndpoint(self, ipaddr, port);
	if (endpoint) {
		if (!KprCoAPEndpointHandleMessage(endpoint, response)) goto bail;

		bailIfError(KprCoAPClientHandleResponse(self, NULL, response, endpoint));
	} else {
		// @TODO
	}

bail:
	return err;
}

void KprCoAPClientReportError(KprCoAPClient self, FskErr err, const char *reason)
{
	self->callbacks.errorCallback(err, reason, self->refcon);
}

static UInt16 KprCoAPClientGenerateMessageId(KprCoAPClient self)
{
	UInt16 messageId = self->messageId++;
	if (messageId == 0) messageId = 1;
	return messageId;
}

static void KprCoAPClient_receiveCallback(KprCoAPMessage message, UInt32 remoteAddr, UInt16 remotePort, void *refcon)
{
	KprCoAPClient self = (KprCoAPClient) refcon;
	FskErr err;


	err = KprCoAPClientDispatchResponse(self, remoteAddr, remotePort, message);

	if (err) {
		KprCoAPClientReportError(self, err, "response handling");
	}
}

static void KprCoAPClient_errorCallback(FskErr err, const char *reason, void *refcon)
{
	KprCoAPClient self = (KprCoAPClient) refcon;
	KprCoAPClientReportError(self, err, reason);
}

static KprCoAPEndpoint KprCoAPClientFindEndpoint(KprCoAPClient self, UInt32 ipaddr, UInt16 port)
{
	KprCoAPEndpoint endpoint = self->endpoints;

	while (endpoint) {
		if (endpoint->ipaddr == ipaddr && endpoint->port == port) return endpoint;
		endpoint = endpoint->next;
	}

	return NULL;
}

static FskErr KprCoAPClientCreateEndpoint(KprCoAPClient self, UInt32 ipaddr, UInt16 port, KprCoAPEndpoint *it)
{
	FskErr err = kFskErrNone;
	KprCoAPEndpoint endpoint = NULL;
	KprCoAPEndpointCallbacks endpointCallback = {
		KprCoAPClient_retryCallback,
		KprCoAPClient_errorCallback,
		KprCoAPClient_deliveryErrorCallback,
	};

	bailIfError(KprCoAPEndpointNew(&endpoint, self->socket, ipaddr, port, &endpointCallback, self));

	FskListAppend(&self->endpoints, endpoint);
	*it = endpoint;

bail:
	if (err) {
		KprCoAPEndpointDispose(endpoint);
	}

	return err;
}

static FskErr KprCoAPClientGetEndpoint(KprCoAPClient self, UInt32 ipaddr, UInt16 port, KprCoAPEndpoint *it)
{
	FskErr err = kFskErrNone;
	KprCoAPEndpoint endpoint = NULL;

	endpoint = KprCoAPClientFindEndpoint(self, ipaddr, port);
	if (endpoint == NULL) {
		bailIfError(KprCoAPClientCreateEndpoint(self, ipaddr, port, &endpoint));
	}
	*it = endpoint;

bail:
	return err;
}

static void KprCoAPClientDisposeUnusedEndpoint(KprCoAPClient self)
{
	// @TODO
}

static FskErr KprCoAPClientNextAutoToken(KprCoAPClient self, KprMemoryBlock *token)
{
	FskErr err = kFskErrNone;
	UInt32 max, value;

	if (self->recycleTokens) {
		*token = FskListRemoveFirst(&self->recycleTokens);
		(void)KprRetain(*token);
		return kFskErrNone;
	}

	switch (self->nextTokenBytes) {
		case 1:
			max = 0xffU;
			break;

		case 2:
			max = 0xffffU;
			break;

		case 3:
			max = 0xffffffU;
			break;

		case 4:
			max = 0xffffffffU;
			break;

		default:
			return kFskErrMemFull;
	}

	value = self->nextTokenId;
	value = FskEndianU32_NtoL(value);

	bailIfError(KprMemoryBlockNew(self->nextTokenBytes, &value, token));

	if (self->nextTokenId >= max) {
		self->nextTokenBytes += 1;
		self->nextTokenId = 0;
	} else {
		self->nextTokenId += 1;
	}

bail:
	return err;
}

FskErr KprCoAPClientStartRequest(KprCoAPClient self, UInt32 ipaddr, UInt16 port, KprCoAPMessage message)
{
	FskErr err = kFskErrNone;
	KprCoAPClientRequest request = NULL;
	KprCoAPEndpoint endpoint = NULL;
	KprMemoryBlock generatedToken = NULL;

	if (self->autoToken && message->token == NULL) {
		bailIfError(KprCoAPClientNextAutoToken(self, &generatedToken));
		message->token = KprRetain(generatedToken);
		KprMemoryBlockDispose(generatedToken);
	}

	bailIfError(KprCoAPClientGetEndpoint(self, ipaddr, port, &endpoint));

	bailIfError(KprCoAPClientRequestNew(&request, self, message, endpoint));
	FskListAppend(&self->requests, request);

	bailIfError(KprCoAPEndpointSendMessage(endpoint, message));

bail:
	if (err) {
		FskListRemove(&self->requests, request);
		KprCoAPClientRequestDispose(request);

		KprCoAPClientDisposeUnusedEndpoint(self);
	}
	return err;
}

FskErr KprCoAPClientEndRequest(KprCoAPClient self, KprCoAPClientRequest request, const char *reason)
{
	FskErr err = kFskErrNone;

	if (self->callbacks.requestEndCallback) {
		bailIfError(self->callbacks.requestEndCallback(request->message, reason, self->refcon));
	}

	FskListRemove(&self->requests, request);

	if (request->message->token) {
		FskListAppend(&self->recycleTokens, KprRetain(request->message->token));
	}

	bailIfError(KprCoAPClientRequestDispose(request));

bail:
	return err;
}

static void KprCoAPClient_retryCallback(KprCoAPEndpoint endpoint, KprCoAPMessage request, UInt32 retryCount, void *refcon)
{
		FskErr err = kFskErrNone;
	KprCoAPClient self = (KprCoAPClient) refcon;
	err = self->callbacks.retryCallback(request, retryCount, self->refcon);

	if (err != kFskErrNone) {
		KprCoAPClientReportError(self, err, "retry callback error");
	}
}

static void KprCoAPClient_deliveryErrorCallback(KprCoAPEndpoint endpoint, KprCoAPMessage message, KprCoAPEndpointDeliveryFailure failure, void *refcon)
{
	FskErr err = kFskErrNone;
	KprCoAPClient self = (KprCoAPClient) refcon;
	char *reason;
	KprCoAPClientRequest request;

	switch (failure) {
		case kKprCoAPEndpointDeliveryFailureMaxRetry:
			reason = "max retry";
			break;

		case kKprCoAPEndpointDeliveryFailureReset:
			reason = "reset by peer";
			break;

		default:
			reason = "unknown";
			break;

	}

	err = self->callbacks.deliveryFailureCallback(message, reason, self->refcon);

	if (err != kFskErrNone) {
		KprCoAPClientReportError(self, err, "retry delivery error");
	}

	request = self->requests;

	while (request) {
		if (request->message == message) {
			KprCoAPClientEndRequest(self, request, reason);
			break;
		}

		request = request->next;
	}
}

