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
#define __FSKECMASCRIPT_PRIV__
#define __FSKEXTENSIONS_PRIV__
#define __FSKTEXT_PRIV__
#define __FSKUUID_PRIV__

#include "FskMain.h"

#include "Fsk.h"
#include "FskConsole.h"
#include "FskHTTPClient.h"
#include "FskEnvironment.h"

#include "FskAudio.h"
#include "FskBlit.h"
#include "FskECMAScript.h"
#include "FskFiles.h"
#include "FskExtensions.h"
#include "FskImage.h"
#include "FskNetInterface.h"
#include "FskText.h"
#include "FskUUID.h"
#include "FskWindow.h"

#ifndef SUPPORT_SPLASH
	#define SUPPORT_SPLASH 0
#endif

#if TARGET_OS_WIN32 && SUPPORT_SPLASH
	void FskSplashShow(void);
	void FskSplashHide(void);
#else
	#define FskSplashShow()
	#define FskSplashHide()
#endif

#if TARGET_OS_MAC
    #if TARGET_OS_IPHONE
        #include "FskCocoaSupportPhone.h"
    #else
        #include "FskCocoaSupport.h"
    #endif
#endif

#if TARGET_OS_ANDROID || TARGET_OS_WIN32
	#if FSK_EMBED && FSK_ZIP
		extern FskErr fsZip_fskLoad(FskLibrary library);
		extern FskErr FskZipOpen(void **zipOut, const char *path, UInt32 permissions);
		extern void FskZipClose(void *zip);

		static void *sZipFile = NULL;
	#endif
#endif

#if TARGET_OS_ANDROID || TARGET_OS_MAC
static void checkTimeout() {
	time_t	now, later;
	struct tm timeoutTime;

#define DO_TIMEOUT 0
#if TARGET_OS_ANDROID
	#undef DO_TIMEOUT
	#define DO_TIMEOUT 0
#endif
#if !DO_TIMEOUT
	return;
#endif

	FskMemSet(&timeoutTime, 0, sizeof(struct tm));
	timeoutTime.tm_year = 2011;
	timeoutTime.tm_mon = 9;
	timeoutTime.tm_mday = 15;

	timeoutTime.tm_mon -= 1;				// month is 0 based
	timeoutTime.tm_year -= 1900;
	timeoutTime.tm_sec = 0;
	timeoutTime.tm_min = 0;
	timeoutTime.tm_hour = 0;
	timeoutTime.tm_wday = 0;
	timeoutTime.tm_yday = 0;
	timeoutTime.tm_isdst = 0;

	fprintf(stderr, "Timeout set for %4d-%02d-%02d\n", timeoutTime.tm_year + 1900, timeoutTime.tm_mon + 1, timeoutTime.tm_mday);

	later = mktime(&timeoutTime);
	now = time(NULL);

	fprintf(stderr, "later: %d now: %d\n", (int)later, (int)now);

	if (later < now) {
		fprintf(stderr, "later: %d < now: %d timeout!\n", (int)later, (int)now);
		exit(5);	
	}
}		
#endif

static UInt32 gInitializationFlags;
Boolean gQuitting = false;
FskErr gExitCode = kFskErrNone;
static FskThreadEventHandler gQuitHandler;

