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
#include "FskEndian.h"
#include "FskImage.h"

#include "kprBehavior.h"
#include "kprContent.h"
#include "kprEffect.h"
#include "kprHTTPClient.h"
#include "kprImage.h"
#include "kprLayer.h"
#include "kprMessage.h"
#include "kprSkin.h"
#include "kprURL.h"
#include "kprShell.h"
#include "kprUtilities.h"

/* IMAGE */

static void KprImageCacheClear(KprImageCache self, char* url);
static KprImageEntry KprImageCacheGet(KprImageCache self, char* url, UInt32 aspect, UInt32 width, UInt32 height, UInt32* sum);
static FskErr KprImageCacheIterate(KprImageCache self);
static void KprImageCacheLoadMessageComplete(KprMessage message, void* it);
static void KprImageCachePut(KprImageCache self, KprImageEntry entry);
static void KprImageCacheRemove(KprImageCache self, KprImageEntry entry);
static UInt32 KprImageCacheSum(char* url);

static FskErr KprImageEntryNew(KprImageEntry *it, KprImageCache cache, KprImage image, UInt32 aspect, UInt32 width, UInt32 height);
static void KprImageEntryDispose(KprImageEntry self);
static void KprImageEntryInvalidate(KprContent content);
static void KprImageEntryLoaded(KprImageEntry self);
static FskErr KprImageEntryLoad(KprImageEntry self, char* url, char* mime);
//static FskErr KprImageEntryLoadFile(KprImageEntry self, char* url, char* mime);
static FskErr KprImageEntryLoadMessage(KprImageEntry self, char* url, char* mime);
static void KprImageEntryLoadMessageComplete(KprMessage message, void* it);
static void KprImageEntryRefresh(KprImageEntry self, Boolean delegate);
static void KprImageEntryRemove(KprImageEntry self, KprImage image);

static FskErr KprImageLinkNew(KprImageLink *it, KprImage image);
static void KprImageLinkDispose(KprImageLink self);

static FskErr KprImageTargetNew(KprImageTarget* it, UInt32 aspect, UInt32 width, UInt32 height);
static void KprImageTargetDispose(void* it);
static void KprImageTargetTransform(void* it, KprMessage message, xsMachine* machine);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprImageCacheInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprImageCache", FskInstrumentationOffset(KprImageCacheRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

static UInt32 gImageMaximumWidthAndHeight = 0;

