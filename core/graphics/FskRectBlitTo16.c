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
#if FskName2(SRC_,FskName3(fsk,SrcPixelKind,KindFormat)) &&	FskName2(DST_,FskName3(fsk,DstPixelKind,KindFormat))						\
	&&	(FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)	\
	&&	(FskName3(fsk,DstPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)	\
	&&	(FskName3(fsk,DstPixelKind,Bytes) == 2)										\


/********************************************************************************
 ********************************************************************************
 **		GetSrcAsDst() and BilerpSrcAsDst()
 ********************************************************************************
 ********************************************************************************/

#if   (FskName3(fsk,SrcPixelKind,Bytes) == 1) || (FskName3(fsk,SrcPixelKind,Bytes) == 2) || (FskName3(fsk,SrcPixelKind,Bytes) == 4)
	#define GetSrcAsDst(s, pix)		pix = *s;												FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix)
	#define BilerpSrcAsDst(s, pix)	pix = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);	FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix)
#elif FskName3(fsk,SrcPixelKind,Bytes) == 3
	#define GetSrcAsDst(s, pix)		/* */													FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, pix)
	#define BilerpSrcAsDst(s, pix)	Fsk24BitType pix24 = FskBilerp24(xf, yf, s, srb);		FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix24, pix)
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
		*d = (UInt16)pix;
	} BLIT_ENDLOOP
}


/********************************************************************************
 * Bilinear Copy
 ********************************************************************************/
#if (FskName3(fsk,SrcPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits) && (FskName3(fsk,DstPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits) && defined(MARVELL_SOC_PXA168)
	extern void		FskBilinearCopySetImm_arm_wMMX_s(void);
	//extern UInt16	FskBilinearCopyRGB565_arm_wMMX_s(UInt32 p0001,UInt32 p1011,UInt32 di0i,UInt32 dj0j);
	extern void FskBilinearCopyRGB565_arm_wMMX_s(UInt16 *d, const UInt16 *sr, UInt32 dy, FskFixed x, FskFixed xd, SInt32 srb, UInt32 width);
#endif /* 16RGB565SE && MARVELL_SOC_PXA168 */

static void
FskName4(FskBilinear,Copy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	#if (FskName3(fsk,SrcPixelKind,RedPosition) == fsk16RGB565SERedPosition) && (FskName3(fsk,DstPixelKind,RedPosition) == fsk16RGB565SERedPosition)	/* 16RGB565LE || 32A16RGB565LE */
	#if (FskName3(fsk,SrcPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits) && (FskName3(fsk,DstPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits)			/* 16RGB565LE */
	/* When doing 16RGB565SE, inline the bilinear interpolation. */
	UInt32 width, height = params->dstHeight;
	SInt32 sourcerb = params->srcRowBytes, destrb = params->dstRowBytes;

	FskFixed x0 = params->srcX0, y = params->srcY0;
	FskFixed xd = params->srcXInc, yd = params->srcYInc;
	register FskFixed x;
	const UInt32 mask1 = 0x07E0F81F;
	#ifndef MARVELL_SOC_PXA168
		const UInt32 mask2 = 0x0FE1F83F;
	#endif /* MARVELL_SOC_PXA168 */

	const UInt8		*s0 = (const UInt8*)(params->srcBaseAddr);
	UInt16			*sr = (UInt16*)(s0 + (y >> FWD_SUBBITS) * sourcerb), *s;
	const UInt16	*d0 = (UInt16*)params->dstBaseAddr;
	UInt16			*dr = (UInt16*)d0, *d;

#define FskCopyRGB565_Linear(p0, p1, di, di0) \
	do {\
		p0 |= p0 << 16; p0 &= mask1;\
		p1 |= p1 << 16; p1 &= mask1;\
		p0 = ((p0 * di0 + p1 * di) >> 4) & mask1;\
		p0 |= (p0 >> 16);\
	}while (0)

#ifdef MARVELL_SOC_PXA168
	FskBilinearCopySetImm_arm_wMMX_s();
#endif /* MARVELL_SOC_PXA168 */
	for ( ; height--; ) {
		UInt32 dj, dj0, di, di0;
		UInt32 p00, p01;
		d = dr;
		dj = yf;

		if(0 == dj) {
			for (x = x0, width = params->dstWidth; width; width--) {
				s = (sr + (x >> FWD_SUBBITS));
				di = xf;
				p00 = *s;
				if(0 != di) {
					di0 = (1 << 4) - di;
					p01 = *(s + 1); // Next pixel
					FskCopyRGB565_Linear(p00, p01, di, di0);
				}
				else { /* Do nothing */
				}
				x += xd;

				*d = (UInt16)p00;
				d++;
		 	}
		}
		else {
			#ifdef MARVELL_SOC_PXA168
				FskBilinearCopyRGB565_arm_wMMX_s(d, sr, dj, x0, xd, sourcerb, params->dstWidth);
			#else /* !MARVELL_SOC_PXA168 */
				UInt32 p10, p11;
				dj0 = (1 << 4) - dj;
				for (x = x0, width = params->dstWidth; width--; ) {
					s = (sr + (x >> FWD_SUBBITS));
					di = xf;
					p00 = *s;

					if(0 != di) {
						di0 = (1 << 4) - di;
						p10 = (UInt32)sourcerb;
						p11 = (UInt32)sourcerb + sizeof(UInt16);
						p01 = *(s + 1); // Next pixel
						p10 = *((const UInt16*)(((const char*)(s)) + (SInt32)p10));
						p11 = *((const UInt16*)(((const char*)(s)) + (SInt32)p11));
						p00 |= p00 << 16; p00 &= mask1;
						p01 |= p01 << 16; p01 &= mask1;
						p10 |= p10 << 16; p10 &= mask1;
						p11 |= p11 << 16; p11 &= mask1;

						p00 = (((p00 * di0 + p01 * di) >> 3) & mask2) * dj0 + (((p10 * di0 + p11 * di) >> 3) & mask2) * dj;
						p00 = (p00 >> 5) & mask1;
						p00 |= (p00 >> 16);
					}
					else {
						p10 = *((const UInt16*)(((const char*)(s)) + (UInt32)sourcerb));
						FskCopyRGB565_Linear(p00, p10, dj, dj0);
					}

					*d = (UInt16)p00;
					d++;
					x += xd;
				}
			#endif /* !MARVELL_SOC_PXA168 */
		}
		dr = (UInt16*)((UInt8*) dr + destrb);
		y += yd;
		sr = (UInt16*)(s0 + (y >> FWD_SUBBITS) * sourcerb);
	}
	#else	/* 32A16RGB565LE */
		BLIT_DECL;
		const UInt32 mask1 = 0x07E0F81F;
	 	const UInt32 mask2 = 0x0FE1F83F;
		BLIT_STARTLOOP {
			UInt32 p00, p01, p10, p11;
			UInt32 di, dj, di0, dj0;
			di = xf;
			dj = yf;
			p00 = *(const UInt16*)s;
			p01 = sizeof(FskName3(Fsk,SrcPixelKind,Type));	if (di == 0)	p01 = 0;
			p10 = (UInt32)srb;								if (dj == 0)	p10 = 0;
			p11 = p01 + p10;
			p01 = *((const UInt16*)(((const char*)(s)) + (SInt32)p01));
			p10 = *((const UInt16*)(((const char*)(s)) + (SInt32)p10));
			p11 = *((const UInt16*)(((const char*)(s)) + (SInt32)p11));
			p00 |= p00 << 16;	p00 &= mask1;
			p01 |= p01 << 16;	p01 &= mask1;
			p10 |= p10 << 16;	p10 &= mask1;
			p11 |= p11 << 16;	p11 &= mask1;
			di0 = (1 << 4) - di;
			dj0 = (1 << 4) - dj;
//			p00	= (((p00 * di0 + p01 * di + 0x00802004) >> 3) & mask2) * dj0 + (((p10 * di0 + p11 * di + 0x00802004) >> 3) & mask2) * dj;
//			p00 = ((p00 + 0x02008010) >> 5) & mask1;
			p00	= (((p00 * di0 + p01 * di) >> 3) & mask2) * dj0 + (((p10 * di0 + p11 * di) >> 3) & mask2) * dj;
			p00 = (p00 >> 5) & mask1;
			p00 |= (p00 >> 16);
			*d = (UInt16)p00;
		} BLIT_ENDLOOP
	#endif /* 32A16RGB565LE || 16RGB565LE */
	#else /* Sufficiently different */
		BLIT_DECL;
		BLIT_STARTLOOP {
			UInt32 pix;
			BilerpSrcAsDst(s, pix);
			*d = (UInt16)pix;
		} BLIT_ENDLOOP
	#endif /* Sufficiently different */
}


