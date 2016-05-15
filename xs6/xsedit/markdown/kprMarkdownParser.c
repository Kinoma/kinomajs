
#line 1 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
// !!! RAGEL GENERATED .C FILE - DO NOT EDIT !!! //

// http://www.colm.net/files/ragel/ragel-6.9.tar.gz

/*

Generate & Build

  ragel -C -T0 xs6/xsedit/markdown/kprMarkdownParser.rl -o xs6/xsedit/markdown/kprMarkdownParser.c
  kprconfig6 -d -m $F_HOME/xs6/xsedit/manifest.json

Visualizations

  ragel -C -T0 -S md -V xs6/xsedit/markdown/kprMarkdownParser.rl > xs6/xsedit/markdown/kprMarkdownParser-md.dot
  ragel -C -T0 -S md_inline -V xs6/xsedit/markdown/kprMarkdownParser.rl > xs6/xsedit/markdown/kprMarkdownParser-md_inline.dot

  dot xs6/xsedit/markdown/kprMarkdownParser-md.dot -Tpng > xs6/xsedit/markdown/kprMarkdownParser-md.png
  dot xs6/xsedit/markdown/kprMarkdownParser-md_inline.dot -Tpng > xs6/xsedit/markdown/kprMarkdownParser-md_inline.png

  ragel -C -T0 -M error -S md -V xs6/xsedit/markdown/kprMarkdownParser.rl > xs6/xsedit/markdown/kprMarkdownParser-md-error.dot
  ragel -C -T0 -M fenced_code_scanner -S md -V xs6/xsedit/markdown/kprMarkdownParser.rl > xs6/xsedit/markdown/kprMarkdownParser-md-fenced_code_scanner.dot
  ragel -C -T0 -M inner -S md -V xs6/xsedit/markdown/kprMarkdownParser.rl > xs6/xsedit/markdown/kprMarkdownParser-md-inner.dot
  ragel -C -T0 -M markdown -S md -V xs6/xsedit/markdown/kprMarkdownParser.rl > xs6/xsedit/markdown/kprMarkdownParser-md-markdown.dot

  dot xs6/xsedit/markdown/kprMarkdownParser-md-error.dot -Tpng > xs6/xsedit/markdown/kprMarkdownParser-md-error.png
  dot xs6/xsedit/markdown/kprMarkdownParser-md-fenced_code_scanner.dot -Tpng > xs6/xsedit/markdown/kprMarkdownParser-md-fenced_code_scanner.png
  dot xs6/xsedit/markdown/kprMarkdownParser-md-inner.dot -Tpng > xs6/xsedit/markdown/kprMarkdownParser-md-inner.png
  dot xs6/xsedit/markdown/kprMarkdownParser-md-markdown.dot -Tpng > xs6/xsedit/markdown/kprMarkdownParser-md-markdown.png

*/

#include "kprMarkdownParser.h"

#if TARGET_OS_WIN32
#define _IsSpace(c) (iswspace(c))
#define _IsPunct(c) (iswpunct(c))
#else
#define _IsSpace(c) (isspace(c))
#define _IsPunct(c) (ispunct(c))
#endif

char* _FontStyles[] = {
/* 0 */ "normal",
/* 1 */ "italic",
/* 2 */ "bold",
/* 3 */ "bold italic",
/* 4 */ "strike",
/* 5 */ "strike italic",
/* 6 */ "strike bold",
/* 7 */ "strike bold italic",
	NULL
};

static Boolean _blank(char *p, char *pe, SInt32 length);
static SInt32 _column(char *p, char *pe, SInt32 indent);
static SInt32 _indent(char *p, char *pe, SInt32 spaces, Boolean flag);
static SInt32 _spaces(char *p, char *pe, SInt32 indent, Boolean flag);

static Boolean _blank(char *p, char *pe, SInt32 length)
{
	if (pe == NULL)
		pe = p + length;
	else if (length == -1)
		length = pe - p;
	while ((p < pe) && (length > 0)) {
		if (!_IsSpace(*p))
			return false;
		length--;
		p++;
	}
	return true;
}

static SInt32 _column(char *p, char *pe, SInt32 indent)
{
	return _spaces(p, pe, indent, 1);
}

static SInt32 _indent(char *p, char *pe, SInt32 spaces, Boolean flag)
{
	SInt32 indent = 0, s = 0;
	while ((p < pe) && (s < spaces)) {
		if (*p == '\t')
			s += (4 - (s % 4));
		else if (*p == ' ')
			s++;
		else if (flag)
			s++;
		indent++;
		p++;
	}
	return indent;
}

static SInt32 _spaces(char *p, char *pe, SInt32 indent, Boolean flag)
{
	SInt32 spaces = 0;
	if (indent == -1)
		indent = pe - p;
	while ((p < pe) && (indent > 0)) {
		if (*p == '\t')
			spaces += (4 - (spaces % 4));
		else if (*p == ' ')
			spaces++;
		else if (flag)
			spaces++;
		indent--;
		p++;
	}
	return spaces;
}

#define KPRMARKDOWNDEBUGMARKDOWN 0
#define KPRMARKDOWNDEBUGMARKUP 0
#define KPRMARKDOWNDEBUGTEXT 0

// http://ascii-table.com/ansi-escape-sequences.php

#define KNRM  "\x1B[0m"
#define KBLK  "\x1B[30m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"

typedef struct KprMarkdownElementInfoStruct KprMarkdownElementInfoRecord, *KprMarkdownElementInfo;

struct KprMarkdownElementInfoStruct {
	char *name;
	SInt16 length;
	SInt16 state; // 0x0 = empty element, 0x1 = inner text expected, 0x2 = inner elements expected, 0x4 = special
	SInt32 type;
};

// https://developer.mozilla.org/en-US/docs/Glossary/empty_element

static KprMarkdownElementInfoRecord kprMarkdownElementInfo[] = {
	{ "a",           1, 0x1, kprMarkdownA          },
	{ "b",           1, 0x1, kprMarkdownB          }, // bold
	{ "i",           1, 0x1, kprMarkdownI          }, // italic
	{ "p",           1, 0x3, kprMarkdownP          },
	{ "br",          2, 0x0, kprMarkdownBR         },
	{ "em",          2, 0x1, kprMarkdownEM         }, // italic
	{ "h1",          2, 0x1, kprMarkdownH1         },
	{ "h2",          2, 0x1, kprMarkdownH2         },
	{ "h3",          2, 0x1, kprMarkdownH3         },
	{ "h4",          2, 0x1, kprMarkdownH4         },
	{ "h5",          2, 0x1, kprMarkdownH5         },
	{ "h6",          2, 0x1, kprMarkdownH6         },
	{ "hr",          2, 0x0, kprMarkdownHR         },
	{ "li",          2, 0x3, kprMarkdownLI         },
	{ "ol",          2, 0x3, kprMarkdownOL         },
	{ "td",          2, 0x3, kprMarkdownTD         },
	{ "th",          2, 0x3, kprMarkdownTH         },
	{ "tr",          2, 0x2, kprMarkdownTR         },
	{ "ul",          2, 0x3, kprMarkdownUL         },
	{ "del",         3, 0x1, kprMarkdownDEL        }, // strikethrough
	{ "div",         3, 0x3, kprMarkdownDIV        },
	{ "img",         3, 0x0, kprMarkdownIMG        },
	{ "pre",         3, 0x3, kprMarkdownPRE        },
	{ "sup",         3, 0x1, kprMarkdownSUP        },
	{ "code",        4, 0x1, kprMarkdownCODE       },
	{ "span",        4, 0x1, kprMarkdownSPAN       },
	{ "table",       5, 0x2, kprMarkdownTABLE      },
	{ "tbody",       5, 0x2, kprMarkdownTBODY      },
	{ "thead",       5, 0x2, kprMarkdownTHEAD      },
	{ "iframe",      6, 0x4, kprMarkdownIFRAME     },
	{ "strike",      6, 0x1, kprMarkdownSTRIKE     }, // strikethrough
	{ "strong",      6, 0x1, kprMarkdownSTRONG     }, // bold
	{ "blockquote", 10, 0x3, kprMarkdownBLOCKQUOTE },
	{ NULL }
};

static KprMarkdownElementInfo getElementInfoByName(char *nameStart, char *nameStop);
static KprMarkdownElementInfo getElementInfoByType(KprMarkdownElementType type);

static FskErr doAttribute(KprMarkdownParser parser, char *str, KprMarkdownState state)
{
	FskErr err = kFskErrNone;
	if (state->flags & kprMarkdownParseElements) {
		KprMarkdownElement element = state->stack[state->top];
		if (element->attributes == NULL) {
			bailIfError(FskGrowableArrayNew(sizeof(KprMarkdownAttributeRecord), kprMarkdownDefaultAttributesOption, (FskGrowableArray*)&(element->attributes)));
		}
		bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)element->attributes, (void *)&(state->attributeRecord)));
	}
bail:
	return err;
}

static FskErr doAttributeName(KprMarkdownParser parser, char *str, KprMarkdownState state)
{
	if (state->flags & kprMarkdownParseElements) {
		state->attributeRecord.n.length = state->textStop - state->textStart;
		state->attributeRecord.n.offset = (SInt16)(state->textStart - str - state->offset);
		state->attributeRecord.v.length = -1;
		state->attributeRecord.v.offset = 0;
	}
	return kFskErrNone;
}

static FskErr doAttributeValue(KprMarkdownParser parser, char *str, KprMarkdownState state)
{
	if (state->flags & kprMarkdownParseElements) {
		state->attributeRecord.v.length = state->textStop - state->textStart - 2;
		state->attributeRecord.v.offset = (SInt16)(state->textStart - str - state->offset + 1);
	}
	return kFskErrNone;
}

static FskErr doEnterElement(KprMarkdownParser parser, char *str, KprMarkdownState state)
{
	FskErr err = kFskErrNone;
	KprMarkdownElement element = NULL;
	if (state->flags & kprMarkdownParseElements) {
		KprMarkdownElementInfo info = getElementInfoByName(state->elementStart, state->elementStop);
		if (state->top == -1) {
			bailIfError(FskMemPtrNewClear(sizeof(KprMarkdownElementRecord), (FskMemPtr *)&element));
			element->n.length = state->elementStop - state->elementStart;
			element->n.offset = (SInt16)(state->elementStart - str - state->offset);
			element->t.length = -1;
			element->t.offset = 0;
			element->type = info ? info->type : -1;
		}
		else {
			element = state->stack[state->top];
			if (element->elements == NULL) {
				bailIfError(FskGrowableArrayNew(sizeof(KprMarkdownElementRecord), kprMarkdownDefaultElementsOption, (FskGrowableArray*)&(element->elements)));
			}
			state->elementRecord.n.length = state->elementStop - state->elementStart;
			state->elementRecord.n.offset = (SInt16)(state->elementStart - str - state->offset);
			state->elementRecord.type = info ? info->type : -1;
			bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)element->elements, (void *)&(state->elementRecord)));
			FskGrowableArrayGetPointerToItem((FskGrowableArray)element->elements, FskGrowableArrayGetItemCount((FskGrowableArray)element->elements) - 1, (void **)&element);
			state->elementRecord.n.length = -1;
			state->elementRecord.n.offset = 0;
		}
	}
	state->depth++;
	state->top++;
	FskAssert(state->top < KPRMARKDOWNSTACKSIZE);
	state->stack[state->top] = element;
	#if KPRMARKDOWNDEBUGMARKUP
		fprintf(stdout, KRED "%6d <", (int)state->line);
		fwrite(state->elementStart, state->elementStop - state->elementStart, 1, stdout);
		fprintf(stdout, ">\n" RESET);
	#endif
bail:
	return err;
}

static FskErr doExitElement(KprMarkdownParser parser, char *str, KprMarkdownState state)
{
	state->depth--;
	state->top--;
	#if KPRMARKDOWNDEBUGMARKUP
		fprintf(stdout, KRED "%6d </", (int)state->line);
		fwrite(state->elementStart, state->elementStop - state->elementStart, 1, stdout);
		fprintf(stdout, ">\n" RESET);
	#endif
	return kFskErrNone;
}

static FskErr doHold(KprMarkdownParser parser, char *str, KprMarkdownState state, char *pc, SInt32 line)
{
	state->depth = 0;
	state->holdLine = line;
	state->holdPC = pc;
	state->markPC = NULL;
	state->offset = pc - str;
	state->top = -1;
	return kFskErrNone;
}

static FskErr doMark(KprMarkdownParser parser, char *str, KprMarkdownState state, char *pc)
{
	state->markPC = pc;
	return kFskErrNone;
}

static FskErr doProcessMarkup(KprMarkdownParser parser, char *str, KprMarkdownState state, char *stop, SInt32 shaper)
{
	FskErr err = kFskErrNone;
	SInt32 line = state->holdLine;
	KprMarkdownElement element = NULL;
	char *pc = state->holdPC;
	if (pc == NULL) { // markup error on the first line
		pc = state->holdPC = state->pc;
	}
	if (shaper == -1) {
		KprMarkdownElementDispose(parser, state->stack[0]);
	}
	else if (state->top == -1) {
		element = state->stack[0];
	}
	state->stack[0] = NULL;
	while (pc < stop) {
		char c = *pc;
		if ((c == '\n') || (c == '\0')) {
			KprMarkdownRunRecord runRecord;
			KprMarkdownRun run = &runRecord;
			FskMemSet(run, 0, sizeof(runRecord));
			runRecord.type = kprMarkdownMarkupLine;
			runRecord.colors = kprMarkdownLineMarkupColor;
			runRecord.length = pc + 1 - state->holdPC;
			runRecord.offset = state->holdPC - str;
			runRecord.marker = 0;
			runRecord.shaper = shaper;
			runRecord.u.element = element;
			bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
			if (shaper == 1) {
				shaper = 2;
			}
			if (c == '\n') {
				state->holdPC = pc + 1;
				state->markPC = NULL;
				line++;
			}
			else {
				state->holdPC = NULL;
				state->markPC = NULL;
			}
			element = NULL;
		}
		pc++;
	}
bail:
	return err;
}

static FskErr doProcessText(KprMarkdownParser parser, char *str, KprMarkdownState state, char *stop, SInt32 line)
{
	FskErr err = kFskErrNone;
	SInt32 length = state->textStop - state->textStart, space = 0;
	if (state->depth > 0) {
		char *text = state->textStart;
		while (text < state->textStop) {
			if (!_IsSpace(*text))
				break;
			space++;
			text++;
		}
	}
	if ((length > space) || (state->depth == 0)) {
		if (state->depth > 0) {
			KprMarkdownElement element = state->stack[state->top];
			if (element) {
				if (element->elements == NULL) {
					bailIfError(FskGrowableArrayNew(sizeof(KprMarkdownElementRecord), kprMarkdownDefaultElementsOption, (FskGrowableArray*)&(element->elements)));
				}
				state->elementRecord.type = 0;
				state->elementRecord.t.length = state->textStop - state->textStart;
				state->elementRecord.t.offset = (SInt16)(state->textStart - str - state->offset);
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)element->elements, (void *)&(state->elementRecord)));
				state->elementRecord.t.length = -1;
				state->elementRecord.t.offset = 0;
			}
		}
		#if KPRMARKDOWNDEBUGTEXT
			fprintf(stdout, (state->depth ? KYEL "%6d " : KNRM "%6d "), (int)line);
			if (state->textStart) {
				fwrite(state->textStart, state->textStop - state->textStart, 1, stdout);
			}
			fprintf(stdout, "\n" RESET);
		#endif
	}
bail:
	return err;
}

static FskErr doProcessTextSpecial(KprMarkdownParser parser, char *str, KprMarkdownState state, char *stop, SInt32 line)
{
	if (state->stack[0]) {
		state->stack[0]->t.length = state->textStop - state->textStart;
		state->stack[0]->t.offset = (SInt16)(state->textStart - str - state->offset);
	}
	return kFskErrNone;
}

static FskErr doResetState(KprMarkdownParser parser, char *str, KprMarkdownState state, SInt32 flags)
{
	state->elementStart = NULL;
	state->elementStop = NULL;
	state->textStart = NULL;
	state->textStop = NULL;
	state->depth = 0;
	state->indent = 0;
	state->offset = 0;
	state->spaces = 0;
	state->stack[0] = NULL;
	state->top = -1;
	if (flags & 0x4) {
		FskMemSet(&state->attributeRecord, 0, sizeof(state->attributeRecord));
		FskMemSet(&state->elementRecord, 0, sizeof(state->elementRecord));
		state->flags = 0;
		state->holdLine = 0;
		state->holdPC = NULL;
		state->markPC = NULL;
		if (flags & 0x8)
			state->line = 0;
		else
			state->line = 1;
		state->pc = NULL;
	}
	state->elementRecord.n.length = -1;
	state->elementRecord.n.offset = 0;
	state->elementRecord.t.length = -1;
	state->elementRecord.t.offset = 0;
	if (flags & 0x2)
		state->marker = 0;
	if (flags & 0x1)
		state->tableRow = 0;
	return kFskErrNone;
}

