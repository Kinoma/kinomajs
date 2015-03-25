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
#include "FskBitmap.h"
#include "FskEndian.h"
#include "FskFiles.h"
#include "FskImage.h"
#include "FskPixelOps.h"
#include "FskPngDecode.h"
#include "FskPort.h"
#include "FskUtilities.h"
#include "FskArch.h"
#include "zlib.h"

#define SUPPORT_SCALING 1

#if SUPPORT_SCALING
    #include "FskAAScaleBitmap.h"
    #if FSKBITMAP_OPENGL
        #include "FskGLBlit.h"
    #endif
#endif

#if SUPPORT_INSTRUMENTATION
FskInstrumentedTypeRecord gFskPngDecodeTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "FskPngDecode"};
//FskInstrumentedSimpleType(FskPngDecode, FskPngDecode);
#endif

typedef struct FskPNGDecodeStateRecord FskPNGDecodeStateRecord;
typedef struct FskPNGDecodeStateRecord *FskPNGDecodeState;

struct FskPNGDecodeStateRecord {
	FskMediaSpooler					spooler;
	UInt32							spoolerPosition;
	Boolean							spoolerOpen;
    FskImageDecompress              deco;
};

static FskErr pngDecodeSetSpooler(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr pngDecodeGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord pngDecodeProperties[] = {
	{kFskMediaPropertySpooler,			kFskMediaPropertyTypeSpooler,		NULL,						pngDecodeSetSpooler},
	{kFskMediaPropertyDLNASinks,		kFskMediaPropertyTypeStringList,	pngDecodeGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,		kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

#if SUPPORT_SCALING
    static void *getOutputLine(void *userData, Boolean done);
#endif

static FskErr FskPngDecodeFromMemory(FskPNGDecodeState state, unsigned char *pngBits, SInt32 pngBitsLen, FskBitmap *bits, FskDimension dimensions, UInt32 *depth);
static FskErr spoolerEnsure(FskPNGDecodeState state, unsigned char **pngBits, UInt32 bytesNeeded);

FskErr pngDecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
	*canHandle =	(mime && (0 == FskStrCompare(mime, "image/png"))) ||
					(extension && (0 == FskStrCompare(extension, "png")));

	return kFskErrNone;
}

FskErr pngDecodeSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	if (dataSize > 8) {
		if ((0x89 == data[0]) && (0x50 == data[1]) && (0x4e == data[2]) && (0x47 == data[3]) &&
			(0x0d == data[4]) && (0x0a == data[5]) && (0x1a == data[6]) && (0x0a == data[7])) {
			*mime = FskStrDoCopy("image/png");
			return kFskErrNone;
		}
	}

	return kFskErrUnknownElement;
}

FskErr pngDecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension)
{
	FskErr err;

    err = FskMemPtrNewClear(sizeof(FskPNGDecodeStateRecord), (FskMemPtr*)(void*)(&deco->state));
    BAIL_IF_ERR(err);

    ((FskPNGDecodeState)(deco->state))->deco = deco;

bail:
    return err;
}

FskErr pngDecodeDispose(void *stateIn, FskImageDecompress deco)
{
	FskPNGDecodeState state = stateIn;
	FskMemPtrDisposeAt((void**)(void*)(&state->spooler));
	FskMemPtrDisposeAt(&deco->state);
	return kFskErrNone;
}

FskErr pngDecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	FskErr err = kFskErrNone;
	FskBitmap bits = deco->bits;

	err = FskPngDecodeFromMemory(stateIn, (void *)data, dataSize, &bits, NULL, NULL);
	BAIL_IF_ERR(err);

	deco->bits = bits;

bail:
	return err;
}

FskErr pngDecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 property, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	FskErr err;
	FskDimensionRecord dimensions;
	UInt32 depth;

	if ((property != kFskImageDecompressMetaDataDimensions) && (property != kFskImageDecompressMetaDataBitDepth))
		return kFskErrUnimplemented;

	err = FskPngDecodeFromMemory(stateIn, deco->data, deco->dataSize, NULL, &dimensions, &depth);
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

typedef struct FskPngDecodeRecord FskPngDecodeRecord;
typedef FskPngDecodeRecord *FskPngDecode;

typedef void (*pngCopyProc)(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
typedef void (*pngIndexCopyProc)(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);

struct FskPngDecodeRecord {
	unsigned char		*pngBits;
	UInt32				pngBitsLen;

	UInt32				width;
	UInt32				height;
	unsigned char		bitDepth;
	unsigned char		bitsPerPixel;
	unsigned char		filterBytesPerPixel;
	unsigned char		colorType;
	unsigned char		compressionMethod;
	unsigned char		filterMethod;
	unsigned char		interlaceMethod;
	unsigned char		channelCount;

	FskBitmap			bitmap;
	SInt32				rowBytes;
	UInt32				depth;
	FskBitmapFormatEnum	pixelFormat;
	UInt32				pixelFormatSizeInBytes;

	unsigned char		*scanLine;			// over allocated by kScanLineSlop bytes at the start
	unsigned char		*prevScanLine;		// over allocated by kScanLineSlop bytes at the start
	unsigned char		*scanBuffers;
	UInt32				scanLineByteCount;
	UInt32				bytesInScanLine;
	unsigned char		*scanOut;
	unsigned char		*initialBaseAddr;
	unsigned char		*palette;
	Boolean				paletteHasAlpha;
	unsigned char		*crushBuffer;

	UInt32				pass;				// when interlaced, steps from 0 to 6 (7 total passes)
	UInt32				y;

	pngCopyProc			copyProc;
	pngIndexCopyProc	indexCopyProc;

	z_stream			zlib;
	Boolean				initializedZlib;

	Boolean				isApple;			// Normal PNG is RGBA; Apple PNG is BGRA.
	void (*read_filter[5])(UInt32 width,FskPngDecode png);

#if SUPPORT_SCALING
	FskAAScaler         scaler;
    unsigned char       *scalingBuffer;
#endif
};

// 4 channels @ 16 bits per pixel = 8 bytes
#define kScanLineSlop (8)

static FskErr pngIDAT(FskPngDecode png, unsigned char *pngBits, SInt32 pngBitsLen);

static void png1ToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
static void png2ToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
static void png4ToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
static void png8ToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);

static void png1AlphaToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
static void png2AlphaToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
static void png4AlphaToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
static void png8AlphaToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);


#if SRC_24BGR
	static void png24To24BGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void png24AppleTo24BGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void pngIndexTo24BGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_32ARGB
	static void png24To32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void png24AppleTo32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void pngIndexTo32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_32RGBA
	static void png24To32RGBA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
