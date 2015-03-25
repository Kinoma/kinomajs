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
#ifndef __FSK_FILES_H__
#define __FSK_FILES_H__

#include "FskMemory.h"
#include "FskBitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	kFskFilePermissionReadOnly = 0,
	kFskFilePermissionReadWrite = 1 << 0,
	kFskFilePermissionRecovery = 1 << 2
};

enum {
	kFskDirectoryItemIsFile = 1,
	kFskDirectoryItemIsDirectory = 2,
	kFskDirectoryItemIsLink = 3
};

enum {
	kFskDirectorySpecialTypeDocument,
	kFskDirectorySpecialTypePhoto,
	kFskDirectorySpecialTypeMusic,
	kFskDirectorySpecialTypeVideo,
	kFskDirectorySpecialTypeTV,
	kFskDirectorySpecialTypeApplicationPreference,
	kFskDirectorySpecialTypeApplicationPreferenceRoot,
	kFskDirectorySpecialTypeTemporary,
	kFskDirectorySpecialTypeStartMenu,
	kFskDirectorySpecialTypeMusicSync,
	kFskDirectorySpecialTypeVideoSync,
	kFskDirectorySpecialTypePhotoSync,
	kFskDirectorySpecialTypePlaylistSync,
	kFskDirectorySpecialTypeDownload,
	kFskDirectorySpecialTypeCache,

	kFskDirectorySpecialTypeSharedFlag = 0x80000000			// flag indicating that request is for folder shared across all users
};

enum {
	kFskVolumeTypeNone,
	kFskVolumeTypeUnknown,
	kFskVolumeTypeFixed,
	kFskVolumeTypeFD,
	kFskVolumeTypeOptical,
	kFskVolumeTypeCD,
	kFskVolumeTypeDVD,
	kFskVolumeTypeNetwork,
	kFskVolumeTypeMemoryStick,
	kFskVolumeTypeMMC,
	kFskVolumeTypeSDMemory,
	kFskVolumeTypeCompactFlash,
	kFskVolumeTypeSmartMedia,
	kFskVolumeTypeDirectory
};

enum {
	kFskVolumeHello = 1,
	kFskVolumeBye
};

typedef FskErr (*FskVolumeNotifierCallbackProc)(UInt32 status, UInt32 volumeID, void *refCon);
typedef FskErr (*FskDirectoryChangeNotifierCallbackProc)(UInt32 flags, const char *path, void *refCon);

enum {
	kFskDirectoryChangeFileUnknown = 0,
	kFskDirectoryChangeFileCreated = 1,
	kFskDirectoryChangeFileDeleted = 2,
	kFskDirectoryChangeFileChanged = 3
};

enum {
	kFskDirectoryChangeMonitorSubTree = 1
};

enum {
	kFileFileLocked = 1L << 1,
	kFileFileHidden = 1L << 2
};

typedef struct FskFileInfo {
	FskInt64	filesize;
	UInt32		filetype;
	UInt32		fileCreationDate;		// UTC
	UInt32		fileModificationDate;	// UTC
	UInt32		flags;
#if TARGET_OS_ANDROID
	UInt32		fileDevice;		// to determine crossing mountpoints
#endif
	FskInt64	fileNode;
} FskFileInfo;

typedef struct FskFileChooseEntryRecord FskFileChooseEntryRecord;
typedef struct FskFileChooseEntryRecord *FskFileChooseEntry;

#if TARGET_OS_WIN32 || TARGET_OS_MAC
	struct FskFileChooseEntryRecord {
		char			*extension;			// extension list, terminated with a null string
		char			*label;
	};
#endif

#if !defined(__FSKFILES_PRIV__) && !SUPPORT_INSTRUMENTATION
	FskDeclarePrivateType(FskFile)
	FskDeclarePrivateType(FskDirectoryIterator)
	FskDeclarePrivateType(FskVolumeIterator)
	FskDeclarePrivateType(FskFileMapping)
	FskDeclarePrivateType(FskVolumeNotifier)
	FskDeclarePrivateType(FskDirectoryChangeNotifier)
