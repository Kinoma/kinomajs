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
	\file		FskPolygon.h
	\brief		Polygon rendering.
*/

#ifndef __FSKPOLYGON__
#define __FSKPOLYGON__

#ifndef __FSKFIXEDMATH__
# include "FskFixedMath.h"
#endif /* __FSKFIXEDMATH__ */

#ifndef __FSKGRAPHICS_H__
# include "FskGraphics.h"
#endif /* __FSKGRAPHICS_H__ */

#ifndef __FSKBITMAP__
# include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifndef __FSKRECTANGLE__
	#include "FskRectangle.h"
#endif /* __FSKRECTANGLE__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** End cap specification. */
enum {
	kFskLineEndCapRound			= 0,					/**< Rounded end caps. */
	kFskLineEndCapSquare		= 1,					/**< Square end caps, circumscribing the round end caps. */
	kFskLineEndCapButt			= 2,					/**< Butt end caps, like square, except terminates right in the endpoint. */
	kFskLineEndCapClosed		= 4,					/**< This is not used in external APIs. */
	kFskLineEndCapSVGDefault	= kFskLineEndCapButt,	/**< The SVG default endcap is butt. */
	kFskLineEndCapCanvasDefault	= kFskLineEndCapButt	/**< The Canvas default endcap is butt. */
};


/** Line join specification. */
enum {
	kFskLineJoinRound			= 0,					/**< Round line join. */
	kFskLineJoinBevel			= 0x10000,				/**< Bevel line join, i.e. all joins are chamfered (miter limit of 0 degrees). */
	kFskLineJoinMiter90			= 0x16A0A,				/**< 90 degree miter limit = 2*arccsc(0x16A0A/65536); other miter limits are computed as csc(angle/2). */
	kFskLineJoinSVGDefault		= 0x40000,				/**< SVG has a default of 29 degrees = 2*arccsc(4). */
	kFskLineJoinCanvasDefault	= 0xA0000				/**< Canvas has a default of 11.5 degrees = 2*arccsc(10). */
};


/** Fill rule specification. */
enum {
	kFskFillRuleNone			= (-1),					/**< Don't fill (use in API's that don't explicitly state frame or fill */
	kFskFillRuleNonZero			= 0,					/**< SVG terminology */
	kFskFillRuleWindingNumber	= 0,					/**< Traditional terminology */
	kFskFillRuleEvenOdd			= 1,					/**< SVG terminology */
	kFskFillRuleParity			= 1,					/**< Traditional terminology */
	kFskFillRuleSVGDefault		= kFskFillRuleNonZero,	/**< The default SVG fill rule is nonzero. */
	kFskFillRuleCanvasDefault	= kFskFillRuleNonZero	/**< Canvas specifies that the nonzero rule is to be used for filling. */
};


/** Line width specification. Normal line widths are specified as 16.16 fixed point numbers. */
enum {
	kFskStrokeWidthNone			= (-1)	/**< Don't frame. Used in APIs that don't explicitly state frame or fill. */
};


