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
#ifndef __KINOMAINTERFACELIB__
#define __KINOMAINTERFACELIB__

#include <stdarg.h>
#include <jni.h>

#include "Fsk.h"
#include "FskFiles.h"
#include "FskFrameBuffer.h"
#include "FskThread.h"
#include "FskWindow.h"
#include "FskBrowser.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifndef OBJECTBASE
	#define OBJECTBASE	Java_com_kinoma_kinomaplay_
#endif /* OBJECTBASE */

#define mY( x, y ) x##y
#define mYY( x, y ) mY( x, y )
#define JAVANAME( x ) mYY( OBJECTBASE , x )


/*
 * From KinomaInterface.cpp, froyo.cpp, gingerbread.cpp
 */


void		JAVANAME(KinomaPlay_setDeviceUsername)( JNIEnv *env, jobject thiz, jstring deviceName);

extern char *androidLangString;
void		JAVANAME(KinomaPlay_setAndroidLanguage)( JNIEnv *env, jobject thiz, jstring lang);

void		JAVANAME(KinomaPlay_setAndroidBasetime)( JNIEnv *env, jobject thiz, jint s, jint ms);

/* Class:		com_kinoma_kinomaplay_FskView
 * Method:		setFskSurface
 * Signature:	(Landroid/view/Surface;)I
 */
jint			JAVANAME(FskView_setFskSurface)(JNIEnv* env, jobject viewObj, jobject surfaceObject);

/* Class:		com_kinoma_kinomaplay_FskView
 * Method:		unsetFskSurface
 * Signature:	()I
 */
jint			JAVANAME(FskView_unsetFskSurface)(JNIEnv* env, jobject viewObj);

/* Class:		com_kinoma_kinomaplay_FskView
 * Method:		doFskSurfaceChanged
 * Signature:	(II)I
 */
jint			JAVANAME(FskView_doFskSurfaceChanged)(JNIEnv* env, jobject viewObj, jint width, jint height);

/* Class:		com_kinoma_kinomaplay_FskView
 * Method:		doSizeAboutToChange
 * Signature:	(IIII)I
 */
jint			JAVANAME(FskView_doSizeAboutToChange)(JNIEnv* env, jobject viewObj, jint width, jint height, jint newWidth, jint newHeight);

void		doFskSurfaceChanged(void *a, void *b, void *c, void *d);

void		FskWindowAndroidSizeChanged(int win);
void		dupeBitmap(FskBitmap from, FskBitmap to, int wackRB);

jboolean	JAVANAME(RemoteControlReceiver_doFskKeyEvent)(JNIEnv* env, jclass clazz, jint keyCode, jint modifiers, jint action, jint param, jint repeat);
jboolean	JAVANAME(KinomaPlay_doFskKeyEvent)(JNIEnv* env, jclass clazz, jint keyCode, jint modifiers, jint action, jint param, jint repeat);
jboolean	JAVANAME(KinomaPlay_doFskMotionEvent)(JNIEnv* env, jclass clazz, jint action, jint x, jint y, jint deltaMS);
jboolean	JAVANAME(KinomaPlay_doFskMotionMultiple)(JNIEnv* env, jclass clazz, jintArray arr);

void		JAVANAME(KinomaPlay_doFskOnTextChanged)(JNIEnv* env, jclass clazz, jstring str, jint start, jint before, jint count);
jboolean	JAVANAME(KinomaPlay_doFskMotionTouch)(JNIEnv* env, jclass clazz, jint action, jint pointer, jint x, jint y, jint deltaMS);
jboolean	JAVANAME(KinomaPlay_doFskMotionMultipleTouch)(JNIEnv* env, jclass clazz, jintArray arr);
void		JAVANAME(KinomaPlay_setWindowUpdateInterval)(JNIEnv* env, jclass clazz, jlong interval);
void		JAVANAME(KinomaPlay_fskWindowUpdate)(JNIEnv* env, jclass clazz, jlong time);
void		JAVANAME(KinomaPlay_doFskOnTextChanged)(JNIEnv* env, jclass clazz, jstring str, jint start, jint before, jint count);

int			addAMoved(int x, int y, int ms, int pointer);
void		motionMovedCB(struct FskTimeCallBackRecord *callback, const FskTime time, void *param);
FskErr		issueEvent(int cod, int x, int y, UInt32 clicks, FskTime mouseTime, int mod, FskWindow win);

FskErr		androidDoOrQueue(FskWindow window, FskEvent event);

void		attachJava(FskThread self);

void		JAVANAME(KinomaPlay_fskPhoneOperatorChanged)( JNIEnv *env, jobject thiz, jstring oper);
void 		JAVANAME(KinomaPlay_fskPhoneSSIDChanged)( JNIEnv *env, jobject thiz, jstring SSID);