#else
	typedef struct FskFileRecord *FskFile;
	typedef struct FskDirectoryIteratorRecord *FskDirectoryIterator;
	typedef struct FskVolumeIteratorRecord *FskVolumeIterator;
	typedef struct FskFileMappingRecord *FskFileMapping;
	typedef struct FskVolumeNotifierRecord *FskVolumeNotifier;
	typedef struct FskDirectoryChangeNotifierRecord *FskDirectoryChangeNotifier;

	typedef FskErr (*FskFileFilterProc)(const char *fullPath, SInt32 *priority);

	typedef FskErr (*FskFileOpenProc)(const char *fullPath, UInt32 permissions, FskFile *fref);
	typedef FskErr (*FskFileCloseProc)(FskFile fref);
	typedef FskErr (*FskFileGetSizeProc)(FskFile fref, FskInt64 *size);
	typedef FskErr (*FskFileSetSizeProc)(FskFile fref, const FskInt64 *size);
	typedef FskErr (*FskFileSetPositionProc)(FskFile fref, const FskInt64 *position);
	typedef FskErr (*FskFileGetPositionProc)(FskFile fref, FskInt64 *position);
	typedef FskErr (*FskFileReadProc)(FskFile fref, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead);
	typedef FskErr (*FskFileWriteProc)(FskFile fref, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten);
	typedef FskErr (*FskFileGetFileInfoProc)(const char *fullpath, FskFileInfo *itemInfo);
	typedef FskErr (*FskFileSetFileInfoProc)(const char *fullpath, const FskFileInfo *itemInfo);
	typedef FskErr (*FskFileGetThumbnailProc)(const char *fullPath, const UInt32 width, const UInt32 height, FskBitmap *thumbnail);
	typedef FskErr (*FskFileCreateProc)(const char *fullpath);
	typedef FskErr (*FskFileCreateDirectoryProc)(const char *fullpath);
	typedef FskErr (*FskFileDeleteProc)(const char *fullpath);
	typedef FskErr (*FskFileDeleteDirectoryProc)(const char *fullpath);
	typedef FskErr (*FskFileRenameProc)(const char *fullPath, const char *newName);
	typedef FskErr (*FskFilePathToNativeProc)(const char *fskPath, char **nativePath);
	typedef FskErr (*FskFileRenameDirectoryProc)(const char *fullPath, const char *newName);
	typedef FskErr (*FskFileMapProc)(const char *fullPath, unsigned char **data, FskInt64 *dataSize, FskFileMapping *map);
	typedef FskErr (*FskFileDisposeMapProc)(FskFileMapping map);
	typedef FskErr (*FskDirectoryIteratorNewProc)(const char *directoryPath, FskDirectoryIterator *dirIt, UInt32 flags);
	typedef FskErr (*FskDirectoryIteratorDisposeProc)(FskDirectoryIterator dirIt);
	typedef FskErr (*FskDirectoryIteratorGetNextProc)(FskDirectoryIterator dirIt, char **name, UInt32 *itemType);
	typedef FskErr (*FskDirectoryGetSpecialPathProc)(UInt32 type, const Boolean create, const char *volumeName, char **fullPath);
	typedef FskErr (*FskVolumeIteratorNewProc)(FskVolumeIterator *volIt);
	typedef FskErr (*FskVolumeIteratorDisposeProc)(FskVolumeIterator volIt);
	typedef FskErr (*FskVolumeIteratorGetNextProc)(FskVolumeIterator volIt, UInt32 *id, char **path, char **name);
	typedef FskErr (*FskVolumeNotifierNewProc)(FskVolumeNotifierCallbackProc callback, void *refCon, FskVolumeNotifier *volNtf);
	typedef FskErr (*FskVolumeNotifierDisposeProc)(FskVolumeNotifier volNtf);
	typedef FskErr (*FskVolumeGetInfoProc)(UInt32 volumeID, char **path, char **name, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace);
	typedef FskErr (*FskVolumeGetInfoFromPathProc)(const char *pathIn, char **pathOut, char **name, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace);
	typedef FskErr (*FskVolumeGetDeviceInfoProc)(UInt32 volumeID, char **vendor, char **product, char **revision, char **vendorSpecific);
	typedef FskErr (*FskVolumeGetIDProc)(const char *fullPath, UInt32 *volumeID);
	typedef FskErr (*FskVolumeEjectProc)(const char *fullPath, UInt32 flags);
	typedef FskErr (*FskFileChooseProc)(const FskFileChooseEntry types, const char *prompt, Boolean allowMultiple, const char *initialPath, char **files);
	typedef FskErr (*FskFileChooseSaveProc)(const char *defaultName, const char *prompt, const char *initialDirectory, char **file);
	typedef FskErr (*FskDirectoryChooseProc)(const char *prompt, const char *initialPath, char **path);
	typedef FskErr (*FskFileTerminateProc)(void);
	typedef FskErr (*FskDirectoryChangeNotifierNewProc)(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNtf);
	typedef FskErr (*FskDirectoryChangeNotifierDisposeProc)(FskDirectoryChangeNotifier dirChangeNtf);
	typedef FskErr (*FskFileInitializeProc)(void);
	typedef FskErr (*FskFileResolveLinkProc)(const char *path, char **resolved);
	typedef FskErr (*FskFileFlushProc)(FskFile fref);

	struct FskFileDispatchTableRecord {
		UInt32							version;

		FskFileFilterProc				fileFilter;

		FskFileOpenProc					fileOpen;
		FskFileCloseProc				fileClose;
		FskFileGetSizeProc				fileGetSize;
		FskFileSetSizeProc				fileSetSize;
		FskFileSetPositionProc			fileSetPosition;
		FskFileGetPositionProc			fileGetPosition;
		FskFileReadProc					fileRead;
		FskFileWriteProc				fileWrite;
		FskFileGetFileInfoProc			fileGetInfo;
		FskFileSetFileInfoProc			fileSetInfo;
		FskFileGetThumbnailProc			fileGetThumbnail;
		FskFileCreateProc				fileCreate;
		FskFileDeleteProc				fileDelete;
		FskFileRenameProc				fileRename;
		FskFilePathToNativeProc			filePathToNative;

		FskFileCreateDirectoryProc		dirCreate;
		FskFileDeleteDirectoryProc		dirDelete;
		FskFileRenameDirectoryProc		dirRename;
		FskDirectoryIteratorNewProc		dirIteratorNew;
		FskDirectoryIteratorDisposeProc	dirIteratorDispose;
		FskDirectoryIteratorGetNextProc	dirIteratorGetNext;
		FskDirectoryGetSpecialPathProc	dirGetSpecialPath;

		FskVolumeIteratorNewProc		volIteratorNew;
		FskVolumeIteratorDisposeProc	volIteratorDispose;
		FskVolumeIteratorGetNextProc	volIteratorGetNext;

		FskVolumeNotifierNewProc		volNotifierNew;
		FskVolumeNotifierDisposeProc	volNotifierDispose;
		FskVolumeGetInfoProc			volGetInfo;
		FskVolumeGetInfoFromPathProc	volGetInfoFromPath;
		FskVolumeGetDeviceInfoProc		volGetDeviceInfo;
		FskVolumeGetIDProc				volGetID;
		FskVolumeEjectProc				volEject;

		FskFileMapProc					fileMap;
		FskFileDisposeMapProc			fileDisposeMap;

		FskFileChooseProc				fileChoose;
		FskFileChooseSaveProc			fileChooseSave;
		FskDirectoryChooseProc			directoryChoose;

		FskFileTerminateProc			fileTerminate;

		FskDirectoryChangeNotifierNewProc		dirChangeNotifyNew;
		FskDirectoryChangeNotifierDisposeProc	dirChangeNotifyDispose;

		FskFileInitializeProc			fileInitialize;
		FskFileResolveLinkProc			fileResolveLink;

		FskFileFlushProc				fileFlush;
	} ;
	typedef struct FskFileDispatchTableRecord FskFileDispatchTableRecord;
	typedef struct FskFileDispatchTableRecord *FskFileDispatchTable;

	struct FskFileDispatchRecord {
		FskFileDispatchTable		dispatch;
		void						*refcon;
	};
	typedef struct FskFileDispatchRecord FskFileDispatchRecord;
	typedef struct FskFileDispatchRecord *FskFileDispatch;

	struct FskFileRecord {
		FskFileDispatchRecord		dispatch;
		FskInstrumentedItemDeclaration
	};
	typedef struct FskFileRecord FskFileRecord;

	struct FskDirectoryIteratorRecord {
		FskFileDispatchRecord		dispatch;
		FskInstrumentedItemDeclaration
	};
	typedef struct FskDirectoryIteratorRecord FskDirectoryIteratorRecord;

	struct FskFileMappingRecord {
		FskFileDispatchRecord		dispatch;
		FskInstrumentedItemDeclaration
		FskMemPtr					address;		// for emulated file mapping
	};
	typedef struct FskFileMappingRecord FskFileMappingRecord;

	struct FskVolumeNotifierRecord {
		struct  FskVolumeNotifierRecord		*nextNotifier;
		FskFileDispatchRecord				dispatch;
		FskInstrumentedItemDeclaration
	};
	typedef struct FskVolumeNotifierRecord FskVolumeNotifierRecord;

	struct FskDirectoryChangeNotifierRecord {
		struct  FskDirectoryChangeNotifierRecord		*next;
		FskFileDispatchRecord							dispatch;
		FskInstrumentedItemDeclaration
	};
	typedef struct FskDirectoryChangeNotifierRecord FskDirectoryChangeNotifierRecord;
