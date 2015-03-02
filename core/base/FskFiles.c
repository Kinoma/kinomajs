/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#define __FSKFILES_PRIV__
#include "FskFiles.h"

#include "FskList.h"
#include "FskFS.h"
#include "FskExtensions.h"

extern FskFileDispatchTableRecord gFSDispatch;

static FskFileDispatchTable findDispatchFromPath(const char *path);

#define getIndFileSystem(i) ((0 == (i)) ? &gFSDispatch : (FskFileDispatchTable)FskExtensionGetByIndex(kFskExtensionFileSystem, (i) - 1))

FskErr FskFileInitialize(void)
{
	UInt32					index = 0;
	FskFileDispatchTable	dispatch;

	for (index = 0; (dispatch = (FskFileDispatchTable)FskExtensionGetByIndex(kFskExtensionFileSystem, index)); index++) {
		if (dispatch->fileInitialize)
			(dispatch->fileInitialize)();
	}

	if (gFSDispatch.fileInitialize)
		(gFSDispatch.fileInitialize)();

	return kFskErrNone;
}

FskErr FskFileTerminate(void)
{
	UInt32					index = 0;
	FskFileDispatchTable	dispatch;

	for (index = 0; (dispatch = (FskFileDispatchTable)FskExtensionGetByIndex(kFskExtensionFileSystem, index)); index++) {
		if (dispatch->fileTerminate)
			(dispatch->fileTerminate)();
	}

	if (gFSDispatch.fileTerminate)
		(gFSDispatch.fileTerminate)();

	return kFskErrNone;
}

#if SUPPORT_INSTRUMENTATION
	#include <stdio.h>

	static Boolean doFormatMessageFile(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gFileTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"file",
		FskInstrumentationOffset(FskFileRecord),
		NULL,
		0,
		NULL,
		doFormatMessageFile
	};
#endif /* SUPPORT_INSTRUMENTATION */

FskErr FskFileOpen(const char *fullPath, UInt32 permissions, FskFile *fref)
{
	FskErr err;
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->fileOpen)
		return kFskErrUnimplemented;

	err = (dispatch->fileOpen)(fullPath, permissions, fref);
	if (kFskErrNone == err) {
		(*fref)->dispatch.dispatch = dispatch;
		(*fref)->dispatch.refcon = NULL;
		FskInstrumentedItemNew(*fref, FskStrDoCopy_Untracked(fullPath), &gFileTypeInstrumentation);
	}
#if SUPPORT_INSTRUMENTATION
	else {
		FskFileInstrMsgFileOpenFailedRecord msg;
		msg.err = err;
		msg.path = (char *)fullPath;
		FskInstrumentedTypeSendMessageNormal(&gFileTypeInstrumentation, kFskFileInstrMsgFileOpenFailed, (void *)&msg);
	}
#endif

	return err;
}

FskErr FskFileClose(FskFile fref)
{
	if (!fref)
		return kFskErrNone;
	if (!fref->dispatch.dispatch->fileClose)
		return kFskErrUnimplemented;
#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemDispose(fref);
	FskMemPtrDispose_Untracked((char *)fref->_instrumented.name);
	fref->_instrumented.name = NULL;
#endif
	return (fref->dispatch.dispatch->fileClose)(fref);
}

FskErr FskFileGetSize(FskFile fref, FskInt64 *size)
{
	if (!fref->dispatch.dispatch->fileGetSize)
		return kFskErrUnimplemented;
	return (fref->dispatch.dispatch->fileGetSize)(fref, size);
}

FskErr FskFileSetSize(FskFile fref, const FskInt64 *size)
{
	if (!fref->dispatch.dispatch->fileSetSize)
		return kFskErrUnimplemented;
	FskInstrumentedItemSendMessageNormal(fref, kFskFileInstrMsgSetSize, (void *)size);
	return (fref->dispatch.dispatch->fileSetSize)(fref, size);
}

FskErr FskFileSetPosition(FskFile fref, const FskInt64 *position)
{
	if (!fref->dispatch.dispatch->fileSetPosition)
		return kFskErrUnimplemented;
	FskInstrumentedItemSendMessageVerbose(fref, kFskFileInstrMsgSetPosition, (void *)position);
	return (fref->dispatch.dispatch->fileSetPosition)(fref, position);
}

