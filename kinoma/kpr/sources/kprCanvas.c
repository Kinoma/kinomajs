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
#define __FSKWINDOW_PRIV__
#include "FskCanvas.h"
#include "FskGlyphPath.h"
#include "FskPixelOps.h"
#include "FskTextConvert.h"

#include "kprBehavior.h"
#include "kprCanvas.h"
#include "kprContent.h"
#include "kprImage.h"
#include "kprSkin.h"
#include "kprShell.h"
#include "kprUtilities.h"

static void KprCanvasDispose(void* it);
static void KprCanvasDraw(void* it, FskPort port, FskRectangle area);
static FskBitmap KprCanvasGetBitmap(void* it, FskPort port, Boolean* owned);
static void KprCanvasPlaced(void* it);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprCanvasInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprCanvas", FskInstrumentationOffset(KprCanvasRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprCanvasDispatchRecord = {
	"canvas",
	KprContentActivated,
	KprContentAdded,
	KprContentCascade,
	KprCanvasDispose,
	KprCanvasDraw,
	KprContentFitHorizontally,
	KprContentFitVertically,
	KprCanvasGetBitmap,
	KprContentHit,
	KprContentIdle,
	KprContentInvalidated,
	KprContentLayered,
	KprContentMark,
	KprContentMeasureHorizontally,
	KprContentMeasureVertically,
	KprContentPlace,
	KprCanvasPlaced,
	KprContentPredict,
	KprContentReflowing,
	KprContentRemoving,
	KprContentSetWindow,
	KprContentShowing,
	KprContentShown,
	KprContentUpdate
};


static FskBitmapFormatEnum PreferredPixelFormat(void)
{
	#if FSKBITMAP_OPENGL && !defined(DISABLE_GL_CANVAS) && !TARGET_OS_ANDROID && !TARGET_OS_IPHONE
		if (gShell->window->usingGL)
			return kFskBitmapFormatGLRGBA;
	#endif
	return kFskBitmapFormatDefaultRGBA;
}

FskErr KprCanvasNew(KprCanvas* it,  KprCoordinates coordinates)
{
	FskErr err = kFskErrNone;
	KprCanvas self;

	bailIfError(FskMemPtrNewClear(sizeof(KprCanvasRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprCanvasInstrumentation);
	self->dispatch = &KprCanvasDispatchRecord;
	self->flags = kprVisible;
	KprContentInitialize((KprContent)self, coordinates, NULL, NULL);
	if (coordinates->width && coordinates->height) {
		bailIfError(FskCanvasNew(coordinates->width, coordinates->height, PreferredPixelFormat(), &self->cnv));
        FskCanvas2dSetOpenGLSourceAccelerated(self->cnv, true);
    }
bail:
	return err;
}

/* DISPATCH */

void KprCanvasDispose(void* it)
{
	KprCanvas self = it;
	if (self->cnv)
		FskCanvasDispose(self->cnv);
	KprContentDispose(it);
}

void KprCanvasDraw(void* it, FskPort port, FskRectangle area UNUSED)
{
	KprCanvas self = it;
	if (self->cnv) {
		FskConstBitmap bitmap = FskGetCanvasBitmap(self->cnv);
		FskRectangleRecord srcRect, dstRect;
		FskBitmapGetBounds(bitmap, &srcRect);
#if CHECK_UNACCELERATED_BITMAPS
		if (FskBitmapIsOpenGLDestinationAccelerated(port->bits) && !bitmap->accelerate)
			return;
#endif
		FskRectangleSet(&dstRect, 0, 0, self->bounds.width, self->bounds.height);
		FskPortBitmapDraw(port, (FskBitmap)bitmap, &srcRect, &dstRect);
	}
}

FskBitmap KprCanvasGetBitmap(void* it, FskPort port, Boolean* owned)
{
	KprCanvas self = it;
	FskBitmap result = NULL;
	if (self->cnv) {
		result = (FskBitmap)FskGetCanvasBitmap(self->cnv);
		*owned = true;
	}
	return result;
}

void KprCanvasPlaced(void* it)
{
	FskErr err = kFskErrNone;
	KprCanvas self = it;
	SInt32 width, height;
	FskRectangleRecord bounds;
	if (self->coordinates.width && self->coordinates.height)
		goto bail;
	width = self->bounds.width;
	height = self->bounds.height;
	if (self->cnv)
		FskBitmapGetBounds(FskGetCanvasBitmap(self->cnv), &bounds);
	else
		FskRectangleSetEmpty(&bounds);
	if ((width != bounds.width) || (height != bounds.height)) {
		FskCanvasDispose(self->cnv);
		self->cnv = NULL;
		bailIfError(FskCanvasNew(width, height, PreferredPixelFormat(), &self->cnv));
        FskCanvas2dSetOpenGLSourceAccelerated(self->cnv, true);
		self->flags |= kprDisplaying;
	}
bail:
	KprContentPlaced(it);
	return;
}

/* ECMASCRIPT */

void KPR_Canvas(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprCanvas self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	xsThrowIfFskErr(KprCanvasNew(&self, &coordinates));
	kprContentConstructor(KPR_Content);
}

void KPR_canvas_get_size(xsMachine *the)
{
	KprCanvas self = xsGetHostData(xsThis);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	if (self->cnv) {
		FskRectangleRecord bounds;
		FskBitmapGetBounds(FskGetCanvasBitmap(self->cnv), &bounds);
		xsNewHostProperty(xsResult, xsID_width, xsInteger(bounds.width), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_height, xsInteger(bounds.height), xsDefault, xsDontScript);
	}
	else if (self->shell) {
		KprShellAdjust(self->shell);
		xsNewHostProperty(xsResult, xsID_width, xsInteger(self->bounds.width), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_height, xsInteger(self->bounds.height), xsDefault, xsDontScript);
	}
}

void KPR_canvas_get_width(xsMachine *the)
{
	KprCanvas self = xsGetHostData(xsThis);
	if (self->cnv) {
		FskRectangleRecord bounds;
		FskBitmapGetBounds(FskGetCanvasBitmap(self->cnv), &bounds);
		xsResult = xsInteger(bounds.width);
	}
	else if (self->shell) {
		KprShellAdjust(self->shell);
		xsResult = xsInteger(self->bounds.width);
	}
}

void KPR_canvas_get_height(xsMachine *the)
{
	KprCanvas self = xsGetHostData(xsThis);
	if (self->cnv) {
		FskRectangleRecord bounds;
		FskBitmapGetBounds(FskGetCanvasBitmap(self->cnv), &bounds);
		xsResult = xsInteger(bounds.height);
	}
	else if (self->shell) {
		KprShellAdjust(self->shell);
		xsResult = xsInteger(self->bounds.height);
	}
}

void KPR_canvas_set_size(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprCanvas self = xsGetHostData(xsThis);
	SInt32 width = 0, height = 0;
	if (self->shell) {
		KprShellAdjust(self->shell);
		xsEnterSandbox();
		if (xsFindInteger(xsArg(0), xsID_width, &width))
			width -= self->bounds.width;
		if (xsFindInteger(xsArg(0), xsID_height, &height))
			height -= self->bounds.height;
		xsLeaveSandbox();
		KprContentSizeBy((KprContent)self, width, height);
	}
	else {
		FskRectangleRecord bounds;
		xsEnterSandbox();
		if (!xsFindInteger(xsArg(0), xsID_width, &width))
			width = self->coordinates.width;
		if (!xsFindInteger(xsArg(0), xsID_height, &height))
			height = self->coordinates.height;
		xsLeaveSandbox();
		if (self->cnv)
			FskBitmapGetBounds(FskGetCanvasBitmap(self->cnv), &bounds);
		else
			FskRectangleSetEmpty(&bounds);
		if ((width != bounds.width) || (height != bounds.height)) {
			FskCanvasDispose(self->cnv);
			self->cnv = NULL;
			if (width && height) {
				bailIfError(FskCanvasNew(width, height, PreferredPixelFormat(), &self->cnv));
                FskCanvas2dSetOpenGLSourceAccelerated(self->cnv, true);
            }
		}
	}
bail:
	return;
}

void KPR_canvas_set_width(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprCanvas self = xsGetHostData(xsThis);
	if (self->shell) {
		KprShellAdjust(self->shell);
		KprContentSizeBy((KprContent)self, xsToInteger(xsArg(0)) - self->bounds.width, 0);
	}
	else {
		SInt32 width = xsToInteger(xsArg(0));
		FskRectangleRecord bounds;
		if (self->cnv)
			FskBitmapGetBounds(FskGetCanvasBitmap(self->cnv), &bounds);
		else
			FskRectangleSetEmpty(&bounds);
		if ((width != bounds.width)) {
			FskCanvasDispose(self->cnv);
			self->cnv = NULL;
			if (width && bounds.height) {
				bailIfError(FskCanvasNew(width, bounds.height, PreferredPixelFormat(), &self->cnv));
                FskCanvas2dSetOpenGLSourceAccelerated(self->cnv, true);
            }
		}
	}
bail:
	return;
}

void KPR_canvas_set_height(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprCanvas self = xsGetHostData(xsThis);
	if (self->shell) {
		KprShellAdjust(self->shell);
		KprContentSizeBy((KprContent)self, 0, xsToInteger(xsArg(0)) - self->bounds.height);
	}
	else {
		SInt32 height = xsToInteger(xsArg(0));
		FskRectangleRecord bounds;
		if (self->cnv)
			FskBitmapGetBounds(FskGetCanvasBitmap(self->cnv), &bounds);
		else
			FskRectangleSetEmpty(&bounds);
		if ((height != bounds.height)) {
			FskCanvasDispose(self->cnv);
			self->cnv = NULL;
			if (bounds.width && height) {
				bailIfError(FskCanvasNew(bounds.width, bounds.height, PreferredPixelFormat(), &self->cnv));
                FskCanvas2dSetOpenGLSourceAccelerated(self->cnv, true);
            }
		}
	}
bail:
	return;
}

void KPR_canvas_getContext(xsMachine *the)
{
	KprCanvas self = xsGetHostData(xsThis);
	if (self->cnv) {
		FskCanvas2dContext ctx = FskCanvasGet2dContext(self->cnv);
		xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("canvasRenderingContext2D")));
		xsSetHostData(xsResult, ctx);
		xsSet(xsResult, xsID("canvas"), xsThis);
		KprContentInvalidate((KprContent)self);
	}
}

