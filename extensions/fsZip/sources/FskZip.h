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
#ifndef __FSK_ZIP__
#define __FSK_ZIP__

#include "Fsk.h"
#include "FskFiles.h"
#include "FskUtilities.h"

#ifndef __FSKZIP_PRIV__
	FskDeclarePrivateType(FskZip)
	FskDeclarePrivateType(FskZipFile)
	FskDeclarePrivateType(FskZipDirectoryIterator)
#else
	#include "zlib.h"
	#include "FskThread.h"

	typedef struct FskZipFileRecord *FskZipFile;

	typedef struct FskZipRecord *FskZip;

	struct FskZipRecord {
		FskZip			next;

		FskThread		thread;
		FskFile			fref;
		char			*path;

		FskInt64		centralDirSize;
		FskMemPtr		centralDir;
		
		FskInt64		centralDirCount;
		unsigned char	**centralDirIndex;

		UInt32			useCount;

		Boolean			needToFlush;
		Boolean			openForWrite;

		FskZipFile		writeFile;				// only one writer allowed at a time

		FskInt64		splitSize;
		UInt32			numberOfThisDisk;
		FskInt64		offsetOfCurrentFile;	// This is the offset from the top of the archive to the current file.
	};
	typedef struct FskZipRecord FskZipRecord;

	#define kFskZipFileBufferSize (1024)

	struct FskZipFileRecord {
		FskFileDispatchRecord	dispatch;
		FskInstrumentedItemDeclaration

		FskZip				zip;

		unsigned char		*entry;

		UInt16				method;
		z_stream			zlib;
		Boolean				initializedZlib;

		Boolean				eof;
		FskInt64			position;
		FskInt64			fileSize;

		unsigned char		buffer[kFskZipFileBufferSize];
		UInt32				bytesInBuffer;
		unsigned char		*bufferPtr;

		FskInt64			compressedBytesRemaining;
		FskInt64			compressedFilePosition;

		FskInt64			compressedFileStartingOffset;
		FskInt64			compressedFileSize;
	};
	typedef struct FskZipFileRecord FskZipFileRecord;

	typedef struct {
		FskFileDispatchRecord	dispatch;
		FskInstrumentedItemDeclaration

		FskZip				zip;
		UInt32				directoryIndex;
		char				*path;
		UInt32				pathLen;
		Boolean				eof;
	} FskZipDirectoryIteratorRecord, *FskZipDirectoryIterator;
#endif

FskErr FskZipCreate(const char *path, const Boolean bSplit);
FskErr FskZipOpen(FskZip *zipOut, const char *path, UInt32 permissions);
void FskZipClose(FskZip zip);

FskErr FskZipGetFileInfo(FskZip zip, const char *path, FskFileInfo *itemInfo);

FskErr FskZipFileOpen(FskZip zip, const char *path, FskZipFile *zipFile);
FskErr FskZipFileClose(FskZipFile zipFile);

FskErr FskZipFileGetSize(FskZipFile zipFile, FskInt64 *size);
FskErr FskZipFileRead(FskZipFile zipFile, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead);
FskErr FskZipFileSetPosition(FskZipFile zipFile, const FskInt64 *position);
FskErr FskZipFileGetPosition(FskZipFile zipFile, FskInt64 *position);

FskErr FskZipDirectoryIteratorNew(const char *directoryPath, FskZipDirectoryIterator *dirIt, FskZip zip);
FskErr FskZipDirectoryIteratorDispose(FskZipDirectoryIterator dirIt);
FskErr FskZipDirectoryIteratorGetNext(FskZipDirectoryIterator dirIt, char **name, UInt32 *itemType);

#endif
