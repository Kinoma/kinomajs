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
/**
	\file	FskPath.h
	\brief	Fixed Point Path Element Definitions.
*/
#ifndef __FSKPATH__
#define __FSKPATH__

#ifndef __FSKFIXEDMATH__
# include "FskFixedMath.h"
#endif /* __FSKFIXEDMATH__ */

#ifndef __FSKGROWABLEFIXEDPOINT2DARRAY__
# include "FskGrowableFixedPoint2DArray.h"
#endif /* __FSKGROWABLEFIXEDPOINT2DARRAY__ */

#ifndef __FSKPOLYGON__
# include "FskPolygon.h"
#endif /* __FSKPOLYGON__ */

#ifndef __FSKGRAPHICS_H__
# include "FskGraphics.h"
#endif /* __FSKGRAPHICS_H__ */

#ifndef __FSKBITMAP__
# include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifndef __FSKRECTANGLE__
# include "FskRectangle.h"
#endif /* __FSKRECTANGLE__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////																		////
////							Path Segments								////
////																		////
////	These naked data structures are intended to be embedded into		////
////	either a tagged linked list or a tagged heterogeneous array.		////
////																		////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


/* Punctuation */

//typedef struct FskPathSegmentClose {
//} FskPathSegmentClose;

//typedef struct FskPathSegmentEndGlyph {
//} FskPathSegmentClose;

/** Move to */
typedef struct FskPathSegmentMoveTo {
	FskFixedPoint2D	p;						/**< Move to this point. */
} FskPathSegmentMoveTo;						/**< Move to */


/** Linear segment. */
typedef struct FskPathSegmentLineTo {
	FskFixedPoint2D	p;						/**< Endpoint of the linear segment. */
} FskPathSegmentLineTo;						/**< Linear segment. */


/** Quadratic Bezier segment. */
typedef struct FskPathSegmentQuadraticBezierTo {
	FskFixedPoint2D p1;						/**< Control (shape) point of the quadratic Bezier segment. */
	FskFixedPoint2D p;						/**< Endpoint of the quadratic Bezier segment. */
} FskPathSegmentQuadraticBezierTo;			/**< Quadratic Bezier segment. */


/** Cubic Bezier segment. */
typedef struct FskPathSegmentCubicBezierTo {
	FskFixedPoint2D p1;						/**< First  control (shape) point of the cubic Bezier segment. */
	FskFixedPoint2D p2;						/**< Second control (shape) point of the cubic Bezier segment. */
	FskFixedPoint2D p;						/**< Endpoint of the cubic Bezier segment. */
} FskPathSegmentCubicBezierTo;				/**< Cubic Bezier segment. */


/** Rational quadratic Bezier segment. */
typedef struct FskPathSegmentRationalQuadraticBezierTo {
	FskFract		w1;						/**< Weight of the control (shape) point of the rational quadratic Bezier segment. */
	FskFixedPoint2D p1;						/**< Control (shape) point of the rational quadratic Bezier segment. */
	FskFixedPoint2D	p;						/**< Endpoint of the rational quadratic Bezier segment. */
} FskPathSegmentRationalQuadraticBezierTo;	/**< Rational quadratic Bezier segment. */


/** Segment types: don't change this without changing the segSizes table in FskPath.c */
typedef enum {
	kFskPathSegmentNull								= 0,	/**< No segment. */
	kFskPathSegmentClose							= 1,	/**< Close the current path. */
	kFskPathSegmentMoveTo							= 2,	/**< End the previous subpath and start a new one at the specified point. */
	kFskPathSegmentLineTo							= 3,	/**< Extend the path from the current point to the new point with a linear segment. */
	kFskPathSegmentQuadraticBezierTo				= 4,	/**< Extend the path from the current point to the new point with a quadratic segment. */
	kFskPathSegmentCubicBezierTo					= 5,	/**< Extend the path from the current point to the new point with a cubic segment. */
	kFskPathSegmentRationalQuadraticBezierTo		= 6,	/**< Extend the path from the current point to the new point with a rational quadratic segment. */
	kFskPathSegmentEndGlyph							= 7		/**< Signal the end of a glyph composed of multiple contours. */
} FskPathElementType;



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////									Path								////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
// This is what a path looks like:                              //
//	typedef struct FskPathRecord {                              //
//		UInt32	numSegments;                                    //
//		UInt32	type;					// Segment 1            //
//		data for the given type ...                             //
//		UInt32	type;					// Segment 2            //
//		data for the given type ...                             //
//		...                                                     //
//		UInt32	type = kFskClose;		// Segment numSegments  //
//	} FskPathRecord;
//////////////////////////////////////////////////////////////////

struct FskPathRecord;								/**< Opaque declaration for a Path -- too complex to be expressed as a structure. */
typedef       struct FskPathRecord	*FskPath;		/**< Pointer to a Path. */
typedef const struct FskPathRecord	*FskConstPath;	/**< Pointer to an immutable Path. */


/** Return the number of segments in the path.
 *	\param[in]	path	the path to be queried.
 *	\return		the number of segments in the path.
 */
FskAPI(UInt32)		FskPathGetSegmentCount(FskConstPath path);


/** Return the number of visible segments in the path.
 *	Like FskPathGetSegmentCountOf(), but skips kFskPathSegmentNull, kFskPathSegmentMoveTo, kFskPathSegmentEndGlyph.
 *	\param[in]	path	the path to be queried.
 *	\return		the number of segments in the path.
 */
FskAPI(UInt32)		FskPathGetVisibleSegmentCount(FskConstPath path);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////								Growable Path							////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


