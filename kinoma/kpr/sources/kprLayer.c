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
#define __FSKWINDOW_PRIV__

#include "FskImage.h"
#include "FskPort.h"
#include "FskUtilities.h"
#if FSKBITMAP_OPENGL
#include "FskGLBlit.h"
#endif

#include "kprBehavior.h"
#include "kprEffect.h"
#include "kprContent.h"
#include "kprLayer.h"
#include "kprMessage.h"
#include "kprSkin.h"
#include "kprShell.h"
#include "kprUtilities.h"

#include <math.h>

static void KprLayerDispose(void* it);
static FskBitmap KprLayerGetBitmap(void* it, FskPort port, Boolean* owned);
static KprContent KprLayerHit(void* it, SInt32 x, SInt32 y);
static void KprLayerLayered(void* it, KprLayer layer, Boolean layerIt);
static void KprLayerPlaced(void* it);
static void KprLayerReflowing(void* it, UInt32 flags);
static void KprLayerSetWindow(void* it, KprShell shell, KprStyle style);
static void KprLayerUpdate(void* it, FskPort port, FskRectangle area);

static void KprLayerAdjust(KprLayer self);
static FskErr KprLayerEnsureHitMatrix(KprLayer self);

static FskColorRGBARecord gLayerClearColor = {0, 0, 0, 0};

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprLayerInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprLayer", FskInstrumentationOffset(KprLayerRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprLayerDispatchRecord = {
	"layer",
	KprContainerActivated,
	KprContainerAdded,
	KprContainerCascade,
	KprLayerDispose,
	KprContentDraw,
	KprContainerFitHorizontally,
	KprContainerFitVertically,
	KprLayerGetBitmap,
	KprLayerHit,
	KprContentIdle,
	KprLayerInvalidated,
	KprLayerLayered,
	KprContainerMark,
	KprContainerMeasureHorizontally,
	KprContainerMeasureVertically,
	KprContainerPlace,
	KprLayerPlaced,
	KprContainerPredict,
	KprLayerReflowing,
	KprContainerRemoving,
	KprLayerSetWindow,
	KprContainerShowing,
	KprContainerShown,
	KprLayerUpdate
};

