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
#if TARGET_OS_IPHONE
	#include "FskCocoaSupportPhone.h"
#endif

#include "kprBehavior.h"
#include "kprContent.h"
#include "kprSkin.h"
#include "kprStyle.h"
#include "kprText.h"
#include "kprURL.h"
#include "kprShell.h"

struct KprTextLinkStruct {
	KprSlotPart;
	KprContentPart;
};

static FskErr KprTextLinkNew(KprTextLink *it);
static void KprTextLinkInvalidated(void* it, FskRectangle area);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprTextLinkInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprTextLink", FskInstrumentationOffset(KprTextLinkRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprTextLinkDispatchRecord = {
	"content",
	KprContentActivated,
	KprContentAdded,
	KprContentCascade,
	KprContentDispose,
	KprContentDraw,
	KprContentFitHorizontally,
	KprContentFitVertically,
	KprContentGetBitmap,
	KprContentHit,
	KprContentIdle,
	KprTextLinkInvalidated,
	KprContentLayered,
	KprContentMark,
	KprContentMeasureHorizontally,
	KprContentMeasureVertically,
	KprContentPlace,
	KprContentPlaced,
	KprContentPredict,
	KprContentReflowing,
	KprContentRemoving,
	KprContentSetWindow,
	KprContentShowing,
	KprContentShown,
	KprContentUpdate
};

