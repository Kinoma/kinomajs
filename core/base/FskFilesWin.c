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
#define _WIN32_WINNT 0x0500
#define COBJMACROS
#include "Fsk.h"

#define __FSKBITMAP_PRIV__
#define __FSKFILES_PRIV__
#define __FSKFSFILES_PRIV__
#include "FskFS.h"
#include "FskTextConvert.h"
#include "FskUtilities.h"
#include "FskPlatformImplementation.h"
#include "ntddscsi.h"
#include "shlobj.h"
#include "objidl.h"
#include "ntverp.h"

#if (VER_PRODUCTVERSION_W <= 0x0501)
	// the following are not in the Visual Studio .NET (2002) headers, so we include them here
	typedef struct _SHChangeNotifyEntry
	{
		LPCITEMIDLIST pidl;
		BOOL   fRecursive;
	} SHChangeNotifyEntry;

	SHSTDAPI_(ULONG) SHChangeNotifyRegister(HWND hwnd, int fSources, LONG fEvents, UINT wMsg, int cEntries, SHChangeNotifyEntry *pshcne);
	SHSTDAPI_(BOOL) SHChangeNotifyDeregister(unsigned long ulID);
#endif

#include "Ntmsapi.h"

#include <shlwapi.h>
#include <shlobj.h>
#include <tchar.h>
#include <stddef.h>

static UInt32 convertFileTime(const FILETIME *ft);
static void unconvertFileTime(UInt32 secs, FILETIME *ft);

enum {
	kFskPathIsAny = 0,
	kFskPathIsFile = 1,
	kFskPathIsDirectory = 2
};

static FskErr fixUpPathForWindows(const char *fullPath, WCHAR **winPath, UInt32 pathType);

static FskErr specialDirectoryTypeToCSIDL(const UInt32 fskSpecialDirectoryType, int *nFolder);
static FskErr doRename(const char *fullPath, const char *newNameIn, DWORD directoryFlag);

static FskErr FskFSFileFilter(const char *fullPath, SInt32 *priority);

static FskMutex gVolNotifiersMutex;						//@@ should be allocated at start-up
static ULONG gSHChangeNotifyRegister;

static WCHAR *gVolumeListCache;
static UInt32 gVolumeListCacheLength;


struct FskFSDirectoryChangeNotifierRecord {
	FskDirectoryChangeNotifierRecord				base;
	UInt32											flags;
	FskDirectoryChangeNotifierCallbackProc			callback;
	void											*refCon;

	struct FskThreadRecord							*thread;
	SInt32											count;
	HANDLE											hFile;
	OVERLAPPED										overlapped;

	unsigned char									buffer[2048];
};
typedef struct FskFSDirectoryChangeNotifierRecord FskFSDirectoryChangeNotifierRecord;
typedef struct FskFSDirectoryChangeNotifierRecord *FskFSDirectoryChangeNotifier;

static FskMutex gDirChangeNotifiersMutex;				//@@ should be allocated at start-up
static FskFSDirectoryChangeNotifier gDirectoryChangeNotifiers;
static HANDLE gDirectoryChangeNotifierEvent = INVALID_HANDLE_VALUE;
static HANDLE gDirectoryChangeNotifierThread = INVALID_HANDLE_VALUE;

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

	(FskFileChooseProc)FskFSFileChoose,
	(FskFileChooseSaveProc)FskFSFileChooseSave,
	(FskDirectoryChooseProc)FskFSDirectoryChoose,

	(FskFileTerminateProc)FskFSFileTerminate,

	(FskDirectoryChangeNotifierNewProc)FskFSDirectoryChangeNotifierNew,
	(FskDirectoryChangeNotifierDisposeProc)FskFSDirectoryChangeNotifierDispose,

	(FskFileInitializeProc)FskFSFileInitialize,
	(FskFileResolveLinkProc)FskFSFileResolveLink,

	(FskFileFlushProc)FskFSFileFlush
};

static FskErr winErrorToFskErr(DWORD winError)
{
	FskErr err = kFskErrNone;

	switch (winError) {
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			err = kFskErrFileNotFound;
			break;
		case ERROR_DIR_NOT_EMPTY:
		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:
			err = kFskErrFileExists;
			break;
		case ERROR_DIRECTORY:
			err = kFskErrNotDirectory;
			break;
		case ERROR_DISK_FULL:
			err = kFskErrDiskFull;
			break;
		case ERROR_WRITE_PROTECT:
			err = kFskErrVolumeLocked;
			break;
		case ERROR_SHARING_VIOLATION:
			err = kFskErrIsBusy;
			break;
		case ERROR_ACCESS_DENIED:
			err = kFskErrFilePermissions;
			break;
		case ERROR_NOT_READY:
		case ERROR_DEVICE_REMOVED:
			err = kFskErrVolumeUnavailable;
			break;
		case ERROR_NETWORK_UNREACHABLE:
		case ERROR_NETNAME_DELETED:
			err = kFskErrConnectionClosed;
			break;
		case ERROR_FILE_INVALID:
			err = kFskErrFileNotOpen;
			break;
		case NOERROR:
			break;
		default:
			err = kFskErrOperationFailed;
			break;
	}

	return err;
}

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
	FskErr err = kFskErrNone;

	if (0 == gSHChangeNotifyRegister) {
		LPITEMIDLIST ppidl;		    

		if (NOERROR == SHGetSpecialFolderLocation(gUtilsWnd, CSIDL_DESKTOP, &ppidl)) {
			SHChangeNotifyEntry shCNE;

			shCNE.pidl = ppidl;
			shCNE.fRecursive = TRUE;

			gSHChangeNotifyRegister = SHChangeNotifyRegister(gUtilsWnd, 0x0003,
				SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED | SHCNE_DRIVEADD | SHCNE_DRIVEREMOVED,    
				RegisterWindowMessage("FskDeviceChange"), 1, &shCNE);

			CoTaskMemFree(ppidl);
		}
	}

	return err;
}

FskErr FskFSFileTerminate()
{
	if (gSHChangeNotifyRegister) {
		SHChangeNotifyDeregister(gSHChangeNotifyRegister);
	}

	FskMutexDispose(gVolNotifiersMutex);
	FskMemPtrDisposeAt((void **)&gVolumeListCache);

	if (NULL != gDirChangeNotifiersMutex) {
		FskMutexAcquire(gDirChangeNotifiersMutex);
		if (INVALID_HANDLE_VALUE != gDirectoryChangeNotifierEvent) {
			HANDLE event = gDirectoryChangeNotifierEvent;
			gDirectoryChangeNotifierEvent = INVALID_HANDLE_VALUE;
			SetEvent(event);
		}
		FskMutexRelease(gDirChangeNotifiersMutex);
		FskMutexDispose(gDirChangeNotifiersMutex);
	}

	return kFskErrNone;
}

FskErr FskFSFileOpen(const char *fullPath, UInt32 permissions, FskFSFile *frefOut)
{
	FskErr err = kFskErrNone;
	FskFSFile fref = NULL;
	DWORD perm;
	WCHAR *pathCopy = NULL;

	err = FskMemPtrNewClear(sizeof(FskFSFileRecord), (FskMemPtr *)&fref);
	BAIL_IF_ERR(err);

	err = fixUpPathForWindows(fullPath, &pathCopy, kFskPathIsFile);
	BAIL_IF_ERR(err);

	fref->thePermissions = permissions;
	if (kFskFilePermissionReadOnly == permissions)
		perm = GENERIC_READ;
	else
		perm = GENERIC_READ | GENERIC_WRITE;

	fref->hFile = CreateFileW((LPCWSTR)pathCopy, perm, FILE_SHARE_READ,
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == fref->hFile) {
		DWORD winErr = GetLastError();
		if (NOERROR == winErr)
			err = kFskErrFileNotFound;		// file does not exist
		else
			err = winErrorToFskErr(winErr);
		goto bail;
	}

bail:
	FskMemPtrDispose(pathCopy);

	if ((kFskErrNone != err) && (NULL != fref)) {
		FskMemPtrDispose(fref);
		fref = NULL;
	}

	*frefOut = fref;

	return err;
}

FskErr FskFSFileClose(FskFSFile fref)
{
	if (fref) {
		CloseHandle(fref->hFile);
		FskMemPtrDispose(fref);
	}

	return kFskErrNone;
}

FskErr FskFSFileGetSize(FskFSFile fref, FskInt64 *size)
{
	DWORD dwFileSize, dwFileSizeHi;

	dwFileSize = GetFileSize(fref->hFile, &dwFileSizeHi);
	if ((INVALID_FILE_SIZE == dwFileSize) && (NO_ERROR != GetLastError())) {
		*size = 0;
		return kFskErrOperationFailed;
	}

	*size = dwFileSize | (((FskInt64)dwFileSizeHi) << 32);

	return kFskErrNone;
}

FskErr FskFSFileSetSize(FskFSFile fref, const FskInt64 *size)
{
	FskErr err = kFskErrNone;
	FskInt64 curPosition;

	if (fref->thePermissions == kFskFilePermissionReadOnly)
		return kFskErrReadOnly; 

	FskFSFileGetPosition(fref, &curPosition);
	FskFSFileSetPosition(fref, size);
	if (SetEndOfFile(fref->hFile))
		FskFSFileSetPosition(fref, &curPosition);
	else {
		DWORD winErr = GetLastError();

		if (winErr == ERROR_INVALID_PARAMETER)
			err = kFskErrDiskFull;
		else
			err = winErrorToFskErr(winErr);
	}

	return err;
}

FskErr FskFSFileSetPosition(FskFSFile fref, const FskInt64 *position)
{
	LONG positionLo = (UInt32)*position, positionHi = (UInt32)(*position >> 32);

	if (INVALID_SET_FILE_POINTER == SetFilePointer(fref->hFile, positionLo, &positionHi, FILE_BEGIN)) {
		if (NO_ERROR == GetLastError())
			return kFskErrNone;
		else
			return kFskErrOperationFailed;
	}
	else
		return kFskErrNone;
}

