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
#include "kpr.h"
#include "kprZeroconfCommon.h"

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprZeroconfServiceInfoInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprZeroconfServiceInfo", FskInstrumentationOffset(KprZeroconfServiceInfoRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

#if 0
#pragma mark - KprBonjourServiceInfo
#endif

FskErr KprZeroconfServiceInfoNew(KprZeroconfServiceInfo *it, const char* type, const char* name, const char* host, const char* ip, UInt32 port, const UInt32 interfaceIndex, char* txt)
{
	FskErr err = kFskErrNone;
	KprZeroconfServiceInfo self = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(KprZeroconfServiceInfoRecord), it));
	self = *it;
	self->type = FskStrDoCopy(type);
	bailIfNULL(self->type);
	self->name = FskStrDoCopy(name);
	bailIfNULL(self->name);
	if (host) {
		self->host = FskStrDoCopy(host);
		bailIfNULL(self->host);
	}
	if (ip) {
		self->ip = FskStrDoCopy(ip);
		bailIfNULL(self->ip);
	}
	self->port = port;
	self->interfaceIndex = interfaceIndex;
	if (txt) {
		self->txt = FskStrDoCopy(txt);
		bailIfNULL(self->txt);
	}
	FskInstrumentedItemNew(self, NULL, &KprZeroconfServiceInfoInstrumentation);
bail:
	if (err)
		KprZeroconfServiceInfoDispose(self);
	return err;
}

void KprZeroconfServiceInfoDispose(KprZeroconfServiceInfo self)
{
	if (self) {
		FskMemPtrDispose(self->txt);
		FskMemPtrDispose(self->ip);
		FskMemPtrDispose(self->host);
		FskMemPtrDispose(self->name);
		FskMemPtrDispose(self->type);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}
