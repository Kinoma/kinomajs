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
#define __FSKBITMAP_PRIV__

#include "FskMatrix.h"
#include "FskPerspective.h"
#include "FskProjectImage.h"
#include "FskUtilities.h"

#include <math.h>
#if TARGET_OS_WIN32 || (TARGET_OS_KPL && defined(_MSC_VER))
	#define M_PI 3.14159265358979323846
#endif

#ifdef FixedToFloat
	#undef FixedToFloat
#endif /* FixedToFloat */
#define FixedToFloat(x)		((x) * (1.0f / 65536.0f))


/* 	In order to enable debug output on Android, it is necessary to:
 *	(1) #define SUPPORT_INSTRUMENTATION 1 in FskPlatform.android.h
 *	(2) put this into manifest.xml:
 *			<instrument platform="android" androidlog="true" threads="true" trace="false">
 *				<kind name="projectbitmap" messages="debug"/>
 *			</instrument>
 *			<instrument platform="mac" threads="true" trace="true">
 *				<kind name="projectbitmap" messages="debug"/>
 *			</instrument>
 *		On android, you can see the output by executing "adb logcat" on Android,
 *		while on the Mac, the output will go to stdout, but
 *	(3) Perhaps #define LOG_PARAMETERS or GL_DEBUG
 *	(4) fskbuild --clean [ --run ] [ --test ]
 *	(5a) To view the output on Android, type "adb logcat" in the host (Mac or Linux) Terminal window.
 *	(5b) To view the output on the Mac, you can
 *	     - launch from XCode, and the output will be displayed in the console window.
 *	     - launch from Terminal with:
 *	     	cd ${F_HOME}/bin/mac/debug/Kinoma Simulator.app/Contents/MacOS"
 *	     	./fsk"
 *	       and you will be able to see the output in the Terminal window.
 */


#define LOG_PARAMETERS

#if SUPPORT_INSTRUMENTATION
	#define PERSPECTIVE_DEBUG 1
#else /* SUPPORT_INSTRUMENTATION */
	#undef LOG_PARAMETERS
	#define PERSPECTIVE_DEBUG 0
#endif /* SUPPORT_INSTRUMENTATION */

FskInstrumentedSimpleType(ProjectBitmap, projectbitmap);

#if PERSPECTIVE_DEBUG
	#define	LOGD(...)	do { FskInstrumentedTypePrintfDebug  (&gProjectBitmapTypeInstrumentation, __VA_ARGS__); } while(0)
	#define	LOGI(...)	do { FskInstrumentedTypePrintfVerbose(&gProjectBitmapTypeInstrumentation, __VA_ARGS__); } while(0)
#endif /* PERSPECTIVE_DEBUG */
#define		LOGE(...)	do { FskInstrumentedTypePrintfMinimal(&gProjectBitmapTypeInstrumentation, __VA_ARGS__); } while(0)
#ifndef LOGD
	#define	LOGD(...)
#endif /* LOGD */
#ifndef LOGI
	#define	LOGI(...)
#endif /* LOGI */
#ifndef LOGE
	#define	LOGE(...)
#endif /* LOGE */

#if PERSPECTIVE_DEBUG
#ifdef LOG_PARAMETERS
#include "FskPixelOps.h"
typedef struct LookupEntry { int code;	const char *str; } LookupEntry;
static const char* StringFromCode(int code, const LookupEntry *tab) {
	const LookupEntry *p;
	for (p = tab; p->code != 0; ++p)
		if (code == p->code)
			break;
	return p->str;
}
static const char* ModeString(UInt32 mode) {
	static const LookupEntry modeTab[] = {
		{	kFskGraphicsModeBilinear|kFskGraphicsModeCopy,		"BilinearCopy"		},
		{	kFskGraphicsModeBilinear|kFskGraphicsModeAlpha,		"BilinearAlpha"		},
		{	kFskGraphicsModeBilinear|kFskGraphicsModeColorize,	"BilinearColorize"	},
		{	kFskGraphicsModeCopy,								"Copy"				},
		{	kFskGraphicsModeAlpha,								"Alpha"				},
		{	kFskGraphicsModeColorize,							"Colorize"			},

	};
	return StringFromCode(mode, modeTab);
}