void		JAVANAME(KinomaPlay_setStaticDeviceInfo)(JNIEnv *env, jobject thiz, jstring model, jstring osVersion,
													jint buttonsReversed, jint needsOrientationRotate, jint touchCapable,
													jstring imei, jstring uuid, jint hDpi, jint vDpi, jint densityDpi, jint screenWidth, jint screenHeight,
													jint statusBarHeight, jstring dataDir, jstring appPath, jstring externalDir);

void		JAVANAME(KinomaPlay_setDeviceOrientation)( JNIEnv *env, jobject thiz, jint orientation);

void		JAVANAME(KinomaPlay_setSpecialPaths)( JNIEnv *env, jobject thiz, jstring musicPath, jstring podcastPath, jstring picturesPath, jstring moviesPath, jstring downloadsPath, jstring dcimPath);

void		myGetSpecialPaths(char **musicDir, char **podcastDir, char **picturesDir, char **moviesDir, char **downloadsDir, char **dcimDir);
void		myGetModelInfo(char **modelName, char **osVersion, int *hasTouch, int *buttonsMirrored, int *usesGL);
void		myGetDPI(int *h, int *v);
FskErr		getLastXY(SInt32 *x, SInt32 *y);
void		androidGetScreenSize(SInt32 *x, SInt32 *y, SInt32 *xmax, SInt32 *ymax);

void*		androidGetScreenSurface(void);
void		androidCheckSizeChangeComplete(void);

int			androidHasSurface();
void		androidDoWindowActivated();
void		sendResidualMouseUps(void);
int		trackMouseUp(int pointer);
void		trackMouseDown(int pointer);

int			androidSystemBarShow(int what);

void		androidSetFBGlobals(FskFBGlobals globals);

int			androidMidWindowResize();
void		androidAfterWindowResize();

char*		androidGetStaticIMEI();
char*		androidGetStaticAppDir();
char*		androidGetStaticDataDir();
char*		androidGetStaticExternalDir();
char*		androidGetDeviceUsername();

			#if DEBUG_CUSTOMER_FILE
FILE*		androidGetCustomerDebugFile();
			#endif

void		sendEventWindowUpdate(FskWindow win, Boolean redrawAll, Boolean skipBeforeUpdate);

FskErr		androidTweakKeyboardSelection(UInt32 selBegin, UInt32 selEnd);
FskErr		androidTweakKeyboard(char *text, UInt32 textCount, UInt32 keyboardMode);

Boolean		GLHasASurface();


/*
 * From KinomaLib.c
 */


double		androidGetVolumeDouble(void);
void		androidSetVolumeDouble(double val);

int			androidGetAppsCookie();

FskErr		androidFskThreadRunloopCycle(SInt32 msec, SInt32 *outmsec);

int			PreInit(void);
int			PostInit(void);
FskErr		doLaunchDocument(int what, char *path);
void		myGetModelInfo(char **modelName, char **osVersion, int *hasTouch, int *buttonsMirrored, int *usesGL);

void		androidiNotifyTerminate();
FskErr		androidDirectoryChangeNotifierDispose(FskDirectoryChangeNotifier dirChangeNtf);
FskErr		androidDirectoryChangeNotifierNew(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNtf);

int			androidSystemBarHeight();
int			androidSystemBarShow(int what);

Boolean		androidCanHandleIntent(char *action, char *uri);
int			androidHandleIntent(char *action, char *uri);
Boolean		androidCanHandleIntentClass(char *action, char *packageName, char *className);
int			androidHandleIntentClass(char *action, char *packageName, char *className);

void		androidEnergySaver(UInt32 mask, UInt32 value);
void		androidWakeMain();
void		androidGetThreadEnv();

FskErr		androidTweakKeyboardSelection(UInt32 selBegin, UInt32 selEnd);
FskErr		androidTweakKeyboard(char *text, UInt32 textCount, UInt32 keyboardMode);
void		androidKeyboardHints(char *hint);
void		androidSetTERect(FskRectangle r);


char*		doFetchContacts(int style);

int			doStartContacts();
void		doStopContacts();
char*		doNextContact(char **image);
char*		doIdxContact(int idx, char **image);
void		doDial(char *number);
void		androidSendSMS(char *number, char *message);
void		androidSendMail(char *whoto, char *subject, char *message);
void		androidSendAttachments(char *whoto, char *subject, char *message, char *attach, int attachSize);

FskErr		androidFskContactsCB(void *a, void *b, void *c, void *d);

void		fskJNIControl_main(void *a, void *b, void *c, void *d);
FskErr		fskJNIFetch_main(void *a, void *b, void *c, void *d);
void		androidJNIControl(int what, int value);
int			androidJNIFetch(int what, int value);

