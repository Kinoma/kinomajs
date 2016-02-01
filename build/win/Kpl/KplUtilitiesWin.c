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
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include "shlobj.h"

#define __FSKTIME_PRIV__
#define __FSKWINDOW_PRIV__
#define __FSKUTILITIES_PRIV__
#define __FSKTHREAD_PRIV__
#define __FSKNETUTILS_PRIV__

#include "FskMemory.h"
#include "FskThread.h"
#include "FskWindow.h"
#include "FskUtilities.h"
#include "FskPlatformImplementation.h"
#include "FskString.h"
#include "FskTextConvert.h"

#include "KplUtilities.h"

HINSTANCE hInst;
HWND gUtilsWnd;
UINT gThreadEventMessage;

static long FAR PASCAL UtilsWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam);

extern void volumeListChanged(const WCHAR *path, UInt32 op);

FskErr KplUtilitiesInitialize(void)
{
	WNDCLASS wc;

	SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);		// don't pop-up system dialogs because of write attempts to locked disks, etc.
	CoInitialize(NULL);
	
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
	
	return kFskErrNone;
}

void KplUtilitiesTerminate(void)
{
	if (NULL != gUtilsWnd) {
		DestroyWindow(gUtilsWnd);
		gUtilsWnd = NULL;
	}
	CoUninitialize();
}

FskErr KplUtilitiesHardwareInitialize(void)
{
	return kFskErrNone;
}

FskErr KplUtilitiesHardwareTerminate(void)
{
	return kFskErrNone;
}

UInt32 KplUtilitiesRandomSeedInit(void)
{
	return GetTickCount();
}

char *KplUtilitiesGetApplicationPath(void)
{
	UInt16 name[_MAX_PATH];
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
}

FskErr KplBrowserOpenURI(const char *uri)
{
	FskErr err = kFskErrNone;
	UInt16 *platformText = NULL;
	SHELLEXECUTEINFOW shellExecuteInfo;

	err = FskTextUTF8ToUnicode16LE((const unsigned char *)uri, FskStrLen(uri), (UInt16 **)&platformText, NULL);
	
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
	
	return err;
}

FskErr KplLauncherOpenURI(const char *uri)
{
	FskErr err = kFskErrNone;
	UInt16 *platformText = NULL;
	SHELLEXECUTEINFOW shellExecuteInfo;

	err = FskTextUTF8ToUnicode16LE((const unsigned char *)uri, FskStrLen(uri), (UInt16 **)&platformText, NULL);
	
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
	
	return err;
}

void KplUtilitiesDelay(UInt32 ms)
{
	Sleep(ms);
}

SInt32 KplUtilitiesRandom(UInt32 *randomSeed)
{
	return rand();
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
		default:
			if (gFskShellExecute == msg) {
				ShellExecuteW(0, L"open", (LPCWSTR)lParam, NULL, NULL, SW_SHOW);
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
			if (gFskOpenFiles == msg) {
				FskWindow w = FskWindowGetInd(0, NULL);
				if (NULL != w) {
					FskEvent		event;
					UInt32 fileListSize = 1;
					HANDLE hFileMap;
					char number[64];
					char *fileName;

					FskStrNumToStr(lParam, number, sizeof(number));
					fileName = FskStrDoCat("FskFileOpen-", number);

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