#endif

FskAPI(FskErr) FskFileInitialize(void);
FskAPI(FskErr) FskFileTerminate(void);

FskAPI(FskErr) FskFileOpen(const char *fullPath, UInt32 permissions, FskFile *fref);
FskAPI(FskErr) FskFileClose(FskFile fref);

FskAPI(FskErr) FskFileGetSize(FskFile fref, FskInt64 *size);
FskAPI(FskErr) FskFileSetSize(FskFile fref, const FskInt64 *size);

FskAPI(FskErr) FskFileSetPosition(FskFile fref, const FskInt64 *position);
FskAPI(FskErr) FskFileGetPosition(FskFile fref, FskInt64 *position);

FskAPI(FskErr) FskFileRead(FskFile fref, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead);
FskAPI(FskErr) FskFileWrite(FskFile fref, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten);
FskAPI(FskErr) FskFileFlush(FskFile fref);

FskAPI(FskErr) FskFileGetFileInfo(const char *fullpath, FskFileInfo *itemInfo);
FskAPI(FskErr) FskFileSetFileInfo(const char *fullpath, const FskFileInfo *itemInfo);
FskAPI(FskErr) FskFileGetThumbnail(const char *fullPath, const UInt32 width, const UInt32 height, FskBitmap *thumbnail);

