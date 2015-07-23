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
#import <Cocoa/Cocoa.h>
#import <CoreAudio/CoreAudio.h>
#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreWLAN/CoreWLAN.h>

#define __FSKWINDOW_PRIV__
#define __FSKMENU_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKIMAGE_PRIV__
#define __FSKAUDIO_PRIV__
#define __FSKTHREAD_PRIV__
#define __FSKTIME_PRIV__

#import "FskCocoaSupport.h"
#import "FskCocoaWindow.h"
#import "FskCocoaView.h"
#import "FskCocoaApplication.h"
#import "FskEnvironment.h"
#import "FskTextConvert.h"
#import "FskTime.h"
#import "FskFS.h"

#pragma mark --- defines ---

#if !USE_CORE_TEXT
#define kATSUIStyleTagCount		6
#define kATSUILayoutTagCount	4
#endif	/* !USE_CORE_TEXT */

#pragma mark --- structures ---
#if !USE_CORE_TEXT
struct FskTextFormatCacheRecord
{
	ATSUStyle style;
};

struct FskTextFontListParamRecord
{
	char 	**fontList;
	UInt32	fontListLength;
};
typedef struct FskTextFontListParamRecord FskTextFontListParamRecord;
typedef struct FskTextFontListParamRecord *FskTextFontListParam;
#endif	/* !USE_CORE_TEXT */

#pragma mark --- externs ---
extern void mainExtensionInitialize();
extern void mainExtensionTerminate();

#pragma mark --- private ---
#if !USE_CORE_TEXT
ATSUStyle FskCocoaTextCreateATSUIStyle(const char *fontName, UInt32 textSize, UInt32 textStyle);
OSStatus ATSFontApply(ATSFontRef fontRef, void *refCon);
#endif	/* !USE_CORE_TEXT */
void FskCocoaTimerCallback(CFRunLoopTimerRef timer, void *info);
void FskCocoaThreadRunLoopSourceCallback(void *info);
void FskCocoaAttributedStringGetAttribute(NSDictionary *attributes, NSString *attributeKey, FskMediaPropertyValue propertyValue);

#pragma mark --- callbacks ---
#if !USE_AUDIO_QUEUE
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

	return err;
}

OSStatus AURender(void *refCon, AudioUnitRenderActionFlags *inActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumFrames, AudioBufferList *ioData)
{
	OSStatus			err = noErr;
	FskAudioOut			fskAudioOut;
    AudioConverterRef	audioConverter;
	Boolean				doRelease = false;

	fskAudioOut = (FskAudioOut)refCon;
	if (fskAudioOut == NULL) goto bail;
	audioConverter = fskAudioOut->audioConverter;
	if (audioConverter == NULL) goto bail;

	FskMutexAcquire(fskAudioOut->mutex);
	doRelease = true;

	// fill the audio converter buffer
	err = AudioConverterFillBuffer(audioConverter, ACInputData, fskAudioOut, &(ioData->mBuffers[0].mDataByteSize), ioData->mBuffers[0].mData);
	BAIL_IF_ERR(err);
	fskAudioOut->currentHostTime = inTimeStamp->mHostTime;

bail:
	if (doRelease)
		FskMutexRelease(fskAudioOut->mutex);
	return err;
}
#endif

#pragma mark --- application ---
void FskCocoaApplicationRun()
{
	@try {
	// run
	[NSApp run];
    }
	@catch (NSException * exception) {
	}
	@finally {
	}
}

void FskCocoaApplicationStop()
{
	NSEvent *event;

	// stop
	[NSApp stop:nil];

	// tickle the main thread's runloop
	event = [NSEvent otherEventWithType:NSApplicationDefined location:NSZeroPoint modifierFlags:0 timestamp:0
		windowNumber:0 context:nil subtype:0 data1:kEventClassFsk data2:kEventWakeupMainRunloopEvent];

	// queue the event
	[NSApp postEvent:event atStart:NO];
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
		NSEvent *event;
		[NSApp stop:nil];
		event = [NSEvent otherEventWithType:NSApplicationDefined location:NSZeroPoint modifierFlags:0 timestamp:0
			windowNumber:0 context:nil subtype:0 data1:kEventClassFsk data2:kEventWakeupMainRunloopEvent];
		[NSApp postEvent:event atStart:NO];
	}
	else if (fskThread->threadIsRunning) {
        fskThread->wake = true;
        CFRunLoopStop(fskThread->cfRunloop);
        CFRunLoopWakeUp(fskThread->cfRunloop);
    }
}

#pragma mark --- window ---
Boolean FskCocoaWindowCreate(FskWindow fskWindow, Boolean isCustomWindow, SInt32 width, SInt32 height)
{
	Boolean			success = false;
	FskCocoaWindow 		*window = nil;
	unsigned int	styleMask;

	if (fskWindow == NULL) goto bail;

	if (isCustomWindow)
		styleMask = NSBorderlessWindowMask;
	else
		styleMask = (NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask);

	// create the window
	window = [[FskCocoaWindow alloc] initWithContentRect:NSMakeRect(100, [[NSScreen mainScreen] frame].size.height - height - 100, width, height)
		styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
	if (window == nil) goto bail;

	[window setFskWindow:fskWindow];
	fskWindow->nsWindow = window;
	success = true;

bail:
	return success;
}

void FskCocoaWindowDispose(FskWindow fskWindow)
{
	NSWindow *window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// dispose the window
	[window orderOut:nil];
	[window close];
	fskWindow->nsWindow = nil;
	[window release];
}

void *FskCocoaWindowGetWindowRef(FskWindow fskWindow)
{
	NSWindow	*window;
	WindowRef	windowRef;

	if (fskWindow == NULL) return NULL;
	window = (NSWindow *)(fskWindow->nsWindow);

	// get the windowRef
	windowRef = [window windowRef];

	return windowRef;
}

void FskCocoaWindowToggleFullScreen(FskWindow fskWindow)
{
	NSWindow *window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);
#if defined(MAC_OS_X_VERSION_10_7) && (MAC_OS_X_VERSION_10_7 <= MAC_OS_X_VERSION_MAX_ALLOWED)
	if (floor(NSAppKitVersionNumber) >= NSAppKitVersionNumber10_7)
    	[window toggleFullScreen:nil];
#endif
}

void FskCocoaWindowIsFullScreen(FskWindow fskWindow, Boolean *isFullScreen)
{
	NSWindow *window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);
#if defined(MAC_OS_X_VERSION_10_7) && (MAC_OS_X_VERSION_10_7 <= MAC_OS_X_VERSION_MAX_ALLOWED)
	*isFullScreen = ([window styleMask] & NSFullScreenWindowMask) ? true : false;
#else
	*isFullScreen = false;
#endif
}

void FskCocoaWindowMinimize(FskWindow fskWindow)
{
	NSWindow *window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// minimize the window
	[window miniaturize:nil];
}

void FskCocoaWindowIsMinimized(FskWindow fskWindow, Boolean *isMinimized)
{
	NSWindow *window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// is the window minimized?
	if (isMinimized)
		*isMinimized = [window isMiniaturized];
}

void FskCocoaWindowZoom(FskWindow fskWindow)
{
	NSWindow *window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	const char *kiosk = FskEnvironmentGet("kiosk");
	if ((NULL != kiosk) && (0 == FskStrCompare("true", kiosk))) {
#define kMargin (1)
		FskCocoaWindowSetOrigin(fskWindow, -kMargin, -kMargin);
		FskCocoaWindowSetSize(fskWindow, [[NSScreen mainScreen] frame].size.width + (kMargin + kMargin), [[NSScreen mainScreen] frame].size.height + (kMargin + kMargin));
	}
	else {
        // zoom the window
        [window zoom:nil];
    }
}

void FskCocoaWindowIsZoomed(FskWindow fskWindow, Boolean *isZoomed)
{
	NSWindow *window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// is the window zoomed?
	if (isZoomed)
		*isZoomed = [window isZoomed];
}

void FskCocoaWindowCopyBits(FskWindow fskWindow, FskBitmap fskBitmap, const FskRectangle sourceFskRect, const FskRectangle destFskRect)
{
	NSWindow *window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// copy bits
	[(FskCocoaView *)[window contentView] copyBits:fskBitmap sourceRect:sourceFskRect destRect:destFskRect];
}

void FskCocoaWindowGetVisible(FskWindow fskWindow, Boolean *visible)
{
	NSWindow *window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// get visibility
	if (visible)
		*visible = [window isVisible];
}

void FskCocoaWindowSetVisible(FskWindow fskWindow, Boolean visible)
{
	NSWindow *window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// set visibility
	if (visible)
		[window makeKeyAndOrderFront:nil];
	else
		[window orderOut:nil];
}

