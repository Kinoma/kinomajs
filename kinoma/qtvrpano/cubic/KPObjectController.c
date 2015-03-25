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


#if __FSK_LAYER__
	#include "Fsk.h"
#endif /* !__FSK_LAYER__ */

#include "KPObjectController.h"

#if !__FSK_LAYER__
	#include <QuickTimeVRFormat.h>
#else /* __FSK_LAYER__ */

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

#endif /* __FSK_LAYER__ */

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


#define F_2PI	6.2831853072f
#define F_PI	3.1415926536f
#define F_PI_2	1.5707963268f


//#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh) || defined(__MWERKS__)
		#define PRAGMA_MARK_SUPPORTED 1
	#else /* !PRAGMA_MARK_SUPPORTED */
		#define PRAGMA_MARK_SUPPORTED 0
	#endif /* !PRAGMA_MARK_SUPPORTED */
//#endif /* PRAGMA_MARK_SUPPORTED */



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


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ******							KPObjectController							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


 struct KPObjectController {
 	/* Media limits */
 	float			minPan;
 	float			maxPan;
 	float			minTilt;
 	float			maxTilt;
 	float			minFov;
 	float			maxFov;

 	long			objectWidth;
 	long			objectHeight;
 	float			unitRadiansPerPixel;	/* At 1:1 zoom */

 	unsigned long	numCols;
 	unsigned long	numRows;

 	/* Controller state */
 	float			pan;
 	float			tilt;
 	float			roll;
 	float			fov;					/* This is more of an inverse scale factor than fov, as QT does it */
 	float			centerX;
 	float			centerY;
 	long			windowWidth;
 	long			windowHeight;
 	float			minWinFov;
 	float			maxWinFov;
 	long			animationSettings;
 	long			controlSettings;

	/* UI Control parameters */
 	float			panSpeed;
 	float			zoomSpeed;
 };


/********************************************************************************
 * ApplyObjectConstraints
 ********************************************************************************/

static KPObjectControllerStatus
ApplyObjectConstraints(KPObjectController *ctlr)
{
	KPObjectControllerStatus	status = 0;
	float scale, f, b, c;


	/* Apply FOV constraints */
	if (ctlr->controlSettings & kQTVRObjectCanZoomOn) {
		if (!(ctlr->fov <= ctlr->maxWinFov)) {
			ctlr->fov = ctlr->maxWinFov;
			status |= kpObjectMaxFOV;
		}
		if (!(ctlr->fov >= ctlr->minWinFov)) {
			ctlr->fov = ctlr->minWinFov;
			status |= kpObjectMinFOV;
		}
	}
	else {
		ctlr->fov = ctlr->maxWinFov;
	}


	/* Apply tilt constraints */
	if (!(ctlr->controlSettings & kQTVRObjectWrapTiltOn)) {
		if (!(ctlr->tilt >= ctlr->minTilt)) {
			ctlr->tilt = ctlr->minTilt;
			status |= kpObjectMinTilt;
		}
		if (!(ctlr->tilt <= ctlr->maxTilt)) {
			ctlr->tilt = ctlr->maxTilt;
			status |= kpObjectMaxTilt;
		}
	}
	else {	/* No constaints on tilt: get tilt into canonical space */
		if (!((ctlr->tilt >= ctlr->minTilt) && (ctlr->tilt <= ctlr->maxTilt)))
			ctlr->tilt = WrapInRange(ctlr->tilt, ctlr->minTilt, ctlr->maxTilt);
	}


	/* Apply pan constraints */
	if (!(ctlr->controlSettings & kQTVRObjectWrapPanOn)) {
		if (!(ctlr->pan >= ctlr->minPan)) {
			ctlr->pan = ctlr->minPan;
			status |= kpObjectMinPan;
		}
		if (!(ctlr->pan <= ctlr->maxPan)) {
			ctlr->pan = ctlr->maxPan;
			status |= kpObjectMaxPan;
		}
	}
	else {	/* No constaints on pan: get pan into canonical space */
		if (!((ctlr->pan >= ctlr->minPan) && (ctlr->pan <= ctlr->maxPan)))
			ctlr->pan = WrapInRange(ctlr->pan, ctlr->minPan, ctlr->maxPan);
	}


	/* Apply translate constraints */
	scale = ctlr->fov / ctlr->maxFov;	/* actually, the inverse scale, mapping dst -> src */

	/* X translation constraints */
	f = (ctlr->objectWidth - 1) * 0.5f;
	b = f - (ctlr->windowWidth   - 1) * 0.5f * scale;
	c = ctlr->centerX - f;
	if (c < -b) {
		c = -b;
		status |= kpObjectMinX;
	}
	if (c > b) {
		c = b;
		status |= kpObjectMaxX;
	}
	if ((status & (kpObjectMinX | kpObjectMaxX)) == (kpObjectMinX | kpObjectMaxX)) {	/* trouble */
		c = 0;
	}
	ctlr->centerX = c + f;

	/* Y translation constraints */
	f = (ctlr->objectHeight     - 1) * 0.5f;
	b = f - (ctlr->windowHeight - 1) * 0.5f * scale;
	c = ctlr->centerY - f;
	if (c < -b) {
		c = -b;
		status |= kpObjectMinY;
	}
	if (c > b) {
		c = b;
		status |= kpObjectMaxY;
	}
	if ((status & (kpObjectMinY | kpObjectMaxY)) == (kpObjectMinY | kpObjectMaxY)) {	/* trouble */
		c = 0;
	}
	ctlr->centerY = c + f;

	return status;
}


