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
#ifndef __KPRMQTTENDPOINT__
#define __KPRMQTTENDPOINT__

#include "kprMQTTCommon.h"
#include "kprUtilities.h"

//--------------------------------------------------
// MQTT Endpoint
//--------------------------------------------------

typedef struct KprMQTTEndpointRecord KprMQTTEndpointRecord, *KprMQTTEndpoint;

// Callbacks

typedef void (*KprMQTTEndpointMessageCallback)(KprMQTTEndpoint self, KprMQTTMessage message, void *refcon);
typedef void (*KprMQTTEndpointErrorCallback)(KprMQTTEndpoint self, FskErr err, char *message, void *refcon);


struct KprMQTTEndpointRecord {
	FskSocket socket;
	KprMQTTProtocolVersion protocolVersion;
	KprSocketReader reader;
	KprSocketWriter writer;

	KprMQTTMessage message;
	UInt32 payloadLength;
	struct {
		Boolean headerBegan;
		Boolean headerEnded;
		UInt32 remainingLength;
		UInt32 readLength;

		UInt16 value16;
		KprMemoryBuffer buffer;
		char *string;

		Boolean hasUserName;
		Boolean hasPassword;
		Boolean hasWill;

		UInt32 lengthMultiplier;
	} reading;

	KprMQTTEndpointMessageCallback messageCallback;
	KprMQTTEndpointErrorCallback errorCallback;
	void *refcon;
};

FskErr KprMQTTEndpointNew(KprMQTTEndpoint* it, FskSocket skt, void *refcon);
FskErr KprMQTTEndpointDispose(KprMQTTEndpoint self);

FskErr KprMQTTEndpointSendMessage(KprMQTTEndpoint self, KprMQTTMessage message);

#endif
