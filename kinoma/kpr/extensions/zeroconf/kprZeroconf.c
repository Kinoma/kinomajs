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
#include "FskAssociativeArray.h"
#include "FskUUID.h"

#include "kpr.h"
#include "kprHTTPServer.h"
#include "kprMessage.h"

#include "kprZeroconf.h"
#include "kprZeroconfCommon.h"
#include "kprZeroconfAdvertisement.h"
#include "kprZeroconfBrowser.h"

static Boolean KprZeroconfServiceAccept(KprService self, KprMessage message);
static void KprZeroconfServiceStart(KprService service, FskThread thread, xsMachine* the);
static void KprZeroconfServiceStop(KprService service);
static void KprZeroconfServiceDiscover(KprService self, char* authority, char* id, Boolean useEnvironment);
static void KprZeroconfServiceForget(KprService self, char* authority, char* id);
static void KprZeroconfServiceShare(KprService self, char* authority, Boolean shareIt, Boolean useEnvironment);

static KprServiceRecord gZeroconfService = {
	NULL,
	0,
	"zeroconf:",
	NULL,
	NULL,
	KprZeroconfServiceAccept,
	KprServiceCancel,
	KprServiceInvoke,
	KprZeroconfServiceStart,
	KprZeroconfServiceStop,
	KprZeroconfServiceDiscover,
	KprZeroconfServiceForget,
	KprZeroconfServiceShare
};

#if 0
#pragma mark - extension
#endif

FskExport(FskErr) kprZeroconf_fskLoad(FskLibrary library)
{
	KprServiceRegister(&gZeroconfService);
	return kFskErrNone;
}


FskExport(FskErr) kprZeroconf_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

#if 0
#pragma mark - KprZeroconfService
#endif

static FskList gKprZeroconfAdvertisements = NULL;
static FskList gKprZeroconfBrowsers = NULL;

Boolean KprZeroconfServiceAccept(KprService self UNUSED, KprMessage message UNUSED)
{
	return false;
}

void KprZeroconfServiceStart(KprService service, FskThread thread, xsMachine* the)
{
	service->machine = the;
	service->thread = thread;
	KprZeroconfPlatformStart();
}

void KprZeroconfServiceStop(KprService service UNUSED)
{
	KprZeroconfPlatformStop();
}

FskErr KprZeroconfServiceNewAuthority(char* type, char** it)
{
#define kKPRZeroconfKinomaServe "_%s._tcp."
	FskErr err = kFskErrNone;
	char* authority = type + 1;
	char* dot = FskStrChr(type, '.');
	char* ptr;
	bailIfNULL(dot);
	*dot = 0;
	authority = FskStrDoCopy(type + 1);
	*dot = '.';
	bailIfNULL(authority);
	for (ptr = FskStrChr(authority, '_'); ptr; ptr = FskStrChr(ptr, '_')) 
		*ptr = '.';
	*it = authority;
bail:
	return err;
}

FskErr KprZeroconfServiceNewType(char* authority, char** it)
{
#define kKPRZeroconfKinomaServe "_%s._tcp."
	FskErr err = kFskErrNone;
	char* type = NULL;
	char* ptr;
	UInt32 length = FskStrLen(authority);
	UInt32 size = length + FskStrLen(kKPRZeroconfKinomaServe);
	bailIfError(FskMemPtrNewClear(size, &type));
	snprintf(type, size, kKPRZeroconfKinomaServe, authority);
	type[length + 1] = 0;
	for (ptr = FskStrChr(type, '.'); ptr; ptr = FskStrChr(ptr, '.')) 
		*ptr = '_';
	type[length + 1] = '.';
	*it = type;
bail:
	return err;
}

 void KprZeroconfDiscoverServerCallback(KprZeroconfBrowser self, KprZeroconfServiceInfo service, Boolean alive)
 {
 #define kKPRZeroconfKinomaDiscoverURL "xkpr://%s/discover"
 #define kKPRZeroconfKinomaDiscoverJSON "{\"id\":\"%s\",\"uuid\":\"%s\",\"url\":\"http://%s:%lu/\",\"protocol\":\"zeroconf\"}"
 #define kKPRZeroconfKinomaForgetURL "xkpr://%s/forget"
 #define kKPRZeroconfKinomaForgetJSON "{\"id\":\"%s\",\"uuid\":\"%s\",\"protocol\":\"zeroconf\"}"
 	FskErr err = kFskErrNone;
 	char id[256];
 	char url[1024];
 	char json[2048];
 	UInt32 size;
 	KprMessage message = NULL;
 	char* authority = NULL;
 	bailIfError(KprZeroconfServiceNewAuthority(service->type, &authority));
	if (alive) {
		snprintf(url, sizeof(url), kKPRZeroconfKinomaDiscoverURL, self->authority);
		snprintf(json, sizeof(json), kKPRZeroconfKinomaDiscoverJSON, authority, service->name, service->ip, service->port);
	}
	else {
		snprintf(url, sizeof(url), kKPRZeroconfKinomaForgetURL, self->authority);
		snprintf(json, sizeof(json), kKPRZeroconfKinomaForgetJSON, authority, service->name);
	}
 	size = FskStrLen(json);
 	bailIfError(KprMessageNew(&message, url));
 	bailIfError(KprMessageSetRequestBody(message, json, size));
 	FskStrNumToStr(size, id, sizeof(id));
 	bailIfError(KprMessageSetRequestHeader(message, kFskStrContentLength, id));
 	bailIfError(KprMessageSetRequestHeader(message, kFskStrContentType, "application/json"));
 	KprMessageInvoke(message, NULL, NULL, NULL);
  	FskMemPtrDispose(authority);
	return;
 bail:
	KprMessageDispose(message);
  	FskMemPtrDispose(authority);
 }