#if defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS)
	static void png24To32RGBA_neon(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	#define png24AppleTo32RGBA png24To32RGBA_neon
#else	
	#define png24AppleTo32RGBA png24To32RGBA
#endif
	static void pngIndexTo32RGBA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_32ABGR
	static void png32To32ABGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void png24To32ABGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	#define png24AppleTo32ABGR png24To32ABGR
	static void pngIndexTo32ABGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_32BGRA
	static void png24To32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	#define png24AppleTo32BGRA png24To32BGRA
	static void pngIndexTo32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_16RGB565LE
	static void png24To16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	#define png24AppleTo16RGB565LE png24To16RGB565LE
	static void pngIndexTo16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_16RGBA4444LE
	static void png24To16RGBA4444LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	#define png24AppleTo16RGBA4444LE png24To16RGBA4444LE
	static void pngIndexTo16RGBA4444LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_16BGR565LE
	static void png24To16BGR565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	#define png24AppleTo16BGR565LE png24To16BGR565LE
	static void pngIndexTo16BGR565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_8G
	static void png24To8G(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	#define png24AppleTo8G png24To8G
	static void pngIndexTo8G(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_32BGRA
	static void png32To32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void png32AppleTo32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void pngG8ATo32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void pngIndexAlphaTo32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_32ABGR
	static void pngG8ATo32ABGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	#define png32AppleTo32ABGR pngG8ATo32ABGR
	static void pngIndexAlphaTo32ABGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_32ARGB
	static void png32To32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void png32AppleTo32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void pngG8ATo32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void pngIndexAlphaTo32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
	static void png24AppleTo32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
#endif
#if SRC_32RGBA
	static void png32To32RGBA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	#define png32AppleTo32RGBA png32To32RGBA
	static void pngG8ATo32RGBA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void pngIndexAlphaTo32RGBA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_16RGBA4444LE
	static void png32To16RGBA4444LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	#define png32AppleTo16RGBA4444LE png32To16RGBA4444LE
	static void pngG8ATo16RGBA4444LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void pngIndexAlphaTo16RGBA4444LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_16AG
	static void png32To16AG(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	#define png32AppleTo16AG png32To16AG
	static void pngG8ATo16AG(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void pngIndexAlphaTo16AG(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
#endif
#if SRC_32A16RGB565LE
	static void png32To32A16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void pngG8ATo32A16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
	static void pngIndexAlphaTo32A16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask);
	static void png32AppleTo32A16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png);
#endif

FskErr FskPngDecodeFromMemory(FskPNGDecodeState state, unsigned char *pngBits, SInt32 pngBitsLen, FskBitmap *bits, FskDimension dimensions, UInt32 *depth)
{
	FskErr err = kFskErrNone;
	UInt32 tagCount = 0;
	Boolean done = false;
	FskPngDecode png = NULL;

	err = FskMemPtrNewClear(sizeof(FskPngDecodeRecord), &png);
	BAIL_IF_ERR(err);
	if (depth)	*depth = 0;	/* Assure that depth is always set */

	// validate the header
	if (pngBitsLen < 8)
		BAIL(kFskErrBadData);

	if (state->spooler) {
		if (state->spooler->doOpen) {
			err = (state->spooler->doOpen)(state->spooler, kFskFilePermissionReadOnly);
			BAIL_IF_ERR(err);
			state->spoolerPosition = 0;
			state->spoolerOpen = true;
		}

		err = spoolerEnsure(state, &pngBits, 8);
		BAIL_IF_ERR(err);
	}


	if ((0x89 != pngBits[0]) || (0x50 != pngBits[1]) || (0x4e != pngBits[2]) || (0x47 != pngBits[3]) ||
		(0x0d != pngBits[4]) || (0x0a != pngBits[5]) || (0x1a != pngBits[6]) || (0x0a != pngBits[7]))
        BAIL(kFskErrBadData);

	pngBits += 8; pngBitsLen -= 8;

	// start walking chunks
	while ((pngBitsLen >= 12) && !done) {
		SInt32 tagLen;
		UInt32 tag;

		err = spoolerEnsure(state, &pngBits, 8);
		BAIL_IF_ERR(err);

		// get the tag length and tag (always big endian, maybe misaligned)
		tagLen = (pngBits[0] << 24) | (pngBits[1] << 16) | (pngBits[2] << 8) | pngBits[3];
		tag = (pngBits[4] << 24) | (pngBits[5] << 16) | (pngBits[6] << 8) | pngBits[7];

		pngBits += 8;
		pngBitsLen -= 8;

		if (tagLen < 0)
			BAIL(kFskErrBadData);

		if ((pngBitsLen - 4) <  tagLen)		// incomplete tag
			BAIL(kFskErrBadData);

		if (1 == ++tagCount) {
			if ('CgBI' == tag)
				png->isApple = true;
			else
			if ('IHDR' != tag)	// must have one of these
                BAIL(kFskErrBadData);
		}

		// deal with this tag
		err = spoolerEnsure(state, &pngBits, tagLen + 4);		// (+4 for CRC at the end)
		BAIL_IF_ERR(err);

		switch (tag) {
			case 'IHDR':
				png->width = (pngBits[0] << 24) | (pngBits[1] << 16) | (pngBits[2] << 8) | pngBits[3];
				png->height = (pngBits[4] << 24) | (pngBits[5] << 16) | (pngBits[6] << 8) | pngBits[7];

				png->bitDepth = pngBits[8];
				png->colorType = pngBits[9];
				png->compressionMethod = pngBits[10];
				png->filterMethod = pngBits[11];
				png->interlaceMethod = pngBits[12];

				// check for undefined values
				if ((png->compressionMethod != 0) || (png->filterMethod != 0))
                    BAIL(kFskErrBadData);

				// sort ouf the bitmap format
				if (6 == png->colorType)
					png->channelCount = 4;
				else if (2 == png->colorType)
					png->channelCount = 3;
				else if (3 == png->colorType)
					png->channelCount = 1;
				else if (0 == png->colorType)
					png->channelCount = 1;
				else if (4 == png->colorType)
					png->channelCount = 2;
				png->bitsPerPixel = png->channelCount * png->bitDepth;
				png->filterBytesPerPixel = (png->bitsPerPixel + 7) >> 3;

				png->depth = png->channelCount * 8;

				// return metadata
				if (dimensions) {
					dimensions->width = png->width;
					dimensions->height = png->height;
				}
				if (depth) *depth = png->depth;

				// meta-data retrival only?
				if (NULL == bits) {
					FskMemPtrDispose(png);
					if (state->spoolerOpen && state->spooler && state->spooler->doClose) {
						(state->spooler->doClose)(state->spooler);
						state->spoolerOpen = false;
					}
					return kFskErrNone;
				}

				// check for limitations on our limitation
				if ((0 == (png->bitDepth & (16 + 8 + 4 + 2 + 1))) || ((png->colorType != 6) && (png->colorType != 4) && (png->colorType != 3) && (png->colorType != 2) && (png->colorType != 0))
						/* || (png->interlaceMethod != 0) */)
                    BAIL(kFskErrUnimplemented);

				// set up zlib
				if (false == png->isApple) {
					if (Z_OK != inflateInit(&png->zlib))
                        BAIL(kFskErrMemFull);
				}
				else {
					if (Z_OK != inflateInit2(&png->zlib, -15))
                        BAIL(kFskErrMemFull);
				}
				png->initializedZlib = true;
				break;

			case 'PLTE': {
				SInt32 i, colors = tagLen / 3;
				unsigned char *src, *dst;

				err = FskMemPtrNew(256 * 4, &png->palette);
				BAIL_IF_ERR(err);

				for (i=0, src = pngBits, dst = png->palette; i<colors; i++) {
					*dst++ = 255;			// a
					*dst++ = *src++;		// r
					*dst++ = *src++;		// g
					*dst++ = *src++;		// b
				}

				for (i=colors; i<256; i++, dst += 4)
					*dst = 255;		//@@ 0??
				}
				break;

			case 'tRNS':
				if (png->palette) {
					SInt32 i;
					unsigned char *src = pngBits, *dst = png->palette;

					for (i=0; i<tagLen; i++) {
						*dst = *src++;
						dst += 4;
					}

					png->paletteHasAlpha = true;
				}
				break;

			case 'IEND':
				done = true;
				break;

			case 'IDAT':
				// make up the bit map
				if (kFskBitmapFormatUnknown == png->pixelFormat) {
					Boolean hasAlpha = (4 == png->channelCount) || (2 == png->channelCount) || png->paletteHasAlpha;
					Boolean std = !png->isApple;

					if (hasAlpha)
						FskPortPreferredRGBFormatsGet(NULL, NULL, &png->pixelFormat);
					else
						FskPortPreferredRGBFormatsGet(NULL, &png->pixelFormat, NULL);

					if (*bits) {
						FskRectangleRecord bounds;
						FskBitmapFormatEnum pixelFormat;
						FskBitmapGetBounds(*bits, &bounds);
						FskBitmapWriteBegin(*bits, (void**)(void*)(&png->scanOut), &png->rowBytes, &pixelFormat);
						if ((bounds.width != (SInt32)png->width) || (bounds.height != (SInt32)png->height) || (pixelFormat != png->pixelFormat))
							BAIL(kFskErrBadData);
						png->bitmap = *bits;
					}
					else {
#if SUPPORT_SCALING
                        FskRectangleRecord scalingFit = {0, 0, 0, 0}, containing, containee;
#if FSKBITMAP_OPENGL
                        UInt32 maxDimension = FskGLMaximumTextureDimension();
                        if (maxDimension && ((png->width > maxDimension) || (png->height > maxDimension))) {
                            FskRectangleSet(&containing, 0, 0, maxDimension, maxDimension);
                            FskRectangleSet(&containee, 0, 0, png->width, png->height);
                            FskRectangleScaleToFit(&containing, &containee, &scalingFit);
                        }
#endif
                        if (state->deco->requestedWidth && state->deco->requestedHeight) {
                            FskRectangleRecord temp;
                            FskRectangleSet(&containing, 0, 0, state->deco->requestedWidth, state->deco->requestedHeight);
                            FskRectangleSet(&containee, 0, 0, png->width, png->height);
                            FskRectangleScaleToFit(&containing, &containee, &temp);
                            if (!scalingFit.width || !scalingFit.height || ((temp.width < scalingFit.width) && (temp.height < scalingFit.height)))
                                scalingFit = temp;
                        }
                        if ((0 == png->interlaceMethod) && scalingFit.width && scalingFit.height && (((UInt32)scalingFit.width < png->width) || ((UInt32)scalingFit.height < png->height))) {
                            FskAAScalerNew(kAAScaleTentKernelType,
                                           png->width, png->height, png->pixelFormat, NULL,
                                           scalingFit.width, scalingFit.height, png->pixelFormat, getOutputLine,
                                           0, 0, 0, 0,
                                           png,
                                           &png->scaler);

                            err = FskBitmapNew(scalingFit.width, scalingFit.height,
                                               png->pixelFormat, &png->bitmap);
                            BAIL_IF_ERR(err);
                        }
                        else
#endif
                        {
                            err = FskBitmapNew(png->width, png->height,
                                    png->pixelFormat, &png->bitmap);
                            BAIL_IF_ERR(err);
                        }

						FskBitmapSetHasAlpha(png->bitmap, hasAlpha);

						FskBitmapWriteBegin(png->bitmap, (void**)(void*)(&png->scanOut), &png->rowBytes, NULL);
					}

					FskBitmapGetDepth(png->bitmap, &png->pixelFormatSizeInBytes);
					if (png->pixelFormatSizeInBytes < 8)
						BAIL(kFskErrUnsupportedPixelType);

					png->pixelFormatSizeInBytes >>= 3;

					png->initialBaseAddr = png->scanOut;
					if ((3 == png->colorType) || (0 == png->colorType)) {
						if ((8 == png->bitsPerPixel) || (16 == png->bitsPerPixel))
							png->copyProc = png->paletteHasAlpha ? png8AlphaToNative : png8ToNative;
						else if (4 == png->bitsPerPixel)
							png->copyProc = png->paletteHasAlpha ? png4AlphaToNative : png4ToNative;
						else if (2 == png->bitsPerPixel)
							png->copyProc = png->paletteHasAlpha ? png2AlphaToNative : png2ToNative;
						else if (1 == png->bitsPerPixel)
							png->copyProc = png->paletteHasAlpha ? png1AlphaToNative : png1ToNative;

						if (0 == png->colorType) {
							unsigned char *dst;
							UInt16 i, end = 1 << ((png->bitsPerPixel > 8) ? 8 : png->bitsPerPixel);
							UInt8 step = 255 / (end - 1), val = 0;

							err = FskMemPtrNew(256 * 4, &png->palette);
							BAIL_IF_ERR(err);

							for (i=0, dst = png->palette, val = 0; i<end; i++, val += step) {
								*dst++ = 255;		// a
								*dst++ = val;		// r
								*dst++ = val;		// g
								*dst++ = val;		// b
							}
						}

						if (hasAlpha) {
							switch(png->pixelFormat) {
#if SRC_16AG
								case kFskBitmapFormat16AG:			png->indexCopyProc = pngIndexAlphaTo16AG; break;
#endif
#if SRC_16RGBA4444LE
								case kFskBitmapFormat16RGBA4444LE:	png->indexCopyProc = pngIndexAlphaTo16RGBA4444LE; break;
#endif
#if SRC_32A16RGB565LE
								case kFskBitmapFormat32A16RGB565LE:	png->indexCopyProc = pngIndexAlphaTo32A16RGB565LE; break;
#endif
#if SRC_32ARGB
								case kFskBitmapFormat32ARGB:		png->indexCopyProc = pngIndexAlphaTo32ARGB; break;
#endif
#if SRC_32RGBA
								case kFskBitmapFormat32RGBA:		png->indexCopyProc = pngIndexAlphaTo32RGBA; break;
#endif
#if SRC_32ABGR
								case kFskBitmapFormat32ABGR:		png->indexCopyProc = pngIndexAlphaTo32ABGR; break;
#endif
#if SRC_32BGRA
								case kFskBitmapFormat32BGRA:		png->indexCopyProc = pngIndexAlphaTo32BGRA; break;
#endif
                                default: break;
							}
						}
						else {
							switch(png->pixelFormat) {
#if SRC_8G
								case kFskBitmapFormat8G:			png->indexCopyProc = pngIndexTo8G; break;
#endif
#if SRC_16RGB565LE
								case kFskBitmapFormat16RGB565LE:	png->indexCopyProc = pngIndexTo16RGB565LE; break;
#endif
#if SRC_16BGR565LE
								case kFskBitmapFormat16BGR565LE:	png->indexCopyProc = pngIndexTo16BGR565LE; break;
#endif
#if SRC_24BGR
								case kFskBitmapFormat24BGR:			png->indexCopyProc = pngIndexTo24BGR; break;
#endif
#if SRC_16RGBA4444LE
								case kFskBitmapFormat16RGBA4444LE:	png->indexCopyProc = pngIndexTo16RGBA4444LE; break;
#endif
#if SRC_32ARGB
								case kFskBitmapFormat32ARGB:		png->indexCopyProc = pngIndexTo32ARGB; break;
#endif
#if SRC_32RGBA
								case kFskBitmapFormat32RGBA:		png->indexCopyProc = pngIndexTo32RGBA; break;
#endif
#if SRC_32ABGR
								case kFskBitmapFormat32ABGR:		png->indexCopyProc = pngIndexTo32ABGR; break;
#endif
#if SRC_32BGRA
								case kFskBitmapFormat32BGRA:		png->indexCopyProc = pngIndexTo32BGRA; break;
#endif
                                default: break;
							}
						}
					}
					else if (4 == png->colorType) {
						switch(png->pixelFormat) {
#if SRC_16AG
							case kFskBitmapFormat16AG:			png->copyProc = pngG8ATo16AG; break;
#endif
#if SRC_16RGBA4444LE
							case kFskBitmapFormat16RGBA4444LE:	png->copyProc = pngG8ATo16RGBA4444LE; break;
#endif
#if SRC_32A16RGB565LE
							case kFskBitmapFormat32A16RGB565LE:	png->copyProc = pngG8ATo32A16RGB565LE; break;
#endif
#if SRC_32ARGB
							case kFskBitmapFormat32ARGB:		png->copyProc = pngG8ATo32ARGB; break;
#endif
#if SRC_32RGBA
							case kFskBitmapFormat32RGBA:		png->copyProc = pngG8ATo32RGBA; break;
#endif
#if SRC_32ABGR
							case kFskBitmapFormat32ABGR:		png->copyProc = pngG8ATo32ABGR; break;
#endif
#if SRC_32BGRA
							case kFskBitmapFormat32BGRA:		png->copyProc = pngG8ATo32BGRA; break;
#endif
                            default: break;
						}
					}
					else {
						if (hasAlpha) {
							switch(png->pixelFormat) {
#if SRC_16AG
								case kFskBitmapFormat16AG:			png->copyProc = std ? png32To16AG : png32AppleTo16AG; break;
#endif
#if SRC_16RGBA4444LE
								case kFskBitmapFormat16RGBA4444LE:	png->copyProc = std ? png32To16RGBA4444LE : png32AppleTo16RGBA4444LE; break;
#endif
#if SRC_32A16RGB565LE
								case kFskBitmapFormat32A16RGB565LE:	png->copyProc = std ? png32To32A16RGB565LE : png32AppleTo32A16RGB565LE; break;
#endif
#if SRC_32ARGB
								case kFskBitmapFormat32ARGB:		png->copyProc = std ? png32To32ARGB : png32AppleTo32ARGB; break;
#endif
#if SRC_32RGBA
								case kFskBitmapFormat32RGBA:		png->copyProc = std ? png32To32RGBA : png32AppleTo32RGBA; break;
#endif
#if SRC_32ABGR
								case kFskBitmapFormat32ABGR:		png->copyProc = std ? png32To32ABGR : png32AppleTo32ABGR; break;
#endif
#if SRC_32BGRA
								case kFskBitmapFormat32BGRA:		png->copyProc = std ? png32To32BGRA : png32AppleTo32BGRA; break;
#endif
                                default: break;
								}
						}
						else {
							switch(png->pixelFormat) {
#if SRC_8G
								case kFskBitmapFormat8G:			png->copyProc = std ? png24To8G : png24AppleTo8G; break;
#endif
#if SRC_16RGB565LE
								case kFskBitmapFormat16RGB565LE:	png->copyProc = std ? png24To16RGB565LE : png24AppleTo16RGB565LE; break;
#endif
#if SRC_16BGR565LE
								case kFskBitmapFormat16BGR565LE:	png->copyProc = std ? png24To16BGR565LE : png24AppleTo16BGR565LE; break;
#endif
#if SRC_24BGR
								case kFskBitmapFormat24BGR:			png->copyProc = std ? png24To24BGR : png24AppleTo24BGR; break;
#endif
#if SRC_16RGBA4444LE
								case kFskBitmapFormat16RGBA4444LE:	png->copyProc = std ? png24To16RGBA4444LE : png24AppleTo16RGBA4444LE; break;
#endif
#if SRC_32ARGB
								case kFskBitmapFormat32ARGB:		png->copyProc = std ? png24To32ARGB : png24AppleTo32ARGB; break;
#endif
#if SRC_32RGBA
								case kFskBitmapFormat32RGBA:
								{		
#if defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS)
									int implementation = FskHardwareGetARMCPU_All();
								 	if( implementation == FSK_ARCH_ARM_V7 )
										png->copyProc = std ? png24To32RGBA_neon : png24AppleTo32RGBA; 
									else
#endif
										png->copyProc = std ? png24To32RGBA : png24AppleTo32RGBA; 
									break;
								}
#endif
#if SRC_32ABGR
								case kFskBitmapFormat32ABGR:		png->copyProc = std ? png24To32ABGR : png24AppleTo32ABGR; break;
#endif
#if SRC_32BGRA
								case kFskBitmapFormat32BGRA:		png->copyProc = std ? png24To32BGRA : png24AppleTo32BGRA; break;
#endif
                                default: break;
							}
						}
					}

					// need a scan line buffer
					png->scanLineByteCount = (png->width * png->bitsPerPixel + 7) >> 3;
					err = FskMemPtrNewClear((png->scanLineByteCount + kScanLineSlop) * 2, (FskMemPtr *)&png->scanBuffers);
					BAIL_IF_ERR(err);

					png->scanLine = png->scanBuffers;
					png->prevScanLine = png->scanBuffers + (png->scanLineByteCount + kScanLineSlop);

					png->bytesInScanLine = 0;

					if (16 == png->bitDepth) {
						err = FskMemPtrNew(png->scanLineByteCount, (FskMemPtr *)&png->crushBuffer);
						BAIL_IF_ERR(err);
					}
#if SUPPORT_SCALING
                    if (png->scaler) {
						err = FskMemPtrNew(4 * png->width, (FskMemPtr *)&png->scalingBuffer);       // worst case calcuation
						BAIL_IF_ERR(err);
                    }
#endif
				}
				err = pngIDAT(png, pngBits, tagLen);
				BAIL_IF_ERR(err);
				break;
		}

		// skip to the next tag (skip the CRC at the end of this tag too)
		pngBits += tagLen + 4; pngBitsLen -= tagLen + 4;
	}

#if 0
	// Some MP3 files have embedded PNG with extra data at the end (TedTalks)
	if ((0 != pngBitsLen) || !done) {
		// there shouldn't be any left over bytes
		BAIL(kFskErrBadData);
	}
#endif

bail:
	if (bits)
		*bits = NULL;

	if (NULL != png) {
		if (NULL != png->scanOut)
			FskBitmapWriteEnd(png->bitmap);

		if (state->spoolerOpen && state->spooler && state->spooler->doClose) {
			(state->spooler->doClose)(state->spooler);
			state->spoolerOpen = false;
		}

		if (bits && (kFskErrNone == err))
			*bits = png->bitmap;
		else
			FskBitmapDispose(png->bitmap);

		if (png->initializedZlib)
			inflateEnd(&png->zlib);

		FskMemPtrDispose(png->scanBuffers);
		FskMemPtrDispose(png->crushBuffer);
		FskMemPtrDispose(png->palette);
#if SUPPORT_SCALING
        FskMemPtrDispose(png->scalingBuffer);
        FskAAScalerDispose(png->scaler);
#endif
		FskMemPtrDispose(png);
	}

	return err;
}
#if defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS)
extern void upFilter_arm_v7_s(UInt32 width, unsigned char *pix, unsigned char *prev);
extern void subFilter3_arm_v7_s(UInt32 width, unsigned char *pix, SInt16 bpp);
extern void subFilter4_align_arm_v7_s(UInt32 width, unsigned char *pix, SInt16 bpp);
extern void averageFilter3_arm_v7_s(UInt32 width, unsigned char *pix, unsigned char *prev, SInt16 bpp);
extern void averageFilter4_arm_v7_s(UInt32 width, unsigned char *pix, unsigned char *prev, SInt16 bpp);
extern void averageFilter4_align_arm_v7_s(UInt32 width, unsigned char *pix, unsigned char *prev, SInt16 bpp);
extern void paethFilter3_arm_v7_s(UInt32 width, unsigned char *pix, unsigned char *prev, SInt16 bpp);
extern void paethFilter4_arm_v7_s(UInt32 width, unsigned char *pix, unsigned char *prev, SInt16 bpp);
extern void paethFilter4_align_arm_v7_s(UInt32 width, unsigned char *pix, unsigned char *prev, SInt16 bpp);

static void upFilterNeon(UInt32 width, FskPngDecode png);
static void subFilterNeonBpp4(UInt32 width, FskPngDecode png);
static void averageFilterNeonBpp4(UInt32 width, FskPngDecode png);
static void paethFilterNeonBpp4(UInt32 width, FskPngDecode png);
static void subFilterNeonBpp3(UInt32 width, FskPngDecode png);
static void averageFilterNeonBpp3(UInt32 width, FskPngDecode png);
static void paethFilterNeonBpp3(UInt32 width, FskPngDecode png);
#endif
static void pngFilterInit(FskPngDecode png);
static void emptyFilter(UInt32 width, FskPngDecode png);
static void subFilter(UInt32 width, FskPngDecode png);
static void upFilter(UInt32 width, FskPngDecode png);
static void averageFilter(UInt32 width, FskPngDecode png);
static void paethFilter(UInt32 width, FskPngDecode png);


static void pngFilterInit(FskPngDecode png)
{
	png->read_filter[0] = emptyFilter;
#if defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS)
	int implementation = FskHardwareGetARMCPU_All();
	if( implementation == FSK_ARCH_ARM_V7 ) {
		png->read_filter[2] = upFilterNeon;
		if(png->filterBytesPerPixel==4)
		{
			png->read_filter[1] = subFilterNeonBpp4;
			png->read_filter[3] = averageFilterNeonBpp4;
			png->read_filter[4] = paethFilterNeonBpp4;
		}
		else if(png->filterBytesPerPixel==3)
		{
			png->read_filter[1] = subFilterNeonBpp3;
			png->read_filter[3] = averageFilterNeonBpp3;
			png->read_filter[4] = paethFilterNeonBpp3;
		}
		else
		{
			png->read_filter[1] = subFilter;
			png->read_filter[3] = averageFilter;
			png->read_filter[4] = paethFilter;
		}

	}
	else
#endif
	{
		png->read_filter[2] = upFilter;
		png->read_filter[1] = subFilter;
		png->read_filter[3] = averageFilter;
		png->read_filter[4] = paethFilter;
	}
}


FskErr pngIDAT(FskPngDecode png, unsigned char *pngBits, SInt32 pngBitsLen)
{
	FskErr err = kFskErrNone;
	static const UInt8 xOffset[] = {0, 4, 0, 2, 0, 1, 0};
	static const UInt8 xStep[] = {8, 8, 4, 4, 2, 2, 1};
	static const UInt8 yOffset[] = {0, 0, 4, 0, 2, 0, 1};
	static const UInt8 yStep[] = {8, 8, 8, 4, 4, 2, 2};
	UInt32 scanLineByteCount, width, xDstStep;
	Boolean recalculatePass = 0 != png->interlaceMethod;

	png->zlib.next_in	= pngBits;
	png->zlib.avail_in	= pngBitsLen;
	png->zlib.total_in	= 0;

	scanLineByteCount = png->scanLineByteCount;
	width = png->width;
	xDstStep = png->pixelFormatSizeInBytes;

	pngFilterInit(png);
	while ((pngBitsLen > 0) || (0 == png->zlib.avail_out)) {
		UInt32 bytesNeeded;
		unsigned char *swap;
		unsigned char filterMode;
		int result;

		if (recalculatePass) {
			UInt32 r;

			width = png->width / xStep[png->pass];
			r = png->width % xStep[png->pass];
			if (r && (xOffset[png->pass] < r))
				width += 1;

			scanLineByteCount = (width * png->bitsPerPixel + 7) >> 3;
			xDstStep = png->pixelFormatSizeInBytes * xStep[png->pass];
			recalculatePass = false;
		}

		bytesNeeded = (scanLineByteCount + 1) - png->bytesInScanLine; // +1 to hold filter type

		png->zlib.next_out	= png->scanLine + kScanLineSlop - 1 + png->bytesInScanLine;
		png->zlib.avail_out	= bytesNeeded;
		png->zlib.total_out	= 0;
		result = inflate(&png->zlib, Z_PARTIAL_FLUSH);
		if ((Z_OK != result) && (Z_STREAM_END != result)) {
			if (Z_DATA_ERROR == result)
				err = kFskErrBadData;
			goto bail;
		}

		pngBitsLen -= (png->zlib.next_in - pngBits);
		png->bytesInScanLine += png->zlib.total_out;

		if (bytesNeeded != png->zlib.total_out) {
			// only have a partial scan line.
			// be sure that we consumed the entire IDAT block and return
			if (0 != png->zlib.avail_in)
				err = kFskErrBadData;
			goto bail;
		}

		// we have a (potentially filtered) scan line to deal with
		filterMode = png->scanLine[kScanLineSlop - 1];
		if (filterMode > 4)
            BAIL(kFskErrBadData);

		png->scanLine[kScanLineSlop - 1] = 0;		// make sure the entire previous pixel is zeros

		png->read_filter[filterMode](scanLineByteCount,png);

		if (NULL == png->crushBuffer) {
#if SUPPORT_SCALING
            if (png->scaler) {
                (png->copyProc)(png->scanLine + kScanLineSlop, png->scalingBuffer, width, xDstStep, png);
				FskAAScalerNextLine(png->scaler, png->scalingBuffer);
            }
            else
#endif
			(png->copyProc)(png->scanLine + kScanLineSlop, png->scanOut, width, xDstStep, png);
        }
		else {
			UInt16 *in = (UInt16 *)(png->scanLine + kScanLineSlop);
			UInt8 *out = png->crushBuffer;
			UInt32 i;
			UInt32 pixelCount = width * png->channelCount;

#if TARGET_RT_LITTLE_ENDIAN
			for (i=0; i<pixelCount; i++)
				*out++ = (UInt8)*in++;
#else
			for (i=0; i<pixelCount; i++)
				*out++ = (UInt8)(*in++ >> 8);
#endif

#if SUPPORT_SCALING
            if (png->scaler) {
                (png->copyProc)(png->crushBuffer, png->scalingBuffer, width, xDstStep, png);
				FskAAScalerNextLine(png->scaler, png->scalingBuffer);
            }
            else
#endif
                (png->copyProc)(png->crushBuffer, png->scanOut, width, xDstStep, png);
        }

#if SUPPORT_SCALING
        if (NULL == png->scaler) {
#endif
		if (0 == png->interlaceMethod)
			png->scanOut += png->rowBytes;
		else {
			png->y += yStep[png->pass];
			if (png->y < png->height)
				png->scanOut += png->rowBytes * yStep[png->pass];
			else {
				while (true) {
					png->pass += 1;
					if (7 == png->pass)
						return kFskErrNone;
					if ((xOffset[png->pass] >= png->width) || (yOffset[png->pass] >= png->height))
						continue;			// this pass starts at a location beyond the bounds of this image
					png->y = yOffset[png->pass];
					png->scanOut = png->initialBaseAddr;
					png->scanOut += png->rowBytes * png->y;
					png->scanOut += png->pixelFormatSizeInBytes * xOffset[png->pass];
					FskMemSet(png->scanLine, 0, png->scanLineByteCount + kScanLineSlop);
					recalculatePass = true;
					break;
				}
			}
		}
#if SUPPORT_SCALING
        }
#endif

		// on-ward
		png->bytesInScanLine = 0;

		swap = png->prevScanLine;
		png->prevScanLine = png->scanLine;
		png->scanLine = swap;
	}

bail:
	return err;
}

#if SUPPORT_SCALING

void *getOutputLine(void *userData, Boolean done)
{
	FskPngDecode png = userData;
    unsigned char *result = png->scanOut;
	png->scanOut += png->rowBytes;
	return result;
}

#endif

void emptyFilter(UInt32 width, FskPngDecode png)
{
	return;
}

#if defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS)

void upFilterNeon(UInt32 width, FskPngDecode png)
{
	unsigned char *p0 = png->scanLine + kScanLineSlop;
	unsigned char *p1 = png->prevScanLine + kScanLineSlop;
	UInt32 k = width>>4;
	if(k>0)
	{
		dlog("calling upFilter_arm_v7_s\n");
		upFilter_arm_v7_s(k,p0,p1);
		width -= (k<<4);
		p0 += (k<<4);
		p1 += (k<<4);
	}
	while (width--)
		*p0++ += *p1++;

}

void subFilterNeonBpp3(UInt32 width, FskPngDecode png)
{
	unsigned char *p0 = png->scanLine + kScanLineSlop;
	int k = width/12;
	if(k>0)
	{
		dlog("calling subFilter3_arm_v7_s\n");
		unsigned char recover_byte = p0[k*12];
		subFilter3_arm_v7_s(k,p0,3);
		p0 += k*12;
		width -= k*12;
		*p0 = recover_byte;
	}
	unsigned char *prior = &p0[-3];
	while (width--)
		*p0++ += *prior++;
}

void subFilterNeonBpp4(UInt32 width, FskPngDecode png)
{
	unsigned char *p0 = png->scanLine + kScanLineSlop;
	if(((int)p0&0xf)!=0)
	{
		UInt32 run_width = 16 - ((int)p0&0xf);
		unsigned char *prior = &p0[-4];
		width = width - run_width;
		while (run_width--)
			*p0++ += *prior++;
	}
	dlog("calling subFilter4_align_arm_v7_s\n");
	subFilter4_align_arm_v7_s(width,p0,4);
}

void averageFilterNeonBpp3(UInt32 width, FskPngDecode png)
{
	unsigned char *p0 = png->scanLine + kScanLineSlop;
	unsigned char *p1 = png->prevScanLine + kScanLineSlop;
	int k = width/12;
	if(k>0)
	{
		dlog("calling averageFilter3_arm_v7_s\n");
		unsigned char recover_byte = p0[k*12];
		averageFilter3_arm_v7_s(k, p0, p1, 3);
		p0 += k*12;
		p1 += k*12;
		width -= k*12;
		*p0 = recover_byte;
	}
	unsigned char *prior = &p0[-3];
	while (width--){
		*p0++ += (*p1++ + *prior++) >> 1;
	}
}

void averageFilterNeonBpp4(UInt32 width, FskPngDecode png)
{
	unsigned char *p0 = png->scanLine + kScanLineSlop;
	unsigned char *p1 = png->prevScanLine + kScanLineSlop;
	if((((int)p0^(int)p1)&0xf)==0)
	{
		if(((int)p0&0xf)!=0)
		{
			UInt32 run_width = 16 - ((int)p0&0xf);
			unsigned char *prior = &p0[-4];
			width = width - run_width;
			while (run_width--){
				*p0++ += (*p1++ + *prior++) >> 1;
			}
		}
		dlog("calling averageFilter4_align_arm_v7_s\n");
		averageFilter4_align_arm_v7_s(width, p0, p1, 4);
	}
	else
	{
		dlog("calling averageFilter4_arm_v7_s\n");
		averageFilter4_arm_v7_s(width, p0, p1, 4);
	}
}

void paethFilterNeonBpp3(UInt32 width, FskPngDecode png)
{
	unsigned char *p0 = png->scanLine + kScanLineSlop;
	unsigned char *p1 = png->prevScanLine + kScanLineSlop;
	int k = width/12;
	if(k>0)
	{
		dlog("calling paethFilter3_arm_v7_s\n");
		unsigned char recover_byte = p0[k*12];
		paethFilter3_arm_v7_s(k, p0, p1, 3);
		p0 += k*12;
		p1 += k*12;
		width -= k*12;
		*p0 = recover_byte;
	}
	unsigned char *prior = &p0[-3];
	unsigned char *prevPrior = &p1[-3];

	while (width--) {
		SInt16 a = *prior++;
		SInt16 b = *p1++;
		SInt16 c = *prevPrior++;
		SInt16 p = a + b - c;
		SInt16 pa = p - a;
		SInt16 pb = p - b;
		SInt16 pc = p - c;
		if (pa < 0) pa = -pa;
		if (pb < 0) pb = -pb;
		if (pc < 0) pc = -pc;
		if ((pa <= pb) && (pa <= pc))
			p = a;
		else if (pb <= pc)
			p = b;
		else
			p = c;
		*p0++ += p;
	}
}

void paethFilterNeonBpp4(UInt32 width, FskPngDecode png)
{
	unsigned char *p0 = png->scanLine + kScanLineSlop;
	unsigned char *p1 = png->prevScanLine + kScanLineSlop;
	if((((int)p0^(int)p1)&0xf)==0)
	{
		if(((int)p0&0xf)!=0)
		{
			UInt32 run_width = 16 - ((int)p0&0xf);
			width = width - run_width;
			unsigned char *prior = &p0[-4];
			unsigned char *prevPrior = &p1[-4];

			while (run_width--) {
				SInt16 a = *prior++;
				SInt16 b = *p1++;
				SInt16 c = *prevPrior++;
				SInt16 p = a + b - c;
				SInt16 pa = p - a;
				SInt16 pb = p - b;
				SInt16 pc = p - c;
				if (pa < 0) pa = -pa;
				if (pb < 0) pb = -pb;
				if (pc < 0) pc = -pc;
				if ((pa <= pb) && (pa <= pc))
					p = a;
				else if (pb <= pc)
					p = b;
				else
					p = c;
				*p0++ += p;
			}
		}
		dlog("calling paethFilter4_align_arm_v7_s\n");
		paethFilter4_align_arm_v7_s(width, p0, p1, 4);
	}
	else
	{
		dlog("calling paethFilter4_arm_v7_s\n");
		paethFilter4_arm_v7_s(width, p0, p1, 4);
	}
}
#endif

void subFilter(UInt32 width, FskPngDecode png)
{
	unsigned char *pix = png->scanLine + kScanLineSlop;
	SInt16 bpp = png->filterBytesPerPixel;
	unsigned char *prior = &pix[-bpp];
	while (width--)
		*pix++ += *prior++;
}

void upFilter(UInt32 width, FskPngDecode png)
{
	unsigned char *pix = png->scanLine + kScanLineSlop;
	unsigned char *prev = png->prevScanLine + kScanLineSlop;
	while (width--)
		*pix++ += *prev++;
}

void averageFilter(UInt32 width, FskPngDecode png)
{
	unsigned char *pix = png->scanLine + kScanLineSlop;
	unsigned char *prev = png->prevScanLine + kScanLineSlop;
	SInt16 bpp = png->filterBytesPerPixel;
	unsigned char *prior = &pix[-bpp];
	while (width--)
	{
		*pix++ += (*prev++ + *prior++) >> 1;
	}

}

void paethFilter(UInt32 width, FskPngDecode png)
{
	unsigned char *pix = png->scanLine + kScanLineSlop;
	unsigned char *prev = png->prevScanLine + kScanLineSlop;
	SInt16	bpp = png->filterBytesPerPixel;
	unsigned char *prior = &pix[-bpp];
	unsigned char *prevPrior = &prev[-bpp];

	while (width--) {
		SInt16 a = *prior++;
		SInt16 b = *prev++;
		SInt16 c = *prevPrior++;
		SInt16 p = a + b - c;
		SInt16 pa = p - a;
		SInt16 pb = p - b;
		SInt16 pc = p - c;
		if (pa < 0) pa = -pa;
		if (pb < 0) pb = -pb;
		if (pc < 0) pc = -pc;
		if ((pa <= pb) && (pa <= pc))
			p = a;
		else if (pb <= pc)
			p = b;
		else
			p = c;

		*pix++ += p;
	}
}

void png8ToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	(png->indexCopyProc)(src, dst, width, xStep, png, 8, 0xff);
}

void png4ToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	(png->indexCopyProc)(src, dst, width, xStep, png, 4, 0x0f);
}

void png2ToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	(png->indexCopyProc)(src, dst, width, xStep, png, 2, 0x03);
}

void png1ToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	(png->indexCopyProc)(src, dst, width, xStep, png, 1, 0x01);
}

