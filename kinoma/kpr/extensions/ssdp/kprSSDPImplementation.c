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
#define __FSKTHREAD_PRIV__
#define __FSKNETUTILS_PRIV__
#include "FskEnvironment.h"
#include "FskHeaders.h"
#include "FskPlatformImplementation.h"
#include "FskThread.h"
#include "FskUUID.h"

#include "kpr.h"
#include "kprApplication.h"
#include "kprHandler.h"
#include "kprHTTPClient.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprSSDPCommon.h"
#include "kprUtilities.h"

#include "time.h"

enum {
	kprSSDPVariantAll,
	kprSSDPVariantRoot,
	kprSSDPVariantDeviceUUID,
	kprSSDPVariantDeviceType,
	kprSSDPVariantServiceType,
	kprSSDPVariantMax
};

#if SUPPORT_INSTRUMENTATION
static char* gKprSSDPVariants[] = {
	"all",
	"root",
	"uuid",
	"deviceType",
	"serviceType"
};
#define kKprSSDPVariant2String(type) (gKprSSDPVariants[type])
#endif

enum {
	kSSDPPacketAlive,
	kSSDPPacketByebye,
	kSSDPPacketSearch,
	kSSDPPacketSearchResponse,
	kSSDPPacketUpdate
};

#if SUPPORT_INSTRUMENTATION
static char* gKprSSDPTypes[] = {
	"ssdp:alive",
	"ssdp:byebye",
	"ssdp:search",
	"ssdp:discover",
	"ssdp:update"
};
#define kKprSSDPType2String(type) (gKprSSDPTypes[type])
#endif

#define kKprSSDPMulticastAddr	((239 << 24) | (255 << 16) | (255 << 8) | (250))
#define kKprSSDPMulticastPort	1900
#define kKprSSDPReceiveBufferSize (64 * 1024)
#define kKprSSDPSendBufferSize (64 * 1024)
#define kKprSSDPPacketBufSize 2048

#define kKprSSDPNotifyStartLine "NOTIFY * HTTP/1.1\r\n"
#define kKprSSDPNotifyStartLineLength 19

#define kKprSSDPSearchStartLine "M-SEARCH * HTTP/1.1\r\n"
#define kKprSSDPSearchStartLineLength 21

#define kKprSSDPResponseStartLine "HTTP/1.1 200 OK\r\n"
#define kKprSSDPResponseStartLineLength 17

#define KprSSDPPacketRandom ((UInt32)FskRandom())
#define kKprSSDPPacketSpreading 50 + (KprSSDPPacketRandom % 50)
#define kKprSSDPPacketInitialDelay (KprSSDPPacketRandom % 100)

typedef struct KprSSDPStruct KprSSDPRecord, *KprSSDP;
typedef struct KprSSDPInterfaceStruct KprSSDPInterfaceRecord, *KprSSDPInterface;
typedef struct KprSSDPPacketStruct KprSSDPPacketRecord, *KprSSDPPacket;
typedef struct KprSSDPPacketMessageStruct KprSSDPPacketMessageRecord, *KprSSDPPacketMessage;

//--------------------------------------------------
// SSDP
//--------------------------------------------------

struct KprSSDPStruct {
	KprSSDPFilter filters;
	FskAssociativeArray devices;
	FskAssociativeArray discoveries;
	KprSSDPInterface ssdpInterfaces;
	FskNetInterfaceNotifier ssdpInterfaceNotifier;
	// FskThread thread;
	xsMachine* machine;
	UInt32 ttl;
	
	KprSSDPPacket packets;
	FskTimeCallBack packetTimer;
	char* userAgent;
	UInt32 bootId;
	UInt32 repeat;
	Boolean byebye;
	UInt32 byebyeRepeat;
	Boolean discoverSelf;
	UInt32 expire;
	UInt32 mx;
	Boolean searchAll;
	FskInstrumentedItemDeclaration
};

//--------------------------------------------------
// SSDP Device
//--------------------------------------------------

struct KprSSDPDeviceStruct {
	KprSSDPDevice next;
	UInt32 expire;
	char* path;
	UInt32 port;
	char* scheme;
	KprSSDPService services;
	char* type;
	char* userAgent;
	char* uuid;
	SInt32 configId;
	UInt32 searchPort;
	KprSSDPPacket alive;
	KprSSDPPacket byebye;
	KprSSDPPacket update;
	FskInstrumentedItemDeclaration
};

static FskAPI(FskErr) KprSSDPDeviceSendAlive(KprSSDPDevice self, KprSSDP ssdp, Boolean doByebye);
static FskAPI(FskErr) KprSSDPDeviceSendByebye(KprSSDPDevice self, KprSSDP ssdp);
static FskAPI(FskErr) KprSSDPDeviceSendUpdate(KprSSDPDevice self, KprSSDP ssdp, UInt32 bootId);
static FskAPI(FskErr) KprSSDPDeviceSetupAlivePacket(KprSSDPDevice self, KprSSDP ssdp);
static FskAPI(FskErr) KprSSDPDeviceSetupByebyePacket(KprSSDPDevice self, KprSSDP ssdp);
static FskAPI(FskErr) KprSSDPDeviceSetupPackets(KprSSDPDevice self, KprSSDP ssdp);
static FskAPI(FskErr) KprSSDPDeviceSetupUpdatePacket(KprSSDPDevice self, KprSSDP ssdp);

//--------------------------------------------------
// SSDP Discovery Description
//--------------------------------------------------

static FskErr KprSSDPDiscoveryDescriptionFromDiscovery(KprSSDPDiscoveryDescription *it, KprSSDPDiscovery discovery);

//--------------------------------------------------
// SSDP Discovery
//--------------------------------------------------

struct KprSSDPDiscoveryStruct {
	KprSSDP ssdp;
	UInt32 bootId;      // MUST be a non-negative, 31-bit integer
	SInt32 configId;    // MUST be a non-negative, 31-bit integer
	UInt32 expire;
	char* location;
	UInt32 searchPort;  // default to 1900
	char* tag;
	char* type;
	char* uuid;
	KprSSDPService services;
	KprSSDPInterface ssdpInterface;
	FskTimeRecord when;
	FskTimeCallBack timer;
	FskInstrumentedItemDeclaration
};

static FskErr KprSSDPDiscoveryNew(KprSSDPDiscovery *it, KprSSDP ssdp, char* tag, char* uuid, char* type, char* location, UInt32 expire, KprSSDPInterface ssdpInterface);
static void KprSSDPDiscoveryDispose(KprSSDPDiscovery self);
static FskErr KprSSDPDiscoveryAddService(KprSSDPDiscovery self, char* type);
static void KprSSDPDiscoveryPrint(KprSSDPDiscovery self, KprSSDP ssdp);
//static FskErr KprSSDPDiscoveryRemoveService(KprSSDPDiscovery self, KprSSDPService service);
static FskErr KprSSDPDiscoveryUpdateTimer(KprSSDPDiscovery self, UInt32 expire);

//--------------------------------------------------
// SSDP Interface
//--------------------------------------------------

struct KprSSDPInterfaceStruct {
	KprSSDPInterface next;
	KprSSDP ssdp;
	Boolean ready;
	UInt32 ip;
	char MAC[6];
	SInt32 status;
	FskTimeCallBack timer;
	char* title;
	
	FskSocket multicast;
	FskThreadDataHandler multicastHandler;
	FskSocket response;
	FskThreadDataHandler responseHandler;
	FskInstrumentedItemDeclaration
};

static FskErr KprSSDPInterfaceNew(KprSSDPInterface *it, KprSSDP ssdp, FskNetInterfaceRecord *interfaceRec, Boolean notify);
static void KprSSDPInterfaceDispose(KprSSDPInterface self);
static FskErr KprSSDPInterfaceConnect(KprSSDPInterface self, Boolean notify);
static void KprSSDPInterfaceDisconnect(KprSSDPInterface self);
static void KprSSDPInterfaceReadMulticast(FskThreadDataHandler handler UNUSED, FskThreadDataSource source, void *refCon);
static void KprSSDPInterfaceReadResponse(FskThreadDataHandler handler UNUSED, FskThreadDataSource source, void *refCon);

//--------------------------------------------------
// SSDP Packet
//--------------------------------------------------

typedef void (*KprSSDPPacketCallbackProc)(KprSSDP self, KprSSDPPacket packet);

struct KprSSDPPacketStruct {
	KprSSDPPacket next;
	UInt32 size;
	KprSSDPPacketMessage message;
	KprSSDPPacketMessage messages;
	FskTimeRecord when;
	UInt32 delay;
	UInt32 index;
	UInt32 repeat;
	KprSSDPDevice device;
	KprSSDPInterface ssdpInterface;
	UInt32 ip;
	UInt32 port;
	KprSSDPPacketCallbackProc callback;
	UInt32 type;
	FskInstrumentedItemDeclaration
};

static FskAPI(FskErr) KprSSDPPacketNew(KprSSDPPacket *it, UInt32 type, FskTime when, UInt32 repeat, KprSSDPPacketCallbackProc callback);
static FskAPI(void) KprSSDPPacketDispose(KprSSDPPacket self);
static FskAPI(FskErr) KprSSDPPacketAddMessage(KprSSDPPacket self, UInt32 type, UInt32 variant, char* message);
static FskAPI(FskErr) KprSSDPPacketAddAliveMessage(KprSSDPPacket self, KprSSDP ssdp, KprSSDPDevice device, KprSSDPService service, UInt32 variant);
static FskAPI(FskErr) KprSSDPPacketAddByebyeMessage(KprSSDPPacket self, KprSSDP ssdp, KprSSDPDevice device, KprSSDPService service, UInt32 variant);
static FskAPI(FskErr) KprSSDPPacketAddSearchMessage(KprSSDPPacket self, KprSSDP ssdp, char* type, UInt32 variant);
static FskAPI(FskErr) KprSSDPPacketAddResponseMessage(KprSSDPPacket self, KprSSDP ssdp, KprSSDPDevice device, KprSSDPService service, UInt32 variant);
static FskAPI(FskErr) KprSSDPPacketAddUpdateMessage(KprSSDPPacket self, KprSSDP ssdp, KprSSDPDevice device, KprSSDPService service, UInt32 variant);
static char* KprSSDPPacketGetLineBootId(KprSSDP self, KprSSDPDevice device, char* buffer, UInt32 size);
static char* KprSSDPPacketGetLineConfigId(KprSSDP self UNUSED, KprSSDPDevice device, char* buffer, UInt32 size);
static char* KprSSDPPacketGetLineNextBootId(KprSSDP self UNUSED, char* buffer, UInt32 size);
static char* KprSSDPPacketGetLineNT(KprSSDP self UNUSED, KprSSDPDevice device, KprSSDPService service, char* buffer, UInt32 size, UInt32 variant);
static char* KprSSDPPacketGetLineSearchPort(KprSSDP self UNUSED, KprSSDPDevice device, char* buffer, UInt32 size);
static char* KprSSDPPacketGetLineUSN(KprSSDP self UNUSED, KprSSDPDevice device, KprSSDPService service, char* buffer, UInt32 size, UInt32 variant);

//--------------------------------------------------
// SSDP Packet Message
//--------------------------------------------------

struct KprSSDPPacketMessageStruct {
	KprSSDPPacketMessage next;
	char* text;
	UInt32 type;
	UInt32 variant;
	FskInstrumentedItemDeclaration
};

static FskAPI(FskErr) KprSSDPPacketMessageNew(KprSSDPPacketMessage *it, UInt32 type, UInt32 variant, char* text);
static FskAPI(void) KprSSDPPacketMessageDispose(KprSSDPPacketMessage self);


