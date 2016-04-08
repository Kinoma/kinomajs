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
#define __FSKWINDOW_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKPORT_PRIV__

#define GL_EXT_422_pixels 1
#import "FskCocoaView.h"
#import "FskCocoaWindow.h"
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <Carbon/Carbon.h>	// only for the char code


#if SUPPORT_INSTRUMENTATION
	#define COCOA_DEBUG	1
	#define FskInstrumentedTypeEnabled(type, level)	FskInstrumentedTypeHasListenersForLevel(FskInstrumentedGlobalType(type), level)	/**< Whether instrumentation will print anything. */
	#define FskInstrumentedGlobalType(type)			(&(g##type##TypeInstrumentation))												/**< The data structure created with FskInstrumentedSimpleType(). */
#else /* !SUPPORT_INSTRUMENTATION */
	#define FskInstrumentedTypeEnabled(type, level)	(0)																				/**< Instrumentation will not print anything. */
#endif /* SUPPORT_INSTRUMENTATION */

FskInstrumentedSimpleType(CocoaView, cocoaview);												/**< This declares the types needed for instrumentation. */

#if COCOA_DEBUG
	#define	LOGD(...)	FskCocoaViewPrintfDebug(__VA_ARGS__)									/**< Print debugging logs. */
	#define	LOGI(...)	FskCocoaViewPrintfVerbose(__VA_ARGS__)									/**< Print information logs. */
#endif	/* COCOA_DEBUG */
#define		LOGE(...)	FskCocoaViewPrintfMinimal(__VA_ARGS__)									/**< Print error logs always, when instrumentation is on. */
#ifndef     LOGD
	#define LOGD(...)																			/**< Don't print debugging logs. */
#endif   		/* LOGD */
#ifndef     LOGI
	#define LOGI(...)																			/**< Don't print information logs. */
#endif   		/* LOGI */
#ifndef     LOGE
	#define LOGE(...)																			/**< Don't print error logs. */
#endif   	/* LOGE */
#define LOGD_ENABLED()	FskInstrumentedTypeEnabled(CocoaView, kFskInstrumentationLevelDebug)	/**< Whether LOGD() will print anything. */
#define LOGI_ENABLED()	FskInstrumentedTypeEnabled(CocoaView, kFskInstrumentationLevelVerbose)	/**< Whether LOGI() will print anything. */
#define LOGE_ENABLED()	FskInstrumentedTypeEnabled(CocoaView, kFskInstrumentationLevelMinimal)	/**< Whether LOGE() will print anything. */


@implementation FskCocoaView

#pragma mark --- defines ---
#define kFskCocoaCopyBitsUseDoubleBuffer 0
#define kNumberOfFingers 10

#pragma mark --- constants ---
const float kFskCocoaViewCornerRadius = 8;

#pragma mark --- init ---
- (id)initWithFrame:(NSRect)frame
{
	NSNotificationCenter 			*notificationCenter;
#if kFskCocoaCopyBitsUseOpenGL
	NSOpenGLPixelFormat				*openGLPixelFormat;
	NSOpenGLPixelFormatAttribute	openGLPixelFormatAttributes[] = {NSOpenGLPFADoubleBuffer, NSOpenGLPFAAccelerated, NSOpenGLPFAColorSize, 32, NSOpenGLPFAClosestPolicy, 0};

	openGLPixelFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes:openGLPixelFormatAttributes] autorelease];

	LOGD("[FskCocoaView initWithFrame:{%g,%g,%g,%g} pixelFormat:%p] in thread %s",
		 frame.origin.x, frame.origin.y, frame.size.width, frame.size.height, openGLPixelFormat, FskThreadName(FskThreadGetCurrent()));
	if (self = [super initWithFrame:frame pixelFormat:openGLPixelFormat])
#else /* !kFskCocoaCopyBitsUseOpenGL */
	if (self = [super initWithFrame:frame])
#endif /* !kFskCocoaCopyBitsUseOpenGL */
	{
		notificationCenter = [NSNotificationCenter defaultCenter];

		// observe bounds/frame change notifications
		[notificationCenter addObserver:self selector:@selector(boundsDidChange:) name:NSViewBoundsDidChangeNotification object:self];
		[notificationCenter addObserver:self selector:@selector(boundsDidChange:) name:NSViewFrameDidChangeNotification object:self];

		// add tracking rect
		[self addTrackingRect];

        self.touches = (NSTouch**)malloc(kNumberOfFingers * sizeof(NSTouch*));
        int j;
        for (j = 0; j < kNumberOfFingers; j++)
            touches[j] = nil;
        //self.touchCount = 0;
        [self setAcceptsTouchEvents:YES];

        _windowClipCGPath = NULL;
#if USE_DISPLAY_LINK
		FskMutexNew(&_displayLinkMutex, "DisplayLink");
#endif
	}

#if kFskCocoaCopyBitsUseOpenGL
	const GLint vals = 0x01;
	[[self openGLContext] setValues:&vals forParameter:NSOpenGLCPSwapInterval];	/* enable vertical sychronization to refresh rate */
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glPixelStorei(GL_UNPACK_ALIGNMENT,   1);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,   (GLint)0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, (GLint)0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#if kFskCocoaCopyBitsUseDoubleBuffer
	glDrawBuffer(GL_BACK);
#else  /* !kFskCocoaCopyBitsUseDoubleBuffer */
	glDrawBuffer(GL_FRONT);
#endif /* !kFskCocoaCopyBitsUseDoubleBuffer */
#endif /* !kFskCocoaCopyBitsUseOpenGL */

	return self;
}

