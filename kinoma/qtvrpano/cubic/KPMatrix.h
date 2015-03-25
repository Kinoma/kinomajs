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

#ifndef __KPMATRIX__
#define __KPMATRIX__

#include "Fsk.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								typedefs								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


		/* 2D points */
typedef struct KPSPoint2D	{ float  x, y;		} KPSPoint2D;
typedef struct KPDPoint2D	{ double x, y;		} KPDPoint2D;

		/* 2D vectors */
typedef struct KPSVector2D	{ float  x, y;		} KPSVector2D;
typedef struct KPDVector2D	{ double x, y;		} KPDVector2D;

		/* 3D points */
typedef struct KPSPoint3D	{ float x, y, z;	} KPSPoint3D;
typedef struct KPDPoint3D	{ double x, y, z;	} KPDPoint3D;

		/* 3D vectors */
typedef struct KPSVector3D	{ float  x, y, z;	} KPSVector3D;
typedef struct KPDVector3D	{ double x, y, z;	} KPDVector3D;

		/* 2x2 matrices */
typedef struct KPSMatrix2x2	{ float  M[2][2];	} KPSMatrix2x2;
typedef struct KPDMatrix2x2	{ double M[2][2];	} KPDMatrix2x2;

		/* 3x3 matrices */
typedef struct KPSMatrix3x3	{ float  M[3][3];	} KPSMatrix3x3;
typedef struct KPDMatrix3x3	{ double M[3][3];	} KPDMatrix3x3;





/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Functions								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * Functions for general matrices.
 ********************************************************************************/


		/* Conversion of precision */
FskAPI(void)	KPCopySingleToDoubleVector(const float *from, double *to, long n);											/* Vector single to double */
FskAPI(void)	KPCopyDoubleToSingleVector(const double *from, float *to, long n);											/* Vector double to single */
#define	KPCopySingleToDoubleMatrix(from, to, nRows, nCols)	KPCopySingleToDoubleVector((from), (to), (nRows)*(nCols))	/* Matrix single to double */
#define	KPCopyDoubleToSingleMatrix(from, to, nRows, nCols)	KPCopyDoubleToSingleVector((from), (to), (nRows)*(nCols))	/* Matrix double to single */

		/* Zero matrices and vectors */
FskAPI(void)	KPSZeroVector( float *v, long n);																		/* Single precision */
FskAPI(void)	KPDZeroVector(double *v, long n);																		/* Double precision */
#define	KPSZeroMatrix(M, nRows, nCols)					KPSZeroVector((M), (nRows) * (nCols))					/* Single precision */
#define	KPDZeroMatrix(M, nRows, nCols)					KPSZeroVector((M), (nRows) * (nCols))					/* Double precision */

		/* Identity matrices */
FskAPI(void)	KPSIdentityMatrix( float *M, long nRows, long nCols);
FskAPI(void)	KPDIdentityMatrix(double *M, long nRows, long nCols);

		/* Copy vectors and matrices */
FskAPI(void)	KPSCopyVector(const  float *fr,  float *to, long n);
FskAPI(void)	KPDCopyVector(const double *fr, double *to, long n);
#define	KPSCopyMatrix(fr, to, nRows, nCols)				KPSCopyVector((fr), (to), (nRows) * (nCols))			/* Single precision */
#define	KPDCopyMatrix(fr, to, nRows, nCols)				KPDCopyVector((fr), (to), (nRows) * (nCols))			/* Double precision */

		/* Scale vectors and matrices */
FskAPI(void)	KPSScaleVector( float s, const  float *fr,  float *to, long n);											/* Single precision */
FskAPI(void)	KPDScaleVector(double s, const double *fr, double *to, long n);											/* Double precision */
#define	KPSScaleMatrix(s, fr, to, nRows, nCols)			KPSScaleVector((s), (fr), (to), (nRows) * (nCols))		/* Single precision */
#define	KPDScaleMatrix(s, fr, to, nRows, nCols)			KPDScaleVector((s), (fr), (to), (nRows) * (nCols))		/* Double precision */

		/* L2 Norm of vectors */
