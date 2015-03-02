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

#if __FSK_LAYER__
	#include "Fsk.h"
	#include "FskMatrix.h"
	#define KPSProjectUsingPanTiltRoll3x3(w, h, p, t, r, f, c, a, m) FskSProjectUsingPanTiltRoll3x3(w, h, p, t, r, f, c, a, m)
#else /* !__FSK_LAYER__ */
	#include "KPMatrix.h"
#endif /* !__FSK_LAYER__ */

#include "KPCubicPanoController.h"

#if !__FSK_LAYER__
	#include <QuickTimeVRFormat.h>
#else /* FSK_LAYER */

	struct QTVRCubicViewAtom {
		float		minPan;
		float		maxPan;
		float		minTilt;
		float		maxTilt;
		float		minFieldOfView;
		float		maxFieldOfView;

		float		defaultPan;
		float		defaultTilt;
		float		defaultFieldOfView;
	};
	typedef struct QTVRCubicViewAtom	QTVRCubicViewAtom;
	typedef QTVRCubicViewAtom			*QTVRCubicViewAtomPtr;

	enum {
		kQTVRObjectAnimateViewFramesOn		= (1L << 0),
		kQTVRObjectPalindromeViewFramesOn	= (1L << 1),
		kQTVRObjectStartFirstViewFrameOn	= (1L << 2),
		kQTVRObjectAnimateViewsOn			= (1L << 3),
		kQTVRObjectPalindromeViewsOn		= (1L << 4),
		kQTVRObjectSyncViewToFrameRate		= (1L << 5),
		kQTVRObjectDontLoopViewFramesOn		= (1L << 6),
		kQTVRObjectPlayEveryViewFrameOn		= (1L << 7),
		kQTVRObjectStreamingViewsOn			= (1L << 8)
	};

	enum {
		kQTVRObjectWrapPanOn				= (1L << 0),
		kQTVRObjectWrapTiltOn				= (1L << 1),
		kQTVRObjectCanZoomOn				= (1L << 2),
		kQTVRObjectReverseHControlOn		= (1L << 3),
		kQTVRObjectReverseVControlOn		= (1L << 4),
		kQTVRObjectSwapHVControlOn			= (1L << 5),
		kQTVRObjectTranslationOn			= (1L << 6)
	};

#endif /* FSK_LAYER */

#include <stdlib.h>
#include <math.h>


#if __FSK_LAYER__
#include "FskMemory.h"
#define ALLOC	FskMemPtrAlloc
#define FREE	FskMemPtrDispose
#else
#define ALLOC	malloc
#define FREE	free
#endif

#if !defined(USE_SINGLE_PRECISION_TRANSCENDENTALS)
	#if defined(_WIN32)
		#define USE_SINGLE_PRECISION_TRANSCENDENTALS 0
	#else /* !_WIN32 */
		#define USE_SINGLE_PRECISION_TRANSCENDENTALS 1
	#endif /* _WIN32 */
#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */


#define D_2PI					6.2831853071795864769			/* pi * 2 */
#define D_PI					3.1415926535897932385			/* pi */
#define D_PI_2					1.5707963267948966192			/* pi / 2 */
#define D_PI_4					0.78539816339744830962			/* pi / 4 */
#define D_2_PI					0.63661977236758134308			/* 2 / pi */
#define D_RADIANS_PER_DEGREE	0.017453292519943295769			/* pi / 180 */

#define F_2PI					((float)D_2PI)					/* pi * 2 */
#define F_PI					((float)D_PI)					/* pi */
#define F_PI_2					((float)D_PI_2)					/* pi / 2 */
#define F_PI_4					((float)D_PI_4)					/* pi / 4 */
#define F_2_PI					((float)D_2_PI)					/* 2 / pi */
#define F_RADIANS_PER_DEGREE	((float)D_RADIANS_PER_DEGREE)	/* pi / 180 */


//#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh) || defined(__MWERKS__)
		#define PRAGMA_MARK_SUPPORTED 1
	#else /* !PRAGMA_MARK_SUPPORTED */
		#define PRAGMA_MARK_SUPPORTED 0
	#endif /* !PRAGMA_MARK_SUPPORTED */
