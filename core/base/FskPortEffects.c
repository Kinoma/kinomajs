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
	\file	FskPortEffects.c
	\brief	Implementation of effects for the port.
*/

#define __FSKBITMAP_PRIV__
#define __FSKPORT_PRIV__
#include "Fsk.h"
#if SUPPORT_INSTRUMENTATION
	#define __FSKGLBLIT_PRIV__
	#include "FskPixelOps.h"
#endif /* SUPPORT_INSTRUMENTATION */
#include "FskPort.h"
#include "FskEffects.h"
#include "FskMemory.h"
#if FSKBITMAP_OPENGL
	#include "FskGLBlit.h"
#endif /* FSKBITMAP_OPENGL */


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								UTILITIES								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

#if SUPPORT_INSTRUMENTATION
	extern FskInstrumentedTypeRecord gPortTypeInstrumentation;
#endif /* SUPPORT_INSTRUMENTATION */

#define CHECK_EFFECT_PARAMETERS
#if defined(CHECK_EFFECT_PARAMETERS) && SUPPORT_INSTRUMENTATION && FSKBITMAP_OPENGL
	static void ASSERT_PARAM_LE(double param, double max) { if (param > max) FskInstrumentedTypePrintfDebug(&gPortTypeInstrumentation, "Effect parameter %g exceeds %g: quality is degraded in GL", param, max); }
#else /* !CHECK_EFFECT_PARAMETERS */
	#define ASSERT_PARAM_LE(param, max)
#endif /* !CHECK_EFFECT_PARAMETERS */


/****************************************************************************//**
 * Scale simple effect parameters by the given scale.
 * This is a helper function for FskPortEffectScale() and is not intended to be invoked directly,
 * because it only scales simple effects, not compound effects.
 *	\param[in]		scale	the scale factor, in 16.16 fixed point.
 *	\param[in,out]	effect	the effect to be scaled.
 ********************************************************************************/

