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
#define __FSKTHREAD_PRIV__
#include "FskEnvironment.h"

#include "FskAssociativeArray.h"
#include "FskFiles.h"
#include "FskThread.h"
#include "FskUtilities.h"
#include "FskList.h"

#if TARGET_OS_KPL
	#include "KplEnvironment.h"
#elif TARGET_OS_LINUX
	#include <unistd.h>
	#include <sys/utsname.h>
#elif TARGET_OS_MAC
	#include <sys/utsname.h>
	#if TARGET_OS_IPHONE
		#include "FskCocoaSupportPhone.h"
	#else
		#include "FskCocoaSupport.h"
	#endif
#elif TARGET_OS_WIN32
	#include "FskPlatformImplementation.h"
	#include "FskTextConvert.h"
	#define SECURITY_WIN32
	#include "security.h"
#endif

static FskAssociativeArray gEnvironment;

FskErr FskEnvironmentInitialize(void)
{
	char *appPath;

	gEnvironment = FskAssociativeArrayNew();

	appPath = FskGetApplicationPath();
	FskEnvironmentSet("applicationPath", appPath);
	FskMemPtrDispose(appPath);

#if TARGET_OS_KPL
	KplEnvironmentInitialize(gEnvironment);
#elif TARGET_OS_ANDROID
	FskEnvironmentSet("application", "PLAY");
#elif TARGET_OS_WIN32 || TARGET_OS_MAC || TARGET_OS_LINUX
	FskEnvironmentSet("application", FSK_APPLICATION);
#else
	FskEnvironmentSet("application", "PLAY");
#endif

#if TARGET_OS_WIN32
	{
	char name[256], *nameTemp = NULL;
	UInt16 nameW[256];
	char num[32];
	DWORD nameSize = sizeof(nameW) / sizeof(UInt16);
	EXTENDED_NAME_FORMAT exNameFormat = NameSamCompatible;

	if (GetUserNameExW(exNameFormat, (LPWSTR)nameW, &nameSize)) {
		FskTextUnicode16LEToUTF8(nameW, nameSize * 2, &nameTemp, NULL);
		FskStrCopy(name, nameTemp);
		FskEnvironmentSet("loginName", name);
		FskMemPtrDispose(nameTemp);
	}

	FskEnvironmentSet("OS", "Windows");
	FskStrNumToStr(gWindowsVersionInfo.dwMajorVersion, name, sizeof(name));
	FskStrCat(name, ".");
	FskStrNumToStr(gWindowsVersionInfo.dwMinorVersion, num, sizeof(num));
	FskStrCat(name, num);
	FskEnvironmentSet("OSVersion", name);
	}
#elif TARGET_OS_MAC
	{
		struct utsname un;
		char name[256], *model;
		SInt32 gen;
	#if TARGET_OS_IPHONE
		FskEnvironmentSet("OS", "iPhone");
	#else
		FskEnvironmentSet("OS", "Mac");
	#endif
		FskCocoaSystemGetVersion(name);
		FskEnvironmentSet("OSVersion", name);
		if (uname(&un) == 0) {
			model = un.machine;
			if (FskStrCompareWithLength(model, "iPhone", 6) == 0)
				gen = FskStrToNum(model + 6);
			else if (FskStrCompareWithLength(model, "iPad", 4) == 0) {
				gen = FskStrToNum(model + 4);
				if (gen == 3) {
					SInt32 minor = FskStrToNum(model + 6);
					if (minor == 4)	/* 4th gen */
						gen = 5;
					else
						gen = 4;	/* Only the 3rd gen iPad doesn't follow the numbering system */
				}
				else
					gen += 2;
			}
			else if (FskStrCompareWithLength(model, "iPod", 4) == 0) {
				gen = FskStrToNum(model + 4);
				if (gen > 1)
					--gen;
			}
			else
				gen = 99;
		}
		else {
			model = "unknown";
			gen = 99;
		}
		FskEnvironmentSet("Model", model);
		FskStrNumToStr(gen, name, sizeof(name));
		FskEnvironmentSet("Generation", name);
	}
		
#elif TARGET_OS_LINUX
	{
	struct utsname name;
	uname(&name);

	if (getlogin())
		FskEnvironmentSet("loginName", getlogin());
	else
		FskEnvironmentSet("loginName", "User");
	FskEnvironmentSet("OS", name.sysname);
	FskEnvironmentSet("OSVersion", name.release);		//@@
	}
#endif

	return kFskErrNone;
}

