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
	\file	FskFixedMath.h
	\brief	Fixed Point Math.
*/
#ifndef __FSKFIXEDMATH__
#define __FSKFIXEDMATH__


#ifndef __FSK__
	#include "Fsk.h"
#endif /* __FSK__ */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/********************************************************************************
 * Typedefs
 ********************************************************************************/

typedef SInt32	FskFixed;																				/**< Fixed point number  with 16 fractional bits. */
typedef SInt32	FskFract;																				/**< Fixed point number  with 30 fractional bits. */
typedef SInt32	FskFixed24;																				/**< Fixed point number  with 24 fractional bits. */
typedef SInt32	FskFixed28;																				/**< Fixed point number  with 28 fractional bits. */
typedef SInt32	FskFixedDegrees;																		/**< Fixed point degrees with 16 fractional bits */
typedef struct	FskIntPoint2D			{	SInt32		x, y;		}			FskIntPoint2D;			/**< 2D Point  with integer coordinates. */
typedef struct	FskFixedPoint2D			{	FskFixed	x, y;		}			FskFixedPoint2D;		/**< 2D Point  with fixed point coordinates with 16 fractional bits. */
typedef struct	FskFixedVector2D		{	FskFixed	x, y;		}			FskFixedVector2D;		/**< 2D Vector with fixed point coordinates with 16 fractional bits. */
typedef struct	FskFractVector2D		{	FskFract	x, y;		}			FskFractVector2D;		/**< 2D Vector with fixed point coordinates with 30 fractional bits. */
typedef struct	FskFixedVector3D		{	FskFixed	x, y, z;	}			FskFixedVector3D;		/**< 3D Vector with fixed point coordinates with 16 fractional bits. */
typedef struct	FskFixedMatrix3x2		{	FskFixed	M[3][2];	}			FskFixedMatrix3x2;		/**< 3x2 affine fixed point matrix (16 fractional bits) for transforming row vectors. */
typedef struct	FskFixedRectangleRecord	{	FskFixed	x, y, width, height; }	FskFixedRectangleRecord, *FskFixedRectangle;	/**< Fixed point rectangle. */
typedef const struct FskFixedRectangleRecord	*FskConstFixedRectangle;								/**<  Const  fixed point rectangle. */
typedef FskFixed (*FskFixedFuncOfFract)(FskFract t, void *p);											/**< Fixed point function of a fixed point fraction. */


#define FskRoundFloatToFract(x)			((FskFract)((x) * 1073741824.0 + (((x) >= 0) ? 0.5 : -0.5)))	/**< Round a float to a FskFract. */
#define FskRoundFloatToFixed(x)			((FskFixed)((x) *      65536.0 + (((x) >= 0) ? 0.5 : -0.5)))	/**< Round a float to a FskFixed. */
#define FskRoundFloatToFixed24(x)		((FskFixed24)((x) * 16777216.0 + (((x) >= 0) ? 0.5 : -0.5)))	/**< Round a float to a FskFixed24. */
#define FskRoundPositiveFloatToFract(x)	((FskFract)((x) * 1073741824.0 + 0.5))							/**< Round a positive float to a FskFract. */
#define FskRoundPositiveFloatToFixed(x)	((FskFixed)((x) *      65536.0 + 0.5))							/**< Round a positive float to a FskFixed. */
#define FskFractToFloat(x)				((x) * 9.31322574615478515625e-10)								/**< Convert a FskFract to a float or double. */
#define FskFixedToFloat(x)				((x) * 0.0000152587890625)										/**< Convert a FskFixed to a float or double. */
#define FskFixed24ToFloat(x)			((x) * 5.9604644775390625e-8)									/**< Convert a FskFixed24 to a float or double. */

#define FskFixedPoint2DSet( p, X, Y)		do { (p)->x = (X); (p)->y = (Y); } while(0)					/**< Set the value of a 2D fixed point. */
#define FskFixedVector2DSet(p, X, Y)		do { (p)->x = (X); (p)->y = (Y); } while(0)					/**< Set the value of a 2D fixed vector. */
#define FskFractVector2DSet(p, X, Y)		do { (p)->x = (X); (p)->y = (Y); } while(0)					/**< Set the value of a 2D fract vector. */
#define FskFixedVector3DSet(p, X, Y, Z)		do { (p)->x = (X); (p)->y = (Y); (p)->z = (Z); } while(0)	/**< Set the value of a 3D fixed vector. */
#define FskFixedRectangleSet(p, X, Y, W, H) do { (p)->x = (X); (p)->y = (Y); (p)->width = (W); (p)->height = (H); } while(0)	/**< Set the value of a fixed rectangle. */
#define FskFixedMatrix3x2Set(p, m00, m01, m10, m11, m20, m21) do {	(p)->M[0][0] = (m00); (p)->M[0][1] = (m01); (p)->M[1][0] = (m10); \
																	(p)->M[1][1] = (m11); (p)->M[2][0] = (m20); (p)->M[2][1] = (m21); } while(0)	/**< Set the value of a fixed 3x2 matrix. */


/********************************************************************************
 * Safe Converters
 ********************************************************************************/

/** Round and saturate a double to a FskFract.
 * \param[in]	d	The floating point number.
 * \return			The fixed point fraction, with 30 fractional bits,
 *					rounded to the closest representable number in [0.0, 1.0].
 */
FskAPI(FskFract)	FskRoundAndSaturateFloatToUnityFract(double d);

