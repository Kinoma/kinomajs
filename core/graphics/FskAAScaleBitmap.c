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
/**
	\file	FskAAScaleBitmap.c
	\brief	Antialiased bitmap scaling.
*/
#define __FSKBITMAP_PRIV__

#include "FskAAScaleBitmap.h"
#include "FskPixelOps.h"
#include "FskMemory.h"
#include "FskArch.h"

#if TARGET_OS_WIN32
	#define _USE_MATH_DEFINES	/* Otherwise M_PI_2 is not defined in math.h */
#endif /* TARGET_OS_WIN32 */
#include <math.h>
#ifndef M_PI_2
	#define M_PI_2          1.57079632679489661923
#endif /* M_PI_2 */
#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)	/* Pre-C99 */
 	static long mylround(double x) { return (long)((x) + (((x) < 0) ? -0.5 : 0.5)); }
 	#define lround(x) mylround(x)
#endif /* Pre-C99 */

#if SUPPORT_INSTRUMENTATION
	FskInstrumentedSimpleType(AAScale, aascale);
	#define LOGD(...)  do { FskInstrumentedTypePrintfDebug  (&gAAScaleTypeInstrumentation, __VA_ARGS__); } while(0)
	#define LOGI(...)  do { FskInstrumentedTypePrintfVerbose(&gAAScaleTypeInstrumentation, __VA_ARGS__); } while(0)
	#define LOGE(...)  do { FskInstrumentedTypePrintfMinimal(&gAAScaleTypeInstrumentation, __VA_ARGS__); } while(0)
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

typedef struct PolyphaseFilter {
	SInt16 offsetHi;
#ifdef HUGE_IMAGES
	UInt16 offsetLo;
#endif /* HUGE_IMAGES */
	UInt16 filterLength;
	SInt16 coeffs[1];	/* actually, the size is filterLength */
} PolyphaseFilter;

typedef SInt16 BufferType;
typedef void (*HDotter)(FskAAScaler scaler, const void *src, BufferType *dst);
typedef void (*VDotter)(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst);
typedef void (*MakeFilterBankProc)(UInt32 support, UInt32 numPhases, UInt32 srcSize, double initialPhase, double scale,
									float *tmpFilter, PolyphaseFilter *filterBank);
typedef float (*MakeFilterProc)(const void *userData, UInt32 filterSupport, double tapPhase, double tapDeltaPhase, float *filter);

struct FskAAScalerRecord {
	UInt32			srcWidth, srcHeight, dstWidth, dstHeight;

	int				hFilterBankStride, vFilterBankStride, hFilterSupport, vFilterSupport;
	PolyphaseFilter	*hFilterBank, *vFilterBank;

	BufferType		*buffer;
	int				bufferRowBytes;

	HDotter			hDot;
	VDotter			vDot;

	FskAAScalerGetNextSrcLine	getNextSrcLine;
	FskAAScalerGetNextDstLine	getNextDstLine;
	void						*userData;

	int		srcY, bufY;
	UInt32	dstY;
	const PolyphaseFilter *vFilter;

};


#define NEXT_CFILTER(filter, stride)			(filter = (const PolyphaseFilter*)((const char*)(filter) + (stride)))
#define NEXT_FILTER(filter, stride)				(filter = (PolyphaseFilter*)((char*)(filter) + (stride)))
#ifdef HUGE_IMAGES
	#define GET_FILTER_OFFSET(filter) 			((((SInt32)(filter->offsetHi)) << 16) | ((SInt32)(filter->offsetLo)))
	#define SET_FILTER_OFFSET(filter, offset)	do { (filter)->offsetHi = (SInt16)((offset) >> 16); (filter)->offsetLo = (UInt16)(offset); } while(0)
#else /* TYPICALLY_SIZED_IMAGES */
	#define GET_FILTER_OFFSET(filter) 			(filter->offsetHi)
	#define SET_FILTER_OFFSET(filter, offset)	do { (filter)->offsetHi = (SInt16)(offset); } while(0)
#endif /* TYPICALLY_SIZED_IMAGES */

#define kFilterFracBits 	14
#define kBufferFracBits		14
#define kFilterScale		(1 << kFilterFracBits)
#define HFILTER_SHIFT(acc, inBits)	(acc += 1 << (inBits + kFilterFracBits - kBufferFracBits - 1),					\
										acc >>= inBits + kFilterFracBits - kBufferFracBits)
#define VFILTER_SHIFT(acc, outBits)	(acc += 1 << (kBufferFracBits + kFilterFracBits - outBits - 1),					\
										acc >>= kBufferFracBits + kFilterFracBits - outBits,						\
										acc = ((acc >> outBits) == 0) ? acc : ((acc < 0) ? 0 : ((1 << outBits) - 1)))


#if SUPPORT_INSTRUMENTATION

static void LogBitmap(FskConstBitmap bm, const char *name) {
	if (!bm)
		return;
	if (!name)
		name = "BM";
	LOGD("\t%s: bounds(%d, %d, %d, %d), depth=%u, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d",
		name, (int)bm->bounds.x, (int)bm->bounds.y, (int)bm->bounds.width, (int)bm->bounds.height, (unsigned)bm->depth,
		FskBitmapFormatName(bm->pixelFormat), (int)bm->rowBytes, bm->bits, bm->hasAlpha, bm->alphaIsPremultiplied);
}

static void LogRect(FskConstRectangle r, const char *name) {
	if (!r)
		return;
	if (!name)
		name = "RECT";
	LOGD("\t%s(%d, %d, %d, %d)", name, (int)r->x, (int)r->y, (int)r->width, (int)r->height);
}

typedef struct LookupEntry {
	int			code;
	const char	*name;
} LookupEntry;

static const char* LookupNameFromCode(const LookupEntry *table, int code) {
	for (; table->name != NULL; ++table)
		if (table->code == code)
			break;
	return table->name ? table->name : "UNKNOWN";
}

static const char* GetKernelNameFromCode(int code) {
	static const LookupEntry kernelTab[] = {
		{	kAAScaleTentKernelType,		"Tent"		},
		{	kAAScaleLanczosKernelType,	"Lanczos"	},
		{	0,							NULL		}
	};
	return LookupNameFromCode(kernelTab, code);
}


#endif /* SUPPORT_INSTRUMENTATION */


/******************************************************************************
 ******************************************************************************
 ** Dot Product machines.
 ** Note: the buffer format is standardized as
 **   { SInt16 alpha, red, green, blue; } ...
 ******************************************************************************
 ******************************************************************************/


/******************************************************************************
 * 32ARGB src
 ******************************************************************************/

static void
HDot32ARGB(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt32			*s0		= (const UInt32*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16 *f	= filter->coeffs;
		const UInt32 *s = s0 + GET_FILTER_OFFSET(filter);
		UInt32 k;
		SInt32 a, r, g, b;
		for (k = filter->filterLength, a = 0, r = 0, g = 0, b = 0; k--;) {
			SInt32 coeff = *f++;
			SInt32 pix = *s++;
			a += coeff * ((pix >> fsk32ARGBAlphaPosition) & 0xFF);
			r += coeff * ((pix >> fsk32ARGBRedPosition)   & 0xFF);
			g += coeff * ((pix >> fsk32ARGBGreenPosition) & 0xFF);
			b += coeff * ((pix >> fsk32ARGBBluePosition)  & 0xFF);
		}
		*dst++ = (BufferType)HFILTER_SHIFT(a, fsk32ARGBAlphaBits);
		*dst++ = (BufferType)HFILTER_SHIFT(r, fsk32ARGBRedBits);
		*dst++ = (BufferType)HFILTER_SHIFT(g, fsk32ARGBGreenBits);
		*dst++ = (BufferType)HFILTER_SHIFT(b, fsk32ARGBBlueBits);
	}
}


/******************************************************************************
 * 32ARGB dst
 ******************************************************************************/

static void
VDot32ARGB(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt32	*d = (UInt32*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16 *f	= filter->coeffs;
		const BufferType  *s = src;
		UInt32 k;
		SInt32 a, r, g, b;
		for (k = filter->filterLength, a = 0, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			a += coeff * s[0];
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}
		k  = VFILTER_SHIFT(a, fsk32ARGBAlphaBits) << fsk32ARGBAlphaPosition;
		k |= VFILTER_SHIFT(r, fsk32ARGBRedBits)   << fsk32ARGBRedPosition;
		k |= VFILTER_SHIFT(g, fsk32ARGBGreenBits) << fsk32ARGBGreenPosition;
		k |= VFILTER_SHIFT(b, fsk32ARGBBlueBits)  << fsk32ARGBBluePosition;
		*d++ = k;
	}
}


/******************************************************************************
 * 32BGRA src
 ******************************************************************************/

static void
HDot32BGRA(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt32			*s0		= (const UInt32*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16 *f	= filter->coeffs;
		const UInt32 *s = s0 + GET_FILTER_OFFSET(filter);
		UInt32 k;
		SInt32 a, r, g, b;
		for (k = filter->filterLength, a = 0, r = 0, g = 0, b = 0; k--;) {
			SInt32 coeff = *f++;
			SInt32 pix = *s++;
			a += coeff * ((pix >> fsk32BGRAAlphaPosition) & 0xFF);
			r += coeff * ((pix >> fsk32BGRARedPosition)   & 0xFF);
			g += coeff * ((pix >> fsk32BGRAGreenPosition) & 0xFF);
			b += coeff * ((pix >> fsk32BGRABluePosition)  & 0xFF);
		}
		*dst++ = (BufferType)HFILTER_SHIFT(a, fsk32BGRAAlphaBits);
		*dst++ = (BufferType)HFILTER_SHIFT(r, fsk32BGRARedBits);
		*dst++ = (BufferType)HFILTER_SHIFT(g, fsk32BGRAGreenBits);
		*dst++ = (BufferType)HFILTER_SHIFT(b, fsk32BGRABlueBits);
	}
}


/******************************************************************************
 * 32BGRA dst
 ******************************************************************************/

static void
VDot32BGRA(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt32	*d = (UInt32*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16 *f	= filter->coeffs;
		const BufferType *s = src;
		UInt32 k;
		SInt32 a, r, g, b;
		for (k = filter->filterLength, a = 0, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			a += coeff * s[0];
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}
		k  = VFILTER_SHIFT(a, fsk32BGRAAlphaBits) << fsk32BGRAAlphaPosition;
		k |= VFILTER_SHIFT(r, fsk32BGRARedBits)   << fsk32BGRARedPosition;
		k |= VFILTER_SHIFT(g, fsk32BGRAGreenBits) << fsk32BGRAGreenPosition;
		k |= VFILTER_SHIFT(b, fsk32BGRABlueBits)  << fsk32BGRABluePosition;
		*d++ = k;
	}
}


/******************************************************************************
 * 32RGBA src
 ******************************************************************************/

