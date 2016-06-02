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
#import <AppKit/NSApplication.h>

void KPR_gotoFront(xsMachine *the)
{
	[[NSRunningApplication currentApplication] activateWithOptions:NSApplicationActivateIgnoringOtherApps];
}

void KPR_Files_toPath(xsMachine *the)
{
	char* path;
	xsThrowIfFskErr(KprURLToPath(xsToString(xsArg(0)), &path));
	xsResult = xsString(path);
	FskMemPtrDispose(path);
}

void KPR_Files_toURI(xsMachine *the)
{
	char* url;
	xsThrowIfFskErr(KprPathToURL(xsToString(xsArg(0)), &url));
	xsResult = xsString(url);
	FskMemPtrDispose(url);
}
