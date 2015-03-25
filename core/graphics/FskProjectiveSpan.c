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
#if (FskName3(fsk,SrcPixelKind,Bytes) == 3) && (FskName3(fsk,DstPixelKind,Bytes) == 3)
	#define COPY_SRC_TO_DST(s, d)   do {                                                  FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*(s), (d)); } while(0)
	#define BILERP_SRC_TO_DST(s, d) do { Fsk24BitType px = FskBilerp24(uf, vf, (s), srb); FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px,   (d)); } while(0)
#elif (FskName3(fsk,SrcPixelKind,Bytes) == 3) && (FskName3(fsk,DstPixelKind,Bytes) != 3)
	#define COPY_SRC_TO_DST(s, d)   do {                                                   FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*(s), i); (d) = (FskName3(Fsk,DstPixelKind,Type))i; } while(0)
	#define BILERP_SRC_TO_DST(s, d) do { Fsk24BitType px = FskBilerp24(uf, vf, (s), srb);  FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px,   i); (d) = (FskName3(Fsk,DstPixelKind,Type))i; } while(0)
#elif (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,DstPixelKind,Bytes) > 2) && (FskName3(fsk,SrcPixelKind,GreenBits) != 5) && (FskName3(fsk,SrcPixelKind,GreenBits) > 0) && (FskName3(fsk,DstPixelKind,Bytes) == 3)
	#define COPY_SRC_TO_DST(s, d)   do { i = (UInt32)(*(s));                                            FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, (d)); } while(0)
	#define BILERP_SRC_TO_DST(s, d) do { i = FskName3(FskBilerp,SrcPixelKind,32ARGB)(uf, vf, (s), srb); FskName3(fskConvert,32ARGB,DstPixelKind)(      i, (d)); } while(0)
#elif (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,DstPixelKind,Bytes) > 2) && (FskName3(fsk,SrcPixelKind,GreenBits) != 5) && (FskName3(fsk,SrcPixelKind,GreenBits) > 0) && (FskName3(fsk,DstPixelKind,Bytes) != 3)
	#define COPY_SRC_TO_DST(s, d)   do { i = (UInt32)(*s);                                              FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i); (d) = (FskName3(Fsk,DstPixelKind,Type))i; } while(0)
	#define BILERP_SRC_TO_DST(s, d) do { i = FskName3(FskBilerp,SrcPixelKind,32ARGB)(uf, vf, (s), srb); FskName3(fskConvert,32ARGB,DstPixelKind)(i);       (d) = (FskName3(Fsk,DstPixelKind,Type))i; } while(0)
#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)
	#define COPY_SRC_TO_DST(s, d)   do { i = (UInt32)(*(s));                                     FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, (d)); } while(0)
	#define BILERP_SRC_TO_DST(s, d) do { i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, (s), srb); FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, (d)); } while(0)
#elif defined(USE_WMMX_BILERP565SE) && (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,SrcPixelKind,GreenBits) == 6) && (FskName3(fsk,SrcPixelKind,RedPosition) == 11)
	#define COPY_SRC_TO_DST(s, d)   do { i = (UInt32)(*(s));                                           FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i); (d) = (FskName3(Fsk,DstPixelKind,Type))i; } while(0)
	#define BILERP_SRC_TO_DST(s, d) do { i = FskBilerp565SE_arm_wMMX(uf, vf, (const UInt16*)(s), srb); FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i); (d) = (FskName3(Fsk,DstPixelKind,Type))i; } while(0)
#else
	#define COPY_SRC_TO_DST(s, d)	do { i = (UInt32)(*s);                                       FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i); (d) = (FskName3(Fsk,DstPixelKind,Type))i; } while(0)
	#define BILERP_SRC_TO_DST(s, d)	do { i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, (s), srb); FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i); (d) = (FskName3(Fsk,DstPixelKind,Type))i; } while(0)
#endif

#if !defined(GATHER_PSAMPL_SIGNATURE) && !defined(GATHER_BILERP_SIGNATURE)
	#ifdef FSK_GATHER_COORDINATE_SIGNATURE
		#define GATHER_PSAMPL_SIGNATURE(i, j)			do { if (gSequenceSignatureAddBytesProc && gFskSequenceSignature) { FskFixed _x;				\
															_x = (i); (*gSequenceSignatureAddBytesProc)(gFskSequenceSignature, &_x, sizeof(_x));		\
															_x = (j); (*gSequenceSignatureAddBytesProc)(gFskSequenceSignature, &_x, sizeof(_x)); } } while(0)
		#define GATHER_BILERP_SIGNATURE(i, j, uf, vf)	do { if (gSequenceSignatureAddBytesProc && gFskSequenceSignature) { FskFixed _x;							\
															_x = ((i) << 4) | uf; (*gSequenceSignatureAddBytesProc)(gFskSequenceSignature, &_x, sizeof(_x));			\
															_x = ((j) << 4) | vf; (*gSequenceSignatureAddBytesProc)(gFskSequenceSignature, &_x, sizeof(_x)); } } while(0)
	#else /* !FSK_GATHER_COORDINATE_SIGNATURE */
		#define GATHER_PSAMPL_SIGNATURE(i, j)			do { } while(0)
		#define GATHER_BILERP_SIGNATURE(i, j, uf, vf)	do { } while(0)
	#endif /* FSK_GATHER_COORDINATE_SIGNATURE */
#endif /* GATHER */

#ifndef USE_SPAN_FORWARD_DIFFERENCING


/********************************************************************************
 ********************************************************************************
 *****				Projective Spans with Division at Every Pixel			*****
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * ProjectiveTexSpanCopy
 ********************************************************************************/

static void
FskName3(ProjectiveTexSpanCopy,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, w, du, dv, dw;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = span->baseAddr;
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	w   = span->w;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	dw  = span->M[0][2];

	for (dx = span->dx; dx--; u += du, v += dv, w += dw) {
		i  = FskFixedNDiv(u, w, 8);																	/* Compute the address of the nearest source pixel */
		j  = FskFixedNDiv(v, w, 8);																	/* w has 8 more fractional bits than u & v */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		COPY_SRC_TO_DST(s, *d);																		/* Do the pixel conversions */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


/********************************************************************************
 * BilerpProjectiveTexSpanCopy
 ********************************************************************************/

static void
FskName3(BilerpProjectiveTexSpanCopy,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, w, du, dv, dw, uf, vf;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = span->baseAddr;
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	w   = span->w;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	dw  = span->M[0][2];
	#if defined(USE_WMMX_BILERP565SE) && (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,SrcPixelKind,GreenBits) == 6) && (FskName3(fsk,SrcPixelKind,RedPosition) == 11) && (FskName3(fsk,DstPixelKind,Bytes) != 3)
		FskBilerp565SESetImm_arm_wMMX_s();
	#endif /* USE_WMMX_BILERP565SE */

	for (dx = span->dx; dx--; u += du, v += dv, w += dw) {
		uf  = FskFixedNDiv(u, w, 8+4);															/* Compute the address of the nearest source pixel, and subpixel deviations */
		vf  = FskFixedNDiv(v, w, 8+4);															/* w has 8 more fractional bits than u & v, plus we generate 4 additional subbits for antialiasing */
		i   = uf >> 4;																			/* Split coordinates into integral ... */
		j   = vf >> 4;
		uf &= 0xF;																				/* ... and fractional parts */
		vf &= 0xF;
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		BILERP_SRC_TO_DST(s, *d);																/* Bilinearly interpolate and do the pixel conversions */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));	/* Increment the destination pixel pointer */
	}
}


