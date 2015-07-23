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
#include "xs.h"
#include "tools.xs.h"

#if mxWindows
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#define mxSeparator '\\'
	#define PATH_MAX 1024
	#include <direct.h>
	#include <process.h>
#else
	#include <dirent.h>
	#include <limits.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#define mxSeparator '/'
#endif
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void Tool_prototype_get_currentDirectory(xsMachine* the)
{
	char buffer[PATH_MAX];
#if mxWindows
	xsElseError(_getcwd(buffer, PATH_MAX));
#else
	xsElseError(getcwd(buffer, PATH_MAX));
#endif
	xsResult = xsString(buffer);
}

void Tool_prototype_set_currentDirectory(xsMachine* the)
{
#if mxWindows
	xsElseError(0 == _chdir(xsToString(xsArg(0))));
#else
	xsElseError(0 == chdir(xsToString(xsArg(0))));
#endif
}

void Tool_prototype_get_currentPlatform(xsMachine* the)
{
	#if mxWindows
		xsResult = xsString("win");
	#elif mxMacOSX
		xsResult = xsString("mac");
	#elif mxLinux
		xsResult = xsString("linux");
	#else
		#pragma error("need a platform")
	#endif
}

void Tool_prototype_execute(xsMachine* the)
{
#if mxWindows
#else
	char buffer[PATH_MAX];
	xsStringValue command = xsToString(xsArg(0));
	FILE* pipe = popen(command, "r");
    xsResult = xsString("");
	if (pipe) {
        xsIntegerValue size;
        for (;;) {
         	size = fread(buffer, sizeof(char), PATH_MAX - 1, pipe);
         	if (size <= 0)
         		break;
        	buffer[size] = 0;
        	xsResult = xsCall1(xsResult, xsID("concat"), xsString(buffer));
        }
		pclose(pipe);
	}
#endif
}

void Tool_prototype_joinPath(xsMachine* the)
{
	char path[PATH_MAX];
	int length;
	strcpy(path, xsToString(xsGet(xsArg(0), xsID("directory"))));
	length = strlen(path);
	path[length] = mxSeparator;
	path[length + 1] = 0;
	strcat(path, xsToString(xsGet(xsArg(0), xsID("name"))));
	if (xsHas(xsArg(0), xsID("extension")))
		strcat(path, xsToString(xsGet(xsArg(0), xsID("extension"))));
	xsResult = xsString(path);
}

void Tool_prototype_report(xsMachine* the)
{
	fprintf(stderr, "%s\n", xsToString(xsArg(0)));
}

void Tool_prototype_reportError(xsMachine* the)
{
	xsIntegerValue c;
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		xsStringValue path = xsToString(xsArg(0));
		xsIntegerValue line = xsToInteger(xsArg(1));
	#if mxWindows
		fprintf(stderr, "%s(%ld): error: ", path, line);
	#else
		fprintf(stderr, "%s:%ld: error: ", path, line);
	#endif
	}
	else
		fprintf(stderr, "# error: ");
	fprintf(stderr, "%s!\n", xsToString(xsArg(2)));
	c = xsToInteger(xsGet(xsThis, xsID("errorCount")));
	xsSet(xsThis, xsID("errorCount"), xsInteger(c + 1));
}

void Tool_prototype_reportWarning(xsMachine* the)
{
	xsIntegerValue c;
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		xsStringValue path = xsToString(xsArg(0));
		xsIntegerValue line = xsToInteger(xsArg(1));
	#if mxWindows
		fprintf(stderr, "%s(%ld): warning: ", path, line);
	#else
		fprintf(stderr, "%s:%ld: warning: ", path, line);
	#endif
	}
	else
		fprintf(stderr, "# warning: ");
	fprintf(stderr, "%s!\n", xsToString(xsArg(2)));
	c = xsToInteger(xsGet(xsThis, xsID("warningCount")));
	xsSet(xsThis, xsID("warningCount"), xsInteger(c + 1));
}

void Tool_prototype_resolveDirectoryPath(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char buffer[PATH_MAX];
#if mxWindows
	DWORD attributes;
	char *separator = path;
	while (*separator) {
		if (*separator == '/')
			*separator = mxSeparator;
		separator++;
	}
	if (_fullpath(buffer, path, PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
			xsResult = xsString(buffer);
		}
	}
#else
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(buffer, &a_stat) == 0) {
			if (S_ISDIR(a_stat.st_mode)) {
				xsResult = xsString(buffer);
			}
		}
	}
#endif
}

void Tool_prototype_resolveFilePath(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char buffer[PATH_MAX];
#if mxWindows
	DWORD attributes;
	char *separator = path;
	while (*separator) {
		if (*separator == '/')
			*separator = mxSeparator;
		separator++;
	}
	if (_fullpath(buffer, path, PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (!(attributes & FILE_ATTRIBUTE_DIRECTORY))) {
			xsResult = xsString(buffer);
		}
	}
#else
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(path, &a_stat) == 0) {
			if (S_ISREG(a_stat.st_mode)) {
				xsResult = xsString(buffer);
			}
		}
	}
#endif
}

void Tool_prototype_resolvePath(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char buffer[PATH_MAX];
#if mxWindows
	DWORD attributes;
	char *separator = path;
	while (*separator) {
		if (*separator == '/')
			*separator = mxSeparator;
		separator++;
	}
	if (_fullpath(buffer, path, PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if (attributes != 0xFFFFFFFF) {
			xsResult = xsString(buffer);
		}
	}
#else
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(path, &a_stat) == 0) {
			xsResult = xsString(buffer);
		}
	}
#endif
}

void Tool_prototype_splitPath(xsMachine* the)
{
	char *path;
	char *slash = NULL;
	char *dot = NULL;
	int length;
	char directory[PATH_MAX];
	char name[PATH_MAX];
	char extension[PATH_MAX];
	
	path = xsToString(xsArg(0));
	slash = strrchr(path, mxSeparator);
	if (slash == NULL)
		slash = path;
	else
		slash++;
	dot = strrchr(slash, '.');
	if (dot == NULL)
		dot = slash + strlen(slash);
	length = slash - path;
	strncpy(directory, path, length);
	if (length)
		directory[length - 1] = 0;
	else
		directory[0] = 0;
	length = dot - slash;
	strncpy(name, slash, length);
	name[length] = 0;
	strcpy(extension, dot);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsSet(xsResult, xsID("directory"), xsString(directory));
	xsSet(xsResult, xsID("name"), xsString(name));
	xsSet(xsResult, xsID("extension"), xsString(extension));
}

