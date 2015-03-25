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

#include <android/log.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "cpu-features.h"
#include "jni.h"

#define DEBUG 1

#define __FSKWINDOW_PRIV__
#define __FSKTHREAD_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKPORT_PRIV__
#define __FSKEVENT_PRIV__	// for debugging

#include "KinomaInterfaceLib.h"

#include "FskEndian.h"
#if SUPPORT_REMOTE_NOTIFICATION
#include "FskEnvironment.h"
#endif
#include "FskHardware.h"
#include "FskMain.h"
#include "FskNetInterface.h"
#if FSKBITMAP_OPENGL
	#include "FskGLBlit.h"
	#include <android/native_window_jni.h>
#endif /* FSKBITMAP_OPENGL */


#define mY( x, y ) x##y
#define mYY( x, y ) mY( x, y )
#define JAVANAME( x ) mYY( OBJECTBASE , x )

#if (USE_JAVACLASS == 1)
	#define JAVACLASS_KINOMAPLAY "com/kinoma/kinomaplay/KinomaPlay"
	#define JAVACLASS_PLAY2ANDROID "com/kinoma/kinomaplay/Play2Android"
#endif


// for callFsk
enum {
	kJNIStartFsk = 1,
	kJNIIdleFsk = 2,
	kJNIStopFsk = 3,
	kJNISetupForCallbacks = 4,
	kJNIFskInvalWindow = 5,
	kJNILowMemory = 6,
	kJNINetworkChanged = 7,
	kJNIFskReceivedSMS = 8,
	kJNIWindowDeactivated = 9,
	kJNIWindowActivated = 10,
	kJNIVolumeMounted = 11,
	kJNIVolumeEjected = 12,
	kJNIPackagesChanged = 13,
	kJNIExtendAudioBuffer = 14,
	kJNIReduceAudioBuffer = 15
};

// for callFskInt
enum {
	kSetVolume	= 1,
	kSetOpenGL	= 2,
	kGetOpenGL	= 3,
	kSetKeyboardState = 4
};

int androidMaxVolume	= 0;
int androidLastVolume	= -1;

int usingOpenGL	= 0;

SInt32 gMainWaitReturnTime = 0;
int gInitStarted = 0;
extern int pendingOrientation;


int appsCookie = 1;		// we need to listen to package manager changes and update apps when necessary.
int curCookie = 0;
char *lastApps = NULL;


extern int staticHdpi;
extern int staticVdpi;
extern int staticDensityDpi;
extern FILE *gCustDebugFile;

extern Boolean gNeedsOrientationRotate;

int gSystemBarHeight = 38;
Boolean gSystemBarShowing = true;


int gAndroidIfaceCount = 2;
FskNetInterfaceRecord gAndroidIface[2];

extern FskFBGlobals fbGlobals;

extern gPendingSystemBar;
extern gPendingSystemBarShow;
extern int gPendingActivate;
extern int gPendingSizeChange;

extern int gTESelectionStart;
extern int gTESelectionEnd;
extern int gTEIgnoreChanged;

Boolean gLastIME = false;

char *doFetchContacts(int style);
void androidSetTransitionState(int state);
extern int gAndroidTransitionState;
extern int gPendingIMEEnable;

FskCondition jniRespCond = NULL;
FskMutex jniRespMutex = NULL;
FskMutex gSurfaceChangedMutex = NULL;

static JNIEnv *gEnv = NULL;
static jclass gKinomaPlayClass = NULL;
static jobject gKinomaPlayObject = NULL;
static jmethodID methodID_launchDoc;
static jmethodID methodID_fskJNIControl;
static jmethodID methodID_fskJNIFetch;
static jmethodID methodID_androidIntent;
static jmethodID methodID_androidIntentClass;
static jfieldID fieldID_mCallLogString;

#ifdef ANDROID_PERMISSION_READ_CONTACTS
static jmethodID methodID_doFetchContacts;
static jmethodID methodID_doStartContacts;
static jmethodID methodID_doStopContacts;
static jmethodID methodID_doNextContact;
static jmethodID methodID_doIdxContact;
static jfieldID fieldID_mContactSet;
#endif
#ifdef ANDROID_PERMISSION_CALL_PHONE
static jmethodID methodID_doDial;
#endif
#ifdef ANDROID_PERMISSION_SEND_SMS
static jmethodID methodID_doSendSMS;
#endif
static jmethodID methodID_doSendMail;
static jmethodID methodID_doSendAttach;

static jmethodID methodID_doIsIMEEnabled;
static jmethodID methodID_doIMEEnable;
static jmethodID methodID_doSetKeyboardStuff;
static jmethodID methodID_doSetKeyboardHints;
static jmethodID methodID_doSetKeyboardSelection;
static jmethodID methodID_doSetTERect;

static jmethodID methodID_doFetchApps;
static jmethodID methodID_doLaunchApp;
static jmethodID methodID_doFetchAppIcon;
static jfieldID	fieldID_mLaunchables;
static jfieldID	fieldID_mIconBuffer;

static jmethodID methodID_webviewCreate;
static jmethodID methodID_webviewDispose;
static jmethodID methodID_webviewActivated;
static jmethodID methodID_webviewSetFrame;
static jmethodID methodID_webviewAttach;
static jmethodID methodID_webviewDetach;
static jmethodID methodID_webviewGetURL;
static jmethodID methodID_webviewSetURL;
static jmethodID methodID_webviewEvaluateScript;
static jmethodID methodID_webviewReload;
static jmethodID methodID_webviewBack;
static jmethodID methodID_webviewForward;
static jmethodID methodID_webviewCanBack;
static jmethodID methodID_webviewCanForward;

static jmethodID methodID_libraryThumbnail;
static jmethodID methodID_libraryFetch;
static jmethodID methodID_libraryGetSongPath;
static jmethodID methodID_librarySaveImage;

static jmethodID methodID_setContinuousDrawing;

static jobject gPlay2AndroidObject;
const char *kPlay2AndroidPath = JAVACLASS_PLAY2ANDROID;
static JavaVM *gJavaVM;

extern int gScreenWidth, gScreenHeight;

FskAndroidCallbacksRecord myCallbacks;
FskPhoneHWInfoRecord myHWInfo;

extern char *staticIMEI;
extern char *staticUUID;
extern int baseTimeSeconds;
extern int baseTimeUseconds;

extern UInt32 gWindowUpdateInterval;

void myGetLanguage(char **lang);
void myGetBasetime(int *s, int *usec);

enum {
	kFskCallState = 1,
	kFskCellDataConnectionState = 2,
	kFskMessageWaitingState = 3,
	kFskServiceState = 4,
	kFskSignalStrength = 5,
	kFskBatteryLevel = 6,
	kFskBatteryPlugged = 7,
	kFskBacklightOn = 9,
	kFskCellDataConnectionType = 10,
	kFskDataConnectionState = 11,
	kFskNetworkEnabled = 12,
	kFskNetworkType = 13,
	kFskDataSignalStrength = 14,

	kFskNetworkWifiAddress = 9999,
};

#define JNI_CLASS_CONTROL	1
#define JNI_CLASS_FETCH		2

#define JNICONTROL_SHOW_SYSTEM_BAR		8
#define JNICONTROL_SET_VOLUME			10
#define JNICONTROL_RESET_ANDROID_HOME	11

#define JNIFETCH_VOLUME			6

#define kIntentCanDo	0
#define kIntentStart	1
#define kIntentClassCanDo	2
#define kIntentClassStart	3

typedef struct intentStructRecord {
	int what;
	char *action;
	char *uri;
	char *packageName;
	char *className;
} intentStructRecord, *intentStruct;


#if DEBUG

char *selectorStr(int class, int what) {
	switch (class) {
		case JNI_CLASS_CONTROL:
			switch (what) {
				case JNICONTROL_SLEEP: return "SLEEP";
				case JNICONTROL_GPS_ON: return "GPS On";
				case JNICONTROL_GPS_OFF: return "GPS Off";
				case JNICONTROL_SPLASH_OFF: return "Splash Off";
				case JNICONTROL_WAKE_MAIN: return "Wake Main";
				case JNICONTROL_KEEP_WIFI_ALIVE: return "Wifi Alive";
				case JNICONTROL_SHOW_SYSTEM_BAR: return "Show System Bar";
				case JNICONTROL_SET_VOLUME: return "Set Volume";
			}
			break;
		case JNI_CLASS_FETCH:
			switch (what) {
				case JNIFETCH_SD_MOUNTED: return "SD Mounted";
				case JNIFETCH_IME_ENABLED: return "IME Enabled";
				case JNIFETCH_PHONE_LOG_START: return "Start Phone Log";
				case JNIFETCH_PHONE_LOG_STOP: return "Stop Phone Log";
				case JNIFETCH_PHONE_LOG_NEXT: return "Next Phone Log entry";
				case JNIFETCH_VOLUME: return "Get Volume";
			}
			break;
	}
	return "unknown";
}

char *fskFn(int what) {
	switch (what) {
		case kJNIStartFsk:
			return "kJNIStartFsk";
		case kJNIIdleFsk:
			return "kJNIIdleFsk";
		case kJNIStopFsk:
			return "kJNIStopFsk";
		case kJNISetupForCallbacks:
			return "kJNISetupForCallbacks";
		case kJNIFskInvalWindow:
			return "kJNIFskInvalWindow";
		case kJNILowMemory:
			return "kJNILowMemory";
		case kJNINetworkChanged:
			return "kJNINetworkChanged";
		case kJNIFskReceivedSMS:
			return "kJNIFskReceivedSMS";
	}
	return "unknown";
}

char *phoneState(int what) {
	switch (what) {
		case kFskCallState:
			return "kFskCallState";
		case kFskCellDataConnectionState:
			return "kFskCellDataConnectionState";
		case kFskMessageWaitingState:
			return "kFskMessageWaitingState";
		case kFskServiceState:
			return "kFskServiceState";
		case kFskSignalStrength:
			return "kFskSignalStrength";
		case kFskBatteryLevel:
			return "kFskBatteryLevel";
		case kFskBatteryPlugged:
			return "kFskBatteryPlugged";
		case kFskBacklightOn:
			return "kFskBacklightOn";
		case kFskCellDataConnectionType:
			return "kFskCellDataConnectionType";
		case kFskDataConnectionState:
			return "kFskDataConnectionState";
		case kFskNetworkEnabled:
			return "kFskNetworkEnabled";
		case kFskNetworkType:
			return "kFskNetworkType";
		case kFskDataSignalStrength:
			return "kFskDataSignalStrength";
		case 9999:
			return "kFskNetworkWifiAddress";
	}
	return "unknown";
}
#endif

FskInstrumentedSimpleType(AndroidTE, androidte);
FskInstrumentedSimpleType(AndroidMainBlock, androidmainblock);
FskInstrumentedSimpleType(AndroidEnergy, androidenergy);
FskInstrumentedSimpleType(AndroidPhoneState, androidphonestate);
FskInstrumentedSimpleType(AndroidJNI, androidjni);
FskInstrumentedSimpleType(AndroidGlue, androidglue);
FskInstrumentedSimpleType(AndroidWindow, androidwindow);
FskInstrumentedSimpleType(AndroidTouch, androidtouch);
FskInstrumentedSimpleType(AndroidEvent, androidevent);




int GetJavaStringAsFskStringWithEnv(JNIEnv *env, jstring jstr, FskMemPtr *out) {
	FskMemPtr tmp;
	int len = (*env)->GetStringLength(env, jstr);
	FskInstrumentedTypePrintfDebug(&gAndroidJNITypeInstrumentation, "GetJavaStringAsFskString len %d", len);
	FskMemPtrNew((len + 1) * 2, &tmp);
	(*env)->GetStringRegion(env, jstr, 0, len, (jchar *)tmp);
	tmp[len * 2] = '\0';
	(void)FskTextUnicode16LEToUTF8(tmp, len * 2, out, NULL);
	FskMemPtrDispose(tmp);
	return len;
}


int GetJavaStringAsFskString(jstring jstr, FskMemPtr *out) {
	return GetJavaStringAsFskStringWithEnv(gEnv, jstr, out);
}


void androidDoWindowActivated() {
	FskEvent event;
	FskWindow win = FskWindowGetActive();

	if (!fbGlobals->surface && !GLHasASurface()) {
		gPendingActivate = 1;
		FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, " androidDoWindowActivated - no surface, pending");
	}
	else {
		gPendingActivate = 0;
		FskTimersSetUiActive(true);
		if (kFskErrNone == FskEventNew(&event, kFskEventWindowActivated, NULL, kFskEventModifierNotSet))
			FskWindowEventSend(win, event);
	}
}

void androidGetDPI(int *hDpi, int *vDpi, int *densityDpi) {
	if (hDpi)
		*hDpi = staticHdpi;
	if (vDpi)
		*vDpi = staticVdpi;
	if (densityDpi)
		*densityDpi = staticDensityDpi;
}

