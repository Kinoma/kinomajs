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
#define __FSKCOCOASUPPORT_PRIV__
#define __FSKWINDOW_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKPORT_PRIV__
#define __FSKGLBLIT_PRIV__

#import "FskCocoaViewPhone.h"
#import "FskCocoaSupportPhone.h"
#import "FskString.h"
#import "FskUtilities.h"
#if FSKBITMAP_OPENGL
#import <OpenGLES/EAGL.h>
#if GLES_VERSION == 1 && kFskCocoaCopyBitsUseOpenGL
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#else
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#endif
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#include "FskGLBlit.h"
#include "FskGLContext.h"
static EAGLContext *gMainContext = nil;
#endif

#if SUPPORT_INSTRUMENTATION

	#define LOG_PARAMETERS

	#include "FskInstrumentation.h"
	#define COCOA_DEBUG	1
	#define FskInstrumentedGlobalType(type)			(&(g##type##TypeInstrumentation))												/**< The data structure created with FskInstrumentedSimpleType(). */
	#define FskInstrumentedTypeEnabled(type, level)	FskInstrumentedTypeHasListenersForLevel(FskInstrumentedGlobalType(type), level)	/**< Whether instrumentation will print anything. */
#else /* !SUPPORT_INSTRUMENTATION */
	#define FskInstrumentedTypeEnabled(type, level)	(0)																				/**< Instrumentation will not print anything. */
#endif /* SUPPORT_INSTRUMENTATION */

FskInstrumentedSimpleType(CocoaView, cocoaview);													/**< This declares the types needed for instrumentation. */

#if COCOA_DEBUG
	#define	LOGD(...)	FskCocoaViewPrintfDebug(__VA_ARGS__)										/**< Print debugging logs. */
	#define	LOGI(...)	FskCocoaViewPrintfVerbose(__VA_ARGS__)										/**< Print information logs. */
#endif	/* COCOA_DEBUG */
#define		LOGE(...)	FskCocoaViewPrintfMinimal(__VA_ARGS__)										/**< Print error logs always, when instrumentation is on. */
#ifndef     LOGD
	#define LOGD(...)																				/**< Don't print debugging logs. */
#endif   		/* LOGD */
#ifndef     LOGI
	#define LOGI(...)																				/**< Don't print information logs. */
#endif   		/* LOGI */
#ifndef     LOGE
	#define LOGE(...)																				/**< Don't print error logs. */
#endif   	/* LOGE */
#define LOGD_ENABLED()	FskInstrumentedTypeEnabled(CocoaView, kFskInstrumentationLevelDebug)		/**< Whether LOGD() will print anything. */
#define LOGI_ENABLED()	FskInstrumentedTypeEnabled(CocoaView, kFskInstrumentationLevelVerbose)		/**< Whether LOGI() will print anything. */
#define LOGE_ENABLED()	FskInstrumentedTypeEnabled(CocoaView, kFskInstrumentationLevelMinimal)		/**< Whether LOGE() will print anything. */



#define OWN_BITMAP_CONTEXT
#define DUMMY_MOUSE_DOWN    1
#define kNumberOfFingers    10
#define DEBUG_TEXT_INPUT_SYSTEM 0

#if TEXT_INPUT_SYSTEM
#if DEBUG_TEXT_INPUT_SYSTEM
#define showKeyboardString()        NSLog(@"%@", _keyboardString)
#define textInputLog(...)           NSLog(__VA_ARGS__);
#else
#define showKeyboardString()
#define textInputLog(...)
#endif

#pragma mark UITextPosition

@interface IndexedPosition : UITextPosition {
    NSUInteger               _index;
    id <UITextInputDelegate> _inputDelegate;
}

@property (nonatomic) NSUInteger index;
+ (IndexedPosition *)positionWithIndex:(NSUInteger)index;

@end


#pragma mark UITextRange

@interface IndexedRange : UITextRange {
    NSRange _range;
}

@property (nonatomic) NSRange range;
+ (IndexedRange *)rangeWithNSRange:(NSRange)range;

@end
#endif

@interface FskCocoaView (Private)

- (CGRect)cgBounds;

#if FSKBITMAP_OPENGL
- (BOOL)createFramebuffer;
- (void)destroyFramebuffer;
#endif

#if defined(SAVE_IMAGE)
- (void)saveImage:(CGImageRef)imageRef name:(NSString*)name;
#endif

@end

@implementation FskCocoaView
{
	FskWindow			_fskWindow;
	FskPointRecord		_lastTouchedPoint;
#if MULTI_TOUCHES
    UITouch             **_touches;
#endif
    BOOL _keyboardActivated;
#if TEXT_INPUT_SYSTEM
    NSMutableString *_keyboardString;
    NSRange _keyboardSelectedRange;
    NSRange _keyboardMarkedRange;
    BOOL _keyboardUpdating;
    UITextInputStringTokenizer *_tokenizer;
    NSDictionary *_markedTextStyle;
    id <UITextInputDelegate> _inputDelegate;
    xsMachine *_the;
    xsSlot _obj;
    UITextAutocapitalizationType _autocapitalizationType;
    UITextAutocorrectionType _autocorrectionType;
    UIKeyboardType _keyboardType;
#endif
#if FSKBITMAP_OPENGL
	EAGLContext *context;
	GLuint viewRenderbuffer, viewFramebuffer;
	GLuint depthRenderbuffer;
	FskGLBlitContext blitContext;
#if GLES_VERSION == 1 && kFskCocoaCopyBitsUseOpenGL
    GLuint _bitmapTexture;
    GLfloat _textureVertices[4][2];
    GLshort _textureCoords[4][2];
#endif
#else
    BOOL				_windowClipIsValid;
	CGMutablePathRef	_windowClipCGPath;
	CGContextRef		_bitmapContextRef;
#endif
}

#pragma mark --- constants ---
const float kFskCocoaViewCornerRadius = 8;

#if FSKBITMAP_OPENGL
+ (Class) layerClass
{
    return [CAEAGLLayer class];
}
#endif

#pragma mark --- init ---
- (id)initWithFrame:(CGRect)frame screenScale:(CGFloat)screenScale isMainView:(BOOL)isMainView
{
	if (self = [super initWithFrame:frame])
	{
        self.contentScaleFactor = screenScale;
		_isMainView = isMainView;

		_lastTouchedPoint.x = _lastTouchedPoint.y = 0;
#if MULTI_TOUCHES
        if (FskMemPtrNewClear(kNumberOfFingers * sizeof(UITouch*), &_touches) != kFskErrNone)
        {
            [self release];
            return nil;
        }
        self.multipleTouchEnabled = YES;
#endif
		self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
		self.autoresizesSubviews = NO;
#if FSKBITMAP_OPENGL
		CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
#if GLES_VERSION == 1 && kFskCocoaCopyBitsUseOpenGL
        GLubyte *texData;
        size_t width, height;
        size_t tWidth, tHeight;
        CGContextRef texContext;
        CGRect cgBounds = [self cgBounds];
        CGColorSpaceRef rgbColorSpace;
        float clearColor[4] = {0.0, 0.0, 0.0, 1.0};
#endif
        BOOL useGL = FskCocoaBitmapUseGL();

		eaglLayer.opaque = YES;
		eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:useGL ? NO : YES], kEAGLDrawablePropertyRetainedBacking,
                                        kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];

