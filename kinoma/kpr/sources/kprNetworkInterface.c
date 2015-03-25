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
#define __FSKHTTPSERVER_PRIV__

#if TARGET_OS_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

#include "Fsk.h"
#include "FskEndian.h"
#include "FskUUID.h"

#if TARGET_OS_KPL
#include "KplNetInterface.h"
#endif

#include "kpr.h"
#include "kprApplication.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHandler.h"
#include "kprHTTPClient.h"
#include "kprHTTPServer.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprURL.h"
#include "kprUtilities.h"

#include "kprNetworkInterface.h"

//--------------------------------------------------
// Network Interface
//--------------------------------------------------

typedef struct KprNetworkInterfaceStruct KprNetworkInterfaceRecord, *KprNetworkInterface;
struct KprNetworkInterfaceStruct {
	KprNetworkInterface next;
	int flag;
	int ip;
	char* name;
};

static KprNetworkInterface gNetworkInterface = NULL;

static FskErr KprNetworkInterfaceMark(int ip, char *name);
static void KprNetworkInterfaceSweep();

#if TARGET_OS_ANDROID
enum {
	kprNetInterfaceWifi = 0,
	kprNetInterfaceCellular,
	kprNetInterfaceUnknown,
};

enum {
	kprNetInterfaceDisconnected = 0,
	kprNetInterfaceConnecting,
	kprNetInterfaceConnected,
};

void KprCellularInterfaceCallback(UInt16 state, UInt32 networkType)
{
	return kFskErrNone;
}

void KprWiFiInterfaceCallback(UInt16 state, UInt32 ip, unsigned char* mac)
{
	if ((state == kprNetInterfaceConnected) && ip) {
		ip = FskEndianU32_BtoN(ip);
		KprNetworkInterfaceMark(ip, "wlan0");

		if (mac) {
			int length = FskStrLen(mac);
			FskDebugStr("INTERFACE MAC = '%s' %d", mac, length);
			if (length == 17) { // 38:AA:3C:7B:02:9F
				unsigned char MAC[6] = { 0, 0, 0, 0, 0, 0 };
				int i, j;
				for (i = length - 1, j = 0; j < 6; j++, i--) {
					unsigned char hex = mac[i--];
					MAC[j] = ((hex > 0x40) ? hex - 7 : hex) & 0xf;
					hex = mac[i--];
					MAC[j] |= (((hex > 0x40) ? hex - 7 : hex) & 0xf) << 4;
				}
				FskDebugStr("-> INTERFACE MAC = %2X:%2X:%2X:%2X:%2X:%2X", MAC[5], MAC[4], MAC[3], MAC[2], MAC[1], MAC[0]);
			}
			FskMemPtrDispose(mac);
		}

	}
	KprNetworkInterfaceSweep();
	return kFskErrNone;
}
#elif TARGET_OS_IPHONE
#include <ifaddrs.h>
#include <arpa/inet.h>
#import <SystemConfiguration/SystemConfiguration.h>

SCNetworkReachabilityRef gNetworkInterfaceReachability = NULL;

static void KprNetworkInterfaceReachabilityCallback(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void* info)
{
	int ip = 0;
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *interface = NULL;
	int success = getifaddrs(&interfaces);
	if (success == 0) {
		interface = interfaces;
		while (interface != NULL) {
			if (interface->ifa_addr->sa_family == AF_INET) {
				// Check if interface is en0 which is the wifi connection on the iPhone
				if (!FskStrCompare(interface->ifa_name, "en0")) {
					ip = ntohl(((struct sockaddr_in *)interface->ifa_addr)->sin_addr.s_addr);
					KprNetworkInterfaceMark(ip, interface->ifa_name);
					break;
				}
			}
			interface = interface->ifa_next;
		}
	}
	freeifaddrs(interfaces);
	KprNetworkInterfaceSweep();
}
#elif TARGET_OS_MAC
#include <ifaddrs.h>
#include <arpa/inet.h>
#import <SystemConfiguration/SystemConfiguration.h>

SCDynamicStoreRef gNetworkInterfaceStore = NULL;
CFRunLoopSourceRef gNetworkInterfaceSource = NULL;

static void KprNetworkInterfaceStoreCallback(SCDynamicStoreRef store, CFArrayRef changedKeys, void *info)
{
	int ip = 0;
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *interface = NULL;
	int success = getifaddrs(&interfaces);
	if (success == 0) {
		interface = interfaces;
		while (interface != NULL) {
			if (interface->ifa_addr->sa_family == AF_INET) {
				ip = ntohl(((struct sockaddr_in *)interface->ifa_addr)->sin_addr.s_addr);
				if (0x7f000001 != ip)
					KprNetworkInterfaceMark(ip, interface->ifa_name);
			}
			interface = interface->ifa_next;
		}
	}
	freeifaddrs(interfaces);
	KprNetworkInterfaceSweep();
}
#elif TARGET_OS_WIN32

