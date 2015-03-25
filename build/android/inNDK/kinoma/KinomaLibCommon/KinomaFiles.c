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
#include <string.h>
#include "jni.h"

#define __FSKTHREAD_PRIV__
#define __FSKFILES_PRIV__

#include "Fsk.h"
#include "FskThread.h"
#include "FskFiles.h"
#include "FskInstrumentation.h"

extern FskFileDispatchTableRecord gFSDispatch;

#include <sys/inotify.h>

FskInstrumentedSimpleType(AndroidFSNotifier, androidfsnotifier);

enum {
    kFskPathIsAny = 0,
    kFskPathIsFile = 1,
    kFskPathIsDirectory = 2
};

FskErr sCheckFullPath(const char *fullPath, UInt32 pathType);


struct FskFSDirectoryChangeNotifierRecord {
	FskDirectoryChangeNotifierRecord        base;
	UInt32                                  flags;
	FskDirectoryChangeNotifierCallbackProc  callback;
	void                                    *refCon;

	FskThread                               thread;
	SInt32                                  count;

	int                                     wd;
};

typedef struct FskFSDirectoryChangeNotifierRecord FskFSDirectoryChangeNotifierRecord;
typedef struct FskFSDirectoryChangeNotifierRecord *FskFSDirectoryChangeNotifier;

static FskMutex gDirChangeNotifiersMutex = NULL;
static FskFSDirectoryChangeNotifier gDirectoryChangeNotifiers = NULL;
static int iNotifyFD = -1;

static FskThreadDataSource gDirChangeSource = NULL;
static FskThreadDataHandler gDirChangeHandler = NULL;

FskErr androidDirectoryChangeNotifierDispose(FskDirectoryChangeNotifier dirChangeNtf);

static void doChangesCallback(void *a1, void *a2, void *a3, void *a4) {
	FskFSDirectoryChangeNotifier dirNot = (FskFSDirectoryChangeNotifier)a1;
	char *path = (char*)a2;
	int mask = (int)a3;
	int flags = 0;

	FskAndroidFSNotifierPrintfDebug("[%s] doChangesCallback\n", threadTag(FskThreadGetCurrent()));
	if (path && dirNot->callback) {
		if (mask & (IN_MODIFY | IN_ATTRIB))
			flags = kFskDirectoryChangeFileChanged;
		if (mask & (IN_MOVED_FROM | IN_DELETE_SELF | IN_DELETE))
			flags = kFskDirectoryChangeFileDeleted;
		if (mask & (IN_MOVED_TO | IN_CREATE))
			flags = kFskDirectoryChangeFileCreated;

		(dirNot->callback)(flags, path, dirNot->refCon);
		FskMemPtrDispose(path);
	}
}

static void dirChangeHandler(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon) {
	FskFSDirectoryChangeNotifier walker;
	struct inotify_event *ev;
	char *p, buf[PATH_MAX];
	int l, ret, count = 0;

	FskAndroidFSNotifierPrintfDebug("[%s] dirChangeHandler\n", threadTag(FskThreadGetCurrent()));
	ret = read(iNotifyFD, buf, PATH_MAX);
	FskAndroidFSNotifierPrintfDebug("read %d\n", ret);
	p = buf;
	FskMutexAcquire(gDirChangeNotifiersMutex);
	while (ret > 0) {
		ev = (struct inotify_event *)p;
		FskAndroidFSNotifierPrintfDebug("dirChangeHandler seq(%d): name %s\n", ++count, ev->len ? ev->name : "none");
		l = sizeof(struct inotify_event) + ev->len;
		p += l;
		ret -= l;
		for (walker = gDirectoryChangeNotifiers; walker != NULL; walker = (FskFSDirectoryChangeNotifier)walker->base.next) {
			if (ev->wd == walker->wd)
				break;
		}
		if (walker) {
			char *buffer;
//			if (ev->name && ev->name[0] == '.') {
			if (ev->name[0] == '.') {
				FskAndroidFSNotifierPrintfDebug("ignoring a dot file: %s\n", ev->name);
				continue;
			}
			if (0 == FskStrCompare(ev->name, "out.txt")) {
				continue;
			}
			buffer = FskStrDoCopy(ev->name);
			walker->count += 1;
			FskThreadPostCallback(walker->thread, doChangesCallback, walker, buffer, (void*)(ev->mask), NULL);
		}
	}
	FskMutexRelease(gDirChangeNotifiersMutex);
}