//--------------------------------------------------
// INSTRUMENTATION
//--------------------------------------------------

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord gKprSSDPInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSSDP", FskInstrumentationOffset(KprSSDPRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord gKprSSDPDeviceInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSSDPDevice", FskInstrumentationOffset(KprSSDPDeviceRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord gKprSSDPDiscoveryDescriptionInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSSDPDiscoveryDescription", FskInstrumentationOffset(KprSSDPDiscoveryDescriptionRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord gKprSSDPDiscoveryInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSSDPDiscovery", FskInstrumentationOffset(KprSSDPDiscoveryRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord gKprSSDPFilterInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSSDPFilter", FskInstrumentationOffset(KprSSDPFilterRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord gKprSSDPInterfaceInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSSDPInterface", FskInstrumentationOffset(KprSSDPInterfaceRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord gKprSSDPPacketInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSSDPPacket", FskInstrumentationOffset(KprSSDPPacketRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord gKprSSDPPacketMessageInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSSDPPacketMessage", FskInstrumentationOffset(KprSSDPPacketMessageRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

//--------------------------------------------------
// SSDP
//--------------------------------------------------

// Interface
// Device
// Discovery
static void KprSSDPAddDiscovery(KprSSDP self, KprSSDPDiscovery discovery);
static FskErr KprSSDPAddDiscoveryService(KprSSDP self, KprSSDPDiscovery discovery, char* service);
static void KprSSDPPrintDiscoveries(KprSSDP self);
static void KprSSDPRemoveDiscovery(KprSSDP self, KprSSDPDiscovery discovery);
// Filter
static FskErr KprSSDPNotifyFilter(KprSSDPFilter filter, KprSSDPDiscovery discovery, Boolean alive);
// Packet
static void KprSSDPSchedulePacket(KprSSDP self, KprSSDPPacket packet);
static void KprSSDPSendPacket(FskTimeCallBack timer UNUSED, const FskTime time UNUSED, void *param);
static void KprSSDPSendPacketBootId(KprSSDP self, KprSSDPPacket packet, UInt32 bootId);
static void KprSSDPSendPacketDefault(KprSSDP self, KprSSDPPacket packet);
static void KprSSDPSendPacketInterface(KprSSDP self, KprSSDPPacket packet);
static void KprSSDPSendPacketSearch(KprSSDP self, KprSSDPPacket packet);
// Annoucements
static FskErr KprSSDPProcessNotify(KprSSDP self, KprSSDPInterface ssdpInterface UNUSED, UInt32 ip UNUSED, UInt32 port UNUSED, char* buffer, UInt32 size UNUSED, Boolean multicast UNUSED);
static FskErr KprSSDPProcessSearch(KprSSDP self, KprSSDPInterface ssdpInterface, UInt32 ip, UInt32 port, char* buffer, UInt32 size UNUSED, Boolean multicast);
static FskErr KprSSDPSearchDevices(KprSSDP self, KprSSDPInterface ssdpInterface, KprSSDPFilter filter);
static FskErr KprSSDPSearchAllDevices(KprSSDP self, KprSSDPInterface ssdpInterface);

#if 0
#pragma mark - KprSSDP
#endif

static KprSSDP gSSDP = NULL;
static FskNetInterface gNetworkInterface = NULL;

static void KprSSDPNetworkInterfaceAdd(FskNetInterface iface, Boolean notify);
static void KprSSDPNetworkInterfaceRemove(FskNetInterface iface);

void KprSSDPNetworkInterfaceAdd(FskNetInterface iface, Boolean notify)
{
	FskErr err = kFskErrNone;
	FskNetInterface existing = NULL;
	char ip[32];
	UInt32 notifyIt = notify;
	
	if (iface->ip && (0x7f000001 != iface->ip) && iface->status) {
		bailIfError(FskMemPtrNewFromData(sizeof(FskNetInterfaceRecord), iface, &existing));
		existing->next = NULL;
		existing->name = FskStrDoCopy(iface->name);
		FskListAppend(&gNetworkInterface, existing);
		FskNetIPandPortToString(existing->ip, 0, ip);
		FskDebugStr("ADD SSDP INTERFACE %s %s", existing->name, ip);
		KprSSDPAddInterface(existing, notifyIt);
	}
bail:
	return;
}

int KprSSDPNetworkInterfaceNotifier(struct FskNetInterfaceRecord* iface, UInt32 status, void* param)
{
	FskErr err = kFskErrNone;
	FskDebugStr("KprSSDPInterfaceNotifier %s -> %lu", iface->name, status);
	switch (status) {
		case kFskNetInterfaceStatusNew:
			KprSSDPNetworkInterfaceAdd(iface, true);
		break;
		case kFskNetInterfaceStatusRemoved:
			KprSSDPNetworkInterfaceRemove(iface);
		break;
		case kFskNetInterfaceStatusChanged:
			KprSSDPNetworkInterfaceRemove(iface);
			KprSSDPNetworkInterfaceAdd(iface, true);
		break;
	}
	return err;
}

void KprSSDPNetworkInterfaceRemove(FskNetInterface iface)
{
	char ip[22];
	FskNetInterface existing = NULL;
	
	for (existing = gNetworkInterface; existing; existing = existing->next) {
		if (!FskStrCompare(existing->name, iface->name)) break;
	}
	if (existing) {
		FskListRemove(&gNetworkInterface, existing);
		FskNetIPandPortToString(existing->ip, 0, ip);
		FskDebugStr("REMOVE SSDP INTERFACE %s %s", existing->name, ip);
		KprSSDPRemoveInterface(existing);
		FskNetInterfaceDescriptionDispose(existing);
	}
}

void KprSSDPStart(KprService service, FskThread thread, xsMachine* the)
{
	FskErr err = kFskErrNone;
    KprSSDP self;
    char buffer[256];

	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPRecord), &self));
	FskInstrumentedItemNew(self, NULL, &gKprSSDPInstrumentation);
	self->devices = FskAssociativeArrayNew();
	self->discoveries = FskAssociativeArrayNew();
	self->byebye = KprEnvironmentGetUInt32("ssdpByebye", 1);
	self->byebyeRepeat = KprEnvironmentGetUInt32("ssdpByebyeRepeat", 1);
	self->discoverSelf = KprEnvironmentGetUInt32("ssdpDiscoverSelf", 1);
	self->expire = KprEnvironmentGetUInt32("ssdpExpire", 1800);
	self->mx = KprEnvironmentGetUInt32("ssdpMX", 5);
	self->repeat = KprEnvironmentGetUInt32("ssdpRepeat", 3);
	self->searchAll = KprEnvironmentGetUInt32("ssdpSearchAll", 1);
	self->ttl = KprEnvironmentGetUInt32("ssdpTTL", 2); // TTL sould default to 4 for UPnP 1.0 devices and 2 for UPnP 1.1 devices.
	
	FskTimeCallbackNew(&self->packetTimer);
	snprintf(buffer, sizeof(buffer), "%s/%s UPnP/1.1 Kinoma/1.1", FskEnvironmentGet("OS"), FskEnvironmentGet("OSVersion"));
	self->userAgent = FskStrDoCopy(buffer);
	bailIfNULL(self->userAgent);
	self->bootId = KprDateNow();
	self->machine = service->machine;
	gSSDP = self;
	{
		// NetInterface
		UInt32 i, count;
		count = FskNetInterfaceEnumerate();
		for (i = 0; i < count; i++) {
			FskNetInterface iface;
			FskNetInterfaceDescribe(i, &iface);
			KprSSDPNetworkInterfaceAdd(iface, false);
			FskNetInterfaceDescriptionDispose(iface);
		}
		self->ssdpInterfaceNotifier = FskNetInterfaceAddNotifier(KprSSDPNetworkInterfaceNotifier, NULL, "KprSSDPNetworkInterfaceNotifier");
	}
	
bail:
	return;
}

void KprSSDPStop(KprService service UNUSED)
{
	KprSSDP self = gSSDP;
	if (self) {
		KprSSDPInterface ssdpInterface, nextInterface;
		KprSSDPFilter filter, nextFilter;
		FskAssociativeArrayIterator iterate;
		{
			FskNetInterface existing, next;
			FskNetInterfaceRemoveNotifier(self->ssdpInterfaceNotifier);
			self->ssdpInterfaceNotifier = NULL;
			for (existing = gNetworkInterface; existing; existing = next) {
				next = existing->next;
				FskNetInterfaceDescriptionDispose(existing);
			}
		}
		
		FskTimeCallbackDispose(self->packetTimer);
		self->packetTimer = NULL;

		iterate = FskAssociativeArrayIteratorNew(self->devices);
		while (iterate) {
			KprSSDPDevice device = (KprSSDPDevice)iterate->value;
			KprSSDPDeviceSendByebye(device, self);
			KprSSDPDeviceDispose(device);
			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		FskAssociativeArrayIteratorDispose(iterate);
		FskAssociativeArrayDispose(self->devices);

		while (self->packets) {
			// remove remaining packets
			KprSSDPPacket packet = FskListRemoveFirst(&self->packets);
			KprSSDPPacketDispose(packet);
		}
		
		iterate = FskAssociativeArrayIteratorNew(self->discoveries);
		while (iterate) {
			KprSSDPDiscovery device = (KprSSDPDiscovery)iterate->value;
			KprSSDPDiscoveryDispose(device);
			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		FskAssociativeArrayIteratorDispose(iterate);
		FskAssociativeArrayDispose(self->discoveries);
		
		for (filter = self->filters; filter; filter = nextFilter) {
			nextFilter = filter->next;
			KprSSDPFilterDispose(filter);
		}
		self->filters = NULL;
		
		for (ssdpInterface = self->ssdpInterfaces; ssdpInterface; ssdpInterface = nextInterface) {
			nextInterface = ssdpInterface->next;
			KprSSDPInterfaceDispose(ssdpInterface);
		}
		self->ssdpInterfaces = NULL;
		
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

// Interface

FskErr KprSSDPAddInterface(FskNetInterfaceRecord *interfaceRec, UInt32 notify)
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	KprSSDPInterface ssdpInterface = NULL;
	UInt32 bootId;
	FskAssociativeArrayIterator iterate;
	FskInstrumentedItemPrintfVerbose(self, "KprSSDPAddInterface %s!\n", interfaceRec->name);
	if (notify) {
		bootId = KprDateNow();
		iterate = FskAssociativeArrayIteratorNew(self->devices);
		while (iterate) {
			KprSSDPDevice device = (KprSSDPDevice)iterate->value;
			KprSSDPDeviceSendUpdate(device, self, bootId);
			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		FskAssociativeArrayIteratorDispose(iterate);
		self->bootId = bootId;
	}
	bailIfError(KprSSDPInterfaceNew(&ssdpInterface, self, interfaceRec, true));
bail:
	FskInstrumentedItemPrintfVerbose(self, "KprSSDPAddInterface %s done!\n", ssdpInterface->title);
	return err;
}

FskErr KprSSDPRemoveInterface(FskNetInterfaceRecord *interfaceRec)
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	KprSSDPInterface ssdpInterface = NULL;
	FskInstrumentedItemPrintfVerbose(self, "KprSSDPRemoveInterface %s!\n", interfaceRec->name);
	for (ssdpInterface = self->ssdpInterfaces; ssdpInterface; ssdpInterface = ssdpInterface->next) {
		if (FskStrCompare(ssdpInterface->title, interfaceRec->name) == 0) {
			// cleanup packets
			{
				KprSSDPPacket previous = NULL, packet = self->packets, next;
				while (packet) {
					next = packet->next;
					if (packet->ssdpInterface != ssdpInterface)
						previous = packet;
					else {
						if (previous)
							previous->next = next;
						else
							self->packets = next;
						KprSSDPPacketDispose(packet);
					}
					packet = next;
				}
			}
			// cleanup discoveries
			{
				FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(self->discoveries);
				while (iterate) {
					KprSSDPDiscovery device = (KprSSDPDiscovery)iterate->value;
					iterate = FskAssociativeArrayIteratorNext(iterate);
					if (device->ssdpInterface == ssdpInterface) {
						KprSSDPRemoveDiscovery(self, device);
					}
				}
				FskAssociativeArrayIteratorDispose(iterate);
				KprSSDPPrintDiscoveries(self);
			}

			bailIfError(FskListRemove(&self->ssdpInterfaces, ssdpInterface) ? kFskErrNone : kFskErrUnknownElement);
			FskInstrumentedItemPrintfVerbose(self, "KprSSDPRemoveInterface %s done!\n", ssdpInterface->title);
			KprSSDPInterfaceDispose(ssdpInterface);
			break;
		}
	}
bail:
	return err;
}

// Server

FskErr KprSSDPDiscover(char* urn, char* services[], KprSSDPDiscoveryCallback callback, void* refcon)
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	KprSSDPFilter filter = NULL;
	bailIfError(KprSSDPFilterNew(&filter, NULL, urn, services, callback));
	filter->isDevice = true;
	filter->refcon = refcon;

	bailIfError(KprSSDPAddFilter(filter));
	bailIfError(KprSSDPSearchDevices(self, NULL, filter));
bail:
	return err;
}

void KprSSDPDiscoverServerCallback(char* authority, KprSSDPDiscoveryDescription description, Boolean alive, void* refcon)
{
#define kKPRSSDPKinomaDiscoverURL "xkpr://%s/discover"
#define kKPRSSDPKinomaDiscoverJSON "{\"id\":\"%s\",\"uuid\":\"%s\",\"url\":\"%s\",\"protocol\":\"ssdp\"}"
#define kKPRSSDPKinomaForgetURL "xkpr://%s/forget"
#define kKPRSSDPKinomaForgetJSON "{\"id\":\"%s\",\"uuid\":\"%s\",\"protocol\":\"ssdp\"}"
	FskErr err = kFskErrNone;
	char id[256];
	char url[1024];
	char json[2048];
	UInt32 size;
	KprMessage message = NULL;
	sscanf(description->type, kKPRSSDPKinomaServe, id);
	size = FskStrLen(id);
	id[size - 2] = 0;
	if (alive) {
		snprintf(url, sizeof(url), kKPRSSDPKinomaDiscoverURL, authority);
		snprintf(json, sizeof(json), kKPRSSDPKinomaDiscoverJSON, id, description->uuid, description->url);
	}
	else {
		snprintf(url, sizeof(url), kKPRSSDPKinomaForgetURL, authority);
		snprintf(json, sizeof(json), kKPRSSDPKinomaForgetJSON, id, description->uuid);
	}
	KprSSDPDiscoveryDescriptionDispose(description);
	size = FskStrLen(json);
	bailIfError(KprMessageNew(&message, url));
	bailIfError(KprMessageSetRequestBody(message, json, size));
	FskStrNumToStr(size, id, sizeof(id));
	bailIfError(KprMessageSetRequestHeader(message, kFskStrContentLength, id));
	bailIfError(KprMessageSetRequestHeader(message, kFskStrContentType, "application/json"));
	KprMessageInvoke(message, NULL, NULL, NULL);
	return;
bail:
	KprMessageDispose(message);
}

FskErr KprSSDPDiscoverDevice(char* authority, char* type, char* services[], KprSSDPDiscoveryCallback callback)
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	char buffer[256];
	char* urn = buffer;
	KprSSDPFilter filter = NULL;
	if (!self) return err;
	if (FskStrCompareWithLength(type, "urn:", 4))
		snprintf(urn, sizeof(buffer), kKPRSSDPKinomaServe, type);
	else
		urn = type;
	for (filter = self->filters; filter; filter = filter->next) {
		if (!FskStrCompare(filter->type, urn)) {
			goto search;
		}
	}
	bailIfError(KprSSDPFilterNew(&filter, authority, urn, services, callback));
	filter->isDevice = true;
	bailIfError(KprSSDPAddFilter(filter));
search:
	bailIfError(KprSSDPSearchDevices(self, NULL, filter));
bail:
	if (services) {
		char** ptr = services;
		while (*ptr)
			FskMemPtrDispose(*ptr++);
		FskMemPtrDispose(services);
	}
	FskMemPtrDispose(authority);
	FskMemPtrDispose(type);
	return err;
}

FskErr KprSSDPDiscoverServer(char* authority, char* id)
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	char buffer[256];
	char* urn = buffer;
	KprSSDPFilter filter = NULL;
	if (!self) return err;
	snprintf(urn, sizeof(buffer), kKPRSSDPKinomaServe, id);
	for (filter = self->filters; filter; filter = filter->next) {
		if (!FskStrCompare(filter->type, urn)) {
			goto search;
		}
	}
	bailIfError(KprSSDPFilterNew(&filter, authority, urn, NULL, KprSSDPDiscoverServerCallback));
	bailIfError(KprSSDPAddFilter(filter));
search:
	bailIfError(KprSSDPSearchDevices(self, NULL, filter));
bail:
	return err;
}

FskErr KprSSDPForget(void* refcon)
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	KprSSDPFilter filter = NULL;
	for (filter = self->filters; filter; filter = filter->next) {
		if (filter->refcon == refcon) {
			KprSSDPRemoveFilter(filter);
			KprSSDPFilterDispose(filter);
			break;
		}
	}
	return err;
}