FskErr KprLayerNew(KprLayer* it,  KprCoordinates coordinates, UInt32 flags)
{
	FskErr err = kFskErrNone;
	KprLayer self;
//	FskFixed scale;
//	SInt32 width, height;
	bailIfError(FskMemPtrNewClear(sizeof(KprLayerRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprLayerInstrumentation);
	self->dispatch = &KprLayerDispatchRecord;
	self->flags = kprContainer | kprLayer | kprClip | kprVisible | kprDirty | flags;
	KprContentInitialize((KprContent)self, coordinates, NULL, NULL);
	bailIfError(FskPortNew(&self->port, NULL, NULL));
    FskInstrumentedItemSetOwner(self->port, self);
	FskPortSetGraphicsMode(self->port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
	self->corners[1].x = 1;
	self->corners[2].x = 1;
	self->corners[2].y = 1;
	self->corners[3].y = 1;
	self->opacity = 1;
	self->scale.x = 1;
	self->scale.y = 1;
	KprLayerComputeMatrix(self);
bail:
	return err;
}

void KprLayerAttach(KprLayer self, KprContent content)
{
	KprContainer container = content->container;
	KprContent previous = content->previous;
	KprContent next = content->next;
	FskRectangle bounds = &content->bounds;
	KprCoordinates coordinates = &content->coordinates;
    KprShellAdjust(content->shell);
	content->container = (KprContainer)self;
	FskInstrumentedItemSetOwner(content, self);
	content->previous = NULL;
	content->next = NULL;
	self->container = container;
	FskInstrumentedItemSetOwner(self, container);
	if (previous)
		previous->next = (KprContent)self;
	else
		container->first = (KprContent)self;
	self->previous = previous;
	if (next)
		next->previous =(KprContent) self;
	else
		container->last = (KprContent)self;
	self->next = next;
	self->bounds = *bounds;
	self->coordinates = *coordinates;
	if (!(self->coordinates.horizontal & kprWidth)) {
        self->coordinates.horizontal |= kprWidth | kprTemporary;
        self->coordinates.width = bounds->width;
	}
    if (!(self->coordinates.vertical & kprHeight)) {
        self->coordinates.vertical |= kprHeight | kprTemporary;
        self->coordinates.height = bounds->height;
	}
    (*self->dispatch->setWindow)(self, container->shell, container->style);
  	(*self->dispatch->placed)(self);
	bounds->x = 0;
	bounds->y = 0;
	coordinates->horizontal = kprLeftRight;
	coordinates->vertical = kprTopBottom;
	coordinates->left = 0;
	coordinates->width = 0;
	coordinates->right = 0;
	coordinates->top = 0;
	coordinates->height = 0;
	coordinates->bottom = 0;
	self->first = content;
	self->last = content;
	self->flags |= kprBlocking;
  	(*self->dispatch->layered)(self, self, true);
}

FskErr KprLayerCapture(KprLayer self, KprContent content, FskBitmap* bitmap)
{
	FskRectangleRecord bounds;
	KprLayerAttach(self, content);
	KprLayerAdjust(self);
	FskPortSetOrigin(self->port, 0, 0);
	FskRectangleSet(&bounds, 0, 0, self->bounds.width, self->bounds.height);
    FskPortBeginDrawing(self->port, (self->flags & kprNoAlpha) ? NULL : &gLayerClearColor);
	(*content->dispatch->update)(content, self->port, &bounds);
    FskPortEndDrawing(self->port);
	FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedLayerBitmapRelease, self);
	*bitmap = self->bitmap;
	FskPortSetBitmap(self->port, NULL);
	self->bitmap = NULL;
	self->flags &= ~kprDirty;
	KprLayerDetach(self);
	return kFskErrNone;
}

KprContent KprLayerDetach(KprLayer self)
{
	KprContent content = self->first;
	KprContainer container = self->container;
	KprContent previous = self->previous;
	KprContent next = self->next;
	FskRectangle bounds = &self->bounds;
	KprCoordinates coordinates = &self->coordinates;
	self->flags &= ~kprBlocking;
	self->first = NULL;
	self->last = NULL;
    (*self->dispatch->setWindow)(self, NULL, NULL);
	self->container = NULL;
	FskInstrumentedItemClearOwner(self);
	self->previous = NULL;
	self->next = NULL;
	content->container = container;
	FskInstrumentedItemSetOwner(content, container);
	if (previous)
		previous->next = content;
	else
		container->first = content;
	content->previous = previous;
	if (next)
		next->previous = content;
	else
		container->last = content;
	content->next = next;
	content->bounds = *bounds;
	content->coordinates = *coordinates;
	if (content->coordinates.horizontal & kprTemporary) {
        content->coordinates.horizontal &= ~(kprWidth | kprTemporary);
        content->coordinates.width = 0;
        //KprContentReflow(content, kprWidthChanged);
    }
	if (content->coordinates.vertical & kprTemporary) {
        content->coordinates.vertical &= ~(kprHeight | kprTemporary);
        content->coordinates.height = 0;
		//KprContentReflow(content, kprHeightChanged);
    }
  	(*content->dispatch->layered)(content, self, false);
	KprContentClose((KprContent)self);
	KprContentInvalidate(content);
    return content;
}

Boolean KprLayerGLContextLost(KprLayer self)
{
	if (self->bitmap && FskBitmapIsOpenGLDestinationAccelerated(self->bitmap)) {
		FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedLayerBitmapDispose, self);
		FskPortSetBitmap(self->port, NULL);
		FskBitmapDispose(self->bitmap);
		self->bitmap = NULL;
		self->flags |= kprPlaced;
		return true;
	}
	return false;
}

void KprLayerMatrixChanged(KprLayer self)
{
	if (!(self->flags & kprPort)) {
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprMatrixChanged | kprPlaced);
	}
}

void KprLayerSetEffect(KprLayer self, KprEffect effect)
{
	KprAssetUnbind(self->effect);
	self->effect = effect;
    KprAssetBind(self->effect);
	KprContentInvalidate((KprContent)self);
}

/* LAYER DISPATCH */

void KprLayerDispose(void* it)
{
	KprLayer self = it;
	if (self->bitmap) {
		FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedLayerBitmapDispose, self);
		FskPortSetBitmap(self->port, NULL);
		FskBitmapDispose(self->bitmap);
	}
	FskPortDispose(self->port);
	KprAssetUnbind(self->effect);
	FskMemPtrDispose(self->hitMatrix);
	KprContainerDispose(it);
}

FskBitmap KprLayerGetBitmap(void* it, FskPort port, Boolean* owned)
{
	FskErr err = kFskErrNone;
	KprLayer self = it;
	FskBitmap result = NULL;
	if (self->bitmap) {
		if (self->effect && self->effect->compound) {
			*owned = false;
			bailIfError(KprEffectApply(self->effect, self->bitmap, port, &result));
		}
		else {
			*owned = true;
			result = self->bitmap;
		}
	}
bail:
	return result;
}

KprContent KprLayerHit(void* it, SInt32 x, SInt32 y)
{
	KprLayer self = it;
	KprContent content;
	KprContent result;
	if (self->flags & kprVisible) {
		if (!(self->flags & kprBlocking)) {
			FskDCoordinate *m, u, v, w;
			if (KprLayerEnsureHitMatrix(self))
				return NULL;
			m = self->hitMatrix;
			u = (FskDCoordinate)x;
			v = (FskDCoordinate)y;
			w = m[2] * u + m[5] * v + m[8];
			x = (SInt32)floor((m[0] * u + m[3] * v + m[6]) / w);
			y = (SInt32)floor((m[1] * u + m[4] * v + m[7]) / w);
			if ((0 <= x) && (0 <= y) && (x < self->bounds.width) && (y < self->bounds.height)) {
				content = self->last;
				while (content) {
					result = (*content->dispatch->hit)(content, x - content->bounds.x, y - content->bounds.y);
					if (result)
						return result;
					content = content->previous;
				}
				if (self->flags & kprActive)
					return (KprContent)self;
			}
		}
		else
			return (KprContent)self;
	}
	return NULL;
}

