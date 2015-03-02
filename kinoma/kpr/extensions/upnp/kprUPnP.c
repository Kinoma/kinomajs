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
#define __FSKHTTPSERVER_PRIV__
#define __FSKMEDIAREADER_PRIV__

#include "limits.h"

#include "kpr.h"
#include "kprApplication.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHandler.h"
#include "kprHTTPClient.h"
#include "kprHTTPServer.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprSSDPImplementation.h"
#include "kprUPnP.h"
#include "kprURL.h"
#include "kprUtilities.h"

#include "FskEnvironment.h"
#include "FskHeaders.h"
#include "FskMediaReader.h"
#include "FskUUID.h"

static char* kUPnPDeviceNameSpace = "urn:schemas-upnp-org:device-1-0";
static char* kUPnPServiceNameSpace = "urn:schemas-upnp-org:service-1-0";
static char* kUPnPEventNameSpace = "urn:schemas-upnp-org:event-1-0";
static char* kUPnPControlNameSpace = "urn:schemas-upnp-org:control-1-0";
static char* kUPnPMetadataNameSpace = "urn:schemas-upnp-org:metadata-1-0/";
static char* kUPnPSoapEncodingNameSpace = "http://schemas.xmlsoap.org/soap/encoding/";
static char* kUPnPSoapEnvelopeNameSpace = "http://schemas.xmlsoap.org/soap/envelope/";
static char* kUPnPMetadataDIDLLiteNameSpace = "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/";
static char* kUPnPMetadataDcNameSpace = "http://purl.org/dc/elements/1.1/";
//static char* kUPnPMetadataDLNANameSpace = "urn:schemas-dlna-org:metadata-1-0/";
static char* kUPnPMetadataUPnPNameSpace = "urn:schemas-upnp-org:metadata-1-0/upnp/";

typedef struct KprUPnPStruct KprUPnPRecord, *KprUPnP;

typedef void (*KprUPnPVariableDispose)(KprUPnPStateVariable self);
typedef FskErr (*KprUPnPVariableFromElement)(KprUPnPStateVariable self, KprXMLElement element);
typedef FskErr (*KprUPnPVariableFromString)(KprUPnPStateVariable self, char* string);
typedef void (*KprUPnPVariableToXS)(xsMachine *the, KprUPnPStateVariable self);

typedef struct KprUPnPSIntDataStruct KprUPnPSIntDataRecord, *KprUPnPSIntData;
typedef struct KprUPnPUIntDataStruct KprUPnPUIntDataRecord, *KprUPnPUIntData;
typedef struct KprUPnPStringDataStruct KprUPnPStringDataRecord, *KprUPnPStringData;

#define kFskStrUserAgent "USER-AGENT"

//--------------------------------------------------
// UPnP Errors
//--------------------------------------------------

static FskResponseCode gKPRUPnPErrors[] = {
  { kUPnPErrInvalidAction, 					"Invalid Action" },
  { kUPnPErrInvalidArgs,					"Invalid Args" },
  { kUPnPErrActionFailed,					"Action Failed" },
  { kUPnPErrArgumentValueInvalid,			"Argument Value Invalid" },
  { kUPnPErrArgumentValueOutOfRange,		"Argument Value Out of Range" },
  { kUPnPErrOptionalActionNotImplemented,	"Optional Action Not Implemented" },
  { kUPnPErrOutOfMemory,					"Out of Memory" },
  { kUPnPErrHumanInterventionRequired,		"Human Intervention Required" },
  { kUPnPErrStringArgumentTooLong,			"String Argument Too Long" },
  { kUPnPErrNoSuchObject,					"No Such Object" },
  { -1, "" }
};

static char *KprUPnPFindError(int error)
{
	FskResponseCode *item = gKPRUPnPErrors;

	while (item->HTTP_code != -1) {
		if (item->HTTP_code == error)
			return item->HTTP_description;
		item++;
	}

	return NULL;
}

//--------------------------------------------------
// UPnP
//--------------------------------------------------

struct KprUPnPStruct {
	FskAssociativeArray devices;
	FskAssociativeArray controllers;
	FskAssociativeArray subscriptions;
	char* userAgent;
	UInt32 subscriptionTimeout;
	UInt32 httpTimeout;
	UInt32 upnpEventModeration;
	UInt32 upnpControllerPingPeriod;
	FskInstrumentedItemDeclaration
};

static Boolean KprUPnPAccept(KprService self, KprMessage message);
static void KprUPnPStart(KprService self, FskThread thread, xsMachine* the);
static void KprUPnPStop(KprService self);

static FskErr KprUPnPGetBehavior(char* authority, KprScriptBehavior* behavior);
static void KprUPnPDefaultMessageDispose(void* it);

//--------------------------------------------------
// UPnP Utilities
//--------------------------------------------------

static FskErr KprUPnPStorageWrite(FskGrowableStorage storage, char* data, UInt32 size);
static FskErr KprUPnPStorageWriteEntity(FskGrowableStorage storage, char* data, UInt32 theFlag);
static FskErr KprUPnPUUIDCreate(FskUUID uuid);
static char* KprUPnPUUIDGetForKey(const char *key);

//--------------------------------------------------
// UPnP Device
//--------------------------------------------------

static FskErr KprUPnPDeviceFromElement(KprUPnPDevice self, KprXMLElement device, char* uuid);
static FskErr KprUPnPDeviceGetPath(KprUPnPDevice self, char* type, char** path);
//static KprUPnPService KprUPnPDeviceGetService(KprUPnPDevice self, char* type);
static FskErr KprUPnPDeviceGetServiceURL(KprUPnPDevice self, char* type, char** url);

//--------------------------------------------------
// UPnP Icon
//--------------------------------------------------

struct KprUPnPIconStruct {
	KprUPnPIcon next;
	char* mime;
	UInt32 width;
	UInt32 height;
	UInt32 depth;
	char* path;
	char* url;
	FskInstrumentedItemDeclaration
};

static FskErr KprUPnPIconNew(KprUPnPIcon* it);
static void KprUPnPIconDispose(KprUPnPIcon self);
static FskErr KprUPnPIconFromElement(KprXMLElement element, KprUPnPDevice device);

//--------------------------------------------------
// UPnP Service
//--------------------------------------------------

static FskErr KprUPnPServiceActionInvoke(KprUPnPService self, KprContext context, KprHandler handler, KprMessage message);
static void KprUPnPServiceActionResponse(KprUPnPService self, KprContext context, KprHandler handler, KprMessage message);
static FskErr KprUPnPServiceFromElement(KprXMLElement element, KprUPnPDevice device);
static FskErr KprUPnPServiceFromSCPDElement(KprUPnPService self, KprXMLElement scpd);

static FskErr KprUPnPServiceAddSubscription(KprUPnPService self, char* url, UInt32 duration, char** uuid);
static FskErr KprUPnPServiceEvent(KprUPnPService self);
static FskErr KprUPnPServiceRenewSubscription(KprUPnPService self, char* uuid, UInt32 duration);
static FskErr KprUPnPServiceRemoveSubscription(KprUPnPService self, char* uuid);

//--------------------------------------------------
// UPnP Subscription
//--------------------------------------------------

struct KprUPnPSubscriptionStruct {
	KprUPnPSubscription next;
	char* eventSubURL;
	char* uuid;
	char* url;
	UInt32 key;
	UInt32 duration;
	KprUPnPController controller;
	KprUPnPService service;
	FskTimeCallBack timer;
	Boolean notified;
	Boolean removed;
	KprMessage event;
	FskGrowableStorage eventBody;
	Boolean eventModeration;
	Boolean eventPending;
	FskTimeCallBack eventTimer;
	KprUPnPStringData values;
	KprUPnPStringData lastChanges;
	FskInstrumentedItemDeclaration
};

static FskErr KprUPnPSubscriptionNew(KprUPnPSubscription* it, KprUPnPService service, char* url, UInt32 duration);
static void KprUPnPSubscriptionDispose(KprUPnPSubscription self);
static FskErr KprUPnPSubscriptionEvent(KprUPnPSubscription self, Boolean changed);
static Boolean KprUPnPSubscriptionEventBody(KprUPnPSubscription self);
static void KprUPnPSubscriptionEventCallback(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *param);
static void KprUPnPSubscriptionEventComplete(KprMessage message, void* it);
static void KprUPnPSubscriptionExpire(FskTimeCallBack callback, const FskTime time, void *param);
static void KprUPnPSubscriptionRenew(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *param);
static void KprUPnPSubscriptionRenewMessageComplete(KprMessage message, void* it);
static FskErr KprUPnPSubscriptionSubscribe(KprUPnPSubscription subscription, KprUPnPController controller);
static void KprUPnPSubscriptionSubscribeMessageComplete(KprMessage message, void* it);
static FskErr KprUPnPSubscriptionUnsubscribe(KprUPnPSubscription subscription, KprUPnPController controller);
static void KprUPnPSubscriptionUnsubscribeMessageComplete(KprMessage message, void* it);

//--------------------------------------------------
// UPnP Action
//--------------------------------------------------

struct KprUPnPActionStruct {
	KprUPnPArgument argumentIn;
	KprUPnPArgument argumentOut;
	KprUPnPStateVariable relatedStateVariable;
	KprUPnPService service;
	char* name;
	FskInstrumentedItemDeclaration
};

static FskErr KprUPnPActionNew(KprUPnPAction* it);
static void KprUPnPActionDispose(KprUPnPAction self);
static FskErr KprUPnPActionFromElement(KprXMLElement action, KprUPnPService service);

//--------------------------------------------------
// UPnP Argument
//--------------------------------------------------

struct KprUPnPArgumentStruct {
	KprUPnPArgument next;
	char* name;
	KprUPnPStateVariable relatedStateVariable;
	Boolean returnValue;
	FskInstrumentedItemDeclaration
};

static FskErr KprUPnPArgumentNew(KprUPnPArgument* it);
static void KprUPnPArgumentDispose(KprUPnPArgument self);
static FskErr KprUPnPArgumentFromElement(KprXMLElement argument, KprUPnPAction action);

//--------------------------------------------------
// UPnP State Varaible
//--------------------------------------------------

struct KprUPnPStateVariableStruct {
	KprUPnPVariableDispose dispose;
	KprUPnPVariableFromElement fromElement;
	KprUPnPVariableFromString fromString;
	KprUPnPVariableToXS toXS;
	Boolean multicast;
	Boolean lastChange;
	Boolean sendEvents;
	KprUPnPService service;
	void* data;
	char* value;
	Boolean changed;
	FskInstrumentedItemDeclaration
};

struct KprUPnPSIntDataStruct {
	SInt32 value;
	SInt32 minimum;
	SInt32 maximum;
	UInt32 step;
};

struct KprUPnPUIntDataStruct {
	UInt32 value;
	UInt32 minimum;
	UInt32 maximum;
	UInt32 step;
};

struct KprUPnPStringDataStruct {
	KprUPnPStringData next;
	char* value;
};

static FskErr KprUPnPStateVariableNew(KprUPnPStateVariable* it);
static void KprUPnPStateVariableDispose(KprUPnPStateVariable self);
static FskErr KprUPnPStateVariableFromElement(KprXMLElement variable, KprUPnPService service);
static char* KprUPnPStateVariableGetValue(KprUPnPStateVariable self);
static FskErr KprUPnPStateVariableSetValue(KprUPnPStateVariable self, char* value);

//--------------------------------------------------
// UPnP Handler Behavior
//--------------------------------------------------

enum {
	kprUPnPMessageDescription,
	kprUPnPMessageControl,
	kprUPnPMessageSubscription,
	kprUPnPMessageIcon,
	kprUPnPMessageEvent,
	kprUPnPMessagePresentation,
	kprUPnPMessageError
};

static FskErr KprUPnPHandlerBehaviorNew(KprBehavior* it, KprContent content, UInt32 command, KprUPnPDevice device, KprUPnPService service, KprUPnPIcon icon);
static void KprUPnPHandlerBehaviorCancel(void* it, KprMessage message);
static void KprUPnPHandlerBehaviorComplete(void* it, KprMessage message);
static void KprUPnPHandlerBehaviorDispose(void* it);
static void KprUPnPHandlerBehaviorInvoke(void* it, KprMessage message);

typedef struct {
	KprDelegate delegate;
	KprSlotPart;
	UInt32 command;
	KprUPnPDevice device;
	KprUPnPService service;
	KprUPnPIcon icon;
	KprUPnPAction action;
} KprUPnPHandlerBehaviorRecord, *KprUPnPHandlerBehavior;

//--------------------------------------------------
// UPnP Controller
//--------------------------------------------------

static void KprUPnPControllerDiscoveryCallback(char* authority, KprSSDPDiscoveryDescription description, Boolean alive, void* refcon);
static void KprUPnPControllerDeviceMessageComplete(KprMessage message, void* it);
static void KprUPnPControllerNotifyDevice(KprUPnPController self, Boolean alive);
static void KprUPnPControllerPingCallback(FskTimeCallBack timer UNUSED, const FskTime time UNUSED, void *param);
static void KprUPnPControllerPingMessageComplete(KprMessage message, void* it);
static void KprUPnPControllerPingSchedule(KprUPnPController self);
static void KprUPnPControllerRemove(KprUPnPController self);
static void KprUPnPControllerServiceMessageComplete(KprMessage message, void* it);
static void KprUPnPControllerServiceActionCallback(KprUPnPController self, char* serviceType, char* actionName, SInt32 errorCode, char* errorDescription);
static void KprUPnPControllerServiceActionCallbackPlay(KprUPnPController self, const char* serviceType);
static void KprUPnPControllerServiceActionCallbackStop(KprUPnPController self, const char* serviceType);
static void KprUPnPControllerServiceActionCallbackGetPositionInfo(KprUPnPController self, const char* serviceType);
static void KprUPnPControllerServiceActionCallbackSeek(KprUPnPController self, const char* serviceType);
static void KprUPnPControllerServiceActionCallbackOther(KprUPnPController self, const char* serviceType, const char *actionName);
static void KprUPnPControllerServiceActionCallbackError(KprUPnPController self, const char* serviceType, const char *actionName, SInt32 errorCode, const char* errorDescription);

static FskErr KprUPnPControllerServiceActionInvoke(KprUPnPController self, char* serviceType, char* actionName);
static Boolean KprUPnPControllerServiceIsAction(KprUPnPController self, char* serviceType, char* actionName);
static void KprUPnPControllerServiceActionComplete(KprMessage message, void* it);
static void KprUPnPControllerServiceEventCallback(KprUPnPController self, char* serviceType, char* name, char* value);

static void KprUPnPControllerParseMetadata(const char *variableValue);
static void KprUPnPControllerGotMetadata(KprUPnPMetadata metadata);
static const char * KprUPnPControllerServiceGetVariable(KprUPnPController self, const char* serviceType, char* variableName);
static double KprUPnPControllerParseTimecode(const char *timecode);

static void KprUPnPControllerUtility(KprUPnPController self, char* actionName);

#if TARGET_OS_IPHONE
void KprUPnPControllerServiceActionCallbackPlay_iOS(KprUPnPController self, const char* serviceType);
void KprUPnPControllerServiceActionCallbackStop_iOS(KprUPnPController self, const char* serviceType);
void KprUPnPControllerServiceActionCallbackGetPositionInfo_iOS(KprUPnPController self, const char* serviceType, double duration, double position);
void KprUPnPControllerServiceActionCallbackSeek_iOS(KprUPnPController self, const char* serviceType);
void KprUPnPControllerServiceActionCallbackOther_iOS(KprUPnPController self, const char* serviceType, const char *actionName);
void KprUPnPControllerServiceActionCallbackError_iOS(KprUPnPController self, const char* serviceType, const char *actionName, SInt32 errorCode, const char* errorDescription);
void KprUPnPControllerGotMetadata_iOS(KprUPnPMetadata metadata);
void KprUPnPControllerUtility_iOS(KprUPnPController self, char* actionName);
#elif TARGET_OS_ANDROID
void KprUPnPControllerServiceActionCallbackPlay_Android(KprUPnPController self, const char* serviceType);
void KprUPnPControllerServiceActionCallbackStop_Android(KprUPnPController self, const char* serviceType);
#endif

//--------------------------------------------------
// INSTRUMENTATION
//--------------------------------------------------

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprUPnPInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUPnP", FskInstrumentationOffset(KprUPnPRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
static FskInstrumentedTypeRecord KprUPnPDeviceInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUPnPDevice", FskInstrumentationOffset(KprUPnPDeviceRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
static FskInstrumentedTypeRecord KprUPnPIconInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUPnPIcon", FskInstrumentationOffset(KprUPnPIconRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
static FskInstrumentedTypeRecord KprUPnPServiceInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUPnPService", FskInstrumentationOffset(KprUPnPServiceRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
static FskInstrumentedTypeRecord KprUPnPSubscriptionInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUPnPSubscription", FskInstrumentationOffset(KprUPnPSubscriptionRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
static FskInstrumentedTypeRecord KprUPnPActionInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUPnPAction", FskInstrumentationOffset(KprUPnPActionRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
static FskInstrumentedTypeRecord KprUPnPArgumentInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUPnPArgument", FskInstrumentationOffset(KprUPnPArgumentRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
static FskInstrumentedTypeRecord KprUPnPStateVariableInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUPnPStateVariable", FskInstrumentationOffset(KprUPnPStateVariableRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
static FskInstrumentedTypeRecord KprUPnPControllerInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUPnPController", FskInstrumentationOffset(KprUPnPControllerRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

//--------------------------------------------------
// UPnP
//--------------------------------------------------
#if 0
#pragma mark - KprUPnP
#endif

static KprUPnP gUPnP = NULL;

KprServiceRecord gUPnPService = {
	NULL,
	0,
	"upnp:",
	NULL,
	NULL,
	KprUPnPAccept,
	KprServiceCancel,
	KprServiceInvoke,
	KprUPnPStart,
	KprUPnPStop,
	NULL,
	NULL,
	NULL
};

FskExport(FskErr) kprUPnP_fskLoad(FskLibrary library)
{
	KprServiceRegister(&gUPnPService);
	return kFskErrNone;
}

FskExport(FskErr) kprUPnP_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

Boolean KprUPnPAccept(KprService self UNUSED, KprMessage message UNUSED)
{
	return false;
}

void KprUPnPStart(KprService service, FskThread thread, xsMachine* the)
{
	FskErr err = kFskErrNone;
	KprUPnP self;
	char buffer[256];

	service->machine = the;
	service->thread = thread;
	bailIfError(FskMemPtrNewClear(sizeof(KprUPnPRecord), &self));
	self->devices = FskAssociativeArrayNew();
	bailIfNULL(self->devices);
	self->controllers = FskAssociativeArrayNew();
	bailIfNULL(self->controllers);
	self->subscriptions = FskAssociativeArrayNew();
	bailIfNULL(self->subscriptions);
	snprintf(buffer, sizeof(buffer), "%s/%s UPnP/1.0 Kinoma/1.1", FskEnvironmentGet("OS"), FskEnvironmentGet("OSVersion"));
	self->userAgent = FskStrDoCopy(buffer);
	bailIfNULL(self->userAgent);
	self->subscriptionTimeout = KprEnvironmentGetUInt32("upnpSubscriptionTimeout", 1800);
	self->httpTimeout = KprEnvironmentGetUInt32("upnpHTTPTimeout", 30);
	self->upnpEventModeration = KprEnvironmentGetUInt32("upnpEventModeration", 250);
	self->upnpControllerPingPeriod = KprEnvironmentGetUInt32("upnpControllerPingPeriod", 0);
	gUPnP = self;
	FskInstrumentedItemNew(self, NULL, &KprUPnPInstrumentation);

	return;
bail:
	FskAssociativeArrayDispose(self->devices);
	FskMemPtrDispose(self);
	return;
}

void KprUPnPStop(KprService service UNUSED)
{
	KprUPnP self = gUPnP;
	FskAssociativeArrayIterator iterate;
	FskMemPtrDispose(self->userAgent);
	
	iterate = FskAssociativeArrayIteratorNew(self->subscriptions);
	while (iterate) {
		KprUPnPSubscription subscription = (KprUPnPSubscription)iterate->value;
		KprUPnPSubscriptionDispose(subscription);
		iterate = FskAssociativeArrayIteratorNext(iterate);
	}
	FskAssociativeArrayIteratorDispose(iterate);
	FskAssociativeArrayDispose(self->subscriptions);
	
	iterate = FskAssociativeArrayIteratorNew(self->controllers);
	while (iterate) {
		KprUPnPController controller = (KprUPnPController)iterate->value;
		KprUPnPControllerDispose(controller);
		iterate = FskAssociativeArrayIteratorNext(iterate);
	}
	FskAssociativeArrayIteratorDispose(iterate);
	FskAssociativeArrayDispose(self->controllers);
	
	iterate = FskAssociativeArrayIteratorNew(self->devices);
	while (iterate) {
		KprUPnPDevice device = (KprUPnPDevice)iterate->value;
		KprUPnPDeviceDispose(device);
		iterate = FskAssociativeArrayIteratorNext(iterate);
	}
	FskAssociativeArrayIteratorDispose(iterate);
	FskAssociativeArrayDispose(self->devices);
	
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

FskErr KprUPnPAddHandler(KprContext context, KprUPnPDevice device, KprUPnPService service, KprUPnPIcon icon, char* relative, UInt32 command)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	KprHandler handler = NULL;
	KprBehavior behavior = NULL;
	
	bailIfError(FskMemPtrNewClear(FskStrLen(device->name) + FskStrLen(relative) + 3, &path));
	FskStrCopy(path, "/");
	FskStrCat(path, device->name);
	FskStrCat(path, "/");
	FskStrCat(path, relative);
	
	bailIfError(KprHandlerNew(&handler, path));
	KprUPnPHandlerBehaviorNew(&behavior, (KprContent)handler, command, device, service, icon);
	KprContentSetBehavior((KprContent)handler, behavior);
	KprContextPutHandler(context, handler);
bail:
	FskMemPtrDispose(path);
	return err;
}