FskErr KprTextLinkNew(KprTextLink *it)
{
	KprTextLink self;
	FskErr err = kFskErrNone;
	bailIfError(FskMemPtrNewClear(sizeof(KprTextRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprTextLinkInstrumentation);
	self->dispatch = &KprTextLinkDispatchRecord;
	self->flags = kprVisible | kprActive;
bail:
	return err;
}

void KprTextLinkInvalidated(void* it, FskRectangle area) 
{
	KprContainer self = it;
	KprContainer container = self->container;
	if (container)
		KprContentInvalidate((KprContent)container);
}

#if TARGET_OS_WIN32
#else
	#define LOWORD(l)   ((UInt16)(UInt32)(l))
	#define HIWORD(l)   ((UInt16)((((UInt32)(l)) >> 16) & 0xFFFF))
	#define MAKELONG(low, high)   ((UInt32)(((UInt16)(low)) | (((UInt32)((UInt16)(high))) << 16)))
#endif

#define kprTextStateCount 256
#define kprTextRunCount 1024

static void KprTextActivated(void* it, Boolean activateIt);
static void KprTextAdded(void* it, KprContent content); 
static void KprTextCascade(void* it, KprStyle style);
static void KprTextDispose(void* it);
static void KprTextDraw(void* it, FskPort port, FskRectangle area);
static void KprTextFitHorizontally(void* it);
static void KprTextFitVertically(void* it);
static KprContent KprTextHit(void* it, SInt32 x, SInt32 y);
//static void KprTextInvalidated(void* it, FskRectangle area);
static void KprTextMeasureHorizontally(void* it);
static void KprTextMeasureVertically(void* it);
static void KprTextPlace(void* it);
static void KprTextPlaced(void* it);
static void KprTextReflowing(void* it, UInt32 flags);
static void KprTextRemoving(void* it, KprContent content); 
static void KprTextSetWindow(void* it, KprShell shell, KprStyle style);

static void KprTextAdjustLineBounds(KprText self, FskRectangle lineBounds);
static void KprTextCascadeStyle(KprText self, KprStyle style);
static void KprTextCascadeStyles(KprText self);
static FskErr KprTextConcatBlock(KprText self);
static FskErr KprTextConcatLink(KprText self, KprContent link);
static FskErr KprTextConcatStyle(KprText self);
static void KprTextDrawLine(KprText self, char *text, KprTextLine line);
static void KprTextDrawRun(KprText self, char *text, SInt32 length, SInt32 x, SInt32 y, SInt32 width, SInt32* delta, SInt32 step, SInt32 *slop);
static FskErr KprTextFormat(KprText self, SInt32 theWidth, SInt32* theHeight);
static FskErr KprTextFormatLines(KprText self, char *text, KprStyle style, Boolean flushIt,
		SInt32* startOffset, SInt32 stopOffset, KprTextRun* startRun, KprTextRun stopRun, 
		FskRectangle lineBounds, SInt32 blockX, SInt32 blockWidth, SInt32* textWidth);
static FskErr KprTextFormatLine(KprText self, char *text, KprStyle style, Boolean flushIt,
		SInt32* startOffset, SInt32 stopOffset, KprTextRun* startRun, KprTextRun stopRun, 
		FskRectangle lineBounds, SInt32 blockX, SInt32 blockWidth);
static void KprTextGetTextBounds(KprText self, char *text, SInt32 length, FskRectangle bounds);
static SInt32 KprTextGetTextWidth(KprText self, char *text, SInt32 length);
static KprTextLink KprTextHitLine(KprText self, char *text, KprTextLine line, FskPoint mouse, UInt32 *offset);
static Boolean KprTextHitRun(KprText self, char *word, SInt32 length, SInt32 x, SInt32* width, SInt32 step, SInt32 *slop, UInt32 *offset);
static Boolean KprTextIsReturn(char *text, SInt32 c);
static Boolean KprTextIsSpaceAdvance(char *text, SInt32* advance);
static Boolean KprTextIsMultibyteBoundary(char *text, UInt32 offset);
static FskErr KprTextMeasureSelection(KprText self);
static FskErr KprTextPopState(KprText self);
static FskErr KprTextPushState(KprText self);
SInt32 KprTextSelectLine(KprText self, char *text, KprTextLine line, SInt32 offset);
void KprTextSelectRun(KprText self, char *word, SInt32 length, SInt32 x, SInt32 y, SInt32* width, SInt32 step, SInt32 *slop);
static void KprTextSetStyle(KprText self, KprStyle style);
static void KprTextUnbindStyles(KprText self);
static void KprTextUncascadeStyle(KprText self, KprStyle style);
static void KprTextUncascadeStyles(KprText self);
#if SUPPORT_INSTRUMENTATION
static void KprTextDumpBlock(KprText self);
static void KprTextDumpLine(KprText self);
static void KprTextDumpRuns(KprText self, SInt32 c, KprTextRun run);
#endif

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprTextInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprText", FskInstrumentationOffset(KprTextRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprTextDispatchRecord = {
	"text",
	KprTextActivated,
	KprTextAdded,
	KprTextCascade,
	KprTextDispose,
	KprTextDraw,
	KprTextFitHorizontally,
	KprTextFitVertically,
	KprContentGetBitmap,
	KprTextHit,
	KprContentIdle,
	KprContainerInvalidated,
	KprContainerLayered,
	KprContainerMark,
	KprTextMeasureHorizontally,
	KprTextMeasureVertically,
	KprTextPlace,
	KprTextPlaced,
	KprContainerPredict,
	KprTextReflowing,
	KprTextRemoving,
	KprTextSetWindow,
	KprContainerShowing,
	KprContainerShown,
	KprContainerUpdate
};

FskErr KprTextNew(KprText* it,  KprCoordinates coordinates, KprSkin skin, KprStyle style, char* text)
{
	FskErr err = kFskErrNone;
	KprText self;
	bailIfError(FskMemPtrNewClear(sizeof(KprTextRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprTextInstrumentation);
	self->dispatch = &KprTextDispatchRecord;
	self->flags = kprContainer | kprVisible;
	KprContentInitialize((KprContent)self, coordinates, skin, style);
	KprTextBegin(self);
    if (text)
        KprTextConcatString(self, text);
	KprTextEnd(self);
bail:
	return err;
}

FskErr KprTextBegin(KprText self) 
{
	FskErr err = kFskErrNone;
	KprContent current = self->first;
	
	KprTextUnbindStyles(self);
	FskGrowableStorageDispose(self->textBuffer);
	self->textBuffer = NULL;
	FskGrowableArrayDispose(self->blockBuffer);
	self->blockBuffer = NULL;
	FskGrowableArrayDispose(self->lineBuffer);
	self->lineBuffer = NULL;
	self->lineCount = 0;
	FskGrowableArrayDispose(self->selectionBuffer);
	self->selectionBuffer = NULL;
	FskGrowableArrayDispose(self->stateBuffer);
	self->stateBuffer = NULL;
	while (current) {
		KprContent next = current->next;
		KprContentClose(current);
		current = next;
	}
	self->first = NULL;
	self->last = NULL;

	bailIfError(FskGrowableStorageNew(1024, &(self->textBuffer)));
	self->textOffset = 0;
	bailIfError(FskGrowableArrayNew(sizeof(KprTextRunRecord), kprTextRunCount, &(self->blockBuffer)));
	self->blockOffset = -1;
	self->blockSize = 0;
	bailIfError(FskGrowableArrayNew(sizeof(KprTextStateRecord), kprTextStateCount, &(self->stateBuffer)));
	self->stateIndex = 0;
	self->stateRegister.link = NULL;
	self->stateRegister.style = NULL;
	self->blockFlag = 1;
	self->styleFlag = 1;
bail:
	return err;
}

FskErr KprTextBeginBlock(KprText self, KprStyle style, KprTextLink link) 
{
	FskErr err = kFskErrNone;
	bailIfError(KprTextPushState(self));
	if (link) {
		KprTextConcatLink(self, (KprContent)link);
		self->stateRegister.link = link;
	}
	if (style) {
		//if (self->shell)
		//	self->stateRegister.style = KprStyleCascade(style, self->stateRegister.style);
		//else
		//	self->stateRegister.style = style;
		bailIfError(KprStyleNew(&self->stateRegister.style, style->context, self->stateRegister.style, style));	
	}
	self->blockFlag = 1;
	self->styleFlag = 1;
bail:
	return err;
}

FskErr KprTextBeginSpan(KprText self, KprStyle style, KprTextLink link) 
{
	FskErr err = kFskErrNone;
	bailIfError(KprTextPushState(self));
	if (link) {
		KprTextConcatLink(self, (KprContent)link);
		self->stateRegister.link = link;
	}
	if (style) {
		//if (self->shell)
		//	self->stateRegister.style = KprStyleCascade(style, self->stateRegister.style);
		//else
		//	self->stateRegister.style = style;
		bailIfError(KprStyleNew(&self->stateRegister.style, style->context, self->stateRegister.style, style));	
	}
	self->styleFlag = 1;
bail:
	return err;
}

FskErr KprTextConcatContent(KprText self, KprContent content, UInt16 adjustment) 
{
	FskErr err = kFskErrNone;
	KprCoordinates coordinates = &content->coordinates;
	KprTextRunRecord aRun;
	
	ASSERT(self != NULL);
	ASSERT(content != NULL);
	ASSERT(content->container == NULL);
	ASSERT(content->previous == NULL);
	ASSERT(content->next == NULL);
	if (self->first) {
		content->previous = self->last;
		self->last->next = content;
	}
	else
		self->first = content;
	self->last = content;
	content->container = (KprContainer)self;
	coordinates->horizontal &= ~kprLeftRight;
	coordinates->left = 0;
	coordinates->right = 0;
	coordinates->vertical &= ~kprTopBottom;
	coordinates->top = 0;
	coordinates->bottom = 0;
	if (self->shell)
		(*content->dispatch->setWindow)(content, self->shell, self->style);
		
	if (self->blockFlag != 0) {
		bailIfError(KprTextConcatBlock(self));
	}
	if (self->styleFlag != 0) {
		bailIfError(KprTextConcatStyle(self));
	}
		
	aRun.item.offset = self->textOffset;
	aRun.item.length = -1;
	aRun.item.content = content;
	aRun.item.adjustment = adjustment;
	aRun.item.dummy = 0;
	bailIfError(FskGrowableArrayAppendItem(self->blockBuffer, (void *)&aRun));
	self->blockSize += 1;
	
	self->blockFlag = 0;
	self->styleFlag = 1;
bail:
	return err;
}

FskErr KprTextConcatString(KprText self, char* string) 
{
	FskErr err = kFskErrNone;
	bailIfError(KprTextConcatText(self, string, FskStrLen(string)));
bail:
	return err;
}

FskErr KprTextConcatText(KprText self, char* text, SInt32 length)
{
	FskErr err = kFskErrNone;
	/*char* p = text;
	SInt32 i = length;
	while (i) {
		if (*p == '\r') {
			SInt32 c = p - text;
			if (self->blockFlag != 0) {
				bailIfError(KprTextConcatBlock(self));
			}
			if (self->styleFlag != 0) {
				bailIfError(KprTextConcatStyle(self));
			}
 			bailIfError(FskGrowableStorageAppendItem(self->textBuffer, text, c));
			self->textOffset += c;
			self->blockFlag = 1;
			self->styleFlag = 1;
			text = p + 1;
			length -= c + 1;
		}
		p++;
		i--;
	}*/
	if (self->blockFlag != 0) {
		bailIfError(KprTextConcatBlock(self));
	}
	if (self->styleFlag != 0) {
		bailIfError(KprTextConcatStyle(self));
	}
 	bailIfError(FskGrowableStorageAppendItem(self->textBuffer, text, length));
	self->textOffset += length;
	self->blockFlag = 0;
	self->styleFlag = 0;
bail:
	return err;
}

FskErr KprTextEnd(KprText self) 
{
	FskErr err = kFskErrNone;
	char aChar = 0x00;
	char* text;
	if (self->textOffset == 0) {
		bailIfError(KprTextConcatBlock(self));
		bailIfError(KprTextConcatStyle(self));
	}
	bailIfError(KprTextConcatBlock(self));
 	bailIfError(FskGrowableStorageAppendItem(self->textBuffer, &aChar, 1));
	self->textOffset++;
	ASSERT(self->stateIndex == 0);
	FskGrowableArrayDispose(self->stateBuffer);
	self->stateBuffer = NULL;
	FskGrowableArrayMinimize(self->blockBuffer);
	FskGrowableStorageMinimize(self->textBuffer);
	FskGrowableStorageGetPointerToItem(self->textBuffer, 0, (void **)&text);
	self->length = fxUnicodeLength(text);
	self->from = self->to = 0;
#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(self, kFskInstrumentationLevelDebug))
		KprTextDumpBlock(self);
#endif
	self->textWidth = -1;
	self->textHeight = 0;
	if (self->shell) {
		KprTextCascadeStyles(self);
		KprContentReflow((KprContent)self, kprSizeChanged);
	}
bail:
	return err;
}

FskErr KprTextEndBlock(KprText self) 
{
	FskErr err = kFskErrNone;
	bailIfError(KprTextPopState(self));
	self->blockFlag = 1;
	self->styleFlag = 1;
bail:
	return err;
}

FskErr KprTextEndSpan(KprText self) 
{
	FskErr err = kFskErrNone;
	bailIfError(KprTextPopState(self));
	self->styleFlag = 1;
bail:
	return err;
}

void KprTextFlagged(KprText self)
{
	if (KprContentIsFocus((KprContent)self)) {
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprPositionChanged);
	}
}

void KprTextGetSelectionBounds(KprText self, FskRectangle bounds)
{
	FskRectangleSetEmpty(bounds);
	if (self->selectionBuffer) {
		UInt32 c = FskGrowableArrayGetItemCount(self->selectionBuffer), i;
		FskRectangleRecord part;
		for (i = 0; i < c; i++) {
			FskGrowableArrayGetItem(self->selectionBuffer, i, (void *)&part);
			FskRectangleUnion(bounds, &part, bounds);
		}
	}
}

UInt32 KprTextHitOffset(KprText self, SInt32 x, SInt32 y)
{
	char *text;
	KprTextLine line, next;
	UInt32 offset = 0;
	FskPointRecord mouse;
	KprTextRun run;
	mouse.x = x;
	mouse.y = y;
	
	if (!self->lineBuffer) return 0;
	if (!self->shell) return 0;
	self->port = self->shell->port;
	FskGrowableStorageGetPointerToItem(self->textBuffer, 0, (void **)&text);
	FskGrowableArrayGetPointerToItem(self->lineBuffer, 0, (void **)&line);
	while (line->count >= 0) {
		next = line + 1 + line->count;
		if (next->y > y) {
			if (x <= line->x) {
				run = (KprTextRun)(line + 1);
				offset = run->span.offset;
			}
			else if ((line->x + line->width) <= x) {
				if (next->count < 0)
					offset = self->textOffset - 1;
				else {
					offset = ((KprTextRun)(next + 1))->span.offset;
					if ((offset > 0) && KprTextIsReturn(text + offset - 1, 1))
						offset--;
				}
			}
			else
				KprTextHitLine(self, text, line, &mouse, &offset);
			break;
		}
		line = next;
	}
	self->port = NULL;
	return fxUTF8ToUnicodeOffset(text, offset);
}

FskErr KprTextInsertString(KprText self, char* string) 
{
	FskErr err = kFskErrNone;
	bailIfError(KprTextInsertStringWithLength(self, string, FskStrLen(string)));
bail:
	return err;
}

FskErr KprTextInsertStringWithLength(KprText self, char* text, SInt32 size) 
{
	FskErr err = kFskErrNone;
	char* buffer;
	SInt32 fromSize;
	SInt32 toSize;
	KprTextBlock block;
	
	FskGrowableArrayDispose(self->lineBuffer);
	self->lineBuffer = NULL;
	
	FskGrowableStorageGetPointerToItem(self->textBuffer, 0, (void **)&buffer);
	fromSize = fxUnicodeToUTF8Offset(buffer, self->from);
	toSize = fxUnicodeToUTF8Offset(buffer, self->to);
	if (fromSize < toSize) {
		FskGrowableStorageRemoveItem(self->textBuffer, fromSize, toSize - fromSize);
		self->textOffset -= toSize - fromSize;
	}
	if (size) {
		FskGrowableStorageInsertItemAtPosition(self->textBuffer, fromSize, text, size);
		self->textOffset += size;
	}
	FskGrowableStorageGetPointerToItem(self->textBuffer, 0, (void **)&buffer);
	self->length = fxUnicodeLength(buffer);
	self->from = self->to = fxUTF8ToUnicodeOffset(buffer, fromSize + size);
	
	FskGrowableArrayGetPointerToItem(self->blockBuffer, 0, (void **)&block);
	block += 2;
	block->offset = self->textOffset - 1;
#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(self, kFskInstrumentationLevelDebug))
		KprTextDumpBlock(self);
#endif
//bail:
	if (self->shell) {
		KprContentInvalidate((KprContent)self);
		self->textWidth = -1;
		self->textHeight = 0;
		KprContentReflow((KprContent)self, kprSizeChanged);
		KprShellAdjust(self->shell); // do it now to format the selection
	}
	return err;
}

FskErr KprTextSelect(KprText self, SInt32 selectionOffset, UInt32 selectionLength)
{
	SInt32 from = selectionOffset;
	SInt32 to = selectionOffset + selectionLength;
	if (from < 0)
		from = 0;
	else if (from > (SInt32)self->length)
		from = (SInt32)self->length;
	if (to < 0)
		to = 0;
	else if (to > (SInt32)self->length)
		to = (SInt32)self->length;
	self->from = from;
	self->to = to;
	if (self->shell) {
		KprTextMeasureSelection(self);
		KprContentInvalidate((KprContent)self);
		KprContentReflow((KprContent)self, kprPositionChanged);
	}
//bail:
	return kFskErrNone;
}

/* DISPATCH */

void KprTextActivated(void* it, Boolean activateIt) 
{
	KprText self = it;
	if (KprContentIsFocus(it)) {
		KprContentInvalidate(it);
		if (activateIt)
			KprContentReflow(it, kprPositionChanged);
		else
			KprShellSetCaret(self->shell, self, NULL);
		KprShellKeyActivate(self->shell, activateIt);
	}
}

void KprTextAdded(void* it UNUSED, KprContent content UNUSED) 
{
	ASSERT(0);
}

void KprTextCascade(void* it, KprStyle style) 
{
	KprText self = it;
	if (self->blockBuffer)
		KprTextUncascadeStyles(self);
	KprContentCascade(it, style);
	if (self->blockBuffer)
		KprTextCascadeStyles(self);
	self->textWidth = -1;
	self->textHeight = 0;
	KprContentReflow(it, kprSizeChanged);
}

void KprTextDispose(void* it) 
{
	KprText self = it;
	KprTextUnbindStyles(self);
	FskGrowableArrayDispose(self->stateBuffer);
	FskGrowableArrayDispose(self->selectionBuffer);
	FskGrowableArrayDispose(self->lineBuffer);
	FskGrowableArrayDispose(self->blockBuffer);
	FskGrowableStorageDispose(self->textBuffer);
	KprContainerDispose(it);
}

void KprTextDraw(void* it, FskPort port, FskRectangle area) 
{
	KprText self = it;
	FskRectangleRecord bounds;
	FskColorRGBARecord color;
	char *text;
	KprTextLine line, next;
	SInt32 clipTop, clipBottom;
	SInt32 lineTop, lineBottom;
	
	FskRectangleSet(&bounds, 0, 0, self->bounds.width, self->bounds.height);
	if (!bounds.width || !bounds.height) return;
	if (self->skin)
		KprSkinFill(self->skin, port, &bounds, self->variant, self->state, self->coordinates.horizontal, self->coordinates.vertical);
	FskPortGetPenColor(port, &color);
	if ((self->flags & kprTextSelectable) || KprContentIsFocus((KprContent)self)) {
		if (self->selectionBuffer) {
			if (self->from == self->to) {
				FskGrowableArrayGetItem(self->selectionBuffer, 0, (void *)&bounds);
				if ((self->flags & kprTextEditable) && (self->shell->caretFlags & 2)) {
					FskPortSetPenColor(port, &self->style->colors[0]);
					FskPortRectangleFill(port, &bounds);
				}
			}
			else if (self->skin) {
				double state = (self->shell->flags & kprWindowActive) ? 3 : 2;
				UInt32 c = FskGrowableArrayGetItemCount(self->selectionBuffer), i;
				for (i = 0; i < c; i++) {
					FskGrowableArrayGetItem(self->selectionBuffer, i, (void *)&bounds);
					KprSkinFill(self->skin, port, &bounds, self->variant, state, kprCenter, kprMiddle);
				}
			}
		}
	}
	if (self->lineBuffer) {
		self->port = port;
		FskPortGetGraphicsMode(self->port, &self->graphicsMode, &self->graphicsModeParameters);
		FskGrowableStorageGetPointerToItem(self->textBuffer, 0, (void **)&text);
		FskGrowableArrayGetPointerToItem(self->lineBuffer, 0, (void **)&line);
		clipTop = area->y;
		clipBottom = clipTop + area->height;
		while (line->count >= 0) {
			next = line + 1 + line->count;
			lineTop = line->y;
			lineBottom = next->y;
			if (clipBottom <= lineTop)
				break;
			if (lineBottom <= clipTop) {
				line = next;
				continue;
			}
			KprTextDrawLine(self, text, line);
			line = next;
		}
		FskMemPtrDispose(self->graphicsModeParameters);
		self->graphicsModeParameters = NULL;
		self->graphicsMode = 0;
		self->port = NULL;
	}
	FskPortSetPenColor(port, &color);
}

void KprTextFitHorizontally(void* it) 
{
	KprText self = it;
	SInt32 height;
	KprContent content = self->first;
	KprContentFitHorizontally(it);
	if (self->textWidth == self->bounds.width) {
		height = self->textHeight;
	}
	else {
		self->port = self->shell->port;
		if ((self->coordinates.horizontal != kprLeftRight) && !(self->coordinates.horizontal & kprWidth)) {
			if (self->textWidth < 0) {
				KprTextFormat(self, 0x7FFF, &height);
				if (self->lineBuffer) {
					KprTextLine line;
					SInt32 width = 0;
					FskGrowableArrayGetPointerToItem(self->lineBuffer, 0, (void **)&line);
					while (line->count >= 0) {
						if (width < line->width)
							width = line->width;
						line = line + 1 + line->count;
					}
					self->bounds.width = width;
				}
				if (self->flags & kprTextEditable) {
					if (height == 0) {
						FskRectangleRecord bounds;
						KprStyleMeasure(self->style, "dp", &bounds);
						height = bounds.height;
					}
					if (self->bounds.width == 0)
						self->bounds.width = 2;
				}
			}
			else {
				height = self->textHeight;
				self->bounds.width = self->textWidth;
			}
		}
		else {	
			KprTextFormat(self, self->bounds.width, &height);
			if (self->flags & kprTextEditable) {
				if (height == 0) {
					FskRectangleRecord bounds;
					KprStyleMeasure(self->style, "dp", &bounds);
					height = bounds.height;
				}
				if (self->bounds.width == 0)
					self->bounds.width = 2;
			}
		}
		self->port = NULL;
		self->textWidth = self->bounds.width;
		self->textHeight = height;
	}
	KprTextMeasureSelection(it);
	if (!(self->coordinates.vertical & kprHeight)) {
		self->coordinates.height = height;
		self->flags |= kprHeightChanged;
	}
	while (content) {
		(*content->dispatch->fitHorizontally)(content);
		content = content->next;
	}
}

void KprTextFitVertically(void* it) 
{
	KprText self = it;
	KprContent content = self->first;
	KprContentFitVertically(it);
	while (content) {
		(*content->dispatch->fitVertically)(content);
		content = content->next;
	}
	self->flags |= kprContentsPlaced;
}

KprContent KprTextHit(void* it, SInt32 x, SInt32 y) 
{
	KprText self = it;
	KprContent result;
	char *text;
	KprTextLine line, next;
//	SInt32 lineTop, lineBottom;
	
	result = KprContainerHit(it, x, y);
	if (result != (KprContent)self)
		return result;
	if (!self->lineBuffer) 
		return result;
	if (!self->shell) 
		return result;
	self->port = self->shell->port;
	FskGrowableStorageGetPointerToItem(self->textBuffer, 0, (void **)&text);
	FskGrowableArrayGetPointerToItem(self->lineBuffer, 0, (void **)&line);
	while (line->count >= 0) {
		next = line + 1 + line->count;
		if (next->y > y) {
			FskPointRecord mouse;
			KprTextLink link;
			UInt32 offset = 0;
			mouse.x = x;
			mouse.y = y;
			link = KprTextHitLine(self, text, line, &mouse, &offset);
			if (link) {
				return (KprContent)link;
			}
			break;
		}
		line = next;
	}
	return result;
}

/*
void KprTextInvalidated(void* it, FskRectangle area) 
{
	KprContainer self = it;
	if (self->flags & kprVisible) {
		KprContainer container = self->container;
		if (container) {
			if (FskRectangleIsEmpty(area))
				FskRectangleSet(area, self->bounds.x, self->bounds.y, self->bounds.width, self->bounds.height);
			else {
				FskRectangleOffset(area, self->bounds.x, self->bounds.y);
				FskRectangleIntersect(area, KprBounds(self), area);
			}
			(*container->dispatch->invalidated)(container, area);
		}
	}
}
*/

void KprTextMeasureHorizontally(void* it) 
{
	KprText self = it;
	KprContent content = self->first;
	while (content) {
		(*content->dispatch->measureHorizontally)(content);
		content = content->next;
	}
	if (!(self->coordinates.horizontal & kprWidth))
		self->coordinates.width = 0;
}

void KprTextMeasureVertically(void* it) 
{
	KprText self = it;
	KprContent content = self->first;
	while (content) {
		(*content->dispatch->measureVertically)(content);
		content = content->next;
	}
}

void KprTextPlace(void* it) 
{
	KprContainerPlace(it);
	/*
	KprContainer self = it;
	KprContent content = self->first;
	while (content) {
		(*content->dispatch->place)(content);
		content = content->next;
	}
	*/
}

void KprTextPlaced(void* it) 
{
	KprText self = it;
	KprContainerPlaced(it);
	if (KprContentIsFocus(it)) {
		if ((self->flags & kprTextEditable) && (self->from == self->to)) {
			FskRectangleRecord bounds;
			FskGrowableArrayGetItem(self->selectionBuffer, 0, (void *)&bounds);
			KprContentToWindowCoordinates((KprContent)self, bounds.x, bounds.y, &bounds.x, &bounds.y);
			KprShellSetCaret(self->shell, self, &bounds);
		}
		else
			KprShellSetCaret(self->shell, self, NULL);
	}
}

void KprTextReflowing(void* it, UInt32 flags)
{
	KprContainer self = it;
	KprContainer container = self->container;
	if (flags & (kprPositionChanged | kprSizeChanged))
		self->flags |= kprSizeChanged;
	else
		self->flags |= kprContentsChanged;
	self->flags |= kprContentsPlaced;
	if (container)
		(*container->dispatch->reflowing)(container, self->flags);
}

void KprTextRemoving(void* it UNUSED, KprContent content UNUSED) 
{
	ASSERT(0);
}

void KprTextSetWindow(void* it, KprShell shell, KprStyle style)
{
	KprText self = it;
	if (self->shell && !shell) {
		if (KprContentIsFocus(it) && (self->flags & kprTextEditable) && (self->from == self->to))
			KprShellSetCaret(self->shell, self, NULL);
	}
	if (!style && self->blockBuffer)
		KprTextUncascadeStyles(self);
	KprContainerSetWindow(it, shell, style);
	if (style && self->blockBuffer)
		KprTextCascadeStyles(self);
}

/* IMPLEMENTATION */

void KprTextAdjustLineBounds(KprText self, FskRectangle lineBounds)
{
	SInt32 c = self->stateIndex, i;
	KprTextRun run;
	FskRectangle bounds;
	for (i = 0; i < c; i++) {
		FskGrowableArrayGetPointerToItem(self->stateBuffer, i, (void **)&run);
		bounds = &run->item.content->bounds;
		if (run->item.adjustment == kprTextFloatLeft) {
			if ((bounds->y <= lineBounds->y) && (lineBounds->y < (bounds->y + bounds->height))) {
				lineBounds->x += bounds->width;
				lineBounds->width -= bounds->width;
			}
		}
		else {
			if ((bounds->y <= lineBounds->y) && (lineBounds->y < (bounds->y + bounds->height))) {
				lineBounds->width -= bounds->width;
			}
		}
	}
}

void KprTextCascadeStyle(KprText self, KprStyle style) 
{
	if (!(style->flags & kprStyleInherited)) {
		if (style->father)
			KprTextCascadeStyle(self, style->father);
		else {
			style->father = self->style;
			KprAssetBind(self->style);
		}
		KprStyleInherit(style);
		style->flags |= kprStyleInherited;
	}
}

void KprTextCascadeStyles(KprText self) 
{
	KprTextBlock block;
	SInt32 c;
	FskGrowableArrayGetPointerToItem(self->blockBuffer, 0, (void **)&block);
	c = block->count;
	while (c >= 0) {
		KprTextRun run = (KprTextRun)(block + 1);
		if (block->style)
			KprTextCascadeStyle(self, block->style);
		else {
			block->style = self->style;
			KprAssetBind(self->style);
		}
		while (c > 0) {
			if (run->span.length >= 0) {
				if (run->span.style)
					KprTextCascadeStyle(self, run->span.style);
				else {
					run->span.style = self->style;
					KprAssetBind(self->style);
				}
			}
			c--;
			run++;
		}
		block = (KprTextBlock)run;
		c = block->count;
	}
	if (block->style)
		KprTextCascadeStyle(self, block->style);
	else {
		block->style = self->style;
		KprAssetBind(self->style);
	}
}

FskErr KprTextConcatBlock(KprText self)
{
	SInt32 aSize;
	KprTextBlock block;
	KprTextBlockRecord blockRecord;
	FskErr err = kFskErrNone;
	
	aSize = self->blockSize + 1;
	if (self->blockOffset >= 0) {
		bailIfError(FskGrowableArrayGetPointerToItem(self->blockBuffer, self->blockOffset, (void **)&block));
		block->count = self->blockSize - self->blockOffset - 1;
	}
	blockRecord.offset = self->textOffset;
	blockRecord.style = self->stateRegister.style;
	blockRecord.count = -1;
	bailIfError(FskGrowableArrayAppendItem(self->blockBuffer, (void *)&blockRecord));
	self->blockOffset = self->blockSize;
	self->blockSize = aSize;
	KprAssetBind(self->stateRegister.style);
bail:
	return err;
}

FskErr KprTextConcatLink(KprText self, KprContent link) 
{
	ASSERT(self != NULL);
	ASSERT(link != NULL);
	ASSERT(link->container == NULL);
	ASSERT(link->previous == NULL);
	ASSERT(link->next == NULL);
	if (self->first) {
		link->previous = self->last;
		self->last->next = link;
	}
	else
		self->first = link;
	self->last = link;
	link->container = (KprContainer)self;
	return kFskErrNone;
}

FskErr KprTextConcatStyle(KprText self)
{
	KprTextRunRecord runRecord;
	FskErr err = kFskErrNone;
	
	runRecord.span.offset = self->textOffset;
	runRecord.span.length = 0;
	runRecord.span.style = self->stateRegister.style;
	runRecord.span.link = self->stateRegister.link;
	bailIfError(FskGrowableArrayAppendItem(self->blockBuffer, (void *)&runRecord));
	self->blockSize += 1;
	KprAssetBind(self->stateRegister.style);
bail:
	return err;
}

void KprTextDrawLine(KprText self, char *text, KprTextLine line)
{
	SInt32 x, y, width, delta, step, slop, c;
	KprTextRun run;
	x = line->x;
	y = line->y + line->ascent;
	width = line->width;
	if (line->slop && line->portion) {
		step = MAKELONG(0, line->slop);
		step /= line->portion;
	}
	else
		step = 0;
	slop = 0;
	c = line->count;
	run = (KprTextRun)(line + 1);
	while (c > 0) {
		if (run->span.length >= 0) {
			FskTextFontInfoRecord fontInfo;
			KprStyleColorize(run->span.style, self->port, (run->span.link) ? run->span.link->state : self->state);
			KprTextSetStyle(self, run->span.style);
			FskPortGetFontInfo(self->port, &fontInfo);
			KprTextDrawRun(self, text + run->span.offset, run->span.length, x, y - fontInfo.ascent, width, &delta, step, &slop);
			FskPortSetGraphicsMode(self->port, self->graphicsMode, self->graphicsModeParameters);
		}
		else if (run->item.adjustment < kprTextFloatLeft)
			delta = run->item.content->bounds.width;
		else
			delta = 0;
		x += delta;
		width -= delta;
		c--;
		run++;
	}
}

void KprTextDrawRun(KprText self, char *word, SInt32 length, SInt32 x, SInt32 y, SInt32 width, SInt32* delta, SInt32 step, SInt32 *slop)
{
	FskRectangleRecord bounds;
	*delta = 0;
	if (!step) {
		if (length) {
			KprTextGetTextBounds(self, word, length, &bounds);
			FskRectangleOffset(&bounds, x, y);
			if (bounds.width > width) {
				UInt32 textStyle;
				FskPortGetTextStyle(self->port, &textStyle);
				textStyle |= kFskTextTruncateEnd;
				FskPortSetTextStyle(self->port, textStyle);
				bounds.width = width;
			}
			FskPortTextDraw(self->port, (char *)word, length, &bounds);
			*delta += bounds.width;
		}
	}
	else {
		char *space = word;
		char *limit = word + length;
		SInt32 advance;
		SInt32 slopHigh;
		while (word < limit) {
			if (KprTextIsSpaceAdvance(word, &advance)) {
				*slop += step;
				slopHigh = HIWORD(*slop);
				KprTextGetTextBounds(self, space, word - space, &bounds);
				FskRectangleOffset(&bounds, x + *delta, y);
				FskPortTextDraw(self->port, (char *)space, word - space, &bounds);
				*delta += bounds.width + slopHigh;
				*slop = LOWORD(*slop);
				space = word;
			}
			word += advance;
		}
		KprTextGetTextBounds(self, space, limit - space, &bounds);
		FskRectangleOffset(&bounds, x + *delta, y);
		if (bounds.width > width) {
			UInt32 textStyle;
			FskPortGetTextStyle(self->port, &textStyle);
			textStyle |= kFskTextTruncateEnd;
			FskPortSetTextStyle(self->port, textStyle);
			bounds.width = width;
		}
		FskPortTextDraw(self->port, (char *)space, limit - space, &bounds);
		*delta += bounds.width;
	}
}

FskErr KprTextFormat(KprText self, SInt32 theWidth, SInt32* theHeight)
{
	FskErr err = kFskErrNone;
	char *text;
	KprTextBlock block;
	KprStyle style;
	SInt32 startOffset, stopOffset, firstOffset, offset = 0, lastOffset;
	SInt32 blockX, blockWidth, breakWidth, textWidth;
	FskRectangleRecord lineBounds = {0, 0, 0, 0};
	UInt32 lineCount;
	SInt32 c, i;
	KprTextRun startRun, stopRun, run;
	SInt32 advance;

	FskInstrumentedItemPrintfNormal(self, "Measure Text");
	FskGrowableArrayDispose(self->lineBuffer);
	self->lineBuffer = NULL;
	FskGrowableArrayDispose(self->stateBuffer);
	self->stateBuffer = NULL;
	
	bailIfError(FskGrowableArrayNew(sizeof(KprTextRunRecord), kprTextStateCount, &(self->stateBuffer)));
	self->stateIndex = 0;
	c = FskGrowableArrayGetItemCount(self->blockBuffer);
	if (c < kprTextRunCount) c = kprTextRunCount;
	bailIfError(FskGrowableArrayNew(sizeof(KprTextRunRecord), (c * sizeof(KprTextRunRecord)), &(self->lineBuffer)));

	FskGrowableStorageGetPointerToItem(self->textBuffer, 0, (void **)&text);
	FskGrowableArrayGetPointerToItem(self->blockBuffer, 0, (void **)&block);

	while (block->count >= 0) {
		style = block->style;
		lineCount = style->lineCount;
		blockX = style->margins.left;
		blockWidth = theWidth - style->margins.left - style->margins.right;
		lineBounds.x = blockX + style->indentation;
		lineBounds.width = blockWidth - style->indentation;
		lineBounds.y += style->margins.top;
		KprTextAdjustLineBounds(self, &lineBounds);
		startOffset = stopOffset = block->offset;
		breakWidth = 0;
		textWidth = 0;
		c = block->count;
		startRun = stopRun = run = (KprTextRun)(block + 1);
		for (i = 0; i < c; i++) {
			if (run->span.length >= 0) {
				KprTextSetStyle(self, run->span.style);
				firstOffset = offset = run->span.offset;
				lastOffset = (run + 1)->span.offset;
				while (offset < lastOffset) {
					if (KprTextIsSpaceAdvance(text + offset, &advance) || KprTextIsReturn(text + offset, advance) || KprTextIsMultibyteBoundary(text, offset)) {
						if (firstOffset < offset) {
							textWidth += KprTextGetTextWidth(self, text + firstOffset, offset - firstOffset);
							firstOffset = offset;
						}
						if (lineBounds.width < breakWidth + textWidth) {
							lineCount--; 
							if (lineCount == 0) {
								block += 1 + c;
								KprTextFormatLine(self, text, style, true, &startOffset, block->offset, &startRun, run, &lineBounds, blockX, blockWidth);
								textWidth = 0;
								goto next;
							}
							if (breakWidth) {
								KprTextFormatLine(self, text, style, false, &startOffset, stopOffset, &startRun, stopRun, &lineBounds, blockX, blockWidth);
								KprTextSetStyle(self, run->span.style);
								breakWidth = 0;
							}
							if (lineBounds.width < textWidth) {
								KprTextFormatLines(self, text, style, false, &startOffset, offset, &startRun, run, &lineBounds, blockX, blockWidth, &textWidth); // @@ lineCount ?
								KprTextSetStyle(self, run->span.style);
								breakWidth = 0;
							}
						}
						if (KprTextIsReturn(text + offset, advance)) {
							KprTextFormatLine(self, text, style, true, &startOffset, offset, &startRun, run, &lineBounds, blockX, blockWidth);
							lineBounds.y += style->margins.bottom;
							lineBounds.x = blockX + style->indentation;
							lineBounds.width = blockWidth - style->indentation;
							lineBounds.y += style->margins.top;
							KprTextAdjustLineBounds(self, &lineBounds);
							startOffset = stopOffset = firstOffset = offset + advance;
							breakWidth = 0;
							textWidth = 0;
							startRun = run;
							lineCount--; 
							if (lineCount == 0)  {
								block += 1 + c;
								goto next;
							}
							KprTextSetStyle(self, run->span.style);
						}
						else {
							breakWidth += textWidth;
							textWidth = 0;
							stopOffset = offset;
						}
						stopRun = run;
					}
					offset += advance;
				}
				if (firstOffset < offset) {
					textWidth += KprTextGetTextWidth(self, text + firstOffset, offset - firstOffset);
					if (lineBounds.width < breakWidth + textWidth) {
						KprTextFormatLines(self, text, style, false, &startOffset, offset, &startRun, run, &lineBounds, blockX, blockWidth, &textWidth); // @@ lineCount ?
						KprTextSetStyle(self, run->span.style);
						breakWidth = 0;
						breakWidth += textWidth;
						textWidth = 0;
						stopOffset = offset;
						stopRun = run;
					}
				}
			}
			else {
				FskRectangle bounds = &(run->item.content->bounds);
				KprCoordinates coordinates = &(run->item.content->coordinates);
				bounds->width = coordinates->width;
				bounds->height = coordinates->height;
				if (run->item.adjustment >= kprTextFloatLeft) {
					if (lineBounds.width < breakWidth + textWidth) {
						 // @@ lineCount ?
						if (breakWidth) {
							KprTextFormatLine(self, text, style, false, &startOffset, stopOffset, &startRun, stopRun, &lineBounds, blockX, blockWidth);
							breakWidth = 0;
						}
					}
					breakWidth += textWidth;
					textWidth = coordinates->width;
					stopOffset = offset;
					stopRun = run;
					if (lineBounds.width < breakWidth + textWidth) {
						 // @@ lineCount ?
						if (breakWidth) {
							KprTextFormatLine(self, text, style, false, &startOffset, stopOffset, &startRun, stopRun, &lineBounds, blockX, blockWidth);
							breakWidth = 0;
						}
					}
					textWidth = 0;
					stopOffset = offset;
					stopRun = run;
					bailIfError(FskGrowableArrayAppendItem(self->stateBuffer, (void *)run));
					self->stateIndex++;
					if (run->item.adjustment == kprTextFloatLeft) {
						bounds->y = lineBounds.y;
						bounds->x = 0;
						lineBounds.x += bounds->width;
						lineBounds.width -= bounds->width;
					}
					else {
						bounds->y = lineBounds.y;
						bounds->x = theWidth - bounds->width;
						lineBounds.width -= bounds->width;
					}
					run->item.content->flags |= kprVisible;
				}
				else {
					if (lineBounds.width >= breakWidth + textWidth) {
						breakWidth += textWidth;
						textWidth = 0;
						stopOffset = offset;
					}
					textWidth += bounds->width;
					if (lineBounds.width < breakWidth + textWidth) {
						lineCount--; 
						if (lineCount == 0) {
							block += 1 + c;
							KprTextFormatLine(self, text, style, true, &startOffset, block->offset, &startRun, run, &lineBounds, blockX, blockWidth);
							textWidth = 0;
							goto next;
						}
						if (breakWidth) {
							KprTextFormatLine(self, text, style, false, &startOffset, stopOffset, &startRun, stopRun, &lineBounds, blockX, blockWidth);
							//KprTextSetStyle(self, run->span.style);
							breakWidth = 0;
						}
						if (lineBounds.width < textWidth) {
							KprTextFormatLines(self, text, style, false, &startOffset, offset, &startRun, run, &lineBounds, blockX, blockWidth, &textWidth); // @@ lineCount ?
							//KprTextSetStyle(self, run->span.style);
							breakWidth = 0;
						}
					}
					breakWidth += textWidth;
					textWidth = 0;
					stopOffset = offset;
					stopRun = run;
				}
			}
			run++;
		}
		if (lineBounds.width < breakWidth + textWidth) {
			lineCount--; 
			if (lineCount == 0) {
				block += 1 + c;
				KprTextFormatLine(self, text, style, true, &startOffset, block->offset, &startRun, stopRun, &lineBounds, blockX, blockWidth);
				textWidth = 0;
				goto next;
			}
			if (breakWidth) {
				KprTextFormatLine(self, text, style, false, &startOffset, stopOffset, &startRun, stopRun, &lineBounds, blockX, blockWidth);
				breakWidth = 0;
			}
			if (lineBounds.width < textWidth) {
				KprTextFormatLines(self, text, style, false, &startOffset, offset, &startRun, stopRun, &lineBounds, blockX, blockWidth, &textWidth); // @@ lineCount ?
				breakWidth = 0;
			}
		}
		breakWidth += textWidth;
		textWidth = 0;
		block += 1 + c;
		if (breakWidth)
			KprTextFormatLine(self, text, style, 1, &startOffset, block->offset, &startRun, (KprTextRun)block - 1, &lineBounds, blockX, blockWidth);
next:
		lineBounds.y += style->margins.bottom;
	}
	{
		KprTextLineRecord aLineRecord;
		aLineRecord.y = lineBounds.y;
		aLineRecord.ascent = 0;
		aLineRecord.descent = 0;
		aLineRecord.x = 0;
		aLineRecord.width = 0;
		aLineRecord.portion = 0;
		aLineRecord.slop = 0;
		aLineRecord.count = -1;
		bailIfError(FskGrowableArrayAppendItem(self->lineBuffer, (void *)&aLineRecord));
	}
	FskGrowableArrayMinimize(self->lineBuffer);
bail:
	if (err) {
		FskGrowableArrayDispose(self->lineBuffer);
		self->lineBuffer = NULL;
	}
	FskGrowableArrayDispose(self->stateBuffer);
	self->stateBuffer = NULL;
	{
		SInt32 height = lineBounds.y;
		KprContent content = self->first;
		while (content) {
			SInt32 y = content->bounds.y + content->bounds.height;
			if (height < y)
				height = y;
			content = content->next;
		}
		*theHeight = height;
	}
	
#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(self, kFskInstrumentationLevelDebug))
		KprTextDumpLine(self);
#endif
	return err;
}

FskErr KprTextFormatLines(KprText self, char *text, KprStyle style, Boolean flushIt UNUSED,
		SInt32* startOffset, SInt32 stopOffset, KprTextRun* startRun, KprTextRun stopRun UNUSED, 
		FskRectangle lineBounds, SInt32 blockX, SInt32 blockWidth, SInt32* textWidth)
{
	SInt32 offset = *startOffset;
	KprTextRun run = *startRun;
	SInt32 lineWidth = blockWidth;
	SInt32 count = 0;
	FskRectangleRecord bounds;
	while (offset < stopOffset) {
		if (run->span.length >= 0) {
			SInt32 length = (run + 1)->span.offset - offset;
			if (length > stopOffset - offset)
				length = stopOffset - offset;
			KprTextSetStyle(self, run->span.style);
			KprTextGetTextBounds(self, text + offset, length, &bounds);
			if (lineWidth < bounds.width) {
				UInt32 fb, fc;
				FskPortTextFitWidth(self->port, text + offset, length, lineWidth, kFskTextFitFlagBreak, &fb, &fc);
				KprTextFormatLine(self, text, style, true, startOffset, offset + fb, startRun, run, lineBounds, blockX, blockWidth);
				offset = *startOffset;
				lineWidth = blockWidth;
				count++;
			}
			else {
				offset += length;
				if (offset >= (run + 1)->span.offset)
					run++;
				lineWidth -= bounds.width;
			}
			
		}
		else {
			bounds = run->item.content->bounds;
			lineWidth -= bounds.width;
			run++;
		}
	}
	*textWidth = blockWidth - lineWidth;
	return kFskErrNone;
}

FskErr KprTextFormatLine(KprText self, char *text, KprStyle style, Boolean flushIt,
		SInt32* startOffset, SInt32 stopOffset, KprTextRun* startRun, KprTextRun stopRun, 
		FskRectangle lineBounds, SInt32 blockX, SInt32 blockWidth)
{
	FskErr err = kFskErrNone;
	KprTextLineRecord lineRecord;
	KprTextLine line;
	SInt32 c;
	KprTextRun run;
	UInt16 ascent, descent, leading, topHeight, middleHeight, bottomHeight;
	UInt16 alignment;
	SInt32 portion, slop;
	Boolean itemFlag;
	SInt32 advance;
	UInt16 itemHeight;
	SInt32 lineHeight;
	
	lineRecord.y = lineBounds->y;
	lineRecord.ascent = 0;
	lineRecord.descent = 0;
	lineRecord.x = (short)lineBounds->x;
	lineRecord.width = (short)lineBounds->width;
	lineRecord.portion = 0;
	lineRecord.slop = 0;
	lineRecord.count = -1;
	bailIfError(FskGrowableArrayAppendItem(self->lineBuffer, (void *)&lineRecord));
	FskGrowableArrayGetPointerToLastItem(self->lineBuffer, (void **)&line);
	c = 0;
	run = *startRun;
	while (run <= stopRun) {
		KprTextRun copy;
		bailIfError(FskGrowableArrayAppendItem(self->lineBuffer, (void *)run));
		FskGrowableArrayGetPointerToLastItem(self->lineBuffer, (void **)&copy);
		if (copy->span.length >= 0) {
			SInt32 offset;
			if (copy->span.offset < *startOffset)
				copy->span.offset = *startOffset;
			offset = (run + 1)->span.offset;
			if (offset > stopOffset)
				offset = stopOffset;
			copy->span.length = offset - copy->span.offset;
		}
		c++;
		run++;
	}
	line->count = (short)c;

	ascent = 0;
	descent = 0;
	leading = 0;
	topHeight = 0;
	middleHeight = 0;
	bottomHeight = 0;
	
	alignment = style->horizontalAlignment;
	if ((alignment & kprLeft) && (alignment & kprRight) && flushIt)
		alignment &= ~kprRight;
	portion = 0;
	slop = line->width;
	itemFlag = 0;
	
	c = line->count;
	run = (KprTextRun)(line + 1);
	while (c > 0) {
		if (run->span.length >= 0) {
			FskTextFontInfoRecord fontInfo;
			KprTextSetStyle(self, run->span.style);
			FskPortGetFontInfo(self->port, &fontInfo);
			if (ascent < fontInfo.ascent)
				ascent = (UInt16)fontInfo.ascent;
			if (descent < fontInfo.descent)
				descent = (UInt16)fontInfo.descent;
			if (leading < fontInfo.leading)
				leading = (UInt16)fontInfo.leading;
			if ((alignment & kprLeft) && (alignment & kprRight)) {
				char *word = text + run->span.offset;
				char *space = word;
				char *limit = word + run->span.length;
				while (word < limit) {
					if (KprTextIsSpaceAdvance(word, &advance)) {
						portion++;
						slop -= KprTextGetTextWidth(self, space, word - space);
						space = word;
					}
					word += advance;
				}
				slop -= KprTextGetTextWidth(self, space, limit - space);
			}
			else
				slop -= KprTextGetTextWidth(self, text + run->span.offset, run->span.length);
		} 
		else {
			if (run->item.adjustment < kprTextFloatLeft) {
				FskRectangle bounds = &(run->item.content->bounds);
				itemHeight = (UInt16)bounds->height;
				switch (run->item.adjustment) {
				case kprTextAscent:
					if (line->ascent < itemHeight)
						line->ascent = itemHeight;
					break;
				case kprTextMiddle:
					itemHeight /= 2;
					if (line->ascent < itemHeight)
						line->ascent = itemHeight;
					if (line->descent < itemHeight)
						line->descent = itemHeight;
					break;
				case kprTextDescent:
					if (line->descent < itemHeight)
						line->descent = itemHeight;
					break;
				case kprTextLineTop:
				case kprTextLineMiddle:
				case kprTextLineBottom:
					if (middleHeight < itemHeight)
						middleHeight = itemHeight;
					break;
				case kprTextTextTop:
					if (topHeight < itemHeight)
						topHeight = itemHeight;
					break;
				case kprTextTextMiddle:
					if (middleHeight < itemHeight)
						middleHeight = itemHeight;
					break;
				case kprTextTextBottom:
					if (bottomHeight < itemHeight)
						bottomHeight = itemHeight;
					break;
				}
				run->item.content->flags |= kprVisible;
				slop -= bounds->width;
				itemFlag = 1;
			}
		}
		c--;
		run++;
	}
	
	if (line->ascent < ascent)
		line->ascent = ascent;
	if (line->descent < descent)
		line->descent = descent;
	if (line->ascent < bottomHeight - descent)
		line->ascent = bottomHeight - descent;
	if (line->descent < topHeight - ascent)
		line->descent = topHeight - ascent;
	itemHeight = line->ascent + line->descent;
	if (middleHeight > itemHeight) {
		itemHeight = middleHeight - itemHeight;
		line->ascent += itemHeight / 2;
		line->descent += itemHeight - (itemHeight / 2);
	}
	lineHeight = style->lineHeight;
	if (lineHeight < 0) {
		SInt32 fromSpace = line->ascent + line->descent;
		SInt32 toSpace = -lineHeight;
		if (fromSpace) {
			line->ascent = (UInt16)((line->ascent * toSpace) / fromSpace);
			line->descent = (UInt16)((line->descent * toSpace) / fromSpace);
			leading = (UInt16)(toSpace - line->ascent - line->descent);
		}
	}
	else if (lineHeight > 0) {
		SInt32 fromSpace = line->ascent + line->descent;
		SInt32 toSpace = lineHeight;
		if (fromSpace && (fromSpace < toSpace)) {
			line->ascent = (UInt16)((line->ascent * toSpace) / fromSpace);
			line->descent = (UInt16)((line->descent * toSpace) / fromSpace);
			leading = (UInt16)(toSpace - line->ascent - line->descent);
		}
	}
	lineHeight = line->ascent + line->descent + leading;

	if (slop < 0)
		slop = 0;
	switch (alignment) {
	case kprCenter:
		line->x += (short)(slop / 2);
		line->width -= (short)slop;
		break;
	case kprLeft:
		line->width -= (short)slop;
		break;
	case kprRight:
		line->x += (short)slop;
		line->width -= (short)slop;
		break;
	default:
		line->slop = (short)slop;
		line->portion = (short)portion;
		break;
	}
	
	// second pass only to position items
	if (itemFlag) { 
		SInt32 x, step;
		x = line->x;
		if (line->slop && line->portion) {
			step = MAKELONG(0, line->slop);
			step /= line->portion;
		}
		else
			step = 0;
		slop = 0;
		c = line->count;
		run = (KprTextRun)(line + 1);
		while (c > 0) {
			if (run->span.length >= 0) {
				KprTextSetStyle(self, run->span.style);
				if (step) {
					char *word = text + run->span.offset;
					char *space = word;
					char *limit = word + run->span.length;
					while (word < limit) {
						if (KprTextIsSpaceAdvance(word, &advance)) {
							slop += step;
							x += KprTextGetTextWidth(self, space, word - space) + HIWORD(slop);
							slop = LOWORD(slop);
							space = word;
						}
						word += advance;
					}
					x += KprTextGetTextWidth(self, space, limit - space);
				}
				else
					x += KprTextGetTextWidth(self, text + run->span.offset, run->span.length);
			} 
			else {
				if (run->item.adjustment < kprTextFloatLeft) {
					FskRectangle bounds = &(run->item.content->bounds);
					itemHeight = (UInt16)run->item.content->coordinates.height;
					switch (run->item.adjustment) {
					case kprTextAscent:
						bounds->y = line->y + line->ascent - itemHeight;
						break;
					case kprTextMiddle:
						bounds->y = line->y + line->ascent - (itemHeight / 2);
						break;
					case kprTextDescent:
						bounds->y = line->y + line->ascent;
						break;
					case kprTextLineTop:
						bounds->y = line->y;
						break;
					case kprTextLineMiddle:
						bounds->y = line->y + ((lineHeight - itemHeight) / 2);
						break;
					case kprTextLineBottom:
						bounds->y = line->y  + lineHeight - itemHeight;
						break;
					case kprTextTextTop:
						bounds->y = line->y + line->ascent - ascent;
						break;
					case kprTextTextMiddle:
						bounds->y = line->y + line->ascent + ((descent - ascent - itemHeight) / 2);
						break;
					case kprTextTextBottom:
						bounds->y = line->y + line->ascent + descent - itemHeight;
						break;
					}
					bounds->x = x;
					x += bounds->width;
				}
			}
			c--;
			run++;
		}
	}
	lineBounds->height = lineHeight;
	
	// prepare next line
	if (!flushIt) {
		if (stopRun->span.length >= 0) {
			SInt32 nextOffset = (stopRun + 1)->span.offset;
			while (stopOffset < self->textOffset) {
				if (stopOffset >= nextOffset) {
					stopRun++;
					if (stopRun->span.length < 0)
						break;
					nextOffset = (stopRun + 1)->span.offset;
				}
				if (!KprTextIsSpaceAdvance(text + stopOffset, &advance))
					break;
				stopOffset += advance;
			}
		}
		else
			stopRun++;
	}
	*startOffset = stopOffset;
	*startRun = stopRun;
	lineBounds->x = blockX;
	lineBounds->width = blockWidth;
	lineBounds->y += lineBounds->height;
	KprTextAdjustLineBounds(self, lineBounds);
	self->lineCount++;
bail:
	return err;
}

void KprTextGetTextBounds(KprText self, char *text, SInt32 length, FskRectangle bounds)
{
	if (length == 0)
		FskRectangleSetEmpty(bounds);
	else
		FskPortTextGetBounds(self->port, text, length, bounds);
}

SInt32 KprTextGetTextWidth(KprText self, char *text, SInt32 length)
{
	FskRectangleRecord bounds;
	if (length == 0)
		return 0;
	FskPortTextGetBounds(self->port, text, length, &bounds);
	return bounds.width;	
}

KprTextLink KprTextHitLine(KprText self, char *text, KprTextLine line, FskPoint mouse, UInt32 *offset)
{
	SInt32 x, step, slop, c;
	KprTextRun run;
	x = line->x;
	if (line->slop && line->portion) {
		step = MAKELONG(0, line->slop);
		step /= line->portion;
	}
	else
		step = 0;
	slop = 0;
	c = line->count;
	run = (KprTextRun)(line + 1);
	while (c > 0) {
		if (run->span.length >= 0) {
//			FskTextFontInfoRecord fontInfo;
			SInt32 width = 0;
			KprTextSetStyle(self, run->span.style);
//			FskPortGetFontInfo(self->port, &fontInfo);
			width = 0;
			if (KprTextHitRun(self, text + run->span.offset, run->span.length, mouse->x - x, &width, step, &slop, offset)) {
				*offset += run->span.offset;
				return run->span.link;
			}
			x += width;
		}
		else if (run->item.adjustment < kprTextFloatLeft) {
			x += run->item.content->bounds.width;
			if (mouse->x < x) {
				*offset = run->item.offset;
				return NULL;
			}
		}
		c--;
		run++;
	}
	return NULL;
}

Boolean KprTextHitRun(KprText self, char *word, SInt32 length, SInt32 x, SInt32* width, SInt32 step, SInt32 *slop, UInt32 *offset)
{
	char* text = word;
	UInt32 fb;
	UInt32 fc;
	FskRectangleRecord bounds;

	if (!step) {
		if (length) {
			KprTextGetTextBounds(self, word, length, &bounds);
			if ((0 <= x) && (x < bounds.width)) {
				FskPortTextFitWidth(self->port, word, length, x, kFskTextFitFlagMidpoint, &fb, &fc);
				*offset = fb;
				return true;
			}
			*width += bounds.width;
		}
	}
	else {
		char *space = word;
		char *limit = word + length;
		SInt32 advance;
		SInt32 slopHigh;
		while (word < limit) {
			if (KprTextIsSpaceAdvance(word, &advance)) {
				*slop += step;
				slopHigh = HIWORD(*slop);
				KprTextGetTextBounds(self, space, word - space, &bounds);
				if ((0 <= x) && (x < bounds.width)) {
					FskPortTextFitWidth(self->port, space, length, x, kFskTextFitFlagMidpoint, &fb, &fc);
					*offset = space - text + fb;
					return true;
				}
				x -= bounds.width;
				*width += bounds.width;
				if ((0 <= x) && (x < slopHigh)) {
					*offset = word - text;
					return true;
				}
				x -= slopHigh;
				*width += slopHigh;
				*slop = LOWORD(*slop);
				space = word;
			}
			word += advance;
		}
		KprTextGetTextBounds(self, space, limit - space, &bounds);
		if ((0 <= x) && (x < bounds.width)) {
			FskPortTextFitWidth(self->port, space, length, x, kFskTextFitFlagMidpoint, &fb, &fc);
			*offset = space - text + fb;
			return true;
		}
		*width += bounds.width;
	}
	return false;
}

Boolean KprTextIsReturn(char *text, SInt32 c)
{
	return ((c == 1) && ((*text == '\n') || (*text == '\r')));
}

Boolean KprTextIsSpaceAdvance(char *text, SInt32* advance)
{
	*advance = FskTextUTF8Advance((unsigned char*)text, 0, 1);
	return FskTextUTF8IsWhitespace((unsigned char*)text, *advance, NULL);
}

Boolean KprTextIsMultibyteWordwrapped(const unsigned char *text, Boolean *wordwrapped)
{
	Boolean multibyte = true;
	SInt32 advance;

	*wordwrapped = false;

	advance = FskTextUTF8Advance(text, 0, 1);

	switch (advance) {
		case 1:	// Ascii
			multibyte = false;
			break;

		case 2:
			// Not Latin-1 (??)
			multibyte = !((0xc0 <= text[0]) && (text[0] <= 0xc3) && (0x80 <= text[1]) && (text[1] <= 0xbf));
			break;

		case 3:
			if (text[0] == 0xe3)
			{
				if (text[1] == 0x80)
				{
					if ((text[2] == 0x81) ||	// Tohten
						(text[2] == 0x82))		// Kuten
					{
						*wordwrapped = true;
					}
				}
			}
			else if (text[0] == 0xef)
			{
				if (text[1] == 0xbc)
				{
					if ((text[2] == 0x9f) ||	// ?
						(text[2] == 0x81))		// !
					{
						*wordwrapped = true;
					}
				}
			}
			break;
	}

	return multibyte;
}

Boolean KprTextIsMultibyteBoundary(char *text, UInt32 offset)
{
#if 1
	// return true if
	// text is multibyte, or
	// text is ascii and previous text is multi byte
	Boolean multibyte, wordwrapped;
	SInt32 advance;

	multibyte = KprTextIsMultibyteWordwrapped((unsigned char *)(text + offset), &wordwrapped);
	if (multibyte)
	{
		// text is multibyte
		// true if text is not wordwrapped.
		return !wordwrapped;
	}

	// text is ascii
	if (offset == 0)
	{
		// no previous text
		return false;
	}
	// true if previous text is multibyte
	advance = FskTextUTF8Advance((unsigned char *)text, offset, -1);
	multibyte = KprTextIsMultibyteWordwrapped((unsigned char *)(text + offset + advance), &wordwrapped);
	return multibyte;
#else
	return false;
#endif
}

FskErr KprTextMeasureSelection(KprText self)
{
	FskErr err = kFskErrNone;
	SInt32 from;
	SInt32 to;
	char *text;
	KprTextLine line, next;
	SInt32 start, stop;
	FskRectangleRecord bounds;
	
	if (!(self->flags & kprTextSelectable)) return err;
	FskInstrumentedItemPrintfNormal(self, "Measure Selection");
	if (!self->lineBuffer) return err;
	FskGrowableArrayDispose(self->selectionBuffer);
	self->selectionBuffer = NULL;
	bailIfError(FskGrowableArrayNew(sizeof(FskRectangleRecord), 1, &(self->selectionBuffer)));
	self->port = self->shell->port;
	FskGrowableStorageGetPointerToItem(self->textBuffer, 0, (void **)&text);
	FskGrowableArrayGetPointerToItem(self->lineBuffer, 0, (void **)&line);
	from = fxUnicodeToUTF8Offset(text, self->from);
	to = fxUnicodeToUTF8Offset(text, self->to);
	if (from == to) {
		if (line->count < 0) {
			KprStyleMeasure(self->style, "dp", &bounds);
			bounds.x = 0;
			bounds.y = 0;
			bounds.width = 2;
		}
		else {
			while (line->count >= 0) {
				start = ((KprTextRun)(line + 1))->span.offset;
				if (from == start) {
					bounds.x = line->x - 1;
					break;
				}
				next = line + 1 + line->count;
				if (next->count < 0) {
					stop = self->textOffset - 1;
					if (from == stop) {
                        bounds.x = line->x + line->width - 1;
						break;
					}
				}
				else
					stop = ((KprTextRun)(next + 1))->span.offset;
				if (from < stop) {
					bounds.x = KprTextSelectLine(self, text, line, from) - 1;
					break;
				}
				line = next;
			}
            if ((from == self->textOffset - 1) &&  KprTextIsReturn(text + self->textOffset - 2, 1)) {
                KprStyleMeasure(self->style, "dp", &bounds);
                bounds.x = 0;
                bounds.y = line->y + line->ascent + line->descent;
                bounds.width = 2;
            }
            else {
                bounds.y = line->y;
                bounds.width = 2;
                bounds.height = line->ascent + line->descent;
            }
		}
		bailIfError(FskGrowableArrayAppendItem(self->selectionBuffer, (void *)&bounds));
	}
	else {
		while (line->count >= 0) {
			start = ((KprTextRun)(line + 1))->span.offset;
			if (to < start)
				break;
			bounds.y = line->y;
			next = line + 1 + line->count;
			if (next->count < 0) {
				stop = self->textOffset;
				bounds.height = line->ascent + line->descent;
			}
			else {
				stop = ((KprTextRun)(next + 1))->span.offset;
				bounds.height = next->y - line->y;
			}
			if (from <= start) {
				bounds.x = self->style->margins.left; //0;
				if (stop <= to)
					bounds.width = self->bounds.width - self->style->margins.right; //self->bounds.width;
				else
					bounds.width = KprTextSelectLine(self, text, line, to);
				bounds.width -= bounds.x;
				bailIfError(FskGrowableArrayAppendItem(self->selectionBuffer, (void *)&bounds));
			}
			else if (from < stop) {
				bounds.x = KprTextSelectLine(self, text, line, from);
				if (stop <= to)
					bounds.width = self->bounds.width - self->style->margins.right; //self->bounds.width;
				else
					bounds.width = KprTextSelectLine(self, text, line, to);
				bounds.width -= bounds.x;
				bailIfError(FskGrowableArrayAppendItem(self->selectionBuffer, (void *)&bounds));
			}
			line = next;
		}
	}
	FskGrowableArrayMinimize(self->selectionBuffer);
	self->port = NULL;
 	if (KprContentIsFocus((KprContent)self))
   		KprShellKeySelect(self->shell);
bail:
	return err;
}

FskErr KprTextPopState(KprText self)
{
	FskErr err = kFskErrNone;
	if (self->stateIndex > 0) {
		self->stateIndex--;
		FskGrowableArrayGetItem(self->stateBuffer, self->stateIndex, (void *)&(self->stateRegister));
		FskGrowableArraySetItemCount(self->stateBuffer, self->stateIndex);
	}
	return err;
}

FskErr KprTextPushState(KprText self)
{
	FskErr err = kFskErrNone;
	bailIfError(FskGrowableArrayAppendItem(self->stateBuffer, (void *)&(self->stateRegister)));
	self->stateIndex++;
bail:
	return err;
}

SInt32 KprTextSelectLine(KprText self, char *text, KprTextLine line, SInt32 offset)
{
	SInt32 x, y, step, slop, c;
	KprTextRun run;
	x = line->x;
	y = line->y + line->ascent;
	if (line->slop && line->portion) {
		step = MAKELONG(0, line->slop);
		step /= line->portion;
	}
	else
		step = 0;
	slop = 0;
	c = line->count;
	run = (KprTextRun)(line + 1);
	while (c > 0) {
		if (run->span.length >= 0) {
			FskTextFontInfoRecord fontInfo;
			SInt32 width = 0;
			SInt32 length = offset - run->span.offset;
			Boolean flag = false;
			KprTextSetStyle(self, run->span.style);
			FskPortGetFontInfo(self->port, &fontInfo);
			width = 0;
			if ((length < 0) || (run->span.length < length))
				length = run->span.length;
			else
				flag = true;
			KprTextSelectRun(self, text + run->span.offset, length, x, y - fontInfo.ascent, &width, step, &slop);
			x += width;
			if (flag)
				return x;
		}
		else if (run->item.adjustment < kprTextFloatLeft)
			x += run->item.content->bounds.width;
		c--;
		run++;
	}
	return x;
}

void KprTextSelectRun(KprText self, char *word, SInt32 length, SInt32 x UNUSED, SInt32 y UNUSED, SInt32* width, SInt32 step, SInt32 *slop)
{
	FskRectangleRecord bounds;
	if (!step) {
		if (length) {
			KprTextGetTextBounds(self, word, length, &bounds);
			*width += bounds.width;
		}
	}
	else {
		char *space = word;
		char *limit = word + length;
		SInt32 advance;
		SInt32 slopHigh;
		while (word < limit) {
			if (KprTextIsSpaceAdvance(word, &advance)) {
				*slop += step;
				slopHigh = HIWORD(*slop);
				KprTextGetTextBounds(self, space, word - space, &bounds);
				bounds.width += slopHigh;
				*width += bounds.width;
				*slop = LOWORD(*slop);
				space = word;
			}
			word += advance;
		}
		KprTextGetTextBounds(self, space, limit - space, &bounds);
		*width += bounds.width;
	}
}

void KprTextSetStyle(KprText self, KprStyle style)
{
	KprStyleApply(style, self->port);
	FskPortSetTextAlignment(self->port, kFskTextAlignLeft, kFskTextAlignCenter);
}

void KprTextUnbindStyles(KprText self)
{
	if (self->blockBuffer) {
		KprTextBlock block;
		SInt32 c;
		FskGrowableArrayGetPointerToItem(self->blockBuffer, 0, (void **)&block);
		c = block->count;
		while (c >= 0) {
			KprTextRun run = (KprTextRun)(block + 1);
			while (c > 0) {
				if (run->span.length >= 0)
					KprAssetUnbind(run->span.style);
				c--;
				run++;
			}
			KprAssetUnbind(block->style);
			block = (KprTextBlock)run;
			c = block->count;
		}
		KprAssetUnbind(block->style);
	}
}

void KprTextUncascadeStyle(KprText self, KprStyle style) 
{
	if (style->flags & kprStyleInherited) {
		if (style->father != self->style)
			KprTextUncascadeStyle(self, style->father);
		else {
			KprAssetUnbind(self->style);
			style->father = NULL;
		}
		style->flags &= ~kprStyleInherited;
	}
}

void KprTextUncascadeStyles(KprText self) 
{
	KprTextBlock block;
	SInt32 c;
	FskGrowableArrayGetPointerToItem(self->blockBuffer, 0, (void **)&block);
	c = block->count;
	while (c >= 0) {
		KprTextRun run = (KprTextRun)(block + 1);
		if (block->style != self->style)
			KprTextUncascadeStyle(self, block->style);
		else {
			KprAssetUnbind(self->style);
			block->style = NULL;
		}
		while (c > 0) {
			if (run->span.length >= 0) {
				if (run->span.style != self->style)
					KprTextUncascadeStyle(self, run->span.style);
				else {
					KprAssetUnbind(self->style);
					run->span.style = NULL;
				}
			}
			c--;
			run++;
		}
		block = (KprTextBlock)run;
		c = block->count;
	}
	if (block->style != self->style)
		KprTextUncascadeStyle(self, block->style);
	else {
		KprAssetUnbind(self->style);
		block->style = NULL;
	}
}

#if SUPPORT_INSTRUMENTATION
void KprTextDumpBlock(KprText self)
{
	KprTextBlock block;

	FskGrowableArrayGetPointerToItem(self->blockBuffer, 0, (void **)&block);
	while (block->count >= 0) {
		FskInstrumentedItemSendMessageForLevel(self, kprInstrumentedTextDumpBlock, block, kFskInstrumentationLevelDebug);
		KprTextDumpRuns(self, block->count, (KprTextRun)(block + 1));
		block += 1 + block->count;
	}
	FskInstrumentedItemSendMessageForLevel(self, kprInstrumentedTextDumpBlock, block, kFskInstrumentationLevelDebug);
}

void KprTextDumpLine(KprText self)
{
	KprTextLine line;

	FskGrowableArrayGetPointerToItem(self->lineBuffer, 0, (void **)&line);
	while (line->count >= 0) {
		FskInstrumentedItemSendMessageForLevel(self, kprInstrumentedTextDumpLine, line, kFskInstrumentationLevelDebug);
		KprTextDumpRuns(self, line->count, (KprTextRun)(line + 1));
		line += 1 + line->count;
	}
	FskInstrumentedItemSendMessageForLevel(self, kprInstrumentedTextDumpLine, line, kFskInstrumentationLevelDebug);
}

void KprTextDumpRuns(KprText self, SInt32 c, KprTextRun run)
{
	char *text;
	SInt32 i;

	FskGrowableStorageGetPointerToItem(self->textBuffer, 0, (void **)&text);
	for (i = 0; i < c; i++) {
		void* params[2];
		SInt32 length = run->span.length;
		params[0] = run;
		if (length >= 0) {
			SInt32 j;
			char buffer[16];
			if (length > 15) 
				length = 12;
			for (j = 0; j < length; j++) {
				if (run->span.offset + j > (SInt32)FskGrowableStorageGetSize(self->textBuffer))
					break;
				if (*(text + run->span.offset + j) <= 20)
					buffer[j] = 20;
				else
					buffer[j] = *(text + run->span.offset + j);
			}
			if (run->span.length > 15) {
				buffer[j++] = '.';
				buffer[j++] = '.';
				buffer[j++] = '.';
			}
			buffer[j] = 0;
			params[1] = buffer;
		}
		else
			params[1] = NULL;
		FskInstrumentedItemSendMessageForLevel(self, kprInstrumentedTextDumpRun, params, kFskInstrumentationLevelDebug);
		run++;
	}
}
#endif

/* ECMASCRIPT */

void KPR_Text(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprSkin skin = NULL;
	KprStyle style = NULL;
	xsStringValue text = NULL;
	KprText self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	if ((c > 1) && xsTest(xsArg(1)))
		skin = kprGetHostData(xsArg(1), skin, skin);
	if ((c > 2) && xsTest(xsArg(2)))
		style = kprGetHostData(xsArg(2), style, style);
	if ((c > 3) && xsTest(xsArg(3)))
		text = xsToString(xsArg(3));
	xsThrowIfFskErr(KprTextNew(&self, &coordinates, skin, style, text));
	kprContentConstructor(KPR_Layout);
}

void KPR_text_get_editable(xsMachine *the)
{
	KprText self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprTextEditable) ? xsTrue : xsFalse;
}

void KPR_text_get_length(xsMachine *the)
{
	KprText self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->length);
}