/** Gradient and Texture spread methods. */
enum {
	kFskSpreadMask			= 3,											/**< The mask for the spread bits (internal). */
	kFskSpreadPosX			= 0,											/**< The position for the X spread (internal). */
	kFskSpreadPosY			= 2,											/**< The position for the Y spread (internal). */
	kFskSpreadTransparentX	= 0<<kFskSpreadPosX,							/**< The pattern is to be transparent outside of its primary domain in X. */
	kFskSpreadPadX			= 1<<kFskSpreadPosX,							/**< The pattern is to be padded to the nearest edge color outside of its primary domain in X. */
	kFskSpreadRepeatX		= 2<<kFskSpreadPosX,							/**< The pattern is to be repeated in X outside of its primary domain. */
	kFskSpreadReflectX		= 3<<kFskSpreadPosX,							/**< The pattern is to be reflected in X outside of its primary domain. */
	kFskSpreadTransparentY	= 0<<kFskSpreadPosY,							/**< The pattern is to be transparent outside of its primary domain in Y. */
	kFskSpreadPadY			= 1<<kFskSpreadPosY,							/**< The pattern is to be padded to the nearest edge color outside of its primary domain in Y. */
	kFskSpreadRepeatY		= 2<<kFskSpreadPosY,							/**< The pattern is to be repeated in Y outside of its primary domain. */
	kFskSpreadReflectY		= 3<<kFskSpreadPosY,							/**< The pattern is to be reflected in Y outside of its primary domain. */
	kFskSpreadTransparent	= kFskSpreadTransparentX|kFskSpreadTransparentY,/**< The pattern is to be transparent outside of its primary domain in X and Y. */
	kFskSpreadPad			= kFskSpreadPadX|kFskSpreadPadY,				/**< The pattern is to be padded to the nearest edge color outside of its primary domain in X and Y. */
	kFskSpreadRepeat		= kFskSpreadRepeatX|kFskSpreadRepeatY,			/**< The pattern is to be repeated in both X and Y outside of its primary domain. */
	kFskSpreadReflect		= kFskSpreadReflectX|kFskSpreadReflectY			/**< The pattern is to be reflected in both X and Y outside of its primary domain. */
};


/** Gradient stop specification. */
typedef struct FskGradientStop {
	FskFract			offset;					/**< 2.30, i.e., specified with 30 fractional bits. */
	FskColorRGBARecord	color;					/**< The color to be used at the given offset. */
} FskGradientStop;


/** Color source specification.
 *	Called a"paint server" in SVG.
 */
#define FSK_POLYGON_TEXTURE 1
enum {
	kFskColorSourceTypeConstant				= 0,	/**< A constant color. */
	kFskColorSourceTypeLinearGradient,				/**< A linear gradient. */
	kFskColorSourceTypeRadialGradient,				/**< A radial gradient. */
	kFskColorSourceTypeTexture,						/**< A texture or pattern. */
	kFskColorSourceTypeProcedure,					/**< A procedure. */
#if defined(FSK_POLYGON_TEXTURE) && FSK_POLYGON_TEXTURE
	kFskColorSourceTypeMax					= kFskColorSourceTypeProcedure-1	/* Procedural color sources not implemented. TODO: implement textures. */
#else /* !FSK_POLYGON_TEXTURE */
	kFskColorSourceTypeMax					= kFskColorSourceTypeTexture-1		/* Procedural and texture color sources not implemented */
#endif /* !FSK_POLYGON_TEXTURE */
};


/** Base class color source. */
typedef struct FskColorSource {
	UInt32					type;				/**< Contains only the type. */
	UInt32					dashCycles;			/**< The number of cycles in the dash pattern */
	FskFixed				*dash;				/**< The duration of the on/off dash patterns */
	FskFixed				dashPhase;			/**< The starting phase */
} FskColorSource;


/* The following data structures have been designed to overlap as best as possible. */


/** Constant color source. */
typedef struct FskColorSourceConstant {
	FskColorSource			colorSource;		/**< Type = kFskColorSourceTypeConstant. */
	FskColorRGBARecord		color;				/**< Constant color specification. */
} FskColorSourceConstant;

/** Convenience method to initialize a constant color source.
 *
 *	\param[out]	s	the color source to be initialized.
 *	\param[in]	r	the red   component of the color.
 *	\param[in]	g	the green component of the color.
 *	\param[in]	b	the blue  component of the color.
 *	\param[in]	a	the alpha component of the color.
 */
#define FskSetBasicConstantColorSource(s, r, g, b, a) do { (s)->colorSource.type = kFskColorSourceTypeConstant; (s)->colorSource.dashCycles = 0; \
															FskColorRGBASet(&((s)->color), (r), (g), (b), (a)); } while(0)

