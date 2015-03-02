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

/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Projective Spans							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


#ifndef USE_SPAN_FORWARD_DIFFERENCING


/********************************************************************************
 ********************************************************************************
 *****				Projective Spans with Division at Every Pixel			*****
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * ProjectiveTexSpan
 ********************************************************************************/

static void
FskName3(ProjectiveTexSpan,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, w, du, dv, dw;
	long								srb;
	unsigned long						i, j, dx;

	s0  = span->baseAddr;
	srb = span->rowBytes;
	d   = span->p;
	u   = span->u;
	v   = span->v;
	w   = span->w;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	dw  = span->M[0][2];

	for (dx = span->dx; dx--; u += du, v += dv, w += dw) {
		/* Compute the address of the nearest source pixel */
		i  = FskFixedNDiv(u, w, 8);		/* w has 8 more fractional bits than u & v */
		j  = FskFixedNDiv(v, w, 8);
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));

		/* Do the pixel conversions */
		CHECK_SRC_BOUNDS
		{
		#if (FskName3(fsk,SrcPixelKind,Bytes) == 3) && (FskName3(fsk,DstPixelKind,Bytes) == 3)		/* 24 -> 24 */
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, *d);
		#elif (FskName3(fsk,SrcPixelKind,Bytes) == 3)												/* 24 -> !24 */
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, i);
			*d = (FskName3(Fsk,DstPixelKind,Type))i;
		#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* !24 -> 24 */
			i = (unsigned long)(*s);
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
		#else																						/* !24 -> !24 */
			i = (unsigned long)(*s);
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
			*d = (FskName3(Fsk,DstPixelKind,Type))i;
		#endif
		}

		/* Increment the destination pixel pointer */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));
	}
}


/********************************************************************************
 * BilerpProjectiveTexSpan
 ********************************************************************************/

static void
FskName3(BilerpProjectiveTexSpan,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, w, du, dv, dw, uf, vf;
	long								srb;
	unsigned long						i, j, dx;

	s0  = span->baseAddr;
	srb = span->rowBytes;
	d   = span->p;
	u   = span->u;
	v   = span->v;
	w   = span->w;
	du  = span->M[0][0];
	dv  = span->M[0][1];
	dw  = span->M[0][2];

	for (dx = span->dx; dx--; u += du, v += dv, w += dw) {
		/* Compute the address of the nearest source pixel, and subpixel deviations */
		uf  = FskFixedNDiv(u, w, 8+4);	/* w has 8 more fractional bits than u & v, plus we generate 4 additional subbits for antialiasing */
		vf  = FskFixedNDiv(v, w, 8+4);
		i   = uf >> 4;					/* Split coordinates into integral ... */
		j   = vf >> 4;
		uf &= 0xF;						/* ... and fractional parts */
		vf &= 0xF;
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));

		/* Bilinearly interpolate and do the pixel conversions */
		CHECK_SRC_BOUNDS
		{
		#if (FskName3(fsk,SrcPixelKind,Bytes) == 3)													/* 24 -> xx */
			Fsk24BitType px = FskBilerp24(uf, vf, s, srb);
			#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 24 -> 24 */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, *d);
			#else																					/* 24 -> !24 */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, i);
				*d = (FskName3(Fsk,DstPixelKind,Type))i;
			#endif
		#elif (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,DstPixelKind,Bytes) > 2) && (FskName3(fsk,SrcPixelKind,GreenBits) != 5)	/* 16 (!5515) -> 24 */
			i = FskName3(FskBilerp,SrcPixelKind,32ARGB)(uf, vf, s, srb);
			#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 16 (!5515) -> 24 */
				FskName3(fskConvert,32ARGB,DstPixelKind)(i, *d);
			#else																					/* 16 (!5515) -> !24 */
				FskName3(fskConvert,32ARGB,DstPixelKind)(i);
				*d = (FskName3(Fsk,DstPixelKind,Type))i;
			#endif
		#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* xx --> 24 */
			i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
		#else																						/* !24, 5515, 32 -> !24 */
			i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
			*d = (FskName3(Fsk,DstPixelKind,Type))i;
		#endif
		}

		/* Increment the destination pixel pointer */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));
	}
}


#else /* USE_SPAN_FORWARD_DIFFERENCING */


/********************************************************************************
 ********************************************************************************
 *****			Projective Spans with Quadratic Forward Differencing		*****
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * ProjectiveTexSpan
 ********************************************************************************/

