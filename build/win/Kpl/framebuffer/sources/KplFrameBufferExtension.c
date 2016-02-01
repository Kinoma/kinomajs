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
#define __FSKBITMAP_PRIV__
#include "FskBitmap.h"
#include "FskExtensions.h"
#include "FskFrameBuffer.h"
#include "KplScreen.h"

//#define SUPPORT_OPENGLES 1

#if SUPPORT_OPENGLES
	#include <EGL/egl.h>
#endif

#define BITS_NULL	((void *)1)	// to make sure the framework crashes if the screen bitmap bits are accessed without locking

static SInt32 gLockCount = 0;
static FskMutex gScreenMutex = NULL;
static FskBitmap gFrameBuffer = NULL;
static KplBitmap gKplScreenBitmap = NULL;
static HWND ghWnd = NULL;

extern void copyBitsToWindow(HWND hWnd, FskBitmap screenBitmap);
extern HWND createWindow(UInt32 width, UInt32 height);

static FskErr fbLockSurface(FskBitmap bitmap, void **baseAddr, int *rowBytes)
{
	if (bitmap != gFrameBuffer)
		return kFskErrNone;
		
	FskMutexAcquire(gScreenMutex);
	
	if (0 == gLockCount++) {
		FskErr err = KplScreenLockBitmap(gKplScreenBitmap);
		if (err) {
			FskMutexRelease(gScreenMutex);
			return err;
		}
		gFrameBuffer->bits = gKplScreenBitmap->baseAddress;
		gFrameBuffer->rowBytes = gKplScreenBitmap->rowBytes;
	}
	
	FskMutexRelease(gScreenMutex);

	if (baseAddr)
		*baseAddr = gFrameBuffer->bits;
	if (rowBytes)
		*rowBytes = gFrameBuffer->rowBytes;
		
	return kFskErrNone;
}

static FskErr fbUnlockSurface(FskBitmap bitmap)
{
	FskErr err = kFskErrNone;
	
	if (bitmap != gFrameBuffer)
		return kFskErrNone;

	FskMutexAcquire(gScreenMutex);
	
	if (0 == --gLockCount) {
		// Just for testing frame buffer drawing
		copyBitsToWindow(ghWnd, gFrameBuffer);
		
		err = KplScreenUnlockBitmap(gKplScreenBitmap);
		if (err) {
			FskMutexRelease(gScreenMutex);
			return err;
		}
		
		gFrameBuffer->bits = BITS_NULL;
	}
	
	FskMutexRelease(gScreenMutex);

	return kFskErrNone;
}

FskErr fbLockSurfaceArea(FskBitmap bitmap, FskRectangleRecord *r, void **baseAddr, int *rowBytes)
{
	return fbLockSurface(bitmap, baseAddr, rowBytes);
}

FskErr fbGetScreenBounds(FskRectangleRecord *bounds)
{
	bounds->x = bounds->y = 0;
	bounds->width = gKplScreenBitmap->width;
	bounds->height = gKplScreenBitmap->height;
	
	return kFskErrNone;
}

FskErr fbGetScreenBitmap(FskBitmap *bitmap)
{
	*bitmap = gFrameBuffer;
	return kFskErrNone;
}

#if SUPPORT_OPENGLES

#define GLES_VERSION	2
#define CONTEXT565		1
//#define CONTEXT8888		1