//#endif /* PRAGMA_MARK_SUPPORTED */


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ******							KPPanoController							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


 struct KPPanoController {
 	/* Media limits */
 	float	minPan;
 	float	maxPan;
 	float	minTilt;
 	float	maxTilt;
 	float	minFov;
 	float	maxFov;
 	char	wraps;
 	char	unused[3];

 	/* Controller state */
 	float	pan;
 	float	tilt;
 	float	roll;
 	float	vfov;
 	float	focalLength;
 	long	windowWidth;
 	long	windowHeight;
 	long	portWidth;
 	long	portHeight;
 	float	winXfm[2][2];

	/* Initial state */
 	float	pan0;
 	float	tilt0;
 	float	roll0;
 	float	vfov0;

	/* UI Control parameters */
 	float	panSpeed;
 	float	zoomSpeed;

 	/* Limits derived from state and media limits */
 	float	minPegPan;
 	float	maxPegPan;
 	float	minPegTilt;
 	float	maxPegTilt;
 };


/********************************************************************************
 * UpdateFocalLengthFromFov
 ********************************************************************************/

static void
UpdateFocalLengthFromFov(KPPanoController *ctlr)
{
	#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
		ctlr->focalLength = (ctlr->portHeight - 1) * 0.5f / (float)tan(ctlr->vfov * 0.5f);
	#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		ctlr->focalLength = (ctlr->portHeight - 1) * 0.5f / tanf(ctlr->vfov * 0.5f);
	#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
}


/********************************************************************************
 * UpdateFovFromFocalLength
 ********************************************************************************/

static void
UpdateFovFromFocalLength(KPPanoController *ctlr)
{
	#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
		ctlr->vfov = (float)atan((ctlr->portHeight - 1) * 0.5f / ctlr->focalLength) * 2.0f;
	#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		ctlr->vfov = atanf((ctlr->portHeight - 1) * 0.5f / ctlr->focalLength) * 2.0f;
	#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
}


/********************************************************************************
 * UpdatePegLimits
 ********************************************************************************/

static void
UpdatePegLimits(KPPanoController *ctlr)
{
	/* We have special treatment for tilt limits at +/- 90 degrees */
	ctlr->minPegTilt = (ctlr->minTilt == -F_PI_2) ? (-F_PI_2) : (ctlr->minTilt + ctlr->vfov * 0.5f);
	ctlr->maxPegTilt = (ctlr->maxTilt ==  F_PI_2) ? ( F_PI_2) : (ctlr->maxTilt - ctlr->vfov * 0.5f);

	ctlr->minPegPan = ctlr->minPan;
	ctlr->maxPegPan = ctlr->maxPan;
	if (!ctlr->wraps) {
		float dPan;
		#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
			dPan = (float)atan((ctlr->portWidth - 1) / (2.0f * ctlr->focalLength));
		#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
			dPan = atanf((ctlr->portWidth - 1) / (2.0f * ctlr->focalLength));
		#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		ctlr->minPegPan += dPan;
		ctlr->maxPegPan -= dPan;
	}
}


/********************************************************************************
 * WrapInRange
 ********************************************************************************/

static float
WrapInRange(float x, float xMin, float xMax)
{
	float dx;

	dx = xMax - xMin;
	x -= xMin;
	#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
		x = (float)fmod(x, dx);
	#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		x = fmodf(x, dx);
	#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
	if (x < 0)
		x += dx;
	x += xMin;

	return x;
}


#if 0
/********************************************************************************
 * ExtremePixelsToDifferentialAngle
 *	Law of Cosines + Law of Sines
 ********************************************************************************/

static float
ExtremePixelsToDifferentialAngle(float radius, float angle, float numPixels)
{
	double a, b, c, beta, gamma;

	a     = radius / cos(angle);
	b     = numPixels;
	gamma = D_PI_2 - angle;
	c     = sqrt(a * a + b * b - 2.0 * a * b * cos(gamma));	/* Law of cosines */
	beta  = asin(sin(gamma) * b / c);						/* Law of sines */

	return (float)beta;
}
#endif


/********************************************************************************
 * ConstrainMaxFOV
 ********************************************************************************/

static void
ConstrainMaxFOV(KPPanoController *ctlr)
{
	float f;

	/* Make sure that the max FOV is no greater than the spread in tilt */
	f = ctlr->maxTilt - ctlr->minTilt;
	//f -= ExtremePixelsToDifferentialAngle(sphereRadius, (fabs(ctlr->maxTilt) < fabs(ctlr->minTilt) ? fabs(ctlr->maxTilt) : fabs(ctlr->minTilt)), 1.0f);	/* subtract a pixel's worth of angle for bilinear interpolation */
	if (ctlr->maxFov > f)
		ctlr->maxFov = f;
}


