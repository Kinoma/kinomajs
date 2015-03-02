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
/**
	\file	FskMatrix.h
	\brief	Floating-point matrix operations.
*/

#ifndef __FSKMATRIX__
#define __FSKMATRIX__

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
typedef struct FskSPoint2D	{ float  x, y;		} FskSPoint2D;
typedef struct FskDPoint2D	{ double x, y;		} FskDPoint2D;

		/* 2D vectors */
typedef struct FskSVector2D	{ float  x, y;		} FskSVector2D;
typedef struct FskDVector2D	{ double x, y;		} FskDVector2D;

		/* 3D points */
typedef struct FskSPoint3D	{ float x, y, z;	} FskSPoint3D;
typedef struct FskDPoint3D	{ double x, y, z;	} FskDPoint3D;

		/* 3D vectors */
typedef struct FskSVector3D	{ float  x, y, z;	} FskSVector3D;
typedef struct FskDVector3D	{ double x, y, z;	} FskDVector3D;

		/* 2x2 matrices */
typedef struct FskSMatrix2x2	{ float  M[2][2];	} FskSMatrix2x2;
typedef struct FskDMatrix2x2	{ double M[2][2];	} FskDMatrix2x2;

		/* 3x2 matrices */
typedef struct FskSMatrix3x2	{ float  M[3][2];	} FskSMatrix3x2;
typedef struct FskDMatrix3x2	{ double M[3][2];	} FskDMatrix3x2;

		/* 3x3 matrices */
typedef struct FskSMatrix3x3	{ float  M[3][3];	} FskSMatrix3x3;
typedef struct FskDMatrix3x3	{ double M[3][3];	} FskDMatrix3x3;





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


		/* Conversion of precision -- these work in-place */
FskAPI(void)	FskCopySingleToDoubleVector(const float *from, double *to, int n);												/* Vector single to double */
FskAPI(void)	FskCopyDoubleToSingleVector(const double *from, float *to, int n);												/* Vector double to single */
#define			FskCopySingleToDoubleMatrix(from, to, nRows, nCols)	FskCopySingleToDoubleVector((from), (to), (nRows)*(nCols))	/* Matrix single to double */
#define			FskCopyDoubleToSingleMatrix(from, to, nRows, nCols)	FskCopyDoubleToSingleVector((from), (to), (nRows)*(nCols))	/* Matrix double to single */

		/* Zero matrices and vectors */
FskAPI(void)	FskSZeroVector( float *v, int n);																		/* Single precision */
FskAPI(void)	FskDZeroVector(double *v, int n);																		/* Double precision */
#define			FskSZeroMatrix(M, nRows, nCols)					FskSZeroVector((M), (nRows) * (nCols))					/* Single precision */
#define			FskDZeroMatrix(M, nRows, nCols)					FskSZeroVector((M), (nRows) * (nCols))					/* Double precision */

		/* Identity matrices */
FskAPI(void)	FskSIdentityMatrix( float *M, int nRows, int nCols);
FskAPI(void)	FskDIdentityMatrix(double *M, int nRows, int nCols);

		/* Copy vectors and matrices */
FskAPI(void)	FskSCopyVector(const  float *fr,  float *to, int n);
FskAPI(void)	FskDCopyVector(const double *fr, double *to, int n);
#define			FskSCopyMatrix(fr, to, nRows, nCols)			FskSCopyVector((fr), (to), (nRows) * (nCols))			/* Single precision */
#define			FskDCopyMatrix(fr, to, nRows, nCols)			FskDCopyVector((fr), (to), (nRows) * (nCols))			/* Double precision */

		/* Scale vectors and matrices */
FskAPI(void)	FskSScaleVector( float s, const  float *fr,  float *to, int n);											/* Single precision */
FskAPI(void)	FskDScaleVector(double s, const double *fr, double *to, int n);											/* Double precision */
#define			FskSScaleMatrix(s, fr, to, nRows, nCols)		FskSScaleVector((s), (fr), (to), (nRows) * (nCols))		/* Single precision */
#define			FskDScaleMatrix(s, fr, to, nRows, nCols)		FskDScaleVector((s), (fr), (to), (nRows) * (nCols))		/* Double precision */

		/* L2 Norm of vectors */