FskAPI(FskErr) FskFileCreate(const char *fullPath);
FskAPI(FskErr) FskFileCreateDirectory(const char *fullPath);

FskAPI(FskErr) FskFileDelete(const char *fullPath);
FskAPI(FskErr) FskFileDeleteDirectory(const char *fullPath);

FskAPI(FskErr) FskFileRename(const char *fullPath, const char *newName);
FskAPI(FskErr) FskFileRenameDirectory(const char *fullPath, const char *newName);

FskAPI(FskErr) FskFilePathToNative(const char *fskPath, char **nativePath);

FskAPI(FskErr) FskFileResolveLink(const char *path, char **resolved);

enum {
	kFskFileMapDoNotEmulate = 1
};

FskAPI(FskErr) FskFileMap(const char *fullPath, unsigned char **data, FskInt64 *dataSize, UInt32 flags, FskFileMapping *map);
FskAPI(FskErr) FskFileDisposeMap(FskFileMapping map);

// file helper
FskAPI(FskErr) FskFileLoad(const char *filePath, FskMemPtr *file, FskInt64 *fileSize);

#if TARGET_OS_ANDROID
	FskErr FskCopyFile(char *src, char *dst);
#endif

/*
	Directory Iterator
*/

FskAPI(FskErr) FskDirectoryIteratorNew(const char *directoryPath, FskDirectoryIterator *dirIt, UInt32 flags);
FskAPI(FskErr) FskDirectoryIteratorDispose(FskDirectoryIterator dirIt);

