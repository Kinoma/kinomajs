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
#include "kprZeroconf.h"
#include "kprZeroconfCommon.h"
#include "kprZeroconfAdvertisement.h"
#include "kprZeroconfBrowser.h"

#include "FskNetUtils.h"

#if TARGET_OS_WIN32
#else
	#include <arpa/inet.h>
	#include <arpa/nameser.h>
#endif

#include "dns_sd.h"

#if TARGET_OS_ANDROID
	#define KPR_ZEROCONF_EMBEDDED 1
#else
	#define KPR_ZEROCONF_EMBEDDED 0
#endif

#if KPR_ZEROCONF_EMBEDDED

#include "FskNetInterface.h"
#include "mDNSEmbeddedAPI.h"// Defines the interface to the mDNS core code
#include "mDNSPosix.h"    // Defines the specific types needed to run mDNS on this platform

// Globals
mDNS mDNSStorage;       // mDNS core uses this to store its globals
static mDNS_PlatformSupport PlatformStorage;  // Stores this platform's globals
#define RR_CACHE_SIZE 500
static CacheEntity gRRCache[RR_CACHE_SIZE];
static FskNetInterfaceNotifier gInterfaceNotifier = NULL;

mDNSexport const char ProgramName[] = "mDNSClientPosix";

static const char *gProgramName = ProgramName;

static FskTimeCallBack gEmbeddedServiceTimer = NULL;
static UInt32 gEmbeddedServiceCount = 0;

mDNSlocal void mDNS_StatusCallback(mDNS *const m, mStatus result)
{
	(void)m; // Unused
	if (result == mStatus_NoError) {
	}
	else if (result == mStatus_ConfigChanged) {
// 		udsserver_handle_configchange(m);
	}
	else if (result == mStatus_GrowCache) {
		// Allocate another chunk of cache storage
		CacheEntity *storage = malloc(sizeof(CacheEntity) * RR_CACHE_SIZE);
		if (storage) mDNS_GrowCache(m, storage, RR_CACHE_SIZE);
	}
}

void KprZeroconfEmbeddedCallback(FskTimeCallBack timer UNUSED, const FskTime time UNUSED, void *param)
{
	struct timeval timeout = { 0, 0 };
	sigset_t pSignalsReceived;
	mDNSBool pDataDispatched;
	mStatus status = mDNSPosixRunEventLoopOnce(&mDNSStorage, &timeout, &pSignalsReceived, &pDataDispatched);
	if (gEmbeddedServiceTimer)
		FskTimeCallbackScheduleFuture(gEmbeddedServiceTimer, 1, 0, KprZeroconfEmbeddedCallback, NULL);
}

int KprZeroconfNetworkInterfaceNotifier(struct FskNetInterfaceRecord* iface, UInt32 status, void* param)
{
	FskErr err = kFskErrNone;
	if (mDNSPlatformPosixRefreshInterfaceList(&mDNSStorage) != kDNSServiceErr_NoError)
		err = kFskErrNetworkInterfaceError;
	return err;
}


#endif

void KprZeroconfPlatformStart()
{
#if KPR_ZEROCONF_EMBEDDED
    mStatus     status;
	status = mDNS_Init(&mDNSStorage, &PlatformStorage,
    	gRRCache, RR_CACHE_SIZE,
    	mDNS_Init_DontAdvertiseLocalAddresses,
    	mDNS_StatusCallback, mDNS_Init_NoInitCallbackContext);
	
	gInterfaceNotifier = FskNetInterfaceAddNotifier(KprZeroconfNetworkInterfaceNotifier, NULL, "KprZeroconfNetworkInterfaceNotifier");
#endif
}

void KprZeroconfPlatformStop()
{
#if KPR_ZEROCONF_EMBEDDED
	if (gInterfaceNotifier) {
		FskNetInterfaceRemoveNotifier(gInterfaceNotifier);
		gInterfaceNotifier = NULL;
	}
	FskTimeCallbackDispose(gEmbeddedServiceTimer);
	gEmbeddedServiceTimer = NULL;
#endif
}

