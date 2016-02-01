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
#include <math.h>

#define __FSKBITMAP_PRIV__
#define __FSKPORT_PRIV__
#define __FSKWINDOW_PRIV__

#include "FskImage.h"
#include "FskPixelOps.h"
#include "FskText.h"
#include "FskTextConvert.h"
#include "FskUtilities.h"

#include "kprContent.h"
#include "kprEffect.h"
#include "kprHandler.h"
#include "kprLayer.h"
#include "kprSkin.h"
#include "kprURL.h"
#include "kprUtilities.h"
#include "kprShell.h"

/* ASSET */

FskErr KprAssetNew(KprAsset *it, UInt32 size, KprContext context, KprAsset *first, KprAssetDisposeProc dispose)
{
	FskErr err = kFskErrNone;
	KprAsset self;
	bailIfError(FskMemPtrNewClear(size, it));
	self = *it;
	self->context = context;
	self->next = *first;
	*first = self;
	self->dispose = dispose;
bail:
	return err;
}

void KprAssetDispose(void* it)
{
	if (it) {
		KprAsset self = it;
		if (!self->context && !self->the && !self->usage)
			(*self->dispose)(self);
	}
}

void KprAssetBind(void* it)
{
	if (it) {
		KprAsset self = it;
		self->usage++;
		FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedAssetBind, self);
	}
}

void KprAssetUnbind(void* it)
{
	if (it) {
		KprAsset self = it;
		self->usage--;
		FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedAssetUnbind, self);
		KprAssetDispose(self);
	}
}

void KprAssetsPurge(KprAsset* first, Boolean flag)
{
	KprAsset walker;
	while ((walker = *first)) {
		KprAsset next = walker->next;
		if (!walker->the && !walker->usage) {
			(*walker->dispose)(walker);
			*first = next;
		}
		else if (flag) {
			walker->context = NULL;
#if SUPPORT_INSTRUMENTATION
			FskInstrumentedItemClearOwner(walker);
#endif
			walker->next = NULL;
			*first = next;
		}
		else
			first = &walker->next;
	}
}

/* TEXTURE */

static void KprTextureDispose(void* it);
static void KprTexturesPurge(KprTexture keep);
static UInt32 gTextureCacheOffset = 0;
static UInt32 gTextureCacheSeed = 0;
static UInt32 gTextureCacheSize = 0;

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprTextureInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprTexture", FskInstrumentationOffset(KprTextureRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprTextureNew(KprTexture *it, KprContext context, char* base, char* url, char* mime, KprContent content, FskFixed scale)
{
	FskErr err = kFskErrNone;
	KprTexture self;
	if (!gTextureCacheSize)
		gTextureCacheSize = KprEnvironmentGetUInt32("textureCacheSize", 1024*1024*2);
	bailIfError(KprAssetNew((KprAsset *)it, sizeof(KprTextureRecord), context, &context->firstTexture, KprTextureDispose));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprTextureInstrumentation);
#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemSetOwner(self, context);
#endif
	if (base && url)
		bailIfError(KprURLMerge(base, url, &self->url));
	if (mime) {
		self->mime = FskStrDoCopy(mime);
		bailIfNULL(self->mime);
	}
	if (content) {
		self->content = content;
		content->container = (KprContainer)context;
	}
	self->scale = scale;
bail:
	return err;
}