FskErr KprUPnPAddEventHandler(KprContext context)
{
	FskErr err = kFskErrNone;
	KprHandler handler = NULL;
	KprBehavior behavior = NULL;
	
	bailIfError(KprHandlerNew(&handler, "/upnp/event"));
	KprUPnPHandlerBehaviorNew(&behavior, (KprContent)handler, kprUPnPMessageEvent, NULL, NULL, NULL);
	KprContentSetBehavior((KprContent)handler, behavior);
	KprContextPutHandler(context, handler);
bail:
	return err;
}

FskErr KprUPnPAddDevice(KprContext context, KprUPnPDevice device)
{
	FskErr err = kFskErrNone;
	KprUPnP self = gUPnP;
	KprSSDPDevice ssdpDevice = NULL;
	SInt32 configId = -1;
	KprHTTPServer server = KprHTTPServerGet(context->id);
	char* path = NULL;
	bailIfNULL(server);
	bailIfError(FskMemPtrNew(1 + FskStrLen(device->name) + 13, &path));
	FskStrCopy(path, "/");
	FskStrCat(path, device->name);
	FskStrCat(path, "/description");
	if (device->configId)
		configId = FskStrToNum(device->configId);
	bailIfError(KprSSDPDeviceNew(&ssdpDevice, KprHTTPServerGetPort(server), path, 0, configId, device->uuid, device->type, NULL));
	{	// add services
		KprUPnPService service;
		for (service = device->service; service; service = service->next) {
			bailIfError(KprSSDPDeviceAddService(ssdpDevice, service->type));
		}
	}
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPAddDevice, ssdpDevice, NULL, NULL, NULL);
	FskAssociativeArrayElementSetReference(self->devices, context->id, device);
	{
		KprUPnPIcon icon;
		KprUPnPService service;

		bailIfError(KprUPnPAddHandler(context, device, NULL, NULL, "description", kprUPnPMessageDescription));
		for (icon = device->icon; icon; icon = icon->next) {
			bailIfError(KprUPnPAddHandler(context, device, NULL, icon, icon->url, kprUPnPMessageIcon));
		}
		for (service = device->service; service; service = service->next) {
			bailIfError(KprUPnPAddHandler(context, device, service, NULL, service->controlURL, kprUPnPMessageControl));
			if (service->eventSubURL) bailIfError(KprUPnPAddHandler(context, device, service, NULL, service->eventSubURL, kprUPnPMessageSubscription));
			bailIfError(KprUPnPAddHandler(context, device, service, NULL, service->SCPDURL, kprUPnPMessageDescription));
		}
		if (device->presentationURL) {
			bailIfError(KprUPnPAddHandler(context, device, NULL, NULL, device->presentationURL, kprUPnPMessagePresentation));
		}
	}
	FskMemPtrDispose(path);
	return err;
bail:
	FskMemPtrDispose(path);
	KprSSDPDeviceDispose(ssdpDevice);
	return err;
}

void KprUPnPDefaultMessageDispose(void* it)
{
	FskMemPtrDispose(it);
	return;
}

FskErr KprUPnPGetBehavior(char* authority, KprScriptBehavior* behavior)
{
	FskErr err = kFskErrNone;
	KprShell shell = gShell;
	UInt32 authorityLength = FskStrLen(authority);
	KprScriptBehavior result = NULL;
	
	if (!FskStrCompareWithLength(authority, shell->id, authorityLength))
		result = (KprScriptBehavior)shell->behavior;
	else {
		KprContentLink link = shell->applicationChain.first;
		while (link) {
			KprApplication application = (KprApplication)link->content;
			if (application->id && (!FskStrCompareWithLength(authority, application->id, authorityLength))) {
				result = (KprScriptBehavior)application->behavior;
				break;
			}
			link = link->next;
		}
	}
	*behavior = result;
	if (!result)
		err = kFskErrNotFound;
	return err;
}

KprUPnPDevice KprUPnPGetDevice(KprContext context)
{
	KprUPnP self = gUPnP;
	return self ? (KprUPnPDevice)FskAssociativeArrayElementGetReference(self->devices, context->id) : NULL;
}

FskErr KprUPnPRemoveDevice(KprContext context)
{
	FskErr err = kFskErrNone;
	KprUPnP self = gUPnP;
	KprUPnPDevice device = KprUPnPGetDevice(context);
	
	if (!device) return kFskErrInvalidParameter;
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPRemoveDevice, FskStrDoCopy(device->uuid), NULL, NULL, NULL);
	FskAssociativeArrayElementDispose(self->devices, context->id);
//bail:
	return err;
}

//--------------------------------------------------
// UPnP Utilities
//--------------------------------------------------

FskErr KprUPnPStorageWrite(FskGrowableStorage storage, char* data, UInt32 size)
{
	FskErr err = kFskErrNone;
	if (!size)
		size = FskStrLen(data);
 	bailIfError(FskGrowableStorageAppendItem(storage, data, size));
bail:
	return err;
}

FskErr KprUPnPStorageWriteEntity(FskGrowableStorage storage, char* data, UInt32 theFlag)
{
	FskErr err = kFskErrNone;
	static unsigned char sEscape[256] = {
	/*  0 1 2 3 4 5 6 7 8 9 A B C D E F */
		3,3,3,3,3,3,3,3,3,1,1,3,3,3,3,3,	/* 0x                    */
		3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,	/* 1x                    */
		0,0,1,0,0,0,3,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
		0,0,0,0,0,0,0,0,0,0,0,0,3,0,2,0,	/* 3x  0123456789:;<=>?  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 	/* 7X  pqrstuvwxyz{|}~   */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 8X                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 9X                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* AX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* BX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* CX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* FX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* EX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 	/* FX                    */
	};
	static unsigned char sHexa[] = "0123456789ABCDEF";
	unsigned char* aText;
	unsigned char* aStop;
	unsigned char* aStart;
	unsigned char aBuffer[8];
	unsigned char aChar;

	if (!data) return err;
	aText = (unsigned char*)data;
	aStop = aStart = aText;
	while ((aChar = *aText++)) {
		if (sEscape[aChar] & theFlag) {
			if (aStop > aStart) {
				*aStop = '\0';
 				bailIfError(FskGrowableStorageAppendItem(storage, aStart, aStop - aStart));
				*aStop = aChar;
			}
			switch (aChar) {
			case '"':
 				bailIfError(FskGrowableStorageAppendItem(storage, "&quot;", 6));
				break;
			case '&':
 				bailIfError(FskGrowableStorageAppendItem(storage, "&amp;", 5));
				break;
			case '<':
 				bailIfError(FskGrowableStorageAppendItem(storage, "&lt;", 4));
				break;
			case '>':
 				bailIfError(FskGrowableStorageAppendItem(storage, "&gt;", 4));
				break;
			default:
				aStart = aBuffer;
				*(aStart++) = '&';
				*(aStart++) = '#';
				*(aStart++) = 'x';
				if (aChar >= 16)
					*(aStart++) = sHexa[aChar / 16];
				*(aStart++) = sHexa[aChar % 16];
				*(aStart++) = ';';
				*aStart = 0;
 				bailIfError(FskGrowableStorageAppendItem(storage, aBuffer, aStart - aBuffer));
				break;
			}
			aStart = ++aStop;
		}
		else
			aStop++;
	}
	if (aStop > aStart) {
		bailIfError(FskGrowableStorageAppendItem(storage, aStart, aStop - aStart));
	}
bail:
	return err;
}

FskErr KprUPnPUUIDCreate(FskUUID uuid)
{
	FskErr err = kFskErrNone;
	bailIfError(FskUUIDCreate(uuid));
	uuid->value[6] = (uuid->value[6] & 0x0f) | 0x10; // DLNA CTT 7.2.20.1 - DCE version 0001b
bail:
	return err;
}

char* KprUPnPUUIDGetForKey(const char *key)
{
	char* uuid = FskUUIDGetForKey(key);
	uuid[14] = '1'; // DLNA CTT 7.2.20.1 - DCE version 0001b
	return uuid;
}

//--------------------------------------------------
// UPnP Device
//--------------------------------------------------
#if 0
#pragma mark - KprUPnPDevice
#endif

FskErr KprUPnPDeviceNew(KprUPnPDevice* it, char* type)
{
	FskErr err = kFskErrNone;
	KprUPnPDevice self = NULL;
	char* ptr;
	char* search;
	UInt32 size;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprUPnPDeviceRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprUPnPDeviceInstrumentation);

	self->type = FskStrDoCopy(type);
	self->version = -1;
	ptr = type;
	if (!FskStrCompareWithLength(ptr, "urn:", 4)) {
		ptr += 4;
		search = FskStrStr(ptr, ":device:");
		if (search) {
			size = search - ptr;
			bailIfError(FskMemPtrNewClear(size + 1, &self->domain));
			FskStrNCopy(self->domain, ptr, size);
			ptr += size + 8;
			
			search = FskStrChr(ptr, ':');
			if (search) {
				size = search - ptr;
				bailIfError(FskMemPtrNewClear(size + 1, &self->name));
				FskStrNCopy(self->name, ptr, size);
				self->version = atoi(search + 1);
			}
		}
	}
	if (self->version == -1) {
		BAIL(kFskErrBadData);
	}
	return err;
bail:
	KprUPnPDeviceDispose(self);
	return err;
}

void KprUPnPDeviceDispose(KprUPnPDevice self)
{
	if (self) {
		{	// dispose services
			KprUPnPService service, next;
			for (service = self->service; service; service = next) {
				next = service->next;
				KprUPnPServiceDispose(service);
			}
		}
		{	// dispose icons
			KprUPnPIcon icon, next;
			for (icon = self->icon; icon; icon = next) {
				next = icon->next;
				KprUPnPIconDispose(icon);
			}
		}
		FskMemPtrDispose(self->domain);
		FskMemPtrDispose(self->name);
		FskMemPtrDispose(self->type);
		FskMemPtrDispose(self->url);
		
		FskMemPtrDispose(self->configId);
		FskMemPtrDispose(self->description);
		FskMemPtrDispose(self->presentationURL);
		FskMemPtrDispose(self->friendlyName);
		FskMemPtrDispose(self->manufacturer);
		FskMemPtrDispose(self->modelName);
		FskMemPtrDispose(self->modelNumber);
		FskMemPtrDispose(self->uuid);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprUPnPDeviceFromElement(KprUPnPDevice self, KprXMLElement device, char* uuid)
{
	FskErr err = kFskErrNone;
	char* friendlyName;
	char* modelNumber;
	KprXMLElement element, item, list;
	char* udn;
	
	// friendlyName
	element = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "friendlyName");
	bailIfNULL(element);
	bailIfNULL(element->element);
	bailIfNULL(element->element->value);
	if ((friendlyName = self->friendlyName)) {
		bailIfNULL(friendlyName = FskStrDoCopy(friendlyName));
		FskMemPtrDispose(element->element->value);
		element->element->value = friendlyName;
		element->element->valueSize = FskStrLen(friendlyName);
	}
	else {
		bailIfNULL(self->friendlyName = FskStrDoCopy(element->element->value));
	}
	// manufacturer
	element = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "manufacturer");
	bailIfNULL(element);
	bailIfNULL(element->element);
	bailIfNULL(element->element->value);
	self->manufacturer = FskStrDoCopy(element->element->value);
	// modelName
	element = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "modelName");
	bailIfNULL(element);
	bailIfNULL(element->element);
	bailIfNULL(element->element->value);
	self->modelName = FskStrDoCopy(element->element->value);
	// modelNumber
	element = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "modelNumber");
	if (element) {
		bailIfNULL(element->element);
		bailIfNULL(element->element->value);
		if ((modelNumber = self->modelNumber)) {
			bailIfNULL(modelNumber = FskStrDoCopy(modelNumber));
			FskMemPtrDispose(element->element->value);
			element->element->value = modelNumber;
			element->element->valueSize = FskStrLen(modelNumber);
		}
		else {
			bailIfNULL(self->modelNumber = FskStrDoCopy(element->element->value));
		}
	}
	// presentationURL
	element = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "presentationURL");
	if (element) {
		self->presentationURL = FskStrDoCopy(KprXMLElementGetValue(element));
	}
	// udn
	self->uuid = FskStrDoCopy(uuid);
	bailIfError(FskMemPtrNew(6 + FskStrLen(uuid), &udn));
	FskStrCopy(udn, "uuid:");
	FskStrCat(udn, uuid);
	element = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "UDN");
	bailIfNULL(element);
	FskMemPtrDispose(element->element->value);
	element->element->value = udn;
	element->element->valueSize = FskStrLen(udn);
	
	list = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "iconList");
	if (list) {
		for (item = KprXMLElementGetFirstElement(list, kUPnPDeviceNameSpace, "icon"); item; item = KprXMLElementGetNextElement(item, kUPnPDeviceNameSpace, "icon")) {
			bailIfError(KprUPnPIconFromElement(item, self));
		}
	}
	list = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "serviceList");
	if (list) {
		for (item = KprXMLElementGetFirstElement(list, kUPnPDeviceNameSpace, "service"); item; item = KprXMLElementGetNextElement(item, kUPnPDeviceNameSpace, "service")) {
			bailIfError(KprUPnPServiceFromElement(item, self));
		}
	}
bail:
	return err;
}

FskErr KprUPnPDeviceFromFile(KprUPnPDevice self, char* url)
{
	FskErr err = kFskErrNone;
	char* slash = NULL, tmp;
	char* path = NULL;
	FskInt64 size;
	unsigned char *data;
	FskFileMapping map = NULL;
	KprXMLElement root = NULL;
	KprXMLElement version, device;
	char* minor;
	char* major;
	char* configId;
	
	self->url = FskStrDoCopy(url);
	slash = FskStrRChr(url, '/');
	bailIfNULL(slash);
	slash++;
	tmp = *slash;
	*slash = 0;
	bailIfError(KprURLToPath(url, &self->path));
	bailIfError(KprUPnPDeviceGetPath(self, self->type, &path));
	bailIfError(FskFileMap(path, &data, &size, 0, &map));
	bailIfError(KprXMLParse(&root, data, size));
			
	// configId
	version = KprXMLElementGetFirstElement(root, kUPnPDeviceNameSpace, "specVersion");
	bailIfNULL(version);
	major = KprXMLElementGetProperty(version, kUPnPDeviceNameSpace, "major");
	minor = KprXMLElementGetProperty(version, kUPnPDeviceNameSpace, "minor");
	configId = KprXMLElementGetAttribute(root, "configId");
	if (!FskStrCompare(major, "1") && !FskStrCompare(minor, "1") && configId) {
		KprXMLElementSetAttributeValue(root, "configId", self->configId);
	}
	else if (FskStrCompare(major, "1") || FskStrCompare(minor, "0") || configId) {
		BAIL(kFskErrBadData);
	}
	
	device = KprXMLElementGetFirstElement(root, kUPnPDeviceNameSpace, "device");
	if (!device) {
		BAIL(kFskErrBadData);
	}
	bailIfError(KprUPnPDeviceFromElement(self, device, KprUPnPUUIDGetForKey(self->type)));
	
	{	// parse service scpd
		KprUPnPService service;
		for (service = self->service; service; service = service->next) {
			bailIfError(KprUPnPServiceFromFile(service, self->configId));
		}
	}
	
	KprXMLSerialize(root, &self->description, &self->descriptionSize);
	
bail:
	if (slash) *slash = tmp;
	FskMemPtrDispose(path);
	KprXMLElementDispose(root);
	FskFileDisposeMap(map);
	return err;
}

FskErr KprUPnPDeviceGetPath(KprUPnPDevice self, char* type, char** path)
{
	FskErr err = kFskErrNone;
	char* from;
	char* to;
	
	to = FskStrRChr(type, ':');
	if (to) {
		*to = 0;
		from = FskStrRChr(type, ':');
		if (from) {
			*from = 0;
			bailIfError(FskMemPtrNewClear(FskStrLen(self->path) + FskStrLen(from + 1) + 5, path));
			FskStrCopy(*path, self->path);
			FskStrCat(*path, from + 1);
			FskStrCat(*path, ".xml");
			*from = ':';
		}
		*to = ':';
	}
bail:
	return err;
}

//KprUPnPService KprUPnPDeviceGetService(KprUPnPDevice self UNUSED, char* type UNUSED)
//{
////	return (KprUPnPService)FskAssociativeArrayElementGetReference(self->service, type);
//	return NULL;
//}

FskErr KprUPnPDeviceGetServiceURL(KprUPnPDevice self, char* type, char** url)
{
	FskErr err = kFskErrNone;
	char* from;
	char* to;
	char* path = NULL;
	
	to = FskStrRChr(type, ':');
	if (to) {
		*to = 0;
		from = FskStrRChr(type, ':');
		if (from) {
			*from = 0;
			bailIfError(FskMemPtrNewClear(FskStrLen(from + 1) + 5, &path));
			FskStrCopy(path, from + 1);
			FskStrCat(path, ".xml");
			*from = ':';
			bailIfError(KprURLMerge(self->url, path, url));
		}
		*to = ':';
	}
bail:
	FskMemPtrDispose(path);
	return err;
}

//--------------------------------------------------
// UPnP Icon
//--------------------------------------------------
#if 0
#pragma mark - KprUPnPIcon
#endif

FskErr KprUPnPIconNew(KprUPnPIcon* it)
{
	FskErr err = kFskErrNone;
	KprUPnPIcon self = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprUPnPIconRecord), &self));
	*it = self;
	FskInstrumentedItemNew(self, NULL, &KprUPnPIconInstrumentation);
bail:
	return err;
}

