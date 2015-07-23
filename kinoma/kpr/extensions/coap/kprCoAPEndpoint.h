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
#ifndef __KPRCOAPENDPOINT__
#define __KPRCOAPENDPOINT__

#include "kpr.h"
#include "kprCoAPCommon.h"
#include "kprCoAPMessage.h"
#include "FskNetUtils.h"
#include "kprUtilities.h"

//--------------------------------------------------
// Typedefs
//--------------------------------------------------

typedef enum {
	kKprCoAPEndpointDeliveryFailureMaxRetry = 1,
	kKprCoAPEndpointDeliveryFailureReset = 2,
} KprCoAPEndpointDeliveryFailure;

typedef struct KprCoAPEndpointCallbacks KprCoAPEndpointCallbacks;

typedef void (*KprCoAPEndpointRetryCallback)(KprCoAPEndpoint endpoint, KprCoAPMessage message, UInt32 retryCount, void *refcon);
typedef void (*KprCoAPEndpointErrorCallback)(FskErr err, const char *reason, void *refcon);
typedef void (*KprCoAPEndpointDeliveryFailureCallback)(KprCoAPEndpoint endpoint, KprCoAPMessage message, KprCoAPEndpointDeliveryFailure failure, void *refcon);

struct KprCoAPEndpointCallbacks {
	KprCoAPEndpointRetryCallback retryCallback;
	KprCoAPEndpointErrorCallback errorCallback;
	KprCoAPEndpointDeliveryFailureCallback deliveryFailureCallback;
};

typedef struct KprCoAPEndpointMessageQueueRecord KprCoAPEndpointMessageQueueRecord, *KprCoAPEndpointMessageQueue;

#define kKprCoAPEndpointBufferSize (64 * 1024L)

struct KprCoAPEndpointRecord {
	KprCoAPEndpoint next;
	
	KprRetainable retainable;

	FskSocket socket;
	UInt32 ipaddr;
	UInt16 port;
//	UInt16 remotePort;

	UInt32 timeout;
	double timeoutFactor;
	UInt32 maxRetryCount;

	KprCoAPMessageQueue waiting; // waiting endpoint ready.

	KprCoAPEndpointMessageQueue resendQueue;
	FskTimeCallBack resendCallback;
	FskTimeRecord timestamp;

	KprCoAPEndpointCallbacks callbacks;
	void *refcon;
};

struct KprCoAPEndpointMessageQueueRecord {
	KprCoAPEndpointMessageQueue next;
	KprCoAPMessage message;

	FskTimeRecord nextDelivery;
	UInt32 interval;
	UInt32 retryCount;
};

//--------------------------------------------------
// Methods
//--------------------------------------------------

FskErr KprCoAPEndpointNew(KprCoAPEndpoint *it, FskSocket skt, UInt32 ip, UInt16 port, KprCoAPEndpointCallbacks *callbacks, void *refcon);
FskErr KprCoAPEndpointDispose(KprCoAPEndpoint self);
FskErr KprCoAPEndpointDisposeAt(KprCoAPEndpoint *it);
KprCoAPEndpoint KprCoAPEndpointRetain(KprCoAPEndpoint self);

FskErr KprCoAPEndpointSendMessage(KprCoAPEndpoint self, KprCoAPMessage message);

Boolean KprCoAPEndpointHandleMessage(KprCoAPEndpoint self, KprCoAPMessage message);

void KprCoAPEndpointGetExpireTime(KprCoAPEndpoint self, FskTime time);

float KprCoAPEndpointGetExchangeLifetime(KprCoAPEndpoint self);
float KprCoAPEndpointGetMaxTransmitSpan(KprCoAPEndpoint self);
float KprCoAPEndpointGetMaxLatency(KprCoAPEndpoint self);
float KprCoAPEndpointGetProcessingDelay(KprCoAPEndpoint self);
float KprCoAPEndpointGetAckTimeout(KprCoAPEndpoint self);
float KprCoAPEndpointGetMaxRetransmit(KprCoAPEndpoint self);
float KprCoAPEndpointGetAckRandomFactor(KprCoAPEndpoint self);

#endif
