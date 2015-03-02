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
	/* 0 */ long offset; /* text */
	/* 4 */ long length; /* -1 */
	/* 8 */ KprContent content;
	/* C */ short adjustment;
	/* E */ short dummy;
#if defined(__LP64__)
	/* - */ short dummy2[2];
#endif
} KprTextItemRunRecord;

typedef struct {
	/* 0 */ long offset; /* text */
	/* 4 */ long length; /* >= 0 */
	/* 8 */ KprStyle style;
	/* C */ KprTextLink link;
} KprTextSpanRunRecord;

typedef union {
	KprTextItemRunRecord item;
	KprTextSpanRunRecord span;
} KprTextRunRecord, *KprTextRun;

typedef struct {
	/* 0 */ long offset; /* text */
	/* 4 */ KprStyle style;
	/* 8 */ long dummy;
	/* C */ long count;
} KprTextBlockRecord, *KprTextBlock;

typedef struct {
	/* 0 */ short y;
	/* 2 */ UInt16 ascent;
	/* 4 */ UInt16 descent;
	/* 6 */ short x;
	/* 8 */ short width;
	/* A */ short portion;
	/* C */ short slop;
	/* E */ short count;
#if defined(__LP64__)
	/* - */ long dummy[2];
#endif
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
	long textOffset;
	long length;

	FskGrowableArray blockBuffer;
	long blockOffset;
	long blockSize;
	
	FskGrowableArray lineBuffer;
	long lineCount;
	
	FskGrowableArray selectionBuffer;
	long from;
	long to;
	
	KprTextStateRecord stateRegister;
	FskGrowableArray stateBuffer;
	short stateIndex;

	short blockFlag;
	short styleFlag;

	FskPort port;
	long textWidth;
	long textHeight;
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
