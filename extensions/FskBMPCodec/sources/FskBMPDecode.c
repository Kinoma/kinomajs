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
#define __FSKIMAGE_PRIV__

#include "FskBlit.h"
#include "FskBMPDecode.h"
#include "FskEndian.h"
#include "FskFiles.h"
#include "FskImage.h"
#include "FskPixelOps.h"
#include "FskUtilities.h"

#include <stddef.h>

static FskErr FskBMPDecodeFromMemory(unsigned char *bmpBits, SInt32 bmpBitsLen, FskBitmap *bits, FskDimension dimensions, UInt32 *depth, FskBitmapFormatEnum outputFormat);

FskErr bmpDecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
	*canHandle =	(mime && (0 == FskStrCompare(mime, "image/bmp"))) ||
					(extension && (0 == FskStrCompare(extension, "bmp")));

	return kFskErrNone;
}

FskErr bmpDecodeSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	if (dataSize > 2) {
		if (0 == FskStrCompareWithLength((char *)data, "BM", 2)) {
			*mime = FskStrDoCopy("image/bmp");
			return kFskErrNone;
		}
	}

	return kFskErrUnknownElement;
}

typedef struct {
	FskBitmapFormatEnum pixelFormat;
} FskBMPDecodeStateRecord, *FskBMPDecodeState;

FskErr bmpDecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension)
{
	return FskMemPtrNewClear(sizeof(FskBMPDecodeStateRecord), (FskMemPtr*)(void*)&deco->state);
}

FskErr bmpDecodeDispose(void *stateIn, FskImageDecompress deco)
{
	FskBMPDecodeState state = stateIn;

	FskMemPtrDispose(state);

	return kFskErrNone;
}

FskErr bmpDecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	FskBMPDecodeState state = stateIn;
	FskErr err = kFskErrNone;
	FskBitmap bits = deco->bits;

	err = FskBMPDecodeFromMemory((void *)data, dataSize, &bits, NULL, NULL, state->pixelFormat);
	if (err) goto bail;

	deco->bits = bits;

bail:
	return err;
}

FskErr bmpDecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 property, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	FskErr err;
	FskDimensionRecord dimensions;
	UInt32 depth;

	if ((property != kFskImageDecompressMetaDataDimensions) && (property != kFskImageDecompressMetaDataBitDepth))
		return kFskErrUnimplemented;

	err = FskBMPDecodeFromMemory(deco->data, deco->dataSize, NULL, &dimensions, &depth, 0);
	if (err) return err;

	if (kFskImageDecompressMetaDataDimensions == property) {
		value->type = kFskMediaPropertyTypeDimension;
		value->value.dimension = dimensions;
	}
	else {
		value->type = kFskMediaPropertyTypeInteger;
		value->value.integer = depth;
	}

	return kFskErrNone;
}

#if TARGET_ALIGN_PACKPUSH
    #pragma pack(push, 2)
#elif TARGET_ALIGN_PACK
    #pragma pack(2)
#endif

typedef struct {
	UInt16		imageFileType;
	UInt32		fileSize;
	UInt16		reserved1;
	UInt16		reserved2;
	UInt32		imageDataOffset;

	// now comes the BITMAPINFOHEADER
	UInt32		biSize;
	SInt32		biWidth;
	SInt32		biHeight;
	UInt16		biPlanes;
	UInt16		biBitCount;
	UInt32		biCompression;
	UInt32		biSizeImage;
	SInt32		biXPelsPerMeter;
	SInt32		biYPelsPerMeter;
	UInt32		biClrUsed;
	UInt32		biClrImportant;
} FSK_PACK_STRUCT FskBMPRecord, *FskBMP;

typedef struct {
	UInt16		imageFileType;
	UInt32		fileSize;
	UInt16		reserved1;
	UInt16		reserved2;
	UInt32		imageDataOffset;

	// now comes the BITMAPINFOHEADER
	UInt32		biSize;
	SInt16		biWidth;
	SInt16		biHeight;
	UInt16		biPlanes;
	UInt16		biBitCount;
} FSK_PACK_STRUCT FskBMPOS221Record, *FskBMPOS221;

#if TARGET_ALIGN_PACKPUSH
    #pragma pack(pop)
#elif TARGET_ALIGN_PACK
    #pragma pack()
#endif