#else /* USE_SPAN_FORWARD_DIFFERENCING */


/********************************************************************************
 ********************************************************************************
 *****			Projective Spans with Quadratic Forward Differencing		*****
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * ProjectiveTexSpanCopy
 ********************************************************************************/

static void
FskName3(ProjectiveTexSpanCopy,SrcPixelKind,DstPixelKind)(register Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);


	/********************************************************************************
	 * Do the first pixel separately
	 ********************************************************************************/

	span->u0  = FskFixedNDiv(span->u, span->w, 8+16);												/* Compute the texture cooordinates at the first point */
	span->v0  = FskFixedNDiv(span->v, span->w, 8+16);												/* w has 8 more fractional bits than u & v, plus we generate 16 fractional bits */
	i   = RoundFixedToInt(span->u0);
	j   = RoundFixedToInt(span->v0);
	s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
	CHECK_SRC_BOUNDS
	COPY_SRC_TO_DST(s, *d);																			/* Do the pixel conversions */
	d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));			/* Increment the destination pixel pointer */


	/********************************************************************************
	 * Do the remaining pixels using adaptive quadratic forward differencing
	 ********************************************************************************/

	for (span->dx--; span->dx > 0; ) {
		if (ComputeForwardDifferenceCoefficients(span)) {											/* Use adaptive quadratic forward differencing */
			FskFixed u0, u1, u2, v0, v1, v2;
			u0 = span->u0;																			/* Gather coefficients into registers */
			v0 = span->v0;
			u1 = span->u1;
			v1 = span->v1;
			u2 = span->u2;
			v2 = span->v2;
			for (dx = span->qdx; dx--; ) {
				u0 += u1 >> QFD_EXTRA_BITS;															/* Advance to the next pixel (the first pixel has already been done) */
				v0 += v1 >> QFD_EXTRA_BITS;
				u1 += u2;
				v1 += v2;
				i   = RoundFixedToInt(u0);															/* Compute the address of the nearest source pixel */
				j   = RoundFixedToInt(v0);
				s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
				CHECK_SRC_BOUNDS
				COPY_SRC_TO_DST(s, *d);																	/* Do the pixel conversions */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));	/* Increment the destination pixel pointer */
			}
			span->u0 = span->un;																	/* Update the state machine. Note that span->dx, span->u, span->v, and span->w ... */
			span->v0 = span->vn;																	/* ... have already been updated in ComputeForwardDifferenceCoefficients(). */
		}
		else {																						/* Projective interpolation */
			FskFixed u, v, w, du, dv, dw;
			u   = span->u;																			/* Gather coefficients into registers */
			v   = span->v;
			w   = span->w;
			du  = span->M[0][0];
			dv  = span->M[0][1];
			dw  = span->M[0][2];

			if (span->dx)																			/* There will be a QFD loop after this */
				span->qdx--;																		/* We perform the last pixel separately */

			for (dx = span->qdx; dx--;) {
				u += du;																			/* Advance to the next pixel (the first pixel has already been done) */
				v += dv;
				w += dw;
				i = FskFixedNDiv(u, w, 8);															/* Compute the address of the nearest source pixel; w has 8 more fractional bits than u & v */
				j = FskFixedNDiv(v, w, 8);															/* FskFixedNDiv() rounds appropriately */
				s = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
				CHECK_SRC_BOUNDS
				COPY_SRC_TO_DST(s, *d);																	/* Do the pixel conversions */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));	/* Increment the destination pixel pointer */
			}

			if (span->dx) {																			/* Last pixel is done a little differently, to set up appropriately for a subsequent QFD subspan */
				u += du;																			/* Advance to the next pixel (the first pixel has already been done) */
				v += dv;
				w += dw;
				span->u0 = FskFixedNDiv(u, w, 8+16);												/* Compute the address of the nearest source pixel */
				span->v0 = FskFixedNDiv(v, w, 8+16);												/* w has 8 more fractional bits than u & v, plus we generate 16 fractional bits */
				i = FskRoundFixedToInt(span->u0);
				j = FskRoundFixedToInt(span->v0);
				s = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
				CHECK_SRC_BOUNDS
				COPY_SRC_TO_DST(s, *d);																	/* Do the pixel conversions */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));	/* Increment the destination pixel pointer */
			}

			span->u = u;																			/* Update the state machine. Note that span->dx ... */
			span->v = v;																			/* ... has already been updated in ComputeForwardDifferenceCoefficients(). */
			span->w = w;
		}
	}
}


/********************************************************************************
 * BilerpProjectiveTexSpanCopy
 ********************************************************************************/

