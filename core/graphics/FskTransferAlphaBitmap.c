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
#define __FSKBITMAP_PRIV__	/* To get access to the FskBitmap data structure */

#include "FskTransferAlphaBitmap.h"
#include "FskRectBlitPatch.h"
#include "FskPixelOps.h"
#if FSKBITMAP_OPENGL
	#include "FskGLBlit.h"
#endif /* FSKBITMAP_OPENGL */



#if SUPPORT_INSTRUMENTATION
	#define LOG_PARAMETERS		/**< Log the parameters of API calls. */
#endif /* SUPPORT_INSTRUMENTATION */

#if SUPPORT_INSTRUMENTATION
	FskInstrumentedSimpleType(TransferAlpha, transferalpha);
	#define LOGD(...)  do { FskInstrumentedTypePrintfDebug  (&gTransferAlphaTypeInstrumentation, __VA_ARGS__); } while(0)
	#define LOGI(...)  do { FskInstrumentedTypePrintfVerbose(&gTransferAlphaTypeInstrumentation, __VA_ARGS__); } while(0)
	#define LOGE(...)  do { FskInstrumentedTypePrintfMinimal(&gTransferAlphaTypeInstrumentation, __VA_ARGS__); } while(0)
#endif /* SUPPORT_INSTRUMENTATION */
#ifndef LOGD
	#define	LOGD(...)
#endif /* LOGD */
#ifndef LOGI
	#define	LOGI(...)
#endif /* LOGI */
#ifndef LOGE
	#define	LOGE(...)
#endif /* LOGE */


/********************************************************************************
 * Platform configuration
 ********************************************************************************/

#undef DST_8G
#define DST_8G 1	/* Coerce 8G as a destination */
#include "FskBlitDispatchDef.h"

/********************************************************************************
 * Rect proc generation
 ********************************************************************************/
#define SRC_KIND 8A
#define BLIT_PROTO_FILE			"FskTransferAlphaProto.c"
#include "FskBlitDst.h"
#undef BLIT_PROTO_FILE


#define FskUnityColorize8AYUV420	NULL

/********************************************************************************
 * PixelFormatToProcTableDstIndex
 ********************************************************************************/

static SInt32
PixelFormatToProcTableDstIndex(UInt32 pixelFormat)
{
	return (pixelFormat < sizeof(dstPixelFormatToPixelKindIndex) / sizeof(dstPixelFormatToPixelKindIndex[0])) ?
			dstPixelFormatToPixelKindIndex[pixelFormat] :
			-1;
}


/********************************************************************************
 * Dispatch table
 ********************************************************************************/

#define P_D(d)	FskName2(FskUnityColorize8A,d)
static FskRectTransferProc procDispatch[NUM_DST_FORMATS] = {
		P_D(DST_KIND_0)
	#if NUM_DST_FORMATS > 1
		,P_D(DST_KIND_1)
	#endif
	#if NUM_DST_FORMATS > 2
		,P_D(DST_KIND_2)
	#endif
	#if NUM_DST_FORMATS > 3
		,P_D(DST_KIND_3)
	#endif
	#if NUM_DST_FORMATS > 4
		,P_D(DST_KIND_4)
	#endif
	#if NUM_DST_FORMATS > 5
		,P_D(DST_KIND_5)
	#endif
	#if NUM_DST_FORMATS > 6
		,P_D(DST_KIND_6)
	#endif
	#if NUM_DST_FORMATS > 7
		,P_D(DST_KIND_7)
	#endif
	#if NUM_DST_FORMATS > 8
		,P_D(DST_KIND_8)
	#endif
	#if NUM_DST_FORMATS > 9
		,P_D(DST_KIND_9)
	#endif
	#if NUM_DST_FORMATS > 10
		,P_D(DST_KIND_10)
	#endif
	#if NUM_DST_FORMATS > 11
		,P_D(DST_KIND_11)
	#endif
	#if NUM_DST_FORMATS > 12
		,P_D(DST_KIND_12)
	#endif
	#if NUM_DST_FORMATS > 13
		,P_D(DST_KIND_13)
	#endif
	#if NUM_DST_FORMATS > 14
		,P_D(DST_KIND_14)
	#endif
	#if NUM_DST_FORMATS > 15
		#error Unexpected large number of destination pixel formats
	#endif
 };


#if defined(SUPPORT_NEON) || defined(SUPPORT_WMMX)
/* For initialization */
static char patchInstalled = 0;

/* Forward declaration */
static void InstallPatches(void);
#endif