/********************************************************************************
 * ApplyPanoConstraints
 ********************************************************************************/

static KPPanoControllerStatus
ApplyPanoConstraints(KPPanoController *ctlr, long fovChanged)
{
	KPPanoControllerStatus status = kpPanoNoConstraint;

	/* Apply FOV constraints */
	if (fovChanged) {	/* Make sure FOV obeys the constraints */
		if (!(ctlr->vfov <= ctlr->maxFov)) {
			ctlr->vfov = ctlr->maxFov;
			status |= kpPanoMaxFOV;
		}
		if (!(ctlr->vfov >= ctlr->minFov)) {
			ctlr->vfov = ctlr->minFov;
			status |= kpPanoMinFOV;
		}
		if (status & (kpPanoMinFOV | kpPanoMaxFOV))
			UpdateFocalLengthFromFov(ctlr);
		#if 0	/* This should never happen because we constrain the max FOV beforehand, right? */
			if ((status & (kpPanoMinFOV | kpPanoMaxFOV)) == (kpPanoMinFOV | kpPanoMaxFOV))	/* Arghhh! FOV is bigger than delta tilt! */
				ConstrainMaxFOV(ctlr);
		#endif /* 0 */
		UpdatePegLimits(ctlr);
	}

	/* Apply tilt constraints */
	if (!(ctlr->tilt >= ctlr->minPegTilt)) {
		ctlr->tilt = ctlr->minPegTilt;
		status |= kpPanoMinTilt;
	}
	if (!(ctlr->tilt <= ctlr->maxPegTilt)) {
		ctlr->tilt = ctlr->maxPegTilt;
		status |= kpPanoMaxTilt;
	}

	/* Apply pan constraints */
	if (!ctlr->wraps) {
		if (!(ctlr->pan >= ctlr->minPegPan)) {
			ctlr->pan = ctlr->minPegPan;
			status |= kpPanoMinPan;
		}
		if (!(ctlr->pan <= ctlr->maxPegPan)) {
			ctlr->pan = ctlr->maxPegPan;
			status |= kpPanoMaxPan;
		}
	}
	else {	/* No constaints on pan: get pan into canonical space */
		if (!((ctlr->pan >= ctlr->minPegPan) && (ctlr->pan <= ctlr->maxPegPan)))
			ctlr->pan = WrapInRange(ctlr->pan, ctlr->minPegPan, ctlr->maxPegPan);
	}

	return status;
}


/********************************************************************************
 * DiffIs360Degrees
 ********************************************************************************/

static int
DiffIs360Degrees(float min, float max)
{
	int wraps;

	#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
		wraps = (int)(fabs(max - min - F_2PI) < 1e-5f);
	#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		wraps = (int)(fabsf(max - min - F_2PI) < 1e-5f);
	#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */

	return wraps;
}



#if TARGET_RT_LITTLE_ENDIAN
/********************************************************************************
 * FlipEndianF32
 *	Convert between big and little-endian single-precision floating point.
 ********************************************************************************/

static void
FlipEndianF32(float *f)
{
	union fcu { float f; char c[4]; } *fc;
	char c;

	fc = (union fcu*)f;
	c = fc->c[0];	fc->c[0] = fc->c[3];	fc->c[3] = c;
	c = fc->c[1];	fc->c[1] = fc->c[2];	fc->c[2] = c;
}
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * ConvertArrayFromDegreesToRadians
 ********************************************************************************/

static void
ConvertArrayFromDegreesToRadians(register float *a, register long n)
{
	for ( ; n--; a++)
		*a *= (float)(F_RADIANS_PER_DEGREE);
}


/********************************************************************************
 * KPNewPanoController
 ********************************************************************************/

KPPanoController*
KPNewPanoController(void)
{
	KPPanoController *ctlr;

	if ((ctlr = (KPPanoController*)ALLOC(sizeof(KPPanoController))) != NULL) {
		/* Set default media limits */
		ctlr->minPan		=  0.0f;		/* Pan from 0 degrees ... */
		ctlr->maxPan		=  F_2PI;		/* ... to 360 degrees */
		ctlr->wraps			=  1;			/* Wrap around in pan */
		ctlr->minTilt		= -F_PI_2;		/* Tilt from -90 degrees ... */
		ctlr->maxTilt		=  F_PI_2;		/* ... to +90 degrees */
		ctlr->minFov		=  0.01f;		/* About 1 degree */
		ctlr->maxFov		=  3.0f;		/* Almost 180 degrees */
		ctlr->unused[0]		=  0;
		ctlr->unused[1]		=  0;
		ctlr->unused[2]		=  0;

		/* Set default controller state */
		ctlr->pan			=  0.0f;		/* Right on the seam */
		ctlr->tilt			=  0.0f;		/* Perfectly level attitude */
		ctlr->roll			=  0.0f;		/* No rocking */
		ctlr->vfov			=  1.0f;		/* About 60 degrees */
		ctlr->windowWidth	=  384;			/* Arbitrary width */
		ctlr->windowHeight	=  256;			/* Arbitrary height */
		ctlr->portWidth		=  ctlr->windowWidth;
		ctlr->portHeight	=  ctlr->windowHeight;
		ctlr->winXfm[0][0]	=  1.0f;	ctlr->winXfm[0][1]	=  0.0f;	/* Identity */
		ctlr->winXfm[1][0]	=  0.0f;	ctlr->winXfm[1][1]	=  1.0f;

		/* Set default UI parameters */
		ctlr->panSpeed		=  2.0f;
		ctlr->zoomSpeed		=  2.0f;	//1.4142135624f;

		/* Ripple state */
		UpdateFocalLengthFromFov(ctlr);
		UpdatePegLimits(ctlr);
	}

	return ctlr;
}


/********************************************************************************
 * KPDeletePanoController
 ********************************************************************************/

void
KPDeletePanoController(KPPanoController *ctlr)
{
	FREE(ctlr);
}


/********************************************************************************
 * KPQTVRCubicViewAtomInitPanoController
 *		Initialization from QTVR cubic view atom
 ********************************************************************************/

void
KPQTVRCubicViewAtomInitPanoController(KPPanoController *ctlr, const struct QTVRCubicViewAtom *cuvw)
{
	ctlr->minPan	= cuvw->minPan;
	ctlr->maxPan	= cuvw->maxPan;
	ctlr->minTilt	= cuvw->minTilt;
	ctlr->maxTilt	= cuvw->maxTilt;
	ctlr->minFov	= cuvw->minFieldOfView;
	ctlr->maxFov	= cuvw->maxFieldOfView;
	ctlr->pan		= cuvw->defaultPan;
	ctlr->tilt		= cuvw->defaultTilt;
	ctlr->vfov		= cuvw->defaultFieldOfView;

	#if TARGET_RT_LITTLE_ENDIAN
		FlipEndianF32(&ctlr->minPan);
		FlipEndianF32(&ctlr->maxPan);
		FlipEndianF32(&ctlr->minTilt);
		FlipEndianF32(&ctlr->maxTilt);
		FlipEndianF32(&ctlr->minFov);
		FlipEndianF32(&ctlr->maxFov);
		FlipEndianF32(&ctlr->pan);
		FlipEndianF32(&ctlr->tilt);
		FlipEndianF32(&ctlr->vfov);
	#endif /* TARGET_RT_LITTLE_ENDIAN */

	ConvertArrayFromDegreesToRadians(&ctlr->minPan, 6);
	ConvertArrayFromDegreesToRadians(&ctlr->pan,    4);

	/* Set reset defaults */
	ctlr->pan0		= ctlr->pan;
	ctlr->tilt0		= ctlr->tilt;
	ctlr->roll0		= ctlr->roll;
	ctlr->vfov0		= ctlr->vfov;

	ctlr->wraps = DiffIs360Degrees(ctlr->minPan, ctlr->maxPan);

	UpdateFocalLengthFromFov(ctlr);

	ConstrainMaxFOV(ctlr);			/* Make sure that the maximum FOV is not greater than that allowed by the tilt limits */
	ApplyPanoConstraints(ctlr, 1);	/* This calls UpdatePegLimits() */
}


/********************************************************************************
 * KPResetPanoController
 ********************************************************************************/