FskErr		androidIntent_main(void *a, void *b, void *c, void *d);
int			androidIntent(int what, char *action, char *uri, char *packageName, char *className);

FskErr		androidIMECallback(void *a, void *b, void *c, void *d);
Boolean		doIsIMEEnabled();
void		doIMEEnable(Boolean enable);

char*		doFetchApps(int style);
void		doLaunchApp(char *pkgName, char *actName);
char*		doFetchAppIcon(char *packageName, int iconNo);

void		androidDetachThread();

void		androidSetFBGlobals(FskFBGlobals fbGlobals);
FskErr		getLastXY(SInt32 *x, SInt32 *y);
void		androidGetScreenSize(SInt32 *x, SInt32 *y, SInt32 *xmax, SInt32 *ymax);
void		androidSetTransitionState(int state);

void		androidAfterWindowResize();
int			androidMidWindowResize();

void		sendResidualMouseUps(void);


char*		androidGetStaticDataDir();
char*		androidGetStaticExternalDir();
char*		androidGetStaticAppDir();
char*		androidGetStaticIMEI();
char*		androidGetDeviceUsername();

void		androidKeepWifiAlive();
void		androidLetWifiDie();

void		androidResetHomeScreen();

int			androidGetRandomNumber(UInt32 *seed);

char*		androidLibraryFetch(const char *kind, const char *option, const char *optionValue);
FskErr		androidLibraryThumbnail(const char *kind, long id, Boolean micro, FskBitmap *outBitmap);
char*		androidLibraryGetSongPath(long id, int index);
FskErr 		androidLibrarySaveImage(char *data, int size);

void		doSetContinuousDrawing(Boolean continuousDrawing);
void		doGetContinuousDrawingUpdateInterval(UInt32 *interval);

/** Runtime query as to whether we are using OpenGL for rendering.
 *	\return	true	if OpenGL is used for rendering.
 *	\return false	otherwise.
 **/
Boolean	androidIsUsingOpenGL(void);


/**
 * Set the drawing surface.
 * Class:		com_kinoma_kinomaplay_FskViewGL.
 * Method:		setFskSurface.
 * Signature:	(Landroid/view/Surface;)I.
 *	\param[in]	env				the Java runtime environment.
 *	\param[in]	viewObj			an instantiation of the FskViewGL class.
 *	\param[in]	surfaceObject	the object, an instantiation of the FskViewGL class.
 *	\return		1				if successful,
 *	\return		0				if unsuccessful.
 **/
jint	JAVANAME(FskViewGL_setFskSurface)(JNIEnv* env, jobject viewObj, jobject surfaceObj);


/**
 * Unset the drawing surface.
 * Class:		com_kinoma_kinomaplay_FskViewGL.
 * Method:		unsetFskSurface.
 * Signature:	()I.
 *	\param[in]	env				the Java runtime environment.
 *	\param[in]	viewObj			the FskViewGL object.
 *	\return		1				if successful,
 *	\return		0				if unsuccessful.
 **/
jint	JAVANAME(FskViewGL_unsetFskSurface)(JNIEnv* env, jobject viewObj);

//jint		JAVANAME(FskViewGL_doSurfaceCreated)(EGLConfig config);


/**
 * Update the drawing surface.
 * Class:		com_kinoma_kinomaplay_FskViewGL.
 * Method:		doFskSurfaceChanged.
 * Signature:	(II)I.
 *	\param[in]	env				the Java runtime environment.
 *	\param[in]	viewObj			the FskViewGL object ID.
 *	\param[in]	surfaceObject	the object, an instantiation of the FskViewGL class.
 *	\param[in]	width			the new width.
 *	\param[in]	height			the new height.
 *	\return		1				if successful,
 *	\return		0				if unsuccessful.
 **/
jint	JAVANAME(FskViewGL_doFskSurfaceChanged)(JNIEnv* env, jobject viewObj, jint width, jint height);


/**
 * Prepare for the size about to change.
 * Class:		com_kinoma_kinomaplay_FskViewGL.
 * Method:		doSizeAboutToChange.
 * Signature:	(IIII)I.
 *	\param[in]	env				the Java runtime environment.
 *	\param[in]	viewObj			the FskViewGL object.
 *	\return		1				if successful,
 *	\return		0				if unsuccessful.
 **/
jint	JAVANAME(FskViewGL_doSizeAboutToChange)(JNIEnv* env, jobject viewObj, jint width, jint height, jint newWidth, jint newHeight);


/**
 * Shut down GL when pausing.
 * Class:		com_kinoma_kinomaplay_KinomaPlay.
 * Method:		doPause.
 * Signature:	()I.
 *	\param[in]	env				the Java runtime environment.
 *	\param[in]	clazz			the KinomaPlay class.
 *	\return		1				if successful,
 *	\return		0				if unsuccessful.
 **/