static void
HDot32RGBA(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt32			*s0		= (const UInt32*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16 *f	= filter->coeffs;
		const UInt32 *s = s0 + GET_FILTER_OFFSET(filter);
		UInt32 k;
		SInt32 a, r, g, b;
		for (k = filter->filterLength, a = 0, r = 0, g = 0, b = 0; k--;) {
			SInt32 coeff = *f++;
			SInt32 pix = *s++;
			a += coeff * ((pix >> fsk32RGBAAlphaPosition) & 0xFF);
			r += coeff * ((pix >> fsk32RGBARedPosition)   & 0xFF);
			g += coeff * ((pix >> fsk32RGBAGreenPosition) & 0xFF);
			b += coeff * ((pix >> fsk32RGBABluePosition)  & 0xFF);
		}
		*dst++ = (BufferType)HFILTER_SHIFT(a, fsk32RGBAAlphaBits);
		*dst++ = (BufferType)HFILTER_SHIFT(r, fsk32RGBARedBits);
		*dst++ = (BufferType)HFILTER_SHIFT(g, fsk32RGBAGreenBits);
		*dst++ = (BufferType)HFILTER_SHIFT(b, fsk32RGBABlueBits);
	}
}


/******************************************************************************
 * 32RGBA dst
 ******************************************************************************/

static void
VDot32RGBA(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt32	*d = (UInt32*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16 *f	= filter->coeffs;
		const BufferType *s = src;
		UInt32 k;
		SInt32 a, r, g, b;
		for (k = filter->filterLength, a = 0, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			a += coeff * s[0];
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}
		k  = VFILTER_SHIFT(a, fsk32RGBAAlphaBits) << fsk32RGBAAlphaPosition;
		k |= VFILTER_SHIFT(r, fsk32RGBARedBits)   << fsk32RGBARedPosition;
		k |= VFILTER_SHIFT(g, fsk32RGBAGreenBits) << fsk32RGBAGreenPosition;
		k |= VFILTER_SHIFT(b, fsk32RGBABlueBits)  << fsk32RGBABluePosition;
		*d++ = k;
	}
}


/******************************************************************************
 * 32ABGR src
 ******************************************************************************/

static void
HDot32ABGR(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt32			*s0		= (const UInt32*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16 *f	= filter->coeffs;
		const UInt32 *s = s0 + GET_FILTER_OFFSET(filter);
		UInt32 k;
		SInt32 a, r, g, b;
		for (k = filter->filterLength, a = 0, r = 0, g = 0, b = 0; k--;) {
			SInt32 coeff = *f++;
			SInt32 pix = *s++;
			a += coeff * ((pix >> fsk32ABGRAlphaPosition) & 0xFF);
			r += coeff * ((pix >> fsk32ABGRRedPosition)   & 0xFF);
			g += coeff * ((pix >> fsk32ABGRGreenPosition) & 0xFF);
			b += coeff * ((pix >> fsk32ABGRBluePosition)  & 0xFF);
		}
		*dst++ = (BufferType)HFILTER_SHIFT(a, fsk32ABGRAlphaBits);
		*dst++ = (BufferType)HFILTER_SHIFT(r, fsk32ABGRRedBits);
		*dst++ = (BufferType)HFILTER_SHIFT(g, fsk32ABGRGreenBits);
		*dst++ = (BufferType)HFILTER_SHIFT(b, fsk32ABGRBlueBits);
	}
}


/******************************************************************************
 * 32ABGR dst
 ******************************************************************************/

static void
VDot32ABGR(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt32	*d = (UInt32*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16 *f	= filter->coeffs;
		const BufferType *s = src;
		UInt32 k;
		SInt32 a, r, g, b;
		for (k = filter->filterLength, a = 0, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			a += coeff * s[0];
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}
		k  = VFILTER_SHIFT(a, fsk32ABGRAlphaBits) << fsk32ABGRAlphaPosition;
		k |= VFILTER_SHIFT(r, fsk32ABGRRedBits)   << fsk32ABGRRedPosition;
		k |= VFILTER_SHIFT(g, fsk32ABGRGreenBits) << fsk32ABGRGreenPosition;
		k |= VFILTER_SHIFT(b, fsk32ABGRBlueBits)  << fsk32ABGRBluePosition;
		*d++ = k;
	}
}


/******************************************************************************
 * 32A16RGB565SE src
 ******************************************************************************/

static void
HDot32A16RGB565SE(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt32			*s0		= (const UInt32*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16 *f	= filter->coeffs;
		const UInt32 *s	= s0 + GET_FILTER_OFFSET(filter);
		UInt32 k;
		SInt32 a, r, g, b;
		for (k = filter->filterLength, a = 0, r = 0, g = 0, b = 0; k--;) {
			SInt32 coeff = *f++;
			UInt32 pix = *s++;										/* Conversion to 32ARGB requires that pix be UInt32 */
			fskConvert32A16RGB565SE32ARGB(pix);
			a += coeff * (SInt32)((pix >> fsk32ARGBAlphaPosition ) & 0xFF);		/* We cast to assure a signed multiply */
			r += coeff * (SInt32)((pix >> fsk32ARGBRedPosition   ) & 0xFF);
			g += coeff * (SInt32)((pix >>  fsk32ARGBGreenPosition) & 0xFF);
			b += coeff * (SInt32)((pix >>  fsk32ARGBBluePosition ) & 0xFF);
		}
		*dst++ = (BufferType)HFILTER_SHIFT(a, fsk32ARGBAlphaBits);
		*dst++ = (BufferType)HFILTER_SHIFT(r, fsk32ARGBRedBits);
		*dst++ = (BufferType)HFILTER_SHIFT(g, fsk32ARGBGreenBits);
		*dst++ = (BufferType)HFILTER_SHIFT(b, fsk32ARGBBlueBits);
	}
}


/******************************************************************************
 * 32A16RGB565SE dst
 ******************************************************************************/

static void
VDot32A16RGB565SE(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt32	*d = (UInt32*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16 *f	= filter->coeffs;
		const BufferType *s = src;
		UInt32 k;
		SInt32 a, r, g, b;
		for (k = filter->filterLength, a = 0, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			a += coeff * s[0];
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}
		k  = VFILTER_SHIFT(a, fsk32A16RGB565SEAlphaBits) << fsk32A16RGB565SEAlphaPosition;
		k |= VFILTER_SHIFT(r, fsk32A16RGB565SERedBits)   << fsk32A16RGB565SERedPosition;
		k |= VFILTER_SHIFT(g, fsk32A16RGB565SEGreenBits) << fsk32A16RGB565SEGreenPosition;
		k |= VFILTER_SHIFT(b, fsk32A16RGB565SEBlueBits)  << fsk32A16RGB565SEBluePosition;
		*d++ = k;
	}
}


/******************************************************************************
 * 24RGB src
 ******************************************************************************/

static void
HDot24RGB(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt8				*s0		= (const UInt8*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16	*f	= filter->coeffs;
		const UInt8		*s = s0 + GET_FILTER_OFFSET(filter) * fsk24RGBBytes;
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--;) {
			SInt32 coeff = *f++;
			r += coeff * (SInt32)(*s++);
			g += coeff * (SInt32)(*s++);
			b += coeff * (SInt32)(*s++);
		}
		*dst++ = (BufferType)(255 << (kBufferFracBits - 8));
		*dst++ = (BufferType)HFILTER_SHIFT(r, fsk24RGBRedBits);
		*dst++ = (BufferType)HFILTER_SHIFT(g, fsk24RGBGreenBits);
		*dst++ = (BufferType)HFILTER_SHIFT(b, fsk24RGBBlueBits);
	}
}


/******************************************************************************
 * 24RGB dst
 ******************************************************************************/

static void
VDot24RGB(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt8	*d = (UInt8*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16 *f	= filter->coeffs;
		const BufferType  *s = src;
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}
		*d++ = (UInt8)VFILTER_SHIFT(r, fsk24RGBRedBits);
		*d++ = (UInt8)VFILTER_SHIFT(g, fsk24RGBGreenBits);
		*d++ = (UInt8)VFILTER_SHIFT(b, fsk24RGBBlueBits);
	}
}


/******************************************************************************
 * 24BGR src
 ******************************************************************************/

static void
HDot24BGR(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt8				*s0		= (const UInt8*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16	*f	= filter->coeffs;
		const UInt8		*s = s0 + GET_FILTER_OFFSET(filter) * fsk24RGBBytes;
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--;) {
			SInt32 coeff = *f++;
			b += coeff * (SInt32)(*s++);
			g += coeff * (SInt32)(*s++);
			r += coeff * (SInt32)(*s++);
		}
		*dst++ = (BufferType)(255 << (kBufferFracBits - 8));
		*dst++ = (BufferType)HFILTER_SHIFT(r, fsk24BGRRedBits);
		*dst++ = (BufferType)HFILTER_SHIFT(g, fsk24BGRGreenBits);
		*dst++ = (BufferType)HFILTER_SHIFT(b, fsk24BGRBlueBits);
	}
}


/******************************************************************************
 * 24BGR dst
 ******************************************************************************/

static void
VDot24BGR(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt8	*d = (UInt8*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16 *f	= filter->coeffs;
		const BufferType  *s = src;
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}
		*d++ = (UInt8)VFILTER_SHIFT(b, fsk24BGRBlueBits);
		*d++ = (UInt8)VFILTER_SHIFT(g, fsk24BGRGreenBits);
		*d++ = (UInt8)VFILTER_SHIFT(r, fsk24BGRRedBits);
	}
}


/******************************************************************************
 * 8G src
 ******************************************************************************/

static void
HDot8G(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt8				*s0		= (const UInt8*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16	*f	= filter->coeffs;
		const UInt8		*s = s0 + GET_FILTER_OFFSET(filter) * fsk8GBytes;
		UInt32 k;
		SInt32 g, r;
		for (k = filter->filterLength, g = 0; k--;) {
			SInt32 coeff = *f++;
			g += coeff * (SInt32)(*s++);
		}
		r = HFILTER_SHIFT(g, fsk8GGrayBits);
		*dst++ = (BufferType)(255 << (kBufferFracBits - 8));
		*dst++ = (BufferType)r;
		*dst++ = (BufferType)r;
		*dst++ = (BufferType)r;
	}
}


/******************************************************************************
 * 8G dst
 ******************************************************************************/

static void
VDot8G(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt8	*d = (UInt8*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16 *f	= filter->coeffs;
		const BufferType  *s = src;
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}

		fskConvertFixed8G(r, g, b, kBufferFracBits + kFilterFracBits, g);
		if ((g >> 8) != 0)
			g = (g < 0) ? 0 : 255;
		*d++ = (UInt8)g;
	}
}


/******************************************************************************
 * 16RGB565SE src
 ******************************************************************************/

