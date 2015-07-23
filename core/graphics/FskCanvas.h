/*
 *    Copyright (C) 2010-2015 Marvell International Ltd.
 *    Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
/**
	\file	FskCanvas.h
	\brief	Vector graphics API tuned to HTML5's Canvas 2d.
*/
#ifndef __FSKCANVAS2D__
#define __FSKCANVAS2D__

#include "FskBitmap.h"
#include "FskPolygon.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/********************************************************************************
 * Structures and Constants
 ********************************************************************************/

/* Many of these enums are duplicated and identical to those in FskPolygon.h. */


/** Line cap style. */
enum {
	kFskCanvas2dLineCapRound	= 0,			/**< Round end caps. */
	kFskCanvas2dLineCapSquare	= 1,			/**< Square end caps, circumscribing the round end caps. */
	kFskCanvas2dLineCapButt		= 2,			/**< Butt end caps: like square, except truncated through the endpoint. */
	kFskCanvas2dLineCapDefault	= kFskCanvas2dLineCapButt		/**< The default end caps for Canvas 2d is kFskCanvas2dLineCapButt. */
};


/** Line join style. */
enum {
	kFskCanvas2dLineJoinRound	= 0,			/**< Round joins between line segments. */
	kFskCanvas2dLineJoinBevel	= 1,			/**< Bevelled joins between line segments. */
	kFskCanvas2dLineJoinMiter	= 2,			/**< Mitered joins between like segments. Subject to the miter limit. */
	kFskCanvas2dLineJoinDefault	= kFskCanvas2dLineJoinMiter		/**< The default join for Canvas 2d is kFskCanvas2dLineJoinMiter, with a limit of 11.5 degrees. */
};


/** Text alignment. */
enum {
	kFskCanvas2dTextAlignStart		= 0,		/**< Align the text at the beginning of the string, normally at the left except for right-to-left scripts. */
	kFskCanvas2dTextAlignCenter		= 1,		/**< Center the text. */
	kFskCanvas2dTextAlignEnd		= 2,		/**< Align the text at the end of the string, normally at the right except for right-to-left scripts. */
	kFskCanvas2dTextAlignLeft		= 3,		/**< Align the text at the left. */
	kFskCanvas2dTextAlignRight		= 4,		/**< Align the text at the right. */
	kFskCanvas2dTextAlignDefault	= kFskCanvas2dTextAlignStart	/**< The default Canvas 2d text alignment is kFskCanvas2dTextAlignStart. */
};


/** Baseline alignment */
enum {
	kFskCanvas2dTextBaselineAlphabetic	= 0,	/**< Alphabetic baseline. */
	kFskCanvas2dTextBaselineIdeographic	= 1,	/**< Ideographic baseline. */
	kFskCanvas2dTextBaselineTop			= 2,	/**< Baseline at the top. */
	kFskCanvas2dTextBaselineHanging		= 3,	/**< Hanging baseline. */
	kFskCanvas2dTextBaselineMiddle		= 4,	/**< Baseline in the middle. */
	kFskCanvas2dTextBaselineBottom		= 5,	/**< Baseline at the bottom. */
	kFskCanvas2dTextBaselineDefault		= kFskCanvas2dTextBaselineAlphabetic	/**< The default baseline alignment is kFskCanvas2dTextBaselineAlphabetic. */
};


/** Fill rule.
 *	This is an extension beyond that required in the original specification,
 *	which required only the non-zero or winding-number fill.
 */
enum {
	kFskCanvas2dFillRuleWindingNumber	= 0,	/**< Winding number or non-zero fill (Canvas 2d default). */
	kFskCanvas2dFillRuleParity			= 1		/**< Parity or even-odd fill. */
};


/** Porter-Duff composition modes. */
enum {
	kFskCanvas2dCompositePreSourceOver		= 12,	/**< Premultiplied Porter-Duff source over destination. */
	kFskCanvas2dCompositePreDestinationOver	= 13,	/**< Premultiplied Porter-Duff destination over source. */
	kFskCanvas2dCompositePreSourceIn		= 14, 	/**< Premultiplied Porter-Duff source in destination. */
	kFskCanvas2dCompositePreDestinationIn	= 15,	/**< Premultiplied Porter-Duff destination in source. */
	kFskCanvas2dCompositePreSourceOut		= 16,	/**< Premultiplied Porter-Duff source out of destination. */
	kFskCanvas2dCompositePreDestinationOut	= 17,	/**< Premultiplied Porter-Duff destination out of source. */
	kFskCanvas2dCompositePreSourceAtop		= 18,	/**< Premultiplied Porter-Duff source atop destination. */
	kFskCanvas2dCompositePreDestinationAtop	= 19,	/**< Premultiplied Porter-Duff destination atop source. */
	kFskCanvas2dCompositePreLighter			= 20,	/**< Premultiplied Porter-Duff lighter (add). */
	kFskCanvas2dCompositePreXor				= 21	/**< Premultiplied Porter-Duff exclusive-OR. */
};


/** Text direction. */
enum {
	kFskCanvas2dTextDirectionLeftToRight	= +1,
	kFskCanvas2dTextDirectionRightToLeft	= -1,
	kFskCanvas2dTextDirectionInherit		= 0
};


/** Pattern repetition. */
enum {
	kFskCanvas2dPatternRepeatNone	= 0,		/**< The pattern is not to be repeated  outside of its primary domain. */
	kFskCanvas2dPatternRepeatX		= 2<<0,		/**< The pattern is to be repeated in X outside of its primary domain. */
	kFskCanvas2dPatternRepeatY		= 2<<2,		/**< The pattern is to be repeated in Y outside of its primary domain. */
	kFskCanvas2dPatternRepeat		= kFskCanvas2dPatternRepeatX|kFskCanvas2dPatternRepeatY	/**< The pattern is to be repeated in both X and Y outside of its primary domain. */
};



/** An array of pixels, in RGBA order */
typedef struct FskCanvas2dPixelArrayRecord {
	UInt32		length;							/**< The number of bytes in this pixel array record, necessarily divisible by 4. */
	UInt8		*bytes;							/**< The pixel data. Can be safely cast to FskColorRGBA. */
} FskCanvas2dPixelArrayRecord,
 *FskCanvas2dPixelArray;						/**< A pixel array object. */

/** A structure to hold image data. */
typedef struct FskCanvas2dImageDataRecord {
	UInt32						width;			/**< The width  of the image data. */
	UInt32						height;			/**< The height of the image data. */
	FskCanvas2dPixelArrayRecord	data;			/**< The pixel data. */
} FskCanvas2dImageDataRecord,
 *FskCanvas2dImageData;							/**< An image data object. */
typedef const struct FskCanvas2dImageDataRecord *FskConstCanvas2dImageData;	/**< A read-only image data object. */


/** Gradient stop specification. */
typedef struct FskCanvas2dGradientStop {
	double				offset;					/**< The offset (in [0, 1]) of the stop, relative to the gradient's coordinate system. */
	FskColorRGBARecord	color;					/**< The color to be used at the associated offset. */
} FskCanvas2dGradientStop;


/* Forward declarations */

/**	\typedef	FskCanvasRecord
 *	\brief		A structure to contain the Canvas state.
 */
/**	\typedef	FskCanvas
 *	\brief		A Canvas state object.
 */
/**	\typedef	FskConstCanvas
 *	\brief		A read-only Canvas state object.
 */
/**	\typedef	FskCanvas2dContextRecord
 *	\brief		A structure to contain the Canvas 2d context.
 */
/**	\typedef	FskCanvas2dContext
 *	\brief		A Canvas Canvas 2d context object.
 */
/**	\typedef	FskConstCanvas2dContext
 *	\brief		A read-only Canvas 2d context object.
 */
/**	\typedef	FskVideoRecord
 *	\brief		A structure to contain a Canvas video. Currently undefined.
 */
/**	\typedef	FskVideo
 *	\brief		A Canvas video object.
 */

struct FskCanvasRecord;				typedef struct FskCanvasRecord				FskCanvasRecord,			*FskCanvas;				typedef const struct FskCanvasRecord			*FskConstCanvas;
struct FskCanvas2dContextRecord;	typedef struct FskCanvas2dContextRecord		FskCanvas2dContextRecord,	*FskCanvas2dContext;	typedef const struct FskCanvas2dContextRecord	*FskConstCanvas2dContext;
struct FskVideoRecord;				typedef struct FskVideoRecord				FskVideoRecord,				*FskVideo;
struct FskColorSource;
struct FskGrowableStorageRecord;	typedef struct FskGrowableStorageRecord		FskCanvas2dPathRecord,		*FskCanvas2dPath;			typedef const struct FskGrowableStorageRecord	*FskConstCanvas2dPath;


/** The encapsulation of a double-precision floating-point 3x2 matrix. */
typedef struct FskCanvasMatrix3x2d {
	double M[3][2];										/**< The matrix itself. */
} FskCanvasMatrix3x2d;									/**< The encapsulation of a double-precision floating-point 3x2 matrix. */



/********************************************************************************
 * API
 ********************************************************************************/


/** Canvas constructor.
 *	\param[in]	width		The desired width  of the canvas.
 *	\param[in]	height		The desired height of the Canvas.
 *	\param[in]	pixelFormat	The desired pixel format of the Canvas. Use 0 for the default pixel format.
 *	\param[out]	cnv			The resultant new Canvas 2d.
 *	\return		kFskErrNone	if the canvas was created successfully.
 */
FskAPI(FskErr)	FskCanvasNew(UInt32 width, UInt32 height, FskBitmapFormatEnum pixelFormat, FskCanvas *cnv);


/** Canvas constructor.
 *	\param[in]	bm			The bitmap to be used for rendering in the canvas.
 *	\param[out]	cnv			The resultant new Canvas 2d.
 *	\return		kFskErrNone	if the canvas was created successfully.
 *	\note		If calling FskCanvasNewFromBitmap() with a GL-accelerated bitmap, it is necessary to call
 *				FskGLPortSetPersistent(bm->glPort, true) explicitly in order to get the same behavior as with FskCanvasNew().
 */
FskAPI(FskErr)	FskCanvasNewFromBitmap(FskBitmap bm, FskCanvas *cnv);


/** Canvas destructor.
 *	\param[in]	cnv	The Canvas to be disposed.
 */
FskAPI(void)	FskCanvasDispose(FskCanvas cnv);


