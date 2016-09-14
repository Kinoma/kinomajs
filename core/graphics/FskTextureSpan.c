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

#include "FskTextureSpan.h"

#include "FskBitmap.h"
#include "FskMemory.h"
#include "FskPixelOps.h"
#include "FskPolygon.h"
#include "FskBlitDispatchDef.h"

#include <limits.h>

#define UNUSED(x)	(void)(x)

#define kMaxLogTextureSize				12						/* Maximum texture size 4096 */
#define kTexCoordBits					16	//32-1-kMaxLogTextureSize	/* 32-1-12 = 19 fractional bits */
typedef FskFixed						FixedFwdDiff;	/* Fixed, with kTexCoordBits fractional bits */
#define kLerpBits 4
typedef FskFixed FskFixedLerp;

#undef FskAssert		/* Why is this on for release builds? */
#define FskAssert(x)


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Typedefs								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



typedef enum SpanMode {
	kSpanModeNormal			= 0,
	kSpanModeTransparent	= 1,
	kSpanModeEdge0			= 2,
	kSpanModeEdge1			= 3,
	kSpanModeWrap			= 4,
	kSpanNumModes			= 5
} SpanMode;


typedef void	(*SetPixelProc)(FskSpan *span);
typedef void	(*InitRegime)(FskSpan *span);
typedef void	(*InitRegimator)(FskSpan *span);



/********************************************************************************
 *							TextureSpanData								*
 ********************************************************************************/

typedef struct TextureSpanData {			/* Data stored in FskSpan's spanData field */
	FskFillSpanProc		flatFill;			/* This method does a flat fill when needed for padding the boundaries */
	FskFillSpanProc		texFill;			/* This method does a texture fill */

	FskFixedMatrix3x2	texXform;			/* {u,v}(x,y) = g[0] * x + g[1] * y + g[2] */
	UInt32				spreadMethod;		/* kFskSpread{Transparent,Pad,Repeat,Reflect}X | kFskSpread{Transparent,Pad,Repeat,Reflect}Y */
	int					quality;

	FixedFwdDiff		ur, vr;				/* Regime u, v is global */
	FixedFwdDiff		u,  v;				/* Span u, v is local */
	FixedFwdDiff		du,  dv;			/* Span du, dv is local */
	FixedFwdDiff		round;				/* Value to round by, depends on the quality */
	FskFixedLerp		uMax, vMax;
	FskFixedLerp		uPeriod, vPeriod;
	int					nu, nv;
	SpanMode 			uSpanMode, vSpanMode;

	FskConstBitmap		tex;
	const void			*texPixels;
	UInt32				texWidth;
	UInt32				texHeight;
	SInt32				texPixBytes;
	SInt32				texRowBytes;

	InitRegime			initURegime;	/* This initializes each regime */
	InitRegime			initVRegime;
} TextureSpanData;



/********************************************************************************
 * DisposeTextureSpan					span->disposeSpanData() method
 ********************************************************************************/

static void
DisposeTextureSpanData(void *spanData)
{
	TextureSpanData	*td = (TextureSpanData*)spanData;
	FskBitmapWriteEnd((FskBitmap)td->tex);
	FskMemPtrDispose(spanData);
}


/********************************************************************************
 * InitTransparentURegime
 ********************************************************************************/

static void InitTransparentURegime(FskSpan *span) {
	TextureSpanData	*td = (TextureSpanData*)(span->spanData);
	FixedFwdDiff uEnd = td->ur + (span->dx - 1) * td->texXform.M[0][0];

	td->du = td->texXform.M[0][0];
	td->u  = td->ur;

	if (td->u < 0) {
		// TODO: use kSpanModeEdge when -one < td->u < 0 */
		td->uSpanMode = kSpanModeTransparent;
		td->nu = (uEnd < 0)        ? span->dx : (1 + (          -td->u + td->texXform.M[0][0] - 1) / td->texXform.M[0][0]);
	}
	else if (td->u > td->uMax) {
		// TODO: use kSpanModeEdge when td->uMax < td->u < td->uMax + one */
		td->uSpanMode = kSpanModeTransparent;
		td->nu = (uEnd > td->uMax) ? span->dx : (1 + (td->uMax - td->u + td->texXform.M[0][0] - 1) / td->texXform.M[0][0]);
	}
	else {
		td->uSpanMode = kSpanModeNormal;
		td->nu	= (uEnd < 0)		? ((0        - td->u) / td->texXform.M[0][0] + 1)
				: (uEnd > td->uMax) ? ((td->uMax - td->u) / td->texXform.M[0][0] + 1)
				: span->dx;
	}
	td->u += td->round;
}


