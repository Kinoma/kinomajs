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
#if		(FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)	\
	&&	(FskName3(fsk,DstPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)


#define SrcPixType	FskName3(Fsk,SrcPixelKind,Type)
#define SrcPixBytes	FskName3(fsk,SrcPixelKind,Bytes)
#define DstPixType	FskName3(Fsk,DstPixelKind,Type)
#define DstPixBytes	FskName3(fsk,DstPixelKind,Bytes)

#if   (DstPixBytes == 1) || (DstPixBytes == 2) || (DstPixBytes == 4)		/* 8, 16 or 32 bit destination */
	#if   (SrcPixBytes == 1) || (SrcPixBytes == 2) || (SrcPixBytes == 4)		/* 8, 16 or 32 bit source */
		#define GetSrcAsDst(s, d)						d = *(s);												FskName3(fskConvert,SrcPixelKind,DstPixelKind)(d)
		#define BilerpSrcAsDst(uf, vf, s, rb, d)		d = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, rb);	FskName3(fskConvert,SrcPixelKind,DstPixelKind)(d)
	#elif SrcPixBytes == 3														/* 24 bit source */
		#define GetSrcAsDst(s, d)		/* */																	FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*(s), d)
		#define BilerpSrcAsDst(uf, vf, s, rb, d)		pix24 = FskBilerp24(uf, vf, s, rb);						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix24, d)
	#else
		#error Unsupported pixel kind SrcPixelKind
	#endif
#elif DstPixBytes == 3														/* 24 bit dstination */
	#if   (SrcPixBytes == 1) || (SrcPixBytes == 2) || (SrcPixBytes == 4)		/* 8, 16 or 32 bit source */
		#define GetSrcAsDst(s, d)						pix = *(s);												FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix, d)
		#define BilerpSrcAsDst(uf, vf, s, rb, d)		pix = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, rb);	FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix, d)
	#elif SrcPixBytes == 3														/* 24 bit source */
		#define GetSrcAsDst(s, d)			/* */																FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*(s), d)
		#if FskName3(fsk,SrcPixelKind,RedPosition) == FskName3(fsk,DstPixelKind,RedPosition)
			#define BilerpSrcAsDst(uf, vf, s, rb, d)	d = FskBilerp24(uf, vf, s, rb)							/* */
		#else /* FskName3(fsk,SrcPixelKind,RedPosition) != FskName3(fsk,DstPixelKind,RedPosition) */
			#define BilerpSrcAsDst(uf, vf, s, rb, d)	FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, rb, pix24);	FskName3(fskConvert,SrcPixelKind,DstPixelKind)(pix24, d)
		#endif /* FskName3(fsk,SrcPixelKind,RedPosition) != FskName3(fsk,DstPixelKind,RedPosition) */
	#else
		#error Unsupported pixel kind SrcPixelKind
	#endif
#else
	#error Unsupported pixel kind DstPixelKind
#endif

#define kInnnerLoopCount (16)
#define kInnnerLoopShift (4)

/********************************************************************************
 * Rotate90 Q0
 ********************************************************************************/

static void
FskName4(FskRotate90P,SrcPixelKind,DstPixelKind,Q0)(
	FskFixed	u0,
	FskFixed	v0,
	FskFixed	du,
	FskFixed 	dv,
	const void	*srcBaseAddr,
	SInt32		srcRowBytes,
	void		*dstBaseAddr,
	SInt32		dstRowBytes,
	UInt32		dstWidth,
	UInt32		dstHeight
)
{
	DstPixType *d = (DstPixType*)dstBaseAddr;
	SInt32 w;
	FskFixed v;
	SInt32 dstBump = DstPixBytes - dstHeight * dstRowBytes;
#if 0
	// cache-unaware
	for (w = dstWidth, v = v0; w--; v += dv, d = (DstPixType*)((char*)d + dstBump)) {
		const char *srcCol  = (const char*)(srcBaseAddr) + (v >> FWD_BITS) * srcRowBytes;
		FskFixed u;
		UInt32 h;
		for (h = dstHeight, u = u0; h--; u += du, d = (DstPixType*)((char*)d + dstRowBytes)) {
			#if   (SrcPixBytes == 1)
				const SrcPixType *s = (const SrcPixType*)(srcCol + (u >> FWD_BITS));
			#elif   (SrcPixBytes == 2)
				const SrcPixType *s = (const SrcPixType*)(srcCol + ((u >> FWD_BITS) << 1));
			#elif   (SrcPixBytes == 4)
				const SrcPixType *s = (const SrcPixType*)(srcCol + ((u >> FWD_BITS) << 2));
			#else
				const SrcPixType *s = (const SrcPixType*)(srcCol + (u >> FWD_BITS) * SrcPixBytes);
			#endif

			#if (DstPixBytes != 3) || (SrcPixBytes != 3)
				register UInt32 pix;
			#endif
			#if DstPixBytes != 3
				GetSrcAsDst(s, pix);
				*d = (DstPixType)pix;
			#else					/* 3 byte pixels */
				GetSrcAsDst(s, *d);
			#endif					/* 3 byte pixels */
		}
	}
#else
	// cache-aware
	SInt32 srb;

	if (((1 << FWD_BITS) == dv) || (-(1 << FWD_BITS) == dv))
		srb = (dv > 0) ? srcRowBytes : -srcRowBytes;			// 1x scaling in source height - can eliminate one multiply per pixel
	else
		srb = 0;
	dstBump += (DstPixBytes * (kInnnerLoopCount - 1));

	for (w = dstWidth, v = v0; w > 0; v += (dv << kInnnerLoopShift), d = (DstPixType*)((char*)d + dstBump), w -= kInnnerLoopCount) {
		FskFixed u;
		UInt32 h;
		for (h = dstHeight, u = u0; h--; u += du, d = (DstPixType*)((char*)d + dstRowBytes)) {
			SInt32 j;
			DstPixType *dd = d;
			#if   (SrcPixBytes == 1)
				const char *sc = (const char*)(srcBaseAddr) + ((u >> FWD_BITS) << 0);
			#elif   (SrcPixBytes == 2)
				const char *sc = (const char*)(srcBaseAddr) + ((u >> FWD_BITS) << 1);
			#elif   (SrcPixBytes == 4)
				const char *sc = (const char*)(srcBaseAddr) + ((u >> FWD_BITS) << 2);
			#else
				const char *sc = (const char*)(srcBaseAddr) + ((u >> FWD_BITS) * SrcPixBytes);
			#endif

			if (srb) {
				const SrcPixType *s = (const SrcPixType*)(sc + (v >> FWD_BITS) * srcRowBytes);
				for (j = (w >= kInnnerLoopCount) ? (kInnnerLoopCount - 1) : (w - 1); j >= 0; j--, dd = (DstPixType *)((char *)dd + DstPixBytes), s = (const SrcPixType *)(srb + (char *)s)) {
					#if (DstPixBytes != 3) || (SrcPixBytes != 3)
						register UInt32 pix;
					#endif
					#if DstPixBytes != 3
						GetSrcAsDst(s, pix);
						*dd = (DstPixType)pix;
					#else					/* 3 byte pixels */
						GetSrcAsDst(s, *dd);
					#endif					/* 3 byte pixels */
				}
			}
			else {
				FskFixed vv = v;
				for (j = (w >= kInnnerLoopCount) ? (kInnnerLoopCount - 1) : (w - 1); j >=0; j--, dd = (DstPixType *)((char *)dd + DstPixBytes), vv += dv) {
					const SrcPixType *s = (const SrcPixType*)(sc + (vv >> FWD_BITS) * srcRowBytes);

					#if (DstPixBytes != 3) || (SrcPixBytes != 3)
						register UInt32 pix;
					#endif
					#if DstPixBytes != 3
						GetSrcAsDst(s, pix);
						*dd = (DstPixType)pix;
					#else					/* 3 byte pixels */
						GetSrcAsDst(s, *dd);
					#endif					/* 3 byte pixels */
				}
			}
		}
	}
#endif
}


/********************************************************************************
 * Rotate90 Q1
 ********************************************************************************/

static void
FskName4(FskRotate90P,SrcPixelKind,DstPixelKind,Q1)(
	FskFixed	u0,
	FskFixed	v0,
	FskFixed	du,
	FskFixed 	dv,
	const void	*srcBaseAddr,
	SInt32		srcRowBytes,
	void		*dstBaseAddr,
	SInt32		dstRowBytes,
	UInt32		dstWidth,
	UInt32		dstHeight
)
{
	DstPixType *d = (DstPixType*)dstBaseAddr;
	SInt32 w;
	FskFixed v;
	SInt32 dstBump = DstPixBytes - dstHeight * dstRowBytes;

#if 0
	for (w = dstWidth, v = v0; w--; v += dv, d = (DstPixType*)((char*)d + dstBump)) {
		const char *srcCol  = (const char*)(srcBaseAddr) + (v >> FWD_BITS) * srcRowBytes;
		SInt32 vf = (v >> (FWD_BITS - 4)) & 0xF;	/* 4 fractional bits */
		FskFixed u;
		UInt32 h;
		for (h = dstHeight, u = u0; h--; u += du, d = (DstPixType*)((char*)d + dstRowBytes)) {
			#if   (SrcPixBytes == 1)
				const SrcPixType *s = (const SrcPixType*)(srcCol + ((u >> FWD_BITS) << 0));
			#elif   (SrcPixBytes == 2)
				const SrcPixType *s = (const SrcPixType*)(srcCol + ((u >> FWD_BITS) << 1));
			#elif   (SrcPixBytes == 4)
				const SrcPixType *s = (const SrcPixType*)(srcCol + ((u >> FWD_BITS) << 2));
			#else
				const SrcPixType *s = (const SrcPixType*)(srcCol + (u >> FWD_BITS) * SrcPixBytes);
			#endif
			SInt32 uf = (u >> (FWD_BITS - 4)) & 0xF;	/* 4 fractional bits */
			#if (SrcPixBytes != 3) || (DstPixBytes != 3)
				register UInt32		pix;
			#endif
			#if		((SrcPixBytes == 3) && (DstPixBytes != 3))	\
				||	((SrcPixBytes == 3) && (DstPixBytes == 3) && FskName3(fsk,SrcPixelKind,RedPosition) != FskName3(fsk,DstPixelKind,RedPosition))
				Fsk24BitType		pix24;
			#endif
			#if DstPixBytes != 3
				BilerpSrcAsDst(uf, vf, s, srcRowBytes, pix);
				*d = (DstPixType)pix;
			#else					/* 3 byte pixels */
				BilerpSrcAsDst(uf, vf, s, srcRowBytes, *d);
			#endif					/* 3 byte pixels */
		}
	}
#else
	dstBump += (DstPixBytes * (kInnnerLoopCount - 1));

	for (w = dstWidth, v = v0; w > 0; v += (dv << kInnnerLoopShift), d = (DstPixType*)((char*)d + dstBump), w -= kInnnerLoopCount) {
		FskFixed u;
		UInt32 h;
		for (h = dstHeight, u = u0; h--; u += du, d = (DstPixType*)((char*)d + dstRowBytes)) {
			SInt32 j;
			DstPixType *dd = d;
			#if   (SrcPixBytes == 1)
				const char *sc = (const char*)(srcBaseAddr) + (u >> FWD_BITS);
			#elif   (SrcPixBytes == 2)
				const char *sc = (const char*)(srcBaseAddr) + ((u >> FWD_BITS) << 1);
			#elif   (SrcPixBytes == 4)
				const char *sc = (const char*)(srcBaseAddr) + ((u >> FWD_BITS) << 2);
			#else
				const char *sc = (const char*)(srcBaseAddr) + ((u >> FWD_BITS) * SrcPixBytes);
			#endif
			FskFixed vv = v;
			SInt32 uf = (u >> (FWD_BITS - 4)) & 0xF;	/* 4 fractional bits */

			for (j = (w >= kInnnerLoopCount) ? (kInnnerLoopCount - 1) : (w - 1); j >=0; j--, dd = (DstPixType *)((char *)dd + DstPixBytes), vv += dv) {
				const SrcPixType *s = (const SrcPixType*)(sc + (vv >> FWD_BITS) * srcRowBytes);
				SInt32 vf = (vv >> (FWD_BITS - 4)) & 0xF;	/* 4 fractional bits */
				#if (SrcPixBytes != 3) || (DstPixBytes != 3)
					register UInt32		pix;
				#endif
				#if		((SrcPixBytes == 3) && (DstPixBytes != 3))	\
					||	((SrcPixBytes == 3) && (DstPixBytes == 3) && FskName3(fsk,SrcPixelKind,RedPosition) != FskName3(fsk,DstPixelKind,RedPosition))
					Fsk24BitType		pix24;
				#endif
				#if DstPixBytes != 3
					BilerpSrcAsDst(uf, vf, s, srcRowBytes, pix);
					*dd = (DstPixType)pix;
				#else					/* 3 byte pixels */
					BilerpSrcAsDst(uf, vf, s, srcRowBytes, *dd);
				#endif					/* 3 byte pixels */
			}
		}
	}
#endif
}


#undef GetSrcAsDst
#undef BilerpSrcAsDst
#undef SrcPixType
#undef DstPixType
#undef SrcPixBytes
#undef DstPixBytes

#elif defined(COMPILER_CAN_GENERATE_CONST_PROC_POINTERS) && COMPILER_CAN_GENERATE_CONST_PROC_POINTERS

	static const FskRotate90Proc FskName4(FskRotate90P,SrcPixelKind,DstPixelKind,Q0) = NULL;
	static const FskRotate90Proc FskName4(FskRotate90P,SrcPixelKind,DstPixelKind,Q1) = NULL;

#endif /* fskUniformChunkyPixelPacking */


#undef SrcPixelKind
#undef DstPixelKind
