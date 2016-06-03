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
#define __FSKFILES_PRIV__
#define __FSKFSFILES_PRIV__
#include "Fsk.h"
#include "FskFiles.h"
#include "KplFiles.h"
#include "KplThread.h"
#include "FskThread.h"
#include "KplSynchronization.h"

#include "FskList.h"

#include "Kpl.h"
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

#include <dirent.h>

#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/mount.h>

#include <sys/vfs.h>
#include <mntent.h>		// UBUNTU

FskInstrumentedSimpleType(KplFiles, kplfiles);

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
	KplInt64	capacity;
	KplInt64	remaining;
#if ANDROID_PLATFORM
	__kernel_fsid_t		fsid;
#endif
} volInfoRec, *volInfo;

volInfo	volumeList = NULL;
void terminateVolumeList();

#if MINITV
#define kKplLinuxVolumeListFile "/proc/mounts"
#else
#define kKplLinuxVolumeListFile "/etc/mtab"
#endif


#if BAD_FTRUNCATE
	int FTRUNCATE(FILE *theFile, FILEOFFSET length);
#else
	#define FTRUNCATE(a,b) ftruncate(fileno(a),b)
#endif

struct KplFileRecord {
	Boolean flushBeforeRead;
	Boolean flushBeforeWrite;
	FILE *theFile;
	UInt32 thePermissions;
};

struct KplFileMappingRecord {
	int 	file;
	char	*address;
	size_t	length;
};

struct KplDirectoryIteratorRecord {
	FskMemPtr	root;
    UInt32      flags;
	DIR			*theDir;
};

struct KplVolumeIteratorRecord {
	void *volumeInfo;
};

#if !USE_INOTIFY
struct KplDirectoryChangeNotifierRecord {

};

struct KplVolumeNotifierRecord {

};
#endif


//static FskErr KplFileGetPathInfo(const char *rootpath, const char *filepath, FskFileInfo *itemInfo);

