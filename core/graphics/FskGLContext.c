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
/**
	\file	FskGLContext.c
	\brief	Device-dependent Open GL Context creation.
*/

#define __FSKGLBLIT_PRIV__
#include "FskGLContext.h"
#include "FskGLBlit.h"
#include "FskMemory.h"
#include "FskPixelOps.h"

#if FSKBITMAP_OPENGL



#ifdef __APPLE__
	#if TARGET_OS_IPHONE
		#if GLES_VERSION == 1
			#include <OpenGLES/ES1/gl.h>
		#else /* GLES_VERSION == 2 */
			#include <OpenGLES/ES2/gl.h>
			#include <OpenGLES/ES2/glext.h>
		#endif /* GLES_VERSION == 2 */
		#include "FskCocoaSupportPhone.h"
	#else /* !TARGET_OS_IPHONE */
		#include <OpenGL/gl.h>			// Header File For The OpenGL Library
	#endif /* !TARGET_OS_IPHONE */
#elif TARGET_OS_ANDROID || TARGET_OS_KPL || defined(__linux__) || (FSK_OPENGLES_ANGLE == 1)
	#define GL_GLEXT_PROTOTYPES
	#ifndef GLES_VERSION
		#include "FskGLBlit.h"
	#endif /* GLES_VERSION */
	#if GLES_VERSION == 2
		#include <GLES2/gl2.h>		// Header File For the GLES 2 Library
		#include <GLES2/gl2ext.h>	// Header file for standard GLES 2 extensions.
	#else /* GLES_VERSION == 1 */
		#include <GLES/gl.h>		// Header File For the GLES 1 Library
		#include <GLES/glext.h>		// Header file for standard GLES 1 extensions.
	#endif /* GLES_VERSION */
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
	#include <EGL/eglplatform.h>
	#define __FSKWINDOW_PRIV__
	#include "FskWindow.h"
	#if TARGET_OS_ANDROID
		#include <android/native_window.h>
	#endif /* android */
	#if TARGET_OS_KPL
		#include "KplGL.h"
	#endif /* TARGET_OS_KPL */
#elif defined(_MSC_VER)
	#include <gl/gl.h>				// Header File For The OpenGL32 Library
#else
	#error unknown OS
#endif /* OS */
#if (FSK_OPENGLES_ANGLE == 1) || !(defined(GL_VERSION_1_1) || defined(GL_VERSION_2_0) )	/* If either ANGLE or GLES */
	#undef GL_UNPACK_SKIP_ROWS		/* Neither the ANGLE library nor GL ES supports calling glPixelStorei() with these values */
	#undef GL_UNPACK_SKIP_PIXELS	/* but some GLES header files define them anyway */
	#undef GL_UNPACK_ROW_LENGTH
#endif /* (FSK_OPENGLES_ANGLE == 1) */


#if SUPPORT_INSTRUMENTATION

	#define LOG_PARAMETERS
	//#define LOG_ALL

	#include "FskInstrumentation.h"
	#define GLCONTEXT_DEBUG	1
	#define FskInstrumentedTypeEnabled(type, level)	FskInstrumentedTypeHasListenersForLevel(FskInstrumentedGlobalType(type), level)	/**< Whether instrumentation will print anything. */
	#define FskInstrumentedGlobalType(type)			(&(g##type##TypeInstrumentation))												/**< The data structure created with FskInstrumentedSimpleType(). */
#else /* !SUPPORT_INSTRUMENTATION */
	#define FskInstrumentedTypeEnabled(type, level)	(0)																				/**< Instrumentation will not print anything. */
#endif /* SUPPORT_INSTRUMENTATION */

FskInstrumentedSimpleType(GLContext, glcontext);												/**< This declares the types needed for instrumentation. */

#if GLCONTEXT_DEBUG
	#define	LOGD(...)	FskGLContextPrintfDebug(__VA_ARGS__)									/**< Print debugging logs. */
	#define	LOGI(...)	FskGLContextPrintfVerbose(__VA_ARGS__)									/**< Print information logs. */
#endif	/* GLCONTEXT_DEBUG */
#define		LOGE(...)	FskGLContextPrintfMinimal(__VA_ARGS__)									/**< Print error logs always, when instrumentation is on. */
#ifndef     LOGD
	#define LOGD(...)																			/**< Don't print debugging logs. */
#endif   	/* LOGD */
#ifndef     LOGI
	#define LOGI(...)																			/**< Don't print information logs. */
#endif   	/* LOGI */
#ifndef     LOGE
	#define LOGE(...)																			/**< Don't print error logs. */
#endif   /* LOGE */
#define LOGD_ENABLED()	FskInstrumentedTypeEnabled(GLContext, kFskInstrumentationLevelDebug)	/**< Whether LOGD() will print anything. */
#define LOGI_ENABLED()	FskInstrumentedTypeEnabled(GLContext, kFskInstrumentationLevelVerbose)	/**< Whether LOGI() will print anything. */
#define LOGE_ENABLED()	FskInstrumentedTypeEnabled(GLContext, kFskInstrumentationLevelMinimal)	/**< Whether LOGE() will print anything. */



