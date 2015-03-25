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
#ifndef __FSK_FSFILES_H__
#define __FSK_FSFILES_H__

#include "Fsk.h"
#include "FskFiles.h"
#include "FskUtilities.h"

#ifdef __FSKFSFILES_PRIV__
	#include "FskThread.h"
	#if TARGET_OS_LINUX
		#include <dirent.h>
		#if !defined(ANDROID)
			#include <mntent.h>
			#include <sys/vfs.h>
			#define kFskLinuxVolumeListFile	"/etc/mtab"
		#endif
	#elif TARGET_OS_IPHONE
		#include <stdio.h>
		#include <dirent.h>
	#elif TARGET_OS_MAC
        #include <stdio.h>
        #include <dirent.h>
        #include <CoreServices/CoreServices.h>
        #include <Carbon/Carbon.h>
	#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __FSKFSFILES_PRIV__
	FskDeclarePrivateType(FskFSFile)
	FskDeclarePrivateType(FskFSDirectoryIterator)
	FskDeclarePrivateType(FskFSVolumeIterator)
	FskDeclarePrivateType(FskFSVolumeNotifier)
	FskDeclarePrivateType(FskFSFileMapping)
#else
	typedef struct {
		FskFileDispatchRecord		dispatch;
		FskInstrumentedItemDeclaration
		UInt32	thePermissions;
	#if TARGET_OS_WIN32
		HANDLE	hFile;
	#elif TARGET_OS_LINUX || TARGET_OS_MAC
		Boolean	flushBeforeRead;
		Boolean	flushBeforeWrite;
		FILE	*theFile;
	#elif TARGET_OS_MAC
		SInt16		forkRefNum;
		FSRef		fs;
		FskInt64 	position;
		Boolean		positionSet;
	#elif TARGET_OS_KPL
		void	*kplFile;
	#else
		#error - And for your platform?
	#endif
	} FskFSFileRecord, *FskFSFile;

	typedef struct {
		FskFileDispatchRecord		dispatch;
		FskInstrumentedItemDeclaration
		UInt32						flags;
	#if TARGET_OS_WIN32
		HANDLE				hFind;
		WIN32_FIND_DATAW	findData;
		WCHAR				*dirPath;
	#elif TARGET_OS_ANDROID
		FskMemPtr		root;
		DIR				*theDir;
		UInt32			device;	// from stat - to prevent crossing filesystems
	#elif TARGET_OS_LINUX || TARGET_OS_MAC
		FskMemPtr		root;
		DIR				*theDir;
	#elif TARGET_OS_KPL
		void			*iterator;
	#else
		#error And for your platform?
	#endif
	} FskFSDirectoryIteratorRecord, *FskFSDirectoryIterator;

	typedef struct {
	#if TARGET_OS_WIN32
		WCHAR		*volList;
		WCHAR		*cur;
	#elif defined(ANDROID)
		int				iter;
	#elif TARGET_OS_LINUX
		void			*volumeInfo;
	#elif TARGET_OS_MAC
		UInt32			volumeIndex;
	#elif TARGET_OS_KPL
		void			*iterator;
	#else
		#error - And for your platform?
	#endif
	} FskFSVolumeIteratorRecord, *FskFSVolumeIterator;

	struct FskFSVolumeNotifierRecord {
		struct FskFSVolumeNotifierRecord	*nextNotifier;
		FskFileDispatchRecord				dispatch;
		FskInstrumentedItemDeclaration

		FskVolumeNotifierCallbackProc	callback;
		void							*refCon;
	#if TARGET_OS_WIN32
		WCHAR							*volumes;

		FskThread						callbackThread;
		void							*next;
	#elif TARGET_OS_ANDROID
		FskThread						callbackThread;
		void							*next;
	#elif TARGET_OS_MAC
		FskThread						callbackThread;
		void							*next;
		void							*instance;
	#elif TARGET_OS_KPL
		void							*notifier;
		FskThread						callbackThread;
		void							*next;
	#endif
	};
	
	typedef struct FskFSVolumeNotifierRecord FskFSVolumeNotifierRecord;
	typedef struct FskFSVolumeNotifierRecord *FskFSVolumeNotifier;

	typedef struct {
		FskFileDispatchRecord		dispatch;
		FskInstrumentedItemDeclaration
	#if TARGET_OS_WIN32
		FskFSFile					file;
		HANDLE						mapping;
		void						*address;
	#elif TARGET_OS_KPL
		void						*mapping;
	#else
		int							file;
		char						*address;
		size_t						length;
	#endif
	} FskFSFileMappingRecord, *FskFSFileMapping;
