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
/* Only generate the procs if we're going to use them and this file is appropriate */
#if		FskName2(SRC_,FskName3(fsk,SrcPixelKind,KindFormat))						\
	&&	FskName2(DST_,FskName3(fsk,DstPixelKind,KindFormat))						\
	&&	(FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)	\
	&&	(FskName3(fsk,DstPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)	\
	&&	(FskName3(fsk,DstPixelKind,Bytes) == 4)										\


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
 ********************************************************************************/

static void
FskName4(Fsk,Copy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	BLIT_STARTLOOP {
		UInt32 pix;
		GetSrcAsDst(s, pix);
		*d = pix;
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
		*d = pix;
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
		FskName2(FskBlend,DstPixelKind)(d, pix, alpha);
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
		FskName2(FskBlend,DstPixelKind)(d, pix, alpha);
	} BLIT_ENDLOOP
}


#if FskName3(fsk,SrcPixelKind,AlphaBits) > 1


/********************************************************************************
 * Alpha
 ********************************************************************************/

static void
FskName4(Fsk,Alpha,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{

/************************ {32ARGB, 32ABGR, 32RGBA, 32BGRA, 32A16RGB565SE, 16AG} --> {32ARGB, 32ABGR, 32RGBA, 32BGRA} ************************/
#if (FskName3(fsk,SrcPixelKind,AlphaBits) == 8) && (FskName3(fsk,DstPixelKind,RedBits) == 8)
	BLIT_DECL;
	UInt32 src;
	UInt8 alpha;

	/* Straight alpha */
	if (0 == params->isPremul) {
		BLIT_STARTLOOP {
			if ((alpha = (UInt8)((src = *s) >> FskName3(fsk,SrcPixelKind,AlphaPosition))) != 0) {	/* If the source is not transparent */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src);								/* Convert the source to destination format */
				if (alpha < 255)	FskBlend32(d, src |= 255 << FskName3(fsk,DstPixelKind,AlphaPosition), alpha);
				else				*d = src;
			}
		} BLIT_ENDLOOP
	}

	/* Premultiplied alpha */
	else {
		BLIT_STARTLOOP {
			if ((alpha = (UInt8)((src = *s) >> FskName3(fsk,SrcPixelKind,AlphaPosition))) != 0) {	/* If the source is not transparent... */
				UInt32 dst;
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src);								/* Convert the source to destination format */
				if (((alpha = 255 - alpha) != 0) &&													/* If the source is not opaque ... */
					((((dst = *d) >> FskName3(fsk,DstPixelKind,AlphaPosition)) & 0xFF) != 0)		/* ... and the destination is not transparent ...  (ARGB) */
				)
					src = FskAXPY8888(alpha, dst, src);
				*d = src;																			/* Update the destination */
			}
		} BLIT_ENDLOOP
	}

/************************ {32ARGB, 32ABGR, 32RGBA, 32BGRA, 32A16RGB565SE, 16AG, 16RGBA4444LE} --> {32A16RGB565SE} ************************/
#else
	BLIT_DECL;

	/* Straight alpha */
	if (0 == params->isPremul) {
		BLIT_STARTLOOP {
			UInt32 src;
			GetSrcAsDst(s, src);
			FskName2(FskAlpha,DstPixelKind)(d, src);
		} BLIT_ENDLOOP
	}

	/* Premultiplied alpha */
	else {
		BLIT_STARTLOOP {
			UInt32 src;
			GetSrcAsDst(s, src);
			FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, src);
		} BLIT_ENDLOOP
	}
#endif
}


/********************************************************************************
 * Bilinear Alpha
 ********************************************************************************/

static void
FskName4(FskBilinear,Alpha,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;

/************************ {32ARGB, 32ABGR, 32RGBA, 32BGRA, 32A16RGB565SE, 16RGBA4444LE, 16AG} --> {32ARGB, 32ABGR, 32RGBA, 32BGRA} ************************/
#if FskName3(fsk,DstPixelKind,RedBits) == 8													/* 8888 bit pixel formats */
		UInt32 src;
		UInt8 alpha;

		/* Straight alpha */
		if (0 == params->isPremul) {
			BLIT_STARTLOOP {
				BilerpSrcAsDst(s, src);
				if ((alpha = FskMoveField(src, FskName3(fsk,DstPixelKind,AlphaBits), FskName3(fsk,DstPixelKind,AlphaPosition), 0)) != 0) {
					if (alpha < 255)	FskBlend32(d, src |= 255 << FskName3(fsk,DstPixelKind,AlphaPosition), alpha);
					else				*d = src;
				}
			} BLIT_ENDLOOP
		}

		/* Premultiplied alpha */
		else {
			BLIT_STARTLOOP {
				BilerpSrcAsDst(s, src);
				alpha = FskMoveField(src, FskName3(fsk,DstPixelKind,AlphaBits), FskName3(fsk,DstPixelKind,AlphaPosition), 0);
				if (alpha != 0) {																		/* If the source is not transparent... */
					UInt32 dst;
					if (((alpha = 255 - alpha) != 0) &&													/* If the source is not opaque ... */
						((((dst = *d) >> FskName3(fsk,DstPixelKind,AlphaPosition)) & 0xFF) != 0)		/* ... and the destination is not transparent ...  (ARGB) */
					)
						src = FskAXPY8888(alpha, dst, src);
					*d = src;
				}
			} BLIT_ENDLOOP
		}

/***** {32ARGB, 32ABGR, 32RGBA, 32BGRA, 32A16RGB565SE, 16RGBA4444LE, 16AG} --> 32A16RGB565LE *****/
	#else /* FskName3(fsk,DstPixelKind,RedBits) < 8 */

		/* Straight alpha */
		if (0 == params->isPremul) {
			BLIT_STARTLOOP {
				UInt32 src;
				BilerpSrcAsDst(s, src);
				FskName2(FskAlpha,DstPixelKind)(d, src);
			} BLIT_ENDLOOP
		}

		/* Premultiplied alpha */
		else {
			BLIT_STARTLOOP {
				UInt32 src;
				BilerpSrcAsDst(s, src);
				FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, src);
			} BLIT_ENDLOOP
		}
	#endif /* FskName3(fsk,DstPixelKind,RedBits) < 8 */
}


