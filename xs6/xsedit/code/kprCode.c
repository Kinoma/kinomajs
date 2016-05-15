/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#include "pcre.h"
#define __FSKPORT_PRIV__
#include "FskText.h"
#include "FskTextConvert.h"
#include "FskUtilities.h"
#include "FskUUID.h"
#if TARGET_OS_IPHONE
	#include "FskCocoaSupportPhone.h"
#endif
#include "FskPort.h"
extern void FskPortTextGetBoundsSubpixel(FskPort port, const char *text, UInt32 textLen, double *width, double *height);

#include "FskExtensions.h"

#include "kprBehavior.h"
#include "kprContent.h"
#include "kprSkin.h"
#include "kprStyle.h"
#include "kprURL.h"
#include "kprShell.h"

#include "kprCode.h"

FskExport(FskErr) kprCode_fskLoad(FskLibrary library)
{
	KprServiceRegister(&gCodeSearch);
	KprServiceRegister(&gCodeService);
	return kFskErrNone;
}

FskExport(FskErr) kprCode_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

extern UInt32 fxUnicodeCharacter(char* string);

enum {
	teCharDone = 0,
	teCharWhiteSpace,
	teCharText,
	teCharSeparator
};
static const char *teSeparators = ".,:;-()\"\\/?<>{}[]+=-!@#%^&*~`|";

static void KprCodeActivated(void* it, Boolean activateIt);
static void KprCodeCascade(void* it, KprStyle style);
static void KprCodeDispose(void* it);
static void KprCodeDraw(void* it, FskPort port, FskRectangle area);
static void KprCodeMeasureHorizontally(void* it);
static void KprCodeMeasureVertically(void* it);
static void KprCodePlaced(void* it);
static void KprCodeSetWindow(void* it, KprShell shell, KprStyle style);

static UInt32 KprCodeClassifyCharacter(UInt32 character);
static SInt32 KprCodeFindBlockBackwards(KprCode self, KprCodeIterator iter, char d);
static SInt32 KprCodeFindBlockForwards(KprCode self, KprCodeIterator iter, char d);
static void KprCodeFindColor(KprCode self, SInt32 hit, SInt32* from, SInt32* to);
static SInt32 KprCodeFindWordBreak(KprCode self, SInt32 result, SInt32 direction);
static void KprCodeHilite(KprCode self, FskPort port, SInt32 fromColumn, SInt32 fromLine, SInt32 toColumn, SInt32 toLine, SInt32 state);
static FskErr KprCodeMeasure(KprCode self);
static void KprCodeMeasureSelection(KprCode self);
static void KprCodeMeasureStyle(KprCode self);
static FskErr KprCodeMeasureText(KprCode self);
static void KprCodeOffsetToColumnLine(KprCode self, SInt32 offset, SInt32* offsetColumn, SInt32* offsetLine);
static FskErr KprCodeSearch(KprCode self, UInt32 size);

static void KprCodeIteratorFirst(KprCode self, KprCodeIterator iter, SInt32 offset);
static void KprCodeIteratorLast(KprCode self, KprCodeIterator iter, SInt32 offset);
static Boolean KprCodeIteratorNext(KprCode self, KprCodeIterator iter);
static Boolean KprCodeIteratorPrevious(KprCode self, KprCodeIterator iter);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprCodeInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprCode", FskInstrumentationOffset(KprCodeRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprCodeDispatchRecord = {
	"code",
	KprCodeActivated,
	KprContentAdded,
	KprCodeCascade,
	KprCodeDispose,
	KprCodeDraw,
	KprContentFitHorizontally,
	KprContentFitVertically,
	KprContentGetBitmap,
	KprContentHit,
	KprContentIdle,
	KprContentInvalidated,
	KprContentLayered,
	KprContentMark,
	KprCodeMeasureHorizontally,
	KprCodeMeasureVertically,
	KprContentPlace,
	KprCodePlaced,
	KprContentPredict,
	KprContentReflowing,
	KprContentRemoving,
	KprCodeSetWindow,
	KprContentShowing,
	KprContentShown,
	KprContentUpdate
};

FskErr KprCodeNew(KprCode* it,  KprCoordinates coordinates, KprSkin skin, KprStyle style, char* string)
{
	FskErr err = kFskErrNone;
	KprCode self;
	bailIfError(FskMemPtrNewClear(sizeof(KprCodeRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprCodeInstrumentation);
	self->dispatch = &KprCodeDispatchRecord;
	self->flags = kprVisible;
	KprContentInitialize((KprContent)self, coordinates, skin, style);
	if (string)
    	KprCodeSetString(self, string);
    else
    	KprCodeSetString(self, "");
bail:
	return err;
}

void KprCodeFlagged(KprCode self)
{
	if (KprContentIsFocus((KprContent)self)) {
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprPositionChanged);
	}
}

void KprCodeGetSelectionBounds(KprCode self, FskRectangle bounds)
{
	KprStyle style = self->style;
	SInt32 line = self->toLine - self->fromLine;
	FskRectangleSet(bounds, 0, 0, self->bounds.width, self->bounds.height);
	if (line == 0) {
		bounds->x = self->fromColumn * self->columnWidth;
		bounds->width = (self->toColumn - self->fromColumn) * self->columnWidth;
		bounds->y = self->fromLine * self->lineHeight;
		bounds->height = self->lineHeight;
	}
	else {
		bounds->x = 0;
		bounds->width = self->bounds.width;
		bounds->y = self->fromLine * self->lineHeight;
		bounds->height = (line + 1) * self->lineHeight;
	}
	bounds->x += style->margins.left;
	bounds->y += style->margins.top;
}

SInt32 KprCodeHitOffset(KprCode self, double x, double y)
{
	KprStyle style = self->style;
	Boolean flag = 0;
	double left = 0;
	double right = 0;
	double top = 0;
	double bottom = self->lineHeight;
	KprCodeRun run;
	char* string = self->string;
	FskGrowableArrayGetPointerToItem(self->runs, 0, (void **)&run);
	x -= style->margins.left;
	y -= style->margins.top;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	x += self->columnWidth / 2;
	if ((top <= y) && (y < bottom))
		flag = 1;
	while (*string) {
		switch(run->kind) {
		case kprCodeLineKind:
			if (flag)
				return fxUTF8ToUnicodeOffset(self->string, string - self->string);
			left = 0;
			top = bottom;
			bottom += self->lineHeight;
			if ((top <= y) && (y < bottom))
				flag = 1;
			break;
		case kprCodeTabKind:
			if (flag) {
				right = left + (run->color * self->columnWidth);
				if ((left <= x) && (x < right))
					return fxUTF8ToUnicodeOffset(self->string, string - self->string);
				left = right;
			}
			break;
		default:
			if (flag) {
				right = left + (fxUTF8ToUnicodeOffset(string, run->count) * self->columnWidth);
				if ((left <= x) && (x < right))
					return fxUTF8ToUnicodeOffset(self->string, string - self->string) + fxUnicodeToUTF8Offset(string, (x - left) / self->columnWidth);
				left = right;
			}
			break;
		}
		string += run->count;
		run++;
	}	
	return self->length;
}

FskErr KprCodeInsertString(KprCode self, char* string)
{
	return KprCodeInsertStringWithLength(self, string, FskStrLen(string));
}

FskErr KprCodeInsertStringWithLength(KprCode self, char* string, UInt32 size)
{
	FskErr err = kFskErrNone;
	UInt32 oldSize = FskStrLen(self->string);
	SInt32 fromSize = fxUnicodeToUTF8Offset(self->string, self->from);
	SInt32 toSize = fxUnicodeToUTF8Offset(self->string, self->to);
	UInt32 newSize = oldSize - (toSize - fromSize) + size;
	char* buffer;
	bailIfError(FskMemPtrNew(newSize + 1, &buffer));
	if (fromSize > 0)
		FskMemMove(buffer, self->string, fromSize); 
	if (size > 0)
		FskMemMove(buffer + fromSize, string, size); 
	if (oldSize - toSize > 0)
		FskMemMove(buffer + fromSize + size, self->string + toSize, oldSize - toSize);
	buffer[newSize] = 0;
	FskMemPtrDispose(self->string);
	self->string = buffer;
	self->length = fxUnicodeLength(buffer);
	self->from = self->to = fxUTF8ToUnicodeOffset(buffer, fromSize + size);
bail:
	KprCodeMeasure(self);
	KprCodeSearch(self, newSize);
	if (self->shell) {
		KprCodeMeasureSelection(self);
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprSizeChanged);
	}
	return err;
}

