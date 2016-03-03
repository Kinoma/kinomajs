/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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

#include "Fsk.h"
#include "FskBitmap.h"
#include "FskCompositeRect.h"
#include "FskErrors.h"
#include "FskFixedMath.h"
#include "FskPixelOps.h"


static UInt8
PixelNormalComposite(UInt8 d, UInt8 s)
{
	return s;
}


typedef UInt8 (*BlendOp)(UInt8 d, UInt8 s);
typedef void (*CompOp)(UInt32 *d, UInt32 s);


/* This table needs to be synchronized with the composition modes defined in the header file. */
static const BlendOp blendProcTable[kFskNumAdobeCompositeModes] = {
	PixelNormalComposite,
	FskPixelMul8,			/* These are defined in FskPixelOps.h */
	FskPixelScreen8,
	FskPixelOverlay8,
	FskPixelDarken8,
	FskPixelLighten8,
	FskPixelColorDodge8,
	FskPixelColorBurn8,
	FskPixelHardLight8,
	FskPixelSoftLight8,
	FskPixelDifference8,
	FskPixelExclusion8
};
static const CompOp compProcTable[kFskNumPorterDuffCompositeModes][2] = {
	{	FskAlphaBlackSourceOverA32,			FskAlphaBlackSourceOver32A		},
	{	FskAlphaBlackDestinationOverA32,	FskAlphaBlackDestinationOver32A	},
	{	FskAlphaBlackSourceInA32,			FskAlphaBlackSourceIn32A		},
	{	FskAlphaBlackDestinationInA32,		FskAlphaBlackDestinationIn32A	},
	{	FskAlphaBlackSourceOutA32,			FskAlphaBlackSourceOut32A		},
	{	FskAlphaBlackDestinationOutA32,		FskAlphaBlackDestinationOut32A	},
	{	FskAlphaBlackSourceAtopA32,			FskAlphaBlackSourceAtop32A		},
	{	FskAlphaBlackDestinationAtopA32,	FskAlphaBlackDestinationAtop32A	},
	{	FskAlphaBlackLighterA32,			FskAlphaBlackLighter32A			},
	{	FskAlphaBlackXorA32,				FskAlphaBlackXor32A				},
	{	FskAlphaBlackSubtractA32,			FskAlphaBlackSubtract32A		}
};


/********************************************************************************
 * FskCompositeRect
 ********************************************************************************/