/********************************************************************************
 * AlphaBlend
 ********************************************************************************/

static void
FskName4(Fsk,AlphaBlend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	UInt8 alpha = params->alpha;
	BLIT_STARTLOOP {
		UInt32 pix;
		GetSrcAsDst(s, pix);
		if (!params->isPremul)	FskName2(FskAlphaBlend,DstPixelKind)(d, pix, alpha);
		else					FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, FskName2(FskAlphaScale,DstPixelKind)(alpha, pix));
	} BLIT_ENDLOOP
}


/********************************************************************************
 * Bilinear AlphaBlend
 ********************************************************************************/

static void
FskName4(FskBilinear,AlphaBlend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	UInt8 alpha = params->alpha;
	BLIT_STARTLOOP {
		UInt32 pix;
		BilerpSrcAsDst(s, pix);
		if (!params->isPremul)	FskName2(FskAlphaBlend,DstPixelKind)(d, pix, alpha);
		else					FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, FskName2(FskAlphaScale,DstPixelKind)(alpha, pix));
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
	UInt32 tint, pix;

	#if FskName3(fsk,SrcPixelKind,RedBits) == 8 && FskName3(fsk,SrcPixelKind,GreenBits) == 8 && FskName3(fsk,SrcPixelKind,BlueBits) == 8 && FskName3(fsk,SrcPixelKind,AlphaBits) == 8
		tint	= (params->red   << FskName3(fsk,SrcPixelKind,RedPosition  ))
				| (params->green << FskName3(fsk,SrcPixelKind,GreenPosition))
				| (params->blue  << FskName3(fsk,SrcPixelKind,BluePosition ))
				| (params->alpha << FskName3(fsk,SrcPixelKind,AlphaPosition));
		BLIT_STARTLOOP {
			#if   (FskName3(fsk,SrcPixelKind,AlphaPosition) ==      FskName3(fsk,DstPixelKind,AlphaPosition))  && (FskName3(fsk,SrcPixelKind,RedPosition)   ==       FskName3(fsk,DstPixelKind,RedPosition))
				pix = FskPixelMul32(*s, tint);									/* ARGB->ARGB or BGRA->BGRA optimization */
			#elif (FskName3(fsk,SrcPixelKind,AlphaPosition) == (3 - FskName3(fsk,DstPixelKind,AlphaPosition))) && (FskName3(fsk,SrcPixelKind,RedPosition)   == (3 - FskName3(fsk,DstPixelKind,RedPosition)))
				pix = FskPixelMul32Swap(*s, tint);								/* ARGB->BGRA or BGRA->ARGB optimization */
			#else
				pix = FskPixelMul32(*s, tint);									/* The general case with big src */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);
			#endif /* (FskName3(fsk,SrcPixelKind,Bytes) != 4) || not same or endian flipped */
			if (!params->isPremul)	FskName2(FskAlpha, DstPixelKind)(d, pix);
			else					FskName2(FskAlphaBlackSourceOver, DstPixelKind)(d, pix);
		} BLIT_ENDLOOP
	#else /* Source is less accurate than dst */
		#if FskName3(fsk,DstPixelKind,RedBits) == 8 && FskName3(fsk,DstPixelKind,GreenBits) == 8 && FskName3(fsk,DstPixelKind,BlueBits) == 8 && FskName3(fsk,DstPixelKind,AlphaBits) == 8
			tint	= (params->red   << FskName3(fsk,DstPixelKind,RedPosition  ))
					| (params->green << FskName3(fsk,DstPixelKind,GreenPosition))
					| (params->blue  << FskName3(fsk,DstPixelKind,BluePosition ))
					| (params->alpha << FskName3(fsk,DstPixelKind,AlphaPosition));
		#else /* Dst is not 8888 */
			tint	= (params->red   << FskName3(fsk,32RGBA,RedPosition  ))
					| (params->green << FskName3(fsk,32RGBA,GreenPosition))
					| (params->blue  << FskName3(fsk,32RGBA,BluePosition ))
					| (params->alpha << FskName3(fsk,32RGBA,AlphaPosition));
			FskName3(fskConvert,32RGBA,DstPixelKind)(pix);
		#endif
		BLIT_STARTLOOP {
			GetSrcAsDst(s, pix);
			pix = FskPixelMul32(pix, tint);
			if (!params->isPremul)	FskName2(FskAlpha, DstPixelKind)(d, pix);
			else					FskName2(FskAlphaBlackSourceOver, DstPixelKind)(d, pix);
		} BLIT_ENDLOOP
	#endif /* Source is less accurate than dst */
}


