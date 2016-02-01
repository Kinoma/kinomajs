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
#ifndef __KPRHOME__
#define __KPRHOME__

#include "kpr.h"
#include "xs.h"

//--------------------------------------------------
// Typedefs
//--------------------------------------------------

typedef struct KprHomeManagerRecord KprHomeManagerRecord, *KprHomeManager;
typedef struct KprHomeBrowserRecord KprHomeBrowserRecord, *KprHomeBrowser;
typedef struct KprHomeRecord KprHomeRecord, *KprHome;
typedef struct KprHomeAccessoryRecord KprHomeAccessoryRecord, *KprHomeAccessory;
typedef struct KprHomeServiceRecord KprHomeServiceRecord, *KprHomeService;
typedef struct KprHomeCharacteristicRecord KprHomeCharacteristicRecord, *KprHomeCharacteristic;
typedef struct KprHomeMetadataRecord KprHomeMetadataRecord, *KprHomeMetadata;
typedef struct KprHomeRoomRecord KprHomeRoomRecord, *KprHomeRoom;
typedef struct KprHomeZoneRecord KprHomeZoneRecord, *KprHomeZone;
typedef struct KprHomeServiceGroupRecord KprHomeServiceGroupRecord, *KprHomeServiceGroup;
typedef struct KprHomeActionSetRecord KprHomeActionSetRecord, *KprHomeActionSet;
typedef struct KprHomeCharacteristicWriteActionRecord KprHomeCharacteristicWriteActionRecord, *KprHomeCharacteristicWriteAction;
typedef struct KprHomeTimerTriggerRecord KprHomeTimerTriggerRecord, *KprHomeTimerTrigger;
typedef struct KprHomeUserRecord KprHomeUserRecord, *KprHomeUser;

typedef struct KPR_Home_Error KPR_Home_Error;

struct KPR_Home_Error {
	SInt32 code;
	char *message;
};

//--------------------------------------------------
// Manager
//--------------------------------------------------

FskErr KprHomeManagerNew(KprHomeManager *it, xsMachine *the, xsIndex *code, xsSlot this);
FskErr KprHomeManagerDispose(KprHomeManager manager);
FskErr KprHomeManagerDisposeAt(KprHomeManager *it);

FskErr KprHomeManagerGetHomes(KprHomeManager manager, xsMachine *the); // @xsResult = [KPR_Home_Home]
FskErr KprHomeManagerGetPrimaryHome(KprHomeManager manager, xsMachine *the); // @xsResult = KPR_Home_Home
FskErr KprHomeManagerIsReady(KprHomeManager manager, xsMachine *the);
FskErr KprHomeManagerGetBrowser(KprHomeManager manager, xsMachine *the); // @xsResult = KPR_Home_Browser
FskErr KprHomeManagerAddHome(KprHomeManager manager, const char *name, xsSlot completion);
FskErr KprHomeManagerRemoveHome(KprHomeManager manager, KprHome home, xsSlot completion);
FskErr KprHomeManagerSetPrimaryHome(KprHomeManager manager, KprHome home, xsSlot completion);

//--------------------------------------------------
// Browser
//--------------------------------------------------

FskErr KprHomeBrowserDispose(KprHomeBrowser browser);

FskErr KprHomeBrowserGetAccessories(KprHomeBrowser browser, xsMachine *the); // @xsResult = [KPR_Home_Accessory]
FskErr KprHomeBrowserStart(KprHomeBrowser browser);
FskErr KprHomeBrowserStop(KprHomeBrowser browser);

//--------------------------------------------------
// Home
//--------------------------------------------------

FskErr KprHomeDispose(KprHome home);

FskErr KprHomeGetName(KprHome home, xsMachine *the); // @xsResult = String
FskErr KprHomeUpdateName(KprHome home, const char *name, xsSlot completion);
FskErr KprHomeIsPrimary(KprHome home, xsMachine *the); // @xsResult = Boolean

FskErr KprHomeGetAccessories(KprHome home, xsMachine *the); // @xsResult = [KPR_Home_Accessory]
FskErr KprHomeAddAccessory(KprHome home, KprHomeAccessory accessory, xsSlot completion);
FskErr KprHomeRemoveAccessory(KprHome home, KprHomeAccessory accessory, xsSlot completion);
FskErr KprHomeAssignAccessory(KprHome home, KprHomeAccessory accessory, KprHomeRoom room, xsSlot completion);
FskErr KprHomeQueryServices(KprHome home, xsSlot types, xsMachine *the); // @xsResult = [KPR_Home_Service]
FskErr KprHomeUnblockAccessory(KprHome home, KprHomeAccessory accessory, xsSlot completion);