HANDLE gNetworkInterfaceHandle = 0;

VOID __stdcall KprNetworkInterfaceCallback(PVOID context, PMIB_IPINTERFACE_ROW interfaceRow, MIB_NOTIFICATION_TYPE notificationType)
{
	int ip;
	FskErr err = kFskErrNone;
	IP_ADAPTER_INFO	*adapters = NULL;
	DWORD result;
	DWORD size = 15000;
	IP_ADAPTER_INFO	*adapter;
	IP_ADDR_STRING *address;

	for (ip = 0; ip < 3; ip++) {
		err = FskMemPtrNew(size, &adapters);
		result = GetAdaptersInfo(adapters, &size);
		if (ERROR_BUFFER_OVERFLOW == result) {
			FskMemPtrDispose(adapters);
			adapters = NULL;
		}
		else
			break;
	}
	if (ERROR_SUCCESS == result) { 
		for (adapter = adapters ; NULL != adapter ; adapter = adapter->Next) {
			for (address = &adapter->IpAddressList; address; address = address->Next) {
                if (FskStrLen(address->IpAddress.String) == 0)
                    continue;
				FskNetStringToIPandPort(address->IpAddress.String, &ip, NULL);
				if (0x7f000001 != ip)
					KprNetworkInterfaceMark(ip, adapter->AdapterName);
			}
		}
		KprNetworkInterfaceSweep();
	}
	FskMemPtrDispose(adapters);
}

#elif TARGET_OS_KPL
	#ifdef KPR_CONFIG

void KprNetworkInterfaceCallback(void *refCon)
{
	FskErr err = kFskErrNone;
	KplNetInterfaceRecord *interfaceList = NULL, *interface;
	bailIfError(KplNetInterfaceEnumerate(&interfaceList));
	interface = interfaceList;
	while (interface != NULL) {
		KplNetInterfaceRecord *next = interface->next;
		if (interface->ip && (0x7f000001 != interface->ip)) {
			KprNetworkInterfaceMark(interface->ip, interface->name);
		}
		FskMemPtrDispose(interface->name);
		FskMemPtrDispose(interface);
		interface = next;
	}
	KprNetworkInterfaceSweep();
bail:
	return;
}

	#endif
#endif

void KprNetworkInterfaceActivate(Boolean activateIt)
{
#if TARGET_OS_IPHONE
	if (activateIt) {
		SCNetworkReachabilityFlags flags;
		SCNetworkReachabilityGetFlags(gNetworkInterfaceReachability, &flags);
		KprNetworkInterfaceReachabilityCallback(gNetworkInterfaceReachability, flags, NULL);
	}
	else {
		KprNetworkInterfaceSweep();
	}
#endif
}

void KprNetworkInterfaceAdd(int ip)
{
	char ipString[16];
	char buffer[64];
	KprMessage message = NULL;
	FskNetIPandPortToString(ip, 0, ipString);
	FskDebugStr("ADD INTERFACE %s", ipString);
	sprintf(buffer, "xkpr:///network/interface/add?ip=%s", ipString);
	KprMessageNew(&message, buffer);
//	if (message) KprMessageNotify(message);
}

