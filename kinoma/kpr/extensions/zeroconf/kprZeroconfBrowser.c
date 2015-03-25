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
#include "kprShell.h"
#include "kprZeroconf.h"
#include "kprZeroconfCommon.h"
#include "kprZeroconfBrowser.h"

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprZeroconfBrowserInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprZeroconfBrowser", FskInstrumentationOffset(KprZeroconfBrowserRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

FskList gZeroconfBrowsers = NULL;

#if 0
#pragma mark - KprZeroconfBrowser
#endif

FskErr KprZeroconfBrowserNew(KprZeroconfBrowser *it, const char* serviceType)
{
	FskErr err = kFskErrNone;
	KprZeroconfBrowser self = NULL;

	if (KprZeroconfBrowserFind(NULL, serviceType)) {
		bailIfError(kFskErrDuplicateElement); // only one browser per serviceType
	}
	bailIfError(FskMemPtrNewClear(sizeof(KprZeroconfBrowserRecord), it));
	self = *it;
	if (serviceType) {
		self->serviceType = FskStrDoCopy(serviceType);
		bailIfNULL(self->serviceType);
	}
	FskInstrumentedItemNew(self, NULL, &KprZeroconfBrowserInstrumentation);
	bailIfError(KprZeroconfPlatformBrowserNew(self));
bail:
	if (err)
		KprZeroconfBrowserDispose(self);
	return err;
}

void KprZeroconfBrowserDispose(KprZeroconfBrowser self)
{
	if (self) {
		KprZeroconfPlatformBrowserDispose(self);
		FskMemPtrDispose(self->serviceType);
		FskMemPtrDispose(self->domain);
		FskMemPtrDispose(self->authority);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

KprZeroconfBrowser KprZeroconfBrowserFind(KprZeroconfBrowser self, const char* serviceType)
{
	KprZeroconfBrowser browser = self ? self : gZeroconfBrowsers;
	while (browser && FskStrCompare(browser->serviceType, serviceType)) {
		browser = (KprZeroconfBrowser)browser->next;
	}
	return browser;
}

void KprZeroconfBrowserServiceUp(KprZeroconfBrowser self, KprZeroconfServiceInfo service)
{
	if (!self) self = KprZeroconfBrowserFind(NULL, service->type);
	if (!self) return;
	FskInstrumentedItemPrintfDebug(self, "%p - KprZeroconfBrowserServiceUp %s %s - %p %p", self, service->type, service->name, KprShellGetThread(gShell), FskThreadGetCurrent());
	if (self->serviceUpCallback)
		(*self->serviceUpCallback)(self, service);
	KprZeroconfServiceInfoDispose(service);
}

void KprZeroconfBrowserServiceDown(KprZeroconfBrowser self, KprZeroconfServiceInfo service)
{
	if (!self) self = KprZeroconfBrowserFind(NULL, service->type);
	if (!self) return;
	FskInstrumentedItemPrintfDebug(self, "%p - KprZeroconfBrowserServiceDown %s %s - %p %p", self, service->type, service->name, KprShellGetThread(gShell), FskThreadGetCurrent());
	if (self->serviceDownCallback)
		(*self->serviceDownCallback)(self, service);
	KprZeroconfServiceInfoDispose(service);
}

FskErr KprZeroconfBrowserStart(KprZeroconfBrowser self)
{
	FskErr err = kFskErrNone;
	bailIfError(KprZeroconfPlatformBrowserStart(self));
	FskListAppend(&gZeroconfBrowsers, self);
bail:
	return err;
}

FskErr KprZeroconfBrowserStop(KprZeroconfBrowser self)
{
	FskErr err = kFskErrNone;
	FskListRemove(&gZeroconfBrowsers, self);
	bailIfError(KprZeroconfPlatformBrowserStop(self));
bail:
	return err;
}
