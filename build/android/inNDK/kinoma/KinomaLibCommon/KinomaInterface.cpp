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


#define DEBUG_STDERR 0

#define USE_FRAMEBUFFER_VECTORS 1
#define __FSKWINDOW_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKEVENT_PRIV__
#define __FSKPORT_PRIV__
#define __FSKTHREAD_PRIV__
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <math.h>
#include <stdio.h>

#include "KinomaInterfaceLib.h"

#include "FskFS.h"
#include "FskHardware.h"
#include "FskTextConvert.h"
#include "FskGLBlit.h"


#define DELETE_AUTO_UPDATE_APK_ON_LAUNCH 1

inline int ABS(int x) {
	if (x < 0) return -x;
	return x;
}

inline int MAX(int x, int y) {
	if (x > y) return x;
	return y;
}

#define mY( x, y ) x##y
#define mYY( x, y ) mY( x, y )
#define JAVANAME( x ) mYY( OBJECTBASE , x )

extern int gSystemBarHeight;

//namespace android {

//static JavaVM *gJavaVM;

extern FskPhoneHWInfoRecord myHWInfo;

int gScreenWidthMax = 480;
int gScreenWidth = 480;
int gScreenHeightMax = 800;
int gScreenHeight = 762;
int gScreenDimensionMax = 800;

char *staticModel = NULL;
char *staticOSVersion = NULL;
char *staticIMEI = NULL;
char *staticUUID = NULL;
char *androidStaticDataDir = NULL;
char *androidStaticExternalDir = NULL;
char *androidStaticAppDir = NULL;
char *userDeviceName = NULL;
char *androidLangString = NULL;

char *staticMusic = NULL;
char *staticPodcast = NULL;
char *staticPictures = NULL;
char *staticMovies = NULL;
char *staticDownloads = NULL;
char *staticDcim = NULL;


int staticHdpi = 0;
int staticVdpi = 0;
int staticDensityDpi = 0;
int staticHasTouch = 0;
static int firstMouseTime = 0;

#define NUM_TOUCH_POINTERS	4		// Android currently only does 2

int glastX[NUM_TOUCH_POINTERS];
int glastY[NUM_TOUCH_POINTERS];
int glastDown[NUM_TOUCH_POINTERS];

int gPendingActivate = 0;

int pendingDoFskSurfaceChanged = 0;
FskPointRecord pendingDoFskSurfaceChangedSize = {0, 0};

extern int gInitStarted;

int gPendingSizeChange = 0;
int gPendingSizeChangeDone = 0;

int gPendingSystemBar = 0;
int gPendingSystemBarShow = 0;

int pendingOrientation = -1;

int baseTimeSeconds = 0;
int baseTimeUseconds = 0;

extern int gPendingSetKbdSelection;
extern int gPendingSetKbdSelectionStart;
extern int gPendingSetKbdSelectionEnd;

FskTimeRecord gWindowUpdateTime = {0, 0};
UInt32 gWindowUpdateInterval = 16;

struct so_t {
	jfieldID context;
    jfieldID surface;
};
static so_t so;


FskFBGlobals fbGlobals = NULL;


FskTimeCallBack touchMovedTimer = NULL;
FskFixed			touchScale = 0;

int gNumPts = 0;
FskPointAndTicksRecord *gPts = NULL;
#define kFskMotionMovedGateMS	30

#define LIMIT_MAX(x, max)	((x > max) ? max : x)

#define LIMIT_SCREEN_X(x)	((x < 0) ? 0 : (x > fbGlobals->frameBuffer->bounds.width) ? fbGlobals->frameBuffer->bounds.width : x)
#define LIMIT_SCREEN_Y(y)	((y < 0) ? 0 : (y > fbGlobals->frameBuffer->bounds.height) ? fbGlobals->frameBuffer->bounds.height : y)

int gTESelectionStart = 0;
int gTESelectionEnd = 0;
int gTEIgnoreChanged = 0;

Boolean anyMouseDown();

int staticButtonsMirrored = 0;
Boolean gNeedsOrientationRotate = true;

char *androidGetStaticDataDir() {
	return androidStaticDataDir;
}
char *androidGetStaticExternalDir() {
	return androidStaticExternalDir;
}
char *androidGetStaticAppDir() {
	return androidStaticAppDir;
}
char *androidGetStaticIMEI() {
	return staticIMEI;
}
char *androidGetStaticUUID() {
	return staticUUID;
}
char *androidGetDeviceUsername() {
	return userDeviceName;
}

FskErr getLastXY(SInt32 *x, SInt32 *y) {
	FskFixed v;

	*x = glastX[0];
	*y = glastY[0];
	v = *x << 16;
	*x = FskFixDiv(v, touchScale) >> 16;
	v = *y << 16;
	*y = FskFixDiv(v, touchScale) >> 16;
    FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, "getLastXY now %d %d --- ", *x, *y);

	return kFskErrNone;
}

void androidGetScreenSize(SInt32 *x, SInt32 *y, SInt32 *xmax, SInt32 *ymax) {
	*x = gScreenWidth;
	*y = gScreenHeight;
	*xmax = gScreenWidthMax;
	*ymax = gScreenHeightMax;
}


void WindowUpdateCallback(void *window, void *time, void *unused2, void *unused3)
{
    FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "inside WindowUpdateCallback(), window: %x", window);

    FskWindow win = (FskWindow)window;

	if (win)
    {
        FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "WindowUpdateCallback calling FskWindowUpdate");
        FskWindowUpdate((FskWindow)win, (FskTime)time);

		FskMutexAcquire(win->drawPumpMutex);
		if(win->drawPumpCnt > 0)
			win->drawPumpCnt--;
		FskMutexRelease(win->drawPumpMutex);
    }
}

void JAVANAME(KinomaPlay_setWindowUpdateInterval)(JNIEnv* env, jclass clazz, jlong interval) {
	gWindowUpdateInterval = (UInt32)interval;
}

void JAVANAME(KinomaPlay_fskWindowUpdate)(JNIEnv* env, jclass clazz, jlong time) {
	FskWindow win;
	win = FskWindowGetActive();
	int update = 0;

	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "enter KinomaPlay_fskWindowUpdate");
	if (win) {
		if (fbGlobals->surface == NULL) {
			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "Choreographer callback fired before surface created, bailing");
			return;
		}

		FskMutexAcquire(win->drawPumpMutex);
		if(win->drawPumpCnt < 1 && !win->updateSuspended) { //preserve one redraw event in Q only!
			win->drawPumpCnt ++;
			update = 1;
		}
		FskMutexRelease(win->drawPumpMutex);

		if(update) {
			time = time - baseTimeSeconds * kFskTimeMsecPerSec - baseTimeUseconds / kFskTimeUsecPerMsec;
			gWindowUpdateTime.seconds = (SInt32)(time / kFskTimeMsecPerSec);
			gWindowUpdateTime.useconds = (SInt32)((time % kFskTimeMsecPerSec) * kFskTimeUsecPerMsec);

			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "Posting the WindowUpdateCallback, the next v-sync should happen @ time %d.%06d", gWindowUpdateTime.seconds, gWindowUpdateTime.useconds);

			FskThreadPostCallback(win->thread, WindowUpdateCallback, win, &gWindowUpdateTime, NULL, NULL);
		}
		else {
			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "Warning...drop frame in draw pump mode.");

		}
	}
}


