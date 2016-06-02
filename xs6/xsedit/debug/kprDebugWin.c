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
#include "kprURL.h"

void KPR_gotoFront(xsMachine *the)
{
}

void KPR_Files_toPath(xsMachine *the)
{
	char* path;
	char* slash;
	char c;
	xsThrowIfFskErr(KprURLToPath(xsToString(xsArg(0)), &path));
	slash = path;
	while ((c = *slash)) {
		if (c == '/')
			*slash = '\\';
		slash++;
	}
	xsResult = xsString(path);
	FskMemPtrDispose(path);
}

void KPR_Files_toURI(xsMachine *the)
{
	char* path = xsToString(xsArg(0));
	char* slash;
	char c;
	FskErr err;
	char* url;
	slash = path;
	while ((c = *slash)) {
		if (c == '\\')
			*slash = '/';
		slash++;
	}
	err = KprPathToURL(path, &url);
	slash = path;
	while ((c = *slash)) {
		if (c == '/')
			*slash = '\\';
		slash++;
	}
	xsThrowIfFskErr(err);
	xsResult = xsString(url);
	FskMemPtrDispose(url);
}