void KprLayerInvalidated(void* it, FskRectangle area)
{
	KprLayer self = it;
	if (self->flags & kprVisible) {
		KprContainer container = self->container;
		if (container) {
			FskRectangleRecord bounds;
			FskDCoordinate *m = &self->matrix[0][0];
			if (area)
				self->flags |= kprDirty;
			else {
				FskRectangleSet(&bounds, 0, 0, self->bounds.width, self->bounds.height);
				area = &bounds;
			}
			if (self->effect)
				KprEffectExtend(self->effect, area);
			if (!m[1] && !m[2] && !m[3] && !m[5]) {
				FskDCoordinate left = ((FskDCoordinate)area->x * m[0]) + m[6];
				FskDCoordinate top = ((FskDCoordinate)area->y * m[4]) + m[7];
				FskDCoordinate right = (((FskDCoordinate)area->x + (FskDCoordinate)area->width) * m[0]) + m[6];
				FskDCoordinate bottom = (((FskDCoordinate)area->y + (FskDCoordinate)area->height) * m[4]) + m[7];
				FskRectangleSet(area, (SInt32)(left), (SInt32)(top), (SInt32)ceil(right - left), (SInt32)ceil(bottom - top));
				FskRectangleOffset(area, self->bounds.x, self->bounds.y);
				(*container->dispatch->invalidated)(container, area);
			}
			else {
				// @@ compute transformed bounds...
				KprShell shell = self->shell;
				if (shell) {
					FskRectangleSet(area, 0, 0, shell->bounds.width, shell->bounds.height);
					(*shell->dispatch->invalidated)(shell, area);
				}
			}
		}
	}
}

void KprLayerLayered(void* it, KprLayer layer, Boolean layerIt)
{
	KprLayer self = it;
	if (self == layer)
		KprContainerLayered(it, layer, layerIt);
}

void KprLayerPlaced(void* it)
{
	KprLayer self = it;
	if (self->flags & kprMatrixChanged)
		KprLayerComputeMatrix(self);
	KprContainerPlaced(it);
	KprLayerAdjust(self);
}

void KprLayerReflowing(void* it, UInt32 flags)
{
	KprLayer self = it;
	self->flags |= kprDirty;
	KprContainerReflowing(it, flags);
}

void KprLayerSetWindow(void* it, KprShell shell, KprStyle style)
{
	KprLayer self = it;
	KprContainerSetWindow(it, shell, style);
	if (!shell && self->bitmap && (!(self->flags & kprFrozen))) {
		FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedLayerBitmapDispose, self);
		FskPortSetBitmap(self->port, NULL);
		FskBitmapDispose(self->bitmap);
		self->bitmap = NULL;
	}
}

void KprLayerUpdate(void* it, FskPort port, FskRectangle area UNUSED)
{
	KprLayer self = it;
	FskRectangleRecord srcRect, dstRect;
	if (self->bitmap) {
		if ((self->flags & kprDirty) && (!(self->flags & kprFrozen))) {
			KprContent content = self->first;
			FskPortSetOrigin(self->port, 0, 0);
			FskRectangleSet(&srcRect, 0, 0, self->bounds.width, self->bounds.height);
            FskPortBeginDrawing(self->port, (self->flags & kprNoAlpha) ? NULL : &gLayerClearColor);
			while (content) {
				(*content->dispatch->update)(content, self->port, &srcRect);
				content = content->next;
			}
            FskPortEndDrawing(self->port);
			self->flags &= ~kprDirty;
			FskInstrumentedItemSendMessageDebug(self, kprInstrumentedLayerUpdate, NULL);
		}
		if (port && (self->flags & kprVisible)) {
			FskPortOffsetOrigin(port, self->bounds.x, self->bounds.y);
			FskRectangleSet(&srcRect, 0, 0, self->bounds.width, self->bounds.height);
			FskRectangleSet(&dstRect, 0, 0, self->bounds.width, self->bounds.height);
			FskPortRectScale(self->port, &srcRect);
			KprLayerBlit(self, port, self->bitmap, &srcRect, &dstRect);
			FskPortOffsetOrigin(port, -self->bounds.x, -self->bounds.y);
		}
	}
}

/* LAYER IMPLEMENTATION */