FskErr KprHomeGetUsers(KprHome home, xsMachine *the);
FskErr KprHomeAddUser(KprHome home, xsSlot completion);
FskErr KprHomeRemoveUser(KprHome home, KprHomeUser user, xsSlot completion);

FskErr KprHomeGetRooms(KprHome home, xsMachine *the);
FskErr KprHomeGetRoomForEntireHome(KprHome home, xsMachine *the);
FskErr KprHomeAddRoom(KprHome home, const char *name, xsSlot completion);
FskErr KprHomeRemoveRoom(KprHome home, KprHomeRoom room, xsSlot completion);

FskErr KprHomeGetZones(KprHome home, xsMachine *the);
FskErr KprHomeAddZone(KprHome home, const char *name, xsSlot completion);
FskErr KprHomeRemoveZone(KprHome home, KprHomeZone zone, xsSlot completion);

FskErr KprHomeGetServiceGroups(KprHome home, xsMachine *the);
FskErr KprHomeAddServiceGroup(KprHome home, const char *name, xsSlot completion);
FskErr KprHomeRemoveServiceGroup(KprHome home, KprHomeServiceGroup serviceGroup, xsSlot completion);

FskErr KprHomeGetActionSets(KprHome home, xsMachine *the);
FskErr KprHomeAddActionSet(KprHome home, const char *name, xsSlot completion);
FskErr KprHomeRemoveActionSet(KprHome home, KprHomeActionSet actionSet, xsSlot completion);
FskErr KprHomeExecuteActionSet(KprHome home, KprHomeActionSet actionSet, xsSlot completion);

FskErr KprHomeGetTriggers(KprHome home, xsMachine *the);
FskErr KprHomeAddTimerTrigger(KprHome self, const char *name, xsSlot date, xsSlot recurrence, xsSlot completion);
FskErr KprHomeRemoveTrigger(KprHome home, KprHomeTimerTrigger trigger, xsSlot completion);

//--------------------------------------------------
// Accessory
//--------------------------------------------------

FskErr KprHomeAccessoryDispose(KprHomeAccessory accessory);

FskErr KprHomeAccessoryGetName(KprHomeAccessory accessory, xsMachine *the);
FskErr KprHomeAccessoryUpdateName(KprHomeAccessory accessory, const char *name, xsSlot callback);

FskErr KprHomeAccessoryGetIdentifier(KprHomeAccessory accessory, xsMachine *the);

FskErr KprHomeAccessoryIsReachabe(KprHomeAccessory accessory, xsMachine *the);

FskErr KprHomeAccessoryIsStandalone(KprHomeAccessory accessory, xsMachine *the);
FskErr KprHomeAccessoryIsBridged(KprHomeAccessory accessory, xsMachine *the);
FskErr KprHomeAccessoryIsBridge(KprHomeAccessory accessory, xsMachine *the);
FskErr KprHomeAccessoryGetBridgedAccessoryIdentifiers(KprHomeAccessory accessory, xsMachine *the);

FskErr KprHomeAccessoryGetRoom(KprHomeAccessory accessory, xsMachine *the);
FskErr KprHomeAccessoryGetServices(KprHomeAccessory accessory, xsMachine *the);

FskErr KprHomeAccessoryIsBlocked(KprHomeAccessory accessory, xsMachine *the);

FskErr KprHomeAccessoryIdentify(KprHomeAccessory accessory, xsSlot callback);

//--------------------------------------------------
// Service
//--------------------------------------------------

FskErr KprHomeServiceDispose(KprHomeService service);

FskErr KprHomeServiceGetAccessory(KprHomeService service, xsMachine *the);
FskErr KprHomeServiceGetType(KprHomeService service, xsMachine *the);
FskErr KprHomeServiceGetName(KprHomeService service, xsMachine *the);
FskErr KprHomeServiceUpdateName(KprHomeService service, const char *name, xsSlot callback);

FskErr KprHomeServiceGetAssociatedType(KprHomeService service, xsMachine *the);
FskErr KprHomeServiceUpdateAssociatedType(KprHomeService service, const char *name, xsSlot callback);

