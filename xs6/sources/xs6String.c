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
#if mxWindows
	#include <Winnls.h>
#elif (mxMacOSX || mxiOS)
	#include <CoreServices/CoreServices.h>
#endif

static void fxStringAccessorGetter(txMachine* the);
static void fxStringAccessorSetter(txMachine* the);
static void fx_String(txMachine* the);
static void fx_String_fromArrayBuffer(txMachine* the);
static void fx_String_fromCharCode(txMachine* the);
static void fx_String_raw(txMachine* the);
static void fx_String_prototype_charAt(txMachine* the);
static void fx_String_prototype_charCodeAt(txMachine* the);
static void fx_String_prototype_compare(txMachine* the);
static void fx_String_prototype_concat(txMachine* the);
static void fx_String_prototype_endsWith(txMachine* the);
static void fx_String_prototype_includes(txMachine* the);
static void fx_String_prototype_indexOf(txMachine* the);
static void fx_String_prototype_lastIndexOf(txMachine* the);
static void fx_String_prototype_match(txMachine* the);
static void fx_String_prototype_normalize(txMachine* the);
static void fx_String_prototype_repeat(txMachine* the);
static void fx_String_prototype_replace(txMachine* the);
static txInteger fx_String_prototype_replace_aux(txMachine* the, txInteger theDelta, txInteger theLength, txInteger theCount, txSlot* theOffsets);
static void fx_String_prototype_search(txMachine* the);
static void fx_String_prototype_slice(txMachine* the);
static void fx_String_prototype_split(txMachine* the);
static txSlot* fx_String_prototype_split_aux(txMachine* the, txSlot* theArray, txSlot* theItem, txInteger theStart, txInteger theStop);
static void fx_String_prototype_startsWith(txMachine* the);
static void fx_String_prototype_substr(txMachine* the);
static void fx_String_prototype_substring(txMachine* the);
static void fx_String_prototype_toCase(txMachine* the, txBoolean flag);
static void fx_String_prototype_toLowerCase(txMachine* the);
static void fx_String_prototype_toUpperCase(txMachine* the);
static void fx_String_prototype_trim(txMachine* the);
static void fx_String_prototype_valueOf(txMachine* the);
static void fx_String_prototype_iterator(txMachine* the);
static void fx_String_prototype_iterator_next(txMachine* the);

static txSlot* fxCheckString(txMachine* the, txSlot* it);
static txString fxCoerceToString(txMachine* the, txSlot* theSlot);
static txInteger fxArgToPosition(txMachine* the, txInteger i, txInteger index, txInteger length);
static txSlot* fxArgToRegExp(txMachine* the, txInteger i);

void fxBuildString(txMachine* the)
{
	static const txHostFunctionBuilder gx_String_prototype_builders[] = {
		{ fx_String_prototype_charAt, 1, _charAt },
		{ fx_String_prototype_charCodeAt, 1, _charCodeAt },
		{ fx_String_prototype_charCodeAt, 1, _codePointAt },
		{ fx_String_prototype_compare, 1, _compare },
		{ fx_String_prototype_concat, 1, _concat },
		{ fx_String_prototype_endsWith, 1, _endsWith },
		{ fx_String_prototype_includes, 1, _includes },
		{ fx_String_prototype_indexOf, 1, _indexOf },
		{ fx_String_prototype_lastIndexOf, 1, _lastIndexOf },
		{ fx_String_prototype_compare, 1, _localeCompare },
		{ fx_String_prototype_match, 1, _match },
		{ fx_String_prototype_normalize, 0, _normalize },
		{ fx_String_prototype_repeat, 1, _repeat },
		{ fx_String_prototype_replace, 2, _replace },
		{ fx_String_prototype_search, 1, _search },
		{ fx_String_prototype_slice, 2, _slice },
		{ fx_String_prototype_split, 2, _split },
		{ fx_String_prototype_startsWith, 2, _startsWith },
		{ fx_String_prototype_substr, 1, _substr },
		{ fx_String_prototype_substring, 2, _substring },
		{ fx_String_prototype_toLowerCase, 0, _toLocaleLowerCase },
		{ fx_String_prototype_valueOf, 0, _toLocaleString },
		{ fx_String_prototype_toUpperCase, 0, _toLocaleUpperCase },
		{ fx_String_prototype_toLowerCase, 0, _toLowerCase },
		{ fx_String_prototype_valueOf, 0, _toString },
		{ fx_String_prototype_toUpperCase, 0, _toUpperCase },
		{ fx_String_prototype_trim, 0, _trim },
		{ fx_String_prototype_valueOf, 0, _valueOf },
		{ fx_String_prototype_iterator, 0, _Symbol_iterator },
		{ C_NULL, 0, 0 },
	};
	static const txHostFunctionBuilder gx_String_builders[] = {
		{ fx_String_fromArrayBuffer, 1, _fromArrayBuffer },
		{ fx_String_fromCharCode, 1, _fromCharCode },
		{ fx_String_fromCharCode, 1, _fromCodePoint },
		{ fx_String_raw, 1, _raw },
		{ C_NULL, 0, 0 },
	};
	const txHostFunctionBuilder* builder;
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewStringInstance(the));
	for (builder = gx_String_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "String", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxStringPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_String, 1, mxID(_String), XS_GET_ONLY));
	for (builder = gx_String_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);

	fxNewHostFunction(the, fxStringAccessorGetter, 0, XS_NO_ID);
	fxNewHostFunction(the, fxStringAccessorSetter, 1, XS_NO_ID);
	mxPushUndefined();
	the->stack->kind = XS_ACCESSOR_KIND;
	the->stack->value.accessor.getter = (the->stack + 2)->value.reference;
	the->stack->value.accessor.setter = (the->stack + 1)->value.reference;
	mxPull(mxStringAccessor);
	the->stack += 2;
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_String_prototype_iterator_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "String Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxStringIteratorPrototype);
}

txSlot* fxGetStringProperty(txMachine* the, txSlot* instance, txInteger index)
{
	instance->next->next->value.integer = index;
	return &mxStringAccessor;
}

txBoolean fxRemoveStringProperty(txMachine* the, txSlot* instance, txInteger index)
{
	return 0;
}

txSlot* fxSetStringProperty(txMachine* the, txSlot* instance, txInteger index)
{
	instance->next->next->value.integer = index;
	return &mxStringAccessor;
}

txSlot* fxNewStringInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_VALUE_FLAG;
	property = fxNextSlotProperty(the, instance, &mxEmptyString, XS_NO_ID, XS_GET_ONLY);
	property = fxNextIntegerProperty(the, property, 0, XS_NO_ID, XS_GET_ONLY);
	property = fxNextIntegerProperty(the, property, 0, mxID(_length), XS_GET_ONLY);
	return instance;
}

void fxStringAccessorGetter(txMachine* the)
{
	txSlot* instance = fxToInstance(the, mxThis);
	txString string = instance->next->value.string;
	txInteger index = instance->next->next->value.integer;
	txInteger from = fxUnicodeToUTF8Offset(string, index);
	if (from >= 0) {
		txInteger to = fxUnicodeToUTF8Offset(string, index + 1);
		if (to >= 0) {
			mxResult->value.string = fxNewChunk(the, to - from + 1);
			c_memcpy(mxResult->value.string, instance->next->value.string + from, to - from);
			mxResult->value.string[to - from] = 0;
			mxResult->kind = XS_STRING_KIND;
		}
	}
}

void fxStringAccessorSetter(txMachine* the)
{
}