static FskErr doSetStateFlags(KprMarkdownParser parser, char *str, KprMarkdownState state, SInt32 flags)
{
	state->flags = flags;
	return kFskErrNone;
}

static FskErr doSetStateOffset(KprMarkdownParser parser, char *str, KprMarkdownState state, SInt32 offset)
{
	state->offset = offset;
	return kFskErrNone;
}

static FskErr doStartComment(KprMarkdownParser parser, char *str, KprMarkdownState state, char *start)
{
	return kFskErrNone;
}

static FskErr doStartElement(KprMarkdownParser parser, char *str, KprMarkdownState state, char *start)
{
	state->elementStart = start;
	state->elementStop = NULL;
	return kFskErrNone;
}

static FskErr doStartText(KprMarkdownParser parser, char *str, KprMarkdownState state, char *start)
{
	state->textStart = start;
	state->textStop = NULL;
	return kFskErrNone;
}

static FskErr doStopComment(KprMarkdownParser parser, char *str, KprMarkdownState state, char *stop)
{
	FskErr err = kFskErrNone;
	KprMarkdownElement element = NULL;
	if (state->top == -1) {
		bailIfError(FskMemPtrNewClear(sizeof(KprMarkdownElementRecord), (FskMemPtr *)&element));
		element->n.length = -1;
		element->n.offset = 0;
		element->t.length = state->textStop - state->textStart;
		element->t.offset = (SInt16)(state->textStart - str - state->offset);
		element->type = 0;
		state->stack[0] = element;
	}
	state->depth = -1;
	#if KPRMARKDOWNDEBUGMARKUP
		fprintf(stdout, KCYN "%6d ", (int)state->holdLine);
		fwrite(state->textStart, state->textStop - state->textStart, 1, stdout);
		fprintf(stdout, "\n" RESET);
	#endif
bail:
	return err;
}

static FskErr doStopElement(KprMarkdownParser parser, char *str, KprMarkdownState state, char *stop)
{
	state->elementStop = stop;
	return kFskErrNone;
}

static FskErr doStopText(KprMarkdownParser parser, char *str, KprMarkdownState state, char *stop)
{
	state->textStop = stop;
	return kFskErrNone;
}

static KprMarkdownElementInfo getElementInfoByName(char *nameStart, char *nameStop)
{
	KprMarkdownElementInfo info = kprMarkdownElementInfo;
	SInt32 length = nameStop - nameStart;
	while (info && info->name) {
		if (info->length == length)
			if (FskStrCompareCaseInsensitiveWithLength(nameStart, info->name, length) == 0)
				return info;
		info++;
	}
	return NULL;
}

static KprMarkdownElementInfo getElementInfoByType(KprMarkdownElementType type)
{
	KprMarkdownElementInfo info = kprMarkdownElementInfo;
	while (info && info->name) {
		if (info->type == type)
			return info;
		info++;
	}
	return NULL;
}

FskErr KprMarkdownElementDispose(KprMarkdownParser parser, KprMarkdownElement element)
{
	if (element) {
		FskGrowableArrayDispose((FskGrowableArray)element->attributes);
		KprMarkdownElementsDispose(parser, element->elements);
		FskMemPtrDispose(element);
	}
	return kFskErrNone;
}

FskErr KprMarkdownElementsDispose(KprMarkdownParser parser, KprMarkdownElement elements)
{
	if (elements) {
		KprMarkdownElement element = NULL;
		SInt32 count = FskGrowableArrayGetItemCount((FskGrowableArray)elements), index;
		for (index = 0; index < count; index++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, index, (void **)&element);
			FskGrowableArrayDispose((FskGrowableArray)element->attributes);
			KprMarkdownElementsDispose(parser, element->elements);
		}
		FskGrowableArrayDispose((FskGrowableArray)elements);
	}
	return kFskErrNone;
}

FskErr KprMarkdownParserDispose(KprMarkdownParser parser)
{
	if (parser) {
		if (parser->runs) {
			KprMarkdownRun run = NULL;
			SInt32 count = (SInt32)FskGrowableArrayGetItemCount((FskGrowableArray)parser->runs), index;
			for (index = 0; index < count; index++) {
				FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, index, (void **)&run);
				if (((run->type == kprMarkdownMarkupLine) || (run->type == kprMarkdownMarkupSpan)) && run->u.element) {
					KprMarkdownElementDispose(parser, run->u.element);
				}
			}
			FskGrowableArrayDispose((FskGrowableArray)parser->runs);
		}
		FskMemPtrDispose(parser);
	}
	return kFskErrNone;
}

FskErr KprMarkdownParserNew(KprMarkdownParser *parserReference, SInt32 option, SInt32 tab)
{
	FskErr err = kFskErrNone;
	bailIfError(FskMemPtrNewClear(sizeof(KprMarkdownParserRecord), (FskMemPtr *)parserReference));
	bailIfError(FskGrowableArrayNew(sizeof(KprMarkdownRunRecord), option ? option : kprMarkdownParserDefaultOption, (FskGrowableArray*)&((*parserReference)->runs)));
	(*parserReference)->tab = tab;
bail:
	return err;
}


#line 584 "xs6/xsedit/markdown/kprMarkdownParser.c"
static const char _md_actions[] = {
	0, 1, 3, 1, 5, 1, 6, 1, 
	9, 1, 10, 1, 11, 1, 13, 1, 
	15, 1, 16, 1, 18, 1, 19, 1, 
	26, 1, 27, 1, 28, 1, 29, 1, 
	35, 1, 38, 1, 39, 1, 59, 1, 
	60, 2, 5, 37, 2, 7, 9, 2, 
	11, 15, 2, 11, 18, 2, 11, 19, 
	2, 12, 9, 2, 12, 11, 2, 13, 
	4, 2, 14, 1, 2, 15, 6, 2, 
	15, 41, 2, 15, 60, 2, 18, 17, 
	2, 18, 55, 2, 18, 56, 2, 19, 
	17, 2, 19, 55, 2, 19, 56, 2, 
	20, 15, 2, 21, 15, 2, 22, 11, 
	2, 24, 11, 2, 24, 34, 2, 25, 
	11, 2, 25, 26, 2, 25, 28, 2, 
	26, 11, 2, 28, 11, 2, 32, 11, 
	2, 33, 11, 3, 6, 7, 9, 3, 
	11, 18, 17, 3, 11, 19, 17, 3, 
	12, 11, 19, 3, 13, 4, 3, 3, 
	13, 5, 37, 3, 14, 2, 0, 3, 
	14, 8, 36, 3, 15, 30, 42, 3, 
	15, 30, 43, 3, 15, 31, 21, 3, 
	15, 31, 58, 3, 20, 15, 57, 3, 
	20, 21, 15, 3, 22, 24, 11, 3, 
	22, 28, 11, 3, 24, 22, 11, 3, 
	25, 11, 18, 3, 25, 11, 19, 3, 
	25, 28, 11, 3, 26, 11, 18, 3, 
	26, 11, 19, 3, 28, 11, 18, 3, 
	32, 22, 11, 3, 33, 22, 11, 3, 
	35, 25, 11, 3, 40, 18, 11, 3, 
	40, 18, 51, 3, 40, 18, 52, 3, 
	40, 18, 54, 3, 40, 19, 11, 4, 
	12, 11, 18, 17, 4, 12, 11, 19, 
	17, 4, 14, 1, 2, 0, 4, 14, 
	2, 0, 3, 4, 15, 6, 7, 9, 
	4, 20, 18, 55, 15, 4, 20, 19, 
	55, 15, 4, 22, 24, 28, 11, 4, 
	24, 34, 26, 11, 4, 25, 28, 11, 
	18, 4, 40, 11, 18, 54, 4, 40, 
	18, 51, 27, 4, 40, 18, 52, 27, 
	4, 40, 19, 46, 23, 4, 40, 23, 
	19, 51, 4, 40, 23, 19, 52, 4, 
	40, 23, 19, 54, 4, 40, 25, 18, 
	48, 4, 40, 27, 18, 45, 4, 40, 
	27, 18, 47, 4, 40, 27, 18, 49, 
	4, 40, 27, 18, 53, 4, 40, 29, 
	18, 50, 5, 14, 1, 2, 0, 3, 
	5, 40, 19, 44, 23, 11, 5, 40, 
	23, 19, 11, 54, 5, 40, 23, 19, 
	25, 48, 5, 40, 23, 19, 27, 49, 
	5, 40, 23, 19, 27, 53, 5, 40, 
	23, 19, 29, 50, 5, 40, 23, 19, 
	51, 27, 5, 40, 23, 19, 52, 27, 
	5, 40, 25, 18, 48, 27, 5, 40, 
	26, 27, 18, 47, 5, 40, 27, 19, 
	45, 23, 5, 40, 27, 19, 47, 23, 
	6, 40, 23, 19, 25, 48, 27, 6, 
	40, 25, 26, 27, 18, 47, 6, 40, 
	26, 27, 19, 47, 23, 6, 40, 28, 
	11, 19, 46, 23, 7, 40, 25, 26, 
	27, 19, 47, 23, 7, 40, 25, 28, 
	11, 19, 46, 23
};

static const char _md_cond_offsets[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 5, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10
};

static const char _md_cond_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 5, 5, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0
};

static const short _md_cond_keys[] = {
	-128, -1, 0, 0, 1, 9, 10, 10, 
	11, 127, -128, -1, 0, 0, 1, 9, 
	10, 10, 11, 127, 0
};

static const char _md_cond_spaces[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0
};

static const short _md_key_offsets[] = {
	0, 0, 2, 4, 5, 11, 12, 13, 
	15, 17, 19, 22, 24, 26, 36, 43, 
	48, 60, 61, 63, 65, 68, 71, 74, 
	76, 78, 83, 86, 96, 104, 106, 109, 
	112, 115, 118, 120, 133, 137, 141, 143, 
	147, 152, 155, 161, 168, 171, 174, 180, 
	185, 190, 192, 195, 200, 203, 206, 211, 
	213, 215, 220, 225, 230, 235, 240, 244, 
	251, 253, 260, 263, 266, 272, 279, 282, 
	288, 293, 306, 319, 332, 337, 339, 343, 
	348, 353, 358, 363, 367, 372, 377, 381, 
	384, 388, 391, 395, 398, 402, 405, 410, 
	415, 419, 422, 426, 429, 433, 436, 440, 
	443, 449, 456, 460, 464, 467, 470, 473, 
	476, 478, 480, 483, 486, 493, 494, 495, 
	497, 499, 501, 504, 507, 512, 523, 524, 
	534, 541, 546, 558, 559, 561, 564, 567, 
	570, 572, 574, 576, 576, 581, 581, 581, 
	596, 596, 598, 601, 601, 603, 608
};

static const short _md_trans_keys[] = {
	32, 60, 32, 60, 60, 33, 95, 65, 
	90, 97, 122, 45, 45, 10, 45, 10, 
	45, 10, 45, 10, 45, 62, 0, 10, 
	0, 10, 32, 47, 62, 95, 45, 58, 
	65, 90, 97, 122, 32, 47, 62, 65, 
	90, 97, 122, 32, 65, 90, 97, 122, 
	32, 45, 47, 61, 62, 95, 48, 58, 
	65, 90, 97, 122, 62, 0, 10, 34, 
	39, 0, 10, 34, 32, 47, 62, 0, 
	10, 39, 0, 10, 0, 10, 0, 9, 
	10, 32, 96, 0, 10, 96, 256, 266, 
	352, 512, 522, 608, 128, 383, 384, 639, 
	256, 266, 512, 522, 128, 383, 384, 639, 
	0, 10, 0, 10, 45, 0, 10, 61, 
	0, 10, 32, 0, 10, 32, 0, 10, 
	0, 9, 10, 32, 45, 58, 62, 96, 
	124, 42, 43, 48, 57, 0, 9, 10, 
	32, 0, 9, 10, 32, 0, 10, 0, 
	9, 10, 32, 0, 9, 10, 32, 45, 
	0, 10, 45, 0, 9, 10, 32, 45, 
	58, 0, 9, 10, 32, 45, 58, 124, 
	0, 10, 45, 0, 10, 45, 0, 9, 
	10, 32, 45, 58, 0, 10, 46, 48, 
	57, 0, 9, 10, 32, 62, 0, 10, 
	0, 10, 62, 0, 9, 10, 32, 62, 
	0, 10, 96, 0, 10, 96, 0, 9, 
	10, 32, 96, 0, 10, 0, 10, 0, 
	9, 10, 32, 96, 0, 9, 10, 32, 
	96, 0, 9, 10, 32, 96, 0, 9, 
	10, 32, 96, 0, 9, 10, 32, 96, 
	0, 9, 10, 32, 0, 9, 10, 32, 
	45, 58, 124, 0, 10, 0, 9, 10, 
	32, 45, 58, 124, 0, 10, 45, 0, 
	10, 45, 0, 9, 10, 32, 45, 58, 
	0, 9, 10, 32, 45, 58, 124, 0, 
	10, 45, 0, 9, 10, 32, 45, 58, 
	0, 9, 10, 32, 124, 0, 9, 10, 
	32, 45, 58, 62, 96, 124, 42, 43, 
	48, 57, 0, 9, 10, 32, 45, 58, 
	62, 96, 124, 42, 43, 48, 57, 0, 
	9, 10, 32, 45, 58, 62, 96, 124, 
	42, 43, 48, 57, 0, 9, 10, 32, 
	35, 0, 10, 0, 9, 10, 32, 0, 
	9, 10, 32, 35, 0, 9, 10, 32, 
	35, 0, 9, 10, 32, 35, 0, 9, 
	10, 32, 35, 0, 9, 10, 32, 0, 
	9, 10, 32, 42, 0, 9, 10, 32, 
	42, 0, 10, 32, 42, 0, 10, 42, 
	0, 10, 32, 42, 0, 10, 42, 0, 
	10, 32, 42, 0, 10, 42, 0, 10, 
	32, 42, 0, 10, 42, 0, 9, 10, 
	32, 45, 0, 9, 10, 32, 45, 0, 
	10, 32, 45, 0, 10, 45, 0, 10, 
	32, 45, 0, 10, 45, 0, 10, 32, 
	45, 0, 10, 45, 0, 10, 32, 45, 
	0, 10, 45, 0, 9, 10, 32, 45, 
	58, 0, 9, 10, 32, 45, 58, 124, 
	0, 10, 32, 45, 0, 10, 32, 45, 
	0, 10, 93, 0, 10, 93, 0, 10, 
	58, 0, 10, 32, 0, 10, 0, 10, 
	0, 10, 60, 0, 10, 60, 33, 47, 
	95, 65, 90, 97, 122, 45, 45, 10, 
	45, 10, 45, 10, 45, 10, 45, 62, 
	0, 10, 60, 95, 65, 90, 97, 122, 
	32, 62, 95, 45, 46, 48, 58, 65, 
	90, 97, 122, 62, 32, 47, 62, 95, 
	45, 58, 65, 90, 97, 122, 32, 47, 
	62, 65, 90, 97, 122, 32, 65, 90, 
	97, 122, 32, 45, 47, 61, 62, 95, 
	48, 58, 65, 90, 97, 122, 62, 34, 
	39, 0, 10, 34, 32, 47, 62, 0, 
	10, 39, 32, 60, 32, 60, 0, 10, 
	0, 9, 10, 32, 96, 0, 9, 10, 
	32, 35, 42, 43, 45, 58, 62, 91, 
	96, 124, 48, 57, 45, 61, 0, 10, 
	32, 45, 61, 0, 10, 32, 45, 61, 
	0
};

static const char _md_single_lengths[] = {
	0, 2, 2, 1, 2, 1, 1, 2, 
	2, 2, 3, 2, 2, 4, 3, 1, 
	6, 1, 2, 2, 3, 3, 3, 2, 
	2, 5, 3, 6, 4, 2, 3, 3, 
	3, 3, 2, 9, 4, 4, 2, 4, 
	5, 3, 6, 7, 3, 3, 6, 3, 
	5, 2, 3, 5, 3, 3, 5, 2, 
	2, 5, 5, 5, 5, 5, 4, 7, 
	2, 7, 3, 3, 6, 7, 3, 6, 
	5, 9, 9, 9, 5, 2, 4, 5, 
	5, 5, 5, 4, 5, 5, 4, 3, 
	4, 3, 4, 3, 4, 3, 5, 5, 
	4, 3, 4, 3, 4, 3, 4, 3, 
	6, 7, 4, 4, 3, 3, 3, 3, 
	2, 2, 3, 3, 3, 1, 1, 2, 
	2, 2, 3, 3, 1, 3, 1, 4, 
	3, 1, 6, 1, 2, 3, 3, 3, 
	2, 2, 2, 0, 5, 0, 0, 13, 
	0, 2, 3, 0, 2, 5, 0
};

static const char _md_range_lengths[] = {
	0, 0, 0, 0, 2, 0, 0, 0, 
	0, 0, 0, 0, 0, 3, 2, 2, 
	3, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 2, 2, 0, 0, 0, 
	0, 0, 0, 2, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 2, 2, 2, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 2, 0, 0, 0, 
	0, 0, 0, 0, 2, 4, 0, 3, 
	2, 2, 3, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	0, 0, 0, 0, 0, 0, 0
};

