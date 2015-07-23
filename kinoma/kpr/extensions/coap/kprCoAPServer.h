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
#ifndef __KPRCOAPSERVER__
#define __KPRCOAPSERVER__

#include "kpr.h"
#include "kprCoAPCommon.h"
#include "FskNetUtils.h"
#include "kprUtilities.h"

//--------------------------------------------------
// Typedefs
//--------------------------------------------------

typedef struct KprCoAPServerCallbacks KprCoAPServerCallbacks;
typedef struct KprCoAPServerInterfaceRecord KprCoAPServerInterfaceRecord, *KprCoAPServerInterface;
typedef struct KprCoAPServerSessionRecord KprCoAPServerSessionRecord, *KprCoAPServerSession;

typedef FskErr (*KprCoAPServerResourceCallback)(KprCoAPServerSession session, void *refcon);
typedef void (*KprCoAPServerRetryCallback)(KprCoAPMessage message, UInt32 retryCount, void *refcon);
typedef void (*KprCoAPServerErrorCallback)(FskErr err, const char *reason, void *refcon);

struct KprCoAPServerCallbacks {
	KprCoAPServerResourceCallback resourceCallback;
	KprCoAPServerRetryCallback retryCallback;
	KprCoAPServerErrorCallback errorCallback;
};

struct KprCoAPServerRecord {
	KprRetainable retainable;

	KprCoAPServerInterface interfaces;

	UInt16 port;
	Boolean all;

	KprCoAPServerSession runningSessions;
	KprCoAPServerSession finishedSessions;
	UInt32 sessionId;
	UInt16 messageId;

	FskTimeCallBack periodicalCallback;

	KprCoAPServerCallbacks callbacks;
	void *refcon;
};

struct KprCoAPServerInterfaceRecord {
	KprCoAPServerInterface next;
	KprCoAPServer server;

	FskSocket socket;
	UInt32 ipaddr;
	UInt16 port;
	const char *interfaceName;

	KprCoAPReceiver receiver;
	
	KprCoAPEndpoint endpoints;
};

//--------------------------------------------------
// Methods
//--------------------------------------------------

FskErr KprCoAPServerNew(KprCoAPServerCallbacks *callbacks, void *refcon, KprCoAPServer *it);
FskErr KprCoAPServerDispose(KprCoAPServer self);
KprCoAPServer KprCoAPServerRetain(KprCoAPServer self);

FskErr KprCoAPServerStart(KprCoAPServer self, UInt16 port, const char *interfaceName);
FskErr KprCoAPServerStop(KprCoAPServer self);

FskErr KprCoAPServerGetRunningSession(KprCoAPServer self, UInt32 sessionId, KprCoAPServerSession *it);
FskErr KprCoAPServerRememberSession(KprCoAPServer self, KprCoAPServerSession session);
FskErr KprCoAPServerFinishSession(KprCoAPServer self, KprCoAPServerSession session);

int KprCoAPServerNextSessionId(KprCoAPServer self);
UInt16 KprCoAPServerGenerateMessageId(KprCoAPServer self);

#endif
