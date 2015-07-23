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
#include "FskMain.h"

#if !TARGET_OS_MAC
    static FskErr doMain(UInt32 flags, int argc, char **argv);
#endif

#if TARGET_OS_KPL
	#include "KplMain.h"
#elif TARGET_OS_IPHONE
	#include "FskCocoaMainPhone.h"
#endif

/* ----------------------------------------------------------------------------------------------- */

#define USE_TIMEBOMB 0
#if USE_TIMEBOMB
	const long kLaunchCheckTimeBombYear = 2008;
	const long kLaunchCheckTimeBombMonth = 9;
	const long kLaunchCheckTimeBombDate = 1;
	static Boolean CheckDate(void);
#endif /* USE_TIMEBOMB */

#if TARGET_OS_WIN32

#if USE_TIMEBOMB

#include <time.h>

Boolean CheckDate(void)
{
	Boolean		dateOK = false;
	time_t		currentTime = 0, expireTime;
	struct tm	expireTimeTM = {0};

	time(&currentTime);

	expireTimeTM.tm_year = kLaunchCheckTimeBombYear - 1900;
	expireTimeTM.tm_mon = kLaunchCheckTimeBombMonth - 1;
	expireTimeTM.tm_mday = kLaunchCheckTimeBombDate;
	expireTimeTM.tm_hour = 23;
	expireTimeTM.tm_min = 59;
	expireTimeTM.tm_sec = 59;

	expireTime = mktime(&expireTimeTM);

	if (currentTime < expireTime)
		dateOK = true;
	else
		goto bail;

bail:
	if (!dateOK)
		MessageBoxW(NULL, L"This application has expired.", L"Expired", MB_OK);

	return dateOK;
}
#endif /* USE_TIMEBOMB */

#define __FSKUTILITIES_PRIV__
#include "FskUtilities.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int argc, i;
	UInt16 **argvUnicode;
	UInt8 **argvUTF8;
	int result;
	HANDLE startupCheck;
	UInt16 *moduleFileName, *mutexName;
	DWORD lastError;
	const UInt16 *kMutexBaseName = L"Fsk-startup-check-";

#if USE_TIMEBOMB
	if (!CheckDate())
		return -1;
#endif

	FskMainSetHInstance(hInstance);

	argvUnicode = CommandLineToArgvW(GetCommandLineW(), &argc);
	argvUTF8 = (UInt8 **)calloc(sizeof(UInt16 *), argc);
	for (i= 0; i< argc; i++) {
		int charCount = wcslen(argvUnicode[i]);
		argvUTF8[i] = malloc((charCount + 1) * 3);
		WideCharToMultiByte(CP_UTF8, 0, argvUnicode[i], charCount + 1, argvUTF8[i], (charCount + 1) * 3, NULL, NULL);
	}
	GlobalFree(argvUnicode);

	moduleFileName = (UInt16 *)malloc(sizeof(UInt16) * 520);
	GetModuleFileNameW(0, moduleFileName, 512);
	mutexName = malloc((1 + wcslen(moduleFileName) + wcslen(kMutexBaseName)) * 2);
	wcscpy(mutexName, kMutexBaseName);
	wcscat(mutexName, wcsrchr(moduleFileName, '\\') + 1);
	free(moduleFileName);
	startupCheck = CreateMutexW(0, true, mutexName);
	lastError = GetLastError();
	free(mutexName);
	
   for (i = 0; i < argc; i++){
      if(strcmp(argvUTF8[i],"-new-instance")==0) {
         lastError=0;
         break;
      }
	}
	switch (lastError) {
		case 0:
			result = doMain(kFskMainNetwork | kFskMainServer, argc, argvUTF8);
			break;

		case ERROR_ALREADY_EXISTS: {
			char *fileList;
			UInt32 fileListSize;

			result = 0;
			FskUtilsSetArgs(argc, argvUTF8);
			fileList = FskUtilsGetFileArgs(&fileListSize);
			if (NULL != fileList) {
				char *fileName;
				HANDLE hMapFile;
				char number[64];
				SInt32 val;
				
				val = FskRandom() ^ GetTickCount();
				FskStrNumToStr(val, number, sizeof(number));
				fileName = FskStrDoCat("FskFileOpen-", number);

				hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, fileListSize, fileName);
				if ((NULL != hMapFile) && (INVALID_HANDLE_VALUE != hMapFile)) {
					unsigned char *pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, fileListSize);
					if (NULL != pBuf) {
						memmove(pBuf, fileList, fileListSize);
						UnmapViewOfFile(pBuf);
						PostMessage(FindWindow("projectf-utils", NULL), RegisterWindowMessage("FskOpenFiles"), 0, val);
					}
					CloseHandle(hMapFile);
				}
				FskMemPtrDispose(fileName);
			}
			}
			break;

		default:
			result = -1;
			break;
	}

	CloseHandle(startupCheck);

	for (i = 0; i < argc; i++)
		free(argvUTF8[i]);
	free(argvUTF8);

	return result;
}

