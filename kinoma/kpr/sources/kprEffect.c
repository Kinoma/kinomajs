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
#define __FSKPORT_PRIV__
#include "FskBlur.h"
#include "FskButtonShade.h"
#include "FskDilateErode.h"
#include "FskECMAScript.h"
#include "FskPixelOps.h"
#include "FskMemory.h"
#if FSKBITMAP_OPENGL
	#include "FskGLBlit.h"	/* needed for FskGLPortSourceTexture, when RARELY_COMPUTE_EFFECTS_IN_GL is defined */
#endif /* FSKBITMAP_OPENGL */

#include "kprSkin.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprEffect.h"
#include "kprHandler.h"
#include "kprLayer.h"
#include "kprShell.h"
#include "kprUtilities.h"

/* EFFECT */

static void KprEffectDispose(void* it);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprEffectInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprEffect", FskInstrumentationOffset(KprEffectRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprEffectNew(KprEffect *it, KprContext context)
{
	FskErr err = kFskErrNone;
	KprEffect self;
	bailIfError(KprAssetNew((KprAsset *)&self, sizeof(KprEffectRecord), context, &context->firstEffect, KprEffectDispose));
    *it = self;
	FskInstrumentedItemNew(self, NULL, &KprEffectInstrumentation);
#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemSetOwner(self, context);
#endif
bail:
	return err;
}

void KprEffectDispose(void* it)
{
	KprEffect self = it;
	FskMemPtrDispose(self->compound);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(it);
}

void KprEffectAdd(KprEffect self, FskEffect step)
{
	FskErr err = kFskErrNone;
	FskEffect compound;
	if (self->compound) {
		if (self->compound->effectID == kFskEffectCompound) {
			UInt32 count = 1 + self->compound->params.compound.numStages;
			bailIfError(FskMemPtrNewClear(((count + 1) * sizeof(FskEffectRecord)), &compound));
			FskMemCopy(compound, self->compound, count * sizeof(FskEffectRecord));
			FskMemCopy(compound + count, step, sizeof(FskEffectRecord));
			compound->params.compound.numStages = count;
		}
		else {
			bailIfError(FskMemPtrNewClear((3 * sizeof(FskEffectRecord)), &compound));
			compound->effectID = kFskEffectCompound;
			compound->params.compound.topology = kFskEffectCompoundTopologyPipeline;
			compound->params.compound.numStages = 2;
			FskMemCopy(compound + 1, self->compound, sizeof(FskEffectRecord));
			FskMemCopy(compound + 2, step, sizeof(FskEffectRecord));
		}
		FskMemPtrDispose(self->compound);
	}
	else {
		bailIfError(FskMemPtrNewClear(sizeof(FskEffectRecord), &compound));
		FskMemCopy(compound, step, sizeof(FskEffectRecord));
	}
	self->compound = compound;
bail:
	return; // @@
}

FskErr KprEffectApply(KprEffect self, FskBitmap srcBits, FskPort port, FskBitmap *result)
{
	FskErr err = kFskErrNone;
	FskRectangleRecord bounds;
	FskBitmap dstBits = NULL;
	FskBitmap tmp = NULL;

	Boolean flag;

	FskBitmapGetBounds(srcBits, &bounds);
	if (port) {
		bailIfError(FskPortGetTempEffectBitmap(port, bounds.width, bounds.height, kFskBitmapFormatDefaultAlpha, &dstBits));
	}
	else {
		bailIfError(FskBitmapNew(bounds.width, bounds.height, kFskBitmapFormatDefaultAlpha, &dstBits));
	}
	FskBitmapGetHasAlpha(srcBits, &flag);
	FskBitmapSetHasAlpha(dstBits, flag);
	FskBitmapGetAlphaIsPremultiplied(srcBits, &flag);
	FskBitmapSetAlphaIsPremultiplied(dstBits, flag);
#if FSKBITMAP_OPENGL
	if (FskBitmapIsOpenGLDestinationAccelerated(dstBits)) {	// If we are using GL to do the effects, ...
		FskBitmapCheckGLSourceAccelerated(srcBits); // ... make sure that the source is uploaded as a texture, ...
		err = FskGLEffectApply(self->compound, srcBits, &bounds, dstBits, NULL);
	}
	else {
#endif
		if (srcBits->pixelFormat != dstBits->pixelFormat) {
			bailIfError(FskBitmapNew(bounds.width, bounds.height, kFskBitmapFormatDefaultAlpha, &tmp));
			if (kFskErrNone == FskBitmapDraw(srcBits, NULL, dstBits, NULL, NULL, NULL, kFskGraphicsModeCopy, NULL))
				srcBits = tmp;
		}
		err = FskEffectApply(self->compound, srcBits, &bounds, dstBits, NULL);
		FskBitmapDispose(tmp);
#if FSKBITMAP_OPENGL
		FskBitmapSetOpenGLSourceAccelerated(dstBits, 1); // ... else make sure the result is accelerated.
	}
#endif
	*result = dstBits;
bail:
	return err;
}