FskErr KprImageCacheNew(KprImageCache *it, UInt32 count, UInt32 size, UInt32 active, SInt32 priority)
{
	FskErr err = kFskErrNone;
	KprImageCache self;
	bailIfError(FskMemPtrNewClear(sizeof(KprImageCacheRecord) + ((size - 1) * sizeof(KprImageEntry)), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprImageCacheInstrumentation);
	self->count = count;
	self->size = size;
	self->priority = priority;
	self->activeCount = active;
	gImageMaximumWidthAndHeight = KprEnvironmentGetUInt32("imageMaximumWidthAndHeight", 0);
bail:
	return err;
}

void KprImageCacheClear(KprImageCache self, char* url)
{
	if (self) {
		UInt32 sum = KprImageCacheSum(url);
		KprImageEntry* address = &self->entries[sum % self->size];
        KprImageEntry entry;
		while ((entry = *address)) {
			if ((entry->sum == sum) && !FskStrCompare(entry->url, url)) {
				KprImageEntry previous = entry->previous;
				KprImageEntry next = entry->next;
				if (previous)
					previous->next = next;
				else
					self->first = next;
				if (next)
					next->previous = previous;
				else
					self->last = previous;
				*address = entry->former;
				KprImageEntryDispose(entry);
			}
			else
				address = &entry->former;
		}
	}
}

void KprImageCacheDispose(KprImageCache self)
{
	if (self) {
		UInt32 c = self->size, i;
		for (i = 0; i < c; i++) {
			KprImageEntry entry = self->entries[i];
			while (entry) {
				KprImageEntry former = entry->former;
				KprImageEntryDispose(entry);
				entry = former;
			}
		}
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprImageCacheDump(KprImageCache self)
{
	if (self) {
		UInt32 c = self->size, i;
		for (i = 0; i < c; i++) {
			KprImageEntry entry = self->entries[i];
			while (entry) {
				KprImageLink link;
				fprintf(stderr, "\t%p ", entry);
				fprintf(stderr, "bitmap %p images ", entry->bitmap);
				link = entry->first;
				while (link) {
					fprintf(stderr, "%p ", link->image);
					link = link->next;
				}
				fprintf(stderr, "%s\n", entry->url);
				entry = entry->former;
			}
		}
	}
}

KprImageEntry KprImageCacheGet(KprImageCache self, char* url, UInt32 aspect, UInt32 width, UInt32 height, UInt32* sum)
{
	KprImageEntry result;
	if (url) {
		UInt32 s = *sum = KprImageCacheSum(url);
		result = self->entries[s % self->size];
		while (result) {
			if ((result->sum == s) && (result->aspect == aspect) && (result->width == width) && (result->height == height) && !FskStrCompare(result->url, url)) {
				FskInstrumentedItemSendMessageDebug(self, kprInstrumentedImageCacheGet, result);
				return result;
			}
			result = result->former;
		}
	}
	return NULL;
}

FskErr KprImageCacheIterate(KprImageCache self)
{
	FskErr err = kFskErrNone;
	if (self->activeIndex < self->activeCount) {
		KprImageEntry entry = self->last;
		while (entry) {
			if (!entry->error && entry->url && !entry->message && !entry->bitmap) break;
			entry = entry->previous;
		}
		if (entry) {
			KprMessage message = NULL;
			KprImageLink link = entry->first;
			KprImageTarget target = NULL;
			self->activeIndex++;
			bailIfError(KprMessageNew(&message, entry->url));
			while (link) {
				KprImageLink next = link->next;
				KprContent content = (KprContent)link->image;
				kprDelegateInvoke(content, message);
				link = next;
			}
			bailIfError(KprImageTargetNew(&target, entry->aspect, entry->width, entry->height));
			KprMessageSetStream(message, (KprStream)target);
			entry->message = message;
			KprMessageInvoke(message, KprImageCacheLoadMessageComplete, NULL, entry);
		}
	}
bail:
	return err;
}

void KprImageCacheLoadMessageComplete(KprMessage message, void* it)
{
	KprImageEntry entry = it;
	KprImageCache self = entry->cache;
	KprImageTarget target = (KprImageTarget)message->stream;
	entry->bitmap = target->bitmap;
	entry->error = message->error ? message->error : (entry->bitmap ? kFskErrNone : kFskErrNoData);
	target->bitmap = NULL;
	entry->message = NULL;
	self->activeIndex--;
	KprImageEntryLoaded(entry);
	KprImageCacheIterate(self);
}

void KprImageCachePurge(KprImageCache self)
{
	if (self) {
		UInt32 c = self->size, i;
		for (i = 0; i < c; i++) {
			KprImageEntry *address = self->entries + i;
			KprImageEntry entry;
			while ((entry = *address)) {
				*address = entry->former;
				KprImageEntryDispose(entry);
			}
		}
		self->first = NULL;
		self->last = NULL;
		self->index = 0;
	}
}

void KprImageCachePut(KprImageCache self, KprImageEntry entry)
{
	KprImageEntry *address = self->entries + (entry->sum % self->size);
	entry->cache = self;
	entry->former = *address;
	*address = entry;
	if (self->last) {
		self->last->next = entry;
		entry->previous = self->last;
	}
	else
		self->first = entry;
	self->last = entry;
	self->index++;
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedImageCachePut, entry);
	while (self->index > self->count) {
		KprImageEntry former;
		entry = self->first;
		FskInstrumentedItemSendMessageDebug(self, kprInstrumentedImageCacheRemove, entry);
		self->first = entry->next;
		self->first->previous = NULL;
		address = self->entries + (entry->sum % self->size);
		while ((former = *address)) {
			if (former == entry) {
				*address = entry->former;
				break;
			}
			address = &former->former;
		}
		KprImageEntryDispose(entry);
		self->index--;
	}
}

void KprImageCacheRemove(KprImageCache self, KprImageEntry entry)
{
	KprImageEntry *address = self->entries + (entry->sum % self->size);
	KprImageEntry former, previous, next;
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedImageCacheRemove, entry);
	while ((former = *address)) {
		if (former == entry) {
			*address = entry->former;
			break;
		}
		address = &former->former;
	}
	previous = entry->previous;
	next = entry->next;
	if (previous)
		previous->next = next;
	else
		self->first = next;
	if (next)
		next->previous = previous;
	else
		self->last = previous;
	self->index--;
	entry->former = NULL;
	entry->next = NULL;
	entry->previous = NULL;
	KprImageEntryDispose(entry);
}

UInt32 KprImageCacheSum(char* url)
{
	UInt32 sum = 0;
	if (url) {
		unsigned char* p = (unsigned char*)url;
		while (*p) {
			sum = (sum << 1) + *p++;
		}
	}
	return sum;
}

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprImageEntryInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprImageEntry", FskInstrumentationOffset(KprImageEntryRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

void KprImageEntryProgress(char* url, FskBitmap bitmap)
{
	KprImageCache cache = gThumbnailImageCache;
	UInt32 sum = 0;
	KprImageEntry self = KprImageCacheGet(cache, url, kprImageFill, 0, 0, &sum);
	if (!self) {
		if (kFskErrNone == FskMemPtrNewClear(sizeof(KprImageEntryRecord), &self)){
			FskInstrumentedItemNew(self, NULL, &KprImageEntryInstrumentation);
			self->url = url;
			url = NULL;
			self->aspect = kprImageFill;
			self->width = 46;
			self->height = 46;
			self->sum = sum;
			KprImageCachePut(cache, self);
			self->bitmap = bitmap;
			bitmap = NULL;
		}
	}
	FskBitmapDispose(bitmap);
	FskMemPtrDispose(url);
}

FskErr KprImageEntryNew(KprImageEntry *it, KprImageCache cache, KprImage image, UInt32 aspect, UInt32 width, UInt32 height)
{
	FskErr err = kFskErrNone;
	KprImageEntry self = NULL;
	KprImageLink link = NULL;
	UInt32 sum = 0;
	bailIfError(KprImageLinkNew(&link, image));
	if (width && height) {
		width = FskPortUInt32Scale(gShell->port, width);
		height = FskPortUInt32Scale(gShell->port, height);
	}
	if (cache)
		self = KprImageCacheGet(cache, image->url, aspect, width, height, &sum);
	if (self)
		*it = self;
	else {
		bailIfError(FskMemPtrNewClear(sizeof(KprImageEntryRecord), it));
		self = *it;
		FskInstrumentedItemNew(self, NULL, &KprImageEntryInstrumentation);
		if (image->url) {
			self->url = FskStrDoCopy(image->url);
			bailIfNULL(self->url);
		}
		if (image->mime) {
			self->mime = FskStrDoCopy(image->mime);
			bailIfNULL(self->mime);
		}
		self->aspect = aspect;
		self->width = width;
		self->height = height;
		if (cache) {
			self->sum = sum;
			KprImageCachePut(cache, self);
		}
	}
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedImageEntryAdd, image);
	image->entry = self;
	link->next = self->first;
	self->first = link;
	FskInstrumentedItemSetOwner(link, self);
	link = NULL;
	if (self->bitmap)
		KprImageEntryInvalidate((KprContent)image);
	else if (!self->error && !self->message && self->url)
		self->error = KprImageEntryLoad(self, self->url, self->mime);
	self = NULL;
bail:
	KprImageLinkDispose(link);
	KprImageEntryDispose(self);
	return err;
}