FskErr FskFSFileGetPosition(FskFSFile fref, FskInt64 *position)
{
	LONG positionHi = 0;
	LONG positionLo = SetFilePointer(fref->hFile, 0, &positionHi, FILE_CURRENT);
	*position = positionLo | (((FskInt64)positionHi) << 32);

	return kFskErrNone;
}

FskErr FskFSFileRead(FskFSFile fref, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead)
{
	DWORD bytesReadW;

	if (NULL != bytesRead)
		*bytesRead = 0;

	if (false == ReadFile(fref->hFile, buffer, bytesToRead, &bytesReadW, NULL)) {
		DWORD error = GetLastError();
		
		return winErrorToFskErr(error);
	}

	if (NULL == bytesRead) {
		if (bytesReadW != bytesToRead)
			return kFskErrOperationFailed;
	}
	else {
		*bytesRead = (UInt32)bytesReadW;
	}

	if ((0 == bytesReadW) && (0 != bytesToRead))
		return kFskErrEndOfFile;

	return kFskErrNone;
}

FskErr FskFSFileWrite(FskFSFile fref, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten)
{
	DWORD bytesWrittenW;

	if (fref->thePermissions == kFskFilePermissionReadOnly)
		return kFskErrReadOnly; 

	if (NULL != bytesWritten)
		*bytesWritten = 0;

	if (false == WriteFile(fref->hFile, buffer, bytesToWrite, &bytesWrittenW, NULL))
		return winErrorToFskErr(GetLastError());

	if (NULL == bytesWritten) {
		if (bytesToWrite != bytesWrittenW)
			return kFskErrOperationFailed;
	}
	else
		*bytesWritten = bytesWrittenW;

	return kFskErrNone;
}

FskErr FskFSFileGetFileInfo(const char *fullpath, FskFileInfo *itemInfo)
{
	WCHAR *path;
	WIN32_FILE_ATTRIBUTE_DATA attr;
	DWORD result;
	FskErr err;

	err = fixUpPathForWindows(fullpath, &path, kFskPathIsAny);
	if (err) return err;

	result = GetFileAttributesExW((LPCWSTR)path, GetFileExInfoStandard, &attr);
	FskMemPtrDispose(path);
	if (0 == result)
		return kFskErrFileNotFound;

	itemInfo->filesize = (((FskInt64)attr.nFileSizeHigh) << 32) | attr.nFileSizeLow;
	itemInfo->fileModificationDate = convertFileTime(&attr.ftLastWriteTime);
	itemInfo->fileCreationDate = convertFileTime(&attr.ftCreationTime);
	itemInfo->filetype = (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? kFskDirectoryItemIsDirectory : kFskDirectoryItemIsFile;
	if (kFskDirectoryItemIsFile == itemInfo->filetype) {
		char *ext = FskStrRChr((char *)fullpath, '.');
		if (ext && (0 == FskStrCompare(ext, ".lnk")))
			itemInfo->filetype = kFskDirectoryItemIsLink;
	}

	itemInfo->flags = 0;
	if (FILE_ATTRIBUTE_HIDDEN & attr.dwFileAttributes)
		itemInfo->flags |= kFileFileHidden;
	if (FILE_ATTRIBUTE_READONLY & attr.dwFileAttributes)
		itemInfo->flags |= kFileFileLocked;
	itemInfo->fileNode = 0;
// Commented to avoid runtime overhead
//	{
//		BY_HANDLE_FILE_INFORMATION info;
//		HANDLE file;
//		
//		file = CreateFile(path, 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_READONLY, NULL);
//		if (file != INVALID_HANDLE_VALUE) {
//			ZeroMemory(&info, sizeof(FileInformation));
//			if (GetFileInformationByHandle (file, &info)) {
//				itemInfo->fileNode = (info.nFileIndexHigh << 32) | info.nFileIndexLow;
//			}
//			CloseHandle(file);
//		}
//	}
	return kFskErrNone;
}

FskErr FskFSFileSetFileInfo(const char *fullpath, const FskFileInfo *itemInfo)
{
	WCHAR *path = NULL;
	WIN32_FILE_ATTRIBUTE_DATA attr;
	DWORD result;
	Boolean get, set;
	FskErr err;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	Boolean isFile;

	err = fixUpPathForWindows(fullpath, &path, kFskPathIsAny);
	BAIL_IF_ERR(err);

	result = GetFileAttributesExW((LPCWSTR)path, GetFileExInfoStandard, &attr);
	if (0 == result) {
		err = winErrorToFskErr(GetLastError());
		goto bail;
	}

	isFile = (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? false : true;

	if (isFile) {
		hFile = CreateFileW((LPCWSTR)path, FILE_WRITE_ATTRIBUTES, 0,
							NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hFile) {
			err = winErrorToFskErr(GetLastError());
			goto bail;
		}
	}

	set = (itemInfo->flags & kFileFileLocked) ? true : false;
	get = (FILE_ATTRIBUTE_READONLY & attr.dwFileAttributes) ? true : false;
	if (set != get) {
		if (set)
			attr.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
		else
			attr.dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
	}

	set = (itemInfo->flags & kFileFileHidden) ? true : false;
	get = (FILE_ATTRIBUTE_HIDDEN & attr.dwFileAttributes) ? true : false;
	if (set != get) {
		if (set)
			attr.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
		else
			attr.dwFileAttributes ^= FILE_ATTRIBUTE_HIDDEN;
	}

	SetFileAttributesW((LPCWSTR)path, attr.dwFileAttributes);

	if (isFile) {
		unconvertFileTime(itemInfo->fileCreationDate, &attr.ftCreationTime);
		unconvertFileTime(itemInfo->fileModificationDate, &attr.ftLastWriteTime);
		if (false == SetFileTime(hFile, &attr.ftCreationTime, NULL, &attr.ftLastWriteTime)) {
			err = winErrorToFskErr(GetLastError());
			goto bail;
		}
	}

bail:
	if (INVALID_HANDLE_VALUE != hFile)
		CloseHandle(hFile);
	FskMemPtrDispose(path);

	return err;
}

UInt32 convertFileTime(const FILETIME *ft)
{
	static const __int64 SECS_BETWEEN_EPOCHS = 11644473600;
	static const __int64 SECS_TO_100NS = 10000000;
	__int64 ut = ((__int64)ft->dwHighDateTime << 32) + ft->dwLowDateTime;
	ut -= (SECS_BETWEEN_EPOCHS * SECS_TO_100NS);
	return (UInt32)(ut / SECS_TO_100NS);
}

void unconvertFileTime(UInt32 secs, FILETIME *ft)
{
	static const __int64 SECS_BETWEEN_EPOCHS = 11644473600;
	static const __int64 SECS_TO_100NS = 10000000;
	__int64 ut = secs;
	ut *= SECS_TO_100NS;
	ut += (SECS_BETWEEN_EPOCHS * SECS_TO_100NS);
	ft->dwHighDateTime = (DWORD)(ut >> 32);
	ft->dwLowDateTime = (DWORD)ut;
}

FskErr fixUpPathForWindows(const char *fullPath, WCHAR **winPath, UInt32 pathType)
{
	FskErr err;
	WCHAR *result = NULL;
	UInt32 i, pathLen;

	if ((NULL == fullPath) || ((NULL == FskStrChr(fullPath, ':')) && !(('/' == fullPath[0]) && ('/' == fullPath[1])))) {
		BAIL(kFskErrInvalidParameter);			// only an absolute path is acceptable.
	}

	pathLen = FskStrLen(fullPath);
	if (kFskPathIsFile == pathType) {
		if ((0 != pathLen) && ('/' == fullPath[pathLen - 1])) {
			BAIL(kFskErrInvalidParameter);
		}
	}
	else
	if (kFskPathIsDirectory == pathType) {
		if ((0 != pathLen) && ('/' != fullPath[pathLen - 1])) {
			BAIL(kFskErrInvalidParameter);
		}
	}

	err = FskTextUTF8ToUnicode16LE((const unsigned char *)fullPath, pathLen, (UInt16 **)&result, NULL);
	BAIL_IF_ERR(err);

	for (i = 0; 0 != result[i]; i++) {
		if ('/' == result[i])
			result[i] = '\\';
	}

bail:
	*winPath = result;

	return err;
}

#include "commctrl.h"

FskErr FskFSFileGetThumbnail(const char *fullPath, const UInt32 width, const UInt32 height, FskBitmap *thumbnail)
{
	WCHAR *pathCopy = NULL;
	FskErr err = kFskErrNone;
	HIMAGELIST s_himlLarge = NULL;

	SHFILEINFOW s_sfiLarge          = {0};

	int cx, cy;
	FskColorRGBARecord fillColor = {0, 0, 0, 255};
	FskBitmap src = NULL, dst = NULL;
	SInt32 srcRowBytes, dstRowBytes;
	FskBitmapFormatEnum srcFormat, dstFormat;
	unsigned char *srcAddr, *dstAddr, *maskAddr;
	SInt32 i, j;

	fixUpPathForWindows(fullPath, (WCHAR **)&pathCopy, kFskPathIsFile);

	s_himlLarge = (HIMAGELIST)SHGetFileInfoW(pathCopy, 0, &s_sfiLarge, sizeof(s_sfiLarge), SHGFI_SYSICONINDEX | SHGFI_LARGEICON);

	if (NULL == s_himlLarge) {
		BAIL(kFskErrOperationFailed);
	}

	ImageList_GetIconSize(s_himlLarge, &cx, &cy);

	err = FskBitmapNew(-cx * 2, cy, kFskBitmapFormatDefaultNoAlpha, &src);
	BAIL_IF_ERR(err);

	err = FskBitmapNew(cx, cy, kFskBitmapFormatSourceDefaultRGBA, &dst);
	BAIL_IF_ERR(err);


	FskBitmapSetHasAlpha(dst, true);



	FskRectangleFill(src, &src->bounds, &fillColor, kFskGraphicsModeCopy, NULL);

	ImageList_Draw(s_himlLarge, s_sfiLarge.iIcon, src->hdc, 0, 0, ILD_NORMAL);

	FskBitmapReadBegin(src, &srcAddr, &srcRowBytes, &srcFormat);
	FskBitmapWriteBegin(dst, &dstAddr, &dstRowBytes, &dstFormat);

	ImageList_Draw(s_himlLarge, s_sfiLarge.iIcon, src->hdc, cx, 0, ILD_MASK);

	maskAddr = srcAddr + cx * 3;
	for (i = 0; i < cy; i ++) {
		for (j = 0; j < cx; j++, dstAddr += 4, maskAddr += 3, srcAddr += 3) {
			if (maskAddr[0] != 0)
				dstAddr[3] = 0;
			else {
				dstAddr[0] = srcAddr[0];
				dstAddr[1] = srcAddr[1];
				dstAddr[2] = srcAddr[2];
				dstAddr[3] = 255;
			}
		}
		dstAddr += dstRowBytes - (cx * 4);
		srcAddr += srcRowBytes - (cx * 3);
		maskAddr += srcRowBytes - (cx * 3);
	}

	FskBitmapReadEnd(src);
	FskBitmapWriteEnd(dst);

	*thumbnail = dst;

bail:
	FskBitmapDispose(src);
	FskMemPtrDispose(pathCopy);

	return err;
}

FskErr FskFSFileDelete(const char *fullPath)
{
	WCHAR *pathCopy = NULL;
	FskErr err = fixUpPathForWindows(fullPath, &pathCopy, kFskPathIsFile);
	WIN32_FILE_ATTRIBUTE_DATA attr;
	BAIL_IF_ERR(err);

	if (GetFileAttributesExW(pathCopy, GetFileExInfoStandard, (LPVOID)&attr) == 0) {
		BAIL(kFskErrFileNotFound);
	}

	if ((attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		BAIL(kFskErrIsDirectory);
	}

	if (DeleteFileW(pathCopy) == 0) {
		err = winErrorToFskErr(GetLastError());
		goto bail;
	}

bail:
	if (pathCopy)
		FskMemPtrDispose(pathCopy);
	return err;
}

FskErr FskFSFileRename(const char *fullPath, const char *newName)
{
	return doRename(fullPath, newName, 0);
}

FskErr FskFSFilePathToNative(const char *fskPath, char **nativePath)
{
	return fixUpPathForWindows(fskPath, (WCHAR **)nativePath, kFskPathIsAny);
}

FskErr FskFSFileResolveLink(const char *fullPath, char **resolved)
{
	HRESULT hr;
	IShellLink *psl;
	FskErr err;
	WCHAR *pathCopy = NULL;

	err = fixUpPathForWindows(fullPath, &pathCopy, kFskPathIsFile);
	BAIL_IF_ERR(err);

	err = kFskErrOperationFailed;

#if !defined(__cplusplus)
	hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (LPVOID *)&psl); 
#else
	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID *)&psl); 
#endif

	if (SUCCEEDED(hr)) {
		IPersistFile *ppf;

#if !defined(__cplusplus)
		hr = IShellLinkW_QueryInterface(psl, &IID_IPersistFile, (LPVOID *)&ppf); 
		if (SUCCEEDED(hr)) {
			hr = IPersistFile_Load(ppf, pathCopy, STGM_READ); 
			if (SUCCEEDED(hr)) {
				hr = IShellLinkW_Resolve(psl, NULL, 0); 
				if (SUCCEEDED(hr)) {
					WCHAR result[MAX_PATH + 1];
					WIN32_FIND_DATAW ffd;

					hr = IShellLinkW_GetPath(psl, (LPSTR)result, MAX_PATH, (WIN32_FIND_DATAA *)&ffd, 0);
					if (SUCCEEDED(hr)) {
						*resolved = fixUpPathForFsk(result);
						err = kFskErrNone;
					}
				}
			}

			IPersistFile_Release(ppf);
		}

		IShellLinkW_Release(psl);
#else
		hr = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf); 
		if (SUCCEEDED(hr)) {
			hr = ppf->Load(pathCopy, STGM_READ); 
			if (SUCCEEDED(hr)) {
				hr = psl->Resolve(NULL, 0); 
				if (SUCCEEDED(hr)) {
					WCHAR result[MAX_PATH + 1];
					WIN32_FIND_DATAW ffd;

					hr = psl->GetPath((LPSTR)result, MAX_PATH, (WIN32_FIND_DATAA *)&ffd, 0);
					if (SUCCEEDED(hr)) {
						*resolved = fixUpPathForFsk(result);
						err = kFskErrNone;
					}
				}
			}

			ppf->Release();
		}

		psl->Release();
#endif
	}