void FskEnvironmentTerminate(void)
{
	FskAssociativeArrayDispose(gEnvironment);
}

char *FskEnvironmentApply(char *input)
{
	FskErr err;
	char *p;
	char *out = NULL;
	char *start, *end;
	UInt32 outCount = 0;
	char tempName[256];
	UInt32 nameLen;
	char *value;
	Boolean doApply = false;

	if (NULL == input)
		goto bail;

	if ('"' == *input) {
		// special case for string literal - useful when there is leading or trailing white space or [ or ] in the string
		UInt32 len = FskStrLen(input);
		if ('"' == input[len - 1]) {
			err = FskMemPtrNew(len - 1, &out);
			BAIL_IF_ERR(err);
			out[len - 2] = 0;
			FskMemMove(out, input + 1, len - 2);
			goto bail;
		}
	}

	// scan
	p = input;
	while (true) {
		start = FskStrChr(p, '[');
		if (NULL == start) {
			outCount += FskStrLen(p);
			break;
		}
		outCount += start - p;
		end = FskStrChr(start + 1, ']');
		if (NULL == end) {
			outCount += FskStrLen(start);
			break;
		}

		nameLen = end - start - 1;
		if (nameLen > (sizeof(tempName) - 1))
			goto bail;
		FskMemMove(tempName, start + 1, nameLen);
		tempName[nameLen] = 0;

		value = FskEnvironmentGet(tempName);
		outCount += FskStrLen(value);

		doApply = true;
		p = end + 1;
	}

	if (!doApply)
		goto bail;

	// replace
	err = FskMemPtrNew(outCount + 1, &out);
	BAIL_IF_ERR(err);

	out[0] = 0;
	p = input;
	while (true) {
		start = FskStrChr(p, '[');
		if (NULL == start) {
			FskStrCat(out, p);
			break;
		}
		outCount += start - p;
		end = FskStrChr(start + 1, ']');
		if (NULL == end) {
			FskStrCat(out, start);
			break;
		}

		FskStrNCat(out, p, start - p);

		nameLen = end - start - 1;
		FskMemMove(tempName, start + 1, nameLen);
		tempName[nameLen] = 0;
		FskStrCat(out, FskEnvironmentGet(tempName));

		p = end + 1;
	}

bail:
	return out;
}

char *FskEnvironmentDoApply(char *input)
{
	char *result = FskEnvironmentApply(input);
	if (result) {
		FskMemPtrDispose(input);
		return result;
	}
	else
		return input;
}

char *FskEnvironmentGet(char *name)
{
	FskThread thread = FskThreadGetCurrent();
	if (thread->environmentVariables) {
		char *result = FskAssociativeArrayElementGetString(thread->environmentVariables, name);
		if (result)
			return result;
	}

	return FskAssociativeArrayElementGetString(gEnvironment, name);
}

void FskEnvironmentSet(const char *name, const char *value)
{
	FskThread thread = FskThreadGetCurrent();
	FskAssociativeArray vars = gEnvironment;

	if (!(kFskThreadFlagsIsMain & thread->flags)) {
		if (NULL == thread->environmentVariables)
			thread->environmentVariables = FskAssociativeArrayNew();
		vars = thread->environmentVariables;
	}
	FskAssociativeArrayElementSetString(vars, name, (char *)value);
}

