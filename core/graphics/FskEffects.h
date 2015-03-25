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
	\file	FskEffects.h
	\brief	Visual effects.
*/
#ifndef __FSKEFFECTS__
#define __FSKEFFECTS__

#include "FskRectangle.h"
#include "FskBitmap.h"



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Identifier, or selector for an effect.
 **/
enum FskEffectID	{
	kFskEffectCopy			= 0,														/**< No effect - just copy. */

	/* End user effects */
	kFskEffectBoxBlur,																	/**< Box blur effect. */
	kFskEffectColorize,																	/**< Colorize effect. */
	kFskEffectDilate,																	/**< Dilate effect. */
	kFskEffectErode,																	/**< Erode effect. */
	kFskEffectGaussianBlur,																/**< Gaussian Blur effect. */
	kFskEffectMonochrome,																/**< Gray effect. */
	kFskEffectInnerGlow,																/**< Inner Glow effect. */
	kFskEffectInnerShadow,																/**< Inner Shadow effect. */
	kFskEffectMask,																		/**< Mask Effect effect. */
	kFskEffectOuterGlow,																/**< Outer Glow effect. */
	kFskEffectOuterShadow,																/**< Outer Shadow effect. */
	kFskEffectShade,																	/**< Shade effect. */

	kFskEffectCompound,																	/**< Compound effect. */

	/* Elementary effects */
	kFskEffectColorizeAlpha,															/**< Colorize Alpha effect. */
	kFskEffectColorizeInner,															/**< Colorize Inner effect. */
	kFskEffectColorizeOuter,															/**< Colorize Outer effect. */
	kFskEffectCopyMirrorBorders,														/**< Copy but mirror the borders. */
	kFskEffectDirectionalBoxBlur,														/**< Directional Box Blur effect. */
	kFskEffectDirectionalGaussianBlur,													/**< Directional Gaussian Blur effect. */
	kFskEffectDirectionalDilate,														/**< Directional Dilate effect. */
	kFskEffectDirectionalErode,															/**< Directional Erode effect. */
	kFskEffectGel,																		/**< Gel effect. */
	kFskEffectPremultiplyAlpha,															/**< Convert from straight to premultiplied alpha. */
	kFskEffectStraightAlpha,															/**< Convert from premultiplied to straight alpha. */
};
typedef enum FskEffectID FskEffectID;



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****					Parameters for specific effects						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/** Parameters for the Copy Effect.
 **/
struct FskEffectCopyRecord {
	UInt32 unused;																	/**< C requires that a structure or union has at least one member */
};
typedef       struct FskEffectCopyRecord	FskEffectCopyRecord, *FskEffectCopy;
typedef const struct FskEffectCopyRecord	*FskConstEffectCopy;


/** Parameters for the CopyMirrorBorders Effect.
 **/
struct FskEffectCopyMirrorBordersRecord {
	UInt32	border;																	/**< The width of the border. */
};
typedef       struct FskEffectCopyMirrorBordersRecord	FskEffectCopyMirrorBordersRecord, *FskEffectCopyMirrorBorders;
typedef const struct FskEffectCopyMirrorBordersRecord	*FskConstEffectCopyMirrorBorders;


/** Parameters for the Box Blur Effect.
 ** Blur the src image with a Box Blur.
 ** Note that there is a separate parameter for each of the horizontal and vertical directions.
 ** This is primarily because the two dimensional blur is implemented with two one dimensional blurs.
 ** As such, an intermediate buffer is required. If none is supplied in the FskEffectCache (i.e. it is NULL),
 ** then each invocation will incur an allocation and deallocation of intermediate buffers.
 ** Note that more than one intermediate buffer may be required in the following circumstances:
 ** - the srcRect is neither NULL nor the entire src bounds.
 ** - the GPU has implemented a low profile of OpenGL.
 ** - the src has no texture associated with it.
 **/
struct FskEffectBoxBlurRecord {
	int	radiusX;																	/**< The radius of a Box kernel (width 2*radius+1) used for blurring horizontally. */
	int	radiusY;																	/**< The radius of a Box kernel (width 2*radius+1) used for blurring vertically.   */
};
typedef       struct FskEffectBoxBlurRecord	FskEffectBoxBlurRecord, *FskEffectBoxBlur;
typedef const struct FskEffectBoxBlurRecord	*FskConstEffectBoxBlur;


/** Parameters for the Colorize Inner Effect.
 ** Note that we use alpha for opacity and do not have a separate, redundant opacity, as the kprEffects do.
 ** This is not intended to be used by the end user, because the SW implementation does not scale the matte while the GL implementation does.
 **/
struct FskEffectColorizeInnerRecord {
	FskConstBitmap		matte;														/**< Matte */
	FskColorRGBARecord	color;														/**< The color corresponding to alpha = one. */
};
typedef			struct FskEffectColorizeInnerRecord	FskEffectColorizeInnerRecord, *FskEffectColorizeInner;
typedef const	struct FskEffectColorizeInnerRecord	*FskConstEffectColorizeInner;


/** Parameters for the Colorize Outer Effect.
 ** Note that we use alpha for opacity and do not have a separate, redundant opacity, as the kprEffects do.
 ** This is not intended to be used by the end user, because the SW implementation does not scale the matte while the GL implementation does.
 **/
struct FskEffectColorizeOuterRecord {
	FskConstBitmap		matte;														/**< Matte */
	FskColorRGBARecord	color;														/**< The color corresponding to alpha = one. */
};
typedef			struct FskEffectColorizeOuterRecord	FskEffectColorizeOuterRecord, *FskEffectColorizeOuter;
typedef const	struct FskEffectColorizeOuterRecord	*FskConstEffectColorizeOuter;