void KprTextureDispose(void* it)
{
	KprTexture self = it;
	KprTexturePurge(self);
	KprAssetUnbind(self->effect);
	if (self->content)
		KprContentClose(self->content);
	FskMemPtrDispose(self->mime);
	FskMemPtrDispose(self->url);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

FskBitmap KprTextureGetBitmap(KprTexture self, FskPort port, Boolean* owned)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFileMapping map = NULL;
	FskBitmap result = NULL;
	if (!self->bitmap) {
		FskBitmap bitmap;
		if (self->url) {
			unsigned char *data;
			FskInt64 size;
			bailIfError(KprURLToPath(self->url, &path));
			err = FskFileMap(path, &data, &size, 0, &map);
            bailIfErrorWithDiagnostic(err, self->context->the, "Error %s opening texture %s\n", FskInstrumentationGetErrorString(err), FskInstrumentationCleanPath(path));
			if (!self->mime) {
				err = FskImageDecompressSniffForMIME(data, (UInt32)size, NULL, self->url, &self->mime);
                bailIfErrorWithDiagnostic(err, self->context->the, "Unrecognized texture image type %s. Error %s.\n", path, FskInstrumentationGetErrorString(err));
			}
			err = FskImageDecompressDataWithOrientation(data, (UInt32)size, self->mime, NULL, 0, 0, NULL, NULL, &bitmap);
            bailIfErrorWithDiagnostic(err, self->context->the, "Error %s decompressing texture %s\n", FskInstrumentationGetErrorString(err), FskInstrumentationCleanPath(path));
			KprTextureSetBitmap(self, bitmap);
		}
		else if (self->content) {
            self->content->container = NULL;
			bitmap = (*self->content->dispatch->getBitmap)(self->content, port, owned);
            self->content->container = (KprContainer)self->context;
            if (!bitmap) {
//				err = kFskErrInvalidParameter;
				goto bail;
			}
			if (*owned) {
				FskRectangleRecord bounds;
				FskBitmap tmp = NULL;
				Boolean flag;
				FskBitmapGetBounds(bitmap, &bounds);
				bailIfError(FskBitmapNew(bounds.width, bounds.height, bitmap->pixelFormat, &tmp));
				FskBitmapGetHasAlpha(bitmap, &flag);
				FskBitmapSetHasAlpha(tmp, flag);
				FskBitmapGetAlphaIsPremultiplied(bitmap, &flag);
				FskBitmapSetAlphaIsPremultiplied(tmp, flag);
				FskBitmapDraw(bitmap, &bounds, tmp, &bounds, NULL, NULL, kFskGraphicsModeCopy, NULL);
				bitmap = tmp;
				*owned = false;
			}
			KprTextureSetBitmap(self, bitmap);
		}
	}
	if (self->bitmap) {
		self->seed = gTextureCacheSeed++;
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
	FskFileDisposeMap(map);
	FskMemPtrDispose(path);
	return result;
}

void KprTexturePurge(KprTexture self)
{
	gTextureCacheOffset -= (self->width * self->height);
	//fprintf(stderr, "- gTextureCacheOffset %d %p %s\n", gTextureCacheOffset, oldest, oldest->url);
    if (self->bitmap) {
        FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedTextureUnload, self);
        FskBitmapDispose(self->bitmap);
        self->bitmap = NULL;
    }
	self->width = 0;
	self->height = 0;
}

void KprTextureSetBitmap(KprTexture self, FskBitmap bitmap)
{
	FskRectangleRecord bounds;
	FskBitmapGetBounds(bitmap, &bounds);
	self->bitmap = bitmap;
	self->width = bounds.width;
	self->height = bounds.height;
	gTextureCacheOffset += (self->width * self->height);
	//fprintf(stderr, "+ gTextureCacheOffset %d %p %s\n", gTextureCacheOffset, self, self->url);
	KprTexturesPurge(self);
#if FSKBITMAP_OPENGL
	if (gShell->window->usingGL)
		FskBitmapSetOpenGLSourceAccelerated(bitmap, true);
#endif
	FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedTextureLoad, self);
}

void KprTextureSetEffect(KprTexture self, KprEffect effect)
{
	KprAssetUnbind(self->effect);
	self->effect = effect;
	KprAssetBind(self->effect);
}

void KprTexturesPurge(KprTexture keep)
{
	while (gTextureCacheOffset > gTextureCacheSize) {
		KprTexture self = (KprTexture)gShell->firstTexture;
		KprTexture oldest = NULL;
		while (self) {
			if ((self != keep) && self->url && self->bitmap) {
				if (oldest) {
					if (oldest->seed > self->seed)
						oldest = self;
				}
				else
					oldest = self;
			}
			self = (KprTexture)self->next;
		}
		if (!oldest)
			break;
		KprTexturePurge(oldest);
	}
}

void KprTexturesMark(xsMachine* the, xsMarkRoot markRoot)
{
	KprTexture self = (KprTexture)gShell->firstTexture;
	while (self) {
		if (self->content && (self->the == the))
			(*self->content->dispatch->mark)(self->content, markRoot);
		self = (KprTexture)self->next;
	}
}

