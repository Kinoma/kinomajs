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
#define __FSKBITMAP_PRIV__
#define __FSKPORT_PRIV__
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
		bailIfError(FskCanvasNew(coordinates->width, coordinates->height, kFskBitmapFormatDefaultRGBA, &self->cnv));
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
		bailIfError(FskCanvasNew(width, height, kFskBitmapFormatDefaultRGBA, &self->cnv));
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
				bailIfError(FskCanvasNew(width, height, kFskBitmapFormatDefaultRGBA, &self->cnv));
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
				bailIfError(FskCanvasNew(width, bounds.height, kFskBitmapFormatDefaultRGBA, &self->cnv));
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
				bailIfError(FskCanvasNew(bounds.width, bounds.height, kFskBitmapFormatDefaultRGBA, &self->cnv));
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
	FskCanvas2dGradientStop stops[4];
	xsVars(2);
	xsVar(0) = xsGet(xsThis, xsID("stops"));
	c = xsToInteger(xsGet(xsVar(0), xsID("length")));
	if (c > 4) c = 4;
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
	FskCanvas2dGradientStop stops[4];
	xsVars(2);
	xsVar(0) = xsGet(xsThis, xsID("stops"));
	c = xsToInteger(xsGet(xsVar(0), xsID("length")));
	if (c > 4) c = 4;
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
	FskCanvas2dBeginPath(ctx);
}

void KPR_canvasRenderingContext2D_closePath(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dClosePath(ctx);
}

void KPR_canvasRenderingContext2D_moveTo(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	FskCanvas2dMoveTo(ctx, x, y);
}

void KPR_canvasRenderingContext2D_lineTo(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	FskCanvas2dLineTo(ctx, x, y);
}

void KPR_canvasRenderingContext2D_quadraticCurveTo(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue cpx = xsToNumber(xsArg(0));
	xsNumberValue cpy = xsToNumber(xsArg(1));
	xsNumberValue x = xsToNumber(xsArg(2));
	xsNumberValue y = xsToNumber(xsArg(3));
	FskCanvas2dQuadraticCurveTo(ctx, cpx, cpy, x, y);
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
	FskCanvas2dBezierCurveTo(ctx, cp1x, cp1y, cp2x, cp2y, x, y);
}

void KPR_canvasRenderingContext2D_arcTo(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x1 = xsToNumber(xsArg(0));
	xsNumberValue y1 = xsToNumber(xsArg(1));
	xsNumberValue x2 = xsToNumber(xsArg(2));
	xsNumberValue y2 = xsToNumber(xsArg(3));
	xsNumberValue radius = xsToNumber(xsArg(4));
	FskCanvas2dArcTo(ctx, x1, y1, x2, y2, radius);
}

void KPR_canvasRenderingContext2D_rect(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	xsNumberValue w = xsToNumber(xsArg(2));
	xsNumberValue h = xsToNumber(xsArg(3));
	FskCanvas2dRect(ctx, x, y, w, h);
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
	FskCanvas2dArc(ctx, x, y, radius, startAngle, endAngle, anticlockwise);
}

void KPR_canvasRenderingContext2D_fill(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dFill(ctx);
}

void KPR_canvasRenderingContext2D_stroke(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dStroke(ctx);
}

void KPR_canvasRenderingContext2D_clip(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	FskCanvas2dClip(ctx);
}

void KPR_canvasRenderingContext2D_isPointInPath(xsMachine *the)
{
	FskCanvas2dContext ctx = xsGetHostData(xsThis);
	xsNumberValue x = xsToNumber(xsArg(0));
	xsNumberValue y = xsToNumber(xsArg(1));
	xsResult = xsBoolean(FskCanvas2dIsPointInPath(ctx, x, y));
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
		*q = 0;
		if (state == 0) {
			if (!FskStrCompare(p, "normal"))
				{}
			else if (!FskStrCompare(p, "italic"))
				font.style = kFskFontStyleItalic;
			else if (!FskStrCompare(p, "oblique"))
				font.style = kFskFontStyleOblique;
			else if (!FskStrCompare(p, "inherit"))
				{}
			else if (!FskStrCompare(p, "bold"))
				font.weight = kFskFontWeightBold;
			else if (!FskStrCompare(p, "100"))
				font.weight = 100;
			else if (!FskStrCompare(p, "200"))
				font.weight = 200;
			else if (!FskStrCompare(p, "300"))
				font.weight = 300;
			else if (!FskStrCompare(p, "400"))
				font.weight = 400;
			else if (!FskStrCompare(p, "500"))
				font.weight = 500;
			else if (!FskStrCompare(p, "600"))
				font.weight = 600;
			else if (!FskStrCompare(p, "700"))
				font.weight = 700;
			else if (!FskStrCompare(p, "800"))
				font.weight = 800;
			else if (!FskStrCompare(p, "900"))
				font.weight = 900;
			else if (!FskStrCompare(p, "small-caps"))
				font.variant = kFskFontVariantSmallCaps;
			else
				state = 1;
		}
		if (state == 1) {
			if (!FskStrCompare(p, "xx-small"))
				font.size = 3 * kFskMediumFontSize / 5;
			else if (!FskStrCompare(p, "x-small"))
				font.size = 3 * kFskMediumFontSize / 4;
			else if (!FskStrCompare(p, "small"))
				font.size = 8 * kFskMediumFontSize / 9;
			else if (!FskStrCompare(p, "medium"))
				font.size = kFskMediumFontSize;
			else if (!FskStrCompare(p, "large"))
				font.size = 6 * kFskMediumFontSize / 5;
			else if (!FskStrCompare(p, "x-large"))
				font.size = 3 * kFskMediumFontSize / 2;
			else if (!FskStrCompare(p, "xx-large"))
				font.size = 2 * kFskMediumFontSize;
			else if (!FskStrCompare(p, "larger"))
				font.size = 6 * kFskMediumFontSize / 5; // @@
			else if (!FskStrCompare(p, "smaller"))
				font.size = 8 * kFskMediumFontSize / 9; // @@
			else {
				size = 0;
				while (((d = *p)) && ('0' <= d) && (d <= '9')) {
					size = (10 * size) + (d - '0');
					p++;
				}
				if (!FskStrCompare(p, "%"))
					font.size = parent->size * size / 100;
				else if (!FskStrCompare(p, "px"))
					font.size = size;
			}
			state = 2;
		}
		if (!c)
			break;
		*q = c;
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