void png8AlphaToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	(png->indexCopyProc)(src, dst, width, xStep, png, 8, 0xff);
}

void png4AlphaToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	(png->indexCopyProc)(src, dst, width, xStep, png, 4, 0x0f);
}

void png2AlphaToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	(png->indexCopyProc)(src, dst, width, xStep, png, 2, 0x03);
}

void png1AlphaToNative(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	(png->indexCopyProc)(src, dst, width, xStep, png, 1, 0x01);
}

#if SRC_24BGR

void png24To24BGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = src[2];
		dst[1] = src[1];
		dst[2] = src[0];
		src += 3;
		dst += xStep;
	}
}

void png24AppleTo24BGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	if (3 == xStep)
		FskMemMove(dst, src, (width << 1) + width);
	else {
		while (width--) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			src += 3;
			dst += xStep;
		}
	}
}

void pngIndexTo24BGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		dst[0] = c[3];
		dst[1] = c[2];
		dst[2] = c[1];
		dst += xStep;
	}
}
#endif

#if SRC_32ARGB
void png24To32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = 0xff;
		dst[1] = *src++;
		dst[2] = *src++;
		dst[3] = *src++;
		dst += xStep;
	}
}

void png24AppleTo32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = 0xff;
		dst[1] = src[2];
		dst[2] = src[1];
		dst[3] = src[0];
		src += 3;
		dst += xStep;
	}
}