void KprLayerAdjust(KprLayer self)
{
	FskFixed scale;
	SInt32 width, height;
	FskRectangleRecord bounds;
	FskErr err = kFskErrNone;
	FskBitmapFormatEnum pixelFormat;
	scale = FskPortScaleGet(self->shell->port);
	width = (self->bounds.width * scale) >> 16;
	height = (self->bounds.height * scale) >> 16;
	if (self->bitmap)
		FskBitmapGetBounds(self->bitmap, &bounds);
	else
		FskRectangleSetEmpty(&bounds);
	if ((width != bounds.width) || (height != bounds.height)) {
		if (self->bitmap) {
			FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedLayerBitmapDispose, self);
			FskPortSetBitmap(self->port, NULL);
			FskBitmapDispose(self->bitmap);
			self->bitmap = NULL;
		}
		FskPortScaleSet(self->port, scale);
 	#if FSKBITMAP_OPENGL
		if (gShell->window->usingGL && !(self->flags & kprNoAcceleration))	
			pixelFormat = (self->flags & kprNoAlpha) ? kFskBitmapFormatGLRGB : kFskBitmapFormatGLRGBA;
		else
	#endif
			pixelFormat = (self->flags & kprNoAlpha) ? kFskBitmapFormatDefaultNoAlpha : kFskBitmapFormatDefaultAlpha;
		bailIfError(FskBitmapNew(width, height, pixelFormat, &self->bitmap));
		if (!(self->flags & kprNoAlpha)) {
			FskBitmapSetHasAlpha(self->bitmap, true);
			FskBitmapSetAlphaIsPremultiplied(self->bitmap, true);
		}
	#if FSKBITMAP_OPENGL
		if (gShell->window->usingGL && (self->flags & kprNoAcceleration))	
			FskBitmapSetOpenGLSourceAccelerated(self->bitmap, true); // ??
	#endif
		FskPortSetBitmap(self->port, self->bitmap);
		FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedLayerBitmapNew, self);
	bail:
		self->flags |= kprDirty;
		self->error = err;
	}
}

void KprLayerBlit(void* it, FskPort port, FskBitmap srcBits, FskRectangle srcRect, FskRectangle dstRect)
{
	KprLayer self = it;
	FskDCoordinate *m = &self->matrix[0][0];
	FskErr err = kFskErrNone;
	FskRectangleRecord bounds;
	FskBitmap dstBits = NULL;
#if CHECK_UNACCELERATED_BITMAPS
	if (FskBitmapIsOpenGLDestinationAccelerated(port->bits) && !srcBits->accelerate)
		return;
#endif
	if (self->effect && self->effect->compound) {
		bounds = *dstRect;
    	FskPortRectScale(port, &bounds);
		if ((srcRect->width == bounds.width) && (srcRect->height == bounds.height) /* no scale */
				&& (m[0] == 1) && !m[1] && !m[2] && !m[3] && (m[4] == 1) && !m[5]) { /* translation matrix */
			FskPointRecord dstPoint = {dstRect->x + m[6], dstRect->y + m[7]};
			FskPortEffectApply(port, srcBits, srcRect, &dstPoint, self->effect->compound);
			return;
		}
		err = KprEffectApply(self->effect, srcBits, port, &dstBits);
		if (err == kFskErrNone)
			srcBits = dstBits;
	}
	if (!m[1] && !m[2] && !m[3] && !m[5]) {
		FskRectangleRecord tmpRect;
		FskDCoordinate left = ((FskDCoordinate)dstRect->x * m[0]) + m[6];
		FskDCoordinate top = ((FskDCoordinate)dstRect->y * m[4]) + m[7];
		FskDCoordinate right = (((FskDCoordinate)dstRect->x + (FskDCoordinate)dstRect->width) * m[0]) + m[6];
		FskDCoordinate bottom = (((FskDCoordinate)dstRect->y + (FskDCoordinate)dstRect->height) * m[4]) + m[7];
		if (self->flags & kprSubPixel) {
			if (self->opacity != 1) {
				FskGraphicsModeParametersRecord param;
				param.dataSize = sizeof(FskGraphicsModeParametersRecord);
				param.blendLevel = (SInt32)(self->opacity * 255);
				FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, &param);
				FskPortBitmapDrawSubpixel(port, srcBits, srcRect, left, top, right - left, bottom - top);
				FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
			}
			else {
				FskPortBitmapDrawSubpixel(port, srcBits, srcRect, left, top, right - left, bottom - top);
			}
		}
		else {
			FskRectangleSet(&tmpRect, (SInt32)(left), (SInt32)(top), (SInt32)(right - left), (SInt32)(bottom - top));
			if (self->opacity != 1) {
				FskGraphicsModeParametersRecord param;
				param.dataSize = sizeof(FskGraphicsModeParametersRecord);
				param.blendLevel = (SInt32)(self->opacity * 255);
				FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, &param);
				FskPortBitmapDraw(port, srcBits, srcRect, &tmpRect);
				FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
			}
			else
				FskPortBitmapDraw(port, srcBits, srcRect, &tmpRect);
		}
	}
	else {
		SInt32 i, j;
		float transform[3][3];
		double rw, rh;
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				transform[i][j] = (float)self->matrix[i][j];
		rw = (double)dstRect->width / (double)srcRect->width;
		rh = (double)dstRect->height / (double)srcRect->height;
		transform[0][0] *= (float)rw;
		transform[0][1] *= (float)rw;
		transform[0][2] *= (float)rw;
		transform[1][0] *= (float)rh;
		transform[1][1] *= (float)rh;
		transform[1][2] *= (float)rh;
		if (self->opacity != 1) {
			FskGraphicsModeParametersRecord param;
			param.dataSize = sizeof(FskGraphicsModeParametersRecord);
			param.blendLevel = (SInt32)(self->opacity * 255);
			FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, &param);
			FskPortBitmapProject(port, srcBits, srcRect, transform);
			FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
		}
		else
			FskPortBitmapProject(port, srcBits, srcRect, transform);
	}
	FskPortReleaseTempEffectBitmap(port, dstBits);
}