/** Parameters for the Colorize Alpha Effect.
 ** Note that we use alpha for opacity and do not have a separate, redundant opacity,
 ** as the kprEffects do.
 **/
struct FskEffectColorizeAlphaRecord {
	FskColorRGBARecord color0;														/**< The color corresponding to alpha = zero. */
	FskColorRGBARecord color1;														/**< The color corresponding to alpha = one. */
};
typedef			struct FskEffectColorizeAlphaRecord	FskEffectColorizeAlphaRecord, *FskEffectColorizeAlpha;
typedef const	struct FskEffectColorizeAlphaRecord	*FskConstEffectColorizeAlpha;


/** Parameters for the Colorize Effect.
 ** Note that we use alpha for opacity and do not have a separate, redundant opacity,
 ** as the kprEffects do.
 **/
struct FskEffectColorizeRecord {
	FskColorRGBARecord color;														/**< The color being applied. */
};
typedef			struct FskEffectColorizeRecord	FskEffectColorizeRecord, *FskEffectColorize;
typedef const	struct FskEffectColorizeRecord	*FskConstEffectColorize;


/** Parameters for the Compound Effect.
 **/
struct FskEffectCompoundRecord {
	int		topology;																/**< The topology of the compound effect. */
	int		numStages;																/**< The number of [additional] stages in the compound effect. */
};
typedef			struct FskEffectCompoundRecord	FskEffectCompoundRecord, *FskEffectCompound;
typedef const	struct FskEffectCompoundRecord	*FskConstEffectCompound;
enum {																				/**< The compound effect topology enumeration. */
	kFskEffectCompoundTopologyUnknown	= 0,										/**< The topology is not known. */
	kFskEffectCompoundTopologyPipeline												/**< The topology is a pipeline. */
};


/** Parameters for the Dilate Effect.
 ** Enlarge by the specified radius.
 **/
struct FskEffectDilateRecord {
	int		radius;																	/**< The radius of the dilation neighborhood. */
};
typedef       struct FskEffectDilateRecord	FskEffectDilateRecord, *FskEffectDilate;
typedef const struct FskEffectDilateRecord	*FskConstEffectDilate;


/** Parameters for the Erode Effect.
 ** Shrink by the specified radius.
 **/
struct FskEffectErodeRecord {
	int		radius;																	/**< The radius of the erosion neighborhood. */
};
typedef       struct FskEffectErodeRecord	FskEffectErodeRecord, *FskEffectErode;
typedef const struct FskEffectErodeRecord	*FskConstEffectErode;


/** Parameters for the Gaussian Blur Effect.
 ** Blur the src image with a Gaussian Blur.
 ** Note that there is a separate parameter for each of the horizontal and vertical directions.
 ** This is primarily because the two dimensional blur is implemented with two one dimensional blurs.
 ** As such, an intermediate buffer is required. If none is supplied in the FskEffectCache (i.e. it is NULL),
 ** then each invocation will incur an allocation and deallocation of intermediate buffers.
 ** Note that more than one intermediate buffer may be required in the following circumstances:
 ** - the srcRect is neither NULL nor the entire src bounds.
 ** - the GPU has implemented a low profile of OpenGL.
 ** - the src has no texture associated with it.
 **/
struct FskEffectGaussianBlurRecord {
	float	sigmaX;																	/**< The standard deviation of a Gaussian kernel used for blurring horizontally. */
	float	sigmaY;																	/**< The standard deviation of a Gaussian kernel used for blurring vertically.   */
};
typedef       struct FskEffectGaussianBlurRecord	FskEffectGaussianBlurRecord, *FskEffectGaussianBlur;
typedef const struct FskEffectGaussianBlurRecord	*FskConstEffectGaussianBlur;


/** Parameters for the Monochrome (Gray) Effect.
 ** The src image is converted to monochrome, then colorized by interpolating between color0 and color1.
 **/
struct FskEffectMonochromeRecord {
	FskColorRGBARecord color0;														/**< The color corresponding to black in luminance. */
	FskColorRGBARecord color1;														/**< The color corresponding to white in luminance. */
};
typedef			struct FskEffectMonochromeRecord	FskEffectMonochromeRecord, *FskEffectMonochrome;
typedef const	struct FskEffectMonochromeRecord	*FskConstEffectMonochrome;


/** Parameters for the InnerGlow Effect.
 ** The alpha of the src image is inset by the specified radius, then blurred with a Gaussian kernel, then colorized and blended
 ** with the src image using the
 **/
struct FskEffectInnerGlowRecord {
	int					radius;														/**< The glow is inset by this radius. */
	float				blurSigma;													/**< Blurred with this Gaussian sigma. */
	FskColorRGBARecord	color;														/**< And this color is applied. */
};
typedef			struct FskEffectInnerGlowRecord	FskEffectInnerGlowRecord, *FskEffectInnerGlow;
typedef const	struct FskEffectInnerGlowRecord	*FskConstEffectInnerGlow;


/** Parameters for the InnerShadow Effect.
 ** Note: there should be a transparent border of max(offset.x, offset.y) + blurSigma * 2.5.
 **/