static void
FskName3(BilerpProjectiveTexSpanCopy,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)			*d;
	const char								*s0;
	SInt32									srb;
	UInt32									i, j, dx;
	FskFixed								uf, vf;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	#if defined(USE_WMMX_BILERP565SE) && (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,SrcPixelKind,GreenBits) == 6) && (FskName3(fsk,SrcPixelKind,RedPosition) == 11) && (FskName3(fsk,DstPixelKind,Bytes) != 3)
		FskBilerp565SESetImm_arm_wMMX_s();
	#endif /* USE_WMMX_BILERP565SE */

	/********************************************************************************
	 * Do the first pixel separately
	 ********************************************************************************/

	span->u0  = FskFixedNDiv(span->u, span->w, 8+16);												/* Compute the texture cooordinates at the first point */
	span->v0  = FskFixedNDiv(span->v, span->w, 8+16);
	uf  = span->u0 + 0x800;
	vf  = span->v0 + 0x800;
	i   = TruncFixedToInt(uf);
	j   = TruncFixedToInt(vf);
	uf  = (uf >> (16 - 4)) & 0xF;
	vf  = (vf >> (16 - 4)) & 0xF;
	s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
	CHECK_SRC_BOUNDS
	GATHER_BILERP_SIGNATURE(i, j, uf, vf);
	BILERP_SRC_TO_DST(s, *d);																		/* Bilinearly interpolate and do the pixel conversions */
	d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));			/* Increment the destination pixel pointer */

	/********************************************************************************
	 * Do the remaining pixels using adaptive quadratic forward differencing
	 ********************************************************************************/
	for (span->dx--; span->dx > 0; ) {
		if (ComputeForwardDifferenceCoefficients(span)) {											/* Use adaptive quadratic forward differencing */
			FskFixed u0, u1, u2, v0, v1, v2;
			u0 = span->u0;																			/* Gather coefficients into registers */
			v0 = span->v0;
			u1 = span->u1;
			v1 = span->v1;
			u2 = span->u2;
			v2 = span->v2;

			for (dx = span->qdx; dx--; ) {
				u0 += u1 >> QFD_EXTRA_BITS;															/* Advance to the next pixel (the first pixel has already been done) */
				v0 += v1 >> QFD_EXTRA_BITS;
				u1 += u2;
				v1 += v2;
				uf  = u0 + 0x800;																	/* Compute the address of the nearest source pixel */
				vf  = v0 + 0x800;
				i   = TruncFixedToInt(uf);
				j   = TruncFixedToInt(vf);
				uf  = (uf >> (16 - 4)) & 0xF;
				vf  = (vf >> (16 - 4)) & 0xF;
				s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
				CHECK_SRC_BOUNDS
				GATHER_BILERP_SIGNATURE(i, j, uf, vf);
				BILERP_SRC_TO_DST(s, *d);																/* Bilinearly interpolate and do the pixel conversions */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));	/* Increment the destination pixel pointer */
			}
			span->u0 = span->un;																	/* Update the state machine. Note that span->dx, span->u, span->v, and span->w ... */
			span->v0 = span->vn;																	/* ... have already been updated in ComputeForwardDifferenceCoefficients(). */
		}
		else {																						/* Projective interpolation */
			FskFixed u, v, w, du, dv, dw;
			u   = span->u;																			/* Gather coefficients into registers */
			v   = span->v;
			w   = span->w;
			du  = span->M[0][0];
			dv  = span->M[0][1];
			dw  = span->M[0][2];

			if (span->dx)																			/* There will be a QFD loop after this */
				span->qdx--;																		/* We perform the last pixel separately */

			for (dx = span->qdx; dx--;) {
				u += du;																			/* Advance to the next pixel (the first pixel has already been done) */
				v += dv;
				w += dw;
				uf  = FskFixedNDiv(u, w, 8+4);														/* Compute the address of the nearest source pixel; w has 8 extra fractional bits, ... */
				vf  = FskFixedNDiv(v, w, 8+4);														/* ... but we compute 4 additional subbits for bilinear interpolation; FskFixedNDiv() rounds appropriately */
				i   = uf >> 4;
				j   = vf >> 4;
				uf &= 0xF;
				vf &= 0xF;
				s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
				CHECK_SRC_BOUNDS
				GATHER_BILERP_SIGNATURE(i, j, uf, vf);
				BILERP_SRC_TO_DST(s, *d);																/* Bilinearly interpolate and do the pixel conversions */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));	/* Increment the destination pixel pointer */
			}

			if (span->dx) {																			/* Last pixel is done a little differently, to set up appropriately for a subsequent QFD subspan */
				u += du;																			/* Advance to the next pixel */
				v += dv;
				w += dw;
				span->u0 = FskFixedNDiv(u, w, 8+16);												/* Compute the address of the nearest source pixel */
				span->v0 = FskFixedNDiv(v, w, 8+16);												/* w has 8 extra fractional bits, but we compute 16 additional fractional bits */
				vf  = span->v0 + 0x800;
				i   = TruncFixedToInt(uf);
				j   = TruncFixedToInt(vf);
				uf  = (uf >> (16 - 4)) & 0xF;
				vf  = (vf >> (16 - 4)) & 0xF;
				s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
				CHECK_SRC_BOUNDS
				GATHER_BILERP_SIGNATURE(i, j, uf, vf);
				BILERP_SRC_TO_DST(s, *d);																/* Bilinearly interpolate and do the pixel conversions */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));	/* Increment the destination pixel pointer */
			}
			span->u = u;																			/* Update the state machine. Note that span->dx ... */
			span->v = v;																			/* ... has already been updated in ComputeForwardDifferenceCoefficients(). */
			span->w = w;
		}
	}
}



#endif /* USE_SPAN_FORWARD_DIFFERENCING */


/********************************************************************************
 * ProjectiveTexSpanBlend
 ********************************************************************************/

static void
FskName3(ProjectiveTexSpanBlend,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, w, du, dv, dw;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	w   = span->w;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	dw  = span->M[0][2];

	for (dx = span->dx; dx--; u += du, v += dv, w += dw) {
		i  = FskFixedNDiv(u, w, 8);																	/* Compute the address of the nearest source pixel */
		j  = FskFixedNDiv(v, w, 8);																	/* w has 8 more fractional bits than u & v */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if FskName3(fsk,DstPixelKind,Bytes) != 3
			COPY_SRC_TO_DST(s, i);
			FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, span->blendLevel);
		#else /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
		{	Fsk24BitType pix;
			COPY_SRC_TO_DST(s, pix);
			FskName2(FskBlend,DstPixelKind)(d, pix, span->blendLevel);
		}
		#endif /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


/********************************************************************************
 * BilerpProjectiveTexSpanBlend
 ********************************************************************************/

static void
FskName3(BilerpProjectiveTexSpanBlend,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, w, du, dv, dw, uf, vf;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	w   = span->w;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	dw  = span->M[0][2];

	for (dx = span->dx; dx--; u += du, v += dv, w += dw) {
		uf  = FskFixedNDiv(u, w, 8+4);																/* Compute the address of the nearest source pixel, and subpixel deviations */
		vf  = FskFixedNDiv(v, w, 8+4);																/* w has 8 more fractional bits than u & v, plus we generate 4 additional subbits for antialiasing */
		i   = uf >> 4;																				/* Split coordinates into integral ... */
		j   = vf >> 4;
		uf &= 0xF;																					/* ... and fractional parts */
		vf &= 0xF;
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if FskName3(fsk,DstPixelKind,Bytes) != 3
			BILERP_SRC_TO_DST(s, i);
			FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, span->blendLevel);
		#else /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
		{	Fsk24BitType pix;
			BILERP_SRC_TO_DST(s, pix);
			FskName2(FskBlend,DstPixelKind)(d, pix, span->blendLevel);
		}
		#endif /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


#if FskName3(fsk,SrcPixelKind,AlphaBits) >= 4

/********************************************************************************
 * ProjectiveTexSpanAlpha
 ********************************************************************************/

static void
FskName3(ProjectiveTexSpanAlpha,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, w, du, dv, dw;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	w   = span->w;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	dw  = span->M[0][2];

	for (dx = span->dx; dx--; u += du, v += dv, w += dw) {
		i  = FskFixedNDiv(u, w, 8);																	/* Compute the address of the nearest source pixel */
		j  = FskFixedNDiv(v, w, 8);																	/* w has 8 more fractional bits than u & v */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if (FskName3(fsk,DstPixelKind,AlphaBits) >= 4)												/* If dst has a bit enough alpha */
			i = *s;
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);										/* Convert src to dst format */
			if (!span->isPremul)	FskName2(FskAlpha,DstPixelKind)(d, i);							/* Blend as dst */
			else	/* premul */	FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, i);
		#else /* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
			i = *s;																					/* Get src pixel */
			j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
			#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
				j *= 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);						/* Make sure that the src alpha is 8 bits */
			#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);									/* Convert src to dst format */
				if (!span->isPremul) {
					#if FskName3(fsk,DstPixelKind,AlphaBits) > 0
						i |= fskFieldMask(FskName3(fsk,DstPixelKind,AlphaBits), FskName3(fsk,DstPixelKind,AlphaPosition))
					#endif /* FskName3(fsk,DstPixelKind,AlphaBits) > 0 */
					FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, (UInt8)j);
				}
				else {	/* premul */
					#if FskName3(fsk,DstPixelKind,GreenBits) == 6
						*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j) >> 2, *d, (FskName3(Fsk,DstPixelKind,Type))i);	/* Blend and write - NB AXPY565 takes a 6 bit alpha */
					#else /* FskName3(fsk,DstPixelKind,GreenBits) != 6 */
						*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j),      *d, (FskName3(Fsk,DstPixelKind,Type))i);	/* Blend and write */
					#endif /* FskName3(fsk,DstPixelKind,GreenBits) != 6 */
				}
			#else /* FskName3(Fsk,DstPixelKind,Bytes) == 3 */
			{	FskName3(Fsk,DstPixelKind,Type) pix;
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, pix);								/* Convert src to dst format */
				if (!span->isPremul)	FskName2(FskBlend,DstPixelKind)(d, pix, (UInt8)j);			/* Blend and write */
				else	/* premul */	*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j), *d, pix);
			}
			#endif /* FskName3(fsk,DstPixelKind,Bits) == 3 */
		#endif /* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


