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
#ifndef KPR_CONFIG

#define __FSKECMASCRIPT_PRIV__
#define __FSKBITMAP_PRIV__
#include "xs.h"

#include "FskECMAScript.h"
#include "FskEndian.h"
#include "FskYUV422toYUV420.h"

static FskErr YUV422ToYUV420(FskBitmap yuv422, char *formatIn, FskBitmap *yuv);
static FskErr bgraToYUV420(FskBitmap bgra, FskBitmap *yuv);
static FskErr argbToYUV420(FskBitmap argb, FskBitmap *yuv);

void xs_media_reader_bitmap(xsMachine *the)
{
	UInt32 length = xsToInteger(xsGet(xsArg(0), xsID("length")));
	SInt32 width = xsToInteger(xsArg(2));
	SInt32 height = xsToInteger(xsArg(3));
	unsigned char *baseAddr = xsGetHostData(xsArg(0));
	char *formatIn = xsToStringCopy(xsArg(1));
	FskBitmap bits = NULL;
	FskErr err;
	SInt32 argc = xsToInteger(xsArgc);
	UInt32 bitmapFormatIn = 0;

	if ((0 == FskStrCompare(formatIn, "format:iyuv")) || (0 == FskStrCompare(formatIn, "format:y420"))) {
		UInt32 lengthNeeded = ((width * height) * 3) / 2;
		if (lengthNeeded > length)
			FskECMAScriptThrowIf(the, kFskErrInvalidParameter);
		bitmapFormatIn = kFskBitmapFormatYUV420;
		err = FskBitmapNewWrapper(width, height, bitmapFormatIn, 8, baseAddr, width, &bits);
	}
	else
	if (0 == FskStrCompare(formatIn, "format:bgra")) {
		UInt32 lengthNeeded = (width * height) * 4;
		if (lengthNeeded > length)
			FskECMAScriptThrowIf(the, kFskErrInvalidParameter);
		bitmapFormatIn = kFskBitmapFormat32BGRA;
		err = FskBitmapNewWrapper(width, height, bitmapFormatIn, 32, baseAddr, width * 4, &bits);
	}
	else
	if (0 == FskStrCompare(formatIn, "format:argb")) {
		UInt32 lengthNeeded = (width * height) * 4;
		if (lengthNeeded > length)
			FskECMAScriptThrowIf(the, kFskErrInvalidParameter);
		bitmapFormatIn = kFskBitmapFormat32ARGB;
		err = FskBitmapNewWrapper(width, height, bitmapFormatIn, 32, baseAddr, width * 4, &bits);
	}
	else
	if ((0 == FskStrCompare(formatIn, "format:yuv2")) || (0 == FskStrCompare(formatIn, "format:uyvy")) ||
		(0 == FskStrCompare(formatIn, "format:2vuy"))) {
		UInt32 lengthNeeded = ((width * height) * 3) / 2;
		if (lengthNeeded > length)
			FskECMAScriptThrowIf(the, kFskErrInvalidParameter);
		bitmapFormatIn = kFskBitmapFormatYUV422;
		err = FskBitmapNewWrapper(width, height, bitmapFormatIn, 8, baseAddr, width, &bits);
	}
	else
		err = kFskErrUnsupportedPixelType;
	BAIL_IF_ERR(err);

	xsSetHostData(xsArg(0), NULL);		// we ate the chunk
	bits->bitsToDispose = baseAddr;		// dispose the pixels when the bitmap is disposed

	if (argc > 4) {
		char *formatOut = xsToString(xsArg(4));
		if (0 != FskStrCompare(formatIn, formatOut)) {
			if (0 == FskStrCompare(formatOut, "format:iyuv")) {
				FskBitmap converted = NULL;
				switch (bitmapFormatIn)
				{
					case kFskBitmapFormatYUV420:
						break;
					case kFskBitmapFormatYUV422:
						err = YUV422ToYUV420(bits, formatIn, &converted);
						break;
					case kFskBitmapFormat32BGRA:
						err = bgraToYUV420(bits, &converted);
						break;
					case kFskBitmapFormat32ARGB:
						err = argbToYUV420(bits, &converted);
						break;
					default:
						err = kFskErrUnimplemented;
						break;
				}
				BAIL_IF_ERR(err);

				if (converted) {
					FskBitmapDispose(bits);
					bits = converted;
				}
			}
		}
	}

	fskBitmapToXSBitmap(the, bits, true, &xsResult);
bail:
	FskMemPtrDispose(formatIn);
	FskECMAScriptThrowIf(the, err);
}

