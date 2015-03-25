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
#define __FSKBITMAP_PRIV__	/* To get access to the FskBitmap data structure */

#include "FskSpan.h"
#include "FskPixelOps.h"
#include "FskMemory.h"
#include "FskPolygon.h"

#define UNUSED(x)				(void)(x)




/*******************************************************************************
 * FlatFillSpan8
 *******************************************************************************/

static void
FlatFillSpan8(FskSpan *span)
{
	register UInt32	dx;
	register UInt8	*p;

	if (span->setPixel == NULL) {											/* We can write directly into the frame buffer */
		register UInt8	color	= span->fillColor.p8;
		for (dx = span->dx, p = (UInt8*)(span->p); dx--; )
			*p++ = color;
	}
	else {																	/* We need to call setPixel() */
		for (dx = span->dx, p = (UInt8*)(span->p); dx--; span->p = ++p)
			span->setPixel(span);
	}
}


/*******************************************************************************
 * FlatFillSpan16
 *******************************************************************************/

static void
FlatFillSpan16(FskSpan *span)
{
	register UInt32	dx;
	register UInt16	*p;

	if (span->setPixel == NULL) {											/* We can write directly into the frame buffer */
		register UInt16	color	= span->fillColor.p16;
		for (dx = span->dx, p = (UInt16*)(span->p); dx--; )
			*p++ = color;
	}
	else {																	/* We need to call setPixel() */
		for (dx = span->dx, p = (UInt16*)(span->p); dx--; span->p = ++p)
			span->setPixel(span);
	}
}


/*******************************************************************************
 * FlatFillSpan24
 *******************************************************************************/

static void
FlatFillSpan24(FskSpan *span)
{
	register UInt32	dx;
	register UInt8	*p;

	if (span->setPixel == NULL) {											/* We can write directly into the frame buffer */
		for (dx = span->dx, p = (UInt8*)(span->p); dx--; ) {
			*p++ = span->fillColor.p24.c[0];
			*p++ = span->fillColor.p24.c[1];
			*p++ = span->fillColor.p24.c[2];
		}
	}
	else {																	/* We need to call setPixel() */
		for (dx = span->dx, p = (UInt8*)(span->p); dx--; span->p = p += 3)
			span->setPixel(span);
	}
}


/*******************************************************************************
 * FlatFillSpan32
 *******************************************************************************/

static void
FlatFillSpan32(FskSpan *span)
{
	register UInt32	dx;
	register UInt32	*p;

	if (span->setPixel == NULL) {											/* We can write directly into the frame buffer */
		register UInt32	color	= span->fillColor.p32;
		for (dx = span->dx, p = (UInt32*)(span->p); dx--; )
			*p++ = color;
	}
	else {																	/* We need to call setPixel() */
		for (dx = span->dx, p = (UInt32*)(span->p); dx--; span->p = ++p)
			span->setPixel(span);
	}
}


/*******************************************************************************
 * FlatAlphaBlendFillSpan16
 *******************************************************************************/

static void
FlatAlphaBlendFillSpan16(FskSpan *span)
{
	FskPixelType	fillColor	= span->fillColor;							/* Save the fill color */
	UInt8			alpha		= span->fillColor.b4[2];					/* We store alpha after the 16 bit color (third byte) by convention */
	UInt32			dx;
	UInt16			*p;

	for (dx = span->dx, p = (UInt16*)(span->p); dx--; span->p = ++p) {
		span->blendPixel(span, alpha);
		span->fillColor = fillColor;										/* Fill color is trashed by blendPixel(): restore it */
	}
}


/*******************************************************************************
 * FlatAlphaFillSpan16RGB565SE
 *******************************************************************************/

static void
FlatAlphaFillSpan16RGB565SE(FskSpan *span)
{
	if (span->blendPixel == NULL) {											/* We can write directly into the frame buffer */
		UInt16	color	= span->fillColor.p16;
		UInt8	alpha	= span->fillColor.b4[2];							/* We store alpha after the 16 bit color (third byte) by convention */
		UInt32	dx;
		UInt16	*p;

		for (dx = span->dx, p = (UInt16*)(span->p); dx--; p++)
			FskBlend565SE(p, color, alpha);
	}
	else {																	/* We need to call a blendPixel() function to write into the frame buffer */
		FlatAlphaBlendFillSpan16(span);
	}
}
#define FlatAlphaFillSpan16BGR565SE FlatAlphaFillSpan16RGB565SE


/*******************************************************************************
 * FlatAlphaFillSpan16RGB565DE
 *******************************************************************************/