static xsStringValue kFskCanvas2dCompositeStrings[10] = {
	"source-over",
	"destination-over",
	"source-in",
	"destination-in",
	"source-out",
	"destination-out",
	"source-atop",
	"destination-atop",
	"lighter",
	"xor"
};

static xsStringValue kFskCanvas2dLineCapStrings[3] = {
	"round",
	"square",
	"butt"
};

static xsStringValue kFskCanvas2dLineJoinStrings[3] = {
	"round",
	"bevel",
	"miter"
};

static xsStringValue kFskCanvas2dTextAlignStrings[5] = {
	"start",
	"center",
	"end",
	"left",
	"right"
};

static xsStringValue kFskCanvas2dTextBaselineStrings[6] = {
	"alphabetic",
	"ideographic",
	"top",
	"hanging",
	"middle",
	"bottom"
};

static xsStringValue kFskCanvas2dFillRuleStrings[2] = {
	"nonzero",
	"evenodd"
};

static void KPR_canvasRenderingContext2D_get_enum(xsMachine *the, UInt32 (*getter)(FskConstCanvas2dContext), xsStringValue* array, UInt32 start, UInt32 stop)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	UInt32 value = (*getter)(ctx);
	if ((start <= value) && (value <= stop))
		xsResult = xsString(array[value - start]);
}

static void KPR_canvasRenderingContext2D_set_enum(xsMachine *the, void (*setter)(FskCanvas2dContext, UInt32), xsStringValue* array, UInt32 start, UInt32 stop)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsStringValue it = xsToString(xsArg(0));
	UInt32 value;
	for (value = start; value <= stop; value++) {
		if (!FskStrCompare(it, array[value - start])) {
			(*setter)(ctx, value);
			break;
		}
	}
}

void KPR_canvasRenderingContext2D_destructor(void *hostData UNUSED)
{
}

// state
void KPR_canvasRenderingContext2D_save(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dSave(ctx);
}

void KPR_canvasRenderingContext2D_restore(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dRestore(ctx);
}

// transformations
void KPR_canvasRenderingContext2D_scale(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	FskCanvas2dScale(ctx, x, y);
}

void KPR_canvasRenderingContext2D_rotate(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue angle = xsToNumber(xsArg(0));
	FskCanvas2dRotate(ctx, angle);
}

void KPR_canvasRenderingContext2D_translate(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	FskCanvas2dTranslate(ctx, x, y);
}

void KPR_canvasRenderingContext2D_transform(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue a = xsToNumber(xsArg(0));
	xsNumberValue b = xsToNumber(xsArg(1));
	xsNumberValue c = xsToNumber(xsArg(2));
	xsNumberValue d = xsToNumber(xsArg(3));
	xsNumberValue e = xsToNumber(xsArg(4));
	xsNumberValue f = xsToNumber(xsArg(5));
	FskCanvas2dTransform(ctx, a, b, c, d, e, f);
}

void KPR_canvasRenderingContext2D_setTransform(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue a = xsToNumber(xsArg(0));
	xsNumberValue b = xsToNumber(xsArg(1));
	xsNumberValue c = xsToNumber(xsArg(2));
	xsNumberValue d = xsToNumber(xsArg(3));
	xsNumberValue e = xsToNumber(xsArg(4));
	xsNumberValue f = xsToNumber(xsArg(5));
	FskCanvas2dSetTransform(ctx, a, b, c, d, e, f);
}

void KPR_canvasRenderingContext2D_setDeviceTransform(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue a = xsToNumber(xsArg(0));
	xsNumberValue b = xsToNumber(xsArg(1));
	xsNumberValue c = xsToNumber(xsArg(2));
	xsNumberValue d = xsToNumber(xsArg(3));
	xsNumberValue e = xsToNumber(xsArg(4));
	xsNumberValue f = xsToNumber(xsArg(5));
	FskCanvas2dSetDeviceTransform(ctx, a, b, c, d, e, f);
}

// compositing
void KPR_canvasRenderingContext2D_get_globalAlpha(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsResult = xsNumber(FskCanvas2dGetGlobalAlpha(ctx));
}

void KPR_canvasRenderingContext2D_set_globalAlpha(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue it = xsToNumber(xsArg(0));
	FskCanvas2dSetGlobalAlpha(ctx, it);
}

void KPR_canvasRenderingContext2D_get_globalCompositeOperation(xsMachine *the)
{
	KPR_canvasRenderingContext2D_get_enum(the, FskCanvas2dGetGlobalCompositeOperation, kFskCanvas2dCompositeStrings, kFskCanvas2dCompositePreSourceOver, kFskCanvas2dCompositePreXor);
}

void KPR_canvasRenderingContext2D_set_globalCompositeOperation(xsMachine *the)
{
	KPR_canvasRenderingContext2D_set_enum(the, FskCanvas2dSetGlobalCompositeOperation, kFskCanvas2dCompositeStrings, kFskCanvas2dCompositePreSourceOver, kFskCanvas2dCompositePreXor);
}