FskErr KprCodeSearch(KprCode self, UInt32 size)
{
	FskErr err = kFskErrNone;
	KprCodeResult result;
	FskGrowableArrayDispose(self->results);
	self->results = NULL;
	if (self->pcre) {
		SInt32 capture = 0;
		SInt32 offset = 0;
		SInt32 itemCount = 0;
		SInt32 itemSize = 0;
		int* offsets;
		
		pcre_fullinfo(self->pcre, NULL, PCRE_INFO_CAPTURECOUNT, &capture);
		capture++;
		pcre_fullinfo(self->pcre, NULL, PCRE_INFO_BACKREFMAX, &offset);
		if (capture < offset)
			capture = offset;
		capture *= 3;
		itemCount = 0;
		itemSize = sizeof(KprCodeResultRecord) + (capture * sizeof(int));
		bailIfError(FskGrowableArrayNew(itemSize, itemSize, &(self->results)));
		for (;;) {
			FskGrowableArraySetItemCount(self->results, itemCount + 1);
			FskGrowableArrayGetPointerToItem(self->results, itemCount, (void **)&result);
			offsets = (int*)(((char*)result) + sizeof(KprCodeResultRecord));
			result->count = pcre_exec(self->pcre, NULL, self->string, size, offset, 0, offsets, capture);
			if (result->count <= 0) {
				break;
			}
			offset = offsets[1];
			result->from = offsets[0];
			result->to = offset;
			KprCodeOffsetToColumnLine(self, result->from, &result->fromColumn, &result->fromLine);
			KprCodeOffsetToColumnLine(self, result->to, &result->toColumn, &result->toLine);
			result->from = fxUTF8ToUnicodeOffset(self->string, result->from);
			result->to = fxUTF8ToUnicodeOffset(self->string, result->to);
			itemCount++;
		}
	}
	else {
		bailIfError(FskGrowableArrayNew(sizeof(KprCodeResultRecord), sizeof(KprCodeResultRecord), &(self->results)));
		FskGrowableArraySetItemCount(self->results, 1);
		FskGrowableArrayGetPointerToItem(self->results, 0, (void **)&result);
		result->count = -1;
	}
bail:
	return err;
}

FskErr KprCodeSelect(KprCode self, SInt32 selectionOffset, UInt32 selectionLength)
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
		KprCodeMeasureSelection(self);
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprPositionChanged);
	}
	return kFskErrNone;
}

FskErr KprCodeSetString(KprCode self, char* string)
{
	return KprCodeSetStringWithLength(self, string, FskStrLen(string));
}

FskErr KprCodeSetStringWithLength(KprCode self, char* string, UInt32 size)
{
	FskErr err = kFskErrNone;
	char* buffer;
	bailIfError(FskMemPtrNew(size + 1, &buffer));
	if (size > 0)
		FskMemMove(buffer, string, size); 
	buffer[size] = 0;
	self->length = fxUnicodeLength(string);
	self->from = self->to = 0;
	FskMemPtrDispose(self->string);
	self->string = buffer;
bail:
	KprCodeMeasure(self);
	KprCodeSearch(self, size);
	if (self->shell) {
		KprCodeMeasureSelection(self);
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprSizeChanged);
	}
	return err;
}

/* DISPATCH */

void KprCodeActivated(void* it, Boolean activateIt) 
{
	KprCode self = it;
	if (KprContentIsFocus(it)) {
		KprContentInvalidate(it);
		if (activateIt)
			KprContentReflow(it, kprPositionChanged);
		else
			KprShellSetCaret(self->shell, self, NULL);
		KprShellKeyActivate(self->shell, activateIt);
	}
}

void KprCodeCascade(void* it, KprStyle style) 
{
	KprCode self = it;
	KprContentCascade(it, style);
	KprCodeMeasureStyle(self);
	KprContentReflow(it, kprSizeChanged);
}

void KprCodeDispose(void* it) 
{
	KprCode self = it;
	FskMemPtrDispose(self->string);
	FskGrowableArrayDispose(self->runs);
	FskGrowableArrayDispose(self->results);
	KprContentDispose(it);
}

void KprCodeHilite(KprCode self, FskPort port, SInt32 fromColumn, SInt32 fromLine, SInt32 toColumn, SInt32 toLine, SInt32 state)
{
	KprStyle style = self->style;
	FskRectangleRecord bounds;
	SInt32 line = toLine - fromLine;
	if (line == 0) {
		bounds.x = style->margins.left + (fromColumn * self->columnWidth);
		bounds.width = (toColumn - fromColumn) * self->columnWidth;
		bounds.y = style->margins.top + (fromLine * self->lineHeight);
		bounds.height = self->lineHeight;
		KprSkinFill(self->skin, port, &bounds, self->variant, state, kprCenter, kprMiddle);
	}
	else {
		bounds.x = style->margins.left + (fromColumn * self->columnWidth);
		bounds.width = 0x7FFF;
		bounds.y = style->margins.top + (fromLine * self->lineHeight);
		bounds.height = self->lineHeight;
		KprSkinFill(self->skin, port, &bounds, self->variant, state, kprCenter, kprMiddle);
		if (line > 1) {
			bounds.x = style->margins.left;
			bounds.width = 0x7FFF;
			bounds.y = style->margins.top + ((fromLine + 1) * self->lineHeight);
			bounds.height = (line - 1) * self->lineHeight;
			KprSkinFill(self->skin, port, &bounds, self->variant, state, kprCenter, kprMiddle);
		}
		bounds.x = style->margins.left;
		bounds.width = toColumn * self->columnWidth;
		bounds.y = style->margins.top + (toLine * self->lineHeight);
		bounds.height = self->lineHeight;
		KprSkinFill(self->skin, port, &bounds, self->variant, state, kprCenter, kprMiddle);
	}
}

void KprCodeDraw(void* it, FskPort port, FskRectangle area UNUSED)
{
	KprCode self = it;
	FskRectangleRecord bounds;
	FskColorRGBARecord color;
	KprStyle style = self->style;
	KprCodeResult result;
	xsIntegerValue limit = 0;
	
	FskPortGetPenColor(port, &color);
	FskGrowableArrayGetPointerToItem(self->results, 0, (void **)&result);
	while (result->count >= 0) {
		if (limit > 256)
			break;
		KprCodeHilite(self, port, result->fromColumn, result->fromLine, result->toColumn, result->toLine, 0);
		limit++;
		FskGrowableArrayGetPointerToItem(self->results, limit, (void **)&result);
	}
	if ((self->flags & kprTextSelectable) && self->skin) {
		if (self->from < self->to)
			KprCodeHilite(self, port, self->fromColumn, self->fromLine, self->toColumn, self->toLine, 3);
		if ((self->flags & kprTextEditable) && KprContentIsFocus((KprContent)self)) {
			if (self->shell->caretFlags & 2) {
				bounds.x = (self->toColumn * self->columnWidth) - 1;
				bounds.width = 2;
				bounds.y = (self->toLine * self->lineHeight) - 1;
				bounds.height = self->lineHeight + 2;
				bounds.x += style->margins.left;
				bounds.y += style->margins.top;
				FskPortSetPenColor(port, &style->colors[0]);
				FskPortRectangleFill(port, &bounds);
			}
		}
	}	
	if (!KprStyleColorize(style, port, self->state))
		return;
	style->textStyle |= kFskTextCode;
	KprStyleApply(style, port);
	FskPortSetTextAlignment(port, kFskTextAlignLeft, kFskTextAlignTop);
	{
		double clipTop = area->y;
		double clipBottom = clipTop + area->height;
		Boolean flag = (clipTop <= (style->margins.top + self->lineHeight)) ? 1 : 0;
		KprCodeRun run;
		char* string = self->string;
		double x = style->margins.left;
		double y = style->margins.top;
		double width = 0x7FFF;
		double height = self->lineHeight;
		FskGrowableArrayGetPointerToItem(self->runs, 0, (void **)&run);
		while (*string) {
			switch(run->kind) {
			case kprCodeLineKind:
				x = style->margins.left;
				y += self->lineHeight;
				if (clipBottom <= y)
					return;
				if (clipTop <= (y + self->lineHeight))
					flag = 1;
				break;
			case kprCodeTabKind:
				if (flag)
					x += run->color * self->columnWidth;
				break;
			default:
				if (flag && run->count) {
					FskPortSetPenColor(port, &style->colors[run->color]);
				#if TARGET_OS_IPHONE || TARGET_OS_MAC
					FskPortTextDrawSubpixel(port, string, run->count, x, y, width, height);
				#else
					FskRectangleSet(&bounds, x, y, width, height);
					FskPortTextDraw(port, string, run->count, &bounds);
				#endif
					x += fxUTF8ToUnicodeOffset(string, run->count) * self->columnWidth;
				}
				break;
			}
			string += run->count;
			run++;
		}	
	}	
}

void KprCodeMeasureHorizontally(void* it) 
{
	KprCode self = it;
	KprStyle style = self->style;
	UInt16 horizontal = self->coordinates.horizontal & kprLeftRightWidth;
	KprCodeMeasureStyle(self);
	if ((horizontal != kprLeftRight) && (!(horizontal & kprWidth))) {
		self->coordinates.width = style->margins.left + (self->columnCount * self->columnWidth) + style->margins.right;
	}
}

void KprCodeMeasureVertically(void* it) 
{
	KprCode self = it;
	KprStyle style = self->style;
	UInt16 vertical = self->coordinates.vertical & kprTopBottomHeight;
	if ((vertical != kprTopBottom) && (!(vertical & kprHeight))) {
		self->coordinates.height = style->margins.top + (self->lineCount * self->lineHeight) + style->margins.bottom;
	}
}

void KprCodePlaced(void* it) 
{
	KprCode self = it;
	KprStyle style = self->style;
	KprContentPlaced(it);
	if (KprContentIsFocus(it)) {
		if (self->flags & kprTextEditable) {
			FskRectangleRecord bounds;
			bounds.x = (self->toColumn * self->columnWidth) - 1;
			bounds.y = (self->toLine * self->lineHeight) - 1;
			bounds.width = 2;
			bounds.height = self->lineHeight + 2;
			bounds.x += style->margins.left;
			bounds.y += style->margins.top;
			KprContentToWindowCoordinates((KprContent)self, bounds.x, bounds.y, &bounds.x, &bounds.y);
			KprShellSetCaret(self->shell, self, &bounds);
		}
		else
			KprShellSetCaret(self->shell, self, NULL);
	}
}

void KprCodeSetWindow(void* it, KprShell shell, KprStyle style) 
{
	KprCode self = it;
	if (self->shell && !shell) {
		if (KprContentIsFocus(it) && (self->flags & kprTextEditable))
			KprShellSetCaret(self->shell, self, NULL);
	}
	KprContentSetWindow(it, shell, style);
}

/* IMPLEMENTATION */

UInt32 KprCodeClassifyCharacter(UInt32 character)
{
	const char *c;

	if (0 == character)
		return teCharDone;

	if ((' ' == character) || (9 == character) || (10 == character) || (13 == character) || (12288 == character))		// 12288: double-byte space
		return teCharWhiteSpace;

	c = teSeparators;
	while (*c)
		if (*c++ == (char)character)
			return teCharSeparator;

	return teCharText;
}

SInt32 KprCodeFindBlockBackwards(KprCode self, KprCodeIterator iter, char d)
{
	while (KprCodeIteratorPrevious(self, iter)) {
		if (iter->color == 0) {
			char c = iter->character;
			SInt32 result = iter->offset;
			if (c == d)
				return result;
			if (c == ')')
				result = KprCodeFindBlockBackwards(self, iter, '(');
			else if (c == ']')
				result = KprCodeFindBlockBackwards(self, iter, '[');
			else if (c == '}')
				result = KprCodeFindBlockBackwards(self, iter, '{');
			if (result < 0)
				return result;
		}
	}
	return -1;
}

SInt32 KprCodeFindBlockForwards(KprCode self, KprCodeIterator iter, char d)
{
	while (KprCodeIteratorNext(self, iter)) {
		if (iter->color == 0) {
			char c = iter->character;
			SInt32 result = iter->offset;
			if (c == d)
				return result;
			if (c == '(')
				result = KprCodeFindBlockForwards(self, iter, ')');
			else if (c == '[')
				result = KprCodeFindBlockForwards(self, iter, ']');
			else if (c == '{')
				result = KprCodeFindBlockForwards(self, iter, '}');
			if (result < 0)
				return result;
		}
	}
	return -1;
}

void KprCodeFindColor(KprCode self, SInt32 hit, SInt32* from, SInt32* to)
{
	SInt16 color = 0;
	SInt32 start = 0;
	SInt32 stop = 0;
	KprCodeRun run;
	char* string = self->string;
	FskGrowableArrayGetPointerToItem(self->runs, 0, (void **)&run);
	while (*string) {
		if (run->kind == kprCodeColorKind) {
			if (run->color != color) {
				color = run->color;
				start = stop;
			}
		}
		stop += run->count;
		if ((start <= hit) && (hit < stop))
			break;
		string += run->count;
		run++;
	}	
	*from = start;
	*to = stop;
}

SInt32 KprCodeFindWordBreak(KprCode self, SInt32 result, SInt32 direction)
{
	if ((direction < 0) && (result == 0))
		result = 0;
	else if ((direction > 0) && (result >= (SInt32)self->length))
		result = self->length;
	else {
		char *string = self->string;
		UInt32 classification;
		if (direction > 0)
			string += fxUnicodeToUTF8Offset(string, result);
		else
			string += fxUnicodeToUTF8Offset(string, result - 1);
		classification = KprCodeClassifyCharacter(fxUnicodeCharacter(string));
		while (KprCodeClassifyCharacter(fxUnicodeCharacter(string)) == classification) {
			SInt32 advance = FskTextUTF8Advance((unsigned char *)string, 0, direction);
			if (0 == advance)
				break;
			string += advance;
			result += direction;
		}
	}
	return result;
}

FskErr KprCodeMeasure(KprCode self) 
{
	if (self->type == 1)
		return KprCodeMeasureJS(self);
	if (self->type == 2)
		return KprCodeMeasureJSON(self);
	if (self->type == 3)
		return KprCodeMeasureXML(self);
	if (self->type == 4)
		return KprCodeMeasureMarkdown(self);
	return KprCodeMeasureText(self);
}
	
void KprCodeMeasureSelection(KprCode self) 
{
	SInt32 offset = fxUnicodeToUTF8Offset(self->string, self->from);
	KprCodeOffsetToColumnLine(self, offset, &self->fromColumn, &self->fromLine);
	if (self->to == self->from) {
		self->toColumn = self->fromColumn;
		self->toLine = self->fromLine;
	}
	else {
		offset = fxUnicodeToUTF8Offset(self->string, self->to);
		KprCodeOffsetToColumnLine(self, offset, &self->toColumn, &self->toLine);
	}
}

void KprCodeMeasureStyle(KprCode self) 
{
#if TARGET_OS_IPHONE || TARGET_OS_MAC
	KprShellAdjust(gShell);
	KprStyleApply(self->style, gShell->port);
	FskPortTextGetBoundsSubpixel(gShell->port, " ", 1, &self->columnWidth, &self->lineHeight);
	self->lineHeight = ceil(self->lineHeight);
	if (self->lineHeight < 16)
		self->lineHeight = 16;
#else
	FskRectangleRecord bounds;
	KprShellAdjust(gShell);
	self->style->textStyle |= kFskTextCode;
	KprStyleApply(self->style, gShell->port);
	FskPortTextGetBounds(gShell->port, " ", 1, &bounds);
	self->columnWidth = bounds.width;
	self->lineHeight = 16;
#endif
}

FskErr KprCodeMeasureText(KprCode self) 
{
	FskErr err = kFskErrNone;
	xsStringValue p = self->string;
	SInt32 input = 0;
	SInt32 output = 0;
	SInt32 lineCount = 0;
	SInt32 columnCount = 0;
	SInt32 columnIndex = 0;
	SInt32 tabCount = 4;
	SInt32 tabIndex;
	KprCodeRunRecord runRecord;
	
	FskGrowableArrayDispose(self->runs);
	self->runs = NULL;
	bailIfError(FskGrowableArrayNew(sizeof(KprCodeRunRecord), sizeof(KprCodeRunRecord) * 1024, &(self->runs)));
	while (*p) {
		SInt32 advance = FskTextUTF8Advance((unsigned char*)p, 0, 1);	
		if (advance == 1) {
			if (*p == 9) {
				if (input > output) {
					runRecord.kind = kprCodeColorKind;
					runRecord.color = 0;
					runRecord.count = input - output;
					bailIfError(FskGrowableArrayAppendItem(self->runs, (void *)&runRecord));
				}
				tabIndex = columnIndex;
				columnIndex += tabCount;
				columnIndex -= columnIndex % tabCount;
				runRecord.kind = kprCodeTabKind;
				runRecord.color = columnIndex - tabIndex;
				runRecord.count = 1;
				bailIfError(FskGrowableArrayAppendItem(self->runs, (void *)&runRecord));
				output = input + advance;
			}
			else if ((*p == 10) || (*p == 13)) {
				if (input > output) {
					runRecord.kind = kprCodeColorKind;
					runRecord.color = 0;
					runRecord.count = input - output;
					bailIfError(FskGrowableArrayAppendItem(self->runs, (void *)&runRecord));
				}
				if ((*p == 13) && (*(p + 1) == 10))
					advance++;
				runRecord.kind = kprCodeLineKind;
				runRecord.color = 0;
				runRecord.count = advance;
				bailIfError(FskGrowableArrayAppendItem(self->runs, (void *)&runRecord));
				if (columnCount < columnIndex)
					columnCount = columnIndex;
				columnIndex = 0;
				lineCount++;
				output = input + advance;
			}
			else
				columnIndex++;
		}
		else
			columnIndex++;
		p += advance;
		input += advance;
	}
	if (input > output) {
		runRecord.kind = kprCodeColorKind;
		runRecord.color = 0;
		runRecord.count = input - output;
		bailIfError(FskGrowableArrayAppendItem(self->runs, (void *)&runRecord));
	}
	if (columnCount < columnIndex)
		columnCount = columnIndex;
	lineCount++;
	self->columnCount = columnCount;
	self->lineCount = lineCount;
bail:
	return err;
}

void KprCodeOffsetToColumnLine(KprCode self, SInt32 offset, SInt32* offsetColumn, SInt32* offsetLine) 
{
	SInt32 lineIndex = 0;
	SInt32 columnIndex = 0;
	KprCodeRun run;
	char* string = self->string;
	FskGrowableArrayGetPointerToItem(self->runs, 0, (void **)&run);
	while (*string) {
		switch(run->kind) {
		case kprCodeLineKind:
			if (offset == 0) 
				goto bail;
			lineIndex++;
			columnIndex = 0;
			break;
		case kprCodeTabKind:
			if (offset == 0) 
				goto bail;
			columnIndex += run->color;
			break;
		default:
			if (offset < run->count) {
				columnIndex += fxUTF8ToUnicodeOffset(string, offset);
				goto bail;
			}
			columnIndex += fxUTF8ToUnicodeOffset(string, run->count);
			break;
		}
		offset -= run->count;
		string += run->count;
		run++;
	}	
bail:
	*offsetColumn = columnIndex;
	*offsetLine = lineIndex;
}


void KprCodeIteratorFirst(KprCode self, KprCodeIterator iter, SInt32 offset)
{
	FskGrowableArrayGetPointerToItem(self->runs, 0, (void **)&(iter->run));
	iter->offset = 0;
	if (iter->run->kind == kprCodeLineKind)
		iter->limit = iter->offset + 1;
	else if (iter->run->kind == kprCodeTabKind)
		iter->limit = iter->offset + 1;
	else {
		iter->color = iter->run->color;
		iter->limit = iter->offset + iter->run->count;
	}
	while (KprCodeIteratorNext(self, iter)) {
		if (iter->offset == offset)
			break;
	}
}

void KprCodeIteratorLast(KprCode self, KprCodeIterator iter, SInt32 offset)
{
	UInt32 count = FskGrowableArrayGetItemCount(self->runs);
	FskGrowableArrayGetPointerToItem(self->runs, count - 1, (void **)&(iter->run));
	iter->offset = FskStrLen(self->string);
	if (iter->run->kind == kprCodeLineKind)
		iter->limit = iter->offset - 1;
	else if (iter->run->kind == kprCodeTabKind)
		iter->limit = iter->offset - 1;
	else {
		iter->color = iter->run->color;
		iter->limit = iter->offset - iter->run->count;
	}
	while (KprCodeIteratorPrevious(self, iter)) {
		if (iter->offset == offset)
			break;
	}
}


Boolean KprCodeIteratorNext(KprCode self, KprCodeIterator iter)
{
	iter->offset++;
	iter->character = *(self->string + iter->offset);
	if (!iter->character)
		return 0;
	if (iter->offset >= iter->limit) {
		iter->run++;
		if (iter->run->kind == kprCodeColorKind)
			iter->color = iter->run->color;
		iter->limit = iter->offset + iter->run->count;
	}
	return 1;
}

Boolean KprCodeIteratorPrevious(KprCode self, KprCodeIterator iter)
{
	if (!iter->offset)
		return 0;
	if (iter->offset == iter->limit) {
		iter->run--;
		if (iter->run->kind == kprCodeColorKind)
			iter->color = iter->run->color;
		iter->limit = iter->offset - iter->run->count;
	}
	iter->offset--;
	iter->character = *(self->string + iter->offset);
	return 1;
}

/* ECMASCRIPT */

extern void KPR_BehaviorDictionary(xsMachine* the, xsSlot* slot, KprContent self, xsSlot* data, xsSlot* context); 
extern void KPR_ContentDictionary(xsMachine* the, xsSlot* slot, KprContent self); 
static void KPR_CodeDictionary(xsMachine* the, xsSlot* slot, KprCode self);

void KPR_CodeDictionary(xsMachine* the, xsSlot* slot, KprCode self) 
{
	xsBooleanValue boolean;
	xsStringValue string;
	if (xsFindBoolean(*slot, xsID_editable, &boolean)) {
		if (boolean)
			self->flags |= kprTextEditable | kprTextSelectable;
		else
			self->flags &= ~kprTextEditable;
	}
	if (xsFindBoolean(*slot, xsID_hidden, &boolean)) {
		if (boolean)
			self->flags |= kprTextHidden;
		else
			self->flags &= ~kprTextHidden;
	}
	if (xsFindBoolean(*slot, xsID_selectable, &boolean)) {
		if (boolean)
			self->flags |= kprTextSelectable;
		else
			self->flags &= ~(kprTextEditable | kprTextSelectable);
	}
	if (xsFindString(*slot, xsID_string, &string))
		KprCodeSetString(self, string);
}

void KPR_Code(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprCode self;
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (!xsIsInstanceOf(xsThis, xsResult))
		xsThis = xsNewInstanceOf(xsResult);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	xsThrowIfFskErr(KprCodeNew(&self, &coordinates, NULL, NULL, NULL));
	kprContentConstructor(KPR_Code);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_CodeDictionary(the, &xsArg(1), self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_code_get_columnCount(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->columnCount);
}

void KPR_code_get_columnWidth(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->columnWidth);
}

void KPR_code_get_editable(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprTextEditable) ? xsTrue : xsFalse;
}

void KPR_code_get_length(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->length);
}

void KPR_code_get_lineCount(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->lineCount);
}

void KPR_code_get_lineHeight(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->lineHeight);
}

void KPR_code_get_selectable(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprTextSelectable) ? xsTrue : xsFalse;
}

void KPR_code_get_resultCount(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsResult = xsInteger(FskGrowableArrayGetItemCount(self->results) - 1);
}

void KPR_code_get_selectionBounds(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	FskRectangleRecord bounds;
	KprCodeGetSelectionBounds(self, &bounds);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsInteger(bounds.x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsInteger(bounds.y), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(bounds.width), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(bounds.height), xsDefault, xsDontScript);
}

void KPR_code_get_selectionLength(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->to - self->from);
}

void KPR_code_get_selectionOffset(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->from);
}

void KPR_code_get_selectionString(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	char *s = self->string;
	char *p = s + fxUnicodeToUTF8Offset(s, self->from);
	char *q = s + fxUnicodeToUTF8Offset(s, self->to);
	xsResult = xsStringBuffer(p, q - p);
}

void KPR_code_get_string(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsResult = xsString(self->string);
}

void KPR_code_get_type(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	switch (self->type) {
		case 1:
			xsResult = xsString("javascript");
			break;
		case 2:
			xsResult = xsString("json");
			break;
		case 3:
			xsResult = xsString("xml");
			break;
		case 4:
			xsResult = xsString("markdown");
			break;
		default:
			xsResult = xsString("text");
			break;
	}
}

void KPR_code_set_active(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprActive;
	else
		self->flags &= ~(kprActive | kprTextEditable | kprTextSelectable);
	KprCodeFlagged(self);
}

void KPR_code_set_editable(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprActive | kprTextEditable | kprTextSelectable;
	else
		self->flags &= ~kprTextEditable;
	KprCodeFlagged(self);
}

void KPR_code_set_selectable(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprActive | kprTextSelectable;
	else
		self->flags &= ~(kprTextEditable | kprTextSelectable);
	KprCodeFlagged(self);
}

void KPR_code_set_string(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		KprCodeSetString(self, xsToString(xsArg(0)));
	else
		KprCodeSetStringWithLength(self, "", 0);
}

void KPR_code_set_type(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	xsStringValue string = xsToString(xsArg(0));
	if (!FskStrCompare(string, "javascript"))
		self->type = 1;
	else if (!FskStrCompare(string, "json"))
		self->type = 2;
	else if (!FskStrCompare(string, "xml"))
		self->type = 3;
	else if (!FskStrCompare(string, "markdown"))
		self->type = 4;
	else
		self->type = 0;
	KprCodeMeasure(self);
}

