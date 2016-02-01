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
#define __FSKTEXT_PRIV__ 1
#include "FskText.h"
#include "FskExtensions.h"
#include "FskUtilities.h"
#if FSKBITMAP_OPENGL
	#include "FskGLBlit.h"
#endif /* FSKBITMAP_OPENGL */


#if SUPPORT_INSTRUMENTATION
	#define FskInstrumentedGlobalType(type)			(&(g##type##TypeInstrumentation))												/**< The data structure created with FskInstrumentedSimpleType(). */
	#define FskInstrumentedTypeEnabled(type, level)	FskInstrumentedTypeHasListenersForLevel(FskInstrumentedGlobalType(type), level)	/**< Whether instrumentation will print anything. */
	#define	LOGD(...)	FskTextPrintfDebug(__VA_ARGS__)																				/**< Print debugging logs. */
	#define	LOGI(...)	FskTextPrintfVerbose(__VA_ARGS__)																			/**< Print information logs. */
	#define	LOGE(...)	FskTextPrintfMinimal(__VA_ARGS__)																			/**< Print error logs always, when instrumentation is on. */
#else /* !SUPPORT_INSTRUMENTATION */
	#define FskInstrumentedTypeEnabled(type, level)	(0)																				/**< Instrumentation will not print anything. */
	#define LOGD(...)																												/**< Don't print debugging logs. */
	#define LOGI(...)																												/**< Don't print information logs. */
	#define LOGE(...)																												/**< Don't print error logs. */
#endif /* SUPPORT_INSTRUMENTATION */

FskInstrumentedSimpleType(Text, text);														/**< This declares the types needed for instrumentation. */

#define LOGD_ENABLED()	FskInstrumentedTypeEnabled(Text, kFskInstrumentationLevelDebug)		/**< Whether LOGD() will print anything. */
#define LOGI_ENABLED()	FskInstrumentedTypeEnabled(Text, kFskInstrumentationLevelVerbose)	/**< Whether LOGI() will print anything. */
#define LOGE_ENABLED()	FskInstrumentedTypeEnabled(Text, kFskInstrumentationLevelMinimal)	/**< Whether LOGE() will print anything. */

#define FIX_TEXT_SIZE(fte, textSize) \
    if ((textSize >= 0x8000) && !fte->fractionalTextSize)    \
        textSize = FskRoundFixedToInt(textSize);

#if SUPPORT_INSTRUMENTATION
#include "FskPixelOps.h"

static void LogRect(FskConstRectangle r, const char *name) {
	if (!r)
		return;
	if (!name)
		name = "RECT";
	LOGD("\t%s(%d, %d, %d, %d)", name, (int)r->x, (int)r->y, (int)r->width, (int)r->height);
}

static void LogDstBitmap(FskConstBitmap dstBM, const char *name) {
	if (!dstBM)
		return;
	if (!name)
		name = "DSTBM";
	LOGD("\t%s: bounds(%d, %d, %d, %d), name, depth=%u, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d",
		name, (int)dstBM->bounds.x, (int)dstBM->bounds.y, (int)dstBM->bounds.width, (int)dstBM->bounds.height, (unsigned)dstBM->depth,
		FskBitmapFormatName(dstBM->pixelFormat), (int)dstBM->rowBytes, dstBM->bits, dstBM->hasAlpha, dstBM->alphaIsPremultiplied);
}

static void LogColor(FskConstColorRGBA color, const char *name) {
	if (!color)
		return;
	if (!name)
		name = "COLOR";
	LOGD("\t%s(%d, %d, %d, %d)", name, color->r, color->g, color->b, color->a);
}

static void LogTextFormatCache(FskTextFormatCache cache, const char *name) {
	#if defined(SUPPORT_TEXT_FORMAT_CACHE_DEBUG) && SUPPORT_TEXT_FORMAT_CACHE_DEBUG
		if (LOGD_ENABLED() && cache) {
			char	*infoStr	= NULL;
			#if     TARGET_OS_IPHONE
			#elif   TARGET_OS_MAC || TARGET_OS_MACOSX
				void FskCocoaTextFormatCacheInfoStringGet(FskTextFormatCache cache, char **pInfoStr);
				FskCocoaTextFormatCacheInfoStringGet(cache, &infoStr);
			#elif   TARGET_OS_WIN32
			#elif   TARGET_OS_ANDROID
			#elif   TARGET_OS_LINUX
			#elif   TARGET_OS_KPL
			#else /*TARGET_OS_UNKNOWN*/
			#endif/*TARGET_OS*/
			if (infoStr) {
				if (!name) name = "cache";
				LOGD("\t%s: %s", name, infoStr);
				FskMemPtrDispose(infoStr);
			}
		}
	#else /* SUPPORT_TEXT_FORMAT_CACHE_DEBUG */
		if (cache || name){}
	#endif /* SUPPORT_TEXT_FORMAT_CACHE_DEBUG */
}