#ifdef LOG_PARAMETERS
static void LogFskGLContext(FskConstGLContext ctx, const char *name);
static void LogFskGLOffscreenContextNew(UInt32 width, UInt32 height, FskBitmapFormatEnum pixelFormat, UInt32 version, FskGLContext share, FskGLContext *pCtx) {
	LOGD("FskGLOffscreenContextNew(width=%u height=%u pixelFormat=%s version=%u share=%p pCtx=%p)",
		(unsigned)width, (unsigned)height, FskBitmapFormatName(pixelFormat), (unsigned)version, share, pCtx);
}
static void LogFskGLContextDispose(FskGLContext ctx, Boolean terminateGL) {
	LOGD("FskGLContextDispose(ctx=%p terminateGL=%d)", ctx, terminateGL);
	LogFskGLContext(ctx, NULL);
}
static void LogFskGLContextFrameBuffer(FskConstGLContext ctx) {
	LOGD("FskGLBlitContextSet(ctx=%p)", ctx);
	LogFskGLContext(ctx, NULL);
}
static void LogFskGLContextMakeCurrent(FskConstGLContext ctx) {
	LOGD("FskGLContextMakeCurrent(ctx=%p)", ctx);
	LogFskGLContext(ctx, NULL);
}

#ifndef EGL_VERSION
#ifndef __FSKTESTUTILS__
typedef struct LookupEntry {
	int			code;
	const char	*name;
} LookupEntry;
static const char* LookupNameFromCode(const LookupEntry *table, int code) {
	for (; table->name != NULL; ++table)
		if (table->code == code)
			return table->name;
	return "UNKNOWN";
}
#endif /* __FSKTESTUTILS__*/

static const char* AttachmentTypeFromCode(int code) {
	const LookupEntry lut[] = {
		{ GL_TEXTURE,		"GL_TEXTURE"		},
		{ GL_RENDERBUFFER,	"GL_RENDERBUFFER"	},
		{ GL_NONE,			"GL_NONE"			},
		{ 0,				NULL,				}
	};
	return LookupNameFromCode(lut, code);
}
#endif /* EGL_VERSION */
#endif /* LOG_PARAMETERS */



#if 0
#pragma mark ======== EGL ========
#endif
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
#ifdef EGL_VERSION		/******** EGL - Android - Linux - Kpl ******** EGL - Android - Linux - Kpl ******** EGL - Android - Linux - Kpl *********/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/


struct FskGLContextRecord {
	EGLDisplay	display;
	EGLSurface	surface;
	EGLContext	context;
	GLuint		framebuffer;
	const char*	format;
};

#ifdef LOG_PARAMETERS /* EGL */
static void LogFskGLContext(FskConstGLContext ctx, const char *name) {
	if (!ctx)	return;
	if (!name)	name = "context";
	LOGD("\t%s(type=EGL display=%p surface=%p context=%p framebuffer=%u format=%s)", name, ctx->display, ctx->surface, ctx->context, ctx->framebuffer, ctx->format);
}
#endif /* LOG_PARAMETERS */


/********************************************************************************
 * FskGLOffscreenContextNew - EGL
 ********************************************************************************/

