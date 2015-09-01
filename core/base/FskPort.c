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
#define __FSKTEXT_PRIV__
#define __FSKWINDOW_PRIV__
#include "FskPort.h"

#include <math.h>

#include "FskEffects.h"
#include "FskList.h"
#include "FskRectBlit.h"
#include "FskText.h"
#include "FskUtilities.h"
#include "FskWindow.h"
#if FSKBITMAP_OPENGL
    #include "FskGLBlit.h"
    #include "FskEnvironment.h"
#endif

static void updateAggregateClip(FskPort port);
static void flushTextFormat(FskPort port, Boolean changingFont);

#if SUPPORT_INSTRUMENTATION
	#include <stddef.h>
	#include "FskPixelOps.h"

	static FskInstrumentedValueRecord gInstrumentationPortValues[] = {
		{ "bitmap",				offsetof(FskPortRecord, bits),				kFskInstrumentationKindNamed,	"bitmap"},
		{ "window",				offsetof(FskPortRecord, window),			kFskInstrumentationKindNamed,	"window"},
		{ "font",				offsetof(FskPortRecord, fontName),			kFskInstrumentationKindString},
		{ NULL,					0,											kFskInstrumentationKindUndefined}
	};

	static Boolean doFormatMessagePort(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	FskInstrumentedTypeRecord gPortTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"port",
		FskInstrumentationOffset(FskPortRecord),
		NULL,
		0,
		NULL,
		doFormatMessagePort,
		gInstrumentationPortValues
	};
#endif

#define defineScalers(FACTOR) \
    static void scaleRect##FACTOR##x(FskPort port, FskRectangle r);      \
    static void unscaleRect##FACTOR##x(FskPort port, FskRectangle r);    \
    static UInt32 scaleUInt32##FACTOR##x(FskPort port, UInt32 v);        \
    static UInt32 unscaleUInt32##FACTOR##x(FskPort port, UInt32 v);      \
    static SInt32 scaleSInt32##FACTOR##x(FskPort port, SInt32 v);        \
    static SInt32 unscaleSInt32##FACTOR##x(FskPort port, SInt32 v);      \
    static double scaleDouble##FACTOR##x(FskPort port, double v);        \
    static double unscaleDouble##FACTOR##x(FskPort port, double v);

defineScalers(1);
defineScalers(15);      // 1.5x
defineScalers(2);
defineScalers(3);
defineScalers(4);
defineScalers(Any);     // arbitrary

static void FskPortPurge(void *refcon);

FSKPORT_DECLARE_VECTOR_FUNCTIONS(nop);
FSKPORT_DECLARE_VECTOR_FUNCTIONS(picSave);
FSKPORT_DECLARE_VECTOR_DRAW_FUNCTIONS(render);

struct FskPortVectorsRecord gNopPort = FSKPORT_DECLARE_VECTOR_RECORD(nop);
struct FskPortVectorsRecord gPicSavePort = FSKPORT_DECLARE_VECTOR_RECORD(picSave);
struct FskPortVectorsRecord gRenderPort = {
    renderBeginDrawing,
    renderEndDrawing,
    renderRectangleFill,
    renderRectangleFrame,
    renderBitmapDraw,
    renderBitmapScaleOffset,
    renderBitmapDrawSubpixel,
    renderBitmapTile,
    renderTextDraw,
    renderPicSaveAdd,
    renderEffectApply,
    nopSetOrigin,
    nopSetClipRectangle,
    nopSetPenColor,
    nopTextFormatApply,
    nopSetTextAlignment,
    nopSetTextFont,
    nopSetTextSize,
    nopSetTextStyle,
    nopSetGraphicsMode,
    nopScaleSet,
    nopGetBitmap
};

FskErr FskPortNew(FskPort *portOut, FskBitmap bits, const char *textEngine)
{
	FskErr err;
	FskPort port;
    static const FskColorRGBARecord black = {0, 0, 0, 255};

	err = FskMemPtrNewClear(sizeof(FskPortRecord), &port);
	BAIL_IF_ERR(err);

    port->vector = gNopPort;

	err = FskTextEngineNew(&port->textEngine, textEngine);
	BAIL_IF_ERR(err);

	port->bits = bits;

	port->doScaleRect = scaleRect1x;
	FskPortScaleSet(port, FskIntToFixed(1));

	port->useCount = 1;

	FskInstrumentedItemNew(port, NULL, &gPortTypeInstrumentation);

	// set up initial port state
	FskPortSetUpdateRectangle(port, NULL);
	FskPortSetClipRectangle(port, NULL);
	FskPortSetPenColor(port, &black);
	FskPortSetGraphicsMode(port, kFskGraphicsModeCopy, NULL);
	FskPortSetTextSize(port, 12);

bail:
	*portOut = port;

	return err;
}

FskErr FskPortDispose(FskPort port)
{
	if (port) {
		port->useCount -= 1;
		if (port->useCount <= 0) {
			FskInstrumentedItemDispose(port);
            if (port->lowMemoryWarningRegistered)
                FskNotificationUnregister(kFskNotificationLowMemory, FskPortPurge, port);
			if ((void *)port->graphicsModeParameters != (void *)port->graphicsModeCache)
				FskMemPtrDispose(port->graphicsModeParameters);
			flushTextFormat(port, true);
			if (port->fontNameCache != port->fontName)
				FskMemPtrDispose(port->fontName);
			FskTextEngineDispose(port->textEngine);
            FskMemPtrDispose(port->picSave);
			#if FSKBITMAP_OPENGL
				FskGLEffectCacheDisposeAllBitmaps();
			#endif
			FskMemPtrDispose(port);
		}
	}

	return kFskErrNone;
}

FskErr FskPortSetOrigin(FskPort port, SInt32 x, SInt32 y)
{
#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(port, kFskInstrumentationLevelVerbose)) {
		SInt32 data[2];
		data[0] = x;
		data[1] = y;
		FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgSetOrigin, data);
	}
#endif

    return (port->vector.doSetOrigin)(port, x, y);
}

FskErr FskPortGetOrigin(FskPort port, SInt32 *x, SInt32 *y)
{
	*x = port->origin.x;
	*y = port->origin.y;

	return kFskErrNone;
}

FskErr FskPortOffsetOrigin(FskPort port, SInt32 x, SInt32 y)
{
#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(port, kFskInstrumentationLevelVerbose)) {
		SInt32 data[2];
		data[0] = x;
		data[1] = y;
		FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgOffsetOrigin, data);
	}
#endif

    if (!x && !y)
        return kFskErrNone;

    return (port->vector.doSetOrigin)(port, port->origin.x + x, port->origin.y + y);
}

FskErr FskPortSetGraphicsMode(FskPort port, UInt32 graphicsMode, FskConstGraphicsModeParameters graphicsModeParameters)
{
#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(port, kFskInstrumentationLevelVerbose)) {
		void *data[2];
		data[0] = (void*)graphicsMode;
		data[1] = (void*)graphicsModeParameters;
		FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgSetGraphicsMode, data);
	}
#endif

    return port->vector.doSetGraphicsMode(port, graphicsMode, graphicsModeParameters);
}

FskErr FskPortGetGraphicsMode(FskPort port, UInt32 *graphicsMode, FskGraphicsModeParameters *graphicsModeParameters)
{
	FskErr err = kFskErrNone;

	if (graphicsMode)
		*graphicsMode = port->graphicsMode;

	if (graphicsModeParameters) {
		if (NULL != port->graphicsModeParameters) {
			err = FskMemPtrNewFromData(port->graphicsModeParameters->dataSize, port->graphicsModeParameters, (FskMemPtr *)graphicsModeParameters);
			BAIL_IF_ERR(err);
		}
		else
			*graphicsModeParameters = NULL;
	}

bail:
	return err;
}

FskErr FskPortSetUpdateRectangle(FskPort port, FskConstRectangle updateRect)
{
	if (NULL != updateRect)
		port->updateRect = *updateRect;
	else
		FskRectangleSetFull(&port->updateRect);

	updateAggregateClip(port);

	return kFskErrNone;
}

FskErr FskPortGetUpdateRectangle(FskPort port, FskRectangle updateRect)
{
	*updateRect = port->updateRect;
	FskRectangleOffset(updateRect, -port->origin.x, -port->origin.y);
	return kFskErrNone;
}

FskErr FskPortSetClipRectangle(FskPort port, FskConstRectangle clip)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgSetClipRectangle, (void *)clip);

    return (port->vector.doSetClipRectangle)(port, clip);
}

void updateAggregateClip(FskPort port)
{
	FskRectangleIntersect(&port->clipRect, &port->updateRect, &port->aggregateClip);
	if (NULL != port->bits) {
		FskRectangleRecord bounds = port->bits->bounds;
		FskPortRectUnscale(port, &bounds);
		FskRectangleIntersect(&bounds, &port->aggregateClip, &port->aggregateClip);
	}
        port->aggregateClipScaled = port->aggregateClip;
		FskPortRectScale(port, &port->aggregateClipScaled);
	}

FskErr FskPortGetClipRectangle(FskPort port, FskRectangle clip)
{
	*clip = port->clipRect;
	FskRectangleOffset(clip, -port->origin.x, -port->origin.y);
	return kFskErrNone;
}

FskErr FskPortSetPenColor(FskPort port, FskConstColorRGBA color)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgSetPenColor, (void*)color);

    return (port->vector.doSetPenColor)(port, color);
}

FskErr FskPortGetPenColor(FskPort port, FskColorRGBA color)
{
	*color = port->penColor;
	return kFskErrNone;
}

FskErr FskPortSetTextAlignment(FskPort port, UInt16 hAlign, UInt16 vAlign)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgSetTextAlignment, (void *)((hAlign << 16) | vAlign));

    return (port->vector.doSetTextAlignment)(port, hAlign, vAlign);
}

FskErr FskPortGetTextAlignment(FskPort port, UInt16 *hAlign, UInt16 *vAlign)
{
	*hAlign = port->textHAlign;
	*vAlign = port->textVAlign;
	return kFskErrNone;
}

FskErr FskPortSetTextSize(FskPort port, UInt32 size)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgSetTextSize, (void *)(int)size);

    return (port->vector.doSetTextSize)(port, size);
}

FskErr FskPortGetTextSize(FskPort port, UInt32 *size)
{
	*size = port->textSize;
	return kFskErrNone;
}

FskErr FskPortSetTextStyle(FskPort port, UInt32 style)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgSetTextStyle, (void *)style);

    return (port->vector.doSetTextStyle)(port, style);
}

FskErr FskPortGetTextStyle(FskPort port, UInt32 *style)
{
	*style = port->textStyle;
	return kFskErrNone;
}

FskErr FskPortSetTextFont(FskPort port, const char *fontName)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgSetTextFont, (void *)fontName);

    return (port->vector.doSetTextFont)(port, fontName);
}

FskErr FskPortGetTextFont(FskPort port, char **font)
{
    return FskStrListDoCopy(port->fontName, font);
}

FskErr FskPortResetInvalidRectangle(FskPort port)
{
	FskRectangleSetEmpty(&port->invalidArea);
	if (port->window)
		FskWindowResetInvalidContentRectangle(port->window);

	return kFskErrNone;
}

FskErr FskPortInvalidateRectangle(FskPort port, FskConstRectangle area)
{
	FskRectangleRecord r;

	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgInvalidateRectangle, (void*)area);

	if (NULL == area) {
		if (NULL == port->bits)
			return kFskErrOperationFailed;
		r = port->bits->bounds;
		FskPortRectUnscale(port, &r);
	}
	else {
		r = *area;
		FskRectangleOffset(&r, port->origin.x, port->origin.y);
	}

	FskRectangleUnion(&r, &port->invalidArea, &port->invalidArea);
	if (port->window)
		FskWindowInvalidateContentRectangle(port->window, &r);

	return kFskErrNone;
}

FskErr FskPortGetInvalidRectangle(FskPort port, FskRectangle area)
{
	*area = port->invalidArea;
	FskRectangleOffset(area, -port->origin.x, -port->origin.y);
	return kFskErrNone;
}

FskErr FskPortSetBitmap(FskPort port, FskBitmap bits)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgSetBitmap, bits);

	port->bits = bits;

	updateAggregateClip(port);
	FskPortInvalidateRectangle(port, NULL);	// need to redraw port contents

	return kFskErrNone;
}

FskErr FskPortGetBitmap(FskPort port, FskBitmap *bits)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgGetBitmap, bits);
    return (port->vector.doGetBitmap)(port, bits);
}

FskErr FskPortGetBounds(FskPort port, FskRectangle bounds)
{
	*bounds = port->bits->bounds;
	FskPortRectUnscale(port, bounds);
	FskRectangleOffset(bounds, -port->origin.x, -port->origin.y);
	return kFskErrNone;
}

FskErr FskPortBeginDrawing(FskPort port, FskConstColorRGBA background)
{
	FskInstrumentedItemSendMessageNormal(port, kFskPortInstrMsgBeginDrawing, port->window);

    port->drawingDepth += 1;
    if (1 != port->drawingDepth)
        return kFskErrNone;

    FskRectangleSetEmpty(&port->changedArea);

    if (port->externalVector.doBeginDrawing)
        port->vector = port->externalVector;
    else if (FskBitmapIsOpenGLDestinationAccelerated(port->bits))
        port->vector = gPicSavePort;
    else
        port->vector = gRenderPort;

    return (port->vector.doBeginDrawing)(port, background);
}

FskErr FskPortEndDrawing(FskPort port)
{
    FskErr err;

	FskInstrumentedItemSendMessageNormal(port, kFskPortInstrMsgEndDrawing, NULL);

    port->drawingDepth -= 1;
    if (0 != port->drawingDepth)
        return kFskErrNone;

    err = (port->vector.doEndDrawing)(port);

    port->vector = gNopPort;

    return err;
}

Boolean FskPortAccumulateChange(FskPort port, FskConstRectangle area)
{
	FskRectangleRecord r;

	if (FskRectangleIntersect(&port->aggregateClip, area, &r)) {
		FskRectangleUnion(&r, &port->changedArea, &port->changedArea);
		return true;
	}

	return false;
}

void FskPortRectangleFill(FskPort port, FskConstRectangle rect)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgRectangleFill, (void*)rect);

    (port->vector.doRectangleFill)(port, rect);
}

void FskPortRectangleFrame(FskPort port, FskConstRectangle rect)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgRectangleFrame, (void*)rect);

    (port->vector.doRectangleFrame)(port, rect);
}

void FskPortBitmapDraw(FskPort port, FskBitmap bits, FskConstRectangle srcRect, FskConstRectangle dstRect)
{
#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(port, kFskInstrumentationLevelVerbose)) {
		void *p[3];

		p[0] = (void*)bits;
		p[1] = (void*)dstRect;
		p[2] = (void*)(srcRect ? srcRect : &bits->bounds);
		FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgBitmapDraw, p);
	}
#endif

    (port->vector.doBitmapDraw)(port, bits, srcRect, dstRect);
}

void FskPortBitmapScaleOffset(FskPort port, FskBitmap bits, FskConstRectangle srcRect, const struct FskScaleOffset *scaleOffset)
{
#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(port, kFskInstrumentationLevelVerbose)) {
		void *p[3];

		p[0] = (void*)bits;
		p[1] = (void*)srcRect;
		p[2] = (void*)scaleOffset;
		FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgBitmapScaleOffset, p);
	}
#endif

    (port->vector.doBitmapScaleOffset)(port, bits, srcRect, scaleOffset);
}

