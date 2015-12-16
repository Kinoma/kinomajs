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
#define __FSKBITMAP_PRIV__
#include "FskBitmap.h"

#include "FskBlit.h"
#include "FskUtilities.h"
#include "FskEndian.h"
#include "FskRectBlit.h"
#include "FskFrameBuffer.h"
#include "FskMemoryAllocator.h"
#include "FskPixelOps.h"

#if TARGET_OS_MAC
    #if TARGET_OS_IPHONE
        #include "FskCocoaSupportPhone.h"
    #else
        #include "FskCocoaSupport.h"
    #endif
#endif /* TARGET_OS_MAC */

#if FSKBITMAP_OPENGL
	#include "FskGLBlit.h"
#endif /* FSKBITMAP_OPENGL */

#ifndef FSKBITMAP_DOUBLE_ALLOC
	#if !FSKBITMAP_OPENGL
		#define FSKBITMAP_DOUBLE_ALLOC 0	/* Allocate bitmaps with one alloc. */
	#else /* FSKBITMAP_OPENGL */
		#define FSKBITMAP_DOUBLE_ALLOC 1	/* Allocate bitmaps with two allocs. */
	#endif /* FSKBITMAP_OPENGL*/
#endif /* FSKBITMAP_DOUBLE_ALLOC */

#if SUPPORT_INSTRUMENTATION
	#include <stddef.h>
	#include <stdio.h>

	static Boolean doFormatMessageBitmap(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);
	static FskInstrumentedValueRecord gInstrumentationBitmapValues[] = {
		{ "bounds",				offsetof(FskBitmapRecord, bounds),			kFskInstrumentationKindRectangle},
		{ "pixelFormat",		offsetof(FskBitmapRecord, pixelFormat),		kFskInstrumentationKindInteger},
		{ "depth",				offsetof(FskBitmapRecord, depth),			kFskInstrumentationKindInteger},
		{ "rowBytes",			offsetof(FskBitmapRecord, rowBytes),		kFskInstrumentationKindInteger},
		{ "hasAlpha",			offsetof(FskBitmapRecord, hasAlpha),		kFskInstrumentationKindBoolean},
		{ NULL,					0,											kFskInstrumentationKindUndefined}
	};

	static FskInstrumentedTypeRecord gBitmapTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"bitmap",
		FskInstrumentationOffset(FskBitmapRecord),
		NULL,
		0,
		NULL,
		doFormatMessageBitmap,
		gInstrumentationBitmapValues
	};
#endif /* SUPPORT_INSTRUMENTATION */


#define ALIGN_PIXEL_BYTES			64													/* Cache lines are 64 bytes */
#define ALIGN_ROW_BYTES				4													/* Hardware likes blocks of 8 pixels, but can handle 1 pixel alignment */
#define ALIGN_YUV_ROW_BYTES			(2*ALIGN_ROW_BYTES)									/* Since U&V are subsampled, this assures that they also have ALIGN_ROW_BYTES */
#define BLOCKIFY(i, blockSize)		(((i) + ((blockSize) - 1)) & ~((blockSize) - 1))	/* Blocksize must be a power of 2 */
#define ALIGNED_POINTER(ptr, align)	((void*)BLOCKIFY((long)(ptr), (align)))


/********************************************************************************
 * FskBitmapNew
 ********************************************************************************/