/********************************************************************************
 * InitTransparentVRegime
 ********************************************************************************/

static void InitTransparentVRegime(FskSpan *span) {
	TextureSpanData	*td = (TextureSpanData*)(span->spanData);
	FixedFwdDiff vEnd = td->vr + (span->dx - 1) * td->texXform.M[0][1];

	td->dv = td->texXform.M[0][1];
	td->v  = td->vr;

	if (td->v < 0) {
		// TODO: use kSpanModeEdge when -one < td->v < 0 */
		td->vSpanMode = kSpanModeTransparent;
		td->nv = (vEnd < 0)	       ? span->dx : (          -td->v + td->texXform.M[0][1] - 1) / td->texXform.M[0][1];
	}
	else if (td->v > td->vMax) {
		// TODO: use kSpanModeEdge when td->vMax < td->v < td->vMax + one */
		td->vSpanMode = kSpanModeTransparent;
		td->nv = (vEnd > td->vMax) ? span->dx : (td->vMax - td->v + td->texXform.M[0][1] - 1) / td->texXform.M[0][1];
	}
	else {
		td->vSpanMode = kSpanModeNormal;
		td->nv	= (vEnd < 0)		? (0        - td->v) / td->texXform.M[0][1]	/* floor */
				: (vEnd > td->vMax) ? (td->vMax - td->v) / td->texXform.M[0][1]	/* floor */
				: span->dx;
	}
	td->v += td->round;
}


/********************************************************************************
 * InitPadURegime
 ********************************************************************************/

static void InitPadURegime(FskSpan *span) {
	TextureSpanData	*td = (TextureSpanData*)(span->spanData);
	FixedFwdDiff uEnd = td->ur + (span->dx - 1) * td->texXform.M[0][0];
	td->u = td->ur;

	td->uSpanMode = kSpanModeNormal;
	if (td->u < 0) {
		td->nu = (uEnd < 0)        ? span->dx : (          -td->u + td->texXform.M[0][0] - 1) / td->texXform.M[0][0];
		td->u  = 0;
		td->du = 0;
	}
	else if (td->u > td->uMax) {
		td->nu = (uEnd > td->uMax) ? span->dx : (td->uMax - td->u + td->texXform.M[0][0] - 1) / td->texXform.M[0][0];
		td->u  = td->uMax;
		td->du = 0;
	}
	else {
		td->nu	= (uEnd < 0)		? (0        - td->u) / td->texXform.M[0][0]	/* floor */
				: (uEnd > td->uMax) ? (td->uMax - td->u) / td->texXform.M[0][0]	/* floor */
				: span->dx;
		td->du = td->texXform.M[0][0];
	}
	td->u += td->round;
}


/********************************************************************************
 * InitPadVRegime
 ********************************************************************************/

static void InitPadVRegime(FskSpan *span) {
	TextureSpanData	*td = (TextureSpanData*)(span->spanData);
	FixedFwdDiff vEnd = td->vr + (span->dx - 1) * td->texXform.M[0][1];
	td->v = td->vr;

	td->vSpanMode = kSpanModeNormal;
	if (td->v < 0) {
		td->nv = (vEnd < 0)	       ? span->dx : (          -td->v + td->texXform.M[0][1] - 1) / td->texXform.M[0][1];
		td->v  = 0;
		td->dv = 0;
	}
	else if (td->v > td->vMax) {
		td->nv = (vEnd > td->vMax) ? span->dx : (td->vMax - td->v + td->texXform.M[0][1] - 1) / td->texXform.M[0][1];
		td->v  = td->vMax;
		td->dv = 0;
	}
	else {
		td->nv	= (vEnd < 0)		? (0        - td->v) / td->texXform.M[0][1]	/* floor */
				: (vEnd > td->vMax) ? (td->vMax - td->v) / td->texXform.M[0][1]	/* floor */
				: span->dx;
		td->dv = td->texXform.M[0][1];
	}
	td->v += td->round;
}