FskErr FskFileGetPosition(FskFile fref, FskInt64 *position)
{
	if (!fref->dispatch.dispatch->fileGetPosition)
		return kFskErrUnimplemented;
	return (fref->dispatch.dispatch->fileGetPosition)(fref, position);
}

FskErr FskFileRead(FskFile fref, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead)
{
	FskErr err;

	if (!fref->dispatch.dispatch->fileRead)
		return kFskErrUnimplemented;

	err = (fref->dispatch.dispatch->fileRead)(fref, bytesToRead, buffer, bytesRead);

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListeners(fref)) {
		FskFileInstrMsgReadRecord msg;
		msg.bytesToRead = bytesToRead;
		msg.buffer = buffer;
		msg.bytesRead = bytesRead;
		msg.err = err;
		FskInstrumentedItemSendMessageVerbose(fref, kFskFileInstrMsgRead, &msg);
	}
#endif

	return err;
}

FskErr FskFileWrite(FskFile fref, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten)
{
	FskErr err;

	if (!fref->dispatch.dispatch->fileWrite)
		return kFskErrUnimplemented;

	err = (fref->dispatch.dispatch->fileWrite)(fref, bytesToWrite, buffer, bytesWritten);

#if 0 && SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListeners(fref)) {
		FskFileInstrMsgWriteRecord msg;
		msg.bytesToWrite = bytesToWrite;
		msg.buffer = buffer;
		msg.bytesWritten = bytesWritten;
		msg.err = err;
		FskInstrumentedItemSendMessageVerbose(fref, kFskFileInstrMsgWrite, &msg);
	}
#endif

	return err;
}

FskErr FskFileFlush(FskFile fref)
{
	FskErr err = kFskErrNone;

	if (fref->dispatch.dispatch->fileFlush)
		err = (fref->dispatch.dispatch->fileFlush)(fref);

	return err;
}

FskErr FskFileGetFileInfo(const char *fullPath, FskFileInfo *itemInfo)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->fileGetInfo)
		return kFskErrUnimplemented;
	FskInstrumentedTypeSendMessageNormal(&gFileTypeInstrumentation, kFskFileInstrMsgGetFileInfo, (void *)fullPath);
	return (dispatch->fileGetInfo)(fullPath, itemInfo);
}

FskErr FskFileSetFileInfo(const char *fullPath, const FskFileInfo *itemInfo)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->fileSetInfo)
		return kFskErrUnimplemented;
	return (dispatch->fileSetInfo)(fullPath, itemInfo);
}

FskErr FskFileGetThumbnail(const char *fullPath, const UInt32 width, const UInt32 height, FskBitmap *thumbnail)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->fileGetThumbnail)
		return kFskErrUnimplemented;
	return (dispatch->fileGetThumbnail)(fullPath, width, height, thumbnail);
}

FskErr FskFileCreate(const char *fullPath)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->fileCreate)
		return kFskErrUnimplemented;
	FskInstrumentedTypeSendMessageNormal(&gFileTypeInstrumentation, kFskFileInstrMsgFileCreate, (void *)fullPath);
	return (dispatch->fileCreate)(fullPath);
}

FskErr FskFileCreateDirectory(const char *fullPath)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->dirCreate)
		return kFskErrUnimplemented;
	FskInstrumentedTypeSendMessageNormal(&gFileTypeInstrumentation, kFskFileInstrMsgDirectoryCreate, (void *)fullPath);
	return (dispatch->dirCreate)(fullPath);
}

FskErr FskFileDelete(const char *fullPath)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->fileDelete)
		return kFskErrUnimplemented;
	FskInstrumentedTypeSendMessageNormal(&gFileTypeInstrumentation, kFskFileInstrMsgFileDelete, (void *)fullPath);
	return (dispatch->fileDelete)(fullPath);
}

FskErr FskFileDeleteDirectory(const char *fullPath)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->dirDelete)
		return kFskErrUnimplemented;
	FskInstrumentedTypeSendMessageNormal(&gFileTypeInstrumentation, kFskFileInstrMsgDirectoryDelete, (void *)fullPath);
	return (dispatch->dirDelete)(fullPath);
}