FskErr FskBitmapNew(SInt32 width, SInt32 height, FskBitmapFormatEnum pixelFormat, FskBitmap *bitsOut)
{
	FskErr err = kFskErrNone;
	UInt32 depth = 0;
	FskBitmap bits = NULL;
#if TARGET_OS_WIN32
	UInt32 colorPad = 0;
#endif /* TARGET_OS_WIN32 */
#if !TARGET_OS_WIN32
	const Boolean wantsNativeBitmap = true;
#else
	Boolean wantsNativeBitmap = false;
#endif

	if (width < 0) {
		width = -width;
#if TARGET_OS_WIN32
		wantsNativeBitmap = true;
#endif
	}

    if (kFskBitmapFormatUnknown == pixelFormat)
        pixelFormat = kFskBitmapFormatDefault;
    else if (kFskBitmapFormatDefaultNoAlpha == pixelFormat)
        pixelFormat = kFskBitmapFormatDefaultRGB;
    else if (kFskBitmapFormatDefaultAlpha == pixelFormat)
        pixelFormat = kFskBitmapFormatDefaultRGBA;

/**************************************** Windows ****************************************/
#if TARGET_OS_WIN32
	if (wantsNativeBitmap) {
		switch (pixelFormat) {
			case kFskBitmapFormat16RGB565LE:
				depth = 16;
				colorPad = 20;
				break;

			case kFskBitmapFormat24BGR:
				depth = 24;
				break;

			case kFskBitmapFormat32ARGB:
			case kFskBitmapFormat32BGRA:
				depth = 32;
				break;

			case kFskBitmapFormat8A:
				depth = 8;
				colorPad = 256 * 4;
				break;
		}
	}

	if (0 != depth) {
		err = FskMemPtrNewClear(sizeof(FskBitmapRecord) + colorPad, (FskMemPtr *)&bits);
		BAIL_IF_ERR(err);

		FskInstrumentedItemNew(bits, NULL, &gBitmapTypeInstrumentation);

		bits->rowBytes = (width * depth) / 8;
		if (bits->rowBytes % 4)
			bits->rowBytes += (4 - (bits->rowBytes % 4));
//		bits->rowBytes = bits->rowBytes;

		bits->bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bits->bmpInfo.bmiHeader.biWidth = width;
		bits->bmpInfo.bmiHeader.biHeight = (SInt32)-height;	// positive to get an upside down DIB (Windows deals with it better...)
		bits->bmpInfo.bmiHeader.biPlanes = 1;
		bits->bmpInfo.bmiHeader.biBitCount = (WORD)depth;
		if (16 == depth) {
			DWORD *colors;
			bits->bmpInfo.bmiHeader.biCompression = BI_BITFIELDS;
			colors = (DWORD *)&bits->bmpInfo.bmiColors;
			colors[0] = 0x1f << 11;
			colors[1] = 0x3f << 5;
			colors[2] = 0x1f;
		}
		else {
			bits->bmpInfo.bmiHeader.biCompression = BI_RGB;

			if (kFskBitmapFormat8A == pixelFormat) {
				RGBQUAD *color = bits->bmpInfo.bmiColors;
				UInt32 i;
				bits->bmpInfo.bmiHeader.biClrUsed = 255;
				for (i=0; i<256; i++, color++) {
					color->rgbRed =
					color->rgbGreen =
					color->rgbBlue = (BYTE)i;
					color->rgbReserved = 0;
				}
			}
		}

		if ((0 != bits->bmpInfo.bmiHeader.biWidth) && (0 != bits->bmpInfo.bmiHeader.biHeight)) {
			bits->hdc = CreateCompatibleDC(NULL);
			bits->hbmp = CreateDIBSection(bits->hdc, &bits->bmpInfo, DIB_RGB_COLORS,
								(void**)&bits->dibBits, 0, 0);
			if (NULL == bits->hbmp) {
				BAIL(kFskErrMemFull);
			}
//			bits->bits = (void *)((-bits->rowBytes * (height - 1)) + (unsigned char *)bits->dibBits);
			bits->bits = (void *)bits->dibBits;

			bits->ext = (void *)SelectObject(bits->hdc, bits->hbmp);
			SetBkMode(bits->hdc, TRANSPARENT);
		}
	}

/**************************************** Macintosh ****************************************/
#elif TARGET_OS_MAC
    switch (pixelFormat) {
        case kFskBitmapFormat32ARGB:
        case kFskBitmapFormat32RGBA:
            depth = 32;
            break;
        //case kFskBitmapFormat16RGB565LE:
        //	depth = 16;	// 565 not supported
        //	break;
        case kFskBitmapFormat8G:
            depth = 8;
            break;

        default:
            break;
    }

    if (0 != depth) {
        err = FskMemPtrNewClear(sizeof(FskBitmapRecord), (FskMemPtr *)&bits);
        BAIL_IF_ERR(err);

        FskInstrumentedItemNew(bits, NULL, &gBitmapTypeInstrumentation);

        if (!FskCocoaBitmapCreate(bits, pixelFormat, width, height)) {
			BAIL(kFskErrMemFull);
        }
    }
#endif


/**************************************** General Case ****************************************/
	if (0 == depth) {
		// fall back case - instantiates bitmap even though host platform does not support the pixel format
		UInt32 pixelsSize = 0;
		SInt32 rowBytes = 0;

		switch (pixelFormat) {
			case kFskBitmapFormat24BGR:
			case kFskBitmapFormat24RGB:
				depth = 24;
				break;

			case kFskBitmapFormat32A16RGB565LE:
			case kFskBitmapFormat32ABGR:
			case kFskBitmapFormat32ARGB:
			case kFskBitmapFormat32BGRA:
			case kFskBitmapFormat32RGBA:
				depth = 32;
				break;

			case kFskBitmapFormat16AG:
			case kFskBitmapFormat16BGR565LE:
			case kFskBitmapFormat16RGB5515LE:
			case kFskBitmapFormat16RGB565BE:
			case kFskBitmapFormat16RGB565LE:
			case kFskBitmapFormat16RGBA4444LE:
			case kFskBitmapFormatYUV422:								// Chunky: UYVY, YUYV, YVYU, or VYUY
				depth = 16;
				break;

			case kFskBitmapFormat8A:
			case kFskBitmapFormat8G:
				depth = 8;
				break;

			/* In the following cases, the rowbytes computation is different than the standard */

			case kFskBitmapFormatYUV420:								// Planar: Y[4n], U[n], V[n]
				depth = 8;												// of Y
				rowBytes = width;										// Row bytes of Y plane
				rowBytes = BLOCKIFY(rowBytes, ALIGN_YUV_ROW_BYTES);		// Assure that rowbytes is a multiple of ALIGN_ROW_BYTES.
				pixelsSize = (rowBytes * height);						// Bytes of y plane
				pixelsSize += (rowBytes * (height + (height & 1))) / 2;	// U & V planes
				break;

			case kFskBitmapFormatYUV420spuv:							// Semi-planar: Y[4n], UV[n]
			case kFskBitmapFormatYUV420spvu:							// Semi-planar: Y[4n], VU[n]
				depth = 8;												// of Y
				rowBytes = width;										// Row bytes of Y plane, and the UV plane
				rowBytes = BLOCKIFY(rowBytes, ALIGN_YUV_ROW_BYTES);		// Assure that rowbytes is a multiple of ALIGN_ROW_BYTES.
				pixelsSize = (rowBytes * height);						// Bytes of y plane
				pixelsSize += rowBytes * ((height + 1) / 2);			// UV plane
				break;

			case kFskBitmapFormatYUV420i:								// Chunky: UVYYYY UVYYYY ...
				depth = 8;												// of Y
				rowBytes = width * 3;									// 2 scan lines (1.5 bytes per pixel)
				rowBytes = BLOCKIFY(rowBytes, ALIGN_YUV_ROW_BYTES);		// Assure that rowbytes is a multiple of ALIGN_ROW_BYTES.
				pixelsSize = rowBytes * ((height + 1) >> 1);			// Since each row contains 2 scanlines, halve the [evenized] height
				break;

			case kFskBitmapFormatUYVY:									//< YUV 422 interleaved - uyvyuyvy...
				depth = 16;												// of Y
				rowBytes = width*2;
				rowBytes = BLOCKIFY(rowBytes, ALIGN_YUV_ROW_BYTES);		// Assure that rowbytes is a multiple of ALIGN_ROW_BYTES.
				pixelsSize = rowBytes * height;
				break;

		#if FSKBITMAP_OPENGL
			case kFskBitmapFormatGLRGB:
			case kFskBitmapFormatGLRGBA:
				depth		= 0;
				rowBytes	= 0;
				pixelsSize	= 0;
				break;
		#endif /* FSKBITMAP_OPENGL */

			default:
				BAIL(kFskErrInvalidParameter);
		}

		if (0 == rowBytes) {
			rowBytes = (depth * width) / 8;
			rowBytes = BLOCKIFY(rowBytes, ALIGN_ROW_BYTES);
		}

		if ((0 == pixelsSize) && (0 != height)) {
			pixelsSize = (UInt32)height * (UInt32)rowBytes;
			BAIL_IF_FALSE((pixelsSize / (UInt32)height) == (UInt32)rowBytes, err, kFskErrMemFull);			/* Check for overflow */
		}

		if (0 != pixelsSize && wantsNativeBitmap) {
			if (kFskErrNone != FskFrameBufferBitmapNew(pixelFormat, pixelsSize, width, height, &bits))
				bits = NULL;
		}

		if (NULL == bits) {
			#if !FSKBITMAP_DOUBLE_ALLOC 																	/* Allocate bitmap with one alloc */
				UInt32 alignedBitmapSize = sizeof(FskBitmapRecord) + ALIGN_PIXEL_BYTES;						/* Add enough pad to align the pixels */
				BAIL_IF_ERR(err = FskMemPtrNew(pixelsSize + alignedBitmapSize, &bits));						/* Allocate bitmap and pixels in one alloc */
				FskMemSet(bits, 0, sizeof(FskBitmapRecord));												/* Init structure */
				if (pixelsSize)
					bits->bits = (char*)ALIGNED_POINTER((char*)bits + sizeof(FskBitmapRecord), ALIGN_PIXEL_BYTES);
			#else /* FSKBITMAP_DOUBLE_ALLOC */																/* Allocate bitmap with 2 allocs */
				BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskBitmapRecord), &bits));						/* The first alloc for the bitmap */
				if (pixelsSize) {
					BAIL_IF_ERR(err = FskMemPtrNew(pixelsSize + ALIGN_PIXEL_BYTES, &bits->bitsToDispose));	/* The second alloc for the pixels */
					bits->bits =(char*)ALIGNED_POINTER((char*)(bits->bitsToDispose), ALIGN_PIXEL_BYTES);
				}
			#endif /* FSKBITMAP_DOUBLE_ALLOC */
			FskInstrumentedItemNew(bits, NULL, &gBitmapTypeInstrumentation);
		}

		if (!bits->rowBytes)
			bits->rowBytes = rowBytes;
		bits->alphaIsPremultiplied = false;
	}

	FskRectangleSet(&bits->bounds, 0, 0, width, height);
	bits->pixelFormat = pixelFormat;
	bits->depth = depth;
	#if FSKBITMAP_OPENGL
		switch (pixelFormat) {
			case kFskBitmapFormatGLRGBA:
			case kFskBitmapFormatGLRGB:
				err = FskGLBitmapTextureSetRenderable(bits, true);
			default:
				break;
		}
	#endif /* FSKBITMAP_OPENGL */

	FskInstrumentedItemSendMessageNormal(bits, kFskBitmapInstrMsgInitialize, bits);

