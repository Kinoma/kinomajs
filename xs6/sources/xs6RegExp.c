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
enum {
	XS_REGEXP_G = 1,
	XS_REGEXP_I = 2,
	XS_REGEXP_M = 4,
	XS_REGEXP_U = 8,
	XS_REGEXP_Y = 16,
};

extern txInteger fxExecuteRegExp(txMachine* the, txSlot* theRegExp, txSlot* theString, txInteger theStickyOffset, txInteger theGlobalOffset);
static void fxInitializeRegExp(txMachine* the);

static void fx_RegExp(txMachine* the);
static void fx_RegExp_prototype_get_flags(txMachine* the);
static void fx_RegExp_prototype_get_global(txMachine* the);
static void fx_RegExp_prototype_get_ignoreCase(txMachine* the);
static void fx_RegExp_prototype_get_multiline(txMachine* the);
static void fx_RegExp_prototype_get_source(txMachine* the);
static void fx_RegExp_prototype_get_sticky(txMachine* the);
static void fx_RegExp_prototype_get_unicode(txMachine* the);

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
	slot = fxNextHostAccessorProperty(the, slot, fx_RegExp_prototype_get_flags, C_NULL, mxID(_flags), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_RegExp_prototype_get_global, C_NULL, mxID(_global), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_RegExp_prototype_get_ignoreCase, C_NULL, mxID(_ignoreCase), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_RegExp_prototype_get_multiline, C_NULL, mxID(_multiline), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_RegExp_prototype_get_source, C_NULL, mxID(_source), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_RegExp_prototype_get_sticky, C_NULL, mxID(_sticky), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_RegExp_prototype_get_unicode, C_NULL, mxID(_unicode), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	for (builder = gx_RegExp_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "RegExp", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxRegExpPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_RegExp, 2, mxID(_RegExp), XS_GET_ONLY));
	slot = fxNextHostAccessorProperty(the, slot, fx_species_get, C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	fxNewHostFunction(the, fxInitializeRegExp, 2, XS_NO_ID);
	mxInitializeRegExpFunction = *the->stack;
	the->stack++;
#if mxRegExp
	pcre_setup();
#endif
}

txInteger fxExecuteRegExp(txMachine* the, txSlot* theRegExp, txSlot* theString, txInteger theStickyOffset, txInteger theGlobalOffset)
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
	return pcre_exec(aCode, NULL, aString + theStickyOffset, c_strlen(aString) - theStickyOffset, theGlobalOffset, 0, (int*)offsets, aCount);
#endif
	return 0;
}