/********************************************************************************
 * BilerpProjectiveTexSpanAlpha
 ********************************************************************************/

static void
FskName3(BilerpProjectiveTexSpanAlpha,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, w, du, dv, dw, uf, vf;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	w   = span->w;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	dw  = span->M[0][2];

	for (dx = span->dx; dx--; u += du, v += dv, w += dw) {
		uf  = FskFixedNDiv(u, w, 8+4);																/* Compute the address of the nearest source pixel, and subpixel deviations */
		vf  = FskFixedNDiv(v, w, 8+4);																/* w has 8 more fractional bits than u & v, plus we generate 4 additional subbits for antialiasing */
		i   = uf >> 4;																				/* Split coordinates into integral ... */
		j   = vf >> 4;
		uf &= 0xF;																					/* ... and fractional parts */
		vf &= 0xF;
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if (FskName3(fsk,DstPixelKind,AlphaBits) >= 4)												/* If dst has a bit enough alpha */
			BILERP_SRC_TO_DST(s, i);																/* Bilinearly interpolate and do the pixel conversions */
			if (!span->isPremul)	FskName2(FskAlpha,DstPixelKind)(d, i);							/* Blend as dst */
			else	/* premul */	FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, i);
		#else 	/* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
			i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);									/* Get src pixel */
			j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
			#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
				j *= 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);						/* Make sure that the src alpha is 8 bits */
			#endif 	/* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);									/* Convert src to dst format */
				if (!span->isPremul) {
					#if FskName3(fsk,DstPixelKind,AlphaBits) > 0
						i |= fskFieldMask(FskName3(fsk,DstPixelKind,AlphaBits), FskName3(fsk,DstPixelKind,AlphaPosition))
					#endif 	/* FskName3(fsk,DstPixelKind,AlphaBits) > 0 */
					FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, (UInt8)j);
				}
				else {	/* premul */
					#if FskName3(fsk,DstPixelKind,GreenBits) == 6
						*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j) >> 2, *d, (FskName3(Fsk,DstPixelKind,Type))i);	/* Blend and write: N.B. AXPY565LE takes a 6 bit alpha */
					#else /* FskName3(fsk,DstPixelKind,GreenBits) != 6 */
						*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j),      *d, (FskName3(Fsk,DstPixelKind,Type))i);	/* Blend and write */
					#endif /* FskName3(fsk,DstPixelKind,GreenBits) != 6 */
				}
			#else 	/* FskName3(fsk,DstPixelKind,Bytes) == 3 */
			{	FskName3(Fsk,DstPixelKind,Type) pix;
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, pix);								/* Convert src to dst format */
				if (!span->isPremul)	FskName2(FskBlend,DstPixelKind)(d, pix, (UInt8)j);			/* Blend and write */
				else	/* premul */	*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j), *d, pix);
			}
			#endif 	/* FskName3(fsk,DstPixelKind,Bits) == 3 */
		#endif 	/* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


/********************************************************************************
 * ProjectiveTexSpanAlphaBlend
 ********************************************************************************/

static void
FskName3(ProjectiveTexSpanAlphaBlend,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, w, du, dv, dw;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	w   = span->w;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	dw  = span->M[0][2];

	for (dx = span->dx; dx--; u += du, v += dv, w += dw) {
		i  = FskFixedNDiv(u, w, 8);																	/* Compute the address of the nearest source pixel */
		j  = FskFixedNDiv(v, w, 8);																	/* w has 8 more fractional bits than u & v */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if (FskName3(fsk,DstPixelKind,AlphaBits) >= 4)												/* If dst has a bit enough alpha */
			i = *s;
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);										/* Convert src to dst format */
			if (!span->isPremul) {
				FskName2(FskAlphaBlend,DstPixelKind)(d, i, span->blendLevel);						/* Alpha blend as dst */
			}
			else {	/* premul */
				i = FskName2(FskAlphaScale,DstPixelKind)(span->blendLevel, i);						/* Scale premultiplied color by the blendLevel */
				FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, i);								/* Blend over */
			}
		#else /* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
			i = *s;																					/* Get src pixel */
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				if (!span->isPremul) {
					j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
					#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
						j *= 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);				/* Make sure that the src alpha is 8 bits */
					#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
					j = FskAlphaMul(span->blendLevel, (UInt8)j);									/* Scale alpha by the blend level */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);								/* Convert src to dst format */
					#if FskName3(fsk,DstPixelKind,AlphaBits) > 0
						i |= fskFieldMask(FskName3(fsk,DstPixelKind,AlphaBits), FskName3(fsk,DstPixelKind,AlphaPosition))	/* Max src alpha because blend will restore it */
					#endif /* FskName3(fsk,DstPixelKind,AlphaBits) > 0 */
					FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, (UInt8)j);
				}
				else {	/* premul */
					UInt32 dst = *d;
					#if FskName3(fsk,SrcPixelKind,RedBits) == 8
						FskName3(fskConvert,DstPixelKind,SrcPixelKind)(dst);
						FskName2(FskAlphaBlackSourceOver,SrcPixelKind)(&dst, FskName2(FskAlphaScale,SrcPixelKind)((UInt8)(span->blendLevel), i));
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(dst);
					#else /* FskName3(fsk,SrcPixelKind,RedBits) < 8 */
						FskName3(fskConvert,SrcPixelKind,32ARGB)(i);
						FskName3(fskConvert,DstPixelKind,32ARGB)(dst);
						FskName2(FskAlphaBlackSourceOver,32ARGB)(&dst, FskName2(FskAlphaScale,32ARGB)((UInt8)(span->blendLevel), i));
						FskName3(fskConvert,32ARGB,DstPixelKind)(dst);
					#endif /* FskName3(fsk,SrcPixelKind,RedBits) < 8 */
					*d = (FskName3(Fsk,DstPixelKind,Type))dst;
				}
			#else /* FskName3(Fsk,DstPixelKind,Bytes) == 3 */
			{	FskName3(Fsk,DstPixelKind,Type) pix;
				j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, pix);								/* Convert src to dst format */
				if (!span->isPremul)	FskName2(FskBlend,DstPixelKind)(d, pix, (UInt8)j);			/* Blend and write */
				else	/* premul */	*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j), *d, pix);
			}
			#endif /* FskName3(fsk,DstPixelKind,Bits) == 3 */
		#endif /* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


