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
#include "FskDHClipPolygon.h"



/****************************************************************
 * Polygon clipping
 ****************************************************************/

#define SpaceCode(p) (	((int)(p->x < -p->z) << 0) | ((int)(p->x > p->z) << 1) | \
				 		((int)(p->y < -p->z) << 2) | ((int)(p->y > p->z) << 3) )

#define DoesCrossPlane(x, y)	( ( ((x) > 0) && ((y) < 0)) || ( ((x) < 0) && ((y) > 0) ) )


/****************************************************************/

void
FskDHTrivialClipPolygon(register int nPts, register FskDVector3D *p, int *inCode, int *outCode)
{
	register int code, in, out;

	for (in = FSKDHCLIP_ALLIN, out = FSKDHCLIP_ALLOUT; nPts--; p++) {
		code = SpaceCode(p);
		in  |= code;
		out &= code;
	}
	*inCode  = in;
	*outCode = out;
}


/****************************************************************
 * ClipPolygonAgainstPlane
 ****************************************************************/

static void
ClipPolygonAgainstPlane(int planeID, int *nPts, FskDVector3D *pts0, FskDVector3D *pts1)
{
	int		k, n0, n1, nOut, numPts;
	double	d, d0;

	if ((numPts = *nPts) < 3)
		return;		/* No longer a valid polygon */

	/* Check every point in the original point list */
	for (k = 0, nOut = 0, n1 = 0, d = 0, d0 = 0; k <= numPts; k++, d0 = d, n1 = n0) {
		n0 = (k == numPts) ? 0 : k;
		switch (planeID) {
			case FSKDHCLIP_LEFT:		d = pts0[n0].z + pts0[n0].x;	break;
			case FSKDHCLIP_RIGHT:	d = pts0[n0].z - pts0[n0].x;	break;
			case FSKDHCLIP_TOP:		d = pts0[n0].z + pts0[n0].y;	break;
			case FSKDHCLIP_BOTTOM:	d = pts0[n0].z - pts0[n0].y;	break;
		}

		/* First point, do nothing */
		if (k == 0)
			continue;

		/* Edge crosses the n0 clipping Plane */
		if (DoesCrossPlane(d0, d)) {			/* Crosses: interpolate (x,y,z) */
			double t = d / (d - d0);
			pts1[nOut].x = (pts0[n1].x - pts0[n0].x) * t + pts0[n0].x;
			pts1[nOut].y = (pts0[n1].y - pts0[n0].y) * t + pts0[n0].y;
			pts1[nOut].z = (pts0[n1].z - pts0[n0].z) * t + pts0[n0].z;
			nOut++;								/* Increment the output point count */
		}

		/* The current point is visible */
		if (d >= 0) {
			pts1[nOut] = pts0[n0];				/* Copy the point over */
			nOut++;
		}
	}

	*nPts = nOut;								/* Set the resulting number of points */
}


/****************************************************************
 * ClipPolygonAgainstPlanes
 * 		returns 0 if poly0 has the result, 1 if poly1, and -1 if nothing left.
 ****************************************************************/

static int
ClipPolygonAgainstPlanes(int *nPts, FskDVector3D *pts0, FskDVector3D *pts1, int in)
{
	int clipTotal = 0;
	int planeID;

	for (planeID = 0; planeID < FSKDHCLIP_PLANES; planeID++) {
		if (in & (1 << planeID)) {
			if (clipTotal & 1)	ClipPolygonAgainstPlane(planeID, nPts, pts1, pts0);
			else				ClipPolygonAgainstPlane(planeID, nPts, pts0, pts1);
			if (*nPts < 3)
				return(-1);
			clipTotal++;
		}
	}

	return(clipTotal & 1);
}


/****************************************************************
 * FskDHClipPolygon
 ****************************************************************/

void
FskDHClipPolygon(int *nPts, FskDVector3D *pts, FskDVector3D *temp, const FskRect *r)
{
	int			inCode, outCode, i;
	double		x0, y0, dx, dy, idx, idy;
	register	FskDVector3D *p;

	/* Convert to canonical coordinates */
	dx  = (r->xMax - r->xMin) * 0.5;	/* Half width */
	dy  = (r->yMax - r->yMin) * 0.5;	/* Half height */
	x0  = r->xMin + dx - 0.5;			/* Center x */
	y0  = r->yMin + dy - 0.5;			/* Center y */
	idx = 1.0 / dx;						/* Multiplications are usually faster than divisions */
	idy = 1.0 / dy;

	for (i = *nPts, p = pts; i--; p++) {
		p->x = (p->x - x0 * p->z) * idx;
		p->y = (p->y - y0 * p->z) * idy;
	}

	/* Do a trivial clip test */
	FskDHTrivialClipPolygon(*nPts, pts, &inCode, &outCode);

	if (outCode == 0) {
		if (inCode != FSKDHCLIP_ALLIN) {	/* Need to clip */
			int clipCode;
			clipCode = ClipPolygonAgainstPlanes(nPts, pts, temp, inCode);
			switch (clipCode) {
				case -1:				/* Totally clipped out */
					*nPts = 0;
					break;
				case 0:					/* All done */
					break;
				case 1:					/* Move result from temp to pts */
					for (i = *nPts, p = pts; i--; )
						*p++ = *temp++;
					break;
			}
		}

		/* Convert back from canonical coordinates */
		for (i = *nPts, p = pts; i--; p++) {
			p->x = p->x * dx + p->z * x0;
			p->y = p->y * dy + p->z * y0;
		}
	}
	else {
		*nPts = 0;
	}
}