/********************************************************************************
 * Blend
 ********************************************************************************/
#if (FskName3(fsk,SrcPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits) && (FskName3(fsk,DstPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits) && defined(MARVELL_SOC_PXA168)
	extern void		FskAlphaBlendSetImm_arm_wMMX_s			(UInt8 alpha);
 	extern UInt32 FskAlphaBlendPartAlphaAlignD_arm_wMMX_s	(UInt16 *d, UInt32 width2,UInt16 *sr,SInt32 x,SInt32 xd);
void FskBilinearAlpha16RGB565SE16RGB565SE_wmmx(const FskRectBlitParams *params) {
	#define BlendCopyHWord() \
		do {\
			s = (sr + (x >> FWD_SUBBITS));\
			*d++ = *s;\
			x += xd;\
		}while(0)

	#define BlendAlphaHWord() \
		do {\
			UInt32 pix, drb, dg;\
			s = (sr + (x >> FWD_SUBBITS));\
			pix = *s;\
			dg = *d;\
			drb = 0xF81F & dg;\
			dg &= 0x07E0;\
			drb = ((pix & 0xF81F) - drb) * alpha + (drb << 6) - drb + 0x10020;\
			drb += (drb >> 6) & 0xF81F;\
			drb = (drb >> 6) & 0xF81F;\
			dg = ((pix & 0x07E0) - dg ) * alpha + (dg << 6) - dg + 0x00400;\
			dg += (dg >> 6) & 0x07E0;\
			dg = (dg >> 6) & 0x07E0;\
			*d++ = (UInt16)(drb | dg);\
			x += xd;\
		}while(0)

	UInt8 alpha = params->alpha >> 2;	/* 6 bit alpha */
	if( (alpha == 0) || (params->dstWidth == 0))
		return;
	else {
		UInt32 width, height = params->dstHeight;
		SInt32 sourcerb = params->srcRowBytes, destrb = params->dstRowBytes;

		FskFixed x0 = params->srcX0, y = params->srcY0;
		FskFixed xd = params->srcXInc, yd = params->srcYInc;
		register FskFixed x;

		const UInt8		*s0 = (const UInt8*)(params->srcBaseAddr);
		UInt16			*sr = (UInt16*)(s0 + (y >> FWD_SUBBITS) * sourcerb), *s;
		const UInt16	*d0 = (UInt16*)params->dstBaseAddr;
		UInt16			*dr = (UInt16*)d0, *d;
		UInt32 			width2;

		if(63 != alpha) {
			FskAlphaBlendSetImm_arm_wMMX_s(alpha);
			for ( ; height--; ) {
				d = dr;
				width = params->dstWidth;
				x = x0;

				// made dst word aligned
				if(2 & (int)d){
					BlendAlphaHWord();
					width--;
				}

				// made dst Dword aligned
				width2 = width >>1;
				if(width2){
					if(4 & (int)d){
						BlendAlphaHWord();
						BlendAlphaHWord();
						width2--;
					}
				}
				if(width2 & 1){
					BlendAlphaHWord();
					BlendAlphaHWord();
				}

				// now dst is DWord aligned
				width2 = width2 >>1;
				x = FskAlphaBlendPartAlphaAlignD_arm_wMMX_s(d, width2,sr,x,xd);
				d+= width2<<2;

				//Tail
				if(width & 1) {
					BlendAlphaHWord();
			 	}

				dr = (UInt16*)((UInt8*) dr + destrb);
				y += yd;
				sr = (UInt16*)(s0 + (y >> FWD_SUBBITS) * sourcerb);
			}
		}
		else {
			for ( ; height--; ) {
				d = dr;
				width = params->dstWidth;
				x = x0;

				while(width--) {
					BlendCopyHWord();
			 	}

				dr = (UInt16*)((UInt8*) dr + destrb);
				y += yd;
				sr = (UInt16*)(s0 + (y >> FWD_SUBBITS) * sourcerb);
			}
		}
	}
}
#endif /* 16RGB565SE && MARVELL_SOC_PXA168 */