void KprNetworkInterfaceCleanup()
{
#if TARGET_OS_IPHONE
	if (gNetworkInterfaceReachability) {
		SCNetworkReachabilityUnscheduleFromRunLoop(gNetworkInterfaceReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		CFRelease(gNetworkInterfaceReachability);
	}
#elif TARGET_OS_MAC
	if (gNetworkInterfaceSource) {
		CFRelease(gNetworkInterfaceSource);
		gNetworkInterfaceSource = NULL;
	}
	if (gNetworkInterfaceStore) {
		CFRelease(gNetworkInterfaceStore);
		gNetworkInterfaceStore = NULL;
	}
#elif TARGET_OS_WIN32
	if (gNetworkInterfaceHandle) {
		CancelMibChangeNotify2(gNetworkInterfaceHandle);
		gNetworkInterfaceHandle = 0;
	}
#elif TARGET_OS_KPL
	#ifdef KPR_CONFIG
	KplNetInterfaceTerminate();
	#endif
#endif
}

FskErr KprNetworkInterfaceMark(int ip, char *name)
{
	FskErr err = kFskErrNone;
	KprNetworkInterface* address = &gNetworkInterface;
	KprNetworkInterface self;
	while ((self = *address)) {
		if ((self->ip == ip) && !FskStrCompare(self->name, name)) {
			self->flag = 1;
			return err;
		}
        address = &self->next;
	}
	bailIfError(FskMemPtrNewClear(sizeof(KprNetworkInterfaceRecord), address));
	self = *address;
	self->ip = ip;
	self->name = FskStrDoCopy(name);
	bailIfNULL(self->name);
bail:
	return err;
}

void KprNetworkInterfaceRemove(int ip)
{
	char ipString[16];
	char buffer[64];
	KprMessage message = NULL;
	FskNetIPandPortToString(ip, 0, ipString);
	FskDebugStr("ADD INTERFACE %s", ipString);
	sprintf(buffer, "xkpr:///network/interface/remove?ip=%s", ipString);
	KprMessageNew(&message, buffer);
	if (message) KprMessageNotify(message);
}

void KprNetworkInterfaceSetup()
{
#if TARGET_OS_IPHONE
	struct sockaddr_in localWifiAddress;
	SCNetworkReachabilityContext context = {0, NULL, NULL, NULL, NULL};
	SCNetworkReachabilityFlags flags;
	bzero(&localWifiAddress, sizeof(localWifiAddress));
	localWifiAddress.sin_len = sizeof(localWifiAddress);
	localWifiAddress.sin_family = AF_INET;
	localWifiAddress.sin_addr.s_addr = htonl(IN_LINKLOCALNETNUM); // IN_LINKLOCALNETNUM is defined in <netinet/in.h> as 169.254.0.0
	gNetworkInterfaceReachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr*)&localWifiAddress);
	SCNetworkReachabilitySetCallback(gNetworkInterfaceReachability, KprNetworkInterfaceReachabilityCallback, &context);
	SCNetworkReachabilityScheduleWithRunLoop(gNetworkInterfaceReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	SCNetworkReachabilityGetFlags(gNetworkInterfaceReachability, &flags);
	KprNetworkInterfaceReachabilityCallback(gNetworkInterfaceReachability, flags, NULL);
#elif TARGET_OS_MAC
	FskErr err = kFskErrNone;
    SCDynamicStoreContext context = {0, NULL, NULL, NULL, NULL};
    CFStringRef pattern = NULL;
    CFArrayRef patternList = NULL;
	pattern = SCDynamicStoreKeyCreateNetworkServiceEntity(NULL, kSCDynamicStoreDomainState, kSCCompAnyRegex, kSCEntNetIPv4);
    if (!pattern) { err = kFskErrOperationFailed; goto bail; }
    patternList = CFArrayCreate(NULL, (const void **)&pattern, 1, &kCFTypeArrayCallBacks);
    if (!patternList) { err = kFskErrOperationFailed; goto bail; }
    gNetworkInterfaceStore = SCDynamicStoreCreate(NULL, CFSTR("KprNetWorkInterface"), KprNetworkInterfaceStoreCallback, &context);
    if (!gNetworkInterfaceStore) { err = kFskErrOperationFailed; goto bail; }
	if (!SCDynamicStoreSetNotificationKeys(gNetworkInterfaceStore, NULL, patternList)) { err = kFskErrOperationFailed; goto bail; }
    gNetworkInterfaceSource = SCDynamicStoreCreateRunLoopSource(NULL, gNetworkInterfaceStore, 0);
    if (!gNetworkInterfaceSource) { err = kFskErrOperationFailed; goto bail; }
    CFRunLoopAddSource(CFRunLoopGetCurrent(), gNetworkInterfaceSource, kCFRunLoopCommonModes);
    KprNetworkInterfaceStoreCallback(gNetworkInterfaceStore, NULL, NULL);
bail:
    if (err != noErr) {
    	if (gNetworkInterfaceSource) {
        	CFRelease(gNetworkInterfaceSource);
        	gNetworkInterfaceSource = NULL;
    	}
    	if (gNetworkInterfaceStore) {
        	CFRelease(gNetworkInterfaceStore);
        	gNetworkInterfaceStore = NULL;
    	}
    }
	if (patternList)
		CFRelease(patternList);
	if (pattern) 
		CFRelease(pattern);
#elif TARGET_OS_WIN32
	NETIOAPI_API status = NotifyIpInterfaceChange(AF_INET,  &KprNetworkInterfaceCallback, NULL, TRUE, &gNetworkInterfaceHandle);
#elif TARGET_OS_KPL
	#ifdef KPR_CONFIG
	KplNetInterfaceInitialize();
	KplNetInterfaceSetChangedCallback(KprNetworkInterfaceCallback, 0L);
	KprNetworkInterfaceCallback(0);
	#endif
#endif
}

void KprNetworkInterfaceSweep()
{
	KprNetworkInterface* address = &gNetworkInterface;
	KprNetworkInterface self;
	while ((self = *address)) {
		if (self->flag < 0) {
			KprNetworkInterfaceRemove(self->ip);
			*address = self->next;
			FskMemPtrDispose(self->name);
			FskMemPtrDispose(self);
		}
		else {
			if (self->flag == 0)
				KprNetworkInterfaceAdd(self->ip);
			self->flag = -1;
			address = &self->next;
		}
	}
}
