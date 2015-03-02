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
#define __FSKWINDOW_PRIV__

#import "FskCocoaViewControllerPhone.h"

#import "FskCocoaApplicationPhone.h"
#import "FskCocoaSupportPhone.h"
#import "FskCocoaViewPhone.h"
#import "FskWindow.h"
#import "FskMain.h"

extern void mainExtensionInitialize(void);
extern void mainExtensionTerminate(void);

@interface FskCocoaViewController () {
	CADisplayLink *displayLink;
}

- (void)fskInitialize;
- (void)fskTerminate;

@end

@implementation FskCocoaViewController

+ (FskCocoaViewController *)sharedViewController
{
	return (FskCocoaViewController *)[FskCocoaApplication sharedApplication].window.rootViewController;
}

- (void)loadView
{
    self.view = [[[UIView alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.notifyViewOrientation = NO;
#if (__IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_7_0)
    self.wantsFullScreenLayout = YES;
#endif
}

- (void)dealloc
{
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
    [self performSelector:@selector(fskInitialize) withObject:nil afterDelay:0.0];
}

- (void)viewDidUnload
{
    [super viewDidUnload];

    if (self.view)
    {
        FskWindow	fskWindow;
        FskEvent 	fskEvent;
        UInt32		commandID = 0;

        fskWindow = [(FskCocoaView *)[[self.view subviews] lastObject] fskWindow];

		// send Fsk quit event
		if (kFskErrNone == FskEventNew(&fskEvent, kFskEventMenuCommand, NULL, kFskEventModifierNotSet))
		{
			FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(commandID), &commandID);
			FskWindowEventQueue(fskWindow, fskEvent);
		}
    }
    [self fskTerminate];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
	[[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
    [self becomeFirstResponder];
}

- (void)viewWillDisappear:(BOOL)animated {
    [[UIApplication sharedApplication] endReceivingRemoteControlEvents];
    [self resignFirstResponder];
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

@synthesize notifyViewOrientation = notifyViewOrientation_;

- (void)resizeIfNeeded
{
    CGRect bounds;
    FskWindow fskWindow;
    UInt32 width, height, vWidth, vHeight;
    CGFloat scale;

    bounds = self.view.bounds;
    scale = FskCocoaDeviceScreenScaleFactor();
    vWidth = (UInt32)(bounds.size.width * scale);
    vHeight = (UInt32)(bounds.size.height * scale);
    fskWindow = [(FskCocoaView *)[[self.view subviews] lastObject] fskWindow];
    if (fskWindow == NULL)
    {
        [self performSelector:@selector(resizeIfNeeded) withObject:nil afterDelay:0.2];
        return;
    }
    FskCocoaWindowGetSize(fskWindow, &width, &height);
    if ((width != vWidth) || (height != vHeight))
    {
        if (FskCocoaBitmapUseGL())
        {
            SInt32 rotation = FskCocoaApplicationGetRotation();
            if ((rotation == 90) || (rotation == 270))
            {
                UInt32 tmp = vWidth;
                vWidth = vHeight;
                vHeight = tmp;
            }
        }
        FskCocoaWindowSetSize(fskWindow, vWidth, vHeight);
        FskWindowCocoaSizeChanged(fskWindow);
    }
}

- (void)fskInitialize
{
    //NSLog(@"fskInitialize started");

	(void)FskMainInitialize(kFskMainNetwork | kFskMainServer, 0, NULL);
	(void)mainExtensionInitialize();

	[self setDisplayLinkActive:YES];

    //[self performSelector:@selector(resizeIfNeeded) withObject:nil afterDelay:0.0];
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
	FskWindow fskWindow = [(FskCocoaView *)[[self.view subviews] lastObject] fskWindow];
	FskEvent fskEvent;

	if (FskEventNew(&fskEvent, down ? kFskEventKeyDown : kFskEventKeyUp, NULL, 0) == kFskErrNone) {
		char chars[2];
		chars[0] = keyCode;
		chars[1] = 0;
		FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 2, chars);
		if (functionKey)
			FskEventParameterAdd(fskEvent, kFskEventParameterFunctionKey, sizeof(functionKey), &functionKey);
		FskWindowEventQueue(fskWindow, fskEvent);
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
	if (self.view == nil)
		return;
	FskCocoaView *view = (FskCocoaView *)[[self.view subviews] lastObject];
	if (view == nil)
		return;
	FskWindow fskWindow = [view fskWindow];
	if (fskWindow == NULL)
		return;

	CFTimeInterval frameTime = displayLink.timestamp + displayLink.duration;
	FskTimeRecord time;
	time.seconds = floor(frameTime);
	time.useconds = (SInt32)((frameTime - time.seconds) * 1000000);
	FskWindowUpdate(fskWindow, &time);
}
@end
