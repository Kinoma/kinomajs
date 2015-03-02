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
#include "FskClipLine2D.h"
#include "limits.h"

#define T_BITS	24				/* This allows clipping of things 128X larger than the clip window */
#define T_ONE	(1 << T_BITS)


/*******************************************************************************
 * SafeTDivide
 *******************************************************************************/

static FskFract
SafeTDivide(FskFixed n, FskFixed d)
{
	FskInt64 q = ((FskInt64)n << T_BITS) / d;
	if (q > kFskSInt32Max)		q = kFskSInt32Max;
	else if (q < kFskSInt32Min)	q = kFskSInt32Min;
	return (FskFract)q;
}


/********************************************************************************
 * SClipT
 ********************************************************************************/

static long
SClipT(FskFixed p, FskFixed q, FskFract t[2])
{
	FskFract	r;
	long		accept = 1;

	if (p < 0) {
		r = SafeTDivide(q, p);
		if      (r > t[1])	accept = 0;	/* Set up to reject */
		else if (r > t[0])	t[0] = r;
	}
	else if (p > 0) {
		r = SafeTDivide(q, p);
		if      (r < t[0])	accept = 0;	/* Set up to reject */
		else if (r < t[1])	t[1] = r;
	}
	else if (q < 0)			accept = 0;	/* Set up to reject */

	return(accept);
}


/********************************************************************************
 * FskClipLine2D
 ********************************************************************************/

long
FskClipLine2D(FskConstRectangle clipRect, FskFixedPoint2D *p0, FskFixedPoint2D *p1)
{
	FskFract	t[2];
	long		visible	= 0;
	FskFixed	deltaX	= p1->x - p0->x;
	FskFixed	deltaY	= p1->y - p0->y;
#ifdef ORIG_BUT_CRASHES
	FskFixed	minX	= clipRect->x << 16;
	FskFixed	minY	= clipRect->y << 16;
	FskFixed	maxX	= minX + ((clipRect->width  - 1) << 16);
	FskFixed	maxY	= minY + ((clipRect->height - 1) << 16);
#else /* !ORIG_BUT_CRASHES */
	FskFixed	minX	= (clipRect->x << 16) + 4;						/* Add in a little safety factor */
	FskFixed	minY	= (clipRect->y << 16) + 4;
	FskFixed	maxX	= minX + (clipRect->width  << 16) - 0x10004;
	FskFixed	maxY	= minY + (clipRect->height << 16) - 0x10004;
#endif /* !ORIG_BUT_CRASHES */

	if (	(deltaX == 0)	&&	(deltaY == 0)
		&&	(minX <= p0->x) &&	(p0->x <= maxX)
		&&	(minY <= p0->y) &&	(p0->y <= maxY)
	) {
		visible = 1;
	}
	else {
		t[0] = 0;
		t[1] = T_ONE;
		if (	SClipT(-deltaX, p0->x - minX, t)
			&&	SClipT( deltaX, maxX - p0->x, t)
			&&	SClipT(-deltaY, p0->y - minY, t)
			&&	SClipT( deltaY, maxY - p0->y, t)
		) {
			visible = 1;
			if (t[1] < T_ONE) {	p1->x = p0->x + FskFixedNMul(t[1], deltaX, T_BITS);
								p1->y = p0->y + FskFixedNMul(t[1], deltaY, T_BITS);
			} if (t[0] > 0) {	p0->x +=        FskFixedNMul(t[0], deltaX, T_BITS);
								p0->y +=        FskFixedNMul(t[0], deltaY, T_BITS);
			}
		}
	}

	return(visible);
}