static void
HDot16RGB565SE(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt16			*s0		= (const UInt16*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16	*f	= filter->coeffs;
		const UInt16	*s = s0 + GET_FILTER_OFFSET(filter);
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--;) {
			SInt32 coeff = *f++;
			UInt32 pix   = *s++;
			UInt8 rgb[3];
			fskConvert16RGB565SE24RGB(pix, rgb[0]);
			r += coeff * (SInt32)(rgb[0]);
			g += coeff * (SInt32)(rgb[1]);
			b += coeff * (SInt32)(rgb[2]);
		}
		*dst++ = (BufferType)(255 << (kBufferFracBits - 8));
		*dst++ = (BufferType)HFILTER_SHIFT(r, fsk24RGBRedBits);
		*dst++ = (BufferType)HFILTER_SHIFT(g, fsk24RGBGreenBits);
		*dst++ = (BufferType)HFILTER_SHIFT(b, fsk24RGBBlueBits);
	}
}


/******************************************************************************
 * 16RGB565SE dst
 ******************************************************************************/

static void
VDot16RGB565SE(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt16	*d = (UInt16*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16		*f = filter->coeffs;
		const BufferType	*s = src;
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}
		k  = VFILTER_SHIFT(r, fsk16RGB565SERedBits)   << fsk16RGB565SERedPosition;
		k |= VFILTER_SHIFT(g, fsk16RGB565SEGreenBits) << fsk16RGB565SEGreenPosition;
		k |= VFILTER_SHIFT(b, fsk16RGB565SEBlueBits)  << fsk16RGB565SEBluePosition;
		*d++ = (UInt16)k;
	}
}


/******************************************************************************
 * 16RGB565DE src
 ******************************************************************************/

static void
HDot16RGB565DE(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt16			*s0		= (const UInt16*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16	*f	= filter->coeffs;
		const UInt16	*s = s0 + GET_FILTER_OFFSET(filter);
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--;) {
			SInt32 coeff = *f++;
			UInt32 pix   = *s++;
			UInt8 rgb[3];
			fskConvert16RGB565DE24RGB(pix, rgb[0]);
			r += coeff * (SInt32)(rgb[0]);
			g += coeff * (SInt32)(rgb[1]);
			b += coeff * (SInt32)(rgb[2]);
		}
		*dst++ = (BufferType)(255 << (kBufferFracBits - 8));
		*dst++ = (BufferType)HFILTER_SHIFT(r, fsk24RGBRedBits);
		*dst++ = (BufferType)HFILTER_SHIFT(g, fsk24RGBGreenBits);
		*dst++ = (BufferType)HFILTER_SHIFT(b, fsk24RGBBlueBits);
	}
}


/******************************************************************************
 * 16RGB565DE dst
 ******************************************************************************/

static void
VDot16RGB565DE(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt16	*d = (UInt16*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16		*f = filter->coeffs;
		const BufferType	*s = src;
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}
		k  = VFILTER_SHIFT(r, fsk16RGB565DERedBits)   << fsk16RGB565DERedPosition;
		k |= VFILTER_SHIFT(g, fsk16RGB565DEGreenBits) << fsk16RGB565DEGreenPosition;
		k |= VFILTER_SHIFT(b, fsk16RGB565DEBlueBits)  << fsk16RGB565DEBluePosition;
		k |= k >> 16;
		*d++ = (UInt16)k;
	}
}



/******************************************************************************
 * 16BGR565SE src
 ******************************************************************************/

static void
HDot16BGR565SE(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt16			*s0		= (const UInt16*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16	*f	= filter->coeffs;
		const UInt16	*s = s0 + GET_FILTER_OFFSET(filter);
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--;) {
			SInt32 coeff = *f++;
			UInt32 pix   = *s++;
			UInt8 rgb[3];
			fskConvert16BGR565SE24RGB(pix, rgb[0]);
			r += coeff * (SInt32)(rgb[0]);
			g += coeff * (SInt32)(rgb[1]);
			b += coeff * (SInt32)(rgb[2]);
		}
		*dst++ = (BufferType)(255 << (kBufferFracBits - 8));
		*dst++ = (BufferType)HFILTER_SHIFT(r, fsk24RGBRedBits);
		*dst++ = (BufferType)HFILTER_SHIFT(g, fsk24RGBGreenBits);
		*dst++ = (BufferType)HFILTER_SHIFT(b, fsk24RGBBlueBits);
	}
}


/******************************************************************************
 * 16BGR565SE dst
 ******************************************************************************/

static void
VDot16BGR565SE(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt16	*d = (UInt16*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16		*f = filter->coeffs;
		const BufferType	*s = src;
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}
		k  = VFILTER_SHIFT(r, fsk16BGR565SERedBits)   << fsk16BGR565SERedPosition;
		k |= VFILTER_SHIFT(g, fsk16BGR565SEGreenBits) << fsk16BGR565SEGreenPosition;
		k |= VFILTER_SHIFT(b, fsk16BGR565SEBlueBits)  << fsk16BGR565SEBluePosition;
		*d++ = (UInt16)k;
	}
}


#if TARGET_RT_BIG_ENDIAN
/******************************************************************************
 * 16BGR565DE src
 ******************************************************************************/

static void
HDot16BGR565DE(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt16			*s0		= (const UInt16*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride)) {
		const SInt16	*f	= filter->coeffs;
		const UInt16	*s = s0 + GET_FILTER_OFFSET(filter);
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--;) {
			SInt32 coeff = *f++;
			UInt32 pix   = *s++;
			UInt8 rgb[3];
			fskConvert16BGR565DE24RGB(pix, rgb[0]);
			r += coeff * (SInt32)(rgb[0]);
			g += coeff * (SInt32)(rgb[1]);
			b += coeff * (SInt32)(rgb[2]);
		}
		*dst++ = (BufferType)(255 << (kBufferFracBits - 8));
		*dst++ = (BufferType)HFILTER_SHIFT(r, fsk24RGBRedBits);
		*dst++ = (BufferType)HFILTER_SHIFT(g, fsk24RGBGreenBits);
		*dst++ = (BufferType)HFILTER_SHIFT(b, fsk24RGBBlueBits);
	}
}


/******************************************************************************
 * 16BGR565DE dst
 ******************************************************************************/

static void
VDot16BGR565DE(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt16	*d = (UInt16*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4) {
		const SInt16		*f = filter->coeffs;
		const BufferType	*s = src;
		UInt32 k;
		SInt32 r, g, b;
		for (k = filter->filterLength, r = 0, g = 0, b = 0; k--; s = (const BufferType*)((const char*)s + scaler->bufferRowBytes)) {
			SInt32 coeff = *f++;
			r += coeff * s[1];
			g += coeff * s[2];
			b += coeff * s[3];
		}
		k  = VFILTER_SHIFT(r, fsk16BGR565DERedBits)   << fsk16BGR565DERedPosition;
		k |= VFILTER_SHIFT(g, fsk16BGR565DEGreenBits) << fsk16BGR565DEGreenPosition;
		k |= VFILTER_SHIFT(b, fsk16BGR565DEBlueBits)  << fsk16BGR565DEBluePosition;
		k |= k >> 16;
		*d++ = (UInt16)k;
	}
}
#endif /* TARGET_RT_BIG_ENDIAN */


#ifdef SUPPORT_NEON

void HDot32ARGB_arm_v7_s(SInt32 filter_len, const SInt16 *filter_coeffs, const UInt32 *src, BufferType *dst);
void VDot32ARGB_arm_v7_s(SInt32 filter_len, const SInt16 *filter_coeffs, const BufferType *src, UInt32 *dst, SInt32 rb);
void HDot32BGRA_arm_v7_s(SInt32 filter_len, const SInt16 *filter_coeffs, const UInt32 *src, BufferType *dst);
void VDot32BGRA_arm_v7_s(SInt32 filter_len, const SInt16 *filter_coeffs, const BufferType *src, UInt32 *dst, SInt32 rb);
void HDot32RGBA_arm_v7_s(SInt32 filter_len, const SInt16 *filter_coeffs, const UInt32 *src, BufferType *dst);
void VDot32RGBA_arm_v7_s(SInt32 filter_len, const SInt16 *filter_coeffs, const BufferType *src, UInt32 *dst, SInt32 rb);
void HDot32ABGR_arm_v7_s(SInt32 filter_len, const SInt16 *filter_coeffs, const UInt32 *src, BufferType *dst);
void VDot32ABGR_arm_v7_s(SInt32 filter_len, const SInt16 *filter_coeffs, const BufferType *src, UInt32 *dst, SInt32 rb);
void HDot16RGB565SE_arm_v7_s(SInt32 filter_len, const SInt16 *filter_coeffs, const UInt16 *src, BufferType *dst);
void VDot16RGB565SE_arm_v7_s(SInt32 filter_len, const SInt16 *filter_coeffs, const BufferType *src, UInt16 *dst, SInt32 rb);

/******************************************************************************
 * 32ARGB src
 ******************************************************************************/

static void
HDot32ARGB_arm_v7(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt32			*s0		= (const UInt32*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride), dst += 4) {
		const SInt16 *f	= filter->coeffs;
		const UInt32 *s = s0 + GET_FILTER_OFFSET(filter);

		SInt32 k = filter->filterLength;

		HDot32ARGB_arm_v7_s(k, f, s, dst);
	}
}


/******************************************************************************
 * 32ARGB dst
 ******************************************************************************/

static void
VDot32ARGB_arm_v7(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt32	*d = (UInt32*)dst;
	UInt32	w;


	for (w = scaler->dstWidth; w--; src += 4, d++) {
		const SInt16 *f	= filter->coeffs;
		const BufferType  *s = src;
		SInt32 k = filter->filterLength;
		SInt32 rb = scaler->bufferRowBytes;

		VDot32ARGB_arm_v7_s(k, f, s, d, rb);
	}
}

/******************************************************************************
 * 32BGRA src
 ******************************************************************************/

static void
HDot32BGRA_arm_v7(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt32			*s0		= (const UInt32*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride), dst += 4) {
		const SInt16 *f	= filter->coeffs;
		const UInt32 *s = s0 + GET_FILTER_OFFSET(filter);

		SInt32 k = filter->filterLength;

		HDot32BGRA_arm_v7_s(k, f, s, dst);
	}
}


/******************************************************************************
 * 32BGRA dst
 ******************************************************************************/

static void
VDot32BGRA_arm_v7(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt32	*d = (UInt32*)dst;
	UInt32	w;


	for (w = scaler->dstWidth; w--; src += 4, d++) {
		const SInt16 *f	= filter->coeffs;
		const BufferType  *s = src;
		SInt32 k = filter->filterLength;
		SInt32 rb = scaler->bufferRowBytes;

		VDot32BGRA_arm_v7_s(k, f, s, d, rb);
	}
}

/******************************************************************************
 * 32ABGR src
 ******************************************************************************/

static void
HDot32ABGR_arm_v7(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt32			*s0		= (const UInt32*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride), dst += 4) {
		const SInt16 *f	= filter->coeffs;
		const UInt32 *s = s0 + GET_FILTER_OFFSET(filter);

		SInt32 k = filter->filterLength;

		HDot32ABGR_arm_v7_s(k, f, s, dst);
	}
}


