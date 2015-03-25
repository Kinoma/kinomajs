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
#define __FSKWINDOW_PRIV__

#import "FskCocoaViewControllerPhone.h"

#import "FskCocoaApplicationPhone.h"
#import "FskCocoaSupportPhone.h"
#import "FskCocoaViewPhone.h"
#import "FskEnvironment.h"
#import "FskMain.h"

extern void mainExtensionInitialize(void);
extern void mainExtensionTerminate(void);

@interface FskCocoaViewController () {
	CGRect _initialFrame;
	CADisplayLink *displayLink;
}

- (void)fskInitialize;
- (void)fskTerminate;

@end

@implementation FskCocoaViewController

@dynamic fskWindow;

- (FskCocoaView *)cocoaView
{
	if (self.view.subviews.count == 0)
		return nil;
	return (FskCocoaView *)[self.view.subviews lastObject];
}

- (FskWindow)fskWindow
{
	if (self.cocoaView == nil)
		return NULL;
	return self.cocoaView.fskWindow;
}

- (BOOL)empty
{
	return (self.view.subviews.count == 0) ? YES : NO;
}

- (id)initWithFrame:(CGRect)frame screenScale:(CGFloat)screenScale
{
	self = [super init];
	if (self != nil)
	{
		_initialFrame = frame;	// @@ may change before view is loaded?
		_screenScale = screenScale;
	}
	return self;
}

- (void)loadView
{
    self.view = [[[UIView alloc] initWithFrame:_initialFrame] autorelease];
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.notifyViewOrientation = NO;
#if (__IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_7_0)
    self.wantsFullScreenLayout = YES;
#endif
}

- (void)dealloc
{
#if SUPPORT_EXTERNAL_SCREEN
	if (self == [FskCocoaApplication sharedApplication].mainViewController)
	{
		[[NSNotificationCenter defaultCenter] removeObserver:self
														name:UIScreenDidConnectNotification
													  object:nil];

		[[NSNotificationCenter defaultCenter] removeObserver:self
														name:UIScreenDidDisconnectNotification
													  object:nil];

		[[NSNotificationCenter defaultCenter] removeObserver:self
														name:UIScreenModeDidChangeNotification
													  object:nil];
	}
#endif	/* SUPPORT_EXTERNAL_SCREEN */

    self.view = nil;
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];

	if (self != [FskCocoaApplication sharedApplication].mainViewController)
		return;

    [self performSelector:@selector(fskInitialize) withObject:nil afterDelay:0.0];
}

- (void)viewDidUnload
{
    [super viewDidUnload];

	if (self != [FskCocoaApplication sharedApplication].mainViewController)
		return;

    if (self.view)
    {
        FskEvent 	fskEvent;
        UInt32		commandID = 0;

		// send Fsk quit event
		if (kFskErrNone == FskEventNew(&fskEvent, kFskEventMenuCommand, NULL, kFskEventModifierNotSet))
		{
			FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(commandID), &commandID);
			FskWindowEventQueue(self.fskWindow, fskEvent);
		}
    }
    [self fskTerminate];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];

	if (self != [FskCocoaApplication sharedApplication].mainViewController)
		return;

	[[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
    [self becomeFirstResponder];
}

- (void)viewWillDisappear:(BOOL)animated {
	if (self == [FskCocoaApplication sharedApplication].mainViewController)
	{
		[[UIApplication sharedApplication] endReceivingRemoteControlEvents];
		[self resignFirstResponder];
	}
    [super viewWillDisappear:animated];
}

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
- (UIStatusBarStyle)preferredStatusBarStyle
{
    return UIStatusBarStyleLightContent;
}

- (BOOL)shouldAutorotate
{
    return YES;
}

static NSUInteger gSupportedInterfaceOrientations = 0;

- (NSUInteger)supportedInterfaceOrientations
{
    if (gSupportedInterfaceOrientations == 0)
    {
        NSArray *orientations = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"UISupportedInterfaceOrientations"];

        for (NSString *orientation in orientations)
        {
            if ([orientation isEqualToString:@"UIInterfaceOrientationPortrait"])
            {
                gSupportedInterfaceOrientations |= UIInterfaceOrientationMaskPortrait;
            }
            else if ([orientation isEqualToString:@"UIInterfaceOrientationPortraitUpsideDown"])
            {
                gSupportedInterfaceOrientations |= UIInterfaceOrientationMaskPortraitUpsideDown;
            }
            else if ([orientation isEqualToString:@"UIInterfaceOrientationLandscapeLeft"])
            {
                gSupportedInterfaceOrientations |= UIInterfaceOrientationMaskLandscapeLeft;
            }
            else if ([orientation isEqualToString:@"UIInterfaceOrientationLandscapeRight"])
            {
                gSupportedInterfaceOrientations |= UIInterfaceOrientationMaskLandscapeRight;
            }
        }
        if (gSupportedInterfaceOrientations == 0)
        {
            gSupportedInterfaceOrientations = UIInterfaceOrientationMaskPortrait;
        }
    }
    return gSupportedInterfaceOrientations;
}
#endif

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    if (FskCocoaBitmapUseGL())
    {
        NSArray *orientations = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"UISupportedInterfaceOrientations"];
        NSString *toOrientation;

        if ([orientations count] == 0)
        {
            return (interfaceOrientation == UIInterfaceOrientationPortrait);
        }

        switch (interfaceOrientation)
        {
            case UIInterfaceOrientationPortrait:
            default:
                toOrientation = @"UIInterfaceOrientationPortrait";
                break;
            case UIInterfaceOrientationPortraitUpsideDown:
                toOrientation = @"UIInterfaceOrientationPortraitUpsideDown";
                break;
            case UIInterfaceOrientationLandscapeLeft:
                toOrientation = @"UIInterfaceOrientationLandscapeLeft";
                break;
            case UIInterfaceOrientationLandscapeRight:
                toOrientation = @"UIInterfaceOrientationLandscapeRight";
                break;
        }

        for (NSString *orientation in orientations)
        {
            if ([orientation isEqualToString:toOrientation])
            {
                return YES;
            }
        }
        return NO;
    }

    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration
{
    if (self.notifyViewOrientation)
    {
        [[NSNotificationCenter defaultCenter] postNotificationName:FskCocoaApplicationViewOrientationDidChangeNotification object:nil userInfo:nil];
    }
}

- (void) remoteControlReceivedWithEvent: (UIEvent *) receivedEvent {
    if (receivedEvent.type == UIEventTypeRemoteControl) {
		UInt32 functionKey = 0;

        switch (receivedEvent.subtype) {
            case UIEventSubtypeRemoteControlPlay:
				functionKey = kFskEventFunctionKeyPlay;    // @@ as PlayPause
                //NSLog(@"play");
                break;
            case UIEventSubtypeRemoteControlPause:
				functionKey = kFskEventFunctionKeyPause;    // @@ as PlayPause
                //NSLog(@"pause");
                break;
            case UIEventSubtypeRemoteControlTogglePlayPause:
				functionKey = kFskEventFunctionKeyTogglePlayPause;
                //NSLog(@"toggle play pause");
                break;
            case UIEventSubtypeRemoteControlPreviousTrack:
				functionKey = kFskEventFunctionKeyPreviousTrack;
                //NSLog(@"previous track");
                break;
            case UIEventSubtypeRemoteControlNextTrack:
				functionKey = kFskEventFunctionKeyNextTrack;
                //NSLog(@"next track");
                break;
            case UIEventSubtypeRemoteControlStop:
				functionKey = kFskEventFunctionKeyStop;
                //NSLog(@"stop");
                break;
            case UIEventSubtypeRemoteControlBeginSeekingBackward:
				functionKey = kFskEventFunctionKeyBeginSeekingBackward;
                //NSLog(@"begin seeking backward");
                break;
            case UIEventSubtypeRemoteControlEndSeekingBackward:
				functionKey = kFskEventFunctionKeyEndSeekingBackward;
                //NSLog(@"end seeking backward");
                break;
            case UIEventSubtypeRemoteControlBeginSeekingForward:
				functionKey = kFskEventFunctionKeyBeginSeekingForward;
                //NSLog(@"begin seeking forward");
                break;
            case UIEventSubtypeRemoteControlEndSeekingForward:
				functionKey = kFskEventFunctionKeyEndSeekingForward;
                //NSLog(@"end seeking forward");
                break;
            default:
                break;
        }
		if (functionKey) {
			[self throwKeyEvent:0 keyDown:YES functionKey:functionKey];
			[self throwKeyEvent:0 keyDown:NO functionKey:functionKey];
		}
    }
}