#if GLES_VERSION == 1
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
		LOGD("initWithFrame: GLES1 context = %p", context);
#else
		if (self.isMainView) {
			context = [[EAGLContext alloc] initWithAPI:(useGL ? kEAGLRenderingAPIOpenGLES2 : kEAGLRenderingAPIOpenGLES1)]; // until copyBits supports ES2
			gMainContext = context;
		} else {
			context = [[EAGLContext alloc] initWithAPI:(useGL ? kEAGLRenderingAPIOpenGLES2 : kEAGLRenderingAPIOpenGLES1) sharegroup:[gMainContext sharegroup]];
		}
		LOGD("initWithFrame: GLES%d context = %p", (useGL ? 2 : 1), context);
#endif

		if (!context || ![EAGLContext setCurrentContext:context]) {
			[self release];
			return nil;
		}

		if (self.isMainView) {
			blitContext = FskGLBlitContextGetCurrent();
		} else {
			FskGLBlitContextNewFromCurrent(&blitContext);
			FskGLBlitContextMakeCurrent(blitContext);
		}
#if GLES_VERSION == 1 && kFskCocoaCopyBitsUseOpenGL
	if (!useGL) {
		glMatrixMode(GL_PROJECTION);
		glOrthof(0, cgBounds.size.width, cgBounds.size.height, 0, -1, 1);
		glViewport(0, 0, cgBounds.size.width, cgBounds.size.height);
		glMatrixMode(GL_MODELVIEW);

		glDisable(GL_DITHER);
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_VERTEX_ARRAY);

        width = (size_t)cgBounds.size.width;
        height = (size_t)cgBounds.size.height;
        tWidth = 64;
        while (tWidth < width) {
            tWidth <<= 1;
        }
        tHeight = 64;
        while (tHeight < height) {
            tHeight <<= 1;
        }
        cgBounds.origin.x = 0;
        cgBounds.origin.y = 0;

        rgbColorSpace = CGColorSpaceCreateDeviceRGB();
        texData = (GLubyte *)calloc(tWidth * tHeight * 4, sizeof(GLubyte));
        texContext = CGBitmapContextCreate(texData, tWidth, tHeight, 8, tWidth * 4, rgbColorSpace, (CGBitmapInfo)kCGImageAlphaPremultipliedLast);
        CGColorSpaceRelease(rgbColorSpace);
        CGContextSetFillColor(texContext, clearColor);
        CGContextFillRect(texContext, cgBounds);
        CGContextRelease(texContext);

        glGenTextures(1, &_bitmapTexture);
        glBindTexture(GL_TEXTURE_2D, _bitmapTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tWidth, tHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
        free(texData);

        _textureVertices[0][0] = 0;
        _textureVertices[0][1] = 0;
        _textureVertices[1][0] = (GLfloat)tWidth;
        _textureVertices[1][1] = 0;
        _textureVertices[2][0] = 0;
        _textureVertices[2][1] = (GLfloat)tHeight;
        _textureVertices[3][0] = (GLfloat)tWidth;
        _textureVertices[3][1] = (GLfloat)tHeight;
        _textureCoords[0][0] = 0;
        _textureCoords[0][1] = 0;
        _textureCoords[1][0] = 1;
        _textureCoords[1][1] = 0;
        _textureCoords[2][0] = 0;
        _textureCoords[2][1] = 1;
        _textureCoords[3][0] = 1;
        _textureCoords[3][1] = 1;
	}
#endif
#else
#if defined(OWN_BITMAP_CONTEXT)
        CGRect cgBounds = [self cgBounds];
        CGFloat width, height;
        CGColorSpaceRef rgbColorSpace;
        CGImageAlphaInfo alphaInfo;
        UInt32 rowBytes;

        width = cgBounds.size.width;
        height = cgBounds.size.height;

        if (FskCocoaBitmapGetColorInfo(kFskBitmapFormatDefaultRGB, width, &alphaInfo, &rgbColorSpace, &rowBytes)) {

            rgbColorSpace = CGColorSpaceCreateDeviceRGB();
            _bitmapContextRef = CGBitmapContextCreate(NULL, width, height, 8, rowBytes, rgbColorSpace, alphaInfo);
            CGColorSpaceRelease(rgbColorSpace);
            [self clearBitmapContextWithRect:cgBounds];
        }
#endif
#endif
		NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
		[notificationCenter addObserver:self selector:@selector(keyboardDidHide:) name:UIKeyboardDidHideNotification object:nil];
		[notificationCenter addObserver:self selector:@selector(keyboardDidShow:) name:UIKeyboardDidShowNotification object:nil];
		[notificationCenter addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
		[notificationCenter addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
	}

	return self;
}

- (void)dealloc
{
    self.keyboardActive = NO;
#if TEXT_INPUT_SYSTEM
    self.keyboardString = nil;
    if (_tokenizer != nil)
    {
        [_tokenizer release];
    }
#endif
#if MULTI_TOUCHES
    FskMemPtrDispose(_touches);
#endif
#if FSKBITMAP_OPENGL
	FskGLBlitContextDispose(blitContext);
	[context release];
#else
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[self setFskWindow:nil];
	[self setWindowClipCGPath:nil];

	if (_bitmapContextRef)
		CGContextRelease(_bitmapContextRef);
#endif

	[super dealloc];
}

#pragma mark --- getters/setters ---

@synthesize isMainView = _isMainView;
@dynamic keyboardActive;
#if TEXT_INPUT_SYSTEM
@synthesize keyboardString = _keyboardString;
@dynamic keyboardSelectedRange;
@synthesize keyboardMarkedRange = _keyboardMarkedRange;
@synthesize keyboardUpdating = _keyboardUpdating;
@synthesize markedTextStyle = _markedTextStyle;
@synthesize inputDelegate = _inputDelegate;
@synthesize machine = _the;
@synthesize obj = _obj;
@synthesize autocapitalizationType = _autocapitalizationType;
@synthesize autocorrectionType = _autocorrectionType;
@synthesize keyboardType = _keyboardType;
#endif

- (BOOL)keyboardActive
{
    return self.isFirstResponder;
}

- (void)setKeyboardActive:(BOOL)active
{
    if (active)
    {
        if (!self.isFirstResponder)
        {
            _keyboardActivated = YES;
            [self becomeFirstResponder];
        }
    }
    else
    {
        if (self.isFirstResponder)
        {
            [self resignFirstResponder];
        }
    }
}

