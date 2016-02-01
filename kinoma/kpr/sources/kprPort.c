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
#include "kpr.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprImage.h"
#include "kprLabel.h"
#include "kprLayer.h"
#include "kprPort.h"
#include "kprSkin.h"
#include "kprText.h"
#include "kprShell.h"
#include "kprUtilities.h"
#include "FskPerspective.h"
#include "FskMemory.h"

static void KprPortDispose(void* it);
static void KprPortDraw(void* it, FskPort port, FskRectangle area);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprPortInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprPort", FskInstrumentationOffset(KprPortRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprPortDispatchRecord = {
	"port",
	KprContentActivated,
	KprContentAdded,
	KprContentCascade,
	KprPortDispose,
	KprPortDraw,
	KprContentFitHorizontally,
	KprContentFitVertically,
	KprContentGetBitmap,
	KprContentHit,
	KprContentIdle,
	KprContentInvalidated,
	KprContentLayered,
	KprContentMark,
	KprContentMeasureHorizontally,
	KprContentMeasureVertically,
	KprContentPlace,
	KprContentPlaced,
	KprContentPredict,
	KprContentReflowing,
	KprContentRemoving,
	KprContentSetWindow,
	KprContentShowing,
	KprContentShown,
	KprContentUpdate
};

FskErr KprPortNew(KprPort* it,  KprCoordinates coordinates)
{
	FskErr err = kFskErrNone;
	KprPort self;
	KprCoordinatesRecord zeroes = {0, 0, 0, 0, 0, 0, 0, 0};
	bailIfError(FskMemPtrNewClear(sizeof(KprPortRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprPortInstrumentation);
	self->dispatch = &KprPortDispatchRecord;
	self->flags = kprVisible | kprPort;
	KprContentInitialize((KprContent)self, coordinates, NULL, NULL);
	self->corners[1].x = 1;
	self->corners[2].x = 1;
	self->corners[2].y = 1;
	self->corners[3].y = 1;
	self->opacity = 1;
	self->scale.x = 1;
	self->scale.y = 1;
	KprContentNew(&self->content, &zeroes, NULL, NULL);
	KprLabelNew(&self->label, &zeroes, NULL, NULL, "");
	KprTextNew(&self->text, &zeroes, NULL, NULL, "");
bail:
	return err;
}

/* DISPATCH */

void KprPortDispose(void* it)
{
	KprPort self = it;
	KprAssetUnbind(self->effect);
	if (self->text)
		(*self->text->dispatch->dispose)(self->text);
	if (self->label)
		(*self->label->dispatch->dispose)(self->label);
	if (self->content)
		(*self->content->dispatch->dispose)(self->content);
	FskGrowableArrayDispose(self->clipBuffer);
	FskMemPtrDispose(self->hitMatrix);
	KprContentDispose(it);
}

void KprPortDraw(void* it, FskPort port, FskRectangle area)
{
	KprPort content = it;
	KprScriptBehavior self = (KprScriptBehavior)content->behavior;
#if SUPPORT_INSTRUMENTATION
	void* params[2] = { self, "xsID_onDraw" };
	FskInstrumentedItemSendMessageDebug(content, kprInstrumentedContentCallBehavior, params);
#endif
    if (self) {
        xsBeginHostSandboxCode(self->the, self->code);
        {
            xsVars(2);
            if (xsFindResult(self->slot, xsID_onDraw)) {
                FskRectangleRecord clip;
                FskPortGetClipRectangle(port, &clip);
                //fprintf(stderr, "KprPortDraw %p %ld %ld\n", content, content->bounds.width, content->bounds.height);
                xsVar(0) = self->slot;
                xsVar(1) = kprContentGetter(content);
                content->port = port;
                content->area = area;
                (void)xsCallFunction5(xsResult, xsVar(0), xsVar(1), xsInteger(0), xsInteger(0),
                        xsInteger(content->bounds.width), xsInteger(content->bounds.height));
                content->port = NULL;
                FskPortSetClipRectangle(port, &clip);
            }
        }
        xsEndHostSandboxCode();
    }
}

/* ECMASCRIPT */

void KPR_Port(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprPort self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	xsThrowIfFskErr(KprPortNew(&self, &coordinates));
	kprContentConstructor(KPR_Port);
}

static void KPR_port_drawContent(xsMachine* the, KprPort self, KprContent content)
{
	KprCoordinates coordinates = &content->coordinates;
	coordinates->horizontal = kprLeft | kprWidth;
	coordinates->vertical = kprTop | kprHeight;
	coordinates->left = xsToInteger(xsArg(1));
	coordinates->right = 0;
	coordinates->top = xsToInteger(xsArg(2));
	coordinates->bottom = 0;
	coordinates->width = xsToInteger(xsArg(3));
	coordinates->height = xsToInteger(xsArg(4));

	self->first = self->last = content;
	(*content->dispatch->setWindow)(content, self->shell, self->style);
	content->container = (KprContainer)self;
	(*content->dispatch->measureHorizontally)(content);
	(*content->dispatch->fitHorizontally)(content);
	KprContentPlaceHorizontally(content, self->bounds.width);
	(*content->dispatch->measureVertically)(content);
	(*content->dispatch->fitVertically)(content);
	KprContentPlaceVertically(content, self->bounds.height);

    (*content->dispatch->update)(content, self->port, self->area);

	content->container = NULL;
	(*content->dispatch->setWindow)(content, NULL, NULL);
	self->first = self->last = NULL;
}

static void KPR_port_measureContent(xsMachine*the UNUSED, KprPort self, KprContent content, SInt32 width, SInt32 height)
{
	KprCoordinates coordinates = &content->coordinates;
	coordinates->horizontal = kprLeft;
	if (width) coordinates->horizontal |= kprWidth;
	coordinates->vertical = kprTop;
	if (height) coordinates->vertical |= kprHeight;
	coordinates->left = 0;
	coordinates->right = 0;
	coordinates->top = 0;
	coordinates->bottom = 0;
	coordinates->width = width;
	coordinates->height = height;
	self->first = self->last = content;
	(*content->dispatch->setWindow)(content, self->shell, self->style);
	content->container = (KprContainer)self;
	(*content->dispatch->measureHorizontally)(content);
    (*content->dispatch->fitHorizontally)(content);
	(*content->dispatch->measureVertically)(content);
    (*content->dispatch->fitVertically)(content);
	content->container = NULL;
	(*content->dispatch->setWindow)(content, NULL, NULL);
	self->first = self->last = NULL;
}

void KPR_port_drawImage(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	Boolean owned = false;
	FskBitmap bitmap = NULL;
	FskRectangleRecord bounds, dstRect, srcRect;
	if (!self->port)
		xsError(kFskErrOutOfSequence);
	if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID_texture))) {
		KprTexture texture = xsGetHostData(xsArg(0));
		bitmap = KprTextureGetBitmap(texture, self->port, &owned);
	}
	else {
		KprContent content = xsGetHostData(xsArg(0));
		bitmap = (*content->dispatch->getBitmap)(content, self->port, &owned);
	}
	//fprintf(stderr, "KPR_port_drawImage %p %p\n", self, bitmap);
	if (!bitmap)
		return;
	FskBitmapGetBounds(bitmap, &bounds);
	if ((c > 1) && xsTest(xsArg(1)))
		dstRect.x = xsToInteger(xsArg(1));
	else
		dstRect.x = 0;
	if ((c > 2) && xsTest(xsArg(2)))
		dstRect.y = xsToInteger(xsArg(2));
	else
		dstRect.y = 0;
	if ((c > 3) && xsTest(xsArg(3)))
		dstRect.width = xsToInteger(xsArg(3));
	else
		dstRect.width = bounds.width;
	if ((c > 4) && xsTest(xsArg(4)))
		dstRect.height = xsToInteger(xsArg(4));
	else
		dstRect.height = bounds.height;
	if ((c > 5) && xsTest(xsArg(5)))
		srcRect.x = xsToInteger(xsArg(5));
	else
		srcRect.x = 0;
	if ((c > 6) && xsTest(xsArg(6)))
		srcRect.y = xsToInteger(xsArg(6));
	else
		srcRect.y = 0;
	if ((c > 7) && xsTest(xsArg(7)))
		srcRect.width = xsToInteger(xsArg(7));
	else
		srcRect.width = bounds.width;
	if ((c > 8) && xsTest(xsArg(8)))
		srcRect.height = xsToInteger(xsArg(8));
	else
		srcRect.height = bounds.height;
	bounds = self->bounds;
	FskRectangleSet(&self->bounds, 0, 0, srcRect.width, srcRect.height);
	KprLayerComputeMatrix((KprLayer)self);
	KprLayerBlit(self, self->port, bitmap, &srcRect, &dstRect);
	self->bounds = bounds;
	if (!owned)
		FskBitmapDispose(bitmap);
}