#define checkSrcBits(count) {if ((srcBits + count) > srcEnd) {err = kFskErrBadData; goto bail;}}

FskErr FskBMPDecodeFromMemory(unsigned char *bmpBits, SInt32 bmpBitsLen, FskBitmap *bits, FskDimension dimensions, UInt32 *depth, FskBitmapFormatEnum outputFormat)
{
	FskErr err = kFskErrNone;
	FskBMP bmp = (FskBMP)bmpBits;
	UInt8 *srcBits = bmpBits, *dstBits = NULL, *srcEnd = bmpBits + bmpBitsLen;
	SInt32 srcRowBytes, dstRowBytes;
    SInt32 rawHeight;
	UInt32 x, y, width, height, biBitCount, biCompression;
	UInt8 *palette = NULL, *scanBuffer = NULL;
	UInt32 mask, shift;
	FskBMPRecord scratch;
	UInt32 paletteWidth = 4;
	UInt32 biSize;
	FskBitmap allocatedBitmap = NULL;
	UInt32 aSum = 0;

	checkSrcBits(6);
	if (('B' != bmpBits[0]) || ('M' != bmpBits[1]))
		return kFskErrBadData;

	checkSrcBits(offsetof(FskBMPRecord, biWidth));
	biSize = FskEndianU32_LtoN(bmp->biSize);
	if (biSize < 12)
		return kFskErrBadData;
	checkSrcBits(biSize);
	if (12 == biSize) {
		FskBMPOS221 t = (FskBMPOS221)bmp;
		FskMemSet(&scratch, 0, sizeof(scratch));
		scratch.imageDataOffset = t->imageDataOffset;
		scratch.biSize = t->biSize;
		scratch.biWidth = t->biWidth;
		scratch.biHeight = t->biHeight;
		scratch.biPlanes = t->biPlanes;
		scratch.biBitCount = t->biBitCount;
		bmp = &scratch;
		paletteWidth = 3;
		width = FskEndianS16_LtoN(bmp->biWidth);
		rawHeight = FskEndianS16_LtoN(bmp->biHeight);
	}
	else {
		width = FskEndianU32_LtoN(bmp->biWidth);
		rawHeight = FskEndianU32_LtoN(bmp->biHeight);
	}

    height = (rawHeight >= 0) ? rawHeight : -rawHeight;

	if (dimensions) {
		dimensions->width = width;
		dimensions->height = height;
	}
	if (depth) *depth = FskEndianU16_LtoN(bmp->biBitCount);

	if (NULL == bits)
		return kFskErrNone;

	biBitCount = FskEndianU16_LtoN(bmp->biBitCount);
	biCompression = FskEndianU32_LtoN(bmp->biCompression);
	if ((1 != FskEndianU16_LtoN(bmp->biPlanes)) ||
		(biCompression > 2))
		return kFskErrUnimplemented;

	if (NULL == *bits) {
		if (outputFormat)
			;
		else if (32 == biBitCount)
			outputFormat = kFskBitmapFormatDefaultRGBA;
		else
			outputFormat = kFskBitmapFormatDefaultRGB;
		err = FskBitmapNew(width, height, outputFormat, bits);
		if (err) return err;

		allocatedBitmap = *bits;
	}
	else {
		FskRectangleRecord bounds;
		FskBitmapGetBounds(*bits, &bounds);
		if ((bounds.width != (SInt32)width) || (bounds.height != (SInt32)height))
			return kFskErrBadData;

		FskBitmapGetPixelFormat(*bits, &outputFormat);
	}

	palette = biSize + 14 + (UInt8 *)bmpBits;
	srcBits = bmpBits + FskEndianU32_LtoN(bmp->imageDataOffset);
	checkSrcBits(0);

	srcRowBytes = (width * biBitCount + 7) >> 3;
	if (srcRowBytes % 4)
		srcRowBytes += 4 - (srcRowBytes % 4);

	if (0 != biCompression) {
		err = FskMemPtrNew(srcRowBytes, &scanBuffer);
		if (err) return err;
	}

	FskBitmapWriteBegin(*bits, (void**)(void*)&dstBits, &dstRowBytes, NULL);
    if (rawHeight > 0)
        dstBits += (height - 1) * dstRowBytes;
	mask = (1 << biBitCount) - 1;
	shift = 8 - biBitCount;

	for (y=0; y<height; y++) {
		UInt8 *s = srcBits;
		UInt8 *d = dstBits;
		UInt8 p = 0, pBits = 0;
		UInt8 *color;

		if (1 == biCompression) {
			UInt8 *out = scanBuffer;
			x = 0;
			while (true) {
				unsigned char count;

				checkSrcBits(2);
				count = *srcBits++;
				if (count) {
					// encoded run
					x += count;
					while (count--)
						*out++ = *srcBits;
					srcBits++;
				}
				else {
					// literal or escape
					count = *srcBits++;
					if (count >= 3) {
						UInt32 pad = count & 1;
						checkSrcBits(count + pad);
						x += count;
						while (count--)
							*out++ = *srcBits++;
						srcBits += pad;		// keep even aligned
					}
					else if (0 == count)
						break;
					else if (1 == count) {
						y = height;
						break;
					}
					else if (2 == count) {
						err = kFskErrUnimplemented;
						goto bail;
					}
				}
			}

			s = scanBuffer;
		}
		else if (2 == biCompression) {
			UInt8 *out = scanBuffer;
			x = 0;
			FskMemSet(out, 0, srcRowBytes);
			while (true) {
				unsigned char count;

				checkSrcBits(2);
				count = *srcBits++;
				if (count) {
					// encoded run
					unsigned char val = *srcBits++;
					unsigned char val0, val1;

					if (1 & x) {
						val0 = (val & 0x0f) << 4;
						val1 = (val & 0xf0) >> 4;
					}
					else {
						val0 = val & 0xf0;
						val1 = val & 0x0f;
					}

					while (count--) {
						out[x >> 1] |= (1 & x) ? val1 : val0;
						x += 1;
					}
				}
				else {
					// literal or escape
					count = *srcBits++;
					if (count >= 3) {
						UInt32 pad = ((count + 1) >> 1) & 1;
						unsigned char val0 = 0, val1 = 0, pos = 0;
						while (count--) {
							if (0 == (1 & pos)) {
								unsigned char val;
								checkSrcBits(1);
								val = *srcBits++;
								if (1 & x) {
									val0 = (val & 0x0f) << 4;
									val1 = (val & 0xf0) >> 4;
								}
								else {
									val0 = val & 0xf0;
									val1 = val & 0x0f;
								}
							}

							out[x >> 1] |= (1 & x) ? val1 : val0;
							x++;
							pos++;
						}
						srcBits += pad;		// keep even aligned
					}
					else if (0 == count)
						break;
					else if (1 == count) {
						y = height;
						break;
					}
					else if (2 == count) {
						err = kFskErrUnimplemented;
						goto bail;
					}
				}
			}

			s = scanBuffer;
		}

		for (x=0; x<width; x++) {
			UInt8 a = 255, r, g, b;

			if (0 == biCompression)
				checkSrcBits(srcRowBytes);

			/* Unpack components into r, g, b, a */
			if (24 == biBitCount) {
				b = *s++;
				g = *s++;
				r = *s++;
			}
			else if (16 == biBitCount) {
				UInt16	c;
				c = *s++;
				c |= (*s++ << 8);
				b = (c >> 0) & 0x1f;
				b = (b << 3) | (b >> 2);
				g = (c >> 5) & 0x1f;
				g = (g << 3) | (g >> 2);
				r = (c >> 10) & 0x1f;
				r = (r << 3) | (r >> 2);
			}
			else if (32 == biBitCount) {
				b = *s++;
				g = *s++;
				r = *s++;
				a = *s++;
				aSum += a;
			}
			else {
				if (0 == pBits) {
					pBits = 8;
					p = *s++;
				}
				color = &palette[paletteWidth * ((p >> shift) & mask)];
				p <<= biBitCount;
				b = color[0];
				g = color[1];
				r = color[2];
				pBits -= (UInt8)biBitCount;
			}

			/* Reformat components according to the desired output format. */
			switch (outputFormat) {
				case kFskBitmapFormat24BGR:													/* 24BGR: Windows style */
					*d++ = b;
					*d++ = g;
					*d++ = r;
					break;
				case kFskBitmapFormat24RGB:													/* 24RGB: GL format */
					*d++ = r;
					*d++ = g;
					*d++ = b;
					break;
				case kFskBitmapFormat32ARGB:												/* 32ARGB: Macintosh style */
					*d++ = a;
					*d++ = r;
					*d++ = g;
					*d++ = b;
					break;
				case kFskBitmapFormat32RGBA:												/* 32RGBA: GL format */
					*d++ = r;
					*d++ = g;
					*d++ = b;
					*d++ = a;
					break;
				case kFskBitmapFormat32ABGR:
					*d++ = a;
					*d++ = b;
					*d++ = g;
					*d++ = r;
					break;
				case kFskBitmapFormat16RGB565LE:
					*(UInt16 *)d = ((r >> 3) << 11) | ((g >> 2) << 5) | ((b >> 3) << 0);
					d += 2;
					break;
				case kFskBitmapFormat16BGR565LE:
					*(UInt16 *)d = ((b >> 3) << 11) | ((g >> 2) << 5) | ((r >> 3) << 0);
					d += 2;
					break;
				case kFskBitmapFormat32BGRA:												/* 32BGRA: Windows style */
					*d++ = b;
					*d++ = g;
					*d++ = r;
					*d++ = a;
					break;
				case kFskBitmapFormat8G:
				{	UInt16 g16;
					fskConvertRGBtoGray(r, g, b, g16);
					*d++ = (UInt8)g16;
				}	break;
				case kFskBitmapFormat16AG:
				{	UInt16 g16;
					fskConvertRGBtoGray(r, g, b, g16);
					*d++ = a;
					*d++ = (UInt8)g16;
				}	break;
				case kFskBitmapFormat16RGBA4444LE:
					*(UInt16 *)d = ((r >> 4) << 12) | ((g >> 4) << 8) | ((b >> 4) << 4) | ((a >> 4) << 0);
					d += 2;
					break;
				default:
					err = kFskErrUnsupportedPixelType;
					goto bail;
			}
		}

		if (0 == biCompression)
			srcBits += srcRowBytes;
        if (rawHeight > 0)
            dstBits -= dstRowBytes;
        else
            dstBits += dstRowBytes;
	}

	if (0 != aSum)
		FskBitmapSetHasAlpha(*bits, true);

bail:
	if ((NULL != bits) && (NULL != *bits))
		FskBitmapWriteEnd(*bits);

	if ((kFskErrNone != err) && (NULL != allocatedBitmap)) {
		FskBitmapDispose(allocatedBitmap);
		*bits = NULL;
	}

	FskMemPtrDispose(scanBuffer);

	return err;
}


