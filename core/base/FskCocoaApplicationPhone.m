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
#define DELAY_FSK_RUN_LOOP

#include <AudioToolbox/AudioServices.h>

#define __FSKECMASCRIPT_PRIV__
#if defined(DELAY_FSK_RUN_LOOP)
#define __FSKTIME_PRIV__
#define __FSKTHREAD_PRIV__
#endif
#define __FSKWINDOW_PRIV__

#import "FskCocoaApplicationPhone.h"
#import "FskCocoaSupportPhone.h"
#if USE_TEXT_INPUT_VIEW
#import "FskCocoaTextInputViewControllerPhone.h"
#endif
#import "FskCocoaViewControllerPhone.h"
#import "FskCocoaViewPhone.h"
#import "FskECMAScript.h"
#if defined(DELAY_FSK_RUN_LOOP)
#import "FskTime.h"
#import "FskWindow.h"
#endif
#if GLES_VERSION == 1 && kFskCocoaCopyBitsUseOpenGL
#import <OpenGLES/ES1/gl.h>
#else
#import <OpenGLES/ES2/gl.h>
#endif

NSString *FskCocoaApplicationViewOrientationDidChangeNotification = @"FskCocoaApplicationViewOrientationDidChangeNotification";

@implementation FskCocoaApplication {
	UIBackgroundTaskIdentifier _background_task;
#if SUPPORT_REMOTE_NOTIFICATION
	NSDictionary *_remoteNotificationUserInfo;
#endif	/* SUPPORT_REMOTE_NOTIFICATION */
#if SUPPORT_EXTERNAL_SCREEN
	NSMutableArray *_externalWindows;
	NSMutableArray *_externalViewControllers;
#endif	/* SUPPORT_EXTERNAL_SCREEN */
}

@synthesize window = _window;
@dynamic mainViewController;

+ (FskCocoaApplication *)sharedApplication
{
	return (FskCocoaApplication *)[UIApplication sharedApplication].delegate;
}

- (FskCocoaViewController *)mainViewController
{
	return (FskCocoaViewController *)self.window.rootViewController;
}

#pragma mark --- UIApplicationDelegate ---
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	UIScreen *mainScreen;
	CGRect bounds;
    FskCocoaViewController *rootViewController;
    
	mainScreen = [UIScreen mainScreen];
	bounds = mainScreen.bounds;

	_window = [[UIWindow alloc] initWithFrame:bounds];
	if (_window == nil) return YES;

    rootViewController = [[FskCocoaViewController alloc] initWithFrame:bounds screenScale:mainScreen.scale];
    if (rootViewController == nil) return YES;

    _window.rootViewController = rootViewController;
    [_window addSubview:rootViewController.view];
    [rootViewController release];
    [_window makeKeyAndVisible];

#if SUPPORT_EXTERNAL_SCREEN
	_externalWindows = [[NSMutableArray alloc] initWithCapacity:0];
	_externalViewControllers = [[NSMutableArray alloc] initWithCapacity:0];
#endif	/* SUPPORT_EXTERNAL_SCREEN */

#if SUPPORT_REMOTE_NOTIFICATION
	_remoteNotificationUserInfo = [[launchOptions objectForKey:UIApplicationLaunchOptionsRemoteNotificationKey] retain];