void androidDoWindowDeactivated() {
	FskEvent event;
	FskWindow win = FskWindowGetActive();

	FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "androidDoWindowDeactivated - activeWin: %x", win);

	sendResidualMouseUps();	// to prevent hangs when sleeping and screen is touched

	if (kFskErrNone == FskEventNew(&event, kFskEventWindowDeactivated, NULL, kFskEventModifierNotSet))
		FskWindowEventSend(win, event);
	FskTimersSetUiActive(false);
}

int androidNoWindowDontDraw() {
	if (NULL == fbGlobals->surface && !GLHasASurface())
		return 1;
	return 0;
}

Boolean androidIsUsingOpenGL(void) {
	return usingOpenGL;
}


jint JAVANAME(KinomaPlay_callFskInt)( JNIEnv* env, jobject thiz, jint what, jint val) {
	jint result = 0;
	switch (what) {
		case kSetVolume:
			androidLastVolume = val;
			break;
		case kSetOpenGL:
			usingOpenGL = val;
			break;
		case kGetOpenGL:
			result = usingOpenGL;
			break;
		case kSetKeyboardState:
			gLastIME = val ? true : false;
			break;
	}
	return result;
}

jmethodID getMethodID(jclass clazz, const char *method, const char *sig) {
	jmethodID methodID;
	methodID = (*gEnv)->GetMethodID(gEnv, clazz, method, sig);
	if (NULL == methodID) {
		FskInstrumentedTypePrintfNormal(&gAndroidJNITypeInstrumentation, "can't get methodID for %s %s", method, sig);
	}
}

jfieldID getFieldID(jclass clazz, const char *field, const char *sig) {
	jfieldID jfieldID;
	jfieldID = (*gEnv)->GetFieldID(gEnv, clazz, field, sig);
	if (NULL == jfieldID) {
		FskInstrumentedTypePrintfNormal(&gAndroidJNITypeInstrumentation, "can't get jfieldID for %s %s", field, sig);
	}
}

void initJNI() {
	methodID_launchDoc = getMethodID(gKinomaPlayClass, "launchDoc", "(ILjava/lang/String;)V");
	methodID_fskJNIFetch = getMethodID(gKinomaPlayClass, "fskJNIFetch", "(II)I");
	methodID_fskJNIControl = getMethodID(gKinomaPlayClass, "fskJNIControl", "(II)V");

	fieldID_mCallLogString = getFieldID(gKinomaPlayClass, "mCallLogString", "Ljava/lang/String;");

	methodID_androidIntent = getMethodID(gKinomaPlayClass, "androidIntent", "(ILjava/lang/String;Ljava/lang/String;)I");
	methodID_androidIntentClass = getMethodID(gKinomaPlayClass, "androidIntentClass", "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");

#ifdef ANDROID_PERMISSION_READ_CONTACTS
	methodID_doFetchContacts = getMethodID(gKinomaPlayClass, "doFetchContacts", "(I)V");
	methodID_doStartContacts = getMethodID(gKinomaPlayClass, "doStartContacts", "(I)I");
	methodID_doStopContacts = getMethodID(gKinomaPlayClass, "doStopContacts", "()V");
	methodID_doNextContact = getMethodID(gKinomaPlayClass, "doNextContact", "()V");
	methodID_doIdxContact = getMethodID(gKinomaPlayClass, "doIdxContact", "(I)V");
	fieldID_mContactSet = getFieldID(gKinomaPlayClass, "mContactSet", "Ljava/lang/String;");
#endif // ANDROID_PERMISSION_READ_CONTACTS
#ifdef ANDROID_PERMISSION_CALL_PHONE
	methodID_doDial = getMethodID(gKinomaPlayClass, "doDial", "(Ljava/lang/String;)V");
#endif // ANDROID_PERMISSION_CALL_PHONE
#ifdef ANDROID_PERMISSION_SEND_SMS
	methodID_doSendSMS = getMethodID(gKinomaPlayClass, "doSendSMS", "(Ljava/lang/String;Ljava/lang/String;)V");
#endif // ANDROID_PERMISSION_SEND_SMS
	methodID_doSendMail = getMethodID(gKinomaPlayClass, "doSendMail", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	methodID_doSendAttach = getMethodID(gKinomaPlayClass, "doSendAttach", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;I)V");

	methodID_doIsIMEEnabled = getMethodID(gKinomaPlayClass, "doIsIMEEnabled", "(I)I");
	methodID_doIMEEnable = getMethodID(gKinomaPlayClass, "doIMEEnable", "(I)V");
	methodID_doSetKeyboardStuff = getMethodID(gKinomaPlayClass, "doSetKeyboardStuff", "(ILjava/lang/String;)V");
	methodID_doSetKeyboardHints = getMethodID(gKinomaPlayClass, "doSetKeyboardHints", "(Ljava/lang/String;)V");
	methodID_doSetKeyboardSelection = getMethodID(gKinomaPlayClass, "doSetKeyboardSelection", "(II)V");
	methodID_doSetTERect = getMethodID(gKinomaPlayClass, "doSetTERect", "(IIII)V");

	methodID_doFetchApps = getMethodID(gKinomaPlayClass, "doFetchApps", "(I)V");
	methodID_doLaunchApp = getMethodID(gKinomaPlayClass, "doLaunchApp", "(Ljava/lang/String;Ljava/lang/String;)V");

	methodID_doFetchAppIcon = getMethodID(gKinomaPlayClass, "doFetchAppIcon", "(Ljava/lang/String;I)I");
	fieldID_mLaunchables = getFieldID(gKinomaPlayClass, "mLaunchables", "Ljava/lang/String;");
	fieldID_mIconBuffer = getFieldID(gKinomaPlayClass, "mIconBuffer", "[B");

	methodID_webviewCreate = getMethodID(gKinomaPlayClass, "webviewCreate", "(I)V");
	methodID_webviewDispose = getMethodID(gKinomaPlayClass, "webviewDispose", "(I)V");
	methodID_webviewActivated = getMethodID(gKinomaPlayClass, "webviewActivated", "(II)V");
	methodID_webviewSetFrame = getMethodID(gKinomaPlayClass, "webviewSetFrame", "(IIIII)V");
	methodID_webviewAttach = getMethodID(gKinomaPlayClass, "webviewAttach", "(I)V");
	methodID_webviewDetach = getMethodID(gKinomaPlayClass, "webviewDetach", "(I)V");
	methodID_webviewGetURL = getMethodID(gKinomaPlayClass, "webviewGetURL", "(I)Ljava/lang/String;");
	methodID_webviewSetURL = getMethodID(gKinomaPlayClass, "webviewSetURL", "(ILjava/lang/String;)V");
	methodID_webviewEvaluateScript = getMethodID(gKinomaPlayClass, "webviewEvaluateScript", "(ILjava/lang/String;)I");
	methodID_webviewReload = getMethodID(gKinomaPlayClass, "webviewReload", "(I)V");
	methodID_webviewBack = getMethodID(gKinomaPlayClass, "webviewBack", "(I)V");
	methodID_webviewForward = getMethodID(gKinomaPlayClass, "webviewForward", "(I)V");
	methodID_webviewCanBack = getMethodID(gKinomaPlayClass, "webviewCanBack", "(I)I");
	methodID_webviewCanForward = getMethodID(gKinomaPlayClass, "webviewCanForward", "(I)I");

	methodID_libraryThumbnail = getMethodID(gKinomaPlayClass, "libraryThumbnail", "(Ljava/lang/String;IZ)Landroid/graphics/Bitmap;");
	methodID_libraryFetch = getMethodID(gKinomaPlayClass, "libraryFetch", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
	methodID_libraryGetSongPath = getMethodID(gKinomaPlayClass, "getAlbumSongContentPath", "(II)Ljava/lang/String;");
	methodID_librarySaveImage = getMethodID(gKinomaPlayClass, "saveImage", "([B)V");

	methodID_setContinuousDrawing = getMethodID(gKinomaPlayClass, "setContinuousDrawing", "(Z)V");
}


jint JAVANAME(KinomaPlay_callFsk)( JNIEnv* env, jobject thiz, jint what, jstring jstr)
{
	jint err;
	FskEvent event;

//don't call this - it's too early	xprintf(stderr, "callFsk - what: (%d:%s)\n", what, fskFn(what));

	switch (what) {
		case kJNIStartFsk:
			PreInit();
		    err = FskMainInitialize(1, 0, NULL);
			FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "MainInit returns %d", err);
			PostInit();
			FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "after PostInit");
			myHWInfo.afterInit = 1;
			return err;
			break;
		case kJNIIdleFsk:
			FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "before androidFskThreadRunloopCycle");
			androidFskThreadRunloopCycle(0, &gMainWaitReturnTime);

			if (gQuitting)
				return -1;
			return gMainWaitReturnTime;
		case kJNIStopFsk:
			if (lastApps)
				FskMemPtrDispose(lastApps);
    		return FskMainTerminate();
			break;
		case kJNISetupForCallbacks:
			{
		    jclass localRef = (*env)->GetObjectClass(env, thiz);
		    if (localRef == NULL) {
		        FskInstrumentedTypePrintfNormal(&gAndroidJNITypeInstrumentation, "Can't find %s", JAVACLASS_KINOMAPLAY);
		        return -1;
		    }

		    gKinomaPlayClass = (*env)->NewGlobalRef(env, localRef);
		    gKinomaPlayObject = (*env)->NewGlobalRef(env, thiz);

		    gEnv = env;
			initJNI();
			return 0;
			}
			break;
		case kJNIFskInvalWindow:
			FskInstrumentedTypePrintfDebug(&gAndroidWindowTypeInstrumentation, "JNIFskInvalWindow - UNUSED\n");
			return 0;
		case kJNILowMemory:
			FskInstrumentedTypePrintfNormal(&gAndroidGlueTypeInstrumentation, "JNILowMemory");
			FskECMAScriptHibernate();
			FskNotificationPost(kFskNotificationLowMemory);
			break;
		case kJNINetworkChanged:
//@@MDK is this getting called?
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "JNI called us to tell us the network changed");
			LinuxInterfacesChanged();
			break;

		case kJNIFskReceivedSMS: {
			int len;
			char *msg;
			len = GetJavaStringAsFskString(jstr, (FskMemPtr*)&msg);
			if (len > 0) {

				FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "received sms - posting %s", msg);
				if (kFskErrNone == FskEventNew(&event, kFskEventWindowOpenFiles, NULL, kFskEventModifierNotSet)) {
					if (kFskErrNone == FskEventParameterAdd(event, kFskEventParameterFileList, len + 1, msg))
						FskWindowEventQueue(FskWindowGetActive(), event);
				}
				FskMemPtrDispose(msg);
			}
			break;
			}

		case kJNIWindowDeactivated:
			FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "JNI called us to tell us the window was deactivated");
			androidDoWindowDeactivated();
			break;

		case kJNIWindowActivated:
			FskInstrumentedTypePrintfVerbose(&gAndroidWindowTypeInstrumentation, "JNI called us to tell us the window was activated");
			androidDoWindowActivated();
			break;

		case kJNIVolumeMounted:
			{
			char *msg;
			GetJavaStringAsFskString(jstr, (FskMemPtr*)&msg);
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, " Calling Mounted for %s", msg);
			androidVolumeEvent(1, msg);		// kFskVolumeHello
			FskMemPtrDispose(msg);
			}
			break;

		case kJNIVolumeEjected:
			{
			char *msg;
			GetJavaStringAsFskString(jstr, (FskMemPtr*)&msg);
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, " Calling Ejected for %s", msg);
			androidVolumeEvent(2, msg);		// kFskVolumeBye
			FskMemPtrDispose(msg);
			}
			break;

		case kJNIPackagesChanged:
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "packages changed");
			if (lastApps)
				FskMemPtrDispose(lastApps);
			GetJavaStringAsFskString(jstr, (FskMemPtr*)&lastApps);
			appsCookie++;
			break;

		case kJNIExtendAudioBuffer:
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "extend audio buffer to 10 seconds");
			FskAudioOutSetOutputBufferSize(NULL, 0, 10);
			break;
		case kJNIReduceAudioBuffer:
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "reduce audio buffer to default (1/2 second)");
			FskAudioOutSetOutputBufferSize(NULL, 0, 0);
			break;
	}

	return 0;
}



// These callbacks need to be set prior to launching fsk

