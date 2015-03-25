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
#define __FSKVIDEOSPRITE_PRIV__
#include "FskVideoSprite.h"

#include "FskList.h"
#include "FskMemory.h"

FskErr FskVideoSpriteWorldNew(FskVideoSpriteWorld *worldOut)
{
	return FskMemPtrNewClear(sizeof(FskVideoSpriteWorldRecord), worldOut);
}

void FskVideoSpriteWorldDispose(FskVideoSpriteWorld world)
{
	if (!world) return;

	while (world->sprites)
		FskVideoSpriteDispose(world->sprites);

	FskMemPtrDispose(world);
}

FskErr FskVideoSpriteNew(FskVideoSpriteWorld world, FskVideoSprite *spriteOut, FskBitmap bits, FskRectangle srcRect)
{
	FskErr err;
	FskVideoSprite sprite = NULL;

	err = FskMemPtrNewClear(sizeof(FskVideoSpriteRecord), &sprite);
	if (err) goto bail;

	sprite->world = world;
	sprite->bits = bits;
	if (srcRect)
		sprite->srcRect = *srcRect;
	else
		FskBitmapGetBounds(bits, &sprite->srcRect);

	FskBitmapGetDepth(bits, &sprite->depth);
	FskBitmapReadBegin(bits, (const void**)(const void*)&sprite->baseAddr, &sprite->row_bytes, &sprite->pixelFormat);
	FskBitmapReadEnd(bits);		//@@ this is bad... cann't assume baseAddr is valid after this line. should defer to FskVideoSpriteDispose... or do on each blit
	sprite->baseAddr += (sprite->srcRect.y * sprite->row_bytes) + (sprite->srcRect.x * (sprite->depth / 8));

	FskVideoSpriteSetGraphicsMode(sprite, kFskGraphicsModeCopy, NULL);

	sprite->dst_x = 0;
	sprite->dst_y = 0;

	world->updated = true;

	FskListPrepend(&world->sprites, sprite);

bail:
	if (err) {
		FskVideoSpriteDispose(sprite);
		sprite = NULL;
	}

	*spriteOut = sprite;

	return err;
}

void FskVideoSpriteDispose(FskVideoSprite sprite)
{
	if (!sprite) return;

	FskListRemove(&sprite->world->sprites, sprite);

	FskMemPtrDispose(sprite);
}

FskErr FskVideoSpriteSetLocation(FskVideoSprite sprite, SInt32 x, SInt32 y)
{
	if ((x == (SInt32)sprite->dst_x) && (y == (SInt32)sprite->dst_y))
		return kFskErrNone;

	sprite->dst_x = x;
	sprite->dst_y = y;

	sprite->world->updated = true;

	return kFskErrNone;
}


FskErr FskVideoSpriteSetGraphicsMode(FskVideoSprite sprite, UInt32 graphicsMode, const FskGraphicsModeParameters graphicsModeParameters)
{
	sprite->mode = graphicsMode;
//@@ ignoring graphicsModeParameters

	sprite->world->updated = true;

	return kFskErrNone;
}
