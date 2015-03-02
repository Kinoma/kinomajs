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


extern FskFBGlobals			fbGlobals;
extern int					gScreenHeight;
extern int					gScreenWidth;
extern int					gUsingBackingAsFramebuffer;
extern FskPhoneHWInfoRecord	myHWInfo;


struct so_t {
    jfieldID context;
    jfieldID surface;
};
static so_t so;


jint JAVANAME(FskView_setFskSurface)(JNIEnv* env, jobject viewObj, jobject surfaceObject)
{
	int didLock;
//@@MDK - try moving this part into JNIInit
	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "setFskSurface\n");
	jclass jsurface = env->FindClass("android/view/Surface");
	if (jsurface == NULL) {
		FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "fskView_setFskSurface - jsurface is NULL - return\n");
		return 0;
	}

	so.surface = env->GetFieldID(jsurface, "mSurface", "I");
	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "fskView_setFskSurface FieldID of surface: %d\n", so.surface);
	if (!so.surface) {
		FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, " - so.surface is null\n");
		return 0;
	}
//@@MDK - END try moving this part into JNIInit

//	theNativeWindow = ANativeWindow_fromSurface(env, surfaceObject);

	if (!fbGlobals || !fbGlobals->frameBuffer) {
		fprintf(stderr, "MDK - %p fbGlobals or ->frameBuffer %p is NULL here! -__ CHECK BAD BUILD\n", fbGlobals, fbGlobals ? fbGlobals->frameBuffer : 0);
		return kFskErrNone;
	}
	// was above the fbGlobals check - will we lose "surface" if we hit the case above?
	fbGlobals->surface = (void*)env->GetIntField(surfaceObject, so.surface);
//	fbGlobals->surface = theNativeWindow;

fprintf(stderr, "about to trylock screen mutex\n");
    // FskFrameBufferGrabScreenForDrawing();
	didLock = (0 ==FskMutexTrylock(fbGlobals->screenMutex));
fprintf(stderr, "trylock screen mutex returned %d\n", didLock);

	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, " - fbGlobals->surface is %x\n", fbGlobals->surface);

	FskWindow win = FskWindowGetActive();
	int invalidate = 0;

	//fprintf(stderr, "win is: %x\n", win);


	if ((fbGlobals->frameBuffer->bounds.height != gScreenHeight)
		|| (fbGlobals->frameBuffer->bounds.width != gScreenWidth)) {

		FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "setFskSurface - MDK was going to copy from backing store (%d x %d) but the size is different (%d x %d).\n", gScreenWidth, gScreenHeight, fbGlobals->frameBuffer->bounds.width, fbGlobals->frameBuffer->bounds.height);

		JAVANAME(FskView_doSizeAboutToChange)(env, viewObj, fbGlobals->backingBuffer->bounds.width, fbGlobals->backingBuffer->bounds.height, fbGlobals->frameBuffer->bounds.width, fbGlobals->frameBuffer->bounds.height);

		if (0) {
        	FskColorRGBARecord black = {123, 0, 0, 255};
        	FskRectangleFill(fbGlobals->frameBuffer, &fbGlobals->frameBuffer->bounds, &black, kFskGraphicsModeCopy, NULL);
		}

		gScreenWidth = fbGlobals->frameBuffer->bounds.width;
		gScreenHeight = fbGlobals->frameBuffer->bounds.height;

		fbGlobals->backingBuffer->bounds.width = gScreenWidth;
		fbGlobals->backingBuffer->bounds.height = gScreenHeight;
		fbGlobals->backingBuffer->rowBytes = gScreenWidth * 2;

		if (0) {
			FskColorRGBARecord black = {0, 123, 0, 255};
			FskRectangleFill(fbGlobals->backingBuffer, &fbGlobals->backingBuffer->bounds, &black, kFskGraphicsModeCopy, NULL);
		}
		invalidate = 1;

		FskWindowAndroidSizeChanged((int)win);	// need this to get AfterResize
		//	androidAfterWindowResize();
	}
	else {
		FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "setFskSurface - about to copy from backing store (%d x %d).\n",  fbGlobals->frameBuffer->bounds.width, fbGlobals->frameBuffer->bounds.height);

		dupeBitmap(fbGlobals->backingBuffer, fbGlobals->frameBuffer, 0);

		FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "[%s] setFskSurface orientation %d - done copying backing store bits %x to FB bits %x. %d %d %d %d\n", threadTag(FskThreadGetCurrent()),  myHWInfo.orientation, fbGlobals->backingBuffer->bits, fbGlobals->frameBuffer->bits, fbGlobals->frameBuffer->bounds.x, fbGlobals->frameBuffer->bounds.y, fbGlobals->frameBuffer->bounds.width, fbGlobals->frameBuffer->bounds.height);
		invalidate = 1;
		gUsingBackingAsFramebuffer = 0;
	}

	if (invalidate && win) {
		FskRectangleRecord b;
		FskPortGetBounds(win->port, &b);
		FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "[%s] - invalidating port bounds %d %d %d %d\n", threadTag(FskThreadGetCurrent()), b.x, b.y, b.width, b.height);
		FskPortInvalidateRectangle(win->port, &b);
	}

	if (didLock) {
fprintf(stderr, "about to release from trylock'd screen mutex\n");
		FskMutexRelease(fbGlobals->screenMutex);
	}

	return 1;
}

jint
JAVANAME(FskView_unsetFskSurface)(JNIEnv* env, jobject viewObj) {
	FskBitmap fb;
	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "FskView_unsetFskSurface -- fbGlobals->surface is %x - being set to NULL\n", fbGlobals->surface);

	if (fbGlobals->surface == NULL)
		return 0;

	FskFrameBufferGrabScreenForDrawing();

//fprintf(stderr, "unsetFskSurface - about to LockSurfaceForReading\n");
	FskFrameBufferLockSurfaceForReading(&fb);
	dupeBitmap(fbGlobals->frameBuffer, fbGlobals->backingBuffer, 1);	// replace rowbytes in backbuffer
	FskFrameBufferUnlockSurfaceForReading(fb);

	fbGlobals->surface = NULL;
	fbGlobals->frameBuffer->bits = fbGlobals->backingBuffer->bits;
	gUsingBackingAsFramebuffer = 1;

	FskFrameBufferReleaseScreenForDrawing();

	FskECMAScriptHibernate();

//fprintf(stderr, "unsetFskSurface - about to release window\n");
//	ANativeWindow_release(theNativeWindow);
//	theNativeWindow = NULL;

	return 1;
}


/********************************************************************************
 * FskViewGL_setFskSurface
 ********************************************************************************/

Boolean GLHasASurface(void) {
	return false;
}

jint JAVANAME(FskViewGL_setFskSurface)(JNIEnv* env, jobject viewObj, jobject surfaceObj) {
	return 0;
}