void KprUPnPIconDispose(KprUPnPIcon self)
{
	if (self) {
		FskMemPtrDispose(self->mime);
		FskMemPtrDispose(self->path);
		FskMemPtrDispose(self->url);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprUPnPIconFromElement(KprXMLElement element, KprUPnPDevice device)
{
	FskErr err = kFskErrNone;
	char* url;
	char* mime;
	char* width;
	char* height;
	char* depth;
	KprUPnPIcon self;
	char* iconURL = NULL;

	url = KprXMLElementGetProperty(element, kUPnPDeviceNameSpace, "url");
	mime = KprXMLElementGetProperty(element, kUPnPDeviceNameSpace, "mimetype");
	width = KprXMLElementGetProperty(element, kUPnPDeviceNameSpace, "width");
	height = KprXMLElementGetProperty(element, kUPnPDeviceNameSpace, "height");
	depth = KprXMLElementGetProperty(element, kUPnPDeviceNameSpace, "depth");
	if (!url || !mime || !width || !height || !depth) {
		BAIL(kFskErrBadData);
	}
	bailIfError(KprUPnPIconNew(&self));
	bailIfError(KprURLMerge(device->url, url, &iconURL));
	bailIfError(KprURLToPath(iconURL, &self->path));
	
	if (device->controller)
		self->url = FskStrDoCopy(iconURL);
	else
		self->url = FskStrDoCopy(url);
	bailIfNULL(self->url);
	
	self->mime = FskStrDoCopy(mime);
	self->width = FskStrToNum(width);
	self->height = FskStrToNum(height);
	self->depth = FskStrToNum(depth);
	FskListAppend(&device->icon, self);
bail:
	FskMemPtrDispose(iconURL);
	return err;
}

//--------------------------------------------------
// UPnP Service
//--------------------------------------------------
#if 0
#pragma mark - KprUPnPService
#endif

FskErr KprUPnPServiceNew(KprUPnPService* it, char* type)
{
	FskErr err = kFskErrNone;
	KprUPnPService self = NULL;
	char* ptr;
	char* search;
	UInt32 size;
	
	if (!type) {
		BAIL(kFskErrBadData);
	}
	bailIfError(FskMemPtrNewClear(sizeof(KprUPnPServiceRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprUPnPServiceInstrumentation);
	self->version = -1;
	ptr = type;
	if (!FskStrCompareWithLength(ptr, "urn:", 4)) {
		ptr += 4;
		search = FskStrStr(ptr, ":service:");
		if (search) {
			size = search - ptr;
			bailIfError(FskMemPtrNewClear(size + 1, &self->domain));
			FskStrNCopy(self->domain, ptr, size);
			ptr += size + 9;
			
			search = FskStrChr(ptr, ':');
			if (search) {
				size = search - ptr;
				bailIfError(FskMemPtrNewClear(size + 1, &self->name));
				FskStrNCopy(self->name, ptr, size);
				self->version = atoi(search + 1);
			}
		}
	}
	if (self->version == -1) {
		BAIL(kFskErrBadData);
	}
	self->type = FskStrDoCopy(type);
	
	self->actions = FskAssociativeArrayNew();
	self->stateVariables = FskAssociativeArrayNew();
	return err;
bail:
	KprUPnPServiceDispose(self);
	return err;
}

void KprUPnPServiceDispose(KprUPnPService self)
{
	KprUPnPController controller = self->device->controller;
	if (self) {
		{	// dispose subscriptions
			KprUPnPSubscription subscription, next;
			for (subscription = self->subscription; subscription; subscription = next) {
				next = subscription->next;
				if (controller)
					KprUPnPSubscriptionUnsubscribe(subscription, controller);
				else
					KprUPnPSubscriptionDispose(subscription);
			}
			self->subscription = NULL;
		}
		{	// dispose actions
			FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(self->actions);
			while (iterate) {
				KprUPnPAction action = (KprUPnPAction)iterate->value;
				KprUPnPActionDispose(action);
				iterate = FskAssociativeArrayIteratorNext(iterate);
			}
			FskAssociativeArrayIteratorDispose(iterate);
			FskAssociativeArrayDispose(self->actions);
		}
		{	// dispose state variables
			FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(self->stateVariables);
			while (iterate) {
				KprUPnPStateVariable variable = (KprUPnPStateVariable)iterate->value;
				KprUPnPStateVariableDispose(variable);
				iterate = FskAssociativeArrayIteratorNext(iterate);
			}
			FskAssociativeArrayIteratorDispose(iterate);
			FskAssociativeArrayDispose(self->stateVariables);
		}
		FskMemPtrDispose(self->actionErrorDescription);
		FskMemPtrDispose(self->controlURL);
		FskMemPtrDispose(self->description);
		FskMemPtrDispose(self->domain);
		FskMemPtrDispose(self->eventSubURL);
		FskMemPtrDispose(self->SCPDURL);
		FskMemPtrDispose(self->name);
		FskMemPtrDispose(self->url);
		FskMemPtrDispose(self->type);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprUPnPServiceActionInvoke(KprUPnPService self, KprContext context, KprHandler handler, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprXMLElement root = NULL;
	KprXMLElement element = NULL;
	KprUPnPHandlerBehavior behavior = (KprUPnPHandlerBehavior)handler->behavior;
	KprMessage action = NULL;
	char* url = NULL;
	behavior->action = NULL;
	if (KprXMLParse(&root, message->request.body, message->request.size) == kFskErrNone) {
		if (KprXMLElementIsEqual(root, kUPnPSoapEnvelopeNameSpace, "Envelope")) {
			element = KprXMLElementGetFirstElement(root, kUPnPSoapEnvelopeNameSpace, "Body");
			if (element) {
				element = element->element;
				behavior->action = FskAssociativeArrayElementGetReference(self->actions, element->name);
				self->actionError = 0;
				if (self->actionErrorDescription)
					FskMemPtrDisposeAt(&self->actionErrorDescription);
				if (behavior->action && KprXMLElementIsEqual(element, self->type, element->name)) {
					KprUPnPArgument argument;
					KprXMLElement arg = NULL;
					FskErr upnpErr;
					UInt32 size;
					
					FskInstrumentedItemPrintfNormal(self, "UPnP Service %p -> %s/%s", message, self->name, behavior->action->name);
					FskInstrumentedItemPrintfDebug(self, "%.*s", message->request.size, message->request.body);
					for (argument = behavior->action->argumentIn, arg = element->element, upnpErr = kUPnPErrInvalidArgs; argument && arg; argument = argument->next, arg = arg->next, upnpErr = kUPnPErrInvalidArgs) {
						if (!argument->relatedStateVariable || FskStrCompare(argument->name, arg->name)) break;
					//	if (!argument->relatedStateVariable || !arg->element || FskStrCompare(argument->name, arg->name)) break;
						if ((upnpErr = KprUPnPStateVariableSetValue(argument->relatedStateVariable, arg->element ? arg->element->value : ""))) break;
						FskInstrumentedItemPrintfVerbose(self, " -> %s = '%s' (%d)", argument->name, arg->element ? arg->element->value : "", argument->relatedStateVariable->changed);
					}
					if (argument) // argument mismatch
						self->actionError = upnpErr;
				//	else if (arg) { // ignore extra arguments UDA-3.1.3
				//		self->actionError = kUPnPErrInvalidArgs;
				//		KprUPnPServiceActionResponse(self, context, handler, message);
				//	}
					size = 7 + FskStrLen(context->id) + 1 + FskStrLen(self->device->name) + 1 + FskStrLen(self->name) + 1 + FskStrLen(element->name) + 1;
					if (self->actionError) {
						size += 10;
						if (arg) {
							size += 6 + FskStrLen(argument->name);
							if (self->actionError != kUPnPErrInvalidArgs)
								size += 7;
								if (arg->element) size += FskStrLen(arg->element->value);
						}
					}
					bailIfError(FskMemPtrNewClear(size, &url));
					FskStrCopy(url, "xkpr://");
					FskStrCat(url, context->id);
					FskStrCat(url, "/");
					FskStrCat(url, self->device->name);
					FskStrCat(url, "/");
					FskStrCat(url, self->name);
					FskStrCat(url, "/");
					FskStrCat(url, element->name);
					if (self->actionError) {
						char string[4];
						FskStrCat(url, "?error=");
						FskStrNumToStr(self->actionError, string, sizeof(string));
						FskStrCat(url, string);
						if (arg) {
							FskStrCat(url, "&name=");
							FskStrCat(url, argument->name);
							if (self->actionError != kUPnPErrInvalidArgs) {
								FskStrCat(url, "&value=");
								if (arg->element) FskStrCat(url, arg->element->value);
							}
						}
					}
					bailIfError(KprMessageNew(&action, url));
					action->request.headers = message->request.headers;
					message->request.headers = NULL;
					KprHandlerTrigger(handler, action);
				}
			}
		}
	}
	else {
		self->actionError = kUPnPErrInvalidAction;
	}
	if (!behavior->action) {
		self->actionError = kUPnPErrInvalidAction;
		KprUPnPServiceActionResponse(self, context, handler, message);
	}
bail:
	if (err)
		KprMessageDispose(action);
	FskMemPtrDispose(url);
	KprXMLElementDispose(root);
	return err;
}

void KprUPnPServiceActionResponse(KprUPnPService self, KprContext UNUSED context, KprHandler handler, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprUPnPHandlerBehavior behavior = (KprUPnPHandlerBehavior)handler->behavior;
	KprUPnPAction action = behavior->action;
	KprUPnPArgument argument;
	char string[16];
	FskGrowableStorage storage = NULL;
	UInt32 size;
	void* data;
	char* description = NULL;
	
	KprHTTPTargetMessageSetResponseProtocol(message, "HTTP/1.1"); // DLNA CTT 7.2.5.6
	KprMessageSetResponseHeader(message, kFskStrContentType, kFskStrTextXMLCharset);
	KprMessageSetResponseHeader(message, kFskStrServer, gUPnP->userAgent);
	bailIfError(FskGrowableStorageNew(1024, &storage));
	bailIfError(KprUPnPStorageWrite(storage, "<?xml version=\"1.0\"?>\n", 0));
	bailIfError(KprUPnPStorageWrite(storage, "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"", 0));
	bailIfError(KprUPnPStorageWrite(storage, kUPnPSoapEncodingNameSpace, 0));
	bailIfError(KprUPnPStorageWrite(storage, "\">\n\t<s:Body>\n", 0));
	if (self->actionError) {
		message->status = 500;
		bailIfError(KprUPnPStorageWrite(storage, "\t\t<s:Fault>\n\t\t\t<faultcode>s:Client</faultcode>\n\t\t\t<faultstring>UPnPError</faultstring>\n\t\t\t<detail>\n\t\t\t\t<UPnPError xmlns=\"", 0));
		bailIfError(KprUPnPStorageWrite(storage, kUPnPControlNameSpace, 0));
		bailIfError(KprUPnPStorageWrite(storage, "\">\n", 0));
		bailIfError(KprUPnPStorageWrite(storage, "\t\t\t\t\t<errorCode>", 0));
		FskStrNumToStr(self->actionError, string, sizeof(string));
		bailIfError(KprUPnPStorageWrite(storage, string, 0));
		bailIfError(KprUPnPStorageWrite(storage, "</errorCode>\n", 0));
		description = (self->actionErrorDescription) ? self->actionErrorDescription : KprUPnPFindError(self->actionError);
		if (description) {
			bailIfError(KprUPnPStorageWrite(storage, "\t\t\t\t\t<errorDescription>", 0));
			bailIfError(KprUPnPStorageWrite(storage, description, 0));
			bailIfError(KprUPnPStorageWrite(storage, "</errorDescription>\n", 0));
		}
		bailIfError(KprUPnPStorageWrite(storage, "\t\t\t\t</UPnPError>\n\t\t\t</detail>\n\t\t</s:Fault>\n", 0));
		FskInstrumentedItemPrintfNormal(self, "UPnP Service %p <- %s/%s -> %lu (%s)", message, self->name, (action) ? action->name : "?", self->actionError, description);
	}
	else if (action) {
		FskInstrumentedItemPrintfNormal(self, "UPnP Service %p <- %s/%s", message, self->name, behavior->action->name);
		message->status = 200;
		bailIfError(KprUPnPStorageWrite(storage, "\t\t<u:", 0));
		bailIfError(KprUPnPStorageWrite(storage, action->name, 0));
		bailIfError(KprUPnPStorageWrite(storage, "Response xmlns:u=\"", 0));
		bailIfError(KprUPnPStorageWrite(storage, self->type, 0));
		bailIfError(KprUPnPStorageWrite(storage, "\">\n", 0));
		for (argument = action->argumentOut; argument; argument = argument->next) {
			char* value = KprUPnPStateVariableGetValue(argument->relatedStateVariable);
			bailIfError(KprUPnPStorageWrite(storage, "\t\t\t<", 0));
			bailIfError(KprUPnPStorageWrite(storage, argument->name, 0));
			bailIfError(KprUPnPStorageWrite(storage, ">", 0));
			bailIfError(KprUPnPStorageWriteEntity(storage, value, 2));
			bailIfError(KprUPnPStorageWrite(storage, "</", 0));
			bailIfError(KprUPnPStorageWrite(storage, argument->name, 0));
			bailIfError(KprUPnPStorageWrite(storage, ">\n", 0));
			FskInstrumentedItemPrintfVerbose(self, " <- %s = '%s'", argument->name, value);
		}
		bailIfError(KprUPnPStorageWrite(storage, "\t\t</u:", 0));
		bailIfError(KprUPnPStorageWrite(storage, action->name, 0));
		bailIfError(KprUPnPStorageWrite(storage, "Response>\n", 0));
	}
	else {
		FskInstrumentedItemPrintfVerbose(self, "UPnP Service");
	}
	bailIfError(KprUPnPStorageWrite(storage, "\t</s:Body>\n</s:Envelope>", 0));
	FskGrowableStorageGetPointerToItem(storage, 0, (void **)&data);
	size = FskGrowableStorageGetSize(storage);
	bailIfError(KprMessageSetResponseBody(message, data, size));
	FskInstrumentedItemPrintfDebug(self, "%.*s", size, data);
	FskGrowableStorageDispose(storage);
	FskStrNumToStr(message->response.size, string, sizeof(string));
	KprMessageSetResponseHeader(message, kFskStrContentLength, string);
	KprMessageSetResponseHeader(message, "EXT", "");
	self->actionError = 0;
	if (self->actionErrorDescription)
		FskMemPtrDisposeAt(&self->actionErrorDescription);

	return;
bail:
	FskGrowableStorageDispose(storage);
	message->status = 500;
	return;
}

FskErr KprUPnPServiceFromElement(KprXMLElement element, KprUPnPDevice device)
{
	FskErr err = kFskErrNone;
	KprUPnPService self = NULL;
	char* serviceType;
	char* serviceId;
	char* path;

	serviceType = KprXMLElementGetProperty(element, kUPnPDeviceNameSpace, "serviceType");
	bailIfNULL(serviceType);
	bailIfError(KprUPnPServiceNew(&self, serviceType));
	self->device = device;

	serviceId = KprXMLElementGetProperty(element, kUPnPDeviceNameSpace, "serviceId");
	
	path = KprXMLElementGetProperty(element, kUPnPDeviceNameSpace, "SCPDURL");
	if (device->controller) {
		bailIfError(KprURLMerge(device->url, path, &self->SCPDURL));
	}
	else {
		self->SCPDURL = FskStrDoCopy(path);
		bailIfNULL(self->SCPDURL);
	}
	path = KprXMLElementGetProperty(element, kUPnPDeviceNameSpace, "controlURL");
	if (device->controller) {
		bailIfError(KprURLMerge(device->url, path, &self->controlURL));
	}
	else {
		self->controlURL = FskStrDoCopy(path);
		bailIfNULL(self->controlURL);
	}
	path = KprXMLElementGetProperty(element, kUPnPDeviceNameSpace, "eventSubURL");
	if (path) {
		if (device->controller) {
			bailIfError(KprURLMerge(device->url, path, &self->eventSubURL));
		}
		else {
			self->eventSubURL = FskStrDoCopy(path);
			bailIfNULL(self->eventSubURL);
		}
	}

	if (FskStrStr(device->url, "http://") == device->url) {
		bailIfError(KprURLMerge(device->url, self->SCPDURL, &self->url));
	}
	else {
		bailIfError(KprUPnPDeviceGetServiceURL(device, serviceType, &self->url));
		bailIfError(KprUPnPDeviceGetPath(device, serviceType, &self->path));
	}

	if (!serviceType || !serviceId || !self->SCPDURL || !self->controlURL) {
		BAIL(kFskErrBadData);
	}
	FskListAppend(&device->service, self);
	return err;
bail:
	KprUPnPServiceDispose(self);
	return err;
}

FskErr KprUPnPServiceFromFile(KprUPnPService self, char* deviceConfigId)
{
	FskErr err = kFskErrNone;
	FskInt64 size;
	unsigned char *data;
	FskFileMapping map = NULL;
	FskFileInfo info;
	KprXMLElement scpd = NULL;
	KprXMLElement version;
	char* major;
	char* minor;
	char* configId;

	if (FskFileGetFileInfo(self->path, &info)) {
		BAIL(kFskErrBadData);
	}
	bailIfError(FskFileMap(self->path, &data, &size, 0, &map));
	bailIfError(KprXMLParse(&scpd, data, size));

	version = KprXMLElementGetFirstElement(scpd, kUPnPServiceNameSpace, "specVersion");
	bailIfNULL(version);
	major = KprXMLElementGetProperty(version, kUPnPServiceNameSpace, "major");
	minor = KprXMLElementGetProperty(version, kUPnPServiceNameSpace, "minor");
	configId = KprXMLElementGetAttribute(scpd, "configId");
	if (!FskStrCompare(major, "1") && !FskStrCompare(minor, "1") && configId) {
		KprXMLElementSetAttributeValue(scpd, "configId", deviceConfigId);
	}
	else if (FskStrCompare(major, "1") || FskStrCompare(minor, "0") || configId) {
		BAIL(kFskErrBadData);
	}
	bailIfError(KprUPnPServiceFromSCPDElement(self, scpd));
	
	KprXMLSerialize(scpd, &self->description, &self->descriptionSize);
bail:
	FskFileDisposeMap(map);
	KprXMLElementDispose(scpd);
	return err;
}

FskErr KprUPnPServiceFromSCPDElement(KprUPnPService self, KprXMLElement scpd)
{
	FskErr err = kFskErrNone;
	KprXMLElement action, actionList = KprXMLElementGetFirstElement(scpd, kUPnPServiceNameSpace, "actionList");
	KprXMLElement stateVariable, stateVariableList = KprXMLElementGetFirstElement(scpd, kUPnPServiceNameSpace, "serviceStateTable");
	if (!stateVariableList) // some device mistakenly use kUPnPDeviceNameSpace instaed of kUPnPServiceNameSpace
		stateVariableList = KprXMLElementGetFirstElement(scpd, kUPnPDeviceNameSpace, "serviceStateTable");
	if (stateVariableList) {
		for (stateVariable = KprXMLElementGetFirstElement(stateVariableList, kUPnPServiceNameSpace, "stateVariable"); stateVariable; stateVariable = KprXMLElementGetNextElement(stateVariable, kUPnPServiceNameSpace, "stateVariable")) {
			bailIfError(KprUPnPStateVariableFromElement(stateVariable, self));
		}
	}
	if (actionList) {
		for (action = KprXMLElementGetFirstElement(actionList, kUPnPServiceNameSpace, "action"); action; action = KprXMLElementGetNextElement(action, kUPnPServiceNameSpace, "action")) {
			bailIfError(KprUPnPActionFromElement(action, self));
		}
	}
bail:
	return err;
}

// subscription

FskErr KprUPnPServiceAddSubscription(KprUPnPService self, char* url, UInt32 duration, char** uuid)
{
	FskErr err = kFskErrNone;
	KprUPnPSubscription subscription = NULL;

	for (subscription = self->subscription; subscription; subscription = subscription->next) {
		if (!FskStrCompare(subscription->url, url)) {
			break;
		}
	}
	if (subscription) {
		KprUPnPStringData item;
		subscription->duration = duration;
		*uuid = subscription->uuid;
		FskInstrumentedItemPrintfNormal(subscription, "Recover %s to %s for %lu seconds", self->name, url, subscription->duration);
		for (item = subscription->values; item; item = item->next)
			FskMemPtrDisposeAt(&item->value);
		for (item = subscription->lastChanges; item; item = item->next)
			FskMemPtrDisposeAt(&item->value);
		if (subscription->event) {
			KprMessageCancel(subscription->event);
			subscription->event = NULL;
		}
		subscription->eventModeration = false;
	}
	else {
		bailIfError(KprUPnPSubscriptionNew(&subscription, self, url, duration));
		FskListAppend(&self->subscription, subscription);
		*uuid = subscription->uuid;
		FskInstrumentedItemPrintfNormal(subscription, "Add %s to %s for %lu seconds", self->name, url, subscription->duration);
	}
	FskTimeCallbackScheduleFuture(subscription->timer, subscription->duration, 0, KprUPnPSubscriptionExpire, subscription);
	bailIfError(KprUPnPSubscriptionEvent(subscription, true));
bail:
	return err;
}

FskErr KprUPnPServiceEvent(KprUPnPService self)
{
	FskErr err = kFskErrNone;
	KprUPnPSubscription subscription = NULL;

//	FskInstrumentedItemPrintfDebug(self, "Event %s", self->name);
	for (subscription = self->subscription; subscription; subscription = subscription->next) {
		bailIfError(KprUPnPSubscriptionEvent(subscription, true));
	}
bail:
	return err;
}

FskErr KprUPnPServiceRenewSubscription(KprUPnPService self, char* uuid, UInt32 duration)
{
	FskErr err = kFskErrNone;
	KprUPnPSubscription subscription = NULL;
	for (subscription = self->subscription; subscription; subscription = subscription->next) {
		if (!FskStrCompare(subscription->uuid, uuid)) break;
	}
	if (!subscription) {
		BAIL(kFskErrInvalidParameter);
	}
	subscription->duration = duration;
	FskInstrumentedItemPrintfNormal(subscription, "Renew %s to %s for %lu seconds", self->name, subscription->url, subscription->duration);
	FskTimeCallbackScheduleFuture(subscription->timer, subscription->duration, 0, KprUPnPSubscriptionExpire, subscription);
bail:
	return err;
}

FskErr KprUPnPServiceRemoveSubscription(KprUPnPService self, char* uuid)
{
	FskErr err = kFskErrNone;
	KprUPnPSubscription subscription = NULL;
	for (subscription = self->subscription; subscription; subscription = subscription->next) {
		if (!FskStrCompare(subscription->uuid, uuid)) break;
	}
	if (!subscription) {
		BAIL(kFskErrInvalidParameter);
	}
	FskListRemove(&self->subscription, subscription);
	FskInstrumentedItemPrintfNormal(subscription, "Remove %s to %s", self->name, subscription->url);
	if (subscription->notified)
		KprUPnPSubscriptionDispose(subscription);
	else
		subscription->removed = true;
bail:
	return err;
}

//--------------------------------------------------
// UPnP Subscription
//--------------------------------------------------
#if 0
#pragma mark - KprUPnPSubscription
#endif

FskErr KprUPnPSubscriptionNew(KprUPnPSubscription* it, KprUPnPService service, char* url, UInt32 duration)
{
	FskErr err = kFskErrNone;
	KprUPnPSubscription self = NULL;
	FskUUIDRecord uuid;
	char* uuidString = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprUPnPSubscriptionRecord), it));
	self = *it;
	self->service = service;
	self->url = FskStrDoCopy(url);
	self->duration = duration;
	if (!service->device->controller) {
		bailIfError(KprUPnPUUIDCreate(&uuid));
		bailIfError(FskMemPtrNewClear(42, &self->uuid));
		FskStrCopy(self->uuid, "uuid:");
		uuidString = FskUUIDtoString_844412(&uuid);
		FskStrCat(self->uuid, uuidString);
	}
	else {
		self->eventSubURL = FskStrDoCopy(service->eventSubURL);
	}
	FskTimeCallbackNew(&self->timer);
	FskTimeCallbackNew(&self->eventTimer);
	bailIfError(FskGrowableStorageNew(1024, &self->eventBody));
	{	// allocate variable value placeholders
		KprUPnPStringData* current = &self->values;
		KprUPnPStringData* lastChange = &self->lastChanges;
		FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(service->stateVariables);
		
		while (iterate) {
			KprUPnPStateVariable variable = (KprUPnPStateVariable)iterate->value;
			KprUPnPStringData value = NULL;
			
			if (variable->sendEvents) {
				bailIfError(FskMemPtrNewClear(sizeof(KprUPnPStringDataRecord), &value));
				*current = value;
				current = &(value->next);
			}
			if (variable->lastChange) {
				bailIfError(FskMemPtrNewClear(sizeof(KprUPnPStringDataRecord), &value));
				*lastChange = value;
				lastChange = &(value->next);
			}
			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		FskAssociativeArrayIteratorDispose(iterate);
	}
	FskInstrumentedItemNew(self, NULL, &KprUPnPSubscriptionInstrumentation);
	FskMemPtrDispose(uuidString);
	return err;
bail:
	FskMemPtrDispose(uuidString);
	KprUPnPSubscriptionDispose(self);
	return err;
}

void KprUPnPSubscriptionDispose(KprUPnPSubscription self)
{
	if (self) {
		if (self->timer) {
			FskTimeCallbackDispose(self->timer);
			self->timer = NULL;
		}
		if (self->event)
			KprMessageCancel(self->event);
		if (self->eventTimer) {
			FskTimeCallbackDispose(self->eventTimer);
			self->eventTimer = NULL;
		}
		if (self->values) {
			KprUPnPStringData next, current;
			for (current = self->values; current; current = next) {
				next = current->next;
				FskMemPtrDispose(current->value);
				FskMemPtrDispose(current);
			}
		}
		if (self->lastChanges) {
			KprUPnPStringData next, current;
			for (current = self->lastChanges; current; current = next) {
				next = current->next;
				FskMemPtrDispose(current->value);
				FskMemPtrDispose(current);
			}
		}
		FskGrowableStorageDispose(self->eventBody);
		FskMemPtrDispose(self->uuid);
		FskMemPtrDispose(self->url);
		FskMemPtrDispose(self->eventSubURL);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprUPnPSubscriptionEventLastChange(KprUPnPSubscription self, KprUPnPStringData lastChange)
{
	FskErr err = kFskErrNone;
	Boolean empty = true;
	FskGrowableStorage storage = NULL;
	FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(self->service->stateVariables);
	KprUPnPStringData current = NULL;

	bailIfError(FskGrowableStorageNew(1024, &storage));

	bailIfError(KprUPnPStorageWrite(storage, "<Event xmlns=\"", 0));
	bailIfError(KprUPnPStorageWriteEntity(storage, self->service->lastChangeNamespace, 1));
	bailIfError(KprUPnPStorageWrite(storage, "\">", 0));
	bailIfError(KprUPnPStorageWrite(storage, "<InstanceID val=\"0\">", 0));

	current = self->lastChanges;
	while (iterate) {
		KprUPnPStateVariable variable = (KprUPnPStateVariable)iterate->value;
		if (variable->lastChange) {
			if (FskStrCompare(current->value, variable->value)) {
				FskMemPtrDispose(current->value);
				current->value = FskStrDoCopy(variable->value);
				if (current->value) {
					bailIfError(KprUPnPStorageWrite(storage, "<", 0));
					bailIfError(KprUPnPStorageWrite(storage, iterate->name, 0));
					if (!FskStrCompare(iterate->name, "Mute") || !FskStrCompare(iterate->name, "Volume")) {
						KprUPnPStateVariable channel = FskAssociativeArrayElementGetReference(self->service->stateVariables, "A_ARG_TYPE_Channel");
						if (channel) {
							bailIfError(KprUPnPStorageWrite(storage, " channel=\"", 0));
							bailIfError(KprUPnPStorageWriteEntity(storage, channel->value, 1));
							bailIfError(KprUPnPStorageWrite(storage, "\"", 0));
						}
					}
					bailIfError(KprUPnPStorageWrite(storage, " val=\"", 0));
					bailIfError(KprUPnPStorageWriteEntity(storage, current->value, 1));
					bailIfError(KprUPnPStorageWrite(storage, "\"/>", 0));
				}
				empty = false;
			}
			current = current->next;
		}
		iterate = FskAssociativeArrayIteratorNext(iterate);
	}
	FskAssociativeArrayIteratorDispose(iterate);

	bailIfError(KprUPnPStorageWrite(storage, "</InstanceID>", 0));
	bailIfError(KprUPnPStorageWrite(storage, "</Event>", 0));
	FskMemPtrDisposeAt(&lastChange->value);
	if (!empty) {
		UInt32 size;
		char* data = NULL;

		FskGrowableStorageGetPointerToItem(storage, 0, (void **)&data);
		size = FskGrowableStorageGetSize(storage);
		bailIfError(FskMemPtrNewClear(size + 1, &lastChange->value));
		FskStrNCopy(lastChange->value, data, size);
		FskInstrumentedItemPrintfDebug(self, "LastChange:\n%s", lastChange->value);
	}
bail:
	FskGrowableStorageDispose(storage);
	return err;
}

Boolean KprUPnPSubscriptionEventBody(KprUPnPSubscription self)
{
	FskErr err = kFskErrNone;
	Boolean empty = true;
	FskGrowableStorage storage = self->eventBody;
	FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(self->service->stateVariables);
	KprUPnPStringData current = self->values;

	bailIfError(FskGrowableStorageSetSize(storage, 0));
	bailIfError(KprUPnPStorageWrite(storage, "<?xml version=\"1.0\"?>\n", 0));
	bailIfError(KprUPnPStorageWrite(storage, "<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\">\n", 0));
	while (iterate) {
		KprUPnPStateVariable variable = (KprUPnPStateVariable)iterate->value;
		if (variable->sendEvents) {
			if (!FskStrCompare(iterate->name, "LastChange")) {
				bailIfError(KprUPnPSubscriptionEventLastChange(self, current));
			}
			else if (FskStrCompare(current->value, variable->value)) {
				FskMemPtrDispose(current->value);
				current->value = FskStrDoCopy(variable->value);
			}
			if (current->value) {
				char* name = iterate->name;
				bailIfError(KprUPnPStorageWrite(storage, "\t<e:property>\n", 0));
				bailIfError(KprUPnPStorageWrite(storage, "\t\t<", 0));
				bailIfError(KprUPnPStorageWriteEntity(storage, name, 1));
				bailIfError(KprUPnPStorageWrite(storage, ">", 0));
				bailIfError(KprUPnPStorageWriteEntity(storage, current->value, 2));
				bailIfError(KprUPnPStorageWrite(storage, "</", 0));
				bailIfError(KprUPnPStorageWriteEntity(storage, name, 1));
				bailIfError(KprUPnPStorageWrite(storage, ">\n", 0));
				bailIfError(KprUPnPStorageWrite(storage, "\t</e:property>\n", 0));
				empty = false;
			}
			current = current->next;
		}
		iterate = FskAssociativeArrayIteratorNext(iterate);
	}
	bailIfError(KprUPnPStorageWrite(storage, "</e:propertyset>", 0));
	if (empty) {
		BAIL(kFskErrNotFound);
	}
bail:
	FskAssociativeArrayIteratorDispose(iterate);
	return err ? false : !empty;
}

void KprUPnPSubscriptionEventCallback(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *param)
{
	KprUPnPSubscription self = param;

	FskInstrumentedItemPrintfDebug(self, "Event temporization %s", self->url);
	self->eventModeration = false;
	KprUPnPSubscriptionEvent(self, false);
}

void KprUPnPSubscriptionEventComplete(KprMessage message, void* it)
{
	FskErr err = kFskErrNone;
	KprUPnPSubscription self = it;
	FskTimeRecord now;

	FskInstrumentedItemPrintfVerbose(self, "Event completed %s (%d)", self->url, message->status);
	self->event = NULL;
	self->eventModeration = true;
	self->notified = true;
	if (self->removed) {
		KprUPnPSubscriptionDispose(self);
	}
	else if (message->status == 0) {
		bailIfError(KprUPnPServiceRemoveSubscription(self->service, self->uuid));
	}
	else {
		FskTimeGetNow(&now);
		FskTimeAddMS(&now, gUPnP->upnpEventModeration);
		FskTimeCallbackSet(self->eventTimer, &now, KprUPnPSubscriptionEventCallback, self);
	}
bail:
	return;
}

FskErr KprUPnPSubscriptionEvent(KprUPnPSubscription self, Boolean changed)
{
	FskErr err = kFskErrNone;
	KprMessage message = NULL;

	self->eventPending |= changed;
	if (self->event || self->eventModeration || !self->eventPending || !self->eventBody)
		return err;
	else {
		self->eventPending = false;

		if (KprUPnPSubscriptionEventBody(self)) {
			UInt32 size;
			char* data = NULL;
			char stringSize[16];
			
			FskGrowableStorageGetPointerToItem(self->eventBody, 0, (void **)&data);
			size = FskGrowableStorageGetSize(self->eventBody);

			// something to notify
			FskInstrumentedItemPrintfNormal(self, "Event to %s", self->url);
			FskInstrumentedItemPrintfDebug(self, "%.*s\n", size, data);

			bailIfError(KprMessageNew(&message, self->url));
			KprMessageSetTimeout(message, gUPnP->httpTimeout);
			bailIfError(KprMessageSetMethod(message, "NOTIFY"));
			bailIfError(KprMessageSetRequestHeader(message, kFskStrContentType, kFskStrTextXMLCharset));
			bailIfError(KprMessageSetRequestHeader(message, kFskStrUserAgent, gUPnP->userAgent));
			bailIfError(KprMessageSetRequestHeader(message, "NT", "upnp:event"));
			bailIfError(KprMessageSetRequestHeader(message, "NTS", "upnp:propchange"));
			bailIfError(KprMessageSetRequestHeader(message, "SID", self->uuid));
			FskStrNumToStr(self->key++, stringSize, 16);
			bailIfError(KprMessageSetRequestHeader(message, "SEQ", stringSize));
			FskStrNumToStr(size, stringSize, 16);
			bailIfError(KprMessageSetRequestHeader(message, kFskStrContentLength, stringSize));
//			bailIfError(KprMessageSetRequestHeader(message, "CONNECTION", "close")); // DLNA CTT 7.3.93.2 but DLNA CTT Bugzilla ID 1324
			bailIfError(KprMessageSetRequestBody(message, data, size));
			KprMessageInvoke(message, KprUPnPSubscriptionEventComplete, NULL, self);
			self->event = message;		
		}
	}
bail:
	if (err)
		KprMessageDispose(message);
	return err;
}

void KprUPnPSubscriptionExpire(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *param)
{
	FskErr err = kFskErrNone;
	KprUPnPSubscription self = param;
	FskInstrumentedItemPrintfNormal(self, "Expire %s (%s)", self->eventSubURL, self->uuid);
	bailIfError(KprUPnPServiceRemoveSubscription(self->service, self->uuid));
bail:
	return;
}

void KprUPnPSubscriptionRenew(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *param)
{
	FskErr err = kFskErrNone;
	KprUPnPSubscription self = param;
	KprMessage message = NULL;
	char timeout[32];

	FskInstrumentedItemPrintfNormal(self, "Renew %s (%s)", self->eventSubURL, self->uuid);
	bailIfError(KprMessageNew(&message, self->eventSubURL));
	KprMessageSetTimeout(message, gUPnP->httpTimeout);
	bailIfError(KprMessageSetMethod(message, "SUBSCRIBE"));
	bailIfError(KprMessageSetRequestHeader(message, "SID", self->uuid));
	snprintf(timeout, sizeof(timeout), "Second-%lu", self->duration);
	bailIfError(KprMessageSetRequestHeader(message, "TIMEOUT", timeout));
	KprMessageInvoke(message, KprUPnPSubscriptionRenewMessageComplete, KprUPnPDefaultMessageDispose, FskStrDoCopy(self->uuid));
bail:
	if (err)
		KprMessageDispose(message);
	return;
}

static void KprUPnPSubscriptionRenewMessageComplete(KprMessage message, void* it)
{
	FskErr err = kFskErrNone;
	char* uuid = it;
	char* sid;
	char* timeout;
	KprUPnPSubscription self;
	
	sid = KprMessageGetRequestHeader(message, "SID");
	if (!FskStrCompare(sid, uuid)) {
		bailIfNULL(self = FskAssociativeArrayElementGetReference(gUPnP->subscriptions, sid));
		timeout = KprMessageGetResponseHeader(message, "TIMEOUT");
		if (!FskStrCompareCaseInsensitiveWithLength(timeout, "Second-", 7)) {
			self->duration = FskStrToNum(timeout + 7);
			FskTimeCallbackScheduleFuture(self->timer, (self->duration * 90) / 100, 0, KprUPnPSubscriptionRenew, self);
			FskInstrumentedItemPrintfNormal(self, "Renwed %s (%s) for %d seconds", self->eventSubURL, self->uuid, self->duration);
		}
	}
bail:
	return;
}

FskErr KprUPnPSubscriptionSubscribe(KprUPnPSubscription self, KprUPnPController controller)
{
	FskErr err = kFskErrNone;
	KprMessage message = NULL;
	char* callback = NULL;
	char timeout[32];
	UInt32 size = FskStrLen(self->url) + 3;
	FskUUIDRecord uuidRecord;
	char* uuid;

	FskInstrumentedItemPrintfNormal(self, "Subscribe %s %s to %s", self->service->device->friendlyName, self->service->name, self->eventSubURL);
	bailIfError(KprMessageNew(&message, self->eventSubURL));
	KprMessageSetTimeout(message, gUPnP->httpTimeout);
	bailIfError(KprMessageSetMethod(message, "SUBSCRIBE"));
	bailIfError(KprMessageSetRequestHeader(message, "NT", "upnp:event"));
	bailIfError(FskMemPtrNewClear(size, &callback));
	snprintf(callback, size, "<%s>", self->url);
	bailIfError(KprMessageSetRequestHeader(message, "CALLBACK", callback));
	snprintf(timeout, sizeof(timeout), "Second-%lu", self->duration);
	bailIfError(KprMessageSetRequestHeader(message, "TIMEOUT", timeout));
	bailIfError(FskUUIDCreate(&uuidRecord));
	uuid = FskUUIDtoString(&uuidRecord);
	FskAssociativeArrayElementSetReference(gUPnP->subscriptions, uuid, self);
	KprMessageInvoke(message, KprUPnPSubscriptionSubscribeMessageComplete, KprUPnPDefaultMessageDispose, uuid);
	self->controller = controller;
bail:
	if (err)
		KprMessageDispose(message);
	FskMemPtrDispose(callback);
	return err;
}

void KprUPnPSubscriptionSubscribeMessageComplete(KprMessage message, void* it)
{
	FskErr err = kFskErrNone;
	KprUPnPSubscription self = NULL;
	char* uuid = it;
	char* timeout = NULL;
	char* sid;
	
	bailIfNULL(self = FskAssociativeArrayElementGetReference(gUPnP->subscriptions, uuid));
	FskAssociativeArrayElementDispose(gUPnP->subscriptions, uuid);
	if (message->status / 100 != 2) {
		BAIL(kFskErrInvalidParameter);
	}
	sid = KprMessageGetResponseHeader(message, "SID");
	bailIfNULL(self->uuid = FskStrDoCopy(sid));
	timeout = KprMessageGetResponseHeader(message, "TIMEOUT");
	if (!FskStrCompareCaseInsensitiveWithLength(timeout, "Second-", 7)) {
		self->duration = FskStrToNum(timeout + 7);
		FskInstrumentedItemPrintfNormal(self, "Subscribed to %s (%s) for %d seconds", self->eventSubURL, sid, self->duration);
		FskAssociativeArrayElementSetReference(gUPnP->subscriptions, sid, self);
		if (self->removed) {
			bailIfError(KprUPnPSubscriptionUnsubscribe(self, self->controller));
		}
		else {
			FskTimeCallbackScheduleFuture(self->timer, (self->duration * 90) / 100, 0, KprUPnPSubscriptionRenew, self);
		}
	}
bail:
	return;
}

FskErr KprUPnPSubscriptionUnsubscribe(KprUPnPSubscription self, KprUPnPController controller)
{
	FskErr err = kFskErrNone;
	KprMessage message = NULL;

	if (self->uuid) {
		FskInstrumentedItemPrintfNormal(self, "Unsubscribe from %s (%s)", self->eventSubURL, self->uuid);
		bailIfError(KprMessageNew(&message, self->eventSubURL));
		KprMessageSetTimeout(message, gUPnP->httpTimeout);
		bailIfError(KprMessageSetMethod(message, "UNSUBSCRIBE"));
		bailIfError(KprMessageSetRequestHeader(message, "SID", self->uuid));
		KprMessageInvoke(message, KprUPnPSubscriptionUnsubscribeMessageComplete, KprUPnPDefaultMessageDispose, FskStrDoCopy(self->uuid));
	}
	else {
		self->removed = true;
	}
bail:
	if (err) {
		KprMessageDispose(message);
	}
	return err;
}

void KprUPnPSubscriptionUnsubscribeMessageComplete(KprMessage message, void* it)
{
	FskErr err = kFskErrNone;
	char* uuid = it;
	char* sid;
	KprUPnPSubscription self;
	
	sid = KprMessageGetRequestHeader(message, "SID");
	if (!FskStrCompare(sid, uuid)) {
		bailIfNULL(self = FskAssociativeArrayElementGetReference(gUPnP->subscriptions, sid));
		FskInstrumentedItemPrintfNormal(self, "Unsubscribed from %s (%s)", self->eventSubURL, sid);
		FskAssociativeArrayElementDispose(gUPnP->subscriptions, uuid);
		KprUPnPSubscriptionDispose(self);
	}
bail:
	return;
}

//--------------------------------------------------
// UPnP Action
//--------------------------------------------------
#if 0
#pragma mark - KprUPnPAction
#endif

FskErr KprUPnPActionNew(KprUPnPAction* it)
{
	FskErr err = kFskErrNone;
	KprUPnPAction self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprUPnPActionRecord), &self));
	*it = self;
	FskInstrumentedItemNew(self, NULL, &KprUPnPActionInstrumentation);
bail:
	return err;
}

void KprUPnPActionDispose(KprUPnPAction self)
{
	if (self) {
		KprUPnPArgument argument, next;
		for (argument = self->argumentIn; argument; argument = next) {
			next = argument->next;
			KprUPnPArgumentDispose(argument);
		}
		self->argumentIn = NULL;
		for (argument = self->argumentOut; argument; argument = next) {
			next = argument->next;
			KprUPnPArgumentDispose(argument);
		}
		self->argumentOut = NULL;
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self->name);
		FskMemPtrDispose(self);
	}
}

FskErr KprUPnPActionFromElement(KprXMLElement action, KprUPnPService service)
{
	FskErr err = kFskErrNone;
	KprUPnPAction self = NULL;
	char* name = KprXMLElementGetProperty(action, kUPnPServiceNameSpace, "name");
	KprXMLElement argumentList = KprXMLElementGetFirstElement(action, kUPnPServiceNameSpace, "argumentList");
	KprXMLElement argument;
	KprUPnPArgument argumentOut;
	bailIfError(KprUPnPActionNew(&self));
	self->service = service;
	if (argumentList) {
		for (argument = KprXMLElementGetFirstElement(argumentList, kUPnPServiceNameSpace, "argument"); argument; argument = KprXMLElementGetNextElement(argument, kUPnPServiceNameSpace, "argument")) {
			bailIfError(KprUPnPArgumentFromElement(argument, self));
		}
		if (self->argumentOut) {
			UInt32 count = 0;
			for (argumentOut = self->argumentOut->next; argumentOut; argumentOut = argumentOut->next) {
				if (argumentOut->returnValue) count++;
			}
			if (count > 0) { // only first argument out can be the return value
				BAIL(kFskErrBadData);
			}
		}
	}
	self->name = FskStrDoCopy(name);
	bailIfNULL(self->name);
	FskAssociativeArrayElementSetReference(service->actions, name, self);
	return err;
bail:
	KprUPnPActionDispose(self);
	return err;
}

//--------------------------------------------------
// UPnP Argument
//--------------------------------------------------
#if 0
#pragma mark - KprUPnPArgument
#endif

FskErr KprUPnPArgumentNew(KprUPnPArgument* it)
{
	FskErr err = kFskErrNone;
	KprUPnPArgument self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprUPnPArgumentRecord), &self));
	*it = self;
	FskInstrumentedItemNew(self, NULL, &KprUPnPArgumentInstrumentation);
bail:
	return err;
}

void KprUPnPArgumentDispose(KprUPnPArgument self)
{
	if (self) {
		FskMemPtrDispose(self->name);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprUPnPArgumentFromElement(KprXMLElement argument, KprUPnPAction action)
{
	FskErr err = kFskErrNone;
	KprUPnPArgument self = NULL;
	char* name = KprXMLElementGetProperty(argument, kUPnPServiceNameSpace, "name");
	char* direction = KprXMLElementGetProperty(argument, kUPnPServiceNameSpace, "direction");
	KprXMLElement retval = KprXMLElementGetFirstElement(argument, kUPnPServiceNameSpace, "retval");
	char* relatedStateVariable = KprXMLElementGetProperty(argument, kUPnPServiceNameSpace, "relatedStateVariable");
	KprUPnPStateVariable variable = NULL;
	if (relatedStateVariable)
		variable = FskAssociativeArrayElementGetReference(action->service->stateVariables, relatedStateVariable);
	if (!name || !direction || !variable) {
		BAIL(kFskErrBadData);
	}
	
	bailIfError(KprUPnPArgumentNew(&self));
	self->name = FskStrDoCopy(name);
	self->relatedStateVariable = variable;
	if (!FskStrCompare(direction, "in")) {
		if (retval) {
			BAIL(kFskErrBadData);
		}
		FskListAppend(&action->argumentIn, self);
	}
	else if (!FskStrCompare(direction, "out")) {
		FskListAppend(&action->argumentOut, self);
		if (retval) {
			self->returnValue = true;
		}
	}
	else {
		BAIL(kFskErrBadData);
	}
	return err;
bail:
	KprUPnPArgumentDispose(self);
	return err;
}

//--------------------------------------------------
// UPnP State Variable
//--------------------------------------------------
#if 0
#pragma mark - KprUPnPStateVariable
#endif

FskErr KprUPnPStateVariableStringToInt64(char* str, FskInt64* value) {
	FskErr err = kFskErrNone;
	int len, i, neg = 0, n;
	FskInt64 accum = 0;
	char *start;

	start = FskStrStripHeadSpace(str);
	if (start[0] == '-') {
		neg = 1;
		start++;
	}
	else if (start[0] == '+')
		start++;
	len = FskStrLen(start);
	for (i = 0; i < len; i++) {
		n = start[i] - '0';
		if ((n < 0) || (n > 9))
			err = kUPnPErrInvalidArgs;
		accum *= 10;
		accum += n;
	}

	if (neg)
		*value = -accum;
	else
		*value = accum;
	return err;
}

// UPnP State Variable Default value

void KprUPnPStateVariableDefaultDispose(KprUPnPStateVariable self)
{
	FskMemPtrDisposeAt(&self->data);
	FskMemPtrDisposeAt(&self->value);
}

FskErr KprUPnPStateVariableDefaultFromElement(KprUPnPStateVariable self UNUSED, KprXMLElement variable)
{
	FskErr err = kFskErrNone;
	KprXMLElement allowedValueList = KprXMLElementGetFirstElement(variable, kUPnPServiceNameSpace, "allowedValueList");
	KprXMLElement allowedValueRange = KprXMLElementGetFirstElement(variable, kUPnPServiceNameSpace, "allowedValueRange");
	if (allowedValueList || allowedValueRange) {
		BAIL(kFskErrBadData);
	}
bail:
	return err;
}

FskErr KprUPnPStateVariableDefaultFromString(KprUPnPStateVariable self, char* string)
{
	FskErr err = kFskErrNone;
	FskMemPtrDispose(self->value);
	self->value = FskStrDoCopy(string);
	bailIfNULL(self->value);
bail:
	if (err == kFskErrMemFull) err = kUPnPErrOutOfMemory;
	return err;
}

void KprUPnPStateVariableDefaultToXS(xsMachine *the, KprUPnPStateVariable self)
{
    if (self->value)
        xsResult = xsString(self->value);
}

// UPnP State Variable Boolean value

void KprUPnPStateVariableBooleanDispose(KprUPnPStateVariable self UNUSED)
{
}

FskErr KprUPnPStateVariableBooleanFromElement(KprUPnPStateVariable self UNUSED, KprXMLElement variable)
{
	FskErr err = kFskErrNone;
	KprXMLElement allowedValueList = KprXMLElementGetFirstElement(variable, kUPnPServiceNameSpace, "allowedValueList");
	KprXMLElement allowedValueRange = KprXMLElementGetFirstElement(variable, kUPnPServiceNameSpace, "allowedValueRange");
	if (allowedValueList || allowedValueRange) {
		BAIL(kFskErrBadData);
	}
bail:
	return err;
}

FskErr KprUPnPStateVariableBooleanFromString(KprUPnPStateVariable self, char* string)
{
	FskErr err = kFskErrNone;
	UInt32 value;
	if (!FskStrCompareCaseInsensitive(string, "1")
		|| !FskStrCompareCaseInsensitive(string, "YES")
		|| !FskStrCompareCaseInsensitive(string, "TRUE"))
		value = true;
	else if (!FskStrCompareCaseInsensitive(string, "0")
		|| !FskStrCompareCaseInsensitive(string, "NO")
		|| !FskStrCompareCaseInsensitive(string, "FALSE"))
		value = false;
	else {
		BAIL(kUPnPErrInvalidArgs);
	}
	self->data = (void*)value;
	if (value)
		self->value = "1";
	else
		self->value = "0";
bail:
	return err;
}

void KprUPnPStateVariableBooleanToXS(xsMachine *the, KprUPnPStateVariable self)
{
	xsResult = (self->data) ? xsTrue : xsFalse;
}

// UPnP State Variable SInt value

FskErr KprUPnPStateVariableSIntFromElement(KprUPnPStateVariable self, KprXMLElement variable)
{
	FskErr err = kFskErrNone;
	KprUPnPSIntData data = NULL;
	char* dataType = KprXMLElementGetProperty(variable, kUPnPServiceNameSpace, "dataType");
	KprXMLElement allowedValueList = KprXMLElementGetFirstElement(variable, kUPnPServiceNameSpace, "allowedValueList");
	KprXMLElement allowedValueRange = KprXMLElementGetFirstElement(variable, kUPnPServiceNameSpace, "allowedValueRange");
	if (allowedValueList) {
		BAIL(kFskErrBadData);
	}
	bailIfError(FskMemPtrNewClear(sizeof(KprUPnPSIntDataRecord), &data));
	self->data = data;
	if (!FskStrCompare(dataType, "i1")) {
		data->minimum = SCHAR_MIN;
		data->maximum = SCHAR_MAX;
		data->step = 1;
	}
	else if (!FskStrCompare(dataType, "i2")) {
		data->minimum = INT_MIN;
		data->maximum = INT_MAX;
		data->step = 1;
	}
	else if (!FskStrCompare(dataType, "i4")) {
		data->minimum = kFskSInt32Min;
		data->maximum = kFskSInt32Max;
		data->step = 1;
	}
	else if (!FskStrCompare(dataType, "int")) {
		data->minimum = kFskSInt32Min;
		data->maximum = kFskSInt32Max;
		data->step = 1;
	}
	if (allowedValueRange) {
		char* minimum = KprXMLElementGetProperty(allowedValueRange, kUPnPServiceNameSpace, "minimum");
		char* maximum = KprXMLElementGetProperty(allowedValueRange, kUPnPServiceNameSpace, "maximum");
		char* step = KprXMLElementGetProperty(allowedValueRange, kUPnPServiceNameSpace, "step");
		FskInt64 limit;
		if (minimum) {
			limit = FskStrToFskInt64(minimum);
			if ((limit < data->minimum) || (limit > data->maximum)) {
				BAIL(kFskErrBadData);
			}
			data->minimum = limit;
		}
		if (maximum) {
			limit = FskStrToFskInt64(maximum);
			if ((limit < data->minimum) || (limit > data->maximum)) {
				BAIL(kFskErrBadData);
			}
			data->step = (UInt32)limit;
		}
		if (step) {
			UInt32 n;
			data->step = FskStrToNum(step);
			if (!data->step) {
				BAIL(kFskErrBadData);
			}
			n = (data->maximum - data->minimum) / data->step;
			if (data->maximum != data->minimum + (SInt32)(n * data->step)) {
				BAIL(kFskErrBadData);
			}
		}
	}
	return err;
bail:
	(*self->dispose)(self);
	return err;
}

FskErr KprUPnPStateVariableSIntFromString(KprUPnPStateVariable self, char* string)
{
	FskErr err = kFskErrNone;
	KprUPnPSIntData data = self->data;
	FskInt64 value;
	bailIfError(KprUPnPStateVariableStringToInt64(string, &value));
	if ((value < data->minimum) || (value > data->maximum)) {
		bailIfError(kUPnPErrArgumentValueOutOfRange);
	}
	data->value = value;
	FskMemPtrDisposeAt(&self->value);
	self->value = FskStrDoCopy(string);
	bailIfNULL(self->value);
bail:
	if (err == kFskErrMemFull) err = kUPnPErrOutOfMemory;
	return err;
}

void KprUPnPStateVariableSIntToXS(xsMachine *the, KprUPnPStateVariable self)
{
	KprUPnPSIntData data = self->data;
	xsResult = xsInteger(data->value);
}

// UPnP State Variable UInt value

FskErr KprUPnPStateVariableUIntFromElement(KprUPnPStateVariable self, KprXMLElement variable)
{
	FskErr err = kFskErrNone;
	KprUPnPUIntData data = NULL;
	char* dataType = KprXMLElementGetProperty(variable, kUPnPServiceNameSpace, "dataType");
	KprXMLElement allowedValueList = KprXMLElementGetFirstElement(variable, kUPnPServiceNameSpace, "allowedValueList");
	KprXMLElement allowedValueRange = KprXMLElementGetFirstElement(variable, kUPnPServiceNameSpace, "allowedValueRange");
	if (allowedValueList) {
		BAIL(kFskErrBadData);
	}
	bailIfError(FskMemPtrNewClear(sizeof(KprUPnPUIntDataRecord), &data));
	self->data = data;
	if (!FskStrCompare(dataType, "ui1")) {
		data->minimum = 0;
		data->maximum = UCHAR_MAX;
		data->step = 1;
	}
	else if (!FskStrCompare(dataType, "ui2")) {
		data->minimum = 0;
		data->maximum = UINT_MAX;
		data->step = 1;
	}
	else if (!FskStrCompare(dataType, "ui4")) {
		data->minimum = 0;
		data->maximum = kFskUInt32Max;
		data->step = 1;
	}
	if (allowedValueRange) {
		char* minimum = KprXMLElementGetProperty(allowedValueRange, kUPnPServiceNameSpace, "minimum");
		char* maximum = KprXMLElementGetProperty(allowedValueRange, kUPnPServiceNameSpace, "maximum");
		char* step = KprXMLElementGetProperty(allowedValueRange, kUPnPServiceNameSpace, "step");
		FskInt64 limit;
		if (minimum) {
			limit = FskStrToFskInt64(minimum);
			if ((limit < data->minimum) || (limit > data->maximum)) {
				BAIL(kFskErrBadData);
			}
			data->minimum = limit;
		}
		if (maximum) {
			limit = FskStrToFskInt64(maximum);
			if ((limit < data->minimum) || (limit > data->maximum)) {
				BAIL(kFskErrBadData);
			}
			data->maximum = limit;
		}
		if (step) {
			UInt32 n;
			data->step = FskStrToNum(step);
			if (!data->step) {
				BAIL(kFskErrBadData);
			}
			n = (data->maximum - data->minimum) / data->step;
			if (data->maximum != data->minimum + (n * data->step)) {
				BAIL(kFskErrBadData);
			}
		}
	}
	return err;
bail:
	(*self->dispose)(self);
	return err;
}

FskErr KprUPnPStateVariableUIntFromString(KprUPnPStateVariable self, char* string)
{
	FskErr err = kFskErrNone;
	KprUPnPUIntData data = self->data;
	FskInt64 value;
	bailIfError(KprUPnPStateVariableStringToInt64(string, &value));
	if ((value < data->minimum) || (value > data->maximum)) {
		bailIfError(kUPnPErrArgumentValueOutOfRange);
	}
	data->value = value;
	FskMemPtrDisposeAt(&self->value);
	self->value = FskStrDoCopy(string);
	bailIfNULL(self->value);
bail:
	if (err == kFskErrMemFull) err = kUPnPErrOutOfMemory;
	return err;
}

void KprUPnPStateVariableUIntToXS(xsMachine *the, KprUPnPStateVariable self)
{
	KprUPnPUIntData data = self->data;
	xsResult = xsInteger(data->value);
}

// UPnP State Variable Char value

FskErr KprUPnPStateVariableCharFromString(KprUPnPStateVariable self, char* string)
{
	FskErr err = kFskErrNone;
	if (FskUnicodeStrLen((const UInt16 *)string) != 1) {
		BAIL(kFskErrBadData);
	}
	FskMemPtrDispose(self->value);
	self->value = FskStrDoCopy(string);
	bailIfNULL(self->value);
bail:
	if (err == kFskErrMemFull) err = kUPnPErrOutOfMemory;
	return err;
}

// UPnP State Variable String value

void KprUPnPStateVariableStringDispose(KprUPnPStateVariable self)
{
	KprUPnPStringData data, next;
	for (data = self->data; data; data = next) {
		next = data->next;
		FskMemPtrDispose(data->value);
		FskMemPtrDispose(data);
	}
	FskMemPtrDispose(self->value);
}

FskErr KprUPnPStateVariableFromStringElement(KprUPnPStateVariable self UNUSED, KprXMLElement variable)
{
	FskErr err = kFskErrNone;
	KprXMLElement allowedValueList = KprXMLElementGetFirstElement(variable, kUPnPServiceNameSpace, "allowedValueList");
	KprXMLElement allowedValueRange = KprXMLElementGetFirstElement(variable, kUPnPServiceNameSpace, "allowedValueRange");
	KprUPnPStringData data = NULL;
	if (allowedValueRange) {
		BAIL(kFskErrBadData);
	}
	if (allowedValueList) {
		char* value;
		KprXMLElement element;
		for (element = allowedValueList->element; element; element = element->next) {
			if (KprXMLElementIsEqual(element, kUPnPServiceNameSpace, "allowedValue")) {
				bailIfError(FskMemPtrNewClear(sizeof(KprUPnPStringDataRecord), &data));
				value = element->element->value;
				data->value = FskStrDoCopy(value);
				FskListAppend(&self->data, data);
			}
		}
	}
	return err;
bail:
	FskMemPtrDispose(data);
	return err;
}

FskErr KprUPnPStateVariableStringFromString(KprUPnPStateVariable self, char* string)
{
	FskErr err = kFskErrNone;
	if (self->data) {
		KprUPnPStringData data;
		for (data = self->data; data; data = data->next) {
			if (!FskStrCompare(data->value, string)) break;
		}
		if (!data) {
			bailIfError(kUPnPErrNoSuchObject);
		}
	}
	FskMemPtrDispose(self->value);
	self->value = FskStrDoCopy(string);
	bailIfNULL(self->value);
bail:
	if (err == kFskErrMemFull) err = kUPnPErrOutOfMemory;
	return err;
}

// UPnP State Variable UUID value

FskErr KprUPnPStateVariableUUIDFromString(KprUPnPStateVariable self, char* string)
{
	FskErr err = kFskErrNone;
	if (FskStrLen(string) != 36) { //@@ could check more precisely if it is a UUID
		bailIfError(kUPnPErrInvalidArgs);
	}
	FskMemPtrDispose(self->value);
	self->value = FskStrDoCopy(string);
	bailIfNULL(self->value);
bail:
	if (err == kFskErrMemFull) err = kUPnPErrOutOfMemory;
	return err;
}

// UPnP State Variable URI value

FskErr KprUPnPStateVariableURIFromString(KprUPnPStateVariable self, char* string)
{
	FskErr err = kFskErrNone;
	KprURLPartsRecord parts;
	KprURLSplit(string, &parts);
	if (!parts.scheme) {
		BAIL(kFskErrBadData);
	}
	FskMemPtrDispose(self->value);
	self->value = FskStrDoCopy(string);
	bailIfNULL(self->value);
bail:
	if (err == kFskErrMemFull) err = kUPnPErrOutOfMemory;
	return err;
}

// UPnP State Variable

FskErr KprUPnPStateVariableNew(KprUPnPStateVariable* it)
{
	FskErr err = kFskErrNone;
	KprUPnPStateVariable self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprUPnPStateVariableRecord), &self));
	*it = self;
	FskInstrumentedItemNew(self, NULL, &KprUPnPStateVariableInstrumentation);
bail:
	return err;
}

