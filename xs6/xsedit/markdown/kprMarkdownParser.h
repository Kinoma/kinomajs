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
#ifndef __KPRMARKDOWNPARSER__
#define __KPRMARKDOWNPARSER__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define kprMarkdownColorMask 0x3
#define kprMarkdownColorMixFlag 0x40
#define kprMarkdownColorMixShift 2

// color 0: text (black)
#define kprMarkdownTextColor 0x0
#define kprMarkdownTextColorMix 0x40
// color 1: line marker / line markup (blue) *
#define kprMarkdownLineMarkerColor 0x1
#define kprMarkdownLineMarkupColor 0x1
// color 2: span marker / span markup (red)
#define kprMarkdownSpanMarkerColor 0x2
#define kprMarkdownSpanMarkupColor 0x2
// color 3: code / code span / text span (green) **
#define kprMarkdownCodeColor 0x3
#define kprMarkdownCodeColorMix 0x4C
#define kprMarkdownCodeSpanColor 0x3
#define kprMarkdownCodeSpanColorMix 0x4C
#define kprMarkdownTextSpanColor 0x3
#define kprMarkdownTextSpanColorMix 0x4C
//  * note: lines with no span items are color 1 (blue)
// ** note: spans with no text items are color 3 (green)

#ifdef mxDebug
#define kprMarkdownDebug 0x1
#define kprMarkdownDebugInline 0x2
#define kprMarkdownDebugInlineColorDepth 0x8
#define kprMarkdownDebugSummaryInfo 0x4
#endif

#define kprMarkdownParseAll 0x1
#define kprMarkdownParseElements 0x2
#define kprMarkdownParseToFirstHeader 0x4

#define kprMarkdownDefaultAttributesOption 1
#define kprMarkdownDefaultElementsOption 2
#define kprMarkdownParserDefaultOption 512
#define kprMarkdownParserDefaultTab 4

typedef enum {
	kprMarkdownA = 1,
	kprMarkdownB = 2,
	kprMarkdownBLOCKQUOTE = 3,
	kprMarkdownBR = 4,
	kprMarkdownCODE = 5,
	kprMarkdownDEL = 6,
	kprMarkdownDIV = 7,
	kprMarkdownEM = 8,
	kprMarkdownH1 = 9,
	kprMarkdownH2 = 10,
	kprMarkdownH3 = 11,
	kprMarkdownH4 = 12,
	kprMarkdownH5 = 13,
	kprMarkdownH6 = 14,
	kprMarkdownHR = 15,
	kprMarkdownI = 16,
	kprMarkdownIFRAME = 17,
	kprMarkdownIMG = 18,
	kprMarkdownLI = 19,
	kprMarkdownOL = 20,
	kprMarkdownP = 21,
	kprMarkdownPRE = 22,
	kprMarkdownSPAN = 23,
	kprMarkdownSTRIKE = 24,
	kprMarkdownSTRONG = 25,
	kprMarkdownTABLE = 26,
	kprMarkdownTBODY = 27,
	kprMarkdownTD = 28,
	kprMarkdownTH = 29,
	kprMarkdownTHEAD = 30,
	kprMarkdownTR = 31,
	kprMarkdownUL = 32,
} KprMarkdownElementType;

typedef struct KprMarkdownElementStruct KprMarkdownElementRecord, *KprMarkdownElement;
typedef struct KprMarkdownAttributeStruct KprMarkdownAttributeRecord, *KprMarkdownAttribute;

struct KprMarkdownAttributeStruct {
	struct {
		SInt16 length;
		SInt16 offset; // relative offset
	} n; // name
	struct {
		SInt16 length; // -1 no value
		SInt16 offset; // relative offset
	} v; // value
};

struct KprMarkdownElementStruct {
	KprMarkdownElementType type;
	KprMarkdownAttribute attributes;
	KprMarkdownElement elements;
	struct {
		SInt16 length; // -1 no text
		SInt16 offset; // relative offset
	} t; // text
};

typedef struct KprMarkdownParserStruct KprMarkdownParserRecord, *KprMarkdownParser;
typedef struct KprMarkdownStateStruct KprMarkdownStateRecord, *KprMarkdownState;
typedef struct KprMarkdownRunStruct KprMarkdownRunRecord, *KprMarkdownRun;

typedef enum {
	kprMarkdownBlankLine = 1, // empty
	kprMarkdownBlockquoteLine = 2,
	kprMarkdownCodeSpan = 3, // empty
	kprMarkdownDoubleDash = 4, // empty
	kprMarkdownFencedCodeBeginLine = 5, // empty
	kprMarkdownFencedCodeEndLine = 6, // empty
	kprMarkdownFencedCodeLine = 7, // empty
	kprMarkdownFontStyle = 8, // emphasis | strikethrough | strong
	kprMarkdownHeaderLine = 9,
	kprMarkdownHeaderZeroLine = 10, // empty
	kprMarkdownHorizontalRuleLine = 11, // empty
	kprMarkdownHTMLEntity = 12, // empty
	kprMarkdownImageReferenceSpan = 13, // empty
	kprMarkdownImageSpan = 14, // empty
	kprMarkdownIndentedCodeLine = 15, // empty
	kprMarkdownLinkReferenceSpan = 16,
	kprMarkdownLinkSpan = 17,
	kprMarkdownListLine = 18,
	kprMarkdownMarkupBreak = 19, // empty
	kprMarkdownMarkupLine = 20, // empty
	kprMarkdownMarkupStyleSpan = 21,
	kprMarkdownParagraphLine = 22,
	kprMarkdownReferenceDefinitionLine = 23, // empty
	kprMarkdownTableColumnDivider = 24,
	kprMarkdownTableLine = 25, // empty with table optimization
	kprMarkdownTextSpan = 26,
} KprMarkdownType;

