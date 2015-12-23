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
	\file	FskPerspective.h
	\brief	Textured polygons in perspective.
*/

#ifndef __FSKPERSPECTIVE__
#define __FSKPERSPECTIVE__

#ifndef __FSKFIXEDMATH__
# include "FskFixedMath.h"
#endif /* __FSKFIXEDMATH__ */

#ifndef __FSKBITMAP__
# include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Transform the image by the given perspective transformation.
 *	\param[in]	srcBM			the source to be transformed.
 *	\param[in]	srcRect			The subrect of the source to be transformed. If NULL, implies the full source.
 *	\param[in]	dstBM			the destination bitmap.
 *	\param[in]	dstClip			the destination clipping rectangle (may be NULL).
 *	\param[in]	M				the perspective matrix.
 *	\param[in]	opColor			Operation color used if needed for the given transfer mode.
 *	\param[in]	mode			Transfer mode, incorporating quality.
 *	\param[in]	modeParams		We get blend level from here.
 *	\return		kFskErrNone					if the rendering completed successively.
 *	\return		kFskErrNothingRendered		if nothing was rendered due to clipping.
 *	\return		kFskErrUnsupportedPixelType	if either the source or destination pixel format cannot be accommodated.
 **/

FskAPI(FskErr)
FskProjectBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	const float						M[3][3],
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
);


/** Embedding matrix. */
typedef       struct FskEmbeddingRecord { float M[4][4]; }	*FskEmbedding,		FskEmbeddingRecord;
typedef const struct FskEmbeddingRecord						*FskConstEmbedding;

/** Camera matrix. */
typedef       struct FskCameraRecord	{ float M[4][4]; }	*FskCamera,			FskCameraRecord;
typedef const struct FskCameraRecord						*FskConstCamera;

/** Real-world rectangle */
typedef       struct FskFloatRectangleRecord	{ float x, y, width, height; }	*FskFloatRectangle,		FskFloatRectangleRecord;
typedef const struct FskFloatRectangleRecord									*FskConstFloatRectangle;

/** Quaternion. */
typedef struct FskQuaternionWXYZ	{ float w, x, y, z;	}	FskQuaternionWXYZ;

/** 3D Vector and point. */
typedef struct FskVector3F			{ float x, y, z;	}	FskVector3F, FskPoint3F;

/** 2D Vector and point. */
typedef struct FskVector2F			{ float x, y;	}	FskVector2F, FskPoint2F;

/** Set the elements of a quaternion. */
#define FskQuaternionWXYZSet(q, W, X, Y, Z)	do { (q)->w = W; (q)->x = X; (q)->y = Y; (q)->z = Z; } while(0)

/** Set the elements of a 3D point or vector, or floating point rectangle. */
#define FskPoint2FSet(v, X, Y)				do { (v)->x = X; (v)->y = Y;             } while(0)
#define FskPoint3FSet(v, X, Y, Z)			do { (v)->x = X; (v)->y = Y; (v)->z = Z; } while(0)
#define FskVector3FSet(v, X, Y, Z)			do { (v)->x = X; (v)->y = Y; (v)->z = Z; } while(0)
#define FskFloatRectangleSet(R, X, Y, W, H)	do { (R)->x = X; (R)->y = Y; (R)->width = W; (R)->height = H; } while(0)

/** Compute the conjugate of the given quaternion.
 *	qc->w = q->w; qc->x = -q->x; qc->y = -q->y; qc->z = -q->z;
 *	This is equivalent to a unit quaternion inverse.
 *	This works in-place.
 *	\param[in]	q	a pointer to the given quaternion.
 *	\param[out]	qc	a pointer to the conjugated quaternion. This can be the same as q.
 *	\return			a pointer to the conjugated quaternion, qc.
 */
FskAPI(FskQuaternionWXYZ*)	FskQuaternionWXYZConjugated(const FskQuaternionWXYZ *q, FskQuaternionWXYZ *qc);

/** Multiply two quaternions.
 *	Can be used in-place.
 *	\param[in]	q1	the  left quaternion.
 *	\param[in]	q2	the right quaternion.
 *	\param[out]	qf	the product, q1 * q2, of the two quaternions.
 *	\return			the quaternion product, qf.
 */
FskAPI(FskQuaternionWXYZ*)	FskMultiplyQuaternionWXYZ(const FskQuaternionWXYZ *q1, const FskQuaternionWXYZ *q2, FskQuaternionWXYZ *qf);

/** Euler order macros.
 * sg = { 0, 1 }, such that E3 * E2 = (-1)^sg * E1
 *		is a function of the order of the rotations as well as the handedness of the system.
 * e1 = { 0, 1, 2 } for { x, y, z }
 * e2 = { 0, 1, 2 } for { x, y, z }
 * e3 = { 0, 1, 2 } for { x, y, z }
 * { e1, e2, e3 } must be distinct.
 */