- (void)dealloc
{
#if USE_DISPLAY_LINK
	FskMutexDispose(_displayLinkMutex);
#endif
	if (_windowClipCGPath)
		CGPathRelease(_windowClipCGPath);
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[self removeTrackingRect];
	[self setFskWindow:nil];
	[self setWindowClipCGPath:nil];
	free(self.touches);
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
}

- (void)setWindowClipCGPath:(CGMutablePathRef)windowClipCGPath
{
	if (_windowClipCGPath)
		CGPathRelease(_windowClipCGPath);

	_windowClipCGPath = windowClipCGPath;
}

#pragma mark --- notifications ---
- (void)boundsDidChange:(NSNotification *)notification
{
	// update tracking rect
	[self updateTrackingRect];

	// update window clip
	_windowClipIsValid = NO;
}

#pragma mark --- NSView overrides ---
- (void)drawRect:(NSRect)rect
{
#if USE_DISPLAY_LINK
	if ([self isDisplayLinkRunning])
		return;
#endif

	FskRectangleRecord fskRectangle;

	LOGD("[FskCocoaView:%p drawRect:{%g,%g,%g,%g}] in thread:%s, context:%p",
		 self, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height, FskThreadName(FskThreadGetCurrent()), [self openGLContext]);

	if (_fskWindow == NULL) return;
	_isDrawing = YES;

	fskRectangle.x = rect.origin.x;
	fskRectangle.y = [self bounds].size.height - rect.origin.y - rect.size.height;
	fskRectangle.width = rect.size.width;
	fskRectangle.height = rect.size.height;

	// invalidate
	FskPortInvalidateRectangle(_fskWindow->port, &fskRectangle);

	// draw
    FskWindowUpdate(_fskWindow, NULL);

	_isDrawing = NO;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (BOOL)canBecomeKeyView
{
	return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
	return YES;
}

- (void)mouseDown:(NSEvent *)event
{
	[self handleMouseEvent:event];
}

- (void)mouseDragged:(NSEvent *)event
{
	[self handleMouseEvent:event];
}

- (void)mouseUp:(NSEvent *)event
{
	[self handleMouseEvent:event];

	if (_fskWindow)
		[(FskCocoaWindow *)(_fskWindow->nsWindow) hideCursor];
}

- (void)mouseMoved:(NSEvent *)event
{
	[self handleMouseEvent:event];

	[(FskCocoaWindow *)(_fskWindow->nsWindow) hideCursor];
}

- (void)mouseEntered:(NSEvent *)event
{
	[self handleMouseEvent:event];
}

- (void)mouseExited:(NSEvent *)event
{
	FskEvent fskEvent;

	if (_fskWindow == NULL) return;

	// create and queue the mouse exit event
	if (FskEventNew(&fskEvent, kFskEventMouseLeave, NULL, kFskEventModifierNotSet) == kFskErrNone)
	{
		FskWindowEventQueue(_fskWindow, fskEvent);
	}
}

- (void)rightMouseDown:(NSEvent *)event
{
	[self handleMouseEvent:event];
}

- (void)rightMouseDragged:(NSEvent *)event
{
	[self handleMouseEvent:event];
}

- (void)rightMouseUp:(NSEvent *)event
{
	[self handleMouseEvent:event];
}

- (void)scrollWheel:(NSEvent *)event
{
	FskEvent	fskEvent;
	float		deltaX, deltaY;
	int			touched;
	Boolean		precise;

	if (_fskWindow == NULL) return;
	deltaX = [event scrollingDeltaX];
	deltaY = [event scrollingDeltaY];
	precise = [event hasPreciseScrollingDeltas];

	if (precise)
		touched = ([event phase] & ~(NSEventPhaseEnded | NSEventPhaseCancelled)) != 0;
	else {
		deltaX *= 5;
		deltaY *= 5;
		touched = true;
	}
	// create and queue the scroll wheel event
	if (FskEventNew(&fskEvent, kFskEventMouseWheel, NULL, kFskEventModifierNotSet) == kFskErrNone)
	{
		FskEventParameterAdd(fskEvent, kFskEventParameterMouseWheelDeltaX, sizeof(deltaX), &deltaX);
		FskEventParameterAdd(fskEvent, kFskEventParameterMouseWheelDeltaY, sizeof(deltaY), &deltaY);
		FskEventParameterAdd(fskEvent, kFskEventParameterMouseWheelTouched, sizeof(touched), &touched);
		FskWindowEventQueue(_fskWindow, fskEvent);
	}
	if (!precise) {
		deltaX = 0;
		deltaY = 0;
		touched = false;
		if (FskEventNew(&fskEvent, kFskEventMouseWheel, NULL, kFskEventModifierNotSet) == kFskErrNone)
		{
			FskEventParameterAdd(fskEvent, kFskEventParameterMouseWheelDeltaX, sizeof(deltaX), &deltaX);
			FskEventParameterAdd(fskEvent, kFskEventParameterMouseWheelDeltaY, sizeof(deltaY), &deltaY);
			FskEventParameterAdd(fskEvent, kFskEventParameterMouseWheelTouched, sizeof(touched), &touched);
			FskWindowEventQueue(_fskWindow, fskEvent);
		}
	}
}

- (void)keyDown:(NSEvent *)event
{
	[self handleKeyEvent:event];
}

- (void)keyUp:(NSEvent *)event
{
	[self handleKeyEvent:event];
}

- (void)flagsChanged:(NSEvent *)event
{
	unsigned int modifierFlags;

	_keyModifiers = 0;
	modifierFlags = [event modifierFlags];

	// update key modifiers
	if (modifierFlags & NSCommandKeyMask)
		_keyModifiers |= kFskEventModifierControl;

	if (modifierFlags & NSAlternateKeyMask)
		_keyModifiers |= kFskEventModifierAlt;

	if (modifierFlags & NSShiftKeyMask)
		_keyModifiers |= kFskEventModifierShift;

	if (modifierFlags & NSAlphaShiftKeyMask)
		_keyModifiers |= kFskEventModifierCapsLock;
}

- (void)displayIfNeeded
{
	LOGD("[FskCocoaView:%p displayIfNeeded] in thread %s", self, FskThreadName(FskThreadGetCurrent()));
}

- (BOOL)isOpaque
{
	return NO;
}


#ifdef CHECK_FOR_ERRORS
static const char* GLErrorStringFromCode(GLenum code) {
	struct GLErrEntry { GLenum code;	const char *name;	};
	static const struct GLErrEntry glErrTab[] = {
		{	GL_INVALID_ENUM,		"GL_INVALID_ENUM"		},
		{	GL_INVALID_VALUE,		"GL_INVALID_VALUE"		},
		{	GL_INVALID_OPERATION,	"GL_INVALID_OPERATION"	},
		{	GL_STACK_OVERFLOW,		"GL_STACK_OVERFLOW"		},
		{	GL_STACK_UNDERFLOW,		"GL_STACK_UNDERFLOW"	},
		{	GL_OUT_OF_MEMORY,		"GL_OUT_OF_MEMORY"		},
		{	0,						""						}
	};
	const struct GLErrEntry *p = glErrTab;
	for (; p->code != 0; ++p)
		if (code == p->code)
			break;
	return p->name;
};
#endif /* CHECK_FOR_ERRORS */


#pragma mark --- methods ---
- (void)copyBits:(FskBitmap)fskBitmap sourceRect:(FskRectangle)sourceRect destRect:(FskRectangle)destRect
{
#if kFskCocoaCopyBitsUseOpenGL
	FskBitmap		tmp			= NULL;		/* Temporary buffer, in case we need to convert pixel format */
	unsigned char	*bits		= NULL;
	SInt32			pixBytes;
	NSRect			viewBounds;
	GLenum			glFormat, glType;
#ifdef CHECK_FOR_ERRORS
	FskErr			err;
#endif /* CHECK_FOR_ERRORS */

#if kFskCocoaCopyBitsUseDoubleBuffer
	NSOpenGLContext *openGLContext = [self openGLContext];
#endif
	viewBounds = [self bounds];

	[self lockFocus];

	// Set view transform.
	glViewport((GLint)destRect->x, (GLint)((GLint)viewBounds.size.height - destRect->y - destRect->height), (GLsizei)destRect->width, (GLsizei)destRect->height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#if kFskCocoaCopyBitsUseDoubleBuffer
	glDrawBuffer(GL_BACK);
#else
	glDrawBuffer(GL_FRONT);
#endif

	// clip
	glEnable(GL_SCISSOR_TEST);
	glScissor((GLint)destRect->x, (GLint)((GLint)viewBounds.size.height - destRect->y - destRect->height), (GLsizei)destRect->width, (GLsizei)destRect->height);

	// draw
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glPixelZoom(1.0, -1.0);
	glRasterPos2i(-1, 1);

#ifdef CHECK_FOR_ERRORS
	err = glGetError();
	if (err)
		printf("pre-glDrawPixels error: %04x: %s\n", err, GLErrorStringFromCode(err));
#endif /* CHECK_FOR_ERRORS */

	switch (fskBitmap->pixelFormat) {		/* Convert from pixel format to GL format and type */
	#if TARGET_RT_LITTLE_ENDIAN
		case kFskBitmapFormat32ABGR:		glFormat = GL_RGBA;	glType = GL_UNSIGNED_INT_8_8_8_8;		break;	/* untested */
		case kFskBitmapFormat32ARGB:		glFormat = GL_BGRA;	glType = GL_UNSIGNED_INT_8_8_8_8;		break;	/* tested */
		case kFskBitmapFormat32BGRA:		glFormat = GL_BGRA;	glType = GL_UNSIGNED_INT_8_8_8_8_REV;	break;	/* untested */
		case kFskBitmapFormat32RGBA:		glFormat = GL_RGBA;	glType = GL_UNSIGNED_INT_8_8_8_8_REV;	break;	/* untested */
		case kFskBitmapFormat16RGB565LE:	glFormat = GL_RGB;	glType = GL_UNSIGNED_SHORT_5_6_5;		break;	/* untested */
	#else /* TARGET_RT_BIG_ENDIAN */
		case kFskBitmapFormat32ABGR:		glFormat = GL_RGBA;	glType = GL_UNSIGNED_INT_8_8_8_8_REV;	break;	/* untested */
		case kFskBitmapFormat32ARGB:		glFormat = GL_BGRA;	glType = GL_UNSIGNED_INT_8_8_8_8_REV;	break;	/* untested */
		case kFskBitmapFormat32BGRA:		glFormat = GL_BGRA;	glType = GL_UNSIGNED_INT_8_8_8_8;		break;	/* untested */
		case kFskBitmapFormat32RGBA:		glFormat = GL_RGBA;	glType = GL_UNSIGNED_INT_8_8_8_8;		break;	/* untested */
		case kFskBitmapFormat16RGB565BE:	glFormat = GL_RGB;	glType = GL_UNSIGNED_SHORT_5_6_5;		break;	/* untested */
		case kFskBitmapFormat16RGB565LE:	glFormat = GL_RGB;	glType = GL_UNSIGNED_SHORT_5_6_5_REV;	break;	/* untested */
	#endif /* TARGET_RT_BIG_ENDIAN */
		case kFskBitmapFormatYUV420:		glFormat = GL_BGRA;	glType = GL_UNSIGNED_INT_8_8_8_8;
		#ifndef UNITY_CONVERSION_FROM_420
			FskBitmapNew(destRect->width, destRect->height, kFskBitmapFormat32ARGB, &tmp);	/* TODO: Why resize during conversion? */	/* Allocate temporary ARGB */
			FskBitmapDraw(fskBitmap, sourceRect, tmp, &tmp->bounds, NULL, NULL, kFskGraphicsModeCopy | kFskGraphicsModeBilinear, NULL);	/* Convert 420 to ARGB with scaling */
		#else /* UNITY_CONVERSION_FROM_420, letting GL do the scaling. */
			FskBitmapNew(sourceRect->width, sourceRect->height, kFskBitmapFormat32ARGB, &tmp);											/* Allocate temporary ARGB */
			FskBitmapDraw(fskBitmap, sourceRect, tmp, &tmp->bounds, NULL, NULL, kFskGraphicsModeCopy | kFskGraphicsModeBilinear, NULL);	/* Unity conversion from 420 to ARGB */
		#endif /* UNITY_CONVERSION_FROM_420 */
			fskBitmap  = tmp;
			sourceRect = &tmp->bounds;
			break;
		default:
			glFormat = GL_RGBA;	glType = GL_UNSIGNED_INT_8_8_8_8;	/* better than nothing... */
			break;
	}

	pixBytes = fskBitmap->depth >> 3;																			/* Compute pixel bytes. */
	glPixelStorei(GL_UNPACK_ROW_LENGTH,  (GLint)(fskBitmap->rowBytes / pixBytes));								/* Set row pixels, i.e. row stride in pixels. */
	glPixelStorei(GL_UNPACK_ALIGNMENT, (pixBytes == 4) ? 4 : 1);												/* Set alignment. */
	bits = (unsigned char*)(fskBitmap->bits) + sourceRect->y * fskBitmap->rowBytes + sourceRect->x * pixBytes;	/* Offset pixel pointer by (x, y). */
	glDrawPixels((GLsizei)destRect->width, (GLsizei)destRect->height, glFormat, glType, bits);
#ifdef CHECK_FOR_ERRORS
	err = glGetError();
	if (err)
		printf("glDrawPixels error: %04x: %s: format=%04x, type=%04x\n", err, GLErrorStringFromCode(err), glFormat, glType);
#endif /* CHECK_FOR_ERRORS */

#if kFskCocoaCopyBitsUseDoubleBuffer
	[openGLContext flushBuffer];
#else
	glFlush();
#endif

	[self unlockFocus];

	FskBitmapDispose(tmp);
#else
	NSRect				bounds;
    CGContextRef 		cgContext;
    CGImageRef 			image;
    CGRect				bitmapCGRect, sourceCGRect, destCGRect;
    CGContextRef		cgBitmapContext;

	if ((_fskWindow == NULL) || (fskBitmap == NULL) || (sourceRect == NULL) || (destRect == NULL)) return;
	cgContext = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	cgBitmapContext = (CGContextRef)(fskBitmap->cgBitmapContext);

	// lock focus and save state
	[self lockFocus];
	CGContextSaveGState(cgContext);

	bounds = [self bounds];
	bitmapCGRect = CGRectMake(fskBitmap->bounds.x, fskBitmap->bounds.y, fskBitmap->bounds.width, fskBitmap->bounds.height);
	sourceCGRect = CGRectMake(sourceRect->x, bounds.size.height - sourceRect->y - sourceRect->height, sourceRect->width, sourceRect->height);
	destCGRect = CGRectMake(destRect->x, bounds.size.height - destRect->y - destRect->height, destRect->width, destRect->height);

	// clear
	[[NSColor clearColor] set];
	NSRectFill(*(NSRect *)((CGRect *)(&destCGRect)));

	// clip window
	if (_fskWindow->isCustomWindow)
		[self clipWindow];

	// clip
	CGContextClipToRect(cgContext, destCGRect);

	// translate and scale
	if (!CGRectEqualToRect(destCGRect, sourceCGRect))
	{
		CGContextTranslateCTM(cgContext, destCGRect.origin.x - sourceCGRect.origin.x, destCGRect.origin.y - sourceCGRect.origin.y);
		CGContextScaleCTM(cgContext, destCGRect.size.width / sourceCGRect.size.width, destCGRect.size.height / sourceCGRect.size.height);
	}

	// draw the image
	image = CGBitmapContextCreateImage(cgBitmapContext);

	if (image)
	{
		CGContextDrawImage(cgContext, bitmapCGRect, image);
		CGImageRelease(image);

		if (!_isDrawing)
			CGContextFlush(cgContext);
	}

	// restore state and unlock focus
	CGContextRestoreGState(cgContext);
	[self unlockFocus];
#endif
}

- (void)handleMouseEvent:(NSEvent *)event
{
	FskEvent		fskEvent;
	NSEventType		eventType;
	UInt32			eventCode = 0;
	int				clickCount = 0;
	NSPoint			point;
	FskPointAndTicksRecord
					pat;
	NSWindow		*window;
	NSTimeInterval	when;
	FskTimeRecord	mouseTimeR;
	SInt32			scale;
	SInt32			rotate;

	if (_fskWindow == NULL) return;
	if (!FskWindowCursorIsVisible(_fskWindow)) return;

	window = (NSWindow *)(_fskWindow->nsWindow);
	eventType = [event type];

	scale = FskWindowScaleGet(_fskWindow);
	rotate = FskWindowRotateGet(_fskWindow);

	switch (eventType)
	{
		case NSLeftMouseDown:
			clickCount = [event clickCount];
			eventCode = ((clickCount == 2) ? kFskEventMouseDoubleClick : kFskEventMouseDown);
			_keyModifiers |= kFskEventModifierMouseButton;
			break;
		case NSRightMouseDown:
			clickCount = [event clickCount];
			eventCode = kFskEventRightMouseDown;
			_keyModifiers |= kFskEventModifierMouseButton;
			break;
		case NSLeftMouseUp:
			eventCode = kFskEventMouseUp;
			FskWindowCancelStillDownEvents(_fskWindow);
			break;
		case NSRightMouseUp:
			eventCode = kFskEventRightMouseUp;
			FskWindowCancelStillDownEvents(_fskWindow);
			break;
		case NSLeftMouseDragged:
		case NSRightMouseDragged:
		case NSMouseMoved:
		case NSMouseEntered:
			eventCode = kFskEventMouseMoved;
			break;
		default:
			eventCode = kFskEventNone;
			break;
	}

	// calculate the mouse window position
	point = [event locationInWindow];
    if (_fskWindow->usingGL)
        point.y = [window.contentView frame].size.height - point.y;
    else { // @@ does not work with GL?
        point = [self convertPoint:point fromView:nil];
        point.y = [self frame].size.height - point.y;
	}
    pat.pt.x = point.x;
	pat.pt.y = point.y;

	pat.pt.x = FskPortSInt32Unscale(_fskWindow->port, pat.pt.x);
	pat.pt.y = FskPortSInt32Unscale(_fskWindow->port, pat.pt.y);

	pat.pt.x /= scale;
	pat.pt.y /= scale;
	if (0 != rotate) {
		if (90 == rotate) {
			SInt32 temp = pat.pt.x;
			pat.pt.x = pat.pt.y;
			pat.pt.y = (_fskWindow->bits->bounds.height - 1) - temp;
		}
		else if (270 == rotate) {
			SInt32 temp = pat.pt.x;
			pat.pt.x = (_fskWindow->bits->bounds.width - 1) - pat.pt.y;
			pat.pt.y = temp;
		}
		else if (180 == rotate) {
			pat.pt.x = (_fskWindow->bits->bounds.width - 1) - pat.pt.x;
			pat.pt.y = (_fskWindow->bits->bounds.height - 1) - pat.pt.y;
		}
	}

	if (_fskWindow->port)
	{
		pat.pt.x -= _fskWindow->port->origin.x;
		pat.pt.y -= _fskWindow->port->origin.y;
	}

	when = [event timestamp];
	pat.ticks = (UInt32)(when * 1000.0);
	pat.index = 0;
	mouseTimeR.useconds = mouseTimeR.seconds = 0;
	FskTimeAddMS(&mouseTimeR, pat.ticks);

	// create and queue the mouse event
	if (FskEventNew(&fskEvent, eventCode, &mouseTimeR, _keyModifiers) == kFskErrNone)
	{
		FskEventParameterAdd(fskEvent, kFskEventParameterMouseLocation, sizeof(pat), &pat);

		if (clickCount)
			FskEventParameterAdd(fskEvent, kFskEventParameterMouseClickNumber, sizeof(clickCount), &clickCount);

		FskWindowEventQueue(_fskWindow, fskEvent);
	}

	_keyModifiers &= ~kFskEventModifierMouseButton;
}

- (void)handleKeyEvent:(NSEvent *)event
{
	FskEvent	fskEvent;
	UInt32		eventCode;
	NSString	*keysString;
	const char	*keys = NULL;
	UInt32		keysLength = 0;
	unichar		keyChar = 0;
	char		chars[2];
	UInt32		functionKey = 0;

	if (_fskWindow == NULL) return;
	eventCode = (([event type] == NSKeyDown) ? kFskEventKeyDown : kFskEventKeyUp);
	keysString = [event charactersIgnoringModifiers];
	[self flagsChanged:event];
	if ([keysString length] > 0)
	{
		SInt32			rotate = FskWindowRotateGet(_fskWindow);

		keyChar = [keysString characterAtIndex:0];

		// get/remap the characters
		switch (keyChar)
		{
			case NSUpArrowFunctionKey:
			case NSDownArrowFunctionKey:
			case NSLeftArrowFunctionKey:
			case NSRightArrowFunctionKey:
				if (keyChar == NSUpArrowFunctionKey) {
					if (90 == rotate)
						chars[0] = kLeftArrowCharCode;
					else if (270 == rotate)
						chars[0] = kRightArrowCharCode;
					else
						chars[0] = kUpArrowCharCode;
				}
				else if (keyChar == NSDownArrowFunctionKey) {
					if (90 == rotate)
						chars[0] = kRightArrowCharCode;
					else if (270 == rotate)
						chars[0] = kLeftArrowCharCode;
					else
						chars[0] = kDownArrowCharCode;
				}
				else if (keyChar == NSLeftArrowFunctionKey) {
					if (90 == rotate)
						chars[0] = kDownArrowCharCode;
					else if (270 == rotate)
						chars[0] = kUpArrowCharCode;
					else
						chars[0] = kLeftArrowCharCode;
				}
				else if (keyChar == NSRightArrowFunctionKey) {
					if (90 == rotate)
						chars[0] = kUpArrowCharCode;
					else if (270 == rotate)
						chars[0] = kDownArrowCharCode;
					else
						chars[0] = kRightArrowCharCode;
				}

				chars[1] = 0;
				keys = chars;
				keysLength = 2;
				break;
			case kDeleteCharCode:
				chars[0] = kBackspaceCharCode;
				chars[1] = 0;
				keys = chars;
				keysLength = 2;
				break;
			case NSF1FunctionKey:
			case NSF2FunctionKey:
			case NSF3FunctionKey:
			case NSF4FunctionKey:
			case NSF5FunctionKey:
			case NSF6FunctionKey:
			case NSF7FunctionKey:
			case NSF8FunctionKey:
			case NSF9FunctionKey:
			case NSF10FunctionKey:
			case NSF11FunctionKey:
			case NSF12FunctionKey:
			case NSF13FunctionKey:
			case NSF14FunctionKey:
			case NSF15FunctionKey:
			case NSF16FunctionKey:
			case NSF17FunctionKey:
			case NSF18FunctionKey:
			case NSF19FunctionKey:
			case NSF20FunctionKey:
				keys = "";
				keysLength = 1;
				functionKey = keyChar - NSF1FunctionKey + 1;
				break;
			case NSDeleteFunctionKey:
				chars[0] = kDeleteCharCode;
				chars[1] = 0;
				keys = chars;
				keysLength = 2;
				break;
			case NSHomeFunctionKey:
				chars[0] = kHomeCharCode;
				chars[1] = 0;
				keys = chars;
				keysLength = 2;
				break;
			case NSEndFunctionKey:
				chars[0] = kEndCharCode;
				chars[1] = 0;
				keys = chars;
				keysLength = 2;
				break;
			case NSPageUpFunctionKey:
				chars[0] = kPageUpCharCode;
				chars[1] = 0;
				keys = chars;
				keysLength = 2;
				break;
			case NSPageDownFunctionKey:
				chars[0] = kPageDownCharCode;
				chars[1] = 0;
				keys = chars;
				keysLength = 2;
				break;

			default:
				keys = [keysString UTF8String];
				keysLength = strlen(keys) + 1;
				break;
		}
	}

	// create and queue the key event
	if (FskEventNew(&fskEvent, eventCode, NULL, _keyModifiers) == kFskErrNone)
	{
		FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, keysLength, keys);

		if (functionKey)
			FskEventParameterAdd(fskEvent, kFskEventParameterFunctionKey, sizeof(functionKey), &functionKey);

		FskWindowEventQueue(_fskWindow, fskEvent);
	}
}

- (void)addTrackingRect
{
	// add tracking rect
	_trackingRectTag = [self addTrackingRect:[self bounds] owner:self userData:nil assumeInside:NO];
}

- (void)removeTrackingRect
{
	// remove tracking rect
	if (_trackingRectTag)
	{
		[self removeTrackingRect:_trackingRectTag];
		_trackingRectTag = 0;
	}
}

- (void)updateTrackingRect
{
	// update tracking rect
	[self removeTrackingRect];
	[self addTrackingRect];
}

- (void)clipWindow
{
	CGContextRef	cgContext;
	NSRect			bounds;
	CGRect			innerBoundsCGRect;
    BOOL			invalidateShadow = NO;

	cgContext = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];

	if (!_windowClipIsValid)
	{
		// build the window clip path
		bounds = [self bounds];
		innerBoundsCGRect = CGRectInset(*(CGRect *)(&bounds), kFskCocoaViewCornerRadius, kFskCocoaViewCornerRadius);
        CGMutablePathRef path = CGPathCreateMutable();
		[self setWindowClipCGPath:path];
        CFRelease(path);

		// bottom left
		CGPathMoveToPoint(_windowClipCGPath, NULL, bounds.origin.x, bounds.origin.y + kFskCocoaViewCornerRadius);
		CGPathAddArc(_windowClipCGPath, NULL, innerBoundsCGRect.origin.x, innerBoundsCGRect.origin.y, kFskCocoaViewCornerRadius, M_PI, M_PI * 3 / 2, false);

		// bottom edge
		CGPathAddLineToPoint(_windowClipCGPath, NULL, bounds.origin.x + innerBoundsCGRect.origin.x + innerBoundsCGRect.size.width, bounds.origin.y);

		// bottom right
		CGPathAddArc(_windowClipCGPath, NULL, innerBoundsCGRect.origin.x + innerBoundsCGRect.size.width, innerBoundsCGRect.origin.y, kFskCocoaViewCornerRadius, M_PI / 3 * 2, 0, false);

		// right edge
		CGPathAddLineToPoint(_windowClipCGPath, NULL, bounds.origin.x + bounds.size.width, bounds.origin.y + innerBoundsCGRect.origin.y + innerBoundsCGRect.size.height);

		// top right
		CGPathAddArc(_windowClipCGPath, NULL, innerBoundsCGRect.origin.x + innerBoundsCGRect.size.width, innerBoundsCGRect.origin.y + innerBoundsCGRect.size.height, kFskCocoaViewCornerRadius, 0, M_PI / 2, false);

		// top edge
		CGPathAddLineToPoint(_windowClipCGPath, NULL, bounds.origin.x + innerBoundsCGRect.origin.x, bounds.origin.y + bounds.size.height);

		// top left
		CGPathAddArc(_windowClipCGPath, NULL, innerBoundsCGRect.origin.x, innerBoundsCGRect.origin.y + innerBoundsCGRect.size.height, kFskCocoaViewCornerRadius, M_PI / 2, M_PI, false);

		// close path
		CGPathCloseSubpath(_windowClipCGPath);
		_windowClipIsValid = YES;

		invalidateShadow = YES;
	}

	// clip
	CGContextAddPath(cgContext, _windowClipCGPath);
	CGContextClip(cgContext);

	// invalidate the window shadow
	if (invalidateShadow)
		[[self window] invalidateShadow];
}