- (void)keyboardDidHide:(NSNotification*)aNotification
{
    NSDictionary* info = [aNotification userInfo];
    CGSize size = [[info objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
    UIScreen* screen = [UIScreen mainScreen];
	FskEvent fskEvent;
	UInt32 cmd = 1029;
	UInt32 height = (size.width == screen.bounds.size.width) ? size.height : size.width;
	if (FskEventNew(&fskEvent, kFskEventKeyDown, NULL, 0) == kFskErrNone) {
    	(void)FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(UInt32), &cmd);
    	(void)FskEventParameterAdd(fskEvent, kFskEventParameterIntegerValue, sizeof(UInt32), &height);
		FskWindowEventSend(_fskWindow, fskEvent);
	}
}

- (void)keyboardDidShow:(NSNotification*)aNotification
{
    NSDictionary* info = [aNotification userInfo];
    CGSize size = [[info objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue].size;
    UIScreen* screen = [UIScreen mainScreen];
	FskEvent fskEvent;
	UInt32 cmd = 1027;
	UInt32 height = (size.width == screen.bounds.size.width) ? size.height : size.width;
	if (FskEventNew(&fskEvent, kFskEventKeyDown, NULL, 0) == kFskErrNone) {
    	(void)FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(UInt32), &cmd);
    	(void)FskEventParameterAdd(fskEvent, kFskEventParameterIntegerValue, sizeof(UInt32), &height);
		FskWindowEventSend(_fskWindow, fskEvent);
	}
}

- (void)keyboardWillHide:(NSNotification*)aNotification
{
    NSDictionary* info = [aNotification userInfo];
    CGSize size = [[info objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
    UIScreen* screen = [UIScreen mainScreen];
	FskEvent fskEvent;
	UInt32 cmd = 1028;
	UInt32 height = (size.width == screen.bounds.size.width) ? size.height : size.width;
	if (FskEventNew(&fskEvent, kFskEventKeyDown, NULL, 0) == kFskErrNone) {
    	(void)FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(UInt32), &cmd);
    	(void)FskEventParameterAdd(fskEvent, kFskEventParameterIntegerValue, sizeof(UInt32), &height);
		FskWindowEventSend(_fskWindow, fskEvent);
	}
}

- (void)keyboardWillShow:(NSNotification*)aNotification
{
    NSDictionary* info = [aNotification userInfo];
    CGSize size = [[info objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue].size;
    UIScreen* screen = [UIScreen mainScreen];
	FskEvent fskEvent;
	UInt32 cmd = 1026;
	UInt32 height = (size.width == screen.bounds.size.width) ? size.height : size.width;
	if (FskEventNew(&fskEvent, kFskEventKeyDown, NULL, 0) == kFskErrNone) {
    	(void)FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(UInt32), &cmd);
    	(void)FskEventParameterAdd(fskEvent, kFskEventParameterIntegerValue, sizeof(UInt32), &height);
		FskWindowEventSend(_fskWindow, fskEvent);
	}
}

#if TEXT_INPUT_SYSTEM
- (NSString *)keyboardString
{
    return _keyboardString;
}

- (void)setKeyboardString:(NSMutableString*)string
{
    if (string == nil)
    {
        [_keyboardString release];
        _keyboardString = nil;
        _keyboardSelectedRange = NSMakeRange(0, 0);
        _keyboardMarkedRange = NSMakeRange(NSNotFound, 0);
        return;
    }

    if (![string isEqualToString:_keyboardString])
    {
        [self.inputDelegate textWillChange:self];
        [_keyboardString release];
        _keyboardString = [string retain];
        [self.inputDelegate textDidChange:self];
    }
}

- (NSRange)keyboardSelectedRange
{
    return _keyboardSelectedRange;
}

- (void)setKeyboardSelectedRange:(NSRange)range
{
    if (!NSEqualRanges(range, _keyboardSelectedRange))
    {
        [self.inputDelegate selectionWillChange:self];
        _keyboardSelectedRange = range;
        _keyboardMarkedRange = NSMakeRange(NSNotFound, 0);
        [self.inputDelegate selectionDidChange:self];
    }
}

- (void)setKeyboardType:(UIKeyboardType)newType
{
    if (newType != _keyboardType)
    {
        _keyboardType = newType;

        if (self. isFirstResponder)
        {
            [self resignFirstResponder];
            [self becomeFirstResponder];
        }
    }
}
#endif

- (FskWindow)fskWindow
{
	return _fskWindow;
}

- (void)setFskWindow:(FskWindow)fskWindow
{
	_fskWindow = fskWindow;

    if (fskWindow->isFullscreenWindow && FskCocoaBitmapUseGL())
    {
        self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    }
}

#if !kFskCocoaCopyBitsUseOpenGL && !FSKBITMAP_OPENGL
- (void)setWindowClipCGPath:(CGMutablePathRef)windowClipCGPath
{
	if (_windowClipCGPath)
		CGPathRelease(_windowClipCGPath);

	_windowClipCGPath = windowClipCGPath;
}
#endif

- (FskPointRecord)lastTouchedPoint
{
	return _lastTouchedPoint;
}

#pragma mark --- UIView overrides ---
#if FSKBITMAP_OPENGL
-(void)layoutSubviews
{
	FskGLBlitContextMakeCurrent(blitContext);
	[self destroyFramebuffer];
	[self createFramebuffer];
    FskWindowCocoaSizeChanged(_fskWindow);
}
#else
- (void)drawRect:(CGRect)rect
{
	FskRectangleRecord	fskRectangle;
	CGImageRef			imageRef;
	CGContextRef		contextRef;
	CGRect				bounds;

	if (_fskWindow == NULL) return;
	if (_bitmapContextRef == NULL) return;

	imageRef = CGBitmapContextCreateImage(_bitmapContextRef);
    if (imageRef == NULL) return;

	bounds = [self bounds];
	contextRef = UIGraphicsGetCurrentContext();

	CGContextSaveGState(contextRef);
    CGContextSetBlendMode(contextRef, kCGBlendModeCopy);
	CGContextTranslateCTM(contextRef, 0, bounds.size.height);
	CGContextScaleCTM(contextRef, 1.0, -1.0);

	CGContextDrawImage(contextRef, bounds, imageRef);
#if defined(SAVE_IMAGE)
	[self saveImage:imageRef name:@"drawRect"];
#endif
	CGImageRelease(imageRef);

	CGContextRestoreGState(contextRef);
	//NSLog(@"drawRect: (%d %d) (%d %d)", (int)rect.origin.x, (int)rect.origin.y, (int)rect.size.width, (int)rect.size.height);
}
#endif

- (BOOL)canBecomeFirstResponder
{
	return _keyboardActivated;
}

- (BOOL)canResignFirstResponder
{
	return YES;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self handleTouches:touches];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self handleTouches:touches];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self handleTouches:touches];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self handleTouches:touches];
}

- (void)throwKeyEvent:(char)keyCode keyDown:(BOOL)down
{
	FskEvent fskEvent;

	if (FskEventNew(&fskEvent, down ? kFskEventKeyDown : kFskEventKeyUp, NULL, 0) == kFskErrNone) {
		char chars[2];
		chars[0] = keyCode;
		chars[1] = 0;
		FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 2, chars);
		FskWindowEventQueue(_fskWindow, fskEvent);
	}
}

- (void)motionBegan:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
	[self throwKeyEvent:(char)8 keyDown:YES];
	[self throwKeyEvent:(char)8 keyDown:NO];
}

- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
	// [self throwKeyEvent:(char)8 keyDown:NO];	// motionBegan and motionEnded are exclusive??
}

#pragma mark --- methods ---
- (void)copyBits:(FskBitmap)fskBitmap sourceRect:(FskRectangle)sourceRect destRect:(FskRectangle)destRect
{
#if GLES_VERSION == 1 && kFskCocoaCopyBitsUseOpenGL
	[EAGLContext setCurrentContext:context];
	glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);

	// must be called only when !useGL
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