void KprLayerComputeMatrix(KprLayer self)
{
	static FskDPoint2D defaultCorners[4] = {{0,0},{1,0},{1,1},{0,1}};
	FskDCoordinate buffer0[3][3], buffer1[3][3];
	FskDCoordinate *m0 = &buffer0[0][0], *m1 = &buffer1[0][0], *m2;
	FskDCoordinate r, x, y;

	FskDIdentityMatrix(m0, 3, 3);
	m0[6] = -self->origin.x;
	m0[7] = -self->origin.y;

	if (memcmp(self->corners, defaultCorners, sizeof(defaultCorners))) {
		SInt32 width = self->bounds.width, height = self->bounds.height;
		FskDCoordinate from[8], to[8];
		SInt32 i, j = 0;
		FskDCoordinate buffer2[3][3];
		for (i = 0; i < 4; i++) {
			from[j] = defaultCorners[i].x * width;
			to[j] = self->corners[i].x * width;
			j++;
			from[j] = defaultCorners[i].y * height;
			to[j] = self->corners[i].y * height;
			j++;
		}
		m2 = &buffer2[0][0];
		if (FskDProjectionFromCorrespondences3x3(from, to, m2)) {
			FskDLinearTransform(m2, m0, m1, 3, 3, 3);
			m2 = m0;
			m0 = m1;
			m1 = m2;
		}
	}

	x = self->scale.x;
	y = self->scale.y;
	if ((x != 1.0) || (y != 1.0)) {
		m1[0] = m0[0] * x;
		m1[1] = m0[1] * y;
		m1[2] = m0[2];
		m1[3] = m0[3] * x;
		m1[4] = m0[4] * y;
		m1[5] = m0[5];
		m1[6] = m0[6] * x;
		m1[7] = m0[7] * y;
		m1[8] = m0[8];
		m2 = m0;
		m0 = m1;
		m1 = m2;
	}

	r = (self->rotation / 180) * M_PI;
	if (r) {
		x = cos(r);
		y = sin(r);
		m1[0] = m0[0] * x + m0[1] * -y;
		m1[1] = m0[0] * y + m0[1] * x ;
		m1[2] = m0[2];
		m1[3] = m0[3] * x + m0[4] * -y;
		m1[4] = m0[3] * y + m0[4] * x;
		m1[5] = m0[5];
		m1[6] = m0[6] * x + m0[7] * -y;
		m1[7] = m0[6] * y + m0[7] * x;
		m1[8] = m0[8];
		m2 = m0;
		m0 = m1;
		m1 = m2;
	}
	r = (self->skew.x / 180) * M_PI;
	if (r) {
		FskDCoordinate buffer2[3][3];
		m2 = &buffer2[0][0];
		FskDIdentityMatrix(m2, 3, 3);
		m2[3] = tan(r);
		FskDLinearTransform(m0, m2, m1, 3, 3, 3);
		m2 = m0;
		m0 = m1;
		m1 = m2;
	}
	r = (self->skew.y / 180) * M_PI;
	if (r) {
		FskDCoordinate buffer2[3][3];
		m2 = &buffer2[0][0];
		FskDIdentityMatrix(m2, 3, 3);
		m2[1] = tan(r);
		FskDLinearTransform(m0, m2, m1, 3, 3, 3);
		m2 = m0;
		m0 = m1;
		m1 = m2;
	}
	x = self->translation.x + self->origin.x;
	y = self->translation.y + self->origin.y;
	if (x || y) {
		m1[0] = m0[0] + m0[2] * x;
		m1[1] = m0[1] + m0[2] * y;
		m1[2] = m0[2];
		m1[3] = m0[3] + m0[5] * x;
		m1[4] = m0[4] + m0[5] * y;
		m1[5] = m0[5];
		m1[6] = m0[6] + m0[8] * x;
		m1[7] = m0[7] + m0[8] * y;
		m1[8] = m0[8];
		m0 = m1;
	}
	FskDCopyMatrix(m0, self->matrix[0], 3, 3);
	if (self->hitMatrix)
		FskDInvertMatrix(m0, 3, 3, self->hitMatrix);
	self->flags &= ~kprMatrixChanged;
}

