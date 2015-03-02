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
#include "FskRectangle.h"

Boolean FskRectangleIntersect(FskConstRectangle r1, FskConstRectangle r2, FskRectangle ri)
{
	FskRectangleRecord r;
	SInt32 v1, v2;

	// left
	if (r1->x < r2->x)
		r.x = r2->x;
	else
		r.x = r1->x;

	// top
	if (r1->y < r2->y)
		r.y = r2->y;
	else
		r.y = r1->y;

	// right
	v1 = r1->x + r1->width;
	v2 = r2->x + r2->width;

	if ((v1 < r1->x) || (v2 < r2->x))
		r.width = 0;				// wrap around
	else if (v1 < v2)
		r.width = v1 - r.x;
	else
		r.width = v2 - r.x;

	// bottom
	v1 = r1->y + r1->height;
	v2 = r2->y + r2->height;

	if ((v1 < r1->y) || (v2 < r2->y))
		r.height = 0;				// wrap around
	else if (v1 < v2)
		r.height= v1 - r.y;
	else
		r.height = v2 - r.y;

	v1 = 1;											/* Set the return value assuming intersection */
	if ((r.width <= 0) || (r.height <= 0)) {		/* If either width or height is null... */
		r.width  = 0;								/* ... then the whole rectangle is null ... */
		r.height = 0;								/* ... so lets assure it. */
		v1 = 0;										/* Indicate that the rectangles do not intersect */
	}

	// finish up
	*ri = r;										/* Copy the local result to the result parameter, guaranteeing that it works in-place */

	return (Boolean)v1;								/* Indicate whether or not there were intersections */
}

void FskRectangleUnion(FskConstRectangle r1, FskConstRectangle r2, FskRectangle ri)
{
	FskRectangleRecord r;
	SInt32 v1, v2;

	// deal with degenerate cases first
	if ((0 == r1->width) || (0 == r1->height))
		*ri = *r2;
	else
	if ((0 == r2->width) || (0 == r2->height))
		*ri = *r1;
	else {
		// neither rectangle is empty, so do some work

		// left
		if (r1->x > r2->x)
			r.x = r2->x;
		else
			r.x = r1->x;

		// top
		if (r1->y > r2->y)
			r.y = r2->y;
		else
			r.y = r1->y;

		// right
		v1 = r1->x + r1->width;
		v2 = r2->x + r2->width;
		if (v1 > v2)
			r.width = v1 - r.x;
		else
			r.width = v2 - r.x;

		// bottom
		v1 = r1->y + r1->height;
		v2 = r2->y + r2->height;
		if (v1 > v2)
			r.height= v1 - r.y;
		else
			r.height = v2 - r.y;

		if ((r.width <= 0) || (r.height <= 0)) {	/* Fruitless paranoia: ... */
			r.width  = 0;							/* ... All the cases that should have caused this ... */
			r.height = 0;							/* ... have been already taken care of above ... */
		}											/* ... unless input width or height were less than zero */

		// finish up
		*ri = r;									/* Copy the local result to the result parameter, guaranteeing that it works in-place */
	}
}

Boolean FskRectangleIsEmpty(FskConstRectangle r)
{
	return (0 == r->width) || (0 == r->height);
}

Boolean FskRectangleIsEqual(FskConstRectangle r1, FskConstRectangle r2)
{
	return	(r1->x == r2->x) &&
			(r1->y == r2->y) &&
			(r1->width == r2->width) &&
			(r1->height == r2->height);
}

void FskRectangleOffset(FskRectangle r, SInt32 dx, SInt32 dy)
{
	r->x += dx;
	r->y += dy;
}

void FskRectangleInset(FskRectangle r, SInt32 dx, SInt32 dy)
{
	r->x += dx;
	r->width -= dx + dx;
	if (r->width < 0)
		r->width = 0;

	r->y += dy;
	r->height -= dy + dy;
	if (r->height < 0)
		r->height = 0;
}

void FskRectangleSet(FskRectangle r, SInt32 x, SInt32 y, SInt32 width, SInt32 height)
{
	r->x = x;
	r->y = y;
	r->width = width;
	r->height = height;
}

void FskRectangleSetFull(FskRectangle r)
{
	r->x = -((SInt32)0x20000000);
	r->y = r->x;
	r->width = 0x3fffffff;
	r->height = r->width;
}

void FskRectangleSetEmpty(FskRectangle r)
{
	r->x = 0;
	r->y = 0;
	r->width = 0;
	r->height = 0;
}

Boolean FskRectangleContainsPoint(FskConstRectangle r, FskConstPoint p)
{
	Boolean result = false;

	if ((r->x <= p->x) && (p->x < (r->x + r->width))) {
		if ((r->y <= p->y) && (p->y < (r->y + r->height)))
			result = true;
	}

	return result;
}

void FskRectangleScaleToFit(FskConstRectangle containing, FskConstRectangle containee, FskRectangle fitOut)
{
	FskRectangleRecord fit;

	if ((0 == containee->height) || (0 == containee->width)) {
		FskRectangleSetEmpty(fitOut);
		return;
	}

	fit.width = containing->width;
	fit.height = (containee->height * containing->width) / containee->width;
	if (fit.height > containing->height) {
		fit.height = containing->height;
		fit.width = (containee->width * containing->height) / containee->height;
	}

	fit.x = (containing->width - fit.width) / 2;
	fit.y = (containing->height - fit.height) / 2;
	FskRectangleOffset(&fit, containing->x, containing->y);

	*fitOut = fit;
}


/********************************************************************************
 * FskRectanglesDoIntersect
 * Returns true only if the rectangles have non-null overlap.
 * In particular, abutting rectangles return false.
 ********************************************************************************/

Boolean FskRectanglesDoIntersect(FskConstRectangle r0, FskConstRectangle r1)
{
	long d;

	return (	(((d = r1->x - r0->x) < 0) ? (r1->width  > -d) : (r0->width  > d))
			&&	(((d = r1->y - r0->y) < 0) ? (r1->height > -d) : (r0->height > d))
	);
}


/********************************************************************************
 * FskRectangleContainsRectangle
 * Returns true if the inner rectangle is wholly contained within the outer rectangle,
 * i.e. Intersect(inner, outer) == inner.
 ********************************************************************************/

Boolean FskRectangleContainsRectangle(FskConstRectangle outer, FskConstRectangle inner) {
	return	outer->x <= inner->x										&&
			outer->y <= inner->y										&&
			(outer->x + outer->width ) >= (inner->x + inner->width )	&&
			(outer->y + outer->height) >= (inner->y + inner->height);
}