typedef       struct FskGrowableStorageRecord	*FskGrowablePath;		/**< Pointer to a growable path. */
typedef const struct FskGrowableStorageRecord	*FskConstGrowablePath;	/**< Pointer to an immutable growable path. */


/** Constructor: initialize with zero or expected size.
 *	\param[in]	expectedSize	Preallocate this much memory, though the size will be set to zero.
 *	\param[out]	growable		The resultant growable path.
 *	\return		kFskErrNone		if the operation completed successfully.
 */
FskAPI(FskErr)			FskGrowablePathNew(UInt32 expectedSize, FskGrowablePath *growable);

/** Reset the size of the growable path to zero so it can be reused easily.
 *	\param[in]	path	the growable path.
 *	\return		kFskErrNone		if the operation completed successfully.
 */
FskAPI(FskErr)			FskGrowablePathClear(FskGrowablePath path);

/** Get the number of segments in a growable path.
 *	\param[in]	path	the growable path.
 *	\return		the number of segments in the path.
 */
FskAPI(UInt32)			FskGrowablePathGetSegmentCount(FskGrowablePath path);

/** Get the number of bytes used in a path.
 *	\param[in]	path	the path.
 *	\return		the number of bytes in the path.
 */
FskAPI(UInt32)			FskPathSize(FskConstPath path);

/** Get the path from a growable path.
 *	\param[in]	path	the growable path.
 *	\return		the path.
 */
FskAPI(FskPath)			FskGrowablePathGetPath(FskGrowablePath path);

/** Get the immutable path from an immutable growable path.
 *	\param[in]	path	the growable path.
 *	\return		the path.
 */
FskAPI(FskConstPath)	FskGrowablePathGetConstPath(FskConstGrowablePath path);

/** After assembly, it is useful to make a fixed-size FskPath from a FskGrowablePath, for memory efficiency.
 *	\param[in]	growable	the growable path.
 *	\param[out]	path		the path.
 *	\return		kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)			FskGrowablePathNewPath(FskConstGrowablePath growable, FskPath *path);

/** Make a new, exact copy of a path.
 *	\param[in]	fr		the path to duplicate.
 *	\param[out]	to		the new path.
 *	\return		kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)			FskPathClone(FskConstPath fr, FskPath *to);

/** Make a new, exact copy of a growable path.
 *	\param[in]	fr		the growable path to duplicate.
 *	\param[out]	to		the new growable path.
 *	\return		kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)			FskGrowablePathClone(FskConstGrowablePath fr, FskGrowablePath *to);

/** Append a path to a growable path.
 *	\param[in]		fr		the path to append.
 *	\param[in,out]	to		the growable path, which will be appended by fr.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)			FskGrowablePathAppendPath(FskConstPath fr, FskGrowablePath to);

/** Transform and append a path to a growable path.
 *	\param[in]		fr		the path to append.
 *	\param[in]		M		the transformation matrix.
 *	\param[in,out]	to		the growable path, which will be appended by fr.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)			FskGrowablePathAppendTransformedPath(FskConstPath fr, const FskFixedMatrix3x2 *M, FskGrowablePath to);

/** Dispose a growable path.
 *	\param[in]	path	the path to dispose.
 */
FskAPI(void)			FskGrowablePathDispose(FskGrowablePath path);

/** Dispose a fixed-size path.
 *	\param[in]	path	the path to dispose.
 */
FskAPI(void)			FskPathDispose(FskPath path);

/** Get the last point in the path.
 *	\param[in]	path	the path to query.
 *	\param[out]	pt		a place to store the last point.
 *	\return		kFskErrNone		if the operation was completed successfully.
*/
FskAPI(FskErr)			FskGrowablePathGetLastPoint(FskGrowablePath path, FskFixedPoint2D	*pt);


/* Append segments: fixed-point API */