static void ScaleEffectParams(FskFixed scale, FskEffect effect) {
	const SInt32	xScale1 = scale >> (16 - 1);					/* Convert to 1 fractional bit, accommodates 0.5, 1.0, 1.5, 2.0, 2.5, ... */
	const float		fScale  = scale * (1.f / 65536.f);				/* Convert scale to floating-point */

	switch (effect->effectID) {
		case kFskEffectColorize:
		case kFskEffectColorizeAlpha:
		case kFskEffectColorizeInner:
		case kFskEffectColorizeOuter:
		case kFskEffectCopy:
		case kFskEffectCopyMirrorBorders:
		case kFskEffectGel:
		case kFskEffectMask:
		case kFskEffectMonochrome:
		case kFskEffectShade:
			break;																								/* Nothing needs to be scaled */

		case kFskEffectBoxBlur:
			effect->params.boxBlur.radiusX = (effect->params.boxBlur.radiusX * xScale1) >> 1;
			effect->params.boxBlur.radiusY = (effect->params.boxBlur.radiusY * xScale1) >> 1;
			ASSERT_PARAM_LE(effect->params.boxBlur.radiusX, 32);												/* Cannot be larger than  32  for GL */
			ASSERT_PARAM_LE(effect->params.boxBlur.radiusY, 32);												/* Cannot be larger than  32  for GL */
			break;

		case kFskEffectDilate:
			effect->params.dilate.radius = (effect->params.dilate.radius * xScale1) >> 1;
			ASSERT_PARAM_LE(effect->params.dilate.radius, 32);													/* Cannot be larger than  32  for GL */
			break;

		case kFskEffectDirectionalBoxBlur:
			effect->params.directionalBoxBlur.radius = (effect->params.directionalBoxBlur.radius * xScale1) >> 1;
			ASSERT_PARAM_LE(effect->params.directionalBoxBlur.radius, 32);										/* Cannot be larger than  32  for GL */
			break;

		case kFskEffectDirectionalDilate:
			effect->params.directionalDilate.radius = (effect->params.directionalDilate.radius * xScale1) >> 1;
			ASSERT_PARAM_LE(effect->params.directionalDilate.radius, 32);										/* Cannot be larger than  32  for GL */
			break;

		case kFskEffectDirectionalErode:
			effect->params.directionalErode.radius = (effect->params.directionalErode.radius * xScale1) >> 1;
			ASSERT_PARAM_LE(effect->params.directionalErode.radius, 32);										/* Cannot be larger than  32  for GL */
			break;

		case kFskEffectDirectionalGaussianBlur:
			effect->params.directionalGaussianBlur.sigma *= fScale;
			ASSERT_PARAM_LE(effect->params.directionalGaussianBlur.sigma, 12.8);								/* Cannot be larger than 12.8 for GL */
			break;

		case kFskEffectErode:
			effect->params.erode.radius = (effect->params.erode.radius * xScale1) >> 1;
			ASSERT_PARAM_LE(effect->params.erode.radius, 32);													/* Cannot be larger than  32  for GL */
			break;

		case kFskEffectGaussianBlur:
			effect->params.gaussianBlur.sigmaX *= fScale;
			effect->params.gaussianBlur.sigmaY *= fScale;
			ASSERT_PARAM_LE(effect->params.gaussianBlur.sigmaX, 12.8);											/* Cannot be larger than 12.8 for GL */
			ASSERT_PARAM_LE(effect->params.gaussianBlur.sigmaY, 12.8);											/* Cannot be larger than 12.8 for GL */
			break;

		case kFskEffectInnerGlow:
			effect->params.innerGlow.radius = (effect->params.innerGlow.radius * xScale1) >> 1;
			effect->params.innerGlow.blurSigma *= fScale;
			ASSERT_PARAM_LE(effect->params.innerGlow.radius,    32);											/* Cannot be larger than  32  for GL */
			ASSERT_PARAM_LE(effect->params.innerGlow.blurSigma, 12.8);											/* Cannot be larger than 12.8 for GL */
			break;

		case kFskEffectInnerShadow:
			effect->params.innerShadow.offset.x = (effect->params.innerShadow.offset.x * xScale1) >> 1;			/* No restriction on offset */
			effect->params.innerShadow.offset.y = (effect->params.innerShadow.offset.y * xScale1) >> 1;			/* No restriction on offset */
			effect->params.innerShadow.blurSigma *= fScale;
			ASSERT_PARAM_LE(effect->params.innerShadow.blurSigma, 12.8);										/* Cannot be larger than 12.8 for GL */
			break;

		case kFskEffectOuterGlow:
			effect->params.outerGlow.radius = (effect->params.outerGlow.radius * xScale1) >> 1;
			effect->params.outerGlow.blurSigma *= fScale;
			ASSERT_PARAM_LE(effect->params.outerGlow.radius,    32);											/* Cannot be larger than  32  for GL */
			ASSERT_PARAM_LE(effect->params.outerGlow.blurSigma, 12.8);											/* Cannot be larger than 12.8 for GL */
			break;

		case kFskEffectOuterShadow:
			effect->params.outerShadow.offset.x = (effect->params.outerShadow.offset.x * xScale1) >> 1;			/* No restriction on offset*/
			effect->params.outerShadow.offset.y = (effect->params.outerShadow.offset.y * xScale1) >> 1;			/* No restriction on offset */
			effect->params.outerShadow.blurSigma *= fScale;
			ASSERT_PARAM_LE(effect->params.outerShadow.blurSigma, 12.8);										/* Cannot be larger than 12.8 for GL */
			break;

		default:
			FskInstrumentedTypePrintfMinimal(&gPortTypeInstrumentation, "Don't know how to scale unknown effect #%d", (int)effect->effectID);    /* Unknown effect */
			break;
	}
}


/********************************************************************************
 * Scale port effect parameters by the current port scale and offset by the current origin.
 *	\param[in]		port		the scale factor, in 16.16 fixed point.
 *	\param[in,out]	portEffect	the port effect to be scaled and offset.
 ********************************************************************************/

void FskPortEffectScale(FskPort port, struct FskPortPicEffectParametersRecord *portEffect) {
	int			n;
	FskEffect	e;

	portEffect->dstPoint.x += port->origin.x;
	portEffect->dstPoint.y += port->origin.y;
	if (port->scale == (1 << 16))
		return;																						/* If the scale is 1, we only need to apply the port offset */

	portEffect->dstPoint.x = FskPortSInt32Scale(port, portEffect->dstPoint.x);						/* Scale the dstPoint after offset */
	portEffect->dstPoint.y = FskPortSInt32Scale(port, portEffect->dstPoint.y);

	e = &portEffect->effect;
	n = (kFskEffectCompound == e->effectID) ? (e++)->params.compound.numStages : 1;
	while (n--)
		ScaleEffectParams(port->scale, e++);
}


#if FSKBITMAP_OPENGL && GLES_VERSION == 2 /*  OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL */


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							OPEN GL EFFECTS								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/****************************************************************************//**
 * Get the port clip region suitable for use in the effects.
 * It is necessary to scale this rectangle by the current port scale.
 *	\param[in]	port	the port.
 *	\param[out]	cr		a place to store the desired clip rectangle.
 *	\return		cr		return the parameter cr, to allow GetPortEffectClip() to be used as a parameter.
 *						in the invocation of another function.
 ********************************************************************************/

static FskRectangle GetPortEffectClip(FskPort port, FskRectangle cr) {
	*cr = port->aggregateClip;
	FskPortRectScale(port, cr);
	return cr;
}