#ifdef LOG_PARAMETERS
static void LogBitmap(FskConstBitmap bm, const char *name) {
	#if FSKBITMAP_OPENGL
		if (!name) name = "BM";
		if (bm->glPort)
			LOGD("\t%s: bounds(%d, %d, %d, %d), depth=%d, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d, texture=#%d",
				name, bm->bounds.x, bm->bounds.y, bm->bounds.width, bm->bounds.height, bm->depth,
				FskBitmapFormatName(bm->pixelFormat), bm->rowBytes, bm->bits, bm->hasAlpha, bm->alphaIsPremultiplied,
				FskGLPortSourceTexture(bm->glPort));
		else
	#endif /* FSKBITMAP_OPENGL */
	LOGD("\t%s: bounds(%d, %d, %d, %d), depth=%d, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d",
		name, bm->bounds.x, bm->bounds.y, bm->bounds.width, bm->bounds.height, bm->depth,
		FskBitmapFormatName(bm->pixelFormat), bm->rowBytes, bm->bits, bm->hasAlpha, bm->alphaIsPremultiplied);
}

static void LogRect(FskConstRectangle r, const char *name) {
	if (!name) name = "RECT";
	if (r) LOGD("\t%s(%d, %d, %d, %d)", name, r->x, r->y, r->width, r->height);
}

static void LogPoint(FskConstPoint pt, const char *name) {
	if (!name) name = "POINT";
	if (pt) LOGD("\t%s(%d, %d)", name, pt->x, pt->y);
}

static void LogColor(FskConstColorRGBA color, const char *name) {
	if (!name) name = "COLOR";
	if (color) LOGD("\t%s(%d, %d, %d, %d)", name, color->r, color->g, color->b, color->a);
}

static void LogModeParams(FskConstGraphicsModeParameters modeParams) {
	if (modeParams) {
		if (modeParams->dataSize <= sizeof(FskGraphicsModeParametersRecord)) {
			LOGD("\tmodeParams: dataSize=%d, blendLevel=%d", modeParams->dataSize, modeParams->blendLevel);
		} else {
			FskGraphicsModeParametersVideo videoParams = (FskGraphicsModeParametersVideo)modeParams;
			LOGD("\tmodeParams: dataSize=%d, blendLevel=%d, kind='%4s, contrast=%f, brightness=%f, sprites=%p",
				videoParams->header.dataSize, videoParams->header.blendLevel, &videoParams->kind, videoParams->contrast, videoParams->brightness, videoParams->sprites);
		}
	}
}
#endif /* LOG_PARAMETERS */


/********************************************************************************
 * FskTransferAlphaBitmap
 ********************************************************************************/

