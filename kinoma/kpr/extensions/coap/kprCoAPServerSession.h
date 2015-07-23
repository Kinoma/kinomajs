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
#ifndef __KPRCOAPSERVERSESSION__
#define __KPRCOAPSERVERSESSION__

#include "kpr.h"
#include "kprCoAPServer.h"

//--------------------------------------------------
// Typedefs
//--------------------------------------------------

struct KprCoAPServerSessionRecord {
	KprCoAPServerSession next;
	KprRetainable retainable;
	KprCoAPServer server;

	UInt32 sessionId;
	Boolean autoAck;
	
	KprCoAPEndpoint endpoint;

	KprCoAPMessage request;
	Boolean confiramableRequest;
	FskTimeRecord expireAt;

	Boolean responded;
	Boolean emptyAckSent;
	KprCoAPMessage lastResponse;

	Boolean observeRequested;
	Boolean observeAccepted;
	UInt32 observeId;
};

//--------------------------------------------------
// Methods
//--------------------------------------------------

FskErr KprCoAPServerSessionNew(KprCoAPServer server, KprCoAPMessage request, KprCoAPEndpoint endpoint, KprCoAPServerSession *it);
FskErr KprCoAPServerSessionDispose(KprCoAPServerSession self);
KprCoAPServerSession KprCoAPServerSessionRetain(KprCoAPServerSession self);

FskErr KprCoAPServerSessionCreateResponse(KprCoAPServerSession self, KprCoAPMessage *it);
FskErr KprCoAPServerSessionSendResponse(KprCoAPServerSession self, KprCoAPMessage response);
FskErr KprCoAPServerSessionSendAck(KprCoAPServerSession self);
FskErr KprCoAPServerSessionAcceptObserve(KprCoAPServerSession self);
FskErr KprCoAPServerSessionEndObserve(KprCoAPServerSession self);

Boolean KprCoAPServerSessionCompareWith(KprCoAPServerSession self, KprCoAPMessage request, KprCoAPEndpoint endpoint);
FskErr KprCoAPServerSessionRun(KprCoAPServerSession self);
FskErr KprCoAPServerSessionRunAgain(KprCoAPServerSession self);


#endif