void KprZeroconfServiceServiceUpCallback(KprZeroconfBrowser self, KprZeroconfServiceInfo service)
{
	KprZeroconfDiscoverServerCallback(self, service, true);
	return;
}

void KprZeroconfServiceServiceDownCallback(KprZeroconfBrowser self, KprZeroconfServiceInfo service)
{
	KprZeroconfDiscoverServerCallback(self, service, false);
	return;
}

void KprZeroconfServiceDiscover(KprService self, char* authority, char* id, Boolean useEnvironment)
{
	FskErr err = kFskErrNone;
	char* type = NULL;
	KprZeroconfBrowser browser = NULL;
	if (useEnvironment ? KprEnvironmentGetUInt32("useZeroconf", 0) : true) {
		bailIfError(KprZeroconfServiceNewType(id, &type));
		browser = KprZeroconfBrowserFind(gKprZeroconfBrowsers, type);
		if (!browser) {
			bailIfError(KprZeroconfBrowserNew(&browser, type));
			browser->serviceUpCallback = KprZeroconfServiceServiceUpCallback;
			browser->serviceDownCallback = KprZeroconfServiceServiceDownCallback;
			browser->authority = FskStrDoCopy(authority);
			bailIfError(KprZeroconfBrowserStart(browser));
			FskListAppend(&gKprZeroconfBrowsers, browser);
		}
	}
bail:
	if (err)
		KprZeroconfBrowserDispose(browser);
	FskMemPtrDispose(type);
	FskMemPtrDispose(authority);
	FskMemPtrDispose(id);
	return;
}

void KprZeroconfServiceForget(KprService self, char* authority, char* id)
{
	FskErr err = kFskErrNone;
	char* type = NULL;
	KprZeroconfBrowser browser = NULL;
	bailIfError(KprZeroconfServiceNewType(id, &type));
	browser = KprZeroconfBrowserFind(gKprZeroconfBrowsers, type);
	if (browser) {
		FskListRemove(&gKprZeroconfBrowsers, browser);
		bailIfError(KprZeroconfBrowserStop(browser));
		KprZeroconfBrowserDispose(browser);
	}
bail:
	FskMemPtrDispose(type);
	FskMemPtrDispose(authority);
	FskMemPtrDispose(id);
	return;
}

void KprZeroconfServiceShare(KprService self, char* authority, Boolean shareIt, Boolean useEnvironment)
{
	FskErr err = kFskErrNone;
	char* type = NULL;
	KprZeroconfAdvertisement advertisement = NULL;
	KprHTTPServer server = KprHTTPServerGet(authority);
	bailIfError(KprZeroconfServiceNewType(authority, &type));
	advertisement = KprZeroconfAdvertisementFind(gKprZeroconfAdvertisements, type, 0);
	if (shareIt && useEnvironment)
		shareIt = KprEnvironmentGetUInt32("useZeroconf", 0);
	if (shareIt && server) {
		if (!advertisement) {
			UInt32 port = server ? KprHTTPServerGetPort(server) : 0;
			Boolean secure = server ? KprHTTPServerIsSecure(server) : false;
			char* uuid = FskUUIDGetForKey(authority);
			FskAssociativeArray txt = NULL;
			if (secure) {
				txt = FskAssociativeArrayNew();
				FskAssociativeArrayElementSetString(txt, "secure", "true");
			}
			bailIfError(KprZeroconfAdvertisementNew(&advertisement, type, uuid, port, txt));
			bailIfError(KprZeroconfPlatformAdvertisementStart(advertisement));
			FskListAppend(&gKprZeroconfAdvertisements, advertisement);
		}
	}
	else {
		if (advertisement) {
			FskListRemove(&gKprZeroconfAdvertisements, advertisement);
			bailIfError(KprZeroconfPlatformAdvertisementStop(advertisement));
			KprZeroconfAdvertisementDispose(advertisement);
		}
	}
bail:
	if (err)
		KprZeroconfAdvertisementDispose(advertisement);
	FskMemPtrDispose(type);
	FskMemPtrDispose(authority);
	return;
}