struct FskEffectInnerShadowRecord {
	FskPointRecord		offset;														/**< The shadow is offset by this vector */
	float				blurSigma;													/**< Blurred with this Gaussian sigma. */
	FskColorRGBARecord	color;														/**< And this color is applied. */
};
typedef			struct FskEffectInnerShadowRecord	FskEffectInnerShadowRecord, *FskEffectInnerShadow;
typedef const	struct FskEffectInnerShadowRecord	*FskConstEffectInnerShadow;


/** Parameters for the Mask Effect.
 **/
struct FskEffectMaskRecord {
	FskConstBitmap		mask;														/**< The mask. */
	FskRectangleRecord	maskRect;													/**< Rect of the mask. */
};
typedef			struct FskEffectMaskRecord	FskEffectMaskRecord, *FskEffectMask;
typedef const	struct FskEffectMaskRecord	*FskConstEffectMask;


/** Parameters for the OuterGlow Effect.
 **/
struct FskEffectOuterGlowRecord {
	int					radius;														/**< The glow is outset by this radius. */
	float				blurSigma;													/**< Blurred with this Gaussian sigma. */
	FskColorRGBARecord	color;														/**< And this color is applied. */
};
typedef			struct FskEffectOuterGlowRecord	FskEffectOuterGlowRecord, *FskEffectOuterGlow;
typedef const	struct FskEffectOuterGlowRecord	*FskConstEffectOuterGlow;


/** Parameters for the OuterShadow Effect.
 ** Note: there should be a transparent border of max(offset.x, offset.y) + blurSigma * 2.5.
 **/
struct FskEffectOuterShadowRecord {
	FskPointRecord		offset;														/**< The shadow is offset by this vector */
	float				blurSigma;													/**< Blurred with this Gaussian sigma. */
	FskColorRGBARecord	color;														/**< And this color is applied. */
};
typedef			struct FskEffectOuterShadowRecord	FskEffectOuterShadowRecord, *FskEffectOuterShadow;
typedef const	struct FskEffectOuterShadowRecord	*FskConstEffectOuterShadow;


/** Parameters for the Shade Effect.
 **/
struct FskEffectShadeRecord {
	FskConstBitmap		shadow;														/**< The bitmap to be used as a pre-computed shadow. */
	FskRectangleRecord	shadowRect;													/**< Rect of the shadow. */
	UInt8				opacity;													/**< Opacity of the shadow. */
};
typedef			struct FskEffectShadeRecord	FskEffectShadeRecord, *FskEffectShade;
typedef const	struct FskEffectShadeRecord	*FskConstEffectShade;


/** Parameters for the Gel Effect.
 **/
struct FskEffectGelRecord {
	FskColorRGBARecord	color;														/**< This color modulates the source. */
};
typedef			struct FskEffectGelRecord	FskEffectGelRecord, *FskEffectGel;
typedef const	struct FskEffectGelRecord	*FskConstEffectGel;



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****					Parameters for elementary effects					*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/** Parameters for the 1 dimensional Box Blur Effect.
 ** Blur the src image with a Box Blur in the specified direction.
 **/
struct FskEffectDirectionalBoxBlurRecord {
	int		radius;																	/**< The radius of a Box kernel (width 2*radius+1) used for blurring in the specified direction. */
	float	direction[2];															/**< Direction of blur. This should be either (1,0) or (0,1), or at least normalized. */
};
typedef       struct FskEffectDirectionalBoxBlurRecord	FskEffectDirectionalBoxBlurRecord, *FskEffectDirectionalBoxBlur;
typedef const struct FskEffectDirectionalBoxBlurRecord	*FskConstEffectDirectionalBoxBlur;


/** Parameters for the 1 dimensional Gaussian Blur Effect.
 ** Blur the src image with a Gaussian Blur in the specified direction.
 **/
struct FskEffectDirectionalGaussianBlurRecord {
	float	sigma;																	/**< The standard deviation of a Gaussian kernel used for blurring in the specified direction. */
	float	direction[2];															/**< Direction of blur. This should be either (1,0) or (0,1), or at least normalized. */
};
typedef       struct FskEffectDirectionalGaussianBlurRecord	FskEffectDirectionalGaussianBlurRecord, *FskEffectDirectionalGaussianBlur;
typedef const struct FskEffectDirectionalGaussianBlurRecord	*FskConstEffectDirectionalGaussianBlur;


/** Parameters for the 1 dimensional Dilate Effect.
 ** Enlarge in the specified direction.
 **/
struct FskEffectDirectionalDilateRecord {
	int		radius;																	/**< The interval of the dilation neighborhood. */
	float	direction[2];															/**< Direction of dilation. This should be either (1,0) or (0,1), or at least normalized. */
};
typedef       struct FskEffectDirectionalDilateRecord	FskEffectDirectionalDilateRecord, *FskEffectDirectionalDilate;
typedef const struct FskEffectDirectionalDilateRecord	*FskConstEffectDirectionalDilate;


/** Parameters for the 1 dimensional Erode Effect.
 ** Shrink in the specified direction.
 **/
struct FskEffectDirectionalErodeRecord {
	int		radius;																	/**< The interval of the erosion neighborhood. */
	float	direction[2];															/**< Direction of erosion. This should be either (1,0) or (0,1), or at least normalized. */
};
typedef       struct FskEffectDirectionalErodeRecord	FskEffectDirectionalErodeRecord, *FskEffectDirectionalErode;
typedef const struct FskEffectDirectionalErodeRecord	*FskConstEffectDirectionalErode;


/** Parameters for the Effect to Convert from Straight Alpha to Premultiplied Alpha.
 **/