typedef struct KprZeroconfPlatformAdvertisementStruct KprZeroconfPlatformAdvertisementRecord, *KprZeroconfPlatformAdvertisement;
typedef struct KprZeroconfPlatformBrowserStruct KprZeroconfPlatformBrowserRecord, *KprZeroconfPlatformBrowser;
typedef struct KprZeroconfPlatformServiceStruct KprZeroconfPlatformServiceRecord, *KprZeroconfPlatformService;

struct KprZeroconfPlatformAdvertisementStruct {
	KprZeroconfPlatformService service;
	FskInstrumentedItemDeclaration
};

struct KprZeroconfPlatformBrowserStruct {
	KprZeroconfPlatformService service;
	KprZeroconfPlatformService services;
	KprZeroconfPlatformService types;
	FskInstrumentedItemDeclaration
};

struct KprZeroconfPlatformServiceStruct {
	KprZeroconfPlatformService next;
	KprZeroconfPlatformService owner;
	char* name;
	char* txt;
#if KPR_ZEROCONF_EMBEDDED
#elif TARGET_OS_MAC
	CFSocketRef socket;
	CFRunLoopSourceRef source;
#else
	FskThreadDataHandler handler;
	FskThreadDataSource source;
#endif
	UInt32 port;
	DNSServiceRef serviceRef;
	FskInstrumentedItemDeclaration
};

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprZeroconfPlatformAdvertisementInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprZeroconfPlatformAdvertisement", FskInstrumentationOffset(KprZeroconfPlatformAdvertisementRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord KprZeroconfPlatformBrowserInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprZeroconfPlatformBrowser", FskInstrumentationOffset(KprZeroconfPlatformBrowserRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord KprZeroconfPlatformServiceInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprZeroconfPlatformService", FskInstrumentationOffset(KprZeroconfPlatformServiceRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

#if TARGET_OS_MAC
void KprZeroconfPlatformCallBack(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
#else
void KprZeroconfPlatformCallBack(FskThreadDataHandler handler, FskThreadDataSource source, void* context)
#endif
{
	DNSServiceRef serviceRef = context;
	DNSServiceErrorType error = kDNSServiceErr_NoError;
	error = DNSServiceProcessResult(serviceRef);
	if (error)
		FskDebugStr("!!! KprZeroconfPlatformCallBack error %d", error);
}

#if 0
#pragma mark - KprZeroconfPlatformService
#endif

FskErr KprZeroconfPlatformServiceNew(KprZeroconfPlatformService* it, KprZeroconfPlatformService owner, DNSServiceRef serviceRef, const char* name, UInt32 port);
void KprZeroconfPlatformServiceDispose(KprZeroconfPlatformService self);
KprZeroconfPlatformService KprZeroconfPlatformServiceFind(KprZeroconfPlatformService self, DNSServiceRef serviceRef);

FskErr KprZeroconfPlatformServiceNew(KprZeroconfPlatformService* it, KprZeroconfPlatformService owner, DNSServiceRef serviceRef, const char* name, UInt32 port)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformService self = NULL;
	SInt32 fd = DNSServiceRefSockFD(serviceRef);
	bailIfError(FskMemPtrNewClear(sizeof(KprZeroconfPlatformServiceRecord), &self));
	FskInstrumentedItemNew(self, NULL, &KprZeroconfPlatformServiceInstrumentation);
	self->owner = owner;
	self->serviceRef = serviceRef;
	if (name) {
		self->name = FskStrDoCopy(name);
		bailIfNULL(self->name);
	}
	self->port = port;
#if KPR_ZEROCONF_EMBEDDED
	if (!gEmbeddedServiceCount) {
		FskTimeCallbackNew(&gEmbeddedServiceTimer);
		FskTimeCallbackScheduleFuture(gEmbeddedServiceTimer, 0, 0, KprZeroconfEmbeddedCallback, NULL);
	}
	gEmbeddedServiceCount++;
#elif TARGET_OS_MAC
	CFSocketContext context;
	FskMemSet(&context, 0, sizeof(context));
	context.info = (void*)serviceRef;
	self->socket = CFSocketCreateWithNative(kCFAllocatorDefault, fd, kCFSocketReadCallBack, KprZeroconfPlatformCallBack, &context);
	self->source = CFSocketCreateRunLoopSource(NULL, self->socket, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), self->source, kCFRunLoopCommonModes);
#else
	self->source = FskThreadCreateDataSource(fd);
	FskSocketActivate(self->source, true);
	FskThreadAddDataHandler(&self->handler, self->source, KprZeroconfPlatformCallBack, true, false, serviceRef);
#endif
	*it = self;
	return err;
bail:
	if (err) {
		KprZeroconfPlatformServiceDispose(self);
	}
	return err;
}