/* SKIN */

static void KprSkinDispose(void* it);
static Boolean KprSkinFillColor(FskColorRGBA colors, FskPort port, double state, double index);
static void KprSkinFillTexture(KprSkin self, FskPort port, FskRectangle bounds, UInt32 i, UInt32 j, UInt16 horizontal, UInt16 vertical);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprSkinInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSkin", FskInstrumentationOffset(KprSkinRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprSkinNew(KprSkin *it, KprContext context, UInt32 flags, KprSkinData data)
{
	FskErr err = kFskErrNone;
	KprSkin self;
	bailIfError(KprAssetNew((KprAsset *)it, sizeof(KprSkinRecord), context, &context->firstSkin, KprSkinDispose));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprSkinInstrumentation);
#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemSetOwner(self, context);
#endif
	self->flags = flags;
	self->data = *data;
	if (self->flags)
		KprAssetBind(self->data.pattern.texture);
bail:
	return kFskErrNone;
}

void KprSkinDispose(void* it)
{
	KprSkin self = it;
	if (self->flags)
		KprAssetUnbind(self->data.pattern.texture);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(it);
}

void KprSkinExtend(KprSkin self, FskRectangle bounds)
{
	if (self->flags) {
		bounds->x -= self->data.pattern.margins.left;
		bounds->y -= self->data.pattern.margins.top;
		bounds->width += self->data.pattern.margins.left + self->data.pattern.margins.right;
		bounds->height += self->data.pattern.margins.top + self->data.pattern.margins.bottom;
	}
}

void KprSkinFill(KprSkin self, FskPort port, FskRectangle bounds, UInt32 variant, double state, UInt16 horizontal, UInt16 vertical)
{
	double index;
	if (state < 0) state = 0;
	if (3 < state) state = 3;
	index = floor(state);
	if (self->flags) {
		index = floor(state);
		KprSkinFillTexture(self, port, bounds, variant, (UInt32)index, horizontal, vertical);
		if (index < state) {
			FskGraphicsModeParametersRecord param;
			param.dataSize = sizeof(FskGraphicsModeParametersRecord);
			param.blendLevel = (UInt8)((state - index) * 255.0);
			FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, &param);
			KprSkinFillTexture(self, port, bounds, variant, (UInt32)index + 1, horizontal, vertical);
			FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
		}
	}
	else {
		FskRectangleRecord area;
		FskColorRGBARecord color;
		FskPortGetPenColor(port, &color);
		if (KprSkinFillColor(self->data.color.fill, port, state, index)) {
			area.x = bounds->x + self->data.color.borders.left;
			area.y = bounds->y + self->data.color.borders.top;
			area.width = bounds->width - self->data.color.borders.left - self->data.color.borders.right;
			area.height = bounds->height - self->data.color.borders.top - self->data.color.borders.bottom;
			FskPortRectangleFill(port, &area);
		}
		if (KprSkinFillColor(self->data.color.stroke, port, state, index)) {
			if (self->data.color.borders.top) {
				area.x = bounds->x + self->data.color.borders.left;
				area.y = bounds->y;
				area.width = bounds->width - self->data.color.borders.left;
				area.height = self->data.color.borders.top;
				FskPortRectangleFill(port, &area);
			}
			if (self->data.color.borders.right) {
				area.x = bounds->x + bounds->width - self->data.color.borders.right;
				area.y = bounds->y + self->data.color.borders.top;
				area.width = self->data.color.borders.right;
				area.height = bounds->height - self->data.color.borders.top;
				FskPortRectangleFill(port, &area);
			}
			if (self->data.color.borders.bottom) {
				area.x = bounds->x;
				area.y = bounds->y + bounds->height - self->data.color.borders.bottom;
				area.width = bounds->width - self->data.color.borders.right;
				area.height = self->data.color.borders.bottom;
				FskPortRectangleFill(port, &area);
			}
			if (self->data.color.borders.left) {
				area.x = bounds->x;
				area.y = bounds->y;
				area.width = self->data.color.borders.left;
				area.height = bounds->height - self->data.color.borders.bottom;
				FskPortRectangleFill(port, &area);
			}
		}
		FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
		FskPortSetPenColor(port, &color);
	}
}

