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
#include "Fsk.h"
#include "FskList.h"
#define __FSKFILES_PRIV__
#define __FSKFSFILES_PRIV__
#include "FskFS.h"
#include "FskUtilities.h"
#include "FskEnvironment.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>		// mmap'd files
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>

#ifdef _LARGEFILE64_SOURCE
	#define STATTYPE	struct stat64
	#define FSTAT		fstat64
	#define STAT		lstat64
	#define FSEEK		fseeko64
	#define FTELL		ftello64
	#define FILEOFFSET	off64_t
	#define FOPEN		fopen64
	#define OPEN		open64
	#define CREATEFLAGS	O_CREAT | O_EXCL | O_LARGEFILE
#else
	#define STATTYPE	struct stat
	#define FSTAT		fstat
	#define STAT		lstat
	#define FSEEK		fseek
	#define FTELL		ftell
	#define FILEOFFSET	off_t
	#define FOPEN		fopen
	#define OPEN		open
	#define CREATEFLAGS	O_CREAT | O_EXCL
#endif

#ifdef _DARWIN_FEATURE_64_BIT_INODE
	#define ST_CTIME	st_birthtime
#else
	#define ST_CTIME	st_ctime
#endif

FskInstrumentedSimpleType(LinuxFiles, linuxfiles);

typedef struct volInfoRec {
	struct volInfoRec *next;
	char		*mountPoint;
	char		*name;
	char		*typeStr;
	UInt32		type;
	Boolean		removable;
	Boolean		mounted;
	FskInt64	capacity;
	FskInt64	remaining;
} volInfoRec, *volInfo;

#if !TARGET_OS_MAC
volInfo	*volumeList = NULL;
void terminateVolumeList();
#endif

#if BAD_FTRUNCATE
	int FTRUNCATE(FILE *theFile, FILEOFFSET length);	
#else
	#define FTRUNCATE(a,b) ftruncate(fileno(a),b)
#endif


// ---------------------------------------------------------------------
static char *sPermsToPermStr(UInt32 permissions) {
	switch (permissions) {
		case kFskFilePermissionReadOnly: return "rb";
		case kFskFilePermissionReadWrite: return "rb+";
	}
	return "rb";
}

// ---------------------------------------------------------------------
static FskErr errnoToFskErr(int theErrno);
FskErr errnoToFskErr(int theErrno) {
	FskErr ret;

	switch (theErrno) {
		case EBADF:
		case EINVAL:
			ret = kFskErrInvalidParameter;
			break;
		case EACCES:
			ret = kFskErrFilePermissions;
			break;
		case EISDIR:
			ret = kFskErrIsDirectory;
			break;
		case ENOENT:
			ret = kFskErrFileNotFound;
			break;
		case ENOTEMPTY:
		case EEXIST:
			ret = kFskErrFileExists;
			break;
		case EROFS:
			ret = kFskErrReadOnly;
			break;
		case EFBIG:
		case ENOSPC:
			ret = kFskErrDiskFull;
			break;
		case ENOTDIR:
			ret = kFskErrNotDirectory;
			break;
		case EMFILE:
		case ENFILE:
			ret = kFskErrTooManyOpenFiles;
			break;
		case ENOMEM:
			ret = kFskErrMemFull;
			break;
		default:
			ret = kFskErrUnknown;
			break;
	}

	return ret;
}

#if !TARGET_OS_MAC
// ---------------------------------------------------------------------
static FskErr FskFSFileFilter(const char *fullPath, SInt32 *priority);

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
		(FskFileGetThumbnailProc)NULL,
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

		(FskVolumeNotifierNewProc)NULL,		// vol notifier new
		(FskVolumeNotifierDisposeProc)NULL,		// vol notifier dispose
		(FskVolumeGetInfoProc)FskFSVolumeGetInfo,		// vol get info
		(FskVolumeGetInfoFromPathProc)FskFSVolumeGetInfoFromPath,		// vol get info by path
		(FskVolumeGetDeviceInfoProc)FskFSVolumeGetDeviceInfo,		// vol get device info
		(FskVolumeGetIDProc)NULL,		// vol get id
		(FskVolumeEjectProc)NULL,

		(FskFileMapProc)FskFSFileMap,
		(FskFileDisposeMapProc)FskFSFileDisposeMap,

		(FskFileChooseProc)NULL,
		(FskFileChooseSaveProc)NULL,
		(FskDirectoryChooseProc)NULL,

		(FskFileTerminateProc)FskFSFileTerminate,

		(FskDirectoryChangeNotifierNewProc)NULL,
		(FskDirectoryChangeNotifierDisposeProc)NULL,

		(FskFileInitializeProc)NULL,
		(FskFileResolveLinkProc)FskFSFileResolveLink,
};

