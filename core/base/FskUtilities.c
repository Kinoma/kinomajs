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
#define _WIN32_WINNT 0x0500
#define __FSKTIME_PRIV__
#define __FSKWINDOW_PRIV__
#define __FSKUTILITIES_PRIV__
#define __FSKTHREAD_PRIV__
#define __FSKNETUTILS_PRIV__

#include "FskMemory.h"
#include "FskThread.h"
#include "FskWindow.h"
#include "FskUtilities.h"
#include "FskMain.h"
#include "FskPlatformImplementation.h"
#include "FskTime.h"
#include "FskFiles.h"
#include "FskString.h"
#include "FskTextConvert.h"
#include "FskNetUtils.h"

#if TARGET_OS_ANDROID
#include "FskHardware.h"
#endif

#include <stdlib.h>

#if TARGET_OS_MAC
	#include <mach/mach.h>
	#include <mach/mach_time.h>
#if TARGET_OS_IPHONE
	#include "FskCocoaSupportPhone.h"
#else
	#include "FskCocoaSupport.h"
#endif
#endif /* TARGET_OS_MAC */

#if TARGET_OS_WIN32
	#include "shlobj.h"
	#define COBJMACROS
	#include "ntverp.h"

	#if VER_PRODUCTVERSION_W <= 0x0501
		// the following are not in the Visual Studio .NET (2002) headers, so we include them here
		typedef struct _SHChangeNotifyEntry
		{
			LPCITEMIDLIST pidl;
			BOOL   fRecursive;
		} SHChangeNotifyEntry;

		SHSTDAPI_(BOOL) SHGetPathFromIDListW(LPCITEMIDLIST pidl, LPWSTR pszPath);
	#endif /* VER_PRODUCTVERSION_W <= 0x0501 */

	extern FskErr fixUpPathForWindows(const char *fullPath, UInt16 **winPath);

#endif /* TARGET_OS_WIN32 */

#if TARGET_OS_KPL
	#include "KplUtilities.h"
	#include "KplDevice.h"
	#define __FSKMEMORY_KPL_PRIV__
	#include "KplMemory.h"
#endif

static FskListMutex gNotificationProcs;

// ---------------------------------------------------------------------
// Launcher
FskErr FskLauncherOpenDocument(const char *fullPath, UInt32 kind)
{
#if TARGET_OS_WIN32
	FskErr err = kFskErrNone;
	UInt16 *platformText = NULL;
	SHELLEXECUTEINFOW shellExecuteInfo;

	if (0 == kind)
		err = FskFilePathToNative((char*)fullPath, (char**)&platformText);
	else
		err = FskTextUTF8ToUnicode16LE((const unsigned char *)fullPath, FskStrLen(fullPath), (UInt16 **)&platformText, NULL);
	if (!err) {
		memset(&shellExecuteInfo, 0, sizeof(shellExecuteInfo));
		shellExecuteInfo.cbSize = sizeof(shellExecuteInfo);
		shellExecuteInfo.fMask = 0;
		shellExecuteInfo.lpVerb = L"open";
		shellExecuteInfo.lpFile = (LPCWSTR)platformText;
		shellExecuteInfo.hwnd = GetDesktopWindow();
		shellExecuteInfo.nShow = SW_SHOWNORMAL;
		ShellExecuteExW(&shellExecuteInfo);
		FskMemPtrDispose(platformText);
	}
#elif TARGET_OS_ANDROID
	gAndroidCallbacks->launchDocumentCB(kind, (char*)fullPath);
#elif TARGET_OS_MAC
	FskErr err = kFskErrNone;

	switch (kind)
	{
		case 0:
			if (!FskCocoaOpenFile(fullPath))
				err = kFskErrOperationFailed;
			break;
		case 1:
			if (!FskCocoaOpenURL(fullPath))
				err = kFskErrOperationFailed;
			break;
		default:
			err = kFskErrUnimplemented;
			break;
	}

	return err;
#elif TARGET_OS_KPL
	FskErr err;
	
	switch(kind) {
		case 0:
			err = KplLauncherOpenURI(fullPath);
			break;
		case 1:
			err = KplBrowserOpenURI(fullPath);
			break;
		default:
			err = kFskErrUnimplemented;
			break;
	}
	
	return err;
#else
	// Fill in other targets here.
#endif

	return kFskErrNone;
}