Boolean KprSkinFillColor(FskColorRGBA colors, FskPort port, double state, double index)
{
	FskColorRGBARecord color;
	if (index == state)
		color = colors[(UInt32)index];
	else {
		UInt32 dst, src;
		FskConvertColorRGBAToBitmapPixel(&colors[(UInt32)index], kFskBitmapFormat32ARGB, &dst);
		FskConvertColorRGBAToBitmapPixel(&colors[(UInt32)index + 1], kFskBitmapFormat32ARGB, &src);
		FskName2(FskAlphaBlend,fsk32ARGBFormatKind)(&dst, src, (UInt8)((state - index) * 255.0));
		FskConvertBitmapPixelToColorRGBA(&dst, kFskBitmapFormat32ARGB, &color);
	}
	if (!color.a)
		return false;
	if (color.a != 255) {
		FskGraphicsModeParametersRecord param;
		param.dataSize = sizeof(FskGraphicsModeParametersRecord);
		param.blendLevel = (SInt32)color.a;
		FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, &param);
		color.a = 255;
	}
	else
		FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
	FskPortSetPenColor(port, &color);
	return true;
}

#define SCALE15x(x) (x + ((x + 1) >> 1))
#define UNSCALE15x(x) (((x << 1) + 2) / 3)

static void scaleRect1x(FskRectangle r UNUSED)
{
}

static void scaleRect15x(FskRectangle r)
{
	SInt32 dx = r->x + r->width, dy = r->y + r->height;
	r->x = SCALE15x(r->x);
	r->y = SCALE15x(r->y);
	r->width = SCALE15x(dx) - r->x;
	r->height = SCALE15x(dy) - r->y;
}

static void scaleRect2x(FskRectangle r)
{
	r->x <<= 1;
	r->y <<= 1;
	r->width <<= 1;
	r->height <<= 1;
}

#define TILE_BITMAP(X, Y, DX, DY, U, V, DU, DV) \
	FskRectangleSet(&dstRect, x + X, y + Y, DX, DY); \
	FskRectangleSet(&srcRect, u + U, v + V, DU, DV); \
	(*scaleRect)(&srcRect); \
	FskPortBitmapTile(port, bitmap, &srcRect, &dstRect, scale)

