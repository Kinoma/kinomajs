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
#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AssetsLibrary/AssetsLibrary.h>

#define __FSKCOCOASUPPORT_PRIV__
#define __FSKWINDOW_PRIV__
#define __FSKMENU_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKIMAGE_PRIV__
#define __FSKAUDIO_PRIV__
#define __FSKTHREAD_PRIV__
#define __FSKTIME_PRIV__

#import "FskCocoaSupportPhone.h"
#import "FskCocoaApplicationPhone.h"
#import "FskCocoaViewPhone.h"
#import "FskCocoaViewControllerPhone.h"
#import "FskTime.h"

#define USE_AUDIO_QUEUE 1

#pragma mark --- structures ---
#if !USE_CORE_TEXT
struct FskTextFormatCacheRecord
{
	UInt16 textSize;
	char *fontName;
};
#endif  /* !USE_CORE_TEXT */

#pragma mark --- externs ---
extern void mainExtensionInitialize();
extern void mainExtensionTerminate();

#pragma mark --- private ---
void FskCocoaTimerCallback(CFRunLoopTimerRef timer, void *info);
void FskCocoaThreadRunLoopSourceCallback(void *info);
void FskCocoaAttributedStringGetAttribute(NSDictionary *attributes, NSString *attributeKey, FskMediaPropertyValue propertyValue);

#pragma mark --- callbacks ---
#if !USE_AUDIO_QUEUE
/*
...
OSStatus ACInputData(AudioConverterRef inAudioConverter, UInt32 *ioDataSize, void **outData, void *inUserData)
{
	OSStatus			err = noErr;
	FskAudioOut			fskAudioOut;
	FskAudioOutBlock	fskAudioOutBlock;

	fskAudioOut = (FskAudioOut)inUserData;
	*ioDataSize = 0;
	*outData = NULL;

	// remove unused from queue
	FskAudioOutRemoveUnusedFromQueue(fskAudioOut);

	// get the data
	if (fskAudioOut->playing)
	{
		// get the next block
		for (fskAudioOutBlock = fskAudioOut->blocks; fskAudioOutBlock && fskAudioOutBlock->done; fskAudioOutBlock = fskAudioOutBlock->next)
			{}

		if (fskAudioOutBlock)
		{
			fskAudioOutBlock->done = true;
			*ioDataSize = fskAudioOutBlock->dataSize;
			*outData = fskAudioOutBlock->data;
		}
	}

	// refill queue
	FskAudioOutRefillQueue(fskAudioOut);

bail:
	return err;
}
*/

OSStatus AURender(void *refCon, AudioUnitRenderActionFlags *inActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumFrames, AudioBufferList *ioData)
{
	OSStatus			err = noErr;
	FskAudioOut			fskAudioOut;
//...    AudioConverterRef	audioConverter;

	fskAudioOut = (FskAudioOut)refCon;
	if (fskAudioOut == NULL) goto bail;
//	audioConverter = fskAudioOut->audioConverter;
//	if (audioConverter == NULL) goto bail;

	// fill the audio converter buffer
//...	err = AudioConverterFillBuffer(audioConverter, ACInputData, fskAudioOut, &(ioData->mBuffers[0].mDataByteSize), ioData->mBuffers[0].mData);
	BAIL_IF_ERR(err);
	fskAudioOut->currentHostTime = inTimeStamp->mHostTime;

bail:
	return err;
}
#endif

#pragma mark --- system ---
void FskCocoaSystemGetVersion(char *version)
{
	FskStrCopy(version, [[UIDevice currentDevice].systemVersion UTF8String]);
}

#pragma mark --- application ---
void FskCocoaApplicationRun()
{
#if 0
	CFRunLoopRun();
#endif
}

void FskCocoaApplicationStop()
{
#if 0
	CFRunLoopStop(CFRunLoopGetCurrent());
#endif
}

void FskCocoaApplicationRunLoop(void)
{
	[[FskCocoaApplication sharedApplication] runFskRunLoop];
}

SInt32 FskCocoaApplicationGetRotation(void)
{
    return [[FskCocoaApplication sharedApplication] viewRotation];
}

void WaitCallBack(CFRunLoopObserverRef observer,
				  CFRunLoopActivity activity,
				  void *info)
{
    FskCocoaApplicationRunLoop();
}

void FskCocoaApplicationRunLoopAddCallback(void)
{
	FskThread thread = FskThreadGetCurrent();
	CFRunLoopObserverRef observer;

	observer = CFRunLoopObserverCreate(kCFAllocatorDefault, kCFRunLoopBeforeWaiting, true, 0, WaitCallBack, NULL);
	CFRunLoopAddObserver(thread->cfRunloop, observer, kCFRunLoopCommonModes);
}

void FskCocoaApplicationSetIdleTimerDisabled(Boolean disabled)
{
    [[UIApplication sharedApplication] setIdleTimerDisabled:disabled ? YES : NO];
}

UInt32 FskCocoaApplicationGetStatusBarHeight()
{
    CGSize statusBarSize = [UIApplication sharedApplication].statusBarFrame.size;
    // smaller size must be the actual height
    return MIN(statusBarSize.height, statusBarSize.width);
}

void FskCocoaApplicationSetStatusBarHidden(Boolean hidden)
{
    UIApplication *app = [UIApplication sharedApplication];

    if (hidden != app.statusBarHidden)
    {
        FskWindow win = [FskCocoaApplication sharedApplication].mainViewController.fskWindow;

        [app setStatusBarHidden:hidden withAnimation:UIStatusBarAnimationFade];
        FskWindowCocoaSizeChanged(win);
    }
}

static FskCocoaApplicationInterruptionCB gInterruptionCB = NULL;
static NSMutableDictionary *gInterruptionCallbacks = nil;
static Boolean gInterrupted = false;

void FskCocoaApplicationSetInterruptionCB(FskCocoaApplicationInterruptionCB cb)
{
	gInterruptionCB = cb;
}

UInt32 FskCocoaApplicationInstallInterruptionCB(void (^callback)(Boolean interrupted, Boolean resume))
{
	static UInt32 callbackId = 0;

	if (gInterruptionCallbacks == nil) {
		gInterruptionCallbacks = [[NSMutableDictionary alloc] initWithCapacity:10];
	}

	id obj = [callback copy];
	[gInterruptionCallbacks setObject:obj forKey:@(++callbackId)];
	[obj release];

	return callbackId;
}

void FskCocoaApplicationRemoveInterruptionCB(UInt32 callbackId)
{
	NSNumber *key = @(callbackId);
	id obj = [gInterruptionCallbacks objectForKey:key];
	if (obj) {
		[gInterruptionCallbacks removeObjectForKey:key];
	}
}

void FskCocoaApplicationSetInterruption(Boolean interrupted, Boolean resume)
{
    if (interrupted == gInterrupted)
        return;

    gInterrupted = interrupted;

	if (gInterruptionCB)
	{
		(*gInterruptionCB)(interrupted, resume);
	}

	if (gInterruptionCallbacks) {
		for (NSNumber *key in gInterruptionCallbacks) {
			void (^callback)(Boolean interrupted, Boolean resume) = gInterruptionCallbacks[key];
			callback(interrupted, resume);
		}
	}
}