static void
FskName3(ProjectiveTexSpan,SrcPixelKind,DstPixelKind)(register Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	long								srb;
	unsigned long						i, j, dx;

	s0  = span->baseAddr;
	srb = span->rowBytes;
	d   = span->p;


	/********************************************************************************
	 * Do the first pixel separately
	 ********************************************************************************/

	/* Compute the texture cooordinates at the first point */
	span->u0  = FskFixedNDiv(span->u, span->w, 8+16);		/* w has 8 more fractional bits than u & v, plus we generate 16 fractional bits */
	span->v0  = FskFixedNDiv(span->v, span->w, 8+16);
	i   = RoundFixedToInt(span->u0);
	j   = RoundFixedToInt(span->v0);
	s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));


	/* Do the pixel conversions */
	CHECK_SRC_BOUNDS
	{
	#if (FskName3(fsk,SrcPixelKind,Bytes) == 3) && (FskName3(fsk,DstPixelKind,Bytes) == 3)		/* 24 -> 24 */
		FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, *d);
	#elif (FskName3(fsk,SrcPixelKind,Bytes) == 3)												/* 24 -> !24 */
		FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, i);
		*d = (FskName3(Fsk,DstPixelKind,Type))i;
	#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* !24 -> 24 */
		i = (unsigned long)(*s);
		FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
	#else																						/* !24 -> !24 */
		i = (unsigned long)(*s);
		FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
		*d = (FskName3(Fsk,DstPixelKind,Type))i;
	#endif
	}

	/* Increment the destination pixel pointer */
	d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));


	/********************************************************************************
	 * Do the remaining pixels using adaptive quadratic forward differencing
	 ********************************************************************************/

	for (span->dx--; span->dx > 0; ) {
		if (ComputeForwardDifferenceCoefficients(span)) {	/* Use adaptive quadratic forward differencing */
			FskFixed u0, u1, u2, v0, v1, v2;

			/* Gather coefficients into registers */
			u0 = span->u0;
			v0 = span->v0;
			u1 = span->u1;
			v1 = span->v1;
			u2 = span->u2;
			v2 = span->v2;

			for (dx = span->qdx; dx--; ) {

				/* Advance to the next pixel (the first pixel has already been done) */
				u0 += u1 >> QFD_EXTRA_BITS;
				v0 += v1 >> QFD_EXTRA_BITS;
				u1 += u2;
				v1 += v2;

				/* Compute the address of the nearest source pixel */
				i   = RoundFixedToInt(u0);
				j   = RoundFixedToInt(v0);
				s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));

				/* Do the pixel conversions */
				CHECK_SRC_BOUNDS
				{
				#if (FskName3(fsk,SrcPixelKind,Bytes) == 3) && (FskName3(fsk,DstPixelKind,Bytes) == 3)		/* 24 -> 24 */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, *d);
				#elif (FskName3(fsk,SrcPixelKind,Bytes) == 3)												/* 24 -> !24 */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* !24 -> 24 */
					i = (unsigned long)(*s);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
				#else																						/* !24 -> !24 */
					i = (unsigned long)(*s);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#endif
				}

				/* Increment the destination pixel pointer */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));
			}

			/* Update the state machine. Note that span->dx, span->u, span->v, and span->w
			 * have already been updated in ComputeForwardDifferenceCoefficients().
			 */
			span->u0 = span->un;
			span->v0 = span->vn;
		}
		else {	/* Projective interpolation */
			FskFixed u, v, w, du, dv, dw;

			/* Gather coefficients into registers */
			u   = span->u;
			v   = span->v;
			w   = span->w;
			du  = span->M[0][0];
			dv  = span->M[0][1];
			dw  = span->M[0][2];

			if (span->dx)		/* There will be a QFD loop after this */
				span->qdx--;	/* We perform the last pixel separately */

			for (dx = span->qdx; dx--;) {

				/* Advance to the next pixel (the first pixel has already been done) */
				u += du;
				v += dv;
				w += dw;

				/* Compute the address of the nearest source pixel */
				i = FskFixedNDiv(u, w, 8);				/* w has 8 more fractional bits than u & v */
				j = FskFixedNDiv(v, w, 8);				/* FskFixedNDiv() rounds appropriately */
				s = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));

				/* Do the pixel conversions */
				CHECK_SRC_BOUNDS
				{
				#if (FskName3(fsk,SrcPixelKind,Bytes) == 3) && (FskName3(fsk,DstPixelKind,Bytes) == 3)		/* 24 -> 24 */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, *d);
				#elif (FskName3(fsk,SrcPixelKind,Bytes) == 3)												/* 24 -> !24 */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* !24 -> 24 */
					i = (unsigned long)(*s);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
				#else																						/* !24 -> !24 */
					i = (unsigned long)(*s);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#endif
				}

				/* Increment the destination pixel pointer */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));
			}

			if (span->dx) {		/* Last pixel is done a little differently, to set up appropriately for a subsequent QFD subspan */
				/* Advance to the next pixel (the first pixel has already been done) */
				u += du;
				v += dv;
				w += dw;

				/* Compute the address of the nearest source pixel */
				span->u0 = FskFixedNDiv(u, w, 8+16);				/* w has 8 more fractional bits than u & v, plus we generate 16 fractional bits */
				span->v0 = FskFixedNDiv(v, w, 8+16);
				i = FskRoundFixedToInt(span->u0);
				j = FskRoundFixedToInt(span->v0);
				s = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));

				/* Do the pixel conversions */
				CHECK_SRC_BOUNDS
				{
				#if (FskName3(fsk,SrcPixelKind,Bytes) == 3) && (FskName3(fsk,DstPixelKind,Bytes) == 3)		/* 24 -> 24 */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, *d);
				#elif (FskName3(fsk,SrcPixelKind,Bytes) == 3)												/* 24 -> !24 */
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* !24 -> 24 */
					i = (unsigned long)(*s);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
				#else																						/* !24 -> !24 */
					i = (unsigned long)(*s);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#endif
				}

				/* Increment the destination pixel pointer */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));
			}

			/* Update the state machine. Note that span->dx
			 * has already been updated in ComputeForwardDifferenceCoefficients().
			 */
			span->u = u;
			span->v = v;
			span->w = w;
		}
	}
}