void pngIndexTo32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		dst[0] = 0xff;
		dst[1] = c[1];
		dst[2] = c[2];
		dst[3] = c[3];
		dst += xStep;
	}
}
#endif
#if SRC_32RGBA
#if defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS)
void png24To32RGBA_neon(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png) {
	if(xStep==4) {
		UInt32 w = width>>3;
		UInt32 alpha = 0xff;
		width &= 0x7;
#if !__clang__
		asm volatile(".fpu neon\n");
#endif
		asm volatile (
		"vdup.8 d3,%[alpha] \n"
		::[alpha] "r" (alpha)
		: "cc", "memory",
		"d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
		"d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
		"d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
		"d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31");
		while(w--) {
			asm volatile (
			"vld3.8 {d0, d1, d2}, [%[src]]!\n"
			"vst4.8 {d0, d1, d2, d3},[%[dst]]!\n"
			:[src] "+&r" (src),[dst] "+&r" (dst)
			:: "cc", "memory",
			"d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
			"d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
			"d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
			"d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31");
		}
		if(width>4) {
			asm volatile (
			"vld3.8 {d0[0], d1[0], d2[0]}, [%[src]]!\n"
			"vld3.8 {d0[1], d1[1], d2[1]}, [%[src]]!\n"
			"vld3.8 {d0[2], d1[2], d2[2]}, [%[src]]!\n"
			"vld3.8 {d0[3], d1[3], d2[3]}, [%[src]]!\n"
			"vst4.8 {d0[0], d1[0], d2[0], d3[0]},[%[dst]]!\n"
			"vst4.8 {d0[1], d1[1], d2[1], d3[1]},[%[dst]]!\n"
			"vst4.8 {d0[2], d1[2], d2[2], d3[2]},[%[dst]]!\n"
			"vst4.8 {d0[3], d1[3], d2[3], d3[3]},[%[dst]]!\n"
			:[src] "+&r" (src),[dst] "+&r" (dst)
			:: "cc", "memory",
			"d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
			"d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
			"d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
			"d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31");
			width -= 4;
		}
	}
	while(width--) {
        dst[0] = *src++;
        dst[1] = *src++;
        dst[2] = *src++;
        dst[3] = 0xff;
		dst += xStep;
	}
}
#endif