void KPR_port_drawLabel(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	xsStringValue string = xsToString(xsArg(0));
	KprLabel label = self->label;
	if (!self->port)
		xsError(kFskErrOutOfSequence);
	//fprintf(stderr, "KPR_port_drawLabel %p\n", self);
	KprLabelSetString(label, string);

	if (self->opacity < 1) {
		FskGraphicsModeParametersRecord param;
		param.dataSize = sizeof(FskGraphicsModeParametersRecord);
		param.blendLevel = (SInt32)(255 * self->opacity);
		FskPortSetGraphicsMode(self->port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, &param);
	}
	KPR_port_drawContent(the, self, (KprContent)label);
	if (self->opacity < 1)
		FskPortSetGraphicsMode(self->port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);

	KprLabelSetString(label, "");
}

void KPR_port_drawText(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	xsStringValue string = xsToString(xsArg(0));
	KprText text = self->text;
	if (!self->port)
		xsError(kFskErrOutOfSequence);
	//fprintf(stderr, "KPR_port_drawText %p\n", self);
	KprTextBegin(text);
    KprTextConcatString(text, string);
	KprTextEnd(text);

	if (self->opacity < 1) {
		FskGraphicsModeParametersRecord param;
		param.dataSize = sizeof(FskGraphicsModeParametersRecord);
		param.blendLevel = (SInt32)(255 * self->opacity);
		FskPortSetGraphicsMode(self->port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, &param);
	}
	KPR_port_drawContent(the, self, (KprContent)text);
	if (self->opacity < 1)
		FskPortSetGraphicsMode(self->port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);

	KprTextBegin(text);
	KprTextEnd(text);
}

void KPR_port_fillColor(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	FskPort port = self->port;
	FskColorRGBARecord oldColor, newColor;
	FskRectangleRecord bounds;
	if (!port)
		xsError(kFskErrOutOfSequence);
	FskPortGetPenColor(port, &oldColor);
	KprParseColor(the, xsToString(xsArg(0)), &newColor);
	newColor.a = (unsigned char)(newColor.a * self->opacity);
	if (newColor.a) {
		if (newColor.a != 255) {
			FskGraphicsModeParametersRecord param;
			param.dataSize = sizeof(FskGraphicsModeParametersRecord);
			param.blendLevel = (SInt32)newColor.a;
			FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, &param);
			newColor.a = 255;
		}
		FskPortSetPenColor(port, &newColor);
		bounds.x = xsToInteger(xsArg(1));
		bounds.y = xsToInteger(xsArg(2));
		bounds.width = xsToInteger(xsArg(3));
		bounds.height = xsToInteger(xsArg(4));
		FskPortRectangleFill(port, &bounds);
	}
	FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
	FskPortSetPenColor(port, &oldColor);
/*
	KprPort self = xsGetHostData(xsThis);
	KprContent content = self->content;
	KprSkin skin = self->skin;
	KprTexture texture = skin->texture ;
	FskColorRGBARecord color;
	if (!self->port)
		xsError(kFskErrOutOfSequence);
	KprParseColor(the, xsToString(xsArg(0)), &color);
	KprSkinSetFillColor(skin, -1, &color);
	skin->texture = NULL;
	content->skin = skin;
	KPR_port_drawContent(the, self, content);
	content->skin = NULL;
	skin->texture = texture;
*/
}

void KPR_port_fillImage(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	Boolean owned = false;
	FskBitmap bitmap = NULL;
	FskRectangleRecord bounds, dstRect, srcRect;
	xsNumberValue opacity = 1;
	if (!self->port)
		xsError(kFskErrOutOfSequence);
	if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID_texture))) {
		KprTexture texture = xsGetHostData(xsArg(0));
		bitmap = KprTextureGetBitmap(texture, self->port, &owned);
	}
	else {
		KprContent content = xsGetHostData(xsArg(0));
		bitmap = (*content->dispatch->getBitmap)(content, self->port, &owned);
	}
	if (!bitmap)
		return;
	FskBitmapGetBounds(bitmap, &bounds);
	if ((c > 1) && xsTest(xsArg(1)))
		dstRect.x = xsToInteger(xsArg(1));
	else
		dstRect.x = 0;
	if ((c > 2) && xsTest(xsArg(2)))
		dstRect.y = xsToInteger(xsArg(2));
	else
		dstRect.y = 0;
	if ((c > 3) && xsTest(xsArg(3)))
		dstRect.width = xsToInteger(xsArg(3));
	else
		dstRect.width = bounds.width;
	if ((c > 4) && xsTest(xsArg(4)))
		dstRect.height = xsToInteger(xsArg(4));
	else
		dstRect.height = bounds.height;
	if ((c > 5) && xsTest(xsArg(5)))
		srcRect.x = xsToInteger(xsArg(5));
	else
		srcRect.x = 0;
	if ((c > 6) && xsTest(xsArg(6)))
		srcRect.y = xsToInteger(xsArg(6));
	else
		srcRect.y = 0;
	if ((c > 7) && xsTest(xsArg(7)))
		srcRect.width = xsToInteger(xsArg(7));
	else
		srcRect.width = bounds.width;
	if ((c > 8) && xsTest(xsArg(8)))
		srcRect.height = xsToInteger(xsArg(8));
	else
		srcRect.height = bounds.height;
	if ((c > 9) && ((xsTypeOf(xsArg(9)) == xsIntegerType) || (xsTypeOf(xsArg(9)) == xsNumberType)))
		opacity = xsToNumber(xsArg(9));
	if (opacity != 1) {
		FskGraphicsModeParametersRecord param;
		param.dataSize = sizeof(FskGraphicsModeParametersRecord);
		param.blendLevel = (SInt32)(opacity * 255);
		FskPortSetGraphicsMode(self->port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, &param);
		FskPortBitmapTile(self->port, bitmap, &srcRect, &dstRect, FskPortScaleGet(self->port));
		FskPortSetGraphicsMode(self->port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
	}
	else
		FskPortBitmapTile(self->port, bitmap, &srcRect, &dstRect, FskPortScaleGet(self->port));
	if (!owned)
		FskBitmapDispose(bitmap);
}

void KPR_port_intersectClip(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	FskRectangleRecord r1, r2;
	if (!self->port)
		xsError(kFskErrOutOfSequence);
	r1.x = xsToInteger(xsArg(0));
	r1.y = xsToInteger(xsArg(1));
	r1.width = xsToInteger(xsArg(2));
	r1.height = xsToInteger(xsArg(3));
	FskPortGetClipRectangle(self->port, &r2);
	FskRectangleIntersect(&r1, &r2, &r1);
	FskPortSetClipRectangle(self->port, &r1);
}

void KPR_port_invalidate(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	KprContentInvalidate((KprContent)self);
}

void KPR_port_measureImage(xsMachine* the)
{
	Boolean owned = false;
	FskBitmap bitmap = NULL;
	FskRectangleRecord bounds;
	if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID_texture))) {
		KprTexture texture = xsGetHostData(xsArg(0));
		bitmap = KprTextureGetBitmap(texture, NULL, &owned);
	}
	else {
		KprContent content = xsGetHostData(xsArg(0));
		bitmap = (*content->dispatch->getBitmap)(content, NULL, &owned);
	}
	if (!bitmap)
		return;
	FskBitmapGetBounds(bitmap, &bounds);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(bounds.width), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(bounds.height), xsDefault, xsDontScript);
}

void KPR_port_measureLabel(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	xsStringValue string = xsToString(xsArg(0));
	KprLabel label = self->label;
	KprLabelSetString(label, string);

	KPR_port_measureContent(the, self, (KprContent)label, 0, 0);

	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(label->coordinates.width), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(label->coordinates.height), xsDefault, xsDontScript);

	KprLabelSetString(label, "");
}