// colors and styles

static void KPR_canvasRenderingContext2D_getStyle(xsMachine *the, xsBooleanValue stroke)
{
	FskCanvas2dContext ctx;
	const FskColorSourceUnion* csu;
	UInt32 c, i;
	FskGradientStop *stop;
	xsVars(3);
	ctx = xsGetHostData(xsThis);
	if (stroke)
		csu = (const FskColorSourceUnion *)FskCanvas2dGetStrokeStyle(ctx);
	else
		csu = (const FskColorSourceUnion *)FskCanvas2dGetFillStyle(ctx);
	switch (csu->so.type) {
	case kFskColorSourceTypeConstant:
		KprSerializeColor(the, &csu->cn.color, &xsResult);
		break;
	case kFskColorSourceTypeLinearGradient:
		xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("canvasLinearGradient")));
		xsSet(xsResult, xsID("x0"), xsNumber(FskFixedToFloat(csu->lg.gradientVector[0].x)));
		xsSet(xsResult, xsID("y0"), xsNumber(FskFixedToFloat(csu->lg.gradientVector[0].y)));
		xsSet(xsResult, xsID("x1"), xsNumber(FskFixedToFloat(csu->lg.gradientVector[1].x)));
		xsSet(xsResult, xsID("y1"), xsNumber(FskFixedToFloat(csu->lg.gradientVector[1].y)));
		c = csu->lg.numStops;
		stop = csu->lg.gradientStops;
		goto getStops;
	case kFskColorSourceTypeRadialGradient:
		xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("canvasRadialGradient")));
		xsSet(xsResult, xsID("x0"), xsNumber(FskFixedToFloat(csu->rg.focus.x)));
		xsSet(xsResult, xsID("y0"), xsNumber(FskFixedToFloat(csu->rg.focus.y)));
		xsSet(xsResult, xsID("r0"), xsNumber(0));
		xsSet(xsResult, xsID("x1"), xsNumber(FskFixedToFloat(csu->rg.center.x)));
		xsSet(xsResult, xsID("y1"), xsNumber(FskFixedToFloat(csu->rg.center.y)));
		xsSet(xsResult, xsID("r1"), xsNumber(FskFixedToFloat(csu->rg.radius)));
		c = csu->rg.numStops;
		stop = csu->rg.gradientStops;
getStops:
		xsVar(0) = xsNew1(xsGlobal, xsID("Array"), xsInteger(c));
		for (i = 0; i < c; i++) {
			xsVar(1) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("canvasGradientStop")));
			xsSet(xsVar(1), xsID("offset"), xsNumber(FskFractToFloat(stop->offset)));
			KprSerializeColor(the, &stop->color, &xsVar(2));
			xsSet(xsVar(1), xsID("color"), xsVar(2));
			xsSetAt(xsVar(0), xsInteger(i), xsVar(1));
			stop++;
		}
		xsSet(xsResult, xsID("stops"), xsVar(0));
		break;
	case kFskColorSourceTypeTexture:
		xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("canvasPattern")));
		// @@ fskBitmapToXSBitmap(the, (FskBitmap)csu->tx.texture, false, &xsVar(0));
		xsSet(xsResult, xsID("image"), xsVar(0));
		c = csu->tx.spreadMethod & kFskCanvas2dPatternRepeatX;
		i = csu->tx.spreadMethod & kFskCanvas2dPatternRepeatY;
		if (c && i)
			xsSet(xsResult, xsID("repetition"), xsString("repeat"));
		else if (c)
			xsSet(xsResult, xsID("repetition"), xsString("repeat-x"));
		else if (i)
			xsSet(xsResult, xsID("repetition"), xsString("repeat-y"));
		else
			xsSet(xsResult, xsID("repetition"), xsString("no-repeat"));
		break;
	}
}

static void KPR_canvasRenderingContext2D_setStyle(xsMachine *the, xsBooleanValue stroke)
{
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		xsStringValue s = xsToString(xsArg(0));
		FskColorRGBARecord color;
		if (KprParseColor(the, s, &color)) {
			FskCanvas2dContext ctx = xsGetHostData(xsThis);
			if (stroke)
				FskCanvas2dSetStrokeStyleColor(ctx, &color);
			else
				FskCanvas2dSetFillStyleColor(ctx, &color);
		}
	}
	else
		xsArg(0) = xsCall2(xsArg(0), xsID("setStyle"), xsThis, xsBoolean(stroke));
}

void KPR_canvasLinearGradient_setStyle(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsArg(0));
	xsNumberValue x0 = xsToNumber(xsGet(xsThis, xsID("x0")));
	xsNumberValue y0 = xsToNumber(xsGet(xsThis, xsID("y0")));
	xsNumberValue x1 = xsToNumber(xsGet(xsThis, xsID("x1")));
	xsNumberValue y1 = xsToNumber(xsGet(xsThis, xsID("y1")));
	UInt32 c, i;
	FskCanvas2dGradientStop stops[kCanvas2DMaxGradientStops];
	xsVars(2);
	xsVar(0) = xsGet(xsThis, xsID("stops"));
	c = xsToInteger(xsGet(xsVar(0), xsID("length")));
	if (c > kCanvas2DMaxGradientStops) c = kCanvas2DMaxGradientStops;
	for (i = 0; i < c; i++) {
		xsVar(1) = xsGetAt(xsVar(0), xsInteger(i));
		stops[i].offset = xsToNumber(xsGet(xsVar(1), xsID("offset")));
		xsVar(1) = xsGet(xsVar(1), xsID("color"));
		if (!KprParseColor(the, xsToString(xsVar(1)), &(stops[i].color)))
			return;
	}
	if (xsTest(xsArg(1)))
		FskCanvas2dSetStrokeStyleLinearGradient(ctx, x0, y0, x1, y1, c, stops);
	else
		FskCanvas2dSetFillStyleLinearGradient(ctx, x0, y0, x1, y1, c, stops);
}

void KPR_canvasRadialGradient_setStyle(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsArg(0));
	xsNumberValue x0 = xsToNumber(xsGet(xsThis, xsID("x0")));
	xsNumberValue y0 = xsToNumber(xsGet(xsThis, xsID("y0")));
	xsNumberValue r0 = xsToNumber(xsGet(xsThis, xsID("r0")));
	xsNumberValue x1 = xsToNumber(xsGet(xsThis, xsID("x1")));
	xsNumberValue y1 = xsToNumber(xsGet(xsThis, xsID("y1")));
	xsNumberValue r1 = xsToNumber(xsGet(xsThis, xsID("r1")));
	UInt32 c, i;
	FskCanvas2dGradientStop stops[kCanvas2DMaxGradientStops];
	xsVars(2);
	xsVar(0) = xsGet(xsThis, xsID("stops"));
	c = xsToInteger(xsGet(xsVar(0), xsID("length")));
	if (c > kCanvas2DMaxGradientStops) c = kCanvas2DMaxGradientStops;
	for (i = 0; i < c; i++) {
		xsVar(1) = xsGetAt(xsVar(0), xsInteger(i));
		stops[i].offset = xsToNumber(xsGet(xsVar(1), xsID("offset")));
		xsVar(1) = xsGet(xsVar(1), xsID("color"));
		if (!KprParseColor(the, xsToString(xsVar(1)), &(stops[i].color)))
			return;
	}
	if (xsTest(xsArg(1)))
		FskCanvas2dSetStrokeStyleRadialGradient(ctx, x0, y0, r0, x1, y1, r1, c, stops);
	else
		FskCanvas2dSetFillStyleRadialGradient(ctx, x0, y0, r0, x1, y1, r1, c, stops);
}