static void
FlatAlphaFillSpan16RGB565DE(FskSpan *span)
{
	if (span->blendPixel == NULL) {											/* We can write directly into the frame buffer */
		UInt16	color	= span->fillColor.p16;
		UInt8	alpha	= span->fillColor.b4[2];							/* We store alpha after the 16 bit color (third byte) by convention */
		UInt32	dx;
		UInt16	*p;

		for (dx = span->dx, p = (UInt16*)(span->p); dx--; p++)
			FskBlend565DE(p, color, alpha);
	}
	else {																	/* We need to call a blendPixel() function to write into the frame buffer */
		FlatAlphaBlendFillSpan16(span);
	}
}
#define FlatAlphaFillSpan16BGR565DE FlatAlphaFillSpan16RGB565DE


/*******************************************************************************
 * FlatAlphaFillSpan16RGB5515SE
 *******************************************************************************/

#if 0
static void
FlatAlphaFillSpan16RGB5515SE(FskSpan *span)
{
	if (span->blendPixel == NULL) {											/* We can write directly into the frame buffer */
		UInt16	color	= span->fillColor.p16;
		UInt8	alpha	= span->fillColor.b4[2];							/* We store alpha after the 16 bit color (third byte) by convention */
		UInt32	dx;
		UInt16	*p;

		for (dx = span->dx, p = (UInt16*)(span->p); dx--; p++)
			FskBlend5515SE(p, color, alpha);
	}
	else {																	/* We need to call a blendPixel() function to write into the frame buffer */
		FlatAlphaBlendFillSpan16(span);
	}
}
#endif /* 0 */


/*******************************************************************************
 * FlatAlphaFillSpan16RGBA4444SE
 *******************************************************************************/

#if 0
static void
FlatAlphaFillSpan16RGBA4444SE(FskSpan *span)
{
	if (span->blendPixel == NULL) {											/* We can write directly into the frame buffer */
		UInt16	color	= span->fillColor.p16;
		UInt8	alpha	= span->fillColor.b4[2];							/* We store alpha after the 16 bit color (third byte) by convention */
		UInt32	dx;
		UInt16	*p;

		for (dx = span->dx, p = (UInt16*)(span->p); dx--; p++)
			FskBlend4444(p, color, alpha);
	}
	else {																	/* We need to call a blendPixel() function to write into the frame buffer */
		FlatAlphaBlendFillSpan16(span);
	}
}
#endif /*0 */


/*******************************************************************************
 * FlatAlphaBlendFillSpan8
 *******************************************************************************/

static void
FlatAlphaBlendFillSpan8(FskSpan *span)
{
	FskPixelType	fillColor	= span->fillColor;							/* Save the fill color */
	UInt8			alpha		= span->fillColor.b4[2];					/* We store alpha after the 16 bit color (third byte) by convention */
	UInt32			dx;
	UInt8			*p;

	for (dx = span->dx, p = (UInt8*)(span->p); dx--; span->p = ++p) {
		span->blendPixel(span, alpha);
		span->fillColor = fillColor;										/* Fill color is trashed by blendPixel(): restore it */
	}
}



/*******************************************************************************
 * FlatAlphaFillSpan8G
 *******************************************************************************/

static void
FlatAlphaFillSpan8G(FskSpan *span)
{
	if (span->blendPixel == NULL) {											/* We can write directly into the frame buffer */
		UInt8	color	= span->fillColor.b4[0];
		UInt8	alpha	= span->fillColor.b4[1];							/* We store alpha after the 8 bit color (second byte) by convention */
		UInt32	dx;
		UInt8	*p;

		for (dx = span->dx, p = (UInt8*)(span->p); dx--; p++)
			FskBlend8(p, color, alpha);
	}
	else {																	/* We need to call a blendPixel() function to write into the frame buffer */
		FlatAlphaBlendFillSpan8(span);
	}
}


#if 0
/*******************************************************************************
 * FlatAlphaFillSpan16RGB5515DE
 *******************************************************************************/

static void
FlatAlphaFillSpan16RGB5515DE(FskSpan *span)
{
	if (span->blendPixel == NULL) {											/* We can write directly into the frame buffer */
		UInt16	color	= span->fillColor.p16;
		UInt8	alpha	= span->fillColor.b4[2];							/* We store alpha after the 16 bit color by convention */
		UInt32	dx;
		UInt16	*p;
		for (dx = span->dx, p = (UInt16*)(span->p); dx--; p++)
			FskBlend5515DE(p, color, alpha);
	}
	else {																	/* We need to call a blendPixel() function to write into the frame buffer */
		FlatAlphaBlendFillSpan16(span);
	}
}
#endif /* 0 */


/*******************************************************************************
 * FlatAlphaFillSpan24
 *******************************************************************************/

