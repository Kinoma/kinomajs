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
#ifndef __KPL_FILES_H__
#define __KPL_FILES_H__

#include "Kpl.h"
#include "KplBitmap.h"
#include "FskErrors.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	kKplFilePermissionReadOnly = 0,
	kKplFilePermissionReadWrite = 1 << 0,
};

enum {
	kKplFileFlagFileLocked = 1L << 1,
	kKplFileFlagFileHidden = 1L << 2
};

typedef struct KplFileInfo {
	KplInt64	filesize;
	UInt32		filetype;
	UInt32		fileCreationDate;		// UTC
	UInt32		fileModificationDate;	// UTC
	UInt32		flags;
	KplInt64	fileNode;
} KplFileInfo;

enum {
	kKplDirectoryItemIsFile = 1,
	kKplDirectoryItemIsDirectory = 2,
	kKplDirectoryItemIsLink = 3
};

typedef FskErr (*KplDirectoryChangeNotifierCallbackProc)(UInt32 whatChanged, const char *path, void *refCon);

enum {
	kKplDirectoryChangeFileUnknown = 0,
	kKplDirectoryChangeFileCreated = 1,
	kKplDirectoryChangeFileDeleted = 2,
	kKplDirectoryChangeFileChanged = 4
};

typedef FskErr (*KplVolumeNotifierCallbackProc)(UInt32 whatChanged, UInt32 volumeID, void *refCon);

enum {
	kKplVolumeAdded = 1,
	kKplVolumeRemoved
};

enum {
	kKplVolumeTypeNone,
	kKplVolumeTypeUnknown,
	kKplVolumeTypeFixed,
	kKplVolumeTypeFD,
	kKplVolumeTypeOptical,
	kKplVolumeTypeCD,
	kKplVolumeTypeDVD,
	kKplVolumeTypeNetwork,
	kKplVolumeTypeMemoryStick,
	kKplVolumeTypeMMC,
	kKplVolumeTypeSDMemory,
	kKplVolumeTypeCompactFlash,
	kKplVolumeTypeSmartMedia,
	kKplVolumeTypeDirectory
};

enum {
	kKplDirectorySpecialTypeDocument,
	kKplDirectorySpecialTypePhoto,
	kKplDirectorySpecialTypeMusic,
	kKplDirectorySpecialTypeVideo,
	kKplDirectorySpecialTypeTV,
	kKplDirectorySpecialTypeApplicationPreference,
	kKplDirectorySpecialTypeApplicationPreferenceRoot,
	kKplDirectorySpecialTypeTemporary,
	
	kKplDirectorySpecialTypeSharedFlag = 0x80000000			// flag indicating that request is for folder shared across all users
};
	
KplDeclarePrivateType(KplFile)
KplDeclarePrivateType(KplVolumeIterator)
KplDeclarePrivateType(KplDirectoryIterator)
KplDeclarePrivateType(KplDirectoryChangeNotifier)
KplDeclarePrivateType(KplVolumeNotifier)
KplDeclarePrivateType(KplFileMapping)

FskErr KplFileInitialize(void);
FskErr KplFileTerminate(void);

FskErr KplFileOpen(const char *fullPath, UInt32 permissions, KplFile *fref);
FskErr KplFileClose(KplFile fref);

FskErr KplFileGetSize(KplFile fref, KplInt64 *size);
FskErr KplFileSetSize(KplFile fref, const KplInt64 *size);

FskErr KplFileSetPosition(KplFile fref, const KplInt64 *position);
FskErr KplFileGetPosition(KplFile fref, KplInt64 *position);

FskErr KplFileRead(KplFile fref, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead);
FskErr KplFileWrite(KplFile fref, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten);
FskErr KplFileFlush(KplFile fref);

FskErr KplFileGetFileInfo(const char *fullpath, KplFileInfo *itemInfo);
FskErr KplFileSetFileInfo(const char *fullpath, const KplFileInfo *itemInfo);

FskErr KplFileGetThumbnail(const char *fullPath, const UInt32 width, const UInt32 height, KplBitmap *thumbnail);

FskErr KplFileCreate(const char *fullPath);
FskErr KplFileCreateDirectory(const char *fullPath);

FskErr KplFileDelete(const char *fullPath);
FskErr KplFileDeleteDirectory(const char *fullPath);

FskErr KplFileRename(const char *fullPath, const char *newName);
FskErr KplFileRenameDirectory(const char *fullPath, const char *newName);

FskErr KplFilePathToNative(const char *fskPath, char **nativePath);

FskErr KplFileResolveLink(const char *path, char **resolved);

/*
	Directory Iterator
*/

FskErr KplDirectoryIteratorNew(const char *directoryPath, UInt32 flags, KplDirectoryIterator *dirIt);
FskErr KplDirectoryIteratorDispose(KplDirectoryIterator dirIt);

// if name or itemType is null, it wont be returned
FskErr KplDirectoryIteratorGetNext(KplDirectoryIterator dirIt, char **name, UInt32 *itemType);

// special directory
FskErr KplDirectoryGetSpecialPath(UInt32 type, const Boolean create, const char *volumeName, char **fullPath);

/*
	Volume Iterator
*/

FskErr KplVolumeIteratorNew(KplVolumeIterator *volIt);
FskErr KplVolumeIteratorDispose(KplVolumeIterator volIt);
FskErr KplVolumeIteratorGetNext(KplVolumeIterator volIt, UInt32 *id, char **path, char **name);	// if id, path, or name are null, they won't be returned

FskErr KplVolumeGetInfo(UInt32 volumeID, char **path, char **name, UInt32 *volumeType, Boolean *isRemovable, KplInt64 *capacity, KplInt64 *freeSpace);
FskErr KplVolumeGetInfoFromPath(const char *pathIn, char **pathOut, char **name, UInt32 *volumeType, Boolean *isRemovable, KplInt64 *capacity, KplInt64 *freeSpace);
FskErr KplVolumeGetDeviceInfo(UInt32 volumeID, char **vendor, char **product, char **revision, char **vendorSpecific);

FskErr KplVolumeGetID(const char *fullPath, UInt32 *volumeID);

/*
	Volume Eject
*/
FskErr KplVolumeEject(const char *fullPath, UInt32 flags);

/*
	File mapping
*/

FskErr KplFileMap(const char *fullPath, unsigned char **data, KplInt64 *dataSize, KplFileMapping *map);
FskErr KplFileDisposeMap(KplFileMapping map);

/*
	Directory change notifier
*/

FskErr KplDirectoryChangeNotifierNew(const char *path, UInt32 flags, KplDirectoryChangeNotifierCallbackProc callback, void *refCon, KplDirectoryChangeNotifier *dirChangeNtf);
FskErr KplDirectoryChangeNotifierDispose(KplDirectoryChangeNotifier dirChangeNtf);

/*
	Volume notifier
*/

FskErr KplVolumeNotifierNew(KplVolumeNotifierCallbackProc callback, void *refCon, KplVolumeNotifier *volNtf);
FskErr KplVolumeNotifierDispose(KplVolumeNotifier volNtf);

#ifdef __cplusplus
}
#endif

#endif // __KPL_FILES_H__