/** Append a "close" segment to a growable path.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentClose(FskGrowablePath path);

/** Append an "end glyph" segment to a growable path.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentEndGlyph(FskGrowablePath path);

/** Append a "move to" segment to a growable path, with fixed-point coordinates.
 *	\param[in]		x				the X-coordinate of the point.
 *	\param[in]		y				the Y-coordinate of the point.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentMoveTo(FskFixed x, FskFixed y, FskGrowablePath path);

/** Append a "line to" segment to a growable path, with fixed-point coordinates.
 *	This causes a line to connect the previous endpoint to this one.
 *	\param[in]		x				the X-coordinate of the endpoint.
 *	\param[in]		y				the Y-coordinate of the endpoint.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentLineTo(FskFixed x, FskFixed y, FskGrowablePath path);

/** Append a "quadratic to" segment to a growable path, with fixed-point coordinates.
 *	This causes a quadratic Bezier curve to connect the previous endpoint to this one.
 *	\param[in]		x1				the X-coordinate of the control (shape) point.
 *	\param[in]		y1				the Y-coordinate of the control (shape) point.
 *	\param[in]		x				the X-coordinate of the endpoint.
 *	\param[in]		y				the Y-coordinate of the endpoint.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentQuadraticBezierTo(FskFixed x1, FskFixed y1, FskFixed x, FskFixed y, FskGrowablePath path);

/** Append a "cubic to" segment to a growable path, with fixed-point coordinates.
 *	This causes a cubic Bezier curve to connect the previous endpoint to this one.
 *	\param[in]		x1				the X-coordinate of the first  control (shape) point.
 *	\param[in]		y1				the Y-coordinate of the first  control (shape) point.
 *	\param[in]		x2				the X-coordinate of the second control (shape) point.
 *	\param[in]		y2				the Y-coordinate of the second control (shape) point.
 *	\param[in]		x				the X-coordinate of the endpoint.
 *	\param[in]		y				the Y-coordinate of the endpoint.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentCubicBezierTo(FskFixed x1, FskFixed y1, FskFixed x2, FskFixed y2, FskFixed x, FskFixed y, FskGrowablePath path);

/** Append a "rational quadratic to" segment to a growable path, with fixed-point coordinates.
 *	This causes a rational quadratic Bezier curve to connect the previous endpoint to this one.
 *	\param[in]		x1				the X-coordinate of the control (shape) point.
 *	\param[in]		y1				the Y-coordinate of the control (shape) point.
 *	\param[in]		w1				the    weight    of the control (shape) point.
 *	\param[in]		x				the X-coordinate of the endpoint.
 *	\param[in]		y				the Y-coordinate of the endpoint.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentRationalQuadraticBezierTo(FskFixed x1, FskFixed y1, FskFract w1, FskFixed x, FskFixed y, FskGrowablePath path);

/** Append several "line to" segments to a growable path, with fixed-point coordinates, using a polyline interface.
 *	This causes a polyline to connect the previous endpoint to this one.
 *	\param[in]		numPts			the number of points in the polyline.
 *	\param[in]		pts				the points of the polyline.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentPolyLineTo(UInt32 numPts, const FskFixedPoint2D *pts, FskGrowablePath path);

/** Append several "quadratic to" segments to a growable path, with fixed-point coordinates, using a quadratic B-spline interface.
 *	This causes a quadratic B-spline curve to connect the previous endpoint to this one.
 *	\param[in]		numPts			the number of points in the quadratic B-spline.
 *	\param[in]		pts				the points of the quadratic B-spline.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentQuadraticBSplineTo(UInt32 numPts, const FskFixedPoint2D *pts, FskGrowablePath path);

/** Append several "cubic to" segments to a growable path, with fixed-point coordinates, using a cubic B-spline interface.
 *	This causes a cubic B-spline curve to connect the previous endpoint to this one.
 *	\param[in]		numPts			the number of points in the cubic B-spline.
 *	\param[in]		pts				the points of the cubic B-spline.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentCubicBSplineTo(UInt32 numPts, const FskFixedPoint2D *pts, FskGrowablePath path);


/* Append segments: floating-point API */


/** Append a "move to" segment to a growable path, with floating-point coordinates.
 *	\param[in]		x				the X-coordinate of the point.
 *	\param[in]		y				the Y-coordinate of the point.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentFloatMoveTo(double x, double y, FskGrowablePath path);

/** Append a "line to" segment to a growable path, with floating-point coordinates.
 *	\param[in]		x				the X-coordinate of the point.
 *	\param[in]		y				the Y-coordinate of the point.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentFloatLineTo(double x, double y, FskGrowablePath path);

/** Append a "quadratic to" segment to a growable path, with floating-point coordinates.
 *	\param[in]		x1				the X-coordinate of the control (shape) point.
 *	\param[in]		y1				the Y-coordinate of the control (shape) point.
 *	\param[in]		x				the X-coordinate of the endpoint.
 *	\param[in]		y				the Y-coordinate of the endpoint.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentFloatQuadraticBezierTo(double x1, double y1, double x, double y, FskGrowablePath path);

/** Append a "cubic to" segment to a growable path, with floating-point coordinates.
 *	\param[in]		x1				the X-coordinate of the first  control (shape) point.
 *	\param[in]		y1				the Y-coordinate of the first  control (shape) point.
 *	\param[in]		x2				the X-coordinate of the second control (shape) point.
 *	\param[in]		y2				the Y-coordinate of the second control (shape) point.
 *	\param[in]		x				the X-coordinate of the endpoint.
 *	\param[in]		y				the Y-coordinate of the endpoint.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentFloatCubicBezierTo(double x1, double y1, double x2, double y2, double x, double y, FskGrowablePath path);

/** Append a "rational quadratic to" segment to a growable path, with floating-point coordinates.
 *	\param[in]		x1				the X-coordinate of the control (shape) point.
 *	\param[in]		y1				the Y-coordinate of the control (shape) point.
 *	\param[in]		w1				the weight control (shape) point.
 *	\param[in]		x				the X-coordinate of the endpoint.
 *	\param[in]		y				the Y-coordinate of the endpoint.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentFloatRationalQuadraticBezierTo(double x1, double y1, double w1, double x, double y, FskGrowablePath path);

/** Append an "elliptical arc to" segment to a growable path, with floating-point coordinates.
 *	\param[in]		rx				the X-radius of the arc.
 *	\param[in]		ry				the Y-radius of the arc.
 *	\param[in]		xAxisRotation	the rotation from the x-axis.
 *	\param[in]		largeArcFlag	the large arc flag.
 *	\param[in]		sweepFlag		the sweep flag.
 *	\param[in]		x2				the X-coordinate of the endpoint.
 *	\param[in]		y2				the Y-coordinate of the endpoint.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendSegmentEllipticalArc(double rx, double ry, double xAxisRotation, Boolean largeArcFlag, Boolean sweepFlag, double x2, double y2, FskGrowablePath path);

/** Append a path in string form to an existing growable path.
 *	This string is in the format specified by SVG:
 *		Mx,y			Absolute move to
 *		mx,y			Relative move to
 *		Lx,y			Absolute line to
 *		lx,y			Relative line to
 *		Hx				Absolute horizontal line
 *		hx				Relative horizontal line
 *		Vy				Absolute vertical line
 *		vy				Relative vertical line
 *		Cx,y,x,y,x,y	Absolute cubic Bezier
 *		cx,y,x,y,x,y	Relative cubic Bezier
 *		Sx,y,x,y		Absolute continuous cubic Bezier
 *		sx,y,x,y		Relative continuous cubic Bezier
 *		Qx,yu,x,y		Absolute quadratic Bezier
 *		qx,y,x,y		Relative quadratic Bezier
 *		Tx,y			Absolute continuous quadratic Bezier
 *		tx,y			Relative continuous quadratic Bezier
 *		Ax,y,r,a,s,x,y	Absolute elliptical arc
 *		ax,y,r,a,s,x,y	Relative elliptical arc
 *		Z				End path
 *		z				End path
 *	In addition, we parse our own extensions:
 *		G				End glyph
 *		g				End glyph
 *		Kx,y,w,x,y		Conic (rational quadratic) Bezier
 *	This can be used in conjunction with FskPathString() to implement serialization.
 *	\param[in]		pathStr			the path string.
 *	\param[in,out]	path			the growable path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendPathString(const char *pathStr, FskGrowablePath path);

/** Append a string representing the path to the given storage.
 *	This can be used in conjunction with FskGrowablePathAppendPathString() to implement serialization.
 *	This only generates the 'M', 'L', 'C', 'Q', 'K', 'Z', 'G' forms above.
 *	Example usage:
 *		FskGrowableStorage str;
 *		FskGrowableStorageNew(0, &str);
 *		FskPathString(path, str);
 *		printf("The path is \"%s\"\n", FskGrowableStorageGetPointerToCString(str));
 *		FskGrowableStorageDispose(str);
 *	\param[in]	path		the path.
 *	\param[in]	precision	The number of significant digits printed. Zero implies the default.
 *	\param[out]	str			the storage contining the resultant string.
 *							This is not allocated by FskPathString().
 *	\return		kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskPathString(FskConstPath path, int precision, FskGrowableStorage str);

/** Transform a path.
 *	\param[in,out]	path	the path to be transformed.
 *	\param[in]		M		the affine transformation matrix.
 */