@synthesize touches;
@synthesize touchCount;
@synthesize touchLocation;
@synthesize touchPosition;

- (void)handleTouch:(NSTouch *)touch at:(UInt32)j with:(NSEvent*)event as:(UInt32)eventCode eventOut:(FskEvent *)eventOut
{
	NSWindow *window = (NSWindow *)(_fskWindow->nsWindow);
	NSSize windowSize = [window.contentView frame].size;
	SInt32	scale = FskWindowScaleGet(_fskWindow);
	SInt32	rotate = FskWindowRotateGet(_fskWindow);
	NSPoint	 point;
	FskPointAndTicksRecord pat;
	NSTimeInterval	when;
	FskTimeRecord	mouseTimeR;
	FskEvent		fskEvent;

	if (FskWindowCursorIsVisible(_fskWindow)) return;
	//point.x = touchLocation.x + ((touch.normalizedPosition.x - touchPosition.x) * windowSize.width);
	//point.y = touchLocation.y + ((touch.normalizedPosition.y - touchPosition.y) * windowSize.height);
	point.x = touch.normalizedPosition.x * windowSize.width;
	point.y = touch.normalizedPosition.y * windowSize.height;

	point.y = windowSize.height - point.y;
	pat.pt.x = point.x;
	pat.pt.y = point.y;

	pat.pt.x /= scale;
	pat.pt.y /= scale;
	if (0 != rotate) {
		if (90 == rotate) {
			SInt32 temp = pat.pt.x;
			pat.pt.x = pat.pt.y;
			pat.pt.y = (_fskWindow->bits->bounds.height - 1) - temp;
		}
		else if (270 == rotate) {
			SInt32 temp = pat.pt.x;
			pat.pt.x = (_fskWindow->bits->bounds.width - 1) - pat.pt.y;
			pat.pt.y = temp;
		}
	}
	if (_fskWindow->port) {
		pat.pt.x -= _fskWindow->port->origin.x;
		pat.pt.y -= _fskWindow->port->origin.y;
	}

	[self flagsChanged:event];
	//fprintf(stderr, "%d [%d] %d %d\n", eventCode, j, pat.pt.x, pat.pt.y);
	if (!eventOut || !*eventOut) {
		when = [event timestamp];
		pat.ticks = (UInt32)(when * 1000.0);
		pat.index = j;
		mouseTimeR.useconds = mouseTimeR.seconds = 0;
		FskTimeAddMS(&mouseTimeR, pat.ticks);
		if (kFskErrNone != FskEventNew(&fskEvent, eventCode, &mouseTimeR, _keyModifiers))
			return;

		FskEventParameterAdd(fskEvent, kFskEventParameterMouseLocation, sizeof(pat), &pat);
		FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(j), &j);	//@@

		if (eventOut)
			*eventOut = fskEvent;
		else
			FskWindowEventQueue(_fskWindow, fskEvent);
	}
	else {
		UInt32 size;
		FskPointAndTicks params, first, last;

		fskEvent = *eventOut;

		// assumes that points are all at the same time (and consequently that each index appears, at most, once)
		FskEventParameterGetSize(fskEvent, kFskEventParameterMouseLocation, &size);
		size += sizeof(FskPointAndTicksRecord);
		if (kFskErrNone != FskMemPtrNew(size, &params)) {
			FskEventDispose(fskEvent);
			*eventOut = NULL;
			return;
		}
		FskEventParameterGet(fskEvent, kFskEventParameterMouseLocation, params);
		first = params;
		last = first + (size / sizeof(FskPointAndTicksRecord)) - 1;
		last->index = j;
		last->ticks = first->ticks;
		last->pt = pat.pt;
		FskEventParameterRemove(fskEvent, kFskEventParameterMouseLocation);
		FskEventParameterAdd(fskEvent, kFskEventParameterMouseLocation, size, params);
		FskMemPtrDispose(params);
	}
}