void KprImageEntryDispose(KprImageEntry self)
{
	if (self) {
		KprImageLink link = self->first;
		while (link) {
			KprImageLink next = link->next;
			KprImage image = link->image;
			image->entry = NULL;
			KprImageLinkDispose(link);
			link = next;
		}
		if (self->message) {
			if (self->cache)
				self->cache->activeIndex--;
			KprMessageCancel(self->message);
		}
		FskMemPtrDispose(self->mime);
		FskMemPtrDispose(self->url);
		FskBitmapDispose(self->bitmap);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprImageEntryInvalidate(KprContent content)
{
	UInt32 flags = 0;
	if ((content->coordinates.horizontal & kprLeftRightWidth) < kprLeftRight)
		flags |= kprWidthChanged;
	if ((content->coordinates.vertical & kprTopBottomHeight) < kprTopBottom)
		flags |= kprHeightChanged;
	if (flags)
		KprContentReflow(content, flags);
	else
		KprContentInvalidate(content);
}

FskErr KprImageEntryLoad(KprImageEntry self, char* url, char* mime)
{
	if (url) {
		//if (!FskStrCompareWithLength(url, "file://", 7))
		//	return KprImageEntryLoadFile(self, url, mime);
		if ((!FskStrCompareWithLength(url, "file://", 7))
#if TARGET_OS_IPHONE
		|| (!FskStrCompareWithLength(url, "assets-library://", 17))
#endif
		|| (!FskStrCompareWithLength(url, "http://", 7))
		|| (!FskStrCompareWithLength(url, "https://", 8))
		|| (!FskStrCompareWithLength(url, "xkpr://", 7))
		|| (!FskStrCompareWithLength(url, "xkpr-library://", 15))
		|| (!FskStrCompareWithLength(url, "xmpp://", 7))) {
            if (self->cache)
                return KprImageCacheIterate(self->cache);
            else
                return KprImageEntryLoadMessage(self, url, mime);
        }
	}
	return kFskErrUnimplemented;
}

void KprImageEntryLoaded(KprImageEntry self)
{
	KprImageLink link = self->first;
#if FSKBITMAP_OPENGL
    if (self->bitmap) {
        FskBitmapSetOpenGLSourceAccelerated(self->bitmap, true);
#if !TARGET_OS_ANDROID
        (void)FskBitmapSetSourceDiscardable(self->bitmap);
#endif
    }
#endif
	while (link) {
        KprImageLink next = link->next;
		KprContent content = (KprContent)link->image;
		KprImageEntryInvalidate(content);
		//if (self->flags & kprDisplaying) {
			kprDelegateLoaded(content);
		//}
		link = next;
	}
}

/*
FskErr KprImageEntryLoadFile(KprImageEntry self, char* url, char* mime)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFileMapping map = NULL;
	unsigned char *data;
	FskInt64 size;
	char* sniff = NULL;
	bailIfError(KprURLToPath(url, &path));
	bailIfError(FskFileMap(path, &data, &size, 0, &map));
	if (!mime) {
		bailIfError(FskImageDecompressSniffForMIME(data, (UInt32)size, NULL, url, &sniff));
		mime = sniff;
	}
	bailIfError(FskImageDecompressDataWithOrientation(data, (UInt32)size, mime, NULL, self->width, self->height, NULL, NULL, &self->bitmap));
	if (self->width && self->height)
		KprImageTargetScale(&self->bitmap, self->aspect, self->width, self->height);
	KprImageEntryLoaded(self);
bail:
	FskMemPtrDispose(sniff);
	FskFileDisposeMap(map);
	FskMemPtrDispose(path);
	return err;
}
*/

FskErr KprImageEntryLoadMessage(KprImageEntry self, char* url, char* mime UNUSED)
{
	FskErr err = kFskErrNone;
	KprMessage message = NULL;
	KprImageLink link = self->first;
	KprImageTarget target = NULL;

	bailIfError(KprMessageNew(&message, url));
	while (link) {
        KprImageLink next = link->next;
		KprContent content = (KprContent)link->image;
		kprDelegateInvoke(content, message);
		link = next;
	}
	bailIfError(KprImageTargetNew(&target, self->aspect, self->width, self->height));
	KprMessageSetStream(message, (KprStream)target);
	self->message = message;
	KprMessageInvoke(message, KprImageEntryLoadMessageComplete, NULL, self);
bail:
	return err;
}

void KprImageEntryLoadMessageComplete(KprMessage message, void* it)
{
	KprImageEntry self = it;
	KprImageTarget target = (KprImageTarget)message->stream;
	self->bitmap = target->bitmap;
	self->error = message->error ? message->error : (self->bitmap ? kFskErrNone : kFskErrNoData);
	target->bitmap = NULL;
	self->message = NULL;
	KprImageEntryLoaded(self);
}

void KprImageEntryRefresh(KprImageEntry self, Boolean delegate UNUSED)
{
	if (self) {
		KprImageCache cache = self->cache;
		if (cache) {
			//if (delegate)
			//	self->flags |= kprDisplaying;
			if (cache->last != self) {
				KprImageEntry previous = self->previous;
				KprImageEntry next = self->next;
				if (previous)
					previous->next = next;
				else
					cache->first = next;
				if (next)
					next->previous = previous;
				else
					cache->last = previous;
				cache->last->next = self;
				self->previous = cache->last;
				cache->last = self;
				self->next = NULL;
			}
		}
	}
}

void KprImageEntryRemove(KprImageEntry self, KprImage image)
{
	KprImageLink *linkAddress = &self->first, link;
	while ((link = *linkAddress)) {
		if (link->image == image) {
			*linkAddress = link->next;
			KprImageLinkDispose(link);
			break;
		}
		linkAddress = &link->next;
	}
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedImageEntryRemove, image);
	if (!self->first) {
		if (self->cache)
			KprImageCacheRemove(self->cache, self);
		else
			KprImageEntryDispose(self);
	}
}

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprImageLinkInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprImageLink", FskInstrumentationOffset(KprImageLinkRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprImageLinkNew(KprImageLink *it, KprImage image)
{
	FskErr err = kFskErrNone;
	KprImageLink self;
	bailIfError(FskMemPtrNewClear(sizeof(KprImageLinkRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprImageLinkInstrumentation);
	self->image = image;
bail:
	return err;
}

void KprImageLinkDispose(KprImageLink self)
{
	if (self) {
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprImageTargetInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprImageTarget", FskInstrumentationOffset(KprImageTargetRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
KprStreamDispatchRecord kprImageStreamDispatchRecord = {
	KprImageTargetDispose,
	NULL,
	NULL,
	NULL,
	KprImageTargetTransform
};

FskErr KprImageTargetNew(KprImageTarget* it, UInt32 aspect, UInt32 width, UInt32 height)
{
	FskErr err = kFskErrNone;
	KprImageTarget self;
	bailIfError(FskMemPtrNewClear(sizeof(KprImageTargetRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprImageTargetInstrumentation);
	self->dispatch = &kprImageStreamDispatchRecord;
	self->aspect = aspect;
	self->width = width;
	self->height = height;
bail:
	return err;
}

void KprImageTargetTransform(void* it, KprMessage message, xsMachine* machine)
{
	FskErr err = kFskErrNone;
	KprImageTarget self = it;
	void* data = message->response.body;
	char* mime = KprMessageGetResponseHeader(message, "content-type");
	UInt32 size = message->response.size;
	char* sniff = NULL;
	UInt32 aspect = self->aspect;
	UInt32 width = self->width;
	UInt32 height = self->height;
	if (message->error) return;
	if (!mime || FskStrCompareWithLength(mime, "image/", 6)) {
        bailIfErrorWithDiagnostic(FskImageDecompressSniffForMIME(data, size, NULL, message->url, &sniff), machine, "Unrecognized image file type %s. Error %s.\n", message->url, FskInstrumentationGetErrorString(err));
		mime = sniff;
	}
	if (!aspect || !width)
		width = gImageMaximumWidthAndHeight;
	if (!aspect || !height)
		height = gImageMaximumWidthAndHeight;
	bailIfErrorWithDiagnostic(FskImageDecompressDataWithOrientation(data, (UInt32)size, mime, NULL, width, height, NULL, NULL, &self->bitmap), machine,
                "Failed to decompress texture %s. Error %s.\n", message->url, FskInstrumentationGetErrorString(err));
	//fprintf(stderr, "%dx%d -> %dx%d %s\n", self->bitmap->bounds.width, self->bitmap->bounds.height, self->width, self->height, message->url);
	if (aspect && self->width && self->height)
		KprImageTargetScale(&self->bitmap, self->aspect, self->width, self->height);
bail:
	FskMemPtrDispose(sniff);
	message->error = err;
}

void KprImageTargetDispose(void* it)
{
	if (it) {
		KprImageTarget self = it;
		FskBitmapDispose(self->bitmap);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprImageTargetScale(FskBitmap* bitmap, UInt32 aspect, UInt32 width, UInt32 height)
{
	FskErr err = kFskErrNone;
	FskBitmap src, dst;
	Boolean hasAlpha, isPremultiplied;
	FskBitmapFormatEnum pixelFormat = kFskBitmapFormatUnknown;
	FskRectangleRecord srcRect, dstRect;
	src = *bitmap;
	FskBitmapGetHasAlpha(src, &hasAlpha);
	FskBitmapGetPixelFormat(src, &pixelFormat);
    FskBitmapGetAlphaIsPremultiplied(src, &isPremultiplied);
	FskBitmapGetBounds(src, &srcRect);
	FskRectangleSet(&dstRect, 0, 0, width, height);
	if (!FskRectangleIsEqual(&srcRect, &dstRect)) {
		KprAspectApply(aspect, &srcRect, &dstRect);
        if (kFskBitmapFormat32A16RGB565LE == pixelFormat)
            pixelFormat = kFskBitmapFormatDefaultRGBA;
		bailIfError(FskBitmapNew(dstRect.width, dstRect.height, pixelFormat, &dst));
		FskBitmapSetHasAlpha(dst, hasAlpha);
		FskBitmapSetAlphaIsPremultiplied(dst, isPremultiplied);
        dstRect.x = dstRect.y = 0;
		FskBitmapDraw(src, &srcRect, dst, &dstRect, NULL, NULL, kFskGraphicsModeCopy | kFskGraphicsModeBilinear, NULL);
		FskBitmapDispose(src);
		*bitmap = dst;
	}
bail:
	return err;
}

/* PICTURE */

static void KprPictureDispose(void* it);
static void KprPictureDraw(void* it, FskPort port, FskRectangle area);
static FskBitmap KprPictureGetBitmap(void* it, FskPort port, Boolean* owned);
static void KprPictureMeasureHorizontally(void* it);
static void KprPictureMeasureVertically(void* it);
static void KprPicturePlaced(void* it);
static void KprPictureUpdate(void* it, FskPort port, FskRectangle area);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprPictureInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprPicture", FskInstrumentationOffset(KprPictureRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprPictureDispatchRecord = {
	"picture",
	KprContentActivated,
	KprContentAdded,
	KprContentCascade,
	KprPictureDispose,
	KprPictureDraw,
	KprContentFitHorizontally,
	KprContentFitVertically,
	KprPictureGetBitmap,
	KprContentHit,
	KprContentIdle,
	KprLayerInvalidated,
	KprContentLayered,
	KprContentMark,
	KprPictureMeasureHorizontally,
	KprPictureMeasureVertically,
	KprContentPlace,
	KprPicturePlaced,
	KprContentPredict,
	KprContentReflowing,
	KprContentRemoving,
	KprContentSetWindow,
	KprContentShowing,
	KprContentShown,
	KprPictureUpdate
};

FskErr KprPictureNew(KprPicture* it,  KprCoordinates coordinates)
{
	FskErr err = kFskErrNone;
	KprPicture self;

	bailIfError(FskMemPtrNewClear(sizeof(KprPictureRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprPictureInstrumentation);
	self->dispatch = &KprPictureDispatchRecord;
	self->flags = kprVisible | kprImageFit;
	KprContentInitialize((KprContent)self, coordinates, NULL, NULL);
	self->corners[1].x = 1;
	self->corners[2].x = 1;
	self->corners[2].y = 1;
	self->corners[3].y = 1;
	self->opacity = 1;
	self->scale.x = 1;
	self->scale.y = 1;
	KprLayerComputeMatrix((KprLayer)self);
bail:
	return err;
}

void KprPictureSetURL(KprPicture self, char* url, char* mime)
{
	FskErr err = kFskErrNone;
	KprContentInvalidate((KprContent)self);
	if (self->entry) {
		KprImageEntryRemove(self->entry, (KprImage)self);
		self->entry = NULL;
	}
	FskMemPtrDisposeAt(&self->mime);
	FskMemPtrDisposeAt(&self->url);
	if (url) {
		self->url = FskStrDoCopy(url);
		bailIfNULL(url);
	}
	if (mime) {
		self->mime = FskStrDoCopy(mime);
		bailIfNULL(self->mime);
	}
	if (self->url) {
		UInt32 aspect = self->flags & (kprImageFill | kprImageFit);
		UInt32 width = (self->coordinates.horizontal & kprWidth) ? self->coordinates.width : 0;
		UInt32 height = (self->coordinates.vertical & kprHeight) ? self->coordinates.height : 0;
		bailIfError(KprImageEntryNew(&self->entry, gPictureImageCache, (KprImage)self, aspect, width, height));
	}
bail:
	KprContentReflow((KprContent)self, kprSizeChanged);
}

/* PICTURE DISPATCH */

void KprPictureDispose(void* it)
{
	KprPicture self = it;
	KprAssetUnbind(self->effect);
	if (self->entry)
		KprImageEntryRemove(self->entry, (KprImage)self);
	FskMemPtrDispose(self->mime);
	FskMemPtrDispose(self->url);
	FskMemPtrDispose(self->hitMatrix);
	KprContentDispose(it);
}

void KprPictureDraw(void* it, FskPort port, FskRectangle area UNUSED)
{
	KprPicture self = it;
	if (self->entry) {
		FskBitmap bitmap = self->entry->bitmap;
		if (bitmap) {
			FskRectangleRecord srcRect, dstRect;
			FskRectangleSet(&dstRect, 0, 0, self->bounds.width, self->bounds.height);
			FskBitmapGetBounds(bitmap, &srcRect);
			if (!FskRectangleIsEmpty(&self->crop))
				FskRectangleIntersect(&srcRect, &self->crop, &srcRect);
			KprAspectApply(self->flags, &srcRect, &dstRect);
			KprLayerBlit(it, port, bitmap, &srcRect, &dstRect);
		}
	}
	return;
}

FskBitmap KprPictureGetBitmap(void* it, FskPort port, Boolean* owned)
{
	FskErr err = kFskErrNone;
	KprPicture self = it;
	FskBitmap result = NULL;
	if (self->entry && self->entry->bitmap) {
		if (self->effect && self->effect->compound) {
			*owned = false;
			bailIfError(KprEffectApply(self->effect, self->entry->bitmap, port, &result));
		}
		else {
			*owned = true;
			result = self->entry->bitmap;
		}
	}
bail:
	return result;
}

void KprPictureMeasureHorizontally(void* it)
{
	KprPicture self = it;
	UInt16 horizontal = self->coordinates.horizontal;
	if (!(horizontal & kprWidth)) {
		if (self->entry && self->entry->bitmap) {
			FskRectangleRecord bounds;
			FskBitmapGetBounds(self->entry->bitmap, &bounds);
			self->coordinates.width = bounds.width;
		}
		else
			self->coordinates.width = 0;
	}
}

void KprPictureMeasureVertically(void* it)
{
	KprPicture self = it;
	UInt16 vertical = self->coordinates.vertical;
	if (!(vertical & kprHeight)) {
		if (self->entry && self->entry->bitmap) {
			FskRectangleRecord bounds;
			FskBitmapGetBounds(self->entry->bitmap, &bounds);
			self->coordinates.height = bounds.height;
		}
		else
			self->coordinates.height = 0;
	}
}

void KprPicturePlaced(void* it)
{
	KprLayer self = it;
	if (self->flags & kprMatrixChanged)
		KprLayerComputeMatrix(self);
	KprContentPlaced(it);
}

void KprPictureUpdate(void* it, FskPort port, FskRectangle area)
{
	KprPicture self = it;
	Boolean flag = false;
	if (self->flags & kprVisible) {
		FskDCoordinate *m = &self->matrix[0][0];
		FskRectangleRecord bounds;
		FskRectangleSet(&bounds, 0, 0, self->bounds.width, self->bounds.height);
		if (!m[1] && !m[2] && !m[3] && !m[5]) {
			FskDCoordinate left = ((FskDCoordinate)bounds.x * m[0]) + m[6];
			FskDCoordinate top = ((FskDCoordinate)bounds.y * m[4]) + m[7];
			FskDCoordinate right = (((FskDCoordinate)bounds.x + (FskDCoordinate)bounds.width) * m[0]) + m[6];
			FskDCoordinate bottom = (((FskDCoordinate)bounds.y + (FskDCoordinate)bounds.height) * m[4]) + m[7];
			FskRectangleSet(&bounds, (SInt32)(left), (SInt32)(top), (SInt32)ceil(right - left), (SInt32)ceil(bottom - top));
			FskRectangleOffset(&bounds, self->bounds.x, self->bounds.y);
			flag = FskRectangleIsIntersectionNotEmpty(&bounds, area);
		}
		else
			flag = true;
	}
	if (flag) {
		FskPortOffsetOrigin(port, self->bounds.x, self->bounds.y);
		FskRectangleOffset(area, -self->bounds.x, -self->bounds.y);
		(*self->dispatch->draw)(self, port, area);
		FskRectangleOffset(area, self->bounds.x, self->bounds.y);
		FskPortOffsetOrigin(port, -self->bounds.x, -self->bounds.y);
	}
	if (self->flags & kprDisplayed) {
		if (self->shell->port == port) {
			self->flags &= ~kprDisplayed;
			kprDelegateDisplayed(self);
		}
	}
}

/* PICTURE ECMASCRIPT */

void KPR_Picture(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprPicture self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	xsThrowIfFskErr(KprPictureNew(&self, &coordinates));
	kprContentConstructor(KPR_Picture);
	if (c > 2)
		xsCall2_noResult(xsThis, xsID_load, xsArg(1), xsArg(2));
	else if (c > 1)
		xsCall1_noResult(xsThis, xsID_load, xsArg(1));
}

void KPR_picture_get_aspect(xsMachine *the)
{
	KprPicture self = xsGetHostData(xsThis);
	if (self->flags & kprImageFill) {
		if (self->flags & kprImageFit)
			xsResult = xsString("stretch");
		else
			xsResult = xsString("fill");
	}
	else {
		if (self->flags & kprImageFit)
			xsResult = xsString("fit");
		else
			xsResult = xsString("draw");
	}
}

void KPR_picture_get_crop(xsMachine *the)
{
	KprPicture self = xsGetHostData(xsThis);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsInteger(self->crop.x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsInteger(self->crop.y), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(self->crop.width), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(self->crop.height), xsDefault, xsDontScript);
}

void KPR_picture_get_ready(xsMachine *the)
{
	KprPicture self = xsGetHostData(xsThis);
	xsResult = (self->entry && self->entry->bitmap) ? xsTrue : xsFalse;
}

void KPR_picture_get_size(xsMachine *the)
{
	KprPicture self = xsGetHostData(xsThis);
	if (self->shell) {
		KprShellAdjust(self->shell);
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID_width, xsInteger(self->bounds.width), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_height, xsInteger(self->bounds.height), xsDefault, xsDontScript);
	}
	else if (self->entry && self->entry->bitmap) {
		FskRectangleRecord bounds;
		FskBitmapGetBounds(self->entry->bitmap, &bounds);
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID_width, xsInteger(bounds.width), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_height, xsInteger(bounds.height), xsDefault, xsDontScript);
	}
}

void KPR_picture_get_url(xsMachine *the)
{
	KprPicture self = xsGetHostData(xsThis);
	char* url = self->url;
	if (url)
		xsResult = xsString(url);
}

void KPR_picture_get_width(xsMachine *the)
{
	KprPicture self = xsGetHostData(xsThis);
	if (self->shell) {
		KprShellAdjust(self->shell);
		xsResult = xsInteger(self->bounds.width);
	}
	else if (self->entry && self->entry->bitmap) {
		FskRectangleRecord bounds;
		FskBitmapGetBounds(self->entry->bitmap, &bounds);
		xsResult = xsInteger(bounds.width);
	}
}

void KPR_picture_get_height(xsMachine *the)
{
	KprPicture self = xsGetHostData(xsThis);
	if (self->shell) {
		KprShellAdjust(self->shell);
		xsResult = xsInteger(self->bounds.height);
	}
	else if (self->entry && self->entry->bitmap) {
		FskRectangleRecord bounds;
		FskBitmapGetBounds(self->entry->bitmap, &bounds);
		xsResult = xsInteger(bounds.height);
	}
}

void KPR_picture_set_aspect(xsMachine *the)
{
	KprPicture self = xsGetHostData(xsThis);
	char* aspect = xsToString(xsArg(0));
	self->flags &= ~(kprImageFill | kprImageFit);
	if (!FskStrCompare(aspect, "fill"))
		self->flags |= kprImageFill;
	else if (!FskStrCompare(aspect, "fit"))
		self->flags |= kprImageFit;
	else if (!FskStrCompare(aspect, "stretch"))
		self->flags |= kprImageFill | kprImageFit;
	KprContentInvalidate((KprContent)self);
}

void KPR_picture_set_crop(xsMachine *the)
{
	KprPicture self = xsGetHostData(xsThis);
	Boolean flag = false;
	if (xsTest(xsArg(0))) {
		xsEnterSandbox();
		flag |= xsFindInteger(xsArg(0), xsID_x, &self->crop.x);
		flag |= xsFindInteger(xsArg(0), xsID_y, &self->crop.y);
		flag |= xsFindInteger(xsArg(0), xsID_width, &self->crop.width);
		flag |= xsFindInteger(xsArg(0), xsID_height, &self->crop.height);
		xsLeaveSandbox();
	}
	else {
		flag = true;
		self->crop.x = 0;
		self->crop.y = 0;
		self->crop.width = 0;
		self->crop.height = 0;
	}
	if (flag)
		KprLayerMatrixChanged((KprLayer)self);
}

void KPR_picture_set_url(xsMachine *the)
{
	xsCall1_noResult(xsThis, xsID_load, xsArg(0));
}

void KPR_picture_load(xsMachine *the)
{
	FskErr err = kFskErrNone;
	xsIntegerValue c = xsToInteger(xsArgc);
	KprPicture self = kprGetHostData(xsThis, this, picture);
	char* url = NULL;
	void* data = NULL;
	UInt32 size = 0;
	UInt32 infoSize = 0;
	char* mime = NULL;
	void* info = NULL;
	char* sniff = NULL;
	UInt32 format = 0;
	FskImageDecompress deco = NULL;
	FskBitmap bitmap = NULL;

	if ((c > 0) && xsTest(xsArg(0))) {
		if (xsTypeOf(xsArg(0)) == xsStringType) {
			xsStringValue reference = xsToString(xsArg(0));
			xsStringValue base = xsToString(xsModuleURL);
			bailIfError(KprURLMerge(base, reference, &url));
		}
		else {
			data = xsGetHostData(xsArg(0));
			size = xsToInteger(xsGet(xsArg(0), xsID_length));
		}
	}
	if ((c > 1) && xsTest(xsArg(1)))
		mime = xsToString(xsArg(1));
	if ((c > 2) && xsTest(xsArg(2))) {
		info = xsGetHostData(xsArg(2));
		infoSize = xsToInteger(xsGet(xsArg(2), xsID_length));
	}
	if (url) {
		KprPictureSetURL(self, url, mime);
	}
	else if (data) {
		if (!mime) {
			bailIfErrorWithDiagnostic(FskImageDecompressSniffForMIME(data, size, NULL, NULL, &sniff), self->the, "Unrecognized picture file type %s\n", url);
			mime = sniff;
		}
		if (0 == FskStrCompareWithLength(mime, "fourcc:", 7)) {
			FskMemMove(&format, mime + 7, 4);
			format = FskEndianU32_BtoN(format);
			mime = NULL;
		}
		bailIfError(FskImageDecompressNew(&deco, format, mime, NULL));
		if (info) {
			FskMediaPropertyValueRecord property;
			property.type = kFskMediaPropertyTypeData;
			property.value.data.data = info;
			property.value.data.dataSize = infoSize;
			bailIfError(FskImageDecompressSetProperty(deco, kFskMediaPropertyFormatInfo, &property));
		}
		bailIfErrorWithDiagnostic(FskImageDecompressFrame(deco, data, size, &bitmap, true, NULL, NULL, NULL, NULL, NULL, NULL, kFskImageFrameTypeSync), self->the, "Failed to decompress picture %s", url);
		KprContentInvalidate((KprContent)self);
		if (self->entry) {
			KprImageEntryRemove(self->entry, (KprImage)self);
			self->entry = NULL;
		}
		FskMemPtrDisposeAt(&self->mime);
		FskMemPtrDisposeAt(&self->url);
		bailIfError(KprImageEntryNew(&self->entry, gPictureImageCache, (KprImage)self, 0, 0, 0));
		self->entry->bitmap = bitmap;
		KprImageEntryLoaded(self->entry);
	}
	else
		KprPictureSetURL(self, NULL, NULL);
bail:
	FskImageDecompressDispose(deco);
	FskMemPtrDispose(sniff);
	FskMemPtrDispose(url);
}

/* THUMBNAIL */

static void KprThumbnailDraw(void* it, FskPort port, FskRectangle area);
static FskBitmap KprThumbnailGetBitmap(void* it, FskPort port, Boolean* owned);
static void KprThumbnailPredict(void* it, FskRectangle area);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprThumbnailInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprThumbnail", FskInstrumentationOffset(KprThumbnailRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprThumbnailDispatchRecord = {
	"thumbnail",
	KprContentActivated,
	KprContentAdded,
	KprContentCascade,
	KprPictureDispose,
	KprThumbnailDraw,
	KprContentFitHorizontally,
	KprContentFitVertically,
	KprThumbnailGetBitmap,
	KprContentHit,
	KprContentIdle,
	KprLayerInvalidated,
	KprContentLayered,
	KprContentMark,
	KprPictureMeasureHorizontally,
	KprPictureMeasureVertically,
	KprContentPlace,
	KprPicturePlaced,
	KprThumbnailPredict,
	KprContentReflowing,
	KprContentRemoving,
	KprContentSetWindow,
	KprContentShowing,
	KprContentShown,
	KprPictureUpdate
};

FskErr KprThumbnailNew(KprThumbnail* it,  KprCoordinates coordinates)
{
	FskErr err = kFskErrNone;
	KprThumbnail self;

	bailIfError(FskMemPtrNewClear(sizeof(KprThumbnailRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprThumbnailInstrumentation);
	self->dispatch = &KprThumbnailDispatchRecord;
	self->flags = kprVisible | kprImageFill;
	KprContentInitialize((KprContent)self, coordinates, NULL, NULL);
	self->corners[1].x = 1;
	self->corners[2].x = 1;
	self->corners[2].y = 1;
	self->corners[3].y = 1;
	self->opacity = 1;
	self->scale.x = 1;
	self->scale.y = 1;
	KprLayerComputeMatrix((KprLayer)self);
bail:
	return err;
}

void KprThumbnailSetURL(KprThumbnail self, char* url, char* mime)
{
	FskErr err = kFskErrNone;
	KprContentInvalidate((KprContent)self);
	if (self->entry) {
		KprImageEntryRemove(self->entry, (KprImage)self);
		self->entry = NULL;
	}
	FskMemPtrDisposeAt(&self->mime);
	FskMemPtrDisposeAt(&self->url);
	if (url) {
		self->url = FskStrDoCopy(url);
		bailIfNULL(url);
	}
	if (mime) {
		self->mime = FskStrDoCopy(mime);
		bailIfNULL(mime);
	}
bail:
	KprContentReflow((KprContent)self, kprSizeChanged);
}

/* THUMBNAIL DISPATCH */

void KprThumbnailDraw(void* it, FskPort port, FskRectangle area)
{
	KprThumbnail self = it;
	if (self->entry)
		KprImageEntryRefresh(self->entry, false);
	else if (self->url) {
		UInt32 aspect = self->flags & (kprImageFill | kprImageFit);
		KprImageEntryNew(&self->entry, gThumbnailImageCache, (KprImage)self, aspect, self->bounds.width, self->bounds.height);
	}
	KprPictureDraw(it, port, area);
}

FskBitmap KprThumbnailGetBitmap(void* it, FskPort port, Boolean* owned)
{
	FskErr err = kFskErrNone;
	KprThumbnail self = it;
	if (self->url && !self->entry) {
		UInt32 aspect = self->flags & (kprImageFill | kprImageFit);
		bailIfError(KprImageEntryNew(&self->entry, gThumbnailImageCache, (KprImage)self, aspect, self->bounds.width, self->bounds.height));
	}
	KprImageEntryRefresh(self->entry, true);
bail:
	return KprPictureGetBitmap(it, port, owned);
}

void KprThumbnailPredict(void* it, FskRectangle area)
{
	FskErr err = kFskErrNone;
	KprThumbnail self = it;
	if ((self->flags & kprVisible) && FskRectangleIsIntersectionNotEmpty(KprBounds((KprContent)self), area)) {
		if (self->url && !self->entry) {
			UInt32 aspect = self->flags & (kprImageFill | kprImageFit);
			bailIfError(KprImageEntryNew(&self->entry, gThumbnailImageCache, (KprImage)self, aspect, self->bounds.width, self->bounds.height));
		}
		KprImageEntryRefresh(self->entry, false);
	}
bail:
	return;
}

/* THUMBNAIL ECMASCRIPT */

void KPR_Thumbnail(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprThumbnail self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	xsThrowIfFskErr(KprThumbnailNew(&self, &coordinates));
	kprContentConstructor(KPR_Thumbnail);
	if (c > 2)
		xsCall2_noResult(xsThis, xsID_load, xsArg(1), xsArg(2));
	else if (c > 1)
		xsCall1_noResult(xsThis, xsID_load, xsArg(1));
}

void KPR_thumbnail_get_url(xsMachine *the)
{
	KprThumbnail self = xsGetHostData(xsThis);
	char* url = self->url;
	if (url)
		xsResult = xsString(url);
}

void KPR_thumbnail_set_url(xsMachine *the)
{
	xsCall1_noResult(xsThis, xsID_load, xsArg(0));
}

void KPR_thumbnail_load(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprThumbnail self = kprGetHostData(xsThis, this, thumbnail);
	char* url = NULL;
	char* mime = NULL;
	xsTry {
		if ((c > 0) && xsTest(xsArg(0))) {
			xsStringValue reference = xsToString(xsArg(0));
			xsStringValue base = xsToString(xsModuleURL);
			xsThrowIfFskErr(KprURLMerge(base, reference, &url));
		}
		if ((c > 1) && xsTest(xsArg(1)))
			mime = xsToString(xsArg(1));
		KprThumbnailSetURL(self, url, mime);
		FskMemPtrDispose(url);
	}
	xsCatch {
		FskMemPtrDispose(url);
	}
}

void KPR_Thumbnail_clear(xsMachine *the)
{
	char *url = xsToString(xsArg(0));
	KprImageCacheClear(gThumbnailImageCache, url);
}

void KPR_Thumbnail_getCacheSize(xsMachine *the)
{
	xsResult = xsInteger(gThumbnailImageCache->count);
}

void KPR_Thumbnail_setCacheSize(xsMachine *the)
{
	gThumbnailImageCache->count = xsToInteger(xsArg(0));
}

void KPR_Thumbnail_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("Thumbnail"));
	xsNewHostProperty(xsResult, xsID("clear"), xsNewHostFunction(KPR_Thumbnail_clear, 1), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("getCacheSize"), xsNewHostFunction(KPR_Thumbnail_getCacheSize, 0), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("setCacheSize"), xsNewHostFunction(KPR_Thumbnail_setCacheSize, 1), xsDefault, xsDontScript);
}





