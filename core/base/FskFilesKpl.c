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
#define __FSKFSFILES_PRIV__
#define __FSKFILES_PRIV__
#define __FSKBITMAP_PRIV__
#include "Fsk.h"
#include "FskFS.h"

#include "FskErrors.h"
#include "KplFiles.h"

struct FskFSDirectoryChangeNotifierRecord {
	FskDirectoryChangeNotifierRecord				base;

	KplDirectoryChangeNotifier						kplDirChangeNot;
};
typedef struct FskFSDirectoryChangeNotifierRecord FskFSDirectoryChangeNotifierRecord;
typedef struct FskFSDirectoryChangeNotifierRecord *FskFSDirectoryChangeNotifier;

static FskErr FskFSFileFilter(const char *fullPath, SInt32 *priority);
static FskErr FskKplVolumeNotifierCallback(UInt32 status, UInt32 volumeID, void *refCon);

FskFileDispatchTableRecord gFSDispatch = {
	0,
	FskFSFileFilter,

	(FskFileOpenProc)FskFSFileOpen,
	(FskFileCloseProc)FskFSFileClose,
	(FskFileGetSizeProc)FskFSFileGetSize,
	(FskFileSetSizeProc)FskFSFileSetSize,
	(FskFileSetPositionProc)FskFSFileSetPosition,
	(FskFileGetPositionProc)FskFSFileGetPosition,
	(FskFileReadProc)FskFSFileRead,
	(FskFileWriteProc)FskFSFileWrite,
	(FskFileGetFileInfoProc)FskFSFileGetFileInfo,
	(FskFileSetFileInfoProc)FskFSFileSetFileInfo,
	(FskFileGetThumbnailProc)FskFSFileGetThumbnail,
	(FskFileCreateProc)FskFSFileCreate,
	(FskFileDeleteProc)FskFSFileDelete,
	(FskFileRenameProc)FskFSFileRename,
	(FskFilePathToNativeProc)FskFSFilePathToNative,

	(FskFileCreateDirectoryProc)FskFSFileCreateDirectory,
	(FskFileDeleteDirectoryProc)FskFSFileDeleteDirectory,
	(FskFileRenameDirectoryProc)FskFSFileRenameDirectory,
	(FskDirectoryIteratorNewProc)FskFSDirectoryIteratorNew,
	(FskDirectoryIteratorDisposeProc)FskFSDirectoryIteratorDispose,
	(FskDirectoryIteratorGetNextProc)FskFSDirectoryIteratorGetNext,
	(FskDirectoryGetSpecialPathProc)FskFSDirectoryGetSpecialPath,

	(FskVolumeIteratorNewProc)FskFSVolumeIteratorNew,
	(FskVolumeIteratorDisposeProc)FskFSVolumeIteratorDispose,
	(FskVolumeIteratorGetNextProc)FskFSVolumeIteratorGetNext,

	(FskVolumeNotifierNewProc)FskFSVolumeNotifierNew,
	(FskVolumeNotifierDisposeProc)FskFSVolumeNotifierDispose,

	(FskVolumeGetInfoProc)FskFSVolumeGetInfo,
	(FskVolumeGetInfoFromPathProc)FskFSVolumeGetInfoFromPath,
	(FskVolumeGetDeviceInfoProc)FskFSVolumeGetDeviceInfo,
	(FskVolumeGetIDProc)FskFSVolumeGetID,
	(FskVolumeEjectProc)FskFSVolumeEject,

	(FskFileMapProc)FskFSFileMap,
	(FskFileDisposeMapProc)FskFSFileDisposeMap,

	(FskFileChooseProc)NULL,
	(FskFileChooseSaveProc)NULL,
	(FskDirectoryChooseProc)NULL,

	(FskFileTerminateProc)FskFSFileTerminate,

	(FskDirectoryChangeNotifierNewProc)FskFSDirectoryChangeNotifierNew,
	(FskDirectoryChangeNotifierDisposeProc)FskFSDirectoryChangeNotifierDispose,

	(FskFileInitializeProc)FskFSFileInitialize,
	(FskFileResolveLinkProc)FskFSFileResolveLink,

	(FskFileFlushProc)FskFSFileFlush
};

static FskMutex gVolNotifiersMutex = NULL;
static FskList gVolNotifiers = NULL;

FskErr FskFSFileFilter(const char *fullPath, SInt32 *priority)
{
	*priority = 0;
	return kFskErrNone;
}

/*
	Files
*/

FskErr FskFSFileInitialize()
{
	FskErr err;

	err = KplFileInitialize();

	return err;
}

FskErr FskFSFileTerminate()
{
	FskErr err;

	err = KplFileTerminate();

	return err;
}