/** Draw a bitmap to the port with subpixel placement, to provide smoother scrolling.
 *	The maximum scale factor (i.e. ratio of port to src dimensions) is 127.99999994039536.
 *	\param[in,out]	port	the port.
 *	\param[in]		bits	the source bitmap to be drawn.
 *	\param[in]		srcRect	the subRect of the source bitmap to be drawn; NULL implies bits->bounds.
 *	\param[in]		x		the horizontal location in the port corresponding to srcRect->x.
 *	\param[in]		y		the  vertical  location in the port corresponding to srcRect->y.
 *	\param[in]		width	the width  in the port to which srcRect->width  is to be stretched.
 *	\param[in]		height	the height in the port to which srcRect->height is to be stretched.
 */

void FskPortBitmapDrawSubpixel(FskPort port, FskBitmap bits, FskConstRectangle srcRect, double x, double y, double width, double height)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgBitmapSubpixel, bits);
    (port->vector.doBitmapDrawSubpixel)(port, bits, srcRect, x, y, width, height);
}

void FskPortBitmapTile(FskPort port, FskBitmap srcBits, FskConstRectangle srcRectIn, FskConstRectangle dstRect, FskFixed scale)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgBitmapTile, srcBits);
    (port->vector.doBitmapTile)(port, srcBits, srcRectIn, dstRect, scale);

}
void FskPortTextDraw(FskPort port, const char *text, UInt32 textLen, FskConstRectangle bounds)
{
#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(port, kFskInstrumentationLevelVerbose)) {
		void *p[2];

		p[0] = (void*)text;
		p[1] = (void*)bounds;
		FskInstrumentedItemSendMessageForLevel(port, kFskPortInstrMsgTextDraw, p, kFskInstrumentationLevelVerbose);
	}
#endif

    (port->vector.doTextDraw)(port, text, textLen, bounds);
}

void FskPortStringDraw(FskPort port, const char *text, FskConstRectangle bounds)
{
	FskPortTextDraw(port, text, FskStrLen(text), bounds);
}

void FskPortTextGetBounds(FskPort port, const char *text, UInt32 textLen, FskRectangle bounds)
{
	if (NULL == port->textFormatCache) {
		if (kFskErrNone == FskTextFormatCacheNew(port->textEngine, &port->textFormatCache, port->bits, FskPortUInt32Scale(port, port->textSize), port->textStyle, port->fontName))
            port->textFromFormat = false;
	}
	FskTextGetBounds(port->textEngine, port->bits, text, textLen, FskPortUInt32Scale(port, port->textSize), port->textStyle, port->fontName, bounds, port->textFormatCache);
	FskPortRectUnscale(port, bounds);

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(port, kFskInstrumentationLevelVerbose)) {
		void *temp[3];
		temp[0] = (void *)text;
		temp[1] = (void *)textLen;
		temp[2] = (void *)bounds;
		FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgTextGetBounds, (void *)temp);
	}
#endif
}

void FskPortStringGetBounds(FskPort port, const char *text, FskRectangle bounds)
{
	FskPortTextGetBounds(port, text, FskStrLen(text), bounds);
}

FskErr FskPortTextFitWidth(FskPort port, const char *text, UInt32 textLen, UInt32 width, UInt32 flags, UInt32 *fitBytes, UInt32 *fitChars)
{
	FskErr err;

	if (NULL == port->textFormatCache) {
		if (kFskErrNone == FskTextFormatCacheNew(port->textEngine, &port->textFormatCache, port->bits, FskPortUInt32Scale(port, port->textSize), port->textStyle, port->fontName))
            port->textFromFormat = false;
	}

	width = FskPortUInt32Scale(port, width);
	err = FskTextFitWidth(port->textEngine, port->bits, text, textLen, FskPortUInt32Scale(port, port->textSize), port->textStyle, port->fontName, width, flags, fitBytes, fitChars, port->textFormatCache);

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(port, kFskInstrumentationLevelVerbose)) {
		void *temp[4];
		temp[0] = (void *)text;
		temp[1] = (void *)textLen;
		temp[2] = (void *)fitBytes;
		temp[3] = (void *)fitChars;
		FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgTextFitWidth, (void *)temp);
	}
#endif

	return err;
}

FskErr FskPortGetFontInfo(FskPort port, FskTextFontInfo info)
{
	FskErr err;
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgGetFontInfo, NULL);

	if (NULL == port->textFormatCache) {
		if (kFskErrNone == FskTextFormatCacheNew(port->textEngine, &port->textFormatCache, port->bits, FskPortUInt32Scale(port, port->textSize), port->textStyle, port->fontName))
            port->textFromFormat = false;
	}

    info->glyphHeight = ~0;
	err = FskTextGetFontInfo(port->textEngine, info, port->fontName, FskPortUInt32Scale(port, port->textSize), port->textStyle, port->textFormatCache);
	if (kFskErrNone == err) {
		info->ascent = FskPortUInt32Unscale(port, info->ascent);
		info->descent = FskPortUInt32Unscale(port, info->descent);
		info->leading = FskPortUInt32Unscale(port, info->leading);
		info->height = FskPortUInt32Unscale(port, info->height);
        if ((UInt32)~0 != info->glyphHeight)
            info->glyphHeight = FskPortUInt32Unscale(port, info->glyphHeight);
        else
            info->glyphHeight = info->ascent + info->descent;
	}
	return err;
}

FskErr FskPortTextFormatNew(FskPortTextFormat *textFormatOut, FskPort port, UInt32 textSize, UInt32 textStyle, const char *fontNames)
{
	FskErr err;
	FskPortTextFormat textFormat = NULL, walker;
	UInt32 fontNameLen = 0;

	for (walker = port->textFormats; NULL != walker; walker = walker->next) {
		if ((walker->textSize == textSize) && (walker->textStyle == textStyle) && (0 == FskStrListCompare(fontNames, walker->fontName))) {
			walker->useCount += 1;
			*textFormatOut = walker;
			return kFskErrNone;
		}
	}

	fontNameLen = FskStrListLen(fontNames);

	err = FskMemPtrNewClear(sizeof(FskPortTextFormatRecord) + fontNameLen, &textFormat);
	BAIL_IF_ERR(err);

	FskListPrepend((FskList*)(void*)&port->textFormats, textFormat);
	port->useCount += 1;
	textFormat->port = port;
	textFormat->useCount = 1;

	FskMemMove(textFormat->fontName, fontNames, fontNameLen);
	textFormat->textSize = textSize;
	textFormat->textStyle = textStyle;

	err = FskTextFormatCacheNew(port->textEngine, &textFormat->textFormatCache, port->bits, FskPortUInt32Scale(port, textSize), textStyle, fontNames);
	if (kFskErrUnimplemented == err)
		err = kFskErrNone;
	else
        BAIL_IF_ERR(err);

bail:
	if (kFskErrNone != err) {
		FskPortTextFormatDispose(textFormat);
		textFormat = NULL;
	}

	*textFormatOut = textFormat;

	return err;
}

FskErr FskPortTextFormatDispose(FskPortTextFormat textFormat)
{
	FskErr err = kFskErrNone;

	if (NULL != textFormat) {
		textFormat->useCount -= 1;
		if (textFormat->useCount <= 0) {
			FskPort port = textFormat->port;
			if (port->textFormatCache == textFormat->textFormatCache) {
				port->textFormatCache = NULL;
				port->textFromFormat = false;
			}
			if (port->fontName == textFormat->fontName) {
				UInt32 fontNameLen = FskStrListLen(textFormat->fontName);
				err = FskMemPtrNew(fontNameLen, &port->fontName);
				BAIL_IF_ERR(err);
				FskMemMove(port->fontName, textFormat->fontName, fontNameLen);
			}
			FskTextFormatCacheDispose(port->textEngine, textFormat->textFormatCache);
			FskListRemove((FskList*)(void*)&port->textFormats, textFormat);
			FskPortDispose(port);		// decrement useCount
			FskMemPtrDispose(textFormat);
		}
	}

bail:
	return err;
}

FskErr FskPortTextFormatApply(FskPort port, FskPortTextFormat textFormat)
{
	if (NULL == textFormat)
		return kFskErrParameterError;

	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgTextFormatApply, textFormat);

	return (port->vector.doTextFormatApply)(port, textFormat);
}

void flushTextFormat(FskPort port, Boolean settingFont)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgTextFlushCache, NULL);

	if (false == port->textFromFormat)
		FskTextFormatCacheDispose(port->textEngine, port->textFormatCache);
	else {
		port->textFromFormat = false;
		if (settingFont)
			port->fontName = NULL;
		else {
			UInt32 fontNameLen = FskStrListLen(port->fontName);
			if (fontNameLen < sizeof(port->fontNameCache)) {
				FskMemMove(port->fontNameCache, port->fontName, fontNameLen);
				port->fontName = port->fontNameCache;
			}
			else
				FskStrListDoCopy(port->fontName, &port->fontName);
		}
	}

	port->textFormatCache = NULL;
}

FskErr FskPortTextGetEngine(FskPort port, char **engine)
{
	if (NULL == port->textEngine->dispatch->name) {
		*engine = NULL;
		return kFskErrNone;
	}

	*engine = FskStrDoCopy(port->textEngine->dispatch->name);
	return *engine ? kFskErrNone : kFskErrMemFull;
}

/********************************************************************************
 * GL Effects Manager
 ********************************************************************************/

#if FSKBITMAP_OPENGL
static void CheckGLEffectSourcesAreAccelerated(FskBitmap src, FskConstEffect e) {
	unsigned i;
	if (src)	(void)FskBitmapCheckGLSourceAccelerated(src);
	switch (e->effectID) {
		case kFskEffectMask:			(void)FskBitmapCheckGLSourceAccelerated((FskBitmap)e->params.mask.mask);					break;
		case kFskEffectShade:			(void)FskBitmapCheckGLSourceAccelerated((FskBitmap)e->params.shade.shadow);					break;
		case kFskEffectColorizeInner:	(void)FskBitmapCheckGLSourceAccelerated((FskBitmap)e->params.colorizeInner.matte);			break;
		case kFskEffectColorizeOuter:	(void)FskBitmapCheckGLSourceAccelerated((FskBitmap)e->params.colorizeOuter.matte);			break;
		case kFskEffectCompound:		for (i = e->params.compound.numStages; i--;) CheckGLEffectSourcesAreAccelerated(NULL, ++e);	break;
		default:																													break;
	}
}
#endif

#define kEffCacheTimeout		200		/* Keep an effect cache around for this many frames */

#if FSKBITMAP_OPENGL && GLES_VERSION == 2

//@@
#if SUPPORT_INSTRUMENTATION
void FskPortLogEffect(FskPort port, void *params, const char *name) {
	if (FskInstrumentedItemHasListenersForLevel(port, kFskInstrumentationLevelDebug)) {
		struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
		void *p[4];
		p[0] = (void*)(portEffect->src);
		p[1] = (void*)(&portEffect->srcRect);
		p[2] = (void*)(&portEffect->dstPoint);
		p[3] = (void*)params;
		FskInstrumentedItemPrintfDebug(port, "Phase %s: ", name);
		FskInstrumentedItemSendMessageDebug(port, kFskPortInstrMsgEffect, p);
	}
}
#endif

static FskErr EffectPostCompositePrepare(FskPort port, void *params, UInt32 paramsSize)
{
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskBitmap		mid1	= NULL;
	int				flags	= portEffect->src->hasAlpha ? kFskGLEffectCacheBitmapWithAlpha : 0;
	FskErr			err;

	if (paramsSize) {}	/* Quiet unused parameter messages */
	FskPortEffectCheckSourcesAreAccelerated(portEffect->src, &portEffect->effect);
	(void)FskPortEffectGetSrcCompatibleGLCacheBitmap(port, portEffect->src, &portEffect->srcRect, flags, &mid1);	/* Just the rect */
	err = FskGLEffectApply(&portEffect->effect, portEffect->src, &portEffect->srcRect, mid1, NULL);					/* This takes care of boundary conditions */
    FskBitmapDispose(portEffect->src);																				/* This merely decreases the useCount */
	portEffect->src = mid1;																							/* The src is now just the rect */
	return err;
}


static FskErr EffectTmpCompositeRender(FskPort port, void *params, UInt32 paramsSize)
{
	struct FskPortPicEffectParametersRecord *portEffect = (struct FskPortPicEffectParametersRecord*)params;
	FskRectangleRecord 	dstRect;
	FskErr err;

	if (paramsSize) {}	/* Quiet unused parameter messages */
	FskRectangleSet(&dstRect, portEffect->dstPoint.x, portEffect->dstPoint.y, portEffect->src->bounds.width, portEffect->src->bounds.height);
	err = FskGLBitmapDraw(portEffect->src, &portEffect->src->bounds, port->bits, &dstRect, &port->aggregateClipScaled, &port->penColor, port->graphicsMode, port->graphicsModeParameters);
	FskGLEffectCacheReleaseBitmap(portEffect->src);																	/* Done with intermediate bitmap */
	return err;
}

#endif


void FskPortEffectApply(FskPort port, FskBitmap src, FskConstRectangle srcRect, FskConstPoint dstPoint, FskConstEffect effect)
{
#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(port, kFskInstrumentationLevelVerbose)) {
		void *p[4];

		p[0] = (void*)src;
		p[1] = (void*)srcRect;
		p[2] = (void*)dstPoint;
		p[3] = (void*)effect;
		FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgEffect, p);
	}
#endif

    (port->vector.doEffectApply)(port, src, srcRect, dstPoint, effect);
}

#define MAX_YUV_FORMATS	6	/* 420, 420spuv, 420spvu, 420i, UYVY, 422 */

