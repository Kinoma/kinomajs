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
#ifndef __FSKPLATFORMIMPLEMENTATION_H__
#define __FSKPLATFORMIMPLEMENTATION_H__

/*
	This file contains only platform specific definitions.

	WARNING:
	
		This file should NEVER be included by clients of the
		Fsk software layer. It is for the Fsk implementation
		itself only.
*/

#if TARGET_OS_WIN32

#include "Windows.h"

#ifdef __cplusplus
extern "C" {
#endif

	#define kFskWindowClassName "FskWnd1"
	#define FSK_WM_MOUSEMOVE (WM_APP + 0x1123)
	#define FSK_WM_MOUSE_WITH_TIME (WM_APP + 0x1124)

	// timer window	
	extern HINSTANCE hInst;
	extern HWND gUtilsWnd;
	extern Boolean gEmptyingClipboard;

	// network window
	extern HWND nethwnd;
	extern UINT gAsyncSelectMessage;

	char *fixUpPathForFsk(const WCHAR *path);
	void volumeListChanged(const WCHAR *path, UInt32 op);
	FskAPI(FskErr) FskWinCreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, HANDLE *handle);

#if TARGET_OS_KPL
	extern int toupper(int c);
	extern int isdigit(int c);
#endif

	// FskEvent posted to windows message queue
	extern UINT gThreadEventMessage;

	// OS version info
	extern OSVERSIONINFOEX gWindowsVersionInfo;
	
#ifdef __cplusplus
}
#endif
#elif TARGET_OS_MAC
	#ifndef INADDR_ANY
	#define INADDR_ANY	0
	#endif
	Boolean FskIsMacOSX(void);

#elif TARGET_OS_LINUX
	#ifndef INADDR_ANY
	#define INADDR_ANY	0
	#endif

#elif TARGET_OS_KPL
	#include "FskPlatformImplementation.Kpl.h"
#endif

#endif
