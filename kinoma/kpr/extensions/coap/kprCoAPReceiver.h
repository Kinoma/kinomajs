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
#ifndef __KPRCOAPRECEIVER__
#define __KPRCOAPRECEIVER__

#include "kpr.h"
#include "kprCoAPCommon.h"
#include "kprCoAPMessage.h"
#include "FskNetUtils.h"
#include "kprUtilities.h"

//--------------------------------------------------
// Typedefs
//--------------------------------------------------

typedef struct KprCoAPReceiverCallbacks KprCoAPReceiverCallbacks;

typedef void (*KprCoAPReceiverReceiveCallback)(KprCoAPMessage message, UInt32 remoteAddr, UInt16 remotePort, void *refcon);
typedef void (*KprCoAPReceiverErrorCallback)(FskErr err, const char *reason, void *refcon);

struct KprCoAPReceiverCallbacks {
	KprCoAPReceiverReceiveCallback receiveCallback;
	KprCoAPReceiverErrorCallback errorCallback;
};

#define kKprCoAPReceiverBufferSize (64 * 1024L)

struct KprCoAPReceiverRecord {
	FskSocket socket;

	KprSocketReader reader;
	UInt8 buffer[kKprCoAPReceiverBufferSize];

	KprCoAPReceiverCallbacks callbacks;
	void *refcon;
};

//--------------------------------------------------
// Methods
//--------------------------------------------------

FskErr KprCoAPReceiverNew(KprCoAPReceiver *it, FskSocket skt, KprCoAPReceiverCallbacks *callbacks, void *refcon);
FskErr KprCoAPReceiverDispose(KprCoAPReceiver self);

#endif
