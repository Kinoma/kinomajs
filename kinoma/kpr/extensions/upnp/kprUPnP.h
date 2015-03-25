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
#ifndef __KPRUPNP__
#define __KPRUPNP__

enum {
	kUPnPErrInvalidAction					= 401,
	kUPnPErrInvalidArgs						= 402,
	kUPnPErrActionFailed					= 501,
	kUPnPErrArgumentValueInvalid			= 600,
	kUPnPErrArgumentValueOutOfRange			= 601,
	kUPnPErrOptionalActionNotImplemented	= 602,
	kUPnPErrOutOfMemory						= 603,
	kUPnPErrHumanInterventionRequired		= 604,
	kUPnPErrStringArgumentTooLong			= 605,
	kUPnPErrNoSuchObject					= 701,
};

typedef struct KprUPnPControllerStruct KprUPnPControllerRecord, *KprUPnPController;
typedef struct KprUPnPDeviceStruct KprUPnPDeviceRecord, *KprUPnPDevice;
typedef struct KprUPnPIconStruct KprUPnPIconRecord, *KprUPnPIcon;
typedef struct KprUPnPServiceStruct KprUPnPServiceRecord, *KprUPnPService;
typedef struct KprUPnPActionStruct KprUPnPActionRecord, *KprUPnPAction;
typedef struct KprUPnPArgumentStruct KprUPnPArgumentRecord, *KprUPnPArgument;
typedef struct KprUPnPStateVariableStruct KprUPnPStateVariableRecord, *KprUPnPStateVariable;
typedef struct KprUPnPSubscriptionStruct KprUPnPSubscriptionRecord, *KprUPnPSubscription;
typedef struct KprUPnPMetadataStruct KprUPnPMetadataRecord, *KprUPnPMetadata;

// UPnP

FskAPI(FskErr) KprUPnPAddDevice(KprContext context, KprUPnPDevice self);
FskAPI(KprUPnPDevice) KprUPnPGetDevice(KprContext context);
FskAPI(FskErr) KprUPnPRemoveDevice(KprContext context);

// UPnP DEVICE

struct KprUPnPDeviceStruct {
	char* domain;
	char* name;
	char* path;
	char* type;
	char* url;
	SInt32 version;

	char* baseURL;
	char* configId;
	char* description;
	UInt32 descriptionSize;
	char* friendlyName;
	char* manufacturer;
	char* modelName;
	char* modelNumber;
	KprUPnPController controller;
	KprUPnPIcon icon;
	char* presentationURL;
	KprUPnPService service;
	char* uuid;
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprUPnPDeviceNew(KprUPnPDevice* it, char* type);
FskAPI(void) KprUPnPDeviceDispose(KprUPnPDevice self);
FskAPI(FskErr) KprUPnPDeviceFromFile(KprUPnPDevice self, char* url);

// UPnP SERVICE

struct KprUPnPServiceStruct {
	KprUPnPService next;
	FskAssociativeArray actions;
	char* domain;
	char* name;
	char* path;
	char* type;
	char* url;
	SInt32 version;
	
//	SInt32 configId;
	char* controlURL;
	char* description;
	UInt32 descriptionSize;
	KprUPnPDevice device;
	char* eventSubURL;
	char* SCPDURL;
	FskAssociativeArray stateVariables;
	KprUPnPSubscription subscription;
	UInt32 actionError;
	char* actionErrorDescription;
	Boolean lastChnage;
	char* lastChangeNamespace;
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprUPnPServiceNew(KprUPnPService* it, char* type);
FskAPI(void) KprUPnPServiceDispose(KprUPnPService self);
FskAPI(FskErr) KprUPnPServiceFromFile(KprUPnPService self, char* deviceConfigId);

// UPnP CONTROLLER

struct KprUPnPControllerStruct {
	char* authority;
	KprContext context;
	KprUPnPDevice device;
	KprMessage currentMessage;
	FskList waitingMessages;
	char* ip;
	char* uuid;
	Boolean removed;
	Boolean alive;
	FskTimeCallBack pingTimer;
	
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprUPnPControllerNew(KprUPnPController* it, char* authority, char* ip, char* uuid);
FskAPI(void) KprUPnPControllerDispose(KprUPnPController self);
//FskAPI(FskErr) KprUPnPControllerFromFile(KprUPnPController self, char* url);

struct KprUPnPMetadataStruct {
	char *upnpClass;
	char *resourceUrl;
	double duration;
	char *artworkUri;
	char *title;
	char *album;
	char *artist;
	char *creator;
	char *genre;
	int track;
};

#endif
