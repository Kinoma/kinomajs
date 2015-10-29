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
#import <HomeKit/HomeKit.h>

@class KprHomeDelegate;

#define KprHomeObjectDefine(name, class, ivar) struct name {\
	KprHomeGeneric next;\
	class *ivar;\
	KprHomeDelegate *delegate;\
	xsSlot slot;\
}

typedef struct KprHomeGenericRecord KprHomeGenericRecord, *KprHomeGeneric;
KprHomeObjectDefine(KprHomeGenericRecord, NSObject, object);

KprHomeObjectDefine(KprHomeManagerRecord, HMHomeManager, manager);
KprHomeObjectDefine(KprHomeBrowserRecord, HMAccessoryBrowser, browser);
KprHomeObjectDefine(KprHomeRecord, HMHome, home);
KprHomeObjectDefine(KprHomeAccessoryRecord, HMAccessory, accessory);
KprHomeObjectDefine(KprHomeServiceRecord, HMService, service);
KprHomeObjectDefine(KprHomeCharacteristicRecord, HMCharacteristic, characteristic);
KprHomeObjectDefine(KprHomeMetadataRecord, HMCharacteristicMetadata, metadata);
KprHomeObjectDefine(KprHomeRoomRecord, HMRoom, room);
KprHomeObjectDefine(KprHomeZoneRecord, HMZone, zone);
KprHomeObjectDefine(KprHomeServiceGroupRecord, HMServiceGroup, serviceGroup);
KprHomeObjectDefine(KprHomeActionSetRecord, HMActionSet, actionSet);
KprHomeObjectDefine(KprHomeCharacteristicWriteActionRecord, HMCharacteristicWriteAction, action);
KprHomeObjectDefine(KprHomeTimerTriggerRecord, HMTimerTrigger, trigger);
KprHomeObjectDefine(KprHomeUserRecord, HMUser, user);

FskErr KprHomeGenericDispose(void *object);

#pragma mark -
//--------------------------------------------------
// Delegate to manage callbacks
//--------------------------------------------------

@interface KprHomeDelegate : NSObject<HMHomeManagerDelegate, HMAccessoryBrowserDelegate, HMHomeDelegate, HMAccessoryDelegate>

- (instancetype)initWith:(xsMachine *)the code:(xsIndex *)code manager:(xsSlot)manager;

@property(readonly) xsMachine *the;
@property(readonly) xsSlot manager;
@property(readonly, getter=isReady) BOOL ready;

- (NSObject *)xsValueToNativeValue:(xsSlot)value;
- (BOOL)xsValueFromNativeValue:(NSObject *)value characteristic:(HMCharacteristic *)characteristic;
- (NSDate *)toNativeDate:(xsSlot)date;
- (void)fromNativeDate:(NSDate *)date;
- (NSDateComponents *)toDateComponent:(xsSlot)recurrence;
- (void)fromDateComponents:(NSDateComponents *)components;
- (const char *)serviceTypeString:(NSString *)type;
- (NSString *)nativeServiceTypeString:(const char *)typeStr;

- (BOOL)releaseRecord:(KprHomeGeneric)object;

- (KprHomeBrowser)newBrowser:(FskErr *)outErr;
- (KprHome)newHome:(HMHome *)home err:(FskErr *)outErr;
- (KprHomeAccessory)newAccessory:(HMAccessory *)accessory err:(FskErr *)outErr;
- (KprHomeService)newService:(HMService *)service err:(FskErr *)outErr;
- (KprHomeCharacteristic)newCharacteristic:(HMCharacteristic *)characteristic err:(FskErr *)outErr;
- (KprHomeMetadata)newMetadata:(HMCharacteristicMetadata *)metadata err:(FskErr *)outErr;
- (KprHomeRoom)newRoom:(HMRoom *)room err:(FskErr *)outErr;
- (KprHomeZone)newZone:(HMZone *)zone err:(FskErr *)outErr;
- (KprHomeServiceGroup)newServiceGroup:(HMServiceGroup *)serviceGroup err:(FskErr *)outErr;
- (KprHomeActionSet)newActionSet:(HMActionSet *)actionSet err:(FskErr *)outErr;
- (KprHomeCharacteristicWriteAction)newCharacteristicWriteAction:(HMCharacteristicWriteAction *)native err:(FskErr *)outErr;
- (KprHomeTimerTrigger)newTimerTrigger:(HMTimerTrigger *)trigger err:(FskErr *)outErr;
- (KprHomeUser)newUser:(HMUser *)user err:(FskErr *)outErr;

- (void (^)(NSError *error))completionHandlerForCallback:(xsSlot)callback;
- (void (^)(HMHome *home, NSError *error))completionHandlerForCallbackWithHome:(xsSlot)callback;
- (void (^)(HMRoom *room, NSError *error))completionHandlerForCallbackWithRoom:(xsSlot)callback;
- (void (^)(HMZone *zone, NSError *error))completionHandlerForCallbackWithZone:(xsSlot)callback;
- (void (^)(HMServiceGroup *, NSError *))completionHandlerForCallbackWithServiceGroup:(xsSlot)callback;
- (void (^)(HMActionSet *, NSError *))completionHandlerForCallbackWithActionSet:(xsSlot)callback;
- (void (^)(HMUser *user, NSError *error))completionHandlerForCallbackWithUser:(xsSlot)callback;
- (void (^)(NSError *error))completionHandlerForCallback:(xsSlot)callback withArgument:(void (^)(xsMachine *the))argument;

@end

#define kKprHomeCompletionSlotKey "_completion"

#pragma mark -
//--------------------------------------------------
// Manager
//--------------------------------------------------

FskErr KprHomeManagerNew(KprHomeManager *it, xsMachine *the, xsIndex *code, xsSlot this)
{
	FskErr err = kFskErrNone;
	KprHomeManager self = NULL;

	bailIfError(FskMemPtrNew(sizeof(KprHomeManagerRecord), &self));

	xsNewHostProperty(this, xsID("_browser"), xsNull, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);

	xsNewHostProperty(this, xsID(kKprHomeCompletionSlotKey), xsNewInstanceOf(xsObjectPrototype), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);

	self->manager = [[HMHomeManager alloc] init];

	self->delegate = [[KprHomeDelegate alloc] initWith:the code:code manager:this];

	self->manager.delegate = self->delegate;

	*it = self;
bail:
	if (err) {
		KprHomeManagerDispose(self);
	}

	return err;
}

FskErr KprHomeManagerDispose(KprHomeManager self)
{
	if (self) {
		self->manager.delegate = nil;
	}
	return KprHomeGenericDispose(self);
}

FskErr KprHomeManagerDisposeAt(KprHomeManager *it)
{
	if (it && *it) {
		KprHomeManagerDispose(*it);
		*it = NULL;
	}
	return kFskErrNone;
}

FskErr KprHomeManagerGetHomes(KprHomeManager self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMHome *native in self->manager.homes) {
		home = [self->delegate newHome:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);

		home = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeDispose(home);
	return err;
}

FskErr KprHomeManagerGetPrimaryHome(KprHomeManager self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHome home = NULL;
	HMHome *native = self->manager.primaryHome;

	if (native) {
		home = [self->delegate newHome:native err:&err];
		bailIfError(err);
	} else {
		xsResult = xsNull;
	}
bail:
	if (err) {
		KprHomeDispose(home);
	}
	return err;
}

FskErr KprHomeManagerIsReady(KprHomeManager self, xsMachine *the)
{
	FskErr err = kFskErrNone;

	xsResult = self->delegate.ready ? xsTrue : xsFalse;
	return err;
}