static void LogProjectBitmap(
	FskConstBitmap					srcBM,		/* This should be declared FskConstBitmap */
	FskConstRectangle				srcRect,	/* Map this rect... */
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,	/* But clip thuswise */
	const float						M[3][3],
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* We get blend level and tint color from here */
) {
	LOGD("ProjectBitmap(srcBM=%p, srcRect=%p, dstBM=%p, dstClip=%p, M=%p, opColor=%p, mode=%s, modeParams=%p)",
		srcBM, srcRect, dstBM, dstClip, M, opColor, ModeString(mode), modeParams);
	LOGD("\tsrcBM: bounds(%d, %d, %d, %d), depth=%u, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d",
		(int)srcBM->bounds.x, (int)srcBM->bounds.y, (int)srcBM->bounds.width, (int)srcBM->bounds.height, (unsigned)srcBM->depth,
		FskBitmapFormatName(srcBM->pixelFormat), (int)srcBM->rowBytes, srcBM->bits, srcBM->hasAlpha, srcBM->alphaIsPremultiplied);
	if (srcRect) LOGD("\tsrcRect(%d, %d, %d, %d)", (int)srcRect->x, (int)srcRect->y, (int)srcRect->width, (int)srcRect->height);
	LOGD("\tdstBM: bounds(%d, %d, %d, %d), depth=%u, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d",
		(int)dstBM->bounds.x, (int)dstBM->bounds.y, (int)dstBM->bounds.width, (int)dstBM->bounds.height, (unsigned)dstBM->depth,
		FskBitmapFormatName(dstBM->pixelFormat), (int)dstBM->rowBytes, dstBM->bits, dstBM->hasAlpha, dstBM->alphaIsPremultiplied);
	if (dstClip) LOGD("\tdstClip(%d, %d, %d, %d)", (int)dstClip->x, (int)dstClip->y, (int)dstClip->width, (int)dstClip->height);
	if (M)	LOGD("\tM={ {%.3g %.3g %.3g}, {%.3g %.3g %.3g}, {%.3g %.3g %.3g} }",
				M[0][0],	M[0][1],	M[0][2],
				M[1][0],	M[1][1],	M[1][2],
				M[2][0],	M[2][1],	M[2][2]);
	if (opColor) LOGD("\topColor(%d, %d, %d, %d)", opColor->r, opColor->g, opColor->b, opColor->a);
	if (modeParams) {
		if (modeParams->dataSize <= sizeof(FskGraphicsModeParametersRecord)) {
			LOGD("\tmodeParams: dataSize=%u, blendLevel=%d", (unsigned)modeParams->dataSize, (int)modeParams->blendLevel);
		} else {
			FskGraphicsModeParametersVideo videoParams = (FskGraphicsModeParametersVideo)modeParams;
			LOGD("\tmodeParams: dataSize=%u, blendLevel=%d, kind='%4s, contrast=%f, brightness=%f, sprites=%p",
				(unsigned)videoParams->header.dataSize, (int)videoParams->header.blendLevel, (char*)(&videoParams->kind),
				 videoParams->contrast/65536.0, videoParams->brightness/65536.0, videoParams->sprites);
		}
	}
}
#endif /* LOG_PARAMETERS */

#endif /* PERSPECTIVE_DEBUG */


/********************************************************************************
 * FskProjectBitmap
 ********************************************************************************/