void KPR_code_extract(xsMachine *the)
{
	KprCode self = kprGetHostData(xsThis, this, code);
	SInt32 from = xsToInteger(xsArg(0));
	SInt32 to = from + xsToInteger(xsArg(1));
	if (from < 0)
		from = 0;
	else if (from > (SInt32)self->length)
		from = self->length;
	if (to < 0)
		to = 0;
	else if (to > (SInt32)self->length)
		to = self->length;
	xsResult = xsStringBuffer(self->string + from, to - from);
}

void KPR_code_find(xsMachine *the)
{
	KprCode self = kprGetHostData(xsThis, this, code);
	UInt32 argc = xsToInteger(xsArgc);
	xsStringValue pattern = NULL;
	xsBooleanValue caseless = 0;
	KprCodeResult result;
    UInt32 size;
	if (argc > 0)
		pattern = xsToString(xsArg(0));
	if (argc > 1)
		caseless = xsToBoolean(xsArg(1));
	
	self->pcreCount = 0;
	FskMemPtrDisposeAt(&(self->pcreOffsets));
	if (self->pcre) {
		pcre_free(self->pcre);
		self->pcre = NULL;
	}	
	if (pattern && pattern[0]) {
		char* aMessage;
		xsIntegerValue options = PCRE_UTF8; 
		xsIntegerValue offset;
		if (caseless)
			options += PCRE_CASELESS;
		self->pcre = pcre_compile(pattern, options, (const char **)(void*)&aMessage, (int*)(void*)&offset, NULL); 
	}
	KprCodeSearch(self, FskStrLen(self->string));
	FskGrowableArrayGetPointerToItem(self->results, 0, (void **)&result);
    size = FskGrowableArrayGetItemSize(self->results);
	if (result->count >= 0) {
		xsResult = xsTrue;
		while (result->count >= 0) {
			if (self->from <= result->from) {
				KprCodeSelect(self, result->from, result->to - result->from);
				return;
			}
			result = (KprCodeResult)(((char*)result) + size);
		}
		FskGrowableArrayGetPointerToItem(self->results, 0, (void **)&result);
		KprCodeSelect(self, result->from, result->to - result->from);
	}
	else {
		xsResult = xsFalse;
		KprCodeSelect(self, self->from, 0);
	}
}

void KPR_code_findAgain(xsMachine *the)
{
	KprCode self = kprGetHostData(xsThis, this, code);
	UInt32 argc = xsToInteger(xsArgc);
	xsIntegerValue direction = 1;
	SInt32 count, index;
	KprCodeResult result;
    UInt32 size;
	if (argc > 0)
		direction = xsToInteger(xsArg(0));
	count = (SInt32)FskGrowableArrayGetItemCount(self->results) - 1;
	if (count) {
		xsResult = xsTrue;
		if (direction > 0) {
			FskGrowableArrayGetPointerToItem(self->results, 0, (void **)&result);
    		size = FskGrowableArrayGetItemSize(self->results);
			for (index = 0; index < count; index++) {
				if (self->to <= result->from) {
					KprCodeSelect(self, result->from, result->to - result->from);
					return;
				}
				result = (KprCodeResult)(((char*)result) + size);
			}
			FskGrowableArrayGetPointerToItem(self->results, 0, (void **)&result);
			KprCodeSelect(self, result->from, result->to - result->from);
		}
		else if (direction < 0) {
			count--;
			FskGrowableArrayGetPointerToItem(self->results, count, (void **)&result);
    		size = FskGrowableArrayGetItemSize(self->results);
			for (index = count; index >= 0; index--) {
				if (self->from >= result->to) {
					KprCodeSelect(self, result->from, result->to - result->from);
					return;
				}
				result = (KprCodeResult)(((char*)result) - size);
			}
			FskGrowableArrayGetPointerToItem(self->results, count, (void **)&result);
			KprCodeSelect(self, result->from, result->to - result->from);
		}
	}
	else
		xsResult = xsFalse;
}

void KPR_code_findBlock(xsMachine *the)
{
	KprCode self = kprGetHostData(xsThis, this, code);
	KprCodeIteratorRecord iteratorRecord;
	KprCodeIterator iter = &iteratorRecord;
	SInt32 offset = xsToInteger(xsArg(0));
	SInt32 from = -1, to = -1;
	char c = self->string[offset], d = 0;
	if ((c == '"') || (c == '\'')) {
		KprCodeFindColor(self, offset, &from, &to);
		goto bail;
	}
	if (c == '(') d = ')';
	else if (c == '[') d = ']';
	else if (c == '{') d = '}';
	if (d) {
		KprCodeIteratorFirst(self, iter, offset);
		if (iter->color == 0) {
			to = KprCodeFindBlockForwards(self, iter, d);
			if (to >= 0) {
				from = offset;
				to++;
			}
		}
		goto bail;
	}
	if (c == ')') d = '(';
	else if (c == ']') d = '[';
	else if (c == '}') d = '{';
	if (d) {
		KprCodeIteratorLast(self, iter, offset);
		if (iter->color == 0) {
			from = KprCodeFindBlockBackwards(self, iter, d);
			if (from >= 0)
				to = offset + 1;
		}
		goto bail;
	}
bail:
	if (from < to) {
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID("from"), xsInteger(from), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID("to"), xsInteger(to), xsDefault, xsDontScript);
	}
}

void KPR_code_findLineBreak(xsMachine *the)
{
	KprCode self = kprGetHostData(xsThis, this, code);
	UInt32 argc = xsToInteger(xsArgc);
	SInt32 result = xsToInteger(xsArg(0));
	SInt32 direction = (argc > 1) ? (xsToBoolean(xsArg(1)) ? +1 : -1) : + 1;
	if ((direction < 0) && (result == 0))
		result = 0;
	else if ((direction > 0) && (result >= (SInt32)self->length))
		result = self->length;
	else {
		char *s = self->string;
		char *p = s + fxUnicodeToUTF8Offset(s, result);
		char c;
		if (direction > 0) {
			result = self->length;
			while ((c = *p)) {
				if ((c == 10) || (c == 13)) {
					result = fxUTF8ToUnicodeOffset(s, p + 1 - s);
					break;
				}
				p++;
			}
		}
		else {
			p--;
			result = 0;
			while (p >= s) {
				c = *p;
				if ((c == 10) || (c == 13)) {
					result = fxUTF8ToUnicodeOffset(s, p + 1 - s);
					break;
				}
				p--;
			}
		}
	}
	xsResult = xsInteger(result);
}

void KPR_code_findWordBreak(xsMachine *the)
{
	KprCode self = kprGetHostData(xsThis, this, code);
	UInt32 argc = xsToInteger(xsArgc);
	SInt32 offset = xsToInteger(xsArg(0));
	SInt32 direction = (argc > 1) ? (xsToBoolean(xsArg(1)) ? +1 : -1) : + 1;
	xsResult = xsInteger(KprCodeFindWordBreak(self, offset, direction));
}

void KPR_code_hitOffset(xsMachine *the)
{
	KprCode self = kprGetHostData(xsThis, this, code);
	UInt32 offset = KprCodeHitOffset(self, xsToInteger(xsArg(0)), xsToInteger(xsArg(1)));
	xsResult = xsInteger(offset);
}