/****************************************************************************//**
 * Get a GL bitmap from the GLEffects cache, to encourage reuse of intermediate textures.
 *	\param[in]	port	the port, keeper of the cache.
 *	\param[in]	width	the desired width of the bitmap.
 *	\param[in]	height	the desired height of the bitmap.
 *	\param[in]	flags	Logical OR of { 0, kFskGLEffectCacheBitmapWithAlpha, kFskGLEffectCacheBitmapInit }.
 *	\param[in]	bmp		a place to store the resultant bitmap.
 *	\return		kFskErrNone	if the operation was successful.
 ********************************************************************************/

static FskErr GLPortEffectGetCacheBitmap(FskPort port, SInt32 width, SInt32 height, int flags, FskBitmap *bmp) {
	return FskGLEffectCacheGetBitmap(width, height, flags/* | kFskGLEffectCacheBitmapInit*/, bmp);
}


/********************************************************************************
 * EffectPostCopyPrepare
 ********************************************************************************/

static FskErr EffectPostCopyPrepare(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskBitmap		mid1	= NULL;
	int				flags	= portEffect->src->hasAlpha ? kFskGLEffectCacheBitmapWithAlpha : 0;
	FskErr			err;

	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectPostCopyPrepare");
	#endif /* SUPPORT_INSTRUMENTATION */

	if (paramsSize) {}	/* Quiet unused parameter messages */
	FskPortEffectCheckSourcesAreAccelerated(portEffect->src, &portEffect->effect);
	(void)FskPortEffectGetSrcCompatibleGLCacheBitmap(port, portEffect->src, &portEffect->srcRect, flags, &mid1);	/* Just the rect */
	err = FskGLEffectApply(&portEffect->effect, portEffect->src, &portEffect->srcRect, mid1, NULL);					/* This takes care of boundary conditions */
    FskBitmapDispose(portEffect->src);																				/* This merely decreases the useCount */

	FskEffectCopySet(&portEffect->effect);																			/* Set parameters for render phase */
	portEffect->src = mid1;																							/* The src is now just the rect */
	return err;
}


/********************************************************************************
 * EffectTmpCopyRender
 ********************************************************************************/

static FskErr EffectTmpCopyRender(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect;
	FskRectangleRecord cr;
	FskErr err;

	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectTmpCopyRender");
	#endif /* SUPPORT_INSTRUMENTATION */

	if (paramsSize) {}	/* Quiet unused parameter messages */
	portEffect = (struct FskPortPicEffectParametersRecord*)params;

	err = FskGLEffectApplyMode(&portEffect->effect, portEffect->src, NULL, port->bits, &portEffect->dstPoint, GetPortEffectClip(port, &cr), kFskGraphicsModeCopy, NULL);	/* Rect already incorporated into src at this phase */
	FskGLEffectCacheReleaseBitmap(portEffect->src);													/* Done with intermediate bitmap */
	return err;
}


/********************************************************************************
 * EffectInnerGlowPrepare
 ********************************************************************************/

static FskErr EffectInnerGlowPrepare(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskBitmap		mid1 = NULL, mid2 = NULL;
	FskErr			err;
	FskEffectRecord	subEffect;

	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectInnerGlowPrepare");
	#endif /* SUPPORT_INSTRUMENTATION */

	if (paramsSize) {}	/* Quiet unused parameter messages */
    (void)FskBitmapCheckGLSourceAccelerated(portEffect->src);
	(void)FskPortEffectGetSrcCompatibleGLCacheBitmap(port, portEffect->src, &portEffect->srcRect, kFskGLEffectCacheBitmapWithAlpha, &mid1);	/* Always alpha */
	FskEffectErodeSet(&subEffect, portEffect->effect.params.innerGlow.radius);																/* 2D Erode */
	(void)FskGLEffectApply(&subEffect, portEffect->src, &portEffect->srcRect, mid1, NULL);													/* This takes care of boundary conditions */
	(void)FskPortEffectGetSrcCompatibleGLCacheBitmap(port, portEffect->src, &portEffect->srcRect, kFskGLEffectCacheBitmapWithAlpha, &mid2);	/* This might resuse a cache buffer from the preceding Erode */
	mid2->alphaIsPremultiplied = portEffect->src->alphaIsPremultiplied;
	FskEffectGaussianBlurSet(&subEffect, portEffect->effect.params.innerGlow.blurSigma, portEffect->effect.params.innerGlow.blurSigma);		/* 2D Blur, incorporating boundary conditions */
	err = FskGLEffectApply(&subEffect, mid1, NULL, mid2, NULL);
	subEffect = portEffect->effect;																	/* Copy incoming parameters to set up render */
	FskEffectColorizeInnerSet(&portEffect->effect, mid2, &subEffect.params.innerGlow.color);		/* Set parameters for render phase */
	FskGLEffectCacheReleaseBitmap(mid1);															/* Done with mid1 temporary bitmap */
	return err;
}