FskErr KprHomeServiceGetCharacteristics(KprHomeService service, xsMachine *the);

//--------------------------------------------------
// Characteristic
//--------------------------------------------------

FskErr KprHomeCharacteristicDispose(KprHomeCharacteristic characteristic);

FskErr KprHomeCharacteristicGetHash(KprHomeCharacteristic characteristic, xsMachine *the);
FskErr KprHomeCharacteristicGetService(KprHomeCharacteristic characteristic, xsMachine *the);
FskErr KprHomeCharacteristicGetType(KprHomeCharacteristic characteristic, xsMachine *the);
FskErr KprHomeCharacteristicGetProperties(KprHomeCharacteristic characteristic, xsMachine *the);
FskErr KprHomeCharacteristicIsReadable(KprHomeCharacteristic characteristic, xsMachine *the);
FskErr KprHomeCharacteristicIsWritable(KprHomeCharacteristic characteristic, xsMachine *the);
FskErr KprHomeCharacteristicSupportsEventNotification(KprHomeCharacteristic characteristic, xsMachine *the);
FskErr KprHomeCharacteristicGetMetadata(KprHomeCharacteristic characteristic, xsMachine *the);

FskErr KprHomeCharacteristicGetValue(KprHomeCharacteristic characteristic, xsMachine *the);
FskErr KprHomeCharacteristicWriteValue(KprHomeCharacteristic characteristic, xsSlot value, xsSlot callback);
FskErr KprHomeCharacteristicReadValue(KprHomeCharacteristic characteristic, xsSlot callback);

FskErr KprHomeCharacteristicIsNotificationEnabled(KprHomeCharacteristic characteristic, xsMachine *the);
FskErr KprHomeCharacteristicSetEnableNotification(KprHomeCharacteristic characteristic, Boolean enable, xsSlot callback);

//--------------------------------------------------
// Characteristic Metadata
//--------------------------------------------------

FskErr KprHomeMetadataDispose(KprHomeMetadata metadata);

FskErr KprHomeMetadataGetMinimumValue(KprHomeMetadata metadata, xsMachine *the);
FskErr KprHomeMetadataGetMaximumValue(KprHomeMetadata metadata, xsMachine *the);
FskErr KprHomeMetadataGetStepValue(KprHomeMetadata metadata, xsMachine *the);
FskErr KprHomeMetadataGetMaxLength(KprHomeMetadata metadata, xsMachine *the);
FskErr KprHomeMetadataGetFormat(KprHomeMetadata metadata, xsMachine *the);
FskErr KprHomeMetadataGetUnits(KprHomeMetadata metadata, xsMachine *the);
FskErr KprHomeMetadataGetManufacturerDescription(KprHomeMetadata metadata, xsMachine *the);

//--------------------------------------------------
// Room
//--------------------------------------------------

FskErr KprHomeRoomDispose(KprHomeRoom room);

FskErr KprHomeRoomGetName(KprHomeRoom room, xsMachine *the);
FskErr KprHomeRoomUpdateName(KprHomeRoom room, const char *name, xsSlot callback);
FskErr KprHomeRoomGetAccessories(KprHomeRoom room, xsMachine *the);

//--------------------------------------------------
// Zone
//--------------------------------------------------

FskErr KprHomeZoneDispose(KprHomeZone zone);

FskErr KprHomeZoneGetName(KprHomeZone zone, xsMachine *the);
FskErr KprHomeZoneUpdateName(KprHomeZone zone, const char *name, xsSlot callback);
FskErr KprHomeZoneGetRooms(KprHomeZone zone, xsMachine *the);
FskErr KprHomeZoneAddRoom(KprHomeZone zone, KprHomeRoom room, xsSlot callback);
FskErr KprHomeZoneRemoveRoom(KprHomeZone zone, KprHomeRoom room, xsSlot callback);

//--------------------------------------------------
// Service Group
//--------------------------------------------------

FskErr KprHomeServiceGroupDispose(KprHomeServiceGroup serviceGroup);

FskErr KprHomeServiceGroupGetName(KprHomeServiceGroup serviceGroup, xsMachine *the);
FskErr KprHomeServiceGroupUpdateName(KprHomeServiceGroup serviceGroup, const char *name, xsSlot callback);
FskErr KprHomeServiceGroupGetServices(KprHomeServiceGroup serviceGroup, xsMachine *the);
FskErr KprHomeServiceGroupAddService(KprHomeServiceGroup serviceGroup, KprHomeService service, xsSlot callback);
FskErr KprHomeServiceGroupRemoveService(KprHomeServiceGroup serviceGroup, KprHomeService service, xsSlot callback);