FskErr KprSSDPForgetDevice(char* authority, char* type, char* services[])
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	char buffer[256];
	char* urn = buffer;
	KprSSDPFilter filter = NULL;
	if (!self) return err;
	if (type) {
		if (FskStrCompareWithLength(type, "urn:", 4))
			snprintf(urn, sizeof(buffer), kKPRSSDPKinomaServe, type);
		else
			urn = type;
	}
	for (filter = self->filters; filter; filter = filter->next) {
		if (!FskStrCompare(filter->authority, authority)) {
			if (!type || !FskStrCompare(filter->type, urn)) {
				if (services) {
					UInt16 i;
					KprSSDPService filterService;
					for (i = 0, filterService = filter->services; services[i] && filterService; i++, filterService = filterService->next) {
						if (FskStrCompare(services[i], filterService->type)) break;
					}
					if (!(services[i] || filterService)) {
						KprSSDPRemoveFilter(filter);
						KprSSDPFilterDispose(filter);
					}
				}
				else if (!filter->services) {
					KprSSDPRemoveFilter(filter);
					KprSSDPFilterDispose(filter);
				}
				break;
			}
		}
	}
//bail:
	if (services) {
		char** ptr = services;
		while (*ptr)
			FskMemPtrDispose(*ptr++);
		FskMemPtrDispose(services);
	}
	FskMemPtrDispose(authority);
	FskMemPtrDispose(type);
	return err;
}

FskErr KprSSDPForgetServer(char* authority, char* id)
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	char buffer[256];
	char* urn = buffer;
	KprSSDPFilter filter = NULL;
	if (!self) return err;
	snprintf(urn, sizeof(buffer), kKPRSSDPKinomaServe, id);
	for (filter = self->filters; filter; filter = filter->next) {
		if (!FskStrCompare(filter->authority, authority)) {
			if (!FskStrCompare(filter->type, urn)) {
				KprSSDPRemoveFilter(filter);
				KprSSDPFilterDispose(filter);
				break;
			}
		}
	}
//bail:
	return err;
}

FskErr KprSSDPSearch(void* refcon) {
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	KprSSDPFilter filter = NULL;
	if (!self) return err;
	for (filter = self->filters; filter; filter = filter->next) {
		if (filter->refcon == refcon) {
			bailIfError(KprSSDPSearchDevices(self, NULL, filter));
			break;
		}
	}
bail:
	return err;
}

// Device

FskErr KprSSDPAddDevice(KprSSDPDevice device)
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	if (!self) return err;
	FskInstrumentedItemPrintfDebug(device, "+++ ADD DEVICE %s (%s)", device->uuid, device->type);
	FskAssociativeArrayElementSetReference(self->devices, device->uuid, device);
	bailIfError(KprSSDPDeviceSetupPackets(device, self));
	bailIfError(KprSSDPDeviceSendAlive(device, self, self->byebye));
bail:
	return err;
}

KprSSDPDevice KprSSDPGetDevice(char* uuid)
{
	KprSSDP self = gSSDP;
	KprSSDPDevice device = NULL;
	if (!self) goto bail;
	device = FskAssociativeArrayElementGetReference(self->devices, uuid);
bail:
	return device;
}

FskErr KprSSDPRemoveDevice(char* uuid)
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	KprSSDPDevice device;
	if (!self) goto bail;
	device = FskAssociativeArrayElementGetReference(self->devices, uuid);
	if (device) {
		FskInstrumentedItemPrintfDebug(device, "--- REMOVE DEVICE %s", uuid);
		KprSSDPDeviceSendByebye(device, self);
		KprSSDPDeviceDispose(device);
		FskAssociativeArrayElementDispose(self->devices, uuid);
	}
bail:
	FskMemPtrDispose(uuid);
	return err;
}

// Discovery

void KprSSDPAddDiscovery(KprSSDP self, KprSSDPDiscovery discovery)
{
	KprSSDPFilter filter;
	KprSSDPDiscovery exist;
	
	FskAssociativeArrayElementSetReference(self->discoveries, discovery->tag, discovery);
	// notify filters
	for (filter = self->filters; filter; filter = filter->next) {
		if ((!filter->type && discovery->type) || (filter->type && !FskStrCompare(filter->type, discovery->type))) {
			exist = (KprSSDPDiscovery)FskAssociativeArrayElementGetReference(filter->discoveries, discovery->tag);
			if (!exist) {
				if (filter->services) {
					KprSSDPService filterService;
					KprSSDPService discoveryService = NULL;
					
					for (filterService = filter->services; filterService; filterService = filterService->next) {
						for (discoveryService = discovery->services; discoveryService; discoveryService = discoveryService->next) {
							if (!FskStrCompare(filterService->type, discoveryService->type)) break;
						}
						if (!discoveryService) break;
					}
					if (!discoveryService) continue;
				}
				// notify filter target that a device is discovered
				FskAssociativeArrayElementSetReference(filter->discoveries, discovery->tag, discovery);
				KprSSDPNotifyFilter(filter, discovery, true);
			}
		}
	}
}

FskErr KprSSDPAddDiscoveryService(KprSSDP self, KprSSDPDiscovery discovery, char* service)
{
	FskErr err = kFskErrNone;
	KprSSDPFilter filter;
	KprSSDPDiscovery exist;

	bailIfError(KprSSDPDiscoveryAddService(discovery, service));
	// notify filters
	for (filter = self->filters; filter; filter = filter->next) {
		if ((!filter->type && discovery->type) || (filter->type && !FskStrCompare(filter->type, discovery->type))) {
			exist = (KprSSDPDiscovery)FskAssociativeArrayElementGetReference(filter->discoveries, discovery->tag);
			if (!filter->type || !exist) {
				KprSSDPService filterService;
				KprSSDPService discoveryService = NULL;
				
				for (filterService = filter->services; filterService; filterService = filterService->next) {
					for (discoveryService = discovery->services; discoveryService; discoveryService = discoveryService->next) {
						if (!FskStrCompare(filterService->type, discoveryService->type)) break;
					}
					if (!discoveryService) break;
				}
				if ((!filter->type && !filter->services) || discoveryService) {
					// notify filter target that a device service is discovered
					FskAssociativeArrayElementSetReference(filter->discoveries, discovery->tag, discovery);
					KprSSDPNotifyFilter(filter, discovery, true);
				}
			}
		}
	}
bail:
	if (err == kFskErrDuplicateElement) err = kFskErrNone;
	return err;
}

void KprSSDPPrintDiscoveries(KprSSDP self)
{
	FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(self->discoveries);
	FskInstrumentedItemPrintfVerbose(self, "");
	FskInstrumentedItemPrintfVerbose(self, "DEVICE LIST");
	while (iterate) {
		KprSSDPDiscovery device = (KprSSDPDiscovery)iterate->value;
		KprSSDPDiscoveryPrint(device, self);
		iterate = FskAssociativeArrayIteratorNext(iterate);
	}
	FskAssociativeArrayIteratorDispose(iterate);
	FskInstrumentedItemPrintfVerbose(self, "");
}

void KprSSDPRemoveDiscovery(KprSSDP self, KprSSDPDiscovery discovery)
{
	KprSSDPFilter filter;
	KprSSDPDiscovery exist;

	FskAssociativeArrayElementDispose(self->discoveries, discovery->tag);
	// notify filters
	for (filter = self->filters; filter; filter = filter->next) {
		if (!filter->type || !FskStrCompare(filter->type, discovery->type)) {
			exist = (KprSSDPDiscovery)FskAssociativeArrayElementGetReference(filter->discoveries, discovery->tag);
			if (exist) {
				// notify filter target that a device is removed
				FskAssociativeArrayElementDispose(filter->discoveries, discovery->tag);
				KprSSDPNotifyFilter(filter, discovery, false);
			}
		}
	}
	// dispose discovery
	KprSSDPDiscoveryDispose(discovery);
}

FskErr KprSSDPRemoveDiscoveryByUUID(char* uuid)
{
	KprSSDP self = gSSDP;
	KprSSDPDiscovery discovery = NULL;
	FskAssociativeArrayIterator iterate = NULL, next = FskAssociativeArrayIteratorNew(self->discoveries);
	
	FskInstrumentedItemPrintfVerbose(self, "KprSSDPRemoveDiscoveryByUUID %s", uuid);
	while (next) {
		iterate = next;
		discovery = (KprSSDPDiscovery)iterate->value;
		next = FskAssociativeArrayIteratorNext(iterate);
		if (!FskStrCompare(discovery->uuid, uuid))
			KprSSDPRemoveDiscovery(self, discovery);
	}
	FskAssociativeArrayIteratorDispose(iterate);
	FskMemPtrDispose(uuid);
	KprSSDPPrintDiscoveries(self);
	return kFskErrNone;
}

FskErr KprSSDPRemoveDiscoveryByLocation(char* location)
{
	KprSSDP self = gSSDP;
	KprSSDPDiscovery discovery = NULL;
	FskAssociativeArrayIterator iterate = NULL, next = FskAssociativeArrayIteratorNew(self->discoveries);
	
	FskInstrumentedItemPrintfVerbose(self, "KprSSDPRemoveDiscoveryByLocation %s", location);
	while (next) {
		iterate = next;
		discovery = (KprSSDPDiscovery)iterate->value;
		next = FskAssociativeArrayIteratorNext(iterate);
		if (!FskStrCompare(discovery->location, location))
			KprSSDPRemoveDiscovery(self, discovery);
	}
	FskAssociativeArrayIteratorDispose(iterate);
	FskMemPtrDispose(location);
	KprSSDPPrintDiscoveries(self);
	return kFskErrNone;
}

// Filter

FskErr KprSSDPNotifyFilter(KprSSDPFilter filter, KprSSDPDiscovery discovery, Boolean alive)
{
	FskErr err = kFskErrNone;
	FskAssociativeArrayIterator iterate = NULL;
	KprSSDPDiscoveryDescription description = NULL;
	
	if (filter->refcon || filter->isDevice) {
		UInt32 flag = alive;
		if (discovery) {
			bailIfError(KprSSDPDiscoveryDescriptionFromDiscovery(&description, discovery));
			FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)filter->callback, filter->authority, description, (void*)flag, filter->refcon);
		}
		else {
			iterate = FskAssociativeArrayIteratorNew(filter->discoveries);
			while (iterate) {
				discovery = (KprSSDPDiscovery)iterate->value;
				bailIfError(KprSSDPDiscoveryDescriptionFromDiscovery(&description, discovery));
				FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)filter->callback, filter->authority, description, (void*)flag, filter->refcon);
				iterate = FskAssociativeArrayIteratorNext(iterate);
			}
		}
	}
	else { // servers
		if (discovery) {
			bailIfError(KprSSDPDiscoveryDescriptionFromDiscovery(&description, discovery));
			(*filter->callback)(filter->authority, description, alive, NULL);
		}
		else {
			iterate = FskAssociativeArrayIteratorNew(filter->discoveries);
			while (iterate) {
				discovery = (KprSSDPDiscovery)iterate->value;
				bailIfError(KprSSDPDiscoveryDescriptionFromDiscovery(&description, discovery));
				(*filter->callback)(filter->authority, description, alive, NULL);
				iterate = FskAssociativeArrayIteratorNext(iterate);
			}
		}
	}