/** Round and saturate a double to a fixed point fraction, with specified fractional bits.
 * \param[in]	d			The floating point number.
 * \param[in]	fracBits	The number of fractional bits desired in the result.
 * \return					The fixed point fraction, with the specified number of fractional bits.
 *							The result is rounded to the closest representable number,
 *							which means saturation to 0x80000000 or 0x7FFFFFFF when the floating-point
 *							number is out of range.
 */
FskAPI(FskFract)	FskRoundAndSaturateFloatToNFixed(double d, UInt32 fracBits);

/** Round and saturate a double to a FskFixed.
 * \param[in]	d			The floating point number.
 * \return					The fixed point number, with 16 fractional bits.
 *							The result is rounded to the closest representable number,
 *							saturating to [-32768.0, 32767.9999847412].
 */
#define				FskRoundAndSaturateFloatToFixed(d)	FskRoundAndSaturateFloatToNFixed((d), 16)

/** Round and saturate a double to a FskFract.
 * \param[in]	d			The floating point number.
 * \return					The fixed point number, with 30 fractional bits.
 *							The result is rounded to the closest representable number,
 *							saturating to [-2.0, 1.9999999991].
 */
#define				FskRoundAndSaturateFloatToFract(d)	FskRoundAndSaturateFloatToNFixed((d), 30)


/********************************************************************************
 * Arithmetic
 ********************************************************************************/

/** Priority encoder.
 * \param[in]	x	The argument to be evaluated.
 * \return			The number of leading zeros in the 64 bit number, starting from the MSB.
 */
FskAPI(SInt32) FskLeadingZeros64(FskInt64 x);


/*
 * Multiplication
 */


/** Multiplication of two fixed point numbers, with 16 fractional bits.
 * \param[in]	x	The  multiplier,  with 16 fractional bits.
 * \param[in]	y	The multiplicand, with 16 fractional bits.
 * \return			The product, rounded to the closest representable value, with 16 fractional bits.
 *					The product is saturated to 0x80000000 and 0x7FFFFFFF for negative and positive products, respectively.
 */
FskAPI(FskFixed)	FskFixMul(FskFixed x, FskFixed y);

/** Multiplication of two fixed point numbers, with 30 fractional bits.
 * \param[in]	x	The  multiplier,  with 30 fractional bits.
 * \param[in]	y	The multiplicand, with 30 fractional bits.
 * \return			The product, rounded to the closest representable value, with 30 fractional bits.
 *					The product is saturated to 0x80000000 and 0x7FFFFFFF for negative and positive products, respectively.
 */
FskAPI(FskFract)	FskFracMul(FskFract x, FskFract y);				/* 30 fractional bits */

/** Multiplication of two fixed point numbers, with arbitrary numbers of fractional bits.
 * \param[in]	x	The  multiplier,  with (bx) fractional bits.
 * \param[in]	y	The multiplicand, with (by) fractional bits.
 * \param[in]	b	The amount to shift the result, computed as b = bx + by - bp.
 * \return			The product, rounded to the closest representable value, with (bp) fractional bits.
 *					The product is saturated to 0x80000000 and 0x7FFFFFFF for negative and positive products, respectively.
 * \bug				May not work if b > 31.
 */
FskAPI(SInt32)		FskFixedNMul(SInt32 x, SInt32 y, SInt32 b);

/** Multiplication of two 32-bit numbers, resulting in a 64-bit product.
 * \param[in]	x	The  multiplier.
 * \param[in]	y	The multiplicand.
 * \return			The 64-bit product.
 */
FskAPI(FskInt64)	FskMultiply32by32giving64(SInt32 x, SInt32 y);	/* 32 x 32 --> 64 */


/*
 * Division
 */


/** Division of two fixed point numbers, with 16 fractional bits.
 * \param[in]	n	The dividend, with 16 fractional bits.
 * \param[in]	d	The divisor,  with 16 fractional bits.
 * \result			The quotient, rounded to the closest representable value, with 16 fractional bits.
 *					The quotient is saturated to 0x80000000 and 0x7FFFFFFF for negative and positive products, respectively.
 *					The quotient of 0/0 is returned as 0.
 */
FskAPI(FskFixed)	FskFixDiv(FskFixed n, FskFixed d);				/* 16 fractional bits */

/** Division of two fixed point numbers, with 30 fractional bits.
 * \param[in]	n	The dividend, with 30 fractional bits.
 * \param[in]	d	The divisor,  with 30 fractional bits.
 * \result			The quotient, rounded to the closest representable value, with 30 fractional bits.
 *					The quotient is saturated to 0x80000000 and 0x7FFFFFFF for negative and positive products, respectively.
 *					The quotient of 0/0 is returned as 0.
 */
FskAPI(FskFract)	FskFracDiv(FskFract n, FskFract d);				/* 30 fractional bits */

/** Division of two fixed point numbers, with arbitrary numbers of fractional bits.
 * \param[in]	n	The dividend, with (bn) fractional bits.
 * \param[in]	d	The divisor,  with (bd) fractional bits.
 * \param[in]	b	The number of bits to shift the dividend, computed as b = bd - bn + bq.
 * \result			The quotient, rounded to the closest representable value, with (bq) fractional bits.
 *					The quotient is saturated to 0x80000000 and 0x7FFFFFFF for negative and positive products, respectively.
 *					The quotient of 0/0 is returned as 0.
 * \bug				May not work if b > 31.
 */
FskAPI(FskFixed)	FskFixedNDiv(SInt32 n, SInt32 d, SInt32 b);

/** Division of a 64 bit integer by a 32 bit integer, rounded to the closest 32-bit integer.
 * \param[in]	n	The dividend.
 * \param[in]	d	The divisor.
 * \return			The quotient of n/d, rounded to the closest representable integer.
 *					The quotient is saturated to 0x80000000 and 0x7FFFFFFF for negative and positive products, respectively.
 */
