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
/* We offer the opportunity to define procs with their pixel kind or their pixel format. */
#if !defined(SrcPixelKind) && defined(SrcPixelFmt)
	#define SrcPixelKind FskName3(fsk,SrcPixelFmt,FormatKind)
#endif /* SrcPixelKind */
#if !defined(DstPixelKind) && defined(DstPixelFmt)
	#define DstPixelKind FskName3(fsk,DstPixelFmt,FormatKind)
#endif /* DstPixelKind */


/*******************************************************************************
 * TexFillSpanQ0
 *******************************************************************************/

static void FskName4(TexFillSpan,SrcPixelKind,DstPixelKind,Q0)(FskSpan *span) {
	TextureSpanData	*td = (TextureSpanData*)(span->spanData);
	FixedFwdDiff	u	= td->u;	/* Coordinate are pre-rounded by 1 << (kTexCoordBits - 1) */
	FixedFwdDiff	v	= td->v;
	int				ui, vi;
	FskName3(Fsk,DstPixelKind,Type)	*d = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);

	#if FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking
		const FskName3(Fsk,SrcPixelKind,Type) *s;
	#elif FskName3(fsk,SrcPixelKind,PixelPacking) == fskPlanarPixelPacking
		int cbOffset = td->texRowBytes * td->texHeight;	/* Assume that this is divisible by 4 */
		int crOffset = cbOffset >> 2;
	#endif /* YUV420 */

	if (span->setPixel == NULL) {	/* We can write directly into the frame buffer */
		for (; span->dx--; u += td->du, v += td->dv, d = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes))) {
			#if FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking
				ui = u >> kTexCoordBits;
				vi = v >> kTexCoordBits;
				s  = (const FskName3(Fsk,SrcPixelKind,Type)*)((const char*)(td->texPixels) + vi * td->texRowBytes + ui * FskName3(fsk,SrcPixelKind,Bytes));
				#if (FskName3(fsk,SrcPixelKind,Bytes) == 3) && (FskName3(fsk,DstPixelKind,Bytes) == 3)		/* 24 -> 24 */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, *d);
				#elif (FskName3(fsk,SrcPixelKind,Bytes) == 3)												/* 24 -> !24 */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, ui);
					*d = (FskName3(Fsk,DstPixelKind,Type))ui;
				#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* !24 -> 24 */
					ui = (int)(UInt32)(*s);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(ui, *d);
				#else																						/* !24 -> !24 */
					ui = (int)(UInt32)(*s);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(ui);
					*d = (FskName3(Fsk,DstPixelKind,Type))ui;
				#endif

			#elif FskName3(fsk,SrcPixelKind,PixelPacking) == fskPlanarPixelPacking
				UInt8 y, cb, cr;
				const UInt8 *s;
				ui = u >> kTexCoordBits;				/* Luma coordinates */
				vi = v >> kTexCoordBits;
				s  = (const UInt8*)(td->texPixels) + vi * td->texRowBytes + ui;
				y  = *s;
				ui = (u + (1 << (kTexCoordBits - 0)) - (1 << (kTexCoordBits - 1))) >> (kTexCoordBits + 1);		/* Chroma coordinates, re-rounded appropriate for its sampling */
				vi = (v + (1 << (kTexCoordBits - 0)) - (1 << (kTexCoordBits - 1))) >> (kTexCoordBits + 1);
				s  = (const UInt8*)(td->texPixels) + cbOffset + vi * td->texRowBytes + ui;
				cb = s[0];
				cr = s[crOffset];
				*d = FskName2(FskConvertYUV420,DstPixelKind)(y, cb, cr);

			#else
				#error Unsupported pixel packing
			#endif
		}
		span->p = d;				/* Update span pixel pointer */
	}
	else {
		FskPixelType saveColor = span->fillColor;
		for (; span->dx--; u += td->du, v += td->dv, span->p = d = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes))) {
			#if FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking
				ui = u >> kTexCoordBits;
				vi = v >> kTexCoordBits;
				s  = (const FskName3(Fsk,SrcPixelKind,Type)*)((const char*)(td->texPixels) + vi * td->texRowBytes + ui * FskName3(fsk,SrcPixelKind,Bytes));
				#if (FskName3(fsk,SrcPixelKind,Bytes) == 3) && (FskName3(fsk,DstPixelKind,Bytes) == 3)		/* 24 -> 24 */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, span->fillColor.p24);
				#elif (FskName3(fsk,SrcPixelKind,Bytes) == 3)												/* 24 -> !24 */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, ui);
					*((FskName3(Fsk,DstPixelKind,Type)*)(&span->fillColor)) = (FskName3(Fsk,DstPixelKind,Type))ui;
				#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* !24 -> 24 */
					ui = (int)(UInt32)(*s);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(ui, span->fillColor.p24);
				#else																						/* !24 -> !24 */
					ui = (int)(UInt32)(*s);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(ui);
					*((FskName3(Fsk,DstPixelKind,Type)*)(&span->fillColor)) = (FskName3(Fsk,DstPixelKind,Type))ui;
				#endif

			#elif FskName3(fsk,SrcPixelKind,PixelPacking) == fskPlanarPixelPacking
				UInt8 y, cb, cr;
				const UInt8 *s;
				ui = (u + (1 << (kTexCoordBits - 1))) >> kTexCoordBits;										/* Round luma coordinates */
				vi = (v + (1 << (kTexCoordBits - 1))) >> kTexCoordBits;
				s  = (const UInt8*)(td->texPixels) + vi * td->texRowBytes + ui;
				y  = *s;																					/* Fetch Y */
				ui = (u + (1 << (kTexCoordBits - 0))) >> (kTexCoordBits + 1);								/* Round chroma coordinates */
				vi = (v + (1 << (kTexCoordBits - 0))) >> (kTexCoordBits + 1);
				s  = (const UInt8*)(td->texPixels) + cbOffset + vi * td->texRowBytes + ui;
				cb = s[0];																					/* Fetch Cb */
				cr = s[crOffset];																			/* Fetch Cr */
				*((FskName3(Fsk,DstPixelKind,Type)*)(&span->fillColor)) = FskName2(FskConvertYUV420,DstPixelKind)(y, cb, cr);

			#else
				#error Unsupported pixel packing
			#endif
			span->setPixel(span);
		}
		span->fillColor = saveColor;
	}
}