FskErr FskGLOffscreenContextNew(UInt32 width, UInt32 height, FskBitmapFormatEnum pixelFormat, UInt32 version, FskGLContext share, FskGLContext *pCtx) {
	FskErr			err	= kFskErrNone;
	FskGLContext	ctx;
	EGLConfig		cfg;
	EGLint			configAttribs[24], contextAttribs[4], pbufAttribs[6], *a;

	#ifdef LOG_PARAMETERS
		LogFskGLOffscreenContextNew(width, height, pixelFormat, version, share, pCtx);
	#endif /* LOG_PARAMETERS */

	/* Specify configuration attributes */
	a = configAttribs;
	*a++ = EGL_SURFACE_TYPE;			*a++ = EGL_PBUFFER_BIT;											/*  2 */
	*a++ = EGL_COLOR_BUFFER_TYPE;		*a++ = EGL_RGB_BUFFER;											/*  4 */
	*a++ = EGL_DEPTH_SIZE;				*a++ = 0;														/*  6 */
	*a++ = EGL_STENCIL_SIZE;			*a++ = 0;														/*  8 */
	*a++ = EGL_SAMPLES;					*a++ = 0;														/* 10 */
	*a++ = EGL_RENDERABLE_TYPE;			*a++ = (version == 1) ? EGL_OPENGL_ES_BIT : EGL_OPENGL_ES2_BIT;	/* 12 */
	*a++ = EGL_BUFFER_SIZE;				*a++ = FskBitmapFormatDepth(pixelFormat);						/* 14 */
	*a++ = EGL_ALPHA_SIZE;				*a++ = FskBitmapFormatAlphaBits(pixelFormat);					/* 16 */
	*a++ = EGL_RED_SIZE;				*a++ = FskBitmapFormatRedBits(pixelFormat);						/* 18 */
	*a++ = EGL_GREEN_SIZE;				*a++ = FskBitmapFormatGreenBits(pixelFormat);					/* 20 */
	*a++ = EGL_BLUE_SIZE;				*a++ = FskBitmapFormatBlueBits(pixelFormat);					/* 22 */
	*a++ = EGL_NONE;					*a   = EGL_NONE;												/* 24 */

	/* Specify context attributes */
	a = contextAttribs;
	*a++ = EGL_CONTEXT_CLIENT_VERSION;	*a++ = version;													/*  2 */
	*a++ = EGL_NONE;					*a   = EGL_NONE;												/*  4 */

	/* Specify PBuffer attributes */
	a = pbufAttribs;
	if (width && height) {
		*a++ = EGL_WIDTH;				*a++ = width;													/*  2 */
		*a++ = EGL_HEIGHT;				*a++ = height;													/*  4 */
	}
	*a++ = EGL_NONE;					*a   = EGL_NONE;												/*  6 */

	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(**pCtx), pCtx));
	ctx = *pCtx;
	ctx->format = (FskBitmapFormatDepth(pixelFormat) == 16) ? "RGB565-Pbuffer" : "RGBA8888-Pbuffer";
	BAIL_IF_FALSE(EGL_NO_DISPLAY != (ctx->display = eglGetDisplay((NativeDisplayType)EGL_DEFAULT_DISPLAY)),	err, kFskErrEGLBadDisplay);
	BAIL_IF_FALSE(                   eglInitialize(ctx->display, NULL, NULL),								err, kFskErrEGLBadDisplay);
	BAIL_IF_NULL(                   (cfg = FskChooseBestEGLConfig(ctx->display, &configAttribs[0])),		err, kFskErrUnsupportedPixelType);
	ctx->surface = eglCreatePbufferSurface(ctx->display, cfg, pbufAttribs);
	if ((EGL_NO_SURFACE == ctx->surface) && (width == 0) && (height == 0)) {
		a = pbufAttribs;
		pbufAttribs[0] = EGL_WIDTH;
		pbufAttribs[1] = 16;
		pbufAttribs[2] = EGL_HEIGHT;
		pbufAttribs[3] = 16;
		pbufAttribs[4] = EGL_NONE;
		ctx->surface = eglCreatePbufferSurface(ctx->display, cfg, pbufAttribs);
	}
	BAIL_IF_FALSE(EGL_NO_SURFACE != ctx->surface, err, kFskErrEGLBadSurface);
	BAIL_IF_FALSE(EGL_NO_CONTEXT != (ctx->context = eglCreateContext(ctx->display, cfg, (share ? share->context : NULL), contextAttribs)),	err, kFskErrEGLBadContext);

bail:
	if (err) {	FskGLContextDispose(*pCtx, false); *pCtx = NULL; }
	return err;
}


/********************************************************************************
 * FskGLContextNewFromEGL - EGL
 ********************************************************************************/

FskErr FskGLContextNewFromEGL(void* display, void* surface, void* context, FskGLContext *pCtx) {
	FskErr			err;

	#ifdef LOG_PARAMETERS
		LOGD("FskGLOffscreenContextNew(display=%p surface=%p context=%p pCtx=%p)", display, surface, context, pCtx);
	#endif /* LOG_PARAMETERS */

	BAIL_IF_FALSE(EGL_NO_CONTEXT != context, err, kFskErrEGLBadContext);							/* Parameter validation */
	BAIL_IF_FALSE(EGL_NO_DISPLAY != display, err, kFskErrEGLBadDisplay);
	BAIL_IF_FALSE(EGL_NO_SURFACE != surface, err, kFskErrEGLBadSurface);
	BAIL_IF_NULL(pCtx, err, kFskErrInvalidParameter);
	*pCtx = NULL;

	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(**pCtx), pCtx));										/* Make and initialize an FskGLContext */
	(**pCtx).display = display;
	(**pCtx).surface = surface;
	(**pCtx).context = context;

bail:
	return err;
}


/********************************************************************************
 * FskGLContextNewFromCurrentContext - EGL
 ********************************************************************************/

FskErr FskGLContextNewFromCurrentContext(FskGLContext *pCtx) {
	FskErr		err;
	EGLDisplay	display;
	EGLSurface	surface;
	EGLContext	context;

	#ifdef LOG_PARAMETERS
		LOGD("FskGLContextNewFromCurrentContext(pCtx=%p)", pCtx);
	#endif /* LOG_PARAMETERS */

	display	= eglGetCurrentDisplay();																/* Get the current display, surface and context */
	surface = eglGetCurrentSurface(EGL_DRAW);
	context = eglGetCurrentContext();
	BAIL_IF_FALSE(EGL_NO_SURFACE != surface, err, kFskErrEGLCurrentSurface);						/* Validate that we have a current surface */

	err = FskGLContextNewFromEGL(display, surface, context, pCtx);									/* We validate the display and context here */

bail:
	return err;
}


/********************************************************************************
 * FskGLContextGetCurrentContext - EGL
 ********************************************************************************/