FskErr FskFileRename(const char *fullPath, const char *newName)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->fileRename)
		return kFskErrUnimplemented;
	return (dispatch->fileRename)(fullPath, newName);
}

FskErr FskFileRenameDirectory(const char *fullPath, const char *newName)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->dirRename)
		return kFskErrUnimplemented;
	return (dispatch->dirRename)(fullPath, newName);
}

FskErr FskFilePathToNative(const char *fskPath, char **nativePath)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fskPath);
	if (!dispatch || !dispatch->filePathToNative)
		return kFskErrUnimplemented;
	return (dispatch->filePathToNative)(fskPath, nativePath);
}

FskErr FskFileResolveLink(const char *fullPath, char **resolved)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->fileResolveLink)
		return kFskErrUnimplemented;
	return (dispatch->fileResolveLink)(fullPath, resolved);
}

#if SUPPORT_INSTRUMENTATION
	static Boolean doFormatMessageDirectoryIterator(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gDirectoryIteratorTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"directoryiterator",
		FskInstrumentationOffset(FskDirectoryIteratorRecord),
		NULL,
		0,
		NULL,
		doFormatMessageDirectoryIterator
	};
#endif

FskErr FskDirectoryIteratorNew(const char *directoryPath, FskDirectoryIterator *dirIt, UInt32 flags)
{
	FskErr err;
	FskFileDispatchTable dispatch = findDispatchFromPath(directoryPath);
	if (!dispatch || !dispatch->dirIteratorNew)
		return kFskErrUnimplemented;

	err = (dispatch->dirIteratorNew)(directoryPath, dirIt, flags);
	if (kFskErrNone == err) {
		(*dirIt)->dispatch.dispatch = dispatch;
		(*dirIt)->dispatch.refcon = NULL;
		FskInstrumentedItemNew(*dirIt, FskStrDoCopy_Untracked(directoryPath), &gDirectoryIteratorTypeInstrumentation);
	}

	return err;
}

FskErr FskDirectoryIteratorDispose(FskDirectoryIterator dirIt)
{
	if (NULL == dirIt)
		return kFskErrNone;
	if (!dirIt->dispatch.dispatch->dirIteratorDispose)
		return kFskErrUnimplemented;
#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemDispose(dirIt);
	FskMemPtrDispose_Untracked((char *)dirIt->_instrumented.name);
#endif
	return (dirIt->dispatch.dispatch->dirIteratorDispose)(dirIt);
}

FskErr FskDirectoryIteratorGetNext(FskDirectoryIterator dirIt, char **name, UInt32 *itemType)
{
	FskErr err;

	if (!dirIt->dispatch.dispatch->dirIteratorGetNext)
		return kFskErrUnimplemented;

	err = (dirIt->dispatch.dispatch->dirIteratorGetNext)(dirIt, name, itemType);

	FskInstrumentedItemSendMessageVerbose(dirIt, kFskDirectoryIteratorInstrMsgNext, (kFskErrNone == err) ? (name ? *name : "(anonymous)") : NULL);

	return err;
}

FskErr FskDirectoryGetSpecialPath(UInt32 type, const Boolean create, const char *volumeName, char **fullPath)
{
	FskErr err;
	FskFileDispatchTable dispatch = volumeName ? findDispatchFromPath(volumeName) : &gFSDispatch;
	if (!dispatch || !dispatch->dirGetSpecialPath)
		return kFskErrUnimplemented;
	err = (dispatch->dirGetSpecialPath)(type, create, volumeName, fullPath);
	if (err) return err;

	if (create) {
		FskFileInfo itemInfo;
		err = FskFileGetFileInfo(*fullPath, &itemInfo);	// path has trailing slash
		if (kFskErrFileNotFound == err) {
			char *p = *fullPath + 1;
			while ((p = FskStrChr(p, '/'))) {
				char tmp = *(++p);
				*p = 0;

				err = FskFileGetFileInfo(*fullPath, &itemInfo);	// path has trailing slash
				if (kFskErrFileNotFound == err)
					err = FskFileCreateDirectory(*fullPath);
				BAIL_IF_ERR(err);

				*p = tmp;
			}
		}
	}

bail:
	if (err)
		FskMemPtrDisposeAt((void **)fullPath);

	return err;
}