/** Clear the canvas to the specified color.
 *	This is an enhancement beyond that of the Canvas 2d specification.
 *	Colors are specified in the straight alpha convention, but the canvas bitmap contains premultiplied colors.
 *	As a result, the color written to the canvas will be
 *		{ r*a, g*a, b*a, a }
 *	\param[in]	cnv		The Canvas to be cleared.
 *	\param[in]	r		The  red  component of the clear color.
 *	\param[in]	g		The green component of the clear color.
 *	\param[in]	b		The blue  component of the clear color.
 *	\param[in]	a		The     opacity     of the clear color.
 */
FskAPI(void)	FskCanvasClear(FskCanvas cnv, UInt8 r, UInt8 g, UInt8 b, UInt8 a);


/** Get a pointer to the Canvas Bitmap.
 *	This is an enhancement that is not in the Canvas 2d spec.
 *	This is much more efficient than FskCanvas2dGetImageData().
 *	\param[in]	cnv	The canvas.
 *	\return		A pointer to the canvas bitmap. Note that this is read-only.
 */
FskAPI(FskConstBitmap)	FskGetCanvasBitmap(FskCanvas cnv);


/** Set the bitmap to be used for rendering canvas.
 *	This has the side-effect of resetting all of the canvas state.
 *	\param[in]	cnv		The canvas, whose bitmap is to be set.
 *	\param[in]	bm		The new bitmap to be used for rendering.
 *	\return		kFskErrNone					if the bitmap was set successfully.
 *	\return		kFskErrUnsupportedPixelType	if the pixel type is not supported.
 *	\return		kFskErrUnimplemented		if the pixel type is expected to be supported, but curently unimplemented.
 *	\note		If calling FskSetCanvasBitmap() with a GL-accelerated bitmap, it is necessary to call
 *				FskGLPortSetPersistent(bm->glPort, true) explicitly in order to get the same behavior as with FskCanvasNew().
 */
FskAPI(FskErr)	FskSetCanvasBitmap(FskCanvas cnv, FskBitmap bm);


/** Check whether the canvas context is accelerated with OpenGL.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		true	if the Canvas context is accelerated; false otherwise.
 */
FskAPI(Boolean)	FskCanvasIsAccelerated(FskConstCanvas2dContext ctx);


/** Returns a data: URL of the form "data:image/png;base64,dataComposedOfUrlCharacters".
 *	\param[in]	cnv		The canvas.
 *	\param[in]	type	The type of the image desired; if NULL is supplied, then "image/png" is used.
 *						Possibilities are "image/png", "image/jpeg", and "image/svg+xml".
 *						Only "image/png" is required by the Canvas specification.
 *	\param[in]	quality	The desired quality of the encoded image. This is ignored for everything
 *						except for "image/jpeg". Quality varies from 0 to 1.
 *	\param[out]	dataURL	The resultant data:url-encoded image, newly allocated. Dispose with FskMemPtrDispose();
 *	\return		kFskErrNone	if the data:URL was successfully created.
 *	\bug		We only implement "image/jpeg", not PNG nor SVG.
 *	\warning	This can generate strings that are longer than 1024, which is the suggested maximum length of HTML attributes.
 *	\todo		Implement PNG and SVG.
 */
FskAPI(FskErr)	FskCanvasToDataURL(FskCanvas cnv, const char *type, float quality, char **dataURL);


/** Get the context from a canvas.
 *	\param[in]	cnv		The canvas.
 *	\return		The context of the 2d Canvas.
 */
FskAPI(FskCanvas2dContext)	FskCanvasGet2dContext(FskCanvas cnv);


/** Get a back-reference to the canvas from its context.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		A back-reference to this context's canvas.
 */
FskAPI(FskCanvas)	FskCanvas2dGetCanvas(FskCanvas2dContext ctx);


/** Push state onto the state stack.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return	kFskErrNone	if the state was saved successfully.
 */
FskAPI(FskErr)	FskCanvas2dSave(FskCanvas2dContext ctx);


/** Pop the state stack and restore state to its value prior to the the last Save.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return	kFskErrNone	if the state was restored successfully.
 */
FskAPI(FskErr)	FskCanvas2dRestore(FskCanvas2dContext ctx);


/** Flush the drawing to the bitmap.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return	kFskErrNone	if the operation was executed successfully.
 */
FskAPI(FskErr)	FskCanvas2dCommit(FskCanvas2dContext ctx);


/** Get quality of the rendering.
 *	This is an enhancement beyond that in the Canvas 2d specification.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		0		if low  quality has been specified.
 *	\return		1		if high quality has been specified (default).
 */
FskAPI(UInt32)	FskCanvas2dGetQuality(FskConstCanvas2dContext ctx);


/** Set quality of the rendering.
 *	This is an enhancement beyond that in the Canvas 2d specification.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	quality	The desired quality: 0 for low quality, 1 for high quality.
 *						The default is high quality.
 */
FskAPI(void)	FskCanvas2dSetQuality(FskCanvas2dContext ctx, UInt32 quality);


/********************************************************************************
 * Transforms
 ********************************************************************************/


/** Scale the current transform.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	x		The scale to be applied to the X dimension.
 *	\param[in]	y		The scale to be applied to the Y dimension.
 */
FskAPI(void)	FskCanvas2dScale(FskCanvas2dContext ctx, double x, double y);


/** Rotate the current transform.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	angle	The angle by which to rotate.
 *						This is specified in radians measured from the X-axis toward the Y-axis.
 */
FskAPI(void)	FskCanvas2dRotate(FskCanvas2dContext ctx, double angle);


/** Translate the current transform.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	x		The translation to be applied to the X dimension.
 *	\param[in]	y		The translation to be applied to the Y dimension.
 */
FskAPI(void)	FskCanvas2dTranslate(FskCanvas2dContext ctx, double x, double y);


/** Concatenate an affine transform to the current transform.
 *	Concatenated as:\n
	\verbatim
		[a  b  0]   [t00  t01  0]
		[c  d  0]   [t10  t11  0]
		[e  f  1]   [t20  t21  1]
	\endverbatim
 * or, equivalently,
	\verbatim
		[u00  u01  u02]   [a  c  e]
		[u10  u11  u12]   [b  d  f]
		[ 0    0    1 ]   [0  0  1]
	\endverbatim
 *	Some call this "pre-" or "reverse" concatenation, and seems to be more natural in a procedural environment.
 *	It performs increasingly local transformations.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	a		the isotropic X scale.
 *	\param[in]	b		the X-to-Y skew.
 *	\param[in]	c		the Y-toX skew.
 *	\param[in]	d		the isotropic Y scale.
 *	\param[in]	e		the X-translation.
 *	\param[in]	f		the Y-translation.
 */
FskAPI(void)	FskCanvas2dTransform(FskCanvas2dContext ctx, double a, double b, double c, double d, double e, double f);


/** Set the current transform.
 *	The transform is set directly, not concatenated.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	a		the isotropic X scale.
 *	\param[in]	b		the X-to-Y skew.
 *	\param[in]	c		the Y-toX skew.
 *	\param[in]	d		the isotropic Y scale.
 *	\param[in]	e		the X-translation.
 *	\param[in]	f		the Y-translation.
 */
FskAPI(void)	FskCanvas2dSetTransform(FskCanvas2dContext ctx, double a, double b, double c, double d, double e, double f);


/** Get the current transform.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[out]	transform	The current transform.
 *	\note	Calling FskCanvas2dSetTransform() then FskCanvas2dGetTransform()
 *			may result in different values due to quantization in the implementation.
 */
FskAPI(void)	FskCanvas2dGetTransform(FskCanvas2dContext ctx, double transform[6]);


/** Set the current device transform.
 *	This is concatenated on the global side, i.e. post-concatenanation.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	a		the isotropic X scale.
 *	\param[in]	b		the X-to-Y skew.
 *	\param[in]	c		the Y-toX skew.
 *	\param[in]	d		the isotropic Y scale.
 *	\param[in]	e		the X-translation.
 *	\param[in]	f		the Y-translation.
 *	\return		kFskErrNone				if the system transform was set successfully.
 *	\return		kFskErrUnimplemented	if the class of transformation is not supported.
 *	\note		The intent is to use this to support current isotropic scaling by 1.0X, 1.5X, and 2.0X,
 *				and possibly rotation by multiples of 90 degrees.
 *	\note		This is an extension to the Canvas API, but seems to be anticipated in the spec.
 */
FskAPI(void)	FskCanvas2dSetDeviceTransform(FskCanvas2dContext ctx, double a, double b, double c, double d, double e, double f);


/** Get the current device transform.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[out]	transform	The current transform.
 */
FskAPI(void)	FskCanvas2dGetDeviceTransform(FskCanvas2dContext ctx, double transform[6]);


/********************************************************************************
 * Compositing
 ********************************************************************************/


/** Get the global alpha.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\return		The global alpha.
 *				The default is global alpha = 1.0, i.e. totally opaque.
 */
FskAPI(double)	FskCanvas2dGetGlobalAlpha(FskConstCanvas2dContext ctx);


/** Set the global alpha.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	alpha		The desired global alpha.
 */
FskAPI(void)	FskCanvas2dSetGlobalAlpha(FskCanvas2dContext ctx, double alpha);


/** Get the global composition operation.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\return		The global composition operation.
 *				The default is  kFskCanvas2dCompositePreSourceOver.
 */
FskAPI(UInt32)	FskCanvas2dGetGlobalCompositeOperation(FskConstCanvas2dContext ctx);


/** Set the global composition operation.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	op			The desired Porter-Duff composition operation.
 */
FskAPI(void)	FskCanvas2dSetGlobalCompositeOperation(FskCanvas2dContext ctx, UInt32 op);


/********************************************************************************
 * Colors and styles
 ********************************************************************************/

/** The maximum number of color stops accommodatde for linear and radial gradients. */
#define kCanvas2DMaxGradientStops			6


/** Get the current stroke style as a color source.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\return		A pointer to the current stroke color source.
 *				The default is opaque black.
 *	\see		FskColorSource
 *	\see		FskPolygon.h
 */
FskAPI(const FskColorSource*)	FskCanvas2dGetStrokeStyle(FskConstCanvas2dContext ctx);


/** Set the current stroke style as a color source.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	cs			The desired color source to be used for stroking paths.
 *	\bug		In this implementation, dashes are not accommodated, because they are not part of the canvas spec.
 *	\bug		In this implementation, gradients are restricted to at most kCanvas2DMaxGradientStops color stops.
 *	\todo		Accommodate an arbitrary number of gradient color stops.
 *	\see		FskColorSource
 *	\see		FskPolygon.h
 */
FskAPI(void)	FskCanvas2dSetStrokeStyle(FskCanvas2dContext ctx, const FskColorSource *cs);