FskErr KprLayerEnsureHitMatrix(KprLayer self)
{
	FskErr err = kFskErrNone;
	FskDCoordinate *m = self->hitMatrix;
	if (!m) {
		bailIfError(FskMemPtrNew(sizeof(FskDCoordinate) * 9, &m));
		FskDInvertMatrix(self->matrix[0], 3, 3, m);
		self->hitMatrix = m;
	}
bail:
	return err;
}

/* ECMASCRIPT */

void KPR_Layer(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	UInt32 flags = 0;
	KprLayer self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	if ((c > 1) && xsTest(xsArg(1)))
		flags |= kprNoAlpha;
	if ((c > 2) && xsTest(xsArg(2)))
		flags |= kprNoAcceleration;
	KprLayerNew(&self, &coordinates, flags);
	kprContentConstructor(KPR_Layer);
}

void KPR_layer_get_blocking(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprBlocking) ? xsTrue : xsFalse;
}

void KPR_layer_get_corners(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	xsIntegerValue i;
	xsVars(1);
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	xsSet(xsResult, xsID_length, xsInteger(4));
	for (i = 0; i < 4; i++) {
		xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsVar(0), xsID_x, xsNumber(self->corners[i].x), xsDefault, xsDontScript);
		xsNewHostProperty(xsVar(0), xsID_y, xsNumber(self->corners[i].y), xsDefault, xsDontScript);
		xsSetAt(xsResult, xsInteger(i), xsVar(0));
	}
}

void KPR_layer_get_effect(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	xsResult = kprVolatileGetter(self->effect, xsID_effect);
}

void KPR_layer_get_opacity(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->opacity);
}

void KPR_layer_get_origin(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsNumber(self->origin.x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsNumber(self->origin.y), xsDefault, xsDontScript);
}

void KPR_layer_get_rotation(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->rotation);
}

void KPR_layer_get_scale(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsNumber(self->scale.x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsNumber(self->scale.y), xsDefault, xsDontScript);
}

void KPR_layer_get_skew(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsNumber(self->skew.x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsNumber(self->skew.y), xsDefault, xsDontScript);
}

void KPR_layer_get_subPixel(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprSubPixel) ? xsTrue : xsFalse;
}

void KPR_layer_get_translation(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsNumber(self->translation.x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsNumber(self->translation.y), xsDefault, xsDontScript);
}

void KPR_layer_set_blocking(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprBlocking;
	else
		self->flags &= ~kprBlocking;
}

void KPR_layer_set_corners(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	Boolean flag = false;
	xsEnterSandbox();
	if (xsIsInstanceOf(xsArg(0), xsArrayPrototype)) {
		if (xsToInteger(xsGet(xsArg(0), xsID_length)) == 4) {
			xsIntegerValue i;
			xsVars(1);
			for (i = 0; i < 4; i++) {
				xsVar(0) = xsGetAt(xsArg(0), xsInteger(i));
				if (xsIsInstanceOf(xsVar(0), xsObjectPrototype)) {
					flag |= xsFindNumber(xsVar(0), xsID_x, &(self->corners[i].x));
					flag |= xsFindNumber(xsVar(0), xsID_y, &(self->corners[i].y));
				}
			}
		}
	}
	xsLeaveSandbox();
	if (flag)
		KprLayerMatrixChanged(self);
}

void KPR_layer_set_effect(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	KprEffect effect = NULL;
	if (xsTest(xsArg(0)))
		effect = kprGetHostData(xsArg(0), it, effect);
	KprLayerSetEffect(self, effect);
}

void KPR_layer_set_opacity(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	self->opacity = xsToNumber(xsArg(0));
	if (!(self->flags & kprPort))
		KprContentInvalidate((KprContent)self);
}

void KPR_layer_set_origin(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	Boolean flag = false;
	xsEnterSandbox();
	flag |= xsFindNumber(xsArg(0), xsID_x, &self->origin.x);
	flag |= xsFindNumber(xsArg(0), xsID_y, &self->origin.y);
	xsLeaveSandbox();
	if (flag)
		KprLayerMatrixChanged(self);
}

void KPR_layer_set_rotation(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	self->rotation = xsToNumber(xsArg(0));
	KprLayerMatrixChanged(self);
}

void KPR_layer_set_scale(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	Boolean flag = false;
	xsEnterSandbox();
	flag |= xsFindNumber(xsArg(0), xsID_x, &self->scale.x);
	flag |= xsFindNumber(xsArg(0), xsID_y, &self->scale.y);
	xsLeaveSandbox();
	if (flag)
		KprLayerMatrixChanged(self);
}