// ---------------------------------------------------------------------
SInt32 FskRandom(void)
{
#if !TARGET_OS_WIN32
	FskThread thread = FskThreadGetCurrent();
#endif
	
#if TARGET_OS_ANDROID
	return gAndroidCallbacks->getRandomCB(&thread->randomSeed);
#elif TARGET_OS_WIN32
	return rand();
#elif TARGET_OS_MAC || TARGET_OS_LINUX
	return rand_r(&thread->randomSeed);
#elif TARGET_OS_KPL
	return KplUtilitiesRandom(&thread->randomSeed);
#else
	#error - FskRandom() needs to be implemented for this platform
#endif
}

void FskRandomInit(void)
{
	FskThread thread = FskThreadGetCurrent();
	UInt32 randomSeed;
#if TARGET_OS_WIN32
	randomSeed = GetTickCount();
#elif TARGET_OS_MAC
	randomSeed = (UInt32)mach_absolute_time();
#elif TARGET_OS_KPL
	randomSeed = KplUtilitiesRandomSeedInit();
#else /* !TARGET_OS_MAC */
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	randomSeed = tv.tv_usec;
#endif /* !TARGET_OS_MAC */
	if (thread)
		thread->randomSeed = randomSeed;
	srand(randomSeed);
}

/*
	Quick sort excitement
*/

void FskQSort(void *base, UInt32 num, UInt32 size, FskCompareFunction compare)
{
	qsort(base, num, size, compare);
}

void *FskBSearch(const void *key, const void *base, UInt32 num, UInt32 width, FskCompareFunction compare)
{
	return bsearch(key, base, num, width, compare);
}


/* Deferrer */

typedef struct FskDeferrerCallbackRecord FskDeferrerCallbackRecord, *FskDeferrerCallback;

static void FskDeferrerTaskDispose(FskDeferrer deferrer, FskDeferrerCallback taskP);
static void FskFeferrerCallback(void *a, void *b, void *c, void *d);

struct FskDeferrerRecord {
	FskDeferrerCallback tasks;
	Boolean disposeRequested;
};

struct FskDeferrerCallbackRecord {
	FskDeferrerCallback next;

	FskDeferredTask task;
	void *arg1;
	void *arg2;
	void *arg3;
	void *arg4;
};

FskErr FskDeferrerNew(FskDeferrer *it)
{
	FskErr err = kFskErrNone;

	err = FskMemPtrNewClear(sizeof(FskDeferrerRecord), it);
	BAIL_IF_ERR(err);

bail:
	return err;
}

FskErr FskDeferrerDispose(FskDeferrer deferrer)
{
	FskErr err = kFskErrNone;
	if (deferrer) {
		Boolean hasTasks = (deferrer->tasks != NULL);

		while (deferrer->tasks) {
			FskDeferrerTaskDispose(deferrer, deferrer->tasks);
		}

		if (hasTasks) {
			deferrer->disposeRequested = true;
		} else {
			FskMemPtrDispose(deferrer);
		}
	}
	return err;
}

void *FskDeferrerAddTask(FskDeferrer deferrer, FskDeferredTask task, void *arg1, void *arg2, void *arg3, void *arg4)
{
	FskErr err = kFskErrNone;
	FskDeferrerCallback taskP = NULL;

	err = FskMemPtrNewClear(sizeof(FskDeferrerCallbackRecord), &taskP);
	BAIL_IF_ERR(err);

	taskP->task = task;
	taskP->arg1 = arg1;
	taskP->arg2 = arg2;
	taskP->arg3 = arg3;
	taskP->arg4 = arg4;

	if (deferrer->tasks == NULL) {
		FskThreadPostCallback(FskThreadGetCurrent(), FskFeferrerCallback, deferrer, NULL, NULL, NULL);
	}
	FskListAppend(&deferrer->tasks, taskP);

bail:
	return (void *) taskP;
}

void FskDeferrerRemoveTask(FskDeferrer deferrer, void *taskP)
{
	if (FskListContains(deferrer->tasks, taskP)) {
		FskDeferrerTaskDispose(deferrer, taskP);
	}
}

static void FskDeferrerTaskDispose(FskDeferrer deferrer, FskDeferrerCallback taskP)
{
	FskListRemove(&deferrer->tasks, taskP);
	FskMemPtrDispose(taskP);
}