/** Get the current fill style as a color source.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\return		A pointer to the current fill color source.
 *				The default is opaque black.
 *	\see		FskColorSource
 *	\see		FskPolygon.h
 */
FskAPI(const FskColorSource*)	FskCanvas2dGetFillStyle(FskConstCanvas2dContext ctx);


/** Set the current fill style as a color source.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	cs			The desired color source to be used for filling paths.
 *	\bug		In this implementation, dashes are not accommodated, because they are not part of the canvas spec.
 *	\bug		In this implementation, gradients are restricted to at most kCanvas2DMaxGradientStops color stops.
 *	\todo		Accommodate an arbitrary number of gradient color stops.
 *	\see		FskColorSource
 *	\see		FskPolygon.h
 */
FskAPI(void)	FskCanvas2dSetFillStyle(FskCanvas2dContext ctx, const FskColorSource *cs);


/** Convenience interface to set the fill style to a constant color.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	color		Pointer to an FskColorRGBARecord that specifies the desired fill color.
 */
FskAPI(void)	FskCanvas2dSetFillStyleColor(  FskCanvas2dContext ctx, FskConstColorRGBA color);


/** Convenience interface to set the stroke style to a constant color.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	color		Pointer to an FskColorRGBARecord that specifies the desired stroke color.
 */
FskAPI(void)	FskCanvas2dSetStrokeStyleColor(FskCanvas2dContext ctx, FskConstColorRGBA color);


/** Convenience interface to set the fill style to a linear gradient.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	x0			The X-coordinate of the beginning of the linear gradient.
 *	\param[in]	y0			The Y-coordinate of the beginning of the linear gradient.
 *	\param[in]	x1			The X-coordinate of the end of the linear gradient.
 *	\param[in]	y1			The Y-coordinate of the end of the linear gradient.
 *	\param[in]	numStops	The number of stops used to specify the gradient.
 *	\param[in]	stops		The color stops used to define the gradient.
 *	\return		kFskErrNone		If the gradient fill style was successfully set.
 *	\return		kFskErrTooMany	If too many stops were specified.
 *	\note		(x0,y0) and (x1,y1) set up a 1-dimensional coordinate system,
 *				where an offset of 0 corresponds to (x0,y0) and (an offset of 1 corresponds to (x1,y1).
 *				Offsets between 0 and 1 are linearly interpolated between (x0,y0) and (x1,y1).
 *	\bug		Only 4 gradient stops are currently accommodated.
 *	\todo		Accommodate an arbitrary number of gradient stops.
 */
FskAPI(FskErr)	FskCanvas2dSetFillStyleLinearGradient(	FskCanvas2dContext ctx,
														double x0, double y0, double x1, double y1,
														UInt32 numStops, const FskCanvas2dGradientStop *stops);


/** Convenience interface to set the stroke style to a linear gradient.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	x0			The X-coordinate of the beginning of the linear gradient.
 *	\param[in]	y0			The Y-coordinate of the beginning of the linear gradient.
 *	\param[in]	x1			The X-coordinate of the end of the linear gradient.
 *	\param[in]	y1			The Y-coordinate of the end of the linear gradient.
 *	\param[in]	numStops	The number of stops used to specify the gradient.
 *	\param[in]	stops		The color stops used to define the gradient.
 *	\return		kFskErrNone		If the gradient stroke style was successfully set.
 *	\return		kFskErrTooMany	If too many stops were specified.
 *	\note		(x0,y0) and (x1,y1) set up a 1-dimensional coordinate system,
 *				where an offset of 0 corresponds to (x0,y0) and (an offset of 1 corresponds to (x1,y1).
 *				Offsets between 0 and 1 are linearly interpolated between (x0,y0) and (x1,y1).
 *	\bug		Only 4 gradient stops are currently accommodated.
 *	\todo		Accommodate an arbitrary number of gradient stops.
 */
FskAPI(FskErr)	FskCanvas2dSetStrokeStyleLinearGradient(FskCanvas2dContext ctx,
														double x0, double y0, double x1, double y1,
														UInt32 numStops, const FskCanvas2dGradientStop *stops);


/** Convenience interface to set the fill style to a radial gradient.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	x0			The X-coordinate of the focus of the inner circle of the radial gradient.
 *	\param[in]	y0			The Y-coordinate of the focus of the inner circle of the radial gradient.
 *	\param[in]	r0			The radius of the inner circle of the radial gradient.
 *	\param[in]	x1			The X-coordinate of the center of the outer circle of the radial gradient.
 *	\param[in]	y1			The Y-coordinate of the center of the outer circle of the radial gradient.
 *	\param[in]	r1			The radius of the outer circle of the radial gradient.
 *	\param[in]	numStops	The number of stops used to specify the gradient.
 *	\param[in]	stops		The color stops used to define the gradient.
 *	\return		kFskErrNone		If the gradient fill style was successfully set.
 *	\return		kFskErrTooMany	If too many stops were specified.
 *	\note		(x0,y0,r0) and (x1,y1,r1) set up a 1-dimensional coordinate system,
 *				where an offset of 0 corresponds to (x0,y0,r0) and (an offset of 1 corresponds to (x1, y1,r1).
 *				Offsets between 0 and 1 are linearly interpolated between (x0,y0,r0) and (x1,y1,r1).
 *	\bug		r0 is currently ignored, and has an effective value of 0.
 *	\bug		Only 4 gradient stops are currently accommodated.
 *	\todo		Implement r0 != 0.
 *	\todo		Accommodate an arbitrary number of gradient stops.
 */
FskAPI(FskErr)	FskCanvas2dSetFillStyleRadialGradient(	FskCanvas2dContext ctx,
														double x0, double y0, double r0, double x1, double y1, double r1,
														UInt32 numStops, const FskCanvas2dGradientStop *stops);


/** Convenience interface to set the stroke style to a radial gradient.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	x0			The X-coordinate of the focus of the inner circle of the radial gradient.
 *	\param[in]	y0			The Y-coordinate of the focus of the inner circle of the radial gradient.
 *	\param[in]	r0			The radius of the inner circle of the radial gradient.
 *	\param[in]	x1			The X-coordinate of the center of the outer circle of the radial gradient.
 *	\param[in]	y1			The Y-coordinate of the center of the outer circle of the radial gradient.
 *	\param[in]	r1			The radius of the outer circle of the radial gradient.
 *	\param[in]	numStops	The number of stops used to specify the gradient.
 *	\param[in]	stops		The color stops used to define the gradient.
 *	\return		kFskErrNone		If the gradient stroke style was successfully set.
 *	\return		kFskErrTooMany	If too many stops were specified.
 *	\note		(x0,y0,r0) and (x1,y1,r1) set up a 1-dimensional coordinate system,
 *				where an offset of 0 corresponds to (x0,y0,r0) and (an offset of 1 corresponds to (x1, y1,r1).
 *				Offsets between 0 and 1 are linearly interpolated between (x0,y0,r0) and (x1,y1,r1).
 *	\bug		r0 is currently ignored, and has an effective value of 0.
 *	\bug		Only 4 gradient stops are currently accommodated.
 *	\todo		Implement r0 != 0.
 *	\todo		Accommodate an arbitrary number of gradient stops.
 */
FskAPI(FskErr)	FskCanvas2dSetStrokeStyleRadialGradient(FskCanvas2dContext ctx,
														double x0, double y0, double r0, double x1, double y1, double r1,
														UInt32 numStops, const FskCanvas2dGradientStop *stops);


/** Convenience interface to set the fill style to a pattern.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	repetition	What to do outside of the pattern's principal domain:
 *							kFskCanvas2dPatternRepeatNone, kFskCanvas2dPatternRepeatX, kFskCanvas2dPatternRepeatY, kFskCanvas2dPatternRepeat.
 *	\param[in]	pattern		The image to be used as a pattern for filling.
 *	\return		kFskErrNone		If the pattern fill style was successfully set.
 */
FskAPI(FskErr)	FskCanvas2dSetFillStylePattern(  FskCanvas2dContext ctx, UInt32 repetition, FskConstBitmap pattern /*, const FskCanvasMatrix3x2d *M*/);


/** Convenience interface to set the stroke style to a pattern.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	repetition	What to do outside of the pattern's principal domain:
 *							kFskCanvas2dPatternRepeatNone, kFskCanvas2dPatternRepeatX, kFskCanvas2dPatternRepeatY, kFskCanvas2dPatternRepeat.
 *	\param[in]	pattern		The image to be used as a pattern for filling.
 *	\return		kFskErrNone		If the pattern stroke style was successfully set.
 */
FskAPI(FskErr)	FskCanvas2dSetStrokeStylePattern(FskCanvas2dContext ctx, UInt32 repetition, FskConstBitmap pattern /*, const FskCanvasMatrix3x2d *M*/);


/********************************************************************************
 * Line parameters
 ********************************************************************************/


/** Get the current stroke line width.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\return		The current stroke line width. The default is 1.0.
 *	\note		The returned line width may be different than the line width set with FskCanvas2dSetLineWidth() due to quantization.
 */
FskAPI(double)	FskCanvas2dGetLineWidth(FskConstCanvas2dContext ctx);


/** Set the current stroke line width.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	w			The desired stroke line width.
 */
FskAPI(void)	FskCanvas2dSetLineWidth(FskCanvas2dContext ctx, double w);


/** Get the current type of cap used on ends of lines.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\return		The current type of caps used on the ends on lines.
 *				The default is kFskCanvas2dLineCapButt.
 *				Other possibilities are kFskCanvas2dLineCapRound and kFskCanvas2dLineCapSquare.
 */
FskAPI(UInt32)	FskCanvas2dGetLineCap(FskConstCanvas2dContext ctx);


/** Set the current type of cap used on ends of lines.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	lineCap		The desired type of line cap {kFskCanvas2dLineCapRound, kFskCanvas2dLineCapSquare, kFskCanvas2dLineCapButt};
 */
FskAPI(void)	FskCanvas2dSetLineCap(FskCanvas2dContext ctx, UInt32 lineCap);


/** Get the current type of join used between linear segments.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\return		The current type of join used between linear segments.
 *				The default is kFskCanvas2dLineJoinMiter.
 *				Other possibilities are kFskCanvas2dLineJoinRound and kFskCanvas2dLineJoinBevel.
 */
FskAPI(UInt32)	FskCanvas2dGetLineJoin(FskConstCanvas2dContext ctx);


/** Set the current type of join used between linear segments.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	joinType	The type of join desired between linear segments.
 *	\return		The current type of join used between linear segments
 *				{ kFskCanvas2dLineJoinRound, kFskCanvas2dLineJoinBevel, kFskCanvas2dLineJoinMiter }.
 */