static const short _md_index_offsets[] = {
	0, 0, 3, 6, 8, 13, 15, 17, 
	20, 23, 26, 30, 33, 36, 44, 50, 
	54, 64, 66, 69, 72, 76, 80, 84, 
	87, 90, 96, 100, 109, 116, 119, 123, 
	127, 131, 135, 138, 150, 155, 160, 163, 
	168, 174, 178, 185, 193, 197, 201, 208, 
	213, 219, 222, 226, 232, 236, 240, 246, 
	249, 252, 258, 264, 270, 276, 282, 287, 
	295, 298, 306, 310, 314, 321, 329, 333, 
	340, 346, 358, 370, 382, 388, 391, 396, 
	402, 408, 414, 420, 425, 431, 437, 442, 
	446, 451, 455, 460, 464, 469, 473, 479, 
	485, 490, 494, 499, 503, 508, 512, 517, 
	521, 528, 536, 541, 546, 550, 554, 558, 
	562, 565, 568, 572, 576, 582, 584, 586, 
	589, 592, 595, 599, 603, 607, 615, 617, 
	625, 631, 635, 645, 647, 650, 654, 658, 
	662, 665, 668, 671, 672, 678, 679, 680, 
	695, 696, 699, 703, 704, 707, 713
};

static const unsigned char _md_trans_targs[] = {
	2, 4, 0, 3, 4, 0, 4, 0, 
	5, 13, 13, 13, 0, 6, 0, 7, 
	0, 8, 9, 8, 8, 9, 8, 8, 
	10, 8, 8, 10, 11, 8, 137, 137, 
	12, 137, 137, 12, 14, 17, 18, 13, 
	13, 13, 13, 0, 15, 17, 18, 16, 
	16, 0, 15, 16, 16, 0, 14, 16, 
	17, 19, 18, 16, 16, 16, 16, 0, 
	18, 0, 137, 137, 12, 20, 22, 0, 
	0, 0, 21, 20, 14, 17, 18, 0, 
	0, 0, 21, 22, 139, 139, 23, 141, 
	141, 24, 141, 25, 141, 25, 26, 24, 
	141, 141, 27, 24, 141, 141, 27, 142, 
	142, 27, 24, 28, 0, 141, 141, 142, 
	142, 24, 28, 0, 144, 145, 29, 143, 
	143, 30, 143, 143, 143, 31, 143, 143, 
	143, 33, 143, 143, 143, 34, 143, 143, 
	143, 143, 144, 35, 145, 35, 40, 45, 
	48, 52, 63, 36, 47, 29, 144, 37, 
	145, 37, 29, 144, 39, 145, 39, 38, 
	144, 145, 38, 144, 39, 145, 39, 38, 
	144, 37, 145, 37, 41, 29, 144, 145, 
	42, 29, 144, 43, 145, 43, 42, 46, 
	29, 144, 43, 145, 43, 44, 45, 46, 
	29, 144, 145, 41, 29, 144, 145, 44, 
	29, 144, 43, 145, 43, 44, 45, 29, 
	144, 145, 36, 47, 29, 144, 50, 145, 
	50, 51, 49, 144, 145, 49, 144, 145, 
	51, 49, 144, 50, 145, 50, 51, 49, 
	144, 145, 53, 29, 144, 145, 54, 29, 
	147, 56, 148, 56, 57, 55, 147, 148, 
	55, 147, 148, 55, 147, 56, 148, 56, 
	58, 55, 147, 56, 148, 56, 59, 55, 
	147, 56, 148, 56, 60, 55, 147, 56, 
	148, 56, 61, 55, 147, 56, 148, 56, 
	62, 55, 147, 56, 148, 56, 55, 144, 
	65, 145, 65, 66, 70, 72, 64, 144, 
	145, 64, 144, 65, 145, 65, 66, 70, 
	72, 64, 144, 145, 67, 64, 144, 145, 
	68, 64, 144, 69, 145, 69, 68, 71, 
	64, 144, 69, 145, 69, 66, 70, 71, 
	64, 144, 145, 66, 64, 144, 69, 145, 
	69, 66, 70, 64, 144, 72, 145, 72, 
	72, 64, 146, 35, 149, 74, 40, 45, 
	48, 52, 63, 36, 47, 29, 146, 35, 
	149, 75, 40, 45, 48, 52, 63, 36, 
	47, 29, 146, 35, 149, 35, 40, 45, 
	48, 52, 63, 36, 47, 29, 144, 78, 
	145, 78, 79, 77, 144, 145, 77, 144, 
	78, 145, 78, 77, 144, 78, 145, 78, 
	80, 77, 144, 78, 145, 78, 81, 77, 
	144, 78, 145, 78, 82, 77, 144, 78, 
	145, 78, 83, 77, 144, 78, 145, 78, 
	77, 144, 37, 145, 85, 90, 29, 144, 
	39, 145, 39, 86, 38, 144, 145, 87, 
	88, 38, 144, 145, 88, 38, 144, 145, 
	89, 88, 38, 144, 145, 88, 38, 144, 
	145, 91, 92, 29, 144, 145, 92, 29, 
	144, 145, 93, 92, 29, 144, 145, 92, 
	29, 144, 37, 145, 95, 100, 29, 144, 
	39, 145, 39, 96, 38, 144, 145, 97, 
	98, 38, 144, 145, 98, 38, 144, 145, 
	99, 98, 38, 144, 145, 98, 38, 144, 
	145, 101, 104, 29, 144, 145, 102, 29, 
	144, 145, 103, 102, 29, 144, 145, 102, 
	29, 144, 43, 145, 105, 104, 46, 29, 
	144, 43, 145, 43, 106, 45, 46, 29, 
	144, 145, 103, 107, 29, 144, 145, 103, 
	104, 29, 144, 145, 29, 109, 144, 145, 
	110, 109, 144, 145, 111, 29, 144, 145, 
	113, 112, 144, 145, 112, 144, 145, 112, 
	0, 115, 116, 115, 114, 115, 114, 115, 
	117, 124, 127, 127, 127, 0, 118, 0, 
	119, 0, 120, 121, 120, 120, 121, 120, 
	120, 122, 120, 120, 122, 123, 120, 0, 
	115, 116, 115, 125, 125, 125, 0, 126, 
	150, 125, 125, 125, 125, 125, 0, 150, 
	0, 128, 131, 114, 127, 127, 127, 127, 
	0, 129, 131, 114, 130, 130, 0, 129, 
	130, 130, 0, 128, 130, 131, 132, 114, 
	130, 130, 130, 130, 0, 114, 0, 133, 
	135, 0, 0, 0, 134, 133, 128, 131, 
	114, 0, 0, 0, 134, 135, 1, 4, 
	0, 1, 4, 0, 139, 139, 23, 138, 
	141, 25, 141, 25, 26, 24, 140, 140, 
	146, 35, 146, 73, 76, 84, 36, 94, 
	45, 48, 108, 52, 63, 47, 29, 143, 
	30, 31, 143, 143, 143, 32, 143, 143, 
	30, 31, 143, 143, 143, 32, 30, 31, 
	143, 0, 143, 143, 143, 143, 143, 138, 
	140, 140, 143, 143, 143, 143, 143, 143, 
	0
};

static const short _md_trans_actions[] = {
	0, 44, 17, 0, 44, 17, 44, 17, 
	0, 9, 9, 9, 17, 0, 17, 0, 
	17, 53, 47, 11, 21, 15, 0, 21, 
	15, 0, 21, 15, 0, 0, 255, 260, 
	59, 77, 86, 0, 62, 62, 147, 0, 
	0, 0, 0, 17, 0, 0, 1, 11, 
	11, 17, 0, 11, 11, 17, 265, 0, 
	265, 65, 370, 0, 0, 0, 0, 17, 
	3, 17, 135, 139, 11, 11, 11, 17, 
	17, 17, 0, 0, 155, 155, 270, 17, 
	17, 17, 0, 0, 19, 21, 0, 19, 
	21, 0, 19, 0, 21, 0, 107, 0, 
	19, 21, 31, 0, 19, 21, 31, 199, 
	203, 231, 0, 110, 0, 19, 21, 19, 
	21, 0, 0, 0, 247, 335, 0, 83, 
	92, 0, 39, 83, 92, 0, 39, 80, 
	89, 0, 37, 80, 89, 0, 37, 80, 
	89, 37, 305, 128, 382, 125, 122, 11, 
	104, 104, 11, 122, 122, 11, 247, 29, 
	335, 29, 0, 247, 23, 335, 23, 23, 
	355, 394, 0, 355, 23, 394, 23, 23, 
	247, 29, 335, 29, 0, 0, 247, 335, 
	0, 0, 243, 0, 330, 0, 0, 0, 
	0, 243, 0, 330, 0, 0, 0, 0, 
	0, 247, 335, 0, 0, 247, 335, 0, 
	0, 243, 0, 330, 0, 0, 0, 0, 
	247, 335, 0, 0, 0, 247, 113, 335, 
	113, 113, 113, 345, 436, 0, 345, 436, 
	113, 113, 345, 113, 436, 113, 113, 113, 
	247, 335, 0, 0, 247, 335, 0, 0, 
	300, 207, 484, 207, 207, 207, 19, 320, 
	0, 219, 469, 122, 300, 207, 484, 207, 
	207, 207, 300, 207, 484, 207, 207, 207, 
	300, 207, 484, 207, 207, 207, 300, 207, 
	484, 207, 207, 207, 300, 207, 484, 207, 
	207, 207, 300, 207, 484, 207, 207, 239, 
	23, 325, 23, 23, 23, 23, 23, 360, 
	400, 0, 310, 23, 412, 23, 23, 23, 
	23, 23, 360, 400, 0, 0, 360, 400, 
	0, 0, 315, 0, 418, 0, 0, 0, 
	0, 315, 0, 418, 0, 0, 0, 0, 
	0, 360, 400, 0, 0, 315, 0, 418, 
	0, 0, 0, 0, 310, 0, 412, 0, 
	0, 0, 235, 128, 376, 125, 122, 11, 
	104, 104, 11, 122, 122, 11, 235, 128, 
	376, 125, 122, 11, 104, 104, 11, 122, 
	122, 11, 235, 128, 376, 125, 122, 11, 
	104, 104, 11, 122, 122, 11, 455, 113, 
	476, 113, 113, 113, 350, 442, 0, 430, 
	23, 462, 23, 23, 455, 113, 476, 113, 
	113, 113, 455, 113, 476, 113, 113, 113, 
	455, 113, 476, 113, 113, 113, 455, 113, 
	476, 113, 113, 113, 455, 113, 476, 113, 
	113, 247, 29, 335, 29, 0, 0, 247, 
	23, 335, 23, 23, 23, 355, 394, 0, 
	0, 0, 355, 394, 0, 0, 424, 448, 
	0, 0, 0, 424, 448, 0, 0, 247, 
	335, 0, 0, 0, 247, 335, 0, 0, 
	340, 388, 0, 0, 0, 340, 388, 0, 
	0, 247, 29, 335, 29, 0, 0, 247, 
	23, 335, 23, 23, 23, 355, 394, 0, 
	0, 0, 355, 394, 0, 0, 424, 448, 
	0, 0, 0, 424, 448, 0, 0, 247, 
	335, 0, 0, 0, 247, 335, 0, 0, 
	340, 388, 0, 0, 0, 340, 388, 0, 
	0, 340, 0, 388, 0, 0, 0, 0, 
	340, 0, 388, 0, 0, 0, 0, 0, 
	340, 388, 0, 0, 0, 340, 388, 0, 
	0, 0, 247, 335, 0, 23, 247, 335, 
	25, 0, 247, 335, 0, 0, 247, 335, 
	116, 116, 365, 406, 0, 365, 406, 27, 
	17, 53, 7, 11, 159, 21, 159, 0, 
	0, 0, 9, 9, 9, 17, 0, 17, 
	0, 17, 53, 47, 11, 21, 15, 0, 
	21, 15, 0, 21, 15, 0, 0, 17, 
	143, 56, 59, 9, 9, 9, 17, 13, 
	151, 0, 0, 0, 0, 0, 17, 41, 
	17, 62, 62, 147, 0, 0, 0, 0, 
	17, 0, 0, 1, 11, 11, 17, 0, 
	11, 11, 17, 265, 0, 265, 65, 370, 
	0, 0, 0, 0, 17, 3, 17, 11, 
	11, 17, 17, 17, 0, 0, 155, 155, 
	270, 17, 17, 17, 0, 0, 5, 131, 
	17, 68, 275, 17, 50, 53, 11, 71, 
	211, 119, 215, 119, 295, 119, 167, 163, 
	235, 227, 251, 223, 195, 290, 191, 290, 
	101, 195, 187, 195, 101, 191, 101, 74, 
	98, 98, 74, 280, 285, 95, 179, 175, 
	171, 171, 175, 280, 285, 95, 183, 183, 
	179, 17, 39, 39, 37, 37, 37, 71, 
	167, 163, 74, 74, 179, 175, 175, 179, 
	0
};

static const short _md_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 33, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 33, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	33, 0, 33, 0, 33, 0, 0, 33, 
	0, 0, 0, 0, 0, 0, 0
};

static const short _md_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 35, 0, 35, 0, 0, 35, 
	0, 0, 0, 0, 0, 0, 0
};

static const short _md_eof_actions[] = {
	0, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	0, 15, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0
};

static const short _md_eof_trans[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 716, 716, 
	719, 719, 719, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 720, 0, 721, 722, 0, 
	724, 724, 728, 727, 727, 728, 0
};

static const int md_start = 136;
static const int md_first_final = 136;
static const int md_error = 0;

static const int md_en_error = 138;
static const int md_en_fenced_code_scanner = 140;
static const int md_en_markdown = 143;
static const int md_en_inner = 114;
static const int md_en_main = 136;


#line 583 "xs6/xsedit/markdown/kprMarkdownParser.rl"


#pragma unused (md_en_error, md_en_fenced_code_scanner, md_en_inner, md_en_main, md_en_markdown)

#define STACKSIZE KPRMARKDOWNSTACKSIZE

static Boolean _indentedCodeLine(char *str, char *ts, char *te, KprMarkdownRun run, KprMarkdownState state)
{
	if ((state->spaces - 4) >= state->marker) {
		int indent = _indent(ts, te - 1, state->marker + 4, 0);
		int length = te - 1 - ts - indent;
		int offset = indent; //@@ ts - str
		run->type = kprMarkdownIndentedCodeLine;
		run->colors = kprMarkdownCodeColor;
		run->length = te - ts;
		run->offset = ts - str;
		run->marker = state->marker;
		run->shaper = _blank(ts + offset, NULL, length);
		run->u.s.t.length = length;
		run->u.s.t.offset = offset;
		run->u.s.v.length = -1;
		run->u.s.v.offset = 0;
		return true;
	}
	return false;
}

static Boolean _mergeTableColumnDivider(KprMarkdownParser parser, char *str, SInt32 offset, char *ts, char *te, KprMarkdownRun run)
{
	if ((ts - str) > offset) {
		KprMarkdownRun textRun = NULL;
		SInt32 index = (SInt32)FskGrowableArrayGetItemCount((FskGrowableArray)parser->runs) - 1, length = 0;
		FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, index, (void **)&textRun);
		if (textRun->type == kprMarkdownTextSpan) {
			char *t1 = str + textRun->offset + textRun->u.s.t.offset;
			char *t2 = t1 + textRun->u.s.t.length - 1;
			while (t2 >= t1) {
				if (!_IsSpace(*t2))
					break;
				length++;
				t2--;
			}
			if (length > 0) {
				if (length == textRun->u.s.t.length) {
					// remove blank text span, create new run with combined offset and length
					FskGrowableArraySetItemCount((FskGrowableArray)parser->runs, index);
				}
				else {
					// update text span with reduced length, create new run with adjusted offset and length
					textRun->length -= length;
					textRun->u.s.t.length -= length;
				}
				run->length += length;
				run->offset -= length;
				run->u.s.v.length += length;
				return true;
			}
		}
	}
	return false;
}

static Boolean _mergeTextSpan(KprMarkdownParser parser, char *str, SInt32 offset, char *ts, char *te)
{
	if ((ts - str) > offset) {
		KprMarkdownRun textRun = NULL;
		SInt32 index = (SInt32)FskGrowableArrayGetItemCount((FskGrowableArray)parser->runs) - 1, length = te - ts;
		FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, index, (void **)&textRun);
		if (textRun->type == kprMarkdownTextSpan) {
			textRun->length += length;
			textRun->u.s.t.length += length;
			return true;
		}
	}
	return false;
}