void JAVANAME(KinomaPlay_doFskOnTextChanged)(JNIEnv* env, jclass clazz, jstring str, jint start, jint before, jint count) {
	const char *afterStr;
	char *loc;
	FskErr err = kFskErrNone;
	FskWindow win;
	FskEvent ev;
	int len;
	UInt32 cmd;
	FskMemPtr cutText = NULL;
	int i, j, adv;

	if (gTEIgnoreChanged) {
		FskInstrumentedTypePrintfDebug(&gAndroidTETypeInstrumentation, "IGNORING OnTextChanged %s start %d before %d count %d", afterStr, start, before, count);
		return;
	}

	afterStr = env->GetStringUTFChars(str, NULL);
	FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "OnTextChanged %s start %d before %d count %d", afterStr, start, before, count);

	if (before == 0 && count == 0) {
		FskInstrumentedTypePrintfDebug(&gAndroidTETypeInstrumentation, " before and count are both 0, bail");
		goto bail;
	}

	win = FskWindowGetActive();

	loc = (char*)afterStr;
	adv = 0;
	for (i=0; i<start; i++) {
//		FskInstrumentedTypePrintfDebug(&gAndroidTETypeInstrumentation, "Looking at %s - adv is %d", &(loc[adv]), FskTextUTF8Advance((const unsigned char*)loc, adv, 1));
		adv += FskTextUTF8Advance((const unsigned char*)loc, adv, 1);
	}
	len = 0;
	for (i=0; i<count; i++) {
		//		FskInstrumentedTypePrintfDebug(&gAndroidTETypeInstrumentation, "Looking at %s - len is %d", (&loc[adv+len]), FskTextUTF8Advance((const unsigned char*)loc, adv + len, 1));
		len += FskTextUTF8Advance((const unsigned char*)loc, adv + len, 1);
	}

//	FskInstrumentedTypePrintfDebug(&gAndroidTETypeInstrumentation, "OnTextChanged - newmem - %d bytes", len + 1);
	FskMemPtrNew(len + 1, &cutText);
	if (len != 0)
		FskMemCopy(cutText, ((char*)afterStr) + adv, len);
	cutText[len] = '\0';
//	FskInstrumentedTypePrintfDebug(&gAndroidTETypeInstrumentation, "OnTextChanged - cutText - %s", cutText);

	if (kFskErrNone == FskEventNew(&ev, kFskEventKeyDown, NULL, kFskEventModifierNone)) {
		char filelist[256], *pos;
		FskEventParameterAdd(ev, kFskEventParameterKeyUTF8, len + 1, (void*)cutText);

		cmd = 1025;
		FskEventParameterAdd(ev, kFskEventParameterCommand, sizeof(UInt32), &cmd);

		pos = filelist;
		len = sprintf(pos, "%d", start);
		pos[len++] = '\0';
		len += sprintf(&pos[len], "%d", before);
		pos[len++] = '\0';
		pos[len++] = '\0';

		FskEventParameterAdd(ev, kFskEventParameterFileList, len, filelist);
		androidDoOrQueue(win, ev);
	}

bail:
	env->ReleaseStringUTFChars(str, afterStr);
	FskMemPtrDispose(cutText);

	gTESelectionStart = start + count;
	gTESelectionEnd = start + count;
}


void JAVANAME(KinomaPlay_fskPhoneSSIDChanged)( JNIEnv *env, jobject thiz, jstring SSID)
{
    const char *ssidStr = env->GetStringUTFChars(SSID, NULL);

    if (myHWInfo.ssid)
        FskMemPtrDispose(myHWInfo.ssid);
    myHWInfo.ssid = FskStrDoCopy(ssidStr);
    FskSetPhoneHWInfo(&myHWInfo);

    env->ReleaseStringUTFChars(SSID, ssidStr);
}

void JAVANAME(KinomaPlay_fskPhoneOperatorChanged)( JNIEnv *env, jobject thiz, jstring oper)
{
    const char *operStr = env->GetStringUTFChars(oper, NULL);

    if (myHWInfo.operatorStr)
        FskMemPtrDispose(myHWInfo.operatorStr);
    myHWInfo.operatorStr = FskStrDoCopy(operStr);
    FskSetPhoneHWInfo(&myHWInfo);

    env->ReleaseStringUTFChars(oper, operStr);
}

void JAVANAME(KinomaPlay_setDeviceOrientation)( JNIEnv *env, jobject thiz, jint orientation) {
	if (!gInitStarted) {
		pendingOrientation = orientation;
		return;
	}

	FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "setDeviceOrientation was called. Last orientation was %d, new orientation is %d", myHWInfo.orientation, orientation);

	if (myHWInfo.orientation != orientation) {
		if (fbGlobals->midSizeChange) {
			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, " setDeviceOrientation was called -- DURING A SIZE CHANGE (%d) . Last orientation was %d, new orientation is %d", fbGlobals->midSizeChange, myHWInfo.orientation, orientation);
			pendingOrientation = orientation;
		}
		else {
			pendingOrientation = orientation;
		}
	}

}

void JAVANAME(KinomaPlay_setDeviceUsername)( JNIEnv *env, jobject thiz, jstring deviceName) {
	const char *deviceNameStr;

	if (deviceName) {
		deviceNameStr = env->GetStringUTFChars(deviceName, NULL);
		userDeviceName = (char*)FskMemPtrAlloc(strlen(deviceNameStr) + 1);
		strcpy(userDeviceName, deviceNameStr);
		env->ReleaseStringUTFChars(deviceName, deviceNameStr);
	}

//MDK - we would notify the property change here.
}

void JAVANAME(KinomaPlay_setAndroidLanguage)( JNIEnv *env, jobject thiz, jstring
 jstr) {
    const char *langStr;

    if (jstr) {
        langStr = env->GetStringUTFChars(jstr, NULL);
        androidLangString = (char*)FskMemPtrAlloc(strlen(langStr) + 1);
        strcpy(androidLangString, langStr);
        env->ReleaseStringUTFChars(jstr, langStr);
    }
    else {
        if (androidLangString)
            FskMemPtrDispose(androidLangString);
        androidLangString = NULL;
    }

//    FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "android language set to '%s'", androidLangString);
}

void JAVANAME(KinomaPlay_setSpecialPaths)( JNIEnv *env, jobject thiz, jstring musicPath, jstring podcastPath, jstring picturesPath, jstring moviesPath, jstring downloadsPath, jstring dcimPath) {
	const char *musicStr = env->GetStringUTFChars(musicPath, NULL);
	const char *podcastStr = env->GetStringUTFChars(podcastPath, NULL);
	const char *picturesStr = env->GetStringUTFChars(picturesPath, NULL);
	const char *moviesStr = env->GetStringUTFChars(moviesPath, NULL);
	const char *downloadsStr = env->GetStringUTFChars(downloadsPath, NULL);
	const char *dcimStr = env->GetStringUTFChars(dcimPath, NULL);

	staticMusic = (char*)FskMemPtrAlloc(strlen(musicStr) + 1);
	strcpy(staticMusic, musicStr);
	staticPodcast = (char*)FskMemPtrAlloc(strlen(podcastStr) + 1);
	strcpy(staticPodcast, podcastStr);
	staticPictures = (char*)FskMemPtrAlloc(strlen(picturesStr) + 1);
	strcpy(staticPictures, picturesStr);
	staticMovies = (char*)FskMemPtrAlloc(strlen(moviesStr) + 1);
	strcpy(staticMovies, moviesStr);
	staticDownloads = (char*)FskMemPtrAlloc(strlen(downloadsStr) + 1);
	strcpy(staticDownloads, downloadsStr);
	staticDcim = (char*)FskMemPtrAlloc(strlen(dcimStr) + 1);
	strcpy(staticDcim, dcimStr);

    env->ReleaseStringUTFChars(musicPath, musicStr);
    env->ReleaseStringUTFChars(podcastPath, podcastStr);
    env->ReleaseStringUTFChars(picturesPath, picturesStr);
    env->ReleaseStringUTFChars(moviesPath, moviesStr);
    env->ReleaseStringUTFChars(downloadsPath, downloadsStr);
    env->ReleaseStringUTFChars(dcimPath, dcimStr);
}