bail:
	if (kFskErrNone != err) {
		FskBitmapDispose(bits);
		bits = NULL;
	}

	*bitsOut = bits;

	return err;
}


/********************************************************************************
 * FskBitmapNewWrapper
 ********************************************************************************/

FskErr FskBitmapNewWrapper(SInt32 width, SInt32 height, FskBitmapFormatEnum pixelFormat, UInt32 bitDepth, void *baseAddr, SInt32 rowBytes, FskBitmap *bitsOut)
{
	FskErr err;
	FskBitmap bits;

	err = FskMemPtrNewClear(sizeof(FskBitmapRecord), &bits);
	BAIL_IF_ERR(err);

	bits->bounds.width = width;
	bits->bounds.height = height;
	bits->depth = bitDepth;
	bits->pixelFormat = pixelFormat;
	bits->rowBytes = rowBytes;
	bits->bits = baseAddr;

	FskInstrumentedItemNew(bits, NULL, &gBitmapTypeInstrumentation);
	FskInstrumentedItemSendMessageNormal(bits, kFskBitmapInstrMsgInitializeWrapper, bits);

	*bitsOut = bits;

bail:
	return err;
}


/********************************************************************************
 * FskBitmapDispose
 ********************************************************************************/

FskErr FskBitmapDispose(FskBitmap bits)
{
	if (NULL != bits) {
#if SUPPORT_INSTRUMENTATION
		if ((0 != bits->readLock) || (0 != bits->writeLock))
			FskInstrumentedItemSendMessageMinimal(bits, kFskBitmapInstrMsgDisposeLocked, (void *)(long)(bits->readLock + bits->writeLock));
#endif /* SUPPORT_INSTRUMENTATION */

		bits->useCount -= 1;
		if (bits->useCount >= 0)
			return kFskErrNone;

#if FSKBITMAP_OPENGL
		if (NULL != bits->glPort)
			FskGLPortDisposeAt(&bits->glPort);
#endif /* FSKBITMAP_OPENGL */

		if (bits->doDispose) {
			(bits->doDispose)(bits, bits->doDisposeRefcon);
			return kFskErrNone;
		}

		if (kFskErrNone != FskFrameBufferBitmapDispose(bits)) {
#if TARGET_OS_WIN32
			if (bits->hbmp && bits->hdc)
				SelectObject(bits->hdc, bits->ext);
			if (NULL != bits->hbmp)
				DeleteObject(bits->hbmp);
			if (NULL != bits->hdc)
				DeleteDC(bits->hdc);
#elif TARGET_OS_MAC
			FskCocoaBitmapDispose(bits);
#endif /* TARGET_OS_LINUX */

			FskMemPtrDispose(bits->bitsToDispose);

#if SUPPORT_INSTRUMENTATION
			FskInstrumentedItemDispose(bits);
			FskMemPtrDispose((void *)FskInstrumentedItemGetName(bits));
#endif /* SUPPORT_INSTRUMENTATION */

			FskMemPtrDispose(bits);
		}
	}

	return kFskErrNone;
}