bail:
	FskMemPtrDispose(pathCopy);

	return err;
}

FskErr FskFSFileFlush(FskFSFile fref)
{
	return kFskErrNone;
}

FskErr FskFSFileCreate(const char *fullPath)
{
	FskErr err;
	HANDLE	hFile;
	WCHAR *pathCopy;
	WIN32_FILE_ATTRIBUTE_DATA attr;

	err = fixUpPathForWindows(fullPath, &pathCopy, kFskPathIsFile);
	BAIL_IF_ERR(err);

	hFile = CreateFileW(pathCopy, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,
				NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	if (INVALID_HANDLE_VALUE == hFile) {
		DWORD winErr = GetLastError();

		if (GetFileAttributesExW(pathCopy, GetFileExInfoStandard, &attr))
			err = kFskErrFileExists;
		else
			err = winErrorToFskErr(winErr);
	}
	else
		CloseHandle(hFile);

bail:
	FskMemPtrDispose(pathCopy);
	return err;
}

FskErr FskFSFileCreateDirectory(const char *fullPath)
{
	FskErr err;
	WCHAR *pathCopy;

	err = fixUpPathForWindows(fullPath, &pathCopy, kFskPathIsDirectory);
	BAIL_IF_ERR(err);

	if (false == CreateDirectoryW(pathCopy, NULL))
		err = winErrorToFskErr(GetLastError());

bail:
	FskMemPtrDispose(pathCopy);
	return err;
}

FskErr FskFSFileRenameDirectory(const char *fullPath, const char *newNameIn)
{
	return doRename(fullPath, newNameIn, FILE_ATTRIBUTE_DIRECTORY);
}

FskErr doRename(const char *fullPath, const char *newNameIn, DWORD directoryFlag)
{
	FskErr err;
	WCHAR *pathCopy = NULL;
	WCHAR *newPath = NULL, *newName = NULL;
	WCHAR *lastSlash;
	WIN32_FILE_ATTRIBUTE_DATA attr;
	char *fullPathCopy = NULL;

	// remove trailing '/'
	if ((directoryFlag != 0) && (fullPath[FskStrLen(fullPath) - 1] == '/')) {
		fullPathCopy = FskStrDoCopy(fullPath);

		if (fullPathCopy) {
			fullPathCopy[FskStrLen(fullPathCopy) - 1] = 0;
			err = fixUpPathForWindows(fullPathCopy, &pathCopy, kFskPathIsAny);
			FskMemPtrDispose(fullPathCopy);
			if (err) return err;
		}
		else
			return kFskErrOperationFailed;
	}
	else {
		err = fixUpPathForWindows(fullPath, &pathCopy, kFskPathIsFile);
		if (err) return err;
	}

	if (GetFileAttributesExW(pathCopy, GetFileExInfoStandard, &attr) == 0) {
		err = winErrorToFskErr(GetLastError());
		goto bail;
	}

	if ((attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != directoryFlag) {
		err = (directoryFlag ? kFskErrNotDirectory : kFskErrIsDirectory);
		goto bail;
	}

	FskTextUTF8ToUnicode16LE((const unsigned char *)newNameIn, FskStrLen(newNameIn) + 1, (UInt16 **)&newName, NULL);

	FskMemPtrNewClear((wcslen(pathCopy) + wcslen(newName) + 2) * 2, (FskMemPtr *)&newPath);
	lastSlash = pathCopy + wcslen(pathCopy);
	while (--lastSlash > pathCopy) {
		if ('\\' == *lastSlash)
			break;
	}
	wcsncpy(newPath, pathCopy, lastSlash - pathCopy + 1);
	wcscat(newPath, newName);

	if (MoveFileW(pathCopy, newPath) == 0) {
		err = winErrorToFskErr(GetLastError());
		goto bail;
	}

bail:
	if (newPath)
		FskMemPtrDispose(newPath);
	if (newName)
		FskMemPtrDispose(newName);
	if (pathCopy)
		FskMemPtrDispose(pathCopy);
	return err;
}

FskErr FskFSFileDeleteDirectory(const char *fullPath)
{
	Boolean result;
	WCHAR *pathCopy;
	FskErr err;

	err = fixUpPathForWindows(fullPath, &pathCopy, kFskPathIsDirectory);
	if (err) return err;

	result = RemoveDirectoryW(pathCopy);

	if (result == 0)
		err = winErrorToFskErr(GetLastError());

	FskMemPtrDispose(pathCopy);

	return err;
}

/*
	Volume iterator
	
	Note: we are not using FindFirstVolume because it isn't supported
		on 95 /98 / ME. Utlimately may want to do a run-time check.
*/

static WCHAR *doGetLogicalDriveStrings(void);
static Boolean hasVolume(WCHAR *volumeList, WCHAR *volume);

FskErr FskFSVolumeIteratorNew(FskFSVolumeIterator *volIt)
{
	FskErr err = kFskErrNone;
	FskFSVolumeIterator vi;

	err = FskMemPtrNewClear(sizeof(FskFSVolumeIteratorRecord), (FskMemPtr *)&vi);
	BAIL_IF_ERR(err);

	vi->volList = doGetLogicalDriveStrings();
	vi->cur = vi->volList;

	*volIt = vi;

bail:
	return err;
}

FskErr FskFSVolumeIteratorDispose(FskFSVolumeIterator volIt)
{
	if (NULL != volIt) {
		FskMemPtrDispose(volIt->volList);
		FskMemPtrDispose(volIt);
	}

	return kFskErrNone;
}

FskErr FskFSVolumeIteratorGetNext(FskFSVolumeIterator volIt, UInt32 *id, char **pathOut, char **nameOut)
{
	FskErr err = kFskErrNone;
	UInt32 len = (volIt->cur ? wcslen(volIt->cur) : 0);

	if (0 == len) {
		BAIL(kFskErrIteratorComplete);
	}

	// note: since this is just a drive letter, we don't bother transcoding the path to UTF-8
	if (pathOut) {
		FskTextUnicode16LEToUTF8((UInt16 *)volIt->cur, 2 * len, pathOut, NULL);
		(*pathOut)[len - 1] = '/';
	}

	if (nameOut) {
		WCHAR name[MAX_PATH];
		if (!GetVolumeInformationW(volIt->cur, name, MAX_PATH, NULL, NULL, NULL, NULL, 0)) {
			err = kFskErrOperationFailed;
			if (pathOut)
				FskMemPtrDisposeAt((void **)pathOut);
			goto bail;
		}
		FskTextUnicode16LEToUTF8((UInt16 *)name, 2 * wcslen(name), nameOut, NULL);
	}

	if (id)
		*id = (volIt->cur)[0];		//@@ maybe we should use the volume serial number here instead?

	volIt->cur += len + 1;

bail:
	return err;
}

/*
	Directories
*/

FskErr FskFSDirectoryIteratorNew(const char *directoryPath, FskFSDirectoryIterator *dirIt, UInt32 flags)
{
	FskErr err = kFskErrNone;
	FskFSDirectoryIterator di = NULL;
	FskFileInfo itemInfo;
	char *tempPath;

	err = FskMemPtrNewClear(sizeof(FskFSDirectoryIteratorRecord), (FskMemPtr *)&di);
	BAIL_IF_ERR(err);

	di->flags = flags;

	// check that this specifies a directory and nothing more (make sure we don't get wildcarding behavior)
	if (directoryPath[FskStrLen(directoryPath) - 1] != '/') {
		BAIL(kFskErrInvalidParameter);
	}

	// make sure path exists
	err = FskFileGetFileInfo(directoryPath, &itemInfo);

	if (err == kFskErrFileNotFound) {
		tempPath = FskStrDoCopy(directoryPath);

		if (tempPath) {
			// look for a file by the same name (remove the trailing '/')
			tempPath[FskStrLen(tempPath) - 1] = 0;
			err = FskFSFileGetFileInfo(tempPath, &itemInfo);
			FskMemPtrDispose(tempPath);
			BAIL_IF_ERR(err);
		}
		else
			goto bail;
	}

	if (itemInfo.filetype != kFskDirectoryItemIsDirectory) {
		BAIL(kFskErrNotDirectory);
	}

	fixUpPathForWindows(directoryPath, &di->dirPath, kFskPathIsAny);

bail:
	if ((kFskErrNone != err) && (NULL != di)) {
		FskMemPtrDispose(di);
		di = NULL;
	}

	*dirIt = di;

	return err;
}

FskErr FskFSDirectoryIteratorDispose(FskFSDirectoryIterator dirIt)
{
	if (dirIt) {
		FskMemPtrDispose(dirIt->dirPath);
		if ((NULL != dirIt->hFind) && (INVALID_HANDLE_VALUE != dirIt->hFind))
			FindClose(dirIt->hFind);

		FskMemPtrDispose(dirIt);
	}

	return kFskErrNone;
}

FskErr FskFSDirectoryIteratorGetNext(FskFSDirectoryIterator dirIt, char **name, UInt32 *itemType)
{
	if (NULL != name)
		*name = NULL;
	if (NULL != itemType)
		*itemType = 0;

	while (true) {
		if (NULL == dirIt->hFind) {
			// first file
			WCHAR *searchString;
			FskErr err;

			err = FskMemPtrNew(2 * (wcslen(dirIt->dirPath) + 2), (FskMemPtr *)&searchString);
			if (kFskErrNone != err)
				return err;
			wcscpy(searchString, dirIt->dirPath);
			wcscat(searchString, L"*");
			dirIt->hFind = FindFirstFileW(searchString, &dirIt->findData);
			FskMemPtrDispose(searchString);
			if (INVALID_HANDLE_VALUE == dirIt->hFind) {
				DWORD winErr = GetLastError();
				if ((ERROR_FILE_NOT_FOUND == winErr) || (ERROR_NO_MORE_FILES == winErr))
					return kFskErrIteratorComplete;
				else
				if (NOERROR == winErr)
					return kFskErrIteratorComplete;
				return winErrorToFskErr(winErr);
			}
		}
		else {
			// other files
			if (false == FindNextFileW(dirIt->hFind, &dirIt->findData)) {
				DWORD winErr = GetLastError();
				if (ERROR_NO_MORE_FILES == winErr) 
					return kFskErrIteratorComplete;
				else
				if (NOERROR == winErr)
					return kFskErrIteratorComplete;
				return winErrorToFskErr(winErr);
			}
		}

		// we don't want useless, distracting results...
		if ((0 != (FILE_ATTRIBUTE_HIDDEN & dirIt->findData.dwFileAttributes)) &&
			 (0 == (1 & dirIt->flags)))
			continue;

		if ((0 == wcscmp(dirIt->findData.cFileName, L".")) ||
			(0 == wcscmp(dirIt->findData.cFileName, L"..")))
			continue;

		// return the name & object type
		if (NULL != name) {
			FskErr err = FskTextUnicode16LEToUTF8((const UInt16 *)dirIt->findData.cFileName, 2 * (wcslen(dirIt->findData.cFileName) + 1), name, NULL);
			if (kFskErrNone != err)
				return err;
		}
		if (NULL != itemType) {
			if (FILE_ATTRIBUTE_DIRECTORY & dirIt->findData.dwFileAttributes)
				*itemType = kFskDirectoryItemIsDirectory;
			else {
				*itemType = kFskDirectoryItemIsFile;
				if (dirIt->findData.cFileName[0]) {
					WCHAR *ext = wcsrchr(dirIt->findData.cFileName, '.');
					if (ext && (0 == _wcsicmp(ext, L".lnk")))
						*itemType = kFskDirectoryItemIsLink;
				}
			}
		}
		break;
	}

	return kFskErrNone;
}

FskErr FskFSFileMap(const char *fullPath, unsigned char **data, FskInt64 *dataSize, FskFSFileMapping *mapOut)
{
	FskErr err = kFskErrNone;
	FskFSFileMapping map = NULL;
	FskInt64 size;

	err = FskMemPtrNewClear(sizeof(FskFSFileMappingRecord), (FskMemPtr *)&map);
	BAIL_IF_ERR(err);

	err = FskFSFileOpen(fullPath, kFskFilePermissionReadOnly, &map->file);
	BAIL_IF_ERR(err);

	FskFSFileGetSize(map->file, &size);
	if (size > 0xffffffff) {
		BAIL(kFskErrOperationFailed);
	}

	map->mapping = CreateFileMapping(map->file->hFile, NULL, PAGE_READONLY, (UInt32)(size >> 32), (UInt32)size, NULL);
	if (NULL == map->mapping) {
		BAIL(kFskErrOperationFailed);
	}

	map->address = MapViewOfFile(map->mapping, FILE_MAP_READ, 0, 0, (SIZE_T)size);
	if (NULL == map->address) {
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
		if (map->address)
			UnmapViewOfFile(map->address);
		if (map->mapping)
			CloseHandle(map->mapping);
		FskFSFileClose(map->file);
		FskMemPtrDispose(map);
	}
	return kFskErrNone;
}

FskErr FskFSDirectoryGetSpecialPath(UInt32 type, const Boolean create, const char *volumeName, char **fullPath)
{
	FskErr err;
	int nFolder;
	WCHAR *path = NULL;
	char *t1 = NULL;

	*fullPath = NULL;
	if (kFskDirectorySpecialTypeTemporary != type) {
		err = specialDirectoryTypeToCSIDL(type, &nFolder);
		BAIL_IF_ERR(err);

		err = FskMemPtrNewClear(MAX_PATH * 2, (FskMemPtr *)&path);
		BAIL_IF_ERR(err);

		if (!SHGetSpecialFolderPathW(NULL, path, nFolder, create)) {
			BAIL(kFskErrFileNotFound);
		}

		t1 = fixUpPathForFsk(path);
		err = FskMemPtrNewClear(FskStrLen(t1) + 20, (FskMemPtr *)fullPath);
		BAIL_IF_ERR(err);

		FskStrCopy(*fullPath, t1);
		type &= ~kFskDirectorySpecialTypeSharedFlag;
		FskStrCat(*fullPath, (kFskDirectorySpecialTypeApplicationPreference != type) ? "/" : "/kinoma/");
	}
	else {
		WCHAR wp[MAX_PATH];
		DWORD result = GetTempPathW(MAX_PATH, wp);

		if (0 == result) {
			err = winErrorToFskErr(GetLastError());
			goto bail;
		}	

		*fullPath = fixUpPathForFsk(wp);

		err = kFskErrNone;
	}

bail:
	FskMemPtrDispose(path);
	FskMemPtrDispose(t1);

	return err;
}

char *fixUpPathForFsk(const WCHAR *fullPath)
{
	char *toReplace, *result;

	FskTextUnicode16LEToUTF8((UInt16 *)fullPath, wcslen(fullPath) * 2, &result, NULL);
	for (toReplace = strchr(result, '\\'); NULL != toReplace; toReplace = strchr(toReplace, '\\'))
		*toReplace = '/';

	return result;
}

FskErr specialDirectoryTypeToCSIDL(const UInt32 fskSpecialDirectoryType, int *nFolder)
{
	FskErr error;

	error = kFskErrNone;
	switch (fskSpecialDirectoryType) {
		case kFskDirectorySpecialTypeDocument:
			*nFolder = CSIDL_PERSONAL;
			break;
		case kFskDirectorySpecialTypeMusic:
			*nFolder = CSIDL_MYMUSIC;
			break;

		case kFskDirectorySpecialTypePhoto:
			*nFolder = CSIDL_MYPICTURES;
			break;

		case kFskDirectorySpecialTypeVideo:
			*nFolder = CSIDL_MYVIDEO;
			break;

		case kFskDirectorySpecialTypeStartMenu:
			*nFolder = CSIDL_STARTMENU;
			break;

		case kFskDirectorySpecialTypeTV:
			*nFolder = CSIDL_MYVIDEO; // Tentative!!
			break;

		case kFskDirectorySpecialTypeApplicationPreference:
		case kFskDirectorySpecialTypeApplicationPreferenceRoot:
			*nFolder = CSIDL_LOCAL_APPDATA;
			break;

		case kFskDirectorySpecialTypeDocument | kFskDirectorySpecialTypeSharedFlag:
			*nFolder = CSIDL_COMMON_DOCUMENTS;
			break;

		case kFskDirectorySpecialTypeMusic | kFskDirectorySpecialTypeSharedFlag:
			*nFolder = CSIDL_COMMON_MUSIC;
			break;

		case kFskDirectorySpecialTypePhoto | kFskDirectorySpecialTypeSharedFlag:
			*nFolder = CSIDL_COMMON_PICTURES;
			break;

		case kFskDirectorySpecialTypeVideo | kFskDirectorySpecialTypeSharedFlag:
			*nFolder = CSIDL_COMMON_VIDEO;
			break;

		case kFskDirectorySpecialTypeApplicationPreference | kFskDirectorySpecialTypeSharedFlag:
		case kFskDirectorySpecialTypeApplicationPreferenceRoot | kFskDirectorySpecialTypeSharedFlag:
			*nFolder = CSIDL_COMMON_APPDATA;
			break;

		case kFskDirectorySpecialTypeCache:
			*nFolder = CSIDL_INTERNET_CACHE;
			break;
			
		default:
			error = kFskErrInvalidParameter;
			break;
	}

	return error;
}

static void callVolumeListChangedCallback(void *arg0, void *arg1, void *arg2, void *arg3);

static FskList gVolNotifiers;

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

	FskMutexAcquire(gVolNotifiersMutex);

	volNtf->dispatch.dispatch = &gFSDispatch;
	volNtf->dispatch.refcon = NULL;
	volNtf->callback = callback;
	volNtf->refCon = refCon;
	volNtf->volumes = doGetLogicalDriveStrings();
	volNtf->callbackThread = FskThreadGetCurrent();
	FskListPrepend(&gVolNotifiers, &volNtf->next);

	FskMutexRelease(gVolNotifiersMutex);

bail:
	*volNtfOut = volNtf;

	return err;
}

FskErr FskFSVolumeNotifierDispose(FskFSVolumeNotifier volNtf)
{
	FskMutexAcquire(gVolNotifiersMutex);

	FskListRemove(&gVolNotifiers, &volNtf->next);

	FskMemPtrDispose(volNtf->volumes);
	FskMemPtrDispose(volNtf);

	FskMutexRelease(gVolNotifiersMutex);

	return kFskErrNone;
}

void volumeListChanged(const WCHAR *path, UInt32 op)
{
	FskFSVolumeNotifier walker = NULL;

	if (NULL == gVolNotifiersMutex)
		return;

	FskMutexAcquire(gVolNotifiersMutex);

	switch (op) {
		case 0:		// flush
			FskMemPtrDisposeAt((void **)&gVolumeListCache);
			gVolumeListCacheLength = 0;
			break;

		case 1:		// add
			if (NULL != gVolumeListCache) {		// make sure it isn't already in the list
				WCHAR *p = gVolumeListCache;
				while (*p) {
					UInt32 itemLen = wcslen(p) + 1;
					if (0 == wcscmp(p, path))
						goto done;
					p += itemLen;
				}
			}

			{
			UInt32 volSerialNumber;
			if (0 != GetVolumeInformationW(path, NULL, 0, &volSerialNumber, NULL, NULL, NULL, 0)) {
				if (kFskErrNone == FskMemPtrRealloc((1 + 1 + wcslen(path)) * 2 + gVolumeListCacheLength, (FskMemPtr *)&gVolumeListCache)) {
					if (!gVolumeListCacheLength)
						gVolumeListCacheLength = 2;
					wcscpy((WCHAR *)(gVolumeListCacheLength - 2 + (char *)gVolumeListCache), path);
					gVolumeListCacheLength += (1 + wcslen(path)) * 2;
					*(UInt16 *)(gVolumeListCacheLength - 2 + (char *)gVolumeListCache) = 0;
				}
			}
			else
				goto done;
			}
			break;

		case 2:		// remove
			if (NULL != gVolumeListCache) {
				WCHAR *p = gVolumeListCache;
				while (*p) {
					UInt32 itemLen = wcslen(p) + 1;
					if (0 == wcscmp(p, path)) {
						char *next = (char *)(p + itemLen);
						FskMemMove(p, next, gVolumeListCacheLength - (next -(char *)gVolumeListCache));
						gVolumeListCacheLength -= 2 * itemLen;
						break;
					}
					p += itemLen;
				}
			}
			break;
	}

	while (gVolNotifiers) {
		FskFSVolumeNotifier notifier;

		walker = (FskFSVolumeNotifier)FskListGetNext(gVolNotifiers, walker);
		if (NULL == walker)
			break;

		notifier = (FskFSVolumeNotifier)(((char *)walker) - offsetof(FskFSVolumeNotifierRecord, next));
		FskThreadPostCallback(notifier->callbackThread, callVolumeListChangedCallback, notifier, NULL, NULL, NULL);
	}

done:
	FskMutexRelease(gVolNotifiersMutex);
}

void callVolumeListChangedCallback(void *arg0, void *arg1, void *arg2, void *arg3)
{
	FskFSVolumeNotifier volNtf = (FskFSVolumeNotifier)arg0;
	WCHAR *currentVolumes, *walker;
	Boolean valid;

	FskMutexAcquire(gVolNotifiersMutex);
		valid = FskListContains(gVolNotifiers, offsetof(FskFSVolumeNotifierRecord, next) + (char *)volNtf);
	FskMutexRelease(gVolNotifiersMutex);

	if (!valid)
		return;

	currentVolumes = doGetLogicalDriveStrings();

	// look for anything that got lost
	for (walker = volNtf->volumes; 0 != *walker; walker += (wcslen(walker) + 1)) {
		if (!hasVolume(currentVolumes, walker))
			(volNtf->callback)(kFskVolumeBye, walker[0], volNtf->refCon);
	}

	// announce anything new
	for (walker = currentVolumes; 0 != *walker; walker += (wcslen(walker) + 1)) {
		if (!hasVolume(volNtf->volumes, walker))
			(volNtf->callback)(kFskVolumeHello, walker[0], volNtf->refCon);
	}

	FskMemPtrDispose(volNtf->volumes);
	volNtf->volumes = currentVolumes;
}

// filters out any empty drives
WCHAR *doGetLogicalDriveStrings(void)
{
	UInt32 count;
	WCHAR *volumes, *in, *out;

	if (NULL == gVolNotifiersMutex) {
		if (kFskErrNone != FskMutexNew(&gVolNotifiersMutex, "VolNotifiersMutex"))
			return NULL;
	}

	FskMutexAcquire(gVolNotifiersMutex);

	if (NULL != gVolumeListCache) {
		BAIL_IF_ERR(FskMemPtrNewFromData(gVolumeListCacheLength, gVolumeListCache, (FskMemPtr *)&volumes));
		volumes = NULL;
		goto bail;
	}

	count = GetLogicalDriveStringsW(0, NULL);
	BAIL_IF_ERR(FskMemPtrNew((count * 2) + 2, (FskMemPtr *)&volumes));
	GetLogicalDriveStringsW(count, volumes);

	in = volumes;
	out = volumes;
	while (*in) {
		UInt32 volSerialNumber;
		UInt32 len = wcslen(in) + 1;
		if (GetVolumeInformationW(in, NULL, 0, &volSerialNumber, NULL, NULL, NULL, 0)) {
			wcscpy(out, in);
			out += len;
		}

		in += len;
	}
	*out++ = 0;

	gVolumeListCacheLength = (out - volumes) * 2;
	gVolumeListCache = volumes;

	FskMemPtrNewFromData(gVolumeListCacheLength, gVolumeListCache, (FskMemPtr *)&volumes);

bail:
	FskMutexRelease(gVolNotifiersMutex);

	return volumes;
}

Boolean hasVolume(WCHAR *volumeList, WCHAR *volume)
{
	WCHAR *walker;
	for (walker = volumeList; 0 != *walker; walker += (wcslen(walker) + 1))
		if (0 == wcscmp(walker, volume))
			return true;
	return false;
}

FskErr FskFSVolumeGetInfo(UInt32 volumeID, char **pathOut, char **nameOut, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace)
{
	char path[4];

	path[0] = (char)volumeID;
	path[1] = ':';
	path[2] = '/';
	path[3] = 0;

	return FskFSVolumeGetInfoFromPath(path, pathOut, nameOut, volumeType, isRemovable, capacity, freeSpace);
}

FskErr FskFSVolumeGetInfoFromPath(const char *pathIn, char **pathOut, char **nameOut, UInt32 *volumeTypeOut, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace)
{
	FskErr err;
	UInt32 driveType;
	WCHAR *pathTemp = NULL, *path = NULL;
	UInt32 volumeType = kFskVolumeTypeUnknown;

	err = fixUpPathForWindows(pathIn, &pathTemp, kFskPathIsAny);
	BAIL_IF_ERR(err);

	err = FskMemPtrNew((1 + FskUnicodeStrLen((UInt16 *)pathTemp)) * 2, (FskMemPtr *)&path);
	BAIL_IF_ERR(err);

	if (!GetVolumePathNameW(pathTemp, path, 1 + FskUnicodeStrLen((UInt16 *)pathTemp))) {
		BAIL(kFskErrFileNotFound);
	}

	if (nameOut) {
		WCHAR name[MAX_PATH];
		if (!GetVolumeInformationW(path, name, MAX_PATH, NULL, NULL, NULL, NULL, 0)) {
			err = winErrorToFskErr(GetLastError());
			goto bail;
		}
		FskTextUnicode16LEToUTF8((UInt16 *)name, 2 * wcslen(name), nameOut, NULL);
	}

	if (volumeTypeOut || isRemovable) {
		driveType = GetDriveTypeW(path);
		switch (driveType) {
			case DRIVE_FIXED:
				volumeType = kFskVolumeTypeFixed;
				break;

			case DRIVE_CDROM:	// it just means optical disc
				volumeType = kFskVolumeTypeOptical;
				break;

			case DRIVE_REMOVABLE: {
				// we may want to share this code with other platforms
				UInt32 volumeID;
				char *t;

				volumeType = kFskVolumeTypeUnknown;

				t = fixUpPathForFsk(path);
				if (kFskErrNone == FskFSVolumeGetID(t, &volumeID)) {
					char *product, *vendor, *vendorSpecific;

					if (kFskErrNone == FskFSVolumeGetDeviceInfo(volumeID, &vendor, &product, NULL, &vendorSpecific)) {
						if (FskStrStr(product, "MSC") || FskStrStr(vendorSpecific, "MEMORYSTICK") ||
							(0 == FskStrCompareWithLength(vendor, "MEMORYST", 8) && (0 == FskStrCompareWithLength(product, "ICK", 3))))
							volumeType = kFskVolumeTypeMemoryStick;
						else if (FskStrStr(product, "SD"))
							volumeType = kFskVolumeTypeSDMemory;
						else if (FskStrStr(product, "MMC"))
							volumeType = kFskVolumeTypeMMC;
						else if (FskStrStr(product, "CFC"))
							volumeType = kFskVolumeTypeCompactFlash;
						else if (FskStrStr(product, "SMC"))
							volumeType = kFskVolumeTypeSmartMedia;

						FskMemPtrDispose(vendor);
						FskMemPtrDispose(product);
						FskMemPtrDispose(vendorSpecific);
					}
				}
				FskMemPtrDispose(t);
				}	
				break;

			case DRIVE_REMOTE:
				volumeType = kFskVolumeTypeNetwork;
				break;
		}
	}

	if (kFskVolumeTypeUnknown == volumeType) {
		if ((0 == wcscmp(L"A:\\", path)) || (0 == wcscmp(L"B:\\", path)))
			volumeType = kFskVolumeTypeFD;			// yea, well... if you know a better way to do this on Windows, please share.
	}

	if (volumeTypeOut)
		*volumeTypeOut = volumeType;

	if (isRemovable)
		*isRemovable = (DRIVE_REMOVABLE == driveType) || (DRIVE_CDROM == driveType) ||
						(DRIVE_REMOTE == driveType) || (kFskVolumeTypeFD == volumeType);

	if (freeSpace || capacity) {
		ULARGE_INTEGER free, total;
		GetDiskFreeSpaceExW(path, &free, &total, NULL);
		if (freeSpace)
			*freeSpace = free.QuadPart;
		if (capacity)
			*capacity = total.QuadPart;
	}

bail:
	if ((kFskErrNone == err) && (NULL != pathOut))
		*pathOut = fixUpPathForFsk(path);

	FskMemPtrDispose(path);
	FskMemPtrDispose(pathTemp);

	return err;
}

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
	SCSI_PASS_THROUGH	spt;
	ULONG				filler;	// realign buffers to double word boundary
	UCHAR				ucSenseBuf[24];
	UCHAR				ucDataBuf[255];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

FskErr FskFSVolumeGetDeviceInfo(UInt32 volumeID, char **vendor, char **product, char **revision, char **vendorSpecific)
{
	FskErr err = kFskErrNone;
	HANDLE device;
	SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
	DWORD bufferSize, returned;
	char name[7];

	name[0] = name[1] = name[3] = '\\';
	name[2] = '.';
	name[4] = (char)volumeID;
	name[5] = ':';
	name[6] = 0;

	err = FskWinCreateFile(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL, &device);
	BAIL_IF_ERR(err);

	ZeroMemory(&sptwb, sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));
	sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
	sptwb.spt.PathId = 0;
	sptwb.spt.TargetId = 1;
	sptwb.spt.Lun = 0;
	sptwb.spt.CdbLength = 6;
	sptwb.spt.SenseInfoLength = 24;
	sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
	sptwb.spt.DataTransferLength = 255;
	sptwb.spt.TimeOutValue = 1;	// sec
	sptwb.spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf);
	sptwb.spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucSenseBuf);
	sptwb.spt.Cdb[0] = 0x12;	// inquiry
	sptwb.spt.Cdb[4] = (UCHAR)sptwb.spt.DataTransferLength;	// allocation length
	bufferSize = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf) + sptwb.spt.DataTransferLength;
	if (!DeviceIoControl(device, IOCTL_SCSI_PASS_THROUGH, &sptwb, sizeof(SCSI_PASS_THROUGH), &sptwb, bufferSize, &returned, NULL)
		|| sptwb.spt.ScsiStatus || !sptwb.spt.DataTransferLength) {
		BAIL(kFskErrOperationFailed);
	}

	if (vendor)
		FskTextToUTF8((const char *)&sptwb.ucDataBuf[8], 8, vendor, NULL);
	if (product)
		FskTextToUTF8((const char *)&sptwb.ucDataBuf[16], 16, product, NULL);
	if (revision)
		FskTextToUTF8((const char *)&sptwb.ucDataBuf[32], 4, revision, NULL);
	if (vendorSpecific)
		FskTextToUTF8((const char *)&sptwb.ucDataBuf[36], 20, vendorSpecific, NULL);