void KprZeroconfPlatformServiceDispose(KprZeroconfPlatformService self)
{
	if (self) {
#if KPR_ZEROCONF_EMBEDDED
		gEmbeddedServiceCount--;
		if (!gEmbeddedServiceCount) {
			FskTimeCallbackDispose(gEmbeddedServiceTimer);
			gEmbeddedServiceTimer = NULL;
		}
#elif TARGET_OS_MAC
		if (self->source) {
			CFRunLoopRemoveSource(CFRunLoopGetCurrent(), self->source, kCFRunLoopCommonModes);
			CFRelease(self->source);
			self->source = NULL;
		}
		if (self->socket) {
			CFSocketInvalidate(self->socket);
			CFRelease(self->socket);
			self->socket = NULL;
		}
#else
		FskSocketActivate(self->source, false);
		if (self->handler) {
			FskThreadRemoveDataHandler(&self->handler);
			self->handler = NULL;
		}
		if (self->source) {
			FskMemPtrDispose(self->source);
			self->source = NULL;
		}
#endif
		if (self->serviceRef) {
			DNSServiceRefDeallocate(self->serviceRef);
			self->serviceRef = NULL;
		}
		FskMemPtrDispose(self->txt);
		FskMemPtrDispose(self->name);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

KprZeroconfPlatformService KprZeroconfPlatformServiceFind(KprZeroconfPlatformService self, DNSServiceRef serviceRef)
{
	KprZeroconfPlatformService service = self;
	while (service) {
		if (service->serviceRef == serviceRef) break;
		service = service->next;
	}
	return service;
}

KprZeroconfPlatformService KprZeroconfPlatformServiceFindType(KprZeroconfPlatformService self, char* name)
{
	KprZeroconfPlatformService service = self;
	while (service) {
		if (!FskStrCompare(service->name, name)) break;
		service = service->next;
	}
	return service;
}

#if 0
#pragma mark - KprZeroconfPlatformAdvertisement
#endif

void DNSSD_API KprZeroconfPlatformAdvertisementProcess(DNSServiceRef ref, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char* name, const char* type, const char* domain, void* context)
{
	KprZeroconfAdvertisement self = context;
	if (errorCode != kDNSServiceErr_NoError) {
		FskInstrumentedItemPrintfDebug((KprZeroconfPlatformAdvertisement)(self->platform), "KprZeroconfPlatformResolveCallBack returned %d\n", errorCode);
	}
	else {
		KprZeroconfServiceInfo serviceInfo = NULL;
		FskInstrumentedItemPrintfDebug((KprZeroconfPlatformAdvertisement)(self->platform), "Advertisement: %s %s %s", name, type, domain);
		KprZeroconfServiceInfoNew(&serviceInfo, type, name, NULL, NULL, 0, 0, NULL);
		KprZeroconfAdvertisementServiceRegistered(self, serviceInfo);
	}
}

FskErr KprZeroconfPlatformAdvertisementNew(KprZeroconfAdvertisement self)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformAdvertisement advertisement = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprZeroconfPlatformAdvertisementRecord), &advertisement));
	FskInstrumentedItemNew(advertisement, NULL, &KprZeroconfPlatformAdvertisementInstrumentation);

	self->platform = advertisement;
bail:
	if (err)
		KprZeroconfPlatformAdvertisementDispose(self);
	return err;
}

void KprZeroconfPlatformAdvertisementDispose(KprZeroconfAdvertisement self)
{
	KprZeroconfPlatformAdvertisement advertisement = self->platform;
	if (advertisement) {
		KprZeroconfPlatformAdvertisementStop(self);
		FskInstrumentedItemDispose(advertisement);
		FskMemPtrDispose(advertisement);
		self->platform = NULL;
	}
}