/********************************************************************************
 * FskBitmapSetOpenGLSysContext
 ********************************************************************************/

FskErr FskBitmapSetOpenGLSysContext(FskBitmap bm, void *sysContext) {
	#if FSKBITMAP_OPENGL
		FskErr	err		= kFskErrNone;

		if (bm == NULL)
			return kFskErrInvalidParameter;

		if (bm->glPort == NULL)
			err = FskGLPortNew(bm->bounds.width, bm->bounds.height, sysContext, &bm->glPort);
		else
			FskGLPortSetSysContext(bm->glPort, sysContext);

		return err;
	#else /* !FSKBITMAP_OPENGL */
		return kFskErrExtensionNotFound;
	#endif /* !FSKBITMAP_OPENGL */
}


/********************************************************************************
 * FskBitmapGetOpenGLSysContext
 ********************************************************************************/

void* FskBitmapGetOpenGLSysContext(FskBitmap bm) {
	#if FSKBITMAP_OPENGL
		if (bm)
			return FskGLPortGetSysContext(bm->glPort);
	#endif /* !FSKBITMAP_OPENGL */
	return NULL;
}


/********************************************************************************
 * FskBitmapMaximumOpenGLDimension
 ********************************************************************************/

SInt32 FskBitmapMaximumOpenGLDimension() {
	#if FSKBITMAP_OPENGL
		return FskGLMaximumTextureDimension();
	#else /* !FSKBITMAP_OPENGL */
		return 0x7FFFFFFF;
	#endif /* FSKBITMAP_OPENGL */
}


/********************************************************************************
 * FskBitmapSetOpenGLSourceAccelerated
 ********************************************************************************/

