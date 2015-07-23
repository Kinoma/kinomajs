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
#ifndef __KPRCOAPCLIENTREQUEST__
#define __KPRCOAPCLIENTREQUEST__

#include "kpr.h"
#include "kprCoAPClient.h"
#include "kprUtilities.h"

//--------------------------------------------------
// Typedefs
//--------------------------------------------------

struct KprCoAPClientRequestRecord {
	KprCoAPClientRequest next;

	KprCoAPMessage message;
	KprCoAPClient client;
	KprCoAPEndpoint endpoint;

	Boolean ackReceived;
	Boolean responseReceived;
	Boolean observeRequested;
	Boolean observeAccepted;
};

//--------------------------------------------------
// Methods
//--------------------------------------------------

FskErr KprCoAPClientRequestNew(KprCoAPClientRequest *it, KprCoAPClient client, KprCoAPMessage message, KprCoAPEndpoint endpoint);
FskErr KprCoAPClientRequestDispose(KprCoAPClientRequest self);

FskErr KprCoAPClientRequestSendMessage(KprCoAPClientRequest self, KprCoAPMessage message);
Boolean KprCoAPClientRequestMatchResponse(KprCoAPClientRequest self, KprCoAPMessage response);
Boolean KPrCoAPClientRequestExpectsMoreResponse(KprCoAPClientRequest self);
FskErr KprCoAPClientRequestHandleResponse(KprCoAPClientRequest self, KprCoAPMessage response);

#endif
