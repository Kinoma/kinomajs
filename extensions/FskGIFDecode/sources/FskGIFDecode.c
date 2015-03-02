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
#define __FSKIMAGE_PRIV__
#include "FskImage.h"

#include "FskBitmap.h"
#include "FskFiles.h"
#include "FskPixelOps.h"
#include "FskUtilities.h"

#define kMaxLWZBits (12)
#define kStackSize ((1 << kMaxLWZBits) * 2)

typedef struct {
	const UInt8				*data;
	SInt32					dataSize;
	Boolean					eof;
	Boolean					metaOnly;

	FskBitmap				bits;
	FskDimensionRecord		dimensions;
	UInt32					depth;

	UInt8					codeBuf[280];
	SInt32					codeCurBit;
	SInt32					codeLastBit;
	SInt32					codeDone;
	SInt32					codeLastByte;

	Boolean					firstTime;
	SInt32					codeSize;
	SInt32					setCodeSize;
	SInt32					maxCode;
	SInt32					maxCodeSize;
	SInt32					firstCode;
	SInt32					oldCode;
	SInt32					clearCode;
	SInt32					endCode;
	SInt32					table[2][1 << kMaxLWZBits];
	SInt32					stack[kStackSize];
	SInt32					*sp;
} FskGIFRecord, *FskGIF;

#define kGIFInterlace (0x40)
#define kGIFColorTable  (0x80)

static SInt32 doExtension(FskGIF gif, SInt32 label, SInt32 *transparent);
static UInt8 getDataBlock(FskGIF gif, UInt8 *buf);
static SInt32 getCode(FskGIF gif);
static void initLZW(FskGIF gif, SInt32 codeSize);
static SInt16 getByteLZW(FskGIF gif);

static FskErr parseStream(FskGIF gif);
static void parseImage(FskGIF gif, SInt32 len, SInt32 height, UInt8 *colors, SInt32 interlace, SInt32 transparentIndex);
static FskErr getBytes(FskGIF gif, void *buffer, SInt32 len);

FskErr gifDecodeCanHandle(FskBitmapFormatEnum format, const char *mime, const char *extension, Boolean *canHandle)
{
	*canHandle =	(mime && (0 == FskStrCompare(mime, "image/gif"))) ||
					(extension && (0 == FskStrCompare(extension, "gif")));

	return kFskErrNone;
}

FskErr gifDecodeSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	if (dataSize > 6) {
		if ((0 == FskStrCompareWithLength((char *)data, "GIF87a", 6)) || (0 == FskStrCompareWithLength((char *)data, "GIF89a", 6))) {
			*mime = FskStrDoCopy("image/gif");
			return kFskErrNone;
		}
	}

	return kFskErrUnknownElement;
}

FskErr gifDecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	FskGIF gif = NULL;
	FskErr err;

	if (NULL == data)
		return kFskErrOperationFailed;

	err = FskMemPtrNewClear(sizeof(FskGIFRecord), &gif);
	if (err) goto bail;

	gif->data = data;
	gif->dataSize = dataSize;
	gif->bits = deco->bits;

	err = parseStream(gif);
	if (err) goto bail;

	deco->bits = gif->bits;

bail:
	FskMemPtrDispose(gif);
	return err;
}

FskErr gifDecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 property, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	FskGIF gif;
	FskErr err;

	if ((property != kFskImageDecompressMetaDataDimensions) && (property != kFskImageDecompressMetaDataBitDepth))
		return kFskErrUnimplemented;

	err = FskMemPtrNewClear(sizeof(FskGIFRecord), &gif);
	if (err) goto bail;

	gif->data = deco->data;
	gif->dataSize = deco->dataSize;
	gif->metaOnly = true;

	err = parseStream(gif);
	if (err) goto bail;

	if (kFskImageDecompressMetaDataDimensions == property) {
		value->type = kFskMediaPropertyTypeDimension;
		value->value.dimension = gif->dimensions;
	}
	else {
		value->type = kFskMediaPropertyTypeInteger;
		value->value.integer = gif->depth;
	}

bail:
	FskMemPtrDispose(gif);

	return err;
}