FskAPI(SInt32)		FskDivide64by32giving32(FskInt64 n, SInt32 d);

/** Division of a 64 bit integer by a 32 bit integer, with specified number of fractional bits for the quotient.
 * \param[in]	n	The dividend.
 * \param[in]	d	The divisor.
 * \param[in]	b	The number of fractional bits desired in the product.
 * \return			The quotient of n/d, rounded to the closest representable integer.
 *					The quotient is saturated to 0x80000000 and 0x7FFFFFFF for negative and positive products, respectively.
 */
FskAPI(FskFixed)	FskFixNDiv64(FskInt64 n, FskInt64 d, SInt32 b);	/* 64 / 64 -> 32 with b fractional bits */


/*
 * Ratio
 */

/** Scale a number by a rational number.
 * \param[in]	x	The number to be scaled.
 * \param[in]	num	The  numerator  of the rational fraction.
 * \param[in]	den	The denominator of the rational fraction.
 * \return			The scaled number, with the same number of fractional bits as x.
 */
FskAPI(FskFixed)	FskFixedRatio( FskFixed x, FskFixed num, FskFixed den);

/** Scale a number by a rational number, with the specified number of fractional bits for the result.
 * \param[in]	x	The number to be scaled.
 * \param[in]	num	The  numerator  of the rational fraction.
 * \param[in]	den	The denominator of the rational fraction.
 * \param[in]	n	The excess of numerator over denominator fractional bits.
 * \return			The scaled number, with the same number of fractional bits as x.
 */
FskAPI(FskFixed)	FskFixedNRatio(FskFixed x, FskFixed num, FskFixed den, SInt32 n);

/* Inline versions -- note: these may not be faster if there are assembly language versions */
#if 0	/* No rounding, with error of +/- 1, for fastest speed */
	#define	FskFixedNMulInline(a, b, n)		((FskFixed)(((FskInt64)(a) * ( b)) >> (n)))
	#define	FskFixedNDivInline(n, d, b)		((FskFixed)(((FskInt64)(n) << (b)) /  (d)))
	#define	FskFixedRatioInline(x, n, d)	((FskFixed)(((FskInt64)(x) *  (n)) /  (d)))
#else	/* Rounding, with error of +/- 0.5 */
	/** Inline fixed-point multiplication, with rounding.
	 * \param[in]	a	The multiplier.
	 * \param[in]	b	The multiplicand.
	 * \param[in]	n	The number of bits to shift out of the product.
	 * \return			The product.
	 */
	#define	FskFixedNMulInline(a, b, n)		((FskFixed)((((FskInt64)(a) *  (b)) + ((FskInt64)1 << ((n) - 1))) >> (n)))
	/** Inline fixed-point division, with rounding.
	 * \param[in]	n	The dividend.
	 * \param[in]	d	The divisor.
	 * \param[in]	b	The number of bits of fractional bits desired in the quotient,
	 *					assuming that n and d have the same number of fractional bits as each other.
	 * \return			The quotient.
	 */
	#define	FskFixedNDivInline(n, d, b)		((FskFixed)((((FskInt64)(n) << (b)) + ((((      (n) ^ (d)) < 0) ? -(d) : (d)) >> 1)) / (d)))
	/** Inline fixed-point ratio, with rounding.
	 * \param[in]	x	The number to be scaled.
	 * \param[in]	n	The  numerator  of the rational fraction.
	 * \param[in]	d	The denominator of the rational fraction.
	 * \return			The result.
	 */
	#define	FskFixedRatioInline(x, n, d)	((FskFixed)((((FskInt64)(x) *  (n)) + (((((x) ^ (n) ^ (d)) < 0) ? -(d) : (d)) >> 1)) / (d)))
#endif


/*
 * Conversion between ints
 */


/** Truncate a 16.16 fixed point number to an integer.
 * \param[in]	x	The fixed point number, with 16 fractional bits.
 * \return			The number truncated to the next lowest integer.
 */
#define FskTruncFixedToInt(x)	((x) >> 16)

/** Return the closest integer less than or equal to the given fixed point number with 16 fractional bits.
 * Same as truncation.
 * \param[in]	x	The fixed point number, with 16 fractional bits.
 * \return			The number floored to the next lowest integer.
 */
#define FskFloorFixedToInt(x)	((x) >> 16)

/** Return the closest integer greater than or equal to the given fixed point number with 16 fractional bits.
 * \param[in]	x	The fixed point number, with 16 fractional bits.
 * \return			The number ceiled to the next higher integer.
 */
#define FskCeilFixedToInt(x)	(((x) + 0xFFFF) >> 16)

/** Return the nearest integer to the given fixed point number with 16 fractional bits.
 * \param[in]	x	The fixed point number, with 16 fractional bits.
 * \return			The number rounded to the closest integer.
 */
#define FskRoundFixedToInt(x)	(((x) + 0x8000) >> 16)

/** Convert an integer to a fixed point number with 16 fractional bits.
 * \param[in]	x	The integer.
 * \return			The fixed point equivalent, with 16 fractional bits.
 */
#define FskIntToFixed(x)		((x) << 16)


/********************************************************************************
 * Elementary functions
 ********************************************************************************/


/*
 * Square roots.
 */


/** Fixed point 16.16 square root.
 *	\param[in]	a	The argument, in 16.16 fixed point format.
 *	\return			The square root, in 16.16 fixed point format.
 *					If the argument was negative, the result has the sign bit set to indicate imaginary.
 */