int PreInit() {
	int i;
	gInitStarted = 1;

	android_getCpuFeatures();

	//FskMemSet(&myCallbacks, 0, sizeof(myCallbacks));	/* Make sure that we start with all NULL pointers. TODO: Is this OK??? */

	myCallbacks.setWallpaperCB = NULL;
	myCallbacks.getMemoryInfoCB = NULL;
	myCallbacks.getModelInfoCB = myGetModelInfo;
	myCallbacks.getDPICB = androidGetDPI;
	myCallbacks.getLanguageCB = myGetLanguage;
	myCallbacks.getBasetimeCB = myGetBasetime;
	myCallbacks.launchDocumentCB = doLaunchDocument;
	myCallbacks.jniControlCB = androidJNIControl;
	myCallbacks.jniFetchCB = androidJNIFetch;

	myCallbacks.getContactsCB = doFetchContacts;
	myCallbacks.startContactsCB = doStartContacts;
	myCallbacks.stopContactsCB = doStopContacts;
	myCallbacks.nextContactCB = doNextContact;
	myCallbacks.idxContactCB = doIdxContact;

	myCallbacks.dialCB = doDial;
	myCallbacks.smsSendCB = androidSendSMS;
	myCallbacks.mailSendCB = androidSendMail;
	myCallbacks.attachmentsSendCB = androidSendAttachments;

    myCallbacks.isIMEEnabledCB = doIsIMEEnabled;
    myCallbacks.IMEEnableCB = doIMEEnable;

	myCallbacks.fetchAppsCB = doFetchApps;
	myCallbacks.launchAppCB = doLaunchApp;
	myCallbacks.fetchAppIconCB = doFetchAppIcon;

	myCallbacks.dirChangeNotifierNewCB = androidDirectoryChangeNotifierNew;
	myCallbacks.dirChangeNotifierDisposeCB = androidDirectoryChangeNotifierDispose;
	myCallbacks.dirChangeNotifierTermCB = androidiNotifyTerminate;

	myCallbacks.systemBarHeightCB = androidSystemBarHeight;
	myCallbacks.systemBarShowCB = androidSystemBarShow;

	myCallbacks.canHandleIntentCB = androidCanHandleIntent;
	myCallbacks.handleIntentCB = androidHandleIntent;
	myCallbacks.canHandleIntentClassCB = androidCanHandleIntentClass;
	myCallbacks.handleIntentClassCB = androidHandleIntentClass;

	myCallbacks.energySaverCB = androidEnergySaver;

	myCallbacks.wakeMainCB = androidWakeMain;
	myCallbacks.detachThreadCB = androidDetachThread;


	myCallbacks.getLastXYCB = getLastXY;
	myCallbacks.getScreenSizeCB = androidGetScreenSize;
	myCallbacks.checkSizeChangeCompleteCB = androidCheckSizeChangeComplete;
	myCallbacks.noWindowDontDrawCB = androidNoWindowDontDraw;

	myCallbacks.tweakKbdCB = androidTweakKeyboard;
	myCallbacks.tweakKbdSelectionCB = androidTweakKeyboardSelection;
	myCallbacks.setTERectCB = androidSetTERect;

	myCallbacks.afterWindowResizeCB = androidAfterWindowResize;
	myCallbacks.getMidWindowResizeCB = androidMidWindowResize;
	myCallbacks.setTransitionStateCB = androidSetTransitionState;

	myCallbacks.getStaticDataDirCB = androidGetStaticDataDir;
	myCallbacks.getStaticExternalDirCB = androidGetStaticExternalDir;
	myCallbacks.getStaticAppDirCB = androidGetStaticAppDir;
	myCallbacks.getSpecialPathsCB = myGetSpecialPaths;

	myCallbacks.getStaticIMEICB = androidGetStaticIMEI;
	myCallbacks.getDeviceUsernameCB = androidGetDeviceUsername;

	myCallbacks.setFBGlobalsCB = androidSetFBGlobals;

	myCallbacks.getVolumeCB = androidGetVolumeDouble;
	myCallbacks.setVolumeCB = androidSetVolumeDouble;

	myCallbacks.getAppsCookieCB = androidGetAppsCookie;
	myCallbacks.setKeyboardHintCB = androidKeyboardHints;

	myCallbacks.addWifiSocketCB = androidKeepWifiAlive;
	myCallbacks.removeWifiSocketCB = androidLetWifiDie;

	myCallbacks.resetPhoneHomeCB = androidResetHomeScreen;

	myCallbacks.logPrintCB  = fsk_android_log_print;
	myCallbacks.logVPrintCB = fsk_android_log_vprint;

	myCallbacks.webviewCreateCB = androidWebViewCreate;
	myCallbacks.webviewDisposeCB = androidWebViewDispose;
	myCallbacks.webviewActivatedCB = androidWebViewActivated;
	myCallbacks.webviewSetFrameCB = androidWebViewSetFrame;
	myCallbacks.webviewAttachCB = androidWebViewAttach;
	myCallbacks.webviewDetachCB = androidWebViewDetach;
	myCallbacks.webviewGetURLCB = androidWebViewGetURL;
	myCallbacks.webviewSetURLCB = androidWebViewSetURL;
	myCallbacks.webviewEvaluateScriptCB = androidWebViewEvaluateScript;
	myCallbacks.webviewReloadCB = androidWebViewReload;
	myCallbacks.webviewBackCB = androidWebViewBack;
	myCallbacks.webviewForwardCB = androidWebViewForward;
	myCallbacks.webviewCanBackCB = androidWebViewCanBack;
	myCallbacks.webviewCanForwardCB = androidWebViewCanForward;

	myCallbacks.libraryFetchCB = androidLibraryFetch;
	myCallbacks.libraryThumbnailCB = androidLibraryThumbnail;
	myCallbacks.libraryGetSongPathCB = androidLibraryGetSongPath;
	myCallbacks.librarySaveImageCB = androidLibrarySaveImage;

	myCallbacks.getRandomCB = androidGetRandomNumber;

	myCallbacks.setContinuousDrawingCB = doSetContinuousDrawing;
	myCallbacks.getContinuousDrawingUpdateIntervalCB = doGetContinuousDrawingUpdateInterval;

	FskSetAndroidCallbacks(&myCallbacks);


	FskMemSet(&myHWInfo, 0, sizeof(myHWInfo));

	myHWInfo.batteryLevel = 50;
	myHWInfo.signalStrength = 0;
	myHWInfo.dataSignalStrength = 0;
	myHWInfo.imei = staticIMEI;
	myHWInfo.uuid = staticUUID;
	myHWInfo.backlightOn = true;

	for (i = 0; i < gAndroidIfaceCount; i++) {
		gAndroidIface[i].name = NULL;
		FskMemSet(&gAndroidIface[i].MAC, 0, 6);
		gAndroidIface[i].status = 0;
	}

	myHWInfo.keyboardType = kFskKeyboardTypeVirtual;

	if (pendingOrientation != -1)
		myHWInfo.orientation = pendingOrientation;
	else
		myHWInfo.orientation = 0;
	myHWInfo.needsRotationTransition = gNeedsOrientationRotate;

	return 0;
}

int PostInit() {
	if (NULL ==  FskThreadGetCurrent()->name) {
		FskDebugStr("init failed");
		exit(0);
	}

	(void)FskConditionNew(&jniRespCond);
	(void)FskMutexNew(&jniRespMutex, "javaResponse Cond");
	(void)FskMutexNew(&gSurfaceChangedMutex, "Surface Changed");
	return 0;
}

void JAVANAME(KinomaPlay_fskPhoneStateChanged)( JNIEnv *env, jobject thiz, jint what, jint state )
{
	int hwInfoSet = 1;

	if (myHWInfo.afterInit) {
		FskInstrumentedTypePrintfVerbose(&gAndroidPhoneStateTypeInstrumentation, "phone state (%d:%s) changed: %d", what, phoneState(what), state);
	}
	switch (what) {
		case kFskCallState:
			hwInfoSet = 0;
			switch (state) {
				case 0:				// CALL_STATE_IDLE
					FskSetCallStatus(1, kFskLineCallStateIdle);
					break;
				case 1:				// CALL_STATE_RINGING
					FskSetCallStatus(1, kFskLineCallStateOffering);
					break;
				case 2:				// CALL_STATE_OFFHOOK
					FskSetCallStatus(1, kFskLineCallStateConnected);
					break;
			}
			break;
		case kFskCellDataConnectionType:
			switch(state) {
			case 0:			// NETWORK_TYPE_UNKNOWN
				myHWInfo.cellularNetworkType = kFskPhoneNetworkCellularUnknown;
				break;
			case 1:			// NETWORK_TYPE_GPRS
				myHWInfo.cellularNetworkType = kFskPhoneNetworkCellularGPRS;
				break;
			case 2:			// NETWORK_TYPE_EDGE
				myHWInfo.cellularNetworkType = kFskPhoneNetworkCellularEDGE;
				break;
			case 3:			// NETWORK_TYPE_UMTS
				myHWInfo.cellularNetworkType = kFskPhoneNetworkCellularUMTS;
				break;
			case 4:			// NETWORK_TYPE_CDMA
				myHWInfo.cellularNetworkType = kFskPhoneNetworkCellularWCDMA;
				break;
			case 5:			// NETWORK_TYPE_EVDO_0
				myHWInfo.cellularNetworkType = kFskPhoneNetworkCellular1XEVD0;
				break;
			case 6:			// NETWORK_TYPE_EVDO_A
				myHWInfo.cellularNetworkType = kFskPhoneNetworkCellular1XEVDV;
				break;
			case 7:			// NETWORK_TYPE_1xRTT
				myHWInfo.cellularNetworkType = kFskPhoneNetworkCellular1xRTT;
				break;
			case 8:			// NETWORK_TYPE_HSDPA
			case 9:			// NETWORK_TYPE_HSPA
			case 10:		// NETWORK_TYPE_HSUPA
				myHWInfo.cellularNetworkType = kFskPhoneNetworkCellularHSDPA;
				break;
			}
			break;
		case kFskNetworkType:
			myHWInfo.activeNetworkType = state;
			if (state == 1)
				myHWInfo.dataNetworkType = kFskPhoneNetworkDataWifi;
			else {
				FskInstrumentedTypePrintfDebug(&gAndroidPhoneStateTypeInstrumentation, "network type - state is %d - calling it cellular", state);
				myHWInfo.dataNetworkType = kFskPhoneNetworkDataCellular;
			}
			break;
		case kFskNetworkEnabled:
			if (state == 0) {
				myHWInfo.cellularNetworkState = kFskDataNetworkStateOff;
				myHWInfo.dataNetworkState = kFskDataNetworkStateOff;
			}
			break;
		case kFskCellDataConnectionState:
			myHWInfo.cellularNetworkState = state;
				// DATA_DISCONNECTED => kFskCellularNetworkStateDisconnected
				//  DATA_CONNECTING => kFskCellularNetworkStateConnecting
				// DATA_CONNECTED => kFskCellularNetworkStateConnected
				// DATA_SUSPENDED => kFskCellularNetworkStateSuspended
			break;
		case kFskDataConnectionState:
			myHWInfo.dataNetworkState = state;
				// DATA_DISCONNECTED => kFskCellularNetworkStateDisconnected
				//  DATA_CONNECTING => kFskCellularNetworkStateConnecting
				// DATA_CONNECTED => kFskCellularNetworkStateConnected
				// DATA_SUSPENDED => kFskCellularNetworkStateSuspended
			if (state == kFskDataNetworkStateDisconnected)
				myHWInfo.wifiAddr = 0;
			break;
		case kFskMessageWaitingState:
			myHWInfo.unreadMessages = 1;		// MDK need real #
			break;
		case kFskServiceState:
			switch (state) {
				case 0: 		// STATE_IN_SERVICE
					myHWInfo.cellularNetworkState = kFskCellularNetworkStateConnected;
					break;
				case 1:			// STATE_OUT_OF_SERVICE
					myHWInfo.cellularNetworkState = kFskCellularNetworkStateSuspended;
					break;
				case 2:			// STATE_EMERGENCY_ONLY:
					myHWInfo.cellularNetworkState = kFskCellularNetworkStateEmergencyOnly;
					break;
				case 3:			// STATE_POWER_OFF
					myHWInfo.cellularNetworkState = kFskCellularNetworkStateOff;;
					break;
			}
			break;
		case kFskSignalStrength:
			myHWInfo.signalStrength = state;
			FskInstrumentedTypePrintfDebug(&gAndroidPhoneStateTypeInstrumentation, "signal strength %d - cellular", state);
			break;
		case kFskDataSignalStrength:
			myHWInfo.dataSignalStrength = state;
			FskInstrumentedTypePrintfDebug(&gAndroidPhoneStateTypeInstrumentation, "signal strength %d - data", state);
			break;
		case kFskBatteryLevel: myHWInfo.batteryLevel = state; break;
		case kFskBatteryPlugged: myHWInfo.chargerPlugged = state; break;
		case kFskBacklightOn: myHWInfo.backlightOn = state; break;

		case kFskNetworkWifiAddress: myHWInfo.wifiAddr = FskEndianU32_BtoN(state); break;
	}

	if (hwInfoSet)
		FskSetPhoneHWInfo(&myHWInfo);
}



int doStartContacts(int sortOrder) {
	int num;

	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doStartContacts - trying to acquire jniRespMutex");
	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskContactsCB, (void*)1, (void*)&num, (void*)sortOrder, NULL);
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
	return num;
}

void doStopContacts() {
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doStopContacts - trying to acquire jniRespMutex");
	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskContactsCB, (void*)2, NULL, NULL, NULL);
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
}

char *doNextContact(char **image) {
	char *resp = NULL;
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doNextContact - trying to acquire jniRespMutex");
	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskContactsCB, (void*)3, (void*)&resp, (void*)image, NULL);
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
	FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "doNextContact got %s icon: %x", resp, *image);
	return resp;
}