FskErr
FskTransferAlphaBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstPoint					dstLocation,
	FskConstRectangle				dstClip,
	FskConstColorRGBA				fgColor,
	FskConstGraphicsModeParameters	modeParams
)
{
	FskErr				err					= kFskErrNone;
	FskRectTransferProc	blitter				= NULL;
	FskRectangleRecord	sRect, dRect;
	FskRectBlitParams	p;
	SInt32				i;
	SInt32				dstIndex;

	#ifdef LOG_PARAMETERS
		LOGD("TransferAlphaBitmap(srcBM=%p, srcRect=%p, dstBM=%p, dstLoc=%p, dstClip=%p, fgColor=%p, modeParams=%p)",
			srcBM, srcRect, dstBM, dstLocation, dstClip, fgColor, modeParams);
		LogBitmap(srcBM, "srcBM");
		LogRect(srcRect, "srcRect");
		LogBitmap(dstBM, "dstBM");
		LogPoint(dstLocation, "dstLoc");
		LogRect(dstClip, "dstClip");
		LogColor(fgColor, "fgColor");
		LogModeParams(modeParams);
	#endif /* LOG_PARAMETERS */

	#if FSKBITMAP_OPENGL
	if (FskBitmapIsOpenGLDestinationAccelerated(dstBM)) {
		FskGLSetGLView(dstBM->glPort);
		return FskGLTransferAlphaBitmap(srcBM, srcRect, dstBM, dstLocation, dstClip, fgColor, modeParams);
	}
	#endif /* FSKBITMAP_OPENGL */

	FskBitmapReadBegin((FskBitmap)srcBM, NULL, NULL, NULL);
	FskBitmapWriteBegin(          dstBM, NULL, NULL, NULL);

	BAIL_IF_NULL(srcBM,       err, kFskErrInvalidParameter);
	BAIL_IF_FALSE((srcBM->pixelFormat == kFskBitmapFormat8A || srcBM->pixelFormat == kFskBitmapFormat8G), err, kFskErrUnsupportedPixelType);
	BAIL_IF_NEGATIVE((dstIndex = PixelFormatToProcTableDstIndex(dstBM->pixelFormat)), err, kFskErrUnsupportedPixelType);

	if (dstLocation == NULL)
		dstLocation = (const void*)(&srcBM->bounds.x);
	dRect.x =  dstLocation->x;
	dRect.y =  dstLocation->y;
	if (srcRect) {
		sRect = *srcRect;
		if ((i = srcBM->bounds.x - sRect.x) > 0) { sRect.x += i; sRect.width  -= i; dRect.x += i; }					/* Special left clip */
		if ((i = srcBM->bounds.y - sRect.y) > 0) { sRect.y += i; sRect.height -= i; dRect.y += i; }					/* Special top  clip */
		if (!FskRectangleIntersect(&srcBM->bounds, &sRect, &sRect)) { err = kFskErrNothingRendered; goto bail; }	/* Right and bottom clip */
	}
	else {
		sRect = srcBM->bounds;
	}

	/* Compute sRect and dRect, modulo clipping */
	dRect.width  = sRect.width;
	dRect.height = sRect.height;
	if (dstClip != NULL)	FskRectangleIntersect(dstClip, &dRect, &dRect);
	if (!FskRectangleIntersect(&dstBM->bounds, &dRect, &dRect)) { err = kFskErrNothingRendered; goto bail; }
	sRect.x     += dRect.x - dstLocation->x;	/* Transfer left-top dst clipping to src */
	sRect.y     += dRect.y - dstLocation->y;
	sRect.width  = dRect.width;					/* Transfer bottom-right dst clipping to src */
	sRect.height = dRect.height;

	if ((dRect.width <= 0) || (dRect.height <= 0)) { err = kFskErrNone; goto bail; }

	/* Set parameters for blit */
	p.srcRowBytes	= srcBM->rowBytes;
	p.dstRowBytes	= dstBM->rowBytes;
	p.srcBaseAddr	= (UInt8*)srcBM->bits + sRect.y * p.srcRowBytes + sRect.x;	// this function assumes an kFskBitmapFormat8A bitmap, so the depth must be 8
	p.dstBaseAddr	= (UInt8*)dstBM->bits + dRect.y * p.dstRowBytes + dRect.x * (dstBM->depth >> 3);
	p.dstWidth		= dRect.width;
	p.dstHeight		= dRect.height;
	p.red			= fgColor->r;
	p.green			= fgColor->g;
	p.blue			= fgColor->b;
	if (modeParams == NULL) {	p.alpha = fgColor->a;	}
	else {						p.alpha = (UInt8)(i = modeParams->blendLevel);
				if (i > 255)	p.alpha = 255;
				else if (i < 0)	p.alpha = 0;
	}

	if (p.alpha == 0) {			/* Totally transparent */
		err = kFskErrNone;		/* But that is OK, ... */
		goto bail;				/* ... and we are all done */
	}

	/* The following are not used, but we set them to avoid confusion */
	p.srcX0			= 0;
	p.srcY0			= 0;
	p.srcXInc		= 0x00010000;
	p.srcYInc		= 0x00010000;
	p.srcUBaseAddr	= NULL;
	p.srcVBaseAddr	= NULL;
	p.dstUBaseAddr	= NULL;
	p.dstVBaseAddr	= NULL;
	p.srcWidth		= dRect.width;	/* This is redundant, but we set it to avoid confusion */
	p.srcHeight		= dRect.height;	/* This is redundant, but we set it to avoid confusion */
	p.extension		= NULL;

#if defined( SUPPORT_NEON ) ||  defined( SUPPORT_WMMX )
	/* Initialize the dispatch table by patching in machine-specific blitters */
	if (!patchInstalled)
	{
		InstallPatches();
		patchInstalled = 1;
	}
#endif

	/* Choose a blitter */
	blitter = procDispatch[dstIndex];
	BAIL_IF_NULL(blitter, err, kFskErrUnsupportedPixelType);

	/* Do it */
	(*blitter)(&p);

bail:
	FskBitmapReadEnd((FskBitmap)srcBM);
	FskBitmapWriteEnd(dstBM);

	return err;
}


/********************************************************************************
 * SwapProc
 ********************************************************************************/