FskAPI(FskFixed)	FskFixSqrt( FskFixed a);

/** Fixed point fractional 2.30 square root.
 *	\param[in]	a	The argument, in 2.30 fixed point format.
 *	\return			The square root, in 2.30 fixed point format.
 *					If the argument was negative, the result has the sign bit set to indicate imaginary.
 */
FskAPI(FskFract)	FskFracSqrt(FskFract a);

/** Fixed point fractional (32-n).n square root.
 *	\param[in]	a	The argument, in (32-n).n fixed point format.
 *	\param[in]	n	The number of fractional bits must be even.
 *	\return			The square root, in (32-n).n fixed point format.
 *					If the argument was negative, the result has the sign bit set to indicate imaginary.
 */
FskAPI(FskFixed)	FskFixedNSqrt(FskFixed a, SInt32 n);

/** Integer square root.
 * The result is rounded to the nearest integer.
 *	\param[in]	x	The argument, a 64-bit integer.
 *	\return			The square root, a 32-bit integer.
 *					If the argument was negative, the result has the sign bit set to indicate imaginary.
 */
FskAPI(FskFixed)	FskFixedSqrt64to32(FskInt64 x);

/** Compute the hypotenuse of a right triangle, or the magnitude of a 2-vector.
 *	\param[in]	x	The x-coordinate.
 *	\param[in]	y	The y-coordinate.
 *	\return			The square root, with the same number of fractional bits that the coordinates have.
 */
FskAPI(FskFixed)	FskFixedHypot(FskFixed x, FskFixed y);


/*
 * Trigonometry, where angle is given in fixed point (16.16) degrees.
 */


/** Compute the cosine and sine of an angle.
 * The accuracy is approximately 20 bits.
 * \param[in]	theta		The angle, in degrees, with 16 fractional bits.
 * \param[out]	cosineSine	A pointer to a location to successively store the compute cosine and sine.
 */
FskAPI(void)	FskFracCosineSine(FskFixedDegrees theta, FskFract *cosineSine);

/** Rotate a vector.
 * This can be used to convert from polar to rectangular by setting x=radius, y=0.
 * \param[in,out]	xy	Pointer to the 2-vector to be rotated. The rotated vector is returned in the same location.
 * \param[in]		theta	The rotation angle, in degrees, expressed with 16 fractional bits.
 */
FskAPI(void)	FskFixedVectorRotate(SInt32 xy[2], FskFixedDegrees theta);

/** Determine the inclination of a vector.
 * \param[in]	x	The x-coordinate of the vector.
 * \param[in]	y	The y-coordinate of the vector.
 * \return			The angle of inclination, measured from the x-axis to the y-axis, in degrees with 16 fractional bits.
 */
FskAPI(FskFixedDegrees)	FskFixedVectorInclination(SInt32 x, SInt32 y);

/** Convert from rectangular to polar coordinates.
 * \param[in,out]	argx	On input,  pointer to the x-coordinate.
 *							On output, pointer to the radius.
 * \param[in,out]	argy	On input,  pointer to the y-coordinate.
 *							On output, pointer to the angle, in degrees, with 16 fractional bits.
 */
FskAPI(void)	FskFixedPolarize(SInt32 *argx, SInt32 *argy);

/** Compute the function atan2 in fixed-point coordinates.
 * \param[in]	y	The y-coordinate.
 * \param[in]	x	The x-coordinate.
 * \return			The angle of the slope y/x, but in all four quadrants.
 *					Represented in degrees with 16 fractional bits.
 */
#define			FskFixedAtan2(y, x)			FskFixedVectorInclination((x), (y))


/********************************************************************************
 * Bezier curve evaluations - via the deCasteljau algorithm.
 *
 * eval is lower triangular, and contains at least
 *		(order) * (order + 1) / 2
 * elements
 *	...
 *	b03	...
 *	b02	b12	...
 *	b01	b11	b21	...
 *	b0	b1	b2	b3	...
 ********************************************************************************/

/** The maximum order (degree + 1) of a Bezier curve -- used for allocating temporary storage. */
#define		kFskMaxBezierOrder	4

/** Evaluate a vector-valued Bezier, componentwise, using the deCasteljau algorithm.
 * The entire evaluation tree is returned.
 * \param[in]	control		The control vector.
 * \param[in]	order		The order (degree + 1) of the Bezier curve.
 * \param[in]	strideBytes	The number of bytes between one control point and the next.
 * \param[in]	t			The parametric value where the curve is to be evaluated.
 * \param[out]	eval		The deCasteljau tree, beginning with the value of the curve, with (order) * (order + 1) / 2 elements.
 */
FskAPI(void)	FskFixedDeCasteljau(const FskFixed *control, UInt32 order, SInt32 strideBytes, FskFract t, FskFixed *eval);

/** Evaluate a scalar Bezier, with deCasteljau tree initialized with the control points.
 * \param[in,out]	tri		The deCasteljau tree, with (order) * (order + 1) / 2 elements.
 *							The last (order) values are initialized by the caller.
 *							The remainder are computed by this function.
 * \param[in]		order	The order (degree + 1) of the curve.
 * \param[in]		t		The parametric value where the curve is to be evaluated.
 */
FskAPI(void)	FskFixedDeCasteljauKernel(FskFixed *tri, UInt32 order, FskFract t);