FskErr FskProjectBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	const float						M[3][3],
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
)
{
	FskErr		err				= kFskErrNone;
	char		*srcBaseAddr	= NULL;
	char		*dstBaseAddr	= NULL;
	float		T[3][3];
	FskRectangleRecord	srcBounds, dstBounds;

	#if defined(LOG_PARAMETERS)
		LogProjectBitmap(srcBM, srcRect, dstBM, dstClip, M, opColor, mode, modeParams);
	#endif /* LOG_PARAMETERS */

	if ((M[0][2] == 0) && (M[1][2] == 0))
		mode |= kFskGraphicsModeAffine;
	if (srcBM->hasAlpha && srcBM->alphaIsPremultiplied)
		mode |= kFskSrcIsPremul;

	/* Get rects of rendering */
	if (srcRect)	BAIL_IF_FALSE(FskRectangleIntersect(srcRect, &srcBM->bounds, &srcBounds), err, kFskErrNothingRendered);
	else			srcBounds = srcBM->bounds;
	if (dstClip)	BAIL_IF_FALSE(FskRectangleIntersect(dstClip, &dstBM->bounds, &dstBounds), err, kFskErrNothingRendered);
	else			dstBounds = dstBM->bounds;

	/* Get bitmap accessors */
	BAIL_IF_ERR(err = FskBitmapReadBegin ((FskBitmap)srcBM, (const void**)(const void*)(&srcBaseAddr), NULL, NULL));
	BAIL_IF_ERR(err = FskBitmapWriteBegin(           dstBM,       (void**)      (void*)(&dstBaseAddr), NULL, NULL));

	FskMemCopy(&T[0][0], &M[0][0], sizeof(T));

	/* Adjust matrix based on src bounds */
	if (srcBounds.x || srcBounds.y) {
		T[2][0] += T[0][0] * srcBounds.x + T[1][0] * srcBounds.y;
		T[2][1] += T[0][1] * srcBounds.x + T[1][1] * srcBounds.y;
		T[2][2] += T[0][2] * srcBounds.x + T[1][2] * srcBounds.y;
		srcBaseAddr += srcBounds.y *  srcBM->rowBytes
					+  srcBounds.x * (srcBM->depth >> 3);
	}

	/* Adjust matrix and dstBaseAddr based on dst bounds */
	if (dstBounds.x) {
		T[0][0] -= T[0][2] * dstBounds.x;
		T[1][0] -= T[1][2] * dstBounds.x;
		T[2][0] -= T[2][2] * dstBounds.x;
		dstBaseAddr += dstBounds.x * (dstBM->depth >> 3);
	}
	if (dstBounds.y) {
		T[0][1] -= T[0][2] * dstBounds.y;
		T[1][1] -= T[1][2] * dstBounds.y;
		T[2][1] -= T[2][2] * dstBounds.y;
		dstBaseAddr += dstBounds.y * dstBM->rowBytes;
	}

	err = FskProjectImage(srcBaseAddr, srcBM->pixelFormat, srcBM->rowBytes, srcBounds.width, srcBounds.height,
			(const float(*)[3])T, 0, NULL, mode, modeParams,
			dstBaseAddr, dstBM->pixelFormat, dstBM->rowBytes, dstBounds.width, dstBounds.height, 0, NULL
		);

bail:
	if (dstBaseAddr)	FskBitmapWriteEnd(dstBM);
	if (srcBaseAddr)	FskBitmapReadEnd((FskBitmap)srcBM);
	return err;
}


#if 0
/*******************************************************************************
 * PrintMatrix
 *******************************************************************************/

static void PrintMatrix(const char *str, const float *M, int nRows, int nCols)
{
	int i, j;
	printf("\n");
	if (str)
		printf("%s:\n", str);
	for (i = nRows; i--; ) {
		printf("\t|");
		for (j = nCols; j--; )
			printf("%12.6g", *M++);
		printf("  |\n");
	}
}
#endif /* 0 */


/********************************************************************************
 * QuaternionWXYZToRowMatrix
 ********************************************************************************/