static void
FskName4(Fsk,Blend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
#if (FskName3(fsk,SrcPixelKind,RedPosition) == fsk16RGB565SERedPosition) && (FskName3(fsk,DstPixelKind,RedPosition) == fsk16RGB565SERedPosition)	/* 16RGB565LE or 32A16RGB565LE */
	#if (FskName3(fsk,SrcPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits) && (FskName3(fsk,DstPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits) 		/* no alpha - 16RGB565LE */
	/* When doing 16RGB565SE, inline the blend. */
	#define BlendCopyHWord() \
		do {\
			s = (sr + (x >> FWD_SUBBITS));\
			*d++ = *s;\
			x += xd;\
		}while(0)

	#define BlendAlphaHWord() \
		do {\
			UInt32 pix, drb, dg;\
			s = (sr + (x >> FWD_SUBBITS));\
			pix = *s;\
			dg = *d;\
			drb = 0xF81F & dg;\
			dg &= 0x07E0;\
			drb = ((pix & 0xF81F) - drb) * alpha + (drb << 6) - drb + 0x10020;\
			drb += (drb >> 6) & 0xF81F;\
			drb = (drb >> 6) & 0xF81F;\
			dg = ((pix & 0x07E0) - dg ) * alpha + (dg << 6) - dg + 0x00400;\
			dg += (dg >> 6) & 0x07E0;\
			dg = (dg >> 6) & 0x07E0;\
			*d++ = (UInt16)(drb | dg);\
			x += xd;\
		}while(0)

	UInt8 alpha = params->alpha >> 2;	/* 6 bit alpha */
	if( (alpha == 0) || (params->dstWidth == 0))
		return;
	else {
		UInt32 width, height = params->dstHeight;
		SInt32 sourcerb = params->srcRowBytes, destrb = params->dstRowBytes;

		FskFixed x0 = params->srcX0, y = params->srcY0;
		FskFixed xd = params->srcXInc, yd = params->srcYInc;
		register FskFixed x;

		const UInt8		*s0 = (const UInt8*)(params->srcBaseAddr);
		UInt16			*sr = (UInt16*)(s0 + (y >> FWD_SUBBITS) * sourcerb), *s;
		const UInt16	*d0 = (UInt16*)params->dstBaseAddr;
		UInt16			*dr = (UInt16*)d0, *d;
		UInt32 			width2;

		if(63 != alpha) {
			for ( ; height--; ) {
				d = dr;
				width = params->dstWidth;
				x = x0;

				// made dst word aligned
				if(2 & (int)d){
					BlendAlphaHWord();
					width--;
				}

				// made dst Dword aligned
				width2 = width >>1;
				if(width2){
					if(4 & (int)d){
						BlendAlphaHWord();
						BlendAlphaHWord();
						width2--;
					}
				}
				if(width2 & 1){
					BlendAlphaHWord();
					BlendAlphaHWord();
				}

				// now dst is DWord aligned
				width2 = width2 >>1;
					while (width2--) {
						BlendAlphaHWord();
						BlendAlphaHWord();
						BlendAlphaHWord();
						BlendAlphaHWord();
					}

				//Tail
				if(width & 1) {
					BlendAlphaHWord();
			 	}

				dr = (UInt16*)((UInt8*) dr + destrb);
				y += yd;
				sr = (UInt16*)(s0 + (y >> FWD_SUBBITS) * sourcerb);
			}
		}
		else {
			for ( ; height--; ) {
				d = dr;
				width = params->dstWidth;
				x = x0;

				while(width--) {
					BlendCopyHWord();
			 	}

				dr = (UInt16*)((UInt8*) dr + destrb);
				y += yd;
				sr = (UInt16*)(s0 + (y >> FWD_SUBBITS) * sourcerb);
			}
		}
	}
	#else	/* 32A16RGB565LE */
	BLIT_DECL;
	UInt8 alpha = params->alpha >> 2;	/* 6 bit alpha */
	BLIT_STARTLOOP {
		UInt32 pix, drb, dg;
		GetSrcAsDst(s, pix);
		dg  = *d;
		drb = 0xF81F & dg;
		dg &= 0x07E0;
		drb = ((pix  & 0xF81F) - drb) * alpha + (drb << 6) - drb + 0x10020; drb += (drb >> 6) & 0xF81F; drb = (drb >> 6) & 0xF81F;
		dg  = ((pix  & 0x07E0) - dg ) * alpha + (dg  << 6) - dg  + 0x00400; dg  += (dg  >> 6) & 0x07E0; dg  = (dg  >> 6) & 0x07E0;
		*d = (UInt16)(drb |= dg);
	} BLIT_ENDLOOP
	#endif	/* 32A16RGB565LE || 16RGB565LE */
#else /* !(32A16RGB565LE || 16RGB565LE) */
	BLIT_DECL;
	UInt8 alpha = params->alpha;
	BLIT_STARTLOOP {
		UInt32 pix;
		GetSrcAsDst(s, pix);
		FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, (UInt8)alpha);
	} BLIT_ENDLOOP
#endif /* !(32A16RGB565LE || 16RGB565LE) */
}

/********************************************************************************
 * Bilinear Blend
 ********************************************************************************/