void KPR_port_measureText(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	xsStringValue string = xsToString(xsArg(0));
	xsIntegerValue width = xsToInteger(xsArg(1));
	KprText text = self->text;
	//if ((c > 2) && xsTest(xsArg(2)))
	//	text.truncate

	KprTextBegin(text);
    KprTextConcatString(text, string);
	KprTextEnd(text);

	KPR_port_measureContent(the, self, (KprContent)text, width, 0);

	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(text->bounds.width), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(text->bounds.height), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_lines, xsInteger(text->lineCount), xsDefault, xsDontScript);

	KprTextBegin(text);
	KprTextEnd(text);
}

void KPR_port_popClip(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	FskRectangleRecord clip;
	if (!self->port)
		xsError(kFskErrOutOfSequence);
	if (!self->clipBuffer)
		xsError(kFskErrOutOfSequence);
	xsThrowIfFskErr(FskGrowableArrayGetItem(self->clipBuffer, 0, &clip));
	FskGrowableArrayRemoveItem(self->clipBuffer, 0);
	FskPortSetClipRectangle(self->port, &clip);
}

void KPR_port_pushClip(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	FskRectangleRecord clip;
	if (!self->port)
		xsError(kFskErrOutOfSequence);
	FskPortGetClipRectangle(self->port, &clip);
	if (!self->clipBuffer)
		xsThrowIfFskErr(FskGrowableArrayNew(sizeof(FskRectangleRecord), 1, &(self->clipBuffer)));
	xsThrowIfFskErr(FskGrowableArrayInsertItemAtPosition(self->clipBuffer, 0, (void *)&clip));
}

void KPR_port_reset(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	self->corners[1].x = 0;
	self->corners[1].y = 0;
	self->corners[1].x = 1;
	self->corners[1].y = 0;
	self->corners[2].x = 1;
	self->corners[2].y = 1;
	self->corners[3].x = 0;
	self->corners[3].y = 1;
	self->opacity = 1;
	self->origin.x = 0;
	self->origin.y = 0;
	self->rotation = 0;
	self->scale.x = 1;
	self->scale.y = 1;
	self->skew.x = 0;
	self->skew.y = 0;
	self->translation.x = 0;
	self->translation.y = 0;
}

#include "FskPerspective.h"

