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
#ifndef __KPRTEXTLAYOUT__
#define __KPRTEXTLAYOUT__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* adjusments */
enum { 
	kprTextAscent = 0,
	kprTextMiddle = 1,
	kprTextDescent = 2,
	kprTextLineTop = 3,
	kprTextLineMiddle = 4,
	kprTextLineBottom = 5,
	kprTextTextTop = 6,
	kprTextTextMiddle = 7,
	kprTextTextBottom = 8,
	kprTextFloatLeft = 9,
	kprTextFloatRight = 10
};

typedef struct {
	/*  0 -  0 */ SInt32 offset; /* text */
	/*  4 -  4 */ SInt32 length; /* -1 */
	/*  8 -  8 */ KprContent content;
	/*  C - 10 */ SInt16 adjustment;
	/*  E - 12 */ SInt16 dummy;
	/* 10 - 14 */ SInt16 dummy1;
#if defined(__LP64__)
	/* 12 - 16 */ SInt16 dummy2;
#endif
	/* 12 - 18 */
} KprTextItemRunRecord;

typedef struct {
	/*  0 -  0 */ SInt32 offset; /* text */
	/*  4 -  4 */ SInt32 length; /* >= 0 */
	/*  8 -  8 */ KprStyle style;
	/*  C - 10 */ KprTextLink link;
#if !defined(__LP64__)
	/* 10 - 18 */ SInt16 dummy;
#endif
	/* 12 - 18 */
} KprTextSpanRunRecord;

typedef union {
	KprTextItemRunRecord item;
	KprTextSpanRunRecord span;
} KprTextRunRecord, *KprTextRun;

typedef struct {
	/*  0 -  0 */ SInt32 offset; /* text */
	/*  4 -  4 */ SInt32 count;
	/*  8 -  8 */ KprStyle style;
	/*  C - 10 */ SInt16 dummies[3];
#if defined(__LP64__)
	/* 12 - 16 */ SInt16 dummy;
#endif
	/* 12 - 18 */
} KprTextBlockRecord, *KprTextBlock;

typedef struct {
	/*  0 -  0 */ SInt32 y;
	/*  4 -  4 */ UInt16 ascent;
	/* 	6 -  6 */ UInt16 descent;
	/*  8 -  8 */ SInt16 x;
	/*  A -  A */ SInt16 width;
	/*  C -  C */ SInt16 portion;
	/*  E -  E */ SInt16 slop;
	/* 10 - 10 */ SInt16 count;
#if defined(__LP64__)
	/* 12 - 12 */ SInt16 dummies[3];
#endif
	/* 12 - 18 */
} KprTextLineRecord, *KprTextLine;

typedef struct {
	KprStyle style;
	KprTextLink link;
} KprTextStateRecord, *KprTextState;

struct KprTextStruct {
	KprSlotPart;
	KprContentPart;
	KprContainerPart;
	
	FskGrowableStorage textBuffer;
	SInt32 textOffset;
	SInt32 length;

	FskGrowableArray blockBuffer;
	SInt32 blockOffset;
	SInt32 blockSize;
	
	FskGrowableArray lineBuffer;
	SInt32 lineCount;
	
	FskGrowableArray selectionBuffer;
	SInt32 from;
	SInt32 to;
	
	KprTextStateRecord stateRegister;
	FskGrowableArray stateBuffer;
	SInt16 stateIndex;

	SInt16 blockFlag;
	SInt16 styleFlag;

	FskPort port;
	SInt32 textWidth;
	SInt32 textHeight;
	UInt32 graphicsMode;
	FskGraphicsModeParameters graphicsModeParameters;
};

FskAPI(FskErr) KprTextNew(KprText* it,  KprCoordinates coordinates, KprSkin skin, KprStyle style, char* text);
FskAPI(FskErr) KprTextBegin(KprText self);
FskAPI(FskErr) KprTextBeginBlock(KprText self, KprStyle style, KprTextLink link);
FskAPI(FskErr) KprTextBeginSpan(KprText self, KprStyle style, KprTextLink link);
FskAPI(FskErr) KprTextConcatContent(KprText self, KprContent content, UInt16 adjustment);
FskAPI(FskErr) KprTextConcatString(KprText self, char* string);
FskAPI(FskErr) KprTextConcatText(KprText self, char* text, SInt32 length);
FskAPI(FskErr) KprTextEnd(KprText self);
FskAPI(FskErr) KprTextEndBlock(KprText self);
FskAPI(FskErr) KprTextEndSpan(KprText self);
FskAPI(void) KprTextFlagged(KprText self);
FskAPI(void) KprTextGetSelectionBounds(KprText self, FskRectangle bounds);
UInt32 KprTextHitOffset(KprText self, SInt32 x, SInt32 y);
FskAPI(FskErr) KprTextInsertString(KprText self, char* string);
FskAPI(FskErr) KprTextInsertStringWithLength(KprText self, char* text, SInt32 length);
FskAPI(FskErr) KprTextSelect(KprText self, SInt32 selectionOffset, UInt32 selectionLength);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
