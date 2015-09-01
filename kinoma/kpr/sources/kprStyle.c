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
#include "FskImage.h"
#include "FskPixelOps.h"
#include "FskRectBlit.h"
#include "FskText.h"
#include "FskTextConvert.h"
#include "FskUtilities.h"

#include "kprContent.h"
#include "kprEffect.h"
#include "kprHandler.h"
#include "kprLayer.h"
#include "kprSkin.h"
#include "kprStyle.h"
#include "kprURL.h"
#include "kprShell.h"
#include "kprUtilities.h"

/* STYLE */

static void KprStyleDispose(void* it);
static void KprStyleFormat(KprStyle self);
static void KprStyleInvalidate(KprStyle self);
static void KprStyleUpdate(KprStyle self);

static UInt32 KprComputeScaledSize(FskPort port, UInt32 size);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprStyleInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprStyle", FskInstrumentationOffset(KprStyleRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

char* kprDefaultFont = NULL;
static UInt32 kprFitFonts = 1;

FskErr KprStyleNew(KprStyle *it, KprContext context, KprStyle father, KprStyle mother)
{
	FskErr err = kFskErrNone;
	KprStyle self;
	bailIfError(KprAssetNew((KprAsset *)it, sizeof(KprStyleRecord), context, &context->firstStyle, KprStyleDispose));
	self = *it;	
	FskInstrumentedItemNew(self, NULL, &KprStyleInstrumentation);
#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemSetOwner(self, context);
#endif
	self->father = father;
	KprAssetBind(father);
	self->mother = mother;
	KprAssetBind(mother);
	if (mother) {
		self->nextChild = mother->firstChild;
		mother->firstChild = self;
	}
bail:
	return err;
}

void KprStyleApply(KprStyle self, FskPort port)
{
	if (!self->textFormat)
		KprStyleFormat(self);
    FskPortTextFormatApply(port, self->textFormat);
}

KprStyle KprStyleCascade(KprStyle self, KprStyle father)
{
	KprStyle child;
	ASSERT(father);
	if (!self)
		return father;
	ASSERT(!self->mother);
	if (father->mother == self)
		return father;
	child = self->firstChild;
	while (child) {
		if (child->father == father)
			return child;
		child = child->nextChild;
	}
	KprStyleNew(&child, self->context, father, self);
	KprStyleInherit(child);	
	return child;
}

void KprStyleClearColor(KprStyle self, SInt32 which)
{
	if ((0 <= which) && (which < 4))
		self->flags &= ~(1 << which);
	else
		self->flags &= ~(kprColor0 | kprColor1 | kprColor2 | kprColor3);
	KprStyleInvalidate(self);
}

void KprStyleClearHorizontalAlignment(KprStyle self)
{
	self->flags &= ~kprHorizontalAlignment;
	KprStyleInvalidate(self);
}

void KprStyleClearVerticalAlignment(KprStyle self)
{
	self->flags &= ~kprVerticalAlignment;
	KprStyleInvalidate(self);
}

void KprStyleClearIndentation(KprStyle self)
{
	self->flags &= ~kprIndentation;
	KprStyleInvalidate(self);
}

void KprStyleClearLineCount(KprStyle self)
{
	self->flags &= ~kprLineCount;
	KprStyleInvalidate(self);
}

void KprStyleClearLineHeight(KprStyle self)
{
	self->flags &= ~kprLineHeight;
	KprStyleInvalidate(self);
}

void KprStyleClearMarginLeft(KprStyle self)
{
	self->flags &= ~kprMarginLeft;
	KprStyleInvalidate(self);
}

void KprStyleClearMarginTop(KprStyle self)
{
	self->flags &= ~kprMarginTop;
	KprStyleInvalidate(self);
}

void KprStyleClearMarginRight(KprStyle self)
{
	self->flags &= ~kprMarginRight;
	KprStyleInvalidate(self);
}

void KprStyleClearMarginBottom(KprStyle self)
{
	self->flags &= ~kprMarginBottom;
	KprStyleInvalidate(self);
}