FskErr getBytes(FskGIF gif, void *buffer, SInt32 len)
{
	if (len <= gif->dataSize) {
		FskMemMove(buffer, gif->data, len);
		gif->data += len;
		gif->dataSize -= len;
		return kFskErrNone;
	}

	return kFskErrEndOfFile;
}

FskErr parseStream(FskGIF gif)
{
	SInt32 transparentIndex = -1;
	unsigned char buf[16];
	unsigned char   c;
	SInt32 width, height, colorCount;
	const unsigned char *globalColorTable = NULL;

	if (kFskErrNone != getBytes(gif, buf, 6))
		goto bail;

	if ((FskStrCompareWithLength((char*)buf, "GIF87a", 6) != 0) && (FskStrCompareWithLength((char*)buf, "GIF89a", 6) != 0))
		goto bail;

	if (kFskErrNone != getBytes(gif, buf, 7))
		goto bail;

	colorCount = 2 << (buf[4] & 7);

	if (kGIFColorTable & buf[4]) {
		globalColorTable = gif->data;
		gif->data += colorCount * 3;
		gif->dataSize -= colorCount * 3;

		gif->depth = (buf[4] & 7) + 1;
	}

	while (true) {
		if (kFskErrNone != getBytes(gif, &c, 1))
			goto bail;

		if (';' == c)         /* GIF terminator */
			goto bail;

		if ('!' == c) {         /* Extension */
			if (kFskErrNone != getBytes(gif, &c, 1))
				goto bail;
			doExtension(gif, c, &transparentIndex);
			continue;
		}

		if (',' != c)	        /* Not a valid start character */
			continue;

		if (kFskErrNone != getBytes(gif, buf, 9))
			goto bail;


		if (kGIFColorTable & buf[8]) {
			if (buf[8])
				gif->depth = (buf[8] & 7) + 1;
			colorCount = 2 << (buf[8] & 7);
		}
		width = (buf[5] << 8) | buf[4];
		height = (buf[7] << 8) | buf[6];

		gif->dimensions.width = width;
		gif->dimensions.height = height;
		if (gif->metaOnly)
			return kFskErrNone;

		if (gif->bits) {
			FskRectangleRecord bounds;
			FskBitmapFormatEnum format;
			FskBitmapGetBounds(gif->bits, &bounds);
			if ((bounds.width != width) || (bounds.height != height))
				goto bail;
			FskBitmapGetPixelFormat(gif->bits, &format);
			if (kFskBitmapFormatSourceDefaultRGBA != format)
				goto bail;
		}
		else {
			if (kFskErrNone != FskBitmapNew(width, height, kFskBitmapFormatSourceDefaultRGBA, &gif->bits))
				goto bail;
		}

		if (!(kGIFColorTable & buf[8]))
			parseImage(gif, width, height, (UInt8 *)globalColorTable, 0 != (kGIFInterlace & buf[8]), transparentIndex);
		else {
			const unsigned char *colors = gif->data;
			gif->data += colorCount * 3;
			gif->dataSize -= colorCount * 3;

			parseImage(gif, width, height, (UInt8 *)colors, 0 != (kGIFInterlace & buf[8]), transparentIndex);
		}

		FskBitmapSetHasAlpha(gif->bits, transparentIndex != -1);

		return kFskErrNone;
	}

bail:
	return kFskErrBadData;
}

SInt32 doExtension(FskGIF gif, SInt32 label, SInt32 *transparent)
{
	UInt8 buf[256];

	switch (label) {
		case 0xf9:              /* Graphic Control Extension */
			if (getDataBlock(gif, buf) >= 4) {
				if (1 & buf[0])
					*transparent = buf[3];

				while (getDataBlock(gif, buf) != 0)
					;
			}
			return false;
	}

	while (getDataBlock(gif, buf) != 0)
		;

	return false;
}

UInt8 getDataBlock(FskGIF gif, UInt8 *buf)
{
	UInt8 count;

	if (kFskErrNone != getBytes(gif, &count, 1))
		return 0;

	gif->eof = (0 == count);

	if (count && (kFskErrNone != getBytes(gif, buf, count)))
		return 0;

	return count;
}