FskErr FskFSFileOpen(const char *fullPath, UInt32 permissions, FskFSFile *frefOut)
{
	FskErr err = kFskErrNone;
	FskFSFile fref = NULL;
	KplFile kplFile = NULL;

	err = FskMemPtrNewClear(sizeof(FskFSFileRecord), (FskMemPtr *)&fref);
	BAIL_IF_ERR(err);

	err = KplFileOpen(fullPath, permissions, &kplFile);
	BAIL_IF_ERR(err);

	fref->kplFile = kplFile;
	fref->thePermissions = permissions;

bail:
	if (0 != err) {
		FskMemPtrDispose(fref);
		fref = NULL;
	}
	
	*frefOut = fref;

	return err;
}

FskErr FskFSFileClose(FskFSFile fref)
{
	if (fref) {
		KplFileClose(fref->kplFile);
		FskMemPtrDispose(fref);
	}

	return kFskErrNone;
}

FskErr FskFSFileGetSize(FskFSFile fref, FskInt64 *size)
{
	FskErr	err;

	err = KplFileGetSize(fref->kplFile, size);

	return err;
}

FskErr FskFSFileSetSize(FskFSFile fref, const FskInt64 *size)
{
	FskErr err;

	if (fref->thePermissions == kFskFilePermissionReadOnly)
		return kFskErrReadOnly; 

	err = KplFileSetSize(fref->kplFile, (const KplInt64 *)size);

	return err;
}

FskErr FskFSFileSetPosition(FskFSFile fref, const FskInt64 *position)
{
	FskErr err;

	err = KplFileSetPosition(fref->kplFile, (const KplInt64 *)position);

	return err;
}

FskErr FskFSFileGetPosition(FskFSFile fref, FskInt64 *position)
{
	FskErr err;

	err = KplFileGetPosition(fref->kplFile, (KplInt64 *)position);

	return err;
}

FskErr FskFSFileRead(FskFSFile fref, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead)
{
	FskErr	err;
	UInt32 bytesReadW;

	if (NULL != bytesRead)
		*bytesRead = 0;

	err = KplFileRead(fref->kplFile, bytesToRead, buffer, &bytesReadW);
	BAIL_IF_ERR(err);

	if (NULL == bytesRead) {
		if (bytesReadW != bytesToRead)
			return kFskErrOperationFailed;
	}
	else {
		*bytesRead = bytesReadW;
	}

	if ((0 == bytesReadW) && (0 != bytesToRead))
		return kFskErrEndOfFile;

bail:
	return err;
}

FskErr FskFSFileWrite(FskFSFile fref, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten)
{
	FskErr	err;
	UInt32 bytesWrittenW;

	if (fref->thePermissions == kFskFilePermissionReadOnly)
		return kFskErrReadOnly; 

	if (NULL != bytesWritten)
		*bytesWritten = 0;

	err = KplFileWrite(fref->kplFile, bytesToWrite, buffer, &bytesWrittenW);
	BAIL_IF_ERR(err);

	if (NULL == bytesWritten) {
		if (bytesToWrite != bytesWrittenW)
			return kFskErrOperationFailed;
	}
	else
		*bytesWritten = bytesWrittenW;

bail:
	return err;
}

FskErr FskFSFileGetFileInfo(const char *fullpath, FskFileInfo *itemInfo)
{
	FskErr err;

	err = KplFileGetFileInfo(fullpath, (KplFileInfo*)itemInfo);

	return err;
}

FskErr FskFSFileSetFileInfo(const char *fullpath, const FskFileInfo *itemInfo)
{
	FskErr err;

	err = KplFileSetFileInfo(fullpath, (const KplFileInfo*)itemInfo);

	return err;
}

FskErr FskFSFileGetThumbnail(const char *fullPath, const UInt32 width, const UInt32 height, FskBitmap *thumbnail)
{
	FskErr err;

	err = KplFileGetThumbnail(fullPath, width, height, (KplBitmap*)thumbnail);

	return err;
}

FskErr FskFSFileDelete(const char *fullPath)
{
	FskErr err;

	err = KplFileDelete(fullPath);

	return err;
}

FskErr FskFSFileRename(const char *fullPath, const char *newName)
{
	FskErr err;

	err = KplFileRename(fullPath, newName);

	return err;
}

FskErr FskFSFilePathToNative(const char *fskPath, char **nativePath)
{
	FskErr err;

	err = KplFilePathToNative(fskPath, nativePath);

	return err;
}

FskErr FskFSFileResolveLink(const char *fullPath, char **resolved)
{
	FskErr err;

	err = KplFileResolveLink(fullPath, resolved);

	return err;
}

FskErr FskFSFileFlush(FskFSFile fref)
{
	FskErr err;

	err = KplFileFlush(fref->kplFile);

	return err;
}