FskErr
FskCompositeRect(
	int					compositionMode,
	FskConstBitmap		srcBM,
	FskConstRectangle	srcRect,
	FskBitmap			dstBM,
	FskConstPoint		dstLoc
) {
	FskErr	err = kFskErrNone;
	FskRectangleRecord srcR, dstR;
	int w, h, sBump, dBump, aOffset, pixelBytes;

	FskBitmapReadBegin((FskBitmap)srcBM, NULL, NULL, NULL);
	FskBitmapWriteBegin(          dstBM, NULL, NULL, NULL);

	BAIL_IF_FALSE(srcBM->pixelFormat == dstBM->pixelFormat, err, kFskErrUnimplemented);		/* src and dst must have the same pixel format */
	BAIL_IF_NEGATIVE(compositionMode, err, kFskErrInvalidParameter);

	/* Compute dst rect as the intersection of the source and destination. */
	srcR = (srcRect != NULL) ? *srcRect : srcBM->bounds;									/* If a srcRect as given, use it, otherwise use the src bounds as the srcRect. */
	if (dstLoc != NULL) {																	/* If a destination location was given, ... */
		dstR.x = dstLoc->x;																	/* ... use it. */
		dstR.y = dstLoc->y;
	}
	else {																					/* Otherwise, ... */
		dstR.x = srcR.x;																	/* .. use the srcRect's location. */
		dstR.y = srcR.y;
	}

	if ((dstR.width  = dstBM->bounds.x + dstBM->bounds.width  - dstR.x) > srcR.width)		/* Composition width  is the minimum of the remainder of the dst ... */
		dstR.width  = srcR.width;															/* ... and the src rect width. */
	if ((dstR.height = dstBM->bounds.y + dstBM->bounds.height - dstR.y) > srcR.height)		/* Composition height is the minimum of the remainder of the dst ... */
		dstR.height = srcR.height;															/* ... and the src rect height. */

	switch (dstBM->pixelFormat) {
		case kFskBitmapFormat32ARGB:
		case kFskBitmapFormat32ABGR:
			aOffset = 0;																	/* Alpha is the first byte */
			break;
		case kFskBitmapFormat32BGRA:
		case kFskBitmapFormat32RGBA:
			aOffset = 3;																	/* Alpha is the last byte */
			break;
		default:
			return kFskErrUnsupportedPixelType;												/* Only 32-bit pixels supported at this time */
	}

	pixelBytes = dstBM->depth >> 3;
	sBump = srcBM->rowBytes - dstR.width * pixelBytes;
	dBump = dstBM->rowBytes - dstR.width * pixelBytes;

	if (compositionMode < kFskNumAdobeCompositeModes) {										/* Adobe composition operators */
		BlendOp blendop = blendProcTable[compositionMode];
		const UInt8 *s = (const UInt8*)(srcBM->bits) + srcR.y * srcBM->rowBytes + srcR.x * pixelBytes;	/* Advance pixel pointer to the srcRect */
		UInt8 *d =             (UInt8*)(dstBM->bits) + dstR.y * dstBM->rowBytes + dstR.x * pixelBytes;	/* Advance pixel pointer to the dstRect */
		if (aOffset == 0) {
			++s;																			/* Advance pointers to the first color component. */
			++d;
			aOffset = -1;																	/* Update the offset of alpha relative to the first color component. */
		}

		for (h = dstR.height; h--; s += sBump, d += dBump) {
			for (w = dstR.width; w--; ++s, ++d) {											/* Advance alpha here, color below */
				int cDst, cSrc, cBln, cTmp;													/* Coefficients */
				UInt8 ar = FskPixelScreen8(d[aOffset], s[aOffset]);							/* Result alpha */
				cTmp = (ar != 0) ? ((UInt32)(s[aOffset]) * 256 + (ar >> 1)) / ar : 0;		/* as / ar, scaled by 256 */
				cDst = 256 - cTmp;															/* 1 - as / ar, scaled by 256 */
				cSrc = ((255 - d[aOffset]) * cTmp + (255 >> 1)) / 255;						/* (as / ar) * (1 - ad), scaled by 256 */
				cBln = (cTmp * d[aOffset] + (255 >> 1)) / 255;								/* (as / ar) * ad, scaled by 256 */
				d[aOffset] = ar;															/* Store the result alpha */
				for (cTmp = 3; cTmp--; ++d, ++s)											/* Here we advance the color */
					*d = (cDst * *d + cSrc * *s + cBln * (*blendop)(*d, *s) + (1 << (8 - 1))) >> 8;
			}
		}
	}
	else if (compositionMode < (kFskNumAdobeCompositeModes + kFskNumPorterDuffCompositeModes)) {	/* Porter-Duff Composition Operators */
		CompOp compOp;
		const UInt32 *s;
		UInt32 *d;
		#if TARGET_RT_BIG_ENDIAN
			w = aOffset ? 1 : 0;
		#else /* TARGET_RT_LITTLE_ENDIAN */
			w = aOffset ? 0 : 1;
		#endif /* TARGET_RT_LITTLE_ENDIAN */
		compOp = compProcTable[compositionMode - kFskNumAdobeCompositeModes][w];
		s = (const UInt32*)((const char*)(srcBM->bits) + srcR.y * srcBM->rowBytes + srcR.x * pixelBytes);	/* Advance s pixel pointe rto the src rect */
		d =       (UInt32*)((      char*)(dstBM->bits) + dstR.y * dstBM->rowBytes + dstR.x * pixelBytes);	/* Advance d pixel pointe rto the dst rect */
		for (h = dstR.height; h--; s = (const UInt32*)((const char*)s + sBump), d = (UInt32*)((char*)d + dBump))
			for (w = dstR.width; w--; ++s, ++d)
				(*compOp)(d, *s);
	}
	else {
		err = kFskErrInvalidParameter;
	}

bail:
	FskBitmapWriteEnd(          dstBM);
	FskBitmapReadEnd((FskBitmap)srcBM);
	return err;
}


/********************************************************************************
 * FskMatteCompositeRect
 ********************************************************************************/

