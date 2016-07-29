
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

Caution: environment variables ($F_HOME or $XS6) in ragel paths are expanded in the generated .c file!

         For example:
         #line 1 "/Users/mwharton/Development/kinoma/fsk/xs6/xsedit/markdown/kprMarkdownParser.rl"

         Compared to:
         #line 1 "xs6/xsedit/markdown/kprMarkdownParser.rl"

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
	char *start = state->textStart;
	SInt32 length = state->textStop - start, space = 0;
	if (state->depth > 0) {
		while (start < state->textStop) {
			if (!_IsSpace(*start))
				break;
			space++;
			start++;
		}
	}
	if ((length > space) || (state->depth == 0)) {
		if (state->depth > 0) {
			KprMarkdownElement element = state->stack[state->top];
			if (element) {
				if ((space == 1) && (*state->textStart == ' ')) {
					// The <code>top</code> and <code>bottom</code> coordinates...
					start = state->textStart;
				}
				if (element->elements == NULL) {
					bailIfError(FskGrowableArrayNew(sizeof(KprMarkdownElementRecord), kprMarkdownDefaultElementsOption, (FskGrowableArray*)&(element->elements)));
				}
				state->elementRecord.type = 0;
				state->elementRecord.t.length = state->textStop - start;
				state->elementRecord.t.offset = (SInt16)(start - str - state->offset);
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)element->elements, (void *)&(state->elementRecord)));
				state->elementRecord.t.length = -1;
				state->elementRecord.t.offset = 0;
			}
		}
		#if KPRMARKDOWNDEBUGTEXT
			fprintf(stdout, (state->depth ? KYEL "%6d " : KNRM "%6d "), (int)line);
			if (start) {
				fwrite(start, state->textStop - start, 1, stdout);
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


#line 596 "xs6/xsedit/markdown/kprMarkdownParser.c"
static const char _md_actions[] = {
	0, 1, 3, 1, 5, 1, 6, 1, 
	9, 1, 10, 1, 11, 1, 13, 1, 
	15, 1, 16, 1, 18, 1, 19, 1, 
	23, 1, 26, 1, 27, 1, 28, 1, 
	29, 1, 35, 1, 38, 1, 39, 1, 
	60, 1, 61, 2, 5, 37, 2, 7, 
	9, 2, 11, 15, 2, 11, 18, 2, 
	11, 19, 2, 12, 9, 2, 12, 11, 
	2, 13, 4, 2, 14, 1, 2, 15, 
	6, 2, 15, 41, 2, 15, 61, 2, 
	18, 17, 2, 18, 55, 2, 18, 56, 
	2, 19, 17, 2, 19, 55, 2, 19, 
	56, 2, 19, 57, 2, 20, 15, 2, 
	21, 15, 2, 22, 11, 2, 23, 11, 
	2, 23, 25, 2, 23, 26, 2, 23, 
	27, 2, 23, 29, 2, 24, 11, 2, 
	24, 34, 2, 25, 11, 2, 25, 26, 
	2, 25, 28, 2, 26, 11, 2, 27, 
	23, 2, 28, 11, 2, 32, 11, 2, 
	33, 11, 2, 40, 19, 3, 6, 7, 
	9, 3, 11, 18, 17, 3, 11, 19, 
	17, 3, 12, 11, 19, 3, 13, 4, 
	3, 3, 13, 5, 37, 3, 14, 2, 
	0, 3, 14, 8, 36, 3, 15, 30, 
	42, 3, 15, 30, 43, 3, 15, 31, 
	21, 3, 15, 31, 59, 3, 20, 15, 
	58, 3, 20, 21, 15, 3, 22, 24, 
	11, 3, 22, 28, 11, 3, 23, 25, 
	27, 3, 24, 22, 11, 3, 25, 11, 
	18, 3, 25, 11, 19, 3, 25, 28, 
	11, 3, 26, 11, 18, 3, 26, 11, 
	19, 3, 26, 27, 23, 3, 28, 11, 
	18, 3, 28, 11, 23, 3, 29, 18, 
	57, 3, 29, 19, 57, 3, 32, 22, 
	11, 3, 33, 22, 11, 3, 35, 25, 
	11, 3, 40, 18, 11, 3, 40, 18, 
	51, 3, 40, 18, 52, 3, 40, 18, 
	54, 3, 40, 19, 11, 3, 40, 19, 
	44, 3, 40, 19, 45, 3, 40, 19, 
	46, 3, 40, 19, 47, 3, 40, 19, 
	48, 3, 40, 19, 49, 3, 40, 19, 
	50, 3, 40, 19, 51, 3, 40, 19, 
	52, 3, 40, 19, 53, 3, 40, 19, 
	54, 4, 12, 11, 18, 17, 4, 12, 
	11, 19, 17, 4, 14, 1, 2, 0, 
	4, 14, 2, 0, 3, 4, 15, 6, 
	7, 9, 4, 20, 18, 55, 15, 4, 
	20, 19, 55, 15, 4, 22, 24, 28, 
	11, 4, 24, 34, 26, 11, 4, 25, 
	26, 27, 23, 4, 25, 28, 11, 18, 
	4, 25, 28, 11, 23, 4, 40, 11, 
	18, 54, 4, 40, 18, 51, 27, 4, 
	40, 18, 52, 27, 4, 40, 19, 46, 
	23, 4, 40, 23, 19, 51, 4, 40, 
	23, 19, 52, 4, 40, 23, 19, 54, 
	4, 40, 25, 18, 48, 4, 40, 27, 
	18, 45, 4, 40, 27, 18, 47, 4, 
	40, 27, 18, 49, 4, 40, 27, 18, 
	53, 4, 40, 29, 18, 50, 5, 14, 
	1, 2, 0, 3, 5, 40, 19, 44, 
	23, 11, 5, 40, 23, 19, 11, 54, 
	5, 40, 23, 19, 25, 48, 5, 40, 
	23, 19, 27, 49, 5, 40, 23, 19, 
	27, 53, 5, 40, 23, 19, 29, 50, 
	5, 40, 23, 19, 51, 27, 5, 40, 
	23, 19, 52, 27, 5, 40, 25, 18, 
	48, 27, 5, 40, 26, 27, 18, 47, 
	5, 40, 27, 19, 45, 23, 5, 40, 
	27, 19, 47, 23, 6, 40, 23, 19, 
	25, 48, 27, 6, 40, 25, 26, 27, 
	18, 47, 6, 40, 26, 27, 19, 47, 
	23, 6, 40, 28, 11, 19, 46, 23, 
	7, 40, 25, 26, 27, 19, 47, 23, 
	7, 40, 25, 28, 11, 19, 46, 23
	
};

static const char _md_cond_offsets[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 7, 
	14, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15
};

static const char _md_cond_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 7, 7, 
	1, 0, 0, 0, 0, 0, 0, 0, 
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
	0, 0, 0, 0
};