void png24To32RGBA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
#if TARGET_RT_LITTLE_ENDIAN
        *(UInt32 *)dst = (src[2] << 16) | (src[1] << 8) | (src[0] << 0) | (0xff << 24);
        src += 3;
#else
        dst[0] = *src++;
        dst[1] = *src++;
        dst[2] = *src++;
        dst[3] = 0xff;
#endif
		dst += xStep;
	}
}

void pngIndexTo32RGBA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		dst[0] = c[1];
		dst[1] = c[2];
		dst[2] = c[3];
		dst[3] = 0xff;
		dst += xStep;
	}
}
#endif
#if SRC_32ABGR
void png24To32ABGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = 0xff;
		dst[1] = src[2];
		dst[2] = src[1];
		dst[3] = src[0];
		dst += xStep;
		src += 3;
	}
}

void pngIndexTo32ABGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		dst[0] = 0xff;
		dst[1] = c[3];
		dst[2] = c[2];
		dst[3] = c[1];
		dst += xStep;
	}
}
#endif
#if SRC_32BGRA
void png24To32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = src[2];
		dst[1] = src[1];
		dst[2] = src[0];
		dst[3] = 0xff;
		dst += xStep;
		src += 3;
	}
}

void pngIndexTo32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		dst[0] = c[3];
		dst[1] = c[2];
		dst[2] = c[1];
		dst[3] = 0xff;
		dst += xStep;
	}
}
#endif
#if SRC_16RGB565LE
void png24To16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	UInt16 *out = (UInt16 *)dst;
	while (width--) {
		UInt16 pixel;

		pixel = (*src++ >> 3) << 11;
		pixel |= (*src++ >> 2) << 5;
		pixel |= *src++ >> 3;
		*out = FskEndianU16_NtoL(pixel);
		out = (UInt16 *)(xStep + (unsigned char *)out);
	}
}