void KPR_text_get_selectable(xsMachine *the)
{
	KprText self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprTextSelectable) ? xsTrue : xsFalse;
}

void KPR_text_get_selectionBounds(xsMachine *the)
{
	KprText self = xsGetHostData(xsThis);
	FskRectangleRecord bounds;
	KprTextGetSelectionBounds(self, &bounds);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsInteger(bounds.x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsInteger(bounds.y), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(bounds.width), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(bounds.height), xsDefault, xsDontScript);
}

void KPR_text_get_selectionLength(xsMachine *the)
{
	KprText self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->to - self->from);
}

void KPR_text_get_selectionOffset(xsMachine *the)
{
	KprText self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->from);
}

void KPR_text_get_spans(xsMachine *the)
{
	xsDebugger();
}

void KPR_text_get_string(xsMachine *the)
{
	KprText self = xsGetHostData(xsThis);
	if (self->textBuffer) {
		char *text;
		FskGrowableStorageGetPointerToItem(self->textBuffer, 0, (void **)&text);
		xsResult = xsString(text);
	}
}

void KPR_text_set_active(xsMachine *the)
{
	KprText self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprActive;
	else
		self->flags &= ~(kprActive | kprTextEditable | kprTextSelectable);
	KprTextFlagged(self);
}

void KPR_text_set_editable(xsMachine *the)
{
	KprText self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprActive | kprTextEditable | kprTextSelectable;
	else
		self->flags &= ~kprTextEditable;
	KprTextFlagged(self);
}

