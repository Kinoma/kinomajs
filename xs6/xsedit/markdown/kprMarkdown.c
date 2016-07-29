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
#include "FskExtensions.h"

FskExport(FskErr) kprMarkdown_fskLoad(FskLibrary library)
{
	return kFskErrNone;
}

FskExport(FskErr) kprMarkdown_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

#include "kprBehavior.h"
#include "kprContent.h"
#include "kprImage.h"
#include "kprLabel.h"
#include "kprMedia.h"
#include "kprSkin.h"
#include "kprTable.h"
#include "kprText.h"
#include "kprURL.h"

#include "kprMarkdown.h"
#include "kprMarkdownParser.h"

#define KPRMARKDOWNEXPERIMENTALCODE 0
#define KPRMARKDOWNALLHARDLINEBREAKS 0
#define KPRMARKDOWNIFRAMEMEDIASUPPORT 0

typedef enum {
	argString = 0,
	argOptions = 1,
} KprMarkdownArg;

typedef enum {
	varBuffer = 0,
	varCodeType,
	varH0, // meta-data
	varH0Items,
	varH1,
	varH1Items,
	varH2,
	varH2Items,
	varH3,
	varH3Items,
	varH4, // no items
	varURL,
	varCount,
} KprMarkdownVar;

static void _KprMarkdownBuffer(xsMachine* the, SInt32 offset, SInt32 length)
{
	xsStringValue buffer, string;
	xsVar(varBuffer) = xsStringBuffer(NULL, length);
	string = xsToString(xsArg(argString));
	buffer = xsToString(xsVar(varBuffer));
	FskMemCopy(buffer, string + offset, length);
	buffer[length] = 0;
}

static void _KprMarkdownListItem(xsMachine* the, KprMarkdownParser parser, KprMarkdownRun lineRun, SInt32 lineNumber, SInt32 item)
{
	SInt32 items = item - 1, list = item - 2;
	if (item > varH0) {
		if (!xsTest(xsVar(list)) && (list >= varH0))
			_KprMarkdownListItem(the, parser, NULL, 0, list);
		else {
			if (item < varH1) xsVar(varH1) = xsUndefined;
			if (item < varH2) xsVar(varH2) = xsUndefined;
			if (item < varH3) xsVar(varH3) = xsUndefined;
			if (item < varH4) xsVar(varH4) = xsUndefined;
		}
		if (xsHas(xsVar(list), xsID_items))
			xsVar(items) = xsGet(xsVar(list), xsID_items);
		else {
			xsVar(items) = xsNewInstanceOf(xsArrayPrototype);
			xsSet(xsVar(list), xsID_items, xsVar(items));
		}
		xsVar(item) = xsNewInstanceOf(xsObjectPrototype);
		xsCall1(xsVar(items), xsID_push, xsVar(item));
	}
	else
		xsVar(item) = xsNewInstanceOf(xsObjectPrototype);
	if (lineRun) {
		if (lineRun->count) {
			KprMarkdownRun spanRun;
			SInt32 spanCount = lineRun->count, spanIndex;
			xsResult = xsString("");
			for (spanIndex = 0; spanIndex < spanCount; spanIndex++) {
				FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, lineRun->index + spanIndex, (void **)&spanRun);
				if ((spanRun->type == kprMarkdownLinkReferenceSpan) && (parser->referenceDefinitionCount == 0)) {
					_KprMarkdownBuffer(the, spanRun->offset, spanRun->length);
					xsResult = xsCall1(xsResult, xsID_concat, xsVar(varBuffer));
				}
				else if (spanRun->u.s.t.length > 0) {
					_KprMarkdownBuffer(the, spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length);
					xsResult = xsCall1(xsResult, xsID_concat, xsVar(varBuffer));
				}
			}
			xsVar(varBuffer) = xsResult;
		}
		else
			_KprMarkdownBuffer(the, lineRun->offset + lineRun->u.s.t.offset, lineRun->u.s.t.length);
		xsResult = xsCall1(xsString("@"), xsID_concat, xsInteger(lineNumber));
		xsSet(xsVar(item), xsID("anchor"), xsResult);
		xsSet(xsVar(item), xsID("title"), xsVar(varBuffer));
	}
	else
		xsResult = xsUndefined;
}

void KprMarkdownParserDestructor(void* data)
{
	(void)KprMarkdownParserDispose((KprMarkdownParser)data);
}

void KPR_parseMarkdown(xsMachine* the)
{
	xsIntegerValue argc = xsToInteger(xsArgc);
	xsStringValue string = xsToString(xsArg(argString));
	KprMarkdownParser parser = NULL;
	KprMarkdownRun lineRun = NULL;
	SInt32 c, i, lineNumber;
	SInt32 mode = 0;
	xsVars(varCount);
	xsTry {
		xsThrowIfFskErr(KprMarkdownParserNew(&parser, kprMarkdownParserDefaultOption, kprMarkdownParserDefaultTab));
		if (argc > 1)
			mode = xsToInteger(xsArg(1));
		if (mode > 1)
			xsDebugger();
		else if (mode == 0) {
			xsThrowIfFskErr(KprMarkdownParse(parser, string, 0, FskStrLen(string) + 1, kprMarkdownParseAll));
			_KprMarkdownListItem(the, parser, NULL, 0, varH0);
			c = parser->lineCount;
			for (i = 0, lineNumber = 1; i < c; i++, lineNumber++) {
				FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, i, (void **)&lineRun);
				switch (lineRun->type) {
				case kprMarkdownHeaderLine: {
					switch (lineRun->shaper) {
					case 1:
						_KprMarkdownListItem(the, parser, lineRun, lineNumber, varH1);
						if (!xsHas(xsVar(varH0), xsID("title")))
							xsSet(xsVar(varH0), xsID("title"), xsVar(varBuffer));
						break;
					case 2:
						_KprMarkdownListItem(the, parser, lineRun, lineNumber, varH2);
						break;
					case 3:
						_KprMarkdownListItem(the, parser, lineRun, lineNumber, varH3);
						break;
					default:
						break;
					}
					break;
				}
				case kprMarkdownMarkupLine: {
					switch (lineRun->shaper) {
					case 0: {
						SInt32 length = lineRun->length - 1;
						if (length && (string[lineRun->offset] != '<') && (!xsHas(xsVar(varH0), xsID("intro")))) {
							if (((length - 3) >= 0) && (FskStrCompareWithLength(string + lineRun->offset + length - 3, "-->", 3) == 0))
								length -= 3; // trim comment text that ends with "-->"
							if (length) {
								_KprMarkdownBuffer(the, lineRun->offset, length);
								xsSet(xsVar(varH0), xsID("intro"), xsVar(varBuffer));
							}
						}
						break;
					}
					default:
						break;
					}
					break;
				}
				default:
					break;
				}
			}
			xsResult = xsVar(varH0); // meta-data
		}
		else if (mode == -1) {
			xsThrowIfFskErr(KprMarkdownParse(parser, string, 0, FskStrLen(string) + 1, kprMarkdownParseAll));
			xsResult = xsNewHostObject(KprMarkdownParserDestructor);
			xsSetHostData(xsResult, (void *)parser);
			parser = NULL; // defer disposal
		}
	}
	xsCatch {
		xsResult = xsUndefined;
	}
	KprMarkdownParserDispose(parser);
}