/******************************************************************************
 * 32ABGR dst
 ******************************************************************************/

static void
VDot32ABGR_arm_v7(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt32	*d = (UInt32*)dst;
	UInt32	w;


	for (w = scaler->dstWidth; w--; src += 4, d++) {
		const SInt16 *f	= filter->coeffs;
		const BufferType  *s = src;
		SInt32 k = filter->filterLength;
		SInt32 rb = scaler->bufferRowBytes;

		VDot32ABGR_arm_v7_s(k, f, s, d, rb);
	}
}

/******************************************************************************
 * 32RGBA src
 ******************************************************************************/

static void
HDot32RGBA_arm_v7(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt32			*s0		= (const UInt32*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride), dst += 4) {
		const SInt16 *f	= filter->coeffs;
		const UInt32 *s = s0 + GET_FILTER_OFFSET(filter);

		SInt32 k = filter->filterLength;

		HDot32RGBA_arm_v7_s(k, f, s, dst);
	}
}


/******************************************************************************
 * 32RGBA dst
 ******************************************************************************/

static void
VDot32RGBA_arm_v7(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt32	*d = (UInt32*)dst;
	UInt32	w;


	for (w = scaler->dstWidth; w--; src += 4, d++) {
		const SInt16 *f	= filter->coeffs;
		const BufferType  *s = src;
		SInt32 k = filter->filterLength;
		SInt32 rb = scaler->bufferRowBytes;

		VDot32RGBA_arm_v7_s(k, f, s, d, rb);
	}
}


/******************************************************************************
 * 16RGB565SE src
 ******************************************************************************/

static void
HDot16RGB565SE_arm_v7(FskAAScaler scaler, const void *src, BufferType *dst)
{
	const PolyphaseFilter	*filter	= scaler->hFilterBank;
	const UInt16			*s0		= (const UInt16*)src;
	UInt32 w;

	for (w = scaler->dstWidth; w--; NEXT_CFILTER(filter, scaler->hFilterBankStride), dst += 4) {
		const SInt16	*f	= filter->coeffs;
		const UInt16	*s = s0 + GET_FILTER_OFFSET(filter);
		UInt32 k = filter->filterLength;

		HDot16RGB565SE_arm_v7_s(k, f, s, dst);
	}
}

/******************************************************************************
 * 16RGB565SE dst
 ******************************************************************************/

static void
VDot16RGB565SE_arm_v7(FskAAScaler scaler, const PolyphaseFilter *filter, const BufferType *src, void *dst)
{
	UInt16	*d = (UInt16*)dst;
	UInt32	w;

	for (w = scaler->dstWidth; w--; src += 4, d++) {
		const SInt16		*f = filter->coeffs;
		const BufferType	*s = src;
		UInt32 k = filter->filterLength;
		SInt32 rb = scaler->bufferRowBytes;

		VDot16RGB565SE_arm_v7_s(k, f, s, d, rb);
	}
}


#endif /* SUPPORT_NEON */

/******************************************************************************
 ******************************************************************************
 ** Support for building filters.
 ******************************************************************************
 ******************************************************************************/


/********************************************************************************
 * RemoveLeadingAndTrailingFilterBankZeroCoefficients
 ********************************************************************************/

static void
RemoveLeadingAndTrailingFilterBankZeroCoefficients(PolyphaseFilter *filterBank, int filterStride, int numPhases)
{
	SInt16	*f, *g;
	int		i, width, trim;
	int		offset, lastOffset;

	filterBank = (PolyphaseFilter*)((char*)filterBank + (numPhases - 1) * filterStride);	/* Advance to last phase */

	for (lastOffset = 0x7FFFFFFF; numPhases--; filterBank = (PolyphaseFilter*)((char*)filterBank - filterStride), lastOffset = offset) {
		width = filterBank->filterLength;

		/* Trim the end */
		for (i = width, f = &filterBank->coeffs[width - 1]; i-- && (*f == 0); f--)
			width--;

		/* Trim the beginning */
		offset = GET_FILTER_OFFSET(filterBank);
		for (i = width, trim = 0, f = &filterBank->coeffs[0]; i-- && (*f == 0); f++)
			trim++;
		if ((offset + trim) > lastOffset)
			trim = lastOffset - offset;	/* Keep the offsets monotonic at the expense of multiplies by zero */
		if (trim > 0) {
			width -= trim;
			for (i = width, f = &filterBank->coeffs[0], g = f + trim; i--; )
				*f++ = *g++;
			offset += trim;
			SET_FILTER_OFFSET(filterBank, offset);
		}

		filterBank->filterLength = (UInt16)width;
	}
}


/********************************************************************************
 * TweakFilter
 * Distribute excess to strongest coefficients, to assure that coefficients add up to unity.
 ********************************************************************************/

static void
TweakFilter(SInt16 *filter, int width, int extra)
{
	SInt16	*f, *fMax;
	int		i, max, t, nMax;

	if (width > 0) {
		for (i = width, f = fMax = filter, max = -kFilterScale, nMax = 0; i--; f++) {
			if (max < (t = *f)) {
				max = t;
				nMax = 1;
				fMax = f;
			}
			else if (max == t)
				nMax++;
		}
		if ((t = extra / nMax) != 0) {
			for (i = nMax, f = fMax; i--; )
				*f++ += (short)t;
			extra -= t * nMax;
		}
		fMax[nMax >> 1] += (short)extra;
	}
}


/********************************************************************************
 * RescaleFilter
 * Use this after adjustment, to assure that coefficients add again to unity.
 ********************************************************************************/

static void
RescaleFilter(SInt16 *filter, int width)
{
	int		j, sum;
	SInt16	*f;
	double	norm;

	for (j = width, f = filter, sum = 0; j--; )
		sum += *f++;
	if (sum != 0) {
		norm = (double)kFilterScale / sum;
		for (j = width, f = filter, sum = 0; j--; f++)
			sum += (*f = (SInt16)lround(*f * norm));
		if ((sum -= kFilterScale) != 0)
			TweakFilter(filter, width, -sum);
	}
}


/********************************************************************************
 * ChopFilterBoundary
 ********************************************************************************/

static void
ChopFilterBoundary(PolyphaseFilter *filterBank, int filterStride, int numPhases, int srcSize)
{
	PolyphaseFilter	*filter;
	SInt16	*p, *q;
	int		i, j, offset, width;

	/* Truncate the beginning and distribute to nearby coefficients */
	for (i = (int)numPhases, filter = filterBank; i--; NEXT_FILTER(filter, filterStride)) {
		if ((offset = GET_FILTER_OFFSET(filter)) >= 0)		/* Get offset (filter points to offset initially) */
			break;
		SET_FILTER_OFFSET(filter, 0);						/* Set negative offset to zero */
		if ((width = filter->filterLength + offset) < 0)	/* Make width shorter */
			width = 0;
		filter->filterLength = (UInt16)width;				/* Record the updated width (filter now points to first coefficient) */
		for (j = width, p = &filter->coeffs[0], q = &filter->coeffs[-offset]; j--; )
			*p++ = *q++;									/* Shift coefficients over */
		RescaleFilter(filter->coeffs, width);
	}

	/* Truncate the end and distribute to nearby coefficients */
	filter = (PolyphaseFilter*)((char*)filterBank + (numPhases - 1) * filterStride);	/* Advance to last phase */
	for (i = numPhases; i--; filter = (PolyphaseFilter*)((char*)filter - filterStride)) {
		offset = GET_FILTER_OFFSET(filter);					/* Get offset (filter points to offset initially) */
		width = filter->filterLength;
		if ((j = offset + width - srcSize) <= 0)
			break;
		if ((width -= j) < 0)								/* Make width shorter */
			width = 0;
		filter->filterLength = (UInt16)width;				/* Update width (advance filter to point to coefficients) */
		RescaleFilter(filter->coeffs, width);
	}
}


/******************************************************************************
 * MakeFilterBank
 ******************************************************************************/

static void
MakeFilterBank(
	MakeFilterProc	makeFilter,		/* This computes one filter */
	double			kernelRadius,	/* radius of the kernel - in source space */
	const void		*userData,		/* Extra parameters for makefilter */
	UInt32			support,		/* The maximum number of coefficients per filter */
	UInt32			numPhases,		/* The number of filters to be implemented */
	UInt32			srcSize,		/* The number of elements in the source */
	double			phase,			/* Phase of the first filter */
	double			phaseInc,		/* Increment of phase between successive filters */
	double			tapDeltaPhase,	/* The increment in phase between the taps of the filter */
	float			*tmpFilter,		/* A place to store the temporary filter, prior to conditioning */
	PolyphaseFilter	*filterBank		/* The resultant filter bank */
) {
	int				filterStride = sizeof(filterBank[0]) + sizeof(filterBank->coeffs[0]) * (support - 1);
	int				tapOffset, tapWidth, i, n, backup, iNorm;
	PolyphaseFilter	*filter;
	SInt16			*c;
	double			tapPhase;
	float			norm, *d;

	for (n = numPhases, filter = filterBank; n--; NEXT_FILTER(filter, filterStride), phase += phaseInc) {
		tapOffset = (int)floor(phase);					/* Index of center tap */
		tapPhase = (tapOffset - phase) * tapDeltaPhase;	/* Phase of center tap */
		backup = (int)((tapPhase + kernelRadius) / tapDeltaPhase);
		tapPhase -= backup * tapDeltaPhase;				/* Phase of leading tap */
		if (tapPhase <= -kernelRadius) {				/* Skip zero coefficients */
			backup--;
			tapPhase += tapDeltaPhase;
		}
		tapWidth = (int)((kernelRadius - tapPhase) / tapDeltaPhase);
		if ((tapPhase + tapWidth * tapDeltaPhase) < kernelRadius)
			tapWidth++;

		norm = (*makeFilter)(userData, tapWidth, tapPhase, tapDeltaPhase, tmpFilter);

		norm = kFilterScale / norm;
		tapOffset -= backup;
		SET_FILTER_OFFSET(filter, tapOffset);
		filter->filterLength = tapWidth;
		for (i = tapWidth, d = tmpFilter, c = filter->coeffs, iNorm = 0; i--; )
			iNorm += (*c++ = (SInt16)lround(*d++ * norm));
		for (i = (int)support - tapWidth; i-- > 0; )
			*c++ = 0;
		if ((iNorm -= kFilterScale) != 0)
			TweakFilter(filter->coeffs, tapWidth, -iNorm);
	}

	ChopFilterBoundary(filterBank, filterStride, numPhases, srcSize);

	RemoveLeadingAndTrailingFilterBankZeroCoefficients(filterBank, filterStride, numPhases);
}


/******************************************************************************
 * MakeTentFilter
 ******************************************************************************/