bail:
	if (INVALID_HANDLE_VALUE != device)
		CloseHandle(device);

	return err;
}

FskErr FskFSVolumeGetID(const char *fullPath, UInt32 *volumeID)
{
	FskErr err = kFskErrNone;
	WCHAR *path = NULL;

	err = fixUpPathForWindows(fullPath, &path, kFskPathIsAny);
	BAIL_IF_ERR(err);

	if (GetVolumeInformationW(path, NULL, 0, NULL, NULL, NULL, NULL, 0) && 3 <= FskStrLen(fullPath) && ':' == fullPath[1] && '/' == fullPath[2])
		*volumeID = toupper(fullPath[0]);
	else
		err = kFskErrInvalidParameter;

bail:
	FskMemPtrDispose(path);

	return err;
}

FskErr FskFSVolumeEject(const char *fullPath, UInt32 eject)
{
#if 1
	FskErr err = kFskErrOperationFailed;
	char *msg = NULL, *msg2 = NULL;

	if (FskStrLen(fullPath) < 2)
		goto bail;

	msg = FskStrDoCat("open ", fullPath);
	if (NULL == msg)
		goto bail;

	msg[7] = 0;
	msg2 = FskStrDoCat(msg, " type cdaudio alias cdromdrive");
	if (NULL == msg2)
		goto bail;

	//  alias to drive letter of target
	//	mciSendString("open G: type cdaudio alias cdromdrive", NULL, 0, 0);
	mciSendString(msg2, NULL, 0, 0);

	if (0 == mciSendString("set cdromdrive door open", NULL, 0, NULL))
		err = kFskErrNone;

	// close aliased drive
	mciSendString("close cdromdrive", NULL, 0, 0);  

bail:
	FskMemPtrDispose(msg);
	FskMemPtrDispose(msg2);
	return err;
#else

	FskErr err = kFskErrOperationFailed;
	DWORD bytes = 0;
	HANDLE h;
	char *path = FskStrDoCat("\\\\.\\", fullPath);
	UInt32 pathLen = FskStrLen(path);
	if ('/' == path[pathLen - 1])
		path[pathLen - 1] = 0;
	h = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (INVALID_HANDLE_VALUE != h) {
		if (DeviceIoControl(h, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes, NULL)) {
			if (DeviceIoControl(h, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &bytes, NULL))
				err = kFskErrNone;
		}
		CloseHandle(h);
	}
	FskMemPtrDispose(path);

	return err;
#endif
}