static void QuaternionWXYZToRowMatrix(const FskQuaternionWXYZ *q, float R[4][4]) {
	double a, b, c;

	/* Diagonal terms */
	a = q->x; a *= a;
	b = q->y; b *= b;
	c = q->z; c *= c;
	R[0][0] = (float)(1. - 2. * (b + c));
	R[1][1] = (float)(1. - 2. * (a + c));
	R[2][2] = (float)(1. - 2. * (a + b));

	/* Skew terms */
	a = (double)q->x * (double)q->y;
	b = (double)q->w * (double)q->z;
	R[0][1] = (float)(2. * (a + b));
	R[1][0] = (float)(2. * (a - b));

	a = (double)q->y * (double)q->z;
	b = (double)q->w * (double)q->x;
	R[1][2] = (float)(2. * (a + b));
	R[2][1] = (float)(2. * (a - b));

	a = (double)q->x * (double)q->z;
	b = (double)q->w * (double)q->y;
	R[2][0] = (float)(2. * (a + b));
	R[0][2] = (float)(2. * (a - b));

	/* Embedding terms */
	R[0][3] = 0.f;
	R[1][3] = 0.f;
	R[2][3] = 0.f;
	R[3][0] = 0.f;
	R[3][1] = 0.f;
	R[3][2] = 0.f;
	R[3][3] = 1.f;
}


/********************************************************************************
 * TranslationToRowMatrix
 ********************************************************************************/

static void TranslationToRowMatrix(const FskVector3F *translation, float T[4][4]) {
	T[0][0] = 1.f;				T[0][1] = 0.f;				T[0][2] = 0.f;				T[0][3] = 0.f;
	T[1][0] = 0.f;				T[1][1] = 1.f;				T[1][2] = 0.f;				T[1][3] = 0.f;
	T[2][0] = 0.f;				T[2][1] = 0.f;				T[2][2] = 1.f;				T[2][3] = 0.f;
	T[3][0] = translation->x;	T[3][1] = translation->y;	T[3][2] = translation->z;	T[3][3] = 1.f;
}


/********************************************************************************
 * FskMultiplyQuaternionWXYZ
 ********************************************************************************/

FskQuaternionWXYZ* FskMultiplyQuaternionWXYZ(const FskQuaternionWXYZ *q1, const FskQuaternionWXYZ *q2, FskQuaternionWXYZ *qf) {
	FskQuaternionWXYZ qr;

	qr.w = (float)(	(double)q1->w * q2->w
				-	(double)q1->x * q2->x
				-	(double)q1->y * q2->y
				-	(double)q1->z * q2->z);
	qr.x = (float)(	(double)q2->w * q1->x
				+	(double)q1->w * q2->x
				+	(double)q1->y * q2->z
				-	(double)q1->z * q2->y);
	qr.y = (float)(	(double)q2->w * q1->y
				+	(double)q1->w * q2->y
				+	(double)q1->z * q2->x
				-	(double)q1->x * q2->z);
	qr.z = (float)(	(double)q2->w * q1->z
				+	(double)q1->w * q2->z
				+	(double)q1->x * q2->y
				-	(double)q1->y * q2->x);
	*qf = qr;
	return qf;
}


#define ROLL_TILT_PAN_RIGHT	FSK_EULER_ORDER(1, 2, 0, 1)	/* Z-X-Y right-handed */
#define ROLL_TILT_PAN_LEFT	FSK_EULER_ORDER(0, 2, 0, 1)	/* Z-X-Y  left-handed */
#define DEFAULT_EULER_ORDER	ROLL_TILT_PAN_LEFT			/* Default order is left-handed { roll, tilt, pan }, i.e. { Z, X, Y } */


/********************************************************************************
 * FskEulerToQuaternionWXYZ
 ********************************************************************************/

