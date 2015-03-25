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
/* Only generate the procs if we're going to use them and this file is appropriate */
#if		FskName2(SRC_,FskName3(fsk,SrcPixelKind,KindFormat))						\
	&&	FskName2(DST_,FskName3(fsk,DstPixelKind,KindFormat))						\
	&&	(FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)	\
	&&	(FskName3(fsk,DstPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)	\
	&&	(FskName3(fsk,DstPixelKind,Bytes) == 1)										\


/********************************************************************************
 ********************************************************************************
 **		GetSrcAsDst() and BilerpSrcAsDst()
 ********************************************************************************
 ********************************************************************************/

#if   FskName3(fsk,SrcPixelKind,Bytes) == 1
	#define GetSrcAsDst(s, pix)		pix = *s;												FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix)
	#define BilerpSrcAsDst(s, pix)	pix = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);	FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix)
#elif FskName3(fsk,SrcPixelKind,Bytes) == 2
# if FskName3(fsk,SrcPixelKind,AlphaBits) == 8
	#define GetSrcAsDst(s, pix)		pix = *s;												FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix)
	#define BilerpSrcAsDst(s, pix)	pix = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);	FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix)
# else /* FskName3(fsk,SrcPixelKind,AlphaBits) == 1 or 0 */
	#define GetSrcAsDst(s, pix)		pix = *s;												FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix)
	#define BilerpSrcAsDst(s, pix)	pix = FskName3(FskBilerp,SrcPixelKind,DstPixelKind)(xf, yf, s, srb)
# endif /* FskName3(fsk,SrcPixelKind,AlphaBits) == 1 or 0 */
#elif FskName3(fsk,SrcPixelKind,Bytes) == 3
	#define GetSrcAsDst(s, pix)		/* */													FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, pix)
	#define BilerpSrcAsDst(s, pix)	Fsk24BitType pix24 = FskBilerp24(xf, yf, s, srb);		FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix24, pix)
#elif FskName3(fsk,SrcPixelKind,Bytes) == 4
	#define GetSrcAsDst(s, pix)		pix = *s;												FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix)
	#define BilerpSrcAsDst(s, pix)	pix = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);	FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix)
#else
	#error Unsupported pixel kind SrcPixelKind
#endif



/********************************************************************************
 ********************************************************************************
 **		Rect Procs
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * Copy
 *		This could probably benefit from a double-pixel write.
 ********************************************************************************/

static void
FskName4(Fsk,Copy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	BLIT_STARTLOOP {
		UInt32 pix;
		GetSrcAsDst(s, pix);
		*d = (UInt8)pix;
	} BLIT_ENDLOOP
}


/********************************************************************************
 * Bilinear Copy
 ********************************************************************************/

static void
FskName4(FskBilinear,Copy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	BLIT_STARTLOOP {
		UInt32 pix;
		BilerpSrcAsDst(s, pix);
		*d = (UInt8)pix;
	} BLIT_ENDLOOP
}


/********************************************************************************
 * Blend
 ********************************************************************************/

static void
FskName4(Fsk,Blend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	UInt8 alpha = params->alpha;
	BLIT_STARTLOOP {
		UInt32 pix;
		GetSrcAsDst(s, pix);
		FskName2(FskBlend,DstPixelKind)(d, (UInt8)pix, (UInt8)alpha);
	} BLIT_ENDLOOP
}


/********************************************************************************
 * Bilinear Blend
 ********************************************************************************/

static void
FskName4(FskBilinear,Blend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	UInt8 alpha = params->alpha;
	BLIT_STARTLOOP {
		UInt32 pix;
		BilerpSrcAsDst(s, pix);
		FskName2(FskBlend,DstPixelKind)(d, (UInt8)pix, (UInt8)alpha);
	} BLIT_ENDLOOP
}


#if FskName3(fsk,SrcPixelKind,AlphaBits) > 1


/********************************************************************************
 * Alpha
 ********************************************************************************/

static void
FskName4(Fsk,Alpha,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	BLIT_STARTLOOP {
		UInt32	pix		= *s;
		UInt8	alpha	= (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition));
		#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
			alpha <<= (8 - FskName3(fsk,SrcPixelKind,AlphaBits));						/* Convert to ... */
			alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);						/* ... 8 bits */
		#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
		if (alpha != 0) {
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);
			if (alpha == ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1)) {
				*d = (UInt8)pix;
			}
			else {
				if (!params->isPremul) {
					FskName2(FskBlend,DstPixelKind)(d, (UInt8)pix, alpha);
				} else {
					UInt32 t;
					#if FskName3(fsk,SrcPixelKind,GreenBits) == 8
						alpha = 255 - alpha;
						t = alpha * *d; t += (1 << (8 - 1)); t += t >> 8; t >>= 8; t += pix;
						#ifndef NO_CHECKS
							if (t > 255) t = 255;
						#endif /* !NO_CHECKS */
						*d = (UInt8)t;
					#else /* FskName3(fsk,SrcPixelKind,GreenBits) < 8 */
						alpha = 63 - (alpha >> 2);
						t = alpha * *d; t += (1 << (6 - 1)); t += t >> 6; t >>= 6; t += pix;
						#ifndef NO_CHECKS
							if (t > 255) t = 255;
						#endif /* !NO_CHECKS */
						*d = (UInt8)t;
					#endif /* FskName3(fsk,SrcPixelKind,GreenBits) < 8 */
				}
			}
		}
	} BLIT_ENDLOOP
}