/********************************************************************************
 * SetWinFovs
 ********************************************************************************/

static void
SetWinFovs(KPObjectController *ctlr)
{
	float xScale, yScale, scale;

	ctlr->maxWinFov = ctlr->maxFov;
	ctlr->minWinFov = ctlr->minFov;
	if ((ctlr->objectWidth > 1) && (ctlr->objectHeight > 1)) {
		xScale = (float)(ctlr->windowWidth  - 1) / (float)(ctlr->objectWidth  - 1);
		yScale = (float)(ctlr->windowHeight - 1) / (float)(ctlr->objectHeight - 1);
		scale = (xScale > yScale) ? xScale : yScale;
		if (scale > 1) {
			ctlr->maxWinFov /= scale;
			if (ctlr->minWinFov > ctlr->maxWinFov)
				ctlr->minWinFov = ctlr->maxWinFov;
		}
	}
}


/********************************************************************************
 * KPNewObjectController
 ********************************************************************************/

KPObjectController*
KPNewObjectController(void)
{
	KPObjectController *ctlr;

	if ((ctlr = (KPObjectController*)ALLOC(sizeof(KPObjectController))) != NULL) {
		/* Set default media limits */
		ctlr->minPan			=  0.0f;		/* Pan from 0 degrees ... */
		ctlr->maxPan			=  F_2PI;		/* ... to 360 degrees */
		ctlr->minTilt			= -F_PI_2;		/* Tilt from -90 degrees ... */
		ctlr->maxTilt			=  F_PI_2;		/* ... to +90 degrees */
		ctlr->minFov			=  8.01f;		/* 8:1 zoom */
		ctlr->maxFov			=  64.0f;		/* Reference for zoom */

		ctlr->objectWidth		= 1;
		ctlr->objectHeight		= 1;
		ctlr->unitRadiansPerPixel	= F_PI;

		ctlr->numCols			= 0;
		ctlr->numRows			= 0;

		/* Set default controller state */
		ctlr->pan				=  0.0f;		/* Right on the seam */
		ctlr->tilt				=  0.0f;		/* Perfectly level attitude */
		ctlr->roll				=  0.0f;		/* No rocking */
		ctlr->fov				=  1.0f;		/* Anout 60 degrees */
		ctlr->centerX			=  192;			/* Arbitrary center */
		ctlr->centerY			=  128;			/* Arbitrary center */
		ctlr->windowWidth		=  384;			/* Arbitrary width */
		ctlr->windowHeight		=  256;			/* Arbitrary height */
		ctlr->minWinFov			= ctlr->minFov;
		ctlr->maxWinFov			= ctlr->maxFov;
		ctlr->animationSettings	= 0;
		ctlr->controlSettings	= kQTVRObjectWrapPanOn | kQTVRObjectCanZoomOn | kQTVRObjectTranslationOn;

		/* Set default UI parameters */
		ctlr->panSpeed			=  F_PI;
		ctlr->zoomSpeed			=  1.4142135624f;

		/* Ripple state */
		ApplyObjectConstraints(ctlr);
	}

	return ctlr;
}


/********************************************************************************
 * KPDeleteObjectController
 ********************************************************************************/

void
KPDeleteObjectController(KPObjectController *ctlr)
{
	FREE(ctlr);
}


/********************************************************************************
 * KPSetViewConstraintsOfObjectController
 ********************************************************************************/