FskAPI(void)	FskCanvas2dSetLineJoin(FskCanvas2dContext ctx, UInt32 joinType);


/** Get the current miter limit.
 *	The miter length is the distance from the point where the join occurs
 *	to the intersection of the line edges on the outside of the join.
 *	The miter limit ratio is the maximum allowed ratio of the miter length to half the line width.
 *	The miter limit can be computed from the subtended angle as cosecant(angle/2) = 1./sine(angle/2).
 *	The default miter limit is 10, which corresponds to approximately 11.5 degrees.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\return		the current miter limit.
 */
FskAPI(double)	FskCanvas2dGetMiterLimit(FskConstCanvas2dContext ctx);


/** Set the current miter limit.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	miterLimit	The desired miter limit.
 */
FskAPI(void)	FskCanvas2dSetMiterLimit(FskCanvas2dContext ctx, double miterLimit);


/********************************************************************************
 * Shadows
 ********************************************************************************/


/** Get the current X component of the shadow offset.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		The current X component of the shadow offset. The default is 0.
 */
FskAPI(double)	FskCanvas2dGetShadowOffsetX(FskConstCanvas2dContext ctx);


/** Set the current X component of the shadow offset.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	ox		The desired X component of the shadow offset.
 */
FskAPI(void)	FskCanvas2dSetShadowOffsetX(FskCanvas2dContext ctx, double ox);


/** Get the current Y component of the shadow offset.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		The current Y component of the shadow offset. The default is 0.
 */
FskAPI(double)	FskCanvas2dGetShadowOffsetY(FskConstCanvas2dContext ctx);


/** Set the current Y component of the shadow offset.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	oy		The desired Y component of the shadow offset.
 */
FskAPI(void)	FskCanvas2dSetShadowOffsetY(FskCanvas2dContext ctx, double oy);


/** Get the current Gaussian sigma for the shadow blur.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		The current value of Gaussian sigma for the shadow blur.
 *				The default is 0.
 */
FskAPI(double)	FskCanvas2dGetShadowBlur(FskConstCanvas2dContext ctx);


/** Set the current Gaussian sigma for the shadow blur.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	blur	The desired value of Gaussian sigma to be use for shadow blurring.
 */
FskAPI(void)	FskCanvas2dSetShadowBlur(FskCanvas2dContext ctx, double blur);


/** Get the current shadow color.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		The current shadow color. The default is transparent black.
 */
FskAPI(FskConstColorRGBA)	FskCanvas2dGetShadowColor(FskConstCanvas2dContext ctx);


/** Set the current shadow color.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	color	The desired shadow color.
 */
FskAPI(void)	FskCanvas2dSetShadowColor(FskCanvas2dContext ctx, FskConstColorRGBA color);


/********************************************************************************
 * Geometric shapes.
 ********************************************************************************/


/** Clear the specified rectangle to transparent black.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	x		The left edge of the rectangle to be cleared.
 *	\param[in]	y		The top edge of the rectangle to be cleared.
 *	\param[in]	w		The width of the rectangle to be cleared.
 *	\param[in]	h		The height of the rectangle to be cleared.
 *	\return		kFskErrNone				if the rectangle was successfully cleared.
 *	\return		kFskErrNothingRendered	if nothing was cleared, due to clipping.
 *	\note		The coordinates are quantized to integers.
 */
FskAPI(FskErr)	FskCanvas2dClearRect( FskCanvas2dContext ctx, double x, double y, double w, double h);


/** Fill the specified rectangle with the current fill style.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	x		The left edge of the rectangle to be filled.
 *	\param[in]	y		The top edge of the rectangle to be filled.
 *	\param[in]	w		The width of the rectangle to be filled.
 *	\param[in]	h		The height of the rectangle to be filled.
 *	\return		kFskErrNone	If the rectangle was successfully filled.
 */
FskAPI(FskErr)	FskCanvas2dFillRect(  FskCanvas2dContext ctx, double x, double y, double w, double h);


/** Stroke the specified rectangle with the current stroke style.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	x		The left edge of the rectangle to be stroked.
 *	\param[in]	y		The top edge of the rectangle to be stroked.
 *	\param[in]	w		The width of the rectangle to be stroked.
 *	\param[in]	h		The height of the rectangle to be stroked.
 *	\return		kFskErrNone	If the rectangle's outline was successfully stroked.
 */
FskAPI(FskErr)	FskCanvas2dStrokeRect(FskCanvas2dContext ctx, double x, double y, double w, double h);


/** Fill the specified circle with the current fill style.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	x		The X-coordinate of the center of the circle to be filled.
 *	\param[in]	y		The Y-coordinate of the center of the circle to be filled.
 *	\param[in]	r		The radius of the circle to be filled.
 *	\return		kFskErrNone	If the circle was successfully filled.
 */
FskAPI(FskErr)	FskCanvas2dFillCircle(FskCanvas2dContext ctx, double x, double y, double r);


/** Stroke the specified circle with the current stroke style.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	x		The X-coordinate of the center of the circle to be stroked.
 *	\param[in]	y		The Y-coordinate of the center of the circle to be stroked.
 *	\param[in]	r		The radius of the circle to be stroked.
 *	\return		kFskErrNone	If the circle was successfully stroked.
 */
FskAPI(FskErr)	FskCanvas2dStrokeCircle(FskCanvas2dContext ctx, double x, double y, double r);


/** Fill the specified ellipse with the current fill style.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	x		The X-coordinate of the center of the ellipse to be filled.
 *	\param[in]	y		The Y-coordinate of the center of the ellipse to be filled.
 *	\param[in]	rx		The X-radius of the ellipse to be filled.
 *	\param[in]	ry		The Y-radius of the ellipse to be filled.
 *	\return		kFskErrNone	If the ellipse was successfully filled.
 */
FskAPI(FskErr)	FskCanvas2dFillEllipse(FskCanvas2dContext ctx, double x, double y, double rx, double ry);


/** Stroke the specified ellipse with the current stroke style.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	x		The X-coordinate of the center of the ellipse to be stroked.
 *	\param[in]	y		The Y-coordinate of the center of the ellipse to be stroked.
 *	\param[in]	rx		The X-radius of the ellipse to be stroked.
 *	\param[in]	ry		The Y-radius of the ellipse to be stroked.
 *	\return		kFskErrNone	If the ellipse was successfully stroked.
 */
FskAPI(FskErr)	FskCanvas2dStrokeEllipse(FskCanvas2dContext ctx, double x, double y, double rx, double ry);


/** Allocate a new path.
 *	\param[out]	pPath		A place to store the newly allocated path.
 *	\return		kFskErrNone	If the path was successfully allocated.
 */
FskAPI(FskErr)	FskCanvas2dPathNew(FskCanvas2dPath *pPath);


/** Dispose a path.
 *	\param[out]	path		The path to dispose.
 */
FskAPI(void)	FskCanvas2dPathDispose(FskCanvas2dPath path);


/** Reset the path.
 *	\param[in]	ctx			The Canvas 2d context. Can be NULL if path is not NULL.
 *	\param[in]	path		The path. NULL implies the path in the context.
 *	\return		kFskErrNone	If the current path was successfully reset and initialized to the null path.
 */
FskAPI(FskErr)	FskCanvas2dPathBegin(FskCanvas2dContext ctx, FskCanvas2dPath path);


/** Reset the current path.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\return		kFskErrNone	If the current path was successfully reset and initialized to the null path.
 */
FskErr	FskCanvas2dBeginPath(FskCanvas2dContext ctx);
#define FskCanvas2dBeginPath(ctx)	FskCanvas2dPathBegin(ctx, NULL)


/** Close the given path to the most recent MoveTo or equivalent.
 *	\param[in]	ctx			The Canvas 2d context. Can be NULL if path is not NULL.
 *	\param[in]	path		The path. NULL implies the path in the context.
 *	\return		kFskErrNone	If the current path was successfully closed.
 */
FskAPI(FskErr)	FskCanvas2dPathClose(FskCanvas2dContext ctx, FskCanvas2dPath path);


/** Close the current path to the most recent MoveTo or equivalent.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\return		kFskErrNone	If the current path was successfully closed.
 */
FskErr	FskCanvas2dClosePath(FskCanvas2dContext ctx);
#define FskCanvas2dClosePath(ctx)	FskCanvas2dPathClose(ctx, NULL)


/** Set the current point in the given path.
 *	\param[in]	ctx			The Canvas 2d context. Can be NULL if path is not NULL.
 *	\param[in]	path		The path. NULL implies the path in the context.
 *	\param[in]	x			The desired X-coordinate of the current point.
 *	\param[in]	y			The desired Y-coordinate of the current point.
 *	\return		kFskErrNone	If the current point was successfully set.
 *	\note		The path is not implicitly closed.
 */
FskAPI(FskErr)	FskCanvas2dPathMoveTo(FskCanvas2dContext ctx, FskCanvas2dPath path, double x, double y);


/** Set the current point in the current path.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	x			The desired X-coordinate of the current point.
 *	\param[in]	y			The desired Y-coordinate of the current point.
 *	\return		kFskErrNone	If the current point was successfully set.
 *	\note		The path is not implicitly closed.
 */
FskErr	FskCanvas2dMoveTo(FskCanvas2dContext ctx, double x, double y);
#define FskCanvas2dMoveTo(ctx, x, y)	FskCanvas2dPathMoveTo(ctx, NULL, x, y)


/** Append a linear segment to the given path.
 *	The segment extends from the current point to (x, y).
 *	Afterward, the current point is updated to (x, y).
 *	\param[in]	ctx			The Canvas 2d context. Can be NULL if path is not NULL.
 *	\param[in]	path		The path. NULL implies the path in the context.
 *	\param[in]	x			The X-coordinate of the far end of the new linear segment.
 *	\param[in]	y			The Y-coordinate of the far end of the new linear segment.
 *	\return		kFskErrNone	If the linear segment was successfully appended.
 */
FskAPI(FskErr)	FskCanvas2dPathLineTo(FskCanvas2dContext ctx, FskCanvas2dPath path, double x, double y);


/** Append a linear segment to the current path.
 *	The segment extends from the current point to (x, y).
 *	Afterward, the current point is updated to (x, y).
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	x			The X-coordinate of the far end of the new linear segment.
 *	\param[in]	y			The Y-coordinate of the far end of the new linear segment.
 *	\return		kFskErrNone	If the linear segment was successfully appended.
 */