typedef enum {
	kprMarkdownEmphasis = 0x1,
	kprMarkdownStrikethrough = 0x4,
	kprMarkdownStrong = 0x2,
} KprMarkdownFontStyle;

struct KprMarkdownParserStruct {
//	KprMarkdownElement elements;
	KprMarkdownRun runs;
	SInt32 codeRunByte;
	SInt32 columnCount;
	SInt32 columnIndex;
	SInt32 fontStyle; // 0x0 = normal, 0x1 = italic, 0x2 = bold, 0x4 = strike
	SInt32 lineCount;
	SInt32 referenceDefinitionCount;
	SInt32 tab;
};

struct KprMarkdownRunStruct {
	KprMarkdownType type;
	SInt16 count;
	SInt16 index;
	SInt32 length;
	SInt32 offset;
	SInt8 colors;
	SInt8 marker;
	SInt8 shaper; // final line item flag, font style, header level, list flag, table flag
	union {
		UInt32 codepoints[2];
		KprMarkdownElement element;
		struct {
			struct {
				SInt16 length; // -1 no text
				SInt16 offset; // relative offset
			} t; // text
			struct {
				SInt16 length; // -1 no value
				SInt16 offset; // relative offset
			} v; // value
		} s;
	} u;
};

#define KPRMARKDOWNSTACKSIZE 256

struct KprMarkdownStateStruct {
	KprMarkdownAttributeRecord attributeRecord;
	KprMarkdownElementRecord elementRecord;
	KprMarkdownElement stack[KPRMARKDOWNSTACKSIZE];
	char *elementStart;
	char *elementStop;
	char *textStart;
	char *textStop;
	char *holdPC;
	char *markPC;
	char *pc;
	SInt32 depth;
	SInt32 flags;
	SInt32 holdLine;
	SInt32 indent;
	SInt32 line;
	SInt32 marker;
	SInt32 offset;
	SInt32 spaces;
	SInt32 tableRow;
	SInt32 top;
};

FskErr KprMarkdownElementDispose(KprMarkdownParser parser, KprMarkdownElement element);
FskErr KprMarkdownElementsDispose(KprMarkdownParser parser, KprMarkdownElement elements);
FskErr KprMarkdownParserDispose(KprMarkdownParser parser);
FskErr KprMarkdownParserNew(KprMarkdownParser *parserReference, SInt32 option, SInt32 tab);

FskErr KprMarkdownParse(KprMarkdownParser parser, char *str, SInt32 offset, SInt32 length, SInt32 flags);
FskErr KprMarkdownParseInline(KprMarkdownParser parser, char *str, SInt32 offset, SInt32 length, KprMarkdownRun inlineRun);

#ifdef mxDebug
FskErr KprMarkdownPrintDebugInfo(KprMarkdownParser parser, char *str, SInt32 flags);
FskErr KprMarkdownPrintDebugInfoInline(KprMarkdownParser parser, char *str, KprMarkdownRun inlineRun, SInt32 depth, SInt32 flags);
#endif

typedef struct KprMarkdownWriteStackStruct KprMarkdownWriteStackRecord, *KprMarkdownWriteStack;

struct KprMarkdownWriteStackStruct {
	char *attr;
	char *name;
	SInt32 marker;
};

typedef struct KprMarkdownWriteStateStruct KprMarkdownWriteStateRecord, *KprMarkdownWriteState;

struct KprMarkdownWriteStateStruct {
	KprMarkdownWriteStackRecord stack[KPRMARKDOWNSTACKSIZE];
	SInt32 top;
};

#ifdef mxDebug
FskErr KprMarkdownWriteHTML(KprMarkdownParser parser, char *str, FILE *stream);
FskErr KprMarkdownWriteHTMLCode(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownWriteState state, SInt32 *index, SInt32 count);
FskErr KprMarkdownWriteHTMLInline(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownRun inlineRun);
FskErr KprMarkdownWriteHTMLInlineText(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownRun inlineRun);
FskErr KprMarkdownWriteHTMLPop(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownWriteState state, SInt32 flag);
FskErr KprMarkdownWriteHTMLPush(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownWriteState state, SInt32 flag, char *name, char *attr);
FskErr KprMarkdownWriteHTMLTable(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownWriteState state, SInt32 *index, SInt32 count);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPRMARKDOWNPARSER__ */
