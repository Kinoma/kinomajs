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
	\file	FskPath.c
	\brief	Implementation of paths composed of line, quadratic Bezier, cubic Bezier, and circular arc segments.
*/
#define __FSKBITMAP_PRIV__	/* To get access to the FskBitmap data structure */

#include "FskPath.h"
#include "FskClipLine2D.h"
#include "FskLine.h"
#include "FskMatrix.h"
#include "FskPolygon.h"
#include "FskUtilities.h"


#include <math.h>
#include <ctype.h>


#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh) || defined(__MWERKS__)
		#define PRAGMA_MARK_SUPPORTED 1
	#else // !PRAGMA_MARK_SUPPORTED
		#define PRAGMA_MARK_SUPPORTED 0
	#endif // !PRAGMA_MARK_SUPPORTED
#endif // PRAGMA_MARK_SUPPORTED


#if SUPPORT_INSTRUMENTATION
	#define LOG_PARAMETERS
	//#define LOG_CONTOURS
	#define FskInstrumentedGlobalType(type)			(&(g##type##TypeInstrumentation))												/**< The data structure created with FskInstrumentedSimpleType(). */
	#define FskInstrumentedTypeEnabled(type, level)	FskInstrumentedTypeHasListenersForLevel(FskInstrumentedGlobalType(type), level)	/**< Whether instrumentation will print anything. */
	#define	LOGD(...)	FskPathPrintfDebug(__VA_ARGS__)										/**< Print debugging logs. */
	#define	LOGI(...)	FskPathPrintfVerbose(__VA_ARGS__)									/**< Print information logs. */
#else /* !SUPPORT_INSTRUMENTATION */
	#define FskInstrumentedTypeEnabled(type, level)	(0)																				/**< Instrumentation will not print anything. */
	#define LOGD(...)	do {} while(0)														/**< Don't print debugging logs. */
	#define LOGI(...)	do {} while(0)														/**< Don't print information logs. */
#endif /* SUPPORT_INSTRUMENTATION */
#define		LOGE(...)	FskPathPrintfMinimal(__VA_ARGS__)									/**< Print error logs always, when instrumentation is on. */
#define LOGD_ENABLED()	FskInstrumentedTypeEnabled(Path, kFskInstrumentationLevelDebug)		/**< Whether LOGD() will print anything. */
#define LOGI_ENABLED()	FskInstrumentedTypeEnabled(Path, kFskInstrumentationLevelVerbose)	/**< Whether LOGI() will print anything. */
#define LOGE_ENABLED()	FskInstrumentedTypeEnabled(Path, kFskInstrumentationLevelMinimal)	/**< Whether LOGE() will print anything. */

FskInstrumentedSimpleType(Path, path);														/**< This declares the types needed for instrumentation. */




/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***							typedefs and macros							  ***
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


typedef struct DRay2D {	double	x, y, dx, dy;	} DRay2D;


#define MyMoveData(src,dst,size) FskMemCopy(dst, src, size)			/* I'm one of those people who prefers to copy left to right */
#define UNUSED(x)				(void)(x)
#define CeilFixedToInt(x)		(((x) + 0xFFFF) >> 16)
#define FloorFixedToInt(x)		(((x)         ) >> 16)
#define FRACT_ONE				((FskFract)0x40000000)
#define FRACT_SQRT_HALF			((FskFract)0x2D413CCD)				/* 759250125 = 2^30 * sqrt(0.5) */
#define FIXED_ONE				((FskFixed)0x00010000)
#define MIN_S32					((SInt32)0x80000000)				/* -2147483648 */
#define MAX_S32					((SInt32)0x7FFFFFFF)				/* +2147483647 */
#define MAX_RECT_SIZE			((SInt32)0x7FFFFFFF)				/* +2147483647 */
#define MIN_RECT_COORD			((SInt32)0xC0000000)				/* -1073741824 */
#define MAX_RECT_COORD			((SInt32)0x3FFFFFFF)				/* +1073741823 */
#define D_2PI					6.2831853071795864769
#define D_PI					3.1415926535897932385
#define D_RADIANS_PER_DEGREE	0.017453292519943295769
#define W_FRACBITS				30									/* The number of fractional bits in the denominator of a rational Bezier, yielding a range of [-2, +2) */
#define W_ONE					(1 << W_FRACBITS)
#define LINEAR_TOLERANCE		(FIXED_ONE / 8)						/* Tesselation parameters */
#define TessellateBezier2D		FskTessellateBezier2DRegularly		/* We choose to tesselate regularly rather than adaptively */
//#define TessellateBezier2D	FskTessellateBezier2DAdaptively




/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***								Instrumentation							  ***
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

#if SUPPORT_INSTRUMENTATION
#include "FskPixelOps.h"
static void LogBitmap(FskConstBitmap bm, const char *name) {
	if (!bm)
		return;
	if (!name)
		name = "BM";
	LOGD("\t%s: bounds(%d, %d, %d, %d) depth=%u format=%s rowBytes=%d bits=%p alpha=%d premul=%d",
		name, (int)bm->bounds.x, (int)bm->bounds.y, (int)bm->bounds.width, (int)bm->bounds.height, (unsigned)bm->depth,
		FskBitmapFormatName(bm->pixelFormat), (int)bm->rowBytes, bm->bits, bm->hasAlpha, bm->alphaIsPremultiplied);
}
static void LogRect(FskConstRectangle r, const char *name) {
	if (!r)
		return;
	if (!name)
		name = "RECT";
	LOGD("\t%s(%d, %d, %d, %d)", name, (int)r->x, (int)r->y, (int)r->width, (int)r->height);
}
static void LogLongString(const char *str, int indent, const char *name) {
	const UInt32	kMaxCharsPerLine	= 1000;
	UInt32			strSize				= FskStrLen(str);
	const char		*sep;
	UInt32			offset;
	char			indentStr[24];

	indentStr[indent] = 0;
	for (offset = indent; offset--;)
		indentStr[offset] = '\t';
	if (name)	sep = ": ";
	else		sep = name = "";
	if (strSize <= kMaxCharsPerLine) {
		LOGD("%*c%s%s%s", indent, ' ', name, sep, str);
	}
	else {
		#define TRUNCATE_PATH_STRING
		#ifdef TRUNCATE_PATH_STRING
			LOGD("%s%s%s%.*s...", indentStr, name, sep, kMaxCharsPerLine, str);
		#else /* KEEP_PATH_STRING */
			const UInt32 kMaxCharsPerBreak = 160;
			LOGD("%s%s%s%.*s", indentStr, name, sep, kMaxCharsPerBreak, str);
			for (offset = kMaxCharsPerBreak; offset < strSize; offset += kMaxCharsPerBreak)
			LOGD("\t%s%.*s", indentStr, kMaxCharsPerBreak, str + offset);
		#endif /* TRUNCATE_PATH_STRING */
	}
}
static void LogPath(FskConstPath path, const char *name) {
	const UInt32		kCharsPerSeg	= 64;
	FskGrowableStorage	str				= NULL;
	FskErr				err;

	if (!path)	return;
	if (!name)	name = "PATH";

	BAIL_IF_ERR(err = FskGrowableStorageNew(kCharsPerSeg * FskPathGetSegmentCount(path), &str));
	BAIL_IF_ERR(err = FskPathString(path, 6, str));
	LogLongString(FskGrowableStorageGetPointerToCString(str), 1, name);
bail:
	FskGrowableStorageDispose(str);
}
static const char* SpreadMethodNameFromCode(UInt32 spreadMethod) {
	static const char *spreads[] = {
		"transparent",
		"padX|transparentY",
		"repeatX|transparentY",
		"reflectX|transparentY",
		"transparentX|padY",
		"pad",
		"repeatX|padY",
		"reflectX|padY",
		"transparentX|repeatY",
		"padX|repeatY",
		"repeat",
		"reflectX|repeatY",
		"transparentX|reflectY",
		"padX|reflectY",
		"repeatX|reflectY",
		"reflect"
	};
	return spreads[spreadMethod & 0xF];
}
static const char* EndCapsNameFromCode(UInt32 endCaps) {
	switch (endCaps) {
		case kFskLineEndCapRound:	return "round";
		case kFskLineEndCapSquare:	return "square";
		case kFskLineEndCapButt:	return "butt";
		default:					return "UNKNOWN";
	}
}
static const char* FillRuleNameFromCode(SInt32 fillRule) {
	switch (fillRule) {
		case kFskFillRuleNonZero:	return "nonzero";
		case kFskFillRuleEvenOdd:	return "even-odd";
		default:					return "UNKNOWN";
	}
}
static void LogFixedMatrix3x2(const FskFixedMatrix3x2 *M, const char *name) {
	if (!M)
		return;
	if (!name)	name = "MTX";
	LOGD("\t%s: [[%g %g],[%g %g],[%g %g]]", name, M->M[0][0] * (1./65536.), M->M[0][1] * (1./65536.),
		M->M[1][0] * (1./65536.), M->M[1][1] * (1./65536.), M->M[2][0] * (1./65536.), M->M[2][1] * (1./65536.));
}
static void LogGradientStops(UInt32 numStops, const FskGradientStop *stops) {
	UInt32 i;
	if (!numStops || !stops)
		return;
	for (i = 0; i < numStops; ++i, ++stops)
		LOGD("\tstop[%u]: offset=%6.4f color={%3u %3u %3u %3u}", i, stops->offset * (1./1073741824.),
			stops->color.r, stops->color.g, stops->color.b, stops->color.a);
}
static void LogDash(UInt32 dashCycles, const FskFixed *dash, FskFixed dashPhase) {
	FskGrowableStorage	str					= NULL;
	if (!dashCycles || !dash)
		return;
	if (kFskErrNone == FskGrowableStorageNew(dashCycles * 20, &str)) {
		unsigned i;
		for (i = 0; i < dashCycles; ++i) {
			if (i)
				FskGrowableStorageAppendF(str, ", ");
			FskGrowableStorageAppendF(str, "{%.2f,%.2f} ", dash[2*i+0]/65536., dash[2*i+1]/65536.);
		}
	}
	LOGD("\tdash[%u @ %.2f]: %s", dashCycles, dashPhase/65536., FskGrowableStorageGetPointerToCString(str));
}
static void LogColorSource(const FskColorSource *cs, const char *name) {
	const FskColorSourceUnion *csu = (FskColorSourceUnion*)cs;
	if (!csu)
		return;
	if (!name)	name = "COLORSOURCE";
	switch (csu->so.type) {
		case kFskColorSourceTypeConstant:
			LOGD("\t%s: Constant(r=%u g=%u b=%u a=%u)", name, csu->cn.color.r, csu->cn.color.g, csu->cn.color.b, csu->cn.color.a);
			break;
		case kFskColorSourceTypeLinearGradient:
			LOGD("\t%s: LinearGradient(gradientVector={{%g, %g}, {%g, %g}} gradientMatrix=%p spreadMethod=%s numStops=%u stops=%p)", name,
				csu->lg.gradientVector[0].x * (1./65536.), csu->lg.gradientVector[0].y * (1./65536.),
				csu->lg.gradientVector[1].x * (1./65536.), csu->lg.gradientVector[1].y * (1./65536.),
				csu->lg.gradientMatrix, SpreadMethodNameFromCode(csu->lg.spreadMethod), csu->lg.numStops, csu->lg.gradientStops);
			LogFixedMatrix3x2(csu->lg.gradientMatrix, "gradientMatrix");
			LogGradientStops(csu->lg.numStops, csu->lg.gradientStops);
			break;
		case kFskColorSourceTypeRadialGradient:
			LOGD("\t%s: RadialGradient(focus(x=%g y=%g r=%g) outer(x=%g y=%g r=%g) gradientMatrix=%p spreadMethod=%s numStops=%u stops=%p)", name,
				csu->rg.focus.x  * (1./65536.), csu->rg.focus.y  * (1./65536.), csu->rg.focalRadius * (1./65536.),
				csu->rg.center.x * (1./65536.), csu->rg.center.y * (1./65536.), csu->rg.radius      * (1./65536.),
				csu->rg.gradientMatrix, SpreadMethodNameFromCode(csu->rg.spreadMethod), csu->rg.numStops, csu->rg.gradientStops);
			LogFixedMatrix3x2(csu->rg.gradientMatrix, "gradientMatrix");
			LogGradientStops(csu->rg.numStops, csu->rg.gradientStops);
			break;
		case kFskColorSourceTypeTexture:
			LOGD("\t%s: Texture(bm=%p textureFrame=%p spreadMethod=%s)", name, csu->tx.texture, csu->tx.textureFrame, SpreadMethodNameFromCode(csu->tx.spreadMethod));
			LogBitmap(csu->tx.texture, "textureBM");
			LogFixedMatrix3x2(csu->tx.textureFrame, "textureFrame");
			break;
		default:
			break;
	}
	LogDash(csu->so.dashCycles, csu->so.dash, csu->so.dashPhase);
}
static double AreaOfFixedPolygon(UInt32 numPts, const FskFixedPoint2D *pts) {
	const FskFixedPoint2D *pmi;
	double area;
	for (pmi = pts + numPts - 1, area = 0.; numPts--; pmi = pts++)
		area += (double)(pts->x + pmi->x) * (double)(pts->y - pmi->y);
	return area * (.5f / 65536.);
}
static void LogContours(UInt32 numCtr, const UInt32 *numPts, const FskFixedPoint2D *pts) {
	const UInt32		kMaxCharsPerLine	= 1000;
	FskGrowableStorage	str					= NULL;
	int					i;

	if (kFskErrNone != FskGrowableStorageNew(kMaxCharsPerLine, &str))
		return;
	LOGD("\t<polygon numContours=\"%u\">", (unsigned)numCtr);
	for (; numCtr--; ++numPts) {
		FskGrowableStorageSetSize(str, 0);
		FskGrowableStorageAppendF(str, "<contour numPoints=\"%u\" area=\"%.0f\">", (unsigned)(*numPts), AreaOfFixedPolygon(*numPts, pts));
		for (i = *numPts; i-- > 0; ++pts)
			FskGrowableStorageAppendF(str, " %.2f,%.2f", pts->x/65536., pts->y/65536.);
		FskGrowableStorageAppendF(str, " </contour>");
		LogLongString(FskGrowableStorageGetPointerToCString(str), 2, NULL);
	}
	LOGD("\t</polygon>");
	FskGrowableStorageDispose(str);
}
static void LogPolyline(UInt32 numPts, const FskFixedPoint2D *pts) {
	const UInt32		kMaxCharsPerLine	= 1000;
	FskGrowableStorage	str					= NULL;
	int					i;

	if (kFskErrNone != FskGrowableStorageNew(kMaxCharsPerLine, &str))
		return;
	FskGrowableStorageAppendF(str, "<polyline numPoints=\"%u\">", (unsigned)numPts);
	for (i = numPts; i-- > 0; ++pts)
		FskGrowableStorageAppendF(str, " %.2f,%.2f", pts->x/65536., pts->y/65536.);
	FskGrowableStorageAppendF(str, " </polyline>");
	LogLongString(FskGrowableStorageGetPointerToCString(str), 1, NULL);
	FskGrowableStorageDispose(str);
}
#endif /* SUPPORT_INSTRUMENTATION */


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***									Utilities							  ***
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/*******************************************************************************
 * ConvertEllipticalArcFromEndpointToCenterParameterization
 *	This is from the SVG implementation notes.
 *******************************************************************************/

static void
ConvertEllipticalArcFromEndpointToCenterParameterization(
	double	x1,
	double	y1,
	double	rx,
	double	ry,
	double	xAxisRotation,
	Boolean	largeArcFlag,
	Boolean	sweepFlag,
	double	x2,
	double	y2,
	double	*pcx,
	double	*pcy,
	double	*pTheta1,
	double	*pDTheta
)
{
	double	dx, dy, mx, my, x1p, y1p, x1p2, y1p2, rx2, ry2, c, s, r, cxp, cyp, cx, cy, theta1, theta2, dTheta, lambda;

	/* Ensure radii are nonzero */
	if ((rx == 0) || (ry == 0)) {
		*pDTheta = *pTheta1 = *pcx = *pcy = 0;
		return;
	}

	/* Ensure radii are positive */
	rx		= fabs(rx);
	ry		= fabs(ry);

	dx		= (x1 - x2) * 0.5;
	dy		= (y1 - y2) * 0.5;
	mx		= x2 + dx;
	my		= y2 + dy;
	c		= cos(xAxisRotation);
	s		= sin(xAxisRotation);
	x1p		=  c * dx + s * dy;
	y1p		= -s * dx + c * dy;
	rx2		= rx * rx;
	ry2		= ry * ry;
	x1p2	= x1p * x1p;
	y1p2	= y1p * y1p;

	/* Ensure radii are large enough */
	lambda	= x1p2 / rx2 + y1p2 / ry2;
	if (lambda > 1) {
		lambda	= sqrt(lambda);
		rx		*= lambda;
		ry		*= lambda;
//		rx2		= rx * rx;
//		ry2		= ry * ry;
		r		= 0;
	}
	else {
		r = (rx2 * ry2 - rx2 * y1p2 - ry2 * x1p2) / (rx2 * y1p2 + ry2 * x1p2);
		if (r > 0)	r = sqrt(r);
		else		r = 0;
	}
	if (sweepFlag == largeArcFlag)
		r = -r;
	cxp		=  r * rx * y1p / ry;
	cyp		= -r * ry * x1p / rx;
	cx		= c * cxp - s * cyp + mx;
	cy		= s * cxp + c * cyp + my;
	theta1	= atan2(( y1p - cyp) / ry, ( x1p - cxp) / rx);
	theta2	= atan2((-y1p - cyp) / ry, (-x1p - cxp) / rx);
	dTheta	= fmod(theta2 - theta1, D_2PI);
	if (sweepFlag == 0) {
		if (dTheta > 0)
			dTheta -= D_2PI;
	}
	else {
		if (dTheta < 0)
			dTheta += D_2PI;
	}

	*pcx		= cx;
	*pcy		= cy;
	*pTheta1	= theta1;
	*pDTheta	= dTheta;
}


/********************************************************************************
 * IntersectRays2D
 * Gaussian elimination with full pivoting.
 * This is robust enough for fixed-point computations.
 ********************************************************************************/

static void
IntersectRays2D(const DRay2D *r0, const DRay2D *r1, double *xy)
{
	double	max, s, t;
	int		maxi, i;
	double	M[4], b[2];

	M[0] = r0->dx;	M[1] = -r1->dx;
	M[2] = r0->dy;	M[3] = -r1->dy;
	b[0] = r1->x - r0->x;
	b[1] = r1->y - r0->y;

	max  = fabs(M[0]);
	maxi = 0;
	for (i = 1; i < 4; i++) {
		if (max < (t = fabs(M[i]))) {
			max  = t;
			maxi = i;
		}
	}

	switch (maxi) {
		case 0:									/* Pivot around [0][0] */
		default:
			s = M[2] / M[0];					/* Scale to nullify [1][0] */
			t = (b[1] - s * b[0]) / (M[3] - s * M[1]);	/* Solve for t1 */
			xy[0] = r1->x + t * r1->dx;			/* Compute intersection on ray1 */
			xy[1] = r1->y + t * r1->dy;
			break;
		case 1:									/* Pivot around [1][1] */
			s = M[3] / M[1];					/* Scale to nullify [1][0] */
			t = (b[1] - s * b[0]) / (M[2] - s * M[0]);	/* Solve for t0 */
			xy[0] = r0->x + t * r0->dx;			/* Compute intersection on ray0 */
			xy[1] = r0->y + t * r0->dy;
			break;
		case 2:									/* Pivot around [0][0] */
			s = M[0] / M[2];					/* Scale to nullify [1][0] */
			t = (b[0] - s * b[1]) / (M[1] - s * M[3]);	/* Solve for t1 */
			xy[0] = r1->x + t * r1->dx;			/* Compute intersection on ray1 */
			xy[1] = r1->y + t * r1->dy;
			break;
		case 3:									/* Pivot around [1][1] */
			s = M[1] / M[3];					/* Scale to nullify [0][1] */
			t = (b[0] - s * b[1]) / (M[0] - s * M[2]);	/* Solve for t0 */
			xy[0] = r0->x + t * r0->dx;			/* Compute intersection on ray0 */
			xy[1] = r0->y + t * r0->dy;
			break;
	}
}


/*******************************************************************************
 * ScaleStrokeWidth
 *******************************************************************************/

static FskFixed
ScaleStrokeWidth(FskFixed strokeWidth, const FskFixedMatrix3x2 *M)
{
	return FskFixMul(strokeWidth, FskFixedMatrixNorm2x2(M->M[0], 2));
}



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***							Growable Path methods						  ***
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * segSizes
 ********************************************************************************/

static const UInt8 segSizes[] = {
	0,	/* sizeof(FskPathSegmentNull) */
	0,	/* sizeof(FskPathSegmentClose) */
	sizeof(FskPathSegmentMoveTo),
	sizeof(FskPathSegmentLineTo),
	sizeof(FskPathSegmentQuadraticBezierTo),
	sizeof(FskPathSegmentCubicBezierTo),
	sizeof(FskPathSegmentRationalQuadraticBezierTo),
	0,	/* sizeof(FskPathSegmentEndGlyph) */
};

typedef struct FskPathRecord {
	UInt32					numSegments;
	/* TaggedSegment ... */
} FskPathRecord;

typedef struct TaggedSegment {								UInt32	type;													} TaggedSegment;
typedef struct TaggedSegmentClose {							UInt32	type;													} TaggedSegmentClose;
typedef struct TaggedSegmentEndGlyph {						UInt32	type;													} TaggedSegmentEndGlyph;
typedef struct TaggedSegmentMoveTo {						UInt32	type;	FskPathSegmentMoveTo					data;	} TaggedSegmentMoveTo;
typedef struct TaggedSegmentLineTo {						UInt32	type;	FskPathSegmentLineTo					data;	} TaggedSegmentLineTo;
typedef struct TaggedSegmentQuadraticBezierTo {				UInt32	type;	FskPathSegmentQuadraticBezierTo			data;	} TaggedSegmentQuadraticBezierTo;
typedef struct TaggedSegmentCubicBezierTo {					UInt32	type;	FskPathSegmentCubicBezierTo				data;	} TaggedSegmentCubicBezierTo;
typedef struct TaggedSegmentRationalQuadraticBezierTo {		UInt32	type;	FskPathSegmentRationalQuadraticBezierTo	data;	} TaggedSegmentRationalQuadraticBezierTo;


#define GetPathSegmentCount(path)		(path->numSegments)
#define GetPathSegments(path)			((TaggedSegment*)(path + 1))
#define GetConstPathSegments(path)		((const TaggedSegment*)(path + 1))
#define NextTaggedPathSegment(seg)		((TaggedSegment*)((char*)(seg) + sizeof(UInt32) + segSizes[(seg)->type]))
#define NextConstTaggedPathSegment(seg)	((const TaggedSegment*)((const char*)(seg) + sizeof(UInt32) + segSizes[(seg)->type]))
#define PreviousPointToSegment(seg)		(((const FskFixedPoint2D*)(seg))[-1])


/********************************************************************************
 * FskPathGetSegmentCount
 ********************************************************************************/

UInt32
FskPathGetSegmentCount(FskConstPath path)
{
	return GetPathSegmentCount(path);
}


/********************************************************************************
 * FskPathGetVisibleSegmentCount
 ********************************************************************************/

UInt32
FskPathGetVisibleSegmentCount(FskConstPath path)
{
	const TaggedSegment	*segPtr;
	UInt32				numSegs;
	UInt32				numVisibleSegments	= 0;

	numSegs  = GetPathSegmentCount(path);
	segPtr   = GetConstPathSegments(path);

	for ( ; numSegs--; segPtr = NextConstTaggedPathSegment(segPtr)) {
		switch (segPtr->type) {
			case kFskPathSegmentNull:
			case kFskPathSegmentMoveTo:
			case kFskPathSegmentEndGlyph:
				break;
			case kFskPathSegmentClose:
			case kFskPathSegmentLineTo:
			case kFskPathSegmentQuadraticBezierTo:
			case kFskPathSegmentCubicBezierTo:
			case kFskPathSegmentRationalQuadraticBezierTo:
				numVisibleSegments++;
				break;
			default:			/* Unknown segment type */
				numVisibleSegments = 0;
				goto bail;
		}
	}

bail:
	return numVisibleSegments;
}


/********************************************************************************
 * FskGrowablePathGetSegmentCount
 ********************************************************************************/

UInt32
FskGrowablePathGetSegmentCount(FskGrowablePath path)
{
	UInt32	*p;

	return (FskGrowableStorageGetPointerToItem(path, 0, (void**)(void*)(&p)) == kFskErrNone) ? *p : 0;
}


/********************************************************************************
 * FskPathSize
 ********************************************************************************/

UInt32
FskPathSize(FskConstPath path)
{
	const TaggedSegment	*segPtr;
	UInt32				numSegs;

	if (path == NULL)	return 0;

	numSegs  = GetPathSegmentCount(path);
	segPtr   = GetConstPathSegments(path);
	for ( ; numSegs--; segPtr = NextConstTaggedPathSegment(segPtr))
		continue;

	return((UInt32)((const char*)segPtr - (const char*)path));
}


/********************************************************************************
 * AppendItemToGrowablePath
 ********************************************************************************/

static FskErr
AppendItemToGrowablePath(FskGrowablePath grPath, const void *item, UInt32 itemSize)
{
	FskErr		err;
	FskPath		path;

	if (	((err = FskGrowableStorageAppendItem(grPath, item, itemSize)) == kFskErrNone)
		&&	((err = FskGrowableStorageGetPointerToItem(grPath, 0, (void**)(void*)(&path))) == kFskErrNone)
	)
		(path->numSegments)++;

	return err;
}


/********************************************************************************
 * FskGrowablePathAppendPath
 ********************************************************************************/

FskErr
FskGrowablePathAppendPath(FskConstPath fr, FskGrowablePath to)
{
	FskErr	err;
	FskPath	path;

	if (	((err = FskGrowableStorageAppendItem(to, GetConstPathSegments(fr), FskPathSize(fr) - sizeof(UInt32))) == kFskErrNone)
		&&	((err = FskGrowableStorageGetPointerToItem(to, 0, (void**)(void*)(&path))) == kFskErrNone)
	)
		path->numSegments += GetPathSegmentCount(fr);

	return err;
}


/********************************************************************************
 * FskGrowablePathAppendTransformedPath
 ********************************************************************************/

FskErr
FskGrowablePathAppendTransformedPath(FskConstPath fr, const FskFixedMatrix3x2 *M, FskGrowablePath to)
{
	FskErr			err;
	FskPath			path;
	FskPathRecord	savePathHeader;
	UInt32			oldSize, newSegCount;

	oldSize		= FskGrowableStorageGetSize(to);																							/* Get location of new path */
	if (	((err = FskGrowableStorageAppendItem(to, GetConstPathSegments(fr), FskPathSize(fr) - sizeof(FskPathRecord))) == kFskErrNone)	/* Append fr path */
		&&	((err = FskGrowableStorageGetPointerToItem(to, 0, (void**)(void*)(&path))) == kFskErrNone)										/* Get pointer to the to path header */
	) {
		newSegCount = GetPathSegmentCount(fr);								/* Get the number of segments in the fr path */
		path->numSegments += newSegCount;									/* Update segment count to include the new segments */
		path = (FskPath)((char*)path + oldSize - sizeof(FskPathRecord));	/* Pointer to the phantom path header */
		savePathHeader = *path;												/* Save phantom path header */
		path->numSegments = newSegCount;									/* Temporarily put the new segment count into the phantom segment count */
		FskPathTransform(path, M);											/* Transform the recently appended path */
		*path = savePathHeader;												/* Restore phantom path header */
	}

	return err;
}


/********************************************************************************
 * AppendTransformedPointToGrowableArray
 ********************************************************************************/

static FskErr
AppendTransformedPointToGrowableArray(FskGrowableArray ptArray, const FskFixedMatrix3x2 *M, const FskFixedPoint2D *pt)
{
	FskFixedPoint2D	tp;
	FskErr			err;

	if (M == NULL) {
		err = FskGrowableArrayAppendItem(ptArray, pt);
	}
	else {
		FskTransformFixedRowPoints2D(pt, 1, M, &tp);
		err = FskGrowableArrayAppendItem(ptArray, &tp);
	}

	return err;
}


/********************************************************************************
 * ClearPathHeader
 ********************************************************************************/

#define ClearPathHeader(path)	FskMemSet((path), 0, sizeof(*(path)))


/********************************************************************************
 * FskGrowablePathNew
 ********************************************************************************/

FskErr
FskGrowablePathNew(UInt32 expectedSize, FskGrowablePath *pStorage)
{
	FskErr			err;
	FskPathRecord	path;

	ClearPathHeader(&path);
	if ((err = FskGrowableStorageNew(expectedSize, (FskGrowableStorage*)pStorage)) == kFskErrNone)
		err = FskGrowableStorageAppendItem(*pStorage, &path, sizeof(FskPathRecord));

	return err;
}


/********************************************************************************
 * FskGrowablePathDispose
 ********************************************************************************/

void
FskGrowablePathDispose(FskGrowablePath storage)
{
	FskGrowableStorageDispose((FskGrowableStorage)storage);
}


/********************************************************************************
 * FskGrowablePathClear
 ********************************************************************************/

FskErr
FskGrowablePathClear(FskGrowablePath storage)
{
	FskPath		path;
	FskErr		err;

	BAIL_IF_ERR(err = FskGrowableStorageSetSize((FskGrowableStorage)storage, sizeof(FskPathRecord)));
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(storage, 0, (void**)(void*)(&path)));
	ClearPathHeader(path);

bail:
	return err;
}


/********************************************************************************
 * FskGrowablePathGetPath
 ********************************************************************************/

FskPath
FskGrowablePathGetPath(FskGrowablePath growable)
{
	FskPath	path;

	if (kFskErrNone != FskGrowableStorageGetPointerToItem(growable, 0, (void**)(void*)(&path)))
		path = NULL;

	return path;
}


/********************************************************************************
 * FskGrowablePathGetConstPath
 ********************************************************************************/

FskConstPath
FskGrowablePathGetConstPath(FskConstGrowablePath growable)
{
	FskConstPath	path;

	if (kFskErrNone != FskGrowableStorageGetConstPointerToItem(growable, 0, (const void**)(const void*)(&path)))
		path = NULL;

	return path;
}


/********************************************************************************
 * FskGrowablePathNewPath
 ********************************************************************************/

FskErr
FskGrowablePathNewPath(FskConstGrowablePath storage, FskPath *path)
{
	UInt32		z;
	const void	*src;
	FskErr		err;

	z = FskGrowableStorageGetSize(storage);
	if (	((err = FskMemPtrNew(z, (FskMemPtr*)path)) == kFskErrNone)
		&&	((err = FskGrowableStorageGetConstPointerToItem(storage, 0, &src)) == kFskErrNone)
	)
		MyMoveData(src, *path, z);

	return err;
}


/********************************************************************************
 * FskPathClone
 ********************************************************************************/

FskErr
FskPathClone(FskConstPath src, FskPath *dst)
{
	UInt32		z;
	FskErr		err;

	z = FskPathSize(src);
	if ((err = FskMemPtrNew(z, (FskMemPtr*)dst)) == kFskErrNone)
		MyMoveData(src, *dst, z);

	return err;
}


/********************************************************************************
 * FskGrowablePathClone
 ********************************************************************************/

FskErr
FskGrowablePathClone(FskConstGrowablePath src, FskGrowablePath *dst)
{
	FskErr			err;
	UInt32			pathSize	= FskGrowableStorageGetSize(src);
	const void		*srcPath;


	if (	((err = FskGrowableStorageNew(pathSize, dst)) == kFskErrNone)
		&&	((err = FskGrowableStorageGetConstPointerToItem(src, 0, &srcPath)) == kFskErrNone)
	)
		err = FskGrowableStorageAppendItem(*dst, srcPath, pathSize);

	return err;
}


/********************************************************************************
 * FskPathDispose
 ********************************************************************************/

void
FskPathDispose(FskPath path)
{
	(void)FskMemPtrDispose(path);
}


/********************************************************************************
 * FskGrowablePathAppendSegmentClose
 ********************************************************************************/

FskErr
FskGrowablePathAppendSegmentClose(FskGrowablePath path)
{
	TaggedSegmentClose seg;
	seg.type = kFskPathSegmentClose;
	return AppendItemToGrowablePath(path, &seg, sizeof(TaggedSegmentClose));

}


/********************************************************************************
 * FskGrowablePathAppendSegmentEndGlyph
 ********************************************************************************/

FskErr
FskGrowablePathAppendSegmentEndGlyph(FskGrowablePath path)
{
	TaggedSegmentEndGlyph seg;
	seg.type = kFskPathSegmentEndGlyph;
	return AppendItemToGrowablePath(path, &seg, sizeof(TaggedSegmentEndGlyph));

}


/********************************************************************************
 * FskGrowablePathAppendSegmentMoveTo
 ********************************************************************************/

FskErr
FskGrowablePathAppendSegmentMoveTo(FskFixed x, FskFixed y, FskGrowablePath path)
{
	TaggedSegmentMoveTo seg;
	seg.type		= kFskPathSegmentMoveTo;
	seg.data.p.x	= x;
	seg.data.p.y	= y;
	return AppendItemToGrowablePath(path, &seg, sizeof(TaggedSegmentMoveTo));
}


/********************************************************************************
 * FskGrowablePathAppendSegmentLineTo
 ********************************************************************************/

FskErr
FskGrowablePathAppendSegmentLineTo(FskFixed x, FskFixed y, FskGrowablePath path)
{
	TaggedSegmentLineTo seg;
	seg.type		= kFskPathSegmentLineTo;
	seg.data.p.x	= x;
	seg.data.p.y	= y;
	return AppendItemToGrowablePath(path, &seg, sizeof(TaggedSegmentLineTo));
}


/********************************************************************************
 * FskGrowablePathAppendSegmentQuadraticBezierTo
 ********************************************************************************/

FskErr
FskGrowablePathAppendSegmentQuadraticBezierTo(FskFixed x1, FskFixed y1, FskFixed x, FskFixed y, FskGrowablePath path)
{
	TaggedSegmentQuadraticBezierTo seg;
	seg.type		= kFskPathSegmentQuadraticBezierTo;
	seg.data.p1.x	= x1;
	seg.data.p1.y	= y1;
	seg.data.p.x	= x;
	seg.data.p.y	= y;
	return AppendItemToGrowablePath(path, &seg, sizeof(TaggedSegmentQuadraticBezierTo));
}


/********************************************************************************
 * FskGrowablePathAppendSegmentCubicBezierTo
 ********************************************************************************/

FskErr
FskGrowablePathAppendSegmentCubicBezierTo(FskFixed x1, FskFixed y1, FskFixed x2, FskFixed y2, FskFixed x, FskFixed y, FskGrowablePath path)
{
	TaggedSegmentCubicBezierTo seg;
	seg.type		= kFskPathSegmentCubicBezierTo;
	seg.data.p1.x	= x1;
	seg.data.p1.y	= y1;
	seg.data.p2.x	= x2;
	seg.data.p2.y	= y2;
	seg.data.p.x	= x;
	seg.data.p.y	= y;
	return AppendItemToGrowablePath(path, &seg, sizeof(TaggedSegmentCubicBezierTo));
}


/********************************************************************************
 * FskGrowablePathAppendSegmentRationalQuadraticBezierTo
 ********************************************************************************/

FskErr
FskGrowablePathAppendSegmentRationalQuadraticBezierTo(FskFixed x1, FskFixed y1, FskFract w1, FskFixed x, FskFixed y, FskGrowablePath path)
{
	TaggedSegmentRationalQuadraticBezierTo seg;
	seg.type		= kFskPathSegmentRationalQuadraticBezierTo;
	seg.data.w1		= w1;
	seg.data.p1.x	= x1;
	seg.data.p1.y	= y1;
	seg.data.p.x	= x;
	seg.data.p.y	= y;
	return AppendItemToGrowablePath(path, &seg, sizeof(TaggedSegmentRationalQuadraticBezierTo));
}


/********************************************************************************
 * FskGrowablePathAppendSegmentPolyLineTo
 ********************************************************************************/

FskErr
FskGrowablePathAppendSegmentPolyLineTo(UInt32 numPts, const FskFixedPoint2D *pt, FskGrowablePath path)
{
	FskErr err	= kFskErrNone;

	for ( ; numPts--; pt++)
		if ((err = FskGrowablePathAppendSegmentLineTo(pt->x, pt->y, path)) != kFskErrNone)
			break;

	return err;
}


/********************************************************************************
 * FskGrowablePathAppendSegmentQuadraticBSplineTo
 ********************************************************************************/

FskErr
FskGrowablePathAppendSegmentQuadraticBSplineTo(UInt32 numPts, const FskFixedPoint2D *pt, FskGrowablePath path)
{
	FskErr		err;

	BAIL_IF_FALSE((numPts > 0), err, kFskErrBadData);
	if (numPts == 1) {							/* Not enough for a quadratic segment -- append a line instead */
		err = FskGrowablePathAppendSegmentLineTo(pt[0].x, pt[0].y, path);
	}
	else {
		FskFixed	xm, ym;
		for (numPts -=2; numPts--; pt++) {		/* Convert from quadratic B-spline to quadratic Bezier */
			xm = (pt[1].x + pt[0].x) >> 1;
			ym = (pt[1].y + pt[0].y) >> 1;
			BAIL_IF_ERR(err = FskGrowablePathAppendSegmentQuadraticBezierTo(pt[0].x, pt[0].y, xm, ym, path));
		}
		err = FskGrowablePathAppendSegmentQuadraticBezierTo(pt[0].x, pt[0].y, pt[1].x, pt[1].y, path);
	}

bail:
	return err;
}


/********************************************************************************
 * FskGrowablePathAppendSegmentCubicBSplineTo
 ********************************************************************************/

#define kFracThird	0x15555555

FskErr
FskGrowablePathAppendSegmentCubicBSplineTo(UInt32 numPts, const FskFixedPoint2D *pt, FskGrowablePath path)
{
	FskErr			err;
	FskFixedPoint2D	q1, q2, q3, q4;

	q1 = *pt++;
	switch (numPts) {
		case 0:
		case 1:
		case 2:
			return FskGrowablePathAppendSegmentQuadraticBSplineTo(numPts, --pt, path);				/* Not enough points for a cubic -- do a quadratic instead */
		case 3:																						/* Already a cubic Bezier */
			q4 = q1;
			break;
		case 4:
			q4.x = (pt[0].x + pt[1].x)   >> 1;										q4.y = (pt[0].x + pt[1].x)   >> 1;
			q2.x = (q1.x    + pt[0].x)   >> 1;										q2.y = (q1.y    + pt[0].x)   >> 1;
			q3.x = (q2.x    + q4.x     ) >> 1;										q3.y = (q2.y    + q4.y     ) >> 1;
			BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatCubicBezierTo(q1.x, q1.y, q2.x, q2.y, q3.x, q3.y, path));
			++pt;
			break;
		default:
			q4.x = FskFracMul((pt[0].x << 1) + (pt[1].x << 0), kFracThird);			q4.y = FskFracMul((pt[0].y << 1) + (pt[1].y << 0), kFracThird);
			q2.x =            (q1.x          +  pt[0].x     ) >> 1;					q2.y =            (q1.y          +  pt[0].y     ) >> 1;
			q3.x =            (q2.x          +  q4.x        ) >> 1;					q3.y =            (q2.y          +  q4.y        ) >> 1;
			BAIL_IF_ERR(err = FskGrowablePathAppendSegmentCubicBezierTo(q1.x, q1.y, q2.x, q2.y, q3.x, q3.y, path));
			q1 = q4;
			for (numPts -= 5; numPts--; ++pt, q1 = q4) {
				q4.x = FskFracMul((pt[1].x << 1) + (pt[2].x << 0), kFracThird);		q4.y = FskFracMul((pt[1].y << 1) + (pt[2].y << 0), kFracThird);
				q2.x = FskFracMul((pt[0].x << 0) + (pt[1].x << 1), kFracThird);		q2.y = FskFracMul((pt[0].y << 0) + (pt[1].y << 1), kFracThird);
				q3.x =            (q2.x          +  q4.x        ) >> 1;				q3.y =            (q2.y          +  q4.y        ) >> 1;
				BAIL_IF_ERR(err = FskGrowablePathAppendSegmentCubicBezierTo(q1.x, q1.y, q2.x, q2.y, q3.x, q3.y, path));
			}
			q4.x =            (pt[1].x       +  pt[2].x     ) >> 1;					q4.y =            (pt[1].y       +  pt[2].y     ) >> 1;
			q2.x = FskFracMul((pt[0].x << 0) + (pt[1].x << 1), kFracThird);			q2.y = FskFracMul((pt[0].y << 0) + (pt[1].y << 1), kFracThird);
			q3.x =            (q2.x          +  q4.x        ) >> 1;					q3.y =            (q2.y          +  q4.y        ) >> 1;
			BAIL_IF_ERR(err = FskGrowablePathAppendSegmentCubicBezierTo(q1.x, q1.y, q2.x, q2.y, q3.x, q3.y, path));
			pt += 2;
			break;
	}
	err = FskGrowablePathAppendSegmentCubicBezierTo(q4.x, q4.y, pt[0].x, pt[0].y, pt[1].x, pt[1].y, path);

bail:
	return err;
}


/********************************************************************************
 * FskGrowablePathGetLastPoint
 ********************************************************************************/

FskErr
FskGrowablePathGetLastPoint(FskGrowablePath grath, FskFixedPoint2D	*pt)
{
	FskErr err = kFskErrNone;

	UInt32			numSegs;
	TaggedSegment	*segPtr;
	FskFixedPoint2D	*lastPt = NULL;
	FskPath			path	= FskGrowablePathGetPath(grath);

	numSegs   = GetPathSegmentCount(path);
	segPtr    = GetPathSegments(path);

	for ( ; numSegs--; segPtr = NextTaggedPathSegment(segPtr)) {
		switch (segPtr->type) {
			case kFskPathSegmentMoveTo:						lastPt = &(((TaggedSegmentMoveTo*)                   segPtr)->data.p);	break;
			case kFskPathSegmentLineTo:						lastPt = &(((TaggedSegmentLineTo*)                   segPtr)->data.p);	break;
			case kFskPathSegmentQuadraticBezierTo:			lastPt = &(((TaggedSegmentQuadraticBezierTo*)        segPtr)->data.p);	break;
			case kFskPathSegmentCubicBezierTo:				lastPt = &(((TaggedSegmentCubicBezierTo*)			 segPtr)->data.p);	break;
			case kFskPathSegmentRationalQuadraticBezierTo:	lastPt = &(((TaggedSegmentRationalQuadraticBezierTo*)segPtr)->data.p);	break;
			case kFskPathSegmentEndGlyph:					lastPt = NULL;															break;
			default:											break;
		}
	}
	if  (lastPt == NULL)	err = kFskErrNoSubpath;
	else if (pt != NULL)	*pt = *lastPt;
	return err;
}


/********************************************************************************
 * FskPathTransform
 ********************************************************************************/

void
FskPathTransform(
	FskPath					path,
	const FskFixedMatrix3x2	*M
)
{
	UInt32			numSegs;
	TaggedSegment	*segPtr;

	numSegs   = GetPathSegmentCount(path);
	segPtr    = GetPathSegments(path);

	for ( ; numSegs--; segPtr = NextTaggedPathSegment(segPtr)) {
		switch (segPtr->type) {
			case kFskPathSegmentMoveTo:
			{	TaggedSegmentMoveTo								*moveTo = (TaggedSegmentMoveTo*)(segPtr);
																FskTransformFixedRowPoints2D(&moveTo->data.p, 1, M, &moveTo->data.p);
			}													break;
			case kFskPathSegmentLineTo:
			{	TaggedSegmentLineTo								*lineTo = (TaggedSegmentLineTo*)(segPtr);
																FskTransformFixedRowPoints2D(&lineTo->data.p, 1, M, &lineTo->data.p);
			}													break;
			case kFskPathSegmentQuadraticBezierTo:
			{	TaggedSegmentQuadraticBezierTo					*quadTo = (TaggedSegmentQuadraticBezierTo*)(segPtr);
																FskTransformFixedRowPoints2D(&quadTo->data.p1, 2, M, &quadTo->data.p1);
			}													break;
			case kFskPathSegmentCubicBezierTo:
			{	TaggedSegmentCubicBezierTo						*cubcTo = (TaggedSegmentCubicBezierTo*)(segPtr);
																FskTransformFixedRowPoints2D(&cubcTo->data.p1, 3, M, &cubcTo->data.p1);
			}													break;
			case kFskPathSegmentRationalQuadraticBezierTo:
			{	TaggedSegmentRationalQuadraticBezierTo			*concTo = (TaggedSegmentRationalQuadraticBezierTo*)(segPtr);
																FskTransformFixedRowPoints2D(&concTo->data.p1, 2, M, &concTo->data.p1);
			}													break;
			default:											break;
		}
	}
}


/********************************************************************************
 * FskGrowablePathTransform
 ********************************************************************************/

FskErr
FskGrowablePathTransform(
	FskGrowablePath			growablePath,
	const FskFixedMatrix3x2	*M
)
{
	FskErr			err;
	FskPath			path;

	if ((err = FskGrowableStorageGetPointerToItem(growablePath, 0, (void**)(void*)(&path))) == kFskErrNone)
		FskPathTransform(path, M);

	return err;
}


/********************************************************************************
 * FskPathOffset
 ********************************************************************************/

void
FskPathOffset(FskPath path, FskFixed dx, FskFixed dy)
{
	UInt32			numSegs;
	TaggedSegment	*segPtr;

	numSegs   = GetPathSegmentCount(path);
	segPtr    = GetPathSegments(path);

	for ( ; numSegs--; segPtr = NextTaggedPathSegment(segPtr)) {
		switch (segPtr->type) {
			case kFskPathSegmentMoveTo:
			{	TaggedSegmentMoveTo								*moveTo = (TaggedSegmentMoveTo*)(segPtr);
																moveTo->data.p.x	+= dx;	moveTo->data.p.y	+= dy;
			}													break;
			case kFskPathSegmentLineTo:
			{	TaggedSegmentLineTo								*lineTo = (TaggedSegmentLineTo*)(segPtr);
																lineTo->data.p.x	+= dx;	lineTo->data.p.y	+= dy;
			}													break;
			case kFskPathSegmentQuadraticBezierTo:
			{	TaggedSegmentQuadraticBezierTo					*quadTo = (TaggedSegmentQuadraticBezierTo*)(segPtr);
																quadTo->data.p1.x	+= dx;	quadTo->data.p1.y	+= dy;
																quadTo->data.p.x	+= dx;	quadTo->data.p.y	+= dy;
			}													break;
			case kFskPathSegmentCubicBezierTo:
			{	TaggedSegmentCubicBezierTo						*cubcTo = (TaggedSegmentCubicBezierTo*)(segPtr);
																cubcTo->data.p1.x	+= dx;	cubcTo->data.p1.y	+= dy;
																cubcTo->data.p2.x	+= dx;	cubcTo->data.p2.y	+= dy;
																cubcTo->data.p.x	+= dx;	cubcTo->data.p.y	+= dy;
			}													break;
			case kFskPathSegmentRationalQuadraticBezierTo:
			{	TaggedSegmentRationalQuadraticBezierTo			*concTo = (TaggedSegmentRationalQuadraticBezierTo*)(segPtr);
																concTo->data.p1.x	+= dx;	concTo->data.p1.y	+= dy;
																concTo->data.p.x	+= dx;	concTo->data.p.y	+= dy;
			}													break;
			default:											break;
		}
	}
}


/********************************************************************************
 * FskGrowablePathOffset
 ********************************************************************************/

FskErr
FskGrowablePathOffset(FskGrowablePath growablePath, FskFixed dx, FskFixed dy)
{

	FskErr			err;
	FskPath			path;

	if ((err = FskGrowableStorageGetPointerToItem(growablePath, 0, (void**)(void*)(&path))) == kFskErrNone)
		FskPathOffset(path, dx, dy);

	return err;
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****				Floating point API Convenience Methods					*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/*******************************************************************************
 * FskGrowablePathAppendSegmentFloatMoveTo
 *******************************************************************************/

FskErr
FskGrowablePathAppendSegmentFloatMoveTo(double x, double y, FskGrowablePath path)
{
	return FskGrowablePathAppendSegmentMoveTo((FskFixed)(x * 65536), (FskFixed)(y * 65536), path);
}


/*******************************************************************************
 * FskGrowablePathAppendSegmentFloatLineTo
 *******************************************************************************/

FskErr
FskGrowablePathAppendSegmentFloatLineTo(double x, double y, FskGrowablePath path)
{
	return FskGrowablePathAppendSegmentLineTo((FskFixed)(x * 65536), (FskFixed)(y * 65536), path);
}


/*******************************************************************************
 * FskGrowablePathAppendSegmentFloatQuadraticBezierTo
 *******************************************************************************/

FskErr
FskGrowablePathAppendSegmentFloatQuadraticBezierTo(double x1, double y1, double x, double y, FskGrowablePath path)
{
	return FskGrowablePathAppendSegmentQuadraticBezierTo((FskFixed)(x1 * 65536), (FskFixed)(y1 * 65536), (FskFixed)(x * 65536), (FskFixed)(y * 65536), path);
}


/*******************************************************************************
 * FskGrowablePathAppendSegmentFloatCubicBezierTo
 *******************************************************************************/

FskErr
FskGrowablePathAppendSegmentFloatCubicBezierTo(double x1, double y1, double x2, double y2, double x, double y, FskGrowablePath path)
{
	return FskGrowablePathAppendSegmentCubicBezierTo((FskFixed)(x1 * 65536), (FskFixed)(y1 * 65536), (FskFixed)(x2 * 65536), (FskFixed)(y2 * 65536), (FskFixed)(x * 65536), (FskFixed)(y * 65536), path);
}


/*******************************************************************************
 * FskGrowablePathAppendSegmentFloatRationalQuadraticBezierTo
 *******************************************************************************/

FskErr
FskGrowablePathAppendSegmentFloatRationalQuadraticBezierTo(double x1, double y1, double w1, double x, double y, FskGrowablePath path)
{
	return FskGrowablePathAppendSegmentRationalQuadraticBezierTo((FskFixed)(x1 * 65536), (FskFixed)(y1 * 65536), (FskFract)(w1 * W_ONE), (FskFixed)(x * 65536), (FskFixed)(y * 65536), path);
}


/********************************************************************************
 * FskGrowablePathAppendSegmentEllipse, as specified in the Canvas spec.
 ********************************************************************************/

FskErr FskGrowablePathAppendSegmentEllipse(double cx, double cy, double rx, double ry, double rotation, double startAngle, double endAngle, Boolean anticlockwise, FskGrowablePath path)
{
	SInt32					subDivisions;
	double					ch, th, dAngle, hAngle;
	double					M[3][2], C[2][2], P[2][2], Q[2][2], R[2][2];
	Boolean					isCircle;
	FskFixedPoint2D			p0, p1;

	/* Compute coordinate frame.
	 * | +rx * cos(rot)		+rx * sin(rot)	|
	 * | -rx * sin(rot)		+ry * cos(rot)	|
	 * | cx					cy				|
	 */
	M[0][0] =   M[1][1] = cos(rotation);
	M[1][0] = -(M[0][1] = sin(rotation));
	M[0][0] *= rx;
	M[0][1] *= rx;
	M[1][0] *= ry;
	M[1][1] *= ry;
	M[2][0] = cx;
	M[2][1] = cy;

	/* Get angles consistent */
	dAngle = endAngle - startAngle;
	if (anticlockwise) {																			/* Counterclockwise angles are negative */
		if ((dAngle < -D_2PI) || ((dAngle > 0) && ((dAngle -= D_2PI) >= 0)))						/* Try to map angle into [-2pi, 0) */
			endAngle = startAngle + (dAngle = -D_2PI);												/* Or saturate to -2pi if the magnitude is greater than 2pi */
	}
	else /* clockwise */ {																			/* Clockwise        angles are positive */
		if ((dAngle > +D_2PI) || ((dAngle < 0) && ((dAngle += D_2PI) <= 0)))						/* Try to map angle into (0, +2pi] */
			endAngle = startAngle + (dAngle = +D_2PI);												/* Or saturate to +2pi if the magnitude is greater than 2pi */
	}
	isCircle = fabs((fabs(dAngle) - D_2PI)) < 1.e-14;

	/* Determine the number of subdivisions and transformation between them */
	subDivisions = (SInt32)(fabs(dAngle * .5));														/* No more than 2 radians per segment */
	dAngle /= (subDivisions + 1);
	R[0][0] =   R[1][1] = cos(dAngle);
	R[1][0] = -(R[0][1] = sin(dAngle));

	/* Initialize first subdivision */
	hAngle = dAngle * .5;
	ch = cos(hAngle);																				/* All segments share the same w, since they are equally divided */
	th = tan(hAngle);
	C[0][0] = cos(startAngle);																		/* First point on  circle */
	C[0][1] = sin(startAngle);
	C[1][0] = C[0][0] - C[0][1] * th;																/* First point off circle */
	C[1][1] = C[0][1] + C[0][0] * th;
	FskDAffineTransform(C[0], M[0], P[0], 2, 2, 2);													/* Transformation into target frame */

	/* If there was a previous point, join it to the start point with a line, otherwise move to the start point */
	p1.x = (FskFixed)(P[0][0] * 65536);																/* Convert the first point to fixed point */
	p1.y = (FskFixed)(P[0][1] * 65536);
	if (kFskErrNone != FskGrowablePathGetLastPoint(path, &p0))										/* If there was no previous point, ... */
		(void)FskGrowablePathAppendSegmentMoveTo(p1.x, p1.y, path);									/* ... move to the first point */
	else if ((abs(p1.x - p0.x) + abs(p1.y - p0.y)) > 2)												/* If there was a previous point and it was sufficiently different ... */
		(void)FskGrowablePathAppendSegmentLineTo(p1.x, p1.y, path);									/* ... draw a line from it to the first point */

	/* If necessary, insert up to 3 additional rational quadratic Bezier segments */
	for (; subDivisions--;) {
		Q[0][0] = C[0][0]; Q[0][1] = C[0][1]; Q[1][0] = C[1][0]; Q[1][1] = C[1][1];					/* Save old C in Q, because FskDLinearTransform does not work in place */
		FskDLinearTransform(Q[0], R[0], C[0], 2, 2, 2);												/* Rotate C */
		Q[1][0] = P[1][0]; Q[1][1] = P[1][1];														/* Save old off-circle point */
		FskDAffineTransform(C[0], M[0], P[0], 2, 2, 2);												/* Transformation into target frame */
		(void)FskGrowablePathAppendSegmentFloatRationalQuadraticBezierTo(Q[1][0], Q[1][1], ch, P[0][0], P[0][1], path);
	}

	if (isCircle)
		endAngle = startAngle;																		/* Assure that we get the identical coordinates for the end */
	C[0][0] = cos(endAngle);																		/* End point on circle */
	C[0][1] = sin(endAngle);
	FskDAffineTransform(C[0], M[0], P[0], 1, 2, 2);													/* Transformation into target frame */
	return FskGrowablePathAppendSegmentFloatRationalQuadraticBezierTo(P[1][0], P[1][1], ch, P[0][0], P[0][1], path);
}


/********************************************************************************
 * FskGrowablePathAppendSegmentEllipticalArc, as specified in the SVG spec.
 *	This returns either:
 *		- 1 line segment
 *		- 1 or more homogeneous quadratic Bezier segments
 ********************************************************************************/

FskErr
FskGrowablePathAppendSegmentEllipticalArc(
	double					rx,
	double					ry,
	double					xAxisRotation,
	Boolean					largeArcFlag,
	Boolean					sweepFlag,
	double					x2,
	double					y2,
	FskGrowablePath			path
)
{
	FskErr			err	;
	FskFixedPoint2D	p0;
	double			cx, cy, theta1, dTheta, x0, y0;

	/* Get the previous point */
	BAIL_IF_ERR(err = FskGrowablePathGetLastPoint(path, &p0));
	x0 = p0.x * (1. / 65536.);
	y0 = p0.y * (1. / 65536.);
	ConvertEllipticalArcFromEndpointToCenterParameterization(x0, y0, rx, ry, xAxisRotation, largeArcFlag, sweepFlag, x2, y2, &cx, &cy, &theta1, &dTheta);
	if (dTheta == 0)	err = FskGrowablePathAppendSegmentFloatLineTo(x2, y2, path);
	else				err = FskGrowablePathAppendSegmentEllipse(cx, cy, rx, ry, xAxisRotation, theta1, theta1 + dTheta, dTheta < 0, path);

bail:
	return err;
}


/********************************************************************************
 * ParseDoubles
 ********************************************************************************/

static Boolean
ParseDoubles(const char **pStr, unsigned n, double *d)
{
	char *p0, *p1;
	for (p0 = p1 = (char*)(*pStr); n--; p0 = p1) {
		*d++ = strtod(p0, &p1);
		if (p0 == p1)
			return false;
		while (isspace(*p1) || *p1 == ',')
			++p1;
	}
	*pStr = p1;
	return true;
}


/********************************************************************************
 * FskGrowablePathAppendPathString
 ********************************************************************************/

FskErr
FskGrowablePathAppendPathString(const char *pathStr, FskGrowablePath path)
{
	FskErr		err		= kFskErrNone;
	double		lastX	= 0,
				lastY	= 0,
				backX	= 0,
				backY	= 0;
	double		num[7];

	while (*pathStr) {
		switch (*pathStr++) {
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			case ',':
				break;
			case 'M':																				/* Absolute move to */
				while (ParseDoubles(&pathStr, 2, num)) {
					lastX = num[0];		lastY = num[1];
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatMoveTo(lastX, lastY, path));
				}
				break;
			case 'm':																				/* Relative move to */
				while (ParseDoubles(&pathStr, 2, num)) {
					lastX += num[0];	lastY += num[1];
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatMoveTo(lastX, lastY, path));
				}
				break;
			case 'L':																				/* Absolute line to */
				while (ParseDoubles(&pathStr, 2, num)) {
					lastX = num[0];		lastY = num[1];
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatLineTo(lastX, lastY, path));
				}
				break;
			case 'l':																				/* Relative line to */
				while (ParseDoubles(&pathStr, 2, num)) {
					lastX += num[0];	lastY += num[1];
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatLineTo(lastX, lastY, path));
				}
				break;
			case 'H':																				/* Absolute horizontal line */
				while (ParseDoubles(&pathStr, 1, num)) {
					lastX = num[0];
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatLineTo(lastX, lastY, path));
				}
				break;
			case 'h':																				/* Relative horizontal line */
				while (ParseDoubles(&pathStr, 1, num)) {
					lastX += num[0];
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatLineTo(lastX, lastY, path));
				}
				break;
			case 'V':																				/* Absolute vertical line */
				while (ParseDoubles(&pathStr, 1, num)) {
					lastY = num[0];
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatLineTo(lastX, lastY, path));
				}
				break;
			case 'v':																				/* Relative vertical line */
				while (ParseDoubles(&pathStr, 1, num)) {
					lastY += num[0];
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatLineTo(lastX, lastY, path));
				}
				break;
			case 'C':																				/* Absolute cubic Bezier */
				while (ParseDoubles(&pathStr, 6, num)) {
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatCubicBezierTo(num[0], num[1], num[2], num[3], num[4], num[5], path));
					backX = num[2];		backY = num[3];
					lastX = num[4];		lastY = num[5];
				}
				break;
			case 'c':																				/* Relative cubic Bezier */
				while (ParseDoubles(&pathStr, 6, num)) {
					num[0] += lastX;	num[1] += lastY;
					num[2] += lastX;	num[3] += lastY;
					num[4] += lastX;	num[5] += lastY;
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatCubicBezierTo(num[0], num[1], num[2], num[3], num[4], num[5], path));
					backX = num[2];		backY = num[3];
					lastX = num[4];		lastY = num[5];
				}
				break;
			case 'S':																				/* Absolute continuous cubic Bezier */
				while (ParseDoubles(&pathStr, 4, num+2)) {
					num[0] = lastX * 2. - backX;		num[1] = lastY * 2. - backY;
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatCubicBezierTo(num[0], num[1], num[2], num[3], num[4], num[5], path));
					backX = num[2];		backY = num[3];
					lastX = num[4];		lastY = num[5];
				}
				break;
			case 's':																				/* Relative continuous cubic Bezier */
				while (ParseDoubles(&pathStr, 4, num+2)) {
					num[0] = lastX * 2. - backX;		num[1] = lastY * 2. - backY;
					num[2] += lastX;					num[3] += lastY;
					num[4] += lastX;					num[5] += lastY;
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatCubicBezierTo(num[0], num[1], num[2], num[3], num[4], num[5], path));
					backX = num[2];		backY = num[3];
					lastX = num[4];		lastY = num[5];
				}
				break;
			case 'Q':																				/* Absolute quadratic Bezier */
				while (ParseDoubles(&pathStr, 4, num)) {
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatQuadraticBezierTo(num[0], num[1], num[2], num[3], path));
					backX = num[0];		backY = num[1];
					lastX = num[2];		lastY = num[3];
				}
				break;
			case 'q':																				/* Relative quadratic Bezier */
				while (ParseDoubles(&pathStr, 4, num)) {
					num[0] += lastX;	num[1] += lastY;
					num[2] += lastX;	num[3] += lastY;
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatQuadraticBezierTo(num[0], num[1], num[2], num[3], path));
					backX = num[0];			backY = num[1];
					lastX = num[2];			lastY = num[3];
				}
				break;
			case 'T':																				/* Absolute continuous quadratic Bezier */
				while (ParseDoubles(&pathStr, 2, num+2)) {
					num[0] = lastX * 2. - backX;		num[1] = lastY * 2. - backY;
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatQuadraticBezierTo(num[0], num[1], num[2], num[3], path));
					backX = num[0];		backY = num[1];
					lastX = num[2];		lastY = num[3];
				}
				break;
			case 't':																				/* Relative continuous quadratic Bezier */
				while (ParseDoubles(&pathStr, 2, num+2)) {
					num[0] = lastX * 2. - backX;		num[1] = lastY * 2. - backY;
					num[2] += lastX;					num[3] += lastY;
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatQuadraticBezierTo(num[0], num[1], num[2], num[3], path));
					backX = num[0];		backY = num[1];
					lastX = num[2];		lastY = num[3];
				}
				break;
			case 'A':																				/* Absolute elliptical arc */
				while (ParseDoubles(&pathStr, 7, num)) {
					lastX = num[5];		lastY = num[6];
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentEllipticalArc(num[0], num[1], num[2] * D_RADIANS_PER_DEGREE, (num[3] != 0), (num[4] != 0), lastX, lastY, path));
				}
				break;
			case 'a':																				/* Relative elliptical arc */
				while (ParseDoubles(&pathStr, 7, num)) {
					lastX += num[5];		lastY += num[6];
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentEllipticalArc(num[0], num[1], num[2] * D_RADIANS_PER_DEGREE, (num[3] != 0), (num[4] != 0), lastX, lastY, path));
				}
				break;
			case 'Z':																				/* End path */
			case 'z':
				BAIL_IF_ERR(err = FskGrowablePathAppendSegmentClose(path));
				break;
			case 'G':																				/* End glyph (our extension) */
			case 'g':
				BAIL_IF_ERR(err = FskGrowablePathAppendSegmentEndGlyph(path));
				break;
			case 'K':																				/* Absolute conic (rational quadratic) Bezier (our extension) */
				while (ParseDoubles(&pathStr, 5, num)) {
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatRationalQuadraticBezierTo(num[0], num[1], num[2], num[3], num[4], path));	/* x, y, w, x, y */
					backX = num[0];		backY = num[1];
					lastX = num[3];		lastY = num[4];
				}
			case 'k':																				/* Absolute conic (rational quadratic) Bezier (our extension) */
				while (ParseDoubles(&pathStr, 5, num)) {
					num[0] += lastX;	num[1] += lastY;
					num[3] += lastX;	num[4] += lastY;
					BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatRationalQuadraticBezierTo(num[0], num[1], num[2], num[3], num[4], path));	/* x, y, w, x, y */
					backX = num[0];		backY = num[1];
					lastX = num[3];		lastY = num[4];
				}
				break;
			default:
				err = kFskErrBadData;
				break;
		}
	}
bail:
	return err;
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Curve Tessellation							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * DistanceFromSeparatedBezier2DBase
 ********************************************************************************/

static FskFixed
DistanceFromSeparatedBezier2DBase(const FskFixedPoint2D *first, const FskFixedPoint2D *latter, UInt32 order)
{
	FskFixed				t, d;
	FskFractVector2D		baseVec, skewVec;
	UInt32					i;
	const FskFixedPoint2D	*p;

	/* Compute unit vector along base */
	p = latter + order - 2;
	baseVec.x = p->x - first->x;
	baseVec.y = p->y - first->y;
	if (FskFixedVector2DNormalize(&baseVec.x)) {
		for (i = order - 2, p--, d = 0; i--; p--) {
			skewVec.x = p->x - first->x;
			skewVec.y = p->y - first->y;
			t = FskFractCrossProduct2D(&baseVec, &skewVec);
			if (t < 0)
				t = -t;
			if (d < t)	/* Find maximum distance of control points from the base */
				d = t;
		}
	}
	else {
		for (i = order - 2, p--, d = 0; i--; p--) {
			t = FskFixedHypot(p->x - first->x, p->y - first->y);
			if (d < t)	/* Find maximum distance of control points from the base */
				d = t;
		}
	}

	return(d);
}


/********************************************************************************
 * NumberOfSegmentsInFixedBezier2D
 ********************************************************************************/

static UInt32
NumberOfSegmentsInFixedBezier2D(
	const FskFixedPoint2D	*first,
	const FskFixedPoint2D	*latter,
	UInt32					order,
	FskFixed				tolerance
)
{
	return((FskFixedNSqrt(FskFixedNDiv(DistanceFromSeparatedBezier2DBase(first, latter, order), tolerance, 4), 4) + (1<<3))  >> 4);
}


/********************************************************************************
 * FskTessellateBezier2DRegularly
 ********************************************************************************/

FskErr
FskTessellateBezier2DRegularly(
	const FskFixedPoint2D			*first,
	const FskFixedPoint2D			*latter,
	UInt32							order,
	const FskFixedMatrix3x2			*M,
	FskGrowableFixedPoint2DArray	array
)
{
	FskFixedPoint2D			points[kFskMaxBezierOrder];
	FskFixed				dcj[2][(kFskMaxBezierOrder * (kFskMaxBezierOrder + 1)) >> 1];
	UInt32					i, j, n;
	FskErr					err;
	FskFixedPoint2D			pt;
	const FskFixedPoint2D	*p;

	if (M != NULL) {
		FskTransformFixedRowPoints2D(first,    1,     M, &points[0]);
		FskTransformFixedRowPoints2D(latter, order-1, M, &points[1]);
		first  = &points[0];
		latter = &points[1];
	}

	if ((n = NumberOfSegmentsInFixedBezier2D(first, latter, order, LINEAR_TOLERANCE)) > 1) {
		j = (order * (order - 1)) >> 1;
		dcj[0][j] = first->x;
		dcj[1][j] = first->y;
		for (i = order - 1, j++, p = latter; i--; j++, p++) {
			dcj[0][j] = p->x;	/* ignore false warning: array subscript is above array bounds */
			dcj[1][j] = p->y;	/* ignore false warning: array subscript is above array bounds */
		}

		{																/* Tessellate and append */
			FskFract		t, dt;
			for (i = n - 1, t = dt = FRACT_ONE / n; i--; t += dt) {
				FskFixedDeCasteljauKernel(dcj[0], order, t);
				FskFixedDeCasteljauKernel(dcj[1], order, t);
				pt.x = dcj[0][0];
				pt.y = dcj[1][0];
				BAIL_IF_ERR(err = FskGrowableArrayAppendItem(array, &pt));
			}
		}
	}

	err = FskGrowableArrayAppendItem(array, latter + order - 2);

bail:
	return(err);
}


/********************************************************************************
 * FskTessellateRationalBezier2DRegularly
 ********************************************************************************/

FskErr
FskTessellateRationalBezier2DRegularly(
	const FskFixedPoint2D			*first,
	const FskFixedPoint2D			*latter,
	const FskFract					*midWeights,
	UInt32							order,
	const FskFixedMatrix3x2			*M,
	FskGrowableFixedPoint2DArray	array
)
{
	FskErr					err;
	FskFixedPoint2D			points[kFskMaxBezierOrder];
	FskFixed				dcj[3][kFskMaxBezierOrder * (kFskMaxBezierOrder + 1) / 2];
	const FskFixedPoint2D	*p;
	const FskFract			*w;
	FskFract				weight;
	UInt32					i, j, n;
	FskFixedPoint2D			pt;


	if (M != NULL) {
		FskTransformFixedRowPoints2D(first,    1,     M, &points[0]);
		FskTransformFixedRowPoints2D(latter, order-1, M, &points[1]);
		first  = &points[0];
		latter = &points[1];
	}

	/* Convert from rational Bezier to homogeneous Bezier and put into deCasteljau */
	j = (order * (order - 1)) >> 1;
	dcj[0][j] = first->x;
	dcj[1][j] = first->y;
	dcj[2][j] = FRACT_ONE;
	for (i = order - 2, j++, p = latter, w = midWeights; i--; j++, p++, w++) {
		weight = *w;
		dcj[0][j] = FskFracMul(p->x, weight);
		dcj[1][j] = FskFracMul(p->y, weight);
		dcj[2][j] = weight;
	}
	dcj[0][j] = p->x;
	dcj[1][j] = p->y;
	dcj[2][j] = FRACT_ONE;

	if ((n = NumberOfSegmentsInFixedBezier2D(first, latter, order, LINEAR_TOLERANCE)) > 1) {
		FskFract	t, dt;			/* Tessellate and append */
		for (i = n - 1, t = dt = FRACT_ONE / n; i--; t += dt) {
			FskFixedDeCasteljauKernel(dcj[0], order, t);
			FskFixedDeCasteljauKernel(dcj[1], order, t);
			FskFixedDeCasteljauKernel(dcj[2], order, t);
			pt.x = FskFracDiv(dcj[0][0], dcj[2][0]);
			pt.y = FskFracDiv(dcj[1][0], dcj[2][0]);
			BAIL_IF_ERR(err = FskGrowableArrayAppendItem(array, &pt));
		}
	}

	err = FskGrowableArrayAppendItem(array, latter + order - 2);

bail:
	return(err);
}


/********************************************************************************
 * BisectBezier2DAdaptively
 ********************************************************************************/

static FskErr
BisectBezier2DAdaptively(const FskFixedPoint2D *points, UInt32 order, FskGrowableFixedPoint2DArray array)
{
	FskErr	err	= kFskErrNone;

	if (FskFixedDeviationOfBezierControlPoints2D(points, order) < (LINEAR_TOLERANCE*4)) {
		for (order--, points++; order--; points++) {
			BAIL_IF_ERR(err = FskGrowableArrayAppendItem(array, points));
		}
	}
	else {
		FskFixedPoint2D L[kFskMaxBezierOrder], R[kFskMaxBezierOrder];
		FskFixedBisectBezier(&points->x, order, 2, &L[0].x, &R[0].x);
		BAIL_IF_ERR(err = BisectBezier2DAdaptively(L, order, array));
		BAIL_IF_ERR(err = BisectBezier2DAdaptively(R, order, array));
	}

bail:
	return err;
}


/********************************************************************************
 * FskTessellateBezier2DAdaptively
 ********************************************************************************/

FskErr
FskTessellateBezier2DAdaptively(
	const FskFixedPoint2D		*first,
	const FskFixedPoint2D		*latter,
	UInt32						order,
	const FskFixedMatrix3x2		*M,
	FskGrowableFixedPoint2DArray array
)
{
	FskFixedPoint2D	points[kFskMaxBezierOrder], *p = points;
	UInt32			i;

	if (M == NULL) {
		*p++ = *first;
		for (i = order - 1; i--; )
			*p++ = *latter++;
	}
	else {
		FskTransformFixedRowPoints2D(first,    1,     M, &points[0]);
		FskTransformFixedRowPoints2D(latter, order-1, M, &points[1]);
	}

	return BisectBezier2DAdaptively(points, order, array);
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Rendering								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskPathTessellate
 ********************************************************************************/

FskErr
FskPathTessellate(
	FskConstPath			path,
	const FskFixedMatrix3x2	*M,
	FskGrowableArray		numPointArray,
	FskGrowableArray		ptArray,
	UInt32					*pEndCaps
) {
	FskErr					err		= kFskErrNone;
	const TaggedSegment		*segPtr;
	UInt32					prevNumPoints, numPoints, numSegs;
	FskFixedPoint2D			p0, pStart;


	pStart.x  = pStart.y = p0.x = p0.y = 0;
	numSegs   = GetPathSegmentCount(path);
	segPtr    = GetConstPathSegments(path);
	numPoints = numSegs;

	for (prevNumPoints = 0; numSegs--; segPtr = NextConstTaggedPathSegment(segPtr)) {
		switch (segPtr->type) {
			case kFskPathSegmentNull:
				break;
			case kFskPathSegmentClose:
			{													/* BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &pStart)); not needed for polygons */
																if (p0.x == pStart.x && p0.y == pStart.y)
																	BAIL_IF_ERR(err = FskGrowableArraySetItemCount(ptArray, FskGrowableArrayGetItemCount(ptArray) - 1));	/* Remove duplicate point */
																else
																	p0 = pStart;
																if (pEndCaps) *pEndCaps |= kFskLineEndCapClosed;
			}	break;
			case kFskPathSegmentMoveTo:
			{	const TaggedSegmentMoveTo						*moveTo = (const TaggedSegmentMoveTo*)(segPtr);
																if ((numPoints = FskGrowableArrayGetItemCount(ptArray)) != 0) {						/* Latter contours */
																	numPoints -= prevNumPoints;
																	if (numPoints)
																		BAIL_IF_ERR(err = FskGrowableArrayAppendItem(numPointArray, &numPoints));	/* Record the number of points in the previous contour */
																	prevNumPoints += numPoints;
																}
																BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &moveTo->data.p));
																pStart = p0 = moveTo->data.p;
			}	break;
			case kFskPathSegmentLineTo:
			{	const TaggedSegmentLineTo						*lineTo = (const TaggedSegmentLineTo*)(segPtr);
																BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &lineTo->data.p));
																p0 = lineTo->data.p;
			}	break;
			case kFskPathSegmentQuadraticBezierTo:
			{	const TaggedSegmentQuadraticBezierTo			*quadTo = (const TaggedSegmentQuadraticBezierTo*)(segPtr);
																BAIL_IF_ERR(err = TessellateBezier2D(&p0, &quadTo->data.p1, 3, M, ptArray));
																p0 = quadTo->data.p;
			}	break;
			case kFskPathSegmentCubicBezierTo:
			{	const TaggedSegmentCubicBezierTo				*cubcTo = (const TaggedSegmentCubicBezierTo*)(segPtr);
																BAIL_IF_ERR(err = TessellateBezier2D(&p0, &cubcTo->data.p1, 4, M, ptArray));
																p0 = cubcTo->data.p;
			}	break;
			case kFskPathSegmentRationalQuadraticBezierTo:
			{	const TaggedSegmentRationalQuadraticBezierTo	*concTo = (const TaggedSegmentRationalQuadraticBezierTo*)(segPtr);
																BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&p0, &concTo->data.p1, &concTo->data.w1, 3, M, ptArray));
																p0 = concTo->data.p;
			}	break;
			case kFskPathSegmentEndGlyph:
			{													/* BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &pStart)); not needed for polygons */
																if ((numPoints = FskGrowableArrayGetItemCount(ptArray) - prevNumPoints) != 0) {
																	/* Record the number of points in the last contour */
																	if (numPoints)
																		BAIL_IF_ERR(err = FskGrowableArrayAppendItem(numPointArray, &numPoints));
																}
																pStart.x  = pStart.y = p0.x = p0.y = 0;
																prevNumPoints += numPoints;
			}	break;
			default:
				goto unknownSegmentType;
		}
	}

unknownSegmentType:
	if ((numPoints = FskGrowableArrayGetItemCount(ptArray) - prevNumPoints) != 0) {
		BAIL_IF_ERR(err = FskGrowableArrayAppendItem(numPointArray, &numPoints));	/* Record the number of points in the last contour */
	}

bail:
	return err;
}


/********************************************************************************
 * FskPathFill
 ********************************************************************************/

FskErr
FskPathFill(
	FskConstPath			path,
	const FskColorSource	*fillColor,
	SInt32					fillRule,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr							err;
	FskErr							renderErr		= kFskErrNothingRendered;
	FskGrowableArray				numPointArray	= NULL;
	FskGrowableArray				ptArray			= NULL;
	const TaggedSegment				*segPtr;
	UInt32							prevNumPoints, numPoints, numContours, *numPointList, numSegs;
	FskFixedPoint2D					p0, pStart, *points;
	FskColorSourceUnion				csu;
	FskFixedMatrix3x2				G;

	#ifdef LOG_PARAMETERS
		LOGD("PathFill(path=%p fillColor=%p fillRule=%s M=%p quality=%u clipRect=%p dstBM=%p", path, fillColor, FillRuleNameFromCode(fillRule), M, (unsigned)quality, clipRect, dstBM);
		LogPath(path, "path");
		LogColorSource(fillColor, "fillColor");
		LogFixedMatrix3x2(M, "M");
		LogRect(clipRect, "clipRect");
		LogBitmap(dstBM, "dstBM");
	#endif /* LOG_PARAMETERS */

	pStart.x  = pStart.y = p0.x = p0.y = 0;
	numSegs   = GetPathSegmentCount(path);
	segPtr    = GetConstPathSegments(path);
	numPoints = numSegs;

	BAIL_IF_ERR(err = FskGrowableArrayNew(sizeof(UInt32), 10, &numPointArray));
	BAIL_IF_ERR(err = FskGrowableArrayNew(sizeof(FskFixedPoint2D), numPoints * 10, &ptArray));

	for (prevNumPoints = 0; numSegs--; segPtr = NextConstTaggedPathSegment(segPtr)) {
		switch (segPtr->type) {
			case kFskPathSegmentNull:
				break;
			case kFskPathSegmentClose:
			{													/* BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &pStart)); not needed for polygons */
																if (p0.x == pStart.x && p0.y == pStart.y && (numPoints - prevNumPoints) > 1)
																	BAIL_IF_ERR(err = FskGrowableArraySetItemCount(ptArray, FskGrowableArrayGetItemCount(ptArray) - 1));	/* Remove duplicate point */
																else
																	p0 = pStart;
			}	break;
			case kFskPathSegmentMoveTo:
			{	const TaggedSegmentMoveTo						*moveTo = (const TaggedSegmentMoveTo*)(segPtr);
																if ((numPoints = FskGrowableArrayGetItemCount(ptArray)) != 0) {						/* Latter contours */
																	numPoints -= prevNumPoints;
																	if (numPoints)
																		BAIL_IF_ERR(err = FskGrowableArrayAppendItem(numPointArray, &numPoints));	/* Record the number of points in the previous contour */
																	prevNumPoints += numPoints;
																}
																BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &moveTo->data.p));
																pStart = p0 = moveTo->data.p;
			}	break;
			case kFskPathSegmentLineTo:
			{	const TaggedSegmentLineTo						*lineTo = (const TaggedSegmentLineTo*)(segPtr);
																BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &lineTo->data.p));
																p0 = lineTo->data.p;
			}	break;
			case kFskPathSegmentQuadraticBezierTo:
			{	const TaggedSegmentQuadraticBezierTo			*quadTo = (const TaggedSegmentQuadraticBezierTo*)(segPtr);
																BAIL_IF_ERR(err = TessellateBezier2D(&p0, &quadTo->data.p1, 3, M, ptArray));
																p0 = quadTo->data.p;
			}	break;
			case kFskPathSegmentCubicBezierTo:
			{	const TaggedSegmentCubicBezierTo				*cubcTo = (const TaggedSegmentCubicBezierTo*)(segPtr);
																BAIL_IF_ERR(err = TessellateBezier2D(&p0, &cubcTo->data.p1, 4, M, ptArray));
																p0 = cubcTo->data.p;
			}	break;
			case kFskPathSegmentRationalQuadraticBezierTo:
			{	const TaggedSegmentRationalQuadraticBezierTo	*concTo = (const TaggedSegmentRationalQuadraticBezierTo*)(segPtr);
																BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&p0, &concTo->data.p1, &concTo->data.w1, 3, M, ptArray));
																p0 = concTo->data.p;
			}	break;
			case kFskPathSegmentEndGlyph:
			{													/* BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &pStart)); not needed for polygons */
																if ((numPoints = FskGrowableArrayGetItemCount(ptArray) - prevNumPoints) != 0) {
																	/* Record the number of points in the last contour */
																	if (numPoints)
																		BAIL_IF_ERR(err = FskGrowableArrayAppendItem(numPointArray, &numPoints));
																}
																if ((numContours = FskGrowableArrayGetItemCount(numPointArray)) > 0) {
																	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(numPointArray, 0, (void**)(void*)(&numPointList)));
																	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(ptArray,       0, (void**)(void*)(&points)));
																	#ifdef LOG_CONTOURS
																		LogContours(numContours, numPointList, points);
																	#endif /* LOG_CONTOURS */
																	if ((err = FskFillPolygonContours(numContours, numPointList, points, FskTransformColorSource(fillColor, M, &G, &csu, NULL),
																		fillRule, NULL, quality, clipRect, dstBM)) < renderErr
																	)
																		if ((renderErr = err) < kFskErrNone)
																			goto bail;
																}
																FskGrowableArraySetItemCount(numPointArray, 0);
																FskGrowableArraySetItemCount(ptArray,       0);
																pStart.x  = pStart.y = p0.x = p0.y = prevNumPoints = 0;
			}	break;
			default:
				goto unknownSegmentType;
		}
	}

unknownSegmentType:
	if ((numPoints = FskGrowableArrayGetItemCount(ptArray) - prevNumPoints) != 0) {
		BAIL_IF_ERR(err = FskGrowableArrayAppendItem(numPointArray, &numPoints));	/* Record the number of points in the last contour */
	}
	if ((numContours = FskGrowableArrayGetItemCount(numPointArray)) > 0) {
		BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(numPointArray, 0, (void**)(void*)(&numPointList)));
		BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(ptArray,       0, (void**)(void*)(&points)));
		#ifdef LOG_CONTOURS
			LogContours(numContours, numPointList, points);
		#endif /* LOG_CONTOURS */
		if ((err = FskFillPolygonContours(numContours, numPointList, points, FskTransformColorSource(fillColor, M, &G, &csu, NULL), fillRule, NULL, quality, clipRect, dstBM)) < renderErr)
			renderErr = err;
	}
	if (err >= kFskErrNone)
		err = renderErr;

bail:
	if (numPointArray != NULL)	FskGrowableArrayDispose(numPointArray);
	if (ptArray       != NULL)	FskGrowableArrayDispose(ptArray);
	return err;
}


/********************************************************************************
 * FskPathFrame
 ********************************************************************************/

FskErr
FskPathFrame(
	FskConstPath			path,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	UInt32					endCaps,
	const FskColorSource	*frameColor,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr							err;
	FskErr							renderErr	= kFskErrNothingRendered;
	FskGrowableArray				ptArray		= NULL;
	FskFixed						*ndsh		= NULL;		/* New dash if needed */
	const TaggedSegment				*segPtr;
	UInt32							n, nSegs;
	FskFixedPoint2D					p0, pStart, *points;
	FskColorSourceUnion				csu;
	FskFixedMatrix3x2				G;

	#ifdef LOG_PARAMETERS
		LOGD("PathFrame(path=%p strokeWidth=%g jointSharpness=%g endCaps=%s frameColor=%p M=%p quality=%u clipRect=%p dstBM=%p",
			path, strokeWidth/65536., jointSharpness/65536., EndCapsNameFromCode(endCaps), frameColor, M, (unsigned)quality, clipRect, dstBM);
		LogPath(path, "path");
		LogColorSource(frameColor, "frameColor");
		LogFixedMatrix3x2(M, "M");
		LogRect(clipRect, "clipRect");
		LogBitmap(dstBM, "dstBM");
	#endif /* LOG_PARAMETERS */

	pStart.x  = pStart.y = p0.x = p0.y = 0;
	nSegs     = GetPathSegmentCount(path);
	segPtr    = GetConstPathSegments(path);

	BAIL_IF_ERR(err = FskGrowableArrayNew(sizeof(FskFixedPoint2D), nSegs * 10, &ptArray));

	if (M)	strokeWidth = ScaleStrokeWidth(strokeWidth, M);

	for ( ; nSegs--; segPtr = NextConstTaggedPathSegment(segPtr)) {
		switch (segPtr->type) {
			case kFskPathSegmentNull:
				break;
			case kFskPathSegmentClose:
			{													BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &pStart));
																if (p0.x == pStart.x && p0.y == pStart.y)
																	BAIL_IF_ERR(err = FskGrowableArraySetItemCount(ptArray, FskGrowableArrayGetItemCount(ptArray) - 1));	/* Remove duplicate point */
																else
																	p0 = pStart;
																endCaps |= kFskLineEndCapClosed;
			}	break;
			case kFskPathSegmentEndGlyph:
			{													/* BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &pStart)); assume Close preceded it */
																if ((n = FskGrowableArrayGetItemCount(ptArray)) >= 2) {
																	BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayGetPointerToItem(ptArray, 0, (void**)(void*)(&points)));
																	if ((err = FskFramePolyLine(n, points, strokeWidth, jointSharpness, endCaps, frameColor, NULL, quality, clipRect, dstBM)) < renderErr)
																		if ((renderErr = err) < kFskErrNone)
																			goto bail;
																}
																FskGrowableArraySetItemCount(ptArray, 0);
																pStart.x  = pStart.y = p0.x = p0.y = 0;
			}	break;
			case kFskPathSegmentMoveTo:
			{	const TaggedSegmentMoveTo						*moveTo = (const TaggedSegmentMoveTo*)(segPtr);
																if ((n = FskGrowableArrayGetItemCount(ptArray)) >= 2) {
																	BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayGetPointerToItem(ptArray, 0, (void**)(void*)(&points)));
																	#ifdef LOG_CONTOURS
																		LogPolyline(n, points);
																	#endif /* LOG_CONTOURS */
																	if ((err = FskFramePolyLine(n, points, strokeWidth, jointSharpness, endCaps, frameColor, NULL, quality, clipRect, dstBM)) < renderErr)
																		if ((renderErr = err) < kFskErrNone)
																			goto bail;
																}
																FskGrowableArraySetItemCount(ptArray, 0);
																BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &moveTo->data.p));
																pStart = p0 = moveTo->data.p;
			}	break;
			case kFskPathSegmentLineTo:
			{	const TaggedSegmentLineTo						*lineTo = (const TaggedSegmentLineTo*)(segPtr);
																BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &lineTo->data.p));
																p0 = lineTo->data.p;
			}	break;
			case kFskPathSegmentQuadraticBezierTo:
			{	const TaggedSegmentQuadraticBezierTo			*quadTo = (const TaggedSegmentQuadraticBezierTo*)(segPtr);
																BAIL_IF_ERR(err = TessellateBezier2D(&p0, &quadTo->data.p1, 3, M, ptArray));
																p0 = quadTo->data.p;
			}	break;
			case kFskPathSegmentCubicBezierTo:
			{	const TaggedSegmentCubicBezierTo				*cubcTo = (const TaggedSegmentCubicBezierTo*)(segPtr);
																BAIL_IF_ERR(err = TessellateBezier2D(&p0, &cubcTo->data.p1, 4, M, ptArray));
																p0 = cubcTo->data.p;
			}	break;
			case kFskPathSegmentRationalQuadraticBezierTo:
			{	const TaggedSegmentRationalQuadraticBezierTo	*concTo = (const TaggedSegmentRationalQuadraticBezierTo*)(segPtr);
																BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&p0, &concTo->data.p1, &concTo->data.w1, 3, M, ptArray));
																p0 = concTo->data.p;
			}	break;
			default:
				goto unknownSegmentType;
		}
	}

unknownSegmentType:
	if ((n = FskGrowableFixedPoint2DArrayGetItemCount(ptArray)) >= 2) {
		BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayGetPointerToItem(ptArray, 0, (void**)(void*)(&points)));
		#ifdef LOG_CONTOURS
			LogPolyline(n, points);
		#endif /* LOG_CONTOURS */
		if ((err = FskFramePolyLine(n, points, strokeWidth, jointSharpness, endCaps, FskTransformColorSource(frameColor, M, &G, &csu, &ndsh), NULL, quality, clipRect, dstBM)) < renderErr)
			renderErr = err;
	}
	if (err >= kFskErrNone)
		err = renderErr;

bail:
	if (ndsh    != NULL)	FskMemPtrDispose(ndsh);
	if (ptArray != NULL)	FskGrowableFixedPoint2DArrayDispose(ptArray);
	return err;
}


/********************************************************************************
 * OpaqueColorSource
 ********************************************************************************/

static Boolean
OpaqueColorSource(const FskColorSource *cs)
{
	FskColorSourceUnion	*csu	= (FskColorSourceUnion*)cs;

	if (csu->so.dashCycles > 0)
		return 0;

	switch (csu->so.type) {
		case kFskColorSourceTypeConstant:
			if (csu->cn.color.a != 255)
				return 0;
			break;
		case kFskColorSourceTypeLinearGradient:
		case kFskColorSourceTypeRadialGradient:
		{	const FskGradientStop *st;
			UInt32 i;
			for (i = csu->lg.numStops, st = csu->lg.gradientStops; i--; st++) {
				if (st->color.a != 255)
					return 0;
			}
		}	break;
		case kFskColorSourceTypeTexture:
			return 0;	/* Antialiasing computations are minimal compared to texturing */
			break;
	}

	return 1;
}


/********************************************************************************
 * FskPathFrameFill
 ********************************************************************************/

FskErr
FskPathFrameFill(
	FskConstPath			path,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	UInt32					endCaps,
	const FskColorSource	*frameColor,
	const FskColorSource	*fillColor,
	SInt32					fillRule,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr							err;
	FskErr							renderErr		= kFskErrNothingRendered;
	FskGrowableArray				numPointArray	= NULL;
	FskGrowableArray				ptArray			= NULL;
	FskFixed						*ndsh			= NULL;		/* New dash if needed */
	const TaggedSegment				*segPtr;
	UInt32							prevNumPoints, numPoints, numContours, *numPointList, numSegs;
	FskFixedPoint2D					p0, pStart, *points;
	FskColorSourceUnion				csu;
	FskFixedMatrix3x2				G;

	pStart.x  = pStart.y = p0.x = p0.y = 0;
	numSegs   = GetPathSegmentCount(path);
	segPtr    = GetConstPathSegments(path);
	numPoints = numSegs;

	BAIL_IF_ERR(err = FskGrowableArrayNew(sizeof(UInt32), 10, &numPointArray));
	BAIL_IF_ERR(err = FskGrowableArrayNew(sizeof(FskFixedPoint2D), numPoints * 10, &ptArray));

	for (prevNumPoints = 0; numSegs--; segPtr = NextConstTaggedPathSegment(segPtr)) {
		switch (segPtr->type) {
			case kFskPathSegmentNull:
				break;
			case kFskPathSegmentClose:
			{													BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &pStart));
																p0 = pStart;
																endCaps |= kFskLineEndCapClosed;
			}	break;
			case kFskPathSegmentMoveTo:
			{	const TaggedSegmentMoveTo						*moveTo = (const TaggedSegmentMoveTo*)(segPtr);
																if ((numPoints = FskGrowableArrayGetItemCount(ptArray)) != 0) {					/* Latter contours */
																	numPoints -= prevNumPoints;
																	BAIL_IF_ERR(err = FskGrowableArrayAppendItem(numPointArray, &numPoints));	/* Record the number of points in the previous contour */
																	prevNumPoints += numPoints;
																}
																BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &moveTo->data.p));
																pStart = p0 = moveTo->data.p;
			}	break;
			case kFskPathSegmentLineTo:
			{	const TaggedSegmentLineTo						*lineTo = (const TaggedSegmentLineTo*)(segPtr);
																BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &lineTo->data.p));
																p0 = lineTo->data.p;
			}	break;
			case kFskPathSegmentQuadraticBezierTo:
			{	const TaggedSegmentQuadraticBezierTo			*quadTo = (const TaggedSegmentQuadraticBezierTo*)(segPtr);
																BAIL_IF_ERR(err = TessellateBezier2D(&p0, &quadTo->data.p1, 3, M, ptArray));
																p0 = quadTo->data.p;
			}	break;
			case kFskPathSegmentCubicBezierTo:
			{	const TaggedSegmentCubicBezierTo				*cubcTo = (const TaggedSegmentCubicBezierTo*)(segPtr);
																BAIL_IF_ERR(err = TessellateBezier2D(&p0, &cubcTo->data.p1, 4, M, ptArray));
																p0 = cubcTo->data.p;
			}	break;
			case kFskPathSegmentRationalQuadraticBezierTo:
			{	const TaggedSegmentRationalQuadraticBezierTo	*concTo = (const TaggedSegmentRationalQuadraticBezierTo*)(segPtr);
																BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&p0, &concTo->data.p1, &concTo->data.w1, 3, M, ptArray));
																p0 = concTo->data.p;
			}	break;
			case kFskPathSegmentEndGlyph:
			{													/* BAIL_IF_ERR(err = AppendTransformedPointToGrowableArray(ptArray, M, &pStart)); assume Close preceded it */
																if ((numPoints = FskGrowableArrayGetItemCount(ptArray) - prevNumPoints) != 0) {
																	BAIL_IF_ERR(err = FskGrowableArrayAppendItem(numPointArray, &numPoints));	/* Record the number of points in the last contour */
																}
																if ((numContours = FskGrowableArrayGetItemCount(numPointArray)) > 0) {
																	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(numPointArray, 0, (void**)(void*)(&numPointList)));
																	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(ptArray,       0, (void**)(void*)(&points)));
																	#define GLYPH_FRAME_THEN_FILL
																	#ifdef GLYPH_FRAME_THEN_FILL
																	if (quality > 0) {
																		if (M)	strokeWidth = ScaleStrokeWidth(strokeWidth, M);
																		if ((err = FskFramePolylineContours(numContours, numPointList, points, strokeWidth, jointSharpness, endCaps,
																			FskTransformColorSource(frameColor, M, &G, &csu, &ndsh), NULL, quality, clipRect, dstBM)) < 0
																		)
																			goto bail;
																		prevNumPoints = err;		/* FskFramePolylineContours() returns <0: if, ==0 if successful, >0 if nothing rendered */
																		if ((err = FskFillPolygonContours(  numContours, numPointList, points, FskTransformColorSource(fillColor, M, &G, &csu, NULL),
																			fillRule, NULL, quality, clipRect, dstBM)) < renderErr
																		)
																			if ((renderErr = err) < kFskErrNone)
																				goto bail;
																	} else
																	#endif /* !GLYPH_FRAME_THEN_FILL */
																	{
																		if ((err = FskFillPolygonContours(  numContours, numPointList, points, FskTransformColorSource(fillColor, M, &G, &csu, NULL),
																			fillRule, NULL, 0, clipRect, dstBM)) < renderErr
																		)
																			if ((renderErr = err) < kFskErrNone)
																				goto bail;
																		if (M)	strokeWidth = ScaleStrokeWidth(strokeWidth, M);
																		if ((err = FskFramePolylineContours(numContours, numPointList, points, strokeWidth, jointSharpness, endCaps,
																			FskTransformColorSource(frameColor, M, &G, &csu, &ndsh), NULL, quality, clipRect, dstBM)) < renderErr
																		)
																			if ((renderErr = err) < kFskErrNone)
																				goto bail;
																	}
																	err &= prevNumPoints;	/* Merge kFskErrNone and kFskErrNothingRendered */
																}
																FskGrowableArraySetItemCount(numPointArray, 0);
																FskGrowableArraySetItemCount(ptArray,       0);
																pStart.x  = pStart.y = p0.x = p0.y = prevNumPoints = 0;
			}	break;
			default:
				goto unknownSegmentType;
		}
	}

unknownSegmentType:
	if ((numPoints = FskGrowableArrayGetItemCount(ptArray) - prevNumPoints) != 0) {
		BAIL_IF_ERR(err = FskGrowableArrayAppendItem(numPointArray, &numPoints));	/* Record the number of points in the last contour */
	}
	if ((numContours = FskGrowableArrayGetItemCount(numPointArray)) > 0) {
		UInt32	fillQual;
		if (	((fillQual = quality) > 0)
			&&	((endCaps & kFskLineEndCapClosed) != 0)
			&&	OpaqueColorSource(frameColor)
		)
			fillQual = 0;	/* We can use a jagged fill */
		BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(numPointArray, 0, (void**)(void*)(&numPointList)));
		BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(ptArray,       0, (void**)(void*)(&points)));
		if ((err = FskFillPolygonContours(numContours, numPointList, points, FskTransformColorSource(fillColor, M, &G, &csu, NULL),
			fillRule, NULL, fillQual, clipRect, dstBM)) < renderErr
		)
			if ((renderErr = err) < kFskErrNone)
				goto bail;
		if (M)	strokeWidth = ScaleStrokeWidth(strokeWidth, M);
		if ((err = FskFramePolylineContours(numContours, numPointList, points, strokeWidth, jointSharpness, endCaps,
			FskTransformColorSource(frameColor, M, &G, &csu, &ndsh), NULL, quality, clipRect, dstBM)) < renderErr
		)
			renderErr = err;
	}
	if (err >= kFskErrNone)
		err = renderErr;

bail:
	if (ndsh          != NULL)	FskMemPtrDispose(ndsh);
	if (numPointArray != NULL)	FskGrowableArrayDispose(numPointArray);
	if (ptArray       != NULL)	FskGrowableArrayDispose(ptArray);
	return err;
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 * SetRectPoints
 ********************************************************************************/

static FskErr
SetRectPoints(const FskShapeRect *rect, FskFixedPoint2D pts[4])
{
	FskErr err;

	if (rect->size.x <= 0) {
		err = (rect->size.x == 0) ? kFskErrNothingRendered : kFskErrInvalidParameter;
	}
	else if (rect->size.y <= 0) {
		err = (rect->size.y == 0) ? kFskErrNothingRendered : kFskErrInvalidParameter;
	}
	else {
		pts[0].x = pts[3].x = rect->origin.x;
		pts[1].x = pts[2].x = rect->origin.x + rect->size.x;
		pts[0].y = pts[1].y = rect->origin.y;
		pts[2].y = pts[3].y = rect->origin.y + rect->size.y;
		err = kFskErrNone;
	}
	return err;
}


/********************************************************************************
 * SetRoundedRectControlPoints
 *	10	11	0	1
 *	 9			2
 *	 8			3
 *	 7	 6	5	4
 ********************************************************************************/

static FskErr
SetRoundedRectControlPoints(const FskShapeRect *rect, const FskFixedMatrix3x2 *M, FskFixedPoint2D pts[12])
{
	FskErr		err;
	FskFixed	rx, ry;

	if (((rx = rect->radius.x) < 0) || ((ry = rect->radius.y) < 0)) {
		err = kFskErrInvalidParameter;
	}
	else if (rect->size.x <= 0) {
		err = (rect->size.x == 0) ? kFskErrNothingRendered : kFskErrInvalidParameter;
	}
	else if (rect->size.y <= 0) {
		err = (rect->size.y == 0) ? kFskErrNothingRendered : kFskErrInvalidParameter;
	}
	else {
		if      (rx == 0)	rx = ry;
		else if (ry == 0)	ry = rx;

		pts[ 7].x = pts[ 8].x = pts[ 9].x = pts[10].x	= rect->origin.x;
		pts[ 6].x = pts[11].x							= rect->origin.x + rx;
		pts[ 0].x = pts[ 5].x							= rect->origin.x + rect->size.x - rx;
		pts[ 1].x = pts[ 2].x = pts[ 3].x = pts[ 4].x	= rect->origin.x + rect->size.x;

		pts[ 0].y = pts[ 1].y = pts[10].y = pts[11].y	= rect->origin.y;
		pts[ 2].y = pts[ 9].y							= rect->origin.y + ry;
		pts[ 3].y = pts[ 8].y							= rect->origin.y + rect->size.y - ry;
		pts[ 4].y = pts[ 5].y = pts[ 6].y = pts[ 7].y	= rect->origin.y + rect->size.y;

		if (M)
			FskTransformFixedRowPoints2D(pts, 12, M, pts);

		err = kFskErrNone;
	}

	return err;
}


/********************************************************************************
 * TessellateRoundedRectControlPoints
 ********************************************************************************/

static FskErr
TessellateRoundedRectControlPoints(FskFixedPoint2D pts[12], FskGrowableFixedPoint2DArray *returnedArray)
{
	FskErr							err			= kFskErrNone;
	FskFixed						w			= FRACT_SQRT_HALF;
	FskGrowableFixedPoint2DArray	ptArray		= NULL;

	BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayNew(50, returnedArray));
	ptArray = *returnedArray;
	BAIL_IF_ERR(err = FskGrowableArrayAppendItem(ptArray,  &pts[ 0]));										/* Start of top segment */
	BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&pts[ 0], &pts[ 1], &w, 3, NULL, ptArray));	/* Upper right corner */
	BAIL_IF_ERR(err = FskGrowableArrayAppendItem(ptArray,  &pts[ 3]));										/* Right segment */
	BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&pts[ 3], &pts[ 4], &w, 3, NULL, ptArray));	/* Lower right corner */
	BAIL_IF_ERR(err = FskGrowableArrayAppendItem(ptArray,  &pts[ 6]));										/* Bottom segment */
	BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&pts[ 6], &pts[ 7], &w, 3, NULL, ptArray));	/* Lower left corner */
	BAIL_IF_ERR(err = FskGrowableArrayAppendItem(ptArray,  &pts[ 9]));										/* Left segment */
	BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&pts[ 9], &pts[10], &w, 3, NULL, ptArray));	/* Upper left corner */
	BAIL_IF_ERR(err = FskGrowableArrayAppendItem(ptArray,  &pts[ 0]));										/* Top segment */
bail:
	if (err != kFskErrNone) {	FskGrowableFixedPoint2DArrayDispose(ptArray); *returnedArray = NULL;	}
	return(err);
}


/********************************************************************************
 * FskFillShapeRect
 ********************************************************************************/

FskErr
FskFillShapeRect(
	const FskShapeRect		*rect,
	const FskColorSource	*fillColor,
 	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr							err;
	FskGrowableFixedPoint2DArray	ptArray;
	FskFixedPoint2D					pts[12], *points;
	UInt32							n;

	if ((rect->radius.x | rect->radius.y) == 0) {													/* Simple rectangle */
	#define OPTIMIZE_UPRIGHT_RECT_Q0
	#ifdef OPTIMIZE_UPRIGHT_RECT_Q0
		if ((quality == 0) && (fillColor->type == kFskColorSourceTypeConstant)) {					/* If low quality flat fill, ... */
			FskRectangleRecord	r;
			FskColorSourceUnion	*csu;
			FskColorRGBARecord	color;
			FskGraphicsModeParametersRecord params;
			if (M) {
				if (M->M[0][1] || M->M[1][0])														/* ... and is not rotated or skewed */
					goto generalRect;
				pts[0].x = FskFixMul(M->M[0][0], rect->origin.x) + M->M[2][0];						/* Apply scale and translation to origin */
				pts[0].y = FskFixMul(M->M[1][1], rect->origin.y) + M->M[2][1];
				pts[1].x = FskFixMul(M->M[0][0], rect->size.x);										/* Only scale the size */
				pts[1].y = FskFixMul(M->M[1][1], rect->size.y);
			}
			else {																					/* No transformation */
				pts[0]   = rect->origin;
				pts[1].x = rect->size.x;
				pts[1].y = rect->size.y;
			}
			if (pts[1].x < 0) {																		/* Negative width */
				pts[0].x += pts[1].x;
				pts[1].x = -pts[1].x;																/* Convert to positive */
			}
			if (pts[1].y < 0) {																		/* Negative height */
				pts[0].y += pts[1].y;
				pts[1].y = -pts[1].y;																/* Convert to positive */
			}
			pts[1].x += pts[0].x;																	/* Convert (width,height) to (x,y) */
			pts[1].y += pts[0].y;
			r.x = (pts[0].x + 0x8000) >> 16;														/* Convert from fixed point (x0,y0,x1,y1) ... */
			r.y = (pts[0].y + 0x8000) >> 16;
			r.width  = ((pts[1].x + 0xFFFF) >> 16) - r.x;											/* ... to integer (x,y,w,h) */
			r.height = ((pts[1].y + 0xFFFF) >> 16) - r.y;
			if (clipRect)	(void)FskRectangleIntersect(clipRect, &r, &r);
			csu = (FskColorSourceUnion*)fillColor;
			params.dataSize = sizeof(params);
			params.blendLevel = csu->cn.color.a;
			color.r = csu->cn.color.r; color.g = csu->cn.color.g; color.b = csu->cn.color.b; color.a = 255;
			err = FskRectangleFill(dstBM, &r, &color, kFskGraphicsModeCopy, &params);
		}
		else
	generalRect:
	#endif /* OPTIMIZE_UPRIGHT_RECT_Q0 */
		if ((err = SetRectPoints(rect, pts)) == kFskErrNone)											/* Non-degenerate */
			err = FskFillPolygon(4, pts, fillColor, kFskFillRuleNonZero, M, quality, clipRect, dstBM);
	}
	else if (((err = SetRoundedRectControlPoints(rect, M, pts)) == kFskErrNone) && ((err = TessellateRoundedRectControlPoints(pts, &ptArray)) == kFskErrNone)) {	/* Rounded rectangle */
		if ((err = FskGrowableFixedPoint2DArrayGetPointerToItem(ptArray, 0, (void**)(void*)(&points))) == kFskErrNone) {
			n = FskGrowableFixedPoint2DArrayGetItemCount(ptArray);
			err = FskFillPolygon(n, points, fillColor, kFskFillRuleNonZero, NULL, quality, clipRect, dstBM);
		}
		FskGrowableFixedPoint2DArrayDispose(ptArray);
	}
	return err;
}


/********************************************************************************
 * FskFrameShapeRect
 ********************************************************************************/

FskErr
FskFrameShapeRect(
	const FskShapeRect		*rect,
	FskFixed				strokeWidth,
	const FskColorSource	*frameColor,
 	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr							err;
	FskGrowableFixedPoint2DArray	ptArray;
	FskFixedPoint2D					pts[12], *points;
	UInt32							n;
	const FskFixed					jointSharpness		= kFskLineJoinMiter90;

	if (((rect->radius.x | rect->radius.y) == 0) && ((err = SetRectPoints(rect, pts)) == kFskErrNone)) {															/* Simple rectangle */
		err = FskFramePolygon(4, pts, strokeWidth, jointSharpness, frameColor, M, quality, clipRect, dstBM);
	}
	else if (((err = SetRoundedRectControlPoints(rect, M, pts)) == kFskErrNone) && ((err = TessellateRoundedRectControlPoints(pts, &ptArray)) == kFskErrNone)) {	/* Rounded rectangle */
		if ((err = FskGrowableFixedPoint2DArrayGetPointerToItem(ptArray, 0, (void**)(void*)(&points))) == kFskErrNone) {
			n = FskGrowableFixedPoint2DArrayGetItemCount(ptArray);
			if (M)	strokeWidth = ScaleStrokeWidth(strokeWidth, M);
			err = FskFramePolygon(n, points, strokeWidth, jointSharpness, frameColor, NULL, quality, clipRect, dstBM);
		}
		FskGrowableFixedPoint2DArrayDispose(ptArray);
	}
	return err;
}


/********************************************************************************
 * FskFrameFillShapeRect
 ********************************************************************************/

FskErr
FskFrameFillShapeRect(
	const FskShapeRect		*rect,
	FskFixed				strokeWidth,
	const FskColorSource	*frameColor,
	const FskColorSource	*fillColor,
 	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr							err;
	FskGrowableFixedPoint2DArray	ptArray;
	FskFixedPoint2D					pts[12], *points;
	UInt32							n;
	const FskFixed					jointSharpness		= kFskLineJoinMiter90;

	if (((rect->radius.x | rect->radius.y) == 0) && ((err = SetRectPoints(rect, pts)) == kFskErrNone)) {															/* Simple rectangle */
		if ((err = FskFillPolygon( 4, pts, fillColor, kFskFillRuleNonZero,         M, quality, clipRect, dstBM)) >= 0)
			err = FskFramePolygon(4, pts, strokeWidth, jointSharpness, frameColor, M, quality, clipRect, dstBM);
	}
	else if (((err = SetRoundedRectControlPoints(rect, M, pts)) == kFskErrNone) && ((err = TessellateRoundedRectControlPoints(pts, &ptArray)) == kFskErrNone)) {	/* Rounded rectangle */
		if ((err = FskGrowableFixedPoint2DArrayGetPointerToItem(ptArray, 0, (void**)(void*)(&points))) == kFskErrNone) {
			n = FskGrowableFixedPoint2DArrayGetItemCount(ptArray);
			if ((err = FskFillPolygon(n, points, fillColor, kFskFillRuleNonZero,          NULL, quality, clipRect, dstBM)) >= 0) {
				if (M)	strokeWidth = ScaleStrokeWidth(strokeWidth, M);
				err = FskFramePolygon(n, points, strokeWidth, jointSharpness, frameColor, NULL, quality, clipRect, dstBM);
			}
		}
		FskGrowableFixedPoint2DArrayDispose(ptArray);
	}
	return err;
}


/********************************************************************************
 * FskGrowablePathAppendShapeRect
 ********************************************************************************/

FskErr
FskGrowablePathAppendShapeRect(const FskShapeRect *rect, FskGrowablePath path)
{
	FskErr							err;
	FskFixedPoint2D					pts[12];

	if ((rect->radius.x | rect->radius.y) == 0) {																		/* Simple rectangle */
		BAIL_IF_ERR(err = SetRectPoints(rect, pts));
		(void)FskGrowablePathAppendSegmentMoveTo(pts[0].x, pts[0].y, path);
		(void)FskGrowablePathAppendSegmentLineTo(pts[1].x, pts[1].y, path);
		(void)FskGrowablePathAppendSegmentLineTo(pts[2].x, pts[2].y, path);
		(void)FskGrowablePathAppendSegmentLineTo(pts[3].x, pts[3].y, path);
		err = FskGrowablePathAppendSegmentClose(path);
	}
	else {																												/* Rounded rectangle */
		FskFixed w = FRACT_SQRT_HALF;
		BAIL_IF_ERR(err = SetRoundedRectControlPoints(rect, NULL, pts));
		FskGrowablePathAppendSegmentMoveTo(							pts[ 0].x, pts[ 0].y, 								path);
		(void)FskGrowablePathAppendSegmentRationalQuadraticBezierTo(pts[ 1].x, pts[ 1].y, w,	pts[ 2].x, pts[ 2].y,	path);
		(void)FskGrowablePathAppendSegmentLineTo(					pts[ 3].x, pts[ 3].y,								path);
		(void)FskGrowablePathAppendSegmentRationalQuadraticBezierTo(pts[ 4].x, pts[ 4].y, w,	pts[ 5].x, pts[ 5].y,	path);
		(void)FskGrowablePathAppendSegmentLineTo(					pts[ 6].x, pts[ 6].y,								path);
		(void)FskGrowablePathAppendSegmentRationalQuadraticBezierTo(pts[ 7].x, pts[ 7].y, w,	pts[ 8].x, pts[ 8].y,	path);
		(void)FskGrowablePathAppendSegmentLineTo(					pts[ 9].x, pts[ 9].y,								path);
		(void)FskGrowablePathAppendSegmentRationalQuadraticBezierTo(pts[10].x, pts[10].y, w,	pts[11].x, pts[11].y,	path);
		err = FskGrowablePathAppendSegmentClose(path);
	}
bail:
	return err;
}




#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 * SetEllipsePoints
 ********************************************************************************/

static FskErr
SetEllipsePoints(const FskShapeEllipse *lips, const FskFixedMatrix3x2 *M, FskFixedPoint2D pts[9])
{
	FskErr	err;

	if (lips->radius.x <= 0) {
		err = (lips->radius.x == 0) ? kFskErrNothingRendered : kFskErrInvalidParameter;
	}
	else if (lips->radius.y <= 0) {
		err = (lips->radius.y == 0) ? kFskErrNothingRendered : kFskErrInvalidParameter;
	}
	else {
		pts[1].x = pts[2].x = pts[3].x 				= lips->center.x + lips->radius.x;
		pts[0].x = pts[4].x = pts[8].x				= lips->center.x;
		pts[5].x = pts[6].x = pts[7].x				= lips->center.x - lips->radius.x;

		pts[7].y = pts[0].y = pts[1].y = pts[8].y	= lips->center.y - lips->radius.y;
		pts[2].y = pts[6].y							= lips->center.y;
		pts[3].y = pts[4].y = pts[5].y				= lips->center.y + lips->radius.y;

		if (M != NULL)
			FskTransformFixedRowPoints2D(pts, 9, M, pts);

		err = kFskErrNone;
	}
	return err;
}