FskErr	FskCanvas2dLineTo(FskCanvas2dContext ctx, double x, double y);
#define	FskCanvas2dLineTo(ctx, x, y)	FskCanvas2dPathLineTo(ctx, NULL, x, y)


/** Append a quadratic Bezier segment to the given path.
 *	The segment extends from the current point to (x, y),
 *	and its shape is controlled by the Bezier control point (cpx, cpy).
 *	Afterward, the current point is updated to (x, y).
 *	\param[in]	ctx			The Canvas 2d context. Can be NULL if path is not NULL.
 *	\param[in]	path		The path. NULL implies the path in the context.
 *	\param[in]	cpx			The X-coordinate of the quadratic Bezier control point.
 *	\param[in]	cpy			The Y-coordinate of the quadratic Bezier control point.
 *	\param[in]	y			The Y-coordinate of the far end of the new quadratic Bezier segment.
 *	\param[in]	x			The X-coordinate of the far end of the new quadratic Bezier segment.
 *	\param[in]	y			The Y-coordinate of the far end of the new quadratic Bezier segment.
 *	\return		kFskErrNone	If the quadratic segment was successfully appended.
 */
FskAPI(FskErr)	FskCanvas2dPathQuadraticCurveTo(FskCanvas2dContext ctx, FskCanvas2dPath path, double cpx, double cpy, double x, double y);


/** Append a quadratic Bezier segment to the current path.
 *	The segment extends from the current point to (x, y),
 *	and its shape is controlled by the Bezier control point (cpx, cpy).
 *	Afterward, the current point is updated to (x, y).
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	cpx			The X-coordinate of the quadratic Bezier control point.
 *	\param[in]	cpy			The Y-coordinate of the quadratic Bezier control point.
 *	\param[in]	y			The Y-coordinate of the far end of the new quadratic Bezier segment.
 *	\param[in]	x			The X-coordinate of the far end of the new quadratic Bezier segment.
 *	\param[in]	y			The Y-coordinate of the far end of the new quadratic Bezier segment.
 *	\return		kFskErrNone	If the quadratic segment was successfully appended.
 */
FskErr	FskCanvas2dQuadraticCurveTo(FskCanvas2dContext ctx, double cpx, double cpy, double x, double y);
#define	FskCanvas2dQuadraticCurveTo(ctx, cpx, cpy, x, y)	FskCanvas2dPathQuadraticCurveTo(ctx, NULL, cpx, cpy, x, y)


/** Append a cubic Bezier segment to the given path.
 *	The segment extends from the current point to (x, y).
 *	Afterward, the current point is updated to (x, y).
 *	\param[in]	ctx			The Canvas 2d context. Can be NULL if path is not NULL.
 *	\param[in]	path		The path. NULL implies the path in the context.
 *	\param[in]	cp1x		The X-coordinate of the first  cubic Bezier control point.
 *	\param[in]	cp1y		The Y-coordinate of the first  cubic Bezier control point.
 *	\param[in]	cp2x		The X-coordinate of the second cubic Bezier control point.
 *	\param[in]	cp2y		The Y-coordinate of the second cubic Bezier control point.
 *	\param[in]	x			The X-coordinate of the far end of the new cubic Bezier segment.
 *	\param[in]	y			The Y-coordinate of the far end of the new cubic Bezier segment.
 *	\return		kFskErrNone	If the cubic segment was successfully appended.
 */
FskAPI(FskErr)	FskCanvas2dPathCubicCurveTo(FskCanvas2dContext ctx, FskCanvas2dPath path, double cp1x, double cp1y, double cp2x, double cp2y, double x, double y);


/** Append a cubic Bezier segment to the current path.
 *	The segment extends from the current point to (x, y).
 *	Afterward, the current point is updated to (x, y).
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	cp1x		The X-coordinate of the first  cubic Bezier control point.
 *	\param[in]	cp1y		The Y-coordinate of the first  cubic Bezier control point.
 *	\param[in]	cp2x		The X-coordinate of the second cubic Bezier control point.
 *	\param[in]	cp2y		The Y-coordinate of the second cubic Bezier control point.
 *	\param[in]	x			The X-coordinate of the far end of the new cubic Bezier segment.
 *	\param[in]	y			The Y-coordinate of the far end of the new cubic Bezier segment.
 *	\return		kFskErrNone	If the cubic segment was successfully appended.
 */
FskErr	FskCanvas2dCubicCurveTo(FskCanvas2dContext ctx, double cp1x, double cp1y, double cp2x, double cp2y, double x, double y);
#define	FskCanvas2dCubicCurveTo(ctx, cp1x, cp1y, cp2x, cp2y, x, y)	FskCanvas2dPathCubicCurveTo(ctx, NULL, cp1x, cp1y, cp2x, cp2y, x, y)


/** Append a cubic Bezier segment to the given path.
 *	This is an alias for FskCanvas2dPathCubicCurveTo(), because bezierCurveTo is mentioned in the Canvas specification;
 *	however, it is ambiguous because we have both quadratic and cubic (and linear) Bezier segments.
 *	The segment extends from the current point to (x, y).
 *	Afterward, the current point is updated to (x, y).
 *	\param[in]	ctx			The Canvas 2d context. Can be NULL if path is not NULL.
 *	\param[in]	path		The path. NULL implies the path in the context.
 *	\param[in]	cp1x		The X-coordinate of the first  cubic Bezier control point.
 *	\param[in]	cp1y		The Y-coordinate of the first  cubic Bezier control point.
 *	\param[in]	cp2x		The X-coordinate of the second cubic Bezier control point.
 *	\param[in]	cp2y		The Y-coordinate of the second cubic Bezier control point.
 *	\param[in]	x			The X-coordinate of the far end of the new cubic Bezier segment.
 *	\param[in]	y			The Y-coordinate of the far end of the new cubic Bezier segment.
 *	\return		kFskErrNone	If the cubic segment was successfully appended.
 */
#define	FskCanvas2dPathBezierCurveTo(ctx, path, cp1x, cp1y, cp2x, cp2y, x, y) FskCanvas2dPathCubicCurveTo(ctx, path, cp1x, cp1y, cp2x, cp2y, x, y)


/** Append a cubic Bezier segment to the current path.
 *	This is an alias for FskCanvas2dCubicCurveTo(), because bezierCurveTo is mentioned in the Canvas specification;
 *	however, it is ambiguous because we have both quadratic and cubic (and linear) Bezier segments.
 *	The segment extends from the current point to (x, y).
 *	Afterward, the current point is updated to (x, y).
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	cp1x		The X-coordinate of the first  cubic Bezier control point.
 *	\param[in]	cp1y		The Y-coordinate of the first  cubic Bezier control point.
 *	\param[in]	cp2x		The X-coordinate of the second cubic Bezier control point.
 *	\param[in]	cp2y		The Y-coordinate of the second cubic Bezier control point.
 *	\param[in]	x			The X-coordinate of the far end of the new cubic Bezier segment.
 *	\param[in]	y			The Y-coordinate of the far end of the new cubic Bezier segment.
 *	\return		kFskErrNone	If the cubic segment was successfully appended.
 */
#define	FskCanvas2dBezierCurveTo(ctx, cp1x, cp1y, cp2x, cp2y, x, y) FskCanvas2dCubicCurveTo(ctx, cp1x, cp1y, cp2x, cp2y, x, y)


/** Append a circular arc segment to the given path.
 *	Guidelines are constructed from the previous point (call it p0), p1=(x1,y1) and p2=(x2,y2).
 *	A circle with the specified radius is placed tangent to the two guidelines,
 *	and the intersections of the circle with the guidelines shall be called the start point and end point.
 *	First, a linear segment is drawn from p0 to the start point.
 *	Then, an arc wth the specified radius is drawn from the start point to the end point.
 *	The current point is then updated to the end point.\n
 *	Note that the purpose of p2 is only to specify a guideline of tangency,
 *	and that no segment is drawn between the end point and p2.
 *	Once can consider this a fillet at (x1, y1).
 *	\param[in]	ctx			The Canvas 2d context. Can be NULL if path is not NULL.
 *	\param[in]	path		The path. NULL implies the path in the context.
 *	\param[in]	x1			The X-coordinate of the "corner" of the arc.
 *	\param[in]	y1			The Y-coordinate of the "corner" of the arc.
 *	\param[in]	x2			Helps to specify the exit tangent X-component.
 *	\param[in]	y2			Helps to specify the exit tangent Y-component.
 *	\param[in]	radius		The radius of the arc (fillet).
 *	\return		kFskErrNone	If the circular arc segment was successfully appended.
 *	\par	Example: Rounded Rect(x0, y0, x1, y1, r)
 *	\code
 *	FskCanvas2dMoveTo(ctx, (x0+x1)*.5, y0);		// mid top edge
 *	FskCanvas2dArcTo(ctx, x1, y0, x1, y1, r);	// upper right corner
 *	FskCanvas2dArcTo(ctx, x1, y1, x0, y1, r);	// lower right corner
 *	FskCanvas2dArcTo(ctx, x0, y1, x0, y0, r);	// lower left corner
 *	FskCanvas2dArcTo(ctx, x0, y0, x1, y0, r);	// upper left corner
 *	FskCanvas2dClosePath(ctx);			// stroke back to mid top edge
 *	\endcode
 */
FskAPI(FskErr)	FskCanvas2dPathArcTo(FskCanvas2dContext ctx, FskCanvas2dPath path, double x1, double y1, double x2, double y2, double radius);


/** Append a circular arc segment to the current path.
 *	Guidelines are constructed from the previous point (call it p0), p1=(x1,y1) and p2=(x2,y2).
 *	A circle with the specified radius is placed tangent to the two guidelines,
 *	and the intersections of the circle with the guidelines shall be called the start point and end point.
 *	First, a linear segment is drawn from p0 to the start point.
 *	Then, an arc wth the specified radius is drawn from the start point to the end point.
 *	The current point is then updated to the end point.\n
 *	Note that the purpose of p2 is only to specify a guideline of tangency,
 *	and that no segment is drawn between the end point and p2.
 *	Once can consider this a fillet at (x1, y1).
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	x1			The X-coordinate of the "corner" of the arc.
 *	\param[in]	y1			The Y-coordinate of the "corner" of the arc.
 *	\param[in]	x2			Helps to specify the exit tangent X-component.
 *	\param[in]	y2			Helps to specify the exit tangent Y-component.
 *	\param[in]	radius		The radius of the arc (fillet).
 *	\return		kFskErrNone	If the circular arc segment was successfully appended.
 *	\par	Example: Rounded Rect(x0, y0, x1, y1, r)
 *	\code
 *	FskCanvas2dMoveTo(ctx, (x0+x1)*.5, y0);		// mid top edge
 *	FskCanvas2dArcTo(ctx, x1, y0, x1, y1, r);	// upper right corner
 *	FskCanvas2dArcTo(ctx, x1, y1, x0, y1, r);	// lower right corner
 *	FskCanvas2dArcTo(ctx, x0, y1, x0, y0, r);	// lower left corner
 *	FskCanvas2dArcTo(ctx, x0, y0, x1, y0, r);	// upper left corner
 *	FskCanvas2dClosePath(ctx);			// stroke back to mid top edge
 *	\endcode
 */