/********************************************************************************
 * Bilinear Alpha
 ********************************************************************************/

static void
FskName4(FskBilinear,Alpha,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	BLIT_STARTLOOP {
		UInt32	pix		= FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);
		UInt8	alpha	= (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition));
		#if FskName3(fsk,SrcPixelKind,AlphaBits) != 8
			alpha &= ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);						/* Remove non-alpha bits */
		#endif
		if (alpha != 0) {
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);
			if (alpha == ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1)) {
				*d = (UInt8)pix;
			}
			else {
				#if FskName3(fsk,SrcPixelKind,AlphaBits) == 4								/* If alpha was 4 bits ... */
					alpha |= alpha << 4;													/* ... convert to 8 bits */
				#elif FskName3(fsk,SrcPixelKind,AlphaBits) < 8								/* If alpha is not 4 or 8 bits ... */
					alpha = alpha * 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);/* ... convert to 8 bits */
				#endif
				if (!params->isPremul) {
					FskName2(FskBlend,DstPixelKind)(d, (UInt8)pix, alpha);
				} else {
					UInt32 t = (255 - alpha) * *d; t += (1 << (8 - 1)); t += t >> 8; t >>= 8; t += pix;
					#ifndef NO_CHECKS
						if (t > 255) t = 255;
					#endif /* !NO_CHECKS */
					*d = (UInt8)t;
				}
			}
		}
	} BLIT_ENDLOOP
}


/********************************************************************************
 * AlphaBlend
 ********************************************************************************/

static void
FskName4(Fsk,AlphaBlend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	UInt16 scale = params->alpha;
	if (!params->isPremul) {
		BLIT_STARTLOOP {
			UInt32	pix		= *s;
			UInt32	alpha	= (pix >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);
			if (alpha != 0) {
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);									/* Convert to destination pixel format */
				if (alpha == ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1)) {						/* If alpha is max ... */
					FskName2(FskBlend,DstPixelKind)(d, (UInt8)pix, (UInt8)scale);						/* Blend using the scale */
				}
				else {
					#if FskName3(fsk,SrcPixelKind,AlphaBits) == 4										/* If alpha was 4 bits ... */
						alpha |= alpha << 4;															/* ... convert to 8 bits */
					#elif FskName3(fsk,SrcPixelKind,AlphaBits) < 8										/* If alpha is not 4 or 8 bits ... */
						alpha = alpha * 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);		/* ... convert to 8 bits */
					#endif
					alpha *= scale; alpha += alpha >> 8; alpha += 0x80; alpha >>= 8;					/* Multiply alpha and scale */
					if (alpha != 0)	FskName2(FskBlend,DstPixelKind)(d, (UInt8)pix, (UInt8)alpha);		/* Blend */
				}
			}
		} BLIT_ENDLOOP
	}
	else {
		BLIT_STARTLOOP {
			*d = 0xA5;	/* TODO: UNIMPLEMENTED, please implement. */
		} BLIT_ENDLOOP
	}
}


/********************************************************************************
 * Bilinear AlphaBlend
 ********************************************************************************/

