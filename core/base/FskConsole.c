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
#include "FskConsole.h"

#if TARGET_OS_WIN32 || (TARGET_OS_KPL && defined(_MSC_VER))

#include <Windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#include "FskMain.h"

BOOL WINAPI consoleHandler(DWORD dwCtrlType);

Boolean gHasConsole;

FskErr FskConsoleCreate(void)
{
	FILE *hf;
	int hCrt;

	AllocConsole();
	SetConsoleCtrlHandler(consoleHandler, true);

	// fix up std out
	hCrt = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_BINARY);
	if (INVALID_HANDLE_VALUE != (HANDLE)hCrt) {
		hf = _fdopen(hCrt, "w");
		*stdout = *hf;
		setvbuf(stdout, NULL, _IONBF, 0); 
	}

	// fix up std err
	hCrt = _open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), _O_BINARY);
	if (INVALID_HANDLE_VALUE != (HANDLE)hCrt) {
		hf = _fdopen(hCrt, "w");
		*stderr = *hf;
		setvbuf(stderr, NULL, _IONBF, 0); 
	}

	gHasConsole = true;

	return kFskErrNone;
}

FskErr FskConsoleDestroy(void)
{
	if (gHasConsole)
		FreeConsole();

	gHasConsole = false;

	return kFskErrNone;
}


BOOL WINAPI consoleHandler(DWORD dwCtrlType)
{
	switch (dwCtrlType) {
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			FskMainDoQuit(0);
			return true;

		default:
			return false;
	}
}

#elif TARGET_OS_LINUX

FskErr FskConsoleCreate(void)
{
	return kFskErrNone;
}

FskErr FskConsoleDestroy(void)
{
	return kFskErrNone;
}

#elif TARGET_OS_MAC

// #include "SIOUX.h"

FskErr FskConsoleCreate(void)
{
//	SIOUXSettings.stubmode = false;

	return kFskErrNone;
}

FskErr FskConsoleDestroy(void)
{
	return kFskErrNone;
}

#else

FskErr FskConsoleCreate(void)
{
	return kFskErrUnimplemented;
}

FskErr FskConsoleDestroy(void)
{
	return kFskErrUnimplemented;
}

#endif
