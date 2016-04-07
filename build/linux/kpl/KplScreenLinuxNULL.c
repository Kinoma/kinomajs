/*
 *     Copyright (C) 2002-2015 Kinoma, Inc.
 *
 *     All rights reserved.
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */
#include "KplScreen.h"
#include "FskMemory.h"
#include "FskBitmap.h"

#define kScreenWidth		320
#define kScreenHeight		240
#define kScreenDepth		32
#define kScreenPixelFormat	kFskBitmapFormat32BGRA

typedef struct KplScreenStruct KplScreenRecord, *KplScreen;

struct KplScreenStruct {
	KplBitmap bitmap;
};

static KplScreen gKplScreen = NULL;

FskErr KplScreenGetBitmap(KplBitmap *bitmap)
{
	FskErr err = kFskErrNone;

	if (!gKplScreen) {
		BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(KplScreenRecord), (FskMemPtr *)&gKplScreen));
		BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(KplBitmapRecord), (FskMemPtr *)&gKplScreen->bitmap));
		gKplScreen->bitmap->width = kScreenWidth;
		gKplScreen->bitmap->height = kScreenHeight;
		gKplScreen->bitmap->pixelFormat = kScreenPixelFormat;
		gKplScreen->bitmap->depth = kScreenDepth;
		gKplScreen->bitmap->rowBytes = kScreenWidth * (kScreenDepth / 8);
		BAIL_IF_ERR(err = FskMemPtrNewClear(gKplScreen->bitmap->rowBytes * kScreenHeight, (FskMemPtr *)&gKplScreen->bitmap->baseAddress));
	}
	
	*bitmap = gKplScreen->bitmap;

bail:
	return err;
}

FskErr KplScreenDisposeBitmap(KplBitmap bitmap)
{
	if (gKplScreen) {
		FskMemPtrDispose(gKplScreen->bitmap);
		FskMemPtrDisposeAt(&gKplScreen);
	}
	return kFskErrNone;
}

FskErr KplScreenLockBitmap(KplBitmap bitmap)
{
	return kFskErrNone;
}

FskErr KplScreenUnlockBitmap(KplBitmap bitmap)
{
	return kFskErrNone;
}

FskErr KplScreenDrawBitmap(KplBitmap src, KplRectangle srcRect, KplBitmap screen, KplRectangle dstRect)
{
	return kFskErrNone;
}

void initializeLinuxinput(void) {}
void terminateLinuxinput(void) {}