void
KPResetPanoController(KPPanoController *ctlr)
{
	ctlr->pan  = ctlr->pan0;
	ctlr->tilt = ctlr->tilt0;
//	ctlr->roll = ctlr->roll0;
	ctlr->vfov = ctlr->vfov0;
	UpdateFocalLengthFromFov(ctlr);
	ApplyPanoConstraints(ctlr, 1);	/* This calls UpdatePegLimits() */
}


/********************************************************************************
 * KPGetViewMatrixOfPanoController
 ********************************************************************************/

void
KPGetViewMatrixOfPanoController(const KPPanoController *ctlr, float V[3][3])
{
	KPSProjectUsingPanTiltRoll3x3(ctlr->windowWidth, ctlr->windowHeight,
		ctlr->pan, ctlr->tilt, ctlr->roll, ctlr->focalLength, NULL, NULL, V[0]
	);
}


/********************************************************************************
 * KPSetViewConstraintsOfPanoController
 ********************************************************************************/

void
KPSetViewConstraintsOfPanoController(KPPanoController *ctlr, const float *minMaxPanTiltFOV)
{
	ctlr->minPan	= minMaxPanTiltFOV[0];
	ctlr->maxPan	= minMaxPanTiltFOV[1];
	ctlr->minTilt	= minMaxPanTiltFOV[2];
	ctlr->maxTilt	= minMaxPanTiltFOV[3];
	ctlr->minFov	= minMaxPanTiltFOV[4];
	ctlr->maxFov	= minMaxPanTiltFOV[5];

	ctlr->wraps = DiffIs360Degrees(ctlr->minPan, ctlr->maxPan);
	ConstrainMaxFOV(ctlr);
	ApplyPanoConstraints(ctlr, 1);	/* This calls UpdatePegLimits() */
}


/********************************************************************************
 * KPGetViewConstraintsOfPanoController
 ********************************************************************************/

void
KPGetViewConstraintsOfPanoController(const KPPanoController *ctlr, float *minMaxPanTiltFOV)
{
	minMaxPanTiltFOV[0] = ctlr->minPan;
	minMaxPanTiltFOV[1] = ctlr->maxPan;
	minMaxPanTiltFOV[2] = ctlr->minTilt;
	minMaxPanTiltFOV[3] = ctlr->maxTilt;
	minMaxPanTiltFOV[4] = ctlr->minFov;
	minMaxPanTiltFOV[5] = ctlr->maxFov;
}


/********************************************************************************
 * KPSetPanTiltFOVOfPanoController
 ********************************************************************************/

KPPanoControllerStatus
KPSetPanTiltFOVOfPanoController(KPPanoController *ctlr, const float *panTiltFOV)
{
	ctlr->pan  = panTiltFOV[0];
	ctlr->tilt = panTiltFOV[1];
	ctlr->vfov = panTiltFOV[2];
	UpdateFocalLengthFromFov(ctlr);
	return ApplyPanoConstraints(ctlr, 1);	/* This calls UpdatePegLimits() */
}


/********************************************************************************
 * KPGetPanTiltFOVOfPanoController
 ********************************************************************************/

void
KPGetPanTiltFOVOfPanoController(const KPPanoController *ctlr, float *panTiltFOV)
{
	panTiltFOV[0] = ctlr->pan;
	panTiltFOV[1] = ctlr->tilt;
	panTiltFOV[2] = ctlr->vfov;
}


/********************************************************************************
 * KPSetPanTiltOfPanoController
 ********************************************************************************/

KPPanoControllerStatus
KPSetPanTiltOfPanoController(KPPanoController *ctlr, const float *panTilt)
{
	ctlr->pan  = panTilt[0];
	ctlr->tilt = panTilt[1];
	return ApplyPanoConstraints(ctlr, 0);
}


/********************************************************************************
 * KPGetPanTiltOfPanoController
 ********************************************************************************/

void
KPGetPanTiltOfPanoController(const KPPanoController *ctlr, float *panTilt)
{
	panTilt[0] = ctlr->pan;
	panTilt[1] = ctlr->tilt;
}


/********************************************************************************
 * KPSetPanOfPanoController
 ********************************************************************************/

KPPanoControllerStatus
KPSetPanOfPanoController(KPPanoController *ctlr, float pan)
{
	ctlr->pan = pan;
	return ApplyPanoConstraints(ctlr, 0);
}


/********************************************************************************
 * KPGetPanOfPanoController
 ********************************************************************************/

