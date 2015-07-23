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
#define __FSKZIP_PRIV__
#include "FskZip.h"

#include "FskEndian.h"
#include "FskExtensions.h"
#include "FskECMAScript.h"
#include "FskTextConvert.h"

#if !FSK_EXTENSION_EMBED
	#include "fsZip.xs.h"
	FskExport(xsGrammar *) fsZip_fskGrammar = &fsZipGrammar;
#endif

#if TARGET_ALIGN_PACKPUSH
    #pragma pack(push, 2)
#elif TARGET_ALIGN_PACK
    #pragma pack(2)
#endif

typedef struct {
	UInt32		signature;
	UInt16		versionMade;
	UInt16		versionNeeded;
	UInt16		flags;
	UInt16		compressionMethod;
	UInt16		modTime;
	UInt16		modDate;
	UInt32		crc32;
	UInt32		compressedSize;
	UInt32		uncompressedSize;
	UInt16		fileNameLen;
	UInt16		extraFieldLen;
	UInt16		commentLen;
	UInt16		diskNumber;
	UInt16		internalFileAttributes;
	UInt32		externalFileAttributes;
	UInt32		relativeOffset;
} FSK_PACK_STRUCT FskZipCentralDirectoryFileRecord, *FskZipCentralDirectoryFile;		// 46 bytes

typedef struct {
	UInt32		signature;
	UInt16		numberOfThisDisk;
	UInt16		numberOfDiskWithCentralDir;
	UInt16		centralDirectoryCountOnThisDisk;
	UInt16		centralDirectoryCount;
	UInt32		centralDirectorySize;
	UInt32		centralDirectoryOffset;
	UInt16		commentLength;
} FSK_PACK_STRUCT FskZipEndOfCentralDirectoryRecord, *FskZipEndOfCentralDirectory;

typedef struct {
	UInt32		signature;
	UInt16		versionNeeded;
	UInt16		flags;
	UInt16		compressionMethod;
	UInt16		modTime;
	UInt16		modDate;
	UInt32		crc32;
	UInt32		compressedSize;
	UInt32		uncompressedSize;
	UInt16		fileNameLen;
	UInt16		extraFieldLen;
} FSK_PACK_STRUCT FskZipLocalFileHeaderRecord, *FskZipLocalFileHeader;

typedef struct {
	UInt32		signature;
	FskInt64	sizeOfZip64EndOfCentralDir;
	UInt16		versionMadeBy;
	UInt16		versionNeededToExtract;
	UInt32		numberOfThisDisk;
	UInt32		numberOfDiskWithCentralDir;
	FskInt64	centralDirectoryCountOnThisDisk;
	FskInt64	centralDirectoryCount;
	FskInt64	centralDirectorySize;
	FskInt64	centralDirectoryOffset;
} FSK_PACK_STRUCT FskZip64EndOfCentralDirectoryRecord, *FskZip64EndOfCentralDirectory;

typedef struct {
	UInt32		signature;
	UInt32		numberOfDiskWithZip64EndOfCentralDir;
	FskInt64	zip64EndOfcentralDirectoryOffset;
	UInt32		totalNumberOfDisks;
} FSK_PACK_STRUCT FskZip64EndOfCentralDirectoryLocatorRecord, *FskZip64EndOfCentralDirectoryLocator;

typedef struct {
	UInt16		blockType;
	UInt16		blockSize;
	FskInt64	uncompressedSize;
	FskInt64	compressedSize;
	FskInt64	relativeOffset;
	UInt32		diskNumber;
} FSK_PACK_STRUCT FskZip64ExtendedInformationExtraFieldRecord, *FskZip64ExtendedInformationExtraField;

#if TARGET_ALIGN_PACKPUSH
    #pragma pack(pop)
#elif TARGET_ALIGN_PACK
    #pragma pack()
#endif

static unsigned char *findFileInZip(FskZip zip, const char *path);
static int zipCentralDirSort(const void *a, const void *b);
static void *zipAlloc(void *state, uInt items, uInt size);
static void zipFree(void *state, void *mem);
static void splitZipPath(const char *fullPath, char **dotZipOut, char **endPathOut);
static FskErr sortZipDirectory(FskZip zip);
static void zipUpdateCRC(FskZipFile file);
static FskZipCentralDirectoryFile zipGetEndOfFiles(FskZip zip, FskInt64 *offset);
static FskErr zipFlush(FskZip zip);
static FskErr addPathToZip(FskZip zip, const char *path);
static FskErr zipWriteFileHeader(FskZip zip, FskZipCentralDirectoryFile entry);

#define kSplitSize 1073741824	// 1GB
//#define kSplitSize (1024 + (8192 * 16))

static FskErr spannedZipCreateNewFile(FskZip zip);
static FskErr spannedZipFileSetPosition(FskZip zip, const FskInt64 *position);
static FskErr spannedZipFileWrite(FskZip zip, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten);
static FskErr spannedZipFileRead(FskZip zip, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead);
static char *spannedZipCreateFileNameFromNumber(const char *path, UInt32 number);
static FskErr spannedZipGetSplitSize(const char *path, FskInt64 *sizeOut);

static FskErr zip64ReadEndOfCentralDirectory(FskZip zip, FskInt64 locatorPos, FskZip64EndOfCentralDirectory ecd, FskZip64EndOfCentralDirectoryLocator ecdl);
static FskErr zip64GetCentralDirectoryData(UInt8 *entry, FskInt64 *compressSizeOut, FskInt64 *uncompressSizeOut, UInt32 *diskNumberOut, FskInt64 *relativeOffsetOut);
static UInt8* findStartLocationInExtraField(UInt8 *entry, UInt16 headerID);
static FskErr zip64SetCentralDirectoryData(FskZip zip, UInt8 **entryIn, FskInt64* compressedSizeIn, FskInt64* uncompressedSizeIn, UInt32* diskNumberIn, FskInt64* relativeOffsetIn);

static FskListMutex gZipList;	// global list of all open zip files to allow sharing of central directory cache

static const UInt32 crcTable[256] = {
  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
  0x2d02ef8dL
};