void KPR_text_set_selectable(xsMachine *the)
{
	KprText self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprActive | kprTextSelectable;
	else
		self->flags &= ~(kprTextEditable | kprTextSelectable);
	KprTextFlagged(self);
}

void KPR_text_set_string(xsMachine *the)
{
	KprText self = xsGetHostData(xsThis);
	KprTextBegin(self);
	if (xsTest(xsArg(0)))
		KprTextConcatString(self, xsToString(xsArg(0)));
	KprTextEnd(self);
}

void KPR_text_begin(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	KprTextBegin(self);
}

void KPR_text_beginBlock(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	xsIntegerValue c = xsToInteger(xsArgc);
	KprStyle style = NULL;
	KprTextLink link = NULL;
	if ((c > 0) && (xsTest(xsArg(0))))
		style = kprGetHostData(xsArg(0), style, style);
	if ((c > 1) && (xsTest(xsArg(1)))) {
		KprBehavior behavior = NULL;
		xsThrowIfFskErr(KprTextLinkNew(&link));
		xsThrowIfFskErr(KprScriptBehaviorNew(&behavior, (KprContent)link, the, &xsArg(1)));
		link->behavior = behavior;
	}
	KprTextBeginBlock(self, style, link);
}

void KPR_text_beginSpan(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	xsIntegerValue c = xsToInteger(xsArgc);
	KprStyle style = NULL;
	KprTextLink link = NULL;
	if ((c > 0) && (xsTest(xsArg(0))))
		style = kprGetHostData(xsArg(0), style, style);
	if ((c > 1) && (xsTest(xsArg(1)))) {
		KprBehavior behavior = NULL;
		xsThrowIfFskErr(KprTextLinkNew(&link));
		xsThrowIfFskErr(KprScriptBehaviorNew(&behavior, (KprContent)link, the, &xsArg(1)));
		link->behavior = behavior;
	}
	KprTextBeginSpan(self, style, link);
}