void FskCocoaWindowGetSize(FskWindow fskWindow, UInt32 *width, UInt32 *height)
{
	NSRect		windowFrame;
	NSWindow	*window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// get the window size
	windowFrame = [window.contentView frame];

	if (width)
		*width = windowFrame.size.width;

	if (height)
		*height = windowFrame.size.height;
}

void FskCocoaWindowSetSize(FskWindow fskWindow, UInt32 width, UInt32 height)
{
	FskCocoaWindow	*window;
	NSRect			currentFrame, newFrame;
	CGFloat contentHeight, titleBarHeight;

	if (fskWindow == NULL) return;
	window = (FskCocoaWindow *)(fskWindow->nsWindow);
	currentFrame = [window frame];
	contentHeight = [window.contentView frame].size.height;
	titleBarHeight = currentFrame.size.height - contentHeight;
	newFrame = NSMakeRect(currentFrame.origin.x, currentFrame.origin.y - (height - contentHeight), width, height);
	newFrame = [window constrainFrame:newFrame];
	newFrame.size.height += titleBarHeight;

	// set the window size
	[window setFrame:newFrame display:YES];
}

void FskCocoaWindowGetOrigin(FskWindow fskWindow, SInt32 *x, SInt32 *y)
{
	NSRect		frame;
	NSWindow	*window;
	CGFloat contentHeight;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// get the window origin
	frame = [window frame];
	contentHeight = [window.contentView frame].size.height;

	if (x)
		*x = frame.origin.x;

	if (y)
		*y = [[window screen] frame].size.height - frame.origin.y - contentHeight;
}

void FskCocoaWindowSetOrigin(FskWindow fskWindow, SInt32 x, SInt32 y)
{
	NSWindow	*window;
	NSPoint		origin;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);
	origin = NSMakePoint(x, [[window screen] frame].size.height - y - [window.contentView frame].size.height);

	// set the window origin
	[window setFrameOrigin:origin];
}

void FskCocoaWindowGetTitle(FskWindow fskWindow, char **title)
{
	FskErr		err = kFskErrNone;
	NSWindow 	*window;
	const char  *titleString;
	char        *titlePtr = NULL;
	UInt32		length;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// get the title
	titleString = [[window title] UTF8String];
	length = FskStrLen(titleString);
	err = FskMemPtrNewClear(length + 1, &titlePtr);
	BAIL_IF_ERR(err);
	FskMemMove(titlePtr, titleString, length);

bail:
    *title = titlePtr;
}

void FskCocoaWindowSetTitle(FskWindow fskWindow, const char *title)
{
	NSWindow 	*window;
	NSString	*titleString;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// set the title
	if (title)
		titleString = [NSString stringWithUTF8String:title];
	else
		titleString = @"";

	[window setTitle:titleString];
}

void FskCocoaWindowGetMouseLocation(FskWindow fskWindow, FskPoint fskPoint)
{
	NSWindow	*window;
	NSPoint		mouseLocation;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// get the mouse location
	if (fskPoint)
	{
		mouseLocation = [window mouseLocationOutsideOfEventStream];
		fskPoint->x = mouseLocation.x;
		fskPoint->y = [window frame].size.height - mouseLocation.y;
	}
}

void FskCocoaWindowSetNeedsDisplay(FskWindow fskWindow, Boolean needsDisplay)
{
	NSWindow *window;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);

	// set needs display
	[[window contentView] setNeedsDisplay:needsDisplay];
}

void FskCocoaWindowSetNeedsDisplayInRect(FskWindow fskWindow, FskRectangle displayFskRect)
{
	NSWindow	*window;
	NSView		*view;
	NSRect		invalidRect;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);
	view = [window contentView];

	// set needs display in rect
	if (displayFskRect)
	{
		invalidRect = NSMakeRect(displayFskRect->x, [view frame].size.height - displayFskRect->y - displayFskRect->height, displayFskRect->width, displayFskRect->height);
		[view setNeedsDisplayInRect:invalidRect];
	}
}

void FskCocoaWindowDragMouseDown(FskWindow fskWindow)
{
	NSWindow	*window;
	NSPoint		point;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);
	point = [window mouseLocationOutsideOfEventStream];

	// save the initial mouse down location
	fskWindow->dragResizeMousePoint.x = point.x;
	fskWindow->dragResizeMousePoint.y = point.y;
}

void FskCocoaWindowDragMouseMoved(FskWindow fskWindow)
{
	NSWindow		*window;
	NSPoint			currentMouseLocation, origin;
	NSRect			windowFrame, screenFrame;
	NSScreen		*windowScreen;
	static float	sMenuBarHeight = -1;
	float			menuBarHeightOffset = 0;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);
	windowFrame = [window frame];
	windowScreen = [window screen];
	screenFrame = [windowScreen frame];

	if (sMenuBarHeight == -1) {
		const char *kiosk = FskEnvironmentGet("kiosk");
		if ((NULL != kiosk) && (0 == FskStrCompare("true", kiosk)))
			sMenuBarHeight = 0;
		else
			sMenuBarHeight = [[NSApp mainMenu] menuBarHeight];
	}

	if (windowScreen == [[NSScreen screens] objectAtIndex:0])
		menuBarHeightOffset = sMenuBarHeight;

	// drag the window
	currentMouseLocation = [window convertBaseToScreen:[window mouseLocationOutsideOfEventStream]];
	origin = NSMakePoint(currentMouseLocation.x - fskWindow->dragResizeMousePoint.x, currentMouseLocation.y - fskWindow->dragResizeMousePoint.y);

	// peg the window origin so it doesn't get dragged up under the menu bar
	if ((origin.y + windowFrame.size.height) > (screenFrame.origin.y + screenFrame.size.height - menuBarHeightOffset))
		origin.y = screenFrame.origin.y + screenFrame.size.height - windowFrame.size.height - menuBarHeightOffset;

	[window setFrameOrigin:origin];
}

void FskCocoaWindowResizeMouseDown(FskWindow fskWindow)
{
	NSWindow 	*window;
	NSPoint		point;
	NSRect		windowFrame;

	if (fskWindow == NULL) return;
	window = (NSWindow *)(fskWindow->nsWindow);
	point = [window convertBaseToScreen:[window mouseLocationOutsideOfEventStream]];
	windowFrame = [window frame];

	// save the mouse down location and current window frame
	fskWindow->dragResizeMousePoint.x = point.x;
	fskWindow->dragResizeMousePoint.y = point.y;

	fskWindow->lastWindowFrame.x = windowFrame.origin.x;
	fskWindow->lastWindowFrame.y = windowFrame.origin.y;
	fskWindow->lastWindowFrame.width = windowFrame.size.width;
	fskWindow->lastWindowFrame.height = windowFrame.size.height;
}

void FskCocoaWindowResizeMouseMoved(FskWindow fskWindow)
{
	FskCocoaWindow	*window;
	NSPoint			currentMouseLocation;
	NSRect			windowFrame;

	if (fskWindow == NULL) return;
	window = (FskCocoaWindow *)(fskWindow->nsWindow);

	// resize the window
	currentMouseLocation = [window convertBaseToScreen:[window mouseLocationOutsideOfEventStream]];
	windowFrame = NSMakeRect(fskWindow->lastWindowFrame.x, fskWindow->lastWindowFrame.y + currentMouseLocation.y - fskWindow->dragResizeMousePoint.y,
		fskWindow->lastWindowFrame.width + currentMouseLocation.x - fskWindow->dragResizeMousePoint.x, fskWindow->lastWindowFrame.height + fskWindow->dragResizeMousePoint.y - currentMouseLocation.y);
	[window setFrame:[window constrainFrame:windowFrame] display:YES];

	// update mouse location and window frame
	fskWindow->dragResizeMousePoint.x = currentMouseLocation.x;
	fskWindow->dragResizeMousePoint.y = currentMouseLocation.y;

	fskWindow->lastWindowFrame.x = windowFrame.origin.x;
	fskWindow->lastWindowFrame.y = windowFrame.origin.y;
	fskWindow->lastWindowFrame.width = windowFrame.size.width;
	fskWindow->lastWindowFrame.height = windowFrame.size.height;
}

void FskCocoaWindowBeginDraw(FskWindow fskWindow)
{
	NSWindow *window = (NSWindow *)(fskWindow->nsWindow);
	[(FskCocoaView *)[window contentView] beginDraw];
}

void FskCocoaWindowEndDraw(FskWindow fskWindow)
{
	NSWindow *window = (NSWindow *)(fskWindow->nsWindow);
	[(FskCocoaView *)[window contentView] endDraw];
}