bail:
	FskAssociativeArrayIteratorDispose(iterate);
	return err;
}

FskErr KprSSDPAddFilter(KprSSDPFilter filter)
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	FskAssociativeArrayIterator iterate;
	Boolean notify = false;

	if (!self) return err;
	FskListAppend(&self->filters, filter);
	iterate = FskAssociativeArrayIteratorNew(self->discoveries);
	while (iterate) {
		KprSSDPDiscovery exist, discovery = (KprSSDPDiscovery)iterate->value;
		if (!filter->type || !FskStrCompare(filter->type, discovery->type)) {
			exist = (KprSSDPDiscovery)FskAssociativeArrayElementGetReference(filter->discoveries, discovery->tag);
			if (!filter->type || !exist) {
				KprSSDPService filterService;
				KprSSDPService discoveryService = NULL;
				Boolean valid = true;
				
				for (filterService = filter->services; filterService && valid; filterService = filterService->next) {
					for (discoveryService = discovery->services; discoveryService; discoveryService = discoveryService->next) {
						if (!FskStrCompare(filterService->type, discoveryService->type)) break;
					}
					if (!discoveryService) valid = false;
				}
				if (valid) {
					notify = true;
					FskAssociativeArrayElementSetReference(filter->discoveries, discovery->tag, discovery);
				}
			}
		}
		
		iterate = FskAssociativeArrayIteratorNext(iterate);
	}
	FskAssociativeArrayIteratorDispose(iterate);
	if (notify)
		KprSSDPNotifyFilter(filter, NULL, true);
	return err;
}

FskErr KprSSDPRemoveFilter(KprSSDPFilter filter)
{
	FskErr err = kFskErrNone;
	KprSSDP self = gSSDP;
	if (!self) return err;
	KprSSDPNotifyFilter(filter, NULL, false);
	bailIfError(FskListRemove(&self->filters, filter) ? kFskErrNone : kFskErrUnknownElement);
bail:
	return err;
}

// Packet

void KprSSDPSchedulePacket(KprSSDP self, KprSSDPPacket packet)
{
	KprSSDPPacket current, previous = NULL;
	
	for (current = self->packets; current; current = current->next) {
		if (FskTimeCompare(&packet->when, &current->when) <= 0)
			previous = current;
		else
			break;
	}
	if (previous)
		FskListInsertAfter(&self->packets, packet, previous);
	else {
		FskListPrepend(&self->packets, packet);
		if (self->packetTimer)
			FskTimeCallbackSet(self->packetTimer, &self->packets->when, KprSSDPSendPacket, self);
	}
}

void KprSSDPSendPacket(FskTimeCallBack timer UNUSED, const FskTime time UNUSED, void *param)
{
	KprSSDP self = param;
	KprSSDPPacket packet = NULL;
	
	if (!self->packets) return;
	packet = FskListRemoveFirst(&self->packets);
	(*packet->callback)(self, packet);
	if (self->packets && self->packetTimer)
		FskTimeCallbackSet(self->packetTimer, &self->packets->when, KprSSDPSendPacket, self);
}

void KprSSDPSendPacketBootId(KprSSDP self, KprSSDPPacket packet, UInt32 bootId)
{
	KprSSDPInterface ssdpInterface = NULL;
	int	length;
	char buffer[kKprSSDPPacketBufSize];
	char ip[64];
	
	for (ssdpInterface = self->ssdpInterfaces; ssdpInterface; ssdpInterface = ssdpInterface->next) {
		if (ssdpInterface->ready) {
			KprSSDPPacketMessage message;
			FskNetIPandPortToString(ssdpInterface->ip, 0, ip);
			for (message = packet->messages; message; message = message->next) {
				snprintf(buffer, sizeof(buffer), message->text, ip, bootId);
				FskInstrumentedItemPrintfVerbose(ssdpInterface, "%s -> M - %s %s", ip, kKprSSDPType2String(packet->type), kKprSSDPVariant2String(message->variant));
				FskInstrumentedItemPrintfDebug(ssdpInterface, "%s", buffer);
				FskNetSocketSendUDP(ssdpInterface->response, buffer, FskStrLen(buffer), &length, kKprSSDPMulticastAddr, kKprSSDPMulticastPort);
			}
		}
	}
}

void KprSSDPSendPacketDefault(KprSSDP self, KprSSDPPacket packet)
{
	KprSSDPInterface ssdpInterface = NULL;
	int	length;
	char* buffer;
	char ip[64];
	
	if (packet->ssdpInterface) {
		ssdpInterface = packet->ssdpInterface;
		if (ssdpInterface->ready) {
			KprSSDPPacketMessage message;
			FskNetIPandPortToString(ssdpInterface->ip, 0, ip);
			for (message = packet->messages; message; message = message->next) {
				buffer = message->text;
				FskInstrumentedItemPrintfVerbose(ssdpInterface, "%s -> M - %s %s %s", ip, kKprSSDPType2String(packet->type), kKprSSDPVariant2String(message->variant), packet->device ? packet->device->type : "");
				FskInstrumentedItemPrintfDebug(ssdpInterface, "%s", buffer);
				FskNetSocketSendUDP(ssdpInterface->response, buffer, FskStrLen(buffer), &length, kKprSSDPMulticastAddr, kKprSSDPMulticastPort);
			}
		}
	}
	else {
		for (ssdpInterface = self->ssdpInterfaces; ssdpInterface; ssdpInterface = ssdpInterface->next) {
			if (ssdpInterface->ready) {
				KprSSDPPacketMessage message;
				FskNetIPandPortToString(ssdpInterface->ip, 0, ip);
				for (message = packet->messages; message; message = message->next) {
					buffer = message->text;
					FskInstrumentedItemPrintfVerbose(ssdpInterface, "%s -> M - %s %s %s", ip, kKprSSDPType2String(packet->type), kKprSSDPVariant2String(message->variant), packet->device ? packet->device->type : "");
					FskInstrumentedItemPrintfDebug(ssdpInterface, "%s", buffer);
					FskNetSocketSendUDP(ssdpInterface->response, buffer, FskStrLen(buffer), &length, kKprSSDPMulticastAddr, kKprSSDPMulticastPort);
				}
			}
		}
	}
	if (packet->repeat == 1) {
		if (packet->type == kSSDPPacketByebye) {
			if (packet->device->alive) { // byebye -> alive
				packet = packet->device->alive;
				FskTimeGetNow(&packet->when);
				FskTimeAddMS(&packet->when, kKprSSDPPacketSpreading);
				FskInstrumentedItemPrintfVerbose(self, "schedule alive for %ld.%03ld for %s", packet->when.seconds, packet->when.useconds / 1000, packet->device->type);
				KprSSDPSchedulePacket(self, packet);
			}
			else	// final byebye, dispose device
				KprSSDPDeviceDispose(packet->device);
		}
		else
			KprSSDPPacketDispose(packet);
	}
	else {
		packet->repeat--;
		FskTimeGetNow(&packet->when);
		FskTimeAddMS(&packet->when, kKprSSDPPacketSpreading);
		KprSSDPSchedulePacket(self, packet);
	}
	if (self->packets && self->packetTimer)
		FskTimeCallbackSet(self->packetTimer, &self->packets->when, KprSSDPSendPacket, self);
}

void KprSSDPSendPacketInterface(KprSSDP self, KprSSDPPacket packet)
{
	KprSSDPInterface ssdpInterface = NULL;
	int	length;
	char buffer[kKprSSDPPacketBufSize];
	char ip[64];
	KprSSDPPacketMessage message = NULL;
	
	message = packet->message ? packet->message : packet->messages;
	for (ssdpInterface = self->ssdpInterfaces; ssdpInterface; ssdpInterface = ssdpInterface->next) {
		if (ssdpInterface->ready) {
			FskNetIPandPortToString(ssdpInterface->ip, 0, ip);
			snprintf(buffer, sizeof(buffer), message->text, ip);
			FskInstrumentedItemPrintfVerbose(ssdpInterface, "%s -> M - %s %s %s", ip, kKprSSDPType2String(packet->type), kKprSSDPVariant2String(message->variant), packet->device->type);
			FskInstrumentedItemPrintfDebug(ssdpInterface, "%s", buffer);
			FskNetSocketSendUDP(ssdpInterface->response, buffer, FskStrLen(buffer), &length, kKprSSDPMulticastAddr, kKprSSDPMulticastPort);
		}
	}
	packet->message = message->next;
	if (packet->repeat && !packet->message) {
		packet->repeat--;
	}
	FskTimeGetNow(&packet->when);
	if (packet->repeat) {
		FskTimeAddMS(&packet->when, kKprSSDPPacketSpreading);
	}
	else {
		UInt32 seconds = (packet->device->expire) / (3 * packet->size); // spread advertisements (3 times per expire period to be safe)
		if (!seconds) seconds = 1;
		FskTimeAddSecs(&packet->when, seconds);
		FskTimeAddMS(&packet->when, (KprSSDPPacketRandom % 500)); // delay 0 to 500 ms
	}
	KprSSDPSchedulePacket(self, packet);
		
	if (self->packets && self->packetTimer)
		FskTimeCallbackSet(self->packetTimer, &self->packets->when, KprSSDPSendPacket, self);
}

void KprSSDPSendPacketSearch(KprSSDP self, KprSSDPPacket packet)
{
	KprSSDPInterface ssdpInterface = packet->ssdpInterface;
	int	length;
	char buffer[kKprSSDPPacketBufSize];
	char ip[64];
	KprSSDPPacketMessage message = packet->message ? packet->message : packet->messages;
	
	FskNetIPandPortToString(ssdpInterface->ip, 0, ip);
	snprintf(buffer, sizeof(buffer), message->text, ip);
	if (ssdpInterface->ready) {
		FskInstrumentedItemPrintfVerbose(ssdpInterface, "%s -> U %ld.%ld.%ld.%ld:%ld - %s %s %s", ip,
			(packet->ip >> 24) & 255, (packet->ip >> 16) & 255, (packet->ip >> 8) & 255, packet->ip & 255, packet->port,
			kKprSSDPType2String(packet->type), kKprSSDPVariant2String(message->variant), packet->device->type);
		FskInstrumentedItemPrintfDebug(ssdpInterface, "%s", buffer);
		FskNetSocketSendUDP(ssdpInterface->response, buffer, FskStrLen(buffer), &length, packet->ip, packet->port);
	}
	packet->index++;
	packet->message = message->next;
	if (packet->message) {
		FskTimeGetNow(&packet->when);
		FskTimeAddMS(&packet->when, KprSSDPPacketRandom % packet->delay); // spread in MX
		KprSSDPSchedulePacket(self, packet);
	}
	else {
		KprSSDPPacketDispose(packet);
	}
	
	if (self->packets && self->packetTimer)
		FskTimeCallbackSet(self->packetTimer, &self->packets->when, KprSSDPSendPacket, self);
}

// Annoucements

