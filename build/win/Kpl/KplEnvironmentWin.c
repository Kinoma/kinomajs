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
#include "KplEnvironment.h"
#include "FskTextConvert.h"
#include "FskString.h"

#include "windows.h"

#define SECURITY_WIN32
#include "security.h"

FskErr KplEnvironmentInitialize(FskAssociativeArray environment)
{
	char name[256], *nameTemp = NULL;
	UInt16 nameW[256];
	char num[32];
	DWORD nameSize = sizeof(nameW) / sizeof(UInt16);
	OSVERSIONINFOEX windowsVersionInfo;
	EXTENDED_NAME_FORMAT exNameFormat = NameSamCompatible;

	if (GetUserNameExW(exNameFormat, (LPWSTR)nameW, &nameSize)) {
		FskTextUnicode16LEToUTF8(nameW, nameSize * 2, &nameTemp, NULL);
		FskStrCopy(name, nameTemp);
		FskEnvironmentSet("loginName", name);
		FskMemPtrDispose(nameTemp);
	}

	ZeroMemory(&windowsVersionInfo, sizeof(OSVERSIONINFOEX));
	windowsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((LPOSVERSIONINFOA)&windowsVersionInfo);

	FskEnvironmentSet("OS", "Windows");
	FskStrNumToStr(windowsVersionInfo.dwMajorVersion, name, sizeof(name));
	FskStrCat(name, ".");
	FskStrNumToStr(windowsVersionInfo.dwMinorVersion, num, sizeof(num));
	FskStrCat(name, num);
	FskEnvironmentSet("OSVersion", name);

	return kFskErrNone;
}