#pragma mark --- drag and drop ---
void FskCocoaDragDropWindowRegister(FskWindow fskWindow, FskDragDropTargetProc dropTargetProc)
{
	FskCocoaWindow *window;

	if (fskWindow == NULL) return;
	window = (FskCocoaWindow *)(fskWindow->nsWindow);

	// register for drag and drop
	[window registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil]];
	[window setDragDropTargetProc:dropTargetProc];
}

void FskCocoaDragDropWindowUnregister(FskWindow fskWindow)
{
	FskCocoaWindow *window;

	if (fskWindow == NULL) return;
	window = (FskCocoaWindow *)(fskWindow->nsWindow);

	// unregister for drag and drop
	[window unregisterDraggedTypes];
	[window setDragDropTargetProc:nil];
}

void FskCocoaDragDropWindowResult(FskWindow fskWindow, Boolean result)
{
	FskCocoaWindow *window;

	if (fskWindow == NULL) return;
	window = (FskCocoaWindow *)(fskWindow->nsWindow);

	// set the drag and drop result
	[window setDragDropResult:result];
}

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
		NSEvent 	*event;
		NSWindow	*window;

		if (fskWindow == NULL) return;
		window = (NSWindow *)(fskWindow->nsWindow);

		// create the event
		event = [NSEvent otherEventWithType:NSApplicationDefined location:NSZeroPoint modifierFlags:0 timestamp:0
			windowNumber:[window windowNumber] context:nil subtype:0 data1:eventClass data2:eventType];

		// queue the event
		[NSApp postEvent:event atStart:NO];
	}
}

#pragma mark --- cursor ---
void FskCocoaCursorSet(FskWindow fskWindow, UInt32 cursorShape)
{
	//NSWindow	*window;
	NSCursor	*cursor = nil;

	if (fskWindow == NULL) return;
	//window = (NSWindow *)(fskWindow->nsWindow);

	// set the cursor
	switch (cursorShape)
	{
		case kFskCursorArrow:
			cursor = [NSCursor arrowCursor];
			break;
		case kFskCursorAliasArrow:
			cursor = [NSCursor dragLinkCursor];
			break;
		case kFskCursorCopyArrow:
			cursor = [NSCursor dragCopyCursor];
			break;
		case kFskCursorWait:
			cursor = [NSCursor disappearingItemCursor];
			break;
		case kFskCursorIBeam:
			cursor = [NSCursor IBeamCursor];
			break;
		case kFskCursorNotAllowed:
			cursor = [NSCursor operationNotAllowedCursor];
			break;
		case kFskCursorResizeAll:
		case kFskCursorResizeLeftRight:
		case kFskCursorResizeNESW:
		case kFskCursorResizeNWSE:
			cursor = [NSCursor resizeLeftRightCursor];
			break;
		case kFskCursorResizeTopBottom:
			cursor = [NSCursor resizeUpDownCursor];
			break;
		case kFskCursorLink:
			//cursor = [NSCursor openHandCursor];
			cursor = [NSCursor pointingHandCursor];
			break;
		case kFskCursorResizeColumn:
			cursor = [NSCursor resizeLeftRightCursor];
			break;
		case kFskCursorResizeRow:
			cursor = [NSCursor resizeUpDownCursor];
			break;
		default:
			break;
	}

	if (cursor)
	{
		fskWindow->cursorShape = cursorShape;
		[cursor set];
	}
}

#pragma mark --- menu (primitives) ---
void CocoaMenuBarClear()
{
	NSMenu			*mainMenu, *applicationMenu;
	NSEnumerator	*menuEnumerator, *menuItemEnumerator;
	NSMenuItem		*menu, *menuItem;
	NSString		*appName = [[NSRunningApplication currentApplication] localizedName];
	
	mainMenu = [NSApp mainMenu];

	// clear the menu bar (except for the Apple menu)
	menuEnumerator = [[mainMenu itemArray] objectEnumerator];
	menu = [menuEnumerator nextObject];
	applicationMenu = [menu submenu];

	if (menu)
	{
		// remove the About, separator, and Preferences menu items
		for (menuItemEnumerator = [[applicationMenu itemArray] objectEnumerator]; menuItem = [menuItemEnumerator nextObject];)
		{
			if ([menuItem action] == @selector(handleMenuAction:))
				[applicationMenu removeItem:menuItem];
			else {
				NSString *title = [menuItem title];
				NSRange range = [title rangeOfString:@"Kinoma Simulator"];
				if (range.length > 0) {
					title = [title stringByReplacingCharactersInRange:range withString:appName];
					[menuItem setTitle:title];
				}
			}
		}

		while (menu = [menuEnumerator nextObject])
			[mainMenu removeItem:menu];
	}
}

void CocoaMenuAdd(UInt32 menuID, char *title)
{
	NSMenu		*menu;
	NSMenuItem 	*menuItem;
	NSString	*string;

	if (title == NULL) return;

	// add the menu
	string = [NSString stringWithUTF8String:title];
	menuItem = [[[NSMenuItem alloc] init] autorelease];
	menu = [[[NSMenu alloc] initWithTitle:string] autorelease];
	[menu setDelegate:[NSApp delegate]];
	[menu setAutoenablesItems: NO];
	[menuItem setSubmenu:menu];
	[menuItem setTag:(menuID + 1)];
	[menuItem setTitle:string];
	[[NSApp mainMenu] addItem:menuItem];
}

void CocoaMenuItemAdd(UInt32 menuID, UInt32 menuItemID, char *title, char *key, char *command)
{
	NSMenu		*menu;
	NSMenuItem	*menuItem;
	NSString	*keyString;
	int			numberOfItems;
	BOOL		addItem = YES, addSeparator = NO, deleteLastMenuItem = NO, applicationMenuItem = NO;
	UInt32 mask = NSCommandKeyMask;
	char* p;

	if (title == NULL) return;

	menu = [[[NSApp mainMenu] itemWithTag:(menuID + 1)] submenu];
	numberOfItems = [menu numberOfItems];
	if (key && (p = FskStrRChr(key, '+'))) {
		if (FskStrStr(key, "Shift")) mask |= NSShiftKeyMask;
		if (FskStrStr(key, "Alt")) mask |= NSAlternateKeyMask;
		key = p + 1;
	}
	keyString = ((key && key[0]) ? [[NSString stringWithUTF8String:key] lowercaseString] : @"");

	// handle special commands
	if (command)
	{
		if (strcmp(command, "doQuit") == 0)
		{
			deleteLastMenuItem = YES;
			addItem = NO;
		}
		else if (strcmp(command, "doAbout") == 0)
		{
			deleteLastMenuItem = YES;
			addSeparator = YES;
			applicationMenuItem = YES;
		}
		else if (strcmp(command, "doPreferences") == 0)
		{
			deleteLastMenuItem = YES;
			applicationMenuItem = YES;
		}
	}

	if (deleteLastMenuItem && (numberOfItems > 0))
		[menu removeItemAtIndex:(numberOfItems - 1)];

	// add the menu item
	if (addItem)
	{
		menuItem = [[[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:title] action:@selector(handleMenuAction:) keyEquivalent:keyString] autorelease];
		[menuItem setKeyEquivalentModifierMask:mask];
		[menuItem setTag:menuItemID];

		if (applicationMenuItem)
		{
			menu = [[[NSApp mainMenu] itemAtIndex:0] submenu];

			if (addSeparator)
				[menu insertItem:[NSMenuItem separatorItem] atIndex:0];

			[menu insertItem:menuItem atIndex:0];
		}
		else
			[menu addItem:menuItem];
	}
}

void CocoaMenuItemAddSeparator(UInt32 menuID)
{
	NSMenu		*menu;

	menu = [[[NSApp mainMenu] itemWithTag:(menuID + 1)] submenu];

	// add the menu item separator
	[menu addItem:[NSMenuItem separatorItem]];
}

void CocoaMenuItemSetEnable(UInt32 menuID, UInt32 menuItemID, Boolean enable)
{
	NSMenu		*menu;
	NSMenuItem	*menuItem;

	menu = [[[NSApp mainMenu] itemWithTag:(menuID + 1)] submenu];
	menuItem = [menu itemWithTag:menuItemID];

	// set enable
	[menuItem setEnabled:enable];
}

void CocoaMenuItemSetCheck(UInt32 menuID, UInt32 menuItemID, Boolean check)
{
	NSMenu		*menu;
	NSMenuItem	*menuItem;

	menu = [[[NSApp mainMenu] itemWithTag:(menuID + 1)] submenu];
	menuItem = [menu itemWithTag:menuItemID];

	// set check
	[menuItem setState:(check ? NSOnState : NSOffState)];
}