#pragma mark --- thread ---
void FskCocoaThreadInitialize(FskThread fskThread)
{
	CFRunLoopSourceContext sourceContext = {0};

	if (fskThread == NULL) return;

	sourceContext.perform = &FskCocoaThreadRunLoopSourceCallback;

	// initialize
	fskThread->cfRunloop = [[NSRunLoop currentRunLoop] getCFRunLoop];
	fskThread->cfRunLoopSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &sourceContext);
	CFRunLoopAddSource(fskThread->cfRunloop, fskThread->cfRunLoopSource, kCFRunLoopDefaultMode);
	if (!(fskThread->flags & kFskThreadFlagsIsMain))
		fskThread->nsAutoreleasePool = [[NSAutoreleasePool alloc] init];
}

void FskCocoaThreadTerminate(FskThread fskThread)
{
	NSAutoreleasePool *autoreleasePool;

	if (fskThread == NULL) return;
	autoreleasePool = fskThread->nsAutoreleasePool;

	// terminate
	CFRunLoopSourceInvalidate(fskThread->cfRunLoopSource);
	CFRelease(fskThread->cfRunLoopSource);

	if (autoreleasePool)
		[autoreleasePool release];
}

void FskCocoaThreadRun(FskThread fskThread, SInt32 milliseconds)
{
	Boolean returnAfterSourceHandled = false;

	if (fskThread == NULL) return;

	if (fskThread->wake)
	{
		milliseconds = 1;
		returnAfterSourceHandled = true;
		fskThread->wake = false;
	}

	// run
	if (milliseconds >= 0)
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, ((CFTimeInterval)milliseconds / 1000), returnAfterSourceHandled);
	else
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, (CFTimeInterval)0x7fffffff, returnAfterSourceHandled);
}

void FskCocoaThreadWake(FskThread fskThread)
{
	if (fskThread == NULL) return;

	if (fskThread->flags & kFskThreadFlagsIsMain) {
        if (CFRunLoopIsWaiting(fskThread->cfRunloop))
            CFRunLoopWakeUp(fskThread->cfRunloop);
//		FskCocoaApplicationRunLoop();
	} else {
		fskThread->wake = true;
		CFRunLoopStop(fskThread->cfRunloop);
		CFRunLoopWakeUp(fskThread->cfRunloop);
	}
}

#pragma mark --- window ---
Boolean FskCocoaWindowCreate(FskWindow fskWindow, Boolean isCustomWindow, Boolean isFullScreenWindow, SInt32 width, SInt32 height)
{
	Boolean success = false;
	FskCocoaApplication *app = [FskCocoaApplication sharedApplication];
	FskCocoaViewController *controller = [app getEmptyController];
    FskCocoaView *view = nil;
	CGRect bounds;
	CGFloat screenScale;
	BOOL isMainView = (controller == app.mainViewController);

	if ((fskWindow == NULL) || (controller == nil)) goto bail;

	screenScale = controller.screenScale;
	bounds = controller.view.bounds;

	// create the window
	view = [[[FskCocoaView alloc] initWithFrame:bounds screenScale:screenScale isMainView:isMainView] autorelease];
	if (view == nil) goto bail;

	view.fskWindow = fskWindow;
	fskWindow->uiWindow = view;
	fskWindow->uiWindowController = controller;

	[controller.view addSubview:view];

	success = true;

bail:
	return success;
}

void FskCocoaWindowDispose(FskWindow fskWindow)
{
	FskCocoaView *view;

	if (fskWindow == NULL) return;
	view = (FskCocoaView *)(fskWindow->uiWindow);

	// dispose the window
	fskWindow->uiWindow = nil;
	[view removeFromSuperview];
}

void FskCocoaWindowCopyBits(FskWindow fskWindow, FskBitmap fskBitmap, const FskRectangle sourceFskRect, const FskRectangle destFskRect)
{
	FskCocoaView *view;

	if (fskWindow == NULL) return;
	view = (FskCocoaView *)(fskWindow->uiWindow);

	// copy bits
	[view copyBits:fskBitmap sourceRect:sourceFskRect destRect:destFskRect];
}

void FskCocoaWindowGetVisible(FskWindow fskWindow, Boolean *visible)
{
	FskCocoaView *view;

	if (fskWindow == NULL) return;
	view = (FskCocoaView *)(fskWindow->uiWindow);

	// get visibility
	if (visible)
		*visible = !view.hidden;
}

void FskCocoaWindowSetVisible(FskWindow fskWindow, Boolean visible)
{
	FskCocoaView *view;

	if (fskWindow == NULL) return;
	view = (FskCocoaView *)(fskWindow->uiWindow);

	// set visibility
	view.hidden = !visible;
}

void FskCocoaWindowGetSize(FskWindow fskWindow, UInt32 *width, UInt32 *height)
{
	CGRect		windowFrame;
    FskCocoaView *window;
    UInt32 deviceWidth, deviceHeight, statusBarHeight;

	if (fskWindow == NULL) return;
    window = (FskCocoaView *)(fskWindow->uiWindow);

	// get the window size
	windowFrame = [window bounds];

	CGFloat scale = ((FskCocoaView *)fskWindow->uiWindow).contentScaleFactor;
    deviceWidth = windowFrame.size.width * scale;
    deviceHeight = windowFrame.size.height * scale;
    
	if (fskWindow->uiWindowController == [FskCocoaApplication sharedApplication].mainViewController)
	{
        BOOL fullScreen = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"FSKFullScreen"] boolValue];
    
        if( ! fullScreen )
        {
            statusBarHeight = FskCocoaApplicationGetStatusBarHeight();
            
            if (statusBarHeight > 0) {
                deviceHeight -= statusBarHeight * scale;
            }
        }
	}

	if (width)
		*width = deviceWidth;

	if (height)
		*height = deviceHeight;
}

void FskCocoaWindowSetSize(FskWindow fskWindow, UInt32 width, UInt32 height)
{
    if (FskCocoaBitmapUseGL()) {
        SInt32 rotate = FskCocoaApplicationGetRotation();

        if (fskWindow->isFullscreenWindow)
        {
            return;
        }
        if ((rotate == 90) || (rotate == 270))
        {
            UInt32 tmp = height;
            height = width;
            width = tmp;
        }
    }

    FskCocoaView *window;
	CGRect newFrame;

	if (fskWindow == NULL) return;

	CGFloat scale = ((FskCocoaView *)fskWindow->uiWindow).contentScaleFactor;
    if (1.0 < scale) {
        width /= scale;
        height /= scale;
    }
    window = (FskCocoaView *)(fskWindow->uiWindow);
	newFrame = CGRectMake(0.0, 0.0, (float)width, (float)height);

	// set the window size
	window.frame = newFrame;
}

void FskCocoaWindowGetOrigin(FskWindow fskWindow, SInt32 *x, SInt32 *y)
{
	CGRect		frame;
    FskCocoaView *window;

	if (fskWindow == NULL) return;
    window = (FskCocoaView *)(fskWindow->uiWindow);

	// get the window origin
	frame = window.frame;

	CGFloat scale = ((FskCocoaView *)fskWindow->uiWindow).contentScaleFactor;
	if (x)
		*x = frame.origin.x * scale;

	if (y)
		*y = ([[UIScreen mainScreen] bounds].size.height - frame.origin.y - frame.size.height) * scale;
}

void FskCocoaWindowSetOrigin(FskWindow fskWindow, SInt32 x, SInt32 y)
{
    FskCocoaView *window;
	CGRect		currentFrame;

	if (fskWindow == NULL) return;
    if (FskCocoaBitmapUseGL() && (fskWindow->isFullscreenWindow)) return;
	CGFloat scale = ((FskCocoaView *)fskWindow->uiWindow).contentScaleFactor;
    if (1.0 < scale) {
        x /= scale;
        y /= scale;
    }
    window = (FskCocoaView *)(fskWindow->uiWindow);
	currentFrame = window.frame;
	currentFrame.origin = CGPointMake(x, [[UIScreen mainScreen] bounds].size.height - y - [window frame].size.height);

	// set the window origin
	window.frame = currentFrame;
}