static float
MakeTentFilter(const void *userData, UInt32 filterSupport, double tapPhase, double tapDeltaPhase, float *filter)
{
	float sum = 0;

	for (; filterSupport--; tapPhase += tapDeltaPhase, ++filter) {
		float val = (float)(1 - fabs(tapPhase));
		if (val < 0)
			val = 0;
		*filter = val;
		sum += val;
	}

	return sum;
}


/********************************************************************************
 * MakeTentFilterBank
 ********************************************************************************/

#define TENT_KERNEL_RADIUS 1.0

static void
MakeTentFilterBank(
	UInt32	support,
	UInt32	numPhases,
	UInt32 	srcSize,
	double	initialPhase,
	double	scale,
	float	*tmpFilter,
	PolyphaseFilter	*filterBank
) {
	double phaseInc, tapDeltaPhase;

	phaseInc		= 1.0 / scale;						/* The phase increment between filters */
	tapDeltaPhase	= (scale < 1.0) ? scale : 1.0;		/* The phase delta between taps */

	MakeFilterBank(
		MakeTentFilter,
		TENT_KERNEL_RADIUS,
		NULL,
		support,
		numPhases,
		srcSize,
		initialPhase,
		phaseInc,
		tapDeltaPhase,
		tmpFilter,
		filterBank
	);
}


/********************************************************************************
 * MakeLanczosFilter
 ********************************************************************************/

static float
MakeLanczosFilter(const void *userData, UInt32 support, double phase, double phaseInc, float *filter)
{
	const double	*cosSinTapHalfDelta	= (const double*)userData;
	double			cosTapHalfDelta		= cosSinTapHalfDelta[0];
	double			sinTapHalfDelta		= cosSinTapHalfDelta[1];
	double			cosTapHalfPhase, sinTapHalfPhase, t, sum;

	for (sum = 0, cosTapHalfPhase = cos(phase * M_PI_2), sinTapHalfPhase = sin(phase * M_PI_2); support--;
		filter++,
		phase			+= phaseInc,
		t               =  cosTapHalfPhase * cosTapHalfDelta - sinTapHalfPhase * sinTapHalfDelta,
		sinTapHalfPhase =  sinTapHalfPhase * cosTapHalfDelta + cosTapHalfPhase * sinTapHalfDelta,
		cosTapHalfPhase =  t
	) {
		float val = (fabs(phase) < 1e-7) ?
			1.0f :
			(float)((sinTapHalfPhase * sinTapHalfPhase * cosTapHalfPhase) / (phase * phase * (M_PI_2 * M_PI_2)));
		*filter = val;
		sum += val;
	}

	return (float)sum;
}


/********************************************************************************
 * MakeLanczosFilterBank
 ********************************************************************************/

#define LANCZOS_KERNEL_RADIUS 2.0

static void
MakeLanczosFilterBank(
	UInt32	support,
	UInt32	numPhases,
	UInt32 	srcSize,
	double	initialPhase,
	double	scale,
	float	*tmpFilter,
	PolyphaseFilter	*filterBank
) {
	double phaseInc, tapDeltaPhase, theta, cosineSine[2];

	phaseInc		= 1.0 / scale;						/* The phase increment between filters */
	tapDeltaPhase	= (scale < 1.0) ? scale : 1.0;		/* The phase delta between taps */
	theta			= tapDeltaPhase * M_PI_2;			/* Delta theta between taps */
	cosineSine[0]	= cos(theta);						/* for rotating taps */
	cosineSine[1]	= sin(theta);						/* for rotating taps */

	MakeFilterBank(
		MakeLanczosFilter,
		LANCZOS_KERNEL_RADIUS,
		cosineSine,
		support,
		numPhases,
		srcSize,
		initialPhase,
		phaseInc,
		tapDeltaPhase,
		tmpFilter,
		filterBank
	);
}


/******************************************************************************
 ******************************************************************************
 ** The scaling machine.
 ******************************************************************************
 ******************************************************************************/


/******************************************************************************
 * FskAAScalerRun
 ******************************************************************************/

void
FskAAScalerRun(FskAAScaler scaler)
{
	const UInt8	*src	= (const UInt8*)(scaler->getNextSrcLine(scaler->userData));
	UInt8		*dst	= (      UInt8*)(scaler->getNextDstLine(scaler->userData, 0));
	int			offset, bufShift, filterHeight, i;

	for (scaler->dstY = 0, scaler->vFilter = scaler->vFilterBank, scaler->srcY = scaler->bufY = offset = GET_FILTER_OFFSET(scaler->vFilter); scaler->dstY < scaler->dstHeight;
			NEXT_CFILTER(scaler->vFilter, scaler->vFilterBankStride), ++(scaler->dstY), dst = (UInt8*)(scaler->getNextDstLine(scaler->userData, 0))
		) {
		offset = GET_FILTER_OFFSET(scaler->vFilter);
		filterHeight = scaler->vFilter->filterLength;
		if ((bufShift = offset - scaler->bufY) > 0) {
			FskMemMove(	scaler->buffer,
						(char*)(scaler->buffer) + bufShift * scaler->bufferRowBytes,
						(int)((scaler->vFilterSupport - bufShift) * scaler->bufferRowBytes)
			);
			scaler->bufY = offset;
		}
		for (i = filterHeight - (scaler->srcY - scaler->bufY); i-- > 0; ++(scaler->srcY), src = (const UInt8*)(scaler->getNextSrcLine(scaler->userData)))
			(*scaler->hDot)(scaler, src, (BufferType*)((char*)(scaler->buffer) + (scaler->srcY - scaler->bufY) * scaler->bufferRowBytes));
		(*scaler->vDot)(scaler, scaler->vFilter, scaler->buffer, dst);
	}
	scaler->getNextDstLine(scaler->userData, 1);
}


/******************************************************************************
 * FskAAScalerNextLine
 ******************************************************************************/

FskErr FskAAScalerNextLine(FskAAScaler scaler, const void *src) {
	FskErr err = kFskErrNone;
	BAIL_IF_FALSE(scaler && src, err, kFskErrInvalidParameter);										/* Check for valid parameters */
	(*scaler->hDot)(scaler, src, (BufferType*)((char*)(scaler->buffer) + (scaler->srcY - scaler->bufY) * scaler->bufferRowBytes));	/* Decimate horizontally into buffer */
	++(scaler->srcY);																				/* Advance the src line */
	while (scaler->srcY - scaler->bufY >= scaler->vFilter->filterLength) {							/* If we have enough to output a line, ... */
		int offset, bufShift;
		UInt8 *dst = (UInt8*)(scaler->getNextDstLine(scaler->userData, 0));							/* Get a place to put the output line */
		(*scaler->vDot)(scaler, scaler->vFilter, scaler->buffer, dst);								/* Decimate the buffer into the output line */
		if (++(scaler->dstY) == scaler->dstHeight) {												/* We have filtered the last line */
			scaler->getNextDstLine(scaler->userData, 1);											/* Signal the dst to flush */
			break;																					/* Done */
		}
		NEXT_CFILTER(scaler->vFilter, scaler->vFilterBankStride);									/* Get the next output line's filter */
		offset = GET_FILTER_OFFSET(scaler->vFilter);												/* Determine the start of the src line for the filter */
		if ((bufShift = offset - scaler->bufY) > 0) {												/* Determine how much we need to shift to get top srcLine to the top of the buffer */
			FskMemMove(	scaler->buffer,																/* Shift the buffer */
						(char*)(scaler->buffer) + bufShift * scaler->bufferRowBytes,
						(int)((scaler->vFilterSupport - bufShift) * scaler->bufferRowBytes)
			);
			scaler->bufY = offset;																	/* Record the curent position of the buffer */
		}
	}
bail:
	return err;
}


/******************************************************************************
 * AAScalerAlloc
 ******************************************************************************/

static FskErr AAScalerAlloc(
	int							kernelType,
	UInt32						srcWidth,
	UInt32						srcHeight,
	FskBitmapFormatEnum			srcPixelFormat,
	UInt32						dstWidth,
	UInt32						dstHeight,
	FskBitmapFormatEnum			dstPixelFormat,
	double						hScale,
	double						vScale,
	double						hOffset,
	double						vOffset,
	FskAAScaler					*pScaler
) {
	FskErr				err				= kFskErrNone;
	MakeFilterBankProc	makeFilterBank	= NULL;
	FskAAScaler			scaler			= NULL;
	FskAAScalerRecord	scalerParams;
	UInt32				hFilterBytes, vFilterBytes, bufferBytes, workingBytes, neededBytes, kernelSupport;
	float				*workingMem;

	*pScaler = NULL;

	switch (srcPixelFormat) {
		case kFskBitmapFormat32ARGB:		scalerParams.hDot = FskName2(HDot, fsk32ARGBFormatKind);		break;
		case kFskBitmapFormat32BGRA:		scalerParams.hDot = FskName2(HDot, fsk32BGRAFormatKind);		break;
		case kFskBitmapFormat32RGBA:		scalerParams.hDot = FskName2(HDot, fsk32RGBAFormatKind);		break;
		case kFskBitmapFormat32ABGR:		scalerParams.hDot = FskName2(HDot, fsk32ABGRFormatKind);		break;
#if TARGET_RT_LITTLE_ENDIAN
		case kFskBitmapFormat32A16RGB565LE:	scalerParams.hDot = FskName2(HDot, fsk32A16RGB565LEFormatKind);	break;
#endif /* TARGET_RT_LITTLE_ENDIAN */
		case kFskBitmapFormat24BGR:			scalerParams.hDot = HDot24BGR;									break;
		case kFskBitmapFormat24RGB:			scalerParams.hDot = HDot24RGB;									break;
		case kFskBitmapFormat16BGR565LE:	scalerParams.hDot = FskName2(HDot, fsk16BGR565LEFormatKind);	break;
		case kFskBitmapFormat16RGB565BE:	scalerParams.hDot = FskName2(HDot, fsk16RGB565BEFormatKind);	break;
		case kFskBitmapFormat16RGB565LE:	scalerParams.hDot = FskName2(HDot, fsk16RGB565LEFormatKind);	break;
		case kFskBitmapFormat8G:			scalerParams.hDot = HDot8G;										break;
		case kFskBitmapFormat16AG:
		case kFskBitmapFormat16RGB5515LE:
		case kFskBitmapFormat16RGBA4444LE:
		case kFskBitmapFormat8A:
		default:							err = kFskErrUnsupportedPixelType;	return err;
	}
	switch (dstPixelFormat) {
		case kFskBitmapFormat32ARGB:		scalerParams.vDot = FskName2(VDot, fsk32ARGBFormatKind);		break;
		case kFskBitmapFormat32BGRA:		scalerParams.vDot = FskName2(VDot, fsk32BGRAFormatKind);		break;
		case kFskBitmapFormat32RGBA:		scalerParams.vDot = FskName2(VDot, fsk32RGBAFormatKind);		break;
		case kFskBitmapFormat32ABGR:		scalerParams.vDot = FskName2(VDot, fsk32ABGRFormatKind);		break;
#if TARGET_RT_LITTLE_ENDIAN
		case kFskBitmapFormat32A16RGB565LE:	scalerParams.vDot = FskName2(VDot, fsk32A16RGB565LEFormatKind);	break;
#endif /* TARGET_RT_LITTLE_ENDIAN */
		case kFskBitmapFormat24BGR:			scalerParams.vDot = VDot24BGR;									break;
		case kFskBitmapFormat24RGB:			scalerParams.vDot = VDot24RGB;									break;
		case kFskBitmapFormat16BGR565LE:	scalerParams.vDot = FskName2(VDot, fsk16BGR565LEFormatKind);	break;
		case kFskBitmapFormat16RGB565BE:	scalerParams.vDot = FskName2(VDot, fsk16RGB565BEFormatKind);	break;
		case kFskBitmapFormat16RGB565LE:	scalerParams.vDot = FskName2(VDot, fsk16RGB565LEFormatKind);	break;
		case kFskBitmapFormat8G:			scalerParams.vDot = VDot8G;										break;
		case kFskBitmapFormat16AG:
		case kFskBitmapFormat16RGB5515LE:
		case kFskBitmapFormat16RGBA4444LE:
		case kFskBitmapFormat8A:
		default:							err = kFskErrUnsupportedPixelType;	return err;
	}

#ifdef SUPPORT_NEON
	int implementation = FskHardwareGetARMCPU_All();

	if (implementation == FSK_ARCH_ARM_V7) {
		switch (srcPixelFormat) {
			case kFskBitmapFormat32ARGB:		scalerParams.hDot = HDot32BGRA_arm_v7;		break;
			case kFskBitmapFormat32BGRA:		scalerParams.hDot = HDot32ARGB_arm_v7;		break;
			case kFskBitmapFormat32RGBA:		scalerParams.hDot = HDot32ABGR_arm_v7;		break;
			case kFskBitmapFormat32ABGR:		scalerParams.hDot = HDot32RGBA_arm_v7;		break;
			case kFskBitmapFormat16RGB565LE:	scalerParams.hDot = HDot16RGB565SE_arm_v7;	break;
			default:																		break;
		}
		switch (dstPixelFormat) {
			case kFskBitmapFormat32ARGB:		scalerParams.vDot = VDot32BGRA_arm_v7;		break;
			case kFskBitmapFormat32BGRA:		scalerParams.vDot = VDot32ARGB_arm_v7;		break;
			case kFskBitmapFormat32RGBA:		scalerParams.vDot = VDot32ABGR_arm_v7;		break;
			case kFskBitmapFormat32ABGR:		scalerParams.vDot = VDot32RGBA_arm_v7;		break;
			case kFskBitmapFormat16RGB565LE:	scalerParams.vDot = VDot16RGB565SE_arm_v7;	break;
			default:																		break;
		}
	}
#endif

	scalerParams.srcWidth	= srcWidth;
	scalerParams.srcHeight	= srcHeight;
	scalerParams.dstWidth	= dstWidth;
	scalerParams.dstHeight	= dstHeight;
	scalerParams.vFilter	= NULL;
	scalerParams.dstY		= 0;
	scalerParams.userData	= NULL;

	/* Determine memory needs: center + radius to the left + radius to the right */
	switch (kernelType) {
		default:
		case kAAScaleTentKernelType:	kernelSupport =    (UInt32)(TENT_KERNEL_RADIUS * 2 + 1);	makeFilterBank = MakeTentFilterBank;	break;
		case kAAScaleLanczosKernelType:	kernelSupport = (UInt32)(LANCZOS_KERNEL_RADIUS * 2 + 1);	makeFilterBank = MakeLanczosFilterBank;	break;
	}
	if (hScale <= 0)	hScale = (double)(dstWidth  - 1) / (srcWidth  - 1);
	if (vScale <= 0)	vScale = (double)(dstHeight - 1) / (srcHeight - 1);

	scalerParams.hFilterSupport		= (int)(ceil((hScale >= 1) ? kernelSupport : (kernelSupport / hScale)));
	scalerParams.vFilterSupport		= (int)(ceil((vScale >= 1) ? kernelSupport : (kernelSupport / vScale)));
	scalerParams.hFilterBankStride	= sizeof(scalerParams.hFilterBank[0]) + (scalerParams.hFilterSupport - 1) * sizeof(scalerParams.hFilterBank->coeffs[0]);
	scalerParams.vFilterBankStride	= sizeof(scalerParams.vFilterBank[0]) + (scalerParams.vFilterSupport - 1) * sizeof(scalerParams.vFilterBank->coeffs[0]);
	scalerParams.bufY				= 0;	/* Although this is set later, perhaps this will keep Coverity happy */
	scalerParams.srcY				= 0;	/* Although this is set later, perhaps this will keep Coverity happy */
	hFilterBytes					= scalerParams.dstWidth  * scalerParams.hFilterBankStride;
	vFilterBytes					= scalerParams.dstHeight * scalerParams.vFilterBankStride;
	scalerParams.bufferRowBytes		= scalerParams.dstWidth * 4 * sizeof(scalerParams.buffer[0]);	/* ARGB */
	bufferBytes						= scalerParams.bufferRowBytes * scalerParams.vFilterSupport;
	workingBytes = ((scalerParams.hFilterSupport > scalerParams.vFilterSupport) ? scalerParams.hFilterSupport : scalerParams.vFilterSupport) * sizeof(*workingMem);
	workingBytes += sizeof(*workingMem);	/* Might need a few extra bytes for alignment. */
	if (bufferBytes < workingBytes)			/* The buffer and working memory share the same memory, but use it at different times */
		bufferBytes = workingBytes;
	neededBytes = sizeof(FskAAScalerRecord) + hFilterBytes + vFilterBytes + bufferBytes;

	/* Allocate memory */
	BAIL_IF_ERR(err = FskMemPtrNew(neededBytes, pScaler));
	scaler = *pScaler;
	*scaler = scalerParams;
	scaler->hFilterBank	= (PolyphaseFilter*)(scaler + 1);
	scaler->vFilterBank	= (PolyphaseFilter*)((char*)(scaler->hFilterBank) + hFilterBytes);
	scaler->buffer		=      (BufferType*)((char*)(scaler->vFilterBank) + vFilterBytes);

	/* Initialize filters */
	workingMem = (float*)(((long)(scaler->buffer) + sizeof(*workingMem) - 1) & ~(sizeof(*workingMem) - 1));	/* Bump up to align to float */
	(*makeFilterBank)(scaler->hFilterSupport, scaler->dstWidth,  scaler->srcWidth,  hOffset, hScale, workingMem, scaler->hFilterBank);
	(*makeFilterBank)(scaler->vFilterSupport, scaler->dstHeight, scaler->srcHeight, vOffset, vScale, workingMem, scaler->vFilterBank);

bail:
	if (err)	FskMemPtrDisposeAt(pScaler);

	return(err);
}


/******************************************************************************
 * BitmapLineAdvance
 ******************************************************************************/

typedef struct BitmapLineAdvance {
	const UInt8 *src;
	UInt8 *dst;
	SInt32 srb,  drb;
} BitmapLineAdvance;

static const void *nextSrcBitmapLine(void *userData) {
	BitmapLineAdvance *a = (BitmapLineAdvance*)userData;
	const void *nextLine = a->src;
	a->src += a->srb;
	return nextLine;
}

static void *nextDstBitmapLine(void *userData, Boolean done) {
	BitmapLineAdvance *a = (BitmapLineAdvance*)userData;
	void *nextLine = a->dst;
	a->dst += a->drb;
	return nextLine;
}


/******************************************************************************
 ******************************************************************************
 ** API
 ******************************************************************************
 ******************************************************************************/


/******************************************************************************
 * FskAAScalerReset
 ******************************************************************************/

void FskAAScalerReset(FskAAScaler scaler) {
	scaler->srcY	= 0;
	scaler->dstY	= 0;
	scaler->bufY	= 0;
	scaler->vFilter	= scaler->vFilterBank;
}


/******************************************************************************
 * FskAAScalerNew
 ******************************************************************************/

FskErr FskAAScalerNew(
	int			kernelType,
	UInt32		srcWidth,	UInt32	srcHeight,	FskBitmapFormatEnum	srcPixelFormat,	FskAAScalerGetNextSrcLine	getNextSrcLine,
	UInt32		dstWidth,	UInt32	dstHeight,	FskBitmapFormatEnum	dstPixelFormat,	FskAAScalerGetNextDstLine	getNextDstLine,
	double		hScale,		double	vScale,
	double		hOffset,	double	vOffset,
	void		*userData,
	FskAAScaler	*pScaler
) {
	FskErr			err		= kFskErrNone;
	FskAAScaler		scaler	= NULL;

	if (pScaler)	*pScaler = NULL;

	BAIL_IF_ERR(err = AAScalerAlloc(kernelType, srcWidth, srcHeight, srcPixelFormat, dstWidth, dstHeight, dstPixelFormat, hScale, vScale, hOffset, vOffset, &scaler));
	FskAAScalerReset(scaler);
	scaler->getNextSrcLine	= getNextSrcLine;
	scaler->getNextDstLine	= getNextDstLine;
	scaler->userData		= userData;

	if (pScaler) {								/* Push mode or repetitive pull mode */
		*pScaler = scaler;
		scaler = NULL;
	}
	else if (getNextSrcLine && getNextDstLine)	/* Pull */
		FskAAScalerRun(scaler);

bail:
	FskMemPtrDispose(scaler);
	return err;
}


/******************************************************************************
 * FskAAScalerDispose
 ******************************************************************************/

FskErr FskAAScalerDispose(FskAAScaler scaler) {
	return FskMemPtrDispose(scaler);
}


/******************************************************************************
 * FskAAScaleBitmap
 ******************************************************************************/

