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
#include "kprCode.h"
#include "kprMarkdownParser.h"

// color 0: text (black)
// color 1: line marker / line markup (blue) *
// color 2: span marker / span markup (red)
// color 3: code / code span / text span (green) **
//  * note: lines with no span items are color 1 (blue)
// ** note: spans with no text items are color 3 (green)

FskErr KprCodeMeasureMarkdownPushRun(KprCode self, KprMarkdownParser parser, KprCodeRun run)
{
	FskErr err = kFskErrNone;
	char *mark, *start, *stop;
	mark = start = self->string + parser->codeRunByte;
	stop = start + run->count;
	while (start < stop) {
		parser->codeRunByte++;
		parser->columnIndex++;
		if (*start == '\t') {
			KprCodeRunRecord runRecord;
			SInt32 columnIndex = parser->columnIndex - 1;
			parser->columnIndex = columnIndex + parser->tab;
			parser->columnIndex -= parser->columnIndex % parser->tab;
			if (mark < start) {
				run->count = start - mark;
				bailIfError(FskGrowableArrayAppendItem(self->runs, (void *)run));
			}
			mark = start + 1;
			runRecord.kind = kprCodeTabKind;
			runRecord.count = 1;
			runRecord.color = parser->columnIndex - columnIndex;
			bailIfError(FskGrowableArrayAppendItem(self->runs, (void *)&runRecord));
		}
		start++;
	}
	if (mark < start) {
		run->count = start - mark;
		bailIfError(FskGrowableArrayAppendItem(self->runs, (void *)run));
	}
bail:
	return err;
}

FskErr KprCodeMeasureMarkdownInline(KprCode self, KprMarkdownParser parser, KprMarkdownRun inlineRun)
{
	FskErr err = kFskErrNone;
	KprCodeRunRecord runRecord;
	KprMarkdownRun spanRun;
	SInt32 spanCount = inlineRun->count, spanIndex;
	for (spanIndex = 0; spanIndex < spanCount; spanIndex++) {
		FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, inlineRun->index + spanIndex, (void **)&spanRun);
		if (spanRun->colors & kprMarkdownColorMixFlag) {
			SInt32 c1, c2, c3, c4, c5, cc;
			if (spanRun->u.s.v.offset && (spanRun->u.s.v.offset < spanRun->u.s.t.offset)) {
				c1 = spanRun->u.s.v.offset;
				c2 = spanRun->u.s.v.length;
				c3 = spanRun->u.s.t.offset - (c2 + c1);
				c4 = spanRun->u.s.t.length;
				c5 = spanRun->length - (c4 + c3 + c2 + c1);
				cc = spanRun->count ? 4 : 0;
			}
			else {
				c1 = spanRun->u.s.t.offset;
				c2 = spanRun->u.s.t.length;
				if (spanRun->u.s.v.offset) {
					c3 = spanRun->u.s.v.offset - (c2 + c1);
					c4 = spanRun->u.s.v.length;
					c5 = spanRun->length - (c4 + c3 + c2 + c1);
				}
				else {
					c3 = spanRun->length - (c2 + c1);
					c4 = 0;
					c5 = 0;
				}
				cc = spanRun->count ? 2 : 0;
			}
			if (c1) {
				runRecord.count = c1;
				runRecord.color = spanRun->colors & kprMarkdownColorMask;
				bailIfError(KprCodeMeasureMarkdownPushRun(self, parser, &runRecord));
			}
			if (c2) {
				if (cc == 2) {
					KprCodeMeasureMarkdownInline(self, parser, spanRun);
				}
				else {
					runRecord.count = c2;
					runRecord.color = (spanRun->colors >> kprMarkdownColorMixShift) & kprMarkdownColorMask;
					bailIfError(KprCodeMeasureMarkdownPushRun(self, parser, &runRecord));
				}
			}
			if (c3) {
				runRecord.count = c3;
				runRecord.color = spanRun->colors & kprMarkdownColorMask;
				bailIfError(KprCodeMeasureMarkdownPushRun(self, parser, &runRecord));
			}
			if (c4) {
				if (cc == 4) {
					KprCodeMeasureMarkdownInline(self, parser, spanRun);
				}
				else {
					runRecord.count = c4;
					runRecord.color = (spanRun->colors >> kprMarkdownColorMixShift) & kprMarkdownColorMask;
					bailIfError(KprCodeMeasureMarkdownPushRun(self, parser, &runRecord));
				}
			}
			if (c5) {
				runRecord.count = c5;
				runRecord.color = spanRun->colors & kprMarkdownColorMask;
				bailIfError(KprCodeMeasureMarkdownPushRun(self, parser, &runRecord));
			}
		}
		else {
			if (spanRun->count) {
				KprCodeMeasureMarkdownInline(self, parser, spanRun);
			}
			else {
				runRecord.count = spanRun->length;
				runRecord.color = spanRun->colors & kprMarkdownColorMask;
				bailIfError(KprCodeMeasureMarkdownPushRun(self, parser, &runRecord));
			}
		}
	}
bail:
	return err;
}

FskErr KprCodeMeasureMarkdown(KprCode self) 
{
	FskErr err = kFskErrNone;
	KprCodeRunRecord runRecord;
	KprMarkdownParser parser = NULL;
	FskGrowableArrayDispose(self->runs);
	self->runs = NULL;
	bailIfError(FskGrowableArrayNew(sizeof(KprCodeRunRecord), sizeof(KprCodeRunRecord) * 1024, &(self->runs)));
	xsBeginHost(self->the);
	{
		xsIndex parseMarkdownID = xsID("parseMarkdown");
		if (xsHas(xsGet(xsGlobal, xsID_KPR), parseMarkdownID)) {
			xsResult = xsCall2(xsGet(xsGlobal, xsID_KPR), parseMarkdownID, xsString(self->string), xsInteger(-1));
			if (xsTest(xsResult) && (parser = (KprMarkdownParser)xsGetHostData(xsResult))) {
				KprMarkdownRun lineRun;
				SInt32 lineCount = parser->lineCount, lineIndex;
				parser->codeRunByte = 0;
				parser->columnCount = 0;
				for (lineIndex = 0; lineIndex < lineCount; lineIndex++) {
					FskGrowableArrayGetPointerToItem((FskGrowableArray)parser->runs, lineIndex, (void **)&lineRun);
					runRecord.kind = kprCodeColorKind;
					parser->columnIndex = 0;
					if (lineRun->count) {
						if (lineRun->u.s.t.offset) {
							runRecord.count = lineRun->u.s.t.offset;
							runRecord.color = lineRun->colors & kprMarkdownColorMask;
							bailIfError(KprCodeMeasureMarkdownPushRun(self, parser, &runRecord));
						}
						KprCodeMeasureMarkdownInline(self, parser, lineRun);
					}
					else if (lineRun->length > 1) {
						runRecord.count = lineRun->length - 1;
						runRecord.color = lineRun->colors & kprMarkdownColorMask;
						bailIfError(KprCodeMeasureMarkdownPushRun(self, parser, &runRecord));
					}
					runRecord.kind = kprCodeLineKind;
					runRecord.count = 1; // [\n\0]
					runRecord.color = 0;
					bailIfError(KprCodeMeasureMarkdownPushRun(self, parser, &runRecord));
					if (parser->columnCount < parser->columnIndex)
						parser->columnCount = parser->columnIndex;
				}
			}
			self->columnCount = parser->columnCount;
			self->lineCount = parser->lineCount;
		}
	}
	xsEndHost();
bail:
	return err;
}

// END OF FILE