FskErr FskGLContextGetCurrentContext(FskGLContextStorage *storage, FskGLContext *pCtx) {
	FskErr			err		= kFskErrNone;
	FskGLContext	ctx		= NULL;
	EGLContext		context;
	EGLDisplay		display;
	EGLSurface		surface;

	#ifdef LOG_PARAMETERS
		LOGD("FskGLContextGetCurrentContext(storage=%p pCtx=%p)", storage, pCtx);
	#endif /* LOG_PARAMETERS */

	FskMemSet(storage, 0, sizeof(*storage));
	display	= eglGetCurrentDisplay();																/* Get the current display, surface and context */
	surface = eglGetCurrentSurface(EGL_DRAW);
	context = eglGetCurrentContext();
	BAIL_IF_FALSE(EGL_NO_CONTEXT != context && EGL_NO_DISPLAY != display && EGL_NO_SURFACE != surface, err, kFskErrNotAccelerated);
	ctx = (FskGLContext)storage;
	ctx->display = display;
	ctx->surface = surface;
	ctx->context = context;

bail:
	if (pCtx)	*pCtx = ctx;
	return err;
}


/********************************************************************************
 * FskGLContextDispose - EGL
 ********************************************************************************/

void FskGLContextDispose(FskGLContext ctx, Boolean terminateGL) {
	#ifdef LOG_PARAMETERS
		LogFskGLContextDispose(ctx, terminateGL);
	#endif /* LOG_PARAMETERS */

	if (ctx) {
		if (ctx->display) {
			eglMakeCurrent(ctx->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			if (ctx->context)	eglDestroyContext(ctx->display, ctx->context);
			if (ctx->surface)	eglDestroySurface(ctx->display, ctx->surface);
		}
		if (terminateGL)
			eglTerminate(ctx->display);
		FskMemPtrDispose(ctx);
	}
}


/********************************************************************************
 * FskGLContextGetEGL
 ********************************************************************************/

void FskGLContextGetEGL(FskGLContext ctx, void** pDisplay, void** pSurface, void** pContext) {
	#ifdef LOG_PARAMETERS
		LOGD("FskGLContextGetEGL(ctx=%p pDisplay=%p pSurface=%p pContex=%p)", ctx, pDisplay, pSurface, pContext);
	#endif /* LOG_PARAMETERS */

	if (pDisplay)	*pDisplay = ctx->display;
	if (pSurface)	*pSurface = ctx->surface;
	if (pContext)	*pContext = ctx->context;
}


/********************************************************************************
 * FskGLContextFrameBuffer - EGL
 ********************************************************************************/

unsigned FskGLContextFrameBuffer(FskConstGLContext ctx) {
	#ifdef LOG_PARAMETERS
		LogFskGLContextFrameBuffer(ctx);
	#endif /* LOG_PARAMETERS */
	return ctx ? ctx->framebuffer : 0;
}


/********************************************************************************
 * FskGLContextMakeCurrent - EGL
 ********************************************************************************/

FskErr FskGLContextMakeCurrent(FskConstGLContext ctx) {
	FskErr err;
	#ifdef LOG_PARAMETERS
		LogFskGLContextMakeCurrent(ctx);
	#endif /* LOG_PARAMETERS */
	err = ctx	? eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, ctx->context)
				: eglMakeCurrent(eglGetDisplay((NativeDisplayType)EGL_DEFAULT_DISPLAY), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	BAIL_IF_FALSE(err, err, kFskErrGraphicsContext);
	err = kFskErrNone;
bail:
	return err;
}


#if 0
#pragma mark ======== Mac ========
#endif
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE	/******** Mac ******** Mac ******** Mac ******** Mac ******** Mac ******** Mac ******** Mac *********/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/

#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/OpenGL.h>

struct FskGLContextRecord {
	CGLContextObj	context;
	GLuint			framebuffer;
	GLuint			fbTexture;
	GLuint			attachmentType;
};


#ifdef LOG_PARAMETERS
static void LogFskGLContext(FskConstGLContext ctx, const char *name) {
	if (!ctx)	return;
	if (!name)	name = "context";
	LOGD("\t%s(type=MAC CGLcontext=%p framebuffer=%u attachmentType=%s fbTexture=#%u)",
		name, ctx->context, ctx->framebuffer, AttachmentTypeFromCode(ctx->attachmentType), ctx->fbTexture);
}
#endif /* LOG_PARAMETERS */


/** Return an error string decoding a CGL Error.
 **	\param[in]	the error code.
 **	\return		the corresponding C string.
 **/
#define CGLErrorStringFromCode(c)	CGLErrorString(c)


/********************************************************************************
 * FskErrorFromCGLError
 ********************************************************************************/

static FskErr FskErrorFromCGLError(CGLError cglErr) {
	struct FskCglErr { CGLError cgl; FskErr fsk; };
	static const struct FskCglErr tab[] = {
		{	kCGLNoError,			kFskErrNone					},
		{	kCGLBadAttribute,		kFskErrCGLBadAttribute		},
		{	kCGLBadProperty,		kFskErrCGLBadProperty		},
		{	kCGLBadPixelFormat,		kFskErrCGLBadPixelFormat	},
		{	kCGLBadRendererInfo,	kFskErrCGLBadRendererInfo	},
		{	kCGLBadContext,			kFskErrCGLBadContext		},
		{	kCGLBadDrawable,		kFskErrCGLBadDrawable		},
		{	kCGLBadDisplay,			kFskErrCGLBadDisplay		},
		{	kCGLBadState,			kFskErrCGLBadState			},
		{	kCGLBadValue,			kFskErrCGLBadValue			},
		{	kCGLBadMatch,			kFskErrCGLBadMatch			},
		{	kCGLBadEnumeration,		kFskErrCGLBadEnumeration	},
		{	kCGLBadOffScreen,		kFskErrCGLBadOffScreen		},
		{	kCGLBadFullScreen,		kFskErrCGLBadFullScreen		},
		{	kCGLBadWindow,			kFskErrCGLBadWindow			},
		{	kCGLBadAddress,			kFskErrCGLBadAddress		},
		{	kCGLBadCodeModule,		kFskErrCGLBadCodeModule		},
		{	kCGLBadAlloc,			kFskErrCGLBadAlloc			},
		{	kCGLBadConnection,		kFskErrCGLBadConnection		}
	};
	const struct FskCglErr *p;
	for (p = tab; p < &tab[sizeof(tab) / sizeof(tab[0])]; ++p)
		if (cglErr == p->cgl)
			return p->fsk;
	return kFskErrUnknown;
}


/********************************************************************************
 * FskGLOffscreenContextNew - Mac
 ********************************************************************************/

FskErr FskGLOffscreenContextNew(UInt32 width, UInt32 height, FskBitmapFormatEnum pixelFormat, UInt32 version, FskGLContext share, FskGLContext *pCtx) {
	CGLContextObj			saveContext	= CGLGetCurrentContext();
	CGLError				cglErr		= kCGLNoError;
	FskErr					err			= kFskErrNone;
	CGLPixelFormatAttribute attributes[6];
	CGLPixelFormatObj		pix;
	GLint					num;
	FskGLContext			ctx;

	#ifdef LOG_PARAMETERS
		LogFskGLOffscreenContextNew(width, height, pixelFormat, version, share, pCtx);
	#endif /* LOG_PARAMETERS */

	/* Specify pixel format attributes */
	attributes[0] = kCGLPFAAccelerated;													/* No software rendering */
	attributes[1] = kCGLPFAColorSize;
	attributes[2] = (CGLPixelFormatAttribute)(	FskBitmapFormatRedBits(pixelFormat)   +
												FskBitmapFormatGreenBits(pixelFormat) +
												FskBitmapFormatBlueBits(pixelFormat));	/* Color bits */
	attributes[3] = kCGLPFAAlphaSize;
	attributes[4] = FskBitmapFormatAlphaBits(pixelFormat);								/* Alpha bits */
	attributes[5] = (CGLPixelFormatAttribute)0;											/* Terminator */

	*pCtx = NULL;
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(**pCtx), pCtx));
	ctx = *pCtx;
	BAIL_IF_ERR((FskErr)(cglErr = CGLChoosePixelFormat(attributes, &pix, &num)));
	BAIL_IF_ERR((FskErr)(cglErr = CGLCreateContext(pix, (share ? share->context : NULL), &ctx->context)));
	CGLDestroyPixelFormat(pix);
	BAIL_IF_ERR((FskErr)(cglErr = CGLSetCurrentContext(ctx->context)));


	if (width && height) {
		unsigned	glFormat, glType;
		int			glInternalFormat;

		BAIL_IF_ERR(err = FskGLGetFormatAndTypeFromPixelFormat(pixelFormat, &glFormat, &glType, &glInternalFormat));

		glGenFramebuffers(1, &ctx->framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, ctx->framebuffer);

		glGenTextures(1, &ctx->fbTexture);
		glBindTexture(GL_TEXTURE_2D, ctx->fbTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, width, height, 0, glFormat, glType, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->fbTexture, 0);
		num = glCheckFramebufferStatus(GL_FRAMEBUFFER);												/* Check the frame buffer status */
		if (GL_FRAMEBUFFER_COMPLETE == num)	err = kFskErrNone;
		else								BAIL(FskErrorFromGLError(num));
		ctx->attachmentType = GL_TEXTURE;															/* Indicate that a texture has been attached to the framebuffer */
	}

	/* err =*/ FskErrorFromGLError(glGetError());

