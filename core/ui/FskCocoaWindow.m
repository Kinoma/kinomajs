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
#define __FSKECMASCRIPT_PRIV__

#import "FskCocoaWindow.h"
#import "FskCocoaView.h"
#import "FskECMAScript.h"
#import "FskEvent.h"
#import "FskEnvironment.h"
#import "FskString.h"

@implementation FskCocoaWindow

#pragma mark --- init ---
- (id)initWithContentRect:(NSRect)contentRect styleMask:(unsigned int)styleMask backing:(NSBackingStoreType)backingType defer:(BOOL)defer
{
	FskCocoaView 			*fskCocoaView;
	NSNotificationCenter	*notificationCenter;

	if (self = [super initWithContentRect:contentRect styleMask:styleMask backing:backingType defer:defer])
	{
		_defaultFrame = contentRect;
		notificationCenter = [NSNotificationCenter defaultCenter];

		// create and set the content view
		fskCocoaView = [[[FskCocoaView alloc] initWithFrame:NSMakeRect(0, 0, contentRect.size.width, contentRect.size.height)] autorelease];
		[self setContentView:fskCocoaView];

		// set up the window
		[self setBackgroundColor:[NSColor clearColor]];
		if (styleMask == NSBorderlessWindowMask) {
        [self setOpaque:NO];
			[self setHasShadow:NO];
		}
		else {
			[self setOpaque:YES];
			[self setHasShadow:YES];
        #if defined(MAC_OS_X_VERSION_10_7) && (MAC_OS_X_VERSION_10_7 <= MAC_OS_X_VERSION_MAX_ALLOWED)
            if (floor(NSAppKitVersionNumber) >= NSAppKitVersionNumber10_7)
                [self setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
        #endif
		}
		[self setAcceptsMouseMovedEvents:YES];
		[self setDelegate:(id<NSWindowDelegate>)self];
		[self setReleasedWhenClosed:NO];
		[self setIgnoresMouseEvents:NO];
		
		const char *value = FskEnvironmentGet("hideCursor");
		_hideCursor = value && (0 == FskStrCompare(value, "true"));
		[self hideCursor];

		// observe main window changed notifications
		[notificationCenter addObserver:self selector:@selector(didBecomeMain:) name:NSWindowDidBecomeMainNotification object:self];
		[notificationCenter addObserver:self selector:@selector(didResignMain:) name:NSWindowDidResignMainNotification object:self];
		[notificationCenter addObserver:self selector:@selector(didBecomeKey:) name:NSWindowDidBecomeKeyNotification object:self];
		
	}

	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[self setFskWindow:nil];
	[self setContentView:nil];
	[super dealloc];
}

#pragma mark --- getters/setters ---
- (FskWindow)fskWindow
{
	return _fskWindow;
}

- (void)setFskWindow:(FskWindow)fskWindow
{
	_fskWindow = fskWindow;
	[(FskCocoaView *)[self contentView] setFskWindow:_fskWindow];
}

- (void)setDragDropTargetProc:(void *)dragDropTargetProc
{
	_dragDropTargetProc = dragDropTargetProc;
}

- (void)setDragDropResult:(BOOL)dragDropResult
{
	_dragDropResult = dragDropResult;
}

- (void)setDropTargetProc:(void *)dropTargetProc
{
}

#pragma mark --- actions ---
- (IBAction)handleMenuAction:(id)sender
{
	FskEvent 	fskEvent;
	int 		tag;

	if (_fskWindow == NULL) return;
	tag = [sender tag];

	// send the menu command event
	if (FskEventNew(&fskEvent, kFskEventMenuCommand, NULL, kFskEventModifierNotSet) == kFskErrNone)
	{
		FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(tag), &tag);
		FskWindowEventQueue(_fskWindow, fskEvent);
	}
	
	[self hideCursor];
}

- (void)hideCursor
{
	if (_hideCursor)
		CGDisplayHideCursor(0);
}

#pragma mark --- notifications ---
- (void)didBecomeMain:(NSNotification *)notification
{
	FskEvent fskEvent;

	if (_fskWindow == NULL) return;

	[self hideCursor];

	// send window activated event
	//if (_fskWindow->isCustomWindow)
	//{
		if (FskEventNew(&fskEvent, kFskEventWindowActivated, NULL, kFskEventModifierNotSet) == kFskErrNone)
		{
			FskWindowEventSend(_fskWindow, fskEvent);
			//FskTimersSetUiActive(true);
		}
	//}
}