void KPR_canvasPattern_setStyle(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsArg(0));
	UInt32 repetition = kFskCanvas2dPatternRepeat;
	FskConstBitmap bitmap = NULL;
	Boolean owned = false;
	xsVars(1);
	xsVar(0) = xsGet(xsThis, xsID("repetition"));
	if (xsTest(xsVar(0))) {
		xsStringValue it = xsToString(xsVar(0));
		if (!FskStrCompare(it, "no-repeat")) repetition = kFskCanvas2dPatternRepeatNone;
		else if (!FskStrCompare(it, "repeat-x")) repetition = kFskCanvas2dPatternRepeatX;
		else if (!FskStrCompare(it, "repeat-y")) repetition = kFskCanvas2dPatternRepeatY;
		else if (!FskStrCompare(it, "repeat")) repetition = kFskCanvas2dPatternRepeat;
		else xsError(kFskErrInvalidParameter);
	}
	xsVar(0) = xsGet(xsThis, xsID("image"));
	if (xsIsInstanceOf(xsVar(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID_texture))) {
		KprTexture texture = xsGetHostData(xsVar(0));
		bitmap = KprTextureGetBitmap(texture, NULL, &owned);
	}
	else {
		KprContent content = xsGetHostData(xsVar(0));
		bitmap = (*content->dispatch->getBitmap)(content, NULL, &owned);
	}
	if (!bitmap)
		xsError(kFskErrInvalidParameter);
	if (xsTest(xsArg(1)))
		FskCanvas2dSetStrokeStylePattern(ctx, repetition, bitmap);
	else
		FskCanvas2dSetFillStylePattern(ctx, repetition, bitmap);
	if (!owned)
		FskBitmapDispose((FskBitmap)bitmap);
}

void KPR_canvasRenderingContext2D_get_strokeStyle(xsMachine *the)
{
	KPR_canvasRenderingContext2D_getStyle(the, 1);
}

void KPR_canvasRenderingContext2D_set_strokeStyle(xsMachine *the)
{
	KPR_canvasRenderingContext2D_setStyle(the, 1);
}

void KPR_canvasRenderingContext2D_get_fillStyle(xsMachine *the)
{
	KPR_canvasRenderingContext2D_getStyle(the, 0);
}

void KPR_canvasRenderingContext2D_set_fillStyle(xsMachine *the)
{
	KPR_canvasRenderingContext2D_setStyle(the, 0);
}

// line caps/joins
void KPR_canvasRenderingContext2D_get_lineWidth(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsResult = xsNumber(FskCanvas2dGetLineWidth(ctx));
}

void KPR_canvasRenderingContext2D_set_lineWidth(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue it = xsToNumber(xsArg(0));
	FskCanvas2dSetLineWidth(ctx, it);
}

void KPR_canvasRenderingContext2D_get_lineCap(xsMachine *the)
{
	KPR_canvasRenderingContext2D_get_enum(the, FskCanvas2dGetLineCap, kFskCanvas2dLineCapStrings, kFskCanvas2dLineCapRound, kFskCanvas2dLineCapButt);
}

void KPR_canvasRenderingContext2D_set_lineCap(xsMachine *the)
{
	KPR_canvasRenderingContext2D_set_enum(the, FskCanvas2dSetLineCap, kFskCanvas2dLineCapStrings, kFskCanvas2dLineCapRound, kFskCanvas2dLineCapButt);
}

void KPR_canvasRenderingContext2D_get_lineJoin(xsMachine *the)
{
	KPR_canvasRenderingContext2D_get_enum(the, FskCanvas2dGetLineJoin, kFskCanvas2dLineJoinStrings, kFskCanvas2dLineJoinRound, kFskCanvas2dLineJoinMiter);
}

void KPR_canvasRenderingContext2D_set_lineJoin(xsMachine *the)
{
	KPR_canvasRenderingContext2D_set_enum(the, FskCanvas2dSetLineJoin, kFskCanvas2dLineJoinStrings, kFskCanvas2dLineJoinRound, kFskCanvas2dLineJoinMiter);
}

void KPR_canvasRenderingContext2D_get_miterLimit(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsResult = xsNumber(FskCanvas2dGetMiterLimit(ctx));
}

void KPR_canvasRenderingContext2D_set_miterLimit(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue it = xsToNumber(xsArg(0));
	FskCanvas2dSetMiterLimit(ctx, it);
}

// shadows
void KPR_canvasRenderingContext2D_get_shadowOffsetX(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsResult = xsNumber(FskCanvas2dGetShadowOffsetX(ctx));
}

void KPR_canvasRenderingContext2D_set_shadowOffsetX(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue it = xsToNumber(xsArg(0));
	FskCanvas2dSetShadowOffsetX(ctx, it);
}

void KPR_canvasRenderingContext2D_get_shadowOffsetY(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsResult = xsNumber(FskCanvas2dGetShadowOffsetY(ctx));
}

void KPR_canvasRenderingContext2D_set_shadowOffsetY(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue it = xsToNumber(xsArg(0));
	FskCanvas2dSetShadowOffsetY(ctx, it);
}

void KPR_canvasRenderingContext2D_get_shadowBlur(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsResult = xsNumber(FskCanvas2dGetShadowBlur(ctx));
}

void KPR_canvasRenderingContext2D_set_shadowBlur(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue it = xsToNumber(xsArg(0));
	FskCanvas2dSetShadowBlur(ctx, it);
}

void KPR_canvasRenderingContext2D_get_shadowColor(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	KprSerializeColor(the, FskCanvas2dGetShadowColor(ctx), &xsResult);
}

void KPR_canvasRenderingContext2D_set_shadowColor(xsMachine *the)
{
	xsStringValue s = xsToString(xsArg(0));
	FskColorRGBARecord color;
	if (KprParseColor(the, s, &color)) {
		FskCanvas2dContext ctx = xsGetHostData(xsThis);
		FskCanvas2dSetShadowColor(ctx, &color);
	}
}

// rects
void KPR_canvasRenderingContext2D_clearRect(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	xsNumberValue w = xsToNumber(xsArg(2));
	xsNumberValue h = xsToNumber(xsArg(3));
	FskCanvas2dClearRect(ctx, x, y, w, h);
}

void KPR_canvasRenderingContext2D_fillRect(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	xsNumberValue w = xsToNumber(xsArg(2));
	xsNumberValue h = xsToNumber(xsArg(3));
	FskCanvas2dFillRect(ctx, x, y, w, h);
}

void KPR_canvasRenderingContext2D_strokeRect(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	xsNumberValue w = xsToNumber(xsArg(2));
	xsNumberValue h = xsToNumber(xsArg(3));
	FskCanvas2dStrokeRect(ctx, x, y, w, h);
}

// path API
void KPR_canvasRenderingContext2D_beginPath(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dPathBegin(ctx, NULL);
}

void KPR_canvasRenderingContext2D_closePath(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dPathClose(ctx, NULL);
}

void KPR_canvasRenderingContext2D_moveTo(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	FskCanvas2dPathMoveTo(ctx, NULL, x, y);
}

void KPR_canvasRenderingContext2D_lineTo(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	FskCanvas2dPathLineTo(ctx, NULL, x, y);
}

void KPR_canvasRenderingContext2D_quadraticCurveTo(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue cpx = xsToNumber(xsArg(0));
	xsNumberValue cpy = xsToNumber(xsArg(1));
	xsNumberValue x = xsToNumber(xsArg(2));
	xsNumberValue y = xsToNumber(xsArg(3));
	FskCanvas2dPathQuadraticCurveTo(ctx, NULL, cpx, cpy, x, y);
}