char *doIdxContact(int idx, char **image) {
	char *resp = NULL;
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doIdxContact - trying to acquire jniRespMutex");
	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskContactsCB, (void*)4, (void*)idx, (void*)&resp, image);
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
	FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "doIdxContact(%d) got %s - icon: %x", idx, resp, *image);
	return resp;
}

char *doFetchContacts(int style) {
	char *resp = NULL;
#ifdef ANDROID_PERMISSION_READ_CONTACTS
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doFetchContacts - trying to acquire jniRespMutex");
	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskContactsCB, (void*)5, (void*)style, (void*)&resp, NULL);
	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "doFetchContacts - posted callback, waiting for response");
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "doFetchContact got %s", resp);
#endif // ANDROID_PERMISSION_READ_CONTACTS
	return resp;
}

void doDial(char *number) {
#ifdef ANDROID_PERMISSION_CALL_PHONE
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doDial - trying to acquire jniRespMutex");
	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskContactsCB, (void*)6, (void*)number, NULL, NULL);
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "after doDial");
#endif // ANDROID_PERMISSION_CALL_PHONE
}

void androidSendSMS(char *number, char *message) {
#ifdef ANDROID_PERMISSION_SEND_SMS
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidSendSMS - trying to acquire jniRespMutex");
	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskContactsCB, (void*)7, (void*)number, (void*)message, NULL);
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "after doSendSMS");
#endif // ANDROID_PERMISSION_SEND_SMS
}

void androidSendMail(char *whoto, char *subject, char *message) {
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidSendMail - trying to acquire jniRespMutex");
	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskContactsCB, (void*)10, (void*)whoto, (void*)subject, (void*)message);
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "after androidSendMail");
}

typedef struct sndAttachRec {
	char *to;
	char *sub;
	char *body;
	char *fileList;
	int	numFiles;
} sndAttachRec, *sndAttach;

void androidSendAttachments(char *whoto, char *subject, char *message, char *attachFileList, int attachNum) {
	sndAttachRec snd;
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidSendAttachments - trying to acquire jniRespMutex");

	FskMutexAcquire(jniRespMutex);
	snd.to = whoto;
	snd.sub = subject;
	snd.body = message;
	snd.fileList = attachFileList;
	snd.numFiles = attachNum;
	FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "androidSendAttachments - postThreadCallback");
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskContactsCB, (void*)11, (void*)&snd, NULL, NULL);
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "after androidSendAttachments");
}

void doSendSMS(char *number, char *message) {
	FskDebugStr("MDK - don't call this anymore");
	androidSendSMS(number, message);
}

FskErr androidFskLaunchCB(void *a, void *b, void *c, void *d) {
	char *pkgName = (char*)a;
	char *actName = (char*)b;
	jstring pkgNameString = (*gEnv)->NewStringUTF(gEnv, pkgName);
	jstring actNameString = (*gEnv)->NewStringUTF(gEnv, actName);

	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doLaunchApp, pkgNameString, actNameString);

	if (pkgNameString != NULL)
		(*gEnv)->DeleteLocalRef(gEnv, pkgNameString);
	if (actNameString != NULL)
		(*gEnv)->DeleteLocalRef(gEnv, actNameString);
	return 0;
}

FskErr androidSetContinuousDrawingCB(void *a, void *b, void *c, void *d) {
	Boolean continuousDrawing = (Boolean) a;

	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_setContinuousDrawing, continuousDrawing);

	return 0;
}


void doSetContinuousDrawing(Boolean continuousDrawing) {

	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "SetContinuousDrawing - %d", continuousDrawing);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidSetContinuousDrawingCB, (void*)continuousDrawing, NULL, NULL, NULL);
}

void doGetContinuousDrawingUpdateInterval(UInt32 *interval) {
	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "GetContinuousDrawingUpdateInterval - %u", gWindowUpdateInterval);
	*interval = gWindowUpdateInterval;
}

void doLaunchApp(char *pkgName, char *actName) {
	char *resp = NULL;

	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "doLaunchApp - %s %s", pkgName, actName);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskLaunchCB, (void*)pkgName, (void*)actName, (void*)NULL, NULL);
}

int androidGetAppsCookie() {
	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "appsCookie is %d", appsCookie);
	return appsCookie;
}

char *doFetchApps(int style) {
	char *resp = NULL;

	if ((NULL == lastApps) || (appsCookie != curCookie)) {
		curCookie = appsCookie;

		FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doFetchApps - trying to acquire jniRespMutex");
		FskMutexAcquire(jniRespMutex);
		FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskContactsCB, (void*)8, (void*)style, (void*)&resp, NULL);
		FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doFetchApps - posted callback, waiting for response");
		if (!gQuitting) {
			FskConditionWait(jniRespCond, jniRespMutex);
			FskMutexRelease(jniRespMutex);
		}
		if (resp)
			lastApps = FskStrDoCopy(resp);
	}
	else {
		resp = FskStrDoCopy(lastApps);
		FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "doFetchApps returns old list");
	}
	return resp;
}

char *doFetchAppIcon(char *packageName, int iconNo) {
	char *resp = NULL;
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doFetchAppIcon - trying to acquire jniRespMutex");
	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidFskContactsCB, (void*)9, (void*)packageName, (void*)iconNo, (void*)&resp);
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doFetchAppIcon - posted callback, waiting for response");
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
	FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "doFetchAppIcon got %x", resp);
	return resp;
}

FskErr androidFskContactsCB(void *a, void *b, void *c, void *d) {
	int *num, what, sort;
	jbyteArray jb;
	jbyte *theData;
	int len, ret, dataSize;
	jstring jstr;
	char *str, *p;

	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidFskContactsCB - trying to acquire jniRespMutex");
	FskMutexAcquire(jniRespMutex);
	FskConditionSignal(jniRespCond);

	if (NULL == gEnv) {
		goto bail;
	}

	what = (int)a;
	switch (what) {
		case 1:
#ifdef ANDROID_PERMISSION_READ_CONTACTS
			num = (int*)b;
			sort = (int)c;
			*num = (*gEnv)->CallIntMethod(gEnv, gKinomaPlayObject, methodID_doStartContacts, sort);
#endif // ANDROID_PERMISSION_READ_CONTACTS
			break;
		case 2:
#ifdef ANDROID_PERMISSION_READ_CONTACTS
			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doStopContacts);
#endif // ANDROID_PERMISSION_READ_CONTACTS
			break;
		case 3: {
#ifdef ANDROID_PERMISSION_READ_CONTACTS
			char **out = (char**)b;
			char **img = (char**)c;

			FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "doNextContact");
			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doNextContact);
			jstr = (*gEnv)->GetObjectField(gEnv, gKinomaPlayObject, fieldID_mContactSet);
			GetJavaStringAsFskString(jstr, (FskMemPtr*)out);

			dataSize = 0;
			jb = (*gEnv)->GetObjectField(gEnv, gKinomaPlayObject, fieldID_mIconBuffer);
			if (jb)
				dataSize = (*gEnv)->GetArrayLength(gEnv, jb);

			if (dataSize) {

				FskMemPtrNew(dataSize + 4, &p);
				*img = p;
				*(int*)p = dataSize;
				p += 4;
				(*gEnv)->GetByteArrayRegion(gEnv, jb, 0, dataSize, p);
			}
			else
				*img = NULL;
#endif // ANDROID_PERMISSION_READ_CONTACTS
		}
		break;
		case 4: {
#ifdef ANDROID_PERMISSION_READ_CONTACTS
			char **out = (char**)c;
			char **img = (char**)d;

			FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "doIdxContact (%d)", (int)b);
			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doIdxContact, (int)b);
			jstr = (*gEnv)->GetObjectField(gEnv, gKinomaPlayObject, fieldID_mContactSet);
			GetJavaStringAsFskString(jstr, (FskMemPtr*)out);

			dataSize = 0;
			jb = (*gEnv)->GetObjectField(gEnv, gKinomaPlayObject, fieldID_mIconBuffer);
			if (jb)
				dataSize = (*gEnv)->GetArrayLength(gEnv, jb);
			if (dataSize) {

				FskMemPtrNew(dataSize + 4, &p);
				*img = p;
				*(int*)p = dataSize;
				p += 4;
				(*gEnv)->GetByteArrayRegion(gEnv, jb, 0, dataSize, p);
			}
			else
				*img = NULL;
#endif // ANDROID_PERMISSION_READ_CONTACTS
		}
		break;
		case 5: {
#ifdef ANDROID_PERMISSION_READ_CONTACTS
			char **out = (char**)c;
			int style = (int)b;

			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "doFetchContacts %d", style);
			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doFetchContacts, style);
			jstr = (*gEnv)->GetObjectField(gEnv, gKinomaPlayObject, fieldID_mContactSet);
			GetJavaStringAsFskString(jstr, (FskMemPtr*)out);
#endif // ANDROID_PERMISSION_READ_CONTACTS
		}
		break;
		case 6: {
#ifdef ANDROID_PERMISSION_CALL_PHONE
			jstr = (*gEnv)->NewStringUTF(gEnv, (char*)b);
			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doDial, jstr);
			(*gEnv)->DeleteLocalRef(gEnv, jstr);
#endif // ANDROID_PERMISSION_CALL_PHONE
		}
		break;
		case 7: {
#ifdef ANDROID_PERMISSION_SEND_SMS
			jstring jstrNum, jstrMsg;

			jstrNum = (*gEnv)->NewStringUTF(gEnv, (char*)b);
			jstrMsg = (*gEnv)->NewStringUTF(gEnv, (char*)c);
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "before doSendSMS %s %s", (char*)b, (char*)c);
			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doSendSMS, jstrNum, jstrMsg);
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "after doSendSMS %s %s", (char*)b, (char*)c);
			(*gEnv)->DeleteLocalRef(gEnv, jstrNum);
			(*gEnv)->DeleteLocalRef(gEnv, jstrMsg);
#endif // ANDROID_PERMISSION_SEND_SMS
		}
		break;
		case 8: {
			char **out = (char**)c;
			int style = (int)b;

 			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "doFetchApps %d", style);
			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doFetchApps, style);
			jstr = (*gEnv)->GetObjectField(gEnv, gKinomaPlayObject, fieldID_mLaunchables);
			GetJavaStringAsFskString(jstr, (FskMemPtr*)out);
		}
		break;
		case 9: {
			jstring jstrPackage;
			int iconNo = (int)c;
			char **out = (char**)d;

			jstrPackage = (*gEnv)->NewStringUTF(gEnv, (char*)b);
			ret = (*gEnv)->CallIntMethod(gEnv, gKinomaPlayObject, methodID_doFetchAppIcon, jstrPackage, iconNo);

			if (0 == ret) {
				jb = (*gEnv)->GetObjectField(gEnv, gKinomaPlayObject, fieldID_mIconBuffer);
				dataSize = (*gEnv)->GetArrayLength(gEnv, jb);

				FskMemPtrNew(dataSize + 4, &p);
				*out = p;
				*(int*)p = dataSize;
				p += 4;

				(*gEnv)->GetByteArrayRegion(gEnv, jb, 0, dataSize, p);

			}
			else
				*out = NULL;
			(*gEnv)->DeleteLocalRef(gEnv, jstrPackage);
		}
		break;
		case 10: {
			jstring jstrWho, jstrSub, jstrMsg;

			jstrWho = (*gEnv)->NewStringUTF(gEnv, (char*)b);
			jstrSub = (*gEnv)->NewStringUTF(gEnv, (char*)c);
			jstrMsg = (*gEnv)->NewStringUTF(gEnv, (char*)d);
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "before doSendMail %s %s %s", (char*)b, (char*)c, (char*)d);
			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doSendMail, jstrWho, jstrSub, jstrMsg);
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "after doSendMail");
			(*gEnv)->DeleteLocalRef(gEnv, jstrWho);
			(*gEnv)->DeleteLocalRef(gEnv, jstrSub);
			(*gEnv)->DeleteLocalRef(gEnv, jstrMsg);
		}
		break;
		case 11: {
			jstring jstrWho, jstrSub, jstrMsg;
			sndAttach snd = (sndAttach)b;
			jobjectArray ret;
			int i;
			char *path;

			ret = (jobjectArray)(*gEnv)->NewObjectArray(gEnv, snd->numFiles,
				(*gEnv)->FindClass(gEnv, "java/lang/String"),
				(*gEnv)->NewStringUTF(gEnv, ""));
			path = snd->fileList;
			for (i=0; i<snd->numFiles; i++) {
				(*gEnv)->SetObjectArrayElement(gEnv, ret, i, (*gEnv)->NewStringUTF(gEnv, path));
				path += FskStrLen(path) + 1;
			}

			jstrWho = (*gEnv)->NewStringUTF(gEnv, snd->to);
			jstrSub = (*gEnv)->NewStringUTF(gEnv, snd->sub);
			jstrMsg = (*gEnv)->NewStringUTF(gEnv, snd->body);

			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "before doSendAttachments %s %s %s", snd->to, snd->sub, snd->body);

			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doSendAttach, jstrWho, jstrSub, jstrMsg, ret, snd->numFiles);

			FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "after doSendAttachments");
			(*gEnv)->DeleteLocalRef(gEnv, jstrWho);
			(*gEnv)->DeleteLocalRef(gEnv, jstrSub);
			(*gEnv)->DeleteLocalRef(gEnv, jstrMsg);

		}
		break;

	}