FskErr FskMainInitialize(UInt32 flags, int argc, char **argv)
{
	FskErr err = kFskErrNone;

	gInitializationFlags = flags;

	FskSplashShow();

	FskUtilsSetArgs(argc, argv);
	
	FskInstrumentationInitialize();

#if TARGET_OS_ANDROID || TARGET_OS_MAC
	checkTimeout();
#endif

	err = FskHardwareInitialize();
	BAIL_IF_ERR(err);

	FskMemoryInitialize();

	if (kFskMainNetwork & gInitializationFlags) {
		err = FskNetInitialize();
		BAIL_IF_ERR(err);
	}
	err = FskUtilsInitialize();
	BAIL_IF_ERR(err);

	if (kFskMainNetwork & gInitializationFlags) {
		err = FskNetInterfaceInitialize();
		BAIL_IF_ERR(err);
	}

	err = FskEnvironmentInitialize();
	BAIL_IF_ERR(err);

	err = FskExtensionsInitialize();
	BAIL_IF_ERR(err);

	err = FskFileInitialize();
	BAIL_IF_ERR(err);

	if (kFskMainNetwork & gInitializationFlags) {
		err = FskHTTPClientInitialize();
		BAIL_IF_ERR(err);
	}

	FskImageCodecInitialize();
	FskAudioCodecInitialize();
	FskBlitInitialize();
	FskWindowInitialize();
	FskTextInitialize();

	gQuitHandler = FskThreadAddEventHandler(kFskEventSystemQuitRequest, (FskThreadEventHandlerRoutine)FskThreadDefaultQuitHandler, &gQuitting);

	if (!(kFskMainNoECMAScript & gInitializationFlags)) {
#if FSK_EMBED && FSK_ZIP
		char *rootPath, *zipPath;

#if TARGET_OS_ANDROID
		rootPath = FskEnvironmentDoApply(FskStrDoCopy("[applicationPath]res/raw/"));
		zipPath = FskStrDoCat(rootPath, "kinoma.jet");
#else
		rootPath = FskEnvironmentDoApply(FskStrDoCopy("[applicationPath]"));
		zipPath = FskStrDoCat(rootPath, "kinoma.zip");
#endif

		fsZip_fskLoad(NULL);

		err = FskZipOpen(&sZipFile, zipPath, kFskFilePermissionReadOnly);
		BAIL_IF_ERR(err);

#if TARGET_OS_ANDROID
		err = FskECMAScriptInitialize("res/raw/kinoma.jet/kconfig.xml");
#else
		err = FskECMAScriptInitialize("kinoma.zip/kconfig.xml");
#endif

		FskMemPtrDispose(zipPath);
		FskMemPtrDispose(rootPath);
#else
		err = FskECMAScriptInitialize("kconfig.xml");		// root/shared vm ("system")
#endif
		BAIL_IF_ERR(err);

		err = FskECMAScriptInitializationComplete();		// aliased vms ("applications")
		BAIL_IF_ERR(err);
	}

bail:
	FskSplashHide();

	return err;
}

FskErr FskMainTerminate(void)
{
	if (!(kFskMainNoECMAScript & gInitializationFlags)){
#if FSK_EMBED && FSK_ZIP
		FskZipClose(sZipFile);
#endif

		FskECMAScriptTerminate();
	}

	FskThreadRemoveEventHandler(gQuitHandler);

	FskTextTerminate();
	FskWindowTerminate();
	FskImageCodecTerminate();

//	FskBlitTerminate();

	FskFileTerminate();

	if (kFskMainNetwork & gInitializationFlags) {
		FskHTTPClientTerminate();
	}

	FskExtensionsTerminate();

	FskEnvironmentTerminate();

	FskUUIDTerminate();

	if (kFskMainNetwork & gInitializationFlags) {
		FskNetInterfaceTerminate();
	}

	FskUtilsTerminate();

	if (kFskMainNetwork & gInitializationFlags) {
		FskNetTerminate();
	}

	FskMemoryTerminate();
	
	FskHardwareTerminate();

	FskInstrumentationTerminate();			// all instrumented objects should be gone at this point. dump the table to prove that.

	FskConsoleDestroy();

	return kFskErrNone;
}

FskErr FskMainDoQuit(FskErr exitCode)
{
	FskEvent event;
	FskErr err;
	Boolean wasQuitting = gQuitting;

#if TARGET_OS_WIN32 || (TARGET_OS_KPL && defined(_MSC_VER))
	extern DWORD gMainThreadID;
	gQuitting = true;
	PostThreadMessage(gMainThreadID, WM_QUIT, 0, 0);
#elif TARGET_OS_KPL
	gQuitting = true;
#elif TARGET_OS_MAC
	gQuitting = true;
    FskCocoaApplicationStop();
#endif

	gExitCode = exitCode;

	if (false == wasQuitting) {
 		err = FskEventNew(&event, kFskEventSystemQuitRequest, NULL, 0);
		if (event && !err) {
			FskThreadBroadcastEvent(event);
			FskEventDispose(event);
		}
	}

	return kFskErrNone;
}

