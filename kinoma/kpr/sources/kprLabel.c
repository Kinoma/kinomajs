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
#define __FSKPORT_PRIV__
#include "FskText.h"
#include "FskTextConvert.h"
#include "FskUtilities.h"

#include "kprBehavior.h"
#include "kprContent.h"
#include "kprSkin.h"
#include "kprStyle.h"
#include "kprLabel.h"
#include "kprURL.h"
#include "kprShell.h"

static const char gUTF8Hidden[] = {0xE2, 0x80, 0xa2, 0};		// bullet

/* LABEL */

static void KprLabelActivated(void* it, Boolean activateIt);
static void KprLabelCascade(void* it, KprStyle style);
static void KprLabelDispose(void* it);
static void KprLabelDraw(void* it, FskPort port, FskRectangle area);
static void KprLabelMeasureHorizontally(void* it);
static void KprLabelMeasureVertically(void* it);
static void KprLabelPlaced(void* it) ;
static void KprLabelSetWindow(void* it, KprShell shell, KprStyle style);

static void KprLabelMeasureSelection(KprLabel self);
static void KprLabelMeasureText(KprLabel self, char* text, SInt32 offset, FskRectangle bounds);
static void KprLabelShowLastCallback(FskTimeCallBack callback, const FskTime time, void *param);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprLabelInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprLabel", FskInstrumentationOffset(KprLabelRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprLabelDispatchRecord = {
	"label",
	KprLabelActivated,
	KprContentAdded,
	KprLabelCascade,
	KprLabelDispose,
	KprLabelDraw,
	KprContentFitHorizontally,
	KprContentFitVertically,
	KprContentGetBitmap,
	KprContentHit,
	KprContentIdle,
	KprContentInvalidated,
	KprContentLayered,
	KprContentMark,
	KprLabelMeasureHorizontally,
	KprLabelMeasureVertically,
	KprContentPlace,
	KprLabelPlaced,
	KprContentPredict,
	KprContentReflowing,
	KprContentRemoving,
	KprLabelSetWindow,
	KprContentShowing,
	KprContentShown,
	KprContentUpdate
};

FskErr KprLabelNew(KprLabel *it, KprCoordinates coordinates, KprSkin skin, KprStyle style, char* text)
{
	FskErr err = kFskErrNone;
	KprLabel self;
	bailIfError(FskMemPtrNewClear(sizeof(KprLabelRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprLabelInstrumentation);
	self->dispatch = &KprLabelDispatchRecord;
	self->flags = kprVisible;
	KprContentInitialize((KprContent)self, coordinates, skin, style);
	KprLabelSetString(self, text);
bail:
	return err;
}

void KprLabelFlagged(KprLabel self)
{
	if (KprContentIsFocus((KprContent)self)) {
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprPositionChanged);
	}
}

void KprLabelGetSelectionBounds(KprLabel self, FskRectangle bounds)
{
	KprStyle style = self->style;
	FskRectangleSet(bounds, 0, 0, self->bounds.width, self->bounds.height);
	KprMarginsApply(&style->margins, bounds, bounds);
	FskRectangleSet(bounds, bounds->x + self->left, bounds->y, self->right - self->left, bounds->height);
}

UInt32 KprLabelHitOffset(KprLabel self, SInt32 x, SInt32 y UNUSED)
{
	KprStyle style = self->style;
	UInt32 fb, fc;
	if (x < style->margins.left)
		return 0;
	if (x > self->bounds.width - style->margins.right)
		return self->length;
	KprStyleApply(style, gShell->port);
	FskPortTextFitWidth(gShell->port, self->text, FskStrLen(self->text), x - style->margins.left, kFskTextFitFlagMidpoint, &fb, &fc);
	return fxUTF8ToUnicodeOffset(self->text, fb);
}

FskErr KprLabelInsertString(KprLabel self, char* string)
{
	return KprLabelInsertStringWithLength(self, string, FskStrLen(string));
}

FskErr KprLabelInsertStringWithLength(KprLabel self, char* text, UInt32 size)
{
	FskErr err = kFskErrNone;
	UInt32 oldSize = FskStrLen(self->text);
	SInt32 fromSize = fxUnicodeToUTF8Offset(self->text, self->from);
	SInt32 toSize = fxUnicodeToUTF8Offset(self->text, self->to);
	UInt32 newSize = oldSize - (toSize - fromSize) + size;
	char* buffer;
	bailIfError(FskMemPtrNew(newSize + 1, &buffer));
	if (fromSize > 0)
		FskMemMove(buffer, self->text, fromSize); 
	if (size > 0)
		FskMemMove(buffer + fromSize, text, size); 
	if (oldSize - toSize > 0)
		FskMemMove(buffer + fromSize + size, self->text + toSize, oldSize - toSize);
	buffer[newSize] = 0;
	FskMemPtrDispose(self->text);
	if (self->flags & kprTextHidden) {
		if ((self->from == self->length) && (self->to == self->length) && (fxUTF8ToUnicodeOffset(text, size) == 1)) {
			self->flags |= kprTextShowLast;
			if (!self->showLastCallback)
				FskTimeCallbackNew(&self->showLastCallback);
			FskTimeCallbackScheduleFuture(self->showLastCallback, 1, 0, KprLabelShowLastCallback, self);		
		}
		else {
			if (self->showLastCallback)
				FskTimeCallbackRemove(self->showLastCallback);
			self->flags &= ~kprTextShowLast;
		}
	}
	self->text = buffer;
	self->length = fxUnicodeLength(buffer);
	self->from = self->to = fxUTF8ToUnicodeOffset(buffer, fromSize + size);
bail:
	if (self->shell) {
		KprLabelMeasureSelection(self);
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprSizeChanged);
	}
	return err;
}

FskErr KprLabelSelect(KprLabel self, SInt32 selectionOffset, UInt32 selectionLength)
{
	SInt32 from = selectionOffset;
	SInt32 to = selectionOffset + selectionLength;
	if (from < 0)
		from = 0;
	else if (from > (SInt32)self->length)
		from = self->length;
	if (to < 0)
		to = 0;
	else if (to > (SInt32)self->length)
		to = self->length;
	self->from = from;
	self->to = to;
	if (self->shell) {
		KprLabelMeasureSelection(self);
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprPositionChanged);
	}
	return kFskErrNone;
}

FskErr KprLabelSetString(KprLabel self, char* string)
{
	return KprLabelSetText(self, string, FskStrLen(string));
}

FskErr KprLabelSetText(KprLabel self, char* text, UInt32 size)
{
	FskErr err = kFskErrNone;
	char* buffer;
	bailIfError(FskMemPtrNew(size + 1, &buffer));
	if (size > 0)
		FskMemMove(buffer, text, size); 
	buffer[size] = 0;
	self->length = fxUnicodeLength(text);
	self->from = self->to = 0;
	FskMemPtrDispose(self->text);
	self->text = buffer;
bail:
	if (self->shell) {
		KprLabelMeasureSelection(self);
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprSizeChanged);
	}
	return err;
}

/*  LABEL DISPATCH */

void KprLabelActivated(void* it, Boolean activateIt) 
{
	KprLabel self = it;
	if (KprContentIsFocus(it)) {
		KprContentInvalidate(it);
		if (activateIt)
			KprContentReflow(it, kprPositionChanged);
		else
			KprShellSetCaret(self->shell, self, NULL);
		KprShellKeyActivate(self->shell, activateIt);
	}
}

void KprLabelCascade(void* it, KprStyle style) 
{
	KprContentCascade(it, style);
	KprContentReflow(it, kprSizeChanged);
}

void KprLabelDispose(void* it) 
{
	KprLabel self = it;
	FskTimeCallbackDispose(self->showLastCallback);
	FskMemPtrDispose(self->text);
	KprContentDispose(it);
}

void KprLabelDraw(void* it, FskPort port, FskRectangle area UNUSED)
{
	KprLabel self = it;
	KprStyle style = self->style;
	char* text = self->text;
	FskRectangleRecord bounds;
	FskRectangleRecord selection;
	FskColorRGBARecord color;
	
	FskRectangleSet(&bounds, 0, 0, self->bounds.width, self->bounds.height);
	if (!bounds.width || !bounds.height) return;
	if (self->skin)
		KprSkinFill(self->skin, port, &bounds, self->variant, self->state, self->coordinates.horizontal, self->coordinates.vertical);
	KprMarginsApply(&style->margins, &bounds, &bounds);
	FskPortGetPenColor(port, &color);
	if ((self->flags & kprTextSelectable) && KprContentIsFocus((KprContent)self)) {
		FskRectangleSet(&selection, bounds.x + self->left, bounds.y, self->right - self->left, bounds.height);
		if (self->from == self->to) {
			if ((self->flags & kprTextEditable) && (self->shell->caretFlags & 2)) {
				FskPortSetPenColor(port, &style->colors[0]);
				FskPortRectangleFill(port, &selection);
			}
		}
		else if (self->skin) {
			double state = (self->shell->flags & kprWindowActive) ? 3 : 2;
			KprSkinFill(self->skin, port, &selection, self->variant, state, kprCenter, kprMiddle);
		}
	}
	if (KprStyleColorize(style, port, self->state)) {
		KprStyleApply(style, port);
		FskPortSetTextAlignment(port, style->horizontalAlignment, kFskTextAlignTop);
		if (style->verticalAlignment != kFskTextAlignTop) {
			UInt16 ascent, descent;
			KprStyleGetInfo(style, &ascent, &descent);
			if (style->verticalAlignment == kFskTextAlignBottom) {
				bounds.y += bounds.height - (ascent + descent);
			}
			else {
				bounds.y += (bounds.height - (ascent + descent)) >> 1;
			}
		}
		if (self->flags & kprTextHidden) {
			SInt32 c, i;
			FskPortStringGetBounds(port, gUTF8Hidden, &selection);
			bounds.width = selection.width;
			c = self->length - 1;
			for (i = 0; i < c; i++, bounds.x += selection.width)
				FskPortStringDraw(port, gUTF8Hidden, &bounds);
			if (self->flags & kprTextShowLast) {
				SInt32 offset = fxUnicodeToUTF8Offset(self->text, c);
				bounds.width = self->bounds.width - bounds.x;
				FskPortStringDraw(port, text + offset, &bounds);
			}
			else if (i <= c)
				FskPortStringDraw(port, gUTF8Hidden, &bounds);
		}
		else {
			if (!(self->flags & kprTextEditable)) {
				UInt32 textStyle;
				FskPortGetTextStyle(port, &textStyle);
				textStyle |= kFskTextTruncateEnd;
				FskPortSetTextStyle(port, textStyle);
			}
			FskPortStringDraw(port, text, &bounds);
		}
	}
	FskPortSetGraphicsMode(port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
	FskPortSetPenColor(port, &color);
}

void KprLabelMeasureHorizontally(void* it) 
{
	KprLabel self = it;
	UInt16 horizontal = self->coordinates.horizontal & kprLeftRightWidth;
	if ((horizontal != kprLeftRight) && (!(horizontal & kprWidth))) {
		KprStyle style = self->style;
		char* text = self->text;
		if (style && text) {
			FskRectangleRecord bounds;
			KprStyleApply(style, self->shell->port);
			KprLabelMeasureText(self, text, self->length, &bounds);
			self->coordinates.width = bounds.width + style->margins.left + style->margins.right;
		}
		else
			self->coordinates.width = 0;
	}
}

void KprLabelMeasureVertically(void* it) 
{
	KprLabel self = it;
	UInt16 vertical = self->coordinates.vertical & kprTopBottomHeight;
	if ((vertical != kprTopBottom) && (!(vertical & kprHeight))) {
		KprStyle style = self->style;
		char* text = self->text;
		if (style && text) {
			UInt16 ascent, descent;
			KprStyleGetInfo(style, &ascent, &descent);
			self->coordinates.height = style->margins.top + ascent + descent + style->margins.bottom;
		}
		else
			self->coordinates.height = 0;
	}
}

void KprLabelPlaced(void* it) 
{
	KprLabel self = it;
	KprContentPlaced(it);
	if (KprContentIsFocus(it)) {
		if ((self->flags & kprTextEditable) && (self->from == self->to)) {
			KprStyle style = self->style;
			FskRectangleRecord bounds;
			bounds.x = style->margins.left + self->left;
			bounds.y = style->margins.top;
			bounds.width = self->right - self->left;
			bounds.height = self->bounds.height - style->margins.top - style->margins.bottom;
			KprContentToWindowCoordinates((KprContent)self, bounds.x, bounds.y, &bounds.x, &bounds.y);
			KprShellSetCaret(self->shell, self, &bounds);
		}
		else
			KprShellSetCaret(self->shell, self, NULL);
	}
}

void KprLabelSetWindow(void* it, KprShell shell, KprStyle style) 
{
	KprLabel self = it;
	if (self->shell && !shell) {
		if (KprContentIsFocus(it) && (self->flags & kprTextEditable) && (self->from == self->to))
			KprShellSetCaret(self->shell, self, NULL);
		if (self->showLastCallback) {
			FskTimeCallbackDispose(self->showLastCallback);
			self->showLastCallback = NULL;
		}
	}
	KprContentSetWindow(it, shell, style);
	if (shell) 
		KprLabelMeasureSelection(self);
}

/*  LABEL IMPLEMENTATION */

void KprLabelMeasureSelection(KprLabel self)
{
	FskRectangleRecord bounds;
	SInt32 left, right;
	if (!(self->flags & kprTextSelectable)) return;
	FskInstrumentedItemPrintfNormal(self, "Measure Selection");
	KprStyleApply(self->style, self->shell->port);
	if (self->from) {
		KprLabelMeasureText(self, self->text, self->from, &bounds);
		left = bounds.width;
	}
	else
		left = 0;
	if (self->to) {
		KprLabelMeasureText(self, self->text, self->to, &bounds);
		right = bounds.width;
	}
	else
		right = 0;
	if (left == right) {
		left--;
		right++;
	}
	self->left = left;
	self->right = right;
 	if (KprContentIsFocus((KprContent)self))
   		KprShellKeySelect(self->shell);
}

void KprLabelMeasureText(KprLabel self, char* text, SInt32 length, FskRectangle bounds)
{
	FskInstrumentedItemPrintfNormal(self, "Measure Text");
	if (self->flags & kprTextHidden) {
		FskPortStringGetBounds(self->shell->port, gUTF8Hidden, bounds);
		if ((self->flags & kprTextShowLast) && ((SInt32)self->length == length)) {
			FskRectangleRecord last;
			SInt32 offset = fxUnicodeToUTF8Offset(self->text, length - 1);
			UInt32 size = FskStrLen(self->text);
			FskPortTextGetBounds(self->shell->port, text + offset, size - offset, &last);
			bounds->width = (bounds->width * (length - 1)) + last.width;
		}
		else
			bounds->width *= length;
	}
	else
		FskPortTextGetBounds(self->shell->port, text, fxUnicodeToUTF8Offset(text, length), bounds);
}

void KprLabelShowLastCallback(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *param)
{
	KprLabel self = param;
	if (self->flags & kprTextShowLast) {
		self->flags &= ~kprTextShowLast;
		KprLabelMeasureSelection(self);
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprSizeChanged);
	}
}

