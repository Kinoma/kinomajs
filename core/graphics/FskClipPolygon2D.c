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
#include "FskClipPolygon2D.h"

#include "limits.h"

#define	POSITIVE_INFINITY	kFskSInt32Max
#define	NEGATIVE_INFINITY	kFskSInt32Min
#define T_BITS				24
#define T_ONE				(1 << T_BITS)


/*******************************************************************************
 * SafeTDivide
 *******************************************************************************/

static FskFract
SafeTDivide(FskFixed n, FskFixed d)
{
	FskInt64 q = ((FskInt64)n << T_BITS) / d;
	if (q > kFskSInt32Max)		q = kFskSInt32Max;
	else if (q < (int)kFskSInt32Min)	q = (int)kFskSInt32Min;
	return (FskFract)q;
}



/*******************************************************************************
 * FskClipPolygon2D
 *
 * Barsky-Liang method.
 *
 * The version of the algorithm in the '83 paper by Liang is wrong.
 * The version here is that which is found in the second edition of Foley, vanDam, et al.
 *******************************************************************************/


#define AppendVertex(a, b)		( vOut->x = (a), vOut->y = (b), vOut++, nOutCount++		)


void
FskClipPolygon2D(
	UInt32						nIn,
	register FskFixedPoint2D	*vIn,
	FskConstRectangle			clip,
	UInt32						*nOut,
	FskFixedPoint2D				*vOut
)
{
	FskFixed		xIn, xOut, yIn, yOut;		/* Coordinates of entry and exit points */
	FskFract		tOut1, tIn2, tOut2;			/* Parameter values of same */
	FskFract		tInX, tOutX, tInY, tOutY;	/* Parameter values for intersections */
	FskFixed		deltaX, deltaY;				/* Direction of edge */
	FskFixed		xMax, xMin, yMin, yMax;		/* Rectangular clipping boundaries */
	register SInt32	i;
	UInt32			nOutCount;

	nOutCount = 0;
	
	/* We need to assure that the first point is duplicated as the last. */
	vIn[nIn] = vIn[0];									/* Structure assignment */
	
	/* Initialize fixed-point clip values */
	xMin	= (clip->x << 16) - (1 << 15);	/* from 0.5 to N-0.5 */
	yMin	= (clip->y << 16) - (1 << 15);
	xMax	= xMin + ((clip->width ) << 16);
	yMax	= yMin + ((clip->height) << 16);

	for (i = nIn; i-- > 0; vIn++) {						/* Edge V[i]V[i+1] */
		/* Determine X direction of edge */
		if (((deltaX = (vIn + 1)->x - vIn->x) > 0) || ((deltaX == 0) && (vIn->x > xMax)) )
				{	xIn = xMin;	xOut = xMax;	}		/* l[i] points right */
		else	{	xIn = xMax;	xOut = xMin;	}		/* l[i] points left */
		
		if (deltaX != 0)								tOutX = SafeTDivide(xOut - vIn->x, deltaX);
		else if ((vIn->x <= xMax) && (xMin <= vIn->x))	tOutX = POSITIVE_INFINITY;
		else											tOutX = NEGATIVE_INFINITY;

		/* Determine Y direction of edge */
		if (((deltaY = (vIn + 1)->y - vIn->y) > 0) || ((deltaY == 0) && (vIn->y > yMax)) )
				{	yIn = yMin;	yOut = yMax;	}		/* l[i] points up */
		else	{	yIn = yMax;	yOut = yMin;	}		/* l[i] points down */

		if (deltaY != 0)								tOutY = SafeTDivide(yOut - vIn->y, deltaY);
		else if ((vIn->y <= yMax) && (yMin <= vIn->y))	tOutY = POSITIVE_INFINITY;
		else											tOutY = NEGATIVE_INFINITY;

		/* Order the two exit points */
		if (tOutX < tOutY)	{	tOut1 = tOutX;	tOut2 = tOutY;	}	/* First exit at x, then y */
		else				{	tOut1 = tOutY;	tOut2 = tOutX;	}	/* First exit at y, then x */

		
		if (tOut2 > 0) {								/* There could be output -- compute tIn2 */
			if (deltaX != 0)	tInX = SafeTDivide(xIn - vIn->x, deltaX);
			else				tInX = NEGATIVE_INFINITY;
			if (deltaY != 0)	tInY = SafeTDivide(yIn - vIn->y, deltaY);
			else				tInY = NEGATIVE_INFINITY;
			if (tInX < tInY)	tIn2 = tInY;
			else				tIn2 = tInX;
			
			if (tOut1 < tIn2) {							/* No visible segment */
				if ((0 < tOut1) && (tOut1 <= T_ONE)) {	/* Line crosses over intermediate corner region */
					if (tInX < tInY)				AppendVertex(xOut, yIn);
					else							AppendVertex(xIn, yOut);
				}
			}
			else {										/* Line crosses through window */
				if ((0 < tOut1) && (tIn2 <= T_ONE)) {
					if (0 < tIn2) {						/* Visible segment */
						if (tInX > tInY)			AppendVertex(xIn, vIn->y + FskFixedNMul(tInX, deltaY, T_BITS));
						else						AppendVertex(vIn->x + FskFixedNMul(tInY, deltaX, T_BITS), yIn);
					}
					if (T_ONE > tOut1) {
						if (tOutX < tOutY)			AppendVertex(xOut, vIn->y + FskFixedNMul(tOutX, deltaY, T_BITS));
						else						AppendVertex(vIn->x + FskFixedNMul(tOutY, deltaX, T_BITS), yOut);
					}
					else							AppendVertex((vIn + 1)->x, (vIn + 1)->y);
				}
			}
			if ((0 < tOut2) && (tOut2 <= T_ONE))	AppendVertex(xOut, yOut);
		}
	}

	*nOut = nOutCount;
}
