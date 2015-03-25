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
#ifndef __FSK_EXIF_SCAN__
#define __FSK_EXIF_SCAN__

#include "FskMedia.h"

typedef struct FskEXIFScanRecord FskEXIFScanRecord;
typedef struct FskEXIFScanRecord *FskEXIFScan;

typedef struct {
	UInt32							exifTag;
	UInt32							exifCount;
	UInt16							exifType;
	unsigned char					*exifValuePtr;
	FskMediaPropertyValueRecord		value;
} exifValueRecord, *exifValue;

struct FskEXIFScanRecord{
	unsigned char		*exifHeader;
	unsigned char		*exifEnd;
	Boolean				littleEndian;
	UInt32				directoryCount;
	unsigned char		*idf;
	UInt32				idfSize;

	UInt32				valueCount;
	exifValueRecord		values[1];
};

Boolean FskEXIFScanNew(unsigned char *exif, UInt32 exifLen, UInt32 ifdIndex, unsigned char **thumbBits, SInt32 *thumbBitsLen, UInt32 *orientation, FskEXIFScan *info);
void FskEXIFScanDisposeAt(FskEXIFScan *ei);

FskErr FskEXIFScanFindTagValue(FskEXIFScan ei, UInt32 tag, FskMediaPropertyValue *valueOut);		// note: does not make a copy - so caller must not dispose the value
FskErr FskEXIFScanModifyTagValue(FskEXIFScan ei, UInt32 tag, const FskMediaPropertyValue valueIn);

#endif