- (void)fskInitialize
{
    //NSLog(@"fskInitialize started");

	(void)FskMainInitialize(kFskMainNetwork | kFskMainServer, 0, NULL);
	(void)mainExtensionInitialize();

	[self setDisplayLinkActive:YES];

#if SUPPORT_REMOTE_NOTIFICATION
	/* Remote Notification */
	char *env = FskEnvironmentGet("remoteNotification");
	if (env != NULL)
	{
		SInt32 envTypes = FskStrToNum(env);
		UIApplication *app = [UIApplication sharedApplication];

#if defined (__IPHONE_8_0) && (__IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_8_0)
		if ([app respondsToSelector:@selector(registerForRemoteNotifications)])
		{
			UIUserNotificationType types = UIUserNotificationTypeNone;

			if (envTypes & 1)
				types |= UIUserNotificationTypeBadge;
			if (envTypes & 2)
				types |= UIUserNotificationTypeSound;
			if (envTypes & 4)
				types |= UIUserNotificationTypeAlert;

			if (types != UIUserNotificationTypeNone)
			{
				/* handle remote notification if recieved on launch */
				[[FskCocoaApplication sharedApplication] handleRemoteNotification];

				/* register remote notification */
				UIUserNotificationSettings *settings;

				[app registerForRemoteNotifications];

				settings = [UIUserNotificationSettings settingsForTypes:types categories:nil];
				[app registerUserNotificationSettings:settings];
			}
		}
		else
#endif
		{
			UIRemoteNotificationType types = UIRemoteNotificationTypeNone;

			if (envTypes & 1)
				types |= UIRemoteNotificationTypeBadge;
			if (envTypes & 2)
				types |= UIRemoteNotificationTypeSound;
			if (envTypes & 4)
				types |= UIRemoteNotificationTypeAlert;

			if (types != UIRemoteNotificationTypeNone)
			{
				/* handle remote notification if recieved on launch */
				[[FskCocoaApplication sharedApplication] handleRemoteNotification];

				/* register remote notification */
				[app registerForRemoteNotificationTypes:types];
			}
		}
	}
#endif	/* SUPPORT_REMOTE_NOTIFICATION */

#if SUPPORT_EXTERNAL_SCREEN
	env = FskEnvironmentGet("externalScreen");
	if ((env != NULL) && FskStrToNum(env) > 0)
	{
		// check whether the external screen is already connected.
		[self findExternalScreen];

		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(screenDidConnect:)
													 name:UIScreenDidConnectNotification
												   object:nil];

		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(screenDidDisconnect:)
													 name:UIScreenDidDisconnectNotification
												   object:nil];

		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(screenModeDidChange:)
													 name:UIScreenModeDidChangeNotification
												   object:nil];
	}
#endif	/* SUPPORT_EXTERNAL_SCREEN */

    //NSLog(@"fskInitialize finished");
}

- (void)fskTerminate
{
	[self setDisplayLinkActive:NO];
	mainExtensionTerminate();
	(void)FskMainTerminate();
}