- (void)touchesBeganWithEvent:(NSEvent *)event
{
	//fprintf(stderr, "touchesBeganWithEvent\n");
    NSSet *set = [event touchesMatchingPhase:NSTouchPhaseBegan inView:nil];
	NSArray *array = [set allObjects];
    int c = set.count, i;
	for (i = 0; i < c; i++) {
		NSTouch* touch = [array objectAtIndex:i];
    	int j;
   		for (j = 0; j < kNumberOfFingers; j++) {
    		NSTouch* former = touches[j];
    		if (former == nil) {
    			/*touchCount++;
				if ((touchCount == 1) && (j == 0)) {
					touchLocation = [event locationInWindow];
					touchPosition = touch.normalizedPosition;
				}*/
    			[self handleTouch:touch at:j with:event as:kFskEventMouseDown eventOut:NULL];
    			touches[j] = [touch retain];
    			break;
    		}
    	}
	}
}

- (void)touchesMovedWithEvent:(NSEvent *)event
{
	//fprintf(stderr, "touchesMovedWithEvent\n");
    NSSet *set = [event touchesMatchingPhase:NSTouchPhaseMoved inView:nil];
	NSArray *array = [set allObjects];
    int c = set.count, i;
	FskEvent fskEvent = NULL;
    for (i = 0; i < c; i++) {
    	NSTouch* touch = [array objectAtIndex:i];
    	int j;
   		for (j = 0; j < kNumberOfFingers; j++) {
    		NSTouch* former = touches[j];
    		if ([former.identity isEqual:touch.identity]) {
				/*if ((touchCount == 1) && (j == 0)) {
					touchLocation = [event locationInWindow];
					touchPosition = touch.normalizedPosition;
				}*/
    			[self handleTouch:touch at:j with:event as:kFskEventMouseMoved eventOut:&fskEvent];
    			[former release];
    			touches[j] = [touch retain];
    			break;
    		}
    	}
    }

	if (fskEvent)
		FskWindowEventQueue(_fskWindow, fskEvent);
}