KprMarkdownAttribute KPR_text_markdownAttribute(xsMachine* the, KprMarkdownRun lineRun, KprMarkdownAttribute attributes, char* key)
{
	if (attributes) {
		KprMarkdownAttribute attribute = NULL;
		xsStringValue string = xsToString(xsArg(argString));
		SInt32 c = FskGrowableArrayGetItemCount((FskGrowableArray)attributes), i;
		for (i = 0; i < c; i++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)attributes, i, (void **)&attribute);
			if (FskStrCompareCaseInsensitiveWithLength(string + lineRun->offset + attribute->n.offset, key, attribute->n.length) == 0)
				return attribute;
		}
	}
	return NULL;
}

KprMarkdownElement KPR_text_markdownElement(xsMachine* the, KprMarkdownRun lineRun, KprMarkdownElement elements, SInt32 index)
{
	if (elements) {
		KprMarkdownElement element = NULL;
		if (index < (SInt32)FskGrowableArrayGetItemCount((FskGrowableArray)elements)) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, index, (void **)&element);
			return element;
		}
	}
	return NULL;
}

typedef enum {
	skinHorizontalRule = 0,
	skinHTMLTableData,
	skinHTMLTableHead,
	skinHTMLTableRowFirst,
	skinHTMLTableRowNext,
	skinCount,
} KprMarkdownSkin;

typedef enum {
	styleBlockquoteFirst = 0,
	styleBlockquoteNext,
	styleCodeFirst,
	styleCodeNext,
	styleCodeSpan,
	styleColumn,
	styleHeader1,
	styleHeader2,
	styleHeader3,
	styleHeader4,
	styleHeader5,
	styleHeader6,
	styleHTMLTableData,
	styleHTMLTableHead,
	styleLinkSpan,
	styleListFirst,
	styleListNext,
	styleParagraph,
	styleTextSpan,
	styleX0NormalSpan,
	styleX1ItalicSpan,
	styleX2BoldSpan,
	styleX3BoldItalicSpan,
	styleCount,
	styleHeaderBase = styleHeader1,
	styleEmphasisBase = styleX0NormalSpan,
} KprMarkdownStyle;

typedef struct KprMarkdownOptionsStruct KprMarkdownOptionsRecord, *KprMarkdownOptions;

struct KprMarkdownOptionsStruct {
	KprSkin skins[skinCount];
	KprStyle styles[styleCount];
	SInt32 columnWidth;
};

void KPR_text_markdownOptions(xsMachine* the, xsSlot* slot, KprMarkdownOptions options)
{
	static char *skinName, *skinNames[] = {
		"skinHorizontalRule",
		"skinHTMLTableData",
		"skinHTMLTableHead",
		"skinHTMLTableRowFirst",
		"skinHTMLTableRowNext",
		NULL,
	};
	static char *styleName, *styleNames[] = {
		"styleBlockquoteFirst",
		"styleBlockquoteNext",
		"styleCodeFirst",
		"styleCodeNext",
		"styleCodeSpan",
		"styleColumn",
		"styleHeader1",
		"styleHeader2",
		"styleHeader3",
		"styleHeader4",
		"styleHeader5",
		"styleHeader6",
		"styleHTMLTableData",
		"styleHTMLTableHead",
		"styleLinkSpan",
		"styleListFirst",
		"styleListNext",
		"styleParagraph",
		"styleTextSpan",
		"styleX0NormalSpan",
		"styleX1ItalicSpan",
		"styleX2BoldSpan",
		"styleX3BoldItalicSpan",
		NULL,
	};
	SInt32 i;
	
	for (i = 0, skinName = skinNames[i]; skinName; skinName = skinNames[++i]) {
		if (xsHas(*slot, xsID(skinName))) {
			xsResult = xsGet(*slot, xsID(skinName));
			options->skins[i] = xsTest(xsResult) ? kprGetHostData(xsResult, skin, skin) : NULL;
		}
	}
	for (i = 0, styleName = styleNames[i]; styleName; styleName = styleNames[++i]) {
		if (xsHas(*slot, xsID(styleName))) {
			xsResult = xsGet(*slot, xsID(styleName));
			options->styles[i] = xsTest(xsResult) ? kprGetHostData(xsResult, style, style) : NULL;
		}
	}
	if (xsHas(*slot, xsID("columnWidth")))
		options->columnWidth = xsToInteger(xsGet(*slot, xsID("columnWidth")));
}

#if TARGET_OS_WIN32
static char BULLET[] = "*";
static char EM_DASH[] = "--";
static char EN_DASH[] = "-";
#else
static char BULLET[] = "\u2022";
static char EM_DASH[] = "\u2014";
static char EN_DASH[] = "\u2013";
static char ZERO_WIDTH_SPACE[] = "\u200B";
#endif