static void
FlatAlphaFillSpan24(FskSpan *span)
{
	UInt8	alpha	= span->fillColor.b4[3];								/* We store alpha after the 24 bit color by convention */
	UInt32	dx;
	UInt8	*p;

	if (span->blendPixel == NULL) {											/* We can write directly into the frame buffer */
		UInt32			dx;
		UInt8			*p;
		Fsk24BitType	color	= span->fillColor.p24;
		for (dx = span->dx, p = (UInt8*)(span->p); dx--; p += 3)
			FskBlend24((Fsk24BitType*)p, color, alpha);
	}
	else {																	/* We need to call a blendPixel() function to write into the frame buffer */
		FskPixelType fillColor = span->fillColor;							/* Save the fill color */
		for (dx = span->dx, p = (UInt8*)(span->p); dx--; span->p = p += 3) {
			span->blendPixel(span, alpha);
			span->fillColor = fillColor;									/* Fill color is trashed in the blend process: restore it */
		}
	}
}
#define FlatAlphaFillSpan24BGR	FlatAlphaFillSpan24
#define FlatAlphaFillSpan24RGB	FlatAlphaFillSpan24


/*******************************************************************************
 * FlatAlphaBlendFillSpan32
 *******************************************************************************/

static void
FlatAlphaBlendFillSpan32(FskSpan *span, UInt8 alpha)
{
	FskPixelType	fillColor = span->fillColor;							/* Save the fill color */
	UInt32			dx;
	UInt32			*p;
	for (dx = span->dx, p = (UInt32*)(span->p); dx--; span->p = ++p) {
		span->blendPixel(span, alpha);
		span->fillColor = fillColor;										/* Fill color is trashed in the blend process: restore it */
	}
}


/*******************************************************************************
 * FlatAlphaFillSpanA32 - alpha in the MS byte
 *******************************************************************************/

static void
FlatAlphaFillSpanA32(FskSpan *span)
{
	UInt32	color	= span->fillColor.p32;
	UInt8	alpha	= (UInt8)(color >> 24);									/* Alpha is in the MS byte for this pixel format */

	if (span->blendPixel == NULL) {											/* We can write directly into the frame buffer */
		UInt32	dx, *p;
		for (dx = span->dx, p = (UInt32*)(span->p); dx--; p++)
			FskAlphaA32(p, color);
	}
	else {																	/* We need to call a blendPixel() function to write into the frame buffer */
		FlatAlphaBlendFillSpan32(span, alpha);
	}
}
#define FlatAlphaFillSpan32ABGR FlatAlphaFillSpanA32
#define FlatAlphaFillSpan32ARGB FlatAlphaFillSpanA32


/*******************************************************************************
 * FlatAlphaFillSpan32A - alpha in the LS byte
 *******************************************************************************/

static void
FlatAlphaFillSpan32A(FskSpan *span)
{
	UInt32	color	= span->fillColor.p32;
	UInt8	alpha	= (UInt8)(color >> 0);									/* Alpha is in the LS byte for this pixel format */

	if (span->blendPixel == NULL) {											/* We can write directly into the frame buffer */
		UInt32	dx, *p;
		for (dx = span->dx, p = (UInt32*)(span->p)	; dx--; p++)
			FskAlpha32A(p, color);
	}
	else {																	/* We need to call a blendPixel() function to write into the frame buffer */
		FlatAlphaBlendFillSpan32(span, alpha);
	}
}
#define FlatAlphaFillSpan32RGBA FlatAlphaFillSpan32A
#define FlatAlphaFillSpan32BGRA FlatAlphaFillSpan32A


/*******************************************************************************
 * FskInitSpan
 *******************************************************************************/

void
FskInitSpan(FskSpan *span, FskBitmap dstBM, UInt32 baseEdgeBytes)
{
	FskMemSet(span, 0, sizeof(FskSpan));

	span->rowBytes			= dstBM->rowBytes;
	span->pixelBytes		= dstBM->depth >> 3;
	span->baseAddr			= (void*)((char*)(dstBM->bits) - dstBM->bounds.y * dstBM->rowBytes - dstBM->bounds.x * (dstBM->depth >> 3));

	span->edgeBytes			= baseEdgeBytes;								/* Make sure edges are on quadByte boundaries */

	switch (span->pixelBytes) {												/* Initialize for sanity */
		case 1:	span->fill	= FlatFillSpan8;	break;
		case 2:	span->fill	= FlatFillSpan16;	break;
		case 3:	span->fill	= FlatFillSpan24;	break;
		case 4:	span->fill	= FlatFillSpan32;	break;
	}

	span->set				= NULL;
	span->initEdge			= NULL;
	span->advanceEdge		= NULL;
}


#if TARGET_OS_MAC /* Keep this in, to allow Peter's configuration to keep compiling. */
	#define FlatAlphaFillSpan16RGB5515DE	FlatAlphaFillSpan16RGB5515SE
	#define FlatAlphaFillSpan16RGBA4444DE	FlatAlphaFillSpan16RGBA4444SE
