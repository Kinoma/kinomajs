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
#define YUVYUV_BLIT_DECL		UInt32		width	= params->dstWidth,		height	= params->dstHeight,	i, j;				\
								SInt32		srb		= params->srcRowBytes,	drb		= params->dstRowBytes,	k;					\
								FskFixed	xd		= params->srcXInc,		yd		= params->srcYInc,		x0, x, y, xC, yC;	\
								FskFixed	xCmax	= ((params->srcWidth  >> 1) - 1) << FWD_SUBBITS;							\
								FskFixed	yCmax	= ((params->srcHeight >> 1) - 1) << FWD_SUBBITS;							\
								const UInt8	*sY0	= (const UInt8*)(params->srcBaseAddr ),		*sYr, *sY;						\
								const UInt8	*sU0	= (const UInt8*)(params->srcUBaseAddr),		*sUr, *sU;						\
								const UInt8	*sV0	= (const UInt8*)(params->srcVBaseAddr),		*sVr, *sV;						\
								UInt8		*dY0	= (UInt8*)(params->dstBaseAddr),			*dY;							\
								UInt8		*dU0	= (UInt8*)(params->dstUBaseAddr),			*dU;							\
								UInt8		*dV0	= (UInt8*)(params->dstVBaseAddr),			*dV			/* YUVYUV_BLIT_DECL */
