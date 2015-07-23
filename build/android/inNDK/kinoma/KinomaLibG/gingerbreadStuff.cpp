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
#define USE_FRAMEBUFFER_VECTORS 1
#define __FSKWINDOW_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKEVENT_PRIV__
#define __FSKPORT_PRIV__
#define __FSKTHREAD_PRIV__

#include <stdio.h>

#include "../KinomaLibCommon/KinomaInterfaceLib.h"

#define __FSKECMASCRIPT_PRIV__		/* Needed for FskECMAScriptHibernate() */
#include "FskECMAScript.h"			/* Needed for FskECMAScriptHibernate() */
#include "FskHardware.h"

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "FskGLBlit.h"


extern FskFBGlobals			fbGlobals;
extern int					gScreenHeight;
extern int					gScreenWidth;
extern FskPhoneHWInfoRecord	myHWInfo;

extern Boolean gQuitting;
extern FskCondition jniRespCond;
extern FskMutex jniRespMutex;


ANativeWindow *theNativeWindow = NULL;

jint JAVANAME(FskView_setFskSurface)(JNIEnv* env, jobject viewObj, jobject surfaceObject)
{
	int didLock;

	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "gingerbread setFskSurface\n");
	theNativeWindow = ANativeWindow_fromSurface(env, surfaceObject);

	if (!fbGlobals || !fbGlobals->frameBuffer) {
        FskInstrumentedTypePrintfMinimal(&gAndroidMainBlockTypeInstrumentation, "MDK - %x fbGlobals or ->frameBuffer %x is NULL here! -__ CHECK BAD BUILD\n", fbGlobals, fbGlobals ? fbGlobals->frameBuffer : 0);
		return kFskErrNone;
	}
	// was above the fbGlobals check - will we lose "surface" if we hit the case above?
	fbGlobals->surface = theNativeWindow;
    FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "theNativeWindow/fbGlobals->surface is %x\n", theNativeWindow);

	didLock = (0 ==FskMutexTrylock(fbGlobals->screenMutex));

	FskWindow win = FskWindowGetActive();
	int invalidate = 0;

	if ((fbGlobals->frameBuffer->bounds.height != gScreenHeight)
		|| (fbGlobals->frameBuffer->bounds.width != gScreenWidth)) {

	}
	else {
		FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "gingerbread setFskSurface - NOT about to copy from backing store (%d x %d).\n",  fbGlobals->frameBuffer->bounds.width, fbGlobals->frameBuffer->bounds.height);

		invalidate = 1;
	}

	if (invalidate && win) {
		FskRectangleRecord b;
		FskPortGetBounds(win->port, &b);
		FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "gingerbread - invalidating port bounds %d %d %d %d\n", b.x, b.y, b.width, b.height);
		FskPortInvalidateRectangle(win->port, &b);
	}

	if (didLock) {
		FskInstrumentedTypePrintfVerbose(&gAndroidMainBlockTypeInstrumentation, "releasing from trylock\n");
		FskMutexRelease(fbGlobals->screenMutex);
	}

	return 1;
}

jint
JAVANAME(FskView_unsetFskSurface)(JNIEnv* env, jobject viewObj) {
	FskBitmap fb;
	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "gingerbread FskView_unsetFskSurface -- fbGlobals->surface is %x\n", fbGlobals->surface);

	if (fbGlobals->surface == NULL)
		return 0;

	FskECMAScriptHibernate();

	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "unsetFskSurface - about to release theNativeWindow %x\n", theNativeWindow);
	if (theNativeWindow)
		ANativeWindow_release(theNativeWindow);

	fbGlobals->surface = NULL;
	theNativeWindow = NULL;

	return 1;
}


/********************************************************************************
 * InitGLCallback - to be called from the render thread.
 ********************************************************************************/

Boolean GLHasASurface(void) {
	return theNativeWindow != NULL;
}