FskAPI(float)	FskSNormVector(const  float *v, int n);
FskAPI(double)	FskDNormVector(const double *v, int n);

		/* L2 Normalization of vectors */
FskAPI(float)	FskSNormalizeVector(const  float *fr,  float *to, int n);
FskAPI(double)	FskDNormalizeVector(const double *fr, double *to, int n);

		/* Dot product */
FskAPI(float)	FskSDotProductVector(const  float *v0, const  float *v1, int n);
FskAPI(double)	FskDDotProductVector(const double *v0, const double *v1, int n);

		/* Cross Product */
FskAPI(void)	FskSCrossProductVector3D(const  float *L, const  float *R,  float *P);
FskAPI(void)	FskDCrossProductVector3D(const double *L, const double *R, double *P);

		/* Linear transformations (does not not work in-place) */
FskAPI(void)	FskSLinearTransform(const float  *L, const float  *R, float  *P, int nRows, int lCol, int rCol);		/* Single precision */
FskAPI(void)	FskDLinearTransform(const double *L, const double *R, double *P, int nRows, int lCol, int rCol);		/* Double precision */

		/* Affine transformations (does not not work in-place) */
FskAPI(void)	FskSAffineTransform(const float	 *L, const float  *R, float  *P, int nRows, int lCols, int rCols);		/* Single precision */
FskAPI(void)	FskDAffineTransform(const double *L, const double *R, double *P, int nRows, int lCols, int rCols);		/* Double precision */

		/* Transpose a square matrix in-place */
FskAPI(void)	FskSTransposeSquareMatrixInPlace(float  *M, int n);														/* Single precision */
FskAPI(void)	FskDTransposeSquareMatrixInPlace(double *M, int n);														/* Double precision */

		/* LU Decomposition and solution (does not work in-place) */
FskAPI(int)	FskSLUDecompose(const float  *a, double *lu, int n);														/* Single precision */
FskAPI(int)	FskDLUDecompose(const double *a, double *lu, int n);														/* Double precision */
FskAPI(void)	FskSLUSolve(const double *lu, const float  *b, float  *x, int n);										/* Single precision */
FskAPI(void)	FskDLUSolve(const double *lu, const double *b, double *x, int n);										/* Double precision */

		/* Invert matrix (this works in-place) */
FskAPI(int)	FskSInvertMatrix(const float  *M, int nRows, int n, float  *Minv);											/* Single precision */
FskAPI(int)	FskDInvertMatrix(const double *M, int nRows, int n, double *Minv);											/* Double precision */

		/* L-infinity norm of a matrix, induced by ROW vectors */
FskAPI(float)	FskSNormMatrix(const float  *M, int height, int width);													/* Single precision */
FskAPI(double)	FskDNormMatrix(const double *M, int height, int width);													/* Double precision */

		/* Determinant of a square matrix */
FskAPI(float)	FskSDeterminantMatrix(const  float *a, int n);
FskAPI(double)	FskDDeterminantMatrix(const double *a, int n);


/********************************************************************************
 * Specializations for 3x3 matrices.
 ********************************************************************************/

		/* Adjoint of a 3x3 matrix (this works in-place) */
FskAPI(void)	FskSAdjointMatrix3x3(const float  *Min, float  *Mout);													/* Single precision */
FskAPI(void)	FskDAdjointMatrix3x3(const double *Min, double *Mout);													/* Double precision */

		/* Rotate about the X axis */
FskAPI(void)	FskSSetRotationX3x3(float  rad, float  *M);																/* Single precision */
FskAPI(void)	FskDSetRotationX3x3(double rad, double *M);																/* Double precision */

		/* Rotate about the Y axis */
FskAPI(void)	FskSSetRotationY3x3(float  rad, float  *M);																/* Single precision */
FskAPI(void)	FskDSetRotationY3x3(double rad, double *M);																/* Double precision */

		/* Rotate aboutthe Z axis */
FskAPI(void)	FskSSetRotationZ3x3(float  rad, float  *M);																/* Single precision */
FskAPI(void)	FskDSetRotationZ3x3(double rad, double *M);																/* Double precision */