bail:
	FskMutexRelease(jniRespMutex);
	return kFskErrNone;
}


//---------------------------------------------

int gPendingSetKbdSelection = 0;
int gPendingSetKbdSelectionStart = 0;
int gPendingSetKbdSelectionEnd = 0;

FskErr androidTweakKeyboardSelection(UInt32 selBegin, UInt32 selEnd) {
	FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "androidTweakKeyboardSelection  %d %d (old: %d %d)",  selBegin, selEnd, gTESelectionStart, gTESelectionEnd);

	if ((gTESelectionStart == selBegin)
		&& (gTESelectionEnd == selEnd)) {
			FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "androidTweakKeyboardSelection - same selection - do nothing");
	}
	else {
		if (fbGlobals->midSizeChange) {
			FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "- is midSizeChange %d - delay set selection: %d %d", fbGlobals->midSizeChange, selBegin, selEnd);

			gPendingSetKbdSelection = 1;
			gPendingSetKbdSelectionStart = selBegin;
			gPendingSetKbdSelectionEnd = selEnd;
			return kFskErrNone;
		}

		FskThreadPostCallback(FskThreadGetMain(), (void*)androidIMECallback, (void*)4, (void*)selBegin, (void*)selEnd, NULL);
		FskThreadYield();
		usleep(10000);
		FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidTweakKeyboardSelection - not acquiring mutex - just posting callback and yielding");
	}
	return kFskErrNone;
}

// androidTweakKeyboard is responsible for deleting the passed in memory "text"
// it gets done in the callback on the main thread
FskErr androidTweakKeyboard(char *text, UInt32 textCount, UInt32 keyboardMode) {
	FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "androidTweakKeyboard mode: %d", keyboardMode);

	gTESelectionStart = -1;
	gTESelectionEnd = -1;

	FskThreadPostCallback(FskThreadGetMain(), (void*)androidIMECallback, (void*)3, (void*)FskStrDoCopy(text), (void*)textCount, (void*)keyboardMode);
	FskThreadYield();
	usleep(10000);
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidTweakKeyboard - posted callback, waiting for response");
	return kFskErrNone;
}

void androidSetTERect(FskRectangle r) {
//	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidSetTERect - trying to acquire jniRespMutex");
//	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidIMECallback, (void*)6, (void*)r, NULL, NULL);
	FskThreadYield();
	usleep(10000);
//	if (!gQuitting) {
//		FskConditionWait(jniRespCond, jniRespMutex);
//		FskMutexRelease(jniRespMutex);
//	}
}

void androidKeyboardHints(char *hint) {
	FskInstrumentedTypePrintfDebug(&gAndroidTETypeInstrumentation, "androidKeyboardHints - Hint: %s", hint);
//	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidKeyboardHints - trying to acquire jniRespMutex");
//	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidIMECallback, (void*)5, (void*)FskStrDoCopy(hint), NULL, NULL);
	FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "androidKeyboardHints - posted callback, waiting for response");
	FskThreadYield();
	usleep(10000);
//	if (!gQuitting) {
//		FskConditionWait(jniRespCond, jniRespMutex);
//		FskMutexRelease(jniRespMutex);
//	}
}

FskErr androidIMECallback(void *a, void *b, void *c, void *d) {
	int what = (int)a;
	int *resp;

	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidIMECallback");

	switch (what) {
		case 1:
			resp = (int*)b;
			FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "call methodID_doIsIMEEnabled");
			*resp = (*gEnv)->CallIntMethod(gEnv, gKinomaPlayObject, methodID_doIsIMEEnabled, 1);

			break;
		case 2:
			FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "doIMEEnable %d", (int)b);
			gTESelectionStart = -1;
			gTESelectionEnd = -1;
			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doIMEEnable, (int)b);
			break;

		case 3: {		// TweakKeyboard
			char *text= (char*)b;
			UInt32 textCount = (UInt32)c;
			UInt32 keyboardType = (UInt32)d;
			jstring jstrText;

			gTEIgnoreChanged = 1;
			if (text) {
				jstrText = (*gEnv)->NewStringUTF(gEnv, text);
				FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "sending text %s %x (count %d) type: %d", text, text, textCount, keyboardType);

			}
			else {
				char *mtString = NULL;
				FskMemPtrNew(64, &mtString);
				*mtString = '\0';
				jstrText = (*gEnv)->NewStringUTF(gEnv, mtString);
				FskMemPtrDispose(mtString);
				FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "sending text '\0' (count %d) type: %d", 1, keyboardType);
				textCount = 1;
			}

			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doSetKeyboardStuff, keyboardType, jstrText);
			gTEIgnoreChanged = 0;

			if (text) {
				FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "disposing text %s %x (count %d)", text, text, textCount);
				FskMemPtrDispose(text);
			}

			(*gEnv)->DeleteLocalRef(gEnv, jstrText);

			} break;

		case 4: {		// TweakKeyboardSelection
			gTESelectionStart = (UInt32)b;
			gTESelectionEnd = (UInt32)c;

			FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "do set selection begin %d, end %d", gTESelectionStart, gTESelectionEnd);
			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doSetKeyboardSelection, gTESelectionStart, gTESelectionEnd);
			} break;

		case 5: {		// androidKeyboardHints
			jstring jstrText = NULL;

			FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "androidKeyboardHints - hint: %s", ((char*)b));

			if (b != NULL) {
				jstrText = (*gEnv)->NewStringUTF(gEnv, (char*)b);
				FskMemPtrDispose((char*)b);
			}
			(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doSetKeyboardHints, jstrText);
			if (jstrText)
				(*gEnv)->DeleteLocalRef(gEnv, jstrText);
			} break;
		case 6: {		// androidSetTERect
			FskRectangle r = (FskRectangle)b;

			FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "androidSetTERect - %d %d %d %d", r->x,r->y,r->width,r->height);

			if (b != NULL)
				(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_doSetTERect, r->x, r->y, r->width, r->height);
			} break;

	}

//	FskConditionSignal(jniRespCond);
//	FskMutexRelease(jniRespMutex);

	return kFskErrNone;
}

void JAVANAME(KinomaPlay_setIMEEnabled)(JNIEnv *env, jclass clazz, jint enabled) {
	if (enabled)
		gLastIME = true;
	else
		gLastIME = false;
}

Boolean doIsIMEEnabled() {
	return gLastIME;
}

Boolean realdoIsIMEEnabled() {
	int resp;

	if (gPendingSizeChange || fbGlobals->midSizeChange) {
		return gLastIME;
	}
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doIsIMEEnabled - trying to acquire jniRespMutex");
	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidIMECallback, (void*)1, (void*)&resp, NULL, NULL);
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "doIsIMEEnabled - posted callback, waiting for response");
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
	FskInstrumentedTypePrintfVerbose(&gAndroidTETypeInstrumentation, "doIsIMEEnabled got %s", resp ? "true" : "false");
	gLastIME = resp ? true : false;
	return gLastIME;
}

void doIMEEnable(Boolean enable) {
	if (gAndroidTransitionState) {
		gPendingIMEEnable = enable ? 1 : 0;
		return;
	}

	FskThreadPostCallback(FskThreadGetMain(), (void*)androidIMECallback, (void*)2, (void*)(enable ? 1 : 0), NULL, NULL);
}

int androidSystemBarHeight() {
	if (gSystemBarShowing) {
		FskWindow win = FskWindowGetActive();
		FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "androidSystemBarHeight %d - showing - scaled to %d", gSystemBarHeight, FskPortUInt32Unscale(win->port, gSystemBarHeight));
		return FskPortUInt32Unscale(win->port, gSystemBarHeight);
	}
	FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "androidSystemBarHeight %d - hidden", gSystemBarHeight);
	return 0;
}

int androidSystemBarShow(int what) {
	if (gPendingSizeChange || fbGlobals->midSizeChange) {
		FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "androidSystemBarShow(%d) (current:%d) -- Delayed because PendingSizeChange (%d) or midSizeChange (%d)", what, gSystemBarShowing, gPendingSizeChange, fbGlobals->midSizeChange);
		gPendingSystemBar = 1;
		gPendingSystemBarShow = what;
	}
	else {
		FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "androidSystemBarShow(%d) (current:%d)", what, gSystemBarShowing);
		if (what == gSystemBarShowing)
			return 0;
		androidJNIControl(JNICONTROL_SHOW_SYSTEM_BAR, what);
		gSystemBarShowing = what;
	}
	return 0;
}



/*---------------------------------*/
/* fskJNIControl_main calls into Java to do some action.
 * It is one-shot and async, without return value.
 * It has to be called by the main thread.
 *
 * androidJNIControl(int what, int value) can be used
 * from any thread to make it happen.
 */
void fskJNIControl_main(void *a, void *b, void *c, void *d) {
	int what = (int)a;
	int value = (int)b;

	FskInstrumentedTypePrintfVerbose(&gAndroidJNITypeInstrumentation, "calling fskJNIControl (%d:%s, %d) gEnv=%p gKinomaPlayObject=%p methodID=%+d",
		what, selectorStr(JNI_CLASS_CONTROL, what), value, gEnv, gKinomaPlayObject, methodID_fskJNIControl);
	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_fskJNIControl, what, value);
}

void androidJNIControl(int what, int value) {
	FskThread	main, current;

	main = FskThreadGetMain();
	current = FskThreadGetCurrent();

	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "androidJNIControl [%d %d]", what, value);
	if (main == current) {
		FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, " - call right through");
		fskJNIControl_main((void *)what, (void *)value, NULL, NULL);
	}
	else {
		FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, " - post it to main");
		FskThreadPostCallback(main, fskJNIControl_main, (void*)what, (void*)value, NULL, NULL);
	}
}

static int wificount = 0;
void androidKeepWifiAlive() {
	++wificount;

	if (wificount == 1) {
		FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, " -- androidKeepWifiAlive %d", wificount);
		androidJNIControl(JNICONTROL_KEEP_WIFI_ALIVE, 1);
	}
}

void androidLetWifiDie() {
	 --wificount;

	if (wificount == 0) {
		FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, " -- androidLetWifiDie now %d", wificount);
		androidJNIControl(JNICONTROL_KEEP_WIFI_ALIVE, 0);
	}
}

void androidResetHomeScreen() {
	androidJNIControl(JNICONTROL_RESET_ANDROID_HOME, 0);
}

void androidEnergySaver(UInt32 mask, UInt32 value) {
	static int sDisableDim = 0;
	static int sDisableSleep = 0;
	static int sDisableScreenSleep = 0;

	FskInstrumentedTypePrintfDebug(&gAndroidEnergyTypeInstrumentation, "EnergySaver - mask: %x, value; %x", mask, value);
	if (mask & kFskUtilsEnergySaverDisableScreenDimming) {
		FskInstrumentedTypePrintfDebug(&gAndroidEnergyTypeInstrumentation, " -- dimming mask set - value is: %d, disableDim was: %d -", value & kFskUtilsEnergySaverDisableScreenDimming, sDisableDim);
		if (value & kFskUtilsEnergySaverDisableScreenDimming)
			sDisableDim++;
		else if (sDisableDim)
			sDisableDim--;
		FskInstrumentedTypePrintfDebug(&gAndroidEnergyTypeInstrumentation, " now: %d", sDisableDim);
	}
	if (mask & kFskUtilsEnergySaverDisableScreenSleep) {
		FskInstrumentedTypePrintfDebug(&gAndroidEnergyTypeInstrumentation, " -- disableScreenSleep mask set - value is: %d, disableScreenSleep was: %d -", value & kFskUtilsEnergySaverDisableScreenSleep, sDisableScreenSleep);
		if (value & kFskUtilsEnergySaverDisableScreenSleep)
			sDisableScreenSleep++;
		else if (sDisableScreenSleep)
			sDisableScreenSleep--;
		FskInstrumentedTypePrintfDebug(&gAndroidEnergyTypeInstrumentation, " now: %d", sDisableScreenSleep);
	}
	if (mask & kFskUtilsEnergySaverDisableSleep) {
		FskInstrumentedTypePrintfDebug(&gAndroidEnergyTypeInstrumentation, " -- disableSleep mask set - value is: %d, disableSleep was: %d -", value & kFskUtilsEnergySaverDisableSleep, sDisableSleep);
		if (value & kFskUtilsEnergySaverDisableSleep)
			sDisableSleep++;
		else if (sDisableSleep)
			sDisableSleep--;
		FskInstrumentedTypePrintfDebug(&gAndroidEnergyTypeInstrumentation, " now: %d", sDisableSleep);
	}

	if (sDisableDim) {
		FskInstrumentedTypePrintfVerbose(&gAndroidEnergyTypeInstrumentation, " -- sDisableDim is set, call Wakelock set 1");
		androidJNIControl(JNICONTROL_SLEEP, 1);
	}
	else if (sDisableScreenSleep) {
		FskInstrumentedTypePrintfVerbose(&gAndroidEnergyTypeInstrumentation, " -- sDisableScreenSleep is set, call Wakelock set 3");
		androidJNIControl(JNICONTROL_SLEEP, 3);
	}
	else if (sDisableSleep) {
		FskInstrumentedTypePrintfVerbose(&gAndroidEnergyTypeInstrumentation, " -- sDisableSleep is set, call Wakelock set 2");
		androidJNIControl(JNICONTROL_SLEEP, 2);
	}
	else {
		FskInstrumentedTypePrintfVerbose(&gAndroidEnergyTypeInstrumentation, " -- neither sDisableSleep or sDisableDim is set, call Wakelock set 0");
		androidJNIControl(JNICONTROL_SLEEP, 0);
	}

}