void fx_String(txMachine* the)
{
	txSlot* slot;
	txSlot* instance;
	if (mxArgc > 0) {
		slot = mxArgv(0);
		if ((mxTarget->kind == XS_UNDEFINED_KIND) && (slot->kind == XS_SYMBOL_KIND)) {
			fxSymbolToString(the, slot);
			*mxResult = *slot;
			return;
		}
		fxToString(the, slot);
	}
	else {
		slot = &mxEmptyString;
	}
	if (mxTarget->kind == XS_UNDEFINED_KIND) {
		*mxResult = *slot;
		return;
	}
	instance = mxThis->value.reference;
	instance->next->value.string = slot->value.string;
	instance->next->next->next->value.integer = fxUnicodeLength(slot->value.string);	
}

void fx_String_fromArrayBuffer(txMachine* the)
{
	txSlot* slot;
	txSlot* arrayBuffer = C_NULL;
	txInteger length;
	txString string;
	if (mxArgc < 1)
		mxTypeError("no argument");
	slot = mxArgv(0);
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference->next;
		if (slot && (slot->kind == XS_ARRAY_BUFFER_KIND))
			arrayBuffer = slot;
	}
	if (!arrayBuffer)
		mxTypeError("argument is no ArrayBuffer instance");
	length = arrayBuffer->value.arrayBuffer.length;
	string = fxNewChunk(the, length + 1);
	c_memcpy(string, arrayBuffer->value.arrayBuffer.address, length);
	string[length] = 0;
	mxResult->value.string = string;
	mxResult->kind = XS_STRING_KIND;
}

void fx_String_fromCharCode(txMachine* the)
{
	txInteger aLength;
	txInteger aCount;
	txInteger anIndex;
	txU4 c; 
	txU1* p;
	
	aLength = 0;
	aCount = mxArgc;
	for (anIndex = 0; anIndex < aCount; anIndex++) {
		c = fxToUnsigned(the, mxArgv(anIndex));
		if (c < 0x80)
			aLength++;
		else if (c < 0x800)
			aLength += 2;
		else if (c < 0x10000)
			aLength += 3;
		else
			aLength += 4;
	}
	mxResult->value.string = (txString)fxNewChunk(the, aLength + 1);
	mxResult->kind = XS_STRING_KIND;
	p = (txU1*)mxResult->value.string;
	for (anIndex = 0; anIndex < aCount; anIndex++) {
		c = fxToUnsigned(the, mxArgv(anIndex));
		if (c < 0x80) {
			*p++ = (txU1)c;
		}
		else if (c < 0x800) {
			*p++ = (txU1)(0xC0 | (c >> 6));
			*p++ = (txU1)(0x80 | (c & 0x3F));
		}
		else if (c < 0x10000) {
			*p++ = (txU1)(0xE0 | (c >> 12));
			*p++ = (txU1)(0x80 | ((c >> 6) & 0x3F));
			*p++ = (txU1)(0x80 | (c & 0x3F));
		}
		else if (c < 0x200000) {
			*p++ = (txU1)(0xF0 | (c >> 18));
			*p++ = (txU1)(0x80 | ((c >> 12) & 0x3F));
			*p++ = (txU1)(0x80 | ((c >> 6) & 0x3F));
			*p++ = (txU1)(0x80 | (c  & 0x3F));
		}
	}	
	*p = 0;
}

void fx_String_raw(txMachine* the)
{
	mxUnknownError("TBD");
}

void fx_String_prototype_charAt(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txInteger anOffset;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	aLength = fxUnicodeLength(aString);
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND))
		anOffset = fxToInteger(the, mxArgv(0));
	else
		anOffset = -1;
	if ((0 <= anOffset) && (anOffset < aLength)) {
		anOffset = fxUnicodeToUTF8Offset(aString, anOffset);
		aLength = fxUnicodeToUTF8Offset(aString + anOffset, 1);
		if ((anOffset >= 0) && (aLength > 0)) {
			mxResult->value.string = (txString)fxNewChunk(the, aLength + 1);
			c_memcpy(mxResult->value.string, mxThis->value.string + anOffset, aLength);
			mxResult->value.string[aLength] = 0;
			mxResult->kind = XS_STRING_KIND;
		}
		else {
			mxResult->value.string = mxEmptyString.value.string;
			mxResult->kind = mxEmptyString.kind;
		}
	}
	else {
		mxResult->value.string = mxEmptyString.value.string;
		mxResult->kind = mxEmptyString.kind;
	}
}

void fx_String_prototype_charCodeAt(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txInteger anOffset;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	aLength = fxUnicodeLength(aString);
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND))
		anOffset = fxToInteger(the, mxArgv(0));
	else
		anOffset = -1;
	if ((0 <= anOffset) && (anOffset < aLength)) {
		anOffset = fxUnicodeToUTF8Offset(aString, anOffset);
		aLength = fxUnicodeToUTF8Offset(aString + anOffset, 1);
		if ((anOffset >= 0) && (aLength > 0)) {
			mxResult->value.integer = fxUnicodeCharacter(aString + anOffset);
			mxResult->kind = XS_INTEGER_KIND;
		}
		else {
			mxResult->value.number = C_NAN;
			mxResult->kind = XS_NUMBER_KIND;
		}
	}
	else {
		mxResult->value.number = C_NAN;
		mxResult->kind = XS_NUMBER_KIND;
	}
}

void fx_String_prototype_compare(txMachine* the)
{
	txString aString;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	if (mxArgc < 1)
		mxResult->value.integer = c_strcmp(aString, "undefined");
	else {
		fxToString(the, mxArgv(0));
		mxResult->value.integer = c_strcmp(aString, mxArgv(0)->value.string);
	}
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_String_prototype_concat(txMachine* the)
{
	txString aString;
	txInteger aCount;
	txInteger aLength;
	txInteger anIndex;
	
	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	aCount = mxArgc;
	aLength = c_strlen(mxThis->value.string);
	for (anIndex = 0; anIndex < aCount; anIndex++)
		aLength += c_strlen(fxToString(the, mxArgv(anIndex)));
	mxResult->value.string = (txString)fxNewChunk(the, aLength + 1);
	mxResult->kind = XS_STRING_KIND;
	c_strcpy(mxResult->value.string, mxThis->value.string);
	for (anIndex = 0; anIndex < aCount; anIndex++)
		c_strcat(mxResult->value.string, mxArgv(anIndex)->value.string);
}

void fx_String_prototype_endsWith(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txInteger length = c_strlen(string);
	txString searchString;
	txInteger searchLength;
	txInteger offset;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc < 1)
		return;
	if (fxArgToRegExp(the, 0))
		mxTypeError("future editions");
	searchString = fxToString(the, mxArgv(0));
	searchLength = c_strlen(searchString);
	offset = fxUnicodeToUTF8Offset(string, fxArgToPosition(the, 1, length, fxUnicodeLength(string)));
	if (offset < searchLength)
		return;
	if (!c_strncmp(string + offset - searchLength, searchString, searchLength))
		mxResult->value.boolean = 1;
}

void fx_String_prototype_includes(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txInteger length = c_strlen(string);
	txString searchString;
	txInteger searchLength;
	txInteger offset;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc < 1)
		return;
	if (fxArgToRegExp(the, 0))
		mxTypeError("future editions");
	searchString = fxToString(the, mxArgv(0));
	searchLength = c_strlen(searchString);
	offset = fxUnicodeToUTF8Offset(string, fxArgToPosition(the, 1, 0, fxUnicodeLength(string)));
	if ((length - offset) < searchLength)
		return;
	if (c_strstr(string + offset, searchString))
		mxResult->value.boolean = 1;
}

