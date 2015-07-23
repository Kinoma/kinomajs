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
#ifndef __KPRCOAPCLIENTRESOLVER__
#define __KPRCOAPCLIENTRESOLVER__

//--------------------------------------------------
// Typedefs
//--------------------------------------------------

typedef struct KprCoAPClientResolverRecord KprCoAPClientResolverRecord, *KprCoAPClientResolver;

#include "kpr.h"
#include "kprUtilities.h"
#include "kprCoAPClient.h"
#include "FskResolver.h"

struct KprCoAPClientResolverRecord {
	KprCoAPClientResolver next;

	KprCoAPClient client;
	const char *host;
	UInt32 ipaddr;
	UInt16 port;
	KprCoAPMessageQueue waiting; // waiting endpoint ready.

	FskResolver resolver;
	FskTimeRecord resolvedAt;

	Boolean constructed;
	FskErr err;
};

//--------------------------------------------------
// Methods
//--------------------------------------------------

FskErr KprCoAPClientResolverNew(KprCoAPClientResolver *it, KprCoAPClient client, const char *host, UInt16 port, KprCoAPMessage message);
FskErr KprCoAPClientResolverDispose(KprCoAPClientResolver self);

Boolean KprCoAPClientResolverIsResolved(KprCoAPClientResolver self);

FskErr KprCoAPClientResolverQueueMessage(KprCoAPClientResolver self, KprCoAPMessage message);


#endif