FskErr	FskBitmapSetOpenGLSourceAccelerated(FskBitmap bm, Boolean accelerated) {
	#if FSKBITMAP_OPENGL
        FskInstrumentedItemSendMessageNormal(bm, kFskBitmapInstrMsgSetAccelerated, (void*)(int)accelerated);
		bm->accelerate = accelerated;
		if (!accelerated) {
			FskGLDeaccelerateBitmapSource(bm);
			bm->accelerateSeed = 0;
		}
		return kFskErrNone;
	#else /* OPENGL */
		return kFskErrExtensionNotFound;
	#endif /* OPENGL */
}


/****************************************************************************//**
 * Dispose local pixels and adjust the parameters of a bitmap after successfully
 * uploading the source pixels to a GL texture.
 *	\param[in,out]	bm	the bitmap whose pixels are to be disposed.
 ********************************************************************************/

#if FSKBITMAP_DOUBLE_ALLOC
static void DoGLSourceDispose(FskBitmap bm) {
	#if TARGET_OS_MAC
		FskCocoaBitmapDispose(bm);
	#endif /* TARGET_OS_MAC */
	FskMemPtrDispose(bm->bitsToDispose);
	bm->bitsToDispose			= NULL;
	bm->bits					= NULL;
	bm->depth					= 0;
	bm->rowBytes				= 0;
	bm->disposeUploadedPixels	= 0;
	if (fskUniformChunkyPixelPacking == FskBitmapFormatPixelPacking(bm->pixelFormat))	/* Indicate that non-YUV ... */
		bm->pixelFormat = kFskBitmapFormatGLRGBA;										/* ... is now GLRGBA */
}
#endif /* FSKBITMAP_DOUBLE_ALLOC */


/********************************************************************************
 * FskBitmapCheckGLSourceAccelerated
 ********************************************************************************/

FskErr	FskBitmapCheckGLSourceAccelerated(FskBitmap bits) {
	#if FSKBITMAP_OPENGL
		FskErr err;
		if (!bits->accelerate)
			return kFskErrNone;								/* Not destined to be accelerated */
        if (0 == bits->rowBytes)
            return kFskErrNone;     //@@ GL-only bitmap. no upload required
		if (bits->accelerateSeed != 0) {
			if (bits->writeSeed == bits->accelerateSeed)
				return kFskErrNone;							/* Already up-to-date as a texture */
			if (kFskErrNone == FskGLUpdateSource(bits)) {	/* Already have a texture, just need to update */
				bits->accelerateSeed = bits->writeSeed;
				#if FSKBITMAP_DOUBLE_ALLOC
					if (bits->disposeUploadedPixels)
						DoGLSourceDispose(bits);
				#endif /* FSKBITMAP_DOUBLE_ALLOC */
				return kFskErrNone;
			}
		}
		err = FskGLAccelerateBitmapSource(bits);			/* Either the bitmap has never been accelerated or FskGLUpdateSource failed */
		if (kFskErrNone == err) {
			bits->accelerateSeed = bits->writeSeed;
			#if FSKBITMAP_DOUBLE_ALLOC
				if (bits->disposeUploadedPixels)
					DoGLSourceDispose(bits);
			#endif /* FSKBITMAP_DOUBLE_ALLOC */
		}
		#if SUPPORT_INSTRUMENTATION
			else
				FskInstrumentedItemPrintfMinimal(bits, "ERROR: CheckGLSourceAccelerated %d", (int)err);
		#endif /* SUPPORT_INSTRUMENTATION */
		return err;
	#else /* OPENGL */
		return kFskErrExtensionNotFound;
	#endif /* OPENGL */
}


/********************************************************************************
 * FskBitmapIsOpenGLSourceAccelerated
 ********************************************************************************/

Boolean FskBitmapIsOpenGLSourceAccelerated(FskConstBitmap bm) {
	#if FSKBITMAP_OPENGL
		return bm->accelerate;
	#else /* OPENGL */
		return false;
	#endif /* OPENGL */
}


/********************************************************************************
 * FskBitmapUpdateGLSource
 ********************************************************************************/

FskAPI(FskErr)	FskBitmapUpdateGLSource(FskConstBitmap bm) {
	#if FSKBITMAP_OPENGL
		return FskGLUpdateSource(bm);
	#else /* OPENGL */
		return kFskErrExtensionNotFound;
	#endif /* OPENGL */
}


/********************************************************************************
 * FskBitmapIsOpenGLDestinationAccelerated
 ********************************************************************************/

Boolean FskBitmapIsOpenGLDestinationAccelerated(FskConstBitmap bm) {
	#if FSKBITMAP_OPENGL
		return bm && (FskGLPortIsDestination(bm->glPort) || (kFskBitmapFormatGLRGBA == bm->pixelFormat) || (kFskBitmapFormatGLRGB == bm->pixelFormat));
	#else /* OPENGL */
		return false;
	#endif /* OPENGL */
}


/********************************************************************************
 * FskBitmapSetSourceDiscardable
 ********************************************************************************/