void fx_String_prototype_indexOf(txMachine* the)
{
	txString aString;
	txString aSubString;
	txInteger aLength;
	txInteger aSubLength;
	txInteger anOffset;
	txNumber aNumber;
	txInteger aLimit;
	txString p;
	txString q;
	
	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	if (mxArgc < 1) {
		mxResult->value.integer = -1;
		mxResult->kind = XS_INTEGER_KIND;
		return;
	}
	aSubString = fxToString(the, mxArgv(0));
	aLength = fxUnicodeLength(aString);
	aSubLength = fxUnicodeLength(aSubString);
	anOffset = 0;
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		aNumber = fxToNumber(the, mxArgv(1));
		anOffset = (c_isnan(aNumber)) ? 0 : (aNumber < 0) ? 0 : (aNumber > aLength) ? aLength : (txInteger)c_floor(aNumber);
	}
	if (anOffset + aSubLength <= aLength) {
		anOffset = fxUnicodeToUTF8Offset(aString, anOffset);
		aLimit = c_strlen(aString) - c_strlen(aSubString);
		while (anOffset <= aLimit) {
			p = aString + anOffset;
			q = aSubString;
			while (*q && (*p == *q)) {
				p++;
				q++;
			}
			if (*q)
				anOffset++;
			else
				break;
		}
		if (anOffset <= aLimit)
			anOffset = fxUTF8ToUnicodeOffset(aString, anOffset);
		else
			anOffset = -1;
	}
	else
		anOffset = -1;
	mxResult->value.integer = anOffset;
	mxResult->kind = XS_INTEGER_KIND;
}

static txInteger fx_String_prototype_indexOf_aux(txMachine* the, txString theString, 
		txInteger theLength, txInteger theOffset,
		txString theSubString, txInteger theSubLength, txInteger* theOffsets)
{
	txString p;
	txString q;
	
	theOffsets[0] = theOffset;
	theOffsets[1] = theOffset + theSubLength;
	while (theOffsets[1] <= theLength) {
		p = theString + theOffsets[0];
		q = theSubString;
		while (*q && (*p == *q)) {
			p++;
			q++;
		}
		if (*q) {
			theOffsets[0]++;
			theOffsets[1]++;
		}
		else
			return 1;
	}
	return 0;
}

void fx_String_prototype_lastIndexOf(txMachine* the)
{
	txString aString;
	txString aSubString;
	txInteger aLength;
	txInteger aSubLength;
	txInteger anOffset;
	txNumber aNumber;
	txString p;
	txString q;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	if (mxArgc < 1) {
		mxResult->value.integer = -1;
		mxResult->kind = XS_INTEGER_KIND;
		return;
	}
	aSubString = fxToString(the, mxArgv(0));
	aLength = fxUnicodeLength(aString);
	aSubLength = fxUnicodeLength(aSubString);
	anOffset = aLength;
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		aNumber = fxToNumber(the, mxArgv(1));
		anOffset = (c_isnan(aNumber)) ? aLength : (aNumber < 0) ? 0 : (aNumber > aLength) ? aLength : (txInteger)c_floor(aNumber);
		anOffset += aSubLength;
		if (anOffset > aLength)
			anOffset = aLength;
	}
	if (anOffset - aSubLength >= 0) {
		anOffset = fxUnicodeToUTF8Offset(aString, anOffset - aSubLength);
		while (anOffset >= 0) {
			p = aString + anOffset;
			q = aSubString;
			while (*q && (*p == *q)) {
				p++;
				q++;
			}
			if (*q)
				anOffset--;
			else
				break;
		}		
		anOffset = fxUTF8ToUnicodeOffset(aString, anOffset);
	}
	else
		anOffset = -1;
	mxResult->value.integer = anOffset;
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_String_prototype_match(txMachine* the)
{
	txString aString;
	txSlot* aRegExp;
	txSlot* aProperty;
	txInteger anOffset;
	txSlot* anArray;
	txSlot* anItem;
	txInteger aCount;
	txInteger aSubOffset;
	txInteger aSubLength;
	
	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	if ((mxArgc > 0) && mxIsReference(mxArgv(0))) {
		aRegExp = fxGetInstance(the, mxArgv(0));
		if (!mxIsRegExp(aRegExp))
			aRegExp = C_NULL;
	}
	else
		aRegExp = C_NULL;
	if (!aRegExp) {
		if (mxArgc > 0) {
			mxPushSlot(mxArgv(0));
			mxPushInteger(1);
		}
		else
			mxPushInteger(0);
		mxPush(mxGlobal);
		fxNewID(the, mxID(_RegExp));
		mxPullSlot(mxArgv(0));
		aRegExp = fxGetInstance(the, mxArgv(0));
	}
	fxToString(the, mxThis);
	aProperty = fxGetProperty(the, aRegExp, mxID(_global));
	if (aProperty->value.boolean) {
		anOffset = 0;
		mxPush(mxArrayPrototype);
		anArray = fxNewArrayInstance(the);
		mxPullSlot(mxResult);
		anItem = aProperty = anArray->next;
		for (;;) {
			aCount = fxExecuteRegExp(the, aRegExp, mxThis, anOffset);
			if (aCount <= 0)
				break;
			aSubOffset = aRegExp->next->value.regexp.offsets[0];
			aSubLength = aRegExp->next->value.regexp.offsets[1] - aSubOffset;
			anItem->next = fxNewSlot(the);
			anItem = anItem->next;
			anItem->next = C_NULL;
			anItem->ID = XS_NO_ID;
			anItem->flag = XS_NO_FLAG;
			anItem->value.string = (txString)fxNewChunk(the, aSubLength + 1);
			c_memcpy(anItem->value.string, mxThis->value.string + aSubOffset, aSubLength);
			anItem->value.string[aSubLength] = 0;
			anItem->kind = XS_STRING_KIND;
			aProperty->value.array.length++;
			if (anOffset == aRegExp->next->value.regexp.offsets[1])
				anOffset++;
			else
				anOffset = aRegExp->next->value.regexp.offsets[1];
		} 
		fxCacheArray(the, anArray);
	}
	else {
		mxPushSlot(mxThis);
		mxPushInteger(1);
		mxPushSlot(mxArgv(0));
		fxCallID(the, mxID(_exec));
		mxPullSlot(mxResult);
	}
}