FskErr KprSSDPProcessNotify(KprSSDP self, KprSSDPInterface ssdpInterface UNUSED, UInt32 ip UNUSED, UInt32 port UNUSED, char* buffer, UInt32 size UNUSED, Boolean multicast)
{
	FskErr err = kFskErrNone;
	char* key;
	char* value;
	char* ptr;
//	char* host = NULL;
	UInt32 expire = 0;
	char* location = NULL;
	char* nt = NULL;
	char* nts = NULL;
//	char* server = NULL;
	char* usn = NULL;
//	UInt32 bootId = 0;
//	SInt32 configId = 0;
//	UInt32 searchPort = 0;
	KprSSDPDiscovery discovery = NULL;
	KprSSDPDiscovery newDiscovery = NULL;
	char* uuid = NULL;
	char* tag = NULL;
	
	while ((ptr = FskStrChr(buffer, ':'))) {
		*ptr = 0;
		key = FskStrStripHeadSpace(buffer);
		FskStrStripTailSpace(key);
		ptr++;
		value = FskStrStripHeadSpace(ptr);
		ptr = FskStrStr(value, "\r\n");
		bailIfNULL(ptr);
		*ptr = 0;
		FskStrStripTailSpace(value);
		buffer = ptr + 2;
		
/*		if (!FskStrCompareCaseInsensitive(key, kFskStrHost))
			host = value;
		else */ if (!FskStrCompareCaseInsensitive(key, "CACHE-CONTROL")) {
			char* string = NULL;
			if (!FskStrCompareWithLength(value, "max-age", 7)) {
				string = FskStrChr(value + 7, '=');
			}
			if (string)
				expire = FskStrToNum(string + 1);
			else
				BAIL(kFskErrBadData);
		}
		else if (!FskStrCompareCaseInsensitive(key, "LOCATION"))
			location = value;
		else if (!FskStrCompareCaseInsensitive(key, "NT"))
			nt = value;
		else if (!FskStrCompareCaseInsensitive(key, "ST")) {
			nt = value;
			nts = "ssdp:alive";
		}
		else if (!FskStrCompareCaseInsensitive(key, "NTS"))
			nts = value;
/*		else if (!FskStrCompareCaseInsensitive(key, "SERVER"))
			server = value; */
		else if (!FskStrCompareCaseInsensitive(key, "USN"))
			usn = value;
/*		else if (!FskStrCompareCaseInsensitive(key, "BOOTID.UPNP.ORG"))
			bootId = FskStrToNum(value); */
/*		else if (!FskStrCompareCaseInsensitive(key, "CONFIGID.UPNP.ORG"))
			configId = FskStrToNum(value); */
/*		else if (!FskStrCompareCaseInsensitive(key, "SEARCHPORT.UPNP.ORG"))
			searchPort = FskStrToNum(value); */
	}
//	if (FskStrCompare(buffer, "\r\n"))
//		BAIL(kFskErrBadData);
	if (!nt || !usn)
		BAIL(kFskErrBadData);
	uuid = usn + 5;
	if (!FskStrCompareWithLength(nt, "uuid:", 5)) {
		if (FskStrCompare(nt, usn)) BAIL(kFskErrBadData);
	}
	else {
		char* separator = FskStrStr(uuid, "::");
		if (separator) {
			*separator = 0;
		}
	}
	
	bailIfError(FskMemPtrNewClear(FskStrLen(ssdpInterface->title) + 1 + FskStrLen(uuid) + 1, &tag));
	FskStrCat(tag, ssdpInterface->title);
	FskStrCat(tag, "-");
	FskStrCat(tag, uuid);

	discovery = (KprSSDPDiscovery)FskAssociativeArrayElementGetReference(self->discoveries, tag);

	FskInstrumentedItemPrintfVerbose(ssdpInterface, "%lu.%lu.%lu.%lu <- %s %lu.%lu.%lu.%lu:%lu - %s %s %s", (ssdpInterface->ip >> 24) & 255, (ssdpInterface->ip >> 16) & 255, (ssdpInterface->ip >> 8) & 255, ssdpInterface->ip & 255,
		multicast ? "M" : "U",
		(ip >> 24) & 255, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255, port,
		nts, nt, location ? location : "");
	if (!FskStrCompare(nts, "ssdp:alive")) {
		UInt32 variant;
		
		if (!FskStrCompare(nt, "ssdp:all"))
			variant = kprSSDPVariantAll;
		else if (!FskStrCompare(nt, "upnp:rootdevice"))
			variant = kprSSDPVariantRoot;
		else if (!FskStrCompareWithLength(nt, "uuid:", 5))
			variant = kprSSDPVariantDeviceUUID;
		else if (!FskStrCompareWithLength(nt, "urn:schemas-upnp-org:device:", 28))
			variant = kprSSDPVariantDeviceType;
		else if (!FskStrCompareWithLength(nt, "urn:schemas-upnp-org:service:", 29))
			variant = kprSSDPVariantServiceType;
		else if (FskStrStr(nt, ":device:"))
			variant = kprSSDPVariantDeviceType;
		else if (FskStrStr(nt, ":service:"))
			variant = kprSSDPVariantServiceType;
		else
			BAIL(kFskErrBadData);
		if (discovery && FskStrCompare(discovery->location, location)) {
			FskInstrumentedItemPrintfVerbose(ssdpInterface, "location changed %s -> %s", discovery->location, location);
			KprSSDPRemoveDiscovery(self, discovery);
			discovery = NULL;
		}
		if (discovery) {
			switch (variant) {
				case kprSSDPVariantAll:
				case kprSSDPVariantRoot:
				case kprSSDPVariantDeviceUUID:
					break;
				case kprSSDPVariantDeviceType:
					if (!discovery->type) {
						discovery->type = FskStrDoCopy(nt);
						bailIfNULL(discovery->type);
						KprSSDPAddDiscovery(self, discovery);
						KprSSDPPrintDiscoveries(self);
					}
					break;
				case kprSSDPVariantServiceType:
					bailIfError(KprSSDPAddDiscoveryService(self, discovery, nt));
					break;
			}
			KprSSDPDiscoveryUpdateTimer(discovery, expire);
		}
		else {
			switch (variant) {
				case kprSSDPVariantAll:
				case kprSSDPVariantRoot:
				case kprSSDPVariantDeviceUUID:
				//	bailIfError(KprSSDPDiscoveryNew(&newDiscovery, self, tag, uuid, NULL, location, expire, ssdpInterface));
					break;
				case kprSSDPVariantDeviceType:
					bailIfError(KprSSDPDiscoveryNew(&newDiscovery, self, tag, uuid, nt, location, expire, ssdpInterface));
					break;
				case kprSSDPVariantServiceType:
					bailIfError(KprSSDPDiscoveryNew(&newDiscovery, self, tag, uuid, NULL, location, expire, ssdpInterface));
					bailIfError(KprSSDPAddDiscoveryService(self, newDiscovery, nt));
					break;
			}
			if (newDiscovery) {
				KprSSDPAddDiscovery(self, newDiscovery);
				KprSSDPPrintDiscoveries(self);
				KprSSDPDiscoveryUpdateTimer(newDiscovery, expire);
			}
		}
	}
	else if (!FskStrCompare(nts, "ssdp:byebye")) {
		if (!discovery) goto bail;
		KprSSDPRemoveDiscovery(self, discovery);
		KprSSDPPrintDiscoveries(self);
	}
	else if (!FskStrCompare(nts, "ssdp:update")) {
	
	}
	
bail:
	if (err)
		KprSSDPDiscoveryDispose(newDiscovery);
	FskMemPtrDispose(tag);
	return err;
}

FskErr KprSSDPProcessSearch(KprSSDP self, KprSSDPInterface ssdpInterface, UInt32 ip, UInt32 port, char* buffer, UInt32 size UNUSED, Boolean multicast)
{
	FskErr err = kFskErrNone;
	char* key;
	char* value;
	char* ptr;
//	char* agent = NULL;
	SInt32 delay = -1;
//	char* host = NULL;
	char* man = NULL;
	char* st = NULL;
	
	while ((ptr = FskStrChr(buffer, ':'))) {
		*ptr = 0;
		key = FskStrStripHeadSpace(buffer);
		FskStrStripTailSpace(key);
		ptr++;
		value = FskStrStripHeadSpace(ptr);
		ptr = FskStrStr(value, "\r\n");
		bailIfNULL(ptr);
		*ptr = 0;
		FskStrStripTailSpace(value);
		buffer = ptr + 2;
		
/*		if (!FskStrCompareCaseInsensitive(key, kFskStrHost))
			host = value;
		else */ if (!FskStrCompareCaseInsensitive(key, "MAN")) {
			man = value;
		}
		else if (!FskStrCompareCaseInsensitive(key, "MX")) {
			if (!multicast)
				BAIL(kFskErrBadData);
			delay = FskStrToNum(value);
			if (delay > 5)
				delay = 5;
			else if (delay < 1)
				delay = 1;
		}
		else if (!FskStrCompareCaseInsensitive(key, "ST"))
			st = value;
/*		else if (!FskStrCompareCaseInsensitive(key, "USER-AGENT"))
			agent = value; */
	}
	if (delay < 0)
		BAIL(kFskErrBadData);
	if (FskStrCompare(buffer, "\r\n"))
		BAIL(kFskErrBadData);

	if (!FskStrCompare(man, "\"ssdp:discover\"")) {
		UInt32 variant;
		FskTimeRecord when;
		FskAssociativeArrayIterator iterate;
		
		if (!st)
			BAIL(kFskErrBadData);
		FskInstrumentedItemPrintfVerbose(ssdpInterface, "%lu.%lu.%lu.%lu <- M %lu.%lu.%lu.%lu:%lu - ssdp:discover %s", (ssdpInterface->ip >> 24) & 255, (ssdpInterface->ip >> 16) & 255, (ssdpInterface->ip >> 8) & 255, ssdpInterface->ip & 255,
			(ip >> 24) & 255, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255, port,
			st);
		if (!FskStrCompare(st, "ssdp:all"))
			variant = kprSSDPVariantAll;
		else if (!FskStrCompare(st, "upnp:rootdevice"))
			variant = kprSSDPVariantRoot;
		else if (!FskStrCompareWithLength(st, "uuid:", 5))
			variant = kprSSDPVariantDeviceUUID;
		else if (!FskStrCompareWithLength(st, "urn:schemas-upnp-org:device:", 28))
			variant = kprSSDPVariantDeviceType;
		else if (!FskStrCompareWithLength(st, "urn:schemas-upnp-org:service:", 29))
			variant = kprSSDPVariantServiceType;
		else if (FskStrStr(st, ":device:"))
			variant = kprSSDPVariantDeviceType;
		else if (FskStrStr(st, ":service:"))
			variant = kprSSDPVariantServiceType;
		else
			BAIL(kFskErrBadData);
		
		FskTimeGetNow(&when);
		iterate = FskAssociativeArrayIteratorNew(self->devices);
		while (iterate) {
			KprSSDPPacket packet = NULL;
			KprSSDPDevice device = (KprSSDPDevice)iterate->value;
			KprSSDPService service = NULL;
			bailIfError(KprSSDPPacketNew(&packet, kSSDPPacketSearchResponse, &when, 0, KprSSDPSendPacketSearch));
			switch (variant) {
				case kprSSDPVariantAll:
					KprSSDPPacketAddResponseMessage(packet, self, device, NULL, kprSSDPVariantRoot);
					KprSSDPPacketAddResponseMessage(packet, self, device, NULL, kprSSDPVariantDeviceUUID);
					KprSSDPPacketAddResponseMessage(packet, self, device, NULL, kprSSDPVariantDeviceType);
					for (service = device->services; service; service = service->next) {
						KprSSDPPacketAddResponseMessage(packet, self, device, service, kprSSDPVariantServiceType);
					}
					break;
				case kprSSDPVariantRoot:
					KprSSDPPacketAddResponseMessage(packet, self, device, NULL, kprSSDPVariantRoot);
					break;
				case kprSSDPVariantDeviceUUID:
					if (!FskStrCompareCaseInsensitive(device->uuid, st + 5)) {
						KprSSDPPacketAddResponseMessage(packet, self, device, NULL, kprSSDPVariantDeviceUUID);
					}
					break;
				case kprSSDPVariantDeviceType:
					if (!FskStrCompare(device->type, st)) {
						KprSSDPPacketAddResponseMessage(packet, self, device, NULL, kprSSDPVariantDeviceType);
					}
					break;
				case kprSSDPVariantServiceType:
					for (service = device->services; service; service = service->next) {
						if (!FskStrCompare(service->type, st)) {
							KprSSDPPacketAddResponseMessage(packet, self, device, service, kprSSDPVariantServiceType);
						}
					}
					break;
			}
			if (packet->messages) {
				packet->ssdpInterface = ssdpInterface;
				packet->ip = ip;
				packet->port = port;
				packet->device = device;
				if (delay > 1) delay -= 1; // safety for delayed UDP packets (UPnP 1.1 arch 1.3.3)
				packet->delay = (delay * 1000) / packet->size; // UPnP UDA-1.2.3 can fail as there is no access to send time
				FskTimeAddMS(&packet->when, KprSSDPPacketRandom % packet->delay);
				KprSSDPSchedulePacket(self, packet);
			}
			else
				KprSSDPPacketDispose(packet);
			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		FskAssociativeArrayIteratorDispose(iterate);
	}
	else
		BAIL(kFskErrBadData);
bail:
	return err;
}

FskErr KprSSDPSearchDevices(KprSSDP self, KprSSDPInterface ssdpInterface, KprSSDPFilter filter)
{
	FskErr err = kFskErrNone;
	KprSSDPService service;
	KprSSDPPacket packet = NULL;
	FskTimeRecord when;
	
	FskTimeGetNow(&when);
	bailIfError(KprSSDPPacketNew(&packet, kSSDPPacketSearch, &when, self->repeat, KprSSDPSendPacketDefault));
	packet->ssdpInterface = ssdpInterface;
	bailIfError(KprSSDPPacketAddSearchMessage(packet, self, filter->type, kprSSDPVariantDeviceType));
	for (service = filter->services; service; service = service->next) {
		bailIfError(KprSSDPPacketAddSearchMessage(packet, self, service->type, kprSSDPVariantServiceType));
	}
	KprSSDPSchedulePacket(self, packet);
	return err;
bail:
	KprSSDPPacketDispose(packet);
	return err;
}

FskErr KprSSDPSearchAllDevices(KprSSDP self, KprSSDPInterface ssdpInterface)
{
	FskErr err = kFskErrNone;
	KprSSDPPacket packet = NULL;
	FskTimeRecord when;
	
	FskTimeGetNow(&when);
	bailIfError(KprSSDPPacketNew(&packet, kSSDPPacketSearch, &when, self->repeat, KprSSDPSendPacketDefault));
	packet->ssdpInterface = ssdpInterface;
	bailIfError(KprSSDPPacketAddSearchMessage(packet, self, "ssdp:all", kprSSDPVariantAll));
	KprSSDPSchedulePacket(self, packet);
	return err;
bail:
	KprSSDPPacketDispose(packet);
	return err;
}

//--------------------------------------------------
// SSDP Device
//--------------------------------------------------
#if 0
#pragma mark - KprSSDPDevice
#endif

FskErr KprSSDPDeviceNew(KprSSDPDevice *it, char* scheme, UInt32 port, char* path, UInt32 expire, SInt32 configId, char* uuid, char* type, char* services[])
{
	FskErr err = kFskErrNone;
	KprSSDPDevice self = NULL;
	KprSSDPService service = NULL;
	char buffer[256];
	
	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPDeviceRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprSSDPDeviceInstrumentation);
	self->scheme = FskStrDoCopy(scheme);
	bailIfNULL(self->scheme);
	self->port = port;
	self->path = FskStrDoCopy(path);
	bailIfNULL(self->path);
	self->expire = (expire) ? expire : gSSDP->expire;
	self->type = FskStrDoCopy(type);
	bailIfNULL(self->type);
	self->uuid = FskStrDoCopy(uuid);
	bailIfNULL(self->uuid);
	self->configId = configId;
	self->searchPort = 1900;
	snprintf(buffer, sizeof(buffer), "%s/%s UPnP/1.%d Kinoma/1.1", FskEnvironmentGet("OS"), FskEnvironmentGet("OSVersion"), (self->configId >= 0) ? 1 : 0);
	self->userAgent = FskStrDoCopy(buffer);
	bailIfNULL(self->userAgent);
	
	if (!services) return err;
	while (*services) {
		bailIfError(FskMemPtrNewClear(sizeof(KprSSDPServiceRecord), &service));
		service->type = FskStrDoCopy(*services++);
		bailIfNULL(service->type);
		FskListAppend(&self->services, service);
	}
	return err;
bail:
	KprSSDPDeviceDispose(self);
	return err;
}