FskErr FskFSFileCreate(const char *fullPath)
{
	FskErr err;

	err = KplFileCreate(fullPath);

	return err;
}

FskErr FskFSFileCreateDirectory(const char *fullPath)
{
	FskErr err;

	err = KplFileCreateDirectory(fullPath);

	return err;
}

FskErr FskFSFileRenameDirectory(const char *fullPath, const char *newNameIn)
{
	FskErr err;

	err = KplFileRenameDirectory(fullPath, newNameIn);

	return err;
}

FskErr FskFSFileDeleteDirectory(const char *fullPath)
{
	FskErr err;

	err = KplFileDeleteDirectory(fullPath);

	return err;
}

FskErr FskFSVolumeIteratorNew(FskFSVolumeIterator *volIt)
{
	FskErr err;
	FskFSVolumeIterator vi;

	err = FskMemPtrNewClear(sizeof(FskFSVolumeIteratorRecord), (FskMemPtr *)&vi);
	BAIL_IF_ERR(err);

	err = KplVolumeIteratorNew((KplVolumeIterator*)&vi->iterator);

bail:
	if (kFskErrNone != err) {
		FskFSVolumeIteratorDispose(vi);
		vi = NULL;
	}
	*volIt = vi;

	return err;
}

FskErr FskFSVolumeIteratorDispose(FskFSVolumeIterator volIt)
{
	if (volIt) {
		KplVolumeIteratorDispose((KplVolumeIterator)volIt->iterator);
		FskMemPtrDispose(volIt);
	}

	return kFskErrNone;
}

FskErr FskFSVolumeIteratorGetNext(FskFSVolumeIterator volIt, UInt32 *id, char **pathOut, char **nameOut)
{
	FskErr err;

	err = KplVolumeIteratorGetNext((KplVolumeIterator)volIt->iterator, id, pathOut, nameOut);

	return err;
}

FskErr FskFSDirectoryIteratorNew(const char *directoryPath, FskFSDirectoryIterator *dirIt, UInt32 flags)
{
	FskErr err;
	FskFSDirectoryIterator di = NULL;

	err = FskMemPtrNewClear(sizeof(FskFSDirectoryIteratorRecord), (FskMemPtr*)&di);
	BAIL_IF_ERR(err);
	
	err = KplDirectoryIteratorNew(directoryPath, flags, (KplDirectoryIterator*)&di->iterator);
	BAIL_IF_ERR(err);
	
	di->flags = flags;
	
bail:
	if (err) {
		FskFSDirectoryIteratorDispose(di);
		di = NULL;
	}
	*dirIt = di;
	
	return err;
}

FskErr FskFSDirectoryIteratorDispose(FskFSDirectoryIterator dirIt)
{
	KplDirectoryIteratorDispose((KplDirectoryIterator)dirIt->iterator);
	FskMemPtrDispose(dirIt);

	return kFskErrNone;
}

FskErr FskFSDirectoryIteratorGetNext(FskFSDirectoryIterator dirIt, char **name, UInt32 *itemType)
{
	FskErr err;

	if (NULL != name)
		*name = NULL;
	if (NULL != itemType)
		*itemType = 0;

	err = KplDirectoryIteratorGetNext((KplDirectoryIterator)dirIt->iterator, name, itemType);

	return err;
}

FskErr FskFSVolumeEject(const char *fullPath, UInt32 flags)
{
	FskErr err;

	err = KplVolumeEject(fullPath, flags);

// need to map the error here
	return err;
}

FskErr FskFSFileMap(const char *fullPath, unsigned char **data, FskInt64 *dataSize, FskFSFileMapping *mapOut)
{
	FskErr err;
	FskFSFileMapping map = NULL;

	err = FskMemPtrNewClear(sizeof(FskFSFileMappingRecord), (FskMemPtr *)&map);
	BAIL_IF_ERR(err);

	err = KplFileMap(fullPath, data, (KplInt64*)dataSize, (KplFileMapping*)&map->mapping);

bail:
	if (kFskErrNone != err) {
		FskFSFileDisposeMap(map);
		map = NULL;
	}

	*mapOut = map;

	return err;
}

FskErr FskFSFileDisposeMap(FskFSFileMapping map)
{
	if (NULL != map) {
		KplFileDisposeMap((KplFileMapping)map->mapping);
		FskMemPtrDispose(map);
	}

	return kFskErrNone;
}

FskErr FskFSDirectoryGetSpecialPath(UInt32 type, const Boolean create, const char *volumeName, char **fullPath)
{
	FskErr err;

	err = KplDirectoryGetSpecialPath(type, create, volumeName, fullPath);

	return err;
}

