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
#import "FskWindow.h"
#import "FskDragDrop.h"

@class FskCocoaView;

@interface FskCocoaWindow : NSWindow
{
	FskWindow 	_fskWindow;
	void		*_dragDropTargetProc;
	BOOL		_dragDropResult;
	BOOL		_hideCursor;
	NSRect		_defaultFrame;
}

	// init
- (id)initWithContentRect:(NSRect)contentRect styleMask:(unsigned int)styleMask backing:(NSBackingStoreType)backingType defer:(BOOL)defer;

	// getters/setters
- (FskWindow)fskWindow;
- (void)setFskWindow:(FskWindow)fskWindow;
- (void)setDropTargetProc:(void *)dropTargetProc;
- (void)setDragDropTargetProc:(void *)dragDropTargetProc;
- (void)setDragDropResult:(BOOL)dragDropResult;

	// actions
- (IBAction)handleMenuAction:(id)sender;
- (void)hideCursor;

	// notifications
- (void)didBecomeMain:(NSNotification *)notification;
- (void)didResignMain:(NSNotification *)notification;
- (void)didBecomeKey:(NSNotification *)notification;
- (void)windowDidExitFullScreen:(NSNotification *)notification;

	// NSDraggingDestination protocol
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender;
- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender;
- (void)draggingExited:(id <NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender;
- (BOOL)wantsPeriodicDraggingUpdates;

	// NSWindow overrides
- (BOOL)canBecomeMainWindow;
- (BOOL)canBecomeKeyWindow;
- (NSSize)minSize;
- (NSSize)maxSize;
- (void)windowDidResize:(NSNotification *)notification;
- (void)close;
- (void)sendEvent:(NSEvent *)event;
- (void)zoom:(id)sender;
- (void)displayIfNeeded;

	// delegates
- (NSRect)windowWillUseStandardFrame:(NSWindow *)sender defaultFrame:(NSRect)defaultFrame;

	// methods
- (NSRect)constrainFrame:(NSRect)frame;
- (void)handleDragging:(id <NSDraggingInfo>)draggingInfo dragOperation:(UInt32)dragOperation;
- (FskDragDropFile)createDragDropFileList:(NSArray *)files;
- (void)disposeDragDropFileList:(FskDragDropFile)dragDropFileList;

@end