FskErr FskPortPreferredYUVFormatsGet(FskPort port, FskBitmapFormatEnum **yuvFormat)
{
	FskErr err = kFskErrNone;
	FskBitmapFormatEnum *fmtp;

	BAIL_IF_NULL(yuvFormat, err, kFskErrParameterError);
	BAIL_IF_ERR(err = FskMemPtrNewClear((MAX_YUV_FORMATS+1) * sizeof(**yuvFormat), yuvFormat));
	fmtp = *yuvFormat;

	#if FSKBITMAP_OPENGL
		if (port && FskBitmapIsOpenGLDestinationAccelerated(port->bits)) {  /* Hardware and firmware preferences */
			UInt32 stp[4];													/* Source types with increasing cost */
			FskGLSourceTypes(stp);											/* 0: hardware; 1: shader; 2: in-place lossless conversion; 3: conversion at a high price */
			if (stp[0] & (1 << kFskBitmapFormatYUV420))						/* If there is hardware support for YUV 4:2:0 planar, ... */
				*fmtp++ = kFskBitmapFormatYUV420;							/* ... it is the next choice */
			if (stp[0] & (1 << kFskBitmapFormatYUV420spuv))					/* If there is hardware support for YUV 4:2:0 semi-planar, ... */
				*fmtp++ = kFskBitmapFormatYUV420spuv;						/* ... it is the next choice */
			if (stp[0] & (1 << kFskBitmapFormatYUV420spvu))					/* If there is hardware support for YUV 4:2:0 semi-planar, ... */
				*fmtp++ = kFskBitmapFormatYUV420spvu;						/* ... it is the next choice */
			if (stp[0] & (1 << kFskBitmapFormatUYVY))						/* If there is hardware support for YUV 4:2:2 UYVY chunky, ... */
				*fmtp++ = kFskBitmapFormatUYVY;								/* ... it is the next choice */
			#if GLES_VERSION == 2
				if ((stp[0] ^ stp[1]) & (1 << kFskBitmapFormatYUV420spuv))	/* If YUV 4:2:0sp was not supported in hardware, but is in a shader, ... */
					*fmtp++ = kFskBitmapFormatYUV420spuv;					/* ... it is the next choice */
				if ((stp[0] ^ stp[1]) & (1 << kFskBitmapFormatYUV420spvu))	/* If YUV 4:2:0sp was not supported in hardware, but is in a shader, ... */
					*fmtp++ = kFskBitmapFormatYUV420spvu;					/* ... it is the next choice */
				if ((stp[0] ^ stp[1]) & (1 << kFskBitmapFormatYUV420))		/* If YUV 4:2:0 was not supported in hardware, but is in a shader, ... */
					*fmtp++ = kFskBitmapFormatYUV420;						/* ... it is the next choice */
				if ((stp[0] ^ stp[1]) & (1 << kFskBitmapFormatUYVY))		/* If YUV 4:2:2 UYVY was not supported in hardware, but is in a shader, ... */
					*fmtp++ = kFskBitmapFormatUYVY;							/* ... it is the next choice */
			#endif
			/* We skip analysis of stp[2] because that is convert-in-place, which we can't do with YUV */
			if ((stp[1] ^ stp[3]) & (1 << kFskBitmapFormatYUV420))			/* If YUV 4:2:0 was not supported in HW or shader, but can be converted at any expense ... */
				*fmtp++ = kFskBitmapFormatYUV420;							/* ... it is the next choice */
			if ((stp[1] ^ stp[3]) & (1 << kFskBitmapFormatYUV420spuv))		/* If YUV 4:2:0sp was not supported in HW or shader, but can be converted at any expense ... */
				*fmtp++ = kFskBitmapFormatYUV420spuv;						/* ... it is the next choice */
			if ((stp[1] ^ stp[3]) & (1 << kFskBitmapFormatYUV420spvu))		/* If YUV 4:2:0sp was not supported in HW or shader, but can be converted at any expense ... */
				*fmtp++ = kFskBitmapFormatYUV420spvu;						/* ... it is the next choice */
			if ((stp[1] ^ stp[3]) & (1 << kFskBitmapFormatUYVY))			/* If YUV 4:2:2 UYVY was not supported in HW or shader, but can be converted at any expense ... */
				*fmtp++ = kFskBitmapFormatUYVY;								/* ... it is the next choice */
		}
		else
	#endif

	{																	/* Software preferences */
		if (SRC_YUV420i)
			*fmtp++ = kFskBitmapFormatYUV420i;							/* Software 420i is smallest and fastest */
		if (SRC_YUV420spuv)
			*fmtp++ = kFskBitmapFormatYUV420spuv;						/* Software 420spuv is smallest and second fastest */
		if (SRC_YUV420spvu)
			*fmtp++ = kFskBitmapFormatYUV420spvu;						/* Software 420spuv is smallest and second fastest */
		if (SRC_YUV420)
			*fmtp++ = kFskBitmapFormatYUV420;							/* Software 420 is smallest */
		if (SRC_UYVY)
			*fmtp++ = kFskBitmapFormatUYVY;								/* Chunky 4:2:2, ordering UYVY */
		if (SRC_YUV422)
			*fmtp++ = kFskBitmapFormatYUV422;							/* Planar 4:2:2, but is it UYVY, VYUY, YUYV or YVYU? */
	}
	if (fmtp == *yuvFormat)
		err = kFskErrUnsupportedPixelType;

	#if SUPPORT_INSTRUMENTATION
	{	char types[256];
		types[0] = 0;
		for (fmtp = *yuvFormat; *fmtp != 0; ++fmtp) {
			switch (*fmtp) {
				case kFskBitmapFormatYUV420:		FskStrCat(types, " 420");		break;
				case kFskBitmapFormatYUV420spuv:	FskStrCat(types, " 420spuv");	break;
				case kFskBitmapFormatYUV420spvu:	FskStrCat(types, " 420spvu");	break;
				case kFskBitmapFormatYUV420i:		FskStrCat(types, " 420i");		break;
				case kFskBitmapFormatUYVY:			FskStrCat(types, " UYVY");		break;
				case kFskBitmapFormatYUV422:		FskStrCat(types, " 422");		break;
				default:															break;
			}
		}
		#if FSKBITMAP_OPENGL
			FskInstrumentedTypePrintfVerbose(&gPortTypeInstrumentation, "FskPortPreferredYUVFormatsGet is%s using GL and returns%s",
					((port && FskBitmapIsOpenGLDestinationAccelerated(port->bits)) ? "" : " not"), types);
		#else
			FskInstrumentedTypePrintfVerbose(&gPortTypeInstrumentation, "FskPortPreferredYUVFormatsGet is not using GL and returns%s" , types);
		#endif
	}
	#endif

bail:
	return err;
}

#if FSKBITMAP_OPENGL
static SInt8 sUsingGL = -1;
#endif

FskErr FskPortPreferredRGBFormatsGet(FskPort port, FskBitmapFormatEnum *noAlpha, FskBitmapFormatEnum *withAlpha)
{
	if (withAlpha)
		*withAlpha = kFskBitmapFormatSourceDefaultRGBA;

	if (noAlpha)
		*noAlpha = kFskBitmapFormatDefaultRGB;

#if FSKBITMAP_OPENGL
    if (-1 == sUsingGL) {
        const char *value = FskEnvironmentGet("useGL");
        sUsingGL = value && (0 == FskStrCompare("1", value));
    }

	if (withAlpha) {
        if ((SRC_32RGBA && (kFskBitmapFormatSourceDefaultRGBA != kFskBitmapFormat32RGBA)) ||
            (SRC_32BGRA && (kFskBitmapFormatSourceDefaultRGBA != kFskBitmapFormat32BGRA)) ||
            (SRC_32RGBA && (kFskBitmapFormatDefaultRGB        != kFskBitmapFormat32RGBA))
        ) {
            if (port ? FskBitmapIsOpenGLDestinationAccelerated(port->bits) : sUsingGL) {
                if (SRC_32RGBA)
                    *withAlpha = kFskBitmapFormat32RGBA;		// prefer RGBA
                else if (SRC_32BGRA)
                    *withAlpha = kFskBitmapFormat32BGRA;		// BGRA is OK too
            }
        }
	}

	if (noAlpha) {
        if ((kFskBitmapFormat16RGB565LE != kFskBitmapFormatDefaultRGB) && (
            (SRC_32RGBA && (kFskBitmapFormatDefaultRGB != kFskBitmapFormat32RGBA)) ||
            (SRC_32BGRA && (kFskBitmapFormatDefaultRGB != kFskBitmapFormat32BGRA)) ||
            (SRC_32RGBA && (kFskBitmapFormatDefaultRGB != kFskBitmapFormat32RGBA))
        )) {
            if (port ? FskBitmapIsOpenGLDestinationAccelerated(port->bits) : sUsingGL) {
                if (SRC_32RGBA)
                    *noAlpha = kFskBitmapFormat32RGBA;		// prefer RGBA
                else if (SRC_32BGRA)
                    *noAlpha = kFskBitmapFormat32BGRA;		// BGRA is OK too
            }
        }
	}
#endif

	return kFskErrNone;
}

FskFixed FskPortScaleGet(FskPort port)
{
	return port->scale;
}

void FskPortScaleSet(FskPort port, FskFixed scale)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgScaleSet, (void*)scale);

    (port->vector.doScaleSet)(port, scale);
}

void FskPortSetVectors(FskPort port, FskPortVectors vector)
{
#define patchVector(name) \
        port->externalVector.do##name = vector->do##name ? vector->do##name : gNopPort.do##name;
    if (vector) {
        patchVector(BeginDrawing);
        patchVector(EndDrawing);
        patchVector(RectangleFill);
        patchVector(RectangleFrame);
        patchVector(BitmapDraw);
        patchVector(BitmapScaleOffset);
        patchVector(BitmapDrawSubpixel);
        patchVector(BitmapTile);
        patchVector(TextDraw);
        patchVector(PicSaveAdd);
        patchVector(EffectApply);
        patchVector(SetOrigin);
        patchVector(SetClipRectangle);
        patchVector(SetPenColor);
        patchVector(TextFormatApply);
        patchVector(SetTextAlignment);
        patchVector(SetTextFont);
        patchVector(SetTextSize);
        patchVector(SetTextStyle);
        patchVector(SetGraphicsMode);
        patchVector(ScaleSet);
        patchVector(GetBitmap);
    }
    else
        port->externalVector.doBeginDrawing = NULL;
}

/*
    scale functions
*/

void scaleRectAnyx(FskPort port, FskRectangle r)
{
	SInt32 scale = port->scale;

	r->x = FskFixMul(r->x, scale);
	r->y = FskFixMul(r->y, scale);
	r->width = FskFixMul(r->width, scale);
	r->height = FskFixMul(r->height, scale);
}

// not a perfect match for discrete scale factors
void unscaleRectAnyx(FskPort port, FskRectangle r)
{
	double scale = (double)port->scale / 65536.0;

	r->x      = (SInt32)(r->x      / scale);
	r->y      = (SInt32)(r->y      / scale);
	r->width  = (SInt32)(r->width  / scale);
	r->height = (SInt32)(r->height / scale);
}

UInt32 scaleUInt32Anyx(FskPort port, UInt32 v)
{
	return FskFixMul(v, port->scale);
}

UInt32 unscaleUInt32Anyx(FskPort port, UInt32 v)
{
	return (UInt32)(v / ((double)port->scale / 65536.0));
}

SInt32 scaleSInt32Anyx(FskPort port, SInt32 v)
{
	return FskFixMul(v, port->scale);
}

SInt32 unscaleSInt32Anyx(FskPort port, SInt32 v)
{
	return (SInt32)(v / ((double)port->scale / 65536.0));
}

double scaleDoubleAnyx(FskPort port, double v)
{
	return v * (double)port->scale / 65536.0;
}

double unscaleDoubleAnyx(FskPort port, double v)
{
	return v / ((double)port->scale / 65536.0);
}

void scaleRect1x(FskPort port, FskRectangle r)
{
	return;
}

void unscaleRect1x(FskPort port, FskRectangle r)
{
	return;
}

UInt32 scaleUInt321x(FskPort port, UInt32 v)
{
	return v;
}

UInt32 unscaleUInt321x(FskPort port, UInt32 v)
{
	return v;
}

SInt32 scaleSInt321x(FskPort port, SInt32 v)
{
	return v;
}

SInt32 unscaleSInt321x(FskPort port, SInt32 v)
{
	return v;
}

double scaleDouble1x(FskPort port, double v)
{
	return v;
}

double unscaleDouble1x(FskPort port, double v)
{
	return v;
}

void scaleRect2x(FskPort port, FskRectangle r)
{
	r->x <<= 1;
	r->y <<= 1;
	r->width <<= 1;
	r->height <<= 1;
}

void unscaleRect2x(FskPort port, FskRectangle r)
{
	SInt32 dx = r->x + r->width, dy = r->y + r->height;
	r->x >>= 1;
	r->y >>= 1;
	r->width = ((dx + 1) >> 1) - r->x;
	r->height = ((dy + 1) >> 1) - r->y;
}

UInt32 scaleUInt322x(FskPort port, UInt32 v)
{
	return v << 1;
}

UInt32 unscaleUInt322x(FskPort port, UInt32 v)
{
	return (v + 1) >> 1;
}

SInt32 scaleSInt322x(FskPort port, SInt32 v)
{
	return v << 1;
}

SInt32 unscaleSInt322x(FskPort port, SInt32 v)
{
	return (v + 1) >> 1;
}

double scaleDouble2x(FskPort port, double v)
{
	return v * 2.0;
}

double unscaleDouble2x(FskPort port, double v)
{
	return v / 2.0;
}

void scaleRect3x(FskPort port, FskRectangle r)
{
	r->x *= 3;
	r->y *= 3;
	r->width *= 3;
	r->height *= 3;
}

void unscaleRect3x(FskPort port, FskRectangle r)
{
	SInt32 dx = r->x + r->width, dy = r->y + r->height;
	r->x /= 3;
	r->y /= 3;
	r->width = ((dx + 2) / 3) - r->x;
	r->height = ((dy + 2) / 3) - r->y;
}

UInt32 scaleUInt323x(FskPort port, UInt32 v)
{
	return v + v + v;
}

UInt32 unscaleUInt323x(FskPort port, UInt32 v)
{
	return (v + 2) / 3;
}

SInt32 scaleSInt323x(FskPort port, SInt32 v)
{
	return v + v + v;
}

SInt32 unscaleSInt323x(FskPort port, SInt32 v)
{
	return (v + 2) / 3;
}

double scaleDouble3x(FskPort port, double v)
{
	return v * 3.0;
}

double unscaleDouble3x(FskPort port, double v)
{
	return v / 3.0;
}

void scaleRect4x(FskPort port, FskRectangle r)
{
	r->x <<= 2;
	r->y <<= 2;
	r->width <<= 2;
	r->height <<= 2;
}

void unscaleRect4x(FskPort port, FskRectangle r)
{
	SInt32 dx = r->x + r->width, dy = r->y + r->height;
	r->x >>= 2;
	r->y >>= 2;
	r->width = ((dx + 3) >> 2) - r->x;
	r->height = ((dy + 3) >> 2) - r->y;
}

UInt32 scaleUInt324x(FskPort port, UInt32 v)
{
	return v << 2;
}

UInt32 unscaleUInt324x(FskPort port, UInt32 v)
{
	return (v + 3) >> 2;
}

SInt32 scaleSInt324x(FskPort port, SInt32 v)
{
	return v << 2;
}

SInt32 unscaleSInt324x(FskPort port, SInt32 v)
{
	return (v + 3) >> 2;
}

double scaleDouble4x(FskPort port, double v)
{
	return v * 4.0;
}

double unscaleDouble4x(FskPort port, double v)
{
	return v / 4.0;
}

#define SCALE15x(x)		((x) + (((x) + 1) >> 1))
#define UNSCALE15x(x)	((((x) << 1) + 2) / 3)

void scaleRect15x(FskPort port, FskRectangle r)
{
	SInt32 dx = r->x + r->width, dy = r->y + r->height;
	r->x = SCALE15x(r->x);
	r->y = SCALE15x(r->y);
	r->width = SCALE15x(dx) - r->x;
	r->height = SCALE15x(dy) - r->y;
}

void unscaleRect15x(FskPort port, FskRectangle r)
{
	SInt32 dx = r->x + r->width, dy = r->y + r->height;
	r->x = UNSCALE15x(r->x);
	r->y = UNSCALE15x(r->y);
	r->width = UNSCALE15x(dx) - r->x;
	r->height = UNSCALE15x(dy) - r->y;
}

UInt32 scaleUInt3215x(FskPort port, UInt32 v)
{
	return SCALE15x(v);
}

SInt32 unscaleSInt3215x(FskPort port, SInt32 v)
{
	return UNSCALE15x(v);
}

SInt32 scaleSInt3215x(FskPort port, SInt32 v)
{
	return SCALE15x(v);
}

UInt32 unscaleUInt3215x(FskPort port, UInt32 v)
{
	return UNSCALE15x(v);
}

double scaleDouble15x(FskPort port, double v)
{
	return v * 1.5;
}

double unscaleDouble15x(FskPort port, double v)
{
	return v / 1.5;
}

void FskPortPurge(void *refcon)
{
    FskPort port = refcon;

    FskMemPtrDisposeAt(&port->picSave);
#if FSKBITMAP_OPENGL
    (void)FskGLEffectCacheDisposeAllBitmaps();
#endif
}

FskErr FskPortPicSaveAdd(FskPort port, FskPortPicRenderItem render, FskPortPicPrepareItem prepare, void *params, UInt32 paramsSize)
{
	FskInstrumentedItemSendMessageVerbose(port, kFskPortInstrMsgPicSaveAdd, NULL);
    return (port->vector.doPicSaveAdd)(port, render, prepare, params, paramsSize);
}