void
KPSetViewConstraintsOfObjectController(KPObjectController *ctlr, const float *minMaxPanTiltFOV)
{
	ctlr->minPan	= minMaxPanTiltFOV[0];
	ctlr->maxPan	= minMaxPanTiltFOV[1];
	ctlr->minTilt	= minMaxPanTiltFOV[2];
	ctlr->maxTilt	= minMaxPanTiltFOV[3];
	ctlr->minFov	= minMaxPanTiltFOV[4];
	ctlr->maxFov	= minMaxPanTiltFOV[5];
	SetWinFovs(ctlr);
	ApplyObjectConstraints(ctlr);
}


/********************************************************************************
 * KPGetViewConstraintsOfObjectController
 ********************************************************************************/

void
KPGetViewConstraintsOfObjectController(const KPObjectController *ctlr, float *minMaxPanTiltFOV)
{
	minMaxPanTiltFOV[0] = ctlr->minPan;
	minMaxPanTiltFOV[1] = ctlr->maxPan;
	minMaxPanTiltFOV[2] = ctlr->minTilt;
	minMaxPanTiltFOV[3] = ctlr->maxTilt;
	minMaxPanTiltFOV[4] = ctlr->minFov;
	minMaxPanTiltFOV[5] = ctlr->maxFov;
}



/********************************************************************************
 * KPSetPanTiltFOVOfObjectController
 ********************************************************************************/

KPObjectControllerStatus
KPSetPanTiltFOVOfObjectController(KPObjectController *ctlr, const float *panTiltFOV)
{

	ctlr->pan  = panTiltFOV[0];
	ctlr->tilt = panTiltFOV[1];
	ctlr->fov  = panTiltFOV[2];
	return ApplyObjectConstraints(ctlr);
}


/********************************************************************************
 * KPGetPanTiltFOVOfObjectController
 ********************************************************************************/


void
KPGetPanTiltFOVOfObjectController(const KPObjectController *ctlr, float *panTiltFOV)
{
	panTiltFOV[0] = ctlr->pan;
	panTiltFOV[1] = ctlr->tilt;
	panTiltFOV[2] = ctlr->fov;
}


/********************************************************************************
 * KPSetPanTiltOfObjectController
 ********************************************************************************/

KPObjectControllerStatus
KPSetPanTiltOfObjectController(KPObjectController *ctlr, const float *panTilt)
{
	ctlr->pan  = panTilt[0];
	ctlr->tilt = panTilt[1];
	return ApplyObjectConstraints(ctlr);
}


/********************************************************************************
 * KPGetPanTiltOfObjectController
 ********************************************************************************/

void
KPGetPanTiltOfObjectController(const KPObjectController *ctlr, float *panTilt)
{
	panTilt[0] = ctlr->pan;
	panTilt[1] = ctlr->tilt;
}


/********************************************************************************
 * KPSetPanOfObjectController
 ********************************************************************************/

KPObjectControllerStatus
KPSetPanOfObjectController(KPObjectController *ctlr, float pan)
{
	ctlr->pan = pan;
	return ApplyObjectConstraints(ctlr);
}


/********************************************************************************
 * KPGetPanOfObjectController
 ********************************************************************************/

float
KPGetPanOfObjectController(const KPObjectController *ctlr)
{
	return ctlr->pan;
}


/********************************************************************************
 * KPSetTiltOfObjectController
 ********************************************************************************/

KPObjectControllerStatus
KPSetTiltOfObjectController(KPObjectController *ctlr, float tilt)
{
	ctlr->tilt = tilt;
	return ApplyObjectConstraints(ctlr);

}


/********************************************************************************
 * KPGetTiltOfObjectController
 ********************************************************************************/

float
KPGetTiltOfObjectController(const KPObjectController *ctlr)
{
	return ctlr->tilt;
}


/********************************************************************************
 * KPSetFOVOfObjectController
 ********************************************************************************/

KPObjectControllerStatus
KPSetFOVOfObjectController(KPObjectController *ctlr, float fov)
{
	ctlr->fov = fov;
	return ApplyObjectConstraints(ctlr);
}


/********************************************************************************
 * KPGetFOVOfObjectController
 ********************************************************************************/

float
KPGetFOVOfObjectController(const KPObjectController *ctlr)
{
	return ctlr->fov;
}


/********************************************************************************
 * KPSetCenterOfObjectController
 ********************************************************************************/

KPObjectControllerStatus
KPSetCenterOfObjectController(KPObjectController *ctlr, const float *centerXY)
{
	ctlr->centerX = centerXY[0];
	ctlr->centerY = centerXY[1];
	return ApplyObjectConstraints(ctlr);
}


