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

#include "xsAll.h"
#include "xs_pcre.h"

#define mxCheckRegExp(THE_SLOT) \
	if (!mxIsRegExp(THE_SLOT)) \
		mxDebug0(the, XS_TYPE_ERROR, "this is no RegExp")
		
#define mxOffsetCount 30

static void fx_RegExp(txMachine* the);
static void fx_RegExp_exec(txMachine* the);
static void fx_RegExp_test(txMachine* the);
static void fx_RegExp_toString(txMachine* the);

void fxBuildRegExp(txMachine* the)
{
	mxPush(mxGlobal);

	mxPush(mxObjectPrototype);
	fxNewRegExpInstance(the);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "global"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "ignoreCase"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "multiline"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "source"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_RegExp_exec, 0);
	fxQueueID(the, fxID(the, "exec"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_RegExp_test, 0);
	fxQueueID(the, fxID(the, "test"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_RegExp_toString, 0);
	fxQueueID(the, fxID(the, "toString"), XS_DONT_ENUM_FLAG);
	
	fxAliasInstance(the, the->stack);
	mxRegExpPrototype = *the->stack;
	fxNewHostConstructor(the, fx_RegExp, 2);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxRegExpPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "RegExp"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	the->stack++;
	
#if mxRegExp
	pcre_setup();
#endif
}

txInteger fxExecuteRegExp(txMachine* the, txSlot* theRegExp, txSlot* theString, txInteger theOffset)
{
#if mxRegExp
	pcre* aCode = theRegExp->next->value.regexp.code;
	txInteger* offsets = theRegExp->next->value.regexp.offsets;
	txString aString = theString->value.string;
	txInteger aCount;
	txInteger anOffset;
	pcre_fullinfo(aCode, NULL, PCRE_INFO_CAPTURECOUNT, &aCount);
	aCount++;
	pcre_fullinfo(aCode, NULL, PCRE_INFO_BACKREFMAX, &anOffset);
	if (aCount < anOffset)
		aCount = anOffset;
	aCount *= 3;
	return pcre_exec(aCode, NULL, aString, c_strlen(aString), theOffset, 0, (int*)offsets, aCount);
#endif
	return 0;
}

txSlot* fxNewRegExpInstance(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;

	anInstance = fxNewSlot(the);
	anInstance->flag = XS_VALUE_FLAG;
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = C_NULL;
	if (the->stack->kind == XS_ALIAS_KIND)
		anInstance->ID = the->stack->value.alias;
	else
		anInstance->value.instance.prototype = the->stack->value.reference;
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;

	aProperty = anInstance->next = fxNewSlot(the);
	aProperty->ID = XS_NO_ID;
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_REGEXP_KIND;
	aProperty->value.regexp.code = (void*)mxEmptyString.value.string;
	aProperty->value.regexp.offsets = (txInteger*)mxEmptyString.value.string;

	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->ID = fxID(the, "lastIndex");
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	aProperty->kind = XS_INTEGER_KIND;
	aProperty->value.integer = 0;
	
	return anInstance;
}

void fx_RegExp(txMachine* the)
{
	txSlot* aRegExp;
	txSlot* aPattern;
	txSlot* aProperty;
	char c, g = 0, i = 0, m = 0;
	txInteger aCount, anIndex;
	txFlag aFlag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
#if mxRegExp
	txInteger options;
	char* aMessage;
	txInteger anOffset;
	pcre* aCode;
	txSize aSize;
    txChunk* aChunk;
#endif

	if ((mxArgc > 0) && (mxIsReference(mxArgv(0))) && (aRegExp = fxGetInstance(the, mxArgv(0))) && (mxIsRegExp(aRegExp))) {
		if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND))
			mxDebug0(the, XS_TYPE_ERROR, "RegExp: no flags with a RegExp pattern");
		if (mxResult->kind == XS_UNDEFINED_KIND) {
			*mxResult = *mxArgv(0);
			return;
		}
		aPattern = fxGetProperty(the, aRegExp, fxID(the, "source"));
		aProperty = fxGetProperty(the, aRegExp, the->globalID);
		if (aProperty->value.boolean)
			g = 1;
		aProperty = fxGetProperty(the, aRegExp, fxID(the, "ignoreCase"));
		if (aProperty->value.boolean)
			i = 1;
		aProperty = fxGetProperty(the, aRegExp, fxID(the, "multiline"));
		if (aProperty->value.boolean)
			m = 1;
	}
	else {
		if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
			fxToString(the, mxArgv(0));
			aPattern = mxArgv(0);
		}
		else
			aPattern = &mxEmptyString;
		if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {	
			fxToString(the, mxArgv(1));
			aCount = c_strlen(mxArgv(1)->value.string);
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				c = mxArgv(1)->value.string[anIndex];
				if ((c == 'g') && !g)
					g = 1;
				else if ((c == 'i') && !i)
					i = 1;
				else if ((c == 'm') && !m)
					m = 1;
				else
					mxDebug0(the, XS_SYNTAX_ERROR, "RegExp: invalid flags");
			}
		}
	}
	
	if (mxResult->kind == XS_UNDEFINED_KIND) {
		mxPush(mxRegExpPrototype);
		aRegExp = fxNewRegExpInstance(the);
		*mxResult = *(the->stack++);
	}
	else
		aRegExp = fxGetOwnInstance(the, mxResult);
	mxCheckRegExp(aRegExp);
	aProperty = fxSetProperty(the, aRegExp, fxID(the, "source"), &aFlag);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->value.string = aPattern->value.string;
	aProperty->kind = XS_STRING_KIND;
	
	aProperty = fxSetProperty(the, aRegExp, the->globalID, &aFlag);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->value.boolean = g ? 1 : 0;
	aProperty->kind = XS_BOOLEAN_KIND;
	
	aProperty = fxSetProperty(the, aRegExp, fxID(the, "ignoreCase"), &aFlag);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->value.boolean = i ? 1 : 0;
	aProperty->kind = XS_BOOLEAN_KIND;
	
	aProperty = fxSetProperty(the, aRegExp, fxID(the, "multiline"), &aFlag);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->value.boolean = m ? 1 : 0;
	aProperty->kind = XS_BOOLEAN_KIND;