FskErr KprMarkdownParse(KprMarkdownParser parser, char *str, SInt32 offset, SInt32 length, SInt32 flags)
{
	char *p = str + offset, *pe = p + length, *eof = pe, *te, *ts;
	char *b, *h, *h1, *h2, *m1, *m2, *t1, *t2, *v1, *v2;
	SInt32 act, cs, i, n, stack[STACKSIZE], top;
	
	FskErr err = kFskErrNone;
	KprMarkdownRunRecord runRecord;
	KprMarkdownRun run = &runRecord;
	KprMarkdownStateRecord stateRecord;
	KprMarkdownState state = &stateRecord;
	doResetState(parser, str, state, 0x7);
	doSetStateFlags(parser, str, state, flags);
	state->pc = p; // markup error on the first line
	
	
#line 1256 "xs6/xsedit/markdown/kprMarkdownParser.c"
	{
	cs = md_start;
	top = 0;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 1265 "xs6/xsedit/markdown/kprMarkdownParser.c"
	{
	int _klen;
	unsigned int _trans;
	short _widec;
	const char *_acts;
	unsigned int _nacts;
	const short *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_acts = _md_actions + _md_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 39:
#line 1 "NONE"
	{ts = p;}
	break;
#line 1287 "xs6/xsedit/markdown/kprMarkdownParser.c"
		}
	}

	_widec = (*p);
	_klen = _md_cond_lengths[cs];
	_keys = _md_cond_keys + (_md_cond_offsets[cs]*2);
	if ( _klen > 0 ) {
		const short *_lower = _keys;
		const short *_mid;
		const short *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( _widec < _mid[0] )
				_upper = _mid - 2;
			else if ( _widec > _mid[1] )
				_lower = _mid + 2;
			else {
				switch ( _md_cond_spaces[_md_cond_offsets[cs] + ((_mid - _keys)>>1)] ) {
	case 0: {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 869 "xs6/xsedit/markdown/kprMarkdownParser.rl"
 (i == n) && (_spaces(ts, m1, -1, 0) == state->spaces)  ) _widec += 256;
		break;
	}
				}
				break;
			}
		}
	}

	_keys = _md_trans_keys + _md_key_offsets[cs];
	_trans = _md_index_offsets[cs];

	_klen = _md_single_lengths[cs];
	if ( _klen > 0 ) {
		const short *_lower = _keys;
		const short *_mid;
		const short *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( _widec < *_mid )
				_upper = _mid - 1;
			else if ( _widec > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _md_range_lengths[cs];
	if ( _klen > 0 ) {
		const short *_lower = _keys;
		const short *_mid;
		const short *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( _widec < _mid[0] )
				_upper = _mid - 2;
			else if ( _widec > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
_eof_trans:
	cs = _md_trans_targs[_trans];

	if ( _md_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _md_actions + _md_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 676 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttribute(parser, str, state));
		}
	break;
	case 1:
#line 680 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttributeName(parser, str, state));
		}
	break;
	case 2:
#line 684 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttributeValue(parser, str, state));
		}
	break;
	case 3:
#line 688 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			KprMarkdownElementInfo info = getElementInfoByName(state->elementStart, state->elementStop);
			if (info && ((info->type == kprMarkdownBR) || (info->type == kprMarkdownSPAN)) && (state->depth == 1)) {
				p = state->pc; // backtrack (block level <br> and <span> to markdown)
				bailIfError(doProcessMarkup(parser, str, state, NULL, -1));
				doResetState(parser, str, state, 0x0);
				state->holdPC = NULL;
				state->markPC = NULL;
				p--; {cs = 143; goto _again;}
			}
			else if (info && (info->state == 0)) { // empty element
				bailIfError(doExitElement(parser, str, state));
			}
			else {
				{stack[top++] = cs; cs = 114; goto _again;}
			}
		}
	break;
	case 4:
#line 706 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doEnterElement(parser, str, state));
		}
	break;
	case 5:
#line 710 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			KprMarkdownElementInfo info = getElementInfoByName(state->elementStart, state->elementStop);
			if (info && ((info->type == kprMarkdownBR) || (info->type == kprMarkdownSPAN)) && (state->depth == 1)) {
				p = state->pc; // backtrack (block level <br> and <span> to markdown)
				bailIfError(doProcessMarkup(parser, str, state, NULL, -1));
				doResetState(parser, str, state, 0x0);
				state->holdPC = NULL;
				state->markPC = NULL;
				p--; {cs = 143; goto _again;}
			}
			else {
				bailIfError(doExitElement(parser, str, state));
			}
		}
	break;
	case 6:
#line 725 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doHold(parser, str, state, p, state->line));
		}
	break;
	case 7:
#line 729 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doMark(parser, str, state, p));
		}
	break;
	case 8:
#line 733 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doProcessText(parser, str, state, p, state->line));
		}
	break;
	case 9:
#line 737 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartComment(parser, str, state, p));
		}
	break;
	case 10:
#line 741 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartElement(parser, str, state, p));
		}
	break;
	case 11:
#line 745 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartText(parser, str, state, p));
		}
	break;
	case 12:
#line 749 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopComment(parser, str, state, p));
		}
	break;
	case 13:
#line 753 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopElement(parser, str, state, p));
		}
	break;
	case 14:
#line 757 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopText(parser, str, state, p));
		}
	break;
	case 15:
#line 761 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopText(parser, str, state, p - 1));
		}
	break;
	case 16:
#line 765 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			if (state->holdPC && state->markPC) {
				char c = *(p - 1);
				if ((c == '\n') || (c == '\0')) { // special case (c character caused the markup parse error)
					bailIfError(doProcessMarkup(parser, str, state, p + 0, -1)); // error
					doResetState(parser, str, state, 0x3);
					state->holdPC = NULL;
					state->markPC = NULL;
					if ((c == '\n') && ((*p) != '\0')) {
						state->line++;
					}
					p--; {cs = 136; goto _again;}
				}
				else {
					if (state->line > state->holdLine) {
						SInt32 shaper = (state->depth == -1) ? 0 : 1;
						bailIfError(doProcessMarkup(parser, str, state, state->pc, shaper));
					}
					state->holdLine = state->line;
					state->holdPC = state->pc;
					state->markPC = NULL;
					p--; {cs = 138; goto _again;}
				}
			}
			else {
				if (state->holdPC && !state->markPC) {
					p = state->holdPC; // backtrack
				}
				state->holdPC = NULL;
				state->markPC = NULL;
				p--; {cs = 143; goto _again;}
			}
		}
	break;
	case 17:
#line 799 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			SInt32 shaper = (state->depth == -1) ? 0 : 1;
			bailIfError(doProcessMarkup(parser, str, state, p + 1, shaper));
			doResetState(parser, str, state, 0x3);
			state->holdPC = NULL;
			state->markPC = NULL;
		}
	break;
	case 18:
#line 807 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ /*state->line++; */state->pc = NULL; }
	break;
	case 19:
#line 809 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ state->line++; state->pc = p + 1; }
	break;
	case 20:
#line 837 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ b = p; }
	break;
	case 21:
#line 838 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ h = p; }
	break;
	case 22:
#line 840 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ h1 = p; }
	break;
	case 23:
#line 841 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ h2 = p; }
	break;
	case 24:
#line 842 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ m1 = p; }
	break;
	case 25:
#line 843 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ m2 = p; }
	break;
	case 26:
#line 844 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ t1 = p; }
	break;
	case 27:
#line 845 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ t2 = p; }
	break;
	case 28:
#line 846 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ v1 = p; }
	break;
	case 29:
#line 847 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ v2 = p; }
	break;
	case 30:
#line 849 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ t2 = p - 1; }
	break;
	case 31:
#line 850 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ v2 = p - 1; }
	break;
	case 32:
#line 852 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ state->indent++; state->spaces++; }
	break;
	case 33:
#line 853 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ state->indent++; state->spaces += (4 - (state->spaces % 4)); }
	break;
	case 34:
#line 869 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ i = 1; }
	break;
	case 35:
#line 869 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ i++; }
	break;
	case 36:
#line 1270 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ p--; }
	break;
	case 37:
#line 1274 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ {cs = stack[--top]; goto _again;} }
	break;
	case 40:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 41:
#line 825 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				bailIfError(doProcessMarkup(parser, str, state, p + 1, -1)); // error
				doResetState(parser, str, state, 0x3);
				state->holdPC = NULL;
				state->markPC = NULL;
				{cs = 136; goto _again;}
			}}
	break;
	case 42:
#line 869 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{ // semantic condition
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownFencedCodeEndLine;
				runRecord.colors = kprMarkdownLineMarkerColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.marker = state->marker;
				runRecord.shaper = 0;
				runRecord.u.s.t.length = -1;
				runRecord.u.s.t.offset = 0;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				doResetState(parser, str, state, 0x3);
				{cs = 136; goto _again;} // no fret here
			}}
	break;
	case 43:
#line 889 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				int indent = _indent(t1, t2, state->spaces, 1);
				int length = t2 - t1 - indent;
				int offset = indent; //@@ t1 - str
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownFencedCodeLine;
				runRecord.colors = kprMarkdownCodeColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.marker = state->marker;
				runRecord.shaper = _blank(t1 + offset, NULL, length);
			//	if (length > 0) {
					runRecord.u.s.t.length = length;
					runRecord.u.s.t.offset = offset;
			//	}
			//	else {
			//		runRecord.u.s.t.length = -1;
			//		runRecord.u.s.t.offset = 0;
			//	}
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				doResetState(parser, str, state, 0x0);
			}}
	break;
	case 44:
#line 939 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 4;}
	break;
	case 45:
#line 977 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 5;}
	break;
	case 46:
#line 1001 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 6;}
	break;
	case 47:
#line 1025 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 7;}
	break;
	case 48:
#line 1093 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 9;}
	break;
	case 49:
#line 1113 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 10;}
	break;
	case 50:
#line 1137 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 11;}
	break;
	case 51:
#line 1158 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 12;}
	break;
	case 52:
#line 1180 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 13;}
	break;
	case 53:
#line 1202 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 14;}
	break;
	case 54:
#line 1227 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 15;}
	break;
	case 55:
#line 939 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p+1;{
				#if KPRMARKDOWNDEBUGMARKDOWN
					state->textStart = NULL;
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownBlankLine;
				runRecord.colors = kprMarkdownLineMarkerColor;
				runRecord.length = b - ts;
				runRecord.offset = ts - str;
				runRecord.marker = state->marker;
				runRecord.shaper = 0;
				runRecord.u.s.t.length = -1;
				runRecord.u.s.t.offset = 0;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				if (te > b) {
					FskMemSet(run, 0, sizeof(runRecord));
					runRecord.type = kprMarkdownBlankLine;
					runRecord.colors = kprMarkdownLineMarkerColor;
					runRecord.length = te - b;
					runRecord.offset = b - str;
					runRecord.marker = 0;
					runRecord.shaper = 0;
					runRecord.u.s.t.length = -1;
					runRecord.u.s.t.offset = 0;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
					bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
					doResetState(parser, str, state, 0x3 );
				}
				else {
					doResetState(parser, str, state, 0x1 );
				}
				{cs = 136; goto _again;}
			}}
	break;
	case 56:
#line 1052 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p+1;{
				// caution: runs concurrently with other line patterns
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.count = kprMarkdownCountDirective;
				runRecord.type = kprMarkdownHeaderLine;
				runRecord.colors = kprMarkdownLineMarkerColor;
				runRecord.length = h - ts;
				runRecord.offset = ts - str;
				runRecord.marker = 0;
				runRecord.shaper = (*h == '=') ? 1 : 2;
				runRecord.u.s.t.length = h2 - h1;
				runRecord.u.s.t.offset = h1 - str - runRecord.offset;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownHeaderZeroLine;
				runRecord.colors = kprMarkdownLineMarkerColor;
				runRecord.length = te - h;
				runRecord.offset = h - str;
				runRecord.marker = 0;
				runRecord.shaper = 0;
				runRecord.u.s.t.length = -1;
				runRecord.u.s.t.offset = 0;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				if (flags & kprMarkdownParseToFirstHeader) {
					state->line--;
					{p++; goto _out; }
				}
				else {
					doResetState(parser, str, state, 0x3);
					//state->line++;
					{cs = 136; goto _again;}
				}
			}}
	break;
	case 57:
#line 939 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				#if KPRMARKDOWNDEBUGMARKDOWN
					state->textStart = NULL;
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownBlankLine;
				runRecord.colors = kprMarkdownLineMarkerColor;
				runRecord.length = b - ts;
				runRecord.offset = ts - str;
				runRecord.marker = state->marker;
				runRecord.shaper = 0;
				runRecord.u.s.t.length = -1;
				runRecord.u.s.t.offset = 0;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				if (te > b) {
					FskMemSet(run, 0, sizeof(runRecord));
					runRecord.type = kprMarkdownBlankLine;
					runRecord.colors = kprMarkdownLineMarkerColor;
					runRecord.length = te - b;
					runRecord.offset = b - str;
					runRecord.marker = 0;
					runRecord.shaper = 0;
					runRecord.u.s.t.length = -1;
					runRecord.u.s.t.offset = 0;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
					bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
					doResetState(parser, str, state, 0x3 );
				}
				else {
					doResetState(parser, str, state, 0x1 );
				}
				{cs = 136; goto _again;}
			}}
	break;
	case 58:
#line 1001 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				if (!_indentedCodeLine(str, ts, te, run, state)) {
					runRecord.type = kprMarkdownFencedCodeBeginLine;
					runRecord.colors = kprMarkdownLineMarkerColor;
					runRecord.length = te - ts;
					runRecord.offset = ts - str;
					runRecord.marker = state->marker;
					runRecord.shaper = 0;
					runRecord.u.s.t.length = -1;
					runRecord.u.s.t.offset = 0;
					runRecord.u.s.v.length = v2 - v1;
					runRecord.u.s.v.offset = v1 - str - runRecord.offset;
				}
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				if ((*p)) {
					n = (SInt32)(m2 - m1);
					{cs = 140; goto _again;} // no fcall here
				}
			}}
	break;
	case 59:
#line 939 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{{p = ((te))-1;}{
				#if KPRMARKDOWNDEBUGMARKDOWN
					state->textStart = NULL;
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownBlankLine;
				runRecord.colors = kprMarkdownLineMarkerColor;
				runRecord.length = b - ts;
				runRecord.offset = ts - str;
				runRecord.marker = state->marker;
				runRecord.shaper = 0;
				runRecord.u.s.t.length = -1;
				runRecord.u.s.t.offset = 0;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				if (te > b) {
					FskMemSet(run, 0, sizeof(runRecord));
					runRecord.type = kprMarkdownBlankLine;
					runRecord.colors = kprMarkdownLineMarkerColor;
					runRecord.length = te - b;
					runRecord.offset = b - str;
					runRecord.marker = 0;
					runRecord.shaper = 0;
					runRecord.u.s.t.length = -1;
					runRecord.u.s.t.offset = 0;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
					bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
					doResetState(parser, str, state, 0x3 );
				}
				else {
					doResetState(parser, str, state, 0x1 );
				}
				{cs = 136; goto _again;}
			}}
	break;
	case 60:
#line 1 "NONE"
	{	switch( act ) {
	case 4:
	{{p = ((te))-1;}
				#if KPRMARKDOWNDEBUGMARKDOWN
					state->textStart = NULL;
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownBlankLine;
				runRecord.colors = kprMarkdownLineMarkerColor;
				runRecord.length = b - ts;
				runRecord.offset = ts - str;
				runRecord.marker = state->marker;
				runRecord.shaper = 0;
				runRecord.u.s.t.length = -1;
				runRecord.u.s.t.offset = 0;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				if (te > b) {
					FskMemSet(run, 0, sizeof(runRecord));
					runRecord.type = kprMarkdownBlankLine;
					runRecord.colors = kprMarkdownLineMarkerColor;
					runRecord.length = te - b;
					runRecord.offset = b - str;
					runRecord.marker = 0;
					runRecord.shaper = 0;
					runRecord.u.s.t.length = -1;
					runRecord.u.s.t.offset = 0;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
					bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
					doResetState(parser, str, state, 0x3 );
				}
				else {
					doResetState(parser, str, state, 0x1 );
				}
				{cs = 136; goto _again;}
			}
	break;
	case 5:
	{{p = ((te))-1;}
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				if (!_indentedCodeLine(str, ts, te, run, state)) {
					state->marker = _column(ts, t1, -1);
					runRecord.count = kprMarkdownCountDirective;
					runRecord.type = kprMarkdownBlockquoteLine;
					runRecord.colors = kprMarkdownLineMarkerColor;
					runRecord.length = te - ts;
					runRecord.offset = ts - str;
					runRecord.marker = state->marker;
					runRecord.shaper = 0;
					runRecord.u.s.t.length = t2 - t1;
					runRecord.u.s.t.offset = t1 - str - runRecord.offset;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
				}
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				doResetState(parser, str, state, 0x1);
				{cs = 136; goto _again;}
			}
	break;
	case 6:
	{{p = ((te))-1;}
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				if (!_indentedCodeLine(str, ts, te, run, state)) {
					runRecord.type = kprMarkdownFencedCodeBeginLine;
					runRecord.colors = kprMarkdownLineMarkerColor;
					runRecord.length = te - ts;
					runRecord.offset = ts - str;
					runRecord.marker = state->marker;
					runRecord.shaper = 0;
					runRecord.u.s.t.length = -1;
					runRecord.u.s.t.offset = 0;
					runRecord.u.s.v.length = v2 - v1;
					runRecord.u.s.v.offset = v1 - str - runRecord.offset;
				}
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				if ((*p)) {
					n = (SInt32)(m2 - m1);
					{cs = 140; goto _again;} // no fcall here
				}
			}
	break;
	case 7:
	{{p = ((te))-1;}
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.count = kprMarkdownCountDirective;
				runRecord.type = kprMarkdownHeaderLine;
				runRecord.colors = kprMarkdownLineMarkerColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.marker = 0;
				runRecord.shaper = (SInt16)(m2 - m1);
				runRecord.u.s.t.length = t2 - t1;
				runRecord.u.s.t.offset = t1 - str - runRecord.offset;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				if (flags & kprMarkdownParseToFirstHeader) {
					state->line--;
					{p++; goto _out; }
				}
				else {
					doResetState(parser, str, state, 0x3);
					{cs = 136; goto _again;}
				}
			}
	break;
	case 9:
	{{p = ((te))-1;}
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownHorizontalRuleLine;
				runRecord.colors = kprMarkdownLineMarkerColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.marker = 0;
				runRecord.shaper = 0;
				runRecord.u.s.t.length = -1;
				runRecord.u.s.t.offset = 0;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				doResetState(parser, str, state, 0x3);
				{cs = 136; goto _again;}
			}
	break;
	case 10:
	{{p = ((te))-1;}
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				if (!_indentedCodeLine(str, ts, te, run, state)) {
					state->marker = _column(ts, t1, -1);
					runRecord.count = kprMarkdownCountDirective;
					runRecord.type = kprMarkdownListLine;
					runRecord.colors = kprMarkdownLineMarkerColor;
					runRecord.length = te - ts;
					runRecord.offset = ts - str;
					runRecord.marker = state->marker;
					runRecord.shaper = (*(v2 - 1) == '.') ? -1 : 1; // -ordered +unordered
					runRecord.u.s.t.length = t2 - t1;
					runRecord.u.s.t.offset = t1 - str - runRecord.offset;
					runRecord.u.s.v.length = v2 - v1;
					runRecord.u.s.v.offset = v1 - str - runRecord.offset;
				}
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				doResetState(parser, str, state, 0x1);
				{cs = 136; goto _again;}
			}
	break;
	case 11:
	{{p = ((te))-1;}
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				parser->referenceDefinitionCount++;
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownReferenceDefinitionLine;
				runRecord.colors = kprMarkdownLineMarkerColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.marker = 0;
				runRecord.shaper = 0;
				runRecord.u.s.t.length = t2 - t1;
				runRecord.u.s.t.offset = t1 - str - runRecord.offset;
				runRecord.u.s.v.length = v2 - v1;
				runRecord.u.s.v.offset = v1 - str - runRecord.offset;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				doResetState(parser, str, state, 0x3);
				{cs = 136; goto _again;}
			}
	break;
	case 12:
	{{p = ((te))-1;}
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				if (!_indentedCodeLine(str, ts, te, run, state)) {
					runRecord.type = kprMarkdownTableLine;
					runRecord.colors = kprMarkdownLineMarkerColor;
					runRecord.length = te - ts;
					runRecord.offset = ts - str;
					runRecord.marker = state->marker;
					runRecord.shaper = -1;
					runRecord.u.s.t.length = runRecord.length - 1;
					runRecord.u.s.t.offset = runRecord.offset - runRecord.offset;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
				}
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				doResetState(parser, str, state, 0x1);
				{cs = 136; goto _again;}
			}
	break;
	case 13:
	{{p = ((te))-1;}
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				if (!_indentedCodeLine(str, ts, te, run, state)) {
					runRecord.type = kprMarkdownTableLine;
					runRecord.colors = kprMarkdownLineMarkerColor;
					runRecord.length = te - ts;
					runRecord.offset = ts - str;
					runRecord.marker = state->marker;
					runRecord.shaper = -2;
					runRecord.u.s.t.length = runRecord.length - 1;
					runRecord.u.s.t.offset = runRecord.offset - runRecord.offset;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
				}
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				doResetState(parser, str, state, 0x1);
				{cs = 136; goto _again;}
			}
	break;
	case 14:
	{{p = ((te))-1;}
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				if (!_indentedCodeLine(str, ts, te, run, state)) {
					runRecord.count = kprMarkdownCountDirective;
					runRecord.type = kprMarkdownTableLine;
					runRecord.colors = kprMarkdownLineMarkerColor;
					runRecord.length = te - ts;
					runRecord.offset = ts - str;
					runRecord.marker = state->marker;
					runRecord.shaper = state->tableRow++;
					runRecord.u.s.t.length = t2 - t1;
					runRecord.u.s.t.offset = t1 - str - runRecord.offset;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
				}
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				doResetState(parser, str, state, 0x0);
				{cs = 136; goto _again;}
			}
	break;
	case 15:
	{{p = ((te))-1;}
				// caution: runs concurrently with other line patterns
				// note: changes to table_line with table_column_divider
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, p, state->line - 1);
				#endif
				FskMemSet(run, 0, sizeof(runRecord));
				if (!_indentedCodeLine(str, ts, te, run, state)) {
					int indent = _indent(ts, te - 1, (state->spaces < state->marker) ? state->spaces : state->marker, 0);
					int length = te - 1 - ts - indent;
					state->marker = (state->spaces < state->marker) ? state->spaces : state->marker;
					runRecord.count = kprMarkdownCountDirective;
					runRecord.type = kprMarkdownParagraphLine;
					runRecord.colors = kprMarkdownTextColor;
					runRecord.length = te - ts;
					runRecord.offset = ts - str;
					runRecord.marker = state->marker;
					runRecord.shaper = 0;
					runRecord.u.s.t.length = length;
					runRecord.u.s.t.offset = runRecord.offset + indent - runRecord.offset;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
				}
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				doResetState(parser, str, state, 0x1);
				{cs = 136; goto _again;}
			}
	break;
	}
	}
	break;
#line 2219 "xs6/xsedit/markdown/kprMarkdownParser.c"
		}
	}

_again:
	_acts = _md_actions + _md_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 38:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 2232 "xs6/xsedit/markdown/kprMarkdownParser.c"
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _md_eof_trans[cs] > 0 ) {
		_trans = _md_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	const char *__acts = _md_actions + _md_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 15:
#line 761 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopText(parser, str, state, p - 1));
		}
	break;
	case 16:
#line 765 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			if (state->holdPC && state->markPC) {
				char c = *(p - 1);
				if ((c == '\n') || (c == '\0')) { // special case (c character caused the markup parse error)
					bailIfError(doProcessMarkup(parser, str, state, p + 0, -1)); // error
					doResetState(parser, str, state, 0x3);
					state->holdPC = NULL;
					state->markPC = NULL;
					if ((c == '\n') && ((*p) != '\0')) {
						state->line++;
					}
					p--; {cs = 136; goto _again;}
				}
				else {
					if (state->line > state->holdLine) {
						SInt32 shaper = (state->depth == -1) ? 0 : 1;
						bailIfError(doProcessMarkup(parser, str, state, state->pc, shaper));
					}
					state->holdLine = state->line;
					state->holdPC = state->pc;
					state->markPC = NULL;
					p--; {cs = 138; goto _again;}
				}
			}
			else {
				if (state->holdPC && !state->markPC) {
					p = state->holdPC; // backtrack
				}
				state->holdPC = NULL;
				state->markPC = NULL;
				p--; {cs = 143; goto _again;}
			}
		}
	break;
#line 2293 "xs6/xsedit/markdown/kprMarkdownParser.c"
		}
	}
	}

	_out: {}
	}

#line 1280 "xs6/xsedit/markdown/kprMarkdownParser.rl"

	
	if ((cs == md_error) || (cs < md_first_final)) {
		err = kFskErrScript;
	}
bail:
	parser->lineCount = state->line;
	if (err == kFskErrNone) {
		KprMarkdownRun blankLineRun = NULL, formerLineRun = NULL, lineRun = NULL;
		SInt32 blankLineIndex = -1, formerLineIndex = -1, lineCount = parser->lineCount, lineIndex;
		for (lineIndex = 0; lineIndex < lineCount; lineIndex++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, lineIndex, (void **)&lineRun);
			if (lineRun->count == kprMarkdownCountDirective) {
				if (flags & kprMarkdownParseAll) {
					lineRun->index = (SInt32)FskGrowableArrayGetItemCount((FskGrowableArray)parser->runs);
					bailIfError(KprMarkdownParseInline(parser, str, lineRun->offset + lineRun->u.s.t.offset, lineRun->u.s.t.length, flags, lineIndex, lineRun));
					if (blankLineIndex >= 0) {
						FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, blankLineIndex, (void **)&blankLineRun);
					}
					else {
						blankLineRun = NULL;
					}
					if (formerLineIndex >= 0) {
						FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, formerLineIndex, (void **)&formerLineRun);
					}
					else {
						formerLineRun = NULL;
					}
					FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, lineIndex, (void **)&lineRun);
				}
				else
					lineRun->count = 0;
			}
			if (lineRun->type == kprMarkdownBlankLine) {
				if (blankLineRun && (blankLineRun->type == kprMarkdownBlankLine)) {
					blankLineRun->type = kprMarkdownIndentedCodeLine;
					blankLineRun->shaper = 2; // blank line -> indented code line
				}
				if (formerLineRun && (formerLineRun->type == kprMarkdownIndentedCodeLine)) {
					if (formerLineRun->shaper > 0) {
						formerLineRun->type = kprMarkdownBlankLine;
						formerLineRun->shaper = 1; // indented code line -> blank line
					}
					blankLineIndex = lineIndex;
					blankLineRun = lineRun;
				}
				else {
					blankLineIndex = -1;
					blankLineRun = NULL;
				}
				parser->fontStyle = 0;
			}
			else {
				if (lineRun->type == kprMarkdownIndentedCodeLine) {
					if (blankLineRun && (blankLineRun->type == kprMarkdownBlankLine)) {
						blankLineRun->type = kprMarkdownIndentedCodeLine;
						blankLineRun->shaper = 2; // blank line -> indented code line
					}
				}
				else {
					if (lineRun->type == kprMarkdownParagraphLine) {
						char *s = str + lineRun->offset + lineRun->length - 1;
						if ((lineRun->length > 2) && (*(--s) == ' ') && (*(--s) == ' ')) {
							lineRun->shaper = 1; // hard linebreak
						}
					}
					if (formerLineRun && (formerLineRun->type == kprMarkdownIndentedCodeLine)) {
						if (formerLineRun->shaper > 0) {
							formerLineRun->type = kprMarkdownBlankLine;
							formerLineRun->shaper = 1; // indented code line -> blank line
						}
					}
				}
				blankLineIndex = -1;
				blankLineRun = NULL;
			}
			formerLineIndex = lineIndex;
			formerLineRun = lineRun;
			n = lineRun->length - 1;
			if (n > parser->columnCount) {
				parser->columnCount = n;
			}
		}
		FskGrowableArrayMinimize((FskGrowableArray)parser->runs);
	}
	return err;
}


#line 2391 "xs6/xsedit/markdown/kprMarkdownParser.c"
static const char _md_inline_actions[] = {
	0, 1, 10, 1, 11, 1, 13, 1, 
	15, 1, 16, 1, 17, 1, 18, 1, 
	19, 1, 29, 1, 30, 1, 31, 1, 
	33, 1, 42, 1, 44, 1, 45, 1, 
	46, 1, 47, 2, 11, 15, 2, 12, 
	42, 2, 13, 4, 2, 14, 1, 2, 
	14, 8, 2, 17, 18, 2, 18, 32, 
	2, 18, 39, 2, 18, 40, 2, 20, 
	34, 2, 20, 35, 2, 20, 36, 2, 
	20, 37, 2, 20, 41, 2, 20, 43, 
	2, 23, 33, 2, 24, 33, 2, 25, 
	33, 2, 26, 33, 2, 27, 33, 2, 
	28, 33, 2, 31, 19, 2, 31, 22, 
	3, 5, 16, 38, 3, 11, 14, 8, 
	3, 14, 2, 0, 3, 18, 21, 33, 
	3, 19, 20, 34, 3, 19, 20, 35, 
	3, 19, 20, 36, 3, 19, 20, 37, 
	3, 31, 3, 16, 4, 13, 5, 16, 
	38, 4, 14, 1, 2, 0, 4, 31, 
	6, 7, 9, 5, 31, 13, 4, 3, 
	16, 6, 31, 14, 2, 0, 3, 16, 
	7, 31, 14, 1, 2, 0, 3, 16
	
};

static const short _md_inline_key_offsets[] = {
	0, 1, 2, 6, 7, 8, 9, 10, 
	11, 15, 18, 24, 31, 38, 47, 56, 
	63, 72, 79, 88, 95, 104, 113, 122, 
	129, 139, 147, 155, 162, 170, 178, 186, 
	193, 194, 195, 198, 201, 204, 208, 218, 
	225, 230, 242, 243, 245, 248, 251, 252, 
	253, 258, 269, 270, 272, 275, 276, 280, 
	281, 282, 283, 284, 285, 286, 287, 298, 
	309, 310, 326, 327, 329, 329, 330, 330, 
	336, 336, 337, 338, 346, 346, 348, 349, 
	351
};

static const char _md_inline_trans_keys[] = {
	93, 93, 9, 32, 40, 91, 91, 93, 
	93, 41, 41, 88, 120, 48, 57, 59, 
	48, 57, 48, 57, 65, 70, 97, 102, 
	59, 48, 57, 65, 70, 97, 102, 59, 
	48, 57, 65, 90, 97, 122, 59, 77, 
	109, 48, 57, 65, 90, 97, 122, 59, 
	80, 112, 48, 57, 65, 90, 97, 122, 
	59, 48, 57, 65, 90, 97, 122, 59, 
	84, 116, 48, 57, 65, 90, 97, 122, 
	59, 48, 57, 65, 90, 97, 122, 59, 
	84, 116, 48, 57, 65, 90, 97, 122, 
	59, 48, 57, 65, 90, 97, 122, 59, 
	85, 117, 48, 57, 65, 90, 97, 122, 
	59, 79, 111, 48, 57, 65, 90, 97, 
	122, 59, 84, 116, 48, 57, 65, 90, 
	97, 122, 59, 48, 57, 65, 90, 97, 
	122, 59, 77, 109, 112, 48, 57, 65, 
	90, 97, 122, 59, 111, 48, 57, 65, 
	90, 97, 122, 59, 115, 48, 57, 65, 
	90, 97, 122, 59, 48, 57, 65, 90, 
	97, 122, 59, 98, 48, 57, 65, 90, 
	97, 122, 59, 115, 48, 57, 65, 90, 
	97, 122, 59, 112, 48, 57, 65, 90, 
	97, 122, 59, 48, 57, 65, 90, 97, 
	122, 45, 45, 0, 10, 45, 0, 10, 
	45, 0, 10, 45, 0, 10, 45, 62, 
	32, 47, 62, 95, 45, 58, 65, 90, 
	97, 122, 32, 47, 62, 65, 90, 97, 
	122, 32, 65, 90, 97, 122, 32, 45, 
	47, 61, 62, 95, 48, 58, 65, 90, 
	97, 122, 62, 34, 39, 0, 10, 34, 
	32, 47, 62, 60, 47, 95, 65, 90, 
	97, 122, 32, 62, 95, 45, 46, 48, 
	58, 65, 90, 97, 122, 62, 47, 60, 
	0, 10, 39, 93, 9, 32, 40, 91, 
	91, 93, 93, 41, 41, 42, 96, 33, 
	38, 42, 45, 60, 91, 92, 95, 96, 
	124, 126, 33, 38, 42, 45, 60, 124, 
	126, 91, 92, 95, 96, 91, 35, 65, 
	71, 76, 81, 97, 103, 108, 110, 113, 
	48, 57, 66, 90, 98, 122, 42, 42, 
	95, 45, 33, 95, 65, 90, 97, 122, 
	60, 93, 33, 47, 58, 64, 91, 96, 
	123, 126, 42, 95, 96, 9, 32, 126, 
	0
};

static const char _md_inline_single_lengths[] = {
	1, 1, 4, 1, 1, 1, 1, 1, 
	2, 1, 0, 1, 1, 3, 3, 1, 
	3, 1, 3, 1, 3, 3, 3, 1, 
	4, 2, 2, 1, 2, 2, 2, 1, 
	1, 1, 3, 3, 3, 4, 4, 3, 
	1, 6, 1, 2, 3, 3, 1, 1, 
	1, 3, 1, 2, 3, 1, 4, 1, 
	1, 1, 1, 1, 1, 1, 11, 7, 
	1, 10, 1, 2, 0, 1, 0, 2, 
	0, 1, 1, 0, 0, 2, 1, 2, 
	1
};

static const char _md_inline_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 1, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	0, 0, 0, 0, 0, 0, 3, 2, 
	2, 3, 0, 0, 0, 0, 0, 0, 
	2, 4, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 2, 
	0, 3, 0, 0, 0, 0, 0, 2, 
	0, 0, 0, 4, 0, 0, 0, 0, 
	0
};