FskErr
FskAAScaleBitmap(
	int					kernelType,
	FskBitmap			srcBM,
	FskConstRectangle	srcRect,	/* Map this rect... */
	FskBitmap			dstBM,
	FskConstRectangle	dstRect		/* ...to this rect */
) {
	FskErr				err				= kFskErrNone;
	FskRectangleRecord	srcClipRect, dstClipRect;
	double				hScale, vScale, hOffset, vOffset;
	BitmapLineAdvance	bmla;

	#if SUPPORT_INSTRUMENTATION
		LOGD("FskAAScaleBitmap(kernel=%s srcBM=%p, srcRect=%p, dstBM=%p, dstRect=%p)", GetKernelNameFromCode(kernelType), srcBM, srcRect, dstBM, dstRect);
		LogBitmap(srcBM, "src"); LogRect(srcRect, "srcRect"); LogBitmap(dstBM, "dst"); LogRect(dstRect, "dstRect");
	#endif /* SUPPORT_INSTRUMENTATION */

	/* Intersect srcRect and srcBounds */
	if (srcRect) {
		if (!FskRectangleIntersect(srcRect, &srcBM->bounds, &srcClipRect))
			return kFskErrNone;
	}
	else {
		srcClipRect = srcBM->bounds;
	}
	if (!(srcClipRect.width > 0 && srcClipRect.height > 0))
		return kFskErrNothingRendered;

	/* Intersect dstRect and dstBounds */
	if (dstRect) {
		if (!FskRectangleIntersect(dstRect, &dstBM->bounds, &dstClipRect))
			return kFskErrNone;
	}
	else {
		dstClipRect = dstBM->bounds;
	}

	FskBitmapReadBegin(srcBM, NULL, NULL, NULL);
	FskBitmapWriteBegin(dstBM, NULL, NULL, NULL);

	BAIL_IF_FALSE(srcClipRect.width > 1 && srcClipRect.height > 1, err, kFskErrParameterError);

	hScale = (double)(dstClipRect.width  - 1) / (srcClipRect.width  - 1);
	vScale = (double)(dstClipRect.height - 1) / (srcClipRect.height - 1);
	hOffset = 0;	/* @@@ Perhaps we want 1/2 phase and d/s instead of (d-1)/(s-1). */
	vOffset = 0;

	bmla.srb = srcBM->rowBytes;
	bmla.src = (const UInt8*)(srcBM->bits)
			- srcBM->bounds.y * bmla.srb
			- srcBM->bounds.x * (srcBM->depth >> 3);
	bmla.drb	= dstBM->rowBytes;
	bmla.dst	= (UInt8*)(dstBM->bits)
				+ (dstClipRect.y - dstBM->bounds.y) * bmla.drb
				+ (dstClipRect.x - dstBM->bounds.x) * (dstBM->depth >> 3);

	BAIL_IF_ERR(err = FskAAScalerNew(kernelType,
									srcBM->bounds.width,	srcBM->bounds.height,	srcBM->pixelFormat,	nextSrcBitmapLine,
									dstClipRect.width,		dstClipRect.height,		dstBM->pixelFormat,	nextDstBitmapLine,
									hScale,		vScale,		hOffset,		vOffset,
									&bmla, NULL));

bail:
	FskBitmapReadEnd(srcBM);
	FskBitmapWriteEnd(dstBM);

	return(err);
}


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****					BOX FILTER INTEGRAL DECIMATION						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


#define NORM_BITS			24

/****************************************************************************//**
 *	FskBoxDecimateKernel - decimate using a box kernel.
 *	\param[in]	pixBytes		the number of bytes between one pixel and the next horizontally, in the source and the destination.
 *	\param[in]	nComps			the number of contiguous components in the pixel.
 *	\param[in]	src				pointer to the first pixel in the source to be processed.
 *	\param[in]	srcRowBytes		the number of bytes between one pixel and the next vertically in the source.
 *	\param[in]	srcBlockWidth	the horizontal decimation ratio, i.e. the width  of a source block.
 *	\param[in]	srcBlockHeight	the  vertical  decimation ratio, i.e. the height of a source block.
 *	\param[out]	dst				pointer to the first pixel in the destination to be replaced.
 *	\param[in]	dstRowBytes		the number of bytes between one pixel and the next vertically in the destination.
 *	\param[in]	dstWidth		the width  of the destination to be replaced.
 *	\param[in]	dstHeight		the height of the destination to be replaced.
 *	\return		kFskErrNone	if the decimation was successful.
 ********************************************************************************/

static void FskBoxDecimateKernel(SInt32 pixBytes, UInt32 nComps,
					const	UInt8 *src, SInt32 srcRowBytes, UInt32 srcBlockWidth, UInt32 srcBlockHeight,
							UInt8 *dst, SInt32 dstRowBytes, UInt32 dstWidth,      UInt32 dstHeight
) {
	const UInt32	norm		= (1U << NORM_BITS) / (srcBlockWidth * srcBlockHeight);	/* We divide by multiplying by the reciprocal */
	const SInt32	dstBumpPix  = pixBytes                     - nComps         * sizeof(*dst);
	const SInt32	dstBumpRow  = dstRowBytes                  - dstWidth       * pixBytes;
	const SInt32	srcBumpCol  = pixBytes                     - srcBlockHeight * srcRowBytes;
	const SInt32	srcBumpComp = sizeof(*src)                 - srcBlockWidth  * pixBytes;
	const SInt32	srcBumpPix  = srcBlockWidth * pixBytes     - nComps         * sizeof(*src);
 	const SInt32	srcBumpRow  = srcBlockHeight * srcRowBytes - dstWidth       * srcBlockWidth * pixBytes;
	UInt32			acc, n, u, v, w;

	for (; dstHeight--; src += srcBumpRow, dst += dstBumpRow)
		for (w = dstWidth; w--; src += srcBumpPix, dst += dstBumpPix)
			for (n = nComps; n--; src += srcBumpComp, *dst++ = (acc * norm + (1 << (NORM_BITS - 1))) >> NORM_BITS)	/* TODO: parallelize */
				for (u = srcBlockWidth, acc = 0; u--; src += srcBumpCol)											/* TODO: unroll */
					for (v = srcBlockHeight; v--; src += srcRowBytes)												/* TODO: unroll */
						acc += *src;
}


/****************************************************************************//**
 *	FskBoxDecimateKernel - decimate using a box kernel for 565SE pixels.
 *	\param[in]	pixBytes		the number of bytes between one pixel and the next horizontally, in the source and the destination.
 *								This allows this routine to be used for 16XYZ565SE and 32A16XYZ565SE.
 *	\param[in]	src				pointer to the first pixel in the source to be processed.
 *	\param[in]	srcRowBytes		the number of bytes between one pixel and the next vertically in the source.
 *	\param[in]	srcBlockWidth	the horizontal decimation ratio, i.e. the width  of a source block.
 *	\param[in]	srcBlockHeight	the  vertical  decimation ratio, i.e. the height of a source block.
 *	\param[out]	dst				pointer to the first pixel in the destination to be replaced.
 *	\param[in]	dstRowBytes		the number of bytes between one pixel and the next vertically in the destination.
 *	\param[in]	dstWidth		the width  of the destination to be replaced.
 *	\param[in]	dstHeight		the height of the destination to be replaced.
 *	\return		kFskErrNone	if the decimation was successful.
 ********************************************************************************/

static void FskBoxDecimateKernel565SE(SInt32 pixBytes,
									const	UInt16 *src, SInt32 srcRowBytes, UInt32 srcBlockWidth, UInt32 srcBlockHeight,
											UInt16 *dst, SInt32 dstRowBytes, UInt32 dstWidth,      UInt32 dstHeight
) {
	const UInt32	norm		= (1U << NORM_BITS) / (srcBlockWidth * srcBlockHeight);	/* We divide by multiplying by the reciprocal */
	const SInt32	dstBumpRow  = dstRowBytes                  - dstWidth       * pixBytes;
	const SInt32	srcBumpCol  = pixBytes                     - srcBlockHeight * srcRowBytes;
 	const SInt32	srcBumpRow  = srcBlockHeight * srcRowBytes - dstWidth       * srcBlockWidth * pixBytes;
	UInt32			rAcc, gAcc, bAcc, u, v, w, pix;

	for (; dstHeight--; src = (const UInt16*)((const char*)src + srcBumpRow), dst = (UInt16*)((char*)dst + dstBumpRow)) {
		for (w = dstWidth; w--; dst = (UInt16*)((char*)dst + pixBytes)) {
			for (u = srcBlockWidth, rAcc = gAcc = bAcc = 0; u--; src = (const UInt16*)((const char*)src + srcBumpCol)) {
				for (v = srcBlockHeight; v--; src = (const UInt16*)((const char*)src + srcRowBytes)) {
					pix = *src;
					rAcc += FskMoveField(pix, fsk16RGB565SERedBits,   fsk16RGB565SERedPosition,   0);
					gAcc += FskMoveField(pix, fsk16RGB565SEGreenBits, fsk16RGB565SEGreenPosition, 0);
					bAcc += FskMoveField(pix, fsk16RGB565SEBlueBits,  fsk16RGB565SEBluePosition,  0);
				}
			}
			rAcc = (rAcc * norm + (1 << (NORM_BITS - 1))) >> NORM_BITS;
			gAcc = (gAcc * norm + (1 << (NORM_BITS - 1))) >> NORM_BITS;
			bAcc = (bAcc * norm + (1 << (NORM_BITS - 1))) >> NORM_BITS;
			*dst = (UInt16)((rAcc << fsk16RGB565SERedPosition) | (gAcc << fsk16RGB565SEGreenPosition) | (bAcc << fsk16RGB565SEBluePosition));
		}
	}
}


#if 1 /* (TARGET_RT_LITTLE_ENDIAN && DST_16RGB565BE) || (TARGET_RT_BIG_ENDIAN && DST_16RGB565LE) TODO: We should probably do this for Tent and Lanczos as well. */
	#define DST_16RGB565DE 1
/****************************************************************************//**
 *	FskBoxDecimateKernel - decimate using a box kernel for 565DE pixels.
 *	\param[in]	pixBytes		the number of bytes between one pixel and the next horizontally, in the source and the destination.
 *								This allows this routine to be used for 16XYZ565SE and 32A16XYZ565SE.
 *	\param[in]	src				pointer to the first pixel in the source to be processed.
 *	\param[in]	srcRowBytes		the number of bytes between one pixel and the next vertically in the source.
 *	\param[in]	srcBlockWidth	the horizontal decimation ratio, i.e. the width  of a source block.
 *	\param[in]	srcBlockHeight	the  vertical  decimation ratio, i.e. the height of a source block.
 *	\param[out]	dst				pointer to the first pixel in the destination to be replaced.
 *	\param[in]	dstRowBytes		the number of bytes between one pixel and the next vertically in the destination.
 *	\param[in]	dstWidth		the width  of the destination to be replaced.
 *	\param[in]	dstHeight		the height of the destination to be replaced.
 *	\return		kFskErrNone	if the decimation was successful.
 ********************************************************************************/