FskAPI(void)	FskSSetRotationZAboutCenter3x3(float  rad, const float  *center, float  *M);							/* Single precision */
FskAPI(void)	FskDSetRotationZAboutCenter3x3(double rad, const double *center, double *M);							/* Double precision */

FskAPI(void)	FskSSetScaleRotationZFromCenterToCenter3x3(																/* Single precision */
			float			scale,		/* Isotropic scale factor */
			float			radians,	/* Rotation angle in radians */
			const float		*frCenter,	/* Rotate the image about this center */
			const float 	*toCenter,	/* Translate the frCenter to the toCenter */
			float			*M			/* The resultant matrix */
		);

FskAPI(void)	FskDSetScaleRotationZFromCenterToCenter3x3(																/* Double precision */
			double			scale,		/* Isotropic scale factor */
			double			radians,	/* Rotation angle in radians */
			const double	*frCenter,	/* Rotate the image about this center */
			const double 	*toCenter,	/* Translate the frCenter to the toCenter */
			double			*M			/* The resultant matrix */
		);

		/* Rotation matrix from pan, tilt, and roll */
FskAPI(void)	FskSSetRotationPanTiltRoll3x3(float  pan, float  tilt, float  roll, float  *M);							/* Single precision */
FskAPI(void)	FskDSetRotationPanTiltRoll3x3(double pan, double tilt, double roll, double *M);							/* Double precision */

		/* Rotation matrix from pan and tilt */
FskAPI(void)	FskSSetRotationPanTilt3x3(float  pan, float  tilt, float  *M);											/* Single precision */
FskAPI(void)	FskDSetRotationPanTilt3x3(double pan, double tilt, double *M);											/* Double precision */

		/* Rotation matrix from normalized axis, sine and cosine */
FskAPI(void)	FskSSetRotationAxisSineCosine3x3(const float  *a, float  s, float  c, float  *M);						/* Single precision */
FskAPI(void)	FskDSetRotationAxisSineCosine3x3(const double *a, double s, double c, double *M);						/* Double precision */

		/* Rotation matrix from normalized axis and angle */
FskAPI(void)	FskSAxisAngleToMatrix3x3(const float  *a, float  radians, float  *M);									/* Single precision */
FskAPI(void)	FskDAxisAngleToMatrix3x3(const double *a, double radians, double *M);									/* Double precision */

		/* Rotation matrix from normalized WXYZ quaternion */
FskAPI(void)	FskSQuaternionWXYZToMatrix3x3(const float  *q, float  *M);												/* Single precision */
FskAPI(void)	FskDQuaternionWXYZToMatrix3x3(const double *q, double *M);												/* Double precision */


/********************************************************************************
 * Immersion for 3x3 matrices.
 ********************************************************************************/

		/* Immersion matrix from matrix and focal length */
