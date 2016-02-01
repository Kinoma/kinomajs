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
#define __FSKTEXT_PRIV__
#include "FskText.h"

#include "FskEdgeEnhancedText.h"
#include "FskExtensions.h"
#include "FskPixelOps.h"
#include "FskTextConvert.h"
#include "FskTransferAlphaBitmap.h"
#include "FskUtilities.h"

#if !USE_CORE_TEXT
	#include <ATSFont.h>
#endif

#include "FskCocoaSupportCommon.h"

static FskErr macTextNew(FskTextEngineState *state);
static FskErr macTextDispose(FskTextEngineState state);
static FskErr macTextFormatCacheNew(FskTextEngineState state, FskTextFormatCache *cache, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName);
static FskErr macTextFormatCacheDispose(FskTextEngineState state, FskTextFormatCache cache);
static FskErr macTextBox(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, FskConstRectangle bounds, FskConstRectangleFloat boundsFloat, FskConstRectangle clipRect, FskConstColorRGBA color, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, FskFixed textExtra, const char *fontName, FskTextFormatCache cache);
static FskErr macTextGetBounds(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, FskFixed textExtra, const char *fontName, FskRectangle bounds, FskDimensionFloat dimension, FskTextFormatCache cache);
static FskErr macTextGetFontInfo(FskTextEngineState state, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache formatCache);
static FskErr macTextFitWidth(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, FskFixed textExtra, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytes, UInt32 *fitChars, FskTextFormatCache cache);
static FskErr macTextGetLayout(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, FskFixed textExtra, const char *fontName, UInt16 **unicodeText, UInt32 *unicodeLen, FskFixed **layout, FskTextFormatCache cache);

static FskErr macTextAddFontFile(FskTextEngineState state, const char *path);
static FskErr macTextGetFontList(FskTextEngineState state, char **fontNames);

static FskErr macTextGetFractionalTextSize(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord macTextProperties[] = {
	{kFskTextEnginePropertyFractionalTextSize,		kFskMediaPropertyTypeBoolean,		macTextGetFractionalTextSize,		NULL},
	{kFskMediaPropertyUndefined,                    kFskMediaPropertyTypeUndefined,		NULL,                               NULL}
};

static FskTextEngineDispatchRecord gFskTextMacDispatch = {
	"Cocoa/CoreText",
	macTextNew,
	macTextDispose,
	macTextFormatCacheNew,
	macTextFormatCacheDispose,
	macTextBox,
	macTextGetBounds,
	macTextGetFontInfo,
	macTextFitWidth,
	macTextGetLayout,
	macTextAddFontFile,
	NULL,
	macTextGetFontList,
	NULL,
	macTextProperties
};

FskErr FskTextMacInitialize(void);
void FskTextMacUninitialize(void);

/*
	configuration
*/

FskErr FskTextMacInitialize(void)
{
	FskExtensionInstall(kFskExtensionTextEngine, &gFskTextMacDispatch);

	FskCocoaTextInitialize();

	return kFskErrNone;
}

void FskTextMacUninitialize(void)
{
	FskCocoaTextUninitialize();

	FskExtensionUninstall(kFskExtensionTextEngine, &gFskTextMacDispatch);
}

FskErr macTextAddFontFile(FskTextEngineState state, const char *path)
{
	return kFskErrUnimplemented;
}


FskErr macTextNew(FskTextEngineState *state)
{
	return FskCocoaTextNew((void**)state);
}

FskErr macTextDispose(FskTextEngineState state)
{
	return FskCocoaTextDispose((void*)state);
}

/********************************************************************************
 * FskTextBox
 ********************************************************************************/

FskErr macTextBox(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, FskConstRectangle r, FskConstRectangleFloat rFloat, FskConstRectangle clipRect, FskConstColorRGBA color,UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, FskFixed textExtra, const char *fontName, FskTextFormatCache cache)
{
	FskErr err = kFskErrNone;

	if (!FskCocoaTextDraw((void*)state, bits, text, textLen, r, clipRect, color, blendLevel, textSize, textStyle, hAlign, vAlign, textExtra, fontName, cache))
		err = kFskErrOperationFailed;

	return err;
}


/********************************************************************************
 * FskTextGetBounds
 ********************************************************************************/

FskErr macTextGetBounds(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, FskFixed textExtra, const char *fontName, FskRectangle bounds, FskDimensionFloat dimension, FskTextFormatCache cache)
{
	if (dimension) {
		FskCocoaTextGetBoundsSubpixel((void*)state, text, textLen, textSize, textStyle, textExtra, fontName, dimension, cache);
		FskRectangleSet(bounds, 0, 0, (SInt32)ceilf(dimension->width), (SInt32)roundf(dimension->height));
	}
	else
		FskCocoaTextGetBounds((void*)state, text, textLen, textSize, textStyle, textExtra, fontName, bounds, cache);

	return kFskErrNone;
}

static FskErr macTextGetLayout(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, FskFixed textExtra, const char *fontName,
								UInt16 **unicodeText, UInt32 *unicodeLen, FskFixed **layout, FskTextFormatCache cache)
{
	FskCocoaTextGetLayout((void*)state, text, textLen, textSize, textStyle, textExtra, fontName, unicodeText, unicodeLen, layout, cache);
	return kFskErrNone;
}

/********************************************************************************
 * FskTextGetFontList
 ********************************************************************************/

FskErr macTextGetFontList(FskTextEngineState state, char **fontNames)
{
	FskCocoaTextGetFontList((void*)state, fontNames);

	return kFskErrNone;
}

FskErr macTextFitWidth(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, FskFixed textExtra, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytesOut, UInt32 *fitCharsOut, FskTextFormatCache cache)
{
	return FskCocoaTextFitWidth((void*)state, text, textLen, textSize, textStyle, textExtra, fontName, width, flags, fitBytesOut, fitCharsOut, cache) ? kFskErrNone: kFskErrOperationFailed;
}

FskErr macTextGetFontInfo(FskTextEngineState state, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache cache)
{
	FskCocoaTextGetFontInfo((void*)state, info, fontName, textSize, textStyle, cache);

	return kFskErrNone;
}

FskErr macTextFormatCacheNew(FskTextEngineState state, FskTextFormatCache *cache, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName)
{
	FskErr err = kFskErrNone;

	if (!FskCocoaTextFormatCacheNew((void*)state, cache, bits, textSize, textStyle, fontName))
		err = kFskErrOperationFailed;

	return err;
}

FskErr macTextFormatCacheDispose(FskTextEngineState state, FskTextFormatCache cache)
{
	FskCocoaTextFormatCacheDispose((void*)state, cache);

	return kFskErrNone;
}

/********************************************************************************
 * FskTextEngineGetProperty
 ********************************************************************************/

FskErr macTextGetFractionalTextSize(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeBoolean;
#if USE_FRACTIONAL_TEXT_SIZE
	property->value.b = true;
#else
	property->value.b = false;
#endif

	return kFskErrNone;
}
