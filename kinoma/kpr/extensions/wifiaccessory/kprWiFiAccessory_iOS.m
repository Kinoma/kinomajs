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
#include "kprWiFiAccessory.h"

#import "FskCocoaApplicationPhone.h"
#import <ExternalAccessory/ExternalAccessory.h>

@class KprWiFiAccessoryBrowserDelegate;

struct KprWiFiAccessoryBrowserStruct {
	EAWiFiUnconfiguredAccessoryBrowser	*native;
	KprWiFiAccessoryBrowserDelegate		*delegate;
};

struct KprWiFiAccessoryStruct {
	EAWiFiUnconfiguredAccessory			*native;
	KprWiFiAccessoryBrowser				browser;
};

@interface KprWiFiAccessoryBrowserDelegate : NSObject<EAWiFiUnconfiguredAccessoryBrowserDelegate>
- (instancetype)initWith:(xsMachine *)the code:(xsIndex *)code browser:(xsSlot)browser;
@end

@implementation KprWiFiAccessoryBrowserDelegate {
	xsMachine* _the;
	xsIndex* _code;
	xsSlot _browser;
}

- (instancetype)initWith:(xsMachine *)the code:(xsIndex *)code browser:(xsSlot)browser
{
	self = [super init];
	if (self) {
		_the = the;
		_code = code;
		_browser = browser;
	}
	return self;
}

- (void)sandboxWithVars:(NSInteger)n then:(void (^)(xsMachine *the))block
{
	xsBeginHostSandboxCode(_the, _code);
	xsThis = xsGet(_browser, xsID_behavior);
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
		if (xsFindResult(_browser, xsID(name))) {
			block(the);
		}
	}];
}

- (void)accessoryBrowser:(EAWiFiUnconfiguredAccessoryBrowser *)browser didFindUnconfiguredAccessories:(NSSet *)accessories
{
	NSLog(@"didFindUnconfiguredAccessories: %@", accessories);
	[self sandboxWithVars:0 callback:"onWiFiAccessoryBrowserDidFindAccessory" then:^(xsMachine *the) {
		(void)xsCallFunction1(xsResult, xsThis, _browser);
	}];
	
}

- (void)accessoryBrowser:(EAWiFiUnconfiguredAccessoryBrowser *)browser didRemoveUnconfiguredAccessories:(NSSet *)accessories
{
	NSLog(@"didRemoveUnconfiguredAccessories%@", accessories);
	[self sandboxWithVars:0 callback:"onWiFiAccessoryBrowserDidLoseAccessory" then:^(xsMachine *the) {
		(void)xsCallFunction1(xsResult, xsThis, _browser);
	}];
}

- (void)accessoryBrowser:(EAWiFiUnconfiguredAccessoryBrowser *)browser didFinishConfiguringAccessory:(EAWiFiUnconfiguredAccessory *)accessory withStatus:(EAWiFiUnconfiguredAccessoryConfigurationStatus)status
{
	NSLog(@"didFinishConfiguringAccessory %@", accessory);

	[self sandboxWithVars:0 callback:"onWiFiAccessoryBrowserDidFinishConfiguringAccessory" then:^(xsMachine *the) {
		(void)xsCallFunction1(xsResult, xsThis, _browser);
	}];
}

@end

FskErr KprWiFiAccessoryBrowserNew(KprWiFiAccessoryBrowser *it, xsMachine *the, xsIndex *code, xsSlot this)
{
	FskErr err = kFskErrNone;
	KprWiFiAccessoryBrowser self = NULL;

	NSLog(@"KprWiFiAccessoryBrowserNew");

	bailIfError(FskMemPtrNew(sizeof(KprWiFiAccessoryBrowser), &self));

#if TARGET_IPHONE_SIMULATOR
	self->delegate = [[KprWiFiAccessoryBrowserDelegate alloc] initWith:the code:code browser:this];
	self->native = nil;
#else
	self->delegate = [[KprWiFiAccessoryBrowserDelegate alloc] initWith:the code:code browser:this];
	self->native = [[EAWiFiUnconfiguredAccessoryBrowser alloc] initWithDelegate:self->delegate queue:nil];
#endif

	*it = self;

bail:
	if (err) {
		KprWiFiAccessoryBrowserDispose(self);
	}

	return err;
}