static void FskFeferrerCallback(void *a, void *b, void *c, void *d)
{
	FskDeferrer deferrer = (FskDeferrer) a;
	while (deferrer->tasks) {
		FskDeferrerCallback taskP = deferrer->tasks;

		taskP->task(taskP->arg1, taskP->arg2, taskP->arg3, taskP->arg4);
		FskDeferrerTaskDispose(deferrer, taskP);
	}

	if (deferrer->disposeRequested) {
		FskMemPtrDispose(deferrer);
	}
}

void FskDelay(UInt32 ms)
{
#if TARGET_OS_WIN32
	Sleep(ms);
#elif TARGET_OS_MAC || TARGET_OS_LINUX
	if (ms > (0x7fffffff / 1000))
		sleep(ms / 1000);
	else
		usleep(ms * 1000);
#elif TARGET_OS_KPL
	KplUtilitiesDelay(ms);
#endif
}

static char **gArgv;
static int gArgc;

void FskUtilsSetArgs(int argc, char **argv)
{
	gArgc = argc;
	gArgv = argv;
}

void FskUtilsGetArgs(int *argc, char ***argv)
{
	*argc = gArgc;
	*argv = gArgv;
}

char *FskUtilsGetFileArgs(UInt32 *fileListSizeOut)
{
	unsigned char *fileList = NULL;
	UInt32 fileListSize = 0;
	int i;

	for (i = 1; i < gArgc; i++) {
		unsigned char *arg = (unsigned char*)gArgv[i];
		UInt32 argLen = FskStrLen((char*)arg) + 1;
		if (('-' == arg[0]) && ('-' == arg[1]))
			continue;

#if TARGET_OS_MAC
		if (('-' == arg[0]) && ('p' == arg[1]) && ('s' == arg[2]) && ('n' == arg[3]))
			continue;
		if (0 == FskStrCompare((char*)arg, "-NSDocumentRevisionsDebugMode")) {
            i += 1;
            continue;
        }
#endif /* TARGET_OS_MAC */

		if (0 == FskStrCompare((char*)arg, "-config")) {
			i += 1;
			continue;
		}

		if (kFskErrNone != FskMemPtrRealloc(fileListSize + argLen + 1, (FskMemPtr *)&fileList))
			break;

		FskMemMove(fileList + fileListSize, arg, argLen);

#if TARGET_OS_WIN32
		while (true) {
			char *slash = FskStrChr((const char *)fileList + fileListSize, '\\');
			if (NULL == slash)
				break;
			*slash = '/';
		}
#endif /* TARGET_OS_WIN32 */

		fileListSize += argLen;
		fileList[fileListSize] = 0;
	}

	*fileListSizeOut = fileListSize + 1;

	return (char*)fileList;
}


static FskErr FskUtilsCommonInit(void)
{
	FskErr err;

	err = FskTimeInitialize();
	if (err) return err;

	FskRandomInit();

	return err;
}

static FskErr FskUtilsCommonTerm(void)
{
	FskListMutexDispose(gNotificationProcs);
	gNotificationProcs = NULL;

	return FskTimeTerminate();
}

/*
	Start-up, tear-down
*/

#if TARGET_OS_WIN32

HWND gUtilsWnd;
Boolean gEmptyingClipboard;
static long FAR PASCAL UtilsWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam);
static FskErr stopService(const char* name, DWORD timeout);

FskErr FskHardwareInitialize(void) {
	return kFskErrNone;
}

FskErr FskHardwareTerminate(void) {
	return kFskErrNone;
}

FskErr FskUtilsEnergySaverUpdate(UInt32 mask, UInt32 value)
{
	return kFskErrNone;
}


OSVERSIONINFOEX gWindowsVersionInfo;

FskErr FskUtilsInitialize(void)
{
	WNDCLASS wc;

	SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);		// don't pop-up system dialogs because of write attempts to locked disks, etc.
	CoInitialize(NULL);

	ZeroMemory(&gWindowsVersionInfo, sizeof(OSVERSIONINFOEX));
	gWindowsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((LPOSVERSIONINFOA)&gWindowsVersionInfo);

	if ((VER_PLATFORM_WIN32_NT != gWindowsVersionInfo.dwPlatformId) ||
		(gWindowsVersionInfo.dwMajorVersion < 5))
		return kFskErrUnimplemented;

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = UtilsWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "projectf-utils";
	RegisterClass(&wc);

	FskUtilsCommonInit();
	gUtilsWnd = CreateWindow("projectf-utils", NULL,
		WS_DISABLED,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInst,
		NULL);

	gThreadEventMessage = RegisterWindowMessage("FskThreadEvent");

	FskThreadCreateMain(NULL);

	return kFskErrNone;
}