void fx_String_prototype_normalize(txMachine* the)
{
	txString string;
	txInteger stringLength;
	txString result = mxEmptyString.value.string;
	string = fxCoerceToString(the, mxThis);
	if (!string)
		mxTypeError("this is null or undefined");
	stringLength = c_strlen(string);
	#if (mxWindows && (WINVER >= 0x0600))
	{
		NORM_FORM form;
		txInteger unicodeLength;
		txU2* unicodeBuffer = NULL;
		txInteger normLength;
		txU2* normBuffer = NULL;
		txInteger resultLength;
		mxTry(the) {
			if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND))
				form = NormalizationC;
			else {
				result = fxToString(the, mxArgv(0));
				if (!c_strcmp(result, "NFC"))
					form = NormalizationC;
				else if (!c_strcmp(result, "NFD"))
					form = NormalizationD;
				else if (!c_strcmp(result, "NFKC"))
					form = NormalizationKC;
				else if (!c_strcmp(result, "NFKD"))
					form = NormalizationKD;
				else
					mxRangeError("invalid form");
			}
			unicodeLength = MultiByteToWideChar(CP_UTF8, 0, string, stringLength, NULL, 0);
			unicodeBuffer = c_malloc(unicodeLength * 2);
			if (!unicodeBuffer) fxJump(the);
			MultiByteToWideChar(CP_UTF8, 0, string, stringLength, unicodeBuffer, unicodeLength);
			normLength = NormalizeString(form, unicodeBuffer, unicodeLength, NULL, 0);
			normBuffer = c_malloc(normLength * 2);
			if (!normBuffer) fxJump(the);
			NormalizeString(form, unicodeBuffer, unicodeLength, normBuffer, normLength);
			resultLength = WideCharToMultiByte(CP_UTF8, 0, normBuffer, normLength, NULL, 0, NULL, NULL);
			result = fxNewChunk(the, resultLength + 1);
			WideCharToMultiByte(CP_UTF8, 0, normBuffer, normLength, result, resultLength, NULL, NULL);
			result[resultLength] = 0;
			c_free(unicodeBuffer);
		}
		mxCatch(the) {
			if (unicodeBuffer)
				c_free(unicodeBuffer);
			fxJump(the);
		}
	}
	#elif (mxMacOSX || mxiOS)
	{
		CFStringNormalizationForm form;
		CFStringRef cfString = NULL;
		CFMutableStringRef mutableCFString = NULL;
		CFIndex resultLength;
		CFRange range;
		mxTry(the) {
			if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND))
				form = kCFStringNormalizationFormC;
			else {
				result = fxToString(the, mxArgv(0));
				if (!c_strcmp(result, "NFC"))
					form = kCFStringNormalizationFormC;
				else if (!c_strcmp(result, "NFD"))
					form = kCFStringNormalizationFormD;
				else if (!c_strcmp(result, "NFKC"))
					form = kCFStringNormalizationFormKC;
				else if (!c_strcmp(result, "NFKD"))
					form = kCFStringNormalizationFormKD;
				else
					mxRangeError("invalid form");
			}
			cfString = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)string, stringLength, kCFStringEncodingUTF8, false);
			if (cfString == NULL) fxJump(the);
			mutableCFString = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, cfString);
			if (mutableCFString == NULL) fxJump(the);
			CFStringNormalize(mutableCFString, form);
			range = CFRangeMake(0, CFStringGetLength(mutableCFString));
			CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, NULL, 0, &resultLength);
			result = fxNewChunk(the, resultLength + 1);
			CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, (UInt8 *)result, resultLength, NULL);
			result[resultLength] = 0;
			CFRelease(mutableCFString);
			CFRelease(cfString);
		}
		mxCatch(the) {
			if (mutableCFString)
				CFRelease(mutableCFString);
			if (cfString)
				CFRelease(cfString);
			fxJump(the);
		}
	}
	#else
	{
		mxRangeError("invalid form");
	}
	#endif
	mxResult->value.string = result;
	mxResult->kind = XS_STRING_KIND;
}

void fx_String_prototype_repeat(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis), result;
	txInteger length = c_strlen(string);
	txInteger count = fxArgToInteger(the, 0, 0);
	if (count < 0)
		mxRangeError("count < 0");
	result = mxResult->value.string = (txString)fxNewChunk(the, (length * count) + 1);
	mxResult->kind = XS_STRING_KIND;
	while (count) {
		c_memcpy(result, string, length);
		count--;
		result += length;
	}
	*result = 0;
	string = mxThis->value.string;
}

void fx_String_prototype_replace(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txInteger aDelta;
	txSlot* aRegExp;
	txSlot* aProperty;
	txInteger anOffset;
	txInteger aCount;
	txSlot aSubSlot;
	txInteger aSubLength;
	txInteger subOffsets[2];
	txString p;
	txString q;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	if (mxArgc < 2)
		mxSyntaxError("no replaceValue parameter");
	if (mxArgc < 1)
		mxSyntaxError("no searchValue parameter");
	aLength = c_strlen(aString);
	*mxResult = *mxThis;
	if (mxIsReference(mxArgv(0))) {
		aRegExp = fxGetInstance(the, mxArgv(0));
		if (!mxIsRegExp(aRegExp))
			aRegExp = C_NULL;
	}
	else
		aRegExp = C_NULL;
	if (aRegExp) {
		aProperty = fxGetProperty(the, aRegExp, mxID(_global));
		if (aProperty->value.boolean) {
			aDelta = 0;
			anOffset = 0;
			for (;;) {
				aCount = fxExecuteRegExp(the, aRegExp, mxThis, anOffset);
				if (aCount <= 0)
					break;
				aDelta = fx_String_prototype_replace_aux(the, aDelta, aLength, aCount, aRegExp->next);
				if (anOffset == aRegExp->next->value.regexp.offsets[1])
					anOffset++;
				else
					anOffset = aRegExp->next->value.regexp.offsets[1];
			}
		}
		else {
			aCount = fxExecuteRegExp(the, aRegExp, mxThis, 0);
			if (aCount > 0)
				fx_String_prototype_replace_aux(the, 0, aLength, aCount, aRegExp->next);
		}
	}
	else {
		/* fake regexp slot! */
		aSubSlot.next = C_NULL; 
		aSubSlot.flag = XS_NO_FLAG; 
		aSubSlot.kind = XS_UNDEFINED_KIND; 
		aSubSlot.ID = XS_NO_ID;
		aSubSlot.value.regexp.code = C_NULL; 
		aSubSlot.value.regexp.offsets = subOffsets; 
		aSubLength = c_strlen(fxToString(the, mxArgv(0)));
		aDelta = 0;
		subOffsets[0] = 0;
		subOffsets[1] = aSubLength;
		while (subOffsets[1] <= aLength) {
			p = mxThis->value.string + subOffsets[0];
			q = mxArgv(0)->value.string;
			while (*q && (*p == *q)) {
				p++;
				q++;
			}
			if (*q) {
				subOffsets[0]++;
				subOffsets[1]++;
			}
			else {
				aDelta = fx_String_prototype_replace_aux(the, aDelta, aLength, 1, &aSubSlot);
				if (!aSubLength)
					break;
				subOffsets[0] += aSubLength;
				subOffsets[1] += aSubLength;
			}
		}
	}
}
	
