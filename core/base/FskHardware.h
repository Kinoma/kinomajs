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
#ifndef __FSKHARDWARE_H__
#define  __FSKHARDWARE_H__

#include <stdarg.h>

#include "Fsk.h"
#include "FskFiles.h"
#include "FskTime.h"
#include "FskFrameBuffer.h"
#include "FskBrowser.h"

#ifdef __cplusplus
extern "C" {
#endif

#if TARGET_OS_ANDROID
enum {
    kFskPhonePropertyOperator = 1,
    kFskPhonePropertyMissedCalls = 2,
    kFskPhonePropertyUnreadMessages = 3,
    kFskPhonePropertySignalStrength = 4,
    kFskPhonePropertyBattery = 5,
    kFskPhonePropertyCellularNetworkType = 6,           // no notifications... poll when signal strength changes
    kFskPhonePropertyDataNetworkType = 7,
    kFskPhonePropertyPluggedIn = 8,
    kFskPhonePropertyDataNetworkSignalStrength = 9,
	kFskPhonePropertyDataNetworkBitRate = 10,
	kFskPhonePropertyCellularNetworkBitRate = 11,
	kFskPhonePropertyInputMode = 12,
	kFskPhonePropertyConnected = 13,
	kFskPhonePropertyIMEI = 14,
	kFskPhonePropertyOrientation = 15,
	kFskPhonePropertyGPS = 16,
	kFskPhonePropertyBacklight = 17,

    kFskPhonePropertyUnreadVoiceMails = 18,
    kFskPhonePropertyUnreadSMS = 19,

    kFskPhonePropertySystemBar = 21,
    kFskPhonePropertySystemBarHeight = 22,

    kFskPhonePropertySystemVolume = 23,
    kFskPhonePropertySystemLanguage = 24,
	kFskPhonePropertyUUID = 25,
	kFskPhonePropertyMACAddress = 26,
	kFskPhonePropertySSID = 27
};

typedef void (*FskPhonePropertyCB)(SInt32 property);

void FskHardwareSetPhonePropertyCallback(FskPhonePropertyCB cb);

enum {
	kFskPhoneNetworkCellularUnknown = 0,
	kFskPhoneNetworkCellularNone = 1,
	kFskPhoneNetworkCellularGSM,
	kFskPhoneNetworkCellularWCDMA,		// wcdma
	kFskPhoneNetworkCellularUMA,
	kFskPhoneNetworkCellularIMS,
	kFskPhoneNetworkCellularHSDPA,		// hsdpa
	kFskPhoneNetworkCellularGPRS,		// gprs
	kFskPhoneNetworkCellular1XEVD0,		// 1xevdo
	kFskPhoneNetworkCellular1XEVDV,		// 1xevdv
	kFskPhoneNetworkCellularEDGE,		// edge
	kFskPhoneNetworkCellularUMTS,		// umts
	kFskPhoneNetworkCellular1xRTT		// 1xrtt
};

enum {
	kFskPhoneNetworkDataEthernet = 1,
	kFskPhoneNetworkDataWifi,
	kFskPhoneNetworkDataCellular,
	kFskPhoneNetworkDataUnknown,
};
enum {
	kFskCellularNetworkStateDisconnected = 0,
	kFskCellularNetworkStateConnecting = 1,
	kFskCellularNetworkStateConnected = 2,
	kFskCellularNetworkStateSuspended = 3,
	kFskCellularNetworkStateOff = 4,
	kFskCellularNetworkStateEmergencyOnly = 5,
	kFskDataNetworkStateDisconnected = 0,
	kFskDataNetworkStateConnecting = 1,
	kFskDataNetworkStateConnected = 2,
	kFskDataNetworkStateSuspended = 3,
	kFskDataNetworkStateOff = 4,
};

typedef struct FskPhoneHWInfoRecord {
    UInt8       batteryLevel;
    Boolean     chargerPlugged;

	int			activeNetworkType;
    int         cellularNetworkType;
    int         cellularNetworkState;
    UInt8       signalStrength;
    int         dataNetworkType;
    int         dataNetworkState;
    UInt8       dataSignalStrength;

    int         missedCalls;
    int         unreadMessages;
    char*       operatorStr;
    char*       imei;
	int			backlightOn;
	char*		language;

	int			gpsStatus;
	double		gpsLat;
	double		gpsLong;
	double		gpsAlt;
	double		gpsHeading;
	double		gpsSpeed;
	int			gpsVisible;
	double		gpsUTC;
	double		gpsWhen;
	double		gpsAccuracy;

	int			orientation;
	Boolean		needsRotationTransition;
	int			keyboardType;

	int			wifiAddr;

	int			afterInit;

	char		*MACAddress;
	char		*uuid;
	char		*ssid;

	FskTimeRecord	baseTime;

} FskPhoneHWInfoRecord, *FskPhoneHWInfo;

extern FskPhoneHWInfo gFskPhoneHWInfo;

FskErr FskSetPhoneHWInfo(FskPhoneHWInfo info);
FskErr FskPhoneHWInfoSetCB(FskThread thread, SInt32 prop, FskThreadCallback cb, void *refcon);

enum {
	kFskLineCallStateDialing	= 0,
	kFskLineCallStateIdle,
	kFskLineCallStateOffering,
	kFskLineCallStateConnected,
	kFskLineCallStateDisconnected
};

typedef FskErr (*FskCallStatusCB)(int line, int state);

FskErr FskSetCallStatusCB(FskCallStatusCB cb);
FskErr FskSetCallStatus(int line, int state);



typedef struct {
	long	memTotal;
	long	memFree;
	long	mallocable;
} FskHWMemoryInfoRecord, *FskHWMemoryInfo;

#define JNICONTROL_SLEEP		1
//#define JNICONTROL_DIM_OK		2
#define JNICONTROL_GPS_ON		3
#define JNICONTROL_GPS_OFF		4
#define JNICONTROL_SPLASH_OFF	5
#define JNICONTROL_WAKE_MAIN	6
#define JNICONTROL_KEEP_WIFI_ALIVE	7

#define JNIFETCH_SD_MOUNTED		1		// no value
#define JNIFETCH_IME_ENABLED	2		// no value

#define JNIFETCH_PHONE_LOG_START	3		// no value
#define JNIFETCH_PHONE_LOG_STOP		4		// no value
#define JNIFETCH_PHONE_LOG_NEXT		5		// no value

#define JNIFETCH_TRAMPOLINE_INSTALLED	7	// no value - returns 0 or 1

typedef FskErr	(*FskSetWallpaperCBFn)(char *path);
typedef FskErr	(*FskGetMemoryInfoCBFn)(FskHWMemoryInfo memInfo);

typedef void	(*FskGetDPIFn)(int *hDpi, int *vDpi, int *densityDpi);
typedef void	(*FskGetModelInfoFn)(char **modelName, char **osVersion, int *hasTouch, int *buttonsMirrored, int *usesGL);

typedef void	(*FskGetLanguageFn)(char **lang);
typedef void	(*FskGetBasetimeFn)(int *sec, int *usec);

typedef FskErr	(*FskLaunchDocumentCBFn)(int what, char *path);
typedef void	(*FskJNIControlFn)(int what, int value);
typedef int		(*FskJNIFetchFn)(int what, int value);

typedef char *	(*FskFetchContactsFn)(int what);

typedef int		(*FskStartContactsFn)(int sort);
typedef void 	(*FskStopContactsFn)(void);
typedef char *	(*FskNextContactFn)(char **image);
typedef char *	(*FskIdxContactFn)(int what, char **image);

typedef void	(*FskDialFn)(char *number);
typedef void	(*FskSendSMSFn)(char *number, char *message);
typedef void	(*FskSendMailFn)(char *whoto, char *subject, char *message);
typedef void	(*FskSendMailAttachmentsFn)(char *whoto, char *subject, char *message, char *attachments, int attachmentsSize);

typedef Boolean	(*FskIsIMEEnabledFn)();
typedef void	(*FskIMEEnableFn)(Boolean enable);

typedef char *	(*FskFetchAppsFn)(int what);
typedef void	(*FskLaunchAppFn)(char *pkgName, char *actName);
typedef char *	(*FskFetchAppIconFn)(char *packageName, int iconNo);

typedef FskErr	(*FskDirChangeNotNewFn)(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNtf);
typedef FskErr	(*FskDirChangeNotDisposeFn)(FskDirectoryChangeNotifier dirChangeNtf);
typedef void	(*FskDirChangeNotTerminateFn)(void);

typedef int		(*FskSystemBarHeightFn)();
typedef int		(*FskSystemBarShowFn)(int what);

typedef	Boolean (*FskCanHandleIntentFn)(char *action, char *uri);
typedef	int		(*FskHandleIntentFn)(char *action, char *uri);
typedef	Boolean (*FskCanHandleIntentClassFn)(char *action, char *package, char *classname);
typedef	int		(*FskHandleIntentClassFn)(char *action, char *package, char *classname);

typedef void	(*FskEnergySaverFn)(UInt32 mask, UInt32 value);

typedef void	(*FskWakeMainFn)();
typedef void	(*FskDetachThreadFn)();

typedef FskErr	(*FskAudioInNewFn)(void * audioIn);
typedef FskErr	(*FskAudioInDisposeFn)(void * audioIn);
typedef FskErr	(*FskAudioInGetFormatFn)(void * audioIn);
typedef FskErr	(*FskAudioInSetFormatFn)(void * audioIn);
typedef FskErr	(*FskAudioInStartFn)(void * audioIn);
typedef FskErr	(*FskAudioInStopFn)(void * audioIn);

typedef FskErr	(*FskGetLastXYFn)(SInt32 *x, SInt32 *y);
typedef void	(*FskGetScreenSizeFn)(SInt32 *x, SInt32 *y, SInt32 *xmax, SInt32 *ymax);

typedef FskErr	(*FskTweakKbdFn)(char *text, UInt32 textCount, UInt32 keyboardMode);
typedef FskErr	(*FskTweakKbdSelectionFn)(UInt32 selBegin, UInt32 selEnd);
typedef FskErr	(*FskGetKbdRectFn)(FskRectangleRecord *r);
typedef void	(*FskSetTERectFn)(FskRectangleRecord *r);

typedef void	(*FskCheckSizeChangeCompleteFn)(void);
typedef void	(*FskAfterWindowResizeFn)(void);
typedef int		(*FskGetMidWindowResizeFn)();
typedef int		(*FskNoWindowDontDrawFn)();

typedef void	(*FskSetTransitionStateFn)(int state);

typedef char*	(*FskGetStaticDataDirFn)();
typedef char*	(*FskGetStaticAppDirFn)();
typedef char*	(*FskGetStaticExternalDirFn)();

typedef void	(*FskGetSpecialPathsFn)(char **musicDir, char **podcastDir, char **picturesDir, char **moviesDir, char **downloadsDir, char **dcimDir);

typedef char*	(*FskGetStaticIMEIFn)();
typedef char*	(*FskGetDeviceUsernameFn)();

typedef void	(*FskSetFBGlobalsFn)(FskFBGlobals fbGlobals);

typedef double	(*FskGetVolumeFn)();
typedef void	(*FskSetVolumeFn)(double val);

typedef int		(*FskGetAppsCookieFn)();
typedef void	(*FskSetKbdHintFn)(char *hint);

typedef void	(*FskWifiSocketFn)();

typedef void	(*FskResetPhoneHomeFn)();

typedef int		(*FskGetRandomFn)(UInt32 *seed);

typedef int		(*FskLogPrintFn)(int prio, const char *tag, const char *fmt, ...);
typedef int		(*FskLogVPrintFn)(int prio, const char *tag, const char *fmt, va_list ap);

typedef FskErr	(*FskWebViewCreateFn)(FskBrowser browser);
typedef void	(*FskWebViewDisposeFn)(FskBrowser browser);
typedef void	(*FskWebViewActivatedFn)(FskBrowser browser, Boolean activeIt);
typedef void	(*FskWebViewSetFrameFn)(FskBrowser browser, FskRectangle area);
typedef void	(*FskWebViewAttachFn)(FskBrowser browser, FskWindow window);
typedef void	(*FskWebViewDetachFn)(FskBrowser browser);
typedef char*	(*FskWebViewGetURLFn)(FskBrowser browser);
typedef void	(*FskWebViewSetURLFn)(FskBrowser browser, char *url);
typedef char*	(*FskWebViewEvaluateScriptFn)(FskBrowser browser, char *script);
typedef void	(*FskWebViewReloadFn)(FskBrowser browser);
typedef void	(*FskWebViewBackFn)(FskBrowser browser);
typedef void	(*FskWebViewForwardFn)(FskBrowser browser);
typedef Boolean (*FskWebViewCanBackFn)(FskBrowser browser);
typedef Boolean (*FskWebViewCanForwardFn)(FskBrowser browser);

typedef char*	(*FskLibraryFetchFn)(const char *kind, const char *option, const char *optionValue);
typedef FskErr	(*FskLibraryThumbnailFn)(const char *kind, long id, Boolean micro, FskBitmap *bitmap);
typedef char*	(*FskLibraryGetSongPathFn)(long id, int index);
typedef FskErr	(*FskLibrarySaveImageFn)(char *data, int size);

typedef void	(*FskSetContinuousDrawingFn)(Boolean continuousDrawing);
typedef void	(*FskGetContinuousDrawingUpdateIntervalFn)(UInt32 *Interval);

typedef struct {
    FskSetWallpaperCBFn					setWallpaperCB;
    FskGetMemoryInfoCBFn				getMemoryInfoCB;

	FskGetModelInfoFn					getModelInfoCB;
	FskGetDPIFn							getDPICB;
	FskGetLanguageFn					getLanguageCB;
	FskGetBasetimeFn					getBasetimeCB;

	FskLaunchDocumentCBFn				launchDocumentCB;
	FskJNIControlFn						jniControlCB;
	FskJNIFetchFn						jniFetchCB;

	FskFetchContactsFn					getContactsCB;

	FskStartContactsFn					startContactsCB;
	FskStopContactsFn					stopContactsCB;
	FskNextContactFn					nextContactCB;
	FskIdxContactFn						idxContactCB;

	FskDialFn							dialCB;
	FskSendSMSFn						smsSendCB;
	FskSendMailFn						mailSendCB;
	FskSendMailAttachmentsFn			attachmentsSendCB;

	FskIsIMEEnabledFn					isIMEEnabledCB;
	FskIMEEnableFn						IMEEnableCB;

	FskFetchAppsFn						fetchAppsCB;
	FskLaunchAppFn						launchAppCB;
	FskFetchAppIconFn					fetchAppIconCB;
	FskGetAppsCookieFn					getAppsCookieCB;

	FskDirChangeNotDisposeFn			dirChangeNotifierDisposeCB;
	FskDirChangeNotNewFn				dirChangeNotifierNewCB;
	FskDirChangeNotTerminateFn			dirChangeNotifierTermCB;

	FskSystemBarHeightFn				systemBarHeightCB;
	FskSystemBarShowFn					systemBarShowCB;

	FskCanHandleIntentFn				canHandleIntentCB;
	FskHandleIntentFn					handleIntentCB;
	FskCanHandleIntentClassFn			canHandleIntentClassCB;
	FskHandleIntentClassFn				handleIntentClassCB;

	FskEnergySaverFn					energySaverCB;

	FskWakeMainFn						wakeMainCB;
	FskDetachThreadFn					detachThreadCB;

	FskAudioInNewFn						audioInNewCB;
	FskAudioInDisposeFn					audioInDisposeCB;
	FskAudioInGetFormatFn				audioInGetFormatCB;
	FskAudioInSetFormatFn				audioInSetFormatCB;
	FskAudioInStartFn					audioInStartCB;
	FskAudioInStopFn					audioInStopCB;

	FskGetLastXYFn						getLastXYCB;
	FskGetScreenSizeFn					getScreenSizeCB;
	FskCheckSizeChangeCompleteFn		checkSizeChangeCompleteCB;
	FskNoWindowDontDrawFn				noWindowDontDrawCB;

	FskTweakKbdFn						tweakKbdCB;
	FskTweakKbdSelectionFn				tweakKbdSelectionCB;
	FskGetKbdRectFn						getKeyboardRectCB;
	FskSetTERectFn						setTERectCB;
	FskSetKbdHintFn						setKeyboardHintCB;

	FskAfterWindowResizeFn				afterWindowResizeCB;
	FskGetMidWindowResizeFn				getMidWindowResizeCB;
	FskSetTransitionStateFn				setTransitionStateCB;

	FskGetStaticExternalDirFn			getStaticExternalDirCB;
	FskGetStaticDataDirFn				getStaticDataDirCB;
	FskGetStaticAppDirFn				getStaticAppDirCB;
	FskGetSpecialPathsFn				getSpecialPathsCB;
	FskGetStaticIMEIFn					getStaticIMEICB;
	FskGetDeviceUsernameFn				getDeviceUsernameCB;

	FskSetFBGlobalsFn					setFBGlobalsCB;

	FskGetVolumeFn						getVolumeCB;
	FskSetVolumeFn						setVolumeCB;

	FskWifiSocketFn						addWifiSocketCB;
	FskWifiSocketFn						removeWifiSocketCB;

	FskResetPhoneHomeFn					resetPhoneHomeCB;

	FskLogPrintFn						logPrintCB;
	FskLogVPrintFn						logVPrintCB;

	FskWebViewCreateFn					webviewCreateCB;
	FskWebViewDisposeFn					webviewDisposeCB;
	FskWebViewActivatedFn				webviewActivatedCB;
	FskWebViewSetFrameFn				webviewSetFrameCB;
	FskWebViewAttachFn					webviewAttachCB;
	FskWebViewDetachFn					webviewDetachCB;
	FskWebViewGetURLFn					webviewGetURLCB;
	FskWebViewSetURLFn					webviewSetURLCB;
	FskWebViewEvaluateScriptFn			webviewEvaluateScriptCB;
	FskWebViewReloadFn					webviewReloadCB;
	FskWebViewBackFn					webviewBackCB;
	FskWebViewForwardFn					webviewForwardCB;
	FskWebViewCanBackFn					webviewCanBackCB;
	FskWebViewCanForwardFn				webviewCanForwardCB;

	FskLibraryFetchFn					libraryFetchCB;
	FskLibraryThumbnailFn				libraryThumbnailCB;
	FskLibraryGetSongPathFn				libraryGetSongPathCB;
	FskLibrarySaveImageFn				librarySaveImageCB;

	FskGetRandomFn						getRandomCB;
	FskSetContinuousDrawingFn			setContinuousDrawingCB;
	FskGetContinuousDrawingUpdateIntervalFn			getContinuousDrawingUpdateIntervalCB;

} FskAndroidCallbacksRecord, *FskAndroidCallbacks;

extern FskAndroidCallbacks gAndroidCallbacks;

FskErr FskSetAndroidCallbacks(FskAndroidCallbacks callbacks);


#endif // ANDROID

#ifdef __cplusplus
}
#endif


#endif // __FSKHARDWARE_H__