//--------------------------------------------------
// Action Set
//--------------------------------------------------

FskErr KprHomeActionSetDispose(KprHomeActionSet actionSet);

FskErr KprHomeActionSetGetName(KprHomeActionSet actionSet, xsMachine *the);
FskErr KprHomeActionSetUpdateName(KprHomeActionSet actionSet, const char *name, xsSlot callback);
FskErr KprHomeActionSetGetActions(KprHomeActionSet actionSet, xsMachine *the);
FskErr KprHomeActionSetAddCharacteristicWriteAction(KprHomeActionSet actionSet, KprHomeCharacteristic characteristic, xsSlot value, xsSlot callback);
FskErr KprHomeActionSetRemoveAction(KprHomeActionSet actionSet, KprHomeCharacteristicWriteAction action, xsSlot callback);
FskErr KprHomeActionSetIsExecuting(KprHomeActionSet actionSet, xsMachine *the);

//--------------------------------------------------
// Characteristic Write Action
//--------------------------------------------------

FskErr KprHomeCharacteristicWriteActionNew(KprHomeCharacteristicWriteAction *it, xsMachine *the, xsSlot this, KprHomeCharacteristic characteristic, xsSlot value);
FskErr KprHomeCharacteristicWriteActionDispose(KprHomeCharacteristicWriteAction action);

FskErr KprHomeCharacteristicWriteActionGetCharacteristic(KprHomeCharacteristicWriteAction action, xsMachine *the);
FskErr KprHomeCharacteristicWriteActionGetTargetValue(KprHomeCharacteristicWriteAction action, xsMachine *the);
FskErr KprHomeCharacteristicWriteActionUpdateTargetValue(KprHomeCharacteristicWriteAction action, xsSlot value, xsSlot callback);

//--------------------------------------------------
// Timer Trigger
//--------------------------------------------------

FskErr KprHomeTimerTriggerNew(KprHomeTimerTrigger *it, xsMachine *the, xsSlot this);
FskErr KprHomeTimerTriggerDispose(KprHomeTimerTrigger trigger);

FskErr KprHomeTimerTriggerGetName(KprHomeTimerTrigger trigger, xsMachine *the);
FskErr KprHomeTimerTriggerUpdateName(KprHomeTimerTrigger trigger, const char *name, xsSlot callback);
FskErr KprHomeTimerTriggerIsEnabled(KprHomeTimerTrigger trigger, xsMachine *the);
FskErr KprHomeTimerTriggerSetEnable(KprHomeTimerTrigger trigger, Boolean enable, xsSlot callback);
FskErr KprHomeTimerTriggerGetActionSets(KprHomeTimerTrigger trigger, xsMachine *the);
FskErr KprHomeTimerTriggerAddActionSet(KprHomeTimerTrigger trigger, KprHomeActionSet actionSet, xsSlot callback);
FskErr KprHomeTimerTriggerRemoveActionSet(KprHomeTimerTrigger trigger, KprHomeActionSet actionSet, xsSlot callback);
FskErr KprHomeTimerTriggerGetFireDate(KprHomeTimerTrigger trigger, xsMachine *the);
FskErr KprHomeTimerTriggerGetRecurrence(KprHomeTimerTrigger trigger, xsMachine *the);
FskErr KprHomeTimerTriggerGetLastFireDate(KprHomeTimerTrigger trigger, xsMachine *the);
FskErr KprHomeTimerTriggerUpdateFireDate(KprHomeTimerTrigger trigger, xsSlot date, xsSlot callback);
FskErr KprHomeTimerTriggerUpdateRecurrence(KprHomeTimerTrigger trigger, xsSlot recurrence, xsSlot callback);

//--------------------------------------------------
// User
//--------------------------------------------------

FskErr KprHomeUserDispose(KprHomeUser user);

FskErr KprHomeUserGetName(KprHomeUser user, xsMachine *the);


//--------------------------------------------------
// HOME Script Interfaces
//--------------------------------------------------