float
KPGetPanOfPanoController(const KPPanoController *ctlr)
{
	return ctlr->pan;
}


/********************************************************************************
 * KPSetTiltOfPanoController
 ********************************************************************************/

KPPanoControllerStatus
KPSetTiltOfPanoController(KPPanoController *ctlr, float tilt)
{
	ctlr->tilt = tilt;
	return ApplyPanoConstraints(ctlr, 0);

}


/********************************************************************************
 * KPGetTiltOfPanoController
 ********************************************************************************/

float
KPGetTiltOfPanoController(const KPPanoController *ctlr)
{
	return ctlr->tilt;
}


/********************************************************************************
 * KPSetFOVOfPanoController
 ********************************************************************************/

KPPanoControllerStatus
KPSetFOVOfPanoController(KPPanoController *ctlr, float vfov)
{
	ctlr->vfov = vfov;
	UpdateFocalLengthFromFov(ctlr);
	return ApplyPanoConstraints(ctlr, 1);	/* This calls UpdatePegLimits() */
}


/********************************************************************************
 * KPGetFOVOfPanoController
 ********************************************************************************/

float
KPGetFOVOfPanoController(const KPPanoController *ctlr)
{
	return ctlr->vfov;
}


/********************************************************************************
 * KPSetFocalLengthOfPanoController
 ********************************************************************************/

KPPanoControllerStatus
KPSetFocalLengthOfPanoController(KPPanoController *ctlr, float focalLength)
{
	ctlr->focalLength = focalLength;
	UpdateFovFromFocalLength(ctlr);
	return ApplyPanoConstraints(ctlr, 1);	/* This calls UpdatePegLimits() */
}


/********************************************************************************
 * KPGetFocalLengthOfPanoController
 ********************************************************************************/

float
KPGetFocalLengthOfPanoController(const KPPanoController *ctlr)
{
	return ctlr->focalLength;
}


/********************************************************************************
 * KPSetOrientationOfPanoController
 ********************************************************************************/

KPPanoControllerStatus
KPSetOrientationOfPanoController(KPPanoController *ctlr, long orientation)
{
	orientation &= 3;																				/* Canonical enum */
	ctlr->roll = orientation * F_PI_2;																/* Convert to 0, pi/2, pi, pi*3/2 */

	if ((orientation & 1) == 0) {																	/* 0 or 180 degrees */
		ctlr->winXfm[0][1] = ctlr->winXfm[1][0] = 0;												/* Zero skew diagonal */
		ctlr->winXfm[0][0] = ctlr->winXfm[1][1] = (((orientation & 2) == 0) ?  1.0f : -1.0f);		/* Set diagonal */
		ctlr->portWidth  = ctlr->windowWidth;														/* Port is aligned with window */
		ctlr->portHeight = ctlr->windowHeight;
	}
	else {																							/* 90 or 270 degrees */
		ctlr->winXfm[0][0] = ctlr->winXfm[1][1] = 0;												/* Zero diagonal */
		ctlr->winXfm[1][0] = -(ctlr->winXfm[0][1] = (((orientation & 2) == 0) ? -1.0f :  1.0f));	/* Set skew diagonal for 90 & 270 degrees */
		ctlr->portWidth  = ctlr->windowHeight;														/* Port is rotated from window */
		ctlr->portHeight = ctlr->windowWidth;
	}

	UpdateFocalLengthFromFov(ctlr);
	return ApplyPanoConstraints(ctlr, 1);
}


/********************************************************************************
 * KPGetOrientationOfPanoController
 ********************************************************************************/

long
KPGetOrientationOfPanoController(const KPPanoController *ctlr)
{
	long	orientation;
	float	f;

	#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
		f = (float)fmod(ctlr->roll * D_2_PI, 4.0);
	#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		f = fmodf(ctlr->roll * F_2_PI, 4.0f);
	#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		if (f < 0)
			f += 4.0f;
		orientation = (long)(f + 0.5f);

	return orientation;
}


/********************************************************************************
 * KPSetWindowSizeOfPanoController
 ********************************************************************************/

