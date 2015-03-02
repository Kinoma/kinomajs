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
#include "Fsk.h"
#include "FskList.h"
#define __FSKFILES_PRIV__
#define __FSKFSFILES_PRIV__
#include "FskFS.h"
#include "FskUtilities.h"
#include "FskHardware.h"
#include "FskEnvironment.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>		// mmap'd files
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <utime.h>

#include <mntent.h>

FskInstrumentedSimpleType(AndroidFiles, androidfiles);

FskErr FskFSDirectoryChangeNotifierNew(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNtf);
FskErr FskFSDirectoryChangeNotifierDispose(FskDirectoryChangeNotifier dirChangeNtf);

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

#if BAD_FTRUNCATE
	int FTRUNCATE(FILE *theFile, FILEOFFSET length);
#else
	#define FTRUNCATE(a,b) ftruncate(fileno(a),b)
#endif

const char kInternalDataStorageName[] = "(Kinoma Internal Storage)";
const char kExternalDataStorageName[] = "Card";

static FskErr FskFSFileGetPathInfo(const char *rootpath, const char *filepath, FskFileInfo *itemInfo);

static char *androidSystemMusicDir = NULL;
static char *androidSystemPodcastDir = NULL;
static char *androidSystemPicturesDir = NULL;
static char *androidSystemMoviesDir = NULL;
static char *androidSystemDownloadsDir = NULL;
static char *androidSystemDcimDir = NULL;

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
		case EBADF:							/* Bad file descriptor */
		case EINVAL:						/* Invalid argument */
		case EFAULT:						/* Bad address */
			ret = kFskErrInvalidParameter;
			break;
		case EACCES:						/* Permission denied */
			ret = kFskErrFilePermissions;
			break;
		case EISDIR:						/* Is a directory */
			ret = kFskErrIsDirectory;
			break;
		case ENOENT:						/* No such file or directory */
			ret = kFskErrFileNotFound;
			break;
		case ENOTEMPTY:						/* Directory not empty */
		case EEXIST:						/* File exists */
			ret = kFskErrFileExists;
			break;
		case EROFS:							/* Read-only file system */
			ret = kFskErrReadOnly;
			break;
		case EFBIG:							/* File too large */
		case ENOSPC:						/* No space left on device */
			ret = kFskErrDiskFull;
			break;
		case ENOTDIR:						/* Not a directory */
			ret = kFskErrNotDirectory;
			break;
		case EMFILE:						/* Too many open files */
		case ENFILE:						/* Too many open files in system */
			ret = kFskErrTooManyOpenFiles;
			break;
		case ENOMEM:						/* Cannot allocate memory */
			ret = kFskErrMemFull;
			break;
		case EPERM:							/* Operation not permitted */
			ret = kFskErrOperationFailed;
			break;
		case EIO:							/* Input/output error */
			ret = kFskErrFileRecoveryImpossible;
			break;
		case ENAMETOOLONG:					/* File name too long */
			ret = kFskErrURLTooLong;
			break;
		case EOVERFLOW:						/* Value too large to be stored in data type */
			ret = kFskErrRequestTooLarge;
			break;
		default:
			ret = kFskErrUnknown;
			break;
	}

	return ret;
}

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

		(FskVolumeNotifierNewProc)FskFSVolumeNotifierNew,
		(FskVolumeNotifierDisposeProc)FskFSVolumeNotifierDispose,
		(FskVolumeGetInfoProc)FskFSVolumeGetInfo,
		(FskVolumeGetInfoFromPathProc)FskFSVolumeGetInfoFromPath,
		(FskVolumeGetDeviceInfoProc)FskFSVolumeGetDeviceInfo,
		(FskVolumeGetIDProc)FskFSVolumeGetID,
		(FskVolumeEjectProc)NULL,

		(FskFileMapProc)FskFSFileMap,
		(FskFileDisposeMapProc)FskFSFileDisposeMap,

		(FskFileChooseProc)NULL,
		(FskFileChooseSaveProc)NULL,
		(FskDirectoryChooseProc)NULL,

		(FskFileTerminateProc)FskFSFileTerminate,

		(FskDirectoryChangeNotifierNewProc)FskFSDirectoryChangeNotifierNew,
		(FskDirectoryChangeNotifierDisposeProc)FskFSDirectoryChangeNotifierDispose,

		(FskFileInitializeProc)FskFSFileInitialize,
		(FskFileResolveLinkProc)FskFSFileResolveLink
};

FskErr FskFSFileFilter(const char *fullPath, SInt32 *priority)
{
        *priority = 0;
        return kFskErrNone;
}


enum {
	kFskPathIsAny = 0,
	kFskPathIsFile = 1,
	kFskPathIsDirectory = 2
};