/** Linear gradient color source. */
typedef struct FskColorSourceLinearGradient {
	FskColorSource			colorSource;		/**< Type = kFskColorSourceTypeLinearGradient. */
	UInt32					spreadMethod;		/**< Spread method = { kFskSpreadPad, kFskSpreadRepeat, kFskSpreadReflect }. */
	FskFixedMatrix3x2		*gradientMatrix;	/**< The gradient transformation matrix, in 16.16 fixed point. Can be NULL. */
	FskGradientStop			*gradientStops;		/**< Pointer to "numStops" FskGradientStops: breakpoints for a piecewise-linear gradient specification. */
	UInt32					numStops;			/**< The number of breakpoints for a piecewise-linear gradient specification (<= 1 reduces to constant fill). */
	SInt32					gradientFracBits;	/**< The number of fractional bits used to specify the gradientVector. */
	FskFixedPoint2D			gradientVector[2];	/**< The coordinate system used for specifying the offset in the FskGradientStop, using gradientFracBits fractional bits. */
} FskColorSourceLinearGradient;


/** Radial gradient color source. */
typedef struct FskColorSourceRadialGradient {
	FskColorSource			colorSource;		/**< Type = kFskColorSourceTypeRadialGradient. */
	UInt32					spreadMethod;		/**< Spread method = { kFskSpreadPad, kFskSpreadRepeat, kFskSpreadReflect }. */
	FskFixedMatrix3x2		*gradientMatrix;	/**< The gradient transformation matrix, in 16.16 fixed point. Can be NULL. */
	FskGradientStop			*gradientStops;		/**< Pointer to "numStops" FskGradientStops: breakpoints for a piecewise-linear gradient specification. */
	UInt32					numStops;			/**< Number of breakpoints for a piecewise-linear gradient specification (<= 1 reduces to constant fill). */
	SInt32					gradientFracBits;	/**< The number of fractional bits used to specify the center, focus, and radius. */
	FskFixedPoint2D			focus;				/**< Focus,  using gradientFracBits fractional bits. */
	FskFixedPoint2D			center;				/**< Center, using gradientFracBits fractional bits. */
	FskFixed				focalRadius;		/**< Inner radius, using gradientFracBits fractional bits. */
	FskFixed				radius;				/**< Radius, using gradientFracBits fractional bits. */
} FskColorSourceRadialGradient;


/** Texture color source. */
typedef struct FskColorSourceTexture {
	FskColorSource			colorSource;		/**< Type = kFskColorSourceTypeTexture */
	UInt32					spreadMethod;		/**<  kFskSpreadPad, kFskSpreadRepeat, kFskSpreadReflect */
	const FskFixedMatrix3x2	*textureFrame;		/**<  { [Xu Yu], [Xv Yv], [X0, Y0] }, maps (u,v) into (x,y) */
	FskConstBitmap			texture;
} FskColorSourceTexture;


/** Procedural color source. */
typedef struct FskColorSourceProcedure {
	FskColorSource			colorSource;		/**< Type = kFskColorSourceTypeProcedure */
	UInt32					unused;				/**< Inserted to align the next two pointers with pointers in the gradients */
	void					(*colorSourceProc)(void *userData, const FskFixedPoint2D *uv, FskColorRGB color);	/**< The colorsource proc.
												\param[in] userData shader-specific data. \param[in] uv the (u,v) coordinates. \param[out] color the resultant color. */
	void					*userData;			/**< The shader-specific data. */
} FskColorSourceProcedure;

typedef struct FskColorSourceConstantRGB {
	FskColorSource			colorSource;		/**< Type = kFskColorSourceTypeConstant. */
	FskColorRGBRecord		color;				/**< Constant color specification. */
} FskColorSourceConstantRGB;


typedef union FskColorSourceUnion {
	FskColorSource						so;
	FskColorSourceConstant				cn;
	FskColorSourceLinearGradient		lg;
	FskColorSourceRadialGradient		rg;
	FskColorSourceTexture				tx;
	FskColorSourceProcedure				pr;
	FskColorSourceConstantRGB			rgb;
} FskColorSourceUnion;