- (void)touchesEndedWithEvent:(NSEvent *)event
{
	//fprintf(stderr, "touchesEndedWithEvent\n");
    NSSet *set = [event touchesMatchingPhase:NSTouchPhaseEnded inView:nil];
	NSArray *array = [set allObjects];
    int c = set.count, i;
    for (i = 0; i < c; i++) {
    	NSTouch* touch = [array objectAtIndex:i];
    	int j;
   		for (j = 0; j < kNumberOfFingers; j++) {
    		NSTouch* former = touches[j];
    		if ([former.identity isEqual:touch.identity]) {
    			[self handleTouch:touch at:j with:event as:kFskEventMouseUp eventOut:NULL];
    			[former release];
    			touches[j] = nil;
    			//touchCount--;
    			break;
    		}
    	}
    }
}

- (void)touchesCancelledWithEvent:(NSEvent *)event
{
	//fprintf(stderr, "touchesCancelledWithEvent\n");
    NSSet *set = [event touchesMatchingPhase:NSTouchPhaseCancelled inView:nil];
	NSArray *array = [set allObjects];
    int c = set.count, i;
    for (i = 0; i < c; i++) {
    	NSTouch* touch = [array objectAtIndex:i];
    	int j;
   		for (j = 0; j < kNumberOfFingers; j++) {
    		NSTouch* former = touches[j];
    		if ([former.identity isEqual:touch.identity]) {
    			[self handleTouch:touch at:j with:event as:kFskEventMouseUp eventOut:NULL];
    			[former release];
    			touches[j] = nil;
    			//touchCount--;
    			break;
    		}
    	}
    }
}