void FskCocoaWindowGetMouseLocation(FskWindow fskWindow, FskPoint fskPoint)
{
    FskCocoaView *view;

	if (fskWindow == NULL) return;
    view = (FskCocoaView *)(fskWindow->uiWindow);

	// get the mouse location
	if (fskPoint)
	{
		FskPointRecord lastTouchedPoint;

		lastTouchedPoint = view.lastTouchedPoint;
		fskPoint->x = lastTouchedPoint.x;
		fskPoint->y = lastTouchedPoint.y;
	}
}

void FskCocoaWindowSetNeedsDisplay(FskWindow fskWindow, Boolean needsDisplay)
{
    FskCocoaView *view;

	if (fskWindow == NULL) return;
    view = (FskCocoaView *)(fskWindow->uiWindow);
	[view setNeedsDisplay];
}

void FskCocoaWindowSetNeedsDisplayInRect(FskWindow fskWindow, FskRectangle displayFskRect)
{
	FskCocoaView *view;
	CGRect		invalidRect;

	if (fskWindow == NULL) return;
    view = (FskCocoaView *)(fskWindow->uiWindow);

	// set needs display in rect
	if (displayFskRect)
	{
		invalidRect = CGRectMake(displayFskRect->x, [view frame].size.height - displayFskRect->y - displayFskRect->height, displayFskRect->width, displayFskRect->height);
		[view setNeedsDisplayInRect:invalidRect];
	}
}

void FskCocoaWindowBeginDraw(FskWindow fskWindow)
{
	FskCocoaView *view = (FskCocoaView *)(fskWindow->uiWindow);

	if (view)
		[view beginDraw];
}

void FskCocoaWindowEndDraw(FskWindow fskWindow)
{
	FskCocoaView *view = (FskCocoaView *)(fskWindow->uiWindow);

	if (view)
		[view endDraw];
}

Boolean FskCocoaWindowGetSIPEnabled(FskWindow fskWindow)
{
    FskCocoaView *win = (FskCocoaView *)fskWindow->uiWindow;

    return win.keyboardActive;
}

void FskCocoaWindowSetSIPEnabled(FskWindow fskWindow, Boolean enable)
{
    FskCocoaView *win = (FskCocoaView *)fskWindow->uiWindow;

    win.keyboardActive = enable;
}

void FskCocoaWindowInputTextActivate(FskWindow fskWindow, xsMachine *the, xsSlot obj, Boolean active, int mode)
{
#if TEXT_INPUT_SYSTEM
    FskCocoaView *win = (FskCocoaView *)fskWindow->uiWindow;

    win.machine = the;
    win.obj = obj;
	[win resetStorage];
    win.autocapitalizationType = UITextAutocapitalizationTypeNone;
//    win.autocorrectionType = UITextAutocorrectionTypeNo;
	win.returnKeyType = UIReturnKeyNext;
    switch (mode)
    {
        case cocoaTextInputTypeNumeric:
            //win.keyboardType = UIKeyboardTypeNumbersAndPunctuation;
            win.keyboardType = UIKeyboardTypeNumberPad;
            //win.keyboardType = UIKeyboardTypeDecimalPad;
            break;
        case cocoaTextInputTypeEmail:
            win.keyboardType = UIKeyboardTypeEmailAddress;
            break;
        case cocoaTextInputTypeURI:
            win.keyboardType = UIKeyboardTypeURL;
            break;
        case cocoaTextInputTypePhone:
            win.keyboardType = UIKeyboardTypePhonePad;
            break;
		case cocoaTextInputTypeMultiLine:
			win.returnKeyType = UIReturnKeyDefault;
			win.keyboardType = UIKeyboardTypeDefault;
			break;
		case cocoaTextInputTypeDefault:
		case cocoaTextInputTypePassword:
		default:
			win.keyboardType = UIKeyboardTypeDefault;
			break;
    }
#endif
}

void FskCocoaWindowInputTextSetSelection(FskWindow fskWindow, const char *text, UInt32 textLength, SInt32 selectionStart, SInt32 selectionEnd)
{
#if TEXT_INPUT_SYSTEM
    FskCocoaView *win = (FskCocoaView *)fskWindow->uiWindow;

    if (win.keyboardUpdating)
    {
        return;
    }

	[win setStorageText:[NSString stringWithUTF8String:text] selection:NSMakeRange((NSUInteger)selectionStart, (NSUInteger)(selectionEnd - selectionStart))];
 #endif
}

void FskCocoaWindowSetUpdates(FskWindow window)
{
	FskCocoaViewController *controller = window->uiWindowController;
	[controller pauseDisplayLink:!window->doBeforeUpdate];
}

Boolean FskCocoaWindowDisplayPaused(FskWindow window)
{
	FskCocoaViewController *controller = window->uiWindowController;
	return [controller isDisplayLinkActive] && ![controller isDisplayLinkRunning];
}

void FskCocoaWindowGetScreenScale(FskWindow window, float *scale)
{
	FskCocoaViewController *controller = window->uiWindowController;
	*scale = (controller != nil) ? controller.screenScale : 1.0;
}

#if SUPPORT_EXTERNAL_SCREEN && TEST_EXTERNAL_SCREEN
FskWindow FskCocoaWindowGetExternalWindow(FskWindow window)
{
	FskCocoaApplication *app = [FskCocoaApplication sharedApplication];
	FskCocoaViewController *controller = [app getExternalViewControllerAtIndex:0];

	return (controller != nil) ? controller.fskWindow : NULL;
}

void FskCocoaWindowSetBitmapForExternalWindow(FskWindow window, FskBitmap bitmap)
{
	FskCocoaApplication *app = [FskCocoaApplication sharedApplication];
	FskCocoaViewController *controller = [app getExternalViewControllerAtIndex:0];

	if (controller != nil)
	{
		controller.bitmap = bitmap;
	}
}
#endif	/* TEST_EXTERNAL_SCREEN && SUPPORT_EXTERNAL_SCREEN */

#pragma mark --- event ---
void FskCocoaEventSend(FskWindow fskWindow, UInt32 eventClass, UInt32 eventType)
{
	if ((eventClass == kEventClassFsk) && (eventType == kEventFskProcessEvent))
	{
		// check events
		FskWindowCheckEvents();
	}
	else
	{
/*
// ...
		NSEvent 	*event;
		UIWindow	*window;

		if (fskWindow == NULL) return;
		window = (UIWindow *)(fskWindow->uiWindow);

		// create the event
		event = [NSEvent otherEventWithType:NSApplicationDefined location:NSZeroPoint modifierFlags:0 timestamp:0
			windowNumber:[window windowNumber] context:nil subtype:0 data1:eventClass data2:eventType];

		// queue the event
		[NSApp postEvent:event atStart:NO];
*/
	}
}

