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
#include "kprHome.h"
#include "xs.h"
#include "FskExtensions.h"

static FskErr extractHostData(xsMachine *the, xsSlot object, char *name, void **hostData);

#define isObject(x) xsIsInstanceOf(x, xsObjectPrototype)
#define isString(x) xsIsInstanceOf(x, xsStringPrototype)
#define isChunk(x) xsIsInstanceOf(x, xsChunkPrototype)
#define isFunc(x) xsIsInstanceOf(x, xsFunctionPrototype)
#define hasCallback(x) xsFindResult(env->slot, xsID(x)) && isFunc(xsResult)


/* ================== */

void KPR_Home_Manager(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeManager self = NULL;

	bailIfError(KprHomeManagerNew(&self, the, the->code, xsThis));

	if (!xsTest(xsArg(0))) {
		xsNewHostProperty(xsThis, xsID_behavior, xsThis, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	} else {
		xsNewHostProperty(xsThis, xsID_behavior, xsArg(0), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	}

	xsSetHostData(xsThis, self);

bail:
	if (err) KprHomeManagerDispose(self);
	xsThrowIfFskErr(err);
}

void KPR_Home_manager_destructor(void *it)
{
	KprHomeManagerDispose(it);
}

void KPR_Home_manager_get_homes(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeManager manager = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeManagerGetHomes(manager, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_manager_get_primaryHome(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeManager manager = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeManagerGetPrimaryHome(manager, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_manager_is_ready(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeManager manager = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeManagerIsReady(manager, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_manager_get_browser(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeManager manager = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeManagerGetBrowser(manager, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_manager_addHome(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeManager manager = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeManagerAddHome(manager, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_manager_removeHome(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeManager manager = xsGetHostData(xsThis);
	KprHome home;

	bailIfError(extractHostData(the, xsArg(0), "home", (void**) &home));
	bailIfError(KprHomeManagerRemoveHome(manager, home, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_manager_setPrimaryHome(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeManager manager = xsGetHostData(xsThis);
	KprHome home;

	bailIfError(extractHostData(the, xsArg(0), "home", (void**) &home));
	bailIfError(KprHomeManagerSetPrimaryHome(manager, home, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_browser_destructor(void *it)
{
	KprHomeBrowserDispose(it);
}

void KPR_Home_browser_get_accessories(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeBrowser browser = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeBrowserGetAccessories(browser, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_browser_start(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeBrowser browser = xsGetHostData(xsThis);

	bailIfError(KprHomeBrowserStart(browser));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_browser_stop(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeBrowser browser = xsGetHostData(xsThis);

	bailIfError(KprHomeBrowserStop(browser));

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_home_destructor(void *it)
{
	KprHomeDispose(it);
}

void KPR_Home_home_get_name(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeGetName(home, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_updateName(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeUpdateName(home, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_is_primary(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeIsPrimary(home, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_get_accessories(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeGetAccessories(home, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_addAccessory(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);
	KprHomeAccessory accessory;

	bailIfError(extractHostData(the, xsArg(0), "accessory", (void**) &accessory));
	bailIfError(KprHomeAddAccessory(home, accessory, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_removeAccessory(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);
	KprHomeAccessory accessory;

	bailIfError(extractHostData(the, xsArg(0), "accessory", (void**) &accessory));
	bailIfError(KprHomeRemoveAccessory(home, accessory, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_assignAccessory(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);
	KprHomeAccessory accessory;
	KprHomeRoom room;

	bailIfError(extractHostData(the, xsArg(0), "accessory", (void**) &accessory));
	bailIfError(extractHostData(the, xsArg(1), "room", (void**) &room));
	bailIfError(KprHomeAssignAccessory(home, accessory, room, xsArg(2)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_queryServices(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	xsEnterSandbox();
	bailIfError(KprHomeQueryServices(home, xsArg(0), the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_unblockAccessory(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);
	KprHomeAccessory accessory;

	bailIfError(extractHostData(the, xsArg(0), "accessory", (void**) &accessory));
	bailIfError(KprHomeUnblockAccessory(home, accessory, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_get_users(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeGetUsers(home, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_addUser(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	bailIfError(KprHomeAddUser(home, xsArg(0)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_removeUser(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);
	KprHomeUser user;

	bailIfError(extractHostData(the, xsArg(0), "user", (void**) &user));
	bailIfError(KprHomeRemoveUser(home, user, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_get_rooms(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeGetRooms(home, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_addRoom(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeAddRoom(home, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_removeRoom(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);
	KprHomeRoom room;

	bailIfError(extractHostData(the, xsArg(0), "room", (void**) &room));
	bailIfError(KprHomeRemoveRoom(home, room, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_get_zones(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeGetZones(home, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_addZone(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeAddZone(home, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_removeZone(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);
	KprHomeZone zone;

	bailIfError(extractHostData(the, xsArg(0), "zone", (void**) &zone));
	bailIfError(KprHomeRemoveZone(home, zone, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_get_serviceGroups(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeGetServiceGroups(home, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_addServiceGroup(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeAddServiceGroup(home, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_removeServiceGroup(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);
	KprHomeServiceGroup serviceGroup;

	bailIfError(extractHostData(the, xsArg(0), "serviceGroup", (void**) &serviceGroup));
	bailIfError(KprHomeRemoveServiceGroup(home, serviceGroup, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_get_actionSets(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeGetActionSets(home, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_addActionSet(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeAddActionSet(home, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_removeActionSet(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);
	KprHomeActionSet actionSet;

	bailIfError(extractHostData(the, xsArg(0), "actionSet", (void**) &actionSet));
	bailIfError(KprHomeRemoveActionSet(home, actionSet, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_executeActionSet(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);
	KprHomeActionSet actionSet;

	bailIfError(extractHostData(the, xsArg(0), "actionSet", (void**) &actionSet));
	bailIfError(KprHomeExecuteActionSet(home, actionSet, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_get_triggers(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeGetTriggers(home, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_addTimerTrigger(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);
	if (!xsIsInstanceOf(xsArg(1), xsDatePrototype)) bailIfError(kFskErrInvalidParameter);
	if (!xsIsInstanceOf(xsArg(2), xsObjectPrototype)) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeAddTimerTrigger(home, xsToString(xsArg(0)), xsArg(1), xsArg(2), xsArg(3)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_home_removeTrigger(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = xsGetHostData(xsThis);
	KprHomeTimerTrigger trigger;

	bailIfError(extractHostData(the, xsArg(0), "timerTrigger", (void**) &trigger));
	bailIfError(KprHomeRemoveTrigger(home, trigger, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_accessory_destructor(void *it)
{
	KprHomeAccessoryDispose(it);
}

void KPR_Home_accessory_get_name(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeAccessoryGetName(accessory, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_accessory_updateName(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeAccessoryUpdateName(accessory, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_accessory_get_identifier(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeAccessoryGetIdentifier(accessory, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_accessory_is_reachable(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeAccessoryIsReachabe(accessory, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_accessory_is_standalone(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeAccessoryIsStandalone(accessory, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_accessory_is_bridged(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeAccessoryIsBridged(accessory, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_accessory_is_bridge(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeAccessoryIsBridge(accessory, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_accessory_get_bridgedAccessoryIdentifiers(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeAccessoryGetBridgedAccessoryIdentifiers(accessory, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_accessory_get_room(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeAccessoryGetRoom(accessory, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_accessory_get_services(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeAccessoryGetServices(accessory, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_accessory_is_blocked(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeAccessoryIsBlocked(accessory, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_accessory_identify(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = xsGetHostData(xsThis);

	bailIfError(KprHomeAccessoryIdentify(accessory, xsArg(0)));

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_service_destructor(void *it)
{
	KprHomeServiceDispose(it);
}

void KPR_Home_service_get_accessory(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeService service = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeServiceGetAccessory(service, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_service_get_type(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeService service = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeServiceGetType(service, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_service_get_name(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeService service = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeServiceGetName(service, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_service_updateName(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeService service = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeServiceUpdateName(service, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_service_get_associatedType(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeService service = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeServiceGetAssociatedType(service, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_service_updateAssociatedType(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeService service = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeServiceUpdateAssociatedType(service, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_service_get_characteristics(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeService service = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeServiceGetCharacteristics(service, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_characteristic_destructor(void *it)
{
	KprHomeCharacteristicDispose(it);
}

void KPR_Home_characteristic_get_hash(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicGetHash(characteristic, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_get_service(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicGetService(characteristic, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_get_type(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicGetType(characteristic, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_get_properties(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicGetProperties(characteristic, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_is_readable(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicIsReadable(characteristic, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_is_writable(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicIsWritable(characteristic, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_is_supportsEventNotification(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicSupportsEventNotification(characteristic, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_get_metadata(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicGetMetadata(characteristic, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_get_value(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicGetValue(characteristic, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_writeValue(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	bailIfError(KprHomeCharacteristicWriteValue(characteristic, xsArg(0), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_readValue(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	bailIfError(KprHomeCharacteristicReadValue(characteristic, xsArg(0)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_is_notificationEnabled(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicIsNotificationEnabled(characteristic, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_enableNotification(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	bailIfError(KprHomeCharacteristicSetEnableNotification(characteristic, true, xsArg(0)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristic_disableNotification(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = xsGetHostData(xsThis);

	bailIfError(KprHomeCharacteristicSetEnableNotification(characteristic, false, xsArg(0)));

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_characteristicMetadata_destructor(void *it)
{
	KprHomeMetadataDispose(it);
}

void KPR_Home_characteristicMetadata_get_minimumValue(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeMetadata metadata = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeMetadataGetMinimumValue(metadata, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristicMetadata_get_maximumValue(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeMetadata metadata = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeMetadataGetMaximumValue(metadata, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristicMetadata_get_stepValue(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeMetadata metadata = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeMetadataGetStepValue(metadata, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristicMetadata_get_maxLength(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeMetadata metadata = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeMetadataGetMaxLength(metadata, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristicMetadata_get_format(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeMetadata metadata = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeMetadataGetFormat(metadata, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristicMetadata_get_units(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeMetadata metadata = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeMetadataGetUnits(metadata, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristicMetadata_get_manufacturerDescription(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeMetadata metadata = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeMetadataGetManufacturerDescription(metadata, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_room_destructor(void *it)
{
	KprHomeRoomDispose(it);
}

void KPR_Home_room_get_name(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeRoom room = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeRoomGetName(room, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_room_updateName(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeRoom room = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeRoomUpdateName(room, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_room_get_accessories(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeRoom room = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeRoomGetAccessories(room, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_zone_destructor(void *it)
{
	KprHomeZoneDispose(it);
}

void KPR_Home_zone_get_name(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeZone zone = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeZoneGetName(zone, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_zone_updateName(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeZone zone = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeZoneUpdateName(zone, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_zone_get_rooms(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeZone zone = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeZoneGetRooms(zone, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_zone_addRoom(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeZone zone = xsGetHostData(xsThis);
	KprHomeRoom room;

	bailIfError(extractHostData(the, xsArg(0), "room", (void**) &room));
	bailIfError(KprHomeZoneAddRoom(zone, room, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_zone_removeRoom(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeZone zone = xsGetHostData(xsThis);
	KprHomeRoom room;

	bailIfError(extractHostData(the, xsArg(0), "room", (void**) &room));
	bailIfError(KprHomeZoneRemoveRoom(zone, room, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_serviceGroup_destructor(void *it)
{
	KprHomeServiceGroupDispose(it);
}

void KPR_Home_serviceGroup_get_name(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeServiceGroup serviceGroup = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeServiceGroupGetName(serviceGroup, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_serviceGroup_updateName(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeServiceGroup serviceGroup = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeServiceGroupUpdateName(serviceGroup, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_serviceGroup_get_services(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeServiceGroup serviceGroup = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeServiceGroupGetServices(serviceGroup, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_serviceGroup_addService(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeServiceGroup serviceGroup = xsGetHostData(xsThis);
	KprHomeService service;

	bailIfError(extractHostData(the, xsArg(0), "service", (void**) &service));
	bailIfError(KprHomeServiceGroupAddService(serviceGroup, service, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_serviceGroup_removeService(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeServiceGroup serviceGroup = xsGetHostData(xsThis);
	KprHomeService service;

	bailIfError(extractHostData(the, xsArg(0), "service", (void**) &service));
	bailIfError(KprHomeServiceGroupAddService(serviceGroup, service, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_actionSet_destructor(void *it)
{
	KprHomeActionSetDispose(it);
}

void KPR_Home_actionSet_get_name(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeActionSet actionSet = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeActionSetGetName(actionSet, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_actionSet_updateName(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeActionSet actionSet = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeActionSetUpdateName(actionSet, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_actionSet_get_actions(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeActionSet actionSet = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeActionSetGetActions(actionSet, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_actionSet_addCharacteristicWriteAction(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeActionSet actionSet = xsGetHostData(xsThis);
	KprHomeCharacteristic characteristic;

	bailIfError(extractHostData(the, xsArg(0), "characteristic", (void**) &characteristic));
	bailIfError(KprHomeActionSetAddCharacteristicWriteAction(actionSet, characteristic, xsArg(1), xsArg(2)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_actionSet_removeAction(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeActionSet actionSet = xsGetHostData(xsThis);
	KprHomeCharacteristicWriteAction action;

	bailIfError(extractHostData(the, xsArg(0), "characteristicWriteAction", (void**) &action));
	bailIfError(KprHomeActionSetRemoveAction(actionSet, action, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_actionSet_is_executing(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeActionSet actionSet = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeActionSetIsExecuting(actionSet, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_characteristicWriteAction_destructor(void *it)
{
	KprHomeCharacteristicWriteActionDispose(it);
}

void KPR_Home_characteristicWriteAction_get_characteristic(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristicWriteAction action = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicWriteActionGetCharacteristic(action, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristicWriteAction_get_targetValue(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristicWriteAction action = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeCharacteristicWriteActionGetTargetValue(action, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_characteristicWriteAction_updateTargetValue(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristicWriteAction action = xsGetHostData(xsThis);

	bailIfError(KprHomeCharacteristicWriteActionUpdateTargetValue(action, xsArg(0), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_timerTrigger_destructor(void *it)
{
	KprHomeTimerTriggerDispose(it);
}

void KPR_Home_timerTrigger_get_name(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeTimerTriggerGetName(trigger, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_updateName(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeTimerTriggerUpdateName(trigger, xsToString(xsArg(0)), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_is_enabled(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeTimerTriggerIsEnabled(trigger, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_enable(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);

	bailIfError(KprHomeTimerTriggerSetEnable(trigger, true, xsArg(0)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_disable(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);

	bailIfError(KprHomeTimerTriggerSetEnable(trigger, false, xsArg(0)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_get_actionSets(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeTimerTriggerGetActionSets(trigger, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_addActionSet(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);
	KprHomeActionSet actionSet;

	bailIfError(extractHostData(the, xsArg(0), "actionSet", (void**) &actionSet));
	bailIfError(KprHomeTimerTriggerAddActionSet(trigger, actionSet, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_removeActionSet(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);
	KprHomeActionSet actionSet;

	bailIfError(extractHostData(the, xsArg(0), "actionSet", (void**) &actionSet));
	bailIfError(KprHomeTimerTriggerRemoveActionSet(trigger, actionSet, xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_get_fireDate(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeTimerTriggerGetFireDate(trigger, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_get_recurrence(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeTimerTriggerGetRecurrence(trigger, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_get_lastFireDate(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeTimerTriggerGetLastFireDate(trigger, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_updateFireDate(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeTimerTriggerUpdateFireDate(trigger, xsArg(0), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Home_timerTrigger_updateRecurrence(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = xsGetHostData(xsThis);

	if (!xsTest(xsArg(0))) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprHomeTimerTriggerUpdateRecurrence(trigger, xsArg(0), xsArg(1)));

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

void KPR_Home_user_destructor(void *it)
{
	KprHomeUserDispose(it);
}

void KPR_Home_user_get_name(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeUser user = xsGetHostData(xsThis);

	xsEnterSandbox();
	bailIfError(KprHomeUserGetName(user, the));
	xsLeaveSandbox();

bail:
	xsThrowIfFskErr(err);
}

/* ================== */

static xsSlot getHome(xsMachine *the)
{
	return xsGet(xsGlobal, xsID("Home"));
}

static xsSlot getPrototype(xsMachine *the, char *name)
{
	return xsGet(getHome(the), xsID(name));
}

static FskErr extractHostData(xsMachine *the, xsSlot object, char *name, void **hostData)
{
	if (!xsTest(object)) return kFskErrInvalidParameter;
	if (!xsIsInstanceOf(object, getPrototype(the, name))) return kFskErrInvalidParameter;
	*hostData = xsGetHostData(object);
	return kFskErrNone;
}

#if defined(RUN_UNITTEST) && RUN_UNITTEST

/* ==================
 * UNIT TEST
 * ================== */

#include "kunit.h"

//ku_test(PointerArray);

ku_main();

static void KPR_Home_test() {
	ku_begin();
//	ku_run(PointerArray);
	ku_finish();
}

#endif

/* ================== */

FskErr kprHome_fskLoad(FskLibrary library)
{
#if defined(RUN_UNITTEST) && RUN_UNITTEST
	KPR_Home_test();
#endif
	return kFskErrNone;
}

FskErr kprHome_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