void androidHideSplash() {
	androidJNIControl(JNICONTROL_SPLASH_OFF, 0);
}

void initClassHelper(JNIEnv *env, const char *path, jobject *objptr) {
	jclass cls = (*env)->FindClass(env, path);

//	FskDebugStr("initClassHelper - vm is %s", path);

	if (!cls) {
//		FskDebugStr("initClassHelper failed to get %s class ref", path);
		return;
	}
	jmethodID constr = (*env)->GetMethodID(env, cls, "<init>", "()V");
	if (!constr) {
//		FskDebugStr("initClassHelper failed to get %s constructor", path);
		return;
	}
	jobject obj = (*env)->NewObject(env, cls, constr);
	if (!obj) {
//		FskDebugStr("initClassHelper failed to get %s object", path);
		return;
	}
	(*objptr) = (*env)->NewGlobalRef(env, obj);
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	JNIEnv *env;
//	FskDebugStr("JNI_OnLoad - vm is %x", vm);
	gJavaVM = vm;
	if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_4) != JNI_OK) {
		FskDebugStr("Failed to get env using GetEnv()");
		return -1;
	}
	initClassHelper(env, kPlay2AndroidPath, &gPlay2AndroidObject);

	return JNI_VERSION_1_4;
}

void androidGetThreadEnv() {
	int status;
	JNIEnv		*env;

	FskThread self = FskThreadGetCurrent();
	FskInstrumentedTypePrintfDebug(&gAndroidJNITypeInstrumentation, "androidGetThreadEnv");

    if (!self->jniEnv) {
		status = (*gJavaVM)->GetEnv(gJavaVM, (void**)&env, JNI_VERSION_1_4);
		if (status < 0) {
			FskInstrumentedTypePrintfMinimal(&gAndroidJNITypeInstrumentation, "failed to get JNI Env - AttachCurrentThread");
			status = (*gJavaVM)->AttachCurrentThread(gJavaVM, &env, NULL);
			if (status < 0) {
				//				FskDebugStr("android wake", "failed to attach");
				return;
			}
			self->attachedJava = 1;
		}
		jclass interfaceClass = (*env)->GetObjectClass(env, gPlay2AndroidObject);
		if (!interfaceClass) {
			FskInstrumentedTypePrintfMinimal(&gAndroidJNITypeInstrumentation, "failed to get class ref");
			androidDetachThread();
			return;
		}

		jmethodID method = (*env)->GetStaticMethodID(env, interfaceClass, "callback", "()V");
		if (!method) {
			FskInstrumentedTypePrintfMinimal(&gAndroidJNITypeInstrumentation, "failed to get method id");
			androidDetachThread();
			return;
		}
    	self->jniEnv = (int)env;
		self->javaMainWakeCallback = (int)method;
		self->play2AndroidClass = (int)interfaceClass;
	}
	else {
		env = (JNIEnv *)self->jniEnv;
	}
	FskInstrumentedTypePrintfDebug(&gAndroidJNITypeInstrumentation, "about to call javaMainWakeCallback");
	(*env)->CallStaticVoidMethod(env, (jclass)self->play2AndroidClass, (jmethodID)self->javaMainWakeCallback);

	FskThreadGetMain()->wakePending = 0;
}

//@@MDK -- should make this
// attach/detachAndroidThread and separate out the Wake part

void androidWakeMain() {
	int status;
	JNIEnv		*env;

	FskThread self = FskThreadGetCurrent();
	FskInstrumentedTypePrintfDebug(&gAndroidJNITypeInstrumentation, "androidWakeMain");

    if (!self->jniEnv) {
		jmethodID	cbMethod;

		status = (*gJavaVM)->GetEnv(gJavaVM, (void**)&env, JNI_VERSION_1_4);
		if (status < 0) {
			FskInstrumentedTypePrintfMinimal(&gAndroidJNITypeInstrumentation, "failed to get JNI Env - AttachCurrentThread");
			status = (*gJavaVM)->AttachCurrentThread(gJavaVM, &env, NULL);
			if (status < 0) {
//				FskDebugStr("android wake", "failed to attach");
				return;
			}
			self->attachedJava = 1;
		}

		jclass interfaceClass = (*env)->GetObjectClass(env, gPlay2AndroidObject);
		if (!interfaceClass) {
			FskInstrumentedTypePrintfMinimal(&gAndroidJNITypeInstrumentation, "failed to get class ref");
			androidDetachThread();
			return;
		}

		jmethodID method = (*env)->GetStaticMethodID(env, interfaceClass, "callback", "()V");
		if (!method) {
			FskInstrumentedTypePrintfMinimal(&gAndroidJNITypeInstrumentation, "failed to get method id");
			androidDetachThread();
			return;
		}

    	self->jniEnv = (int)env;
		self->javaMainWakeCallback = (int)method;
		self->play2AndroidClass = (int)interfaceClass;
	}
	else {
		env = (JNIEnv*)self->jniEnv;
	}

	FskInstrumentedTypePrintfDebug(&gAndroidJNITypeInstrumentation, "about to call javaMainWakeCallback");
	(*env)->CallStaticVoidMethod(env, (jclass)self->play2AndroidClass, (jmethodID)self->javaMainWakeCallback);

	FskThreadGetMain()->wakePending = 0;
}

void androidDetachThread() {
	FskThread self = FskThreadGetCurrent();
	if (self->attachedJava) {
		FskInstrumentedTypePrintfVerbose(&gAndroidJNITypeInstrumentation, "DetachCurrentThread");
		(*gJavaVM)->DetachCurrentThread(gJavaVM);
	}
	self->attachedJava = 0;
}



/*---------------------------------*/
/* fskJNIFetch_main uses a condition to halt calling thread until
 * we can call through the main thread and return wanted data.
 *
 */
FskErr fskJNIFetch_main(void *a, void *b, void *c, void *d) {
	int selector = (int)a;
	int value = (int)b;
	int len, ret;
	jstring jstr;
	char *str, *p;

	FskInstrumentedTypePrintfVerbose(&gAndroidMainBlockTypeInstrumentation, "fskJNIFetch: (%d:%s) val: %d", selector, selectorStr(JNI_CLASS_FETCH, selector), value);
	FskMutexAcquire(jniRespMutex);
	FskConditionSignal(jniRespCond);

	ret = (*gEnv)->CallIntMethod(gEnv, gKinomaPlayObject, methodID_fskJNIFetch, selector, value);
	FskInstrumentedTypePrintfVerbose(&gAndroidMainBlockTypeInstrumentation, " -- fskJNIFetch returns %d", ret);
	*(int*)c = ret;

	switch (selector) {
		case JNIFETCH_PHONE_LOG_START:
			if (ret != 0) {
				char **out = c;
				jstr = (*gEnv)->GetObjectField(gEnv, gKinomaPlayObject, fieldID_mCallLogString);

				GetJavaStringAsFskString(jstr, (FskMemPtr*)out);
			}
			else
				*(int*)c = 0;
			break;
	}

	FskMutexRelease(jniRespMutex);

	return kFskErrNone;
}

int androidJNIFetch(int what, int value) {
	FskThread	main, current;
	int ret = 0;

	main = FskThreadGetMain();
	current = FskThreadGetCurrent();

	if (main == current) {
		fskJNIFetch_main((void *)what, (void *)value, (void *)&ret, NULL);
	}
	else {
		FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidJNIFetch - trying to acquire jniRespMutex");
		FskMutexAcquire(jniRespMutex);
		FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "androidJNIFetch(%d, %d)", what, value);
		FskThreadPostCallback(main, (void*)fskJNIFetch_main, (void*)what, (void*)value, (void*)&ret, NULL);

		if (!gQuitting) {
			FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidJNIFetch(%d, %d) waiting for condition", what, value);
			FskConditionWait(jniRespCond, jniRespMutex);
			FskMutexRelease(jniRespMutex);
		}
	}

	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "androidJNIFetch returns %d", ret);
	return ret;
}

void myGetLanguage(char **lang) {
	if (lang) {
		FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "myGetLanguage returns '%s'", androidLangString);
		*lang = (char*)androidLangString;
	}
}

void myGetBasetime(int *s, int *usec) {
	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "myGetBasetime returns '%d:%d'", baseTimeSeconds, baseTimeUseconds);
	*s = baseTimeSeconds;
	*usec = baseTimeUseconds;
}


int myGetCardStatus() {
	return androidJNIFetch(JNIFETCH_SD_MOUNTED, 0);
}



FskErr androidLaunchDoc(void *a, void *b, void *c, void *d) {
    int what = (int)a;
    char *fullPath = (char*)b;

	jstring launchPath = (*gEnv)->NewStringUTF(gEnv, fullPath);

	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_launchDoc, what, launchPath);
	if (launchPath != NULL) {
		(*gEnv)->DeleteLocalRef(gEnv, launchPath);
	}
    return -1;
}

FskErr doLaunchDocument(int what, char *path) {
	FskThreadPostCallback(FskThreadGetMain(), (void*)androidLaunchDoc, (void*)what, (void*)path, NULL, NULL);
	return 0;
}


FskErr androidIntent_main(void *a, void *b, void *c, void *d) {
	int selector = (int)a;
	intentStruct intentData = (intentStruct)b;
	int len, ret;
	jstring jaction, juri, jpackage, jclass;
	char *str, *p;

	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidIntent_main: %d", selector);
	FskMutexAcquire(jniRespMutex);
	FskConditionSignal(jniRespCond);

	switch (selector) {
		case kIntentCanDo:
		case kIntentStart:
			jaction = (*gEnv)->NewStringUTF(gEnv, intentData->action);
			juri = (*gEnv)->NewStringUTF(gEnv, intentData->uri);
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "calling androidIntent %s %s", intentData->action, intentData->uri);

			ret = (*gEnv)->CallIntMethod(gEnv, gKinomaPlayObject, methodID_androidIntent, selector, jaction, juri);
			*(int*)d = ret;

			(*gEnv)->DeleteLocalRef(gEnv, jaction);
			(*gEnv)->DeleteLocalRef(gEnv, juri);
			break;

		case kIntentClassCanDo:
		case kIntentClassStart:
			jaction = (*gEnv)->NewStringUTF(gEnv, intentData->action);
			jpackage = (*gEnv)->NewStringUTF(gEnv, intentData->packageName);
			jclass = (*gEnv)->NewStringUTF(gEnv, intentData->className);
			FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "calling androidIntentClass %s %s", intentData->action, intentData->packageName, intentData->className);

			ret = (*gEnv)->CallIntMethod(gEnv, gKinomaPlayObject, methodID_androidIntentClass, selector, jaction, jpackage, jclass);
			*(int*)d = ret;

			(*gEnv)->DeleteLocalRef(gEnv, jaction);
			(*gEnv)->DeleteLocalRef(gEnv, jpackage);
			(*gEnv)->DeleteLocalRef(gEnv, jclass);
			break;
	}

	if (intentData) {
		FskMemPtrDispose(intentData->action);
		FskMemPtrDispose(intentData->uri);
		FskMemPtrDispose(intentData->packageName);
		FskMemPtrDispose(intentData->className);
		FskMemPtrDispose(intentData);
	}

	FskMutexRelease(jniRespMutex);

	return kFskErrNone;
}

int androidIntent(int what, char *action, char *uri, char *packageName, char *className) {
	FskThread		main, current;
	intentStruct	intentData = NULL;
	int 	ret = 0;
	FskErr	err = kFskErrNone;

	main = FskThreadGetMain();
	current = FskThreadGetCurrent();

	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "androidIntent called %d", what);
	err = FskMemPtrNewClear(sizeof(intentStructRecord), &intentData);
	if (err)
		goto fail;

	intentData->what = what;
	switch (what) {
		case kIntentCanDo:
		case kIntentStart:
			intentData->action = FskStrDoCopy(action);
			intentData->uri = FskStrDoCopy(uri);
			break;
		case kIntentClassCanDo:
		case kIntentClassStart:
			intentData->action = FskStrDoCopy(action);
			intentData->packageName = FskStrDoCopy(packageName);
			intentData->className = FskStrDoCopy(className);
			break;
	}

	if (main == current) {
		androidIntent_main((void *)what, (void *)intentData, NULL, (void*)&ret);
	}
	else {
		FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, "androidIntent - trying to acquire jniRespMutex");
		FskMutexAcquire(jniRespMutex);
		FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "androidIntent(%d, %s, %s)", what, action, uri);
		FskThreadPostCallback(main, (void*)androidIntent_main, (void*)what, (void*)intentData, NULL, (void*)&ret);

		if (!gQuitting) {
			FskConditionWait(jniRespCond, jniRespMutex);
			FskMutexRelease(jniRespMutex);
		}
	}