void KPR_port_projectImage3D(xsMachine* the)
{
	KprPort self = xsGetHostData(xsThis);
	Boolean owned = false;
	FskBitmap bitmap = NULL;
	FskRectangleRecord bounds;
	FskFloatRectangleRecord dstRect;
	SInt32 width;
	SInt32 height;
	float billboardWidth;
	float billboardHeight;
	FskPoint3F position;
	FskQuaternionWXYZ orientation;
	xsNumberValue number;
	FskEmbeddingRecord embedding;
	float focalLength;
	FskCameraRecord camera;
	float P[4][4], M[3][3];

	if (!self->port)
		xsError(kFskErrOutOfSequence);
	if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID_texture))) {
		KprTexture texture = xsGetHostData(xsArg(0));
		bitmap = KprTextureGetBitmap(texture, self->port, &owned);
	}
	else {
		KprContent content = xsGetHostData(xsArg(0));
		bitmap = (*content->dispatch->getBitmap)(content, self->port, &owned);
	}
	if (!bitmap)
		return;
	FskBitmapGetBounds(bitmap, &bounds);

	xsEnterSandbox();
	width = bounds.width;
	height = bounds.height;
	position.x = 0;
	position.y = 0;
	position.z = 0;
	orientation.w = 1;
	orientation.x = 0;
	orientation.y = 0;
	orientation.z = 0;
	if (!xsFindNumber(xsArg(1), xsID_width, &billboardWidth))
        billboardWidth = 0;
	if (!xsFindNumber(xsArg(1), xsID_height, &billboardHeight))
        billboardHeight = 0;
	if (xsFindResult(xsArg(1), xsID_position)) {
		if (xsFindNumber(xsResult, xsID_x, &number))
			position.x = (float)number;
		if (xsFindNumber(xsResult, xsID_y, &number))
			position.y = (float)number;
		if (xsFindNumber(xsResult, xsID_z, &number))
			position.z = (float)number;
	}
	if (xsFindResult(xsArg(1), xsID_orientation)) {
		if (xsFindNumber(xsResult, xsID_w, &number))
			orientation.w = (float)number;
		if (xsFindNumber(xsResult, xsID_x, &number))
			orientation.x = (float)number;
		if (xsFindNumber(xsResult, xsID_y, &number))
			orientation.y = (float)number;
		if (xsFindNumber(xsResult, xsID_z, &number))
			orientation.z = (float)number;
	}

	dstRect.x = -billboardWidth * 0.5f;
	dstRect.y = billboardHeight * 0.5f;
	dstRect.width = billboardWidth;
	dstRect.height = -billboardHeight;

	FskEmbeddingMake(&bounds, &dstRect, &position, &orientation, &embedding);

	width = bounds.width;
	height = bounds.height;
	position.x = 0;
	position.y = 0;
	position.z = 1;
	orientation.w = 1;
	orientation.x = 0;
	orientation.y = 0;
	orientation.z = 0;
	focalLength = 1;
	xsFindInteger(xsArg(2), xsID_width, &width);
	xsFindInteger(xsArg(2), xsID_height, &height);
	if (xsFindResult(xsArg(2), xsID_position)) {
		if (xsFindNumber(xsResult, xsID_x, &number))
			position.x = (float)number;
		if (xsFindNumber(xsResult, xsID_y, &number))
			position.y = (float)number;
		if (xsFindNumber(xsResult, xsID_z, &number))
			position.z = (float)number;
	}
	if (xsFindResult(xsArg(2), xsID_orientation)) {
		if (xsFindNumber(xsResult, xsID_w, &number))
			orientation.w = (float)number;
		if (xsFindNumber(xsResult, xsID_x, &number))
			orientation.x = (float)number;
		if (xsFindNumber(xsResult, xsID_y, &number))
			orientation.y = (float)number;
		if (xsFindNumber(xsResult, xsID_z, &number))
			orientation.z = (float)number;
	}
	if (xsFindNumber(xsArg(2), xsID_focalLength, &number))
		focalLength = (float)number;
	FskCameraMake(width, height, &position, &orientation, focalLength, &camera);
	xsLeaveSandbox();

	FskSLinearTransform(embedding.M[0], camera.M[0], P[0], 4, 4, 4);
	M[0][0] = P[0][0];	M[0][1] = P[0][1];	M[0][2] = P[0][2];
	M[1][0] = P[1][0];	M[1][1] = P[1][1];	M[1][2] = P[1][2];
	M[2][0] = P[3][0];	M[2][1] = P[3][1];	M[2][2] = P[3][2];

	if (self->opacity != 1) {
		FskGraphicsModeParametersRecord param;
		param.dataSize = sizeof(FskGraphicsModeParametersRecord);
		param.blendLevel = (SInt32)(self->opacity * 255);
		FskPortSetGraphicsMode(self->port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, &param);
		FskPortBitmapProject(self->port, bitmap, &bounds, M);
		FskPortSetGraphicsMode(self->port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
	}
	else
		FskPortBitmapProject(self->port, bitmap, &bounds, M);

	if (!owned)
		FskBitmapDispose(bitmap);
}