FskErr KprZeroconfPlatformAdvertisementStart(KprZeroconfAdvertisement self)
{
	FskErr err = kFskErrNone;
	char* txt = NULL;
	UInt32 size = 0, offset = 0, length;
	KprZeroconfPlatformAdvertisement advertisement = self->platform;
	if (!advertisement->service) {
		DNSServiceErrorType error;
		DNSServiceRef serviceRef;
		FskAssociativeArrayIterator iterator = FskAssociativeArrayIteratorNew(self->txt);

		FskInstrumentedItemPrintfDebug(advertisement, "DNSServiceRegister %s %s %lu\n", self->serviceName, self->serviceType, self->port);
		while (iterator) {
			UInt32 nameLength = FskStrLen(iterator->name);
			UInt32 valueLength = FskStrLen(iterator->value);
			length = nameLength + 1 + valueLength;
			if (length <= 255) {
				size += length + 1;
				err = txt ? FskMemPtrRealloc(size + 1, &txt) : FskMemPtrNew(size + 1, &txt);
				if (err)
					size -= length + 1;
				else {
					txt[offset++] = (char)length;
					FskStrCopy(txt + offset, iterator->name); offset += nameLength;
					txt[offset++] = '=';
					FskStrCopy(txt + offset, iterator->value); offset += valueLength;
					txt[offset] = 0;
				}
			}
			iterator = FskAssociativeArrayIteratorNext(iterator);
		}
		FskAssociativeArrayIteratorDispose(iterator);

		error = DNSServiceRegister(&serviceRef, 0, 0, self->serviceName, self->serviceType, "", NULL, // use default host name
					htons(self->port), txt ? FskStrLen(txt) : 0, txt, KprZeroconfPlatformAdvertisementProcess, self);
		if (error != kDNSServiceErr_NoError) {
			FskInstrumentedItemPrintfDebug(advertisement, "DNSServiceRegister error %d\n", error);
			bailIfError(kFskErrNetworkErr);
		}
		bailIfError(KprZeroconfPlatformServiceNew(&advertisement->service, NULL, serviceRef, NULL, 0));
	}
bail:
	FskMemPtrDispose(txt);
	return err;
}

FskErr KprZeroconfPlatformAdvertisementStop(KprZeroconfAdvertisement self)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformAdvertisement advertisement = self->platform;
	if (advertisement->service) {
		KprZeroconfPlatformServiceDispose(advertisement->service);
		advertisement->service = NULL;
	}
	return err;
}

#if 0
#pragma mark - KprZeroconfPlatformBrowser
#endif

void DNSSD_API KprZeroconfPlatformGetAddrInfoCallBack(DNSServiceRef serviceRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *hostname, const struct sockaddr *address, uint32_t ttl, void *context)
{
	KprZeroconfBrowser self = context;
	KprZeroconfPlatformBrowser browser = self->platform;
	KprZeroconfPlatformService service = KprZeroconfPlatformServiceFind(browser->services, serviceRef);
	char* serviceType = self->serviceType;
	if (!self->serviceType) {
		serviceType = service->owner->name;
	}
	if (!service || (errorCode != kDNSServiceErr_NoError)) {
		FskInstrumentedItemPrintfDebug(browser, "KprZeroconfPlatformResolveCallBack returned %d\n", errorCode);
	}
	else {
		int addr;
#if TARGET_OS_WIN32
		char ip[256];
		if (address && address->sa_family == AF_INET) {
			const unsigned char *b = (const unsigned char *) &((struct sockaddr_in *)address)->sin_addr;
			snprintf(ip, sizeof(ip), "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
		}
#else
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(((struct sockaddr_in *)address)->sin_addr), ip, INET_ADDRSTRLEN);
#endif
		FskNetStringToIPandPort(ip, &addr, NULL);
		if (FskNetIsLocalNetwork(addr)) {
			KprZeroconfServiceInfo serviceInfo = NULL;
			FskInstrumentedItemPrintfDebug(browser, "GETADDRINFO: %s %s is at %s -> %s:%lu", self->serviceType, service->name, hostname, ip, service->port);
			KprZeroconfServiceInfoNew(&serviceInfo, serviceType, service->name, hostname, ip, service->port, interfaceIndex, service->txt);
			KprZeroconfBrowserServiceUp(self, serviceInfo);
		}
		FskListRemove(&browser->services, service);
		KprZeroconfPlatformServiceDispose(service);
	}
}