#endif	/* SUPPORT_REMOTE_NOTIFICATION */

    //Extend the splash screen for a while
    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.5]];

	return YES;
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
	NSMutableString *buffer = [NSMutableString stringWithCapacity:100];
	[buffer setString:@"http://"];
	
	NSString *host = [url host];
	if (host) [buffer appendString:host];
	
	NSNumber *port = [url port];
	if (port) [buffer appendFormat:@":%@", port];
	
	NSString *path = [url path];
	if (path) [buffer appendString:path];
	
	NSString *query = [url query];
	if (query) [buffer appendFormat:@"?%@", query];
	
	NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:buffer]];
							
	[NSURLConnection sendAsynchronousRequest:request queue:[NSOperationQueue mainQueue] completionHandler:^(NSURLResponse *response, NSData*data, NSError *error) {
		NSLog(@"%@", [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease]);
	}];
	
	return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application
{
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	[self endBackgroundTask:application];
	
	FskEvent fskEvent;
    FskWindow fskWindow = self.mainViewController.fskWindow;

	[self.mainViewController setDisplayLinkActive:YES];
#if SUPPORT_EXTERNAL_SCREEN
	for (FskCocoaViewController *externalViewController in _externalViewControllers)
	{
		[externalViewController setDisplayLinkActive:YES];
	}
#endif	/* SUPPORT_EXTERNAL_SCREEN */

	if (fskWindow == NULL) return;

    if (FskEventNew(&fskEvent, kFskEventWindowActivated, NULL, kFskEventModifierNotSet) == kFskErrNone)
    {
        FskWindowEventSend(fskWindow, fskEvent);
        FskTimersSetUiActive(true);
	}
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
    FskWindow fskWindow = self.mainViewController.fskWindow;
	if (fskWindow != NULL)
	{
		FskEvent fskEvent;
		if (FskEventNew(&fskEvent, kFskEventWindowDeactivated, NULL, kFskEventModifierNotSet) == kFskErrNone)
		{
			FskWindowEventSend(fskWindow, fskEvent);
			FskTimersSetUiActive(false);
		}
	}
	[self.mainViewController setDisplayLinkActive:NO];
#if SUPPORT_EXTERNAL_SCREEN
	for (FskCocoaViewController *externalViewController in _externalViewControllers)
	{
		[externalViewController setDisplayLinkActive:NO];
	}
#endif	/* SUPPORT_EXTERNAL_SCREEN */
	FskECMAScriptHibernate();
#if FSKBITMAP_OPENGL
	glFinish();
#endif
	_background_task = [application beginBackgroundTaskWithExpirationHandler:^ {
		[self endBackgroundTask:application];
	}];
	
	dispatch_time_t when = dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC);
	dispatch_after(when, dispatch_get_main_queue(), ^{
		[self endBackgroundTask:application];
	});
}

- (void)applicationWillResignActive:(UIApplication*)application
{
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
#if SUPPORT_REMOTE_NOTIFICATION
	[self resetNotifications];
#endif	/* SUPPORT_REMOTE_NOTIFICATION */
}

- (void)endBackgroundTask:(UIApplication*)application
{
	if (_background_task != UIBackgroundTaskInvalid) {
		[application endBackgroundTask:_background_task];
		_background_task = UIBackgroundTaskInvalid;
	}
}

- (void)application:(UIApplication *)application didChangeStatusBarFrame:(CGRect)oldStatusBarFrame
{
    FskWindow fskWindow = self.mainViewController.fskWindow;

	if (fskWindow == NULL) return;
#if !FSKBITMAP_OPENGL
	FskWindowCocoaSizeChanged(fskWindow);
#endif
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    FskECMAScriptHibernate();
    FskNotificationPost(kFskNotificationLowMemory);
}

- (void)dealloc
{
    [_window release];
#if SUPPORT_EXTERNAL_SCREEN
	[_externalWindows release];
	[_externalViewControllers release];
#endif	/* SUPPORT_EXTERNAL_SCREEN */
    [super dealloc];
}

#pragma mark --- push notification ---

#if SUPPORT_REMOTE_NOTIFICATION
- (void)resetNotifications
{
	UIApplication *app = [UIApplication sharedApplication];
	[app setApplicationIconBadgeNumber:0];
	[app cancelAllLocalNotifications];
}

