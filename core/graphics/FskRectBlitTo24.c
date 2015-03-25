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
	&&	(FskName3(fsk,DstPixelKind,Bytes) == 3)										\


/********************************************************************************
 ********************************************************************************
 **		GetSrcAsDst() and BilerpSrcAsDst()
 ********************************************************************************
 ********************************************************************************/

#if   FskName3(fsk,SrcPixelKind,Bytes) == 1
	#define GetSrcAsDst(s, dst)			UInt32 src = *s;																FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, dst)
	#define BilerpSrcAsDst(s, dst)		UInt32 src = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, dst)
#elif FskName3(fsk,SrcPixelKind,Bytes) == 2
# if FskName3(fsk,SrcPixelKind,AlphaBits) > 1
	#define GetSrcAsDst(s, dst)			UInt32 src = *s;																FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, dst)
	#define BilerpSrcAsDst(s, dst)		UInt32 src = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, dst)
# else /* FskName3(fsk,SrcPixelKind,AlphaBits) == 1 or 0 */
	#define GetSrcAsDst(s, dst)			UInt32 src = *s;																FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, dst)
	#define BilerpSrcAsDst(s, dst)		FskName3(FskBilerp,SrcPixelKind,DstPixelKind)(xf, yf, s, srb, &(dst))			/* */
# endif /* FskName3(fsk,SrcPixelKind,AlphaBits) == 1 or 0 */
#elif FskName3(fsk,SrcPixelKind,Bytes) == 3
	#define GetSrcAsDst(s, dst)		/* */																				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, dst)
	#if FskName3(fsk,SrcPixelKind,RedPosition) == FskName3(fsk,DstPixelKind,RedPosition)
		#define BilerpSrcAsDst(s, dst)	dst = FskBilerp24(xf, yf, s, srb)												/* */
	#else /* FskName3(fsk,SrcPixelKind,RedPosition) != FskName3(fsk,DstPixelKind,RedPosition) */
		#define BilerpSrcAsDst(s, dst)	Fsk24BitType src = FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix24, pix);	FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, dst)
	#endif /* FskName3(fsk,SrcPixelKind,RedPosition) != FskName3(fsk,DstPixelKind,RedPosition) */
#elif FskName3(fsk,SrcPixelKind,Bytes) == 4
	#define GetSrcAsDst(s, dst)			UInt32 src = *s;																FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, dst)
	#define BilerpSrcAsDst(s, dst)		UInt32 src = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, dst)
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
 ********************************************************************************/

static void
FskName4(Fsk,Copy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	BLIT_STARTLOOP {
		GetSrcAsDst(s, *d);
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
		BilerpSrcAsDst(s, *d);
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
		Fsk24BitType pix;
		GetSrcAsDst(s, pix);
		FskBlend24(d, pix, alpha);
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
		Fsk24BitType pix;
		BilerpSrcAsDst(s, pix);
		FskBlend24(d, pix, alpha);
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
		UInt32	src		= *s;
		UInt8	alpha	= (UInt8)(src >> FskName3(fsk,SrcPixelKind,AlphaPosition));	/* Extract alpha from source */;
		#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
			alpha <<= 8 - FskName3(fsk,SrcPixelKind,AlphaBits);						/* Left justify alpha*/
			alpha |=     alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);				/* Double the number of bits in alpha (good enough for >= 4 bits) */
		#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
		#if FskName3(fsk,SrcPixelKind,AlphaBits) < 4
			alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits) * 2;				/* Quadruple the number of bits in alpha (good enough for >= 2 bits) */
		#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 4 */

		if (alpha != 0) {															/* If the source is not transparent... */
			Fsk24BitType s24;
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, s24);				/* Convert the src to the dst format */
			if      (alpha == 255)		*d = s24;									/* If the source is opaque, write it directly */
			else if (!params->isPremul) FskBlend24(d, s24, alpha);					/* If    straight   alpha,  blend it in */
			else						*d = FskAXPY24(~alpha, *d, s24);			/* If premultiplied alpha,  add it to the scaled destination */
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
		UInt32	src		= FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);
		UInt8	alpha	= (UInt8)(src >> FskName3(fsk,SrcPixelKind,AlphaPosition));	/* Extract alpha from source */;
		#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
			alpha <<= 8 - FskName3(fsk,SrcPixelKind,AlphaBits);						/* Left justify alpha*/
			alpha |=     alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);				/* Double the number of bits in alpha (good enough for >= 4 bits) */
		#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
		#if FskName3(fsk,SrcPixelKind,AlphaBits) < 4
			alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits) * 2;				/* Quadruple the number of bits in alpha (good enough for >= 2 bits) */
		#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 4 */

		if (alpha != 0) {															/* If the source is not transparent... */
			Fsk24BitType s24;
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, s24);				/* Convert the src to the dst format */
			if      (alpha == 255)		*d = s24;									/* If the source is opaque, write it directly */
			else if (!params->isPremul) FskBlend24(d, s24, alpha);					/* If    straight   alpha,  blend it in */
			else						*d = FskAXPY24(~alpha, *d, s24);			/* If premultiplied alpha,  add it to the scaled destination */
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
	UInt8	scale		= params->alpha;

	BLIT_STARTLOOP {
		UInt32	src		= *s;
		UInt8	alpha	= (UInt8)(src >> FskName3(fsk,SrcPixelKind,AlphaPosition));	/* Extract alpha from source */;
		#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
			alpha <<= 8 - FskName3(fsk,SrcPixelKind,AlphaBits);						/* Left justify alpha*/
			alpha |=     alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);				/* Double the number of bits in alpha (good enough for >= 4 bits) */
		#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
		#if FskName3(fsk,SrcPixelKind,AlphaBits) < 4
			alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits) * 2;				/* Quadruple the number of bits in alpha (good enough for >= 2 bits) */
		#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 4 */

		if	((alpha != 0) && ((alpha = (alpha == 255) ? scale : FskAlphaMul(alpha, scale)) != 0)) {	/* If the source is not transparent... */
			Fsk24BitType s24;
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, s24);								/* Convert the src to the dst format */
			if      (alpha == 255)		*d = s24;													/* If the source is opaque, write it directly */
			else if (!params->isPremul) FskBlend24(d, s24, alpha);									/* If    straight   alpha,  blend it in */
			else						*d = FskAXPY24(~alpha, *d, FskAlphaScale24(scale, s24));	/* If premultiplied alpha,  scale and add it to the scaled destination */
		}
	} BLIT_ENDLOOP
}


