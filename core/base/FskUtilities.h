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
#ifndef __FSKUTILITIES__
#define __FSKUTILITIES__

#include "FskMemory.h"
#include "FskString.h"

#if TARGET_OS_WIN32
	#include "windows.h"
#elif TARGET_OS_LINUX || TARGET_OS_MAC
	#include <pthread.h>
#endif /* TARGET_OS */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


FskAPI(SInt32) FskRandom(void);

#ifdef __FSKUTILITIES_PRIV__
	void FskRandomInit(void);
	FskAPI(char *) FskUtilsGetFileArgs(UInt32 *fileListSize);
#endif /* __FSKUTILITIES_PRIV__ */

// Debug
#define FskDebugStr(...) FskInstrumentedSystemPrintf(__VA_ARGS__)

// Sort
typedef int (*FskCompareFunction)(const void *, const void *);

FskAPI(void) FskQSort(void *base, UInt32 num, UInt32 size, FskCompareFunction compare);
FskAPI(void) *FskBSearch(const void *key, const void *base, UInt32 num, UInt32 width, FskCompareFunction compare);


// Launcher
FskAPI(FskErr) FskLauncherOpenDocument(const char *fullpath, UInt32 kind);		// 0 == file path, 1 == URL

// Deferrer
typedef struct FskDeferrerRecord FskDeferrerRecord, *FskDeferrer;
typedef void (*FskDeferredTask)(void *a, void *b, void *c, void *d);

FskAPI(FskErr) FskDeferrerNew(FskDeferrer *it);
FskAPI(FskErr) FskDeferrerDispose(FskDeferrer deferrer);
FskAPI(void *) FskDeferrerAddTask(FskDeferrer deferrer, FskDeferredTask task, void *arg1, void *arg2, void *arg3, void *arg4);
FskAPI(void) FskDeferrerRemoveTask(FskDeferrer deferrer, void *taskP);

#define FskDeferrerAddTask0(d, t) FskDeferrerAddTask(d, t, NULL, NULL, NULL, NULL)
#define FskDeferrerAddTask1(d, t, a) FskDeferrerAddTask(d, t, a, NULL, NULL, NULL)
#define FskDeferrerAddTask2(d, t, a, b) FskDeferrerAddTask(d, t, a, b, NULL, NULL)
#define FskDeferrerAddTask3(d, t, a, b, c) FskDeferrerAddTask(d, t, a, b, c, NULL)

FskAPI(void) FskDelay(UInt32 ms);

// Global utils
FskAPI(FskErr) FskUtilsInitialize(void);
FskAPI(void) FskUtilsTerminate(void);

FskAPI(void) FskExit(SInt32 result);

FskAPI(void) FskUtilsSetArgs(int argc, char **argv);
FskAPI(void) FskUtilsGetArgs(int *argc, char ***argv);
FskAPI(char *)FskGetApplicationPath(void);

// Hardware
FskAPI(FskErr) FskHardwareInitialize(void);
FskAPI(FskErr) FskHardwareTerminate(void);

// Energy Saver
enum
{
	kFskUtilsEnergySaverDisableSleep			= (1L << 0),
	kFskUtilsEnergySaverDisableScreenDimming	= (1L << 1),
	kFskUtilsEnergySaverDisableScreenSleep	    = (1L << 2)
};

FskAPI(FskErr) FskUtilsEnergySaverUpdate(UInt32 mask, UInt32 value);

// keyboard

enum {
	kFskKeyboardTypeAlphanumeric = 0x100,
	kFskKeyboardTypePhone12Keys = 0x200,
	kFskKeyboardTypeAlphaAndPhone12Keys = 0x400,		//@@ could make this 0x300 - combination of alpha + 12
	kFskKeyboardTypeVirtual = 0x800,

	kFskKeyboardSubTypePalmThumb = 1,
	kFskKeyboardSubTypeSamsungMITSThumb = 2,
	kFskKeyboardSubTypeDopodWizardThumb = 3,
	kFskKeyboardSubTypeHP691xThumb = 4,
	kFskKeyboardSubTypeHermesThumb = 5,
	kFskKeyboardSubTypeXperia = 6,

	kFskKeyboardTypeMask = 0x00ff00,
	kFskKeyboardSubTypeMask = 0x0000ff
};

#if TARGET_OS_LINUX
	FskAPI(UInt32) FskUtilsGetKeyboardType(void);
#elif TARGET_OS_IPHONE
    #define FskUtilsGetKeyboardType() (kFskKeyboardTypeVirtual)
#else /* TARGET_OS */
	#define FskUtilsGetKeyboardType() (kFskKeyboardTypeAlphanumeric)
#endif /* TARGET_OS */

/*
    Notifications
*/

typedef enum {
    kFskNotificationLowMemory = 1,
    kFskNotificationGLContextAboutToLose,
    kFskNotificationGLContextLost
} FskNotification;

typedef void (*FskNotificationProc)(void *refcon);

FskAPI(void) FskNotificationRegister(FskNotification kind, FskNotificationProc proc, void *refcon);
FskAPI(void) FskNotificationUnregister(FskNotification kind, FskNotificationProc proc, void *refcon);
FskAPI(void) FskNotificationPost(FskNotification kind);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKUTILITIES__ */