static void DNSSD_API KprZeroconfPlatformResolveCallBack(DNSServiceRef resolveRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *hostname, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context)
{
	FskErr err = kFskErrNone;
	KprZeroconfBrowser self = context;
	KprZeroconfPlatformBrowser browser = self->platform;
	KprZeroconfPlatformService resolver = KprZeroconfPlatformServiceFind(browser->services, resolveRef);
	if (!resolver || (errorCode != kDNSServiceErr_NoError)) {
		FskInstrumentedItemPrintfDebug(browser, "KprZeroconfPlatformResolveCallBack returned %d\n", errorCode);
	}
	else {
		DNSServiceErrorType error;
		DNSServiceRef serviceRef;
		KprZeroconfPlatformService service = NULL;
		FskInstrumentedItemPrintfDebug(browser, "RESOLVE: %s %s is at %s:%d", self->serviceType, resolver->name, hostname, ntohs(port));
		error = DNSServiceGetAddrInfo(&serviceRef, 0, interfaceIndex, kDNSServiceProtocol_IPv4, hostname, KprZeroconfPlatformGetAddrInfoCallBack, self);
		if (error != kDNSServiceErr_NoError) {
			bailIfError(kFskErrNetworkErr);
		}
		bailIfError(KprZeroconfPlatformServiceNew(&service, resolver->owner, serviceRef, resolver->name, ntohs(port)));
		if (txtLen > 1) {
			bailIfError(FskMemPtrNewClear(txtLen + 1, &service->txt));
			FskStrNCopy(service->txt, (const char*)txtRecord, txtLen);
		}
		FskListAppend(&browser->services, service);
		FskListRemove(&browser->services, resolver);
		KprZeroconfPlatformServiceDispose(resolver);
	}
bail:
    return;
}

static void DNSSD_API KprZeroconfPlatformBrowseCallback(DNSServiceRef serviceRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char* name, const char* type, const char* domain, void* context)
{
	FskErr err = kFskErrNone;
	KprZeroconfBrowser self = context;
	KprZeroconfPlatformBrowser browser = self->platform;
	char* serviceType = self->serviceType;
	KprZeroconfPlatformService service = NULL;
	if (!serviceType) {
		service = KprZeroconfPlatformServiceFind(browser->types, serviceRef);
		if (service)
			serviceType = service->name;
	}
	if (!serviceType || (errorCode != kDNSServiceErr_NoError)) {
		FskInstrumentedItemPrintfDebug(browser, "KprZeroconfPlatformBrowseCallback returned %d\n", errorCode);
	}
	else if (flags & kDNSServiceFlagsAdd) {
		DNSServiceErrorType error;
		DNSServiceRef resolveRef;
		KprZeroconfPlatformService resolver = NULL;
		FskInstrumentedItemPrintfDebug(browser, "ADD: %d %s %s %s", interfaceIndex, name, type, domain);
		error = DNSServiceResolve(&resolveRef, kDNSServiceFlagsForceMulticast, interfaceIndex, name, type, domain, KprZeroconfPlatformResolveCallBack, self);
		if (error != kDNSServiceErr_NoError) {
			bailIfError(kFskErrNetworkErr);
		}
		bailIfError(KprZeroconfPlatformServiceNew(&resolver, service, resolveRef, name, 0));
		FskListAppend(&browser->services, resolver);
	}
	else {
		KprZeroconfServiceInfo serviceInfo = NULL;
		bailIfError(KprZeroconfServiceInfoNew(&serviceInfo, type, name, NULL, NULL, 0, interfaceIndex, NULL));
		KprZeroconfBrowserServiceDown(self, serviceInfo);
		FskInstrumentedItemPrintfDebug(browser, "REMOVE: %d %s %s %s", interfaceIndex, name, type, domain);
	}
bail:
    return;
}