#if TARGET_OS_MAC

FskErr FskMainApplicationLoop(void)
{
    while (!gQuitting)
        FskThreadRunloopCycle(0);

	return gExitCode;
}

#else

FskErr FskMainApplicationLoop(void)
{
	while (!gQuitting)
		FskThreadRunloopCycle(-1);

	return gExitCode;
}

#endif

/*
	Platform utilities
*/

#if TARGET_OS_WIN32 || (TARGET_OS_KPL && defined(_MSC_VER))

HINSTANCE hInst;
DWORD gMainThreadID;

void FskMainSetHInstance(HINSTANCE hInstance)
{
	hInst = hInstance;
	gMainThreadID = GetCurrentThreadId();
}

HINSTANCE FskMainGetHInstance(void)
{
	return hInst;
}

#endif

#if TARGET_OS_WIN32

static HWND gSplash;
static HBITMAP gSplashBMP;
static int gSplashW = 0, gSplashH = 0;

#define kSplashClass ("kinoma-splash-class")

static DWORD WINAPI splashProc(void *refcon);
static LRESULT CALLBACK SplashWndProc(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam);


#if TARGET_OS_WIN32 && SUPPORT_SPLASH
void FskSplashShow(void)
{
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)splashProc, NULL, 0, NULL);
}

DWORD WINAPI splashProc(void *refcon)
{
	WNDCLASS wc;
	RECT r, rSize;
	BITMAP bits;
	DWORD width, height;

	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = SplashWndProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = kSplashClass;
	RegisterClass(&wc);

	gSplashBMP = LoadBitmap(hInst, MAKEINTRESOURCE(1002));
	GetObject(gSplashBMP, sizeof(bits), &bits);

	GetWindowRect(GetDesktopWindow(), &r);
	rSize.top = 0;
	rSize.left = 0;
	rSize.right = bits.bmWidth;
	rSize.bottom = bits.bmHeight;
	AdjustWindowRectEx(&rSize, WS_BORDER | WS_POPUP, false, 0);
	gSplashW = width = rSize.right - rSize.left;
	gSplashH = height = rSize.bottom - rSize.top;
	gSplash = CreateWindowExA(0, kSplashClass, "",
						WS_BORDER | WS_POPUP,
						(r.right / 2) - (width / 2), (r.bottom / 2) - (height / 2),
						width, height,
						NULL, NULL, hInst, NULL);

	ShowWindow(gSplash, true);
	UpdateWindow(gSplash);

	while (true) {
		MSG msg;

		if (-1 == GetMessage(&msg, gSplash, 0, 0))
			break;

		if (WM_APP == msg.message)
			break;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DestroyWindow(gSplash);
	DeleteObject(gSplashBMP);
	gSplash = 0;

	return 0;
}

void FskSplashHide(void)
{
	if (gSplash) {
		ShowWindow(gSplash, SW_HIDE);
		PostMessage(gSplash, WM_APP, 0, 0);
	}
}

LRESULT CALLBACK SplashWndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_ERASEBKGND: {

			HDC hdc = (HDC)wParam;
			RECT rect;
			HBRUSH hbrush;
			GetClientRect(hwnd, &rect);
			hbrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
			FillRect(hdc, &rect, hbrush);
			DeleteObject(hbrush);
			}
	        break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = GetDC(hwnd);
			HDC srcDC = CreateCompatibleDC(NULL);
			HGDIOBJ saveObj;

			saveObj = SelectObject(srcDC, gSplashBMP);

			BeginPaint(hwnd, &ps);
				BitBlt(hdc, 0, 0, gSplashW, gSplashH, srcDC, 0, 0, SRCCOPY);
			EndPaint(hwnd, &ps);

			SelectObject(srcDC, saveObj);
			ReleaseDC(hwnd, hdc);
			DeleteDC(srcDC);
			}
			break;


		default:
            return (DefWindowProc(hwnd, Msg, wParam, lParam));

    }

    return 0;
}
#endif

#endif