void JAVANAME(KinomaPlay_setStaticDeviceInfo)( JNIEnv *env, jobject thiz, jstring model, jstring OSVersion, jint buttonsReversed, jint needsOrientationRotate, jint touchCapable, jstring imei, jstring uuid, jint hDpi, jint vDpi, jint densityDpi, jint screenWidth, jint screenHeight, jint statusBarHeight, jstring dataDir, jstring appPath, jstring externalDir) {
	int i;
	const char *modelStr = env->GetStringUTFChars(model, NULL);
	const char *OSVersionStr = env->GetStringUTFChars(OSVersion, NULL);
	const char *imeiStr = env->GetStringUTFChars(imei, NULL);
	const char *uuidStr = env->GetStringUTFChars(uuid, NULL);
	const char *dataDirStr = env->GetStringUTFChars(dataDir, NULL);
	const char *externalDirStr = env->GetStringUTFChars(externalDir, NULL);
	const char *appPathStr = env->GetStringUTFChars(appPath, NULL);

#if DEBUG_STDERR
{
	char foo[256];
	FskErr err;
	FskFileInfo info;
	int p = 1;
	err = FskFSFileCreateDirectory("/sdcard/tmp/");
	err = FskFSFileGetFileInfo("/sdcard/tmp/", &info);
	if (!err) {
		do {
			sprintf(foo, "/sdcard/tmp/out%d.txt", p++);
			err = FskFSFileGetFileInfo(foo, &info);
			if (err) {
			    freopen(foo, "w", stderr);
    			setbuf(stderr, NULL);
			}
		} while (err == 0);
	}
	fprintf(stderr, "Start\n");
}
#endif

	staticModel = (char*)FskMemPtrAlloc(strlen(modelStr) + 1);
	strcpy(staticModel, modelStr);
	staticOSVersion = (char*)FskMemPtrAlloc(strlen(OSVersionStr) + 1);
	strcpy(staticOSVersion, OSVersionStr);
	staticIMEI = (char*)FskMemPtrAlloc(strlen(imeiStr) + 1);
	strcpy(staticIMEI, imeiStr);
	staticUUID = (char*)FskMemPtrAlloc(strlen(uuidStr) + 1);
	strcpy(staticUUID, uuidStr);
	androidStaticDataDir = (char*)FskMemPtrAlloc(strlen(dataDirStr) + 1);
	strcpy(androidStaticDataDir, dataDirStr);
	androidStaticExternalDir = (char*)FskMemPtrAlloc(strlen(externalDirStr) + 1);
	strcpy(androidStaticExternalDir, externalDirStr);
	androidStaticAppDir = (char*)FskMemPtrAlloc(strlen(appPathStr) + 1);
	strcpy(androidStaticAppDir, appPathStr);

//	fprintf(stderr, "modelStr: %s\n", modelStr);
//	fprintf(stderr, "dataDirStr: %s\n", dataDirStr);
//	fprintf(stderr, "externalDirStr: %s\n", externalDirStr);
//	fprintf(stderr, "appPathStr: %s\n", appPathStr);

    env->ReleaseStringUTFChars(model, modelStr);
    env->ReleaseStringUTFChars(OSVersion, OSVersionStr);
    env->ReleaseStringUTFChars(imei, imeiStr);
    env->ReleaseStringUTFChars(uuid, uuidStr);
    env->ReleaseStringUTFChars(dataDir, dataDirStr);
    env->ReleaseStringUTFChars(externalDir, externalDirStr);
    env->ReleaseStringUTFChars(appPath, appPathStr);

	if (touchCapable == 3 || touchCapable == 2)
		staticHasTouch = 1;

	for (i=0; i<NUM_TOUCH_POINTERS; i++) {
		glastX[i] = 0;
		glastY[i] = 0;
		glastDown[i] = 0;
	}

	staticHdpi = hDpi;
	staticVdpi = vDpi;
	staticDensityDpi = densityDpi;

	gScreenWidthMax = screenWidth;
	gScreenHeightMax = screenHeight;
	gScreenWidth = screenWidth;
	gScreenHeight = screenHeight - statusBarHeight;

	gSystemBarHeight = statusBarHeight;

	staticButtonsMirrored = buttonsReversed;

	gNeedsOrientationRotate = needsOrientationRotate;

#if DELETE_AUTO_UPDATE_APK_ON_LAUNCH
	{
		FskErr err;
		char foo[256];
		FskFileInfo info;
		sprintf(foo, "%sDownload/KinomaPlay.apk", androidStaticDataDir);
		err = FskFSFileGetFileInfo(foo, &info);
		if (!err)
			FskFileDelete(foo);

	}
#endif
}

void myGetSpecialPaths(char **musicDir, char **podcastDir, char **picturesDir, char **moviesDir, char **downloadsDir, char **dcimDir)
{
	if (musicDir)
		*musicDir = staticMusic;
	if (podcastDir)
		*podcastDir = staticPodcast;
	if (picturesDir)
		*picturesDir = staticPictures;
	if (moviesDir)
		*moviesDir = staticMovies;
	if (downloadsDir)
		*downloadsDir = staticDownloads;
	if (dcimDir)
		*dcimDir = staticDcim;
}

void myGetModelInfo(char **modelName, char **osVersion, int *hasTouch, int *buttonsMirrored, int *usesGL) {
	if (modelName)
		*modelName = FskStrDoCopy(staticModel);
	if (osVersion)
		*osVersion = FskStrDoCopy(staticOSVersion);
	if (hasTouch)
		*hasTouch = staticHasTouch;
	if (buttonsMirrored)
		*buttonsMirrored = staticButtonsMirrored;
	if (usesGL)
		*usesGL = usingOpenGL;
}