// ---------------------------------------------------------------------
static char *sPermsToPermStr(UInt32 permissions) {
	switch (permissions) {
		case kKplFilePermissionReadOnly: return "rb";
		case kKplFilePermissionReadWrite: return "rb+";
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

// ---------------------------------------------------------------------
FskErr KplFileTerminate() {

	terminateVolumeList();

	return kFskErrNone;
}

enum {
	kKplPathIsAny = 0,
	kKplPathIsFile = 1,
	kKplPathIsDirectory = 2
};

static FskErr sCheckFullPath(const char *fullPath, UInt32 pathType) {
	SInt32 pathLen;

	if (NULL == fullPath)
		return kFskErrInvalidParameter;

	if ('/' == fullPath[0] )
		;
	else {
		FskKplFilesPrintfDebug("## Bad path - %s - needs to be fullpath\n", fullPath);
		return kFskErrInvalidParameter;
	}

	pathLen = FskStrLen(fullPath);
	if (kKplPathIsFile == pathType) {
		if ((0 != pathLen) && ('/' == fullPath[pathLen - 1])) {
			FskKplFilesPrintfDebug("## Bad path - %s - was looking for a file\n", fullPath);
			return kFskErrInvalidParameter;
		}
	}
	else
	if (kKplPathIsDirectory == pathType) {
		if ((0 != pathLen) && ('/' != fullPath[pathLen - 1])) {
			FskKplFilesPrintfDebug("## Bad path - %s - was looking for a directory\n", fullPath);
			return kFskErrInvalidParameter;
		}
	}

	return kFskErrNone;
}

// ---------------------------------------------------------------------

FskErr KplFileOpen(const char *fullPath, UInt32 permissions, KplFile *frefOut) {
	FskErr		err = kFskErrNone;
	KplFile	fref;
	KplFileInfo itemInfo;

	err = FskMemPtrNewClear(sizeof(KplFileRecord), (FskMemPtr *)&fref);
	if (err) goto bail;

	err = sCheckFullPath(fullPath, kKplPathIsFile);
	if (err) goto bail;

	err = KplFileGetFileInfo(fullPath, &itemInfo);
	if (err) goto bail;

	if (itemInfo.filetype == kKplDirectoryItemIsDirectory) {
		err = kFskErrIsDirectory;
		goto bail;
	}

	fref->thePermissions = permissions;

	fref->theFile = FOPEN(fullPath, sPermsToPermStr(permissions));
	if (!fref->theFile) {
		err = errnoToFskErr(errno);
		goto bail;
	}

bail:
	if ((kFskErrNone != err) && (NULL != fref)) {
		FskMemPtrDispose(fref);
		fref = NULL;
		FskKplFilesPrintfDebug("## open file %s failed\n", fullPath);
	}

	*frefOut = fref;

	return err;
}

FskErr KplFileClose(KplFile fref)
{
	if (fref) {
		fclose(fref->theFile);
		FskMemPtrDispose(fref);
	}

	return kFskErrNone;
}

FskErr KplFileGetSize(KplFile fref, KplInt64 *size)
{
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
	if (ret < 0) {
		FskKplFilesPrintfDebug("## FileGetSize failed -- errno %d\n", errno);
		err = errnoToFskErr(errno);
	}

	return err;
}

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

FskErr KplFileSetSize(KplFile fref, const KplInt64 *size) {
	FILEOFFSET	pos, oldpos;
	int ret;

	if (!fref) return kFskErrInvalidParameter;

	if (fref->thePermissions == kKplFilePermissionReadOnly)
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

FskErr KplFileSetPosition(KplFile fref, const KplInt64 *position)
{
	int		err;
	FILEOFFSET	pos;

	if (!fref) return kFskErrInvalidParameter;

	pos = *position;

	fref->flushBeforeRead = false;
	fref->flushBeforeWrite = false;

	err = FSEEK(fref->theFile, pos, SEEK_SET);
	if (err == -1)
		return errnoToFskErr(errno);

	return kFskErrNone;
}

FskErr KplFileGetPosition(KplFile fref, KplInt64 *position)
{
	FILEOFFSET pos;

	if (!fref) return kFskErrInvalidParameter;

	pos = FTELL(fref->theFile);
	if (pos == -1) return errnoToFskErr(errno);

	*position = pos;

	if (*position != pos) return kFskErrOutOfRange;

	return kFskErrNone;
}

FskErr KplFileRead(KplFile fref, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead)
{
	size_t	amt;

	if (bytesRead)
		*bytesRead = 0;

	if (!fref) return kFskErrInvalidParameter;

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
			FskKplFilesPrintfDebug("## read error errno: %d\n", errno);
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

FskErr KplFileWrite(KplFile fref, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten)
{
	size_t	amt;

	if (!fref)
		return kFskErrInvalidParameter;

	if (fref->thePermissions == kKplFilePermissionReadOnly) {
		FskKplFilesPrintfDebug("## KplFileWrite failed - read-only file\n");
		return kFskErrReadOnly;
	}

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

static void GetFileInfoFromStat(STATTYPE *statbuf, KplFileInfo *itemInfo)
{
	if (S_ISLNK(statbuf->st_mode))
		itemInfo->filetype = kKplDirectoryItemIsLink;
	else if (S_ISDIR(statbuf->st_mode))
		itemInfo->filetype = kKplDirectoryItemIsDirectory;
	else
		itemInfo->filetype = kKplDirectoryItemIsFile;

	itemInfo->filesize = statbuf->st_size;
	itemInfo->fileModificationDate = statbuf->st_mtime;
	itemInfo->fileCreationDate = statbuf->st_ctime;
	itemInfo->fileNode = statbuf->st_ino;
}

FskErr KplFileGetFileInfo(const char *fullPath, KplFileInfo *itemInfo) {
	int err;
	STATTYPE statbuf;

	err = sCheckFullPath(fullPath, kKplPathIsAny);
	if (err) return err;

	err = STAT(fullPath, &statbuf);
	if (err == -1) return errnoToFskErr(errno);

	GetFileInfoFromStat(&statbuf, itemInfo);

	itemInfo->flags = 0;

	return kFskErrNone;
}

FskErr KplFileSetFileInfo(const char *fullPath, const KplFileInfo *itemInfo)
{
	int err;
	struct utimbuf times = {0};

	err = sCheckFullPath(fullPath, kKplPathIsAny);
	if (err)
		return err;

	times.modtime = itemInfo->fileModificationDate;

	err = utime(fullPath, &times);
	if (err == -1)
		return errnoToFskErr(errno);

	return kFskErrNone;
}

FskErr KplFileGetPathInfo(const char *rootpath, const char *filepath, KplFileInfo *itemInfo) {
	int err;
	STATTYPE statbuf;
	char	*fullpath;

	err = FskMemPtrNew(FskStrLen(rootpath) + FskStrLen(filepath) + 2, (FskMemPtr *)&fullpath);
	if (err) return err;

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

	return err;
}

FskErr KplFileGetThumbnail(const char *fullPath, const UInt32 width, const UInt32 height, KplBitmap *thumbnail)
{
    return kFskErrUnimplemented;
}

FskErr KplFileDelete(const char *fullPath)
{
	int err;

	err = sCheckFullPath(fullPath, kKplPathIsFile);
	if (err) return err;

	err = unlink(fullPath);
	if (err == -1) {
		return errnoToFskErr(errno);
	}
	else
		return kFskErrNone;
}

FskErr KplFileRename(const char *fullPath, const char *newName)
{
	int err;
	char *p, newPath[PATH_MAX];
	KplFileInfo itemInfo;

	err = sCheckFullPath(fullPath, kKplPathIsFile);
	if (err) return err;

	err = KplFileGetFileInfo(fullPath, &itemInfo);
	if (err) return err;
	if (itemInfo.filetype == kKplDirectoryItemIsDirectory)
		return kFskErrIsDirectory;

	p = FskStrRChr((char *)newName, '/');
	if (p) return kFskErrOperationFailed;	// newName contains path elements

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

FskErr KplFilePathToNative(const char *fskPath, char **nativePath)
{
	if ((NULL == fskPath) || ('/' != fskPath[0]))
		return kFskErrInvalidParameter;

	*nativePath = FskStrDoCopy(fskPath);
	return (NULL != *nativePath) ? kFskErrNone : kFskErrMemFull;
}

FskErr KplFileResolveLink(const char *fullPath, char **resolved)
{
	FskErr	err = kFskErrNone;
	char 	linkPath[PATH_MAX];

	if (realpath(fullPath, linkPath) == NULL) {
		err = kFskErrOperationFailed;
		goto bail;
	}

	err = FskMemPtrNewFromData(FskStrLen(linkPath) + 1, linkPath, (FskMemPtr *)resolved);
	if (err) goto bail;

bail:
	return err;
}

FskErr KplFileFlush(KplFile fref)
{
	fflush(fref->theFile);
    return kFskErrNone;
}

FskErr KplFileCreate(const char *fullPath)
{
	int err;

	err = sCheckFullPath(fullPath, kKplPathIsFile);
	if (err) return err;

	err = OPEN(fullPath, CREATEFLAGS, 0666);
	if (err == -1) {
		FskKplFilesPrintfDebug("## KplFileCreate %s failed - errno: %d\n", fullPath, errno);
		return errnoToFskErr(errno);
	}
	else {
		close(err);
		return kFskErrNone;
	}
}

FskErr KplFileCreateDirectory(const char *fullPath)
{
	int err;

	err = sCheckFullPath(fullPath, kKplPathIsDirectory);
	if (err) return err;

	err = mkdir(fullPath, 0777);
	if (err == -1) {
		FskKplFilesPrintfDebug("## KplFileCreateDirectory %s failed - errno: %d\n", fullPath, errno);
		return errnoToFskErr(errno);
	}
	else
		return kFskErrNone;
}

FskErr KplFileRenameDirectory(const char *fullPath, const char *newName)
{
	int err, len;
	char *p, newPath[PATH_MAX];
	KplFileInfo itemInfo;

	err = sCheckFullPath(fullPath, kKplPathIsDirectory);
	if (err) return err;

	len = FskStrLen(fullPath);
	if (len < 2) return kFskErrOperationFailed;			// can't rename root

	err = KplFileGetFileInfo(fullPath, &itemInfo);
	if (err) return err;

	if (itemInfo.filetype != kKplDirectoryItemIsDirectory)
		return kFskErrNotDirectory;

	p = FskStrRChr((char *)newName, '/');
	if (p) return kFskErrOperationFailed;	// newName contains path elements

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

FskErr KplFileDeleteDirectory(const char *fullPath)
{
	int err;

	err = sCheckFullPath(fullPath, kKplPathIsDirectory);
	if (err) return err;

	err = rmdir(fullPath);
	if (err == -1)
		return errnoToFskErr(errno);
	else
		return kFskErrNone;
}

// ---------------------------------------------------------------------
// Volume Eject
// ---------------------------------------------------------------------

FskErr KplVolumeEject(const char *fullPath, UInt32 flags) {
#if MINITV
	int oserr;

	oserr = umount(fullPath);
	if (0 == oserr)
		return kFskErrNone;

	fprintf(stderr, "error %d attempting to umount(%s)\n", oserr, fullPath);
	return kFskErrOperationFailed;

#else
	return kFskErrUnimplemented;
#endif
}

// ---------------------------------------------------------------------
// Volume Iterator
// ---------------------------------------------------------------------

typedef struct {
	char *name;
	int	type;
	Boolean removable;
} mntdevmap;

static mntdevmap gMntDevMap[] = {
	{ "usbdevfs", kKplVolumeTypeMemoryStick, true },
	{ "iso9660", kKplVolumeTypeOptical, true },
	{ "rootfs", kKplVolumeTypeFixed, false },
	{ "jffs2", kKplVolumeTypeFixed, false },
	{ "vfat", kKplVolumeTypeSDMemory, true },
	{ "ext3", kKplVolumeTypeOptical, false },
	{ "ext", kKplVolumeTypeFixed, false },
	{ "udf", kKplVolumeTypeOptical, true },
	{ "nfs", kKplVolumeTypeNetwork, false },
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
	while ( NULL != (vol = FskListRemoveFirst((void **)&volumeList)) ) {
		FskMemPtrDispose(vol->name);
		FskMemPtrDispose(vol->mountPoint);
		FskMemPtrDispose(vol->typeStr);
		FskMemPtrDispose(vol);
	}
}

#ifdef ANDROID_PLATFORM
    // t7 like platform - TODO
void scanVolumes() {
	int err;
	volInfo	vi, walker;
	struct statfs fsinfo;

// ROOT
	FskMemPtrNewClear(sizeof(volInfoRec), &vi);
	vi->type = kKplVolumeTypeFixed;
	vi->removable = false;
	vi->mounted = true;
	vi->typeStr = FskStrDoCopy("ext4");

	vi->mountPoint = FskStrDoCopy("/\0");
	vi->name = FskStrDoCopy("/\0");
	err = statfs("/", &fsinfo);
	vi->capacity = fsinfo.f_blocks * fsinfo.f_bsize;
	vi->remaining = fsinfo.f_bavail * fsinfo.f_bsize;
	vi->fsid = fsinfo.f_fsid;
	FskListAppend(&volumeList, vi);
//fprintf(stderr, " fsid: %d:%d, capacity: %d, remaining: %d\n", vi->fsid.__val[0], vi->fsid.__val[1], vi->capacity, vi->remaining);

// SDCARD - there may be a better way to do this
	FskMemPtrNewClear(sizeof(volInfoRec), &vi);
	vi->type = kKplVolumeTypeSDMemory;
	vi->removable = true;
	vi->mounted = true;
	vi->typeStr = FskStrDoCopy("vfat");

	vi->mountPoint = FskStrDoCopy("/mnt/external_sdcard/\0");
	vi->name = FskStrDoCopy("sdcard\0");
	vi->fsid.__val[0] = -1;
	vi->fsid.__val[1] = -1;
	err = statfs("/mnt/sdcard", &fsinfo);
	if (0 == err) {
		walker = volumeList;
		while (walker) {
			if ((fsinfo.f_fsid.__val[0] == walker->fsid.__val[0])
				&& (fsinfo.f_fsid.__val[1] == walker->fsid.__val[1])) {
				// same fsid - it's not a different mount
				break;
			}
			walker = walker->next;
		}
		if (!walker) {		// didn't find an entry with the same fsid
			vi->mounted = true;
			vi->fsid = fsinfo.f_fsid;
			vi->capacity = fsinfo.f_blocks * fsinfo.f_bsize;
			vi->remaining = fsinfo.f_bavail * fsinfo.f_bsize;
//fprintf(stderr, " fsid: %d:%d, capacity: %d, remaining: %d\n", vi->fsid.__val[0], vi->fsid.__val[1], vi->capacity, vi->remaining);
		}
	}
	if (vi->fsid.__val[0] == -1 && vi->fsid.__val[1] == -1) {
		FskMemPtrDispose(vi);
	}
	else
		FskListAppend(&volumeList, vi);
}

#else
void scanVolumes() {
	FILE *mountList;
	struct mntent	*cur;
	int err;
	volInfo	vi;
	mntdevmap *walker;
	struct statfs fsinfo;

	mountList = setmntent(kKplLinuxVolumeListFile, "r");
	if (mountList) {
		while (NULL != (cur = getmntent(mountList))) {
			err = statfs(cur->mnt_dir, &fsinfo);
			if (0 != err) {
				continue;
			}

			for (walker = gMntDevMap; NULL != walker->name; walker++) {
				if (0 == FskStrCompareWithLength(cur->mnt_type, walker->name, FskStrLen(walker->name)))
					break;
			}

			if (!walker->name) {
				continue;
			}

			FskMemPtrNewClear(sizeof(volInfoRec), &vi);
			vi->type = walker->type;
			vi->removable = walker->removable;
			vi->mounted = true;
			vi->typeStr = FskStrDoCopy(cur->mnt_type);

			if (kFskErrNone == FskMemPtrNew(FskStrLen(cur->mnt_dir) + 2, &vi->mountPoint)) {
				FskStrCopy(vi->mountPoint, cur->mnt_dir);
				if (vi->mountPoint[FskStrLen(vi->mountPoint)-1] != '/')
					FskStrCat(vi->mountPoint, "/");
			}

			vi->name = copyNiceName(cur->mnt_fsname);
			vi->capacity = fsinfo.f_blocks * fsinfo.f_bsize;
			vi->remaining = fsinfo.f_bavail * fsinfo.f_bsize;
			FskListAppend(&volumeList, vi);
		}
		endmntent(mountList);
	}
}

#endif

FskErr KplVolumeIteratorNew(KplVolumeIterator *volIt)
{
	FskErr	ret = kFskErrNone;
	KplVolumeIterator	vol;

	if (!volumeList)
		scanVolumes();

	ret = FskMemPtrNewClear(sizeof(KplVolumeIteratorRecord), (FskMemPtr*)&vol);
	if (kFskErrNone == ret)
		vol->volumeInfo = volumeList;

	if (kFskErrNone == ret) {
		*volIt = vol;
	}
	else {
		*volIt = NULL;
		FskMemPtrDispose(vol);
	}
	return ret;
}

FskErr KplVolumeIteratorDispose(KplVolumeIterator volIt)
{
	if (volIt) {
		FskMemPtrDispose(volIt);
	}
	return kFskErrNone;
}

FskErr KplVolumeIteratorGetNext(KplVolumeIterator volIt, UInt32 *id, char **path, char **name)
{
	FskErr ret = kFskErrIteratorComplete;

	if (volIt) {
		if (volIt->volumeInfo) {
			volInfo vi = (volInfo)volIt->volumeInfo;
			if (id)
				*id = (UInt32)vi;
			if (path)
				*path = FskStrDoCopy(vi->mountPoint);
			if (name)
				*name = FskStrDoCopy(vi->name);
			volIt->volumeInfo = vi->next;
			ret = kFskErrNone;
		}
	}

	return ret;
}


FskErr KplVolumeGetInfo(UInt32 volumeID, char **pathOut, char **nameOut, UInt32 *volumeType, Boolean *isRemovable, KplInt64 *capacity, KplInt64 *freeSpace)
{
	volInfoRec *vi;
	FskErr	err = kFskErrOperationFailed;

	if (!volumeList)
		scanVolumes();
	vi = volumeList;
	while (vi) {
		if (volumeID == (UInt32)vi) {
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

FskErr KplVolumeGetInfoFromPath(const char *pathIn, char **pathOut, char **nameOut, UInt32 *volumeType, Boolean *isRemovable, KplInt64 *capacity, KplInt64 *freeSpace) {
	volInfo vi;
	FskErr	err = kFskErrOperationFailed;

	if (!volumeList)
		scanVolumes();

	vi = volumeList;
	while (vi) {
		if (0 == FskStrCompare(vi->mountPoint, pathIn)) {
			err = KplVolumeGetInfo((UInt32)vi, pathOut, nameOut, volumeType, isRemovable, capacity, freeSpace);
			break;
		}
		vi = vi->next;
	}

	return err;
}

FskErr KplVolumeGetDeviceInfo(UInt32 volumeID, char **vendor, char **product, char **revision, char **vendorSpecific) {

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

// ---------------------------------------------------------------------
// Directories
// ---------------------------------------------------------------------

FskErr KplDirectoryIteratorNew(const char *directoryPath, UInt32 flags, KplDirectoryIterator *dirIt)
{
	KplDirectoryIterator di = NULL;
	FskErr	err;
	KplFileInfo itemInfo;

	if (directoryPath[FskStrLen(directoryPath) -1] != '/') {
		err = kFskErrInvalidParameter;
		goto bail;
	}

	err = KplFileGetFileInfo(directoryPath, &itemInfo);
	if (err) goto bail;

	if (itemInfo.filetype != kKplDirectoryItemIsDirectory) {
		err = kFskErrNotDirectory;
		goto bail;
	}

	err = FskMemPtrNew(sizeof(KplDirectoryIteratorRecord), (FskMemPtr*)&di);
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

	*dirIt = (KplDirectoryIterator)di;
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

FskErr KplDirectoryIteratorDispose(KplDirectoryIterator dirIt)
{
	if (dirIt && dirIt->theDir) {
		closedir(dirIt->theDir);
		FskMemPtrDispose(dirIt->root);
		FskMemPtrDispose(dirIt);
		return kFskErrNone;
	}

	return kFskErrNone;
}

FskErr KplDirectoryIteratorGetNext(KplDirectoryIterator dirIt, char **name, UInt32 *itemType)
{
	struct dirent *ent;
	FskErr	err;
	KplFileInfo finfo;

	if (!dirIt || !dirIt->theDir)
		return kFskErrInvalidParameter;

	ent = readdir(dirIt->theDir);
	if (!ent) {
		return kFskErrIteratorComplete;
	}

	err = KplFileGetPathInfo((char *)dirIt->root, ent->d_name, &finfo);
	if (err != kFskErrNone)
		return err;

	if (itemType)
		*itemType = finfo.filetype;

    if ('.' == ent->d_name[0]) {
        if ((0 == (dirIt->flags & 1)) ||
            (0 == FskStrCompare(ent->d_name, ".")) ||
            (0 == FskStrCompare(ent->d_name, "..")))
            return KplDirectoryIteratorGetNext(dirIt, name, itemType);        // skip hidden files
    }

    if (name)
        *name = FskStrDoCopy(ent->d_name);

	return kFskErrNone;
}

FskErr KplFileMap(const char *fullPath, unsigned char **data, KplInt64 *dataSize, KplFileMapping *mapOut)
{
    FskErr err = kFskErrNone;
    KplFileMapping map = NULL;
    KplInt64 size;
	int fp;
	struct stat statbuf;

	err = sCheckFullPath(fullPath, kKplPathIsFile);
	if (err)
		return err;

    err = FskMemPtrNewClear(sizeof(KplFileMappingRecord), (FskMemPtr *)&map);
    if (err) goto bail;

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
		err = kFskErrOperationFailed;
		goto bail;
	}

	map->length = size;
	map->address = mmap(NULL, map->length, PROT_READ, MAP_SHARED, map->file, 0);
	if (MAP_FAILED == map->address) {
		map->address = NULL;
		err = kFskErrOperationFailed;
		goto bail;
	}

	*data = (unsigned char *)map->address;
	*dataSize = size;

bail:
	if (kFskErrNone != err) {
		KplFileDisposeMap(map);
		map = NULL;
	}

	*mapOut = map;

	return err;
}

FskErr KplFileDisposeMap(KplFileMapping map)
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

FskErr KplDirectoryGetSpecialPath(UInt32 type, const Boolean create, const char *volumeName, char **fullPath)
{
	FskErr	err = kFskErrFileNotFound;
	KplFileInfo finfo;
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
#ifdef ANDROID_PLATFORM
		case kFskDirectorySpecialTypeApplicationPreference:
			tmp = FskEnvironmentGet("PreferencesDir");
			if (!tmp)
				*fullPath = FskStrDoCopy("/data/local/tmp/.kinoma/");
			else
				*fullPath = FskStrDoCopy(tmp);
			err = kFskErrNone;
			goto bail;
		case kFskDirectorySpecialTypeTemporary:
			*fullPath = FskStrDoCopy("/data/local/tmp/");
			err = kFskErrNone;
			goto bail;
		case kFskDirectorySpecialTypeCache:
			*fullPath = FskStrDoCopy("/data/local/tmp/");
			err = kFskErrNone;
			goto bail;
#else
		case kFskDirectorySpecialTypeApplicationPreference:
			tmp = FskEnvironmentGet("PreferencesDir");
			if (!tmp)
#if BG3CDP
			{
				*fullPath = FskStrDoCopy("/data/.kinoma/");
			}
#else
			{
				if (0 == FskStrCompare("/", getenv("HOME")))
					*fullPath = FskStrDoCopy("/.kinoma/");
				else
					*fullPath = FskStrDoCat(getenv("HOME"), "/.kinoma/");
			}
#endif
			else
				*fullPath = FskStrDoCopy(tmp);
			err = kFskErrNone;
			goto bail;
		case kFskDirectorySpecialTypeTemporary:
			*fullPath = FskStrDoCopy("/tmp/");
			err = kFskErrNone;
			goto bail;
		case kFskDirectorySpecialTypeCache:
			*fullPath = FskStrDoCopy("/tmp/");
			err = kFskErrNone;
			goto bail;
#endif
		default:
			err = kFskErrInvalidParameter;
			goto bail;
	}

	if (specialDirEnv)
		tmp = FskStrDoCopy(FskEnvironmentGet(specialDirEnv));
	if (!tmp) {
		char *home;
		home = getenv("HOME");
		err = FskMemPtrNewClear(FskStrLen(home) + FskStrLen(specialDirEnv) + 3, (FskMemPtr*)&tmp);
		if (err) goto bail;
		FskStrCopy(tmp, home);
		if (tmp[FskStrLen(tmp)-1] != '/')
			FskStrCat(tmp, "/");
		FskStrCat(tmp, specialDirEnv);
		FskStrCat(tmp, "/");
	}
	*fullPath = tmp;

	if (create) {
		err = KplFileCreateDirectory(*fullPath);
	}
	else {
		err = KplFileGetFileInfo(*fullPath, &finfo);
		if (kFskErrNone == err) {
			if (kKplDirectoryItemIsDirectory != finfo.filetype)
				err = kFskErrNotDirectory;
		}
	}

bail:
	if (kFskErrFileExists == err)
		err = kFskErrNone;
	return err;
}



#ifdef USE_INOTIFY
#define INOTIFY_CHANGE_FILE_MASK (IN_CLOSE_WRITE)
#define INOTIFY_DELETE_FILE_MASK (IN_MOVED_FROM | IN_DELETE)
#define INOTIFY_CREATE_FILE_MASK (IN_MOVED_TO | IN_CREATE)

extern FskFileDispatchTableRecord gFSDispatch;

struct FskFSDirectoryChangeNotifierRecord {
	FskDirectoryChangeNotifierRecord		base;
	UInt32									flags;
	KplDirectoryChangeNotifierCallbackProc	callback;
	void									*refCon;

	FskThread								thread;
	SInt32									count;

	int										wd;
};

typedef struct FskFSDirectoryChangeNotifierRecord FskFSDirectoryChangeNotifierRecord;
typedef struct FskFSDirectoryChangeNotifierRecord *FskFSDirectoryChangeNotifier;

static FskMutex gDirChangeNotifiersMutex = NULL;
static FskFSDirectoryChangeNotifier gDirectoryChangeNotifiers = NULL;
static int iNotifyFD = -1;

static FskThreadDataSource gDirChangeSource = NULL;
static FskThreadDataHandler gDirChangeHandler = NULL;

static void doChangesCallback(void *a1, void *a2, void *a3, void *a4) {
	KplDirectoryChangeNotifierCallbackProc callback = (KplDirectoryChangeNotifierCallbackProc)a3;
	void* refCon = a4;
	char *path = (char*)a1;
	int mask = (int)a2;
	int flags = 0;

	if (callback) {
		if (mask & INOTIFY_CHANGE_FILE_MASK)
			flags |= kKplDirectoryChangeFileChanged;
		if (mask & INOTIFY_DELETE_FILE_MASK)
			flags |= kKplDirectoryChangeFileDeleted;
		if (mask & INOTIFY_CREATE_FILE_MASK)
			flags |= kKplDirectoryChangeFileCreated;
		if (mask)
			(callback)(flags, NULL, refCon);
	}
	FskMemPtrDispose(path);
}

static void dirChangeHandler(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon) {
	FskFSDirectoryChangeNotifier walker;
	struct inotify_event *ev;
	char *p, buf[1024];
	int l, ret;

	ret = read(iNotifyFD, buf, 1024);
	p = buf;
	FskMutexAcquire(gDirChangeNotifiersMutex);
	while (ret > 0) {
		ev = (struct inotify_event *)p;
		l = sizeof(struct inotify_event) + ev->len;
		p += l;
		ret -= l;
		if (ev->len && ev->name && ev->name[0] == '.') {
			continue;
		}
		for (walker = gDirectoryChangeNotifiers; walker != NULL; walker = (FskFSDirectoryChangeNotifier)walker->base.next) {
			if (ev->wd == walker->wd) {
				char *buffer = NULL;
				if (ev->len)
					buffer = FskStrDoCopy(ev->name);
				FskThreadPostCallback(walker->thread, doChangesCallback, buffer, (void *)ev->mask, walker->callback, walker->refCon);
			}
		}
	}
	FskMutexRelease(gDirChangeNotifiersMutex);
}


FskErr KplDirectoryChangeNotifierNew(const char *path, UInt32 flags, KplDirectoryChangeNotifierCallbackProc callback, void *refCon, KplDirectoryChangeNotifier *dirChangeNtf)
{
	FskErr err;
	FskFSDirectoryChangeNotifier dirNot = NULL;
	int mask;

	*dirChangeNtf = NULL;

	if (NULL == gDirChangeNotifiersMutex) {
		err = FskMutexNew(&gDirChangeNotifiersMutex, "KplDirChangeNotifiers");
		if (err) return err;

		iNotifyFD = inotify_init();
		if (iNotifyFD < 0) {
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

	if (!sCheckFullPath(path, kKplPathIsDirectory))
		mask = INOTIFY_DELETE_FILE_MASK | INOTIFY_CREATE_FILE_MASK;
	else if (!sCheckFullPath(path, kKplPathIsFile))
		mask = INOTIFY_CHANGE_FILE_MASK;
	else
		BAIL_IF_ERR(kFskErrInvalidParameter);
	
	dirNot->wd = inotify_add_watch(iNotifyFD, path, mask);
	if (dirNot->wd == -1)
		BAIL_IF_ERR(kFskErrOperationFailed);

bail:
	FskMutexRelease(gDirChangeNotifiersMutex);

	if (kFskErrNone != err) {
		KplDirectoryChangeNotifierDispose((KplDirectoryChangeNotifier)dirNot);
		dirNot = NULL;
	}
	*dirChangeNtf = (KplDirectoryChangeNotifier)dirNot;

	return err;
}


FskErr KplDirectoryChangeNotifierDispose(KplDirectoryChangeNotifier dirChangeNtf)
{
    FskFSDirectoryChangeNotifier dirNot = (FskFSDirectoryChangeNotifier)dirChangeNtf;

	if (NULL == dirNot)
		return kFskErrNone;

	dirNot->callback = NULL;

	FskMutexAcquire(gDirChangeNotifiersMutex);
	FskListRemove((FskList*)&gDirectoryChangeNotifiers, dirNot);
	if (dirNot->wd != -1) {
		FskFSDirectoryChangeNotifier walker;
		for (walker = gDirectoryChangeNotifiers; walker != NULL; walker = (FskFSDirectoryChangeNotifier)walker->base.next) {
			if (dirNot->wd == walker->wd) {
				break;
			}
		}
		if (!walker)
			inotify_rm_watch(iNotifyFD, dirNot->wd);
	}
	FskMutexRelease(gDirChangeNotifiersMutex);
	FskMemPtrDispose(dirNot);

	return kFskErrNone;
}
#else 	// ! USE_INOTIFY

FskErr KplDirectoryChangeNotifierNew(const char *path, UInt32 flags, KplDirectoryChangeNotifierCallbackProc callback, void *refCon, KplDirectoryChangeNotifier *dirChangeNtf)
{
	return kFskErrUnimplemented;
}
FskErr KplDirectoryChangeNotifierDispose(KplDirectoryChangeNotifier dirChangeNtf)
{
	return kFskErrUnimplemented;
}

#endif

FskErr KplVolumeGetID(const char *fullPath, UInt32 *volumeID)
{
#if ANDROID_PLATFORM
	volInfo vi, cur = NULL;
	UInt32 resp, len;

	// looking for mountpoint of fullPath
	vi = volumeList;
	while (vi) {
		len = FskStrLen(vi->mountPoint);
		if (0 == FskStrCompareWithLength(vi->mountPoint, fullPath, len)) {
			if (!cur) {
				// matches - set as candidate
				cur = vi;
			}
			else {
				if (len > FskStrLen(cur->mountPoint)) {
					// new length is greater than old - take more specific mountPoint
					cur = vi;
				}
			}
		}
		vi = vi->next;
	}

	if (cur)
		*volumeID = (UInt32)cur;
	else
		*volumeID = -1;

	return kFskErrNone;
#else
	return kFskErrUnimplemented;
#endif
}

FskErr KplVolumeNotifierNew(KplVolumeNotifierCallbackProc callback, void *refCon, KplVolumeNotifier *volNtf)
{
	return kFskErrUnimplemented;
}

FskErr KplVolumeNotifierDispose(KplVolumeNotifier volNtf)
{
	return kFskErrUnimplemented;
}

KplDirectoryChangeNotifier sd1, sd2;

FskErr sdMountNotifierCallback(UInt32 whatChanged, const char *path, void *refCon) {
	fprintf(stderr, "sdMountNotifierCallback - what: %u, path: %s, ref: %d\n", (unsigned)whatChanged, path, (int)refCon);
	return kFskErrNone;
}

void watchSDMounts() {
#if 0
	FskErr err;
	err = KplDirectoryChangeNotifierNew("/dev/mmcblk0p1", kKplDirectoryChangeFileChanged, sdMountNotifierCallback, (void*)1, &sd1);
	err = KplDirectoryChangeNotifierNew("/dev/mmcblk0p2", kKplDirectoryChangeFileChanged, sdMountNotifierCallback, (void*)2, &sd2);
#endif
}

FskErr KplFileInitialize()
{
//	err = KplVolumeEject("/mnt/SD1", 0);
//	fprintf(stderr, "error %d trying to eject /mnt/SD1\n", err);
//	watchSDMounts();

#if TEST_VOLUME_ITERATOR
	KplVolumeIterator vit;
	char *path, *name;
	UInt32 id;
	FskErr err = KplVolumeIteratorNew(&vit);
	while (!err) {
		err = KplVolumeIteratorGetNext(vit, &id, &path, &name);
		if (!err)
			fprintf(stderr, "Volume name: %s, ID: %d, path: %s\n", name, id, path);
	}
	if (err)
		fprintf(stderr, "err: %d\n", err);
	KplVolumeIteratorDispose(vit);
#endif // TEST_VOLUME_ITERATOR
	return kFskErrNone;
}