/********************************************************************************
 * EffectInnerShadowPrepare
 ********************************************************************************/

static FskErr EffectInnerShadowPrepare(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskBitmap		mid1 = NULL;
	FskErr			err;
	FskEffectRecord	subEffect;

	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectInnerShadowPrepare");
	#endif /* SUPPORT_INSTRUMENTATION */

	if (paramsSize) {}	/* Quiet unused parameter messages */
    (void)FskBitmapCheckGLSourceAccelerated(portEffect->src);
	(void)FskPortEffectGetSrcCompatibleGLCacheBitmap(port, portEffect->src, &portEffect->srcRect, kFskGLEffectCacheBitmapWithAlpha|kFskGLEffectCacheBitmapInit, &mid1);	/* Always alpha */
	FskEffectGaussianBlurSet(&subEffect, portEffect->effect.params.innerShadow.blurSigma, portEffect->effect.params.innerShadow.blurSigma);	/* 2D Blur, incorporating boundary conditions */
	err = FskGLEffectApply(&subEffect, portEffect->src, &portEffect->srcRect, mid1, &portEffect->effect.params.innerShadow.offset);
	subEffect = portEffect->effect;																	/* Copy incoming parameters to set up render */
	FskEffectColorizeInnerSet(&portEffect->effect, mid1, &subEffect.params.innerShadow.color);		/* Set parameters for render phase */
	return err;
}


/********************************************************************************
 * EffectInnerRender
 ********************************************************************************/

static FskErr EffectInnerRender(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskRectangleRecord cr;
	FskErr err;
	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectInnerRender");
	#endif /* SUPPORT_INSTRUMENTATION */
	if (paramsSize) {}	/* Quiet unused parameter messages */
	err = FskGLEffectApplyMode(&portEffect->effect, portEffect->src, &portEffect->srcRect, port->bits, &portEffect->dstPoint, GetPortEffectClip(port, &cr), kFskGraphicsModeCopy, NULL);
	FskGLEffectCacheReleaseBitmap((FskBitmap)portEffect->effect.params.colorizeInner.matte);		/* Done with intermediate bitmap */
	FskBitmapDispose(portEffect->src);																/* This only decreases the useCount */
	return err;
}


/********************************************************************************
 * EffectOuterGlowPrepare
 ********************************************************************************/

static FskErr EffectOuterGlowPrepare(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskBitmap		mid1 = NULL, mid2 = NULL;
	FskErr			err;
	FskEffectRecord	subEffect;

	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectOuterGlowPrepare");
	#endif /* SUPPORT_INSTRUMENTATION */

	if (paramsSize) {}	/* Quiet unused parameter messages */
    (void)FskBitmapCheckGLSourceAccelerated(portEffect->src);
	(void)FskPortEffectGetSrcCompatibleGLCacheBitmap(port, portEffect->src, &portEffect->srcRect, kFskGLEffectCacheBitmapWithAlpha, &mid1);	/* Always alpha */
	FskEffectDilateSet(&subEffect, portEffect->effect.params.outerGlow.radius);																/* 2D Dilate */
	(void)FskGLEffectApply(&subEffect, portEffect->src, &portEffect->srcRect, mid1, NULL);													/* This takes care of boundary conditions */
	(void)FskPortEffectGetSrcCompatibleGLCacheBitmap(port, portEffect->src, &portEffect->srcRect, kFskGLEffectCacheBitmapWithAlpha, &mid2);	/* This might resuse a cache buffer from the preceding Dilate */
	FskEffectGaussianBlurSet(&subEffect, portEffect->effect.params.outerGlow.blurSigma, portEffect->effect.params.outerGlow.blurSigma);		/* 2D Blur, incorporating boundary conditions */
	err = FskGLEffectApply(&subEffect, mid1, NULL, mid2, NULL);
	subEffect = portEffect->effect;																	/* Copy incoming parameters to set up render */
	FskEffectColorizeOuterSet(&portEffect->effect, mid2, &subEffect.params.outerGlow.color);		/* Set parameters for render phase */
	FskGLEffectCacheReleaseBitmap(mid1);															/* Done with mid1 temporary bitmap */
	return err;
}


/********************************************************************************
 * EffectOuterShadowPrepare
 ********************************************************************************/