/*
    No-op drawing functions

        these never draw
        they only update the port state
*/

#define kStyleMask (kFskTextBold | kFskTextItalic | kFskTextUnderline | kFskTextStrike | kFskTextOutline | kFskTextOutlineHeavy )

FskErr nopBeginDrawing(FskPort port, FskConstColorRGBA background)
{
    return kFskErrNone;
}

FskErr nopEndDrawing(FskPort port)
{
    return kFskErrNone;
}

void nopRectangleFill(FskPort port, FskConstRectangle rect)
{
}

void nopRectangleFrame(FskPort port, FskConstRectangle rect)
{
}

void nopBitmapDraw(FskPort port, FskBitmap bits, FskConstRectangle srcRect, FskConstRectangle dstRect)
{
}

void nopBitmapScaleOffset(FskPort port, FskBitmap bits, FskConstRectangle srcRect, const struct FskScaleOffset *scaleOffset)
{
}

void nopBitmapDrawSubpixel(FskPort port, FskBitmap bits, FskConstRectangle srcRect, double x, double y, double width, double height)
{
}

void nopBitmapTile(FskPort port, FskBitmap srcBits, FskConstRectangle srcRect, FskConstRectangle dstRect, FskFixed scale)
{
}

void nopTextDraw(FskPort port, const char *text, UInt32 textLen, FskConstRectangle bounds)
{
}

FskErr nopPicSaveAdd(FskPort port, FskPortPicRenderItem render, FskPortPicPrepareItem prepare, void *params, UInt32 paramsSize)
{
    return kFskErrNone;
}

void nopEffectApply(FskPort port, FskBitmap src, FskConstRectangle srcRect, FskConstPoint dstPoint, const struct FskEffectRecord* effect)
{
}

FskErr nopSetOrigin(FskPort port, SInt32 x, SInt32 y)
{
	port->origin.x = x;
	port->origin.y = y;

    return kFskErrNone;
}

FskErr nopSetClipRectangle(FskPort port, FskConstRectangle clipIn)
{
    FskRectangleRecord clip;

	if (NULL != clipIn) {
		clip = *clipIn;
		FskRectangleOffset(&clip, port->origin.x, port->origin.y);
	}
	else
		FskRectangleSetFull(&clip);

    if (FskRectangleIsEqual(&clip, &port->clipRect))
        return kFskErrNone;

    port->clipRect = clip;
	updateAggregateClip(port);

    return kFskErrNone;
}

FskErr nopSetPenColor(FskPort port, FskConstColorRGBA color)
{
    port->penColor = *color;

    return kFskErrNone;
}

FskErr nopTextFormatApply(FskPort port, FskPortTextFormat textFormat)
{
	if (NULL == textFormat)
		return kFskErrParameterError;

    if (NULL == port)
        port = textFormat->port;
    else if (port->textEngine->dispatch != textFormat->port->textEngine->dispatch) {
        FskInstrumentedItemPrintfVerbose(port, " warning - uncached case in FskPortTextFormatApply");
        FskPortSetTextSize(port, textFormat->textSize);
        FskPortSetTextFont(port, textFormat->fontName);
        FskPortSetTextStyle(port, textFormat->textStyle);
        return kFskErrNone;
    }

	if (port->textFormatCache && (port->textFormatCache == textFormat->textFormatCache)) {
		// the part of the style outside kStyleMask may still be different, so need to reassert that
		port->textStyle = textFormat->textStyle;

		return kFskErrNone;
	}

	flushTextFormat(port, false);

	port->textFromFormat = true;
	port->textFormatCache = textFormat->textFormatCache;
	if (port->fontNameCache != port->fontName)
		FskMemPtrDispose(port->fontName);
	port->fontName = textFormat->fontName;
	port->textSize = textFormat->textSize;
	port->textStyle = textFormat->textStyle;

	return kFskErrNone;
}

FskErr nopSetTextAlignment(FskPort port, UInt16 hAlign, UInt16 vAlign)
{
    port->textHAlign = hAlign;
    port->textVAlign = vAlign;

    return kFskErrNone;
}


FskErr nopSetTextSize(FskPort port, UInt32 size)
{
	if (size != port->textSize) {
        flushTextFormat(port, false);
        port->textSize = size;
    }

    return kFskErrNone;
}

FskErr nopSetTextStyle(FskPort port, UInt32 style)
{
	if (port->textStyle != style) {
		if ((kStyleMask & port->textStyle) != (kStyleMask & style))
			flushTextFormat(port, false);

		port->textStyle = style;
	}

    return kFskErrNone;
}

FskErr nopSetGraphicsMode(FskPort port, UInt32 graphicsMode, FskConstGraphicsModeParameters graphicsModeParameters)
{
	FskErr err = kFskErrNone;

    if ((graphicsMode == port->graphicsMode) && (NULL == graphicsModeParameters) && (NULL == port->graphicsModeParameters))
        return kFskErrNone;

    port->graphicsMode = graphicsMode;

    if (port->graphicsModeParameters) {
        if ((void *)port->graphicsModeParameters != (void *)port->graphicsModeCache)
            FskMemPtrDispose(port->graphicsModeParameters);
        port->graphicsModeParameters = NULL;
    }

    if (NULL != graphicsModeParameters) {
        if (graphicsModeParameters->dataSize <= sizeof(port->graphicsModeCache)) {
            FskMemMove(port->graphicsModeCache, graphicsModeParameters, graphicsModeParameters->dataSize);
            port->graphicsModeParameters = /*(FskGraphicsModeParameters)*/(void*)(port->graphicsModeCache);
        }
        else {
            BAIL_IF_ERR(FskMemPtrNewFromData(graphicsModeParameters->dataSize, graphicsModeParameters, (FskMemPtr*)(void*)(&port->graphicsModeParameters)));
        }
    }

bail:
    return err;
}

void nopScaleSet(FskPort port, FskFixed scale)
{
	if (scale == port->scale)
		return;

	FskPortRectScale(port, &port->clipRect);
	FskPortRectScale(port, &port->updateRect);

	switch (scale) {
		case 0:
			FskExit(kFskErrInvalidParameter);
            break;

		case 65536:
			port->scale = scale;
			port->doScaleRect = scaleRect1x;
			port->doUnscaleRect = unscaleRect1x;
			port->doScaleUInt32 = scaleUInt321x;
			port->doUnscaleUInt32 = unscaleUInt321x;
			port->doScaleSInt32 = scaleSInt321x;
			port->doUnscaleSInt32 = unscaleSInt321x;
			port->doScaleDouble = scaleDouble1x;
			port->doUnscaleDouble = unscaleDouble1x;
			break;

		case 131072:
			port->scale = scale;
			port->doScaleRect = scaleRect2x;
			port->doUnscaleRect = unscaleRect2x;
			port->doScaleUInt32 = scaleUInt322x;
			port->doUnscaleUInt32 = unscaleUInt322x;
			port->doScaleSInt32 = scaleSInt322x;
			port->doUnscaleSInt32 = unscaleSInt322x;
			port->doScaleDouble = scaleDouble2x;
			port->doUnscaleDouble = unscaleDouble2x;
			break;

		case 98304:
			port->scale = scale;
			port->doScaleRect = scaleRect15x;
			port->doUnscaleRect = unscaleRect15x;
			port->doScaleUInt32 = scaleUInt3215x;
			port->doUnscaleUInt32 = unscaleUInt3215x;
			port->doScaleSInt32 = scaleSInt3215x;
			port->doUnscaleSInt32 = unscaleSInt3215x;
			port->doScaleDouble = scaleDouble15x;
			port->doUnscaleDouble = unscaleDouble15x;
			break;

		case 196608:
			port->scale = scale;
			port->doScaleRect = scaleRect3x;
			port->doUnscaleRect = unscaleRect3x;
			port->doScaleUInt32 = scaleUInt323x;
			port->doUnscaleUInt32 = unscaleUInt323x;
			port->doScaleSInt32 = scaleSInt323x;
			port->doUnscaleSInt32 = unscaleSInt323x;
			port->doScaleDouble = scaleDouble3x;
			port->doUnscaleDouble = unscaleDouble3x;
			break;

		case 262144:
			port->scale = scale;
			port->doScaleRect = scaleRect4x;
			port->doUnscaleRect = unscaleRect4x;
			port->doScaleUInt32 = scaleUInt324x;
			port->doUnscaleUInt32 = unscaleUInt324x;
			port->doScaleSInt32 = scaleSInt324x;
			port->doUnscaleSInt32 = unscaleSInt324x;
			port->doScaleDouble = scaleDouble4x;
			port->doUnscaleDouble = unscaleDouble4x;
			break;

		default:
			port->scale = scale;
			port->doScaleRect = scaleRectAnyx;
			port->doUnscaleRect = unscaleRectAnyx;
			port->doScaleUInt32 = scaleUInt32Anyx;
			port->doUnscaleUInt32 = unscaleUInt32Anyx;
			port->doScaleSInt32 = scaleSInt32Anyx;
			port->doUnscaleSInt32 = unscaleSInt32Anyx;
			port->doScaleDouble = scaleDoubleAnyx;
			port->doUnscaleDouble = unscaleDoubleAnyx;
			break;
	}

	FskPortRectUnscale(port, &port->clipRect);
	FskPortRectUnscale(port, &port->updateRect);
	updateAggregateClip(port);
}

FskErr nopSetTextFont(FskPort port, const char *fontName)
{
	if (0 != FskStrListCompare(port->fontName, fontName)) {
		UInt32 fontNameLen = FskStrLen(fontName);

		flushTextFormat(port, true);
		if (port->fontNameCache != port->fontName)
			FskMemPtrDispose(port->fontName);
		if (fontNameLen < (sizeof(port->fontNameCache) - 2)) {
			port->fontName = port->fontNameCache;
			if (NULL != fontName) {
				FskMemMove(port->fontName, fontName, fontNameLen + 1);
				port->fontName[fontNameLen + 1] = 0;						// terminate the list of one
			}
			else {
				port->fontName[0] = 0;
				port->fontName[1] = 0;
			}
		}
		else {
			port->fontName = FskStrDoCat(fontName, " ");
			if (port->fontName)
				port->fontName[FskStrLen(port->fontName) - 1] = 0;		// terminate the list of one
		}
	}
    return kFskErrNone;

}

FskErr nopGetBitmap(FskPort port, FskBitmap *bits)
{
    *bits = port->bits;

    return kFskErrNone;
}

/*
    picSave drawing functions

        by convention, these never change the drawing state in the port. they call out to nop* to do that.
*/

typedef union {
	struct {
		FskRectangleRecord		rect;
	} rectangleFill;
	struct {
		FskRectangleRecord		rect;
	} rectangleFrame;
	struct {
		FskBitmap				bits;
		FskRectangleRecord		srcRect;
		FskRectangleRecord		dstRect;
	} bitmapDraw;
	struct {
		FskBitmap				bits;
		FskRectangleRecord		srcRect;
		FskScaleOffset			scaleOffset;
	} bitmapScaleOffset;
	struct {
		FskBitmap				bits;
		FskRectangleRecord		srcRect;
		double					x;
		double					y;
		double					width;
		double					height;
	} bitmapDrawSubpixel;
	struct {
		FskBitmap				bits;
		FskRectangleRecord		srcRect;
		FskRectangleRecord		dstRect;
        FskFixed                scale;
	} bitmapTile;
	struct {
		UInt32					textLen;
		FskRectangleRecord		bounds;
        char                    *text;
	} textDraw;
	struct {
		FskRectangleRecord		clip;
		Boolean					hasClip;
	} clip;
	struct {
		FskColorRGBARecord		color;
	} penColor;
	struct {
		FskPortTextFormat		textFormat;
	} textFormatApply;
	struct {
		UInt16					hAlign;
		UInt16					vAlign;
	} textAlignment;
	struct {
		char					*name;
	} textFont;
	struct {
		UInt32					size;
	} textSize;
	struct {
		UInt32					style;
	} textStyle;
	struct {
		UInt32					mode;
		FskGraphicsModeParameters	parameters;
	} graphicsMode;
	struct {
		FskFixed					scale;
	} scale;
	struct {
		FskPortPicRenderItem		render;
		FskPortPicPrepareItem		prepare;
		UInt32						paramsSize;
        char                        params[1];
	} external;
	struct {
		SInt32						x;
		SInt32						y;
	} origin;
	struct FskPortPicEffectParametersRecord effect;
	struct {
		FskBitmap               portBits;
        Boolean                 copyPrevious;
	} swapBits;
} FskPortPicParametersRecord;

typedef enum {
	kFskPortOpDone = 0,
	kFskPortOpRectangleFill = 1,
	kFskPortOpRectangleFrame,
	kFskPortOpBitmapDraw,
	kFskPortOpBitmapScaleOffset,
	kFskPortOpBitmapDrawSubpixel,
	kFskPortOpBitmapTile,
	kFskPortOpTextDraw,
	kFskPortOpClip,
	kFskPortOpPenColor,
	kFskPortOpTextFormatApply,
	kFskPortOpTextAlignment,
	kFskPortOpTextSize,
	kFskPortOpTextStyle,
	kFskPortOpGraphicsMode,
	kFskPortOpScale,
	kFskPortOpTextFont,
	kFskPortOpExternal,
	kFskPortOpOrigin,
    kFskPortOpBackgroundColor,
	kFskPortOpEffect,
    kFskPortOpSwapBits
} FskPortOpEnum;

typedef struct FskPortPicItemRecord FskPortPicItemRecord;
typedef struct FskPortPicItemRecord *FskPortPicItem;

struct FskPortPicItemRecord {
	struct FskPortPicItemRecord					*next;

	FskPortOpEnum								operation;
	FskPortPicParametersRecord					parameters;

    // variable size daata may follow here
};

static FskPortPicItem picSaveFlushDeferredState(FskPort port);

FskErr picSaveBeginDrawing(FskPort port, FskConstColorRGBA background)
{
    FskErr err = kFskErrNone;
    FskPortPicItem item;

    nopBeginDrawing(port, background);

    FskInstrumentedItemPrintfVerbose(port, " picsave - begin");

    if (NULL == port->picSave) {
        if (false == port->lowMemoryWarningRegistered) {
            FskNotificationRegister(kFskNotificationLowMemory, FskPortPurge, port);
            port->lowMemoryWarningRegistered = true;
        }

        err = FskMemPtrNew(65536 * 4, &port->picSave);
        BAIL_IF_ERR(err);
    }

    port->picSaveNext = port->picSave;

    // flush current state

    item = port->picSaveNext;

    if (background) {
        item->operation = kFskPortOpBackgroundColor;                // this must be the first op-code, if used
        item->parameters.penColor.color = *background;
        item = item->next = item + 1;
    }

    item->operation = kFskPortOpSwapBits;
    item->parameters.swapBits.portBits = port->bits;
    item->parameters.swapBits.copyPrevious = false;
    port->picSaveLastSwapBits = item;
    port->bits->useCount += 1;
    item = item->next = item + 1;

    item->operation = kFskPortOpOrigin;
    item->parameters.origin.x = port->origin.x;
    item->parameters.origin.y = port->origin.y;
    item = item->next = item + 1;

    item->operation = kFskPortOpTextAlignment;
    item->parameters.textAlignment.hAlign = port->textHAlign;
    item->parameters.textAlignment.vAlign = port->textVAlign;
    item = item->next = item + 1;

    {
        SInt32 len;

        len = FskStrListLen(port->fontName);

        item->operation = kFskPortOpTextSize;
        item->parameters.textSize.size = port->textSize;
        item = item->next = item + 1;

        item->operation = kFskPortOpTextFont;
        item->parameters.textFont.name = (char *)(item + 1);
        FskMemMove(item->parameters.textFont.name, port->fontName, len);
        item = item->next = (FskPortPicItem)(len + (4 - (len % 4)) + item->parameters.textFont.name);

        item->operation = kFskPortOpTextStyle;
        item->parameters.textStyle.style = port->textStyle;
        item = item->next = item + 1;
    }

    item->operation = kFskPortOpPenColor;
    item->parameters.penColor.color = port->penColor;
    item = item->next = item + 1;

    item->operation = kFskPortOpClip;
    item->parameters.clip.clip = port->clipRect;
    item->parameters.clip.hasClip = true;
    item = item->next = item + 1;

    item->operation = kFskPortOpGraphicsMode;
    item->parameters.graphicsMode.mode = port->graphicsMode;
    item->parameters.graphicsMode.parameters = NULL;
    item = item->next = item + 1;

    item->operation = kFskPortOpScale;
    item->parameters.scale.scale = port->scale;
    item = item->next = item + 1;

    port->picSaveNext = item;

    port->deferredOrigin = false;
    port->deferredPenColor = false;
    port->deferredTextStyle = false;

bail:
    return err;
}