fail:
	if (err) {
		if (intentData) {
			FskMemPtrDispose(action);
			FskMemPtrDispose(uri);
			FskMemPtrDispose(packageName);
			FskMemPtrDispose(className);
			FskMemPtrDispose(intentData);
		}
	}
	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "androidIntent returns %d", ret);
	return ret;
}


Boolean androidCanHandleIntent(char *action, char *uri) {
	if (androidIntent(kIntentCanDo, action, uri, NULL, NULL)) {
		FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "- androidCanHandleIntent %s, %s - returning true", action, uri);
		return true;
	}
	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "- androidCanHandleIntent %s, %s - returning false", action, uri);
	return false;
}

int androidHandleIntent(char *action, char *uri) {
	return androidIntent(kIntentStart, action, uri, NULL, NULL);
}

Boolean androidCanHandleIntentClass(char *action, char *package, char *classname) {
	if (androidIntent(kIntentClassCanDo, action, NULL, package, classname))
		return true;
	return false;
}

int androidHandleIntentClass(char *action, char *package, char *classname) {
	return androidIntent(kIntentClassStart, action, NULL, package, classname);
}

jint JAVANAME(KinomaPlay_fskSetGPSStatus)(JNIEnv *env, jclass clazz, jint status) {
	if (myHWInfo.gpsStatus != status) {
		FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "GPS Status change - %d", status);
		myHWInfo.gpsStatus = status;
		FskSetPhoneHWInfo(&myHWInfo);
	}
	return 0;
}

jint JAVANAME(KinomaPlay_fskSetGPSInfo)(JNIEnv *env, jclass clazz, jdouble lat, jdouble lng, jdouble alt, jdouble heading, jdouble speed, jint status, jint visible, jdouble UTC, jdouble accuracy) {
	FskTimeRecord nowTime;

	myHWInfo.gpsStatus = status;
	myHWInfo.gpsLat = lat;
	myHWInfo.gpsLong = lng;
	myHWInfo.gpsAlt = alt;
	myHWInfo.gpsHeading = heading;
	myHWInfo.gpsSpeed = speed;
	myHWInfo.gpsVisible = visible;
	myHWInfo.gpsUTC = UTC;
	myHWInfo.gpsAccuracy = accuracy;

	{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	myHWInfo.gpsWhen = ((double)tv.tv_usec / 1000.0);	// msec
	myHWInfo.gpsWhen += (tv.tv_sec * 1000.0);			// secs -> msec
	}


	FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "GPS Status: %d - when %g", status, myHWInfo.gpsWhen);
	FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "GPS Info: lat: %g long: %g, alt: %g, heading: %g, speed %g, visible: %d, UTC: %g accuracy: %g", lat, lng, alt, heading, speed, visible, UTC, accuracy);
	FskSetPhoneHWInfo(&myHWInfo);
	return 0;
}


void JAVANAME(KinomaPlay_setFskKeyboardType)( JNIEnv *env, jobject thiz, jint kbdType ) {
	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "setFskKeyboardType %d", kbdType);
//	gAndroidKeyboardType = kbdType;
	myHWInfo.keyboardType = kbdType;
	FskSetPhoneHWInfo(&myHWInfo);
}


void JAVANAME(KinomaPlay_fskSetVolumeMax)( JNIEnv* env, jobject thiz, jint max) {
	androidMaxVolume = max;
}

#if SUPPORT_REMOTE_NOTIFICATION
jstring JAVANAME(KinomaPlay_fskGetEnvironment)( JNIEnv* env, jobject thiz, jstring keystr ) {
  const char *key = (*env)->GetStringUTFChars(env, keystr, NULL);
  char *value = FskEnvironmentGet((char *)key);
  if (!value) {
    return NULL;
  }
  return (*env)->NewStringUTF(env, value);
}
#endif


void androidSetVolume(int vol) {
	int scaled;

	scaled = (vol * androidMaxVolume) / 256;

	if (scaled != androidLastVolume) {
		androidJNIControl(JNICONTROL_SET_VOLUME, scaled);
		androidLastVolume = scaled;
	}
	else {
		FskInstrumentedTypePrintfDebug(&gAndroidGlueTypeInstrumentation, "androidSetVolume vol %d -- was already", vol);
	}

}

void androidSetVolumeDouble(double val) {
	int volume;
	volume = (int)(val * 256.0);
	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "androidSetVolumeDouble val %g - calculated to %d", val, volume);
	androidSetVolume(volume);
}

int androidGetVolume(void) {
	int vol;
	vol = (androidLastVolume * 256) / androidMaxVolume;
	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "androidGetVolume returns %d (unscaled %d)", vol, androidLastVolume);
	return vol;
}

double androidGetVolumeDouble(void) {
	int vol;
	double ret;

	ret = (double)androidGetVolume();
	ret = ret / 256.0;
	FskInstrumentedTypePrintfVerbose(&gAndroidGlueTypeInstrumentation, "androidGetVolumeDouble - calculated to %g", ret);
	return ret;
}

int androidGetRandomNumber(UInt32 *seed) {		// Reentrant random function from POSIX.1c.
	unsigned int next = *seed;
	int result;

	next *= 1103515245;
	next += 12345;
	result = (unsigned int) (next / 65536) % 2048;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int) (next / 65536) % 1024;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int) (next / 65536) % 1024;

	*seed = next;

	return result;
}

/********************************************************************************
 * androidWebView
 ********************************************************************************/

static void __kinoma_debug_log(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	__android_log_vprint(ANDROID_LOG_DEBUG, "kinoma", fmt, ap);
	va_end(ap);
}

#define LOG(msg, ...) __kinoma_debug_log("thread<%s> " msg, FskThreadGetCurrent()->name, ##__VA_ARGS__)


static FskThread sBrowserThread = NULL;

static void runThreadTaskSync(FskThreadCallback task, void *a, void *b, void *c, ...) {
	LOG("start execute task");

	FskMutexAcquire(jniRespMutex);
	FskConditionSignal(jniRespCond);
	task(a, b, c, NULL);
	FskMutexRelease(jniRespMutex);

	LOG("end execute task");
}

static void _callOnThreadSync(FskThread thread, FskThreadCallback task, void *a, void *b, void *c) {
	void *d;

	LOG("aquire mutex");

	FskMutexAcquire(jniRespMutex);
	FskThreadPostCallback(thread, runThreadTaskSync, task, a, b, c);

	LOG("wait mutex");
	if (!gQuitting) {
		FskConditionWait(jniRespCond, jniRespMutex);
		FskMutexRelease(jniRespMutex);
	}
}

static void callOnMainThreadSync(FskThreadCallback task, void *a, void *b, void *c) {
	FskThread thread = FskThreadGetMain();

	if (sBrowserThread == NULL) sBrowserThread = thread;

	if (thread == NULL || thread == FskThreadGetCurrent()) {
		task(a, b, c, NULL);
	} else {
		_callOnThreadSync(thread, task, a, b, c);
	}
}

static void callOnMainThreadAsync(FskThreadCallback task, void *a, void *b, void *c) {
	FskThread thread = FskThreadGetMain();

	if (sBrowserThread == NULL) {
		sBrowserThread = FskThreadGetCurrent();
	}

	if (thread == NULL || thread == FskThreadGetCurrent()) {
		task(a, b, c, NULL);
	} else {
		FskThreadPostCallback(thread, task, a, b, c, NULL);
	}
}

static void callOnBrowserThreadSync(FskThreadCallback task, void *a, void *b, void *c) {
	FskThread thread = sBrowserThread;

	if (thread == NULL || thread == FskThreadGetCurrent()) {
		task(a, b, c, NULL);
		return;
	}

	if (thread == NULL || thread == FskThreadGetCurrent()) {
		task(a, b, c, NULL);
	} else {
		_callOnThreadSync(thread, task, a, b, c);
	}
}

static void callOnBrowserThreadAsync(FskThreadCallback task, void *a, void *b, void *c) {
	FskThread thread = sBrowserThread;

	if (thread == NULL || thread == FskThreadGetCurrent()) {
		task(a, b, c, NULL);
		return;
	}

	if (thread == NULL || thread == FskThreadGetCurrent()) {
		task(a, b, c, NULL);
	} else {
		FskThreadPostCallback(thread, task, a, b, c, NULL);
	}
}

static FskBrowser webviewIdToBrowser(int webviewId) {
	return (FskBrowser)webviewId;
}

/* WebView implementation on main thread */

void androidWebViewCreate_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	FskErr *err = b;

	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_webviewCreate, webviewId);
}

void androidWebViewDispose_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_webviewDispose, webviewId);
}

void androidWebViewActivated_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	jint activeIt = (jint)b;
	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_webviewActivated, webviewId, activeIt);
}

void androidWebViewSetFrame_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	FskWindow win = FskWindowGetActive();
	double scale = FskPortDoubleScale(win->port, 1.0);
	FskRectangle area = (FskRectangle)b;
	jint x = area->x * scale;
	jint y = area->y * scale;
	jint width = area->width * scale;
	jint height = area->height * scale;
	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_webviewSetFrame, webviewId, x, y, width, height);
}

void androidWebViewAttach_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	FskWindow window = (FskWindow)b;
	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_webviewAttach, webviewId);
}

void androidWebViewDetach_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_webviewDetach, webviewId);
}

void androidWebViewGetURL_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	char **result = (char **)b;
	jstring url = (*gEnv)->CallObjectMethod(gEnv, gKinomaPlayObject, methodID_webviewGetURL, webviewId);
	if (url) {
		GetJavaStringAsFskString(url, (FskMemPtr *)result);
	}
}

void androidWebViewSetURL_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	jstring url = (*gEnv)->NewStringUTF(gEnv, (char *)b);
	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_webviewSetURL, webviewId, url);
}

void androidWebViewReload_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_webviewReload, webviewId);
}

void androidWebViewBack_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_webviewBack, webviewId);
}

void androidWebViewForward_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	(*gEnv)->CallVoidMethod(gEnv, gKinomaPlayObject, methodID_webviewForward, webviewId);
}

void androidWebViewCanBack_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	Boolean *result = (Boolean *)b;
	*result = (Boolean)(*gEnv)->CallIntMethod(gEnv, gKinomaPlayObject, methodID_webviewCanBack, webviewId);
}

void androidWebViewCanForward_main(void *a, void *b, void *c, void *d) {
	jint webviewId = (jint)a;
	Boolean *result = (Boolean *)b;
	*result = (Boolean)(*gEnv)->CallIntMethod(gEnv, gKinomaPlayObject, methodID_webviewCanForward, webviewId);
}

void androidWebViewHandleLoading_main(void *a, void *b, void *c, void *d) {
	FskBrowser browser = (FskBrowser)a;
	if (browser->didStartLoadCallback) {
		browser->didStartLoadCallback(browser, browser->refcon);
	}
}

void androidWebViewHandleLoaded_main(void *a, void *b, void *c, void *d) {
	FskBrowser browser = (FskBrowser)a;
	if (browser->didLoadCallback) {
		browser->didLoadCallback(browser, browser->refcon);
	}
}

void androidWebViewShouldHandleUrl_main(void *a, void *b, void *c, void *d) {
	FskBrowser browser = (FskBrowser)a;
	char *url = (char *)b;
	Boolean *handle = (Boolean *)c;
	*handle = browser->shouldHandleURLCallback(browser, url, browser->refcon);
	FskMemPtrDispose(url);
}

/* WebView interface on Fsk thread */

FskErr androidWebViewCreate(FskBrowser browser) {
	FskErr err = kFskErrNone;

	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewCreate_main, browser, &err, NULL);
	return err;
}

void androidWebViewDispose(FskBrowser browser) {
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewDispose_main, browser, NULL, NULL);
}

void androidWebViewActivated(FskBrowser browser, Boolean activeIt) {
	// FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	// callOnMainThreadSync(androidWebViewActivated_main, browser, (void*)(int)activeIt, NULL);
}

void androidWebViewSetFrame(FskBrowser browser, FskRectangle area) {
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewSetFrame_main, browser, (void*)area, NULL);
}

void androidWebViewAttach(FskBrowser browser, FskWindow window) {
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewAttach_main, browser, window, NULL);
}

void androidWebViewDetach(FskBrowser browser) {
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewDetach_main, browser, NULL, NULL);
}

char *androidWebViewGetURL(FskBrowser browser) {
	char *result = NULL;
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewGetURL_main, browser, &result, NULL);
	return result;
}

void androidWebViewSetURL(FskBrowser browser, char *url) {
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewSetURL_main, browser, url, NULL);
}

void androidWebViewReload(FskBrowser browser) {
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewReload_main, browser, NULL, NULL);
}

void androidWebViewBack(FskBrowser browser) {
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewBack_main, browser, NULL, NULL);
}