void KprUPnPStateVariableDispose(KprUPnPStateVariable self)
{
	if (self) {
		if (self->dispose)
			(*self->dispose)(self);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprUPnPStateVariableFromElement(KprXMLElement variable, KprUPnPService service)
{
	FskErr err = kFskErrNone;
	KprUPnPStateVariable self = NULL;
	char* sendEvents = KprXMLElementGetAttribute(variable, "sendEvents");
	char* multicast = KprXMLElementGetProperty(variable, kUPnPServiceNameSpace, "multicast");
	char* name = KprXMLElementGetProperty(variable, kUPnPServiceNameSpace, "name");
	char* dataType = KprXMLElementGetProperty(variable, kUPnPServiceNameSpace, "dataType");
	char* defaultValue = KprXMLElementGetProperty(variable, kUPnPServiceNameSpace, "defaultValue");
	
	bailIfError(KprUPnPStateVariableNew(&self));
	self->service = service;
	// sendEvents
	if (!sendEvents || !FskStrCompare(sendEvents, "yes")) // default is true
		self->sendEvents = true;
	else if (!FskStrCompare(sendEvents, "no"))
		self->sendEvents = false;
	else {
		BAIL(kFskErrBadData);
	}
	// multicast
	if (!multicast || !FskStrCompare(multicast, "no")) // default is false
		self->multicast = false;
	else if (!FskStrCompare(multicast, "yes"))
		self->multicast = true;
	else {
		BAIL(kFskErrBadData);
	}
	 // defaults
	self->fromElement = KprUPnPStateVariableDefaultFromElement;
	self->fromString = KprUPnPStateVariableDefaultFromString;
	self->dispose = KprUPnPStateVariableDefaultDispose;
	self->toXS = KprUPnPStateVariableDefaultToXS;
	// dataType
	if (!FskStrCompare(dataType, "ui1")
		|| !FskStrCompare(dataType, "ui2")
		|| !FskStrCompare(dataType, "ui4")) {
		self->fromElement = KprUPnPStateVariableUIntFromElement;
		self->fromString = KprUPnPStateVariableUIntFromString;
		self->toXS = KprUPnPStateVariableUIntToXS;
	}
	else if (!FskStrCompare(dataType, "i1")
		|| !FskStrCompare(dataType, "i2")
		|| !FskStrCompare(dataType, "i4")
		|| !FskStrCompare(dataType, "int")) {
		self->fromElement = KprUPnPStateVariableSIntFromElement;
		self->fromString = KprUPnPStateVariableSIntFromString;
		self->toXS = KprUPnPStateVariableSIntToXS;
	}
	else if (!FskStrCompare(dataType, "r4") //@@ should verifiy format
		|| !FskStrCompare(dataType, "r8")
		|| !FskStrCompare(dataType, "number")
		|| !FskStrCompare(dataType, "float")) {
		self->fromString = KprUPnPStateVariableURIFromString;
	}
	else if (!FskStrCompare(dataType, "fixed")) { //@@ should verifiy format
		self->fromString = KprUPnPStateVariableURIFromString;
	}
	else if (!FskStrCompare(dataType, "char")) {
		self->fromString = KprUPnPStateVariableCharFromString;
	}
	else if (!FskStrCompare(dataType, "string")) {
		self->fromElement = KprUPnPStateVariableFromStringElement;
		self->fromString = KprUPnPStateVariableStringFromString;
		self->dispose = KprUPnPStateVariableStringDispose;
	}
	else if (!FskStrCompare(dataType, "date") //@@ should verifiy format
		|| !FskStrCompare(dataType, "dateTime")
		|| !FskStrCompare(dataType, "dateTime.tz")
		|| !FskStrCompare(dataType, "time")
		|| !FskStrCompare(dataType, "time.tz")) {
	}
	else if (!FskStrCompare(dataType, "boolean")) {
		self->fromElement = KprUPnPStateVariableBooleanFromElement;
		self->fromString = KprUPnPStateVariableBooleanFromString;
		self->dispose = KprUPnPStateVariableBooleanDispose;
		self->toXS = KprUPnPStateVariableBooleanToXS;
	}
	else if (!FskStrCompare(dataType, "bin.base64")) {
	}
	else if (!FskStrCompare(dataType, "bin.hex")) {
	}
	else if (!FskStrCompare(dataType, "uri")) {
		self->fromString = KprUPnPStateVariableURIFromString;
	}
	else if (!FskStrCompare(dataType, "uuid")) {
		self->fromString = KprUPnPStateVariableUUIDFromString;
	}
	else {
		BAIL(kFskErrBadData);
	}
	bailIfError((*self->fromElement)(self, variable));
	if (defaultValue)
		bailIfError(KprUPnPStateVariableSetValue(self, defaultValue));
	FskAssociativeArrayElementSetReference(service->stateVariables, name, self);
	return err;
bail:
	KprUPnPStateVariableDispose(self);
	return err;
}

char* KprUPnPStateVariableGetValue(KprUPnPStateVariable self)
{
	return self->value;
}

FskErr KprUPnPStateVariableSetValue(KprUPnPStateVariable self, char* value)
{
	FskErr err = kFskErrNone;
	self->changed = (FskStrCompare(self->value, value) != 0);
	if (self->changed) {
		bailIfError((*self->fromString)(self, value));
	}
bail:
	return err;
}

//--------------------------------------------------
// UPnP Handler
//--------------------------------------------------
#if 0
#pragma mark - KprUPnPHandlerBehavior
#endif

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprUPnPHandlerBehaviorInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUPnPHandlerBehavior", FskInstrumentationOffset(KprUPnPHandlerBehaviorRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

KprDelegateRecord kprUPnPHandlerBehaviorDelegateRecord = {
	KprDefaultBehaviorAccept,
	KprDefaultBehaviorActivated,
	KprDefaultBehaviorAdapt,
	KprUPnPHandlerBehaviorCancel,
	KprUPnPHandlerBehaviorComplete,
	KprDefaultBehaviorDisplayed,
	KprDefaultBehaviorDisplaying,
	KprUPnPHandlerBehaviorDispose,
	KprDefaultBehaviorFinished,
	KprDefaultBehaviorFocused,
	KprUPnPHandlerBehaviorInvoke,
	KprDefaultBehaviorKeyDown,
	KprDefaultBehaviorKeyUp,
	KprDefaultBehaviorLaunch,
	KprDefaultBehaviorLoaded,
	KprDefaultBehaviorLoading,
	KprDefaultBehaviorMark,
	KprDefaultBehaviorMeasureHorizontally,
	KprDefaultBehaviorMeasureVertically,
	KprDefaultBehaviorMetadataChanged,
	KprDefaultBehaviorQuit,
	KprDefaultBehaviorScrolled,
	KprDefaultBehaviorSensorBegan,
	KprDefaultBehaviorSensorChanged,
	KprDefaultBehaviorSensorEnded,
	KprDefaultBehaviorStateChanged,
	KprDefaultBehaviorTimeChanged,
	KprDefaultBehaviorTouchBegan,
	KprDefaultBehaviorTouchCancelled,
	KprDefaultBehaviorTouchEnded,
	KprDefaultBehaviorTouchMoved,
	KprDefaultBehaviorTransitionBeginning,
	KprDefaultBehaviorTransitionEnded,
	KprDefaultBehaviorUndisplayed,
	KprDefaultBehaviorUnfocused
};

FskErr KprUPnPHandlerBehaviorNew(KprBehavior* it, KprContent content, UInt32 command, KprUPnPDevice device, KprUPnPService service, KprUPnPIcon icon)
{
	FskErr err = kFskErrNone;
	KprUPnPHandlerBehavior self;
	bailIfError(FskMemPtrNewClear(sizeof(kprUPnPHandlerBehaviorDelegateRecord), it));
	self = *((KprUPnPHandlerBehavior*)it);
	FskInstrumentedItemNew(self, NULL, &KprUPnPHandlerBehaviorInstrumentation);
	FskInstrumentedItemSetOwner(self, content);
	self->delegate = &kprUPnPHandlerBehaviorDelegateRecord;
	self->command = command;
	self->device = device;
	self->service = service;
	self->icon = icon;
bail:
	return err;
}

void KprUPnPHandlerBehaviorDispose(void* it)
{
	KprContent content = it;
	KprUPnPHandlerBehavior self = (KprUPnPHandlerBehavior)content->behavior;
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

void KprUPnPHandlerBehaviorCancel(void* it UNUSED, KprMessage message UNUSED)
{
//	KprHandler handler = it;
//	KprContext context = (KprContext)handler->container;
//	KprUPnPHandlerBehavior behavior = (KprUPnPHandlerBehavior)handler->behavior;
}

void KprUPnPHandlerBehaviorComplete(void* it, KprMessage action)
{
	KprHandler handler = it;
	KprMessage message = handler->message;
	KprContext context = (KprContext)handler->container;
	KprUPnPHandlerBehavior behavior = (KprUPnPHandlerBehavior)handler->behavior;
	
	message->response.headers = action->response.headers;
	action->response.headers = NULL;

	message->response.body = action->response.body;
	message->response.size = action->response.size;
	action->response.body = NULL;
	action->response.size = 0;
	
	message->timeout = action->timeout;
	
	KprUPnPServiceEvent(behavior->service);
	KprUPnPServiceActionResponse(behavior->service, context, handler, message);
}

FskErr KprUPnPHandlerBehaviorDoDescription(KprContext context UNUSED, KprHandler handler UNUSED, KprUPnPHandlerBehavior behavior, KprMessage message)
{
	FskErr err = kFskErrNone;
	char* data = NULL;
	UInt32 dataSize;
	char stringSize[16];

	if (behavior->service) {
		data = behavior->service->description;
		dataSize = behavior->service->descriptionSize;
	}
	else {
		data = behavior->device->description;
		dataSize = behavior->device->descriptionSize;
	}
	message->status = 200;
	if (!FskStrCompareCaseInsensitive(KprMessageGetMethod(message), "GET"))
		KprMessageSetResponseBody(message, data, dataSize);
	else if (FskStrCompareCaseInsensitive(KprMessageGetMethod(message), "HEAD"))
		message->status = 405;
	if (message->status == 200) {
		FskStrNumToStr(dataSize, stringSize, 16);
		KprMessageSetResponseHeader(message, kFskStrContentLength, stringSize);
		KprMessageSetResponseHeader(message, kFskStrContentType, kFskStrTextXMLCharset);
	}
	if (KprMessageGetRequestHeader(message, kprHTTPAcceptLanguage))
		KprMessageSetResponseHeader(message, kprHTTPHeaderContentLanguage, "en");
//bail:
	return err;
}

FskErr KprUPnPHandlerBehaviorDoControl(KprContext context, KprHandler handler, KprUPnPHandlerBehavior behavior, KprMessage message)
{
	FskErr err = kFskErrNone;
	if (!FskStrCompareCaseInsensitive(KprMessageGetMethod(message), "POST")) {
		bailIfError(KprUPnPServiceActionInvoke(behavior->service, context, handler, message));
	}
	else
		message->status = 405;
bail:
	return err;
}

FskErr KprUPnPHandlerBehaviorDoSubscription(KprContext context UNUSED, KprHandler handler UNUSED, KprUPnPHandlerBehavior behavior, KprMessage message)
{
	FskErr err = kFskErrNone;
	char* host = KprMessageGetRequestHeader(message, kFskStrHost);
	char* callback = KprMessageGetRequestHeader(message, "CALLBACK");
	char* nt = KprMessageGetRequestHeader(message, "NT");
	char* sid = KprMessageGetRequestHeader(message, "SID");
	char* timeout = KprMessageGetRequestHeader(message, "TIMEOUT");
	KprUPnP self = gUPnP;
	UInt32 duration = self->subscriptionTimeout;
	UInt32 size;
	char string[256];

	size = FskStrLen(callback);
	message->status = 200;
	if (!FskStrCompareCaseInsensitive(KprMessageGetMethod(message), "SUBSCRIBE")) {
		if (timeout) {
			if (!FskStrCompareCaseInsensitiveWithLength(timeout, "Second-", 7)) {
				UInt32 value = (UInt32)FskStrToNum(timeout + 7);
				if (value > duration)
					duration = value;
			}
		}
		if (host && callback && nt && !sid) {
			KprURLPartsRecord parts;
			KprURLSplit(callback, &parts);
			if (FskStrCompare(nt, "upnp:event") || !parts.scheme)
				message->status = 412;
			if ((callback[0] != '<') && (callback[size - 1] != '>'))
				message->status = 412;
			else {
				callback[size - 1] = 0;
				bailIfError(KprUPnPServiceAddSubscription(behavior->service, callback + 1, duration, &sid));
				callback[size - 1] = '>';
			}
		}
		else if (host && !callback && !nt && sid && timeout) {
			if (KprUPnPServiceRenewSubscription(behavior->service, sid, duration) == kFskErrInvalidParameter)
				message->status = 412;
		}
		else if (sid && (nt || callback))
			message->status = 400;
		else
			message->status = 412;
		if (message->status == 200) {
			KprMessageSetResponseHeader(message, kFskStrServer, gUPnP->userAgent);
			KprMessageSetResponseHeader(message, "SID", sid);
			KprMessageSetResponseHeader(message, kFskStrContentLength, "0");
			snprintf(string, 256, "Second-%lu", duration);
			KprMessageSetResponseHeader(message, "TIMEOUT", string);
		}
	}
	else if (!FskStrCompareCaseInsensitive(KprMessageGetMethod(message), "UNSUBSCRIBE")) {
		if (host && !callback && !nt && sid && !timeout) {
			FskInstrumentedItemPrintfDebug(self, "Unsubscribe %s to %s", behavior->service->name, sid);
			if (KprUPnPServiceRemoveSubscription(behavior->service, sid) == kFskErrInvalidParameter)
				message->status = 412;
		}
		else if (sid && (nt || callback))
			message->status = 400;
		else
			message->status = 412;
	}
	else
		message->status = 405;
bail:
	return err;
}

FskErr KprUPnPHandlerBehaviorDoIcon(KprContext context UNUSED, KprHandler handler UNUSED, KprUPnPHandlerBehavior behavior, KprMessage message)
{
	FskErr err = kFskErrNone;
	FskInt64 size;
	unsigned char *data;
	FskFileMapping map = NULL;
	char stringSize[16];

	KprUPnPIcon icon = behavior->icon;
	bailIfError(FskFileMap(icon->path, &data, &size, 0, &map));
	FskStrNumToStr(size, stringSize, 16);
	bailIfError(KprMessageSetResponseBody(message, data, size));
	KprMessageSetResponseHeader(message, kFskStrContentType, icon->mime);
	KprMessageSetResponseHeader(message, kFskStrContentLength, stringSize);
bail:
	FskFileDisposeMap(map);
	return err;
}

FskErr KprUPnPHandlerBehaviorDoEvent(KprContext context UNUSED, KprHandler handler UNUSED, KprUPnPHandlerBehavior behavior, KprMessage message)
{
	FskErr err = kFskErrNone;
	FskAssociativeArray query = NULL;
	char* sid;
	char* serviceName;
	KprUPnPSubscription subscription = NULL;
	KprUPnPController controller = NULL;
	KprUPnPService service = NULL;
	KprXMLElement property, variable, propertySet = NULL, lastChange = NULL;
	
	sid = KprMessageGetRequestHeader(message, "sid");
	bailIfNULL(subscription = FskAssociativeArrayElementGetReference(gUPnP->subscriptions, sid));
	bailIfNULL(controller = subscription->controller);
	bailIfNULL(service = subscription->service);
	serviceName = service->name;
	bailIfError(KprXMLParse(&propertySet, message->request.body, message->request.size));
	for (property = KprXMLElementGetFirstElement(propertySet, kUPnPEventNameSpace, "property"); property; property = KprXMLElementGetNextElement(property, kUPnPEventNameSpace, "property")) {
		char* value = NULL;
		variable = property->element;
		value = KprXMLElementGetValue(variable);
		if (variable->name && value) {
			KprUPnPStateVariable stateVariable;
			char* name = variable->name;
			stateVariable = FskAssociativeArrayElementGetReference(service->stateVariables, name);
			if (stateVariable) {
				bailIfError(KprUPnPStateVariableSetValue(stateVariable, value));

				FskInstrumentedItemPrintfNormal(controller, "Event %s %s:%s:%s", controller->device->uuid, controller->device->name, serviceName, name);
				// Lets the application deal with the LastChange variable
				if (!FskStrCompare(name, "LastChange")) {
					KprXMLElement instance;
					FskInstrumentedItemPrintfDebug(controller, "%.*s", FskStrLen(value), value);
					bailIfError(KprXMLParse(&lastChange, (unsigned char *)value, (SInt32)FskStrLen(value)));
					instance = lastChange->element;
					if (instance && !FskStrCompare(instance->name, "InstanceID") && !FskStrCompareCaseInsensitiveWithLength(instance->nameSpace->value, kUPnPMetadataNameSpace, FskStrLen(kUPnPMetadataNameSpace))) {
						for (variable = instance->element; variable; variable = variable->next) {
							char* val = KprXMLElementGetAttribute(variable, "val");
							if (variable->name && val) {
								stateVariable = FskAssociativeArrayElementGetReference(service->stateVariables, variable->name);
								if (stateVariable) {
									FskInstrumentedItemPrintfVerbose(controller, "+ %s:%s <= %s", service->name, variable->name, val);
									KprUPnPStateVariableSetValue(stateVariable, val); // do not check error
								}
							}
						}
					}
				}
				else {
					FskInstrumentedItemPrintfVerbose(controller, "+ %s:%s <= %s", service->name, variable->name, value);
				}
				KprUPnPControllerServiceEventCallback(controller, serviceName, name, value);
			}
		}
	}
	KprUPnPControllerPingSchedule(controller);
bail:
	FskAssociativeArrayDispose(query);
	KprXMLElementDispose(lastChange);
	KprXMLElementDispose(propertySet);
	return err;
}

FskErr KprUPnPHandlerBehaviorDoPresentation(KprContext context UNUSED, KprHandler handler UNUSED, KprUPnPHandlerBehavior behavior, KprMessage message)
{
	FskErr err = kFskErrNone;
	return err;
}

void KprUPnPHandlerBehaviorInvoke(void* it, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprHandler handler = it;
	KprContext context = (KprContext)handler->container;
	KprUPnPHandlerBehavior behavior = (KprUPnPHandlerBehavior)handler->behavior;

	switch (behavior->command) {
		case kprUPnPMessageDescription:
			bailIfError(KprUPnPHandlerBehaviorDoDescription(context, handler, behavior, message));
			break;
		case kprUPnPMessageControl:
			bailIfError(KprUPnPHandlerBehaviorDoControl(context, handler, behavior, message));
			break;
		case kprUPnPMessageSubscription:
			bailIfError(KprUPnPHandlerBehaviorDoSubscription(context, handler, behavior, message));
			break;
		case kprUPnPMessageIcon:
			bailIfError(KprUPnPHandlerBehaviorDoIcon(context, handler, behavior, message));
			break;
		case kprUPnPMessageEvent:
			bailIfError(KprUPnPHandlerBehaviorDoEvent(context, handler, behavior, message));
			break;
		case kprUPnPMessagePresentation:
			bailIfError(KprUPnPHandlerBehaviorDoPresentation(context, handler, behavior, message));
			break;
		default:
			message->status = 404;
			break;
	}
bail:
	if (err)
		message->status = 500;
	return;
}

//--------------------------------------------------
// KprUPnPController
//--------------------------------------------------
#if 0
#pragma mark - KprUPnPController
#endif

FskErr KprUPnPControllerNew(KprUPnPController* it, char* authority, char* ip, char* uuid)
{
	FskErr err = kFskErrNone;
	KprUPnPController self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprUPnPControllerRecord), it));
	self = *((KprUPnPController*)it);
	self->authority = FskStrDoCopy(authority);
	bailIfNULL(self->authority);
	self->ip = FskStrDoCopy(ip);
	bailIfNULL(self->ip);
	self->uuid = FskStrDoCopy(uuid);
	bailIfNULL(self->uuid);
	FskTimeCallbackNew(&self->pingTimer);
	bailIfNULL(self->pingTimer);
	FskInstrumentedItemNew(self, NULL, &KprUPnPControllerInstrumentation);
	return err;
bail:
	KprUPnPControllerDispose(self);
	return err;
}