static const short _md_inline_index_offsets[] = {
	0, 2, 4, 9, 11, 13, 15, 17, 
	19, 23, 26, 30, 35, 40, 47, 54, 
	59, 66, 71, 78, 83, 90, 97, 104, 
	109, 117, 123, 129, 134, 140, 146, 152, 
	157, 159, 161, 165, 169, 173, 178, 186, 
	192, 196, 206, 208, 211, 215, 219, 221, 
	223, 227, 235, 237, 240, 244, 246, 251, 
	253, 255, 257, 259, 261, 263, 265, 277, 
	287, 289, 303, 305, 308, 309, 311, 312, 
	317, 318, 320, 322, 327, 328, 331, 333, 
	336
};

static const unsigned char _md_inline_indicies[] = {
	2, 1, 4, 3, 5, 5, 6, 7, 
	0, 7, 0, 9, 8, 11, 10, 13, 
	12, 15, 14, 17, 17, 16, 0, 19, 
	18, 0, 20, 20, 20, 0, 19, 20, 
	20, 20, 0, 22, 21, 21, 21, 0, 
	22, 23, 23, 21, 21, 21, 0, 22, 
	24, 24, 21, 21, 21, 0, 25, 21, 
	21, 21, 0, 22, 26, 26, 21, 21, 
	21, 0, 27, 21, 21, 21, 0, 22, 
	28, 28, 21, 21, 21, 0, 29, 21, 
	21, 21, 0, 22, 30, 30, 21, 21, 
	21, 0, 22, 31, 31, 21, 21, 21, 
	0, 22, 32, 32, 21, 21, 21, 0, 
	33, 21, 21, 21, 0, 22, 23, 23, 
	34, 21, 21, 21, 0, 22, 35, 21, 
	21, 21, 0, 22, 36, 21, 21, 21, 
	0, 37, 21, 21, 21, 0, 22, 38, 
	21, 21, 21, 0, 22, 39, 21, 21, 
	21, 0, 22, 40, 21, 21, 21, 0, 
	41, 21, 21, 21, 0, 42, 0, 43, 
	0, 0, 0, 45, 44, 0, 0, 47, 
	46, 0, 0, 48, 46, 0, 0, 48, 
	49, 46, 50, 52, 53, 51, 51, 51, 
	51, 0, 54, 55, 56, 57, 57, 0, 
	54, 57, 57, 0, 58, 59, 60, 61, 
	62, 59, 59, 59, 59, 0, 63, 0, 
	64, 65, 0, 0, 0, 67, 66, 68, 
	69, 70, 0, 73, 72, 74, 71, 75, 
	75, 75, 71, 76, 78, 77, 77, 77, 
	77, 77, 71, 63, 71, 74, 73, 71, 
	0, 0, 67, 79, 81, 80, 82, 82, 
	83, 84, 0, 84, 0, 86, 85, 88, 
	87, 90, 89, 92, 91, 94, 93, 96, 
	95, 98, 99, 100, 101, 102, 103, 104, 
	105, 106, 107, 108, 97, 109, 109, 109, 
	109, 109, 109, 109, 109, 109, 97, 110, 
	109, 111, 112, 113, 114, 115, 116, 113, 
	114, 117, 115, 21, 21, 21, 109, 119, 
	118, 94, 94, 118, 118, 120, 109, 121, 
	122, 123, 123, 123, 109, 124, 127, 126, 
	129, 128, 130, 130, 130, 130, 109, 131, 
	132, 94, 118, 109, 133, 135, 135, 134, 
	94, 109, 0
};

static const char _md_inline_trans_targs[] = {
	62, 1, 2, 1, 2, 3, 6, 4, 
	5, 62, 5, 62, 7, 62, 7, 62, 
	9, 10, 9, 62, 11, 12, 62, 14, 
	15, 62, 17, 62, 19, 62, 21, 22, 
	23, 62, 25, 26, 27, 62, 29, 30, 
	31, 62, 33, 34, 35, 36, 35, 36, 
	37, 72, 39, 38, 42, 73, 40, 42, 
	73, 41, 39, 41, 42, 43, 73, 62, 
	44, 52, 44, 45, 39, 42, 73, 62, 
	46, 47, 48, 49, 50, 49, 62, 52, 
	53, 54, 55, 58, 56, 57, 62, 57, 
	62, 59, 62, 59, 62, 62, 68, 61, 
	62, 63, 64, 65, 66, 69, 71, 74, 
	75, 77, 78, 79, 80, 62, 0, 8, 
	13, 16, 18, 20, 24, 28, 62, 67, 
	70, 62, 32, 38, 62, 62, 46, 51, 
	53, 54, 76, 62, 60, 61, 62, 79
};

static const unsigned char _md_inline_trans_actions[] = {
	33, 11, 50, 0, 13, 0, 0, 0, 
	15, 120, 0, 62, 15, 124, 0, 65, 
	11, 11, 0, 116, 0, 0, 23, 0, 
	0, 83, 0, 92, 0, 89, 0, 0, 
	0, 80, 0, 0, 0, 86, 0, 0, 
	0, 95, 0, 0, 3, 35, 0, 7, 
	7, 9, 41, 0, 41, 155, 0, 0, 
	136, 3, 145, 0, 145, 44, 168, 104, 
	3, 3, 0, 0, 112, 112, 161, 31, 
	0, 47, 0, 1, 5, 0, 140, 0, 
	0, 13, 0, 0, 0, 15, 128, 0, 
	68, 15, 132, 0, 71, 29, 0, 0, 
	53, 0, 21, 101, 15, 11, 150, 21, 
	0, 98, 21, 15, 15, 27, 0, 0, 
	0, 0, 0, 0, 0, 0, 74, 0, 
	0, 59, 0, 1, 38, 25, 3, 108, 
	11, 50, 11, 56, 0, 11, 77, 0
};

static const unsigned char _md_inline_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 17, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

static const unsigned char _md_inline_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 19, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

static const short _md_inline_eof_trans[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 72, 72, 
	72, 72, 72, 72, 1, 1, 1, 1, 
	1, 1, 1, 1, 94, 1, 0, 110, 
	110, 110, 119, 119, 119, 110, 122, 110, 
	125, 126, 110, 110, 132, 119, 110, 135, 
	110
};

static const int md_inline_start = 62;
static const int md_inline_first_final = 62;
static const int md_inline_error = -1;

static const int md_inline_en_main = 62;


#line 1371 "xs6/xsedit/markdown/kprMarkdownParser.rl"


#define unescapeHexa(X) \
	((('0' <= (X)) && ((X) <= '9')) \
		? ((X) - '0') \
		: ((('a' <= (X)) && ((X) <= 'f')) \
			? (10 + (X) - 'a') \
			: (10 + (X) - 'A')))

#pragma unused (md_inline_en_main)

FskErr KprMarkdownParseInline(KprMarkdownParser parser, char *str, SInt32 offset, SInt32 length, SInt32 flags, SInt32 inlineIndex, KprMarkdownRun inlineRun)
{
	char *p = str + offset, *pe = p + length, *eof = pe, *te, *ts;
	char *t1, *t2, *v1, *v2;
	SInt32 act, cs;
	UInt32 codepoints[2];
	
	FskErr err = kFskErrNone;
	KprMarkdownRunRecord runRecord;
	KprMarkdownRun run = &runRecord;
	KprMarkdownStateRecord stateRecord;
	KprMarkdownState state = &stateRecord;
	doResetState(parser, str, state, 0xF);
	doSetStateFlags(parser, str, state, flags);
	doSetStateOffset(parser, str, state, offset);
	
	
#line 2686 "xs6/xsedit/markdown/kprMarkdownParser.c"
	{
	cs = md_inline_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 2694 "xs6/xsedit/markdown/kprMarkdownParser.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
_resume:
	_acts = _md_inline_actions + _md_inline_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 30:
#line 1 "NONE"
	{ts = p;}
	break;
#line 2713 "xs6/xsedit/markdown/kprMarkdownParser.c"
		}
	}

	_keys = _md_inline_trans_keys + _md_inline_key_offsets[cs];
	_trans = _md_inline_index_offsets[cs];

	_klen = _md_inline_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _md_inline_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _md_inline_indicies[_trans];
_eof_trans:
	cs = _md_inline_trans_targs[_trans];

	if ( _md_inline_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _md_inline_actions + _md_inline_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 1399 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttribute(parser, str, state));
		}
	break;
	case 1:
#line 1403 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttributeName(parser, str, state));
		}
	break;
	case 2:
#line 1407 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttributeValue(parser, str, state));
		}
	break;
	case 3:
#line 1411 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			// nop
		}
	break;
	case 4:
#line 1415 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doEnterElement(parser, str, state));
		}
	break;
	case 5:
#line 1419 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doExitElement(parser, str, state));
		}
	break;
	case 6:
#line 1423 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doHold(parser, str, state, p, state->line));
		}
	break;
	case 7:
#line 1427 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doMark(parser, str, state, p));
		}
	break;
	case 8:
#line 1431 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doProcessTextSpecial(parser, str, state, p, state->line));
		}
	break;
	case 9:
#line 1435 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartComment(parser, str, state, p));
		}
	break;
	case 10:
#line 1439 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartElement(parser, str, state, p));
		}
	break;
	case 11:
#line 1443 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartText(parser, str, state, p));
		}
	break;
	case 12:
#line 1447 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopComment(parser, str, state, p));
		}
	break;
	case 13:
#line 1451 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopElement(parser, str, state, p));
		}
	break;
	case 14:
#line 1455 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopText(parser, str, state, p));
		}
	break;
	case 15:
#line 1459 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopText(parser, str, state, p - 1));
		}
	break;
	case 16:
#line 1463 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			// nop
		}
	break;
	case 17:
#line 1482 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ t1 = p; }
	break;
	case 18:
#line 1483 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ t2 = p; }
	break;
	case 19:
#line 1484 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ v1 = p; }
	break;
	case 20:
#line 1485 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ v2 = p; }
	break;
	case 21:
#line 1487 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			if ((*t1 == 'X') || (*t1 == 'x')) {
				t1++;
				while (t1 < t2) {
					codepoints[0] *= 0x10;
					codepoints[0] += unescapeHexa(*t1);
					t1++;
				}
			}
			else {
				while (t1 < t2) {
					codepoints[0] *= 10;
					codepoints[0] += ((*t1) - '0');
					t1++;
				}
			}
		}
	break;
	case 22:
#line 1513 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = codepoints[1] = 0; }
	break;
	case 23:
#line 1515 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x00022; }
	break;
	case 24:
#line 1516 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x00026; }
	break;
	case 25:
#line 1517 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x00027; }
	break;
	case 26:
#line 1518 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x0003C; }
	break;
	case 27:
#line 1519 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x0003E; }
	break;
	case 28:
#line 1520 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x000A0; }
	break;
	case 31:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 32:
#line 1554 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p+1;{
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownCodeSpan;
				runRecord.colors = kprMarkdownSpanMarkerColor | kprMarkdownCodeSpanColorMix;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.s.t.length = t2 - t1;
				runRecord.u.s.t.offset = t1 - str - runRecord.offset;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
			}}
	break;
	case 33:
#line 1663 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p+1;{ // union special case: u.codepoints
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownHTMLEntity;
				runRecord.colors = kprMarkdownSpanMarkerColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.codepoints[0] = codepoints[0];
				runRecord.u.codepoints[1] = codepoints[1];
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
			}}
	break;
	case 34:
#line 1674 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p+1;{
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.count = kprMarkdownCountDirective;
				runRecord.type = kprMarkdownImageReferenceSpan; // not displayed, not searched
				runRecord.colors = kprMarkdownSpanMarkerColor | kprMarkdownTextSpanColorMix;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.s.t.length = t2 - t1;
				runRecord.u.s.t.offset = t1 - str - runRecord.offset;
				runRecord.u.s.v.length = v2 - v1;
				runRecord.u.s.v.offset = v1 - str - runRecord.offset;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
			}}
	break;
	case 35:
#line 1688 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p+1;{
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.count = kprMarkdownCountDirective;
				runRecord.type = kprMarkdownImageSpan; // not displayed, not searched
				runRecord.colors = kprMarkdownSpanMarkerColor | kprMarkdownTextSpanColorMix;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.s.t.length = t2 - t1;
				runRecord.u.s.t.offset = t1 - str - runRecord.offset;
				runRecord.u.s.v.length = v2 - v1;
				runRecord.u.s.v.offset = v1 - str - runRecord.offset;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
			}}
	break;
	case 36:
#line 1702 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p+1;{
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.count = kprMarkdownCountDirective;
				runRecord.type = kprMarkdownLinkReferenceSpan;
				runRecord.colors = kprMarkdownSpanMarkerColor | kprMarkdownTextSpanColorMix;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.s.t.length = t2 - t1;
				runRecord.u.s.t.offset = t1 - str - runRecord.offset;
				runRecord.u.s.v.length = v2 - v1;
				runRecord.u.s.v.offset = v1 - str - runRecord.offset;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
			}}
	break;
	case 37:
#line 1716 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p+1;{
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.count = kprMarkdownCountDirective;
				runRecord.type = kprMarkdownLinkSpan;
				runRecord.colors = kprMarkdownSpanMarkerColor | kprMarkdownTextSpanColorMix;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.s.t.length = t2 - t1;
				runRecord.u.s.t.offset = t1 - str - runRecord.offset;
				runRecord.u.s.v.length = v2 - v1;
				runRecord.u.s.v.offset = v1 - str - runRecord.offset;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
			}}
	break;
	case 38:
#line 1730 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p+1;{
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.count = kprMarkdownCountDirective;
				runRecord.type = kprMarkdownMarkupSpan;
				runRecord.colors = kprMarkdownSpanMarkupColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.shaper = (state->depth == -1) ? 0 : 1;
				runRecord.u.element = state->stack[0];
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				state->stack[0] = NULL;
			}}
	break;
	case 39:
#line 1541 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownTextSpan;
				runRecord.colors = kprMarkdownSpanMarkerColor | kprMarkdownTextColorMix;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.s.t.length = t2 - t1;
				runRecord.u.s.t.offset = t1 - str - runRecord.offset;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
			}}
	break;
	case 40:
#line 1567 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownDoubleDash;
				runRecord.colors = kprMarkdownSpanMarkerColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.s.t.length = t2 - t1;
				runRecord.u.s.t.offset = t1 - str - runRecord.offset;
				runRecord.u.s.v.length = -1;
				runRecord.u.s.v.offset = 0;
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
			}}
	break;
	case 41:
#line 1580 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				int matched = 0;
				int size = (int)(v2 - v1);
				t1 = str + offset;
				t2 = str + offset + length;
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownFontStyle;
				runRecord.colors = kprMarkdownSpanMarkerColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.s.t.length = -1;
				runRecord.u.s.t.offset = 0;
				runRecord.u.s.v.length = v2 - v1;
				runRecord.u.s.v.offset = v1 - str - runRecord.offset;
				if (size == 1) {
					if (parser->fontStyle & kprMarkdownEmphasis) {
						if ((matched = (
										(v1 == t1 || !_IsSpace(*(v1 - 1)))
									&&
										(v2 == t2 || _IsSpace(*v2) || _IsPunct(*v2))
						))) parser->fontStyle ^= kprMarkdownEmphasis;
					}
					else {
						if ((matched = (
										(v1 == t1 || _IsSpace(*(v1 - 1)))
									&&
										(v2 == t2 || !_IsSpace(*v2))
						))) parser->fontStyle ^= kprMarkdownEmphasis;
					}
				}
				else if (size == 2) {
					if ((matched = (*v1 == '~'))) {
						parser->fontStyle ^= kprMarkdownStrikethrough;
					}
					else {
						if (parser->fontStyle & kprMarkdownStrong) {
							if ((matched = (
											(v1 == t1 || !_IsSpace(*(v1 - 1)))
										&&
											(v2 == t2 || _IsSpace(*v2) || _IsPunct(*v2))
							))) parser->fontStyle ^= kprMarkdownStrong;
						}
						else {
							if ((matched = (
											(v1 == t1 || _IsSpace(*(v1 - 1)))
										&&
											(v2 == t2 || !_IsSpace(*v2))
							))) parser->fontStyle ^= kprMarkdownStrong;
						}
					}
				}
				else if (size == 3) {
					int strongEmphasis = kprMarkdownStrong | kprMarkdownEmphasis;
					if ((parser->fontStyle & strongEmphasis) == strongEmphasis) {
						if ((matched = (
										(v1 == t1 || !_IsSpace(*(v1 - 1)))
									&&
										(v2 == t2 || _IsSpace(*v2) || _IsPunct(*v2))
						))) parser->fontStyle &= ~strongEmphasis;
					}
					else {
						if ((matched = (
										(v1 == t1 || _IsSpace(*(v1 - 1)))
									&&
										(v2 == t2 || !_IsSpace(*v2))
						))) parser->fontStyle |= strongEmphasis;
					}
				}
				if (matched) {
					runRecord.shaper = parser->fontStyle;
					bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				}
				else if (!_mergeTextSpan(parser, str, offset, ts, te)) {
					runRecord.type = kprMarkdownTextSpan;
					runRecord.colors = kprMarkdownTextColor;
					runRecord.u.s.t.length = runRecord.u.s.v.length;
					runRecord.u.s.t.offset = runRecord.u.s.v.offset;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
					bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				}
			}}
	break;
	case 42:
#line 1730 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.count = kprMarkdownCountDirective;
				runRecord.type = kprMarkdownMarkupSpan;
				runRecord.colors = kprMarkdownSpanMarkupColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.shaper = (state->depth == -1) ? 0 : 1;
				runRecord.u.element = state->stack[0];
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				state->stack[0] = NULL;
			}}
	break;
	case 43:
#line 1743 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownTableColumnDivider;
				runRecord.colors = kprMarkdownLineMarkerColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.s.t.length = -1;
				runRecord.u.s.t.offset = 0;
				runRecord.u.s.v.length = v2 - v1;
				runRecord.u.s.v.offset = v1 - str - runRecord.offset;
				(void)_mergeTableColumnDivider(parser, str, offset, ts, te, run);
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, inlineIndex, (void **)&inlineRun);
				if (inlineRun->type == kprMarkdownParagraphLine) {
					inlineRun->type = kprMarkdownTableLine; // change table line type
				}
			}}
	break;
	case 44:
#line 1763 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				// caution: runs concurrently with other line patterns
				bailIfError(doProcessMarkup(parser, str, state, NULL, -1));
				if (inlineRun && (length == (te - ts))) {
					// optimization where text is the span text
					// there is no need to record any item
				}
				else if (!_mergeTextSpan(parser, str, offset, ts, te)) {
					FskMemSet(run, 0, sizeof(runRecord));
					runRecord.type = kprMarkdownTextSpan;
					runRecord.colors = kprMarkdownTextColor;
					runRecord.length = te - ts;
					runRecord.offset = ts - str;
					runRecord.u.s.t.length = runRecord.length;
					runRecord.u.s.t.offset = runRecord.offset - runRecord.offset;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
					bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				}
			}}
	break;
	case 45:
#line 1580 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{{p = ((te))-1;}{
				int matched = 0;
				int size = (int)(v2 - v1);
				t1 = str + offset;
				t2 = str + offset + length;
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownFontStyle;
				runRecord.colors = kprMarkdownSpanMarkerColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.s.t.length = -1;
				runRecord.u.s.t.offset = 0;
				runRecord.u.s.v.length = v2 - v1;
				runRecord.u.s.v.offset = v1 - str - runRecord.offset;
				if (size == 1) {
					if (parser->fontStyle & kprMarkdownEmphasis) {
						if ((matched = (
										(v1 == t1 || !_IsSpace(*(v1 - 1)))
									&&
										(v2 == t2 || _IsSpace(*v2) || _IsPunct(*v2))
						))) parser->fontStyle ^= kprMarkdownEmphasis;
					}
					else {
						if ((matched = (
										(v1 == t1 || _IsSpace(*(v1 - 1)))
									&&
										(v2 == t2 || !_IsSpace(*v2))
						))) parser->fontStyle ^= kprMarkdownEmphasis;
					}
				}
				else if (size == 2) {
					if ((matched = (*v1 == '~'))) {
						parser->fontStyle ^= kprMarkdownStrikethrough;
					}
					else {
						if (parser->fontStyle & kprMarkdownStrong) {
							if ((matched = (
											(v1 == t1 || !_IsSpace(*(v1 - 1)))
										&&
											(v2 == t2 || _IsSpace(*v2) || _IsPunct(*v2))
							))) parser->fontStyle ^= kprMarkdownStrong;
						}
						else {
							if ((matched = (
											(v1 == t1 || _IsSpace(*(v1 - 1)))
										&&
											(v2 == t2 || !_IsSpace(*v2))
							))) parser->fontStyle ^= kprMarkdownStrong;
						}
					}
				}
				else if (size == 3) {
					int strongEmphasis = kprMarkdownStrong | kprMarkdownEmphasis;
					if ((parser->fontStyle & strongEmphasis) == strongEmphasis) {
						if ((matched = (
										(v1 == t1 || !_IsSpace(*(v1 - 1)))
									&&
										(v2 == t2 || _IsSpace(*v2) || _IsPunct(*v2))
						))) parser->fontStyle &= ~strongEmphasis;
					}
					else {
						if ((matched = (
										(v1 == t1 || _IsSpace(*(v1 - 1)))
									&&
										(v2 == t2 || !_IsSpace(*v2))
						))) parser->fontStyle |= strongEmphasis;
					}
				}
				if (matched) {
					runRecord.shaper = parser->fontStyle;
					bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				}
				else if (!_mergeTextSpan(parser, str, offset, ts, te)) {
					runRecord.type = kprMarkdownTextSpan;
					runRecord.colors = kprMarkdownTextColor;
					runRecord.u.s.t.length = runRecord.u.s.v.length;
					runRecord.u.s.t.offset = runRecord.u.s.v.offset;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
					bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				}
			}}
	break;
	case 46:
#line 1730 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{{p = ((te))-1;}{
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.count = kprMarkdownCountDirective;
				runRecord.type = kprMarkdownMarkupSpan;
				runRecord.colors = kprMarkdownSpanMarkupColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.shaper = (state->depth == -1) ? 0 : 1;
				runRecord.u.element = state->stack[0];
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				state->stack[0] = NULL;
			}}
	break;
	case 47:
#line 1763 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{{p = ((te))-1;}{
				// caution: runs concurrently with other line patterns
				bailIfError(doProcessMarkup(parser, str, state, NULL, -1));
				if (inlineRun && (length == (te - ts))) {
					// optimization where text is the span text
					// there is no need to record any item
				}
				else if (!_mergeTextSpan(parser, str, offset, ts, te)) {
					FskMemSet(run, 0, sizeof(runRecord));
					runRecord.type = kprMarkdownTextSpan;
					runRecord.colors = kprMarkdownTextColor;
					runRecord.length = te - ts;
					runRecord.offset = ts - str;
					runRecord.u.s.t.length = runRecord.length;
					runRecord.u.s.t.offset = runRecord.offset - runRecord.offset;
					runRecord.u.s.v.length = -1;
					runRecord.u.s.v.offset = 0;
					bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
				}
			}}
	break;
#line 3352 "xs6/xsedit/markdown/kprMarkdownParser.c"
		}
	}

_again:
	_acts = _md_inline_actions + _md_inline_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 29:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 3365 "xs6/xsedit/markdown/kprMarkdownParser.c"
		}
	}

	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _md_inline_eof_trans[cs] > 0 ) {
		_trans = _md_inline_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	}

#line 1788 "xs6/xsedit/markdown/kprMarkdownParser.rl"

	
	if ((cs == md_inline_error) || (cs < md_inline_first_final)) {
		err = kFskErrScript;
	}
bail:
	if (inlineRun) {
		FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, inlineIndex, (void **)&inlineRun);
		inlineRun->count = (SInt32)FskGrowableArrayGetItemCount((FskGrowableArray)parser->runs) - inlineRun->index;
		if (err == kFskErrNone) {
			KprMarkdownRun spanRun = NULL;
			SInt32 count = inlineRun->count, index;
			for (index = 0; index < count; index++) {
				SInt32 spanIndex = inlineRun->index + index;
				FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, spanIndex, (void **)&spanRun);
				if (spanRun->count == kprMarkdownCountDirective) {
					spanRun->index = (SInt32)FskGrowableArrayGetItemCount((FskGrowableArray)parser->runs);
					if (spanRun->type == kprMarkdownMarkupSpan) { // union special case: u.element
						if (spanRun->u.element && spanRun->u.element->t.length > 0) {
							bailIfError(KprMarkdownParseInline(parser, str, spanRun->offset + spanRun->u.element->t.offset, spanRun->u.element->t.length, flags, spanIndex, spanRun));
							FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, spanIndex, (void **)&spanRun);
						}
						else
							spanRun->count = 0;
					}
					else {
						bailIfError(KprMarkdownParseInline(parser, str, spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length, flags, spanIndex, spanRun));
						FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, spanIndex, (void **)&spanRun);
					}
				}
			}
			for (index = 0; index < count; index++) {
				FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, inlineRun->index + index, (void **)&spanRun);
				if (!((spanRun->count == 0) && (spanRun->type == kprMarkdownTextSpan))) break;
				length -= spanRun->u.s.t.length;
			}
			if ((length == 0) && (index == count)) {
				// optimization where text is the line text
				// there is no need to record individual items as they are all text
				FskGrowableArraySetItemCount((FskGrowableArray)parser->runs, inlineRun->index);
				inlineRun->count = 0;
			}
		}
	}
	return err;
}

#ifdef mxDebug

#define INDENT 5

FskErr KprMarkdownPrintDebugInfo(KprMarkdownParser parser, char *str, SInt32 flags)
{
	KprMarkdownRun lineRun = NULL;
	SInt32 depth = 0, indent = (depth + 1) * INDENT;
	SInt32 lineCount = parser->lineCount, lineIndex;
	for (lineIndex = 0; lineIndex < lineCount; lineIndex++) {
		FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, lineIndex, (void **)&lineRun);
		if (flags & kprMarkdownDebug) {
			switch (lineRun->type) {
			
				case kprMarkdownBlankLine:
					fprintf(stdout, "%d%*d%*d blank_line\n", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker);
					break;
					
				case kprMarkdownBlockquoteLine:
					fprintf(stdout, "%d%*d%*d blockquote_line: ", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker);
					fwrite(str + lineRun->offset + lineRun->u.s.t.offset, lineRun->u.s.t.length, 1, stdout);
					fprintf(stdout, "\n");
					break;
					
				case kprMarkdownFencedCodeBeginLine:
					if (lineRun->u.s.v.length > 0) {
						fprintf(stdout, "%d%*d%*d fenced_code_begin_line: ", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker);
						fwrite(str + lineRun->offset + lineRun->u.s.v.offset, lineRun->u.s.v.length, 1, stdout);
						fprintf(stdout, "\n");
					}
					else {
						fprintf(stdout, "%d%*d%*d fenced_code_begin_line\n", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker);
					}
					break;
					
				case kprMarkdownFencedCodeEndLine:
					fprintf(stdout, "%d%*d%*d fenced_code_end_line\n", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker);
					break;
					
				case kprMarkdownFencedCodeLine:
					fprintf(stdout, "%d%*d%*d fenced_code_line[%d]: ", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker, (int)lineRun->shaper);
					fwrite(str + lineRun->offset + lineRun->u.s.t.offset, lineRun->u.s.t.length, 1, stdout);
					fprintf(stdout, "\n");
					break;
					
				case kprMarkdownHeaderLine:
					fprintf(stdout, "%d%*d%*d header_line[%d]: ", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker, (int)lineRun->shaper);
					fwrite(str + lineRun->offset + lineRun->u.s.t.offset, lineRun->u.s.t.length, 1, stdout);
					fprintf(stdout, "\n");
					break;
					
				case kprMarkdownHeaderZeroLine:
					fprintf(stdout, "%d%*d%*d header_line___\n", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker);
					break;
					
				case kprMarkdownHorizontalRuleLine:
					fprintf(stdout, "%d%*d%*d horizontal_rule_line\n", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker);
					break;
					
				case kprMarkdownIndentedCodeLine:
					fprintf(stdout, "%d%*d%*d indented_code_line[%d]: ", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker, (int)lineRun->shaper);
					if (lineRun->u.s.t.length > 0) {
						fwrite(str + lineRun->offset + lineRun->u.s.t.offset, lineRun->u.s.t.length, 1, stdout);
					}
					fprintf(stdout, "\n");
					break;
					
				case kprMarkdownListLine:
					fprintf(stdout, "%d%*d%*d list_line[%d]: ", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker, (int)lineRun->shaper);
					fwrite(str + lineRun->offset + lineRun->u.s.v.offset, lineRun->u.s.v.length, 1, stdout);
					fprintf(stdout, " | ");
					fwrite(str + lineRun->offset + lineRun->u.s.t.offset, lineRun->u.s.t.length, 1, stdout);
					fprintf(stdout, "\n");
					break;
					
				case kprMarkdownMarkupLine: // union special case: u.element
					fprintf(stdout, "%d%*d%*d markup_line[%d]: ", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker, (int)lineRun->shaper);
					fwrite(str + lineRun->offset, lineRun->length - 1, 1, stdout);
					fprintf(stdout, "\n");
					break;
					
				case kprMarkdownParagraphLine:
					fprintf(stdout, "%d%*d%*d paragraph_line[%d]: ", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker, (int)lineRun->shaper);
					fwrite(str + lineRun->offset + lineRun->u.s.t.offset, lineRun->u.s.t.length, 1, stdout);
					fprintf(stdout, "\n");
					break;
					
				case kprMarkdownReferenceDefinitionLine:
					fprintf(stdout, "%d%*d%*d reference_definition_line: ", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker);
					fwrite(str + lineRun->offset + lineRun->u.s.v.offset, lineRun->u.s.v.length, 1, stdout);
					fprintf(stdout, " | ");
					fwrite(str + lineRun->offset + lineRun->u.s.t.offset, lineRun->u.s.t.length, 1, stdout);
					fprintf(stdout, "\n");
					break;
					
				case kprMarkdownTableLine:
					fprintf(stdout, "%d%*d%*d table_line[%d]: ", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker, (int)lineRun->shaper);
					fwrite(str + lineRun->offset + lineRun->u.s.t.offset, lineRun->u.s.t.length, 1, stdout);
					fprintf(stdout, "\n");
					break;
					
				default:
					fprintf(stdout, "%d%*d%*d ?\n", (int)depth, (int)indent, (int)(lineIndex + 1), INDENT, (int)lineRun->marker);
					break;
			}
		}
		else if (flags & kprMarkdownDebugInline) {
			fprintf(stdout, "%*d\n", (int)(lineIndex + 1), INDENT);
		}
		if ((flags & kprMarkdownDebugInline) && lineRun->count && lineRun->index) {
			KprMarkdownPrintDebugInfoInline(parser, str, lineRun, depth + 1, flags);
		}
	}
	if (flags & kprMarkdownDebugSummaryInfo) {
		SInt32 itemCount = FskGrowableArrayGetItemCount((FskGrowableArray)parser->runs);
		SInt32 lineCount = 1, source = 0, storage = itemCount * sizeof(KprMarkdownRunRecord);
		char *s = str;
		while (*s) {
			source++;
			if (*s++ == '\n')
				lineCount++;
		}
		fprintf(stdout, "\n");
		fprintf(stdout, "# Em/Strike/Strong :%3d%3d%3d\n", (int)(parser->fontStyle & kprMarkdownEmphasis), (int)(parser->fontStyle & kprMarkdownStrikethrough), (int)(parser->fontStyle & kprMarkdownStrong));
		fprintf(stdout, "# Item Count       :%6d KprMarkdownRun\n", (int)itemCount);
		fprintf(stdout, "# Line Count       :%6d %s\n", (int)parser->lineCount, parser->lineCount == lineCount ? "Success" :"Failure");
		fprintf(stdout, "# Source           :%6d %s\n", (int)(source < 1024 ? source : source / 1024), source < 1024 ? "Bytes" : "KB");
		fprintf(stdout, "# Storage          :%6d %s\n", (int)(storage < 1024 ? storage : storage / 1024), storage < 1024 ? "Bytes" : "KB");
	}
	fprintf(stdout, "\n");
	return kFskErrNone;
}