/*
 * ColorSource Constructors.
 */

/** Constant color source constructor.
 *
 *	\param[in]	dashCycles	The number of cycles needed to specify a dash pattern.
 *	\return		a new constant color source, initialized to black, with dashes are all zero.
 */
FskColorSourceConstant*				FskNewColorSourceConstant(UInt32 dashCycles);

/** Linear gradient color source constructor.
 *
 *	\param[in]	numStops	The number of stops needed to specify the gradient.
 *	\param[in]	dashCycles	The number of cycles needed to specify a dash pattern.
 *	\return		a new linear gradient color source, initialized to black, with dashes all zero.
 */
FskColorSourceLinearGradient*		FskNewColorSourceLinearGradient(UInt32 numStops, UInt32 dashCycles);

/** Radial gradient color source constructor.
 *
 *	\param[in]	numStops	The number of stops needed to specify the gradient.
 *	\param[in]	dashCycles	The number of cycles needed to specify a dash pattern.
 *	\return		a new radial gradient color source, initialized to black, with dashes all zero.
 */
FskColorSourceRadialGradient*		FskNewColorSourceRadialGradient(UInt32 numStops, UInt32 dashCycles);

/** Texture color source constructor.
 *
 *	\param[in]	dashCycles	The number of cycles needed to specify a dash pattern.
 *	\return		a new texture color source, initialized to black, with dashes all zero.
 */
FskColorSourceTexture*				FskNewColorSourceTexture(UInt32 dashCycles);

/** Procedural color source constructor.
 *
 *	\param[in]	dashCycles	The number of cycles needed to specify a dash pattern.
 *	\return		a new procedural color source, initialized to black, with dashes all zero.
 */
FskColorSourceProcedure*			FskNewColorSourceProcedure(UInt32 dashCycles);


/*
 * ColorSource Destructors
 */

/** Dispose a color source (base class).
 *	\param[in]	cs	the color source.
 */
void							FskDisposeColorSource(FskColorSource *cs);

/** Dispose a constant color source.
 *	\param[in]	cs	the color source.
 */
void							FskDisposeColorSourceConstant(FskColorSourceConstant *cs);

/** Dispose a linear gradient color source.
 *	\param[in]	cs	the color source.
 */
void							FskDisposeColorSourceLinearGradient(FskColorSourceLinearGradient *cs);

/** Dispose a radial gradient color source.
 *	\param[in]	cs	the color source.
 */
void							FskDisposeColorSourceRadialGradient(FskColorSourceRadialGradient *cs);

/** Dispose a texture color source.
 *	\param[in]	cs	the color source.
 */
void							FskDisposeColorSourceTexture(FskColorSourceTexture *cs);

/** Dispose a procedural color source.
 *	\param[in]	cs	the color source.
 */
void							FskDisposeColorSourceProcedure(FskColorSourceProcedure *cs);

#define FskDisposeColorSourceConstant(cs)				FskDisposeColorSource(&cs->colorSource)	/* Inherit base class destructor */
#define FskDisposeColorSourceLinearGradient(cs)			FskDisposeColorSource(&cs->colorSource)	/* Inherit base class destructor */
#define FskDisposeColorSourceRadialGradient(cs)			FskDisposeColorSource(&cs->colorSource)	/* Inherit base class destructor */
#define FskDisposeColorSourceTexture(cs)				FskDisposeColorSource(&cs->colorSource)	/* Inherit base class destructor */
#define FskDisposeColorSourceProcedure(cs)				FskDisposeColorSource(&cs->colorSource)	/* Inherit base class destructor */