static const short _md_cond_keys[] = {
	-128, -1, 0, 0, 1, 9, 10, 10, 
	11, 12, 13, 13, 14, 127, -128, -1, 
	0, 0, 1, 9, 10, 10, 11, 12, 
	13, 13, 14, 127, 10, 10, 0
};

static const char _md_cond_spaces[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const short _md_key_offsets[] = {
	0, 0, 2, 4, 5, 11, 12, 13, 
	15, 17, 19, 22, 25, 28, 29, 39, 
	46, 51, 63, 64, 67, 69, 72, 75, 
	78, 81, 82, 85, 86, 92, 96, 108, 
	118, 120, 123, 127, 128, 132, 133, 134, 
	138, 142, 145, 159, 164, 169, 172, 173, 
	178, 184, 188, 195, 203, 204, 208, 212, 
	219, 225, 231, 234, 235, 239, 245, 249, 
	253, 259, 262, 263, 266, 272, 278, 284, 
	290, 296, 301, 309, 312, 313, 321, 322, 
	326, 330, 337, 345, 346, 350, 357, 363, 
	364, 365, 379, 380, 394, 408, 414, 417, 
	418, 423, 429, 435, 441, 447, 452, 458, 
	464, 469, 473, 478, 479, 483, 488, 492, 
	497, 498, 502, 508, 514, 519, 523, 528, 
	532, 537, 541, 546, 550, 557, 558, 566, 
	571, 576, 580, 584, 587, 590, 591, 595, 
	598, 599, 602, 606, 610, 613, 614, 617, 
	620, 623, 630, 631, 632, 634, 636, 638, 
	641, 644, 649, 660, 661, 671, 678, 683, 
	695, 696, 698, 701, 704, 707, 709, 711, 
	714, 714, 720, 720, 720, 736, 736, 738, 
	742, 742, 744, 750
};

static const short _md_trans_keys[] = {
	32, 60, 32, 60, 60, 33, 95, 65, 
	90, 97, 122, 45, 45, 10, 45, 10, 
	45, 10, 45, 10, 45, 62, 0, 10, 
	13, 0, 10, 13, 10, 32, 47, 62, 
	95, 45, 58, 65, 90, 97, 122, 32, 
	47, 62, 65, 90, 97, 122, 32, 65, 
	90, 97, 122, 32, 45, 47, 61, 62, 
	95, 48, 58, 65, 90, 97, 122, 62, 
	0, 10, 13, 34, 39, 0, 10, 34, 
	32, 47, 62, 0, 10, 39, 0, 10, 
	13, 10, 0, 10, 13, 10, 0, 9, 
	10, 13, 32, 96, 0, 10, 13, 96, 
	256, 266, 269, 352, 512, 522, 525, 608, 
	128, 383, 384, 639, 256, 266, 269, 512, 
	522, 525, 128, 383, 384, 639, 266, 522, 
	0, 10, 13, 0, 10, 13, 45, 10, 
	0, 10, 13, 61, 10, 10, 0, 10, 
	13, 32, 0, 10, 13, 32, 0, 10, 
	13, 0, 9, 10, 13, 32, 45, 58, 
	62, 96, 124, 42, 43, 48, 57, 0, 
	9, 10, 13, 32, 0, 9, 10, 13, 
	32, 0, 10, 13, 10, 0, 9, 10, 
	13, 32, 0, 9, 10, 13, 32, 45, 
	0, 10, 13, 45, 0, 9, 10, 13, 
	32, 45, 58, 0, 9, 10, 13, 32, 
	45, 58, 124, 10, 0, 10, 13, 45, 
	0, 10, 13, 45, 0, 9, 10, 13, 
	32, 45, 58, 0, 10, 13, 46, 48, 
	57, 0, 9, 10, 13, 32, 62, 0, 
	10, 13, 10, 0, 10, 13, 62, 0, 
	9, 10, 13, 32, 62, 0, 10, 13, 
	96, 0, 10, 13, 96, 0, 9, 10, 
	13, 32, 96, 0, 10, 13, 10, 0, 
	10, 13, 0, 9, 10, 13, 32, 96, 
	0, 9, 10, 13, 32, 96, 0, 9, 
	10, 13, 32, 96, 0, 9, 10, 13, 
	32, 96, 0, 9, 10, 13, 32, 96, 
	0, 9, 10, 13, 32, 0, 9, 10, 
	13, 32, 45, 58, 124, 0, 10, 13, 
	10, 0, 9, 10, 13, 32, 45, 58, 
	124, 10, 0, 10, 13, 45, 0, 10, 
	13, 45, 0, 9, 10, 13, 32, 45, 
	58, 0, 9, 10, 13, 32, 45, 58, 
	124, 10, 0, 10, 13, 45, 0, 9, 
	10, 13, 32, 45, 58, 0, 9, 10, 
	13, 32, 124, 10, 10, 0, 9, 10, 
	13, 32, 45, 58, 62, 96, 124, 42, 
	43, 48, 57, 10, 0, 9, 10, 13, 
	32, 45, 58, 62, 96, 124, 42, 43, 
	48, 57, 0, 9, 10, 13, 32, 45, 
	58, 62, 96, 124, 42, 43, 48, 57, 
	0, 9, 10, 13, 32, 35, 0, 10, 
	13, 10, 0, 9, 10, 13, 32, 0, 
	9, 10, 13, 32, 35, 0, 9, 10, 
	13, 32, 35, 0, 9, 10, 13, 32, 
	35, 0, 9, 10, 13, 32, 35, 0, 
	9, 10, 13, 32, 0, 9, 10, 13, 
	32, 42, 0, 9, 10, 13, 32, 42, 
	0, 10, 13, 32, 42, 0, 10, 13, 
	42, 0, 10, 13, 32, 42, 10, 0, 
	10, 13, 42, 0, 10, 13, 32, 42, 
	0, 10, 13, 42, 0, 10, 13, 32, 
	42, 10, 0, 10, 13, 42, 0, 9, 
	10, 13, 32, 45, 0, 9, 10, 13, 
	32, 45, 0, 10, 13, 32, 45, 0, 
	10, 13, 45, 0, 10, 13, 32, 45, 
	0, 10, 13, 45, 0, 10, 13, 32, 
	45, 0, 10, 13, 45, 0, 10, 13, 
	32, 45, 0, 10, 13, 45, 0, 9, 
	10, 13, 32, 45, 58, 10, 0, 9, 
	10, 13, 32, 45, 58, 124, 0, 10, 
	13, 32, 45, 0, 10, 13, 32, 45, 
	0, 10, 13, 93, 0, 10, 13, 93, 
	0, 10, 93, 0, 10, 93, 58, 0, 
	10, 13, 32, 0, 10, 13, 10, 0, 
	10, 13, 0, 10, 13, 58, 0, 10, 
	13, 32, 0, 10, 13, 10, 0, 10, 
	13, 0, 10, 60, 0, 10, 60, 33, 
	47, 95, 65, 90, 97, 122, 45, 45, 
	10, 45, 10, 45, 10, 45, 10, 45, 
	62, 0, 10, 60, 95, 65, 90, 97, 
	122, 32, 62, 95, 45, 46, 48, 58, 
	65, 90, 97, 122, 62, 32, 47, 62, 
	95, 45, 58, 65, 90, 97, 122, 32, 
	47, 62, 65, 90, 97, 122, 32, 65, 
	90, 97, 122, 32, 45, 47, 61, 62, 
	95, 48, 58, 65, 90, 97, 122, 62, 
	34, 39, 0, 10, 34, 32, 47, 62, 
	0, 10, 39, 32, 60, 32, 60, 0, 
	10, 13, 0, 9, 10, 13, 32, 96, 
	0, 9, 10, 13, 32, 35, 42, 43, 
	45, 58, 62, 91, 96, 124, 48, 57, 
	45, 61, 0, 10, 13, 32, 45, 61, 
	0, 10, 13, 32, 45, 61, 0
};

static const char _md_single_lengths[] = {
	0, 2, 2, 1, 2, 1, 1, 2, 
	2, 2, 3, 3, 3, 1, 4, 3, 
	1, 6, 1, 3, 2, 3, 3, 3, 
	3, 1, 3, 1, 6, 4, 8, 6, 
	2, 3, 4, 1, 4, 1, 1, 4, 
	4, 3, 10, 5, 5, 3, 1, 5, 
	6, 4, 7, 8, 1, 4, 4, 7, 
	4, 6, 3, 1, 4, 6, 4, 4, 
	6, 3, 1, 3, 6, 6, 6, 6, 
	6, 5, 8, 3, 1, 8, 1, 4, 
	4, 7, 8, 1, 4, 7, 6, 1, 
	1, 10, 1, 10, 10, 6, 3, 1, 
	5, 6, 6, 6, 6, 5, 6, 6, 
	5, 4, 5, 1, 4, 5, 4, 5, 
	1, 4, 6, 6, 5, 4, 5, 4, 
	5, 4, 5, 4, 7, 1, 8, 5, 
	5, 4, 4, 3, 3, 1, 4, 3, 
	1, 3, 4, 4, 3, 1, 3, 3, 
	3, 3, 1, 1, 2, 2, 2, 3, 
	3, 1, 3, 1, 4, 3, 1, 6, 
	1, 2, 3, 3, 3, 2, 2, 3, 
	0, 6, 0, 0, 14, 0, 2, 4, 
	0, 2, 6, 0
};

static const char _md_range_lengths[] = {
	0, 0, 0, 0, 2, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 3, 2, 
	2, 3, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 2, 2, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 2, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 2, 0, 2, 2, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 2, 0, 0, 0, 0, 0, 0, 
	0, 2, 4, 0, 3, 2, 2, 3, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 1, 0, 0, 0, 
	0, 0, 0, 0
};

static const short _md_index_offsets[] = {
	0, 0, 3, 6, 8, 13, 15, 17, 
	20, 23, 26, 30, 34, 38, 40, 48, 
	54, 58, 68, 70, 74, 77, 81, 85, 
	89, 93, 95, 99, 101, 108, 113, 124, 
	133, 136, 140, 145, 147, 152, 154, 156, 
	161, 166, 170, 183, 189, 195, 199, 201, 
	207, 214, 219, 227, 236, 238, 243, 248, 
	256, 262, 269, 273, 275, 280, 287, 292, 
	297, 304, 308, 310, 314, 321, 328, 335, 
	342, 349, 355, 364, 368, 370, 379, 381, 
	386, 391, 399, 408, 410, 415, 423, 430, 
	432, 434, 447, 449, 462, 475, 482, 486, 
	488, 494, 501, 508, 515, 522, 528, 535, 
	542, 548, 553, 559, 561, 566, 572, 577, 
	583, 585, 590, 597, 604, 610, 615, 621, 
	626, 632, 637, 643, 648, 656, 658, 667, 
	673, 679, 684, 689, 693, 697, 699, 704, 
	708, 710, 714, 719, 724, 728, 730, 734, 
	738, 742, 748, 750, 752, 755, 758, 761, 
	765, 769, 773, 781, 783, 791, 797, 801, 
	811, 813, 816, 820, 824, 828, 831, 834, 
	838, 839, 846, 847, 848, 864, 865, 868, 
	873, 874, 877, 884
};

static const unsigned char _md_trans_targs[] = {
	2, 4, 0, 3, 4, 0, 4, 0, 
	5, 14, 14, 14, 0, 6, 0, 7, 
	0, 8, 9, 8, 8, 9, 8, 8, 
	10, 8, 8, 10, 11, 8, 166, 166, 
	13, 12, 166, 166, 13, 12, 166, 0, 
	15, 18, 19, 14, 14, 14, 14, 0, 
	16, 18, 19, 17, 17, 0, 16, 17, 
	17, 0, 15, 17, 18, 20, 19, 17, 
	17, 17, 17, 0, 19, 0, 166, 166, 
	13, 12, 21, 23, 0, 0, 0, 22, 
	21, 15, 18, 19, 0, 0, 0, 22, 
	23, 168, 168, 25, 24, 168, 0, 170, 
	170, 27, 26, 170, 0, 170, 28, 170, 
	27, 28, 29, 26, 170, 170, 27, 30, 
	26, 170, 170, 27, 30, 171, 171, 32, 
	30, 26, 31, 0, 170, 170, 27, 171, 
	171, 32, 26, 31, 0, 170, 171, 0, 
	173, 174, 37, 33, 172, 172, 35, 34, 
	172, 172, 172, 172, 172, 35, 36, 172, 
	174, 0, 172, 172, 172, 172, 38, 40, 
	172, 172, 172, 38, 41, 172, 172, 172, 
	38, 172, 173, 42, 174, 37, 42, 48, 
	54, 57, 62, 74, 43, 56, 33, 173, 
	44, 174, 37, 44, 33, 173, 47, 174, 
	37, 47, 45, 173, 174, 46, 45, 174, 
	0, 173, 47, 174, 46, 47, 45, 173, 
	44, 174, 37, 44, 49, 33, 173, 174, 
	37, 50, 33, 173, 51, 174, 52, 51, 
	50, 55, 33, 173, 51, 174, 52, 51, 
	53, 54, 55, 33, 174, 0, 173, 174, 
	37, 49, 33, 173, 174, 37, 53, 33, 
	173, 51, 174, 52, 51, 53, 54, 33, 
	173, 174, 37, 43, 56, 33, 173, 60, 
	174, 37, 60, 61, 58, 173, 174, 59, 
	58, 174, 0, 173, 174, 59, 61, 58, 
	173, 60, 174, 59, 60, 61, 58, 173, 
	174, 37, 63, 33, 173, 174, 37, 64, 
	33, 176, 67, 177, 66, 67, 68, 65, 
	176, 177, 66, 65, 177, 0, 176, 177, 
	66, 65, 176, 67, 177, 66, 67, 69, 
	65, 176, 67, 177, 66, 67, 70, 65, 
	176, 67, 177, 66, 67, 71, 65, 176, 
	67, 177, 66, 67, 72, 65, 176, 67, 
	177, 66, 67, 73, 65, 176, 67, 177, 
	66, 67, 65, 173, 77, 174, 87, 77, 
	79, 84, 86, 75, 173, 174, 76, 75, 
	174, 0, 173, 77, 174, 78, 77, 79, 
	84, 86, 75, 174, 0, 173, 174, 76, 
	80, 75, 173, 174, 76, 81, 75, 173, 
	82, 174, 83, 82, 81, 85, 75, 173, 
	82, 174, 83, 82, 79, 84, 85, 75, 
	174, 0, 173, 174, 76, 79, 75, 173, 
	82, 174, 83, 82, 79, 84, 75, 173, 
	86, 174, 78, 86, 86, 75, 174, 0, 
	175, 0, 175, 42, 178, 90, 91, 48, 
	54, 57, 62, 74, 43, 56, 33, 178, 
	0, 175, 42, 178, 90, 92, 48, 54, 
	57, 62, 74, 43, 56, 33, 175, 42, 
	178, 90, 42, 48, 54, 57, 62, 74, 
	43, 56, 33, 173, 96, 174, 95, 96, 
	97, 94, 173, 174, 95, 94, 174, 0, 
	173, 96, 174, 95, 96, 94, 173, 96, 
	174, 95, 96, 98, 94, 173, 96, 174, 
	95, 96, 99, 94, 173, 96, 174, 95, 
	96, 100, 94, 173, 96, 174, 95, 96, 
	101, 94, 173, 96, 174, 95, 96, 94, 
	173, 44, 174, 37, 103, 109, 33, 173, 
	47, 174, 37, 47, 104, 45, 173, 174, 
	46, 105, 106, 45, 173, 174, 46, 106, 
	45, 173, 174, 107, 108, 106, 45, 174, 
	0, 173, 174, 107, 106, 45, 173, 174, 
	37, 110, 111, 33, 173, 174, 37, 111, 
	33, 173, 174, 112, 113, 111, 33, 174, 
	0, 173, 174, 112, 111, 33, 173, 44, 
	174, 37, 115, 120, 33, 173, 47, 174, 
	37, 47, 116, 45, 173, 174, 46, 117, 
	118, 45, 173, 174, 46, 118, 45, 173, 
	174, 107, 119, 118, 45, 173, 174, 107, 
	118, 45, 173, 174, 37, 121, 124, 33, 
	173, 174, 37, 122, 33, 173, 174, 112, 
	123, 122, 33, 173, 174, 112, 122, 33, 
	173, 51, 174, 125, 126, 124, 55, 33, 
	174, 0, 173, 51, 174, 125, 51, 127, 
	54, 55, 33, 173, 174, 112, 123, 128, 
	33, 173, 174, 112, 123, 124, 33, 173, 
	174, 131, 33, 130, 173, 174, 131, 138, 
	130, 0, 174, 133, 132, 0, 0, 133, 
	132, 134, 0, 0, 0, 0, 137, 135, 
	172, 172, 136, 135, 172, 0, 172, 172, 
	136, 135, 173, 174, 37, 139, 33, 173, 
	174, 37, 142, 140, 173, 174, 141, 140, 
	174, 0, 173, 174, 141, 140, 0, 144, 
	145, 144, 143, 144, 143, 144, 146, 153, 
	156, 156, 156, 0, 147, 0, 148, 0, 
	149, 150, 149, 149, 150, 149, 149, 151, 
	149, 149, 151, 152, 149, 0, 144, 145, 
	144, 154, 154, 154, 0, 155, 179, 154, 
	154, 154, 154, 154, 0, 179, 0, 157, 
	160, 143, 156, 156, 156, 156, 0, 158, 
	160, 143, 159, 159, 0, 158, 159, 159, 
	0, 157, 159, 160, 161, 143, 159, 159, 
	159, 159, 0, 143, 0, 162, 164, 0, 
	0, 0, 163, 162, 157, 160, 143, 0, 
	0, 0, 163, 164, 1, 4, 0, 1, 
	4, 0, 168, 168, 25, 24, 167, 170, 
	28, 170, 27, 28, 29, 26, 169, 169, 
	175, 42, 175, 88, 89, 93, 102, 43, 
	114, 54, 57, 129, 62, 74, 56, 33, 
	172, 34, 36, 172, 172, 172, 38, 39, 
	172, 172, 34, 36, 172, 172, 172, 38, 
	39, 34, 36, 172, 0, 172, 172, 172, 
	172, 172, 172, 172, 167, 169, 169, 172, 
	172, 172, 172, 172, 172, 0
};

static const short _md_trans_actions[] = {
	0, 46, 17, 0, 46, 17, 46, 17, 
	0, 9, 9, 9, 17, 0, 17, 0, 
	17, 55, 49, 11, 21, 15, 0, 21, 
	15, 0, 21, 15, 0, 0, 345, 350, 
	61, 61, 79, 88, 0, 0, 88, 17, 
	64, 64, 173, 0, 0, 0, 0, 17, 
	0, 0, 1, 11, 11, 17, 0, 11, 
	11, 17, 355, 0, 355, 67, 470, 0, 
	0, 0, 0, 17, 3, 17, 161, 165, 
	11, 11, 11, 11, 17, 17, 17, 0, 
	0, 181, 181, 360, 17, 17, 17, 0, 
	0, 19, 21, 0, 0, 21, 0, 19, 
	21, 0, 0, 21, 0, 19, 0, 21, 
	0, 0, 127, 0, 19, 21, 0, 33, 
	0, 19, 21, 0, 33, 229, 233, 130, 
	277, 0, 130, 0, 19, 21, 0, 19, 
	21, 0, 0, 0, 0, 21, 21, 0, 
	293, 435, 23, 0, 85, 94, 0, 0, 
	41, 94, 41, 85, 94, 0, 0, 41, 
	341, 0, 91, 39, 82, 91, 0, 0, 
	39, 82, 91, 0, 0, 39, 82, 91, 
	0, 39, 405, 151, 482, 109, 148, 145, 
	11, 124, 124, 11, 145, 145, 11, 293, 
	31, 435, 23, 31, 0, 293, 25, 435, 
	23, 25, 25, 455, 494, 118, 0, 321, 
	0, 455, 25, 494, 118, 25, 25, 293, 
	31, 435, 23, 31, 0, 0, 293, 435, 
	23, 0, 0, 289, 0, 430, 23, 0, 
	0, 0, 0, 289, 0, 430, 23, 0, 
	0, 0, 0, 0, 333, 0, 293, 435, 
	23, 0, 0, 293, 435, 23, 0, 0, 
	289, 0, 430, 23, 0, 0, 0, 0, 
	293, 435, 23, 0, 0, 0, 293, 133, 
	435, 23, 133, 133, 133, 445, 536, 142, 
	0, 305, 0, 445, 536, 142, 133, 133, 
	445, 133, 536, 142, 133, 133, 133, 293, 
	435, 23, 0, 0, 293, 435, 23, 0, 
	0, 395, 237, 584, 400, 237, 237, 237, 
	19, 420, 23, 0, 309, 0, 253, 569, 
	257, 145, 395, 237, 584, 400, 237, 237, 
	237, 395, 237, 584, 400, 237, 237, 237, 
	395, 237, 584, 400, 237, 237, 237, 395, 
	237, 584, 400, 237, 237, 237, 395, 237, 
	584, 400, 237, 237, 237, 395, 237, 584, 
	400, 237, 237, 285, 25, 425, 23, 25, 
	25, 25, 25, 25, 460, 500, 118, 0, 
	337, 0, 410, 25, 512, 118, 25, 25, 
	25, 25, 25, 329, 0, 460, 500, 118, 
	0, 0, 460, 500, 118, 0, 0, 415, 
	0, 518, 118, 0, 0, 0, 0, 415, 
	0, 518, 118, 0, 0, 0, 0, 0, 
	333, 0, 460, 500, 118, 0, 0, 415, 
	0, 518, 118, 0, 0, 0, 0, 410, 
	0, 512, 118, 0, 0, 0, 329, 0, 
	154, 0, 281, 151, 476, 109, 148, 145, 
	11, 124, 124, 11, 145, 145, 11, 301, 
	0, 281, 151, 476, 109, 148, 145, 11, 
	124, 124, 11, 145, 145, 11, 281, 151, 
	476, 109, 148, 145, 11, 124, 124, 11, 
	145, 145, 11, 555, 133, 576, 390, 133, 
	133, 133, 450, 542, 142, 0, 313, 0, 
	530, 25, 562, 249, 25, 25, 555, 133, 
	576, 390, 133, 133, 133, 555, 133, 576, 
	390, 133, 133, 133, 555, 133, 576, 390, 
	133, 133, 133, 555, 133, 576, 390, 133, 
	133, 133, 555, 133, 576, 390, 133, 133, 
	293, 31, 435, 23, 31, 0, 0, 293, 
	25, 435, 23, 25, 25, 25, 455, 494, 
	118, 0, 0, 0, 455, 494, 118, 0, 
	0, 524, 548, 221, 0, 0, 0, 317, 
	0, 524, 548, 221, 0, 0, 293, 435, 
	23, 0, 0, 0, 293, 435, 23, 0, 
	0, 440, 488, 112, 0, 0, 0, 317, 
	0, 440, 488, 112, 0, 0, 293, 31, 
	435, 23, 31, 0, 0, 293, 25, 435, 
	23, 25, 25, 25, 455, 494, 118, 0, 
	0, 0, 455, 494, 118, 0, 0, 524, 
	548, 221, 0, 0, 0, 524, 548, 221, 
	0, 0, 293, 435, 23, 0, 0, 0, 
	293, 435, 23, 0, 0, 440, 488, 112, 
	0, 0, 0, 440, 488, 112, 0, 0, 
	440, 0, 488, 112, 0, 0, 0, 0, 
	317, 0, 440, 0, 488, 112, 0, 0, 
	0, 0, 0, 440, 488, 112, 0, 0, 
	0, 440, 488, 112, 0, 0, 0, 293, 
	435, 115, 0, 25, 293, 435, 23, 27, 
	0, 0, 341, 27, 0, 0, 0, 27, 
	0, 0, 0, 0, 0, 0, 136, 136, 
	261, 265, 31, 0, 97, 0, 261, 265, 
	31, 29, 293, 435, 23, 0, 0, 293, 
	435, 23, 136, 136, 465, 506, 121, 0, 
	325, 0, 465, 506, 121, 29, 17, 55, 
	7, 11, 185, 21, 185, 0, 0, 0, 
	9, 9, 9, 17, 0, 17, 0, 17, 
	55, 49, 11, 21, 15, 0, 21, 15, 
	0, 21, 15, 0, 0, 17, 169, 58, 
	61, 9, 9, 9, 17, 13, 177, 0, 
	0, 0, 0, 0, 17, 43, 17, 64, 
	64, 173, 0, 0, 0, 0, 17, 0, 
	0, 1, 11, 11, 17, 0, 11, 11, 
	17, 355, 0, 355, 67, 470, 0, 0, 
	0, 0, 17, 3, 17, 11, 11, 17, 
	17, 17, 0, 0, 181, 181, 360, 17, 
	17, 17, 0, 0, 5, 157, 17, 70, 
	365, 17, 52, 55, 11, 11, 73, 241, 
	139, 245, 139, 139, 385, 139, 193, 189, 
	281, 273, 297, 11, 269, 225, 380, 217, 
	380, 106, 225, 213, 225, 106, 217, 106, 
	76, 103, 103, 76, 370, 375, 100, 100, 
	205, 201, 197, 197, 201, 370, 375, 100, 
	100, 209, 209, 205, 17, 41, 41, 41, 
	39, 39, 39, 39, 73, 193, 189, 76, 
	76, 205, 201, 201, 205, 0
};

static const short _md_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 35, 0, 0, 0, 0, 
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
	0, 0, 0, 0, 0, 0, 0, 35, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 35, 0, 35, 
	0, 35, 0, 0, 35, 0, 0, 0, 
	0, 0, 0, 0
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
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 37, 
	0, 37, 0, 0, 37, 0, 0, 0, 
	0, 0, 0, 0
};

