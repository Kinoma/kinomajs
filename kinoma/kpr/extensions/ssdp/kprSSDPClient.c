/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#include "kpr.h"
#include "kprHTTPClient.h"

#include "kprSSDPCommon.h"
#include "kprSSDPClient.h"

#include "FskList.h"

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprSSDPClientInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSSDPClient", FskInstrumentationOffset(KprSSDPClientRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

#if 0
#pragma mark - KprSSDPClient
#endif

static KprSSDPClient gKprSSDPClients = NULL;

FskErr KprSSDPClientNew(KprSSDPClient *it, const char* type)
{
	FskErr err = kFskErrNone;
	KprSSDPClient self = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPClientRecord), it));
	self = *it;
	if (type) {
		self->type = FskStrDoCopy(type);
		bailIfNULL(self->type);
	}
	FskInstrumentedItemNew(self, NULL, &KprSSDPClientInstrumentation);
bail:
	if (err)
		KprSSDPClientDispose(self);
	return err;
}

void KprSSDPClientDispose(KprSSDPClient self)
{
	if (!self) return;
	if (FskListContains(&gKprSSDPClients, self)) {
		(void)KprSSDPClientStop(self);
	}
	if (self->services) {
		UInt32 i;
		for (i = 0; self->services[i]; i++) {
			FskMemPtrDispose(self->services[i]);
		}
		FskMemPtrDispose(self->services);
	}
	FskMemPtrDispose(self->type);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

void KprSSDPClientCallback(char* authority, KprSSDPDiscoveryDescription description, Boolean alive, void* refcon)
{
	KprSSDPClient self = refcon;

	if (FskListContains(&gKprSSDPClients, self)) {
		if (alive) {
			if (self->addServerCallback)
				(*self->addServerCallback)(self, description);
		}
		else if (self->removeServerCallback)
			(*self->removeServerCallback)(self, description);
	}
	KprSSDPDiscoveryDescriptionDispose(description);
}

FskErr KprSSDPClientRemove(KprSSDPClient self, const char* uuid)
{
	FskErr err = kFskErrNone;
	char* it = FskStrDoCopy(uuid);
	bailIfNULL(it);
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPRemoveDiscoveryByUUID, it, NULL, NULL, NULL);
bail:
	return err;
}

FskErr KprSSDPClientSearch(KprSSDPClient self)
{
	FskErr err = kFskErrNone;
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPSearch, self, NULL, NULL, NULL);
//bail:
	return err;
}

FskErr KprSSDPClientStart(KprSSDPClient self)
{
	FskErr err = kFskErrNone;
	FskListAppend(&gKprSSDPClients, self);
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPDiscover, self->type, self->services, KprSSDPClientCallback, self);
//bail:
	return err;
}

FskErr KprSSDPClientStop(KprSSDPClient self)
{
	FskErr err = kFskErrNone;
	FskListRemove(&gKprSSDPClients, self);
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPForget, self, NULL, NULL, NULL);
//bail:
	return err;
}