FskErr fbGetEGLContext(void **displayOut, void **surfaceOut, void **contextOut, void **nativeWindowOut)
{
	static const EGLint configAttribs8888[] = {
		EGL_BUFFER_SIZE,		32,
		EGL_ALPHA_SIZE,			8,
		EGL_RED_SIZE,			8,
		EGL_GREEN_SIZE,			8,
		EGL_BLUE_SIZE,			8,
		EGL_DEPTH_SIZE,			0,
		EGL_STENCIL_SIZE,		0,
		EGL_SAMPLES,			0,
		EGL_COLOR_BUFFER_TYPE,	EGL_RGB_BUFFER,
#if GLES_VERSION == 1
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES_BIT,
#else
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
#endif
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_NONE,				EGL_NONE
	};
	static const EGLint configAttribs565[] = {
		EGL_BUFFER_SIZE,		16,
		EGL_ALPHA_SIZE,			0,
		EGL_RED_SIZE,			5,
		EGL_GREEN_SIZE,			6,
		EGL_BLUE_SIZE,			5,
		EGL_DEPTH_SIZE,			0,
		EGL_STENCIL_SIZE,		0,
		EGL_SAMPLES,			0,
		EGL_COLOR_BUFFER_TYPE,	EGL_RGB_BUFFER,
#if GLES_VERSION == 1
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES_BIT,
#else
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
#endif
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_NONE,				EGL_NONE
	};
	static const EGLint ctxAttr[] = {
		EGL_CONTEXT_CLIENT_VERSION,	GLES_VERSION,
		EGL_NONE,					EGL_NONE
	};
	FskErr		err			= kFskErrNone;
	EGLDisplay	display		= NULL;
	EGLSurface	surface		= NULL;
	EGLContext	context		= NULL;
	EGLConfig	*config		= NULL;
	EGLint		numConfigs;
	const EGLint *attribList;
#ifdef CONTEXT565
	attribList = configAttribs565;
#else
	attribList = configAttribs8888;
#endif	

	BAIL_IF_FALSE(EGL_NO_DISPLAY != (display = eglGetDisplay((NativeDisplayType)EGL_DEFAULT_DISPLAY)), err, kFskErrGraphicsContext);
	BAIL_IF_FALSE(                  eglInitialize(display, NULL, NULL), err, kFskErrGraphicsContext);	/* (display, &major, &minor) */
	BAIL_IF_FALSE(eglChooseConfig(display, attribList, NULL, 0, &numConfigs), err, kFskErrGraphicsContext);
	BAIL_IF_ZERO(numConfigs, err, kFskErrMismatch);
	BAIL_IF_ERR(err = FskMemPtrNew(numConfigs * sizeof(*config), (FskMemPtr*)(void*)(&config)));
	BAIL_IF_FALSE(eglChooseConfig(display, attribList, config, numConfigs, &numConfigs), err, kFskErrGraphicsContext);

	BAIL_IF_FALSE(EGL_NO_SURFACE !=	(surface = eglCreateWindowSurface(display, config[0], ghWnd, NULL)), err, kFskErrGraphicsContext);
	BAIL_IF_FALSE(EGL_NO_CONTEXT != (context = eglCreateContext(display, config[0], EGL_NO_CONTEXT, ctxAttr)), err, kFskErrGraphicsContext);
	BAIL_IF_FALSE(EGL_FALSE      != eglMakeCurrent(display, surface, surface, context), err, kFskErrGraphicsContext);

bail:
	if (err) {
		if (display) {
			if (surface)
				eglDestroySurface(display, surface);
			if (context)
				eglDestroyContext(display, context);
			//eglTerminate(display);
		}
		display = NULL;
		surface = NULL;
		context = NULL;
	}

	*displayOut = display;
	*surfaceOut = surface;
	*contextOut = context;
	*nativeWindowOut = ghWnd;
	
	return err;
}

#endif /* SUPPORT_OPENGLES */

FskExport(FskErr) KplFrameBuffer_fskLoad(FskLibrary library)
{
	FskErr err;
	static FrameBufferVectors vectors;

	err = KplScreenGetBitmap(&gKplScreenBitmap);
	if (err) goto bail;
	
	err = FskBitmapNewWrapper(gKplScreenBitmap->width, gKplScreenBitmap->height, gKplScreenBitmap->pixelFormat, gKplScreenBitmap->depth, BITS_NULL, 0, &gFrameBuffer);
	if (err) goto bail;
	
	FskMemSet(&vectors, 0, sizeof(FrameBufferVectors));
	
	vectors.doLockSurface = fbLockSurface;
	vectors.doUnlockSurface = fbUnlockSurface;
	vectors.doGetScreenBounds = fbGetScreenBounds;
	vectors.doGetScreenBitmap = fbGetScreenBitmap;
	
	vectors.doLockSurfaceArea = fbLockSurfaceArea;

#if SUPPORT_OPENGLES
	vectors.doGetEGLContext= fbGetEGLContext;
#endif
	
	FskFrameBufferSetVectors(&vectors);
	
	FskMutexNew(&gScreenMutex, (char*)"screenMutex");
	
	// Just for testing frame buffer drawing
	ghWnd = createWindow(gKplScreenBitmap->width, gKplScreenBitmap->height);

bail:
	return err;
}

FskExport(FskErr) KplFrameBuffer_fskUnload(FskLibrary library)
{
	FskMutexDispose(gScreenMutex);
	FskBitmapDispose(gFrameBuffer);
	FskFrameBufferSetVectors(NULL);
	
	return kFskErrNone;
}