struct VolumeIteratorIteratorRecord {
	UInt32					fileSystemIndex;
	FskFileDispatchTable	currentDispatch;
	FskVolumeIterator		currentIt;
};
typedef struct VolumeIteratorIteratorRecord VolumeIteratorIteratorRecord;
typedef struct VolumeIteratorIteratorRecord *VolumeIteratorIterator;

FskErr FskVolumeIteratorNew(FskVolumeIterator *volIt)
{
	return FskMemPtrNewClear(sizeof(VolumeIteratorIteratorRecord), (FskMemPtr *)volIt);
}

FskErr FskVolumeIteratorDispose(FskVolumeIterator volIt)
{
	VolumeIteratorIterator itit = (VolumeIteratorIterator)volIt;

	if (NULL == itit)
		return kFskErrNone;

	if (itit->currentDispatch && itit->currentDispatch->volIteratorDispose)
		(itit->currentDispatch->volIteratorDispose)(itit->currentIt);

	FskMemPtrDispose(itit);

	return kFskErrNone;
}

FskErr FskVolumeIteratorGetNext(FskVolumeIterator volIt, UInt32 *id, char **path, char **name)
{
	FskErr err;
	VolumeIteratorIterator itit = (VolumeIteratorIterator)volIt;

	while (true) {
		if (NULL == itit->currentDispatch) {
			itit->currentDispatch = getIndFileSystem(itit->fileSystemIndex);
			itit->fileSystemIndex++;
			if (NULL == itit->currentDispatch) {
				err = kFskErrIteratorComplete;
				break;
			}

			if ((NULL == itit->currentDispatch->volIteratorNew) || (NULL == itit->currentDispatch->volIteratorGetNext)) {
				// this file system does't implement volume iteration
				itit->currentDispatch = NULL;
				continue;
			}

			if (kFskErrNone != (itit->currentDispatch->volIteratorNew)(&itit->currentIt)) {
				// failed to instantiate iterator
				itit->currentDispatch = NULL;
				continue;
			}
		}

		err = (itit->currentDispatch->volIteratorGetNext)(itit->currentIt, id, path, name);
		if (kFskErrNone == err)
			break;

		if (itit->currentDispatch->volIteratorDispose)
			(itit->currentDispatch->volIteratorDispose)(itit->currentIt);
		itit->currentIt = NULL;
		itit->currentDispatch = NULL;
	}

	return err;
}

#if SUPPORT_INSTRUMENTATION
	static FskInstrumentedTypeRecord gVolumeNotifierTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"volumenotifier",
		FskInstrumentationOffset(FskVolumeNotifierRecord),
		NULL
	};
#endif

FskErr FskVolumeNotifierNew(FskVolumeNotifierCallbackProc callback, void *refCon, FskVolumeNotifier *volNtf)
{
	FskErr err = kFskErrNone;
	UInt32 fileSystemIndex = 0;

	*volNtf = NULL;

	while (true) {
		FskFileDispatchTable dispatch = getIndFileSystem(fileSystemIndex);
		FskVolumeNotifier thisVolNtf;
		fileSystemIndex++;

		if (NULL == dispatch)
			break;

		if (NULL == dispatch->volNotifierNew)
			continue;

		err = (dispatch->volNotifierNew)(callback, refCon, &thisVolNtf);
		BAIL_IF_ERR(err);

		thisVolNtf->nextNotifier = NULL;
		FskListAppend((FskList *)volNtf, thisVolNtf);
	}

	FskInstrumentedItemNew(*volNtf, NULL, &gVolumeNotifierTypeInstrumentation);

bail:
	if (kFskErrNone != err) {
		FskVolumeNotifierDispose(*volNtf);
		*volNtf = NULL;
	}

	return err;
}

FskErr FskVolumeNotifierDispose(FskVolumeNotifier volNtf)
{
#if SUPPORT_INSTRUMENTATION
	if (NULL != volNtf)
		FskInstrumentedItemDispose(volNtf);
#endif

	while (NULL != volNtf) {
		FskVolumeNotifier nextNotifier = volNtf->nextNotifier;

		if (NULL != volNtf->dispatch.dispatch->volNotifierDispose)
			(volNtf->dispatch.dispatch->volNotifierDispose)(volNtf);

		volNtf = nextNotifier;
	}

	return kFskErrNone;
}