void KPR_text_formatMarkdownP(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element, KprColumn column, KprStyle style)
{
	KprMarkdownElement elements = element->elements;
	KprCoordinatesRecord coordinates = { kprLeftRight, kprTop, 0, 0, 0, 0, 0, 0 };
	KprPicture picture = NULL;
	KprText text = NULL;
	KprTextLink link = NULL;
	KprBehavior behavior = NULL;
	
	if (column) {
		xsThrowIfFskErr(KprTextNew(&text, &coordinates, self->skin, style, ""));
		text->flags |= (kprActive | kprTextSelectable);
		text->state = 1;
		KprContainerAdd((KprContainer)column, (KprContent)text);
		KprTextBegin(text);
	}
	else {
		KprTextBeginBlock(self, style, NULL);
		text = self;
	}
	
	if (elements) {
		char tmp;
		KprMarkdownAttribute attribute;
		KprMarkdownElement item;
		KprMarkdownElementType formerElement = kprMarkdownP;
		SInt32 c = FskGrowableArrayGetItemCount((FskGrowableArray)elements), i;
		SInt32 index;
		xsStringValue buffer;
		xsStringValue start, stop;
		xsStringValue string = xsToString(xsArg(argString));
		if (element->type == kprMarkdownLI) {
			SInt32 BULLET_LEN = FskStrLen(BULLET);
			KprTextConcatText(text, BULLET, BULLET_LEN);
			KprTextConcatText(text, " ", 1);
		}
		for (i = 0; i < c; i++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, i, (void **)&item);
			switch (item->type) {
			case kprMarkdownA:
				element = KPR_text_markdownElement(the, lineRun, item->elements, 0);
				if (element && element->t.length > 0) {
					attribute = KPR_text_markdownAttribute(the, lineRun, item->attributes, "href");
					if (attribute && (attribute->v.length > 0)) {
						start = string + lineRun->offset + attribute->v.offset;
						stop = start + attribute->v.length;
						tmp = *stop;
						*stop = 0;
						xsThrowIfFskErr(KprURLMerge(xsToString(xsVar(varURL)), start, &buffer));
						*stop = tmp;
						xsResult = xsNew1(xsArg(argOptions), xsID("LinkBehavior"), xsString(buffer));
						string = xsToString(xsArg(argString));
						FskMemPtrDisposeAt(&buffer);
						
						xsThrowIfFskErr(KprTextLinkNew(&link));
						xsThrowIfFskErr(KprScriptBehaviorNew(&behavior, (KprContent)link, the, &xsResult));
						link->behavior = behavior;
					}
					else
						link = NULL;
					KprTextBeginSpan(text, options->styles[styleLinkSpan], link);
					KprTextConcatText(text, string + lineRun->offset + element->t.offset, element->t.length);
					KprTextEndSpan(text);
				}
				break;
				
			case kprMarkdownBR:
				if (formerElement != kprMarkdownBR)
					KprTextConcatText(text, "\n", 1);
				break;
				
			case kprMarkdownCODE:
				index = 0;
				do { // <code>@<i>attribute</i></code>
					element = KPR_text_markdownElement(the, lineRun, item->elements, index);
					if (element) {
						if (element->t.length > 0) {
							KprTextBeginSpan(text, options->styles[styleCodeSpan], NULL);
							KprTextConcatText(text, string + lineRun->offset + element->t.offset, element->t.length);
							KprTextEndSpan(text);
						}
						else {
							element = KPR_text_markdownElement(the, lineRun, element->elements, 0);
							if (element && element->t.length > 0) {
								KprTextBeginSpan(text, options->styles[styleCodeSpan], NULL);
								KprTextConcatText(text, string + lineRun->offset + element->t.offset, element->t.length);
								KprTextEndSpan(text);
							}
						}
					}
					index++;
				} while (element);
				break;
				
			case kprMarkdownIMG:
				attribute = KPR_text_markdownAttribute(the, lineRun, item->attributes, "src");
				if (attribute && (attribute->v.length > 0)) {
					xsThrowIfFskErr(KprPictureNew(&picture, &coordinates));
					
					start = string + lineRun->offset + attribute->v.offset;
					stop = start + attribute->v.length;
					tmp = *stop;
					*stop = 0;
					xsThrowIfFskErr(KprURLMerge(xsToString(xsVar(varURL)), start, &buffer));
					*stop = tmp;
					KprPictureSetURL(picture, buffer, NULL);
					FskMemPtrDisposeAt(&buffer);
					
					KprTextConcatContent(text, (KprContent)picture, kprTextFloatLeftRight);
				}
				break;
				
			case kprMarkdownEM:
			case kprMarkdownI:
				element = KPR_text_markdownElement(the, lineRun, item->elements, 0);
				if (element && element->t.length > 0) {
					KprTextBeginSpan(text, options->styles[styleX1ItalicSpan], NULL);
					KprTextConcatText(text, string + lineRun->offset + element->t.offset, element->t.length);
					KprTextEndSpan(text);
				}
				break;
				
			case kprMarkdownSTRONG:
			case kprMarkdownB:
				element = KPR_text_markdownElement(the, lineRun, item->elements, 0);
				if (element && element->t.length > 0) {
					KprTextBeginSpan(text, options->styles[styleX2BoldSpan], NULL);
					KprTextConcatText(text, string + lineRun->offset + element->t.offset, element->t.length);
					KprTextEndSpan(text);
				}
				break;
				
			default:
				break;
			}
			if (item->t.length > 0)
				KprTextConcatText(text, string + lineRun->offset + item->t.offset, item->t.length);
			formerElement = item->type;
		}
	}
	
	if (column)
		KprTextEnd(text);
	else
		KprTextEndBlock(self);
}

void KPR_text_formatMarkdownLI(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element, KprColumn column, SInt32 first)
{
	KprStyle style = first ? options->styles[styleListFirst] : options->styles[styleListNext];
	KPR_text_formatMarkdownP(the, self, options, lineRun, element, column, style);
}

void KPR_text_formatMarkdownOL(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element, KprColumn column)
{
	KprMarkdownElement elements = element->elements;
	
	if (elements) {
		KprMarkdownElement item;
		SInt32 c = FskGrowableArrayGetItemCount((FskGrowableArray)elements), i;
		for (i = 0; i < c; i++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, i, (void **)&item);
			switch (item->type) {
			case kprMarkdownLI:
				KPR_text_formatMarkdownLI(the, self, options, lineRun, item, column, i == 0);
				break;
			default:
				break;
			}
		}
	}
}

void KPR_text_formatMarkdownUL(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element, KprColumn column)
{
	KprMarkdownElement elements = element->elements;
	
	if (elements) {
		KprMarkdownElement item;
		SInt32 c = FskGrowableArrayGetItemCount((FskGrowableArray)elements), i;
		for (i = 0; i < c; i++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, i, (void **)&item);
			switch (item->type) {
			case kprMarkdownLI:
				KPR_text_formatMarkdownLI(the, self, options, lineRun, item, column, i == 0);
				break;
			default:
				break;
			}
		}
	}
}

void KPR_text_formatMarkdownTD(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element, KprLine line, SInt32 width)
{
	KprMarkdownElement elements = element->elements, p = NULL;
	KprCoordinatesRecord coordinates = { kprWidth, kprTopBottom, 0, width, 0, 0, 0, 0 };
	KprColumn column = NULL;
	
	xsThrowIfFskErr(KprColumnNew(&column, &coordinates, options->skins[skinHTMLTableData], options->styles[styleHTMLTableData]));
	KprContainerAdd((KprContainer)line, (KprContent)column);
	
	if (elements) {
		KprMarkdownElement item;
		SInt32 c = FskGrowableArrayGetItemCount((FskGrowableArray)elements), d, i;
		for (d = i = 0; i < c; i++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, i, (void **)&item);
			switch (item->type) {
			case kprMarkdownP:
				if (p) {
					KPR_text_formatMarkdownP(the, self, options, lineRun, p, column, NULL);
					FskGrowableArrayDispose((FskGrowableArray)p->elements);
					FskMemPtrDispose(p);
					p = NULL;
				}
				KPR_text_formatMarkdownP(the, self, options, lineRun, item, column, NULL);
				break;
			case kprMarkdownOL:
				if (p) {
					KPR_text_formatMarkdownP(the, self, options, lineRun, p, column, NULL);
					FskGrowableArrayDispose((FskGrowableArray)p->elements);
					FskMemPtrDispose(p);
					p = NULL;
				}
				KPR_text_formatMarkdownOL(the, self, options, lineRun, item, column);
				break;
			case kprMarkdownUL:
				if (p) {
					KPR_text_formatMarkdownP(the, self, options, lineRun, p, column, NULL);
					FskGrowableArrayDispose((FskGrowableArray)p->elements);
					FskMemPtrDispose(p);
					p = NULL;
				}
				KPR_text_formatMarkdownUL(the, self, options, lineRun, item, column);
				break;
			default:
				if (p == NULL) {
					FskMemPtrNewClear(sizeof(KprMarkdownElementRecord), (FskMemPtr *)&p);
					if (p) {
						p->n.length = -1;
						p->n.offset = 0;
						p->t.length = -1;
						p->t.offset = 0;
						p->type = kprMarkdownP;
						FskGrowableArrayNew(sizeof(KprMarkdownElementRecord), kprMarkdownDefaultElementsOption, (FskGrowableArray*)&(p->elements));
					}
				}
				if (p && p->elements) {
					FskGrowableArrayAppendItem((FskGrowableArray)p->elements, (void *)item);
				}
				break;
			}
		}
		if (p) {
			KPR_text_formatMarkdownP(the, self, options, lineRun, p, column, NULL);
			FskGrowableArrayDispose((FskGrowableArray)p->elements);
			FskMemPtrDispose(p);
			p = NULL;
		}
	}
}

void KPR_text_formatMarkdownTH(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element, KprLine line, SInt32 width)
{
	KprMarkdownElement elements = element->elements;
	KprCoordinatesRecord coordinates = { kprWidth, kprTopBottom, 0, width, 0, 0, 0, 0 };
	KprColumn column = NULL;
	
	xsThrowIfFskErr(KprColumnNew(&column, &coordinates, options->skins[skinHTMLTableHead], options->styles[styleHTMLTableHead]));
	KprContainerAdd((KprContainer)line, (KprContent)column);
	
	if (elements) {
		KprMarkdownElement item;
		SInt32 c = FskGrowableArrayGetItemCount((FskGrowableArray)elements), d, i;
		for (d = i = 0; i < c; i++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, i, (void **)&item);
			switch (item->type) {
			case kprMarkdownP:
				KPR_text_formatMarkdownP(the, self, options, lineRun, item, column, NULL);
				break;
			default:
				d++;
				break;
			}
		}
		if (d == c)
			KPR_text_formatMarkdownP(the, self, options, lineRun, element, column, NULL);
	}
}

void KPR_text_formatMarkdownTR(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element, SInt32 first, SInt32 head)
{
	KprMarkdownElement elements = element->elements;
	KprLine line = NULL;
	KprCoordinatesRecord coordinates = { kprLeft, kprMiddle, 0, 0, 0, 0, 0, 0 };
	KprSkin lineSkin = first ? options->skins[skinHTMLTableRowFirst] : options->skins[skinHTMLTableRowNext];
	
	xsThrowIfFskErr(KprLineNew(&line, &coordinates, lineSkin, NULL));
	KprTextBeginBlock(self, NULL, NULL);
	// note: KprTextConcatContent zeroes LRTB coordinates
	KprTextConcatContent(self, (KprContent)line, kprTextFloatLeftRight);
	KprTextEndBlock(self);
	
	if (elements) {
		KprMarkdownElement item;
		KprSkin skin = head ? options->skins[skinHTMLTableHead] : options->skins[skinHTMLTableData];
		SInt32 c = FskGrowableArrayGetItemCount((FskGrowableArray)elements), i;
		SInt32 width = skin ? skin->data.color.borders.right : 0;
		if (width > 0) {
			KprColumn column = NULL;
			KprCoordinatesRecord coordinates = { kprWidth, kprTopBottom, 0, width, 0, 0, 0, 0 };
			xsThrowIfFskErr(KprColumnNew(&column, &coordinates, skin, NULL));
			KprContainerAdd((KprContainer)line, (KprContent)column);
		}
		for (i = 0; i < c; i++) {
			SInt32 width = options->columnWidth;
			if ((c == 3) && (i == 2)) width *= 2;
			else if ((c == 2) && (i == 1)) width *= 3;
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, i, (void **)&item);
			switch (item->type) {
			case kprMarkdownTD:
				KPR_text_formatMarkdownTD(the, self, options, lineRun, item, line, width);
				break;
			case kprMarkdownTH:
				KPR_text_formatMarkdownTH(the, self, options, lineRun, item, line, width);
				break;
			default:
				break;
			}
		}
	}
}

void KPR_text_formatMarkdownTBODY(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element, SInt32* rowIndex)
{
	KprMarkdownElement elements = element->elements;
	
	if (elements) {
		KprMarkdownElement item;
		SInt32 c = FskGrowableArrayGetItemCount((FskGrowableArray)elements), i;
		for (i = 0; i < c; i++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, i, (void **)&item);
			switch (item->type) {
			case kprMarkdownTR:
				KPR_text_formatMarkdownTR(the, self, options, lineRun, item, (*rowIndex)++ == 0, false);
				break;
			default:
				break;
			}
		}
	}
}

void KPR_text_formatMarkdownTHEAD(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element, SInt32* rowIndex)
{
	KprMarkdownElement elements = element->elements;
	
	if (elements) {
		KprMarkdownElement item;
		SInt32 c = FskGrowableArrayGetItemCount((FskGrowableArray)elements), i;
		for (i = 0; i < c; i++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, i, (void **)&item);
			switch (item->type) {
			case kprMarkdownTR:
				KPR_text_formatMarkdownTR(the, self, options, lineRun, item, (*rowIndex)++ == 0, true);
				break;
			default:
				break;
			}
		}
	}
}

#if KPRMARKDOWNIFRAMEMEDIASUPPORT
void KPR_text_formatMarkdownIFRAME(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element)
{
	KprMarkdownAttribute attribute, attributes = element->attributes;
	xsIntegerValue width = 320, height = 240;
	
	attribute = KPR_text_markdownAttribute(the, lineRun, attributes, "width");
	if (attribute && (attribute->v.length > 0)) {
		_KprMarkdownBuffer(the, lineRun->offset + attribute->v.offset, attribute->v.length);
		width = xsToInteger(xsVar(varBuffer));
	}
	attribute = KPR_text_markdownAttribute(the, lineRun, attributes, "height");
	if (attribute && (attribute->v.length > 0)) {
		_KprMarkdownBuffer(the, lineRun->offset + attribute->v.offset, attribute->v.length);
		height = xsToInteger(xsVar(varBuffer));
	}
	attribute = KPR_text_markdownAttribute(the, lineRun, attributes, "src");
	if (attribute && (attribute->v.length > 0)) {
		KprCoordinatesRecord coordinates = { kprWidth, kprHeight, 0, width, 0, 0, height, 0 };
		KprMedia media = NULL;
		_KprMarkdownBuffer(the, lineRun->offset + attribute->v.offset, attribute->v.length);
		xsThrowIfFskErr(KprMediaNew(&media, &coordinates));
		KprMediaSetURL(media, xsToString(xsVar(varBuffer)), NULL);
		KprTextBeginBlock(self, NULL, NULL);
		KprTextConcatContent(self, (KprContent)media, kprTextFloatLeftRight);
		KprTextEndBlock(self);
	}
}
#else
void KPR_text_formatMarkdownIFRAME(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element)
{
	KprMarkdownElement elements = element->elements;
	
	if (elements) {
		KprMarkdownElement item;
		KprStyle style = options->styles[styleParagraph];
		SInt32 c = FskGrowableArrayGetItemCount((FskGrowableArray)elements), d, i;
		for (d = i = 0; i < c; i++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, i, (void **)&item);
			switch (item->type) {
			case kprMarkdownP:
				KPR_text_formatMarkdownP(the, self, options, lineRun, item, NULL, style);
				break;
			default:
				d++;
				break;
			}
		}
		if (d == c)
			KPR_text_formatMarkdownP(the, self, options, lineRun, element, NULL, style);
	}
}
#endif