txInteger fx_String_prototype_replace_aux(txMachine* the, txInteger theDelta, txInteger theLength, 
		txInteger theCount, txSlot* theSlot)
{
	txSlot* aFunction;
	txByte c;
	txInteger i;
	txInteger l;
	txString p;
	txString q;
	txString r;
	txString s;
	txInteger aDelta;
	
	if (mxIsReference(mxArgv(1))) {
		aFunction = fxGetInstance(the, mxArgv(1));
		if (!mxIsFunction(aFunction))
			aFunction = C_NULL;
	}
	else
		aFunction = C_NULL;
	if (aFunction) {
	
		mxPushUndefined();
		l = theSlot->value.regexp.offsets[1] - theSlot->value.regexp.offsets[0];
		the->stack->value.string = (txString)fxNewChunk(the, l + 1);
		c_memcpy(the->stack->value.string, mxThis->value.string + theSlot->value.regexp.offsets[0], l);
		the->stack->value.string[l] = 0;
		the->stack->kind = XS_STRING_KIND;
		
		for (i = 1; i < theCount; i++) {
			mxPushUndefined();
			l = theSlot->value.regexp.offsets[(2 * i) + 1] - theSlot->value.regexp.offsets[2 * i];
			the->stack->value.string = (txString)fxNewChunk(the, l + 1);
			c_memcpy(the->stack->value.string, mxThis->value.string + theSlot->value.regexp.offsets[2 * i], l);
			the->stack->value.string[l] = 0;
			the->stack->kind = XS_STRING_KIND;
		}
				
		mxPushInteger(theSlot->value.regexp.offsets[0]);
		mxPushString(mxThis->value.string);
		mxCall(mxArgv(1), mxThis, theCount + 2);
	}
	else
		mxPushSlot(mxArgv(1));

	r = fxToString(the, the->stack);
	
	l = theDelta + theSlot->value.regexp.offsets[0];
	while ((c = *r++)) {
		if ((c == '$') && !aFunction) {
			c = *r++;
			switch (c) {
			case '$':
				l++;
				break;
			case '&':
				l += theSlot->value.regexp.offsets[1] - theSlot->value.regexp.offsets[0];
				break;
			case '`':
				l += theSlot->value.regexp.offsets[0];
				break;
			case '\'':
				l += theLength - theSlot->value.regexp.offsets[1];
				break;
			default:
				if (('0' <= c) && (c <= '9')) {
					i = c - '0';
					c = *r;
					if (('0' <= c) && (c <= '9')) {
						i = (i * 10) + c - '0';
						r++;
					}
					if ((0 < i) && (i < theCount))
						l += theSlot->value.regexp.offsets[(2 * i) + 1] - theSlot->value.regexp.offsets[2 * i];
				}
				else {
					l++;
					if (c)
						l++;
				}
				//else
				//	mxSyntaxError("String.replace: invalid replaceValue");
				break;
			}
			if (!c)
				break;
		}
		else
			l++;
	}
	l += theLength - theSlot->value.regexp.offsets[1];
	aDelta = l - theLength;
	
	p = (txString)fxNewChunk(the, l + 1);
	q = mxResult->value.string;
	r = the->stack->value.string;
	s = mxThis->value.string;
	mxResult->value.string = p;
	
	l = theDelta + theSlot->value.regexp.offsets[0];
	c_memcpy(p, q, l);
	p += l;
	while ((c = *r++)) {
		if ((c == '$') && !aFunction) {
			c = *r++;
			switch (c) {
			case '$':
				*p++ = c;
				break;
			case '&':
				l = theSlot->value.regexp.offsets[1] - theSlot->value.regexp.offsets[0];
				c_memcpy(p, s + theSlot->value.regexp.offsets[0], l);
				p += l;
				break;
			case '`':
				l = theSlot->value.regexp.offsets[0];
				c_memcpy(p, s, l);
				p += l;
				break;
			case '\'':
				l = theLength - theSlot->value.regexp.offsets[1];
				c_memcpy(p, s + theSlot->value.regexp.offsets[1] , l);
				p += l;
				break;
			default:
				if (('0' <= c) && (c <= '9')) {
					i = c - '0';
					c = *r;
					if (('0' <= c) && (c <= '9')) {
						i = (i * 10) + c - '0';
						r++;
					}
					if ((0 < i) && (i < theCount)) {
						l = theSlot->value.regexp.offsets[(2 * i) + 1] - theSlot->value.regexp.offsets[2 * i];
						c_memcpy(p, s + theSlot->value.regexp.offsets[2 * i], l);
						p += l;
					}
				}
				else {
					*p++ = '$';
					if (c)
						*p++ = c;
				}
				break;
			}
			if (!c)
				break;
		}
		else
			*p++ = c;
	}
	l = theLength - theSlot->value.regexp.offsets[1];
	c_memcpy(p, s + theSlot->value.regexp.offsets[1], l);
	p += l;
	*p = 0;
		
	the->stack++;
	return aDelta;
}

void fx_String_prototype_search(txMachine* the)
{
	txString aString;
	txSlot* aRegExp;
	txInteger aCount;
	txInteger anOffset = -1;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	if ((mxArgc > 0) && mxIsReference(mxArgv(0))) {
		aRegExp = fxGetInstance(the, mxArgv(0));
		if (!mxIsRegExp(aRegExp))
			aRegExp = C_NULL;
	}
	else
		aRegExp = C_NULL;
	if (!aRegExp) {
		if (mxArgc > 0) {
			mxPushSlot(mxArgv(0));
			mxPushInteger(1);
		}
		else
			mxPushInteger(0);
		mxPush(mxGlobal);
		fxNewID(the, mxID(_RegExp));
		mxPullSlot(mxArgv(0));
		aRegExp = fxGetInstance(the, mxArgv(0));
	}
	fxToString(the, mxThis);
	aCount = fxExecuteRegExp(the, aRegExp, mxThis, 0);
	if (aCount > 0)
		anOffset = fxUTF8ToUnicodeOffset(mxThis->value.string, aRegExp->next->value.regexp.offsets[0]);
	the->stack++;
	mxResult->value.integer = anOffset;
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_String_prototype_slice(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txInteger aStart;
	txInteger aStop;
	txInteger anOffset;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	aLength = fxUnicodeLength(aString);
	aStart = 0;
	aStop = aLength;
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
		aStart = fxToInteger(the, mxArgv(0));
		if (aStart < 0) {
			aStart = aLength + aStart;
			if (aStart < 0)
				aStart = 0;
		}
		else if (aStart > aLength)
			aStart = aLength;
	}
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		aStop = fxToInteger(the, mxArgv(1));
		if (aStop < 0) {
			aStop = aLength + aStop;
			if (aStop < 0)
				aStop = 0;
		}
		else if (aStop > aLength)
			aStop = aLength;
	}
	if (aStart < aStop) {
		anOffset = fxUnicodeToUTF8Offset(aString, aStart);
		aLength = fxUnicodeToUTF8Offset(aString + anOffset, aStop - aStart);
		if ((anOffset >= 0) && (aLength > 0)) {
			mxResult->value.string = (txString)fxNewChunk(the, aLength + 1);
			c_memcpy(mxResult->value.string, mxThis->value.string + anOffset, aLength);
			mxResult->value.string[aLength] = 0;
			mxResult->kind = XS_STRING_KIND;
		}
		else {
			mxResult->value.string = mxEmptyString.value.string;
			mxResult->kind = mxEmptyString.kind;
		}
	}
	else {
		mxResult->value.string = mxEmptyString.value.string;
		mxResult->kind = mxEmptyString.kind;
	}
}

void fx_String_prototype_split(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txInteger aLimit;
	txSlot* anArray;
	txSlot* anItem;
	txSlot* aRegExp;
	txInteger anOffset;
	txInteger aCount;
	txInteger anIndex;
	txInteger aSubLength;
	txInteger aSubOffset;
	txInteger subOffsets[2];

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	aLength = c_strlen(fxToString(the, mxThis));

	aLimit = fxArgToArrayLimit(the, 1);
		
	mxPush(mxArrayPrototype);
	anArray = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	fxGetInstance(the, mxResult);
	if (!aLimit)
		goto bail;

	anItem = anArray->next;
	
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND)) {
		fx_String_prototype_split_aux(the, anArray, anItem, 0, aLength);
		goto bail;
	}
		
	if (mxIsReference(mxArgv(0))) {
		aRegExp = fxGetInstance(the, mxArgv(0));
		if (!mxIsRegExp(aRegExp))
			aRegExp = C_NULL;
	}
	else
		aRegExp = C_NULL;

	if (aRegExp) {
		anOffset = 0;
		for (aSubOffset = 0; aSubOffset < aLength;) {
			aCount = fxExecuteRegExp(the, aRegExp, mxThis, aSubOffset);
			if (aCount <= 0) 
				break;
			if (aRegExp->next->value.regexp.offsets[0] == aRegExp->next->value.regexp.offsets[1]) {
				if (anOffset < aRegExp->next->value.regexp.offsets[0]) {
					anItem = fx_String_prototype_split_aux(the, anArray, anItem, anOffset, aRegExp->next->value.regexp.offsets[0]);
					if (anArray->next->value.array.length >= aLimit)
						goto bail;
				}
			}
			else {
				if (anOffset <= aRegExp->next->value.regexp.offsets[0]) {
					anItem = fx_String_prototype_split_aux(the, anArray, anItem, anOffset, aRegExp->next->value.regexp.offsets[0]);
					if (anArray->next->value.array.length >= aLimit)
						goto bail;
				}
			}
			anOffset = aRegExp->next->value.regexp.offsets[1];
			for (anIndex = 1; anIndex < aCount; anIndex++) {
				anItem = fx_String_prototype_split_aux(the, anArray, anItem, aRegExp->next->value.regexp.offsets[2 * anIndex], aRegExp->next->value.regexp.offsets[(2 * anIndex) + 1]);
				if (anArray->next->value.array.length >= aLimit)
					goto bail;
			}
			if (aSubOffset == aRegExp->next->value.regexp.offsets[1])
				aSubOffset++;
			else
				aSubOffset = aRegExp->next->value.regexp.offsets[1];
		}
		if (anOffset <= aLength)
			fx_String_prototype_split_aux(the, anArray, anItem, anOffset, aLength);
	}
	else {
		aSubLength = c_strlen(fxToString(the, mxArgv(0)));
		if (aSubLength == 0) {
			anOffset = 0;
			while (anOffset < aLength) {
				aSubOffset = anOffset + fxUnicodeToUTF8Offset(mxThis->value.string + anOffset, 1);
				anItem = fx_String_prototype_split_aux(the, anArray, anItem, anOffset, aSubOffset);
				if (anArray->next->value.array.length >= aLimit)
					goto bail;
				anOffset = aSubOffset;
			}
		}
		else if (aLength == 0) {
			fx_String_prototype_split_aux(the, anArray, anItem, 0, 0);
		}
		else {
			anOffset = 0;
			for (;;) {
				aCount = fx_String_prototype_indexOf_aux(the, mxThis->value.string, aLength, anOffset, mxArgv(0)->value.string, aSubLength, subOffsets);
				if (aCount <= 0)
					break;
				if (anOffset <= subOffsets[0]) {
					anItem = fx_String_prototype_split_aux(the, anArray, anItem, anOffset, subOffsets[0]);
					if (anArray->next->value.array.length >= aLimit)
						goto bail;
				}
				anOffset = subOffsets[1];
			}
			if (anOffset <= aLength)
				fx_String_prototype_split_aux(the, anArray, anItem, anOffset, aLength);
		}
	}
bail:
	fxCacheArray(the, anArray);
}

txSlot* fx_String_prototype_split_aux(txMachine* the, txSlot* theArray, txSlot* theItem, txInteger theStart, txInteger theStop)
{
	theStop -= theStart;
	theItem->next = fxNewSlot(the);
	theItem = theItem->next;
	theItem->next = C_NULL;
	theItem->ID = XS_NO_ID;
	theItem->flag = XS_NO_FLAG;
	if (theStart >= 0) {
		theItem->value.string = (txString)fxNewChunk(the, theStop + 1);
		c_memcpy(theItem->value.string, mxThis->value.string + theStart, theStop);
		theItem->value.string[theStop] = 0;
		theItem->kind = XS_STRING_KIND;
	}
	theArray->next->value.array.length++;
	return theItem;
}

void fx_String_prototype_startsWith(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txInteger length = c_strlen(string);
	txString searchString;
	txInteger searchLength;
	txInteger offset;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc < 1)
		return;
	if (fxArgToRegExp(the, 0))
		mxTypeError("future editions");
	searchString = fxToString(the, mxArgv(0));
	searchLength = c_strlen(searchString);
	offset = fxUnicodeToUTF8Offset(string, fxArgToPosition(the, 1, 0, fxUnicodeLength(string)));
	if (length - offset < searchLength)
		return;
	if (!c_strncmp(string + offset, searchString, searchLength))
		mxResult->value.boolean = 1;
}

void fx_String_prototype_substr(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txInteger length = c_strlen(string);
	txInteger size = fxUnicodeLength(string);
	txInteger start = fxArgToIndex(the, 0, 0, size);
	txInteger stop = (mxArgc > 1) ? start + fxToInteger(the, mxArgv(1)) : size;
	if (start < stop) {
		start = fxUnicodeToUTF8Offset(string, start);
		stop = fxUnicodeToUTF8Offset(string, stop);
		length = stop - start;
		mxResult->value.string = (txString)fxNewChunk(the, length + 1);
		c_memcpy(mxResult->value.string, string + start, length);
		mxResult->value.string[length] = 0;
		mxResult->kind = XS_STRING_KIND;
	}
	else {
		mxResult->value.string = mxEmptyString.value.string;
		mxResult->kind = mxEmptyString.kind;
	}
		
}

void fx_String_prototype_substring(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txNumber aNumber;
	txInteger aStart;
	txInteger aStop;
	txInteger anOffset;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	aLength = fxUnicodeLength(aString);
	aStart = 0;
	aStop = aLength;
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
		aNumber = fxToNumber(the, mxArgv(0));
		aStart = (c_isnan(aNumber)) ? 0 : (aNumber < 0) ? 0 : (aNumber > aLength) ? aLength : (txInteger)c_floor(aNumber);
	}
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		aNumber = fxToNumber(the, mxArgv(1));
		aStop = (c_isnan(aNumber)) ? 0 : (aNumber < 0) ? 0 : (aNumber > aLength) ? aLength : (txInteger)c_floor(aNumber);
	}
	if (aStart > aStop) {
		aLength = aStart;
		aStart = aStop;
		aStop = aLength;
	}
	if (aStart < aStop) {
		anOffset = fxUnicodeToUTF8Offset(aString, aStart);
		aLength = fxUnicodeToUTF8Offset(aString + anOffset, aStop - aStart);
		if ((anOffset >= 0) && (aLength > 0)) {
			mxResult->value.string = (txString)fxNewChunk(the, aLength + 1);
			c_memcpy(mxResult->value.string, mxThis->value.string + anOffset, aLength);
			mxResult->value.string[aLength] = 0;
			mxResult->kind = XS_STRING_KIND;
		}
		else {
			mxResult->value.string = mxEmptyString.value.string;
			mxResult->kind = mxEmptyString.kind;
		}
	}
	else {
		mxResult->value.string = mxEmptyString.value.string;
		mxResult->kind = mxEmptyString.kind;
	}
}