- (void)didResignMain:(NSNotification *)notification
{
	FskEvent fskEvent;

	if (_fskWindow == NULL) return;

	// send window deactivated event
	//if (_fskWindow->isCustomWindow)
	//{
		if (FskEventNew(&fskEvent, kFskEventWindowDeactivated, NULL, kFskEventModifierNotSet) == kFskErrNone)
		{
			FskWindowEventSend(_fskWindow, fskEvent);
			//FskTimersSetUiActive(false);
			//FskECMAScriptHibernate();
		}
	//}
}

- (void)didBecomeKey:(NSNotification *)notification
{
	[self hideCursor];
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
	FskEvent fskEvent;
	int tag = -1;
	if (FskEventNew(&fskEvent, kFskEventMenuCommand, NULL, kFskEventModifierNotSet) == kFskErrNone) {
		FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(tag), &tag);
		FskWindowEventQueue(_fskWindow, fskEvent);
	}
}

#pragma mark --- NSDraggingDestination protocol ---
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
	// dragging entered
	[self handleDragging:sender dragOperation:kFskDragDropTargetEnterWindow];

	return (_dragDropResult ? NSDragOperationEvery : NSDragOperationNone);
}

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender
{
	// dragging updated
	[self handleDragging:sender dragOperation:kFskDragDropTargetOverWindow];

	return (_dragDropResult ? NSDragOperationEvery : NSDragOperationNone);
}

- (void)draggingExited:(id <NSDraggingInfo>)sender
{
	// dragging exited
	[self handleDragging:sender dragOperation:kFskDragDropTargetLeaveWindow];

	//return (_dragDropResult ? NSDragOperationEvery : NSDragOperationNone);
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
	// perform drag operation
	if (_dragDropResult)
		[self handleDragging:sender dragOperation:kFskDragDropTargetDropInWindow];

	return _dragDropResult;
}

- (BOOL)wantsPeriodicDraggingUpdates
{
	return NO;
}

#pragma mark --- NSWindow overrides ---
- (BOOL)canBecomeMainWindow
{
	return YES;
}

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

- (NSSize)minSize
{
	NSSize minSize = NSMakeSize(0, 0);

	if (_fskWindow == NULL) goto bail;

	// get the minimum size
	if (_fskWindow->minWidth && _fskWindow->minHeight)
		minSize = NSMakeSize(_fskWindow->minWidth, _fskWindow->minHeight);

bail:
	return minSize;
}

- (NSSize)maxSize
{
	NSSize maxSize = NSMakeSize(0, 0);

	if (_fskWindow == NULL) goto bail;

	// get the maximum size
	if (_fskWindow->maxWidth && _fskWindow->maxHeight)
		maxSize = NSMakeSize(_fskWindow->maxWidth, _fskWindow->maxHeight);

bail:
	return maxSize;
}

- (void)windowDidResize:(NSNotification *)notification
{
	if (_fskWindow == NULL) return;

	FskWindowCocoaSizeChanged(_fskWindow);

	[self hideCursor];
}

- (void)close
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];

	[super close];

	if (_fskWindow == NULL) return;

	// close and dispose
	FskWindowCocoaClose(_fskWindow);
	FskWindowDispose(_fskWindow);
	[self setFskWindow:nil];
}