FskErr sCheckFullPath(const char *fullPath, UInt32 pathType) {
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

	if (frefOut)
		*frefOut = NULL;

	err = sCheckFullPath(fullPath, kFskPathIsFile);
	BAIL_IF_ERR(err);

	err = FskFSFileGetFileInfo(fullPath, &itemInfo);
	BAIL_IF_ERR(err);

	if (itemInfo.filetype == kFskDirectoryItemIsDirectory)
		BAIL(kFskErrIsDirectory);

	err = FskMemPtrNewClear(sizeof(FskFSFileRecord), (FskMemPtr*)(void*)&fref);
	BAIL_IF_NONZERO(err, err, kFskErrMemFull);

	fref->thePermissions = permissions;

	fref->theFile = FOPEN(fullPath, sPermsToPermStr(permissions));
	if (!fref->theFile) {
		FskMemPtrDispose(fref);
		BAIL(errnoToFskErr(errno));
	}

	*frefOut = fref;
	
bail:
	return err;
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
	int	ret;

	ret = FTELL(fref->theFile);
	if (ret < 0) goto bail;

	pos = ret;
	FSEEK(fref->theFile, 0, SEEK_END);
	ret = FTELL(fref->theFile);
	if (ret < 0) goto bail;

	end = ret;
	ret = FSEEK(fref->theFile, pos, SEEK_SET);

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
	int ret;

	if (!fref)
		return kFskErrInvalidParameter;

	if (fref->thePermissions == kFskFilePermissionReadOnly)
		return kFskErrReadOnly;

	fref->flushBeforeRead = false;
	fref->flushBeforeWrite = false;

	pos = *size;
	oldpos = FTELL(fref->theFile);
	if (oldpos > pos)
		ret = FSEEK(fref->theFile, pos, SEEK_SET);

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
	itemInfo->fileCreationDate = statbuf->st_ctime;
	itemInfo->fileDevice = statbuf->st_dev;
	itemInfo->fileNode = statbuf->st_ino;
}