/** Split a Bezier into two at the specified parameter value t.
 * \param[in]	control		The control vector.
 * \param[in]	order		The order (degree + 1) of the Bezier curve.
 * \param[in]	dimension	The dimension of the points, e.g. 2 for a planar Euclidean curve, 3 for a planar homogeneous curve.
 * \param[in]	t			The parametric value where the curve is to be evaluated.
 * \param[out]	L			The resultant left  curve, corresponding to the interval [0,t].
 * \param[out]	R			The resultant right curve, corresponding to the interval [t,1].
 */
FskAPI(void)	FskFixedSplitBezier(const FskFixed *control, UInt32 order, UInt32 dimension, FskFixed t, FskFixed *L, FskFixed *R);

/** Evaluate a vector-valued Bezier at 1/2, componentwise.
 * \param[in]	control		The control vector.
 * \param[in]	order		The order (degree + 1) of the Bezier curve.
 * \param[in]	strideBytes	The number of bytes between one control point and the next.
 * \param[out]	eval		The deCasteljau tree, beginning with the value of the curve, with (order) * (order + 1) / 2 elements.
 */
FskAPI(void)	FskFixedBisectDeCasteljau(const FskFixed *control, UInt32 order, SInt32 strideBytes, FskFixed *eval);

/** Split a Bezier into two at the parameter value 1/2.
 * \param[in]	control		The control vector, with components interleaved..
 * \param[in]	order		The order (degree + 1) of the Bezier curve.
 * \param[in]	dimension	The dimension of the points, e.g. 2 for a planar Euclidean curve, 3 for a planar homogeneous curve.
 * \param[out]	L			The resultant left  curve, corresponding to the interval [0,1/2].
 * \param[out]	R			The resultant right curve, corresponding to the interval [1/2,1].
 */
FskAPI(void)	FskFixedBisectBezier(const FskFixed *control, UInt32 order, UInt32 dimension, FskFixed *L, FskFixed *R);

/** Compute the maximum distance of the control points from the base of a 2D Bezier polygon.
 * \param[in]	points	The control points (x, y).
 * \param[in]	order	The order of the curve, also the number of control points.
 * \return				The maximum distance of the control points from the base.
 */
FskAPI(FskFixed)	FskFixedDeviationOfBezierControlPoints2D(const FskFixedPoint2D *points, long order);

/** Compute the derivative Bezier of a Bezier polynomial.
 * \param[in]	coeff	The coefficients of the Bezier polynomial.
 * \param[out]	deriv	The coefficients of the derivative Bezier curve.
 * \param[in]	order	The order of the Bezier polynomial. Its derivative has order (order-1).
 */
FskAPI(void)	FskFixedDerivativeOfBezier(const FskFixed *coeff, FskFixed *deriv, SInt32 order);

/** Bezier vector polynomial evaluation.
 * Computes position and tangent.
 * \param[in]	coeff		The coefficient vector, with components interleaved.
 * \param[in]	order		The order of the Bezier curve.
 * \param[in]	dimension	The number of components in each control point.
 * \param[in]	t			The parametric value where the curve is to be evaluated.
 * \param[out]	position	The position of the curve at parametric value t, with (dimension) components.
 * \param[out]	tangent		The position of the curve at parametric value t, with (dimension) components.
 */
FskAPI(void)	FskFixedEvaluateBezierVector(const FskFixed *coeff, SInt32 order, UInt32 dimension, FskFixed t, FskFixed *position, FskFixed *tangent);



/** Evaluation of a polynomial with coefficients given in the traditional power basis, by the method of Horner.
 * \param[in]	coeff	The coefficient vector.
 * \param[in]	order	The order (degree - 1) of the polynomial.
 * \param[in]	x		The ordinate.
 * \return				The value of the polynomial at the location (x).
 */
FskAPI(FskFixed)	FskFixedHorner(const FskFixed *coeff, SInt32 order, FskFract x);



/********************************************************************************
 * Vector and Matrix Functions
 ********************************************************************************/

/*
 * Vector norms and normalization
 */


/** Euclidean norm of a vector.
 * \param[in]	v	The vector.
 * \param[in]	n	The length of the vector.
 * \return			The L2, Euclidean norm of the vector.
 */
FskAPI(FskFixed)	FskFixedVectorNorm(const FskFixed *v, SInt32 n);

/** Normalize a vector.
 * \param[in,out]	v	On input, the vector, given any number of fractional bits.
 *						On output, the normalized vector, with 30 fractional bits.
 * \param[in]		n	The length of the vector.
 * \return				The norm of the original vector.
 */
FskAPI(FskFract)	FskFixedVectorNormalize( FskFract *v, SInt32 n);

/** Inline Euclidean norm of a 2-vector.
 * \param[in]	a	Pointer to a 2-vector, such as FskFixedVector2D.
 * \return			The L2, Euclidean norm of the vector.
 */
#define				FskFixedVectorNorm2D(a)	FskFixedSqrt64to32((FskInt64)((a)->x) * (a)->x + (FskInt64)((a)->y) * (a)->y)


/*
 * Dot product
 */


/** Compute the dot product of a vector of fixed point numbers.
 * The highest possible precision is used in the computations, and the result is rounded at the end.
 * \param[in]	a			One vector.
 * \param[in]	b			The other vector.
 * \param[in]	length		The length of the a and b vectors.
 * \param[in]	numSubBits	The number of fractional bits in the a and b vectors.
 * \return					The dot product, rounded to the closest representable value.
 */
FskAPI(FskFixed)	FskFixedNDotProduct(const FskFixed *a, const FskFixed *b, UInt32 length, SInt32 numSubBits);

