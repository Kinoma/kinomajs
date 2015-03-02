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
/* Only generate the procs if we're going to use them */
#if		FskName2(SRC_,FskName3(fsk,SrcPixelKind,KindFormat))						\
	&&	FskName2(DST_UNITY_,FskName3(fsk,DstPixelKind,KindFormat))					\
	&&	(FskName3(fsk,DstPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)

#if (FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)


#if   (FskName3(fsk,SrcPixelKind,Bytes) != 3) && (FskName3(fsk,DstPixelKind,Bytes) != 3)
	#define GetSrcAsDst(s, pix)		pix = *s;	FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix)
#else
	#define GetSrcAsDst(s, pix)		/* */		FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, pix)
#endif




#if		(FskName3(fsk,SrcPixelKind,Bytes)         == FskName3(fsk,DstPixelKind,Bytes))			\
	&&	(FskName3(fsk,SrcPixelKind,Bytes)         < 3)											\
	&&	(FskName3(fsk,SrcPixelKind,RedPosition)   == FskName3(fsk,DstPixelKind,RedPosition))	\
	&&	(FskName3(fsk,SrcPixelKind,GreenPosition) == FskName3(fsk,DstPixelKind,GreenPosition))	\
	&&	(FskName3(fsk,SrcPixelKind,BluePosition)  == FskName3(fsk,DstPixelKind,BluePosition))


/********************************************************************************
 * FskUnityCopy for homogeneous pixels
 ********************************************************************************/

static void
FskName3(FskUnityCopy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	FskUnityCopyHomogeneousRect(params, FskName3(fsk,SrcPixelKind,Bytes));
}

#else


/********************************************************************************
 * FskUnityCopy for heterogeneous pixels
 ********************************************************************************/

static void
FskName3(FskUnityCopy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	UInt32			width	= params->dstWidth,				height	= params->dstHeight;
	SInt32			srb		= params->srcRowBytes, 			drb		= params->dstRowBytes;
	const UInt8		*s0		= (const UInt8*)(params->srcBaseAddr)
							+ (params->srcY0 >> FWD_SUBBITS) * srb
							+ (params->srcX0 >> FWD_SUBBITS) * FskName3(fsk,SrcPixelKind,Bytes);
	UInt8			*d0		= (UInt8*)(params->dstBaseAddr);
	register UInt32	w;
	register const FskName3(Fsk,SrcPixelKind,Type)	*s;
	register FskName3(Fsk,DstPixelKind,Type)		*d;

	for ( ; height--; s0 += srb, d0 += drb) {
		for (w = width, s = (const FskName3(Fsk,SrcPixelKind,Type)*)s0, d = (FskName3(Fsk,DstPixelKind,Type)*)d0; w--;
			#if FskName3(fsk,SrcPixelKind,Bytes) != 3
				s++,
			#else /* FskName3(fsk,SrcPixelKind,Bytes) == 3 */
				s = (const FskName3(Fsk,SrcPixelKind,Type*))((const char*)s + FskName3(fsk,SrcPixelKind,Bytes)),
			#endif /* FskName3(fsk,SrcPixelKind,Bytes) == 3 */
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				d++
			#else /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
				d = (FskName3(Fsk,DstPixelKind,Type*))((char*)d + FskName3(fsk,DstPixelKind,Bytes))
			#endif /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
		) {
			#if (FskName3(fsk,DstPixelKind,Bytes) != 3)
				UInt32 pix;
				GetSrcAsDst(s, pix);
				*d = (FskName3(Fsk,DstPixelKind,Type))pix;
			#elif (FskName3(fsk,SrcPixelKind,Bytes) != 3)
				UInt32 pix = *s;
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix, *d);
			#else /* 3 bytes -> 3 bytes */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, *d);
			#endif /* FskName3(fsk,SrcPixelKind,Bytes) == 3 */
		}
	}
}

#endif /* Heterogeneous copy */


#if FskName3(fsk,SrcPixelKind,AlphaBits) > 1


/********************************************************************************
 * FskUnityAlpha
 ********************************************************************************/