static FskErr EffectOuterShadowPrepare(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskBitmap		mid1 = NULL;
	FskErr			err;
	FskEffectRecord	subEffect;

	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectOuterShadowPrepare");
	#endif /* SUPPORT_INSTRUMENTATION */

	if (paramsSize) {}	/* Quiet unused parameter messages */
    (void)FskBitmapCheckGLSourceAccelerated(portEffect->src);
	(void)FskPortEffectGetSrcCompatibleGLCacheBitmap(port, portEffect->src, &portEffect->srcRect, kFskGLEffectCacheBitmapWithAlpha|kFskGLEffectCacheBitmapInit, &mid1);	/* Always alpha */
	FskEffectGaussianBlurSet(&subEffect, portEffect->effect.params.outerShadow.blurSigma, portEffect->effect.params.outerShadow.blurSigma);	/* 2D Blur, incorporating boundary conditions */
	err = FskGLEffectApply(&subEffect, portEffect->src, &portEffect->srcRect, mid1, &portEffect->effect.params.outerShadow.offset);
	subEffect = portEffect->effect;																	/* Copy incoming parameters to set up render */
	FskEffectColorizeOuterSet(&portEffect->effect, mid1, &subEffect.params.outerShadow.color);		/* Set parameters for render phase */
	return err;
}


/********************************************************************************
 * EffectOuterRender
 ********************************************************************************/

static FskErr EffectOuterRender(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskRectangleRecord cr;
	FskErr err;
	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectOuterRender");
	#endif /* SUPPORT_INSTRUMENTATION */
	if (paramsSize) {}	/* Quiet unused parameter messages */
	err = FskGLEffectApplyMode(&portEffect->effect, portEffect->src, &portEffect->srcRect, port->bits, &portEffect->dstPoint, GetPortEffectClip(port, &cr), kFskGraphicsModeCopy, NULL);
	FskGLEffectCacheReleaseBitmap((FskBitmap)portEffect->effect.params.colorizeOuter.matte);		/* Done with intermediate bitmap */
	FskBitmapDispose(portEffect->src);																/* This only decreases the useCount */
	return err;
}


/********************************************************************************
 * EffectDontPrepare
 ********************************************************************************/

static FskErr EffectDontPrepare(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectDontPrepare");
	#endif /* SUPPORT_INSTRUMENTATION */
	if (port || paramsSize) {}	/* Quiet unused parameter messages */
	FskPortEffectCheckSourcesAreAccelerated(portEffect->src, &portEffect->effect);
	return kFskErrNone;
}


/********************************************************************************
 * EffectAlphaRender
 ********************************************************************************/

static FskErr EffectAlphaRender(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskRectangleRecord cr;
	FskErr err;
	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectAlphaRender");
	#endif /* SUPPORT_INSTRUMENTATION */
	if (paramsSize) {}	/* Quiet unused parameter messages */
	err = FskGLEffectApplyMode(&portEffect->effect, portEffect->src, &portEffect->srcRect, port->bits, &portEffect->dstPoint, GetPortEffectClip(port, &cr), kFskGraphicsModeAlpha, NULL);
	FskBitmapDispose(portEffect->src);																/* This only decreases the useCount */
	return err;
}


/********************************************************************************
 * EffectInnerAlphaRender
 ********************************************************************************/

static FskErr EffectInnerAlphaRender(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskRectangleRecord cr;
	FskErr err;
	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectInnerAlphaRender");
	#endif /* SUPPORT_INSTRUMENTATION */
	if (paramsSize) {}	/* Quiet unused parameter messages */
	err = FskGLEffectApplyMode(&portEffect->effect, portEffect->src, &portEffect->srcRect, port->bits, &portEffect->dstPoint, GetPortEffectClip(port, &cr), kFskGraphicsModeAlpha, NULL);
	FskGLEffectCacheReleaseBitmap((FskBitmap)portEffect->effect.params.colorizeInner.matte);		/* Done with intermediate bitmap */
	FskBitmapDispose(portEffect->src);																/* This only decreases the useCount */
	return err;
}


/********************************************************************************
 * EffectOuterAlphaRender
 ********************************************************************************/

static FskErr EffectOuterAlphaRender(FskPort port, void *params, UInt32 paramsSize) {
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskRectangleRecord cr;
	FskErr err;
	#if SUPPORT_INSTRUMENTATION
		FskPortLogEffect(port, params, "EffectOuterAlphaRender");
	#endif /* SUPPORT_INSTRUMENTATION */
	if (paramsSize) {}	/* Quiet unused parameter messages */
	err = FskGLEffectApplyMode(&portEffect->effect, portEffect->src, &portEffect->srcRect, port->bits, &portEffect->dstPoint, GetPortEffectClip(port, &cr), kFskGraphicsModeAlpha, NULL);
	FskGLEffectCacheReleaseBitmap((FskBitmap)portEffect->effect.params.colorizeOuter.matte);		/* Done with intermediate bitmap */
	FskBitmapDispose(portEffect->src);																/* This only decreases the useCount */
	return err;
}


/********************************************************************************
 * Definition of specific Prepare/Render procs in terms of reusable ones.
 ********************************************************************************/

