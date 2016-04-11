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
#include "kpr.h"
#include "kprCode.h"
#include "kprContent.h"
#include "kprLabel.h"
#include "kprText.h"

#define KPRMARKDOWNFINDBLOCK 1000

typedef struct {
	void* pcre;
	int capture;
	int* offsets;
	int depth;
	int index;
} KprTextFindParamRecord, *KprTextFindParam;

static void KPR_text_findInCode(xsMachine *the, KprTextFindParam param, KprCode self);
static void KPR_text_findInContent(xsMachine *the, KprTextFindParam param, KprContent self);
static void KPR_text_findInLabel(xsMachine *the, KprTextFindParam param, KprLabel self);
static void KPR_text_findInString(xsMachine *the, KprTextFindParam param, char* string, int offset, int size);
static void KPR_text_findInText(xsMachine *the, KprTextFindParam param, KprText self);

void KPR_text_find(xsMachine *the)
{
	KprText self = kprGetHostData(xsThis, this, text);
	UInt32 argc = xsToInteger(xsArgc);
	xsStringValue pattern = NULL;
	xsBooleanValue caseless = 0;
	xsVars(4);    
	if (argc > 0)
		pattern = xsToString(xsArg(0));
	if (argc > 1)
		caseless = xsToBoolean(xsArg(1));
	if (pattern && pattern[0]) {
		KprTextFindParamRecord paramRecord;
		int options = PCRE_UTF8; 
		char* message;
		int offset;
		if (caseless)
			options += PCRE_CASELESS;
		paramRecord.pcre = pcre_compile(pattern, options, (const char **)(void*)&message, (int*)(void*)&offset, NULL); 
		if (paramRecord.pcre) {
			pcre_fullinfo(paramRecord.pcre, NULL, PCRE_INFO_CAPTURECOUNT, &paramRecord.capture);
			paramRecord.capture++;
			pcre_fullinfo(paramRecord.pcre, NULL, PCRE_INFO_BACKREFMAX, &offset);
			if (paramRecord.capture < offset)
				paramRecord.capture = offset;
			paramRecord.capture *= 3;
			if (kFskErrNone == FskMemPtrNew(paramRecord.capture * sizeof(int), &paramRecord.offsets)) {
				paramRecord.depth = 1;
				paramRecord.index = 0;
				xsVar(paramRecord.depth) = xsThis;
				KPR_text_findInText(the, &paramRecord, self);
				if (paramRecord.index > 0)
					xsSet(xsResult, xsID_length, xsInteger(paramRecord.index));
				FskMemPtrDispose(paramRecord.offsets);
			}
			pcre_free(paramRecord.pcre);
		}
	}
	if (xsTypeOf(xsResult) == xsUndefinedType)
		xsResult = xsNewInstanceOf(xsArrayPrototype);
}

void KPR_text_findInCode(xsMachine *the, KprTextFindParam param, KprCode self)
{
	KPR_text_findInString(the, param, self->string, 0, FskStrLen(self->string));
}

void KPR_text_findInContent(xsMachine *the, KprTextFindParam param, KprContent self)
{
	if (!FskStrCompare(self->dispatch->type, "code")) {
		if (param->depth < 3) {
			param->depth++;
			xsVar(param->depth) = kprContentGetter(self);
			KPR_text_findInCode(the, param, (KprCode)self);
			param->depth--;
		}
	}
	else if (!FskStrCompare(self->dispatch->type, "label")) {
		if (param->depth < 3) {
			param->depth++;
			xsVar(param->depth) = kprContentGetter(self);
			KPR_text_findInLabel(the, param, (KprLabel)self);
			param->depth--;
		}
	}
	else if (!FskStrCompare(self->dispatch->type, "text")) {
		if (param->depth < 3) {
			param->depth++;
			xsVar(param->depth) = kprContentGetter(self);
			KPR_text_findInText(the, param, (KprText)self);
			param->depth--;
		}
	}
	else if (self->flags & kprContainer) {
		KprContent content = ((KprContainer)self)->first;
		while (content) {
			KPR_text_findInContent(the, param, content);
			content = content->next;
		}
	}
}

void KPR_text_findInLabel(xsMachine *the, KprTextFindParam param, KprLabel self)
{
	KPR_text_findInString(the, param, self->text, 0, FskStrLen(self->text));
}

void KPR_text_findInString(xsMachine *the, KprTextFindParam param, char* string, int offset, int size)
{
	for (;;) {
		int count = pcre_exec(param->pcre, NULL, string, size, offset, PCRE_NOTEMPTY, param->offsets, param->capture);
		if (count <= 0) {
			break;
		}
		offset = param->offsets[0];
		if ((param->index % KPRMARKDOWNFINDBLOCK) == 0) {
			xsVar(0) = xsNew1(xsGlobal, xsID_Array, xsInteger(KPRMARKDOWNFINDBLOCK));
			(void)xsCall1(xsVar(0), xsID_fill, xsUndefined); // dense array
			if (xsTypeOf(xsResult) == xsUndefinedType)
				xsResult = xsVar(0);
			else
				xsResult = xsCall1(xsResult, xsID_concat, xsVar(0));
		}
		xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
		xsSet(xsVar(0), xsID_content, xsVar(param->depth));
		xsSet(xsVar(0), xsID_offset, xsInteger(fxUTF8ToUnicodeOffset(string, offset)));
		xsSet(xsVar(0), xsID_length, xsInteger(fxUTF8ToUnicodeOffset(string + offset, param->offsets[1] - offset)));
		xsSet(xsResult, param->index++, xsVar(0));
		offset = param->offsets[1];
	}
}

void KPR_text_findInText(xsMachine *the, KprTextFindParam param, KprText self)
{
	char* string;
	KprTextBlock block;
	KprTextRun run;
	int from, to, c, i;
	FskGrowableStorageGetPointerToItem(self->textBuffer, 0, (void **)&string);
	FskGrowableArrayGetPointerToItem(self->blockBuffer, 0, (void **)&block);
	from = 0;
	while (block->count >= 0) {
		c = block->count;
		run = (KprTextRun)(block + 1);
		for (i = 0; i < c; i++) {
			if (run->span.length < 0) {
				to = run->span.offset;
				if (to > from)
					KPR_text_findInString(the, param, string, from, to);
				KPR_text_findInContent(the, param, run->item.content);
				from = (run + 1)->span.offset;
			}
			run++;
		}
		block += 1 + c;
	}
	to = block->offset;
	if (to > from)
		KPR_text_findInString(the, param, string, from, to);
}