void KprSkinFillTexture(KprSkin self, FskPort port, FskRectangle bounds, UInt32 i, UInt32 j, UInt16 horizontal, UInt16 vertical)
{
	UInt32 flags = self->flags;
	KprTexture texture = self->data.pattern.texture;
	Boolean owned = false;
	FskBitmap bitmap = KprTextureGetBitmap(texture, port, &owned);
	FskFixed scale = texture->scale;
	void (*scaleRect)(FskRectangle r);
	if (scale == 131072) {
		scaleRect = scaleRect2x;
		scale = FskPortScaleGet(port) >> 1;
	}
	else if (scale == 98304) {
		scaleRect = scaleRect15x;
		scale = UNSCALE15x(FskPortScaleGet(port));
	}
	else {
		scaleRect = scaleRect1x;
		scale = FskPortScaleGet(port);
	}
	if (bitmap) {
		SInt32 x, y, dx, dy, u, v, du, dv, dl, dt, dr, db;
		FskRectangleRecord srcRect, dstRect;
#if CHECK_UNACCELERATED_BITMAPS
		if (FskBitmapIsOpenGLDestinationAccelerated(port->bits) && !bitmap->accelerate)
			return;
#endif
		x = bounds->x;
		y = bounds->y;
		dx = bounds->width;
		dy = bounds->height;
		x -= self->data.pattern.margins.left;
		y -= self->data.pattern.margins.top;
		dx += self->data.pattern.margins.left + self->data.pattern.margins.right;
		dy += self->data.pattern.margins.top + self->data.pattern.margins.bottom;
		u = self->data.pattern.bounds.x + (i * self->data.pattern.delta.x);
		v = self->data.pattern.bounds.y + (j * self->data.pattern.delta.y);
		du = self->data.pattern.bounds.width;
		dv = self->data.pattern.bounds.height;
		dl = self->data.pattern.tiles.left;
		dt = self->data.pattern.tiles.top;
		dr = self->data.pattern.tiles.right;
		db = self->data.pattern.tiles.bottom;
		if (flags & kprRepeatX) {
			if (flags & kprRepeatY) {
				TILE_BITMAP(0, 0, dl, dt, 0, 0, dl, dt);
				TILE_BITMAP(dl, 0, dx - dl - dr, dt, dl, 0, du - dl - dr, dt);
				TILE_BITMAP(dx - dr, 0, dr, dt, du - dr, 0, dr, dt);
				TILE_BITMAP(0, dt, dl, dy - dt - db, 0, dt, dl, dv - dt - db);
				TILE_BITMAP(dl, dt, dx - dl - dr, dy - dt - db, dl, dt, du - dl - dr, dv - dt - db);
				TILE_BITMAP(dx - dr, dt, dr, dy - dt - db, du - dr, dt, dr, dv - dt - db);
				TILE_BITMAP(0, dy - db, dl, db, 0, dv - db, dl, db);
				TILE_BITMAP(dl, dy - db, dx - dl - dr, db, dl, dv - db, du - dl - dr, db);
				TILE_BITMAP(dx - dr, dy - db, dr, db, du - dr, dv - db, dr, db);
			}
			else {
				switch (vertical & kprTopBottom) {
				case kprTop: break;
				case kprBottom: y += dy - dv; break;
				default: y += (dy - dv + 1) >> 1; break;
				}
				TILE_BITMAP(0, 0, dl, dv, 0, 0, dl, dv);
				TILE_BITMAP(dl, 0, dx - dl - dr, dv, dl, 0, du - dl - dr, dv);
				TILE_BITMAP(dx - dr, 0, dr, dv, du - dr, 0, dr, dv);
			}
		}
		else {
			if (flags & kprRepeatY) {
				switch (horizontal & kprLeftRight) {
				case kprLeft: break;
				case kprRight: x += dx - du; break;
				default: x += (dx - du + 1) >> 1; break;
				}
				TILE_BITMAP(0, 0, du, dt, 0, 0, du, dt);
				TILE_BITMAP(0, dt, du, dy - dt - db, 0, dt, du, dv - dt - db);
				TILE_BITMAP(0, dy - db, du, db, 0, dv - db, du, db);
			}
			else {
				FskRectangleSet(&dstRect, x, y, dx, dy);
				FskRectangleSet(&srcRect, u, v, du, dv);
				KprAspectApply(flags, &srcRect, &dstRect);
				(*scaleRect)(&srcRect);
				FskPortBitmapDraw(port, bitmap, &srcRect, &dstRect);
			}
		}
		if (!owned)
			FskBitmapDispose(bitmap);
	}
}

void KprSkinMeasure(KprSkin self, FskRectangle bounds)
{
	UInt32 flags = self->flags;
	FskRectangleSetEmpty(bounds);
	if (flags) {
		if (!(flags & kprRepeatX))
			bounds->width = self->data.pattern.bounds.width - self->data.pattern.margins.left - self->data.pattern.margins.right;
		if (!(flags & kprRepeatY))
			bounds->height = self->data.pattern.bounds.height - self->data.pattern.margins.top - self->data.pattern.margins.bottom;
	}
}

/* ECMASCRIPT */
#ifndef KPR_NO_GRAMMAR

void KPR_texture(void *it)
{
	if (it) {
		KprTexture self = it;
		kprVolatileDestructor(KPR_texture);
		KprAssetDispose(self);
	}
}

void KPR_Texture(xsMachine *the)
{
	FskFixed scale = 65536;
	KprTexture self;
	xsVars(1);
	if ((xsToInteger(xsArgc) > 1) && xsTest(xsArg(1))) {
		xsNumberValue number = xsToNumber(xsArg(1));
		if (number == 2) scale = 131072;
		else if (number == 1.5) scale = 98304;
		else if (number == 1) scale = 65536;
	}
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		xsStringValue reference = xsToString(xsArg(0));
		xsStringValue base = xsToString(xsModuleURL);
		xsThrowDiagnosticIfFskErr(KprTextureNew(&self, xsGetContext(the), base, reference, NULL, NULL, scale), "Unable to load texture %s", reference);
	}
	else {
		KprContent content = kprGetHostData(xsArg(0), content, content);
		ASSERT(content->container == NULL);
		ASSERT(content->previous == NULL);
		ASSERT(content->next == NULL);
		xsThrowIfFskErr(KprTextureNew(&self, xsGetContext(the), NULL, NULL, NULL, content, scale));
	}
	kprVolatileConstructor(KPR_Texture);
}