FskErr YUV422ToYUV420(FskBitmap yuv422, char *formatIn, FskBitmap *yuv)
{
	FskErr 				err = kFskErrNone;
	FskRectangleRecord 	bounds;
	SInt32 				width, height, srcRowBytes, dstYRowBytes, dstUVRowBytes;
	UInt8 				*dstBits, *dstY, *dstU, *dstV;
	UInt32				*srcBits;

	FskBitmapGetBounds(yuv422, &bounds);
	width = bounds.width;
	height = bounds.height;

	err = FskBitmapNew(width, height, kFskBitmapFormatYUV420, yuv);
	if (err) return err;

	FskBitmapReadBegin(yuv422, (void *)&srcBits, NULL, NULL);
	FskBitmapWriteBegin(*yuv, (void *)&dstBits, NULL, NULL);

	srcRowBytes = width * 2;
	dstYRowBytes = width;
	dstUVRowBytes = width / 2;
	dstY = dstBits;
	dstU = dstBits + dstYRowBytes * height;
	dstV = dstU + dstUVRowBytes * height / 2;

	// YUV2/UYVY/2VUY 4.2.2 -> YUV 4.2.0
	if (!strcmp(formatIn, "format:yuv2"))
		FskSignedYUYV422toMPEG4YUV420(width, height, srcBits, srcRowBytes, dstY, dstYRowBytes, dstU, dstV, dstUVRowBytes);
	else if (!strcmp(formatIn, "format:uyvy"))
		FskSignedUYVY422toMPEG4YUV420(width, height, srcBits, srcRowBytes, dstY, dstYRowBytes, dstU, dstV, dstUVRowBytes);
	else if (!strcmp(formatIn, "format:2vuy"))
		FskUYVY422toMPEG4YUV420(width, height, srcBits, srcRowBytes, dstY, dstYRowBytes, dstU, dstV, dstUVRowBytes);

	FskBitmapReadEnd(yuv422);
	FskBitmapWriteEnd(*yuv);

	return kFskErrNone;
}