static FskRectTransferProc
SwapProc(FskRectTransferProc *procSlot, FskRectTransferProc blitProc)
{
	FskRectTransferProc oldProc;

	if (procSlot != NULL) {
		oldProc   = *procSlot;
		*procSlot = blitProc;

	}
	else {
		oldProc = (FskRectTransferProc)kFskErrInvalidParameter;
	}

	return oldProc;
}


/********************************************************************************
 * FskPatchAlphaBlitProc
 ********************************************************************************/

FskRectTransferProc
FskPatchAlphaBlitProc(
	FskRectTransferProc		blitProc,
	UInt32					dstPixelFormat
)
{
	int						dstIndex;
	FskRectTransferProc		*procSlot		= NULL;

	if ((dstIndex = PixelFormatToProcTableDstIndex(dstPixelFormat)) >= 0)
		procSlot = &procDispatch[dstIndex];
	blitProc = (procSlot != NULL) ? SwapProc(procSlot, blitProc) : (FskRectTransferProc)kFskErrUnsupportedPixelType;

	return blitProc;
}


#if defined( SUPPORT_NEON ) ||  defined( SUPPORT_WMMX )
#include "FskArch.h"

#ifdef SUPPORT_NEON//
void alpha_blend_255_16rgbse_arm_v7(    unsigned char *src, unsigned char *dst, unsigned char *color,  int width, int height, int srb, int drb );
void alpha_blend_generic_16rgbse_arm_v7(    unsigned char *src, unsigned char *dst, unsigned char *color,  int width, int height, int srb, int drb );
void alpha_blend_255_32argb_arm_v7(		unsigned char *src, unsigned char *dst, unsigned char *color,  int width, int height, int srb, int drb );
void alpha_blend_generic_32argb_arm_v7(		unsigned char *src, unsigned char *dst, unsigned char *color,  int width, int height, int srb, int drb );
#endif	//SUPPORT_NEON


#ifdef SUPPORT_WMMX//
void alpha_blend_255_16rgbse_wmmx(unsigned char *src, unsigned char *dst, unsigned char *color,  int width, int height, int srb, int drb );
void alpha_blend_generic_16rgbse_wmmx(unsigned char *src, unsigned char *dst, unsigned char *color,  int width, int height, int srb, int drb );
void alpha_blend_255_32argb_wmmx(unsigned char *src, unsigned char *dst, unsigned char *color,  int width, int height, int srb, int drb );
void alpha_blend_generic_32argb_wmmx(unsigned char *src, unsigned char *dst, unsigned char *color,  int width, int height, int srb, int drb );
#endif	//SUPPORT_WMMX



//#define USE_C_Reference		//turn this on for verification with C reference code
#ifdef USE_C_Reference