/** ColorSource Transformations.
 *
 *	\param[in]		cs0		The original color source.
 *	\param[in]		M0		The geometry transformation matrix.
 *	\param[in,out]	M1		A new transformation matrix to be provided to the transformed color source.
 *	\param[in,out]	cs1		Storage for the new color source if needed.
 *	\param[out]		newDash	A place to store a new dash if needed; caller needs to dispose.
 *	\return			the transformed color source, stored in either cs0 or cs1.
 */
const FskColorSource*
FskTransformColorSource(
	const FskColorSource	*cs0,
	const FskFixedMatrix3x2	*M0,
	FskFixedMatrix3x2		*M1,
	FskColorSourceUnion		*cs1,
	FskFixed				**newDash
);


/** Set the gradient matrix from an array of floating point numbers; bounds can be NULL.
 *	\param[out]	to		The matrix to be set.
 *	\param[in]	fr		The floating point matrix.
 *	\param[in]	bounds	The bounds of coordinates expected.
 */
FskAPI(void)	FskSetGradientMatrixFromFloatArray(FskFixedMatrix3x2 *to, const double *fr, FskConstFixedRectangle bounds);


/*				-------- procedure --------	------------------------ geometry --------------------------	-------------------------------------- port / context -------------------------------------- */

/** Fill a polygon.
 *	\param[in]		nPts		The number of points on the polygon.
 *	\param[in]		pts			The points (vertices) of the polygon.
 *	\param[in]		fillColor	Fill the polygon using this color source.
 *	\param[in]		fillRule	The rule to be used for filling the polygon.
 *	\param[in]		matrix		A matrix to be used for transforming the polygon. Can be NULL.
 *	\param[in]		quality		The quality to be use when rendering.
 *	\param[in]		clipRect	The rectangle to which all filling is to be constrained. Can be NULL.
 *	\param[in,out]	dstBM		The bitmap to which the polygon will be drawn.
 *	\return			kFskErrNone	if there were no errors.
 */