/** Compute the dot product of a vector of fixed point numbers with 16 fractional bits.
 * The highest possible precision is used in the computations, and the result is rounded at the end.
 * \param[in]	a	One vector, with 16 fractional bits.
 * \param[in]	b	The other vector, with 16 fractional bits.
 * \param[in]	n	The length of the a and b vectors.
 * \return			The dot product, with 16 fractional bits, rounded to the closest representable value.
 */
FskAPI(FskFixed)	FskFixedDotProduct( const FskFixed *a, const FskFixed *b, UInt32 n);

/** Compute the dot product of a vector of fixed point numbers with 30 fractional bits.
 * The highest possible precision is used in the computations, and the result is rounded at the end.
 * \param[in]	a	One vector, with 30 fractional bits.
 * \param[in]	b	The other vector, with 30 fractional bits.
 * \param[in]	n	The length of the a and b vectors.
 * \return			The dot product, with 30 fractional bits, rounded to the closest representable value.
 */
FskAPI(FskFract)	FskFractDotProduct( const FskFract *a, const FskFract *b, UInt32 n);

/** Linear combination of two fixed point numbers.
 * The highest possible precision is used in the computations, and the result is rounded at the end.
 * \param[in]	a0	The scale  factor   for     b0,       with n fractional bits.
 * \param[in]	b0	The first  element from the b vector, with m fractional bits.
 * \param[in]	a1	The scale  factor   for     b1,       with n fractional bits.
 * \param[in]	b1	The second element from the b vector, with m fractional bits.
 * \param[in]	n	The number of fractional bits in the scale factors.
 * \return			The product a0*b0 + a1*b1,            with m fractional bits.
 */
FskAPI(FskFixed)	FskFixedNLinear2D(FskFixed a0, FskFixed b0, FskFixed a1, FskFixed b1, SInt32 n);

/** Affine combination of two fixed point numbers.
 * The highest possible precision is used in the computations, and the result is rounded at the end.
 * \param[in]	a0	The scale  factor  for      b0,       with n fractional bits.
 * \param[in]	b0	The first  element from the b vector, with m fractional bits.
 * \param[in]	a1	The scale  factor  for      b1,       with n fractional bits.
 * \param[in]	b1	The second element from the b vector, with m fractional bits.
 * \param[in]	a2	The offset,                           with m fractional bits.
 * \param[in]	n	The number of fractional bits in the scale factors.
 * \return			The sum a0*b0 + a1*b1 + a2,           with m fractional bits.
 */
FskAPI(FskFixed)	FskFixedNAffine2D(FskFixed a0, FskFixed b0, FskFixed a1, FskFixed b1, FskFixed a2, SInt32 n);

/** Linear combination of two fixed point numbers, with 16 fractional bits.
 * The highest possible precision is used in the computations, and the result is rounded at the end.
 * \param[in]	a0	The scale  factor   for     b0,       with 16 fractional bits.
 * \param[in]	b0	The first  element from the b vector, with  m fractional bits.
 * \param[in]	a1	The scale  factor   for     b1,       with 16 fractional bits.
 * \param[in]	b1	The second element from the b vector, with  m fractional bits.
 * \return			The product a0*b0 + a1*b1,            with  m fractional bits.
 */
#define				FskFixedLinear2D(a0, b0, a1, b1)		FskFixedNLinear2D(a0, b0, a1, b1, 16)

/** Affine combination of two fixed point numbers, with 16 fractional bits.
 * The highest possible precision is used in the computations, and the result is rounded at the end.
 * \param[in]	a0	The scale  factor  for      b0,       with 16 fractional bits.
 * \param[in]	b0	The first  element from the b vector, with  m fractional bits.
 * \param[in]	a1	The scale  factor  for      b1,       with 16 fractional bits.
 * \param[in]	b1	The second element from the b vector, with  m fractional bits.
 * \param[in]	a2	The offset,                           with  m fractional bits.
 * \return			The sum a0*b0 + a1*b1 + a2,           with  m fractional bits.
 */
#define				FskFixedAffine2D(a0, b0, a1, b1, a2)	FskFixedNAffine2D(a0, b0, a1, b1, a2, 16)

/** Dot product of two fixed point 2-vectors.
 * \param[in]	a	The first  2-vector,                with n fractional bits.
 * \param[in]	b	The second 2-vector,                with m fractional bits.
 * \param[in]	n	The number of fractional bits in the a vector.
 * \return			The product a[0]*b[0] + a[1]*b[1], with  m fractional bits.
 */
#define				FskFixedNDotProduct2D(a, b, n)			FskFixedNLinear2D((a)->x, (b)->x, (a)->y, (b)->y, (n))

/** Dot product of two fixed point 2-vectors, with 16 fractional bits.
 * \param[in]	a	The first  2-vector,               with 16 fractional bits.
 * \param[in]	b	The second 2-vector,               with  m fractional bits.
 * \return			The product a[0]*b[0] + a[1]*b[1], with  m fractional bits.
 */
#define				FskFixedDotProduct2D(a, b)				FskFixedNLinear2D((a)->x, (b)->x, (a)->y, (b)->y, 16)

/** Dot product of two fixed point 2-vectors, with 30 fractional bits.
 * \param[in]	a	The first  2-vector,                with 30 fractional bits.
 * \param[in]	b	The second 2-vector,                with m fractional bits.
 * \return			The product a[0]*b[0] + a[1]*b[1], with  m fractional bits.
 */
#define				FskFractDotProduct2D(a, b)				FskFixedNLinear2D((a)->x, (b)->x, (a)->y, (b)->y, 30)


/*
 * Cross Product
 */