/* ----------------------------------------------------------------------------------------------- */
#elif TARGET_OS_KPL
int KplMain(UInt32 flags, int argc, char **argv, KplMainProc kplMainProc, void *kplMainProcRefcon)
{
	extern void mainExtensionInitialize(void);
	extern void mainExtensionTerminate(void);
#ifdef ANDROID_PLATFORM
	extern void initializeT7input(void);
	extern void terminateT7input(void);
#endif // ANDROID_PLATFORM
#if LINUX_PLATFORM
	extern void initializeLinuxinput(void);
	extern void terminateLinuxinput(void);
#endif
	FskErr err;

	err = FskMainInitialize(flags, argc, argv);
    BAIL_IF_ERR(err);

	mainExtensionInitialize();
#ifdef ANDROID_PLATFORM
	initializeT7input();
#endif // ANDROID_PLATFORM
#if LINUX_PLATFORM
	initializeLinuxinput();
#endif

	err = kplMainProc(kplMainProcRefcon);

#ifdef ANDROID_PLATFORM
	terminateT7input();
#endif // ANDROID_PLATFORM
#if LINUX_PLATFORM
	terminateLinuxinput();
#endif

	mainExtensionTerminate();
	FskMainTerminate();

bail:
	return err;
}
#else

int main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	
#if TARGET_OS_MAC
	#if USE_TIMEBOMB
		CheckDate();
	#endif /* USE_TIMEBOMB */
	return FskCocoaMain(kFskMainNetwork | kFskMainServer, argc, argv);
#else
	int err = (int)doMain(kFskMainNetwork | kFskMainServer, argc, argv);
#if TARGET_OS_LINUX
	fprintf(stderr, "successfully exited with error code %d.\n", err);
#endif
	return err;
#endif
}

#endif

#if !TARGET_OS_MAC
/* ----------------------------------------------------------------------------------------------- */
FskErr doMain(UInt32 flags, int argc, char **argv)
{
	extern void mainExtensionInitialize(void);
	extern void mainExtensionTerminate(void);
	FskErr err;

	err = FskMainInitialize(flags, argc, argv);
	BAIL_IF_ERR(err);

	mainExtensionInitialize();

	err = FskMainApplicationLoop();

	mainExtensionTerminate();
	FskMainTerminate();

bail:
	return err;
}
#endif

#if TARGET_OS_MAC && USE_TIMEBOMB

Boolean CheckDate(void)
{
	Boolean		dateOK = false;
	time_t		currentTime = 0, expireTime;
	struct tm	expireTimeTM = {0};
	
	time(&currentTime);
	
	expireTimeTM.tm_year = kLaunchCheckTimeBombYear - 1900;
	expireTimeTM.tm_mon = kLaunchCheckTimeBombMonth - 1;
	expireTimeTM.tm_mday = kLaunchCheckTimeBombDate;
	expireTimeTM.tm_hour = 23;
	expireTimeTM.tm_min = 59;
	expireTimeTM.tm_sec = 59;
	expireTime = mktime(&expireTimeTM);
	
	dateOK = (currentTime < expireTime);
	if (!dateOK)
		fskExpired();
	
	return dateOK;
}

#endif /* TARGET_OS_MAC && USE_TIMEBOMB */