/********************************************************************************
 * InitRepeatURegime
 ********************************************************************************/

static void InitRepeatURegime(FskSpan *span) {
	TextureSpanData	*td = (TextureSpanData*)(span->spanData);
	FixedFwdDiff uEnd;

	td->u  = (td->ur + td->round) % td->uPeriod;									/* Convert from global to local coordinates ... */
	if (td->u < 0) td->u += td->uPeriod;											/* ... that is, positive local coordinates. */
	td->du = td->texXform.M[0][0];
	uEnd   = td->u + (span->dx - 1) * td->texXform.M[0][0];							/* Where we'll be at the end of the span */

	if (td->u >= td->uMax) {			/* In the wrap region */
		td->uSpanMode = td->quality ? kSpanModeWrap : kSpanModeNormal;				/* TODO: append this span to an adjacent on when kSpanModeNormal */
		if (td->texXform.M[0][0] >= 0) {											/* U increasing */
			td->nu = (uEnd < td->uPeriod)	? span->dx								/* Stays in the wrap region until the end of the span */
											: (td->uPeriod - td->u + td->texXform.M[0][0] - 1) / td->texXform.M[0][0];	/* ceiling */
		}
		else /* (td->texXform.M[0][0] < 0) */ {										/* U decreasing */
			td->nu = (uEnd >= td->uMax)		? span->dx								/* Stays in the wrap region until the end of the span */
											: ((td->uMax - td->u) / td->texXform.M[0][0] + 1);
		}
	}

	else {								/* in the fill region */
		td->uSpanMode = kSpanModeNormal;
		td->nu	= (uEnd < 0)		? ((          -td->u) / td->texXform.M[0][0] + 1)
				: (uEnd > td->uMax) ? ((td->uMax - td->u) / td->texXform.M[0][0] + 1)
				: span->dx;
		// TODO: We should include the wrap section when doing low quality
	}
}


/********************************************************************************
 * InitRepeatVRegime
 ********************************************************************************/

static void InitRepeatVRegime(FskSpan *span) {
	TextureSpanData	*td = (TextureSpanData*)(span->spanData);
	FixedFwdDiff vEnd;

	td->v = (td->vr + td->round) % td->vPeriod;										/* Convert from global to local coordinates ... */
	if (td->v < 0)	td->v += td->vPeriod;											/* ... that is, positive local coordinates. */
	td->dv = td->texXform.M[0][1];
	vEnd = td->v + (span->dx - 1) * td->texXform.M[0][1];							/* Where we'll be at the end of the span */

	if (td->v >= td->vMax) {				/* In the wrap region */
		td->vSpanMode = td->quality ? kSpanModeWrap : kSpanModeNormal;				/* TODO: append this span to an adjacent one when kSpanModeNormal */
		if (td->texXform.M[0][1] >= 0) {											/* U increasing */
			td->nv = (vEnd < td->vPeriod)	? span->dx								/* Stays in the wrap region until the end of the span */
											: (td->vPeriod - td->v + td->texXform.M[0][1] - 1) / td->texXform.M[0][1];	/* ceiling */
		}
		else /* (td->texXform.M[0][1] < 0) */ {										/* U decreasing */
			td->nv = (vEnd >= td->vMax)		? span->dx								/* Stays in the wrap region until the end of the span */
											: ((td->vMax - td->v) / td->texXform.M[0][1] + 1);
		}
	}

	else {									/* in the fill region */
		td->vSpanMode = kSpanModeNormal;
		td->nv	= (vEnd < 0)		? ((          -td->v) / td->texXform.M[0][1] + 1)
				: (vEnd > td->vMax) ? ((td->vMax - td->v) / td->texXform.M[0][1] + 1)
				: span->dx;
		// TODO: We should include the wrap section when doing low quality
	}
}


/********************************************************************************
 * InitReflectURegime
 ********************************************************************************/

