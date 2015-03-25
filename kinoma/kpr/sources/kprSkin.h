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
#ifndef __KPRSKIN__
#define __KPRSKIN__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct KprAssetStruct {
	KprSlotPart;
	KprAssetPart;
};

FskAPI(FskErr) KprAssetNew(KprAsset *it, UInt32 size, KprContext context, KprAsset *first, KprAssetDisposeProc dispose);
FskAPI(void) KprAssetDispose(void* it);
FskAPI(void) KprAssetBind(void* it);
FskAPI(void) KprAssetUnbind(void* it);
FskAPI(void)  KprAssetsPurge(KprAsset* first, Boolean flag);

struct KprTextureStruct {
	KprSlotPart;
	KprAssetPart;
	char* url;
	char* mime;
	KprContent content;
	FskFixed scale;
	KprEffect effect;
	FskBitmap bitmap;
	UInt32 seed;
	SInt32 width;
	SInt32 height;
};

FskAPI(FskErr) KprTextureNew(KprTexture *it, KprContext context, char* base, char* url, char* mime, KprContent content, FskFixed scale);
FskAPI(FskBitmap) KprTextureGetBitmap(KprTexture self, FskPort port, Boolean* owned);
FskAPI(void) KprTextureSetBitmap(KprTexture self, FskBitmap bitmap);
FskAPI(void) KprTextureSetEffect(KprTexture self, KprEffect effect);
FskAPI(void) KprTexturePurge(KprTexture self); 
FskAPI(void) KprTexturesMark(xsMachine* the, xsMarkRoot markRoot);

enum {
	kprColor = 0,
	kprPattern = 1 << 0,
	kprRepeatX = 1 << 1,
	kprRepeatY = 1 << 2
};

typedef union {
	struct {
		FskColorRGBARecord fill[4];
		FskColorRGBARecord stroke[4];
		KprMarginsRecord borders;
	} color;
	struct {
		KprTexture texture;
		FskRectangleRecord bounds;
		FskPointRecord delta;
		KprMarginsRecord tiles;
		KprMarginsRecord margins;
	} pattern;
} KprSkinDataRecord, *KprSkinData;

struct KprSkinStruct {
	KprSlotPart;
	KprAssetPart;
	UInt32 flags;
	KprSkinDataRecord data;
};

FskAPI(FskErr) KprSkinNew(KprSkin *it, KprContext context, UInt32 flags, KprSkinData data);
FskAPI(void) KprSkinExtend(KprSkin self, FskRectangle bounds);
FskAPI(void) KprSkinFill(KprSkin self, FskPort port, FskRectangle bounds, UInt32 i, double state, UInt16 horizontal, UInt16 vertical);
FskAPI(void) KprSkinMeasure(KprSkin self, FskRectangle bounds);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