FskErr picSaveEndDrawing(FskPort port)
{
    FskPortPicItem picSave, item, runStart;
    FskBitmap copyPrevious = NULL;
    FskPortPicItemRecord disposeListHead = {0};
    FskPortPicItem dispose = &disposeListHead;

    updateAggregateClip(port);

    ((FskPortPicItem)port->picSaveNext)->operation = kFskPortOpDone;
    picSave = runStart = port->picSave;
    port->picSave = port->picSaveNext = NULL;

    dispose->operation = kFskPortOpDone;
    dispose->next = NULL;

    FskInstrumentedItemPrintfVerbose(port, " picsave - accelerate bitmaps");

    nopEndDrawing(port);

    port->vector = gRenderPort;
    (port->vector.doBeginDrawing)(port, NULL);

    while (runStart) {
#if FSKBITMAP_OPENGL
        Boolean flushTextGlyphs = false;

        item = runStart;
        do {
            switch (item->operation) {
                case kFskPortOpBitmapDraw:
                    FskBitmapCheckGLSourceAccelerated(item->parameters.bitmapDraw.bits);
                    item = item->next;
                    break;
                case kFskPortOpBitmapScaleOffset:
                    FskBitmapCheckGLSourceAccelerated(item->parameters.bitmapScaleOffset.bits);
                    item = item->next;
                    break;
                case kFskPortOpBitmapDrawSubpixel:
                    FskBitmapCheckGLSourceAccelerated(item->parameters.bitmapDrawSubpixel.bits);
                    item = item->next;
                    break;
                case kFskPortOpBitmapTile:
                    FskBitmapCheckGLSourceAccelerated(item->parameters.bitmapTile.bits);
                    item = item->next;
                    break;
                case kFskPortOpTextDraw:
                    if (NULL == port->textFormatCache) {
                        if (kFskErrNone == FskTextFormatCacheNew(port->textEngine, &port->textFormatCache, port->bits, FskPortUInt32Scale(port, port->textSize), port->textStyle, port->fontName))
                            port->textFromFormat = false;
                    }
                    FskGLTextGlyphsLoad(port->textEngine, item->parameters.textDraw.text, item->parameters.textDraw.textLen,
                                            FskPortUInt32Scale(port, port->textSize), port->textStyle, port->fontName,
                                            port->textFormatCache);
                    flushTextGlyphs = true;
                    item = item->next;
                    break;
                case kFskPortOpTextFont:
                    (port->vector.doSetTextFont)(port, (const char*)item->parameters.textFont.name);
                    item = item->next;
                    break;
                case kFskPortOpTextSize:
                    (port->vector.doSetTextSize)(port, item->parameters.textSize.size);
                    item = item->next;
                    break;
                case kFskPortOpTextStyle:
                    (port->vector.doSetTextStyle)(port, item->parameters.textStyle.style);
                    item = item->next;
                    break;
                case kFskPortOpTextFormatApply:
                    (port->vector.doTextFormatApply)(port, item->parameters.textFormatApply.textFormat);
                    item = item->next;
                    break;
                case kFskPortOpDone:
                    item = NULL;
                    break;
                case kFskPortOpExternal:
                    // the implementation of FskPortPicSaveAdd assumes it will NOT be called here - must call external.prepare directly
                    (item->parameters.external.prepare)(port, item->parameters.external.params, item->parameters.external.paramsSize);
                    item = item->next;
                    break;
                case kFskPortOpEffect:
                    CheckGLEffectSourcesAreAccelerated(item->parameters.effect.src, &item->parameters.effect.effect);
                    item = item->next;
                    break;
                case kFskPortOpSwapBits:
                    if (port->bits != item->parameters.swapBits.portBits)
                        item = NULL;
                    else
                        item = item->next;
                    break;
                default:
                    item = item->next;
                    break;
            }
        } while (item);

        if (flushTextGlyphs)
            FskGLTextGlyphsFlush();

        FskGLRenderToBitmapTexture(port->bits, (kFskPortOpBackgroundColor == runStart->operation) ? &runStart->parameters.penColor.color : NULL);
#endif

        FskInstrumentedItemPrintfNormal(port, " picsave - play drawing operations");

        item = runStart;
        do {
            if (copyPrevious) {
                FskBitmapDraw(copyPrevious, NULL, port->bits, &port->bits->bounds, NULL, &port->penColor, kFskGraphicsModeCopy, NULL);
                copyPrevious = NULL;
            }

            switch (item->operation) {
                case kFskPortOpDone:
                    port->picSave = picSave;        // reuse next time
                    runStart = item = NULL;
                    break;

                case kFskPortOpRectangleFill:
                    (port->vector.doRectangleFill)(port, &item->parameters.rectangleFill.rect);
                    item = item->next;
                    break;

                case kFskPortOpRectangleFrame:
                    FskPortRectangleFrame(port, &item->parameters.rectangleFrame.rect);
                    item = item->next;
                    break;

                case kFskPortOpBitmapDraw:
                    (port->vector.doBitmapDraw)(port, item->parameters.bitmapDraw.bits, &item->parameters.bitmapDraw.srcRect, &item->parameters.bitmapDraw.dstRect);
                    if (item->parameters.bitmapDraw.bits->useCount-- <= 0)
                        dispose->next = item, dispose = item;
                    item = item->next;
                    break;

                case kFskPortOpBitmapScaleOffset:
                    (port->vector.doBitmapScaleOffset)(port, item->parameters.bitmapScaleOffset.bits, &item->parameters.bitmapScaleOffset.srcRect, &item->parameters.bitmapScaleOffset.scaleOffset);
                    if (item->parameters.bitmapScaleOffset.bits->useCount-- <= 0)
                        dispose->next = item, dispose = item;
                    item = item->next;
                    break;

                case kFskPortOpBitmapDrawSubpixel:
                    (port->vector.doBitmapDrawSubpixel)(port, item->parameters.bitmapDrawSubpixel.bits, &item->parameters.bitmapDrawSubpixel.srcRect, item->parameters.bitmapDrawSubpixel.x, item->parameters.bitmapDrawSubpixel.y, item->parameters.bitmapDrawSubpixel.width, item->parameters.bitmapDrawSubpixel.height);
                    if (item->parameters.bitmapDrawSubpixel.bits->useCount-- <= 0)
                        dispose->next = item, dispose = item;
                    item = item->next;
                    break;

                case kFskPortOpBitmapTile:
                    (port->vector.doBitmapTile)(port, item->parameters.bitmapTile.bits, &item->parameters.bitmapTile.srcRect, &item->parameters.bitmapTile.dstRect, item->parameters.bitmapTile.scale);
                    if (item->parameters.bitmapTile.bits->useCount-- <= 0)
                        dispose->next = item, dispose = item;
                    item = item->next;
                    break;

                case kFskPortOpTextDraw:
                    (port->vector.doTextDraw)(port, item->parameters.textDraw.text, item->parameters.textDraw.textLen, &item->parameters.textDraw.bounds);
                    item = item->next;
                    break;

                case kFskPortOpClip:
                    if (item->parameters.clip.hasClip) {
                        port->clipRect = item->parameters.clip.clip;
                        updateAggregateClip(port);
                    }
                    else
                        (port->vector.doSetClipRectangle)(port, NULL);
                    item = item->next;
                    break;

                case kFskPortOpPenColor:
                    port->penColor = item->parameters.penColor.color;
                    item = item->next;
                    break;

                case kFskPortOpTextFormatApply:
                    (port->vector.doTextFormatApply)(port, item->parameters.textFormatApply.textFormat);
                    item->parameters.textFormatApply.textFormat->useCount -= 1;
                    if (item->parameters.textFormatApply.textFormat->useCount <= 0)
                        FskPortTextFormatDispose(item->parameters.textFormatApply.textFormat);
                    item = item->next;
                    break;

                case kFskPortOpTextAlignment:
                    port->textHAlign = item->parameters.textAlignment.hAlign;
                    port->textVAlign = item->parameters.textAlignment.vAlign;
                    item = item->next;
                    break;

                case kFskPortOpTextSize:
                    (port->vector.doSetTextSize)(port, item->parameters.textSize.size);
                    item = item->next;
                    break;

                case kFskPortOpTextStyle:
                    (port->vector.doSetTextStyle)(port, item->parameters.textStyle.style);
                    item = item->next;
                    break;

                case kFskPortOpGraphicsMode:
                    if ((NULL == item->parameters.graphicsMode.parameters) && (NULL == port->graphicsModeParameters))
                        port->graphicsMode = item->parameters.graphicsMode.mode;
                    else
                        (port->vector.doSetGraphicsMode)(port, item->parameters.graphicsMode.mode, item->parameters.graphicsMode.parameters);
                    item = item->next;
                    break;

                case kFskPortOpScale:
                    (port->vector.doScaleSet)(port, item->parameters.scale.scale);
                    item = item->next;
                    break;

                case kFskPortOpTextFont:
                    (port->vector.doSetTextFont)(port, item->parameters.textFont.name);
                    item = item->next;
                    break;

                case kFskPortOpExternal:
                    // the implementation of renderPicSaveAdd assumes it will NOT be called here - must call external.render directly
                    (item->parameters.external.render)(port, item->parameters.external.params, item->parameters.external.paramsSize);
                    item = item->next;
                    break;

                case kFskPortOpOrigin:
                    port->origin.x = item->parameters.origin.x;
                    port->origin.y = item->parameters.origin.y;
                    item = item->next;
                    break;

                case kFskPortOpBackgroundColor:
                    item = item->next;
                    break;

                case kFskPortOpEffect:
                    {	FskRectangleRecord dstRect;	/* SW case. GL case goes through another path */
                        FskBitmap tmp;
                        FskRectangleSet(&dstRect, item->parameters.effect.dstPoint.x, item->parameters.effect.dstPoint.y, item->parameters.effect.srcRect.width, item->parameters.effect.srcRect.height);
                        if (kFskErrNone == FskBitmapNew(item->parameters.effect.srcRect.width, item->parameters.effect.srcRect.height, kFskBitmapFormatDefaultRGBA, &tmp)) {
                            tmp->hasAlpha = true;
                            tmp->alphaIsPremultiplied = item->parameters.effect.src->alphaIsPremultiplied;
                            (void)FskEffectApply(&item->parameters.effect.effect, item->parameters.effect.src, &item->parameters.effect.srcRect, tmp, NULL);
                            (void)FskBitmapDraw(tmp, &tmp->bounds, port->bits, &dstRect, &port->clipRect, &port->penColor, port->graphicsMode, port->graphicsModeParameters);
                            FskBitmapDispose(tmp);
                        }
                    }
                    if (--(item->parameters.effect.src->useCount) < 0)
                        FskBitmapDispose(item->parameters.effect.src);
                    item = item->next;
                    break;

                case kFskPortOpSwapBits:
                    if (port->bits != item->parameters.swapBits.portBits) {
                        if (item->parameters.swapBits.copyPrevious)
                            copyPrevious = port->bits;
                        else
                            copyPrevious = NULL;
                        port->bits = item->parameters.swapBits.portBits;
                        runStart = item->next;
                        dispose->next = item, dispose = item;
                        item = NULL;
                    }
                    else {
                        dispose->next = item, dispose = item;
                        item = item->next;
                    }
                    break;

                default:
                    FskExit(kFskErrBadData);
                    break;
            }
        } while (item);
    }
    FskInstrumentedItemPrintfVerbose(port, " picsave - played drawing commands");

    dispose->next = NULL;
    for (dispose = &disposeListHead; NULL != dispose; dispose = dispose->next) {
        switch (dispose->operation) {
            case kFskPortOpBitmapDraw:
                FskBitmapDispose(dispose->parameters.bitmapDraw.bits);
                break;

            case kFskPortOpBitmapScaleOffset:
                FskBitmapDispose(dispose->parameters.bitmapScaleOffset.bits);
                break;

            case kFskPortOpBitmapDrawSubpixel:
                FskBitmapDispose(dispose->parameters.bitmapDrawSubpixel.bits);
                break;

            case kFskPortOpBitmapTile:
                FskBitmapDispose(dispose->parameters.bitmapTile.bits);
                break;

            case kFskPortOpEffect:
                FskBitmapDispose(dispose->parameters.effect.src);
                break;

            case kFskPortOpSwapBits:
                FskBitmapDispose(dispose->parameters.swapBits.portBits);
                break;

            case kFskPortOpDone:
                break;

            default:
                FskExit(kFskErrBadData);
                break;
        }
    }

    return (port->vector.doEndDrawing)(port);
}

void picSaveRectangleFill(FskPort port, FskConstRectangle rect)
{
    FskPortPicItem item = picSaveFlushDeferredState(port);

    item->operation = kFskPortOpRectangleFill;
    item->parameters.rectangleFill.rect = *rect;

    port->picSaveNext = item->next = item + 1;
}

void picSaveRectangleFrame(FskPort port, FskConstRectangle rect)
{
    FskPortPicItem item = picSaveFlushDeferredState(port);

    item->operation = kFskPortOpRectangleFrame;
    item->parameters.rectangleFrame.rect = *rect;

    port->picSaveNext = item->next = item + 1;
}

void picSaveBitmapDraw(FskPort port, FskBitmap bits, FskConstRectangle srcRect, FskConstRectangle dstRect)
{
    FskPortPicItem item = picSaveFlushDeferredState(port);

    item->operation = kFskPortOpBitmapDraw;
    item->parameters.bitmapDraw.bits = bits;
    item->parameters.bitmapDraw.srcRect = srcRect ? *srcRect : bits->bounds;
    item->parameters.bitmapDraw.dstRect = *dstRect;

    bits->useCount += 1;

    port->picSaveNext = item->next = item + 1;
}

void picSaveBitmapScaleOffset(FskPort port, FskBitmap bits, FskConstRectangle srcRect, const struct FskScaleOffset *scaleOffset)
{
    FskPortPicItem item = picSaveFlushDeferredState(port);

    item->operation                                = kFskPortOpBitmapScaleOffset;
    item->parameters.bitmapScaleOffset.bits        = bits;
    item->parameters.bitmapScaleOffset.srcRect     = srcRect ? *srcRect : bits->bounds;
    item->parameters.bitmapScaleOffset.scaleOffset = *scaleOffset;

    bits->useCount += 1;

    port->picSaveNext = item->next = item + 1;
}