#if 0
	FskBitmap tmp = NULL;
	FskBitmapNew(destRect->width, destRect->height, kFskBitmapFormat32RGBA, &tmp);
	FskBitmapDraw(fskBitmap, sourceRect, tmp, &tmp->bounds, NULL, NULL, kFskGraphicsModeCopy, NULL);
	glTexSubImage2D(GL_TEXTURE_2D, 0, destRect->x, destRect->y, destRect->width, destRect->height, GL_RGBA, GL_UNSIGNED_BYTE, tmp->bits);
	FskBitmapDispose(tmp);
#else
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (size_t)fskBitmap->bounds.width, (size_t)fskBitmap->bounds.height, GL_RGBA, GL_UNSIGNED_BYTE, fskBitmap->bits);
#endif

	glBindTexture(GL_TEXTURE_2D, _bitmapTexture);
	glVertexPointer(2, GL_FLOAT, 0, _textureVertices);
	glTexCoordPointer(2, GL_SHORT, 0, _textureCoords);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
	[context presentRenderbuffer:GL_RENDERBUFFER];
#elif FSKBITMAP_OPENGL
    // do nothing
#else
#if !defined(OWN_BITMAP_CONTEXT)
    CGRect				bitmapCGRect;
#else
	CGRect				bounds;
    CGImageRef 			image;
    CGRect				bitmapCGRect, sourceCGRect, destCGRect, excludeCGRect;
    FskWindowExclude	fskWindowExclude;
    CGContextRef		cgBitmapContext;
#endif

	if ((_fskWindow == NULL) || (fskBitmap == NULL) || (sourceRect == NULL) || (destRect == NULL)) return;

#if !defined(OWN_BITMAP_CONTEXT)
    if (_bitmapContextRef != NULL) {
        CGContextRelease(_bitmapContextRef);
    }
    _bitmapContextRef = (CGContextRef)(fskBitmap->cgBitmapContext);
    CGContextRetain(_bitmapContextRef);
	bitmapCGRect = CGRectMake(fskBitmap->bounds.x, fskBitmap->bounds.y, fskBitmap->bounds.width, fskBitmap->bounds.height);
    [self setNeedsDisplayInRect:bitmapCGRect];
#else
	cgBitmapContext = (CGContextRef)(fskBitmap->cgBitmapContext);

	// save state
	CGContextSaveGState(_bitmapContextRef);
    CGContextSetBlendMode(_bitmapContextRef, kCGBlendModeCopy);

	bounds = [self cgBounds];
	bitmapCGRect = CGRectMake(fskBitmap->bounds.x, fskBitmap->bounds.y, fskBitmap->bounds.width, fskBitmap->bounds.height);
	sourceCGRect = CGRectMake(sourceRect->x, bounds.size.height - sourceRect->y - sourceRect->height, sourceRect->width, sourceRect->height);
	destCGRect = CGRectMake(destRect->x, bounds.size.height - destRect->y - destRect->height, destRect->width, destRect->height);

	//[self clearBitmapContextWithRect:destCGRect];

	// clip window
	if (_fskWindow->isCustomWindow)
		[self clipWindow];

	// clip
	CGContextClipToRect(_bitmapContextRef, destCGRect);

	for (fskWindowExclude = _fskWindow->excludes; fskWindowExclude; fskWindowExclude = fskWindowExclude->next)
	{
		excludeCGRect = CGRectMake(fskWindowExclude->bounds.x, bounds.size.height - fskWindowExclude->bounds.y - fskWindowExclude->bounds.height, fskWindowExclude->bounds.width, fskWindowExclude->bounds.height);
		CGContextClipToRect(_bitmapContextRef, excludeCGRect);
	}

	// translate and scale
	if (!CGRectEqualToRect(destCGRect, sourceCGRect))
	{
		CGContextTranslateCTM(_bitmapContextRef, destCGRect.origin.x - sourceCGRect.origin.x, destCGRect.origin.y - sourceCGRect.origin.y);
		CGContextScaleCTM(_bitmapContextRef, destCGRect.size.width / sourceCGRect.size.width, destCGRect.size.height / sourceCGRect.size.height);
	}

	// draw the image
	image = CGBitmapContextCreateImage(cgBitmapContext);

	if (image)
	{
		CGContextDrawImage(_bitmapContextRef, bitmapCGRect, image);
#if defined(SAVE_IMAGE)
        [self saveImage:image name:@"copyBits"];
#endif
		CGImageRelease(image);

		[self setNeedsDisplayInRect:bitmapCGRect];
	}
	//else {NSLog(@"no image");}

	// restore state
	CGContextRestoreGState(_bitmapContextRef);
#endif
#endif

    //NSLog(@"copyBits:(%d %d) (%d %d) -> (%d %d) (%d %d)", (int)sourceRect->x, (int)sourceRect->y, (int)sourceRect->width, (int)sourceRect->height,
    //                                                      (int)destRect->x, (int)destRect->y, (int)destRect->width, (int)destRect->height);
}

#if !kFskCocoaCopyBitsUseOpenGL && !FSKBITMAP_OPENGL
- (void)clearBitmapContextWithRect:(CGRect)destCGRect
{
	CGColorSpaceRef		rgbColorSpace;
	float				clearColor[4] = {0.0, 0.0, 0.0, 0.0};

	rgbColorSpace = CGColorSpaceCreateDeviceRGB();

	if (rgbColorSpace)
	{
		CGContextSetFillColorSpace(_bitmapContextRef, rgbColorSpace);
		CGColorSpaceRelease(rgbColorSpace);
	}

	CGContextSetFillColor(_bitmapContextRef, clearColor);
	CGContextFillRect(_bitmapContextRef, destCGRect);
}

- (void)clipWindow
{
	CGContextRef	cgContext;
	CGRect			bounds, innerBoundsCGRect;

	cgContext = UIGraphicsGetCurrentContext();

	if (!_windowClipIsValid)
	{
		// build the window clip path
		bounds = [self bounds];
		innerBoundsCGRect = CGRectInset(*(CGRect *)(&bounds), kFskCocoaViewCornerRadius, kFskCocoaViewCornerRadius);
		[self setWindowClipCGPath:CGPathCreateMutable()];

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
	}

	// clip
	CGContextAddPath(cgContext, _windowClipCGPath);
	CGContextClip(cgContext);
}
#endif

#if MULTI_TOUCHES
- (SInt32)newTouchIndex:(UITouch*)touch
{
    SInt32 index = -1;

    for (int i = 0; i < kNumberOfFingers; i++)
    {
        if (_touches[i] == nil)
        {
            _touches[i] = touch;
            index = i;
            break;
        }
    }

    return index;
}

- (SInt32)findTouchIndex:(UITouch*)touch dispose:(BOOL)dispose
{
    SInt32 index = -1;

    for (int i = 0; i < kNumberOfFingers; i++)
    {
        if (_touches[i] == touch)
        {
            index = i;
            if (dispose)
            {
                _touches[i] = nil;
            }
            break;
        }
    }

    return index;
}
#endif

- (void)throwTouchEvent:(UInt32)eventCode pointAndTicks:(FskPointAndTicks)pat clickCount:(UInt32)clickCount
{
	FskEvent fskEvent;
    FskTimeRecord mouseTimeR;

	mouseTimeR.useconds = mouseTimeR.seconds = 0;
	FskTimeAddMS(&mouseTimeR, pat->ticks);

    if (FskEventNew(&fskEvent, eventCode, &mouseTimeR, kFskEventModifierMouseButton) == kFskErrNone)
    {
        FskEventParameterAdd(fskEvent, kFskEventParameterMouseLocation, sizeof(FskPointAndTicksRecord), pat);
        FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(pat->index), &pat->index);	//@@

        if (clickCount)
            FskEventParameterAdd(fskEvent, kFskEventParameterMouseClickNumber, sizeof(clickCount), &clickCount);

        FskWindowEventQueue(_fskWindow, fskEvent);
    }
}