bail:
	CGLSetCurrentContext(saveContext);
	if (kCGLNoError != cglErr)	err = FskErrorFromCGLError(cglErr);
	if (err) {					FskGLContextDispose(*pCtx, false); *pCtx = NULL; }
	return err;
}


/********************************************************************************
 * FskGLContextNewFromCGLContext - Mac
 ********************************************************************************/

FskErr FskGLContextNewFromCGLContext(CGLContextObj context, FskGLContext *pCtx) {
	CGLContextObj			saveContext	= CGLGetCurrentContext();
	CGLError				cglErr		= kCGLNoError;
	FskErr					err			= kFskErrNone;
	GLint					num;
	FskGLContext			ctx;

	#ifdef LOG_PARAMETERS
		LOGD("FskGLOffscreenContextNewFromCGLContext(context=%p)", context);
	#endif /* LOG_PARAMETERS */

	*pCtx = NULL;
	BAIL_IF_NULL(context, err, kFskErrInvalidParameter);
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(**pCtx), pCtx));
	ctx = *pCtx;
	ctx->context = context;
	BAIL_IF_ERR((FskErr)(cglErr = CGLSetCurrentContext(ctx->context)));

	/* Query for the currently bound framebuffer */
	num = -1;																			/* 0 is used for the screen, so -1 should be an invalid frame buffer name */
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &num);
	BAIL_IF_FALSE(-1 != num, err, kFskErrGLFramebufferUndefined);						/* There should be some framebuffer; don't know what else to do */
	ctx->framebuffer = num;

	/* Query for the type of object attached to the framebuffer, either GL_TEXTURE or GL_RENDERBUFFER. */
	num = 0;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &num);
	//BAIL_IF_TRUE(GL_NONE == num, err, kFskErrGLFramebufferIncompleteMissingAttachment);	/* Don't know what to do if there is no framebuffer attachment */
	ctx->attachmentType = num;

	/* Query for texture (or render buffer ) attached to the framebuffer */
	num = 0;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &num);
	//BAIL_IF_ZERO(num, err, kFskErrGLFramebufferIncompleteAttachment);					/* There should be something attached to the framebuffer */
	ctx->fbTexture = num;

	glGetError();
	//BAIL_IF_ERR(err = FskErrorFromGLError(glGetError()));								/* See if there were any GL errors */