static const short _md_eof_actions[] = {
	0, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
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
	0, 0, 0, 0, 0, 0, 0, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 0, 15, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0
};

static const short _md_eof_trans[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 888, 888, 888, 0, 892, 892, 
	892, 892, 0, 0, 0, 0, 0, 0, 
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
	893, 0, 894, 895, 0, 897, 897, 901, 
	900, 900, 901, 0
};

static const int md_start = 165;
static const int md_first_final = 165;
static const int md_error = 0;

static const int md_en_error = 167;
static const int md_en_fenced_code_scanner = 169;
static const int md_en_markdown = 172;
static const int md_en_inner = 143;
static const int md_en_main = 165;


#line 595 "xs6/xsedit/markdown/kprMarkdownParser.rl"


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
	SInt32 act, cs, i, m, n, stack[STACKSIZE], top;
	
	FskErr err = kFskErrNone;
	KprMarkdownRunRecord runRecord;
	KprMarkdownRun run = &runRecord;
	KprMarkdownStateRecord stateRecord;
	KprMarkdownState state = &stateRecord;
	doResetState(parser, str, state, 0x7);
	doSetStateFlags(parser, str, state, flags);
	state->pc = p; // markup error on the first line
	
	
#line 1381 "xs6/xsedit/markdown/kprMarkdownParser.c"
	{
	cs = md_start;
	top = 0;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 1390 "xs6/xsedit/markdown/kprMarkdownParser.c"
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
#line 1412 "xs6/xsedit/markdown/kprMarkdownParser.c"
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
#line 881 "xs6/xsedit/markdown/kprMarkdownParser.rl"
 (i == n) && (_spaces(ts, m1, -1, 0) == m)  ) _widec += 256;
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
#line 688 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttribute(parser, str, state));
		}
	break;
	case 1:
#line 692 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttributeName(parser, str, state));
		}
	break;
	case 2:
#line 696 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttributeValue(parser, str, state));
		}
	break;
	case 3:
#line 700 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			KprMarkdownElementInfo info = getElementInfoByName(state->elementStart, state->elementStop);
			if (info && ((info->type == kprMarkdownBR) || (info->type == kprMarkdownSPAN)) && (state->depth == 1)) {
				p = state->pc; // backtrack (block level <br> and <span> to markdown)
				bailIfError(doProcessMarkup(parser, str, state, NULL, -1));
				doResetState(parser, str, state, 0x0);
				state->holdPC = NULL;
				state->markPC = NULL;
				p--; {cs = 172; goto _again;}
			}
			else if (info && (info->state == 0)) { // empty element
				bailIfError(doExitElement(parser, str, state));
			}
			else {
				{stack[top++] = cs; cs = 143; goto _again;}
			}
		}
	break;
	case 4:
#line 718 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doEnterElement(parser, str, state));
		}
	break;
	case 5:
#line 722 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			KprMarkdownElementInfo info = getElementInfoByName(state->elementStart, state->elementStop);
			if (info && ((info->type == kprMarkdownBR) || (info->type == kprMarkdownSPAN)) && (state->depth == 1)) {
				p = state->pc; // backtrack (block level <br> and <span> to markdown)
				bailIfError(doProcessMarkup(parser, str, state, NULL, -1));
				doResetState(parser, str, state, 0x0);
				state->holdPC = NULL;
				state->markPC = NULL;
				p--; {cs = 172; goto _again;}
			}
			else {
				bailIfError(doExitElement(parser, str, state));
			}
		}
	break;
	case 6:
#line 737 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doHold(parser, str, state, p, state->line));
		}
	break;
	case 7:
#line 741 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doMark(parser, str, state, p));
		}
	break;
	case 8:
#line 745 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doProcessText(parser, str, state, p, state->line));
		}
	break;
	case 9:
#line 749 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartComment(parser, str, state, p));
		}
	break;
	case 10:
#line 753 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartElement(parser, str, state, p));
		}
	break;
	case 11:
#line 757 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartText(parser, str, state, p));
		}
	break;
	case 12:
#line 761 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopComment(parser, str, state, p));
		}
	break;
	case 13:
#line 765 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopElement(parser, str, state, p));
		}
	break;
	case 14:
#line 769 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopText(parser, str, state, p));
		}
	break;
	case 15:
#line 773 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopText(parser, str, state, p - 1));
		}
	break;
	case 16:
#line 777 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
					p--; {cs = 165; goto _again;}
				}
				else {
					if (state->line > state->holdLine) {
						SInt32 shaper = (state->depth == -1) ? 0 : 1;
						bailIfError(doProcessMarkup(parser, str, state, state->pc, shaper));
					}
					state->holdLine = state->line;
					state->holdPC = state->pc;
					state->markPC = NULL;
					p--; {cs = 167; goto _again;}
				}
			}
			else {
				if (state->holdPC && !state->markPC) {
					p = state->holdPC; // backtrack
				}
				state->holdPC = NULL;
				state->markPC = NULL;
				p--; {cs = 172; goto _again;}
			}
		}
	break;
	case 17:
#line 811 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			SInt32 shaper = (state->depth == -1) ? 0 : 1;
			bailIfError(doProcessMarkup(parser, str, state, p + 1, shaper));
			doResetState(parser, str, state, 0x3);
			state->holdPC = NULL;
			state->markPC = NULL;
		}
	break;
	case 18:
#line 819 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ /*state->line++; */state->pc = NULL; }
	break;
	case 19:
#line 821 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ state->line++; state->pc = p + 1; }
	break;
	case 20:
#line 849 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ b = p; }
	break;
	case 21:
#line 850 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ h = p; }
	break;
	case 22:
#line 852 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ h1 = p; }
	break;
	case 23:
#line 853 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ h2 = p; }
	break;
	case 24:
#line 854 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ m1 = p; }
	break;
	case 25:
#line 855 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ m2 = p; }
	break;
	case 26:
#line 856 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ t1 = p; }
	break;
	case 27:
#line 857 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ t2 = p; }
	break;
	case 28:
#line 858 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ v1 = p; }
	break;
	case 29:
#line 859 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ v2 = p; }
	break;
	case 30:
#line 861 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ t2 = p - 1; }
	break;
	case 31:
#line 862 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ v2 = p - 1; }
	break;
	case 32:
#line 864 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ state->indent++; state->spaces++; }
	break;
	case 33:
#line 865 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ state->indent++; state->spaces += (4 - (state->spaces % 4)); }
	break;
	case 34:
#line 881 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ i = 1; }
	break;
	case 35:
#line 881 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ i++; }
	break;
	case 36:
#line 1283 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ p--; }
	break;
	case 37:
#line 1287 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ {cs = stack[--top]; goto _again;} }
	break;
	case 40:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 41:
#line 837 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				bailIfError(doProcessMarkup(parser, str, state, p + 1, -1)); // error
				doResetState(parser, str, state, 0x3);
				state->holdPC = NULL;
				state->markPC = NULL;
				{cs = 165; goto _again;}
			}}
	break;
	case 42:
#line 881 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
				{cs = 165; goto _again;} // no fret here
			}}
	break;
	case 43:
#line 901 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p;p--;{
				int indent = _indent(t1, t2, m, 1);
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
#line 951 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 4;}
	break;
	case 45:
#line 989 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 5;}
	break;
	case 46:
#line 1013 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 6;}
	break;
	case 47:
#line 1038 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 7;}
	break;
	case 48:
#line 1106 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 9;}
	break;
	case 49:
#line 1126 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 10;}
	break;
	case 50:
#line 1150 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 11;}
	break;
	case 51:
#line 1171 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 12;}
	break;
	case 52:
#line 1193 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 13;}
	break;
	case 53:
#line 1215 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 14;}
	break;
	case 54:
#line 1240 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{act = 15;}
	break;
	case 55:
#line 951 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
				{cs = 165; goto _again;}
			}}
	break;
	case 56:
#line 1065 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
					{cs = 165; goto _again;}
				}
			}}
	break;
	case 57:
#line 1150 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{te = p+1;{
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
				{cs = 165; goto _again;}
			}}
	break;
	case 58:
#line 951 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
				{cs = 165; goto _again;}
			}}
	break;
	case 59:
#line 1013 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
					m = state->spaces;
					n = (SInt32)(m2 - m1);
					{cs = 169; goto _again;} // no fcall here
				}
			}}
	break;
	case 60:
#line 951 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
				{cs = 165; goto _again;}
			}}
	break;
	case 61:
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
				{cs = 165; goto _again;}
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
				{cs = 165; goto _again;}
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
					m = state->spaces;
					n = (SInt32)(m2 - m1);
					{cs = 169; goto _again;} // no fcall here
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
					{cs = 165; goto _again;}
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
				{cs = 165; goto _again;}
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
				{cs = 165; goto _again;}
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
				{cs = 165; goto _again;}
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
				{cs = 165; goto _again;}
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
				{cs = 165; goto _again;}
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
				{cs = 165; goto _again;}
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
				{cs = 165; goto _again;}
			}
	break;
	}
	}
	break;
#line 2369 "xs6/xsedit/markdown/kprMarkdownParser.c"
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
#line 2382 "xs6/xsedit/markdown/kprMarkdownParser.c"
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
#line 773 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopText(parser, str, state, p - 1));
		}
	break;
	case 16:
#line 777 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
					p--; {cs = 165; goto _again;}
				}
				else {
					if (state->line > state->holdLine) {
						SInt32 shaper = (state->depth == -1) ? 0 : 1;
						bailIfError(doProcessMarkup(parser, str, state, state->pc, shaper));
					}
					state->holdLine = state->line;
					state->holdPC = state->pc;
					state->markPC = NULL;
					p--; {cs = 167; goto _again;}
				}
			}
			else {
				if (state->holdPC && !state->markPC) {
					p = state->holdPC; // backtrack
				}
				state->holdPC = NULL;
				state->markPC = NULL;
				p--; {cs = 172; goto _again;}
			}
		}
	break;
#line 2443 "xs6/xsedit/markdown/kprMarkdownParser.c"
		}
	}
	}

	_out: {}
	}

#line 1293 "xs6/xsedit/markdown/kprMarkdownParser.rl"

	
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


#line 2541 "xs6/xsedit/markdown/kprMarkdownParser.c"
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


#line 1384 "xs6/xsedit/markdown/kprMarkdownParser.rl"


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
	
	
#line 2836 "xs6/xsedit/markdown/kprMarkdownParser.c"
	{
	cs = md_inline_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 2844 "xs6/xsedit/markdown/kprMarkdownParser.c"
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
#line 2863 "xs6/xsedit/markdown/kprMarkdownParser.c"
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
#line 1412 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttribute(parser, str, state));
		}
	break;
	case 1:
#line 1416 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttributeName(parser, str, state));
		}
	break;
	case 2:
#line 1420 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doAttributeValue(parser, str, state));
		}
	break;
	case 3:
#line 1424 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			// nop
		}
	break;
	case 4:
#line 1428 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doEnterElement(parser, str, state));
		}
	break;
	case 5:
#line 1432 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doExitElement(parser, str, state));
		}
	break;
	case 6:
#line 1436 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doHold(parser, str, state, p, state->line));
		}
	break;
	case 7:
#line 1440 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doMark(parser, str, state, p));
		}
	break;
	case 8:
#line 1444 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doProcessTextSpecial(parser, str, state, p, state->line));
		}
	break;
	case 9:
#line 1448 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartComment(parser, str, state, p));
		}
	break;
	case 10:
#line 1452 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartElement(parser, str, state, p));
		}
	break;
	case 11:
#line 1456 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStartText(parser, str, state, p));
		}
	break;
	case 12:
#line 1460 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopComment(parser, str, state, p));
		}
	break;
	case 13:
#line 1464 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopElement(parser, str, state, p));
		}
	break;
	case 14:
#line 1468 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopText(parser, str, state, p));
		}
	break;
	case 15:
#line 1472 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			bailIfError(doStopText(parser, str, state, p - 1));
		}
	break;
	case 16:
#line 1476 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{
			// nop
		}
	break;
	case 17:
#line 1495 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ t1 = p; }
	break;
	case 18:
#line 1496 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ t2 = p; }
	break;
	case 19:
#line 1497 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ v1 = p; }
	break;
	case 20:
#line 1498 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ v2 = p; }
	break;
	case 21:
#line 1500 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1526 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = codepoints[1] = 0; }
	break;
	case 23:
#line 1528 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x00022; }
	break;
	case 24:
#line 1529 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x00026; }
	break;
	case 25:
#line 1530 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x00027; }
	break;
	case 26:
#line 1531 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x0003C; }
	break;
	case 27:
#line 1532 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x0003E; }
	break;
	case 28:
#line 1533 "xs6/xsedit/markdown/kprMarkdownParser.rl"
	{ codepoints[0] = 0x000A0; }
	break;
	case 31:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 32:
#line 1567 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1676 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1687 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1701 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1715 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1729 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1743 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1554 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1580 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1593 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1743 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1756 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1776 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1593 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1743 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 1776 "xs6/xsedit/markdown/kprMarkdownParser.rl"
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
#line 3502 "xs6/xsedit/markdown/kprMarkdownParser.c"
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
#line 3515 "xs6/xsedit/markdown/kprMarkdownParser.c"
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

#line 1801 "xs6/xsedit/markdown/kprMarkdownParser.rl"

	
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