void KPR_canvasRenderingContext2D_bezierCurveTo(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue cp1x = xsToNumber(xsArg(0));
	xsNumberValue cp1y = xsToNumber(xsArg(1));
	xsNumberValue cp2x = xsToNumber(xsArg(2));
	xsNumberValue cp2y = xsToNumber(xsArg(3));
	xsNumberValue x = xsToNumber(xsArg(4));
	xsNumberValue y = xsToNumber(xsArg(5));
	FskCanvas2dPathBezierCurveTo(ctx, NULL, cp1x, cp1y, cp2x, cp2y, x, y);
}

void KPR_canvasRenderingContext2D_arcTo(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x1 = xsToNumber(xsArg(0));
	xsNumberValue y1 = xsToNumber(xsArg(1));
	xsNumberValue x2 = xsToNumber(xsArg(2));
	xsNumberValue y2 = xsToNumber(xsArg(3));
	xsNumberValue radius = xsToNumber(xsArg(4));
	FskCanvas2dPathArcTo(ctx, NULL, x1, y1, x2, y2, radius);
}

void KPR_canvasRenderingContext2D_rect(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	xsNumberValue w = xsToNumber(xsArg(2));
	xsNumberValue h = xsToNumber(xsArg(3));
	FskCanvas2dPathRect(ctx, NULL, x, y, w, h);
}

void KPR_canvasRenderingContext2D_arc(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	xsNumberValue radius = xsToNumber(xsArg(2));
	xsNumberValue startAngle = xsToNumber(xsArg(3));
	xsNumberValue endAngle = xsToNumber(xsArg(4));
	xsBooleanValue anticlockwise = (xsToInteger(xsArgc) > 5) ? xsTest(xsArg(5)) : 0;
	FskCanvas2dPathArc(ctx, NULL, x, y, radius, startAngle, endAngle, anticlockwise);
}

static SInt32 GetFillRule(xsStringValue fillStr) {
	SInt32 fillRule = kFskCanvas2dFillRuleNonZero;	/* Default */
	if (fillStr) {
		if      (!FskStrCompare(fillStr, kFskCanvas2dFillRuleStrings[0]))	fillRule = kFskCanvas2dFillRuleNonZero;
		else if (!FskStrCompare(fillStr, kFskCanvas2dFillRuleStrings[1]))	fillRule = kFskCanvas2dFillRuleEvenOdd;
	}
	return fillRule;
}

void KPR_canvasRenderingContext2D_fill(xsMachine *the)
{
	FskCanvas2dContext	ctx			= xsGetHostData(xsThis);
	FskCanvas2dPath		path		= NULL;
	SInt32				fillRule	= kFskCanvas2dFillRuleNonZero;
	int					numArgs		= xsToInteger(xsArgc);

	if (numArgs > 0) {																		/* ctx.fill(), when numArgs==0 */
		if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID("path2D")))) {	/* ctx.fill(path) */
			path = xsGetHostData(xsArg(0));
			if (numArgs > 1)	fillRule = GetFillRule(xsToString(xsArg(1)));				/* ctx.fill(path, fillRule) */
		} else {				fillRule = GetFillRule(xsToString(xsArg(0)));				/* ctx.fill(fillRule) */
		}
	}
	FskCanvas2dPathFill(ctx, path, fillRule);
}

void KPR_canvasRenderingContext2D_stroke(xsMachine *the)
{
	FskCanvas2dContext	ctx		= xsGetHostData(xsThis);
	FskCanvas2dPath		path	= (xsToInteger(xsArgc) > 0) ? xsGetHostData(xsArg(0)) : NULL;
	FskCanvas2dPathStroke(ctx, path);
}

void KPR_canvasRenderingContext2D_clip(xsMachine *the)
{
	FskCanvas2dContext	ctx			= xsGetHostData(xsThis);
	FskCanvas2dPath		path		=  NULL;
	SInt32				fillRule	= kFskCanvas2dFillRuleNonZero;
	int					numArgs		= xsToInteger(xsArgc);

	if (numArgs > 0) {																		/* ctx.clip(), when numArgs==0 */
		if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID("path2D")))) {	/* ctx.clip(path) */
			path = xsGetHostData(xsArg(0));
			if (numArgs > 1)	fillRule = GetFillRule(xsToString(xsArg(1)));				/* ctx.clip(path, fillRule) */
		} else {				fillRule = GetFillRule(xsToString(xsArg(0)));				/* ctx.clip(fillRule) */
		}
	}
	FskCanvas2dPathClip(ctx, path, fillRule);
}

void KPR_canvasRenderingContext2D_isPointInPath(xsMachine *the)
{
	FskCanvas2dContext	ctx			= xsGetHostData(xsThis);
	FskCanvas2dPath		path		= NULL;
	SInt32				fillRule	= kFskCanvas2dFillRuleNonZero;
	int					numArgs		= xsToInteger(xsArgc);
	xsNumberValue		x, y;

	if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID("path2D")))) {	/* ctx.isPointInPath(path, x, y) */
		path = xsGetHostData(xsArg(0));
		x = xsToNumber(xsArg(1));
		y = xsToNumber(xsArg(2));
		if (numArgs > 3)	fillRule = GetFillRule(xsToString(xsArg(3)));				/* ctx.isPointInPath(path, x, y, fillRule) */
	} else {																			/* ctx.isPointInPath(x, y) */
		x = xsToNumber(xsArg(0));
		y = xsToNumber(xsArg(1));
		if (numArgs > 2)	fillRule = GetFillRule(xsToString(xsArg(2)));				/* ctx.isPointInPath(x, y, fillRule) */
	}
	xsResult = xsBoolean(FskCanvas2dIsPointInPathFill(ctx, path, x, y, fillRule));
}


void KPR_canvasRenderingContext2D_isPointInPathStroke(xsMachine *the)
{
	FskCanvas2dContext	ctx			= xsGetHostData(xsThis);
	FskCanvas2dPath		path		= NULL;
	xsNumberValue		x, y;

	if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID("path2D")))) {	/* ctx.isPointInPathStroke(path, x, y) */
		path = xsGetHostData(xsArg(0));
		x = xsToNumber(xsArg(1));
		y = xsToNumber(xsArg(2));
	} else {																			/* ctx.isPointInPathStroke(x, y) */
		x = xsToNumber(xsArg(0));
		y = xsToNumber(xsArg(1));
	}
	xsResult = xsBoolean(FskCanvas2dIsPointInPathStroke(ctx, path, x, y));
}


// focus management
void KPR_canvasRenderingContext2D_drawFocusRing(xsMachine *the)
{
	xsDebugger();
}

// text
void KPR_canvasRenderingContext2D_get_font(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	const struct FskFontAttributes* font = FskCanvas2dGetFont(ctx);
	char buffer[1024] = "";
	if (font->style == kFskFontStyleItalic)
		FskStrCat(buffer, "italic ");
	else if (font->style == kFskFontStyleOblique)
		FskStrCat(buffer, "oblique ");
	if (font->weight == kFskFontWeightBold)
		FskStrCat(buffer, "bold ");
	else if (font->weight != kFskFontWeightNormal) {
		FskStrNumToStr(font->weight, buffer + FskStrLen(buffer), sizeof(buffer) - FskStrLen(buffer));
		FskStrCat(buffer, " ");
	}
	if (font->variant == kFskFontVariantSmallCaps)
		FskStrCat(buffer, "small-caps ");
	FskStrNumToStr((SInt32)(font->size), buffer + FskStrLen(buffer), sizeof(buffer) - FskStrLen(buffer));
	FskStrCat(buffer, "px ");
	FskStrCat(buffer, font->family);
	xsResult = xsString(buffer);
}