void KprStyleClearTextFont(KprStyle self)
{
	FskMemPtrDisposeAt(&self->textFont);
	self->flags &= ~kprTextFont;
	KprStyleInvalidate(self);
}

void KprStyleClearTextSize(KprStyle self)
{
	self->flags &= ~kprTextSize;
	KprStyleInvalidate(self);
}

void KprStyleClearTextStyle(KprStyle self)
{
	self->flags &= ~kprTextStyle;
	KprStyleInvalidate(self);
}

Boolean KprStyleColorize(KprStyle self, FskPort port, double state)
{
	double index;
	FskColorRGBARecord color;
	if (state < 0) state = 0;
	if (3 < state) state = 3;
	index = floor(state);
	if (index == state)
		color = self->colors[(UInt32)index];
	else {
		UInt32 dst, src;
		FskConvertColorRGBAToBitmapPixel(&self->colors[(UInt32)index], kFskBitmapFormat32ARGB, &dst);
		FskConvertColorRGBAToBitmapPixel(&self->colors[(UInt32)index + 1], kFskBitmapFormat32ARGB, &src);
		FskName2(FskAlphaBlend,fsk32ARGBFormatKind)(&dst, src, (UInt8)((state - index) * 255.0));
		FskConvertBitmapPixelToColorRGBA(&dst, kFskBitmapFormat32ARGB, &color);
	}
		
	if (port->graphicsModeParameters && (port->graphicsModeParameters->dataSize == sizeof(FskGraphicsModeParametersRecord))) {
		UInt32 c = color.a;
		UInt32 a = port->graphicsModeParameters->blendLevel;
		color.a = (c *= a, c += (1 << (8 - 1)), c += c >> 8, c >>= 8);
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

void KprStyleDispose(void* it)
{
	KprStyle self = it;
	KprStyle father = self->father;
	KprStyle mother = self->mother;
	if (father)
		KprAssetUnbind(father);
	if (mother) {
		KprStyle *address = &mother->firstChild, child;
		while ((child = *address)) {
			if (self == child) {
				*address = child->nextChild;
				break;
			}
			address = &child->nextChild;
		}
		KprAssetUnbind(mother);
	}
	FskPortTextFormatDispose(self->textFormat);
	if (self->flags & kprTextFont)
		FskMemPtrDispose(self->textFont);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(it);
}

void KprStyleFormat(KprStyle self)
{
	char* textFont;
	UInt32 textScaledSize;
	FskInstrumentedItemSendMessageNormal(self, kprInstrumentedStyleFormat, NULL);
	textFont = self->textFont;
	if (!textFont)
		textFont = kprDefaultFont;
	FskPortSetTextFont(gShell->port, textFont);
	FskPortSetTextStyle(gShell->port, self->textStyle);
	textScaledSize = KprComputeScaledSize(gShell->port, self->textSize);
	if (self->textFormat) {
		if ((self->textFormat->textStyle != self->textStyle) || (self->textFormat->textSize != textScaledSize) || FskStrCompare(self->textFormat->fontName, textFont)) {
			FskPortTextFormatDispose(self->textFormat);
			self->textFormat = NULL;
		}
	}
	if (!self->textFormat) {
		FskTextFontInfoRecord fontInfo;
		FskPortTextFormatNew(&self->textFormat, gShell->port, textScaledSize, self->textStyle, textFont);
    	FskPortTextFormatApply(gShell->port, self->textFormat);
		FskPortGetFontInfo(gShell->port, &fontInfo);
		self->ascent = fontInfo.ascent;
		self->descent = fontInfo.descent;
	}
	self->flags &= ~kprStyleDirty;
}

void KprStyleGetInfo(KprStyle self, UInt16* ascent, UInt16* descent)
{
	if (!self->textFormat)
		KprStyleFormat(self);
	*ascent = self->ascent;
	*descent = self->descent;
}

void KprStyleInherit(KprStyle self)
{
	KprStyle father = self->father;
	KprStyle mother = self->mother;
	UInt32 flags = mother->flags;
	if (flags & kprColor0)
		self->colors[0] = mother->colors[0];
	else
		self->colors[0] = father->colors[0];
	if (flags & kprColor1)
		self->colors[1] = mother->colors[1];
	else
		self->colors[1] = father->colors[1];
	if (flags & kprColor0)
		self->colors[2] = mother->colors[2];
	else
		self->colors[2] = father->colors[2];
	if (flags & kprColor0)
		self->colors[3] = mother->colors[3];
	else
		self->colors[3] = father->colors[3];
	if (flags & kprHorizontalAlignment)
		self->horizontalAlignment = mother->horizontalAlignment;
	else
		self->horizontalAlignment = father->horizontalAlignment;
	if (flags & kprVerticalAlignment)
		self->verticalAlignment = mother->verticalAlignment;
	else
		self->verticalAlignment = father->verticalAlignment;
	if (flags & kprIndentation)
		self->indentation = mother->indentation;
	else
		self->indentation = father->indentation;
	if (flags & kprLineCount)
		self->lineCount = mother->lineCount;
	else
		self->lineCount = father->lineCount;
	if (flags & kprLineHeight)
		self->lineHeight = mother->lineHeight;
	else
		self->lineHeight = father->lineHeight;
	if (flags & kprMarginLeft)
		self->margins.left = mother->margins.left;
	else
		self->margins.left = father->margins.left;
	if (flags & kprMarginTop)
		self->margins.top = mother->margins.top;
	else
		self->margins.top = father->margins.top;
	if (flags & kprMarginRight)
		self->margins.right = mother->margins.right;
	else
		self->margins.right = father->margins.right;
	if (flags & kprMarginBottom)
		self->margins.bottom = mother->margins.bottom;
	else
		self->margins.bottom = father->margins.bottom;
	if (flags & kprTextFont)
		self->textFont = mother->textFont;
	else
		self->textFont = father->textFont;
	if (flags & kprTextSize) {
		if (mother->textSize >= 0)
			self->textSize = mother->textSize;
		else
			self->textSize = (father->textSize * -mother->textSize) / 100;
	}
	else
		self->textSize = father->textSize;
	if (flags & kprTextStyle) {
		if (mother->textStyle == kFskTextPlain)
			self->textStyle = kFskTextPlain;
		else
			self->textStyle = father->textStyle | mother->textStyle;
	}
	else
		self->textStyle = father->textStyle;
	if (self->textFormat) {
		FskPortTextFormatDispose(self->textFormat);
		self->textFormat = NULL;
	}
}

void KprStyleInvalidate(KprStyle self)
{
	self->flags |= kprStyleDirty;
	if (self->textFormat) {
		FskPortTextFormatDispose(self->textFormat);
		self->textFormat = NULL;
	}
	if (self->usage) {
		gShell->flags |= kprAssetsChanged;
		FskPortInvalidateRectangle(gShell->port, NULL);
	}
}

void KprStyleMeasure(KprStyle self, char* string, FskRectangle bounds)
{
	KprShellAdjust(gShell);
	KprStyleApply(self, gShell->port);
	FskPortTextGetBounds(gShell->port, string, FskStrLen(string), bounds);
	bounds->width += self->margins.left + self->margins.right;
	bounds->height += self->margins.top + self->margins.bottom;
}

void KprStyleSetColor(KprStyle self, SInt32 which, FskColorRGBA color)
{
	if ((0 <= which) && (which < 4)) {
		self->colors[which] = *color;
		self->flags |= 1 << which;
	}
	else {
		for (which = 0; which < 4; which++)
			self->colors[which] = *color;
		self->flags |= kprColor0 | kprColor1 | kprColor2 | kprColor3;
	}
	KprStyleInvalidate(self);
}
 
void KprStyleSetHorizontalAlignment(KprStyle self, UInt16 alignment)
{
	self->horizontalAlignment = alignment;
	self->flags |= kprHorizontalAlignment;
	KprStyleInvalidate(self);
}

void KprStyleSetVerticalAlignment(KprStyle self, UInt16 alignment)
{
	self->verticalAlignment = alignment;
	self->flags |= kprVerticalAlignment;
	KprStyleInvalidate(self);
}

void KprStyleSetIndentation(KprStyle self, UInt32 indentation)
{
	self->indentation = indentation;
	self->flags |= kprIndentation;
	KprStyleInvalidate(self);
}

void KprStyleSetLineCount(KprStyle self, UInt32 lineCount)
{
	self->lineCount = lineCount;
	self->flags |= kprLineCount;
	KprStyleInvalidate(self);
}

void KprStyleSetLineHeight(KprStyle self, UInt32 lineHeight)
{
	self->lineHeight = lineHeight;
	self->flags |= kprLineHeight;
	KprStyleInvalidate(self);
}

void KprStyleSetMarginLeft(KprStyle self, SInt32 marginLeft)
{
	self->margins.left = marginLeft;
	self->flags |= kprMarginLeft;
	KprStyleInvalidate(self);
}

void KprStyleSetMarginTop(KprStyle self, SInt32 marginTop)
{
	self->margins.top = marginTop;
	self->flags |= kprMarginTop;
	KprStyleInvalidate(self);
}

void KprStyleSetMarginRight(KprStyle self, SInt32 marginRight)
{
	self->margins.right = marginRight;
	self->flags |= kprMarginRight;
	KprStyleInvalidate(self);
}

void KprStyleSetMarginBottom(KprStyle self, SInt32 marginBottom)
{
	self->margins.bottom = marginBottom;
	self->flags |= kprMarginBottom;
	KprStyleInvalidate(self);
}

void KprStyleSetTextFont(KprStyle self, char* textFont)
{
	if (self->flags & kprTextFont)
		FskMemPtrDisposeAt(&self->textFont);
	if (textFont) {
        UInt32 length = FskStrLen(textFont) + 2;
 		if (kFskErrNone == FskMemPtrNew(length, &self->textFont)) {
            char *d = self->textFont;
            FskMemMove(d, textFont, length - 1);
            while ((d = FskStrChr(d, ',')))
                *d++ = 0;
            self->textFont[length - 1] = 0;
        }
    }
	self->flags |= kprTextFont;
	KprStyleInvalidate(self);
}

void KprStyleSetTextSize(KprStyle self, SInt32 textSize)
{
	self->textSize = textSize;
	self->flags |= kprTextSize;
	KprStyleInvalidate(self);
}

void KprStyleSetTextStyle(KprStyle self, UInt32 textStyle)
{
	self->textStyle = textStyle;
	self->flags |= kprTextStyle;
	KprStyleInvalidate(self);
}

KprStyle KprStyleUncascade(KprStyle self)
{
	if (self->mother)
		return self->mother;
	return self;
}

void KprStyleUpdate(KprStyle self)
{
	KprStyle father = self->father;
	KprStyle mother = self->mother;
	if (father && mother) {
		KprStyleUpdate(father);
		if (!(self->flags & kprStyleDirty)) {
			if ((father->flags & kprStyleDirty) || (mother->flags & kprStyleDirty))
				KprStyleInherit(self);
			self->flags |= kprStyleDirty;
		}
	}
}

/* SHELL */

FskErr KprShellDefaultStyles(KprShell self)
{
	FskErr err = kFskErrNone;
	char* font = NULL;
	UInt32 size, style;
#if TARGET_OS_MAC && TARGET_OS_IPHONE
	kprDefaultFont = "Helvetica\0";
#elif TARGET_OS_LINUX && TARGET_OS_ANDROID
	char* version = NULL;
	if (gAndroidCallbacks->getModelInfoCB)
		gAndroidCallbacks->getModelInfoCB(NULL, &version, NULL, NULL, NULL);
	if (version && (FskStrToNum(version + 8) >= 4))
		kprDefaultFont = "Roboto\0";
	else
		kprDefaultFont = "Droid Sans\0";
	FskMemPtrDispose(version);
#else
	kprDefaultFont = "Arial\0";
#endif
	FskPortGetTextFont(self->port, &font);
	FskPortGetTextSize(self->port, &size);
	FskPortGetTextStyle(self->port, &style);
	bailIfError(KprStyleNew(&self->style, (KprContext)self, NULL, NULL));
	KprAssetBind(self->style);
	KprStyleSetTextFont(self->style, font);
	KprStyleSetTextSize(self->style, size);
	KprStyleSetTextStyle(self->style, style);
	kprFitFonts = KprEnvironmentGetUInt32("fitFonts", 1);
bail:
	FskMemPtrDispose(font);
	return err;
}

void KprShellUpdateStyles(KprShell self)
{
	KprStyle style = (KprStyle)self->firstStyle;
	while (style) {
		KprStyleUpdate(style);
		style = (KprStyle)style->next;
	}
	style = (KprStyle)self->firstStyle;
	while (style) {
		style->flags &= ~kprStyleDirty;
		style = (KprStyle)style->next;
	}
}

/* UTILITIES */

UInt32 KprComputeScaledSize(FskPort port, UInt32 size)
{
	if (!port) return 0;
    if (kprFitFonts) {
		UInt32 targetHeight = size;
        size <<= 16;
		while (size > 0) {
			FskTextFontInfoRecord info;

			FskPortSetTextSize(port, size);
			FskPortGetFontInfo(port, &info);

			if (info.glyphHeight <= targetHeight)
				break;

			size -= (1 << 16) >> 2;
		}
	}
	else
		size = FskPortUInt32Scale(port, size << 16);
	return size;
}

Boolean KprParseHorizontalAlignment(char* s, UInt16 *alignment)
{
	if (!FskStrCompare(s, "center")) {
		*alignment = kFskTextAlignCenter;
		return true;
	}
	if (!FskStrCompare(s, "left")) {
		*alignment = kFskTextAlignLeft;
		return true;
	}
	if (!FskStrCompare(s, "right")) {
		*alignment = kFskTextAlignRight;
		return true;
	}
	if (!FskStrCompare(s, "justify")) {
		*alignment = kprLeftRight;
		return true;
	}
	return false;
}

Boolean KprParseVerticalAlignment(char* s, UInt16 *alignment)
{
	if (!FskStrCompare(s, "middle")) {
		*alignment = kFskTextAlignCenter;
		return true;
	}
	if (!FskStrCompare(s, "top")) {
		*alignment = kFskTextAlignTop;
		return true;
	}
	if (!FskStrCompare(s, "bottom")) {
		*alignment = kFskTextAlignBottom;
		return true;
	}
	return false;
}

/* ECMASCRIPT */

void KPR_style(void *it)
{
	if (it) {
		KprStyle self = it;
		kprVolatileDestructor(KPR_style);
		KprAssetDispose(self);
	}
}

void KPR_Style(xsMachine *the)
{
	KprStyle self;
	xsIntegerValue c = xsToInteger(xsArgc);
	xsVars(1);
	KprStyleNew(&self, xsGetContext(the), NULL, NULL);
	kprVolatileConstructor(KPR_Style);
	KPR_style_set_font(the);
	if ((c > 1) && xsTest(xsArg(1))) {
		FskColorRGBARecord color;
		if (xsIsInstanceOf(xsArg(1), xsArrayPrototype)) {
			xsIntegerValue i, l = xsToInteger(xsGet(xsArg(1), xsID_length));
			if (l > 4) l = 4;
			for (i = 0; i < l; i++) {
				xsVar(0) = xsGetAt(xsArg(1), xsInteger(i));
				if (xsTest(xsVar(0)))
					if (KprParseColor(the, xsToString(xsVar(0)), &color))
						KprStyleSetColor(self, i, &color);
			}
		}
		else {
			if (KprParseColor(the, xsToString(xsArg(1)), &color))
				KprStyleSetColor(self, -1, &color);
		}
	}
	if ((c > 2) && xsTest(xsArg(2))) {
		UInt16 alignment;
		if (KprParseHorizontalAlignment(xsToString(xsArg(2)), &alignment))
			KprStyleSetHorizontalAlignment(self, alignment);
	}
	if ((c > 3) && xsTest(xsArg(3)))
		KprStyleSetMarginLeft(self, xsToInteger(xsArg(3)));
	if ((c > 4) && xsTest(xsArg(4)))
		KprStyleSetMarginRight(self, xsToInteger(xsArg(4)));
	if ((c > 5) && xsTest(xsArg(5)))
		KprStyleSetIndentation(self, xsToInteger(xsArg(5)));
	if ((c > 6) && xsTest(xsArg(6))) {
		UInt16 alignment;
		if (KprParseVerticalAlignment(xsToString(xsArg(6)), &alignment))
			KprStyleSetVerticalAlignment(self, alignment);
	}
	if ((c > 7) && xsTest(xsArg(7)))
		KprStyleSetMarginTop(self, xsToInteger(xsArg(7)));
	if ((c > 8) && xsTest(xsArg(8)))
		KprStyleSetMarginBottom(self, xsToInteger(xsArg(8)));
	if ((c > 9) && xsTest(xsArg(9)))
		KprStyleSetLineHeight(self, xsToInteger(xsArg(9)));
	if ((c > 10) && xsTest(xsArg(10)))
		KprStyleSetLineCount(self, xsToInteger(xsArg(10)));
}

void KPR_style_get_colors(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	xsIntegerValue i;
	xsVars(1);
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	for (i = 0; i < 4; i++) {
		KprSerializeColor(the, &self->colors[i], &xsVar(0));
		xsSetAt(xsResult, xsInteger(i), xsVar(0));
	}
}

void KPR_style_get_font(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	static char* names[3] = { "bold", "italic", "underline" };
	char buffer[1024] = "";
	SInt32 i;
	if (self->flags & kprTextStyle) {
		for (i = 0; i < 3; i++) {
			if (self->textStyle & (1 << i)) {
				if (*buffer)
					FskStrCat(buffer, " ");
				FskStrCat(buffer, names[i]);
			}
		}
		if (!*buffer)
			FskStrCopy(buffer, "normal");
	}
	if (self->flags & kprTextSize) {
		if (*buffer)
			FskStrCat(buffer, " ");
		i = FskStrLen(buffer);
		FskStrNumToStr(self->textSize, buffer + i, sizeof(buffer) - i);
		if (self->textSize > 0)
			FskStrCat(buffer, "px");
		else 
			FskStrCat(buffer, "%");
	}
	if (self->flags & kprTextFont) {
		if (*buffer)
			FskStrCat(buffer, " ");
		FskStrCat(buffer, self->textFont);
	}
	xsResult = xsString(buffer);
}

void KPR_style_get_horizontalAlignment(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (self->flags & kprHorizontalAlignment) {
		switch (self->horizontalAlignment) {
		case kFskTextAlignCenter: xsResult = xsString("center"); break;
		case kFskTextAlignLeft: xsResult = xsString("left"); break;
		case kFskTextAlignRight: xsResult = xsString("right"); break;
		case kprLeftRight: xsResult = xsString("justify"); break;
		}
	}
}

void KPR_style_get_indentation(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (self->flags & kprIndentation)
		xsResult = xsInteger(self->indentation);
}

void KPR_style_get_lineCount(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (self->flags & kprLineCount)
		xsResult = xsInteger(self->lineCount);
}

void KPR_style_get_lineHeight(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (self->flags & kprLineHeight)
		xsResult = xsInteger(self->lineHeight);
}

void KPR_style_get_margins(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	if (self->flags & kprMarginLeft)
		xsNewHostProperty(xsResult, xsID_left, xsInteger(self->margins.left), xsDefault, xsDontScript);
	if (self->flags & kprMarginTop)
		xsNewHostProperty(xsResult, xsID_right, xsInteger(self->margins.top), xsDefault, xsDontScript);
	if (self->flags & kprMarginRight)
		xsNewHostProperty(xsResult, xsID_top, xsInteger(self->margins.right), xsDefault, xsDontScript);
	if (self->flags & kprMarginBottom)
		xsNewHostProperty(xsResult, xsID_bottom, xsInteger(self->margins.bottom), xsDefault, xsDontScript);
}


void KPR_style_get_size(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (self->flags & kprTextSize)
		xsResult = xsInteger(self->textSize);
}

void KPR_style_get_verticalAlignment(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (self->flags & kprVerticalAlignment) {
		switch (self->verticalAlignment) {
		case kFskTextAlignCenter: xsResult = xsString("middle"); break;
		case kFskTextAlignTop: xsResult = xsString("top"); break;
		case kFskTextAlignBottom: xsResult = xsString("bottom"); break;
		}
	}
}

#define kFskMediumFontSize 12
void KPR_style_set_font(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0))) {
		xsStringValue it = xsToString(xsArg(0));
		xsStringValue p = it;
		xsStringValue q;
		char c;
		char state = 0;
		char* textFont = NULL;
		SInt32 textSize = 0;
		UInt32 textStyle = 0;
		for (;;) {
			while ((c = *p)) {
				if (c != ' ')
					break;
				p++;
			}
			q = p;
			while ((c = *q)) {
				if (c == ' ')
					break;
				q++;
			}
			if (p == q)
				break;
			if (state == 2) {
				textFont = p;
				break;
			}
			if (state == 0) {
				if (!FskStrCompareWithLength(p, "100", q - p))
					{}
				else if (!FskStrCompareWithLength(p, "200", q - p))
					{}
				else if (!FskStrCompareWithLength(p, "300", q - p))
					{}
				else if (!FskStrCompareWithLength(p, "400", q - p))
					textStyle |= kFskTextBold;
				else if (!FskStrCompareWithLength(p, "500", q - p))
					textStyle |= kFskTextBold;
				else if (!FskStrCompareWithLength(p, "600", q - p))
					textStyle |= kFskTextBold;
				else if (!FskStrCompareWithLength(p, "700", q - p))
					textStyle |= kFskTextBold;
				else if (!FskStrCompareWithLength(p, "800", q - p))
					textStyle |= kFskTextBold;
				else if (!FskStrCompareWithLength(p, "900", q - p))
					textStyle |= kFskTextBold;
				else if (!FskStrCompareWithLength(p, "bold", q - p))
					textStyle |= kFskTextBold;
				else if (!FskStrCompareWithLength(p, "inherit", q - p))
					{}
				else if (!FskStrCompareWithLength(p, "italic", q - p))
					textStyle |= kFskTextItalic;
				else if (!FskStrCompareWithLength(p, "normal", q - p))
					textStyle = kFskTextPlain;
				else if (!FskStrCompareWithLength(p, "oblique", q - p))
					textStyle |= kFskTextItalic;
				else if (!FskStrCompareWithLength(p, "small-caps", q - p))
					{}
				else if (!FskStrCompareWithLength(p, "underline", q - p))
					textStyle |= kFskTextUnderline;
				else
					state = 1;
			}
			if (state == 1) {
				if (!FskStrCompareWithLength(p, "xx-small", q - p))
					textSize = 3 * kFskMediumFontSize / 5;
				else if (!FskStrCompareWithLength(p, "x-small", q - p))
					textSize = 3 * kFskMediumFontSize / 4;
				else if (!FskStrCompareWithLength(p, "small", q - p))
					textSize = 8 * kFskMediumFontSize / 9;
				else if (!FskStrCompareWithLength(p, "medium", q - p))
					textSize = kFskMediumFontSize;
				else if (!FskStrCompareWithLength(p, "large", q - p))
					textSize = 6 * kFskMediumFontSize / 5;
				else if (!FskStrCompareWithLength(p, "x-large", q - p))
					textSize = 3 * kFskMediumFontSize / 2;
				else if (!FskStrCompareWithLength(p, "xx-large", q - p))
					textSize = 2 * kFskMediumFontSize;
				else if (!FskStrCompareWithLength(p, "larger", q - p))
					textSize = 6 * kFskMediumFontSize / 5; // @@
				else if (!FskStrCompareWithLength(p, "smaller", q - p))
					textSize = 8 * kFskMediumFontSize / 9; // @@
				else {
					xsStringValue r = p;
					char d;
					SInt32 size = 0;
					while ((r < q) && ((d = *r)) && ('0' <= d) && (d <= '9')) {
						size = (10 * size) + (d - '0');
						r++;
					}
					if (!FskStrCompareWithLength(r, "%", q - r))
						textSize = -size;
					else if (!FskStrCompareWithLength(r, "px", q - r))
						textSize = size;
					else {
						textFont = p;
						break;
					}
				}
				state = 2;
			}
			p = q;
		}
		if (textFont)
			KprStyleSetTextFont(self, textFont);
		else
			KprStyleClearTextFont(self);
		if (textSize)
			KprStyleSetTextSize(self, textSize);
		else
			KprStyleClearTextSize(self);
		if (textStyle)
			KprStyleSetTextStyle(self, textStyle);
		else
			KprStyleClearTextStyle(self);
	}
	else {
		KprStyleClearTextFont(self);
		KprStyleClearTextSize(self);
		KprStyleClearTextStyle(self);
	}
}