void KPR_texture_get_content(xsMachine *the)
{
	KprTexture self = xsGetHostData(xsThis);
	KprContent content = self->content;
	if (content)
		xsResult = kprContentGetter(content);
}

void KPR_texture_get_effect(xsMachine *the)
{
	KprTexture self = xsGetHostData(xsThis);
	xsResult = kprVolatileGetter(self->effect, xsID_effect);
}

void KPR_texture_get_scale(xsMachine *the)
{
	KprTexture self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->scale / 65536);
}

void KPR_texture_get_size(xsMachine *the)
{
	KprTexture self = xsGetHostData(xsThis);
	Boolean owned = false;
	FskBitmap bitmap = KprTextureGetBitmap(self, NULL, &owned);
	if (bitmap) {
		FskRectangleRecord bounds;
		FskBitmapGetBounds(bitmap, &bounds);
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID_width, xsInteger(bounds.width), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_height, xsInteger(bounds.height), xsDefault, xsDontScript);
		if (!owned)
			FskBitmapDispose(bitmap);
	}
}

void KPR_texture_get_width(xsMachine *the)
{
	KprTexture self = xsGetHostData(xsThis);
	Boolean owned = false;
	FskBitmap bitmap = KprTextureGetBitmap(self, NULL, &owned);
	if (bitmap) {
		FskRectangleRecord bounds;
		FskBitmapGetBounds(bitmap, &bounds);
		xsResult = xsInteger(bounds.width);
		if (!owned)
			FskBitmapDispose(bitmap);
	}
}

void KPR_texture_get_height(xsMachine *the)
{
	KprTexture self = xsGetHostData(xsThis);
	Boolean owned = false;
	FskBitmap bitmap = KprTextureGetBitmap(self, NULL, &owned);
	if (bitmap) {
		FskRectangleRecord bounds;
		FskBitmapGetBounds(bitmap, &bounds);
		xsResult = xsInteger(bounds.height);
		if (!owned)
			FskBitmapDispose(bitmap);
	}
}

void KPR_texture_set_effect(xsMachine *the)
{
	KprTexture self = xsGetHostData(xsThis);
	KprEffect effect = NULL;
	if (xsTest(xsArg(0)))
		effect = kprGetHostData(xsArg(0), it, effect);
	KprTextureSetEffect(self, effect);
}

void KPR_texture_purge(xsMachine *the)
{
	KprTexture self = xsGetHostData(xsThis);
	KprTexturePurge(self);
}

void KPR_skin(void *it)
{
	if (it) {
		KprSkin self = it;
		kprVolatileDestructor(KPR_skin);
		KprAssetDispose(self);
	}
}