/********************************************************************************
 * BilerpProjectiveTexSpan
 ********************************************************************************/

static void
FskName3(BilerpProjectiveTexSpan,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	long								srb;
	unsigned long						i, j, dx;
	FskFixed							uf, vf;

	s0  = span->baseAddr;
	srb = span->rowBytes;
	d   = span->p;


	/********************************************************************************
	 * Do the first pixel separately
	 ********************************************************************************/

	/* Compute the texture cooordinates at the first point */
	span->u0  = FskFixedNDiv(span->u, span->w, 8+16);
	span->v0  = FskFixedNDiv(span->v, span->w, 8+16);
	uf  = span->u0 + 0x800;
	vf  = span->v0 + 0x800;
	i   = TruncFixedToInt(uf);
	j   = TruncFixedToInt(vf);
	uf  = (uf >> (16 - 4)) & 0xF;
	vf  = (vf >> (16 - 4)) & 0xF;
	s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));

	/* Bilinearly interpolate and do the pixel conversions */
	CHECK_SRC_BOUNDS
	{
	#if (FskName3(fsk,SrcPixelKind,Bytes) == 3)													/* 24 -> xx */
		Fsk24BitType px = FskBilerp24(uf, vf, s, srb);
		#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 24 -> 24 */
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, *d);
		#else																					/* 24 -> !24 */
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, i);
			*d = (FskName3(Fsk,DstPixelKind,Type))i;
		#endif
	#elif (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,DstPixelKind,Bytes) > 2) && (FskName3(fsk,SrcPixelKind,GreenBits) != 5)	/* 16 (!5515) -> 24 */
		i = FskName3(FskBilerp,SrcPixelKind,32ARGB)(uf, vf, s, srb);
		#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 16 (!5515) -> 24 */
			FskName3(fskConvert,32ARGB,DstPixelKind)(i, *d);
		#else																					/* 16 (!5515) -> !24 */
			FskName3(fskConvert,32ARGB,DstPixelKind)(i);
			*d = (FskName3(Fsk,DstPixelKind,Type))i;
		#endif
	#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* xx --> 24 */
		i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
		FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
	#else																						/* !24, 5515, 32 -> !24 */
		i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
		FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
		*d = (FskName3(Fsk,DstPixelKind,Type))i;
	#endif
	}

	/* Increment the destination pixel pointer */
	d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));


	/********************************************************************************
	 * Do the remaining pixels using adaptive quadratic forward differencing
	 ********************************************************************************/
	#ifdef USE_WMMX_BILERP565SE
		FskBilerp565SESetImm_arm_wMMX_s();
	#endif /* USE_WMMX_BILERP565SE */
	for (span->dx--; span->dx > 0; ) {
		if (ComputeForwardDifferenceCoefficients(span)) {	/* Use adaptive quadratic forward differencing */
			FskFixed u0, u1, u2, v0, v1, v2;

			/* Gather coefficients into registers */
			u0 = span->u0;
			v0 = span->v0;
			u1 = span->u1;
			v1 = span->v1;
			u2 = span->u2;
			v2 = span->v2;

			for (dx = span->qdx; dx--; ) {

				/* Advance to the next pixel (the first pixel has already been done) */
				u0 += u1 >> QFD_EXTRA_BITS;
				v0 += v1 >> QFD_EXTRA_BITS;
				u1 += u2;
				v1 += v2;

				/* Compute the address of the nearest source pixel */
				uf  = u0 + 0x800;
				vf  = v0 + 0x800;
				i   = TruncFixedToInt(uf);
				j   = TruncFixedToInt(vf);
				uf  = (uf >> (16 - 4)) & 0xF;
				vf  = (vf >> (16 - 4)) & 0xF;
				s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));

				/* Bilinearly interpolate and do the pixel conversions */
				CHECK_SRC_BOUNDS
				{
				#if (FskName3(fsk,SrcPixelKind,Bytes) == 3)													/* 24 -> xx */
					Fsk24BitType px = FskBilerp24(uf, vf, s, srb);
					#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 24 -> 24 */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, *d);
					#else																					/* 24 -> !24 */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, i);
						*d = (FskName3(Fsk,DstPixelKind,Type))i;
					#endif
				#elif (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,DstPixelKind,Bytes) > 2) && (FskName3(fsk,SrcPixelKind,GreenBits) != 5)	/* 16 (!5515) -> 24 */
					i = FskName3(FskBilerp,SrcPixelKind,32ARGB)(uf, vf, s, srb);
					#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 16 (!5515) -> 24 */
						FskName3(fskConvert,32ARGB,DstPixelKind)(i, *d);
					#else																					/* 16 (!5515) -> !24 */
						FskName3(fskConvert,32ARGB,DstPixelKind)(i);
						*d = (FskName3(Fsk,DstPixelKind,Type))i;
					#endif
				#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* xx --> 24 */
					i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
				#elif defined(USE_WMMX_BILERP565SE) && (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,SrcPixelKind,GreenBits) == 6) && (FskName3(fsk,SrcPixelKind,RedPosition) == 11)
					i = FskBilerp565SE_arm_wMMX(uf, vf, (const UInt16*)s, srb);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#else																						/* !24, 5515, 32 -> !24 */
					i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#endif
				}

				/* Increment the destination pixel pointer */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));
			}

			/* Update the state machine. Note that span->dx, span->u, span->v, and span->w
			 * have already been updated in ComputeForwardDifferenceCoefficients().
			 */
			span->u0 = span->un;
			span->v0 = span->vn;
		}
		else {	/* Projective interpolation */
			FskFixed u, v, w, du, dv, dw;

			/* Gather coefficients into registers */
			u   = span->u;
			v   = span->v;
			w   = span->w;
			du  = span->M[0][0];
			dv  = span->M[0][1];
			dw  = span->M[0][2];

			if (span->dx)		/* There will be a QFD loop after this */
				span->qdx--;	/* We perform the last pixel separately */

			for (dx = span->qdx; dx--;) {

				/* Advance to the next pixel (the first pixel has already been done) */
				u += du;
				v += dv;
				w += dw;

				/* Compute the address of the nearest source pixel */
				uf  = FskFixedNDiv(u, w, 8+4);			/* w has 8 extra fractional bits, but we compute 4 additional subbits for bilinear interpolation */
				vf  = FskFixedNDiv(v, w, 8+4);																/* FskFixedNDiv() rounds appropriately */
				i   = uf >> 4;
				j   = vf >> 4;
				uf &= 0xF;
				vf &= 0xF;
				s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));

				/* Bilinearly interpolate and do the pixel conversions */
				CHECK_SRC_BOUNDS
				{
				#if (FskName3(fsk,SrcPixelKind,Bytes) == 3)													/* 24 -> xx */
					Fsk24BitType px = FskBilerp24(uf, vf, s, srb);
					#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 24 -> 24 */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, *d);
					#else																					/* 24 -> !24 */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, i);
						*d = (FskName3(Fsk,DstPixelKind,Type))i;
					#endif
				#elif (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,DstPixelKind,Bytes) > 2) && (FskName3(fsk,SrcPixelKind,GreenBits) != 5)	/* 16 (!5515) -> 24 */
					i = FskName3(FskBilerp,SrcPixelKind,32ARGB)(uf, vf, s, srb);
					#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 16 (!5515) -> 24 */
						FskName3(fskConvert,32ARGB,DstPixelKind)(i, *d);
					#else																					/* 16 (!5515) -> !24 */
						FskName3(fskConvert,32ARGB,DstPixelKind)(i);
						*d = (FskName3(Fsk,DstPixelKind,Type))i;
					#endif
				#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* xx --> 24 */
					i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
				#elif defined(USE_WMMX_BILERP565SE) && (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,SrcPixelKind,GreenBits) == 6) && (FskName3(fsk,SrcPixelKind,RedPosition) == 11)
					i = FskBilerp565SE_arm_wMMX(uf, vf, (const UInt16*)s, srb);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#else
					i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#endif
				}

				/* Increment the destination pixel pointer */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));
			}

			if (span->dx) {		/* Last pixel is done a little differently, to set up appropriately for a subsequent QFD subspan */
				/* Advance to the next pixel */
				u += du;
				v += dv;
				w += dw;

				/* Compute the address of the nearest source pixel */
				span->u0 = FskFixedNDiv(span->u, span->w, 8+16);		/* w has 8 extra fractional bits, but we compute 16 additional fractional bits */
				span->v0 = FskFixedNDiv(span->v, span->w, 8+16);
				uf  = span->u0 + 0x800;
				vf  = span->v0 + 0x800;
				i   = TruncFixedToInt(uf);
				j   = TruncFixedToInt(vf);
				uf  = (uf >> (16 - 4)) & 0xF;
				vf  = (vf >> (16 - 4)) & 0xF;
				s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));

				/* Bilinearly interpolate and do the pixel conversions */
				CHECK_SRC_BOUNDS
				{
				#if (FskName3(fsk,SrcPixelKind,Bytes) == 3)													/* 24 -> xx */
					Fsk24BitType px = FskBilerp24(uf, vf, s, srb);
					#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 24 -> 24 */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, *d);
					#else																					/* 24 -> !24 */
						FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, i);
						*d = (FskName3(Fsk,DstPixelKind,Type))i;
					#endif
				#elif (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,DstPixelKind,Bytes) > 2) && (FskName3(fsk,SrcPixelKind,GreenBits) != 5)	/* 16 (!5515) -> 24 */
					i = FskName3(FskBilerp,SrcPixelKind,32ARGB)(uf, vf, s, srb);
					#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 16 (!5515) -> 24 */
						FskName3(fskConvert,32ARGB,DstPixelKind)(i, *d);
					#else																					/* 16 (!5515) -> !24 */
						FskName3(fskConvert,32ARGB,DstPixelKind)(i);
						*d = (FskName3(Fsk,DstPixelKind,Type))i;
					#endif
				#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* xx --> 24 */
					i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
				#elif defined(USE_WMMX_BILERP565SE) && (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,SrcPixelKind,GreenBits) == 6) && (FskName3(fsk,SrcPixelKind,RedPosition) == 11)
					i = FskBilerp565SE_arm_wMMX(uf, vf, (const UInt16*)s, srb);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#else
					i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
					FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
					*d = (FskName3(Fsk,DstPixelKind,Type))i;
				#endif
				}

				/* Increment the destination pixel pointer */
				d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));
			}

			/* Update the state machine. Note that span->dx
			 * has already been updated in ComputeForwardDifferenceCoefficients().
			 */
			span->u = u;
			span->v = v;
			span->w = w;
		}
	}
}