FskErr KprWiFiAccessoryBrowserDispose(KprWiFiAccessoryBrowser self)
{
	FskErr err = kFskErrNone;

	if (self) {
		if (self->native)
			[self->native release];

		if (self->delegate)
			[self->delegate release];

		self->native = nil;
		self->delegate = nil;

		FskMemPtrDispose(self);
	}
	
	return err;
}

FskErr KprWiFiAccessoryBrowserGetAccessories(KprWiFiAccessoryBrowser self, xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprWiFiAccessory accessory = NULL;

	xsEnterSandbox();
	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	if (self->native) {
		for (EAWiFiUnconfiguredAccessory *native in self->native.unconfiguredAccessories.allObjects) {
			NSLog(@"Accessory %@", native);
			bailIfError(KprWiFiAccessoryNew(&accessory));
			accessory->native = [native retain];
			accessory->browser = self;

			xsResult = xsGet(xsGlobal, xsID("WiFiAccessory"));
			xsResult = xsGet(xsResult, xsID("accessory"));
			xsResult = xsNewInstanceOf(xsResult);
			xsSetHostData(xsResult, accessory);
			
			xsCall1(xsVar(0), xsID("push"), xsResult);
			accessory = NULL;
		}
	}
	else {
#if TARGET_IPHONE_SIMULATOR
		//Fake one accessory with no native
		bailIfError(KprWiFiAccessoryNew(&accessory));
		accessory->native = nil;
		accessory->browser = self;

		xsResult = xsGet(xsGlobal, xsID("WiFiAccessory"));
		xsResult = xsGet(xsResult, xsID("accessory"));
		xsResult = xsNewInstanceOf(xsResult);
		xsSetHostData(xsResult, accessory);
		
		xsCall1(xsVar(0), xsID("push"), xsResult);
		accessory = NULL;
#endif
	}

bail:
	xsResult = xsVar(0);
	xsLeaveSandbox();

	KprWiFiAccessoryDispose(accessory);
	return err;
}

FskErr KprWiFiAccessoryBrowserStart(KprWiFiAccessoryBrowser self)
{
	FskErr err = kFskErrNone;

	NSLog(@"KprWiFiAccessoryBrowserStart");

	if (self->native)
		[self->native startSearchingForUnconfiguredAccessoriesMatchingPredicate:nil];
	else {
#if TARGET_IPHONE_SIMULATOR
		[self->delegate accessoryBrowser:nil didFindUnconfiguredAccessories:nil];
#endif	
	}

	return err;
}

FskErr KprWiFiAccessoryBrowserStop(KprWiFiAccessoryBrowser self)
{
	FskErr err = kFskErrNone;

	if (self->native)
		[self->native stopSearchingForUnconfiguredAccessories];

	return err;
}

FskErr KprWiFiAccessoryNew(KprWiFiAccessory *it)
{
	FskErr err = kFskErrNone;
	KprWiFiAccessory self = NULL;

	bailIfError(FskMemPtrNew(sizeof(KprWiFiAccessory), &self));
	
	*it = self;

bail:
	return err;
}

FskErr KprWiFiAccessoryDispose(KprWiFiAccessory self)
{
	FskErr err = kFskErrNone;

	if (self) {
		if (self->native)
			[self->native release];

		self->native = nil;
		self->browser = nil;

		FskMemPtrDispose(self);
	}

	return err;
}

FskErr KprWiFiAccessoryGetName(KprWiFiAccessory self, xsMachine *the)
{
	FskErr err = kFskErrNone;

	if (self->native) {
		const char * name = [self->native.name UTF8String];
		xsResult = xsString((char *) name);
	}
	else {
#if TARGET_IPHONE_SIMULATOR
		xsResult = xsString("Test on Simulator");
#endif
	}

	return err;
}

FskErr KprWiFiAccessoryConfigure(KprWiFiAccessory self, xsSlot callback)
{
	FskErr err = kFskErrNone;

	if (self->browser->native) {
		FskCocoaViewController *viewController = [FskCocoaApplication sharedApplication].mainViewController;
		[self->browser->native configureAccessory:self->native withConfigurationUIOnViewController:viewController];
	}
	else {
#if TARGET_IPHONE_SIMULATOR
		[self->browser->delegate accessoryBrowser:nil didFinishConfiguringAccessory:nil withStatus:1];
#endif
	}

	return err;
}