void fxInitializeRegExp(txMachine* the) 
{
	txSlot* instance = fxToInstance(the, mxThis);
	txSlot* regexp = instance->next;
	txSlot* key = regexp->next;
	txString pattern;
	txString flags;
	txU4 sum;
	txInteger length, index;

	if (mxArgv(0)->kind == XS_UNDEFINED_KIND)
		*mxArgv(0) = mxEmptyString;
	else
		fxToString(the, mxArgv(0));
	pattern = mxArgv(0)->value.string;
	if (mxArgv(1)->kind == XS_UNDEFINED_KIND)
		*mxArgv(1) = mxEmptyString;
	else
		fxToString(the, mxArgv(1));
	flags = mxArgv(1)->value.string;
	
	if (mxArgv(0)->kind == XS_STRING_X_KIND)
		key->kind = XS_KEY_X_KIND;
	key->value.key.string = pattern;
	sum = 0;
	length = c_strlen(flags);
	for (index = 0; index < length; index++) {
		char c = flags[index];
		if ((c == 'g') && !(sum & XS_REGEXP_G))
			sum |= XS_REGEXP_G;
		else if ((c == 'i') && !(sum & XS_REGEXP_I))
			sum |= XS_REGEXP_I;
		else if ((c == 'm') && !(sum & XS_REGEXP_M))
			sum |= XS_REGEXP_M;
		else if ((c == 'u') && !(sum & XS_REGEXP_U))
			sum |= XS_REGEXP_U;
		else if ((c == 'y') && !(sum & XS_REGEXP_Y))
			sum |= XS_REGEXP_Y;
		else
			mxSyntaxError("invalid flags");
	}
	key->value.key.sum = sum;
	
#if mxRegExp
{
	txInteger options;
	char* aMessage;
	txInteger aCount;
	txInteger anOffset;
	pcre* aCode;
	txSize aSize;
    txChunk* aChunk;

	options = 0; //PCRE_UTF8;
	if (sum & XS_REGEXP_I)
		options |= PCRE_CASELESS;
	if (sum & XS_REGEXP_M)
		options |= PCRE_MULTILINE;
	aCode = pcre_compile(pattern, options, (const char **)(void*)&aMessage, (int*)(void*)&anOffset, NULL); /* use default character tables */
	if (aCode == NULL)
		mxSyntaxError("invalid pattern: %s", aMessage);
	pcre_fullinfo(aCode, NULL, PCRE_INFO_SIZE, &aSize);
	aChunk = fxNewChunk(the, aSize);
	c_memcpy(aChunk, aCode, aSize);
	regexp->value.regexp.code = aChunk;
	pcre_fullinfo(aCode, NULL, PCRE_INFO_CAPTURECOUNT, &aCount);
	aCount++;
	pcre_fullinfo(aCode, NULL, PCRE_INFO_BACKREFMAX, &anOffset);
	if (aCount < anOffset)
		aCount = anOffset;
	aCount *= 3;
	regexp->value.regexp.offsets = fxNewChunk(the, aCount * sizeof(txInteger));
	(pcre_free)(aCode);
}
#endif
	*mxResult = *mxThis;
}

txBoolean fxIsRegExp(txMachine* the, txSlot* slot)
{
    if (mxIsReference(slot)) {
        txSlot* instance = slot->value.reference;
        mxPushSlot(slot);
        fxGetID(the, mxID(_Symbol_match));
        if (the->stack->kind != XS_UNDEFINED_KIND)
            return fxToBoolean(the, the->stack++);
        the->stack++;
        if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_REGEXP_KIND))
            return 1;
    }
	return 0;
}

txSlot* fxNewRegExpInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_VALUE_FLAG;

	property = instance->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_REGEXP_KIND;
	property->value.regexp.code = (void*)mxEmptyString.value.string;
	property->value.regexp.offsets = (txInteger*)mxEmptyString.value.string;

	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_KEY_KIND;
	property->value.key.string = mxEmptyString.value.string;
	property->value.key.sum = 0;

	property = fxNextIntegerProperty(the, property, 0, mxID(_lastIndex), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	
	return instance;
}

void fx_RegExp(txMachine* the)
{
	txSlot* aRegExp = C_NULL;
	txSlot* pattern = ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) ? mxArgv(0) : C_NULL;
	txSlot* flags = ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) ? mxArgv(1) : C_NULL;
	txBoolean patternIsRegExp = (pattern && fxIsRegExp(the, pattern)) ? 1 : 0;
	if (mxTarget->kind == XS_UNDEFINED_KIND) {
		if (patternIsRegExp && !flags) {
			mxPushSlot(pattern);
			fxGetID(the, mxID(_constructor));
			if ((the->stack->kind == mxFunction->kind) && (the->stack->value.reference == mxFunction->value.reference)) {
				the->stack++;
				*mxResult = *pattern;
				return;
			}
			the->stack++;
		}
		mxPush(mxRegExpPrototype);
		aRegExp = fxNewRegExpInstance(the);
		mxPullSlot(mxResult);
	}
	else {
		aRegExp = fxGetInstance(the, mxThis);
		mxCheckRegExp(aRegExp);
	}
	if (patternIsRegExp) {
		mxPushSlot(pattern);
		fxGetID(the, mxID(_source));
		if (flags)
			mxPushSlot(flags);
		else {
			mxPushSlot(pattern);
			fxGetID(the, mxID(_flags));
		}
	}
	else {
		if (pattern)
			mxPushSlot(pattern);
		else
			mxPushUndefined();
		if (flags)
			mxPushSlot(flags);
		else
			mxPushUndefined();
	}
	mxPushInteger(2);
	mxPushReference(aRegExp);
	mxPush(mxInitializeRegExpFunction);
	fxCall(the);
	mxPullSlot(mxResult);
}