/** Cross product of two FskFixed 2-vectors.
 * \param[in]	a	The first   (left) 2-vector, with 16 fractional bits.
 * \param[in]	b	The second (right) 2-vector, with 16 fractional bits.
 * \return			The cross product a X b,     with 16 fractional bits.
 */
FskAPI(FskFixed)	FskFixedCrossProduct2D(const FskFixedVector2D *a, const FskFixedVector2D *b);

/** Cross product of two FskFract 2-vectors.
 * \param[in]	a	The first   (left) 2-vector, with 30 fractional bits.
 * \param[in]	b	The second (right) 2-vector, with 30 fractional bits.
 * \return			The cross product a X b,     with 30 fractional bits.
 */
FskAPI(FskFract)	FskFractCrossProduct2D(const FskFractVector2D *a, const FskFractVector2D *b);

/*
 * Distances
 */

/** Euclidean, L2, distance between two n-dimensional fixed-point points.
 * \param[in]	p0	The first  n-dimensional point.
 * \param[in]	p1	The second n-dimensional point.
 * \param[in]	n	The dimension of the points.
 * \result			The L2 Euclidean distance between the two points.
 */
FskAPI(FskFixed)	FskFixedDistance(const FskFixed *p0, const FskFixed *p1, SInt32 n);

/** The distance between a point and a line determined by two other points.
 * \param[in]	point	Pointer to a 2D point.
 * \param[in]	line	Pointer to two 2D points, which determine a line.
 * \return				The closest distance from the point to the infinite line determined by two points.
 */
FskAPI(FskFixed)	FskFixedPointLineDistance2D(const FskFixedPoint2D *point, const FskFixedPoint2D *line);

/** "Perp" a 2D vector, i.e. rotate it by 90 degrees.
 * This function works in-place, i.e. when fr == to.
 * \param[in]	fr	The original vector.
 * \param[out]	to	The rotated, or "perped" vector.
 */
FskAPI(void)		FskFixedPerpVector2D(const FskFixedVector2D *fr, FskFixedVector2D *to);

/** Interpolate between two points.
 * \param[in]	t	The parameter of interpolation, typically between [0, 1].
 * \param[in]	n	The dimension of the points.
 * \param[in]	p0	The point corresponding to t==0.
 * \param[in]	p1	The point corresponding to t==1.
 * \param[out]	pt	The interpolated point, pt = (1 - t) * p0 + t * p1.
 */
FskAPI(void)		FskFixedInterpolate(FskFract t, UInt32 n, const FskFixed *p0, const FskFixed *p1, FskFixed *pt);

/** 2x2 Matrix norm - suitable for determining a single scale factor.
 * This is the geometric average of the two eigenvalues, equivalent to finding the radius of
 * a circle whose area is the same as the ellipse represented by the matrix.
 * \param[in]	M		Pointer to a matrix of fixed-point numbers with 16 fractional bits.
 * \param[in]	nCols	The delta index between one row of the matrix and the next.
 *						The elements of the matrix are chosen as:
 *							M[0]		M[1]
 *							M[nCols+0]	M[nCols+1]
 * \return				The norm of the matrix.
 */
FskAPI(FskFixed)	FskFixedMatrixNorm2x2(const FskFixed *M, SInt32 nCols);

/** Multiply two fixed-point 3x2 matrices.
 * This works in-place, i.e. P can be either L or R or something altogether different.
 * \param[in]	L	The  left   3x2 matrix.
 * \param[in]	R	The  right  3x2 matrix.
 * \param[out]	P	The product 3x2 matrix.
 * \param[in]	n	The number of bits to shift down after the product: n = nL + nR - nP,
 *					where nL, nR, and nP are the numbers of fractional bits in the left, right, and product matrices, respectively.
 */
FskAPI(void)		FskFixedNMultiplyMatrix3x2(const FskFixedMatrix3x2 *L, const FskFixedMatrix3x2 *R, FskFixedMatrix3x2 *P, SInt32 n);

/** Multiply two fixed-point 3x2 matrices, with 16 fractional bits.
 * This works in-place, i.e. P can be either L or R or something altogether different.
 * \param[in]	L	The  left   3x2 matrix.
 * \param[in]	R	The  right  3x2 matrix.
 * \param[out]	P	The product 3x2 matrix.
 */
#define				FskFixedMultiplyMatrix3x2(L, R, P)		FskFixedNMultiplyMatrix3x2(L, R, P, 16)

/** Multiply two general 3x2 fixed point matrices,
 * where each matrix has its own number of fractional bits for the upper 2x2 and the lower 1x2.
 * \param[in]	L			The left matrix.
 * \param[in]	LScaleBits	The number of fractional bits in the upper 2x2 of L.
 * \param[in]	LTransBits	The number of fractional bits in the lower 1x2 of L.
 * \param[in]	R			The right matrix.
 * \param[in]	RScaleBits	The number of fractional bits in the upper 2x2 of R.
 * \param[in]	RTransBits	The number of fractional bits in the lower 1x2 of R.
 * \param[out]	P			The product matrix, P = L R
 * \param[in]	PScaleBits	The number of fractional bits in the upper 2x2 of P.
 * \param[in]	PTransBits	The number of fractional bits in the lower 1x2 of P.
 */
FskAPI(void)		FskFixedGeneralMultiplyMatrix3x2(
						const FskFixedMatrix3x2	*L,		SInt32 LScaleBits,	SInt32 LTransBits,
						const FskFixedMatrix3x2	*R,		SInt32 RScaleBits,	SInt32 RTransBits,
						FskFixedMatrix3x2		*P,		SInt32 PScaleBits,	SInt32 PTransBits
					);

