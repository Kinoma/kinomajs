/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#define __FSKBITMAP_PRIV__	/* To get access to the FskBitmap data structure */

#include "FskLine.h"
#include "FskClipLine2D.h"
#include "FskPixelOps.h"



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***							typedefs and macros							  ***
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


#ifdef IntToFixed
 #undef IntToFixed
#endif /* IntToFixed */
#define RoundFixedToInt(x)	(((x) + 0x8000) >> 16)
#define CeilFixedToInt(x)	(((x) + 0xFFFF) >> 16)
#define FloorFixedToInt(x)	(((x)         ) >> 16)
#define IntToFixed(i)		((i)            << 16)
#define FRACT_ONE			0x40000000




/**********************************************************************************
 * FskDrawClippedJaggedLine
 **********************************************************************************/

void
FskDrawClippedJaggedLine(
	const FskFixedPoint2D	*p0,		/* Start point */
	const FskFixedPoint2D	*p1,		/* End point */
	FskConstColorRGB		frameColor,
	FskBitmap				dstBM
)
{
	register char		*px;
	register SInt32		pip, pin, nx;
	register FskFixed	mu, mip, min;
	FskFixed			dx, dy;
	SInt32				ny, t, i0, j0, i1, j1, rb, pb, dpx, dpy;
	FskFixed			f0, g0;
	FskPixelType		cvtColor;

	FskConvertColorRGBToBitmapPixel(frameColor, dstBM->pixelFormat, &cvtColor);

	dx = p1->x - p0->x;				/* Deltas */
	dy = p1->y - p0->y;
	i0 = RoundFixedToInt(p0->x);	/* Integral endpoints */
	j0 = RoundFixedToInt(p0->y);
	i1 = RoundFixedToInt(p1->x);
	j1 = RoundFixedToInt(p1->y);
	nx = i1 - i0;					/* Integral deltas */
	ny = j1 - j0;
	f0 = IntToFixed(i0) - p0->x;	/* Fractional part */
	g0 = IntToFixed(j0) - p0->y;
	rb = dstBM->rowBytes;			/* Vertical and horizontal pixel increments */
	pb = dstBM->depth >> 3;

	/********************************************************************************
	 * Compute address increments by reflecting into octant 1
	 ********************************************************************************/

  	if (dx < 0) {
		dpx = -pb;
		dx = -dx;
		nx = -nx;
		f0 = -f0;
	}
   	else if (dx == 0)
		dpx = 0;
   	else /* (dx > 0)  */
		dpx = pb;

   	if (dy < 0) {
		dpy = -rb;
		dy = -dy;
		ny = -ny;
		g0 = -g0;
	}
   	else if (dy == 0)
		dpy = 0;
   	else /* (dy > 0) */
		dpy = rb;

	/* Transform coordinates to make dx >= dy */
   	if (dy > dx) {
   		t = dx;	dx = dy;	dy = t;	/* Swap dx, dy */
		t = f0;	f0 = g0;	g0 = t;	/* Swap frac x, y */
		nx = ny;					/* Don't need ny any more */
   		pip = dpx + dpy;
   		pin = dpy;
   	}
   	else {
		pip = dpy + dpx;
		pin = dpx;
	}

	nx++;
   	min = dy << 1;
    mip = min - (dx << 1);
	mu  = (FskFixed)(((FskInt64)f0 * dy - (FskInt64)(g0 << 1) * dx + (1 << 15)) >> 16) + min - dx;
	if (kFskErrNone != FskBitmapWriteBegin(dstBM, NULL, NULL, NULL))
		return;
	px = (char*)(dstBM->bits) + (j0 - dstBM->bounds.y) * rb + (i0 - dstBM->bounds.x) * pb;


	/********************************************************************************
	 * Drawing loops
	 ********************************************************************************/

	switch (pb) {

		case 1: {
			register UInt8 color	= cvtColor.p8;
		   	while (nx--) {
			   	*((UInt8*)px) = color;
		    	if (mu >= 0) {	px += pip;	mu += mip;	}
				else {			px += pin;	mu += min;	}
		   	}
		}	break;

		case 2: {
			register UInt16 color	= cvtColor.p16;
		   	while (nx--) {
			   	*((UInt16*)px) = color;
		    	if (mu >= 0) {	px += pip;	mu += mip;	}
				else {			px += pin;	mu += min;	}
		   	}
		}	break;

		case 3: {
			UInt8 color[3];
			color[0] = cvtColor.p24.c[0];
			color[1] = cvtColor.p24.c[1];
			color[2] = cvtColor.p24.c[2];
		   	while (nx--) {
			   	px[0] = color[0];
			   	px[1] = color[1];
			   	px[2] = color[2];
		    	if (mu >= 0) {	px += pip;	mu += mip;	}
				else {			px += pin;	mu += min;	}
		   	}
		}	break;

		case 4: {
			register UInt32 color	= cvtColor.p32;
		   	while (nx--) {
			   	*((UInt32*)px) = color;
		    	if (mu >= 0) {	px += pip;	mu += mip;	}
				else {			px += pin;	mu += min;	}
			}
		}	break;

	}
	FskBitmapWriteEnd(dstBM);
}