#define kFskMediumFontSize 12
void KPR_canvasRenderingContext2D_set_font(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	const struct FskFontAttributes* parent = FskCanvas2dGetFont(ctx);
	FskFontAttributes font;
	xsStringValue it = xsToString(xsArg(0));
	xsStringValue p = it;
	xsStringValue q;
	char c, d;
	float size;
	char state = 0;
	font.family = NULL;
	font.size = 0;
	font.weight = kFskFontWeightNormal;
	font.style = kFskFontStyleNormal;
	font.anchor = kFskTextAnchorStart;
	font.stretch = kFskFontStretchNormal;
	font.decoration = kFskFontDecorationNone;
	font.variant = kFskFontVariantNormal;
	font.sizeAdjust	= 0;
	for (;;) {
		while ((c = *p)) {
			if (c != ' ')
				break;
			p++;
		}
		if (!c)
			break;
		if (state == 2) {
			font.family = p;
			break;
		}
		q = p;
		while ((c = *q)) {
			if (c == ' ')
				break;
			q++;
		}
		if (state == 0) {
			if (!FskStrCompareWithLength(p, "normal", q - p))
				{}
			else if (!FskStrCompareWithLength(p, "italic", q - p))
				font.style = kFskFontStyleItalic;
			else if (!FskStrCompareWithLength(p, "oblique", q - p))
				font.style = kFskFontStyleOblique;
			else if (!FskStrCompareWithLength(p, "inherit", q - p))
				{}
			else if (!FskStrCompareWithLength(p, "bold", q - p))
				font.weight = kFskFontWeightBold;
			else if (!FskStrCompareWithLength(p, "100", q - p))
				font.weight = 100;
			else if (!FskStrCompareWithLength(p, "200", q - p))
				font.weight = 200;
			else if (!FskStrCompareWithLength(p, "300", q - p))
				font.weight = 300;
			else if (!FskStrCompareWithLength(p, "400", q - p))
				font.weight = 400;
			else if (!FskStrCompareWithLength(p, "500", q - p))
				font.weight = 500;
			else if (!FskStrCompareWithLength(p, "600", q - p))
				font.weight = 600;
			else if (!FskStrCompareWithLength(p, "700", q - p))
				font.weight = 700;
			else if (!FskStrCompareWithLength(p, "800", q - p))
				font.weight = 800;
			else if (!FskStrCompareWithLength(p, "900", q - p))
				font.weight = 900;
			else if (!FskStrCompareWithLength(p, "small-caps", q - p))
				font.variant = kFskFontVariantSmallCaps;
			else
				state = 1;
		}
		if (state == 1) {
			if (!FskStrCompareWithLength(p, "xx-small", q - p))
				font.size = 3 * kFskMediumFontSize / 5;
			else if (!FskStrCompareWithLength(p, "x-small", q - p))
				font.size = 3 * kFskMediumFontSize / 4;
			else if (!FskStrCompareWithLength(p, "small", q - p))
				font.size = 8 * kFskMediumFontSize / 9;
			else if (!FskStrCompareWithLength(p, "medium", q - p))
				font.size = kFskMediumFontSize;
			else if (!FskStrCompareWithLength(p, "large", q - p))
				font.size = 6 * kFskMediumFontSize / 5;
			else if (!FskStrCompareWithLength(p, "x-large", q - p))
				font.size = 3 * kFskMediumFontSize / 2;
			else if (!FskStrCompareWithLength(p, "xx-large", q - p))
				font.size = 2 * kFskMediumFontSize;
			else if (!FskStrCompareWithLength(p, "larger", q - p))
				font.size = 6 * kFskMediumFontSize / 5; // @@
			else if (!FskStrCompareWithLength(p, "smaller", q - p))
				font.size = 8 * kFskMediumFontSize / 9; // @@
			else {
				size = 0;
				while ((p < q) && ((d = *p)) && ('0' <= d) && (d <= '9')) {
					size = (10 * size) + (d - '0');
					p++;
				}
				if (!FskStrCompareWithLength(p, "%", q - p))
					font.size = parent->size * size / 100;
				else if (!FskStrCompareWithLength(p, "px", q - p))
					font.size = size;
			}
			state = 2;
		}
		if (!c)
			break;
		p = q;
	}
	if (!font.family || !font.size)
		return;
	FskCanvas2dSetFont(ctx, &font);
}

void KPR_canvasRenderingContext2D_get_textAlign(xsMachine *the)
{
	KPR_canvasRenderingContext2D_get_enum(the, FskCanvas2dGetTextAlignment, kFskCanvas2dTextAlignStrings, kFskCanvas2dTextAlignStart, kFskCanvas2dTextAlignRight);
}

void KPR_canvasRenderingContext2D_set_textAlign(xsMachine *the)
{
	KPR_canvasRenderingContext2D_set_enum(the, FskCanvas2dSetTextAlignment, kFskCanvas2dTextAlignStrings, kFskCanvas2dTextAlignStart, kFskCanvas2dTextAlignRight);
}

void KPR_canvasRenderingContext2D_get_textBaseline(xsMachine *the)
{
	KPR_canvasRenderingContext2D_get_enum(the, FskCanvas2dGetTextBaseline, kFskCanvas2dTextBaselineStrings, kFskCanvas2dTextBaselineAlphabetic, kFskCanvas2dTextBaselineBottom);
}

void KPR_canvasRenderingContext2D_set_textBaseline(xsMachine *the)
{
	KPR_canvasRenderingContext2D_set_enum(the, FskCanvas2dSetTextBaseline, kFskCanvas2dTextBaselineStrings, kFskCanvas2dTextBaselineAlphabetic, kFskCanvas2dTextBaselineBottom);
}

void KPR_canvasRenderingContext2D_fillText(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsStringValue text = xsToString(xsArg(0));
	xsNumberValue x = xsToNumber(xsArg(1));
	xsNumberValue y = xsToNumber(xsArg(2));
	xsNumberValue maxWidth = (xsToInteger(xsArgc) > 3) ? xsToNumber(xsArg(3)) : -1;
	UInt16* buffer;
	if (kFskErrNone == FskTextUTF8ToUnicode16NE((unsigned char*)text, FskStrLen(text), &buffer, NULL)) {
		FskCanvas2dFillText(ctx, buffer, x, y, maxWidth);
		FskMemPtrDispose(buffer);
	}
}

void KPR_canvasRenderingContext2D_strokeText(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsStringValue text = xsToString(xsArg(0));
	xsNumberValue x = xsToNumber(xsArg(1));
	xsNumberValue y = xsToNumber(xsArg(2));
	xsNumberValue maxWidth = (xsToInteger(xsArgc) > 3) ? xsToNumber(xsArg(3)) : -1;
	UInt16* buffer;
	if (kFskErrNone == FskTextUTF8ToUnicode16NE((unsigned char*)text, FskStrLen(text), &buffer, NULL)) {
		FskCanvas2dStrokeText(ctx, buffer, x, y, maxWidth);
		FskMemPtrDispose(buffer);
	}
}

void KPR_canvasRenderingContext2D_measureText(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsStringValue text = xsToString(xsArg(0));
	UInt16* buffer;
	if (kFskErrNone == FskTextUTF8ToUnicode16NE((unsigned char*)text, FskStrLen(text), &buffer, NULL)) {
		xsNumberValue width = FskCanvas2dMeasureText(ctx, buffer);
		FskMemPtrDispose(buffer);
		xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("textMetrics")));
		xsSet(xsResult, xsID("width"), xsNumber(width));
	}
}