void KprSSDPDeviceDispose(KprSSDPDevice self)
{
	if (self) {
		KprSSDPService service, next;
		for (service = self->services; service; service = next) {
			next = service->next;
			FskMemPtrDispose(service->type);
			FskMemPtrDispose(service);
		}
		KprSSDPPacketDispose(self->update);
		KprSSDPPacketDispose(self->byebye);
		KprSSDPPacketDispose(self->alive);
		FskMemPtrDispose(self->scheme);
		FskMemPtrDispose(self->path);
		FskMemPtrDispose(self->uuid);
		FskMemPtrDispose(self->userAgent);
		FskMemPtrDispose(self->type);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprSSDPDeviceAddService(KprSSDPDevice self, char* type)
{
	FskErr err = kFskErrNone;
	KprSSDPService service = NULL;
	
	for (service = self->services; service; service = service->next) {
		if (!FskStrCompare(service->type, type)) return kFskErrDuplicateElement;
	}
	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPServiceRecord), &service));
	service->type = FskStrDoCopy(type);
	bailIfNULL(service->type);
	FskListAppend(&self->services, service);
bail:
	return err;
}

FskErr KprSSDPDeviceRemoveService(KprSSDPDevice self, char* type)
{
	FskErr err = kFskErrNone;
	KprSSDPService service = NULL;

	for (service = self->services; service; service = service->next) {
		if (!FskStrCompare(service->type, type)) break;
	}
	bailIfNULL(service);
	FskListRemove(&self->services, service);
bail:
	return err;
}

FskErr KprSSDPDeviceSendAlive(KprSSDPDevice self, KprSSDP ssdp, Boolean doByebye)
{
	FskErr err = kFskErrNone;
	KprSSDPPacket packet = doByebye ? self->byebye : self->alive;
	FskTimeGetNow(&packet->when);
	FskTimeAddMS(&packet->when, kKprSSDPPacketInitialDelay); // delay 0 to 100 ms
	packet->repeat = doByebye ? 1 : ssdp->repeat;
	packet->index = 0;
	KprSSDPSchedulePacket(ssdp, packet);
	return err;
}

FskErr KprSSDPDeviceSendByebye(KprSSDPDevice self, KprSSDP ssdp)
{
	FskErr err = kFskErrNone;
	KprSSDPPacket packet, next;
	KprSSDPInterface ssdpInterface = NULL;
	int	length;
	char* buffer;
	
	// send byebye immediately once
	packet = self->byebye;
	bailIfNULL(packet);
	for (ssdpInterface = ssdp->ssdpInterfaces; ssdpInterface; ssdpInterface = ssdpInterface->next) {
		if (ssdpInterface->ready) {
			KprSSDPPacketMessage message;
			UInt32 repeat;
			for (repeat = ssdp->byebyeRepeat; repeat; repeat--) {
				for (message = packet->messages; message; message = message->next) {
					buffer = message->text;
					FskInstrumentedItemPrintfVerbose(ssdpInterface, "%lu.%lu.%lu.%lu -> M - %s %s %s",
						(ssdpInterface->ip >> 24) & 255, (ssdpInterface->ip >> 16) & 255, (ssdpInterface->ip >> 8) & 255, ssdpInterface->ip & 255,
						kKprSSDPType2String(packet->type), kKprSSDPVariant2String(message->variant), packet->device->type);
					FskInstrumentedItemPrintfDebug(ssdpInterface, "%s", buffer);
					FskNetSocketSendUDP(ssdpInterface->response, buffer, FskStrLen(buffer), &length, kKprSSDPMulticastAddr, kKprSSDPMulticastPort);
				}
			}
		}
	}
bail:
	// remove device packets
	for (packet = ssdp->packets; packet; packet = next) {
		next = packet->next;
		if (packet->device == self) {
			FskListRemove(&ssdp->packets, packet);
			if (packet->type == kSSDPPacketSearchResponse)
				KprSSDPPacketDispose(packet);
		}
	}
	if (ssdp->packetTimer) {
		if (ssdp->packets)
			FskTimeCallbackSet(ssdp->packetTimer, &ssdp->packets->when, KprSSDPSendPacket, ssdp);
		else
			FskTimeCallbackRemove(ssdp->packetTimer);
	}
	return err;
}

FskErr KprSSDPDeviceSendUpdate(KprSSDPDevice self, KprSSDP ssdp, UInt32 bootId)
{
	FskErr err = kFskErrNone;
	KprSSDPPacket packet;
	// remove alive packet
	for (packet = ssdp->packets; packet; packet = packet->next) {
		if ((packet->device == self) && (packet->type == kSSDPPacketAlive))
			break;
	}
	if (packet) {
		FskListRemove(&ssdp->packets, packet);
		// send update packet
		KprSSDPSendPacketBootId(ssdp, self->update, bootId);
	}
	if (ssdp->packetTimer) {
		if (ssdp->packets)
			FskTimeCallbackSet(ssdp->packetTimer, &ssdp->packets->when, KprSSDPSendPacket, ssdp);
		else
			FskTimeCallbackRemove(ssdp->packetTimer);
	}
//bail:
	return err;
}

FskErr KprSSDPDeviceSetupAlivePacket(KprSSDPDevice self, KprSSDP ssdp)
{
	FskErr err = kFskErrNone;
	KprSSDPService service;
	KprSSDPPacket packet = NULL;
	FskTimeRecord when;
	
	FskTimeGetNow(&when);
	FskTimeAddMS(&when, kKprSSDPPacketInitialDelay); // delay 0 to 100 ms
	bailIfError(KprSSDPPacketNew(&packet, kSSDPPacketAlive, &when, ssdp->repeat, KprSSDPSendPacketInterface));
	packet->device = self;
	bailIfError(KprSSDPPacketAddAliveMessage(packet, ssdp, self, NULL, kprSSDPVariantRoot));
	bailIfError(KprSSDPPacketAddAliveMessage(packet, ssdp, self, NULL, kprSSDPVariantDeviceUUID));
	bailIfError(KprSSDPPacketAddAliveMessage(packet, ssdp, self, NULL, kprSSDPVariantDeviceType));
	for (service = self->services; service; service = service->next) {
		bailIfError(KprSSDPPacketAddAliveMessage(packet, ssdp, self, service, kprSSDPVariantServiceType));
	}
	self->alive = packet;
	return err;
bail:
	KprSSDPPacketDispose(packet);
	return err;
}

FskErr KprSSDPDeviceSetupByebyePacket(KprSSDPDevice self, KprSSDP ssdp)
{
	FskErr err = kFskErrNone;
	KprSSDPService service;
	KprSSDPPacket packet = NULL;
	FskTimeRecord when;
	
	FskTimeGetNow(&when);
	FskTimeAddMS(&when, kKprSSDPPacketInitialDelay); // delay 0 to 100 ms
	bailIfError(KprSSDPPacketNew(&packet, kSSDPPacketByebye, &when, ssdp->repeat, KprSSDPSendPacketDefault));
	packet->device = self;
	bailIfError(KprSSDPPacketAddByebyeMessage(packet, ssdp, self, NULL, kprSSDPVariantRoot));
	bailIfError(KprSSDPPacketAddByebyeMessage(packet, ssdp, self, NULL, kprSSDPVariantDeviceUUID));
	bailIfError(KprSSDPPacketAddByebyeMessage(packet, ssdp, self, NULL, kprSSDPVariantDeviceType));
	for (service = self->services; service; service = service->next) {
		bailIfError(KprSSDPPacketAddByebyeMessage(packet, ssdp, self, service, kprSSDPVariantServiceType));
	}
	self->byebye = packet;
	return err;
bail:
	KprSSDPPacketDispose(packet);
	return err;
}

FskErr KprSSDPDeviceSetupPackets(KprSSDPDevice self, KprSSDP ssdp)
{
	FskErr err = kFskErrNone;
	bailIfError(KprSSDPDeviceSetupAlivePacket(self, ssdp));
	bailIfError(KprSSDPDeviceSetupByebyePacket(self, ssdp));
	bailIfError(KprSSDPDeviceSetupUpdatePacket(self, ssdp));
bail:
	return err;
}

FskErr KprSSDPDeviceSetupUpdatePacket(KprSSDPDevice self, KprSSDP ssdp)
{
	FskErr err = kFskErrNone;
	KprSSDPService service;
	KprSSDPPacket packet = NULL;
	FskTimeRecord when;
	
	FskTimeGetNow(&when);
	bailIfError(KprSSDPPacketNew(&packet, kSSDPPacketUpdate, &when, 1, KprSSDPSendPacketInterface));
	packet->device = self;
	bailIfError(KprSSDPPacketAddUpdateMessage(packet, ssdp, self, NULL, kprSSDPVariantRoot));
	bailIfError(KprSSDPPacketAddUpdateMessage(packet, ssdp, self, NULL, kprSSDPVariantDeviceUUID));
	bailIfError(KprSSDPPacketAddUpdateMessage(packet, ssdp, self, NULL, kprSSDPVariantDeviceType));
	for (service = self->services; service; service = service->next) {
		bailIfError(KprSSDPPacketAddUpdateMessage(packet, ssdp, self, service, kprSSDPVariantServiceType));
	}
	self->update = packet;
	return err;
bail:
	KprSSDPPacketDispose(packet);
	return err;
}

//--------------------------------------------------
// SSDP Discovery Description
//--------------------------------------------------
#if 0
#pragma mark - KprSSDPDiscoveryDescription
#endif

static FskErr KprSSDPDiscoveryDescriptionFromDiscovery(KprSSDPDiscoveryDescription *it, KprSSDPDiscovery discovery)
{
	FskErr err = kFskErrNone;
	KprSSDPDiscoveryDescription self = NULL;
	char ip[64];
	KprSSDPService service, newService;
	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPDiscoveryDescriptionRecord), it));
	self = *it;
	self->expire = discovery->expire;
	FskNetIPandPortToString(discovery->ssdpInterface->ip, 0, ip);
	self->ip = FskStrDoCopy(ip);
	bailIfNULL(self->ip);
	self->interfaceName = FskStrDoCopy(discovery->ssdpInterface->title);
	bailIfNULL(self->interfaceName);
	self->type = FskStrDoCopy(discovery->type);
	bailIfNULL(self->type);
	for (service = discovery->services; service; service = service->next) {
		bailIfError(FskMemPtrNewClear(sizeof(KprSSDPServiceRecord), &newService));
		newService->type = FskStrDoCopy(service->type);
		bailIfNULL(newService->type);
		FskListAppend(&self->services, newService);
	}
	self->url = FskStrDoCopy(discovery->location);
	bailIfNULL(self->url);
	self->uuid = FskStrDoCopy(discovery->uuid);
	bailIfNULL(self->uuid);
	FskInstrumentedItemNew(self, NULL, &gKprSSDPDiscoveryDescriptionInstrumentation);
bail:
	if (err)
		KprSSDPDiscoveryDescriptionDispose(self);
	return err;
}