- (void)handleTouches:(NSSet *)touches
{
#if MULTI_TOUCHES
    UInt32 when = 0;
	UInt32 eventCode = 0, clickCount = 0;
	SInt32 rotate = FskCocoaApplicationGetRotation();
	CGPoint cgPoint;
    SInt32 statusBarHeight;
    FskPointAndTicksRecord pat;
    BOOL firstTouch = YES;
    BOOL touchCancelled;
#if DUMMY_MOUSE_DOWN
    BOOL needsDummyMouseDown;
#endif
    int index;
    int i;

    i = 0;
    for (UITouch *touch in touches)
    {
        touchCancelled = NO;
#if DUMMY_MOUSE_DOWN
        needsDummyMouseDown = NO;
#endif

        if (firstTouch)
        {
            eventCode = clickCount = 0;

            switch (touch.phase)
            {
                case UITouchPhaseBegan:
                    eventCode = kFskEventMouseDown;
                    break;
                case UITouchPhaseMoved:
                    eventCode = kFskEventMouseMoved;
                    break;
                case UITouchPhaseCancelled:
                    touchCancelled = YES;
                case UITouchPhaseEnded:
                    eventCode = kFskEventMouseUp;
                    FskWindowCancelStillDownEvents(_fskWindow);
                    break;
                default:
                    break;
            }

            if (eventCode == 0) return;

            if (eventCode == kFskEventMouseDown)
            {
                clickCount = (UInt32)touch.tapCount;

                if (clickCount == 2)
                    eventCode = kFskEventMouseDoubleClick;
            }
            when = (UInt32)([touch timestamp] * 1000.0);
            firstTouch = NO;
        }

        if (eventCode == kFskEventMouseDown)
        {
            index = [self newTouchIndex:touch];
        }
        else
        {
            index = [self findTouchIndex:touch dispose:(eventCode == kFskEventMouseUp) ? YES : NO];
         }
#if DUMMY_MOUSE_DOWN
        // corresponding down event was not found?
        if ((index < 0) && (eventCode == kFskEventMouseUp)) {
            index = [self newTouchIndex:touch];
            (void)[self findTouchIndex:touch dispose:YES];\
            needsDummyMouseDown = YES;
            //NSLog(@"needs dummy mouseDown");
        }
#endif
        if (index < 0)
        {
#if 0
            switch (eventCode) {
                case kFskEventMouseDown:
                    NSLog(@"touch start fail: no spaces");
                    break;
                case kFskEventMouseStillDown:
                    NSLog(@"touch moved fail: not found");
                    break;
                case kFskEventMouseUp:
                    NSLog(@"touch End fail: not found");
                    break;
                default:
                    break;
            }
#endif
            return;
        }

		cgPoint = [touch locationInView:FskCocoaBitmapUseGL() ? self : nil];
        statusBarHeight = FskCocoaApplicationGetStatusBarHeight();
        if (statusBarHeight > 0)
        {
            cgPoint.y -= statusBarHeight;
        }
		pat.pt.x = cgPoint.x;
		pat.pt.y = cgPoint.y;

        if (!FskCocoaBitmapUseGL() &&  (0 != rotate)) {
            if (90 == rotate)
            {
                SInt32 temp = pat.pt.x;
                pat.pt.x = pat.pt.y;
                pat.pt.y = (self.bounds.size.width - 1) - temp;
            }
            else if (270 == rotate)
            {
                SInt32 temp = pat.pt.x;
                pat.pt.x = (self.bounds.size.height - 1) - pat.pt.y;
                pat.pt.y = temp;
            }
        }
		if (_fskWindow->port)
		{
			pat.pt.x -= _fskWindow->port->origin.x;
			pat.pt.y -= _fskWindow->port->origin.y;
		}
        if (touchCancelled)
        {
            pat.pt.x = -1;
            pat.pt.y = -1;
        }
		if (i == 0)
		{
			_lastTouchedPoint.x = pat.pt.x;
			_lastTouchedPoint.y = pat.pt.y;
		}
        pat.index = index;
#if DUMMY_MOUSE_DOWN
        if (needsDummyMouseDown)
        {
            pat.ticks = when - 1;
            [self throwTouchEvent:kFskEventMouseDown pointAndTicks:&pat clickCount:clickCount];
        }
#endif
		pat.ticks = when;
		[self throwTouchEvent:eventCode pointAndTicks:&pat clickCount:clickCount];
#if 0
        switch (eventCode) {
            case kFskEventMouseDown:
                NSLog(@"touch start %d", (int)pat.index);
                break;
            case kFskEventMouseStillDown:
                NSLog(@"touch moved %d", (int)pat.index);
                break;
            case kFskEventMouseUp:
                NSLog(@"touch end %d", (int)pat.index);
                break;
            default:
                break;
        }
#endif
        i++;
    }
#if 0
    if (eventCode == kFskEventMouseUp)
    {
		int j;
        for (j = 0; j < kNumberOfFingers; j++)
        {
            if (_touches[j] != nil)
            {
                NSLog(@"touches not finished");
                break;
            }
        }
        if (j == kNumberOfFingers)
        {
            NSLog(@"touches finished");
        }
    }
#endif
#else
	FskEvent		fskEvent;
	UIWindow		*window;
	UInt32			eventCode = 0, clickCount = 0;
	SInt32          rotate = FskCocoaApplicationGetRotation();
	CGPoint			cgPoint;
	FskPointRecord	point;
	BOOL			firstTouch = YES;

	for (UITouch *touch in touches)
	{
		eventCode = clickCount = 0;

		switch (touch.phase)
		{
			case UITouchPhaseBegan:
				eventCode = kFskEventMouseDown;
				break;
			case UITouchPhaseMoved:
				eventCode = kFskEventMouseStillDown;
				break;
			case UITouchPhaseEnded:
				eventCode = kFskEventMouseUp;
				break;
			default:
				break;
		}

		if (eventCode == 0) continue;

		window = [touch window];
		cgPoint = [touch locationInView:nil];
		point.x = cgPoint.x;
		point.y = cgPoint.y;

        if (0 != rotate) {
            if (90 == rotate)
            {
                SInt32 temp = point.x;
                point.x = point.y;
                point.y = (self.bounds.size.width - 1) - temp;
            }
            else if (270 == rotate)
            {
                SInt32 temp = point.x;
                point.x = (self.bounds.size.height - 1) - point.y;
                point.y = temp;
            }
        }
		if (_fskWindow->port)
		{
			point.x -= _fskWindow->port->origin.x;
			point.y -= _fskWindow->port->origin.y;
		}
		if (firstTouch)
		{
			_lastTouchedPoint.x = point.x;
			_lastTouchedPoint.y = point.y;
			firstTouch = NO;
		}

		if (eventCode == kFskEventMouseDown)
		{
			clickCount = touch.tapCount;

			if (clickCount == 2)
				eventCode = kFskEventMouseDoubleClick;
		}

		if (FskEventNew(&fskEvent, eventCode, NULL, kFskEventModifierMouseButton) == kFskErrNone)
		{
			FskEventParameterAdd(fskEvent, kFskEventParameterMouseLocation, sizeof(point), &point);

			if (clickCount)
				FskEventParameterAdd(fskEvent, kFskEventParameterMouseClickNumber, sizeof(clickCount), &clickCount);

			FskWindowEventQueue(_fskWindow, fskEvent);
		}
	}
#endif
}