- (void)handleRemoteNotification
{
	if (_remoteNotificationUserInfo == nil)
	{
		return;
	}

	if ([NSJSONSerialization isValidJSONObject:_remoteNotificationUserInfo])
	{
		NSData *data = [NSJSONSerialization dataWithJSONObject:_remoteNotificationUserInfo options:NSJSONWritingPrettyPrinted error:NULL];
		NSString *string = [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease];
		const char *cstring = [string UTF8String];
		//NSLog(@"JSON %s", cstring);
		FskEvent fskEvent;
		if (FskEventNew(&fskEvent, kFskEventSystemRemoteNotification, NULL, kFskEventModifierNotSet) == kFskErrNone) {
			FskWindow fskWindow = self.mainViewController.fskWindow;
			(void)FskEventParameterAdd(fskEvent, kFskEventParameterStringValue, FskStrLen(cstring) + 1, cstring);
			//FskWindowEventSend(fskWindow, fskEvent);	// Window??
			FskWindowEventQueue(fskWindow, fskEvent);	// Window??
		}
	}

	[self resetNotifications];

	[_remoteNotificationUserInfo release];
	_remoteNotificationUserInfo = nil;
}

- (void)sendRemoteNotificationRegsteredEvent:(NSString *)deviceToken
{
	FskEvent fskEvent;

	if (FskEventNew(&fskEvent, kFskEventSystemRemoteNotificationRegistered, NULL, kFskEventModifierNotSet) == kFskErrNone) {
		const char *token = [deviceToken UTF8String];
		FskWindow fskWindow = self.mainViewController.fskWindow;
		(void)FskEventParameterAdd(fskEvent, kFskEventParameterStringValue, FskStrLen(token) + 1, token);
		//FskWindowEventSend(fskWindow, fskEvent);	// Window??
		FskWindowEventQueue(fskWindow, fskEvent);	// Window??
	}

}

- (void)application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken
{
	NSString *deviceTokenString = [[deviceToken description] stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"<>"]];

	deviceTokenString = [deviceTokenString stringByReplacingOccurrencesOfString:@" " withString:@""];

	[self sendRemoteNotificationRegsteredEvent:deviceTokenString];

//    NSLog(@"didRegisterForRemoteNotificationsWithDeviceToken: %@", deviceTokenString);
}

- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
	[self sendRemoteNotificationRegsteredEvent:@""];

//	NSLog(@"didFailToRegisterForRemoteNotificationsWithError");
}

#if defined (__IPHONE_8_0) && (__IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_8_0)
- (void)application:(UIApplication *)application didRegisterUserNotificationSettings:(UIUserNotificationSettings *)notificationSettings
{
//	NSLog(@"didRegisterUserNotificationSettings");
}
#endif

- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo
{
	_remoteNotificationUserInfo = [userInfo retain];
	[self handleRemoteNotification];
}
#endif	/* SUPPORT_REMOTE_NOTIFICATION */

#pragma mark --- external screen ---

#if SUPPORT_EXTERNAL_SCREEN
#if TEST_EXTERNAL_SCREEN
Boolean extEventHandler(FskEvent event, UInt32 eventCode, FskWindow window, void *it)
{
	Boolean result = false;
	switch (eventCode) {
		case kFskEventWindowClose:
			break;
		case kFskEventWindowActivated:
			break;
		case kFskEventWindowDeactivated:
			break;

		case kFskEventWindowUpdate: {
			FskCocoaViewController *controller = (FskCocoaViewController *)window->uiWindowController;
			if ((controller != nil) && (controller.bitmap != NULL))
			{
				//				NSLog(@"bitmaDraw start");
				FskRectangleRecord srcRect, dstRect;
				FskBitmapGetBounds(controller.bitmap, &srcRect);
				FskPort port = window->port;
				FskPortGetBounds(port, &dstRect);
				FskPortBitmapDraw(port, controller.bitmap, &srcRect, &dstRect);
				FskPortResetInvalidRectangle(port);
				//				NSLog(@"bitmaDraw end");
			}
		}
			break;
		case kFskEventWindowAfterUpdate:
			break;
		case kFskEventWindowBeforeUpdate:
			break;

		case kFskEventWindowBitmapChanged:
			break;

		case kFskEventWindowBeforeResize:
			break;
		case kFskEventWindowResize:
			break;
		case kFskEventWindowAfterResize:
			result = true;
			break;
		case kFskEventMouseDown:
			break;
		case kFskEventRightMouseDown:
			break;
		case kFskEventMouseDoubleClick:
			break;
		case kFskEventMouseUp:
			break;
		case kFskEventRightMouseUp:
			break;

		case kFskEventMouseLeave:
			break;
		case kFskEventMouseMoved:
			break;
		case kFskEventMouseStillDown:
			break;
		case kFskEventMouseWheel:
			break;

		case kFskEventKeyDown:
			break;
		case kFskEventKeyUp:
			break;

		case kFskEventApplication:
			break;
		case kFskEventClipboardChanged:
			break;
		case kFskEventMenuCommand:
			break;
		case kFskEventMenuStatus:
			break;
		case kFskEventWindowOpenFiles:
			break;
		case kFskEventWindowDragEnter:
			break;
		case kFskEventWindowDragLeave:
			break;
		case kFskEventButton:
			break;
		case kFskEventWindowTransition:
			break;
	}

	return result;
}
#endif	/* TEST_EXTERNAL_SCREEN */