void KprEffectExtend(KprEffect self, FskRectangle area)
{
	// ??
}

/* ECMASCRIPT */
#ifndef KPR_NO_GRAMMAR

void KPR_effect(void *it)
{
	if (it) {
		KprEffect self = it;
		kprVolatileDestructor(KPR_effect);
		KprAssetDispose(self);
	}
}

void KPR_Effect(xsMachine *the)
{
	KprEffect self;
	xsThrowIfFskErr(KprEffectNew(&self, xsGetContext(the)));
	kprVolatileConstructor(KPR_Effect);
}

#define APPLY_OPACITY_TO_ALPHA(opacity, alpha)	do { if ((opacity) < 1.f) (alpha) = (UInt8)((alpha) * (opacity) + .5f);  } while(0)

void KPR_effect_colorize(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprEffect self = kprGetHostData(xsThis, this, effect);
	FskColorRGBARecord color = { 128, 128, 128, 255 };
	float opacity = 1;
	FskEffectRecord step;
	if ((c > 0) && xsTest(xsArg(0)))
		KprParseColor(the, xsToString(xsArg(0)), &color);
	if (c > 1) {
		opacity = (float)xsToNumber(xsArg(1));
		APPLY_OPACITY_TO_ALPHA(opacity, color.a);
	}
	FskEffectColorizeSet(&step, &color);
	KprEffectAdd(self, &step);
}

void KPR_effect_gaussianBlur(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprEffect self = kprGetHostData(xsThis, this, effect);
	float x = 1;
	float y = 1;
	FskEffectRecord step;
	if (c > 0)
		x = y = (float)xsToNumber(xsArg(0));
	if (c > 1)
		y = (float)xsToNumber(xsArg(1));
	FskEffectGaussianBlurSet(&step, x, y);
	KprEffectAdd(self, &step);
}

void KPR_effect_gray(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprEffect self = kprGetHostData(xsThis, this, effect);
	FskColorRGBARecord dark = { 0, 0, 0, 255 };
	FskColorRGBARecord lite = { 255, 255, 255, 255 };
	FskEffectRecord step;
	if ((c > 0) && xsTest(xsArg(0)))
		KprParseColor(the, xsToString(xsArg(0)), &dark);
	if ((c > 1 && xsTest(xsArg(1))))
		KprParseColor(the, xsToString(xsArg(1)), &lite);
	FskEffectMonochromeSet(&step, &dark, &lite);
	KprEffectAdd(self, &step);
}

void KPR_effect_innerGlow(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprEffect self = kprGetHostData(xsThis, this, effect);
	float blur = 1;
	FskColorRGBARecord color = { 255, 255, 255, 255 };
	float opacity = 1;
	SInt32 radius = 1;
	FskEffectRecord step;
	if ((c > 0) && xsTest(xsArg(0)))
		KprParseColor(the, xsToString(xsArg(0)), &color);
	if (c > 1) {
		opacity = (float)xsToNumber(xsArg(1));
		APPLY_OPACITY_TO_ALPHA(opacity, color.a);
	}
	if (c > 2)
		blur = (float)xsToNumber(xsArg(2));
	if (c > 3)
		radius = xsToInteger(xsArg(3));
	FskEffectInnerGlowSet(&step, radius, blur, &color);
	KprEffectAdd(self, &step);
}

void KPR_effect_innerHilite(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprEffect self = kprGetHostData(xsThis, this, effect);
	float blur = 2;
	FskColorRGBARecord color = { 255, 255, 255, 255 };
	float opacity = 1;
	SInt32 x = 2, y = 2;
	FskEffectRecord step;
	if ((c > 0) && xsTest(xsArg(0)))
		KprParseColor(the, xsToString(xsArg(0)), &color);
	if (c > 1) {
		opacity = (float)xsToNumber(xsArg(1));
		APPLY_OPACITY_TO_ALPHA(opacity, color.a);
	}
	if (c > 2)
		blur = (float)xsToNumber(xsArg(2));
	if (c > 3)
		x = xsToInteger(xsArg(3));
	if (c > 4)
		y = xsToInteger(xsArg(4));
	FskEffectInnerShadowScalarSet(&step, x, y, blur, color.r, color.g, color.b, color.a);
	KprEffectAdd(self, &step);
}