- (void)beginDraw
{
#if FSKBITMAP_OPENGL
	BOOL ok;
	LOGD("beginDraw: setCurrentContext(%p), previous=%p, isMain=%s", context, [EAGLContext currentContext], self.isMainView ? "Yes" : "No");
	ok = (FskGLBlitContextMakeCurrent(blitContext) == kFskErrNone);
	#if SUPPORT_INSTRUMENTATION
		if (!ok) LOGE("ERROR: beginDraw: setCurrentContext(%p) FAILS", context);
	#endif /* SUPPORT_INSTRUMENTATION */
#endif // FSKBITMAP_OPENGL
}

- (void)endDraw
{
#if FSKBITMAP_OPENGL
	if (viewRenderbuffer) {
		int retVal;
		LOGD("endDraw: bind and present RenderBuffer(%d) in context (%p), isMain=%s", viewRenderbuffer, context, self.isMainView ? "Yes" : "No");
		glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
		#if SUPPORT_INSTRUMENTATION
			if (LOGE_ENABLED()) {
				if (GL_NO_ERROR != (retVal = glGetError())) {		// Get GL error code ...
					retVal = FskErrorFromGLError(retVal);			// ... and convert it to our error code
					LOGE("endDraw: Fsk glBindRenderbuffer() returns %d", retVal);
				}
			}
		#endif /* SUPPORT_INSTRUMENTATION */
		retVal = [context presentRenderbuffer:GL_RENDERBUFFER];		// Returns BOOL
		#if SUPPORT_INSTRUMENTATION
			if (FALSE == retVal) LOGE("endDraw: presentRenderbuffer FAILS");
		#endif /* SUPPORT_INSTRUMENTATION */
	}
#endif // FSKBITMAP_OPENGL
}

#if TEXT_INPUT_SYSTEM
#pragma mark --- UITextInput methods ---

#pragma mark UITextInput - Replacing and Returning Text

- (void)insertStringToTextEdit:(NSString*)string withSelection:(NSRange)selection
{
    FskEvent fskEvent;
    const char *s;
    UInt32 len;
	UInt32 cmd;
    char empty = 0;
    char cr = '\r';
    char filelist[256], *pos;

    if (string != nil)
    {
        s = [string UTF8String];
        len = (UInt32)[string lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
        if ((len == 1) && (FskStrLen(s) == 1) && (s[0] == '\n'))
        {
            // replace nl to cr
            s = &cr;
        }
    }
    else
    {
        s = &empty;
        len = 0;
    }

	if (kFskErrNone != FskEventNew(&fskEvent, kFskEventKeyDown, NULL, 0))
    {
        return;
    }

    (void)FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, len + 1, (void*)s);

    cmd = 1025;
    (void)FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(UInt32), &cmd);

    pos = filelist;
    len = sprintf(pos, "%d", (int)selection.location);
    pos[len++] = '\0';
    len += sprintf(&pos[len], "%d", (int)selection.length);
    pos[len++] = '\0';
    pos[len++] = '\0';

    (void)FskEventParameterAdd(fskEvent, kFskEventParameterFileList, len, filelist);

    _keyboardUpdating = YES;
    FskWindowEventQueue(_fskWindow, fskEvent);
    _keyboardUpdating = NO;
}

- (void)insertString:(NSString*)string withSelection:(NSRange)selection
{
    if (string == nil)
    {
        [_keyboardString deleteCharactersInRange:selection];
    }
    else if (selection.length > 0)
    {
        [_keyboardString replaceCharactersInRange:selection withString:string];
    }
    else
    {
        [_keyboardString insertString:string atIndex:selection.location];
    }
    [self insertStringToTextEdit:string withSelection:selection];
}

- (void)insertMarkedString:(NSString*)string withSelection:(NSRange)selection
{
    FskEvent fskEvent;
    const char *s;
    UInt32 len;
	UInt32 cmd;
    char empty = 0;

#if 0
    if (NSEqualRanges(selection, _keyboardMarkedRange))
    {
        NSString *originalString;

        originalString = [_keyboardString substringWithRange:selection];
        if ([originalString isEqualToString:string])
        {
            return;
        }
    }
#endif

    /* updating local string */
    [_keyboardString replaceCharactersInRange:selection withString:string];

    /* delete characters in selection */
    [self insertStringToTextEdit:nil withSelection:selection];

    /* then add marked string and marked(selected in Fsk) */
    if (string != nil)
    {
        s = [string UTF8String];
        len = (UInt32)[string lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    }
    else
    {
        s = &empty;
        len = 0;
    }

	if (kFskErrNone != FskEventNew(&fskEvent, kFskEventKeyDown, NULL, 0))
    {
        return;
    }

    (void)FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, len + 1, (void*)s);

    cmd = 1024;
    (void)FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(UInt32), &cmd);

    _keyboardUpdating = YES;
    FskWindowEventQueue(_fskWindow, fskEvent);
    _keyboardUpdating = NO;
}

- (void)updateTextEditSelection
{
    NSString *originalString;

    originalString = [_keyboardString substringWithRange:_keyboardSelectedRange];
    [self insertStringToTextEdit:originalString withSelection:_keyboardSelectedRange];
}

- (NSString *)textInRange:(UITextRange *)range
{
    IndexedRange *r = (IndexedRange *)range;
    return ([_keyboardString substringWithRange:r.range]);
}

- (void)replaceRange:(UITextRange *)range withText:(NSString *)text
{
    IndexedRange *r = (IndexedRange *)range;

    textInputLog(@"replaceRange:(%d %d) withText:%@", (int)r.range.location, (int)r.range.length, text);

    if ((r.range.location + r.range.length) <= _keyboardSelectedRange.location) {
        _keyboardSelectedRange.location -= (r.range.length - text.length);
    } else {
        // Need to also deal with overlapping ranges.  Not addressed
		// in this simplified sample.
    }

    [self insertString:text withSelection:r.range];

    showKeyboardString();

    [self updateTextEditSelection];
}

#pragma mark UITextInput - Working with Marked and Selected Text

- (UITextRange *)selectedTextRange
{
    return [IndexedRange rangeWithNSRange:_keyboardSelectedRange];
}

- (void)setSelectedTextRange:(UITextRange *)range
{
    IndexedRange *r = (IndexedRange *)range;
    _keyboardSelectedRange = r.range;

    textInputLog(@"setSelectedTextRange:(%d %d)", (int)r.range.location, (int)r.range.length);

    showKeyboardString();

    /* updating text edit */
    [self updateTextEditSelection];
}

- (UITextRange *)markedTextRange
{
    return [IndexedRange rangeWithNSRange:_keyboardMarkedRange];
}

