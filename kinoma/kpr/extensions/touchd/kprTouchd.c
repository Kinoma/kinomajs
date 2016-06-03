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
#include "kprMessage.h"
#include "kprShell.h"

#if MINITV
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#define TOUCHD_CONF "/etc/touchd/touchd.conf"

#define TOUCH_PIPE "/etc/touchd/pipe"

// extension

FskExport(FskErr) kprTouchd_fskLoad(FskLibrary library)
{
	return kFskErrNone;
}


FskExport(FskErr) kprTouchd_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

// system

void KPR_system_get_backGesture(xsMachine *the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	char* pathName = NULL;
	FskFileMapping map = NULL;
	unsigned char *data;
	FskInt64 size;
#if MINITV
	pathName = TOUCHD_CONF;
#else
	bailIfError(FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeApplicationPreference, true, NULL, &path));
	pathName = FskStrDoCat(path, "touchd.conf");
	bailIfNULL(pathName);
#endif
	bailIfError(FskFileMap(pathName, &data, &size, 0, &map));
	xsResult = xsStringBuffer((xsStringValue)data, (xsIntegerValue)size);
bail:
	FskFileDisposeMap(map);	
	if (path) {
		FskMemPtrDispose(path);
		FskMemPtrDispose(pathName);
	}
}

void KPR_system_set_backGesture(xsMachine *the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	char* pathName = NULL;
	FskFile file = NULL;
	char* data = xsToString(xsArg(0));
	xsIntegerValue size = FskStrLen(data);
#if MINITV
	pathName = TOUCHD_CONF;
	// system("/etc/init.d/touchd.sh stop");
#else
	bailIfError(FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeApplicationPreference, true, NULL, &path));
	pathName = FskStrDoCat(path, "touchd.conf");
	bailIfNULL(pathName);
#endif
	err = FskFileOpen(pathName, kFskFilePermissionReadWrite, &file);
	if (kFskErrFileNotFound == err) {
		bailIfError(FskFileCreate(pathName));
		err = FskFileOpen(pathName, kFskFilePermissionReadWrite, &file);
	}
	else {
		FskInt64 zero = 0;
		err = FskFileSetSize(file, &zero);
	}
	bailIfError(err);
	bailIfError(FskFileWrite(file, size, data, NULL));
bail:
	FskFileClose(file);
#if MINITV
    int pipe = open(TOUCH_PIPE, O_WRONLY | O_NONBLOCK);
    if (pipe >= 0) {
    	char data = 1;
    	write(pipe, &data, 1);
        close(pipe);
    }
	// system("/etc/init.d/touchd.sh start");
#endif
	if (path) {
		FskMemPtrDispose(path);
		FskMemPtrDispose(pathName);
	}
	xsThrowIfFskErr(err);
}
