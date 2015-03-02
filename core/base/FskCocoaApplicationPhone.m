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
}

@synthesize window = _window;
@dynamic rootView;

+ (FskCocoaApplication *)sharedApplication
{
	return (FskCocoaApplication *)[UIApplication sharedApplication].delegate;
}

- (UIView *)rootView
{
    return _window.rootViewController.view;
}

#pragma mark --- methods ---
- (void)applicationDidFinishLaunching:(UIApplication *)application
{
	CGRect		bounds;
    FskCocoaViewController *rootViewController;
    
    //NSLog(@"applicationDidFinishLaunching start");

	bounds = [[UIScreen mainScreen] bounds];

	_window = [[UIWindow alloc] initWithFrame:bounds];
	if (_window == nil) return;

    rootViewController = [[FskCocoaViewController alloc] init];
    if (rootViewController == nil) return;

    [_window addSubview:rootViewController.view];
    _window.rootViewController = rootViewController;
    [rootViewController release];
    [_window makeKeyAndVisible];

    //NSLog(@"applicationDidFinishLaunching done");
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
    FskWindow fskWindow = FskWindowGetActive();

	[(FskCocoaViewController*)_window.rootViewController setDisplayLinkActive:YES];

	if (fskWindow == NULL) return;

    if (FskEventNew(&fskEvent, kFskEventWindowActivated, NULL, kFskEventModifierNotSet) == kFskErrNone)
    {
        FskWindowEventSend(fskWindow, fskEvent);
        FskTimersSetUiActive(true);
	}
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
	FskWindow fskWindow = FskWindowGetActive();
	if (fskWindow != NULL)
	{
		FskEvent fskEvent;
		if (FskEventNew(&fskEvent, kFskEventWindowDeactivated, NULL, kFskEventModifierNotSet) == kFskErrNone)
		{
			FskWindowEventSend(fskWindow, fskEvent);
			FskTimersSetUiActive(false);
		}
	}
	[(FskCocoaViewController*)_window.rootViewController setDisplayLinkActive:NO];
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
	FskWindow fskWindow = FskWindowGetActive();

	if (fskWindow == NULL) return;
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
    FskWindow fskWindow = FskWindowGetActive();

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
    [super dealloc];
}

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

- (BOOL)windowCheckEvents:(FskThread)thread
{
	FskWindow	win = NULL;

	for (UIView *view in self.rootView.subviews) {
		if (![view isKindOfClass:[FskCocoaView class]])
			continue;

		win = [(FskCocoaView *)view fskWindow];
		if (win == NULL)
			continue;

		if (win->thread == thread) {
			return !FskListMutexIsEmpty(win->eventQueue) ? YES : NO;
		}
	}

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

@end