#pragma mark --- bitmap ---
Boolean FskCocoaBitmapGetColorInfo(UInt32 pixelFormat, SInt32 width, CGImageAlphaInfo *alphaInfoOut, CGColorSpaceRef *colorSpaceOut, UInt32 *rowBytesOut)
{
	UInt32 				rowBytes = 0;
	CGColorSpaceRef		colorSpace = NULL;
	CGImageAlphaInfo	alphaInfo;

    switch (pixelFormat)
    {
        case kFskBitmapFormat32ARGB:
            colorSpace = CGColorSpaceCreateDeviceRGB();
            alphaInfo = kCGImageAlphaNoneSkipFirst;
            rowBytes = width * 4;
            break;
        case kFskBitmapFormat32RGBA:
            colorSpace = CGColorSpaceCreateDeviceRGB();
            alphaInfo = kCGImageAlphaNoneSkipLast;
            rowBytes = width * 4;
            break;
        case kFskBitmapFormat16RGB565LE:
            colorSpace = CGColorSpaceCreateDeviceRGB();
            alphaInfo = kCGImageAlphaNone;
            rowBytes = width * 2;
            break;
        case kFskBitmapFormat8G:
            colorSpace = CGColorSpaceCreateDeviceGray();
            alphaInfo = kCGImageAlphaNone;
            rowBytes = width;
            break;
        default:
            return false;
    }

    if (alphaInfoOut)
        *alphaInfoOut = alphaInfo;
    if (colorSpaceOut)
        *colorSpaceOut = colorSpace;
    else
        CGColorSpaceRelease(colorSpace);
    if (rowBytesOut)
        *rowBytesOut = rowBytes;

    return true;
}

Boolean FskCocoaBitmapCreate(FskBitmap fskBitmap, UInt32 pixelFormat, SInt32 width, SInt32 height)
{
	Boolean				success = false;
	UInt32 				rowBytes = 0;
	CGColorSpaceRef		colorSpace = NULL;
	CGContextRef		cgBitmapContext = NULL;
	void				*bits = NULL;
	CGImageAlphaInfo	alphaInfo;

	if (fskBitmap == NULL) goto bail;

	if ((width > 0) && (height > 0))
	{
		if (!FskCocoaBitmapGetColorInfo(pixelFormat, width, &alphaInfo, &colorSpace, &rowBytes))
			goto bail;

		if ((bits = FskMemPtrAlloc(rowBytes * height)) == NULL)
			goto bail;

		// create the bitmap
		cgBitmapContext = CGBitmapContextCreate(bits, width, height, 8, rowBytes, colorSpace, (CGBitmapInfo)alphaInfo);
	}

	fskBitmap->cgBitmapContext = cgBitmapContext;
	fskBitmap->bits = bits;
	fskBitmap->rowBytes = rowBytes;

	success = true;

bail:
	if (colorSpace)
		CGColorSpaceRelease(colorSpace);

	return success;
}

void FskCocoaBitmapDispose(FskBitmap fskBitmap)
{
	CGContextRef	cgBitmapContext;
	void			*bits;

	if (fskBitmap == NULL) return;
	cgBitmapContext = (CGContextRef)(fskBitmap->cgBitmapContext);

	// dispose the bitmap and bits
	if (cgBitmapContext)
	{
		bits = fskBitmap->bits;

		CGContextRelease(cgBitmapContext);
		fskBitmap->cgBitmapContext = NULL;

		if (bits)
			FskMemPtrDispose(bits);
	}
}

#pragma mark --- text ---

#if !USE_CORE_TEXT
void FskCocoaTextGetFontList(char **fontList)
{
	NSArray *familyNames, *fontNames;
	char	*fontNameString;
	UInt32	fontNameStringLength, fontListLength = 0;

	if (fontList == NULL) return;
	*fontList = NULL;

	familyNames = [UIFont familyNames];

	for (NSString *familyName in familyNames)
	{
		fontNames = [UIFont fontNamesForFamilyName:familyName];

		for (NSString *fontName in fontNames)
		{
			fontNameString = (char *)[fontName UTF8String];
			fontNameStringLength = strlen(fontNameString);

			if (FskMemPtrRealloc(fontListLength + fontNameStringLength + 2, fontList) != kFskErrNone)
				goto bail;

			FskMemMove(*fontList + fontListLength, fontNameString, fontNameStringLength + 1);
			fontListLength += fontNameStringLength + 1;
			(*fontList)[fontListLength] = 0;
		}
	}

bail:
	return;
}

void FskCocoaTextGetBounds(const char *text, UInt32 textLength, UInt16 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache cache)
{
	NSString	*textString;
	UIFont		*font;
	CGSize		size;

	if (bounds == NULL) return;

	if ((text == NULL) || (textLength == 0))
	{
		FskRectangleSet(bounds, 0, 0, 0, 0);
		return;
	}

	if (NULL != cache)
	{
		textSize = cache->textSize;
		fontName = cache->fontName;
	}

	textString = [[NSString alloc] initWithBytes:text length:textLength encoding:NSUTF8StringEncoding];

	font = [UIFont fontWithName:[NSString stringWithUTF8String:fontName] size:(float)textSize];
	size = [textString sizeWithFont:font];

	FskRectangleSet(bounds, 0, 0, size.width, size.height);

	[textString release];
}