FskAPI(float)	KPSNormVector(const  float *v, long n);
FskAPI(double)	KPDNormVector(const double *v, long n);

		/* L2 Normalization of vectors */
FskAPI(float)	KPSNormalizeVector(const  float *fr,  float *to, long n);
FskAPI(double)	KPDNormalizeVector(const double *fr, double *to, long n);

		/* Dot product */
FskAPI(float)	KPSDotProductVector(const  float *v0, const  float *v1, long n);
FskAPI(double)	KPDDotProductVector(const double *v0, const double *v1, long n);

		/* Cross Product */
FskAPI(void)	KPSCrossProductVector3D(const  float *L, const  float *R,  float *P);
FskAPI(void)	KPDCrossProductVector3D(const double *L, const double *R, double *P);

		/* Linear transformations (does not not work in-place) */
FskAPI(void)	KPSLinearTransform(const float  *L, const float  *R, float  *P, long nRows, long lCol, long rCol);		/* Single precision */
FskAPI(void)	KPDLinearTransform(const double *L, const double *R, double *P, long nRows, long lCol, long rCol);		/* Double precision */

		/* Affine transformations (does not not work in-place) */
FskAPI(void)	KPSAffineTransform(const float	*L, const float  *R, float  *P, long nRows, long lCols, long rCols);	/* Single precision */
FskAPI(void)	KPDAffineTransform(const double	*L, const double *R, double *P, long nRows, long lCols, long rCols);	/* Double precision */

		/* Transpose a square matrix in-place */
FskAPI(void)	KPSTransposeSquareMatrixInPlace(float  *M, long n);														/* Single precision */
FskAPI(void)	KPDTransposeSquareMatrixInPlace(double *M, long n);														/* Double precision */

		/* LU Decomposition and solution (does not work in-place) */
FskAPI(long)	KPSLUDecompose(const float  *a, double *lu, long n);													/* Single precision */
FskAPI(long)	KPDLUDecompose(const double *a, double *lu, long n);													/* Double precision */
FskAPI(void)	KPSLUSolve(const double *lu, const float  *b, float  *x, long n);										/* Single precision */
FskAPI(void)	KPDLUSolve(const double *lu, const double *b, double *x, long n);										/* Double precision */

		/* Invert matrix (this works in-place) */
FskAPI(long)	KPSInvertMatrix(const float  *M, long nRows, long n, float  *Minv);										/* Single precision */
FskAPI(long)	KPDInvertMatrix(const double *M, long nRows, long n, double *Minv);										/* Double precision */

		/* L-infinity norm of a matrix, induced by ROW vectors */
FskAPI(float)	KPSNormMatrix(const float  *M, long height, long width);												/* Single precision */
FskAPI(double)	KPDNormMatrix(const double *M, long height, long width);												/* Double precision */

		/* Determinant of a square matrix */
FskAPI(float)	KPSDeterminantMatrix(const  float *a, long n);
FskAPI(double)	KPDDeterminantMatrix(const double *a, long n);


/********************************************************************************
 * Specializations for 3x3 matrices.
 ********************************************************************************/

		/* Adjoint of a 3x3 matrix (this works in-place) */
FskAPI(void)	KPSAdjointMatrix3x3(const float  *Min, float  *Mout);													/* Single precision */
FskAPI(void)	KPDAdjointMatrix3x3(const double *Min, double *Mout);													/* Double precision */

		/* Rotate about the X axis */
FskAPI(void)	KPSSetRotationX3x3(float  rad, float  *M);																/* Single precision */
FskAPI(void)	KPDSetRotationX3x3(double rad, double *M);																/* Double precision */

		/* Rotate about the Y axis */
FskAPI(void)	KPSSetRotationY3x3(float  rad, float  *M);																/* Single precision */
FskAPI(void)	KPDSetRotationY3x3(double rad, double *M);																/* Double precision */

		/* Rotate aboutthe Z axis */
FskAPI(void)	KPSSetRotationZ3x3(float  rad, float  *M);																/* Single precision */
FskAPI(void)	KPDSetRotationZ3x3(double rad, double *M);																/* Double precision */

FskAPI(void)	KPSSetRotationZAboutCenter3x3(float  rad, const float  *center, float  *M);								/* Single precision */
FskAPI(void)	KPDSetRotationZAboutCenter3x3(double rad, const double *center, double *M);								/* Double precision */

FskAPI(void)	KPSSetScaleRotationZFromCenterToCenter3x3(																/* Single precision */
			float			scale,		/* Isotropic scale factor */
			float			radians,	/* Rotation angle in radians */
			const float		*frCenter,	/* Rotate the image about this center */
			const float 	*toCenter,	/* Translate the frCenter to the toCenter */
			float			*M			/* The resultant matrix */
		);

FskAPI(void)	KPDSetScaleRotationZFromCenterToCenter3x3(																/* Double precision */
			double			scale,		/* Isotropic scale factor */
			double			radians,	/* Rotation angle in radians */
			const double	*frCenter,	/* Rotate the image about this center */
			const double 	*toCenter,	/* Translate the frCenter to the toCenter */
			double			*M			/* The resultant matrix */
		);

		/* Rotation matrix from pan, tilt, and roll */
FskAPI(void)	KPSSetRotationPanTiltRoll3x3(float  pan, float  tilt, float  roll, float  *M);							/* Single precision */
FskAPI(void)	KPDSetRotationPanTiltRoll3x3(double pan, double tilt, double roll, double *M);							/* Double precision */

		/* Rotation matrix from pan and tilt */
FskAPI(void)	KPSSetRotationPanTilt3x3(float  pan, float  tilt, float  *M);											/* Single precision */
FskAPI(void)	KPDSetRotationPanTilt3x3(double pan, double tilt, double *M);											/* Double precision */

		/* Rotation matrix from normalized axis, sine and cosine */
FskAPI(void)	KPSSetRotationAxisSineCosine3x3(const float  *a, float  s, float  c, float  *M);						/* Single precision */
FskAPI(void)	KPDSetRotationAxisSineCosine3x3(const double *a, double s, double c, double *M);						/* Double precision */

		/* Rotation matrix from normalized axis and angle */
FskAPI(void)	KPSAxisAngleToMatrix3x3(const float  *a, float  radians, float  *M);									/* Single precision */
FskAPI(void)	KPDAxisAngleToMatrix3x3(const double *a, double radians, double *M);									/* Double precision */

		/* Rotation matrix from normalized WXYZ quaternion */
FskAPI(void)	KPSQuaternionWXYZToMatrix3x3(const float  *q, float  *M);												/* Single precision */
FskAPI(void)	KPDQuaternionWXYZToMatrix3x3(const double *q, double *M);												/* Double precision */


/********************************************************************************
 * Immersion for 3x3 matrices.
 ********************************************************************************/

		/* Immersion matrix from matrix and focal length */
FskAPI(void)	KPSImmerseUsingMatrix3x3(																				/* Single precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			const float		*R,				/* Rotation matrix */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	KPDImmerseUsingMatrix3x3(																				/* Double precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			const double	*R,				/* Rotation matrix */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Immersion matrix from quaternion and focal length */
FskAPI(void)	KPSImmerseUsingQuaternion3x3(																			/* Single precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			const float		*quat,			/* Orientation is specified using a normalized WXYZ quaternion */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	KPDImmerseUsingQuaternion3x3(																			/* Double precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			const double	*quat,			/* Orientation is specified using a normalized WXYZ quaternion */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Immersion matrix from pan, tilt, roll and focal length */
FskAPI(void)	KPSImmerseUsingPanTiltRoll3x3(																			/* Single precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			float			pan,			/* Rotation about the Y axis */
			float			tilt,			/* Rotation about the X axis */
			float			roll,			/* Rotation about the Z axis */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	KPDImmerseUsingPanTiltRoll3x3(																			/* Double precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			double			pan,			/* Rotation about the Y axis */
			double			tilt,			/* Rotation about the X axis */
			double			roll,			/* Rotation about the Z axis */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Immersion matrix from pan, tilt, roll and focal length */
FskAPI(void)	KPSImmerseUsingPanTilt3x3(																				/* Single precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			float			pan,			/* Rotation about the Y axis */
			float			tilt,			/* Rotation about the X axis */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	KPDImmerseUsingPanTilt3x3(																				/* Double precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			double			pan,			/* Rotation about the Y axis */
			double			tilt,			/* Rotation about the X axis */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);


/********************************************************************************
 * Projection for 3x3 matrices.
 ********************************************************************************/

		/* Projection matrix from rotation matrix and focal length */
FskAPI(void)	KPSProjectUsingMatrix3x3(																				/* Single precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			const float		*R,				/* Rotation matrix */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	KPDProjectUsingMatrix3x3(																				/* Double precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			const double	*R,				/* Rotation matrix */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Projection matrix from quaternion and focal length */
FskAPI(void)	KPSProjectUsingQuaternion3x3(																			/* Single precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			const float		*quat,			/* Orientation is specified using a normalized WXYZ quaternion */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	KPDProjectUsingQuaternion3x3(																			/* Double precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			const double	*quat,			/* Orientation is specified using a normalized WXYZ quaternion */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Projection matrix from pan, tilt, roll and focal length */
FskAPI(void)	KPSProjectUsingPanTiltRoll3x3(																			/* Single precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			float			pan,			/* Rotation about the Y axis */
			float			tilt,			/* Rotation about the X axis */
			float			roll,			/* Rotation about the Z axis */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	KPDProjectUsingPanTiltRoll3x3(																			/* Double precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			double			pan,			/* Rotation about the Y axis */
			double			tilt,			/* Rotation about the X axis */
			double			roll,			/* Rotation about the Z axis */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Projection matrix from pan and tilt and focal length */
FskAPI(void)	KPSProjectUsingPanTilt3x3(																				/* Single precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			float			pan,			/* Rotation about the Y axis */
			float			tilt,			/* Rotation about the X axis */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	KPDProjectUsingPanTilt3x3(																				/* Double precision */
			long			width,			/* Width of the image */
			long			height,			/* Height of the image */
			double			pan,			/* Rotation about the Y axis */
			double			tilt,			/* Rotation about the X axis */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Compute the projective coordinate frame mapping the unit square to the given 4 points, returning 1 if successful */
FskAPI(long)	KPSProjectiveFrameFromPoints3x3(const  float *pts,  float *M);											/* Single precision */
FskAPI(long)	KPDProjectiveFrameFromPoints3x3(const double *pts, double *M);											/* Double precision */

		/* Compute the projective transformation from the given 4 point correspondences, returning 1 if successful */
FskAPI(long)	KPSProjectionFromCorrespondences3x3(const  float *fr, const  float *to,  float *M);						/* Single precision */
FskAPI(long)	KPDProjectionFromCorrespondences3x3(const double *fr, const double *to, double *M);						/* Double precision */

		/* Compute the projective transformation from the given 4 point correspondences, returning 1 if successful */
FskAPI(long)	KPSDirectProjectionFromCorrespondences3x3(const  float *fr, const  float *to,  float *M);				/* Single precision */
FskAPI(long)	KPDDirectProjectionFromCorrespondences3x3(const double *fr, const double *to, double *M);				/* Double precision */


/********************************************************************************
 * Specializations for 4x4 matrices.
 ********************************************************************************/

		/* Standard embedding of a 3x3 matrix into a 4x4 matrix */
FskAPI(void)	KPSEmbed3x3In4x4Matrix(const float  *M3, float  *M4);													/* Single precision */
FskAPI(void)	KPDEmbed3x3In4x4Matrix(const double *M3, double *M4);													/* Double precision */

		/* Standard embedding of a 3x3 matrix into a 4x4 matrix, with transposition */
FskAPI(void)	KPSEmbedTransposed3x3In4x4Matrix(const float  *M3, float  *M4);											/* Single precision */
FskAPI(void)	KPDEmbedTransposed3x3In4x4Matrix(const double *M3, double *M4);											/* Double precision */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPMATRIX__ */