void KPR_text_formatMarkdownTABLE(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownRun lineRun, KprMarkdownElement element)
{
	KprMarkdownAttribute attribute, attributes = element->attributes;
	KprMarkdownElement elements = element->elements;
	KprMarkdownOptionsRecord optionsRecord;
	xsSlot slot;
	
	optionsRecord = *options;
	attribute = KPR_text_markdownAttribute(the, lineRun, attributes, "class");
	if (attribute && (attribute->v.length > 0)) {
		_KprMarkdownBuffer(the, lineRun->offset + attribute->v.offset, attribute->v.length);
		slot = xsCallFunction1(xsGet(xsArg(argOptions), xsID("TableClassHelper")), xsNull, xsVar(varBuffer));
		if (xsIsInstanceOf(slot, xsObjectPrototype))
			KPR_text_markdownOptions(the, &slot, &optionsRecord);
	}
	if (elements) {
		KprMarkdownElement item;
		SInt32 c = FskGrowableArrayGetItemCount((FskGrowableArray)elements), i;
		SInt32 rowIndex = 0;
		for (i = 0; i < c; i++) {
			FskGrowableArrayGetPointerToItem((FskGrowableArray)elements, i, (void **)&item);
			switch (item->type) {
			case kprMarkdownTBODY:
				KPR_text_formatMarkdownTBODY(the, self, &optionsRecord, lineRun, item, &rowIndex);
				break;
			case kprMarkdownTHEAD:
				KPR_text_formatMarkdownTHEAD(the, self, &optionsRecord, lineRun, item, &rowIndex);
				break;
			case kprMarkdownTR:
				KPR_text_formatMarkdownTR(the, self, &optionsRecord, lineRun, item, rowIndex++ == 0, false);
				break;
			default:
				break;
			}
		}
	}
}

void KPR_text_formatMarkdownInlineHTMLEntity(xsMachine* the, KprText self, UInt32* codepoints, SInt32 length)
{
	int i;
	for (i = 0; i < length; i++) {
		UInt32 d = codepoints[i];
		char str[5] = "....";
		char *dst = str;
		if (d < 0x80) {
			*dst++ = d;
		}
		else if (d < 0x800) {
			*dst++ = 0xC0 | (d >> 6);
			*dst++ = 0x80 | (d & 0x3F);
		}
		else if (d < 0x10000) {
			*dst++ = 0xE0 | (d >> 12);
			*dst++ = 0x80 | ((d >> 6) & 0x3F);
			*dst++ = 0x80 | (d & 0x3F);
		}
		else if (d < 0x200000) {
			*dst++ = 0xF0 | (d >> 18);
			*dst++ = 0x80 | ((d >> 12) & 0x3F);
			*dst++ = 0x80 | ((d >> 6) & 0x3F);
			*dst++ = 0x80 | (d  & 0x3F);
		}
		*dst = '\0';
		KprTextConcatString(self, str);
	}
}

void KPR_text_formatMarkdownInline(xsMachine* the, KprText self, KprMarkdownOptions options, KprMarkdownParser parser, KprMarkdownRun inlineRun)
{
	char tmp;
	KprBehavior behavior = NULL;
	KprCoordinatesRecord coordinates = { 0, 0, 0, 0, 0, 0, 0, 0 };
	KprPicture picture = NULL;
	KprStyle style = NULL;
	KprTextLink link = NULL;
	xsStringValue buffer;
	xsStringValue start, stop;
	xsStringValue string = xsToString(xsArg(argString));
	
	if (inlineRun->count && inlineRun->index) {
		SInt32 EN_DASH_LEN = FskStrLen(EN_DASH);
		SInt32 c = inlineRun->count, i;
		for (i = 0; i < c; i++) {
			KprMarkdownRun spanRun;
			FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, inlineRun->index + i, (void **)&spanRun);
			switch (spanRun->type) {
			case kprMarkdownTextSpan:
				if (style)
					KprTextBeginSpan(self, style, NULL);
				KprTextConcatText(self, string + spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length);
				if (style)
					KprTextEndSpan(self);
				break;
				
			case kprMarkdownCodeSpan:
				KprTextBeginSpan(self, options->styles[styleCodeSpan], NULL);
				KprTextConcatText(self, string + spanRun->offset + spanRun->u.s.t.offset, spanRun->u.s.t.length);
				KprTextEndSpan(self);
				break;
				
			case kprMarkdownDoubleDash:
				KprTextConcatText(self, EN_DASH, EN_DASH_LEN);
				break;
				
			case kprMarkdownFontStyle:
				style = options->styles[styleEmphasisBase + (spanRun->shaper & 3)];
				break;
				
			case kprMarkdownHTMLEntity: // union special case: u.codepoints
				if (spanRun->u.codepoints[0] > 0)
					KPR_text_formatMarkdownInlineHTMLEntity(the, self, spanRun->u.codepoints, (spanRun->u.codepoints[1] > 0) ? 2 : 1);
				else
					KprTextConcatText(self, "?", 1); // self, string + spanRun->offset, spanRun->length
				break;
				
			case kprMarkdownImageSpan:
				xsThrowIfFskErr(KprPictureNew(&picture, &coordinates));
				
				start = string + spanRun->offset + spanRun->u.s.v.offset;
				stop = FskStrNChr(start, spanRun->u.s.v.length, ' '); // drop title
				if (!stop)
					stop = start + spanRun->u.s.v.length;
				tmp = *stop;
				*stop = 0;
				xsThrowIfFskErr(KprURLMerge(xsToString(xsVar(varURL)), start, &buffer));
				*stop = tmp;
				KprPictureSetURL(picture, buffer, NULL);
				FskMemPtrDisposeAt(&buffer);
				
				KprTextConcatContent(self, (KprContent)picture, kprTextFloatLeftRight);
				break;
				
			case kprMarkdownLinkSpan:
				start = string + spanRun->offset + spanRun->u.s.v.offset;
				stop = start + spanRun->u.s.v.length;
				tmp = *stop;
				*stop = 0;
				xsThrowIfFskErr(KprURLMerge(xsToString(xsVar(varURL)), start, &buffer));
				*stop = tmp;
				xsResult = xsNew1(xsArg(argOptions), xsID("LinkBehavior"), xsString(buffer));
				string = xsToString(xsArg(argString));
				FskMemPtrDisposeAt(&buffer);
				
				xsThrowIfFskErr(KprTextLinkNew(&link));
				xsThrowIfFskErr(KprScriptBehaviorNew(&behavior, (KprContent)link, the, &xsResult));
				link->behavior = behavior;
				KprTextBeginSpan(self, options->styles[styleLinkSpan], link);
				KPR_text_formatMarkdownInline(the, self, options, parser, spanRun);
				KprTextEndSpan(self);
				break;
				
			case kprMarkdownMarkupSpan:
				if (spanRun->u.element && (spanRun->shaper == 1)) {
					KprMarkdownAttribute attribute = NULL;
					KprMarkdownElement element = spanRun->u.element;
					switch (element->type) {
					case kprMarkdownBR:
						KprTextBeginSpan(self, NULL, NULL);
						KprTextConcatText(self, "\n", 1);
						KprTextEndSpan(self);
						break;
					case kprMarkdownSPAN:
						attribute = KPR_text_markdownAttribute(the, spanRun, element->attributes, "style");
						if (attribute && (attribute->v.length > 0)) {
							_KprMarkdownBuffer(the, spanRun->offset + attribute->v.offset, attribute->v.length);
							xsResult = xsCallFunction1(xsGet(xsArg(argOptions), xsID("SpanStyleHelper")), xsNull, xsVar(varBuffer));
							if (xsIsInstanceOf(xsResult, xsObjectPrototype))
								style = kprGetHostData(xsResult, style, style);
							string = xsToString(xsArg(argString));
						}
						KprTextBeginSpan(self, style, NULL);
						if (spanRun->u.element && spanRun->u.element->t.length > 0)
							KPR_text_formatMarkdownInline(the, self, options, parser, spanRun);
						KprTextEndSpan(self);
						style = NULL;
						break;
					default:
						if (spanRun->u.element && spanRun->u.element->t.length > 0)
							KPR_text_formatMarkdownInline(the, self, options, parser, spanRun);
						break;
					}
				}
				break;
				
			default:
				KPR_text_formatMarkdownInline(the, self, options, parser, spanRun);
				break;
			}
		}
	}
	else if (inlineRun->type == kprMarkdownMarkupSpan)
		KprTextConcatText(self, string + inlineRun->offset + inlineRun->u.element->t.offset, inlineRun->u.element->t.length);
	else if ((inlineRun->type == kprMarkdownLinkReferenceSpan) && (parser->referenceDefinitionCount == 0))
		KprTextConcatText(self, string + inlineRun->offset, inlineRun->length);
	else if (inlineRun->u.s.t.length > 0)
		KprTextConcatText(self, string + inlineRun->offset + inlineRun->u.s.t.offset, inlineRun->u.s.t.length);
}