void KPR_code_insert(xsMachine *the)
{
	KprCode self = kprGetHostData(xsThis, this, code);
	if ((xsToInteger(xsArgc) > 0) && xsTest(xsArg(0)))
		KprCodeInsertString(self, xsToString(xsArg(0)));
	else
		KprCodeInsertStringWithLength(self, "", 0);
}

void KPR_code_isSpace(xsMachine *the)
{
	KprCode self = kprGetHostData(xsThis, this, code);
	SInt32 offset = xsToInteger(xsArg(0));
	char *string = self->string;
	UInt32 classification;
	string += fxUnicodeToUTF8Offset(string, offset);
	classification = KprCodeClassifyCharacter(fxUnicodeCharacter(string));
	xsResult = xsBoolean(classification == teCharWhiteSpace);
}

void KPR_code_locate(xsMachine *the)
{
	KprCode self = kprGetHostData(xsThis, this, code);
	KprStyle style = self->style;
	SInt32 offset = fxUnicodeToUTF8Offset(self->string, xsToInteger(xsArg(0)));
	SInt32 column, line;
	KprCodeOffsetToColumnLine(self, offset, &column, &line);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsInteger(style->margins.left + (column * self->columnWidth)), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsInteger(style->margins.top + (line * self->lineHeight)), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(self->columnWidth), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(self->lineHeight), xsDefault, xsDontScript);
}

static FskErr KPR_code_replace_add(xsMachine *the, char* src, SInt32 srcSize, char** dst, SInt32* dstSize, SInt32 from, SInt32 to);
static FskErr KPR_code_replace_aux(xsMachine *the, char* src, SInt32 srcSize, char** dst, SInt32* dstSize, SInt32 count, int* offsets, xsStringValue replacement);

void KPR_code_replace(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprCode self = kprGetHostData(xsThis, this, code);
	xsStringValue replacement = xsToString(xsArg(0));
	KprCodeResult result;
    UInt32 resultSize;
    xsStringValue src = self->string;
    SInt32 srcOffset = 0;
   	SInt32 srcSize = FskStrLen(src);
    xsStringValue dst = NULL;
    SInt32 dstSize = 0;
    int* offsets;
    SInt32 from = self->from;
    SInt32 to = self->to;

	FskGrowableArrayGetPointerToItem(self->results, 0, (void **)&result);
	if (result->count < 0)
		return;
    resultSize = FskGrowableArrayGetItemSize(self->results);
	while (result->count >= 0) {
		if (self->from <= result->from)
			break;
		result = (KprCodeResult)(((char*)result) + resultSize);
	}
	if (result->count < 0)
		FskGrowableArrayGetPointerToItem(self->results, 0, (void **)&result);
	bailIfError(FskMemPtrNew(1, &dst));
	dst[dstSize] = 0;
	offsets = (int*)(((char*)result) + sizeof(KprCodeResultRecord));
	bailIfError(KPR_code_replace_add(the, src, srcSize, &dst, &dstSize, srcOffset, offsets[0]));
	srcOffset = offsets[1];
	from = dstSize;
	bailIfError(KPR_code_replace_aux(the, src, srcSize, &dst, &dstSize, result->count, offsets, replacement));
	to = dstSize;
	bailIfError(KPR_code_replace_add(the, src, srcSize, &dst, &dstSize, srcOffset, srcSize));
	FskMemPtrDispose(self->string);
	self->string = dst;
	self->from = fxUTF8ToUnicodeOffset(dst, from);
	self->to = fxUTF8ToUnicodeOffset(dst, to);
	KprCodeMeasure(self);
	KprCodeSearch(self, dstSize);
	if (self->shell) {
		KprCodeMeasureSelection(self);
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprSizeChanged);
	}
	return;
bail:
	FskMemPtrDispose(dst);
	xsThrowIfFskErr(err);
}

void KPR_code_replaceAll(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprCode self = kprGetHostData(xsThis, this, code);
	xsStringValue replacement = xsToString(xsArg(0));
	KprCodeResult result;
    UInt32 resultSize;
    xsStringValue src = self->string;
    SInt32 srcOffset = 0;
   	SInt32 srcSize = FskStrLen(src);
    xsStringValue dst = NULL;
    SInt32 dstSize = 0;

	FskGrowableArrayGetPointerToItem(self->results, 0, (void **)&result);
    resultSize = FskGrowableArrayGetItemSize(self->results);
	bailIfError(FskMemPtrNew(1, &dst));
	dst[dstSize] = 0;
	while (result->count >= 0) {
		int* offsets = (int*)(((char*)result) + sizeof(KprCodeResultRecord));
		bailIfError(KPR_code_replace_add(the, src, srcSize, &dst, &dstSize, srcOffset, offsets[0]));
        srcOffset = offsets[1];
		bailIfError(KPR_code_replace_aux(the, src, srcSize, &dst, &dstSize, result->count, offsets, replacement));
		result = (KprCodeResult)(((char*)result) + resultSize);
	}
	bailIfError(KPR_code_replace_add(the, src, srcSize, &dst, &dstSize, srcOffset, srcSize));
	FskMemPtrDispose(self->string);
	self->string = dst;
	KprCodeMeasure(self);
	KprCodeSearch(self, dstSize);
	if (self->shell) {
		KprCodeMeasureSelection(self);
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprSizeChanged);
	}
	return;
bail:
	FskMemPtrDispose(dst);
	xsThrowIfFskErr(err);
}

FskErr KPR_code_replace_add(xsMachine *the, char* src, SInt32 srcSize, char** dst, SInt32* dstSize, SInt32 from, SInt32 to)
{
	FskErr err = kFskErrNone;
    SInt32 length = to - from;
    if (length) {
        bailIfError(FskMemPtrRealloc(*dstSize + length + 1, dst));
        FskMemCopy(*dst + *dstSize, src + from, length);
        *dstSize += length;
        (*dst)[*dstSize] = 0;
    }
bail:
    return err;
}