void picSaveBitmapDrawSubpixel(FskPort port, FskBitmap bits, FskConstRectangle srcRect, double x, double y, double width, double height)
{
    FskPortPicItem item = picSaveFlushDeferredState(port);

    item->operation = kFskPortOpBitmapDrawSubpixel;
    item->parameters.bitmapDrawSubpixel.bits = bits;
    item->parameters.bitmapDrawSubpixel.srcRect = srcRect ? *srcRect : bits->bounds;
    item->parameters.bitmapDrawSubpixel.x = x;
    item->parameters.bitmapDrawSubpixel.y = y;
    item->parameters.bitmapDrawSubpixel.width = width;
    item->parameters.bitmapDrawSubpixel.height = height;

    bits->useCount += 1;

    port->picSaveNext = item->next = item + 1;
}

void picSaveBitmapTile(FskPort port, FskBitmap srcBits, FskConstRectangle srcRect, FskConstRectangle dstRect, FskFixed scale)
{
    FskPortPicItem item = picSaveFlushDeferredState(port);

    item->operation = kFskPortOpBitmapTile;
    item->parameters.bitmapTile.bits = srcBits;
    item->parameters.bitmapTile.srcRect = srcRect ? *srcRect : srcBits->bounds;
    item->parameters.bitmapTile.dstRect = *dstRect;
    item->parameters.bitmapTile.scale = scale;

    srcBits->useCount += 1;

    port->picSaveNext = item->next = item + 1;
}

void picSaveTextDraw(FskPort port, const char *text, UInt32 textLen, FskConstRectangle bounds)
{
    FskPortPicItem item = picSaveFlushDeferredState(port);

    item->operation = kFskPortOpTextDraw;
    item->parameters.textDraw.textLen = textLen;
    item->parameters.textDraw.bounds = *bounds;
    item->parameters.textDraw.text = (char *)(item + 1);
    FskMemMove(item->parameters.textDraw.text, text, textLen);

    port->picSaveNext = item->next = (FskPortPicItem)(textLen + (4 - (textLen % 4)) + (char *)(item + 1));
}

FskErr picSaveSetOrigin(FskPort port, SInt32 x, SInt32 y)
{
    if (x == port->origin.x && y == port->origin.y)
        return kFskErrNone;

    port->deferredOrigin = true;

    return nopSetOrigin(port, x, y);
}

FskErr picSaveSetClipRectangle(FskPort port, FskConstRectangle clip)
{
    FskRectangleRecord saveClip = port->clipRect;
    FskPortPicItem item;

    nopSetClipRectangle(port, clip);
    if (FskRectangleIsEqual(&saveClip, &port->clipRect))
        return kFskErrNone;

    item = picSaveFlushDeferredState(port);
    item->operation = kFskPortOpClip;
    if (NULL != clip) {
        item->parameters.clip.hasClip = true;
        item->parameters.clip.clip = port->clipRect;
    }
    else
        item->parameters.clip.hasClip = false;

    port->picSaveNext = item->next = item + 1;

    return kFskErrNone;
}

FskErr picSaveSetPenColor(FskPort port, FskConstColorRGBA color)
{
    if ((color->a == port->penColor.a) && (color->r == port->penColor.r) && (color->g == port->penColor.g) && (color->b == port->penColor.b))
        return kFskErrNone;

    port->deferredPenColor = true;

    return nopSetPenColor(port, color);
}

FskErr picSaveTextFormatApply(FskPort port, FskPortTextFormat textFormat)
{
    FskPortPicItem item;

	if (port->textFormatCache && (port->textFormatCache == textFormat->textFormatCache)) {
		// the part of the style outside kStyleMask may still be different, so need to reassert that
		if (port->textStyle != textFormat->textStyle) {
			item = port->picSaveNext;

			item->operation = kFskPortOpTextStyle;
			item->parameters.textStyle.style = textFormat->textStyle;

			port->picSaveNext = item->next = item + 1;
		}
	}
    else {
        item = port->picSaveNext;

        item->operation = kFskPortOpTextFormatApply;
        item->parameters.textFormatApply.textFormat = textFormat;

        port->picSaveNext = item->next = item + 1;

        textFormat->useCount += 1;
    }

    return nopTextFormatApply(port, textFormat);
}

FskErr picSaveSetTextAlignment(FskPort port, UInt16 hAlign, UInt16 vAlign)
{
    FskErr err = kFskErrNone;

	if ((port->textHAlign != hAlign) || (port->textVAlign != vAlign)) {
        FskPortPicItem item = port->picSaveNext;

        BAIL_IF_ERR(nopSetTextAlignment(port, hAlign, vAlign));

        item->operation = kFskPortOpTextAlignment;
        item->parameters.textAlignment.hAlign = hAlign;
        item->parameters.textAlignment.vAlign = vAlign;

        port->picSaveNext = item->next = item + 1;
	}

bail:
    return err;
}

FskErr picSaveSetTextSize(FskPort port, UInt32 size)
{
    FskErr err = kFskErrNone;
    FskPortPicItem item;

	if (size != port->textSize) {
        BAIL_IF_ERR(nopSetTextSize(port, size));

        item = port->picSaveNext;
        item->operation = kFskPortOpTextSize;
        item->parameters.textSize.size = size;

        port->picSaveNext = item->next = item + 1;
	}

bail:
    return err;
}

FskErr picSaveSetTextStyle(FskPort port, UInt32 style)
{
	if (port->textStyle != style)
        port->deferredTextStyle = true;

    return nopSetTextStyle(port, style);
}

FskErr picSaveSetGraphicsMode(FskPort port, UInt32 graphicsMode, FskConstGraphicsModeParameters graphicsModeParameters)
{
    FskErr err;
    FskPortPicItem item;

    err = nopSetGraphicsMode(port, graphicsMode, graphicsModeParameters);
    BAIL_IF_ERR(err);

    item = port->picSaveNext;
    item->operation = kFskPortOpGraphicsMode;
    item->parameters.graphicsMode.mode = graphicsMode;

    if (graphicsModeParameters) {
        UInt32 dataSize = graphicsModeParameters->dataSize;
        item->parameters.graphicsMode.parameters = (FskGraphicsModeParameters)(item + 1);
        FskMemMove(item->parameters.graphicsMode.parameters, graphicsModeParameters, dataSize);
        port->picSaveNext = item->next = (FskPortPicItem)(dataSize + (4 - (dataSize % 4)) + (char *)(item->parameters.graphicsMode.parameters));
    }
    else {
        item->parameters.graphicsMode.parameters = NULL;
        port->picSaveNext = item->next = item + 1;
    }

bail:
    return err;
}

void picSaveScaleSet(FskPort port, FskFixed scale)
{
    FskPortPicItem item;

	if (scale == port->scale)
		return;

    nopScaleSet(port, scale);

    item = port->picSaveNext;
    item->operation = kFskPortOpScale;
    item->parameters.scale.scale = scale;

    port->picSaveNext = item->next = item + 1;
}

FskErr picSaveSetTextFont(FskPort port, const char *fontName)
{
    FskErr err = kFskErrNone;

	if (0 != FskStrListCompare(port->fontName, fontName)) {
        FskPortPicItem item;
        UInt32 len;

        BAIL_IF_ERR(nopSetTextFont(port, fontName));

        item = port->picSaveNext;
        item->operation = kFskPortOpTextFont;

        item->parameters.textFont.name = (char *)(item + 1);
        if (port->fontName && *port->fontName) {
            len = FskStrListLen(port->fontName);
            FskMemMove(item->parameters.textFont.name, port->fontName, len);
        }
        else {
            len = 2;
            item->parameters.textFont.name[0] = item->parameters.textFont.name[1] = 0;
        }

        port->picSaveNext = item->next = (FskPortPicItem)(len + (4 - (len % 4)) + item->parameters.textFont.name);
    }

bail:
    return err;
}

FskErr picSavePicSaveAdd(FskPort port, FskPortPicRenderItem render, FskPortPicPrepareItem prepare, void *params, UInt32 paramsSize)
{
    FskPortPicItem item = picSaveFlushDeferredState(port);

    item->operation = kFskPortOpExternal;
    item->parameters.external.render = render;
    item->parameters.external.prepare = prepare;
    item->parameters.external.paramsSize = paramsSize;
    FskMemMove(item->parameters.external.params, params, paramsSize);

    port->picSaveNext = item->next = (FskPortPicItem)(paramsSize + (4 - (paramsSize % 4)) + item->parameters.external.params);

    return kFskErrNone;
}

void picSaveEffectApply(FskPort port, FskBitmap src, FskConstRectangle srcRect, FskConstPoint dstPoint, const struct FskEffectRecord* effect)
{
	FskBitmap			dst		= port->bits;

	if (!srcRect)	srcRect  = &src->bounds;
	if (!dstPoint)	dstPoint = /*(FskConstPoint)*/(const void*)(&dst->bounds);

    #if FSKBITMAP_OPENGL && GLES_VERSION == 2 /* OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL -- OPENGL */
        if (FskBitmapIsOpenGLDestinationAccelerated(dst)) {											/* If the destination is accelerated, ... */
            if (kFskErrNone == FskPortEffectQueue(port, src, srcRect, dstPoint, effect))		/* Special prepare/render split */
                return;
            /* Fall through if we do not have a special prepare/render split */
            {	FskPortPicItem	item		= picSaveFlushDeferredState(port);
                unsigned		overSize	= (kFskEffectCompound == effect->effectID) ? sizeof(FskEffectRecord) * effect->params.compound.numStages : 0;
                struct FskPortPicEffectParametersRecord *pEffect	= (void*)(item->parameters.external.params);		/* We need an arbitrary size to accommodate compound effects */
                item->operation						= kFskPortOpExternal;
                item->parameters.external.render	= &EffectTmpCompositeRender;
                item->parameters.external.prepare	= &EffectPostCompositePrepare;
                item->parameters.external.paramsSize= sizeof(struct FskPortPicEffectParametersRecord) + overSize;
                pEffect->src		= src;
                pEffect->srcRect	= *srcRect;
                pEffect->dstPoint	= *dstPoint;
                FskMemCopy(&pEffect->effect, effect, sizeof(FskEffectRecord) + overSize);
                FskPortEffectScale(port, pEffect);
                port->picSaveNext = item->next = (FskPortPicItem)((char*)item->parameters.external.params + item->parameters.external.paramsSize);
                src->useCount += 1;
                return;
            }
        }																						/* End: destination is accelerated */
        /* Accelerated destinations which are neither composited nor have a special prepare/render split fall through, because they only have one stage */
        FskPortEffectCheckSourcesAreAccelerated(src, effect);
    #endif

    {	FskPortPicItem	item		= picSaveFlushDeferredState(port);
        unsigned		overSize	= (kFskEffectCompound == effect->effectID) ? sizeof(FskEffectRecord) * effect->params.compound.numStages : 0;
        item->operation                  = kFskPortOpEffect;
        item->parameters.effect.src      = src;
        item->parameters.effect.srcRect  = *srcRect;
        item->parameters.effect.dstPoint = *dstPoint;
        FskMemCopy(&item->parameters.effect.effect, effect, sizeof(FskEffectRecord) + overSize);
        FskPortEffectScale(port, &item->parameters.effect);
        src->useCount += 1;
        port->picSaveNext = item->next = (FskPortPicItem)((char*)(item + 1) + overSize);
    }
}

FskErr picSaveGetBitmap(FskPort port, FskBitmap *bits)
{
    FskErr err;
    FskPortPicItem item = port->picSaveNext;

    err = FskBitmapNew(port->bits->bounds.width, port->bits->bounds.height, kFskBitmapFormatGLRGB, bits);
    if (err) goto bail;

    ((FskPortPicItem)(port->picSaveLastSwapBits))->parameters.swapBits.portBits = *bits;

    item->operation = kFskPortOpSwapBits;
    item->parameters.swapBits.portBits = port->bits;
    item->parameters.swapBits.copyPrevious = true;
    port->picSaveNext = item->next = item + 1;
    port->picSaveLastSwapBits = item;

bail:
    return err;
}

FskPortPicItem picSaveFlushDeferredState(FskPort port)
{
    if (port->deferredOrigin) {
        FskPortPicItem item = port->picSaveNext;

        item->operation = kFskPortOpOrigin;
        item->parameters.origin.x = port->origin.x;
        item->parameters.origin.y = port->origin.y;

        port->picSaveNext = item->next = item + 1;

        port->deferredOrigin = false;
    }

    if (port->deferredPenColor) {
		FskPortPicItem item = port->picSaveNext;

		item->operation = kFskPortOpPenColor;
		item->parameters.penColor.color = port->penColor;

		port->picSaveNext = item->next = item + 1;

        port->deferredPenColor = false;
    }

    if (port->deferredTextStyle) {
        FskPortPicItem item = port->picSaveNext;

        item->operation = kFskPortOpTextStyle;
        item->parameters.textStyle.style = port->textStyle;

        port->picSaveNext = item->next = item + 1;

        port->deferredTextStyle = false;
    }

    return port->picSaveNext;
}


/*
    Rendering functions
*/

FskErr renderBeginDrawing(FskPort port, FskConstColorRGBA background)
{
    FskErr err = kFskErrNone;

    err = nopBeginDrawing(port, background);
    BAIL_IF_ERR(err);

#if FSKBITMAP_OPENGL
    if (FskBitmapIsOpenGLDestinationAccelerated(port->bits))
        FskGLPortFocus(port->bits->glPort);
#endif

    err = FskBitmapWriteBegin(port->bits, NULL, NULL, NULL);
    BAIL_IF_ERR(err);

    if (NULL != port->window) {
        FskRectangleRecord r;
        FskRectangleIntersect(&port->bits->bounds, &port->updateRect, &r);
        FskWindowBeginDrawing(port->window, &r);
    }

    if (background)
        FskRectangleFill(port->bits, &port->bits->bounds, background, kFskGraphicsModeCopy, NULL);

bail:
    return err;
}

FskErr renderEndDrawing(FskPort port)
{
    FskBitmap bits = port->bits;
#if FSKBITMAP_OPENGL
    if (FskBitmapIsOpenGLDestinationAccelerated(bits)) {
    	(void)FskGLEffectCacheCountDown();			/* If the effects cache has not been used for a while, dispose of it, to free up resources. */
        if (port->window)
            FskGLPortDefocus(bits->glPort);
    }
#endif

    if (NULL != port->window) {
        FskRectangleRecord r = port->changedArea;
        FskPortRectScale(port, &r);
        FskRectangleIntersect(&bits->bounds, &r, &r);
        FskWindowEndDrawing(port->window, &r);
    }

    FskBitmapWriteEnd(bits);

    nopEndDrawing(port);

    return kFskErrNone;
}

void renderRectangleFill(FskPort port, FskConstRectangle rect)
{
	FskRectangleRecord fillRect = *rect;

	FskRectangleOffset(&fillRect, port->origin.x, port->origin.y);
	if (FskRectangleIntersect(&fillRect, &port->aggregateClip, &fillRect)) {
		(void)FskPortAccumulateChange(port, &fillRect);
		FskPortRectScale(port, &fillRect);
		FskRectangleFill(port->bits, &fillRect, &port->penColor, port->graphicsMode, port->graphicsModeParameters);
	}
}