void fx_RegExp_prototype_get_flags(txMachine* the)
{
	char flags[6];
	txIndex index = 0;
	fxToInstance(the, mxThis);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_global));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 'g';
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_ignoreCase));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 'i';
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_multiline));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 'm';
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_unicode));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 'u';
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_sticky));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 'y';
	flags[index] = 0;
	fxCopyStringC(the, mxResult, flags);
}

void fx_RegExp_prototype_get_global(txMachine* the)
{
	txSlot* slot = fxGetInstance(the, mxThis);
	mxCheckRegExp(slot);
	slot = slot->next->next;
	mxResult->value.boolean = (slot->value.key.sum & XS_REGEXP_G) ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_RegExp_prototype_get_ignoreCase(txMachine* the)
{
	txSlot* slot = fxGetInstance(the, mxThis);
	mxCheckRegExp(slot);
	slot = slot->next->next;
	mxResult->value.boolean = (slot->value.key.sum & XS_REGEXP_I) ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_RegExp_prototype_get_multiline(txMachine* the)
{
	txSlot* slot = fxGetInstance(the, mxThis);
	mxCheckRegExp(slot);
	slot = slot->next->next;
	mxResult->value.boolean = (slot->value.key.sum & XS_REGEXP_M) ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_RegExp_prototype_get_source(txMachine* the)
{
	txSlot* slot = fxGetInstance(the, mxThis);
	mxCheckRegExp(slot);
	slot = slot->next->next;
	mxResult->value.string = slot->value.key.string;
	mxResult->kind = XS_STRING_KIND;
}

void fx_RegExp_prototype_get_sticky(txMachine* the)
{
	txSlot* slot = fxGetInstance(the, mxThis);
	mxCheckRegExp(slot);
	slot = slot->next->next;
	mxResult->value.boolean = (slot->value.key.sum & XS_REGEXP_Y) ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_RegExp_prototype_get_unicode(txMachine* the)
{
	txSlot* slot = fxGetInstance(the, mxThis);
	mxCheckRegExp(slot);
	slot = slot->next->next;
	mxResult->value.boolean = (slot->value.key.sum & XS_REGEXP_U) ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_RegExp_prototype_compile(txMachine* the)
{
	// nop
}

void fx_RegExp_prototype_exec(txMachine* the)
{
	txSlot* aProperty;
	txSlot* argument;
	txBoolean aGlobalFlag;
	txBoolean aStickyFlag;
	txInteger aGlobalOffset;
	txInteger aStickyOffset;
	txInteger anOffset;
	txInteger aCount;
	txSlot* anArray;
	txSlot* anItem;
	txInteger aLength;
	txInteger anIndex;
	txSlot* aRegExp = fxToInstance(the, mxThis);
	mxCheckRegExp(aRegExp);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	fxToString(the, the->stack);
	argument = the->stack;

	aProperty = aRegExp->next->next;
	aGlobalFlag = (aProperty->value.key.sum & XS_REGEXP_G) ? 1 : 0;
	aStickyFlag = (aProperty->value.key.sum & XS_REGEXP_Y) ? 1 : 0;
	
	if (aGlobalFlag) {
		aProperty = fxGetProperty(the, aRegExp, mxID(_lastIndex));
		aGlobalOffset = fxUnicodeToUTF8Offset(mxArgv(0)->value.string, aProperty->value.integer);
		aProperty->value.integer = 0;
	}
	else
		aGlobalOffset = 0;
	if (aStickyFlag) {
		aProperty = fxGetProperty(the, aRegExp, mxID(_lastIndex));
		aStickyOffset = fxUnicodeToUTF8Offset(mxArgv(0)->value.string, aProperty->value.integer);
		aProperty->value.integer = 0;
	}
	else
		aStickyOffset = 0;
	 
	mxResult->kind = XS_NULL_KIND;
	
	aCount = fxExecuteRegExp(the, aRegExp, argument, aStickyOffset, aGlobalOffset);
	if (aCount > 0) {
		if (aGlobalFlag || aStickyFlag)
			aProperty->value.integer = fxUTF8ToUnicodeOffset(argument->value.string, aStickyOffset + aRegExp->next->value.regexp.offsets[1]);
		mxPush(mxArrayPrototype);
		anArray = fxNewArrayInstance(the);
		mxPullSlot(mxResult);
		anItem = aProperty = anArray->next;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			anItem->next = fxNewSlot(the);
			anItem = anItem->next;
			anOffset = aStickyOffset + aRegExp->next->value.regexp.offsets[2 * anIndex];
			if (anOffset >= 0) {
				aLength = aStickyOffset + aRegExp->next->value.regexp.offsets[(2 * anIndex) + 1] - anOffset;
				anItem->value.string = (txString)fxNewChunk(the, aLength + 1);
				c_memcpy(anItem->value.string, argument->value.string + anOffset, aLength);
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
		anItem->value.integer = fxUTF8ToUnicodeOffset(argument->value.string, aStickyOffset + aRegExp->next->value.regexp.offsets[0]);
		anItem->next = fxNewSlot(the);
		anItem = anItem->next;
		anItem->ID = mxID(_input);
		anItem->value.string = argument->value.string;
		anItem->kind = XS_STRING_KIND;
	}
	the->stack++;
}

void fx_RegExp_prototype_match(txMachine* the)
{
	txSlot* argument;
	fxToInstance(the, mxThis);
	if (mxArgc == 0)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	argument = the->stack;
	fxToString(the, argument);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_global));
	if (fxToBoolean(the, the->stack++)) {
		txIndex count = 0;
		mxPushSlot(mxThis);
		fxGetID(the, mxID(_unicode));
		mxPop();
		mxPushInteger(0);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_lastIndex));
		mxPop();
		mxPush(mxArrayPrototype);
		fxNewArrayInstance(the);
		mxPullSlot(mxResult);
		for (;;) {
			mxPushSlot(argument);
			mxPushInteger(1);
			mxPushSlot(mxThis);
			fxCallID(the, mxID(_exec));
			if (the->stack->kind == XS_NULL_KIND) {
				if (count == 0)
					mxPullSlot(mxResult);
				else
					mxPop();
				break;
			}
			fxGetID(the, 0);
			fxDefineDataProperty(the, mxResult->value.reference, count, the->stack);
			mxPop();
			count++;
		}
	}
	else {
		mxPushSlot(argument);
		mxPushInteger(1);
		mxPushSlot(mxThis);
		fxCallID(the, mxID(_exec));
		mxPullSlot(mxResult);
	}
	mxPop();
}

void fx_RegExp_prototype_split_aux(txMachine* the, txSlot* string, txIndex start, txIndex stop, txSlot* item)
{
	txInteger anOffset = fxUnicodeToUTF8Offset(string->value.string, start);
	txInteger aLength = fxUnicodeToUTF8Offset(string->value.string + anOffset, stop - start);
	if ((anOffset >= 0) && (aLength > 0)) {
		item->value.string = (txString)fxNewChunk(the, aLength + 1);
		c_memcpy(item->value.string, string->value.string + anOffset, aLength);
		item->value.string[aLength] = 0;
		item->kind = XS_STRING_KIND;
	}
	else {
		item->value.string = mxEmptyString.value.string;
		item->kind = mxEmptyString.kind;
	}
}


void fx_RegExp_prototype_replace(txMachine* the)
{
/*
	txSlot* aRegExp;
	txString aString;
	txInteger aLength;
	txSlot* aProperty;
	txInteger aDelta;
	txInteger anOffset;
	txInteger aCount;
	
	aRegExp = fxGetInstance(the, mxThis);
	mxCheckRegExp(aRegExp);
	if (mxArgc < 1)
		mxSyntaxError("not enough parameters");
	aString = fxToString(the, mxArgv(0));
	aLength = c_strlen(aString);
	*mxResult = *mxArgv(0);
	aProperty = fxGetProperty(the, aRegExp, mxID(_global));
	if (aProperty->value.boolean) {
		aDelta = 0;
		anOffset = 0;
		for (;;) {
			aCount = fxExecuteRegExp(the, aRegExp, mxArgv(0), 0, anOffset);
			if (aCount <= 0)
				break;
			aDelta = fx_String_prototype_replace_aux(the, mxArgv(0), aDelta, aLength, aCount, aRegExp->next);
			if (anOffset == aRegExp->next->value.regexp.offsets[1])
				anOffset++;
			else
				anOffset = aRegExp->next->value.regexp.offsets[1];
		}
	}
	else {
		aCount = fxExecuteRegExp(the, aRegExp, mxArgv(0), 0, 0);
		if (aCount > 0)
			fx_String_prototype_replace_aux(the, mxArgv(0), 0, aLength, aCount, aRegExp->next);
	}
*/
	{
		txSlot* argument;
		txSlot* function = C_NULL;;
		txSlot* replacement = C_NULL;;
		txBoolean globalFlag;
		txSlot* list;
		txSlot* item;
		txInteger size; 
		txInteger utf8Size; 
		txInteger former; 
		txSlot* result;
		txSlot* matched;
		txInteger matchLength; 
		txInteger c, i;
		txInteger position; 
		fxToInstance(the, mxThis);
		if (mxArgc <= 0)
			mxPushUndefined();
		else
			mxPushSlot(mxArgv(0));
		argument = the->stack;
		fxToString(the, argument);
		if (mxArgc <= 1)
			mxPushUndefined();
		else
			mxPushSlot(mxArgv(1));
		if (mxIsReference(the->stack) && mxIsFunction(the->stack->value.reference))
			function = the->stack;
		else {		
			replacement = the->stack;
			fxToString(the, replacement);
		}
		mxPushSlot(mxThis);
		fxGetID(the, mxID(_global));
		globalFlag = fxToBoolean(the, the->stack++);
		if (globalFlag) {
			mxPushSlot(mxThis);
			fxGetID(the, mxID(_unicode));
			the->stack++;
            
			mxPushInteger(0);
			mxPushSlot(mxThis);
			fxSetID(the, mxID(_lastIndex));
			the->stack++;
		}
		list = item = fxNewSlot(the);
		mxPushSlot(list);
		size = fxUnicodeLength(argument->value.string);
		utf8Size = c_strlen(argument->value.string);
		former = 0;
		for (;;) {
			mxPushSlot(argument);
			mxPushInteger(1);
			mxPushSlot(mxThis);
			fxCallID(the, mxID(_exec));
			if (the->stack->kind == XS_NULL_KIND) {
				the->stack++;
				break;
			}
			result = the->stack;
			
			
			mxPushSlot(result);
			fxGetID(the, mxID(_index));
			position = fxToInteger(the, the->stack++);
			if (position < 0) position = 0;
			else if (position > size) position = size;
			
			if (former < position) {
				item = item->next = fxNewSlot(the);
				fx_RegExp_prototype_split_aux(the, argument, former, position, item);
			}

			mxPushSlot(result);
			fxGetID(the, 0);
			fxToString(the, the->stack);
			matched = the->stack;
			matchLength = fxUnicodeLength(matched->value.string);

			mxPushSlot(result);
			fxGetID(the, mxID(_length));
			c = fxToInteger(the, the->stack++);

			mxPushSlot(matched);
			for (i = 1; i < c; i++) {
				mxPushSlot(result);
				fxGetID(the, i);
				fxToString(the, the->stack);
			}
			if (function) {
				mxPushInteger(position);
				mxPushSlot(argument);
				mxPushInteger(3 + i - 1);
				mxPushUndefined();
				mxPushSlot(function);
				fxCall(the);
				fxToString(the, the->stack);
				item = item->next = fxNewSlot(the);
				mxPullSlot(item);
			}
			else {
				fxPushSubstitutionString(the, argument, utf8Size, fxUnicodeToUTF8Offset(argument->value.string, position), matched, c_strlen(matched->value.string), i - 1, the->stack, replacement);
				item = item->next = fxNewSlot(the);
				mxPullSlot(item);
				the->stack += i;			
			}
			former = position + matchLength;

			if (!globalFlag)
				break;
				
			if (0 == matchLength) {
				mxPushSlot(mxThis);
				fxGetID(the, mxID(_lastIndex));
				fxToInteger(the, the->stack);
				the->stack->value.integer++;
				mxPushSlot(mxThis);
				fxSetID(the, mxID(_lastIndex));
				the->stack++;
			}
			the->stack++;
		}
		if (former < size) {
			item = item->next = fxNewSlot(the);
			fx_RegExp_prototype_split_aux(the, argument, former, size, item);
		}
		size = 0;
		item = list->next;
		while (item) {
			item->value.key.sum = c_strlen(item->value.string);
			size += item->value.key.sum;
			item = item->next;
		}
		size++;
		mxResult->value.string = (txString)fxNewChunk(the, size);
		size = 0;
		item = list->next;
		while (item) {
			c_memcpy(mxResult->value.string + size, item->value.string, item->value.key.sum);
			size += item->value.key.sum;
			item = item->next;
		}
		mxResult->value.string[size] = 0;
		mxResult->kind = XS_STRING_KIND;
		the->stack++;
		the->stack++;
	}
}

void fx_RegExp_prototype_search(txMachine* the)
{
	txSlot* argument;
	txIndex lastIndex = 0;
	fxToInstance(the, mxThis);
	if (mxArgc == 0)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	argument = the->stack;
	fxToString(the, argument);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_lastIndex));
	lastIndex = fxToInteger(the, the->stack++);
	mxPushInteger(0);
	mxPushSlot(mxThis);
	fxSetID(the, mxID(_lastIndex));
	mxPop();
	mxPushSlot(argument);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxCallID(the, mxID(_exec));
	mxPushInteger(lastIndex);
	mxPushSlot(mxThis);
	fxSetID(the, mxID(_lastIndex));
	mxPop();
	if (the->stack->kind == XS_NULL_KIND) {
		the->stack->kind = XS_INTEGER_KIND;
		the->stack->value.integer = -1;
	}
	else
		fxGetID(the, mxID(_index));
	mxPullSlot(mxResult);
	mxPop();
}