FskErr KPR_code_replace_aux(xsMachine *the, char* src, SInt32 srcSize, char** dst, SInt32* dstSize, SInt32 count, int* offsets, xsStringValue replacement)
{
	FskErr err = kFskErrNone;
	xsStringValue r;
	SInt32 l;
	xsBooleanValue flag;
	char c, d;
	SInt32 i;
	int* captures;
	xsStringValue s;
	r = replacement;
	l = 0;
	flag = 0;
	while ((c = *r++)) {
		if (c == '$') {
			c = *r++;
			switch (c) {
			case '$':
				l++;
				flag = 1;
				break;
			case '&':
				l += offsets[1] - offsets[0];
				flag = 1;
				break;
			case '`':
				l += offsets[0];
				flag = 1;
				break;
			case '\'':
				l += srcSize - offsets[1];
				flag = 1;
				break;
			default:
				if (('0' <= c) && (c <= '9')) {
					i = c - '0';
					d = *r;
					if (('0' <= d) && (d <= '9')) {
						i = (i * 10) + d - '0';
						r++;
					}
					if ((0 < i) && (i <= count)) {
						captures = offsets + (2 * i);
						if (captures[0] >= 0)
							l += captures[1] - captures[0];
						flag = 1;
					}
					else {
						l++;
						l++;
						if (d)
							l++;
					}
				}
				else {
					l++;
					if (c)
						l++;
				}
				break;
			}
			if (!c)
				break;
		}
		else
			l++;
	}
	if (flag) {
		bailIfError(FskMemPtrRealloc(*dstSize + l + 1, dst));
		r = replacement;
		s = *dst + *dstSize;
		*dstSize += l;
		while ((c = *r++)) {
			if (c == '$') {
				c = *r++;
				switch (c) {
				case '$':
					*s++ = c;
					break;
				case '&':
					l = offsets[1] - offsets[0];
					FskMemCopy(s, src + offsets[0], l);
					s += l;
					break;
				case '`':
					l = offsets[0];
					FskMemCopy(s, src, l);
					s += l;
					break;
				case '\'':
					l = srcSize - offsets[1];
					FskMemCopy(s, src + offsets[1], l);
					s += l;
					break;
				default:
					if (('0' <= c) && (c <= '9')) {
						i = c - '0';
						d = *r;
						if (('0' <= d) && (d <= '9')) {
							i = (i * 10) + d - '0';
							r++;
						}
						else
							d = 0;
						if ((0 <= i) && (i < count)) {
							captures = offsets + (2 * i);
							if (captures[0] >= 0) {
								l = captures[1] - captures[0];
								FskMemCopy(s, src + captures[0], l);
								s += l;
							}
						}
						else {
							*s++ = '$';
							*s++ = c;
							if (d)
								*s++ = d;
						}
					}
					else {
						*s++ = '$';
						if (c)
							*s++ = c;
					}
					break;
				}
				if (!c)
					break;
			}
			else
				*s++ = c;
		}
		*s = 0;
	}
	else {
		l = FskStrLen(replacement);
		bailIfError(FskMemPtrRealloc(*dstSize + l + 1, dst));
		FskMemCopy(*dst + *dstSize, replacement, l + 1);
		*dstSize += l;
	}
bail:
    return err;
}

void KPR_code_select(xsMachine *the)
{
	KprCode self = kprGetHostData(xsThis, this, code);
	KprCodeSelect(self, xsToInteger(xsArg(0)), xsToInteger(xsArg(1)));
}

void KPR_code_tab(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprCode self = kprGetHostData(xsThis, this, code);
	xsStringValue s = self->string;
	SInt32 from = fxUnicodeToUTF8Offset(s, self->from);
	SInt32 to = fxUnicodeToUTF8Offset(s, self->to);
	SInt32 direction = (xsToInteger(xsArgc) > 0) ? (xsToBoolean(xsArg(0)) ? +1 : -1) : +1;
	xsStringValue p, q, r, buffer = NULL;
	SInt32 delta = 0, size;
	p = s + from;
	while (p > s) {
		char c = *(--p);
		if ((c == 10) || (c == 13)) {
			p++;
			break;
		}
	}
	q = s + to;
	if (q > s) {
		char c = *(--q);
		if ((c != 10) && (c != 13))
			q++;
	}
	if (p < q) {
		r = p;
		if (direction < 0) {
			if (*r == '\t') {
				r++;
				delta--;
				if ((p - s) < from)
					from--;
			}
			while (r < q) {
				char c = *r++;
				if (((c == 10) || (c == 13)) && (*r == 9)) {
					r++;
					delta--;
				}
			}
		}
		else {
			delta++;
			if ((p - s) < from)
				from++;
			while (r < q) {
				char c = *r;
				if ((c == 10) || (c == 13))
					delta++;
				r++;
			}
		}
		to += delta;
	}
	if (delta) {
		self->from = fxUTF8ToUnicodeOffset(s, p - s);
		self->to = fxUTF8ToUnicodeOffset(s, q - s);
		size = q - p;
		bailIfError(FskMemPtrNew(size + delta, &buffer));
		r = buffer;
		if (direction < 0) {
			if (*p == '\t')
				p++;
			while (p < q) {
				char c = *p++;
				*r++ = c;
				if (((c == 10) || (c == 13)) && (*p == 9))
					p++;
			}
		}
		else {
			*r++ = 9;
			while (p < q) {
				char c = *p++;
				*r++ = c;
				if ((c == 10) || (c == 13))
					*r++ = 9;
			}
		}
		bailIfError(KprCodeInsertStringWithLength(self, buffer, size + delta));
		from = fxUTF8ToUnicodeOffset(self->string, from);
		to = fxUTF8ToUnicodeOffset(self->string, to);
		KprCodeSelect(self, from, to - from);
	}
bail:
	FskMemPtrDispose(buffer);
	xsThrowIfFskErr(err);
}

void process_getenv(xsMachine* the)
{
	xsStringValue result = getenv(xsToString(xsArg(0)));
	if (result)
		xsResult = xsString(result);
}

void KPR_shell_changeCursor(xsMachine* the)
{
	KprShell self = xsGetHostData(xsThis);
	(void)FskWindowSetCursorShape(self->window, xsToInteger(xsArg(0)));
}

void KPR_system_getUUID(xsMachine *the)
{
	char* id = xsToString(xsArg(0));
	char* uuid = FskUUIDGetForKey(id);
	xsResult = xsString(uuid);
}


void KPR_Shell_patch(xsMachine* the)
{
	xsVars(1);
	xsResult = xsGet(xsGlobal, xsID("system"));
	xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsVar(0), xsID("arrow"), xsInteger(kFskCursorArrow), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("aliasArrow"), xsInteger(kFskCursorAliasArrow), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("copyArrow"), xsInteger(kFskCursorCopyArrow), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("wait"), xsInteger(kFskCursorWait), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("iBeam"), xsInteger(kFskCursorIBeam), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("notAllowed"), xsInteger(kFskCursorNotAllowed), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("resize"), xsInteger(kFskCursorResizeAll), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("resizeEW"), xsInteger(kFskCursorResizeLeftRight), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("resizeNS"), xsInteger(kFskCursorResizeTopBottom), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("resizeNESW"), xsInteger(kFskCursorResizeNESW), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("resizeNWSE"), xsInteger(kFskCursorResizeNWSE), xsDefault, xsDontScript);			
	xsNewHostProperty(xsVar(0), xsID("link"), xsInteger(kFskCursorLink), xsDefault, xsDontScript);			
	xsNewHostProperty(xsVar(0), xsID("resizeColumn"), xsInteger(kFskCursorResizeColumn), xsDefault, xsDontScript);			
	xsNewHostProperty(xsVar(0), xsID("resizeRow"), xsInteger(kFskCursorResizeRow), xsDefault, xsDontScript);			
	xsNewHostProperty(xsResult, xsID("cursors"), xsVar(0), xsDefault, xsDontScript);
}

