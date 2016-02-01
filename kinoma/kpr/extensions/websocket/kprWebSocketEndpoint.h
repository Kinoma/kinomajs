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
#ifndef __KPRWEBSOCKETENDPOINT__
#define __KPRWEBSOCKETENDPOINT__

#include "kprWebSocketCommon.h"

//--------------------------------------------------
// WebSocket Endpoint
//--------------------------------------------------

typedef struct KprWebSocketEndpointStruct KprWebSocketEndpointRecord, *KprWebSocketEndpoint;

// Callbacks

typedef void (*KprWebSocketEndpointOpenCallback)(KprWebSocketEndpoint self, void *refcon);
typedef void (*KprWebSocketEndpointCloseCallback)(KprWebSocketEndpoint self, UInt16 code, char *reason, Boolean wasClean, void *refcon);
typedef void (*KprWebSocketEndpointTextCallback)(KprWebSocketEndpoint self, char *text, void *refcon);
typedef void (*KprWebSocketEndpointBinaryCallback)(KprWebSocketEndpoint self, void *data, UInt32 length, void *refcon);
typedef void (*KprWebSocketEndpointErrorCallback)(KprWebSocketEndpoint self, FskErr err, char *message, void *refcon);


struct KprWebSocketEndpointStruct {
	char	*url;
	FskStrParsedUrl parts;
	char	*origin;
	UInt8	state;
	Boolean isSecure;

	// @TBD quick and dirty fix for the Alain's websocket crash on close
	UInt8	pendingSendCount; // @DIRTYHACK
	Boolean disposeRequested; // @DIRTYHACK

	Boolean doMask;
	Boolean needMaskedFrame;
	Boolean needNonMaskedFrame;

	char	*key;
	Boolean closeWasSent;
	Boolean closeWasReceived;
	Boolean ignoreFurtherFrame;

	FskTimeRecord pingSent;
	Boolean pongReceived;

	UInt16 closeCode;
	char *closeReason;
	Boolean cleanClose;

	FskSocket socket;
	int		ip;
	KprSocketReader reader;
	KprSocketWriter writer;
	Boolean cancelConnection;
	
	struct {
		FskHeaders *headers;
		char buffer[8];
		
		UInt8 opcode;
		Boolean fin;
		Boolean mask;
		UInt8 maskData[4];
		UInt8 *message;
		UInt32 length;
	} read;
	
	KprWebSocketEndpointOpenCallback openCallback;
	KprWebSocketEndpointCloseCallback closingCallback;
	KprWebSocketEndpointCloseCallback closeCallback;
	KprWebSocketEndpointTextCallback textCallback;
	KprWebSocketEndpointBinaryCallback binaryCallback;
	KprWebSocketEndpointErrorCallback errorCallback;

	FskInstrumentedItemDeclaration

	void *refcon;
};

FskErr KprWebSocketEndpointNew(KprWebSocketEndpoint* it, void *refcon);
void KprWebSocketEndpointDispose(KprWebSocketEndpoint self);

FskErr KprWebSocketEndpointConnect(KprWebSocketEndpoint self, char *url, char *origin);
FskErr KprWebSocketEndpointOpenWithSocket(KprWebSocketEndpoint self, FskSocket skt);
FskErr KprWebSocketEndpointClose(KprWebSocketEndpoint self, UInt16 code, char *reason);

FskErr KprWebSocketEndpointSendString(KprWebSocketEndpoint self, char *message);
FskErr KprWebSocketEndpointSendBinary(KprWebSocketEndpoint self, void *data, UInt32 length);
FskErr KprWebSocketEndpointSendPing(KprWebSocketEndpoint self);

UInt32 KprWebSocketEndpointGetPendingData(KprWebSocketEndpoint self);

// Deferred Utilities

void KprWebSocketEndpointInvokeAfter(void *func, void *param1, void *param2, void *param3, void *param4);
#define INVOKE_AFTER0(f) KprWebSocketEndpointInvokeAfter(f, NULL, NULL, NULL, NULL)
#define INVOKE_AFTER1(f, p1) KprWebSocketEndpointInvokeAfter(f, p1, NULL, NULL, NULL)
#define INVOKE_AFTER2(f, p1, p2) KprWebSocketEndpointInvokeAfter(f, p1, p2, NULL, NULL)
#define INVOKE_AFTER3(f, p1, p2, p3) KprWebSocketEndpointInvokeAfter(f, p1, p2, p3, NULL)
#define INVOKE_AFTER4(f, p1, p2, p3, p4) KprWebSocketEndpointInvokeAfter(f, p1, p2, p3, p4)

#endif