struct FskEffectPremultiplyAlphaRecord {
	UInt32 unused;																	/**< C requires that a structure or union has at least one member */
};
typedef       struct FskEffectPremultiplyAlphaRecord	FskEffectPremultiplyAlphaRecord, *FskEffectPremultiplyAlpha;
typedef const struct FskEffectPremultiplyAlphaRecord	*FskConstEffectPremultiplyAlpha;


/** Parameters for the Effect to Convert from Premultiplied Alpha to Straight Alpha.
 **/
struct FskEffectStraightAlphaRecord {
	FskColorRGBRecord	color;														/**< The color to be used when alpha is zero. */
};
typedef       struct FskEffectStraightAlphaRecord	FskEffectStraightAlphaRecord, *FskEffectStraightAlpha;
typedef const struct FskEffectStraightAlphaRecord	*FskConstEffectStraightAlpha;



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Generic effects								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/** Union of all effects parameters.
 ** This is typically used as part of an FskEffectRecord.
 **/
union FskEffectParametersRecord {
	FskEffectBoxBlurRecord					boxBlur;
	FskEffectColorizeAlphaRecord			colorizeAlpha;
	FskEffectColorizeInnerRecord			colorizeInner;
	FskEffectColorizeOuterRecord			colorizeOuter;
	FskEffectColorizeRecord					colorize;
	FskEffectCompoundRecord					compound;
	FskEffectCopyMirrorBordersRecord		copyMirrorBorders;
	FskEffectCopyRecord						copy;
	FskEffectDilateRecord					dilate;
	FskEffectDirectionalBoxBlurRecord		directionalBoxBlur;
	FskEffectDirectionalDilateRecord		directionalDilate;
	FskEffectDirectionalErodeRecord			directionalErode;
	FskEffectDirectionalGaussianBlurRecord	directionalGaussianBlur;
	FskEffectErodeRecord					erode;
	FskEffectGaussianBlurRecord				gaussianBlur;
	FskEffectGelRecord						gel;
	FskEffectInnerGlowRecord				innerGlow;
	FskEffectInnerShadowRecord				innerShadow;
	FskEffectMaskRecord						mask;
	FskEffectMonochromeRecord				monochrome;
	FskEffectOuterGlowRecord				outerGlow;
	FskEffectOuterShadowRecord				outerShadow;
	FskEffectPremultiplyAlphaRecord			premultiplyAlpha;
	FskEffectShadeRecord					shade;
	FskEffectStraightAlphaRecord			straightAlpha;
};
typedef       union FskEffectParametersRecord		FskEffectParametersRecord, *FskEffectParameters;
typedef const union FskEffectParametersRecord		*FskConstEffectParameters;

/** Data structure to encapsulate an entire effect.
 ** This is typically used in a call to a generic effect invocation.
 **/
struct FskEffectRecord {
	FskEffectID					effectID;												/**< Effect id (selector) */
	FskEffectParametersRecord	params;													/**< The parameters. */
};
typedef       struct FskEffectRecord	FskEffectRecord, *FskEffect;
typedef const struct FskEffectRecord	*FskConstEffect;


/*
 * Convenience methods for setting effects parameters. The prototypes are mainly for reference, because the compiler comes after the preprocessor.
 */

void FskEffectBoxBlurSet(FskEffect p, int radiusX, int radiusY);
	#define FskEffectBoxBlurSet(p, rx, ry)	\
	do { (p)->effectID = kFskEffectBoxBlur; (p)->params.boxBlur.radiusX = (rx); (p)->params.boxBlur.radiusY = (ry); } while(0)

void FskEffectColorizeAlphaSet(FskEffect p, FskColorRGBA color0, FskColorRGBA color1);
	#define FskEffectColorizeAlphaSet(p, c0, c1)	\
	do { (p)->effectID = kFskEffectColorizeAlpha; (p)->params.colorizeAlpha.color0 = *(c0); (p)->params.colorizeAlpha.color0 = *(c1); } while(0)
void FskEffectColorizeAlphaScalarSet(FskEffect p, UInt8 r0, UInt8 g0, UInt8 b0, UInt8 a0, UInt8 r1, UInt8 g1, UInt8 b1, UInt8 a1);
	#define FskEffectColorizeAlphaScalarSet(p, r0, g0, b0, a0, r1, g1, b1, a1)	\
	do { (p)->effectID = kFskEffectColorizeAlpha; FskColorRGBASet(&(p)->params.colorizeAlpha.color0, r0, g0, b0, a0); FskColorRGBASet(&(p)->params.colorizeAlpha.color1, r1, g1, b1, a1); } while(0)

void FskEffectColorizeSet(FskEffect p, FskColorRGBA color);
	#define FskEffectColorizeSet(p, c)	\
	do { (p)->effectID = kFskEffectColorize; (p)->params.colorize.color = *(c); } while(0)
void FskEffectColorizeScalarSet(FskEffect p, UInt8 r, UInt8 g, UInt8 b, UInt8 a);
	#define FskEffectColorizeScalarSet(p, R, G, B, A)	\
	do { (p)->effectID = kFskEffectColorize; FskColorRGBASet(&(p)->params.colorize.color, R, G, B, A); } while(0)

void FskEffectColorizeInnerSet(FskEffect p, FskConstBitmap matte, FskColorRGBA color);
	#define FskEffectColorizeInnerSet(p, m, c)	\
	do { (p)->effectID = kFskEffectColorizeInner; (p)->params.colorizeInner.matte = (m); (p)->params.colorizeInner.color = *(c); } while(0)
