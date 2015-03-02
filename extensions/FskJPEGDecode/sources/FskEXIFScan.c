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
#include "FskEXIFScan.h"

#include "FskEndian.h"
#include "FskImage.h"
#include "FskUtilities.h"

static UInt32 getExifInteger(UInt16 type, unsigned char *exif, Boolean littleEndian);
static void setExifInteger(UInt32 value, UInt16 type, unsigned char *exif, Boolean littleEndian);
static exifValue findTag(FskEXIFScan ei, UInt32 tag);
static Boolean isRangeValid(FskEXIFScan ei, unsigned char *start, UInt32 length);

Boolean FskEXIFScanNew(unsigned char *exif, UInt32 exifLen, UInt32 ifdIndex, unsigned char **thumbBits, SInt32 *thumbBitsLen, UInt32 *orientation, FskEXIFScan *eiOut)
{
	FskErr err = kFskErrNone;
	Boolean foundIt = false;
	unsigned char *exifBase, *exifIDF = NULL, *gpsIDF = NULL;
	UInt16 directoryCount, i, exifDirectoryCount = 0, gpsDirectoryCount = 0, passNumber = 0;
	UInt32 tagBias = 0;
	UInt32 thumbCompression = 0, thumbOffset = 0, thumbSize = 0, offset;
	Boolean littleEndian;
	FskEXIFScanRecord eiBase;
	FskEXIFScan ei;
	exifValueRecord *evp;

	*eiOut = NULL;

	if (('E' != exif[0]) || ('x' != exif[1]) || ('i' != exif[2]) || ('f' != exif[3]) || (exif[4] && exif[5]))
		return false;

	FskMemSet(&eiBase, 0, sizeof(eiBase));

	eiBase.exifHeader = (unsigned char *)exif;
	eiBase.exifEnd = exif + exifLen;

	if (false == isRangeValid(&eiBase, exif, 14))
		return false;

	exif += 6;
	exifBase = exif;
	if (!((('M' == exif[0]) || ('I' == exif[0])) && (exif[0] == exif[1])))
		return false;

	littleEndian = ('I' == exif[0]);
	if ((littleEndian && ((0x2a != exif[2]) || (0 != exif[3]))) ||
		(!littleEndian && ((0 != exif[2]) || (0x2a != exif[3]))))
		return false;

	eiBase.littleEndian = littleEndian;

	exif += 4;
	if (littleEndian)
		offset = (exif[0] | (exif[1] << 8) | (exif[2] << 16) | (exif[3] << 24));
	else
		offset = (exif[3] | (exif[2] << 8) | (exif[1] << 16) | (exif[0] << 24));

	if ((0 == offset) || ((offset + 2) >= exifLen))
		return false;

	exif = exifBase + offset;
	if (false == isRangeValid(&eiBase, exif, 2))
		return false;

	directoryCount = littleEndian ? (exif[0] | (exif[1] << 8)) : (exif[1] | (exif[0] << 8));
	if (0 == directoryCount)
		return false;
	exif += 2;

	if (0 == ifdIndex)
		;
	else
	if (1 == ifdIndex) {
		UInt32 ifd1Offset;

		exif += 12 * directoryCount;
		if (false == isRangeValid(&eiBase, exif, 4))
			return false;

		if (littleEndian)
			ifd1Offset = exif[0] | (exif[1] << 8) | (exif[2] << 16) | (exif[3] << 24);
		else
			ifd1Offset = exif[3] | (exif[2] << 8) | (exif[1] << 16) | (exif[0] << 24);

		if ((0 == ifd1Offset) || ((ifd1Offset + 2) >= exifLen))
			return false;

		exif = exifBase + ifd1Offset;
		if (false == isRangeValid(&eiBase, exif, 2))
			return false;

		directoryCount = littleEndian ? (exif[0] | (exif[1] << 8)) : (exif[1] | (exif[0] << 8));
		exif += 2;
	}
	else
		return false;

	eiBase.idf = exif;
	eiBase.directoryCount = directoryCount;

	if (false == isRangeValid(&eiBase, exif, 12 * directoryCount))
		return false;

	// scan ahead for the embedded exif & gps directories
	for (i = 0; i < directoryCount; i++, exif += 12) {
		UInt16 tag = littleEndian ? (exif[0] | (exif[1] << 8)) : (exif[1] | (exif[0] << 8));
		if ((0x8769 == tag) || (0x8825 == tag)) {
			UInt32 offset;

			if (littleEndian)
				offset = exif[8] | (exif[9] << 8) | (exif[10] << 16) | (exif[11] << 24);
			else
				offset = exif[11] | (exif[10] << 8) | (exif[9] << 16) | (exif[8] << 24);

			if ((offset + 6 + 2) > exifLen)
				goto fail;

			if (0x8769 == tag) {
				exifIDF = offset + eiBase.exifHeader + 6;
				exifDirectoryCount = littleEndian ? (exifIDF[0] | (exifIDF[1] << 8)) : (exifIDF[1] | (exifIDF[0] << 8));

				if (false == isRangeValid(&eiBase, exifIDF, 12 * exifDirectoryCount))
					return false;
			}
			else {
				gpsIDF = offset + eiBase.exifHeader + 6;
				gpsDirectoryCount = littleEndian ? (gpsIDF[0] | (gpsIDF[1] << 8)) : (gpsIDF[1] | (gpsIDF[0] << 8));

				if (false == isRangeValid(&eiBase, gpsIDF, 12 * gpsDirectoryCount))
					return false;
			}
		}
	}
	exif -= directoryCount * 12;

	if (kFskErrNone != FskMemPtrNewClear(sizeof(eiBase) + ((directoryCount + exifDirectoryCount + gpsDirectoryCount) * sizeof(exifValueRecord)), &ei))
		return false;

	*ei = eiBase;
	*eiOut = ei;
	evp = &ei->values[0];
	ei->valueCount = directoryCount + exifDirectoryCount + gpsDirectoryCount;

	while (true) {
		for (i = 0; i < directoryCount; i++, exif += 12) {
			UInt32 tag = tagBias + (littleEndian ? (exif[0] | (exif[1] << 8)) : (exif[1] | (exif[0] << 8)));
			UInt16 type = littleEndian ? (exif[2] | (exif[3] << 8)) : (exif[3] | (exif[2] << 8));
			UInt32 value, count, offset;
			unsigned char *valuePtr;

			if (littleEndian)
				count = exif[4] | (exif[5] << 8) | (exif[6] << 16) | (exif[7] << 24);
			else
				count = exif[7] | (exif[6] << 8) | (exif[5] << 16) | (exif[4] << 24);

			if (littleEndian)
				offset = exif[8] | (exif[9] << 8) | (exif[10] << 16) | (exif[11] << 24);
			else
				offset = exif[11] | (exif[10] << 8) | (exif[9] << 16) | (exif[8] << 24);

			valuePtr = offset + eiBase.exifHeader + 6;
			value = getExifInteger(type, &exif[8], littleEndian);

			switch (tag) {
				case 0x103:
					thumbCompression = value;
					break;
				case 0x112:
					if (orientation)
						*orientation = value;
					break;
				case 0x201:
					thumbOffset = value;
					break;
				case 0x202:
					thumbSize = value;
					break;
			}

			evp->exifTag = tag;
			evp->exifType = type;
			evp->exifCount = count;
			switch (type) {
				case 1:
				case 3:
				case 4:
				case 9:
					if (1 == count) {
						evp->value.type = kFskMediaPropertyTypeInteger;
						evp->value.value.integer = value;
						evp->exifValuePtr = &exif[8];
						evp++;
					}
					else {
						UInt32 elementSize, arraySize;
						unsigned char *ep;

						if (1 == type)
							elementSize = 1;
						else if (3 == type)
							elementSize = 2;
						else
							elementSize = 4;
						arraySize = elementSize * count;
						ep = (arraySize <= 4) ? &exif[8] : valuePtr;
						evp->exifValuePtr = ep;
						if (false == isRangeValid(&eiBase, evp->exifValuePtr, arraySize))
							goto fail;

						if (kFskErrNone == FskMemPtrNew(count * sizeof(UInt32), (FskMemPtr*)(void*)&evp->value.value.integers.integer)) {
							UInt32 j;

							evp->value.type = kFskMediaPropertyTypeUInt32List;
							evp->value.value.integers.count = count;
							for (j = 0; j < count; j++, ep += elementSize)
								evp->value.value.integers.integer[j] = getExifInteger(type, ep, littleEndian);
							evp++;
						}
					}
					break;

				case 5:
				case 10:
					if (false == isRangeValid(&eiBase, valuePtr, 8))
						goto fail;
					evp->value.type = kFskMediaPropertyTypeRatio;
					evp->value.value.ratio.numer = getExifInteger(4, valuePtr, littleEndian);
					evp->value.value.ratio.denom = getExifInteger(4, valuePtr + 4, littleEndian);
					evp->exifValuePtr = valuePtr;
					evp++;
					break;

				case 2:
					evp->exifValuePtr = (count <= 4) ? &exif[8] : valuePtr;
					if (false == isRangeValid(&eiBase, evp->exifValuePtr, count))
						goto fail;
					evp->value.type = kFskMediaPropertyTypeString;
					err = FskMemPtrNewClear(count + 1, (FskMemPtr*)(void*)&evp->value.value.str);
					if (err) goto fail;
					FskMemMove(evp->value.value.str, evp->exifValuePtr, count);
					evp++;
					break;
			}
		}

		if (0 == passNumber)
			eiBase.idfSize = exif - eiBase.idf;
		passNumber += 1;

		if (NULL != exifIDF) {
			exif = exifIDF + 2;
			directoryCount = exifDirectoryCount;
			exifIDF = NULL;
			exifDirectoryCount = 0;

			tagBias = kFskImageMetdataExifExifPrivate;
		}
		else
		if (NULL != gpsIDF) {
			exif = gpsIDF + 2;
			directoryCount = gpsDirectoryCount;
			gpsIDF = NULL;
			gpsDirectoryCount = 0;

			tagBias = kFskImageMetdataExifGPS;
		}
		else
			break;
	}

	eiBase.valueCount = evp - ei->values;
	ei->idfSize = eiBase.idfSize;
	ei->valueCount = eiBase.valueCount;

	if (((6 == thumbCompression) && thumbOffset && thumbSize && thumbBits && thumbBitsLen) &&
				((exifBase + thumbOffset + thumbSize) <= (eiBase.exifHeader + exifLen))) {		// some corrupt files have a thumbnail length that overruns the exif area
		foundIt = true;
		*thumbBits = exifBase + thumbOffset;
		*thumbBitsLen = thumbSize;
	}

	return foundIt;

fail:
 	FskEXIFScanDisposeAt(eiOut);
	return false;
}