#define EffectColorizeAlphaPrepare		EffectDontPrepare
#define EffectColorizeAlphaRender		EffectAlphaRender
#define EffectDilatePrepare				EffectPostCopyPrepare
#define EffectDilateRender				EffectTmpCopyRender
#define EffectErodePrepare				EffectPostCopyPrepare
#define EffectErodeRender				EffectTmpCopyRender
#define EffectGaussianBlurPrepare		EffectPostCopyPrepare
#define EffectGaussianBlurRender		EffectTmpCopyRender
#define EffectInnerGlowAlphaPrepare		EffectInnerGlowPrepare
#define EffectInnerGlowAlphaRender		EffectInnerAlphaRender
#define EffectInnerGlowRender			EffectInnerRender
#define EffectInnerShadowAlphaPrepare	EffectInnerShadowPrepare
#define EffectInnerShadowAlphaRender	EffectInnerAlphaRender
#define EffectInnerShadowRender			EffectInnerRender
#define EffectMonochromeAlphaPrepare	EffectDontPrepare
#define EffectMonochromeAlphaRender		EffectAlphaRender
#define EffectOuterGlowAlphaPrepare		EffectOuterGlowPrepare
#define EffectOuterGlowAlphaRender		EffectOuterAlphaRender
#define EffectOuterGlowRender			EffectOuterRender
#define EffectOuterShadowAlphaPrepare	EffectOuterShadowPrepare
#define EffectOuterShadowAlphaRender	EffectOuterAlphaRender
#define EffectOuterShadowRender			EffectOuterRender


/********************************************************************************
 * Definition for effect prepare/render split dispatch table entry.
 ********************************************************************************/

struct EffectTabEntry { FskEffectID id; FskPortPicPrepareItem prepare; FskPortPicRenderItem render; };


/********************************************************************************
 * effectCopyTable
 ********************************************************************************/

static const struct EffectTabEntry effectCopyTable[] = {
	{	kFskEffectDilate,		EffectDilatePrepare,			EffectDilateRender			},
	{	kFskEffectErode,		EffectErodePrepare,				EffectErodeRender			},
	{	kFskEffectGaussianBlur,	EffectGaussianBlurPrepare,		EffectGaussianBlurRender	},
	{	kFskEffectInnerGlow,	EffectInnerGlowPrepare,			EffectInnerGlowRender		},
	{	kFskEffectInnerShadow,	EffectInnerShadowPrepare,		EffectInnerShadowRender		},
	{	kFskEffectOuterGlow,	EffectOuterGlowPrepare,			EffectOuterGlowRender		},
	{	kFskEffectOuterShadow,	EffectOuterShadowPrepare,		EffectOuterShadowRender		},
	{	(FskEffectID)0,			NULL,							NULL						}
};


/********************************************************************************
 * effectAlphaTable
 ********************************************************************************/

static const struct EffectTabEntry effectAlphaTable[] = {
	{	kFskEffectColorize,		EffectColorizeAlphaPrepare,		EffectColorizeAlphaRender	},
	{	kFskEffectMonochrome,	EffectMonochromeAlphaPrepare,	EffectMonochromeAlphaRender	},
	{	kFskEffectInnerGlow,	EffectInnerGlowAlphaPrepare,	EffectInnerGlowAlphaRender	},
	{	kFskEffectInnerShadow,	EffectInnerShadowAlphaPrepare,	EffectInnerShadowAlphaRender},
	{	kFskEffectOuterGlow,	EffectOuterGlowAlphaPrepare,	EffectOuterGlowAlphaRender	},
	{	kFskEffectOuterShadow,	EffectOuterShadowAlphaPrepare,	EffectOuterShadowAlphaRender},
	{	(FskEffectID)0,			NULL,							NULL						}
};


/********************************************************************************
 * GetEffectEntryFromID
 ********************************************************************************/

static const struct EffectTabEntry* GetEffectEntryFromID(FskEffectID id, const struct EffectTabEntry *tab) {
	for (; tab->render != NULL; ++tab)
		if (tab->id == id)
			return tab;
	return NULL;
}


#endif /* FSKBITMAP_OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL */



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****									API									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/****************************************************************************//**
 * Queue Prepare and Render procs for GL Effects in the port.
 *	\param[in,out]	port		the port.
 *	\param[in]		src			the source bitmap.
 *	\param[in]		srcRect		the source rectangle; this cannot be NULL.
 *	\param[in]		dstPoint	the upper left of the destination.
 *	\param[in]		effect		the effect to be applied.
 *	\return			kFskErrNone	if the operate was completed successfully.
 ********************************************************************************/