void Math_slerpUnitQuaternionWXYZ(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	float alpha;		/* interpolation parameter (0 to 1) */
	xsNumberValue number;
	FskQuaternionWXYZ q0;
	FskQuaternionWXYZ q1;
	int spin;
	FskQuaternionWXYZ q;

	xsEnterSandbox();
	alpha = (float)xsToNumber(xsArg(0));
	q0.w = 1;
	q0.x = 0;
	q0.y = 0;
	q0.z = 0;
	if (xsFindNumber(xsArg(1), xsID_w, &number))
		q0.w = (float)number;
	if (xsFindNumber(xsArg(1), xsID_x, &number))
		q0.x = (float)number;
	if (xsFindNumber(xsArg(1), xsID_y, &number))
		q0.y = (float)number;
	if (xsFindNumber(xsArg(1), xsID_z, &number))
		q0.z = (float)number;
	q1.w = 1;
	q1.x = 0;
	q1.y = 0;
	q1.z = 0;
	if (xsFindNumber(xsArg(2), xsID_w, &number))
		q1.w = (float)number;
	if (xsFindNumber(xsArg(2), xsID_x, &number))
		q1.x = (float)number;
	if (xsFindNumber(xsArg(2), xsID_y, &number))
		q1.y = (float)number;
	if (xsFindNumber(xsArg(2), xsID_z, &number))
		q1.z = (float)number;
	spin = xsToInteger(xsArg(3));
	FskSlerpUnitQuaternionWXYZ(alpha, &q0, &q1, spin, &q);
	if ((c > 4) && xsTest(xsArg(4)))
		xsResult = xsArg(4);
	else
		xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsSet(xsResult, xsID_w, xsNumber(q.w));
	xsSet(xsResult, xsID_x, xsNumber(q.x));
	xsSet(xsResult, xsID_y, xsNumber(q.y));
	xsSet(xsResult, xsID_z, xsNumber(q.z));
	xsLeaveSandbox();
}

void Math_quaternionWXYZFromNormalUp(xsMachine* the) {
	xsIntegerValue c = xsToInteger(xsArgc);
	xsNumberValue number;
	FskVector3F normal;
	FskVector3F up;
	FskQuaternionWXYZ q;

	xsEnterSandbox();

	normal.x = 0;
	normal.y = 0;
	normal.z = 0;
	if (xsFindNumber(xsArg(0), xsID_x, &number))
		normal.x = (float)number;
	if (xsFindNumber(xsArg(0), xsID_y, &number))
		normal.y = (float)number;
	if (xsFindNumber(xsArg(0), xsID_z, &number))
		normal.z = (float)number;

	up.x = 0;
	up.y = 0;
	up.z = 0;
	if (xsFindNumber(xsArg(1), xsID_x, &number))
		up.x = (float)number;
	if (xsFindNumber(xsArg(1), xsID_y, &number))
		up.y = (float)number;
	if (xsFindNumber(xsArg(1), xsID_z, &number))
		up.z = (float)number;

	FskQuaternionWXYZFromNormalUp(&normal, &up, &q);

	if ((c > 2) && xsTest(xsArg(2)))
		xsResult = xsArg(2);
	else
		xsResult = xsNewInstanceOf(xsObjectPrototype);

	xsSet(xsResult, xsID_w, xsNumber(q.w));
	xsSet(xsResult, xsID_x, xsNumber(q.x));
	xsSet(xsResult, xsID_y, xsNumber(q.y));
	xsSet(xsResult, xsID_z, xsNumber(q.z));

	xsLeaveSandbox();
}

// void FskCameraDistances(FskConstCamera camera, FskConstEmbedding embedding, UInt32 numPts, const FskPoint2F *pts, float *distances) {
// Math.cameraDistances(textureWidth, textureHeight, camera, billboard, pts, distancesOut)