jint xJNI_OnLoad(JavaVM *vm, void *reserved) {
//	gJavaVM = vm;
//FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "gJavaVM is %x", vm);
	return JNI_VERSION_1_4;
}
enum {
		KEYCODE_UNKNOWN         = 0,
		KEYCODE_SOFT_LEFT       = 1,
		KEYCODE_SOFT_RIGHT      = 2,
		KEYCODE_HOME            = 3,
		KEYCODE_BACK            = 4,
		KEYCODE_CALL            = 5,
		KEYCODE_ENDCALL         = 6,
		KEYCODE_0               = 7,
		KEYCODE_1               = 8,
		KEYCODE_2               = 9,
		KEYCODE_3               = 10,
		KEYCODE_4               = 11,
		KEYCODE_5               = 12,
		KEYCODE_6               = 13,
		KEYCODE_7               = 14,
		KEYCODE_8               = 15,
		KEYCODE_9               = 16,
		KEYCODE_STAR            = 17,
		KEYCODE_POUND           = 18,
		KEYCODE_DPAD_UP         = 19,
		KEYCODE_DPAD_DOWN       = 20,
		KEYCODE_DPAD_LEFT       = 21,
		KEYCODE_DPAD_RIGHT      = 22,
		KEYCODE_DPAD_CENTER     = 23,
		KEYCODE_VOLUME_UP       = 24,
		KEYCODE_VOLUME_DOWN     = 25,
		KEYCODE_POWER           = 26,
		KEYCODE_CAMERA          = 27,
		KEYCODE_CLEAR           = 28,
		KEYCODE_A               = 29,
		KEYCODE_B               = 30,
		KEYCODE_C               = 31,
		KEYCODE_D               = 32,
		KEYCODE_E               = 33,
		KEYCODE_F               = 34,
		KEYCODE_G               = 35,
		KEYCODE_H               = 36,
		KEYCODE_I               = 37,
		KEYCODE_J               = 38,
		KEYCODE_K               = 39,
		KEYCODE_L               = 40,
		KEYCODE_M               = 41,
		KEYCODE_N               = 42,
		KEYCODE_O               = 43,
		KEYCODE_P               = 44,
		KEYCODE_Q               = 45,
		KEYCODE_R               = 46,
		KEYCODE_S               = 47,
		KEYCODE_T               = 48,
		KEYCODE_U               = 49,
		KEYCODE_V               = 50,
		KEYCODE_W               = 51,
		KEYCODE_X               = 52,
		KEYCODE_Y               = 53,
		KEYCODE_Z               = 54,
		KEYCODE_COMMA           = 55,
		KEYCODE_PERIOD          = 56,
		KEYCODE_ALT_LEFT        = 57,
		KEYCODE_ALT_RIGHT       = 58,
		KEYCODE_SHIFT_LEFT      = 59,
		KEYCODE_SHIFT_RIGHT     = 60,
		KEYCODE_TAB             = 61,
		KEYCODE_SPACE           = 62,
		KEYCODE_SYM             = 63,
		KEYCODE_EXPLORER        = 64,
		KEYCODE_ENVELOPE        = 65,
		KEYCODE_ENTER           = 66,
		KEYCODE_DEL             = 67,
		KEYCODE_GRAVE           = 68,
		KEYCODE_MINUS           = 69,
		KEYCODE_EQUALS          = 70,
		KEYCODE_LEFT_BRACKET    = 71,
		KEYCODE_RIGHT_BRACKET   = 72,
		KEYCODE_BACKSLASH       = 73,
		KEYCODE_SEMICOLON       = 74,
		KEYCODE_APOSTROPHE      = 75,
		KEYCODE_SLASH           = 76,
		KEYCODE_AT              = 77,
		KEYCODE_NUM             = 78,
		KEYCODE_HEADSETHOOK     = 79,
		KEYCODE_FOCUS           = 80,   // *Camera* focus
		KEYCODE_PLUS            = 81,
		KEYCODE_MENU            = 82,
		KEYCODE_NOTIFICATION    = 83,
		KEYCODE_SEARCH          = 84,
		KEYCODE_MEDIA_PLAY_PAUSE= 85,
		KEYCODE_MEDIA_STOP      = 86,
		KEYCODE_MEDIA_NEXT      = 87,
		KEYCODE_MEDIA_PREVIOUS  = 88,
		KEYCODE_MEDIA_REWIND    = 89,
		KEYCODE_MEDIA_FAST_FORWARD = 90,
		KEYCODE_MUTE            = 91
};

enum {
	ACTION_DOWN = 0,
	ACTION_UP = 1,
    /**
     * {@link #getAction} value: multiple duplicate key events have
     * occurred in a row, or a complex string is being delivered.  If the
     * key code is not {#link {@link #KEYCODE_UNKNOWN} then the
     * {#link {@link #getRepeatCount()} method returns the number of times
     * the given key code should be executed.
     * Otherwise, if the key code {@link #KEYCODE_UNKNOWN}, then
     * this is a sequence of characters as returned by {@link #getCharacters}.
     */
	ACTION_MULTIPLE = 2,	// Multiple for keys
	ACTION_MOVE = 2,		// move for pointer events
	ACTION_CANCEL = 3,
	ACTION_OUTSIDE = 4,
	ACTION_POINTER_DOWN = 5,
	ACTION_POINTER_UP = 6,

	ACTION_MASK = 0x00ff,
	ACTION_POINTER_ID_MASK = 0xff00,
	ACTION_POINTER_ID_SHIFT = 8
};

enum {
	META_SHIFT_ON = 1,
	META_ALT_ON	= 2,
	META_SYM_ON	= 4,
	META_ALT_LEFT_ON = 16,
	META_ALT_RIGHT_ON = 32,
	META_SHIFT_LEFT_ON = 64,
	META_SHIFT_RIGHT_ON = 128
};



void motionMovedCB(struct FskTimeCallBackRecord *callback, const FskTime time, void *param) {
	FskEvent ev = NULL;
	FskErr err = kFskErrNone;
	FskWindow win = FskWindowGetActive();
	FskTimeRecord mouseTimeR = {0, 0};
	int index = 0;

	// this should still be okay.
	if (gNumPts > 0) {
		FskTimeAddMS(&mouseTimeR, gPts[gNumPts - 1].ticks);
		FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, "touchMovedTimer - blasting %d points @ time[%d,%d]",gNumPts, mouseTimeR.seconds, mouseTimeR.useconds);

		FskEventNew(&ev, kFskEventMouseMoved, &mouseTimeR, kFskEventModifierNone);
		FskEventParameterAdd(ev, kFskEventParameterMouseLocation, gNumPts * sizeof(FskPointAndTicksRecord), gPts);

		index = 0;
		FskEventParameterAdd(ev, kFskEventParameterCommand, sizeof(index), &index);
		androidDoOrQueue(win, ev);

		FskMemPtrDispose(gPts);
		gPts = NULL;
		gNumPts = 0;
	}

	if (anyMouseDown())
		FskTimeCallbackScheduleFuture(touchMovedTimer, 0, win->updateInterval, motionMovedCB, win);
	else {
		FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, "no mouse down - stop motionMovedCB");
		FskWindowCancelStillDownEvents(win);
		FskTimeCallbackRemove(touchMovedTimer);
		touchMovedTimer = NULL;
	}
}

#if SUPPORT_INSTRUMENTATION
static const char *eventNames[] = {
	"ACTION_DOWN",
	"ACTION_UP",
	"ACTION_MOVE",
	"ACTION_CANCEL",
	"ACTION_OUTSIDE",
	"ACTION_POINTER_DOWN",
	"ACTION_POINTER_UP",
	""};
static const char *andEventName(int i) {
	if (i < 7)
		return eventNames[i];
	return "unknown";
}
#endif

void sendResidualMouseUps(void) {
	int i;
	FskErr err;
	FskTimeRecord mouseTimeR = {0, 0};
	FskEvent ev = NULL;
	FskPointAndTicksRecord pt;
	FskWindow win = FskWindowGetActive();
	FskEventCodeEnum cod = kFskEventMouseUp;

	for (i=0; i<NUM_TOUCH_POINTERS; i++) {
		while (glastDown[i] > 0) {
			FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, "pointer [%d] was still down %d times", i, glastDown[i]);
			FskEventNew(&ev, cod, &mouseTimeR, kFskEventModifierNone);
			FskEventParameterAdd(ev, kFskEventParameterCommand, sizeof(i), &i);
			androidDoOrQueue(win, ev);
			glastDown[i]--;
		}
		if (glastDown[i] < 0) {
			FskInstrumentedTypePrintfMinimal(&gAndroidTouchTypeInstrumentation, "pointer [%d] still down went negative %d !?!?!", i, glastDown[i]);
			glastDown[i] = 0;
		}

	}
}

Boolean anyMouseDown() {
	Boolean ret = false;
	int i;
	for (i=0; i<NUM_TOUCH_POINTERS; i++)
		if (glastDown[i] > 0)
			ret = true;

	return ret;
}

int trackMouseUp(int pointer) {
	glastDown[pointer]--;
	if (glastDown[pointer] < 0) {
		glastDown[pointer] = 0;
		FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, "trackMouseUp went negative - let caller know");
		return 0;
	}
	return 1;
}

void trackMouseDown(int pointer) {
	glastDown[pointer]++;
}

void JAVANAME(KinomaPlay_setAndroidBasetime)(JNIEnv* env, jobject thiz, jint s, jint ms) {
	baseTimeSeconds = s;
	baseTimeUseconds = ms * kFskTimeUsecPerMsec;
}