FskAPI(void)	FskSImmerseUsingMatrix3x3(																				/* Single precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			const float		*R,				/* Rotation matrix */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	FskDImmerseUsingMatrix3x3(																				/* Double precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			const double	*R,				/* Rotation matrix */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Immersion matrix from quaternion and focal length */
FskAPI(void)	FskSImmerseUsingQuaternion3x3(																			/* Single precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			const float		*quat,			/* Orientation is specified using a normalized WXYZ quaternion */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	FskDImmerseUsingQuaternion3x3(																			/* Double precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			const double	*quat,			/* Orientation is specified using a normalized WXYZ quaternion */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Immersion matrix from pan, tilt, roll and focal length */
FskAPI(void)	FskSImmerseUsingPanTiltRoll3x3(																			/* Single precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			float			pan,			/* Rotation about the Y axis */
			float			tilt,			/* Rotation about the X axis */
			float			roll,			/* Rotation about the Z axis */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	FskDImmerseUsingPanTiltRoll3x3(																			/* Double precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			double			pan,			/* Rotation about the Y axis */
			double			tilt,			/* Rotation about the X axis */
			double			roll,			/* Rotation about the Z axis */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Immersion matrix from pan, tilt, roll and focal length */
FskAPI(void)	FskSImmerseUsingPanTilt3x3(																				/* Single precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			float			pan,			/* Rotation about the Y axis */
			float			tilt,			/* Rotation about the X axis */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	FskDImmerseUsingPanTilt3x3(																				/* Double precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
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
FskAPI(void)	FskSProjectUsingMatrix3x3(																				/* Single precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			const float		*R,				/* Rotation matrix */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	FskDProjectUsingMatrix3x3(																				/* Double precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			const double	*R,				/* Rotation matrix */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Projection matrix from quaternion and focal length */
FskAPI(void)	FskSProjectUsingQuaternion3x3(																			/* Single precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			const float		*quat,			/* Orientation is specified using a normalized WXYZ quaternion */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	FskDProjectUsingQuaternion3x3(																			/* Double precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			const double	*quat,			/* Orientation is specified using a normalized WXYZ quaternion */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Projection matrix from pan, tilt, roll and focal length */
FskAPI(void)	FskSProjectUsingPanTiltRoll3x3(																			/* Single precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			float			pan,			/* Rotation about the Y axis */
			float			tilt,			/* Rotation about the X axis */
			float			roll,			/* Rotation about the Z axis */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	FskDProjectUsingPanTiltRoll3x3(																			/* Double precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			double			pan,			/* Rotation about the Y axis */
			double			tilt,			/* Rotation about the X axis */
			double			roll,			/* Rotation about the Z axis */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Projection matrix from pan and tilt and focal length */
FskAPI(void)	FskSProjectUsingPanTilt3x3(																				/* Single precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			float			pan,			/* Rotation about the Y axis */
			float			tilt,			/* Rotation about the X axis */
			float			focalLength,	/* Focal length of the image plane, in pixels */
			const float		*center,		/* Center of image, relative to center of projection (can be NULL) */
			const float		*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			float			*M				/* The resulting matrix */
		);
FskAPI(void)	FskDProjectUsingPanTilt3x3(																				/* Double precision */
			int				width,			/* Width of the image */
			int				height,			/* Height of the image */
			double			pan,			/* Rotation about the Y axis */
			double			tilt,			/* Rotation about the X axis */
			double			focalLength,	/* Focal length of the image plane, in pixels */
			const double	*center,		/* Center of image, relative to center of projection (can be NULL) */
			const double	*aspectSkew,	/* Aspect ratio and skew (can be NULL) */
			double			*M				/* The resulting matrix */
		);

		/* Compute the projective coordinate frame mapping the unit square to the given 4 points, returning 1 if successful */
FskAPI(int)	FskSProjectiveFrameFromPoints3x3(const  float *pts,  float *M);												/* Single precision */
FskAPI(int)	FskDProjectiveFrameFromPoints3x3(const double *pts, double *M);												/* Double precision */

		/* Compute the projective transformation from the given 4 point correspondences, returning 1 if successful */
FskAPI(int)	FskSProjectionFromCorrespondences3x3(const  float *fr, const  float *to,  float *M);						/* Single precision */
FskAPI(int)	FskDProjectionFromCorrespondences3x3(const double *fr, const double *to, double *M);						/* Double precision */

		/* Compute the projective transformation from the given 4 point correspondences, returning 1 if successful */
FskAPI(int)	FskSDirectProjectionFromCorrespondences3x3(const  float *fr, const  float *to,  float *M);					/* Single precision */
FskAPI(int)	FskDDirectProjectionFromCorrespondences3x3(const double *fr, const double *to, double *M);					/* Double precision */


/********************************************************************************
 * Specializations for 4x4 matrices.
 ********************************************************************************/

		/* Standard embedding of a 3x3 matrix into a 4x4 matrix */
FskAPI(void)	FskSEmbed3x3In4x4Matrix(const float  *M3, float  *M4);													/* Single precision */
FskAPI(void)	FskDEmbed3x3In4x4Matrix(const double *M3, double *M4);													/* Double precision */

		/* Standard embedding of a 3x3 matrix into a 4x4 matrix, with transposition */
FskAPI(void)	FskSEmbedTransposed3x3In4x4Matrix(const float  *M3, float  *M4);										/* Single precision */
FskAPI(void)	FskDEmbedTransposed3x3In4x4Matrix(const double *M3, double *M4);										/* Double precision */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKMATRIX__ */


