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
//#include "fips180.h"
#include <time.h>
#include "md5.h"

#define __FSKBITMAP_PRIV__
#define __FSKPORT_PRIV__
#define __FSKWINDOW_PRIV__
#define __FSKECMASCRIPT_PRIV__

#include "FskBitmap.h"
#include "FskEnvironment.h"
#if FSKBITMAP_OPENGL
#include "FskGLBlit.h"
#endif
#include "FskHTTPClient.h"
#include "FskPixelOps.h"
#include "FskPerspective.h"
#include "FskPort.h"
#include "FskRectBlit.h"
#include "FskECMAScript.h"

#if TARGET_OS_WIN32
#include "Windows.h"
#include "FskTextConvert.h"
#include "FskUtilities.h"
#define SECURITY_WIN32
#include "security.h"
#elif TARGET_OS_IPHONE
#include "FskCocoaSupportPhone.h"
#elif TARGET_OS_MAC
#include "FskCocoaSupport.h"
#elif TARGET_OS_ANDROID
#include "FskHardware.h"
#else
#include "unistd.h"
#endif

#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHTTPCache.h"
#include "kprHTTPClient.h"
#include "kprHTTPServer.h"
#include "kprImage.h"
#include "kprLayer.h"
#include "kprMessage.h"
#include "kprSkin.h"
#include "kprShell.h"
#include "kprText.h"
#include "kprTransition.h"
#include "kprUtilities.h"

#if SUPPORT_INSTRUMENTATION
typedef struct {
	KprSlotPart;
} KprVolatileRecord, *KprVolatile;

Boolean KprInsrumentationFormatMessage(FskInstrumentedType dispatch UNUSED, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
	case kprInstrumentedModuleRequire:
		snprintf(buffer, bufferSize, "require \"%s\"", (char*)msgData);
		return true;
	case kprInstrumentedVolatileConstruct:
		snprintf(buffer, bufferSize, "construct (the: %p)", ((KprVolatile)msgData)->the);
		return true;
	case kprInstrumentedVolatileDestruct:
		snprintf(buffer, bufferSize, "destruct (the: %p)", ((KprVolatile)msgData)->the);
		return true;
	case kprInstrumentedVolatileReconstruct:
		snprintf(buffer, bufferSize, "reconstruct (the: %p)", ((KprVolatile)msgData)->the);
		return true;

	case kprInstrumentedContentBeginCollect:
		snprintf(buffer, bufferSize, "begin collect machine (the: %p)", ((KprContent)msgData)->the);
		return true;
	case kprInstrumentedContentBeginPurge:
		snprintf(buffer, bufferSize, "begin purge assets (the: %p)", ((KprContent)msgData)->the);
		return true;
	case kprInstrumentedContentClose:
		snprintf(buffer, bufferSize, "close (the: %p)", ((KprContent)msgData)->the);
		return true;
	case kprInstrumentedContentCreateMachine:
		snprintf(buffer, bufferSize, "create machine (the: %p)", ((KprContent)msgData)->the);
		return true;
	case kprInstrumentedContentDeleteMachine:
		snprintf(buffer, bufferSize, "delete machine (the: %p)", ((KprContent)msgData)->the);
		return true;
	case kprInstrumentedContentEndCollect:
		snprintf(buffer, bufferSize, "end collect machine (the: %p)", ((KprContent)msgData)->the);
		return true;
	case kprInstrumentedContentEndPurge:
		snprintf(buffer, bufferSize, "end purge assets (the: %p)", ((KprContent)msgData)->the);
		return true;
	case kprInstrumentedContentPutBehavior:
		snprintf(buffer, bufferSize, "put behavior %p (the: %p, slot: %p)", msgData, ((KprScriptBehavior)msgData)->the, &(((KprScriptBehavior)msgData)->slot));
		return true;
	case kprInstrumentedContentRemoveBehavior:
		snprintf(buffer, bufferSize, "remove behavior %p (the: %p, slot: %p)", msgData, ((KprScriptBehavior)msgData)->the, &(((KprScriptBehavior)msgData)->slot));
		return true;

	case kprInstrumentedAssetBind:
		snprintf(buffer, bufferSize, "bind %d (the: %p)", (int) ((KprAsset)msgData)->usage, ((KprAsset)msgData)->the);
		return true;
	case kprInstrumentedAssetUnbind:
		snprintf(buffer, bufferSize, "unbind %d (the: %p)", (int) ((KprAsset)msgData)->usage, ((KprAsset)msgData)->the);
		return true;
	case kprInstrumentedImageCacheGet:
		snprintf(buffer, bufferSize, "get %p %s", msgData, ((KprImageEntry)msgData)->url);
		return true;
	case kprInstrumentedImageCachePut:
		snprintf(buffer, bufferSize, "put %p %s", msgData, ((KprImageEntry)msgData)->url);
		return true;
	case kprInstrumentedImageCacheRemove:
		snprintf(buffer, bufferSize, "remove %p %s", msgData, ((KprImageEntry)msgData)->url);
		return true;
	case kprInstrumentedImageEntryAdd:
		snprintf(buffer, bufferSize, "add %p %s", msgData, ((KprImage)msgData)->url);
		return true;
	case kprInstrumentedImageEntryRemove:
		snprintf(buffer, bufferSize, "remove %p %s", msgData, ((KprImage)msgData)->url);
		return true;

	case kprInstrumentedContentCallBehavior:
		{
			void** params = msgData;
			snprintf(buffer, bufferSize, "call behavior %p %s (the: %p)", params[0], (char*)params[1], ((KprScriptBehavior)params[0])->the);
		}
		return true;

	case kprInstrumentedTextDumpBlock:
		{
			KprTextBlock block = msgData;
			snprintf(buffer, bufferSize, "Block offset: %d style %p count: %d", (int) block->offset, block->style, (int) block->count);
		}
		return true;
	case kprInstrumentedTextDumpLine:
		{
			KprTextLine line = msgData;
			snprintf(buffer, bufferSize, "Line y: %d ascent: %d descent: %d x: %d width: %d portion: %d slop: %d count: %d", (int)line->y, line->ascent, line->descent, line->x, line->width, line->portion, line->slop, line->count);
		}
		return true;
	case kprInstrumentedTextDumpRun:
		{
			void** params = msgData;
			KprTextRun run = params[0];
			if (run->span.length >= 0)
				snprintf(buffer, bufferSize, "    Span offset: %d length: %d style %p link %p \"%s\"", (int) run->span.offset, (int) run->span.length, run->span.style, run->span.link, (char*)params[1]);
			else
				snprintf(buffer, bufferSize, "    Item offset: %d x: %d y: %d width: %d height: %d adjustment: %d", (int) run->item.offset, (int) run->item.content->bounds.x, (int) run->item.content->bounds.y, (int) run->item.content->bounds.width, (int) run->item.content->bounds.height, run->item.adjustment);
		}
		return true;

	case kprInstrumentedWindowInvalidated:
		snprintf(buffer, bufferSize, "invalidated %d %d %d %d", (int) ((FskRectangle)msgData)->x, (int) ((FskRectangle)msgData)->y, (int) ((FskRectangle)msgData)->width, (int) ((FskRectangle)msgData)->height);
		return true;

	case kprInstrumentedMessageCancel:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "cancel %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;
	case kprInstrumentedMessageComplete:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "complete %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;
	case kprInstrumentedMessageConstruct:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "construct %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;
	case kprInstrumentedMessageDestruct:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "destruct %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;
	case kprInstrumentedMessageInvoke:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "invoke %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;
	case kprInstrumentedMessageNotify:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "notify %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;
	case kprInstrumentedMessageRedirect:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "redirect %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;
	case kprInstrumentedMessageResume:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "resume %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;
	case kprInstrumentedMessageSuspend:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "suspend %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;

	case kprInstrumentedMessageHTTPBegin:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "HTTP begin %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;
	case kprInstrumentedMessageHTTPContinue:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "HTTP continue %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;
	case kprInstrumentedMessageHTTPEnd:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "HTTP end %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;

	case kprInstrumentedMessageLibraryBegin:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "Library begin %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;
	case kprInstrumentedMessageLibraryEnd:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "Library end %d \"%.64s\"", (int) self->usage, self->url);
		}
		return true;

	case kprInstrumentedHTTPConnectionAvailable:
		snprintf(buffer, bufferSize, "available %p", msgData);
		return true;
	case kprInstrumentedHTTPConnectionCandidate:
		snprintf(buffer, bufferSize, "candidate %p", msgData);
		return true;
	case kprInstrumentedHTTPConnectionClose:
		snprintf(buffer, bufferSize, "close %p (error %d)", msgData, (int)((FskHTTPClient)msgData)->status.lastErr);
		return true;
	case kprInstrumentedHTTPConnectionComplete:
		{
			KprMessage self = msgData;
			snprintf(buffer, bufferSize, "complete %d %d %d %s", (int) self->status, (int) self->error, (int) self->response.size, self->url);
		}
		return true;
	case kprInstrumentedHTTPConnectionFinished:
		snprintf(buffer, bufferSize, "finished %p", msgData);
		return true;
	case kprInstrumentedHTTPConnectionHeaders:
		snprintf(buffer, bufferSize, "headers");
		return true;
	case kprInstrumentedHTTPConnectionOpen:
		snprintf(buffer, bufferSize, "open %p", msgData);
		return true;
	case kprInstrumentedHTTPConnectionProcess:
		snprintf(buffer, bufferSize, "process %s", (char*)msgData);
		return true;
	case kprInstrumentedHTTPConnectionReceiving:
		snprintf(buffer, bufferSize, "receiving %d bytes", *((int*)msgData));
		return true;
	case kprInstrumentedHTTPConnectionSending:
		snprintf(buffer, bufferSize, "sending %d bytes", *((int*)msgData));
		return true;

	case kprInstrumentedHTTPCacheValueDisposeData:
		{
			snprintf(buffer, bufferSize, "dispose data");
		}
		return true;
	case kprInstrumentedTextureLoad:
		{
			KprTexture self = msgData;
			FskRectangleRecord bounds;
			FskBitmapGetBounds(self->bitmap, &bounds);
			if (self->url)
				snprintf(buffer, bufferSize, "load bitmap %p %dx%d \"%s\"", self->bitmap, (int)bounds.width, (int)bounds.height, self->url);
			else
				snprintf(buffer, bufferSize, "load bitmap %p %dx%d (content %p)", self->bitmap, (int)bounds.width, (int)bounds.height, self->content);
		}
		return true;
	case kprInstrumentedTextureUnload:
		{
			KprTexture self = msgData;
			FskRectangleRecord bounds;
			FskBitmapGetBounds(self->bitmap, &bounds);
			if (self->url)
				snprintf(buffer, bufferSize, "unload bitmap %p %dx%d \"%s\"", self->bitmap, (int)bounds.width, (int)bounds.height, self->url);
			else
				snprintf(buffer, bufferSize, "unload bitmap %p %dx%d (content %p)", self->bitmap, (int)bounds.width, (int)bounds.height, self->content);
		}
		return true;
	case kprInstrumentedContainerReflowing:
		{
			UInt32 flags = *(UInt32*)msgData;
			snprintf(buffer, bufferSize, "reflowing %s%s%s%s%s%s%s%s%s",
				(flags & kprPlaced) ? " placed" : "",
				(flags & kprXChanged) ? " x" : "",
				(flags & kprYChanged) ? " y" : "",
				(flags & kprWidthChanged) ? " width" : "",
				(flags & kprHeightChanged) ? " height" : "",
				(flags & kprContentsPlaced) ? " contents" : "",
				(flags & kprContentsHorizontallyChanged) ? " horizontal" : "",
				(flags & kprContentsVerticallyChanged) ? " vertical" : "",
				(flags & kprMatrixChanged) ? " matrix" : "");
		}
		return true;
	case kprInstrumentedLayerUpdate:
		snprintf(buffer, bufferSize, "updated");
		return true;
	case kprInstrumentedLayerBitmapNew:
		{
			KprLayer self = msgData;
			FskRectangleRecord bounds;
			FskBitmapGetBounds(self->bitmap, &bounds);
 			#if FSKBITMAP_OPENGL
			snprintf(buffer, bufferSize, "new bitmap %p %dx%d %s%s",
				self->bitmap, (int)bounds.width, (int)bounds.height,
				(gShell->window->usingGL && !(self->flags & kprNoAcceleration))	? "GL RGB" : "RGB",
				(self->flags & kprNoAlpha) ? "" : "A");
			#else
			snprintf(buffer, bufferSize, "new bitmap %p %dx%d %s%s",
				self->bitmap, (int)bounds.width, (int)bounds.height,
				"RGB",
				(self->flags & kprNoAlpha) ? "" : "A");
			#endif
		}
		return true;
	case kprInstrumentedLayerBitmapDispose:
		{
			KprLayer self = msgData;
			FskRectangleRecord bounds;
			FskBitmapGetBounds(self->bitmap, &bounds);
 			#if FSKBITMAP_OPENGL
			snprintf(buffer, bufferSize, "dispose bitmap %p %dx%d %s%s",
				self->bitmap, (int)bounds.width, (int)bounds.height,
				(gShell->window->usingGL && !(self->flags & kprNoAcceleration))	? "GL RGB" : "RGB",
				(self->flags & kprNoAlpha) ? "" : "A");
			#else
			snprintf(buffer, bufferSize, "dispose bitmap %p %dx%d %s%s",
				self->bitmap, (int)bounds.width, (int)bounds.height,
				"RGB",
				(self->flags & kprNoAlpha) ? "" : "A");
			#endif
		}
		return true;
	case kprInstrumentedLayerBitmapRelease:
		{
			KprLayer self = msgData;
			FskRectangleRecord bounds;
			FskBitmapGetBounds(self->bitmap, &bounds);
 			#if FSKBITMAP_OPENGL
			snprintf(buffer, bufferSize, "release bitmap %p %dx%d %s%s",
				self->bitmap, (int)bounds.width, (int)bounds.height,
				(gShell->window->usingGL && !(self->flags & kprNoAcceleration))	? "GL RGB" : "RGB",
				(self->flags & kprNoAlpha) ? "" : "A");
			#else
			snprintf(buffer, bufferSize, "release bitmap %p %dx%d %s%s",
				self->bitmap, (int)bounds.width, (int)bounds.height,
				"RGB",
				(self->flags & kprNoAlpha) ? "" : "A");
			#endif
		}
		return true;
    case kprInstrumentedStyleFormat:
        snprintf(buffer, bufferSize, "format");
        return true;
    case kprInstrumentedTransitionLink:
        snprintf(buffer, bufferSize, "link %p", msgData);
        return true;
    case kprInstrumentedTransitionUnlink:
        snprintf(buffer, bufferSize, "unlink %p", msgData);
        return true;
	}
	return false;
}
#endif

/* UTILITIES */

void KprAspectApply(UInt32 aspect, FskRectangle srcRect, FskRectangle dstRect)
{
	FskRectangleRecord tmpRect;
	if (!FskRectangleIsEqual(srcRect, dstRect)) {
		if (aspect & kprImageFill) {
			if (aspect & kprImageFit) {
			}
			else {
				FskRectangleScaleToFit(srcRect, dstRect, &tmpRect);
				*srcRect = tmpRect;
			}
		}
		else {
			if (aspect & kprImageFit) {
				FskRectangleScaleToFit(dstRect, srcRect, &tmpRect);
				*dstRect = tmpRect;
			}
			else {
				if (srcRect->width < dstRect->width) {
					dstRect->x += (dstRect->width - srcRect->width) >> 1;
					dstRect->width = srcRect->width;
				}
				else if (srcRect->width > dstRect->width) {
					srcRect->x += (srcRect->width - dstRect->width) >> 1;
					srcRect->width = dstRect->width;
				}
				if (srcRect->height < dstRect->height) {
					dstRect->y += (dstRect->height - srcRect->height) >> 1;
					dstRect->height = srcRect->height;
				}
				else if (srcRect->height > dstRect->height) {
					srcRect->y += (srcRect->height - dstRect->height) >> 1;
					srcRect->height = dstRect->height;
				}
			}
		}
	}
}

static const char* KprParseColorStrings[18] = {
	"black",
	"silver",
	"gray",
	"white",
	"maroon",
	"red",
	"purple",
	"fuchsia",
	"green",
	"lime",
	"olive",
	"yellow",
	"navy",
	"blue",
	"teal",
	"aqua",
	"orange",
	"transparent"
};

static const FskColorRGBARecord KprParseColorValues[18] = {
	{ 0x00, 0x00, 0x00, 0xff },
	{ 0xc0, 0xc0, 0xc0, 0xff },
	{ 0x80, 0x80, 0x80, 0xff },
	{ 0xff, 0xff, 0xff, 0xff },
	{ 0x80, 0x00, 0x00, 0xff },
	{ 0xff, 0x00, 0x00, 0xff },
	{ 0x80, 0x00, 0x80, 0xff },
	{ 0xff, 0x00, 0xff, 0xff },
	{ 0x00, 0x80, 0x00, 0xff },
	{ 0x00, 0xff, 0x00, 0xff },
	{ 0x80, 0x80, 0x00, 0xff },
	{ 0xff, 0xff, 0x00, 0xff },
	{ 0x00, 0x00, 0x80, 0xff },
	{ 0x00, 0x00, 0xff, 0xff },
	{ 0x00, 0x80, 0x80, 0xff },
	{ 0x00, 0xff, 0xff, 0xff },
	{ 0xff, 0xa5, 0x00, 0xff },
	{ 0x00, 0x00, 0x00, 0x00 }
};


#if SUPPORT_XS_DEBUG
Boolean KprParseColor(xsMachine *the, char* s, FskColorRGBA color)
#else
Boolean KprParseColor_(char* s, FskColorRGBA color)
#endif
{
	if ('#' == s[0]) {
		UInt32 len;
		s++;												/* Skip over '#' */
		len = FskStrLen(s);
		if (4 == len) {										/* 4 bits each for A, R, G, B */
			color->a = (UInt8)FskStrHexToNum(&s[0], 1) * 17;/* Promote 4 alpha bits to 8 */
			s++;											/* Advance to the RGB part */
			goto do_3;
		}
		else if (8 == len) {								/* 8 bits each for A, R, G, B */
			color->a = (UInt8)FskStrHexToNum(&s[0], 2);		/* Do only alpha now */
			s += 2;											/* Advance to the RGB part */
			goto do_6;
		}
		else if (3 == len) {								/* 4 bits each for R, G, B */
			color->a = 255;
do_3:
			color->r = (UInt8)FskStrHexToNum(&s[0], 1) * 17;/* Promote 4 R bits to 8 */
			color->g = (UInt8)FskStrHexToNum(&s[1], 1) * 17;/* Promote 4 G bits to 8 */
			color->b = (UInt8)FskStrHexToNum(&s[2], 1) * 17;/* Promote 4 B bits to 8 */
		}
		else if (6 == len) {								/* 8 bits each for R, G, B */
			color->a = 255;
do_6:
			color->r = (UInt8)FskStrHexToNum(&s[0], 2);
			color->g = (UInt8)FskStrHexToNum(&s[2], 2);
			color->b = (UInt8)FskStrHexToNum(&s[4], 2);
		}
		else
			return 0;
		return 1;
	}
	else {
		UInt32 format = 0;
		UInt32 c = 0;
		if ((s[0] == 'r') && (s[1] == 'g') && (s[2] == 'b')) {
			if (s[3] == '(') {
				format = 2;																			/* 2 --> rgb ((format & 2) --> rgb) */
				c = 2;
				s += 4;
			}
			else if ((s[3] == 'a') && (s[4] == '(')) {
				format = 3;																			/* 3 --> rgba (odd --> has alpha, (format & 2) --> rgb) */
				c = 3;
				s += 5;
			}

		}
		else if ((s[0] == 'h') && (s[1] == 's') && (s[2] == 'l')) {
			if (s[3] == '(') {
				format = 4;																			/* 4 --> hsl ((format & 4) --> hsl) */
				c = 2;
				s += 4;
			}
			else if ((s[3] == 'a') && (s[4] == '(')) {
				format = 5;																			/* 5 --> hsla (odd --> has alpha, (format & 4) --> hsl) */
				c = 3;
				s += 5;
			}
		}
		if (format) {
			float values[4];
			float value = 0;
			Boolean negative;
			UInt32 i;
			char d;
			float f;
			for (i = 0; i <= c; i++) {
				value = 0;
				negative = 0;
				while ((d = *s)) {
					if (!FskStrIsSpace(d))
						break;
					s++;
				}
				if (!d) return 0;
				if ('-' == *s) {
					if (!((format & 4) && (i == 0))) return 0;										/* Negative is only allowed for hue */
					negative = 1;
					s++;
				}
				while (((d = *s)) && ('0' <= d) && (d <= '9')) {
					value = (10 * value) + (d - '0');
					s++;
				}
				if (!d) return 0;
				if (d == '.') {
					if (!(((format & 1) && (i == c)) || ((format & 4) && (i == 0))))	return 0;	/* Fail if a decimal is found in other than alpha or hue */
					f = 0.1f;
					s++;
					while (((d = *s)) && ('0' <= d) && (d <= '9')) {
						value = value + (f * (d - '0'));
						s++;
						f /= 10;
					}
					if (!d) return 0;
				}
				if (d == '%') {
					if ((format & 1) && (i == c)) return 0;											/* Doesn't like percent in the alpha position */
					if ((format & 4) && (i == 0)) return 0;											/* Doesn't like percent for hue */
					value *= 255.f / 100.f;
					s++;
					d = *s;
					if (!d) return 0;
				}
				else if ((format & 4) && ((i == 1) || (i == 2))) return 0;							/* Only likes percentages for saturation and lightness, not integers or fractions */
				while ((d = *s)) {
					if (!FskStrIsSpace(d))
						break;
					s++;
				}
				if (!d) return 0;
				if (i < c) {
					if (d != ',') return 0;
					s++;
					d = *s;
				}
				else {
					if (d != ')') return 0;
					s++;
					d = *s;
				}
				if (negative) value = -value;
				values[i] = value;
			}
			if (d) return 0;
			if (format & 2) {																		/* rgb (TODO: rounding) */
				value = values[0]; if (value < 0) value = 0; else if (value > 255) value = 255;
				color->r = (UInt8)value;
				value = values[1]; if (value < 0) value = 0; else if (value > 255) value = 255;
				color->g = (UInt8)value;
				value = values[2]; if (value < 0) value = 0; else if (value > 255) value = 255;
				color->b = (UInt8)value;
			}
			else {																					/* hsl (TODO: rounding) */
				UInt8 hsl[3];
				if ((value = fmodf(values[0], 360.f)) < 0) value += 360.f;
				hsl[0] = (UInt8)(value * (255.f / 360.f));
				value = values[1]; if (value < 0) value = 0; else if (value > 255) value = 255;
				hsl[1] = (UInt8)value;
				value = values[2]; if (value < 0) value = 0; else if (value > 255) value = 255;
				hsl[2] = (UInt8)value;
				FskConvertHSLRGB(hsl, (UInt8*)color);
			}
			if (format & 1) {																		/* alpha - code shared between rgb and hsl (TODO: rounding) */
				value = 255 * values[3]; if (value < 0) value = 0; else if (value > 255) value = 255;
				color->a = (UInt8)value;
			}
			else
				color->a = 255U;
			return 1;
		}
		else {																						/* named color */
			UInt32 i;
			for (i = 0; i < sizeof(KprParseColorStrings)/sizeof(KprParseColorStrings[0]); i++) {
				if (!FskStrCompareCaseInsensitive(s, KprParseColorStrings[i])) {
					*color = KprParseColorValues[i];
					return 1;
				}
			}
		}
	}

#if SUPPORT_XS_DEBUG
    if (the) {
        xsTraceDiagnostic("invalid color specified: %s\n", s);
        xsDebugger();
    }
#endif

	return 0;
}

void KprSerializeColor(xsMachine* the, FskConstColorRGBA color, xsSlot* slot)
{
	static char* hexas = "0123456789abcdef";
	char buffer[256];
	char* s = buffer;
	UInt32 a = (color->a * ((1<<16) * 100U / 255U) + (1<<(16-1))) >> 16;	/* exactly equal to (UInt32)round((100 * (float)color->a) / 255) for [0, 255], but portable unlike round(). */
	if (a < 100) {
		if ((a == 0) && (color->r == 0) && (color->g == 0) && (color->b == 0)) {
			FskStrCopy(s, "transparent");
		}
		else {
			FskStrCopy(s, "rgba(");
			FskStrNumToStr(color->r, s + FskStrLen(s), sizeof(s) - FskStrLen(s));
			FskStrCat(s, ", ");
			FskStrNumToStr(color->g, s + FskStrLen(s), sizeof(s) - FskStrLen(s));
			FskStrCat(s, ", ");
			FskStrNumToStr(color->b, s + FskStrLen(s), sizeof(s) - FskStrLen(s));
			FskStrCat(s, ", .");
			if (a < 10)
				FskStrCat(s, "0");
			else if (!(a % 10))
				a /= 10;
			FskStrNumToStr(a, s + FskStrLen(s), sizeof(s) - FskStrLen(s));
			FskStrCat(s, ")");
		}
	}
	else {
		s[0] = '#';
		s[1] = hexas[color->r >> 4];
		s[2] = hexas[color->r & 15];
		s[3] = hexas[color->g >> 4];
		s[4] = hexas[color->g & 15];
		s[5] = hexas[color->b >> 4];
		s[6] = hexas[color->b & 15];
		s[7] = 0;
	}
	*slot = xsString(buffer);
}

UInt32 KprDateNow(void)
{
#if TARGET_OS_WIN32
	struct _timeb tb;

	_ftime(&tb);
	return (UInt32)tb.time;
#else
	struct timeval time;

	gettimeofday(&time, NULL);
	return time.tv_sec;
#endif
}

// Compute seconds from "01-01-1970 00:00:00" from a GMT FskTimeElements
// Valid until end of 2099
static SInt32 FskTimeElementsToEpoch(FskTimeElements fsktm)
{
	static unsigned short sDaysFromLeapYear[4][12] = {
		{   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335},
		{ 366, 397, 425, 456, 486, 517, 547, 578, 609, 639, 670, 700},
		{ 731, 762, 790, 821, 851, 882, 912, 943, 974,1004,1035,1065},
		{1096,1127,1155,1186,1216,1247,1277,1308,1339,1369,1400,1430},
	};
    SInt32 year   = fsktm->tm_year - 68; // from 1968
    return
		(year / 4 * (365 * 4 + 1) + sDaysFromLeapYear[year % 4][fsktm->tm_mon] + fsktm->tm_mday - 1) * 86400
		+ (fsktm->tm_hour * 3600) + (fsktm->tm_min * 60) + fsktm->tm_sec
		- 63158400; // seconds from 1968 to 1970
}

FskErr KprDateFromHTTP(char* text, UInt32* date)
{
	if (text) {
		// http://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.3.1
		// According to above spec, we all assume %Z infomation is GMT (case sensitive)
		static char* formats[] = {
			"%a,%n%d%n%b%n%Y%n%H:%M:%S%n",	// RFC 822, updated by RFC 1123
			"%A,%n%d-%b-%y%n%H:%M:%S%n",	// RFC 850, obsoleted by RFC 1036
			"%a%n%b%n%d%n%H:%M:%S%n%Y",		// ANSI C's asctime() format
			"%a,%n%d-%b-%Y%n%H:%M:%S%n",	// Cookie format
			NULL };
		UInt32 i = 0;
		char* format;
		char* result;
		FskTimeElementsRecord fsktm;
		for (format = formats[i++]; format; format = formats[i++]) {
			memset(&fsktm, 0, sizeof(fsktm));
			FskTimeStrptime(text, format, &fsktm, &result);
			if (result)
				break;
		};
		if (format) {
			*date = FskTimeElementsToEpoch(&fsktm);
			return kFskErrNone;
		}
	}
	*date = 0;
	return kFskErrInvalidParameter;
}

UInt32 KprEnvironmentGetUInt32(char* key, UInt32 it)
{
	const char *value = FskEnvironmentGet(key);
	if (value)
		return FskStrToL(value, NULL, 10);
	else
		return it;
}

#if kprDumpMemory

static UInt32 gMemoryCount = 0;

FskErr KprMemPtrNew_(UInt32 size, FskMemPtr *newMemory, char* file, int line)
{
	FskErr err = FskMemPtrNew(size, newMemory);
	gMemoryCount++;
	fprintf(stderr, "MMM %08x + FskMemPtrNew (%u): %d - %u [%s:%d]\n", (unsigned int)*newMemory, (unsigned) size, (int) err, (unsigned) gMemoryCount, file, line);
	return err;
}

FskErr KprMemPtrNewClear_(UInt32 size, FskMemPtr *newMemory, char* file, int line)
{
	FskErr err = FskMemPtrNewClear(size, newMemory);
	gMemoryCount++;
	fprintf(stderr, "MMM %08x + FskMemPtrNewClear (%u): %d - %u [%s:%d]\n", (unsigned int)*newMemory, (unsigned) size, (int) err, (unsigned) gMemoryCount, file, line);
	return err;
}

FskErr KprMemPtrRealloc_(UInt32 size, FskMemPtr *newMemory, char* file, int line)
{
	FskMemPtr ptr = *newMemory;
	FskErr err = FskMemPtrRealloc(size, newMemory);
	fprintf(stderr, "MMM %08x <- %08x + FskMemPtrRealloc (%u): %d - %u [%s:%d]\n", (unsigned int)*newMemory, (unsigned int)ptr, (unsigned) size, (int) err, (unsigned) gMemoryCount, file, line);
	return err;
}

FskErr KprMemPtrNewFromData_(UInt32 size, const void *data, FskMemPtr *newMemory, char* file, int line)
{
	FskErr err = FskMemPtrNewFromData(size, data, newMemory);
	gMemoryCount++;
	fprintf(stderr, "MMM %08x <- %08x + FskMemPtrNewFromData (%u): %d - %u [%s:%d]\n", (unsigned int)*newMemory, (unsigned int)data, (unsigned) size, (int) err, (unsigned) gMemoryCount, file, line);
	return err;
}

void KprMemPtrDispose_(void *ptr, char* file, int line)
{
	if (ptr) gMemoryCount--;
	fprintf(stderr, "MMM %08x - FskMemPtrDispose - %u [%s:%d]\n", (unsigned int)ptr, (unsigned) gMemoryCount, file, line);
	FskMemPtrDispose(ptr);
}

void KprMemPtrDisposeAt_(void **ptr, char* file, int line)
{
	if (*ptr) gMemoryCount--;
	fprintf(stderr, "MMM %08x - FskMemPtrDisposeAt - %u [%s:%d]\n", (unsigned int)*ptr, (unsigned) gMemoryCount, file, line);
	FskMemPtrDisposeAt(ptr);
}

char* KprStrDoCopy_(const char *str, char* file, int line)
{
	char* result = FskStrDoCopy(str);
	if (str) gMemoryCount++;
	fprintf(stderr, "MMM %08x <- %08x - FskStrDoCopy '%s' - %u [%s:%d]\n", (unsigned int)result, (unsigned int)str, result, (unsigned) gMemoryCount, file, line);
	return result;
}

#endif

FskErr FskStrB64Decode(const char *base64, UInt32 base64Size, char **pDst, UInt32 *pDstSize)
{
	FskErr err = kFskErrNone;
	UInt8* src;
	UInt32 dstSize, srcIndex, dstIndex;
	Boolean aFlag = false;
	UInt8 aByte;
	UInt8 aBuffer[4];
	UInt8* dst;

	src = (UInt8*)base64;

	dstSize = (base64Size / 4) * 3;
	if (base64[base64Size - 1] == '=')
		dstSize--;
	if (base64[base64Size - 2] == '=')
		dstSize--;
	srcIndex = 0;

	bailIfError(FskMemPtrNewClear(dstSize + 1, pDst));
	dst = (UInt8*)*pDst;
	dstIndex = 3;
	while ((aByte = *src++)) {
		if (('A' <= aByte) && (aByte <= 'Z'))
			aByte = aByte - 'A';
		else if (('a' <= aByte) && (aByte <= 'z'))
			aByte = aByte - 'a' + 26;
		else if (('0' <= aByte) && (aByte <= '9'))
			aByte = aByte - '0' + 52;
		else if (aByte == '+')
			aByte = 62;
		else if (aByte == '/')
			aByte = 63;
		else if (aByte == '=') {
			if (srcIndex == 2) {
				if (*src == '=') {
					aBuffer[srcIndex++] = 0;
					dstIndex = 1;
					aByte = 0;
					aFlag = true;
				}
				else
					continue;
			}
			else if (srcIndex == 3) {
				dstIndex = 2;
				aByte = 0;
				aFlag = true;
			}
			else
				continue;
		}
		else
			continue;
		aBuffer[srcIndex++] = aByte;
		if (srcIndex == 4) {
			*dst++ = (aBuffer[0] << 2) | ((aBuffer[1] & 0x30) >> 4);
			if (dstIndex > 1)
				*dst++ = ((aBuffer[1] & 0x0F) << 4) | ((aBuffer[2] & 0x3C) >> 2);
			if (dstIndex > 2)
				*dst++ = ((aBuffer[2] & 0x03) << 6) | (aBuffer[3] & 0x3F);
			srcIndex = 0;
		}
		if (aFlag)
			break;
	}
	if (pDstSize)
		*pDstSize = dstSize;
bail:
	return err;
}

char *FskStrStrCaseInsensitive(const char *haystack, const char *needle)
{
	int nlen = strlen(needle);
	int hlen = strlen(haystack) - nlen + 1;
	int i;

	for (i = 0; i < hlen; i++) {
		int j;
		for (j = 0; j < nlen; j++) {
			unsigned char c1 = haystack[i+j];
			unsigned char c2 = needle[j];
			if (toupper(c1) != toupper(c2))
				goto next;
		}
		return (char *) haystack + i;
	next:
			;
	}
	return NULL;
}

typedef struct {
	FskImageDecompress deco;
    const void *data;
    UInt32 dataSize;
	FskImageDecompressComplete completion;
	void *refcon;
} FskImageDecompressDataWithOrientationParamsRecord, *FskImageDecompressDataWithOrientationParams;

static FskErr FskImageDecompressDataWithOrientationAux(FskImageDecompress deco, const void *data, UInt32 dataSize, FskBitmap *bits);
static void FskImageDecompressDataWithOrientationComplete(FskImageDecompress deco, void *refcon, FskErr result, FskBitmap bits);

FskErr FskImageDecompressDataWithOrientation(const void *data, UInt32 dataSize, const char *mimeType, const char *extension, UInt32 targetWidth, UInt32 targetHeight, FskImageDecompressComplete completion, void *refcon, FskBitmap *bits)
{
	FskErr err;
	FskImageDecompress deco;
	err = FskImageDecompressNew(&deco, 0, mimeType, extension);
	if (kFskErrNone == err) {
		FskImageDecompressRequestSize(deco, targetWidth, targetHeight);
		if (NULL == completion) {
			err = FskImageDecompressFrame(deco, data, dataSize, bits, true, NULL, NULL, NULL, NULL, NULL, NULL, kFskImageFrameTypeSync);
			if (kFskErrNone == err)
				err = FskImageDecompressDataWithOrientationAux(deco, data, dataSize, bits);
			FskImageDecompressDispose(deco);
		}
		else {
			FskImageDecompressDataWithOrientationParams params;
			err = FskMemPtrNew(sizeof(FskImageDecompressDataWithOrientationParamsRecord), &params);
			if (kFskErrNone == err) {
				params->deco = deco;
				params->data = data;
				params->dataSize = dataSize;
				params->completion = completion;
				params->refcon = refcon;
				err = FskImageDecompressFrame(deco, data, dataSize, bits, true, FskImageDecompressDataWithOrientationComplete, params, NULL, NULL, NULL, NULL, kFskImageFrameTypeSync);
			}
		}
	}
	return err;
}

FskErr FskImageDecompressDataWithOrientationAux(FskImageDecompress deco, const void *data, UInt32 dataSize, FskBitmap *bits)
{
	FskErr err = kFskErrNone;
	FskMediaPropertyValueRecord property;
	FskBitmap srcBits, dstBits = NULL;
	FskPort port = NULL;
	FskRectangleRecord bounds;
	float transform[3][3];
	UInt32 x, y, width, height;

	bailIfError(FskImageDecompressSetData(deco, (void*)data, dataSize));
	err = FskImageDecompressGetMetaData(deco, 0x40000112, 1, &property, NULL);
	if ((kFskErrNone == err) && (property.type == kFskMediaPropertyTypeInteger) && (property.value.integer > 1)) { // up
		srcBits = *bits;
		FskBitmapGetBounds(srcBits, &bounds);
		x = bounds.width;
		y = bounds.height;
		switch (property.value.integer) {
		case 2: // up-mirrored
			transform[0][0] = -1.f; 	transform[0][1] =  0.f;
			transform[1][0] =  0.f; 	transform[1][1] =  1.f;
			transform[2][0] = (float)x; transform[2][1] =  0.f;
			width  = x;
			height = y;
			break;
		case 3: // down
			transform[0][0] = -1.f; 	transform[0][1] =  0.f;
			transform[1][0] =  0.f; 	transform[1][1] = -1.f;
			transform[2][0] = (float)x;	transform[2][1] = (float)y;
			width  = x;
			height = y;
			break;
		case 4: // down-mirrored
			transform[0][0] =  1.f;		transform[0][1] =  0.f;
			transform[1][0] =  0.f; 	transform[1][1] = -1.f;
			transform[2][0] =  0.f; 	transform[2][1] = (float)y;
			width  = x;
			height = y;
			break;
		case 5: // left-mirrored
			transform[0][0] =  0.f; 	transform[0][1] =  1.f;
			transform[1][0] =  1.f; 	transform[1][1] =  0.f;
			transform[2][0] =  0.f; 	transform[2][1] =  0.f;
			width  = y;
			height = x;
			break;
		case 6: // left
			transform[0][0] =  0.f; 	transform[0][1] =  1.f;
			transform[1][0] = -1.f; 	transform[1][1] =  0.f;
			transform[2][0] = (float)y;	transform[2][1] =  0.f;
			width  = y;
			height = x;
			break;
		case 7: // right-mirrored
			transform[0][0] =  0.f; 	transform[0][1] = -1.f;
			transform[1][0] = -1.f; 	transform[1][1] =  0.f;
			transform[2][0] = (float)y; transform[2][1] = (float)x;
			width  = y;
			height = x;
			break;
		case 8: // right
			transform[0][0] =  0.f; 	transform[0][1] = -1.f;
			transform[1][0] =  1.f; 	transform[1][1] =  0.f;
			transform[2][0] =  0.f; 	transform[2][1] = (float)x;
			width  = y;
			height = x;
			break;
		default:
			goto bail;
		}
		transform[0][2] = 0.f;
		transform[1][2] = 0.f;
		transform[2][2] = 1.f;
		bailIfError(FskPortNew(&port, NULL, NULL));
		bailIfError(FskBitmapNew(width, height, kFskBitmapFormatDefaultNoAlpha, &dstBits));
		FskPortSetBitmap(port, dstBits);
		FskPortBeginDrawing(port, NULL);
		FskPortBitmapProject(port, srcBits, &bounds, transform);
		FskPortEndDrawing(port);
		FskPortSetBitmap(port, NULL);
		*bits = dstBits;
		dstBits = srcBits;
	}
bail:
	FskBitmapDispose(dstBits);
	FskPortDispose(port);
	return kFskErrNone;
}

void FskImageDecompressDataWithOrientationComplete(FskImageDecompress deco, void *refcon, FskErr err, FskBitmap bits)
{
	FskImageDecompressDataWithOrientationParams params = (FskImageDecompressDataWithOrientationParams)refcon;
	if (kFskErrNone == err)
		err = FskImageDecompressDataWithOrientationAux(deco, params->data, params->dataSize, &bits);
	(params->completion)(deco, params->refcon, err, bits);
	FskImageDecompressDispose(params->deco);
	FskMemPtrDispose(params);
}


typedef struct {
	FskBitmap bits;
	FskRectangleRecord srcRect;
	float transform[3][3];
	UInt32 quality;
} FskPortBitmapProjectParams;

typedef const float (*TCFloatX3)[3];
typedef const float (*TCFloatX2)[2];

static FskErr FskPortBitmapProjectPrepareProc(FskPort port, void *paramsIn UNUSED, UInt32 paramsSize UNUSED)
{
#if FSKBITMAP_OPENGL
	FskPortBitmapProjectParams *p = paramsIn;
	if (FskBitmapIsOpenGLDestinationAccelerated(port->bits))
		FskBitmapCheckGLSourceAccelerated(p->bits);
#endif
	return kFskErrNone;
}

static FskErr FskPortBitmapProjectProc(FskPort port, void *paramsIn, UInt32 paramsSize UNUSED)
{
	FskErr err = kFskErrNone;
	FskPortBitmapProjectParams *p = paramsIn;
	FskRectangleRecord dstClip;
	float dstTransform[3][3], transform[3][3];
	FskBitmap srcBits = p->bits;
	FskBitmap dstBits = port->bits;

	dstClip = port->aggregateClip;
	FskPortRectScale(port, &dstClip);
	FskSIdentityMatrix(dstTransform[0], 3, 3);
	dstTransform[0][0] = (float)FskPortDoubleScale(port, 1.0);
	dstTransform[1][1] = (float)FskPortDoubleScale(port, 1.0);
	dstTransform[2][0] = (float)FskPortDoubleScale(port, port->origin.x);
	dstTransform[2][1] = (float)FskPortDoubleScale(port, port->origin.y);
	FskSLinearTransform(p->transform[0], dstTransform[0], transform[0], 3, 3, 3);
#if FSKBITMAP_OPENGL
	if (FskBitmapIsOpenGLDestinationAccelerated(dstBits))
		err = FskGLProjectBitmap(srcBits, &p->srcRect, dstBits, &dstClip, (TCFloatX3)transform, &port->penColor, port->graphicsMode, port->graphicsModeParameters);
	else
#endif
	{
		UInt32 graphicsMode = port->graphicsMode;
		if (transform[0][2] == 0 && transform[1][2] == 0) {			/* Affine */
			if (transform[0][1] == 0 && transform[1][0] == 0) {		/* Scale */
				FskScaleOffset scaleOffset;
				scaleOffset.scaleX = FskRoundFloatToFixed24(transform[0][0] / transform[2][2]);
				scaleOffset.scaleY = FskRoundFloatToFixed24(transform[1][1] / transform[2][2]);
				scaleOffset.offsetX = FskRoundFloatToFixed(transform[2][0] / transform[2][2]);
				scaleOffset.offsetY = FskRoundFloatToFixed(transform[2][1] / transform[2][2]);
				FskScaleOffsetBitmap(srcBits, &p->srcRect, dstBits,  &dstClip, &scaleOffset, &port->penColor, graphicsMode, port->graphicsModeParameters);/* Do identity with FskScaleOffset */
			}
			else {
				graphicsMode |= kFskGraphicsModeAffine;
				FskProjectBitmap(srcBits, &p->srcRect, dstBits, &dstClip, (TCFloatX3)transform, &port->penColor, graphicsMode, port->graphicsModeParameters);
			}
		}
		else {
			FskProjectBitmap(srcBits, &p->srcRect, dstBits, &dstClip, (TCFloatX3)transform, &port->penColor, graphicsMode, port->graphicsModeParameters);
		}
	}
	if (--(srcBits->useCount) < 0)
		FskBitmapDispose(srcBits);
	return err;
}

FskErr FskPortBitmapProject(FskPort port, FskBitmap srcBits, FskRectangle srcRect, float transform[3][3])
{
	FskPortBitmapProjectParams p;
	p.bits = srcBits;
	p.srcRect = *srcRect;
	FskMemMove(p.transform, transform, sizeof(float) * 9);
	p.quality = (port->graphicsMode & kFskGraphicsModeBilinear) ? 1 : 0;
	srcBits->useCount += 1;
    return FskPortPicSaveAdd(port, FskPortBitmapProjectProc, FskPortBitmapProjectPrepareProc, &p, sizeof(p));
}

Boolean FskRectangleIsIntersectionNotEmpty(FskConstRectangle a, FskConstRectangle b)
{
	if (FskRectangleIsEmpty(a)) return false;
	if (FskRectangleIsEmpty(b)) return false;
	return FskRectanglesDoIntersect(a, b);
}

void FskRectangleScaleToFill(FskConstRectangle containing, FskConstRectangle containee, FskRectangle fillOut)
{
	FskRectangleRecord fill;

	if ((0 == containee->height) || (0 == containee->width)) {
		FskRectangleSetEmpty(fillOut);
		return;
	}

	fill.width = containing->width;
	fill.height = (containee->height * containing->width) / containee->width;
	if (fill.height < containing->height) {
		fill.height = containing->height;
		fill.width = (containee->width * containing->height) / containee->height;
	}

	fill.x = (containing->width - fill.width) / 2;
	fill.y = (containing->height - fill.height) / 2;
	FskRectangleOffset(&fill, containing->x, containing->y);

	*fillOut = fill;
}

// CRYPT

FskErr KprCryptMD5(void* input, UInt32 size, UInt8* binary, char* output)
{
	FskErr err = kFskErrNone;
	unsigned char* in = input;
	unsigned char* out = (unsigned char*)output;
	unsigned char bin[16];
	SInt32 count = 16, i;
	md5_hash_state state;
	md5_init(&state);
	md5_process(&state, in, size);
	md5_done(&state, bin);

	if (binary) {
		FskMemCopy(binary, bin, 16);
	}
	if (output) {
		for (i = 0; i < count; i++) {
			unsigned char j;

			j = (bin[i] >> 4) & 0xf;
			if (j <= 9)
				out[i*2] = (j + '0');
			else
				out[i*2] = (j + 'a' - 10);
			j = bin[i] & 0xf;
			if (j <= 9)
				out[i*2+1] = (j + '0');
			else
				out[i*2+1] = (j + 'a' - 10);
		};
		out[count * 2] = 0;
	}

	return err;
}

//FskErr KprCryptSHA1(void* input, UInt32 size, UInt8* binary, char* output)
//{
//	FskErr err = kFskErrNone;
//	unsigned char* in = input;
//	unsigned char* out = (unsigned char*)output;
//	unsigned char bin[KPR_CRYPT_SHA1_HASH_SIZE];
//	SInt32 count = KPR_CRYPT_SHA1_HASH_SIZE, i;
//	struct sha1 state;
//	sha1_create(&state);
//	sha1_update(&state, in, size);
//	sha1_fin(&state, bin);
//
//	if (binary) {
//		FskMemCopy(binary, bin, count);
//	}
//	if (output) {
//		for (i = 0; i < count; i++) {
//			unsigned char j;
//
//			j = (bin[i] >> 4) & 0xf;
//			if (j <= 9)
//				out[i*2] = (j + '0');
//			else
//				out[i*2] = (j + 'a' - 10);
//			j = bin[i] & 0xf;
//			if (j <= 9)
//				out[i*2+1] = (j + '0');
//			else
//				out[i*2+1] = (j + 'a' - 10);
//		};
//		out[count * 2] = 0;
//	}
//
//	return err;
//}
//
//FskErr KprCryptPKCS5_PBKDF2_HMAC_SHA1(const char *password, UInt32 passwordSize, const unsigned char *salt, UInt32 saltSize, UInt32 iteration, UInt32 size, unsigned char* output)
//{
//	// http://examples.oreilly.com/0636920023234/ch10/PKCS5_PBKDF2_HMAC_SHA1.c
//	FskErr err = kFskErrNone;
//	KprCryptHMAC_SHA1ContextRecord context;
//	unsigned char digtmp[KPR_CRYPT_SHA1_HASH_SIZE], *p, itmp[4];
//	int cplen, j, k, tkeylen;
//	unsigned long i = 1;
//	p = output;
//	tkeylen = size;
//
//	if (passwordSize == 0)
//		passwordSize = FskStrLen(password);
//	if (saltSize == 0)
//		saltSize = FskStrLen(salt);
//	while (tkeylen) {
//		if (tkeylen > KPR_CRYPT_SHA1_HASH_SIZE)
//		    cplen = KPR_CRYPT_SHA1_HASH_SIZE;
//		else
//		    cplen = tkeylen;
//		itmp[0] = (unsigned char)((i >> 24) & 0xff);
//		itmp[1] = (unsigned char)((i >> 16) & 0xff);
//		itmp[2] = (unsigned char)((i >> 8) & 0xff);
//		itmp[3] = (unsigned char)(i & 0xff);
//		KprCryptHMACReset_SHA1(&context, password, passwordSize);
//		KprCryptHMACInput_SHA1(&context, salt, saltSize);
//		KprCryptHMACInput_SHA1(&context, itmp, 4);
//		KprCryptHMACResult_SHA1(&context, digtmp);
//		FskMemCopy(p, digtmp, cplen);
//		for (j = 1; j < iteration; j++) {
//			KprCryptHMAC_SHA1(password, passwordSize, digtmp, KPR_CRYPT_SHA1_HASH_SIZE, digtmp);
//			for (k = 0; k < cplen; k++)
//				p[k] ^= digtmp[k];
//		}
//		i++;
//		p += cplen;
//		tkeylen -= cplen;
//	}
//
//	return err;
//}
//
//FskErr KprCryptHMAC_SHA1(const void *key, UInt32 keySize, const void *data, UInt32 dataSize, void *output)
//{
//	FskErr err = kFskErrNone;
//	KprCryptHMAC_SHA1ContextRecord context;
//
//	KprCryptHMACReset_SHA1(&context, key, keySize);
//	KprCryptHMACInput_SHA1(&context, data, dataSize);
//	KprCryptHMACResult_SHA1(&context, output);
//
//	return err;
//}
//
//void KprCryptHMACInput_SHA1(KprCryptHMAC_SHA1Context context, const void *data, UInt32 dataSize)
//{
//	sha1_update(&context->state, data, dataSize);
//}
//
//void KprCryptHMACReset_SHA1(KprCryptHMAC_SHA1Context context, const void *key, UInt32 keySize)
//{
//	UInt32 i, blockSize = KPR_CRYPT_SHA1_BLOCK_SIZE, hashSize = KPR_CRYPT_SHA1_HASH_SIZE;
//	unsigned char k_ipad[blockSize], tempKey[hashSize];
//
//	if (keySize > blockSize) {
//		struct sha1 state;
//		sha1_create(&state);
//		sha1_update(&state, key, keySize);
//		sha1_fin(&state, tempKey);
//		key = tempKey;
//		keySize = hashSize;
//	}
//	for (i = 0; i < keySize; i++) {
//		k_ipad[i] = ((unsigned char *)key)[i] ^ 0x36;
//		context->k_opad[i] = ((unsigned char *)key)[i] ^ 0x5c;
//	}
//	for ( ; i < blockSize; i++) {
//		k_ipad[i] = 0x36;
//		context->k_opad[i] = 0x5c;
//	}
//	sha1_create(&context->state);
//	sha1_update(&context->state, k_ipad, blockSize);
//}
//
//void KprCryptHMACResult_SHA1(KprCryptHMAC_SHA1Context context, void *output)
//{
//	sha1_fin(&context->state, output);
//	sha1_create(&context->state);
//	sha1_update(&context->state, context->k_opad, KPR_CRYPT_SHA1_BLOCK_SIZE);
//	sha1_update(&context->state, output, KPR_CRYPT_SHA1_HASH_SIZE);
//	sha1_fin(&context->state, output);
//}
//
//void KprCryptXOR(void* input1, void* input2, UInt32 size, char* output)
//{
//	unsigned char* in1 = input1;
//	unsigned char* in2 = input2;
//	unsigned char* out = (unsigned char*)output;
//	SInt32 i;
//
//	for (i = 0; i < size; i++) {
//		out[i] = in1[i] ^ in2[i];
//	}
//}

// MACHINE NAME

char* KprMachineName(void)
{
	char* result = NULL;
#if TARGET_OS_WIN32
	UInt16 machineNameW[1024];
	char *machineName = NULL;
	DWORD size = sizeof(machineNameW) / sizeof(UInt16);
	GetComputerNameW(machineNameW, &size);
	FskTextUnicode16LEToUTF8(machineNameW, size * 2, &machineName, NULL);
	result = FskStrDoCopy(machineName);
	FskMemPtrDispose(machineName);
#elif TARGET_OS_MAC
	result = FskStrDoCopy(FskCocoaDeviceName());
#elif TARGET_OS_ANDROID
	char *btName = gAndroidCallbacks->getDeviceUsernameCB();
	char *buffer = NULL;
	if (btName)
		buffer = FskStrDoCopy(btName);
	else
		buffer = FskStrDoCat("Android:", gAndroidCallbacks->getStaticIMEICB());
	result = FskStrDoCopy(buffer);
#else
	char buffer[1024];
	gethostname(buffer, sizeof(buffer)-1);
	result = FskStrDoCopy(buffer);
#endif
	return result;
}

void KprEnsureDirectory(char* path)
{
	FskErr err = kFskErrNone;
	FskFileInfo info;
	if (kFskErrNone != FskFileGetFileInfo(path, &info)) {
		char* p = path + 1;
		char* q = FskStrRChr(p, '/');
		if (q) {
			char c;
			q++;
			c = *q;
			*q = 0;
			while ((p = FskStrChr(p, '/'))) {
				char cc;
				p++;
				cc = *p;
				*p = 0;
				if (kFskErrNone != FskFileGetFileInfo(path, &info)) {
					bailIfError(FskFileCreateDirectory(path));
				}
				*p = cc;
			}
			*q = c;
		}
	}
bail:
	return;
}

//--------------------------------------------------
// XML Helper
//--------------------------------------------------

static void KprXMLElementDisposeNext(KprXMLElement self);
static FskErr KprXMLAttributeNew(KprXMLAttribute *it, const char* name, const char* value, Boolean isNamespace);
static void KprXMLAttributeDispose(KprXMLAttribute self);