void KprUPnPControllerDispose(KprUPnPController self)
{
	if (self) {
		if (self->currentMessage) {
			KprMessage message = self->currentMessage;
			self->currentMessage = NULL;
			FskInstrumentedItemPrintfVerbose(self, "cancel %s", message->url);
			KprMessageCancel(message);
			while ((message = FskListRemoveFirst(&self->waitingMessages))) {
				KprMessageDispose(message);
			}
		}
		FskTimeCallbackDispose(self->pingTimer);
		self->pingTimer = NULL;
		KprUPnPDeviceDispose(self->device);
		self->device = NULL;
		FskMemPtrDispose(self->uuid);
		FskMemPtrDispose(self->authority);
		FskMemPtrDispose(self->ip);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprUPnPControllerDiscoveryCallback(char* authority, KprSSDPDiscoveryDescription description, Boolean alive, void* refcon)
{
	FskErr err = kFskErrNone;
	KprUPnPController self;
	KprMessage message = NULL;
	char* uuid = description->uuid;
	char* location = description->url;
	char* ip = description->ip;
	bailIfNULL(uuid);
	self = FskAssociativeArrayElementGetReference(gUPnP->controllers, uuid);
	if (alive && !self) { // alive and new
		bailIfError(KprMessageNew(&message, location));
		KprMessageSetTimeout(message, gUPnP->httpTimeout);
		bailIfError(KprMessageSetRequestHeader(message, kFskStrUserAgent, gUPnP->userAgent));
		bailIfError(KprUPnPControllerNew(&self, authority, ip, uuid));
		FskAssociativeArrayElementSetReference(gUPnP->controllers, uuid, self);
		FskInstrumentedItemPrintfVerbose(self, "get %s device description - %s", uuid, location);
		KprMessageInvoke(message, KprUPnPControllerDeviceMessageComplete, KprUPnPDefaultMessageDispose, FskStrDoCopy(self->uuid));
		self->currentMessage = message;
	}
	else if (!alive && self) { // byebye
		FskInstrumentedItemPrintfVerbose(self, "remove device %s", uuid);
		KprUPnPControllerRemove(self);
	}
bail:
	if (err) {
		KprMessageDispose(message);
	}
	KprSSDPDiscoveryDescriptionDispose(description);
	return;
}

void KprUPnPControllerDeviceMessageComplete(KprMessage message, void* it)
{
	FskErr err = kFskErrNone;
	char* uuid = it;
	KprUPnPController self = NULL;
	KprXMLElement device, element, root = NULL, udn;
	KprMessage nextMessage = NULL;

	bailIfNULL(self = FskAssociativeArrayElementGetReference(gUPnP->controllers, uuid));
	self->currentMessage = NULL;
	FskInstrumentedItemPrintfVerbose(self, "got %s device description - %s -> %d", uuid, message->url, message->status);
	if (message->status / 100 != 2) {
		BAIL(kFskErrNotFound);
	}
	bailIfError(KprXMLParse(&root, message->response.body, message->response.size));
	bailIfNULL(device = KprXMLElementGetFirstElement(root, kUPnPDeviceNameSpace, "device"));
	bailIfNULL(udn = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "UDN"));
	if (FskStrCompare(uuid, KprXMLElementGetValue(udn) + 5)) {
		bailIfNULL(element = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "deviceList"));
		device = KprXMLElementGetFirstElement(element, kUPnPDeviceNameSpace, "device");
		while (device) {
			bailIfNULL(udn = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "UDN"));
			FskDebugStr(KprXMLElementGetValue(udn))
			if (!FskStrCompare(uuid, KprXMLElementGetValue(udn) + 5))
				break;
			device = KprXMLElementGetNextElement(device, kUPnPDeviceNameSpace, "device");
		}
		bailIfNULL(device);
	}
	bailIfNULL(element = KprXMLElementGetFirstElement(device, kUPnPDeviceNameSpace, "deviceType"));
	bailIfError(KprUPnPDeviceNew(&self->device, element->element->value));
	self->device->controller = self;
	self->device->url = FskStrDoCopy(message->url);
	bailIfError(KprUPnPDeviceFromElement(self->device, device, self->uuid));
	if (self->device->service) {
		// get service descriptions
		bailIfError(KprMessageNew(&nextMessage, self->device->service->url));
		KprMessageSetTimeout(nextMessage, gUPnP->httpTimeout);
		bailIfError(KprMessageSetRequestHeader(nextMessage, kFskStrUserAgent, gUPnP->userAgent));
		FskInstrumentedItemPrintfVerbose(self, "get %s service description - %s", uuid, self->device->service->url);
		KprMessageInvoke(nextMessage, KprUPnPControllerServiceMessageComplete, KprUPnPDefaultMessageDispose, FskStrDoCopy(self->uuid));
		self->currentMessage = nextMessage;
	}
	else {
		KprUPnPControllerNotifyDevice(self, true);
	}