- (void)beginDraw
{
	LOGD("[FskCocoaView:%p beginDraw] in thread %s", self, FskThreadName(FskThreadGetCurrent()));
#if kFskCocoaCopyBitsUseOpenGL
//	NSOpenGLContext *openGLContext;

//	openGLContext = [self openGLContext];
	[self lockFocus];
#endif /* kFskCocoaCopyBitsUseOpenGL */
}

- (void)endDraw
{
	LOGD("[FskCocoaView:%p   endDraw] in thread %s", self, FskThreadName(FskThreadGetCurrent()));
#if kFskCocoaCopyBitsUseOpenGL
	[[self openGLContext] flushBuffer];	/* This acts like a swapBuffers() */
	[self unlockFocus];
#endif /* kFskCocoaCopyBitsUseOpenGL */
}

#if USE_DISPLAY_LINK
#pragma mark --- CVDisplayLink ---

- (BOOL)canUpdateWindow
{
	FskMutexAcquire(_displayLinkMutex);
	if (_windowUpdating) {
		FskMutexRelease(_displayLinkMutex);
		return NO;
	}

	_windowUpdating = YES;
	FskMutexRelease(_displayLinkMutex);
	return YES;
}

- (void)updateWindowFinished
{
	FskMutexAcquire(_displayLinkMutex);
	_windowUpdating = NO;
	FskMutexRelease(_displayLinkMutex);
}