FskErr	FskCanvas2dArcTo(FskCanvas2dContext ctx, double x1, double y1, double x2, double y2, double radius);
#define	FskCanvas2dArcTo(ctx, x1, y1, x2, y2, radius)	FskCanvas2dPathArcTo(ctx, NULL, x1, y1, x2, y2, radius)


/** Append a circular arc to the given path.
 *	If a previous subpath exists, then a straight line is first drawn
 *	from the current point to the starting point on the arc.
 *	\param[in]	ctx		The Canvas 2d context. Can be NULL if path is not NULL.
 *	\param[in]	path	The path. NULL implies the path in the context.
 *	\param[in]	cx		The X-coordinate of the center of the arc.
 *	\param[in]	cy		The Y-coordinate of the center of the arc.
 *	\param[in]	radius	The radius of the arc.
 *	\param[in]	startAngle	The start angle of the arc.
 *	\param[in]	endAngle	The end angle of the arc.
 *	\param[in]	counterClockwise	If true,  draws the arc counterclockwise from the start angle to the end angle;
 *									if false, draws the arc        clockwise from the start angle to the end angle.
 *	\return		kFskErrNone	If the circular arc segment was successfully appended.
 */
FskAPI(FskErr)	FskCanvas2dPathArc(FskCanvas2dContext ctx, FskCanvas2dPath path, double cx, double cy, double radius, double startAngle, double endAngle, Boolean counterClockwise);


/** Append a circular arc to the current path.
 *	If a previous subpath exists, then a straight line is first drawn
 *	from the current point to the starting point on the arc.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	cx		The X-coordinate of the center of the arc.
 *	\param[in]	cy		The Y-coordinate of the center of the arc.
 *	\param[in]	radius	The radius of the arc.
 *	\param[in]	startAngle	The start angle of the arc.
 *	\param[in]	endAngle	The end angle of the arc.
 *	\param[in]	counterClockwise	If true,  draws the arc counterclockwise from the start angle to the end angle;
 *									if false, draws the arc        clockwise from the start angle to the end angle.
 *	\return		kFskErrNone	If the circular arc segment was successfully appended.
 */
#define	FskCanvas2dArc(ctx, cx, cy, radius, startAngle, endAngle, counterClockwise)	FskCanvas2dPathArc(ctx, NULL, cx, cy, radius, startAngle, endAngle, counterClockwise)


/** Append a rectangle to the given path.
 *	\param[in]	ctx		The Canvas 2d context. Can be NULL if path is not NULL.
 *	\param[in]	path	The path. NULL implies the path in the context.
 *	\param[in]	x		The left edge of the rectangle.
 *	\param[in]	y		The top edge of the rectangle.
 *	\param[in]	w		The width of the rectangle.
 *	\param[in]	h		The height of the rectangle.
 *	\return		kFskErrNone	If the rectangle was successfully appended.
 */
FskAPI(FskErr)	FskCanvas2dPathRect(FskCanvas2dContext ctx, FskCanvas2dPath path, double x, double y, double w, double h);


/** Append a rectangle to the current path.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	x		The left edge of the rectangle.
 *	\param[in]	y		The top edge of the rectangle.
 *	\param[in]	w		The width of the rectangle.
 *	\param[in]	h		The height of the rectangle.
 *	\return		kFskErrNone	If the rectangle was successfully appended.
 */
FskErr	FskCanvas2dRect(FskCanvas2dContext ctx, double x, double y, double w, double h);
#define	FskCanvas2dRect(ctx, x, y, w, h)	FskCanvas2dPathRect(ctx, NULL, x, y, w, h)


/** Parse a path string (as described in the SVG specification) and append it to the given path.
 *	\param[in]	ctx		The Canvas 2d context. Can be NULL if path is not NULL.
 *	\param[in]	path	The path. NULL implies the path in the context.
 *	\param[in]	pathStr	The path string.
 *	\return		kFskErrNone	If the operation was completed successfully.
 */
FskAPI(FskErr)	FskCanvas2dPathAppendPathString(FskCanvas2dContext ctx, FskCanvas2dPath path, const char *pathStr);


/** Parse a path string (as described in the SVG specification) and append it to the current path.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	pathStr	The path string.
 *	\return		kFskErrNone	If the operation was completed successfully.
 */
FskErr	FskCanvasAppendPathString(FskCanvas2dContext ctx, const char *pathStr);
#define	FskCanvasAppendPathString(ctx, pathStr)	FskCanvas2dPathAppendPathString(ctx, NULL, pathStr)


/** Append a path to the given path.
 *	\param[in]	ctx		The Canvas 2d context. Can be NULL if dst is not NULL.
 *	\param[in]	dst		The destination path. NULL implies the path in the context.
 *	\param[in]	src		The path to be appended to the dst path.
 *	\param[in]	M		A transformation matrix. NULL implies the identity.
 *	\param[in]	pathStr	The path string.
 *	\return		kFskErrNone	If the operation was completed successfully.
 */
FskAPI(FskErr)	FskCanvas2dPathAppendPath(FskCanvas2dContext ctx, FskCanvas2dPath dst, FskConstCanvas2dPath src, const FskCanvasMatrix3x2d *M);


/** Append an EndGlyph code to the path.
 *	\param[in]	ctx			The Canvas 2d context. Can be NULL if dst is not NULL.
 *	\param[in]	path		The destination path. NULL implies the path in the context.
 *	\return		kFskErrNone	If the operation was completed successfully.
 */
FskAPI(FskErr)	FskCanvas2dPathEndGlyph(FskCanvas2dContext ctx, FskCanvas2dPath path);


/** Fill the given path with the current fill style.
 *	\param[in]	ctx			The Canvas 2d context. Cannot be NULL.
 *	\param[in]	path		The path to be filled. NULL implies the path in the context.
 *	\param[in]	fillRule	The fill rule: { kFskCanvas2dFillRuleWindingNumber, kFskCanvas2dFillRuleParity, 0 },
 *							where 0 is the default kFskCanvas2dFillRuleWindingNumber.
 *	\return		kFskErrNone	If the path was successfully filled.
 */
FskAPI(FskErr)	FskCanvas2dPathFill(FskCanvas2dContext ctx, FskConstCanvas2dPath path, SInt32 fillRule);


/** Fill the current path with the current fill style.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		kFskErrNone	If the path was successfully filled.
 */
FskErr	FskCanvas2dFill(FskCanvas2dContext ctx);
#define	FskCanvas2dFill(ctx)	FskCanvas2dPathFill(ctx, NULL, kFskCanvas2dFillRuleWindingNumber)


/** Stroke the given path with the current stroke style.
 *	\param[in]	ctx		The Canvas 2d context. Cannot be NULL.
 *	\return		kFskErrNone	If the path was successfully stroked.
 */
FskAPI(FskErr)	FskCanvas2dPathStroke(FskCanvas2dContext ctx, FskConstCanvas2dPath path);


/** Stroke the current path with the current stroke style.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		kFskErrNone	If the path was successfully stroked.
 */
FskErr	FskCanvas2dStroke(FskCanvas2dContext ctx);
#define	FskCanvas2dStroke(ctx)	FskCanvas2dPathStroke(ctx, NULL)


/** Intersect the given clip region with the given path, and update the current clip region.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	path		The clip path. NULL implies the path in the context.
 *	\param[in]	fillRule	The rule for filling { kFskCanvas2dFillRuleWindingNumber, kFskCanvas2dFillRuleParity, 0 },
 *							where kFskCanvas2dFillRuleWindingNumber=0 is the default.
 *	\return		kFskErrNone	If the clip region was successfully set.
 */
FskAPI(FskErr)	FskCanvas2dPathClip(FskCanvas2dContext ctx, FskConstCanvas2dPath path, SInt32 fillRule);


/** Intersect the current clip region with the current path, and update the current clip region.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		kFskErrNone	If the clip region was successfully set.
 */
FskErr	FskCanvas2dClip(FskCanvas2dContext ctx);
#define	FskCanvas2dClip(ctx)	FskCanvas2dPathClip(ctx, NULL, kFskCanvas2dFillRuleWindingNumber)


/** Reset the clip region to the largest infinite surface.
 *	\param[in]	ctx			The Canvas 2d context.
 */
FskAPI(void)	FskCanvas2dClipReset(FskCanvas2dContext ctx);


/** Query as to whether the specified point is within the current path.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	path	The
 *	\param[in]	x		The X-coordinate of the point in question.
 *	\param[in]	y		The Y-coordinate of the point in question.
 *	\param[in]	fillRule	The rule for filling { kFskCanvas2dFillRuleWindingNumber, kFskCanvas2dFillRuleParity, 0 },
 *							where kFskCanvas2dFillRuleWindingNumber=0 is the default.
 *	\return		True if the point is contained within the path,
 *				false otherwise.
 */
FskAPI(Boolean)	FskCanvas2dIsPointInGivenPath(FskCanvas2dContext ctx, FskConstCanvas2dPath path, double x, double y, SInt32 fillRule);


/** Query as to whether the specified point is within the current path.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	x		The X-coordinate of the point in question.
 *	\param[in]	y		The Y-coordinate of the point in question.
 *	\return		True if the point is contained within the path,
 *				false otherwise.
 */
Boolean	FskCanvas2dIsPointInPath(FskCanvas2dContext ctx, double x, double y);
#define	FskCanvas2dIsPointInPath(ctx, x, y)	FskCanvas2dIsPointInGivenPath(ctx, NULL, x, y, kFskCanvas2dFillRuleWindingNumber)


/** Query as to whether the specified point is within the stroke of the current path.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	path	The path to be tested.
 *	\param[in]	x		The X-coordinate of the point in question.
 *	\param[in]	y		The Y-coordinate of the point in question.
 *	\return		True if the point is contained within the path,
 *				false otherwise.
 */
FskAPI(Boolean)	FskCanvas2dIsPointInPathStroke(FskCanvas2dContext ctx, FskConstCanvas2dPath path, double x, double y);