#if (FskName3(fsk,SrcPixelKind,RedPosition) == fsk16RGB565SERedPosition)	&& \
	(FskName3(fsk,DstPixelKind,RedPosition) == fsk16RGB565SERedPosition)	&& \
	(FskName3(fsk,SrcPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits)		&& \
	(FskName3(fsk,DstPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits)

static void BilerpBlendSpanRGB565SE(const UInt16 *src, FskFixed x, FskFixed dx, UInt32 dj, SInt32 srcRB, SInt32 blendLevel, UInt16 *dst, UInt32 width) {
	const UInt32	mGRB	= 0x07E0F81F;													/* -----GGGGGG-----RRRRR------BBBBB */
	UInt32			di, di0, dj0, p00, p01, p10, p11;
	const UInt16	*s;

	blendLevel >>= 2;																		/* Convert from 8 to 6 bits */

	if (dj != 0) {
		dj0 = (1 << 4) - dj;
		for (; width--; x += dx, ++dst) {
			s = src + (x >> FWD_SUBBITS);
			p00 = *s;	p00 |= p00 << 16;	p00 &= mGRB;									/* Get upper left pixel, formatted as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
			p10 = *((UInt16*)(((const char*)s) + srcRB)); p10 |= p10 << 16; p10 &= mGRB;	/* Get lower left pixel, formatted as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
			di = (x >> (FWD_SUBBITS - 4)) & 0xF;
			if (di != 0) {																	/* Interpolate in X and Y */
				p01 = s[1];	p01 |= p01 << 16;	p01 &= mGRB;								/* Get upper right pixel, formatted as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
				p11 = *((UInt16*)(((const char*)s) + srcRB + sizeof(UInt16))); p11 |= p11 << 16; p11 &= mGRB;	/* Get lower right pixel, formatted ... */
				di0 = (1 << 4) - di;
				p00	= (((p00 * di0 + p01 * di) >> 3) & 0x0FE1F83F) * dj0					/* Interpolate R,G,B in parallel */
					+ (((p10 * di0 + p11 * di) >> 3) & 0x0FE1F83F) * dj;
				p00 = ((p00 + 0x02008010) >> 5) & mGRB;
			}
			else {																			/* Interpolate in Y */
				p00  = ((p00 * dj0 + p10 * dj + 0x01004008) >> 4) & mGRB;					/* Interpolate R,G,B in parallel */
			}
			p00 |= (p00 >> 16);																/* Reassemble */
			p10  = *dst;																	/* RRRRRGGGGGGBBBBB */
			p11  = 0x07E0 & p10;															/* -----GGGGGG----- | 6 bit green;        after alphamul:   -----GGGGGGgggggg----- */
			p10 &= 0xF81F;																	/* RRRRR------BBBBB | 5 bit red and blue; after alphamul:	RRRRRrrrrrrBBBBBbbbbbb */
			p10  = (SInt32)((p00 & 0xF81F) - p10) * blendLevel + (p10 << 6) - p10 + 0x10020; p10 += (p10 >> 6) & 0xF81F; p10 = (p10 >> 6) & 0xF81F;
			p11  = (SInt32)((p00 & 0x07E0) - p11) * blendLevel + (p11 << 6) - p11 + 0x00400; p11 += (p11 >> 6) & 0x07E0; p11 = (p11 >> 6) & 0x07E0;

			*dst = (UInt16)(p10 | p11);
		}
	}
	else {
		for (; width--; x += dx, ++dst) {
			s = src + (x >> FWD_SUBBITS);
			p00 = *s;																		/* Get upper left pixel */
			di = (x >> (FWD_SUBBITS - 4)) & 0xF;
			if (di != 0) {																	/* Interpolate in X */
				p00 |= p00 << 16;	p00 &= mGRB;											/* Format upper left pixel as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
				di0 = (1 << 4) - di;
				p01 = s[1];	p01 |= p01 << 16;	p01 &= mGRB;								/* Get upper right pixel, formatted as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
				p00  = ((p00 * di0 + p01 * di + 0x01004008) >> 4) & mGRB;					/* Interpolate R,G,B in parallel */
				p00 |= (p00 >> 16);															/* Reassemble */
			}
			else {																			/* No interpolation necessary */
			}
			p10  = *dst;																	/* RRRRRGGGGGGBBBBB */
			p11  = 0x07E0 & p10;															/* -----GGGGGG----- | 6 bit green;        after alphamul:   -----GGGGGGgggggg----- */
			p10 &= 0xF81F;																	/* RRRRR------BBBBB | 5 bit red and blue; after alphamul:	RRRRRrrrrrrBBBBBbbbbbb */
			p10  = (SInt32)((p00 & 0xF81F) - p10) * blendLevel + (p10 << 6) - p10 + 0x10020; p10 += (p10 >> 6) & 0xF81F; p10 = (p10 >> 6) & 0xF81F;
			p11  = (SInt32)((p00 & 0x07E0) - p11) * blendLevel + (p11 << 6) - p11 + 0x00400; p11 += (p11 >> 6) & 0x07E0; p11 = (p11 >> 6) & 0x07E0;

			*dst = (UInt16)(p10 | p11);
		}
	}
}

#if defined(SUPPORT_WMMX)	&& defined(MARVELL_SOC_PXA168)//
extern UInt16 *FskBilerpAndBlend565SE_new_wMMX_di  (UInt16 *d, const UInt8 *sr, FskFixed x, FskFixed xd, UInt32 width);
extern UInt16 *FskBilerpAndBlend565SE_new_wMMX_didj(UInt16 *d, const UInt8 *sr, UInt32 dy, FskFixed x, FskFixed xd, SInt32 srb, UInt32 width);
extern void FskBilerpAndBlend565SESetImm_new_wMMX_s(UInt8 alpha);

void FskBilinearBlend16RGB565SE16RGB565SE_wmmx(const FskRectBlitParams *params)
{
	UInt32 widthBytes = params->dstWidth * 2, height = params->dstHeight;
	SInt32 srb = params->srcRowBytes, dBump = params->dstRowBytes - widthBytes;
	FskFixed x0 = params->srcX0, y = params->srcY0;
	FskFixed xd = params->srcXInc, yd = params->srcYInc;
	const UInt8 *s0 = (const UInt8*)(params->srcBaseAddr), *sr;
	register UInt16 *d = (UInt16*)params->dstBaseAddr;

	UInt8 alpha = params->alpha>>2;
	height--;
	FskBilerpAndBlend565SESetImm_new_wMMX_s(alpha);
	for ( ; height--; y += yd, d = (UInt16*)(dBump + (char *)d)) {
		UInt32 width = params->dstWidth;
		UInt32 struggles = (UInt32)d & 7;
		FskFixed xt = x0;
		sr = s0 + (y >> 18) * srb;
		if(struggles) {
			struggles = (8 - struggles)>>1;
			BilerpBlendSpanRGB565SE((const UInt16*)sr, xt, xd, yf, srb, alpha<<2, (UInt16*)d, struggles);
			xt = xt + struggles*xd;
			d += struggles;
			width -= struggles;
		}
		struggles = (width & 3);
		width >>= 2;
		if(yf)
			d =	FskBilerpAndBlend565SE_new_wMMX_didj(d, sr, yf, xt, xd, srb, width);
		else
			d = FskBilerpAndBlend565SE_new_wMMX_di(d, sr, xt, xd, width);
		if(struggles) {
			xt += xd * width * 4;
			BilerpBlendSpanRGB565SE((const UInt16*)sr, xt, xd, yf, srb, alpha<<2, (UInt16*)d, struggles);
			d += struggles;
		}
	}
	BilerpBlendSpanRGB565SE((const UInt16*)(s0 + (y >> FWD_SUBBITS) * srb), x0, xd, yf, srb, params->alpha, (UInt16*)d, params->dstWidth);
}
#endif /* SUPPORT_WMMX */
#endif /* 16RGB565LE */


static void
FskName4(FskBilinear,Blend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
#if (FskName3(fsk,SrcPixelKind,RedPosition) == fsk16RGB565SERedPosition)	&& \
	(FskName3(fsk,DstPixelKind,RedPosition) == fsk16RGB565SERedPosition)	&& \
	(FskName3(fsk,SrcPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits)		&& \
	(FskName3(fsk,DstPixelKind,AlphaBits) == fsk16RGB565SEAlphaBits)

	const UInt8	*s0		= (const UInt8*)(params->srcBaseAddr);
	UInt8		*d		= (UInt8*)(params->dstBaseAddr);
	UInt32		height	= params->dstHeight;
	FskFixed	y		= params->srcY0;
	for (; height--; y += params->srcYInc, d += params->dstRowBytes)
		BilerpBlendSpanRGB565SE((const UInt16*)(s0 + (y >> FWD_SUBBITS) * params->srcRowBytes), params->srcX0, params->srcXInc, yf, params->srcRowBytes, params->alpha, (UInt16*)d, params->dstWidth);
#else
	BLIT_DECL;
	UInt8 alpha = params->alpha;
	BLIT_STARTLOOP {
		UInt32 pix;
		BilerpSrcAsDst(s, pix);
		FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, (UInt8)alpha);
	} BLIT_ENDLOOP
#endif
}


#if FskName3(fsk,SrcPixelKind,AlphaBits) > 1


/********************************************************************************
 * Alpha
 ********************************************************************************/
#if	(FskName3(fsk,SrcPixelKind,AlphaBits) == 8) && \
	(FskName3(fsk,SrcPixelKind,RedPosition)   == fsk16RGB565SERedPosition) 	 && \
	(FskName3(fsk,SrcPixelKind,GreenPosition) == fsk16RGB565SEGreenPosition) && \
	(FskName3(fsk,SrcPixelKind,BluePosition)  == fsk16RGB565SEBluePosition)  && \
	(FskName3(fsk,DstPixelKind,RedPosition)   == fsk16RGB565SERedPosition)   && \
	(FskName3(fsk,DstPixelKind,GreenPosition) == fsk16RGB565SEGreenPosition) && \
	(FskName3(fsk,DstPixelKind,BluePosition)  == fsk16RGB565SEBluePosition)  && \
	defined(SUPPORT_WMMX) && \
	defined(MARVELL_SOC_PXA168)
	extern void FskBilinearAlpha32A16RGB565SE16RGB565SE_arm_wmmx_didj(UInt16 *d, const UInt8 *sr, UInt32 dy, FskFixed x, FskFixed xd, SInt32 srb, UInt32 width);
	extern UInt16 *FskAlpha32Blend_arm_wMMX_s(UInt16 *d, const UInt8 *sr, UInt32 dy, FskFixed x, FskFixed xd, SInt32 srb, UInt32 width);
	extern UInt16 *FskAlpha32Blend_arm_wMMX_s_di  (UInt16 *d, const UInt8 *sr, FskFixed x, FskFixed xd, UInt32 width);
	extern void FskAlpha32BlendSetImm_arm_wMMX_s();
void FskBilinearAlpha32A16RGB565SE16RGB565SE_wmmx(const FskRectBlitParams *params)
{
	UInt32 widthBytes = params->dstWidth * 2, height = params->dstHeight;
	SInt32 srb = params->srcRowBytes, dBump = params->dstRowBytes - widthBytes;
	FskFixed x0 = params->srcX0,       y = params->srcY0;
	FskFixed xd = params->srcXInc, yd = params->srcYInc;
	const UInt8 *s0 = (const UInt8*)(params->srcBaseAddr), *sr;
	const UInt32 *s;
	register UInt16*d = (UInt16*)params->dstBaseAddr;
	register FskFixed x;
	register UInt8 *dEnd;
	if (0 == params->isPremul) {
		FskAlpha32BlendSetImm_arm_wMMX_s();
		height--;
		for ( ; height--; y += yd, d = (UInt16*)(dBump + (char *)d)) {
			sr = s0 + (y >> 18) * srb;
			if(yf)
				d = FskAlpha32Blend_arm_wMMX_s(d,sr,yf,x0,xd,srb,params->dstWidth);
			else
				d = FskAlpha32Blend_arm_wMMX_s_di(d, sr, x0, xd, params->dstWidth);
		}
		sr = s0 + (y >> 18) * srb;
		for (x = x0, dEnd = widthBytes + (UInt8 *)d; (UInt8 *)d != dEnd; x += xd, d = (UInt16*)(((char*)d) + 2)) {
			UInt32 pix;
			UInt8 alpha;
			s = (const UInt32*)(sr + (x >> 18) * 4);
			pix = FskBilerp32A16RGB565SE(((x >> (18 - 4)) & 0xF), ((y >> (18 - 4)) & 0xF), s, srb);
			alpha = (UInt8)(pix >> 26);
			if (alpha != 0) {
				if (alpha < 63) {
					UInt32 drb, dg;
					dg = *d;
					drb = 0xF81F & dg;
					dg &= 0x07E0;
					drb = ((pix & 0xF81F) - drb) * (SInt32)alpha + (drb << 6) - drb + 0x10020; drb += (drb >> 6) & 0xF81F; drb = (drb >> 6) & 0xF81F;
					dg = ((pix & 0x07E0) - dg ) * (SInt32)alpha + (dg << 6) - dg + 0x00400; dg += (dg >> 6) & 0x07E0; dg = (dg >> 6) & 0x07E0;
					pix = (drb |= dg);
				}
				*d = (UInt16)pix;
			}
		}
	}
	else {
		for ( ; height--; y += yd, d = (UInt16*)(dBump + (char *)d)) {
			sr = s0 + (y >> 18) * srb;
			for (x = x0, dEnd = widthBytes + (UInt8 *)d; (UInt8 *)d != dEnd; x += xd, d = (UInt16*)(((char*)d) + 2)) {
				s = (const UInt32*)(sr + (x >> 18) * 4);
				UInt32 pix = FskBilerp32A16RGB565SE(((x >> (18 - 4)) & 0xF), ((y >> (18 - 4)) & 0xF), s, srb);
				UInt8 alpha = (UInt8)(pix >> 24);
				alpha >>= 2;
				if (alpha != 0) {
					if ((alpha = 63 - alpha) != 0)
					pix = FskAXPY16RGB565SE(alpha, *d, (UInt16)pix);
					*d = (UInt16)pix;
				}
			}
		}
	}
}
#endif


static void
FskName4(Fsk,Alpha,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;

	/************************
	 *    Straight Alpha    *
	 ************************/
	if (0 == params->isPremul) {
		BLIT_STARTLOOP {
			UInt32	pix	= *s;

			/************************* 32XXXXXXX --> 16RGB4444, straight alpha *************************/
			#if	FskName3(fsk,DstPixelKind,AlphaBits) >= 4
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);							/* Convert to destination pixel format */
				FskName2(FskAlpha,DstPixelKind)(d, (UInt16)pix);								/* ... use our special alpha compositing operator */

			/************************* 32XXXXXXX --> 16NoAlpha *************************/
			#else /* DstAlphaBits < 4 */
				UInt8 alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition));			/* Extract alpha from the source */
				#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
					alpha <<= (8 - FskName3(fsk,SrcPixelKind,AlphaBits));						/* Convert to ... */
					alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);						/* ... 8 bits */
				#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
				alpha >>= 2;																	/* 6 bit alpha. Guaranteed that it doesn't overflow when rounding */

			/************************* 32XXXXX--> 16RGB565LE, straight alpha *************************/
				#if	(FskName3(fsk,DstPixelKind,RedPosition)   == fsk16RGB565SERedPosition)   && \
					(FskName3(fsk,DstPixelKind,GreenPosition) == fsk16RGB565SEGreenPosition) && \
					(FskName3(fsk,DstPixelKind,BluePosition)  == fsk16RGB565SEBluePosition)
					if (alpha != 0) {															/* If the source is not transparent */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);					/* Convert to destination pixel format */
						if (alpha < 63) {														/* If the source is not opaque */
							UInt32 drb, dg;
							dg  = *d;
							drb = 0xF81F & dg;
							dg &= 0x07E0;
							drb = ((pix  & 0xF81F) - drb) * (SInt32)alpha + (drb << 6) - drb + 0x10020; drb += (drb >> 6) & 0xF81F; drb = (drb >> 6) & 0xF81F;
							dg  = ((pix  & 0x07E0) - dg ) * (SInt32)alpha + (dg  << 6) - dg  + 0x00400; dg  += (dg  >> 6) & 0x07E0; dg  = (dg  >> 6) & 0x07E0;
							pix = (drb |= dg);													/* Mix in some of the destination. */
						}
						*d = (UInt16)pix;
					}

			/************************* General case, straight alpha *************************/
				#else /* !16RGB565LE */
					if (0 != alpha) {
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);												/* Convert to destination pixel format */
						if (63 == alpha)	*d = (FskName3(Fsk,DstPixelKind,Type))pix;										/* If alpha is max, write it in directly */
						else				FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, (alpha << 2) | (alpha >> 4));	/* Otherwise convert alpha back to 8 bits and blend */
					}
				#endif /* !16RGB565LE */
			#endif /* DstAlphaBits < 4 -- essentially no alpha */
		} BLIT_ENDLOOP
	}


	/************************
	 * Premultiplied Alpha  *
	 ************************/
	else {
		BLIT_STARTLOOP {
			UInt32	pix	= *s;

			/************************* 32XXXXXXX --> 16RGB4444, premultiplied alpha *************************/
			#if	FskName3(fsk,DstPixelKind,AlphaBits) >= 4
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);							/* Convert to destination pixel format */
				FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, (UInt16)pix);					/* ... use our special alpha compositing operator */

			/************************* 32XXXXXXX --> 16NoAlpha, premultiplied alpha *************************/
			#else /* DstAlphaBIts < 4 */
				UInt8	alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition));		/* Extract alpha from the source */
				#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
					alpha <<= (8 - FskName3(fsk,SrcPixelKind,AlphaBits));						/* Convert to ... */
					alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);						/* ... 8 bits */
				#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
				alpha >>= 2;																	/* 6 bit alpha. Guaranteed that it doesn't overflow when rounding */

			/************************* 32XXXXX --> 16RGB565LE, premultiplied alpha *************************/
				#if	(FskName3(fsk,DstPixelKind,RedPosition)   == fsk16RGB565SERedPosition)   && \
					(FskName3(fsk,DstPixelKind,GreenPosition) == fsk16RGB565SEGreenPosition) && \
					(FskName3(fsk,DstPixelKind,BluePosition)  == fsk16RGB565SEBluePosition)
					if (alpha != 0) {															/* If the source is not transparent */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);					/* Convert it to dst pixel format */
						if ((alpha = 63 - alpha) != 0)											/* If the source is not opaque ... */
							pix = FskAXPY16RGB565SE(alpha, *d, (UInt16)pix);					/* ... blend some of the dst into the src */
						*d = (UInt16)pix;
					}

			/************************* General case *************************/
				#else /* !16RGB565LE */
					if (63 == alpha) {															/* If alpha is max ... */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);					/* Convert to destination pixel format */
						*d = (FskName3(Fsk,DstPixelKind,Type))pix;								/* ... write it in directly */
					}
					else if (0 != alpha) {														/* Otherwise */
						UInt32 dst = *d;
						FskName3(fskConvert,DstPixelKind,32BGRA)(dst);							/* Convert dst to 32BGRA */
						FskName3(fskConvert,SrcPixelKind,32BGRA)(pix);							/* Convert src to 32BGRA */
						pix &= ~3; pix |= alpha >> (6 - 2);										/* Quantize src alpha to 6 bits */
						FskAlphaBlackSourceOver32BGRA(&dst, pix);								/* Do the blending in 32BGRA */
						FskName2(fskConvert32BGRA,DstPixelKind)(dst);							/* Convert 32BGRA to dst pixel format */
						*d = (UInt16)dst;
					}
				#endif /* !16RGB565LE */
			#endif /* DstAlphaBIts < 4 */
		} BLIT_ENDLOOP
	}
}