FskQuaternionWXYZ* FskEulerToQuaternionWXYZ(const float euler[3], int order, FskQuaternionWXYZ *q)
{
	double	e, ang, c1, s1, c2, s2, c3, s3;
	int		e1, e2, e3;

	if (order == 0)
		order = DEFAULT_EULER_ORDER;

	e	= (double)(1 - ((order >> (FSK_EULER_SG_POS - 1)) & 2));	/* Extract signature */
	e1	= (order >> FSK_EULER_E1_POS) & FSK_EULER_E0_MASK;			/* Extract first  rotation axis */
	e2	= (order >> FSK_EULER_E2_POS) & FSK_EULER_E0_MASK;			/* Extract second rotation axis */
	e3	= (order >> FSK_EULER_E3_POS) & FSK_EULER_E0_MASK;			/* Extract third  rotation axis */
	ang = euler[e1] * 0.5;	c1 = cos(ang);	s1 = sin(ang);			/* First  angle */
	ang = euler[e2] * 0.5;	c2 = cos(ang);	s2 = sin(ang);			/* Second angle */
	ang = euler[e3] * 0.5;	c3 = cos(ang);	s3 = sin(ang);			/* Third  angle */
    q->w        = (float)(c3 * c2 * c1 - e * s3 * s2 * s1);			/* p0, scalar component */
    (&q->x)[e1] = (float)(c3 * c2 * s1 + e * s3 * s2 * c1);			/* p1 */
	(&q->x)[e2] = (float)(c3 * s2 * c1 - e * s3 * c2 * s1);			/* p2 */
	(&q->x)[e3] = (float)(s3 * c2 * c1 + e * c3 * s2 * s1);			/* p3 */

	return q;
}


/********************************************************************************
 * FskQuaternionWXYZToEuler
 ********************************************************************************/

float* FskQuaternionWXYZToEuler(const FskQuaternionWXYZ *q, int order, float euler[3]) {
	double	e, p0, p1, p2, p3, t;
	int		e1, e2, e3;

	if (order == 0)
		order = DEFAULT_EULER_ORDER;

	e	= (double)(1 - ((order >> (FSK_EULER_SG_POS - 1)) & 2));	/* Extract signature */
	e1	= (order >> FSK_EULER_E1_POS) & FSK_EULER_E0_MASK;			/* Extract first  rotation axis */
	e2	= (order >> FSK_EULER_E2_POS) & FSK_EULER_E0_MASK;			/* Extract second rotation axis */
	e3	= (order >> FSK_EULER_E3_POS) & FSK_EULER_E0_MASK;			/* Extract third  rotation axis */
	p0 = q->w;														/* Extract scalar component */
	p1 = (&q->x)[e1];												/* Extract component of the first  rotation axis */
	p2 = (&q->x)[e2];												/* Extract component of the second rotation axis */
	p3 = (&q->x)[e3];												/* Extract component of the third  rotation axis */

	t	= 2. * (p0 * p2 + e * p1 * p3);
	if (t > +.999) {
		euler[e1] = (float)(+2.0 * atan2(p1, p0));
		euler[e2] = (float)(+0.5 * M_PI);
		euler[e3] = 0.f;
	}
	else if (t < -.999) {
		euler[e1] = (float)(-2.0 * atan2(p1, p0));
		euler[e2] = (float)(-0.5 * M_PI);
		euler[e3] = 0.f;
	}
	else {
		euler[e1] = (float)atan2(2. * (p0 * p1 - e * p2 * p3), 1. - 2. * (p1 * p1 + p2 * p2));	/* [ -PI,   +PI   ) */
		euler[e2] = (float)asin(t);																/* [ -PI/2, +PI/2 ) */
		euler[e3] = (float)atan2(2. * (p0 * p3 - e * p1 * p2), 1. - 2. * (p2 * p2 + p3 * p3));	/* [ -PI,   +PI   ) */
	}
	return euler;
}


/********************************************************************************
 * FskQuaternionWXYZToPanTiltRoll
 ********************************************************************************/