- (void)setMarkedText:(NSString *)markedText selectedRange:(NSRange)selectedRange
{
    textInputLog(@"setMarkedText:%@ selectedRange:(%d %d)", markedText, (int)selectedRange.location, (int)selectedRange.length);

    if (markedText == nil)
        markedText = @"";

    if (_keyboardMarkedRange.location != NSNotFound) {
        [self insertMarkedString:markedText withSelection:_keyboardMarkedRange];

        _keyboardMarkedRange.length = markedText.length;
    } else if (_keyboardSelectedRange.length > 0) {
        [self insertMarkedString:markedText withSelection:_keyboardSelectedRange];

        _keyboardMarkedRange.location = _keyboardSelectedRange.location;
        _keyboardMarkedRange.length = markedText.length;
    } else {
        [self insertMarkedString:markedText withSelection:_keyboardSelectedRange];

        _keyboardMarkedRange.location = _keyboardSelectedRange.location;
        _keyboardMarkedRange.length = markedText.length;
    }

    if (selectedRange.location == NSNotFound)
    {
        selectedRange.location = 0;
        selectedRange.length = 0;
    }
    _keyboardSelectedRange = NSMakeRange(selectedRange.location + _keyboardMarkedRange.location, selectedRange.length);

    if (markedText.length == 0)
    {
        _keyboardMarkedRange = NSMakeRange(NSNotFound, 0);
    }

    showKeyboardString();
}

- (void)unmarkText
{
    textInputLog(@"unmarkText");

    if (_keyboardMarkedRange.location == NSNotFound)
        return;

    _keyboardMarkedRange.location = NSNotFound;

    [self updateTextEditSelection];

    showKeyboardString();
}

#pragma mark UITextInput - Computing Text Ranges and Text Positions

- (UITextPosition *)beginningOfDocument
{
    return [IndexedPosition positionWithIndex:0];
}

- (UITextPosition *)endOfDocument
{
    return [IndexedPosition positionWithIndex:_keyboardString.length];
}

- (UITextRange *)textRangeFromPosition:(UITextPosition *)fromPosition toPosition:(UITextPosition *)toPosition
{
    IndexedPosition *from = (IndexedPosition *)fromPosition;
    IndexedPosition *to = (IndexedPosition *)toPosition;
    NSRange range = NSMakeRange(MIN(from.index, to.index), ABS(to.index - from.index));
    return [IndexedRange rangeWithNSRange:range];

}

- (UITextPosition *)positionFromPosition:(UITextPosition *)position offset:(NSInteger)offset
{
    IndexedPosition *pos = (IndexedPosition *)position;
    NSInteger end = pos.index + offset;
    if (end > _keyboardString.length || end < 0)
        return nil;

    return [IndexedPosition positionWithIndex:end];
}

- (UITextPosition *)positionFromPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction offset:(NSInteger)offset
{
    IndexedPosition *pos = (IndexedPosition *)position;
    NSInteger newPos = pos.index;

    switch (direction) {
        case UITextLayoutDirectionRight:
            newPos += offset;
            break;
        case UITextLayoutDirectionLeft:
            newPos -= offset;
            break;
        case UITextLayoutDirectionUp:
        case UITextLayoutDirectionDown:
        default:
            break;
    }

    if (newPos < 0)
        newPos = 0;

    if (newPos > _keyboardString.length)
        newPos = _keyboardString.length;

    return [IndexedPosition positionWithIndex:newPos];
}

#pragma mark UITextInput - Evaluating Text Positions

- (NSComparisonResult)comparePosition:(UITextPosition *)position toPosition:(UITextPosition *)other
{
    IndexedPosition *pos = (IndexedPosition *)position;
    IndexedPosition *o = (IndexedPosition *)other;

    if (pos.index == o.index) {
        return NSOrderedSame;
    } if (pos.index < o.index) {
        return NSOrderedAscending;
    } else {
        return NSOrderedDescending;
    }
}

- (NSInteger)offsetFromPosition:(UITextPosition *)from toPosition:(UITextPosition *)toPosition
{
    IndexedPosition *f = (IndexedPosition *)from;
    IndexedPosition *t = (IndexedPosition *)toPosition;
    return (t.index - f.index);
}

#pragma mark UITextInput - Text Input Delegate and Text Input Tokenizer

- (id <UITextInputTokenizer>)tokenizer
{
    if (_tokenizer == nil)
    {
        _tokenizer = [[UITextInputStringTokenizer alloc] initWithTextInput:self];
    }
    return _tokenizer;
}

#pragma mark UITextInput - Text Layout, writing direction and position related methods

- (UITextPosition *)positionWithinRange:(UITextRange *)range farthestInDirection:(UITextLayoutDirection)direction
{
    IndexedRange *r = (IndexedRange *)range;
	NSInteger pos;

    switch (direction) {
        case UITextLayoutDirectionUp:
        case UITextLayoutDirectionLeft:
            pos = r.range.location;
            break;
        case UITextLayoutDirectionRight:
        case UITextLayoutDirectionDown:
            pos = r.range.location + r.range.length;
            break;
    }

    return [IndexedPosition positionWithIndex:pos];
}

- (UITextRange *)characterRangeByExtendingPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction
{
    IndexedPosition *pos = (IndexedPosition *)position;
	NSRange result;

    switch (direction) {
        case UITextLayoutDirectionUp:
        case UITextLayoutDirectionLeft:
            result = NSMakeRange(pos.index - 1, 1);
            break;
        case UITextLayoutDirectionRight:
        case UITextLayoutDirectionDown:
            result = NSMakeRange(pos.index, 1);
            break;
    }

    return [IndexedRange rangeWithNSRange:result];
}

- (UITextWritingDirection)baseWritingDirectionForPosition:(UITextPosition *)position inDirection:(UITextStorageDirection)direction
{
    return UITextWritingDirectionLeftToRight;
}

- (void)setBaseWritingDirection:(UITextWritingDirection)writingDirection forRange:(UITextRange *)range
{
}

#pragma mark UITextInput - Geometry methods

- (CGRect)firstRectForRange:(UITextRange *)range
{
    return CGRectZero;  // @@
}

- (CGRect)caretRectForPosition:(UITextPosition *)position
{
    return CGRectZero;  // @@
}

#pragma mark UITextInput - Hit testing

- (UITextPosition *)closestPositionToPoint:(CGPoint)point
{
    xsMachine *the = _the;
    NSUInteger offset = 0;

    xsBeginHost(the);
    xsVars(1);
    xsVar(0) = xsCall2(_obj, xsID("pointToOffset"), xsInteger(point.x), xsInteger(point.y));
    offset = xsToInteger(xsVar(0));
    (void)xsEndHost(the);

    return [IndexedPosition positionWithIndex:offset];
}

#if defined(MAC_OS_X_VERSION_10_6) && (MAC_OS_X_VERSION_10_6 <= MAC_OS_X_VERSION_MAX_ALLOWED)
- (NSArray *)selectionRectsForRange:(UITextRange *)range
{
	return nil;	// @@
}
#endif

- (UITextPosition *)closestPositionToPoint:(CGPoint)point withinRange:(UITextRange *)range
{
    return nil; // @@
}

- (UITextRange *)characterRangeAtPoint:(CGPoint)point
{
    return nil; // @@
}
#endif

#pragma mark UIKeyInput

- (BOOL)hasText
{
#if TEXT_INPUT_SYSTEM
    return (_keyboardString.length != 0);
#else
    return NO;
#endif
}

- (void)insertText:(NSString *)text
{
    textInputLog(@"insertText:%@", text);

#if TEXT_INPUT_SYSTEM
    if (_keyboardMarkedRange.location != NSNotFound) {
        [self insertString:text withSelection:_keyboardMarkedRange];

        _keyboardSelectedRange.location = _keyboardMarkedRange.location + text.length;
        _keyboardSelectedRange.length = 0;
        _keyboardMarkedRange = NSMakeRange(NSNotFound, 0);

    } else if (_keyboardSelectedRange.length > 0) {
        [self insertString:text withSelection:_keyboardSelectedRange];

        _keyboardSelectedRange.length = 0;
        _keyboardSelectedRange.location += text.length;

    } else {
        [self insertString:text withSelection:_keyboardSelectedRange];

        _keyboardSelectedRange.location += text.length;
    }

    showKeyboardString();

    /* updating text edit */
    [self updateTextEditSelection];
#else
    int i;
    char charCode;

    for (i = 0; i < text.length; i++)
    {
        charCode = (char)[text characterAtIndex:i];
        [self throwKeyEvent:charCode keyDown:YES];
        [self throwKeyEvent:charCode keyDown:NO];
    }
#endif
}

- (void)deleteBackward
{
    textInputLog(@"deleteBackward");

#if TEXT_INPUT_SYSTEM
    if (_keyboardMarkedRange.location != NSNotFound) {
        [self insertString:nil withSelection:_keyboardMarkedRange];

        _keyboardSelectedRange.location = _keyboardMarkedRange.location;
        _keyboardSelectedRange.length = 0;
        _keyboardMarkedRange = NSMakeRange(NSNotFound, 0);

    } else if (_keyboardSelectedRange.length > 0) {
        [self insertString:nil withSelection:_keyboardSelectedRange];

        _keyboardSelectedRange.length = 0;

    } else if (_keyboardSelectedRange.location > 0) {
        /* updating local string */
        _keyboardSelectedRange.location--;
        _keyboardSelectedRange.length = 1;

        [self insertString:nil withSelection:_keyboardSelectedRange];

        _keyboardSelectedRange.length = 0;
    }

    showKeyboardString();
#else
    char delCode = 0x08;

    [self throwKeyEvent:delCode keyDown:YES];
    [self throwKeyEvent:delCode keyDown:NO];
#endif
}

#pragma mark --- Private ---

- (CGRect)cgBounds
{
    CGRect cgBounds = self.bounds;
    CGFloat scale;

    scale = self.contentScaleFactor;
    if (1.0 < scale) {
        cgBounds.origin.x *= scale;
        cgBounds.origin.y *= scale;
        cgBounds.size.width *= scale;
        cgBounds.size.height *= scale;
    }
    return cgBounds;
}

#if FSKBITMAP_OPENGL
- (BOOL)createFramebuffer
{
	GLint backingWidth;
	GLint backingHeight;

	// Generate IDs for a framebuffer object and a color renderbuffer
	glGenFramebuffers(1, &viewFramebuffer);
	glGenRenderbuffers(1, &viewRenderbuffer);

	LOGD("createFramebuffer: fb=%d rb=%d", viewFramebuffer, viewRenderbuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
	// This call associates the storage for the current render buffer with the EAGLDrawable (our CAEAGLLayer)
	// allowing us to draw into a buffer that will later be rendered to screen wherever the layer is (which corresponds with our view).
	[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(id<EAGLDrawable>)self.layer];
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, viewRenderbuffer);

	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
		return NO;
	}

	// Notify FskGLBlit that we are changing the screen frame buffer. We need to be very careful that the state that we leave it in is consistent with FskGLBlit's notion.
	// FskGLSetDefaultFBO() has no side effects, but FskGLBitmapTextureTargetSet() assumes that there is only 1 FBO, to which it attaches textures as needed.
	// Calling glBindFramebuffer(GL_FRAMEBUFFER, 0) directs GL to render to the "screen" rather than an FBO, to assure that no foreign (i.e. second) FBO is currently bound,
	// We do this to assure that FskGLSetDefaultFBO() has no undesirable side effects to the previously bound FBO.
	// However, FskGLBitmapTextureTargetSet() may cause errors because it may execute functions that are only appropriate for an FBO, rather than the screen.
	// We clear this sticky error, because we don't consider this to be an error, and don't want to mess things up later by reporting an error when there was none.
	// Finally, we bind the viewFramebuffer, because that is the state that FskGLBitmapTextureTargetSet() thinks that it was left in, and all subsequent
	// FskGLBlit calls will think as well.
	FskGLSetDefaultFBO(viewFramebuffer);				// This merely changes a state variable, telling FskGLBlit that this would be the destination when FskGLBitmapTextureTargetSet(NULL) is called.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);				// Choose the default frame buffer (which has no texture nor render buffer), to avoid tossing the FBO texture.
	FskGLBitmapTextureTargetSet(NULL);					// Notify FskGLBlit that we are setting the target to the default framebuffer.
	(void)glGetError();									// Clear any recently generated errors
	glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);	// Make sure that the bound frame buffer is consistent with FskGLBlit's notion.

	return YES;
}

// Clean up any buffers we have allocated.
- (void)destroyFramebuffer
{
	LOGD("destroyFramebuffer: fb=%d rb=%d", viewFramebuffer, viewRenderbuffer);
	glDeleteFramebuffers(1, &viewFramebuffer);
	viewFramebuffer = 0;
	glDeleteRenderbuffers(1, &viewRenderbuffer);
	viewRenderbuffer = 0;
}
#endif

#if defined(SAVE_IMAGE)
static NSUInteger sIndex = 0;

- (void)saveImage:(CGImageRef)imageRef name:(NSString*)name
{
    NSString *name2 = [NSString stringWithFormat:@"%03d%@.png", (int)sIndex++, name];
    if (imageRef == NULL) {
        //NSLog(@"saveImage: imageRef is NULL");
        return;
    }
    UIImage *uiImage = [UIImage imageWithCGImage:imageRef];
    if (uiImage == nil) {
        //NSLog(@"saveImage: uiImage is nil");
        return;
    }
    NSData *data = UIImagePNGRepresentation(uiImage);
    if (data == nil) {
        //NSLog(@"saveImage: data is nil");
        return;
    }
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    [data writeToFile:[(NSString *)[paths lastObject] stringByAppendingPathComponent:name2] atomically:NO];
}
#endif

@end

#if TEXT_INPUT_SYSTEM
#pragma mark IndexedPosition implementation

@implementation IndexedPosition

@synthesize index = _index;

+ (IndexedPosition *)positionWithIndex:(NSUInteger)index
{
    IndexedPosition *pos = [[IndexedPosition alloc] init];
    pos.index = index;
    return [pos autorelease];
}

@end

#pragma mark IndexedRange implementation

@implementation IndexedRange

@synthesize range = _range;

+ (IndexedRange *)rangeWithNSRange:(NSRange)theRange
{
    if (theRange.location == NSNotFound)
        return nil;

    IndexedRange *range = [[IndexedRange alloc] init];
    range.range = theRange;
    return [range autorelease];
}

- (UITextPosition *)start
{
    return [IndexedPosition positionWithIndex:self.range.location];
}

- (UITextPosition *)end
{
	return [IndexedPosition positionWithIndex:(self.range.location + self.range.length)];
}

-(BOOL)isEmpty
{
    return (self.range.length == 0);
}

@end
#endif