static void InitGLCallback(void *vAWin, void *vUnused1, void *vUnused2, void *vUnused3) {
	ANativeWindow	*aWin	= (ANativeWindow*)vAWin;
	FskErr			err;

    FskMutexAcquire(jniRespMutex);
    FskConditionSignal(jniRespCond);

	FskInstrumentedTypePrintfMinimal(&gAndroidMainBlockTypeInstrumentation, "[%p] InitGLCallback: calling FskGLInit(%p)", pthread_self(), aWin);
	err = FskGLInit(aWin);
	FskInstrumentedTypePrintfMinimal(&gAndroidMainBlockTypeInstrumentation, "[%p] InitGLCallback: FskGLInit(%p) returns err=%d", pthread_self(), aWin, (int)err);

    FskMutexRelease(jniRespMutex);
}

/********************************************************************************
 * ShutdownGLCallback - to be called from the render thread.
 ********************************************************************************/

static void ShutdownGLCallback(void *v1, void *v2, void *v3, void *v4) {
	FskNotificationPost(kFskNotificationGLContextAboutToLose);

	FskECMAScriptHibernate();
	{	FskWindow fWin = FskWindowGetActive();
		if (fWin)
			::FskPortReleaseGLResources(fWin->port);	/* This calls, in turn, FskGLEffectsShutdown(), which calls FskGLEffectCacheDispose() and FskGLEffectsShutdown() */
	}
	FskGLShutdown();

	FskNotificationPost(kFskNotificationGLContextLost);
}

/********************************************************************************
 * PortResizeCallback
 ********************************************************************************/

static void PortResizeCallback(void *v1, void *v2, void *v3, void *v4) {
	UInt32		width	= (UInt32)v1,
				height	= (UInt32)v2;
	FskGLPort	glPort	= FskGLPortGetCurrent();
	FskGLPortResize(glPort, width, height);
}


/********************************************************************************
 * FskViewGL_setFskSurface
 ********************************************************************************/

jint JAVANAME(FskViewGL_setFskSurface)(JNIEnv* env, jobject viewObj, jobject surfaceObj) {
	FskWindow		fWin 		= FskWindowGetActive();
	FskErr			err			= kFskErrNone;
	FskErr			retErr;
	FskThread		drawThread;

	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "[%p] FskViewGL_setFskSurface(%p, %p, %p)", (void*)pthread_self(), env, viewObj, surfaceObj);

	ANativeWindow *nw = theNativeWindow;
	theNativeWindow = ANativeWindow_fromSurface(env, surfaceObj);
	fbGlobals->surface = theNativeWindow;

	if (theNativeWindow && theNativeWindow == nw) {
		FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, " native window surface is same as before - don't setFskSurface twice\n");
		return 0;
	}

	FskGLSetNativeWindow((void*)theNativeWindow);
	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "FskViewGL_setFskSurface: active window is %p%s", fWin, fWin ? "" : ", bailing");

	BAIL_IF_NULL(fWin, err, kFskErrBadState);
	drawThread	= fWin->thread;
	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "FskViewGL_setFskSurface: drawThread is %p", drawThread, drawThread ? "" : ", bailing");
	BAIL_IF_NULL(drawThread, err, kFskErrBadState);
	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "FskViewGL_setFskSurface calling InitGLCallback");

	/* MDK - we need to wait for the callback to complete it's action in the other thread before we continue. I'm doing this by means of the jniRespCond.
	* However, at startup, we don't have the target thread to do it's work yet, so allow this through once at initialization.
	*/
	static int pass = 0;
	if (pass != 0)
		FskMutexAcquire(jniRespMutex);

	FskInstrumentedTypePrintfVerbose(&gAndroidMainBlockTypeInstrumentation, " about to post callback and wait for response from InitGLCallback\n");
	err = FskThreadPostCallback(drawThread, InitGLCallback, theNativeWindow, NULL, NULL, NULL);

	if (pass++ != 0) {
		if (!gQuitting) {
			FskConditionWait(jniRespCond, jniRespMutex);
			FskMutexRelease(jniRespMutex);
		}
	}
	else
		usleep(500);


	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "FskViewGL_setFskSurface: FskThreadPostCallback(InitGLCallback) returns %d", err);

	gScreenWidth = fbGlobals->frameBuffer->bounds.width;
	gScreenHeight = fbGlobals->frameBuffer->bounds.height;

    if (fWin) {
        FskRectangleRecord b;
        FskPortGetBounds(fWin->port, &b);
        FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, " - gl invalidating port bounds %d %d %d %d\n", b.x, b.y, b.width, b.height);
        FskPortInvalidateRectangle(fWin->port, &b);
    }