/********************************************************************************
 * KPGetCenterOfObjectController
 ********************************************************************************/

void
KPGetCenterOfObjectController(const KPObjectController *ctlr, float *centerXY)
{
	centerXY[0] = ctlr->centerX;
	centerXY[1] = ctlr->centerY;
}


/********************************************************************************
 * ComputeObjectUnitRadiansPerPixel
 *	Call this if objectWidth, objectHeight, or panSpeed changes
 ********************************************************************************/

static void
ComputeObjectUnitRadiansPerPixel(KPObjectController *ctlr)
{
	long nrm;

	nrm = ((ctlr->objectWidth < ctlr->objectHeight) ? ctlr->objectWidth : ctlr->objectHeight) - 1;
	if (nrm < 1)
		nrm = 1;
	ctlr->unitRadiansPerPixel = ctlr->panSpeed / nrm;
}


/********************************************************************************
 * KPSetObjectSizeOfObjectController
 ********************************************************************************/

void
KPSetObjectSizeOfObjectController(KPObjectController *ctlr, const long *widthHeight)
{
	ctlr->objectWidth  = widthHeight[0];
	ctlr->objectHeight = widthHeight[1];
	ComputeObjectUnitRadiansPerPixel(ctlr);
}


/********************************************************************************
 * KPGetObjectSizeOfObjectController
 ********************************************************************************/

void
KPGetObjectSizeOfObjectController(const KPObjectController *ctlr, long *widthHeight)
{
	widthHeight[0] = ctlr->objectWidth;
	widthHeight[1] = ctlr->objectHeight;
}


/********************************************************************************
 * KPSetRowsColumnsOfObjectController
 ********************************************************************************/

void
KPSetRowsColumnsOfObjectController(KPObjectController *ctlr, const unsigned long *numRowsColumns)
{
	ctlr->numRows = numRowsColumns[0];
	ctlr->numCols = numRowsColumns[1];
}


/********************************************************************************
 * KPGetRowsColumnsOfObjectController
 ********************************************************************************/

void
KPGetRowsColumnsOfObjectController(const KPObjectController *ctlr, unsigned long *numRowsColumns)
{
	numRowsColumns[0] = ctlr->numRows;
	numRowsColumns[1] = ctlr->numCols;
}


/********************************************************************************
 * KPSetWindowSizeOfObjectController
 ********************************************************************************/

KPObjectControllerStatus
KPSetWindowSizeOfObjectController(KPObjectController *ctlr, const long *widthHeight)
{
	ctlr->windowWidth  = widthHeight[0];
	ctlr->windowHeight = widthHeight[1];
	SetWinFovs(ctlr);
	return ApplyObjectConstraints(ctlr);
}


/********************************************************************************
 * KPGetWindowSizeOfObjectController
 ********************************************************************************/

void
KPGetWindowSizeOfObjectController(const KPObjectController *ctlr, long *widthHeight)
{
	widthHeight[0] = ctlr->windowWidth;
	widthHeight[1] = ctlr->windowHeight;
}


/********************************************************************************
 * KPSetPanSpeedOfObjectController
 ********************************************************************************/

void
KPSetPanSpeedOfObjectController(KPObjectController *ctlr, float velocity)
{
	ctlr->panSpeed = velocity;
}


/********************************************************************************
 * KPGetPanSpeedOfObjectController
 ********************************************************************************/

float
KPGetPanSpeedOfObjectController(KPObjectController *ctlr)
{
	return ctlr->panSpeed;
}


/********************************************************************************
 * KPSetZoomSpeedOfObjectController
 ********************************************************************************/

void
KPSetZoomSpeedOfObjectController(KPObjectController *ctlr, float velocity)
{
	ctlr->zoomSpeed = velocity;
}


/********************************************************************************
 * KPGetZoomSpeedOfObjectController
 ********************************************************************************/

float
KPGetZoomSpeedOfObjectController(KPObjectController *ctlr)
{
	return ctlr->zoomSpeed;
}


/********************************************************************************
 * KPSetAnimationSettingsOfObjectController
 ********************************************************************************/

void
KPSetAnimationSettingsOfObjectController(KPObjectController *ctlr, long settings)
{
	ctlr->animationSettings = settings;
}


/********************************************************************************
 * KPGetAnimationSettingsOfObjectController
 ********************************************************************************/

long
KPGetAnimationSettingsOfObjectController(KPObjectController *ctlr)
{
	return ctlr->animationSettings;
}


/********************************************************************************
 * KPSetControlSettingsOfObjectController
 ********************************************************************************/