#endif

FskErr FskFSFileInitialize(void);
FskErr FskFSFileTerminate(void);

FskErr FskFSFileOpen(const char *fullPath, UInt32 permissions, FskFSFile *fref);
FskErr FskFSFileClose(FskFSFile fref);

FskErr FskFSFileGetSize(FskFSFile fref, FskInt64 *size);
FskErr FskFSFileSetSize(FskFSFile fref, const FskInt64 *size);

FskErr FskFSFileSetPosition(FskFSFile fref, const FskInt64 *position);
FskErr FskFSFileGetPosition(FskFSFile fref, FskInt64 *position);

FskErr FskFSFileRead(FskFSFile fref, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead);
FskErr FskFSFileWrite(FskFSFile fref, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten);
FskErr FskFSFileFlush(FskFSFile fref);

FskErr FskFSFileGetFileInfo(const char *fullpath, FskFileInfo *itemInfo);
FskErr FskFSFileSetFileInfo(const char *fullpath, const FskFileInfo *itemInfo);

FskErr FskFSFileGetThumbnail(const char *fullPath, const UInt32 width, const UInt32 height, FskBitmap *thumbnail);

FskErr FskFSFileCreate(const char *fullPath);
FskErr FskFSFileCreateDirectory(const char *fullPath);

FskErr FskFSFileDelete(const char *fullPath);
FskErr FskFSFileDeleteDirectory(const char *fullPath);

FskErr FskFSFileRename(const char *fullPath, const char *newName);
FskErr FskFSFileRenameDirectory(const char *fullPath, const char *newName);

FskErr FskFSFilePathToNative(const char *fskPath, char **nativePath);

FskErr FskFSFileResolveLink(const char *path, char **resolved);

/*
	Directory Iterator
*/

FskErr FskFSDirectoryIteratorNew(const char *directoryPath, FskFSDirectoryIterator *dirIt, UInt32 flags);
FskErr FskFSDirectoryIteratorDispose(FskFSDirectoryIterator dirIt);

// if name or itemType is null, it wont be returned
FskErr FskFSDirectoryIteratorGetNext(FskFSDirectoryIterator dirIt, char **name, UInt32 *itemType);

// special directory
FskErr FskFSDirectoryGetSpecialPath(UInt32 type, const Boolean create, const char *volumeName, char **fullPath);

/*
	Volume Iterator
*/

FskErr FskFSVolumeIteratorNew(FskFSVolumeIterator *volIt);
FskErr FskFSVolumeIteratorDispose(FskFSVolumeIterator volIt);
FskErr FskFSVolumeIteratorGetNext(FskFSVolumeIterator volIt, UInt32 *id, char **path, char **name);	// if id, path, or name are null, they won't be returned

FskErr FskFSVolumeNotifierNew(FskVolumeNotifierCallbackProc callback, void *refCon, FskFSVolumeNotifier *volNtf);
FskErr FskFSVolumeNotifierDispose(FskFSVolumeNotifier volNtf);

FskErr FskFSVolumeGetInfo(UInt32 volumeID, char **path, char **name, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace);
FskErr FskFSVolumeGetInfoFromPath(const char *pathIn, char **pathOut, char **name, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace);
FskErr FskFSVolumeGetDeviceInfo(UInt32 volumeID, char **vendor, char **product, char **revision, char **vendorSpecific);
FskErr FskFSVolumeGetID(const char *fullPath, UInt32 *volumeID);
FskErr FskFSVolumeEject(const char *fullPath, UInt32 eject);

/*
	File mapping
*/

FskErr FskFSFileMap(const char *fullPath, unsigned char **data, FskInt64 *dataSize, FskFSFileMapping *map);
FskErr FskFSFileDisposeMap(FskFSFileMapping map);

/*
	Directory change notifier
*/

FskErr FskFSDirectoryChangeNotifierNew(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNtf);
FskErr FskFSDirectoryChangeNotifierDispose(FskDirectoryChangeNotifier dirChangeNtf);

/*
	User interface
*/

FskErr FskFSFileChoose(const FskFileChooseEntry types, const char *prompt, Boolean allowMultiple, const char *initialPath, char **files);
FskErr FskFSFileChooseSave(const char *defaultName, const char *prompt, const char *initialDirectory, char **file);
FskErr FskFSDirectoryChoose(const char *prompt, const char *initialPath, char **path);

#ifdef __cplusplus
}
#endif

#endif // __FSK_FILES_H__

