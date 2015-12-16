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
	#include <direct.h>
	#include <errno.h>
#else
	#include <dirent.h>
	#include <sys/stat.h>
	#include <stdlib.h>
	#include <unistd.h>
#endif
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fs_destructor(void* data);

void fs_closeSync(xsMachine* the)
{
	FILE* file = xsGetHostData(xsArg(0));
	if (file) {
		fclose(file);
		xsSetHostData(xsArg(0), NULL);
	}
}

void fs_copyFileSync(xsMachine* the)
{
	char *fromPath = xsToString(xsArg(0));
	char *toPath = xsToString(xsArg(1));
	FILE* fromFile = NULL;
	FILE* toFile = NULL;
	char buffer[1024];
	size_t count;
	
	xsTry {
		fromFile = fopen(fromPath, "r");
		xsElseError(fromFile);
		toFile = fopen(toPath, "w");
		xsElseError(toFile);
		
		while ((count = fread(buffer, 1, sizeof(buffer), fromFile)) > 0) {
			xsElseError(fwrite(buffer, 1, count, toFile) == count);
		}

		fclose(toFile);
		fclose(fromFile);
	}
	xsCatch {
		if (toFile)
			fclose(toFile);
		if (fromFile)
			fclose(fromFile);
		xsThrow(xsException);
	}
}

void fs_destructor(void* data)
{
	if (data)
		fclose(data);
}

void fs_deleteDirectory(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	int result;

#if mxWindows
	result = _rmdir(path);
#else
	result = rmdir(path);
#endif

	if (result)
		xsElseError(NULL);

	xsResult = xsInteger(1);
}

void fs_deleteFile(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	int result;

#if mxWindows
	result = _unlink(path);
#else
	result = unlink(path);
#endif

	if (result) {
		switch (errno) {
			case ENOENT:
				break;
			default:
				xsElseError(NULL);
				break;
		}
	}
	xsResult = xsInteger(1);
}


void fs_existsSync(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
#if mxWindows
	DWORD attributes = GetFileAttributes(path);
	if (attributes != 0xFFFFFFFF) {
		if (attributes & FILE_ATTRIBUTE_DIRECTORY)
			xsResult = xsInteger(-1);
		else 
			xsResult = xsInteger(1);
	}
	else
		xsResult = xsInteger(0);
#else
	struct stat a_stat;
	if (stat(path, &a_stat) == 0) {
		if (S_ISDIR(a_stat.st_mode))
			xsResult = xsInteger(-1);
		else 
			xsResult = xsInteger(1);
	}
	else
		xsResult = xsInteger(0);
#endif
}

void fs_mkdirSync(xsMachine* the)
{
	char* path = xsToString(xsArg(0));
	int result;
#if mxWindows
	result = _mkdir(path);
#else
	result = mkdir(path, 0755);
#endif
	if (result) {
		switch (errno) {
			case EEXIST:
				break;
			default:
				xsElseError(NULL);
				break;
		}
	}
	xsResult = xsArg(0);
}

void fs_readDirSync(xsMachine* the)
{
#if mxWindows
	xsStringValue path, name = NULL;
	UINT32 length, index;
	UINT16 *pathW = NULL;
	HANDLE findHandle = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATAW findData;

	xsTry {
		xsVars(1);
		xsResult = xsNewInstanceOf(xsArrayPrototype);
	
		path = xsToString(xsArg(0));
		length = strlen(path);
		pathW = malloc((length + 3) * 2);
		xsElseError(pathW);
		MultiByteToWideChar(CP_UTF8, 0, path, length + 1, pathW, length + 1);
		for (index = 0; index < length; index++) {
			if (pathW[index] == '/')
				pathW[index] = '\\';
		}
		pathW[length] = '\\';
		pathW[length + 1] = '*';
		pathW[length + 2] = 0;
		findHandle = FindFirstFileW(pathW, &findData);
		if (findHandle != INVALID_HANDLE_VALUE) {
			do {
				if ((findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
					!wcscmp(findData.cFileName, L".") ||
					!wcscmp(findData.cFileName, L".."))
					continue;
				length = wcslen(findData.cFileName);
				name = malloc((length + 1) * 2);
				xsElseError(name);
				WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, length + 1, name, length + 1, NULL, NULL);
				xsVar(0) = xsString(name);
				xsCall1(xsResult, xsID("push"), xsVar(0));
				free(name);
				name = NULL;
			} while (FindNextFileW(findHandle, &findData));
		}
	}
	xsCatch {
	}
	if (name)
		free(name);
	if (findHandle != INVALID_HANDLE_VALUE)
		FindClose(findHandle);
	if (pathW)
		free(pathW);
#else
    DIR* dir;
	char path[1024];
	int length;

	xsVars(1);
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	dir = opendir(xsToStringBuffer(xsArg(0), path, sizeof(path) - 1));
	length = strlen(path);
	path[length] = '/';
	length++;
	if (dir) {
		struct dirent *ent;
		while ((ent = readdir(dir))) {
			struct stat a_stat;
			if (ent->d_name[0] == '.')
				continue;
			strcpy(path + length, ent->d_name);
			if (!stat(path, &a_stat)) {
				xsVar(0) = xsString(ent->d_name);
				(void)xsCall1(xsResult, xsID("push"), xsVar(0));
			}
		}
		closedir(dir);
	}
#endif
}

void fs_readFileSync(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	FILE* file = NULL;
	size_t size;
	char* buffer = NULL;
	xsTry {
		file = fopen(path, "r");
		xsElseError(file);
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		buffer = malloc(size + 1);
		size = fread(buffer, 1, size, file);	
		buffer[size] = 0;	
		xsResult = xsString(buffer);
		free(buffer);
		fclose(file);
	}
	xsCatch {
		if (buffer)
			free(buffer);
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void fs_openSync(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char *flags = xsToString(xsArg(1));
	FILE* file = NULL;
	xsTry {
		file = fopen(path, flags);
		xsElseError(file);
		xsResult = xsNewHostObject(fs_destructor);
		xsSetHostData(xsResult, file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void fs_writeFileSync(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char* buffer = xsToString(xsArg(1));
	size_t size = strlen(buffer);
	FILE* file = NULL;
	xsTry {
		file = fopen(path, "w");
		xsElseError(file);
		fwrite(buffer, 1, size, file);		
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void fs_writeSync(xsMachine* the)
{
	FILE* file = xsGetHostData(xsArg(0));
	char* buffer = xsToString(xsArg(1));
	size_t size = strlen(buffer);
	size = fwrite(buffer, 1, size, file);		
	xsResult = xsInteger(size);
}