#if mxRegExp
	options = 0; //PCRE_UTF8;
	if (i)
		options |= PCRE_CASELESS;
	if (m)
		options |= PCRE_MULTILINE;
	aCode = pcre_compile(aPattern->value.string, options, (const char **)(void*)&aMessage, (int*)(void*)&anOffset, NULL); /* use default character tables */
	if (aCode == NULL)
		mxDebug1(the, XS_SYNTAX_ERROR, "RegExp: invalid pattern: %s", aMessage);
	pcre_fullinfo(aCode, NULL, PCRE_INFO_SIZE, &aSize);
	aChunk = fxNewChunk(the, aSize);
	c_memcpy(aChunk, aCode, aSize);
	aRegExp->next->value.regexp.code = aChunk;
	pcre_fullinfo(aCode, NULL, PCRE_INFO_CAPTURECOUNT, &aCount);
	aCount++;
	pcre_fullinfo(aCode, NULL, PCRE_INFO_BACKREFMAX, &anOffset);
	if (aCount < anOffset)
		aCount = anOffset;
	aCount *= 3;
	aRegExp->next->value.regexp.offsets = fxNewChunk(the, aCount * sizeof(txInteger));
	(pcre_free)(aCode);
#endif
}

void fx_RegExp_exec(txMachine* the)
{
	txSlot* aRegExp;
	txSlot* aProperty;
	txBoolean aGlobalFlag;
	txInteger anOffset;
	txInteger aCount;
	txSlot* anArray;
	txSlot* anItem;
	txInteger aLength;
	txInteger anIndex;

	aRegExp = fxGetOwnInstance(the, mxThis);
	mxCheckRegExp(aRegExp);
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "RegExp.exec: not enough parameters");
	fxToString(the, mxArgv(0));

	aProperty = fxGetProperty(the, aRegExp, the->globalID);
	aGlobalFlag = aProperty->value.boolean;
	
	if (aGlobalFlag) {
		aProperty = fxGetProperty(the, aRegExp, fxID(the, "lastIndex"));
		anOffset = fxUnicodeToUTF8Offset(mxArgv(0)->value.string, aProperty->value.integer);
		aProperty->value.integer = 0;
	}
	else
		anOffset = 0;
	 
	mxResult->kind = XS_NULL_KIND;
	
	aCount = fxExecuteRegExp(the, aRegExp, mxArgv(0), anOffset);
	if (aCount > 0) {
		if (aGlobalFlag)
			aProperty->value.integer = fxUTF8ToUnicodeOffset(mxArgv(0)->value.string, aRegExp->next->value.regexp.offsets[1]);
		mxPush(mxArrayPrototype);
		anArray = fxNewArrayInstance(the);
		*mxResult = *(the->stack++);
		anItem = aProperty = anArray->next;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			anItem->next = fxNewSlot(the);
			anItem = anItem->next;
			anOffset = aRegExp->next->value.regexp.offsets[2 * anIndex];
			if (anOffset >= 0) {
				aLength = aRegExp->next->value.regexp.offsets[(2 * anIndex) + 1] - anOffset;
				anItem->value.string = (txString)fxNewChunk(the, aLength + 1);
				c_memcpy(anItem->value.string, mxArgv(0)->value.string + anOffset, aLength);
				anItem->value.string[aLength] = 0;
				anItem->kind = XS_STRING_KIND;
			}
			aProperty->value.array.length++;
		}
		pcre_fullinfo((const pcre *)aRegExp->next->value.regexp.code, NULL, PCRE_INFO_CAPTURECOUNT, &aCount);
		for (; anIndex <= aCount; anIndex++) {
			anItem->next = fxNewSlot(the);
			anItem = anItem->next;
			aProperty->value.array.length++;
		}
		fxCacheArray(the, anArray);
		anItem = anArray->next;
		anItem->next = fxNewSlot(the);
		anItem = anItem->next;
		anItem->ID = the->indexID;
		anItem->kind = XS_INTEGER_KIND;
		anItem->value.integer = fxUTF8ToUnicodeOffset(mxArgv(0)->value.string, aRegExp->next->value.regexp.offsets[0]);
		anItem->next = fxNewSlot(the);
		anItem = anItem->next;
		anItem->ID = the->inputID;
		anItem->value.string = mxArgv(0)->value.string;
		anItem->kind = mxArgv(0)->kind;
	}
}