void FskUtilsTerminate(void)
{
	if (NULL != gUtilsWnd) {
		DestroyWindow(gUtilsWnd);
		gUtilsWnd = NULL;
	}

	FskUtilsCommonTerm();		// timer depends on threads, so dispose first

	FskThreadTerminateMain();

	CoUninitialize();
}


long FAR PASCAL UtilsWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
	static UINT gFskOpenFiles = 0x7FFF;
	static UINT gFskShellExecute = 0x7FFF;
	static UINT gCDReadErrorMsg = 0x7FFF;
	static UINT gFskDeviceChange= 0x7FFF;

	switch (msg) {
		case WM_CREATE:
			gFskOpenFiles = RegisterWindowMessageW(L"FskOpenFiles");
			gFskShellExecute = RegisterWindowMessageW(L"FskShellExecute");
			gCDReadErrorMsg = RegisterWindowMessage("FskCdReadErr");
			gFskDeviceChange = RegisterWindowMessage("FskDeviceChange");
			break;
		case WM_TIMER:
			FskTimeCallbackService(FskThreadGetCurrent());
			break;
//		case WM_DEVICECHANGE:
//			volumeListChanged();
//			break;

		case WM_DESTROYCLIPBOARD:
			{
				FskWindow win = FskWindowGetActive();
				if ((NULL != win) && (0 == gEmptyingClipboard)) {
					FskEvent fskEvent;

					if (kFskErrNone == FskEventNew(&fskEvent, kFskEventClipboardChanged, NULL, kFskEventModifierNotSet))
						FskWindowEventSend(win, fskEvent);
				}
			}
			break;

		default:
			if (gCDReadErrorMsg == msg) {
				volumeListChanged(NULL, 0);
				break;
			}
			if (gFskDeviceChange == msg) {
				Boolean add = (lParam & SHCNE_DRIVEADD) || (SHCNE_MEDIAINSERTED & lParam) || (SHCNE_NETSHARE & lParam);
				SHChangeNotifyEntry *shns = (SHChangeNotifyEntry *)wParam;
				WCHAR sPath[MAX_PATH];

				if (SHGetPathFromIDListW(shns->pidl, sPath))
					volumeListChanged(sPath, add ? 1 : 2);
				break;
			}
			if (gFskShellExecute == msg) {
				ShellExecuteW(0, L"open", (LPCWSTR)lParam, NULL, NULL, SW_SHOW);
				break;
			}
			if (gFskOpenFiles == msg) {
				FskWindow w = FskWindowGetInd(0, NULL);
				if (NULL != w) {
					FskEvent		event;
					UInt32 fileListSize = 1;
					HANDLE hFileMap;
					char numStr[64];
					char *fileName;

					FskStrNumToStr(lParam, numStr, sizeof(numStr));
					fileName = FskStrDoCat("FskFileOpen-", numStr);

					hFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, fileName);

					if ((NULL != hFileMap) && (INVALID_HANDLE_VALUE != hFileMap)) {
						char *fileList = (char *)MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
						if (NULL != fileList) {
							char *p = fileList;

							while (*p) {
								UInt32 pLen = FskStrLen(p) + 1;
								fileListSize += pLen;
								p += pLen;
							}

							if (kFskErrNone == FskEventNew(&event, kFskEventWindowOpenFiles, NULL, kFskEventModifierNotSet)) {
								if (kFskErrNone == FskEventParameterAdd(event, kFskEventParameterFileList, fileListSize, (char *)fileList))
									FskWindowEventQueue(w, event);
							}

							UnmapViewOfFile(fileList);
						}
					   CloseHandle(hFileMap);
					}
					FskMemPtrDispose(fileName);
				}
				break;
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

#if 0
FskErr stopService(const char* name, DWORD timeout)
{
	SERVICE_STATUS ss;
	SC_HANDLE hSCM = NULL;
	SC_HANDLE hService = NULL;
	UInt32 startTime = GetTickCount();
	FskErr err = kFskErrUnknown;

	hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!hSCM)
		goto bail;

	hService = OpenService(hSCM, name, SERVICE_STOP | SERVICE_QUERY_STATUS);
	if (!hService)
		goto bail;

	// Make sure the service is not already stopped
	if (!QueryServiceStatus(hService, &ss))
		goto bail;

	if (ss.dwCurrentState == SERVICE_STOPPED) 
		goto bail;

	// If a stop is pending, just wait for it
	while (ss.dwCurrentState == SERVICE_STOP_PENDING) {
		Sleep( ss.dwWaitHint );
		if (!QueryServiceStatus( hService, &ss ))
			goto bail;

		if (ss.dwCurrentState == SERVICE_STOPPED) {
			BAIL(kFskErrNone);
		}

		if (GetTickCount() - startTime > timeout) {
			goto bail;
		}
	}

	// Send a stop code to the main service
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &ss))
		goto bail;

	// Wait for the service to stop
	while (ss.dwCurrentState != SERVICE_STOPPED) {
		Sleep( ss.dwWaitHint );
		if (!QueryServiceStatus(hService, &ss))
			goto bail;

		if (ss.dwCurrentState == SERVICE_STOPPED)
			break;

		if (GetTickCount() - startTime > timeout) {
			goto bail;
		}	  
	}
	err = kFskErrNone;