static HHOOK ghHook;
static HWND beginModal(void);
static void endModal(HWND fore);
static LONG CALLBACK OFNHook(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK CommonDialogHook(int nCode, WPARAM wParam, LPARAM lParam);
static int CALLBACK BrowseFolderCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

FskErr FskFSFileChoose(const FskFileChooseEntry types, const char *prompt, Boolean allowMultiple, const char *initialPath, char **files)
{
	FskErr err = kFskErrNone;
	OPENFILENAMEW ofn;
	WCHAR *szFile = NULL;
	HWND saveFore;
	WCHAR *promptW = NULL, *initialPathW = NULL;
	BOOL disposeFilter = false;

	*files = NULL;

	// Initialize OPENFILENAMEW
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.nMaxFile = 64 * 1064L;
	if (kFskErrNone != FskMemPtrNew(ofn.nMaxFile * 2, (FskMemPtr *)&ofn.lpstrFile))
		goto bail;
	szFile = ofn.lpstrFile;
	szFile[0] = 0;

	if (prompt)
		FskTextUTF8ToUnicode16LE((const unsigned char *)prompt, FskStrLen(prompt), (UInt16 **)&promptW, NULL);
	if (initialPath) {
		err = fixUpPathForWindows(initialPath, &initialPathW, kFskPathIsDirectory);
		BAIL_IF_ERR(err);
	}

	// remap file types
	if (NULL == types) {
		ofn.lpstrFilter = L"All Documents\0*.*\0\0";
		ofn.nFilterIndex = 0;
	}
	else {
		FskFileChooseEntry walker;
		UInt32 bytes = 1;
		UInt32 entryCount = 0;
		WCHAR *out;

		// count
		for (walker = types; walker->extension != NULL; walker++, entryCount++) {
			char *p = walker->extension;
			bytes += FskStrLen(walker->label) + 2;
			while (0 != *p) {
				UInt32 len = FskStrLen(p) + 1;
				bytes += len + 3;
				p += len;
			}
			bytes += 6;
		}

		// allocate
		err = FskMemPtrNew(bytes * 2, (FskMemPtr *)&ofn.lpstrFilter);
		BAIL_IF_ERR(err);
		disposeFilter = true;

		// convert
		out = (WCHAR *)ofn.lpstrFilter;
		for (walker = types; walker->extension != NULL; walker++) {
			WCHAR *u16;
			char *p;

			err = FskTextUTF8ToUnicode16LE((const unsigned char *)walker->label, FskStrLen(walker->label), (UInt16 **)&u16, NULL);
			BAIL_IF_ERR(err);

			wcscpy(out, u16);
			out += wcslen(u16) + 1;

			FskMemPtrDispose(u16);

			p = walker->extension;
			while (0 != *p) {
				UInt32 len = FskStrLen(p);
				err = FskTextUTF8ToUnicode16LE((const unsigned char *)p, len, (UInt16 **)&u16, NULL);
				BAIL_IF_ERR(err);

				wcscpy(out, L"*.");
				out += 2;
				wcscpy(out, u16);
				out += wcslen(u16);
				*out++ = L';';

				FskMemPtrDispose(u16);

				p += len + 1;
			}
			*out++ = 0;
		}
		*out++ = 0;
	}
	ofn.lpstrTitle = promptW;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = initialPathW;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_HIDEREADONLY;
	if (allowMultiple)
		ofn.Flags |= OFN_ALLOWMULTISELECT;

	saveFore = beginModal();
	ofn.hwndOwner = saveFore;

	// Hook to force dialog to stay on top
	ghHook = SetWindowsHookEx(WH_CBT, CommonDialogHook, hInst, GetCurrentThreadId());

	if (GetOpenFileNameW(&ofn)) {
		char *result = NULL;
		if (ofn.nFileOffset < wcslen(szFile)) {
			// one file @@ crash-o-rama in unicode case
			char *path = fixUpPathForFsk(szFile);
			FskMemPtrNewClear(FskStrLen(path) + 2, (FskMemPtr *)&result);
			FskMemMove(result, path, FskStrLen(path));
			FskMemPtrDispose(path);
		}
		else {
			// multiple files
			UInt32 size = 0;
			WCHAR *walker = szFile + wcslen(szFile) + 1;
			while (*walker) {
				UInt32 itemSize = wcslen(szFile) + wcslen(walker) + 2;
				char *item;
				WCHAR *path;
				UInt32 itemLen;
				FskMemPtrNew(itemSize * 2, (FskMemPtr *)&path);
				wcscpy(path, szFile);
				wcscat(path, L"\\");
				wcscat(path, walker);
				item = fixUpPathForFsk(path);
				FskMemPtrDispose(path);
				itemLen = FskStrLen(item);
				FskMemPtrRealloc(itemLen + size + 2, (FskMemPtr *)&result);
				FskMemMove(result + size, item, itemLen);
				FskMemPtrDispose(item);
				result[size + itemLen] = 0;
				result[size + itemLen + 1] = 0;
				size += itemLen + 1;
				walker += wcslen(walker) + 1;
			}
		}

		*files = result;
	}

	endModal(saveFore);

bail:
	FskMemPtrDispose((char *)szFile);
	FskMemPtrDispose(initialPathW);
	FskMemPtrDispose(promptW);

	if (disposeFilter)
		FskMemPtrDispose((FskMemPtr)ofn.lpstrFilter);

	return err;
}

FskErr FskFSFileChooseSave(const char *defaultName, const char *prompt, const char *initialPath, char **file)
{
	FskErr err = kFskErrNone;
	OPENFILENAMEW ofn;
	WCHAR szFile[260];
	HWND saveFore;
	WCHAR *promptW = NULL, *initialPathW = NULL;

	*file = NULL;

	if (prompt)
		FskTextUTF8ToUnicode16LE((const unsigned char *)prompt, FskStrLen(prompt), (UInt16 **)&promptW, NULL);
	if (initialPath) {
		err = fixUpPathForWindows(initialPath, &initialPathW, kFskPathIsDirectory);
		BAIL_IF_ERR(err);
	}

	// Initialize OPENFILENAMEW
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szFile;
	szFile[0] = 0;
	if (defaultName) {
		WCHAR *defaultNameW;
		if (kFskErrNone == FskTextUTF8ToUnicode16LE((const unsigned char *)defaultName, FskStrLen(defaultName), (UInt16 **)&defaultNameW, NULL)) {
			if ((wcslen(defaultNameW) * 2) < sizeof(szFile))
				wcscpy(szFile, defaultNameW);
		}
		FskMemPtrDispose(defaultNameW);
	}
	ofn.nMaxFile = sizeof(szFile) / 2;
	ofn.lpstrFilter = NULL;
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = promptW;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = initialPathW;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;

	saveFore = beginModal();
	ofn.hwndOwner = saveFore;

	// Hook to force dialog to stay on top
	ghHook = SetWindowsHookEx(WH_CBT, CommonDialogHook, hInst, GetCurrentThreadId());

	if (GetSaveFileNameW(&ofn)) {
		char *path = fixUpPathForFsk(szFile);	//@@ crash-o-rama in unicode case
		FskMemPtrNewClear(FskStrLen(path) + 2, (FskMemPtr *)file);
		FskMemMove(*file, path, FskStrLen(path));
		FskMemPtrDispose(path);
	}

	endModal(saveFore);

bail:
	FskMemPtrDispose(initialPathW);
	FskMemPtrDispose(promptW);

	return err;
}

FskErr FskFSDirectoryChoose(const char *prompt, const char *initialPath, char **path)
{
	FskErr err = kFskErrNone;
	BROWSEINFOW bi;
	LPITEMIDLIST pidl;
	IMalloc *imalloc;
	HWND saveFore;
	WCHAR *initialPathW = NULL;
	WCHAR *promptW = NULL;

	*path = NULL;

	if (prompt)
		FskTextUTF8ToUnicode16LE((const unsigned char *)prompt, FskStrLen(prompt), (UInt16 **)&promptW, NULL);
	if (initialPath)
		fixUpPathForWindows(initialPath, &initialPathW, kFskPathIsDirectory);

	ZeroMemory(&bi, sizeof(bi));
	bi.lpszTitle = promptW;
	bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_VALIDATE;

	if (initialPath) {
		bi.lpfn = (BFFCALLBACK)BrowseFolderCallbackProc;
		bi.lParam = (LPARAM)initialPathW;
	}

	saveFore = beginModal();
	bi.hwndOwner = saveFore;

	// Hook to force dialog to stay on top
	ghHook = SetWindowsHookEx(WH_CBT, CommonDialogHook, hInst, GetCurrentThreadId());

	SHGetMalloc(&imalloc);
	pidl = SHBrowseForFolderW(&bi);
	if (pidl) {
		WCHAR thePath[MAX_PATH];

		if (TRUE == SHGetPathFromIDListW(pidl, thePath)) {
			char *fskPath = fixUpPathForFsk(thePath);
			UInt32 fskPathLen = FskStrLen(fskPath);
			err = FskMemPtrNew(fskPathLen + 2, (FskMemPtr *)path);
			if (kFskErrNone == err) {
				FskStrCopy(*path, fskPath);
				if ('/' != (*path)[fskPathLen - 1])
					(*path)[fskPathLen] = '/';
				(*path)[fskPathLen + 1] = 0;
			}
			FskMemPtrDispose(fskPath);
		}
#if !defined(__cplusplus)
		imalloc->lpVtbl->Free(imalloc, pidl);
#else
		imalloc->Free(pidl);
#endif
	}

#if !defined(__cplusplus)
	imalloc->lpVtbl->Release(imalloc);
#else
	imalloc->Release();
#endif

	endModal(saveFore);

	FskMemPtrDispose(initialPathW);
	FskMemPtrDispose(promptW);

	return err;
}

static BOOL CALLBACK enableDisableWindow(HWND hwnd, LPARAM lParam);

BOOL CALLBACK enableDisableWindow(HWND hwnd, LPARAM lParam)
{
	HWND *saveFore = (HWND *)lParam;
	char className[128];
	DWORD windowProcess;

	if (0 == RealGetWindowClass(hwnd, className, sizeof(className)))
		goto bail;

	if (0 != FskStrCompare(kFskWindowClassName, className))
		goto bail;

	GetWindowThreadProcessId(hwnd, &windowProcess);
	if (GetCurrentProcessId() != windowProcess)
		goto bail;

	if (saveFore) {
		if (NULL == *saveFore)
			*saveFore = hwnd;
		EnableWindow(hwnd, false);
	}
	else
		EnableWindow(hwnd, true);

bail:
	return true;
}

HWND beginModal(void)
{
	HWND saveFore = NULL;

	EnumWindows(enableDisableWindow, (LPARAM)&saveFore);

	return saveFore;
}

void endModal(HWND saveFore)
{
	EnumWindows(enableDisableWindow, (LPARAM)NULL);

	if (saveFore)
		SetForegroundWindow(saveFore);
}

static VOID CALLBACK CommonDialogTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	DWORD foregroundThreadID;
	DWORD ourThreadID;
	HWND hWndFore;
	char szClass[64];

	hWndFore = GetForegroundWindow();
	GetClassName(hWndFore, szClass, sizeof(szClass));
	if ((hWndFore != hWnd) && (0 == FskStrCompare(kFskWindowClassName, szClass))) {
		foregroundThreadID = GetWindowThreadProcessId(hWndFore, NULL);
		ourThreadID = GetCurrentThreadId();

		// Bring our window to the foreground
		if ((foregroundThreadID == ourThreadID) && (hWndFore != hWnd))
			SetForegroundWindow(hWnd);
	}
}