void pngIndexTo16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt16 *out = (UInt16 *)dst;
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;
		UInt16 pixel;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2) + 1;
		pixel = (*c++ >> 3) << 11;
		pixel |= (*c++ >> 2) << 5;
		pixel |= *c++ >> 3;
		*out = FskEndianU16_NtoL(pixel);
		out = (UInt16 *)(xStep + (unsigned char *)out);
	}
}
#endif
#if SRC_16RGBA4444LE
void png24To16RGBA4444LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		*(UInt16 *)dst = ((src[0] >> 4) << 12) | ((src[1] >> 4) << 8) | src[2] | 0x0f;
		dst += xStep;
		src += 3;
	}
}

void pngIndexTo16RGBA4444LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		*(UInt16 *)dst = ((c[1] >> 4) << 12) | ((c[2] >> 4) << 8) | c[3] | 0x0f;
		dst += xStep;
	}
}
#endif
#if SRC_16BGR565LE
void png24To16BGR565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	UInt16 *out = (UInt16 *)dst;
	while (width--) {
		UInt16 pixel;

		pixel = *src++ >> 3;
		pixel |= (*src++ >> 2) << 5;
		pixel |= (*src++ >> 3) << 11;
		*out = FskEndianU16_NtoL(pixel);
		out = (UInt16 *)(xStep + (unsigned char *)out);
	}
}

void pngIndexTo16BGR565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt16 *out = (UInt16 *)dst;
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;
		UInt16 pixel;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2) + 1;
		pixel = *c++ >> 3;
		pixel |= (*c++ >> 2) << 5;
		pixel |= (*c++ >> 3) << 11;
		*out = FskEndianU16_NtoL(pixel);
		out = (UInt16 *)(xStep + (unsigned char *)out);
	}
}
#endif
#if SRC_8G
void png24To8G(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		UInt16 pixel;

		fskConvertRGBtoGray(src[0], src[1], src[2], pixel);
		*dst = (UInt8)pixel;
		dst += xStep;
		src += 3;
	}
}

void pngIndexTo8G(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;
		UInt16 pixel;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2) + 1;
		fskConvertRGBtoGray(c[0], c[1], c[2], pixel);
		*dst = (UInt8)pixel;
		dst += xStep;
	}
}
#endif

#if SRC_32BGRA
void png32To32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	const UInt32 mask = 0xff00ff00;

	while (width--) {
		UInt32 pixel = *(UInt32 *)src;
		UInt32 out;

		out = (pixel >> 16) & 0x0ff;
		out |= (pixel & 0x0ff) << 16;
		pixel &= mask;
		*(UInt32 *)dst = out | pixel;

		src += 4;
		dst += xStep;
	}
}

