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
#ifndef __KPRIMAGE__
#define __KPRIMAGE__

#include "kpr.h"
#include "FskImage.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct KprImageStruct {
	KprSlotPart;
	KprContentPart;
	KprImagePart;
	KprLayerPart;
};

struct KprImageCacheStruct {
	UInt32 count;
	UInt32 index;
	KprImageEntry first;
	KprImageEntry last;
	UInt32 size;
	SInt32 priority;
	UInt32 activeCount;
	UInt32 activeIndex;
	FskInstrumentedItemDeclaration
	KprImageEntry entries[1];
};

struct KprImageEntryStruct {
	KprImageCache cache;
	KprImageEntry former;
	KprImageEntry next;
	KprImageEntry previous;
	KprImageLink first;
	FskBitmap bitmap;
	char* url;
	char* mime;
	UInt32 aspect;
	UInt32 width;
	UInt32 height;
	UInt32 sum;
	FskErr error;
	KprMessage message;
	FskInstrumentedItemDeclaration
};

struct KprImageLinkStruct {
	KprImageLink next;
	KprImage image;
	FskInstrumentedItemDeclaration
};

struct KprImageTargetStruct {
	KprStreamPart;
	FskBitmap bitmap;
	UInt32 aspect;
	UInt32 width;
	UInt32 height;
};

FskAPI(FskErr) KprImageCacheNew(KprImageCache *it, UInt32 count, UInt32 size, UInt32 active, SInt32 priority);
FskAPI(void) KprImageCacheDispose(KprImageCache self);
FskAPI(void) KprImageCacheDump(KprImageCache self);
FskAPI(void) KprImageCachePurge(KprImageCache self);

FskAPI(void) KprImageEntryProgress(char* url, FskBitmap bitmap);
FskAPI(FskErr) KprImageTargetScale(FskBitmap* bitmap, UInt32 aspect, UInt32 width, UInt32 height);

struct KprPictureStruct {
	KprSlotPart;
	KprContentPart;
	KprImagePart;
	KprLayerPart;
};

FskAPI(FskErr) KprPictureNew(KprPicture *self, KprCoordinates coordinates);
FskAPI(void) KprPictureSetURL(KprPicture self, char* url, char* mime);

struct KprThumbnailStruct {
	KprSlotPart;
	KprContentPart;
	KprImagePart;
	KprLayerPart;
};

FskAPI(FskErr) KprThumbnailNew(KprThumbnail *self, KprCoordinates coordinates);
FskAPI(void) KprThumbnailSetURL(KprThumbnail self, char* url, char* mime);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