FskErr androidDirectoryChangeNotifierNew(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNtf)
{
	FskErr err;
	FskFSDirectoryChangeNotifier dirNot = NULL;
	int mask;

	FskAndroidFSNotifierPrintfDebug("[%s] FskFSDirectoryChangeNotifierNew (%s)\n", threadTag(FskThreadGetCurrent()), path);
	FskAndroidFSNotifierPrintfDebug("directoryChangeNotifier - adding %s\n", path);

	*dirChangeNtf = NULL;

	if (NULL == gDirChangeNotifiersMutex) {
		err = FskMutexNew(&gDirChangeNotifiersMutex, "gDirChangeNotifiers");
		if (err) return err;

		iNotifyFD = inotify_init();
		if (iNotifyFD < 0) {
			FskAndroidFSNotifierPrintfDebug("inotify_init failure\n");
			err = kFskErrOperationFailed;
			goto bail;
		}

		gDirChangeSource = FskThreadCreateDataSource(iNotifyFD);
		FskThreadAddDataHandler(&gDirChangeHandler, gDirChangeSource, dirChangeHandler, true, false, NULL);
	}

	FskMutexAcquire(gDirChangeNotifiersMutex);

	err = FskMemPtrNewClear(sizeof(FskFSDirectoryChangeNotifierRecord), (FskMemPtr *)&dirNot);
	if (err) goto bail;

	dirNot->base.dispatch.dispatch = &gFSDispatch;
	dirNot->flags = flags;
	dirNot->callback = callback;
	dirNot->refCon = refCon;
	dirNot->thread = FskThreadGetCurrent();
	dirNot->count = 1;

	FskListPrepend((FskList *)&gDirectoryChangeNotifiers, dirNot);
	dirNot->wd = -1;

	err = sCheckFullPath(path, kFskPathIsDirectory);
	if (err) {
		FskAndroidFSNotifierPrintfDebug("path %s is not a directory\n", path);
		goto bail;
	}

	mask = IN_MODIFY | IN_ATTRIB | IN_MOVED_FROM | IN_DELETE_SELF | IN_DELETE | IN_MOVED_TO | IN_CREATE;

	if (0 == (flags & kFskDirectoryChangeMonitorSubTree)) {
		FskAndroidFSNotifierPrintfDebug("[%s] - only watch for changes on directory\n", threadTag(FskThreadGetCurrent()));
		mask |= IN_ONLYDIR;
	}

	dirNot->wd = inotify_add_watch(iNotifyFD, path, mask);

	if (dirNot->wd == -1) {
		FskAndroidFSNotifierPrintfDebug("setting up notify for %s failed: %d\n", path, errno);
		err = kFskErrOperationFailed;
		goto bail;
	}

bail:
	FskMutexRelease(gDirChangeNotifiersMutex);

	if (kFskErrNone != err) {
		androidDirectoryChangeNotifierDispose((FskDirectoryChangeNotifier)dirNot);
		dirNot = NULL;
	}
	*dirChangeNtf = (FskDirectoryChangeNotifier)dirNot;

	return err;
}


FskErr androidDirectoryChangeNotifierDispose(FskDirectoryChangeNotifier dirChangeNtf)
{
	FskFSDirectoryChangeNotifier dirNot = (FskFSDirectoryChangeNotifier)dirChangeNtf;

	if (NULL == dirNot)
		return kFskErrNone;

	FskAndroidFSNotifierPrintfDebug("[%s] androidDirectoryChangeNotifierDispose (%d)\n", threadTag(FskThreadGetCurrent()), dirNot->wd);

	dirNot->callback = NULL;
	dirNot->count -= 1;
	if (dirNot->count > 0)
		return kFskErrNone;

	FskMutexAcquire(gDirChangeNotifiersMutex);
	FskListRemove((FskList*)&gDirectoryChangeNotifiers, dirNot);
	if (dirNot->wd != -1) {
//		ioctl(iNotifyFD, INOTIFY_IGNORE, dirNot->wd);
		inotify_rm_watch(iNotifyFD, dirNot->wd);
	}
	FskMutexRelease(gDirChangeNotifiersMutex);
	FskMemPtrDispose(dirNot);

	return kFskErrNone;
}



void androidiNotifyTerminate() {

	FskAndroidFSNotifierPrintfDebug("androidiNotifyTerminate\n");
	if (NULL != gDirChangeNotifiersMutex) {
		FskMutexAcquire(gDirChangeNotifiersMutex);
		FskThreadRemoveDataHandler(&gDirChangeHandler);
		FskMemPtrDispose(gDirChangeSource);
		gDirChangeSource = NULL;
		if (iNotifyFD > 0)
			close(iNotifyFD);
		iNotifyFD = 0;
		FskMutexRelease(gDirChangeNotifiersMutex);
		FskMutexDispose(gDirChangeNotifiersMutex);
		gDirChangeNotifiersMutex = NULL;
	}
}