FskErr FskFSFileFilter(const char *fullPath, SInt32 *priority)
{
        *priority = 0;
        return kFskErrNone;
}
#endif /* !TARGET_OS_MAC */


enum {
	kFskPathIsAny = 0,
	kFskPathIsFile = 1,
	kFskPathIsDirectory = 2
};

static FskErr sCheckFullPath(const char *fullPath, UInt32 pathType) {
	SInt32 pathLen;

	if (NULL == fullPath)
		return kFskErrInvalidParameter;

	if ('/' == fullPath[0] )
		;
	else
		return kFskErrInvalidParameter;

	pathLen = FskStrLen(fullPath);
	if (kFskPathIsFile == pathType) {
		if ((0 != pathLen) && ('/' == fullPath[pathLen - 1]))
			return kFskErrInvalidParameter;
	}
	else
	if (kFskPathIsDirectory == pathType) {
		if ((0 != pathLen) && ('/' != fullPath[pathLen - 1]))
			return kFskErrInvalidParameter;
	}
	
	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskFSFileOpen(const char *fullPath, UInt32 permissions, FskFSFile *frefOut) {
	FskErr		err;
	FskFSFile	fref;
	FskFileInfo itemInfo;

    *frefOut = NULL;

	err = sCheckFullPath(fullPath, kFskPathIsFile);
	if (err)
		return err;

	err = FskFSFileGetFileInfo(fullPath, &itemInfo);
	if (err)
		return err;

	if (itemInfo.filetype == kFskDirectoryItemIsDirectory)
		return kFskErrIsDirectory;

	err = FskMemPtrNewClear(sizeof(FskFSFileRecord), (FskMemPtr *)&fref);
	if (err)
		return kFskErrMemFull;

	fref->thePermissions = permissions;

	fref->theFile = FOPEN(fullPath, sPermsToPermStr(permissions));
	if (!fref->theFile) {
		FskMemPtrDispose(fref);
		return errnoToFskErr(errno);
	}

	*frefOut = fref;
	return kFskErrNone;
}


// ---------------------------------------------------------------------
FskErr FskFSFileClose(FskFSFile fref) {
	if (fref) {
		fclose(fref->theFile);
		FskMemPtrDispose(fref);
	}

	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskFSFileGetSize(FskFSFile fref, FskInt64 *size) {
	FILEOFFSET	pos, end;
	FskErr err = kFskErrNone;
	int ret;

	ret = FTELL(fref->theFile);	
	if (ret < 0) goto bail;
	pos = ret;
	ret = FSEEK(fref->theFile, 0, SEEK_END);
	if (ret < 0) goto bail;
	ret = FTELL(fref->theFile);	
	if (ret < 0) goto bail;
	end = ret;
	ret = FSEEK(fref->theFile, pos, SEEK_SET);
	if (ret < 0) goto bail;

	fref->flushBeforeRead = false;
	fref->flushBeforeWrite = false;

	if (size)		
		*size = end;

bail:
	if (ret < 0)
		err = errnoToFskErr(errno);

	return err;
}

// ---------------------------------------------------------------------
#if BAD_FTRUNCATE
int FTRUNCATE(FILE *theFile, FILEOFFSET length)
{
	STATTYPE	statbuf;
	FILEOFFSET	loc;
	int			ret, fd;
	char c = '\0';

	fd = fileno(theFile);	
	if (-1 == FSTAT(fd, &statbuf))
		return -1;
	if (length == statbuf.st_size)
		return 0;
	if (statbuf.st_size > length) {
		ret = ftruncate(fd, length);		// shrink should work.
		return ret;
	}
	if (-1 == (loc = FSEEK(theFile, 0, SEEK_CUR)))
		return -1;
	if (-1 == FSEEK(theFile, length - 1, SEEK_SET)) {
		if (errno == EINVAL)
			errno = ENOSPC;
		return -1;
	}
	if (1 != fwrite(&c, 1, 1, theFile))
		return -1;
	if (-1 == FSEEK(theFile, loc, SEEK_SET))
		return -1;
	return 0;
}
#endif

FskErr FskFSFileSetSize(FskFSFile fref, const FskInt64 *size) {
	FILEOFFSET	pos, oldpos;
//	int fno;
	int ret;

	if (!fref)
		return kFskErrInvalidParameter;

	if (fref->thePermissions == kFskFilePermissionReadOnly)
		return kFskErrReadOnly;

	fref->flushBeforeRead = false;
	fref->flushBeforeWrite = false;

//	fno = fileno(fref->theFile);
	pos = *size;	
	oldpos = FTELL(fref->theFile);	
	if (oldpos > pos) {
		ret = FSEEK(fref->theFile, pos, SEEK_SET);
		if (-1 == ret)
			return errnoToFskErr(errno);
	}
		
	ret = FTRUNCATE(fref->theFile, pos);
	if (-1 == ret)
		return errnoToFskErr(errno);

	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskFSFileSetPosition(FskFSFile fref, const FskInt64 *position)
{
	int		err;
	FILEOFFSET	pos;

	if (!fref)
		return kFskErrInvalidParameter;

	pos = *position;

	fref->flushBeforeRead = false;
	fref->flushBeforeWrite = false;

	err = FSEEK(fref->theFile, pos, SEEK_SET);
	if (err == -1)
		return errnoToFskErr(errno);
	
	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskFSFileGetPosition(FskFSFile fref, FskInt64 *position)
{
	FILEOFFSET pos;

	if (!fref)
		return kFskErrInvalidParameter;

	pos = FTELL(fref->theFile);	
	if (pos == -1)
		return errnoToFskErr(errno);

	*position = pos;

	if (*position != pos)
		return kFskErrOutOfRange;
	
	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskFSFileRead(FskFSFile fref, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead) {
	size_t	amt;

	if (bytesRead) *bytesRead = 0;

	if (!fref)
		return kFskErrInvalidParameter;

fflush(fref->theFile);
	if (0 != bytesToRead) {
		if (fref->flushBeforeRead) {
			fflush(fref->theFile);
			fref->flushBeforeRead = false;
		}
		amt = fread(buffer, 1, bytesToRead, fref->theFile);
		fref->flushBeforeWrite = true;

		if (amt == 0) {
			if (feof(fref->theFile)) {
				return kFskErrEndOfFile;
			}

			FskLinuxFilesPrintfDebug("read error errno:%d fskerr %d\n", errno, errnoToFskErr(errno));
			return kFskErrOperationFailed;
		}
	}
	else
		amt = 0;

	if (bytesRead)	
		*bytesRead = amt;
	else {
		// can't return amount read, so we've got to report failure if
		// we couldn't read the whole thing
		if (amt != bytesToRead)
			return kFskErrOperationFailed;
	}

	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskFSFileWrite(FskFSFile fref, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten)
{
	size_t	amt;

	if (!fref)
		return kFskErrInvalidParameter;

	if (fref->thePermissions == kFskFilePermissionReadOnly)
		return kFskErrReadOnly;

	if (fref->flushBeforeWrite) {
		fflush(fref->theFile);
		fref->flushBeforeWrite = false;
	}
	amt = fwrite(buffer, 1, bytesToWrite, fref->theFile);
	if (amt != bytesToWrite) {
		if (bytesWritten)
			*bytesWritten = 0;
		return errnoToFskErr(errno);
	}

	fref->flushBeforeRead = true;
	if (bytesWritten)
		*bytesWritten = amt;

	return kFskErrNone;
}

// ---------------------------------------------------------------------
static void GetFileInfoFromStat(STATTYPE *statbuf, FskFileInfo *itemInfo)
{
	if (S_ISLNK(statbuf->st_mode))
		itemInfo->filetype = kFskDirectoryItemIsLink;
	else if (S_ISDIR(statbuf->st_mode))
		itemInfo->filetype = kFskDirectoryItemIsDirectory;
	else
		itemInfo->filetype = kFskDirectoryItemIsFile;

	itemInfo->filesize = statbuf->st_size;
	itemInfo->fileModificationDate = statbuf->st_mtime;
	itemInfo->fileCreationDate = statbuf->ST_CTIME;
	itemInfo->fileNode = statbuf->st_ino;
}

// ---------------------------------------------------------------------
FskErr FskFSFileGetFileInfo(const char *fullPath, FskFileInfo *itemInfo) {
	int err;
	STATTYPE statbuf;

	FskLinuxFilesPrintfDebug("FSFileGetFileInfo %s\n", fullPath);
	err = sCheckFullPath(fullPath, kFskPathIsAny);
	if (err)
		return err;

	err = STAT(fullPath, &statbuf);
	if (err == -1)
		return errnoToFskErr(errno);

	GetFileInfoFromStat(&statbuf, itemInfo);

	itemInfo->flags = 0;

	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskFSFileSetFileInfo(const char *fullPath, const FskFileInfo *itemInfo)
{
	int err;
	struct utimbuf times = {0};

	err = sCheckFullPath(fullPath, kFskPathIsAny);
	if (err)
		return err;

	times.modtime = itemInfo->fileModificationDate;

	err = utime(fullPath, &times);
	if (err == -1)
		return errnoToFskErr(errno);

	return kFskErrNone;
}

// ---------------------------------------------------------------------
static FskErr FskFSFileGetPathInfo(const char *rootpath, const char *filepath, FskFileInfo *itemInfo) {
	int err;
	STATTYPE statbuf;
	char	*fullpath;

	err = FskMemPtrNew(FskStrLen(rootpath) + FskStrLen(filepath) + 2, (FskMemPtr *)&fullpath);
	if (err)
		return err;

	FskStrCopy(fullpath, rootpath);
	FskStrCat(fullpath, "/");
	FskStrCat(fullpath, filepath);

#if 0
	err = sCheckFullPath(fullpath);		// not needed for internal function?
	BAIL_IF_ERR(err);
#endif

	err = STAT(fullpath, &statbuf);
	if (err == -1) {
		err= errnoToFskErr(errno);
		goto bail;
	}

	GetFileInfoFromStat(&statbuf, itemInfo);

bail:
	FskMemPtrDispose(fullpath);

	return err;
}

FskErr FskFSFileDelete(const char *fullPath)
{
	int err;

	err = sCheckFullPath(fullPath, kFskPathIsFile);
	if (err)
		return err;

	err = unlink(fullPath);
	if (err == -1) {
		return errnoToFskErr(errno);
	}
	else
		return kFskErrNone;
}

FskErr FskFSFileRename(const char *fullPath, const char *newName)
{
	int err;
	char *p, newPath[PATH_MAX];
	FskFileInfo itemInfo;

	err = sCheckFullPath(fullPath, kFskPathIsFile);
	if (err)
		return err;

	err = FskFSFileGetFileInfo(fullPath, &itemInfo);
	if (err)
		return err;
	if (itemInfo.filetype == kFskDirectoryItemIsDirectory)
		return kFskErrIsDirectory;

	p = FskStrRChr((char *)newName, '/');
	if (p)
		return kFskErrOperationFailed;	// newName contains path elements

	FskStrCopy(newPath, fullPath);
	p = FskStrRChr(newPath, '/');
	if (p)
		*++p = '\0';
	FskStrCat(newPath, newName);

	err = rename(fullPath, newPath);
	if (err == -1)
		return errnoToFskErr(errno);
	else
		return kFskErrNone;
}

FskErr FskFSFilePathToNative(const char *fskPath, char **nativePath)
{
	if ((NULL == fskPath) || ('/' != fskPath[0]))
		return kFskErrInvalidParameter;

	*nativePath = FskStrDoCopy(fskPath);
	return (NULL != *nativePath) ? kFskErrNone : kFskErrMemFull;
}

FskErr FskFSFileCreate(const char *fullPath)
{
	int err;
	err = sCheckFullPath(fullPath, kFskPathIsFile);
	if (err)
		return err;

	err = OPEN(fullPath, CREATEFLAGS, 0666);
	if (err == -1)
		return errnoToFskErr(errno);
	else {
		close(err);
		return kFskErrNone;
	}
}

FskErr FskFSFileCreateDirectory(const char *fullPath)
{
	int err;

	err = sCheckFullPath(fullPath, kFskPathIsDirectory);
	if (err)
		return err;

	err = mkdir(fullPath, 0777);
	if (err == -1)
		return errnoToFskErr(errno);
	else
		return kFskErrNone;
}

FskErr FskFSFileRenameDirectory(const char *fullPath, const char *newName)
{
	int err, len;
	char *p, newPath[PATH_MAX];
	FskFileInfo itemInfo;

	err = sCheckFullPath(fullPath, kFskPathIsDirectory);
	if (err)
		return err;

	len = FskStrLen(fullPath);
	if (len < 2)
		return kFskErrOperationFailed;			// can't rename root

	err = FskFSFileGetFileInfo(fullPath, &itemInfo);
	if (err) return err;

	if (itemInfo.filetype != kFskDirectoryItemIsDirectory)
		return kFskErrNotDirectory;

	p = FskStrRChr((char *)newName, '/');
	if (p)
		return kFskErrOperationFailed;	// newName contains path elements

	FskStrCopy(newPath, fullPath);

	if (newPath[len - 1] == '/')
		newPath[len - 1] = '\0';		// remove trailing slash

	p = FskStrRChr(newPath, '/');
	if (p)
		*++p = '\0';
	FskStrCat(newPath, newName);

	err = rename(fullPath, newPath);
	if (err == -1)
		return errnoToFskErr(errno);
	else
		return kFskErrNone;
}

FskErr FskFSFileDeleteDirectory(const char *fullPath)
{
	int err;

	err = sCheckFullPath(fullPath, kFskPathIsDirectory);
	if (err)
		return err;

	err = rmdir(fullPath);
	if (err == -1)
		return errnoToFskErr(errno);
	else
		return kFskErrNone;
}

// ---------------------------------------------------------------------
// Volume Iterator
// ---------------------------------------------------------------------
#if !TARGET_OS_MAC
typedef struct {
	char *name;
	int	type;
	Boolean removable;
} mntdevmap;
static mntdevmap gMntDevMap[] = {
	{ "usbdevfs", kFskVolumeTypeMemoryStick, true },
	{ "iso9660", kFskVolumeTypeOptical, true },
	{ "rootfs", kFskVolumeTypeFixed, false },
	{ "jffs2", kFskVolumeTypeFixed, false },
	{ "vfat", kFskVolumeTypeSDMemory, true },
	{ "ext3", kFskVolumeTypeOptical, false },
	{ "ext", kFskVolumeTypeFixed, false },
	{ "udf", kFskVolumeTypeOptical, true },
	{ "nfs", kFskVolumeTypeNetwork, false },
	{ NULL, 0, false }
};
char *copyNiceName(char *name) {
	char *b;

	b = FskStrRChr(name, '/');
	if (b)
		b++;
	else
		b = name;

	return FskStrDoCopy(b);
}

void terminateVolumeList() {
	volInfo vol;
	while ( vol = FskListRemoveFirst((void **)&volumeList) ) {
		FskMemPtrDispose(vol->name);	
		FskMemPtrDispose(vol->mountPoint);	
		FskMemPtrDispose(vol->typeStr);	
		FskMemPtrDispose(vol);	
	}
}

void scanVolumes() {
	FILE *mountList;
	struct mntent	*cur;
	int err;
	volInfo	vi;
	mntdevmap *walker;
	struct statfs fsinfo;
	mountList = setmntent(kFskLinuxVolumeListFile, "r");
	if (mountList) {
		while (NULL != (cur = getmntent(mountList))) {
			err = statfs(cur->mnt_dir, &fsinfo);
			if (0 != err) {
					FskLinuxFilesPrintfDebug("problems getting statfs for %s - %d\n", cur->mnt_dir, errno);
				continue;
			}

			for (walker = gMntDevMap; NULL != walker->name; walker++) {
				if (0 == FskStrCompareWithLength(cur->mnt_type, walker->name, FskStrLen(walker->name)))
					break;
			}
			
			if (!walker->name) {
//					FskLinuxFilesPrintfDebug("I don't know what to do with the type %s, punt on this one\n", cur->mnt_type);
				continue;
			}

			FskMemPtrNewClear(sizeof(volInfoRec), &vi);
			vi->type = walker->type;
			vi->removable = walker->removable;
			vi->mounted = true;
			vi->typeStr = FskStrDoCopy(cur->mnt_type);
			FskMemPtrNew(FskStrLen(cur->mnt_dir) + 2, &vi->mountPoint);
			FskStrCopy(vi->mountPoint, cur->mnt_dir);
			vi->name = copyNiceName(cur->mnt_fsname);
			if (vi->mountPoint[FskStrLen(vi->mountPoint)-1] != '/')
				FskStrCat(vi->mountPoint, "/");
			vi->capacity = fsinfo.f_blocks * fsinfo.f_bsize;
			vi->remaining = fsinfo.f_bavail * fsinfo.f_bsize;
			FskLinuxFilesPrintfDebug("Volume scanned - name: %s, mountpoint: %s, fsname: %s\n", vi->name, vi->mountPoint, cur->mnt_fsname);
			FskListAppend(&volumeList, vi);
		}
		endmntent(mountList);
	}
}

FskErr FskFSVolumeIteratorNew(FskFSVolumeIterator *volIt)
{
	FskErr	ret = kFskErrNone;
	FskFSVolumeIterator	vol;

	if (!volumeList)
		scanVolumes();

	ret = FskMemPtrNewClear(sizeof(FskFSVolumeIteratorRecord), (FskMemPtr*)&vol);
	if (kFskErrNone == ret) {
		vol->volumeInfo = volumeList;
	}

bail:
	if (kFskErrNone == ret) {
		*volIt = vol;
	}
	else {
		*volIt = NULL;
		FskMemPtrDispose(vol);
	}
	return ret;
}

// ---------------------------------------------------------------------
FskErr FskFSVolumeIteratorDispose(FskFSVolumeIterator volIt)
{
	if (volIt) {
		FskMemPtrDispose(volIt);
	}
	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskFSVolumeIteratorGetNext(FskFSVolumeIterator volIt, UInt32 *id, char **path, char **name)
{
	FskErr ret = kFskErrIteratorComplete;

	if (volIt) {
		if (volIt->volumeInfo) {
			volInfo vi = (volInfo)volIt->volumeInfo;
tryAgain:
			if (id)
				*id = vi;
			if (path)
				*path = FskStrDoCopy(vi->mountPoint);
			if (name)
				*name = FskStrDoCopy(vi->name);
			FskLinuxFilesPrintfDebug("VolumeIteratorGetNext returns for %d, %s at %s\n", vi, vi->name, vi->mountPoint);
			volIt->volumeInfo = vi->next;
			ret = kFskErrNone;
		}
	}

bail:
	return ret;
}
FskErr FskFSVolumeGetInfo(UInt32 volumeID, char **pathOut, char **nameOut, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace)
{
	volInfoRec *vi;
	FskErr	err = kFskErrOperationFailed;

	if (!volumeList)
		scanVolumes();
	vi = volumeList;
	while (vi) {
		if (volumeID == vi) {
			if (pathOut)
				*pathOut = FskStrDoCopy(vi->mountPoint);
			if (nameOut)
				*nameOut = FskStrDoCopy(vi->name);
			if (volumeType)
				*volumeType = vi->type;
			if (isRemovable)
				*isRemovable = vi->removable;
			if (capacity)
				*capacity = vi->capacity;
			if (freeSpace)
				*freeSpace = vi->remaining;

			err = kFskErrNone;
			break;
		}
		vi = vi->next;
	}

	return err;
}

FskErr FskFSVolumeGetInfoFromPath(const char *pathIn, char **pathOut, char **nameOut, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace) {
	volInfo vi;
	FskErr	err = kFskErrOperationFailed;

	if (!volumeList)
		scanVolumes();

	vi = volumeList;
	while (vi) {
		if (0 == FskStrCompare(vi->mountPoint, pathIn)) {
			err = FskFSVolumeGetInfo(vi, pathOut, nameOut, volumeType, isRemovable, capacity, freeSpace);
			break;
		}
		vi = vi->next;
	}

	return err;
}

FskErr FskFSVolumeGetDeviceInfo(UInt32 volumeID, char **vendor, char **product, char **revision, char **vendorSpecific) {
	
	if (vendor)
		*vendor = FskStrDoCopy("vendor");
	if (product)
		*product = FskStrDoCopy("product");
	if (revision)
		*revision = FskStrDoCopy("1.0");
	if (vendorSpecific)
		*vendorSpecific = FskStrDoCopy("vendorSpecific");

	return kFskErrNone;
}
#endif	/* !TARGET_OS_MAC */


// ---------------------------------------------------------------------
// Directory Iterator
// ---------------------------------------------------------------------
FskErr FskFSDirectoryIteratorNew(const char *directoryPath, FskFSDirectoryIterator *dirIt, UInt32 flags)
{
	FskFSDirectoryIterator di = NULL;
	FskErr	err;
	FskFileInfo itemInfo;

	if (directoryPath[FskStrLen(directoryPath) -1] != '/') {
		BAIL(kFskErrInvalidParameter);
	}

	err = FskFSFileGetFileInfo(directoryPath, &itemInfo);
	BAIL_IF_ERR(err);

	if (itemInfo.filetype != kFskDirectoryItemIsDirectory) {
		BAIL(kFskErrNotDirectory);
	}

	err = FskMemPtrNew(sizeof(FskFSDirectoryIteratorRecord), (FskMemPtr*)&di);
	if (err != kFskErrNone)
		return err;

    di->flags = flags;
	di->root = (unsigned char *)FskStrDoCopy(directoryPath);

	di->theDir = opendir(directoryPath);
	if (di->theDir == NULL) {
		FskMemPtrDispose(di->root);
		err = errnoToFskErr(err);
		goto bail;
	}

	*dirIt = (FskFSDirectoryIterator)di;
	err = kFskErrNone;

bail:
	if (err) {
		if (di && di->root)
			FskMemPtrDispose(di->root);
		if (di)
			FskMemPtrDispose(di);
	}
	return err;
}

// ---------------------------------------------------------------------
FskErr FskFSDirectoryIteratorDispose(FskFSDirectoryIterator dirIt)
{
	if (dirIt && dirIt->theDir) {
		closedir(dirIt->theDir);
		FskMemPtrDispose(dirIt->root);
		FskMemPtrDispose(dirIt);
		return kFskErrNone;
	}

	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskFSDirectoryIteratorGetNext(FskFSDirectoryIterator dirIt, char **name, UInt32 *itemType)
{
	struct dirent *ent;
	FskErr	err;
	FskFileInfo finfo;

	if (!dirIt || !dirIt->theDir)
		return kFskErrInvalidParameter;

	ent = readdir(dirIt->theDir);
	if (!ent)
        return kFskErrIteratorComplete;

	err = FskFSFileGetPathInfo((char *)dirIt->root, ent->d_name, &finfo);
	if (err != kFskErrNone)
		return err;

	if (itemType)
		*itemType = finfo.filetype;

    if ('.' == ent->d_name[0]) {
        if ((0 == (dirIt->flags & 1)) ||
            (0 == FskStrCompare(ent->d_name, ".")) ||
            (0 == FskStrCompare(ent->d_name, "..")))
            return FskFSDirectoryIteratorGetNext(dirIt, name, itemType);        // skip hidden files
    }

    if (name)
        *name = FskStrDoCopy(ent->d_name);

	return kFskErrNone;
}

#if !TARGET_OS_MAC
FskErr FskFSDirectoryGetSpecialPath(UInt32 type, const Boolean create, const char *volumeName, char **fullPath)
{
	FskErr	err = kFskErrFileNotFound;
	FskFileInfo finfo;
	char *tmp = NULL, *specialDirEnv = NULL;

	*fullPath = NULL;

	switch (type & ~kFskDirectorySpecialTypeSharedFlag) {
		case kFskDirectorySpecialTypeDocument:
			specialDirEnv = "DocumentsDir";
			break;
		case kFskDirectorySpecialTypePhoto:
			specialDirEnv = "PhotosDir";
			break;
		case kFskDirectorySpecialTypeMusic:
			specialDirEnv = "MusicDir";
			break;
		case kFskDirectorySpecialTypeVideo:
			specialDirEnv = "VideoDir";
			break;
		case kFskDirectorySpecialTypeTV:
			specialDirEnv = "TVDir";
			break;
		case kFskDirectorySpecialTypeApplicationPreference:
			tmp = FskEnvironmentGet("PreferencesDir");
			if (!tmp)
				*fullPath = FskStrDoCat(getenv("HOME"), "/.kinoma/");
			else
				*fullPath = FskStrDoCopy(tmp);
			BAIL(kFskErrNone);
		case kFskDirectorySpecialTypeTemporary:
			*fullPath = FskStrDoCopy("/tmp/");
			BAIL(kFskErrNone);
		case kFskDirectorySpecialTypeCache:
			*fullPath = FskStrDoCopy("/tmp/");
			BAIL(kFskErrNone);
		default:
			BAIL(kFskErrInvalidParameter);
	}

	if (specialDirEnv)
		tmp = FskStrDoCopy(FskEnvironmentGet(specialDirEnv));
	if (!tmp) {
		char *home;
		home = getenv("HOME");
		FskMemPtrNewClear(FskStrLen(home) + FskStrLen(specialDirEnv) + 3, (FskMemPtr*)&tmp);
		FskStrCopy(tmp, home);
		if (tmp[FskStrLen(tmp)-1] != '/')
			FskStrCat(tmp, "/");
		FskStrCat(tmp, specialDirEnv);
		FskStrCat(tmp, "/");
	}
	FskLinuxFilesPrintfDebug("looking for %s - got %s\n", specialDirEnv, tmp);
	*fullPath = tmp;

	if (create) {
		err = FskFSFileCreateDirectory(*fullPath);
	}
	else {
		err = FskFSFileGetFileInfo(*fullPath, &finfo);
		if (kFskErrNone == err) {
			if (kFskDirectoryItemIsDirectory != finfo.filetype)
				err = kFskErrNotDirectory;
		}
	}

bail:
	if (kFskErrFileExists == err)
		err = kFskErrNone;
	return err;
}
#endif	/* !TARGET_OS_MAC */

FskErr FskFSFileMap(const char *fullPath, unsigned char **data, FskInt64 *dataSize, FskFSFileMapping *mapOut)
{
    FskErr err = kFskErrNone;
    FskFSFileMapping map = NULL;
    FskInt64 size;
	int fp;
struct stat statbuf;

	err = sCheckFullPath(fullPath, kFskPathIsFile);
	if (err)
		return err;

    err = FskMemPtrNewClear(sizeof(FskFSFileMappingRecord), (FskMemPtr *)&map);
    BAIL_IF_ERR(err);

    map->file = -1;

	fp = open(fullPath, O_RDONLY);
	if (fp < 0) {
		err = errnoToFskErr(errno);
		goto bail;
	}
	map->file = fp;

	fstat(map->file, &statbuf);
	size = statbuf.st_size;
	if (size > 0xffffffff) {
		BAIL(kFskErrOperationFailed);
	}

	map->length = size;
	map->address = mmap(NULL, map->length, PROT_READ, MAP_SHARED, map->file, 0);
	if (MAP_FAILED == map->address) {
		map->address = NULL;
		BAIL(kFskErrOperationFailed);
	}

	*data = (unsigned char *)map->address;
	*dataSize = size;

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
	if (map) {
		if (map->address) {
			munmap(map->address, map->length);
		}
        if (map->file >= 0)
            close(map->file);
		FskMemPtrDispose(map);
	}
	return kFskErrNone;
}

FskErr FskFSFileResolveLink(const char *fullPath, char **resolved)
{
	FskErr	err = kFskErrNone;
	char 	linkPath[PATH_MAX];

	if (realpath(fullPath, linkPath) == NULL) {
		BAIL(kFskErrOperationFailed);
	}

	err = FskMemPtrNewFromData(FskStrLen(linkPath) + 1, linkPath, (FskMemPtr *)resolved);
	BAIL_IF_ERR(err);

bail:
	return err;
}



#if !TARGET_OS_MAC
FskErr FskFSFileTerminate() {

	terminateVolumeList();

	return kFskErrNone;
}
#endif	/* !TARGET_OS_MAC */