bail:
	if (hService) 
		CloseServiceHandle(hService);
	if (hSCM)
		CloseServiceHandle(hSCM);
	return err;
}
#endif /* 0 */
#endif

#if TARGET_OS_MAC

#include <signal.h>

static Boolean gIsMacOSX;

FskErr FskHardwareInitialize(void) {
	return kFskErrNone;
}

FskErr FskHardwareTerminate(void) {
	return kFskErrNone;
}

#if TARGET_OS_IPHONE
static UInt32 sEnergySaverScreenDimming = 0;
#endif

FskErr FskUtilsEnergySaverUpdate(UInt32 mask, UInt32 value)
{
#if TARGET_OS_IPHONE
	Boolean lastEnergySaverScreenDimming = (sEnergySaverScreenDimming != 0);
    Boolean nextEnergySaverScreenDimming;

	if (mask & kFskUtilsEnergySaverDisableScreenDimming) {
		if (value & kFskUtilsEnergySaverDisableScreenDimming)
			sEnergySaverScreenDimming++;
		else if (sEnergySaverScreenDimming)
			sEnergySaverScreenDimming--;
	}
    nextEnergySaverScreenDimming = (sEnergySaverScreenDimming != 0);

	if (nextEnergySaverScreenDimming != lastEnergySaverScreenDimming) {
        // Disable idle timer only if dimming will be off.
        // kpsMedia trys to set backlight on when video media will be played.
        // Audio media can be played while the device is in sleep state.
        FskCocoaApplicationSetIdleTimerDisabled(nextEnergySaverScreenDimming);
	}
#endif

	return kFskErrNone;
}

static void sInvokeCleanup(int whatever) {
	FskMainDoQuit(kFskErrNone);
}


FskErr FskUtilsInitialize(void)
{
	FskUtilsCommonInit();

	// make sure we know what we're running on
#if TARGET_OS_IPHONE
	gIsMacOSX = false;
#else /* !TARGET_OS_IPHONE */
	gIsMacOSX = true;	// no longer support OS9
#endif /* !TARGET_OS_IPHONE */
	
	// set up our app handlers
	{	
	struct sigaction action;
	FskMemSet(&action, 0, sizeof(action));
	action.sa_handler = SIG_IGN;
	action.sa_flags = 0;
	sigaction(SIGPIPE, &action, NULL);
	sigaction(SIGHUP, &action, NULL);
	action.sa_handler = sInvokeCleanup;
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGQUIT, &action, NULL);
	}

	FskThreadCreateMain(NULL);

	return kFskErrNone;
}

void FskUtilsTerminate(void)
{
	FskThreadTerminateMain();
	FskUtilsCommonTerm();
}

Boolean FskIsMacOSX(void)
{
	return gIsMacOSX;
}

#elif TARGET_OS_LINUX
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