// drawing images
void KPR_canvasRenderingContext2D_drawImage(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	FskConstCanvas cnv = NULL;
	FskBitmap bitmap = NULL;
	Boolean owned = false;
	xsVars(1);
	if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID_texture))) {
		KprTexture texture = xsGetHostData(xsArg(0));
		bitmap = KprTextureGetBitmap(texture, NULL, &owned);
	}
	else {
		KprContent content = xsGetHostData(xsArg(0));
		if (content->dispatch == &KprCanvasDispatchRecord)
			cnv = ((KprCanvas)content)->cnv;
		else
			bitmap = (*content->dispatch->getBitmap)(content, NULL, &owned);
	}
	if (!cnv && !bitmap)
		xsError(kFskErrInvalidParameter);
	if (c > 8) {
		xsNumberValue sx = xsToNumber(xsArg(1));
		xsNumberValue sy = xsToNumber(xsArg(2));
		xsNumberValue sw = xsToNumber(xsArg(3));
		xsNumberValue sh = xsToNumber(xsArg(4));
		xsNumberValue dx = xsToNumber(xsArg(5));
		xsNumberValue dy = xsToNumber(xsArg(6));
		xsNumberValue dw = xsToNumber(xsArg(7));
		xsNumberValue dh = xsToNumber(xsArg(8));
		if (cnv)
			FskCanvas2dDrawSubScaledCanvas2d(ctx, cnv, sx, sy, sw, sh, dx, dy, dw, dh);
		else
			FskCanvas2dDrawSubScaledBitmap(ctx, bitmap, sx, sy, sw, sh, dx, dy, dw, dh);
	}
	else if (c > 4) {
		xsNumberValue dx = xsToNumber(xsArg(1));
		xsNumberValue dy = xsToNumber(xsArg(2));
		xsNumberValue dw = xsToNumber(xsArg(3));
		xsNumberValue dh = xsToNumber(xsArg(4));
		if (cnv)
			FskCanvas2dDrawScaledCanvas2d(ctx, cnv, dx, dy, dw, dh);
		else
			FskCanvas2dDrawScaledBitmap(ctx, bitmap, dx, dy, dw, dh);
	}
	else {
		xsNumberValue dx = xsToNumber(xsArg(1));
		xsNumberValue dy = xsToNumber(xsArg(2));
		if (cnv)
			FskCanvas2dDrawCanvas2d(ctx, cnv, dx, dy);
		else
			FskCanvas2dDrawBitmap(ctx, bitmap, dx, dy);
	}
	if (!owned)
		FskBitmapDispose(bitmap);
}

// pixel manipulation
void KPR_canvasRenderingContext2D_createImageData(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue sw = xsToNumber(xsArg(0));
	xsNumberValue sh = xsToNumber(xsArg(1));
	FskCanvas2dImageData data = FskCanvas2dCreateImageData(ctx, sw, sh);
	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("imageData")));
	xsSetHostData(xsResult, data);
}

void KPR_canvasRenderingContext2D_getImageData(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue sx = xsToNumber(xsArg(0));
	xsNumberValue sy = xsToNumber(xsArg(1));
	xsNumberValue sw = xsToNumber(xsArg(2));
	xsNumberValue sh = xsToNumber(xsArg(3));
	FskCanvas2dImageData data = FskCanvas2dGetImageData(ctx, sx, sy, sw, sh);
	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("imageData")));
	xsSetHostData(xsResult, data);
}

void KPR_canvasRenderingContext2D_putImageData(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dImageData data = xsGetHostData(xsArg(0));
	xsNumberValue dx = xsToNumber(xsArg(1));
	xsNumberValue dy = xsToNumber(xsArg(2));
	xsNumberValue dirtyX, dirtyY, dirtyWidth, dirtyHeight;
	if (xsToInteger(xsArgc) > 6) {
		dirtyX = xsToNumber(xsArg(3));
		dirtyY = xsToNumber(xsArg(4));
		dirtyWidth = xsToNumber(xsArg(5));
		dirtyHeight = xsToNumber(xsArg(6));
	}
	else {
		dirtyX = 0;
		dirtyY = 0;
		dirtyWidth = data->width;
		dirtyHeight = data->height;
	}
	FskCanvas2dPutImageData(ctx, data, dx, dy, dirtyX, dirtyY, dirtyWidth, dirtyHeight);
}

// image data
void KPR_canvasRenderingContext2D_cloneImageData(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dImageData data = xsGetHostData(xsArg(0));
	data = FskCanvas2dCloneImageData(ctx, data);
	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("imageData")));
	xsSetHostData(xsResult, data);
}

// clip
void KPR_canvasRenderingContext2D_resetClip(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dClipReset(ctx);
}

// hit regions
void KPR_canvasRenderingContext2D_addHitRegion(xsMachine *the)
{
	FskCanvas2dContext		ctx			= xsGetHostData(xsThis);
	const char				*id			= NULL,
							*control	= NULL;
	FskConstCanvas2dPath	path		= NULL;
	xsVars(1);

	xsEnterSandbox();	/* Enter sandbox to access the properties of an object */
		xsFindString(xsArg(0), xsID("id"),      &id);
		xsFindString(xsArg(0), xsID("control"), &control);
		if (xsHasOwn(xsArg(0), xsID("path")))	path = xsGetHostData(xsGet(xsArg(0), xsID("path")));
		//xsFindString(xsArg(0), xsID("parentID"), &parentID);
		//xsFindString(xsArg(0), xsID("cursor"),   &cursor);
		//xsFindString(xsArg(0), xsID("fillRule"), &fillRuleStr);
		//xsFindString(xsArg(0), xsID("label"),    &label);
		//xsFindString(xsArg(0), xsID("role"),     &label);
	xsLeaveSandbox();
	(void)FskCanvas2dAddHitRegion(ctx, path, id, control);
}

void KPR_canvasRenderingContext2D_removeHitRegion(xsMachine *the)
{
	FskCanvas2dContext	ctx	= xsGetHostData(xsThis);
	xsStringValue		id	= xsToString(xsArg(0));
	FskCanvas2dRemoveHitRegion(ctx, id, NULL);
}

void KPR_canvasRenderingContext2D_clearHitRegions(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dClearHitRegions(ctx);
}

#define ENABLE_HIT_TESTING 0
#if ENABLE_HIT_TESTING
void KPR_canvasRenderingContext2D_pickHitRegion(xsMachine *the)
{
	FskCanvas2dContext	ctx			= xsGetHostData(xsThis);
	xsNumberValue		x			= xsToNumber(xsArg(0)),
						y			= xsToNumber(xsArg(1));
	const char			*id			= NULL,
						*control	= NULL;

	if (kFskErrNone == FskCanvas2dPickHitRegion(ctx, x, y, &id, &control)) {
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		if (id)			xsNewHostProperty(xsResult, xsID("id"),      xsString((char*)id),      xsDefault, xsDontScript);	// TODO: Would we send a mouse event to the id?
		if (control)	xsNewHostProperty(xsResult, xsID("control"), xsString((char*)control), xsDefault, xsDontScript);	// TODO: We should call the control directly.
	}
	else {
		// How do we indicate failure? xsResult = NULL;
	}
}
#endif /* ENABLE_HIT_TESTING */


