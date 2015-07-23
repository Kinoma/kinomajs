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
#ifndef __KPRCOAPCLIENT__
#define __KPRCOAPCLIENT__

#include "kpr.h"
#include "kprCoAPCommon.h"
#include "FskNetUtils.h"
#include "kprCoAPClientResolver.h"

//--------------------------------------------------
// Typedefs
//--------------------------------------------------

typedef struct KprCoAPClientCallbacks KprCoAPClientCallbacks;
typedef struct KprCoAPClientRequestRecord KprCoAPClientRequestRecord, *KprCoAPClientRequest;

typedef FskErr (*KprCoAPClientResponseCallback)(KprCoAPMessage request, KprCoAPMessage response, void *refcon);
typedef FskErr (*KprCoAPClientAcknowledgementCallback)(KprCoAPMessage request, void *refcon);
typedef FskErr (*KprCoAPClientDeliveryFailureCallback)(KprCoAPMessage request, const char *reason, void *refcon);
typedef FskErr (*KprCoAPClientRetryCallback)(KprCoAPMessage request, int count, void *refcon);
typedef FskErr (*KprCoAPClientRequestEndCallback)(KprCoAPMessage request, const char *reason, void *refcon);
typedef void (*KprCoAPClientErrorCallback)(FskErr err, const char *reason, void *refcon);

struct KprCoAPClientCallbacks {
	KprCoAPClientResponseCallback responseCallback;
	KprCoAPClientAcknowledgementCallback acknowledgementCallback;
	KprCoAPClientRetryCallback retryCallback;
	KprCoAPClientDeliveryFailureCallback deliveryFailureCallback;
	KprCoAPClientRequestEndCallback requestEndCallback;
	KprCoAPClientErrorCallback errorCallback;
};

struct KprCoAPClientRecord {
	FskSocket socket;
	KprCoAPReceiver receiver;
	KprCoAPClientResolver resolvers;
	KprCoAPEndpoint endpoints;
	KprCoAPClientRequest requests;

	UInt16 messageId;

	Boolean autoToken;
	KprMemoryChunk recycleTokens;
	UInt32 nextTokenId;
	UInt32 nextTokenBytes;

	KprCoAPClientCallbacks callbacks;
	void *refcon;
};

//--------------------------------------------------
// Methods
//--------------------------------------------------

FskErr KprCoAPClientNew(KprCoAPClient *it, KprCoAPClientCallbacks *callbacks, void *refcon);
FskErr KprCoAPClientDispose(KprCoAPClient self);

FskErr KprCoAPClientCreateRequestMessage(KprCoAPClient self, const char *uri, KprCoAPRequestMethod method, Boolean confirmable, KprCoAPMessage *request);

FskErr KprCoAPClientSendRequest(KprCoAPClient self, KprCoAPMessage request);

FskErr KprCoAPClientCancel(KprCoAPClient self, UInt16 messageId);

FskErr KprCoAPClientHandleResponse(KprCoAPClient self, KprCoAPMessage request, KprCoAPMessage response, KprCoAPEndpoint endpoint);
void KprCoAPClientReportError(KprCoAPClient self, FskErr err, const char *reason);

FskErr KprCoAPClientStartRequest(KprCoAPClient self, UInt32 ipaddr, UInt16 port, KprCoAPMessage message);
FskErr KprCoAPClientEndRequest(KprCoAPClient self, KprCoAPClientRequest request, const char *reason);

extern const char *kKprCoAPClientRequestEndReasonSuccess;
extern const char *kKprCoAPClientRequestEndReasonReset;

#endif