static void LogTextBox(
	struct FskTextEngineRecord		*fte,
	FskConstBitmap					dstBM,
	const char						*text,
	UInt32							textLen,
	FskConstRectangle				dstRect,
	FskConstRectangle				dstClip,
	FskConstColorRGBA				color,
	UInt32							blendLevel,
	UInt32							textSize,
	UInt32							textStyle,
	UInt16							hAlign,
	UInt16							vAlign,
	const char						*fontName,
	struct FskTextFormatCacheRecord	*cache
) {
	LOGD("TextBox(fte=%p, dstBM=%p, text=\"%.*s\", textLen=%u, dstRect=%p, dstClip=%p, color=%p, blendLevel=%u, textSize=%u, textStyle=$%03X, hAlign=%u, vAlign=%u, fontName=\"%s\", cache=%p)",
		fte, dstBM, textLen, text, (unsigned)textLen, dstRect, dstClip, color, (unsigned)blendLevel, (unsigned)textSize, (unsigned)textStyle, (unsigned)hAlign, (unsigned)vAlign, fontName, cache);
	LogDstBitmap(dstBM, "dstBM");
	LogRect(dstRect, "dstRect");
	LogRect(dstClip, "dstClip");
	LogColor(color, "color");
	LogTextFormatCache(cache, "textFormatCache");
}
#endif /* SUPPORT_INSTRUMENTATION */


FskErr FskTextInitialize(void)
{
#if FSK_TEXT_FREETYPE
	extern FskErr FskTextFreeTypeInitialize(void);
	return FskTextFreeTypeInitialize();
#elif TARGET_OS_WIN32
	extern FskErr FskTextWindowsInitialize(void);
	return FskTextWindowsInitialize();
#elif TARGET_OS_MAC
	extern FskErr FskTextMacInitialize(void);
	return FskTextMacInitialize();
#else
	return kFskErrNone;
#endif
}

void FskTextTerminate(void)
{
#if FSK_TEXT_FREETYPE
	extern void FskTextFreeTypeUninitialize(void);
	FskTextFreeTypeUninitialize();
#elif TARGET_OS_WIN32
	extern void FskTextWindowsUninitialize(void);
	FskTextWindowsUninitialize();
#elif TARGET_OS_MAC
	extern void FskTextMacUninitialize(void);
	FskTextMacUninitialize();
#endif
}

FskErr FskTextEngineNew(FskTextEngine *fteOut, const char *name)
{
	FskErr err;
	UInt32 index = 0;
	FskTextEngine fte = NULL;
	FskTextEngineDispatch dispatch = NULL;
    FskMediaPropertyValueRecord prop;

	if ((NULL == name) || (0 == *name))
		name = kFskTextDefaultEngine;

	while (true) {
		FskTextEngineDispatch aDispatch = (FskTextEngineDispatch)FskExtensionGetByIndex(kFskExtensionTextEngine, index++);
		if (NULL == aDispatch)
			break;

		if (0 == FskStrCompare(name, aDispatch->name)) {
			dispatch = aDispatch;
			break;
		}
	}

	if (NULL == dispatch)
        BAIL(kFskErrUnimplemented);

	err = FskMemPtrNewClear(sizeof(FskTextEngineRecord), &fte);
	BAIL_IF_ERR(err);

	fte->dispatch = dispatch;

	if (NULL != dispatch->doNew) {
		err = (dispatch->doNew)(&fte->state);
		BAIL_IF_ERR(err);
	}

    if (kFskErrNone == FskTextEngineGetProperty(fte, kFskTextEnginePropertyFractionalTextSize, &prop))
        fte->fractionalTextSize = (kFskMediaPropertyTypeBoolean == prop.type) && prop.value.b;

bail:
	if ((kFskErrNone != err) && (NULL != fte)) {
		FskMemPtrDispose(fte);
		fte = NULL;
	}
	*fteOut = fte;

	return err;
}

FskErr FskTextEngineDispose(FskTextEngine fte)
{
	if (NULL == fte)
		return kFskErrNone;

	if (NULL != fte->dispatch->doDispose)
		(fte->dispatch->doDispose)(fte->state);

	FskMemPtrDispose(fte);

	return kFskErrNone;
}

FskErr FskTextAddFontFile(FskTextEngine fte, const char *path)
{
	if (NULL == fte->dispatch->doAddFontFile)
		return kFskErrUnimplemented;
	return (fte->dispatch->doAddFontFile)(fte->state, path);
}

FskErr FskTextDefaultFontSet(FskTextEngine fte, const char *defaultFace)
{
	if (NULL == fte->dispatch->doSetDefaultFont)
		return kFskErrUnimplemented;
	return (fte->dispatch->doSetDefaultFont)(fte->state, defaultFace);
}

FskErr FskTextBox(FskTextEngine fte, FskBitmap bits, const char *text, UInt32 textLen, FskConstRectangle dstRect, FskConstRectangleFloat dstRectFloat, FskConstRectangle clipRect, FskConstColorRGBA color, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, FskFixed textExtra, const char *fontName, FskTextFormatCache cache)
{
	#if FSKBITMAP_OPENGL
		if (FskBitmapIsOpenGLDestinationAccelerated(bits))
			return FskGLTextBox(fte, bits, text, textLen, dstRect, dstRectFloat, clipRect, color, blendLevel, textSize, textStyle, hAlign, vAlign, textExtra, fontName, cache);
	#endif /* FSKBITMAP_OPENGL */

	#if SUPPORT_INSTRUMENTATION
		LogTextBox(fte, bits, text, textLen, dstRect, clipRect, color, blendLevel, textSize, textStyle, hAlign, vAlign, fontName, cache);
	#endif /* SUPPORT_INSTRUMENTATION */

	if (NULL == fte->dispatch->doBox)
		return kFskErrUnimplemented;
    
    FIX_TEXT_SIZE(fte, textSize);
	return (fte->dispatch->doBox)(fte->state, bits, text, textLen, dstRect, dstRectFloat, clipRect, color, blendLevel, textSize, textStyle, hAlign, vAlign, textExtra, fontName, cache);
}

FskErr FskTextGetBounds(FskTextEngine fte, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, FskFixed textExtra, const char *fontName, FskRectangle bounds, FskDimensionFloat dimensions, FskTextFormatCache cache)
{
	if (NULL == fte->dispatch->doGetBounds)
		return kFskErrUnimplemented;
    FIX_TEXT_SIZE(fte, textSize);
	return (fte->dispatch->doGetBounds)(fte->state, bits, text, textLen, textSize, textStyle, textExtra, fontName, bounds, dimensions, cache);
}

FskErr FskTextGetLayout(FskTextEngine fte, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, FskFixed textExtra, const char *fontName,
						UInt16 **unicodeText, UInt32 *unicodeLen, FskFixed **layout, FskTextFormatCache cache)
{
	if (NULL == fte->dispatch->doGetLayout)
		return kFskErrUnimplemented;

    FIX_TEXT_SIZE(fte, textSize);
	return (fte->dispatch->doGetLayout)(fte->state, bits, text, textLen, textSize, textStyle, textExtra, fontName, unicodeText, unicodeLen, layout, cache);
}

FskErr FskTextGetFontList(FskTextEngine fte, char **fontNames)
{
	if (NULL == fte->dispatch->doGetFontList)
		return kFskErrUnimplemented;
	return (fte->dispatch->doGetFontList)(fte->state, fontNames);
}

FskErr FskTextFitWidth(FskTextEngine fte, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, FskFixed textExtra, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytesOut, UInt32 *fitCharsOut, FskTextFormatCache cache)
{
	if (NULL == fte->dispatch->doFitWidth)
		return kFskErrUnimplemented;

    FIX_TEXT_SIZE(fte, textSize);
	return (fte->dispatch->doFitWidth)(fte->state, bits, text, textLen, textSize, textStyle, textExtra, fontName, width, flags, fitBytesOut, fitCharsOut, cache);
}

FskErr FskTextFormatCacheNew(FskTextEngine fte, FskTextFormatCache *cache, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName)
{
	if (NULL == fte->dispatch->doFormatCacheNew)
		return kFskErrUnimplemented;

    FIX_TEXT_SIZE(fte, textSize);
	return (fte->dispatch->doFormatCacheNew)(fte->state, cache, bits, textSize, textStyle, fontName);
}

FskErr FskTextFormatCacheDispose(FskTextEngine fte, FskTextFormatCache cache)
{
	if (NULL == fte->dispatch->doFormatCacheDispose)
		return kFskErrUnimplemented;
	return (fte->dispatch->doFormatCacheDispose)(fte->state, cache);
}

FskErr FskTextGetFontInfo(FskTextEngine fte, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache formatCache)
{
	if (NULL == fte->dispatch->doGetFontInfo)
		return kFskErrUnimplemented;

    FIX_TEXT_SIZE(fte, textSize);
	return (fte->dispatch->doGetFontInfo)(fte->state, info, fontName, textSize, textStyle, formatCache);
}

FskErr FskTextSetFamilyMap(FskTextEngine fte, const char *map)
{
	if (NULL == fte->dispatch->doSetFamilyMap)
		return kFskErrUnimplemented;
	return (fte->dispatch->doSetFamilyMap)(fte->state, map);
}

FskErr FskTextEngineHasProperty(FskTextEngine fte, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	return FskMediaHasProperty(fte->dispatch->properties, propertyID, get, set, dataType);
}

FskErr FskTextEngineSetProperty(FskTextEngine fte, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaSetProperty(fte->dispatch->properties, fte->state, fte, propertyID, property);
}

FskErr FskTextEngineGetProperty(FskTextEngine fte, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(fte->dispatch->properties, fte->state, fte, propertyID, property);
}