void fx_RegExp_test(txMachine* the)
{
	txSlot* aRegExp;

	aRegExp = fxGetInstance(the, mxThis);
	mxCheckRegExp(aRegExp);
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "RegExp.test: not enough parameters");
	fxToString(the, mxArgv(0));
	mxResult->value.boolean = fxExecuteRegExp(the, aRegExp, mxArgv(0), 0) > 0 ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_RegExp_toString(txMachine* the)
{
	txSlot* aRegExp;
	txSlot* aProperty;
	
	aRegExp = fxGetInstance(the, mxThis);
	mxCheckRegExp(aRegExp);
	aProperty = fxGetProperty(the, aRegExp, fxID(the, "source"));
	fxCopyStringC(the, mxResult, "/");
	fxConcatString(the, mxResult, aProperty);
	fxConcatStringC(the, mxResult, "/");
	aProperty = fxGetProperty(the, aRegExp, the->globalID);
	if (aProperty->value.boolean)
		fxConcatStringC(the, mxResult, "g");
	aProperty = fxGetProperty(the, aRegExp, fxID(the, "ignoreCase"));
	if (aProperty->value.boolean)
		fxConcatStringC(the, mxResult, "i");
	aProperty = fxGetProperty(the, aRegExp, fxID(the, "multiline"));
	if (aProperty->value.boolean)
		fxConcatStringC(the, mxResult, "m");
}