static void
FskName3(FskUnityAlpha,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	UInt32			width	= params->dstWidth,				height	= params->dstHeight;
	SInt32			srb		= params->srcRowBytes, 			drb		= params->dstRowBytes;
	const UInt8		*s0		= (const UInt8*)(params->srcBaseAddr)
							+ (params->srcY0 >> FWD_SUBBITS) * srb
							+ (params->srcX0 >> FWD_SUBBITS) * FskName3(fsk,SrcPixelKind,Bytes);
	UInt8			*d0		= (UInt8*)(params->dstBaseAddr);
	UInt32			w, pix;
	register const FskName3(Fsk,SrcPixelKind,Type)	*s;
	register FskName3(Fsk,DstPixelKind,Type)		*d;


	/*
	 * Premultiplied alpha
	 */

	if (params->isPremul) {
		for ( ; height--; s0 += srb, d0 += drb) {
			for (w = width, s = (const FskName3(Fsk,SrcPixelKind,Type)*)s0, d = (FskName3(Fsk,DstPixelKind,Type)*)d0; w--; s++, d++) {

			/************************ {32ARGB, 32ABGR, 32RGBA, 32BGRA} --> {24RGB, 24BGR} ************************/
				#if (FskName3(fsk,DstPixelKind,Bytes) == 3) && (FskName3(fsk,SrcPixelKind,RedBits) == 8)
					FskName3(fskConvert,DstPixelKind,SrcPixelKind)(*d, pix);						/* Convert dst to src pixel type */
					FskName2(FskAlphaBlackSourceOver,SrcPixelKind)(&pix, *s);						/* Use our special alpha compositing operator */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix, *d);						/* Convert from ARGB to dst */

			/************************ {32A16RGB565SE, 16AG, 16RGBA4444} --> {24RGB, 24BGR} ************************/
				#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)
					UInt32 src = *s;
					FskName3(fskConvert,SrcPixelKind,32ARGB)(src);									/* Convert src to ARGB */
					FskName3(fskConvert,DstPixelKind,32ARGB)(*d, pix);								/* Convert dst to ARGB */
					FskAlphaBlackSourceOver32ARGB(&pix, src);										/* Use our special alpha compositing operator */
					FskName3(fskConvert,32ARGB,DstPixelKind)(pix, *d);								/* Convert from ARGB to dst */

			/************************ {32ARGB, 32ABGR, 32RGBA, 32BGRA, 32A16RGB565SE, 16AG, 16RGBA4444, 8A} --> {32ARGB, 32ABGR, 32RGBA, 32BGRA} ************************/
				#elif (FskName3(fsk,SrcPixelKind,AlphaBits) == 8) && (FskName3(fsk,DstPixelKind,RedBits) == 8)
					UInt8 alpha = (UInt8)((pix = *s) >> FskName3(fsk,SrcPixelKind,AlphaPosition));	/* Extract alpha from the source */
					#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
						alpha <<= (8 - FskName3(fsk,SrcPixelKind,AlphaBits));						/* Convert to ... */
						alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);						/* ... 8 bits */
					#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
					if (alpha != 0) {																/* If the source is not transparent... */
						UInt32 dst;
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);						/* ... convert to destination pixel format */
						if (((alpha = 255 - alpha) != 0) &&											/* If the source is not opaque ... */
							((((dst=*d) >> FskName3(fsk,DstPixelKind,AlphaPosition)) & 0xFF) != 0)	/* ... and the destination is not transparent ... */
						)
							pix = FskAXPY8888(alpha, dst, pix);										/* ... mix some of the dst with the src */
						*d = pix;																	/* Update the destination */
					}

			/************************* {32ARGB, 32ABGR, 32RGBA, 32BGRA, 32A16RGB565SE, 16AG, 16RGBA4444} --> 16RGB565LE *************************/
				#elif	(FskName3(fsk,DstPixelKind,RedPosition)   == fsk16RGB565SERedPosition)   && \
						(FskName3(fsk,DstPixelKind,GreenPosition) == fsk16RGB565SEGreenPosition) && \
						(FskName3(fsk,DstPixelKind,BluePosition)  == fsk16RGB565SEBluePosition)
					UInt8 alpha = (UInt8)((pix = *s) >> FskName3(fsk,SrcPixelKind,AlphaPosition));	/* Extract alpha from the source */
					#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
						alpha <<= (8 - FskName3(fsk,SrcPixelKind,AlphaBits));						/* Convert to ... */
						alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);						/* ... 8 bits */
					#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
					alpha >>= 2;																	/* 6 bit alpha. Guaranteed that it doesn't overflow when rounding */
					if (alpha != 0) {																/* If the source is not transparent ... */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);						/* ... convert it to dst pixel format */
						if ((alpha = 63 - alpha) != 0)												/* If the source is not opaque ... */
							pix = FskAXPY16RGB565SE(alpha, *d, (UInt16)pix);						/* ... blend some of the dst into the src */
						*d = (UInt16)pix;
					}

			/************************ {32ARGB, 32ABGR, 32RGBA, 32BGRA} --> {32A16RGB565LE, 16RGBA4444, 8G} ************************/
				#elif (FskName3(fsk,SrcPixelKind,Bytes) == 4) && (FskName3(fsk,SrcPixelKind,RedBits) == 8)
					pix = *d;																		/* Get destination pixel */
					FskName3(fskConvert,DstPixelKind,SrcPixelKind)(pix);							/* Convert to source pixel format */
					FskName2(FskAlphaBlackSourceOver,SrcPixelKind)(&pix, *s);						/* Use our special alpha compositing operator */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);							/* Convert to destination pixel format */
					*d = pix;

			/************************************************ General case ************************************************/
				#else
					UInt32 src = *s;
					FskName3(fskConvert,SrcPixelKind,32ARGB)(src);									/* Convert src to ARGB */
					#if FskName3(fsk,DstPixelKind,GreenBits) == 6									/* If 16RGB565LE */
						src &= ~(3 << fsk32ARGBAlphaBits);											/* Clear LS 2 bits of alpha */
						src |= (src >> 6) & (3 << fsk32ARGBAlphaBits);								/* Replicate MS 2 bits into LS 2 bits of alpha */
					#endif /* FskName3(fsk,DstPixelKind,GreenBits) == 6 */
					pix = *d;
					FskName3(fskConvert,DstPixelKind,32ARGB)(pix);									/* Convert dst to ARGB */
					FskAlphaBlackSourceOver32ARGB(&pix, src);										/* Use our special alpha compositing operator */
					FskName3(fskConvert,32ARGB,DstPixelKind)(pix);									/* Convert from ARGB to dst */
					*d = pix;
				#endif /* premul */
			}
		}
	}


	/*
	 * Straight alpha
	 */

	else {
		for ( ; height--; s0 += srb, d0 += drb) {

			/************************  --> {8, 16, 32} ************************/
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				for (w = width, s = (const FskName3(Fsk,SrcPixelKind,Type)*)s0, d = (FskName3(Fsk,DstPixelKind,Type)*)d0; w--; s++, d++) {
					pix = *s;																			/* Get source pixel */

			/************************ {32ARGB, 32ABGR, 32RGBA, 32BGRA, 32A16RGB565SE, 16AG, 16RGBA4444} --> {32ARGB, 32ABGR, 32RGBA, 32BGRA, 32A16RGB565SE, 16AG, 16RGBA4444} ************************/
					#if	FskName3(fsk,DstPixelKind,AlphaBits) >= 4										/* If we have enough alpha bits in the dst */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);							/* Convert to destination pixel format */
						FskName2(FskAlpha,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))pix);		/* Use our special alpha compositing operator */

			/************************ {*} --> {16RGB565LE, 8G} ************************/
					#else																				/* Otherwise (dst does not have enough alpha bits) ... */
					{ 	UInt8 alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition));			/* Extract alpha from the source */
						#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
							alpha <<= (8 - FskName3(fsk,SrcPixelKind,AlphaBits));						/* Convert to ... */
							alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);						/* ... 8 bits */
						#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
						if (alpha != 0) {																/* If the source is not transprent */
							FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);						/* Convert to destination pixel format */
							if (alpha == 255)	*d = (FskName3(Fsk,DstPixelKind,Type))pix;				/* If the source is opaque, write it in directly */
							else				FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))pix, alpha);	/* Otherwise, blend */
						}
					}
					#endif
				}

			/************************ {*} --> {24} ************************/
			#else /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
				for (w = width, s = (const FskName3(Fsk,SrcPixelKind,Type)*)s0, d = (FskName3(Fsk,DstPixelKind,Type)*)d0; w--;
					s++, d = (FskName3(Fsk,DstPixelKind,Type*))((char*)d + FskName3(fsk,DstPixelKind,Bytes))
				) {
					UInt8 alpha;
					pix = *s;																			/* Get source pixel */
					alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition));					/* Extract alpha */
					#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
						alpha <<= (8 - FskName3(fsk,SrcPixelKind,AlphaBits));							/* Convert to ... */
						alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);							/* ... 8 bits */
					#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
					if (alpha != 0) {																	/* If the source is not transparent */
						Fsk24BitType sPix;
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix, sPix);						/* Convert to destination pixel format */
						if (alpha == 255)	*d = sPix;													/* If the source is opaque, write it directly */
						else				FskName2(FskBlend,DstPixelKind)(d, sPix, alpha);			/* Otherwise, blend */
					}
				}
			#endif /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
		}
	}
}