/********************************************************************************
 * TessellateEllipseControlPoints
 ********************************************************************************/

static FskErr
TessellateEllipseControlPoints(FskFixedPoint2D pts[9], FskGrowableFixedPoint2DArray *returnedArray)
{
	FskErr							err			= kFskErrNone;
	FskFixed						w			= FRACT_SQRT_HALF;
	FskGrowableFixedPoint2DArray	ptArray		= NULL;

	BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayNew(50, returnedArray));
	ptArray = *returnedArray;
	BAIL_IF_ERR(err = FskGrowableArrayAppendItem(ptArray,  &pts[ 0]));										/* Start of top segment */
	BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&pts[ 0], &pts[ 1], &w, 3, NULL, ptArray));	/* Upper right corner */
	BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&pts[ 2], &pts[ 3], &w, 3, NULL, ptArray));	/* Lower right corner */
	BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&pts[ 4], &pts[ 5], &w, 3, NULL, ptArray));	/* Lower left corner */
	BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&pts[ 6], &pts[ 7], &w, 3, NULL, ptArray));	/* Upper left corner */
bail:
	if (err != kFskErrNone) {	FskGrowableFixedPoint2DArrayDispose(ptArray); *returnedArray = NULL;	}
	return(err);
}


/********************************************************************************
 * FskFillShapeEllipse
 ********************************************************************************/

FskErr
FskFillShapeEllipse(
	const FskShapeEllipse	*lips,
	const FskColorSource	*fillColor,
 	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr							err;
	FskGrowableFixedPoint2DArray	ptArray;
	FskFixedPoint2D					pts[9], *points;
	UInt32							n;

	if (((err = SetEllipsePoints(lips, M, pts)) == kFskErrNone) && ((err = TessellateEllipseControlPoints(pts, &ptArray)) == kFskErrNone)) {
		if ((err = FskGrowableFixedPoint2DArrayGetPointerToItem(ptArray, 0, (void**)(void*)(&points))) == kFskErrNone) {
			n = FskGrowableFixedPoint2DArrayGetItemCount(ptArray);
			err = FskFillPolygon( n, points,  fillColor, kFskFillRuleNonZero, NULL, quality, clipRect, dstBM);
		}
		FskGrowableFixedPoint2DArrayDispose(ptArray);
	}
	return err;
}


/********************************************************************************
 * FskFrameShapeEllipse
 ********************************************************************************/

FskErr
FskFrameShapeEllipse(
	const FskShapeEllipse	*lips,
	FskFixed				strokeWidth,
	const FskColorSource	*frameColor,
 	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr							err;
	FskGrowableFixedPoint2DArray	ptArray;
	FskFixedPoint2D					pts[9], *points;
	UInt32							n;
	const FskFixed					jointSharpness		= kFskLineJoinMiter90;

	if (((err = SetEllipsePoints(lips, M, pts)) == kFskErrNone) && ((err = TessellateEllipseControlPoints(pts, &ptArray)) == kFskErrNone)) {
		if ((err = FskGrowableFixedPoint2DArrayGetPointerToItem(ptArray, 0, (void**)(void*)(&points))) == kFskErrNone) {
			n = FskGrowableFixedPoint2DArrayGetItemCount(ptArray);
			if (M)	strokeWidth = ScaleStrokeWidth(strokeWidth, M);
			err = FskFramePolygon(n, points, strokeWidth, jointSharpness, frameColor, NULL, quality, clipRect, dstBM);
		}
		FskGrowableFixedPoint2DArrayDispose(ptArray);
	}
	return err;
}


/********************************************************************************
 * FskFrameFillShapeEllipse
 ********************************************************************************/

FskErr
FskFrameFillShapeEllipse(
	const FskShapeEllipse	*lips,
	FskFixed				strokeWidth,
	const FskColorSource	*frameColor,
	const FskColorSource	*fillColor,
 	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr							err;
	FskGrowableFixedPoint2DArray	ptArray;
	FskFixedPoint2D					pts[9], *points;
	UInt32							n;
	const FskFixed					jointSharpness		= kFskLineJoinMiter90;

	if (((err = SetEllipsePoints(lips, M, pts)) == kFskErrNone) && ((err = TessellateEllipseControlPoints(pts, &ptArray)) == kFskErrNone)) {
		if ((err = FskGrowableFixedPoint2DArrayGetPointerToItem(ptArray, 0, (void**)(void*)(&points))) == kFskErrNone) {
			n = FskGrowableFixedPoint2DArrayGetItemCount(ptArray);
			if ((err = FskFillPolygon(n, points, fillColor, kFskFillRuleNonZero,          NULL, quality, clipRect, dstBM)) >= 0) {
				if (M)	strokeWidth = ScaleStrokeWidth(strokeWidth, M);
				err = FskFramePolygon(n, points, strokeWidth, jointSharpness, frameColor, NULL, quality, clipRect, dstBM);
			}
		}
		FskGrowableFixedPoint2DArrayDispose(ptArray);
	}
	return err;
}


/********************************************************************************
 * FskGrowablePathAppendShapeEllipse
 ********************************************************************************/