FskErr KprHomeManagerGetBrowser(KprHomeManager self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeBrowser browser = NULL;

	xsResult = xsGet(self->delegate.manager, xsID("_browser"));
	if (!xsTest(xsResult)) {
		browser = [self->delegate newBrowser:&err];
		bailIfError(err);

		xsNewHostProperty(self->delegate.manager, xsID("_browser"), xsResult, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	}
bail:
	if (err) {
		KprHomeBrowserDispose(browser);
	}
	return err;
}


FskErr KprHomeManagerAddHome(KprHomeManager self, const char *name, xsSlot callback)
{
	void (^completion)(HMHome *home, NSError *error) = [self->delegate completionHandlerForCallbackWithHome:callback];

	[self->manager addHomeWithName:[NSString stringWithCString:name encoding:NSUTF8StringEncoding] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeManagerRemoveHome(KprHomeManager self, KprHome home, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->manager removeHome:home->home completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeManagerSetPrimaryHome(KprHomeManager self, KprHome home, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->manager updatePrimaryHome:home->home completionHandler:completion];

	return kFskErrNone;
}


#pragma mark -
//--------------------------------------------------
// Browser
//--------------------------------------------------

FskErr KprHomeBrowserDispose(KprHomeBrowser self)
{
	if (self) {
		self->browser.delegate = nil;
	}
	return KprHomeGenericDispose(self);
}

FskErr KprHomeBrowserGetAccessories(KprHomeBrowser self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMAccessory *native in self->browser.discoveredAccessories) {
		accessory = [self->delegate newAccessory:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		accessory = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeAccessoryDispose(accessory);
	return err;
}

FskErr KprHomeBrowserStart(KprHomeBrowser self)
{
	[self->browser startSearchingForNewAccessories];
	return kFskErrNone;
}

FskErr KprHomeBrowserStop(KprHomeBrowser self)
{
	[self->browser stopSearchingForNewAccessories];
	return kFskErrNone;
}

#pragma mark -
//--------------------------------------------------
// Home
//--------------------------------------------------

FskErr KprHomeDispose(KprHome self)
{
	if (self) {
		self->home.delegate = nil;
	}
	return KprHomeGenericDispose(self);
}

FskErr KprHomeGetName(KprHome self, xsMachine *the)
{
	const char *name = [self->home.name UTF8String];
	xsResult = xsString((char *) name);
	return kFskErrNone;
}

FskErr KprHomeUpdateName(KprHome self, const char *name, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home updateName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeIsPrimary(KprHome self, xsMachine *the)
{
	Boolean isPrimary = (self->home.primary == YES);
	xsResult = xsBoolean(isPrimary);
	return kFskErrNone;
}

FskErr KprHomeGetAccessories(KprHome self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMAccessory *native in self->home.accessories) {
		accessory = [self->delegate newAccessory:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		accessory = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeAccessoryDispose(accessory);
	return err;
}

FskErr KprHomeAddAccessory(KprHome self, KprHomeAccessory accessory, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home addAccessory:accessory->accessory completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeRemoveAccessory(KprHome self, KprHomeAccessory accessory, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home removeAccessory:accessory->accessory completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeAssignAccessory(KprHome self, KprHomeAccessory accessory, KprHomeRoom room, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home assignAccessory:accessory->accessory toRoom:room->room completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeQueryServices(KprHome self, xsSlot types, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeService service = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	NSMutableArray *types_native = [NSMutableArray arrayWithCapacity:10];
	NSInteger length = xsToInteger(xsGet(types, xsID_length));
	for (NSInteger i = 0; i < length; i++) {
		char *str = xsToString(xsGetAt(types, xsInteger(i)));
		[types_native addObject:[self->delegate nativeServiceTypeString:str]];
	}

	for (HMService *native in [self->home servicesWithTypes:types_native]) {
		service = [self->delegate newService:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		service = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeServiceDispose(service);
	return err;
}


FskErr KprHomeUnblockAccessory(KprHome self, KprHomeAccessory accessory, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home unblockAccessory:accessory->accessory completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeGetUsers(KprHome self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeUser user = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMUser *native in self->home.users) {
		user = [self->delegate newUser:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		user = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeUserDispose(user);
	return kFskErrNone;
}

FskErr KprHomeAddUser(KprHome self, xsSlot callback)
{
	void (^completion)(HMUser *user, NSError *error) = [self->delegate completionHandlerForCallbackWithUser:callback];

	[self->home addUserWithCompletionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeRemoveUser(KprHome self, KprHomeUser user, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home removeUser:user->user completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeGetRooms(KprHome self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeRoom room = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMRoom *native in self->home.rooms) {
		room = [self->delegate newRoom:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		room = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeRoomDispose(room);
	return err;
}

FskErr KprHomeAddRoom(KprHome self, const char *name, xsSlot callback)
{
	void (^completion)(HMRoom *room, NSError *error) = [self->delegate completionHandlerForCallbackWithRoom:callback];

	[self->home addRoomWithName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeRemoveRoom(KprHome self, KprHomeRoom room, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home removeRoom:room->room completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeGetZones(KprHome self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeZone zone = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMZone *native in self->home.zones) {
		zone = [self->delegate newZone:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		zone = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeZoneDispose(zone);
	return err;
}

FskErr KprHomeAddZone(KprHome self, const char *name, xsSlot callback)
{
	void (^completion)(HMZone *, NSError *) = [self->delegate completionHandlerForCallbackWithZone:callback];

	[self->home addZoneWithName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeRemoveZone(KprHome self, KprHomeZone zone, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home removeZone:zone->zone completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeGetServiceGroups(KprHome self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeServiceGroup serviceGroup = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMServiceGroup *native in self->home.serviceGroups) {
		serviceGroup = [self->delegate newServiceGroup:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		serviceGroup = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeServiceGroupDispose(serviceGroup);
	return err;
}

FskErr KprHomeAddServiceGroup(KprHome self, const char *name, xsSlot callback)
{
	void (^completion)(HMServiceGroup *, NSError *) = [self->delegate completionHandlerForCallbackWithServiceGroup:callback];

	[self->home addServiceGroupWithName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeRemoveServiceGroup(KprHome self, KprHomeServiceGroup serviceGroup, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home removeServiceGroup:serviceGroup->serviceGroup completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeGetActionSets(KprHome self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeActionSet actionSet = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMActionSet *native in self->home.actionSets) {
		actionSet = [self->delegate newActionSet:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		actionSet = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeActionSetDispose(actionSet);
	return err;
}

FskErr KprHomeAddActionSet(KprHome self, const char *name, xsSlot callback)
{
	void (^completion)(HMActionSet *, NSError *) = [self->delegate completionHandlerForCallbackWithActionSet:callback];

	[self->home addActionSetWithName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeRemoveActionSet(KprHome self, KprHomeActionSet actionSet, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home removeActionSet:actionSet->actionSet completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeExecuteActionSet(KprHome self, KprHomeActionSet actionSet, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home executeActionSet:actionSet->actionSet completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeGetTriggers(KprHome self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeTimerTrigger trigger = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMTimerTrigger *native in self->home.triggers) {
		// trigger can be other trigger
		if ([native isKindOfClass:[HMTimerTrigger class]] == NO) continue;

		trigger = [self->delegate newTimerTrigger:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		trigger = NULL;
	}

	xsResult = xsVar(0);
bail:
	KprHomeTimerTriggerDispose(trigger);
	return err;
}

FskErr KprHomeAddTimerTrigger(KprHome self, const char *name, xsSlot date, xsSlot recurrence, xsSlot callback)
{
	NSDate *dateNative = [self->delegate toNativeDate:date];
	NSDateComponents *recurrenceNative = [self->delegate toDateComponent:recurrence];

	HMTimerTrigger *trigger = [[HMTimerTrigger alloc] initWithName:[NSString stringWithUTF8String:name] fireDate:dateNative timeZone:nil recurrence:recurrenceNative recurrenceCalendar:nil];

	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback withArgument:^(xsMachine *the) {
		[self->delegate newTimerTrigger:trigger err:NULL];
		[trigger release];
	}];

	[self->home addTrigger:trigger completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeRemoveTrigger(KprHome self, KprHomeTimerTrigger trigger, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->home removeTrigger:trigger->trigger completionHandler:completion];

	return kFskErrNone;
}

#pragma mark -
//--------------------------------------------------
// Accessory
//--------------------------------------------------

FskErr KprHomeAccessoryDispose(KprHomeAccessory self)
{
	if (self) {
		self->accessory.delegate = nil;
	}
	return KprHomeGenericDispose(self);
}

FskErr KprHomeAccessoryGetName(KprHomeAccessory self, xsMachine *the)
{
	const char *name = [self->accessory.name UTF8String];
	xsResult = xsString((char *) name);
	return kFskErrNone;
}

FskErr KprHomeAccessoryUpdateName(KprHomeAccessory self, const char *name, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->accessory updateName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeAccessoryGetIdentifier(KprHomeAccessory self, xsMachine *the)
{
	const char *identifier = [[self->accessory.identifier UUIDString] UTF8String];
	xsResult = xsString((char *) identifier);
	return kFskErrNone;
}

FskErr KprHomeAccessoryIsReachabe(KprHomeAccessory self, xsMachine *the)
{
	Boolean flag = (self->accessory.reachable == YES);
	xsResult = xsBoolean(flag);
	return kFskErrNone;
}

FskErr KprHomeAccessoryIsStandalone(KprHomeAccessory self, xsMachine *the)
{
	Boolean flag = (self->accessory.bridged == NO && [self->accessory identifiersForBridgedAccessories] == nil);
	xsResult = xsBoolean(flag);
	return kFskErrNone;
}

FskErr KprHomeAccessoryIsBridged(KprHomeAccessory self, xsMachine *the)
{
	Boolean flag = (self->accessory.bridged == YES);
	xsResult = xsBoolean(flag);
	return kFskErrNone;
}

FskErr KprHomeAccessoryIsBridge(KprHomeAccessory self, xsMachine *the)
{
	Boolean flag = (self->accessory.bridged == NO && [self->accessory identifiersForBridgedAccessories] != nil);
	xsResult = xsBoolean(flag);
	return kFskErrNone;
}

FskErr KprHomeAccessoryGetBridgedAccessoryIdentifiers(KprHomeAccessory self, xsMachine *the)
{
	xsVars(1);
	xsResult = xsNewInstanceOf(xsArrayPrototype);

	for (NSUUID *uuid in [self->accessory identifiersForBridgedAccessories]) {
		xsVar(0) = xsString((char *) [[uuid UUIDString] UTF8String]);
		xsCall1(xsResult, xsID("push"), xsVar(0));
	}

	return kFskErrNone;
}

FskErr KprHomeAccessoryGetRoom(KprHomeAccessory self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	HMRoom *room = self->accessory.room;

	xsResult = xsNull;

	if (room != nil) {
		[self->delegate newRoom:room err:&err];
	}
	return err;
}

FskErr KprHomeAccessoryGetServices(KprHomeAccessory self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeService service = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMService *native in self->accessory.services) {
		service = [self->delegate newService:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		service = NULL;
	}

	xsResult = xsVar(0);
bail:
	KprHomeServiceDispose(service);
	return err;
}

FskErr KprHomeAccessoryIsBlocked(KprHomeAccessory self, xsMachine *the)
{
	Boolean flag = (self->accessory.blocked == YES);
	xsResult = xsBoolean(flag);
	return kFskErrNone;
}

FskErr KprHomeAccessoryIdentify(KprHomeAccessory self, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->accessory identifyWithCompletionHandler:completion];

	 return kFskErrNone;
}

#pragma mark -
//--------------------------------------------------
// Service
//--------------------------------------------------

FskErr KprHomeServiceDispose(KprHomeService self)
{
	return KprHomeGenericDispose(self);
}

FskErr KprHomeServiceGetAccessory(KprHomeService self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	HMAccessory *accessory = self->service.accessory;

	xsResult = xsNull;

	if (accessory != nil) {
		[self->delegate newAccessory:accessory err:&err];
	}
	return err;
}

FskErr KprHomeServiceGetType(KprHomeService self, xsMachine *the)
{
	if (self->service.serviceType) {
		const char *type = [self->delegate serviceTypeString:self->service.serviceType];
		xsResult = xsString((char *) type);
	} else {
		xsResult = xsNull;
	}
	return kFskErrNone;
}

FskErr KprHomeServiceGetName(KprHomeService self, xsMachine *the)
{
	const char *name = [self->service.name UTF8String];
	if (name)
		xsResult = xsString((char *) name);
	else
		xsResult = xsNull;
	return kFskErrNone;
}

FskErr KprHomeServiceUpdateName(KprHomeService self, const char *name, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->service updateName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeServiceGetAssociatedType(KprHomeService self, xsMachine *the)
{
	if (self->service.associatedServiceType) {
		const char *type = [self->delegate serviceTypeString:self->service.associatedServiceType];
		xsResult = xsString((char *) type);
	} else {
		xsResult = xsNull;
	}
	return kFskErrNone;
}

FskErr KprHomeServiceUpdateAssociatedType(KprHomeService self, const char *type, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->service updateAssociatedServiceType:[self->delegate nativeServiceTypeString:type] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeServiceGetCharacteristics(KprHomeService self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristic characteristic = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMCharacteristic *native in self->service.characteristics) {
		characteristic = [self->delegate newCharacteristic:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		characteristic = NULL;
	}

	xsResult = xsVar(0);
bail:
	KprHomeCharacteristicDispose(characteristic);
	return err;
}

#pragma mark -
//--------------------------------------------------
// Characteristic
//--------------------------------------------------

FskErr KprHomeCharacteristicDispose(KprHomeCharacteristic self)
{
	return KprHomeGenericDispose(self);
}

FskErr KprHomeCharacteristicGetHash(KprHomeCharacteristic characteristic, xsMachine *the)
{
	NSUInteger hash = [characteristic->characteristic hash];
	xsResult = xsInteger(hash);
	return kFskErrNone;
}

FskErr KprHomeCharacteristicGetService(KprHomeCharacteristic self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	if (self->characteristic.service) {
		[self->delegate newService:self->characteristic.service err:&err];
	} else {
		xsResult = xsNull;
	}
	return err;
}

FskErr KprHomeCharacteristicGetType(KprHomeCharacteristic self, xsMachine *the)
{
	if (self->characteristic.characteristicType) {
		NSString *type = self->characteristic.characteristicType;

		if ([type isEqualToString:HMCharacteristicTypePowerState]) {
			type = @"PowerState";
		} else if ([type isEqualToString:HMCharacteristicTypeHue]) {
			type = @"Hue";
		} else if ([type isEqualToString:HMCharacteristicTypeSaturation]) {
			type = @"Saturation";
		} else if ([type isEqualToString:HMCharacteristicTypeBrightness]) {
			type = @"Brightness";
		} else if ([type isEqualToString:HMCharacteristicTypeTemperatureUnits]) {
			type = @"TemperatureUnits";
		} else if ([type isEqualToString:HMCharacteristicTypeCurrentTemperature]) {
			type = @"CurrentTemperature";
		} else if ([type isEqualToString:HMCharacteristicTypeTargetTemperature]) {
			type = @"TargetTemperature";
		} else if ([type isEqualToString:HMCharacteristicTypeCurrentHeatingCooling]) {
			type = @"CurrentHeatingCooling";
		} else if ([type isEqualToString:HMCharacteristicTypeTargetHeatingCooling]) {
			type = @"TargetHeatingCooling";
		} else if ([type isEqualToString:HMCharacteristicTypeCoolingThreshold]) {
			type = @"CoolingThreshold";
		} else if ([type isEqualToString:HMCharacteristicTypeHeatingThreshold]) {
			type = @"HeatingThreshold";
		} else if ([type isEqualToString:HMCharacteristicTypeCurrentRelativeHumidity]) {
			type = @"CurrentRelativeHumidity";
		} else if ([type isEqualToString:HMCharacteristicTypeTargetRelativeHumidity]) {
			type = @"TargetRelativeHumidity";
		} else if ([type isEqualToString:HMCharacteristicTypeCurrentDoorState]) {
			type = @"CurrentDoorState";
		} else if ([type isEqualToString:HMCharacteristicTypeTargetDoorState]) {
			type = @"TargetDoorState";
		} else if ([type isEqualToString:HMCharacteristicTypeObstructionDetected]) {
			type = @"ObstructionDetected";
		} else if ([type isEqualToString:HMCharacteristicTypeName]) {
			type = @"Name";
		} else if ([type isEqualToString:HMCharacteristicTypeManufacturer]) {
			type = @"Manufacturer";
		} else if ([type isEqualToString:HMCharacteristicTypeModel]) {
			type = @"Model";
		} else if ([type isEqualToString:HMCharacteristicTypeSerialNumber]) {
			type = @"SerialNumber";
		} else if ([type isEqualToString:HMCharacteristicTypeIdentify]) {
			type = @"Identify";
		} else if ([type isEqualToString:HMCharacteristicTypeRotationDirection]) {
			type = @"RotationDirection";
		} else if ([type isEqualToString:HMCharacteristicTypeRotationSpeed]) {
			type = @"RotationSpeed";
		} else if ([type isEqualToString:HMCharacteristicTypeOutletInUse]) {
			type = @"OutletInUse";
		} else if ([type isEqualToString:HMCharacteristicTypeVersion]) {
			type = @"Version";
		} else if ([type isEqualToString:HMCharacteristicTypeLogs]) {
			type = @"Logs";
		} else if ([type isEqualToString:HMCharacteristicTypeAudioFeedback]) {
			type = @"AudioFeedback";
		} else if ([type isEqualToString:HMCharacteristicTypeAdminOnlyAccess]) {
			type = @"AdminOnlyAccess";
		} else if ([type isEqualToString:HMCharacteristicTypeMotionDetected]) {
			type = @"MotionDetected";
		} else if ([type isEqualToString:HMCharacteristicTypeCurrentLockMechanismState]) {
			type = @"CurrentLockMechanismState";
		} else if ([type isEqualToString:HMCharacteristicTypeTargetLockMechanismState]) {
			type = @"TargetLockMechanismState";
		} else if ([type isEqualToString:HMCharacteristicTypeLockMechanismLastKnownAction]) {
			type = @"LockMechanismLastKnownAction";
		} else if ([type isEqualToString:HMCharacteristicTypeLockManagementControlPoint]) {
			type = @"LockManagementControlPoint";
		} else if ([type isEqualToString:HMCharacteristicTypeLockManagementAutoSecureTimeout]) {
			type = @"LockManagementAutoSecureTimeout";
		}

		xsResult = xsString((char *) [type UTF8String]);
	} else {
		xsResult = xsNull;
	}
	return kFskErrNone;
}

FskErr KprHomeCharacteristicGetProperties(KprHomeCharacteristic self, xsMachine *the)
{
	xsVars(1);
	xsResult = xsNewInstanceOf(xsArrayPrototype);

	for (NSString *property in self->characteristic.properties) {
		xsVar(0) = xsString((char *) [property UTF8String]);
		xsCall1(xsResult, xsID("push"), xsVar(0));
	}

	return kFskErrNone;
}

FskErr KprHomeCharacteristicIsReadable(KprHomeCharacteristic self, xsMachine *the)
{
	Boolean flag = [self->characteristic.properties containsObject:HMCharacteristicPropertyReadable];
	xsResult = xsBoolean(flag);
	return kFskErrNone;
}

FskErr KprHomeCharacteristicIsWritable(KprHomeCharacteristic self, xsMachine *the)
{
	Boolean flag = [self->characteristic.properties containsObject:HMCharacteristicPropertyWritable];
	xsResult = xsBoolean(flag);
	return kFskErrNone;
}

FskErr KprHomeCharacteristicSupportsEventNotification(KprHomeCharacteristic self, xsMachine *the)
{
	Boolean flag = [self->characteristic.properties containsObject:HMCharacteristicPropertySupportsEventNotification];
	xsResult = xsBoolean(flag);
	return kFskErrNone;
}

FskErr KprHomeCharacteristicGetMetadata(KprHomeCharacteristic self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	if (self->characteristic.metadata) {
		[self->delegate newMetadata:self->characteristic.metadata err:&err];
	} else {
		xsResult = xsNull;
	}
	return err;
}

FskErr KprHomeCharacteristicGetValue(KprHomeCharacteristic self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	id value = self->characteristic.value;
	if ([self->delegate xsValueFromNativeValue:value characteristic:self->characteristic] == NO) {
		err = kFskErrBadData;
	}
	return err;
}

FskErr KprHomeCharacteristicWriteValue(KprHomeCharacteristic self, xsSlot value, xsSlot callback)
{
	id writeValue = [self->delegate xsValueToNativeValue:value];
	if (writeValue == nil) return kFskErrBadData;

	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->characteristic writeValue:writeValue completionHandler:completion];
	return kFskErrNone;
}

FskErr KprHomeCharacteristicReadValue(KprHomeCharacteristic self, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->characteristic readValueWithCompletionHandler:completion];
	return kFskErrNone;
}

FskErr KprHomeCharacteristicIsNotificationEnabled(KprHomeCharacteristic self, xsMachine *the)
{
	Boolean flag = (self->characteristic.notificationEnabled == YES);
	xsResult = xsBoolean(flag);
	return kFskErrNone;
}

FskErr KprHomeCharacteristicSetEnableNotification(KprHomeCharacteristic self, Boolean enable, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->characteristic enableNotification:enable completionHandler:completion];
	return kFskErrNone;
}

#pragma mark -
//--------------------------------------------------
// Characteristic Metadata
//--------------------------------------------------

FskErr KprHomeMetadataDispose(KprHomeMetadata self)
{
	return KprHomeGenericDispose(self);
}

FskErr KprHomeMetadataGetMinimumValue(KprHomeMetadata self, xsMachine *the)
{
	if (self->metadata.minimumValue) {
		xsResult = xsNumber([self->metadata.minimumValue doubleValue]);
	} else {
		xsResult = xsUndefined;
	}
	return kFskErrNone;
}

FskErr KprHomeMetadataGetMaximumValue(KprHomeMetadata self, xsMachine *the)
{
	if (self->metadata.maximumValue) {
		xsResult = xsNumber([self->metadata.maximumValue doubleValue]);
	} else {
		xsResult = xsUndefined;
	}
	return kFskErrNone;
}

FskErr KprHomeMetadataGetStepValue(KprHomeMetadata self, xsMachine *the)
{
	if (self->metadata.stepValue) {
		xsResult = xsNumber([self->metadata.stepValue doubleValue]);
	} else {
		xsResult = xsUndefined;
	}
	return kFskErrNone;
}

FskErr KprHomeMetadataGetMaxLength(KprHomeMetadata self, xsMachine *the)
{
	if (self->metadata.maxLength) {
		xsResult = xsInteger([self->metadata.maxLength integerValue]);
	} else {
		xsResult = xsUndefined;
	}
	return kFskErrNone;
}

FskErr KprHomeMetadataGetFormat(KprHomeMetadata self, xsMachine *the)
{
	if (self->metadata.format) {
		xsResult = xsString((char *)[self->metadata.format UTF8String]);
	} else {
		xsResult = xsUndefined;
	}
	return kFskErrNone;
}

FskErr KprHomeMetadataGetUnits(KprHomeMetadata self, xsMachine *the)
{
	if (self->metadata.units) {
		xsResult = xsString((char *)[self->metadata.units UTF8String]);
	} else {
		xsResult = xsUndefined;
	}
	return kFskErrNone;
}

FskErr KprHomeMetadataGetManufacturerDescription(KprHomeMetadata self, xsMachine *the)
{
	if (self->metadata.manufacturerDescription) {
		xsResult = xsString((char *)[self->metadata.manufacturerDescription UTF8String]);
	} else {
		xsResult = xsUndefined;
	}
	return kFskErrNone;
}


#pragma mark -
//--------------------------------------------------
// Room
//--------------------------------------------------

FskErr KprHomeRoomDispose(KprHomeRoom self)
{
	return KprHomeGenericDispose(self);
}

FskErr KprHomeRoomGetName(KprHomeRoom self, xsMachine *the)
{
	const char *name = [self->room.name UTF8String];
	xsResult = xsString((char *) name);
	return kFskErrNone;
}

FskErr KprHomeRoomUpdateName(KprHomeRoom self, const char *name, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->room updateName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeRoomGetAccessories(KprHomeRoom self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeAccessory accessory = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMAccessory *native in self->room.accessories) {
		accessory = [self->delegate newAccessory:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		accessory = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeAccessoryDispose(accessory);
	return err;
}


#pragma mark -
//--------------------------------------------------
// Zone
//--------------------------------------------------

FskErr KprHomeZoneDispose(KprHomeZone self)
{
	return KprHomeGenericDispose(self);
}

FskErr KprHomeZoneGetName(KprHomeZone self, xsMachine *the)
{
	const char *name = [self->zone.name UTF8String];
	xsResult = xsString((char *) name);
	return kFskErrNone;
}

FskErr KprHomeZoneUpdateName(KprHomeZone self, const char *name, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->zone updateName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeZoneGetRooms(KprHomeZone self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeRoom room = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMRoom *native in self->zone.rooms) {
		room = [self->delegate newRoom:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		room = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeRoomDispose(room);
	return err;
}

FskErr KprHomeZoneAddRoom(KprHomeZone self, KprHomeRoom room, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->zone addRoom:room->room completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeZoneRemoveRoom(KprHomeZone self, KprHomeRoom room, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->zone removeRoom:room->room completionHandler:completion];

	return kFskErrNone;
}


#pragma mark -
//--------------------------------------------------
// Service Group
//--------------------------------------------------

FskErr KprHomeServiceGroupDispose(KprHomeServiceGroup self)
{
	return KprHomeGenericDispose(self);
}

FskErr KprHomeServiceGroupGetName(KprHomeServiceGroup self, xsMachine *the)
{
	const char *name = [self->serviceGroup.name UTF8String];
	xsResult = xsString((char *) name);
	return kFskErrNone;
}

FskErr KprHomeServiceGroupUpdateName(KprHomeServiceGroup self, const char *name, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->serviceGroup updateName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeServiceGroupGetServices(KprHomeServiceGroup self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeService service = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMService *native in self->serviceGroup.services) {
		service = [self->delegate newService:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		service = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeServiceDispose(service);
	return err;
}

FskErr KprHomeServiceGroupAddService(KprHomeServiceGroup self, KprHomeService service, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->serviceGroup addService:service->service completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeServiceGroupRemoveService(KprHomeServiceGroup self, KprHomeService service, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->serviceGroup removeService:service->service completionHandler:completion];

	return kFskErrNone;
}


#pragma mark -
//--------------------------------------------------
// Action Set
//--------------------------------------------------

FskErr KprHomeActionSetDispose(KprHomeActionSet self)
{
	return KprHomeGenericDispose(self);
}

FskErr KprHomeActionSetGetName(KprHomeActionSet self, xsMachine *the)
{
	const char *name = [self->actionSet.name UTF8String];
	xsResult = xsString((char *) name);
	return kFskErrNone;
}

FskErr KprHomeActionSetUpdateName(KprHomeActionSet self, const char *name, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->actionSet updateName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeActionSetGetActions(KprHomeActionSet self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeCharacteristicWriteAction action = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMCharacteristicWriteAction *native in self->actionSet.actions) {
		// Action can be other class.
		if ([native isKindOfClass:[HMCharacteristicWriteAction class]] == NO) continue;

		action = [self->delegate newCharacteristicWriteAction:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		action = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeCharacteristicWriteActionDispose(action);
	return err;
}

FskErr KprHomeActionSetAddCharacteristicWriteAction(KprHomeActionSet self, KprHomeCharacteristic characteristic, xsSlot value, xsSlot callback)
{
	id nativeValue = [self->delegate xsValueToNativeValue:value];
	if (nativeValue == nil) return kFskErrBadData;

	HMCharacteristicWriteAction *action = [[HMCharacteristicWriteAction alloc] initWithCharacteristic:characteristic->characteristic targetValue:nativeValue];

	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback withArgument:^(xsMachine *the) {
		[self->delegate newCharacteristicWriteAction:action err:NULL];
		[action release];
	}];

	[self->actionSet addAction:action completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeActionSetRemoveAction(KprHomeActionSet self, KprHomeCharacteristicWriteAction action, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->actionSet removeAction:action->action completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeActionSetIsExecuting(KprHomeActionSet self, xsMachine *the)
{
	Boolean flag = (self->actionSet.executing == YES);
	xsResult = xsBoolean(flag);
	return kFskErrNone;
}

#pragma mark -
//--------------------------------------------------
// Characteristic Write Action
//--------------------------------------------------

FskErr KprHomeCharacteristicWriteActionDispose(KprHomeCharacteristicWriteAction self)
{
	return KprHomeGenericDispose(self);
}

FskErr KprHomeCharacteristicWriteActionGetCharacteristic(KprHomeCharacteristicWriteAction self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	[self->delegate newCharacteristic:self->action.characteristic err:&err];
	return err;
}

FskErr KprHomeCharacteristicWriteActionGetTargetValue(KprHomeCharacteristicWriteAction self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	id value = self->action.targetValue;
	if ([self->delegate xsValueFromNativeValue:value characteristic:self->action.characteristic] == NO) {
		err = kFskErrBadData;
	}
	return err;
}

FskErr KprHomeCharacteristicWriteActionUpdateTargetValue(KprHomeCharacteristicWriteAction self, xsSlot value, xsSlot callback)
{
	id writeValue = [self->delegate xsValueToNativeValue:value];
	if (writeValue == nil) return kFskErrBadData;

	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->action updateTargetValue:writeValue completionHandler:completion];

	return kFskErrNone;
}


#pragma mark -
//--------------------------------------------------
// TimerTrigger
//--------------------------------------------------

FskErr KprHomeTimerTriggerDispose(KprHomeTimerTrigger self)
{
	return KprHomeGenericDispose(self);
}

FskErr KprHomeTimerTriggerGetName(KprHomeTimerTrigger self, xsMachine *the)
{
	const char *name = [self->trigger.name UTF8String];
	xsResult = xsString((char *) name);
	return kFskErrNone;
}

FskErr KprHomeTimerTriggerUpdateName(KprHomeTimerTrigger self, const char *name, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->trigger updateName:[NSString stringWithUTF8String:name] completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeTimerTriggerIsEnabled(KprHomeTimerTrigger self, xsMachine *the)
{
	Boolean flag = (self->trigger.enabled == YES);
	xsResult = xsBoolean(flag);
	return kFskErrNone;
}

FskErr KprHomeTimerTriggerSetEnable(KprHomeTimerTrigger self, Boolean enable, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->trigger enable:enable completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeTimerTriggerGetActionSets(KprHomeTimerTrigger self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprHomeActionSet actionSet = NULL;

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	for (HMActionSet *native in self->trigger.actionSets) {
		actionSet = [self->delegate newActionSet:native err:&err];
		bailIfError(err);

		xsCall1(xsVar(0), xsID("push"), xsResult);
		actionSet = NULL;
	}

	xsResult = xsVar(0);

bail:
	KprHomeActionSetDispose(actionSet);
	return err;
}

FskErr KprHomeTimerTriggerAddActionSet(KprHomeTimerTrigger self, KprHomeActionSet actionSet, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->trigger addActionSet:actionSet->actionSet completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeTimerTriggerRemoveActionSet(KprHomeTimerTrigger self, KprHomeActionSet actionSet, xsSlot callback)
{
	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->trigger removeActionSet:actionSet->actionSet completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeTimerTriggerGetFireDate(KprHomeTimerTrigger self, xsMachine *the)
{
	[self->delegate fromNativeDate:self->trigger.fireDate];
	return kFskErrNone;
}

FskErr KprHomeTimerTriggerGetRecurrence(KprHomeTimerTrigger self, xsMachine *the)
{
	[self->delegate fromDateComponents:self->trigger.recurrence];
	return kFskErrNone;
}

FskErr KprHomeTimerTriggerGetLastFireDate(KprHomeTimerTrigger self, xsMachine *the)
{
	[self->delegate fromNativeDate:self->trigger.lastFireDate];
	return kFskErrNone;
}

FskErr KprHomeTimerTriggerUpdateFireDate(KprHomeTimerTrigger self, xsSlot date, xsSlot callback)
{
	NSDate *dateNative = [self->delegate toNativeDate:date];

	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->trigger updateFireDate:dateNative completionHandler:completion];

	return kFskErrNone;
}

FskErr KprHomeTimerTriggerUpdateRecurrence(KprHomeTimerTrigger self, xsSlot recurrence, xsSlot callback)
{
	NSDateComponents *recurrenceNative = [self->delegate toDateComponent:recurrence];

	void (^completion)(NSError *error) = [self->delegate completionHandlerForCallback:callback];

	[self->trigger updateRecurrence:recurrenceNative completionHandler:completion];

	return kFskErrNone;
}

#pragma mark -
//--------------------------------------------------
// User
//--------------------------------------------------

FskErr KprHomeUserDispose(KprHomeUser self)
{
	return KprHomeGenericDispose(self);
}

FskErr KprHomeUserGetName(KprHomeUser self, xsMachine *the)
{
	const char *name = [self->user.name UTF8String];
	xsResult = xsString((char *) name);
	return kFskErrNone;
}

#pragma mark -
//--------------------------------------------------
// Generic
//--------------------------------------------------

FskErr KprHomeGenericDispose(void *self)
{
	if (self) {
		KprHomeGeneric object = self;
		KprHomeDelegate *delegate = [object->delegate retain];
		[delegate releaseRecord:object];
		[delegate release];
	}
	return kFskErrNone;
}

#pragma mark -
//--------------------------------------------------
// HomeKit delegate
//--------------------------------------------------

@implementation KprHomeDelegate {
	xsMachine* _the;
	xsIndex* _code;
	xsSlot _manager;
	KPR_Home_Error xsErr;
	BOOL _ready;
	UInt32 _issuedTask;
	KprHomeGeneric _cache;
}

- (instancetype)initWith:(xsMachine *)the code:(xsIndex *)code manager:(xsSlot)manager
{
	self = [super init];
	if (self) {
		_the = the;
		_code = code;
		_manager = manager;
		_ready = NO;
		_issuedTask = 0;
	}
	return self;
}

- (xsMachine *)the
{
	return _the;
}

- (BOOL)isReady
{
	return _ready;
}

- (NSObject *)xsValueToNativeValue:(xsSlot)value
{
	xsMachine *the = _the;
	NSObject *native = nil;

	switch (xsTypeOf(value)) {
		case xsBooleanType:
			native = [NSNumber numberWithBool:xsToBoolean(value)];
			break;

		case xsIntegerType:
			native = [NSNumber numberWithInt:xsToInteger(value)];
			break;

		case xsNumberType:
			native = [NSNumber numberWithDouble:xsToNumber(value)];
			break;

		case xsStringType:
			native = [NSString stringWithUTF8String:xsToString(value)];
			break;

		case xsNullType:
		case xsUndefinedType:
			native = [NSNull null];
			break;

		default: {
			if (xsIsInstanceOf(value, xsChunkPrototype)) {
				NSInteger length = xsToInteger(xsGet(value, xsID_length));
				void *data = xsGetHostData(value);
				native = [NSData dataWithBytes:data length:length];
			}
			break;
		}
	}

	return native;
}

- (BOOL)xsValueFromNativeValue:(NSObject *)value characteristic:(HMCharacteristic *)characteristic
{
	xsMachine *the = _the;
	NSString *format = characteristic.metadata.format;

	if (value) {
		if ([format isEqualToString:HMCharacteristicMetadataFormatBool]) {
			if ([value isKindOfClass:[NSNumber class]]) {
				xsResult = xsBoolean([(NSNumber *)value boolValue]);
			} else {
				return NO;
			}
		} else if ([format isEqualToString:HMCharacteristicMetadataFormatInt]) {
			if ([value isKindOfClass:[NSNumber class]]) {
				xsResult = xsInteger([(NSNumber *)value integerValue]);
			} else {
				return NO;
			}
		} else if ([format isEqualToString:HMCharacteristicMetadataFormatFloat]) {
			if ([value isKindOfClass:[NSNumber class]]) {
				xsResult = xsNumber([(NSNumber *)value floatValue]);
			} else {
				return NO;
			}
		} else if ([format isEqualToString:HMCharacteristicMetadataFormatString]) {
			if ([value isKindOfClass:[NSString class]]) {
				xsResult = xsString((char *) [(NSString *)value UTF8String]);
			} else {
				return NO;
			}
//		} else if ([format isEqualToString:HMCharacteristicMetadataFormatArray]) {
//			err = kFskErrUnimplemented;
//		} else if ([format isEqualToString:HMCharacteristicMetadataFormatDictionary]) {
//			err = kFskErrUnimplemented;
		} else if ([format isEqualToString:HMCharacteristicMetadataFormatUInt8]) {
			xsResult = xsNumber([(NSNumber *)value unsignedCharValue]);
		} else if ([format isEqualToString:HMCharacteristicMetadataFormatUInt16]) {
			xsResult = xsNumber([(NSNumber *)value unsignedShortValue]);
		} else if ([format isEqualToString:HMCharacteristicMetadataFormatUInt32]) {
			xsResult = xsNumber([(NSNumber *)value unsignedIntValue]);
//		} else if ([format isEqualToString:HMCharacteristicMetadataFormatUInt64]) {
//			xsResult = xsNumber([(NSNumber *)value unsignedLongLongValue]);
		} else if ([format isEqualToString:HMCharacteristicMetadataFormatData]) {
			if ([value isKindOfClass:[NSData class]]) {
				NSData *dataObject = (NSData *) value;
				NSInteger size = [dataObject length];

				FskMemPtr data = NULL;
				FskErr err = FskMemPtrNewFromData(size, [dataObject bytes], &data);
				if (err != kFskErrNone) return NO;

				xsResult = xsNewInstanceOf(xsChunkPrototype);
				xsSetHostData(xsResult, data);

				xsDestructor destructor = xsGetHostDestructor(xsResult);
				xsSetHostDestructor(xsResult, NULL);
				xsSet(xsResult, xsID_length, xsInteger(size));
				xsSetHostDestructor(xsResult, destructor);
			} else {
				return NO;
			}
//		} else if ([format isEqualToString:HMCharacteristicMetadataFormatTLV8]) {
//			err = kFskErrUnimplemented;
		} else {
			return NO;
		}
	} else {
		xsResult = xsUndefined;
	}
	return YES;
}

- (NSDate *)toNativeDate:(xsSlot)date
{
	xsMachine *the = self.the;
	NSTimeInterval epoch;

	xsFindResult(date, xsID_valueOf);
	xsResult = xsCallFunction0(xsResult, date);
	epoch = xsToNumber(xsResult) / 1000.0;

	return [NSDate dateWithTimeIntervalSince1970:epoch];
}

- (void)fromNativeDate:(NSDate *)date
{
	xsMachine *the = self.the;
	NSTimeInterval epoch = [date timeIntervalSince1970];

	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsDatePrototype);
	xsFindResult(xsVar(0), xsID("setTime"));
	xsCallFunction1(xsResult, xsVar(0), xsNumber(epoch * 1000));
	xsResult = xsVar(0);
}

- (NSDateComponents *)toDateComponent:(xsSlot)recurrence
{
	xsMachine *the = self.the;

	if (xsTypeOf(recurrence) == xsUndefinedType) return nil;

	NSDateComponents *recurrenceNative = [[[NSDateComponents alloc] init] autorelease];

	NSInteger value = NSNotFound;
	if (xsFindInteger(recurrence, xsID("year"), &value)) recurrenceNative.year = value;
	if (xsFindInteger(recurrence, xsID("month"), &value)) recurrenceNative.month = value;
	if (xsFindInteger(recurrence, xsID("day"), &value)) recurrenceNative.day = value;
	if (xsFindInteger(recurrence, xsID("hour"), &value)) recurrenceNative.hour = value;
	if (xsFindInteger(recurrence, xsID("minute"), &value)) recurrenceNative.minute = value;
	if (xsFindInteger(recurrence, xsID("second"), &value)) recurrenceNative.second = value;

	if (value == NSNotFound) return nil;

	return recurrenceNative;
}

- (void)fromDateComponents:(NSDateComponents *)components
{
	xsMachine *the = self.the;

	xsResult = xsNewInstanceOf(xsObjectPrototype);
	if (components.year) xsSet(xsResult, xsID("year"), xsInteger(components.year));
	if (components.month) xsSet(xsResult, xsID("month"), xsInteger(components.month));
	if (components.day) xsSet(xsResult, xsID("day"), xsInteger(components.day));
	if (components.hour) xsSet(xsResult, xsID("hour"), xsInteger(components.hour));
	if (components.minute) xsSet(xsResult, xsID("minute"), xsInteger(components.minute));
	if (components.second) xsSet(xsResult, xsID("second"), xsInteger(components.second));
}

- (const char *)serviceTypeString:(NSString *)type
{
	if ([type isEqualToString:HMServiceTypeLightbulb]) {
		return "Lightbulb";
	}

	if ([type isEqualToString:HMServiceTypeSwitch]) {
		return "Switch";
	}

	if ([type isEqualToString:HMServiceTypeThermostat]) {
		return "Thermostat";
	}

	if ([type isEqualToString:HMServiceTypeGarageDoorOpener]) {
		return "GarageDoorOpener";
	}

	if ([type isEqualToString:HMServiceTypeAccessoryInformation]) {
		return "AccessoryInformation";
	}

	if ([type isEqualToString:HMServiceTypeFan]) {
		return "Fan";
	}

	if ([type isEqualToString:HMServiceTypeOutlet]) {
		return "Outlet";
	}

	if ([type isEqualToString:HMServiceTypeLockMechanism]) {
		return "LockMechanism";
	}

	if ([type isEqualToString:HMServiceTypeLockManagement]) {
		return "LockManagement";
	}

	return [type UTF8String];
}

- (NSString *)nativeServiceTypeString:(const char *)typeStr
{
	if (FskStrCompare(typeStr, "Lightbulb") == 0) {
		return HMServiceTypeLightbulb;
	}

	if (FskStrCompare(typeStr, "Switch") == 0) {
		return HMServiceTypeSwitch;
	}

	if (FskStrCompare(typeStr, "Thermostat") == 0) {
		return HMServiceTypeThermostat;
	}

	if (FskStrCompare(typeStr, "GarageDoorOpener") == 0) {
		return HMServiceTypeGarageDoorOpener;
	}

	if (FskStrCompare(typeStr, "AccessoryInformation") == 0) {
		return HMServiceTypeAccessoryInformation;
	}

	if (FskStrCompare(typeStr, "Fan") == 0) {
		return HMServiceTypeFan;
	}

	if (FskStrCompare(typeStr, "Outlet") == 0) {
		return HMServiceTypeOutlet;
	}

	if (FskStrCompare(typeStr, "LockMechanism") == 0) {
		return HMServiceTypeLockMechanism;
	}

	if (FskStrCompare(typeStr, "LockManagement") == 0) {
		return HMServiceTypeLockManagement;
	}

	return [NSString stringWithUTF8String:typeStr];
}

- (xsSlot)xsError:(NSError *)error
{
	xsMachine *the = _the;

	if (error) {
		NSInteger code = [error code];
		char *message = (char *) [[error localizedDescription] UTF8String];

		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID("code"), xsInteger(code), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID("message"), xsString(message), xsDefault, xsDontScript);
	} else {
		xsResult = xsNull;
	}

	return xsResult;
}

- (void)sandboxWithVars:(NSInteger)n then:(void (^)(xsMachine *the))block
{
	xsBeginHostSandboxCode(_the, _code);
	xsThis = xsGet(_manager, xsID_behavior);
	xsVars(n);
	xsTry {
		block(the);
	}
	xsCatch {
	}
	xsEndHostSandboxCode();
}

- (void)sandboxWithVars:(NSInteger)n callback:(char *)name then:(void (^)(xsMachine *the))block
{
	[self sandboxWithVars:n then:^(xsMachine *the) {
		if (xsFindResult(xsThis, xsID(name))) {
			block(the);
		}
	}];
}

- (void)cacheRecord:(KprHomeGeneric)record
{
	FskListAppend(&_cache, record);
}

- (void)uncacheRecord:(KprHomeGeneric)record
{
	FskListRemove(&_cache, record);
}

- (KprHomeGeneric)findRecordForObject:(NSObject *)object
{
	KprHomeGeneric record = _cache;
	while (record != NULL) {
		if ([object isEqual:record->object]) return record;
		record = record->next;
	}
	return NULL;
}

- (BOOL)releaseRecord:(KprHomeGeneric)record
{
	[self uncacheRecord:record];

	[record->object release];
	FskMemPtrDispose(record);

	[self release];
	return YES;
}

- (void *)newObject:(NSObject *)native prototype:(char *)prototype err:(FskErr *)outErr
{
	FskErr err = kFskErrNone;
	KprHomeGeneric obj = NULL;
	xsMachine *the = _the;

	obj = [self findRecordForObject:native];
	if (obj) {
		xsResult = obj->slot;
		goto bail;
	}

	bailIfError(FskMemPtrNew(sizeof(KprHomeGenericRecord), &obj));

	obj->object = [native retain];
	obj->delegate = [self retain];

	xsResult = xsGet(xsGlobal, xsID("Home"));
	xsResult = xsGet(xsResult, xsID(prototype));
#if mxDebug
	NSAssert1(xsTest(xsResult), @"prototype '%s' must exists", prototype);
#endif
	xsResult = xsNewInstanceOf(xsResult);
	xsSetHostData(xsResult, obj);
	obj->slot = xsResult;

	[self cacheRecord:obj];

bail:
	if (err) {
		FskMemPtrDispose(obj);
		obj = NULL;
		xsResult = xsNull;
	}

	if (outErr) *outErr = err;

	return obj;
}

- (KprHomeBrowser)newBrowser:(FskErr *)outErr
{
	HMAccessoryBrowser *native =[[HMAccessoryBrowser alloc] init];
	KprHomeBrowser browser = [self newObject:native prototype:"browser" err:outErr];
	browser->browser.delegate = self;
	[native release];
	return browser;
}

- (KprHome)newHome:(HMHome *)native err:(FskErr *)outErr
{
	KprHome home = [self newObject:native prototype:"home" err:outErr];
	home->home.delegate = self;
	return home;
}

- (KprHomeAccessory)newAccessory:(HMAccessory *)native err:(FskErr *)outErr
{
	KprHomeAccessory accessory = [self newObject:native prototype:"accessory" err:outErr];
	accessory->accessory.delegate = self;
	return accessory;
}

- (KprHomeService)newService:(HMService *)native err:(FskErr *)outErr
{
	return [self newObject:native prototype:"service" err:outErr];
}

- (KprHomeCharacteristic)newCharacteristic:(HMCharacteristic *)native err:(FskErr *)outErr
{
	return [self newObject:native prototype:"characteristic" err:outErr];
}

- (KprHomeMetadata)newMetadata:(HMCharacteristicMetadata *)native err:(FskErr *)outErr
{
	return [self newObject:native prototype:"characteristicMetadata" err:outErr];
}

- (KprHomeRoom)newRoom:(HMRoom *)native err:(FskErr *)outErr
{
	return [self newObject:native prototype:"room" err:outErr];
}

- (KprHomeZone)newZone:(HMZone *)native err:(FskErr *)outErr
{
	return [self newObject:native prototype:"zone" err:outErr];
}

- (KprHomeServiceGroup)newServiceGroup:(HMServiceGroup *)native err:(FskErr *)outErr
{
	return [self newObject:native prototype:"serviceGroup" err:outErr];
}

- (KprHomeActionSet)newActionSet:(HMActionSet *)native err:(FskErr *)outErr
{
	return [self newObject:native prototype:"actionSet" err:outErr];
}

- (KprHomeCharacteristicWriteAction)newCharacteristicWriteAction:(HMCharacteristicWriteAction *)native err:(FskErr *)outErr
{
	return [self newObject:native prototype:"characteristicWriteAction" err:outErr];
}

- (KprHomeTimerTrigger)newTimerTrigger:(HMTimerTrigger *)native err:(FskErr *)outErr
{
	return [self newObject:native prototype:"timerTrigger" err:outErr];
}

- (KprHomeUser)newUser:(HMUser *)native err:(FskErr *)outErr
{
	return [self newObject:native prototype:"user" err:outErr];
}

#pragma mark -

- (void)completionContainer:(xsMachine *)the
{
	xsResult = xsGet(_manager, xsID(kKprHomeCompletionSlotKey));
}

- (UInt32)registerCallback:(xsSlot)callback
{
	UInt32 taskNo = ++_issuedTask;

	char buf[10];
	FskStrNumToStr(taskNo, buf, 10);

	xsMachine *the = _the;
	[self completionContainer:the];
	xsNewHostProperty(xsResult, xsID(buf), callback, xsDefault, xsDontScript);

	return taskNo;
}

- (xsSlot)callbackWithNo:(UInt32)taskNo
{
	char buf[10];
	FskStrNumToStr(taskNo, buf, 10);

	xsMachine *the = _the;
	[self completionContainer:the];
	xsResult = xsGet(xsResult, xsID(buf));
	return xsResult;
}

- (void)finishCallback:(UInt32)taskNo
{
	char buf[10];
	FskStrNumToStr(taskNo, buf, 10);

	xsMachine *the = _the;
	[self completionContainer:the];
	xsDelete(xsResult, xsID(buf));
}

- (void (^)(NSError *error))completionHandlerForCallback:(xsSlot)callback
{
	xsMachine *the = _the;

	if (!xsTest(callback)) return nil;

	UInt32 taskNo = [self registerCallback:callback];

	return [^(NSError *error) {
		[self sandboxWithVars:2 then:^(xsMachine *the) {
			xsVar(0) = [self callbackWithNo:taskNo];
			xsVar(1) = [self xsError:error];

			xsCallFunction1(xsVar(0), xsThis, xsVar(1));

			[self finishCallback:taskNo];
		}];
	} copy];
}

- (void (^)(HMHome *home, NSError *error))completionHandlerForCallbackWithHome:(xsSlot)callback
{
	xsMachine *the = _the;

	if (!xsTest(callback)) return nil;

	UInt32 taskNo = [self registerCallback:callback];

	return [^(HMHome *home, NSError *error) {
		[self sandboxWithVars:2 then:^(xsMachine *the) {
			xsVar(0) = [self callbackWithNo:taskNo];
			xsVar(1) = [self xsError:error];
			[self newHome:home err:NULL];

			xsCallFunction2(xsVar(0), xsThis, xsResult, xsVar(1));

			[self finishCallback:taskNo];
		}];
	} copy];
}

- (void (^)(HMRoom *room, NSError *error))completionHandlerForCallbackWithRoom:(xsSlot)callback
{
	xsMachine *the = _the;

	if (!xsTest(callback)) return nil;

	UInt32 taskNo = [self registerCallback:callback];

	return [^(HMRoom *room, NSError *error) {
		[self sandboxWithVars:2 then:^(xsMachine *the) {
			xsVar(0) = [self callbackWithNo:taskNo];
			xsVar(1) = [self xsError:error];
			[self newRoom:room err:NULL];

			xsCallFunction2(xsVar(0), xsThis, xsResult, xsVar(1));

			[self finishCallback:taskNo];
		}];
	} copy];
}

- (void (^)(HMZone *zone, NSError *error))completionHandlerForCallbackWithZone:(xsSlot)callback
{
	xsMachine *the = _the;

	if (!xsTest(callback)) return nil;

	UInt32 taskNo = [self registerCallback:callback];

	return [^(HMZone *zone, NSError *error) {
		[self sandboxWithVars:2 then:^(xsMachine *the) {
			xsVar(0) = [self callbackWithNo:taskNo];
			xsVar(1) = [self xsError:error];
			[self newZone:zone err:NULL];

			xsCallFunction2(xsVar(0), xsThis, xsResult, xsVar(1));

			[self finishCallback:taskNo];
		}];
	} copy];
}

- (void (^)(HMServiceGroup *, NSError *))completionHandlerForCallbackWithServiceGroup:(xsSlot)callback
{
	xsMachine *the = _the;

	if (!xsTest(callback)) return nil;

	UInt32 taskNo = [self registerCallback:callback];

	return [^(HMServiceGroup *serviceGroup, NSError *error) {
		[self sandboxWithVars:2 then:^(xsMachine *the) {
			xsVar(0) = [self callbackWithNo:taskNo];
			xsVar(1) = [self xsError:error];
			[self newServiceGroup:serviceGroup err:NULL];

			xsCallFunction2(xsVar(0), xsThis, xsResult, xsVar(1));

			[self finishCallback:taskNo];
		}];
	} copy];
}

- (void (^)(HMActionSet *, NSError *))completionHandlerForCallbackWithActionSet:(xsSlot)callback
{
	xsMachine *the = _the;

	if (!xsTest(callback)) return nil;

	UInt32 taskNo = [self registerCallback:callback];

	return [^(HMActionSet *actionSet, NSError *error) {
		[self sandboxWithVars:2 then:^(xsMachine *the) {
			xsVar(0) = [self callbackWithNo:taskNo];
			xsVar(1) = [self xsError:error];
			[self newActionSet:actionSet err:NULL];

			xsCallFunction2(xsVar(0), xsThis, xsResult, xsVar(1));

			[self finishCallback:taskNo];
		}];
	} copy];
}

- (void (^)(HMUser *, NSError *))completionHandlerForCallbackWithUser:(xsSlot)callback
{
	xsMachine *the = _the;

	if (!xsTest(callback)) return nil;

	UInt32 taskNo = [self registerCallback:callback];

	return [^(HMUser *user, NSError *error) {
		[self sandboxWithVars:2 then:^(xsMachine *the) {
			xsVar(0) = [self callbackWithNo:taskNo];
			xsVar(1) = [self xsError:error];
			[self newUser:user err:NULL];

			xsCallFunction2(xsVar(0), xsThis, xsResult, xsVar(1));

			[self finishCallback:taskNo];
		}];
	} copy];
}

- (void (^)(NSError *error))completionHandlerForCallback:(xsSlot)callback withArgument:(void (^)(xsMachine *the))argument
{
	xsMachine *the = _the;

	if (!xsTest(callback)) return nil;

	UInt32 taskNo = [self registerCallback:callback];

	return [^(NSError *error) {
		[self sandboxWithVars:2 then:^(xsMachine *the) {
			xsVar(0) = [self callbackWithNo:taskNo];
			xsVar(1) = [self xsError:error];
			argument(the);

			xsCallFunction2(xsVar(0), xsThis, xsResult, xsVar(1));

			[self finishCallback:taskNo];
		}];
	} copy];
}

// HMHomeManagerDelegate

- (void)onHomeManagerUpdate
{
	[self sandboxWithVars:0 callback:"onHomeManagerUpdate" then:^(xsMachine *the) {
		(void)xsCallFunction1(xsResult, xsThis, _manager);
	}];
}

- (void)homeManagerDidUpdateHomes:(HMHomeManager *)manager
{
	if (_ready) {
		[self onHomeManagerUpdate];
	} else {
		_ready = YES;
		[self sandboxWithVars:0 callback:"onHomeManagerReady" then:^(xsMachine *the) {
			(void)xsCallFunction1(xsResult, xsThis, _manager);
		}];
	}
}

- (void)homeManagerDidUpdatePrimaryHome:(HMHomeManager *)manager
{
	[self onHomeManagerUpdate];
}

- (void)homeManager:(HMHomeManager *)manager didAddHome:(HMHome *)home
{
	[self onHomeManagerUpdate];
}

- (void)homeManager:(HMHomeManager *)manager didRemoveHome:(HMHome *)home
{
	[self onHomeManagerUpdate];
}

// HMAccessoryBrowserDelegate

- (void)accessoryBrowser:(HMAccessoryBrowser *)browser didFindNewAccessory:(HMAccessory *)accessory
{
	[self sandboxWithVars:3 callback:"onHomeBrowserDidFindAccessory" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		KprHomeManager manager = xsGetHostData(_manager);
		KprHomeManagerGetBrowser(manager, the);
		xsVar(1) = xsResult;

		[self newAccessory:accessory err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)accessoryBrowser:(HMAccessoryBrowser *)browser didRemoveNewAccessory:(HMAccessory *)accessory
{
	[self sandboxWithVars:3 callback:"onHomeBrowserDidLoseAccessory" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		KprHomeManager manager = xsGetHostData(_manager);
		KprHomeManagerGetBrowser(manager, the);
		xsVar(1) = xsResult;

		[self newAccessory:accessory err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

// HMHomeDelegate

- (void)homeDidUpdateName:(HMHome *)home
{
	[self sandboxWithVars:2 callback:"onHomeDidUpdateName" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

// ---

- (void)home:(HMHome *)home didAddAccessory:(HMAccessory *)accessory
{
	[self sandboxWithVars:3 callback:"onHomeDidAddAccessory" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newAccessory:accessory err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didRemoveAccessory:(HMAccessory *)accessory
{
	[self sandboxWithVars:3 callback:"onHomeDidRemoveAccessory" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newAccessory:accessory err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

// ---

- (void)home:(HMHome *)home didAddUser:(HMUser *)user
{
	if ([user isKindOfClass:[HMUser class]] == NO) return;

	[self sandboxWithVars:3 callback:"onHomeDidAddUser" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newUser:(HMUser *)user err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didRemoveUser:(HMUser *)user
{
	if ([user isKindOfClass:[HMUser class]] == NO) return;

	[self sandboxWithVars:3 callback:"onHomeDidRemoveUser" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newUser:(HMUser *)user err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

// ---

- (void)home:(HMHome *)home didUpdateRoom:(HMRoom *)room forAccessory:(HMAccessory *)accessory
{
	[self sandboxWithVars:3 callback:"onHomeAccessoryDidUpdateRoom" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newAccessory:accessory err:NULL];
		xsVar(1) = xsResult;

		[self newRoom:room err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didAddRoom:(HMRoom *)room
{
	[self sandboxWithVars:3 callback:"onHomeDidAddRoom" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newRoom:room err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didRemoveRoom:(HMRoom *)room
{
	[self sandboxWithVars:3 callback:"onHomeDidRemoveRoom" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newRoom:room err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didUpdateNameForRoom:(HMRoom *)room
{
	[self sandboxWithVars:2 callback:"onHomeRoomDidUpdateName" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newRoom:room err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

// ---

- (void)home:(HMHome *)home didAddZone:(HMZone *)zone
{
	[self sandboxWithVars:3 callback:"onHomeDidAddZone" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newZone:zone err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didRemoveZone:(HMZone *)zone
{
	[self sandboxWithVars:3 callback:"onHomeDidRemoveZone" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newZone:zone err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didUpdateNameForZone:(HMZone *)zone
{
	[self sandboxWithVars:2 callback:"onHomeZoneDidUpdateName" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newZone:zone err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

- (void)home:(HMHome *)home didAddRoom:(HMRoom *)room toZone:(HMZone *)zone
{
	[self sandboxWithVars:3 callback:"onHomeZoneDidAddRoom" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newZone:zone err:NULL];
		xsVar(1) = xsResult;

		[self newRoom:room err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didRemoveRoom:(HMRoom *)room fromZone:(HMZone *)zone
{
	[self sandboxWithVars:3 callback:"onHomeZoneDidRemoveRoom" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newZone:zone err:NULL];
		xsVar(1) = xsResult;

		[self newRoom:room err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

// ---

- (void)home:(HMHome *)home didAddServiceGroup:(HMServiceGroup *)group
{
	[self sandboxWithVars:3 callback:"onHomeDidAddServiceGroup" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newServiceGroup:group err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didRemoveServiceGroup:(HMServiceGroup *)group
{
	[self sandboxWithVars:3 callback:"onHomeDidRemoveServiceGroup" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newServiceGroup:group err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didUpdateNameForServiceGroup:(HMServiceGroup *)group
{
	[self sandboxWithVars:2 callback:"onHomeServiceGroupDidUpdateName" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newServiceGroup:group err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

- (void)home:(HMHome *)home didAddService:(HMService *)service toServiceGroup:(HMServiceGroup *)group
{
	[self sandboxWithVars:3 callback:"onHomeServiceGroupDidAddService" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newServiceGroup:group err:NULL];
		xsVar(1) = xsResult;

		[self newService:service err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didRemoveService:(HMService *)service fromServiceGroup:(HMServiceGroup *)group
{
	[self sandboxWithVars:3 callback:"onHomeServiceGroupDidRemoveService" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newServiceGroup:group err:NULL];
		xsVar(1) = xsResult;

		[self newService:service err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

// ---

- (void)home:(HMHome *)home didAddActionSet:(HMActionSet *)actionSet
{
	[self sandboxWithVars:3 callback:"onHomeDidAddActionSet" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newActionSet:actionSet err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didRemoveActionSet:(HMActionSet *)actionSet
{
	[self sandboxWithVars:3 callback:"onHomeDidRemoveActionSet" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newActionSet:actionSet err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didUpdateNameForActionSet:(HMActionSet *)actionSet
{
	[self sandboxWithVars:2 callback:"onHomeActionSetDidUpdateName" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newActionSet:actionSet err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

- (void)home:(HMHome *)home didUpdateActionsForActionSet:(HMActionSet *)actionSet
{
	[self sandboxWithVars:2 callback:"onHomeActionSetDidUpdateActions" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newActionSet:actionSet err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

// ---

- (void)home:(HMHome *)home didAddTrigger:(HMTrigger *)trigger
{
	if ([trigger isKindOfClass:[HMTimerTrigger class]] == NO) return;

	[self sandboxWithVars:3 callback:"onHomeDidAddTrigger" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newTimerTrigger:(HMTimerTrigger *)trigger err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didRemoveTrigger:(HMTrigger *)trigger
{
	if ([trigger isKindOfClass:[HMTimerTrigger class]] == NO) return;

	[self sandboxWithVars:3 callback:"onHomeDidRemoveTrigger" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newHome:home err:NULL];
		xsVar(1) = xsResult;

		[self newTimerTrigger:(HMTimerTrigger *)trigger err:NULL];
		xsVar(2) = xsResult;

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

- (void)home:(HMHome *)home didUpdateNameForTrigger:(HMTrigger *)trigger
{
	if ([trigger isKindOfClass:[HMTimerTrigger class]] == NO) return;

	[self sandboxWithVars:2 callback:"onHomeTriggerDidUpdateName" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newTimerTrigger:(HMTimerTrigger *)trigger err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

- (void)home:(HMHome *)home didUpdateTrigger:(HMTrigger *)trigger
{
	if ([trigger isKindOfClass:[HMTimerTrigger class]] == NO) return;

	[self sandboxWithVars:3 callback:"onHomeTriggerDidUpdate" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newTimerTrigger:(HMTimerTrigger *)trigger err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

// ---

- (void)home:(HMHome *)home didUnblockAccessory:(HMAccessory *)accessory
{
	[self sandboxWithVars:2 callback:"onHomeAccessoryDidUnblock" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newAccessory:accessory err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

- (void)home:(HMHome *)home didEncounterError:(NSError *)error forAccessory:(HMAccessory *)accessory
{
	[self sandboxWithVars:3 callback:"onHomeAccessoryError" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newAccessory:accessory err:NULL];
		xsVar(1) = xsResult;

		xsVar(2) = [self xsError:error];

		(void)xsCallFunction2(xsVar(0), xsThis, xsVar(1), xsVar(2));
	}];
}

/*!
 * @brief Informs the delegate when the name of the accessory is modified.
 *
 * @param accessory Sender of the message.
 */
- (void)accessoryDidUpdateName:(HMAccessory *)accessory
{
	[self sandboxWithVars:2 callback:"onHomeAccessoryDidUpdateName" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newAccessory:accessory err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

/*!
 * @brief Informs the delegate when the name of a service is modfied.
 *
 * @param accessory Sender of the message.
 *
 * @param service Service whose name was modified.
 */
- (void)accessory:(HMAccessory *)accessory didUpdateNameForService:(HMService *)service
{
	[self sandboxWithVars:2 callback:"onHomeServiceDidUpdateName" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newService:service err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

/*!
 * @brief Informs the delegate when the associated service type of a service is modified.
 *
 * @param accessory Sender of the message.
 *
 * @param service Service whose associated service type was modified.
 */
- (void)accessory:(HMAccessory *)accessory didUpdateAssociatedServiceTypeForService:(HMService *)service
{
	[self sandboxWithVars:2 callback:"onHomeServiceDidUpdateAssociatedServiceType" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newService:service err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

/*!
 * @brief Informs the delegate when the services on the accessory have been dynamically updated.
 *        The services discovered are accessible via the 'services' property of the accessory.
 *
 * @param accessory Sender of the message.
 */
- (void)accessoryDidUpdateServices:(HMAccessory *)accessory
{
	[self sandboxWithVars:2 callback:"onHomeAccessoryDidUpdateServices" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newAccessory:accessory err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

/*!
 * @brief Informs the delegate when the reachability of the accessory changes.
 *
 * @param accessory Sender of the message.
 */
- (void)accessoryDidUpdateReachability:(HMAccessory *)accessory
{
	[self sandboxWithVars:2 callback:"onHomeAccessoryDidUpdateReachability" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newAccessory:accessory err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

/*!
 * @brief Informs the delegate of a change in value of a characteristic.
 *
 * @param accessory Sender of this messqage
 *
 * @param service HMService that contains the characteristic whose value was modified.
 *
 * @param characteristic The characteristic whose value was changed.
 */
- (void)accessory:(HMAccessory *)accessory service:(HMService *)service didUpdateValueForCharacteristic:(HMCharacteristic *)characteristic
{
	[self sandboxWithVars:2 callback:"onHomeCharacteristicDidUpdateValue" then:^(xsMachine *the) {
		xsVar(0) = xsResult;

		[self newCharacteristic:characteristic err:NULL];
		xsVar(1) = xsResult;

		(void)xsCallFunction1(xsVar(0), xsThis, xsVar(1));
	}];
}

@end