void CocoaMenuItemSetTitle(UInt32 menuID, UInt32 menuItemID, char *title)
{
	NSMenu		*menu;
	NSMenuItem	*menuItem;

	if (title == NULL) return;

	menu = [[[NSApp mainMenu] itemWithTag:(menuID + 1)] submenu];
	menuItem = [menu itemWithTag:menuItemID];

	// set title
	[menuItem setTitle:[NSString stringWithUTF8String:title]];
}

void CocoaMenuItemSendAction(UInt32 menuID, UInt32 menuItemID)
{
	NSMenu		*menu;
	NSMenuItem	*menuItem;

	menu = [[[NSApp mainMenu] itemWithTag:(menuID + 1)] submenu];
	menuItem = [menu itemWithTag:menuItemID];

	// send action
	[menu performActionForItemAtIndex:[menu indexOfItem:menuItem]];
}

#pragma mark --- bitmap ---
Boolean FskCocoaBitmapCreate(FskBitmap fskBitmap, UInt32 pixelFormat, SInt32 width, SInt32 height)
{
	Boolean				success			= false;
	UInt32 				rowBytes		= 0;
	CGColorSpaceRef		colorSpace		= NULL;
	CGContextRef		cgBitmapContext	= NULL;
	void				*bits			= NULL;
	CGImageAlphaInfo	alphaInfo;

	if (fskBitmap == NULL) goto bail;	/* We should already have an FskBitmap data structure allocated */

	if ((width > 0) && (height > 0))
	{
		switch (pixelFormat)
		{
			case kFskBitmapFormat32ARGB:
			case kFskBitmapFormat32ABGR:
				colorSpace = CGColorSpaceCreateDeviceRGB();
				alphaInfo = kCGImageAlphaPremultipliedFirst;
				rowBytes = width * 4;
				break;
			case kFskBitmapFormat32BGRA:
			case kFskBitmapFormat32RGBA:
				colorSpace = CGColorSpaceCreateDeviceRGB();
				alphaInfo = kCGImageAlphaPremultipliedLast;
				rowBytes = width * 4;
				break;
			case kFskBitmapFormat8G:
				colorSpace = CGColorSpaceCreateDeviceGray();
				alphaInfo = kCGImageAlphaNone;
				rowBytes = width;
				break;
            default:
                goto bail;
		}

		bits = malloc(rowBytes * height);	/* We allocate pixel storage here, though */
		if (bits == NULL) goto bail;

		/* Create a CG Bitmap Context, for interface to the Mac OS */
		cgBitmapContext = CGBitmapContextCreate(bits, width, height, 8, rowBytes, colorSpace, (CGBitmapInfo)alphaInfo);
		if (NULL == cgBitmapContext) {
			free(bits);
			goto bail;
		}
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
			free(bits);
	}
}

#pragma mark --- JPEG ---
Boolean FskCocoaJPEGDecompressFrame(const void *data, UInt32 dataSize, UInt32 width, UInt32 height, FskBitmap outputFskBitmap)
{
	Boolean				success = false;
	CGImageRef 			image = NULL;
	CGDataProviderRef	dataProvider = NULL;
	CGContextRef		cgBitmapContext;
	CGRect				cgRect;

	if ((data == NULL) || (dataSize == 0) || (outputFskBitmap == NULL)) goto bail;
	cgBitmapContext = (CGContextRef)(outputFskBitmap->cgBitmapContext);
	dataProvider = CGDataProviderCreateWithData(NULL, data, dataSize, NULL);
	image = CGImageCreateWithJPEGDataProvider(dataProvider, NULL, true, kCGRenderingIntentDefault);
	cgRect = CGRectMake(0, 0, width, height);

	// decompress the frame
	CGContextDrawImage(cgBitmapContext, cgRect, image);
	success = true;

bail:
	if (image)
		CGImageRelease(image);

	if (dataProvider)
		CGDataProviderRelease(dataProvider);

	return success;
}

Boolean FskCocoaJPEGCompressFrame(FskImageCompress fskImageCompress, FskBitmap sourceFskBitmap, const void **data, UInt32 *dataSize)
{
	Boolean				success = false;
	FskErr				err;
	NSData 				*jpegData;
	FskBitmap			drawFskBitmap, tempFskBitmap = NULL;
	CGContextRef		sourceCGBitmapContext;
	NSBitmapImageRep	*bitmapImageRep;
	UInt32				jpegSize;
	const void			*jpegBytes;
    unsigned char 		*dataPlanes[1];

	if ((fskImageCompress == NULL) || (sourceFskBitmap == NULL)) goto bail;

	// get the correct size for the source bitmap
	if ((fskImageCompress->width == (UInt32)sourceFskBitmap->bounds.width) && (fskImageCompress->height == (UInt32)sourceFskBitmap->bounds.height))
		drawFskBitmap = sourceFskBitmap;
	else
	{
		err = FskBitmapNew(fskImageCompress->width, fskImageCompress->height, kFskBitmapFormatDefault, &tempFskBitmap);
		BAIL_IF_ERR(err);
		err = FskBitmapDraw(sourceFskBitmap, &(sourceFskBitmap->bounds), tempFskBitmap, &(tempFskBitmap->bounds), NULL, NULL, kFskGraphicsModeCopy, NULL);
		BAIL_IF_ERR(err);
		drawFskBitmap = tempFskBitmap;
	}

	sourceCGBitmapContext = (CGContextRef)(drawFskBitmap->cgBitmapContext);
    dataPlanes[0] = CGBitmapContextGetData(sourceCGBitmapContext);

	// compress the frame
	bitmapImageRep = [[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:dataPlanes
		pixelsWide:CGBitmapContextGetWidth(sourceCGBitmapContext)
		pixelsHigh:CGBitmapContextGetHeight(sourceCGBitmapContext)
		bitsPerSample:CGBitmapContextGetBitsPerComponent(sourceCGBitmapContext)
		samplesPerPixel:4
		hasAlpha:YES
		isPlanar:NO
		colorSpaceName:NSCalibratedRGBColorSpace
		bitmapFormat:NSAlphaFirstBitmapFormat
		bytesPerRow:CGBitmapContextGetBytesPerRow(sourceCGBitmapContext)
		bitsPerPixel:CGBitmapContextGetBitsPerPixel(sourceCGBitmapContext)] autorelease];
	if (bitmapImageRep == nil) goto bail;
	jpegData = [bitmapImageRep representationUsingType:NSJPEGFileType properties:
		[NSDictionary dictionaryWithObject:[NSNumber numberWithFloat:0.5] forKey:NSImageCompressionFactor]];
	jpegBytes = [jpegData bytes];
	jpegSize = [jpegData length];

	if (dataSize)
		*dataSize = jpegSize;

	if (data)
	{
		err = FskMemPtrNewFromData(jpegSize, jpegBytes, (FskMemPtr *)data);
		BAIL_IF_ERR(err);
	}

	success = true;

bail:
	if (tempFskBitmap)
		FskBitmapDispose(tempFskBitmap);

	return success;
}

#pragma mark --- text ---
#if !USE_CORE_TEXT
void FskCocoaTextGetFontList(void *state, char **fontList)
{
	FskTextFontListParamRecord fontListParam = {0};

	if (fontList == NULL) return;
	*fontList = NULL;
	fontListParam.fontList = fontList;

	ATSFontApplyFunction(ATSFontApply, &fontListParam);
}

void FskCocoaTextGetBounds(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache cache)
{
	FskErr				err					= kFskErrNone;
	OSStatus			status				= noErr;
	UInt16 				*unicodeText		= NULL;			/* Needs to be disposed */
	ATSUTextLayout		textLayout			= NULL;			/* Needs to be disposed */
	UInt32 				unicodeTextLength	= 0;
	Boolean				disposeStyle		= false;		/* If true, ... */
	ATSUStyle			style;								/* ... dispose when done */
	UniCharCount		runLengths;
	ATSUTextMeasurement textBefore = 0, textAfter = 0, ascent = 0, descent = 0;

	if (bounds == NULL) return;

	if ((text == NULL) || (textLength == 0))
	{
		FskRectangleSet(bounds, 0, 0, 0, 0);
		return;
	}

	if (cache && cache->style) {
		style = cache->style;															/* We can reuse the style */
	}
	else {
		style = FskCocoaTextCreateATSUIStyle(fontName, textSize, textStyle);			/* This is a one-time use, ... */
		if (style == NULL) goto bail;
		disposeStyle = true;															/* ... that needs to be disposed when done */
	}

	err = FskTextUTF8ToUnicode16NE(text, textLength, &unicodeText, &unicodeTextLength);	/* Allocate a unicode string equivalent */
    BAIL_IF_ERR(err);

	runLengths = kATSUToTextEnd;
	status = ATSUCreateTextLayoutWithTextPtr(unicodeText, kATSUFromTextBeginning, kATSUToTextEnd, unicodeTextLength / 2, 1, &runLengths, &style, &textLayout);
	if (status != noErr) goto bail;

	status = ATSUGetUnjustifiedBounds(textLayout, kATSUFromTextBeginning, kATSUToTextEnd, &textBefore, &textAfter, &ascent, &descent);
	if (status != noErr) goto bail;

bail:
	FskMemPtrDispose(unicodeText);
	if (textLayout)		ATSUDisposeTextLayout(textLayout);
	if (disposeStyle)	ATSUDisposeStyle(style);
	if (err)			FskRectangleSetEmpty(bounds);
	else				FskRectangleSet(bounds, 0, 0, ceil(Fix2X(textAfter - textBefore)), ceil(Fix2X(ascent + descent)));
}