void fx_RegExp_prototype_split(txMachine* the)
{
	txSlot* argument;
	txIndex limit;
	txSlot* constructor;
	txSlot* splitter;
	txSlot* array;
	txSlot* item;
	txIndex size, p, q, c, i;
	
	fxToInstance(the, mxThis);
	if (mxArgc == 0)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	argument = the->stack;
	fxToString(the, argument);
	limit = fxArgToArrayLimit(the, 1);
	
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_constructor));
	if (the->stack->kind == XS_UNDEFINED_KIND) {
		*the->stack = mxGlobal;
		fxGetID(the, mxID(_RegExp));
	}
	else if (the->stack->kind != XS_REFERENCE_KIND) {
		mxTypeError("this.constructor is no object");
	}
	else {
		fxGetID(the, mxID(_Symbol_species));
		if ((the->stack->kind == XS_UNDEFINED_KIND) || (the->stack->kind == XS_NULL_KIND)) {
			*the->stack = mxGlobal;
			fxGetID(the, mxID(_RegExp));
		}
		if (the->stack->kind != XS_REFERENCE_KIND) {
			mxTypeError("this.constructor[Symbol.species] is no object");
		}
		else if (!mxIsFunction(the->stack->value.reference)) {
			mxTypeError("this.constructor[Symbol.species] is no function");
		} 
	}
	constructor = the->stack;	
	
	mxPushSlot(mxThis);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_flags));
	if (!c_strchr(fxToString(the, the->stack), 'y'))
		fxConcatStringC(the, the->stack, "y");
	mxPushInteger(2);
	mxPushSlot(constructor);
	fxNew(the);	
	splitter = the->stack;
	
	mxPush(mxArrayPrototype);
	array = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	item = array->next;
	if (!limit)
		goto bail;
	size = fxUnicodeLength(argument->value.string);
	if (size == 0) {
		mxPushSlot(argument);
		mxPushInteger(1);
		mxPushSlot(splitter);
		fxCallID(the, mxID(_exec));
		if (the->stack->kind == XS_NULL_KIND) {
			item = item->next = fxNewSlot(the);
			item->value.string = mxEmptyString.value.string;
			item->kind = mxEmptyString.kind;
			array->next->value.array.length++;
		}
		mxPop();
		goto bail;
	}
	q = p = 0;
	while (q < size) {
		mxPushInteger(q);
		mxPushSlot(splitter);
		fxSetID(the, mxID(_lastIndex));
		mxPop();
		
		mxPushSlot(argument);
		mxPushInteger(1);
		mxPushSlot(splitter);
		fxCallID(the, mxID(_exec));
		if (the->stack->kind == XS_NULL_KIND) {
			q++;
		}
		else {
			txSlot* result = the->stack;
			mxPushSlot(result);
			fxGetID(the, mxID(_index));
			q = fxToInteger(the, the->stack++);
			if (q == p)
				q++;
			else {
				item = item->next = fxNewSlot(the);
				fx_RegExp_prototype_split_aux(the, argument, p, q, item);
				array->next->value.array.length++;
				mxPushSlot(result);
				fxGetID(the, mxID(_length));
				c = fxToInteger(the, the->stack++);
				c--; if (c < 0) c = 0;
				i = 1;
				while (i <= c) {
					mxPushSlot(result);
					fxGetID(the, i);
					item = item->next = fxNewSlot(the);
					mxPullSlot(item);
					array->next->value.array.length++;
					i++;
				}
				mxPushSlot(splitter);
				fxGetID(the, mxID(_lastIndex));
				q = p = fxToInteger(the, the->stack++);
			}
		}
		mxPop();
	}
	item = item->next = fxNewSlot(the);
	fx_RegExp_prototype_split_aux(the, argument, p, size, item);
	array->next->value.array.length++;
bail:
	fxCacheArray(the, array);
	mxPop();
	mxPop();
	mxPop();
}

void fx_RegExp_prototype_test(txMachine* the)
{
	fxToInstance(the, mxThis);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	fxToString(the, the->stack);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxCallID(the, mxID(_exec));
	mxResult->value.boolean = (the->stack->kind != XS_NULL_KIND) ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	the->stack++;
}

void fx_RegExp_prototype_toString(txMachine* the)
{
	fxToInstance(the, mxThis);
	fxCopyStringC(the, mxResult, "/");
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_source));
	fxConcatString(the, mxResult, the->stack);
	the->stack++;
	fxConcatStringC(the, mxResult, "/");
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_flags));
	fxConcatString(the, mxResult, the->stack);
	the->stack++;
}
