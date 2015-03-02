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

static void fx_String(txMachine* the);
static void fx_String_charAt(txMachine* the);
static void fx_String_charCodeAt(txMachine* the);
static void fx_String_compare(txMachine* the);
static void fx_String_concat(txMachine* the);
static void fx_String_fromCharCode(txMachine* the);
static void fx_String_indexOf(txMachine* the);
static void fx_String_lastIndexOf(txMachine* the);
static void fx_String_match(txMachine* the);
static void fx_String_replace(txMachine* the);
static txInteger fx_String_replace_aux(txMachine* the, txInteger theDelta, txInteger theLength, 
		txInteger theCount, txSlot* theOffsets);
static void fx_String_search(txMachine* the);
static void fx_String_slice(txMachine* the);
static void fx_String_split(txMachine* the);
static txSlot* fx_String_split_aux(txMachine* the, txSlot* theArray, txSlot* theItem, 
		txInteger theStart, txInteger theStop);
static void fx_String_substring(txMachine* the);
static void fx_String_toLowerCase(txMachine* the);
static void fx_String_toUpperCase(txMachine* the);
static void fx_String_trim(txMachine* the);
static void fx_String_valueOf(txMachine* the);

static txString fxCoerceToString(txMachine* the, txSlot* theSlot);

void fxBuildString(txMachine* the)
{
	mxPush(mxGlobal);
	
	mxPush(mxObjectPrototype);
	fxNewStringInstance(the);
	fxNewHostFunction(the, fx_String_charAt, 1);
	fxQueueID(the, fxID(the, "charAt"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_charCodeAt, 1);
	fxQueueID(the, fxID(the, "charCodeAt"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_compare, 1);
	fxQueueID(the, fxID(the, "compare"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_concat, 1);
	fxQueueID(the, fxID(the, "concat"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_indexOf, 1);
	fxQueueID(the, fxID(the, "indexOf"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_lastIndexOf, 1);
	fxQueueID(the, fxID(the, "lastIndexOf"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_compare, 1);
	fxQueueID(the, fxID(the, "localeCompare"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_match, 1);
	fxQueueID(the, fxID(the, "match"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_replace, 2);
	fxQueueID(the, fxID(the, "replace"), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_String_search, 1);
	fxQueueID(the, fxID(the, "search"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_slice, 2);
	fxQueueID(the, fxID(the, "slice"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_split, 2);
	fxQueueID(the, fxID(the, "split"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_substring, 2);
	fxQueueID(the, fxID(the, "substring"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_toLowerCase, 0);
	fxQueueID(the, fxID(the, "toLocaleLowerCase"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_valueOf, 0);
	fxQueueID(the, fxID(the, "toLocaleString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_toUpperCase, 0);
	fxQueueID(the, fxID(the, "toLocaleUpperCase"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_toLowerCase, 0);
	fxQueueID(the, fxID(the, "toLowerCase"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_valueOf, 0);
	fxQueueID(the, fxID(the, "toString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_toUpperCase, 0);
	fxQueueID(the, fxID(the, "toUpperCase"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_trim, 0);
	fxQueueID(the, fxID(the, "trim"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_String_valueOf, 0);
	fxQueueID(the, fxID(the, "valueOf"), XS_DONT_ENUM_FLAG);
	
	fxAliasInstance(the, the->stack);
	mxStringPrototype = *the->stack;
	fxNewHostConstructor(the, fx_String, 1);
	fxNewHostFunction(the, fx_String_fromCharCode, 1);
	fxQueueID(the, fxID(the, "fromCharCode"), XS_DONT_ENUM_FLAG);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxStringPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "String"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	the->stack++;
}

txSlot* fxNewStringInstance(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;

	anInstance = fxNewSlot(the);
	anInstance->next = C_NULL;
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
	aProperty->next = C_NULL;
	aProperty->ID = XS_NO_ID;
	aProperty->flag = XS_NO_FLAG;
	aProperty->kind = mxEmptyString.kind;
	aProperty->value.string = mxEmptyString.value.string;

	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->next = C_NULL;
	aProperty->ID = the->lengthID;
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_INTEGER_KIND;
	aProperty->value.integer = 0;
	
	return anInstance;
}

void fx_String(txMachine* the)
{
	if ((mxThis->kind == XS_REFERENCE_KIND) && ((mxThis->value.reference->flag & XS_SHARED_FLAG) == 0)) {
		txSlot* anInstance = mxThis->value.reference;
        if (mxIsString(anInstance)) {
            if (mxArgc > 0) {
                txString aString = fxToString(the, mxArgv(0));
                anInstance->next->value.string = aString;
                anInstance->next->next->value.integer = fxUnicodeLength(aString);
            }
            *mxResult = *mxThis;
        }
        return;
    }

    if (mxArgc > 0) {
        *mxResult = *mxArgv(0);
        fxToString(the, mxResult);
    }
    else {
        mxResult->value.string = mxEmptyString.value.string;
        mxResult->kind = mxEmptyString.kind;
    }
}

void fx_String_charAt(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txInteger anOffset;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.charAt: this is null or undefined");
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

void fx_String_charCodeAt(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txInteger anOffset;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.charCodeAt: this is null or undefined");
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

void fx_String_compare(txMachine* the)
{
	txString aString;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.compare: this is null or undefined");
	if (mxArgc < 1)
		mxResult->value.integer = c_strcmp(aString, "undefined");
	else {
		fxToString(the, mxArgv(0));
		mxResult->value.integer = c_strcmp(aString, mxArgv(0)->value.string);
	}
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_String_concat(txMachine* the)
{
	txString aString;
	txInteger aCount;
	txInteger aLength;
	txInteger anIndex;
	
	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.concat: this is null or undefined");
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
		c = fxToInteger(the, mxArgv(anIndex)) & 0x0000FFFF;
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
		c = fxToInteger(the, mxArgv(anIndex)) & 0x0000FFFF;
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

void fx_String_indexOf(txMachine* the)
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
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.indexOf: this is null or undefined");
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

static txInteger fx_String_indexOf_aux(txMachine* the, txString theString, 
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

void fx_String_lastIndexOf(txMachine* the)
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
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.lastIndexOf: this is null or undefined");
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

void fx_String_match(txMachine* the)
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
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.match: this is null or undefined");
	if ((mxArgc > 0) && mxIsReference(mxArgv(0))) {
		aRegExp = fxGetInstance(the, mxArgv(0));
		if (!mxIsRegExp(aRegExp))
			aRegExp = C_NULL;
	}
	else
		aRegExp = C_NULL;
	if (!aRegExp) {
		if (mxArgc > 0) {
			*(--the->stack) = *mxArgv(0);
			fxInteger(the, --the->stack, 1);
		}
		else
			fxInteger(the, --the->stack, 0);
		*(--the->stack) = the->stackTop[-1];
		fxNewID(the, fxID(the, "RegExp"));
		*mxArgv(0) = *(the->stack++);
		aRegExp = fxGetInstance(the, mxArgv(0));
	}
	fxToString(the, mxThis);
	aProperty = fxGetProperty(the, aRegExp, the->globalID);
	if (aProperty->value.boolean) {
		anOffset = 0;
		mxPush(mxArrayPrototype);
		anArray = fxNewArrayInstance(the);
		*mxResult = *(the->stack++);
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
		*(--the->stack) = *mxThis;
		fxInteger(the, --the->stack, 1);
		*(--the->stack) = *mxArgv(0);
		fxCallID(the, the->execID);
		*mxResult = *(the->stack++);
	}
}

void fx_String_replace(txMachine* the)
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
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.replace: this is null or undefined");
	if (mxArgc < 2)
		mxDebug0(the, XS_SYNTAX_ERROR, "String.prototype.replace: no replaceValue parameter");
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "String.prototype.replace: no searchValue parameter");
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
		aProperty = fxGetProperty(the, aRegExp, the->globalID);
		if (aProperty->value.boolean) {
			aDelta = 0;
			anOffset = 0;
			for (;;) {
				aCount = fxExecuteRegExp(the, aRegExp, mxThis, anOffset);
				if (aCount <= 0)
					break;
				aDelta = fx_String_replace_aux(the, aDelta, aLength, aCount, aRegExp->next);
				if (anOffset == aRegExp->next->value.regexp.offsets[1])
					anOffset++;
				else
					anOffset = aRegExp->next->value.regexp.offsets[1];
			}
		}
		else {
			aCount = fxExecuteRegExp(the, aRegExp, mxThis, 0);
			if (aCount > 0)
				fx_String_replace_aux(the, 0, aLength, aCount, aRegExp->next);
		}
	}
	else {
		mxZeroSlot(&aSubSlot); /* fake regexp slot! */
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
				aDelta = fx_String_replace_aux(the, aDelta, aLength, 1, &aSubSlot);
				if (!aSubLength)
					break;
				subOffsets[0] += aSubLength;
				subOffsets[1] += aSubLength;
			}
		}
	}
}
	
txInteger fx_String_replace_aux(txMachine* the, txInteger theDelta, txInteger theLength, 
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
	
		mxZeroSlot(--the->stack);
		l = theSlot->value.regexp.offsets[1] - theSlot->value.regexp.offsets[0];
		the->stack->value.string = (txString)fxNewChunk(the, l + 1);
		c_memcpy(the->stack->value.string, mxThis->value.string + theSlot->value.regexp.offsets[0], l);
		the->stack->value.string[l] = 0;
		the->stack->kind = XS_STRING_KIND;
		
		for (i = 1; i < theCount; i++) {
			mxZeroSlot(--the->stack);
			l = theSlot->value.regexp.offsets[(2 * i) + 1] - theSlot->value.regexp.offsets[2 * i];
			the->stack->value.string = (txString)fxNewChunk(the, l + 1);
			c_memcpy(the->stack->value.string, mxThis->value.string + theSlot->value.regexp.offsets[2 * i], l);
			the->stack->value.string[l] = 0;
			the->stack->kind = XS_STRING_KIND;
		}
				
		mxZeroSlot(--the->stack);
		the->stack->value.integer = theSlot->value.regexp.offsets[0];
		the->stack->kind = XS_INTEGER_KIND;

		mxZeroSlot(--the->stack);
		the->stack->value.string = mxThis->value.string;
		the->stack->kind = mxThis->kind;
		
		/* ARGC */
		mxZeroSlot(--the->stack);
		the->stack->kind = XS_INTEGER_KIND;
		the->stack->value.integer = theCount + 2;
		/* THIS */
		*(--the->stack) = *mxThis;
		/* FUNCTION */
		*(--the->stack) = *mxArgv(1);
		/* RESULT */
		mxZeroSlot(--the->stack);
		fxRunID(the, XS_NO_ID);
	}
	else
		*(--the->stack) = *mxArgv(1);

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
				//	mxDebug0(the, XS_SYNTAX_ERROR, "String.replace: invalid replaceValue");
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

void fx_String_search(txMachine* the)
{
	txString aString;
	txSlot* aRegExp;
	txInteger aCount;
	txInteger anOffset = -1;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.search: this is null or undefined");
	if ((mxArgc > 0) && mxIsReference(mxArgv(0))) {
		aRegExp = fxGetInstance(the, mxArgv(0));
		if (!mxIsRegExp(aRegExp))
			aRegExp = C_NULL;
	}
	else
		aRegExp = C_NULL;
	if (!aRegExp) {
		if (mxArgc > 0) {
			*(--the->stack) = *mxArgv(0);
			fxInteger(the, --the->stack, 1);
		}
		else
			fxInteger(the, --the->stack, 0);
		*(--the->stack) = the->stackTop[-1];
		fxNewID(the, fxID(the, "RegExp"));
		*mxArgv(0) = *(the->stack++);
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

void fx_String_slice(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txInteger aStart;
	txInteger aStop;
	txInteger anOffset;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.slice: this is null or undefined");
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

void fx_String_split(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txNumber aLimit;
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
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.slice: this is null or undefined");
	aLength = c_strlen(fxToString(the, mxThis));

	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND))
		aLimit = fxToNumber(the, mxArgv(1));
	else
		aLimit = XS_MAX_INDEX;
		
	mxPush(mxArrayPrototype);
	anArray = fxNewArrayInstance(the);
	*mxResult = *(the->stack++);
	fxGetOwnInstance(the, mxResult);
	if (!aLimit || c_isnan(aLimit))
		goto bail;

	anItem = anArray->next;
	
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND)) {
		fx_String_split_aux(the, anArray, anItem, 0, aLength);
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
					anItem = fx_String_split_aux(the, anArray, anItem, anOffset, aRegExp->next->value.regexp.offsets[0]);
					if (anArray->next->value.array.length >= aLimit)
						goto bail;
				}
			}
			else {
				if (anOffset <= aRegExp->next->value.regexp.offsets[0]) {
					anItem = fx_String_split_aux(the, anArray, anItem, anOffset, aRegExp->next->value.regexp.offsets[0]);
					if (anArray->next->value.array.length >= aLimit)
						goto bail;
				}
			}
			anOffset = aRegExp->next->value.regexp.offsets[1];
			for (anIndex = 1; anIndex < aCount; anIndex++) {
				anItem = fx_String_split_aux(the, anArray, anItem, aRegExp->next->value.regexp.offsets[2 * anIndex], aRegExp->next->value.regexp.offsets[(2 * anIndex) + 1]);
				if (anArray->next->value.array.length >= aLimit)
					goto bail;
			}
			if (aSubOffset == aRegExp->next->value.regexp.offsets[1])
				aSubOffset++;
			else
				aSubOffset = aRegExp->next->value.regexp.offsets[1];
		}
		if (anOffset <= aLength)
			fx_String_split_aux(the, anArray, anItem, anOffset, aLength);
	}
	else {
		aSubLength = c_strlen(fxToString(the, mxArgv(0)));
		if (aSubLength == 0) {
			anOffset = 0;
			while (anOffset < aLength) {
				aSubOffset = anOffset + fxUnicodeToUTF8Offset(mxThis->value.string + anOffset, 1);
				anItem = fx_String_split_aux(the, anArray, anItem, anOffset, aSubOffset);
				if (anArray->next->value.array.length >= aLimit)
					goto bail;
				anOffset = aSubOffset;
			}
		}
		else if (aLength == 0) {
			fx_String_split_aux(the, anArray, anItem, 0, 0);
		}
		else {
			anOffset = 0;
			for (;;) {
				aCount = fx_String_indexOf_aux(the, mxThis->value.string, aLength, anOffset, mxArgv(0)->value.string, aSubLength, subOffsets);
				if (aCount <= 0)
					break;
				if (anOffset <= subOffsets[0]) {
					anItem = fx_String_split_aux(the, anArray, anItem, anOffset, subOffsets[0]);
					if (anArray->next->value.array.length >= aLimit)
						goto bail;
				}
				anOffset = subOffsets[1];
			}
			if (anOffset <= aLength)
				fx_String_split_aux(the, anArray, anItem, anOffset, aLength);
		}
	}
bail:
	fxCacheArray(the, anArray);
}

txSlot* fx_String_split_aux(txMachine* the, txSlot* theArray, txSlot* theItem, txInteger theStart, txInteger theStop)
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

void fx_String_substring(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txNumber aNumber;
	txInteger aStart;
	txInteger aStop;
	txInteger anOffset;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.substring: this is null or undefined");
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

void fx_String_toLowerCase(txMachine* the)
{
	txString aString;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.toLowerCase: this is null or undefined");
	mxResult->value.string = fxStringToLower(the, aString);
	mxResult->kind = XS_STRING_KIND;
}

void fx_String_toUpperCase(txMachine* the)
{
	txString aString;

	aString = fxCoerceToString(the, mxThis);
	if (!aString)
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.toUpperCase: this is null or undefined");
	mxResult->value.string = fxStringToUpper(the, aString);
	mxResult->kind = XS_STRING_KIND;
}

void fx_String_trim(txMachine* the)
{
	txBoolean aFlag = 1;
	txInteger aStart = 0;
	txInteger aStop = 0;
	txInteger aLength = 0;
	txU1* aString;
	txU4 aResult;
	txUTF8Sequence *aSequence;
	txInteger aSize;

	aString = (txU1*)fxCoerceToString(the, mxThis);
	if (!aString)
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.trim: this is null or undefined");
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

void fx_String_valueOf(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;
	
	anInstance = fxGetInstance(the, mxThis);
	if (!mxIsString(anInstance))
		mxDebug0(the, XS_TYPE_ERROR, "String.prototype.valueOf: this is no String");
	aProperty = anInstance->next;
	mxResult->value = aProperty->value;
	mxResult->kind = aProperty->kind;
}

txString fxCoerceToString(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_UNDEFINED_KIND)
		return C_NULL;
	if (theSlot->kind == XS_NULL_KIND)
		return C_NULL;
	return fxToString(the, theSlot);
}