SInt32 getCode(FskGIF gif)
{
	SInt32 i, j, ret;
	UInt8 count;

	if ( (gif->codeCurBit + gif->codeSize) >= gif->codeLastBit) {
		if (gif->codeDone)
			return -1;

		gif->codeBuf[0] = gif->codeBuf[gif->codeLastByte - 2];
		gif->codeBuf[1] = gif->codeBuf[gif->codeLastByte - 1];

		count = getDataBlock(gif, &gif->codeBuf[2]);
		if (!count)
			gif->codeDone = true;

		gif->codeLastByte = 2 + count;
		gif->codeCurBit = (gif->codeCurBit - gif->codeLastBit) + 16;
		gif->codeLastBit = (2 + count) * 8;
	}

	for (i = gif->codeCurBit, j = 0, ret = 0; j < gif->codeSize; ++i, ++j)
		ret |= ((gif->codeBuf[ i / 8 ] & (1 << (i % 8))) != 0) << j;

	gif->codeCurBit += gif->codeSize;
	return ret;
}

void initLZW(FskGIF gif, SInt32 codeSize)
{
	SInt32 i;

	gif->setCodeSize = codeSize;
	gif->codeSize = codeSize + 1;
	gif->clearCode = 1 << codeSize ;
	gif->endCode = gif->clearCode + 1;
	gif->maxCodeSize = 2 * gif->clearCode;
	gif->maxCode = gif->clearCode + 2;
	gif->codeLastByte = 2;

	gif->firstTime = true;

	for (i = 0; i < gif->clearCode; ++i)
		gif->table[1][i] = i;

	gif->sp = gif->stack;
}

SInt16 getByteLZW(FskGIF gif)
{
	SInt32 code, inCode, i;

	if (gif->firstTime) {
		gif->firstTime = false;

		do {
			gif->firstCode = gif->oldCode = getCode(gif);
		} while (gif->firstCode == gif->clearCode);

		return (UInt8)gif->firstCode;
	}

	if (gif->sp > gif->stack)
		return (UInt8)(*--(gif->sp));

	while ((code = getCode(gif)) >= 0) {
		if (code == gif->clearCode) {
			for (i = 0; i < gif->clearCode; ++i) {
				gif->table[0][i] = 0;
				gif->table[1][i] = i;
			}
			for (; i < (1<<kMaxLWZBits); ++i)
				gif->table[0][i] = gif->table[1][i] = 0;
			gif->codeSize = gif->setCodeSize + 1;
			gif->maxCodeSize = 2 * gif->clearCode;
			gif->maxCode = gif->clearCode + 2;
			gif->sp = gif->stack;
			gif->firstCode = gif->oldCode = getCode(gif);
			return (UInt8)gif->firstCode;
		} else if (code == gif->endCode) {
			SInt32 count;
			UInt8 buf[260];

			if (gif->eof)
				return -2;

			while ((count = getDataBlock(gif, buf)) > 0)
				;

			if (0 != count)
				return -2;
		}

		inCode = code;

		if (gif->sp == (gif->stack + kStackSize)) /* Bad compressed data stream */
			return -1;

		if (code >= gif->maxCode) {
			*(gif->sp++) = gif->firstCode;
			code = gif->oldCode;
		}

		while (code >= gif->clearCode) {
			if (gif->sp == (gif->stack + kStackSize))		/* Bad compressed data stream */
				return -1;

			*(gif->sp++) = gif->table[1][code];
			if (code == gif->table[0][code])
				;

			code = gif->table[0][code];
		}

		*(gif->sp++) = gif->firstCode = gif->table[1][code];

		if ((code = gif->maxCode) <(1<<kMaxLWZBits)) {
			gif->table[0][code] = gif->oldCode;
			gif->table[1][code] = gif->firstCode;
			++gif->maxCode;
			if ((gif->maxCode >= gif->maxCodeSize) &&
				(gif->maxCodeSize < (1<<kMaxLWZBits))) {
				gif->maxCodeSize *= 2;
				++(gif->codeSize);
			}
		}

		gif->oldCode = inCode;

		if (gif->sp > gif->stack)
			return (UInt8)*(--gif->sp);
	}

	return (UInt8)code;
}

