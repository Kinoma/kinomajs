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
	// special case: treat unmatched inner </br> as empty element
	if (state->depth > 0) {
		KprMarkdownElementInfo info = getElementInfoByName(state->elementStart, state->elementStop);
		if (info && (info->type == kprMarkdownBR)) {
			KprMarkdownElement element = state->stack[state->top];
			if (element && (element->type != kprMarkdownBR)) {
				doEnterElement(parser, str, state);
				state->flags |= kprMarkdownDoNotReturnToTargetState;
			}
		}
	}
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

%%{
	machine md;
	write data;
}%%

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
	
	%%{
		action attribute {
			bailIfError(doAttribute(parser, str, state));
		}
		
		action attribute_name {
			bailIfError(doAttributeName(parser, str, state));
		}
		
		action attribute_value {
			bailIfError(doAttributeValue(parser, str, state));
		}
		
		action element {
			KprMarkdownElementInfo info = getElementInfoByName(state->elementStart, state->elementStop);
			if (info && ((info->type == kprMarkdownBR) || (info->type == kprMarkdownSPAN)) && (state->depth == 1)) {
				p = state->pc; // backtrack (block level <br> and <span> to markdown)
				bailIfError(doProcessMarkup(parser, str, state, NULL, -1));
				doResetState(parser, str, state, 0x0);
				state->holdPC = NULL;
				state->markPC = NULL;
				fhold; fgoto markdown;
			}
			else if (info && (info->state == 0)) { // empty element
				bailIfError(doExitElement(parser, str, state));
			}
			else {
				fcall inner;
			}
		}
		
		action enter_element {
			bailIfError(doEnterElement(parser, str, state));
		}
		
		action exit_element {
			KprMarkdownElementInfo info = getElementInfoByName(state->elementStart, state->elementStop);
			if (info && ((info->type == kprMarkdownBR) || (info->type == kprMarkdownSPAN)) && (state->depth == 1)) {
				p = state->pc; // backtrack (block level <br> and <span> to markdown)
				bailIfError(doProcessMarkup(parser, str, state, NULL, -1));
				doResetState(parser, str, state, 0x0);
				state->holdPC = NULL;
				state->markPC = NULL;
				fhold; fgoto markdown;
			}
			else {
				bailIfError(doExitElement(parser, str, state));
			}
		}
		
		action handle_target_state {
			// special case: treat unmatched inner </br> as empty element
			if (state->flags & kprMarkdownDoNotReturnToTargetState) {
				state->flags &= ~kprMarkdownDoNotReturnToTargetState;
				fgoto inner;
			}
			else {
				fret;
			}
		}
		
		action hold {
			bailIfError(doHold(parser, str, state, fpc, state->line));
		}
		
		action mark {
			bailIfError(doMark(parser, str, state, fpc));
		}
		
		action process_text {
			bailIfError(doProcessText(parser, str, state, fpc, state->line));
		}
		
		action start_comment {
			bailIfError(doStartComment(parser, str, state, fpc));
		}
		
		action start_element {
			bailIfError(doStartElement(parser, str, state, fpc));
		}
		
		action start_text {
			bailIfError(doStartText(parser, str, state, fpc));
		}
		
		action stop_comment {
			bailIfError(doStopComment(parser, str, state, fpc));
		}
		
		action stop_element {
			bailIfError(doStopElement(parser, str, state, fpc));
		}
		
		action stop_text {
			bailIfError(doStopText(parser, str, state, fpc));
		}
		
		action stop_text_back {
			bailIfError(doStopText(parser, str, state, fpc - 1));
		}
		
		action try_markdown {
			if (state->holdPC && state->markPC) {
				char c = *(fpc - 1);
				if ((c == '\n') || (c == '\0')) { // special case (c character caused the markup parse error)
					bailIfError(doProcessMarkup(parser, str, state, fpc + 0, -1)); // error
					doResetState(parser, str, state, 0x3);
					state->holdPC = NULL;
					state->markPC = NULL;
					if ((c == '\n') && (fc != '\0')) {
						state->line++;
					}
					fhold; fgoto main;
				}
				else {
					if (state->line > state->holdLine) {
						SInt32 shaper = (state->depth == -1) ? 0 : 1;
						bailIfError(doProcessMarkup(parser, str, state, state->pc, shaper));
					}
					state->holdLine = state->line;
					state->holdPC = state->pc;
					state->markPC = NULL;
					fhold; fgoto error;
				}
			}
			else {
				if (state->holdPC && !state->markPC) {
					p = state->holdPC; // backtrack
				}
				state->holdPC = NULL;
				state->markPC = NULL;
				fhold; fgoto markdown;
			}
		}
		
		action use_markup {
			SInt32 shaper = (state->depth == -1) ? 0 : 1;
			bailIfError(doProcessMarkup(parser, str, state, fpc + 1, shaper));
			doResetState(parser, str, state, 0x3);
			state->holdPC = NULL;
			state->markPC = NULL;
		}
		
		end = ( '\0' @{ /*state->line++; */state->pc = NULL; } ); # KPRMARKDOWNDEBUGTEXT wants line++, otherwise line++ not wanted here... why the difference?
		
		newline = ( '\n' @{ state->line++; state->pc = fpc + 1; } );
		
		any_count_line = ( any | newline );
		
		end_count_line = ( end | ( '\r'? newline ) );
		
		maybe_text = ( [^\r\n\0]* );
		
		some_text = ( [^\r\n\0]+ );
		
		base_line = ( ( maybe_text end_count_line ) >start_text %stop_text_back );
		
		# error
		
		error := |*
		
			base_line => {
				bailIfError(doProcessMarkup(parser, str, state, fpc + 1, -1)); // error
				doResetState(parser, str, state, 0x3);
				state->holdPC = NULL;
				state->markPC = NULL;
				fgoto main;
			};
			
		*|;
		
		# markdown
		
		action B { b = fpc; }
		action H { h = fpc; }
		
		action H1 { h1 = fpc; }
		action H2 { h2 = fpc; }
		action M1 { m1 = fpc; }
		action M2 { m2 = fpc; }
		action T1 { t1 = fpc; }
		action T2 { t2 = fpc; }
		action V1 { v1 = fpc; }
		action V2 { v2 = fpc; }
		
		action T2_back { t2 = fpc - 1; }
		action V2_back { v2 = fpc - 1; }
		
		action space { state->indent++; state->spaces++; }
		action tab { state->indent++; state->spaces += (4 - (state->spaces % 4)); }
		
		annex = ( ( [ ] @space | [\t] @tab )* );
		
		annex_nudge = ( ( [ ] @space ){0,3} ); # max slop factor
		
		annex_span = ( ( [ ] | [\t] )* ); # just span, no measurement here
		
		blank_line = ( [ ]{0,3} end_count_line %B ( [ ]{0,3} end_count_line )? );
		
		blockquote_line = ( annex ( [>] [\t ]? )+ >M1 %M2 some_text >T1 %T2 end_count_line );
		
		fenced_code_begin_line = ( annex [`]{3,9} >M1 %M2 [\t ]? base_line >V1 %V2_back );
		
		fenced_code_scanner := |*
		
			( annex_span ( [`] @{ i = 1; } [`]+ @{ i++; } ) >M1 %M2 base_line when { (i == n) && (_spaces(ts, m1, -1, 0) == m) } ) => { // semantic condition
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
				fgoto main; // no fret here
			};
			
			( base_line >T1 %T2_back ) => {
				int indent = _indent(t1, t2, m, 1);
				int length = t2 - t1 - indent;
				int offset = indent; //@@ t1 - str
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
			};
			
		*|;
		
		header_line = ( [#]{1,6} >M1 %M2 [\t ]* maybe_text >T1 %T2 end_count_line ); # atx-style
		
		header_line_2 = ( some_text >H1 %H2 ( '\r'? newline ) ( [=]+ | [\-]+ ) >H end_count_line ); # setext-style *
		
		horizontal_rule_line = ( ( ( [*] [ ]? ){3,} | ( [\-] [ ]? ){3,} ) >M1 %M2 end_count_line );
		
		list_line = ( annex ( [0-9]+ [.] | [*\-+] ) >V1 %V2 [\t ]+ some_text >T1 %T2 end_count_line );
		
		paragraph_line = ( annex base_line ); # * runs concurrently with other line patterns
		
		reference_definition_line = ( ( '[' [^\]\n\0]+ >T1 %T2 ']:' ) >M1 %M2 [ ]? some_text >V1 %V2 end_count_line );
		
		table_line_1 = ( annex [|] ( [\t ]* [|] )* [\t ]* end_count_line );
		
		table_line_2 = ( annex [|]? ( [\t ]* [:]? [\-]{3,} [:]? ( [\t ]+ [|] )* )+ [\t ]* end_count_line );
		
		table_line_n = ( annex [|] [\t ]* some_text >T1 %T2 end_count_line );
		
		markdown := |*
		
			blank_line => {
				#if KPRMARKDOWNDEBUGMARKDOWN
					state->textStart = NULL;
					doProcessText(parser, str, state, fpc, state->line - 1);
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
				fgoto main;
			};
			
			blockquote_line => {
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
				fgoto main;
			};
			
			fenced_code_begin_line => {
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
				if (fc) {
					m = state->spaces;
					n = (SInt32)(m2 - m1);
					fgoto fenced_code_scanner; // no fcall here
				}
			};
			
			header_line => {
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
					fbreak;
				}
				else {
					doResetState(parser, str, state, 0x3);
					fgoto main;
				}
			};
			
			header_line_2 => {
				// caution: runs concurrently with other line patterns
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
					fbreak;
				}
				else {
					doResetState(parser, str, state, 0x3);
					//state->line++;
					fgoto main;
				}
			};
			
			horizontal_rule_line => {
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
				fgoto main;
			};
			
			list_line => {
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
				fgoto main;
			};
			
			reference_definition_line => {
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
				fgoto main;
			};
			
			table_line_1 => {
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
				fgoto main;
			};
			
			table_line_2 => {
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
				fgoto main;
			};
			
			table_line_n => {
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
				fgoto main;
			};
			
			# last
			
			paragraph_line => {
				// caution: runs concurrently with other line patterns
				// note: changes to table_line with table_column_divider
				#if KPRMARKDOWNDEBUGMARKDOWN
					doProcessText(parser, str, state, fpc, state->line - 1);
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
				fgoto main;
			};
			
		*|;
		
		# markup
		
		attribute_name = ( alpha ( alnum | '-' | '_' | ':' )* ); # minimal name support
		attribute_value = ( '"' [^"\n\0]* '"' | '\'' [^'\n\0]* '\'' ); # minimal value support (no escape)
		attribute = ( attribute_name >start_text %stop_text %attribute_name ( '=' attribute_value >start_text %stop_text )? %attribute_value ) %attribute;
		
		comment = ( ( '<!--' ( any_count_line* ) >start_text %stop_text_back :>> '-->' ) >start_comment %stop_comment );
		
		element_name_char = ( alnum | '-' | '_' | '.' | ':' );
		element_name_start_char = ( alpha | '_' );
		element_name = ( element_name_start_char element_name_char* );
		element = ( '<' element_name >start_element %stop_element %enter_element ( [ ]+ attribute )* [ ]? ( '/>' @exit_element | '>' @element ) );
		
		inner_text = ( ( [^<\0] | newline )+ ) >start_text %stop_text %process_text ( '<' | '\0' ) @{ fhold; };
		
		content = ( comment | element | inner_text );
		
		inner := ( content* '</' element_name >start_element %stop_element [ ]? '>' @exit_element @handle_target_state ) $err(try_markdown);
		
		main := ( ( [ ]{0,3} ( comment | element ) >mark ) >hold base_line @use_markup )* $err(try_markdown); # slop factor
		
		write init;
		write exec;
	}%%
	
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

%%{
	machine md_inline;
	write data;
}%%

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
	
	%%{
		action attribute {
			bailIfError(doAttribute(parser, str, state));
		}
		
		action attribute_name {
			bailIfError(doAttributeName(parser, str, state));
		}
		
		action attribute_value {
			bailIfError(doAttributeValue(parser, str, state));
		}
		
		action element {
			// nop
		}
		
		action enter_element {
			bailIfError(doEnterElement(parser, str, state));
		}
		
		action exit_element {
			bailIfError(doExitElement(parser, str, state));
		}
		
		action hold {
			bailIfError(doHold(parser, str, state, fpc, state->line));
		}
		
		action mark {
			bailIfError(doMark(parser, str, state, fpc));
		}
		
		action process_text {
			bailIfError(doProcessTextSpecial(parser, str, state, fpc, state->line));
		}
		
		action start_comment {
			bailIfError(doStartComment(parser, str, state, fpc));
		}
		
		action start_element {
			bailIfError(doStartElement(parser, str, state, fpc));
		}
		
		action start_text {
			bailIfError(doStartText(parser, str, state, fpc));
		}
		
		action stop_comment {
			bailIfError(doStopComment(parser, str, state, fpc));
		}
		
		action stop_element {
			bailIfError(doStopElement(parser, str, state, fpc));
		}
		
		action stop_text {
			bailIfError(doStopText(parser, str, state, fpc));
		}
		
		action stop_text_back {
			bailIfError(doStopText(parser, str, state, fpc - 1));
		}
		
		action use_markup {
			// nop
		}
		
		attribute_name = ( alpha ( alnum | '-' | '_' | ':' )* ); # minimal name support
		attribute_value = ( '"' [^"\n\0]* '"' | '\'' [^'\n\0]* '\'' ); # minimal value support (no escape)
		attribute = ( attribute_name >start_text %stop_text %attribute_name ( '=' attribute_value >start_text %stop_text )? %attribute_value ) %attribute;
		
		comment = ( ( '<!--' [^\n\0]* >start_text %stop_text_back :>> '-->' ) >start_comment %stop_comment );
		
		element_name_char = ( alnum | '-' | '_' | '.' | ':' );
		element_name_start_char = ( alpha | '_' );
		element_name = ( element_name_start_char element_name_char* );
		element = ( '<' element_name >start_element %stop_element %enter_element ( [ ]+ attribute )* [ ]?
					( '/>' @exit_element | '>' @element ( ( [<] | [^<]* ) >start_text %stop_text %process_text
					  '</' element_name >start_element %stop_element [ ]? '>' @exit_element )? ) );
		
		# markdown
		
		action T1 { t1 = fpc; }
		action T2 { t2 = fpc; }
		action V1 { v1 = fpc; }
		action V2 { v2 = fpc; }
		
		action codepoint {
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
		
		backslash_escape = ( [\\] ( [\!\"\#\$\%\&\'\(\)\*\+\,\-\.\/\:\;\<\=\>\?\@\[\\\]\^\_\`\{\|\}\~] ) >T1 %T2 ); # ispunct() punctuation characters
		
		code_span = ( '`' [^`]+ >T1 %T2 '`' ); # `` not supported
		
		double_dash = ( '--' >T1 %T2 ); # EM DASH "\u2014" // http://kinoma.com/develop/documentation/doc-style-sheet/
		
		font_style = ( ( ( [*]{2} [_] | [_] [*]{2} ) | [*]{1,3} | [_]{1,2} | [~]{2} ) >V1 %V2 ); # emphasis | strikethrough | strong
		
		html_entity = ( '&' @{ codepoints[0] = codepoints[1] = 0; } ( # // https://www.w3.org/TR/html5/entities.json
							  ( '#' ( digit+ | [Xx] xdigit+ ) >T1 %T2 %codepoint )
							| (( 'quot'i ) %{ codepoints[0] = 0x00022; } )
							| (( 'amp'i  ) %{ codepoints[0] = 0x00026; } )
							| (( 'apos'  ) %{ codepoints[0] = 0x00027; } ) # lowercase
							| (( 'lt'i   ) %{ codepoints[0] = 0x0003C; } )
							| (( 'gt'i   ) %{ codepoints[0] = 0x0003E; } )
							| (( 'nbsp'  ) %{ codepoints[0] = 0x000A0; } ) # lowercase
							| ( alnum+ )) ';' );
		
		link_text = ( '[' ( [^\]]* ) >T1 %T2 ']' );
		
		link_ref_span = ( link_text [\t ]? '[' [^\]]* >V1 %V2 ']' );
		
		link_span = ( link_text '(' [^\)]* >V1 %V2 ')' );
		
		image_ref_span = ( '!' link_ref_span );
		
		image_span = ( '!' link_span );
		
		markup_span = ( ( ( comment | element ) >mark ) >hold @use_markup );
		
		table_column_divider = ( [|] [\t ]* ) >V1 %V2;
		
		text_span = ( ( [^\\\`\-\*\~\_\&\<\|\[\!]+ ) | any ); # line/inline leading pattern characters, no escape characters here
		
		main := |*
		
			backslash_escape => {
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
			};
			
			code_span => {
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
			};
			
			double_dash => {
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
			};
			
			font_style => {
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
			};
			
			html_entity => { // union special case: u.codepoints
				FskMemSet(run, 0, sizeof(runRecord));
				runRecord.type = kprMarkdownHTMLEntity;
				runRecord.colors = kprMarkdownSpanMarkerColor;
				runRecord.length = te - ts;
				runRecord.offset = ts - str;
				runRecord.u.codepoints[0] = codepoints[0];
				runRecord.u.codepoints[1] = codepoints[1];
				bailIfError(FskGrowableArrayAppendItem((FskGrowableArray)parser->runs, (void *)run));
			};
			
			image_ref_span => {
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
			};
			
			image_span => {
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
			};
			
			link_ref_span => {
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
			};
			
			link_span => {
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
			};
			
			markup_span => {
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
			};
			
			table_column_divider => {
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
			};
			
			# last
			
			text_span => {
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
			};
			
		*|;
		
		write init;
		write exec;
	}%%
	
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