void KprSSDPDiscoveryDescriptionDispose(KprSSDPDiscoveryDescription self)
{
	if (self) {
		KprSSDPService service, next;
		for (service = self->services; service; service = next) {
			next = service->next;
			FskMemPtrDispose(service->type);
			FskMemPtrDispose(service);
		}
		FskMemPtrDispose(self->interfaceName);
		FskMemPtrDispose(self->ip);
		FskMemPtrDispose(self->url);
		FskMemPtrDispose(self->type);
		FskMemPtrDispose(self->uuid);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

//--------------------------------------------------
// SSDP Discovery
//--------------------------------------------------
#if 0
#pragma mark - KprSSDPDiscovery
#endif

FskErr KprSSDPDiscoveryNew(KprSSDPDiscovery *it, KprSSDP ssdp, char* tag, char* uuid, char* type, char* location, UInt32 expire, KprSSDPInterface ssdpInterface)
{
	FskErr err = kFskErrNone;
	KprSSDPDiscovery self = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPDiscoveryRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprSSDPDiscoveryInstrumentation);
	self->ssdp = ssdp;
	if (type) {
		self->type = FskStrDoCopy(type);
		bailIfNULL(self->type);
	}
	self->location = FskStrDoCopy(location);
	bailIfNULL(self->location);
	self->expire = (expire) ? expire : gSSDP->expire;
	self->uuid = FskStrDoCopy(uuid);
	bailIfNULL(self->uuid);
	self->tag = FskStrDoCopy(tag);
	bailIfNULL(self->tag);
	FskTimeCallbackNew(&self->timer);
	self->ssdpInterface = ssdpInterface;
	
	return err;
bail:
	KprSSDPDiscoveryDispose(self);
	return err;
}

void KprSSDPDiscoveryDispose(KprSSDPDiscovery self)
{
	if (self) {
		KprSSDPService service, next;
		for (service = self->services; service; service = next) {
			next = service->next;
			FskMemPtrDispose(service->type);
			FskMemPtrDispose(service);
		}
		FskTimeCallbackDispose(self->timer);
		FskMemPtrDispose(self->uuid);
		FskMemPtrDispose(self->location);
		FskMemPtrDispose(self->type);
		FskMemPtrDispose(self->tag);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprSSDPDiscoveryAddService(KprSSDPDiscovery self, char* type)
{
	FskErr err = kFskErrNone;
	KprSSDPService service = NULL;
	
	for (service = self->services; service; service = service->next) {
		if (!FskStrCompare(service->type, type)) return kFskErrDuplicateElement;
	}
	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPDeviceRecord), &service));
	service->type = FskStrDoCopy(type);
	bailIfNULL(service->type);
	FskListAppend(&self->services, service);
	KprSSDPPrintDiscoveries(self->ssdp);
bail:
	return err;
}

void KprSSDPDiscoveryExpireCallback(FskTimeCallBack timer UNUSED, const FskTime time UNUSED, void *param)
{
	KprSSDPDiscovery self = param;
	KprSSDP ssdp = self->ssdp;
	FskInstrumentedItemPrintfDebug(self, "DEVICE %p: EXPIRED!", self);
	KprSSDPRemoveDiscovery(ssdp, self);
	KprSSDPPrintDiscoveries(ssdp);
	return;
}

void KprSSDPDiscoveryPrint(KprSSDPDiscovery self, KprSSDP ssdp)
{
	KprSSDPService service = NULL;
	FskInstrumentedItemPrintfVerbose(ssdp, "\t- DEVICE %p: %s - %s (%s)", self, self->uuid, self->type, self->tag);
	FskInstrumentedItemPrintfVerbose(ssdp, "\t\t-> %s", self->location);
	for (service = self->services; service; service = service->next) {
		FskInstrumentedItemPrintfVerbose(ssdp, "\t\t- SERVICE %s", service->type);
	}
}

//FskErr KprSSDPDiscoveryRemoveService(KprSSDPDiscovery self, KprSSDPService service)
//{
//	FskErr err = kFskErrNone;
//	bailIfError(FskListRemove(&self->services, service) ? kFskErrNone : kFskErrUnknownElement);
//	FskMemPtrDispose(service->type);
//	FskMemPtrDispose(service);
//bail:
//	return err;
//}

FskErr KprSSDPDiscoveryUpdateTimer(KprSSDPDiscovery self, UInt32 expire)
{
	FskErr err = kFskErrNone;
	FskTimeRecord when;

	FskTimeGetNow(&when);
	when.useconds = 0;
	FskTimeAddSecs(&when, expire);
	if (!FskTimeCompare(&when, &self->when)) return err;
	FskTimeCopy(&self->when, &when);
	FskTimeCallbackSet(self->timer, &when, KprSSDPDiscoveryExpireCallback, self);
#if SUPPORT_INSTRUMENTATION
	{
		time_t tsec;
		char date[32];

		time(&tsec);
		tsec += expire;
		strftime(date, 31, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&tsec));
		FskInstrumentedItemPrintfDebug(self, "DEVICE %s expires at %s (%lu)", self->uuid, date, expire);
	}
#endif
//bail:
	return err;
}

//--------------------------------------------------
// SSDP Filter
//--------------------------------------------------
#if 0
#pragma mark - KprSSDPFilter
#endif

FskErr KprSSDPFilterNew(KprSSDPFilter *it, char* authority, char* type, char* services[], KprSSDPDiscoveryCallback callback)
{
	FskErr err = kFskErrNone;
	KprSSDPFilter self = NULL;
	KprSSDPService service = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPFilterRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprSSDPFilterInstrumentation);
	if (authority) {
		self->authority = FskStrDoCopy(authority);
		bailIfNULL(self->authority);
	}
	self->delay = 5;
	if (type) {
		self->type = FskStrDoCopy(type);
		bailIfNULL(self->type);
	}
	self->discoveries = FskAssociativeArrayNew();
	self->callback = callback;
	if (!services) return err;
	while (*services) {
		bailIfError(FskMemPtrNewClear(sizeof(KprSSDPServiceRecord), &service));
		service->type = FskStrDoCopy(*services++);
		bailIfNULL(service->type);
		FskListAppend(&self->services, service);
	}
	return err;
bail:
	KprSSDPFilterDispose(self);
	return err;
}