static void InitReflectURegime(FskSpan *span) {
	TextureSpanData	*td = (TextureSpanData*)(span->spanData);
	FixedFwdDiff uEnd;

	td->uSpanMode = kSpanModeNormal;
	td->u = td->ur % td->uPeriod;
	if (td->u < 0)	td->u += td->uPeriod;
	if (td->u <= td->uMax) {
		td->du = td->texXform.M[0][0];
	}
	else {
		td->u = td->uPeriod - td->u;
		td->du = -td->texXform.M[0][0];
	}
	uEnd   = td->u + (span->dx - 1) * td->du;
	td->nu	= (uEnd < 0)		? ((          -td->u) / td->du + 1)
			: (uEnd > td->uMax) ? ((td->uMax - td->u) / td->du + 1)
			: span->dx;
	td->u += td->round;
}


/********************************************************************************
 * InitReflectVRegime
 ********************************************************************************/

static void InitReflectVRegime(FskSpan *span) {
	TextureSpanData	*td = (TextureSpanData*)(span->spanData);
	FixedFwdDiff vEnd;

	td->vSpanMode = kSpanModeNormal;
	td->v = td->vr % td->vPeriod;
	if (td->v < 0) 	td->v += td->vPeriod;
	if (td->v <= td->vMax) {
		td->dv = td->texXform.M[0][1];
	}
	else {
		td->v = td->vPeriod - td->v;
		td->dv = -td->texXform.M[0][1];
	}
	vEnd   = td->v + (span->dx - 1) * td->dv;
	td->nv	= (vEnd < 0)		? ((          -td->v) / td->dv + 1)
			: (vEnd > td->vMax) ? ((td->vMax - td->v) / td->dv + 1)
			: span->dx;
	td->v += td->round;
}


/********************************************************************************
 * FillTextureSpan									span->fill method
 ********************************************************************************/

static void
FillTextureSpan(FskSpan *span)
{
	TextureSpanData	*td = (TextureSpanData*)(span->spanData);
	SInt32			spanLength, regimeLength;

	td->du = td->texXform.M[0][0];
	td->dv = td->texXform.M[0][1];

	for (spanLength = span->dx; spanLength > 0;
		 spanLength -= regimeLength,
		 td->ur += td->texXform.M[0][0] * regimeLength,
		 td->vr += td->texXform.M[0][1] * regimeLength
	) {
		span->dx = spanLength;												/* Communicate the span length to the regime initers */
		td->initURegime(span);
		td->initVRegime(span);

		span->dx = regimeLength = (td->nu <= td->nv) ? td->nu : td->nv;		/* Regime length is the smallest of nu and nv */
		if (td->uSpanMode == kSpanModeNormal && td->vSpanMode == kSpanModeNormal) {
			td->texFill(span);												/* This updates span->p and zeros span->dx */
		}
		else if (td->uSpanMode == kSpanModeTransparent || td->vSpanMode == kSpanModeTransparent) {
			span->p = (char*)(span->p) + span->pixelBytes * regimeLength;	/* Update span->p */
			continue;
		}
		else {
			FixedFwdDiff u, v;
			UInt32		pixBuf[4];
			int			dx;
			const char	*savePixels		= (const char*)(td->texPixels);
			UInt32		saveWidth		= td->texWidth;
			UInt32		saveHeight		= td->texHeight;
			SInt32		saveRowBytes	= td->texRowBytes;

			td->texPixels	= pixBuf;
			td->texWidth	= 2;
			td->texHeight	= 2;
			td->texRowBytes	= 2 * td->texPixBytes;

			// @@@ Need a separate loop for low quality
			for (dx = span->dx, u = td->u, v = td->v; dx--; /*span->p = (char*)(span->p) + span->pixelBytes,*/ u += td->du, v += td->dv) {
				FixedFwdDiff u0, v0, u1, v1;
				u0 = u >> kTexCoordBits;
				v0 = v >> kTexCoordBits;
				switch (td->uSpanMode) {
					case kSpanModeNormal:	u1 = u0 + 1;	break;
					case kSpanModeWrap:		u1 = 0;			break;
					default:
					case kSpanModeEdge0:
					case kSpanModeEdge1:	u1 = u0;		break;
				}
				switch (td->vSpanMode) {
					case kSpanModeNormal:	v1 = v0 + 1;	break;
					case kSpanModeWrap:		v1 = 0;			break;
					default:
					case kSpanModeEdge0:
					case kSpanModeEdge1:	v1 = v0;		break;
				}
				FskMemCopy((char*)pixBuf + 0 * td->texPixBytes, savePixels + v0 * saveRowBytes + u0 * td->texPixBytes, td->texPixBytes);
				FskMemCopy((char*)pixBuf + 1 * td->texPixBytes, savePixels + v0 * saveRowBytes + u1 * td->texPixBytes, td->texPixBytes);
				FskMemCopy((char*)pixBuf + 2 * td->texPixBytes, savePixels + v1 * saveRowBytes + u0 * td->texPixBytes, td->texPixBytes);
				FskMemCopy((char*)pixBuf + 3 * td->texPixBytes, savePixels + v1 * saveRowBytes + u1 * td->texPixBytes, td->texPixBytes);
				if (td->uSpanMode == kSpanModeEdge0) {
					FskMemSet((char*)pixBuf + 0 * td->texPixBytes, 0, td->texPixBytes);
					FskMemSet((char*)pixBuf + 2 * td->texPixBytes, 0, td->texPixBytes);
				} else if (td->uSpanMode == kSpanModeEdge1) {
					FskMemSet((char*)pixBuf + 1 * td->texPixBytes, 0, td->texPixBytes);
					FskMemSet((char*)pixBuf + 3 * td->texPixBytes, 0, td->texPixBytes);
				}
				if (td->vSpanMode == kSpanModeEdge0) {
					FskMemSet((char*)pixBuf + 0 * td->texPixBytes, 0, td->texPixBytes);
					FskMemSet((char*)pixBuf + 1 * td->texPixBytes, 0, td->texPixBytes);
				} else if (td->vSpanMode == kSpanModeEdge1) {
					FskMemSet((char*)pixBuf + 2 * td->texPixBytes, 0, td->texPixBytes);
					FskMemSet((char*)pixBuf + 3 * td->texPixBytes, 0, td->texPixBytes);
				}
				span->dx = 1;
				td->u = u & ((1 << kTexCoordBits) - 1);
				td->v = v & ((1 << kTexCoordBits) - 1);
				td->texFill(span);
			}
			td->texPixels	= savePixels;
			td->texWidth	= saveWidth;
			td->texHeight	= saveHeight;
			td->texRowBytes	= saveRowBytes;
			span->dx = -1;
		}
	}
}


/********************************************************************************
 * SetTextureSpan:									span->set() method.
 ********************************************************************************/

static void
SetTextureSpan(const FskEdge *L, const FskEdge *R, SInt32 x, SInt32 y, FskSpan *span)
{
	TextureSpanData	*td			= (TextureSpanData*)(span->spanData);

	UNUSED(L);
	UNUSED(R);

	/* Convert from screen coordinates into the texture coordinate system */
	td->ur = td->texXform.M[2][0]+ td->texXform.M[1][0] * y + td->texXform.M[0][0] * x;
	td->vr = td->texXform.M[2][1]+ td->texXform.M[1][1] * y + td->texXform.M[0][1] * x;
	td->nu = 0;
	td->nv = 0;
}


/********************************************************************************
 * Span procs
 ********************************************************************************/

#if DST_32BGRA								/* Windows, Android */
	#if SRC_32BGRA							/* 32BGRA --> 32BGRA */
		#define SrcPixelFmt	32BGRA
		#define DstPixelFmt	32BGRA
		#include "FskTextureSpanProto.c"
	#endif /* SRC_32BGRA */
	#if SRC_32RGBA							/* 32RGBA --> 32BGRA */
		#define SrcPixelFmt	32RGBA
		#define DstPixelFmt	32BGRA
		#include "FskTextureSpanProto.c"
	#endif /* SRC_32RGBA */
	#if SRC_32ARGB							/* 32ARGB --> 32BGRA */
		#define SrcPixelFmt	32ARGB
		#define DstPixelFmt	32BGRA
		#include "FskTextureSpanProto.c"
	#endif /* SRC_32ARGB */
	#if SRC_24BGR							/* 24BGR --> 32BGRA */
		#define SrcPixelFmt	24BGR
		#define DstPixelFmt	32BGRA
		#include "FskTextureSpanProto.c"
	#endif /* SRC_24BGR */
	#if SRC_YUV420							/* YUV420 --> 32BGRA */
		#define SrcPixelFmt	YUV420
		#define DstPixelFmt	32BGRA
		#include "FskTextureSpanProto.c"
	#endif /* SRC_YUV420 */
#endif /* DST_32BGRA */