void androidWebViewForward(FskBrowser browser) {
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewForward_main, browser, NULL, NULL);
}

Boolean androidWebViewCanBack(FskBrowser browser) {
	Boolean result = false;
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewCanBack_main, browser, &result, NULL);
	return result;
}

Boolean androidWebViewCanForward(FskBrowser browser) {
	Boolean result = false;
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);
	callOnMainThreadSync(androidWebViewCanForward_main, browser, &result, NULL);
	return result;
}

void JAVANAME(KinomaPlay_webviewHandleLoading)(JNIEnv* env, jobject thiz, jint webviewId) {
	FskBrowser browser = webviewIdToBrowser(webviewId);
	callOnBrowserThreadAsync(androidWebViewHandleLoading_main, browser, NULL, NULL);
}

void JAVANAME(KinomaPlay_webviewHandleLoaded)(JNIEnv* env, jobject thiz, jint webviewId) {
	FskBrowser browser = webviewIdToBrowser(webviewId);
	callOnBrowserThreadAsync(androidWebViewHandleLoaded_main, browser, NULL, NULL);
}

jboolean JAVANAME(KinomaPlay_webviewShouldHandleUrl)(JNIEnv* env, jobject thiz, jint webviewId, jstring j_url) {
	FskBrowser browser = webviewIdToBrowser(webviewId);
	if (!browser->shouldHandleURLCallback) return true;

	char *url;
	Boolean handle;
	GetJavaStringAsFskString(j_url, (FskMemPtr*)&url);
	callOnBrowserThreadSync(androidWebViewShouldHandleUrl_main, browser, url, &handle);
	return handle;
}

/* WebView interface Script evaluation needs special thread handling. */

/*
	1. fsk thread will ask main thread to evaluate script, then wait.
	2. On main thread, script will be evaluated by loadUrl().
	3. On webview thread, the JSInterface will be called to capture result of the evaluation
	   and native resultHandler will be called on the same thread.
	4.
*/
static char *sWebViewEvaluationResult;

void androidWebViewEvaluateScript_main(void *a, void *b, void *c, void *d);
void androidWebViewHandleEvaluationResult_main(void *a, void *b, void *c, void *d);

char *androidWebViewEvaluateScript(FskBrowser browser, char *script) {
	FskInstrumentedTypePrintfDebug(&gAndroidMainBlockTypeInstrumentation, __func__);

	sWebViewEvaluationResult = NULL;
	LOG("will evaluatie script '%s'", script);

	FskMutexAcquire(jniRespMutex); // handle mutex manually
	LOG("acquire mutex");

	LOG("will post task on main thread");
	callOnMainThreadAsync(androidWebViewEvaluateScript_main, browser, script, NULL);
	LOG("did post task on main thread");
	LOG("condition wait");
	FskConditionWait(jniRespCond, jniRespMutex);
	LOG("release mutex");
	FskMutexRelease(jniRespMutex);
	LOG("did evaluate and result = '%s'", (sWebViewEvaluationResult ? sWebViewEvaluationResult : NULL));
	return sWebViewEvaluationResult;
}

void androidWebViewEvaluateScript_main(void *a, void *b, void *c, void *d) {
	LOG("acquire mutex on main thread");
	FskMutexAcquire(jniRespMutex);

	jint webviewId = (jint)a;
	jstring script = (*gEnv)->NewStringUTF(gEnv, (char *)b);
	char **result = (char **)c;
	LOG("will invoke Java method webviewEvaluateScript");
	Boolean run = (Boolean)(*gEnv)->CallIntMethod(gEnv, gKinomaPlayObject, methodID_webviewEvaluateScript, webviewId, script);
	if (!run) {
		LOG("failed to run. signal and release mutex now");
		FskConditionSignal(jniRespCond);
		FskMutexRelease(jniRespMutex);
	}
	// FskConditionSignal(jniRespCond);
	// FskMutexRelease(jniRespMutex);
}

void JAVANAME(KinomaPlay_webviewHandleEvaluationResult)(JNIEnv* env, jobject thiz, jstring j_result) {
	LOG("will store result");
	if (j_result) {
		GetJavaStringAsFskStringWithEnv(env, j_result, (FskMemPtr*)&sWebViewEvaluationResult);
	}

	// Don't use callOnMainThreadAsync() here because this in called from the web thread, which in unknow on fsk world.
	// Those thread is treated as main thread and callOnMainThreadAsync() will be confused.
	FskThreadPostCallback(FskThreadGetMain(), androidWebViewHandleEvaluationResult_main, NULL, NULL, NULL, NULL);
}

void androidWebViewHandleEvaluationResult_main(void *a, void *b, void *c, void *d)
{
	LOG("signal and release mutex on main thread");
	FskConditionSignal(jniRespCond);
	FskMutexRelease(jniRespMutex);
}

/********************************************************************************
 * Media Library
 ********************************************************************************/

#define ANDROID_LIBRARY_FETCH_USE_THREAD 0

char* androidLibraryFetch_core(JNIEnv *env, const char *kind, const char *option, const char *optionValue)
{
	jstring jkind, joption, joptionValue, jresult;
	FskMemPtr result = NULL;

	jkind = (*env)->NewStringUTF(env, (char*)kind);

	if (option) joption = (*env)->NewStringUTF(env, (char*)option);
	else joption = NULL;

	if (optionValue) joptionValue = (*env)->NewStringUTF(env, (char*)optionValue);
	else joptionValue = NULL;

	jresult = (*env)->CallObjectMethod(env, gKinomaPlayObject, methodID_libraryFetch, jkind, joption, joptionValue);

	(*env)->DeleteLocalRef(env, jkind);
	if (joption) (*env)->DeleteLocalRef(env, joption);
	if (joptionValue) (*env)->DeleteLocalRef(env, joptionValue);

	if (jresult) {
		FskMemPtr tmp;
		int len = (*env)->GetStringLength(env, jresult);
		FskMemPtrNew((len + 1) * 2, &tmp);
		(*env)->GetStringRegion(env, jresult, 0, len, (jchar *)tmp);
		tmp[len * 2] = '\0';
		(void)FskTextUnicode16LEToUTF8(tmp, len * 2, &result, NULL);
		FskMemPtrDispose(tmp);

		(*env)->DeleteLocalRef(env, jresult);
	}

	return result;
}

#if ANDROID_LIBRARY_FETCH_USE_THREAD

void androidLibraryFetch_main(void *a, void *b, void *c, void *d)
{
	const char *kind = (const char *)a;
	const char *option = (const char *)b;
	const char *optionValue = (const char *)c;
	char **ret = (char **)d;;

	FskMutexAcquire(jniRespMutex);
	FskConditionSignal(jniRespCond);

	*ret = androidLibraryFetch_core(gEnv, kind, option, optionValue);

	FskMutexRelease(jniRespMutex);
}

#endif

char* androidLibraryFetch(const char *kind, const char *option, const char *optionValue) {
#if ANDROID_LIBRARY_FETCH_USE_THREAD
	char *result;
	FskThread	main, current;

	main = FskThreadGetMain();
	current = FskThreadGetCurrent();

	if (main == current) {
		result = androidLibraryFetch_core(gEnv, kind, option, optionValue);
	} else {
		FskMutexAcquire(jniRespMutex);
		FskThreadPostCallback(main, (void*)androidLibraryFetch_main, (void *)kind, (void *)option, (void *)optionValue, (void *)&result);

		if (!gQuitting) {
			FskConditionWait(jniRespCond, jniRespMutex);
			FskMutexRelease(jniRespMutex);
		}
	}

	return result;
#else
	androidGetThreadEnv();
	FskThread self = FskThreadGetCurrent();
	JNIEnv *env = (JNIEnv *)self->jniEnv;

	return androidLibraryFetch_core(env, kind, option, optionValue);
#endif
}

FskErr androidLibrarySaveImage(char *data, int size)
{
	FskErr err = kFskErrNone;

	androidGetThreadEnv();
	FskThread self = FskThreadGetCurrent();
	JNIEnv *env = (JNIEnv *)self->jniEnv;

	jbyteArray dataArray = (*env)->NewByteArray(env, size);
	(*env)->SetByteArrayRegion(env, dataArray, 0, size, (jbyte*)data);

	(*env)->CallVoidMethod(env, gKinomaPlayObject, methodID_librarySaveImage, dataArray);

	(*env)->DeleteLocalRef(env, dataArray);

	return err;
}

static FskErr androidCreateFskBitmapFromBGRABytes(jint *src, jint width, jint height, FskBitmap *outBitmap)
{
	FskErr err;
	FskBitmap bitmap;
	UInt8 *dstP;
	SInt32 rb;
	UInt32 bytes;
	int i, j;

	err = FskBitmapNew(width, height, kFskBitmapFormatDefaultRGBA, &bitmap);
	if (err != kFskErrNone) return err;

	bytes = width * sizeof(jint);
	FskBitmapWriteBegin(bitmap, (void **)&dstP, &rb, NULL);
	for (i = 0; i < height; i++) {
		UInt8 *p1 = (UInt8 *)src, *p2 = dstP;

		for (j = 0; j < width; j++, p1 += sizeof(jint), p2 += sizeof(jint)) {
			p2[2] = p1[0] /* B */;
			p2[1] = p1[1] /* G */;
			p2[0] = p1[2] /* R */;
			p2[3] = p1[3] /* A */;
		}

		src += width;
		dstP += rb;
	}
	FskBitmapWriteEnd(bitmap);

	*outBitmap = bitmap;
	return kFskErrNone;
}

static FskErr androidBitmapToFskBitmap(JNIEnv *env, jobject androidBitmap, FskBitmap *outBitmap)
{
	FskErr err;

	*outBitmap = NULL;

	jclass bitmapClass = (*env)->GetObjectClass(env, androidBitmap);
	jmethodID getWidthID = (*env)->GetMethodID(env, bitmapClass, "getWidth", "()I");
	jmethodID getHeightID = (*env)->GetMethodID(env, bitmapClass, "getHeight", "()I");
	jmethodID getPixelsID = (*env)->GetMethodID(env, bitmapClass, "getPixels", "([IIIIIII)V");
	(*env)->DeleteLocalRef(env, bitmapClass);

	jint w = (*env)->CallIntMethod(env, androidBitmap, getWidthID);
	jint h = (*env)->CallIntMethod(env, androidBitmap, getHeightID);
	jint size = w * h;
	jintArray pixels = (*env)->NewIntArray(env, size);
	(*env)->CallVoidMethod(env, androidBitmap, getPixelsID, pixels, 0, w, 0, 0, w, h);

	jint *tmp = (*env)->GetIntArrayElements(env, pixels, NULL);
	err = androidCreateFskBitmapFromBGRABytes(tmp, w, h, outBitmap);
	(*env)->ReleaseIntArrayElements(env, pixels, tmp, 0);

	(*env)->DeleteLocalRef(env, pixels);

	return err;
}

FskErr androidLibraryThumbnail(const char *kind, long id, Boolean micro, FskBitmap *outBitmap)
{
	FskErr err;
	jstring jkind;
	jobject bitmap;

	*outBitmap = NULL;

	androidGetThreadEnv();
	FskThread self = FskThreadGetCurrent();
	JNIEnv *env = (JNIEnv *)self->jniEnv;

	jkind = (*env)->NewStringUTF(env, (char*)kind);

	bitmap = (*env)->CallObjectMethod(env, gKinomaPlayObject, methodID_libraryThumbnail, jkind, id, micro);

	(*env)->DeleteLocalRef(env, jkind);

	if (bitmap) {
		err = androidBitmapToFskBitmap(env, bitmap, outBitmap);
		(*env)->DeleteLocalRef(env, bitmap);
	} else {
		err = kFskErrNotFound;
	}

	return err;
}

char *androidLibraryGetSongPath(long id, int index)
{
	char *result = NULL;

	androidGetThreadEnv();
	FskThread self = FskThreadGetCurrent();
	JNIEnv *env = (JNIEnv *)self->jniEnv;

	jstring jresult = (*env)->CallObjectMethod(env, gKinomaPlayObject, methodID_libraryGetSongPath, id, index);

	if (jresult) {
		FskMemPtr tmp;
		int len = (*env)->GetStringLength(env, jresult);
		FskMemPtrNew((len + 1) * 2, &tmp);
		(*env)->GetStringRegion(env, jresult, 0, len, (jchar *)tmp);
		tmp[len * 2] = '\0';
		(void)FskTextUnicode16LEToUTF8(tmp, len * 2, &result, NULL);
		FskMemPtrDispose(tmp);

		(*env)->DeleteLocalRef(env, jresult);
	}

	return result;
}


/********************************************************************************
 * fsk_android_log_print
 ********************************************************************************/

int fsk_android_log_print(int prio, const char *tag, const char *fmt, ...) {
	int retVal;
	va_list ap;
	va_start(ap, fmt);
	retVal = __android_log_vprint(prio, tag, fmt, ap);
	va_end(ap);
	return retVal;
}


/********************************************************************************
 * fsk_android_log_vprint
 ********************************************************************************/

int fsk_android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap) {
	return __android_log_vprint(prio, tag, fmt, ap);
}