/* LABEL ECMASCRIPT */

void KPR_Label(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprSkin skin = NULL;
	KprStyle style = NULL;
	xsStringValue text = "";
	KprLabel self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	if ((c > 1) && xsTest(xsArg(1)))
		skin = kprGetHostData(xsArg(1), skin, skin);
	if ((c > 2) && xsTest(xsArg(2)))
		style = kprGetHostData(xsArg(2), style, style);
	if ((c > 3) && xsTest(xsArg(3)))
		text = xsToString(xsArg(3));
	xsThrowIfFskErr(KprLabelNew(&self, &coordinates, skin, style, text));
	kprContentConstructor(KPR_Label);
}

void KPR_label_get_editable(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprTextEditable) ? xsTrue : xsFalse;
}

void KPR_label_get_hidden(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprTextHidden) ? xsTrue : xsFalse;
}

void KPR_label_get_inputType(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	if (self->flags & kprTextEmail)
		xsResult = xsString("email");
	else if (self->flags & kprTextNumeric)
		xsResult = xsString("numeric");
	else if (self->flags & kprTextPassword)
		xsResult = xsString("password");
	else if (self->flags & kprTextPhone)
		xsResult = xsString("phone");
	else if (self->flags & kprTextURL)
		xsResult = xsString("URL");
	else
		xsResult = xsString("default");
}