float* FskQuaternionWXYZToPanTiltRoll(const FskQuaternionWXYZ *q, float *panTiltRoll) {
	float xyz[3];
	FskQuaternionWXYZToEuler(q, ROLL_TILT_PAN_LEFT, xyz);
	panTiltRoll[0] = xyz[1];	/* Pan  is rotation about Y */
	panTiltRoll[1] = xyz[0];	/* Tilt is rotation about X */
	panTiltRoll[2] = xyz[2];	/* Roll is rotation about Z */
	return panTiltRoll;
}


/********************************************************************************
 * FskPanTiltRollToQuaternionWXYZ
 ********************************************************************************/

FskQuaternionWXYZ* FskPanTiltRollToQuaternionWXYZ(float pan, float tilt, float roll, FskQuaternionWXYZ *q) {
	float euler[3] = { tilt, pan, roll };		/* Order as rotations about X, Y, and Z axes */
	FskEulerToQuaternionWXYZ(euler, ROLL_TILT_PAN_LEFT, q);
	return q;
}


/********************************************************************************
 * Vector3FNegated
 ********************************************************************************/

static FskVector3F* Vector3FNegated(const FskVector3F *p, FskVector3F *n) {
	n->x = -p->x;
	n->y = -p->y;
	n->z = -p->z;
	return n;
}


/********************************************************************************
 * QuaternionWXYZConjugated.
 * This works in-place.
 ********************************************************************************/

FskQuaternionWXYZ* FskQuaternionWXYZConjugated(const FskQuaternionWXYZ *q, FskQuaternionWXYZ *qc) {
	qc->w =  q->w;
	qc->x = -q->x;
	qc->y = -q->y;
	qc->z = -q->z;
	return qc;
}


/********************************************************************************
 * FskQuaternionWXYZFromRowRotationMatrix3x3
 ********************************************************************************/

FskQuaternionWXYZ* FskQuaternionWXYZFromRowRotationMatrix3x3(const float R[3][3], FskQuaternionWXYZ *q) {
	float r, s;

    r = R[0][0] + R[1][1] + R[2][2];
	if (r >= 0) {
		s = sqrtf(r + 1.f);
		q->w = .5f * s;
		s    = .5f / s;
		q->x = (R[2][1] - R[1][2]) * s;
		q->y = (R[0][2] - R[2][0]) * s;
		q->z = (R[1][0] - R[0][1]) * s;
	} else if ((R[0][0] > R[1][1]) && (R[0][0] > R[2][2])) {
		s = sqrtf(R[0][0] - R[1][1] - R[2][2] + 1.f);
		q->x = .5f * s;
		s    = .5f / s;
		q->y = (R[1][0] + R[0][1]) * s;
		q->z = (R[0][2] + R[2][0]) * s;
		q->w = (R[2][1] - R[1][2]) * s;
    } else if (R[1][1] > R[2][2]) {
		s = sqrtf(R[1][1] - R[0][0] - R[2][2] + 1.f);
		q->y = .5f * s;
		s    = .5f / s;
		q->x = (R[1][0] + R[0][1]) * s;
		q->z = (R[2][1] + R[1][2]) * s;
		q->w = (R[0][2] - R[2][0]) * s;
    } else {
		s = sqrtf(R[2][2] - R[0][0] - R[1][1] + 1.f);
		q->z = .5f * s;
		s    = .5f / s;
		q->x = (R[0][2] + R[2][0]) * s;
		q->y = (R[2][1] + R[1][2]) * s;
		q->w = (R[1][0] - R[0][1]) * s;
    }

	return q;
}


/********************************************************************************
 * FskQuaternionWXYZFromNormalUp
 ********************************************************************************/

FskQuaternionWXYZ* FskQuaternionWXYZFromNormalUp(const FskVector3F *normal, const FskVector3F *up, FskQuaternionWXYZ *q) {
	float R[3][3];
	FskSNormalizeVector(&normal->x, R[2], 3);										/* z = normalized(normal) */
	FskSCrossProductVector3D(&up->x, R[2], R[0]);									/* x = up X z */
	FskSNormalizeVector(R[0], R[0], 3);												/* x = normalize(x) */
	FskSCrossProductVector3D(R[2], R[0], R[1]);										/* y = z X x */
	return FskQuaternionWXYZFromRowRotationMatrix3x3((const float (*)[3])R, q);
}