#endif /* USE_SPAN_FORWARD_DIFFERENCING */




/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Affine Spans							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/




/********************************************************************************
 * AffineTexSpan
 ********************************************************************************/

static void
FskName3(AffineTexSpan,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, du, dv;
	long								srb;
	unsigned long						i, j, dx;

	s0  = span->baseAddr;
	srb = span->rowBytes;
	d   = span->p;
	u   = span->u;
	v   = span->v;
	du  = span->M[0][0];
	dv  = span->M[0][1];

	for (dx = span->dx; dx--; u += du, v += dv) {
		/* Compute the address of the nearest source pixel */
		i   = (u + (1L << (AFFINE_EXTRA_BITS - 1))) >> AFFINE_EXTRA_BITS;	/* Round to nearest pixel */
		j   = (v + (1L << (AFFINE_EXTRA_BITS - 1))) >> AFFINE_EXTRA_BITS;	/* Round to nearest pixel */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));

		/* Do the pixel conversions */
		CHECK_SRC_BOUNDS
		{
		#if (FskName3(fsk,SrcPixelKind,Bytes) == 3) && (FskName3(fsk,DstPixelKind,Bytes) == 3)		/* 24 -> 24 */
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, *d);
		#elif (FskName3(fsk,SrcPixelKind,Bytes) == 3)												/* 24 -> !24 */
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(*s, i);
			*d = (FskName3(Fsk,DstPixelKind,Type))i;
		#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* !24 -> 24 */
			i = (unsigned long)(*s);
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
		#else																						/* !24 -> !24 */
			i = (unsigned long)(*s);
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
			*d = (FskName3(Fsk,DstPixelKind,Type))i;
		#endif
		}

		/* Increment the destination pixel pointer */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));
	}
}


