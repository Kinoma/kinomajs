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
#include "kprBehavior.h"
#include "kprHandler.h"
#include "kprHTTPServer.h"
#include "kprMessage.h"

#include "FskUUID.h"

#include "kprSSDPCommon.h"
#include "kprSSDPClient.h"
#include "kprSSDPServer.h"

static Boolean KprSSDPServiceAccept(KprService self, KprMessage message);
static void KprSSDPServiceStart(KprService service, FskThread thread, xsMachine* the);
static void KprSSDPServiceStop(KprService service);
static void KprSSDPServiceDiscover(KprService self, char* authority, char* id, Boolean useEnvironment);
static void KprSSDPServiceForget(KprService self, char* authority, char* id);
static void KprSSDPServiceShare(KprService self, char* authority, Boolean shareIt, Boolean useEnvironment);

static KprServiceRecord gSSDPService = {
	NULL,
	kprServicesThread,
	"ssdp:",
	NULL,
	NULL,
	KprSSDPServiceAccept,
	KprServiceCancel,
	KprServiceInvoke,
	KprSSDPServiceStart,
	KprSSDPServiceStop,
	KprSSDPServiceDiscover,
	KprSSDPServiceForget,
	KprSSDPServiceShare
};

#if 0
#pragma mark - extension
#endif

FskExport(FskErr) kprSSDP_fskLoad(FskLibrary library)
{
	KprServiceRegister(&gSSDPService);
	return kFskErrNone;
}


FskExport(FskErr) kprSSDP_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

#if 0
#pragma mark - KprSSDPService
#endif

Boolean KprSSDPServiceAccept(KprService self UNUSED, KprMessage message UNUSED)
{
	return false;
}

void KprSSDPServiceStart(KprService service, FskThread thread, xsMachine* the)
{
	service->machine = the;
	service->thread = thread;
	KprSSDPStart(service, thread, the);
}

void KprSSDPServiceStop(KprService service)
{
	KprSSDPStop(service);
}

void KprSSDPServiceDiscover(KprService self, char* authority, char* id, Boolean useEnvironment)
{
	if (useEnvironment ? KprEnvironmentGetUInt32("useSSDP", 0) : true)
		KprSSDPDiscoverServer(authority, id);
	FskMemPtrDispose(authority);
	FskMemPtrDispose(id);
	return;
}

void KprSSDPServiceForget(KprService self, char* authority, char* id)
{
	KprSSDPForgetServer(authority, id);
	FskMemPtrDispose(authority);
	FskMemPtrDispose(id);
	return;
}

void KprSSDPServiceShare(KprService self, char* authority, Boolean shareIt, Boolean useEnvironment)
{
	FskErr err = kFskErrNone;
	KprSSDPDevice device = NULL;
	KprHTTPServer server = KprHTTPServerGet(authority);
	char* uuid = FskUUIDGetForKey(authority);

	if (shareIt && useEnvironment)
		shareIt = KprEnvironmentGetUInt32("useSSDP", 0);
	if (shareIt && server) {
		if (!KprSSDPGetDevice(uuid)) {
			char type[256];
			snprintf(type, sizeof(type), kKPRSSDPKinomaServe, authority);
			bailIfError(KprSSDPDeviceNew(&device, KprHTTPServerIsSecure(server) ? "https" : "http", KprHTTPServerGetPort(server), "/", 1800, -1, uuid, type, NULL));
			KprSSDPAddDevice(device);
		}
	}
	else {
		KprSSDPRemoveDevice(FskStrDoCopy(uuid));
	}
bail:
	FskMemPtrDispose(authority);
	return;
}

#if 0
#pragma mark - xs
#endif

void SSDP_addService(xsMachine *the)
{
	KprSSDPCommon self = xsGetHostData(xsThis);
	xsThrowIfFskErr(KprSSDPCommonAddService(self, xsToString(xsArg(0))));
}

void SSDP_get_behavior(xsMachine *the)
{
	KprSSDPCommon self = xsGetHostData(xsThis);
	if (xsTypeOf(self->behavior) != xsUndefinedType)
		xsResult = self->behavior;
	else
		xsResult = xsUndefined;
}

void SSDP_get_type(xsMachine *the)
{
	KprSSDPCommon self = xsGetHostData(xsThis);
	xsResult = xsString(self->type);
}

void SSDP_set_behavior(xsMachine *the)
{
	KprSSDPCommon self = xsGetHostData(xsThis);
//	if (xsTypeOf(self->behavior) != xsUndefinedType)
		xsForget(self->behavior);
	if (xsTest(xsArg(0))) {
		if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
			self->behavior = xsArg(0);
			xsRemember(self->behavior);
		}
	}
}

// Server

void SSDP_server_registeredCallback(KprSSDPServer self);
void SSDP_server_unregisteredCallback(KprSSDPServer self);

void SSDP_Server(xsMachine *the)
{
	KprSSDPServer self = NULL;
	char* type = xsToString(xsArg(0));
	UInt32 port = xsToInteger(xsArg(1));
	char* path = xsToString(xsArg(2));
	UInt32 expire = xsToInteger(xsArg(3));
	char* uuid = FskUUIDGetForKey(type);
	char* scheme = "http";
	if ((xsToInteger(xsArgc) > 4) && !xsIsInstanceOf(xsArg(4), xsObjectPrototype)) {
		scheme = xsToString(xsArg(4));
	}
	FskDebugStr("%s", __FUNCTION__);

	{
		xsTry {
			xsThrowIfFskErr(KprSSDPServerNew(&self, scheme, port, path, expire, uuid, type));
			xsSetHostData(xsResult, self);
			self->registeredCallback = SSDP_server_registeredCallback;
			self->unregisteredCallback = SSDP_server_unregisteredCallback;
			self->the = the;
			self->slot = xsResult;
			self->code = the->code;
			self->behavior = xsUndefined;
			xsRemember(self->slot);
		}
		xsCatch {
		}
	}
}