void FskEffectColorizeInnerScalarSet(FskEffect p, FskConstBitmap matte, UInt8 r, UInt8 g, UInt8 b, UInt8 a);
	#define FskEffectColorizeInnerScalarSet(p, m, R, G, B, A)	\
	do { (p)->effectID = kFskEffectColorizeInner; (p)->params.colorizeInner.matte = (m); FskColorRGBASet(&(p)->params.colorizeInner.color, R, G, B, A); } while(0)

void FskEffectColorizeOuterSet(FskEffect p, FskConstBitmap matte, FskColorRGBA color);
	#define FskEffectColorizeOuterSet(p, m, c)	\
	do { (p)->effectID = kFskEffectColorizeOuter; (p)->params.colorizeOuter.matte = (m); (p)->params.colorizeOuter.color = *(c); } while(0)
void FskEffectColorizeOuterScalarSet(FskEffect p, FskConstBitmap matte, UInt8 r, UInt8 g, UInt8 b, UInt8 a);
	#define FskEffectColorizeOuterScalarSet(p, m, R, G, B, A)	\
	do { (p)->effectID = kFskEffectColorizeOuter; (p)->params.colorizeOuter.matte = (m); FskColorRGBASet(&(p)->params.colorizeOuter.color, R, G, B, A); } while(0)

void FskEffectCompoundSet(FskEffect p, int topology, int numStages);
	#define FskEffectCompoundSet(p, t, n)	\
	do { (p)->effectID = kFskEffectCompound; (p)->params.compound.topology = (t); (p)->params.compound.numStages = (n); } while(0)

void FskEffectCopyMirrorBordersSet(FskEffect p, UInt32 border);
	#define FskEffectCopyMirrorBordersSet(p, b)	\
	do { (p)->effectID = kFskEffectCopyMirrorBorders; (p)->params.copyMirrorBorders.border = (b); } while(0)

void FskEffectCopySet(FskEffect p);
	#define FskEffectCopySet(p)	\
	do { (p)->effectID = kFskEffectCopy; } while(0)

void FskEffectDilateSet(FskEffect p, int radius);
	#define FskEffectDilateSet(p, r)	\
	do { (p)->effectID = kFskEffectDilate; (p)->params.dilate.radius = (r); } while(0)

void FskEffectDirectionalBoxBlurSet(FskEffect p, float radius, float dirX, float dirY);
	#define FskEffectDirectionalBoxBlurSet(p, r, dx, dy)	\
	do { (p)->effectID = kFskEffectDirectionalBoxBlur; (p)->params.directionalBoxBlur.radius = (r); (p)->params.directionalBoxBlur.direction[0] = (dx); (p)->params.directionalGaussianBlur.direction[1] = (dy); } while(0)

void FskEffectDirectionalDilateSet(FskEffect p, int radius, float dirX, float dirY);
	#define FskEffectDirectionalDilateSet(p, r, dx, dy)	\
	do { (p)->effectID = kFskEffectDirectionalDilate; (p)->params.directionalDilate.radius = (r); (p)->params.directionalDilate.direction[0] = (dx); (p)->params.directionalDilate.direction[1] = (dy); } while(0)

void FskEffectDirectionalErodeSet(FskEffect p, int radius, float dirX, float dirY);
	#define FskEffectDirectionalErodeSet(p, r, dx, dy)	\
	do { (p)->effectID = kFskEffectDirectionalErode; (p)->params.directionalErode.radius = (r); (p)->params.directionalErode.direction[0] = (dx); (p)->params.directionalErode.direction[1] = (dy); } while(0)

void FskEffectDirectionalGaussianBlurSet(FskEffect p, float sigma, float dirX, float dirY);
	#define FskEffectDirectionalGaussianBlurSet(p, s, dx, dy)	\
	do { (p)->effectID = kFskEffectDirectionalGaussianBlur; (p)->params.directionalGaussianBlur.sigma = (s); (p)->params.directionalGaussianBlur.direction[0] = (dx); (p)->params.directionalGaussianBlur.direction[1] = (dy); } while(0)

void FskEffectErodeSet(FskEffect p, int radius);
	#define FskEffectErodeSet(p, r)	\
	do { (p)->effectID = kFskEffectErode; (p)->params.erode.radius = (r); } while(0)

void FskEffectGaussianBlurSet(FskEffect p, float sigmaX, float sigmaY);
	#define FskEffectGaussianBlurSet(p, sx, sy)	\
	do { (p)->effectID = kFskEffectGaussianBlur; (p)->params.gaussianBlur.sigmaX = (sx); (p)->params.gaussianBlur.sigmaY = (sy); } while(0)

void FskEffectGelSet(FskEffect p, FskConstColorRGBA color);
	#define FskEffectGelSet(p, c)	\
	do { (p)->effectID = kFskEffectGel; (p)->params.gel.color = *(c); } while(0)
void FskEffectGelScalarSet(FskEffect p, UInt8 r, UInt8 g, UInt8 b, UInt8 a);
	#define FskEffectGelScalarSet(p, R, G, B, A)	\
	do { (p)->effectID = kFskEffectGel; FskColorRGBASet(&(p)->params.gel.color, R, G, B, A); } while(0)

void FskEffectInnerGlowSet(FskEffect p, int radius, float blurSigma, FskConstColorRGBA color);
	#define FskEffectInnerGlowSet(p, r, b, c)	\
	do { (p)->effectID = kFskEffectInnerGlow; (p)->params.innerGlow.radius = (r); (p)->params.innerGlow.blurSigma = (b); (p)->params.innerGlow.color = *(c); } while(0)