FskErr bgraToYUV420(FskBitmap bgra, FskBitmap *yuv)
{
	FskErr err;
	const unsigned char *rgb0, *rgb1;
	unsigned char *y0Out, *y1Out, *uOut, *vOut;
	SInt32 yRowBytes, width, height, ySize, bgraRowBytes;
	const int kScaler = 256;
	FskRectangleRecord bounds;

	FskBitmapGetBounds(bgra, &bounds);
	err = FskBitmapNew(bounds.width, bounds.height, kFskBitmapFormatYUV420, yuv);
	if (err) return err;

	FskBitmapWriteBegin(*yuv, (void**)(void*)(&y0Out), &yRowBytes, NULL);

	y1Out = y0Out + yRowBytes;
	ySize = yRowBytes * ((bounds.height + 1) & ~1);
	uOut = y0Out + ySize;
	vOut = uOut + ySize / 4;
	width = bounds.width / 2;
	height = bounds.height / 2;

	FskBitmapReadBegin(bgra, (const void**)(const void*)&rgb0, &bgraRowBytes, NULL);

	rgb1 = rgb0 + bgraRowBytes;

	while (height--) {
		UInt32 w = width;
		while (w--) {
			UInt32 pixel[4];
			SInt32 r, b, g;
			SInt32 y, u, v;
			double yd;

			pixel[0] = *(UInt32 *)&rgb0[0];
			pixel[1] = *(UInt32 *)&rgb0[4];
			pixel[2] = *(UInt32 *)&rgb1[0];
			pixel[3] = *(UInt32 *)&rgb1[4];

#if TARGET_OS_MAC
			pixel[0] = FskEndianU32_LtoN(pixel[0]);
			pixel[1] = FskEndianU32_LtoN(pixel[1]);
			pixel[2] = FskEndianU32_LtoN(pixel[2]);
			pixel[3] = FskEndianU32_LtoN(pixel[3]);
#endif

			r = ((((pixel[0] >> 16) & 0x0ff) + ((pixel[1] >> 16) & 0x0ff) +((pixel[2] >> 16) & 0x0ff) +((pixel[3] >> 16) & 0x0ff)) / 4) * kScaler;
			g = ((((pixel[0] >> 8) & 0x0ff) + ((pixel[1] >> 8) & 0x0ff) +((pixel[2] >> 8) & 0x0ff) +((pixel[3] >> 8) & 0x0ff)) / 4) * kScaler;
			b = ((((pixel[0] >> 0) & 0x0ff) + ((pixel[1] >> 0) & 0x0ff) +((pixel[2] >> 0) & 0x0ff) +((pixel[3] >> 0) & 0x0ff)) / 4) * kScaler;

			yd = (g + (0.1934 * b) + (0.509 * r)) / 507.2;
//			y = (SInt32)yd;
			yd *= 298.0;
			u = (SInt32)((b - yd) / 517.0);
			v = (SInt32)((r - yd) / 409.0);
			u += 128;
			v += 128;
			if (u > 255) u = 255; else if (u < 0) u = 0;
			if (v > 255) v = 255; else if (v < 0) v = 0;
			*uOut++ = (unsigned char)u;
			*vOut++ = (unsigned char)v;

			r = (pixel[0] >> 16) & 0x0ff; g = (pixel[0] >> 8) & 0x0ff; b = (pixel[0] >> 0) & 0x0ff;
			y = (SInt32)(((g + (0.1934 * b) + (0.509 * r)) / 507.2) * kScaler);
			y += 16;
			if (y > 255) y = 255;
			else if (y < 0) y = 0;
			*y0Out++ = (unsigned char)y;

			r = (pixel[2] >> 16) & 0x0ff; g = (pixel[2] >> 8) & 0x0ff; b = (pixel[2] >> 0) & 0x0ff;
			y = (SInt32)(((g + (0.1934 * b) + (0.509 * r)) / 507.2) * kScaler);
			y += 16;
			if (y > 255) y = 255;
			else if (y < 0) y = 0;
			*y1Out++ = (unsigned char)y;

			r = (pixel[1] >> 16) & 0x0ff; g = (pixel[1] >> 8) & 0x0ff; b = (pixel[1] >> 0) & 0x0ff;
			y = (SInt32)(((g + (0.1934 * b) + (0.509 * r)) / 507.2) * kScaler);
			y += 16;
			if (y > 255) y = 255;
			else if (y < 0) y = 0;
			*y0Out++ = (unsigned char)y;

			r = (pixel[3] >> 16) & 0x0ff; g = (pixel[3] >> 8) & 0x0ff; b = (pixel[3] >> 0) & 0x0ff;
			y = (SInt32)(((g + (0.1934 * b) + (0.509 * r)) / 507.2) * kScaler);
			y += 16;
			if (y > 255) y = 255;
			else if (y < 0) y = 0;
			*y1Out++ = (unsigned char)y;

			rgb0 += 8;
			rgb1 += 8;
		}

		y0Out += yRowBytes * 2 - width * 2;
		y1Out += yRowBytes * 2 - width * 2;
		uOut += (yRowBytes / 2) - width;
		vOut += (yRowBytes / 2) - width;
		rgb0 += (bgraRowBytes * 2) - (width * 8);
		rgb1 += (bgraRowBytes * 2) - (width * 8);
	}

	FskBitmapReadEnd(bgra);
	FskBitmapWriteEnd(*yuv);

	return kFskErrNone;
}