static void DNSSD_API KprZeroconfPlatformWildcardCallback(DNSServiceRef serviceRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char* name, const char* type, const char* domain, void* context)
{
	FskErr err = kFskErrNone;
	KprZeroconfBrowser self = context;
	KprZeroconfPlatformBrowser browser = self->platform;
	char* ptr = NULL;
	char* serviceType = NULL;
	if (errorCode != kDNSServiceErr_NoError) {
		FskInstrumentedItemPrintfDebug(browser, "KprZeroconfPlatformWildcardCallback returned %d\n", errorCode);
	}
	else if (flags & kDNSServiceFlagsAdd) {
		char* ptr = FskStrStr(type, "local.");
		if (ptr) {
			*ptr = 0;
			bailIfError(FskMemPtrNew(FskStrLen(name) + FskStrLen(type) + 2, &serviceType));
			FskStrCopy(serviceType, name);
			FskStrCat(serviceType, ".");
			FskStrCat(serviceType, type);
			if (!KprZeroconfPlatformServiceFindType(browser->types, serviceType)) {
				KprZeroconfPlatformService service = NULL;
				DNSServiceErrorType error;
				FskInstrumentedItemPrintfDebug(browser, "WILDCARD: %d %s", interfaceIndex, serviceType);
				error = DNSServiceBrowse(&serviceRef, 0, 0, serviceType, NULL, KprZeroconfPlatformBrowseCallback, self);
				if (error != kDNSServiceErr_NoError) {
					bailIfError(kFskErrNetworkErr);
				}
				bailIfError(KprZeroconfPlatformServiceNew(&service, NULL, serviceRef, serviceType, 0));
				FskListAppend(&browser->types, service);
			}
		}
	}
bail:
	if (ptr) *ptr = 'l';
	FskMemPtrDispose(serviceType);
	return;
}

FskErr KprZeroconfPlatformBrowserNew(KprZeroconfBrowser self)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformBrowser browser = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprZeroconfPlatformBrowserRecord), &browser));
	FskInstrumentedItemNew(browser, NULL, &KprZeroconfPlatformBrowserInstrumentation);
	self->platform = browser;
bail:
	if (err)
		KprZeroconfPlatformBrowserDispose(self);
	return err;
}

void KprZeroconfPlatformBrowserDispose(KprZeroconfBrowser self)
{
	KprZeroconfPlatformBrowser browser = self->platform;
	if (browser) {
		KprZeroconfPlatformBrowserStop(self);
		FskInstrumentedItemDispose(browser);
		FskMemPtrDispose(browser);
		self->platform = NULL;
	}
}

FskErr KprZeroconfPlatformBrowserStart(KprZeroconfBrowser self)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformBrowser browser = self->platform;
	if (!browser->service) {
		DNSServiceErrorType error;
		DNSServiceRef  serviceRef;
		if (self->serviceType)
			error = DNSServiceBrowse(&serviceRef, 0, 0, self->serviceType, NULL, KprZeroconfPlatformBrowseCallback, self);
		else
			error = DNSServiceBrowse(&serviceRef, 0, 0, "_services._dns-sd._udp.", NULL, KprZeroconfPlatformWildcardCallback, self);
		if (error != kDNSServiceErr_NoError) {
			bailIfError(kFskErrNetworkErr);
		}
		bailIfError(KprZeroconfPlatformServiceNew(&browser->service, NULL, serviceRef, NULL, 0));
	}
bail:
	return err;
}

FskErr KprZeroconfPlatformBrowserStop(KprZeroconfBrowser self)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformBrowser browser = self->platform;
	if (browser->types) {
		KprZeroconfPlatformService service, next;
		for (service = browser->types; service; service = next) {
			next = service->next;
			KprZeroconfPlatformServiceDispose(service);
		}
		browser->types = NULL;
	}
	if (browser->services) {
		KprZeroconfPlatformService service, next;
		for (service = browser->services; service; service = next) {
			next = service->next;
			KprZeroconfPlatformServiceDispose(service);
		}
		browser->services = NULL;
	}
	if (browser->service) {
		KprZeroconfPlatformServiceDispose(browser->service);
		browser->service = NULL;
	}
	return err;
}