/********************************************************************************
 * BilerpAffineTexSpan
 ********************************************************************************/

static void
FskName3(BilerpAffineTexSpan,SrcPixelKind,DstPixelKind)(Span *span)
{
	const FskName3(Fsk,SrcPixelKind,Type)	*s;
	FskName3(Fsk,DstPixelKind,Type)		*d;
	const char							*s0;
	FskFixed							u, v, du, dv, uf, vf;
	long								srb;
	unsigned long						i, j, dx;

	s0  = span->baseAddr;
	srb = span->rowBytes;
	d   = span->p;
	u   = span->u;
	v   = span->v;
	du  = span->M[0][0];
	dv  = span->M[0][1];

	for (dx = span->dx; dx--; u += du, v += dv) {
		/* Compute the address of the nearest source pixel, and subpixel deviations */
		uf  = (u + (1L << (AFFINE_EXTRA_BITS - 4 - 1))) >> (AFFINE_EXTRA_BITS - 4);	/* Round to nearest 1/16 of a pixel */
		vf  = (v + (1L << (AFFINE_EXTRA_BITS - 4 - 1))) >> (AFFINE_EXTRA_BITS - 4);	/* Round to nearest 1/16 of a pixel */
		i   = uf >> 4;																/* Integral coordinate */
		j   = vf >> 4;																/* Integral coordinate */
		uf &= 0xF;																	/* Fractional coordinate */
		vf &= 0xF;																	/* Fractional coordinate */
		s   = (const FskName3(Fsk,SrcPixelKind,Type)*)(s0 + (long)j * srb + (long)i * FskName3(fsk,SrcPixelKind,Bytes));

		/* Bilinearly interpolate and do the pixel conversions */
		CHECK_SRC_BOUNDS
		{
		#if (FskName3(fsk,SrcPixelKind,Bytes) == 3)													/* 24 -> xx */
			Fsk24BitType px = FskBilerp24(uf, vf, s, srb);
			#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 24 -> 24 */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, *d);
			#else																					/* 24 -> !24 */
				FskName3(fskConvert,SrcPixelKind,DstPixelKind)(px, i);
				*d = (FskName3(Fsk,DstPixelKind,Type))i;
			#endif
		#elif (FskName3(fsk,SrcPixelKind,Bytes) == 2) && (FskName3(fsk,DstPixelKind,Bytes) > 2) && (FskName3(fsk,SrcPixelKind,GreenBits) != 5)	/* 16 (!5515) -> 24 */
			i = FskName3(FskBilerp,SrcPixelKind,32ARGB)(uf, vf, s, srb);
			#if (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* 16 (!5515) -> 24 */
				FskName3(fskConvert,32ARGB,DstPixelKind)(i, *d);
			#else																					/* 16 (!5515) -> !24 */
				FskName3(fskConvert,32ARGB,DstPixelKind)(i);
				*d = (FskName3(Fsk,DstPixelKind,Type))i;
			#endif
		#elif (FskName3(fsk,DstPixelKind,Bytes) == 3)												/* xx --> 24 */
			i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i, *d);
		#else																						/* !24, 5515, 32 -> !24 */
			i = FskName2(FskBilerp,SrcPixelKind)(uf, vf, s, srb);
			FskName3(fskConvert,SrcPixelKind,DstPixelKind)(i);
			*d = (FskName3(Fsk,DstPixelKind,Type))i;
		#endif
		}

		/* Increment the destination pixel pointer */
		d   = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));
	}
}


#undef SrcPixelKind
#undef DstPixelKind