#if DST_32ARGB								/* Macintosh */
	#if SRC_32ARGB							/* 32ARGB --> 3ARGB */
		#define SrcPixelFmt	32ARGB
		#define DstPixelFmt	32ARGB
		#include "FskTextureSpanProto.c"
	#endif /* SRC_32ARGB */
	#if SRC_32RGBA							/* 32RGBA --> 32ARGB */
		#define SrcPixelFmt	32RGBA
		#define DstPixelFmt	32ARGB
		#include "FskTextureSpanProto.c"
	#endif /* SRC_32RGBA */
	#if SRC_32BGRA							/* 32BGRA --> 32ARGB */
		#define SrcPixelFmt	32BGRA
		#define DstPixelFmt	32ARGB
		#include "FskTextureSpanProto.c"
	#endif /* SRC_32BGRA */
	#if SRC_24BGR							/* 24BGR -> 32ARGB */
		#define SrcPixelFmt	24BGR
		#define DstPixelFmt	32ARGB
		#include "FskTextureSpanProto.c"
	#endif /* SRC_24BGR */
	#if SRC_YUV420							/* YUV420 --> 32ARGB */
		#define SrcPixelFmt	YUV420
		#define DstPixelFmt	32ARGB
		#include "FskTextureSpanProto.c"
	#endif /* SRC_YUV420 */
#endif /* DST_32ARGB */

#if DST_32RGBA								/* GL */
	#if SRC_32RGBA							/* 32RGBA --> 32RGBA */
		#define SrcPixelFmt	32RGBA
		#define DstPixelFmt	32RGBA
		#include "FskTextureSpanProto.c"
	#endif /* SRC_32RGBA */
	#if SRC_32BGRA							/* 32BGRA --> 32RGBA */
		#define SrcPixelFmt	32BGRA
		#define DstPixelFmt	32RGBA
		#include "FskTextureSpanProto.c"
	#endif /* SRC_32BGRA */
	#if SRC_32ARGB							/* 32ARGB --> 32BGBA */
		#define SrcPixelFmt	32ARGB
		#define DstPixelFmt	32RGBA
		#include "FskTextureSpanProto.c"
	#endif /* SRC_32ARGB */
	#if SRC_24BGR							/* 24BGR --> 32RGBA */
		#define SrcPixelFmt	24BGR
		#define DstPixelFmt	32RGBA
		#include "FskTextureSpanProto.c"
	#endif /* SRC_24BGR */
	#if SRC_YUV420							/* YUV420 --> 32RGBA */
		#define SrcPixelFmt	YUV420
		#define DstPixelFmt	32RGBA
		#include "FskTextureSpanProto.c"
	#endif /* SRC_YUV420 */
#endif /* DST_32RGBA */


/********************************************************************************
 * GetTexFillSpanProc
 ********************************************************************************/

#define FILL_SPAN_ENTRY(srcFmt, dstFmt)																		\
		{	FskName2(kFskBitmapFormat,srcFmt),																\
			FskName2(kFskBitmapFormat,dstFmt),																\
			{	FskName4(TexFillSpan,FskName3(fsk,srcFmt,FormatKind),FskName3(fsk,dstFmt,FormatKind),Q0),	\
				FskName4(TexFillSpan,FskName3(fsk,srcFmt,FormatKind),FskName3(fsk,dstFmt,FormatKind),Q1)	\
		}	}