#endif /* TARGET_OS_MAC */


/*******************************************************************************
 * FskInitSolidColorSpanFromColor
 *******************************************************************************/

FskErr
FskInitSolidColorSpanFromColor(FskSpan *span, FskBitmap dstBM, const FskFixedMatrix3x2 *M, UInt32 quality, FskConstColorRGBA color)
{
	FskErr						err;
	UNUSED(M);
	UNUSED(quality);

	err = FskConvertColorRGBAToBitmapPixel(color, dstBM->pixelFormat, &span->fillColor);		/* Convert color */

	span->fill = NULL;
	if (color->a == 255) {
		switch (dstBM->depth >> 3) {
			case 1:	span->fill = FlatFillSpan8;		break;
			case 2:	span->fill = FlatFillSpan16;	break;
			case 3:	span->fill = FlatFillSpan24;	break;
			case 4:	span->fill = FlatFillSpan32;	break;
		}
	}
	else {
		switch (dstBM->pixelFormat) {
//			case kFskBitmapFormat16RGB5515LE:	span->fill = FskName2(FlatAlphaFillSpan,fsk16RGB5515LEFormatKind);	goto stuffAlpha16;	/* 16 bit RGB 5515 little endian  */
//			case kFskBitmapFormat16RGBA4444LE:	span->fill = FskName2(FlatAlphaFillSpan,fsk16RGBA4444LEFormatKind);	break;				/* 4444 */
			case kFskBitmapFormat16RGB565BE:	span->fill = FskName2(FlatAlphaFillSpan,fsk16RGB565BEFormatKind);	goto stuffAlpha16;	/* 16 bit RGB 565 big endian */
			case kFskBitmapFormat16RGB565LE:	span->fill = FskName2(FlatAlphaFillSpan,fsk16RGB565LEFormatKind);	goto stuffAlpha16;	/* 16 bit RGB 565 big endian */
			case kFskBitmapFormat16BGR565LE:	span->fill = FskName2(FlatAlphaFillSpan,fsk16BGR565LEFormatKind);	goto stuffAlpha16;	/* 16 bit RGB 565 big endian */
			stuffAlpha16:						((UInt8*)(&span->fillColor))[2]	= color->a;							break;
			case kFskBitmapFormat24BGR:			span->fill = FskName2(FlatAlphaFillSpan,fsk24BGRFormatKind);		goto stuffAlpha24;	/* windows 24 bit */
			case kFskBitmapFormat24RGB:			span->fill = FskName2(FlatAlphaFillSpan,fsk24RGBFormatKind);		goto stuffAlpha24;	/* PNG 24 bit			(deprecated) */
			stuffAlpha24:						((UInt8*)(&span->fillColor))[3]	= color->a;							break;
			case kFskBitmapFormat32ARGB:		span->fill = FskName2(FlatAlphaFillSpan,fsk32ARGBFormatKind);		break;				/* mac 32 bit */
			case kFskBitmapFormat32ABGR:		span->fill = FskName2(FlatAlphaFillSpan,fsk32ABGRFormatKind);		break;				/* ??? */
			case kFskBitmapFormat32BGRA:		span->fill = FskName2(FlatAlphaFillSpan,fsk32BGRAFormatKind);		break;				/* win 32 bit */
			case kFskBitmapFormat32RGBA:		span->fill = FskName2(FlatAlphaFillSpan,fsk32RGBAFormatKind);		break;				/* GL */
			case kFskBitmapFormat8G:			span->fill = FlatAlphaFillSpan8G;									goto stuffAlpha8;	/* 8 bit grayscale */
			stuffAlpha8:						((UInt8*)(&span->fillColor))[1]	= color->a;							break;
			default:							err = kFskErrUnsupportedPixelType;									break;
		}
	}

	return err;
}


/*******************************************************************************
 * FskInitSolidColorSpan
 *******************************************************************************/

FskErr
FskInitSolidColorSpan(
	FskSpan					*span,
	FskBitmap				dstBM,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	const struct FskColorSource *cs
)
{
	const FskColorSourceUnion	*csu = (const FskColorSourceUnion*)cs;
	FskColorRGBARecord			color;

	switch(csu->so.type) {
		case kFskColorSourceTypeConstant:		color = csu->cn.color;						break;
		case kFskColorSourceTypeLinearGradient:	color = csu->lg.gradientStops[0].color;		break;
		case kFskColorSourceTypeRadialGradient:	color = csu->rg.gradientStops[0].color;		break;
		case kFskColorSourceTypeTexture:
		case kFskColorSourceTypeProcedure:
		default:								FskColorRGBASet(&color, 255,255,255,255);	break;
	}

	return FskInitSolidColorSpanFromColor(span, dstBM, M, quality, &color);	/* This will premultiply if needed */
}