jint JAVANAME(KinomaPlay_doPause)(JNIEnv* env, jclass clazz);


/**
 * Restore the GL environment after resuming from pause.
 * Class:		com_kinoma_kinomaplay_FskViewGL.
 * Method:		doResume.
 * Signature:	()I.
 *	\param[in]	env				the Java runtime environment.
 *	\param[in]	clazz			the KinomaPlay class.
 *	\return		1				if successful,
 *	\return		0				if unsuccessful.
 **/
jint JAVANAME(KinomaPlay_doResume)(JNIEnv* env, jclass clazz);


/**	Send a formatted string to the log, used like printf(fmt,...).
 *	\param[in]	prio	One of ANDROID_LOG_VERBOSE(1), ANDROID_LOG_DEBUG(2), ANDROID_LOG_INFO(3), ANDROID_LOG_WARN(4),
 *						ANDROID_LOG_ERROR(5), ANDROID_LOG_FATAL(6), or ANDROID_LOG_SILENT(7). From <android/log.h>.
 *	\param[in]	tag		A tag which can be filtered in the log window of Eclipse.
 *	\param[in]	fmt		A printf-style format. Followed by extra arguments.
 *	\return		the number of characters printed	if successful,
 *	\return		a negative number					if there was an error.
 **/
int		fsk_android_log_print(int prio, const char *tag, const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__ ((format(printf, 3, 4)))
#endif
;


/**	A variant of fsk_android_log_print() that takes a va_list to list additional parameters.
 *	\param[in]	prio	One of ANDROID_LOG_VERBOSE(1), ANDROID_LOG_DEBUG(2), ANDROID_LOG_INFO(3), ANDROID_LOG_WARN(4),
 *						ANDROID_LOG_ERROR(5), ANDROID_LOG_FATAL(6), or ANDROID_LOG_SILENT(7). From <android/log.h>.
 *	\param[in]	tag		A tag which can be filtered in the log window of Eclipse.
 *	\param[in]	fmt		A printf-style format. Followed by extra arguments.
 *	\param[in]	ap		A variable-argument data structure, initialized with the help of <stdarg.h>.
 *	\return		the number of characters printed	if unsuccessful,
 *	\return		a negative number					if there was an error.
 **/
int		fsk_android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap);


extern FskInstrumentedTypeRecord	gAndroidTETypeInstrumentation;
extern FskInstrumentedTypeRecord	gAndroidMainBlockTypeInstrumentation;
extern FskInstrumentedTypeRecord	gAndroidEnergyTypeInstrumentation;
extern FskInstrumentedTypeRecord	gAndroidPhoneStateTypeInstrumentation;
extern FskInstrumentedTypeRecord	gAndroidJNITypeInstrumentation;
extern FskInstrumentedTypeRecord	gAndroidGlueTypeInstrumentation;
extern FskInstrumentedTypeRecord	gAndroidWindowTypeInstrumentation;
extern FskInstrumentedTypeRecord	gAndroidTouchTypeInstrumentation;
extern FskInstrumentedTypeRecord	gAndroidEventTypeInstrumentation;
extern FskMutex						gSurfaceChangedMutex;

extern int usingOpenGL;

FskErr androidWebViewCreate(FskBrowser browser);
void androidWebViewDispose(FskBrowser browser);
void androidWebViewActivated(FskBrowser browser, Boolean activeIt);
void androidWebViewSetFrame(FskBrowser browser, FskRectangle area);
void androidWebViewAttach(FskBrowser browser, FskWindow window);
void androidWebViewDetach(FskBrowser browser);
char *androidWebViewGetURL(FskBrowser browser);
void androidWebViewSetURL(FskBrowser browser, char *url);
char *androidWebViewEvaluateScript(FskBrowser browser, char *script);
void androidWebViewReload(FskBrowser browser);
void androidWebViewBack(FskBrowser browser);
void androidWebViewForward(FskBrowser browser);
Boolean androidWebViewCanBack(FskBrowser browser);
Boolean androidWebViewCanForward(FskBrowser browser);
void JAVANAME(KinomaPlay_webviewHandleLoading)(JNIEnv* env, jobject thiz, jint webviewId);
void JAVANAME(KinomaPlay_webviewHandleLoaded)(JNIEnv* env, jobject thiz, jint webviewId);
jboolean JAVANAME(KinomaPlay_webviewShouldHandleUrl)(JNIEnv* env, jobject thiz, jint webviewId, jstring url);
void JAVANAME(KinomaPlay_webviewHandleEvaluationResult)(JNIEnv* env, jobject thiz, jstring j_result);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KINOMAINTERFACELIB__ */