void
KPSetControlSettingsOfObjectController(KPObjectController *ctlr, long settings)
{
	ctlr->controlSettings = settings;
}


/********************************************************************************
 * KPGetControlSettingsOfObjectController
 ********************************************************************************/

long
KPGetControlSettingsOfObjectController(KPObjectController *ctlr)
{
	return ctlr->controlSettings;
}


/********************************************************************************
 * KPDragPositionOfObjectController
 ********************************************************************************/

KPObjectControllerStatus
KPDragPositionOfObjectController(KPObjectController *ctlr, const float *xyz, long translateMode)
{
	float dx, dy, scale;

	scale = ctlr->fov / ctlr->maxFov;	/* smaller fov -> greater zoom -> finer adjustments */

	/* Apply control transformation */
	if (!(ctlr->controlSettings & kQTVRObjectSwapHVControlOn)) {
		dx = xyz[0];
		dy = xyz[1];
	}
	else {
		dx = xyz[1];
		dy = xyz[0];
	}
	if (ctlr->controlSettings & kQTVRObjectReverseHControlOn)	dx = -dx;
	if (ctlr->controlSettings & kQTVRObjectReverseVControlOn)	dy = -dy;

	/* Increment parameters, depending on the mode */
	if (!translateMode) {		/* pan/tilt mode */
		scale *= ctlr->unitRadiansPerPixel;
		ctlr->pan  += dx * scale;
		ctlr->tilt += dy * scale;
	}
	else {						/* translate mode */
		ctlr->centerX -= dx * scale;
		ctlr->centerY -= dy * scale;
	}

	if ((ctlr->controlSettings & kQTVRObjectCanZoomOn) && xyz[2]) {
		#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
			ctlr->fov *= (float)pow(ctlr->zoomSpeed, -xyz[2]);
		#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
			ctlr->fov *= powf(ctlr->zoomSpeed, -xyz[2]);
		#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
	}
	return ApplyObjectConstraints(ctlr);
}


/********************************************************************************
 * KPGetScaleOffsetFrameOfObjectController
 ********************************************************************************/

void
KPGetScaleOffsetFrameOfObjectController(const KPObjectController *ctlr, float scale[2], float offset[2], long *frame)
{
	unsigned long	row, col;
	float	t;

	/* Compute transformation
	 *	dstX = srcX * scale + offset[0];
	 *	dstY = srcY * scale + offset[1];
	 */
	scale[0]  = ctlr->maxFov / ctlr->fov;	/* Orthographic FOV */
	scale[1]  = scale[0];					/* Isotropic */
	offset[0] = (ctlr->windowWidth  - 1) * 0.5f - scale[0] * ctlr->centerX;
	offset[1] = (ctlr->windowHeight - 1) * 0.5f - scale[1] * ctlr->centerY;

	/* Compute column */
	col = 0;
	if (ctlr->numCols > 1) {
	//	t = (ctlr->pan    - ctlr->minPan) / (ctlr->maxPan - ctlr->minPan);
		t = (ctlr->maxPan - ctlr->pan)    / (ctlr->maxPan - ctlr->minPan);
		if (ctlr->controlSettings & kQTVRObjectWrapPanOn) {
#if defined(ANDROID)
			double p;
			p = (ctlr->numCols * t);
			p = p + 0.5;
			col = (unsigned long)p;
#else
			col = (unsigned long)(ctlr->numCols * t + 0.5f);
#endif
			if (col >= ctlr->numCols)
				col -= ctlr->numCols;
		}
		else {
#if defined(ANDROID)
			double p;
			p = (ctlr->numCols - 1) * t;
			p = p + 0.5;
			col = (unsigned long)p;
#else
			col = (unsigned long)((ctlr->numCols - 1) * t + 0.5f);
#endif
			if (col >= ctlr->numCols)
				col = ctlr->numCols - 1;
		}
	}

	/* Compute row */
	row = 0;
	if (ctlr->numRows > 1) {
		t = (ctlr->maxTilt - ctlr->tilt) / (ctlr->maxTilt - ctlr->minTilt);	/* Frame 0 is at max tilt */
		if (ctlr->controlSettings & kQTVRObjectWrapTiltOn) {
			row = (long)(ctlr->numRows * t + 0.5f);
			if (row >= ctlr->numRows)
				row -= ctlr->numRows;
		}
		else {
			row = (long)((ctlr->numRows - 1) * t + 0.5f);
			if (row >= ctlr->numRows)
				row = ctlr->numRows - 1;
		}
	}

	/* Compute frame */
	*frame = row * ctlr->numCols + col;
}