FskErr FskPortEffectQueue(FskPort port, FskBitmap src, FskConstRectangle srcRect, FskConstPoint dstPoint, FskConstEffect effect) {
	#if FSKBITMAP_OPENGL && GLES_VERSION == 2
		struct FskPortPicEffectParametersRecord portEffect;
		if (!FskBitmapIsOpenGLDestinationAccelerated(port->bits))
	#else /* !FSKBITMAP_OPENGL */
			return kFskErrNotAccelerated;
	#endif /* FSKBITMAP_OPENGL */

	#if FSKBITMAP_OPENGL && GLES_VERSION == 2 /* OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL */
		if (!port->graphicsModeParameters || (255 == port->graphicsModeParameters->blendLevel)) {	/* No blending */
			const struct EffectTabEntry *te;
			switch (port->graphicsMode & kFskGraphicsModeMask) {
				case kFskGraphicsModeAlpha:
					if (src->hasAlpha) {
						te = GetEffectEntryFromID(effect->effectID, effectAlphaTable);
						if (te != NULL) {															/* ... and we have a special prepare/render split, ... */
							portEffect.src		= src;
							portEffect.srcRect	= *srcRect;
							portEffect.dstPoint	= *dstPoint;
							portEffect.effect	= *effect;
							FskPortEffectScale(port, &portEffect);
							(void)FskPortPicSaveAdd(port, te->render, te->prepare, &portEffect, sizeof(portEffect));	/* ... use that */
							src->useCount += 1;
							return kFskErrNone;
						}
						break;
					} /* If no alpha, fall through to copy */
				case kFskGraphicsModeCopy:
					te = GetEffectEntryFromID(effect->effectID, effectCopyTable);
					if (te != NULL) {																/* ... and we have a special prepare/render split, ... */
						portEffect.src		= src;
						portEffect.srcRect	= *srcRect;
						portEffect.dstPoint	= *dstPoint;
						portEffect.effect	= *effect;
						FskPortEffectScale(port, &portEffect);
						(void)FskPortPicSaveAdd(port, te->render, te->prepare, &portEffect, sizeof(portEffect));	/* ... use that */
						src->useCount += 1;
						return kFskErrNone;
					}
					break;
				default:
					break;
			}
			/* Fall through if we do not have a special prepare/render split */
		}

		/* Accelerated destinations which are neither composited nor have a special prepare/render split fall through, because they only have one stage */
		FskPortEffectCheckSourcesAreAccelerated(src, effect);
		return kFskErrCodecNotFound;

	#endif  /* FSKBITMAP_OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL */
}


/****************************************************************************//**
 * Release all Open GL resources attached to the port.
 *	\param[in,out]	port	the port.
 ********************************************************************************/

void FskPortReleaseGLResources(FskPort port) {
	#if FSKBITMAP_OPENGL && GLES_VERSION == 2
		FskGLEffectCacheDisposeAllBitmaps();
		FskGLEffectsShutdown();
	#endif /* FSKBITMAP_OPENGL */
}


/********************************************************************************
 * Get a temporary bitmap for use in computing an effect.
 * This will allocate a GL texture if the port is accelerated, otherwise a standard bitmap.
 *	\param[in,out]	port		the port, which caches effects bitmaps.
 *	\param[in]		width		the desired width  of the bitmap.
 *	\param[in]		height		the desired height of the bitmap.
 *	\param[in]		pixelFormat	the desired format of the bitmap.
 *	\param[out]		bmp			a place to store the new bitmap.
 *	\return			kFskErrNone	if the operation was completed successfully.
 ********************************************************************************/

FskErr FskPortGetTempEffectBitmap(FskPort port, SInt32 width, SInt32 height, FskBitmapFormatEnum pixelFormat, FskBitmap *bmp) {
	#if FSKBITMAP_OPENGL && GLES_VERSION == 2
		if (port && FskBitmapIsOpenGLDestinationAccelerated(port->bits))
			return GLPortEffectGetCacheBitmap(port, width, height, kFskGLEffectCacheBitmapWithAlpha, bmp);
		else
	#endif /* FSKBITMAP_OPENGL && GLES_VERSION == 2 */
	return FskBitmapNew(width, height, pixelFormat, bmp);
}