void KPR_Home_Manager(xsMachine *the);
void KPR_Home_manager_destructor(void *it);
void KPR_Home_manager_get_homes(xsMachine *the);
void KPR_Home_manager_get_primaryHome(xsMachine *the);
void KPR_Home_manager_get_browser(xsMachine *the);
void KPR_Home_manager_addHome(xsMachine *the);
void KPR_Home_manager_removeHome(xsMachine *the);
void KPR_Home_manager_setPrimaryHome(xsMachine *the);

/* ================== */

void KPR_Home_browser_destructor(void *it);
void KPR_Home_browser_get_accessories(xsMachine *the);
void KPR_Home_browser_start(xsMachine *the);
void KPR_Home_browser_stop(xsMachine *the);

/* ================== */

void KPR_Home_home_destructor(void *it);
void KPR_Home_home_get_name(xsMachine *the);
void KPR_Home_home_updateName(xsMachine *the);
void KPR_Home_home_is_primary(xsMachine *the);
void KPR_Home_home_get_accessories(xsMachine *the);
void KPR_Home_home_addAccessory(xsMachine *the);
void KPR_Home_home_removeAccessory(xsMachine *the);
void KPR_Home_home_assignAccessory(xsMachine *the);
void KPR_Home_home_queryServices(xsMachine *the);
void KPR_Home_home_unblockAccessory(xsMachine *the);
void KPR_Home_home_get_users(xsMachine *the);
void KPR_Home_home_addUser(xsMachine *the);
void KPR_Home_home_removeUser(xsMachine *the);
void KPR_Home_home_get_rooms(xsMachine *the);
void KPR_Home_home_addRoom(xsMachine *the);
void KPR_Home_home_removeRoom(xsMachine *the);
void KPR_Home_home_get_zones(xsMachine *the);
void KPR_Home_home_addZone(xsMachine *the);
void KPR_Home_home_removeZone(xsMachine *the);
void KPR_Home_home_get_serviceGroups(xsMachine *the);
void KPR_Home_home_addServiceGroup(xsMachine *the);
void KPR_Home_home_removeServiceGroup(xsMachine *the);
void KPR_Home_home_get_actionSets(xsMachine *the);
void KPR_Home_home_addActionSet(xsMachine *the);
void KPR_Home_home_removeActionSet(xsMachine *the);
void KPR_Home_home_executeActionSet(xsMachine *the);
void KPR_Home_home_get_triggers(xsMachine *the);
void KPR_Home_home_addTimerTrigger(xsMachine *the);
void KPR_Home_home_removeTrigger(xsMachine *the);

/* ================== */

void KPR_Home_accessory_destructor(void *it);
void KPR_Home_accessory_get_name(xsMachine *the);
void KPR_Home_accessory_updateName(xsMachine *the);
void KPR_Home_accessory_get_identifier(xsMachine *the);
void KPR_Home_accessory_is_reachable(xsMachine *the);
void KPR_Home_accessory_is_standalone(xsMachine *the);
void KPR_Home_accessory_is_bridged(xsMachine *the);
void KPR_Home_accessory_is_bridge(xsMachine *the);
void KPR_Home_accessory_get_bridgedAccessoryIdentifiers(xsMachine *the);
void KPR_Home_accessory_get_room(xsMachine *the);
void KPR_Home_accessory_get_services(xsMachine *the);
void KPR_Home_accessory_is_blocked(xsMachine *the);
void KPR_Home_accessory_unblock(xsMachine *the);
void KPR_Home_accessory_identify(xsMachine *the);

/* ================== */

void KPR_Home_service_destructor(void *it);
void KPR_Home_service_get_accessory(xsMachine *the);
void KPR_Home_service_get_type(xsMachine *the);
void KPR_Home_service_get_name(xsMachine *the);
void KPR_Home_service_updateName(xsMachine *the);
void KPR_Home_service_get_associatedType(xsMachine *the);
void KPR_Home_service_updateAssociatedType(xsMachine *the);
void KPR_Home_service_get_characteristics(xsMachine *the);

/* ================== */

void KPR_Home_characteristic_destructor(void *it);
void KPR_Home_characteristic_get_hash(xsMachine *the);
void KPR_Home_characteristic_get_service(xsMachine *the);
void KPR_Home_characteristic_get_type(xsMachine *the);
void KPR_Home_characteristic_get_properties(xsMachine *the);
void KPR_Home_characteristic_get_metadata(xsMachine *the);
void KPR_Home_characteristic_get_value(xsMachine *the);
void KPR_Home_characteristic_writeValue(xsMachine *the);
void KPR_Home_characteristic_readValue(xsMachine *the);
void KPR_Home_characteristic_is_notificationEnabled(xsMachine *the);
void KPR_Home_characteristic_enableNotification(xsMachine *the);
void KPR_Home_characteristic_disableNotification(xsMachine *the);