static FskFillSpanProc GetTexFillSpanProc(UInt32 srcPixelFormat, UInt32 dstPixelFormat, int quality) {
	struct FillProcEntry {
		UInt32			srcPixelFormat;
		UInt32			dstPixelFormat;
		FskFillSpanProc	spanProc[2];
	};
	static const struct FillProcEntry fillProcTab[] = {
		/*				srcFmt	dstFmt	*/
		#if DST_32BGRA								/* Windows */
			#if SRC_32BGRA
				FILL_SPAN_ENTRY(32BGRA, 32BGRA),
			#endif /* SRC_32BGRA */
			#if SRC_32RGBA
				FILL_SPAN_ENTRY(32RGBA, 32BGRA),
			#endif /* SRC_32RGBA */
			#if SRC_32ARGB
				FILL_SPAN_ENTRY(32ARGB, 32BGRA),
			#endif /* SRC_32ARGB */
			#if SRC_24BGR
				FILL_SPAN_ENTRY(24BGR,  32BGRA),
			#endif /* SRC_24BGR */
			#if SRC_YUV420
				FILL_SPAN_ENTRY(YUV420, 32BGRA),
			#endif /* SRC_YUV420 */
		#endif /* DST_32BGRA */
		#if DST_32ARGB								/* Macintosh */
			#if SRC_32ARGB
				FILL_SPAN_ENTRY(32ARGB, 32ARGB),
			#endif /* SRC_32ARGB */
			#if SRC_32RGBA
				FILL_SPAN_ENTRY(32RGBA, 32ARGB),
			#endif /* SRC_32RGBA */
			#if SRC_32BGRA
				FILL_SPAN_ENTRY(32BGRA, 32ARGB),
			#endif /* SRC_32BGRA */
			#if SRC_24BGR
				FILL_SPAN_ENTRY(24BGR,  32ARGB),
			#endif /* SRC_24BGR */
			#if SRC_YUV420
				FILL_SPAN_ENTRY(YUV420, 32ARGB),
			#endif /* SRC_YUV420 */
		#endif /* DST_32ARGB */
		#if DST_32RGBA								/* GL */
			#if SRC_32RGBA
				FILL_SPAN_ENTRY(32RGBA, 32RGBA),
			#endif /* SRC_32RGBA */
			#if SRC_32BGRA
				FILL_SPAN_ENTRY(32BGRA, 32RGBA),
			#endif /* SRC_32BGRA */
			#if SRC_32ARGB
				FILL_SPAN_ENTRY(32ARGB, 32RGBA),
			#endif /* SRC_32ARGB */
			#if SRC_24BGR
				FILL_SPAN_ENTRY(24BGR,  32RGBA),
			#endif /* SRC_24BGR */
			#if SRC_YUV420
				FILL_SPAN_ENTRY(YUV420, 32RGBA),
			#endif /* SRC_YUV420 */
		#endif /* DST_32RGBA */
		{	0, 0, { NULL, NULL } }
	};
	const struct FillProcEntry *p;
	for (p = fillProcTab; p->spanProc[0] != NULL; ++p)
		if (srcPixelFormat == p->srcPixelFormat && dstPixelFormat == p->dstPixelFormat)
			break;
	return p->spanProc[quality];
}


/********************************************************************************
 * FskInitTextureSpan								InitSpan() method
 ********************************************************************************/