- (void)addExternalScreen:(UIScreen *)extScreen
{
	FskCocoaViewController *extViewController;
	UIWindow *extWindow;
	NSArray	*availableModes = [extScreen availableModes];
	NSInteger selectedRow = [availableModes count] - 1;
	CGRect bounds = extScreen.bounds;
	FskDimensionRecord size;

	extScreen.currentMode = [availableModes objectAtIndex:selectedRow];
	extScreen.overscanCompensation = UIScreenOverscanCompensationInsetApplicationFrame;

	extWindow = [[[UIWindow alloc] initWithFrame:bounds] autorelease];

	extViewController = [[[FskCocoaViewController alloc] initWithFrame:bounds screenScale:extScreen.scale] autorelease];
	[extWindow addSubview:extViewController.view];
	[extWindow makeKeyAndVisible];

	[_externalWindows addObject:extWindow];
	[_externalViewControllers addObject:extViewController];

	FskDimensionSet(&size, bounds.size.width, bounds.size.height);
	FskExtScreenHandleConnected((int)extScreen, &size);

#if TEST_EXTERNAL_SCREEN
	FskWindowEventSetHandler(extViewController.fskWindow, extEventHandler, NULL);
#endif

	extWindow.screen = extScreen;

	[extViewController setDisplayLinkActive:YES];
}

- (void)removeExternalScreen:(UIScreen *)extScreen
{
	FskCocoaViewController *extViewController;
	UIWindow *extWindow;

	FskExtScreenHandleDisconnected((int)extScreen);

	for (NSUInteger i = 0; i < _externalWindows.count; i++)
	{
		extWindow = [_externalWindows objectAtIndex:i];

		if (![extWindow.screen isEqual:extScreen])
			continue;

		extViewController = [_externalViewControllers objectAtIndex:i];

		[extViewController setDisplayLinkActive:NO];

		[_externalWindows removeObjectAtIndex:i];
		[_externalViewControllers removeObjectAtIndex:i];

		break;
	}
}

- (void)updateExternalScreen:(UIScreen *)extScreen
{
	CGRect bounds = extScreen.bounds;
	FskDimensionRecord size;
	FskCocoaViewController *extViewController;
	UIWindow *extWindow;

	for (NSUInteger i = 0; i < _externalWindows.count; i++)
	{
		extWindow = [_externalWindows objectAtIndex:i];

		if (![extWindow.screen isEqual:extScreen])
			continue;

		extWindow.frame = bounds;

		extViewController = [_externalViewControllers objectAtIndex:i];
		extViewController.view.frame = bounds;

		break;
	}

	FskDimensionSet(&size, bounds.size.width, bounds.size.height);
	FskExtScreenHandleChanged((int)extScreen, &size);
}

#if TEST_EXTERNAL_SCREEN
- (FskCocoaViewController *)getExternalViewControllerAtIndex:(NSUInteger)index
{
	if (index >= _externalViewControllers.count)
		return nil;

	return [_externalViewControllers objectAtIndex:index];
}
#endif
#endif

#pragma mark --- methods ---

#if defined(DELAY_FSK_RUN_LOOP)
- (void)runFskRunLoopCore
{
	FskThreadRunloopCycle(0);
}