FskErr argbToYUV420(FskBitmap argb, FskBitmap *yuv)
{
	FskErr err;
	const unsigned char *rgb0, *rgb1;
	unsigned char *y0Out, *y1Out, *uOut, *vOut;
	SInt32 yRowBytes, width, height, ySize, argbRowBytes;
	const int kScaler = 256;
	FskRectangleRecord bounds;

	FskBitmapGetBounds(argb, &bounds);
	err = FskBitmapNew(bounds.width, bounds.height, kFskBitmapFormatYUV420, yuv);
	if (err) return err;

	FskBitmapWriteBegin(*yuv, (void**)(void*)(&y0Out), &yRowBytes, NULL);

	y1Out = y0Out + yRowBytes;
	ySize = yRowBytes * ((bounds.height + 1) & ~1);
	uOut = y0Out + ySize;
	vOut = uOut + ySize / 4;
	width = bounds.width / 2;
	height = bounds.height / 2;

	FskBitmapReadBegin(argb, (const void**)(const void*)&rgb0, &argbRowBytes, NULL);

	rgb1 = rgb0 + argbRowBytes;

	while (height--) {
		UInt32 w = width;
		while (w--) {
			UInt32 pixel[4];
			SInt32 r, b, g;
			SInt32 y, u, v;
			double yd;

			pixel[0] = *(UInt32 *)&rgb0[0];
			pixel[1] = *(UInt32 *)&rgb0[4];
			pixel[2] = *(UInt32 *)&rgb1[0];
			pixel[3] = *(UInt32 *)&rgb1[4];

#if TARGET_OS_MAC
			pixel[0] = FskEndianU32_LtoN(pixel[0]);
			pixel[1] = FskEndianU32_LtoN(pixel[1]);
			pixel[2] = FskEndianU32_LtoN(pixel[2]);
			pixel[3] = FskEndianU32_LtoN(pixel[3]);
#endif

			r = ((((pixel[0] >> 8) & 0x0ff) + ((pixel[1] >> 8) & 0x0ff) +((pixel[2] >> 8) & 0x0ff) +((pixel[3] >> 8) & 0x0ff)) / 4) * kScaler;
			g = ((((pixel[0] >> 16) & 0x0ff) + ((pixel[1] >> 16) & 0x0ff) +((pixel[2] >> 16) & 0x0ff) +((pixel[3] >> 16) & 0x0ff)) / 4) * kScaler;
			b = ((((pixel[0] >> 24) & 0x0ff) + ((pixel[1] >> 24) & 0x0ff) +((pixel[2] >> 24) & 0x0ff) +((pixel[3] >> 24) & 0x0ff)) / 4) * kScaler;

			yd = (g + (0.1934 * b) + (0.509 * r)) / 507.2;
//			y = (SInt32)yd;
			yd *= 298.0;
			u = (SInt32)((b - yd) / 517.0);
			v = (SInt32)((r - yd) / 409.0);
			u += 128;
			v += 128;
			if (u > 255) u = 255; else if (u < 0) u = 0;
			if (v > 255) v = 255; else if (v < 0) v = 0;
			*uOut++ = (unsigned char)u;
			*vOut++ = (unsigned char)v;

			r = (pixel[0] >> 8) & 0x0ff; g = (pixel[0] >> 16) & 0x0ff; b = (pixel[0] >> 24) & 0x0ff;
			y = (SInt32)(((g + (0.1934 * b) + (0.509 * r)) / 507.2) * kScaler);
			y += 16;
			if (y > 255) y = 255;
			else if (y < 0) y = 0;
			*y0Out++ = (unsigned char)y;

			r = (pixel[2] >> 8) & 0x0ff; g = (pixel[2] >> 16) & 0x0ff; b = (pixel[2] >> 24) & 0x0ff;
			y = (SInt32)(((g + (0.1934 * b) + (0.509 * r)) / 507.2) * kScaler);
			y += 16;
			if (y > 255) y = 255;
			else if (y < 0) y = 0;
			*y1Out++ = (unsigned char)y;

			r = (pixel[1] >> 8) & 0x0ff; g = (pixel[1] >> 16) & 0x0ff; b = (pixel[1] >> 24) & 0x0ff;
			y = (SInt32)(((g + (0.1934 * b) + (0.509 * r)) / 507.2) * kScaler);
			y += 16;
			if (y > 255) y = 255;
			else if (y < 0) y = 0;
			*y0Out++ = (unsigned char)y;

			r = (pixel[3] >> 8) & 0x0ff; g = (pixel[3] >> 16) & 0x0ff; b = (pixel[3] >> 24) & 0x0ff;
			y = (SInt32)(((g + (0.1934 * b) + (0.509 * r)) / 507.2) * kScaler);
			y += 16;
			if (y > 255) y = 255;
			else if (y < 0) y = 0;
			*y1Out++ = (unsigned char)y;

			rgb0 += 8;
			rgb1 += 8;
		}

		y0Out += yRowBytes * 2 - width * 2;
		y1Out += yRowBytes * 2 - width * 2;
		uOut += (yRowBytes / 2) - width;
		vOut += (yRowBytes / 2) - width;
		rgb0 += (argbRowBytes * 2) - (width * 8);
		rgb1 += (argbRowBytes * 2) - (width * 8);
	}

	FskBitmapReadEnd(argb);
	FskBitmapWriteEnd(*yuv);

	return kFskErrNone;
}
#endif /* KPR_CONFIG */