#define YUVYUV_BLIT_STARTYLOOP	for (i = height, y = params->srcY0, x0 = params->srcX0; i--; y += yd, dY0 += drb) { sYr = sY0 + (y >> FWD_SUBBITS) * srb;	\
								for (j = width, x = x0, dY = dY0; j--; x += xd, dY++) { sY = sYr + (x >> FWD_SUBBITS);							/* YUVYUV_BLIT_STARTYLOOP */
#define YUVYUV_BLIT_STARTCLOOP	for (i = (height >>= 1), width >>= 1, srb >>= 1, drb >>= 1, y = ((params->srcY0 + ((yd - FWD_ONE) >> 1)) >> 1),				\
								x0 = ((params->srcX0 + ((xd - FWD_ONE) >> 1)) >> 1); i--; y += yd, dU0 += drb, dV0 += drb) {								\
								if ((yC = y) < 0) yC = 0; else if (yC > yCmax) yC = yCmax; sUr = sU0 + (k = (yC >> FWD_SUBBITS) * srb); sVr = sV0 + k;		\
								for (j = width, x = x0, dU = dU0, dV = dV0; j--; x += xd, dU++, dV++) {														\
								if ((xC = x) < 0) xC =0; else if (xC > xCmax) xC = xCmax; sU = sUr + (k = (xC >> FWD_SUBBITS)); sV = sVr + k;	/* YUVYUV_BLIT_STARTCLOOP */

#define YUVYUV_BLIT_ENDYLOOP	}}																											/* YUV_BLIT_ENDLOOP */
#define YUVYUV_BLIT_ENDCLOOP	}}																											/* YUV_BLIT_ENDLOOP */

#define Bilerp8Inline(i, j, s, r, t, d)	(d = s[0], d = (d<<4)+(s[1]-d)*(i), t=s[r], t = (t<<4)+(s[r+1]-t)*(i), d = (((d<<4)+(t-d)*(j) + (1 << 7)) >> 8))


/********************************************************************************
 ********************************************************************************
 **		Rect Procs
 ********************************************************************************
 ********************************************************************************/




/********************************************************************************
 * Copy
 ********************************************************************************/

static void
FskCopyYUV420YUV420(const FskRectBlitParams *params)
{
	YUVYUV_BLIT_DECL;
	YUVYUV_BLIT_STARTYLOOP
		*dY = *sY;
	YUVYUV_BLIT_ENDYLOOP

	YUVYUV_BLIT_STARTCLOOP
		*dU = *sU;
		*dV = *sV;
	YUVYUV_BLIT_ENDCLOOP
}


#define OPTIMIZED_YUV
#ifndef OPTIMIZED_YUV

/********************************************************************************
 * Bilinear Copy
 ********************************************************************************/

static void
FskBilinearCopyYUV420YUV420(const FskRectBlitParams *params)
{
	YUVYUV_BLIT_DECL;
	YUVYUV_BLIT_STARTYLOOP
		*dY = FskBilerp8(xf, yf, sY, srb);
	YUVYUV_BLIT_ENDYLOOP

	YUVYUV_BLIT_STARTCLOOP
		*dU = FskBilerp8(xf, ycf, sU, srb);
		*dV = FskBilerp8(xf, ycf, sV, srb);
	YUVYUV_BLIT_ENDCLOOP
}

#else /* OPTIMIZED_YUV */

static void
FskBilinearCopyYUV420YUV420(const FskRectBlitParams *params)
{
	SInt32		width	= params->dstWidth,		height	= params->dstHeight,	i, j;
	SInt32		srb		= params->srcRowBytes,	drb		= params->dstRowBytes,	k;
	FskFixed	xd		= params->srcXInc,		yd		= params->srcYInc,		x0, x, y, xC, yC;
	FskFixed	xCmax	= ((params->srcWidth  >> 1) - 1) << FWD_SUBBITS;
	FskFixed	yCmax	= ((params->srcHeight >> 1) - 1) << FWD_SUBBITS;
	const UInt8	*sY0	= (const UInt8*)(params->srcBaseAddr ),		*sYr, *sY;
	const UInt8	*sU0	= (const UInt8*)(params->srcUBaseAddr),		*sUr, *sU;
	const UInt8	*sV0	= (const UInt8*)(params->srcVBaseAddr),		*sVr, *sV;
	UInt8		*dY0	= (UInt8*)(params->dstBaseAddr),			*dY;
	UInt8		*dU0	= (UInt8*)(params->dstUBaseAddr),			*dU;
	UInt8		*dV0	= (UInt8*)(params->dstVBaseAddr),			*dV;

	SInt32		tl, dl;	/* This is used for inlining the bilerps */
	SInt32		overhang;


	/****************
	 * Y processing *
	 ****************/

	/* All but the last line */
	for (i = height - 1, y = params->srcY0, x0 = params->srcX0; i--; y += yd, dY0 += drb) {
		sYr = sY0 + (y >> FWD_SUBBITS) * srb;
		for (j = width, x = x0, dY = dY0; j--; x += xd, dY++) {
			sY = sYr + (x >> FWD_SUBBITS);

			*dY = (UInt8)Bilerp8Inline(xf, yf, sY, srb, tl, dl);	/* Unguarded interpolation */
		}
	}

	/* Last line except last pixel */
	sYr = sY0 + (y >> FWD_SUBBITS) * srb;
	for (j = width - 1, x = x0, dY = dY0; j--; x += xd, dY++) {
		sY = sYr + (x >> FWD_SUBBITS);

		*dY = (UInt8)Bilerp8Inline(xf, yf, sY, srb, tl, dl);		/* Unguarded interpolation */
	}

	/* Last pixel */
	*dY = FskBilerp8(xf, yf, sY, srb);						/* Guarded interpolation at the only place it would have caused a memory fault */


	/**********************
	 * U and V processing *
	 **********************/

	/* Pre compute X intervals */
	height >>= 1;
	width  >>= 1;
	srb    >>= 1;
	drb    >>= 1;
	overhang = (yd - FWD_ONE) >> 2;
	if (overhang >= 0) {									/* No overhang worries */
		overhang = 0;
	}
	else {													/* Need to worry about overhang */
		if (yd == 0) {										/* Degenerate */
			overhang = width;								/* Use guarded interpolation everywhere */
			width = 0;
		}
		else {
			overhang = overhang / yd + 1;
			if ((width -= (overhang << 1)) <= 0) {			/* Too narrow */
				overhang = (overhang << 1) + width;			/* Use guarded interpolation everywhere */
				width = 0;
			}												/* else: overhang + width + overhang */
		}
	}
	y  = ((params->srcY0 + ((yd - FWD_ONE) >> 1)) >> 1);
	x0 = ((params->srcX0 + ((xd - FWD_ONE) >> 1)) >> 1);

	/* The first few lines */
	for (i = height; (y < 0) && i--; y += yd, dU0 += drb, dV0 += drb) {
		yC = 0;		/* Clamp the vertical to 0 */
		sUr = sU0;
		sVr = sV0;
		x   = x0;
		dU  = dU0;
		dV  = dV0;

		/* First few pixels of the first few lines */
		for (j = overhang; j--; x += xd, dU++, dV++) {
			if ((xC = x) < 0)		xC = 0;						/* We need to check both ends in case the interval is short */
			else if (xC > xCmax)	xC = xCmax;
			sU = sUr + (k = (xC >> FWD_SUBBITS));
			sV = sVr + k;

			*dU = FskBilerp8(xcf, 0, sU, srb);					/* Guarded interpolation */
			*dV = FskBilerp8(xcf, 0, sV, srb);
		}

		/* The remainder of the first line */
		if (width) {
			/* Middle pixels of the first few lines */
			for (j = width; j--; x += xd, dU++, dV++) {
				xC = x;
				sU = sUr + (k = (xC >> FWD_SUBBITS));
				sV = sVr + k;

				*dU = (UInt8)Bilerp8Inline(xcf, 0, sU, srb, tl, dl);	/* Unguarded interpolation */
				*dV = (UInt8)Bilerp8Inline(xcf, 0, sV, srb, tl, dl);
			}


			/* Last few pixels of the first few lines */
			for (j = overhang; j--; x += xd, dU++, dV++) {
				if (xC > xCmax)	xC = xCmax;
				sU = sUr + (k = (xC >> FWD_SUBBITS));
				sV = sVr + k;

				*dU = FskBilerp8(xcf, 0, sU, srb);				/* Guarded interpolation */
				*dV = FskBilerp8(xcf, 0, sV, srb);
			}
		}
	}

	/* The middle lines */
	for ( ; (y < yCmax) && i--; y += yd, dU0 += drb, dV0 += drb) {
		yC = y;
		sUr = sU0 + (k = (yC >> FWD_SUBBITS) * srb);
		sVr = sV0 + k;
		x   = x0;
		dU  = dU0;
		dV  = dV0;

		/* First few pixels of the middle lines */
		for (j = overhang; j--; x += xd, dU++, dV++) {
			if ((xC = x) < 0)		xC = 0;						/* We need to check both ends in case the interval is short */
			else if (xC > xCmax)	xC = xCmax;
			sU = sUr + (k = (xC >> FWD_SUBBITS));
			sV = sVr + k;

			*dU = FskBilerp8(xcf, ycf, sU, srb);				/* Guarded interpolation */
			*dV = FskBilerp8(xcf, ycf, sV, srb);
		}

		/* Remaining pixels of the middle lines */
		if (width) {
			/* Middle pixels of the middle lines */
			for (j = width; j--; x += xd, dU++, dV++) {
				xC = x;
				sU = sUr + (k = (xC >> FWD_SUBBITS));
				sV = sVr + k;

				*dU = (UInt8)Bilerp8Inline(xcf, ycf, sU, srb, tl, dl);	/* Unguarded interpolation */
				*dV = (UInt8)Bilerp8Inline(xcf, ycf, sV, srb, tl, dl);
			}


			/* Last few pixels of the middle lines */
			for (j = overhang; j--; x += xd, dU++, dV++) {
				if (xC > xCmax)	xC = xCmax;
				sU = sUr + (k = (xC >> FWD_SUBBITS));
				sV = sVr + k;

				*dU = FskBilerp8(xcf, ycf, sU, srb);			/* Guarded interpolation */
				*dV = FskBilerp8(xcf, ycf, sV, srb);
			}
		}
	}

	/* The last few lines */
	for (sUr = sU0 + (k = (yCmax >> FWD_SUBBITS) * srb), sVr = sV0 + k ; i-- > 0; y += yd, dU0 += drb, dV0 += drb) {
		yC = yCmax;
		sUr = sU0 + (k = (yC >> FWD_SUBBITS) * srb);
		sVr = sV0 + k;
		x   = x0;
		dU  = dU0;
		dV  = dV0;

		/* First few pixels of the last lines */
		for (j = overhang; j--; x += xd, dU++, dV++) {
			if ((xC = x) < 0)		xC = 0;						/* We need to check both ends in case the interval is short */
			else if (xC > xCmax)	xC = xCmax;
			sU = sUr + (k = (xC >> FWD_SUBBITS));
			sV = sVr + k;

			*dU = FskBilerp8(xcf, 0, sU, srb);					/* Guarded interpolation */
			*dV = FskBilerp8(xcf, 0, sV, srb);
		}

		/* Remaining pixels of the last lines */
		if (width) {
			/* Middle pixels of the middle lines */
			for (j = width; j--; x += xd, dU++, dV++) {
				xC = x;
				sU = sUr + (k = (xC >> FWD_SUBBITS));
				sV = sVr + k;

				*dU = (UInt8)Bilerp8Inline(xcf, 0, sU, srb, tl, dl);	/* Unguarded interpolation */
				*dV = (UInt8)Bilerp8Inline(xcf, 0, sV, srb, tl, dl);
			}


			/* Last few pixels of the last lines */
			for (j = overhang; j--; x += xd, dU++, dV++) {
				if (xC > xCmax)	xC = xCmax;
				sU = sUr + (k = (xC >> FWD_SUBBITS));
				sV = sVr + k;

				*dU = FskBilerp8(xcf, 0, sU, srb);				/* Guarded interpolation */
				*dV = FskBilerp8(xcf, 0, sV, srb);
			}
		}
	}
}

#endif /* OPTIMIZED_YUV */


/********************************************************************************
 * Blend
 ********************************************************************************/

static void
FskBlendYUV420YUV420(const FskRectBlitParams *params)
{
}


/********************************************************************************
 * Bilinear Blend
 ********************************************************************************/

static void
FskBilinearBlendYUV420YUV420(const FskRectBlitParams *params)
{
}


/********************************************************************************
 * TintCopy
 ********************************************************************************/

static void
FskTintCopyYUV420YUV420(const FskRectBlitParams *params)
{
}


/********************************************************************************
 * Bilinear TintCopy
 ********************************************************************************/

static void
FskBilinearTintCopyYUV420YUV420(const FskRectBlitParams *params)
{
}


/********************************************************************************/


#define FskAlphaYUV420YUV420				FskCopyYUV420YUV420
#define FskBilinearAlphaYUV420YUV420		FskBilinearCopyYUV420YUV420
#define FskAlphaBlendYUV420YUV420			FskBlendYUV420YUV420
#define FskBilinearAlphaBlendYUV420YUV420	FskBilinearBlendYUV420YUV420