FskErr
FskMatteCompositeRect(
	int					compositionMode,
	UInt8				globalAlpha,
	FskConstBitmap		matteBM,
	FskConstBitmap		srcBM,
	FskConstRectangle	srcRect,
	FskBitmap			dstBM,
	FskConstPoint		dstLoc
) {
	FskErr				err			= kFskErrNone;
	const UInt32		*s			= NULL;
	UInt32				*d			= NULL;
	const UInt8			*m			= NULL;
	FskRectangleRecord	srcR, dstR;
	SInt32				w, h, sBump, dBump, mBump;
	CompOp				compOp;

	BAIL_IF_ERR(err = FskBitmapReadBegin((FskBitmap)srcBM,   (const void**)&s, &sBump, NULL));
	BAIL_IF_ERR(err = FskBitmapWriteBegin(dstBM,   (void**)&d, &dBump, NULL));

	if (matteBM) {
		FskBitmapReadBegin((FskBitmap)matteBM, (const void**)&m, &mBump, NULL);
		BAIL_IF_FALSE(matteBM->bounds.width == srcBM->bounds.width && matteBM->bounds.height == srcBM->bounds.height, err, kFskErrMismatch);
	}

	BAIL_IF_FALSE(srcBM->pixelFormat == dstBM->pixelFormat, err, kFskErrUnimplemented);
	BAIL_IF_FALSE(kFskCompositePreSourceOver <= compositionMode && compositionMode <= kFskCompositePreSubtract, err, kFskErrInvalidParameter);

	/* Compute dst rect as the intersection of the source and destination. */
	srcR = (srcRect != NULL) ? *srcRect : srcBM->bounds;									/* If a srcRect as given, use it, otherwise use the src bounds as the srcRect. */
	if (dstLoc != NULL) {																	/* If a destination location was given, ... */
		dstR.x = dstLoc->x;																	/* ... use it. */
		dstR.y = dstLoc->y;
	}
	else {																					/* Otherwise, ... */
		dstR.x = srcR.x;																	/* .. use the srcRect's location. */
		dstR.y = srcR.y;
	}

	if ((dstR.width  = dstBM->bounds.x + dstBM->bounds.width  - dstR.x) > srcR.width)		/* Composition width  is the minimum of the remainder of the dst ... */
		dstR.width  = srcR.width;															/* ... and the src rect width. */
	if ((dstR.height = dstBM->bounds.y + dstBM->bounds.height - dstR.y) > srcR.height)		/* Composition height is the minimum of the remainder of the dst ... */
		dstR.height = srcR.height;															/* ... and the src rect height. */

	switch (dstBM->pixelFormat) {
		#if TARGET_RT_BIG_ENDIAN
			case kFskBitmapFormat32ARGB: case kFskBitmapFormat32ABGR:
		#else /* TARGET_RT_LITTLE_ENDIAN */
			case kFskBitmapFormat32BGRA: case kFskBitmapFormat32RGBA:
		#endif /* TARGET_RT_LITTLE_ENDIAN */
				compOp = compProcTable[compositionMode - kFskCompositePreSourceOver][0];	/* Use A32 ops */
				break;
		#if TARGET_RT_BIG_ENDIAN
			case kFskBitmapFormat32BGRA: case kFskBitmapFormat32RGBA:
		#else /* TARGET_RT_LITTLE_ENDIAN */
			case kFskBitmapFormat32ARGB: case kFskBitmapFormat32ABGR:
		#endif /* TARGET_RT_LITTLE_ENDIAN */
				compOp = compProcTable[compositionMode - kFskCompositePreSourceOver][1];	/* Use 32A ops */
				break;
		default:
			return kFskErrUnsupportedPixelType;												/* Only 32-bit pixels supported at this time */
	}

	s = (const UInt32*)((const char*)s + srcR.y * sBump + srcR.x * sizeof(*s));				/* Advance to the srcRect location */
	d =       (UInt32*)((      char*)d + dstR.y * dBump + dstR.x * sizeof(*d));				/* Advance to the dstRect location */
	sBump -= dstR.width * sizeof(*s);
	dBump -= dstR.width * sizeof(*d);
	if (matteBM) {
		m = (const UInt8*)((const char*)m + srcR.y * mBump + srcR.x * sizeof(*m));			/* Advance to the srcRect location */
		mBump -= dstR.width * sizeof(*m);
		for (h = dstR.height; h--;	m += mBump,
									s = (const UInt32*)((const char*)s + sBump),
									d = (UInt32*)((char*)d + dBump)
		) for (w = dstR.width; w--; ++m, ++s, ++d) {
			(*compOp)(d, FskAlphaScale32(FskAlphaMul(globalAlpha, *m), *s));
		}
	}
	else {
		for (h = dstR.height; h--;	s = (const UInt32*)((const char*)s + sBump),
									d = (UInt32*)((char*)d + dBump)
		) for (w = dstR.width; w--; ++s, ++d) {
			(*compOp)(d, FskAlphaScale32(globalAlpha, *s));
		}
	}


bail:
	if (m)	FskBitmapReadEnd((FskBitmap)matteBM);
	if (d)	FskBitmapWriteEnd(dstBM);
	if (s)	FskBitmapReadEnd((FskBitmap)srcBM);
	return err;
}




#ifdef NECESSARY_FOR_OTHER_THAN_32_BIT_PIXELS
static void
Composite(UInt8 *ad, UInt8 *cd, UInt8 as, const UInt8 *cs, BlendOp blendop)
{
	UInt8 ar;
	int kSrc, kDst, kBln, kTmp;		/* Coefficients for the source, destination, blend, and temp */
	ar = FskPixelScreen8(*ad, as);	/* Result alpha */
	kTmp = (ar != 0) ? (as * 256 + (ar >> 1)) / ar : 0;	/* as / ar, scaled by 256 */
	kDst = 256 - kTmp;									/* 1 - as / ar, scaled by 256 */
	kSrc = ((255 - *ad) * kTmp + (255 >> 1)) / 255;		/* (as / ar) * (1 - ad), scaled by 256 */
	kBln = (kTmp * *ad + (255 >> 1)) / 255;				/* (as / ar) * ad, scaled by 256 */
	*ad = ar;											/* Store the result alpha */
	for (kTmp = 3; kTmp--; ++cd, ++cs)
		*cd = (kDst * *cd + kSrc * *cs + kBln * (*blendop)(*cd, *cs) + (1 << (8 - 1))) >> 8;
}