FskAPI(FskErr)	FskFillPolygon(				UInt32 nPts, const FskFixedPoint2D *pts,
																											const FskColorSource *fillColor, SInt32 fillRule,
																											const FskFixedMatrix3x2 *matrix, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame (stroke) a polygon.
 *	\param[in]		nPts			The number of points on the polygon.
 *	\param[in]		pts				The points (vertices) of the polygon.
 *	\param[in]		strokeWidth		The width of the lines to be drawn, in pixels.
 *	\param[in]		jointSharpness	The sharpness that should be used for the joints between edges of the polygon.
 *	\param[in]		frameColor		Frame the polygon using this color source.
 *	\param[in]		matrix			A matrix to be used for transforming the polygon. Can be NULL.
 *	\param[in]		quality			The quality to be use when rendering.
 *	\param[in]		clipRect		The rectangle to which all filling is to be constrained. Can be NULL.
 *	\param[in,out]	dstBM			The bitmap to which the polygon will be drawn.
 *	\return			kFskErrNone	if there were no errors.
 */
FskAPI(FskErr)	FskFramePolygon(			UInt32 nPts, const FskFixedPoint2D *pts,
																											FskFixed strokeWidth, FskFixed jointSharpness, const FskColorSource *frameColor,
																											const FskFixedMatrix3x2 *matrix, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame (stroke) and fill a polygon.
 *	\param[in]		nPts			The number of points on the polygon.
 *	\param[in]		pts				The points (vertices) of the polygon.
 *	\param[in]		strokeWidth		The width of the lines to be drawn, in pixels.
 *	\param[in]		jointSharpness	The sharpness that should be used for the joints between edges of the polygon.
 *	\param[in]		frameColor		Frame the polygon using this color source.
 *	\param[in]		fillColor		Fill the polygon using this color source.
 *	\param[in]		fillRule		The rule to be used for filling the polygon.
 *	\param[in]		matrix			A matrix to be used for transforming the polygon. Can be NULL.
 *	\param[in]		quality			The quality to be use when rendering.
 *	\param[in]		clipRect		The rectangle to which all filling is to be constrained. Can be NULL.
 *	\param[in,out]	dstBM			The bitmap to which the polygon will be drawn.
 *	\return			kFskErrNone	if there were no errors.
 */
FskAPI(FskErr)	FskFrameFillPolygon(		UInt32 nPts, const FskFixedPoint2D *pts,
																											FskFixed strokeWidth, FskFixed jointSharpness, const FskColorSource *frameColor, const FskColorSource *fillColor, SInt32 fillRule,
																											const FskFixedMatrix3x2 *matrix, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Fill a polygon with multiple contours.
 *	\param[in]		nContours	The number of contours in the polygon.
 *	\param[in]		nPts		The number of points for each contour of the polygon.
 *	\param[in]		pts			The points (vertices) of the polygon.
 *	\param[in]		fillColor	Fill the polygon using this color source.
 *	\param[in]		fillRule	The rule to be used for filling the polygon.
 *	\param[in]		matrix		A matrix to be used for transforming the polygon. Can be NULL.
 *	\param[in]		quality		The quality to be use when rendering.
 *	\param[in]		clipRect	The rectangle to which all filling is to be constrained. Can be NULL.
 *	\param[in,out]	dstBM		The bitmap to which the polygon will be drawn.
 *	\return			kFskErrNone	if there were no errors.
 */
FskAPI(FskErr)	FskFillPolygonContours(		UInt32 nContours, const UInt32 *nPts, const FskFixedPoint2D *pts,
																											const FskColorSource *fillColor, SInt32 fillRule,
																											const FskFixedMatrix3x2 *matrix, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame (stroke) a polygon with multiple contours.
 *	\param[in]		nContours	The number of contours in the polygon.
 *	\param[in]		nPts			The number of points on the polygon.
 *	\param[in]		pts				The points (vertices) of the polygon.
 *	\param[in]		strokeWidth		The width of the lines to be drawn, in pixels.
 *	\param[in]		jointSharpness	The sharpness that should be used for the joints between edges of the polygon.
 *	\param[in]		frameColor		Frame the polygon using this color source.
 *	\param[in]		matrix			A matrix to be used for transforming the polygon. Can be NULL.
 *	\param[in]		quality			The quality to be use when rendering.
 *	\param[in]		clipRect		The rectangle to which all filling is to be constrained. Can be NULL.
 *	\param[in,out]	dstBM			The bitmap to which the polygon will be drawn.
 *	\return			kFskErrNone		if there were no errors.
 */
FskAPI(FskErr)	FskFramePolygonContours(	UInt32 nContours, const UInt32 *nPts, const FskFixedPoint2D *pts,
																											FskFixed strokeWidth, FskFixed jointSharpness, const FskColorSource *frameColor,
																											const FskFixedMatrix3x2 *matrix, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame (stroke) and fill a polygon with multiple contours.
 *	\param[in]		nContours	The number of contours in the polygon.
 *	\param[in]		nPts			The number of points on the polygon.
 *	\param[in]		pts				The points (vertices) of the polygon.
 *	\param[in]		strokeWidth		The width of the lines to be drawn, in pixels.
 *	\param[in]		jointSharpness	The sharpness that should be used for the joints between edges of the polygon.
 *	\param[in]		frameColor		Frame the polygon using this color source.
 *	\param[in]		fillColor		Fill the polygon using this color source.
 *	\param[in]		fillRule		The rule to be used for filling the polygon.
 *	\param[in]		matrix			A matrix to be used for transforming the polygon. Can be NULL.
 *	\param[in]		quality			The quality to be use when rendering.
 *	\param[in]		clipRect		The rectangle to which all filling is to be constrained. Can be NULL.
 *	\param[in,out]	dstBM			The bitmap to which the polygon will be drawn.
 *	\return			kFskErrNone		if there were no errors.
 */
FskAPI(FskErr)	FskFrameFillPolygonContours(UInt32 nContours, const UInt32 *nPts, const FskFixedPoint2D *pts,
																											FskFixed strokeWidth, FskFixed jointSharpness, const FskColorSource *frameColor, const FskColorSource *fillColor, SInt32 fillRule,
																											const FskFixedMatrix3x2 *matrix, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);


/** Frame (stroke) a polyline.
 *	This differs from FskFramePolygon() in that the polyline is not usually closed.
 *	\param[in]		nPts			The number of points on the polygon.
 *	\param[in]		pts				The points (vertices) of the polygon.
 *	\param[in]		strokeWidth		The width of the lines to be drawn, in pixels.
 *	\param[in]		jointSharpness	The sharpness that should be used for the joints between edges of the polygon.
 *	\param[in]		frameColor		Frame the polygon using this color source.
 *	\param[in]		matrix			A matrix to be used for transforming the polygon. Can be NULL.
 *	\param[in]		quality			The quality to be use when rendering.
 *	\param[in]		clipRect		The rectangle to which all filling is to be constrained. Can be NULL.
 *	\param[in,out]	dstBM			The bitmap to which the polygon will be drawn.
 *	\return			kFskErrNone		if there were no errors.
 */
FskAPI(FskErr)	FskFramePolyLine(			UInt32 nPts, const FskFixedPoint2D *pts,
																											FskFixed strokeWidth, FskFixed jointSharpness, UInt32 endCaps, const FskColorSource *frameColor,
																											const FskFixedMatrix3x2 *matrix, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame (stroke) a polyline with multiple contours.
 *	\param[in]		nContours	The number of contours in the polygon.
 *	\param[in]		nPts			The number of points on the polygon.
 *	\param[in]		pts				The points (vertices) of the polygon.
 *	\param[in]		strokeWidth		The width of the lines to be drawn, in pixels.
 *	\param[in]		jointSharpness	The sharpness that should be used for the joints between edges of the polygon.
 *	\param[in]		frameColor		Frame the polygon using this color source.
 *	\param[in]		matrix			A matrix to be used for transforming the polygon. Can be NULL.
 *	\param[in]		quality			The quality to be use when rendering.
 *	\param[in]		clipRect		The rectangle to which all filling is to be constrained. Can be NULL.
 *	\param[in,out]	dstBM			The bitmap to which the polygon will be drawn.
 *	\return			kFskErrNone		if there were no errors.
 */
FskAPI(FskErr)	FskFramePolylineContours(	UInt32 nContours, const UInt32 *nPts, const FskFixedPoint2D *pts,
																											FskFixed strokeWidth, FskFixed jointSharpness, UInt32 endCaps, const FskColorSource *frameColor,
																											const FskFixedMatrix3x2 *matrix, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);

/** Frame (stroke) a single line.
 *	\param[in]		p0		The coordinates of the first  point.
 *	\param[in]		p1		The coordinates of the second point.
 *	\param[in]		strokeWidth		The width of the lines to be drawn, in pixels.
 *	\param[in]		jointSharpness	The sharpness that should be used for the joints between edges of the polygon.
 *	\param[in]		frameColor		Frame the polygon using this color source.
 *	\param[in]		matrix			A matrix to be used for transforming the polygon. Can be NULL.
 *	\param[in]		quality			The quality to be use when rendering.
 *	\param[in]		clipRect		The rectangle to which all filling is to be constrained. Can be NULL.
 *	\param[in,out]	dstBM			The bitmap to which the polygon will be drawn.
 *	\return			kFskErrNone		if there were no errors.
 */
FskAPI(FskErr)	FskFrameLine(				const FskFixedPoint2D *p0, const FskFixedPoint2D *p1,
																											FskFixed strokeWidth, FskFixed jointSharpness, UInt32 endCaps, const FskColorSource *frameColor,
																											const FskFixedMatrix3x2 *matrix, UInt32 quality, FskConstRectangle clipRect, FskBitmap dstBM);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKPOLYGON__ */