FskErr	FskBitmapSetSourceDiscardable(FskBitmap bm) {
	#if !FSKBITMAP_DOUBLE_ALLOC
		return kFskErrUnimplemented;
	#elif !FSKBITMAP_OPENGL
		return kFskErrNotAccelerated;
	#else /* FSKBITMAP_DOUBLE_ALLOC && FSKBITMAP_OPENGL */
		if (!bm->accelerate)
			return kFskErrNotAccelerated;
		if (!bm->bits)
			return kFskErrUnsupportedPixelType;
		bm->disposeUploadedPixels = 1;
		return kFskErrNone;
	#endif /* FSKBITMAP_DOUBLE_ALLOC */
}


/********************************************************************************
 * FskBitmapHasLocalPixels
 ********************************************************************************/

Boolean	FskBitmapHasLocalPixels(FskConstBitmap bm) {
	return bm->bits != 0;
}


/********************************************************************************
 * FskBitmapUse
 ********************************************************************************/

FskErr FskBitmapUse(FskBitmap bmp)
{
	bmp->useCount += 1;

	return kFskErrNone;
}

FskErr FskBitmapReadBegin(FskBitmap bmp, const void **bits, SInt32 *rowBytes, FskBitmapFormatEnum *pixelFormat)
{
	if (0 == (bmp->readLock + bmp->writeLock)) {
#if FSKBITMAP_OPENGL
		if (!bmp->glPort)
#endif /* FSKBITMAP_OPENGL */
			(void)FskFrameBufferLockSurface(bmp, NULL, NULL);
	}

	bmp->readLock += 1;
	bmp->useCount += 1;

	FskInstrumentedItemSendMessageVerbose(bmp, kFskBitmapInstrMsgGet, (void *)(long)bmp->readLock);

	if (bits) *bits = bmp->bits;
	if (rowBytes) *rowBytes = bmp->rowBytes;
	if (pixelFormat) *pixelFormat = bmp->pixelFormat;

	return kFskErrNone;
}

FskErr FskBitmapReadEnd(FskBitmap bmp)
{
	bmp->readLock -= 1;

#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemSendMessageVerbose(bmp, kFskBitmapInstrMsgRelease, (void *)(long)bmp->readLock);

	if (bmp->readLock < 0)
		FskInstrumentedItemSendMessageMinimal(bmp, kFskBitmapInstrMsgTooManyUnlocks, (void *)(long)bmp->readLock);
#endif /* SUPPORT_INSTRUMENTATION */

	if (0 == (bmp->readLock + bmp->writeLock)) {
#if FSKBITMAP_OPENGL
		if (!bmp->glPort)
#endif /* FSKBITMAP_OPENGL */
			(void)FskFrameBufferUnlockSurface(bmp);
	}

	bmp->useCount -= 1;
	if (bmp->useCount < 0)
		return FskBitmapDispose(bmp);

	return kFskErrNone;
}

FskErr FskBitmapWriteBegin(FskBitmap bmp, void **bits, SInt32 *rowBytes, FskBitmapFormatEnum *pixelFormat)
{
	if (0 == (bmp->readLock + bmp->writeLock)) {
#if FSKBITMAP_OPENGL
		if (!bmp->glPort)
#endif /* FSKBITMAP_OPENGL */
			(void)FskFrameBufferLockSurface(bmp, NULL, NULL);
	}

	bmp->writeLock += 1;
	bmp->writeSeed += 1;
	bmp->useCount += 1;

	FskInstrumentedItemSendMessageVerbose(bmp, kFskBitmapInstrMsgGet, (void *)(long)bmp->writeLock);

	if (bits) *bits = bmp->bits;
	if (rowBytes) *rowBytes = bmp->rowBytes;
	if (pixelFormat) *pixelFormat = bmp->pixelFormat;

	return kFskErrNone;
}

FskErr FskBitmapWriteEnd(FskBitmap bmp)
{
	bmp->writeLock -= 1;

#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemSendMessageVerbose(bmp, kFskBitmapInstrMsgRelease, (void *)(long)bmp->writeLock);

	if (bmp->writeLock < 0)
		FskInstrumentedItemSendMessageMinimal(bmp, kFskBitmapInstrMsgTooManyUnlocks, (void *)(long)bmp->writeLock);
#endif /* SUPPORT_INSTRUMENTATION */

	if (0 == (bmp->readLock + bmp->writeLock)) {
#if FSKBITMAP_OPENGL
		if (!bmp->glPort)
#endif /* FSKBITMAP_OPENGL */
			(void)FskFrameBufferUnlockSurface(bmp);
	}

	bmp->useCount -= 1;
	if (bmp->useCount < 0)
		return FskBitmapDispose(bmp);

	return kFskErrNone;
}


/********************************************************************************
 * FskBitmapGetDepth
 ********************************************************************************/

FskErr FskBitmapGetDepth(FskConstBitmap bmp, UInt32 *depth)
{
	*depth = bmp->depth;

	return kFskErrNone;
}


/********************************************************************************
 * FskBitmapGetBounds
 ********************************************************************************/

FskErr FskBitmapGetBounds(FskConstBitmap bmp, FskRectangle bounds)
{
	*bounds = bmp->bounds;
	return kFskErrNone;
}