//--------------------------------------------------
// INSTRUMENTATION
//--------------------------------------------------

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord gKprXMLElementInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprXMLElement", FskInstrumentationOffset(KprXMLElementRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord gKprXMLAttributeInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprXMLAttribute", FskInstrumentationOffset(KprXMLAttributeRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

//--------------------------------------------------
// XML Element
//--------------------------------------------------

FskErr KprXMLElementNew(KprXMLElement *it, KprXMLElement owner, const char* name, const char** attributes)
{
	FskErr err = kFskErrNone;
	KprXMLElement self = NULL;
	char** aPointer;
	char* key;
	char* ptr;
	char* value;
	KprXMLAttribute attribute, namespace;

	bailIfError(FskMemPtrNewClear(sizeof(KprXMLElementRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprXMLElementInstrumentation);
	self->owner = owner;
	if (owner)
		FskListAppend(&owner->element, self);
	// process attributes
	aPointer = (char**)attributes;
	while (*aPointer) {
		key = *aPointer++;
		value = *aPointer++;
		if (!FskStrCompare(key, "xmlns")) {
			bailIfError(KprXMLAttributeNew(&attribute, NULL, value, true));
		}
		else if (!FskStrCompareWithLength(key, "xmlns:", 6)) {
			bailIfError(KprXMLAttributeNew(&attribute, key + 6, value, true));
		}
		else {
			bailIfError(KprXMLAttributeNew(&attribute, NULL, value, false));
		}
		FskListAppend(&self->attribute, attribute);
	}

	if (name) {
		if ((ptr = FskStrChr(name, ':'))) {
			*ptr = 0;
			namespace = KprXMLElementGetNamespace(self, name);
			*ptr = ':';
			name = ptr + 1;
		}
		else
			namespace = KprXMLElementGetNamespace(self, NULL);
		self->nameSpace = namespace;
		self->name = FskStrDoCopy(name);
		FskInstrumentedItemPrintfDebug(self, "+ Element %8p (%s) - %s", self, self->name, self->nameSpace ? self->nameSpace->value : "NULL");
	}

	aPointer = (char**)attributes;
	for (attribute = self->attribute; attribute; attribute = attribute->next) {
		key = *aPointer++;
        aPointer++;       // value
		if (!attribute->isNamespace) {
			if ((ptr = FskStrChr(key, ':'))) {
				*ptr = 0;
				namespace = KprXMLElementGetNamespace(self, key);
				*ptr = ':';
				key = ptr + 1;
			}
			else
				namespace = KprXMLElementGetNamespace(self, NULL);
			attribute->name = FskStrDoCopy(key);
			attribute->nameSpace = namespace;
		}
		FskInstrumentedItemPrintfDebug(attribute, "   + %s %8p (%s -> %s)", attribute->isNamespace ? "Namespace" : "Attribute",
			attribute, attribute->name, attribute->isNamespace ? attribute->value : (attribute->nameSpace ? attribute->nameSpace->value : "NULL"));
	}
	return err;
bail:
	return err;
}

void KprXMLElementDispose(KprXMLElement self)
{
	if (self) {
		if (self->owner) {
			FskListRemove(&self->owner->element, self);
		}
		KprXMLElementDisposeNext(self->next);
		KprXMLElementDisposeNext(self->element);
		KprXMLAttributeDispose(self->attribute);
		FskInstrumentedItemPrintfDebug(self, "- Element %8p (%s)", self, self->name);
		FskMemPtrDispose(self->value);
		FskMemPtrDispose(self->name);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprXMLElementDisposeNext(KprXMLElement self)
{
	if (self) {
		KprXMLElementDisposeNext(self->next);
		KprXMLElementDisposeNext(self->element);
		KprXMLAttributeDispose(self->attribute);
		FskInstrumentedItemPrintfDebug(self, "- Element %8p (%s)", self, self->name);
		FskMemPtrDispose(self->value);
		FskMemPtrDispose(self->name);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

char* KprXMLElementGetAttribute(KprXMLElement self, const char* name)
{
	KprXMLAttribute attribute;
	char* value = NULL;

	for (attribute = self->attribute; attribute; attribute = attribute->next) {
		if (!FskStrCompare(attribute->name, name)) {
			value = attribute->value;
			break;
		}
	}
	return value;
}

KprXMLElement KprXMLElementGetFirstElement(KprXMLElement self, const char* namespace, const char* name)
{
	KprXMLElement element;

	for (element = self->element; element; element = element->next)
		if (KprXMLElementIsEqual(element, namespace, name))
				break;
	return element;
}

SInt32 KprXMLElementGetIntegerValue(KprXMLElement self)
{
	if (self->element)
		return FskStrToNum(self->element->value);
	return 0;
}

KprXMLAttribute KprXMLElementGetNamespace(KprXMLElement self, const char* name)
{
	KprXMLAttribute attribute, namespace = NULL;
	for (attribute = self->attribute; attribute; attribute = attribute->next) {
		if (attribute->isNamespace && ((attribute->name == name) || (!FskStrCompare(attribute->name, name)))) {
			namespace = attribute;
			break;
		}
	}
	if (!namespace && self->owner)
		namespace = KprXMLElementGetNamespace(self->owner, name);
	return namespace;
}

KprXMLElement KprXMLElementGetNextElement(KprXMLElement self, const char* namespace, const char* name)
{
	KprXMLElement element;

	for (element = self->next; element; element = element->next)
		if (KprXMLElementIsEqual(element, namespace, name))
			break;
	return element;
}

char* KprXMLElementGetProperty(KprXMLElement self, const char* namespace, const char* name)
{
	KprXMLElement property = KprXMLElementGetFirstElement(self, namespace, name);
	if (property && property->element)
		return property->element->value;
	else
		return NULL;
}

char* KprXMLElementGetValue(KprXMLElement self)
{
	if (self->element)
		return self->element->value;
	return NULL;
}

Boolean KprXMLElementIsEqual(KprXMLElement self, const char* namespace, const char* name)
{
	if (!FskStrCompare(self->name, name)) {
		if (self->nameSpace)
			return (!FskStrCompare(self->nameSpace->value, namespace));
		else
			return (namespace == NULL);
	}
	return false;
}

FskErr KprXMLElementSetAttributeValue(KprXMLElement self, const char* name, const char* value)
{
	FskErr err = kFskErrNone;
	KprXMLAttribute attribute;

	for (attribute = self->attribute; attribute; attribute = attribute->next) {
		if (!FskStrCompare(attribute->name, name)) {
			FskMemPtrDispose(attribute->value);
			bailIfNULL(attribute->value = FskStrDoCopy(value));
			return err;
		}
	}
bail:
	return kFskErrInvalidParameter;
}

//--------------------------------------------------
// XML Attribute
//--------------------------------------------------

FskErr KprXMLAttributeNew(KprXMLAttribute *it, const char* name, const char* value, Boolean isNamespace)
{
	FskErr err = kFskErrNone;
	KprXMLAttribute self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprXMLAttributeRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprXMLAttributeInstrumentation);
	self->isNamespace = isNamespace;
	self->name = FskStrDoCopy(name);
	self->value = FskStrDoCopy(value);
	return err;
bail:
	return err;
}

void KprXMLAttributeDispose(KprXMLAttribute self)
{
	if (self) {
		KprXMLAttributeDispose(self->next);
		FskInstrumentedItemPrintfDebug(self, "   - %s %8p (%s)", self->isNamespace ? "Namespace" : "Attribute", self, self->name ? self->name : "NULL");
		FskMemPtrDispose(self->value);
		FskMemPtrDispose(self->name);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

//--------------------------------------------------
// XML Parser
//--------------------------------------------------

void KprXMLParserDefault(void *data, const char *text, int size);
void KprXMLParserStartTag(void *data, const char *name, const char **attributes);
void KprXMLParserStopTag(void *data, const char *name);

FskErr KprXMLParse(KprXMLElement* root, unsigned char *data, FskInt64 size)
{
	FskErr err = kFskErrNone;
	KprXMLParserRecord record = {
		NULL,
		NULL,
		NULL
	};
	KprXMLParser self = &record;
	unsigned char* end;

	self->expat = XML_ParserCreate(NULL);
	bailIfNULL(self->expat);

	XML_SetUserData(self->expat, self);
	XML_SetElementHandler(self->expat, KprXMLParserStartTag, KprXMLParserStopTag);
	XML_SetCharacterDataHandler(self->expat, KprXMLParserDefault);
	end = (unsigned char *)FskStrNChr((char *)data, (UInt32)size, 0);
	if (end)
		size = end - data;
	if (!XML_Parse(self->expat, (const char*)data, (int)size, true)) {
		fprintf(stderr, "EXPAT ERROR: %s - %d %d\n", XML_ErrorString(XML_GetErrorCode(self->expat)), (int)XML_GetCurrentLineNumber(self->expat), (int)XML_GetCurrentColumnNumber(self->expat));
		BAIL(kFskErrBadData);
	}
	*root = self->root;
bail:
	if (self->expat)
		XML_ParserFree(self->expat);
	if (err)
		KprXMLElementDispose(self->root);
	return err;
}

void KprXMLParserDefault(void *data, const char *text, int size)
{
	FskErr err = kFskErrNone;
	KprXMLParser self = data;
	KprXMLElement element = self->element;
	const char* attributes = NULL;
	int i;

	for (i = 0; i < size; i++)
		if (!isspace(text[i])) break;
	if (element) {
		if ((i == size) && element->name)
			return;
		if ((i < size) && element->name) {
			bailIfError(KprXMLElementNew(&element, self->element, NULL, &attributes));
			self->element = element;
		}
		bailIfError(FskMemPtrRealloc(element->valueSize + size + 1, &element->value));
		FskMemCopy(element->value + element->valueSize, text, size);
		element->valueSize += size;
		element->value[element->valueSize] = 0;
	}
bail:
	return;
}

void KprXMLParserStartTag(void *data, const char *name, const char **attributes)
{
	FskErr err = kFskErrNone;
	KprXMLParser self = data;
	KprXMLElement element;

	if (!self->element || self->element->name) {
		bailIfError(KprXMLElementNew(&element, self->element, name, attributes));
	}
	else {
		bailIfError(KprXMLElementNew(&element, self->element->owner, name, attributes));
	}
	self->element = element;
	if (!self->root)
		self->root = element;
bail:
	return;
}

void KprXMLParserStopTag(void *data, const char *name UNUSED)
{
	KprXMLParser self = data;
	if (!self->element->name) {
		self->element = self->element->owner;
	}
	self->element = self->element->owner;
	return;
}

static FskErr KprXMLStorageWrite(FskGrowableStorage storage, char* data, UInt32 size)
{
	FskErr err = kFskErrNone;
	if (!size)
		size = FskStrLen(data);
 	bailIfError(FskGrowableStorageAppendItem(storage, data, size));
bail:
	return err;
}

static FskErr KprXMLStorageWriteEntity(FskGrowableStorage storage, char* data, UInt32 size, UInt32 theFlag)
{
	FskErr err = kFskErrNone;
	static unsigned char sEscape[256] = {
	/*  0 1 2 3 4 5 6 7 8 9 A B C D E F */
		3,3,3,3,3,3,3,3,3,1,1,3,3,3,3,3,	/* 0x                    */
		3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,	/* 1x                    */
		0,0,1,0,0,0,3,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
		0,0,0,0,0,0,0,0,0,0,0,0,3,0,2,0,	/* 3x  0123456789:;<=>?  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 	/* 7X  pqrstuvwxyz{|}~   */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 8X                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 9X                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* AX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* BX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* CX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* FX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* EX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 	/* FX                    */
	};
	static unsigned char sHexa[] = "0123456789ABCDEF";
	unsigned char* aText;
	unsigned char* aStop;
	unsigned char* aStart;
	unsigned char aBuffer[8];
	unsigned char aChar;
	Boolean withSize = size > 0;

	if (!data) return err;
	aText = (unsigned char*)data;
	aStop = aStart = aText;
	while ((withSize ? size-- : true) && (aChar = *aText++)) {
		if (sEscape[aChar] & theFlag) {
			if (aStop > aStart) {
				*aStop = '\0';
 				bailIfError(FskGrowableStorageAppendItem(storage, aStart, aStop - aStart));
				*aStop = aChar;
			}
			switch (aChar) {
			case '"':
 				bailIfError(FskGrowableStorageAppendItem(storage, "&quot;", 6));
				break;
			case '&':
 				bailIfError(FskGrowableStorageAppendItem(storage, "&amp;", 5));
				break;
			case '<':
 				bailIfError(FskGrowableStorageAppendItem(storage, "&lt;", 4));
				break;
			case '>':
 				bailIfError(FskGrowableStorageAppendItem(storage, "&gt;", 4));
				break;
			default:
				aStart = aBuffer;
				*(aStart++) = '&';
				*(aStart++) = '#';
				*(aStart++) = 'x';
				if (aChar >= 16)
					*(aStart++) = sHexa[aChar / 16];
				*(aStart++) = sHexa[aChar % 16];
				*(aStart++) = ';';
				*aStart = 0;
 				bailIfError(FskGrowableStorageAppendItem(storage, aBuffer, aStart - aBuffer));
				break;
			}
			aStart = ++aStop;
		}
		else
			aStop++;
	}
	if (aStop > aStart) {
		bailIfError(FskGrowableStorageAppendItem(storage, aStart, aStop - aStart));
	}
bail:
	return err;
}

static FskErr KprXMLSerializeElement(FskGrowableStorage storage, KprXMLElement element)
{
	FskErr err = kFskErrNone;
	KprXMLAttribute attribute = NULL;
	KprXMLElement child = NULL;

	if (element->value) {
		KprXMLStorageWriteEntity(storage, element->value, element->valueSize, 2);
		return err;
	}

	bailIfError(KprXMLStorageWrite(storage, "<", 1));
	if (element->nameSpace && element->nameSpace->name) {
		bailIfError(KprXMLStorageWrite(storage, element->nameSpace->name, 0));
		bailIfError(KprXMLStorageWrite(storage, ":", 1));
		bailIfError(KprXMLStorageWrite(storage, element->name, 0));
	}
	else
		bailIfError(KprXMLStorageWrite(storage, element->name, 0));

	attribute = element->attribute;
	while (attribute) {
		bailIfError(KprXMLStorageWrite(storage, " ", 1));
		if (attribute->isNamespace) {
			bailIfError(KprXMLStorageWrite(storage, "xmlns", 5));
			if (attribute->name) {
				bailIfError(KprXMLStorageWrite(storage, ":", 1));
				bailIfError(KprXMLStorageWrite(storage, attribute->name, 0));
			}
		}
		else {
			if (attribute->nameSpace && attribute->nameSpace->name) {
				bailIfError(KprXMLStorageWrite(storage, attribute->nameSpace->name, 0));
				bailIfError(KprXMLStorageWrite(storage, ":", 1));
			}
			bailIfError(KprXMLStorageWrite(storage, attribute->name, 0));
		}
		bailIfError(KprXMLStorageWrite(storage, "=\"", 2));
		bailIfError(KprXMLStorageWriteEntity(storage, attribute->value, 0, 1));
		bailIfError(KprXMLStorageWrite(storage, "\"", 1));
		attribute = attribute->next;
	}
	child = element->element;
	if (child) {
		bailIfError(KprXMLStorageWrite(storage, ">", 1));
		while (child) {
			bailIfError(KprXMLSerializeElement(storage, child));
			child = child->next;
		}
		bailIfError(KprXMLStorageWrite(storage, "</", 2));
		if (element->nameSpace && element->nameSpace->name) {
			bailIfError(KprXMLStorageWrite(storage, element->nameSpace->name, 0));
			bailIfError(KprXMLStorageWrite(storage, ":", 1));
			bailIfError(KprXMLStorageWrite(storage, element->name, 0));
		}
		else
			bailIfError(KprXMLStorageWrite(storage, element->name, 0));
		bailIfError(KprXMLStorageWrite(storage, ">", 1));
	}
	else {
		bailIfError(KprXMLStorageWrite(storage, "/>", 2));
	}
bail:
	return err;
}

FskErr KprXMLSerialize(KprXMLElement element, char** data, UInt32* size)
{
	FskErr err = kFskErrNone;
	FskGrowableStorage storage = NULL;
	UInt32 storageSize;
	char* storageData;
	char zero = 0;

	bailIfError(FskGrowableStorageNew(1024, &storage));
	KprXMLStorageWrite(storage, "<?xml version=\"1.0\" encoding=\"utf-8\"?>", 0);
	KprXMLSerializeElement(storage, element);
 	bailIfError(FskGrowableStorageAppendItem(storage, &zero, 1));
	FskGrowableStorageMinimize(storage);
	storageSize = FskGrowableStorageGetSize(storage);
	FskGrowableStorageGetPointerToItem(storage, 0, (void **)&storageData);
	bailIfError(FskMemPtrNew(storageSize, data));
	FskMemCopy(*data, storageData, storageSize);
	*size = storageSize - 1;
bail:
	FskGrowableStorageDispose(storage);
	return err;
}

/** KprRetainable */

FskErr KprRetainableNew(KprRetainable *it)
{
	FskErr err = kFskErrNone;
	KprRetainable self = NULL;

	bailIfError(KprMemPtrNew(sizeof(KprRetainableRecord), &self));

	self->retainCount = 1;

	*it = self;

bail:
	return err;
}

FskErr KprRetainableDispose(KprRetainable self)
{
	if (self) {
		KprMemPtrDispose(self);
	}
	return kFskErrNone;
}

void KprRetainableRetain(KprRetainable self)
{
	self->retainCount += 1;
}

Boolean KprRetainableRelease(KprRetainable self)
{
	self->retainCount -= 1;
	return (self->retainCount == 0);
}

/** KprMemoryBlock */

FskErr KprMemoryBlockNew(UInt32 size, const void *data, KprMemoryBlock *it)
{
	FskErr err = kFskErrNone;
	KprMemoryBlock self = NULL;
	char *p;

	bailIfError(KprMemPtrNew(sizeof(KprMemoryBlockRecord) + size + 1, &self));
	bailIfError(KprRetainableNew(&self->retainable));

	self->next = NULL;
	self->size = size;
	p = KprMemoryBlockStart(self);

	if (data && size > 0) FskMemCopy(p, data, size);
	p[size] = 0;

	*it = self;

bail:
	if (err) {
		KprMemPtrDispose(self);
	}
	return err;
}

FskErr KprMemoryBlockDispose(KprMemoryBlock self)
{
	if (self && KprRetainableRelease(self->retainable)) {
		KprMemoryBlockDispose(self->next);
		KprRetainableDispose(self->retainable);
		KprMemPtrDispose(self);
	}
	return kFskErrNone;
}

FskErr KprMemoryBlockDisposeAt(KprMemoryBlock *it)
{
	if (it) {
		KprMemoryBlockDispose(*it);
		*it = NULL;
	}
	return kFskErrNone;
}

KprMemoryBlock KprMemoryBlockRetain(KprMemoryBlock self)
{
	KprRetainableRetain(self->retainable);
	return self;
}

void *KprMemoryBlockStart(KprMemoryBlock self)
{
	UInt8 *p = (UInt8 *) self;
	p += sizeof(KprMemoryBlockRecord);
	return p;
}

void *KprMemoryBlockEnd(KprMemoryBlock self)
{
	UInt8 *p = (UInt8 *) KprMemoryBlockStart(self);
	return p + self->size;
}

void *KprMemoryBlockCopyTo(KprMemoryBlock self, void *dest)
{
	FskMemCopy(dest, KprMemoryBlockStart(self), self->size);
	return (UInt8 *) dest + self->size;
}

FskErr KprMemoryBlockToChunk(KprMemoryBlock self, xsMachine *the, xsSlot *ref)
{
	FskErr err;
	FskMemPtr data = NULL;

	bailIfError(KprMemPtrNewFromData(self->size, KprMemoryBlockStart(self), &data));

	xsMemPtrToChunk(the, ref, data, self->size, false);

bail:
	return err;
}

Boolean KprMemoryBlockIsSame(KprMemoryBlock a, KprMemoryBlock b)
{
	if (a == NULL || b == NULL || a->size != b->size) return false;
	return FskMemCompare(KprMemoryBlockStart(a), KprMemoryBlockStart(b), a->size) == 0;
}

/** KprSocketReader */

#define CALLBACK(x) if (self->x) self->x
#define kHeaderBufferSize 512

static void KprSocketReaderDataReader(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);
static KprSocketReaderState *KprSocketReaderFindForState(KprSocketReader self, int state);

FskErr KprSocketReaderNew(KprSocketReader *it, FskSocket skt, KprSocketReaderState states[], UInt32 stateCount, void *refcon)
{
	FskErr err = kFskErrNone;
	KprSocketReader self = NULL;

	bailIfError(KprMemPtrNewClear(sizeof(KprSocketReaderRecord), &self));
	bailIfError(KprMemPtrNewFromData(sizeof(KprSocketReaderState) * stateCount, states, &self->states));

	self->stateCount = stateCount;
	self->socket = skt;
	self->refcon = refcon;

	*it = self;
bail:
	if (err) KprSocketReaderDispose(self);
	return err;
}

FskErr KprSocketReaderDispose(KprSocketReader self)
{
	if (self) {
		if (self->inThreadDataHandler) {
			self->disposeRequested = true;
		} else {
			FskThreadRemoveDataHandler(&self->handler);
			self->socket = NULL;

			KprMemPtrDispose(self->leftover);
			KprMemPtrDispose(self->states);

			KprMemPtrDispose(self);
		}
	}
	return kFskErrNone;
}

static KprSocketReaderState *KprSocketReaderFindForState(KprSocketReader self, int state)
{
	UInt32 i;

	for (i = 0; i < self->stateCount; i++) {
		if (self->states[i].state == state) return &self->states[i];
	}

	return NULL;
}

static void KprSocketReaderDataReader(FskThreadDataHandler handler UNUSED, FskThreadDataSource source UNUSED, void *refCon)
{
	FskErr err = kFskErrNone;
	KprSocketReader self = refCon;

	self->inThreadDataHandler = true;

	while (err == kFskErrNone && !self->disposeRequested) {
		KprSocketReaderState *state = KprSocketReaderFindForState(self, self->state);

		err = state->callback(self, self->refcon);
	}

	switch (err) {
		case kFskErrNone:
		case kFskErrNoData:
			break;

		case kFskErrConnectionClosed:
			self->closed = true;
			FskThreadRemoveDataHandler(&self->handler);
			// fall through

		default:
			// report error.
			CALLBACK(errorCallback)(kKprSocketErrorOnRead, err, self->refcon);
			break;
	}

	self->inThreadDataHandler = false;

	if (self->disposeRequested) {
		KprSocketReaderDispose(self);
	}
}

void KprSocketReaderSetState(KprSocketReader self, int state)
{
#ifdef mxDebug
	{
		KprSocketReaderState *p = KprSocketReaderFindForState(self, state);
		if (p == NULL) {
			abort();
		}
	}
#endif

	self->state = state;
	KprSocketReaderResetRead(self);

	if (!self->stateInitialized) {
		self->stateInitialized = true;
		FskThreadAddDataHandler(&self->handler, (FskThreadDataSource)self->socket, KprSocketReaderDataReader, true, false, self);
	}

}

void KprSocketReaderResetRead(KprSocketReader self)
{
	self->bufferLength = 0;
}

FskErr KprSocketReaderReadDataFrom(KprSocketReader self, void *buffer, UInt32 *size, UInt32 *remoteIP, UInt16 *remotePort)
{
	FskErr err = kFskErrNone;
	int ip, port, amt;

	err = FskNetSocketRecvUDP(self->socket, buffer, *size, &amt, &ip, &port);
//	FskDebugStr("READ DATAGRAM: size: %d err: %d", amt, err);
	if (err != kFskErrNone) return err;

	*size = amt;
	*remoteIP = ip;
	*remotePort = port;
	FskTimeGetNow(&self->lastDataArrived);
	return kFskErrNone;
}

FskErr KprSocketReaderReadBytes(KprSocketReader self, void *buffer, size_t targetSize)
{
	FskErr err = kFskErrNone;
	UInt8 *p;
	UInt32 offset;
	int size, remains;

	offset = self->bufferLength;
	p = buffer;
	p += offset;

	remains = targetSize - offset;

	if (self->leftover) {
		UInt8 *leftover = self->leftover;

		size = self->leftoverLength;
		self->leftover = NULL;
		if (size > remains) {
			FskMemCopy(p, leftover, remains);

			self->leftoverLength = size - remains;
			err = KprMemPtrNewFromData(self->leftoverLength, leftover + remains, &self->leftover);

			remains = 0;
			self->bufferLength += remains;

		} else {
			FskMemCopy(p, leftover, size);
			remains -= size;
			self->bufferLength += size;
		}

		KprMemPtrDispose(leftover);
		if (err != kFskErrNone) return err;
		if (remains == 0) goto done;
	}

	err = FskNetSocketRecvTCP(self->socket, p, remains, &size);
	FskDebugStr("READ: size: %d offset: %d read: %d err: %d", (int) targetSize, (int) offset, (int) size, (int) err);
	if (err != kFskErrNone) return err;

	FskTimeGetNow(&self->lastDataArrived);

	self->bufferLength += size;
	if (self->bufferLength < targetSize) {
		return kFskErrNoData;
	}

done:
	self->bufferLength = 0;
	return kFskErrNone;
}

FskErr KprSocketReaderReadHTTPHeaders(KprSocketReader self, FskHeaders *headers)
{
	FskErr err = kFskErrNone;
	int size = -1, consumed;
	char buffer[kHeaderBufferSize];

	if (self->leftover) {
		size = self->leftoverLength;
		if (size < kHeaderBufferSize) {
			FskMemCopy(buffer, self->leftover, size);

			KprMemPtrDispose(self->leftover);
			self->leftover = NULL;
			self->leftoverLength = 0;
		} else {
			self->leftoverLength = size - kHeaderBufferSize;
			FskMemCopy(self->leftover, self->leftover + kHeaderBufferSize, self->leftoverLength);
		}
	} else {
		bailIfError(FskNetSocketRecvTCP(self->socket, buffer, kHeaderBufferSize, &size));

		FskTimeGetNow(&self->lastDataArrived);
	}

	consumed = FskHeadersParseChunk(buffer, size, kFskHeaderTypeResponse, headers);
	if (consumed > 0) {
		int leftover = size - consumed;

		if (leftover > 0) {
			bailIfError(KprSocketReaderUnreadBytes(self, buffer + consumed, leftover));
		}
	}

bail:
	return err;
}

FskErr KprSocketReaderUnreadBytes(KprSocketReader self, void *buffer, size_t length)
{
	FskErr err;

	if (self->leftover) return kFskErrBadState;

	bailIfError(KprMemPtrNewFromData(length, buffer, &self->leftover));
	self->leftoverLength = length;

bail:
	return err;
}

/** KprSocketWriter */

static FskErr KprSocketWriterTrySendingData(KprSocketWriter self, void *buffer, UInt32 length);
static void KprSocketWriterDataWriter(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);

FskErr KprSocketWriterNew(KprSocketWriter *it, FskSocket skt, void *refcon)
{
	FskErr err = kFskErrNone;
	KprSocketWriter self = NULL;

	bailIfError(KprMemPtrNewClear(sizeof(KprSocketWriterRecord), &self));

	self->socket = skt;
	self->refcon = refcon;

	*it = self;
bail:
	if (err) KprSocketWriterDispose(self);
	return err;
}

FskErr KprSocketWriterDispose(KprSocketWriter self)
{
	if (self) {
		if (self->inThreadDataHandler) {
			self->disposeRequested = true;
		} else {
			FskThreadRemoveDataHandler(&self->handler);
			self->socket = NULL;

			KprMemPtrDispose(self->pendingData);

			KprMemPtrDispose(self);
		}
	}
	return kFskErrNone;
}

void KprSocketWriterSetDestination(KprSocketWriter self, UInt32 ip, UInt16 port)
{
	self->targetIP = ip;
	self->targetPort = port;
}

void KprSocketWriterSendBytes(KprSocketWriter self, void *buffer, UInt32 length)
{
	FskErr err = kFskErrSocketNotConnected;

	if (self->socket == NULL) goto bail;

	if (self->pendingData) {
		UInt32 oldLength = self->pendingLength, newLength;
		UInt8 *data;

		newLength = oldLength + length;
		bailIfError(KprMemPtrNew(newLength, &data));

		FskMemCopy(data, self->pendingData, oldLength);
		FskMemCopy(data + oldLength, buffer, length);

		KprMemPtrDispose(self->pendingData);
		self->pendingData = data;
		self->pendingLength = newLength;
	} else {
		bailIfError(KprSocketWriterTrySendingData(self, buffer, length));
	}

bail:
	if (err) {
		CALLBACK(errorCallback)(kKprSocketErrorOnWrite, err, self->refcon);
	}
}

static void KprSocketWriterDataWriter(FskThreadDataHandler handler UNUSED, FskThreadDataSource source UNUSED, void *refCon)
{
	FskErr err = kFskErrNone;
	KprSocketWriter self = refCon;
	void *buffer;
	UInt32 length;

	self->inThreadDataHandler = true;

	FskThreadRemoveDataHandler(&self->handler);

	buffer = self->pendingData;
	length = self->pendingLength;
	self->pendingData = NULL;
	self->pendingLength = 0;

	err = KprSocketWriterTrySendingData(self, buffer, length);

	KprMemPtrDispose(buffer);

	if (err) {
		CALLBACK(errorCallback)(kKprSocketErrorOnWrite, err, self->refcon);
	}

	self->inThreadDataHandler = false;

	if (self->disposeRequested) {
		KprSocketWriterDispose(self);
	}
}

static FskErr KprSocketWriterTrySendingData(KprSocketWriter self, void *buffer, UInt32 length)
{
	FskErr err = kFskErrNone;
	int sent = 0;

	if (self->targetIP) {
		err = FskNetSocketSendUDP(self->socket, buffer, length, &sent, self->targetIP, self->targetPort);
	} else {
		err = FskNetSocketSendTCP(self->socket, buffer, length, &sent);
	}
	switch (err) {
		case kFskErrNone:
			length -= sent;
			FskTimeGetNow(&self->lastDataSent);
			break;

		case kFskErrSocketFull:
		case kFskErrNoData:
			err = kFskErrNone;
			break;

		default:
			goto bail;
	}

	if (length > 0 && !self->disposeRequested) {
		bailIfError(KprMemPtrNewFromData(length, (UInt8 *)buffer + sent, &self->pendingData));
		self->pendingLength = length;
		FskThreadAddDataHandler(&self->handler, (FskThreadDataSource)self->socket, KprSocketWriterDataWriter, false, true, self);
	}

bail:
	return err;
}

/** KprSocketServer */

static void KprSocketServerInterfaceAdded(FskNetInterface ifc, void *refcon);
static void KprSocketServerInterfaceRemoved(FskNetInterface ifc, void *refcon);
static void KprSocketServerInterfaceConnected(FskNetInterface ifc, void *refcon);
static void KprSocketServerInterfaceDisconnected(FskNetInterface ifc, void *refcon);

FskErr KprPortListenerNew(KprSocketServer server, UInt16 port, const char *interfaceName, KprPortListener *it);
FskErr KprPortListenerDispose(KprPortListener listener);


FskErr KprSocketServerNew(KprSocketServer *it, void *refcon)
{
	FskErr err = kFskErrNone;
	KprSocketServer self = NULL;

	bailIfError(KprMemPtrNewClear(sizeof(KprSocketServerRecord), &self));

	self->notifier.refcon = self;
	self->notifier.added = KprSocketServerInterfaceAdded;
	self->notifier.removed = KprSocketServerInterfaceRemoved;
	self->notifier.connected = KprSocketServerInterfaceConnected;
	self->notifier.disconnected = KprSocketServerInterfaceDisconnected;
	KprNetworkInterfaceAddNotifier(&self->notifier);

	self->refcon = refcon;

	*it = self;
	self = NULL;

bail:
	KprSocketServerDispose(self);
	return err;
}

FskErr KprSocketServerDispose(KprSocketServer self)
{
	if (self) {
		while (self->listeners) {
			KprPortListenerDispose(self->listeners);
		}

		KprNetworkInterfaceRemoveNotifier(&self->notifier);
		KprMemPtrDispose(self);
	}
	return kFskErrNone;
}

FskErr KprSocketServerListen(KprSocketServer self, UInt16 port, const char *interfaceName)
{
	FskErr err = kFskErrNone;

	if (interfaceName) {
		bailIfError(KprPortListenerNew(self, port, interfaceName, NULL));
	} else {
		FskNetInterfaceRecord *ifc;
		int i, numI;

		self->all = true;
		numI = FskNetInterfaceEnumerate();
		for (i = 0; i < numI; i++) {
			FskErr err = FskNetInterfaceDescribe(i, &ifc);
			if (err) continue;
			if (ifc->status) {
				KprPortListener listener = NULL;
				err = KprPortListenerNew(self, port, ifc->name, &listener);
				if (err == kFskErrNone && port == 0) {
					UInt32 localIP;
					int localPort;
					FskNetSocketGetLocalAddress(listener->socket, &localIP, &localPort);
					port = localPort;
				}
			}
			FskNetInterfaceDescriptionDispose(ifc);
			bailIfError(err);
		}
	}

	self->port = port;

bail:
	return err;
}

static KprPortListener KprSocketServerFindInterface(KprSocketServer self, const char *interfaceName)
{
	KprPortListener listener = self->listeners;

	while (listener) {
		if (FskStrCompare(interfaceName, listener->interfaceName)) break;
		listener = listener->next;
	}

	return listener;
}

static void KprSocketServerInterfaceAdded(FskNetInterface ifc, void *refcon)
{
	KprSocketServer self = refcon;
	KprPortListener	listener;

	if (self->all) {
		KprPortListenerNew(self, self->port, ifc->name, &listener);
	}
}

static void KprSocketServerInterfaceRemoved(FskNetInterface ifc, void *refcon)
{
	KprSocketServer self = refcon;
	KprPortListener	listener;

	listener = KprSocketServerFindInterface(self, ifc->name);
	if (listener) {
		KprPortListenerDispose(listener);
	}

	if (self->interfaceDroppedCallback) {
		self->interfaceDroppedCallback(self, ifc->name, ifc->ip, self->refcon);
	}
}

static void KprSocketServerInterfaceConnected(FskNetInterface ifc, void *refcon)
{
	printf("CONNECTED %s\n", ifc->name);
}

static void KprSocketServerInterfaceDisconnected(FskNetInterface ifc, void *refcon)
{
	printf("DISCONNECTED\n");
}

static FskErr KprPortListenerAcceptNewConnection(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);

FskErr KprPortListenerNew(KprSocketServer server, UInt16 port, const char *interfaceName, KprPortListener *it)
{
	KprPortListener self = NULL;
	FskErr err;
	FskSocket skt = NULL;
	FskNetInterfaceRecord *ifc = NULL;

	FskDebugStr("KprPortListenerNew - interfaceName: %s", interfaceName);
	bailIfError(KprMemPtrNewClear(sizeof(KprPortListenerRecord), &self));
	self->server = server;
	self->interfaceName = FskStrDoCopy(interfaceName);
	bailIfNULL(self->interfaceName);

	err = FskNetSocketNewTCP(&skt, true, server->debugName);
	if (err) {
		FskDebugStr("KprPortListenerNew -  creating socket failed: %d", (int) err);
        err = kFskErrNoMoreSockets;
		goto bail;
	}
	FskNetSocketReuseAddress(skt);
	ifc = FskNetInterfaceFindByName(self->interfaceName);
	if (NULL == ifc) {
		FskDebugStr("KprPortListenerNew - FskNetInterfaceFindByName returned NULL");
		err = kFskErrOperationFailed;
		goto bail;
	}
	self->ip = ifc->ip;
	err = FskNetSocketBind(skt, ifc->ip, port);
	if (kFskErrNone != err) {
		FskDebugStr("KprPortListenerNew - bind failed: %d port: %u", (int) err, (unsigned) port);
		goto bail;
	}

	FskNetSocketMakeNonblocking(skt);
	FskListAppend((FskList*)&server->listeners, self);
	FskDebugStr("KprPortListenerNew -  about to listen");

	FskNetSocketListen(skt);
	FskThreadAddDataHandler(&self->dataHandler, (FskThreadDataSource)skt, (FskThreadDataReadyCallback)KprPortListenerAcceptNewConnection, true, false, self);

	self->socket = skt;
	skt = NULL;

	if (it) *it = self;
	self = NULL;

bail:
	FskNetInterfaceDescriptionDispose(ifc);
	FskNetSocketClose(skt);
	KprPortListenerDispose(self);

	return err;
}

FskErr KprPortListenerDispose(KprPortListener self)
{
	if (self) {
		FskThreadRemoveDataHandler(&self->dataHandler);
		FskNetSocketClose(self->socket);

		if (self->server && self->server->listeners) {
			FskListRemove((FskList*)&self->server->listeners, self);
		}

		KprMemPtrDispose(self->interfaceName);

		KprMemPtrDispose(self);
		FskDebugStr("KprPortListenerDispose - listener: %p", self);
	}
	return kFskErrNone;
}

static FskErr KprPortListenerAcceptNewConnection(FskThreadDataHandler handler UNUSED, FskThreadDataSource source UNUSED, void *refcon) {
	KprPortListener self = refcon;
	FskErr err = kFskErrNone;
	KprSocketServer server = self->server;
	FskSocket skt = NULL;

	if (!server->acceptCallback) {
		err = kFskErrConnectionRefused;
		goto bail;
	}

	bailIfError(FskNetAcceptConnection(self->socket, &skt, server->debugName));

	bailIfError(server->acceptCallback(server, skt, self->interfaceName, self->ip, server->refcon));
	skt = NULL;

bail:
	FskNetSocketClose(skt);
	return err;
}