void KPR_style_set_horizontalAlignment(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0))) {
		UInt16 alignment;
		if (KprParseHorizontalAlignment(xsToString(xsArg(0)), &alignment))
			KprStyleSetHorizontalAlignment(self, alignment);
	}
	else
		KprStyleClearHorizontalAlignment(self);
}

void KPR_style_set_indentation(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		KprStyleSetIndentation(self, xsToInteger(xsArg(0)));
	else
		KprStyleClearIndentation(self);
}

void KPR_style_set_lineCount(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		KprStyleSetLineCount(self, xsToInteger(xsArg(0)));
	else
		KprStyleClearLineCount(self);
}

void KPR_style_set_lineHeight(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		KprStyleSetLineHeight(self, xsToInteger(xsArg(0)));
	else
		KprStyleClearLineHeight(self);
}

void KPR_style_set_margins(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	xsIntegerValue value;
	if (xsTest(xsArg(0))) {
		xsEnterSandbox();
		if (xsFindInteger(xsArg(0), xsID_left, &value))
			KprStyleSetMarginLeft(self, value);
		else
			KprStyleClearMarginLeft(self);
		if (xsFindInteger(xsArg(0), xsID_right, &value))
			KprStyleSetMarginRight(self, value);
		else
			KprStyleClearMarginRight(self);
		if (xsFindInteger(xsArg(0), xsID_top, &value))
			KprStyleSetMarginTop(self, value);
		else
			KprStyleClearMarginTop(self);
		if (xsFindInteger(xsArg(0), xsID_bottom, &value))
			KprStyleSetMarginBottom(self, value);
		else
			KprStyleClearMarginBottom(self);
		xsLeaveSandbox();
	}
	else {
		KprStyleClearMarginLeft(self);
		KprStyleClearMarginRight(self);
		KprStyleClearMarginTop(self);
		KprStyleClearMarginBottom(self);
	}
}

void KPR_style_set_size(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		KprStyleSetTextSize(self, xsToInteger(xsArg(0)));
	else
		KprStyleClearTextSize(self);
}

void KPR_style_set_verticalAlignment(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0))) {
		UInt16 alignment;
		if (KprParseVerticalAlignment(xsToString(xsArg(0)), &alignment))
			KprStyleSetVerticalAlignment(self, alignment);
	}
	else
		KprStyleClearHorizontalAlignment(self);
}

void KPR_style_measure(xsMachine *the)
{
	KprStyle self = xsGetHostData(xsThis);
	FskRectangleRecord bounds;
	KprStyleMeasure(self, xsToString(xsArg(0)), &bounds);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(bounds.width), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(bounds.height), xsDefault, xsDontScript);
}