void KPR_text_concat(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID_content))) {
		KprContent content = kprGetHostData(xsArg(0), content, content);
		UInt16 adjustment = kprTextLineMiddle;
		if ((xsToInteger(xsArgc) > 1) && (xsTest(xsArg(1)))) {
			char* align = xsToString(xsArg(1));
			if (!FskStrCompare(align, "left"))
				adjustment = kprTextFloatLeft;
			else if (!FskStrCompare(align, "right"))
				adjustment = kprTextFloatRight;
			else if (!FskStrCompare(align, "top"))
				adjustment = kprTextLineTop;
			else if (!FskStrCompare(align, "middle"))
				adjustment = kprTextLineMiddle;
			else if (!FskStrCompare(align, "bottom"))
				adjustment = kprTextLineBottom;
		}
		KprTextConcatContent(self, content, adjustment);
	}
	else {
		KprTextConcatString(self, xsToString(xsArg(0)));
	}
}

void KPR_text_end(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	KprTextEnd(self);
}

void KPR_text_endBlock(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	KprTextEndBlock(self);
}

void KPR_text_endSpan(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	KprTextEndSpan(self);
}

void KPR_text_format(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	KprStyle style;
	KprBehavior behavior = NULL;
	KprTextLink link;
	xsStringValue string;
	xsTry {
		xsVars(4);
		KprTextBegin(self);
		xsEnterSandbox();
		if (xsIsInstanceOf(xsArg(0), xsArrayPrototype)) {
			xsIntegerValue block, blocks = xsToInteger(xsGet(xsArg(0), xsID_length));
			for (block = 0; block < blocks; block++) {
				xsVar(0) = xsGetAt(xsArg(0), xsInteger(block));
				if (xsIsInstanceOf(xsVar(0), xsObjectPrototype)) {
					style = NULL;
					if (xsFindResult(xsVar(0), xsID_style)) {
						if (xsTest(xsResult))
							style = kprGetHostData(xsResult, style, style);
					}
					link = NULL;
					if (xsFindResult(xsVar(0), xsID_behavior)) {
						if (xsTest(xsResult)) {
							xsThrowIfFskErr(KprTextLinkNew(&link));
							xsThrowIfFskErr(KprScriptBehaviorNew(&behavior, (KprContent)link, the, &xsResult));
							link->behavior = behavior;
						}
					}
					KprTextBeginBlock(self, style, link);
					if (xsFindResult(xsVar(0), xsID_spans)) {
						xsVar(0) = xsResult;
						if (xsIsInstanceOf(xsVar(0), xsArrayPrototype)) {
							xsIntegerValue span, spans = xsToInteger(xsGet(xsVar(0), xsID_length));
							for (span = 0; span < spans; span++) {
								xsVar(1) = xsGetAt(xsVar(0), xsInteger(span));
								if (xsIsInstanceOf(xsVar(1), xsObjectPrototype)) {
									if (xsFindResult(xsVar(1), xsID_content)) {
										KprContent content = kprGetHostData(xsResult, content, content);
										UInt16 adjustment = kprTextLineMiddle;
										if (xsFindResult(xsVar(1), xsID_align)) {
											char* align = xsToString(xsResult);
											if (!FskStrCompare(align, "left"))
												adjustment = kprTextFloatLeft;
											else if (!FskStrCompare(align, "right"))
												adjustment = kprTextFloatRight;
											else if (!FskStrCompare(align, "top"))
												adjustment = kprTextLineTop;
											else if (!FskStrCompare(align, "middle"))
												adjustment = kprTextLineMiddle;
											else if (!FskStrCompare(align, "bottom"))
												adjustment = kprTextLineBottom;
										}
										KprTextConcatContent(self, content, adjustment);
									}
									else {
										style = NULL;
										if (xsFindResult(xsVar(1), xsID_style)) {
											if (xsTest(xsResult))
												style = kprGetHostData(xsResult, style, style);
										}
										link = NULL;
										if (xsFindResult(xsVar(1), xsID_Behavior)) {
											if (xsIsInstanceOf(xsResult, xsFunctionPrototype)) {
												xsVar(2) = xsResult;
												if (xsFindResult(xsVar(1), xsID("$"))) {
													xsVar(3) = xsResult;
												}
												else {
													xsVar(3) = xsUndefined;
												}
												xsOverflow(-5);
												fxPush(xsThis);
												fxPush(xsVar(3));
												fxPushCount(the, 2);
												fxPush(xsGlobal);
												fxPush(xsVar(2));
												fxNew(the);
												xsResult = fxPop();
											}
											if (xsIsInstanceOf(xsResult, xsObjectPrototype)) {
												xsThrowIfFskErr(KprTextLinkNew(&link));
												xsThrowIfFskErr(KprScriptBehaviorNew(&behavior, (KprContent)link, the, &xsResult));
												link->behavior = behavior;
											}
										}
										else if (xsFindResult(xsVar(1), xsID_behavior)) {
											if (xsTest(xsResult)) {
												xsThrowIfFskErr(KprTextLinkNew(&link));
												xsThrowIfFskErr(KprScriptBehaviorNew(&behavior, (KprContent)link, the, &xsResult));
												link->behavior = behavior;
											}
										}
										KprTextBeginSpan(self, style, link);
										if (xsFindString(xsVar(1), xsID_string, &string)) {
											KprTextConcatString(self, string);
										}
										KprTextEndSpan(self);
									}
								}
								else {
									KprTextConcatString(self, xsToString(xsVar(1)));
								}
							}
						}
					}
					else if (xsFindString(xsVar(0), xsID_string, &string)) {
						KprTextConcatString(self, string);
					}
					KprTextEndBlock(self);
				}
				else {
					KprTextConcatString(self, xsToString(xsVar(0)));
				}
			}
		}
		xsLeaveSandbox();
		KprTextEnd(self);
	}
	xsCatch {
		KprTextBegin(self);
		KprTextEnd(self);
	}
}

void KPR_text_insert(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	if ((xsToInteger(xsArgc) > 0) && xsTest(xsArg(0)))
		KprTextInsertString(self, xsToString(xsArg(0)));
	else
		KprTextInsertStringWithLength(self, "", 0);
}

void KPR_text_hitOffset(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	UInt32 offset = KprTextHitOffset(self, xsToInteger(xsArg(0)), xsToInteger(xsArg(1)));
	xsResult = xsInteger(offset);
}

void KPR_text_select(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	KprTextSelect(self, xsToInteger(xsArg(0)), xsToInteger(xsArg(1)));
}


