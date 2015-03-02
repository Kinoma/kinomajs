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

#define kFskCocoaCopyBitsUseOpenGL	1

#if kFskCocoaCopyBitsUseOpenGL
@interface FskCocoaView : NSOpenGLView
#else
@interface FskCocoaView : NSView
#endif
{
	FskWindow			_fskWindow;
	UInt32 				_keyModifiers;
    NSTrackingRectTag	_trackingRectTag;
    BOOL				_windowClipIsValid;
	CGMutablePathRef	_windowClipCGPath;
	BOOL				_isDrawing;
	
	NSTouch				**touches;
	UInt32 				touchCount;
	NSPoint 			touchLocation;
	NSPoint 			touchPosition;
}

	// init
- (id)initWithFrame:(NSRect)frame;

	// getters/setters
- (FskWindow)fskWindow;
- (void)setFskWindow:(FskWindow)fskWindow;
- (void)setWindowClipCGPath:(CGMutablePathRef)windowClipCGPath;

	// notifications
- (void)boundsDidChange:(NSNotification *)notification;

	// NSView overrides
- (void)drawRect:(NSRect)rect;
- (BOOL)acceptsFirstResponder;
- (BOOL)canBecomeKeyView;
- (void)mouseDown:(NSEvent *)event;
- (void)mouseDragged:(NSEvent *)event;
- (void)mouseUp:(NSEvent *)event;
- (void)mouseMoved:(NSEvent *)event;
- (void)mouseEntered:(NSEvent *)event;
- (void)mouseExited:(NSEvent *)event;
- (void)rightMouseDown:(NSEvent *)event;
- (void)rightMouseDragged:(NSEvent *)event;
- (void)rightMouseUp:(NSEvent *)event;
- (void)scrollWheel:(NSEvent *)event;
- (void)keyDown:(NSEvent *)event;
- (void)keyUp:(NSEvent *)event;
- (void)flagsChanged:(NSEvent *)event;
- (void)displayIfNeeded;
- (BOOL)isOpaque;

	// methods
- (void)copyBits:(FskBitmap)fskBitmap sourceRect:(FskRectangle)sourceRect destRect:(FskRectangle)destRect;
- (void)handleMouseEvent:(NSEvent *)event;
- (void)handleKeyEvent:(NSEvent *)event;
- (void)addTrackingRect;
- (void)removeTrackingRect;
- (void)updateTrackingRect;
- (void)clipWindow;

- (void)beginDraw;
- (void)endDraw;

@property (nonatomic, assign) NSTouch **touches;
@property (nonatomic, assign) UInt32 touchCount;
@property (nonatomic, assign) NSPoint touchLocation;
@property (nonatomic, assign) NSPoint touchPosition;
@end
