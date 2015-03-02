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
#import <Cocoa/Cocoa.h>
#import "FskCocoaMain.h"
#import "FskEnvironment.h"
#import "FskMain.h"
#import "FskString.h"

static Boolean gExpired = false;
static Boolean checkExpired(void);

#pragma mark --- main ---
FskErr FskCocoaMain(UInt32 flags, int argc, char **argv)
{
	FskErr 				err = kFskErrNone;
	NSAutoreleasePool 	*autoreleasePool = nil;
#if defined(MAC_OS_X_VERSION_10_8) && (MAC_OS_X_VERSION_10_8 <= MAC_OS_X_VERSION_MAX_ALLOWED)
	NSArray *array;
#endif

	const char				*kiosk;
	// create the autorelease pool
	autoreleasePool = [[NSAutoreleasePool alloc] init];
	
	[NSApplication sharedApplication];
	
	// enter multithread
	[NSThread detachNewThreadSelector:@selector(enterMultiThreadedMode) toTarget:[NSApp delegate] withObject:nil];

	// load tinyhttp bundle
#if defined(MAC_OS_X_VERSION_10_8) && (MAC_OS_X_VERSION_10_8 <= MAC_OS_X_VERSION_MAX_ALLOWED)
	[[NSBundle mainBundle] loadNibNamed:@"fsk" owner:NSApp topLevelObjects:&array];
#else
	[NSBundle loadNibNamed:@"fsk" owner:NSApp];
#endif

	// initialize
	err = FskMainInitialize(flags, argc, argv);
	BAIL_IF_ERR(err);

	kiosk = FskEnvironmentGet("kiosk");
	if ((NULL != kiosk) && (0 == FskStrCompare("true", kiosk)))
		[NSApp setPresentationOptions: NSApplicationPresentationAutoHideMenuBar | NSApplicationPresentationAutoHideDock];

	mainExtensionInitialize();

   for( int j = 0; j < argc; j++ )
   {
      if( strcmp( argv[j], "--bring-front" ) == 0 ) {
         [[NSRunningApplication currentApplication] activateWithOptions:NSApplicationActivateIgnoringOtherApps];
         break;
      }   
   }

	  if (!checkExpired())
		 err = FskMainApplicationLoop();		// run

	// terminate
	mainExtensionTerminate();
	FskMainTerminate();

bail:
	[NSApp release];
//@@	[autoreleasePool release];		//@@ hack-o-rama. when this line is enabled it leads to a crash on exit fairly often. 

	return err;
}

void fskExpired(void)
{
	gExpired = true;
}

Boolean checkExpired(void)
{
	if (!gExpired)
		return false;

	NSAlert *alert = [[NSAlert alloc] init];
	[alert setMessageText:@"Kinoma Create has expired"];
	[alert setInformativeText:@"Please update to get the latest Kinoma Create for your Mac."];
	[alert runModal];
	[alert release];
	
	return true;
}


FskErr FskCocoaMainKpr(UInt32 flags, int argc, char **argv)
{
	FskErr              err = kFskErrNone;
	NSAutoreleasePool   *autoreleasePool = nil;
#if defined(MAC_OS_X_VERSION_10_8) && (MAC_OS_X_VERSION_10_8 <= MAC_OS_X_VERSION_MAX_ALLOWED)
	static NSArray *array;
#endif

	const char              *kiosk;
	// create the autorelease pool
	autoreleasePool = [[NSAutoreleasePool alloc] init];
	[NSApplication sharedApplication];

	// enter multithread
	[NSThread detachNewThreadSelector:@selector(enterMultiThreadedMode) toTarget:[NSApp delegate] withObject:nil];

	// load tinyhttp bundle
#if defined(MAC_OS_X_VERSION_10_8) && (MAC_OS_X_VERSION_10_8 <= MAC_OS_X_VERSION_MAX_ALLOWED)
	[[NSBundle mainBundle] loadNibNamed:@"fsk" owner:NSApp topLevelObjects:&array];
#else
	[NSBundle loadNibNamed:@"fsk" owner:NSApp];
#endif

	// initialize
	err = FskMainInitialize(flags, argc, argv);
	BAIL_IF_ERR(err);
	kiosk = FskEnvironmentGet("kiosk");
	if ((NULL != kiosk) && (0 == FskStrCompare("true", kiosk)))
		[NSApp setPresentationOptions: NSApplicationPresentationAutoHideMenuBar | NSApplicationPresentationAutoHideDock];
	
	mainExtensionInitialize();


	bail:
	return err;
}

void FskCocoaMainKprRelease(void)
{
	mainExtensionTerminate();
	[NSApp release];
	FskMainTerminate();
	//@@    [autoreleasePool release];      //@@ hack-o-rama. when this line is enabled it leads to a crash on exit fairly often. 
}