void Math_cameraDistances(xsMachine* the) {
	xsIntegerValue c = xsToInteger(xsArgc);

	FskRectangleRecord bounds;
	FskFloatRectangleRecord dstRect;
	SInt32 width;
	SInt32 height;
	float billboardWidth;
	float billboardHeight;
	FskPoint3F position;
	FskQuaternionWXYZ orientation;
	FskEmbeddingRecord embedding;
	float focalLength;
	FskCameraRecord camera;
	xsNumberValue number;
	float distances[16];

	xsEnterSandbox();

	bounds.x = 0;
	bounds.y = 0;
	bounds.width = (UInt32)xsToInteger(xsArg(0));
	bounds.height = (UInt32)xsToInteger(xsArg(1));

	width = 0;
	height = 0;
	position.x = 0;
	position.y = 0;
	position.z = 1;
	orientation.w = 1;
	orientation.x = 0;
	orientation.y = 0;
	orientation.z = 0;
	focalLength = 1;
	xsFindInteger(xsArg(2), xsID_width, &width);
	xsFindInteger(xsArg(2), xsID_height, &height);
	if (xsFindResult(xsArg(2), xsID_position)) {
		if (xsFindNumber(xsResult, xsID_x, &number))
			position.x = (float)number;
		if (xsFindNumber(xsResult, xsID_y, &number))
			position.y = (float)number;
		if (xsFindNumber(xsResult, xsID_z, &number))
			position.z = (float)number;
	}
	if (xsFindResult(xsArg(2), xsID_orientation)) {
		if (xsFindNumber(xsResult, xsID_w, &number))
			orientation.w = (float)number;
		if (xsFindNumber(xsResult, xsID_x, &number))
			orientation.x = (float)number;
		if (xsFindNumber(xsResult, xsID_y, &number))
			orientation.y = (float)number;
		if (xsFindNumber(xsResult, xsID_z, &number))
			orientation.z = (float)number;
	}
	if (xsFindNumber(xsArg(2), xsID_focalLength, &number))
		focalLength = (float)number;
	FskCameraMake(width, height, &position, &orientation, focalLength, &camera);

	width = 0;
	height = 0;
	position.x = 0;
	position.y = 0;
	position.z = 0;
	orientation.w = 1;
	orientation.x = 0;
	orientation.y = 0;
	orientation.z = 0;
	if (!xsFindNumber(xsArg(3), xsID_width, &billboardWidth))
        billboardWidth = 0;
	if (!xsFindNumber(xsArg(3), xsID_height, &billboardHeight))
        billboardHeight = 0;
	if (xsFindResult(xsArg(3), xsID_position)) {
		if (xsFindNumber(xsResult, xsID_x, &number))
			position.x = (float)number;
		if (xsFindNumber(xsResult, xsID_y, &number))
			position.y = (float)number;
		if (xsFindNumber(xsResult, xsID_z, &number))
			position.z = (float)number;
	}
	if (xsFindResult(xsArg(3), xsID_orientation)) {
		if (xsFindNumber(xsResult, xsID_w, &number))
			orientation.w = (float)number;
		if (xsFindNumber(xsResult, xsID_x, &number))
			orientation.x = (float)number;
		if (xsFindNumber(xsResult, xsID_y, &number))
			orientation.y = (float)number;
		if (xsFindNumber(xsResult, xsID_z, &number))
			orientation.z = (float)number;
	}

	dstRect.x = -billboardWidth * 0.5f;
	dstRect.y = billboardHeight * 0.5f;
	dstRect.width = billboardWidth;
	dstRect.height = -billboardHeight;

	FskEmbeddingMake(&bounds, &dstRect, &position, &orientation, &embedding);

	xsVars(1);
	if (xsIsInstanceOf(xsArg(4), xsArrayPrototype)) {
		xsIntegerValue i, numPts = xsToInteger(xsGet(xsArg(4), xsID_length));
		FskPoint2F pts[16];

		if (numPts > 16)
			numPts = 16;
		for (i = 0; i < numPts; i++) {
			xsVar(0) = xsGetAt(xsArg(4), xsInteger(i));
			if (xsIsInstanceOf(xsVar(0), xsObjectPrototype)) {
				if (xsFindNumber(xsVar(0), xsID_x, &number))
					pts[i].x = (float)number;
				if (xsFindNumber(xsVar(0), xsID_y, &number))
					pts[i].y = (float)number;
			}
		}

		FskCameraDistances(&camera, &embedding, numPts, pts, distances);

		if ((c > 5) && xsTest(xsArg(5)))
			xsResult = xsArg(5);
		else {
			xsResult = xsNew1(xsGlobal, xsID_Array, xsNumber(numPts));
			(void)xsCall0(xsResult, xsID_fill);
		}
		for (i=0; i < numPts; i++)
			xsSetAt(xsResult, xsInteger(i), xsNumber(distances[i]));
	}

	xsLeaveSandbox();
}

void Math3D_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("Math"));
	xsNewHostProperty(xsResult, xsID("slerpUnitQuaternionWXYZ"), xsNewHostFunction(Math_slerpUnitQuaternionWXYZ, 5), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("quaternionWXYZFromNormalUp"), xsNewHostFunction(Math_quaternionWXYZFromNormalUp, 3), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("cameraDistances"), xsNewHostFunction(Math_cameraDistances, 5), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
}