void FskCocoaTextGetLayout(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName,
							UInt16 **unicodeTextPtr, UInt32 *unicodeLenPtr, FskFixed **layoutPtr, FskTextFormatCache cache)
{
	FskErr				err					= kFskErrNone;
	UInt16 				*unicodeText		= NULL;			/* Needs to be disposed */
	ATSUTextLayout		textLayout			= NULL;			/* Needs to be disposed */
	UInt32 				unicodeByteLength	= 0;
	Boolean				disposeStyle		= false;		/* If true, ... */
	ATSUStyle			style;								/* ... dispose when done */
	UniCharCount		runLengths;
	ATSUTextMeasurement textBefore, textAfter, ascent, descent;

	if (cache && cache->style) {
		style = cache->style;															/* We can reuse the style */
	}
	else {
		BAIL_IF_NULL(style = FskCocoaTextCreateATSUIStyle(fontName, textSize, textStyle), err, kFskErrOperationFailed);	/* This is a one-time use, ... */
		disposeStyle = true;																							/* ... that needs to be disposed when done */
	}

	BAIL_IF_ERR(err = FskTextUTF8ToUnicode16NE(text, textLength, &unicodeText, &unicodeByteLength));	/* Allocate a unicode string equivalent */
	runLengths = kATSUToTextEnd;
	BAIL_IF_ERR(err = ATSUCreateTextLayoutWithTextPtr(unicodeText, kATSUFromTextBeginning, kATSUToTextEnd, unicodeByteLength / 2, 1, &runLengths, &style, &textLayout));
	BAIL_IF_ERR(err = ATSUGetUnjustifiedBounds(textLayout, kATSUFromTextBeginning, kATSUToTextEnd, &textBefore, &textAfter, &ascent, &descent));

	if (layoutPtr) {
		ItemCount n;
		Fixed *deltaX;
		ItemCount i;
		ATSLayoutRecord* layoutRecords;
		BAIL_IF_ERR(err = ATSUDirectGetLayoutDataArrayPtrFromTextLayout(textLayout, 0, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent, (&layoutRecords), &n));
		BAIL_IF_ZERO(n, err, kFskErrUnknown);
		--n;
		BAIL_IF_FALSE(unicodeByteLength/sizeof(**unicodeTextPtr) == n, err, kFskErrMismatch);
		BAIL_IF_ERR(err = FskMemPtrNew(n * sizeof(**layoutPtr), layoutPtr));
		for (i = 0; i < n; ++i)
			(*layoutPtr)[i] = layoutRecords[i].realPos;
		ATSUDirectReleaseLayoutDataArrayPtr(NULL, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent, &layoutRecords);
	}
	if (unicodeTextPtr) {
		*unicodeTextPtr = unicodeText;	/* Return the unicode text ... */
		unicodeText = NULL;				/* ... rather than disposing it here */
	}
	if (unicodeLenPtr)
		*unicodeLenPtr = unicodeByteLength;

bail:
	FskMemPtrDispose(unicodeText);
	if (textLayout)		ATSUDisposeTextLayout(textLayout);
	if (disposeStyle)	ATSUDisposeStyle(style);
	if (err) {
		if (layoutPtr)		*layoutPtr		= NULL;
		if (unicodeTextPtr)	*unicodeTextPtr	= NULL;
		if (unicodeLenPtr)	*unicodeLenPtr	= 0;
	}
}

void FskCocoaTextFitWidth(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytesOut, UInt32 *fitCharsOut, FskTextFormatCache cache)
{
	FskErr				err = kFskErrNone;
	OSStatus			status = noErr;
	ATSUStyle			style;
	Boolean				disposeStyle = false;
	UInt16 				*unicodeText = NULL;
	UInt32 				unicodeTextLength = 0;
	UniCharCount		runLengths;
	ATSUTextLayout		textLayout = NULL;
	ATSUTextMeasurement lineWidth;
	UniCharArrayOffset 	lineBreak = 0;
	UInt32				bytesOut;
	Boolean				isLeading;
	UniCharArrayOffset	secondaryOffset;

	if (text && (textLength > 0))
	{
		if (cache && cache->style)
			style = cache->style;
		else
		{
			style = FskCocoaTextCreateATSUIStyle(fontName, textSize, textStyle);
			if (style == NULL) goto bail;
			disposeStyle = true;
		}

		err = FskTextUTF8ToUnicode16NE(text, textLength, &unicodeText, &unicodeTextLength);
		BAIL_IF_ERR(err);

		runLengths = kATSUToTextEnd;
		status = ATSUCreateTextLayoutWithTextPtr(unicodeText, kATSUFromTextBeginning, kATSUToTextEnd, unicodeTextLength / 2, 1, &runLengths, &style, &textLayout);
		if (status != noErr) goto bail;

		lineWidth = X2Fix(width);

		if (kFskTextFitFlagBreak & flags)
		{
			status = ATSUBreakLine(textLayout, kATSUFromTextBeginning, lineWidth, false, &lineBreak);
			if (status && (kATSULineBreakInWord != status)) goto bail;
		}
		else
			(void)ATSUPositionToOffset(textLayout, lineWidth, 0, &lineBreak, &isLeading, &secondaryOffset);

		if (fitCharsOut)
			*fitCharsOut = lineBreak;

		if (fitBytesOut)
		{
			bytesOut = 0;

			while (lineBreak--)
			{
				SInt32 advance = FskTextUTF8Advance(text, 0, 1);

				if (advance == 0)
					break;

				bytesOut += advance;
				text += advance;
			}

			*fitBytesOut = bytesOut;
		}
	}
	else
	{
		if (fitBytesOut)
			*fitBytesOut = 0;

		if (fitCharsOut)
			*fitCharsOut = 0;
	}

bail:
	if (textLayout)
		ATSUDisposeTextLayout(textLayout);

	if (disposeStyle)
		ATSUDisposeStyle(style);

	FskMemPtrDispose(unicodeText);
}

void FskCocoaTextGetFontInfo(void *state, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache cache)
{
	OSStatus			status = noErr;
	ATSUStyle			style;
	Boolean				disposeStyle = false;
	ATSUTextMeasurement	textMeasurement;
	UInt32				ascent = 0, descent = 0, leading = 0;
	ByteCount			actualValueSize;

	if (cache && cache->style)
		style = cache->style;
	else
	{
		style = FskCocoaTextCreateATSUIStyle(fontName, textSize, textStyle);
		disposeStyle = true;
	}

	textMeasurement = 0;
	status = ATSUGetAttribute(style, kATSUAscentTag, sizeof(ATSUTextMeasurement), &textMeasurement, &actualValueSize);

	if ((status == noErr) || (status == kATSUNotSetErr))
		ascent = (UInt32)Fix2X(textMeasurement);

	textMeasurement = 0;
	status = ATSUGetAttribute(style, kATSUDescentTag, sizeof(ATSUTextMeasurement), &textMeasurement, &actualValueSize);

	if ((status == noErr) || (status == kATSUNotSetErr))
		descent = (UInt32)Fix2X(textMeasurement);

	textMeasurement = 0;
	status = ATSUGetAttribute(style, kATSULeadingTag, sizeof(ATSUTextMeasurement), &textMeasurement, &actualValueSize);

	if ((status == noErr) || (status == kATSUNotSetErr))
		leading = (UInt32)Fix2X(textMeasurement);

bail:
	if (disposeStyle)
		ATSUDisposeStyle(style);

	info->ascent = ascent;
	info->descent = descent;
	info->leading = leading;
	info->width = ascent + descent + leading;	/* until we figure out how */
	info->height = ascent + descent + leading;
}