FskAPI(void)	FskPathTransform(FskPath path, const FskFixedMatrix3x2 *M);

/** Offset a path.
 *	\param[in,out]	path	the path to be offset.
 *	\param[in]		dx		the horizontal offset.
 *	\param[in]		dy		the  vertical  offset.
 */
FskAPI(void)	FskPathOffset(FskPath path, FskFixed dx, FskFixed dy);

/** Transform a growable path.
 *	\param[in,out]	growablePath	the path to be transformed.
 *	\param[in]		M				the affine transformation matrix.
 */
FskAPI(FskErr)	FskGrowablePathTransform(FskGrowablePath growablePath, const FskFixedMatrix3x2 *M);

/** Offset a path.
 *	\param[in,out]	growablePath	the path to be offset.
 *	\param[in]		dx				the horizontal offset.
 *	\param[in]		dy				the  vertical  offset.
 */
FskAPI(FskErr)	FskGrowablePathOffset(FskGrowablePath growablePath, FskFixed dx, FskFixed dy);

/** Tessellate a path.
 *	\param[in]		path			the path to be tessellated.
 *	\param[in]		M				a transformation matrix. NULL implies the identity.
 *	\param[out]		numPointArray	a growable array of type UInt32 into which the contour point counts are placed. It is allocated by the caller with
 *									err = FskGrowableArrayNew(sizeof(UInt32), 8, &numPointArray);
 *	\param[out]		ptArray			a growable array of type FskFixedPoint2D into which the points are placed. It is allocated by the caller.
 *									err = FskGrowableArrayNew(sizeof(FskFixedPoint2D), 64, &numPointArray);
 *	\param[in,out]	pEndCaps		pointer to the end cap specifier, which may be modified depending on whether any closed curves have been encountered.
 *									THis can be NULL.
 *	\return			kFskErrNone		if the operation was a success.
 */
FskAPI(FskErr)	FskPathTessellate(FskConstPath path, const FskFixedMatrix3x2 *M, FskGrowableArray numPointArray, FskGrowableArray ptArray, UInt32 *pEndCaps);


/** Access the segments in a path:
 *	UInt32		numSegs, segType;
 *	const void	*segData;
 *	for (numSegs = FskPathGetSegmentCount(path), segData = NULL; numSegs--; ) {
 *		FskPathGetNextSegment(path, &segType, &segData);
 *		switch ((FskPathElementType)segType) {
 *			case kFskPathSegmentNull:																					break;
 *			case kFskPathSegmentClose:																					break;
 *			case kFskPathSegmentMoveTo:						Do(	(FskPathSegmentMoveTo*)segData						);	break;
 *			case kFskPathSegmentLineTo:						Do(	(FskPathSegmentLineTo*)segData						);	break;
 *			case kFskPathSegmentQuadraticBezierTo:			Do(	(FskPathSegmentQuadraticBezierTo*)segData			);	break;
 *			case kFskPathSegmentCubicBezierTo:				Do(	(FskPathSegmentCubicBezierTo*)segData				);	break;
 *			case kFskPathSegmentRationalQuadraticBezierTo:	Do(	(FskPathSegmentRationalQuadraticBezierTo*)segData	);	break;
 *			case kFskPathSegmentEndGlyph:																				break;
 *		}
 *	}
 *
 *	\param[in]		path	the path.
 *	\param[out]		segType	a place to store the type of the segment.
 *	\param[in,out]	segData	a place to store the data of the segment, which should be case as illustrated above.
 *					However, it should be initialized to NULL on the first call.
 *	\bug			There is no check for going off the end.
 */