FskErr FskVolumeGetInfo(UInt32 volumeID, char **path, char **name, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace)
{
	FskErr err = kFskErrUnimplemented;
	UInt32 i = 0;
	FskFileDispatchTable dispatch = NULL;

	for (i=0; true; i++) {
		dispatch = getIndFileSystem(i);
		if (NULL == dispatch)
			break;

		if (dispatch->volGetInfo) {
			FskErr tempErr = (dispatch->volGetInfo)(volumeID, path, name, volumeType, isRemovable, capacity, freeSpace);
			if (tempErr == kFskErrNone)
				return kFskErrNone;
			if (kFskErrUnimplemented != tempErr)
				err = tempErr;
		}
	}
	return err;
}

FskErr FskVolumeGetInfoFromPath(const char *pathIn, char **pathOut, char **name, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(pathIn);
	if (NULL == dispatch)
		return kFskErrUnimplemented;
	if (!dispatch->volGetInfoFromPath) {
		UInt32 volumeID;
		FskErr err = FskVolumeGetID(pathIn, &volumeID);
		if (err) return err;

		return FskVolumeGetInfo(volumeID, pathOut, name, volumeType, isRemovable, capacity, freeSpace);
	}
	return (dispatch->volGetInfoFromPath)(pathIn, pathOut, name, volumeType, isRemovable, capacity, freeSpace);
}

FskErr FskVolumeGetDeviceInfo(UInt32 volumeID, char **vendor, char **product, char **revision, char **vendorSpecific)
{
	FskFileDispatchTable dispatch = &gFSDispatch;
	if (!dispatch->volGetDeviceInfo)
		return kFskErrUnimplemented;
	return (dispatch->volGetDeviceInfo)(volumeID, vendor, product, revision, vendorSpecific);
}

FskErr FskVolumeGetID(const char *fullPath, UInt32 *volumeID)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->volGetID)
		return kFskErrUnimplemented;
	return (dispatch->volGetID)(fullPath, volumeID);
}

FskErr FskVolumeEject(const char *fullPath, UInt32 flags)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch || !dispatch->volEject)
		return kFskErrUnimplemented;
	return (dispatch->volEject)(fullPath, flags);
}

/*
	File mapping
*/

#if SUPPORT_INSTRUMENTATION
	static FskInstrumentedTypeRecord gFileMappingTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"filemapping",
		FskInstrumentationOffset(FskFileMappingRecord),
		NULL
	};
#endif

FskErr FskFileMap(const char *fullPath, unsigned char **data, FskInt64 *dataSize, UInt32 flags, FskFileMapping *map)
{
	FskErr err = kFskErrUnimplemented;
	FskFileDispatchTable dispatch = findDispatchFromPath(fullPath);
	if (!dispatch)
		return err;

	if (dispatch->fileMap) {
		err = (dispatch->fileMap)(fullPath, data, dataSize, map);
		if (kFskErrNone == err) {
			(*map)->dispatch.dispatch = dispatch;
			(*map)->dispatch.refcon = NULL;
			FskInstrumentedItemNew(*map, FskStrDoCopy_Untracked(fullPath), &gFileMappingTypeInstrumentation);
		}
	}
	if (kFskErrNone != err) {
		if (kFskFileMapDoNotEmulate & flags)
			err = kFskErrUnimplemented;
		else {
			err = FskFileLoad(fullPath, data, dataSize);
			if (kFskErrNone == err) {
				FskMemPtrNewClear(sizeof(FskFileMappingRecord), (FskMemPtr *)map);
				(*map)->address = *data;
				FskInstrumentedItemNew(*map, FskStrDoCopy_Untracked(fullPath), &gFileMappingTypeInstrumentation);
			}
		}
	}

	return err;
}

FskErr FskFileDisposeMap(FskFileMapping map)
{
	if (!map)
		return kFskErrNone;
#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemDispose(map);
	FskMemPtrDispose_Untracked((char *)map->_instrumented.name);
#endif
	if (!map->dispatch.dispatch) {
		FskMemPtrDispose(map->address);
		FskMemPtrDispose(map);
		return kFskErrNone;
	}
	if (!map->dispatch.dispatch->fileDisposeMap)
		return kFskErrUnimplemented;
	return (map->dispatch.dispatch->fileDisposeMap)(map);
}