Boolean FskCocoaTextFormatCacheNew(void *state, FskTextFormatCache *cache, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName)
{
	Boolean success = false;
	FskErr	err = kFskErrNone;

	err = FskMemPtrNewClear(sizeof(FskTextFormatCacheRecord), (FskMemPtr *)cache);
	BAIL_IF_ERR(err);

	(*cache)->style = FskCocoaTextCreateATSUIStyle(fontName, textSize, textStyle);
	success = true;

bail:
	return success;
}

void FskCocoaTextFormatCacheDispose(void *state, FskTextFormatCache cache)
{
	if (cache)
	{
		if (cache->style)
			ATSUDisposeStyle(cache->style);

		FskMemPtrDispose(cache);
	}
}

Boolean FskCocoaTextDraw(void *state, FskBitmap fskBitmap, const char *text, UInt32 textLength, FskConstRectangle fskRect, FskConstRectangle clipFskRect, FskConstColorRGBA fskColorRGB, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 horizontalAlignment, UInt16 verticalAlignment, const char *fontName, FskTextFormatCache cache)
{
	Boolean 				success = false;
	OSStatus				status = noErr;
	FskErr					err = kFskErrNone;
	CGRect					drawRect;
	CGPoint					drawPoint;
	UInt16 					*unicodeText = NULL;
	UInt32 					unicodeTextLength = 0;
	UniCharCount			runLengths;
	ATSUTextLayout			textLayout = NULL;
	ATSUStyle				style = NULL;
	Boolean					disposeStyle = false;
	RGBColor				rgbColor = {0};
	ATSUTextMeasurement 	lineWidth;
	ATSULineTruncation		lineTruncation;
	Fract					lineFlushFactor;
	ByteCount				actualValueSize;
	ATSUTextMeasurement 	textMeasurement = 0;
	double					ascent = 0, descent = 0;
	CGContextRef			context;
	ATSUAttributeTag		layoutTags[kATSUILayoutTagCount], styleTag;
	ByteCount				layoutSizes[kATSUILayoutTagCount], styleSize;
	ATSUAttributeValuePtr	layoutValues[kATSUILayoutTagCount], styleValue;

	if ((fskBitmap == NULL) || (text == NULL) || (textLength == 0) || (fskRect == NULL)) goto bail;
	context = (CGContextRef)(fskBitmap->cgBitmapContext);
	drawRect = CGRectMake(fskRect->x, fskBitmap->bounds.height - fskRect->y - fskRect->height, fskRect->width, fskRect->height);
	drawPoint = drawRect.origin;

	// set up context
	CGContextSaveGState(context);
	CGContextSetShouldAntialias(context, true);

	// style
	if (cache && cache->style)
		style = cache->style;
	else
	{
		style = FskCocoaTextCreateATSUIStyle(fontName, textSize, textStyle);
		if (style == NULL) goto bail;
		disposeStyle = true;
	}

	// colour
	if (fskColorRGB)
	{
		rgbColor.red = (unsigned short)fskColorRGB->r * (65535 / 255);
		rgbColor.green = (unsigned short)fskColorRGB->g * (65535 / 255);
		rgbColor.blue = (unsigned short)fskColorRGB->b * (65535 / 255);
	}

	styleTag = kATSUColorTag;
	styleSize = sizeof(rgbColor);
	styleValue = &rgbColor;

	status = ATSUSetAttributes(style, 1, &styleTag, &styleSize, &styleValue);
	if (status != noErr) goto bail;

	// layout
	err = FskTextUTF8ToUnicode16NE(text, textLength, &unicodeText, &unicodeTextLength);
	BAIL_IF_ERR(err);

	runLengths = kATSUToTextEnd;
	status = ATSUCreateTextLayoutWithTextPtr(unicodeText, kATSUFromTextBeginning, kATSUToTextEnd, unicodeTextLength / 2, 1, &runLengths, &style, &textLayout);
	if (status != noErr) goto bail;

	// line truncation
	if (textStyle & kFskTextTruncateCenter)
		lineTruncation = kATSUTruncateMiddle | kATSUTruncFeatNoSquishing;
	else if (textStyle & kFskTextTruncateEnd)
		lineTruncation = kATSUTruncateEnd | kATSUTruncFeatNoSquishing;
	else
		lineTruncation = kATSUTruncateNone;

	// line width
	if (lineTruncation == kATSUTruncateNone)
	{
		if (horizontalAlignment == kFskTextAlignCenter)
			drawPoint.x -= (0x7fff - drawRect.size.width) / 2;
		else if (horizontalAlignment == kFskTextAlignRight)
			drawPoint.x -= 0x7fff - drawRect.size.width;

		lineWidth = X2Fix(0x7fff);
	}
	else
		lineWidth = X2Fix(drawRect.size.width);

	// line flush factor
	switch (horizontalAlignment)
	{
		case kFskTextAlignLeft:
			lineFlushFactor = kATSUStartAlignment;
			break;
		case kFskTextAlignCenter:
			lineFlushFactor = kATSUCenterAlignment;
			break;
		case kFskTextAlignRight:
			lineFlushFactor = kATSUEndAlignment;
			break;
		default:
			goto bail;
			break;
	}

	layoutTags[0] = kATSUCGContextTag;
	layoutSizes[0] = sizeof(context);
	layoutValues[0] = &context;

	layoutTags[1] = kATSULineWidthTag;
	layoutSizes[1] = sizeof(lineWidth);
	layoutValues[1] = &lineWidth;

	layoutTags[2] = kATSULineTruncationTag;
	layoutSizes[2] = sizeof(lineTruncation);
	layoutValues[2] = &lineTruncation;

	layoutTags[3] = kATSULineFlushFactorTag;
	layoutSizes[3] = sizeof(lineFlushFactor);
	layoutValues[3] = &lineFlushFactor;

	status = ATSUSetLayoutControls(textLayout, kATSUILayoutTagCount, layoutTags, layoutSizes, layoutValues);
	if (status != noErr) goto bail;

	// vertical alignment
	ATSUGetAttribute(style, kATSUAscentTag, sizeof(ATSUTextMeasurement), &textMeasurement, &actualValueSize);
	if ((status != noErr) && (status != kATSUNotSetErr)) goto bail;

	if (textMeasurement)
		ascent = Fix2X(textMeasurement);

	ATSUGetAttribute(style, kATSUDescentTag, sizeof(ATSUTextMeasurement), &textMeasurement, &actualValueSize);
	if ((status != noErr) && (status != kATSUNotSetErr)) goto bail;

	if (textMeasurement)
		descent = Fix2X(textMeasurement);

	switch (verticalAlignment)
	{
		case kFskTextAlignCenter:
			drawPoint.y += ((drawRect.size.height - ascent - descent) / 2) + descent;
			break;
		case kFskTextAlignTop:
			drawPoint.y += drawRect.size.height - ascent;
			break;
		case kFskTextAlignBottom:
			drawPoint.y += descent;
			break;
		default:
			goto bail;
			break;
	}

	// blend level
	CGContextSetAlpha(context, blendLevel / 255.0);

	// clip
	CGContextClipToRect(context, drawRect);

	if (clipFskRect)
		CGContextClipToRect(context, CGRectMake(clipFskRect->x, fskBitmap->bounds.height - clipFskRect->y - clipFskRect->height, clipFskRect->width, clipFskRect->height));

	// draw
	status = ATSUDrawText(textLayout, kATSUFromTextBeginning, kATSUToTextEnd, X2Fix(drawPoint.x), X2Fix(drawPoint.y));
	if (status != noErr) goto bail;

	// restore context
	CGContextRestoreGState(context);

	success = true;

bail:
	if (textLayout)
		ATSUDisposeTextLayout(textLayout);

	if (disposeStyle)
		ATSUDisposeStyle(style);

	FskMemPtrDispose(unicodeText);

	return success;
}
#endif	/* !USE_CORE_TEXT */