#if SUPPORT_INSTRUMENTATION
static void LogDstBitmap(FskBitmap dstBM, const char *name) {
	FskRectangleRecord	portRect;
	if (!dstBM)
		return;
	if (!name)
		name = "DSTBM";
#if FSKBITMAP_OPENGL
	if (kFskErrNone == FskGLDstBMRectGet(dstBM, &portRect)) {
		unsigned	texName		= FskGLPortSourceTexture(dstBM->glPort);
		int			glIntFormat;
		if (texName)	FskGLPortTexFormatGet(dstBM->glPort, &glIntFormat);
		else			glIntFormat = 0;
		FskInstrumentedTypePrintfDebug(&gPortTypeInstrumentation, "\t%s: bounds(%ld %ld %ld %ld) depth=%lu format=%s rowBytes=%ld bits=%p alpha=%d premul=%d glDstPort(#%u w=%lu h=%lu %#x), useCount=%d",
			name, dstBM->bounds.x, dstBM->bounds.y, dstBM->bounds.width, dstBM->bounds.height, dstBM->depth,
			FskBitmapFormatName(dstBM->pixelFormat), dstBM->rowBytes, dstBM->bits, dstBM->hasAlpha, dstBM->alphaIsPremultiplied,
			texName, portRect.width, portRect.height, (glIntFormat), dstBM->useCount);
	} else
#endif /* FSKBITMAP_OPENGL */
	{
		FskInstrumentedTypePrintfDebug(&gPortTypeInstrumentation, "\t%s: bounds(%ld %ld %ld %ld) depth=%lu format=%s rowBytes=%ld bits=%p alpha=%d premul=%d, useCount=%d",
			name, dstBM->bounds.x, dstBM->bounds.y, dstBM->bounds.width, dstBM->bounds.height, dstBM->depth,
			FskBitmapFormatName(dstBM->pixelFormat), dstBM->rowBytes, dstBM->bits, dstBM->hasAlpha, dstBM->alphaIsPremultiplied, dstBM->useCount);
	}
}
#endif /* SUPPORT_INSTRUMENTATION */


/****************************************************************************//**
 * Release a temporary bitmap: GL bimaps are saved in a cache, whereas standard bitmaps are disposed.
 *	\param[in,out]	port	the port, which caches GL effects bitmaps.
 *	\param[in]		bm		the bitmap to be released.
 ********************************************************************************/

void FskPortReleaseTempEffectBitmap(FskPort port, FskBitmap bm) {
	#if SUPPORT_INSTRUMENTATION
		FskInstrumentedTypePrintfDebug(&gPortTypeInstrumentation, "FskPortReleaseTempEffectBitmap(%p)", bm);
		LogDstBitmap(bm, NULL);
	#endif /* SUPPORT_INSTRUMENTATION */
	#if FSKBITMAP_OPENGL && GLES_VERSION == 2
		if (port && bm && bm->pixelFormat == kFskBitmapFormatGLRGBA)
			FskGLEffectCacheReleaseBitmap(bm);
		else
	#endif /* FSKBITMAP_OPENGL && GLES_VERSION == 2 */
	FskBitmapDispose(bm);
}


#if FSKBITMAP_OPENGL

/********************************************************************************
 * Get a temporary GL bitmap compatible with the source bitmap.
 *	\param[in,out]	port		the port, which caches effects bitmaps.
 *	\param[in]		src			the source bitmap.
 *	\param[in]		srcRect		the source rectangle, whose width and height specifies the size of the new bitmap.
 *	\param[in]		flags		the logical OR of { 0, kFskGLEffectCacheBitmapWithAlpha, kFskGLEffectCacheBitmapInit }.
 *	\param[out]		bmp			a place to store the new bitmap.
 *	\return			kFskErrNone	if the operation was completed successfully.
 ********************************************************************************/

FskErr FskPortEffectGetSrcCompatibleGLCacheBitmap(FskPort port, FskConstBitmap src, FskConstRectangle srcRect, int flags, FskBitmap *bmp) {
	FskErr err = GLPortEffectGetCacheBitmap(port, srcRect->width, srcRect->height, flags, bmp);
	if (!err)
		(**bmp).alphaIsPremultiplied = src->alphaIsPremultiplied;
	return err;
}



/****************************************************************************//**
 * Check that all of the sources for an effect are accelerated.
 * \param[in,out]	src	the source bitmap.
 * \param[in]		e	the effect.
 ********************************************************************************/

 void FskPortEffectCheckSourcesAreAccelerated(FskBitmap src, FskConstEffect e) {
	unsigned i;
	if (src)							(void)FskBitmapCheckGLSourceAccelerated(src);
	switch (e->effectID) {
		case kFskEffectMask:			(void)FskBitmapCheckGLSourceAccelerated((FskBitmap)e->params.mask.mask);							break;
		case kFskEffectShade:			(void)FskBitmapCheckGLSourceAccelerated((FskBitmap)e->params.shade.shadow);							break;
		case kFskEffectColorizeInner:	(void)FskBitmapCheckGLSourceAccelerated((FskBitmap)e->params.colorizeInner.matte);					break;
		case kFskEffectColorizeOuter:	(void)FskBitmapCheckGLSourceAccelerated((FskBitmap)e->params.colorizeOuter.matte);					break;
		case kFskEffectCompound:		for (i = e->params.compound.numStages; i--;) FskPortEffectCheckSourcesAreAccelerated(NULL, ++e);	break;
		default:																															break;
	}
}

#endif /* FSKBITMAP_OPENGL */