/*
static void FskUnityColorize8A16RGB565SE_c0(const FskRectBlitParams *p);
static void FskUnityColorize8A32ARGB_c0(const FskRectBlitParams *p);

//original code
static void blend32(UInt32 *d, UInt32 p, UInt8 alpha)
{
	const UInt32 mask = 0x00FF00FF;
	UInt32 p0 = *d;
	UInt32 dstrb =  p0       & mask;
	UInt32 dstag = (p0 >> 8) & mask;

	UInt32 srcrb =  p       & mask;
	UInt32 srcag = (p >> 8) & mask;

	UInt32 drb = (srcrb - dstrb) * alpha;
	UInt32 dag = (srcag - dstag) * alpha;

	p0  =  ((drb >> 8) + dstrb) & mask;
	p   = (((dag >> 8) + dstag) & mask) << 8;

	*d = p0 | p;
}

//original code, reference only
static void FskUnityColorize8A32ARGB_c0(const FskRectBlitParams *p)
{
	const UInt8									*s				= (const UInt8 *)p->srcBaseAddr;
	SInt32										sRBump			= p->srcRowBytes - p->dstWidth;
	SInt32										dRB				= p->dstRowBytes;
	char										*d0				= (char *)p->dstBaseAddr;
	register const UInt8						*sEnd;
	register UInt32						*d;
	char										*dEnd;
	register UInt8								sPix;
	UInt32										colorScratch;
	register UInt32								color;


	fskConvert24RGB32ARGB(*((Fsk24BitType*)(&(p->red))), colorScratch);
	color = colorScratch;

	dEnd = d0 + (dRB * p->dstHeight);
	if (p->alpha == 255)
	{
		//dlog( "$$$p->alpha == 255: calling FskUnityColorize8A32ARGB_c(), width: %d, height: %d, srb: %d, drb: %d\n", p->dstWidth, p->dstHeight, p->srcRowBytes, p->dstRowBytes);
		for (; d0 != dEnd; s += sRBump, d0 += dRB)
		{
			for (sEnd = s + p->dstWidth, d = (UInt32 *)d0; s != sEnd; )
			{
				if ((sPix = *s++) != 0)
				{
						if (sPix == 255)
						{
							*d++ = (UInt32)color;
							continue;
						}
						else
							blend32(d, (UInt32)color, sPix);
				}
				d++;
			}
		}
	}
	else
	{
		//dlog( "calling FskUnityColorize8A32ARGB_c(), width: %d, height: %d, srb: %d, drb: %d\n", p->dstWidth, p->dstHeight, p->srcRowBytes, p->dstRowBytes);
		register UInt32 blendLevel = (p->alpha * 256L + 127) / 255;
		for (; d0 != dEnd; s += sRBump, d0 += dRB)
		{
			for (sEnd = s + p->dstWidth, d = (UInt32 *)d0; s != sEnd; d++ )
			{
				sPix = (UInt8)((*s++ * blendLevel) >> 8);	//this can never be 255

				if (sPix != 0)
					blend32(d, (UInt32)color, sPix);
			}
		}
	}
}


//original code, reference only, note it's 5 bit alpha, it has been extended in later C and ARM implementations
static void blend565SE(UInt16 *d, UInt16 p, UInt8 alpha)
{
	SInt32	p0 = *d, p1;
	SInt32	sAlpha		= alpha >> 3;

	const UInt32 dstrb = p0 & 0x0F81F;
	const UInt32 dstg  = p0 & 0x007E0;

	const UInt32 srcrb = p  & 0x0F81F;
	const UInt32 srcg  = p  & 0x007E0;

	p0 = (dstrb + (((srcrb - dstrb) * sAlpha + 0x8010) >> 5)) & 0xf81f;
	p1 = (dstg  + (((srcg  - dstg ) * sAlpha + 0x0200) >> 5)) & 0x07e0;

	*d = (UInt16)(p0 | p1);
}


//original code, reference only
static void FskUnityColorize8A16RGB565SE_c0(const FskRectBlitParams *p)
{
	const UInt8						*s		= (const UInt8 *)p->srcBaseAddr;
	SInt32							sRBump	= p->srcRowBytes - p->dstWidth;
	SInt32							dRB		= p->dstRowBytes;
	char							*d0		= (char *)p->dstBaseAddr;
	register const UInt8			*sEnd;
	register UInt16		*d;
	char							*dEnd;
	UInt32							colorScratch;
	register UInt32					color;


	fskConvert24RGB16RGB565SE(*((Fsk24BitType*)(&(p->red))), colorScratch);
	color = colorScratch;

	dEnd = d0 + (dRB * p->dstHeight);
	if (p->alpha == 255)
	{
		//dlog( "$$$p->alpha == 255: calling FskUnityColorize8A16RGB565SE_c(), width: %d, height: %d, srb: %d, drb: %d\n", p->dstWidth, p->dstHeight, p->srcRowBytes, p->dstRowBytes);

		for (; d0 != dEnd; s += sRBump, d0 += dRB)
		{
			for (sEnd = s + p->dstWidth, d = (UInt16 *)d0; s != sEnd; )
			{
				register UInt8 sPix = *s++;

				if( sPix != 0 )
				{
					if (sPix == 255)
					{
						*d++ = (UInt16)color;
						continue;
					}
					else
						blend565SE(d, (UInt16)color, sPix);
				}

				d++;
			}
		}
	}
	else
	{
		//dlog( "calling FskUnityColorize8A16RGB565SE_c(), width: %d, height: %d, srb: %d, drb: %d\n", p->dstWidth, p->dstHeight, p->srcRowBytes, p->dstRowBytes);

		register UInt32 blendLevel = (p->alpha * 256L + 127) / 255;
		for (; d0 != dEnd; s += sRBump, d0 += dRB)
		{
			for (sEnd = s + p->dstWidth, d = (UInt16 *)d0; s != sEnd; d++ )
			{
				register UInt8 sPix = (UInt8)((*s++ * blendLevel) >> 8);	//this can never be 255

				if( sPix != 0 )
					blend565SE(d, (UInt16)color, sPix);
			}
		}
	}
}
*/