bail:
	KprXMLElementDispose(root);
	if (err) {
		KprMessageDispose(nextMessage);
		if (self) {
			FskAssociativeArrayElementDispose(gUPnP->controllers, self->uuid);
			KprUPnPControllerDispose(self);
		}
	}
	return;
}

void KprUPnPControllerNotifyDevice(KprUPnPController self, Boolean alive)
{
	FskErr err = kFskErrNone;
	KprUPnPDevice device = self->device;
	KprScriptBehavior behavior = NULL;
	
	if (self->alive == alive) return;
	if (!device) return;
	bailIfError(KprUPnPGetBehavior(self->authority, &behavior));
	xsBeginHostSandboxCode(behavior->the, behavior->code);
	{
		xsVars(3);
		{
			xsTry {
				FskInstrumentedItemPrintfVerbose(self, "notify %s - %s", device->uuid, alive ? "alive" : "byebye");
				xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
				xsSet(xsVar(0), xsID("friendlyName"), xsString(device->friendlyName));
				xsSet(xsVar(0), xsID("manufacturer"), xsString(device->manufacturer));
				xsSet(xsVar(0), xsID("modelName"), xsString(device->modelName));
				xsSet(xsVar(0), xsID("uuid"), xsString(device->uuid));
				xsSet(xsVar(0), xsID("location"), xsString(device->url));
				xsSet(xsVar(0), xsID("type"), xsString(device->type));
				xsSet(xsVar(0), xsID("ip"), xsString(self->ip));
				if (alive) {
					if (xsFindResult(behavior->slot, xsID_onUPnPAdded)) {
						if (device->icon) {
							KprUPnPIcon icon;
							xsVar(1) = xsNewInstanceOf(xsArrayPrototype);
							for (icon = device->icon; icon; icon = icon->next) {
								char* url = NULL;
								if (KprURLMerge(device->url, icon->url, &url) != kFskErrNone) continue;
								xsVar(2) = xsNewInstanceOf(xsObjectPrototype);
								xsSet(xsVar(2), xsID("mime"), xsString(icon->mime));
								xsSet(xsVar(2), xsID("url"), xsString(url));
								xsSet(xsVar(2), xsID("width"), xsInteger(icon->width));
								xsSet(xsVar(2), xsID("height"), xsInteger(icon->height));
								(void)xsCall1(xsVar(1), xsID("push"), xsVar(2));
								FskMemPtrDispose(url);
							}
							xsSet(xsVar(0), xsID("icons"), xsVar(1));
						}
						if (device->service) {
							KprUPnPService service;
							xsVar(1) = xsNewInstanceOf(xsArrayPrototype);
							for (service = device->service; service; service = service->next) {
								(void)xsCall1(xsVar(1), xsID("push"), xsString(service->type));
							}
							xsSet(xsVar(0), xsID("services"), xsVar(1));
						}
						(void)xsCallFunction1(xsResult, behavior->slot, xsVar(0));
					}
					KprUPnPControllerPingSchedule(self);
				}
				else {
					if (xsFindResult(behavior->slot, xsID_onUPnPRemoved)) {
						(void)xsCallFunction1(xsResult, behavior->slot, xsVar(0));
					}
				}
			}
			xsCatch {
			}
		}
	}
	xsEndHostSandboxCode();
bail:
	self->alive = alive;
	return;
}

void KprUPnPControllerPingCallback(FskTimeCallBack timer UNUSED, const FskTime time UNUSED, void *param)
{
	FskErr err = kFskErrNone;
	KprUPnPController self = param;
	// device likely dissapeared
	// check with HEAD request
	KprMessage message = NULL;
	FskInstrumentedItemPrintfVerbose(self, "Ping %s - %s", self->device->friendlyName, self->device->url);
	bailIfError(KprMessageNew(&message, self->device->url));
	bailIfError(KprMessageSetMethod(message, "HEAD"));
	KprMessageSetTimeout(message, gUPnP->httpTimeout);
	KprMessageInvoke(message, KprUPnPControllerPingMessageComplete, KprUPnPDefaultMessageDispose, FskStrDoCopy(self->uuid));
bail:
	if (err)
		KprMessageDispose(message);
	return;
}

void KprUPnPControllerPingMessageComplete(KprMessage message, void* it)
{
	FskErr err = kFskErrNone;
	char* uuid = it;
	KprUPnPController self;
	
	bailIfNULL(self = FskAssociativeArrayElementGetReference(gUPnP->controllers, uuid));
	if (self) {
		if (message->status) {
			FskInstrumentedItemPrintfVerbose(self, "Ping %s - %s -> %d (%d)", self->device->friendlyName, self->device->url, message->status, message->error);
			KprUPnPControllerPingSchedule(self);
		}
		else {
			FskInstrumentedItemPrintfNormal(self, "Ping %s - %s -> %d (%d) -> removed", self->device->friendlyName, self->device->url, message->status, message->error);
			FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPRemoveDiscoveryByLocation, FskStrDoCopy(self->device->url), NULL, NULL, NULL);
		}
	}
bail:
	return;
}

void KprUPnPControllerPingSchedule(KprUPnPController self)
{
	if (gUPnP->upnpControllerPingPeriod && self->device)
		FskTimeCallbackScheduleFuture(self->pingTimer, gUPnP->upnpControllerPingPeriod / 1000, gUPnP->upnpControllerPingPeriod % 1000, KprUPnPControllerPingCallback, self);
}

void KprUPnPControllerRemove(KprUPnPController self)
{
	if (self) {
		self->removed = true;
		KprUPnPControllerNotifyDevice(self, false);
		FskAssociativeArrayElementDispose(gUPnP->controllers, self->uuid);
		KprUPnPControllerDispose(self);
	}
}