static LRESULT CALLBACK CommonDialogHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(ghHook, nCode, wParam, lParam);

	if (HCBT_ACTIVATE == nCode) {
		RECT rcScreen, rcWindow;
		HWND hWnd = (HWND)wParam;
		LPCBTACTIVATESTRUCT lpCBT = (LPCBTACTIVATESTRUCT)lParam;

		// Just for safety...
		if (FALSE == lpCBT->fMouse) {

			// Center the dialog
			GetWindowRect(hWnd, &rcWindow);
			GetWindowRect(GetDesktopWindow(), &rcScreen);
			{
			HWND hWndFore = GetForegroundWindow();
			if (hWndFore) {
				HMONITOR monitor = MonitorFromWindow(hWndFore, MONITOR_DEFAULTTOPRIMARY);
				MONITORINFO mi;

				FskMemSet(&mi, 0, sizeof(MONITORINFO));
				mi.cbSize = sizeof(MONITORINFO);
				if (GetMonitorInfo(monitor, &mi))
					rcScreen = mi.rcMonitor;
			}
			}

			MoveWindow(hWnd, (rcScreen.right + rcScreen.left) / 2 - (rcWindow.right - rcWindow.left) / 2,
				(rcScreen.bottom + rcScreen.top) / 2 - (rcWindow.bottom - rcWindow.top) / 2,
				rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, true);

			// Use a timer callback function to periodically check and make
			// sure the dialog stays on top.
			SetTimer(hWnd, 0, 100, CommonDialogTimerProc);
			UnhookWindowsHookEx(ghHook);
			return 0;
		}
	}

	return CallNextHookEx(ghHook, nCode, wParam, lParam);
}