void KPR_label_get_length(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->length);
}

void KPR_label_get_selectable(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprTextSelectable) ? xsTrue : xsFalse;
}

void KPR_label_get_selectionBounds(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	FskRectangleRecord bounds;
	KprLabelGetSelectionBounds(self, &bounds);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsInteger(bounds.x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsInteger(bounds.y), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(bounds.width), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(bounds.height), xsDefault, xsDontScript);
}

void KPR_label_get_selectionOffset(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->from);
}

void KPR_label_get_selectionLength(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->to - self->from);
}

void KPR_label_get_string(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	xsResult = xsString(self->text);
}

void KPR_label_set_active(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprActive;
	else
		self->flags &= ~kprActive;
	KprLabelFlagged(self);
}

void KPR_label_set_editable(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprTextEditable | kprTextSelectable;
	else
		self->flags &= ~kprTextEditable;
	KprLabelFlagged(self);
}

void KPR_label_set_hidden(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprTextHidden;
	else
		self->flags &= ~kprTextHidden;
	KprLabelFlagged(self);
}

void KPR_label_set_inputType(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	xsStringValue type = xsToString(xsArg(0));
	self->flags &= ~(kprTextEmail | kprTextNumeric | kprTextPassword | kprTextPhone | kprTextURL);
	if (!FskStrCompare(type, "email"))
		self->flags |= kprTextEmail;
	else if (!FskStrCompare(type, "numeric"))
		self->flags |= kprTextNumeric;
	else if (!FskStrCompare(type, "password"))
		self->flags |= kprTextPassword;
	else if (!FskStrCompare(type, "phone"))
		self->flags |= kprTextPhone;
	else if (!FskStrCompare(type, "URL"))
		self->flags |= kprTextURL;
	KprLabelFlagged(self);
}

void KPR_label_set_selectable(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprTextSelectable;
	else
		self->flags &= ~(kprTextEditable | kprTextSelectable);
	KprLabelFlagged(self);
}

void KPR_label_set_string(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		KprLabelSetString(self, xsToString(xsArg(0)));
	else
		KprLabelSetText(self, "", 0);
}

void KPR_label_insert(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	if ((xsToInteger(xsArgc) > 0) && xsTest(xsArg(0)))
		KprLabelInsertString(self, xsToString(xsArg(0)));
	else
		KprLabelInsertStringWithLength(self, "", 0);
}

void KPR_label_hitOffset(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	xsIntegerValue x = xsToInteger(xsArg(0));
	xsIntegerValue y = xsToInteger(xsArg(1));
	UInt32 offset = KprLabelHitOffset(self, x, y);
	xsResult = xsInteger(offset);
}

void KPR_label_select(xsMachine *the)
{
	KprLabel self = xsGetHostData(xsThis);
	xsIntegerValue from = xsToInteger(xsArg(0));
	xsIntegerValue to = xsToInteger(xsArg(1));
	KprLabelSelect(self, from, to);
}