/********************************************************************************
 * FskBitmapGetPixelFormat
 ********************************************************************************/

FskErr FskBitmapGetPixelFormat(FskConstBitmap bmp, FskBitmapFormatEnum *pixelFormat)
{
	*pixelFormat = bmp->pixelFormat;

	return kFskErrNone;
}


/********************************************************************************
 * FskBitmapSetHasAlpha
 ********************************************************************************/

FskErr FskBitmapSetHasAlpha(FskBitmap bmp, Boolean hasAlpha)
{
	bmp->hasAlpha = hasAlpha;
	return kFskErrNone;
}


/********************************************************************************
 * FskBitmapGetHasAlpha
 ********************************************************************************/

FskErr FskBitmapGetHasAlpha(FskConstBitmap bmp, Boolean *hasAlpha)
{
	*hasAlpha = bmp->hasAlpha;
	return kFskErrNone;
}


/********************************************************************************
 * FskBitmapSetAlphaIsPremultiplied
 ********************************************************************************/

FskErr FskBitmapSetAlphaIsPremultiplied(FskBitmap bmp, Boolean isPremultipled)
{
	bmp->alphaIsPremultiplied = isPremultipled;
	return kFskErrNone;
}


/********************************************************************************
 * FskBitmapGetAlphaIsPremultiplied
 ********************************************************************************/

FskErr FskBitmapGetAlphaIsPremultiplied(FskConstBitmap bmp, Boolean *isPremultipled)
{
	*isPremultipled = bmp->hasAlpha && bmp->alphaIsPremultiplied;
	return kFskErrNone;
}


/********************************************************************************
 * FskBitmapGetPixel
 ********************************************************************************/

FskErr FskBitmapGetPixel(FskConstBitmap bmp, SInt32 x, SInt32 y, FskColorRGBA pixel)
{
	FskErr err;
	const unsigned char *bits;
	FskPointRecord p;

	p.x = x;
	p.y = y;
	BAIL_IF_FALSE(FskRectangleContainsPoint(&bmp->bounds, &p), err, kFskErrInvalidParameter);

	err = FskBitmapReadBegin((FskBitmap)bmp, (const void**)(const void*)&bits, NULL, NULL);
	BAIL_IF_ERR(err);

	bits = (const unsigned char *)bmp->bits;
	bits += (y * bmp->rowBytes);
	bits += (x * bmp->depth) >> 3;

	err = FskConvertBitmapPixelToColorRGBA(bits, bmp->pixelFormat, pixel);
	FskBitmapReadEnd((FskBitmap)bmp);
	BAIL_IF_ERR(err);

bail:
	return err;
}


/********************************************************************************
 * FskBitmapGetNativeBitmap
 ********************************************************************************/

void *FskBitmapGetNativeBitmap(FskBitmap bmp)
{
#if TARGET_OS_WIN32
	return bmp->hbmp;
#elif TARGET_OS_MAC
    return bmp->cgBitmapContext;
#else /* !TARGET_OS_WIN32 && !TARGET_OS_MAC */
	return NULL;
#endif /* !TARGET_OS_WIN32 && !TARGET_OS_MAC */
}


#if BG3CDP
/********************************************************************************
 * FskBitmapSetPhysicalAddress
 ********************************************************************************/

FskErr FskBitmapSetPhysicalAddress(FskBitmap bmp, void *addr)
{
    bmp->physicalAddr = addr;
    return kFskErrNone;
}

/********************************************************************************
 * FskBitmapGetPhysicalAddress
 ********************************************************************************/

void *FskBitmapGetPhysicalAddress(FskConstBitmap bmp)
{
    return bmp->physicalAddr;
}
#endif



/********************************************************************************
 * FskBitmapNewFromWindowsBitmap
 ********************************************************************************/