// if name or itemType is null, it wont be returned
FskAPI(FskErr) FskDirectoryIteratorGetNext(FskDirectoryIterator dirIt, char **name, UInt32 *itemType);

// directory helper
FskAPI(FskErr) FskDirectoryCountItems(const char *directoryPath, UInt32 *files, UInt32 *directories);

//	Special Directory
FskAPI(FskErr) FskDirectoryGetSpecialPath(UInt32 type, const Boolean create, const char *volumeName, char **fullPath);

/*
	Volume Iterator
*/

FskAPI(FskErr) FskVolumeIteratorNew(FskVolumeIterator *volIt);
FskAPI(FskErr) FskVolumeIteratorDispose(FskVolumeIterator volIt);
FskAPI(FskErr) FskVolumeIteratorGetNext(FskVolumeIterator volIt, UInt32 *id, char **path, char **name);	// if id, path, or name are null, they won't be returned

/*
	Volume Notifier
*/
FskAPI(FskErr) FskVolumeNotifierNew(FskVolumeNotifierCallbackProc callback, void *refCon, FskVolumeNotifier *volNtf);
FskAPI(FskErr) FskVolumeNotifierDispose(FskVolumeNotifier volNtf);

FskAPI(FskErr) FskVolumeGetInfo(UInt32 volumeID, char **path, char **name, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace);
FskAPI(FskErr) FskVolumeGetInfoFromPath(const char *path, char **pathOut, char **name, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace);
FskAPI(FskErr) FskVolumeGetDeviceInfo(UInt32 volumeID, char **vendor, char **product, char **revision, char **vendorSpecific);
FskAPI(FskErr) FskVolumeGetID(const char *fullPath, UInt32 *volumeID);
FskAPI(FskErr) FskVolumeEject(const char *fullPath, UInt32 flags);

/*
	Directory change notifier
*/

FskAPI(FskErr) FskDirectoryChangeNotifierNew(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNtf);
FskAPI(FskErr) FskDirectoryChangeNotifierDispose(FskDirectoryChangeNotifier dirChangeNtf);

/*
	user interface
*/

FskAPI(FskErr) FskFileChoose(const FskFileChooseEntry types, const char *prompt, Boolean allowMultiple, const char *initialPath, char **files);
FskAPI(FskErr) FskFileChooseSave(const char *defaultName, const char *prompt, const char *initialDirectory, char **file);
FskAPI(FskErr) FskDirectoryChoose(const char *prompt, const char *initialPath, char **path);

#if SUPPORT_INSTRUMENTATION

enum {
	kFskFileInstrMsgRead = kFskInstrumentedItemFirstCustomMessage,
	kFskFileInstrMsgWrite,
	kFskFileInstrMsgSetPosition,
	kFskFileInstrMsgSetSize,
	kFskFileInstrMsgFileDelete,
	kFskFileInstrMsgDirectoryDelete,
	kFskFileInstrMsgFileCreate,
	kFskFileInstrMsgDirectoryCreate,
	kFskFileInstrMsgGetFileInfo,
	kFskFileInstrMsgFileOpenFailed
};

typedef struct {
	UInt32		bytesToRead;
	const void	*buffer;
	UInt32		*bytesRead;
	FskErr		err;
} FskFileInstrMsgReadRecord;

typedef struct {
	UInt32		bytesToWrite;
	const void	*buffer;
	UInt32		*bytesWritten;
	FskErr		err;
} FskFileInstrMsgWriteRecord;

typedef struct {
	FskErr		err;
	char		*path;
} FskFileInstrMsgFileOpenFailedRecord;

typedef struct {
	UInt32			flags;
	char			*path;
} FskDirectoryChangeInstrMsgChangeRecord;

enum {
	kFskDirectoryIteratorInstrMsgNext = kFskInstrumentedItemFirstCustomMessage,

	kFskDirectoryChangeInstrMsgChanged = kFskInstrumentedItemFirstCustomMessage

};

#endif

#ifdef __cplusplus
}
#endif

#endif // __FSK_FILES_H__