void KPR_effect_innerShadow(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprEffect self = kprGetHostData(xsThis, this, effect);
	float blur = 2;
	FskColorRGBARecord color = { 0, 0, 0, 255 };
	float opacity = 1;
	SInt32 x = 2, y = 2;
	FskEffectRecord step;
	if ((c > 0) && xsTest(xsArg(0)))
		KprParseColor(the, xsToString(xsArg(0)), &color);
	if (c > 1) {
		opacity = (float)xsToNumber(xsArg(1));
		APPLY_OPACITY_TO_ALPHA(opacity, color.a);
	}
	if (c > 2)
		blur = (float)xsToNumber(xsArg(2));
	if (c > 3)
		x = xsToInteger(xsArg(3));
	if (c > 4)
		y = xsToInteger(xsArg(4));
	FskEffectInnerShadowScalarSet(&step, x, y, blur, color.r, color.g, color.b, color.a);
	KprEffectAdd(self, &step);
}

void KPR_effect_mask(xsMachine *the)
{
	KprEffect self = kprGetHostData(xsThis, this, effect);
	KprTexture texture = kprGetHostData(xsArg(0), texture, texture);
	Boolean owned = false;
	FskBitmap bitmap = KprTextureGetBitmap(texture, NULL, &owned);
	FskEffectRecord step;
	FskEffectMaskSet(&step, bitmap, NULL);
	KprEffectAdd(self, &step);
}

void KPR_effect_outerGlow(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprEffect self = kprGetHostData(xsThis, this, effect);
	float blur = 1;
	FskColorRGBARecord color = { 255, 255, 255, 255 };
	float opacity = 1;
	SInt32 radius = 1;
	FskEffectRecord step;
	if ((c > 0) && xsTest(xsArg(0)))
		KprParseColor(the, xsToString(xsArg(0)), &color);
	if (c > 1) {
		opacity = (float)xsToNumber(xsArg(1));
		APPLY_OPACITY_TO_ALPHA(opacity, color.a);
	}
	if (c > 2)
		blur = (float)xsToNumber(xsArg(2));
	if (c > 3)
		radius = xsToInteger(xsArg(3));
	FskEffectOuterGlowSet(&step, radius, blur, &color);
	KprEffectAdd(self, &step);
}

void KPR_effect_outerHilite(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprEffect self = kprGetHostData(xsThis, this, effect);
	float blur = 2;
	FskColorRGBARecord color = { 255, 255, 255, 255 };
	float opacity = 1;
	SInt32 x = 2, y = 2;
	FskEffectRecord step;
	if ((c > 0) && xsTest(xsArg(0)))
		KprParseColor(the, xsToString(xsArg(0)), &color);
	if (c > 1) {
		opacity = (float)xsToNumber(xsArg(1));
		APPLY_OPACITY_TO_ALPHA(opacity, color.a);
	}
	if (c > 2)
		blur = (float)xsToNumber(xsArg(2));
	if (c > 3)
		x = xsToInteger(xsArg(3));
	if (c > 4)
		y = xsToInteger(xsArg(4));
	FskEffectOuterShadowScalarSet(&step, x, y, blur, color.r, color.g, color.b, color.a);
	KprEffectAdd(self, &step);
}

void KPR_effect_outerShadow(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprEffect self = kprGetHostData(xsThis, this, effect);
	float blur = 2;
	FskColorRGBARecord color = { 0, 0, 0, 255 };
	float opacity = 1;
	SInt32 x = 2, y = 2;
	FskEffectRecord step;
	if ((c > 0) && xsTest(xsArg(0)))
		KprParseColor(the, xsToString(xsArg(0)), &color);
	if (c > 1) {
		opacity = (float)xsToNumber(xsArg(1));
		APPLY_OPACITY_TO_ALPHA(opacity, color.a);
	}
	if (c > 2)
		blur = (float)xsToNumber(xsArg(2));
	if (c > 3)
		x = xsToInteger(xsArg(3));
	if (c > 4)
		y = xsToInteger(xsArg(4));
	FskEffectOuterShadowScalarSet(&step, x, y, blur, color.r, color.g, color.b, color.a);
	KprEffectAdd(self, &step);
}

void KPR_effect_shade(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprEffect self = kprGetHostData(xsThis, this, effect);
	KprTexture texture = kprGetHostData(xsArg(0), texture, texture);
	Boolean owned = false;
	FskBitmap bitmap = KprTextureGetBitmap(texture, NULL, &owned);
	float opacity = 0.5;
	FskEffectRecord step;
	if (c > 1)
		opacity = (float)xsToNumber(xsArg(1));
	FskEffectShadeSet(&step, bitmap, NULL, (UInt8)(opacity * 255));
	KprEffectAdd(self, &step);
}

#endif /* KPR_NO_GRAMMAR */