void FskCocoaTextFitWidth(const char *text, UInt32 textLength, UInt16 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytesOut, UInt32 *fitCharsOut, FskTextFormatCache cache)
{
	NSString	*textString, *subString;
	UIFont		*font;
	CGSize		size, wrapSize;
    CGFloat     singleHeight;
    NSUInteger length, minInLength, maxOutLength;

    if (fitBytesOut)
        *fitBytesOut = 0;

    if (fitCharsOut)
        *fitCharsOut = 0;

	if ((text == NULL) || (textLength == 0))
	{
		return;
	}

	if (NULL != cache)
	{
		textSize = cache->textSize;
		fontName = cache->fontName;
	}

	textString = [[NSString alloc] initWithBytes:text length:textLength encoding:NSUTF8StringEncoding];
    length =[textString length];
    maxOutLength = length + 1;
    minInLength = 0;

	font = [UIFont fontWithName:[NSString stringWithUTF8String:fontName] size:(float)textSize];

    size = [textString sizeWithFont:font];

    if (size.width > width)
    {
        if (kFskTextFitFlagBreak & flags)
        {
            NSScanner *scanner;
            NSCharacterSet *set;
            NSUInteger scanLocation;
            NSString *dummy;

            singleHeight = size.height;
            wrapSize = CGSizeMake((CGFloat)width, FLT_MAX);

            set = [NSCharacterSet whitespaceCharacterSet];
            scanner = [NSScanner scannerWithString:textString];

            length = 0;
            do {
                if (![scanner scanUpToCharactersFromSet:set intoString:&dummy])
                {
                    break;
                }
                scanLocation = [scanner scanLocation];
                subString = [textString substringToIndex:scanLocation];
                size = [subString sizeWithFont:font constrainedToSize:wrapSize lineBreakMode:UILineBreakModeWordWrap];
                if (size.height > singleHeight)
                {
                    break;
                }
                length = scanLocation;
            } while (1);
        }
        else
        {
            length *= size.width / width;
            do {
				if (length > [textString length]) length = [textString length];
                subString = [textString substringToIndex:length];
                size = [subString sizeWithFont:font];
                if (size.width <= width)
                {
                    minInLength = length;
                    length += (maxOutLength - length) / 2;
                }
                else
                {
                    maxOutLength = length;
                    length -= (length - minInLength) / 2;
                }
                if ((length <= minInLength) || (length >= (maxOutLength - 1)))
                {
                    if (length == maxOutLength)
                    {
                        length--;
                    }
                    break;
                }
            } while (1);
        }
    }

	if (fitBytesOut)
    {
        subString = [textString substringToIndex:length];
		*fitBytesOut = [subString lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    }

	if (fitCharsOut)
		*fitCharsOut = length;
}

void FskCocoaTextGetFontInfo(FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache cache)
{
	CGFontRef 	cgFontRef;

	if (NULL != cache)
	{
		textSize = cache->textSize;
		fontName = cache->fontName;
	}

	if (fontName == NULL) {
		info->ascent  = 10;
		info->descent = 2;
		info->leading = 12;
		info->height  = info->ascent + info->descent;
		info->width   = info->height;					/* First order estimate */
		return;
	}

	cgFontRef = CGFontCreateWithFontName((CFStringRef)[NSString stringWithUTF8String:fontName]);

	if (cgFontRef)
	{
		float toPixels = (float)textSize / (float)CGFontGetUnitsPerEm(cgFontRef);
		CGRect r      = CGFontGetFontBBox(cgFontRef);
		info->width   = (UInt32)(.5f + toPixels * r.size.x);
		info->height  = (UInt32)(.5f + toPixels * r.size.y);
		info->ascent  = (UInt32)(.5f + toPixels * CGFontGetAscent(cgFontRef));
		info->descent = (UInt32)(.5f + toPixels * fabs(CGFontGetDescent(cgFontRef)));
		info->leading = (UInt32)(.5f + toPixels * CGFontGetLeading(cgFontRef));

		CGFontRelease(cgFontRef);
	}
}

Boolean FskCocoaTextFormatCacheNew(FskTextFormatCache *cache, FskBitmap bits, UInt16 textSize, UInt32 textStyle, const char *fontName)
{
	Boolean success = false;
	FskErr	err = kFskErrNone;

	err = FskMemPtrNewClear(sizeof(FskTextFormatCacheRecord), (FskMemPtr *)cache);
	BAIL_IF_ERR(err);

	(*cache)->textSize = textSize;
	(*cache)->fontName = FskStrDoCopy(fontName);
	if (NULL == (*cache)->fontName)
	{
		FskMemPtrDispose((FskMemPtr)(*cache));
		BAIL(kFskErrMemFull);
	}

	success = true;

bail:
	return success;
}

void FskCocoaTextFormatCacheDispose(FskTextFormatCache cache)
{
	if (cache)
	{
		if (NULL != cache->fontName)
			FskMemPtrDispose(cache->fontName);

		FskMemPtrDispose(cache);
	}
}

Boolean FskCocoaTextDraw(FskBitmap fskBitmap, const char *text, UInt32 textLength, const FskRectangle fskRect, const FskRectangle clipFskRect, const FskColorRGB fskColorRGB, UInt32 blendLevel, UInt16 textSize, UInt32 textStyle, UInt16 horizontalAlignment, UInt16 verticalAlignment, const char *fontName, FskTextFormatCache cache)
{
	CGContextRef	context;
	CGFontRef		cgFontRef;
	CGColorSpaceRef	rgbColorSpace;
	float			ascent, descent, fontColor[4];
	CGPoint			drawPoint;
	int				unitsPerEm;
	NSString		*textString;
	UIFont			*font;
	CGSize			textDimensions;

	if ((fskBitmap == NULL) || (text == NULL) || (textLength == 0) || (fskRect == NULL)) return false;

	context = (CGContextRef)(fskBitmap->cgBitmapContext);

	if (NULL != cache) {
		textSize = cache->textSize;
		fontName = cache->fontName;
	}

	// set up context
	CGContextSaveGState(context);
	CGContextSetShouldAntialias(context, true);

	cgFontRef = CGFontCreateWithFontName((CFStringRef)[NSString stringWithUTF8String:fontName]);

	if (cgFontRef)
	{
		unitsPerEm = CGFontGetUnitsPerEm(cgFontRef);

		ascent = (float)CGFontGetAscent(cgFontRef) / unitsPerEm * textSize;
		descent = fabs((float)CGFontGetDescent(cgFontRef) / unitsPerEm * textSize);

		CGFontRelease(cgFontRef);
	}

	fontColor[0] = (float)fskColorRGB->r / 255;
	fontColor[1] = (float)fskColorRGB->g / 255;
	fontColor[2] = (float)fskColorRGB->b / 255;
	fontColor[3] = (float)blendLevel / 255;

	rgbColorSpace = CGColorSpaceCreateDeviceRGB();

	if (rgbColorSpace)
	{
		CGContextSetFillColorSpace(context, rgbColorSpace);
		CGColorSpaceRelease(rgbColorSpace);
	}

	CGContextSetFillColor(context, fontColor);
	CGContextSelectFont(context, fontName, textSize, kCGEncodingMacRoman);

	drawPoint = CGPointMake(fskRect->x, fskBitmap->bounds.height - fskRect->y - fskRect->height);

	switch (horizontalAlignment)
	{
		case kFskTextAlignCenter:
			textString = [[NSString alloc] initWithBytes:text length:textLength encoding:NSUTF8StringEncoding];
			font = [UIFont fontWithName:[NSString stringWithUTF8String:fontName] size:(float)textSize];
			textDimensions = [textString sizeWithFont:font];
			[textString release];

			drawPoint.x += (fskRect->width - textDimensions.width) / 2;
			break;
		case kFskTextAlignRight:
			textString = [[NSString alloc] initWithBytes:text length:textLength encoding:NSUTF8StringEncoding];
			font = [UIFont fontWithName:[NSString stringWithUTF8String:fontName] size:(float)textSize];
			textDimensions = [textString sizeWithFont:font];
			[textString release];

			drawPoint.x += fskRect->width - textDimensions.width;
			break;
		case kFskTextAlignLeft:
		default:
			break;
	}

	switch (verticalAlignment)
	{
		case kFskTextAlignTop:
			drawPoint.y += fskRect->height - ascent;
			break;
		case kFskTextAlignBottom:
			drawPoint.y += descent;
			break;
		case kFskTextAlignCenter:
		default:
			drawPoint.y += fmax((fskRect->height - ascent - descent + 1) / 2, descent);
			break;
	}

	CGContextShowTextAtPoint(context, drawPoint.x, drawPoint.y, text, textLength);

	// restore context
	CGContextRestoreGState(context);

	return true;
}
#endif  /* !USE_CORE_TEXT */

#pragma mark --- audio ---

#if !USE_AUDIO_QUEUE
Boolean FskCocoaAudioInitialize(FskAudioOut fskAudioOut)
{
    //NSLog(@"FskCocoaAudioInitialize");
	Boolean						success = false;
	OSStatus		 			err = noErr;
	AudioComponentDescription	audioComponentDescription = {0};
	AudioComponent				outputComponent = NULL;
	AudioUnit 					outputAudioUnit = NULL;
//...    AudioConverterRef			audioConverter = NULL;
	AURenderCallbackStruct		auRenderCallback = {0};
    AudioStreamBasicDescription	inputAudioStreamBasicDescription = {0}, outputAudioStreamBasicDescription = {0};
    UInt32						size;

	if (fskAudioOut == NULL) goto bail;

	fskAudioOut->sampleRate = 44100;
	fskAudioOut->numChannels = 2;
#if TARGET_RT_LITTLE_ENDIAN
	fskAudioOut->format = kFskAudioFormatPCM16BitLittleEndian;
	inputAudioStreamBasicDescription.mFormatFlags = kAudioFormatFlagIsSignedInteger;
#else
	fskAudioOut->format = kFskAudioFormatPCM16BitBigEndian;
	inputAudioStreamBasicDescription.mFormatFlags = (kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsBigEndian);
#endif

	inputAudioStreamBasicDescription.mFormatID = kAudioFormatLinearPCM;
	inputAudioStreamBasicDescription.mSampleRate = fskAudioOut->sampleRate;
	inputAudioStreamBasicDescription.mBytesPerPacket = 4;
	inputAudioStreamBasicDescription.mFramesPerPacket = 1;
	inputAudioStreamBasicDescription.mBytesPerFrame = 4;
	inputAudioStreamBasicDescription.mChannelsPerFrame = fskAudioOut->numChannels;
	inputAudioStreamBasicDescription.mBitsPerChannel = 16;

	// initialize the output audio unit
	audioComponentDescription.componentType = kAudioUnitType_Output;
	audioComponentDescription.componentSubType = kAudioUnitSubType_GenericOutput;

	outputComponent = AudioComponentFindNext(NULL, &audioComponentDescription);

	if (outputComponent == NULL)
	{
		BAIL(kFskErrOperationFailed);
	}

	err = AudioComponentInstanceNew(outputComponent, &outputAudioUnit);
	BAIL_IF_ERR(err);

	err = AudioUnitInitialize(outputAudioUnit);
	BAIL_IF_ERR(err);

	// match the input and output stream formats
	size = sizeof(AudioStreamBasicDescription);
	err = AudioUnitGetProperty(outputAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &outputAudioStreamBasicDescription, &size);
	BAIL_IF_ERR(err);
	err = AudioUnitSetProperty(outputAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &outputAudioStreamBasicDescription, size);
	BAIL_IF_ERR(err);

	// set the output audio unit render callback
	auRenderCallback.inputProc = AURender;
	auRenderCallback.inputProcRefCon = fskAudioOut;
	err = AudioUnitSetProperty(outputAudioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &auRenderCallback, sizeof(auRenderCallback));
	BAIL_IF_ERR(err);

	// create the audio converter
//	err = AudioConverterNew(&inputAudioStreamBasicDescription, &outputAudioStreamBasicDescription, &audioConverter);
//...	BAIL_IF_ERR(err);
	BAIL_IF_ERR(err);

	// set the volume
	FskCocoaAudioSetVolume(fskAudioOut, fskAudioOut->leftVolume, fskAudioOut->rightVolume);

	success = true;

bail:
	if (!success)
	{
		if (outputAudioUnit)
		{
			AudioUnitUninitialize(outputAudioUnit);
			AudioComponentInstanceDispose(outputAudioUnit);
			outputAudioUnit = NULL;
		}

//		if (audioConverter)
//		{
//			AudioConverterDispose(audioConverter);
//			audioConverter = NULL;
//		}
	}

	fskAudioOut->outputAudioUnit = outputAudioUnit;
//	fskAudioOut->audioConverter = audioConverter;

	return success;
}

void FskCocoaAudioTerminate(FskAudioOut fskAudioOut)
{
    //NSLog(@"FskCocoaAudioTerminate");
	AudioUnit 			outputAudioUnit;
//    AudioConverterRef	audioConverter;

	if (fskAudioOut == NULL) return;
	outputAudioUnit = fskAudioOut->outputAudioUnit;
//	audioConverter = fskAudioOut->audioConverter;

	// terminate
	if (outputAudioUnit)
	{
		AudioOutputUnitStop(outputAudioUnit);
		AudioUnitUninitialize(outputAudioUnit);
		AudioComponentInstanceDispose(outputAudioUnit);
		fskAudioOut->outputAudioUnit = NULL;
	}

//	if (audioConverter)
//	{
//		AudioConverterDispose(audioConverter);
//		fskAudioOut->audioConverter = NULL;
//	}
}

void FskCocoaAudioGetVolume(FskAudioOut fskAudioOut, UInt16 *leftVolume, UInt16 *rightVolume)
{
    //NSLog(@"FskCocoaAudioGetVolume");
	AudioUnit 	outputAudioUnit;
	Float32		volume = 0;

	if (fskAudioOut == NULL) return;
	outputAudioUnit = fskAudioOut->outputAudioUnit;
	if (outputAudioUnit == NULL) return;

	// get the volume
	AudioUnitGetParameter(outputAudioUnit, kHALOutputParam_Volume, kAudioUnitScope_Global, 0, &volume);
	fskAudioOut->leftVolume = volume * 256;
	fskAudioOut->rightVolume = volume * 256;

	if (leftVolume)
		*leftVolume = fskAudioOut->leftVolume;

	if (rightVolume)
		*rightVolume = fskAudioOut->rightVolume;
}

void FskCocoaAudioSetVolume(FskAudioOut fskAudioOut, UInt16 leftVolume, UInt16 rightVolume)
{
    //NSLog(@"FskCocoaAudioSetVolume");
	AudioUnit outputAudioUnit;

	if (fskAudioOut == NULL) return;
	outputAudioUnit = fskAudioOut->outputAudioUnit;
	if (outputAudioUnit == NULL) return;

	// set the volume
	fskAudioOut->leftVolume = leftVolume;
	fskAudioOut->rightVolume = rightVolume;
	AudioUnitSetParameter(outputAudioUnit, kHALOutputParam_Volume, kAudioUnitScope_Global, 0, ((double)leftVolume / 256), 0);
}

Boolean FskCocoaAudioStart(FskAudioOut fskAudioOut)
{
    //NSLog(@"FskCocoaAudioStart");
	Boolean		success = false;
	OSStatus	err = noErr;
	AudioUnit 	outputAudioUnit;

	if (fskAudioOut == NULL) return success;
	outputAudioUnit = fskAudioOut->outputAudioUnit;
	if (outputAudioUnit == NULL) return success;

	// start
	err = AudioOutputUnitStart(outputAudioUnit);
	BAIL_IF_ERR(err);
	fskAudioOut->startHostTime = AudioGetCurrentHostTime();

	success = true;

bail:
	return success;
}

void FskCocoaAudioStop(FskAudioOut fskAudioOut)
{
    //NSLog(@"FskCocoaAudioStop");
	AudioUnit 			outputAudioUnit;
//    AudioConverterRef	audioConverter;

	if (fskAudioOut == NULL) return;
	outputAudioUnit = fskAudioOut->outputAudioUnit;
//	audioConverter = fskAudioOut->audioConverter;
//	if ((outputAudioUnit == NULL) || (audioConverter == NULL)) return;

	// stop
	fskAudioOut->currentHostTime = AudioGetCurrentHostTime();
	AudioOutputUnitStop(outputAudioUnit);
//	AudioConverterReset(audioConverter);
	AudioUnitReset(outputAudioUnit, kAudioUnitScope_Output, 0);
}

void FskCocoaAudioGetSamplePosition(FskAudioOut fskAudioOut, FskSampleTime *fskSampleTime)
{
	if ((fskAudioOut == NULL) || (fskSampleTime == NULL)) return;

	// get the sample position
	if (fskAudioOut->playing)
		*fskSampleTime = fskAudioOut->zeroTime + (AudioGetCurrentHostTime() - fskAudioOut->startHostTime) * fskAudioOut->sampleRate / AudioGetHostClockFrequency();
	else
		*fskSampleTime = fskAudioOut->zeroTime + (fskAudioOut->currentHostTime - fskAudioOut->startHostTime) * fskAudioOut->sampleRate / AudioGetHostClockFrequency();
}
#endif  /* !USE_AUDIO_QUEUE */

#pragma mark --- timer ---
void FskCocoaTimerReschedule(FskThread thread)
{
	FskTimeRecord			nextTime, nowTime;
	CFTimeInterval			interval;
	CFRunLoopTimerContext	context = {0};

	if (thread->timerCallbacks)
	{
		FskTimeGetNow(&nowTime);
		if (!FskTimeCallbackGetNextTime(&nowTime, &nextTime, thread))
			interval = 0.0001;
		else
		{
			FskTimeSub(&nowTime, &nextTime);

			interval = ((CFTimeInterval)nextTime.seconds);
			interval += ((CFTimeInterval)nextTime.useconds / (1000.0*1000.0));
		}

		if (thread->cfRunLoopTimer)
			CFRunLoopTimerSetNextFireDate(thread->cfRunLoopTimer, CFAbsoluteTimeGetCurrent() + interval);
		else
		{
			context.info = thread;
			thread->cfRunLoopTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + interval, CFAbsoluteTimeGetCurrent() + interval, 0, 0, FskCocoaTimerCallback, &context);
			CFRunLoopAddTimer(thread->cfRunloop, thread->cfRunLoopTimer, kCFRunLoopDefaultMode);
		}
	}
	else if (thread->cfRunLoopTimer)
	{
		CFRunLoopTimerInvalidate(thread->cfRunLoopTimer);
		CFRelease(thread->cfRunLoopTimer);
		thread->cfRunLoopTimer = NULL;
	}
}