void FskEffectInnerGlowScalarSet(FskEffect p, int radius, float blurSigma, UInt8 r, UInt8 g, UInt8 b, UInt8 a);
	#define FskEffectInnerGlowScalarSet(p, r, b, R, G, B, A)	\
	do { (p)->effectID = kFskEffectInnerGlow; (p)->params.innerGlow.radius = (r); (p)->params.innerGlow.blurSigma = (b); FskColorRGBASet(&(p)->params.innerGlow.color, R, G, B, A); } while(0)

void FskEffectInnerShadowSet(FskEffect p, FskConstPoint offset, float blurSigma, FskConstColorRGBA color);
	#define FskEffectInnerShadowSet(p, o, b, c)	\
	do { (p)->effectID = kFskEffectInnerShadow; (p)->params.innerShadow.offset = *(o); (p)->params.innerShadow.blurSigma = (b); (p)->params.innerShadow.color = *(c); } while(0)
void FskEffectInnerShadowScalarSet(FskEffect p, int dx, int dy, float blurSigma, UInt8 r, UInt8 g, UInt8 b, UInt8 a);
	#define FskEffectInnerShadowScalarSet(p, dx, dy, b, R, G, B, A)	\
	do { (p)->effectID = kFskEffectInnerShadow; FskPointSet(&(p)->params.innerShadow.offset, dx, dy); (p)->params.innerShadow.blurSigma = (b); FskColorRGBASet(&(p)->params.innerShadow.color, R, G, B, A); } while(0)

void FskEffectMaskSet(FskEffect p, FskConstBitmap mask, FskConstRectangle maskRect);
	#define FskEffectMaskSet(p, m, r)	\
	do { (p)->effectID = kFskEffectMask; (p)->params.mask.mask = (m); (p)->params.mask.maskRect = ((r) != NULL) ? *(FskConstRectangle)(r) : (m)->bounds; } while(0)
void FskEffectMaskScalarSet(FskEffect p, FskConstBitmap mask, SInt32 x, SInt32 y, SInt32 width, SInt32 height);
	#define FskEffectMaskScalarSet(p, m, x, y, w, h)	\
	do { (p)->effectID = kFskEffectMask; (p)->params.mask.mask = (m); FskRectangleSet(&(p)->params.mask.maskRect, (x), (y), (w), (h)); } while(0)

void FskEffectMonochromeSet(FskEffect p, FskColorRGBA c0, FskColorRGBA c1);
	#define FskEffectMonochromeSet(p, c0, c1)	\
	do { (p)->effectID = kFskEffectMonochrome; (p)->params.monochrome.color0 = *(c0); (p)->params.monochrome.color1 = *(c1); } while(0)
void FskEffectMonochromeScalarSet(FskEffect p, UInt8 r0, UInt8 g0, UInt8 b0, UInt8 a0, UInt8 r1, UInt8 g1, UInt8 b1, UInt8 a1);
	#define FskEffectMonochromeScalarSet(p, r0, g0, b0, a0, r1, g1, b1, a1)	\
	do { (p)->effectID = kFskEffectMonochrome; FskColorRGBASet(&(p)->params.monochrome.color0, r0, g0, b0, a0); FskColorRGBASet(&(p)->params.monochrome.color1, r1, g1, b1, a1); } while(0)

void FskEffectOuterGlowSet(FskEffect p, int radius, float blurSigma, FskConstColorRGBA color);
	#define FskEffectOuterGlowSet(p, r, b, c)	\
	do { (p)->effectID = kFskEffectOuterGlow; (p)->params.outerGlow.radius = (r); (p)->params.outerGlow.blurSigma = (b); (p)->params.outerGlow.color = *(c); } while(0)
void FskEffectOuterGlowScalarSet(FskEffect p, int radius, float blurSigma, UInt8 r, UInt8 g, UInt8 b, UInt8 a);
	#define FskEffectOuterGlowScalarSet(p, r, b, R, G, B, A)	\
	do { (p)->effectID = kFskEffectOuterGlow; (p)->params.outerGlow.radius = (r); (p)->params.outerGlow.blurSigma = (b); FskColorRGBASet(&(p)->params.outerGlow.color, R, G, B, A); } while(0)

void FskEffectOuterShadowSet(FskEffect p, FskConstPoint offset, float blurSigma, FskConstColorRGBA color);
	#define FskEffectOuterShadowSet(p, o, b, c)	\
	do { (p)->effectID = kFskEffectOuterShadow; (p)->params.outerShadow.offset = *(o); (p)->params.outerShadow.blurSigma = (b); (p)->params.outerShadow.color = *(c); } while(0)
void FskEffectOuterShadowScalarSet(FskEffect p, int dx, int dy, float blurSigma, UInt8 r, UInt8 g, UInt8 b, UInt8 a);
	#define FskEffectOuterShadowScalarSet(p, dx, dy, b, R, G, B, A)	\
	do { (p)->effectID = kFskEffectOuterShadow; FskPointSet(&(p)->params.outerShadow.offset, dx, dy); (p)->params.outerShadow.blurSigma = (b); FskColorRGBASet(&(p)->params.outerShadow.color, R, G, B, A); } while(0)

void FskEffectPremultiplyAlphaSet(FskEffect p);
	#define FskEffectPremultiplyAlphaSet(p)	\
	do { (p)->effectID = kFskEffectPremultiplyAlpha; } while(0)