void KprUPnPControllerServiceMessageComplete(KprMessage message, void* it)
{
	FskErr err = kFskErrNone;
	char* uuid = it;
	KprUPnPController self = NULL;
	KprUPnPDevice device;
	KprUPnPService service;
	KprXMLElement scpd = NULL;
	KprMessage nextMessage = NULL;
	
	bailIfNULL(self = FskAssociativeArrayElementGetReference(gUPnP->controllers, uuid));
	self->currentMessage = NULL;
	FskInstrumentedItemPrintfVerbose(self, "got %s service description - %s -> %d", uuid, message->url, message->status);
	if (message->status / 100 != 2) {
		BAIL(kFskErrNotFound);
	}
	bailIfNULL(device = self->device);
	for (service = device->service; service; service = service->next) {
		if (!FskStrCompare(message->url, service->url)) break;
	}
	bailIfNULL(service);
	bailIfError(KprXMLParse(&scpd, message->response.body, message->response.size));
	bailIfError(KprUPnPServiceFromSCPDElement(service, scpd));
	if (service->next) {
		bailIfError(KprMessageNew(&nextMessage, service->next->url));
		KprMessageSetTimeout(nextMessage, gUPnP->httpTimeout);
		bailIfError(KprMessageSetRequestHeader(nextMessage, kFskStrUserAgent, gUPnP->userAgent));
		FskInstrumentedItemPrintfVerbose(self, "get %s service description - %s", uuid, service->next->url);
		KprMessageInvoke(nextMessage, KprUPnPControllerServiceMessageComplete, KprUPnPDefaultMessageDispose, FskStrDoCopy(uuid));
		self->currentMessage = nextMessage;
	}
	else {
		KprUPnPControllerNotifyDevice(self, true);
	}
bail:
	KprXMLElementDispose(scpd);
	if (err) {
		KprMessageDispose(nextMessage);
		if (self) {
			FskAssociativeArrayElementDispose(gUPnP->controllers, self->uuid);
			KprUPnPControllerDispose(self);
		}
	}
	return;
}

void KprUPnPControllerServiceActionCallback(KprUPnPController self, char* serviceType, char* actionName, SInt32 errorCode, char* errorDescription)
{
	FskErr err = kFskErrNone;
	
	{
		KprScriptBehavior behavior = NULL;
		KprUPnPDevice device = self->device;
		
		if (errorCode) {
			KprUPnPControllerServiceActionCallbackError(self, serviceType, actionName, errorCode, errorDescription);
		} else {
			if (FskStrCompare(actionName, "Play") == 0) {
				KprUPnPControllerServiceActionCallbackPlay(self, serviceType);
			} else if (FskStrCompare(actionName, "Stop") == 0 || FskStrCompare(actionName, "Pause") == 0) {
				KprUPnPControllerServiceActionCallbackStop(self, serviceType);
			} else if (FskStrCompare(actionName, "GetPositionInfo") == 0) {
				KprUPnPControllerServiceActionCallbackGetPositionInfo(self, serviceType);
			} else if (FskStrCompare(actionName, "Seek") == 0) {
				KprUPnPControllerServiceActionCallbackSeek(self, serviceType);
			} else {
				KprUPnPControllerServiceActionCallbackOther(self, serviceType, actionName);
			}
		}
		
		bailIfError(KprUPnPGetBehavior(self->authority, &behavior));
		xsBeginHostSandboxCode(behavior->the, behavior->code);
		{
			if (errorCode) {
				if (xsFindResult(behavior->slot, xsID("onUPnPActionError"))) {
					(void)xsCallFunction6(xsResult, behavior->slot, xsString(self->uuid), xsString(device ? device->type : NULL), xsString(serviceType), xsString(actionName), xsInteger(errorCode), xsString(errorDescription));
				}
			}
			else {
				if (xsFindResult(behavior->slot, xsID("onUPnPActionComplete"))) {
					(void)xsCallFunction4(xsResult, behavior->slot, xsString(self->uuid), xsString(device ? device->type : NULL), xsString(serviceType), xsString(actionName));
				}
			}
		}
		xsEndHostSandboxCode();
	}
bail:
	return;
}

void KprUPnPControllerServiceActionCallbackPlay(KprUPnPController self, const char* serviceType)
{
#if TARGET_OS_IPHONE
	KprUPnPControllerServiceActionCallbackPlay_iOS(self, serviceType);
#elif TARGET_OS_ANDROID
	KprUPnPControllerServiceActionCallbackPlay_Android(self, serviceType);
#endif
}

void KprUPnPControllerServiceActionCallbackStop(KprUPnPController self, const char* serviceType)
{
#if TARGET_OS_IPHONE
	KprUPnPControllerServiceActionCallbackStop_iOS(self, serviceType);
#elif TARGET_OS_ANDROID
	KprUPnPControllerServiceActionCallbackStop_Android(self, serviceType);
#endif
}

static void KprUPnPControllerGetDurationAndPosition(KprUPnPController self, const char* serviceType, double *duration, double *position)
{
	const char *durationP = KprUPnPControllerServiceGetVariable(self, serviceType, "CurrentTrackDuration");
	const char *positionP = KprUPnPControllerServiceGetVariable(self, serviceType, "RelativeTimePosition");
	
	*duration = KprUPnPControllerParseTimecode(durationP);
	*position = KprUPnPControllerParseTimecode(positionP);
}

void KprUPnPControllerServiceActionCallbackGetPositionInfo(KprUPnPController self, const char* serviceType)
{
	double duration, position;
	KprUPnPControllerGetDurationAndPosition(self, serviceType, &duration, &position);
	
#if TARGET_OS_IPHONE
	KprUPnPControllerServiceActionCallbackGetPositionInfo_iOS(self, serviceType, duration, position);
#endif
}

void KprUPnPControllerServiceActionCallbackSeek(KprUPnPController self, const char* serviceType)
{
#if TARGET_OS_IPHONE
	KprUPnPControllerServiceActionCallbackSeek_iOS(self, serviceType);
#endif
}

void KprUPnPControllerServiceActionCallbackOther(KprUPnPController self, const char* serviceType, const char *actionName)
{
#if TARGET_OS_IPHONE
	KprUPnPControllerServiceActionCallbackOther_iOS(self, serviceType, actionName);
#endif
}

void KprUPnPControllerServiceActionCallbackError(KprUPnPController self, const char* serviceType, const char *actionName, SInt32 errorCode, const char* errorDescription)
{
#if TARGET_OS_IPHONE
	KprUPnPControllerServiceActionCallbackError_iOS(self, serviceType, actionName, errorCode, errorDescription);
#endif
}


const char *KprUPnPControllerServiceGetVariable(KprUPnPController self, const char* serviceType, char* variableName)
{
	KprUPnPService service = NULL;
	KprUPnPStateVariable variable;
	
	for (service = self->device->service; service; service = service->next) {
		if (FskStrCompare(service->type, serviceType) == 0) break;
	}
	if (service == NULL) return NULL;
	
	variable = FskAssociativeArrayElementGetReference(service->stateVariables, variableName);
	if (variable == NULL) return NULL;
	
	return KprUPnPStateVariableGetValue(variable);
}