struct FskFontAttributes;	/* Forward declaration: defined in FskGlyphPath.h */


/** Get the current font attributes.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		The current font attributes.
 *				The default is 10 pixel sans-serif.
 */
FskAPI(const struct FskFontAttributes*)	FskCanvas2dGetFont(FskCanvas2dContext ctx);


/** Set the current font attributes.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	font	The desired font attributes.
 *	\return		kFskErrNone	If the font attributes were successfully set.
 */
FskAPI(FskErr)	FskCanvas2dSetFont(FskCanvas2dContext ctx, const struct FskFontAttributes *font);


/** Get the current text alignment.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		The current text alignment.
 *				The default is kFskCanvas2dTextAlignStart.
 */
FskAPI(UInt32) FskCanvas2dGetTextAlignment(FskConstCanvas2dContext ctx);


/** Set the current text alignment.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	alignment	The desired text alignment.
 */
FskAPI(void) FskCanvas2dSetTextAlignment(FskCanvas2dContext ctx, UInt32 alignment);

/** Get the current text baseline.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\return		The current text baseline.
 *				The default is kFskCanvas2dTextBaselineAlphabetic.
 */
UInt32 FskCanvas2dGetTextBaseline(FskConstCanvas2dContext ctx);


/** Set the current text baseline.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	baseline	The desired text baseline.
 */
FskAPI(void) FskCanvas2dSetTextBaseline(FskCanvas2dContext ctx, UInt32 baseline);


/** Fill the specified text using the current font attributes and fill style.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	uniChars	The text string, in Unicode (UTF-16).
 *	\param[in]	x			The X-coordinate of the anchor point of the text.
 *	\param[in]	y			The Y-coordinate of the anchor point of the text.
 *	\param[in]	maxWidth	The maximum width tolerated for the text.
 *							If you don't care, make it to negative or zero.
 *							If the text is too wide to fit, the text is progressively condensed until it does.
 *	\return		kFskErrNone		if the text string was successively drawn and filled.
 *	\return		kFskErrTooMany	if the text wouldn't fit even at kFskFontStretchUltraCondensed.
 *	\todo		Scale the path numerically if condensation still doesn't allow it to fit.
 */
FskAPI(FskErr)	FskCanvas2dFillText(   FskCanvas2dContext ctx, const UInt16 *uniChars, double x, double y, double maxWidth);


/** Stroke the specified text using the current font attributes and stroke style.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	uniChars	The text string, in Unicode (UTF-16).
 *	\param[in]	x			The X-coordinate of the anchor point of the text.
 *	\param[in]	y			The Y-coordinate of the anchor point of the text.
 *	\param[in]	maxWidth	The maximum width tolerated for the text.
 *							If you don't care, make it to negative or zero.
 *							If the text is too wide to fit, the text is progressively condensed until it does.
 *	\return		kFskErrNone		if the text string was successively drawn and stroked.
 *	\return		kFskErrTooMany	if the text wouldn't fit even at kFskFontStretchUltraCondensed.
 *	\bug		The fit does not take the stroke width into account,
 *				so there might be some clipping when trying to accommodate maxWidth.
 *	\todo		Scale the path numerically if condensation still doesn't allow it to fit.
 */
FskAPI(FskErr)	FskCanvas2dStrokeText( FskCanvas2dContext ctx, const UInt16 *uniChars, double x, double y, double maxWidth);


/** Determine the width of the text using the current font attributes.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	uniChars	The text string, in Unicode (UTF-16).
 *	\todo	In addition to advance width, this should also have
 *			actualBoundingBoxLeft, actualBoundingBoxRight
 *			fontBoundingBoxAscent, fontBoundingBoxDescent, actualBoundingBoxAscent, actualBoundingBoxDescent,
 *			emHeightAscent, emHeightDescent, hangingBaseline, alphabeticBaseline, deographicBaseline.
 *	\return		The width of the text string.
 */
FskAPI(double)	FskCanvas2dMeasureText(FskCanvas2dContext ctx, const UInt16 *uniChars);


/********************************************************************************
 * Images
 ********************************************************************************/

/** Draw a bitmap at the specified location.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	src		The source bitmap.
 *	\param[in]	dx		The left edge of the image is to be placed here.
 *	\param[in]	dy		The top  edge of the image is to be placed here.
 *	\return		kFskErrNone	if the image was drawn successfully.
 */
FskAPI(FskErr)	FskCanvas2dDrawBitmap(         FskCanvas2dContext ctx, FskConstBitmap src, double dx, double dy);


/** Draw a bitmap at the specified location and size.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	src		The source bitmap.
 *	\param[in]	dx		The left edge of the image is to be placed here.
 *	\param[in]	dy		The top  edge of the image is to be placed here.
 *	\param[in]	dw		The image is scaled to this width.
 *	\param[in]	dh		The image is scaled to this height.
 *	\return		kFskErrNone	if the image was drawn successfully.
 */
FskAPI(FskErr)	FskCanvas2dDrawScaledBitmap(   FskCanvas2dContext ctx, FskConstBitmap src, double dx, double dy, double dw, double dh);


/** Draw a portion of a bitmap at the specified location and size.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	src		The source bitmap.
 *	\param[in]	sx		The left edge of the subrectangle of the source image.
 *	\param[in]	sy		The top  edge of the subrectangle of the source image.
 *	\param[in]	sw		The width  of the subrectangle of the source image.
 *	\param[in]	sh		The height of the subrectangle of the source image.
 *	\param[in]	dx		The left edge of the image is to be placed here.
 *	\param[in]	dy		The top  edge of the image is to be placed here.
 *	\param[in]	dw		The portion of the image is scaled to this width.
 *	\param[in]	dh		The portion of the image is scaled to this height.
 *	\return		kFskErrNone	if the image was drawn successfully.
 */
FskAPI(FskErr)	FskCanvas2dDrawSubScaledBitmap(FskCanvas2dContext ctx, FskConstBitmap src, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh);


/** Draw a Canvas at the specified location.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	src		The source Canvas.
 *	\param[in]	dx		The left edge of the Canvas is to be placed here.
 *	\param[in]	dy		The top  edge of the Canvas is to be placed here.
 *	\return		kFskErrNone	if the Canvas was drawn successfully.
 */
FskAPI(FskErr)	FskCanvas2dDrawCanvas2d(         FskCanvas2dContext ctx, FskConstCanvas src, double dx, double dy);


/** Draw a Canvas at the specified location and size.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	src		The source Canvas.
 *	\param[in]	dx		The left edge of the Canvas is to be placed here.
 *	\param[in]	dy		The top  edge of the Canvas is to be placed here.
 *	\param[in]	dw		The Canvas is scaled to this width.
 *	\param[in]	dh		The Canvas is scaled to this height.
 *	\return		kFskErrNone	if the Canvas was drawn successfully.
 */
FskAPI(FskErr)	FskCanvas2dDrawScaledCanvas2d(   FskCanvas2dContext ctx, FskConstCanvas src, double dx, double dy, double dw, double dh);


/** Draw a portion of a Canvas at the specified location and size.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	src		The source Canvas.
 *	\param[in]	sx		The left edge of the subrectangle of the source Canvas.
 *	\param[in]	sy		The top  edge of the subrectangle of the source Canvas.
 *	\param[in]	sw		The width  of the subrectangle of the source Canvas.
 *	\param[in]	sh		The height of the subrectangle of the source Canvas.
 *	\param[in]	dx		The left edge of the Canvas is to be placed here.
 *	\param[in]	dy		The top  edge of the Canvas is to be placed here.
 *	\param[in]	dw		The portion of the Canvas is scaled to this width.
 *	\param[in]	dh		The portion of the Canvas is scaled to this height.
 *	\return		kFskErrNone	if the Canvas was drawn successfully.
 */
FskAPI(FskErr)	FskCanvas2dDrawSubScaledCanvas2d(FskCanvas2dContext ctx, FskConstCanvas src, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh);


/** Draw a Video at the specified location.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	src		The source Video.
 *	\param[in]	dx		The left edge of the Video is to be placed here.
 *	\param[in]	dy		The top  edge of the Video is to be placed here.
 *	\return		kFskErrNone	if the Video was drawn successfully.
 *	\bug		unimplemented.
 */
FskAPI(FskErr)	FskCanvas2dDrawVideo(         FskCanvas2dContext ctx, FskVideo src, double dx, double dy);


/** Draw a Video at the specified location and size.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	src		The source Video.
 *	\param[in]	dx		The left edge of the Video is to be placed here.
 *	\param[in]	dy		The top  edge of the Video is to be placed here.
 *	\param[in]	dw		The Video is scaled to this width.
 *	\param[in]	dh		The Video is scaled to this height.
 *	\return		kFskErrNone	if the Video was drawn successfully.
 *	\bug		unimplemented.
 */
FskAPI(FskErr)	FskCanvas2dDrawScaledVideo(   FskCanvas2dContext ctx, FskVideo src, double dx, double dy, double dw, double dh);


/** Draw a portion of a Video at the specified location and size.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	src		The source Video.
 *	\param[in]	sx		The left edge of the subrectangle of the source Video.
 *	\param[in]	sy		The top  edge of the subrectangle of the source Video.
 *	\param[in]	sw		The width  of the subrectangle of the source Video.
 *	\param[in]	sh		The height of the subrectangle of the source Video.
 *	\param[in]	dx		The left edge of the Video is to be placed here.
 *	\param[in]	dy		The top  edge of the Video is to be placed here.
 *	\param[in]	dw		The portion of the Video is scaled to this width.
 *	\param[in]	dh		The portion of the Video is scaled to this height.
 *	\return		kFskErrNone	if the Video was drawn successfully.
 *	\bug		unimplemented.
 */
FskAPI(FskErr)	FskCanvas2dDrawSubScaledVideo(FskCanvas2dContext ctx, FskVideo src, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh);


/********************************************************************************
 * Pixel access
 ********************************************************************************/


/** Create a ImageData pixel buffer.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	sw		The desired width  of the pixel buffer.
 *	\param[in]	sh		The desired height of the pixel buffer.
 *	\return		The new image data pixel buffer, or NULL if unsuccessful.
 */
FskAPI(FskCanvas2dImageData)	FskCanvas2dCreateImageData(FskConstCanvas2dContext ctx, double sw, double sh);


/** Delete an ImageData pixel buffer. Alternatively, call FskMemPtrDispose().
 *	\param[in]	id		The image data.
 */
FskAPI(void) FskCanvas2dDisposeImageData(FskCanvas2dImageData id);