bail:
	return !err;
}


/********************************************************************************
 * FskViewGL_unsetFskSurface
 ********************************************************************************/

jint JAVANAME(FskViewGL_unsetFskSurface)(JNIEnv* env, jobject viewObj) {
	FskWindow		fWin 		= FskWindowGetActive();
	FskThread		drawThread	= fWin->thread;

	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "[%p] FskViewGL_unsetFskSurface(%p, %p) calling ShutdownGLCallback()", (void*)pthread_self(), env, viewObj);
	JAVANAME(FskView_unsetFskSurface)(env, viewObj);

	FskThreadPostCallback(drawThread, ShutdownGLCallback, NULL, NULL, NULL, NULL);

	return 1;
}


/********************************************************************************
 * FskViewGL_doFskSurfaceChanged
 ********************************************************************************/

jint JAVANAME(FskViewGL_doFskSurfaceChanged)(JNIEnv* env, jobject viewObj, jint width, jint height) {
	int			retVal;
	FskGLPort	glPort = FskGLPortGetCurrent();

//FskThreadYield();
	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "[%p] FskViewGL_doFskSurfaceChanged(%p, %p, %d, %d), port=%p", pthread_self(), env, viewObj, width, height, glPort);
	if (!glPort) {
		retVal = FskGLPortNew(width, height, viewObj, &glPort);
		switch (retVal) {
			case kFskErrNone:
				FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation,"\tGL Port successfully created!");
				break;
			case kFskErrGraphicsContext:
				if (glPort) {
					FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\tCreated a new GL Port, but there is no EGL context; deferring initialization.");
					break;
				}
				/* fall through */
			default:
				FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\tCannot create a new GL Port(%d, %d) err = %d; create an EGL context first.", width, height, retVal);
				break;
		}
		if (glPort)
			FskGLPortSetCurrent(glPort);
	}
	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\tFskViewGL_doFskSurfaceChanged calling FskView_doFskSurfaceChanged");
	retVal = JAVANAME(FskView_doFskSurfaceChanged)(env, viewObj, width, height);
	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "FskViewGL_doFskSurfaceChanged -- returning");
	return retVal;
}


/********************************************************************************
 * KinomaPlay_doPause
 ********************************************************************************/

jint JAVANAME(KinomaPlay_doPause)(JNIEnv* env, jclass clazz) {
	FskErr		err			= kFskErrNone;
	FskWindow	fWin 		= FskWindowGetActive();
	FskThread	drawThread;

	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "[%p] KinomaPlay_doPause(%p, %p, %p) calling ShutdownGLCallback()", (void*)pthread_self(), env, clazz, fWin);
	BAIL_IF_NULL(fWin, err, kFskErrBadState);
	drawThread = fWin->thread;

	err = FskThreadPostCallback(drawThread, ShutdownGLCallback, NULL, NULL, NULL, NULL);

bail:
	return !err;
}


/********************************************************************************
 * KinomaPlay_doResume
 ********************************************************************************/

jint JAVANAME(KinomaPlay_doResume)(JNIEnv* env, jclass clazz) {
	FskErr		err			= kFskErrNone;
	FskWindow	fWin 		= FskWindowGetActive();
	FskThread	drawThread;

	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation,"[%p] KinomaPlay_doResume(%p, %p, %p)", (void*)pthread_self(), env, clazz, fWin);
	BAIL_IF_NULL(fWin, err, kFskErrBadState);
	drawThread = fWin->thread;
	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "KinomaPlay_doResume: drawThread is %p", drawThread, drawThread ? "" : ", bailing");
	BAIL_IF_NULL(drawThread, err, kFskErrBadState);
	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "KinomaPlay_doResume calling InitGLCallback");

	FskMutexAcquire(jniRespMutex);
	err = FskThreadPostCallback(drawThread, InitGLCallback, theNativeWindow, NULL, NULL, NULL);
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}

	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "KinomaPlay_doResume: FskThreadPostCallback(InitGLCallback) returns %d", err);
	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "[%p] KinomaPlay_doResume successful", (void*)pthread_self());
bail:
	return !err;
}