#define FSK_EULER_SG_POS	12							/**< Position of the signature E3 * E2 = (-1)^sg * E1 */
#define FSK_EULER_E1_POS	8							/**< Position to store the first  rotation axis { 0, 1, 2 } for { X, Y, Z }. */
#define FSK_EULER_E2_POS	4							/**< Position to store the second rotation axis { 0, 1, 2 } for { X, Y, Z }. */
#define FSK_EULER_E3_POS	0							/**< Position to store the third  rotation axis { 0, 1, 2 } for { X, Y, Z }. */
#define FSK_EULER_SG_MASK	(1 << FSK_EULER_SG_POS)		/**< Mask to extract the signature. */
#define FSK_EULER_E0_MASK	3							/**< Mask to extract an axis once it has been shifted into position. */
/** Construct the Euler order argument.
 *	Rotating { roll, tilt, pan , i.e. Z-X-Y  in a right-handed system is invoked by FSK_EULER_ORDER(1, 2, 0, 1)
 *	Rotating { roll, tilt, pan , i.e. Z-X-Y  in a  left-handed system is invoked by FSK_EULER_ORDER(0, 2, 0, 1)
 *	\param[in]		sg	The signature, in { 0, 1 }, such that E3 * E2 = (-1)^sg * E1.
 *	\param[in]		e1	The first  axis of rotation, { 0, 1, 2 }, i.e. { X, Y, Z }.
 *	\param[in]		e2	The second axis of rotation, { 0, 1, 2 }, i.e. { X, Y, Z }.
 *	\param[in]		e3	The third  axis of rotation, { 0, 1, 2 }, i.e. { X, Y, Z }.
 *	\return			order	The appropriately constructed "order" argument for FskEulerToQuaternionWXYZ() and FskQuaternionWXYZToEuler().
 */
#define FSK_EULER_ORDER(sg, e1, e2, e3)	(((sg) << FSK_EULER_SG_POS) | ((e1) << FSK_EULER_E1_POS) | ((e2) << FSK_EULER_E2_POS) | ((e3) << FSK_EULER_E3_POS))

/**	Set a quaternion from Euler angles.
 *	\param[in]	euler	Three Euler angles { Ax, Ay, Az }.
 *	\param[in]	order	The order of the rotations, constructed with FSK_EULER_ORDER().
 *	\param[out]	q		The resultant quaternion.
 *	\result				The quaternion q.
 */
FskAPI(FskQuaternionWXYZ*)	FskEulerToQuaternionWXYZ(const float euler[3], int order, FskQuaternionWXYZ *q);

/**	Set Euler angles from a quaternion.
 *	\param[out]	q		The quaternion.
 *	\param[in]	order	The order of the rotations, constructed with FSK_EULER_ORDER().
 *	\param[in]	euler	The resultant three Euler angles { Ax, Ay, Az }.
 *	\result				The Euler angles euler.
 */
FskAPI(float*)				FskQuaternionWXYZToEuler(const FskQuaternionWXYZ *q, int order, float euler[3]);

/**	Set a quaternion from pan, tilt and roll.
 *	\param[in]	pan		The pan  angle (around a  vertical axis),  in radians.
 *	\param[in]	tilt	The tilt angle (around a horizontal axis),  in radians.
 *	\param[in]	roll	The roll angle (around the view direction), in radians.
 *	\param[out]	q		The resultant quaternion.
 *	\note				This produces a relative rotation.
 *	\return				The resultant quaternion, q.
 */
FskAPI(FskQuaternionWXYZ*)	FskPanTiltRollToQuaternionWXYZ(float pan, float tilt, float roll, FskQuaternionWXYZ *q);

/**	Set {pan, tilt, roll} from a quaternion.
 *	\param[in]	q			The quaternion.
*	\param[in]	panTiltRoll	The 3-vector where the pan, tilt, and roll are to be stored.
 *	\return					The panTiltRoll vector.
 */
FskAPI(float*)				FskQuaternionWXYZToPanTiltRoll(const FskQuaternionWXYZ *q, float *panTiltRoll);

/**	Set a quaternion from a row rotation matrix.
*	\param[in]	R		The orthogonal rotation matrix, suitable for transforming row vectors.
 *	\param[out]	q		The resultant quaternion.
 *	\return				The resultant quaternion, q.
 */
FskAPI(FskQuaternionWXYZ*)	FskQuaternionWXYZFromRowRotationMatrix3x3(const float R[3][3], FskQuaternionWXYZ *q);

/**	Set a quaternion for a planar object from a "normal" and an "up" vector.
 *	\param[in]	normal	The normal vector. This does not need to be a unit vector.
 *	\param[in]	up		The   up   vector. This does not need to be a unit vector, nor does it need to be orthogonal to the normal vector,
 *						but it cannot be parallel to the normal vector.
 *	\param[out]	q		The resultant quaternion.
 *	\return				the input quaternion, q.
 */
FskAPI(FskQuaternionWXYZ*)	FskQuaternionWXYZFromNormalUp(const FskVector3F *normal, const FskVector3F *up, FskQuaternionWXYZ *q);