static void FskBoxDecimateKernel565DE(SInt32 pixBytes,
									const	UInt16 *src, SInt32 srcRowBytes, UInt32 srcBlockWidth, UInt32 srcBlockHeight,
											UInt16 *dst, SInt32 dstRowBytes, UInt32 dstWidth,      UInt32 dstHeight
) {
	const UInt32	norm		= (1U << NORM_BITS) / (srcBlockWidth * srcBlockHeight);	/* We divide by multiplying by the reciprocal */
	const SInt32	dstBumpRow  = dstRowBytes                  - dstWidth       * pixBytes;
	const SInt32	srcBumpCol  = pixBytes                     - srcBlockHeight * srcRowBytes;
 	const SInt32	srcBumpRow  = srcBlockHeight * srcRowBytes - dstWidth       * srcBlockWidth * pixBytes;
	UInt32			rAcc, gAcc, bAcc, u, v, w, pix;

	for (; dstHeight--; src = (const UInt16*)((const char*)src + srcBumpRow), dst = (UInt16*)((char*)dst + dstBumpRow)) {
		for (w = dstWidth; w--; dst = (UInt16*)((char*)dst + pixBytes)) {
			for (u = srcBlockWidth, rAcc = gAcc = bAcc = 0; u--; src = (const UInt16*)((const char*)src + srcBumpCol)) {
				for (v = srcBlockHeight; v--; src = (const UInt16*)((const char*)src + srcRowBytes)) {
					pix = *src;
					pix |= pix << 16;
					rAcc += FskMoveField(pix, fsk16RGB565DERedBits,   fsk16RGB565DERedPosition,   0);
					gAcc += FskMoveField(pix, fsk16RGB565DEGreenBits, fsk16RGB565DEGreenPosition, 0);
					bAcc += FskMoveField(pix, fsk16RGB565DEBlueBits,  fsk16RGB565DEBluePosition,  0);
				}
			}
			rAcc = (rAcc * norm + (1 << (NORM_BITS - 1))) >> NORM_BITS;
			gAcc = (gAcc * norm + (1 << (NORM_BITS - 1))) >> NORM_BITS;
			bAcc = (bAcc * norm + (1 << (NORM_BITS - 1))) >> NORM_BITS;
			pix = (rAcc << fsk16RGB565DERedPosition) | (gAcc << fsk16RGB565DEGreenPosition) | (bAcc << fsk16RGB565DEBluePosition);
			pix |= pix >> 16;
			*dst = (UInt16)pix;
		}
	}
}
#else /* no DIFFERENT_ENDIAN 565 */
	#define DST_16RGB565DE 0
#endif /* DIFFERENT_ENDIAN 565 */


/********************************************************************************
 * FskBoxDecimate
 ********************************************************************************/

FskErr FskBoxDecimate(FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstRectangle dstRect) {
	FskErr		err			= kFskErrNone;
	const UInt8	*s00		= NULL;
	UInt8		*d00		= NULL;
	const UInt8	*s;
	UInt8		*d;
	UInt32		xDec, yDec;
	SInt32		srcRowBytes, dstRowBytes, pixBytes;
	FskRectangleRecord	sRect, dRect, r;

	#if SUPPORT_INSTRUMENTATION
		LOGD("BoxDecimate(src=%p, srcRect=%p, dst=%p, dstRect=%p)", src, srcRect, dst, dstRect);
		LogBitmap(src, "src"); LogRect(srcRect, "srcRect"); LogBitmap(dst, "dst"); LogRect(dstRect, "dstRect");
	#endif /* SUPPORT_INSTRUMENTATION */

	BAIL_IF_FALSE(src->pixelFormat == dst->pixelFormat, err, kFskErrMismatch);						/* We only do like kinds */

	if (!srcRect)	srcRect = &src->bounds;
	if (!dstRect)	dstRect = &dst->bounds;

	xDec = srcRect->width  / dstRect->width;
	yDec = srcRect->height / dstRect->height;
	BAIL_IF_FALSE(	(srcRect->width  == (SInt32)(xDec * dstRect->width)) &&									/* We only do ... */
					(srcRect->height == (SInt32)(yDec * dstRect->height)), err, kFskErrParameterError);		/* ... integral decimations */

	FskRectangleIntersect(dstRect, &dst->bounds, &dRect);
	if (!FskRectangleIsEqual(dstRect, &dRect)) {													/* Need to clip against the destination */
		sRect.x      = xDec * (dRect.x - dstRect->x) + srcRect->x;
		sRect.y      = yDec * (dRect.y - dstRect->y) + srcRect->y,
		sRect.width  = xDec * dRect.width;
		sRect.height = yDec * dRect.height;
		dstRect = &dRect;
		srcRect = &sRect;
	}
	FskRectangleIntersect(srcRect, &src->bounds, &r);
	if (!FskRectangleIsEqual(srcRect, &r)) {														/* Need to clip against the source */
		r.width  = (r.width  - srcRect->x + r.x     ) / xDec + srcRect->x - r.x;					/* Quantize srcClip to blocks */
		r.x      = (r.x      - srcRect->x + xDec - 1) / xDec + srcRect->x;
		r.height = (r.height - srcRect->y + r.y     ) / yDec + srcRect->y - r.y;
		r.y      = (r.y      - srcRect->y + yDec - 1) / yDec + srcRect->y;
		dRect.x      = (r.x - srcRect->x) / xDec + dstRect->x;										/* Set dstRect based on source clipping*/
		dRect.y      = (r.x - srcRect->y) / yDec + dstRect->y;
		dRect.width  = r.width / xDec;
		dRect.height = r.height / yDec;
		dstRect = &dRect;
		sRect = r;																					/* Set srcRect */
		srcRect = &sRect;
	}

	BAIL_IF_ERR(err = FskBitmapReadBegin((FskBitmap)src, (const void**)(const void*)(&s00), &srcRowBytes, NULL));
	BAIL_IF_ERR(err = FskBitmapWriteBegin(          dst,       (void**)(      void*)(&d00), &dstRowBytes, NULL));
	pixBytes = dst->depth >> 3;
	s = s00 + srcRect->y * srcRowBytes + srcRect->x * pixBytes;
	d = d00 + dstRect->y * dstRowBytes + dstRect->x * pixBytes;

	switch (dst->pixelFormat) {
		case kFskBitmapFormat16AG:
		case kFskBitmapFormat24BGR:
		case kFskBitmapFormat24RGB:
		case kFskBitmapFormat32ABGR:
		case kFskBitmapFormat32ARGB:
		case kFskBitmapFormat32BGRA:
		case kFskBitmapFormat32RGBA:
		case kFskBitmapFormat8A:
		case kFskBitmapFormat8G:
			FskBoxDecimateKernel(pixBytes, pixBytes, s, srcRowBytes, xDec, yDec, d, dstRowBytes, dstRect->width, dstRect->height);
			break;

		case kFskBitmapFormatUYVY:
			FskBoxDecimateKernel(pixBytes,    1, s+1, srcRowBytes, xDec, yDec, d+1, dstRowBytes, dstRect->width,    dstRect->height);	/* Y - 2 bytes per pixel @ 1 */
			FskBoxDecimateKernel(pixBytes<<1, 1, s+0, srcRowBytes, xDec, yDec, d+0, dstRowBytes, dstRect->width>>1, dstRect->height);	/* U - 4 bytes per pixel @ 0 */
			FskBoxDecimateKernel(pixBytes<<1, 1, s+2, srcRowBytes, xDec, yDec, d+2, dstRowBytes, dstRect->width>>1, dstRect->height);	/* V - 4 bytes per pixel @ 2 */
			break;

		case kFskBitmapFormatYUV420:
			FskBoxDecimateKernel(1, 1, s, srcRowBytes, xDec, yDec, d, dstRowBytes, dstRect->width, dstRect->height);					/* Y */
			s = s00	+ src->bounds.height *  srcRowBytes
					+ (srcRect->y >> 1)  * (srcRowBytes >> 1) + (srcRect->x >> 1);
			d = d00	+ dst->bounds.height *  dstRowBytes
					+ (dstRect->y >> 1)  * (dstRowBytes >> 1) + (dstRect->x >> 1);
			FskBoxDecimateKernel(1, 1, s, srcRowBytes>>1, xDec, yDec, d, dstRowBytes>>1, dstRect->width>>1, dstRect->height>>1);		/* U */
			s += (src->bounds.height >> 1) * (srcRowBytes >> 1);
			d += (dst->bounds.height >> 1) * (dstRowBytes >> 1);
			FskBoxDecimateKernel(1, 1, s, srcRowBytes>>1, xDec, yDec, d, dstRowBytes>>1, dstRect->width>>1, dstRect->height>>1);		/* V */
			break;

		case kFskBitmapFormatYUV420spuv:
		case kFskBitmapFormatYUV420spvu:
			FskBoxDecimateKernel(1, 1, s, srcRowBytes, xDec, yDec, d, dstRowBytes, dstRect->width, dstRect->height);					/* Y */
			s = s00	+ src->bounds.height * srcRowBytes
					+ (srcRect->y >> 1)  * srcRowBytes + (srcRect->x >> 1) * 2;
			d = d00	+ dst->bounds.height * dstRowBytes
					+ (dstRect->y >> 1)  * dstRowBytes + (dstRect->x >> 1) * 2;
			FskBoxDecimateKernel(2, 2, s, srcRowBytes, xDec, yDec, d, dstRowBytes, dstRect->width>>1, dstRect->height>>1);				/* UV */
			break;

	#if TARGET_RT_LITTLE_ENDIAN
		case kFskBitmapFormat32A16RGB565LE:
			FskBoxDecimateKernel565SE(sizeof(UInt32),   (const UInt16*)s00, srcRowBytes, xDec, yDec, (UInt16*)d00, dstRowBytes, dstRect->width, dstRect->height);
			FskBoxDecimateKernel(     sizeof(UInt32), 2, s00 + 2,           srcRowBytes, xDec, yDec, d00 + 2,      dstRowBytes, dstRect->width, dstRect->height);
			break;
		case kFskBitmapFormat16RGB565LE:
		case kFskBitmapFormat16BGR565LE:
	#else /* TARGET_RT_BIG_ENDIAN */
		case kFskBitmapFormat16RGB565BE:
	#endif /* TARGET_RT_BIG_ENDIAN */
			FskBoxDecimateKernel565SE(sizeof(UInt16), (const UInt16*)s00, srcRowBytes, xDec, yDec, (UInt16*)d00, dstRowBytes, dstRect->width, dstRect->height);
			break;

	#if DST_16RGB565DE		/* Different endian 565 decimator has been generated */
		#if TARGET_RT_LITTLE_ENDIAN
			case kFskBitmapFormat16RGB565BE:
		#else /* TARGET_RT_BIG_ENDIAN */
			case kFskBitmapFormat16RGB565LE:
			case kFskBitmapFormat16BGR565LE:
		#endif /* TARGET_RT_BIG_ENDIAN */
				FskBoxDecimateKernel565DE(sizeof(UInt16), (const UInt16*)s00, srcRowBytes, xDec, yDec, (UInt16*)d00, dstRowBytes, dstRect->width, dstRect->height);
				break;
	#else					/* No different endian 565 decimator exists */
		#if TARGET_RT_LITTLE_ENDIAN
			case kFskBitmapFormat16RGB565BE:
		#else /* TARGET_RT_BIG_ENDIAN */
			case kFskBitmapFormat16RGB565LE:
			case kFskBitmapFormat16BGR565LE:
		#endif /* TARGET_RT_BIG_ENDIAN */
		 	err = kFskErrUnsupportedPixelType;
		 	break;
	#endif /* DST_16RGB565DE */

		case kFskBitmapFormatYUV420i:					/* Irregular layout */

        default:
		 	err = kFskErrUnsupportedPixelType;
		 	break;
	}

bail:
	if (d00)	FskBitmapWriteEnd(dst);
	if (s00)	FskBitmapReadEnd((FskBitmap)src);
	return err;
}