#if 0
#pragma mark - xs
#endif

// common

void Zeroconf_get_behavior(xsMachine *the)
{
	KprZeroconfCommon self = xsGetHostData(xsThis);
	if (xsTypeOf(self->behavior) != xsUndefinedType)
		xsResult = self->behavior;
	else
		xsResult = xsUndefined;
}

void Zeroconf_get_serviceType(xsMachine *the)
{
	KprZeroconfCommon self = xsGetHostData(xsThis);
	xsResult = xsString(self->serviceType);
}

void Zeroconf_set_behavior(xsMachine *the)
{
	KprZeroconfCommon self = xsGetHostData(xsThis);
	xsForget(self->behavior);
	self->behavior = xsUndefined;
	if (xsTest(xsArg(0))) {
		if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
			self->behavior = xsArg(0);
			xsRemember(self->behavior);
		}
	}
}

// advertisement

void Zeroconf_advertisement_registeredCallback(KprZeroconfAdvertisement self);
void Zeroconf_advertisement_unregisteredCallback(KprZeroconfAdvertisement self);

void Zeroconf_Advertisement(xsMachine *the)
{
	KprZeroconfAdvertisement self = NULL;
	char* serviceType = xsToString(xsArg(0));
	char* servicName = xsToString(xsArg(1));
	int servicPort = xsToInteger(xsArg(2));
	xsIntegerValue c = xsToInteger(xsArgc);
	FskAssociativeArray txt = NULL;
	if ((c > 3) && xsIsInstanceOf(xsArg(3), xsObjectPrototype)) {
		xsVars(2);
		xsEnterSandbox();
		fxPush(xsArg(3));
		fxRunForIn(the);
		txt = FskAssociativeArrayNew();
		for (xsVar(0) = fxPop(); xsTypeOf(xsVar(0)) != xsNullType; xsVar(0) = fxPop()) {
			if (xsTypeOf(xsVar(0)) == xsStringType) {
				xsVar(1) = xsGetAt(xsArg(3), xsVar(0));
				if (!xsIsInstanceOf(xsVar(1), xsObjectPrototype)) {
					char* name = xsToString(xsVar(0));
					char* value = xsToString(xsVar(1));
					FskAssociativeArrayElementSetString(txt, name, value);
				}
			}
		}
		xsLeaveSandbox();
	}
	xsThrowIfFskErr(KprZeroconfAdvertisementNew(&self, serviceType, servicName, servicPort, txt));
	xsSetHostData(xsResult, self);
	self->registeredCallback = Zeroconf_advertisement_registeredCallback;
	self->unregisteredCallback = Zeroconf_advertisement_unregisteredCallback;
	self->the = the;
	self->slot = xsResult;
	self->code = the->code;
	self->behavior = xsUndefined;
	xsRemember(self->slot);
}

void Zeroconf_advertisement(void *it)
{
	KprZeroconfAdvertisement self = it;
	if (self) {
		xsMachine *the = self->the;
		xsForget(self->slot);
		xsForget(self->behavior);
		KprZeroconfAdvertisementDispose(self);
	}
}

void Zeroconf_advertisement_callback(KprZeroconfAdvertisement self, char* function)
{
	xsBeginHostSandboxCode(self->the, self->code);
	if (xsTypeOf(self->behavior) == xsUndefinedType) goto bail;
	xsVars(2);
	{
		xsTry {
			xsVar(0) = xsAccess(self->behavior);
			xsVar(1) = xsAccess(self->slot);
			if (xsFindResult(xsVar(0), xsID(function))) {
				(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
			}
		}
		xsCatch {
		}
	}
bail:
	xsEndHostSandboxCode();
}

void Zeroconf_advertisement_get_port(xsMachine *the)
{
	KprZeroconfAdvertisement self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->port);
}

void Zeroconf_advertisement_get_serviceName(xsMachine *the)
{
	KprZeroconfAdvertisement self = xsGetHostData(xsThis);
	xsResult = xsString(self->serviceName);
}

void Zeroconf_advertisement_registeredCallback(KprZeroconfAdvertisement self)
{
	Zeroconf_advertisement_callback(self, "onZeroconfServiceRegistered");
}