/********************************************************************************
 * Bilinear Alpha
 ********************************************************************************/

static void
FskName4(FskBilinear,Alpha,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	BLIT_DECL;

	/************************
	 *    Straight Alpha    *
	 ************************/
	if (0 == params->isPremul) {
		BLIT_STARTLOOP {
			UInt32 pix	= FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);

			/************************* 32XXXXXXX --> 16RGB4444, straight alpha *************************/
			#if	FskName3(fsk,DstPixelKind,AlphaBits) >= 4
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);							/* Convert to destination pixel format */
				FskName2(FskAlpha,DstPixelKind)(d, (UInt16)pix);								/* ... use our special alpha compositing operator */

			/************************* 32XXXXXXX --> 16NoAlpha, straight alpha *************************/
			#else /* DstAlphaBits < 4 */
				UInt8	alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition));		/* Extract alpha from the source */
				#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
					alpha <<= (8 - FskName3(fsk,SrcPixelKind,AlphaBits));						/* Convert alpha to ... */
					alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);						/* ... 8 bits */
				#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
				alpha >>= 2;																	/* 6 bit alpha. Guaranteed that it doesn't overflow when rounding */

			/************************* 32A16RGB565LE --> 16RGB565LE, straight alpha *************************/
				#if	(FskName3(fsk,DstPixelKind,RedPosition)   == fsk16RGB565SERedPosition)   && \
					(FskName3(fsk,DstPixelKind,GreenPosition) == fsk16RGB565SEGreenPosition) && \
					(FskName3(fsk,DstPixelKind,BluePosition)  == fsk16RGB565SEBluePosition)
					if (alpha != 0) {															/* If the source is not transparent */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);					/* Convert to destination pixel format */
						if (alpha < 63) {														/* If the source is not opaque */
							UInt32 drb, dg;
							dg  = *d;
							drb = 0xF81F & dg;
							dg &= 0x07E0;
							drb = ((pix  & 0xF81F) - drb) * (SInt32)alpha + (drb << 6) - drb + 0x10020; drb += (drb >> 6) & 0xF81F; drb = (drb >> 6) & 0xF81F;
							dg  = ((pix  & 0x07E0) - dg ) * (SInt32)alpha + (dg  << 6) - dg  + 0x00400; dg  += (dg  >> 6) & 0x07E0; dg  = (dg  >> 6) & 0x07E0;
							pix = (drb |= dg);													/* Mix in some of the destination. */
						}
						*d = (UInt16)pix;
					}

			/************************* General case, straight alpha *************************/
				#else /* !16RGB565LE */
					if (0 != alpha) {
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);												/* Convert to destination pixel format */
						if (63 == alpha)	*d = (FskName3(Fsk,DstPixelKind,Type))pix; 										/* If alpha is max, write it in directly */
						else				FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, (alpha << 2) | (alpha >> 4));	/* Convert alpha back to 8 bits and blend */
					}
				#endif /* !16RGB565LE */
			#endif /* DstAlphaBits < 4 */
		} BLIT_ENDLOOP
	}


	/************************
	 * Premultiplied Alpha  *
	 ************************/
	else {
		BLIT_STARTLOOP {
			UInt32 pix	= FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);

			/************************* 32XXXXXXX --> 16RGB4444 *************************/
			#if	FskName3(fsk,DstPixelKind,AlphaBits) >= 4
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);							/* Convert to destination pixel format */
				FskName2(FskAlpha,DstPixelKind)(d, (UInt16)pix);								/* ... use our special alpha compositing operator */

			/************************* 32XXXXXXX --> 16NoAlpha *************************/
			#else /* DstAlphaBits < 4 */
				UInt8 alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition));		/* Extract alpha from the source */
				#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
					alpha <<= (8 - FskName3(fsk,SrcPixelKind,AlphaBits));						/* Convert to ... */
					alpha |= alpha >> FskName3(fsk,SrcPixelKind,AlphaBits);						/* ... 8 bits */
				#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
				alpha >>= 2;																	/* 6 bit alpha. Guaranteed that it doesn't overflow when rounding */

			/************************* 32XXXXXX --> 16RGB565SE *************************/
				#if	(FskName3(fsk,DstPixelKind,RedPosition)   == fsk16RGB565SERedPosition)   && \
					(FskName3(fsk,DstPixelKind,GreenPosition) == fsk16RGB565SEGreenPosition) && \
					(FskName3(fsk,DstPixelKind,BluePosition)  == fsk16RGB565SEBluePosition)
					if (alpha != 0) {															/* If the source is not transparent */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);					/* Convert to destination pixel format */
						if ((alpha = 63 - alpha) != 0)											/* If the source is not opaque ... */
							pix = FskAXPY16RGB565SE(alpha, *d, (UInt16)pix);					/* ... blend some of the dst into the sr */
						*d = (UInt16)pix;
					}

			/************************* General case *************************/
				#else /* !16RGB565SE */
					if (63 == alpha) {															/* If alpha is max ... */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);					/* Convert to destination pixel format */
						*d = (FskName3(Fsk,DstPixelKind,Type))pix;								/* ... write it in directly */
					}
					else if (0 != alpha) {														/* Otherwise */
						UInt32 dst = *d;
						FskName3(fskConvert,DstPixelKind,32BGRA)(dst);							/* Convert dst to 32BGRA */
						FskName3(fskConvert,SrcPixelKind,32BGRA)(pix);							/* Convert src to 32BGRA */
						pix &= ~3; pix |= alpha >> (6 - 2);										/* Quantize src alpha to 6 bits */
						FskAlphaBlackSourceOver32BGRA(&dst, pix);								/* Do the blending in 32BGRA */
						FskName2(fskConvert32BGRA,DstPixelKind)(dst);							/* Convert 32BGRA to dst pixel format */
						*d = (UInt16)dst;
					}
				#endif /* !16RGB565SE */
			#endif /* DstAlphaBits < 4 */
		} BLIT_ENDLOOP
	}
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
			#if	FskName3(fsk,DstPixelKind,AlphaBits) >= 4													/* If we have enough alpha bits in the dst */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);										/* Convert to destination pixel format */
				FskName2(FskAlphaBlend,DstPixelKind)(d, (UInt16)pix, (UInt8)scale);							/* Use our special alpha blend operator */
			#else /* DstAlphaBits < 4 */																	/* Otherwise (dst does not have enough alpha bits) ... */
			{ 	UInt32 alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract alpha from the source */
				if (alpha != 0) {
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);									/* Convert to destination pixel format */
					if (alpha == ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1)) {						/* If alpha is max ... */
						FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, (UInt8)scale);						/* Blend using the scale */
					}
					else {
						#if FskName3(fsk,SrcPixelKind,AlphaBits) == 4										/* If alpha was 4 bits ... */
							alpha |= alpha << 4;															/* ... convert to 8 bits */
						#elif FskName3(fsk,SrcPixelKind,AlphaBits) < 8										/* If alpha is not 4 or 8 bits ... */
							alpha = alpha * 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);		/* ... convert to 8 bits */
						#endif /* SrcAlphaBits < 8 */
						alpha *= scale; alpha += 0x80; alpha += alpha >> 8; alpha >>= 8;					/* Multiply alpha and scale */
						if (alpha > 0)	FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, (UInt8)alpha);		/* Blend */
					}
				}
			}
			#endif /* DstAlphaBits < 4 */
		} BLIT_ENDLOOP
	}
	else {
		BLIT_STARTLOOP {
			#if	FskName3(fsk,SrcPixelKind,RedBits) == 8													/* If we have enough alpha bits in the src */
				UInt32 dst = *d;
				UInt32 src = *s;
				FskName3(fskConvert,DstPixelKind,SrcPixelKind)(dst);
				FskName2(FskAlphaBlackSourceOver,SrcPixelKind)(&dst, FskName2(FskAlphaScale,SrcPixelKind)((UInt8)scale, src));
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(dst);
				*d = (UInt16)dst;
			#else /* small src */
				*d = 0xA5A5;	/* TODO: Implement for 32A16RGB565LE and 16RGBA4444LE */
			#endif /* SrcPixelKind */
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
			UInt32 pix		= FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);
			#if	FskName3(fsk,DstPixelKind,AlphaBits) >= 4													/* If we have enough alpha bits in the dst */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);										/* Convert to destination pixel format */
				FskName2(FskAlphaBlend,DstPixelKind)(d, (UInt16)pix, (UInt8)scale);							/* Use our special alpha blend operator */
			#else /* DstAlphaBits < 4 */																	/* Otherwise (dst does not have enough alpha bits) ... */
			{ 	UInt32 alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract alpha from the source */
				if (alpha != 0) {
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);									/* Convert to destination pixel format */
					if (alpha == ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1)) {						/* If alpha is max ... */
						FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, (UInt8)scale);						/* Blend using the scale */
					}
					else {
						#if FskName3(fsk,SrcPixelKind,AlphaBits) == 4										/* If alpha was 4 bits ... */
							alpha |= alpha << 4;															/* ... convert to 8 bits */
						#elif FskName3(fsk,SrcPixelKind,AlphaBits) < 8										/* If alpha is not 4 or 8 bits ... */
							alpha = alpha * 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);		/* ... convert to 8 bits */
						#endif /* SrcAlphaBits < 8 */
						alpha *= scale; alpha += 0x80; alpha += alpha >> 8; alpha >>= 8;					/* Multiply alpha and scale */
						if (alpha > 0)	FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, (UInt8)alpha);		/* Blend */
					}
				}
			}
			#endif /* DstAlphaBits < 4 */
		} BLIT_ENDLOOP
	}
	else {
		BLIT_STARTLOOP {
			#if	FskName3(fsk,SrcPixelKind,RedBits) == 8													/* If we have enough alpha bits in the src */
				UInt32 dst = *d;
				UInt32 src = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);
				FskName3(fskConvert,DstPixelKind,SrcPixelKind)(dst);
				FskName2(FskAlphaBlackSourceOver,SrcPixelKind)(&dst, FskName2(FskAlphaScale,SrcPixelKind)((UInt8)scale, src));
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(dst);
				*d = (UInt16)dst;
			#else /* small src */
				*d = 0xA5A5;	/* TODO: Implement for 32A16RGB565LE and 16RGBA4444LE */
			#endif /* SrcPixelKind */
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

	FskName2(fskConvert24RGB,DstPixelKind)(params->red, tint);

	#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
		if (params->alpha == 255) {
			BLIT_STARTLOOP {
				GetSrcAsDst(s, pix);
				*d = FskName2(FskPixelMul,DstPixelKind)((UInt16)pix, (UInt16)tint);
			} BLIT_ENDLOOP
		}
		else {
			BLIT_STARTLOOP {
				GetSrcAsDst(s, pix);
				pix = FskName2(FskPixelMul,DstPixelKind)((UInt16)pix, (UInt16)tint);
				FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, params->alpha);
			} BLIT_ENDLOOP
		}
	#else /* FskName3(fsk,SrcPixelKind,AlphaBits) == 8 */
		BLIT_STARTLOOP {
			UInt32 alpha;
			pix = *s;
			alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract alpha from the source */
			alpha *= params->alpha; alpha += 0x80; alpha += alpha >> 8; alpha >>= 8;					/* Multiply alpha and scale */
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);
			pix = FskName2(FskPixelMul,DstPixelKind)((UInt16)pix, (UInt16)tint);
			FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, (UInt8)alpha);
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
	UInt32 tint, pix;

	FskName2(fskConvert24RGB,DstPixelKind)(params->red, tint);

	#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
		if (params->alpha == 255) {
			BLIT_STARTLOOP {
				BilerpSrcAsDst(s, pix);
				*d = FskName2(FskPixelMul,DstPixelKind)((UInt16)pix, (UInt16)tint);
			} BLIT_ENDLOOP
		}
		else {
			BLIT_STARTLOOP {
				BilerpSrcAsDst(s, pix);
				pix = FskName2(FskPixelMul,DstPixelKind)((UInt16)pix, (UInt16)tint);
				FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, params->alpha);
			} BLIT_ENDLOOP
		}
	#else /* FskName3(fsk,SrcPixelKind,AlphaBits) == 8 */
		BLIT_STARTLOOP {
			UInt32 alpha;
			pix = FskName2(FskBilerp,SrcPixelKind)(xf, yf, s, srb);
			alpha = (UInt8)(pix >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract alpha from the source */
			alpha *= params->alpha; alpha += 0x80; alpha += alpha >> 8; alpha >>= 8;					/* Multiply alpha and scale */
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix);
			pix = FskName2(FskPixelMul,DstPixelKind)((UInt16)pix, (UInt16)tint);
			FskName2(FskBlend,DstPixelKind)(d, (UInt16)pix, (UInt8)alpha);
		} BLIT_ENDLOOP
	#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) == 8 */
}


/********************************************************************************/


#undef GetSrcAsDst
#undef BilerpSrcAsDst


#endif /* FskName2(SRC_,FskName3(fsk,SrcPixelKind,KindFormat)) && FskName2(DST_,FskName3(fsk,DstPixelKind,KindFormat)) */


#undef SrcPixelKind
#undef DstPixelKind