/********************************************************************************
 * Bilinear AlphaBlend
 ********************************************************************************/

static void
FskName4(FskBilinear,AlphaBlend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	UInt8	scale		= params->alpha;
	BLIT_STARTLOOP {
		UInt32	src		= FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);
		UInt8	alpha	= (UInt8)(src >> FskName3(fsk,SrcPixelKind,AlphaPosition));	/* Extract alpha from source */;
		#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
			alpha <<= 8 - FskName3(fsk,SrcPixelKind,AlphaBits);						/* Left justify alpha*/
			alpha |=     alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);				/* Double the number of bits in alpha (good enough for >= 4 bits) */
		#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
		#if FskName3(fsk,SrcPixelKind,AlphaBits) < 4
			alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits) * 2;				/* Quadruple the number of bits in alpha (good enough for >= 2 bits) */
		#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 4 */

		if	((alpha != 0) && ((alpha = (alpha == 255) ? scale : FskAlphaMul(alpha, scale)) != 0)) {	/* If the source is not transparent... */
			Fsk24BitType s24;
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src, s24);								/* Convert the src to the dst format */
			if      (alpha == 255)		*d = s24;													/* If the source is opaque, write it directly */
			else if (!params->isPremul) FskBlend24(d, s24, alpha);									/* If    straight   alpha,  blend it in */
			else						*d = FskAXPY24(~alpha, *d, FskAlphaScale24(scale, s24));	/* If premultiplied alpha,  scale and add it to the scaled destination */
		}
	} BLIT_ENDLOOP
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
	Fsk24BitType tint, pix;

	tint.c[FskName3(fsk,DstPixelKind,RedPosition  )] = params->red;
	tint.c[FskName3(fsk,DstPixelKind,GreenPosition)] = params->green;
	tint.c[FskName3(fsk,DstPixelKind,BluePosition )] = params->blue;

	#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
		if (params->alpha == 255) {
			BLIT_STARTLOOP {
				GetSrcAsDst(s, pix);
				*d = FskPixelMul24(pix, tint);
			} BLIT_ENDLOOP
		}
		else {
			BLIT_STARTLOOP {
				GetSrcAsDst(s, pix);
				pix = FskPixelMul24(pix, tint);
				FskBlend24(d, pix, params->alpha);
			} BLIT_ENDLOOP
		}
	#else /* FskName3(fsk,SrcPixelKind,AlphaBits) == 8 */
		BLIT_STARTLOOP {
			UInt32 alpha = (UInt8)(*s >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract alpha from the source */
			GetSrcAsDst(s, pix);
			alpha *= params->alpha; alpha += 0x80; alpha += alpha >> 8; alpha >>= 8;					/* Multiply alpha and scale */
			pix = FskName2(FskPixelMul,DstPixelKind)(pix, tint);
			FskName2(FskBlend,DstPixelKind)(d, pix, (UInt8)alpha);
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
	Fsk24BitType tint, pix;

	tint.c[FskName3(fsk,DstPixelKind,RedPosition  )] = params->red;
	tint.c[FskName3(fsk,DstPixelKind,GreenPosition)] = params->green;
	tint.c[FskName3(fsk,DstPixelKind,BluePosition )] = params->blue;

	#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
		if (params->alpha == 255) {
			BLIT_STARTLOOP {
				BilerpSrcAsDst(s, pix);
				*d = FskPixelMul24(pix, tint);
			} BLIT_ENDLOOP
		}
		else {
			BLIT_STARTLOOP {
				BilerpSrcAsDst(s, pix);
				pix = FskPixelMul24(pix, tint);
				FskBlend24(d, pix, params->alpha);
			} BLIT_ENDLOOP
		}
	#else /* FskName3(fsk,SrcPixelKind,AlphaBits) == 8 */
		BLIT_STARTLOOP {
			UInt32 sPix = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);
			UInt32 alpha = (UInt8)(sPix >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract alpha from the source */
			alpha *= params->alpha; alpha += 0x80; alpha += alpha >> 8; alpha >>= 8;					/* Multiply alpha and scale */
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(sPix, pix);
			pix = FskName2(FskPixelMul,DstPixelKind)(pix, tint);
			FskName2(FskBlend,DstPixelKind)(d, pix, (UInt8)alpha);
		} BLIT_ENDLOOP
	#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) == 8 */
}


/********************************************************************************/


#undef GetSrcAsDst
#undef BilerpSrcAsDst


#endif /* FskName2(SRC_,FskName3(fsk,SrcPixelKind,KindFormat)) && FskName2(DST_,FskName3(fsk,DstPixelKind,KindFormat)) */


#undef SrcPixelKind
#undef DstPixelKind