FskAPI(void) FskPathGetNextSegment(FskConstPath path, UInt32 *segType, const void **segData);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////							Basic Shapes								////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/** Rectangle, with potentially rounded corners. */
typedef struct FskShapeRect {
	FskFixedPoint2D		origin;		/**< The left  and  top   of the rectangle */
	FskFixedVector2D	size;		/**< The width and height of the rectangle */
	FskFixedVector2D	radius;		/**< The X and Y radii  of the rectangle corners. */
} FskShapeRect;						/**< Rectangle, with potentially rounded corners. */

/** Circle. */
typedef struct FskShapeCircle {
	FskFixedPoint2D		center;		/**< The center of the circle. */
	FskFixed			radius;		/**< The radius of the circle. */
} FskShapeCircle;					/**< Circle. */

/** Ellipse. */
typedef struct FskShapeEllipse {
	FskFixedPoint2D		center;		/**< The center of the ellipse. */
	FskFixedVector2D	radius;		/**< The X and Y radii of the axis-aligned ellipse. */
} FskShapeEllipse;					/**< Ellipse. */

/** Line. */
typedef struct FskShapeLine {
	FskFixedPoint2D		point[2];	/**< The endpoints of the line. */
} FskShapeLine;						/**< Line. */

/** Polyline.
 *	This is typically open, unless closed by duplicating the first point at the end.
 */
typedef struct FskShapePolyLine {
	UInt32				numPoints;	/**< The number of points in the polyline. */
	FskFixedPoint2D		point[1];	/**< The points, actually point[numPoints]. */
} FskShapePolyLine;					/**< Polyline. */

/** Polygon.
 *	This is automatically closed, and should not have duplicated points.
 */
typedef struct FskShapePolygon {
	UInt32				numPoints;	/**< The number of points in the polygon. */
	FskFixedPoint2D		point[1];	/**< The points, actually point[numPoints]. */
} FskShapePolygon;					/**< Polygon. */


/** Append a rect shape to a growable path, with fixed-point coordinates.
 *	\param[in]		rect			the rect.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendShapeRect(const FskShapeRect *rect, FskGrowablePath path);

/** Append a circle shape to a growable path, with fixed-point coordinates.
 *	\param[in]		circ			the circle.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendShapeCircle(const FskShapeCircle *circ, FskGrowablePath path);

/** Append an ellipse shape to a growable path, with fixed-point coordinates.
 *	\param[in]		lips			the ellipse.
 *	\param[in,out]	path			the path.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowablePathAppendShapeEllipse(const FskShapeEllipse *lips, FskGrowablePath path);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////			Tessellation into a Growable FixedPoint2D Array				////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
////								Tessellation							////
////////////////////////////////////////////////////////////////////////////////


/****************************************************************************//**
 * Tessellate an arbitrary order Bezier curve regularly in parametric space.
 * Note that this is not regular in arc length nor chordal deviation, but is faster.
 * The maximum Bezier order accommodated is kFskMaxBezierOrder, from FskFixedMath.h, with the current value of 4.
 *	\param[in]	first	the first point of the Bezier curve.
 *	\param[in]	latter	the later points of the Bezier curve.
 *	\param[in]	order	the order of the Bezier curve.
 *	\param[in]	M		an affine transformation matrix.
 *	\param[out]	array	the tessellated points are appended to this array.
 *	\return		kFskErrNone	if the operation was completed successfully.
 ********************************************************************************/
FskAPI(FskErr)	FskTessellateBezier2DRegularly(
					const FskFixedPoint2D *first, const FskFixedPoint2D *latter, UInt32 order,
					const FskFixedMatrix3x2 *M, FskGrowableFixedPoint2DArray array
				);


/****************************************************************************//**
 * Tessellate an arbitrary order Bezier curve by bisection adaptively based on chordal deviation.
 * This will usually result in less line segments than regular tessellation, but takes longer to compute.
 * The maximum Bezier order accommodated is kFskMaxBezierOrder, from FskFixedMath.h, with the current value of 4.
 *	\param[in]	first	the first point of the Bezier curve.
 *	\param[in]	latter	the later points of the Bezier curve.
 *	\param[in]	order	the order of the Bezier curve.
 *	\param[in]	M		an affine transformation matrix.
 *	\param[out]	array	the tessellated points are appended to this array.
 *	\return		kFskErrNone	if the operation was completed successfully.
 ********************************************************************************/
FskAPI(FskErr)	FskTessellateBezier2DAdaptively(
					const FskFixedPoint2D *first, const FskFixedPoint2D *latter, UInt32 order,
					const FskFixedMatrix3x2 *M, FskGrowableFixedPoint2DArray array
				);


/****************************************************************************//**
 * Tessellate an arbitrary order rational Bezier curve regularly.
 * Note that this is not regular in arc length nor chordal deviation.
 * The maximum Bezier order accommodated is kFskMaxBezierOrder, from FskFixedMath.h, with the current value of 4.
 *	\param[in]	first		the first point of the Bezier curve.
 *	\param[in]	latter		the later points of the Bezier curve.
 *	\param[in]	midWeights	the weights of the midpoints of the Bezier curve.
 *	\param[in]	order		the order of the Bezier curve.
 *	\param[in]	M			an affine transformation matrix.
 *	\param[out]	array		the tessellated points are appended to this array.
 *	\return		kFskErrNone	if the operation was completed successfully.
 ********************************************************************************/
FskAPI(FskErr)	FskTessellateRationalBezier2DRegularly(
					const FskFixedPoint2D *first, const FskFixedPoint2D *latter, const FskFract *midWeights, UInt32 order,
					const FskFixedMatrix3x2	*M, FskGrowableFixedPoint2DArray array
				);


/****************************************************************************//**
 * Tessellate a circular arc into segments of equal length.
 * The start point is not drawn, but the last point is. This allows chaining without degenerate line segments.
 * We take special care to assure that the end point is exactly diametrically opposite the start point if the arc subtends 180 degrees.
 * The tolerance is specified relative to the radius: sqrtRelTol = sqrt(tolerance / radius);
 * If (sqrtRelTol <= 0), it is instead computed as
 *		sqrtRelTol = FskFracSqrt(FskFracDiv(absTol, FskFixedDistance(&start.x, &center.x, 2)));
 * where
 *		absTol = 1/8 = ((FskFixed)(1 << 16)) >> 3;
 * but its precomputation is more efficient when tessellating multiple circular arcs of the same radius.
 *	\param[in]	start		the first point of the arc.
 *	\param[in]	center		the center of the arc.
 *	\param[in]	angle		the angle subtended by the arc, positive or negative.
 *	\param[in]	sqrtRelTol	the square toot of the relative tolerance.
 *	\param[out]	array		the tessellated points are appended to this array.
 *	\return		kFskErrNone	if the operation was completed successfully.
 ********************************************************************************/
FskAPI(FskErr)	FskTessellateCircularArcStartCenterAngle(
					const FskFixedPoint2D *start, const FskFixedPoint2D *center, FskFixedDegrees angle, FskFract sqrtRelTol,
					FskGrowableFixedPoint2DArray array
				);



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////								Rendering								////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