/********************************************************************************
 * BilerpProjectiveTexSpanAlphaBlend
 ********************************************************************************/

static void
FskName3(BilerpProjectiveTexSpanAlphaBlend,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, w, du, dv, dw, uf, vf;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	w   = span->w;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	dw  = span->M[0][2];

	for (dx = span->dx; dx--; u += du, v += dv, w += dw) {
		uf  = FskFixedNDiv(u, w, 8+4);																/* Compute the address of the nearest source pixel, and subpixel deviations */
		vf  = FskFixedNDiv(v, w, 8+4);																/* w has 8 more fractional bits than u & v, plus we generate 4 additional subbits for antialiasing */
		i   = uf >> 4;																				/* Split coordinates into integral ... */
		j   = vf >> 4;
		uf &= 0xF;																					/* ... and fractional parts */
		vf &= 0xF;
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if (FskName3(fsk,DstPixelKind,AlphaBits) >= 4)												/* If dst has a bit enough alpha */
			BILERP_SRC_TO_DST(s, i);																/* Bilinearly interpolate and do the pixel conversions */
			if (!span->isPremul) {
				FskName2(FskAlphaBlend,DstPixelKind)(d, i, span->blendLevel);						/* Alpha blend as dst */
			}
			else {	/* premul */
				i = FskName2(FskAlphaScale,DstPixelKind)(span->blendLevel, i);						/* Scale premultiplied color by the blendLevel */
				FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, i);								/* Blend over */
			}
		#else 	/* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
			i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);									/* Get src pixel */
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				if (!span->isPremul) {
					j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
					#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
						j *= 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);				/* Make sure that the src alpha is 8 bits */
					#endif 	/* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
					j = FskAlphaMul(span->blendLevel, (UInt8)j);									/* Scale alpha by the blend level */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);								/* Convert src to dst format */
					#if FskName3(fsk,DstPixelKind,AlphaBits) > 0
						i |= fskFieldMask(FskName3(fsk,DstPixelKind,AlphaBits), FskName3(fsk,DstPixelKind,AlphaPosition))
					#endif 	/* FskName3(fsk,DstPixelKind,AlphaBits) > 0 */
					FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, (UInt8)j);
				}
				else {	/* premul */
					UInt32 dst = *d;
					#if FskName3(fsk,SrcPixelKind,RedBits) == 8
						FskName3(fskConvert,DstPixelKind,SrcPixelKind)(dst);
						FskName2(FskAlphaBlackSourceOver,SrcPixelKind)(&dst, FskName2(FskAlphaScale,SrcPixelKind)((UInt8)(span->blendLevel), i));
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(dst);
					#else /* FskName3(fsk,SrcPixelKind,RedBits) < 8 */
						FskName3(fskConvert,SrcPixelKind,32ARGB)(i);
						FskName3(fskConvert,DstPixelKind,32ARGB)(dst);
						FskName2(FskAlphaBlackSourceOver,32ARGB)(&dst, FskName2(FskAlphaScale,32ARGB)((UInt8)(span->blendLevel), i));
						FskName3(fskConvert,32ARGB,DstPixelKind)(dst);
					#endif /* FskName3(fsk,SrcPixelKind,RedBits) < 8 */
					*d = (FskName3(Fsk,DstPixelKind,Type))dst;
				}
			#else 	/* FskName3(fsk,DstPixelKind,Bytes) == 3 */
			{	FskName3(Fsk,DstPixelKind,Type) pix;
				j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, pix);								/* Convert src to dst format */
				if (!span->isPremul)	FskName2(FskBlend,DstPixelKind)(d, pix, (UInt8)j);			/* Blend and write */
				else	/* premul */	*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j), *d, pix);
			}
			#endif 	/* FskName3(fsk,DstPixelKind,Bits) == 3 */
		#endif 	/* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


#else /* FskName3(fsk,SrcPixelKind,AlphaBits) < 4 */


static void            FskName3(ProjectiveTexSpanAlpha,SrcPixelKind,DstPixelKind)(Span *span) {        FskName3(ProjectiveTexSpanCopy,SrcPixelKind,DstPixelKind)(span); }
static void      FskName3(BilerpProjectiveTexSpanAlpha,SrcPixelKind,DstPixelKind)(Span *span) {  FskName3(BilerpProjectiveTexSpanCopy,SrcPixelKind,DstPixelKind)(span); }
static void       FskName3(ProjectiveTexSpanAlphaBlend,SrcPixelKind,DstPixelKind)(Span *span) {       FskName3(ProjectiveTexSpanBlend,SrcPixelKind,DstPixelKind)(span); }
static void FskName3(BilerpProjectiveTexSpanAlphaBlend,SrcPixelKind,DstPixelKind)(Span *span) { FskName3(BilerpProjectiveTexSpanBlend,SrcPixelKind,DstPixelKind)(span); }


#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) >= 4 */



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Affine Spans							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/




/********************************************************************************
 * AffineTexSpanCopy
 ********************************************************************************/