void FskEXIFScanDisposeAt(FskEXIFScan *eiIn)
{
	UInt32 i;
	FskEXIFScan ei = *eiIn;

	if (NULL == ei)
		return;

	for (i = 0; i < ei->valueCount; i++)
		FskMediaPropertyEmpty(&ei->values[i].value);

	FskMemPtrDispose(ei);

	*eiIn = NULL;
}

FskErr FskEXIFScanFindTagValue(FskEXIFScan ei, UInt32 tag, FskMediaPropertyValue *valueOut)
{
	exifValue evp = findTag(ei, tag);
	if (NULL == evp)
		return kFskErrUnknownElement;
	*valueOut = &evp->value;
	return kFskErrNone;
}

FskErr FskEXIFScanModifyTagValue(FskEXIFScan ei, UInt32 tag, const FskMediaPropertyValue valueIn)
{
	FskErr err = kFskErrNone;
	exifValue evp = findTag(ei, tag);

	if (NULL == evp) {
		err = kFskErrUnknownElement;
		goto bail;
	}

	switch (evp->exifType) {
		case 1:
		case 3:
		case 4:
		case 9:
			if (1 == evp->exifCount) {
				// integer
				if (kFskMediaPropertyTypeInteger != valueIn->type) {
					err = kFskErrBadData;
					goto bail;
				}
				setExifInteger(valueIn->value.integer, evp->exifType, evp->exifValuePtr, ei->littleEndian);
				evp->value.value.integer = valueIn->value.integer;
			}
			else {
				// integer array
				err = kFskErrUnimplemented;
				goto bail;
			}
			break;

		case 2:	{	// string
			UInt32 len;
			if (kFskMediaPropertyTypeString != valueIn->type) {
				err = kFskErrBadData;
				goto bail;
			}

			len = FskStrLen(valueIn->value.str) + 1;
			if (evp->exifCount < len) {
				err = kFskErrBadData;
				goto bail;
			}

			FskMemMove(evp->exifValuePtr, valueIn->value.str, len);
			FskMemMove(evp->value.value.str, valueIn->value.str, len);
			if (len < evp->exifCount) {
				FskMemSet(evp->exifValuePtr + len, 0, evp->exifCount - len);
				FskMemSet(evp->value.value.str + len, 0, evp->exifCount - len);
			}
			}
			break;

		case 5:
		case 10:	// ratio
			if (kFskMediaPropertyTypeRatio != valueIn->type) {
				err = kFskErrBadData;
				goto bail;
			}
			setExifInteger(valueIn->value.ratio.numer, 4, evp->exifValuePtr, ei->littleEndian);
			setExifInteger(valueIn->value.ratio.denom, 4, evp->exifValuePtr + 4, ei->littleEndian);
			evp->value.value.ratio = valueIn->value.ratio;
			break;

		default:
			err = kFskErrUnimplemented;
			goto bail;
	}

bail:
	return err;
}