void SSDP_server(void *it)
{
	KprSSDPServer self = it;
	if (self) {
		xsMachine *the = self->the;
		xsForget(self->slot);
		xsForget(self->behavior);
		KprSSDPServerDispose(self);
	}
}

void SSDP_server_callback(KprSSDPServer self, char* function)
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

void SSDP_server_get_path(xsMachine *the)
{
	KprSSDPServer self = xsGetHostData(xsThis);
	xsResult = xsString(self->path);
}

void SSDP_server_get_port(xsMachine *the)
{
	KprSSDPServer self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->port);
}

void SSDP_server_get_uuid(xsMachine *the)
{
	KprSSDPServer self = xsGetHostData(xsThis);
	xsResult = xsString(self->uuid);
}

void SSDP_server_registeredCallback(KprSSDPServer self)
{
	SSDP_server_callback(self, "onSSDPServerRegistered");
}

void SSDP_server_unregisteredCallback(KprSSDPServer self)
{
	SSDP_server_callback(self, "onSSDPServerUnregistered");
}

void SSDP_server_start(xsMachine *the)
{
	KprSSDPServer self = xsGetHostData(xsThis);
	xsThrowIfFskErr(KprSSDPServerStart(self));
}

void SSDP_server_stop(xsMachine *the)
{
	KprSSDPServer self = xsGetHostData(xsThis);
	xsThrowIfFskErr(KprSSDPServerStop(self));
}

// client

void SSDP_client_addServerCallback(KprSSDPClient self, KprSSDPDiscoveryDescription description);
void SSDP_client_removeServerCallback(KprSSDPClient self, KprSSDPDiscoveryDescription description);

void SSDP_Client(xsMachine *the)
{
	KprSSDPClient self = NULL;
	if ((xsToInteger(xsArgc) > 0) && xsTest(xsArg(0)))
		xsThrowIfFskErr(KprSSDPClientNew(&self, xsToString(xsArg(0))));
	else
		xsThrowIfFskErr(KprSSDPClientNew(&self, NULL));
	xsSetHostData(xsResult, self);
	self->addServerCallback = SSDP_client_addServerCallback;
	self->removeServerCallback = SSDP_client_removeServerCallback;
	self->the = the;
	self->slot = xsResult;
	self->code = the->code;
	self->behavior = xsUndefined;
	xsRemember(self->slot);
}

void SSDP_client(void *it)
{
	KprSSDPClient self = it;
	if (self) {
		xsMachine *the = self->the;
		xsForget(self->slot);
		xsForget(self->behavior);
		KprSSDPClientDispose(self);
	}
}

void SSDP_client_callback(KprSSDPClient self, char* function, KprSSDPDiscoveryDescription description)
{
	xsBeginHostSandboxCode(self->the, self->code);
	if (xsTypeOf(self->behavior) == xsUndefinedType) goto bail;
	xsVars(2);
	{
		xsTry {
			xsVar(0) = xsAccess(self->behavior);
			xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
			xsSet(xsVar(1), xsID("type"), xsString(description->type ? description->type : "undefined"));
			xsSet(xsVar(1), xsID("uuid"), xsString(description->uuid));
			xsSet(xsVar(1), xsID("url"), xsString(description->url));
			xsSet(xsVar(1), xsID("interface"), xsString(description->ip));
			xsSet(xsVar(1), xsID("interfaceName"), xsString(description->interfaceName));
			xsResult = xsNewInstanceOf(xsArrayPrototype);
			if (description->services) {
				KprSSDPService service;
				for (service = description->services; service; service = service->next) {
					(void)xsCall1(xsResult, xsID_push, xsString(service->type));
				}
			}
			xsSet(xsVar(1), xsID("services"), xsResult);
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

void SSDP_client_addServerCallback(KprSSDPClient self, KprSSDPDiscoveryDescription description)
{
	SSDP_client_callback(self, "onSSDPServerUp", description);
}

void SSDP_client_removeServerCallback(KprSSDPClient self, KprSSDPDiscoveryDescription description)
{
	SSDP_client_callback(self, "onSSDPServerDown", description);
}

void SSDP_client_remove(xsMachine *the)
{
	KprSSDPClient self = xsGetHostData(xsThis);
	xsThrowIfFskErr(KprSSDPClientRemove(self, xsToString(xsArg(0))));
}

void SSDP_client_search(xsMachine *the)
{
	KprSSDPClient self = xsGetHostData(xsThis);
	xsThrowIfFskErr(KprSSDPClientSearch(self));
}

void SSDP_client_start(xsMachine *the)
{
	KprSSDPClient self = xsGetHostData(xsThis);
	xsThrowIfFskErr(KprSSDPClientStart(self));
}

void SSDP_client_stop(xsMachine *the)
{
	KprSSDPClient self = xsGetHostData(xsThis);
	xsThrowIfFskErr(KprSSDPClientStop(self));
}
