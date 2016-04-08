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
#ifndef __KPRSSDPIMPLEMENTATION__
#define __KPRSSDPIMPLEMENTATION__

#include "kpr.h"
#include "kprMessage.h"

#include "FskNetInterface.h"
#include "FskNetUtils.h"
#include "FskTime.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct KprSSDPDiscoveryStruct KprSSDPDiscoveryRecord, *KprSSDPDiscovery;
typedef struct KprSSDPDiscoveryDescriptionStruct KprSSDPDiscoveryDescriptionRecord, *KprSSDPDiscoveryDescription;
typedef struct KprSSDPFilterStruct KprSSDPFilterRecord, *KprSSDPFilter;
typedef struct KprSSDPDeviceStruct KprSSDPDeviceRecord, *KprSSDPDevice;
typedef struct KprSSDPServiceStruct KprSSDPServiceRecord, *KprSSDPService;

typedef void (*KprSSDPDiscoveryCallback)(char* authority, KprSSDPDiscoveryDescription description, Boolean alive, void* refcon);

#define kKPRSSDPKinomaServe "urn:schemas-kinoma-com:device:%s:1"

//--------------------------------------------------
// SSDP
//--------------------------------------------------

FskAPI(void) KprSSDPStart(KprService service, FskThread thread, xsMachine* the);
FskAPI(void) KprSSDPStop(KprService service);

// Interface
FskAPI(FskErr) KprSSDPAddInterface(FskNetInterfaceRecord *interfaceRec, UInt32 notify);
FskAPI(FskErr) KprSSDPRemoveInterface(FskNetInterfaceRecord *interfaceRec);
// Server
FskAPI(FskErr) KprSSDPDiscover(char* type, char* service[], KprSSDPDiscoveryCallback callback, void* refcon);
FskAPI(FskErr) KprSSDPDiscoverDevice(char* authority, char* type, char* service[], KprSSDPDiscoveryCallback callback);
FskAPI(FskErr) KprSSDPDiscoverServer(char* authority, char* id);
FskAPI(FskErr) KprSSDPForget(void* refcon);
FskAPI(FskErr) KprSSDPForgetDevice(char* authority, char* type, char* service[]);
FskAPI(FskErr) KprSSDPForgetServer(char* authority, char* id);
FskAPI(FskErr) KprSSDPSearch(void* refcon);

// Device
FskAPI(FskErr) KprSSDPAddDevice(KprSSDPDevice device);
FskAPI(KprSSDPDevice) KprSSDPGetDevice(char* uuid);
FskAPI(FskErr) KprSSDPRemoveDevice(char* uuid);
// Discovery
FskAPI(FskErr) KprSSDPRemoveDiscoveryByLocation(char* location);
FskAPI(FskErr) KprSSDPRemoveDiscoveryByUUID(char* uuid);
// Filter
FskAPI(FskErr) KprSSDPAddFilter(KprSSDPFilter filter);
FskAPI(FskErr) KprSSDPRemoveFilter(KprSSDPFilter filter);

//--------------------------------------------------
// SSDP Service
//--------------------------------------------------

struct KprSSDPServiceStruct {
	KprSSDPService next;
	char* type;
};

//--------------------------------------------------
// SSDP DEVICE
//--------------------------------------------------

FskAPI(FskErr) KprSSDPDeviceNew(KprSSDPDevice *it, char* scheme, UInt32 port, char* path, UInt32 expire, SInt32 configId, char* uuid, char* type, char* services[]);
FskAPI(void) KprSSDPDeviceDispose(KprSSDPDevice self);
FskAPI(FskErr) KprSSDPDeviceAddService(KprSSDPDevice self, char* type);
FskAPI(FskErr) KprSSDPDeviceRemoveService(KprSSDPDevice self, char* type);

//--------------------------------------------------
// SSDP Discovery Description
//--------------------------------------------------

struct KprSSDPDiscoveryDescriptionStruct {
	UInt32 expire;
	char* ip;
	char* interfaceName;
	KprSSDPService services;
	char* type;
	char* url;
	char* uuid;
	FskInstrumentedItemDeclaration
};

FskAPI(void) KprSSDPDiscoveryDescriptionDispose(KprSSDPDiscoveryDescription self);

//--------------------------------------------------
// SSDP FILTER
//--------------------------------------------------

struct KprSSDPFilterStruct {
	KprSSDPFilter next;
	UInt32 delay;
	FskAssociativeArray discoveries;
	KprSSDPService services;
	FskThread thread;
	KprSSDPDiscoveryCallback callback;
	char* authority;
	char* type;
	UInt32 isDevice;
	void* refcon;
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprSSDPFilterNew(KprSSDPFilter *it, char* authority, char* type, char* services[], KprSSDPDiscoveryCallback callback);
FskAPI(void) KprSSDPFilterDispose(KprSSDPFilter self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