/*
	simple compress
*/

FskErr bmpEncodeCanHandle(UInt32 format, const char *mime, Boolean *canHandle)
{
	*canHandle = mime && (0 == FskStrCompare(mime, "image/bmp"));

	return kFskErrNone;
}

FskErr bmpEncodeNew(FskImageCompress comp)
{
	return kFskErrNone;
}

FskErr bmpEncodeDispose(void *stateIn, FskImageCompress comp)
{
	return kFskErrNone;
}

FskErr bmpEncodeCompressFrame(void *state, FskImageCompress comp, FskBitmap bits, const void **dataOut, UInt32 *dataSizeOut, UInt32 *frameDuration, UInt32 *compositionTimeOffset, UInt32 *frameType)
{
	FskErr err;
	FskRectangleRecord bounds;
	unsigned char *image = NULL, *p;
	SInt32 imageSize = 0, x, y, outputRowBytes;
	FskBMPOS221Record bmps;
	FskColorRGBARecord rgba;
	Boolean hasAlpha;
	UInt32 *srcPtr;
	SInt32 srcRowBytes;
	FskBitmapFormatEnum srcFormat;

	FskBitmapReadBegin(bits, (const void**)(const void*)&srcPtr, &srcRowBytes, &srcFormat);
	BAIL_IF_ERR(err = FskBitmapGetPixel(bits, 0, 0, &rgba));	// verify that this pixel format is supported
	BAIL_IF_FALSE(srcFormat != kFskBitmapFormatYUV420	&&		// Our simple conversion attempt below doesn't work for YUV
				  srcFormat != kFskBitmapFormatYUV422	&&
				  srcFormat != kFskBitmapFormatUYVY		&&
				  srcFormat != kFskBitmapFormatYUV420i, err, kFskErrUnsupportedPixelType);
	FskBitmapGetBounds(bits, &bounds);

	err = FskBitmapGetHasAlpha(bits, &hasAlpha);
	if (err) goto bail;

	outputRowBytes = bounds.width * (hasAlpha ? 4 : 3);
	if (outputRowBytes % 4)
		outputRowBytes += 4 - (outputRowBytes % 4);

	imageSize = sizeof(FskBMPOS221Record) + (bounds.height * outputRowBytes);
	err = FskMemPtrNewClear(imageSize, (FskMemPtr *)&image);
	if (err) goto bail;

	bmps.imageFileType = (UInt16)(('B'<<8) | 'M');
	bmps.imageFileType = FskEndianU16_NtoB(bmps.imageFileType);
	bmps.fileSize = sizeof(FskBMPOS221Record) + (bounds.height * outputRowBytes);
	bmps.fileSize = FskEndianU32_NtoL(bmps.fileSize);
	bmps.reserved1 = 0;
	bmps.reserved2 = 0;
	bmps.imageDataOffset = FskEndianU32_NtoL(sizeof(bmps));
	bmps.biSize = FskEndianU32_NtoL(12);
	bmps.biWidth = (SInt16)FskEndianU16_NtoL(bounds.width);
	bmps.biHeight = (SInt16)FskEndianU16_NtoL(bounds.height);
	bmps.biPlanes = FskEndianU16_NtoL(1);
	bmps.biBitCount = hasAlpha ? FskEndianU16_NtoL(32) : FskEndianU16_NtoL(24);

	p = image;
	FskMemMove(p, &bmps, sizeof(bmps));
	p += sizeof(bmps);
	if (false == hasAlpha) {
		for (y = bounds.height - 1; y >= 0; y--) {
			for (x = 0; x < bounds.width; x++) {
				FskBitmapGetPixel(bits, x, y, &rgba);		// not the fastest, but the most general purpose
				*p++ = rgba.b;
				*p++ = rgba.g;
				*p++ = rgba.r;
			}
			p -= bounds.width * 3;
			p += outputRowBytes;
		}
	}
	else {
		if (kFskBitmapFormat32BGRA != srcFormat) {
			for (y = bounds.height - 1; y >= 0; y--) {
				for (x = 0; x < bounds.width; x++) {
					FskBitmapGetPixel(bits, x, y, &rgba);		// not the fastest, but the most general purpose
					*p++ = rgba.b;
					*p++ = rgba.g;
					*p++ = rgba.r;
					*p++ = rgba.a;
				}
				p -= bounds.width * 4;
				p += outputRowBytes;
			}
		}
		else {
			srcPtr = (UInt32*)((UInt32)(srcRowBytes * (bounds.height - 1)) + (char *)srcPtr);
			for (y = bounds.height - 1; y >= 0; y--) {
				for (x = 0; x < bounds.width; x++) {
					FskMisaligned32_PutN(srcPtr, p);
					++srcPtr;
					p += 4;
				}
				p -= bounds.width * 4;
				p += outputRowBytes;
				srcPtr = (UInt32 *)(-srcRowBytes - (bounds.width * 4) + (char *)srcPtr);
			}
		}
	}

bail:
	FskBitmapReadEnd(bits);

	if (err) {
		FskMemPtrDisposeAt(&image);
		imageSize = 0;
	}

	*dataOut = image;
	*dataSizeOut = imageSize;

	return err;
}

// bmpDecodeProperties

static FskErr bmpDecodeSetPixelFormat(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr bmpDecodeGetPixelFormat(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr bmpDecodeGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord bmpDecodeProperties[] = {
	{kFskMediaPropertyPixelFormat,		kFskMediaPropertyTypeUInt32List,	bmpDecodeGetPixelFormat,	bmpDecodeSetPixelFormat},
	{kFskMediaPropertyDLNASinks,		kFskMediaPropertyTypeStringList,	bmpDecodeGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,		kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskErr bmpDecodeSetPixelFormat(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskBMPDecodeState state = stateIn;
	FskErr err = kFskErrNone;

	switch (property->value.integer) {
	}

	state->pixelFormat = property->value.integers.integer[0];

	return err;
}

FskErr bmpDecodeGetPixelFormat(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskBMPDecodeState state = stateIn;

	property->type = kFskMediaPropertyTypeInteger;  //@@@need to change to new API
	property->value.number = state->pixelFormat;

	return kFskErrNone;
}

FskErr bmpDecodeGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "image/bmp\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