void FskMultiply32AXXX(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+0, (UInt8*)(d)+1, ((UInt8*)(&p))[0], ((UInt8*)(&p))+1, FskPixelMul8); }
void FskMultiply32XXXA(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+3, (UInt8*)(d)+0, ((UInt8*)(&p))[3], ((UInt8*)(&p))+0, FskPixelMul8); }

void FskScreen32AXXX(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+0, (UInt8*)(d)+1, ((UInt8*)(&p))[0], ((UInt8*)(&p))+1, FskPixelScreen8); }
void FskScreen32XXXA(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+3, (UInt8*)(d)+0, ((UInt8*)(&p))[3], ((UInt8*)(&p))+0, FskPixelScreen8); }

void FskOverlay32AXXX(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+0, (UInt8*)(d)+1, ((UInt8*)(&p))[0], ((UInt8*)(&p))+1, FskPixelOverlay8); }
void FskOverlay32XXXA(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+3, (UInt8*)(d)+0, ((UInt8*)(&p))[3], ((UInt8*)(&p))+0, FskPixelOverlay8); }

void FskDarken32AXXX(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+0, (UInt8*)(d)+1, ((UInt8*)(&p))[0], ((UInt8*)(&p))+1, FskPixelDarken8); }
void FskDarken32XXXA(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+3, (UInt8*)(d)+0, ((UInt8*)(&p))[3], ((UInt8*)(&p))+0, FskPixelDarken8); }

void FskLighten32AXXX(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+0, (UInt8*)(d)+1, ((UInt8*)(&p))[0], ((UInt8*)(&p))+1, FskPixelLighten8); }
void FskLighten32XXXA(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+3, (UInt8*)(d)+0, ((UInt8*)(&p))[3], ((UInt8*)(&p))+0, FskPixelLighten8); }

void FskColorDodge32AXXX(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+0, (UInt8*)(d)+1, ((UInt8*)(&p))[0], ((UInt8*)(&p))+1, FskPixelColorDodge8); }
void FskColorDodge32XXXA(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+3, (UInt8*)(d)+0, ((UInt8*)(&p))[3], ((UInt8*)(&p))+0, FskPixelColorDodge8); }

void FskColorBurn32AXXX(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+0, (UInt8*)(d)+1, ((UInt8*)(&p))[0], ((UInt8*)(&p))+1, FskPixelColorBurn8); }
void FskColorBurn32XXXA(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+3, (UInt8*)(d)+0, ((UInt8*)(&p))[3], ((UInt8*)(&p))+0, FskPixelColorBurn8); }

void FskHardLight32AXXX(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+0, (UInt8*)(d)+1, ((UInt8*)(&p))[0], ((UInt8*)(&p))+1, FskPixelHardLight8); }
void FskHardLight32XXXA(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+3, (UInt8*)(d)+0, ((UInt8*)(&p))[3], ((UInt8*)(&p))+0, FskPixelHardLight8); }

void FskSoftLight32AXXX(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+0, (UInt8*)(d)+1, ((UInt8*)(&p))[0], ((UInt8*)(&p))+1, FskPixelSoftLight8); }
void FskSoftLight32XXXA(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+3, (UInt8*)(d)+0, ((UInt8*)(&p))[3], ((UInt8*)(&p))+0, FskPixelSoftLight8); }

void FskDifference32AXXX(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+0, (UInt8*)(d)+1, ((UInt8*)(&p))[0], ((UInt8*)(&p))+1, FskPixelDifference8); }
void FskDifference32XXXA(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+3, (UInt8*)(d)+0, ((UInt8*)(&p))[3], ((UInt8*)(&p))+0, FskPixelDifference8); }

void FskExclusion32AXXX(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+0, (UInt8*)(d)+1, ((UInt8*)(&p))[0], ((UInt8*)(&p))+1, FskPixelExclusion8); }
void FskExclusion32XXXA(UInt32 *d, UInt32 p) { Composite((UInt8*)(d)+3, (UInt8*)(d)+0, ((UInt8*)(&p))[3], ((UInt8*)(&p))+0, FskPixelExclusion8); }
#endif /* NECESSARY_FOR_OTHER_THAN_32_BIT_PIXELS */