void png32AppleTo32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	if (4 == xStep)
		FskMemMove(dst, src, width << 2);
	else {
		while (width--) {
			*(UInt32 *)dst = *(UInt32 *)src;

			src += 4;
			dst += xStep;
		}
	}
}

void pngG8ATo32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		UInt8 s0 = src[0], s1 = src[1];
		src += 2;
		*(UInt32 *)dst = (s1 << 24) | (s0 << 16) | (s0 << 8) | s0;
//		*(UInt32 *)dst = (s0 << 24) | (s0 << 16) | (s0 << 8) | s1;
//		dst[0] = s0;
//		dst[1] = s0;
//		dst[2] = s0;
//		dst[3] = s1;
		dst += xStep;
	}
}

void pngIndexAlphaTo32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;
		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		dst[0] = c[3];
		dst[1] = c[2];
		dst[2] = c[1];
		dst[3] = c[0];
		dst += xStep;
	}
}
#endif
#if 0 && SRC_32BGRA
void png32To32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[1] = src[3];
		dst[2] = src[2];
		dst[3] = src[1];
		dst[0] = src[0];
		src += 4;
		dst += xStep;
	}
}

void pngG8ATo32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[1] = src[1];
		dst[2] = src[0];
		dst[3] = src[0];
		dst[0] = src[0];
		src += 2;
		dst += xStep;
	}
}

void pngIndexAlphaTo32BGRA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		dst[1] = c[0];
		dst[2] = c[3];
		dst[3] = c[2];
		dst[0] = c[1];
		dst += xStep;
	}
}
#endif
#if SRC_32ABGR
void png32To32ABGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = src[3];
		dst[1] = src[2];
		dst[2] = src[1];
		dst[3] = src[0];
		src += 4;
		dst += xStep;
	}
}

void pngG8ATo32ABGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = src[1];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		src += 2;
		dst += xStep;
	}
}

void pngIndexAlphaTo32ABGR(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		dst[0] = c[0];
		dst[1] = c[3];
		dst[2] = c[2];
		dst[3] = c[1];
		dst += xStep;
	}
}
#endif
#if SRC_32ARGB
void png32To32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = src[3];
		dst[1] = src[0];
		dst[2] = src[1];
		dst[3] = src[2];
		src += 4;
		dst += xStep;
	}
}

void png32AppleTo32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = src[3];
		dst[1] = src[2];
		dst[2] = src[1];
		dst[3] = src[0];
		src += 4;
		dst += xStep;
	}
}

void pngG8ATo32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = src[1];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		src += 2;
		dst += xStep;
	}
}

void pngIndexAlphaTo32ARGB(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		dst[0] = c[0];
		dst[1] = c[1];
		dst[2] = c[2];
		dst[3] = c[3];
		dst += xStep;
	}
}
#endif
#if SRC_32RGBA
void png32To32RGBA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = src[3];
		src += 4;
		dst += xStep;
	}
}

void pngG8ATo32RGBA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[1];
		src += 2;
		dst += xStep;
	}
}

void pngIndexAlphaTo32RGBA(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		dst[0] = c[1];
		dst[1] = c[2];
		dst[2] = c[3];
		dst[3] = c[0];
		dst += xStep;
	}
}
#endif
#if SRC_16RGBA4444LE
void png32To16RGBA4444LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		*(UInt16 *)dst = ((src[0] >> 4) << 12) | ((src[1] >> 4) << 8) | ((src[2] >> 4) << 4) | (src[3] >> 4);
		src += 4;
		dst += xStep;
	}
}

void pngG8ATo16RGBA4444LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		*(UInt16 *)dst = ((src[0] >> 4) << 12) | ((src[0] >> 4) << 8) | ((src[0] >> 4) << 4) | (src[1] >> 4);
		src += 2;
		dst += xStep;
	}
}

void pngIndexAlphaTo16RGBA4444LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		*(UInt16 *)dst = ((c[1] >> 4) << 12) | ((c[2] >> 4) << 8) | ((c[3] >> 4) << 4) | ((c[0] >> 4) << 0);
		dst += xStep;
	}
}
#endif
#if SRC_16AG
void png32To16AG(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = src[3];
		fskConvertRGBtoGray(src[0], src[1], src[2], dst[1]);
		src += 4;
		dst += xStep;
	}
}

void pngG8ATo16AG(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		dst[0] = src[1];
		dst[1] = src[0];
		src += 2;
		dst += xStep;
	}
}

void pngIndexAlphaTo16AG(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		dst[0] = c[0];
		fskConvertRGBtoGray(c[1], c[2], c[3], dst[1]);
		dst += xStep;
	}
}
#endif
#if SRC_32A16RGB565LE
void png32To32A16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		UInt32 out;

		out = (src[0] >> 3) << 11;
		out |= (src[1] >> 2) << 5;
		out |= src[2] >> 3;
		out |= src[3] << 24;

		*(UInt32 *)dst = out;

		src += 4;
		dst += xStep;
	}
}

void png32AppleTo32A16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		UInt32 out;

		out = (src[2] >> 3) << 11;
		out |= (src[1] >> 2) << 5;
		out |= src[0] >> 3;
		out |= src[3] << 24;

		*(UInt32 *)dst = out;

		src += 4;
		dst += xStep;
	}
}

void pngG8ATo32A16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png)
{
	while (width--) {
		UInt32 out;
		UInt8 s0 = src[0], s1 = src[1];
		src += 2;

		out = (s0 >> 3) << 11;
		out |= (s0 >> 2) << 5;
 		out |= s0 >> 3;
		out |= s1 << 24;

		*(UInt32 *)dst = out;
		dst += xStep;
	}
}

void pngIndexAlphaTo32A16RGB565LE(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, FskPngDecode png, UInt16 bitsPerPixel, UInt16 mask)
{
	UInt32 offset;
	unsigned char *palette = png->palette;

	for (offset = 0; width--; offset += bitsPerPixel) {
		UInt8 p = src[offset >> 3];
		UInt32 out;
		unsigned char *c;

		p = (p >> (8 - bitsPerPixel - (offset & 0x7))) & mask;
		c = palette + (p << 2);
		out = (c[1] >> 3) << 11;
		out |= (c[2] >> 2) << 5;
		out |= c[3] >> 3;
		out |= c[0] << 24;
		*(UInt32 *)dst = out;

		dst += xStep;
	}
}
#endif

FskErr pngDecodeSetSpooler(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskPNGDecodeState state = stateIn;

	FskMemPtrDisposeAt((void**)(void*)(&state->spooler));
	if (kFskMediaSpoolerValid & property->value.spooler.flags)
		return FskMemPtrNewFromData(sizeof(FskMediaSpoolerRecord), &property->value.spooler, (FskMemPtr*)(void*)(&state->spooler));

	return kFskErrNone;
}

FskErr spoolerEnsure(FskPNGDecodeState state, unsigned char **pngBits, UInt32 bytesNeeded)
{
	FskErr err;
	UInt32 bytesRead;
	void *data;

	if (!state->spooler)
		return kFskErrNone;

	err = (state->spooler->doRead)(state->spooler, state->spoolerPosition, bytesNeeded, &data, &bytesRead);
	if (err) return err;

	state->spoolerPosition += bytesRead;
	*pngBits = data;

	return kFskErrNone;
}

FskErr pngDecodeGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "image/png\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}