/** Invert L and multiply it with R, where L has LFracBits.
 * This works in-place, i.e. P can be the same as L or R.
 * \param[in]	L			The  left   3x2 matrix.
 * \param[in]	R			The  right  3x2 matrix.
 * \param[out]	P			The product 3x2 matrix.
 * \param[in]	LFracBits	The number of fractional bits in L.
 */
FskAPI(FskFixed)	FskFixedNMultiplyInverseMatrix3x2(const FskFixedMatrix3x2 *L, const FskFixedMatrix3x2 *R, FskFixedMatrix3x2 *P, SInt32 LFracBits);

/** Invert L and multiply it with R, where L has 16 fractional bits.
 * This works in-place, i.e. P can be the same as L or R.
 * \param[in]	L	The  left   3x2 matrix.
 * \param[in]	R	The  right  3x2 matrix.
 * \param[out]	P	The product 3x2 matrix.
 */
#define				FskFixedMultiplyInverseMatrix3x2(L, R, P)		FskFixedNMultiplyInverseMatrix3x2(L, R, P, 16)

/** Multiply L with the inverse of R, where L, R, and P have 16 fractional bits.
 * This works in-place, i.e. P can be the same as L or R.
 * \param[in]	L	The  left   3x2 matrix.
 * \param[in]	R	The  right  3x2 matrix.
 * \param[out]	P	The product 3x2 matrix.
 */
FskAPI(FskFixed)	FskFixedMultiplyMatrixInverse3x2(const FskFixedMatrix3x2 *L, const FskFixedMatrix3x2 *R, FskFixedMatrix3x2 *P);

/** Transform the fixed-point 2D points by the given fixed-point 3x2 matrix.
 * This works in-place, i.e. src can be the same as dst.
 * \param[in]	src			The source vector of 2D points.
 * \param[in]	numPoints	The number of points to be transformed.
 * \param[in]	M			The 3x2 matrix.
 * \param[out]	dst			The resultant transformed points, which can be the same as src.
 */
FskAPI(void)		FskTransformFixedRowPoints2D(const FskFixedPoint2D *src, UInt32 numPoints, const FskFixedMatrix3x2 *M, FskFixedPoint2D *dst);

/** Transform the fixed-point 2D points by the inverse of the given fixed-point 3x2 matrix.
 * This works in-place.
 * \param[in]	src			The source vector of 2D points.
 * \param[in]	numPoints	The number of points to be transformed.
 * \param[in]	M			The 3x2 matrix.
 * \param[out]	dst			The resultant transformed points, which can be the same as src.
 * \return					The scaled determinant.
 */
FskAPI(FskFixed)	FskInverseTransformFixedRowPoints2D(const FskFixedPoint2D *src, UInt32 numPoints, const FskFixedMatrix3x2 *M, FskFixedPoint2D *dst);

/** 2x2 Matrix inversion.
 * This works in-place, i.e. (fr) can be equal to (to).
 * \param[in]	fr	The source matrix, with 16 fractional bits.
 * \param[out]	to	The resultant inverted matrix, with 16 fractional bits.
 * \return			The determinant. Note that the inversion failed if the determinant was zero.
 */
FskAPI(FskFixed)	FskFixedMatrixInvert2x2(const FskFixed *fr, FskFixed *to);

/** 3x2 Matrix inversion.
 * This works in-place, i.e. (fr) can be equal to (to).
 * \param[in]	fr	The source matrix, with 16 fractional bits.
 * \param[out]	to	The resultant inverted matrix, with 16 fractional bits.
 * \return			The determinant. Note that the inversion failed if the determinant was zero.
 */
FskAPI(FskFixed)	FskFixedMatrixInvert3x2(const FskFixedMatrix3x2 *fr, FskFixedMatrix3x2 *to);

/** Invert a general fixed point 3x2 matrix.
 * \param[in]	fr		The source 3x2 matrix.
 * \param[in]	frSBits	The number of fractional bits in the upper 2x2 of (fr).
 * \param[in]	frTBits	The number of fractional bits in the lower 1x2 of (fr).
 * \param[out]	to		The resultant inverted 3x2 matrix. May be the same as src.
 * \param[in]	toSBits	The number of fractional bits in the upper 2x2 of (to).
 * \param[in]	toTBits	The number of fractional bits in the lower 1x2 of (to).
 * \return				The determinant of the (fr) matrix.	How many fractional bits?
 */
FskAPI(FskFixed)	FskFixedGeneralMatrixInvert3x2(
						const FskFixedMatrix3x2	*fr,	SInt32	frSBits,	SInt32	frTBits,
						FskFixedMatrix3x2		*to,	SInt32	toSBits,	SInt32	toTBits
					);

/** Set the given 3x2 matrix to the identity matrix.
 * \param[out]	M	The matrix to be initialized.
 */
FskAPI(void)		FskFixedIdentityMatrix3x2(FskFixedMatrix3x2 *M);

/** Solve the affine matrix equation { b = a M } for 2-vector a, given 3x2 matrix M and 2-vector b, returning the determinant.
 * \param[in]	b	The constant vector,           with 16 fractional bits.
 * \param[out]	a	The unknown (solution) vector, with 16 fractional bits.
 * \param[in]	M	The 3x2 fixed-point matrix,    with 16 fractional bits.
 * \return			The determinant of the matrix, with 16 fractional bits.
 */
FskAPI(FskFixed)	FskSolveFixedMatrix3x2Equation(const FskFixedPoint2D *b, FskFixedPoint2D *a, const FskFixedMatrix3x2 *M);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKFIXEDMATH__ */