static void
FskName4(FskBilinear,AlphaBlend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	UInt16 scale = params->alpha;
	if (!params->isPremul) {
		BLIT_STARTLOOP {
			UInt32	pix		= FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);
			UInt32	alpha	= (pix >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);
			if (alpha != 0) {
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);									/* Convert to destination pixel format */
				if (alpha == ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1)) {						/* If alpha is max ... */
					FskName2(FskBlend,DstPixelKind)(d, (UInt8)pix, (UInt8)scale);						/* Blend using the scale */
				}
				else {
					#if FskName3(fsk,SrcPixelKind,AlphaBits) == 4										/* If alpha was 4 bits ... */
						alpha |= alpha << 4;															/* ... convert to 8 bits */
					#elif FskName3(fsk,SrcPixelKind,AlphaBits) < 8										/* If alpha is not 4 or 8 bits ... */
						alpha = alpha * 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);		/* ... convert to 8 bits */
					#endif
					alpha *= scale; alpha += alpha >> 8; alpha += 0x80; alpha >>= 8;					/* Multiply alpha and scale */
					if (alpha != 0)	FskName2(FskBlend,DstPixelKind)(d, (UInt8)pix, (UInt8)alpha);		/* Blend */
				}
			}
		} BLIT_ENDLOOP
	}
	else {
		BLIT_STARTLOOP {
			*d = 0xA5;	/* TODO: UNIMPLEMENTED, please implement. */
		} BLIT_ENDLOOP
	}
}


#else /* FskName3(fsk,SrcPixelKind,AlphaBits) <= 1 */


/* This takes up a few more bytes than #defines, but is easier to write code for */
static void FskName4(Fsk,Alpha,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)				{ FskName4(Fsk,Copy,SrcPixelKind,DstPixelKind)(params);				}
static void FskName4(FskBilinear,Alpha,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)		{ FskName4(FskBilinear,Copy,SrcPixelKind,DstPixelKind)(params);		}
static void FskName4(Fsk,AlphaBlend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)			{ FskName4(Fsk,Blend,SrcPixelKind,DstPixelKind)(params);			}
static void FskName4(FskBilinear,AlphaBlend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)	{ FskName4(FskBilinear,Blend,SrcPixelKind,DstPixelKind)(params);	}


#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) <= 1 */



/********************************************************************************
 * TintCopy
 ********************************************************************************/

static void
FskName4(Fsk,TintCopy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	UInt32 tint, pix;
	FskName2(fskConvert24RGB,DstPixelKind)(*((Fsk24BitType*)(&(params->red))), tint);

	#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
		BLIT_STARTLOOP {
			GetSrcAsDst(s, pix);
			pix = FskName2(FskPixelMul,DstPixelKind)((UInt8)pix, (UInt8)tint);
			FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, params->alpha);
		} BLIT_ENDLOOP
	#else /* FskName3(fsk,SrcPixelKind,AlphaBits) == 8 */
		BLIT_STARTLOOP {
			UInt32 alpha;
			pix = *s;
			alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract alpha from the source */
			alpha *= params->alpha; alpha += 0x80; alpha += alpha >> 8; alpha >>= 8;					/* Multiply alpha and scale */
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);
			pix = FskName2(FskPixelMul,DstPixelKind)((UInt8)pix, (UInt8)tint);
			FskName2(FskBlend,DstPixelKind)(d, (UInt8)pix, (UInt8)alpha);
		} BLIT_ENDLOOP
	#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) == 8 */
}


/********************************************************************************
 * Bilinear TintCopy
 ********************************************************************************/

static void
FskName4(FskBilinear,TintCopy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	UInt32 tint,pix;
	FskName2(fskConvert24RGB,DstPixelKind)(*((Fsk24BitType*)(&(params->red))), tint);

	#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
		BLIT_STARTLOOP {
			BilerpSrcAsDst(s, pix);
			pix = FskName2(FskPixelMul,DstPixelKind)((UInt8)pix, (UInt8)tint);
			FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, params->alpha);
		} BLIT_ENDLOOP
	#else /* FskName3(fsk,SrcPixelKind,AlphaBits) == 8 */
		BLIT_STARTLOOP {
			UInt32 alpha;
			pix = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);
			alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract alpha from the source */
			alpha *= params->alpha; alpha += 0x80; alpha += alpha >> 8; alpha >>= 8;					/* Multiply alpha and scale */
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);
			pix = FskName2(FskPixelMul,DstPixelKind)((UInt8)pix, (UInt8)tint);
			FskName2(FskBlend,DstPixelKind)(d, (UInt8)pix, (UInt8)alpha);
		} BLIT_ENDLOOP
	#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) == 8 */
}


/********************************************************************************/


#undef GetSrcAsDst
#undef BilerpSrcAsDst


#endif /* FskName2(SRC_,FskName3(fsk,SrcPixelKind,KindFormat)) && FskName2(DST_,FskName3(fsk,DstPixelKind,KindFormat)) */


#undef SrcPixelKind
#undef DstPixelKind