/********************************************************************************
 * Bilinear TintCopy
 ********************************************************************************/

static void
FskName4(FskBilinear,TintCopy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;
	UInt32 tint, pix;

	#if (FskName3(fsk,SrcPixelKind,RedBits) == 8) && (FskName3(fsk,SrcPixelKind,GreenBits) == 8) && (FskName3(fsk,SrcPixelKind,BlueBits) == 8) && (FskName3(fsk,SrcPixelKind,AlphaBits) == 8)
		tint	= (params->red   << FskName3(fsk,SrcPixelKind,RedPosition  ))
				| (params->green << FskName3(fsk,SrcPixelKind,GreenPosition))
				| (params->blue  << FskName3(fsk,SrcPixelKind,BluePosition ))
				| (params->alpha << FskName3(fsk,SrcPixelKind,AlphaPosition));
		BLIT_STARTLOOP {
			pix = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);
			#if   (FskName3(fsk,SrcPixelKind,AlphaPosition) ==      FskName3(fsk,DstPixelKind,AlphaPosition))  && (FskName3(fsk,SrcPixelKind,RedPosition)   ==       FskName3(fsk,DstPixelKind,RedPosition))
				pix = FskPixelMul32(    pix, tint);	/* ARGB->ARGB or BGRA->BGRA */
			#elif (FskName3(fsk,SrcPixelKind,AlphaPosition) == (3 - FskName3(fsk,DstPixelKind,AlphaPosition))) && (FskName3(fsk,SrcPixelKind,RedPosition)   == (3 - FskName3(fsk,DstPixelKind,RedPosition)))
				pix = FskPixelMul32Swap(pix, tint);	/* ARGB->BGRA or BGRA->ARGB */
			#else
				pix = FskPixelMul32(pix, tint);													/* The general case */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);
			#endif /* (FskName3(fsk,SrcPixelKind,Bytes) != 4) || not same or endian flipped */
			if (!params->isPremul)	FskName2(FskAlpha, DstPixelKind)(d, pix);
			else					FskName2(FskAlphaBlackSourceOver, DstPixelKind)(d, pix);
		} BLIT_ENDLOOP
	#else /* Source is less accurate than dst */
		#if (FskName3(fsk,DstPixelKind,RedBits) == 8) && (FskName3(fsk,DstPixelKind,GreenBits) == 8) && (FskName3(fsk,DstPixelKind,BlueBits) == 8) && (FskName3(fsk,DstPixelKind,AlphaBits) == 8)
			tint	= (params->red   << FskName3(fsk,DstPixelKind,RedPosition  ))
					| (params->green << FskName3(fsk,DstPixelKind,GreenPosition))
					| (params->blue  << FskName3(fsk,DstPixelKind,BluePosition ))
					| (params->alpha << FskName3(fsk,DstPixelKind,AlphaPosition));
		#else /* Dst is not 8888 */
			tint	= (params->red   << FskName3(fsk,32RGBA,RedPosition  ))
					| (params->green << FskName3(fsk,32RGBA,GreenPosition))
					| (params->blue  << FskName3(fsk,32RGBA,BluePosition ))
					| (params->alpha << FskName3(fsk,32RGBA,AlphaPosition));
			FskName3(fskConvert,32RGBA,DstPixelKind)(pix);
		#endif
		BLIT_STARTLOOP {
			BilerpSrcAsDst(s, pix);
			pix = FskPixelMul32(pix, tint);
			if (!params->isPremul)	FskName2(FskAlpha, DstPixelKind)(d, pix);
			else					FskName2(FskAlphaBlackSourceOver, DstPixelKind)(d, pix);
		} BLIT_ENDLOOP
	#endif /* Source is less accurate than dst */
}


/********************************************************************************/


#undef GetSrcAsDst
#undef BilerpSrcAsDst


#endif /* FskName2(SRC_,FskName3(fsk,SrcPixelKind,KindFormat)) && FskName2(DST_,FskName3(fsk,DstPixelKind,KindFormat)) */


#undef SrcPixelKind
#undef DstPixelKind