void FskEffectShadeSet(FskEffect p, FskConstBitmap shadow, FskConstRectangle shadowRect, UInt8 opacity);
	#define FskEffectShadeSet(p, s, r, o)	\
	do { (p)->effectID = kFskEffectShade; (p)->params.shade.shadow = (s); (p)->params.shade.shadowRect = ((r) != NULL) ? *(FskConstRectangle)(r) : (s)->bounds; (p)->params.shade.opacity = (o); } while(0)
void FskEffectShadeScalarSet(FskEffect p, FskConstBitmap shadow, SInt32 x, SInt32 y, SInt32 width, SInt32 height, UInt8 opacity);
	#define FskEffectShadeScalarSet(p, s, x, y, w, h, o)	\
	do { (p)->effectID = kFskEffectShade; (p)->params.shade.shadow = (s); FskRectangleSet(&(p)->params.shade.shadowRect, (x), (y), (w), (h)); (p)->params.shade.opacity = (o); } while(0)

void FskEffectStraightAlphaSet(FskEffect p, FskConstColorRGB c);
	#define FskEffectStraightAlphaSet(p, c)	\
	do { (p)->effectID = kFskEffectStraightAlpha; (p)->params.straightAlpha.color = *(c); } while(0)
void FskEffectStraightAlphaScalarSet(FskEffect p, UInt8 r, UInt8 g, UInt8 b, UInt8 a);
	#define FskEffectStraightAlphaScalarSet(p, r, g, b)	\
	do { (p)->effectID = kFskEffectStraightAlpha; FskColorRGBSet(&(p)->params.straightAlpha.color, (r), (g), (b)); } while(0)


struct FskEffectCacheRecord;								/**< Opaque record for the effect cache. */
typedef struct FskEffectCacheRecord FskEffectCacheRecord;	/**< Opaque  type  for the effect cache. */
typedef struct FskEffectCacheRecord *FskEffectCache;		/**< Pointer to an effect cache. */
#define kFskGLEffectCacheBitmapWithAlpha	0x1				/** Indicates to FskGLEffectCacheGetBitmap() that alpha is needed in the bitmap. */
#define kFskGLEffectCacheBitmapInit			0x2				/** After allocation, FskGLEffectCacheGetBitmap() initialize the bitmap with glClear(). */

/** Create a new GL Effect cache.
 **	\param[in]	pCache		pointer to a place to store an effect cache.
 **	\return		kFskErrNone	if the cache was created successfully.
 **/
FskAPI(FskErr) FskGLEffectCacheNew(FskEffectCache *pCache);

/** Dispose a GL Effect cache.
 ** All of the bitmaps contained in tha cache are also disposed.
 **	\param[in]	cache		the cache to be disposed.
 **	\return		kFskErrNone	if the cache was disposed successfully.
 **/
FskAPI(FskErr) FskGLEffectCacheDispose(FskEffectCache cache);

/** Get a bitmap from the GL Effect cache.
 ** If there are no acceptable bitmaps in the cache, FskBitmapNew() is called.
 ** To the caller of this proc, the bitmaps returned by FskGLEffectCacheGetBitmap() and FskBitmapNew() are identical.
 ** When done, call FskGLEffectCacheReleaseBitmap() to put the bitmap into the cache, or FskBitmpDispose() to dispose of it immediately.
 **	\param[in]	cache	the effect cache.
 **	\param[in]	width	the desired width  of the bitmap.
 **	\param[in]	height	the desired height of the bitmap.
 **	\param[in]	flags	flags: a bitwise OR of { kFskGLEffectCacheBitmapWithAlpha, kFskGLEffectCacheBitmapInit }.
 **	\param[out]	bmp		the effect cache.
 **	\return		kFskErrNone	if the bitmap was returned successfully.
 **/
FskAPI(FskErr) FskGLEffectCacheGetBitmap(FskEffectCache cache, unsigned width, unsigned height, int flags, FskBitmap *bmp);

/** Release a bitmap back to the cache.
 ** The bitmap must have a useCount of 0 in order to be returned to the cache.
 ** If the useCount is not zero, FskBitmapDispose() is called instead and an error is returned.
 **	If the pixelFormat is not of the type collected in the cache, FskBitmapDispose() is called instead and an error is returned.
 **	It is not an error to release a GLRGBA bitmap allocated with FskBitmapNew().
 **	\param[in]	cache			the effect cache.
 **	\return		kFskErrNone					if the bitmap was cached successfully.
 **	\return		kFskErrIsBusy				if the bitmap has a nonzero use count.
 **	\return		kFskErrUnsupportedPixelType	if the bitmap is not of the type collected in the cache.
 **/
FskAPI(FskErr) FskGLEffectCacheReleaseBitmap(FskEffectCache cache, FskBitmap bm);



/** Software implementation of Effects.
 **	\param[in]	effect		The parameters for the effect, including its name.
 **	\param[in]	src			The source bitmap.
 **	\param[in]	srcRect		A rectangle specifying a sunset of the src bitmap. NULL implies the whole bitmap.
 **	\param[in]	dst			The destination bitmap.
 **	\param[in]	dstPoint	The location where the result is to be placed in the dst bitmap.
 **	\return		kFskErrNone	If the operation was comopleted successfully.
 **/
FskAPI(FskErr) FskEffectApply(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint);