#pragma mark --- file ---
void FskCocoaFileChoose(const FskFileChooseEntry types, const char *prompt, Boolean allowMultiple, const char *initialPath, char **files)
{
	NSOpenPanel 	*openPanel;
	NSString		*initialPathString = nil;
	NSURL		*url;
	NSEnumerator	*enumerator;
	NSMutableArray	*typesArray = nil;
	char            *fileList = NULL;
	const char      *type, *file;
	UInt32			fileLength, fileListLength = 0;

	if (files == NULL) return;
	openPanel = [NSOpenPanel openPanel];

	if (initialPath)
		initialPathString = [NSString stringWithUTF8String:initialPath];

	if (types)
	{
		typesArray = [NSMutableArray array];

		for (type = types->extension; *type; type += strlen(type) + 1)
			[typesArray addObject:[NSString stringWithUTF8String:type]];
	}

	// set up the open panel
	[openPanel setAllowsMultipleSelection:allowMultiple];
	[openPanel setCanChooseFiles:YES];
	[openPanel setCanChooseDirectories:NO];

	if (prompt)
		[openPanel setPrompt:[NSString stringWithUTF8String:prompt]];

	// run the open panel
	[openPanel setDirectoryURL:[NSURL fileURLWithPath:initialPathString isDirectory:YES]];
	[openPanel setAllowedFileTypes:typesArray];
	if ([openPanel runModal] == NSOKButton)
	{
		for (enumerator = [[openPanel URLs] objectEnumerator]; url = [enumerator nextObject];)
		{
			file = [[url path] UTF8String];
			fileLength = strlen(file);

			if (FskMemPtrRealloc(fileListLength + fileLength + 2, &fileList) != kFskErrNone)
				break;

			FskMemMove(fileList + fileListLength, file, fileLength + 1);
			fileListLength += fileLength + 1;
			fileList[fileListLength] = 0;
		}
	}

	*files = fileList;
}

void FskCocoaFileSaveChoose(const char *defaultName, const char *prompt, const char *initialDirectory, char **file)
{
	NSSavePanel *savePanel;
	NSString	*defaultFileName = nil, *directory = nil;
    const char  *fileString;
	UInt32		fileStringLength;
	char        *fileCopy = NULL;

	if (file == NULL) return;
	savePanel = [NSSavePanel savePanel];

	if (defaultName)
		defaultFileName = [NSString stringWithUTF8String:defaultName];

	if (initialDirectory)
		directory = [NSString stringWithUTF8String:initialDirectory];

	// set up the save panel
	if (prompt)
		[savePanel setPrompt:[NSString stringWithUTF8String:prompt]];

	// run the save panel
	[savePanel setDirectoryURL:[NSURL fileURLWithPath:directory isDirectory:YES]];
	[savePanel setNameFieldStringValue: defaultFileName];
	if ([savePanel runModal] == NSOKButton)
	{
		fileString = [[[savePanel URL] path] UTF8String];
		fileStringLength = strlen(fileString);

		if (FskMemPtrNew(fileStringLength + 1, &fileCopy) != kFskErrNone)
			goto bail;

		FskMemMove(fileCopy, fileString, fileStringLength + 1);
	}

bail:
	*file = fileCopy;
}

void FskCocoaFileDirectoryChoose(const char *prompt, const char *initialPath, char **directory)
{
	NSOpenPanel *openPanel;
	NSArray		*urls;
	const char	*dir;
	UInt32		dirLength;
	char        *directoryCopy = NULL;

	if (directory == NULL) return;
	openPanel = [NSOpenPanel openPanel];

	// set up the open panel
	[openPanel setAllowsMultipleSelection:NO];
	[openPanel setCanChooseFiles:NO];
	[openPanel setCanChooseDirectories:YES];

	if (prompt)
		[openPanel setPrompt:[NSString stringWithUTF8String:prompt]];

	// run the open panel
	if ([openPanel runModal] == NSOKButton)
	{
		urls = [openPanel URLs];

		if ([urls count] == 0)
			goto bail;

		dir = [[[urls objectAtIndex:0] path] UTF8String];
		dirLength = strlen(dir);

		if (FskMemPtrNew(dirLength + 2, &directoryCopy) != kFskErrNone)
			goto bail;

		FskMemMove(directoryCopy, dir, dirLength);
		directoryCopy[dirLength] = '/';
		directoryCopy[dirLength + 1] = 0;
	}

bail:
	*directory = directoryCopy;
}

#pragma mark --- audio ---
#if !USE_AUDIO_QUEUE
Boolean FskCocoaAudioInitialize(FskAudioOut fskAudioOut)
{
	Boolean						success = false;
	ComponentResult 			err = noErr;
	AudioUnit 					outputAudioUnit = NULL;
    AudioConverterRef			audioConverter = NULL;
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
	err = OpenADefaultComponent(kAudioUnitType_Output, kAudioUnitSubType_DefaultOutput, &outputAudioUnit);
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
	err = AudioConverterNew(&inputAudioStreamBasicDescription, &outputAudioStreamBasicDescription, &audioConverter);
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
			CloseComponent(outputAudioUnit);
			outputAudioUnit = NULL;
		}

		if (audioConverter)
		{
			AudioConverterDispose(audioConverter);
			audioConverter = NULL;
		}
	}

	fskAudioOut->outputAudioUnit = outputAudioUnit;
	fskAudioOut->audioConverter = audioConverter;

	return success;
}

void FskCocoaAudioTerminate(FskAudioOut fskAudioOut)
{
	AudioUnit 			outputAudioUnit;
    AudioConverterRef	audioConverter;

	if (fskAudioOut == NULL) return;
	outputAudioUnit = fskAudioOut->outputAudioUnit;
	audioConverter = fskAudioOut->audioConverter;

	// terminate
	if (outputAudioUnit)
	{
		AudioOutputUnitStop(outputAudioUnit);
		AudioUnitUninitialize(outputAudioUnit);
		CloseComponent(outputAudioUnit);
		fskAudioOut->outputAudioUnit = NULL;
	}

	if (audioConverter)
	{
		AudioConverterDispose(audioConverter);
		fskAudioOut->audioConverter = NULL;
	}
}