void parseImage(FskGIF gif, SInt32 len, SInt32 height, UInt8 *colors, SInt32 interlace, SInt32 transparentIndex)
{
	UInt8 c;
	SInt32 x = 0, y = 0, pass = 0;
	UInt8 *bits = NULL;
	SInt32 rowBytes;
	UInt8 *pixel;

	if (kFskErrNone != getBytes(gif, &c, 1))
		return;

	initLZW(gif, c);

	FskBitmapWriteBegin(gif->bits, (void**)(void*)(&bits), &rowBytes, NULL);
	pixel = bits;

	while (true) {
		UInt8 *color;
		SInt16 vv = getByteLZW(gif);
		UInt8 v;

		if (vv < 0)
			break;

		v = (UInt8)vv;

		color = colors + (3 * v);
		if (kFskBitmapFormatSourceDefaultRGBA == kFskBitmapFormat32BGRA) {
			*pixel++ = color[2];
			*pixel++ = color[1];
			*pixel++ = color[0];
			*pixel++ = (transparentIndex == v) ? 0 : 0xff;
		} else if (kFskBitmapFormatSourceDefaultRGBA == kFskBitmapFormat32ARGB) {
			*pixel++ = (transparentIndex == v) ? 0 : 0xff;
			*pixel++ = color[0];
			*pixel++ = color[1];
			*pixel++ = color[2];
		} else if (kFskBitmapFormatSourceDefaultRGBA == kFskBitmapFormat32RGBA) {
			*pixel++ = color[0];
			*pixel++ = color[1];
			*pixel++ = color[2];
			*pixel++ = (transparentIndex == v) ? 0 : 0xff;
		} else if (kFskBitmapFormatSourceDefaultRGBA == kFskBitmapFormat16AG) {
			{
			UInt16 g16;
			*pixel++ = (transparentIndex == v) ? 0 : 0xff;
			fskConvertRGBtoGray(color[0], color[1], color[2], g16);
			*pixel++ = (UInt8)g16;
			}
		} else if (kFskBitmapFormatSourceDefaultRGBA == kFskBitmapFormat16RGBA4444LE) {
			*(UInt16 *)pixel = ((color[0] >> 4) << 12) | ((color[1] >> 4) << 8) | ((color[2] >> 4) << 4) | ((transparentIndex == v) ? 0 : 0x0f);;
			pixel += 2;
		} else if (kFskBitmapFormatSourceDefaultRGBA == kFskBitmapFormat32ABGR) {
			*pixel++ = (transparentIndex == v) ? 0 : 0xff;
			*pixel++ = color[2];
			*pixel++ = color[1];
			*pixel++ = color[0];
		} else if (kFskBitmapFormatSourceDefaultRGBA == kFskBitmapFormat32A16RGB565LE) {
			*(UInt32 *)pixel = ((color[0] >> 3) << 11) | ((color[1] >> 2) << 5) | ((color[2] >> 3) << 0) | ((transparentIndex == v) ? 0 : (0xff << 24));
			pixel += 4;
		}
		++x;
		if (x == len) {
			x = 0;
			if (!interlace)
				y += 1;
			else {
				switch (pass) {
					case 0:
					case 1: y += 8; break;
					case 2: y += 4; break;
					case 3: y += 2; break;
				}

				if (y >= height) {
					++pass;
					switch (pass) {
						case 1: y = 4; break;
						case 2: y = 2; break;
						case 3: y = 1; break;
						default:
							goto done;
					}
				}
			}

			if (y >= height)
				break;

			pixel = bits + (y * rowBytes);
		}
	}

done:
	getByteLZW(gif);

	if (NULL != bits)
		FskBitmapWriteEnd(gif->bits);
}

// gifDecodeProperties

static FskErr gifDecodeGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord gifDecodeProperties[] = {
	{kFskMediaPropertyDLNASinks,		kFskMediaPropertyTypeStringList,	gifDecodeGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,		kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskErr gifDecodeGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "image/gif\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}