void KPR_layer_set_skew(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	Boolean flag = false;
	xsEnterSandbox();
	flag |= xsFindNumber(xsArg(0), xsID_x, &self->skew.x);
	flag |= xsFindNumber(xsArg(0), xsID_y, &self->skew.y);
	if (flag)
		KprLayerMatrixChanged(self);
}

void KPR_layer_set_subPixel(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprSubPixel;
	else
		self->flags &= ~kprSubPixel;
}

void KPR_layer_set_translation(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	Boolean flag = false;
	xsEnterSandbox();
	flag |= xsFindNumber(xsArg(0), xsID_x, &self->translation.x);
	flag |= xsFindNumber(xsArg(0), xsID_y, &self->translation.y);
	xsLeaveSandbox();
	if (flag)
		KprLayerMatrixChanged(self);
}

void KPR_layer_attach(xsMachine *the)
{
	KprLayer self = kprGetHostData(xsThis, this, layer);
	KprContent content = kprGetHostData(xsArg(0), content, content);
	xsAssert(self->first == NULL);
	xsAssert(content->container != NULL);
	xsAssert(content->shell != NULL);
	KprLayerAttach(self, content);
}

void KPR_layer_capture(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprLayer self = kprGetHostData(xsThis, this, layer);
	KprContent content = kprGetHostData(xsArg(0), content, content);
	FskRectangleRecord bounds, crop;
	xsAssert(self->first == NULL);
	xsAssert(content->container != NULL);
	self->flags |= kprNoAcceleration;
	KprLayerAttach(self, content);
	bounds = self->bounds;
	if (c == 5) {
		crop.x = xsToInteger(xsArg(1));
		crop.y = xsToInteger(xsArg(2));
		crop.width = xsToInteger(xsArg(3));
		crop.height = xsToInteger(xsArg(4));
		FskRectangleIntersect(&bounds, &crop, &crop);
		self->bounds.width = crop.width;
		self->bounds.height = crop.height;
	}
	else
		FskRectangleSet(&crop, 0, 0, bounds.width, bounds.height);
	KprLayerAdjust(self);
	FskPortSetOrigin(self->port, -crop.x, -crop.y);
    FskPortBeginDrawing(self->port, (self->flags & kprNoAlpha) ? NULL : &gLayerClearColor);
	(*content->dispatch->update)(content, self->port, &crop);
    FskPortEndDrawing(self->port);
	self->flags |= kprFrozen;
	self->bounds = bounds;
	KprLayerDetach(self);
	self->bounds = crop;
	self->coordinates.horizontal = kprLeft | kprWidth;
	self->coordinates.left = crop.x;
	self->coordinates.right = 0;
	self->coordinates.width = crop.width;
	self->coordinates.vertical = kprTop | kprHeight;
	self->coordinates.top = crop.y;
	self->coordinates.bottom = 0;
	self->coordinates.height = crop.height;
}

#define ASYNC_COMPRESS 1

#if ASYNC_COMPRESS
typedef struct {
    FskImageCompress comp;
    FskBitmap bits;
    Boolean cancelled;
    KprMessage message;
} kprLayerAsyncCompressRecord, *kprLayerAsyncCompress;

static void kprLayerAsyncComplete(FskImageCompress comp, void *refcon, FskErr result, void *data, UInt32 dataSize);
static void kprLayerAsyncCallback(KprMessage message, void *refcon);
static void kprLayerAsyncCallbackDispose(void *refcon);
static void kprLayerAsyncCancelled(void *refcon, void *a1, void *a2, void *a3);
static void kprLayerAsyncResume(void *message, void *a1, void *a2, void *a3);

void kprLayerAsyncComplete(FskImageCompress comp, void *refcon, FskErr result, void *data, UInt32 dataSize)
{
    kprLayerAsyncCompress compRec = refcon;
    KprMessage message = compRec->message;
    if (compRec->cancelled) {
        printf("Compress cancelled %p\n", compRec);
		FskThreadPostCallback(KprShellGetThread(gShell), kprLayerAsyncCancelled, compRec, NULL, NULL, NULL);
        return;
    }
    if (kFskErrNone != result) {
        printf("Compress failed %p\n", compRec);
        message->error = result;
    }
    else {
        printf("Compress succesfull %p\n", compRec);
        message->response.body = data;
        message->response.size = dataSize;
    }
	FskThreadPostCallback(KprShellGetThread(gShell), kprLayerAsyncResume, message, NULL, NULL, NULL);
}

void kprLayerAsyncCallback(KprMessage message, void* refcon)
{
    kprLayerAsyncCompress compRec = refcon;
	// no way to cancel compression?
    compRec->cancelled = true;
    return;
}

void kprLayerAsyncCallbackDispose(void *refcon)
{
	// wait for compression to be done
    kprLayerAsyncCompress compRec = refcon;
    if (!compRec->cancelled) {
        printf("Compress disposed %p\n", compRec);
        FskImageCompressDispose(compRec->comp);
        FskBitmapDispose(compRec->bits);
        FskMemPtrDispose(compRec);
    }
    return;
}