/*
	user interface stuff
*/

FskErr FskFileChoose(const FskFileChooseEntry types, const char *prompt, Boolean allowMultiple, const char *initialPath, char **files)
{
	FskFileDispatchTable dispatch = initialPath ? findDispatchFromPath(initialPath) : &gFSDispatch;
	if (!dispatch || !dispatch->fileChoose)
		return kFskErrUnimplemented;
	return (dispatch->fileChoose)(types, prompt, allowMultiple, initialPath, files);
}

FskErr FskFileChooseSave(const char *defaultName, const char *prompt, const char *initialDirectory, char **file)
{
	FskFileDispatchTable dispatch = initialDirectory ? findDispatchFromPath(initialDirectory) : &gFSDispatch;
	if (!dispatch || !dispatch->fileChooseSave)
		return kFskErrUnimplemented;
	return (dispatch->fileChooseSave)(defaultName, prompt, initialDirectory, file);
}

FskErr FskDirectoryChoose(const char *prompt, const char *initialPath, char **path)
{
	FskFileDispatchTable dispatch = initialPath ? findDispatchFromPath(initialPath) : &gFSDispatch;
	if (!dispatch || !dispatch->directoryChoose)
		return kFskErrUnimplemented;
	return (dispatch->directoryChoose)(prompt, initialPath, path);
}

/*
	Directory change notifier
*/


#if SUPPORT_INSTRUMENTATION
	static Boolean doFormatMessageDirectoryChange(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gDirectoryChangeTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"directorychange",
		FskInstrumentationOffset(FskDirectoryChangeNotifierRecord),
		NULL,
		0,
		NULL,
		doFormatMessageDirectoryChange
	};
#endif


FskErr FskDirectoryChangeNotifierNew(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNtf)
{
	FskFileDispatchTable dispatch = findDispatchFromPath(path);
	FskErr err;

	if (!dispatch || !dispatch->dirChangeNotifyNew)
		return kFskErrUnimplemented;

	err = (dispatch->dirChangeNotifyNew)(path, flags, callback, refCon, dirChangeNtf);
#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemNew(*dirChangeNtf, FskStrDoCopy_Untracked(path), &gDirectoryChangeTypeInstrumentation);
#endif

	return err;
}

FskErr FskDirectoryChangeNotifierDispose(FskDirectoryChangeNotifier dirChangeNtf)
{
	if (!dirChangeNtf)
		return kFskErrNone;

#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemDispose(dirChangeNtf);
	FskMemPtrDispose_Untracked((char *)dirChangeNtf->_instrumented.name);
	dirChangeNtf->_instrumented.name = NULL;
#endif

	return (dirChangeNtf->dispatch.dispatch->dirChangeNotifyDispose)(dirChangeNtf);
}


/*
	Helper functions built on general purpose file utilities
*/

FskErr FskFileLoad(const char *filePath, FskMemPtr *file, FskInt64 *fileSize)
{
	FskErr err;
	FskFile fref = NULL;
	FskInt64 eof = 0;

	err = FskFileOpen(filePath, kFskFilePermissionReadOnly, &fref);
	BAIL_IF_ERR(err);

	FskFileGetSize(fref, &eof);
	err = FskMemPtrNew((UInt32)eof + 1, file);
	BAIL_IF_ERR(err);

	((char *)*file)[eof] = 0;		// always null terminate the file

	err = FskFileRead(fref, (UInt32)eof, *file, NULL);
	BAIL_IF_ERR(err);

bail:
	if (fileSize)
		*fileSize = eof;

	FskFileClose(fref);

	return err;
}