#define USE_FSK_THREAD_CALLBACK	0
#if USE_FSK_THREAD_CALLBACK
static void
sWindowUpdateCallback(void *arg1, void *arg2, void *arg3, void *arg4)
{
	FskCocoaView *cocoaView = (FskCocoaView *)arg1;
	FskWindow fskWindow = [cocoaView fskWindow];

	if (fskWindow)
		FskWindowUpdate(fskWindow, NULL);
	[cocoaView updateWindowFinished];
}
#else
- (void)windowUpdate
{
	if (_fskWindow != NULL)
		FskWindowUpdate(_fskWindow, NULL);
	[self updateWindowFinished];
}
#endif

static CVReturn
sDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
	FskCocoaView *cocoaView = (FskCocoaView *)displayLinkContext;

	if (![cocoaView canUpdateWindow]) {
		return kCVReturnSuccess;
	}

	NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
#if USE_FSK_THREAD_CALLBACK
	if (FskThreadPostCallback([cocoaView fskWindow]->thread, sWindowUpdateCallback, cocoaView, NULL, NULL, NULL) != kFskErrNone) {
		[cocoaView updateWindowFinished];
	}
#else
	[(FskCocoaView *)displayLinkContext performSelectorOnMainThread:@selector(windowUpdate) withObject:nil waitUntilDone:NO];