/* We compute a 6-bit (base-64) alpha by multiplying the 8-bit source alpha by the 8-bit blend level, and normalizing to 64. */
void alpha_blend_generic_16rgbse_c( unsigned char *s, unsigned char *d, unsigned char *color, int width0, int height, int srb, int drb )
{
	int alpha0  = color[0];					/* 8 bit blend level */
	int color_r = ((color[1])>>3)&0x1f;		/* 5 bits of R */
	int color_g = ((color[2])>>2)&0x3f;		/* 6 bits of G */
	int color_b = ((color[3])>>3)&0x1f;		/* 5 bits of B */

	alpha0 = alpha0 + (alpha0>>7);			/* Convert blend level from base-255 to base-256, though since color[0] < 255, alpha0 < 256. */

	while(1)
	{
		int	width = width0;

		while(1)
		{
			int	p0		= *(UInt16 *)d;
			int	alpha	= (int)(*s++);
			int dr = (p0>>11)&0x1f;			/* 5 bits of R */
			int dg = (p0>> 5)&0x3f;			/* 6 bits of G */
			int db = (p0>> 0)&0x1f;			/* 5 bits of B */

			alpha = alpha + (alpha>>7);		/* Convert alpha from base-255 to base-256 */
			alpha = (alpha * alpha0 + (1<<(10-1))) >> 10;	/* Scale alpha by blend level: base-256 * base-256 / 1024 --> base-64. Max product before shifting 0xFF00 easily fits into a 16-bit word */

			dr = (dr + (((color_r - dr) * alpha + (1<<(6-1))) >> 6));	/* Alpha is base-64 (blend algorithm Q64 is equivalent to R64 when implemented serially) */
			dg = (dg + (((color_g - dg) * alpha + (1<<(6-1))) >> 6));
			db = (db + (((color_b - db) * alpha + (1<<(6-1))) >> 6));

			*(UInt16 *)d = (UInt16)( (dr<<11) | (dg<<5) | (db<<0) );
			d += 2;

			width--;
			if( width == 0 )
				break;
		}

		height--;
		if( height == 0 )
			break;

		s += srb;
		d += drb;
	}
}


/* We use a 6-bit alpha for blending, because it is easier to implement in parallel. */
void alpha_blend_255_16rgbse_c( unsigned char *s, unsigned char *d, unsigned char *color, int width0, int height, int srb, int drb )
{
	int color_r = ((color[1])>>3)&0x1f;
	int color_g = ((color[2])>>2)&0x3f;
	int color_b = ((color[3])>>3)&0x1f;

	while(1)
	{
		int	width = width0;

		while(1)
		{
			int	p0		= *(UInt16 *)d;
			int	alpha	= (int)(*s++);
			int dr = (p0>>11)&0x1f;
			int dg = (p0>> 5)&0x3f;
			int db = (p0>> 0)&0x1f;

			alpha = alpha + (alpha>>7);		/* Convert alpha from base-255 to base-256 */
			alpha = alpha >> 2;				/* Convert alpha from base-256 to base-64  */

			dr = (dr + (((color_r - dr) * alpha + (1<<(6-1))) >> 6));	/* alpha is base-64 */
			dg = (dg + (((color_g - dg) * alpha + (1<<(6-1))) >> 6));
			db = (db + (((color_b - db) * alpha + (1<<(6-1))) >> 6));

			*(UInt16 *)d = (UInt16)( (dr<<11) | (dg<<5) | (db<<0) );

			d += 2;

			width--;
			if( width == 0 )
				break;
		}

		height--;
		if( height == 0 )
			break;

		s += srb;
		d += drb;
	}
}

void alpha_blend_255_32argb_c( unsigned char *s, unsigned char *d, unsigned char *color, int width0, int height, int srb, int drb )
{
	int color_a = fskDefaultAlpha;
	int color_r = color[1];
	int color_g = color[2];
	int color_b = color[3];

	while(1)
	{
		int	width = width0;

		while(1)
		{
			int	alpha	= (int)(*s++);
			int da = d[3];
			int dr = d[2];
			int dg = d[1];
			int db = d[0];

			alpha = alpha + (alpha>>7);

			da = (da + (((color_a - da) * alpha + (1<<(8-1)) ) >> 8));	//128
			dr = (dr + (((color_r - dr) * alpha + (1<<(8-1)) ) >> 8));
			dg = (dg + (((color_g - dg) * alpha + (1<<(8-1)) ) >> 8));
			db = (db + (((color_b - db) * alpha + (1<<(8-1)) ) >> 8));

			*(UInt32 *)d = (UInt32)( (da<<24) |  (dr<<16) | (dg<<8) | (db<<0) );
			d += 4;

			width--;
			if( width == 0 )
				break;
		}

		height--;
		if( height == 0 )
			break;

		s += srb;
		d += drb;
	}
}


