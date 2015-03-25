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
#ifndef __FSKVIDEOSPRITE__
#define __FSKVIDEOSPRITE__

#include "FskBitmap.h"
#include "FskRectBlitPatch.h"

typedef struct FskVideoSpriteWorldRecord FskVideoSpriteWorldRecord ;
typedef struct FskVideoSpriteWorldRecord  *FskVideoSpriteWorld;

typedef struct FskVideoSpriteRecord FskVideoSpriteRecord;
typedef struct FskVideoSpriteRecord *FskVideoSprite;

#ifdef __FSKVIDEOSPRITE_PRIV__

struct FskVideoSpriteWorldRecord {
	FskVideoSprite			sprites;
	Boolean					updated;
};

struct FskVideoSpriteRecord {
	FskVideoSprite			next;

	FskVideoSpriteWorld		world;

	FskBitmap				bits;
	FskRectangleRecord		srcRect;
	UInt32					dst_x;
	UInt32					dst_y;

	UInt32					depth;
	FskBitmapFormatEnum		pixelFormat;
	FskRectTransferProc		blitter;

	SInt32					x0,y0;
	SInt32					x1,y1;	
	UInt8					*baseAddr;
	UInt8					*addr;
	SInt32					row_bytes;
	SInt32					in_frame;
	SInt32					in_line;
	SInt32					parsed;
	SInt32					to_blend;
	FskRectBlitParams		p;

	UInt32					mode; 
	char					modeParams[1]; 
};

#endif //__FSKVIDEOSPRITE_PRIV__

FskAPI(FskErr) FskVideoSpriteWorldNew(FskVideoSpriteWorld *world);
FskAPI(void) FskVideoSpriteWorldDispose(FskVideoSpriteWorld world);

FskAPI(FskErr) FskVideoSpriteNew(FskVideoSpriteWorld world, FskVideoSprite *sprite, FskBitmap bits, FskRectangle srcRect);
FskAPI(void) FskVideoSpriteDispose(FskVideoSprite sprite);

FskAPI(FskErr) FskVideoSpriteSetGraphicsMode(FskVideoSprite sprite, UInt32 graphicsMode, const FskGraphicsModeParameters graphicsModeParameters);
FskAPI(FskErr) FskVideoSpriteSetLocation(FskVideoSprite sprite, SInt32 x, SInt32 y);

#endif