#pragma mark --- time ---
time_t FskCocoa_mktime(struct tm *time)
{
	CFGregorianDate	gregorianDate = {0};
	CFAbsoluteTime 	absoluteTime;

	gregorianDate.year = time->tm_year + 1900;
	gregorianDate.month = time->tm_mon + 1;
	gregorianDate.day = time->tm_mday;
	gregorianDate.hour = time->tm_hour;
	gregorianDate.minute = time->tm_min;
	gregorianDate.second = time->tm_sec;

	absoluteTime = CFGregorianDateGetAbsoluteTime(gregorianDate, NULL) + kCFAbsoluteTimeIntervalSince1970;

	return (time_t)absoluteTime;
}

#pragma mark --- word ---
char *FskCocoaWordVersion()
{
 	return "Cocoa NSAttributedString";
}

Boolean FskCocoaWordGetAttributes(char *filePath, FskMediaPropertyValue name, FskMediaPropertyValue author, FskMediaPropertyValue subject)
{
	Boolean 			success = false;
/*
	NSData				*data;
	NSAttributedString 	*docAttributedString = NULL;
	NSDictionary		*docAttributes;

	if (filePath == NULL) goto bail;

	data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:filePath]];

	if (data)
	{
		docAttributedString = [[NSAttributedString alloc] initWithDocFormat:data documentAttributes:&docAttributes];

		if (docAttributes)
		{
			FskCocoaAttributedStringGetAttribute(docAttributes, NSTitleDocumentAttribute, name);
			FskCocoaAttributedStringGetAttribute(docAttributes, NSAuthorDocumentAttribute, author);
			FskCocoaAttributedStringGetAttribute(docAttributes, NSSubjectDocumentAttribute, subject);
		}

		success = true;
	}

bail:
	[docAttributedString release];
*/
	return success;
}