- (BOOL)canBecomeFirstResponder
{
	return YES;
}

- (void)throwKeyEvent:(char)keyCode keyDown:(BOOL)down functionKey:(UInt32)functionKey
{
	FskEvent fskEvent;

	if (FskEventNew(&fskEvent, down ? kFskEventKeyDown : kFskEventKeyUp, NULL, 0) == kFskErrNone) {
		char chars[2];
		chars[0] = keyCode;
		chars[1] = 0;
		FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 2, chars);
		if (functionKey)
			FskEventParameterAdd(fskEvent, kFskEventParameterFunctionKey, sizeof(functionKey), &functionKey);
		FskWindowEventQueue(self.fskWindow, fskEvent);
	}
}

- (void)motionBegan:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
#ifdef SHAKE_BACK_TM
	[self throwKeyEvent:(char)8 keyDown:YES functionKey:0];
	[self throwKeyEvent:(char)8 keyDown:NO functionKey:0];
#endif
}

- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
	// [self throwKeyEvent:(char)8 keyDown:NO functionKey:0];	// motionBegan and motionEnded are exclusive??
}

- (void)setDisplayLinkActive:(BOOL)flag
{
	if (flag) {
		if (displayLink == nil) {
			displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(doFrame:)];
			[displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
		}
	}
	else {
		if (displayLink != nil) {
			[displayLink removeFromRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
			displayLink = nil;
		}
	}
}

- (void)pauseDisplayLink:(BOOL)flag
{
	if (displayLink != nil)
		displayLink.paused = flag;
}

- (BOOL) isDisplayLinkRunning
{
	return displayLink != nil && !displayLink.isPaused;
}

- (BOOL) isDisplayLinkActive
{
	return displayLink != nil;
}

- (void)doFrame:(id)data
{
	FskWindow fskWindow = self.fskWindow;
	if (fskWindow == NULL)
		return;

	CFTimeInterval frameTime = displayLink.timestamp + displayLink.duration;
	FskTimeRecord time;
	time.seconds = floor(frameTime);
	time.useconds = (SInt32)((frameTime - time.seconds) * 1000000);
	FskWindowUpdate(fskWindow, &time);
}

#if SUPPORT_EXTERNAL_SCREEN
#if TEST_EXTERNAL_SCREEN
- (void)setBitmap:(FskBitmap)bitmap
{
	if (_bitmap != NULL)
	{
		FskBitmapDispose(_bitmap);
	}
	_bitmap = bitmap;
	if (bitmap != NULL)
	{
		FskBitmapUse(bitmap);
		FskPortInvalidateRectangle(self.fskWindow->port, NULL);
	}
}
#endif	/* TEST_EXTERNAL_SCREEN */

- (void)screenDidAdd:(UIScreen *)extScreen
{
	FskCocoaApplication *app = [FskCocoaApplication sharedApplication];

	[app addExternalScreen:extScreen];
}

- (void)screenDidConnect:(NSNotification *)notif
{
	[self screenDidAdd:(UIScreen *)notif.object];
}

- (void)screenDidDisconnect:(NSNotification *)notif
{
	UIScreen *extScreen = notif.object;
	FskCocoaApplication *app = [FskCocoaApplication sharedApplication];

	[app removeExternalScreen:extScreen];
}

- (void)screenModeDidChange:(NSNotification *)notif
{
	UIScreen *extScreen = notif.object;
	FskCocoaApplication *app = [FskCocoaApplication sharedApplication];

	[app updateExternalScreen:extScreen];
}

- (void)findExternalScreen
{
	NSArray *screens = [UIScreen screens];

	for (NSUInteger i = 1; i < screens.count; i++)
	{
		UIScreen *extScreen = [screens objectAtIndex:i];
		[self screenDidAdd:extScreen];
	}
}
#endif	/* SUPPORT_EXTERNAL_SCREEN */

@end