void alpha_blend_generic_32argb_c( unsigned char *s, unsigned char *d, unsigned char *color, int width0, int height, int srb, int drb )
{
	int alpha0  = color[0];				/* Alpha0 (blendLevel) < 255, because 255 has been pre-detected and alpha_blend_255_* called instead. [0, ..., 254] */
	int color_r = color[1];				/* Extract source color */
	int color_g = color[2];
	int color_b = color[3];

	alpha0 = alpha0 + (alpha0 >> 7);	/* Convert alpha0 from base-255 to base-256:											[0, ..., 127, 129, ... 255] */

	while(1)
	{
		int	width = width0;

		while(1)
		{
			int alpha = (int)(*s++);									/* This source can take on values									[0, ..., 255] */
			int da    = d[3];											/* Extract dst components */
			int dr    = d[2];
			int dg    = d[1];
			int db    = d[0];

			alpha = (alpha * alpha0) >> 8;								/* Compute the composite alpha, using multiplication algorithm T256:   [0, ..., 254] */
			alpha = alpha + (alpha >> 7);								/* Convert the composite alpha from base-255 to base-256 [0, ..., 127, 129, ... 255] */

			da = (da + (((255     - da) * alpha + (1<<(8-1)) ) >> 8));	/* Composite using algorithm R256 */
			dr = (dr + (((color_r - dr) * alpha + (1<<(8-1)) ) >> 8));
			dg = (dg + (((color_g - dg) * alpha + (1<<(8-1)) ) >> 8));
			db = (db + (((color_b - db) * alpha + (1<<(8-1)) ) >> 8));

			*(UInt32 *)d = (UInt32)( (da<<24) |  (dr<<16) | (dg<<8) | (db<<0) );
			d += 4;

			width--;
			if( width == 0 )
				break;
		}

		height--;
		if( height == 0 )
			break;

		s += srb;
		d += drb;
	}
}