KPPanoControllerStatus
KPSetWindowSizeOfPanoController(KPPanoController *ctlr, const long *widthHeight)
{
	ctlr->windowWidth  = widthHeight[0];
	ctlr->windowHeight = widthHeight[1];
	if ((KPGetOrientationOfPanoController(ctlr) & 1) == 0) {
		ctlr->portWidth  = ctlr->windowWidth;
		ctlr->portHeight = ctlr->windowHeight;
	}
	else {
		ctlr->portWidth  = ctlr->windowHeight;
		ctlr->portHeight = ctlr->windowWidth;
	}
	UpdateFocalLengthFromFov(ctlr);
	return ApplyPanoConstraints(ctlr, 1);
}


/********************************************************************************
 * KPGetWindowSizeOfPanoController
 ********************************************************************************/

void
KPGetWindowSizeOfPanoController(const KPPanoController *ctlr, long *widthHeight)
{
	widthHeight[0] = ctlr->windowWidth;
	widthHeight[1] = ctlr->windowHeight;
}


/********************************************************************************
 * KPSetPanSpeedOfPanoController
 *		Pan/tilt speed is given in pixels/sec/pixel; default is 2
 ********************************************************************************/

void
KPSetPanSpeedOfPanoController(KPPanoController *ctlr, float speed)
{
	ctlr->panSpeed = speed;
}


/********************************************************************************
 * KPGetPanSpeedOfPanoController
 ********************************************************************************/

float
KPGetPanSpeedOfPanoController(KPPanoController *ctlr)
{
	return ctlr->panSpeed;
}


/********************************************************************************
 * KPSetZoomSpeedOfPanoController
 *		Zoom speed is given in zoomFactor/sec; default is sqrt(2)
 ********************************************************************************/

void
KPSetZoomSpeedOfPanoController(KPPanoController *ctlr, float speed)
{
	ctlr->zoomSpeed = speed;
}


/********************************************************************************
 * KPGetZoomSpeedOfPanoController
 ********************************************************************************/

float
KPGetZoomSpeedOfPanoController(KPPanoController *ctlr)
{
	return ctlr->zoomSpeed;
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Interactive Controllers						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * KPDragVelocityOfPanoController
 *		Drag velocity, as in QTVR
 ********************************************************************************/

KPPanoControllerStatus
KPDragVelocityOfPanoController(KPPanoController *ctlr, const float *xyz, float framePeriod)
{
	float	pixToRad;
	long	fovChanged	= 0;

	if (!(framePeriod > 0))
		framePeriod = 0.02f;	/* Assume 50 frames per second unless we know better. */

	pixToRad = ctlr->panSpeed * framePeriod / ctlr->focalLength;
	ctlr->pan  -= (ctlr->winXfm[0][0] * xyz[0] + ctlr->winXfm[1][0] * xyz[1]) * pixToRad;
	ctlr->tilt -= (ctlr->winXfm[0][1] * xyz[0] + ctlr->winXfm[1][1] * xyz[1]) * pixToRad;
	if (xyz[2]) {
		#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
			ctlr->focalLength *= (float)pow(ctlr->zoomSpeed, xyz[2] * framePeriod);
		#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
			ctlr->focalLength *= powf(ctlr->zoomSpeed, xyz[2] * framePeriod);
		#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		UpdateFovFromFocalLength(ctlr);
		fovChanged = 1;
	}
	return ApplyPanoConstraints(ctlr, fovChanged);	/* This calls UpdatePegLimits() if fovChanged */
}


/********************************************************************************
 * KPDragPositionOfPanoController
 *		Drag absolute position
 ********************************************************************************/

KPPanoControllerStatus
KPDragPositionOfPanoController(KPPanoController *ctlr, const float *xyz)
{
	long		fovChanged	= 0;

	ctlr->pan  += (ctlr->winXfm[0][0] * xyz[0] + ctlr->winXfm[1][0] * xyz[1]) / ctlr->focalLength;
	ctlr->tilt += (ctlr->winXfm[0][1] * xyz[0] + ctlr->winXfm[1][1] * xyz[1]) / ctlr->focalLength;
	if (xyz[2]) {
		#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
			ctlr->focalLength *= (float)pow(ctlr->zoomSpeed, xyz[2]);
		#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
			ctlr->focalLength *= powf(ctlr->zoomSpeed, xyz[2]);
		#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		UpdateFovFromFocalLength(ctlr);
		fovChanged = 1;
	}
	return ApplyPanoConstraints(ctlr, fovChanged);	/* This calls UpdatePegLimits() if fovChanged */
}