void fx_String_prototype_toCase(txMachine* the, txBoolean flag)
{
	txString string;
	txInteger stringLength;
	txString result;
	string = fxCoerceToString(the, mxThis);
	if (!string)
		mxTypeError("this is null or undefined");
	stringLength = c_strlen(string);
	if (stringLength) {
	#if mxWindows
		txInteger unicodeLength;
		txU2* unicodeBuffer = NULL;
		txInteger resultLength;
		mxTry(the) {
			unicodeLength = MultiByteToWideChar(CP_UTF8, 0, string, stringLength, NULL, 0);
			mxElseError(unicodeLength);
			unicodeBuffer = c_malloc(unicodeLength * 2);
			MultiByteToWideChar(CP_UTF8, 0, string, stringLength, unicodeBuffer, unicodeLength);
			if (flag)
				CharUpperBuffW(unicodeBuffer, unicodeLength);
			else
				CharLowerBuffW(unicodeBuffer, unicodeLength);
			resultLength = WideCharToMultiByte(CP_UTF8, 0, unicodeBuffer, unicodeLength, NULL, 0, NULL, NULL);
			result = fxNewChunk(the, resultLength + 1);
			WideCharToMultiByte(CP_UTF8, 0, unicodeBuffer, unicodeLength, result, resultLength, NULL, NULL);
			result[resultLength] = 0;
			c_free(unicodeBuffer);
		}
		mxCatch(the) {
			if (unicodeBuffer)
				c_free(unicodeBuffer);
			fxJump(the);
		}
	#elif (mxMacOSX || mxiOS)
		CFStringRef cfString = NULL;
		CFMutableStringRef mutableCFString = NULL;
		CFIndex resultLength;
		CFRange range;
		mxTry(the) {
			cfString = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)string, stringLength, kCFStringEncodingUTF8, false);
			if (cfString == NULL) fxJump(the);
			mutableCFString = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, cfString);
			if (mutableCFString == NULL) fxJump(the);
			if (flag)
				CFStringUppercase(mutableCFString, 0);
			else
				CFStringLowercase(mutableCFString, 0);
			range = CFRangeMake(0, CFStringGetLength(mutableCFString));
			CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, NULL, 0, &resultLength);
			result = fxNewChunk(the, resultLength + 1);
			CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, (UInt8 *)result, resultLength, NULL);
			result[resultLength] = 0;
			CFRelease(mutableCFString);
			CFRelease(cfString);
		}
		mxCatch(the) {
			if (mutableCFString)
				CFRelease(mutableCFString);
			if (cfString)
				CFRelease(cfString);
			fxJump(the);
		}
	#else
		/* compressed from http://www-01.ibm.com/support/knowledgecenter/#!/ssw_ibm_i_71/nls/rbagslowtoupmaptable.htm */
		typedef struct {
			txU2 from; txU2 to; txS2 delta;
		} txCase;
		static txCase gxToLower[84] = {
			{0x0041,0x005A,32},{0x00C0,0x00D6,32},{0x00D8,0x00DE,32},{0x0100,0x0136,0},{0x0139,0x0147,0},{0x014A,0x0176,0},{0x0178,0x0178,-121},
			{0x0179,0x017D,0},{0x0181,0x0181,210},{0x0182,0x0184,0},{0x0186,0x0186,206},{0x0187,0x0187,0},{0x018A,0x018A,205},{0x018B,0x018B,0},
			{0x018E,0x018F,202},{0x0190,0x0190,203},{0x0191,0x0191,0},{0x0193,0x0193,205},{0x0194,0x0194,207},{0x0196,0x0196,211},{0x0197,0x0197,209},
			{0x0198,0x0198,0},{0x019C,0x019C,211},{0x019D,0x019D,213},{0x019F,0x019F,214},{0x01A0,0x01A4,0},{0x01A7,0x01A7,0},{0x01A9,0x01A9,218},
			{0x01AC,0x01AC,0},{0x01AE,0x01AE,218},{0x01AF,0x01AF,0},{0x01B1,0x01B2,217},{0x01B3,0x01B5,0},{0x01B7,0x01B7,219},{0x01B8,0x01B8,0},
			{0x01BC,0x01BC,0},{0x01C4,0x01C4,2},{0x01C7,0x01C7,2},{0x01CA,0x01CA,2},{0x01CD,0x01DB,0},{0x01DE,0x01EE,0},{0x01F1,0x01F1,2},
			{0x01F4,0x01F4,0},{0x01FA,0x0216,0},{0x0386,0x0386,38},{0x0388,0x038A,37},{0x038C,0x038C,64},{0x038E,0x038F,63},{0x0391,0x03A1,32},
			{0x03A3,0x03AB,32},{0x03E2,0x03EE,0},{0x0401,0x040C,80},{0x040E,0x040F,80},{0x0410,0x042F,32},{0x0460,0x0480,0},{0x0490,0x04BE,0},
			{0x04C1,0x04C3,0},{0x04C7,0x04C7,0},{0x04CB,0x04CB,0},{0x04D0,0x04EA,0},{0x04EE,0x04F4,0},{0x04F8,0x04F8,0},{0x0531,0x0556,48},
			{0x10A0,0x10C5,48},{0x1E00,0x1E94,0},{0x1EA0,0x1EF8,0},{0x1F08,0x1F0F,-8},{0x1F18,0x1F1D,-8},{0x1F28,0x1F2F,-8},{0x1F38,0x1F3F,-8},
			{0x1F48,0x1F4D,-8},{0x1F59,0x1F59,-8},{0x1F5B,0x1F5B,-8},{0x1F5D,0x1F5D,-8},{0x1F5F,0x1F5F,-8},{0x1F68,0x1F6F,-8},{0x1F88,0x1F8F,-8},
			{0x1F98,0x1F9F,-8},{0x1FA8,0x1FAF,-8},{0x1FB8,0x1FB9,-8},{0x1FD8,0x1FD9,-8},{0x1FE8,0x1FE9,-8},{0x24B6,0x24CF,26},{0xFF21,0xFF3A,32}
		};
		static txCase gxToUpper[84] = {
			{0x0061,0x007A,-32},{0x00E0,0x00F6,-32},{0x00F8,0x00FE,-32},{0x00FF,0x00FF,121},{0x0101,0x0137,0},{0x013A,0x0148,0},{0x014B,0x0177,0},
			{0x017A,0x017E,0},{0x0183,0x0185,0},{0x0188,0x0188,0},{0x018C,0x018C,0},{0x0192,0x0192,0},{0x0199,0x0199,0},{0x01A1,0x01A5,0},
			{0x01A8,0x01A8,0},{0x01AD,0x01AD,0},{0x01B0,0x01B0,0},{0x01B4,0x01B6,0},{0x01B9,0x01B9,0},{0x01BD,0x01BD,0},{0x01C6,0x01C6,-2},
			{0x01C9,0x01C9,-2},{0x01CC,0x01CC,-2},{0x01CE,0x01DC,0},{0x01DF,0x01EF,0},{0x01F3,0x01F3,-2},{0x01F5,0x01F5,0},{0x01FB,0x0217,0},
			{0x0253,0x0253,-210},{0x0254,0x0254,-206},{0x0257,0x0257,-205},{0x0258,0x0259,-202},{0x025B,0x025B,-203},{0x0260,0x0260,-205},{0x0263,0x0263,-207},
			{0x0268,0x0268,-209},{0x0269,0x0269,-211},{0x026F,0x026F,-211},{0x0272,0x0272,-213},{0x0275,0x0275,-214},{0x0283,0x0283,-218},{0x0288,0x0288,-218},
			{0x028A,0x028B,-217},{0x0292,0x0292,-219},{0x03AC,0x03AC,-38},{0x03AD,0x03AF,-37},{0x03B1,0x03C1,-32},{0x03C3,0x03CB,-32},{0x03CC,0x03CC,-64},
			{0x03CD,0x03CE,-63},{0x03E3,0x03EF,0},{0x0430,0x044F,-32},{0x0451,0x045C,-80},{0x045E,0x045F,-80},{0x0461,0x0481,0},{0x0491,0x04BF,0},
			{0x04C2,0x04C4,0},{0x04C8,0x04C8,0},{0x04CC,0x04CC,0},{0x04D1,0x04EB,0},{0x04EF,0x04F5,0},{0x04F9,0x04F9,0},{0x0561,0x0586,-48},
			{0x10D0,0x10F5,-48},{0x1E01,0x1E95,0},{0x1EA1,0x1EF9,0},{0x1F00,0x1F07,8},{0x1F10,0x1F15,8},{0x1F20,0x1F27,8},{0x1F30,0x1F37,8},
			{0x1F40,0x1F45,8},{0x1F51,0x1F51,8},{0x1F53,0x1F53,8},{0x1F55,0x1F55,8},{0x1F57,0x1F57,8},{0x1F60,0x1F67,8},{0x1F80,0x1F87,8},
			{0x1F90,0x1F97,8},{0x1FA0,0x1FA7,8},{0x1FB0,0x1FB1,8},{0x1FD0,0x1FD1,8},{0x1FE0,0x1FE1,8},{0x24D0,0x24E9,-26},{0xFF41,0xFF5A,-32}
		};
		txU1 *s, *r;
		txU4 c;
		const txUTF8Sequence *aSequence;
		txInteger aSize;
		txCase* current;
		txCase* limit = flag ? &gxToUpper[84] : &gxToLower[84];
		result = fxNewChunk(the, stringLength + 1);
		s = (txU1*)string;
		r = (txU1*)result;
		while ((c = *s++)) {
			for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
				if ((c & aSequence->cmask) == aSequence->cval)
					break;
			}
			aSize = aSequence->size - 1;
			while (aSize > 0) {
				aSize--;
				c = (c << 6) | (*s++ & 0x3F);
			}
			c &= aSequence->lmask;
			current = flag ? gxToUpper : gxToLower;
			while (current < limit) {
				if (c < current->from)
					break;
				if (c <= current->to) {
					if (current->delta)
						c += current->delta;
					else if (flag)
						c &= ~1;
					else
						c |= 1;
					break;
				}
				current++;
			}
			if (c < 0x80) {
				*r++ = (txU1)c;
			}
			else if (c < 0x800) {
				*r++ = (txU1)(0xC0 | (c >> 6));
				*r++ = (txU1)(0x80 | (c & 0x3F));
			}
			else if (c < 0x10000) {
				*r++ = (txU1)(0xE0 | (c >> 12));
				*r++ = (txU1)(0x80 | ((c >> 6) & 0x3F));
				*r++ = (txU1)(0x80 | (c & 0x3F));
			}
			else if (c < 0x200000) {
				*r++ = (txU1)(0xF0 | (c >> 18));
				*r++ = (txU1)(0x80 | ((c >> 12) & 0x3F));
				*r++ = (txU1)(0x80 | ((c >> 6) & 0x3F));
				*r++ = (txU1)(0x80 | (c  & 0x3F));
			}
		}
		*r = 0;
	#endif
	}
	else
		result = mxEmptyString.value.string;
	mxResult->value.string = result;
	mxResult->kind = XS_STRING_KIND;
}