void KPR_Skin(xsMachine *the)
{
	KprSkin self;
	xsIntegerValue c = xsToInteger(xsArgc);
	UInt32 flags = 0;
	KprSkinDataRecord data;
	xsVars(1);
	FskMemSet(&data, 0, sizeof(data));
	xsEnterSandbox();
	if ((c > 0) && xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID_texture))) {
		flags |= kprPattern;
		data.pattern.texture = xsGetHostData(xsArg(0));
		if ((c > 1) && xsTest(xsArg(1))) {
			(void)xsFindInteger(xsArg(1), xsID_x, &data.pattern.bounds.x);
			(void)xsFindInteger(xsArg(1), xsID_y, &data.pattern.bounds.y);
			(void)xsFindInteger(xsArg(1), xsID_width, &data.pattern.bounds.width);
			(void)xsFindInteger(xsArg(1), xsID_height, &data.pattern.bounds.height);
		}
		if (c > 2)
			data.pattern.delta.x = xsToInteger(xsArg(2));
		if (c > 3)
			data.pattern.delta.y = xsToInteger(xsArg(3));
		if ((c > 4) && xsTest(xsArg(4))) {
			if (xsFindInteger(xsArg(4), xsID_left, &data.pattern.tiles.left))
				flags |= kprRepeatX;
			if (xsFindInteger(xsArg(4), xsID_right, &data.pattern.tiles.right))
				flags |= kprRepeatX;
			if (xsFindInteger(xsArg(4), xsID_top, &data.pattern.tiles.top))
				flags |= kprRepeatY;
			if (xsFindInteger(xsArg(4), xsID_bottom, &data.pattern.tiles.bottom))
				flags |= kprRepeatY;
		}
		if ((c > 5) && xsTest(xsArg(5))) {
			(void)xsFindInteger(xsArg(5), xsID_left, &data.pattern.margins.left);
			(void)xsFindInteger(xsArg(5), xsID_right, &data.pattern.margins.right);
			(void)xsFindInteger(xsArg(5), xsID_top, &data.pattern.margins.top);
			(void)xsFindInteger(xsArg(5), xsID_bottom, &data.pattern.margins.bottom);
		}
		if ((c > 6) && xsTest(xsArg(6))) {
			char* aspect = xsToString(xsArg(6));
			if (!FskStrCompare(aspect, "fill"))
				flags |= kprImageFill;
			else if (!FskStrCompare(aspect, "fit"))
				flags |= kprImageFit;
			else if (!FskStrCompare(aspect, "stretch"))
				flags |= kprImageFill | kprImageFit;
		}
	}
	else {
		if (c > 0) {
			if (xsTypeOf(xsArg(0)) == xsStringType) {
				FskColorRGBARecord color;
				if (KprParseColor(the, xsToString(xsArg(0)), &color)) {
					data.color.fill[0] = color;
					data.color.fill[1] = color;
					data.color.fill[2] = color;
					data.color.fill[3] = color;
				}
			}
			else if (xsIsInstanceOf(xsArg(0), xsArrayPrototype)) {
				xsIntegerValue i, l = xsToInteger(xsGet(xsArg(0), xsID_length));
				if (l > 4) l = 4;
				for (i = 0; i < l; i++) {
					FskColorRGBARecord color;
					xsVar(0) = xsGetAt(xsArg(0), xsInteger(i));
					if (xsTest(xsVar(0))) {
						if (KprParseColor(the, xsToString(xsVar(0)), &color))
							data.color.fill[i] = color;
					}
				}
			}
		}
		if ((c > 1) && xsTest(xsArg(1))) {
			(void)xsFindInteger(xsArg(1), xsID_left, &data.color.borders.left);
			(void)xsFindInteger(xsArg(1), xsID_right, &data.color.borders.right);
			(void)xsFindInteger(xsArg(1), xsID_top, &data.color.borders.top);
			(void)xsFindInteger(xsArg(1), xsID_bottom, &data.color.borders.bottom);
		}
		if (c > 2) {
			if (xsTypeOf(xsArg(2)) == xsStringType) {
				FskColorRGBARecord color;
				if (KprParseColor(the, xsToString(xsArg(2)), &color)) {
					data.color.stroke[0] = color;
					data.color.stroke[1] = color;
					data.color.stroke[2] = color;
					data.color.stroke[3] = color;
				}
			}
			else if (xsIsInstanceOf(xsArg(2), xsArrayPrototype)) {
				xsIntegerValue i, l = xsToInteger(xsGet(xsArg(2), xsID_length));
				if (l > 4) l = 4;
				for (i = 0; i < l; i++) {
					FskColorRGBARecord color;
					xsVar(0) = xsGetAt(xsArg(2), xsInteger(i));
					if (xsTest(xsVar(0))) {
						if (KprParseColor(the, xsToString(xsVar(0)), &color))
							data.color.stroke[i] = color;
					}
				}
			}
		}
	}
	xsLeaveSandbox();
	KprSkinNew(&self, xsGetContext(the), flags, &data);
	kprVolatileConstructor(KPR_Skin);
}