/*
	local functions
*/

UInt32 getExifInteger(UInt16 type, unsigned char *exif, Boolean littleEndian)
{
	UInt32 value = 0;

	if (1 == type)
		value = exif[0];
	else if (3 == type) {
		if (littleEndian)
			value = exif[0] | (exif[1] << 8);
		else
			value = exif[1] | (exif[0] << 8);
	}
	else if ((4 == type) || (9 == type)) {
		if (littleEndian)
			value = exif[0] | (exif[1] << 8) | (exif[2] << 16) | (exif[3] << 24);
		else
			value = exif[3] | (exif[2] << 8) | (exif[1] << 16) | (exif[0] << 24);
	}

	return value;
}

void setExifInteger(UInt32 value, UInt16 type, unsigned char *exif, Boolean littleEndian)
{
	if (1 == type)
		exif[0] = (UInt8)value;
	else if (3 == type) {
		if (littleEndian) {
			exif[0] = (UInt8)(value >> 0);
			exif[1] = (UInt8)(value >> 8);
		}
		else {
			exif[0] = (UInt8)(value >> 8);
			exif[1] = (UInt8)(value >> 0);
		}
	}
	else if ((4 == type) || (9 == type)) {
		if (littleEndian) {
			exif[0] = (UInt8)(value >>  0);
			exif[1] = (UInt8)(value >>  8);
			exif[2] = (UInt8)(value >> 16);
			exif[3] = (UInt8)(value >> 24);
		}
		else {
			exif[0] = (UInt8)(value >> 24);
			exif[1] = (UInt8)(value >> 16);
			exif[2] = (UInt8)(value >>  8);
			exif[3] = (UInt8)(value >>  0);
		}
	}
}

exifValue findTag(FskEXIFScan ei, UInt32 tag)
{
	UInt32 i;

	tag &= 0x03ffff;
	for (i = 0; i < ei->valueCount; i++) {
		if (ei->values[i].exifTag == tag)
			return &ei->values[i];
	}
	return NULL;
}

Boolean isRangeValid(FskEXIFScan ei, unsigned char *start, UInt32 length)
{
	if (start < ei->exifHeader)
		return false;				// start is before the beginning

	if (start >= ei->exifEnd)
		return false;				// start is after the end

	if ((UInt32)(ei->exifEnd - start) < length)
		return false;				// data extends beyond end of block

	return true;
}
