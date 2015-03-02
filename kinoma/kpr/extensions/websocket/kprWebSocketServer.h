/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#ifndef __KPRWEBSOCKETSERVER__
#define __KPRWEBSOCKETSERVER__

#include "kpr.h"
#include "kprURL.h"
#include "FskThread.h"
#include "FskNetInterface.h"

#include "kprWebSocketEndpoint.h"

//--------------------------------------------------
// WebSocketServer
//--------------------------------------------------

typedef struct KprWebSocketServerStruct KprWebSocketServerRecord, *KprWebSocketServer;
typedef struct KprWebSocketServerRequestStruct KprWebSocketServerRequestRecord, *KprWebSocketServerRequest;

// Callbacks

typedef void (*KprWebSocketServerLaunchCallback)(KprWebSocketServer self, void *refcon);
typedef void (*KprWebSocketServerConnectCallback)(KprWebSocketServer self, FskSocket sock, void *refcon);
typedef void (*KprWebSocketServerErrorCallback)(KprWebSocketServer self, FskErr err, char *message, void *refcon);

struct KprWebSocketServerStruct {
	KprWebSocketServerRequest activeRequests;

	Boolean stopped;
	void *refCon;

	KprSocketServer server;
	FskThread owner;

	KprWebSocketServerLaunchCallback launchCallback;
	KprWebSocketServerConnectCallback connectCallback;
	KprWebSocketServerErrorCallback errorCallback;
};

struct KprWebSocketServerRequestStruct {
	KprWebSocketServerRequest next;
	KprWebSocketServer server;
	FskSocket skt;
	int requesterAddress;
	int requesterPort;
	FskHeaders *requestHeaders;
	FskStrParsedUrl parts;

	FskHeaders *responseHeaders;
	struct {
		UInt8 *buffer;
		UInt32 size;
		UInt32 length;
		UInt32 pos;
	} out;

	FskThreadDataHandler dataHandler;
};

FskErr KprWebSocketServerNew(KprWebSocketServer* it, void *refCon);
void KprWebSocketServerDispose(KprWebSocketServer self);
FskErr KprWebSocketServerListen(KprWebSocketServer self, int port, char *interfaceName);

#endif