static void FskUnityColorize8A16RGB565SE_C(const FskRectBlitParams *p)
{
	SInt32			srb		= p->srcRowBytes - p->dstWidth;
	SInt32			drb		= p->dstRowBytes - (p->dstWidth<<1);
	unsigned char	*alpha_color  = (unsigned char *)&p->alpha;

	//dlog( "bnie_197!!! FskUnityColorize8A16RGB565SE_c(), width: %d, height: %d, srb: %d, drb: %d, red: %d, green: %d, blue: %d, alpha: %d\n",
	//		(int)p->dstWidth, (int)p->dstHeight, (int)srb, (int)drb, (int)alpha_color[1], (int)alpha_color[2], (int)alpha_color[3], (int)p->alpha);
	if( alpha_color[0] == 255 )
		alpha_blend_255_16rgbse_c( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
	else
		alpha_blend_generic_16rgbse_c( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
}


static void FskUnityColorize8A32ARGB_C(const FskRectBlitParams *p)
{
	SInt32			srb		= p->srcRowBytes - p->dstWidth;
	SInt32			drb		= p->dstRowBytes - (p->dstWidth<<2);
	unsigned char	*alpha_color  = (unsigned char *)&p->alpha;
	//unsigned char	alpha_color[4]  = {254, 255, 255, 255};

	//dlog( "bnie_091  !!! FskUnityColorize8A32ARGB_arm_v7(), width: %d, height: %d, srb: %d, drb: %d, red: %d, green: %d, blue: %d, alpha: %d\n",
	//		(int)p->dstWidth, (int)p->dstHeight, (int)srb, (int)drb, (int)alpha_color[1], (int)alpha_color[2], (int)alpha_color[3], (int)alpha_color[0]);
	if( alpha_color[0] == 255 )
		alpha_blend_255_32argb_c( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
	else
	{
		//verify( (unsigned char *)p->srcBaseAddr, (DST_TYPE *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
		alpha_blend_generic_32argb_c( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
	}
}

#endif	//USE_C_Reference


#ifdef SUPPORT_NEON	//
static void FskUnityColorize8A16RGB565SE_arm_v7(const FskRectBlitParams *p)
{
	SInt32			srb		= p->srcRowBytes - p->dstWidth;
	SInt32			drb		= p->dstRowBytes - (p->dstWidth<<1);
	unsigned char	*alpha_color  = (unsigned char *)&p->alpha;

	//dlog( "bnie_197!!! FskUnityColorize8A16RGB565SE_c(), width: %d, height: %d, srb: %d, drb: %d, red: %d, green: %d, blue: %d, alpha: %d\n",
	//		(int)p->dstWidth, (int)p->dstHeight, (int)srb, (int)drb, (int)alpha_color[1], (int)alpha_color[2], (int)alpha_color[3], (int)p->alpha);
	if( alpha_color[0] == 255 )
		alpha_blend_255_16rgbse_arm_v7( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
	else
		alpha_blend_generic_16rgbse_arm_v7( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
}


static void FskUnityColorize8A32ARGB_arm_v7(const FskRectBlitParams *p)
{
	SInt32			srb		= p->srcRowBytes - p->dstWidth;
	SInt32			drb		= p->dstRowBytes - (p->dstWidth<<2);
	unsigned char	*alpha_color  = (unsigned char *)&p->alpha;
	//unsigned char	alpha_color[4]  = {254, 255, 255, 255};

	//dlog( "bnie_091  !!! FskUnityColorize8A32ARGB_arm_v7(), width: %d, height: %d, srb: %d, drb: %d, red: %d, green: %d, blue: %d, alpha: %d\n",
	//		(int)p->dstWidth, (int)p->dstHeight, (int)srb, (int)drb, (int)alpha_color[1], (int)alpha_color[2], (int)alpha_color[3], (int)alpha_color[0]);
	if( alpha_color[0] == 255 )
		alpha_blend_255_32argb_arm_v7( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
	else
	{
		//verify( (unsigned char *)p->srcBaseAddr, (DST_TYPE *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
		alpha_blend_generic_32argb_arm_v7( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
	}
}
#endif	//SUPPORT_NEON


#ifdef SUPPORT_WMMX	//
static void FskUnityColorize8A16RGB565SE_wmmx(const FskRectBlitParams *p)
{
	SInt32			srb		= p->srcRowBytes - p->dstWidth;
	SInt32			drb		= p->dstRowBytes - (p->dstWidth<<1);
	unsigned char	*alpha_color  = (unsigned char *)&p->alpha;

	if( alpha_color[0] == 255 )
		alpha_blend_255_16rgbse_wmmx( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
	else
		alpha_blend_generic_16rgbse_wmmx( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
}


static void FskUnityColorize8A32ARGB_wmmx(const FskRectBlitParams *p)
{
	SInt32			srb		= p->srcRowBytes - p->dstWidth;
	SInt32			drb		= p->dstRowBytes - (p->dstWidth<<2);
	unsigned char	*alpha_color  = (unsigned char *)&p->alpha;

	if( alpha_color[0] == 255 )
		alpha_blend_255_32argb_wmmx( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
	else
	{
		//verify( (unsigned char *)p->srcBaseAddr, (DST_TYPE *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
		alpha_blend_generic_32argb_wmmx( (unsigned char *)p->srcBaseAddr, (unsigned char *)p->dstBaseAddr, alpha_color, p->dstWidth, p->dstHeight, srb, drb );
	}
}
#endif	//SUPPORT_WMMX


static void
InstallPatches(void)
{
	int implementation = FskHardwareGetARMCPU_All();

#ifdef USE_C_Reference
	//for verification purpose
	FskPatchAlphaBlitProc( FskUnityColorize8A16RGB565SE_C, kFskBitmapFormat16RGB565LE);
	FskPatchAlphaBlitProc( FskUnityColorize8A32ARGB_C, kFskBitmapFormat32BGRA);
	return;
#endif

#ifdef SUPPORT_NEON	//
	if( implementation == FSK_ARCH_ARM_V7 )
	{

		FskPatchAlphaBlitProc(
							  FskUnityColorize8A16RGB565SE_arm_v7,
							  kFskBitmapFormat16RGB565LE
							  );

		FskPatchAlphaBlitProc(
							  FskUnityColorize8A32ARGB_arm_v7,
							  kFskBitmapFormat32BGRA
							  );
		return;
	}
#endif	//SUPPORT_WMMX

#if defined(SUPPORT_WMMX)
	if(implementation == FSK_ARCH_XSCALE) //WMMX by Nick
	{
		FskPatchAlphaBlitProc( FskUnityColorize8A16RGB565SE_wmmx, kFskBitmapFormat16RGB565LE);
		FskPatchAlphaBlitProc( FskUnityColorize8A32ARGB_wmmx, kFskBitmapFormat32BGRA);
		return;
	}
#endif	//SUPPORT_WMMX

}

#endif