/********************************************************************************
 * FskQuaternionWXYZFromLookatUp
 ********************************************************************************/

FskQuaternionWXYZ* FskQuaternionWXYZFromLookatUp(const FskVector3F *lookat, const FskVector3F *up, FskQuaternionWXYZ *q) {
	FskVector3F into;	/* We convert from the vector facing out of the camera to the vector facing into it */
	return FskQuaternionWXYZFromNormalUp(Vector3FNegated(lookat, &into), up, q);
}


/********************************************************************************
 * SlerpUnitQuaternionWXYZ
 *		Spherical "linear" interpolation of unit quaternions with spins
 *		From "Quaternion Interpolation with Extra Spins", by Jack Morrison,
 *		Graphics Gems III.
 ********************************************************************************/

#define EPSILON 1.0E-6 			/* a tiny number */

FskQuaternionWXYZ*
FskSlerpUnitQuaternionWXYZ(
	float					alpha,		/* interpolation parameter (0 to 1) */
	const FskQuaternionWXYZ	*q0,		/* start unit quaternion */
	const FskQuaternionWXYZ	*q1,		/* end unit quaternion */
	int						spin,		/* number of extra spin rotations */
	FskQuaternionWXYZ		*q			/* output interpolated quaternion */
) {
	double	beta, theta, sinTheta, cosTheta, phi;
	int		q1Flip;

	/* cosine theta = dot product of q0 and q1 */
	cosTheta = q0->x * q1->x + q0->y * q1->y + q0->z * q1->z + q0->w * q1->w;

	/* if q1 is on opposite hemisphere from q0, use -q1 instead */
 	if (cosTheta < 0.0) {
		cosTheta = -cosTheta;
		q1Flip = 1;
	} else {
		q1Flip = 0;
	}

	if (cosTheta > 1.0 - EPSILON) {	/* if q1 is (within precision limits) the same as q0... */
		beta = 1.0 - alpha;	/* ... linearly interpolate q0 and q1, since we don't know which direction to spin. */
 	} else {				/* normal case */
 		theta = acos(cosTheta);
 		phi = theta + spin * M_PI;
 		sinTheta = sin(theta);
 		beta = sin(theta - alpha * phi) / sinTheta;
 		alpha = (float)(sin(alpha * phi) / sinTheta);
 	}

	if (q1Flip)
		alpha = -alpha;

	/* interpolate */
 	q->x = (float)(beta * q0->x + alpha * q1->x);
 	q->y = (float)(beta * q0->y + alpha * q1->y);
 	q->z = (float)(beta * q0->z + alpha * q1->z);
 	q->w = (float)(beta * q0->w + alpha * q1->w);

 	return q;
}


/********************************************************************************
 * FskCameraMake
 ********************************************************************************/

FskCamera FskCameraMake(SInt32 width, SInt32 height, const FskPoint3F *position, const FskQuaternionWXYZ *orientation, float focalLength, FskCamera camera) {
	float P[4][4], R[4][4], T[4][4], V[4][4];
	FskVector3F translation;

	/* Make projection matrix */
	P[0][0] = focalLength;		P[0][1] = 0.f;				P[0][2] =  0.f;		P[0][3] = 0.f;
	P[1][0] = 0.f;				P[1][1] = -focalLength;		P[1][2] =  0.f;		P[1][3] = 0.f;		/* Invert Y */
	P[2][0] = (1-width)*.5f;	P[2][1] = (1-height)*.5f;	P[2][2] = -1.f;		P[2][3] = 0.f;		/* Invert Z */
	P[3][0] = 0.f;				P[3][1] = 0.f;				P[3][2] =  0.f;		P[3][3] = 1.f;

	QuaternionWXYZToRowMatrix(orientation, R);
	TranslationToRowMatrix(Vector3FNegated(position, &translation), T);			/* This shifts the world relative to the camera */

	FskSLinearTransform(R[0], P[0], V[0],         4, 4, 4);
	FskSLinearTransform(T[0], V[0], camera->M[0], 4, 4, 4);
	return camera;
}