FskErr FskDirectoryCountItems(const char *directoryPath, UInt32 *files, UInt32 *directories)
{
	FskErr err;
	FskDirectoryIterator di = NULL;
	UInt32 itemType;

	if (NULL != directories)
		*directories = 0;
	if (NULL != files)
		*files = 0;

	err = FskDirectoryIteratorNew(directoryPath, &di, 0);
	BAIL_IF_ERR(err);

	while (kFskErrNone == FskDirectoryIteratorGetNext(di, NULL, &itemType)) {
		if ((NULL != directories) && (kFskDirectoryItemIsDirectory == itemType))
			*directories += 1;
		else
		if ((NULL != files) && (kFskDirectoryItemIsFile == itemType))
			*files += 1;
	}

bail:
	FskDirectoryIteratorDispose(di);

	return err;
}

/*
	resolve path to dispatcher
*/

FskFileDispatchTable findDispatchFromPath(const char *path)
{
	UInt32 i = 0;
	SInt32 bestPriority = -1;
	FskFileDispatchTable bestDispatch = NULL;

	for (i=0; true; i++) {
		SInt32 thisPriority;
		FskFileDispatchTable dispatch = getIndFileSystem(i);
		if (NULL == dispatch)
			break;

		if (kFskErrNone == dispatch->fileFilter(path, &thisPriority)) {
			if (thisPriority > bestPriority) {
				bestPriority = thisPriority;
				bestDispatch = dispatch;
			}
		}
	}

	return bestDispatch;
}

#if SUPPORT_INSTRUMENTATION

Boolean doFormatMessageFile(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kFskFileInstrMsgRead: {
			FskFileInstrMsgReadRecord *d = (FskFileInstrMsgReadRecord *)msgData;
			if (kFskErrNone == d->err)
				snprintf(buffer, bufferSize, "read %lu", d->bytesToRead);
			else
				snprintf(buffer, bufferSize, "read %lu failed, err=%s", d->bytesToRead, FskInstrumentationGetErrorString(d->err));
			}
		   return true;

		case kFskFileInstrMsgWrite: {
			FskFileInstrMsgWriteRecord *d = (FskFileInstrMsgWriteRecord *)msgData;
			if (kFskErrNone == d->err)
				snprintf(buffer, bufferSize, "write %lu", d->bytesToWrite);
			else
				snprintf(buffer, bufferSize, "write %lu failed, err=%s", d->bytesToWrite, FskInstrumentationGetErrorString(d->err));
			}
		   return true;

		case kFskFileInstrMsgSetPosition:
			snprintf(buffer, bufferSize, "setposition %lu", (UInt32)*(FskInt64 *)msgData);
			return true;

		case kFskFileInstrMsgSetSize:
			snprintf(buffer, bufferSize, "setsize %lu", (UInt32)*(FskInt64 *)msgData);
			return true;

		case kFskFileInstrMsgFileDelete:
			snprintf(buffer, bufferSize, "delete file %s", (char *)msgData);
			return true;

		case kFskFileInstrMsgDirectoryDelete:
			snprintf(buffer, bufferSize, "delete directory %s", (char *)msgData);
			return true;

		case kFskFileInstrMsgFileCreate:
			snprintf(buffer, bufferSize, "create file %s", (char *)msgData);
			return true;

		case kFskFileInstrMsgDirectoryCreate:
			snprintf(buffer, bufferSize, "create directory %s", (char *)msgData);
			return true;

		case kFskFileInstrMsgGetFileInfo:
			snprintf(buffer, bufferSize, "get file info %s", (char *)msgData);
			return true;

		case kFskFileInstrMsgFileOpenFailed: {
			FskFileInstrMsgFileOpenFailedRecord *d = (FskFileInstrMsgFileOpenFailedRecord *)msgData;
			snprintf(buffer, bufferSize, "open failed, err = %ld, path = %s", (long)d->err, d->path);
			}
			return true;

		}

	return false;
}


static Boolean doFormatMessageDirectoryIterator(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kFskDirectoryIteratorInstrMsgNext:
			if (msgData)
				snprintf(buffer, bufferSize, "next=%s", (char*)msgData);
			else
				snprintf(buffer, bufferSize, "complete");
			return true;
	}

	return false;
}

static Boolean doFormatMessageDirectoryChange(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kFskDirectoryChangeInstrMsgChanged: {
			FskDirectoryChangeInstrMsgChangeRecord *change = msgData;

			snprintf(buffer, bufferSize, "change, flags=%lu, path=%s", change->flags, change->path);
			}
			return true;
	}

	return false;

}

#endif
