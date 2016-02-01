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
#include "KplScreen.h"
#include "FskBitmap.h"
#include <Windows.h>

// @@ This file just simulates a screen device
#define kScreenWidth 320
//#define kScreenWidth 720
#define kScreenHeight 240
//#define kScreenHeight 460
#define kScreenRowBytes 1280
//#define kScreenRowBytes 2880
#define kScreenPixelFormat kFskBitmapFormat32BGRA
#define kScreenDepth 32

#define BITS_NULL	((void *)1)	// to make sure the framework crashes if the screen bitmap bits are accessed without locking

static KplBitmapRecord gFrameBuffer = {0};
static FskBitmap gScreenBufferBitmap = NULL;
static HBITMAP ghBmp = NULL;
static HDC ghDC = NULL;

static FskErr buildFrameBuffer(void);

FskErr KplScreenGetBitmap(KplBitmap *bitmap)
{
	FskErr err = kFskErrNone;
	
	// Build the screen frame buffer bitmap on the first request
	if (NULL == gScreenBufferBitmap) {
		err = buildFrameBuffer();
		if (err) goto bail;
	}
	
	*bitmap = &gFrameBuffer;
	
bail:
	return err;
}

FskErr KplScreenLockBitmap(KplBitmap bitmap)
{
	if (bitmap != &gFrameBuffer)
		return kFskErrOperationFailed;
		
	gFrameBuffer.baseAddress = gScreenBufferBitmap->bits;
	gFrameBuffer.rowBytes = gScreenBufferBitmap->rowBytes;
	
	return kFskErrNone;
}

FskErr KplScreenUnlockBitmap(KplBitmap bitmap)
{
	if (bitmap != &gFrameBuffer)
		return kFskErrOperationFailed;
		
	gFrameBuffer.baseAddress = BITS_NULL;

	return kFskErrNone;
}

FskErr KplScreenDisposeBitmap(KplBitmap bitmap)
{
	return kFskErrNone;
}

FskErr KplScreenGetAuxInfo(unsigned char **auxInfo, UInt32 *auxInfoSize)
{
	if (auxInfo && *auxInfo)
		*auxInfo = (unsigned char*)ghDC;
	if (auxInfoSize)
		*auxInfoSize = sizeof(HDC);
		
	return kFskErrNone;
}

FskErr buildFrameBuffer(void)
{
	FskErr err;
	UInt32 colorPad = 0;
	BITMAPINFO bmpInfo = {0};
	UInt32 width = kScreenWidth;
	UInt32 height = kScreenHeight;
	UInt32 depth = kScreenDepth;
	UInt32 pixelFormat = kScreenPixelFormat;
	UInt32 rowBytes = kScreenRowBytes;
		
	err = FskBitmapNewWrapper(width, height, pixelFormat, depth, NULL, rowBytes, &gScreenBufferBitmap);
	if (err) goto bail;

	switch (pixelFormat) {
		case kFskBitmapFormat16RGB565LE:
			colorPad = 20;
			break;
		case kFskBitmapFormat8A:
			colorPad = 256 * 4;
			break;
		default:
			break;
	}
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth = width;
	bmpInfo.bmiHeader.biHeight = -(SInt32)height;	// positive to get an upside down DIB (Windows deals with it better...)
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = (WORD)depth;
	if (16 == depth) {
		DWORD *colors;
		bmpInfo.bmiHeader.biCompression = BI_BITFIELDS;
		colors = (DWORD *)&bmpInfo.bmiColors;
		colors[0] = 0x1f << 11;
		colors[1] = 0x3f << 5;
		colors[2] = 0x1f;
	}
	else {
		bmpInfo.bmiHeader.biCompression = BI_RGB;

		if (kFskBitmapFormat8A == pixelFormat) {
			RGBQUAD *color = bmpInfo.bmiColors;
			UInt32 i;
			bmpInfo.bmiHeader.biClrUsed = 255;
			for (i=0; i<256; i++, color++) {
				color->rgbRed =
				color->rgbGreen =
				color->rgbBlue = (BYTE)i;
				color->rgbReserved = 0;
			}
		}
	}

	ghDC = CreateCompatibleDC(NULL);
	ghBmp = CreateDIBSection(ghDC, &bmpInfo, DIB_RGB_COLORS, (void**)&gScreenBufferBitmap->bits, 0, 0);
	
	SelectObject(ghDC, ghBmp);
	SetBkMode(ghDC, TRANSPARENT);
	
	gFrameBuffer.baseAddress = BITS_NULL;
	gFrameBuffer.rowBytes = rowBytes;
	gFrameBuffer.pixelFormat = pixelFormat;
	gFrameBuffer.depth = depth;
	gFrameBuffer.width = width;
	gFrameBuffer.height = height;
	
bail:
	return err;
}
