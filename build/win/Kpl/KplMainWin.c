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
#include <windows.h>
#include <stdlib.h>

#include "FskUtilities.h"
#include "KplMain.h"
#include "FskErrors.h"
#include "KplThread.h"

extern HINSTANCE hInst;
extern Boolean gQuitting;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

static int KplMainApplicationLoop(void *refcon)
{
	while (!gQuitting)
		KplThreadRunloopCycle(-1);
		
	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int argc, i;
	UInt16 **argvUnicode;
	UInt8 **argvUTF8;
	int result;

	// FskMainSetHInstance(hInstance);
	hInst = hInstance;

	argvUnicode = CommandLineToArgvW(GetCommandLineW(), &argc);
	argvUTF8 = (UInt8 **)calloc(sizeof(UInt16 *), argc);
	for (i= 0; i< argc; i++) {
		int charCount = wcslen(argvUnicode[i]);
		argvUTF8[i] = malloc((charCount + 1) * 3);
		WideCharToMultiByte(CP_UTF8, 0, argvUnicode[i], charCount + 1, argvUTF8[i], (charCount + 1) * 3, NULL, NULL);
	}
	GlobalFree(argvUnicode);

	result = KplMain(kKplMainNetwork | kKplMainServer, argc, argvUTF8, KplMainApplicationLoop, 0);

	for (i = 0; i < argc; i++)
		free(argvUTF8[i]);
	free(argvUTF8);

	return result;
}