FskErr FskFSVolumeGetInfo(UInt32 volumeID, char **pathOut, char **nameOut, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace)
{
	FskErr err;

	err = KplVolumeGetInfo(volumeID, pathOut, nameOut, volumeType, isRemovable, (KplInt64*)capacity, (KplInt64*)freeSpace);

	return err;
}

FskErr FskFSVolumeGetDeviceInfo(UInt32 volumeID, char **vendor, char **product, char **revision, char **vendorSpecific) {
	FskErr err;

	err = KplVolumeGetDeviceInfo(volumeID, vendor, product, revision, vendorSpecific);
	return err;
}

FskErr FskFSVolumeGetInfoFromPath(const char *pathIn, char **pathOut, char **nameOut, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace)
{
	FskErr err;

	err = KplVolumeGetInfoFromPath(pathIn, pathOut, nameOut, volumeType, isRemovable, (KplInt64*)capacity, (KplInt64*)freeSpace);

	return err;
}

FskErr FskFSVolumeGetID(const char *fullPath, UInt32 *volumeID)
{
	FskErr err;

	err = KplVolumeGetID(fullPath, volumeID);

	return err;
}

FskErr FskFSDirectoryChangeNotifierNew(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNtf)
{
	FskErr err;
	FskFSDirectoryChangeNotifier dirNot = NULL;

	*dirChangeNtf = NULL;

	err = FskMemPtrNewClear(sizeof(FskFSDirectoryChangeNotifierRecord), (FskMemPtr *)&dirNot);
	BAIL_IF_ERR(err);

	dirNot->base.dispatch.dispatch = &gFSDispatch;

	err = KplDirectoryChangeNotifierNew(path, flags, (KplDirectoryChangeNotifierCallbackProc)callback, refCon, (KplDirectoryChangeNotifier*)&dirNot->kplDirChangeNot);

bail:
	if (kFskErrNone != err) {
		FskFSDirectoryChangeNotifierDispose((FskDirectoryChangeNotifier)dirNot);
		dirNot = NULL;
	}

	*dirChangeNtf = (FskDirectoryChangeNotifier)dirNot;

	return err;
}

FskErr FskFSDirectoryChangeNotifierDispose(FskDirectoryChangeNotifier dirChangeNtf)
{
	FskErr err;
	FskFSDirectoryChangeNotifier dirNot = (FskFSDirectoryChangeNotifier)dirChangeNtf;

	if (NULL == dirNot)
		return kFskErrNone;

	err = KplDirectoryChangeNotifierDispose(dirNot->kplDirChangeNot);
	if (kFskErrNone != err)
		FskMemPtrDispose(dirNot);

	return kFskErrNone;
}

FskErr FskFSVolumeNotifierNew(FskVolumeNotifierCallbackProc callback, void *refCon, FskFSVolumeNotifier *volNtfOut)
{
	FskErr err;
	FskFSVolumeNotifier volNtf = NULL;

	if (NULL == gVolNotifiersMutex) {
		err = FskMutexNew(&gVolNotifiersMutex, "VolNotifiersMutex");
		BAIL_IF_ERR(err);
	}

	err = FskMemPtrNewClear(sizeof(FskFSVolumeNotifierRecord), (FskMemPtr *)&volNtf);
	BAIL_IF_ERR(err);

	err = KplVolumeNotifierNew(FskKplVolumeNotifierCallback, volNtf, (KplVolumeNotifier *)&volNtf->notifier);
	BAIL_IF_ERR(err);

	FskMutexAcquire(gVolNotifiersMutex);

	volNtf->dispatch.dispatch = &gFSDispatch;
	volNtf->dispatch.refcon = NULL;
	volNtf->callback = callback;
	volNtf->refCon = refCon;
	volNtf->callbackThread = FskThreadGetCurrent();
	FskListPrepend(&gVolNotifiers, &volNtf->next);

	FskMutexRelease(gVolNotifiersMutex);

bail:
	if (err) {
		FskMemPtrDispose(volNtf);
		volNtf = NULL;
	}
	*volNtfOut = volNtf;

	return err;
}

FskErr FskFSVolumeNotifierDispose(FskFSVolumeNotifier volNtf)
{
	FskMutexAcquire(gVolNotifiersMutex);

	FskListRemove(&gVolNotifiers, &volNtf->next);

	KplVolumeNotifierDispose(volNtf->notifier);
	
	FskMemPtrDispose(volNtf);

	FskMutexRelease(gVolNotifiersMutex);

	return kFskErrNone;
}

FskErr FskKplVolumeNotifierCallback(UInt32 status, UInt32 volumeID, void *refCon)
{
	FskFSVolumeNotifier volNtf = (FskFSVolumeNotifier)refCon;
	
	(volNtf->callback)(status, volumeID, volNtf->refCon);

	return kFskErrNone;
}