void renderRectangleFrame(FskPort port, FskConstRectangle rectIn)
{
	FskRectangleRecord r;
	FskFixed scale = FskPortScaleGet(port);
	Boolean unscaled = false;
	FskRectangleRecord rect = *rectIn;

	if (FskIntToFixed(1) != scale) {
		FskPortRectScale(port, &rect);
		FskPortScaleSet(port, FskIntToFixed(1));
		unscaled = true;
	}

	if ((rectIn->height > 1) && (rectIn->width > 1)) {
		FskRectangleSet(&r, rect.x, rect.y, 1, rect.height);
		FskPortRectangleFill(port, &r);

		FskRectangleSet(&r, rect.x + rect.width - 1, rect.y, 1, rect.height);
		FskPortRectangleFill(port, &r);

		FskRectangleSet(&r, rect.x + 1, rect.y, rect.width - 2, 1);
		FskPortRectangleFill(port, &r);

		FskRectangleSet(&r, rect.x + 1, rect.y + rect.height - 1, rect.width - 2, 1);
		FskPortRectangleFill(port, &r);
	}
	else if (rectIn->width > 1) {
		FskRectangleSet(&r, rect.x, rect.y, rect.width, 1);
		FskPortRectangleFill(port, &r);
	}
	else {
		FskRectangleSet(&r, rect.x, rect.y, 1, rect.height);
		FskPortRectangleFill(port, &r);
	}

	if (unscaled)
		FskPortScaleSet(port, scale);
}

void renderBitmapDraw(FskPort port, FskBitmap bits, FskConstRectangle srcRect, FskConstRectangle dstRect)
{
	FskRectangleRecord r = *dstRect;

	if (!srcRect)
		srcRect = &bits->bounds;

	FskRectangleOffset(&r, port->origin.x, port->origin.y);
	if (FskPortAccumulateChange(port, &r)) {
		FskPortRectScale(port, &r);
		FskBitmapDraw(bits, srcRect, port->bits, &r, &port->aggregateClipScaled, &port->penColor, port->graphicsMode, port->graphicsModeParameters);
	}
}

void renderBitmapScaleOffset(FskPort port, FskBitmap bits, FskConstRectangle srcRect, const struct FskScaleOffset *scaleOffset)
{
	FskRectangleRecord dstRect;
	FskFixed x0, y0, x1, y1, t;

	if (!srcRect)
		srcRect = &bits->bounds;

	/* Compute the destination rect */
	FskRectangleSet(&dstRect, 0, 0, srcRect->width, srcRect->height);	/* Relative srcRect */

	x0 = scaleOffset->offsetX + (dstRect.x << kFskOffsetBits);                                          /* Offset left */
	y0 = scaleOffset->offsetY + (dstRect.y << kFskOffsetBits);                                          /* Offset top  */
	x1 = FskFixedNMul(scaleOffset->scaleX, (dstRect.width  - 1), kFskScaleBits-kFskOffsetBits) + x0;	/* Scale and offset right  */
	y1 = FskFixedNMul(scaleOffset->scaleY, (dstRect.height - 1), kFskScaleBits-kFskOffsetBits) + y0;	/* Scale and offset bottom */
	if (x0 > x1) { t = x0; x0 = x1; x1 = t; }                                                           /* Reorder so that x0 is left and x1 is right  */
	if (y0 > y1) { t = y0; y0 = y1; y1 = t; }                                                           /* Reorder so that y0 is top  and y1 is bottom */
	x0 += (1 << (kFskOffsetBits-1)); x0 >>= kFskOffsetBits;                                             /* round(x0) */
	y0 += (1 << (kFskOffsetBits-1)); y0 >>= kFskOffsetBits;                                             /* round(y0) */
	x1 += (1 << (kFskOffsetBits-1)); x1 >>= kFskOffsetBits;                                             /* round(x1) */
	y1 += (1 << (kFskOffsetBits-1)); y1 >>= kFskOffsetBits;                                             /* round(y1) */
	FskRectangleSet(&dstRect, x0, y0, x1 - x0 + 1, y1 - y0 + 1);

	FskRectangleOffset(&dstRect, port->origin.x, port->origin.y);		/* Apply port offset */

	if (FskPortAccumulateChange(port, &dstRect)) {
		FskScaleOffset so;

        so.scaleX = FskPortSInt32Scale(port, scaleOffset->scaleX);
        so.scaleY = FskPortSInt32Scale(port, scaleOffset->scaleY);
        so.offsetX = FskPortSInt32Scale(port, scaleOffset->offsetX);
        so.offsetY = FskPortSInt32Scale(port, scaleOffset->offsetY);

		FskScaleOffsetBitmap(bits, srcRect, port->bits, &port->aggregateClipScaled, &so, &port->penColor, port->graphicsMode, port->graphicsModeParameters);
	}
}

#define MAX_SCALE (+2147483647. / (1 << kFskScaleBits))		/* 127.99999994039536 */
#define MIN_SCALE (-2147483647. / (1 << kFskScaleBits))		/* Note: -2147483648 crashes */
#define RoundFloatScaleToFixed(x)	((FskFixed24)((x) * (1 << kFskScaleBits)  + (((x) >= 0) ? 0.5 : -0.5)))	/**< Round a float to a FskFixed24. */
#define RoundFloatOffsetToFixed(x)	((FskFixed)((x)   * (1 << kFskOffsetBits) + (((x) >= 0) ? 0.5 : -0.5)))	/**< Round a float to a FskFixed. */

void renderBitmapDrawSubpixel(FskPort port, FskBitmap bits, FskConstRectangle srcRect, double x, double y, double width, double height)
{
	FskScaleOffset scaleOffset;
	FskConstRectangle sR = srcRect ? srcRect : &bits->bounds;
	FskRectangleRecord r;
	double scaleX, scaleY, offsetX, offsetY;

	if ((sR->width <= 1) || (sR->height <= 1) || fabs(width) <= 1 || fabs(height) <= 1)
		return;

	x += port->origin.x;
	y += port->origin.y;

	FskRectangleSet(&r, (SInt32)(x - 0.5), (SInt32)(y - 0.5), (SInt32)(width + 1), (SInt32)(height + 1));
	if (!FskPortAccumulateChange(port, &r))
        return;

	x = FskPortDoubleScale(port, x);
	y = FskPortDoubleScale(port, y);
	width = FskPortDoubleScale(port, width);
	height = FskPortDoubleScale(port, height);

	scaleX = ((width < 0) ? (width + 1) : (width - 1)) / (sR->width  - 1);
	if (fabs(scaleX) < MAX_SCALE) {													/* The X scale is representable in fixed point  */
 		scaleOffset.scaleX = RoundFloatScaleToFixed(scaleX);
	}
	else {																			/* The X scale is not representable in fixed point */
		if (!(scaleX == scaleX))													/* NaN fails all comparisons -- we can't compute with NaN - abort  */
			return;
		if (scaleX < 0)	{ scaleX = MIN_SCALE;	scaleOffset.scaleX = 0x80000001; }	/* If the X scale is too small, saturate to the smallest representable number */
		else			{ scaleX = MAX_SCALE;	scaleOffset.scaleX = 0x7FFFFFFF; }	/* If the X scale is too large, saturate to the largest  representable number */
	}

	scaleY = ((height < 0) ? (height + 1) : (height - 1)) / (sR->height  - 1);
	if (fabs(scaleY) < MAX_SCALE) {													/* The Y scale is representable in fixed point  */
 		scaleOffset.scaleY = RoundFloatScaleToFixed(scaleY);
	}
	else {																			/* The Y scale is not representable in fixed point */
		if (!(scaleY == scaleY))													/* NaN fails all comparisons -- we can't compute with NaN - abort  */
			return;
		if (scaleY < 0)	{ scaleY = MIN_SCALE;	scaleOffset.scaleY = 0x80000001; }	/* If the Y scale is too small, saturate to the smallest representable number */
		else			{ scaleY = MAX_SCALE;	scaleOffset.scaleY = 0x7FFFFFFF; }	/* If the Y scale is too large, saturate to the largest  representable number */
	}

	offsetX = x;
	offsetY = y;
	if (offsetX >= -32768. && offsetY >= -32768.) {									/* Within the range of parameters to FskScaleOffsetBitmap */
		scaleOffset.offsetX = RoundFloatOffsetToFixed(offsetX);
		scaleOffset.offsetY = RoundFloatOffsetToFixed(offsetY);
		FskScaleOffsetBitmap(bits, sR, port->bits, &port->aggregateClipScaled, &scaleOffset, &port->penColor, port->graphicsMode, port->graphicsModeParameters);
	}
	else {																			/* Crop src to reduce coordinates */
		double d0, d1, s0, s1, t;
		FskRectangleRecord subRect;

		d1 = (d0 = port->bits->bounds.x) + port->bits->bounds.width - 1;			/* Destination range */
		s1 = (s0 = sR->x * scaleX + offsetX) + sR->width * scaleX;					/* Src range transformed to dst */
		if (s1 < s0) { t = s1; s1 = s0; s0 = t; }									/* Accommodate negative scale */
		if (d0 < s0) d0 = s0;														/* Intersect transformed src interval with dst interval */
		if (d1 > s1) d1 = s1;
		s0 = (d0 - offsetX) / scaleX;												/* Source coordinates corresponding to dst range */
		s1 = (d1 - offsetX) / scaleX;
		if (s1 < s0) { t = s1; s1 = s0; s0 = t; }									/* Accommodate negative scale */
		subRect.x     = (SInt32)(s0);												/* floor */
		subRect.width = (SInt32)(s1 + (1.f - 1.f / 65536.f)) + 1 - subRect.x;		/* ceil */

		d1 = (d0 = port->bits->bounds.y) + port->bits->bounds.height - 1;			/* Destination range */
		s1 = (s0 = sR->y * scaleY + offsetY) + sR->height * scaleY;					/* Src range transformed to dst */
		if (s1 < s0) { t = s1; s1 = s0; s0 = t; }									/* Accommodate negative scale */
		if (d0 < s0) d0 = s0;														/* Intersect transformed src interval with dst interval */
		if (d1 > s1) d1 = s1;
		s0 = (d0 - offsetY) / scaleY;												/* Source coordinates corresponding to dst range */
		s1 = (d1 - offsetY) / scaleY;
		if (s1 < s0) { t = s1; s1 = s0; s0 = t; }									/* Accommodate negative scale */
		subRect.y      = (SInt32)(s0);												/* floor */
		subRect.height = (SInt32)(s1 + (1.f - 1.f / 65536.f)) + 1 - subRect.y;		/* ceil */

		FskRectangleIntersect(sR, &subRect, &subRect);								/* Don't go outside the src */
		if (sR != &bits->bounds) FskRectangleIntersect(&bits->bounds, &subRect, &subRect);

		offsetX += scaleX * (subRect.x - sR->x);									/* Reduce negative offsets, hopefully to a good range */
		offsetY += scaleY * (subRect.y - sR->y);
		scaleOffset.offsetX = RoundFloatOffsetToFixed(offsetX);
		scaleOffset.offsetY = RoundFloatOffsetToFixed(offsetY);
		FskScaleOffsetBitmap(bits, &subRect, port->bits, &port->aggregateClipScaled, &scaleOffset, &port->penColor, port->graphicsMode, port->graphicsModeParameters);
	}
}

void renderBitmapTile(FskPort port, FskBitmap srcBits, FskConstRectangle srcRect, FskConstRectangle dstRect, FskFixed scale)
{
    FskRectangleRecord dRect = *dstRect;

    FskRectangleOffset(&dRect, port->origin.x, port->origin.y);
	if (FskPortAccumulateChange(port, &dRect)) {
		FskPortRectScale(port, &dRect);
		FskTileBitmap(srcBits, srcRect ? srcRect : &srcBits->bounds, port->bits, &dRect, &port->aggregateClipScaled, scale, &port->penColor, port->graphicsMode, port->graphicsModeParameters);
	}
}

void renderTextDraw(FskPort port, const char *text, UInt32 textLen, FskConstRectangle bounds)
{
	FskRectangleRecord r = *bounds;

	FskRectangleOffset(&r, port->origin.x, port->origin.y);
	if (FskPortAccumulateChange(port, &r)) {
		if (NULL == port->textFormatCache) {
			if (kFskErrNone == FskTextFormatCacheNew(port->textEngine, &port->textFormatCache, port->bits, FskPortUInt32Scale(port, port->textSize), port->textStyle, port->fontName))
                port->textFromFormat = false;
		}

		FskPortRectScale(port, &r);
		FskTextBox(port->textEngine, port->bits, text, textLen, &r, &port->aggregateClipScaled, &port->penColor,
			port->graphicsModeParameters ? port->graphicsModeParameters->blendLevel : 255,
			FskPortUInt32Scale(port, port->textSize), port->textStyle, port->textHAlign, port->textVAlign, port->fontName, port->textFormatCache);
	}
}

FskErr renderPicSaveAdd(FskPort port, FskPortPicRenderItem render, FskPortPicPrepareItem prepare, void *params, UInt32 paramsSize)
{
    (prepare)(port, params, paramsSize);
    (render)(port, params, paramsSize);
    return kFskErrNone;
}

void renderEffectApply(FskPort port, FskBitmap src, FskConstRectangle srcRect, FskConstPoint dstPoint, const struct FskEffectRecord* effect)
{
	FskBitmap			dst		= port->bits;
	FskRectangleRecord	dRect;

	if (!srcRect)	srcRect  = &src->bounds;
	if (!dstPoint)	dstPoint = /*(FskConstPoint)*/(const void*)(&dst->bounds);

	FskRectangleSet(&dRect, dstPoint->x + port->origin.x, dstPoint->y + port->origin.y, srcRect->width, srcRect->height);

	if (FskPortAccumulateChange(port, &dRect)) {
		unsigned			overSize	= (kFskEffectCompound == effect->effectID) ? sizeof(FskEffectRecord) * effect->params.compound.numStages : 0;
		struct FskPortPicEffectParametersRecord *pEffect;
		if (kFskErrNone != FskMemPtrNew(sizeof(struct FskPortPicEffectParametersRecord) + overSize, &pEffect))
			return;
		pEffect->src		= src;
		pEffect->srcRect	= *srcRect;
		pEffect->dstPoint	= *dstPoint;
		FskMemCopy(&pEffect->effect, effect, sizeof(FskEffectRecord) + overSize);
		FskPortEffectScale(port, pEffect);
		FskPortRectScale(port, &dRect);
#if FSKBITMAP_OPENGL && GLES_VERSION == 2
        if (FskBitmapIsOpenGLDestinationAccelerated(dst))
            (void)FskGLEffectApply(&pEffect->effect, pEffect->src, &pEffect->srcRect, dst, &pEffect->dstPoint);
        else
#endif
		{	FskRectangleRecord dstRect;	/* SW case. GL case goes through another path */
			FskBitmap tmpDst;			/* Allocate srcRect size */
			FskRectangleSet(&dstRect, pEffect->dstPoint.x, pEffect->dstPoint.y, pEffect->srcRect.width, pEffect->srcRect.height);
			if (kFskErrNone == FskBitmapNew(srcRect->width, srcRect->height, kFskBitmapFormatDefaultRGBA, &tmpDst)) {
				tmpDst->hasAlpha = true;
				tmpDst->alphaIsPremultiplied = src->alphaIsPremultiplied;
				if (pEffect->src->pixelFormat != tmpDst->pixelFormat) {		/* Effects only work when src and dst are the same format */
					FskBitmap tmpSrc;										/* Allocate a temporary bitmap of the same format as the dst */
					if (kFskErrNone == FskBitmapNew(pEffect->srcRect.width, pEffect->srcRect.height, kFskBitmapFormatDefaultRGBA, &tmpSrc)) {	/* Only allocate srcRect */
						(void)FskBitmapDraw(pEffect->src, &pEffect->srcRect, tmpSrc, &tmpSrc->bounds, NULL, NULL, kFskGraphicsModeCopy, NULL);	/* Only copy srcRect */
						(void)FskEffectApply(&pEffect->effect, tmpSrc, &tmpSrc->bounds, tmpDst, NULL);											/* Apply effect to whole tmpSrc */
						FskBitmapDispose(tmpSrc);
					}
				}
				else {
					(void)FskEffectApply(&pEffect->effect, pEffect->src, &pEffect->srcRect, tmpDst, NULL);	/* Since src and dst formats match, we can apply the effect directly */
				}
				(void)FskBitmapDraw(tmpDst, &tmpDst->bounds, port->bits, &dstRect, &port->aggregateClipScaled, &port->penColor, port->graphicsMode, port->graphicsModeParameters);
				FskBitmapDispose(tmpDst);
			}
		}
		FskMemPtrDispose(pEffect);
	}
}

