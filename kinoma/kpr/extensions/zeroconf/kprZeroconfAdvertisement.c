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
#include "kprZeroconf.h"

#include "kprZeroconfCommon.h"
#include "kprZeroconfAdvertisement.h"

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprZeroconfAdvertisementInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprZeroconfAdvertisement", FskInstrumentationOffset(KprZeroconfAdvertisementRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

#if 0
#pragma mark - KprZeroconfAdvertisement
#endif

FskList gZeroconfAdvertisements = NULL;

FskErr KprZeroconfAdvertisementNew(KprZeroconfAdvertisement *it, const char* serviceType, const char* serviceName, UInt32 port)
{
	FskErr err = kFskErrNone;
	KprZeroconfAdvertisement self = NULL;

	if (KprZeroconfAdvertisementFind(NULL, serviceType, port)) {
		bailIfError(kFskErrDuplicateElement); // only one advertisement per serviceType / port
	}
	bailIfError(FskMemPtrNewClear(sizeof(KprZeroconfAdvertisementRecord), it));
	self = *it;
	self->serviceType = FskStrDoCopy(serviceType);
	bailIfNULL(self->serviceType);
	self->serviceName = FskStrDoCopy(serviceName);
	bailIfNULL(self->serviceName);
	self->port = port;
	FskInstrumentedItemNew(self, NULL, &KprZeroconfAdvertisementInstrumentation);
	bailIfError(KprZeroconfPlatformAdvertisementNew(self));
bail:
	if (err)
		KprZeroconfAdvertisementDispose(self);
	return err;
}

void KprZeroconfAdvertisementDispose(KprZeroconfAdvertisement self)
{
	if (!self) return;
	KprZeroconfPlatformAdvertisementDispose(self);
	FskMemPtrDispose(self->serviceType);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

KprZeroconfAdvertisement KprZeroconfAdvertisementFind(KprZeroconfAdvertisement self, const char* serviceType, const UInt32 port)
{
	KprZeroconfAdvertisement advertisement = self ? self : gZeroconfAdvertisements;
	while (advertisement && ((port && (advertisement->port != port)) || FskStrCompare(advertisement->serviceType, serviceType))) {
		advertisement = (KprZeroconfAdvertisement)advertisement->next;
	}
	return advertisement;
}

void KprZeroconfAdvertisementServiceRegistered(KprZeroconfAdvertisement self, KprZeroconfServiceInfo service)
{
	if (!self) self = KprZeroconfAdvertisementFind(NULL, service->type, service->port);
	if (!self) return;
	if (FskStrCompare(self->serviceName, service->name)) {
		char* name = FskStrDoCopy(service->name);
		if (name) {
			FskMemPtrDispose(service->name);
			service->name = name;
		}
	}
	if (self->registeredCallback)
		(*self->registeredCallback)(self);
	KprZeroconfServiceInfoDispose(service);
}

FskErr KprZeroconfAdvertisementStart(KprZeroconfAdvertisement self)
{
	FskErr err = kFskErrNone;
	bailIfError(KprZeroconfPlatformAdvertisementStart(self));
	FskListAppend(&gZeroconfAdvertisements, self);
bail:
	return err;
}

FskErr KprZeroconfAdvertisementStop(KprZeroconfAdvertisement self)
{
	FskErr err = kFskErrNone;
	FskListRemove(&gZeroconfAdvertisements, self);
	bailIfError(KprZeroconfPlatformAdvertisementStop(self));
	if (self->unregisteredCallback)
		(*self->unregisteredCallback)(self);
bail:
	return err;
}