- (BOOL)hasTimerCallback:(FskThread)thread
{
	FskTimeCallBack cb;
	FskTimeRecord	now;

	cb = (FskTimeCallBack)thread->timerCallbacks;
	if (NULL == cb)
		return NO;

	FskTimeGetNow(&now);

	return (FskTimeCompare(&now, &cb->trigger) > 0) ? NO : YES;
}

- (BOOL)windowCheckEvents:(FskThread)thread withWindow:(FskWindow)win
{
	if ((win != NULL) && (win->thread == thread) && !FskListMutexIsEmpty(win->eventQueue))
		return YES;

	return NO;
}

- (BOOL)windowCheckEvents:(FskThread)thread
{
	if ([self windowCheckEvents:thread withWindow:self.mainViewController.fskWindow])
		return YES;

#if SUPPORT_EXTERNAL_SCREEN
	for (FskCocoaViewController *externalViewController in _externalViewControllers)
	{
		if ([self windowCheckEvents:thread withWindow:externalViewController.fskWindow])
			return YES;
	}
#endif	/* SUPPORT_EXTERNAL_SCREEN */

	return NO;
}

- (BOOL)needsRunFskRunLoop
{
	FskThread thread;

	thread = FskThreadGetCurrent();
	if ([self hasTimerCallback:thread])
		return YES;

	if (!FskListMutexIsEmpty(thread->eventQueue))
		return YES;

	if ([self windowCheckEvents:thread])
		return YES;

	return NO;
}
#endif

- (void)runFskRunLoop
{
#if defined(DELAY_FSK_RUN_LOOP)
	if ([self needsRunFskRunLoop]) {
		[self performSelector:@selector(runFskRunLoopCore) withObject:nil afterDelay:0.0];
	}
#else
    FskThreadRunloopCycle(0);
#endif
}

#if USE_TEXT_INPUT_VIEW
- (void)showTextInputWithTitle:(NSString*)title initialValue:(NSString*)initialValue machine:(void*)the
{
    FskCocoaTextInputViewController *tivc;
    UINavigationController *navigation;

    tivc = [[FskCocoaTextInputViewController alloc] init];
	(void)tivc.view;
	tivc.textView.text = initialValue;
    tivc.title = title;
	tivc.the = the;
    navigation = [[[UINavigationController alloc] initWithRootViewController:tivc] autorelease];
    
    [navigation setModalTransitionStyle:UIModalTransitionStyleCoverVertical];
    [self.window.rootViewController presentModalViewController:navigation animated:YES];
}

- (void)textEntered:(NSString *)textEntered machine:(void *)the
{
	xsBeginHost(the);
	xsCall1(xsGet(xsGet(xsGlobal, xsID("kp5")), xsID("phone")), xsID("doneTextInput"), xsString((char *)[textEntered UTF8String]));
	xsEndHost(the);
}
#endif

- (void)startViewOrientationNotification
{
    ((FskCocoaViewController*)_window.rootViewController).notifyViewOrientation = YES;
}

- (void)stopViewOrientationNotification
{
    ((FskCocoaViewController*)_window.rootViewController).notifyViewOrientation = NO;
}

- (SInt32)viewRotation
{
    SInt32 rotation;

    switch (_window.rootViewController.interfaceOrientation) {
        case UIInterfaceOrientationPortraitUpsideDown:
            rotation = 180;
            break;
		case UIDeviceOrientationLandscapeLeft:
			rotation = 90;
			break;
		case UIDeviceOrientationLandscapeRight:
			rotation = 270;
			break;
        default:
            rotation = 0;
            break;
    }

    return rotation;
}

- (FskCocoaViewController *)getEmptyController
{
	if (self.mainViewController.empty)
	{
		return self.mainViewController;
	}

#if SUPPORT_EXTERNAL_SCREEN
	for (FskCocoaViewController *extController in _externalViewControllers)
	{
		if (extController.empty)
		{
			return extController;
		}
	}
#endif

	return nil;
}

@end