void KPR_text_formatMarkdown(xsMachine* the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	xsStringValue string = xsToString(xsArg(argString));
	KprCoordinatesRecord coordinates = { 0, 0, 0, 0, 0, 0, 0, 0 };
	KprContent content = NULL;
	KprMarkdownParser parser = NULL;
	KprMarkdownRun lineRun = NULL;
	KprMarkdownType formerLine = kprMarkdownParagraphLine;
	SInt32 c, i, lineNumber;
	SInt32 index, length, offset;
	xsStringValue buffer = NULL;
	
	KprMarkdownOptionsRecord optionsRecord;
	FskMemSet(&optionsRecord, 0, sizeof(optionsRecord));
	
	xsTry {
		xsVars(varCount);
		_KprMarkdownListItem(the, parser, NULL, 0, varH0);
		KPR_text_markdownOptions(the, &xsArg(argOptions), &optionsRecord);
		xsVar(varURL) = xsGet(xsArg(argOptions), xsID_url);
		
		xsThrowIfFskErr(KprMarkdownParserNew(&parser, kprMarkdownParserDefaultOption, kprMarkdownParserDefaultTab));
		xsThrowIfFskErr(KprMarkdownParse(parser, string, 0, FskStrLen(string) + 1, kprMarkdownParseAll | kprMarkdownParseElements));
		//KprMarkdownPrintDebugInfo(parser, string, 0x1 | 0x2);
		
		KprTextBegin(self);
		{
			SInt32 BULLET_LEN = FskStrLen(BULLET);
			#if !TARGET_OS_WIN32
			SInt32 ZERO_WIDTH_SPACE_LEN = FskStrLen(ZERO_WIDTH_SPACE);
			#endif
			#if KPRMARKDOWNALLHARDLINEBREAKS
				char *s = NULL;
			#endif
			c = parser->lineCount;
			for (i = 0, lineNumber = 1; i < c; i++, lineNumber++) {
				FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, i, (void **)&lineRun);
				switch (lineRun->type) {
				case kprMarkdownBlankLine:
					if (formerLine != kprMarkdownTableLine)
						formerLine = kprMarkdownParagraphLine;
					break;
					
				case kprMarkdownParagraphLine:
					switch (formerLine) { // continuation lines
					case kprMarkdownBlockquoteLine:
						KprTextBeginBlock(self, optionsRecord.styles[styleBlockquoteNext], NULL);
						break;
					default:
						KprTextBeginBlock(self, optionsRecord.styles[styleParagraph], NULL);
						formerLine = kprMarkdownParagraphLine;
						break;
					}
					KPR_text_formatMarkdownInline(the, self, &optionsRecord, parser, lineRun);
					#if !KPRMARKDOWNALLHARDLINEBREAKS
						if (lineRun->shaper > 0) // hard linebreak (for paragraph lines)
							KprTextConcatText(self, "\n", 1);
					#endif
					KprTextEndBlock(self);
					break;
					
				case kprMarkdownMarkupLine: // union special case: u.element
					if (lineRun->u.element && (lineRun->shaper == 1)) {
						KprMarkdownAttribute attribute = NULL;
						KprMarkdownElement element = lineRun->u.element;
						switch (element->type) {
						case kprMarkdownA:
							attribute = KPR_text_markdownAttribute(the, lineRun, element->attributes, "id");
							if (attribute && (attribute->v.length > 0)) {
								_KprMarkdownBuffer(the, lineRun->offset + attribute->v.offset, attribute->v.length);
								xsResult = xsNew1(xsArg(argOptions), xsID("AnchorContent"), xsVar(varBuffer));
								content = kprGetHostData(xsResult, anchor, content);
								string = xsToString(xsArg(argString));
								
								KprTextBeginBlock(self, NULL, NULL);
								KprTextConcatContent(self, content, kprTextFloatLeftRight);
								KprTextEndBlock(self);
							}
							break;
						case kprMarkdownDIV:
							// <div class="ccode indentCode"></div>
							// <div class="ccode"></div>
							// <div class="indentCode"></div>
							// <div class="jscode"></div>
							// <div class="xmlcode indentCode"></div>
							// <div class="xmlcode"></div>
							attribute = KPR_text_markdownAttribute(the, lineRun, element->attributes, "class");
							if (attribute && (attribute->v.length > 0)) {
								if (FskStrCompareWithLength(string + lineRun->offset + attribute->v.offset, "jscode", 6) == 0)
									xsVar(varCodeType) = xsString("javascript");
								else if (FskStrCompareWithLength(string + lineRun->offset + attribute->v.offset, "xmlcode", 7) == 0)
									xsVar(varCodeType) = xsString("xml");
								else
									xsVar(varCodeType) = xsUndefined;
							}
							break;
						case kprMarkdownIFRAME:
							KPR_text_formatMarkdownIFRAME(the, self, &optionsRecord, lineRun, element);
							break;
						case kprMarkdownTABLE:
							KPR_text_formatMarkdownTABLE(the, self, &optionsRecord, lineRun, element);
							break;
						default:
							break;
						}
						formerLine = kprMarkdownMarkupLine;
					}
					// comment should not interrupt line sequence with the exception of empty comment
					// for an indented code block placed immediately after a list item
					// note: the empty comment string must match "<!-- -->";
					//       fenced code blocks don't need the empty comment; and
					//       two consecutive blank lines could break a list, but that's not widely supported
					else if ((lineRun->shaper == 0) && (lineRun->length >= 8)) {
						if (FskStrCompareWithLength(string + lineRun->offset, "<!-- -->", 8) == 0)
							formerLine = kprMarkdownMarkupLine;
					}
					break;
					
				case kprMarkdownHeaderLine:
					if (lineRun->shaper == 1) {
						_KprMarkdownListItem(the, parser, lineRun, lineNumber, varH1);
						if (!xsHas(xsVar(varH0), xsID("title")))
							xsSet(xsVar(varH0), xsID("title"), xsVar(varBuffer));
						xsResult = xsNew1(xsArg(argOptions), xsID("AnchorContent"), xsResult);
						content = kprGetHostData(xsResult, anchor, content);
						string = xsToString(xsArg(argString));
						
						KprTextBeginBlock(self, NULL, NULL);
						KprTextConcatContent(self, content, kprTextFloatLeftRight);
						KprTextEndBlock(self);
					}
					if ((lineRun->shaper == 2) || (lineRun->shaper == 3) || (lineRun->shaper == 4)) {
						_KprMarkdownListItem(the, parser, lineRun, lineNumber, (lineRun->shaper == 2) ? varH2 : (lineRun->shaper == 3) ? varH3 : varH4);
						xsResult = xsNew1(xsArg(argOptions), xsID("AnchorContent"), xsResult);
						content = kprGetHostData(xsResult, anchor, content);
						string = xsToString(xsArg(argString));
						
						KprTextBeginBlock(self, NULL, NULL);
						KprTextConcatContent(self, content, kprTextFloatLeftRight);
						KprTextEndBlock(self);
					}
					KprTextBeginBlock(self, optionsRecord.styles[styleHeaderBase + lineRun->shaper - 1], NULL);
					#if !TARGET_OS_WIN32
					KprTextConcatText(self, ZERO_WIDTH_SPACE, ZERO_WIDTH_SPACE_LEN);
					#endif
					KPR_text_formatMarkdownInline(the, self, &optionsRecord, parser, lineRun);
					#if !TARGET_OS_WIN32
					KprTextConcatText(self, ZERO_WIDTH_SPACE, ZERO_WIDTH_SPACE_LEN);
					#endif
					KprTextEndBlock(self);
					formerLine = kprMarkdownParagraphLine;
					break;
					
				case kprMarkdownHorizontalRuleLine:
					xsThrowIfFskErr(KprContentNew(&content, &coordinates, optionsRecord.skins[skinHorizontalRule], NULL));
					KprTextBeginBlock(self, NULL, NULL);
					KprTextConcatContent(self, content, kprTextFloatLeftRight);
					KprTextEndBlock(self);
					formerLine = kprMarkdownHorizontalRuleLine;
					break;
					
				case kprMarkdownFencedCodeBeginLine:
					if (lineRun->u.s.v.length > 0) {
						_KprMarkdownBuffer(the, lineRun->offset + lineRun->u.s.v.offset, lineRun->u.s.v.length);
						xsVar(varCodeType) = xsVar(varBuffer);
					}
					else
						xsVar(varCodeType) = xsUndefined;
					break;
					
				case kprMarkdownFencedCodeLine:
				case kprMarkdownIndentedCodeLine:
					if (xsHas(xsArg(argOptions), xsID("CodeContent"))) {
						index = i, length = 0, offset = 0;
						for (i = index; i < c; i++) {
							FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, i, (void **)&lineRun);
							if ((lineRun->type == kprMarkdownFencedCodeLine) || (lineRun->type == kprMarkdownIndentedCodeLine)) {
								if (lineRun->u.s.t.length > 0)
									length += lineRun->u.s.t.length + 1;
								else
									length += 1;
							}
							else
								break;
						}
						xsVar(varBuffer) = xsStringBuffer(NULL, length);
						string = xsToString(xsArg(argString));
						buffer = xsToString(xsVar(varBuffer));
						for (i = index; i < c; i++) {
							FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, i, (void **)&lineRun);
							if ((lineRun->type == kprMarkdownFencedCodeLine) || (lineRun->type == kprMarkdownIndentedCodeLine)) {
								if (lineRun->u.s.t.length > 0) {
									FskMemCopy(buffer + offset, string + lineRun->offset + lineRun->u.s.t.offset, lineRun->u.s.t.length + 1);
									offset += lineRun->u.s.t.length + 1;
								}
								else {
									FskMemCopy(buffer + offset, "\n", 1);
									offset += 1;
								}
							}
							else
								break;
						}
						buffer[length] = 0;
						xsResult = xsNewInstanceOf(xsObjectPrototype);
						xsSet(xsResult, xsID_string, xsVar(varBuffer));
						if (xsTypeOf(xsVar(varCodeType)) == xsUndefinedType)
							xsSet(xsResult, xsID_type, xsHas(xsArg(argOptions), xsID("defaultCodeType")) ? xsGet(xsArg(argOptions), xsID("defaultCodeType")) : xsString("javascript"));
						else {
							xsSet(xsResult, xsID_type, xsVar(varCodeType));
							xsVar(varCodeType) = xsUndefined;
						}
						xsResult = xsNew1(xsArg(argOptions), xsID("CodeContent"), xsResult);
						content = kprGetHostData(xsResult, container, content);
						
						KprTextBeginBlock(self, NULL, NULL);
						KprTextConcatContent(self, content, kprTextFloatLeftRight);
						KprTextEndBlock(self);
						lineNumber = i--;
					}
					else {
						if (formerLine != kprMarkdownIndentedCodeLine)
							KprTextBeginBlock(self, optionsRecord.styles[styleCodeFirst], NULL);
						else
							KprTextBeginBlock(self, optionsRecord.styles[styleCodeNext], NULL);
						KPR_text_formatMarkdownInline(the, self, &optionsRecord, parser, lineRun);
						KprTextEndBlock(self);
					}
					formerLine = kprMarkdownIndentedCodeLine;
					break;
					
				case kprMarkdownListLine:
					if (formerLine != kprMarkdownListLine)
						KprTextBeginBlock(self, optionsRecord.styles[styleListFirst], NULL);
					else
						KprTextBeginBlock(self, optionsRecord.styles[styleListNext], NULL);
					if (lineRun->shaper < 0)
						KprTextConcatText(self, string + lineRun->offset + lineRun->u.s.v.offset, lineRun->u.s.v.length);
					else
						KprTextConcatText(self, BULLET, BULLET_LEN);
					KprTextConcatText(self, " ", 1);
					KPR_text_formatMarkdownInline(the, self, &optionsRecord, parser, lineRun);
					KprTextEndBlock(self);
					formerLine = kprMarkdownListLine;
					break;
					
				case kprMarkdownBlockquoteLine:
					if ((formerLine != kprMarkdownBlockquoteLine) && (formerLine != kprMarkdownTableLine))
						KprTextBeginBlock(self, optionsRecord.styles[styleBlockquoteFirst], NULL);
					else
						KprTextBeginBlock(self, optionsRecord.styles[styleBlockquoteNext], NULL);
					KPR_text_formatMarkdownInline(the, self, &optionsRecord, parser, lineRun);
					KprTextEndBlock(self);
					formerLine = kprMarkdownBlockquoteLine;
					break;
					
				case kprMarkdownTableLine:
					if (lineRun->count) {
						KprCoordinatesRecord lineCoordinates = { kprLeft, kprMiddle, 0, 0, 0, 0, 0, 0 };
						KprCoordinatesRecord textCoordinates = { kprWidth, kprMiddle, 0, optionsRecord.columnWidth, 0, 0, 0, 0 };
						SInt32 inlineCount = lineRun->count, inlineIndex, inlineMarker;
						KprLine line;
						KprText text;
						xsThrowIfFskErr(KprLineNew(&line, &lineCoordinates, NULL, optionsRecord.styles[styleColumn]));
						for (inlineIndex = inlineMarker = 0; inlineIndex < inlineCount; inlineIndex++) {
							KprMarkdownRun inlineRun;
							FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, lineRun->index + inlineIndex, (void **)&inlineRun);
							if ((inlineRun->type == kprMarkdownTableColumnDivider) || ((inlineIndex + 1) == inlineCount)) {
								if (inlineRun->type != kprMarkdownTableColumnDivider)
									inlineIndex++;
								if ((inlineIndex - inlineMarker) > 1) { // multiple items
									xsThrowIfFskErr(KprTextNew(&text, &textCoordinates, self->skin, optionsRecord.styles[styleX0NormalSpan], ""));
									text->flags |= (kprActive | kprTextSelectable);
									text->state = 1;
									KprTextBegin(text);
									KprTextBeginBlock(text, optionsRecord.styles[styleX0NormalSpan], NULL);
									for (; inlineMarker < inlineIndex; inlineMarker++) {
										FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, lineRun->index + inlineMarker, (void **)&inlineRun);
										if (inlineRun->type == kprMarkdownTextSpan)
											KprTextBeginSpan(text, optionsRecord.styles[styleX0NormalSpan], NULL);
										else if (inlineRun->type == kprMarkdownCodeSpan)
											KprTextBeginSpan(text, optionsRecord.styles[styleCodeSpan], NULL);
										else
											KprTextBeginSpan(text, NULL, NULL);
										KprTextConcatText(text, string + inlineRun->offset + inlineRun->u.s.t.offset, inlineRun->u.s.t.length);
										KprTextEndSpan(text);
									}
									KprTextEndBlock(text);
									KprTextEnd(text);
									KprContainerAdd((KprContainer)line, (KprContent)text);
								}
								else if ((inlineIndex - inlineMarker) == 1) { // single item
									FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, lineRun->index + inlineMarker, (void **)&inlineRun);
									if (inlineRun->type == kprMarkdownTextSpan) {
										xsThrowIfFskErr(KprTextNew(&text, &textCoordinates, self->skin, optionsRecord.styles[styleX0NormalSpan], ""));
										text->flags |= (kprActive | kprTextSelectable);
										text->state = 1;
										KprTextBegin(text);
										KprTextBeginBlock(text, NULL, NULL);
										KprTextBeginSpan(text, NULL, NULL);
										KprTextConcatText(text, string + inlineRun->offset + inlineRun->u.s.t.offset, inlineRun->u.s.t.length);
										KprTextEndSpan(text);
										KprTextEndBlock(text);
										KprTextEnd(text);
										KprContainerAdd((KprContainer)line, (KprContent)text);
									}
									else if (inlineRun->type == kprMarkdownCodeSpan) {
										xsThrowIfFskErr(KprTextNew(&text, &textCoordinates, self->skin, optionsRecord.styles[styleCodeSpan], ""));
										text->flags |= (kprActive | kprTextSelectable);
										text->state = 1;
										KprTextBegin(text);
										KprTextBeginBlock(text, NULL, NULL);
										KprTextBeginSpan(text, NULL, NULL);
										KprTextConcatText(text, string + inlineRun->offset + inlineRun->u.s.t.offset, inlineRun->u.s.t.length);
										KprTextEndSpan(text);
										KprTextEndBlock(text);
										KprTextEnd(text);
										KprContainerAdd((KprContainer)line, (KprContent)text);
									}
								}
								else {
									xsThrowIfFskErr(KprTextNew(&text, &textCoordinates, self->skin, optionsRecord.styles[styleX0NormalSpan], ""));
									text->flags |= (kprActive | kprTextSelectable);
									text->state = 1;
									KprContainerAdd((KprContainer)line, (KprContent)text);
								}
								inlineMarker = inlineIndex + 1;
							}
						}
						KprTextBeginBlock(self, NULL, NULL);
						KprTextConcatContent(self, (KprContent)line, kprTextFloatLeftRight);
						KprTextEndBlock(self);
					}
					formerLine = kprMarkdownTableLine;
					break;
					
				default:
					break;
				}
				#if KPRMARKDOWNALLHARDLINEBREAKS
					s = string + lineRun->offset + lineRun->length - 1; // hard linebreak (for all lines)
					if ((lineRun->length > 2) && (*(--s) == ' ') && (*(--s) == ' '))
						KprTextConcatText(self, "\n", 1);
				#endif
			}
		}
		KprTextEnd(self);
		
	}
	xsCatch {
		KprTextBegin(self);
		KprTextEnd(self);
	}
	KprMarkdownParserDispose(parser);
	xsResult = xsVar(varH0); // meta-data
}

// END OF FILE