void KprSSDPFilterDispose(KprSSDPFilter self)
{
	if (self) {
		KprSSDPService service, next;
		for (service = self->services; service; service = next) {
			next = service->next;
			FskMemPtrDispose(service->type);
			FskMemPtrDispose(service);
		}
		FskMemPtrDispose(self->type);
		FskMemPtrDispose(self->authority);
		FskAssociativeArrayDispose(self->discoveries);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

//--------------------------------------------------
// SSDP Interface
//--------------------------------------------------
#if 0
#pragma mark - KprSSDPInterface
#endif

FskErr KprSSDPInterfaceNew(KprSSDPInterface *it, KprSSDP ssdp, FskNetInterfaceRecord *interfaceRec, Boolean notify)
{
	FskErr err = kFskErrNone;
	KprSSDPInterface self = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPInterfaceRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprSSDPInterfaceInstrumentation);
	self->ssdp = ssdp;
	self->ip = interfaceRec->ip;
	FskMemCopy(self->MAC, interfaceRec->MAC, 6);
	self->status = interfaceRec->status;
	self->title = FskStrDoCopy(interfaceRec->name);
	bailIfNULL(self->title);
	FskListAppend(&ssdp->ssdpInterfaces, self);
	FskTimeCallbackNew(&self->timer);
	
	bailIfError(KprSSDPInterfaceConnect(self, notify));
bail:
	if (err)
		KprSSDPInterfaceDispose(self);
	return err;
}

void KprSSDPInterfaceDispose(KprSSDPInterface self)
{
	if (self) {
		FskTimeCallbackDispose(self->timer);
		KprSSDPInterfaceDisconnect(self);
		FskMemPtrDispose(self->title);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprSSDPInterfaceConnectCallback(FskTimeCallBack timer UNUSED, const FskTime time UNUSED, void *param)
{
	KprSSDPInterface self = param;
	FskInstrumentedItemPrintfDebug(self, "KprSSDPInterfaceConnectCallback %s: retrying!\n", self->title);
	KprSSDPInterfaceConnect(self, true);
}

FskErr KprSSDPInterfaceConnect(KprSSDPInterface self, Boolean notify)
{
	FskErr err = kFskErrNone;
	KprSSDP ssdp = self->ssdp;
	UInt32 ttl = ssdp->ttl;
	
	FskInstrumentedItemPrintfDebug(self, "KprSSDPInterface %s: trying %08lX %lu!\n", self->title, self->ip, self->status);

	FskTimeCallbackRemove(self->timer);
	bailIfError(FskNetSocketNewUDP(&self->multicast, "ssdp multicast socket"));
#if TARGET_OS_ANDROID
	//@@ Alain: fix for Android release where the first UDP socket is 0 and then fails to read
	if (!self->multicast->platSkt) {
		FskInstrumentedItemPrintfDebug(self, "self->multicast retry!\n");
		FskNetSocketClose(self->multicast);
		self->multicast = NULL;
		bailIfError(FskNetSocketNewUDP(&self->multicast, "ssdp multicast socket"));
	}
#endif
	bailIfError(FskNetSocketReuseAddress(self->multicast));

#if TARGET_OS_WIN32
	bailIfError(FskNetSocketBind(self->multicast, self->ip, kKprSSDPMulticastPort));
#elif TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL
	bailIfError(FskNetSocketBind(self->multicast, kKprSSDPMulticastAddr, kKprSSDPMulticastPort));
#else
	#error Bind socket
#endif
	bailIfError(FskNetSocketMulticastJoin(self->multicast, kKprSSDPMulticastAddr, self->ip, ttl));
	bailIfError(FskNetSocketReceiveBufferSetSize(self->multicast, kKprSSDPReceiveBufferSize));
	FskThreadAddDataHandler(&self->multicastHandler, (FskThreadDataSource)self->multicast, KprSSDPInterfaceReadMulticast, true, false, self);

	bailIfError(FskNetSocketNewUDP(&self->response, "ssdp response socket"));
#if TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_WIN32 || TARGET_OS_KPL
	bailIfError(FskNetSocketMulticastLoop(self->response, ssdp->discoverSelf ? 1 : 0));
	bailIfError(FskNetSocketMulticastSetOutgoingInterface(self->response, self->ip, ttl));
#else
	#error ttl socket
#endif
	bailIfError(FskNetSocketReceiveBufferSetSize(self->response, kKprSSDPReceiveBufferSize));
	bailIfError(FskNetSocketSendBufferSetSize(self->response, kKprSSDPSendBufferSize));
	FskThreadAddDataHandler(&self->responseHandler, (FskThreadDataSource)self->response, KprSSDPInterfaceReadResponse, true, false, self);
	
	self->ready = true;
	FskInstrumentedItemPrintfDebug(self, "KprSSDPInterface %s: connection succeeded!\n", self->title);

	if (notify) {
		// advertise devices and search all on new KprSSDPInterface
		FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(ssdp->devices);
		while (iterate) {
			KprSSDPDevice device = (KprSSDPDevice)iterate->value;
			KprSSDPDeviceSendAlive(device, ssdp, false);
			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		FskAssociativeArrayIteratorDispose(iterate);
		if (ssdp->searchAll) {
			bailIfError(KprSSDPSearchAllDevices(ssdp, self));
		}
		else {
			KprSSDPFilter filter;
			for (filter = ssdp->filters; filter; filter = filter->next) {
				bailIfError(KprSSDPSearchDevices(ssdp, self, filter));
			}
		}
	}
	return err;
bail:
	FskInstrumentedItemPrintfDebug(self, "KprSSDPInterface %s: connection failed!\n", self->title);
	KprSSDPInterfaceDisconnect(self);
	{
		FskTimeRecord when;
		FskTimeGetNow(&when);
		FskTimeAddMS(&when, 1000);
		FskTimeCallbackSet(self->timer, &when, KprSSDPInterfaceConnectCallback, self);
	}

	return err;
}

void KprSSDPInterfaceDisconnect(KprSSDPInterface self)
{
	FskThreadRemoveDataHandler(&self->multicastHandler);
	FskNetSocketClose(self->multicast);
	self->multicast = NULL;
	FskThreadRemoveDataHandler(&self->responseHandler);
	FskNetSocketClose(self->response);
	self->response = NULL;
	self->ready = false;
}

Boolean KprSSDPInterfaceValidateIP(KprSSDPInterface iface, UInt32 from, UInt32 to)
{
	if (from == to)
		return true;
	while (iface) {
		if (iface->ip == from)
			return false;
		iface = iface->next;
	}
	return true;
}

void KprSSDPInterfaceReadMulticast(FskThreadDataHandler handler UNUSED, FskThreadDataSource source, void *refCon)
{
	FskErr err = kFskErrNone;
	KprSSDPInterface self = refCon;
	KprSSDP ssdp = self->ssdp;
	char buffer[kKprSSDPPacketBufSize];
	int ip, port, size;

	FskSocket socket = (FskSocket)source;
	while (kFskErrNone == (err = FskNetSocketRecvUDP(socket, buffer, kKprSSDPPacketBufSize - 1, &size, &ip, &port))) {
		if (size == 0)
			continue;
		if (!KprSSDPInterfaceValidateIP(ssdp->ssdpInterfaces, ip, self->ip))
			continue;
		buffer[size] = 0;
		if (FskStrCompareWithLength(buffer, kKprSSDPSearchStartLine, kKprSSDPSearchStartLineLength) == 0) {
			if ((port != 1900) && (port > 1024)) {
				bailIfError(KprSSDPProcessSearch(ssdp, self, ip, port, buffer + kKprSSDPSearchStartLineLength, size - kKprSSDPSearchStartLineLength, true));
			}
		}
		else if (FskStrCompareWithLength(buffer, kKprSSDPNotifyStartLine, kKprSSDPNotifyStartLineLength) == 0) {
			bailIfError(KprSSDPProcessNotify(ssdp, self, ip, port, buffer + kKprSSDPNotifyStartLineLength, size - kKprSSDPNotifyStartLineLength, true));
		}
	}
bail:
	return;
}

void KprSSDPInterfaceReadResponse(FskThreadDataHandler handler UNUSED, FskThreadDataSource source, void *refCon)
{
	FskErr err = kFskErrNone;
	KprSSDPInterface self = refCon;
	KprSSDP ssdp = self->ssdp;
	char buffer[kKprSSDPPacketBufSize];
	int ip, port, size;

	FskSocket socket = (FskSocket)source;
	while (kFskErrNone == (err = FskNetSocketRecvUDP(socket, buffer, kKprSSDPPacketBufSize - 1, &size, &ip, &port))) {
		if (size == 0)
			continue;
		buffer[size] = 0;
		if (FskStrCompareWithLength(buffer, kKprSSDPResponseStartLine, kKprSSDPResponseStartLineLength) == 0) {
			bailIfError(KprSSDPProcessNotify(ssdp, self, ip, port, buffer + kKprSSDPResponseStartLineLength, size - kKprSSDPResponseStartLineLength, false));
		}
	}
bail:
	return;
}

//--------------------------------------------------
// SSDP Packet
//--------------------------------------------------
#if 0
#pragma mark - KprSSDPPacket
#endif

FskErr KprSSDPPacketNew(KprSSDPPacket *it, UInt32 type, FskTime when, UInt32 repeat, KprSSDPPacketCallbackProc callback)
{
	FskErr err = kFskErrNone;
	KprSSDPPacket self = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPPacketRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprSSDPPacketInstrumentation);
	FskTimeCopy(&self->when, when);
	self->repeat = repeat;
	self->callback = callback;
	self->type = type;

	return err;
bail:
	KprSSDPPacketDispose(self);
	return err;
}

void KprSSDPPacketDispose(KprSSDPPacket self)
{
	if (self) {
		KprSSDPPacketMessage message, next;
		for (message = self->messages; message; message = next) {
			next = message->next;
			KprSSDPPacketMessageDispose(message);
		}
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprSSDPPacketAddMessage(KprSSDPPacket self, UInt32 type, UInt32 variant, char* text)
{
	FskErr err = kFskErrNone;
	KprSSDPPacketMessage message;
	
	bailIfError(KprSSDPPacketMessageNew(&message, type, variant, text));
	FskListAppend(&self->messages, message);
	self->size++;
bail:
	return err;
}

FskErr KprSSDPPacketAddAliveMessage(KprSSDPPacket self, KprSSDP ssdp, KprSSDPDevice device, KprSSDPService service, UInt32 variant)
{
	FskErr err = kFskErrNone;
	char buffer[kKprSSDPPacketBufSize];
	char lines[5][128];
	snprintf(buffer, kKprSSDPPacketBufSize,
		kKprSSDPNotifyStartLine
		"HOST: 239.255.255.250:1900\r\n"
		"CACHE-CONTROL: max-age=%lu\r\n"
		"LOCATION: %s://%%s:%lu%s\r\n"
		"NT: %s"
		"NTS: ssdp:alive\r\n"
		"SERVER: %s\r\n"
		"%s"
		"%s"
		"%s"
		"%s"
		"\r\n",
		(unsigned long)device->expire,
		device->scheme, (unsigned long)device->port, device->path,
		KprSSDPPacketGetLineNT(ssdp, device, service, lines[0], 128, variant),
		device->userAgent,
		KprSSDPPacketGetLineUSN(ssdp, device, service, lines[1], 128, variant),
		KprSSDPPacketGetLineBootId(ssdp, device, lines[2], 128),
		KprSSDPPacketGetLineConfigId(ssdp, device, lines[3], 128),
		KprSSDPPacketGetLineSearchPort(ssdp, device, lines[4], 128)
	);
	bailIfError(KprSSDPPacketAddMessage(self, self->type, variant, buffer));
bail:
	return err;
}

FskErr KprSSDPPacketAddByebyeMessage(KprSSDPPacket self, KprSSDP ssdp, KprSSDPDevice device, KprSSDPService service, UInt32 variant)
{
	FskErr err = kFskErrNone;
	char buffer[kKprSSDPPacketBufSize];
	char lines[4][128];
	snprintf(buffer, kKprSSDPPacketBufSize,
		kKprSSDPNotifyStartLine
		"HOST: 239.255.255.250:1900\r\n"
		"NT: %s"
		"NTS: ssdp:byebye\r\n"
		"%s"
		"%s"
		"%s"
		"\r\n",
		KprSSDPPacketGetLineNT(ssdp, device, service, lines[0], 128, variant),
		KprSSDPPacketGetLineUSN(ssdp, device, service, lines[1], 128, variant),
		KprSSDPPacketGetLineBootId(ssdp, device, lines[2], 128),
		KprSSDPPacketGetLineConfigId(ssdp, device, lines[3], 128)
	);
	bailIfError(KprSSDPPacketAddMessage(self, self->type, variant, buffer));
bail:
	return err;
}

FskErr KprSSDPPacketAddSearchMessage(KprSSDPPacket self, KprSSDP ssdp, char* type, UInt32 variant)
{
	FskErr err = kFskErrNone;
	char buffer[kKprSSDPPacketBufSize];
	snprintf(buffer, kKprSSDPPacketBufSize,
		kKprSSDPSearchStartLine
		"HOST: 239.255.255.250:1900\r\n"
		"MAN: \"ssdp:discover\"\r\n"
		"MX: %lu\r\n"
		"ST: %s\r\n"
		"SERVER: %s\r\n"
		"\r\n",
		(unsigned long)ssdp->mx,
		type ? type : "ssdp:all",
		ssdp->userAgent
	);
	bailIfError(KprSSDPPacketAddMessage(self, self->type, variant, buffer));
bail:
	return err;
}

FskErr KprSSDPPacketAddResponseMessage(KprSSDPPacket self, KprSSDP ssdp, KprSSDPDevice device, KprSSDPService service, UInt32 variant)
{
	FskErr err = kFskErrNone;
	char buffer[kKprSSDPPacketBufSize];
	char lines[5][128];
	char date[128];
	time_t tsec = KprDateNow();
	strftime(date, sizeof(date), "DATE: %a, %d %b %Y %H:%M:%S GMT\r\n", gmtime((const time_t*)&tsec));
	snprintf(buffer, kKprSSDPPacketBufSize,
		kKprSSDPResponseStartLine
		"CACHE-CONTROL: max-age=%lu\r\n"
		"%s"
		"EXT: \r\n"
		"LOCATION: %s://%%s:%lu%s\r\n"
		"SERVER: %s\r\n"
		"ST: %s"
		"%s"
		"%s"
		"%s"
		"%s"
		"\r\n",
		(unsigned long)device->expire,
		date,
		device->scheme, (unsigned long)device->port, device->path,
		device->userAgent,
		KprSSDPPacketGetLineNT(ssdp, device, service, lines[0], 128, variant),
		KprSSDPPacketGetLineUSN(ssdp, device, service, lines[1], 128, variant),
		KprSSDPPacketGetLineBootId(ssdp, device, lines[2], 128),
		KprSSDPPacketGetLineConfigId(ssdp, device, lines[3], 128),
		KprSSDPPacketGetLineSearchPort(ssdp, device, lines[4], 128)
	);
	bailIfError(KprSSDPPacketAddMessage(self, self->type, variant, buffer));
bail:
	return err;
}

FskErr KprSSDPPacketAddUpdateMessage(KprSSDPPacket self, KprSSDP ssdp, KprSSDPDevice device, KprSSDPService service, UInt32 variant)
{
	FskErr err = kFskErrNone;
	char buffer[kKprSSDPPacketBufSize];
	char lines[6][128];
	snprintf(buffer, kKprSSDPPacketBufSize,
		kKprSSDPNotifyStartLine
		"NT: %s"
		"HOST: 239.255.255.250:1900\r\n"
		"%s"
		"%s"
		"NTS: ssdp:byebye\r\n"
		"%s"
		"%s"
		"%s"
		"\r\n",
		KprSSDPPacketGetLineNT(ssdp, device, service, lines[0], 128, variant),
		KprSSDPPacketGetLineUSN(ssdp, device, service, lines[1], 128, variant),
		KprSSDPPacketGetLineBootId(ssdp, device, lines[2], 128),
		KprSSDPPacketGetLineConfigId(ssdp, device, lines[3], 128),
		KprSSDPPacketGetLineNextBootId(ssdp, lines[4], 128),
		KprSSDPPacketGetLineSearchPort(ssdp, device, lines[5], 128)
	);
	bailIfError(KprSSDPPacketAddMessage(self, self->type, variant, buffer));
bail:
	return err;
}

char* KprSSDPPacketGetLineBootId(KprSSDP self UNUSED, KprSSDPDevice device, char* buffer, UInt32 size)
{
	(device->configId >= 0) ? snprintf(buffer, size, "BOOTID.UPNP.ORG: %lu\r\n", (unsigned long)self->bootId) : (buffer[0] = 0);
	return buffer;
}

char* KprSSDPPacketGetLineConfigId(KprSSDP self UNUSED, KprSSDPDevice device, char* buffer, UInt32 size)
{
	(device->configId >= 0) ? snprintf(buffer, size, "CONFIGID.UPNP.ORG: %ld\r\n", (unsigned long)device->configId) : (buffer[0] = 0);
	return buffer;
}

char* KprSSDPPacketGetLineNextBootId(KprSSDP self UNUSED, char* buffer, UInt32 size)
{
	(false) ? snprintf(buffer, size, "BOOTID.UPNP.ORG: %%lu\r\n") : (buffer[0] = 0);
	return buffer;
}

char* KprSSDPPacketGetLineNT(KprSSDP self UNUSED, KprSSDPDevice device, KprSSDPService service, char* buffer, UInt32 size, UInt32 variant)
{
	switch (variant) {
		case kprSSDPVariantRoot:
			snprintf(buffer, size, "upnp:rootdevice\r\n");
			break;
		case kprSSDPVariantDeviceUUID:
			snprintf(buffer, size, "uuid:%s\r\n", device->uuid);
			break;
		case kprSSDPVariantDeviceType:
			snprintf(buffer, size, "%s\r\n", device->type);
			break;
		case kprSSDPVariantServiceType:
			snprintf(buffer, size, "%s\r\n", service->type);
			break;
	}
	return buffer;
}

char* KprSSDPPacketGetLineSearchPort(KprSSDP self UNUSED, KprSSDPDevice device, char* buffer, UInt32 size)
{
	(device->searchPort != 1900) ? snprintf(buffer, size, "SEARCHPORT.UPNP.ORG: %lu\r\n", (unsigned long)device->searchPort) : (buffer[0] = 0);
	return buffer;
}

char* KprSSDPPacketGetLineUSN(KprSSDP self UNUSED, KprSSDPDevice device, KprSSDPService service, char* buffer, UInt32 size, UInt32 variant)
{
	switch (variant) {
		case kprSSDPVariantRoot:
			snprintf(buffer, size, "USN: uuid:%s::upnp:rootdevice\r\n", device->uuid);
			break;
		case kprSSDPVariantDeviceUUID:
			snprintf(buffer, size, "USN: uuid:%s\r\n", device->uuid);
			break;
		case kprSSDPVariantDeviceType:
			snprintf(buffer, size, "USN: uuid:%s::%s\r\n", device->uuid, device->type);
			break;
		case kprSSDPVariantServiceType:
			snprintf(buffer, size, "USN: uuid:%s::%s\r\n", device->uuid, service->type);
			break;
	}
	return buffer;
}

//--------------------------------------------------
// SSDP Packet Message
//--------------------------------------------------
#if 0
#pragma mark - KprSSDPPacketMessage
#endif

FskErr KprSSDPPacketMessageNew(KprSSDPPacketMessage *it, UInt32 type, UInt32 variant, char* text)
{
	FskErr err = kFskErrNone;
	KprSSDPPacketMessage self = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprSSDPPacketMessageRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprSSDPPacketMessageInstrumentation);
	self->type = type;
	self->variant = variant;
	self->text = FskStrDoCopy(text);
	bailIfNULL(self->text);

	return err;
bail:
	KprSSDPPacketMessageDispose(self);
	return err;
}

void KprSSDPPacketMessageDispose(KprSSDPPacketMessage self)
{
	if (self) {
		FskMemPtrDispose(self->text);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}