void kprLayerAsyncCancelled(void *refcon, void *a1, void *a2, void *a3)
{
    kprLayerAsyncCompress compRec = refcon;
   compRec->cancelled = false;
    kprLayerAsyncCallbackDispose(refcon);
}

void kprLayerAsyncResume(void *message, void *a1, void *a2, void *a3)
{
    KprMessageResume((KprMessage)message);
}

#endif

void KPR_layer_setResponseJPEG(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprLayer self = kprGetHostData(xsThis, this, layer);
	KprMessage message = kprGetHostData(xsArg(0), message, message);
	FskBitmap bitmap = NULL;
	Boolean owned = false;
	FskRectangleRecord bounds;
	FskImageCompress comp = NULL;
	FskMediaPropertyValueRecord property;
#if ASYNC_COMPRESS
    kprLayerAsyncCompress compRec;
#endif
	if (!self->shell) {
		message->error = kFskErrOutOfSequence;
		return;
	}
	KprLayerAdjust(self);
	KprLayerUpdate(self, NULL, NULL);
	bitmap = KprLayerGetBitmap(self, NULL, &owned);
	FskPortDispose(self->port);
	self->port = NULL;
	self->bitmap = NULL;
    owned = false;
	FskBitmapGetBounds(bitmap, &bounds);
	bailIfError(FskImageCompressNew(&comp, 0, "image/jpeg", bounds.width, bounds.height));
	property.type = kFskMediaPropertyTypeFloat;
	property.value.number = 0.6;
	FskImageCompressSetProperty(comp, kFskMediaPropertyQuality, &property);
#if ASYNC_COMPRESS
    bailIfError(FskMemPtrNewClear(sizeof(kprLayerAsyncCompressRecord), &compRec));
    compRec->comp = comp;
    compRec->message = message;
    compRec->bits = bitmap;

    KprMessageSuspend(message, kprLayerAsyncCallback, kprLayerAsyncCallbackDispose, compRec);
 	printf("Compress begin %p\n", compRec);
    bailIfError(FskImageCompressFrame(comp, bitmap, &message->response.body, &message->response.size, NULL, NULL, NULL, kprLayerAsyncComplete, compRec));
    owned = true;
    comp = NULL;
#else
	bailIfError(FskImageCompressFrame(comp, bitmap, &message->response.body, &message->response.size, NULL, NULL, NULL, NULL, NULL));
#endif
bail:
	FskImageCompressDispose(comp);
	if (!owned)
		FskBitmapDispose(bitmap);
	xsThrowIfFskErr(err);
}

void KPR_layer_detach(xsMachine *the)
{
	KprLayer self = kprGetHostData(xsThis, this, layer);
	KprContent content;
	xsAssert(self->first != NULL);
	xsAssert(self->first == self->last);
    content = KprLayerDetach(self);
    xsResult = kprContentGetter(content);
}

void KPR_layer_transform(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprLayer self = xsGetHostData(xsThis);
	SInt32 x = xsToInteger(xsArg(0));
	SInt32 y = xsToInteger(xsArg(1));
	FskDCoordinate *m, u, v, w;
	if ((c > 2) && xsTest(xsArg(2))) {
		xsThrowIfFskErr(KprLayerEnsureHitMatrix(self));
		m = self->hitMatrix;
	}
	else
		m = &self->matrix[0][0];
	KprContentFromWindowCoordinates((KprContent)self, x, y, &x, &y);
	u = (FskDCoordinate)x;
	v = (FskDCoordinate)y;
	w = m[2] * u + m[5] * v + m[8];
	x = (SInt32)floor((m[0] * u + m[3] * v + m[6]) / w);
	y = (SInt32)floor((m[1] * u + m[4] * v + m[7]) / w);
	KprContentToWindowCoordinates((KprContent)self, x, y, &x, &y);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsInteger(x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsInteger(y), xsDefault, xsDontScript);
}

void KPR_layer_transformTouch(xsMachine *the)
{
	KprLayer self = xsGetHostData(xsThis);
	SInt32 x = xsToInteger(xsArg(0));
	SInt32 y = xsToInteger(xsArg(1));
	FskDCoordinate *m, u, v, w;
	KprContentFromWindowCoordinates((KprContent)self, x, y, &x, &y);
	xsThrowIfFskErr(KprLayerEnsureHitMatrix(self));
	m = self->hitMatrix;
	u = (FskDCoordinate)x;
	v = (FskDCoordinate)y;
	w = m[2] * u + m[5] * v + m[8];
	x = (SInt32)floor((m[0] * u + m[3] * v + m[6]) / w);
	y = (SInt32)floor((m[1] * u + m[4] * v + m[7]) / w);
	KprContentToWindowCoordinates((KprContent)self, x, y, &x, &y);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsInteger(x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsInteger(y), xsDefault, xsDontScript);
}