/** Make a replica of the given pixel buffer.
 *	\param[in]	ctx			The Canvas 2d context.
 *	\param[in]	imagedata	The source image data.
 *	\return		The new image data pixel buffer, or NULL if unsuccessful.
 *				It will have the exact contents of the source image data.
 */
FskAPI(FskCanvas2dImageData)	FskCanvas2dCloneImageData(FskConstCanvas2dContext ctx, FskConstCanvas2dImageData imagedata);


/** Return a new image data buffer that is a copy of a rectangular region of the buffer in the Canvas.
 *	The format will be straight alpha RGBA, converted from premultiplied alpha, so that the sequence
 *	FskCanvas2dPutImageData() followed by FskCanvas2dGetImageData() will not yield the original data,
 *	but one that is equivalent by projection into a premultiplied representation.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	sx		The left edge of the rectangle of interest.
 *	\param[in]	sy		The top  edge of the rectangle of interest.
 *	\param[in]	sw		The width  of the rectangle of interest.
 *	\param[in]	sh		The height of the rectangle of interest.
 *	\return		the contents of the specified rectangle in straight alpha RGBA format as converted from a premultiplied alpha format..
 */
FskAPI(FskCanvas2dImageData)	FskCanvas2dGetImageData(FskConstCanvas2dContext ctx, double sx, double sy, double sw, double sh);


/** Copy the image data to the canvas buffer.
 *	The src should be in straight alpha RGBA format, but will be converted into premultiplied alpha when written into the canvas bitmap,
 *	so that a FskCanvas2dPutImageData() followed by FskCanvas2dGetImageData() will not yield the original data,
 *	but one that is equivalent by projection into a premultiplied representation.
 *	\param[in]	ctx		The Canvas 2d context.
 *	\param[in]	src		The source image data.
 *	\param[in]	dx		The left edge of the destination rectangle.
 *	\param[in]	dy		The top  edge of the destination rectangle.
 *	\param[in]	sx		The left edge of the source rectangle.
 *	\param[in]	sy		The top  edge of the source rectangle.
 *	\param[in]	sw		The width  of the source rectangle.
 *	\param[in]	sh		The height edge of the source rectangle.
 *	\return		kFskErrNone	if the image data was successfully transferred.
 */
FskAPI(FskErr) FskCanvas2dPutImageData(FskCanvas2dContext ctx, FskConstCanvas2dImageData src, double dx, double dy, double sx, double sy, double sw, double sh);


/** Set the Canvas bitmap's OpenGLSourceAccelerated bit.
 *	\param[in,out]	cnv				The Canvas.
 *	\param[in]		accelerated		If true, it should be accelerated; if false it should be decelerated.
 *	\return			kFskErrNone	if the operation was executed successfully.
 */
FskAPI(FskErr)	FskCanvas2dSetOpenGLSourceAccelerated(FskCanvas cnv, Boolean accelerated);


#ifdef PUNT
		/* Focus management */
Boolean FskCanvas2dDrawFocusRing(FskCanvas2dContext ctx, Element element, double xCaret, double yCaret, Boolean canDrawCustom);
void FskCanvas2dDrawFocusIfNeeded(FskCanvas2dContext ctx, Element element);
void FskCanvas2dDrawFocusIfNeeded(FskCanvas2dContext ctx, FskConstCanvas2dPath path, Element element);
void FskCanvas2dScrollPathIntoView(FskCanvas2dContext ctx);
void FskCanvas2dScrollPathIntoView(FskCanvas2dContext ctx, FskConstCanvas2dPath path);

struct FskCanvasDrawingStylesRecord;
FskAPI(FskErr)	FskCanvas2dPathAppendByStrokingPath(FskCanvas2dContext ctx, FskCanvas2dPath dst, FskConstCanvas2dPath src, const struct FskCanvasDrawingStylesRecord *styles, const FskCanvasMatrix3x2d *M);
FskAPI(FskErr)	FskCanvas2dPathAppendText(FskCanvas2dContext ctx, FskCanvas2dPath dst, const UInt16 *uniChars, const struct FskCanvasDrawingStylesRecord *styles, const FskCanvasMatrix3x2d *M, double x, double y, double maxWidth);
FskAPI(FskErr)	FskCanvas2dPathAppendByStrokingText(FskCanvas2dContext ctx, FskCanvas2dPath dst, const UInt16 *uniChars, const struct FskCanvasDrawingStylesRecord *styles, const FskCanvasMatrix3x2d *M, double x, double y, double maxWidth);
FskAPI(FskErr)	FskCanvas2dSetLineDash(FskCanvas2dContext ctx, UInt32 numCycles, const double *dash);
FskAPI(FskErr)	FskCanvas2dGetLineDash(FskConstCanvas2dContext ctx, UInt32 *pNumCycles, const double **dash);
FskAPI(void)	FskCanvas2dSetLineDashOffset(FskCanvas2dContext ctx, double offset);
FskAPI(double)	FskCanvas2dGetLineDashOffset(FskConstCanvas2dContext ctx);
FskAPI(void)	FskCanvas2dSetTextDirection(FskCanvas2dContext ctx, SInt32 direction);
FskAPI(SInt32)	FskCanvas2dGetTextDirection(FskConstCanvas2dContext ctx);
FskAPI(void)	FskCanvas2dAddHitRegion(FskCanvas2dContext ctx, optional HitRegionOptions options);
FskAPI(void)	FskCanvas2dRemoveHitRegion(FskCanvas2dContext ctx, DOMString id);
FskAPI(void)	FskCanvas2dClearHitRegions(FskCanvas2dContext ctx);
FskAPI(FskErr)	FskCanvas2dPathEllipse(FskCanvas2dContext ctx, FskCanvas2dPath path, double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, Boolean anticlockwise);
FskAPI(Boolean)	FskCanvasProbablySupportsContext(const char *);
FskAPI(void)	FskCanvasSetContext(FskCanvas2dContext ctx);
FskAPI(FskErr)	FskCanvasToBlob(FskCanvas cnv, const char *type, float quality, UInt32 *numBytes, char **bytes);
FskAPI(CanvasProxy)	FskCanvasTransferControlToProxy(FskCanvas cnv);

#endif /* PUNT */



#ifdef __FSKCANVAS_PRIV__
#include "FskGlyphPath.h"

/** The encapsulation of a color source. */
typedef struct FskCanvas2dColorSource {
	FskColorSourceUnion	csu;							/**< The color source proper. */
	FskGradientStop		gs[kCanvas2DMaxGradientStops];	/**< The gradient stops associated with the color source. */
} FskCanvas2dColorSource;								/**< The encapsulation of a color source. */


/** The encapsulation of the Canvas 2D state. */
typedef struct FskCanvas2dContextState {
    UInt8					globalAlpha;				/**< (default 1.0) */
	UInt8					lineCap;					/**< "butt", "round", "square" (default "butt") */
	UInt8					lineJoin;					/**< "round", "bevel", "miter" (default "miter") */
    UInt8					textAlign;					/**< "start", "end", "left", "right", "center" (default: "start") */
    UInt8					textBaseline;				/**< "top", "hanging", "middle", "alphabetic", "ideographic", "bottom" (default: "alphabetic") */
	UInt8					globalCompositeOperation;	/**< (default source-over) */
	UInt8					quality;					/**< (default 1) */
	UInt8					fillRule;					/**< (default kFskFillRuleWindingNumber) */
	FskFixed				miterLimit;					/**< (default 10) */
	FskFixed				lineWidth;					/**< (default 1) */
	FskPointRecord			shadowOffset;				/**< (default 0,0) */
	float					shadowBlur;					/**< (default 0) */
    FskCanvas2dColorSource	strokeStyle;				/**< (default black) */
	FskCanvas2dColorSource	fillStyle;					/**< (default black) */
	FskColorRGBARecord		shadowColor;				/**< (default transparent black) */
	FskRectangleRecord		clipRect;					/**< Default everything */
	FskBitmap				clipBM;						/**< Default NULL */
	UInt32					clipBytes;					/**< The number of bytes allocated for the clip buffer */

	FskCanvasMatrix3x2d		transform;					/**< The current transformation matrix. */
	FskFixedMatrix3x2		fixedTransform;				/**< The concatenation of the current floating-point transform with the device transform, in fixed-point */
	FskFontAttributes		font;						/**< (default 10px sans-serif) */
	char					fontFamily[128];			/**< The name of the current font family. */
} FskCanvas2dContextState;								/**< The encapsulation of the Canvas 2D state. */

/** The encapsulation of the Canvas 2D Context. */
struct FskCanvas2dContextRecord {
	FskCanvas				canvas;						/**< The canvas state. */
	FskCanvasMatrix3x2d		deviceTransform;			/**< The current device transform. */
	FskGrowablePath			path;						/**< The current path. */
	FskGrowableArray		state;						/**< FskCanvas2dContextState. */
};

/** The encapsulation of the 2D Canvas. */
struct FskCanvasRecord {
	FskCanvas2dContextRecord	ctx;					/**< The context. */
	FskBitmap					bm;						/**< This is where we render the final images. */
	FskBitmap					tmp;					/**< When there is complex clipping, we render here first, then alpha composite. */
	UInt32						tmpBytes;				/**< The number of bytes allocated for the tmp buffer. */
	Boolean						notMyBitmap;			/**< If true, then do not dispose of bitmap. */
    Boolean                     accelerate;				/**< If true, then the bitmap is targeted to be accelerated by OpenGL. */
};


FskCanvas2dContextState* FskCanvasGetStateFromContext(FskCanvas2dContext ctx);
const FskCanvas2dContextState* FskCanvasGetConstStateFromContext(FskConstCanvas2dContext ctx);
Boolean	FskCanvasCanFillDirectly(const FskCanvas2dContextState *st);
Boolean	FskCanvasCanStrokeDirectly(const FskCanvas2dContextState *st);
Boolean	FskCanvasCanRenderDirectly(const FskCanvas2dContextState *st);
Boolean	FskCanvasStateMatrixIsIdentity(FskCanvas2dContextState *st);
FskErr	FskCanvasGetTextPathThatFits(FskCanvas2dContext ctx, const UInt16 *uniChars, double x, double y, double maxWidth, FskGrowablePath *pPath);
FskErr	FskBitmapToDataURL(FskConstBitmap bm, const char *type, float quality, char **dataURL);
FskErr	FskCanvasConvertImageDataFormat(FskCanvas2dImageData id, FskBitmapFormatEnum srcFmt, FskBitmapFormatEnum dstFmt);

#endif /* __FSKCANVAS_PRIV__ */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKCANVAS2D__ */