/********************************************************************************
 * FskEmbeddingMake
 ********************************************************************************/

FskEmbedding FskEmbeddingMake(FskConstRectangle srcRect, FskConstFloatRectangle dstRect, const FskPoint3F *position, const FskQuaternionWXYZ *orientation, FskEmbedding embedding)
{
	float P[4][4], R[4][4], T[4][4], V[4][4];
	FskQuaternionWXYZ qc;

	/* Make projection matrix */
	FskMemSet(&P[0][0], 0, sizeof(P));
	P[2][2] = P[3][3] = 1.f;
	if (srcRect) {
		float du = (float)(srcRect->width  - 1);
		float dv = (float)(srcRect->height - 1);
		if (dstRect) {
			P[0][0] = dstRect->width  / du;
			P[1][1] = dstRect->height / dv;
			P[3][0] = dstRect->x - dstRect->width  * srcRect->x / du;	/* Width  can be negative to invert x */
			P[3][1] = dstRect->y - dstRect->height * srcRect->y / dv;	/* Height can be negative to invert y */
		}
		else {
			P[0][0] = +1.f;
			P[1][1] = -1.f;					/* Invert Y, converting from Y-down in the image to Y-up in the world */
			P[3][0] = du * -0.5f;			/* Offset half the width */
			P[3][1] = dv * +0.5f;			/* Offset half the height, inverting Y */
		}
	}
	else {
		P[0][0] = +1.f;
		P[1][1] = +1.f;
		P[3][0] = 0.0f;
		P[3][1] = 0.0f;
	}

	QuaternionWXYZToRowMatrix(FskQuaternionWXYZConjugated(orientation, &qc), R);
	TranslationToRowMatrix(position, T);

	FskSLinearTransform(P[0], R[0], V[0],            4, 4, 4);
	FskSLinearTransform(V[0], T[0], embedding->M[0], 4, 4, 4);

	return embedding;
}


/********************************************************************************
 * FskProjectBitmap3D
 ********************************************************************************/

FskErr FskProjectBitmap3D(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	FskConstCamera					camera,
	FskConstEmbedding				embedding,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
) {
	float P[4][4], M[3][3];

	FskSLinearTransform(embedding->M[0], camera->M[0], P[0], 4, 4, 4);

	M[0][0] = P[0][0];	M[0][1] = P[0][1];	M[0][2] = P[0][2];
	M[1][0] = P[1][0];	M[1][1] = P[1][1];	M[1][2] = P[1][2];
	M[2][0] = P[3][0];	M[2][1] = P[3][1];	M[2][2] = P[3][2];

#if 0
	PrintMatrix("emd", embedding->M[0], 4, 4);
	PrintMatrix("cam",    camera->M[0], 4, 4);
	PrintMatrix("pec",            P[0], 4, 4);
	PrintMatrix("mec",            M[0], 3, 3);
#endif /* 0 */
	return FskProjectBitmap(srcBM, srcRect, dstBM, dstClip, (const float(*)[3])M, opColor, mode, modeParams);
}


/********************************************************************************
 * FskCameraDistances
 ********************************************************************************/

void FskCameraDistances(FskConstCamera camera, FskConstEmbedding embedding, UInt32 numPts, const FskPoint2F *pts, float *distances) {
	float P[4][4], toz[3];

	FskSLinearTransform(embedding->M[0], camera->M[0], P[0], 4, 4, 4);	// TODO: Do 3 dot products instead of 16
	toz[0] = P[0][2];
	toz[1] = P[1][2];
	toz[2] = P[3][2];
	FskSAffineTransform((const float*)pts, toz, distances, numPts, 2, 1);
}