#endif
	[autoreleasepool release];

	return kCVReturnSuccess;
}

- (void)setDisplayLinkActive:(BOOL)active
{
	if (active) {
		if (_displayLink == NULL) {
			CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
			CVDisplayLinkSetOutputCallback(_displayLink, &sDisplayLinkCallback, self);
			CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
			CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
			CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(_displayLink, cglContext, cglPixelFormat);
		}
		if (!CVDisplayLinkIsRunning(_displayLink))
			CVDisplayLinkStart(_displayLink);
	} else {
		if (_displayLink) {
			if (CVDisplayLinkIsRunning(_displayLink))
				CVDisplayLinkStop(_displayLink);
			CVDisplayLinkRelease(_displayLink);
			_displayLink = NULL;
			[self updateWindowFinished];
		}
	}
}

- (void)pauseDisplayLink:(BOOL)flag
{
	if (![self isDisplayLinkActive])
		return;

	if (flag)
		CVDisplayLinkStop(_displayLink);
	else
		CVDisplayLinkStart(_displayLink);
}

- (BOOL) isDisplayLinkRunning
{
	return [self isDisplayLinkActive] && CVDisplayLinkIsRunning(_displayLink);
}

- (BOOL) isDisplayLinkActive
{
	return _displayLink != NULL;
}
#endif

@end