void Zeroconf_advertisement_unregisteredCallback(KprZeroconfAdvertisement self)
{
	Zeroconf_advertisement_callback(self, "onZeroconfServiceUnregistered");
}

void Zeroconf_advertisement_start(xsMachine *the)
{
	KprZeroconfAdvertisement self = xsGetHostData(xsThis);
	xsThrowIfFskErr(KprZeroconfAdvertisementStart(self));
}

void Zeroconf_advertisement_stop(xsMachine *the)
{
	KprZeroconfAdvertisement self = xsGetHostData(xsThis);
	xsThrowIfFskErr(KprZeroconfAdvertisementStop(self));
}

// browser

void Zeroconf_browser_serviceUpCallback(KprZeroconfBrowser self, KprZeroconfServiceInfo service);
void Zeroconf_browser_serviceDownCallback(KprZeroconfBrowser self, KprZeroconfServiceInfo service);

void Zeroconf_Browser(xsMachine *the)
{
	KprZeroconfBrowser self = NULL;
	xsTry {
		char* serviveType = NULL;
		if (xsTest(xsArg(0)))
			serviveType = xsToString(xsArg(0));
		xsThrowIfFskErr(KprZeroconfBrowserNew(&self, serviveType));
		xsSetHostData(xsResult, self);
		self->serviceUpCallback = Zeroconf_browser_serviceUpCallback;
		self->serviceDownCallback = Zeroconf_browser_serviceDownCallback;
		self->the = the;
		self->slot = xsResult;
		self->code = the->code;
		self->behavior = xsUndefined;
		xsRemember(self->slot);
	}
	xsCatch {
	}
}

void Zeroconf_browser(void *it)
{
	KprZeroconfBrowser self = it;
	if (self) {
		xsMachine *the = self->the;
		KprZeroconfBrowserStop(self);
		xsForget(self->slot);
		xsForget(self->behavior);
		KprZeroconfBrowserDispose(self);
	}
}

void Zeroconf_browser_callback(KprZeroconfBrowser self, char* function, KprZeroconfServiceInfo service)
{
	xsBeginHostSandboxCode(self->the, self->code);
	if (xsTypeOf(self->behavior) == xsUndefinedType) goto bail;
	xsVars(3);
	{
		xsTry {
			xsVar(0) = xsAccess(self->behavior);
			xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
			xsSet(xsVar(1), xsID("name"), xsString(service->name));
			xsSet(xsVar(1), xsID("type"), xsString(service->type));
			if (service->host)
				xsSet(xsVar(1), xsID("host"), xsString(service->host));
			if (service->ip) {
				xsSet(xsVar(1), xsID("ip"), xsString(service->ip));
				xsSet(xsVar(1), xsID("port"), xsInteger(service->port));
			}
			if (service->txt) {
				char* txt = service->txt;
				UInt32 position = 0, size = FskStrLen(txt);
				UInt32 length = 0;
				xsVar(2) = xsNewInstanceOf(xsObjectPrototype);
				xsSet(xsVar(1), xsID("txt"), xsVar(2));
				length = txt[position++] & 0xFF;
				while ((position + length) <= size) {
					char end;
					char* equal;
					if (!length) break;
					end = txt[position + length];
					txt[position + length] = 0;
					equal = FskStrChr(txt + position, '=');
					if (equal) {
						*equal = 0;
						xsSet(xsVar(2), xsID(txt + position), xsString(equal + 1));
						*equal = '=';
					}
					txt[position + length] = end;
					position += length;
					length = txt[position++] & 0xFF;
				}
			}
			if (xsFindResult(xsVar(0), xsID(function))) {
				(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
			}
		}
		xsCatch {
		}
	}
bail:
	xsEndHostSandboxCode();
}

void Zeroconf_browser_serviceUpCallback(KprZeroconfBrowser self, KprZeroconfServiceInfo service)
{
	Zeroconf_browser_callback(self, "onZeroconfServiceUp", service);
}

void Zeroconf_browser_serviceDownCallback(KprZeroconfBrowser self, KprZeroconfServiceInfo service)
{
	Zeroconf_browser_callback(self, "onZeroconfServiceDown", service);
}

void Zeroconf_browser_start(xsMachine *the)
{
	KprZeroconfBrowser self = xsGetHostData(xsThis);
	xsThrowIfFskErr(KprZeroconfBrowserStart(self));
}

void Zeroconf_browser_stop(xsMachine *the)
{
	KprZeroconfBrowser self = xsGetHostData(xsThis);
	xsThrowIfFskErr(KprZeroconfBrowserStop(self));
}