/*******************************************************************************
 * TexFillSpanQ1
 *******************************************************************************/

static void FskName4(TexFillSpan,SrcPixelKind,DstPixelKind,Q1)(FskSpan *span) {
	TextureSpanData	*td = (TextureSpanData*)(span->spanData);
	FixedFwdDiff	u	= td->u;	/* Coordinate are pre-rounded by 1 << (kTexCoordBits - 1) */
	FixedFwdDiff	v	= td->v;
	int				ui, vi, uf, vf;
	FskName3(Fsk,DstPixelKind,Type)	*d = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);

	#if FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking
		const FskName3(Fsk,SrcPixelKind,Type) *s;
	#elif FskName3(fsk,SrcPixelKind,PixelPacking) == fskPlanarPixelPacking
		int cbOffset = td->texRowBytes * td->texHeight;	/* Assume that this is divisible by 4 */
		int crOffset = cbOffset >> 2;
		int chrRowBytes = td->texRowBytes >> 1;
		FixedFwdDiff uLerpMax = (td->texWidth  << kLerpBits) - 1;
		FixedFwdDiff vLerpMax = (td->texHeight << kLerpBits) - 1;
	#endif /* YUV420 */

	if (span->setPixel == NULL) {	/* We can write directly into the frame buffer */
		for (; span->dx--; u += td->du, v += td->dv, d = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes))) {
			#if FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking
				ui = u >> kTexCoordBits;	uf = (u >> (kTexCoordBits - kLerpBits)) - (ui << kLerpBits);
				vi = v >> kTexCoordBits;	vf = (v >> (kTexCoordBits - kLerpBits)) - (vi << kLerpBits);
				s  = (const FskName3(Fsk,SrcPixelKind,Type)*)((const char*)(td->texPixels) + vi * td->texRowBytes + ui * FskName3(fsk,SrcPixelKind,Bytes));

				#if (FskName3(fsk,SrcPixelKind,Bytes) == 3)													/* 24 -> xx */
				{	Fsk24BitType px = FskBilerp24(uf, vf, s, td->texRowBytes);
					#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 24 -> 24 */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, *d);
					#else																					/* 24 -> !24 */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, ui);
						*d = (FskName3(Fsk,DstPixelKind,Type))ui;
					#endif
				}
				#elif (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,DstPixelKind,Bytes) > 2) && (FskName3(fsk,SrcPixelKind,GreenBits) != 5)	/* 16 (!5515) -> 24 */
					ui = FskName3(FskBilerp,SrcPixelKind,32ARGB)(uf, vf, s, td->texRowBytes);
					#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 16 (!5515) -> 24 */
						FskName3(fskConvert,32ARGB,DstPixelKind)(ui, *d);
					#else																					/* 16 (!5515) -> !24 */
						FskName3(fskConvert,32ARGB,DstPixelKind)(ui);
						*d = (FskName3(Fsk,DstPixelKind,Type))ui;
					#endif
				#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* xx --> 24 */
					ui = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, td->texRowBytes);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(ui, *d);
				#else																						/* !24, 5515, 32 -> !24 */
					ui = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, td->texRowBytes);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(ui);
					*d = (FskName3(Fsk,DstPixelKind,Type))ui;
				#endif

			#elif FskName3(fsk,SrcPixelKind,PixelPacking) == fskPlanarPixelPacking
				UInt8 y, cb, cr;
				const UInt8 *s;
				ui = u >> kTexCoordBits;	uf = (u >> (kTexCoordBits - kLerpBits)) - (ui << kLerpBits);				/* Luma coordinates */
				vi = v >> kTexCoordBits;	vf = (v >> (kTexCoordBits - kLerpBits)) - (vi << kLerpBits);
				s  = (const UInt8*)(td->texPixels) + vi * td->texRowBytes + ui;
				y  = FskBilerp8(uf, vf, s, td->texRowBytes);

				uf = (u + (1 << (kTexCoordBits - 0)) - (1 << (kTexCoordBits - 1))) >> (kTexCoordBits - kLerpBits);		/* Chroma coordinates, re-rounded appropriate for its sampling */
				vf = (v + (1 << (kTexCoordBits - 0)) - (1 << (kTexCoordBits - 1))) >> (kTexCoordBits - kLerpBits);
				if (uf > uLerpMax)
					uf = uLerpMax;	/* We guard against overflow for luma, but not necessarily for chroma too */
				if (vf > vLerpMax)
					vf = vLerpMax;
				ui = uf >> kLerpBits;	uf -= ui << kLerpBits;
				vi = vf >> kLerpBits;	vf -= vi << kLerpBits;
				s  = (const UInt8*)(td->texPixels) + cbOffset + vi * td->texRowBytes + ui;
				cb  = FskBilerp8(uf, vf, s + 0,        td->texRowBytes>>1);
				cr  = FskBilerp8(uf, vf, s + crOffset, td->texRowBytes>>1);
				*d = FskName2(FskConvertYUV420,DstPixelKind)(y, cb, cr);

			#else
				#error Unsupported pixel packing
			#endif
		}
		span->p = d;				/* Update span pixel pointer */
	}
	else {
		FskPixelType saveColor = span->fillColor;
		for (; span->dx--; u += td->du, v += td->dv, span->p = d = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes))) {
			#if FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking
				ui = u >> kTexCoordBits;	uf = (u >> (kTexCoordBits - kLerpBits)) - (ui << kLerpBits);
				vi = v >> kTexCoordBits;	vf = (v >> (kTexCoordBits - kLerpBits)) - (vi << kLerpBits);
				s  = (const FskName3(Fsk,SrcPixelKind,Type)*)((const char*)(td->texPixels) + vi * td->texRowBytes + ui * FskName3(fsk,SrcPixelKind,Bytes));

				#if (FskName3(fsk,SrcPixelKind,Bytes) == 3)													/* 24 -> xx */
				{	Fsk24BitType px = FskBilerp24(uf, vf, s, td->texRowBytes);
					#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 24 -> 24 */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, span->fillColor.p24);
					#else																					/* 24 -> !24 */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, ui);
						*((FskName3(Fsk,DstPixelKind,Type)*)(&span->fillColor)) = (FskName3(Fsk,DstPixelKind,Type))ui;
					#endif
				}
				#elif (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,DstPixelKind,Bytes) > 2) && (FskName3(fsk,SrcPixelKind,GreenBits) != 5)	/* 16 (!5515) -> 24 */
					ui = FskName3(FskBilerp,SrcPixelKind,32ARGB)(uf, vf, s, td->texRowBytes);
					#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 16 (!5515) -> 24 */
						FskName3(fskConvert,32ARGB,DstPixelKind)(ui, span->fillColor.p24);
					#else																					/* 16 (!5515) -> !24 */
						FskName3(fskConvert,32ARGB,DstPixelKind)(ui);
						*((FskName3(Fsk,DstPixelKind,Type)*)(&span->fillColor)) = (FskName3(Fsk,DstPixelKind,Type))ui;
					#endif
				#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* xx --> 24 */
					ui = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, td->texRowBytes);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(ui, span->fillColor.p24);
				#else																						/* !24, 5515, 32 -> !24 */
					ui = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, td->texRowBytes);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(ui);
					*((FskName3(Fsk,DstPixelKind,Type)*)(&span->fillColor)) = (FskName3(Fsk,DstPixelKind,Type))ui;
				#endif

			#elif FskName3(fsk,SrcPixelKind,PixelPacking) == fskPlanarPixelPacking
				UInt8 y, cb, cr;
				const UInt8 *s;
				ui = u >> kTexCoordBits;	uf = (u >> (kTexCoordBits - kLerpBits)) - (ui << kLerpBits);				/* Luma coordinates */
				vi = v >> kTexCoordBits;	vf = (v >> (kTexCoordBits - kLerpBits)) - (vi << kLerpBits);
				s  = (const UInt8*)(td->texPixels) + vi * td->texRowBytes + ui;
				y  = FskBilerp8(uf, vf, s, td->texRowBytes);

				uf = (u + (1 << (kTexCoordBits - 0)) - (1 << (kTexCoordBits - 1))) >> (kTexCoordBits - kLerpBits);		/* Chroma coordinates, re-rounded appropriate for its sampling */
				vf = (v + (1 << (kTexCoordBits - 0)) - (1 << (kTexCoordBits - 1))) >> (kTexCoordBits - kLerpBits);
				if (uf > uLerpMax)
					uf = uLerpMax;	/* We guard against overflow for luma, but not necessarily for chroma too */
				if (vf > vLerpMax)
					vf = vLerpMax;
				ui = uf >> kLerpBits;	uf -= ui << kLerpBits;
				vi = vf >> kLerpBits;	vf -= vi << kLerpBits;
				s  = (const UInt8*)(td->texPixels) + cbOffset + vi * td->texRowBytes + ui;
				cb  = FskBilerp8(uf, vf, s + 0,        chrRowBytes);
				cr  = FskBilerp8(uf, vf, s + crOffset, chrRowBytes);
				*((FskName3(Fsk,DstPixelKind,Type)*)(&span->fillColor)) = FskName2(FskConvertYUV420,DstPixelKind)(y, cb, cr);

			#else
				#error Unsupported pixel packing
			#endif
			span->setPixel(span);
		}
		span->fillColor = saveColor;
	}
}


#undef SrcPixelKind
#undef DstPixelKind
#undef SrcPixelFmt
#undef DstPixelFmt