// this routine converts native events to FskEvents for multi-touch
jboolean
JAVANAME(KinomaPlay_doFskMotionTouch)(JNIEnv* env, jclass clazz, jint action, jint pointer, jint x, jint y, jint deltaMS) {
	FskWindow win = FskWindowGetActive();
	FskErr err = kFskErrNone;
	int i;
    FskEventCodeEnum cod = kFskEventNone;
    FskEventModifierEnum mod = kFskEventModifierNone;
	UInt32 clicks = 0;
	FskTimeRecord mouseTimeR = {0, 0};

	if (NULL == win)
		return false;

	action &= ACTION_MASK;

//	if (x != LIMIT_SCREEN_X(x)) {
//		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, " -- MOTION TOUCH REACHED LIMIT ON X (was %d - now %d)", x, LIMIT_SCREEN_X(x));
//	}
//	if (y != LIMIT_SCREEN_Y(y)) {
//		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, " -- MOTION TOUCH REACHED LIMIT ON Y (was %d - now %d)", y, LIMIT_SCREEN_Y(y));
//	}

	x = LIMIT_SCREEN_X(x);
	y = LIMIT_SCREEN_Y(y);

	glastX[pointer] = x;		// unscaled
	glastY[pointer] = y;

	// MDK - MOVE never gets here - in KinomaPlay.java,
	//		 MOVE goes to doFskMotionMultipleTouch
	// if (action == ACTION_MOVE)

	FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, "doFskMotionTouch pointer %d - %d:%s, %d,%d, evtTime: %d", pointer, action, andEventName(action), x, y, deltaMS);

	switch (action) {
		case ACTION_CANCEL:
			FskWindowCancelStillDownEvents(win);
			FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, "Got an ACTION_CANCEL - try sendResidualMouseUps");
			sendResidualMouseUps();
			return false;
		case ACTION_UP:
		case ACTION_POINTER_UP:
			cod = kFskEventMouseUp;
// don't remove stilldown, because we're only here if we've got multiple pointers
//			FskWindowCancelStillDownEvents(win);
//			FskTimeCallbackRemove(touchMovedTimer);
//			touchMovedTimer = NULL;

			motionMovedCB(NULL, NULL, NULL);

			if (0 == trackMouseUp(pointer)) {
				FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, "TOUCH_UP went negative - someone must have tossed one - bail");
				return true;
			}
			break;
		case ACTION_DOWN:
		case ACTION_POINTER_DOWN:
			//FskTimeGetNow(&DOWNTIME);
			mod = kFskEventModifierMouseButton;
			cod = kFskEventMouseDown;
			clicks = 1;
			if (!touchMovedTimer) {
				FskTimeCallbackNew(&touchMovedTimer);
				FskTimeCallbackScheduleFuture(touchMovedTimer, 0, win->updateInterval, motionMovedCB, win);
			}
			trackMouseDown(pointer);
			touchScale = FskPortScaleGet(win->port);
			break;
		case ACTION_OUTSIDE:
			cod = kFskEventMouseLeave;
			break;
		default:
			FskInstrumentedTypePrintfMinimal(&gAndroidWindowTypeInstrumentation, "UNKNOWN ACTION %d (%x) in FskMotionMoved", action, action);
			return false;
	}

	FskTimeAddMS(&mouseTimeR, deltaMS);

	FskEvent ev = NULL;
	FskPointAndTicksRecord pt;
	FskFixed v;

	FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, " - motionTouch - make and send event %d for pointer %d", cod, pointer);
	err = FskEventNew(&ev, cod, &mouseTimeR, mod);
	if (err) {
		FskInstrumentedTypePrintfVerbose(&gAndroidEventTypeInstrumentation, " - motionTouch(event new err:%d", err);
		return false;
	}

	pt.pt.x = x;
	pt.pt.y = y;
	pt.ticks = deltaMS;
	pt.index = pointer;

	// scale
	v = x << 16;
	pt.pt.x = FskFixDiv(v, touchScale) >> 16;
	v = y << 16;
	pt.pt.y = FskFixDiv(v, touchScale) >> 16;

	FskEventParameterAdd(ev, kFskEventParameterMouseLocation, sizeof(pt), &pt);
	if (clicks) {
		FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, " clicks: %d", clicks);
		FskEventParameterAdd(ev, kFskEventParameterMouseClickNumber, sizeof(clicks), &clicks);
	}
	FskEventParameterAdd(ev, kFskEventParameterCommand, sizeof(pointer), &pointer);

	FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, "doing FskMotionTouch event %d for pointer %d - x:%d y:%d", cod, pointer, pt.pt.x, pt.pt.y);

	androidDoOrQueue(win, ev);

	return true;
}


jboolean JAVANAME(KinomaPlay_doFskMotionMultipleTouch)(JNIEnv* env, jclass clazz, jintArray arr) {
	jsize len = env->GetArrayLength(arr);
	jint *body;
	int num, cur = 0;

	FskMemPtrNew(len * sizeof(jsize), (FskMemPtr*)&body);
	env->GetIntArrayRegion(arr, 0, len, body);

	num = len / 4;
	for (int i=0; i<num; i++) {
		int pointer = body[cur++];
		int x = body[cur++];
		int y = body[cur++];
		int ms = body[cur++];

		FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, "doFskMotionMultipleTouch addMoved [%d] %d - %d,%d", ms, pointer, x, y);

//		if (x != LIMIT_SCREEN_X(x)) {
//			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, " -- REACHED LIMIT ON X (was %d - now %d)", x, LIMIT_SCREEN_X(x));
//		}
//		if (y != LIMIT_SCREEN_Y(y)) {
//			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, " -- REACHED LIMIT ON Y (was %d - now %d)", y, LIMIT_SCREEN_Y(y));
//		}

		x = LIMIT_SCREEN_X(x);
		y = LIMIT_SCREEN_Y(y);

		addAMoved(x, y, ms, pointer);
	}

	FskMemPtrDispose(body);

	return true;
}

int addAMoved(int x, int y, int ms, int pointer) {
	FskErr err;
	FskWindow win = FskWindowGetActive();

	if (glastX[pointer] == x && glastY[pointer] == y) {
//		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "addAMoved - new point same as old point - toss");
		return kFskErrNone;
	}
	glastX[pointer] = x;
	glastY[pointer] = y;
	if (gPts) {
		gNumPts += 1;
		err = FskMemPtrRealloc(gNumPts * sizeof(FskPointAndTicksRecord), &gPts);
		if (err) goto bail;
	}
	else {
		gNumPts = 1;
		err = FskMemPtrNew(sizeof(FskPointAndTicksRecord), &gPts);
		if (err) goto bail;
	}

	// scale x, y here.
	if (win) {
		FskFixed v;

		v = x << 16;
		x = FskFixDiv(v, touchScale) >> 16;
		v = y << 16;
		y = FskFixDiv(v, touchScale) >> 16;
	}

	gPts[gNumPts-1].ticks = ms;
	gPts[gNumPts-1].pt.x = x;
	gPts[gNumPts-1].pt.y = y;
	gPts[gNumPts-1].index = pointer;

bail:
	if (gNumPts >= 18) {
//		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "points >= 18 -- pushing events");
		motionMovedCB(NULL, NULL, NULL);
	}

	return err;
}


FskErr androidDoOrQueue(FskWindow window, FskEvent event) {
	FskErr err = kFskErrNone;

	FskInstrumentedTypePrintfVerbose(&gAndroidEventTypeInstrumentation, "androidDoOrQueue %d", event->eventCode);

	if (event->eventCode == kFskEventMouseMoved) {
		FskListElement l;
		FskEvent lastMoved = NULL;
		FskListMutexAcquireMutex(window->eventQueue);
		l = FskListGetNext(window->eventQueue->list, NULL);
		while (l) {
			FskEvent ev = (FskEvent)l;
			if (ev->eventCode == kFskEventMouseMoved) {
				//if (lastMoved)
				//	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "was going to add to this mouse moved");
				lastMoved = ev;
			}
			else if ((ev->eventCode == kFskEventMouseUp)
					|| (ev->eventCode == kFskEventMouseDown)) {
				//if (lastMoved)
				//	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "was going to add to this mouse moved, but then got a mouse up");
				lastMoved = NULL;
			}
			l = FskListGetNext(window->eventQueue->list, l);
		}

		if (lastMoved) {
			char *paramData, *p;
			UInt32 oldSize, addSize, newSize;

			FskInstrumentedTypePrintfDebug(&gAndroidTouchTypeInstrumentation, "the last event type (%d) in the queue is getting this added", event->eventCode);
			FskEventParameterGetSize(lastMoved, kFskEventParameterMouseLocation, &oldSize);
			FskEventParameterGetSize(event, kFskEventParameterMouseLocation, &addSize);
			newSize = oldSize + addSize;
			if (kFskErrNone != (err = FskMemPtrNew(newSize, &paramData)))
				goto bail;
			if (kFskErrNone != (err = FskEventParameterGet(lastMoved, kFskEventParameterMouseLocation, paramData)))
				goto bail;
			FskEventParameterRemove(lastMoved, kFskEventParameterMouseLocation);
			p = paramData + oldSize;
			err = FskEventParameterGet(event, kFskEventParameterMouseLocation, p);
			FskEventParameterAdd(lastMoved, kFskEventParameterMouseLocation, newSize, paramData);
			FskMemPtrDispose(paramData);

			FskEventDispose(event);
			FskListMutexReleaseMutex(window->eventQueue);
			goto done;		// the event was merged with the "lastMoved" event

bail:
			// we weren't able to merge
			if (paramData)
				FskMemPtrDispose(paramData);
		}

		FskListMutexReleaseMutex(window->eventQueue);
	}
	if (FskListMutexIsEmpty(window->eventQueue)) {
		//FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "sending motion event to window");
		err = FskWindowEventSend(window, event);
	}
	else {
		err = FskWindowEventQueue(window, event);
		//FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "enqueued motion event to window - yielding");
		FskThreadYield();
	}

done:
	return err;
}

jboolean
JAVANAME(RemoteControlReceiver_doFskKeyEvent)(JNIEnv* env, jclass clazz, jint keyCode, jint modifiers, jint action, jint param, jint repeat) {
	return JAVANAME(KinomaPlay_doFskKeyEvent)(env, clazz, keyCode, modifiers, action, param, repeat);
}

jboolean
JAVANAME(KinomaPlay_doFskKeyEvent)(JNIEnv* env, jclass clazz, jint keyCode, jint modifiers, jint action, jint param, jint repeat) {
	FskEvent ev = NULL;
	FskWindow win = NULL;
	FskErr err = kFskErrNone;
    FskEventCodeEnum cod = kFskEventNone;
	int funcKey = 0, pulse = 0;
    FskEventModifierEnum mod = kFskEventModifierNone;


	win = FskWindowGetActive();
	if (!win)
		return false;

	FskInstrumentedTypePrintfDebug(&gAndroidEventTypeInstrumentation, "doFskKeyEvent action:%d repeat:%d keycode:%d", action, repeat, keyCode);
	switch (action) {
		case ACTION_DOWN: cod = kFskEventKeyDown; break;
		case ACTION_UP: cod = kFskEventKeyUp; break;
		case ACTION_MULTIPLE:
			if (KEYCODE_UNKNOWN != keyCode) {
				FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "ACTION_MULTIPLE - %c %d times", param, repeat);
				cod = kFskEventKeyUp;
			}
			else
				FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "ACTION_MULTIPLE - different");
 			break;
		default:
			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "don't know the key action: %d", action);
			return false;
	}
	if (modifiers & META_SHIFT_ON) mod = (FskEventModifierEnum)((UInt32)mod + (UInt32)kFskEventModifierShift);
	if (modifiers & META_ALT_ON) mod = (FskEventModifierEnum)((UInt32)mod + (UInt32)kFskEventModifierAlt);
	if (modifiers & META_SYM_ON) mod = (FskEventModifierEnum)((UInt32)mod + (UInt32)kFskEventModifierControl);

	if (!param) {
		switch (keyCode) {
			case KEYCODE_DPAD_UP:		param = 30; break;
			case KEYCODE_DPAD_DOWN:		param = 31; break;
			case KEYCODE_DPAD_LEFT:		param = 28; break;
			case KEYCODE_DPAD_RIGHT:	param = 29; break;
			case KEYCODE_DPAD_CENTER:	param = 13; break;
			case KEYCODE_ENTER: 		param = 13; break;
			case KEYCODE_VOLUME_UP:		funcKey = 6; break;
			case KEYCODE_VOLUME_DOWN:	funcKey = 7; break;
			case KEYCODE_SOFT_LEFT: 	funcKey = 1; break;
			case KEYCODE_SOFT_RIGHT: 	funcKey = 2; break;
			case KEYCODE_BACK: 			param = 8; break;
//			case KEYCODE_BACK: 			funcKey = 65541; break;	// go to home
			case KEYCODE_CALL: 			funcKey = 15; break;
			case KEYCODE_ENDCALL: 		funcKey = 14; break;
			case KEYCODE_DEL: 			param = 8; break;
			case KEYCODE_TAB:			param = 9; break;
			case KEYCODE_MENU:
				if (staticButtonsMirrored)
					funcKey = 1;
				else
					funcKey = 2;
				break;
			case KEYCODE_HOME:			funcKey = kFskEventFunctionKeyHome; pulse = 1; break;
			case KEYCODE_MEDIA_PLAY_PAUSE:	funcKey = kFskEventFunctionKeyTogglePlayPause; pulse = 1; break;
			case KEYCODE_MEDIA_PREVIOUS:	funcKey = kFskEventFunctionKeyPreviousTrack; pulse = 1; break;
			case KEYCODE_MEDIA_NEXT:		funcKey = kFskEventFunctionKeyNextTrack; pulse = 1; break;
			case KEYCODE_MEDIA_STOP:		funcKey = kFskEventFunctionKeyStop; pulse = 1; break;
//			case KEYCODE_MEDIA_FAST_FORWARD:	funcKey = ??; break;
//			case KEYCODE_MEDIA_REWIND:		funcKey = ??; break;
//			KEYCODE_HOME            = 3,

			case KEYCODE_SEARCH:		funcKey = kFskEventFunctionKeySearch; break;

		}
	}
	if(param == 10) param = 13;		// hack - keyboard Return => enter

#define TRY_PULSE 1
#if TRY_PULSE
	if (pulse || repeat) {
		do {
			if (cod == kFskEventKeyUp) {
				char utf[2];
				utf[0] = 0;
				utf[1] = '\0';
				cod = kFskEventKeyDown;
				if (kFskErrNone == FskEventNew(&ev, cod, NULL, mod)) {
					FskEventParameterAdd(ev, kFskEventParameterKeyUTF8, 2, &utf);
					if (funcKey) {
						FskInstrumentedTypePrintfDebug(&gAndroidEventTypeInstrumentation, " - posting KeyDown funcKey %d", funcKey);
						FskEventParameterAdd(ev, kFskEventParameterFunctionKey, sizeof(funcKey), &funcKey);
					}
					FskInstrumentedTypePrintfDebug(&gAndroidEventTypeInstrumentation, " - keyCode: %d fskEvtCod:%d param:%d funcKey: %d", keyCode, cod, param, funcKey);
					androidDoOrQueue(win, ev);
				}
				cod = kFskEventKeyUp;
				if (kFskErrNone == FskEventNew(&ev, cod, NULL, kFskEventModifierNone)) {
					FskEventParameterAdd(ev, kFskEventParameterKeyUTF8, 2, &utf);
					if (funcKey) {
						FskInstrumentedTypePrintfDebug(&gAndroidEventTypeInstrumentation, " -  posting KeyUp funcKey %d", funcKey);
						FskEventParameterAdd(ev, kFskEventParameterFunctionKey, sizeof(funcKey), &funcKey);
					}
					FskInstrumentedTypePrintfDebug(&gAndroidEventTypeInstrumentation, " - keyCode: %d fskEvtCod:%d param:%d funcKey: %d", keyCode, cod, param, funcKey);
					androidDoOrQueue(win, ev);
				}
			}
		} while (--repeat > 0);
		return true;
	}
	else
#endif
	if ((param || funcKey) && (cod == kFskEventKeyDown || cod == kFskEventKeyUp)) {
		char utf[2];
		utf[0] = param;
		utf[1] = '\0';
		if (kFskErrNone == FskEventNew(&ev, cod, NULL, mod)) {
			FskEventParameterAdd(ev, kFskEventParameterKeyUTF8, 2, &utf);
			if (funcKey) {
				FskInstrumentedTypePrintfDebug(&gAndroidEventTypeInstrumentation, " - funcKey %d", funcKey);
				FskEventParameterAdd(ev, kFskEventParameterFunctionKey, sizeof(funcKey), &funcKey);
			}
			FskInstrumentedTypePrintfDebug(&gAndroidEventTypeInstrumentation, " - keyCode: %d fskEvtCod:%d param:%d funcKey: %d", keyCode, cod, param, funcKey);
   			androidDoOrQueue(win, ev);
			return true;
		}
	}
	return false;
}

#if SUPPORT_REMOTE_NOTIFICATION
static void
sendRemoteNotificationEvent(const char *str, Boolean isRegistration) {
	FskEvent ev;
	FskEventCodeEnum evType = isRegistration ? kFskEventSystemRemoteNotificationRegistered : kFskEventSystemRemoteNotification;

	if (FskEventNew(&ev, evType, NULL, kFskEventModifierNotSet) == kFskErrNone) {
		FskWindow win = FskWindowGetActive();
		UInt32 strLen = FskStrLen(str);
		if (strLen > 0) {
			(void)FskEventParameterAdd(ev, kFskEventParameterStringValue, FskStrLen(str) + 1, str);
		}
		//FskWindowEventSend(win, ev);	// Window??
		FskWindowEventQueue(win, ev);	// Window??
	}
}

void
JAVANAME(KinomaPlay_doFskOnRemoteNotificationRegistered)(JNIEnv* env, jclass clazz, jstring str)
{
	FskEvent ev;

	FskInstrumentedTypePrintfDebug(&gAndroidEventTypeInstrumentation, "doFskOnRemoteNotificationRegistered");
	//FskInstrumentedTypePrintfDebug(&gAndroidEventTypeInstrumentation, "doFskOnRemoteNotificationRegistered %s", str ? (char *)env->GetStringUTFChars(str, NULL) : "null");

	sendRemoteNotificationEvent(str ? env->GetStringUTFChars(str, NULL) : NULL, true);
}

void
JAVANAME(KinomaPlay_doFskOnRemoteNotification)(JNIEnv* env, jclass clazz, jstring str)
{
	const char *json = env->GetStringUTFChars(str, NULL);
	UInt32 jsonLen = FskStrLen(json);

	if (jsonLen == 0) {
		return;
	}

	sendRemoteNotificationEvent(json, false);
}

void
JAVANAME(KinomaPlay_checkLaunched)(JNIEnv* env, jclass clazz)
{
}
#endif

/* called from VM thread */

// this should be within a gScreenMutex lock (by the caller)

void androidCheckSizeChangeComplete() {
	if (gPendingSizeChangeDone) {
		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "androidCheckSizeChangeComplete - gPendingSizeChangeDone(%d) pendingDoFskSurfaceChanged(%d)", gPendingSizeChangeDone, pendingDoFskSurfaceChanged);
		if (0 == --gPendingSizeChangeDone) {
			if (fbGlobals->midSizeChange) {
				FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "fbGlobals->midSizeChange(%d) =--- lower to %d",  fbGlobals->midSizeChange, fbGlobals->midSizeChange - 1);
				fbGlobals->midSizeChange--;
				if (fbGlobals->midSizeChange == 0) {
					FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, " - checkSizeChangeDone - size change reduced to 0 - copy from backing buffer %x to frame buffer %x", fbGlobals->backingBuffer, fbGlobals->frameBuffer);
					if (pendingOrientation != -1) {
						if ( ((myHWInfo.orientation == 1 || myHWInfo.orientation == 3) && pendingOrientation == 0)
						  || ((pendingOrientation == 1 || pendingOrientation == 3) && myHWInfo.orientation == 0) ) {
							FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "Orientation - Pending: %d, cur: %d - CHANGED", pendingOrientation, myHWInfo.orientation);
							myHWInfo.orientation = pendingOrientation;
							FskSetPhoneHWInfo(&myHWInfo);
							pendingOrientation = -1;
						}
						else {
							FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "Orientation - Pending: %d, cur: %d - CHANGED - but equivalent", pendingOrientation, myHWInfo.orientation);
						}
					}

					if (gPendingSizeChange) {
						FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "XXX gPendingSizeChangeDone - WAS GOING TO gPendingKbdSelection %d %d - but PendingSizeChange so no.", gPendingSetKbdSelectionStart, gPendingSetKbdSelectionEnd);
					}
					else if (gPendingSetKbdSelection) {
						FskInstrumentedTypePrintfDebug(&gAndroidTETypeInstrumentation, "XXX gPendingSizeChangeDone - gPendingKbdSelection %d %d - do it", gPendingSetKbdSelectionStart, gPendingSetKbdSelectionEnd);
						gPendingSetKbdSelection = 0;
						androidTweakKeyboardSelection(gPendingSetKbdSelectionStart, gPendingSetKbdSelectionEnd);
					}
				}
			}

			if (gPendingSystemBar) {
				FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "XXX gPendingSizeChangeDone - gPendingSystemBar - do it");
				gPendingSystemBar = 0;
				androidSystemBarShow(gPendingSystemBarShow);
			}
		}
	}
	else if (pendingDoFskSurfaceChanged) {
		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "androidCheckSizeChangeComplete - pendingDoFskSurfaceChanged still pending %d", pendingDoFskSurfaceChanged);
	}

}

int androidMidWindowResize() {
	if (fbGlobals->midSizeChange) {
		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "get midWindowResize is  - %d", fbGlobals->midSizeChange);
	}
	return fbGlobals->midSizeChange;
}

/* called from VM thread */
void androidAfterWindowResize() {
	FskWindow win = FskWindowGetActive();
	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "androidAfterWindowResize - about to sendWindowUpdate - win (%x)", win);

	if (!win) return;

	// MDK? still need? --  need this here or we'll flash
//	sendEventWindowUpdate(win, true, true);

	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "androidAfterWindowResize - fbGlobals->surfaceLocked: %d", fbGlobals->surfaceLocked);

	gPendingSizeChangeDone = 1;

	if (0 == fbGlobals->surfaceLocked && 0 == fbGlobals->backBufferLocked) {
		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "androidAfterWindowResize - gPendingSizeChangeDone - nothing locked do immediate.");
		androidCheckSizeChangeComplete();
	}
	else {
		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "androidAfterWindowResize -buffer locked, let unlock handle gPendingSizeChangeDone");
	}

	//MDK still need this invalidation?
#if 1
	FskRectangleRecord b;
	FskPortGetBounds(win->port, &b);
	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "XXXX - NOW invalidating port bounds %d %d %d %d", b.x, b.y, b.width, b.height);
	FskPortInvalidateRectangle(win->port, &b);
//	FskThreadWake(win->thread);
#endif

//FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "after FskPortInvalidateRectangle");

	/* Window resize done, resume the drawing pump */
	if (win->useFrameBufferUpdate) {
		win->updateSuspended = false;
	}
}


// doFskSurfaceChanged should be called in the thread that draws (kp5-vm)
//	We FskThreadPostCallback(win->thread, doFskSurfaceChanged, (void*)width, (void*)height, (void*)viewObj, NULL)
//	from FskView_doFskSurfaceChanged below.

void doFskSurfaceChanged(void *a, void *b, void *c, void *d)
{
    FskWindow win;
	int width  = (int)a;	/* Generic arguments, passed as pointers, large enough to hold pointers or ints. */
	int height = (int)b;
//	jobject viewObj = (jobject)c;
	Boolean needsNewBuffers = false;
	int myPendingSurfaceChanged;
	FskPointRecord myPendingSize;

	win = FskWindowGetActive();

	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "doFskSurfaceChanged(%d, %d, *, *) (should main be blocked starting here) frameBuffer was %d %d : bits: %x", width, height, fbGlobals->frameBuffer->bounds.width, fbGlobals->frameBuffer->bounds.height, fbGlobals->frameBuffer->bits);

	FskRectangleSet(&fbGlobals->frameBuffer->bounds, 0, 0, width, height);

	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\tdoFskSurfaceChanged - rowbytes was: %d will be: %d %s", fbGlobals->frameBuffer->rowBytes, width * 2, (fbGlobals->frameBuffer->rowBytes != width * 2) ? "DIFFERENT" : "");
	fbGlobals->frameBuffer->rowBytes = width * 2;

	gScreenWidth = width;
	gScreenHeight = height;
	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\t-- setting gScreenWidth: %d and height %d", gScreenWidth, gScreenHeight);

	if (pendingOrientation != -1) {
		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\t-- myHW.orientation - %d, pendingOrientation %d - prior to AndroidSizeChanged",  myHWInfo.orientation, pendingOrientation);
		if (pendingOrientation != myHWInfo.orientation) {
			myHWInfo.orientation = pendingOrientation;
			FskSetPhoneHWInfo(&myHWInfo);
		}
		pendingOrientation = -1;
	}

	/* About to send the window resize event, suspend the drawing pump if it's running */
	if (win->useFrameBufferUpdate) {
		win->updateSuspended = true;
	}

	FskWindowAndroidSizeChanged((int)win);	// This is where GL changes its screen size

	if (gPendingActivate) {
		androidDoWindowActivated();
	}

bail:

	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\tEND  doFskSurfaceChanged - %d %d (should main be blocked to here?)", width, height);

	(void)FskMutexAcquire(gSurfaceChangedMutex);
	myPendingSurfaceChanged = --pendingDoFskSurfaceChanged;
	myPendingSize = pendingDoFskSurfaceChangedSize;
	if (myPendingSurfaceChanged > 0) {
		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\tdoFskSurfaceChanged - still something pending %d (%d %d)", myPendingSurfaceChanged, myPendingSize.x, myPendingSize.y);
		if (pendingDoFskSurfaceChangedSize.x != gScreenWidth && pendingDoFskSurfaceChangedSize.y != gScreenHeight) {
			(void)FskMutexRelease(gSurfaceChangedMutex);
			doFskSurfaceChanged((void*)myPendingSize.x, (void*)myPendingSize.y, NULL, NULL);		/* TODO: Does this need to be recursive? */
			(void)FskMutexAcquire(gSurfaceChangedMutex);
		}
		else {
			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\tdoFskSurfaceChanged size was pending, but size same as now, so ignore.");
			pendingDoFskSurfaceChanged--;
			pendingDoFskSurfaceChangedSize.x = 0;
			pendingDoFskSurfaceChangedSize.y = 0;
		}
	}
	else {	/* myPendingSurfaceChanged <= 0 */
		if (myPendingSurfaceChanged < 0)
			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\tdoFskSurfaceChanged - MYPENDINGSURFACECHANGED IS NEGATIVE!!!! RESETTING to 0)", myPendingSurfaceChanged);
//		myPendingSurfaceChanged = 0;
		pendingDoFskSurfaceChangedSize.x = 0;
		pendingDoFskSurfaceChangedSize.y = 0;
	}
	(void)FskMutexRelease(gSurfaceChangedMutex);
	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "doFskSurfaceChanged -- returning");
}


jint JAVANAME(FskView_doFskSurfaceChanged)(JNIEnv* env, jobject viewObj, jint width, jint height) {
    FskWindow win;

	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "FskView_doFskSurfaceChanged(env=%p view=%p w=%d h=%d)", env, viewObj, width, height);

	(void)FskMutexAcquire(gSurfaceChangedMutex);
	if (pendingDoFskSurfaceChanged) {
		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\tFskView_doFskSurfaceChanged there was already a surfaceChangePending %d, (%d %d) do it later", pendingDoFskSurfaceChanged, width, height);
		if (width != pendingDoFskSurfaceChangedSize.x || height != pendingDoFskSurfaceChangedSize.y) {
			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "surfaceChangePending NOT same as other surfaceChangePending was %d %d - save for later %d %d", pendingDoFskSurfaceChangedSize.x, pendingDoFskSurfaceChangedSize.y, width, height);
			pendingDoFskSurfaceChanged++;
			pendingDoFskSurfaceChangedSize.x = width;
			pendingDoFskSurfaceChangedSize.y = height;
		}
		else {
			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\tsurfaceChangePending same as other surfaceChangePending - toss");
		}
		(void)FskMutexRelease(gSurfaceChangedMutex);
		goto bail;
	}
	(void)FskMutexRelease(gSurfaceChangedMutex);

	win = FskWindowGetActive();
	if (win) {
		FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "\tposting doFskSurfaceChanged callback to window's thread %s (should we lock main here?) ", win->thread->name);

		(void)FskMutexAcquire(gSurfaceChangedMutex);
		pendingDoFskSurfaceChanged++;
		(void)FskMutexRelease(gSurfaceChangedMutex);
		FskThreadPostCallback(win->thread, doFskSurfaceChanged, (void*)width, (void*)height, (void*)viewObj, NULL);
		if (gPendingActivate) {
			androidDoWindowActivated();
		}
	}

bail:
	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "FskView_doFskSurfaceChanged -- returning (should we have waited for kp5-vm thread here and unlocked here?)");
	return 0;
}


void androidSetFBGlobals(FskFBGlobals globals) {
	fbGlobals = globals;
}

FskTimeCallBack delayDismissKeyboard = NULL;

void dismissKbdCallback(struct FskTimeCallBackRecord *callback, const FskTime time, void *param) {
	fprintf(stderr, "[%s] dismissKbdCallback called\n", FskThreadGetCurrent()->name);
	doIMEEnable(-1);
	FskTimeCallbackDispose(delayDismissKeyboard);
	delayDismissKeyboard = NULL;
}

void JAVANAME(FskEditText_doDismissKeyboard)(JNIEnv *env, jclass clazz, jint dismiss) {

	fprintf(stderr, "FskEditText_doDismissKeyboard\n");
	if (doIsIMEEnabled()) {
		if (!delayDismissKeyboard) {
			fprintf(stderr, "creating delayDismissKeyboard callback\n");
			FskTimeCallbackNew(&delayDismissKeyboard);
		}
		fprintf(stderr, "[%s] posting future event for dismissKbdCallback\n", FskThreadGetCurrent()->name);
		FskTimeCallbackScheduleFuture(delayDismissKeyboard, 0, 1, dismissKbdCallback, (void*)1);
	}
	else {
		fprintf(stderr, "[%s] posting KeyEvent for back\n", FskThreadGetCurrent()->name);
		JAVANAME(KinomaPlay_doFskKeyEvent)(env, clazz, KEYCODE_BACK, 0, ACTION_UP, 0, 0);

	}
}
