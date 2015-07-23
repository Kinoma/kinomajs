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
#include "xs6All.h"
#include "xs_pcre.h"

#define mxCheckRegExp(THE_SLOT) \
	if (!mxIsRegExp(THE_SLOT)) \
		mxTypeError("this is no RegExp")
		
#define mxOffsetCount 30

static void fx_RegExp(txMachine* the);
static void fx_RegExp_prototype_compile(txMachine* the);
static void fx_RegExp_prototype_exec(txMachine* the);
static void fx_RegExp_prototype_match(txMachine* the);
static void fx_RegExp_prototype_replace(txMachine* the);
static void fx_RegExp_prototype_search(txMachine* the);
static void fx_RegExp_prototype_split(txMachine* the);
static void fx_RegExp_prototype_test(txMachine* the);
static void fx_RegExp_prototype_toString(txMachine* the);

void fxBuildRegExp(txMachine* the)
{
    static const txHostFunctionBuilder gx_RegExp_prototype_builders[] = {
		{ fx_RegExp_prototype_compile, 0, _compile },
		{ fx_RegExp_prototype_exec, 0, _exec },
		{ fx_RegExp_prototype_match, 0, _Symbol_match },
		{ fx_RegExp_prototype_replace, 0, _Symbol_replace },
		{ fx_RegExp_prototype_search, 0, _Symbol_search },
		{ fx_RegExp_prototype_split, 0, _Symbol_split },
		{ fx_RegExp_prototype_test, 0, _test },
		{ fx_RegExp_prototype_toString, 0, _toString },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewRegExpInstance(the));
	for (builder = gx_RegExp_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextBooleanProperty(the, slot, 0, mxID(_global), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextBooleanProperty(the, slot, 0, mxID(_ignoreCase), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextBooleanProperty(the, slot, 0, mxID(_multiline), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextBooleanProperty(the, slot, 0, mxID(_unicode), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextBooleanProperty(the, slot, 0, mxID(_sticky), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextStringProperty(the, slot, "", mxID(_flags), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextStringProperty(the, slot, "", mxID(_source), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextStringProperty(the, slot, "RegExp", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxRegExpPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_RegExp, 2, mxID(_RegExp), XS_GET_ONLY));
	slot = fxNextHostAccessorProperty(the, slot, fx_species_get, C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
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
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_VALUE_FLAG;

	property = instance->next = fxNewSlot(the);
	property->ID = XS_NO_ID;
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_REGEXP_KIND;
	property->value.regexp.code = (void*)mxEmptyString.value.string;
	property->value.regexp.offsets = (txInteger*)mxEmptyString.value.string;

	property = fxNextIntegerProperty(the, property, 0, mxID(_lastIndex), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	
	return instance;
}

void fx_RegExp(txMachine* the)
{
	txSlot* aRegExp = C_NULL;
	txSlot* aPattern;
	char g = 0, i = 0, m = 0, u = 0, y = 0;
	txInteger aCount, anIndex;
	txSlot* aProperty;
	char flags[6];
#if mxRegExp
	txInteger options;
	char* aMessage;
	txInteger anOffset;
	pcre* aCode;
	txSize aSize;
    txChunk* aChunk;
#endif
	if ((mxArgc > 0) && (mxIsReference(mxArgv(0))) && (aRegExp = fxGetInstance(the, mxArgv(0))) && (mxIsRegExp(aRegExp))) {
		if (mxTarget->kind == XS_UNDEFINED_KIND) {
			*mxResult = *mxArgv(0);
			return;
		}
		aPattern = fxGetProperty(the, aRegExp, mxID(_source));
	}
	else {
		if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
			fxToString(the, mxArgv(0));
			aPattern = mxArgv(0);
		}
		else
			aPattern = &mxEmptyString;
	}
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {	
		fxToString(the, mxArgv(1));
		aCount = c_strlen(mxArgv(1)->value.string);
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			char c = mxArgv(1)->value.string[anIndex];
			if ((c == 'g') && !g)
				g = 1;
			else if ((c == 'i') && !i)
				i = 1;
			else if ((c == 'm') && !m)
				m = 1;
			else if ((c == 'u') && !u)
				u = 1;
			else if ((c == 'y') && !y)
				y = 1;
			else
				mxSyntaxError("invalid flags");
		}
	}
	else if (aRegExp) {
		aProperty = fxGetProperty(the, aRegExp, mxID(_global));
		if (aProperty->value.boolean)
			g = 1;
		aProperty = fxGetProperty(the, aRegExp, mxID(_ignoreCase));
		if (aProperty->value.boolean)
			i = 1;
		aProperty = fxGetProperty(the, aRegExp, mxID(_multiline));
		if (aProperty->value.boolean)
			m = 1;
		aProperty = fxGetProperty(the, aRegExp, mxID(_unicode));
		if (aProperty->value.boolean)
			u = 1;
		aProperty = fxGetProperty(the, aRegExp, mxID(_sticky));
		if (aProperty->value.boolean)
			y = 1;
	}
	
	if (mxTarget->kind == XS_UNDEFINED_KIND) {
		mxPush(mxRegExpPrototype);
		aRegExp = fxNewRegExpInstance(the);
		mxPullSlot(mxResult);
	}
	else
		aRegExp = fxGetInstance(the, mxThis);
	mxCheckRegExp(aRegExp);
	aProperty = fxLastProperty(the, aRegExp);
	aProperty = fxNextBooleanProperty(the, aProperty, g, mxID(_global), XS_GET_ONLY);
	aProperty = fxNextBooleanProperty(the, aProperty, i, mxID(_ignoreCase), XS_GET_ONLY);
	aProperty = fxNextBooleanProperty(the, aProperty, m, mxID(_multiline), XS_GET_ONLY);
	aProperty = fxNextBooleanProperty(the, aProperty, u, mxID(_unicode), XS_GET_ONLY);
	aProperty = fxNextBooleanProperty(the, aProperty, y, mxID(_sticky), XS_GET_ONLY);
	anIndex = 0;
	if (g) flags[anIndex++] = 'g';
	if (i) flags[anIndex++] = 'i';
	if (m) flags[anIndex++] = 'm';
	if (u) flags[anIndex++] = 'u';
	if (y) flags[anIndex++] = 'y';
	flags[anIndex]	= 0;
	aProperty = fxNextStringProperty(the, aProperty, flags, mxID(_flags), XS_GET_ONLY);
	aProperty = fxNextSlotProperty(the, aProperty, aPattern, mxID(_source), XS_GET_ONLY);
	
#if mxRegExp
	options = 0; //PCRE_UTF8;
	if (i)
		options |= PCRE_CASELESS;
	if (m)
		options |= PCRE_MULTILINE;
	aCode = pcre_compile(aPattern->value.string, options, (const char **)(void*)&aMessage, (int*)(void*)&anOffset, NULL); /* use default character tables */
	if (aCode == NULL)
		mxSyntaxError("invalid pattern: %s", aMessage);
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

void fx_RegExp_prototype_compile(txMachine* the)
{
	// nop
}

void fx_RegExp_prototype_exec(txMachine* the)
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

	aRegExp = fxGetInstance(the, mxThis);
	mxCheckRegExp(aRegExp);
	if (mxArgc < 1)
		mxSyntaxError("not enough parameters");
	fxToString(the, mxArgv(0));

	aProperty = fxGetProperty(the, aRegExp, mxID(_global));
	aGlobalFlag = aProperty->value.boolean;
	
	if (aGlobalFlag) {
		aProperty = fxGetProperty(the, aRegExp, mxID(_lastIndex));
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
		mxPullSlot(mxResult);
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
		anItem->ID = mxID(_index);
		anItem->kind = XS_INTEGER_KIND;
		anItem->value.integer = fxUTF8ToUnicodeOffset(mxArgv(0)->value.string, aRegExp->next->value.regexp.offsets[0]);
		anItem->next = fxNewSlot(the);
		anItem = anItem->next;
		anItem->ID = mxID(_input);
		anItem->value.string = mxArgv(0)->value.string;
		anItem->kind = mxArgv(0)->kind;
	}
}

void fx_RegExp_prototype_match(txMachine* the)
{
	mxUnknownError("TBD");
}

void fx_RegExp_prototype_replace(txMachine* the)
{
	mxUnknownError("TBD");
}

void fx_RegExp_prototype_search(txMachine* the)
{
	mxUnknownError("TBD");
}

void fx_RegExp_prototype_split(txMachine* the)
{
	mxUnknownError("TBD");
}

void fx_RegExp_prototype_test(txMachine* the)
{
	txSlot* aRegExp;

	aRegExp = fxGetInstance(the, mxThis);
	mxCheckRegExp(aRegExp);
	if (mxArgc < 1)
		mxSyntaxError("not enough parameters");
	fxToString(the, mxArgv(0));
	mxResult->value.boolean = fxExecuteRegExp(the, aRegExp, mxArgv(0), 0) > 0 ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_RegExp_prototype_toString(txMachine* the)
{
	txSlot* aRegExp;
	txSlot* aProperty;
	
	aRegExp = fxGetInstance(the, mxThis);
	mxCheckRegExp(aRegExp);
	aProperty = fxGetProperty(the, aRegExp, mxID(_source));
	fxCopyStringC(the, mxResult, "/");
	fxConcatString(the, mxResult, aProperty);
	fxConcatStringC(the, mxResult, "/");
	aProperty = fxGetProperty(the, aRegExp, mxID(_global));
	if (aProperty->value.boolean)
		fxConcatStringC(the, mxResult, "g");
	aProperty = fxGetProperty(the, aRegExp, mxID(_ignoreCase));
	if (aProperty->value.boolean)
		fxConcatStringC(the, mxResult, "i");
	aProperty = fxGetProperty(the, aRegExp, mxID(_multiline));
	if (aProperty->value.boolean)
		fxConcatStringC(the, mxResult, "m");
	aProperty = fxGetProperty(the, aRegExp, mxID(_unicode));
	if (aProperty->value.boolean)
		fxConcatStringC(the, mxResult, "u");
	aProperty = fxGetProperty(the, aRegExp, mxID(_sticky));
	if (aProperty->value.boolean)
		fxConcatStringC(the, mxResult, "y");
}