/** OpenGL implementation of Effects.
 **	\param[in]	effect		The parameters for the effect, including its name.
 **	\param[in]	src			The source texture.
 **	\param[in]	srcRect		A rectangle specifying a sunset of the src texture. NULL implies the whole texture.
 **	\param[in]	dst			The destination texture.
 **	\param[in]	dstPoint	The location where the result is to be placed.
 **	\param[in]	cache		A cache of reusable textures. NULL causes a temporary cache to be created when needed.
 **	\return		kFskErrNone	If the operation was comopleted successfully.
 **/
FskAPI(FskErr) FskGLEffectApply(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache);


/** OpenGL implementation of Effects -- with mode.
 ** No blending and only 2 modes are accommodated:		kFskGraphicsModeCopy and kFskGraphicsModeAlpha.
 ** This will work correctly for some effects:			kFskEffectColorize, kFskEffectColorizeAlpha, kFskEffectCopy, kFskEffectCopyMirrorBorders, kFskEffectDirectionalBoxBlur,
 **														kFskEffectDirectionalDilate, kFskEffectDirectionalErode, kFskEffectDirectionalGaussianBlur, kFskEffectMonochrome.
 ** This may work correctly for some effects:			kFskEffectColorizeInner, kFskEffectColorizeOuter.
 ** depending on the implementation.
 ** This will never work correctly for most effects:	kFskEffectBoxBlur, kFskEffectCompound, kFskEffectDilate, kFskEffectErode, kFskEffectGaussianBlur,
 **														kFskEffectMask, kFskEffectShade, kFskEffectInnerGlow, kFskEffectInnerShadow, kFskEffectOuterGlow,
 **														kFskEffectOuterShadow.
 **	\param[in]	effect		The parameters for the effect, including its name.
 **	\param[in]	src			The source texture.
 **	\param[in]	srcRect		A rectangle specifying a sunset of the src texture. NULL implies the whole texture.
 **	\param[in]	dst			The destination texture.
 **	\param[in]	dstPoint	The location where the result is to be placed.
 **	\param[in]	cache		A cache of reusable textures. NULL causes a temporary cache to be created when needed.
 **	\param[in]	clipRect	The clip rectangle, or NULL if no clipping.
 **	\param[in]	mode		The desired blending mode. Only kFskGraphicsModeCopy and kFskGraphicsModeAlpha are accommodated.
 **	\param[in]	modeParams	mode parameters. If modeParams->blendLevel < 255, returns kfskErrUnimplemented.
 **	\return		kFskErrNone				if the operation was comopleted successfully.
 **	\return		kfskErrUnimplemented	if the specified mode is not implemented, or if modeParams->blendLevel < 255.
 ** \note		This does not have an equivalent software API.
 **/
FskAPI(FskErr) FskGLEffectApplyMode(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache,
									FskConstRectangle clipRect, UInt32 mode, FskConstGraphicsModeParameters modeParams);


/**	Initialization of the GL Effects.
 **	\return	kFskErrNone		If the effects were initialized successfully.
 **/
FskAPI(FskErr) FskGLEffectsInit(void);


/** Shutdown of the GL Effects.
 **/
FskAPI(void) FskGLEffectsShutdown(void);


/** Determine the [1-dimensional] radius of pixels incorporated into a Gaussian blur, to yield an error of less than 1.28% as compared to an infinite radius.
 ** The total number of pixels is 1+2*radius, i.e. the central pixel plus radius pixels on either side.
 **	\param[in]	sigma	the Gaussian sigma.
 **	\return		the radius of pixels actually incorporated into the blur.
 **/
int	FskEffectGaussianBlurRadiusFromSigma(float sigma);
	#define FskEffectGaussianBlurRadiusFromSigma(s)			(int)((sigma) * 2.5f)


/** Convert from a Gaussian sigma to a Box blur radius that yields a vaguely similar frequency response.
 ** The box blur is a lower Q filter than Gaussian, with extra attenuation in the passband and leakage in the stopband,
 ** but is considerably simpler computationally and has higher performance.
 ** The computational complexity of the software implementation is O(1) in the blur radius,
 ** (e.g. it takes as long to blur with a radius of 10 as a radius of 100, at least away from the edges),
 ** so it is especially attractive for large blurs.
 **	\param[in]	sigma	the Gaussian sigma.
 **	\return		the radius of a box blur that has similar low pass features as a Gaussian blur.
 **	\bug		the radius is quantized, so the similarity is to a Gaussian sigma equivalent to this quantized radius.
 **/
int FskEffectBoxRadiusFromGaussianSigma(float sigma);
	#define FskEffectBoxRadiusFromGaussianSigma(s)			(int)((s) * 1.7320508f)


/** For larger Gaussian sigma (greater than 5 or 10), a box blur applied three times is visually indistinguishable from the Gaussian.
 **	This function computes a box blur radius that is visually equivalent to a Gaussian blur, when the box blur is applied three times in succession.
 **	\param[in]	sigma	the Gaussian sigma.
 **	\return		the radius of a box blur that has similar low pass features as a Gaussian blur, when the box blur is applied three times.
 **	\bug		the radius is quantized, so the similarity is to a Gaussian sigma equivalent to this quantized radius.
 **/
int FskEffectBoxThriceRadiusFromGaussianSigma(float sigma);
	#define FskEffectBoxThriceRadiusFromGaussianSigma(s)	(int)((sigma) * 0.9523800f)


#ifdef __FSKEFFECTS_PRIV__
struct FskEffectCacheRecord {
	UInt32		numBitmaps;
	FskBitmap	*bitmaps;
};
#endif /* __FSKEFFECTS_PRIV__ */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKEFFECTS__ */