FskErr
FskInitTextureSpan(
	FskSpan					*span,
	FskBitmap				dstBM,
	const FskFixedMatrix3x2	*pathMatrix,		/* Can be NULL */
	UInt32					quality,
	const FskColorSource	*cs
)
{
	const FskColorSourceTexture	*texcs	= &((const FskColorSourceUnion*)cs)->tx;
	FskConstBitmap	tex					= texcs->texture;
	FskErr			err					= kFskErrNone;
	TextureSpanData	*td;

    FskAssert(dstBM);
    FskAssert(span);
    FskAssert(cs);
    FskAssert(quality == 0 || quality == 1);

	/* Initialize span portion */
	BAIL_IF_ERR(err = FskBitmapWriteBegin((FskBitmap)texcs->texture, NULL, NULL, NULL));
	FskInitSolidColorSpan(span, dstBM, pathMatrix, 0, cs);								/* Get the flat fill proc ... */
	span->fill				= FillTextureSpan;
	span->set				= SetTextureSpan;		/* Initialize the texture coordinate for the beginning of the span on each scanline */
	span->initEdge			= NULL;
	span->advanceEdge		= NULL;
	span->disposeSpanData	= DisposeTextureSpanData;
	span->colorIsPremul		= tex->alphaIsPremultiplied;
	span->spanData			= NULL;

	/* Allocate texture data structure and copy initialization parameters. */
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(TextureSpanData), (FskMemPtr*)(void*)(&span->spanData)));	/* allocate texture data structure */
	td					= (TextureSpanData*)span->spanData;	/* Get a local pointer to the span auxilliary data structure */
	td->flatFill		= span->fill;						/* Save the flat fill proc */
	td->spreadMethod	= texcs->spreadMethod;				/* Extract the spread method ... */
	td->tex				= tex;
	td->texPixels		= tex->bits;
	td->texPixBytes		= tex->depth >> 3;
	td->texRowBytes		= tex->rowBytes;
	td->texWidth		= tex->bounds.width;
	td->texHeight		= tex->bounds.height;
	td->texFill			= GetTexFillSpanProc(tex->pixelFormat, dstBM->pixelFormat, quality);
	td->quality			= quality;
	td->round			= quality ? (1 << (kTexCoordBits - 1 - kLerpBits)) : (1 << (kTexCoordBits - 1));
	BAIL_IF_NULL(td->texFill, err, kFskErrUnsupportedPixelType);

	/* Set texture transform */
	if (texcs->textureFrame) {
		if (pathMatrix)	{ FskFixedMultiplyMatrix3x2(texcs->textureFrame, pathMatrix, &td->texXform); FskFixedMatrixInvert3x2(&td->texXform, &td->texXform); }
		else			{ FskFixedMatrixInvert3x2(texcs->textureFrame, &td->texXform);	}
	} else {
		if (pathMatrix)	{ FskFixedMatrixInvert3x2(pathMatrix, &td->texXform);			}
		else			{ FskFixedIdentityMatrix3x2(&td->texXform);						}
	}

	/* Initialize U regimes */
	td->uMax = (tex->bounds.width - 1)  << kTexCoordBits;	/* Max interpolatable value */
	td->uPeriod = td->uMax;									/* Period for transparent and pad */
	switch ((td->spreadMethod >> kFskSpreadPosX) & kFskSpreadMask) {
		case kFskSpreadTransparentX >> kFskSpreadPosX:
			td->initURegime = InitTransparentURegime;
			break;
		case kFskSpreadPadX >> kFskSpreadPosX:
			td->initURegime = InitPadURegime;
			break;
		case kFskSpreadRepeatX >> kFskSpreadPosX:
			td->uPeriod += 1 << kTexCoordBits;				/* Also interpolate between the last and the first */
			td->initURegime = InitRepeatURegime;
			break;
		case kFskSpreadReflectX >> kFskSpreadPosX:
			td->uPeriod <<= 1;								/* Double period for reflection */
			td->initURegime = InitReflectURegime;
			break;
	}

	/* Initialize V regimes */
	td->vMax = (tex->bounds.height - 1) << kTexCoordBits;	/* Max interpolatable value */
	td->vPeriod = td->vMax;									/* Period for transparent and pad */
	switch ((td->spreadMethod >> kFskSpreadPosY) & kFskSpreadMask) {
		case kFskSpreadTransparentY >> kFskSpreadPosY:
			td->initVRegime = InitTransparentVRegime;
			break;
		case kFskSpreadPadY >> kFskSpreadPosY:
			td->initVRegime = InitPadVRegime;
			break;
		case kFskSpreadRepeatY >> kFskSpreadPosY:
			td->vPeriod += 1 << kTexCoordBits;				/* Also interpolate between the last and the first */
			td->initVRegime = InitRepeatVRegime;
			break;
		case kFskSpreadReflectY >> kFskSpreadPosY:
			td->vPeriod <<= 1;								/* Double period for reflection */
			td->initVRegime = InitReflectVRegime;
			break;
	}

bail:
	if (err != kFskErrNone) {
		if (span->spanData != NULL) {
			DisposeTextureSpanData(span->spanData);
			span->spanData = NULL;
		}
	}

	return err;
}


/********************************************************************************
 * Strategy:
 *
 * Regime processing:
 * (a) Generic heterogeneous regime determinimation calling pixel procs
 * (b) {transparentX, transparentY} regime style, calling homogeneous regime span procs
 * (c) {repeatX,      transparentY} regime style, calling homogeneous regime span procs
 * (d) {transparentX, repeatY}      regime style, calling homogeneous regime span procs
 * (e) {repeatX,      repeatY}      regime style, calling homogeneous regime span procs
 *
 * Span procs:
 * (a) Generic span proc, calling pixel procs
 * (b) Platform RGBA -> Platform RGBA span proc
 * (c) Screen format -> screen format spac proc
 * (d) Platform RGBA -> screen format span proc
 *
 * Pixel procs:
 * (1a) point sample src
 * (1b) bilinearly interpolate src
 * (2a) convert src to dst pixel format
 * (3a) write pixel
 * (3b) composite pixel
 ********************************************************************************/