void FskCocoaAudioGetVolume(FskAudioOut fskAudioOut, UInt16 *leftVolume, UInt16 *rightVolume)
{
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
	Boolean			success = false;
	ComponentResult	err = noErr;
	AudioUnit 		outputAudioUnit;

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
	AudioUnit 			outputAudioUnit;
    AudioConverterRef	audioConverter;

	if (fskAudioOut == NULL) return;
	outputAudioUnit = fskAudioOut->outputAudioUnit;
	audioConverter = fskAudioOut->audioConverter;
	if ((outputAudioUnit == NULL) || (audioConverter == NULL)) return;

	// stop
	fskAudioOut->currentHostTime = AudioGetCurrentHostTime();
	AudioOutputUnitStop(outputAudioUnit);
	AudioConverterReset(audioConverter);
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
#endif /* !USE_AUDIO_QUEUE */

#pragma mark --- clipboard ---
void FskCocoaClipboardTextGet(char **text)
{
	NSString		*string;
	NSMutableString	*mutableString;
	const char		*utf8String;
	UInt32			utf8StringLength;
	char            *fskMemPtr = NULL;

	if (text == NULL) return;

	// get the text from the clipboard
	string = [[NSPasteboard generalPasteboard] stringForType:NSStringPboardType];

	if ([string length] > 0)
	{
		mutableString = [NSMutableString stringWithString:string];
		[mutableString replaceOccurrencesOfString:@"\n" withString:@"\r" options:0 range:NSMakeRange(0, [mutableString length])];
		utf8String = [mutableString UTF8String];
		utf8StringLength = strlen(utf8String);

		if (FskMemPtrNew(utf8StringLength + 1, &fskMemPtr) == kFskErrNone)
			FskMemMove(fskMemPtr, utf8String, utf8StringLength + 1);
	}

	*text = fskMemPtr;
}

void FskCocoaClipboardTextSet(char *text)
{
	NSMutableString	*mutableString;
	NSPasteboard	*pasteboard;

	if (text == NULL) return;
	pasteboard = [NSPasteboard generalPasteboard];

	// set the text from the clipboard
	mutableString = [NSMutableString stringWithUTF8String:text];

	if (mutableString)
	{
		[mutableString replaceOccurrencesOfString:@"\r" withString:@"\n" options:0 range:NSMakeRange(0, [mutableString length])];
		[pasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
		[pasteboard setString:mutableString forType:NSStringPboardType];
	}
}

void FskCocoaClipboardTextClear()
{
	[[NSPasteboard generalPasteboard] declareTypes:[NSArray array] owner:nil];
}

void FskCocoaClipboardTextTypes(char **typesListOut)
{
	char			*typesList = NULL;
	Boolean			sawText = false, sawUnknown = false;
	NSEnumerator	*enumerator;
	NSString		*pasteboardType;
	char			*type;
	UInt32			typesListLength = 0, typeLength;

	for (enumerator = [[[NSPasteboard generalPasteboard] types] objectEnumerator]; pasteboardType = [enumerator nextObject];)
	{
		type = NULL;

		if ([pasteboardType isEqualToString:NSStringPboardType] && !sawText)
		{
			type = "text/plain";
			sawText = true;
		}
		else if (!sawUnknown)
		{
			type = "unknown";
			sawUnknown = true;
		}

		if (type)
		{
			typeLength = strlen(type);

			if (FskMemPtrRealloc(typesListLength +typeLength + 2, &typesList) != kFskErrNone)
				break;

			FskMemMove(typesList + typesListLength, type, typeLength + 1);
			typesListLength += typeLength + 1;
			typesList[typesListLength] = 0;
		}
	}

	*typesListOut = typesList;
}

#pragma mark --- timer ---
void FskCocoaTimerReschedule(FskThread thread)
{
	FskTimeRecord			nextTime, nowTime;
	CFTimeInterval			interval;

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
			CFRunLoopTimerContext	context = {0};

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

	return success;
}

Boolean FskCocoaWordConvertRTF(char *sourcePath, char *destinationPath, char *format)
{
	Boolean 			success = false;
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

	return success;
}

Boolean FskCocoaHTMLConvertRTF(char *sourcePath, char *destinationPath, char *format)
{
	Boolean 			success = false;
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

	return success;
}

#pragma mark --- open ---
Boolean FskCocoaOpenURL(const char *url)
{
	return [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]];
}

Boolean FskCocoaOpenFile(const char *filePath)
{
	return [[NSWorkspace sharedWorkspace] openFile:[NSString stringWithUTF8String:filePath]];
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


#pragma mark --- private ---

#if !USE_CORE_TEXT
ATSUStyle FskCocoaTextCreateATSUIStyle(const char *fontName, UInt32 textSize, UInt32 textStyle)
{
	OSStatus				status = noErr;
	ATSUStyle				style = NULL;
	CFStringRef				*fontCFString = NULL;
	ATSFontRef				atsFontRef = kATSFontRefUnspecified;
	ATSUFontID				fontID = kInvalidFont;
	Fixed					fontSize;
	Boolean					boldface, italic, underline, strikeThrough;
	ATSUAttributeTag		styleTags[kATSUIStyleTagCount];
	ByteCount				styleSizes[kATSUIStyleTagCount];
	ATSUAttributeValuePtr	styleValues[kATSUIStyleTagCount];

	if (fontName)
		fontCFString = CFStringCreateWithBytes(kCFAllocatorDefault, fontName, strlen(fontName), kCFStringEncodingUTF8, false);

	if (fontCFString)
	{
		atsFontRef = ATSFontFindFromName(fontCFString, kATSOptionFlagsDefault);

		if (atsFontRef != kATSFontRefUnspecified)
			fontID = FMGetFontFromATSFontRef(atsFontRef);

		CFRelease(fontCFString);
	}

	if (fontID == kInvalidFont)
		fontID = smSystemScript;

	fontSize = Long2Fix(textSize);

	boldface = ((textStyle & kFskTextBold) != 0);
	italic = ((textStyle & kFskTextItalic) != 0);
	underline = ((textStyle & kFskTextUnderline) != 0);
	strikeThrough = ((textStyle & kFskTextStrike) != 0);

	styleTags[0] = kATSUFontTag;
	styleSizes[0] = sizeof(fontID);
	styleValues[0] = &fontID;

	styleTags[1] = kATSUSizeTag;
	styleSizes[1] = sizeof(fontSize);
	styleValues[1] = &fontSize;

	styleTags[2] = kATSUQDBoldfaceTag;
	styleSizes[2] = sizeof(boldface);
	styleValues[2] = &boldface;

	styleTags[3] = kATSUQDItalicTag;
	styleSizes[3] = sizeof(italic);
	styleValues[3] = &italic;

	styleTags[4] = kATSUQDUnderlineTag;
	styleSizes[4] = sizeof(underline);
	styleValues[4] = &underline;

	styleTags[5] = kATSUStyleStrikeThroughTag;
	styleSizes[5] = sizeof(strikeThrough);
	styleValues[5] = &strikeThrough;

	status = ATSUCreateStyle(&style);
	if (status != noErr) goto bail;

	status = ATSUSetAttributes(style, kATSUIStyleTagCount, styleTags, styleSizes, styleValues);
	if (status != noErr) goto bail;

bail:
	return style;
}

OSStatus ATSFontApply(ATSFontRef fontRef, void *refCon)
{
	OSStatus				status = noErr;
	FskTextFontListParam	fontListParam;
	CFStringRef 			fontNameCFString = NULL;
	char					fontName[1024];
	UInt32					fontNameLength;

	fontListParam = refCon;

	if (fontRef)
	{
		status = ATSFontGetName(fontRef, kATSOptionFlagsDefault, &fontNameCFString);
		if (status) goto bail;

		if (CFStringGetCString(fontNameCFString, fontName, sizeof(fontName), kCFStringEncodingUTF8))
		{
			fontNameLength = strlen(fontName);

			if (FskMemPtrRealloc(fontListParam->fontListLength + fontNameLength + 2, fontListParam->fontList) != kFskErrNone)
				goto bail;

			FskMemMove(*fontListParam->fontList + fontListParam->fontListLength, fontName, fontNameLength + 1);
			fontListParam->fontListLength += fontNameLength + 1;
			(*fontListParam->fontList)[fontListParam->fontListLength] = 0;
		}
	}
	else
	{
		status = paramErr;
		goto bail;
	}

bail:
	if (fontNameCFString)
		CFRelease(fontNameCFString);

    return status;
}
#endif	/* !USE_CORE_TEXT */

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

//
// vouume notifier
//
#import "FskFiles.h"

@interface FskCocoaVolumeNotifier : NSObject {
	FskCocoaVolumeNotifierCallback _callback;
	void *_refCon;
}
- (id) initWithCallback: (FskCocoaVolumeNotifierCallback)callback refCon: (void *)refCon;
- (void) didMount: (NSNotification *) notification;
- (void) didUnmount: (NSNotification *) notification;
@end

@implementation FskCocoaVolumeNotifier
- (id) initWithCallback: (FskCocoaVolumeNotifierCallback)callback refCon: (void *)refCon
{
	NSWorkspace *sws = [NSWorkspace sharedWorkspace];
	NSNotificationCenter *notification = [sws notificationCenter];

	self = [super self];
	_callback = callback;
	_refCon = refCon;
	[notification addObserver:self selector: @selector(didMount:) name:NSWorkspaceDidMountNotification object: nil];
	[notification addObserver:self selector: @selector(didUnmount:) name:NSWorkspaceDidUnmountNotification object:nil];
	return self;
}

- (void) dealloc
{
	[[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver: self];
	[super dealloc];
}

- (void) didMount: (NSNotification *) notification
{
	NSString *path = [[notification userInfo] objectForKey : @"NSDevicePath"];
	_callback(kFskVolumeHello, [path UTF8String], _refCon);
}

- (void) didUnmount: (NSNotification *) notification
{
	NSString *path = [[notification userInfo] objectForKey : @"NSDevicePath"];
	_callback(kFskVolumeBye, [path UTF8String], _refCon);
}
@end

void *FskCocoaVolumeNotifierNew(FskCocoaVolumeNotifierCallback callback, void *refCon)
{
	FskCocoaVolumeNotifier *notifier = [[FskCocoaVolumeNotifier alloc] initWithCallback: callback refCon: refCon];
	return notifier;
}

void FskCocoaVolumeNotifierDispose(void *obj)
{
	FskCocoaVolumeNotifier *notifier = obj;

	[notifier dealloc];
}

//
// system
//
const char *FskCocoaDeviceName()
{
	return [[[NSHost currentHost] localizedName] UTF8String];
}

const char *FskCocoaUserName()
{
	return [NSUserName() UTF8String];
}

const char *FskCocoaSSID()
{
	CWInterface *currentInterface = [CWInterface interface];
	if (currentInterface) 
		return [currentInterface.ssid UTF8String];
	return NULL;
}

void FskCocoaEjectAtPath(const char *path)
{
	[[NSWorkspace sharedWorkspace] unmountAndEjectDeviceAtPath:[NSString stringWithUTF8String:path]];
}

Boolean FskCocoaIsRemovable(const char *path)
{
	BOOL isRemovable = NO;

	[[NSWorkspace sharedWorkspace] getFileSystemInfoForPath:[NSString stringWithUTF8String:path]
				       isRemovable:&isRemovable
				       isWritable:NULL
				       isUnmountable:NULL
				       description:NULL
				       type:NULL];
	return (Boolean)isRemovable;
}

void FskCocoaSystemGetVersion(char *version)
{
	FskStrCopy(version, [[[NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"] objectForKey:@"ProductVersion"] UTF8String]);
}

