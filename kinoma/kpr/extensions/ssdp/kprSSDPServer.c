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
#include "kpr.h"
#include "kprHTTPClient.h"
#include "kprShell.h"

#include "kprSSDPCommon.h"
#include "kprSSDPServer.h"

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprSSDPServerInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSSDPServer", FskInstrumentationOffset(KprSSDPServerRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

#if 0
#pragma mark - KprSSDPServer
#endif

static KprSSDPServer gKprSSDPServers = NULL;

FskErr KprSSDPServerNew(KprSSDPServer *it, UInt32 port, const char* path, UInt32 expire, const char* uuid, const char* type)
{
	FskErr err = kFskErrNone;
	KprSSDPServer self = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPServerRecord), it));
	self = *it;
	self->port = port;
	self->path = FskStrDoCopy(path);
	bailIfNULL(self->path);
	self->expire = expire;
	self->uuid = FskStrDoCopy(uuid);
	bailIfNULL(self->uuid);
	self->type = FskStrDoCopy(type);
	bailIfNULL(self->type);
	FskInstrumentedItemNew(self, NULL, &KprSSDPServerInstrumentation);
bail:
	if (err)
		KprSSDPServerDispose(self);
	return err;
}

void KprSSDPServerDispose(KprSSDPServer self)
{
	if (!self) return;
	if (FskListContains(&gKprSSDPServers, self)) {
		(void)KprSSDPServerStop(self);
	}
	if (self->services) {
		UInt32 i;
		for (i = 0; self->services[i]; i++) {
			FskMemPtrDispose(self->services[i]);
		}
		FskMemPtrDispose(self->services);
	}
	FskMemPtrDispose(self->type);
	FskMemPtrDispose(self->path);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

FskErr KprSSDPServerStart(KprSSDPServer self)
{
	FskErr err = kFskErrNone;
	KprSSDPDevice device = NULL;
	bailIfError(KprSSDPDeviceNew(&device, self->port, self->path, 0, 0, self->uuid, self->type, self->services));
	FskListAppend(&gKprSSDPServers, self);
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPAddDevice, device, NULL, NULL, NULL);
	if (self->registeredCallback)
		(*self->registeredCallback)(self);
bail:
	return err;
}

FskErr KprSSDPServerStop(KprSSDPServer self)
{
	FskErr err = kFskErrNone;
	FskListRemove(&gKprSSDPServers, self);
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPRemoveDevice, FskStrDoCopy(self->uuid), NULL, NULL, NULL);
	if (self->unregisteredCallback)
		(*self->unregisteredCallback)(self);
	return err;
}