bail:
	if (saveContext)	CGLSetCurrentContext(saveContext);
	if (kCGLNoError != cglErr)	err = FskErrorFromCGLError(cglErr);
	if (err) FskGLContextDispose(*pCtx, false);
	return err;
}


/********************************************************************************
 * FskGLContextNewFromCurrentContext - CGL
 ********************************************************************************/

FskErr FskGLContextNewFromCurrentContext(FskGLContext *pCtx) {
	FskErr			err;
	CGLContextObj	context;

	BAIL_IF_NULL(pCtx, err, kFskErrInvalidParameter);
	*pCtx = NULL;

	context = CGLGetCurrentContext();
	BAIL_IF_NULL(context, err, kFskErrCGLBadContext);

	err = FskGLContextNewFromCGLContext(context, pCtx);

bail:
	return err;
}


/********************************************************************************
 * FskGLContextGetCurrentContext - CGL
 ********************************************************************************/

FskErr FskGLContextGetCurrentContext(FskGLContextStorage *storage, FskGLContext *pCtx) {
	FskErr			err		= kFskErrNone;
	FskGLContext	ctx		= NULL;
	CGLContextObj	context;
	GLint			fb, att, buf;

	#ifdef LOG_PARAMETERS
		LOGD("FskGLContextGetCurrentContext(storage=%p pCtx=%p)", storage, pCtx);
	#endif /* LOG_PARAMETERS */

	FskMemSet(storage, 0, sizeof(*storage));
	context = CGLGetCurrentContext();
	#ifdef LOG_ALL
		LOGD("CGLGetCurrentContext() returns %p", context);
	#endif /* LOG_ALL */
	BAIL_IF_NULL(context, err, kFskErrNotAccelerated);
	fb = -1;	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fb);
	att = 0;	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &att);
	buf = 0;	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &buf);
	#ifdef LOG_ALL
		LOGD("FskGLContextGetCurrentContext; fb=%d att=%d buf=%d", fb, att, buf);
	#endif /* LOG_ALL */
	BAIL_IF_FALSE(-1 != fb, err, kFskErrNotAccelerated);
	ctx = (FskGLContext)storage;
	ctx->context		= context;
	ctx->framebuffer	= fb;
	ctx->attachmentType	= att;
	ctx->fbTexture		= buf;
	(void)glGetError();		/* Clear errors */

bail:
	if (pCtx)	*pCtx = ctx;
	return err;
}


/********************************************************************************
 * FskGLContextDispose - Mac
 ********************************************************************************/

void FskGLContextDispose(FskGLContext ctx, Boolean terminateGL) {
	#ifdef LOG_PARAMETERS
		LogFskGLContextDispose(ctx, terminateGL);
	#endif /* LOG_PARAMETERS */

	if (ctx) {
		CGLContextObj ocx = CGLGetCurrentContext();												/* Save current context */
		if (ocx == ctx->context)																/* If ctx is the current context, ... */
			ocx = NULL;																			/* ... then there is no context to restore to */
		CGLSetCurrentContext(ctx->context);														/* Set the current context in order to deallocate resources */
		if (ctx->framebuffer) {
			glBindFramebuffer(GL_FRAMEBUFFER, ctx->framebuffer);								/* Set the current frame buffer */
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);	/* Unbind the texture from the frame buffer */
			glBindFramebuffer(GL_FRAMEBUFFER, 0);												/* No current frame buffer */
			glDeleteFramebuffers(1, &ctx->framebuffer);											/* Dispose attached default frame buffer */
		}
		if (ctx->fbTexture) {
			glBindTexture(GL_TEXTURE_2D, 0);													/* Unbind ny textures */
			glDeleteTextures(1, &ctx->fbTexture);												/* Dispose attached frame buffer texture */
		}
		CGLDestroyContext(ctx->context);
		FskMemPtrDispose(ctx);
		CGLSetCurrentContext(ocx);																/* Restore context */
	}
}


/********************************************************************************
 * FskGLContexGetCGL
 ********************************************************************************/

void FskGLContexGetCGL(FskGLContext ctx, CGLContextObj *pContext) {
	*pContext = ctx->context;
}


/********************************************************************************
 * FskGLContextFrameBuffer - Mac
 ********************************************************************************/