/**	Set a quaternion for a camera from a "lookat" and an "up" vector.
 *	\param[in]	lookat	The lookat vector. This does not need to be a unit vector.
 *	\param[in]	up		The   up   vector. This does not need to be a unit vector, nor does it need to be orthogonal to the lookat vector,
 *						but it cannot be parallel to the lookat vector.
 *	\param[out]	q		The resultant quaternion.
 *	\return				The resultant quaternion, q.
 */
FskAPI(FskQuaternionWXYZ*)	FskQuaternionWXYZFromLookatUp(const FskVector3F *lookat, const FskVector3F *up, FskQuaternionWXYZ *q);

/**	Interpolation of a quaternion on a sphere.
 *	\param[in]	alpha	The interpolation parameter, between 0 and 1, inclusive.
 *	\param[in]	q0		The orientation corresponding to alpha=0.
 *	\param[in]	q1		The orientation corresponding to alpha=1.
 *	\param[in]	spin	Any extra spin (spin * 360 degrees) between the starting and ending orientations.
 *	\param[out]	q		The resultant quaternion.
 *	\return				The resultant quaternion, q.
 */
FskAPI(FskQuaternionWXYZ*)	FskSlerpUnitQuaternionWXYZ(
	float					alpha,		/* interpolation parameter (0 to 1) */
	const FskQuaternionWXYZ	*q0,		/* start unit quaternion */
	const FskQuaternionWXYZ	*q1,		/* end unit quaternion */
	int						spin,		/* number of extra spin rotations */
	FskQuaternionWXYZ		*q			/* output interpolated quaternion */
);

/** Create the embedding matrix for an image.
 *	This maps the srcRect in pixel space onto the dstRect in the world-space 2D frame established by the position and orientation.
 *	\param[in]	srcRect		The rect of the source image, in pixels. Typically, srcRect is not NULL, and all elements are positive.
 *	\param[in]	dstRect		The rect in the billboard, to which the image is to be stretched.
 *							This is given in world units, in the 2D coordinate frame formed by the position and orientation.
 *							Elements can be positive or negative. Negative width or height will reverse the the x or y axis, respectively.
 *	\param[in]	position	The 3D position of the billboard, where the image is to be placed, in world units.
 *	\param[in]	orientation	The orientation of the image.
 *	\param[in]	embedding	The resultant embedding matrix.
 *	\return					The resultant embedding matrix.
 */
FskAPI(FskEmbedding)		FskEmbeddingMake(FskConstRectangle srcRect, FskConstFloatRectangle dstRect, const FskPoint3F *position, const FskQuaternionWXYZ *orientation, FskEmbedding embedding);

/** Create a camera matrix.
 *	\param[in]	width		The width  of the image captured by the camera, in pixels.
 *	\param[in]	height		The height of the image captured by the camera, in pixels.
 *	\param[in]	position	The 3D position of the camera, in world units.
 *	\param[in]	orientation	The orientation of the image.
 *	\param[in]	focalLength	The focal length of the camera, in pixels; e.g. to capture 90 degrees horizontally, set equal to the width.
 *	\param[in]	camera		The resultant camera matrix.
 *	\return					The resultant camera matrix.
 */
FskAPI(FskCamera)			FskCameraMake(SInt32 width, SInt32 height, const FskPoint3F *position, const FskQuaternionWXYZ *orientation, float focalLength, FskCamera camera);

/** Project a bitmap using a 3D camera and a 3D embedding.
 *	\param[in]	srcBM		The source bitmap.
 *	\param[in]	srcRect		A portion of the srcBM to be projected. This can be NULL, which implies the whole srcBM.
 *	\param[in]	dstBM		The destination bitmap.
 *	\param[in]	dstClip		The destination clip. This may be NULL, which clips to dstBM->bounds.
 *	\param[in]	camera		The camera matrix.
 *	\param[in]	embedding	The embedding matrix.
 *	\param[in]	opColor		The opColor used with kFskGraphicsModeColorize. Can be NULL, which implies opaque white.
 *	\param[in]	mode		The graphics mode, one of kFskGraphicsModeCopy, kFskGraphicsModeAlpha, kFskGraphicsModeColorize,
 *							perhaps ORed with kFskGraphicsModeBilinear.
 *	\param[in]	modeParams	The mode parameters (i.e. blendLevel). Can be NULL, which implies blendLevel=255.
 *	\return		kFskErrNone	if successful.
 */
FskAPI(FskErr) FskProjectBitmap3D(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	FskConstCamera					camera,
	FskConstEmbedding				embedding,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
);


/** Compute Z distances from a camera to a set of points on an image in an embedding plane.
 *	This is useful to sort for the painter's algorithm.
 *	\param[in]	camera		The camera matrix.
 *	\param[in]	embedding	The embedding matrix.
 *	\param[in]	numPts		The number of points.
 *	\param[in]	pts			The list of numPts points on the image.
 *	\param[out]	distances	The list of numPts distances.
 */
FskAPI(void) FskCameraDistances(FskConstCamera camera, FskConstEmbedding embedding, UInt32 numPts, const FskPoint2F *pts, float *distances);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKPERSPECTIVE__ */