/** Fill a path.
 *  \param[in]		path		the path to be filled.
 *  \param[in]		fillColor	the color source to be used for filling.
 *  \param[in]		fillRule	the fill rule: kFskFillRuleNonZero, kFskFillRuleWindingNumber, kFskFillRuleEvenOdd, kFskFillRuleParity, kFskFillRuleSVGDefault, kFskFillRuleCanvasDefault.
 *  \param[in]		M			the transformation matrix. NULL implies the identity.
 *  \param[in]		quality		the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect	the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM		the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskPathFill(FskConstPath path, const FskColorSource *fillColor, SInt32 fillRule,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame a path.
 *  \param[in]		path			the path to be filled.
 *  \param[in]		frameThickness	the thickness of the frame (line width).
 *  \param[in]		jointSharpness	the sharpness of the joints: kFskLineJoinRound, kFskLineJoinBevel, kFskLineJoinMiter90, kFskLineJoinSVGDefault, kFskLineJoinCanvasDefault.
 *  \param[in]		endCaps			the type of endcaps: kFskLineEndCapRound, kFskLineEndCapSquare, kFskLineEndCapButt, kFskLineEndCapSVGDefault, kFskLineEndCapCanvasDefault.
 *  \param[in]		frameColor		the color source to be used for framing.
 *  \param[in]		M				the transformation matrix. NULL implies the identity.
 *  \param[in]		quality			the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect		the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM			the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskPathFrame(FskConstPath path, FskFixed frameThickness, FskFixed jointSharpness, UInt32 endCaps, const FskColorSource *frameColor,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame and fill a path.
 *  \param[in]		path			the path to be filled.
 *  \param[in]		frameThickness	the thickness of the frame (line width).
 *  \param[in]		jointSharpness	the sharpness of the joints: kFskLineJoinRound, kFskLineJoinBevel, kFskLineJoinMiter90, kFskLineJoinSVGDefault, kFskLineJoinCanvasDefault.
 *  \param[in]		endCaps			the type of endcaps: kFskLineEndCapRound, kFskLineEndCapSquare, kFskLineEndCapButt, kFskLineEndCapSVGDefault, kFskLineEndCapCanvasDefault.
 *  \param[in]		frameColor		the color source to be used for framing.
 *  \param[in]		fillColor		the color source to be used for filling.
 *  \param[in]		fillRule		the fill rule: kFskFillRuleNonZero, kFskFillRuleWindingNumber, kFskFillRuleEvenOdd, kFskFillRuleParity, kFskFillRuleSVGDefault, kFskFillRuleCanvasDefault.
 *  \param[in]		M				the transformation matrix. NULL implies the identity.
 *  \param[in]		quality			the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect		the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM			the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskPathFrameFill(FskConstPath path, FskFixed frameThickness, FskFixed jointSharpness, UInt32 endCaps, const FskColorSource *frameColor,
								const FskColorSource *fillColor, SInt32 fillRule,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Fill a rectangle.
 *  \param[in]		rect		the rectangle to be filled.
 *  \param[in]		fillColor	the color source to be used for filling.
 *  \param[in]		M			the transformation matrix. NULL implies the identity.
 *  \param[in]		quality		the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect	the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM		the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskFillShapeRect(const FskShapeRect *rect, const FskColorSource *fillColor,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame a rectangle.
 *  \param[in]		rect			the rectangle to be filled.
 *  \param[in]		frameThickness	the thickness of the frame (line width).
 *  \param[in]		frameColor		the color source to be used for framing.
 *  \param[in]		M				the transformation matrix. NULL implies the identity.
 *  \param[in]		quality			the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect		the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM			the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskFrameShapeRect(const FskShapeRect *rect, FskFixed frameThickness, const FskColorSource *frameColor,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame and fill a rectangle.
 *  \param[in]		rect			the rectangle to be filled.
 *  \param[in]		frameThickness	the thickness of the frame (line width).
 *  \param[in]		frameColor		the color source to be used for framing.
 *  \param[in]		fillColor		the color source to be used for filling.
 *  \param[in]		M				the transformation matrix. NULL implies the identity.
 *  \param[in]		quality			the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect		the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM			the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskFrameFillShapeRect(const FskShapeRect *rect, FskFixed frameThickness, const FskColorSource *frameColor, const FskColorSource *fillColor,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Fill an ellipse.
 *  \param[in]		lips		the ellipse to be filled.
 *  \param[in]		fillColor	the color source to be used for filling.
 *  \param[in]		M			the transformation matrix. NULL implies the identity.
 *  \param[in]		quality		the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect	the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM		the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskFillShapeEllipse(const FskShapeEllipse *lips, const FskColorSource *fillColor,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame an ellipse.
 *  \param[in]		lips			the ellipse to be filled.
 *  \param[in]		frameThickness	the thickness of the frame (line width).
 *  \param[in]		frameColor		the color source to be used for framing.
 *  \param[in]		M				the transformation matrix. NULL implies the identity.
 *  \param[in]		quality			the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect		the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM			the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskFrameShapeEllipse(const FskShapeEllipse *lips, FskFixed frameThickness, const FskColorSource *frameColor,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame and fill an ellipse.
 *  \param[in]		lips			the ellipse to be filled.
 *  \param[in]		frameThickness	the thickness of the frame (line width).
 *  \param[in]		frameColor		the color source to be used for framing.
 *  \param[in]		fillColor		the color source to be used for filling.
 *  \param[in]		M				the transformation matrix. NULL implies the identity.
 *  \param[in]		quality			the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect		the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM			the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskFrameFillShapeEllipse(const FskShapeEllipse *lips, FskFixed frameThickness, const FskColorSource *frameColor, const FskColorSource *fillColor,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Fill a circle.
 *  \param[in]		circ		the circle to be filled.
 *  \param[in]		fillColor	the color source to be used for filling.
 *  \param[in]		M			the transformation matrix. NULL implies the identity.
 *  \param[in]		quality		the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect	the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM		the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskFillShapeCircle(const FskShapeCircle *circ, const FskColorSource *fillColor,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame a circle.
 *  \param[in]		circ			the circle to be filled.
 *  \param[in]		frameThickness	the thickness of the frame (line width).
 *  \param[in]		frameColor		the color source to be used for framing.
 *  \param[in]		M				the transformation matrix. NULL implies the identity.
 *  \param[in]		quality			the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect		the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM			the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskFrameShapeCircle(const FskShapeCircle *circ, FskFixed frameThickness, const FskColorSource *frameColor,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame and fill a circle.
 *  \param[in]		circ			the circle to be filled.
 *  \param[in]		frameThickness	the thickness of the frame (line width).
 *  \param[in]		frameColor		the color source to be used for framing.
 *  \param[in]		fillColor		the color source to be used for filling.
 *  \param[in]		M				the transformation matrix. NULL implies the identity.
 *  \param[in]		quality			the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect		the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM			the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskFrameFillShapeCircle(const FskShapeCircle *circ, FskFixed frameThickness, const FskColorSource *frameColor, const FskColorSource *fillColor,
								const FskFixedMatrix3x2 *M, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Fill a growable path.
 *  \param[in]		path		the path to be filled.
 *  \param[in]		fillColor	the color source to be used for filling.
 *  \param[in]		fillRule	the fill rule.
 *  \param[in]		M			the transformation matrix. NULL implies the identity.
 *  \param[in]		quality		the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect	the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM		the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
#define FskGrowablePathFill(path, fillColor, fillRule, M, quality, clipRect, dstBM)	\
		FskPathFill(FskGrowablePathGetConstPath(path),	fillColor, fillRule, M, quality, clipRect, dstBM)

/** Frame a growable path.
 *  \param[in]		path			the path to be filled.
 *  \param[in]		frameThickness	the thickness of the frame (line width).
 *  \param[in]		jointSharpness	the sharpness of the joints.
 *  \param[in]		endCaps			the type of endcaps.
 *  \param[in]		frameColor		the color source to be used for framing.
 *  \param[in]		M				the transformation matrix. NULL implies the identity.
 *  \param[in]		quality			the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect		the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM			the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
#define FskGrowablePathFrame(path, frameThickness, jointSharpness, endCaps, frameColor, M, quality, clipRect, dstBM)	\
		FskPathFrame(FskGrowablePathGetConstPath(path),	frameThickness, jointSharpness, endCaps, frameColor, M, quality, clipRect, dstBM)

/** Frame and fill a growable path.
 *  \param[in]		path			the path to be filled.
 *  \param[in]		frameThickness	the thickness of the frame (line width).
 *  \param[in]		jointSharpness	the sharpness of the joints.
 *  \param[in]		endCaps			the type of endcaps.
 *  \param[in]		frameColor		the color source to be used for framing.
 *  \param[in]		fillColor		the color source to be used for filling.
 *  \param[in]		fillRule		the fill rule.
 *  \param[in]		M				the transformation matrix. NULL implies the identity.
 *  \param[in]		quality			the quality of the rendering: 0 or 1.
 *  \param[in]		clipRect		the clipping rectangle. Can be NULL.
 *  \param[in,out]	dstBM			the destination bitmap.
 *  \return	kFskErrNone	if the operation was completed successfully.
 */
#define FskGrowablePathFrameFill(path, frameThickness, jointSharpness, endCaps, frameColor, fillColor, fillRule, M, quality, clipRect, dstBM)	\
		FskPathFrameFill(FskGrowablePathGetConstPath(path),	frameThickness, jointSharpness, endCaps, frameColor, fillColor, fillRule, M, quality, clipRect, dstBM)


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////							Bounding Box								////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