#if TARGET_OS_WIN32
FskErr FskBitmapNewFromWindowsBitmap(FskBitmap *bitsOut, HBITMAP hBitmap)
{
	FskErr err;
	DIBSECTION dibSection;
	int size;
	FskBitmapFormatEnum pixelFormat;
	FskBitmap bits = NULL;

	if (!(size = GetObject(hBitmap, sizeof(DIBSECTION), &dibSection))) {
		BAIL(kFskErrOperationFailed);
	}

	switch (dibSection.dsBm.bmBitsPixel) {
		case 16:
			pixelFormat = kFskBitmapFormat16RGB565LE;
			break;

		case 24:
			pixelFormat = kFskBitmapFormat24BGR;
			break;

		case 32:
			pixelFormat = kFskBitmapFormat32BGRA;
			break;

		case 8:
			pixelFormat = kFskBitmapFormat8A;
			break;

		default:
			BAIL(kFskErrUnsupportedPixelType);		// we don't handle other Windows bitmap formats
			break;
	}

	if (sizeof(DIBSECTION) == size) {
		// it is DIB
		err = FskMemPtrNewClear(sizeof(FskBitmapRecord), (FskMemPtr *)&bits);
		BAIL_IF_ERR(err);

		FskMemCopy(&bits->bmpInfo.bmiHeader, &dibSection.dsBmih, sizeof(BITMAPINFOHEADER));
		bits->bounds.width = dibSection.dsBm.bmWidth;
		bits->bounds.height = dibSection.dsBm.bmHeight;
		bits->rowBytes = -dibSection.dsBm.bmWidthBytes;
		bits->dibBits = dibSection.dsBm.bmBits;
		bits->bits = (void *)((-bits->rowBytes * (bits->bounds.height - 1)) + (unsigned char *)bits->dibBits);
		bits->depth = dibSection.dsBm.bmBitsPixel;
		bits->pixelFormat = pixelFormat;
		bits->hbmp = hBitmap;
	}
	else {
		// it isn't DIB
		HDC srcDC = NULL, destDC = NULL;
		HBITMAP oldSrcBitmap = NULL, oldDestBitmap = NULL;

		if (kFskBitmapFormat32BGRA == pixelFormat)	// temporary workaround
			pixelFormat = kFskBitmapFormat24BGR;

		err = FskBitmapNew(dibSection.dsBm.bmWidth, dibSection.dsBm.bmHeight, pixelFormat, &bits);
		BAIL_IF_ERR(err);

		if (!(srcDC = CreateCompatibleDC(NULL))
			|| !(destDC = CreateCompatibleDC(NULL))
			|| !(oldSrcBitmap = (HBITMAP)SelectObject(srcDC, hBitmap))
			|| !(oldDestBitmap = (HBITMAP)SelectObject(destDC, bits->hbmp))
			|| !BitBlt(destDC, 0, 0, dibSection.dsBm.bmWidth, dibSection.dsBm.bmHeight, srcDC, 0, 0, SRCCOPY))
            err = kFskErrOperationFailed;
		if (srcDC) {
			if (oldSrcBitmap)
                SelectObject(srcDC, oldSrcBitmap);
			DeleteDC(srcDC);
		}
		if (destDC) {
			if (oldDestBitmap)
				SelectObject(destDC, oldDestBitmap);
            DeleteDC(destDC);
		}
		if (!err)
			DeleteObject(hBitmap);
	}

bail:
	*bitsOut = bits;

	return err;
}
#endif /* TARGET_OS_WIN32 */


/********************************************************************************
 * doFormatMessageBitmap
 ********************************************************************************/

#if SUPPORT_INSTRUMENTATION

Boolean doFormatMessageBitmap(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kFskBitmapInstrMsgGet:
			snprintf(buffer, bufferSize, "get, lockCount=%ld", (long)msgData);
			return true;

		case kFskBitmapInstrMsgRelease:
			snprintf(buffer, bufferSize, "release, lockCount=%ld", (long)msgData);
			return true;

		case kFskBitmapInstrMsgDisposeLocked:
			snprintf(buffer, bufferSize, "WARNING: dispose when locked");
			return true;

		case kFskBitmapInstrMsgTooManyUnlocks:
			snprintf(buffer, bufferSize, "WARNING: extra unlock");
			return true;

		case kFskBitmapInstrMsgInitialize: {
			FskBitmap bits = (FskBitmap)msgData;
			snprintf(buffer, bufferSize, "initialize: width=%d, height=%d, pixelFormat=%s, rowBytes=%d, bytes=%ld",
					(int)bits->bounds.width, (int)bits->bounds.height, FskBitmapFormatName(bits->pixelFormat), (int)bits->rowBytes, (long)bits->rowBytes * bits->bounds.height);
			}
			return true;

		case kFskBitmapInstrMsgInitializeWrapper: {
			FskBitmap bits = (FskBitmap)msgData;
			snprintf(buffer, bufferSize, "initialize wrapper: width=%d, height=%d, pixelFormat=%s, rowBytes=%d",
					(int)bits->bounds.width, (int)bits->bounds.height, FskBitmapFormatName(bits->pixelFormat), (int)bits->rowBytes);
			}
			return true;

		case kFskBitmapInstrMsgSetAccelerated:
			snprintf(buffer, bufferSize, "set accelerated %s", msgData ? "true" : "false");
			return true;
	}

	return false;
}

/********************************************************************************
 * FskBitmapIncrementUseCount and FskBitmapDecrementUseCount
 ********************************************************************************/

void FskBitmapIncrementUseCount(FskBitmap bm, const char *file, int line) {
	bm->useCount++;
	FskInstrumentedItemPrintfDebug(bm, "useCount incremented to %d from \"%s\", line %d", (int)(bm->useCount), file, line);
}

void FskBitmapDecrementUseCount(FskBitmap bm, const char *file, int line) {
	if (bm->useCount < 0)
		FskInstrumentedItemPrintfMinimal(bm, "useCount already %d when called from \"%s\", line %d", (int)(bm->useCount), file, line);
	bm->useCount--;
	FskInstrumentedItemPrintfDebug(bm, "useCount decremented to %d from \"%s\", line %d", (int)(bm->useCount), file, line);
}

#endif /* SUPPORT_INSTRUMENTATION */