FskErr KprUPnPControllerServiceActionInvoke(KprUPnPController self, char* serviceType, char* actionName)
{
	FskErr err = kFskErrNone;
	KprUPnPDevice device = self->device;
	KprUPnPService service;
	FskGrowableStorage storage = NULL;
	KprUPnPAction action;
	KprUPnPArgument argument;
	KprMessage message = NULL;
	UInt32 size;
	void* data;
	char string[16];
	char* soapAction = NULL;

	bailIfNULL(device);
	for (service = device->service; service; service = service->next) {
		if (!FskStrCompare(service->type, serviceType)) break;
	}
	bailIfNULL(service);
	action = FskAssociativeArrayElementGetReference(service->actions, actionName);
	bailIfNULL(action);
	bailIfError(FskGrowableStorageNew(1024, &storage));
	bailIfError(KprUPnPStorageWrite(storage, "<?xml version=\"1.0\"?>\n", 0));
	bailIfError(KprUPnPStorageWrite(storage, "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"", 0));
	bailIfError(KprUPnPStorageWrite(storage, kUPnPSoapEncodingNameSpace, 0));
	bailIfError(KprUPnPStorageWrite(storage, "\">\n\t<s:Body>\n", 0));
	bailIfError(KprUPnPStorageWrite(storage, "\t\t<u:", 0));
	bailIfError(KprUPnPStorageWrite(storage, action->name, 0));
	bailIfError(KprUPnPStorageWrite(storage, " xmlns:u=\"", 0));
	bailIfError(KprUPnPStorageWrite(storage, service->type, 0));
	bailIfError(KprUPnPStorageWrite(storage, "\">\n", 0));
	for (argument = action->argumentIn; argument; argument = argument->next) {
		char* value = KprUPnPStateVariableGetValue(argument->relatedStateVariable);
		bailIfError(KprUPnPStorageWrite(storage, "\t\t\t<", 0));
		bailIfError(KprUPnPStorageWrite(storage, argument->name, 0));
		bailIfError(KprUPnPStorageWrite(storage, ">", 0));
		bailIfError(KprUPnPStorageWriteEntity(storage, value, 2));
		bailIfError(KprUPnPStorageWrite(storage, "</", 0));
		bailIfError(KprUPnPStorageWrite(storage, argument->name, 0));
		bailIfError(KprUPnPStorageWrite(storage, ">\n", 0));
	}
	bailIfError(KprUPnPStorageWrite(storage, "\t\t</u:", 0));
	bailIfError(KprUPnPStorageWrite(storage, action->name, 0));
	bailIfError(KprUPnPStorageWrite(storage, ">\n", 0));
	bailIfError(KprUPnPStorageWrite(storage, "\t</s:Body>\n</s:Envelope>", 0));
	KprMessageNew(&message, service->controlURL);
	KprMessageSetTimeout(message, gUPnP->httpTimeout);
	bailIfError(KprMessageSetMethod(message, "POST"));
	bailIfError(KprMessageSetRequestHeader(message, kFskStrContentType, kFskStrTextXMLCharset));
	bailIfError(KprMessageSetRequestHeader(message, kFskStrServer, gUPnP->userAgent));
	size = FskStrLen(service->type) + FskStrLen(actionName) + 4;
	bailIfError(FskMemPtrNewClear(size, &soapAction));
	snprintf(soapAction, size, "\"%s#%s\"", service->type, actionName);
	bailIfError(KprMessageSetRequestHeader(message, "SOAPACTION", soapAction));
	FskGrowableStorageGetPointerToItem(storage, 0, (void **)&data);
	size = FskGrowableStorageGetSize(storage);
	FskStrNumToStr(size, string, sizeof(string));
	bailIfError(KprMessageSetRequestHeader(message, kFskStrContentLength, string));
	bailIfError(KprMessageSetRequestBody(message, data, size));
	if (self->currentMessage) {
		FskListAppend(&self->waitingMessages, message);
		FskInstrumentedItemPrintfNormal(self, "Queue %s %s:%s:%s %p", device->uuid, device->name, service->name, actionName, message);
	}
	else {
		self->currentMessage = message;
		KprMessageInvoke(message, KprUPnPControllerServiceActionComplete, KprUPnPDefaultMessageDispose, FskStrDoCopy(self->uuid));
		FskInstrumentedItemPrintfNormal(self, "Invoke %s %s:%s:%s %p", device->uuid, device->name, service->name, actionName, message);
		FskInstrumentedItemPrintfDebug(self, "%.*s", message->request.size, message->request.body);
	}
bail:
	if (err) {
		if (!device || !service || !action) {
			SInt32 errorCode = kUPnPErrInvalidAction;
			char* errorDescription = "Invalid Action";
			KprUPnPControllerServiceActionCallback(self, serviceType, actionName, errorCode, errorDescription);
		}
		KprMessageDispose(message);
	}
	FskMemPtrDispose(soapAction);
	FskGrowableStorageDispose(storage);
	return err;
}

void KprUPnPControllerServiceActionComplete(KprMessage message, void* it)
{
	FskErr err = kFskErrNone;
	char* uuid = it;
	char* actionResponseName = NULL;
	KprUPnPController self = FskAssociativeArrayElementGetReference(gUPnP->controllers, uuid);
	KprUPnPDevice device = self ? self->device : NULL;
	KprXMLElement root = NULL, element;
	char* actionName;
	char* separator;
	char* serviceType;
	char* soapAction = KprMessageGetRequestHeader(message, "SOAPACTION");
	KprUPnPService service = NULL;
	KprUPnPAction action = NULL;
	KprUPnPArgument argument;
	KprXMLElement arg;
	
	bailIfNULL(self);
	bailIfNULL(device);
	bailIfNULL(soapAction);
	serviceType = soapAction + 1;
	separator = FskStrChr(serviceType, '#');
	bailIfNULL(separator);
	*separator = 0;
	actionName = separator + 1;
	separator = FskStrChr(actionName, '"');
	bailIfNULL(separator);
	*separator = 0;
	for (service = device->service; service; service = service->next) {
		if (!FskStrCompare(service->type, serviceType)) break;
	}
	bailIfNULL(service);
	FskInstrumentedItemPrintfNormal(self, "Complete %s %s:%s:%s %p", device->uuid, device->name, service->name, actionName, message);
	FskInstrumentedItemPrintfDebug(self, "%.*s", message->response.size, message->response.body);
	if (message->status == 0) {
		BAIL(kFskErrNotFound);
	}
	bailIfError(KprXMLParse(&root, message->response.body, message->response.size));
	if (KprXMLElementIsEqual(root, kUPnPSoapEnvelopeNameSpace, "Envelope")) {
		element = KprXMLElementGetFirstElement(root, kUPnPSoapEnvelopeNameSpace, "Body");
		bailIfNULL(element);
		if (message->status == 200) {
			action = FskAssociativeArrayElementGetReference(service->actions, actionName);
			bailIfNULL(action);
			bailIfError(FskMemPtrNewClear(FskStrLen(actionName) + 9, &actionResponseName));
			FskStrCopy(actionResponseName, actionName);
			FskStrCat(actionResponseName, "Response");
			element = KprXMLElementGetFirstElement(element, serviceType, actionResponseName);
			bailIfNULL(element);
			for (argument = action->argumentOut, arg = element->element; argument && arg; argument = argument->next, arg = arg->next) {
				bailIfError(!argument->relatedStateVariable || FskStrCompare(argument->name, arg->name));
				bailIfError(KprUPnPStateVariableSetValue(argument->relatedStateVariable, arg->element ? arg->element->value : ""));
				FskInstrumentedItemPrintfVerbose(self, " + %s <= %s", argument->name, arg->element ? arg->element->value : "");
			}
			KprUPnPControllerServiceActionCallback(self, service->type, actionName, kFskErrNone, NULL);
		}
		else {
			KprXMLElement errorCode;
			KprXMLElement errorDescription;
			
			bailIfNULL(element = KprXMLElementGetFirstElement(element, kUPnPSoapEnvelopeNameSpace, "Fault"));
			bailIfNULL(element = KprXMLElementGetFirstElement(element, NULL, "detail"));
			bailIfNULL(element = KprXMLElementGetFirstElement(element, kUPnPControlNameSpace, "UPnPError"));
			errorCode = KprXMLElementGetFirstElement(element, kUPnPControlNameSpace, "errorCode");
			errorDescription = KprXMLElementGetFirstElement(element, kUPnPControlNameSpace, "errorDescription");
			FskInstrumentedItemPrintfNormal(self, " - Error %d: %s", KprXMLElementGetIntegerValue(errorCode), errorDescription ? KprXMLElementGetValue(errorDescription) : "");
			KprUPnPControllerServiceActionCallback(self, service->type, actionName, KprXMLElementGetIntegerValue(errorCode), errorDescription ? KprXMLElementGetValue(errorDescription) : "");
		}
	}
	KprUPnPControllerPingSchedule(self);
bail:
	FskMemPtrDispose(actionResponseName);
	KprXMLElementDispose(root);
	if (self) {
		if (message->status == 0) {
            if (self->device)
                FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPRemoveDiscoveryByLocation, FskStrDoCopy(self->device->url), NULL, NULL, NULL);
		}
		else {
			message = self->currentMessage = FskListRemoveFirst(&self->waitingMessages);
			if (message) {
				KprMessageInvoke(message, KprUPnPControllerServiceActionComplete, KprUPnPDefaultMessageDispose, FskStrDoCopy(self->uuid));
				FskInstrumentedItemPrintfNormal(self, "Invoke from queue %s %s:%s:%s %p", device->uuid, device->name, service->name, KprMessageGetRequestHeader(message, "SOAPACTION"), message);
				FskInstrumentedItemPrintfDebug(self, "%.*s", message->request.size, message->request.body);
			}
		}
	}
	return;
}

static Boolean KprUPnPControllerServiceIsAction(KprUPnPController self, char* serviceType, char* actionName)
{
	FskErr err = kFskErrNone;
	KprUPnPDevice device = self->device;
	KprUPnPService service;
	KprUPnPAction action;

	bailIfNULL(device);
	for (service = device->service; service; service = service->next) {
		if (!FskStrCompare(service->type, serviceType)) break;
	}
	bailIfNULL(service);
	action = FskAssociativeArrayElementGetReference(service->actions, actionName);
	bailIfNULL(action);
bail:
	return err ? false : true;
}

void KprUPnPControllerServiceEventCallback(KprUPnPController self, char* serviceType, char* name, char* value)
{
	FskErr err = kFskErrNone;
	
	{
		KprScriptBehavior behavior = NULL;
		KprUPnPDevice device = self->device;
		
		bailIfError(KprUPnPGetBehavior(self->authority, &behavior));
		xsBeginHostSandboxCode(behavior->the, behavior->code);
		{
			if (xsFindResult(behavior->slot, xsID("onUPnPEvent"))) {
				(void)xsCallFunction5(xsResult, behavior->slot, xsString(self->uuid), xsString(device ? device->type : NULL), xsString(serviceType), xsString(name), xsString(value));
			}
		}
		xsEndHostSandboxCode();
	}
bail:
	return;
}

//--------------------------------------------------
// XS context
//--------------------------------------------------
#if 0
#pragma mark - XS Device
#endif

void UPnP_Device_start(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprContext context = xsGetContext(the);
	KprUPnP self = gUPnP;
	char* path = NULL;
	char* url = NULL;
	char* friendlyName = NULL;
	char* modelNumber = NULL;
	char* configId = NULL;
	KprUPnPDevice device = KprUPnPGetDevice(context);
	
	xsTry {
		xsAssert(self);
		if (!device) {
			char* deviceType = xsToString(xsArg(0));
			char* from;
			char* to;
			to = FskStrRChr(deviceType, ':');
			if (to) {
				*to = 0;
				from = FskStrRChr(deviceType, ':');
				if (from) {
					*from = 0;
					xsThrowIfFskErr(FskMemPtrNewClear(5 + FskStrLen(from + 1) + 5, &path));
					FskStrCopy(path, "upnp/");
					FskStrCat(path, from + 1);
					FskStrCat(path, ".xml");
					*from = ':';
					xsThrowIfFskErr(KprURLMerge(context->url, path, &url));
				}
				*to = ':';
			}
			xsAssert(url);
			xsThrowIfFskErr(KprUPnPDeviceNew(&device, deviceType));
			if (c > 1 && xsTest(xsArg(1))) {
				friendlyName = xsToString(xsArg(1));
				xsAssert(device->friendlyName = FskStrDoCopy(friendlyName));
			}
			if (c > 2 && xsTest(xsArg(2))) {
				modelNumber = xsToString(xsArg(2));
				xsAssert(device->modelNumber = FskStrDoCopy(modelNumber));
			}
			if (c > 3 && xsTest(xsArg(3))) {
				configId = xsToString(xsArg(3));
				xsAssert(device->configId = FskStrDoCopy(configId));
			}
			xsThrowIfFskErr(KprUPnPDeviceFromFile(device, url));
			xsThrowIfFskErr(KprUPnPAddDevice(context, device));
			
			FskMemPtrDispose(url);
			FskMemPtrDispose(path);
		}
	}
	xsCatch {
		KprUPnPDeviceDispose(device);
		FskMemPtrDispose(url);
		FskMemPtrDispose(path);
		xsThrow(xsException);
	}
}

void UPnP_Device_stop(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprUPnP self = gUPnP;
	
	xsTry {
		KprUPnPDevice device = KprUPnPGetDevice(context);
		xsAssert(self);
		if (device) {
			xsThrowIfFskErr(KprUPnPRemoveDevice(context));
			KprUPnPDeviceDispose(device);
		}
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void UPnP_Device_getFriendlyName(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprUPnP self = gUPnP;
	xsTry {
		KprUPnPDevice device = KprUPnPGetDevice(context);
		xsAssert(self);
		if (device)
			xsResult = xsString(device->friendlyName);
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void UPnP_Device_getRunning(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprUPnPDevice device = KprUPnPGetDevice(context);
	xsResult = device ? xsTrue : xsFalse;
}

void UPnP_Device_getVariable(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprUPnP self = gUPnP;
	xsTry {
		char* serviceType = xsToString(xsArg(0));
		char* variableName = xsToString(xsArg(1));
		KprUPnPDevice device = KprUPnPGetDevice(context);
		xsAssert(self);
		if (device) {
			KprUPnPService service;
			for (service = device->service; service; service = service->next) {
				if (!FskStrCompare(service->type, serviceType)) break;
			}
			xsAssert(service);
			{
				KprUPnPStateVariable variable;
				variable = FskAssociativeArrayElementGetReference(service->stateVariables, variableName);
				if (variable && variable->value) {
					FskInstrumentedItemPrintfDebug(self, "GET VARIABLE %s = %s", variableName, variable->value);
					if (xsToInteger(xsArgc) == 3) {
						if (xsTest(xsArg(2)))
							xsResult = xsString(variable->value);
						else
							(*variable->toXS)(the, variable);
					}
					else
						(*variable->toXS)(the, variable);
				}
			}
		}
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void UPnP_Device_getUUID(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprUPnPDevice device = KprUPnPGetDevice(context);
	if (device)
		xsResult = xsString(device->uuid);
}

void UPnP_Device_changed(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprUPnP self = gUPnP;
	xsTry {
		char* serviceType = xsToString(xsArg(0));
		KprUPnPDevice device = KprUPnPGetDevice(context);
		xsAssert(self);
		if (device) {
			KprUPnPService service;
			for (service = device->service; service; service = service->next) {
				if (!FskStrCompare(service->type, serviceType)) break;
			}
			xsAssert(service);
			xsThrowIfFskErr(KprUPnPServiceEvent(service));
		}
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void UPnP_Device_lastChangeVariables(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprUPnP self = gUPnP;
	xsTry {
		KprUPnPDevice device = KprUPnPGetDevice(context);
		char* serviceType = xsToString(xsArg(0));
		char* namespace = xsToString(xsArg(1));
		
		xsAssert(self);
		if (device) {
			KprUPnPService service;
			for (service = device->service; service; service = service->next) {
				if (!FskStrCompare(service->type, serviceType)) break;
			}
			xsAssert(service);
			{
				UInt32 i, length = xsToInteger(xsGet(xsArg(2), xsID_length));;
				
				service->lastChangeNamespace = FskStrDoCopy(namespace);
				for (i = 0; i < length; i++) {
					char* variableName = xsToString(xsGetAt(xsArg(2), xsInteger(i)));
					KprUPnPStateVariable variable = FskAssociativeArrayElementGetReference(service->stateVariables, variableName);
					if (variable) {
						variable->lastChange = true;
					}
				}
			}
		}
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void UPnP_Device_setError(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprUPnP self = gUPnP;
	xsTry {
		char* serviceType = xsToString(xsArg(0));
		UInt32 error = xsToInteger(xsArg(1));
		char* description = xsToString(xsArg(2));
		KprUPnPDevice device = KprUPnPGetDevice(context);
		xsAssert(self);
		FskDebugStr("UPnP_Device_setError %d - %s", error, description);
		if (device) {
			KprUPnPService service;
			for (service = device->service; service; service = service->next) {
				if (!FskStrCompare(service->type, serviceType)) break;
			}
			xsAssert(service);
			{
				service->actionError = error;
				if (description)
					service->actionErrorDescription = FskStrDoCopy(description);
			}
		}
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void UPnP_Device_setVariable(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprUPnP self = gUPnP;
	xsTry {
		char* serviceType = xsToString(xsArg(0));
		char* variableName = xsToString(xsArg(1));
		KprUPnPDevice device = KprUPnPGetDevice(context);
		xsAssert(self);
		if (device) {
			KprUPnPService service;
			for (service = device->service; service; service = service->next) {
				if (!FskStrCompare(service->type, serviceType)) break;
			}
			xsAssert(service);
			xsAssert(self);
			{
				KprUPnPStateVariable variable;
				variable = FskAssociativeArrayElementGetReference(service->stateVariables, variableName);
				if (variable) {
					char* variableValue = xsToString(xsArg(2));
					FskInstrumentedItemPrintfDebug(self, "SET VARIABLE %s = %s (%d)", variableName, variableValue, variable->changed);
					xsThrowIfFskErr(KprUPnPStateVariableSetValue(variable, variableValue));
				}
			}
		}
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void KPR_get_mediaMimes(xsMachine *the)
{
	UInt32 index = 0;
	UInt32 propertyID = 20751;
	xsIndex id_push = xsID("push");

	xsResult = xsNewInstanceOf(xsArrayPrototype);

	// media readers
	while (true) {
		FskMediaPropertyValueRecord property;
		FskMediaReaderDispatch rd = FskExtensionGetByIndex(kFskExtensionMediaReader, index++);
		if (NULL == rd) break;
		if (kFskErrNone == FskMediaGetProperty(rd->properties, NULL, NULL, propertyID, &property)) {
			if (kFskMediaPropertyTypeStringList == property.type) {
				char *strings = property.value.str;
				while (*strings) {
					xsCall1_noResult(xsResult, id_push, xsString(strings));
					strings += FskStrLen(strings) + 1;
				}
			}
			FskMediaPropertyEmpty(&property);
		}
	}

	// image readers
	index = 0;
	while (true) {
		FskMediaPropertyValueRecord property;
		FskImageDecompressor id = FskExtensionGetByIndex(kFskExtensionImageDecompressor, index++);
		if (NULL == id) break;

		if (kFskErrNone == FskMediaGetProperty(id->properties, NULL, NULL, propertyID, &property)) {
			if (kFskMediaPropertyTypeStringList == property.type) {
				char *strings = property.value.str;
				while (*strings) {
					xsCall1_noResult(xsResult, id_push, xsString(strings));
					strings += FskStrLen(strings) + 1;
				}
			}
			FskMediaPropertyEmpty(&property);
		}
	}
}

#if 0
#pragma mark - XS Controller
#endif

#if TARGET_OS_ANDROID
static FskTimeCallBack gKprUPnPControllerTimer = NULL;

static void KprUPnPControllerPlaybackCallback(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *param UNUSED)
{
	FskEvent fskEvent;
	FskTimeRecord updateTime;
	FskWindow win = FskWindowGetActive();
	
	FskTimeGetNow(&updateTime);
	FskErr err = FskEventNew(&fskEvent, kFskEventWindowBeforeUpdate, &updateTime, kFskEventModifierNotSet);
	if (err == kFskErrNone) FskWindowEventSend(win, fskEvent);
	FskTimeCallbackScheduleFuture(gKprUPnPControllerTimer, 1, 0, KprUPnPControllerPlaybackCallback, NULL);
}

void KprUPnPControllerServiceActionCallbackPlay_Android(KprUPnPController self, const char* serviceType)
{
	FskTimeCallbackNew(&gKprUPnPControllerTimer);
	FskTimeCallbackScheduleFuture(gKprUPnPControllerTimer, 1, 0, KprUPnPControllerPlaybackCallback, NULL);
}

void KprUPnPControllerServiceActionCallbackStop_Android(KprUPnPController self, const char* serviceType)
{
	FskTimeCallbackDispose(gKprUPnPControllerTimer);
	gKprUPnPControllerTimer = NULL;
}

void UPnP_Controller_isBackgroundPlaying(xsMachine *the)
{	
	if (gKprUPnPControllerTimer)
		xsResult = xsTrue;
	else
		xsResult = xsFalse;
}
#elif !TARGET_OS_IPHONE
void UPnP_Controller_isBackgroundPlaying(xsMachine *the)
{
	xsResult = xsTrue;
}
#endif

void UPnP_Controller_discover(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	char* type = FskStrDoCopy(xsToString(xsArg(0)));
	char** services = NULL;
	UInt16 c, i, length = xsToInteger(xsArgc);
	xsAssert(type);
	if (length > 1) {
		c = xsToInteger(xsGet(xsArg(1), xsID("length")));
		xsThrowIfFskErr(FskMemPtrNewClear((c + 1) * sizeof(char*), &services));
		for (i = 0; i < c; i++) {
			services[i] = FskStrDoCopy(xsToString(xsGetAt(xsArg(1), xsInteger(i))));
			xsAssert(services[i]);
		}
	}
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPDiscoverDevice, FskStrDoCopy(context->id), type, services, KprUPnPControllerDiscoveryCallback);
}

void UPnP_Controller_forget(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	char* type = FskStrDoCopy(xsToString(xsArg(0)));
	char** services = NULL;
	UInt16 c, i, length = xsToInteger(xsArgc);
	xsAssert(type);
	if (length > 1) {
		c = xsToInteger(xsGet(xsArg(1), xsID("length")));
		xsThrowIfFskErr(FskMemPtrNewClear((c + 1) * sizeof(char*), &services));
		for (i = 0; i < c; i++) {
			services[i] = FskStrDoCopy(xsToString(xsGetAt(xsArg(1), xsInteger(i))));
			xsAssert(services[i]);
		}
	}
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPForgetDevice, FskStrDoCopy(context->id), type, services, NULL);
}

void UPnP_Controller_subscribe(xsMachine *the)
{
	KprUPnP self = gUPnP;
	KprContext context = xsGetContext(the);
	char* uuid = xsToString(xsArg(0));
	char* serviceType = xsToString(xsArg(1));
	char* url = NULL;
	KprUPnPSubscription subscription = NULL;

	xsTry {
		KprUPnPController controller = NULL;
		KprUPnPDevice device = NULL;
		KprUPnPService service = NULL;
		UInt32 size;
		KprHTTPServer server = KprHTTPServerGet(context->id);
		
		xsAssert(self);
		xsAssert(server);
		xsThrowIfFskErr(KprUPnPAddEventHandler(context));
		xsAssert(uuid);
		controller = FskAssociativeArrayElementGetReference(self->controllers, uuid);
		xsAssert(controller);
		xsAssert(!controller->removed);
		device = controller->device;
		xsAssert(device);
		for (service = device->service; service; service = service->next) {
			if (!FskStrCompare(service->type, serviceType)) break;
		}
		xsAssert(service);
		xsAssert(!service->subscription);
		xsAssert(service->eventSubURL);
		
		size = 32 + FskStrLen(controller->ip);
		xsThrowIfFskErr(FskMemPtrNewClear(size, &url));
		snprintf(url, size, "http://%s:%lu/upnp/event",
			controller->ip, KprHTTPServerGetPort(server));
		xsThrowIfFskErr(KprUPnPSubscriptionNew(&subscription, service, url, self->subscriptionTimeout));
		service->subscription = subscription;
		KprUPnPSubscriptionSubscribe(subscription, controller);
	}
	xsCatch {
		KprUPnPSubscriptionDispose(subscription);
		xsThrow(xsException);
	}
	FskMemPtrDispose(url);
}

void UPnP_Controller_unsubscribe(xsMachine *the)
{
	KprUPnP self = gUPnP;
	char* uuid = xsToString(xsArg(0));
	char* serviceType = xsToString(xsArg(1));

	xsTry {
		KprUPnPController controller = NULL;
		KprUPnPDevice device = NULL;
		KprUPnPService service = NULL;
		
		xsAssert(self);
		xsAssert(uuid);
		controller = FskAssociativeArrayElementGetReference(self->controllers, uuid);
		xsAssert(controller);
		device = controller->device;
		xsAssert(device);
		for (service = device->service; service; service = service->next) {
			if (!FskStrCompare(service->type, serviceType)) break;
		}
		xsAssert(service);
		xsAssert(service->subscription);
		xsAssert(service->eventSubURL);
		KprUPnPSubscriptionUnsubscribe(service->subscription, controller);
		service->subscription->service = NULL;
		service->subscription = NULL;
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void UPnP_Controller_getVariable(xsMachine *the)
{
	KprUPnP self = gUPnP;
	char* uuid = xsToString(xsArg(0));
	char* serviceType = xsToString(xsArg(1));
	char* variableName = xsToString(xsArg(2));

	xsTry {
		KprUPnPController controller = NULL;
		KprUPnPDevice device = NULL;
		KprUPnPService service = NULL;
		KprUPnPStateVariable variable;
		
		xsAssert(self);
		xsAssert(uuid);
		controller = FskAssociativeArrayElementGetReference(self->controllers, uuid);
		xsAssert(controller);
		device = controller->device;
		xsAssert(device);
		for (service = device->service; service; service = service->next) {
			if (!FskStrCompare(service->type, serviceType)) break;
		}
		xsAssert(service);
		variable = FskAssociativeArrayElementGetReference(service->stateVariables, variableName);
		xsAssert(variable);
		(*variable->toXS)(the, variable);
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void UPnP_Controller_getVariableMinimum(xsMachine *the)
{
	KprUPnP self = gUPnP;
	char* uuid = xsToString(xsArg(0));
	char* serviceType = xsToString(xsArg(1));
	char* variableName = xsToString(xsArg(2));

	xsTry {
		KprUPnPController controller = NULL;
		KprUPnPDevice device = NULL;
		KprUPnPService service = NULL;
		KprUPnPStateVariable variable;
		
		xsAssert(self);
		xsAssert(uuid);
		controller = FskAssociativeArrayElementGetReference(self->controllers, uuid);
		xsAssert(controller);
		device = controller->device;
		xsAssert(device);
		for (service = device->service; service; service = service->next) {
			if (!FskStrCompare(service->type, serviceType)) break;
		}
		xsAssert(service);
		variable = FskAssociativeArrayElementGetReference(service->stateVariables, variableName);
		xsAssert(variable);
		if (variable->fromElement == KprUPnPStateVariableSIntFromElement) {
			KprUPnPSIntData data = variable->data;
			xsResult = xsInteger(data->minimum);
		}
		else if (variable->fromElement == KprUPnPStateVariableUIntFromElement) {
			KprUPnPUIntData data = variable->data;
			xsResult = xsInteger(data->minimum);
		}
		else
			xsAssert(NULL);
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void UPnP_Controller_getVariableMaximum(xsMachine *the)
{
	KprUPnP self = gUPnP;
	char* uuid = xsToString(xsArg(0));
	char* serviceType = xsToString(xsArg(1));
	char* variableName = xsToString(xsArg(2));

	xsTry {
		KprUPnPController controller = NULL;
		KprUPnPDevice device = NULL;
		KprUPnPService service = NULL;
		KprUPnPStateVariable variable;
		
		xsAssert(self);
		xsAssert(uuid);
		controller = FskAssociativeArrayElementGetReference(self->controllers, uuid);
		xsAssert(controller);
		device = controller->device;
		xsAssert(device);
		for (service = device->service; service; service = service->next) {
			if (!FskStrCompare(service->type, serviceType)) break;
		}
		xsAssert(service);
		variable = FskAssociativeArrayElementGetReference(service->stateVariables, variableName);
		xsAssert(variable);
		if (variable->fromElement == KprUPnPStateVariableSIntFromElement) {
			KprUPnPSIntData data = variable->data;
			xsResult = xsInteger(data->maximum);
		}
		else if (variable->fromElement == KprUPnPStateVariableUIntFromElement) {
			KprUPnPUIntData data = variable->data;
			xsResult = xsInteger(data->maximum);
		}
		else
			xsAssert(NULL);
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void UPnP_Controller_setVariable(xsMachine *the)
{
	KprUPnP self = gUPnP;
	char* uuid = xsToString(xsArg(0));
	char* serviceType = xsToString(xsArg(1));
	char* variableName = xsToString(xsArg(2));
	char* variableValue = xsToString(xsArg(3));

	xsTry {
		KprUPnPController controller = NULL;
		KprUPnPDevice device = NULL;
		KprUPnPService service = NULL;
		KprUPnPStateVariable variable;
		
		xsAssert(self);
		xsAssert(uuid);
		controller = FskAssociativeArrayElementGetReference(self->controllers, uuid);
		xsAssert(controller);
		xsAssert(!controller->removed);
		device = controller->device;
		xsAssert(device);
		for (service = device->service; service; service = service->next) {
			if (!FskStrCompare(service->type, serviceType)) break;
		}
		xsAssert(service);
		variable = FskAssociativeArrayElementGetReference(service->stateVariables, variableName);
		xsAssert(variable);
		xsThrowIfFskErr(KprUPnPStateVariableSetValue(variable, variableValue));
		FskInstrumentedItemPrintfDebug(self, "SET VARIABLE %s = %s", variableName, variableValue);

		if (FskStrCompare(variableName, "AVTransportURIMetaData") == 0) {
			KprUPnPControllerParseMetadata(variableValue);
		}
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void KprUPnPControllerParseMetadata(const char *variableValue)
{
	KprXMLElement root = NULL;
	KprXMLElement item;
	KprXMLElement element;
	char *attribute;
	KprUPnPMetadataRecord metadata;

	if (KprXMLParse(&root, (UInt8 *)variableValue, FskStrLen(variableValue)) == kFskErrNone) {
		item = root->element;
		/*
		 <DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dlna="urn:schemas-dlna-org:metadata-1-0/" xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">
		 <item id="http://10.0.1.196:8080/83900">
		 <upnp:class>object.item.audioItem</upnp:class>
		 <res duration="00:10:08.613" protocolInfo="http-get:*:audio/mp4:DLNA.ORG_PN=AAC_ISO_320;DLNA.ORG_OP=01">http://10.0.1.196:8080/83900</res>
		 <upnp:albumArtURI>http://10.0.1.196:8080/83901</upnp:albumArtURI>
		 <dc:title>Der Freischutz: Overture</dc:title>
		 <upnp:album>Maestro Nobile: 4</upnp:album>
		 <upnp:artist>Herbert von Karajan</upnp:artist>
		 <dc:creator>Herbert von Karajan</dc:creator>
		 <upnp:genre>Classical</upnp:genre>
		 <upnp:originalTrackNumber>5</upnp:originalTrackNumber>
		 </item>
		 </DIDL-Lite>
		 */

		if (item) {
			metadata.upnpClass = NULL;
			metadata.resourceUrl = NULL;
			metadata.duration = 0.0;
			metadata.artworkUri = NULL;
			metadata.title = NULL;
			metadata.album = NULL;
			metadata.artist = NULL;
			metadata.creator = NULL;
			metadata.genre = NULL;
			metadata.track = NULL;
			
			for (element = item->element; element; element = element->next) {
				if (KprXMLElementIsEqual(element, kUPnPMetadataUPnPNameSpace, "class")) {
					metadata.upnpClass = KprXMLElementGetValue(element);
				} else if (KprXMLElementIsEqual(element, kUPnPMetadataDIDLLiteNameSpace, "res")) {
					metadata.resourceUrl = KprXMLElementGetValue(element);
					
					attribute = KprXMLElementGetAttribute(element, "duration");
					if (attribute)
						metadata.duration = KprUPnPControllerParseTimecode(attribute);
				} else if (KprXMLElementIsEqual(element, kUPnPMetadataUPnPNameSpace, "albumArtURI")) {
					metadata.artworkUri = KprXMLElementGetValue(element);
				} else if (KprXMLElementIsEqual(element, kUPnPMetadataDcNameSpace, "title")) {
					metadata.title = KprXMLElementGetValue(element);
				} else if (KprXMLElementIsEqual(element, kUPnPMetadataUPnPNameSpace, "album")) {
					metadata.album = KprXMLElementGetValue(element);
				} else if (KprXMLElementIsEqual(element, kUPnPMetadataUPnPNameSpace, "artist")) {
					metadata.artist = KprXMLElementGetValue(element);
				} else if (KprXMLElementIsEqual(element, kUPnPMetadataDcNameSpace, "creator")) {
					metadata.creator = KprXMLElementGetValue(element);
				} else if (KprXMLElementIsEqual(element, kUPnPMetadataUPnPNameSpace, "genre")) {
					metadata.genre = KprXMLElementGetValue(element);
				} else if (KprXMLElementIsEqual(element, kUPnPMetadataUPnPNameSpace, "originalTrackNumber")) {
					metadata.track = KprXMLElementGetIntegerValue(element);
				}
			}
			
			KprUPnPControllerGotMetadata(&metadata);
		}
	}

	KprXMLElementDispose(root);
}

void KprUPnPControllerGotMetadata(KprUPnPMetadata metadata)
{
#if TARGET_OS_IPHONE
	KprUPnPControllerGotMetadata_iOS(metadata);
#endif
}

double KprUPnPControllerParseTimecode(const char *timecode) {
	char *c1, *c2, *c3, *dot;
	int day = 0, hour = 0, min = 0, sec = 0, ms = 0;
	double time;

	if (timecode == NULL) return 0.0;
	
	c1 = FskStrChr(timecode, ':');
	if (c1) {
		c1 += 1;
		c2 = FskStrChr(c1, ':');
		if (c2) {
			c2 += 1;
			c3 = FskStrChr(c2, ':');
			if (c3) {
				c3 += 1;
			}
		}
	}

	if (c1 == NULL) {
		sec = FskStrToNum(timecode);
	} else if (c2 == NULL) {
		min = FskStrToNum(timecode);
		sec = FskStrToNum(c1);
	} else if (c3 == NULL) {
		hour = FskStrToNum(timecode);
		min = FskStrToNum(c1);
		sec = FskStrToNum(c2);
	} else {
		day = FskStrToNum(timecode);
		hour = FskStrToNum(c1);
		min = FskStrToNum(c2);
		sec = FskStrToNum(c3);
	}

	dot = FskStrChr(timecode, '.');
	if (dot) {
		ms = FskStrToNum(dot + 1);
	}
	
	time = day * 24 * 60 * 60 + hour * 60 * 60 + min * 60 + sec;
	time += (double) ms / 1000.0;
	return time;
}

void KprUPnPControllerUtility(KprUPnPController self, char* actionName)
{
#if TARGET_OS_IPHONE
	KprUPnPControllerUtility_iOS(self, actionName);
#endif
}

void UPnP_Controller_invokeAction(xsMachine *the)
{
	KprUPnP self = gUPnP;
	char* uuid = xsToString(xsArg(0));
	KprUPnPController controller = NULL;
	char* serviceType = xsToString(xsArg(1));
	char* actionName = xsToString(xsArg(2));

	xsTry {
		xsAssert(self);
		xsAssert(uuid);
		controller = FskAssociativeArrayElementGetReference(self->controllers, uuid);
		xsAssert(controller);
		xsAssert(!controller->removed);
		xsThrowIfFskErr(KprUPnPControllerServiceActionInvoke(controller, serviceType, actionName));
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void UPnP_Controller_isAction(xsMachine *the)
{
	KprUPnP self = gUPnP;
	char* uuid = xsToString(xsArg(0));
	KprUPnPController controller = NULL;
	char* serviceType = xsToString(xsArg(1));
	char* actionName = xsToString(xsArg(2));
	Boolean isAction = false;

	xsTry {
		xsAssert(self);
		xsAssert(uuid);
		controller = FskAssociativeArrayElementGetReference(self->controllers, uuid);
		xsAssert(controller);
		xsAssert(!controller->removed);
		isAction = KprUPnPControllerServiceIsAction(controller, serviceType, actionName);
	}
	xsCatch {
		xsThrow(xsException);
	}
	xsResult = xsBoolean(isAction);
}

void UPnP_Controller_utility(xsMachine *the)
{
	KprUPnP self = gUPnP;
	char* uuid = xsToString(xsArg(0));
	KprUPnPController controller = NULL;
	char* actionName = xsToString(xsArg(1));

	xsTry {
		xsAssert(self);
		xsAssert(uuid);
		controller = FskAssociativeArrayElementGetReference(self->controllers, uuid);
		xsAssert(controller);
		xsAssert(!controller->removed);
		KprUPnPControllerUtility(controller, actionName);
	}
	xsCatch {
		xsThrow(xsException);
	}
}