/* ================== */

void KPR_Home_characteristicMetadata_destructor(void *it);
void KPR_Home_characteristicMetadata_get_minimumValue(xsMachine *the);
void KPR_Home_characteristicMetadata_get_maximumValue(xsMachine *the);
void KPR_Home_characteristicMetadata_get_stepValue(xsMachine *the);
void KPR_Home_characteristicMetadata_get_precision(xsMachine *the);
void KPR_Home_characteristicMetadata_get_maxLength(xsMachine *the);
void KPR_Home_characteristicMetadata_get_format(xsMachine *the);
void KPR_Home_characteristicMetadata_get_units(xsMachine *the);
void KPR_Home_characteristicMetadata_get_manufacturerDescription(xsMachine *the);
void KPR_Home_characteristicMetadata_get_validValues(xsMachine *the);

/* ================== */

void KPR_Home_room_destructor(void *it);
void KPR_Home_room_get_name(xsMachine *the);
void KPR_Home_room_updateName(xsMachine *the);
void KPR_Home_room_get_accessories(xsMachine *the);

/* ================== */

void KPR_Home_zone_destructor(void *it);
void KPR_Home_zone_get_name(xsMachine *the);
void KPR_Home_zone_updateName(xsMachine *the);
void KPR_Home_zone_get_rooms(xsMachine *the);
void KPR_Home_zone_addRoom(xsMachine *the);
void KPR_Home_zone_removeRoom(xsMachine *the);

/* ================== */

void KPR_Home_serviceGroup_destructor(void *it);
void KPR_Home_serviceGroup_get_name(xsMachine *the);
void KPR_Home_serviceGroup_updateName(xsMachine *the);
void KPR_Home_serviceGroup_get_services(xsMachine *the);
void KPR_Home_serviceGroup_addService(xsMachine *the);
void KPR_Home_serviceGroup_removeService(xsMachine *the);

/* ================== */

void KPR_Home_actionSet_destructor(void *it);
void KPR_Home_actionSet_get_name(xsMachine *the);
void KPR_Home_actionSet_updateName(xsMachine *the);
void KPR_Home_actionSet_get_actions(xsMachine *the);
void KPR_Home_actionSet_addCharacteristicWriteAction(xsMachine *the);
void KPR_Home_actionSet_removeAction(xsMachine *the);
void KPR_Home_actionSet_is_executing(xsMachine *the);
void KPR_Home_actionSet_execute(xsMachine *the);

/* ================== */

void KPR_Home_characteristicWriteAction_destructor(void *it);
void KPR_Home_characteristicWriteAction_get_characteristic(xsMachine *the);
void KPR_Home_characteristicWriteAction_get_targetValue(xsMachine *the);
void KPR_Home_characteristicWriteAction_updateTargetValue(xsMachine *the);

/* ================== */

void KPR_Home_timerTrigger_destructor(void *it);
void KPR_Home_timerTrigger_get_name(xsMachine *the);
void KPR_Home_timerTrigger_updateName(xsMachine *the);
void KPR_Home_timerTrigger_is_enabled(xsMachine *the);
void KPR_Home_timerTrigger_enable(xsMachine *the);
void KPR_Home_timerTrigger_disable(xsMachine *the);
void KPR_Home_timerTrigger_get_actionSets(xsMachine *the);
void KPR_Home_timerTrigger_addActionSet(xsMachine *the);
void KPR_Home_timerTrigger_removeActionSet(xsMachine *the);
void KPR_Home_timerTrigger_execute(xsMachine *the);
void KPR_Home_timerTrigger_get_fireDate(xsMachine *the);
void KPR_Home_timerTrigger_get_recurrence(xsMachine *the);
void KPR_Home_timerTrigger_get_lastFireDate(xsMachine *the);
void KPR_Home_timerTrigger_updateFireDate(xsMachine *the);
void KPR_Home_timerTrigger_updateRecurrence(xsMachine *the);

/* ================== */

FskErr kprHome_fskLoad(FskLibrary library);
FskErr kprHome_fskUnload(FskLibrary library);

#endif