void fx_String_prototype_toLowerCase(txMachine* the)
{
	fx_String_prototype_toCase(the, 0);
}

void fx_String_prototype_toUpperCase(txMachine* the)
{
	fx_String_prototype_toCase(the, 1);
}

void fx_String_prototype_trim(txMachine* the)
{
	txBoolean aFlag = 1;
	txInteger aStart = 0;
	txInteger aStop = 0;
	txInteger aLength = 0;
	txU1* aString;
	txU4 aResult;
	const txUTF8Sequence *aSequence;
	txInteger aSize;

	aString = (txU1*)fxCoerceToString(the, mxThis);
	if (!aString)
		mxTypeError("this is null or undefined");
	while ((aResult = *aString++)) {
		for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
			if ((aResult & aSequence->cmask) == aSequence->cval)
				break;
		}
		aSize = aSequence->size - 1;
		while (aSize > 0) {
			aSize--;
			aResult = (aResult << 6) | (*aString++ & 0x3F);
		}
		aResult &= aSequence->lmask;
		aLength += aSequence->size;
		if ((aResult == 0x00000009)
		 || (aResult == 0x0000000A)
		 || (aResult == 0x0000000B)
		 || (aResult == 0x0000000C)
		 || (aResult == 0x0000000D)
		 || (aResult == 0x00000020)
		 || (aResult == 0x000000A0)
		 || (aResult == 0x00002028)
		 || (aResult == 0x00002029)
		 || (aResult == 0x0000FEFF))
			aStop += aSequence->size;
		else {
			if (aFlag) {
				aFlag = 0;
				aStart = aStop;
			}
			aStop = 0;
		}
	}
//	if ((0 == aStart) && (0 == aStop))
//		*mxResult = *mxThis;
//	else
	{
		aLength -= aStart + aStop;
		mxResult->value.string = (txString)fxNewChunk(the, aLength + 1);
		c_memcpy(mxResult->value.string, mxThis->value.string + aStart, aLength);
		mxResult->value.string[aLength] = 0;
		mxResult->kind = XS_STRING_KIND;
	}
}

void fx_String_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckString(the, mxThis);
	if (!slot) mxTypeError("this is no string");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

txSlot* fxCheckString(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if ((it->kind == XS_STRING_KIND) || (it->kind == XS_STRING_X_KIND))
		result = it;
	else if (it->kind == XS_REFERENCE_KIND) {
		it = it->value.reference;
		if (it->flag & XS_VALUE_FLAG) {
			it = it->next;
			if ((it->kind == XS_STRING_KIND) || (it->kind == XS_STRING_X_KIND))
				result = it;
		}
	}
	return result;
}

txString fxCoerceToString(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_UNDEFINED_KIND)
		return C_NULL;
	if (theSlot->kind == XS_NULL_KIND)
		return C_NULL;
	return fxToString(the, theSlot);
}

void fx_String_prototype_iterator(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txSlot* property;
	mxPush(mxStringIteratorPrototype);
	property = fxLastProperty(the, fxNewIteratorInstance(the, mxThis));
	property = fxNextIntegerProperty(the, property, fxUnicodeLength(string), mxID(_length), XS_GET_ONLY);
	mxPullSlot(mxResult);
}

void fx_String_prototype_iterator_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* length = index->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	if (index->value.integer < length->value.integer) {
		txInteger offset = fxUnicodeToUTF8Offset(iterable->value.string, index->value.integer);
		txInteger length = fxUnicodeToUTF8Offset(iterable->value.string + offset, 1);
		value->value.string = (txString)fxNewChunk(the, length + 1);
		c_memcpy(value->value.string, iterable->value.string + offset, length);
		value->value.string[length] = 0;
		value->kind = XS_STRING_KIND;
		index->value.integer++;
	}
	else {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

txInteger fxArgToPosition(txMachine* the, txInteger i, txInteger index, txInteger length)
{
	if ((mxArgc > i) && (mxArgv(i)->kind != XS_UNDEFINED_KIND)) {
		index = fxToInteger(the, mxArgv(i));
		if (index < 0) {
			index = 0;
		}
		else if (index > length)
			index = length;
	}
	return index;
}

txSlot* fxArgToRegExp(txMachine* the, txInteger i)
{
	if (mxArgc > i) {
		txSlot* slot = mxArgv(i);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			if (slot->next && (slot->next->kind == XS_REGEXP_KIND))
				return slot;
		}
	}
	return C_NULL;
}