// ---------------------------------------------------------------------
FskErr FskFSFileGetFileInfo(const char *fullPath, FskFileInfo *itemInfo) {
	int err;
	STATTYPE statbuf;

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
FskErr FskFSFileGetPathInfo(const char *rootpath, const char *filepath, FskFileInfo *itemInfo) {
	int err;
	STATTYPE statbuf;
	char	*fullpath;

	err = FskMemPtrNew(FskStrLen(rootpath) + FskStrLen(filepath) + 2, (FskMemPtr*)(void*)&fullpath);
	if (err)
		return err;

	FskStrCopy(fullpath, rootpath);
	FskStrCat(fullpath, "/");
	FskStrCat(fullpath, filepath);

	err = STAT(fullpath, &statbuf);
	if (err == -1) {
		err = errnoToFskErr(errno);
		goto bail;
	}

	GetFileInfoFromStat(&statbuf, itemInfo);

bail:
	FskMemPtrDispose(fullpath);
	if (err)
		FskMemSet(itemInfo, 0, sizeof(*itemInfo));
	return err;
}

FskErr FskFSFileDelete(const char *fullPath)
{
	int err;

	err = sCheckFullPath(fullPath, kFskPathIsFile);
	if (err)
		return err;

	err = unlink(fullPath);
	if (err == -1)
		return errnoToFskErr(errno);
	else
		return kFskErrNone;
}

FskErr FskFSFileRename(const char *fullPath, const char *newName)
{
	int err;
	char *p, newPath[PATH_MAX];
	FskFileInfo itemInfo;

	err = sCheckFullPath(fullPath, kFskPathIsFile);
	BAIL_IF_ERR(err);

	err = FskFSFileGetFileInfo(fullPath, &itemInfo);
	BAIL_IF_ERR(err);

	if (itemInfo.filetype == kFskDirectoryItemIsDirectory)
		BAIL(kFskErrIsDirectory);

	p = FskStrRChr((char *)newName, '/');
	if (p)
		BAIL(kFskErrOperationFailed);	// newName contains path elements

	FskStrCopy(newPath, fullPath);
	p = FskStrRChr(newPath, '/');
	if (p)
		*++p = '\0';
	FskStrCat(newPath, newName);

	err = rename(fullPath, newPath);
	if (err == -1)
		BAIL(errnoToFskErr(errno));

	err = kFskErrNone;

bail:
	return err;
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
	BAIL_IF_ERR(err);

	len = FskStrLen(fullPath);
	if (len < 2)
		BAIL(kFskErrOperationFailed);			// can't rename root

	err = FskFSFileGetFileInfo(fullPath, &itemInfo);
	BAIL_IF_ERR(err);

	if (itemInfo.filetype != kFskDirectoryItemIsDirectory)
		BAIL(kFskErrNotDirectory);

	p = FskStrRChr((char *)newName, '/');
	if (p)
		BAIL(kFskErrOperationFailed);	// newName contains path elements

	FskStrCopy(newPath, fullPath);

	if (newPath[len - 1] == '/')
		newPath[len - 1] = '\0';		// remove trailing slash

	p = FskStrRChr(newPath, '/');
	if (p)
		*++p = '\0';
	FskStrCat(newPath, newName);

	err = rename(fullPath, newPath);
	if (err == -1)
		BAIL(errnoToFskErr(errno));

	 err = kFskErrNone;

bail:
	return err;
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
typedef struct sMountRec {
	struct sMountRec *next;
	char	*mountPoint;
	char	*name;
	UInt32	type;
	Boolean	removable;
	int		fsID;
} sMountRec, *pMount;

pMount gMounts = NULL;

// /etc/vold.fstab contains definitions for the removable storage
// use that (if available) to vet the /proc/mounts for removable storage

#define VOLD_FSTAB_FILE	"/etc/vold.fstab"
typedef struct extMountRec {
	struct extMountRect	*next;
	char 	*label;
	char 	*mountPoint;
} extMountRec, *extMount;

extMount	gExtMounts = NULL;

typedef struct fstabDescRec {
	const char	*label;
	const char	*name;
	int		type;
	int		removable;
} fstabDescRec, *fstabDesc;

fstabDescRec gfstabDesc[] = {
	{ "external_sdcard",	"Card",			kFskVolumeTypeSDMemory,	1 },
	{ "internal_sdcard",	"Built-in",		kFskVolumeTypeSDMemory,	0 },
	{ "internalstorage",	"Built-in",		kFskVolumeTypeSDMemory, 0 },
	{ "right_sdcard",		"Card",			kFskVolumeTypeSDMemory,	1 },
	{ "left_sdcard",		"Card",			kFskVolumeTypeSDMemory,	1 },
	{ "extsdcard",			"Card",			kFskVolumeTypeSDMemory,	1 },
	{ "sdreader",			"Card",			kFskVolumeTypeSDMemory,	1 },
	{ "microsd",			"Card",			kFskVolumeTypeSDMemory, 1 },
	{ "usbdisk",			"Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sdcard",				"Card",			kFskVolumeTypeSDMemory, 1 },
	{ "udisk",				"Drive",		kFskVolumeTypeSDMemory, 1 },
	{ "emmc",				"Built-in",		kFskVolumeTypeSDMemory,	0 },
	{ "sda",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sdb",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sdc",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sdd",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sde",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sdf",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sdg",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sdh",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sdi",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sdj",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sdk",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "sdl",				"USB Drive",		kFskVolumeTypeSDMemory,	1 },
	{ "/",	kInternalDataStorageName,		kFskVolumeTypeFixed, 	0 },
	{ "",					"",				0,						0 }
};

fstabDesc extMountToType(extMount ext) {
	int iter = 0;
	if (ext && ext->label) {
		int len;
		while ((len = (FskStrLen(gfstabDesc[iter].label)))) {
			if (0 == FskStrCompareWithLength(ext->label, gfstabDesc[iter].label, len)) {
				return &(gfstabDesc[iter]);
			}
			iter++;
		}
		FskAndroidFilesPrintfDebug("#### unknown external filesystem type - %s\n", ext->label);
	}

	return NULL;
}

pMount volumesContain(char *path) {
	pMount mnt = gMounts;
	while (mnt) {
		if (0 == FskStrCompare(path, mnt->mountPoint))
			break;
		mnt = mnt->next;
	}
	return mnt;
}

extMount extMountsContain(char *path) {
	extMount mnt = gExtMounts;

	while (mnt) {
		if (0 == FskStrCompare(mnt->mountPoint, path))
			break;
		mnt = (extMount)mnt->next;
	}

	return mnt;
}

// /proc/mounts format is like fstab format
// fs_spec fs_file fs_vfstype fs_mntops fs_freq fs_passno
#define LINUX_MOUNT_FILE	"/proc/mounts"
#define MOUNT_BUF_SIZE		2048

static int sNumMounts = 0;
static int gNumCards = 0;


void addVolume(char *path, int type, int removable) {
	pMount 	mnt;
	char 	*tmp;
	FskErr	err;

	if ('/' == path[FskStrLen(path)-1])
		tmp = FskStrDoCopy(path);
	else
		tmp = FskStrDoCat(path, "/");

	if (NULL != volumesContain(tmp)) {
		FskMemPtrDispose(tmp);
		return;
	}

	err = FskMemPtrNewClear(sizeof(sMountRec), &mnt);
	BAIL_IF_ERR(err);
	mnt->mountPoint = tmp;

	if (sNumMounts == 0) {
		mnt->name = FskStrDoCopy(kInternalDataStorageName);
	}
	else {
		gNumCards++;
		if (gNumCards > 1) {
			char *name = FskStrDoCat("Card (", path);
			mnt->name = FskStrDoCat(name, ")");
			FskMemPtrDispose(name);
		}
		else {
			mnt->name = FskStrDoCopy("Card");
		}
	}
	mnt->type = type;
	mnt->removable = removable;
	mnt->fsID = ++sNumMounts;
	FskListAppend(&gMounts, mnt);

bail:
	;
}


// scanProcMount
void scanProcMount() {
	FskErr err;
	FILE *mntFile = NULL;
	char *mntFileBuffer;
	int bufEnd = 0;
	int done = 0;
	int amt;
	char *bufPos, *lineEnd, *path;
	extMount ext = NULL;
	fstabDesc fsDesc = NULL;

	// the default
	addVolume(gAndroidCallbacks->getStaticDataDirCB(), kFskVolumeTypeFixed, 0);
	// and the Jellybean user-space
	addVolume(gAndroidCallbacks->getStaticExternalDirCB(), kFskVolumeTypeSDMemory, 0);

	// read mount file
	err = FskMemPtrNew(MOUNT_BUF_SIZE+1, &mntFileBuffer);
	BAIL_IF_ERR(err);
	bufPos = mntFileBuffer;

	mntFile = FOPEN(LINUX_MOUNT_FILE, "r");
	if (NULL == mntFile) {
		FskAndroidFilesPrintfDebug("opening %s - %d\n", LINUX_MOUNT_FILE, errno);
		return;
	}

	while (!done) {
		// fill the buffer
		amt = fread(mntFileBuffer + bufEnd, 1, MOUNT_BUF_SIZE - bufEnd, mntFile);
		mntFileBuffer[bufEnd + amt + 1] = '\0';

		FskAndroidFilesPrintfDebug("fread %x, %d - got %d\n", mntFileBuffer + bufEnd, MOUNT_BUF_SIZE - bufEnd, amt);
		if (amt > 0)
			bufEnd += amt;
		FskAndroidFilesPrintfDebug("check while(bufpos...) - %p < %p (%d left)\n", bufPos, mntFileBuffer + bufEnd, mntFileBuffer+bufEnd - bufPos);
		while (bufPos < mntFileBuffer + bufEnd) {
			char fsName[256], fsMnt[256], fsType[32];

			// until we have a full line
			if (NULL == (lineEnd = FskStrChr(bufPos, kFskLF)))
				break;

			bufPos = FskStrNCopyUntilSpace(fsName, bufPos, 255);
			bufPos = FskStrNCopyUntilSpace(fsMnt, bufPos, 255);
			bufPos = FskStrNCopyUntilSpace(fsType, bufPos, 31);
			FskAndroidFilesPrintfDebug("got Name: %s Mnt: %s Type: %s\n", fsName, fsMnt, fsType);

			path = fsMnt;
			if ((ext = extMountsContain(path))) {
				FskAndroidFilesPrintfDebug("found path in external mounts %s\n", path);
				fsDesc = extMountToType(ext);
				if (fsDesc)
					addVolume(path, fsDesc->type, fsDesc->removable);
				else
					addVolume(path, kFskVolumeTypeSDMemory, 1);
				goto nextLine;
			}
			FskAndroidFilesPrintfDebug(" - didn't find path in external mounts %s\n", path);

			if (0 == FskStrCompare(fsType, "vfat") || 0 == FskStrCompare(fsType, "fuse")) {
				if (0 != FskStrStr(fsMnt, "emulated")) {
					FskAndroidFilesPrintfDebug(" - emulated - ignore\n");
					goto nextLine;
				}
				path = fsMnt;
				FskAndroidFilesPrintfDebug(" - got a vfat (or fuse) - path is %s\n", path);
				if (0 == FskStrStr(path, "/asec")) {
					FskAndroidFilesPrintfDebug(" - vfat without asec: %s\n", path);
					addVolume(path, kFskVolumeTypeSDMemory, 1);
				}
				else {
					FskAndroidFilesPrintfDebug(" - vfat with asec - ignore - %s\n", path);
				}
			}
nextLine:
			bufPos = lineEnd + 1;
		}

		if (amt == 0) {	// we read no more
			done = 1;
		}
		else {
			// push buffer to beginning
			amt = (mntFileBuffer + bufEnd) - bufPos;
			FskAndroidFilesPrintfDebug("push unread %d bytes to beginning of buffer\n - mntFileBuffer: %d  bufEnd: %d  bufPos: %d\n", amt, mntFileBuffer, bufEnd, bufPos);
			if (amt > 0)
				FskMemCopy(mntFileBuffer, bufPos, amt);
			bufPos = mntFileBuffer;
			bufEnd -= amt;
		}
	}

bail:
	if (NULL != mntFile)
		fclose(mntFile);
}

//
// /etc/vold.fstab
// Format: dev_mount <label> <mount_point> <part> <sysfs_path1...>
//
void scanVoldFstab() {
	FskErr err;
	FILE *mntFile;
	char *mntFileBuffer;
	int bufEnd = 0;
	int done = 0;
	int amt;
	char *bufPos, *lineEnd;

	mntFile = FOPEN(VOLD_FSTAB_FILE, "r");
	if (NULL == mntFile) {
		FskAndroidFilesPrintfDebug("failed opening %s - %d\n", VOLD_FSTAB_FILE, errno);
//		oldscanVolumes();
		return;
	}

	err = FskMemPtrNew(MOUNT_BUF_SIZE + 1, &mntFileBuffer);
	BAIL_IF_ERR(err);
	bufPos = mntFileBuffer;

	while (!done) {
		// fill the buffer
		amt = fread(mntFileBuffer + bufEnd, 1, MOUNT_BUF_SIZE - bufEnd, mntFile);
		FskAndroidFilesPrintfDebug("fread %x, %d - got %d\n", mntFileBuffer + bufEnd, MOUNT_BUF_SIZE - bufEnd, amt);
		if (amt > 0) {
			bufEnd += amt;
		}
		mntFileBuffer[bufEnd] = '\0';

		FskAndroidFilesPrintfDebug("check while(bufpos...) - %d < %d\n", bufPos, mntFileBuffer + bufEnd);
		while (bufPos < mntFileBuffer + bufEnd) {
			extMount	mnt = NULL;
			char fsName[256], fsMnt[256], tmp[256], fsPath[256];

			// Format: dev_mount <label> <mount_point> <part> <sysfs_path1...>
			if (NULL == (lineEnd = FskStrChr(bufPos, kFskLF)))
				break;

			bufPos = FskStrNCopyUntilSpace(tmp, bufPos, 255);
			if (0 != FskStrCompare(tmp, "dev_mount")) {
				// no dev_mount, move bufPos to end of line and then continue;
				FskAndroidFilesPrintfDebug("didn't find dev_mount in '%s' - bail\n", tmp);
				bufPos = lineEnd + 1;
				continue;
			}

			// Format: dev_mount <label> <mount_point> <part> <sysfs_path1...>
			bufPos = FskStrNCopyUntilSpace(fsName, bufPos, 255);
			bufPos = FskStrNCopyUntilSpace(fsMnt, bufPos, 255);
			bufPos = FskStrNCopyUntilSpace(tmp, bufPos, 255);
			bufPos = FskStrNCopyUntilSpace(fsPath, bufPos, 255);
			FskAndroidFilesPrintfDebug("got Name: %s Mnt: %s tmp: %s fsPath: %s\n", fsName, fsMnt, tmp, fsPath);

			err = FskMemPtrNew(sizeof(extMountRec), &mnt);
			BAIL_IF_ERR(err);
			mnt->label = FskStrDoCopy(fsName);
			mnt->mountPoint = FskStrDoCopy(fsMnt);
			FskAndroidFilesPrintfDebug("Make an external filesystem record with label: %s, mountPoint: %s\n", mnt->label, mnt->mountPoint);
			FskListAppend(&gExtMounts, mnt);

			bufPos = lineEnd + 1;
		}

		if (amt == 0) {	// we read no more
			done = 1;
		}
		else {
			// push buffer to beginning
			amt = (mntFileBuffer + bufEnd) - bufPos;
			FskAndroidFilesPrintfDebug("push unread %d bytes to beginning of buffer\n - mntFileBuffer: %d  bufEnd: %d  bufPos: %d", amt, mntFileBuffer, bufEnd, bufPos);
			if (amt > 0) {
				FskMemCopy(mntFileBuffer, bufPos, amt);
				bufEnd = amt;
			}
			else
				bufEnd = 0;
			bufPos = mntFileBuffer;
		}
	}

bail:

	fclose(mntFile);
}

void scanVolumes() {
	scanVoldFstab();
	scanProcMount();
}


FskErr FskFSVolumeIteratorNew(FskFSVolumeIterator *volIt)
{
	FskErr	err = kFskErrNone;
	FskFSVolumeIterator	vol;

	if (!gMounts)
		scanVolumes();

	FskAndroidFilesPrintfDebug("FSVolumeIteratorNew -  \n");

	err = FskMemPtrNewClear(sizeof(FskFSVolumeIteratorRecord), (FskMemPtr*)(void*)&vol);
	BAIL_IF_ERR(err);
	*volIt = vol;


bail:
	if (err) {
		*volIt = NULL;
		FskMemPtrDispose(vol);
	}
	return err;
}

// ---------------------------------------------------------------------
FskErr FskFSVolumeIteratorDispose(FskFSVolumeIterator volIt)
{
	if (volIt)
		FskMemPtrDispose(volIt);

	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskFSVolumeIteratorGetNext(FskFSVolumeIterator volIt, UInt32 *id, char **path, char **name)
{
	FskErr ret = kFskErrIteratorComplete;

	if (volIt) {
		pMount theMount = gMounts;
		while (theMount) {
			if (theMount->fsID >= volIt->iter) {
				break;
			}
			theMount = theMount->next;
		}
		if (!theMount) {
			return ret;
		}

		if (id)
			*id = theMount->fsID;

		if (path) *path = FskStrDoCopy(theMount->mountPoint);
		if (name) *name = FskStrDoCopy(theMount->name);

		FskAndroidFilesPrintfDebug("VolIterGetNext iter: %d - path: %s\n", theMount->fsID, theMount->mountPoint);

		volIt->iter = theMount->fsID + 1;
		ret = kFskErrNone;

	}
	return ret;
}

FskErr FskFSVolumeGetInfo(UInt32 volumeID, char **pathOut, char **nameOut, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace)
{
	FskInt64 calc, more;
	pMount theMount;
	FskErr	err = kFskErrOperationFailed;
	struct statfs volStat;

	if (!gMounts)
		scanVolumes();
	theMount = gMounts;

	FskAndroidFilesPrintfDebug("FSVolumeGetInfo for id %d\n", volumeID);
	if (capacity)
		*capacity = 0;
	if (freeSpace)
		*freeSpace = 0;

	while (theMount) {
		if (theMount->fsID == (SInt32)volumeID) {
			break;
		}
		theMount = theMount->next;
	}
	if (theMount) {
		if (pathOut)		*pathOut = FskStrDoCopy(theMount->mountPoint);
		if (nameOut)		*nameOut = FskStrDoCopy(theMount->name);
		if (volumeType)		*volumeType = theMount->type;
		if (isRemovable)	*isRemovable = theMount->removable;

		FskAndroidFilesPrintfDebug("getInfo returns - path: %s, name: %s, ", theMount->mountPoint, theMount->name);

		statfs(theMount->mountPoint, &volStat);
		if (capacity) {
			calc = volStat.f_bsize;
			more = volStat.f_blocks;
			calc = calc * more;
			*capacity = calc;

			FskAndroidFilesPrintfDebug("capacity %d,", calc);
		}
		if (freeSpace) {
			calc = volStat.f_bsize;
			more = volStat.f_bavail;
			calc = calc * more;
			*freeSpace = calc;

			FskAndroidFilesPrintfDebug("freeSpace %d", calc);
		}
		err = kFskErrNone;
	}
	else {
		FskAndroidFilesPrintfDebug("getInfo fails\n");
	}

	return err;
}

FskErr FskFSVolumeGetInfoFromPath(const char *pathIn, char **pathOut, char **nameOut, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace) {
	FskErr	err = kFskErrOperationFailed;
	int drv = -1;

	FskAndroidFilesPrintfDebug("FskFSVolumeGetInfoFromPath: pathIn: %s\n", pathIn);

	if (0 == FskStrCompare("/", pathIn))
		drv = 1;
	else {
		pMount theMount = gMounts;
		while (theMount) {
			int len = FskStrLen(theMount->mountPoint);
			if (0 == FskStrCompareWithLength(theMount->mountPoint, pathIn, len)) {
				drv = theMount->fsID;
				break;
			}
			theMount = theMount->next;
		}
	}

	if (drv == -1) {
		FskAndroidFilesPrintfDebug("FskFSVolumeGetInfoFromPath: pathIn: %s - not found\n", pathIn);
		return err;
	}
	else
		return FskFSVolumeGetInfo(drv, pathOut, nameOut, volumeType, isRemovable, capacity, freeSpace);
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

FskErr FskFSVolumeGetID(const char *fullPath, UInt32 *volumeID) {
	const char *p;
	pMount theMount;

	if (!gMounts)
		scanVolumes();
	theMount = gMounts;

	p = fullPath;

	FskAndroidFilesPrintfDebug("FskFSVolumeGetID(%s, %x)\n", p, volumeID);

	*volumeID = -1;

	while (theMount) {
		int len = FskStrLen(theMount->mountPoint);
		if (0 == FskStrCompareWithLength(theMount->mountPoint, fullPath, len)) {
			*volumeID = theMount->fsID;
			break;
		}
		theMount = theMount->next;
	}

	return kFskErrNone;
}

// ---------------------------------------------------------------------
// Directory Iterator
// ---------------------------------------------------------------------
FskErr FskFSDirectoryIteratorNew(const char *directoryPath, FskFSDirectoryIterator *dirIt, UInt32 flags)
{
	FskFSDirectoryIterator di = NULL;
	FskErr	err;
	FskFileInfo itemInfo;

	FskAndroidFilesPrintfDebug("FSDirectoryIteratorNew: %s\n", directoryPath);
	if (directoryPath[FskStrLen(directoryPath) -1] != '/')
		BAIL(kFskErrInvalidParameter);

	err = FskFSFileGetFileInfo(directoryPath, &itemInfo);
	BAIL_IF_ERR(err);

	if (itemInfo.filetype != kFskDirectoryItemIsDirectory)
		BAIL(kFskErrNotDirectory);

	err = FskMemPtrNew(sizeof(FskFSDirectoryIteratorRecord), (FskMemPtr*)(void*)&di);
	BAIL_IF_ERR(err);

	di->root = (unsigned char *)FskStrDoCopy(directoryPath);
	di->theDir = opendir(directoryPath);
	if (di->theDir == NULL) {
		FskMemPtrDisposeAt(&di->root);
		BAIL(errnoToFskErr(err));
	}

	di->device = itemInfo.fileDevice;

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

	FskAndroidFilesPrintfDebug("FSDirectoryIteratorGetNext: %s\n", ent->d_name);
	err = FskFSFileGetPathInfo((char *)dirIt->root, ent->d_name, &finfo);
	if (err != kFskErrNone)
		return err;

	if (itemType)
		*itemType = finfo.filetype;

	if ((ent->d_name[0] == '.')						// skip hidden files
		|| (finfo.fileDevice != dirIt->device) ) {	// don't cross filesystems
		return FskFSDirectoryIteratorGetNext(dirIt, name, itemType);
	}
	else {
		if (name) {
			*name = FskStrDoCopy(ent->d_name);
		}
	}

	return kFskErrNone;
}


FskErr FskFSDirectoryGetSpecialPath(UInt32 type, const Boolean create, const char *volumeName, char **fullPath)
{
	FskErr	err = kFskErrFileNotFound;
	FskFileInfo finfo;
	char *tmp = NULL, *specialDir = NULL;
	char *dataDir = gAndroidCallbacks->getStaticDataDirCB();
	char *externalDir = gAndroidCallbacks->getStaticExternalDirCB();
	Boolean doCreate = create;

	*fullPath = NULL;

	FskAndroidFilesPrintfDebug("for SpecialPath - type %d, use dataDir as %s (volumeName %s)\n", type, dataDir, volumeName);

	switch (type & ~kFskDirectorySpecialTypeSharedFlag) {
		case kFskDirectorySpecialTypeDownload:
			if (androidSystemDownloadsDir)
				tmp = FskStrDoCat(androidSystemDownloadsDir, "/");
			else if (externalDir)
				tmp = FskStrDoCat(externalDir, "/Kinoma/Downloads/");
			else
				BAIL(kFskErrNotDirectory);
			break;
		case kFskDirectorySpecialTypeDocument:
			doCreate = true;
			if (externalDir)
				tmp = FskStrDoCat(externalDir, "/");
			else if (androidSystemDownloadsDir)
				tmp = FskStrDoCat(androidSystemDownloadsDir, "/");
			else
				specialDir = "Download";
			break;
		case kFskDirectorySpecialTypePhoto:
			if (androidSystemPicturesDir)
				tmp = FskStrDoCat(androidSystemPicturesDir, "/");
			else
				specialDir = "Pictures";
			break;
		case kFskDirectorySpecialTypeMusic:
			if (androidSystemMusicDir)
				tmp = FskStrDoCat(androidSystemMusicDir, "/");
			else
				specialDir = "Music";
			break;
		case kFskDirectorySpecialTypeVideo:
			if (androidSystemMoviesDir)
				tmp = FskStrDoCat(androidSystemMoviesDir, "/");
			else
				specialDir = "Movies";
			break;
		case kFskDirectorySpecialTypeTV:
			specialDir = "TV";
			break;
		case kFskDirectorySpecialTypeApplicationPreference:
			*fullPath = FskStrDoCat(dataDir, "kinoma/");
			err = FskFSFileCreateDirectory(*fullPath);
			err = kFskErrNone;
			goto makeit;
		case kFskDirectorySpecialTypeTemporary:
			*fullPath = FskStrDoCat(dataDir, "tmp/");
			err = FskFSFileCreateDirectory(*fullPath);
			err = kFskErrNone;
			goto makeit;
		case kFskDirectorySpecialTypeCache:
			*fullPath = FskStrDoCat(dataDir, "tmp/");
			err = FskFSFileCreateDirectory(*fullPath);
			err = kFskErrNone;
			goto makeit;
		default:
			FskAndroidFilesPrintfDebug("SpecialDirectory - bad special directory\n");
			BAIL(kFskErrInvalidParameter);
	}

	if (specialDir)
		tmp = FskStrDoCopy(FskEnvironmentGet(specialDir));

	if (!tmp) {
		char *home;
		home = dataDir;
		err = FskMemPtrNewClear(FskStrLen(home) + FskStrLen(specialDir) + 3, (FskMemPtr*)(void*)&tmp);
		BAIL_IF_ERR(err);
		FskStrCopy(tmp, home);
		if (tmp[FskStrLen(tmp)-1] != '/')
			FskStrCat(tmp, "/");
		if (specialDir) {
			FskStrCat(tmp, specialDir);
			FskStrCat(tmp, "/");
		}
	}
	FskAndroidFilesPrintfDebug("looking for %s - got %s", specialDir, tmp);
	*fullPath = tmp;

makeit:
	if (doCreate) {
		err = FskFSFileCreateDirectory(*fullPath);
	}
	else {
		err = FskFSFileGetFileInfo(*fullPath, &finfo);
		if (kFskErrNone == err) {
			if (kFskDirectoryItemIsDirectory != finfo.filetype)
				BAIL(kFskErrNotDirectory);
		}
	}

bail:
	if (kFskErrFileExists == err) {
		err = kFskErrNone;
		FskAndroidFilesPrintfDebug("DIRECTORY EXISTS specialDirectory - looking for type %d - got [%s]", type, *fullPath);
	}
	else if (kFskErrNone != err) {
		FskAndroidFilesPrintfDebug("DIRECGOTRY DIDN'T EXIST - specialDirectory - looking for type %d - got [%s] err: %d", type, *fullPath, err);
	}

	return err;
}

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

    err = FskMemPtrNewClear(sizeof(FskFSFileMappingRecord), (FskMemPtr*)(void*)&map);
	BAIL_IF_ERR(err);

	map->file = -1;

	fp = open(fullPath, O_RDONLY);
	if (fp < 0)
		BAIL(errnoToFskErr(errno));

	map->file = fp;

	fstat(map->file, &statbuf);
	size = statbuf.st_size;
	if (size > 0xffffffff)
		BAIL(kFskErrOperationFailed);

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

	if (realpath(fullPath, linkPath) == NULL)
		BAIL(kFskErrOperationFailed);

	err = FskMemPtrNewFromData(FskStrLen(linkPath) + 1, linkPath, (FskMemPtr *)resolved);

bail:
	return err;
}

FskErr FskFSFileInitialize() {

	if (gAndroidCallbacks->getSpecialPathsCB) {
		gAndroidCallbacks->getSpecialPathsCB(&androidSystemMusicDir, &androidSystemPodcastDir,
				&androidSystemPicturesDir, &androidSystemMoviesDir, &androidSystemDownloadsDir,
				&androidSystemDcimDir);
	}

	return kFskErrNone;
}


FskErr FskFSFileTerminate() {
	gAndroidCallbacks->dirChangeNotifierTermCB();
	return kFskErrNone;
}


static FskList  gVolNotifiers = NULL;
static FskMutex  gVolNotifiersMutex = NULL;

FskErr FskFSVolumeNotifierNew(FskVolumeNotifierCallbackProc callback, void *refCon, FskFSVolumeNotifier *volNtfOut) {
	FskErr err;
	FskFSVolumeNotifier volNtf = NULL;

	FskAndroidFilesPrintfDebug("VolumeNotifierNew\n");

	if (NULL == gVolNotifiers) {
		err = FskMutexNew(&gVolNotifiersMutex, "volume notifier");
		BAIL_IF_ERR(err);
	}

	err = FskMemPtrNewClear(sizeof(FskFSVolumeNotifierRecord), (FskMemPtr*)(void*)&volNtf);
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
	*volNtfOut = volNtf;

	return err;
}

FskErr FskFSVolumeNotifierDispose(FskFSVolumeNotifier volNtf) {
	FskAndroidFilesPrintfDebug("VolumeNotifierDispose\n");
	FskMutexAcquire(gVolNotifiersMutex);

	FskListRemove((FskList *)&gVolNotifiers, &volNtf->next);

	FskMemPtrDispose(volNtf);

	FskMutexRelease(gVolNotifiersMutex);

	return kFskErrNone;
}

void callVolumeNotifier(void *arg0, void *arg1, void *arg2, void *arg3) {
	FskFSVolumeNotifier volNtf = (FskFSVolumeNotifier)arg0;
	int mount = (int)arg1;
	int vid = (int)arg2;
	Boolean valid;

	FskAndroidFilesPrintfDebug("callVolumeNotifier\n");

	FskMutexAcquire(gVolNotifiersMutex);
	valid = FskListContains(gVolNotifiers, offsetof(FskFSVolumeNotifierRecord, next) + (char*)volNtf);
	FskMutexRelease(gVolNotifiersMutex);

	if (!valid)
		return;

	FskAndroidFilesPrintfDebug("about to volNotifCallback %d for %d\n", mount, vid);
	(volNtf->callback)(mount, vid, volNtf->refCon);
}

void androidVolumeEvent(int what, char *path) {
	UInt32 vid;
	FskFSVolumeNotifier walker = NULL;
	char *tmp = NULL;

	FskAndroidFilesPrintfDebug("androidVolumeEvent - %d, %s\n", what, path);
	if (NULL == gVolNotifiersMutex)
		return;

	FskMutexAcquire(gVolNotifiersMutex);

	if (0 == FskStrCompareWithLength(path, "file://", 7))
		path += 7;

	if (path[FskStrLen(path)-1] != '/')
		tmp = FskStrDoCat(path, "/");
	else
		tmp = FskStrDoCopy(path);

	FskFSVolumeGetID(tmp, &vid);


	FskAndroidFilesPrintfDebug("androidVolumeEvent - getvolumeid returned %d\n", vid);

	if (vid == kFskUInt32Max) {
		FskAndroidFilesPrintfDebug("couldn't find a mount for %s - try to scan again###\n", tmp);
		scanProcMount();
		FskFSVolumeGetID(tmp, &vid);
		if (vid == 0) {
			FskAndroidFilesPrintfDebug("#### still no volume id? %s\n", tmp);
		}
	}

	while (gVolNotifiers) {
		FskFSVolumeNotifier notifier;

		walker = (FskFSVolumeNotifier)FskListGetNext(gVolNotifiers, walker);
		if (NULL == walker)
			break;

		FskAndroidFilesPrintfDebug("androidVolumeEvent - notifying %x\n", walker);
		notifier = (FskFSVolumeNotifier)(((char *)walker) - offsetof(FskFSVolumeNotifierRecord, next));
		FskThreadPostCallback(notifier->callbackThread, callVolumeNotifier, notifier, (void*)what, (void*)vid, NULL);
	}

	FskMutexRelease(gVolNotifiersMutex);
	FskMemPtrDispose(tmp);
}


FskErr FskCopyFile(char *src, char *dst) {
    char buf[4096];
    UInt32 amt, amtWrt;
    int doit=0;
    FskFileInfo info, infoSrc;
    FskFile srcFref = NULL, dstFref = NULL;
    FskErr err = kFskErrNone;

    if (kFskErrFileNotFound == FskFileGetFileInfo(dst, &info)) {
        FskAndroidFilesPrintfDebug("dst: %s not found\n", dst);
        doit = 1;
    }
    else if (kFskErrNone == FskFileGetFileInfo(src, &infoSrc)) {
        if (infoSrc.filesize != info.filesize) {
            FskAndroidFilesPrintfDebug("src size: %d, dstSize: %d\n", infoSrc.filesize, info.filesize);
            doit = 1;
        }
    }

    if (doit) {
        FskAndroidFilesPrintfDebug("Need to copy it over.\n");
        err = FskFileOpen(src, kFskFilePermissionReadOnly, &srcFref);
		BAIL_IF_ERR(err);
        err = FskFileCreate(dst);
		BAIL_IF_ERR(err);
        err = FskFileOpen(dst, kFskFilePermissionReadWrite, &dstFref);
		BAIL_IF_ERR(err);
        while (kFskErrNone == err) {
            err = FskFileRead(srcFref, 4096, buf, &amt);
            BAIL_IF_ERR(err);
            if (0 >= amt)
                break;
            while (amt) {
                err = FskFileWrite(dstFref, amt, buf, &amtWrt);
				BAIL_IF_ERR(err);
                amt -= amtWrt;
            }
        }
        err = FskFileClose(dstFref);
        BAIL_IF_ERR(err);
		dstFref = NULL;
        err = FskFileClose(srcFref);
        BAIL_IF_ERR(err);
		srcFref = NULL;
    }

bail:
	if(kFskErrEndOfFile == err){
		err = kFskErrNone;
	}
	if(dstFref)
		FskFileClose(dstFref);
	if(srcFref)
		FskFileClose(srcFref);
	return err;
}

FskErr FskFSDirectoryChangeNotifierNew(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNtf) {
	return gAndroidCallbacks->dirChangeNotifierNewCB(path, flags, callback, refCon, dirChangeNtf);
}

FskErr FskFSDirectoryChangeNotifierDispose(FskDirectoryChangeNotifier dirChangeNtf) {
	return gAndroidCallbacks->dirChangeNotifierDisposeCB(dirChangeNtf);
}