static void
FskName3(AffineTexSpanCopy,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, du, dv;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	du  = span->M[0][0];
	dv  = span->M[0][1];

	for (dx = span->dx; dx--; u += du, v += dv) {
		i   = (u + (1 << (AFFINE_EXTRA_BITS - 1))) >> AFFINE_EXTRA_BITS;							/* Compute the address of the nearest source pixel */
		j   = (v + (1 << (AFFINE_EXTRA_BITS - 1))) >> AFFINE_EXTRA_BITS;							/* ... rounding to nearest pixel */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		COPY_SRC_TO_DST(s, *d);																		/* Do the pixel conversions */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


/********************************************************************************
 * BilerpAffineTexSpanCopy
 ********************************************************************************/

static void
FskName3(BilerpAffineTexSpanCopy,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, du, dv, uf, vf;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	#if defined(USE_WMMX_BILERP565SE) && (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,SrcPixelKind,GreenBits) == 6) && (FskName3(fsk,SrcPixelKind,RedPosition) == 11) && (FskName3(fsk,DstPixelKind,Bytes) != 3)
		FskBilerp565SESetImm_arm_wMMX_s();
	#endif /* USE_WMMX_BILERP565SE */

	for (dx = span->dx; dx--; u += du, v += dv) {
		uf  = (u + (1 << (AFFINE_EXTRA_BITS - 4 - 1))) >> (AFFINE_EXTRA_BITS - 4);					/* Compute the address of the nearest source pixel, and subpixel deviations ... */
		vf  = (v + (1 << (AFFINE_EXTRA_BITS - 4 - 1))) >> (AFFINE_EXTRA_BITS - 4);					/* ... rounding to nearest 1/16 of a pixel */
		i   = uf >> 4;																				/* Integral coordinate */
		j   = vf >> 4;																				/* Integral coordinate */
		uf &= 0xF;																					/* Fractional coordinate */
		vf &= 0xF;																					/* Fractional coordinate */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		BILERP_SRC_TO_DST(s, *d);																	/* Bilinearly interpolate and do the pixel conversions */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


/********************************************************************************
 * AffineTexSpanBlend
 ********************************************************************************/

static void
FskName3(AffineTexSpanBlend,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, du, dv;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	du  = span->M[0][0];
	dv  = span->M[0][1];

	for (dx = span->dx; dx--; u += du, v += dv) {
		i = (u + (1 << (AFFINE_EXTRA_BITS - 1))) >> AFFINE_EXTRA_BITS;								/* Compute the address of the nearest source pixel */
		j = (v + (1 << (AFFINE_EXTRA_BITS - 1))) >> AFFINE_EXTRA_BITS;								/* ... rounding to nearest pixel */
		s = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if FskName3(fsk,DstPixelKind,Bytes) != 3
			COPY_SRC_TO_DST(s, i);
			FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, span->blendLevel);
		#else /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
		{	Fsk24BitType pix;
			COPY_SRC_TO_DST(s, pix);
			FskName2(FskBlend,DstPixelKind)(d, pix, span->blendLevel);
		}
		#endif /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


/********************************************************************************
 * BilerpAffineTexSpanBlend
 ********************************************************************************/

static void
FskName3(BilerpAffineTexSpanBlend,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, du, dv, uf, vf;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	#if defined(USE_WMMX_BILERP565SE) && (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,SrcPixelKind,GreenBits) == 6) && (FskName3(fsk,SrcPixelKind,RedPosition) == 11) && (FskName3(fsk,DstPixelKind,Bytes) != 3)
		FskBilerp565SESetImm_arm_wMMX_s();
	#endif /* USE_WMMX_BILERP565SE */

	for (dx = span->dx; dx--; u += du, v += dv) {
		uf  = (u + (1 << (AFFINE_EXTRA_BITS - 4 - 1))) >> (AFFINE_EXTRA_BITS - 4);					/* Compute the address of the nearest source pixel, and subpixel deviations ... */
		vf  = (v + (1 << (AFFINE_EXTRA_BITS - 4 - 1))) >> (AFFINE_EXTRA_BITS - 4);					/* ... rounding to nearest 1/16 of a pixel */
		i   = uf >> 4;																				/* Integral coordinate */
		j   = vf >> 4;																				/* Integral coordinate */
		uf &= 0xF;																					/* Fractional coordinate */
		vf &= 0xF;																					/* Fractional coordinate */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if FskName3(fsk,DstPixelKind,Bytes) != 3
			BILERP_SRC_TO_DST(s, i);
			FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, span->blendLevel);
		#else /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
		{	Fsk24BitType pix;
			BILERP_SRC_TO_DST(s, pix);
			FskName2(FskBlend,DstPixelKind)(d, pix, span->blendLevel);
		}
		#endif /* FskName3(fsk,DstPixelKind,Bytes) == 3 */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


#if FskName3(fsk,SrcPixelKind,AlphaBits) >= 4


/********************************************************************************
 * AffineTexSpanAlpha
 ********************************************************************************/

static void
FskName3(AffineTexSpanAlpha,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, du, dv;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	du  = span->M[0][0];
	dv  = span->M[0][1];

	for (dx = span->dx; dx--; u += du, v += dv) {
		i = (u + (1 << (AFFINE_EXTRA_BITS - 1))) >> AFFINE_EXTRA_BITS;								/* Compute the address of the nearest source pixel */
		j = (v + (1 << (AFFINE_EXTRA_BITS - 1))) >> AFFINE_EXTRA_BITS;								/* ... rounding to nearest pixel */
		s = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if (FskName3(fsk,DstPixelKind,AlphaBits) >= 4)												/* If dst has a bit enough alpha */
			i = *s;
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);										/* Convert src to dst format */
			if (!span->isPremul)	FskName2(FskAlpha,DstPixelKind)(d, i);							/* Blend as dst */
			else	/* premul */	FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, i);
		#else /* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
			i = *s;																					/* Get src pixel */
			j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
			#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
				j *= 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);						/* Make sure that the src alpha is 8 bits */
			#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);									/* Convert src to dst format */
				if (!span->isPremul) {
					#if FskName3(fsk,DstPixelKind,AlphaBits) > 0
						i |= fskFieldMask(FskName3(fsk,DstPixelKind,AlphaBits), FskName3(fsk,DstPixelKind,AlphaPosition))
					#endif /* FskName3(fsk,DstPixelKind,AlphaBits) > 0 */
					FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, (UInt8)j);
				}
				else {	/* premul */
					#if FskName3(fsk,DstPixelKind,GreenBits) == 6
						*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j) >> 2, *d, (FskName3(Fsk,DstPixelKind,Type))i);	/* Blend and write - NB AXPY565 takes a 6 bit alpha */
					#else /* FskName3(fsk,DstPixelKind,GreenBits) != 6 */
						*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j),      *d, (FskName3(Fsk,DstPixelKind,Type))i);	/* Blend and write */
					#endif /* FskName3(fsk,DstPixelKind,GreenBits) != 6 */
				}
			#else /* FskName3(Fsk,DstPixelKind,Bytes) == 3 */
			{	FskName3(Fsk,DstPixelKind,Type) pix;
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, pix);								/* Convert src to dst format */
				if (!span->isPremul)	FskName2(FskBlend,DstPixelKind)(d, pix, (UInt8)j);			/* Blend and write */
				else	/* premul */	*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j), *d, pix);
			}
			#endif /* FskName3(fsk,DstPixelKind,Bits) == 3 */
		#endif /* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