#else /* FskName3(fsk,SrcPixelKind,AlphaBits) <= 1 */

	#if COMPILER_CAN_GENERATE_CONST_PROC_POINTERS
		static const FskRectTransferProc FskName3(FskUnityAlpha,SrcPixelKind,DstPixelKind) = NULL;
	#else /* It will be necessary to #define XXXXXX NULL in the calling program */
	#endif /* COMPILER_CAN_GENERATE_CONST_PROC_POINTERS */

#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) > 1 */


#undef GetSrcAsDst

#else /* !fskUniformChunkyPixelPacking */

	#if COMPILER_CAN_GENERATE_CONST_PROC_POINTERS
		static const FskRectTransferProc FskName3(FskUnityCopy,SrcPixelKind,DstPixelKind) = NULL;
		static const FskRectTransferProc FskName3(FskUnityAlpha,SrcPixelKind,DstPixelKind) = NULL;
	#else /* It will be necessary to #define XXXXXX NULL in the calling program */
	#endif /* COMPILER_CAN_GENERATE_CONST_PROC_POINTERS */

#endif /* !fskUniformChunkyPixelPacking */

#endif /* FskName2(SRC_,FskName3(fsk,SrcPixelKind,KindFormat)) && FskName2(DST_UNITY_,FskName3(fsk,DstPixelKind,KindFormat)) */


#undef SrcPixelKind
#undef DstPixelKind