void KPR_skin_get_borders(xsMachine *the)
{
	KprSkin self = xsGetHostData(xsThis);
	if (!self->flags) {
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID_left, xsInteger(self->data.color.borders.left), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_right, xsInteger(self->data.color.borders.right), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_top, xsInteger(self->data.color.borders.top), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_bottom, xsInteger(self->data.color.borders.bottom), xsDefault, xsDontScript);
	}
}

void KPR_skin_get_bounds(xsMachine *the)
{
	KprSkin self = xsGetHostData(xsThis);
	if (self->flags) {
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID_x, xsInteger(self->data.pattern.bounds.x), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_y, xsInteger(self->data.pattern.bounds.y), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_width, xsInteger(self->data.pattern.bounds.width), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_height, xsInteger(self->data.pattern.bounds.height), xsDefault, xsDontScript);
	}
}

void KPR_skin_get_fillColors(xsMachine *the)
{
	KprSkin self = xsGetHostData(xsThis);
	if (!self->flags) {
		xsIntegerValue i;
		xsVars(1);
		xsResult = xsNew1(xsGlobal, xsID_Array, xsInteger(4));
		(void)xsCall0(xsResult, xsID_fill);
		for (i = 0; i < 4; i++) {
			KprSerializeColor(the, &self->data.color.fill[i], &xsVar(0));
			xsSetAt(xsResult, xsInteger(i), xsVar(0));
		}
	}
}

void KPR_skin_get_margins(xsMachine *the)
{
	KprSkin self = xsGetHostData(xsThis);
	if (self->flags) {
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID_left, xsInteger(self->data.pattern.margins.left), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_right, xsInteger(self->data.pattern.margins.right), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_top, xsInteger(self->data.pattern.margins.top), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_bottom, xsInteger(self->data.pattern.margins.bottom), xsDefault, xsDontScript);
	}
}

void KPR_skin_get_states(xsMachine *the)
{
	KprSkin self = xsGetHostData(xsThis);
	if (self->flags)
		xsResult = xsInteger(self->data.pattern.delta.y);
}

void KPR_skin_get_texture(xsMachine *the)
{
	KprSkin self = xsGetHostData(xsThis);
	if (self->flags)
		xsResult = kprVolatileGetter(self->data.pattern.texture, xsID_texture);
}

void KPR_skin_get_strokeColors(xsMachine *the)
{
	KprSkin self = xsGetHostData(xsThis);
	if (!self->flags) {
		xsIntegerValue i;
		xsVars(1);
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		for (i = 0; i < 4; i++) {
			KprSerializeColor(the, &self->data.color.stroke[i], &xsVar(0));
			xsSetAt(xsResult, xsInteger(i), xsVar(0));
		}
	}
}

void KPR_skin_get_tiles(xsMachine *the)
{
	KprSkin self = xsGetHostData(xsThis);
	if (self->flags) {
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		if (self->flags & kprRepeatX) {
			xsNewHostProperty(xsResult, xsID_left, xsInteger(self->data.pattern.tiles.left), xsDefault, xsDontScript);
			xsNewHostProperty(xsResult, xsID_right, xsInteger(self->data.pattern.tiles.right), xsDefault, xsDontScript);
		}
		if (self->flags & kprRepeatY) {
			xsNewHostProperty(xsResult, xsID_top, xsInteger(self->data.pattern.tiles.top), xsDefault, xsDontScript);
			xsNewHostProperty(xsResult, xsID_bottom, xsInteger(self->data.pattern.tiles.bottom), xsDefault, xsDontScript);
		}
	}
}

void KPR_skin_get_variants(xsMachine *the)
{
	KprSkin self = xsGetHostData(xsThis);
	if (self->flags)
		xsResult = xsInteger(self->data.pattern.delta.x);
}

void KPR_skin_get_width(xsMachine *the)
{
	KprSkin self = xsGetHostData(xsThis);
	if (self->flags)
		xsResult = xsInteger(self->data.pattern.bounds.width);
}

void KPR_skin_get_height(xsMachine *the)
{
	KprSkin self = xsGetHostData(xsThis);
	if (self->flags)
		xsResult = xsInteger(self->data.pattern.bounds.height);
}

#endif /* KPR_NO_GRAMMAR */
