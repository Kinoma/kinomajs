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
#include "kpr.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprShell.h"

void KPR_system_getClipboardText(xsMachine* the)
{
}

void KPR_system_setClipboardText(xsMachine* the)
{
}

void KPR_system_alert(xsMachine* the)
{
}

void KPR_system_beginModal(xsMachine* the)
{
}

void KPR_system_endModal(xsMachine* the)
{
}

void KPR_system_openDirectory(xsMachine* the)
{
	FskErr err;
	int argc = xsToInteger(xsArgc);
	char *prompt = ((argc > 0) && xsTest(xsArg(0))) ? xsToStringCopy(xsArg(0)) : NULL;
	char *initialPath = ((argc > 1) && xsTest(xsArg(1))) ? xsToStringCopy(xsArg(1)) : NULL;
	char *path = NULL;
	err = FskDirectoryChoose(prompt, initialPath, &path);
	if ((kFskErrNone == err) && (NULL != path)) {
		xsResult = xsString(path);
		FskMemPtrDispose(path);
	}
	FskMemPtrDispose(initialPath);
	FskMemPtrDispose(prompt);
}

void KPR_system_openFile(xsMachine* the)
{
	FskErr err;
	KprShell self = xsGetHostData(xsThis);
	KprScriptBehavior behavior = (KprScriptBehavior)self->behavior;
	int argc = xsToInteger(xsArgc);
	char *prompt = ((argc > 0) && xsTest(xsArg(0))) ? xsToStringCopy(xsArg(0)) : NULL;
	char *initialPath = ((argc > 1) && xsTest(xsArg(1))) ? xsToStringCopy(xsArg(1)) : NULL;
	Boolean allowMultiple = (argc > 2) ? xsToBoolean(xsArg(2)) : false;
	char *paths = NULL;
	err = FskFileChoose(NULL, prompt, allowMultiple, initialPath, &paths);
	if ((kFskErrNone == err) && (NULL != paths)) {
		xsVars(3);
		{
			xsTry {
				xsVar(0) = xsAccess(behavior->slot);
				if (xsFindResult(xsVar(0), xsID("onOpenFiles"))) {
					char *path = paths;
					xsVar(1) = kprContentGetter(self);
					xsVar(2) = xsNewInstanceOf(xsArrayPrototype);
					while (0 != *path) {
						(void)xsCall1(xsResult, xsID("push"), xsString(path));
						path += FskStrLen(path) + 1;
					}
					FskMemPtrDispose(paths);
					(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
				}
			}
			xsCatch {
			}
		}
	}
	FskMemPtrDispose(initialPath);
	FskMemPtrDispose(prompt);
}

void KPR_system_saveDirectory(xsMachine* the)
{
}

void KPR_system_saveFile(xsMachine* the)
{
	FskErr err;
	int argc = xsToInteger(xsArgc);
	char *prompt = ((argc > 0) && xsTest(xsArg(0))) ? xsToStringCopy(xsArg(0)) : NULL;
	char *initialPath = ((argc > 1) && xsTest(xsArg(1))) ? xsToStringCopy(xsArg(1)) : NULL;
	char *name = ((argc > 2) && xsTest(xsArg(2))) ? xsToStringCopy(xsArg(2)) : NULL;
	char *path;
	err = FskFileChooseSave(name, prompt, initialPath, &path);
	if ((kFskErrNone == err) && (NULL != path)) {
		xsResult = xsString(path);
		FskMemPtrDispose(path);
	}
	FskMemPtrDispose(name);
	FskMemPtrDispose(prompt);
	FskMemPtrDispose(initialPath);
}

void KPR_shell_execute(xsMachine* the)
{
}

void KPR_shell_splitError(xsMachine* the)
{
}


void KPR_system_getWifiInfo(xsMachine* the)
{
}

void KPR_system_networkInterfaceIndexToName(xsMachine* the)
{
}