FskErr FskUtilsEnergySaverUpdate(UInt32 mask, UInt32 value)
{
#if TARGET_OS_ANDROID
	gAndroidCallbacks->energySaverCB(mask, value);
#else

	static UInt32 sDisableDim = 0;
	static UInt32 sDisableSleep = 0;
	static Boolean screenIsHeldOn = false;

	if (mask & kFskUtilsEnergySaverDisableScreenDimming)
	{
		if (value & kFskUtilsEnergySaverDisableScreenDimming)
			sDisableDim++;
		else if (sDisableDim)
			sDisableDim--;
	}

	if (mask & kFskUtilsEnergySaverDisableSleep)
	{
		if (value & kFskUtilsEnergySaverDisableSleep)
			sDisableSleep++;
		else if (sDisableSleep)
			sDisableSleep--;
	}

	if (sDisableDim) {
		if (!screenIsHeldOn) {
			screenIsHeldOn = true;
			(*sDontDimCB)();
		}
	}
	else {
		if (screenIsHeldOn) {
			screenIsHeldOn = false;
			(*sNormalDimCB)();
		}
	}

	if (sDisableSleep) {
	}
#endif
	return kFskErrNone;
}

static void sInvokeCleanup(int whatever) {
	FskMainDoQuit(kFskErrNone);
}


FskErr FskUtilsInitialize(void)
{
	struct sigaction action;
	FskMemSet(&action, 0, sizeof(action));
	action.sa_handler = SIG_IGN;
	action.sa_flags = 0;
	sigaction(SIGPIPE, &action, NULL);
	sigaction(SIGHUP, &action, NULL);
	action.sa_handler = sInvokeCleanup;
	sigaction(SIGINT, &action, NULL);
#if ! TARGET_OS_ANDROID
	sigaction(SIGQUIT, &action, NULL);
#endif

	FskThreadCreateMain(NULL);

	FskUtilsCommonInit();
	return kFskErrNone;
}

void FskUtilsTerminate(void)
{
	FskThreadTerminateMain();
	FskUtilsCommonTerm();
}

UInt32 FskUtilsGetKeyboardType(void)
{
#if TARGET_OS_ANDROID
	return gFskPhoneHWInfo->keyboardType;
#else
	return kFskKeyboardTypePhone12Keys;
#endif
}

#elif TARGET_OS_KPL

static UInt32 sEnergySaverScreenDimming = 0;
static UInt32 sEnergySaverSleep = 0;

FskErr FskUtilsEnergySaverUpdate(UInt32 mask, UInt32 value)
{
	UInt32 lastEnergySaverScreenDimming = sEnergySaverScreenDimming;
	UInt32 lastEnergySaverSleep = sEnergySaverSleep;
	Boolean doUpdate = false;
	UInt32 flags = 0;
	FskErr err = kFskErrNone;
	
	if (mask & kFskUtilsEnergySaverDisableScreenDimming) {
		if (value & kFskUtilsEnergySaverDisableScreenDimming)
			sEnergySaverScreenDimming++;
		else if (sEnergySaverScreenDimming)
			sEnergySaverScreenDimming--;
	}

	if (mask & kFskUtilsEnergySaverDisableSleep) {
		if (value & kFskUtilsEnergySaverDisableSleep)
			sEnergySaverSleep++;
		else if (sEnergySaverSleep)
			sEnergySaverSleep--;
	}
	
	if (lastEnergySaverSleep != sEnergySaverSleep) {
		if (sEnergySaverSleep)
			flags |= kFskUtilsEnergySaverDisableSleep;
		doUpdate = true;
	}

	if (lastEnergySaverScreenDimming != sEnergySaverScreenDimming) {
		if (sEnergySaverScreenDimming)
			flags |= kFskUtilsEnergySaverDisableScreenDimming;
		doUpdate = true;
	}

	if (doUpdate) {
		KplPropertyRecord property;
		property.integer = flags;
		err = KplDeviceSetProperty(kKplPropertyDevicePowerFlags, &property);
	}
	
	return err;
}

FskErr FskHardwareInitialize()
{
	return KplUtilitiesHardwareInitialize();
}

FskErr FskHardwareTerminate()
{
	return KplUtilitiesHardwareTerminate();
}

FskErr FskUtilsInitialize(void)
{
	KplUtilitiesInitialize();
	FskUtilsCommonInit();
	FskThreadCreateMain(NULL);
	return kFskErrNone;
}

void FskUtilsTerminate(void)
{
	FskThreadTerminateMain();
	FskUtilsCommonTerm();
	KplUtilitiesTerminate();
}
#endif

void FskExit(SInt32 result)
{
	// -- maybe set the runloop flag to false here...

	exit(result);
}

/*
	System fun
*/

char *FskGetApplicationPath(void)
{
#if TARGET_OS_LINUX
	char *p;

#if TARGET_OS_ANDROID
{
	p = FskStrDoCat(gAndroidCallbacks->getStaticAppDirCB(), "/");
	return p;
}
#else
	char *name, *ret, canonPath[PATH_MAX];
	p = FskStrDoCopy(gArgv[0]);
	name = FskStrRChr(p, '/');
	if (!name) {
		ret = FskStrDoCopy("./");
		FskMemPtrDispose(p);
	}
	else {
		*++name = '\0';
		ret = p;
	}
	realpath(ret, canonPath);
	FskMemPtrDispose(ret);
	ret = FskStrDoCat(canonPath, "/");
	return ret;
#endif
#elif TARGET_OS_WIN32
	UInt16 name[MAX_PATH];
	UInt16 *c = name, *lastSlash = NULL;
	char *result;

	GetModuleFileNameW(NULL, (LPWCH)name, MAX_PATH * 2);

	// canonify
	while (*c) {
		if ('\\' == *c) {
			*c = '/';
			lastSlash = c;
		}
		c++;
	}

	// strip binary name
	if (lastSlash)
		lastSlash[1] = 0;

	FskTextUnicode16LEToUTF8(name, (FskUnicodeStrLen(name) + 1) * 2, &result, NULL);

	return result;
#elif TARGET_OS_MAC
	CFURLRef aURL = NULL;
	char aBuffer[1024];
#if !TARGET_OS_IPHONE && !FSK_EMBED
	char* aSlash;
#endif

	aURL = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	if (aURL) {
		CFURLGetFileSystemRepresentation(aURL, true, (unsigned char *)aBuffer, 1024); 
		CFRelease(aURL);
#if !TARGET_OS_IPHONE
		if (0 == FskStrCompare(aBuffer + FskStrLen(aBuffer) - 4, ".app")) {
#if FSK_EMBED
			FskStrCat(aBuffer, "/Contents/Resources/");
#else
			aSlash = FskStrRChr(aBuffer, '/');
			if (aSlash)
				*(aSlash + 1) = 0;
#endif			
		}
		else
#endif
			FskStrCat(aBuffer, "/");
		return FskStrDoCopy(aBuffer);
	}
	else
		return NULL;
#elif TARGET_OS_KPL
	return KplUtilitiesGetApplicationPath();
#else
	#error fill this in
#endif
}


/*
    Notifications
*/

typedef struct {
    void                    *next;
    FskNotification         kind;
    FskThread               thread;
    FskNotificationProc     proc;
    void                    *refcon;
} FskNotificationItemRecord, *FskNotificationItem;

static void doNotification(void *a0, void *a1, void *a2, void *a3);

void FskNotificationRegister(FskNotification kind, FskNotificationProc proc, void *refcon)
{
    FskNotificationItem item;

    if (NULL == gNotificationProcs) {
        if (kFskErrNone != FskListMutexNew(&gNotificationProcs, "notification procs"))
            return;
#if TARGET_OS_KPL
		KplNotificationInitialize();
#endif
    }

    if (kFskErrNone != FskMemPtrNewClear(sizeof(FskNotificationItemRecord), &item))
        return;

    item->proc = proc;
    item->refcon = refcon;
    item->kind = kind;
    item->thread = FskThreadGetCurrent();
    FskListMutexAppend(gNotificationProcs, item);
}

void FskNotificationUnregister(FskNotification kind, FskNotificationProc proc, void *refcon)
{
    FskNotificationItem item;
    
    for (item = FskListMutexGetNext(gNotificationProcs, NULL); NULL != item; item = FskListMutexGetNext(gNotificationProcs, item)) {
        if ((proc == item->proc) && (refcon == item->refcon) && (kind = item->kind)) {
            FskListMutexRemove(gNotificationProcs, item);
            FskMemPtrDispose(item);
            break;
        }
    }
}

void FskNotificationPost(FskNotification kind)
{
    FskThread thread = FskThreadGetCurrent();
    FskNotificationItem item;

    for (item = FskListMutexGetNext(gNotificationProcs, NULL); NULL != item; item = FskListMutexGetNext(gNotificationProcs, item)) {
        if (item->kind == kind) {
            if (thread == item->thread)
                (item->proc)(item->refcon);
            else
                FskThreadPostCallback(item->thread, doNotification, item, NULL, NULL, NULL);
        }
    }
}

void doNotification(void *a0, void *a1, void *a2, void *a3)
{
    FskNotificationItem item = a0;
    (item->proc)(item->refcon);
}