Boolean FskCocoaWordConvertRTF(char *sourcePath, char *destinationPath, char *format)
{
	Boolean 			success = false;
/*
	NSData				*data, *rtfData = NULL;
	NSAttributedString 	*docAttributedString = NULL;

	if ((sourcePath == NULL) || (destinationPath == NULL)) goto bail;

	data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:sourcePath]];

	if (data && ((format == NULL) || !FskStrCompare(format, "text/rtf") || !FskStrCompare(format, "application/rtf")))
	{
		docAttributedString = [[NSAttributedString alloc] initWithDocFormat:data documentAttributes:NULL];
		rtfData = [docAttributedString RTFFromRange:NSMakeRange(0, [docAttributedString length]) documentAttributes:NULL];

		if (rtfData)
			success = [rtfData writeToFile:[NSString stringWithUTF8String:destinationPath] atomically:NO];
	}

bail:
	[docAttributedString release];
*/
	return success;
}

#pragma mark --- html ---
char *FskCocoaHTMLVersion()
{
 	return "Cocoa NSAttributedString";
}

Boolean FskCocoaHTMLGetAttributes(char *filePath, FskMediaPropertyValue name, FskMediaPropertyValue author, FskMediaPropertyValue subject)
{
	Boolean 			success = false;
/*
	NSData				*data;
	NSAttributedString 	*htmlAttributedString = NULL;
	NSDictionary		*htmlAttributes;

	if (filePath == NULL) goto bail;

	data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:filePath]];

	if (data)
	{
		htmlAttributedString = [[NSAttributedString alloc] initWithHTML:data documentAttributes:&htmlAttributes];

		if (htmlAttributes)
		{
			FskCocoaAttributedStringGetAttribute(htmlAttributes, NSTitleDocumentAttribute, name);
			FskCocoaAttributedStringGetAttribute(htmlAttributes, NSAuthorDocumentAttribute, author);
			FskCocoaAttributedStringGetAttribute(htmlAttributes, NSSubjectDocumentAttribute, subject);
		}

		success = true;
	}

bail:
	[htmlAttributedString release];
*/
	return success;
}

Boolean FskCocoaHTMLConvertRTF(char *sourcePath, char *destinationPath, char *format)
{
	Boolean 			success = false;
/*
	NSData				*data, *rtfData = NULL;
	NSAttributedString 	*htmlAttributedString = NULL;

	if ((sourcePath == NULL) || (destinationPath == NULL)) goto bail;

	data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:sourcePath]];

	if (data && ((format == NULL) || !FskStrCompare(format, "text/rtf") || !FskStrCompare(format, "application/rtf")))
	{
		htmlAttributedString = [[NSAttributedString alloc] initWithHTML:data documentAttributes:NULL];
		rtfData = [htmlAttributedString RTFFromRange:NSMakeRange(0, [htmlAttributedString length]) documentAttributes:NULL];

		if (rtfData)
			success = [rtfData writeToFile:[NSString stringWithUTF8String:destinationPath] atomically:NO];
	}

bail:
	[htmlAttributedString release];
*/
	return success;
}

#pragma mark --- open ---
Boolean FskCocoaOpenURL(const char *url)
{
	return ([[UIApplication sharedApplication] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]] == YES);
}

Boolean FskCocoaOpenFile(const char *filePath)
{
	return ([[UIApplication sharedApplication] openURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:filePath]]] == YES);
}

#pragma mark --- device ---
char *FskCocoaDeviceName()
{
	UIDevice *device = [UIDevice currentDevice];
	NSString *deviceName = [device name];
	return (char *)[deviceName UTF8String];
}

#if 0   // UDID is deprecated
char *FskCocoaDeviceUDID()
{
	UIDevice *device = [UIDevice currentDevice];
	NSString *deviceUDID = [device uniqueIdentifier];
	return (char *)[deviceUDID UTF8String];
}
#endif

static CGFloat sDeviceScreenScaleFactor = -1.0;

float FskCocoaDeviceScreenScaleFactor()
{
	if (sDeviceScreenScaleFactor < 0.0)
	{
		sDeviceScreenScaleFactor = 1.0;

		if ([UIView instancesRespondToSelector:@selector(setContentScaleFactor:)])
		{
			UIScreen *screen;

			screen = [UIScreen mainScreen];

			if ([screen respondsToSelector:@selector(scale)])
			{
				sDeviceScreenScaleFactor = [screen scale];
			}
		}
	}
	return sDeviceScreenScaleFactor;
}

#pragma mark --- autoreleasePool ---

FskCocoaAutoreleasePool FskCocoaAutoreleasePoolNew()
{
	return (FskCocoaAutoreleasePool)[[NSAutoreleasePool alloc] init];
}

void FskCocoaAutoreleasePoolDispose(FskCocoaAutoreleasePool pool)
{
	if (pool) {
		NSAutoreleasePool *autoreleasePool = (NSAutoreleasePool*)pool;
		[autoreleasePool drain];
	}
}


#pragma mark --- EAGL ---
EAGLContext* FskEAGLContextNew(int version, EAGLContext *share)
{
	EAGLRenderingAPI api;
	switch (version) {
		case 1:	api = kEAGLRenderingAPIOpenGLES1;	break;
		default:
		case 2:	api = kEAGLRenderingAPIOpenGLES2;	break;
		case 3:	api = kEAGLRenderingAPIOpenGLES3;	break;
	}
	if (share)	return [[EAGLContext alloc] initWithAPI:api sharegroup:[share sharegroup]];
	else		return [[EAGLContext alloc] initWithAPI:api];
}

EAGLContext* FskEAGLContextGetCurrent(void)
{
	return [EAGLContext currentContext];
}

FskErr FskEAGLContextSetCurrent(EAGLContext *ctx)
{
	return [EAGLContext setCurrentContext:ctx] ? kFskErrNone : kFskErrEAGLBadContext;
}

#ifndef GL_RENDERBUFFER				/* #defined in <OpenGL/gl.h> */
	#define GL_RENDERBUFFER 0x8D41
#endif /* GL_RENDERBUFFER  */

FskErr FskEAGLSwapBuffers(EAGLContext *ctx)
{
	return [ctx presentRenderbuffer:GL_RENDERBUFFER] ? kFskErrNone : kFskErrOperationFailed;
}

#if 0  /* Apparently setCurrentContext invokes automatic reference counting mode, so release is not necessary nor available. */
FskErr FskEAGLContextDispose(EAGLContext *ctx)
{
	if ([EAGLContext currentContext] == ctx]
		[EAGLContext setCurrentContext:nil];
	[ctx release];
}
#endif


#pragma mark --- private ---
void FskCocoaTimerCallback(CFRunLoopTimerRef timer, void *info)
{
	FskCocoaThreadWake(info);
}

void FskCocoaThreadRunLoopSourceCallback(void *info)
{
	// do nothing
}

void FskCocoaAttributedStringGetAttribute(NSDictionary *attributes, NSString *attributeKey, FskMediaPropertyValue propertyValue)
{
	NSString *attribute;

	if (propertyValue)
	{
		attribute = [attributes valueForKey:attributeKey];

		if (attribute)
		{
			propertyValue->type = kFskMediaPropertyTypeString;
			propertyValue->value.str = FskStrDoCopy([attribute UTF8String]);
		}
	}

}

FskCocoaPhotoAssets FskCocoaPhotoAssetsNew(FskCocoaPhotoGroupAssetsCallback callback, void *closure)
{
	ALAssetsLibraryGroupsEnumerationResultsBlock usingBlock = ^(ALAssetsGroup *group, BOOL *stop) {
		(*callback)((FskCocoaPhotoAssetGroup)group, closure);
	};

	ALAssetsLibraryAccessFailureBlock failureBlock = ^(NSError *error) {
		(*callback)(NULL, closure);
	};

	ALAssetsLibrary *assetsLibrary = [[ALAssetsLibrary alloc] init];

	NSUInteger groupTypes = ALAssetsGroupAll;
	[assetsLibrary enumerateGroupsWithTypes:groupTypes usingBlock:usingBlock failureBlock:failureBlock];

	return (FskCocoaPhotoAssets)assetsLibrary;
}

int FskCocoaPhotoAssetsGetCount(FskCocoaPhotoAssetGroup group)
{
	return (int)((ALAssetsGroup *)group).numberOfAssets;
}

void FskCocoaPhotoAssetsGetImage(FskCocoaPhotoAssetGroup group, int i, FskCocoaPhotoAssetsCallback callback, void *closure)
{
	ALAssetsGroupEnumerationResultsBlock usingBlock = ^(ALAsset *result, NSUInteger index, BOOL *stop) {
		ALAssetRepresentation *rep = result.defaultRepresentation;
		FskMemPtr data = NULL;
		UInt32 size = 0;

		if (index == NSNotFound)
			return;
		if (FskMemPtrNew((UInt32)rep.size, &data) == kFskErrNone)
			size = (UInt32)[rep getBytes:data fromOffset:0 length:(NSUInteger)rep.size error:nil];
		(*callback)(data, size, closure);
	};

	[(ALAssetsGroup *)group enumerateAssetsAtIndexes:[NSIndexSet indexSetWithIndex:i] options:0 usingBlock:usingBlock];
}

void FskCocoaPhotoAssetsDispose(FskCocoaPhotoAssets assets)
{
	[(ALAssetsLibrary *)assets release];
}

#pragma mark --- media ---
UIImage *
FskCocoaMediaImageFromArtwork(MPMediaItemArtwork *artwork, SInt32 generation, UInt32 targetWidth, UInt32 targetHeight)
{
	if (artwork == nil) {
		return nil;
	}

	CGSize size;
	if (targetWidth && targetHeight) {
		size.width = targetWidth;
		size.height = targetHeight;
	}
	else {
		size.width = 150;
		size.height = 150;
	}

	CGRect bounds = artwork.bounds;
	CGFloat half = bounds.size.width / 2, quarter = bounds.size.width / 4;
	SInt32 percent = (generation <= 4) ? 95 : 40;

	if (size.width < ((half * percent) / 100))
		size.width = size.height = quarter;
	else if (size.width < ((half * 3) / 2))
		size.width = size.height = half;
	else
		size.width = size.height = bounds.size.width;

	UIImage *image = [artwork imageWithSize: size];

	return image;
}

NSString *
FskCocoaMediaIdFromURLString(NSString *urlString, BOOL urlEncoded)
{
	NSURL *url;
	NSArray *queryArray;
	NSString *idString = nil;

	if (urlEncoded)
	{
#if (__IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_7_0)
		if (![urlString respondsToSelector:@selector(stringByRemovingPercentEncoding)])
		{
			const char *urlStr = [urlString UTF8String];
			char *decodedStr;

			BAIL_IF_ERR(FskMemPtrNewClear(FskStrLen(urlStr) + 1, &decodedStr));
			FskStrDecodeEscapedChars(urlStr, decodedStr);
			urlString = [NSString stringWithUTF8String:decodedStr];
			FskMemPtrDispose(decodedStr);
		}
		else
#endif
		{
			urlString = [urlString stringByRemovingPercentEncoding];
		}
	}

	url = [NSURL URLWithString:urlString];
	queryArray = [url.query componentsSeparatedByString:@"&"];

	for (NSString *queryString in queryArray)
	{
		NSArray *pair = [queryString componentsSeparatedByString:@"="];
		if (pair.count != 2) continue;
		if (![(NSString *)[pair objectAtIndex:0] isEqualToString:@"id"]) continue;
		idString = (NSString *)[pair objectAtIndex:1];
		break;
	}

bail:
	return idString;
}