/** Compute the bounding box of a path.
 *	\param[in]	path	The path to be measured.
 *	\param[in]	M		The transformation matrix. If NULL, implies the identity.
 *	\param[in]	tight	If true, a tight bounding box is computed; if false, a fast but loose bound is computed.
 *	\param[out]	bbox	The computed bounding box.
 *	\return		kFskErrNone	if the operation completed successfully.
 */
FskAPI(FskErr)	FskPathComputeBoundingBox(FskConstPath path, const FskFixedMatrix3x2 *M, Boolean tight, FskFixedRectangle bbox);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////								Arc Length								////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


struct FskArcLengthTableRecord;											/**< Record to store an arc length table. */
typedef struct FskArcLengthTableRecord *FskArcLengthTable;				/**< Pointer to an arc length table. */
typedef const struct FskArcLengthTableRecord *FskConstArcLengthTable;	/**< Pointer to an immutable arc length table. */

/** Generate an arc length table from a path.
 *	\param[in]	path		the path to be measured.
 *	\param[out]	tab			a place to store the generated table.
 *	\param[out]	totalLength	a place to store the total length of the path.
 *	\return		kFskErrNone	if the operation completed successfully.
 */
FskAPI(FskErr)	FskPathArcLengthTableNew(FskConstPath path, FskArcLengthTable *tab, FskFixed *totalLength);

/** Get the start and end relative distances of a given segment from the beginning of the path,
 *	where the beginning and end of the path has distance 0 and 1, respectively.
 *	\param[in]	tab			the arc length table.
 *	\param[in]	segIndex	the index of the segment being queried.
 *	\param[out]	startEnd	a place to store the start and end distances of the requested segment.
 */
FskAPI(void)	FskPathArcLengthTableGetDistancesOfSegment(FskConstArcLengthTable tab, UInt32 segIndex, FskFract startEnd[2]);

/** Dispose of an arc length table.
 *	\param[in]	tab		the arc length table to be disposed.
 */
FskAPI(void)	FskPathArcLengthTableDispose(FskArcLengthTable tab);

/** Generate a list of distances for all visible segments.
 *	\param[in]	path		the path to be measured.
 *	\param[in]	tab			the arc length table.
 *	\param[out]	pNumDists	a place to store the number of distances in the distance table.
 *	\param[out]	pDistances	a place to store the array of distances,
 *							where (*pDistances)[0] = 0 and (*pDistances)[*pNumDists - 1] == 1.
 *	\return		kFskErrNone	if the operation completed successfully.
 */
FskAPI(FskErr)	FskPathGetVisibleSegmentDistances(FskConstPath path, FskConstArcLengthTable tab, UInt32 *pNumDists, double **pDistances);


/** Evaluate a path at a given parametric value.
 *	\param[in]	path	the path to be evaluated.
 *	\param[in]	arcTab	the precomputed arc length table.
 *	\param[in]	s		the [relative] distance along the path to be evaluated, i.e. the beginning and end of the path are 0 and 1, respectively.
 *	\param[out]	M		the result of the evaluation: the frame at the specified arc length.
 *						The top two rows of the matrix (rotation) are FskFract, whereas the bottom row is FskFixed:
 *						{ tangent, normal, position }.
 *	\return		kFskErrNone	if the operation completed successfully.
 */
FskAPI(FskErr)	FskPathEvaluate(FskConstPath path, FskConstArcLengthTable arcTab, FskFract s, FskFixedMatrix3x2 *M);

/** Evaluate a path segment at a fraction along its length.
 *	\param[in]	path		the path to be evaluated.
 *	\param[in]	arcTab		the precomputed arc length table.
 *	\param[in]	segIndex	the index of the segment to be queried.
 *	\param[in]	f			the [relative] distance along the segment to be evaluated, i.e. the beginning and end of the segment are 0 and 1, respectively.
 *	\param[out]	M		the result of the evaluation: the frame at the specified arc length.
 *						The top two rows of the matrix (rotation) are FskFract, whereas the bottom row (translation) is FskFixed: { tangent, normal, position }.
 *	\return		kFskErrNone	if the operation completed successfully.
 */
FskAPI(FskErr)	FskPathEvaluateSegment(FskConstPath path, FskConstArcLengthTable arcTab, UInt32 segIndex, FskFract f, FskFixedMatrix3x2 *M);



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////								Tweening								////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


/** Interpolate (morph) between two paths, allocating a new path for the results.
 *	\param[in]	path0		the path corresponding to fraction 0.
 *	\param[in]	path1		the path corresponding to fraction 1.
 *	\param[in]	fraction	a fraction between 0 and 1, where the paths will be linearly interpolated.
 *	\param[out]	patht		a place to store a new path, which is the interpolation between the given paths.
 */
FskAPI(FskErr)	FskPathNewTween(FskConstPath path0, FskConstPath path1, double fraction, FskPath *patht);

/** Interpolate (morph) between two paths, and store in an already existing compatible path, such as once created with FskClonePath().
 *	\param[in]	path0		the path corresponding to fraction 0.
 *	\param[in]	path1		the path corresponding to fraction 1.
 *	\param[in]	fraction	a fraction between 0 and 1, where the paths will be linearly interpolated.
 *	\param[out]	patht		an existing path, where the tweened path will be stored.
 */
FskAPI(FskErr)	FskPathTween(FskConstPath path0, FskConstPath path1, double fraction, FskPath  patht);

/** Check whether two paths are compatible for tweening.
 *	\param[in]	path0	one of the paths.
 *	\param[in]	path1	the other path.
 *	\return		true	if the paths are compatible,
 *	\return		false	otherwise.
 */
FskAPI(Boolean)	FskPathCompatibleForTweening(FskConstPath path0, FskConstPath path1);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKPATH__ */

