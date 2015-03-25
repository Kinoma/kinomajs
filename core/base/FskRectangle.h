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
#ifndef __FSKRECTANGLE__
#define __FSKRECTANGLE__

#ifndef __FSK__
# include "Fsk.h"
#endif /* __FSK__ */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	SInt32		x;
	SInt32		y;
} FskPointRecord, *FskPoint;
typedef const FskPointRecord *FskConstPoint;
#define FskPointSet(p, X, Y) do { (p)->x = (X); (p)->y = (Y); } while(0)

typedef struct {
	SInt32		width;
	SInt32		height;
} FskDimensionRecord, *FskDimension;
typedef const FskDimensionRecord *FskConstDimension;
#define FskDimensionSet(d, W, H) do { (d)->width = (W); (d)->height = (H); } while(0)

typedef struct {
	SInt32		x;
	SInt32		y;
	SInt32		width;
	SInt32		height;
} FskRectangleRecord, *FskRectangle;
typedef const FskRectangleRecord *FskConstRectangle;

FskAPI(Boolean)	FskRectanglesDoIntersect(FskConstRectangle r0, FskConstRectangle r1);
FskAPI(Boolean)	FskRectangleIntersect(FskConstRectangle r1, FskConstRectangle r2, FskRectangle ri);
FskAPI(void)	FskRectangleUnion(FskConstRectangle r1, FskConstRectangle r2, FskRectangle ri);
FskAPI(Boolean)	FskRectangleIsEmpty(FskConstRectangle r);
FskAPI(Boolean)	FskRectangleIsEqual(FskConstRectangle r1, FskConstRectangle r2);
FskAPI(void)	FskRectangleOffset(FskRectangle r, SInt32 dx, SInt32 dy);
FskAPI(void)	FskRectangleInset(FskRectangle r, SInt32 dx, SInt32 dy);
FskAPI(void)	FskRectangleSet(FskRectangle r, SInt32 x, SInt32 y, SInt32 width, SInt32 height);
FskAPI(void)	FskRectangleSetFull(FskRectangle r);
FskAPI(void)	FskRectangleSetEmpty(FskRectangle r);
FskAPI(Boolean) FskRectangleContainsPoint(FskConstRectangle r, FskConstPoint p);
FskAPI(Boolean) FskRectangleContainsRectangle(FskConstRectangle outer, FskConstRectangle inner);
FskAPI(void)	FskRectangleScaleToFit(FskConstRectangle containing, FskConstRectangle containee, FskRectangle fitOut);

#ifdef __cplusplus
}
#endif

#endif