static int CALLBACK BrowseFolderCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch (uMsg)
	{
		case BFFM_INITIALIZED:
			if (lpData) {
				SendMessage(hwnd, BFFM_SETSELECTIONW, 1, lpData);
			}
			break;
	}

	return 0;
}

static DWORD WINAPI directoryChangeNotifierThread(void *refcon);
static void doChangesCallback(void *a0, void *a1, void *a2, void *a3);
static void CALLBACK changesCallback(DWORD error, DWORD bytes, LPOVERLAPPED overlap);

FskErr FskFSDirectoryChangeNotifierNew(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNtf)
{
	FskErr err;
	FskFSDirectoryChangeNotifier dirNot = NULL;
	WCHAR *winPath = NULL;

	*dirChangeNtf = NULL;

	err = FskMemPtrNewClear(sizeof(FskFSDirectoryChangeNotifierRecord), (FskMemPtr *)&dirNot);
	BAIL_IF_ERR(err);

	dirNot->base.dispatch.dispatch = &gFSDispatch;
	dirNot->flags = flags;
	dirNot->callback = callback;
	dirNot->refCon = refCon;
	dirNot->thread = FskThreadGetCurrent();
	dirNot->count = 1;
	dirNot->hFile = INVALID_HANDLE_VALUE;

	err = fixUpPathForWindows(path, &winPath, kFskPathIsDirectory);
	BAIL_IF_ERR(err);

	dirNot->hFile = CreateFileW(winPath, FILE_LIST_DIRECTORY, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
									(LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING,
									FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
	if (INVALID_HANDLE_VALUE == dirNot->hFile) {
		BAIL(kFskErrOperationFailed);
	}

	changesCallback(0, 0, &dirNot->overlapped);

bail:
	FskMemPtrDispose(winPath);

	if (kFskErrNone != err) {
		FskFSDirectoryChangeNotifierDispose((FskDirectoryChangeNotifier)dirNot);
		dirNot = NULL;
	}
	*dirChangeNtf = (FskDirectoryChangeNotifier)dirNot;

	return err;
}

FskErr FskFSDirectoryChangeNotifierDispose(FskDirectoryChangeNotifier dirChangeNtf)
{
	FskFSDirectoryChangeNotifier dirNot = (FskFSDirectoryChangeNotifier)dirChangeNtf;

	if (NULL == dirNot)
		return kFskErrNone;

	dirNot->callback = NULL;

	dirNot->count -= 1;
	if (dirNot->count > 0)
		return kFskErrNone;

	if (INVALID_HANDLE_VALUE != dirNot->hFile) {
		//@@ at shutdown, the callback won't be invoked so dirNotw isn't disposed.
		CloseHandle(dirNot->hFile);
		dirNot->hFile = INVALID_HANDLE_VALUE;
		// dirNot will be disposed in changesCallback when it receives the cancel notification
	}
	else
		FskMemPtrDispose(dirNot);

	return kFskErrNone;
}

void CALLBACK changesCallback(DWORD error, DWORD bytes, LPOVERLAPPED overlap)
{
	FskFSDirectoryChangeNotifier dirNot = (FskFSDirectoryChangeNotifier)(((unsigned char *)overlap) - offsetof(FskFSDirectoryChangeNotifierRecord, overlapped));
	FskErr err = kFskErrNone;

	if (INVALID_HANDLE_VALUE == dirNot->hFile) {
		// notifier has been disposed
		FskMemPtrDispose(dirNot);
		return;
	}

	if ((0 == error) && (0 != bytes)) {
		FskMemPtr buffer;

		if (kFskErrNone == FskMemPtrNewFromData(bytes, dirNot->buffer, &buffer)) {
			dirNot->count += 1;
			FskThreadPostCallback(dirNot->thread, doChangesCallback, dirNot, buffer, (void *)bytes, NULL);
		}
	}

	if (0 == ReadDirectoryChangesW(dirNot->hFile, dirNot->buffer, sizeof(dirNot->buffer), 0 != (kFskDirectoryChangeMonitorSubTree & dirNot->flags),
					FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
					NULL, &dirNot->overlapped, changesCallback)) {
		err = winErrorToFskErr(GetLastError());
	}
}

void doChangesCallback(void *a0, void *a1, void *a2, void *a3)
{
	FskFSDirectoryChangeNotifier dirNot = a0;
	FILE_NOTIFY_INFORMATION *info = a1;
	UInt32 bytes = (UInt32)a2;
	
	if (0 == bytes) {
		(dirNot->callback)(kFskDirectoryChangeFileUnknown, NULL, dirNot->refCon);
		goto done;
	}

	while (bytes && dirNot->callback) {
		char *path;

		if ((0 != info->FileNameLength) && (kFskErrNone == FskTextUnicode16LEToUTF8(info->FileName, info->FileNameLength, &path, NULL))) {
			UInt32 flags = kFskDirectoryChangeFileUnknown;
			char *toReplace;

			for (toReplace = strchr(path, '\\'); NULL != toReplace; toReplace = strchr(toReplace, '\\'))
				*toReplace = '/';

			switch (info->Action) {
				case FILE_ACTION_ADDED:
				case FILE_ACTION_RENAMED_NEW_NAME:
					flags = kFskDirectoryChangeFileCreated;
					break;

				case FILE_ACTION_REMOVED:
				case FILE_ACTION_RENAMED_OLD_NAME:
					flags = kFskDirectoryChangeFileDeleted;
					break;

				case FILE_ACTION_MODIFIED:
					flags = kFskDirectoryChangeFileChanged;
					break;
			}

#if SUPPORT_INSTRUMENTATION
			if (FskInstrumentedItemHasListeners(&dirNot->base)) {
				FskDirectoryChangeInstrMsgChangeRecord msg = {flags, path};
				FskInstrumentedItemSendMessageVerbose(&dirNot->base, kFskDirectoryChangeInstrMsgChanged, &msg);
			}
#endif

			(dirNot->callback)(flags, path, dirNot->refCon);
			FskMemPtrDispose(path);
		}

		if (0 == info->NextEntryOffset)
			break;

		// remove any duplicate entries for this file that follow in the buffer. on windows mobile, in particular, we can get lots of messages about a single file
		{
		UInt32 b = bytes - info->NextEntryOffset;
		FILE_NOTIFY_INFORMATION *i = (FILE_NOTIFY_INFORMATION *)(info->NextEntryOffset + (unsigned char *)info);
		while (b) {
			if ((info->Action == i->Action) &&
				(info->FileNameLength == i->FileNameLength) &&
				(0 == memcmp(info->FileName, i->FileName, i->FileNameLength)))
				i->FileNameLength = 0;

			if (0 == i->NextEntryOffset)
				break;

			b -= i->NextEntryOffset;
			i = (FILE_NOTIFY_INFORMATION *)(i->NextEntryOffset + (unsigned char *)i);
		}

		}

		bytes -= info->NextEntryOffset;
		info = (FILE_NOTIFY_INFORMATION *)(info->NextEntryOffset + (unsigned char *)info);
	}

done:
	FskMemPtrDispose(a1);

	dirNot->count -= 1;
	if (dirNot->count <= 0)
		FskFSDirectoryChangeNotifierDispose((FskDirectoryChangeNotifier)dirNot);
}

FskErr FskWinCreateFile(LPCTSTR fullPath, DWORD accessMode, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, HANDLE *handle)
{
	FskErr err;
	BSTR name = NULL;

	// Try straightforward CreateFile() first.
	*handle = CreateFile(fullPath, GENERIC_READ | GENERIC_WRITE, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	if (INVALID_HANDLE_VALUE != *handle)
		return kFskErrNone;

	err = kFskErrOperationFailed;

	if (0 == (accessMode & GENERIC_WRITE))
		return err;

	return err;
}