// dashes
void KPR_canvasRenderingContext2D_setLineDash(xsMachine *the)
{
	FskCanvas2dContext	ctx		= xsGetHostData(xsThis);
	UInt32				len		= xsToInteger(xsGet(xsArg(0), xsID("length")));	/* Array of even length */
	double				dash[kCanvas2DMaxDashCycles*2];
	UInt32				i;

	if (len) {
		if (len > sizeof(dash) / sizeof(dash[0])) {								/* If too large, truncate */
			kprTraceDiagnostic(the, "dash length %u > %u is too long", (unsigned)len, sizeof(dash) / sizeof(dash[0]));
			len = sizeof(dash) / sizeof(dash[0]);
		}
		i = (len & 1) ? (len * 2) : len;										/* If odd, double the length to make it even */
		for (i = 0; i < len; ++i)
			dash[i] = xsToNumber(xsGetAt(xsArg(0), xsInteger(i)));				/* Copy from xs Array to C array to interface to FskCanvas2dSetLineDash() */
		if (len & 1) {															/* If odd, ... */
			for (i = 0; i < len; ++i)
				dash[i + len] = dash[i];										/* ... replicate. Now it is even. */
		}
		else {
			len /= 2;															/* Len here is the number of cycles */
		}
	}
	(void)FskCanvas2dSetLineDash(ctx, len, dash);								/* Set the context line dash state */
	return;
}

void KPR_canvasRenderingContext2D_getLineDash(xsMachine *the)
{
	FskCanvas2dContext	ctx = xsGetHostData(xsThis);
	double				*dash	= NULL;
	FskErr				err;
	UInt32				len, i;

	bailIfError(FskCanvas2dGetLineDash(ctx, &len, &dash));
	len *= 2;																	/* Convert from cycles to length */
	xsResult = xsNew1(xsGlobal, xsID("Array"), xsInteger(len));
	for (i = 0; i < len; i++)
		xsSetAt(xsResult, xsInteger(i), xsNumber(dash[i]));
bail:
	(void)FskMemPtrDispose(dash);
	return;
}

void KPR_canvasRenderingContext2D_set_lineDashOffset(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue offset = xsToNumber(xsArg(0));
	FskCanvas2dSetLineDashOffset(ctx, offset);
}

void KPR_canvasRenderingContext2D_get_lineDashOffset(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsResult = xsNumber(FskCanvas2dGetLineDashOffset(ctx));
}

// path objects
void KPR_path2D(void* theData)
{
	FskCanvas2dPath path = (FskCanvas2dPath)theData;
	FskCanvas2dPathDispose(path);
}

void KPR_Path2D(xsMachine *the)
{
	FskCanvas2dPath		path	= NULL;
	FskErr				err;

	bailIfError(FskCanvas2dPathNew(&path));
	if (xsToInteger(xsArgc) > 0) {
		if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID("path2D")))) {
			FskCanvas2dPath fromPath = xsGetHostData(xsArg(0));
			BAIL_IF_NULL(fromPath, err, kFskErrNotFound);
			bailIfError(FskCanvas2dPathAppendPath(NULL, path, fromPath, NULL));
		}
		else {
			xsStringValue str = xsToString(xsArg(0));
			(void)FskCanvas2dPathAppendPathString(NULL, path, str);
		}
	}
	xsSetHostData(xsResult, path);
bail:
	if (err) FskCanvas2dPathDispose(path);
	return;
}

void KPR_path2D_beginPath(xsMachine *the)
{
	FskCanvas2dPath path = xsGetHostData(xsThis);
	FskCanvas2dPathBegin(NULL, path);
}

void KPR_path2D_closePath(xsMachine *the)
{
	FskCanvas2dPath path = xsGetHostData(xsThis);
	FskCanvas2dPathClose(NULL, path);
}

void KPR_path2D_moveTo(xsMachine *the)
{
	FskCanvas2dPath	path	= xsGetHostData(xsThis);
	xsNumberValue	x		= xsToNumber(xsArg(0));
	xsNumberValue	y		= xsToNumber(xsArg(1));
	FskCanvas2dPathMoveTo(NULL, path, x, y);
}

void KPR_path2D_lineTo(xsMachine *the)
{
	FskCanvas2dPath	path	= xsGetHostData(xsThis);
	xsNumberValue	x		= xsToNumber(xsArg(0));
	xsNumberValue	y		= xsToNumber(xsArg(1));
	FskCanvas2dPathLineTo(NULL, path, x, y);
}

void KPR_path2D_quadraticCurveTo(xsMachine *the)
{
	FskCanvas2dPath	path	= xsGetHostData(xsThis);
	xsNumberValue	cpx		= xsToNumber(xsArg(0));
	xsNumberValue	cpy		= xsToNumber(xsArg(1));
	xsNumberValue	x 		= xsToNumber(xsArg(2));
	xsNumberValue	y		= xsToNumber(xsArg(3));
	FskCanvas2dPathQuadraticCurveTo(NULL, path, cpx, cpy, x, y);
}

void KPR_path2D_bezierCurveTo(xsMachine *the)
{
	FskCanvas2dPath	path	= xsGetHostData(xsThis);
	xsNumberValue	cp1x	= xsToNumber(xsArg(0));
	xsNumberValue	cp1y	= xsToNumber(xsArg(1));
	xsNumberValue	cp2x	= xsToNumber(xsArg(2));
	xsNumberValue	cp2y	= xsToNumber(xsArg(3));
	xsNumberValue	x		= xsToNumber(xsArg(4));
	xsNumberValue	y		= xsToNumber(xsArg(5));
	FskCanvas2dPathBezierCurveTo(NULL, path, cp1x, cp1y, cp2x, cp2y, x, y);
}

void KPR_path2D_arcTo(xsMachine *the)
{
	FskCanvas2dPath path = xsGetHostData(xsThis);
	xsNumberValue x1		= xsToNumber(xsArg(0));
	xsNumberValue y1		= xsToNumber(xsArg(1));
	xsNumberValue x2		= xsToNumber(xsArg(2));
	xsNumberValue y2		= xsToNumber(xsArg(3));
	xsNumberValue radius	= xsToNumber(xsArg(4));
	FskCanvas2dPathArcTo(NULL, path, x1, y1, x2, y2, radius);
}

void KPR_path2D_rect(xsMachine *the)
{
	FskCanvas2dPath path = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	xsNumberValue w = xsToNumber(xsArg(2));
	xsNumberValue h = xsToNumber(xsArg(3));
	FskCanvas2dPathRect(NULL, path, x, y, w, h);
}

void KPR_path2D_arc(xsMachine *the)
{
	FskCanvas2dPath	path = xsGetHostData(xsThis);
	xsNumberValue	x				= xsToNumber(xsArg(0));
	xsNumberValue	y				= xsToNumber(xsArg(1));
	xsNumberValue	radius			= xsToNumber(xsArg(2));
	xsNumberValue	startAngle		= xsToNumber(xsArg(3));
	xsNumberValue	endAngle		= xsToNumber(xsArg(4));
	xsBooleanValue	anticlockwise	= (xsToInteger(xsArgc) > 5) ? xsTest(xsArg(5)) : 0;
	FskCanvas2dPathArc(NULL, path, x, y, radius, startAngle, endAngle, anticlockwise);
}

// imageData object
void KPR_imageData_destructor(void *hostData)
{
	if (hostData)
		FskCanvas2dDisposeImageData(hostData);
}

void KPR_imageData_get_width(xsMachine *the)
{
	FskCanvas2dImageData data = xsGetHostData(xsThis);
	xsResult = xsInteger(data->width);
}

void KPR_imageData_get_height(xsMachine *the)
{
	FskCanvas2dImageData data = xsGetHostData(xsThis);
	xsResult = xsInteger(data->height);
}

void KPR_imageData_get_data(xsMachine *the)
{
	FskCanvas2dImageData data = xsGetHostData(xsThis);
	xsResult = xsNewInstanceOf(xsChunkPrototype);
	xsSetHostData(xsResult, (void*)data->data.bytes);
	xsSetHostDestructor(xsResult, NULL);
	xsSet(xsResult, xsID("length"), xsInteger(data->data.length));
}