FskErr FskZipCreate(const char *path, const Boolean bSplit)
{
	FskErr err;
	FskFile fref = NULL;
	static const char header_split[] = {'P', 'K', 7, 8};
	static const char header[] = {'P', 'K', 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	FskInt64 size = 0;

	err = FskFileCreate(path);
	BAIL_IF_ERR(err);

	err = FskFileOpen(path, kFskFilePermissionReadWrite, &fref);
	BAIL_IF_ERR(err);

	err = FskFileSetSize(fref, &size);
	BAIL_IF_ERR(err);

	if (bSplit) {
		err = FskFileWrite(fref, sizeof(header_split), header_split, NULL);
		BAIL_IF_ERR(err);
	}

	err = FskFileWrite(fref, sizeof(header), header, NULL);
	BAIL_IF_ERR(err);

bail:
	FskFileClose(fref);

	return err;
}


FskErr FskZipOpen(FskZip *zipOut, const char *path, UInt32 permissions)
{
	FskErr err;
	FskZip zip = NULL;
	unsigned char buffer32[4];
	FskMemPtr scanBuffer = NULL;
	UInt8 *scanPtr;
	FskInt64 fileSize, position;
	UInt32 scanBufferSize;
	const UInt32 kZipScanSize = 1024;
	FskThread thread = FskThreadGetCurrent();
	UInt32 numberOfDiskWithCentralDir;
	FskZip64EndOfCentralDirectoryRecord zip64ECD;
	Boolean bFoundZip64EndOfCentralDirectory = false;

	if (NULL == gZipList) {
		err = FskListMutexNew(&gZipList, "gZipList");
		BAIL_IF_ERR(err);
	}

	// see if this zip file is already open
	while (true) {
		zip = FskListMutexGetNext(gZipList, zip);
		if (NULL == zip)
			break;

		if ((0 == FskStrCompare(path, zip->path)) && ((kFskFilePermissionReadOnly == permissions) ||
			((kFskFilePermissionReadWrite == permissions) && (zip->thread == thread)))) {
			if ((kFskFilePermissionReadWrite == permissions) && !zip->openForWrite) {
				// need to get write access
				FskFileClose(zip->fref);
				zip->fref = NULL;

				err = FskFileOpen(zip->path, kFskFilePermissionReadWrite, &zip->fref);
				if (err) {
					FskFileOpen(zip->path, kFskFilePermissionReadOnly, &zip->fref);
					return err;
				}
				zip->openForWrite = true;
			}
			zip->useCount += 1;
			*zipOut = zip;
			return kFskErrNone;
		}
	}

	// make a new one
	err = FskMemPtrNewClear(sizeof(FskZipRecord), (FskMemPtr *)&zip);
	BAIL_IF_ERR(err);

	zip->useCount = 1;
	zip->thread = thread;
	FskListMutexPrepend(gZipList, zip);

	zip->path = FskStrDoCopy(path);

	err = FskFileOpen(path, permissions, &zip->fref);
	BAIL_IF_ERR(err);

	zip->openForWrite = (kFskFilePermissionReadWrite == permissions);

	// try to find the end of central dir record (P, K, 0x05, 0x06 is the signature)
	// we only look at the last 1024 bytes - this won't work if there is a big comment
	FskFileGetSize(zip->fref, &fileSize);
	if (fileSize < kZipScanSize)
		scanBufferSize = (UInt32)fileSize;
	else
		scanBufferSize = kZipScanSize;
	position = fileSize - scanBufferSize;
	FskFileSetPosition(zip->fref, &position);

	err = FskMemPtrNew(scanBufferSize, &scanBuffer);
	BAIL_IF_ERR(err);

	err = FskFileRead(zip->fref, scanBufferSize, scanBuffer, NULL);
	BAIL_IF_ERR(err);

	scanPtr = (UInt8 *)scanBuffer + scanBufferSize - 4;
	while (scanBufferSize >= 4) {
		if (('P' == scanPtr[0]) && ('K' == scanPtr[1]) && (5 == scanPtr[2]) && (6 == scanPtr[3]))
			break;
		scanBufferSize -= 1;
		scanPtr -= 1;
	}

	if (scanBufferSize < 4)
        BAIL(kFskErrBadData);

	// find the zip64 end of central dir record (P, K, 0x06, 0x06 is the signature)
	if (scanBufferSize >= 24) {
		position += (scanBufferSize - 24);	// 24 = (end of central dir signature size(4)) + (Zip64 end of central directory locator size(20))
		bFoundZip64EndOfCentralDirectory = (kFskErrNone == zip64ReadEndOfCentralDirectory(zip, position, &zip64ECD, NULL));
	}

	// load the central directory
	scanPtr += 4;
	if (bFoundZip64EndOfCentralDirectory && 0xff == scanPtr[0] && 0xff == scanPtr[1])
		zip->numberOfThisDisk = zip64ECD.numberOfThisDisk;
	else
		zip->numberOfThisDisk = FskEndianU16_LtoN(FskMisaligned16_GetN(scanPtr));

	scanPtr += 2;
	if (bFoundZip64EndOfCentralDirectory && 0xff == scanPtr[0] && 0xff == scanPtr[1])
		numberOfDiskWithCentralDir = zip64ECD.numberOfDiskWithCentralDir;
	else
		numberOfDiskWithCentralDir = FskEndianU16_LtoN(FskMisaligned16_GetN(scanPtr));

	scanPtr += 4;
	if (bFoundZip64EndOfCentralDirectory && 0xff == scanPtr[0] && 0xff == scanPtr[1])
		zip->centralDirCount = zip64ECD.centralDirectoryCount;
	else
		zip->centralDirCount = FskEndianU16_LtoN(FskMisaligned16_GetN(scanPtr));

	scanPtr += 2;
	if (bFoundZip64EndOfCentralDirectory && 0xff == scanPtr[0] && 0xff == scanPtr[1] && 0xff == scanPtr[2] && 0xff == scanPtr[3])
		zip->centralDirSize = zip64ECD.centralDirectorySize;
	else
		zip->centralDirSize = FskEndianU32_LtoN(FskMisaligned32_GetN(scanPtr));

	// read Central Directory
	if (0 != zip->centralDirSize) {
		scanPtr += 4;
		if (bFoundZip64EndOfCentralDirectory && 0xff == scanPtr[0] && 0xff == scanPtr[1] && 0xff == scanPtr[2] && 0xff == scanPtr[3])
			position = zip64ECD.centralDirectoryOffset;
		else
			position = FskEndianU32_LtoN(FskMisaligned32_GetN(scanPtr));

		// If the size and number of Central Directory is over 4GB, we need to change the algorism
		// of reading central directory. (But It's probably a rare case.)
		if (zip->centralDirSize > 0xffffffff || zip->centralDirCount > 0xffffffff)
			BAIL(kFskErrMemFull);
		err = FskMemPtrNew((UInt32)(zip->centralDirSize), (FskMemPtr *)&zip->centralDir);
		BAIL_IF_ERR(err);

		if (numberOfDiskWithCentralDir != zip->numberOfThisDisk) {
			char *otherFilePath;
			FskFile otherFileRef;

			otherFilePath = spannedZipCreateFileNameFromNumber(path, numberOfDiskWithCentralDir + 1);
			if (NULL == otherFilePath)
				BAIL(kFskErrMemFull);
			err = FskFileOpen(otherFilePath, kFskFilePermissionReadOnly, &otherFileRef);
            BAIL_IF_ERR(err);
			FskFileSetPosition(otherFileRef, &position);
			err = FskFileRead(otherFileRef, (UInt32)zip->centralDirSize, zip->centralDir, NULL);
			BAIL_IF_ERR(err);
			FskFileClose(otherFileRef);
			FskMemPtrDispose(otherFilePath);
		}
		else {
			FskFileSetPosition(zip->fref, &position);
			err = FskFileRead(zip->fref, (UInt32)zip->centralDirSize, zip->centralDir, NULL);
			BAIL_IF_ERR(err);
		}

		scanPtr = zip->centralDir;
		if (('P' != scanPtr[0]) || ('K' != scanPtr[1]) || (1 != scanPtr[2]) || (2 != scanPtr[3]))
            BAIL(kFskErrBadData);

		err = sortZipDirectory(zip);
		BAIL_IF_ERR(err);
	}

	// is this Zip file the spanned/split archive?
	if (0 != zip->numberOfThisDisk) {
		FskInt64 size;
		if (kFskErrNone == spannedZipGetSplitSize(path, &size))
			zip->splitSize = (UInt32)size;
		else
			zip->splitSize = kSplitSize;
	}
	else {
		position = 0;
		FskFileSetPosition(zip->fref, &position);
		err = FskFileRead(zip->fref, 4, buffer32, NULL);
		BAIL_IF_ERR(err);

		if ((('P' == buffer32[0]) && ('K' == buffer32[1]) && (7 == buffer32[2]) && (8 == buffer32[3])) ||
			(('P' == buffer32[0]) && ('K' == buffer32[1]) && (3 == buffer32[2]) && (3 == buffer32[3]))) {
			FskInt64 size = 0;
			FskFileGetSize(zip->fref, &size);
			zip->splitSize = (size > kSplitSize) ? size : kSplitSize;
		}
	}

	// offset of this file from top of total files
	zip->offsetOfCurrentFile = zip->numberOfThisDisk * zip->splitSize;

bail:
	FskMemPtrDispose(scanBuffer);

	if (err) {
		FskZipClose(zip);
		zip = NULL;
	}
	*zipOut = zip;

	return err;
}

void FskZipClose(FskZip zip)
{
	if (NULL == zip)
		return;

	zip->useCount -= 1;
	if (0 != zip->useCount)
		return;

	zipFlush(zip);

	FskListMutexRemove(gZipList, zip);
	if (NULL == FskListMutexGetNext(gZipList, NULL)) {
		FskListMutexDispose(gZipList);
		gZipList = NULL;
	}

	FskFileClose(zip->fref);
	FskMemPtrDispose(zip->path);
	FskMemPtrDispose(zip->centralDir);
	FskMemPtrDispose(zip->centralDirIndex);
	FskMemPtrDispose(zip);
}

FskErr FskZipGetFileInfo(FskZip zip, const char *path, FskFileInfo *itemInfo)
{
	FskErr err = kFskErrNone;
	unsigned char *entry = findFileInZip(zip, path);
	UInt32 pathLen = FskStrLen(path);

	itemInfo->fileCreationDate = 0;
	itemInfo->fileModificationDate = 0;
	itemInfo->flags = 0;
	itemInfo->fileNode = 0;
#if TARGET_OS_ANDROID
	itemInfo->fileDevice = 0;
#endif

    BAIL_IF_NULL(entry, err, kFskErrFileNotFound);

	if ('/' == path[pathLen - 1]) {
		itemInfo->filetype = kFskDirectoryItemIsDirectory;
		itemInfo->filesize = 0;
	}
	else {
		itemInfo->filetype = kFskDirectoryItemIsFile;
		itemInfo->filesize = FskMisaligned32_GetN(entry + 24);
		itemInfo->filesize = FskEndianU32_LtoN(itemInfo->filesize);
	}

bail:
	return err;
}

unsigned char *findFileInZip(FskZip zip, const char *path)
{
	UInt32 i;
	UInt32 pathLen = FskStrLen(path);

	for (i=0; i<zip->centralDirCount; i++) {
		unsigned char *entry = zip->centralDirIndex[i];
		UInt32 nameLen = entry[28] + (entry[29] << 8);
		char *encodedText;
		UInt32 encodedTextLen;

		// ZIP files have no indication of character encoding, so we use ad-hoc rules based on the platform
#if TARGET_OS_WIN32
		FskTextToUTF8((char *)entry + 46, nameLen, &encodedText, &encodedTextLen);
#else
		FskTextLatin1ToUTF8((char *)entry + 46, nameLen, &encodedText, &encodedTextLen);
#endif
		if ((encodedTextLen == pathLen) && (0 == FskStrCompareWithLength(encodedText, path, pathLen))) {
			FskMemPtrDispose(encodedText);
			return entry;
		}
		FskMemPtrDispose(encodedText);
	}

	return NULL;
}

// ideally this should compare by path segment not the full string to give the expected result in all cases
// but that would be slower and more complicated.
int zipCentralDirSort(const void *a, const void *b)
{
	SInt32 result;
	const unsigned char *aa = *(const unsigned char **)a, *bb = *(const unsigned char **)b;
	UInt32 nameLenA = aa[28] + (aa[29] << 8);
	UInt32 nameLenB = bb[28] + (bb[29] << 8);
	UInt32 compareLen = nameLenA;
	if (nameLenA > nameLenB)
		compareLen = nameLenB;
	result = FskStrCompareCaseInsensitiveWithLength((char *)aa + 46, (char *)bb + 46, compareLen);
	if ((0 == result) && (nameLenA != nameLenB))
		result = (nameLenA < nameLenB) ? -1 : +1;
	return result;
}

FskErr FskZipFileOpen(FskZip zip, const char *path, FskZipFile *zipFileOut)
{
	FskErr err = kFskErrNone;
	FskZipFile zipFile;
	unsigned char *entry;
	unsigned char buffer[30];
	UInt32 entrySize;
	FskInt64 uncompressSize;
	FskInt64 compressSize;
	FskInt64 relativeOffset;
	UInt32 diskNumber;

	err = FskMemPtrNewClear(sizeof(FskZipFileRecord), (FskMemPtr *)&zipFile);
	BAIL_IF_ERR(err);

	zipFile->zip = zip;
	zip->useCount += 1;
	zipFile->entry = findFileInZip(zip, path);
    BAIL_IF_NULL(zipFile->entry, err, kFskErrFileNotFound);

	entry = zipFile->entry;
	zipFile->method = (entry[10] << 0) | (entry[11] << 8);
	if (8 == zipFile->method) {
		zipFile->zlib.zalloc = zipAlloc;
		zipFile->zlib.zfree = zipFree;
		zipFile->zlib.opaque = NULL;
		if (Z_OK != inflateInit2(&zipFile->zlib, -MAX_WBITS))
            BAIL(kFskErrMemFull);
		zipFile->initializedZlib = true;
	}
	else if (0 == zipFile->method)
		;	// no special preparation required
	else
        BAIL(kFskErrUnimplemented);

	err = zip64GetCentralDirectoryData(entry, &compressSize, &uncompressSize, &diskNumber, &relativeOffset);
	BAIL_IF_ERR(err);
	zipFile->compressedBytesRemaining = compressSize;
	zipFile->compressedFilePosition = relativeOffset + diskNumber * zip->splitSize;
	zipFile->fileSize = uncompressSize;

	spannedZipFileSetPosition(zipFile->zip, &zipFile->compressedFilePosition);
	err = FskFileRead(zipFile->zip->fref, 30, buffer, NULL);
	BAIL_IF_ERR(err);

	entrySize = 30 + (buffer[27] << 8) + buffer[26] + (buffer[29] << 8) + buffer[28];
	zipFile->compressedFilePosition += entrySize;
	zipFile->compressedFileStartingOffset = zipFile->compressedFilePosition;
	zipFile->compressedFileSize = zipFile->compressedBytesRemaining;

bail:
	if (kFskErrNone != err) {
		FskZipFileClose(zipFile);
		zipFile = NULL;
	}
	*zipFileOut = zipFile;

	return err;
}

FskErr FskZipFileClose(FskZipFile zipFile)
{
	FskZip zip;

	if (NULL == zipFile)
		return kFskErrNone;

	zip = zipFile->zip;

	if (zipFile->initializedZlib)
		inflateEnd(&zipFile->zlib);

	if (zip->writeFile == zipFile) {
		zipUpdateCRC(zipFile);
		zipWriteFileHeader(zip, (FskZipCentralDirectoryFile)zip->writeFile->entry);
		zip->writeFile = NULL;
		zip->needToFlush = true;
		zipFlush(zip);
	}

	FskZipClose(zip);

	FskMemPtrDispose(zipFile);

	return kFskErrNone;
}

FskErr FskZipFileGetSize(FskZipFile zipFile, FskInt64 *size)
{
	*size = zipFile->fileSize;
	return kFskErrNone;
}

FskErr FskZipFileRead(FskZipFile zipFile, UInt32 bytesToRead, void *buffer, UInt32 *bytesReadOut)
{
	FskErr err = kFskErrNone;
	UInt32 bytesRead = 0;

	if (0 == zipFile->method) {
		FskInt64 fileSize, position;

        BAIL_IF_TRUE(zipFile->eof, err, kFskErrEndOfFile);

		FskZipFileGetSize(zipFile, &fileSize);

		position = zipFile->compressedFileStartingOffset + zipFile->position;
		err = spannedZipFileSetPosition(zipFile->zip, &position);
		BAIL_IF_ERR(err);

		if (zipFile->position + bytesToRead > zipFile->fileSize) {
            BAIL_IF_NULL(bytesReadOut, err, kFskErrOperationFailed);

			if (zipFile->position >= zipFile->fileSize)
                BAIL(kFskErrEndOfFile);
			bytesToRead = (UInt32)(zipFile->fileSize - zipFile->position);
		}

		err = spannedZipFileRead(zipFile->zip, bytesToRead, buffer, &bytesRead);
		BAIL_IF_ERR(err);

		zipFile->position += bytesRead;
		if (zipFile->position == zipFile->fileSize)
			zipFile->eof = true;

		if (bytesReadOut)
			*bytesReadOut = bytesRead;

		return kFskErrNone;
	}

	while (bytesToRead && (false == zipFile->eof)) {
		int result;
		int originalTotalIn = zipFile->zlib.total_in, originalTotalOut = zipFile->zlib.total_out;
		int bytesConsumed, bytesInflated;

		if ((0 == zipFile->bytesInBuffer) && (0 != zipFile->compressedBytesRemaining)) {
			FskInt64 compressedBytesToRead = kFskZipFileBufferSize;
			UInt32 compressedBytesRead;
			if (compressedBytesToRead > zipFile->compressedBytesRemaining)
				compressedBytesToRead = zipFile->compressedBytesRemaining;
			err = spannedZipFileSetPosition(zipFile->zip, &zipFile->compressedFilePosition);
			BAIL_IF_ERR(err);

			err = spannedZipFileRead(zipFile->zip, (UInt32)compressedBytesToRead, zipFile->buffer, &compressedBytesRead);
			BAIL_IF_ERR(err);

			zipFile->compressedFilePosition += compressedBytesRead;
			zipFile->compressedBytesRemaining -= compressedBytesRead;
			zipFile->bytesInBuffer = compressedBytesRead;
			zipFile->bufferPtr = zipFile->buffer;
		}

		zipFile->zlib.next_in	= zipFile->bufferPtr;
		zipFile->zlib.avail_in	= zipFile->bytesInBuffer;
		zipFile->zlib.next_out	= buffer;
		zipFile->zlib.avail_out	= bytesToRead;
		result = inflate(&zipFile->zlib, Z_SYNC_FLUSH);
		if (Z_STREAM_END == result) {
			zipFile->eof = true;
			zipFile->bytesInBuffer = 0;
		}
		else if (Z_OK != result)
            BAIL(kFskErrBadData);

		bytesInflated = (zipFile->zlib.total_out - originalTotalOut);
		bytesConsumed = (zipFile->zlib.total_in - originalTotalIn);
		bytesToRead -= bytesInflated;
		bytesRead += bytesInflated;
		buffer = bytesInflated + (char *)buffer;
		zipFile->bytesInBuffer -= bytesConsumed;
		zipFile->bufferPtr += bytesConsumed;
	}

bail:
	zipFile->position += bytesRead;

	if (kFskErrNone != err)
		;
	else if (bytesReadOut) {
		*bytesReadOut = bytesRead;
		if ((0 != bytesToRead) && (0 == bytesRead))
			err = kFskErrEndOfFile;
	}
	else if (0 != bytesToRead)
		err = kFskErrOperationFailed;

	return err;
}

FskErr FskZipFileSetPosition(FskZipFile zipFile, const FskInt64 *position)
{
	FskErr err;
	UInt32 bytesToSkip;
	FskMemPtr skip = NULL;
	const int kSkipBufferSize = 1024;

	if ((zipFile != zipFile->zip->writeFile) && (*position >= zipFile->fileSize)) {
		zipFile->position = zipFile->fileSize;
		zipFile->eof = true;
		return kFskErrNone;
	}

	if (0 == zipFile->method) {
		zipFile->position = *position;
		zipFile->eof = false;
		return kFskErrNone;
	}

	if (*position >= zipFile->position)
		bytesToSkip = (UInt32)(*position - zipFile->position);
	else {
		zipFile->compressedFilePosition = zipFile->compressedFileStartingOffset;
		zipFile->compressedBytesRemaining = zipFile->compressedFileSize;
		zipFile->bytesInBuffer = 0;
		bytesToSkip = (UInt32)*position;
		zipFile->position = 0;
		zipFile->eof = false;
		inflateReset(&zipFile->zlib);
	}

	if (0 == bytesToSkip)
		return kFskErrNone;

	err = FskMemPtrNew(kSkipBufferSize, &skip);
	BAIL_IF_ERR(err);

	while (bytesToSkip) {
		UInt32 thisReadSize = kSkipBufferSize;
		if (thisReadSize > bytesToSkip)
			thisReadSize = bytesToSkip;
		err = FskZipFileRead(zipFile, bytesToSkip, skip, NULL);
		BAIL_IF_ERR(err);

		bytesToSkip -= thisReadSize;
	}

bail:
	FskMemPtrDispose(skip);
	return err;
}

FskErr FskZipFileGetPosition(FskZipFile zipFile, FskInt64 *position)
{
	*position = zipFile->position;
	return kFskErrNone;
}

// alloc / free bindings for zlib
void *zipAlloc(void *state, uInt items, uInt size)
{
	return FskMemPtrAlloc(items * size);
}

void zipFree(void *state, void *mem)
{
	FskMemPtrDispose(mem);
}

FskErr FskZipDirectoryIteratorNew(const char *directoryPath, FskZipDirectoryIterator *dirItOut, FskZip zip)
{
	FskErr err;
	UInt32 initialIndex = 0;
	UInt32 directoryPathLen = FskStrLen(directoryPath);
	FskZipDirectoryIterator dirIt = NULL;

	if (0 != *directoryPath) {
		// ensure that this path is valid - and find it's starting index while we're at it
        Boolean haveInitialIndex = false;
		UInt32 i;

		if ('/' != directoryPath[directoryPathLen - 1])
			return kFskErrInvalidParameter;

		for (i=0; i<zip->centralDirCount; i++) {
			unsigned char *entry = zip->centralDirIndex[i];
			UInt32 nameLen = entry[28] + (entry[29] << 8);
			if (nameLen >= directoryPathLen) {
				if (0 == FskStrCompareWithLength(directoryPath, (char *)entry + 46, directoryPathLen)) {
					initialIndex = i;
                    haveInitialIndex = true;
					break;
				}
			}
		}

		if (false == haveInitialIndex) {
			char *temp = FskStrDoCopy(directoryPath);
			if (NULL == temp)
				return kFskErrMemFull;

			temp[directoryPathLen - 1] = 0;
			err = findFileInZip(zip, temp) ? kFskErrNotDirectory : kFskErrFileNotFound;
			FskMemPtrDispose(temp);
			return err;
		}
	}

	err = FskMemPtrNewClear(sizeof(FskZipDirectoryIteratorRecord), (FskMemPtr *)&dirIt);
	BAIL_IF_ERR(err);

	dirIt->zip = zip;
	zip->useCount += 1;
	dirIt->directoryIndex = initialIndex;
	dirIt->path = FskStrDoCopy(directoryPath);
	dirIt->pathLen = directoryPathLen;

bail:
	if (kFskErrNone != err) {
		FskZipDirectoryIteratorDispose(dirIt);
		dirIt = NULL;
	}
	*dirItOut = dirIt;

	return err;
}

FskErr FskZipDirectoryIteratorDispose(FskZipDirectoryIterator dirIt)
{
	if (NULL == dirIt)
		return kFskErrNone;

	FskZipClose(dirIt->zip);
	FskMemPtrDispose(dirIt->path);
	FskMemPtrDispose(dirIt);

	return kFskErrNone;
}

FskErr FskZipDirectoryIteratorGetNext(FskZipDirectoryIterator dirIt, char **nameOut, UInt32 *itemType)
{
	FskErr err = kFskErrNone;
	unsigned char *entry;
	UInt32 nameLen, nameOutLen;
	char *np, *p = NULL;
	char *name;
	Boolean bDirectoryPath;

	if (dirIt->eof)
		return kFskErrIteratorComplete;

	entry = dirIt->zip->centralDirIndex[dirIt->directoryIndex];
	nameLen = entry[28] + (entry[29] << 8);
	np = (char *)entry + 46 + dirIt->pathLen;

	bDirectoryPath = (nameLen == dirIt->pathLen);

	if (!bDirectoryPath) {
		// return this entry
		p = FskStrNChr(np, nameLen - dirIt->pathLen, '/');
		if (NULL != p) {
			if (itemType)
				*itemType = kFskDirectoryItemIsDirectory;
			err = FskMemPtrNewClear(p - np + 1, (FskMemPtr *)&name);
			BAIL_IF_ERR(err);
			FskStrNCopy(name, np, p - np);
		}
		else {
			if (itemType)
				*itemType = kFskDirectoryItemIsFile;
			err = FskMemPtrNewClear(nameLen - dirIt->pathLen + 1, (FskMemPtr *)&name);
			BAIL_IF_ERR(err);
			FskStrNCopy(name, np, nameLen - dirIt->pathLen);
		}

		nameOutLen = FskStrLen(name);
	}

	// scan to the next
	do {
		dirIt->directoryIndex += 1;
		if (dirIt->directoryIndex >= dirIt->zip->centralDirCount)
			dirIt->eof = true;
		else {
			entry = dirIt->zip->centralDirIndex[dirIt->directoryIndex];
			nameLen = entry[28] + (entry[29] << 8);
			if (nameLen < dirIt->pathLen)
				dirIt->eof = true;
			else {
				if (0 != FskStrCompareWithLength((char *)entry + 46, dirIt->path, dirIt->pathLen))
					dirIt->eof = true;
				else if (NULL != p) {
					// scan past all entries in the directory we just returned
					if ((0 == FskStrCompareWithLength((char *)entry + 46 + dirIt->pathLen, name, nameOutLen)) &&
						('/' == *(entry + 46 + dirIt->pathLen + nameOutLen)))
						continue;
					break;
				}
			}
		}
	} while ((NULL != p) && (false == dirIt->eof));

	// if the path of this entry is equal to the directory path, call getNext again.
	if(bDirectoryPath)
		return FskZipDirectoryIteratorGetNext(dirIt, nameOut, itemType);

	if (nameOut)
		*nameOut = name;
	else
		FskMemPtrDispose(name);

bail:

	return err;
}

/*
	Interface with FskFiles.c
*/

static FskErr FskZipFileFilter(const char *fullPath, SInt32 *priority);
static FskErr fskZipFileOpen(const char *fullPath, UInt32 permissions, FskZipFile *frefOut);
static FskErr fskZipDirectoryIteratorNew(const char *directoryPath, FskZipDirectoryIterator *dirItOut, UInt32 flags);
static FskErr fskZipFileGetFileInfo(const char *fullpath, FskFileInfo *itemInfo);
static FskErr fskZipFileCreate(const char *fullpath);
static FskErr fskZipFileWrite(FskZipFile zipFile, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten);

FskFileDispatchTableRecord gZipDispatch = {
	0,
	FskZipFileFilter,

	(FskFileOpenProc)fskZipFileOpen,
	(FskFileCloseProc)FskZipFileClose,
	(FskFileGetSizeProc)FskZipFileGetSize,
	(FskFileSetSizeProc)NULL,
	(FskFileSetPositionProc)FskZipFileSetPosition,
	(FskFileGetPositionProc)FskZipFileGetPosition,
	(FskFileReadProc)FskZipFileRead,
	(FskFileWriteProc)fskZipFileWrite,
	(FskFileGetFileInfoProc)fskZipFileGetFileInfo,
	(FskFileSetFileInfoProc)NULL,
	(FskFileGetThumbnailProc)NULL,
	(FskFileCreateProc)fskZipFileCreate,
	NULL,		// delete
	NULL,		// rename
	NULL,		// pathToNative

	NULL,		// create dir
	NULL,		// delete dir
	NULL,		// rename dir
	(FskDirectoryIteratorNewProc)fskZipDirectoryIteratorNew,
	(FskDirectoryIteratorDisposeProc)FskZipDirectoryIteratorDispose,
	(FskDirectoryIteratorGetNextProc)FskZipDirectoryIteratorGetNext,
	(FskDirectoryGetSpecialPathProc)NULL,

	NULL,		// vol iterator new
	NULL,		// vol iterator dispose
	NULL,		// vol iterator getnext

	NULL,		// vol notifier new
	NULL,		// vol notifier dispose
	NULL,		// vol get info
	NULL,		// vol get info by path
	NULL,		// vol eject

	NULL,		// file map new
	NULL,		// file map dispose

	NULL,		// fileChoose
	NULL,		// fileChooseSave
	NULL,		// directoryChoose

	NULL		// fileTerminate
};

FskErr FskZipFileFilter(const char *fullPath, SInt32 *priority)
{
	FskErr err = kFskErrUnimplemented;
	const char *walker = fullPath;

	while (true) {
		char *dotZip, *endZip;
		splitZipPath(walker, &dotZip, &endZip);
		if (NULL == dotZip)
			break;

		*priority = dotZip - fullPath;
		err = kFskErrNone;
		walker  = endZip + 1;
	}

	return err;
}

FskErr fskZipFileOpen(const char *fullPath, UInt32 permissions, FskZipFile *frefOut)
{
	FskErr err;
	FskZip zip = NULL;
	char *path = FskStrDoCopy(fullPath);
	const char *dotZip = fullPath;
	char *endZip = NULL;
	FskZipFile fref = NULL;

	while (true) {
		char *p, *q;
		splitZipPath(dotZip + 1, &p, &q);
		if (NULL == p)
			break;
		dotZip = p;
		endZip = q;
	}

	if (NULL == endZip)
		return kFskErrInvalidParameter;

	*(path + (endZip - fullPath)) = 0;
	err = FskZipOpen(&zip, path, permissions);
	BAIL_IF_ERR(err);

	if ((kFskFilePermissionReadWrite == permissions) && (NULL != zip->writeFile))
        BAIL(kFskErrTooManyOpenFiles);

	err = FskZipFileOpen(zip, endZip + 1, &fref);
	BAIL_IF_ERR(err);

	if (kFskFilePermissionReadWrite == permissions) {
		FskInt64 maxOffset;
		FskZipCentralDirectoryFile lastFile = zipGetEndOfFiles(zip, &maxOffset);
		if ((lastFile != (FskZipCentralDirectoryFile)fref->entry) || (0 != fref->method))
			BAIL(kFskErrUnimplemented);		// we only implement writing to the last file in the set. method must be uncompressed.

		zip->writeFile = fref;
	}

bail:
	if (kFskErrNone != err) {
		FskZipFileClose(fref);
		fref = NULL;
	}
	*frefOut = fref;

	FskMemPtrDispose(path);
	FskZipClose(zip);

	return err;
}

FskErr fskZipDirectoryIteratorNew(const char *directoryPath, FskZipDirectoryIterator *dirItOut, UInt32 flags)
{
	FskErr err;
	FskZip zip = NULL;
	char *path = FskStrDoCopy(directoryPath);
	char *endZip;

	splitZipPath(directoryPath, NULL, &endZip);
	*(path + (endZip - directoryPath)) = 0;
	err = FskZipOpen(&zip, path, kFskFilePermissionReadOnly);
	BAIL_IF_ERR(err);

	err = FskZipDirectoryIteratorNew(endZip + 1, dirItOut, zip);
	BAIL_IF_ERR(err);

bail:
	FskMemPtrDispose(path);
	FskZipClose(zip);

	return err;
}

FskErr fskZipFileGetFileInfo(const char *fullPath, FskFileInfo *itemInfo)
{
	FskErr err;
	FskZip zip = NULL;
	char *path = FskStrDoCopy(fullPath);
	char *endZip;

	splitZipPath(fullPath, NULL, &endZip);
	*(path + (endZip - fullPath)) = 0;
	err = FskZipOpen(&zip, path, kFskFilePermissionReadOnly);
	BAIL_IF_ERR(err);

	err = FskZipGetFileInfo(zip, endZip + 1, itemInfo);
	BAIL_IF_ERR(err);

bail:
	FskMemPtrDispose(path);
	FskZipClose(zip);

	return err;
}

FskErr fskZipFileCreate(const char *fullPath)
{
	FskErr err;
	FskZip zip = NULL;
	char *path = FskStrDoCopy(fullPath);
	const char *dotZip = fullPath;
	char *endZip = NULL;
	unsigned char *zipFile;

	while (true) {
		char *p, *q;
		splitZipPath(dotZip + 1, &p, &q);
		if (NULL == p)
			break;
		dotZip = p;
		endZip = q;
	}

	if (!endZip)
		return kFskErrInvalidParameter;

	*(path + (endZip - fullPath)) = 0;
	err = FskZipOpen(&zip, path, kFskFilePermissionReadWrite);
	BAIL_IF_ERR(err);

	zipFile = findFileInZip(zip, endZip + 1);
	if (zipFile)
        BAIL(kFskErrFileExists);

	err = addPathToZip(zip, endZip + 1);
	BAIL_IF_ERR(err);

bail:
	FskZipClose(zip);
	FskMemPtrDispose(path);

	return err;
}

FskErr fskZipFileWrite(FskZipFile zipFile, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWrittenOut)
{
	FskErr err;
	FskInt64 position;
	UInt32 bytesWritten;

	position = zipFile->compressedFileStartingOffset + zipFile->position;
	err = spannedZipFileSetPosition(zipFile->zip, &position);
	BAIL_IF_ERR(err);

	err = spannedZipFileWrite(zipFile->zip, bytesToWrite, buffer, &bytesWritten);
	BAIL_IF_ERR(err);

	zipFile->position += bytesWritten;
	if (zipFile->position > zipFile->fileSize) {
		FskZipCentralDirectoryFile entry = (FskZipCentralDirectoryFile)zipFile->entry;
		zipFile->fileSize = zipFile->position;
		zip64SetCentralDirectoryData(zipFile->zip, (UInt8 **)&entry, &(zipFile->fileSize), &(zipFile->fileSize), NULL, NULL);
		zipFile->zip->needToFlush = true;
	}

	if (bytesWrittenOut)
		*bytesWrittenOut = bytesWritten;

bail:
	return err;
}

FskErr addPathToZip(FskZip zip, const char *path)
{
	FskErr err;
	UInt32 sizeNeeded;
	FskZipCentralDirectoryFile entry;
	FskInt64 offset = 0;
	UInt32 diskNumberU32 = 0;

	if (zip->writeFile)
		return kFskErrTooManyOpenFiles;

	// find place to write file entry
	zipGetEndOfFiles(zip, &offset);

	// grow the central directory to hold this one
	sizeNeeded = 46 + FskStrLen(path);

	err = FskMemPtrRealloc((UInt32)zip->centralDirSize + sizeNeeded, (FskMemPtr *)&zip->centralDir);
	BAIL_IF_ERR(err);

	entry = (FskZipCentralDirectoryFile)(zip->centralDir + zip->centralDirSize);
	FskMemSet(entry, 0, sizeNeeded);

	zip->centralDirSize += sizeNeeded;

	// fill out the new entry
	FskMemMove(46 + (char *)entry, path, FskStrLen(path));
	entry->signature = FskEndianU32_NtoB(0x504b0102);
	entry->fileNameLen = (UInt16)FskStrLen(path);
	entry->fileNameLen = FskEndianU16_NtoL(entry->fileNameLen);
	if (zip->splitSize)
		diskNumberU32 = (UInt32)(offset / zip->splitSize);
	offset -= diskNumberU32 * zip->splitSize;
	zip64SetCentralDirectoryData(zip, (UInt8 **)&entry, NULL, NULL, &diskNumberU32, &offset);

	// write file entry
	err = zipWriteFileHeader(zip, entry);
	BAIL_IF_ERR(err);

	// get the central directory up-to-date
	zip->centralDirCount += 1;
	err = sortZipDirectory(zip);
	BAIL_IF_ERR(err);

	// remember that we need to write the directory back out
	zip->needToFlush = true;

bail:
	return err;
}

FskErr zipWriteFileHeader(FskZip zip, FskZipCentralDirectoryFile entry)
{
	FskErr err;
	FskZipLocalFileHeaderRecord header;
	FskInt64 position, relativeOffset;
	UInt32 diskNumber;

	FskMemSet(&header, 0, sizeof(header));
	header.signature = FskEndianU32_NtoB(0x504b0304);
	header.fileNameLen = entry->fileNameLen;
	header.compressedSize = entry->compressedSize;
	header.uncompressedSize = entry->uncompressedSize;
	header.crc32 = entry->crc32;

	zip64GetCentralDirectoryData((UInt8 *)entry, NULL, NULL, &diskNumber, &relativeOffset);
	position = relativeOffset + diskNumber * zip->splitSize;
	err = spannedZipFileSetPosition(zip, &position);
	BAIL_IF_ERR(err);

	err = FskFileWrite(zip->fref, sizeof(header), &header, NULL);
	BAIL_IF_ERR(err);

	err = FskFileWrite(zip->fref, FskEndianU16_LtoN(entry->fileNameLen), entry + 1, NULL);		// file path
	BAIL_IF_ERR(err);

	// If Central Directory of this file has a extra field, I don't write a extra field 
	// to Local File Header too. Because ZIP specification said that "Extra Field records
	// that may contain information about a file that should not be exposed should not be
	// stored in the Local Header and should only be written to the Central Directory where
	// they can be encrypted."(XIV. Strong Encryption Specification (EFS)).

bail:
	return err;
}

FskErr zipFlush(FskZip zip)
{
	FskErr err = kFskErrNone;
	UInt32 i;
	FskInt64 offset, position;
	FskZipEndOfCentralDirectoryRecord ecd;
	FskZip64EndOfCentralDirectoryRecord zip64ecd;
	FskZip64EndOfCentralDirectoryLocatorRecord zip64ecdl;
	Boolean bZip64 = false;

	if (!zip->needToFlush)
		goto bail;

	// In the case of the spanned zip,
	// change the signature(0x504b0708) into temporary signature
	// (0x504b0303), if the spliting process starts but only
	// requires one segment.
	if (0 != zip->splitSize) {
		UInt32 temporarySignature = (0 == zip->numberOfThisDisk)
			                        ? FskEndianU32_NtoB(0x504b0303)
									: FskEndianU32_NtoB(0x504b0708);
		position = 0;
		spannedZipFileSetPosition(zip, &position);
		err = FskFileWrite(zip->fref, 4, &temporarySignature, NULL);
		BAIL_IF_ERR(err);
	}

	zipGetEndOfFiles(zip, &offset);
	position = offset;

	spannedZipFileSetPosition(zip, &position);

	// write central directory entries
	for (i=0; i<zip->centralDirCount; i++) {
		UInt8 *scanPtr = zip->centralDirIndex[i];
		UInt32 nameLen = scanPtr[28] + (scanPtr[29] << 8);
		UInt32 extraLen = scanPtr[30] + (scanPtr[31] << 8);
		UInt32 commentLen = scanPtr[32] + (scanPtr[33] << 8);
		UInt32 totalLen = extraLen + nameLen + commentLen + 46;
		err = FskFileWrite(zip->fref, totalLen, scanPtr, NULL);
		BAIL_IF_ERR(err);
		position += totalLen;
	}

	// write end of central directory record
	FskMemSet(&ecd, 0, sizeof(ecd));
	FskMemSet(&zip64ecd, 0, sizeof(zip64ecd));
	FskMemSet(&zip64ecdl, 0, sizeof(zip64ecdl));

	ecd.signature = FskEndianU32_NtoB(0x504b0506);
	zip64ecd.signature = FskEndianU32_NtoB(0x504b0606);
	zip64ecdl.signature = FskEndianU32_NtoB(0x504b0607);

	if (zip->centralDirCount < 0xFFFF) {
		ecd.centralDirectoryCountOnThisDisk = FskEndianU16_NtoL((UInt16)zip->centralDirCount);
		ecd.centralDirectoryCount = ecd.centralDirectoryCountOnThisDisk;
	}
	else {
		ecd.centralDirectoryCountOnThisDisk = 0xFFFF;
		ecd.centralDirectoryCount  = 0xFFFF;
		bZip64 = true;
	}
	zip64ecd.centralDirectoryCountOnThisDisk = FskEndianU64_NtoL(zip->centralDirCount);
	zip64ecd.centralDirectoryCount = zip64ecd.centralDirectoryCountOnThisDisk;

	if (zip->centralDirSize < 0xFFFFFFFF)
		ecd.centralDirectorySize = FskEndianU32_NtoL((UInt32)zip->centralDirSize);
	else {
		ecd.centralDirectorySize = 0xFFFFFFFF;
		bZip64 = true;
	}
	zip64ecd.centralDirectorySize = FskEndianU64_NtoL(zip->centralDirSize);

	offset -= zip->numberOfThisDisk * zip->splitSize;
	if (offset < 0xFFFFFFFF)
		ecd.centralDirectoryOffset = FskEndianU32_NtoL((UInt32)offset);
	else {
		ecd.centralDirectoryOffset = 0xFFFFFFFF;
		zip64ecd.centralDirectoryOffset = FskEndianU64_NtoL(offset);
		bZip64 = true;
	}

	if (zip->numberOfThisDisk < 0xFFFF) {
		ecd.numberOfThisDisk = FskEndianU16_NtoL((UInt16)zip->numberOfThisDisk);
		ecd.numberOfDiskWithCentralDir = FskEndianU16_NtoL((UInt16)zip->numberOfThisDisk);
	}
	else {
		ecd.numberOfThisDisk = 0xFFFF;
		ecd.numberOfDiskWithCentralDir = 0xFFFF;
		zip64ecd.numberOfThisDisk = FskEndianU32_NtoL(zip->numberOfThisDisk);
		zip64ecd.numberOfDiskWithCentralDir = FskEndianU32_NtoL(zip->numberOfThisDisk);
		bZip64 = true;
	}

	if (bZip64) {
		zip64ecd.sizeOfZip64EndOfCentralDir = 44;
		zip64ecd.sizeOfZip64EndOfCentralDir = FskEndianU64_NtoL(zip64ecd.sizeOfZip64EndOfCentralDir);
		zip64ecdl.numberOfDiskWithZip64EndOfCentralDir = FskEndianU32_NtoL(zip->numberOfThisDisk);
		zip64ecdl.zip64EndOfcentralDirectoryOffset = position - (zip->numberOfThisDisk * zip->splitSize);
		zip64ecdl.zip64EndOfcentralDirectoryOffset = FskEndianU64_NtoL(zip64ecdl.zip64EndOfcentralDirectoryOffset);
		zip64ecdl.totalNumberOfDisks = zip->numberOfThisDisk + 1;
		zip64ecdl.totalNumberOfDisks = FskEndianU32_NtoL(zip64ecdl.totalNumberOfDisks);

		err = FskFileWrite(zip->fref, sizeof(zip64ecd), &zip64ecd, NULL);
		BAIL_IF_ERR(err);
		err = FskFileWrite(zip->fref, sizeof(zip64ecdl), &zip64ecdl, NULL);
		BAIL_IF_ERR(err);
	}

	err = FskFileWrite(zip->fref, sizeof(ecd), &ecd, NULL);
	BAIL_IF_ERR(err);

	zip->needToFlush = false;

bail:
	return err;
}

FskZipCentralDirectoryFile zipGetEndOfFiles(FskZip zip, FskInt64 *offset)
{
	UInt32 i;
	FskZipCentralDirectoryFile best = NULL;
	unsigned char buffer[30];
	FskErr err;

	*offset = (0 == zip->splitSize) ? 0 : 4;
	for (i=0; i<zip->centralDirCount; i++) {
		FskZipCentralDirectoryFile thisEntry = (FskZipCentralDirectoryFile)zip->centralDirIndex[i];
		FskInt64 thisOffset, thisSize, thisEnd, relativeOffset;
		UInt32 diskNumber, entrySize;

		err = zip64GetCentralDirectoryData((UInt8 *)thisEntry, &thisSize, NULL, &diskNumber, &relativeOffset);
		if (err) break;
		thisOffset = relativeOffset + diskNumber * zip->splitSize;

		// read Local file header
		spannedZipFileSetPosition(zip, &thisOffset);
		err = FskFileRead(zip->fref, 30, buffer, NULL);
		if (err) break;
		entrySize = sizeof(FskZipLocalFileHeaderRecord) + (buffer[27] << 8) + buffer[26] + (buffer[29] << 8) + buffer[28];

		//@@ need to consider trailing data descriptor
		thisEnd = thisOffset + thisSize + entrySize;
		if (thisEnd > *offset) {
			*offset = thisEnd;
			best = thisEntry;
		}
	}
	
	return best;
}

#define kBufferSize (512)

void zipUpdateCRC(FskZipFile file)
{
	FskZipCentralDirectoryFile entry = (FskZipCentralDirectoryFile)file->entry;
	UInt8 buffer[kBufferSize];
	FskInt64 bytesRemaining;
	UInt32 crc = 0xffffffff;

	zip64GetCentralDirectoryData((UInt8 *)entry, &bytesRemaining, NULL, NULL, NULL);
	spannedZipFileSetPosition(file->zip, &file->compressedFileStartingOffset);

	while (bytesRemaining) {
		UInt32 bytesToRead = kBufferSize, i;
		UInt32 bytesRead;
		if (bytesToRead > bytesRemaining)
			bytesToRead = (UInt32)bytesRemaining;
		
		if (kFskErrNone != spannedZipFileRead(file->zip, bytesToRead, buffer, &bytesRead))
			break;

		for (i=0; i<bytesRead; i++)
			crc = crcTable[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);

		bytesRemaining -= bytesRead;
	}
	crc ^= 0xffffffff;

	entry->crc32 = FskEndianU32_NtoL(crc);
}

// sort the directory by full path to simplify the iterator
FskErr sortZipDirectory(FskZip zip)
{
	FskErr err;
	UInt32 i;
	UInt8 *scanPtr;

	FskMemPtrDisposeAt((void**)&zip->centralDirIndex);

	err = FskMemPtrNew((UInt32)zip->centralDirCount * sizeof(char *), (FskMemPtr *)&zip->centralDirIndex);
	BAIL_IF_ERR(err);

	for (i=0, scanPtr = zip->centralDir; i<zip->centralDirCount; i++) {
		UInt32 nameLen = scanPtr[28] + (scanPtr[29] << 8);
		UInt32 extraLen = scanPtr[30] + (scanPtr[31] << 8);
		UInt32 commentLen = scanPtr[32] + (scanPtr[33] << 8);
		zip->centralDirIndex[i] = (unsigned char *)scanPtr;
		scanPtr += extraLen + nameLen + commentLen + 46;
	}

	FskQSort(zip->centralDirIndex, (UInt32)zip->centralDirCount, sizeof(char *), zipCentralDirSort);

bail:
	return err;
}

void splitZipPath(const char *fullPath, char **dotZipOut, char **endZipOut)
{
	const char *filterExt[] = {".zip/", ".jet/", ".apk/", ".jgm/", ".dvd-hdd/", NULL};
	char *dotZip = NULL, *endZip = NULL;
	int i;

	i = 0;
	while (NULL != filterExt[i]) {
		dotZip = FskStrStr(fullPath, filterExt[i]);
		if (dotZip) {
			endZip = dotZip + FskStrLen(filterExt[i]) - 1;
			break;
		}
		i++;
	}

	if (dotZipOut)
		*dotZipOut = dotZip;
	if (endZipOut)
		*endZipOut = endZip;
}

/*
	Supporting functions to Spanned Zip files
*/

FskErr spannedZipCreateNewFile(FskZip zip)
{
	FskErr err;
	char *newPath = NULL, *zipPath = NULL;
	char *newName, *p;

	zipPath = spannedZipCreateFileNameFromNumber(zip->path, 0); 

	// only when overflowing ".zip" file, create new file.
	if (0 != FskStrCompareCaseInsensitive(zipPath, zip->path))
        BAIL(kFskErrInvalidParameter);

	// close the ".zip" file
	FskFileClose(zip->fref);
	zip->fref = NULL;

	// rename ".n[num]" to the ".zip" file
	newPath = spannedZipCreateFileNameFromNumber(zipPath, zip->numberOfThisDisk + 1); 
	FskFileDelete(newPath);
	newName = newPath;
	while (NULL != (p = FskStrStr(newName, "/"))) {
		newName = p + 1;
	}
	err = FskFileRename(zipPath, newName);
	BAIL_IF_ERR(err);

	// create the new ".zip" file
	err = FskFileCreate(zipPath);
	BAIL_IF_ERR(err);
	zip->numberOfThisDisk ++;

	// re-open the ".zip" file.
	err = FskFileOpen(zip->path, kFskFilePermissionReadWrite, &zip->fref);
	if (err) {
		FskFileOpen(zip->path, kFskFilePermissionReadOnly, &zip->fref);
		zip->openForWrite = false;
		return err;
	}
	zip->openForWrite = true;
	zip->offsetOfCurrentFile = zip->splitSize * zip->numberOfThisDisk;

bail:
	FskMemPtrDispose(zipPath);
	FskMemPtrDispose(newPath);

	return err;
}

FskErr spannedZipFileSetPosition(FskZip zip, const FskInt64 *position)
{
	FskErr err = kFskErrNone;
	UInt32 num = 0;
	char *path;
	FskInt64 posInDisk;

	if (0 == zip->splitSize) {
		return FskFileSetPosition(zip->fref, position);
	}

	posInDisk = *position;
	while (posInDisk >= zip->splitSize) {
		posInDisk -= zip->splitSize;
		num ++;
	}
	zip->offsetOfCurrentFile = *position - posInDisk;

	path = (num == zip->numberOfThisDisk)
		? spannedZipCreateFileNameFromNumber(zip->path, 0)
		: spannedZipCreateFileNameFromNumber(zip->path, num + 1);

	if (0 != FskStrCompare(path, zip->path)) {
		FskMemPtrDispose(zip->path);
		zip->path = FskStrDoCopy(path);
		FskFileClose(zip->fref);
		zip->fref = NULL;

		err = FskFileOpen(zip->path, kFskFilePermissionReadWrite, &zip->fref);
		if (err) {
			FskFileOpen(zip->path, kFskFilePermissionReadOnly, &zip->fref);
			zip->openForWrite = false;
			return err;
		}
		zip->openForWrite = true;
	}

	FskFileSetPosition(zip->fref, &posInDisk);
	FskMemPtrDispose(path);

	return err;
}

static FskErr spannedZipFileWrite_callback(FskZip zip, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten)
{
	FskErr err;
	FskInt64 position;
	UInt32 availableBytes;

	if (0 == zip->splitSize) {
		return FskFileWrite(zip->fref, bytesToWrite, buffer, bytesWritten);
	}

	FskFileGetPosition(zip->fref, &position);
	availableBytes = (UInt32)(zip->splitSize - position);
	if (bytesToWrite > availableBytes) {
		bytesToWrite = availableBytes;
		err = FskFileWrite(zip->fref, bytesToWrite, buffer, bytesWritten);
		BAIL_IF_ERR(err);

		err = spannedZipCreateNewFile(zip);
	}
	else
		err = FskFileWrite(zip->fref, bytesToWrite, buffer, bytesWritten);

bail:
	return err;
}

FskErr spannedZipFileWrite(FskZip zip, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWrittenOut)
{
	FskErr err = kFskErrNone;
	UInt32 bytesRemaining = bytesToWrite;
	const UInt8 *p = buffer;

	while (bytesRemaining) {
		UInt32 bytesWritten = 0;
		err = spannedZipFileWrite_callback(zip, bytesRemaining, p, &bytesWritten);
		p += bytesWritten;
		bytesRemaining -= bytesWritten;
		if (kFskErrNone != err)
			break;
	}

	if (bytesWrittenOut)
		*bytesWrittenOut = bytesToWrite - bytesRemaining;

	return err;
}

static FskErr spannedZipFileRead_callback(FskZip zip, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead)
{
	FskErr err;
	FskInt64 position;
	UInt32 availableBytes;

	if (0 == zip->splitSize)
		return FskFileRead(zip->fref, bytesToRead, buffer, bytesRead);

	FskFileGetPosition(zip->fref, &position);
	availableBytes = (UInt32)(zip->splitSize - position);
	if (bytesToRead > availableBytes) {
		bytesToRead = availableBytes;
		err = FskFileRead(zip->fref, bytesToRead, buffer, bytesRead);
		FskFileGetPosition(zip->fref, &position);
		position += zip->offsetOfCurrentFile;
		spannedZipFileSetPosition(zip, &position);
	}
	else {
		err = FskFileRead(zip->fref, bytesToRead, buffer, bytesRead);
	}

	return err;
}

FskErr spannedZipFileRead(FskZip zip, UInt32 bytesToRead, void *buffer, UInt32 *bytesReadOut)
{
	FskErr err = kFskErrNone;
	UInt32 bytesRemaining = bytesToRead;
	UInt8* p = buffer;

	while (bytesRemaining) {
		UInt32 bytesRead = 0;
		err = spannedZipFileRead_callback(zip, bytesRemaining, p, &bytesRead);
		p += bytesRead;
		bytesRemaining -= bytesRead;
		if (kFskErrNone != err)
			break;
	}

	if (bytesReadOut)
		*bytesReadOut = bytesToRead - bytesRemaining;

	return err;
}

char *spannedZipCreateFileNameFromNumber(const char *path, UInt32 number)
{
	FskMemPtr newName;
	const char *dotZip = NULL, *p;
	char ext[16];
	UInt32 copyLen;

	dotZip = path - 1;
	while (NULL != (p = FskStrStr(dotZip + 1, ".")))
		dotZip = p;

	// construct extension based on file number
	FskStrCopy(ext, number ? ".z" : ".zip");
	if (number) {
		char numStr[16];
		FskStrNumToStr(number, numStr, sizeof(numStr));
		if (number < 10)
			FskStrCat(ext, "0");
		FskStrCat(ext, numStr);
	}

	if (kFskErrNone != FskMemPtrNew((UInt32)FskStrLen(path) + 16 + 1, &newName))	// string + extension + trailing 0
		return NULL;
	copyLen = dotZip - path;
	FskStrNCopy((char *)newName, path, copyLen);
	newName[copyLen] = '\0';
	FskStrCat((char *)newName, ext);

	return (char *)newName;
}

FskErr spannedZipGetSplitSize(const char *path, FskInt64 *sizeOut)
{
	FskErr err = kFskErrNone;
	FskFile fref;
	FskInt64 size;
	char *z01Path;

	z01Path = spannedZipCreateFileNameFromNumber(path, 1);
	if (NULL == z01Path)
		BAIL(kFskErrMemFull);

	err = FskFileOpen(z01Path, kFskFilePermissionReadOnly, &fref);
    BAIL_IF_ERR(err);
	FskFileGetSize(fref, &size);
	FskFileClose(fref);

	if (sizeOut)
		*sizeOut = size;

bail:
	FskMemPtrDispose(z01Path);
	return err;
}

FskErr zip64ReadEndOfCentralDirectory(FskZip zip, FskInt64 locatorPos, FskZip64EndOfCentralDirectory ecd, FskZip64EndOfCentralDirectoryLocator ecdl)
{
	FskErr err = kFskErrNone;
	UInt8 buffer[64], *scanPtr;
	FskInt64 position;
	UInt32 signature, numberOfDiskWithZip64EndOfCentralDir, totalNumberOfDisks;

	// Zip64 end of central directory locator
	position = locatorPos;
	FskFileSetPosition(zip->fref, &position);
	err = FskFileRead(zip->fref, sizeof(FskZip64EndOfCentralDirectoryLocatorRecord), buffer, NULL);
	BAIL_IF_ERR(err);
	scanPtr = buffer;
	if (('P' != scanPtr[0]) || ('K' != scanPtr[1]) || (6 != scanPtr[2]) || (7 != scanPtr[3]))
		BAIL(kFskErrBadData);
	signature =								FskEndianU32_BtoN(FskMisaligned32_GetN(scanPtr)); scanPtr += 4;
	numberOfDiskWithZip64EndOfCentralDir =	FskEndianU32_LtoN(FskMisaligned32_GetN(scanPtr)); scanPtr += 4;
	position =								FskEndianU64_LtoN(FskMisaligned64_GetN(scanPtr)); scanPtr += 8;
	totalNumberOfDisks =					FskEndianU32_LtoN(FskMisaligned32_GetN(scanPtr));
	if (ecdl) {
		ecdl->signature = signature;
		ecdl->numberOfDiskWithZip64EndOfCentralDir = numberOfDiskWithZip64EndOfCentralDir;
		ecdl->zip64EndOfcentralDirectoryOffset = position;
		ecdl->totalNumberOfDisks = totalNumberOfDisks;
	}

	// Zip64 end of central directory record
	if (numberOfDiskWithZip64EndOfCentralDir + 1 != totalNumberOfDisks) {
		char *otherFilePath;
		FskFile otherFileRef;

		otherFilePath = spannedZipCreateFileNameFromNumber(zip->path, numberOfDiskWithZip64EndOfCentralDir + 1);
        BAIL_IF_NULL(otherFilePath, err, kFskErrMemFull);
		err = FskFileOpen(otherFilePath, kFskFilePermissionReadOnly, &otherFileRef);
        BAIL_IF_ERR(err);
		FskFileSetPosition(otherFileRef, &position);
		err = FskFileRead(otherFileRef, sizeof(FskZip64EndOfCentralDirectoryRecord), buffer, NULL);
		BAIL_IF_ERR(err);
		FskFileClose(otherFileRef);
		FskMemPtrDispose(otherFilePath);
	}
	else {
		FskFileSetPosition(zip->fref, &position);
		err = FskFileRead(zip->fref, sizeof(FskZip64EndOfCentralDirectoryRecord), buffer, NULL);
		BAIL_IF_ERR(err);
	}

	scanPtr = buffer;
	if (('P' != scanPtr[0]) || ('K' != scanPtr[1]) || (6 != scanPtr[2]) || (6 != scanPtr[3]))
        BAIL(kFskErrBadData);
	if (ecd) {
		ecd->signature = FskEndianU32_BtoN(FskMisaligned32_GetN(scanPtr)); scanPtr += 4;
		ecd->sizeOfZip64EndOfCentralDir = FskEndianU64_LtoN(FskMisaligned64_GetN(scanPtr)); scanPtr += 8;
		ecd->versionMadeBy = FskEndianU16_LtoN(FskMisaligned16_GetN(scanPtr)); scanPtr += 2;
		ecd->versionNeededToExtract = FskEndianU16_LtoN(FskMisaligned16_GetN(scanPtr)); scanPtr += 2;
		ecd->numberOfThisDisk = FskEndianU32_LtoN(FskMisaligned32_GetN(scanPtr)); scanPtr += 4;
		ecd->numberOfDiskWithCentralDir = FskEndianU32_LtoN(FskMisaligned32_GetN(scanPtr)); scanPtr += 4;
		ecd->centralDirectoryCountOnThisDisk = FskEndianU64_LtoN(FskMisaligned64_GetN(scanPtr)); scanPtr += 8;
		ecd->centralDirectoryCount = FskEndianU64_LtoN(FskMisaligned64_GetN(scanPtr)); scanPtr += 8;
		ecd->centralDirectorySize = FskEndianU64_LtoN(FskMisaligned64_GetN(scanPtr)); scanPtr += 8;
		ecd->centralDirectoryOffset = FskEndianU64_LtoN(FskMisaligned64_GetN(scanPtr));;
	}

bail:
	return err;
}

FskErr zip64GetCentralDirectoryData(UInt8 *entry, FskInt64 *compressSizeOut, FskInt64 *uncompressSizeOut, UInt32 *diskNumberOut, FskInt64 *relativeOffsetOut)
{
	FskErr err = kFskErrNone;
	FskInt64 uncompressSize = -1;
	FskInt64 compressSize = -1;
	FskInt64 relativeOffset = -1;
	UInt32 diskNumber = 0xFFFFFFFF;
	FskInt64 *dataPtr[3];
	UInt8 *scanPtr;
	UInt16 numField64 = 0, numField32 = 0, i;
    
    BAIL_IF_NULL(entry, err, kFskErrBadData);

	// If the original field is 0xFFFF or 0xFFFFFFFF, then the data in extra field is used.
	if (0xFF == entry[24] && 0xFF == entry[25] && 0xFF == entry[26] && 0xFF == entry[27])
		dataPtr[numField64++] = &uncompressSize;
	if (0xFF == entry[20] && 0xFF == entry[21] && 0xFF == entry[22] && 0xFF == entry[23])
		dataPtr[numField64++] = &compressSize;
	if (0xFF == entry[42] && 0xFF == entry[43] && 0xFF == entry[44] && 0xFF == entry[45])
		dataPtr[numField64++] = &relativeOffset;
	if (0xFF == entry[34] && 0xFF == entry[35])
		numField32++;

	// Read extra field. The ZIP64 extra field structure is,,
	//		Header ID (ZIP64:0x0001)	2 bytes
	//		Data Size					2 bytes
	//		-----------------------------------
	//		Uncompressed Size			8 bytes
	//		Compressed Size				8 bytes
	//		Relative Header Offset		8 bytes
	//		Disk Start Number			4 bytes
	// The order of the fields in the ZIP64 extended information record is *fixed*,
	// but the fields will *only* appear if the corresponding Local or Central directory
	// record field is set to 0xFFFF or 0xFFFFFFFF.
	if (numField64 || numField32) {
		scanPtr = findStartLocationInExtraField(entry, 0x0001);
		scanPtr += 4;
		for (i = 0; i < numField64; i++) {
			*dataPtr[i] = FskEndianU64_LtoN(FskMisaligned64_GetN(scanPtr)); scanPtr += 8;
		}
		if (numField32) {
			diskNumber = FskEndianU32_LtoN(FskMisaligned32_GetN(scanPtr));
		}
	}

	if (compressSizeOut) {
		*compressSizeOut = (-1 != compressSize) ? compressSize : FskUInt32Read_LtoN(entry + 20);
	}
	if (uncompressSizeOut) {
		*uncompressSizeOut = (-1 != uncompressSize) ? uncompressSize : FskUInt32Read_LtoN(entry + 24);
	}
	if (diskNumberOut) {
		*diskNumberOut = (0xFFFFFFFF != diskNumber) ? diskNumber : FskUInt16Read_LtoN(entry + 34);
	}
	if (relativeOffsetOut) {
		*relativeOffsetOut = (-1 != relativeOffset) ? relativeOffset : FskUInt32Read_LtoN(entry + 42);
	}

bail:
	return err;
}

FskErr zip64SetCentralDirectoryData(FskZip zip, UInt8 **entryIn, FskInt64* compressedSizeIn, FskInt64* uncompressedSizeIn, UInt32* diskNumberIn, FskInt64* relativeOffsetIn)
{
	FskErr err = kFskErrNone;
	UInt32 sizeNeeded = 0;
	UInt8 *extra = findStartLocationInExtraField(*entryIn, 0x0001);
	FskInt64 uncompressedSize, compressedSize, relativeOffset;
	UInt32 diskNumber;
	UInt8 *scanPtr;

	// get current values
	zip64GetCentralDirectoryData(*entryIn, &compressedSize, &uncompressedSize, &diskNumber, &relativeOffset);

	// check whether new fields are needed.
	if (uncompressedSizeIn && *uncompressedSizeIn >= 0xFFFFFFFF && uncompressedSize < 0xFFFFFFFF)
		sizeNeeded += 8;
	if (compressedSizeIn && *compressedSizeIn >= 0xFFFFFFFF && compressedSize < 0xFFFFFFFF)
		sizeNeeded += 8;
	if (relativeOffsetIn && *relativeOffsetIn >= 0xFFFFFFFF && relativeOffset < 0xFFFFFFFF)
		sizeNeeded += 8;
	if (diskNumberIn && *diskNumberIn >= 0xFFFF && diskNumber < 0xFFFF)
		sizeNeeded += 4;

	// extend the extra field
	if (0 != sizeNeeded) {
		unsigned char *centralDir;
		UInt32 copyLen, restLen, entryOffset;
		UInt16 extraFieldLen = FskEndianU16_LtoN(*((UInt16*)(*entryIn + 30)));
		UInt16 fileNameLength = FskEndianU16_LtoN(*((UInt16*)(*entryIn + 28)));

		sizeNeeded += 4;	// header size
		err = FskMemPtrNew((UInt32)(zip->centralDirSize + sizeNeeded), (FskMemPtr *)&centralDir);
		BAIL_IF_ERR(err);
		entryOffset = (*entryIn - zip->centralDir);
		copyLen = entryOffset + sizeof(FskZipCentralDirectoryFileRecord) + fileNameLength;
		FskMemMove(centralDir, zip->centralDir, copyLen);
		extra = centralDir + copyLen;

		copyLen += extraFieldLen;
		restLen = (UInt32)(zip->centralDirSize - copyLen);
		FskMemMove(centralDir + (copyLen + sizeNeeded), zip->centralDir + copyLen, restLen);

		*entryIn = centralDir + entryOffset;

		FskMemPtrDispose(zip->centralDir);
		zip->centralDir = centralDir;
		zip->centralDirSize += sizeNeeded;
	}

	// set data into extra fields
	{
		FskZipCentralDirectoryFile entry = (FskZipCentralDirectoryFile)(*entryIn);
		UInt16 blockType = 0x0001, blockSize;

		if (extra)
			scanPtr = extra + 4;	// skip header

		// set a uncompress size
		if (uncompressedSizeIn)
			uncompressedSize = *uncompressedSizeIn;

		if (uncompressedSize < 0xFFFFFFFF)
			entry->uncompressedSize = FskEndianU32_NtoL((UInt32)uncompressedSize);
		else {
			FskInt64 uncompressedSize_tmp = FskEndianU64_NtoL(uncompressedSize);
			entry->uncompressedSize = 0xFFFFFFFF;

			FskMisaligned64_PutN(&uncompressedSize_tmp, scanPtr);
			scanPtr += 8;
		}

		// set a compress size
		if (compressedSizeIn)
			compressedSize = *compressedSizeIn;

		if (compressedSize < 0xFFFFFFFF)
			entry->compressedSize = FskEndianU32_NtoL((UInt32)compressedSize);
		else {
			FskInt64 compressedSize_tmp = FskEndianU64_NtoL(compressedSize);
			entry->compressedSize = 0xFFFFFFFF;

			FskMisaligned64_PutN(&compressedSize_tmp, scanPtr);
			scanPtr += 8;
		}

		// set a relative offset
		if (relativeOffsetIn)
			relativeOffset = *relativeOffsetIn;

		if (relativeOffset < 0xFFFFFFFF)
			entry->relativeOffset = FskEndianU32_NtoL((UInt32)relativeOffset);
		else {
			FskInt64  relativeOffset_tmp = FskEndianU64_NtoL(relativeOffset);
			entry->relativeOffset = 0xFFFFFFFF;

			FskMisaligned64_PutN(&relativeOffset_tmp, scanPtr);
			scanPtr += 8;
		}

		// set a disk number
		if (diskNumberIn)
			diskNumber = *diskNumberIn;

		if (diskNumber < 0xFFFF)
			entry->diskNumber = FskEndianU16_NtoL((UInt16)diskNumber);
		else {
			UInt32 diskNumber_tmp = FskEndianU32_NtoL(diskNumber);
			entry->diskNumber = 0xFFFF;

			FskMisaligned32_PutN(&diskNumber_tmp, scanPtr);
			scanPtr += 4;
		}

		// set header information
		if (extra) {
			UInt16 blockType_tmp, blockSize_tmp;
			
			blockSize = (UInt16)(scanPtr - extra - 4);

			blockType_tmp = FskEndianU16_NtoL(blockType);
			blockSize_tmp = FskEndianU16_NtoL(blockSize);
			FskMisaligned16_PutN(&blockType_tmp, extra);
			FskMisaligned16_PutN(&blockSize_tmp, extra + 2);

			entry->extraFieldLen = 4 + blockSize;
			entry->extraFieldLen = FskEndianU16_NtoL(entry->extraFieldLen);
		}
	}

	// update centralDirIndex
	if (0 != sizeNeeded) {
		FskInt64 i;
		for (i=0, scanPtr = zip->centralDir; i<zip->centralDirCount; i++) {
			UInt32 nameLen = scanPtr[28] + (scanPtr[29] << 8);
			UInt32 extraLen = scanPtr[30] + (scanPtr[31] << 8);
			UInt32 commentLen = scanPtr[32] + (scanPtr[33] << 8);
			zip->centralDirIndex[i] = (unsigned char *)scanPtr;
			scanPtr += extraLen + nameLen + commentLen + 46;
		}
	}

bail:
	return err;
}

// If this function is called too frequently, we had better store
// start locations into caches.
UInt8* findStartLocationInExtraField(UInt8 *entry, UInt16 headerID)
{
	UInt16 fileNameLength, extraFieldLen;
	UInt8 *scanPtr = NULL, *endPtr;
	
	if (NULL == entry) goto bail;

	fileNameLength = FskEndianU16_LtoN(*((UInt16*)(entry + 28)));
	extraFieldLen = FskEndianU16_LtoN(*((UInt16*)(entry + 30)));
	if (0 == extraFieldLen) goto bail;

	scanPtr = entry + fileNameLength + sizeof(FskZipCentralDirectoryFileRecord);
	endPtr = scanPtr + extraFieldLen;
	while (scanPtr < endPtr) {
		UInt16 type = FskEndianU16_LtoN(FskMisaligned16_GetN(scanPtr));
		UInt16 size = FskEndianU16_LtoN(FskMisaligned16_GetN(scanPtr + 2));
		if (headerID == type) {
			return scanPtr;
		}
		scanPtr += (size + sizeof(type) + sizeof(size));
	}

bail:
	return scanPtr;
}

#ifndef KPR_CONFIG
void xs_FileSystem_ZIP_createFile(xsMachine *the)
{
	Boolean spanned = false;
	FskErr err;
	if (xsToInteger(xsArgc) > 1)
		spanned = xsToBoolean(xsArg(1));
	err = FskZipCreate(xsToString(xsArg(0)), spanned);
	FskECMAScriptThrowIf(the, err);
}
#endif

FskExport(FskErr) fsZip_fskLoad(FskLibrary library)
{
	FskExtensionInstall(kFskExtensionFileSystem, &gZipDispatch);
	return kFskErrNone;
}

FskExport(FskErr) fsZip_fskUnload(FskLibrary library)
{
	FskExtensionUninstall(kFskExtensionFileSystem, &gZipDispatch);
	return kFskErrNone;
}