FskErr
FskGrowablePathAppendShapeEllipse(const FskShapeEllipse *lips, FskGrowablePath path)
{
	FskFixed		w		= FRACT_SQRT_HALF;
	FskFixedPoint2D pts[9];
	FskErr			err;

	BAIL_IF_ERR(err = SetEllipsePoints(lips, NULL, pts));
	FskGrowablePathAppendSegmentMoveTo(							pts[0].x, pts[0].y,							path);
	(void)FskGrowablePathAppendSegmentRationalQuadraticBezierTo(pts[1].x, pts[1].y, w,	pts[2].x, pts[2].y,	path);
	(void)FskGrowablePathAppendSegmentRationalQuadraticBezierTo(pts[3].x, pts[3].y, w,	pts[4].x, pts[4].y,	path);
	(void)FskGrowablePathAppendSegmentRationalQuadraticBezierTo(pts[5].x, pts[5].y, w,	pts[6].x, pts[6].y,	path);
	(void)FskGrowablePathAppendSegmentRationalQuadraticBezierTo(pts[7].x, pts[7].y, w,	pts[8].x, pts[8].y,	path);
	err = FskGrowablePathAppendSegmentClose(path);
bail:
	return err;
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 * FskFillShapeCircle
 ********************************************************************************/

FskErr
FskFillShapeCircle(
	const FskShapeCircle	*circ,
	const FskColorSource	*fillColor,
 	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskShapeEllipse lips;
	lips.center = circ->center;
	lips.radius.x = lips.radius.y = circ->radius;
	return FskFillShapeEllipse(&lips, fillColor, M, quality, clipRect, dstBM);
}


/********************************************************************************
 * FskFrameShapeCircle
 ********************************************************************************/

FskErr
FskFrameShapeCircle(
	const FskShapeCircle	*circ,
	FskFixed				strokeWidth,
	const FskColorSource	*frameColor,
 	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskShapeEllipse lips;
	lips.center = circ->center;
	lips.radius.x = lips.radius.y = circ->radius;
	return FskFrameShapeEllipse(&lips, strokeWidth, frameColor, M, quality, clipRect, dstBM);
}


/********************************************************************************
 * FskFrameFillShapeCircle
 ********************************************************************************/

FskErr
FskFrameFillShapeCircle(
	const FskShapeCircle	*circ,
	FskFixed				strokeWidth,
	const FskColorSource	*frameColor,
	const FskColorSource	*fillColor,
 	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskShapeEllipse lips;
	lips.center = circ->center;
	lips.radius.x = lips.radius.y = circ->radius;
	return FskFrameFillShapeEllipse(&lips, strokeWidth, frameColor, fillColor, M, quality, clipRect, dstBM);
}


/********************************************************************************
 * FskGrowablePathAppendShapeCircle
 ********************************************************************************/

FskErr
FskGrowablePathAppendShapeCircle(const FskShapeCircle *circ, FskGrowablePath path)
{
	FskShapeEllipse lips;
	lips.center = circ->center;
	lips.radius.x = lips.radius.y = circ->radius;
	return FskGrowablePathAppendShapeEllipse(&lips, path);
}


/*******************************************************************************
 * FskTessellateCircularArcStartCenterAngle
 *	Don't draw the first point, but do draw the last point.
 *******************************************************************************/

FskErr
FskTessellateCircularArcStartCenterAngle(
	const FskFixedPoint2D			*start,
	const FskFixedPoint2D			*center,
	FskFixedDegrees					angle,
	FskFract						sqrtRelTol,	/* Square root of the relative tolerance = sqrt(tolerance / radius) */
	FskGrowableFixedPoint2DArray	array
)
{
	FskErr				err		= kFskErrNone;
	FskFixedVector2D	v;
	FskFixed			dt;
	FskFract			cs[2];
	FskFixedPoint2D		pt;
	SInt32				n;

	v.x = start->x - center->x;
	v.y = start->y - center->y;
	if (sqrtRelTol <= 0)
		sqrtRelTol = FskFracSqrt(FskFracDiv(LINEAR_TOLERANCE, FskFixedVectorNorm(&v.x, 2)));
	dt = FskFracMul(sqrtRelTol, 0x00A20E93);
	if ((n = angle) < 0) n = -n;
	n = (FskFixed)((((((FskInt64)n) << 16) / dt) + 0xFFFF) >> 16);
	dt = angle / n;
	FskFracCosineSine(dt, cs);

	if ((angle == (180 << 16)) || (angle == (-180 << 16)))	/* Exactly 180 degrees */
		n--;

	for ( ; n-- > 0; ) {

		dt  = FskFixedNLinear2D(v.x, cs[0],   -v.y, cs[1],   30);
		v.y = FskFixedNLinear2D(v.x, cs[1],    v.y, cs[0],   30);
		v.x = dt;
		pt.x = center->x + v.x;
		pt.y = center->y + v.y;
		BAIL_IF_ERR(err = FskGrowableArrayAppendItem(array, &pt));
	}

	if ((angle == (180 << 16)) || (angle == (-180 << 16))) {	/* Exactly 180 degrees - avoid accumulation of error */
		pt.x = 2 * center->x - start->x;
		pt.y = 2 * center->y - start->y;
		BAIL_IF_ERR(err = FskGrowableArrayAppendItem(array, &pt));
	}

bail:
	return err;
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Bounding Box							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * AddXToBoundingBox
 ********************************************************************************/

static void
AddXToBoundingBox(FskFixed x, FskFixedPoint2D box[2])
{
	if (box[0].x > x)
		box[0].x = x;
	if (box[1].x < x)
		box[1].x = x;
}


/********************************************************************************
 * AddYToBoundingBox
 ********************************************************************************/

static void
AddYToBoundingBox(FskFixed y, FskFixedPoint2D box[2])
{
	if (box[0].y > y)
		box[0].y = y;
	if (box[1].y < y)
		box[1].y = y;
}


/********************************************************************************
 * AddPointsToBoundingBox
 ********************************************************************************/

static void
AddPointsToBoundingBox(unsigned numPoints, const FskFixedPoint2D *pt, FskFixedPoint2D box[2])
{
	for (; numPoints--; ++pt) {
		if (box[0].x > pt->x)
			box[0].x = pt->x;
		if (box[1].x < pt->x)
			box[1].x = pt->x;
		if (box[0].y > pt->y)
			box[0].y = pt->y;
		if (box[1].y < pt->y)
			box[1].y = pt->y;
	}
}


/********************************************************************************
 * IsXInBoundingBox, IsYInBoundingBox
 ********************************************************************************/

#define IsXInBoundingBox(xx, box)	(((xx) >= (box)[0].x) && ((xx) <= (box)[1].x))
#define IsYInBoundingBox(yy, box)	(((yy) >= (box)[0].y) && ((yy) <= (box)[1].y))


#if 0
/********************************************************************************
 * IsPointInBoundingBox
 ********************************************************************************/

static UInt32
IsPointInBoundingBox(const FskFixedPoint2D *pt, FskFixedPoint2D box[2])
{
	return (UInt32)(IsXInBoundingBox(pt->x, box) && IsYInBoundingBox(pt->y, box));
}
#endif


/********************************************************************************
 * EvaluateBezierExtrema
 ********************************************************************************/

static UInt32
EvaluateBezierExtrema(const FskFixed x0, const FskFixed *x, SInt32 stride, UInt32 order, FskFixed *fPtr, FskFract *tPtr)
{
	FskFixed	dcj[kFskMaxBezierOrder * (kFskMaxBezierOrder + 1) / 2];
	FskFixed	*b, *d;
	FskFixed	p, q, r;
	FskFract	t;
	UInt32		n;

	/* Initialize deCasteljau coordinates */
	b = dcj + ((order * (order-1)) >> 1);
	*b++ = x0;
	for (n = order - 1; n--; b++, x += stride)
		*b = *x;
	b -= order;

	/* Compute deCasteljau derivative coordinates */
	d = b - order + 1;
	FskFixedDerivativeOfBezier(b, d, order);

	/* Solve for derivative == 0 */
	n = 0;	/* Initially, no extrema within 0 <= t <= 1 */
	switch (order) {
		case 3:	/* Quadratic */
			p = d[0];
			if ((r = d[0] - d[1]) < 0) {						/* Make denominator positive to facilitate range checking of t */
				p = -p;
				r = -r;
			}
			if ((r != 0) && (p >= 0) && (p <= r)) {				/* Parametric value of extremum is in range */
				t = FskFracDiv(p, r);							/* Find where the derivative is zero */
				n = 1;											/* Found 1 extremum */
				if (tPtr != NULL)
					*tPtr = t;									/* Return parametric value of extrema, if requested */
				if (fPtr != NULL) {
					FskFixedDeCasteljauKernel(dcj, order, t);	/* Evlauate the function at that point */
					*fPtr = dcj[0];								/* return value of function at extremum */
				}
			}
			break;

		case 4:	/* Cubic */
			q = FskFixedSqrt64to32((FskInt64)(d[1]) * d[1] - (FskInt64)(d[0]) * d[2]);	/* Conjugate part */
			if (q >= 0) {																/* Conjugate part is real */
				p = d[0] - d[1];														/* Real part */
				if ((r = d[0] - (d[1] << 1) + d[2]) < 0) {								/* Denominator */
					p = -p;
					q = -q;
					r = -r;
				}

				t = p - q;											/* "Smaller" root (assuming q > 0) */
				if ((r != 0) && (t >= 0) && (t <= r)) {				/* Parametric value of extremum is in range */
					n++;											/* Found an extremum */
					t = FskFracDiv(t, r);							/* Compute parametric value of extremum */
					if (tPtr != NULL)
						*tPtr++ = t;								/* Return parametric value is requested */
					if (fPtr != NULL) {
						FskFixedDeCasteljauKernel(dcj, order, t);	/* Evaluate the function extremum at that point */
						*fPtr++ = dcj[0];							/* Return function value if requested */
					}
				}

				t = p + q;											/* "Larger" root (assuming q > 0) */
				if ((t >= 0) && (t <= r)) {
					n++;											/* Found an extremum */
					t = FskFracDiv(t, r);							/* Compute parametric value of zero slope */
					if (tPtr != NULL)
						*tPtr++ = t;								/* Return parametric value is requested */
					if (fPtr != NULL) {
						FskFixedDeCasteljauKernel(dcj, order, t);	/* Evaluate the function at that point */
						*fPtr++ = dcj[0];							/* Return function value if requested */
					}
				}
			}

			break;

		default:
			break;	/* Not implemented */
	}

	return n;
}


/********************************************************************************
 * EvaluateHomogeneousBezier
 ********************************************************************************/

static FskFixed
EvaluateHomogeneousBezier(FskFixed *xdcj, FskFixed *wdcj, UInt32 order, FskFract t)
{
	FskFixedDeCasteljauKernel(xdcj, order, t);
	FskFixedDeCasteljauKernel(wdcj, order, t);
	return FskFracDiv(xdcj[0], wdcj[0]);
}



/********************************************************************************
 * EvaluateRationalQuadraticBezierExtrema
 ********************************************************************************/

static UInt32
EvaluateRationalQuadraticBezierExtrema(const FskFixed x0, const FskFixed *x, FskFract w1, SInt32 stride, FskFixed *fPtr, FskFract *tPtr)
{
	FskFixed	dcj[2][kFskMaxBezierOrder * (kFskMaxBezierOrder + 1) / 2];
	FskFixed	*b, *d;
	FskFixed	p, q, r, x1, x2;
	FskFract	t;
	UInt32		n;
	const UInt32 order = 3;

	/* Initialize deCasteljau coordinates, converting from rational to homogeneous */
	b = dcj[0] + ((order * (order-1)) >> 1);
	b[0] = x0;
	b[1] = FskFracMul(w1, x1 = x[0]);
	b[2] = x2 = x[stride];
	d = dcj[1] + ((order * (order-1)) >> 1);
	d[0] = FRACT_ONE;
	d[1] = w1;
	d[2] = FRACT_ONE;

	/* Solve for derivative == 0: (p +/- q) / r */
	n = 0;																							/* Initially, no extrema within 0 <= t <= 1 */

	if ((p = x2 - x0) == 0) {										/* Symmetric case */
		t = FRACT_ONE / 2;																			/* Extremum at t = 1/2 */
	oneRoot:
		n = 1;
		if (tPtr != NULL)	*tPtr++ = t;															/* Return parametric value of extrema, if requested */
		if (fPtr != NULL)	*fPtr++ = EvaluateHomogeneousBezier(dcj[0], dcj[1], order, t);			/* Return value of function at extremum, if requested */
	}
	else if ((r = FRACT_ONE - w1) == 0) {							/* Non-rational case */
		t = FskFracDiv(x0 - x1, x0 - (x1 << 1) + x2);
		goto oneRoot;
	}
	else {															/* Two roots case */
		r = FskFracMul(r,  p      ) << 1;															/* Denominator */
		t = FskFracMul(w1, x0 - x1) << 1;
		p = p + t;																					/* Real part */
		q = FskFixedSqrt64to32((FskInt64)p * p - (FskInt64)t * r);									/* Conjugate part */
		if (q >= 0) {																				/* Conjugate part is real: real roots */
			if (r < 0) {																			/* Make denominator positive in order to simplify domain check */
				p = -p;
				q = -q;
				r = -r;
			}

			t = p - q;																				/* "Smaller" root" */
			if ((t >= 0) && (t <= r)) {																/* t will be within 0 and 1 */
				n++;
				t = FskFracDiv(t, r);
				if (tPtr != NULL)	*tPtr++ = t;													/* Return parametric value of extrema, if requested */
				if (fPtr != NULL)	*fPtr++ = EvaluateHomogeneousBezier(dcj[0], dcj[1], order, t);	/* Return value of function at extremum, if requested */
			}

			t = p + q;																				/* "Larger" root" */
			if ((t >= 0) && (t <= r)) {																/* t will be within 0 and 1 */
				n++;
				t = FskFracDiv(t, r);
				if (tPtr != NULL)	*tPtr++ = t;													/* Return parametric value of extrema, if requested */
				if (fPtr != NULL)	*fPtr++ = EvaluateHomogeneousBezier(dcj[0], dcj[1], order, t);	/* Return value of function at extremum, if requested */
			}
		}
	}

	return n;
}


/****************************************************************************//**
 * AddQuadraticBezierToBoundingBox.
 *	\param[in]	p0	pointer to the starting point.
 *	\param[in]	p12	pointer to the middle and end points, 2 althogether.
 *	\param[out]	box	the resulting bounding box: { {minX, minY}, {maxX, maxY} }.
 ********************************************************************************/

static void
AddQuadraticBezierToBoundingBox(const FskFixedPoint2D *p0, const FskFixedPoint2D *p12, FskFixedPoint2D box[2])
{
	FskFixed	f;

	AddPointsToBoundingBox(1, &p12[1], box);											/* Add the last point, because the curve passes through it. */
	if (	!IsXInBoundingBox(p12[0].x, box)											/* If the middle point X is outside of the current bounding box */
		&&	(EvaluateBezierExtrema(p0->x, &p12[0].x, sizeof(FskFixedPoint2D) / sizeof(FskFixed), 3, &f, NULL) > 0)	/* Look for an extremun */
	)
		AddXToBoundingBox(f, box);														/* If an extremum was found in X, add it. */
	if (	!IsYInBoundingBox(p12[0].y, box)											/* If the middle point Y is outside of the current bounding box */
		&&	(EvaluateBezierExtrema(p0->y, &p12[0].y, sizeof(FskFixedPoint2D) / sizeof(FskFixed), 3, &f, NULL) > 0)	/* Look for an extremun */
	)
		AddYToBoundingBox(f, box);														/* If an extremum was found in Y, add it. */
}


/****************************************************************************//**
 * AddCubicBezierToBoundingBox.
 *	\param[in]	p0		pointer to the starting point.
 *	\param[in]	p123	pointer to the 2 middle points and 1 end point, 3 althogether.
 *	\param[out]	box		the resulting bounding box: { {minX, minY}, {maxX, maxY} }.
 ********************************************************************************/

static void
AddCubicBezierToBoundingBox(const FskFixedPoint2D *p0, const FskFixedPoint2D *p123, FskFixedPoint2D box[2])
{
	FskFixed	f[2];
	UInt32		n;

	AddPointsToBoundingBox(1, &p123[2], box);											/* Add the last point, because the curve passes through it. */

	if ( (	!(IsXInBoundingBox(p123[0].x, box) && IsXInBoundingBox(p123[1].x, box))	)	/* If either of the two middle points Xs are outside of the current bounding box */
		&&	((n = EvaluateBezierExtrema(p0->x, &p123[0].x, sizeof(FskFixedPoint2D) / sizeof(FskFixed), 4, f, NULL)) > 0)	/* Look for extrema */
	) {
		AddXToBoundingBox(f[0], box);													/* If 1 or 2 extrema were found in X, add them. */
		if (n > 1) AddXToBoundingBox(f[1], box);
	}

	if ( (	!(IsYInBoundingBox(p123[0].y, box) && IsYInBoundingBox(p123[1].y, box))	)	/* If either of the two middle points Ys are outside of the current bounding box */
		&&	((n = EvaluateBezierExtrema(p0->y, &p123[0].y, sizeof(FskFixedPoint2D) / sizeof(FskFixed), 4, f, NULL)) > 0)	/* Look for extrema */
	) {
		AddYToBoundingBox(f[0], box);													/* If 1 or 2 extrema were found in Y, add them. */
		if (n > 1) AddYToBoundingBox(f[1], box);
	}
}


/****************************************************************************//**
 * AddRationalQuadraticBezierToBoundingBox
 *	\param[in]	p0		pointer to the starting point.
 *	\param[in]	w1		the weight associated with point 1.
 *	\param[in]	p12		pointer to the middle point and end point, 2 althogether.
 *	\param[out]	box		the resulting bounding box: { {minX, minY}, {maxX, maxY} }.
 ********************************************************************************/

static void
AddRationalQuadraticBezierToBoundingBox(const FskFixedPoint2D *p0, FskFract w1, const FskFixedPoint2D *p12, FskFixedPoint2D box[2])
{
	FskFixed	f[2];
	UInt32		n;

	AddPointsToBoundingBox(1, &p12[1], box);											/* Add the last point, because the curve passes through it. */

	if (	(!IsXInBoundingBox(p12[0].x, box) || (w1 < 0))								/* If the middle point X is outside of the current bounding box, or w<0 */
		&&	((n = EvaluateRationalQuadraticBezierExtrema(p0->x, &p12[0].x, w1, sizeof(FskFixedPoint2D) / sizeof(FskFixed), f, NULL)) > 0)	/* Look for extrema */
	) {
		AddXToBoundingBox(f[0], box);													/* If 1 or 2 extrema were found in X, add them. */
		if (n > 1) AddXToBoundingBox(f[1], box);
	}
	if (	(!IsYInBoundingBox(p12[0].y, box) || (w1 < 0))								/* If the middle point Y is outside of the current bounding box, or w<0 */
		&&	((n = EvaluateRationalQuadraticBezierExtrema(p0->y, &p12[0].y, w1, sizeof(FskFixedPoint2D) / sizeof(FskFixed), f, NULL)) > 0)	/* Look for extrema */
	) {
		AddYToBoundingBox(f[0], box);													/* If 1 or 2 extrema were found in Y, add them. */
		if (n > 1) AddYToBoundingBox(f[1], box);
	}
}


/********************************************************************************
 * FskPathComputeBoundingBox
 ********************************************************************************/

FskErr
FskPathComputeBoundingBox(FskConstPath path, const FskFixedMatrix3x2 *M, Boolean tight, FskFixedRectangle bBox)
{
	FskErr				err		= kFskErrNone;
	const TaggedSegment	*segPtr;
	UInt32				numSegs;
	FskFixedPoint2D		p0, points[3], box[2];
	FskFixedMatrix3x2	T;

	p0.x = p0.y = 0;
	box[0].x = box[0].y = 0x7FFFFFFF;
	box[1].x = box[1].y = 0x80000000;
	numSegs  = GetPathSegmentCount(path);
	segPtr   = GetConstPathSegments(path);
	if (NULL == M) {											/* This should be called with a matrix, ... */
		M = &T;													/* ... but just in case it is not, ... */
		T.M[0][0] = T.M[1][1] = FIXED_ONE;						/* ... we use the identity matrix. */
		T.M[0][1] = T.M[1][0] = T.M[2][0] = T.M[2][1] = 0;
	}

	for ( ; numSegs--; segPtr = NextConstTaggedPathSegment(segPtr)) {
		switch (segPtr->type) {
			case kFskPathSegmentMoveTo:
			{	const TaggedSegmentMoveTo						*moveTo = (const TaggedSegmentMoveTo*)(segPtr);
																FskTransformFixedRowPoints2D(&moveTo->data.p, 1, M, points);
																AddPointsToBoundingBox(1, points, box);
																p0 = points[0];
			}	break;
			case kFskPathSegmentLineTo:
			{	const TaggedSegmentLineTo						*lineTo = (const TaggedSegmentLineTo*)(segPtr);
																FskTransformFixedRowPoints2D(&lineTo->data.p, 1, M, points);
																AddPointsToBoundingBox(1, points, box);							/* Line stays between its endpoints */
																p0 = points[0];
			}	break;
			case kFskPathSegmentQuadraticBezierTo:
			{	const TaggedSegmentQuadraticBezierTo			*quadTo = (const TaggedSegmentQuadraticBezierTo*)(segPtr);
																FskTransformFixedRowPoints2D(&quadTo->data.p1, 2, M, points);
																if (tight)	AddQuadraticBezierToBoundingBox(&p0, points, box);	/* Solve for exact extremum */
																else		AddPointsToBoundingBox(2, points, box);				/* Quadratic Bezier stays in its convex hull */
																p0 = points[1];
			}	break;
			case kFskPathSegmentCubicBezierTo:
			{	const TaggedSegmentCubicBezierTo				*cubcTo = (const TaggedSegmentCubicBezierTo*)(segPtr);
																FskTransformFixedRowPoints2D(&cubcTo->data.p1, 3, M, points);
																if (tight)	AddCubicBezierToBoundingBox(&p0, points, box);		/* Solve for exact extrema */
																else		AddPointsToBoundingBox(3, points, box);				/* Cubic Bezier stays in its convex hull */
																p0 = points[2];
			}	break;
			case kFskPathSegmentRationalQuadraticBezierTo:
			{	const TaggedSegmentRationalQuadraticBezierTo	*concTo = (const TaggedSegmentRationalQuadraticBezierTo*)(segPtr);
																FskTransformFixedRowPoints2D(&concTo->data.p1, 2, M, points);
																if (tight || (concTo->data.w1 < 0))	AddRationalQuadraticBezierToBoundingBox(&p0, concTo->data.w1, points, box);
																else		AddPointsToBoundingBox(2, points, box);				/* Conic Bezier stays in its convex hull when w >= 0 */
																p0 = points[1];
			}	break;
			default:
				break;
		}
	}

	/* Add an extra 1/65536 pixel border to ensure bounds, being careful about overflow */
	if (box[0].x > MIN_S32)	--box[0].x;							/* Add an extra 1/65536 pixel to ensure bounds, ... */
	if (box[1].x < MAX_S32)	++box[1].x;							/* ... being careful about wraparound. */
	if (box[0].y > MIN_S32)	--box[0].y;							/* Add an extra 1/65536 pixel to ensure bounds, ... */
	if (box[1].y < MAX_S32)	++box[1].y;							/* ... being careful about wraparound. */

	/* Convert from {x0,y0,x1,y1} to {x0,x1,width,height}, being careful about overflow */
	if (box[0].x <= box[1].x) {									/* As it should be: min <= max */
		if ((bBox->width = box[1].x - box[0].x ) >= 0) {		/* As it should be: positive width */
			bBox->x = box[0].x;
		}
		else {													/* Wraparound overflow */
			bBox->x     = MIN_RECT_COORD;						/* Max out range, centered on the origin */
			bBox->width = MAX_RECT_SIZE;						/* MAX_RECT_SIZE indicates infinite interval, irrespective of the value of x */
		}
	}
	else {														/* min > max means NULL rect */
		bBox->x     = 0;
		bBox->width = 0;
	}
	if (box[0].y <= box[1].y) {									/* As it should be: min <= max */
		if ((bBox->height = box[1].y - box[0].y ) >= 0) {		/* As it should be: positive width */
			bBox->y = box[0].y;
		}
		else {													/* Wraparound overflow */
			bBox->y      = MIN_RECT_COORD;						/* Max out range, centered on the origin */
			bBox->height = MAX_RECT_SIZE;						/* MAX_RECT_SIZE indicates infinite interval, irrespective of the value of of y */
		}
	}
	else {														/* min > max means NULL rect */
		bBox->y     = 0;
		bBox->height = 0;
	}

	return err;
}


/********************************************************************************
 * FskPathIsAxisAlignedIntegralRectangle
 ********************************************************************************/

Boolean
FskPathIsAxisAlignedIntegralRectangle(FskConstPath path, const FskFixedMatrix3x2 *M, FskRectangle r)
{
	const TaggedSegment	*segPtr;
	UInt32				numSegs;
	FskFixedPoint2D		points[5];

	numSegs = GetPathSegmentCount(path);
	if (numSegs < 4 || numSegs > 5)
		return false;

	segPtr = GetConstPathSegments(path);															/* Get segment 1 */
	if (segPtr->type != kFskPathSegmentMoveTo)
		return false;
	points[0] = ((const TaggedSegmentMoveTo*)segPtr)->data.p;										/* MoveTo(point 0) */

	segPtr = NextConstTaggedPathSegment(segPtr);													/* Get segment 2 */
	if (segPtr->type != kFskPathSegmentLineTo)
		return false;
	points[1] = ((const TaggedSegmentMoveTo*)segPtr)->data.p;										/* LineTo(point 1) */

	segPtr = NextConstTaggedPathSegment(segPtr);													/* Get segment 3 */
	if (segPtr->type != kFskPathSegmentLineTo)
		return false;
	points[2] = ((const TaggedSegmentMoveTo*)segPtr)->data.p;										/* LineTo(point 2) */

	segPtr = NextConstTaggedPathSegment(segPtr);													/* Get segment 4 */
	if (segPtr->type != kFskPathSegmentLineTo)
		return false;
	points[3] = ((const TaggedSegmentMoveTo*)segPtr)->data.p;										/* LineTo(point 3) */

	/* Fill will do an auto-close, so no explicit close is necessary; this is the case if there are only 4 segments.
	 * With 5 segments, the extra segment could be a close or another line, yielding the following representations for a rectangle:
	 *		M L L L
	 *		M L L L Z	(preferred)
	 *		M L L L L
	 */
	if (numSegs == 5) {																				/* Look for an explicit close (preferred) */
		segPtr = NextConstTaggedPathSegment(segPtr);												/* Get segment 5 */
		if (segPtr->type != kFskPathSegmentClose) {													/* Look for a duplicate of the first point */
			if (segPtr->type != kFskPathSegmentLineTo)
				return false;
			points[4] = ((const TaggedSegmentMoveTo*)segPtr)->data.p;
			if (points[4].x != points[0].x || points[4].y != points[0].y)
				return false;
		}
	}

	/* The path has the right topology. Now check that it has the right geometry */

	if (M != NULL &&																					/* If a transformation is given, and ... */
		(	r != NULL ||																				/* ... either the rectangle is requested, or ... */
			(((M->M[0][0] | M->M[1][1] | M->M[2][0] | M->M[2][1]) & 0xFFFF) | M->M[0][1] | M->M[1][0])	/* ... the transformation can introduce non-integer coordinates, ... */
		)
	)
		FskTransformFixedRowPoints2D(points, 4, M, points);												/* ... transform the points (sufficient but not necessary) */

	if (points[0].x == points[1].x) {
		if ((points[3].x - points[2].x) | (points[0].y - points[3].y) | (points[1].y - points[2].y))
			return false;																			/* Not an aligned rectangle */
	}
	else if (points[0].y == points[1].y) {
		if ((points[3].y - points[2].y) | (points[0].x - points[3].x) | (points[1].x - points[2].x))
			return false;																			/* Not an aligned rectangle */
	}
	else {
		return false;																				/* Not an aligned rectangle */
	}

	if ((points[0].x | points[0].y | points[1].x | points[1].y | points[2].x | points[2].y | points[3].x | points[3].y) & 0x7FFF)
		return false;																				/* Not integral or half-integral coordinates */

	if (r) {																						/* If the rectangle was requested, compute it. */
		if ((r->width  = ((points[2].x + 0x8000) >> 16) - (r->x = (points[0].x + 0x8000) >> 16)) < 0) { r->x += r->width;  r->width  = -r->width;  }
		if ((r->height = ((points[2].y + 0x8000) >> 16) - (r->y = (points[0].y + 0x8000) >> 16)) < 0) { r->y += r->height; r->height = -r->height; }
	}

	return true;
}

#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Arc Length								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * LengthOfPolyLine
 ********************************************************************************/

static FskFixed
LengthOfPolyLine(FskFixedPoint2D *pts, int nPts)
{
	FskFixed len;

	for (len = 0; --nPts > 0; pts++)
		len += FskFixedDistance(&pts[0].x, &pts[1].x, 2);

	return len;
}


/********************************************************************************
 * SubdivideBezierForArcLength
 ********************************************************************************/

static FskFixed
SubdivideBezierForArcLength(FskFixedPoint2D *B, int order, FskFixed d0, int logNumEvals, FskFixed **eval)
{
	FskFixedPoint2D L[kFskMaxBezierOrder], R[kFskMaxBezierOrder];

	if (logNumEvals-- > 0) {
		FskFixedBisectBezier(&B->x, order, 2, &L->x, &R->x);
		d0 = SubdivideBezierForArcLength(L, order, d0, logNumEvals, eval);
		d0 = SubdivideBezierForArcLength(R, order, d0, logNumEvals, eval);
	}
	else {
		*(*eval)++ = d0 += (	LengthOfPolyLine(B, order)						/* Upper bound */
							+	FskFixedDistance(&B[0].x, &B[+order-1].x, 2)	/* Lower bound */
							) >> 1;												/* Averaged */
	}

	return d0;
}


/********************************************************************************
 * SubdivideHomogeneousBezierForArcLength
 ********************************************************************************/

static FskFixed
SubdivideHomogeneousBezierForArcLength(FskFixedVector3D *B, int order, FskFixed d0, int logNumEvals, FskFixed **eval)
{
	FskFixedVector3D L[kFskMaxBezierOrder], R[kFskMaxBezierOrder];

	if (logNumEvals-- > 0) {
		FskFixedBisectBezier(&B->x, order, 3, &L->x, &R->x);
		d0 = SubdivideHomogeneousBezierForArcLength(L, order, d0, logNumEvals, eval);
		d0 = SubdivideHomogeneousBezierForArcLength(R, order, d0, logNumEvals, eval);
	}
	else {
		union Fixed3D2DPtr { FskFixedVector3D *p3; FskFixedPoint2D *p2; };
		union Fixed3D2DPtr B2 = { L };	/* reuse the 3d storage for 2D */
		int i;
		for (i = 0; i < order; i++) {
			B2.p2[i].x = FskFracDiv(B[i].x, B[i].z);
			B2.p2[i].y = FskFracDiv(B[i].y, B[i].z);
		}
		*(*eval)++ = d0 += (	LengthOfPolyLine(B2.p2, order)						/* Upper bound */
							+	FskFixedDistance(&B2.p2[0].x, &B2.p2[order-1].x, 2)	/* Lower bound */
							) >> 1;													/* Averaged */
	}

	return d0;
}


/********************************************************************************
 * EvaluateArcLengthOfNull - evaluated within the context of a Path
 ********************************************************************************/

static void
EvaluateArcLengthOfNull(const TaggedSegment *seg, int logNumEvals, FskFixed *eval)
{
	UNUSED(seg);
	for (logNumEvals = 1 << logNumEvals; logNumEvals--; )
		*eval++ = 0;
}


/********************************************************************************
 * EvaluateArcLengthOfClose - evaluated within the context of a Path
 ********************************************************************************/

#define EvaluateArcLengthOfClose EvaluateArcLengthOfNull


/********************************************************************************
 * EvaluateArcLengthOfEndGlyph - evaluated within the context of a Path
 ********************************************************************************/

#define EvaluateArcLengthOfEndGlyph EvaluateArcLengthOfNull


/********************************************************************************
 * EvaluateArcLengthOfMoveTo - evaluated within the context of a Path
 ********************************************************************************/

#define EvaluateArcLengthOfMoveTo EvaluateArcLengthOfNull


/********************************************************************************
 * EvaluateArcLengthOfLineTo - evaluated within the context of a Path
 ********************************************************************************/

static void
EvaluateArcLengthOfLineTo(const TaggedSegment *aSeg, int logNumEvals, FskFixed *eval)
{
	const TaggedSegmentLineTo *seg	= (const TaggedSegmentLineTo*)aSeg;
	FskFixed len, del;

	len = FskFixedDistance(&PreviousPointToSegment(seg).x, &seg->data.p.x, 2);
	del = len >> logNumEvals;
	for (eval += (logNumEvals = 1 << logNumEvals); logNumEvals--; len -= del)
		*--eval = len;
}


/********************************************************************************
 * EvaluateArcLengthOfQuadraticBezierTo - evaluated within the context of a Path
 ********************************************************************************/

static void
EvaluateArcLengthOfQuadraticBezierTo(const TaggedSegment *aSeg, int logNumEvals, FskFixed *eval)
{
	const TaggedSegmentQuadraticBezierTo *seg = (const TaggedSegmentQuadraticBezierTo*)aSeg;
	FskFixedPoint2D B[3];

	B[0] = PreviousPointToSegment(seg);
	B[1] = seg->data.p1;
	B[2] = seg->data.p;
	SubdivideBezierForArcLength(B, 3, 0, logNumEvals, &eval);
}


/********************************************************************************
 * EvaluateArcLengthOfCubicBezierTo - evaluated within the context of a Path
 ********************************************************************************/

static void
EvaluateArcLengthOfCubicBezierTo(const TaggedSegment *aSeg, int logNumEvals, FskFixed *eval)
{
	const TaggedSegmentCubicBezierTo *seg	= (const TaggedSegmentCubicBezierTo*)aSeg;
	FskFixedPoint2D B[4];

	B[0] = PreviousPointToSegment(seg);
	B[1] = seg->data.p1;
	B[2] = seg->data.p2;
	B[3] = seg->data.p;
	SubdivideBezierForArcLength(B, 4, 0, logNumEvals, &eval);
}


/********************************************************************************
 * EvaluateArcLengthOfRationalQuadraticBezierTo - evaluated within the context of a Path
 ********************************************************************************/

static void
EvaluateArcLengthOfRationalQuadraticBezierTo(const TaggedSegment *aSeg, int logNumEvals, FskFixed *eval)
{
	const TaggedSegmentRationalQuadraticBezierTo *seg	= (const TaggedSegmentRationalQuadraticBezierTo*)aSeg;
	FskFixedVector3D B[3];

	B[0].x = PreviousPointToSegment(seg).x;				B[0].y = PreviousPointToSegment(seg).y;				B[0].z = FRACT_ONE;
	B[1].x = FskFracMul(seg->data.p1.x, seg->data.w1);	B[1].y = FskFracMul(seg->data.p1.y, seg->data.w1);	B[1].z = seg->data.w1;
	B[2].x = seg->data.p.x;								B[2].y = seg->data.p.y;								B[2].z = FRACT_ONE;
	SubdivideHomogeneousBezierForArcLength(B, 3, 0, logNumEvals, &eval);
}


/********************************************************************************
 * FskArcLengthTableRecord
 ********************************************************************************/

#define kLogNumArcEvals		3
#define kNumArcEvals		(1 << kLogNumArcEvals)
#define kNumSubSegs			(kNumArcEvals - 1)
#define kLastSubSeg			(kNumSubSegs - 1)
#define kSegLength			(kNumArcEvals - 1)

typedef struct FskArcLengthTableRecord {
	FskFract	toHere;
	UInt32		segOffset;
	FskFract	subSeg[(1 << kLogNumArcEvals) - 1];	/* 1/8, 2/8, 3/8, 4/8, 5/8, 6/8, 7/8 */
} FskArcLengthTableRecord;


/********************************************************************************
 * FskPathArcLengthTableNew
 ********************************************************************************/

FskErr
FskPathArcLengthTableNew(FskConstPath path, FskArcLengthTable *tab, FskFixed *totalLength)
{
	FskErr					err;
	UInt32					i, j, numSegs;
	FskArcLengthTableRecord	*t;
	const TaggedSegment		*seg, *segs;
	FskFixed				totLen;

	*tab = NULL;
	if (totalLength != NULL)	*totalLength = 0;
	BAIL_IF_ZERO((numSegs = GetPathSegmentCount(path)), err, kFskErrInvalidParameter);
	BAIL_IF_ERR(err = FskMemPtrNew((numSegs + 1) * sizeof(FskArcLengthTableRecord), (FskMemPtr*)tab));

	(**tab).toHere = 0;
	for (i = numSegs, t = *tab, seg = segs = GetConstPathSegments(path); i--; t++, seg = NextConstTaggedPathSegment(seg)) {
		t->segOffset = (UInt32)((const char*)seg - (const char*)segs);
		switch (seg->type) {
			case kFskPathSegmentNull:						EvaluateArcLengthOfNull(                     seg, kLogNumArcEvals, t->subSeg);	break;
			case kFskPathSegmentClose:						EvaluateArcLengthOfClose(                    seg, kLogNumArcEvals, t->subSeg);	break;
			case kFskPathSegmentMoveTo:						EvaluateArcLengthOfMoveTo(                   seg, kLogNumArcEvals, t->subSeg);	break;
			case kFskPathSegmentLineTo:						EvaluateArcLengthOfLineTo(                   seg, kLogNumArcEvals, t->subSeg);	break;
			case kFskPathSegmentQuadraticBezierTo:			EvaluateArcLengthOfQuadraticBezierTo(        seg, kLogNumArcEvals, t->subSeg);	break;
			case kFskPathSegmentCubicBezierTo:				EvaluateArcLengthOfCubicBezierTo(            seg, kLogNumArcEvals, t->subSeg);	break;
			case kFskPathSegmentRationalQuadraticBezierTo:	EvaluateArcLengthOfRationalQuadraticBezierTo(seg, kLogNumArcEvals, t->subSeg);	break;
			case kFskPathSegmentEndGlyph:					EvaluateArcLengthOfEndGlyph(                 seg, kLogNumArcEvals, t->subSeg);	break;
			default:										break;
		}
		if ((&t->subSeg[0])[kSegLength] != 0)
			for (j = kNumSubSegs; j--; )
				t->subSeg[j] = FskFracDiv(t->subSeg[j], (&t->subSeg[0])[kSegLength]);	/* Normalize each segment's arc length */
		t[1].toHere += t[0].toHere;														/* Accumulate lengths of segments (t->subSeg[kSegLength] is aliased to t[1].toHere) */
	}
	t = &tab[0][numSegs];
	for (j = kNumSubSegs; j--; )
		t->subSeg[j] = FRACT_ONE;														/* Set for rational behavior for the last segment */
	totLen = tab[0][numSegs].toHere;
	for (i = numSegs + 1, t = *tab; i--; t++)
		t->toHere = FskFracDiv(t->toHere, totLen);										/* Normalize length */

	if (totalLength != NULL) *totalLength = totLen;										/* return total length if requested */

bail:
	return err;
}


/********************************************************************************
 * FskPathArcLengthTableDispose
 ********************************************************************************/

void
FskPathArcLengthTableDispose(FskArcLengthTable tab)
{
	(void)FskMemPtrDispose(tab);
}


/********************************************************************************
 * FskPathArcLengthTableGetDistancesOfSegment
 ********************************************************************************/

void
FskPathArcLengthTableGetDistancesOfSegment(FskConstArcLengthTable tab, UInt32 segIndex, FskFract startEnd[2])
{
	tab += segIndex;
	startEnd[0] = tab[0].toHere;
	startEnd[1] = tab[1].toHere;
}


/********************************************************************************
 * FskPathGetVisibleSegmentDistances
 ********************************************************************************/

FskErr
FskPathGetVisibleSegmentDistances(FskConstPath path, FskConstArcLengthTable tab, UInt32 *pNumDists, double **pDistances)
{
	FskErr				err;
	UInt32				numDists;
	const TaggedSegment	*segPtr;
	double				*distances;

	segPtr		= GetConstPathSegments(path);
	numDists	= FskPathGetVisibleSegmentCount(path) + 1;	/* Include a distance at the end */

	*pNumDists	= 0;
	*pDistances	= NULL;

	BAIL_IF_ERR(err = FskMemPtrNew(numDists * sizeof(double), (FskMemPtr*)(void*)(&distances)));
	*pNumDists  = numDists;
	*pDistances = distances;

	for (numDists-- ; numDists > 0; segPtr = NextConstTaggedPathSegment(segPtr), tab++) {
		switch (segPtr->type) {
			case kFskPathSegmentNull:
			case kFskPathSegmentMoveTo:
			case kFskPathSegmentEndGlyph:
				break;
			case kFskPathSegmentClose:
			case kFskPathSegmentLineTo:
			case kFskPathSegmentQuadraticBezierTo:
			case kFskPathSegmentCubicBezierTo:
			case kFskPathSegmentRationalQuadraticBezierTo:
				*distances++ = FskFractToFloat(tab->toHere);
				numDists--;
				break;
			default:			/* Unknown segment type */
				err = kFskErrBadData;
				FskMemPtrDispose((FskMemPtr)(*pDistances));
				*pDistances = NULL;
				*pNumDists  = numDists;
				goto bail;
		}
	}
	*distances = FskFractToFloat(tab->toHere);	/* This should always be 1.0 */

bail:
	return err;
}


/********************************************************************************
 * EvaluatePathSegment
 ********************************************************************************/

static FskErr
EvaluatePathSegment(FskConstPath path, const FskArcLengthTableRecord *tab, FskFract f, FskFixedMatrix3x2 *M)
{
	FskErr					err			= kFskErrNone;
	FskFixed				*fp;
	int						i;
	const TaggedSegment		*seg;
	FskFixedPoint2D			pt[5];

	BAIL_IF_FALSE(((f >= 0) && (f <= FRACT_ONE)), err, kFskErrOutOfSequence);											/* Verify 0 <= f <= 1 */

	seg = (const TaggedSegment*)((const char*)GetConstPathSegments(path) + tab->segOffset);								/* Get segment pointer */

	if (seg->type == kFskPathSegmentLineTo) {																			/* Line is easiest */
		M->M[0][0] = ((TaggedSegmentLineTo*)seg)->data.p.x - PreviousPointToSegment(seg).x;								/* Compute line vector */
		M->M[0][1] = ((TaggedSegmentLineTo*)seg)->data.p.y - PreviousPointToSegment(seg).y;
		M->M[2][0] = FskFracMul(f, M->M[0][0]) + PreviousPointToSegment(seg).x;											/* Evaluate point */
		M->M[2][1] = FskFracMul(f, M->M[0][1]) + PreviousPointToSegment(seg).y;
	}
	else {																												/* A curved segment */
		/* Approximate fraction of this arc's parametric value */
		for (i = 0; i <= kLastSubSeg; i++)																				/* Find the subsegment */
			if (f < tab->subSeg[i])
				break;
		if (i == 0) {																									/* First subsegment */
			f = FskFixedNDiv(f, tab->subSeg[0], 30-kLogNumArcEvals);
		}
		else if (i > kLastSubSeg) {																						/* Last subsegment */
			f	= FskFixedNDiv(f - tab->subSeg[kLastSubSeg], FRACT_ONE - tab->subSeg[kLastSubSeg], 30-kLogNumArcEvals)
				+ (FRACT_ONE - (FRACT_ONE >> kLogNumArcEvals));
		}
		else {																											/* One of the middle subsegments */
			f	= FskFixedNDiv(f - tab->subSeg[i-1], tab->subSeg[i] - tab->subSeg[i-1], 30-kLogNumArcEvals)
				+ (FRACT_ONE >> kLogNumArcEvals) * i;
		}

		fp = &pt[0].x;
		*fp++ = PreviousPointToSegment(seg).x;								*fp++ = PreviousPointToSegment(seg).y;										/* 0, common */
		switch (seg->type) {
			case kFskPathSegmentQuadraticBezierTo:
				*fp++ = ((TaggedSegmentQuadraticBezierTo*)seg)->data.p1.x;	*fp++ = ((TaggedSegmentQuadraticBezierTo*)seg)->data.p1.y;					/* 1 */
				*fp++ = ((TaggedSegmentQuadraticBezierTo*)seg)->data.p.x;	*fp   = ((TaggedSegmentQuadraticBezierTo*)seg)->data.p.y;					/* 2 */
				FskFixedEvaluateBezierVector(&pt[0].x, 3, 2, f, M->M[2], M->M[0]);
				break;
			case kFskPathSegmentCubicBezierTo:
				*fp++ = ((TaggedSegmentCubicBezierTo*)seg)->data.p1.x;		*fp++ = ((TaggedSegmentCubicBezierTo*)seg)->data.p1.y;						/* 1 */
				*fp++ = ((TaggedSegmentCubicBezierTo*)seg)->data.p2.x;		*fp++ = ((TaggedSegmentCubicBezierTo*)seg)->data.p2.y;						/* 2 */
				*fp++ = ((TaggedSegmentCubicBezierTo*)seg)->data.p.x;		*fp   = ((TaggedSegmentCubicBezierTo*)seg)->data.p.y;						/* 3 */
				FskFixedEvaluateBezierVector(&pt[0].x, 4, 2, f, M->M[2], M->M[0]);
				break;
			case kFskPathSegmentRationalQuadraticBezierTo:
			{	FskFixed V[2][3];
			#define AVOID_TANGENT_OVERFLOW
			#ifndef AVOID_TANGENT_OVERFLOW
				*fp++ = FRACT_ONE;																														/* 0 weight */
				*fp++ = FskFracMul(((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.p1.x, ((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.w1);	/* 1 */
				*fp++ = FskFracMul(((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.p1.y, ((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.w1);
				*fp++ = ((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.w1;
				*fp++ = ((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.p.x;																		/* 2 */
				*fp++ = ((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.p.y;
				*fp   = FRACT_ONE;
				FskFixedEvaluateBezierVector(&pt[0].x, 3, 3, f, V[1], V[0]);					/* Evaluate homogeneous Bezier */
				M->M[2][0] = FskFracDiv(V[1][0], V[1][2]);										/* x / w */
				M->M[2][1] = FskFracDiv(V[1][1], V[1][2]);										/* y / w */
				M->M[0][0] = FskFracDiv(V[0][0] - FskFracMul(M->M[2][0], V[0][2]), V[1][2]);	/* (x' - x * w') / w */
				M->M[0][1] = FskFracDiv(V[0][1] - FskFracMul(M->M[2][1], V[0][2]), V[1][2]);	/* (y' - y * w') / w */
			#else /* AVOID_TANGENT_OVERFLOW */
				#define DCJOVFBITS	2															/* The number of bits to toss out in w to avoid overflow in the tangent */
				*fp++ = FRACT_ONE >> DCJOVFBITS;																										/* 0 weight */
				*fp++ = FskFracMul(((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.p1.x, ((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.w1);	/* 1 */
				*fp++ = FskFracMul(((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.p1.y, ((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.w1);
				*fp++ = ((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.w1 >> DCJOVFBITS;
				*fp++ = ((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.p.x;																		/* 2 */
				*fp++ = ((TaggedSegmentRationalQuadraticBezierTo*)seg)->data.p.y;
				*fp   = FRACT_ONE >> DCJOVFBITS;
				FskFixedEvaluateBezierVector(&pt[0].x, 3, 3, f, V[1], V[0]);													/* Evaluate homogeneous Bezier */
				M->M[2][0] = FskFixedNDiv(V[1][0], V[1][2], 30-DCJOVFBITS);														/* x / w */
				M->M[2][1] = FskFixedNDiv(V[1][1], V[1][2], 30-DCJOVFBITS);														/* y / w */
				M->M[0][0] = FskFixedNDiv(V[0][0] - FskFixedNMul(M->M[2][0], V[0][2], 30-DCJOVFBITS), V[1][2], 30-DCJOVFBITS);	/* (x' - x * w') / w */
				M->M[0][1] = FskFixedNDiv(V[0][1] - FskFixedNMul(M->M[2][1], V[0][2], 30-DCJOVFBITS), V[1][2], 30-DCJOVFBITS);	/* (y' - y * w') / w */
			#endif /* AVOID_TANGENT_OVERFLOW */
			}	break;
			default:																			/* We should never get here */
				err = kFskErrUnknownElement;
				goto bail;
		}
	}

	FskFixedVector2DNormalize(M->M[0]);															/* Compute unit tangent vector by normalizing */
	M->M[1][0] = -M->M[0][1];		M->M[1][1] =  M->M[0][0];									/* Normal = tangent rotated by 90 degrees */
//	M->M[1][0] =  M->M[0][1];		M->M[1][1] = -M->M[0][0];									/* Normal = tangent rotated by 90 degrees (the other 90) */

bail:
	return err;
}


/********************************************************************************
 * ZeroMatrix3x2
 ********************************************************************************/

static void
ZeroMatrix3x2(FskFixedMatrix3x2 *M)
{
	int i;
	FskFixed *f;

	for (i = sizeof(FskFixedMatrix3x2) / sizeof(FskFixed), f = M->M[0]; i--; )
		*f++ = 0;
}


/********************************************************************************
 * CompareArcTab
 ********************************************************************************/

static int
CompareArcTab(const void *vKey, const void *vTab)
{
	FskFract				s	= *((const FskFract*)vKey);
	FskConstArcLengthTable	t	= (FskConstArcLengthTable)vTab;

	if (s < t[0].toHere)				return -1;	/* In an earlier segment */
	if (s > t[1].toHere)				return 1;	/* In a latter segment */
	if (t[0].subSeg[kLastSubSeg] == 0)	return -1;	/* Zero length segment */
	return 0;										/* A real, non-null segment */
}


/********************************************************************************
 * FskPathEvaluateSegment
 ********************************************************************************/

FskErr
FskPathEvaluateSegment(FskConstPath path, FskConstArcLengthTable arcTab, UInt32 segIndex, FskFract f, FskFixedMatrix3x2 *M)
{
	FskErr	err;
	UInt32	numSegs;

	BAIL_IF_ZERO((numSegs = GetPathSegmentCount(path)), err, kFskErrInvalidParameter);			/* Get segment count */
	BAIL_IF_FALSE((segIndex < numSegs), err, kFskErrInvalidParameter);							/* Verify valid segment number */
	BAIL_IF_NULL(arcTab, err, kFskErrInvalidParameter);											/* Verify that an arc length table exists */
	err = EvaluatePathSegment(path, arcTab + segIndex, f, M);

bail:
	if (err != kFskErrNone)	ZeroMatrix3x2(M);
	return err;
}


/********************************************************************************
 * FskPathEvaluate
 ********************************************************************************/

FskErr
FskPathEvaluate(FskConstPath path, FskConstArcLengthTable arcTab, FskFract s, FskFixedMatrix3x2 *M)
{
	FskErr					err;
	UInt32					numSegs;
	FskConstArcLengthTable	tab;
	FskFract				f;

	BAIL_IF_ZERO((numSegs = GetPathSegmentCount(path)), err, kFskErrInvalidParameter);						/* Get segment count */
	BAIL_IF_FALSE(((s >= 0) && (s <= FRACT_ONE)), err, kFskErrOutOfSequence);								/* Verify 0 <= s <= 1 */
	BAIL_IF_NULL(arcTab, err, kFskErrInvalidParameter);														/* Verify that an arc length table exists */
	if ((tab = (FskConstArcLengthTable)FskBSearch(&s, arcTab, numSegs, sizeof(FskArcLengthTableRecord), CompareArcTab)) == NULL)
		tab = arcTab + numSegs - 1;
	f = FskFracDiv((s - tab[0].toHere), (tab[1].toHere - tab[0].toHere));									/* Fraction of this segment's normalized arc length */
	err = EvaluatePathSegment(path, tab, f, M);

bail:
	if (err != kFskErrNone)	ZeroMatrix3x2(M);
	return err;
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Tweening								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskPathCompatibleForTweening
 ********************************************************************************/

Boolean
FskPathCompatibleForTweening(FskConstPath path0, FskConstPath path1)
{
	UInt32				numSegs;
	const TaggedSegment	*seg0Ptr, *seg1Ptr;

	if (path0 == NULL)	goto bad;
	if (path1 == NULL)	goto bad;
	if ((numSegs = GetPathSegmentCount(path0)) != GetPathSegmentCount(path1))	goto bad;

	seg0Ptr	= GetConstPathSegments(path0);
	seg1Ptr	= GetConstPathSegments(path1);
	for ( ; numSegs--; seg0Ptr = NextConstTaggedPathSegment(seg0Ptr), seg1Ptr = NextConstTaggedPathSegment(seg1Ptr))
		if (seg0Ptr->type != seg1Ptr->type)
			goto bad;

	return 1;

bad:
	return 0;
}


/********************************************************************************
 * FskPathTween
 ********************************************************************************/

FskErr
FskPathTween(FskConstPath path0, FskConstPath path1, double fraction, FskPath pathT)
{
	FskErr				err			= kFskErrNone;
	FskFract			t			= FskRoundAndSaturateFloatToUnityFract(fraction);
	UInt32				numSegs;
	const TaggedSegment	*seg1Ptr;
	TaggedSegment		*segTPtr;

	/* Make sure the paths are compatible */
	BAIL_IF_FALSE(((pathT != NULL) && FskPathCompatibleForTweening(path0, path1)), err, kFskErrInvalidParameter);

	/* Take care of the beginning and end trivially */
	if (t == FRACT_ONE) {	MyMoveData(path1, pathT, FskPathSize(path1));	goto bail;	}	/* Copy path1 for t == 1, and return */
	MyMoveData(path0, pathT, FskPathSize(path0));											/* Copy path0 for everything else */
	if (t == 0)	goto bail;																	/* Done if t == 0 */

	/* Tween the parameters of each segment */
	numSegs = GetPathSegmentCount(pathT);
	seg1Ptr	= GetConstPathSegments(path1);
	segTPtr	= GetPathSegments(pathT);
	for ( ; numSegs--; segTPtr = NextTaggedPathSegment(segTPtr), seg1Ptr = NextConstTaggedPathSegment(seg1Ptr)) {
		switch (segTPtr->type) {
			case kFskPathSegmentMoveTo:
			{	TaggedSegmentMoveTo								*moveToT = (TaggedSegmentMoveTo*)(segTPtr);
				const TaggedSegmentMoveTo						*moveTo1 = (const TaggedSegmentMoveTo*)(seg1Ptr);
																FskFixedInterpolate(t, 1*2, &moveToT->data.p.x, &moveTo1->data.p.x, &moveToT->data.p.x);
			}													break;
			case kFskPathSegmentLineTo:
			{	TaggedSegmentLineTo								*lineToT = (TaggedSegmentLineTo*)(segTPtr);
				const TaggedSegmentLineTo						*lineTo1 = (const TaggedSegmentLineTo*)(seg1Ptr);
																FskFixedInterpolate(t, 1*2, &lineToT->data.p.x, &lineTo1->data.p.x, &lineToT->data.p.x);
			}													break;
			case kFskPathSegmentQuadraticBezierTo:
			{	TaggedSegmentQuadraticBezierTo					*quadToT = (TaggedSegmentQuadraticBezierTo*)(seg1Ptr);
				const TaggedSegmentQuadraticBezierTo			*quadTo1 = (const TaggedSegmentQuadraticBezierTo*)(segTPtr);
																FskFixedInterpolate(t, 2*2, &quadToT->data.p1.x, &quadTo1->data.p1.x, &quadToT->data.p1.x);
			}													break;
			case kFskPathSegmentCubicBezierTo:
			{	TaggedSegmentCubicBezierTo						*cubcToT = (TaggedSegmentCubicBezierTo*)(segTPtr);
				const TaggedSegmentCubicBezierTo				*cubcTo1 = (const TaggedSegmentCubicBezierTo*)(seg1Ptr);
																FskFixedInterpolate(t, 3*2, &cubcToT->data.p1.x, &cubcTo1->data.p1.x, &cubcToT->data.p1.x);
			}													break;
			case kFskPathSegmentRationalQuadraticBezierTo:
			{	TaggedSegmentRationalQuadraticBezierTo			*concToT = (TaggedSegmentRationalQuadraticBezierTo*)(segTPtr);
				const TaggedSegmentRationalQuadraticBezierTo	*concTo1 = (const TaggedSegmentRationalQuadraticBezierTo*)(seg1Ptr);
																FskFixedInterpolate(t, 1+2*2, &concToT->data.w1, &concTo1->data.w1, &concToT->data.w1);
			}													break;
			default:											break;
		}
	}


bail:
	return err;
}


/********************************************************************************
 * FskPathNewTween
 ********************************************************************************/

FskErr
FskPathNewTween(FskConstPath path0, FskConstPath path1, double fraction, FskPath *pathT)
{
	FskErr		err;

	if ((err = FskMemPtrNew(FskPathSize(path0), (FskMemPtr*)pathT)) == kFskErrNone)
		err = FskPathTween(path0, path1, fraction, *pathT);

	return err;
}


/********************************************************************************
 * FskPathGetNextSegment
 ********************************************************************************/

void FskPathGetNextSegment(FskConstPath path, UInt32 *segType, const void **segData) {
	const TaggedSegment	*segPtr;
	if (*segData == NULL) {
		segPtr	= GetConstPathSegments(path);
	}
	else {
		segPtr = ((const TaggedSegment*)(*segData)) - 1;
		segPtr = NextConstTaggedPathSegment(segPtr);
	}
	*segType = segPtr->type;
	*segData = (const void*)(&segPtr->type + 1);
}


/********************************************************************************
 * FskPathString
 ********************************************************************************/

FskErr FskPathString(FskConstPath path, int precision, FskGrowableStorage str) {
	const float	kFixedToFloat	= 1.f / 65536.f;
	const float	kFractToFloat	= 1.f / 1073741824.;
	char		lastCmd			= 0;
	FskErr		err				= kFskErrNone;
	UInt32		numSegs, segType;
	const void	*segData;

	BAIL_IF_NULL(path, err, kFskErrInvalidParameter);
	BAIL_IF_NULL(str,  err, kFskErrInvalidParameter);

	if      (precision <= 0)	precision = 6;	/* Default precision is 6 digits */
	else if (precision > 10)	precision = 10;	/* Max fixed point precision is 10 digits */

	for (numSegs = FskPathGetSegmentCount(path), segData = NULL; numSegs--; ) {
		FskPathGetNextSegment(path, &segType, &segData);
		switch ((FskPathElementType)segType) {
			case kFskPathSegmentNull:
				break;
			case kFskPathSegmentClose:
				BAIL_IF_ERR(err = FskGrowableStorageAppendF(str, "Z"));
				lastCmd = 0;
				break;
			case kFskPathSegmentMoveTo:
			{	const FskPathSegmentMoveTo *moveTo = (const FskPathSegmentMoveTo*)(segData);
				BAIL_IF_ERR(err = FskGrowableStorageAppendF(str, "M%.*g,%.*g",
					precision, moveTo->p.x * kFixedToFloat, precision, moveTo->p.y * kFixedToFloat));
				lastCmd = 0;
			}	break;
			case kFskPathSegmentLineTo:
			{	const FskPathSegmentLineTo *lineTo = (const FskPathSegmentLineTo*)(segData);
				(void)FskGrowableStorageAppendF(str, "%c%.*g,%.*g",
					(('L' == lastCmd) ? ' ' : 'L'),
					precision, lineTo->p.x * kFixedToFloat, precision, lineTo->p.y * kFixedToFloat);
				lastCmd = 'L';
			}	break;
			case kFskPathSegmentQuadraticBezierTo:
			{	const FskPathSegmentQuadraticBezierTo *quadTo = (const FskPathSegmentQuadraticBezierTo*)(segData);
				BAIL_IF_ERR(err = FskGrowableStorageAppendF(str, "%c%.*g,%.*g,%.*g,%.*g",
					(('Q' == lastCmd) ? ' ' : 'Q'),
					precision, quadTo->p1.x * kFixedToFloat, precision, quadTo->p1.y * kFixedToFloat,
					precision, quadTo->p.x  * kFixedToFloat, precision, quadTo->p.y  * kFixedToFloat));
				lastCmd = 'Q';
			}	break;
			case kFskPathSegmentCubicBezierTo:
			{	const FskPathSegmentCubicBezierTo *cubicTo = (const FskPathSegmentCubicBezierTo*)(segData);
				BAIL_IF_ERR(err = FskGrowableStorageAppendF(str, "%c%.*g,%.*g,%.*g,%.*g,%.*g,%.*g",
					(('C' == lastCmd) ? ' ' : 'C'),
					precision, cubicTo->p1.x * kFixedToFloat, precision, cubicTo->p1.y * kFixedToFloat,
					precision, cubicTo->p2.x * kFixedToFloat, precision, cubicTo->p2.y * kFixedToFloat,
					precision, cubicTo->p.x  * kFixedToFloat, precision, cubicTo->p.y  * kFixedToFloat));
				lastCmd = 'C';
			}	break;

			case kFskPathSegmentRationalQuadraticBezierTo:
			{	const FskPathSegmentRationalQuadraticBezierTo *conicTo = (const FskPathSegmentRationalQuadraticBezierTo*)(segData);
				BAIL_IF_ERR(err = FskGrowableStorageAppendF(str, "%c%.*g,%.*g,%.*g,%.*g,%.*g", 			/* Nonstandard -- our proprietary extension */
					(('K' == lastCmd) ? ' ' : 'K'),
					precision, conicTo->p1.x * kFixedToFloat, precision, conicTo->p1.y * kFixedToFloat, precision, conicTo->w1 * kFractToFloat,
					precision, conicTo->p.x  * kFixedToFloat, precision, conicTo->p.y  * kFixedToFloat));
				lastCmd = 'K';
			}	break;
			case kFskPathSegmentEndGlyph:
				BAIL_IF_ERR(err = FskGrowableStorageAppendF(str, "G"));									/* Nonstandard -- our proprietary extension */
				lastCmd = 0;
				break;
		}
	}

bail:
	return err;
}