#if SUPPORT_INSTRUMENTATION

Boolean doFormatMessagePort(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	const char *s;
	int amt;

	switch (msg) {
		case kFskPortInstrMsgSetGraphicsMode:
			amt = snprintf(buffer, bufferSize, "set graphics mode %ld", (SInt32)((void **)msgData)[0]);
			if (NULL != ((void **)msgData)[1]) {
				if ((void *)-1 != ((void **)msgData)[1])
					snprintf(buffer + amt, bufferSize - amt, ", blendLevel=%ld", ((FskGraphicsModeParameters)((void **)msgData)[1])->blendLevel);
			}
			else {
				SInt32 sl = FskStrLen(buffer);
				snprintf(buffer + sl, bufferSize - sl, ", blendLevel=255");
			}
			return true;

		case kFskPortInstrMsgSetClipRectangle:
			s = "set clip rectangle";
			goto doRectangle;

		case kFskPortInstrMsgSetPenColor:
			if (NULL != msgData)
				snprintf(buffer, bufferSize, "set pen color r=%d, g=%d, b=%d, a=%d", ((FskColorRGBA)msgData)->r, ((FskColorRGBA)msgData)->g, ((FskColorRGBA)msgData)->b, ((FskColorRGBA)msgData)->a);
			else
				snprintf(buffer, bufferSize, "set pen color (NULL color)");
			return true;

		case kFskPortInstrMsgSetTextAlignment:
			snprintf(buffer, bufferSize, "set text alignment hAlign = %ld, vAlign = %ld", (SInt32)((UInt32)msgData >> 16), (SInt32)((UInt32)msgData & 0x0ffff));
			return true;

		case kFskPortInstrMsgSetTextSize:
			snprintf(buffer, bufferSize, "set text size %ld", (SInt32)msgData);
			return true;

		case kFskPortInstrMsgSetTextStyle:
			snprintf(buffer, bufferSize, "set text style %ld", (SInt32)msgData);
			return true;

		case kFskPortInstrMsgSetTextFont:
			snprintf(buffer, bufferSize, "set text font %s", (char *)msgData);
			return true;

		case kFskPortInstrMsgInvalidateRectangle:
			s = "invalidate rectangle";
			goto doRectangle;

		case kFskPortInstrMsgSetBitmap:
			snprintf(buffer, bufferSize, "set bitmap %p", msgData);
			return true;

		case kFskPortInstrMsgBeginDrawing:
            if (msgData)
                snprintf(buffer, bufferSize, "begin drawing to window %p", msgData);
            else
                snprintf(buffer, bufferSize, "begin drawing to offscreen");
			return true;

		case kFskPortInstrMsgEndDrawing:
			snprintf(buffer, bufferSize, "end drawing");
			return true;

		case kFskPortInstrMsgRectangleFill:
			s = "rectangle fill";
			goto doRectangle;

		case kFskPortInstrMsgRectangleFrame:
			s = "rectangle frame";
			goto doRectangle;

		case kFskPortInstrMsgApplyMaskAndValue:
			s = "apply mask and value";
			goto doRectangle;

		case kFskPortInstrMsgBitmapDraw: {
			FskBitmap bits = (FskBitmap)((char **)msgData)[0];
			FskRectangle dst = (FskRectangle)((char **)msgData)[1];
			FskRectangle src = (FskRectangle)((char **)msgData)[2];
			Boolean scaled = ((src->width != dst->width) || (src->height != dst->height));

			char *name = (char *)FskInstrumentedItemGetName(bits);
			if (name) {
				snprintf(buffer, bufferSize, scaled ? "bitmap draw scaled '%s' (%p)," : "bitmap draw unscaled '%s' (%p),", name, bits);
				bufferSize -= FskStrLen(buffer);
				buffer += FskStrLen(buffer);
				s = "";
			}
			else {
				snprintf(buffer, bufferSize, scaled ? "bitmap draw scaled (%p)," : "bitmap draw unscaled (%p),", bits);
				bufferSize -= FskStrLen(buffer);
				buffer += FskStrLen(buffer);
				s = "";
			}
			msgData = ((char **)msgData)[1];
			goto doRectangle;
			}

		case kFskPortInstrMsgTextDraw: {
			UInt32 len;

			snprintf(buffer, bufferSize, "text draw '%s',", ((char **)msgData)[0]);
			len = FskStrLen(buffer) + 1;
			if (len >= bufferSize)
				return true;
			bufferSize -= len;
			buffer += len;
			s = "";
			msgData = ((char **)msgData)[1];
			goto doRectangle;
			}

		case kFskPortInstrMsgTextGetBounds: {
			FskRectangle bounds = ((FskRectangle *)msgData)[2];
			char *s = ((char **)msgData)[0];
			SInt32 textLen = ((SInt32 *)msgData)[1];
			if (0 == textLen)
				s = "(0 length string)";
			snprintf(buffer, bufferSize, "get text bounds '%s', width=%ld, height=%ld", s, bounds->width, bounds->height);
			}
			return true;

		case kFskPortInstrMsgTextFitWidth: {
			//FskRectangle bounds = ((FskRectangle *)msgData)[2];
			char *s = ((char **)msgData)[0];
			SInt32 textLen = ((SInt32 *)msgData)[1];
			UInt32 *fitBytes = ((UInt32 **)msgData)[2], *fitChars = ((UInt32 **)msgData)[3];
			if (0 == textLen)
				s = "(0 length string)";
			if (fitBytes && fitChars)
				snprintf(buffer, bufferSize, "text fit width '%s', fitBytes=%ld, fitChars=%ld", s, *fitBytes, *fitChars);
			else if (fitBytes)
				snprintf(buffer, bufferSize, "text fit width '%s', fitBytes=%ld", s, *fitBytes);
			else if (fitChars)
				snprintf(buffer, bufferSize, "text fit width '%s', fitChars=%ld", s, *fitChars);
			else
				snprintf(buffer, bufferSize, "text fit width '%s'", s);
			}
			return true;

		case kFskPortInstrMsgGetFontInfo:
			snprintf(buffer, bufferSize, "get font info");
			return true;

		case kFskPortInstrMsgTextFormatApply:
			snprintf(buffer, bufferSize, "apply text format %p", msgData);
			return true;

		case kFskPortInstrMsgTextFlushCache:
			snprintf(buffer, bufferSize, "flush text cache");
			return true;

		case kFskPortInstrMsgScaleSet:
			snprintf(buffer, bufferSize, "scale set %f", ((UInt32)msgData) / 65536.0);
			return true;

		case kFskPortInstrMsgBitmapScaleOffset:
			snprintf(buffer, bufferSize, "bitmap scale offset");
			return true;

        case kFskPortInstrMsgSetOrigin:
            snprintf(buffer, bufferSize, "set origin x=%d, y=%d", (int)((SInt32*)msgData)[0], (int)((SInt32*)msgData)[1]);
            return true;

        case kFskPortInstrMsgOffsetOrigin:
            snprintf(buffer, bufferSize, "offset origin x=%d, y=%d", (int)((SInt32*)msgData)[0], (int)((SInt32*)msgData)[1]);
            return true;

        case kFskPortInstrMsgGetBitmap:
            snprintf(buffer, bufferSize, "get bitmap");
            return true;

        case kFskPortInstrMsgBitmapTile:
            snprintf(buffer, bufferSize, "bitmap tile");
            return true;

        case kFskPortInstrMsgPicSaveAdd:
            snprintf(buffer, bufferSize, "add external drawing command");
            return true;

        case kFskPortInstrMsgBitmapSubpixel:
            snprintf(buffer, bufferSize, "bitmap subpixel draw");
            return true;

        case kFskPortInstrMsgEffect:
        {	FskConstBitmap		src		=    ((FskConstBitmap*)msgData)[0];
        	FskConstRectangle	srcRect	= ((FskConstRectangle*)msgData)[1];
        	FskConstPoint		dstPt	=     ((FskConstPoint*)msgData)[2];
        	FskConstEffect		eff		=    ((FskConstEffect*)msgData)[3];
			amt = snprintf(buffer, bufferSize, "effect %ssrc=%p srcRect(%d,%d,%d,%d) dstPt(%d,%d): ",
					(src->alphaIsPremultiplied ? "premul " : ""),	src,
					(int)srcRect->x,		(int)srcRect->y,		(int)srcRect->width,	(int)srcRect->height,
					(int)dstPt->x,			(int)dstPt->y);
			if (bufferSize > (UInt32)amt)	bufferSize -= amt;
			else							bufferSize = 0;
			buffer += amt;
			switch (eff->effectID) {
				case kFskEffectBoxBlur:
					snprintf(buffer, bufferSize, "BoxBlur blur(%d,%d)", eff->params.boxBlur.radiusX, eff->params.boxBlur.radiusY);
					break;
				case kFskEffectColorize:
					snprintf(buffer, bufferSize, "Colorize color(%d,%d,%d,%d)",
						eff->params.colorize.color.r, eff->params.colorize.color.b, eff->params.colorize.color.b, eff->params.colorize.color.a);
					break;
				case kFskEffectColorizeAlpha:
					snprintf(buffer, bufferSize, "ColorizeAlpha color0(%d,%d,%d,%d) color1(%d,%d,%d,%d)",
						eff->params.colorizeAlpha.color0.r, eff->params.colorizeAlpha.color0.b, eff->params.colorizeAlpha.color0.b, eff->params.colorizeAlpha.color0.a,
						eff->params.colorizeAlpha.color1.r, eff->params.colorizeAlpha.color1.b, eff->params.colorizeAlpha.color1.b, eff->params.colorizeAlpha.color1.a);
					break;
				case kFskEffectColorizeInner:
					snprintf(buffer, bufferSize, "ColorizeInner matte=%p color(%d,%d,%d,%d)", eff->params.colorizeInner.matte,
						eff->params.colorizeInner.color.r, eff->params.colorizeInner.color.b, eff->params.colorizeInner.color.b, eff->params.colorizeInner.color.a);
					break;
				case kFskEffectColorizeOuter:
					snprintf(buffer, bufferSize, "ColorizeOuter matte=%p color(%d,%d,%d,%d)", eff->params.colorizeOuter.matte,
						eff->params.colorizeOuter.color.r, eff->params.colorizeOuter.color.b, eff->params.colorizeOuter.color.b, eff->params.colorizeOuter.color.a);
					break;
				case kFskEffectCompound:
					snprintf(buffer, bufferSize, "Compound numStages=%d", eff->params.compound.numStages);
					break;
				case kFskEffectCopy:
					snprintf(buffer, bufferSize, "Copy");
					break;
				case kFskEffectCopyMirrorBorders:
					snprintf(buffer, bufferSize, "CopyMirrorBorders border=%u", (unsigned)eff->params.copyMirrorBorders.border);
					break;
				case kFskEffectDirectionalDilate:
					snprintf(buffer, bufferSize, "DirectionalDilate ...");
					break;
				case kFskEffectDirectionalErode:
					snprintf(buffer, bufferSize, "DirectionalErode ...");
					break;
				case kFskEffectDirectionalGaussianBlur:
					snprintf(buffer, bufferSize, "DirectionalGaussianBlur ...");
					break;
				case kFskEffectGaussianBlur:
					snprintf(buffer, bufferSize, "GaussianBlur blur(%g,%g)", eff->params.gaussianBlur.sigmaX, eff->params.gaussianBlur.sigmaY);
					break;
				case kFskEffectInnerGlow:
					snprintf(buffer, bufferSize, "InnerGlow r=%i blur=%g color(%d,%d,%d,%d)", eff->params.innerGlow.radius, eff->params.innerGlow.blurSigma,
						eff->params.innerGlow.color.r, eff->params.innerGlow.color.b, eff->params.innerGlow.color.b, eff->params.innerGlow.color.a);
					break;
				case kFskEffectInnerShadow:
					snprintf(buffer, bufferSize, "InnerShadow d(%d,%d) blur=%g color(%u,%u,%u,%u)",
						(int)eff->params.innerShadow.offset.x, (int)eff->params.innerShadow.offset.y, eff->params.innerShadow.blurSigma,
						eff->params.innerShadow.color.r, eff->params.innerShadow.color.b, eff->params.innerShadow.color.b, eff->params.innerShadow.color.a);
					break;
				case kFskEffectMask:
					snprintf(buffer, bufferSize, "Mask mask=%p maskRect={%d %d %d %d}", eff->params.mask.mask,
						(int)eff->params.mask.maskRect.x, (int)eff->params.mask.maskRect.y, (int)eff->params.mask.maskRect.width, (int)eff->params.mask.maskRect.height);
					break;
				case kFskEffectMonochrome:
					snprintf(buffer, bufferSize, "Monochrome color0(%u,%u,%u,%u) color1(%u,%u,%u,%u)",
						eff->params.monochrome.color0.r, eff->params.monochrome.color0.b, eff->params.monochrome.color0.b, eff->params.monochrome.color0.a,
						eff->params.monochrome.color1.r, eff->params.monochrome.color1.b, eff->params.monochrome.color1.b, eff->params.monochrome.color1.a);
					break;
				case kFskEffectOuterGlow:
					snprintf(buffer, bufferSize, "OuterGlow r=%i blur=%g color(%d,%d,%d,%d)", eff->params.outerGlow.radius, eff->params.outerGlow.blurSigma,
						eff->params.outerGlow.color.r, eff->params.outerGlow.color.b, eff->params.outerGlow.color.b, eff->params.outerGlow.color.a);
					break;
				case kFskEffectOuterShadow:
					snprintf(buffer, bufferSize, "OuterShadow d(%d,%d) blur=%g color(%u,%u,%u,%u)",
						(int)eff->params.outerShadow.offset.x, (int)eff->params.outerShadow.offset.y, eff->params.outerShadow.blurSigma,
						eff->params.outerShadow.color.r, eff->params.outerShadow.color.b, eff->params.outerShadow.color.b, eff->params.outerShadow.color.a);
					break;
				case kFskEffectShade:
					snprintf(buffer, bufferSize, "Shade shadow=%p shadowRect={%d %d %d %d} opacity=%u", eff->params.shade.shadow,
						(int)eff->params.shade.shadowRect.x, (int)eff->params.shade.shadowRect.y, (int)eff->params.shade.shadowRect.width, (int)eff->params.shade.shadowRect.height,
						(unsigned)eff->params.shade.opacity);
					break;
				default:
					snprintf(buffer, bufferSize, "Unknown effectID=%d", (int)(eff->effectID));
					break;
			}
            return true;
        }
	}

	return false;

doRectangle:
	if (NULL != msgData) {
		FskRectangle r = (FskRectangle)msgData;
		snprintf(buffer, bufferSize, "%s x=%ld, y=%ld, width=%ld, height=%ld, area=%lld", s, r->x, r->y, r->width, r->height, (FskInt64)r->width * r->height);
	}
	else
		snprintf(buffer, bufferSize, "%s (NULL rectangle)", s);

	return true;
}

#endif