- (void)sendEvent:(NSEvent *)event
{
	NSEventType	type;
	BOOL		handled = NO;

	if (_fskWindow == NULL) goto bail;
	type = [event type];

	switch (type)
	{
		case NSApplicationDefined:
			switch ([event data1])
			{
				case kEventClassFsk:
					switch ([event data2])
					{
						case kEventCloseWindowEvent:
							FskWindowCocoaClose(_fskWindow);
							handled = YES;
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
		default:
			break;
	}

bail:
	if (!handled)
		[super sendEvent:event];
}

- (void)zoom:(id)sender
{
	if (self.styleMask != NSBorderlessWindowMask) {
        [super zoom:sender];
        return;
    }
	NSRect frame, standardFrame;

	if ([self isMiniaturized]) return;

	frame = [self frame];
	standardFrame = [self windowWillUseStandardFrame:self defaultFrame:NSZeroRect];

	if ((abs(frame.origin.x - standardFrame.origin.x) > 20) ||
		(abs(frame.origin.y - standardFrame.origin.y) > 20) ||
		(abs(frame.size.width - standardFrame.size.width) > 20) ||
		(abs(frame.size.height - standardFrame.size.height) > 20))
	{
		_defaultFrame = frame;
	    [self setFrame:standardFrame display:YES];
	}
	else
	    [self setFrame:_defaultFrame display:YES];
}

- (void)displayIfNeeded
{
	if (self.styleMask != NSBorderlessWindowMask)
		[super displayIfNeeded];
}

#pragma mark --- delegates ---
- (NSRect)windowWillUseStandardFrame:(NSWindow *)sender defaultFrame:(NSRect)defaultFrame
{
#if defined(MAC_OS_X_VERSION_10_7) && (MAC_OS_X_VERSION_10_7 <= MAC_OS_X_VERSION_MAX_ALLOWED)
    if ([sender styleMask] & NSFullScreenWindowMask)
       return defaultFrame;
#endif
	return [[self screen] visibleFrame];
}

#pragma mark --- methods ---
- (NSRect)constrainFrame:(NSRect)frame
{
	NSSize	minSize, maxSize;
	float	newWidth, newHeight;

	minSize = [self minSize];
	maxSize = [self maxSize];

	// constrain the frame size
	newWidth = fmaxf(minSize.width, frame.size.width);
	newHeight = fmaxf(minSize.height, frame.size.height);

	if (!NSEqualSizes(maxSize, NSZeroSize))
	{
		newWidth = fminf(newWidth, maxSize.width);
		newHeight = fminf(newHeight, maxSize.height);
	}

	frame.origin.y += frame.size.height - newHeight;
	frame.size.width = newWidth;
	frame.size.height = newHeight;

	return frame;
}

- (void)handleDragging:(id <NSDraggingInfo>)draggingInfo dragOperation:(UInt32)dragOperation
{
	NSPasteboard	*draggingPasteboard;
	FskDragDropFile fskDragDropList = NULL;
	NSPoint			mouseLocation;

	if (dragOperation == kFskDragDropTargetEnterWindow)
		_dragDropResult = false;

	draggingPasteboard = [draggingInfo draggingPasteboard];

	// get the file list
	if (_dragDropTargetProc && [[draggingPasteboard types] containsObject:NSFilenamesPboardType])
	{
		fskDragDropList = [self createDragDropFileList:[draggingPasteboard propertyListForType:NSFilenamesPboardType]];

		if (fskDragDropList)
		{
			if (dragOperation == kFskDragDropTargetEnterWindow)
				_dragDropResult = true;

			// invoke drag drop target proc
			mouseLocation = [self mouseLocationOutsideOfEventStream];
			((FskDragDropTargetProc)_dragDropTargetProc)(dragOperation, mouseLocation.x, [self frame].size.height - mouseLocation.y, fskDragDropList, _fskWindow);
		}
	}

	if (fskDragDropList)
		[self disposeDragDropFileList:fskDragDropList];
}

- (FskDragDropFile)createDragDropFileList:(NSArray *)files
{
	NSEnumerator	*enumerator;
	NSString		*file;
	BOOL			isDirectory;
	FskDragDropFile fskDragDropList = NULL, fskDragDropFile;
	const char		*fileString;
	UInt32			fileStringLength;
	FskErr			err;

	// create the drag drop file list
	for (enumerator = [files objectEnumerator]; file = [enumerator nextObject];)
	{
		// append '/' to the path for directories
		isDirectory = NO;
		[[NSFileManager defaultManager] fileExistsAtPath:file isDirectory:(&isDirectory)];

		if (isDirectory)
			file = [file stringByAppendingString:@"/"];

		fileString = [file UTF8String];
		fileStringLength = strlen(fileString);

		// add the file to the list
		err = FskMemPtrNewClear(sizeof(FskDragDropFileRecord), (FskMemPtr *)(&fskDragDropFile));
		BAIL_IF_ERR(err);
		err = FskMemPtrNewFromData(fileStringLength + 1, fileString, (FskMemPtr *)(&(fskDragDropFile->fullPathName)));
		if (err)
		{
			FskMemPtrDispose(fskDragDropFile);
			goto bail;
		}

		FskListAppend((FskList *)(&fskDragDropList), fskDragDropFile);
	}

bail:
	return fskDragDropList;
}

- (void)disposeDragDropFileList:(FskDragDropFile)dragDropFileList
{

	// dispose the drag drop file list
	while (dragDropFileList)
	{
        FskDragDropFile fskDragDropFile = dragDropFileList;

		FskListRemove((FskList *)(&dragDropFileList), fskDragDropFile);

		if (fskDragDropFile->fullPathName)
			FskMemPtrDispose(fskDragDropFile->fullPathName);

		FskMemPtrDispose(fskDragDropFile);
	}
}

@end