/********************************************************************************
 * BilerpAffineTexSpanAlpha
 ********************************************************************************/

static void
FskName3(BilerpAffineTexSpanAlpha,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, du, dv, uf, vf;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	#if defined(USE_WMMX_BILERP565SE) && (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,SrcPixelKind,GreenBits) == 6) && (FskName3(fsk,SrcPixelKind,RedPosition) == 11) && (FskName3(fsk,DstPixelKind,Bytes) != 3)
		FskBilerp565SESetImm_arm_wMMX_s();
	#endif /* USE_WMMX_BILERP565SE */

	for (dx = span->dx; dx--; u += du, v += dv) {
		uf  = (u + (1 << (AFFINE_EXTRA_BITS - 4 - 1))) >> (AFFINE_EXTRA_BITS - 4);					/* Compute the address of the nearest source pixel, and subpixel deviations ... */
		vf  = (v + (1 << (AFFINE_EXTRA_BITS - 4 - 1))) >> (AFFINE_EXTRA_BITS - 4);					/* ... rounding to nearest 1/16 of a pixel */
		i   = uf >> 4;																				/* Integral coordinate */
		j   = vf >> 4;																				/* Integral coordinate */
		uf &= 0xF;																					/* Fractional coordinate */
		vf &= 0xF;																					/* Fractional coordinate */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if (FskName3(fsk,DstPixelKind,AlphaBits) >= 4)												/* If dst has a bit enough alpha */
			BILERP_SRC_TO_DST(s, i);																/* Bilinearly interpolate and do the pixel conversions */
			if (!span->isPremul)	FskName2(FskAlpha,DstPixelKind)(d, i);							/* Blend as dst */
			else	/* premul */	FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, i);
		#else 	/* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
			i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);									/* Get src pixel */
			j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
			#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
				j *= 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);						/* Make sure that the src alpha is 8 bits */
			#endif 	/* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);									/* Convert src to dst format */
				if (!span->isPremul) {
					#if FskName3(fsk,DstPixelKind,AlphaBits) > 0
						i |= fskFieldMask(FskName3(fsk,DstPixelKind,AlphaBits), FskName3(fsk,DstPixelKind,AlphaPosition))
					#endif 	/* FskName3(fsk,DstPixelKind,AlphaBits) > 0 */
					FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, (UInt8)j);
				}
				else {	/* premul */
					#if FskName3(fsk,DstPixelKind,GreenBits) == 6
						*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j) >> 2, *d, (FskName3(Fsk,DstPixelKind,Type))i);	/* Blend and write: N.B. AXPY565LE takes a 6 bit alpha */
					#else /* FskName3(fsk,DstPixelKind,GreenBits) != 6 */
						*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j),      *d, (FskName3(Fsk,DstPixelKind,Type))i);	/* Blend and write */
					#endif /* FskName3(fsk,DstPixelKind,GreenBits) != 6 */
				}
			#else 	/* FskName3(fsk,DstPixelKind,Bytes) == 3 */
			{	FskName3(Fsk,DstPixelKind,Type) pix;
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, pix);								/* Convert src to dst format */
				if (!span->isPremul)	FskName2(FskBlend,DstPixelKind)(d, pix, (UInt8)j);			/* Blend and write */
				else	/* premul */	*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j), *d, pix);
			}
			#endif 	/* FskName3(fsk,DstPixelKind,Bits) == 3 */
		#endif 	/* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


/********************************************************************************
 * AffineTexSpanAlphaBlend
 ********************************************************************************/

static void
FskName3(AffineTexSpanAlphaBlend,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, du, dv;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	du  = span->M[0][0];
	dv  = span->M[0][1];

	for (dx = span->dx; dx--; u += du, v += dv) {
		i   = (u + (1 << (AFFINE_EXTRA_BITS - 1))) >> AFFINE_EXTRA_BITS;							/* Compute the address of the nearest source pixel */
		j   = (v + (1 << (AFFINE_EXTRA_BITS - 1))) >> AFFINE_EXTRA_BITS;							/* ... rounding to nearest pixel */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if (FskName3(fsk,DstPixelKind,AlphaBits) >= 4)												/* If dst has a bit enough alpha */
			i = *s;
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);										/* Convert src to dst format */
			if (!span->isPremul) {
				FskName2(FskAlphaBlend,DstPixelKind)(d, i, span->blendLevel);						/* Alpha blend as dst */
			}
			else {	/* premul */
				i = FskName2(FskAlphaScale,DstPixelKind)(span->blendLevel, i);						/* Scale premultiplied color by the blendLevel */
				FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, i);								/* Blend over */
			}
		#else /* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
			i = *s;																					/* Get src pixel */
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				if (!span->isPremul) {
					j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
					#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
						j *= 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);				/* Make sure that the src alpha is 8 bits */
					#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
					j = FskAlphaMul(span->blendLevel, (UInt8)j);									/* Scale alpha by the blend level */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);								/* Convert src to dst format */
					#if FskName3(fsk,DstPixelKind,AlphaBits) > 0
						i |= fskFieldMask(FskName3(fsk,DstPixelKind,AlphaBits), FskName3(fsk,DstPixelKind,AlphaPosition))	/* Max src alpha because blend will restore it */
					#endif /* FskName3(fsk,DstPixelKind,AlphaBits) > 0 */
					FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, (UInt8)j);
				}
				else {	/* premul */
					UInt32 dst = *d;
					#if FskName3(fsk,SrcPixelKind,RedBits) == 8
						FskName3(fskConvert,DstPixelKind,SrcPixelKind)(dst);
						FskName2(FskAlphaBlackSourceOver,SrcPixelKind)(&dst, FskName2(FskAlphaScale,SrcPixelKind)((UInt8)(span->blendLevel), i));
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(dst);
					#else /* FskName3(fsk,SrcPixelKind,RedBits) < 8 */
						FskName3(fskConvert,SrcPixelKind,32ARGB)(i);
						FskName3(fskConvert,DstPixelKind,32ARGB)(dst);
						FskName2(FskAlphaBlackSourceOver,32ARGB)(&dst, FskName2(FskAlphaScale,32ARGB)((UInt8)(span->blendLevel), i));
						FskName3(fskConvert,32ARGB,DstPixelKind)(dst);
					#endif /* FskName3(fsk,SrcPixelKind,RedBits) < 8 */
					*d = (FskName3(Fsk,DstPixelKind,Type))dst;
				}
			#else /* FskName3(Fsk,DstPixelKind,Bytes) == 3 */
			{	FskName3(Fsk,DstPixelKind,Type) pix;
				j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, pix);								/* Convert src to dst format */
				if (!span->isPremul)	FskName2(FskBlend,DstPixelKind)(d, pix, (UInt8)j);			/* Blend and write */
				else	/* premul */	*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j), *d, pix);
			}
			#endif /* FskName3(fsk,DstPixelKind,Bits) == 3 */
		#endif /* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