FskErr KprMarkdownPrintDebugInfoInline(KprMarkdownParser parser, char *str, KprMarkdownRun inlineRun, SInt32 depth, SInt32 flags)
{
	KprMarkdownRun spanRun = NULL;
	SInt32 indent = (depth + 1) * INDENT;
	SInt32 spanCount = inlineRun->count, spanIndex, length, offset;
	for (spanIndex = 0; spanIndex < spanCount; spanIndex++) {
		FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, inlineRun->index + spanIndex, (void **)&spanRun);
		length = spanRun->length;
		offset = spanRun->offset - inlineRun->offset;
		if (flags & kprMarkdownDebugInlineColorDepth) {
			switch (depth) {
				case 1: fprintf(stdout, KGRN); break;
				case 2: fprintf(stdout, KYEL); break;
				case 3: fprintf(stdout, KRED); break;
				case 4: fprintf(stdout, KBLU); break;
				default: break;
			}
		}
		switch (spanRun->type) {
		
			case kprMarkdownCodeSpan:
				fprintf(stdout, "%d%*d%*d code_span: ", (int)depth, (int)indent, (int)offset, INDENT, (int)length);
				fwrite(str + spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length, 1, stdout);
				fprintf(stdout, "\n");
				break;
				
			case kprMarkdownDoubleDash:
				fprintf(stdout, "%d%*d%*d double_dash: ", (int)depth, (int)indent, (int)offset, INDENT, (int)length);
				fwrite(str + spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length, 1, stdout);
				fprintf(stdout, "\n");
				break;
				
			case kprMarkdownFontStyle:
				fprintf(stdout, "%d%*d%*d font_style: ", (int)depth, (int)indent, (int)offset, INDENT, (int)length);
				fwrite(str + spanRun->offset + spanRun->u.s.v.offset, spanRun->u.s.v.length, 1, stdout);
				fprintf(stdout, " (now %s)\n", _FontStyles[spanRun->shaper]);
				break;
				
			case kprMarkdownHTMLEntity: // union special case: u.codepoints
				fprintf(stdout, "%d%*d%*d html_entity: ", (int)depth, (int)indent, (int)offset, INDENT, (int)length);
				fwrite(str + spanRun->offset, spanRun->length, 1, stdout);
				fprintf(stdout, " ( codepoints 0x%0X 0x%0X )\n", (unsigned int)(spanRun->u.codepoints[0]), (unsigned int)(spanRun->u.codepoints[1]));
				break;
				
			case kprMarkdownImageReferenceSpan:
				fprintf(stdout, "%d%*d%*d image_ref_span: ", (int)depth, (int)indent, (int)offset, INDENT, (int)length);
				fwrite(str + spanRun->offset + spanRun->u.s.v.offset, spanRun->u.s.v.length, 1, stdout);
				fprintf(stdout, " | ");
				fwrite(str + spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length, 1, stdout);
				fprintf(stdout, "\n");
				break;
				
			case kprMarkdownImageSpan:
				fprintf(stdout, "%d%*d%*d image_span: ", (int)depth, (int)indent, (int)offset, INDENT, (int)length);
				fwrite(str + spanRun->offset + spanRun->u.s.v.offset, spanRun->u.s.v.length, 1, stdout);
				fprintf(stdout, " | ");
				fwrite(str + spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length, 1, stdout);
				fprintf(stdout, "\n");
				break;
				
			case kprMarkdownLinkReferenceSpan:
				fprintf(stdout, "%d%*d%*d link_ref_span: ", (int)depth, (int)indent, (int)offset, INDENT, (int)length);
				fwrite(str + spanRun->offset + spanRun->u.s.v.offset, spanRun->u.s.v.length, 1, stdout);
				fprintf(stdout, " | ");
				fwrite(str + spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length, 1, stdout);
				fprintf(stdout, "\n");
				break;
				
			case kprMarkdownLinkSpan:
				fprintf(stdout, "%d%*d%*d link_span: ", (int)depth, (int)indent, (int)offset, INDENT, (int)length);
				fwrite(str + spanRun->offset + spanRun->u.s.v.offset, spanRun->u.s.v.length, 1, stdout);
				fprintf(stdout, " | ");
				fwrite(str + spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length, 1, stdout);
				fprintf(stdout, "\n");
				break;
				
			case kprMarkdownMarkupSpan: // union special case: u.element
				fprintf(stdout, "%d%*d%*d markup_span[%d]: ", (int)depth, (int)indent, (int)offset, INDENT, (int)length, (int)spanRun->shaper);
				fwrite(str + spanRun->offset, spanRun->length, 1, stdout);
				if (spanRun->u.element && spanRun->u.element->t.length > 0) {
					fprintf(stdout, " | ");
					fwrite(str + spanRun->offset + spanRun->u.element->t.offset, spanRun->u.element->t.length, 1, stdout);
				}
				fprintf(stdout, "\n");
				break;
				
			case kprMarkdownTableColumnDivider:
				fprintf(stdout, "%d%*d%*d table_column_divider: ", (int)depth, (int)indent, (int)offset, INDENT, (int)length);
				fwrite(str + spanRun->offset + spanRun->u.s.v.offset, spanRun->u.s.v.length, 1, stdout);
				fprintf(stdout, "\n");
				break;
				
			case kprMarkdownTextSpan:
				fprintf(stdout, "%d%*d%*d text_span: ", (int)depth, (int)indent, (int)offset, INDENT, (int)length);
				fwrite(str + spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length, 1, stdout);
				fprintf(stdout, "\n");
				break;
				
			default:
				fprintf(stdout, "%d%*d%*d ?\n", (int)depth, (int)indent, (int)offset, INDENT, (int)length);
				break;
		}
		if (flags & kprMarkdownDebugInlineColorDepth) {
			fprintf(stdout, KNRM);
		}
		KprMarkdownPrintDebugInfoInline(parser, str, spanRun, depth + 1, flags);
	}
	return kFskErrNone;
}

FskErr KprMarkdownWriteHTML(KprMarkdownParser parser, char *str, FILE *stream)
{
	FskErr err = kFskErrNone;
	KprMarkdownRun lineRun = NULL;
	KprMarkdownWriteStack stack = NULL;
	KprMarkdownWriteState state = NULL;
	KprMarkdownWriteStateRecord stateRecord;
	SInt32 lineCount = parser->lineCount, lineIndex;
	
	stack = stateRecord.stack;
	stack[0].attr = stack[0].name = NULL;
	stack[0].marker = 0;
	state = &stateRecord;
	state->top = 0;
	
	// https://validator.w3.org/#validate_by_input
	// https://www.w3.org/TR/xhtml1/#guidelines
	printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"");
	printf("\n\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">");
	bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, "html", "xmlns=\"http://www.w3.org/1999/xhtml\""));
	bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, "head", NULL));
	printf("<meta charset=\"utf-8\" /><title>MARKDOWN</title>");
	bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 0));
	bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, "body", NULL));
	
	for (lineIndex = 0; lineIndex < lineCount; lineIndex++) {
		FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, lineIndex, (void **)&lineRun);
		if (lineRun->marker < stack[0].marker) {
			do {
				bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 1));
			} while (lineRun->marker < stack[0].marker);
		}
		switch (lineRun->type) {
		
			case kprMarkdownBlankLine:
				break;
				
			case kprMarkdownBlockquoteLine:
				if (lineRun->marker > stack[0].marker)
					bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, "blockquote", NULL));
				stack[0].marker = lineRun->marker;
				bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, "p", NULL));
				bailIfError(KprMarkdownWriteHTMLInline(parser, str, stream, lineRun));
				bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 0));
				break;
				
			case kprMarkdownFencedCodeBeginLine:
				break;
				
			case kprMarkdownFencedCodeEndLine:
				break;
				
			case kprMarkdownFencedCodeLine:
				bailIfError(KprMarkdownWriteHTMLCode(parser, str, stream, state, &lineIndex, lineCount));
				break;
				
			case kprMarkdownHeaderLine:
				switch (lineRun->shaper) {
					case 1: stack[0].name = "h1"; break;
					case 2: stack[0].name = "h2"; break;
					case 3: stack[0].name = "h3"; break;
					case 4: stack[0].name = "h4"; break;
					case 5: stack[0].name = "h5"; break;
					case 6: stack[0].name = "h6"; break;
					default: break;
				}
				bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, stack[0].name, NULL));
				bailIfError(KprMarkdownWriteHTMLInline(parser, str, stream, lineRun));
				bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 0));
				break;
				
			case kprMarkdownHeaderZeroLine:
				break;
				
			case kprMarkdownHorizontalRuleLine:
					fprintf(stream, "<hr />");
				break;
				
			case kprMarkdownIndentedCodeLine:
				bailIfError(KprMarkdownWriteHTMLCode(parser, str, stream, state, &lineIndex, lineCount));
				break;
				
			case kprMarkdownListLine:
				if (lineRun->marker > stack[0].marker) {
					stack[0].name = (lineRun->shaper < 0) ? "ol" : "ul";
					bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, stack[0].name, NULL));
				}
				else if (lineRun->marker == stack[0].marker)
					bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 1));
				stack[0].marker = lineRun->marker;
				KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, "li", NULL);
				if (lineRun->u.s.t.length > 0) {
					bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, "p", NULL));
					bailIfError(KprMarkdownWriteHTMLInline(parser, str, stream, lineRun));
					bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 0));
				}
				break;
				
			case kprMarkdownMarkupLine: // union special case: u.element
				fprintf(stream, "\n");
				fwrite(str + lineRun->offset, lineRun->length - 1, 1, stream);
				break;
				
			case kprMarkdownParagraphLine:
				bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, "p", NULL));
				bailIfError(KprMarkdownWriteHTMLInline(parser, str, stream, lineRun));
				bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 0));
				//if (lineRun->shaper > 0) // hard linebreak (for paragraph lines)
					//fprintf(stream, "<br />"); //@@ Marked 2?
				break;
				
			case kprMarkdownReferenceDefinitionLine:
				break;
				
			case kprMarkdownTableLine:
				bailIfError(KprMarkdownWriteHTMLTable(parser, str, stream, state, &lineIndex, lineCount));
				break;
				
			default:
				break;
		}
	}
	while (state->top > 0)
		bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 1));
	fprintf(stream, "\n");
	fflush(stream);
bail:
	return err;
}

FskErr KprMarkdownWriteHTMLCode(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownWriteState state, SInt32 *index, SInt32 count)
{
	FskErr err = kFskErrNone;
	KprMarkdownRun lineRun = NULL;
	SInt32 i = *index, c = count;
	bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, "pre", NULL));
	bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 0, "code", NULL));
	for (i = *index; i < c; i++) {
		FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, i, (void **)&lineRun);
		if ((lineRun->type == kprMarkdownFencedCodeLine) || (lineRun->type == kprMarkdownIndentedCodeLine)) {
			if (lineRun->u.s.t.length > 0) {
				bailIfError(KprMarkdownWriteHTMLInlineText(parser, str, stream, lineRun));
				fprintf(stream, "\n");
			}
		}
		else
			break;
	}
	*index = i - 1;
	bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 0));
	bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 0));
bail:
	return err;
}

FskErr KprMarkdownWriteHTMLInline(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownRun inlineRun)
{
	FskErr err = kFskErrNone;
	if (inlineRun->count && inlineRun->index) {
		KprMarkdownRun spanRun = NULL;
		Boolean _em = false, _strong = false;
		SInt32 spanCount = inlineRun->count, spanIndex;
		for (spanIndex = 0; spanIndex < spanCount; spanIndex++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, inlineRun->index + spanIndex, (void **)&spanRun);
			switch (spanRun->type) {
			
				case kprMarkdownCodeSpan:
					fprintf(stream, "<code>");
					bailIfError(KprMarkdownWriteHTMLInlineText(parser, str, stream, spanRun));
					fprintf(stream, "</code>");
					break;
					
				case kprMarkdownDoubleDash:
					// EN DASH (&#8211;) VS EM DASH (&#8212;)
					fprintf(stream, "&#8212;");
					break;
					
				case kprMarkdownFontStyle:
					switch (spanRun->shaper & 3) {
						case 0: _em = _strong = false; break;
						case 1: _em = true;            break;
						case 2: _strong = true;        break;
						case 3: _strong = _em = true;  break;
						default: break;
					}
					break;
					
				case kprMarkdownHTMLEntity: // union special case: u.codepoints
					fwrite(str + spanRun->offset, spanRun->length, 1, stream);
					break;
					
				case kprMarkdownImageReferenceSpan:
					bailIfError(kFskErrUnimplemented);
					break;
					
				case kprMarkdownImageSpan:
					fprintf(stream, "<figure>");
					if (spanRun->u.s.v.length > 0) {
						fprintf(stream, "<img src=\"");
						fwrite(str + spanRun->offset + spanRun->u.s.v.offset, spanRun->u.s.v.length, 1, stream);
						if (spanRun->u.s.t.length > 0) {
							fprintf(stream, "\" alt=\"");
							fwrite(str + spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length, 1, stream);
						}
						fprintf(stream, "\" />");
					}
					if (spanRun->u.s.t.length > 0) {
						fprintf(stream, "<figcaption>");
						fwrite(str + spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length, 1, stream);
						fprintf(stream, "</figcaption>");
					}
					fprintf(stream, "</figure>");
					break;
					
				case kprMarkdownLinkReferenceSpan:
					bailIfError(kFskErrUnimplemented);
					break;
					
				case kprMarkdownLinkSpan:
					fprintf(stream, "<a");
					if (spanRun->u.s.v.length > 0) {
						fprintf(stream, " href=\"");
						fwrite(str + spanRun->offset + spanRun->u.s.v.offset, spanRun->u.s.v.length, 1, stream);
						fprintf(stream, "\"");
					}
					fprintf(stream, ">");
					bailIfError(KprMarkdownWriteHTMLInline(parser, str, stream, spanRun));
					fprintf(stream, "</a>");
					break;
					
				case kprMarkdownMarkupSpan: // union special case: u.element
					fwrite(str + spanRun->offset, spanRun->length - 0, 1, stream);
					break;
					
				case kprMarkdownTableColumnDivider:
					break;
					
				case kprMarkdownTextSpan:
					if (_strong)
						fprintf(stream, "<strong>");
					if (_em)
						fprintf(stream, "<em>");
					bailIfError(KprMarkdownWriteHTMLInline(parser, str, stream, spanRun));
					if (_em)
						fprintf(stream, "</em>");
					if (_strong)
						fprintf(stream, "</strong>");
					break;
					
				default:
					break;
			}
		}
	}
	else if (inlineRun->u.s.t.length > 0)
		bailIfError(KprMarkdownWriteHTMLInlineText(parser, str, stream, inlineRun));
bail:
	return err;
}

FskErr KprMarkdownWriteHTMLInlineText(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownRun inlineRun)
{
	char *mark, *name = NULL, *start, *stop;
	mark = start = str + inlineRun->offset + inlineRun->u.s.t.offset;
	stop = start + inlineRun->u.s.t.length;
	while (start < stop) {
		switch ((unsigned char)*start) {
			case '"' : name = "quot"; break;
			case '&' : name = "amp";  break;
			case '\'': name = "apos"; break;
			case '<' : name = "lt";   break;
			case '>' : name = "gt";   break;
			case 160 : name = "nbsp"; break;
			default: break;
		}
		if (name) {
			if (mark < start)
				fwrite(mark, start - mark, 1, stream);
			fprintf(stream, "&%s;", name);
			mark = start + 1;
			name = NULL;
		}
		start++;
	}
	if (mark < start)
		fwrite(mark, start - mark, 1, stream);
	return kFskErrNone;
}

FskErr KprMarkdownWriteHTMLPop(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownWriteState state, SInt32 flag)
{
	fflush(stream);
	state->stack[0] = state->stack[(state->top)--];
	if (flag)
		fprintf(stream, "\n%*s%s%s", (int)(((state->top)*2)+2), "</",  state->stack[0].name, ">");
	else
		fprintf(stream, "%s%s%s", "</",  state->stack[0].name, ">");
	fflush(stream);
	return kFskErrNone;
}

FskErr KprMarkdownWriteHTMLPush(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownWriteState state, SInt32 flag, char *name, char *attr)
{
	fflush(stream);
	state->stack[0].name = name;
	if ((state->stack[0].attr = attr)) {
		if (flag)
			fprintf(stream, "\n%*s%s%s%s%s", (int)(((state->top)*2)+1), "<",  state->stack[0].name, " ", state->stack[0].attr, ">");
		else
			fprintf(stream, "%s%s%s%s%s", "<",  state->stack[0].name, " ", state->stack[0].attr, ">");
	}
	else {
		if (flag)
			fprintf(stream, "\n%*s%s%s", (int)(((state->top)*2)+1), "<",  state->stack[0].name, ">");
		else
			fprintf(stream, "%s%s%s", "<",  state->stack[0].name, ">");
	}
	state->stack[++(state->top)] = state->stack[0];
	fflush(stream);
	return kFskErrNone;
}

FskErr KprMarkdownWriteHTMLTable(KprMarkdownParser parser, char *str, FILE *stream, KprMarkdownWriteState state, SInt32 *index, SInt32 count)
{
	FskErr err = kFskErrNone;
	KprMarkdownRun lineRun = NULL;
	SInt32 i = *index, c = count;
	bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 1, "table", NULL));
	bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 0, "tbody", NULL));
	for (i = *index; i < c; i++) {
		FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, i, (void **)&lineRun);
		if (lineRun->type == kprMarkdownTableLine) {
			if (lineRun->count > 0) {
				SInt32 inlineCount = lineRun->count, inlineIndex, inlineMarker;
				bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 0, "tr", NULL));
				for (inlineIndex = inlineMarker = 0; inlineIndex < inlineCount; inlineIndex++) {
					KprMarkdownRun inlineRun = NULL;
					FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, lineRun->index + inlineIndex, (void **)&inlineRun);
					if ((inlineRun->type == kprMarkdownTableColumnDivider) || ((inlineIndex + 1) == inlineCount)) {
						if (inlineRun->type != kprMarkdownTableColumnDivider)
							inlineIndex++;
						if ((inlineIndex - inlineMarker) > 0) {
							for (; inlineMarker < inlineIndex; inlineMarker++) {
								FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, lineRun->index + inlineMarker, (void **)&inlineRun);
								bailIfError(KprMarkdownWriteHTMLPush(parser, str, stream, state, 0, "td", NULL));
								bailIfError(KprMarkdownWriteHTMLInlineText(parser, str, stream, inlineRun));
								bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 0));
							}
						}
						inlineMarker = inlineIndex + 1;
					}
				}
				bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 0));
			}
		}
		else
			break;
	}
	*index = i - 1;
	bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 0));
	bailIfError(KprMarkdownWriteHTMLPop(parser, str, stream, state, 0));
bail:
	return err;
}

#endif

// END OF FILE