unsigned FskGLContextFrameBuffer(FskConstGLContext ctx) {
	#ifdef LOG_PARAMETERS
		LogFskGLContextFrameBuffer(ctx);
	#endif /* LOG_PARAMETERS */
	return ctx ? ctx->framebuffer : 0;
}


/********************************************************************************
 * FskGLContextMakeCurrent - Mac
 ********************************************************************************/

FskErr FskGLContextMakeCurrent(FskConstGLContext ctx) {
	FskErr err;
	#ifdef LOG_PARAMETERS
		LogFskGLContextMakeCurrent(ctx);
	#endif /* LOG_PARAMETERS */
	BAIL_IF_ERR(err = FskErrorFromCGLError(CGLSetCurrentContext(ctx ? ctx->context : NULL)));
bail:
	return err;
}


#if 0
#pragma mark ======== iPhone ========
#endif
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
#elif TARGET_OS_MAC && TARGET_OS_IPHONE	/******** iPhone ******** iPhone ******** iPhone ******** iPhone ******** iPhone ******** iPhone ********/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/

struct FskGLContextRecord {
	EAGLContext	*context;
	GLuint		framebuffer;
	GLuint		fbTexture;
	GLuint		attachmentType;
};


#ifdef LOG_PARAMETERS
static void LogFskGLContext(FskConstGLContext ctx, const char *name) {
	if (!ctx)	return;
	if (!name)	name = "context";
	LOGD("\t%s(type=IOS EAGLcontext=%p framebuffer=%u attachmentType=%s fbTexture=#%u)",
		name, ctx->context, ctx->framebuffer, AttachmentTypeFromCode(ctx->attachmentType), ctx->fbTexture);
}
#endif /* LOG_PARAMETERS */


/********************************************************************************
 * FskGLOffscreenContextNew - iPhone
 ********************************************************************************/

FskErr FskGLOffscreenContextNew(UInt32 width, UInt32 height, FskBitmapFormatEnum pixelFormat, UInt32 version, FskGLContext share, FskGLContext *pCtx) {
	EAGLContext		*saveContext	= FskEAGLContextGetCurrent();
	FskGLContext	ctx;
	FskErr			err;

	#ifdef LOG_PARAMETERS
		LogFskGLOffscreenContextNew(width, height, pixelFormat, version, share, pCtx);
	#endif /* LOG_PARAMETERS */

	*pCtx = NULL;
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(**pCtx), pCtx));
	ctx = *pCtx;

	BAIL_IF_NULL(ctx->context = FskEAGLContextNew(version, (share ? share->context : NULL)), err, kFskErrEAGLBadContext);
	BAIL_IF_ERR(err = FskEAGLContextSetCurrent(ctx->context));

	if (width && height) {
		unsigned	glFormat, glType;
		int			glInternalFormat;

		BAIL_IF_ERR(err = FskGLGetFormatAndTypeFromPixelFormat(pixelFormat, &glFormat, &glType, &glInternalFormat));

		glGenFramebuffers(1, &ctx->framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, ctx->framebuffer);

		glGenTextures(1, &ctx->fbTexture);
		glBindTexture(GL_TEXTURE_2D, ctx->fbTexture);

		glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, width, height, 0, glFormat, glType, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->fbTexture, 0);
		ctx->attachmentType = GL_TEXTURE;															/* Indicate that a texture has been attached to the framebuffer */
	}

	err = FskErrorFromGLError(glGetError());

bail:
	FskEAGLContextSetCurrent(saveContext);
	if (err) { FskGLContextDispose(*pCtx, false); *pCtx = NULL; }
	return err;
}


/********************************************************************************
 * FskGLContextNewFromEAGLContext - iPhone
 ********************************************************************************/

FskErr FskGLContextNewFromEAGLContext(EAGLContext *context, FskGLContext *pCtx) {
	FskErr			err;
	FskGLContext	ctx;
	GLint			num;

	#ifdef LOG_PARAMETERS
		LOGD("FskGLContextNewFromEAGLContext(context=%p)", context);
	#endif /* LOG_PARAMETERS */

	*pCtx = NULL;
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(**pCtx), pCtx));
	ctx = *pCtx;

	ctx->context = context;
	BAIL_IF_ERR(err = FskEAGLContextSetCurrent(ctx->context));

	/* Query for the currently bound framebuffer */
	num = -1;																			/* 0 is used for the screen, so -1 should be an invalid frame buffer name */
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &num);
	//BAIL_IF_FALSE(-1 != num, err, kFskErrGLFramebufferUndefined);						/* There should be some framebuffer; don't know what else to do */
	ctx->framebuffer = num;

	/* Query for the type of object attached to the framebuffer, either GL_TEXTURE or GL_RENDERBUFFER. */
	num = 0;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &num);
	//BAIL_IF_TRUE(GL_NONE == num, err, kFskErrGLFramebufferIncompleteMissingAttachment);	/* Don't know what to do if there is no framebuffer attachment */
	ctx->attachmentType = num;

	/* Query for texture (or render buffer ) attached to the framebuffer */
	num = 0;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &num);
	//BAIL_IF_ZERO(num, err, kFskErrGLFramebufferIncompleteAttachment);					/* There should be something attached to the framebuffer */
	ctx->fbTexture = num;

	glGetError();																		/* Clear any GL errors */

bail:
	if (err) { FskGLContextDispose(*pCtx, false); *pCtx = NULL; }
	return err;
}


/********************************************************************************
 * FskGLContextNewFromCurrentContext - iPhone
 ********************************************************************************/

FskErr FskGLContextNewFromCurrentContext(FskGLContext *pCtx) {
	FskErr			err;
	EAGLContext		*context;

	BAIL_IF_NULL(pCtx, err, kFskErrInvalidParameter);
	*pCtx = NULL;

	BAIL_IF_NULL(context = FskEAGLContextGetCurrent(), err, kFskErrCGLBadContext);

	err = FskGLContextNewFromEAGLContext(context, pCtx);

bail:
	return err;
}


/********************************************************************************
 * FskGLContextGetCurrentContext - iPhone
 ********************************************************************************/

FskErr FskGLContextGetCurrentContext(FskGLContextStorage *storage, FskGLContext *pCtx) {
#if 1
	return kFskErrUnimplemented;
#else
	FskErr			err		= kFskErrNone;
	FskGLContext	ctx		= NULL;
	EAGLContext		*context;
	GLint			fb, att, buf;

	#ifdef LOG_PARAMETERS
		LOGD("FskGLContextGetCurrentContext(storage=%p pCtx=%p)", storage, pCtx);
	#endif /* LOG_PARAMETERS */

	FskMemSet(storage, 0, sizeof(*storage));
	BAIL_IF_NULL((context = FskEAGLContextGetCurrent()), err, kFskErrNotAccelerated);
	fb = -1;	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fb);
	att = 0;	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &att);
	buf = 0;	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &buf);
	BAIL_IF_FALSE(-1 != fb, err, kFskErrNotAccelerated);
	ctx = (FskGLContext)storage;
	ctx->context		= context;
	ctx->framebuffer	= fb;
	ctx->attachmentType	= att;
	ctx->fbTexture		= buf;
	(void)glGetError();		/* Clear errors */

bail:
	if (pCtx)	*pCtx = ctx;
	return err;
#endif
}


/********************************************************************************
 * FskGLContextDispose - iPhone
 ********************************************************************************/

void FskGLContextDispose(FskGLContext ctx, Boolean terminateGL) {
	#ifdef LOG_PARAMETERS
		LogFskGLContextDispose(ctx, terminateGL);
	#endif /* LOG_PARAMETERS */

	if (ctx) {
		EAGLContext *ocx;
		if ((ocx = FskEAGLContextGetCurrent()) == ctx->context)							/* Save the current context, ... */
			ocx = NULL;																	/* ... but nullify it if it is the context to  be disposed */
		FskEAGLContextSetCurrent(ctx->context);
		if (ctx->framebuffer) {
			glBindFramebuffer(GL_FRAMEBUFFER, ctx->framebuffer);						/* Bind the frame buffer */
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);	/* Unbind the texture from the frame buffer */
			glDeleteFramebuffers(1, &ctx->framebuffer);									/* Delete the frame buffer from this context */
			glBindFramebuffer(GL_FRAMEBUFFER, 0);										/* No current frame buffer in this context */
		}
		if (ctx->fbTexture) {
			glDeleteTextures(1, &ctx->fbTexture);										/* Delete the associated frame buffer texture */
		}
		// [ctx->context release]; /* Apparently setCurrentContext invokes automatic reference counting mode, so release is not necessary nor available. */
		FskMemPtrDispose(ctx);
		FskEAGLContextSetCurrent(ocx);													/* Restore the context */
	}
}


/********************************************************************************
 * FskGLContextGetEAGL
 ********************************************************************************/

void FskGLContextGetEAGL(FskGLContext ctx, EAGLContext **pContext) {
	*pContext = ctx->context;
}


/********************************************************************************
 * FskGLContextFrameBuffer - iPhone
 ********************************************************************************/

unsigned FskGLContextFrameBuffer(FskConstGLContext ctx) {
	#ifdef LOG_PARAMETERS
		LogFskGLContextFrameBuffer(ctx);
	#endif /* LOG_PARAMETERS */
	return ctx ? ctx->framebuffer : 0;
}


/********************************************************************************
 * FskGLContextMakeCurrent - iPhone
 ********************************************************************************/

FskErr FskGLContextMakeCurrent(FskConstGLContext ctx) {
	FskErr err = kFskErrNone;
	#ifdef LOG_PARAMETERS
		LogFskGLContextMakeCurrent(ctx);
	#endif /* LOG_PARAMETERS */
	BAIL_IF_ERR(err = FskEAGLContextSetCurrent(ctx ? ctx->context : NULL));
bail:
	return err;
}


/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
#else			/******** UNKNOWN ******** UNKNOWN ******** UNKNOWN ******** UNKNOWN ******** UNKNOWN ******** UNKNOWN ******** UNKNOWN *********/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/

	#error Unknown TARGET_OS

/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
#endif				/******** TARGET_OS ******** TARGET_OS ******** TARGET_OS ******** TARGET_OS ******** TARGET_OS ******** TARGET_OS **********/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/


#endif /* FSKBITMAP_OPENGL */