/********************************************************************************
 * BilerpAffineTexSpanAlphaBlend
 ********************************************************************************/

static void
FskName3(BilerpAffineTexSpanAlphaBlend,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, du, dv, uf, vf;
	SInt32								srb;
	UInt32								i, j, dx;

	s0  = (const char*)(span->baseAddr);
	srb = span->rowBytes;
	d   = (FskName3(Fsk,DstPixelKind,Type)*)(span->p);
	u   = span->u;
	v   = span->v;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	#if defined(USE_WMMX_BILERP565SE) && (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,SrcPixelKind,GreenBits) == 6) && (FskName3(fsk,SrcPixelKind,RedPosition) == 11) && (FskName3(fsk,DstPixelKind,Bytes) != 3)
		FskBilerp565SESetImm_arm_wMMX_s();
	#endif /* USE_WMMX_BILERP565SE */

	for (dx = span->dx; dx--; u += du, v += dv) {
		uf  = (u + (1 << (AFFINE_EXTRA_BITS - 4 - 1))) >> (AFFINE_EXTRA_BITS - 4);					/* Compute the address of the nearest source pixel, and subpixel deviations ... */
		vf  = (v + (1 << (AFFINE_EXTRA_BITS - 4 - 1))) >> (AFFINE_EXTRA_BITS - 4);					/* ... rounding to nearest 1/16 of a pixel */
		i   = uf >> 4;																				/* Integral coordinate */
		j   = vf >> 4;																				/* Integral coordinate */
		uf &= 0xF;																					/* Fractional coordinate */
		vf &= 0xF;																					/* Fractional coordinate */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (SInt32)j * srb + (SInt32)i * FskName3(fsk,SrcPixelKind,Bytes));
		CHECK_SRC_BOUNDS
		#if (FskName3(fsk,DstPixelKind,AlphaBits) >= 4)												/* If dst has a bit enough alpha */
			BILERP_SRC_TO_DST(s, i);																/* Bilinearly interpolate and do the pixel conversions */
			if (!span->isPremul) {
				FskName2(FskAlphaBlend,DstPixelKind)(d, i, span->blendLevel);						/* Alpha blend as dst */
			}
			else {	/* premul */
				i = FskName2(FskAlphaScale,DstPixelKind)(span->blendLevel, i);						/* Scale premultiplied color by the blendLevel */
				FskName2(FskAlphaBlackSourceOver,DstPixelKind)(d, i);								/* Blend over */
			}
		#else 	/* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
			i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);									/* Get src pixel */
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				if (!span->isPremul) {
					j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
					#if FskName3(fsk,SrcPixelKind,AlphaBits) < 8
						j *= 255 / ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);				/* Make sure that the src alpha is 8 bits */
					#endif 	/* FskName3(fsk,SrcPixelKind,AlphaBits) < 8 */
					j = FskAlphaMul(span->blendLevel, (UInt8)j);									/* Scale alpha by the blend level */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);								/* Convert src to dst format */
					#if FskName3(fsk,DstPixelKind,AlphaBits) > 0
						i |= fskFieldMask(FskName3(fsk,DstPixelKind,AlphaBits), FskName3(fsk,DstPixelKind,AlphaPosition))
					#endif 	/* FskName3(fsk,DstPixelKind,AlphaBits) > 0 */
					FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))i, (UInt8)j);
				}
				else {	/* premul */
					UInt32 dst = *d;
					#if FskName3(fsk,SrcPixelKind,RedBits) == 8
						FskName3(fskConvert,DstPixelKind,SrcPixelKind)(dst);
						FskName2(FskAlphaBlackSourceOver,SrcPixelKind)(&dst, FskName2(FskAlphaScale,SrcPixelKind)((UInt8)(span->blendLevel), i));
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(dst);
					#else /* FskName3(fsk,SrcPixelKind,RedBits) < 8 */
						FskName3(fskConvert,SrcPixelKind,32ARGB)(i);
						FskName3(fskConvert,DstPixelKind,32ARGB)(dst);
						FskName2(FskAlphaBlackSourceOver,32ARGB)(&dst, FskName2(FskAlphaScale,32ARGB)((UInt8)(span->blendLevel), i));
						FskName3(fskConvert,32ARGB,DstPixelKind)(dst);
					#endif /* FskName3(fsk,SrcPixelKind,RedBits) < 8 */
					*d = (FskName3(Fsk,DstPixelKind,Type))dst;
				}
			#else 	/* FskName3(fsk,DstPixelKind,Bytes) == 3 */
			{	FskName3(Fsk,DstPixelKind,Type) pix;
				j = (i >> FskName3(fsk,SrcPixelKind,AlphaPosition)) & ((1 << FskName3(fsk,SrcPixelKind,AlphaBits)) - 1);	/* Extract src alpha */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, pix);								/* Convert src to dst format */
				if (!span->isPremul)	FskName2(FskBlend,DstPixelKind)(d, pix, (UInt8)j);			/* Blend and write */
				else	/* premul */	*d = FskName2(FskAXPY,DstPixelKind)((UInt8)(~j), *d, pix);
			}
			#endif 	/* FskName3(fsk,DstPixelKind,Bits) == 3 */
		#endif 	/* (FskName3(fsk,DstPixelKind,AlphaBits) < 4) */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));		/* Increment the destination pixel pointer */
	}
}


#else /* FskName3(fsk,SrcPixelKind,AlphaBits) < 4 */


static void            FskName3(AffineTexSpanAlpha,SrcPixelKind,DstPixelKind)(Span *span) {        FskName3(AffineTexSpanCopy,SrcPixelKind,DstPixelKind)(span); }
static void      FskName3(BilerpAffineTexSpanAlpha,SrcPixelKind,DstPixelKind)(Span *span) {  FskName3(BilerpAffineTexSpanCopy,SrcPixelKind,DstPixelKind)(span); }
static void       FskName3(AffineTexSpanAlphaBlend,SrcPixelKind,DstPixelKind)(Span *span) {       FskName3(AffineTexSpanBlend,SrcPixelKind,DstPixelKind)(span); }
static void FskName3(BilerpAffineTexSpanAlphaBlend,SrcPixelKind,DstPixelKind)(Span *span) { FskName3(BilerpAffineTexSpanBlend,SrcPixelKind,DstPixelKind)(span); }


#endif /* FskName3(fsk,SrcPixelKind,AlphaBits) >= 4 */


#undef SrcPixelKind
#undef DstPixelKind
#undef COPY_SRC_TO_DST
#undef BILERP_SRC_TO_DST
