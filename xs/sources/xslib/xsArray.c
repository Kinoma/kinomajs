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

#include "xsAll.h"

#define mxCheckArray(THE_SLOT) \
	if (!THE_SLOT || !mxIsArray(THE_SLOT)) \
		mxDebug0(the, XS_TYPE_ERROR, "this is no Array")

static void fx_Array(txMachine* the);
static void fx_Array_get_length(txMachine* the);
static void fx_Array_set_length(txMachine* the);
static void fx_Array_concat(txMachine* the);
static void fx_Array_eachDown(txMachine* the);
static void fx_Array_eachUp(txMachine* the);
static void fx_Array_every(txMachine* the);
static void fx_Array_filter(txMachine* the);
static void fx_Array_forEach(txMachine* the);
static void fx_Array_indexOf(txMachine* the);
static void fx_Array_join(txMachine* the);
static void fx_Array_lastIndexOf(txMachine* the);
static void fx_Array_map(txMachine* the);
static void fx_Array_pop(txMachine* the);
static void fx_Array_push(txMachine* the);
static void fx_Array_reduce(txMachine* the);
static void fx_Array_reduceRight(txMachine* the);
static void fx_Array_reverse(txMachine* the);
static void fx_Array_shift(txMachine* the);
static void fx_Array_slice(txMachine* the);
static void fx_Array_some(txMachine* the);
static void fx_Array_sort(txMachine* the);
static void fx_Array_sort_aux(txMachine* the, txSlot* theProperty, txIndex l, txIndex u);
static int fx_Array_sort_compare(txMachine* the, txSlot* theProperty, txIndex i, txIndex j);
static txIndex fx_Array_sort_partition(txMachine* the, txSlot* theProperty, txIndex lb, txIndex ub);
static void fx_Array_sort_swap(txMachine* the, txSlot* theProperty, txIndex i, txIndex j);
static void fx_Array_splice(txMachine* the);
static void fx_Array_toLocaleString(txMachine* the);
static void fx_Array_unshift(txMachine* the);

static void fx_Array_isArray(txMachine* the);

void fxBuildArray(txMachine* the)
{
	mxPush(mxGlobal);
			
	mxPush(mxObjectPrototype);
	fxNewArrayInstance(the);
	fxNewHostFunction(the, fx_Array_get_length, 0);
	fxQueueID(the, the->lengthID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_GETTER_FLAG);
	fxNewHostFunction(the, fx_Array_set_length, 1);
	fxQueueID(the, the->lengthID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_SETTER_FLAG);
	fxNewHostFunction(the, fx_Array_concat, 1);
	fxQueueID(the, fxID(the, "concat"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_eachDown, 1);
	fxQueueID(the, fxID(the, "eachDown"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_eachUp, 1);
	fxQueueID(the, fxID(the, "eachUp"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_every, 1);
	fxQueueID(the, fxID(the, "every"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_filter, 1);
	fxQueueID(the, fxID(the, "filter"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_forEach, 1);
	fxQueueID(the, fxID(the, "forEach"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_indexOf, 1);
	fxQueueID(the, fxID(the, "indexOf"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_join, 1);
	fxQueueID(the, fxID(the, "join"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_lastIndexOf, 1);
	fxQueueID(the, fxID(the, "lastIndexOf"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_map, 1);
	fxQueueID(the, fxID(the, "map"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_pop, 0);
	fxQueueID(the, fxID(the, "pop"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_push, 1);
	fxQueueID(the, fxID(the, "push"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_reduce, 1);
	fxQueueID(the, fxID(the, "reduce"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_reduceRight, 1);
	fxQueueID(the, fxID(the, "reduceRight"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_reverse, 0);
	fxQueueID(the, fxID(the, "reverse"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_shift, 0);
	fxQueueID(the, fxID(the, "shift"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_slice, 2);
	fxQueueID(the, fxID(the, "slice"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_some, 1);
	fxQueueID(the, fxID(the, "some"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_sort, 1);
	fxQueueID(the, fxID(the, "sort"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_splice, 2);
	fxQueueID(the, fxID(the, "splice"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_join, 0);
	fxQueueID(the, fxID(the, "toString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_toLocaleString, 0);
	fxQueueID(the, fxID(the, "toLocaleString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Array_unshift, 1);
	fxQueueID(the, fxID(the, "unshift"), XS_DONT_ENUM_FLAG);

	fxAliasInstance(the, the->stack);
	mxArrayPrototype = *the->stack;
	fxNewHostConstructor(the, fx_Array, 1);
	
	fxNewHostFunction(the, fx_Array_isArray, 1);
	fxQueueID(the, fxID(the, "isArray"), XS_DONT_ENUM_FLAG);

	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxArrayPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "Array"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);

	the->stack++;
}

void fxCacheArray(txMachine* the, txSlot* theArray)
{
	txIndex aLength;
	txSlot* anAddress;
	txSlot* srcSlot;
	txSlot* dstSlot;
	
	aLength = theArray->next->value.array.length;
	if (aLength) {
		anAddress = (txSlot *)fxNewChunk(the, aLength * sizeof(txSlot));
		srcSlot = theArray->next->next;
		dstSlot = anAddress;
		while (srcSlot) {
			dstSlot->ID = XS_NO_ID;
			dstSlot->flag = XS_NO_FLAG;
			dstSlot->kind = srcSlot->kind;
			dstSlot->value = srcSlot->value;
			srcSlot = srcSlot->next;
			dstSlot++;
		}
		theArray->next->value.array.address = anAddress;
		theArray->next->next = C_NULL;
	}
}

txSlot* fxQueueItem(txMachine* the, txSlot* theArray)
{
	txSlot** aSlotAddress;
	txSlot* result;
	
	aSlotAddress = &(theArray->next);
	while ((result = *aSlotAddress)) {
		aSlotAddress = &(result->next);
	}
	*aSlotAddress = result = fxNewSlot(the);
	result->ID = XS_NO_ID;
	theArray->next->value.array.length++;
	return result;
}

txSlot* fxNewArrayInstance(txMachine* the)
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
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_ARRAY_KIND;
	aProperty->value.array.length = 0;
	aProperty->value.array.address = C_NULL;
	
	return anInstance;
}

void fx_Array(txMachine* the)
{
	txFlag aFlag;
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aCount;
	txIndex anIndex;

	if (mxResult->kind == XS_UNDEFINED_KIND) {
		mxPush(mxArrayPrototype);
		anArray = fxNewArrayInstance(the);
		*mxResult = *(the->stack++);
	}
	else
		anArray = fxGetOwnInstance(the, mxResult);
	mxCheckArray(anArray);
	aProperty = anArray->next;
	
	aFlag = 0;
	if (mxArgc == 1) {
		if (mxArgv(0)->kind == XS_INTEGER_KIND) {
			txInteger anInteger = mxArgv(0)->value.integer;
			if ((0 <= anInteger) && (anInteger < XS_MAX_INDEX)) {
				aCount = (txIndex)anInteger;
				aFlag = 1;
			}
		}
		else if (mxArgv(0)->kind == XS_NUMBER_KIND) {
			txNumber aNumber = mxArgv(0)->value.number;
			if ((0 <= aNumber) && (aNumber < XS_MAX_INDEX)) {
				aCount = (txIndex)aNumber;
				aNumber = aCount;
				if (aNumber == mxArgv(0)->value.number)
					aFlag = 1;
			}
		}
	}
	if (aFlag) {
		aProperty->value.array.length = aCount;
		aProperty->value.array.address = (txSlot *)fxNewChunk(the, aCount * sizeof(txSlot));
		c_memset(aProperty->value.array.address, 0, aCount * sizeof(txSlot));
	}
	else {
		aCount = mxArgc;
		aProperty->value.array.length = aCount;
		aProperty->value.array.address = (txSlot *)fxNewChunk(the, aCount * sizeof(txSlot));
		aProperty = aProperty->value.array.address;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			aProperty->ID = XS_NO_ID;
			aProperty->flag = XS_NO_FLAG;
			aProperty->kind = mxArgv(anIndex)->kind;
			aProperty->value = mxArgv(anIndex)->value;
			aProperty++;
		}
	}
}

void fx_Array_get_length(txMachine* the)
{
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aLength;
	
	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	if (aLength <= 0x7FFFFFFF) {
		mxResult->value.integer = aLength;
		mxResult->kind = XS_INTEGER_KIND;
	}
	else {
		mxResult->value.number = aLength;
		mxResult->kind = XS_NUMBER_KIND;
	}
}

void fx_Array_set_length(txMachine* the)
{
	txIndex aParam;
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aLength;
	txSlot* anAddress;

	anArray = fxGetOwnInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc == 0)
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.prototype.length: no parameter");
	if (mxArgv(0)->kind == XS_INTEGER_KIND)
		fxIntegerToIndex(the, mxArgv(0)->value.integer, &aParam);
	else
		fxNumberToIndex(the, fxToNumber(the, mxArgv(0)), &aParam);
		
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anAddress = aProperty->value.array.address;

	if (aLength != aParam) {
		if (aParam) {
			anAddress = (txSlot *)fxNewChunk(the, aParam * sizeof(txSlot));
			if (aLength < aParam) {
				if (aLength)
					c_memcpy(anAddress, aProperty->value.array.address, aLength * sizeof(txSlot));
				c_memset(anAddress + aLength, 0, (aParam - aLength) * sizeof(txSlot));
			}
			else
				c_memcpy(anAddress, aProperty->value.array.address, aParam * sizeof(txSlot));
		}
		else
			anAddress = C_NULL;
	}
	
	aProperty = anArray->next;
	aProperty->value.array.length = aParam;
	aProperty->value.array.address = anAddress;
	if (aLength <= 0x7FFFFFFF) {
		mxResult->value.integer = aParam;
		mxResult->kind = XS_INTEGER_KIND;
	}
	else {
		mxResult->value.number = aParam;
		mxResult->kind = XS_NUMBER_KIND;
	}
}

void fx_Array_concat(txMachine* the)
{
	txSlot* dstArray;
	txIndex dstLength;
	txSlot* dstAddress;
	txSlot* dstSlot;
	txIndex aCount;
	txIndex anIndex;
	txSlot* srcArray;
	txIndex srcLength;
	txSlot* srcSlot;
	
	mxPush(mxArrayPrototype);
	dstArray = fxNewArrayInstance(the);
	*mxResult = *(the->stack++);
	dstLength = 0;
	dstAddress = C_NULL;
	
	aCount = mxArgc;

	srcArray = fxGetInstance(the, mxThis);
	mxCheckArray(srcArray);
	srcSlot = srcArray->next;
	srcLength = srcSlot->value.array.length;
	srcSlot = srcSlot->value.array.address;
	while (srcLength) {
		if (srcSlot->ID)
			dstLength++;
		srcLength--;
		srcSlot++;
	}
	for (anIndex = 0; anIndex < aCount; anIndex++) {
		srcSlot = mxArgv(anIndex);
		if (mxIsReference(srcSlot)) {
			srcArray = fxGetInstance(the, srcSlot);
            if (mxIsArray(srcArray)) {
                srcSlot = srcArray->next;
                srcLength = srcSlot->value.array.length;
                srcSlot = srcSlot->value.array.address;
                while (srcLength) {
                    if (srcSlot->ID)
                        dstLength++;
                    srcLength--;
                    srcSlot++;
                }
                continue;
            }
        }
        dstLength++;
	}
	
	if (dstLength) {
		dstSlot = dstAddress = (txSlot *)fxNewChunk(the, dstLength * sizeof(txSlot));

		srcArray = fxGetInstance(the, mxThis);
		srcSlot = srcArray->next;
		srcLength = srcSlot->value.array.length;
		srcSlot = srcSlot->value.array.address;
		while (srcLength) {
			if (srcSlot->ID) {
				*dstSlot = *srcSlot;
				dstSlot++;
			}
			srcLength--;
			srcSlot++;
		}
		
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			srcSlot = mxArgv(anIndex);
			if (mxIsReference(srcSlot)) {
				srcArray = fxGetInstance(the, srcSlot);
                if (mxIsArray(srcArray)) {
                    srcSlot = srcArray->next;
                    srcLength = srcSlot->value.array.length;
                    srcSlot = srcSlot->value.array.address;
                    while (srcLength) {
                        if (srcSlot->ID) {
                            *dstSlot = *srcSlot;
                            dstSlot++;
                        }
                        srcLength--;
                        srcSlot++;
                    }
                    continue;
                }
            }
            dstSlot->ID = XS_NO_ID;
            dstSlot->flag = XS_NO_FLAG;
            dstSlot->kind = srcSlot->kind;
            dstSlot->value = srcSlot->value;
            dstSlot++;
		}
		
		dstSlot = dstArray->next;
		dstSlot->value.array.length = dstLength;
		dstSlot->value.array.address = dstAddress;
	}
}

void fx_Array_eachDown(txMachine* the)
{
	txSlot* anArray;
	txSlot* aFunction;
	txSlot* aProperty;
	txIndex aLength;
	txSlot* aSlot;
	txNumber aStart;
	txNumber aStop;
	txIndex anOffset;
	txIndex anIndex;

	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.prototype.eachDown: no iteratefn parameter");
	if (!mxIsReference(mxArgv(0)))
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.prototype.eachDown: iteratefn is no instance");
	aFunction = fxGetInstance(the, mxArgv(0));
	if (!mxIsFunction(aFunction))
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.prototype.eachDown: iteratefn is no no function");
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;

	aStart = 0;
	aStop = aLength;
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		aStart = c_floor(fxToNumber(the, mxArgv(1)));
		if (aStart < 0) {
			aStart += aLength;
			if (aStart < 0)
				aStart = 0;
		}
		else if (aStart > aLength)
			aStart = aLength;
	}
	if ((mxArgc > 2) && (mxArgv(2)->kind != XS_UNDEFINED_KIND)) {
		aStop = c_floor(fxToNumber(the, mxArgv(2)));
		if (aStop < 0) {
			aStop += aLength;
			if (aStop < 0)
				aStop = 0;
		}
		else if (aStop > aLength)
			aStop = aLength;
	}
	if (aStart >= aStop)
		return;
		
	anOffset = (txIndex)aStart;
	for (anIndex = (txIndex)aStop - 1; anIndex >= anOffset; anIndex--) {
		mxZeroSlot(--the->stack);
		aSlot = aProperty->value.array.address + anIndex;
		the->stack->value = aSlot->value;
		the->stack->kind = aSlot->kind;
		mxZeroSlot(--the->stack);
		the->stack->value.integer = anIndex;
		the->stack->kind = XS_INTEGER_KIND;		
		/* ARGC */
		mxZeroSlot(--the->stack);
		the->stack->value.integer = 2;
		the->stack->kind = XS_INTEGER_KIND;
		/* THIS */
		*(--the->stack) = *mxThis;
		/* FUNCTION */
		*(--the->stack) = *mxArgv(0);
		/* RESULT */
		mxZeroSlot(--the->stack);
		fxRunID(the, XS_NO_ID);
		if (the->stack->kind != XS_UNDEFINED_KIND) {
			*mxResult = *the->stack;
			break;
		}
		the->stack++;
	}
}

void fx_Array_eachUp(txMachine* the)
{
	txSlot* anArray;
	txSlot* aFunction;
	txSlot* aProperty;
	txIndex aLength;
	txSlot* aSlot;
	txNumber aStart;
	txNumber aStop;
	txIndex anOffset;
	txIndex anIndex;

	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.prototype.eachUp: no iteratefn parameter");
	if (!mxIsReference(mxArgv(0)))
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.prototype.eachDown: iteratefn is no instance");
	aFunction = fxGetInstance(the, mxArgv(0));
	if (!mxIsFunction(aFunction))
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.prototype.eachDown: iteratefn is no no function");
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;

	aStart = 0;
	aStop = aLength;
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		aStart = c_floor(fxToNumber(the, mxArgv(1)));
		if (aStart < 0) {
			aStart += aLength;
			if (aStart < 0)
				aStart = 0;
		}
		else if (aStart > aLength)
			aStart = aLength;
	}
	if ((mxArgc > 2) && (mxArgv(2)->kind != XS_UNDEFINED_KIND)) {
		aStop = c_floor(fxToNumber(the, mxArgv(2)));
		if (aStop < 0) {
			aStop += aLength;
			if (aStop < 0)
				aStop = 0;
		}
		else if (aStop > aLength)
			aStop = aLength;
	}
	if (aStart >= aStop)
		return;

	anOffset = (txIndex)aStop;
	for (anIndex = (txIndex)aStart; anIndex < anOffset; anIndex++) {
		mxZeroSlot(--the->stack);
		aSlot = aProperty->value.array.address + anIndex;
		the->stack->value = aSlot->value;
		the->stack->kind = aSlot->kind;
		mxZeroSlot(--the->stack);
		the->stack->value.integer = anIndex;
		the->stack->kind = XS_INTEGER_KIND;		
		/* ARGC */
		mxZeroSlot(--the->stack);
		the->stack->value.integer = 2;
		the->stack->kind = XS_INTEGER_KIND;
		/* THIS */
		*(--the->stack) = *mxThis;
		/* FUNCTION */
		*(--the->stack) = *mxArgv(0);
		/* RESULT */
		mxZeroSlot(--the->stack);
		fxRunID(the, XS_NO_ID);
		if (the->stack->kind != XS_UNDEFINED_KIND) {
			*mxResult = *the->stack;
			break;
		}
		the->stack++;
	}
}

void fxCallItem(txMachine* the, txSlot* theItem, txIndex theIndex)
{
	/* ARG0 */
	mxZeroSlot(--the->stack);
	the->stack->value = theItem->value;
	the->stack->kind = theItem->kind;;	
	/* ARG1 */
	mxZeroSlot(--the->stack);
	the->stack->value.integer = theIndex;
	the->stack->kind = XS_INTEGER_KIND;	
	/* ARG2 */
	mxZeroSlot(--the->stack);
	the->stack->value = mxThis->value;
	the->stack->kind = mxThis->kind;
	/* ARGC */
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 3;
	the->stack->kind = XS_INTEGER_KIND;
	/* THIS */
	if (mxArgc > 1)
		*(--the->stack) = *mxArgv(1);
	else
		mxZeroSlot(--the->stack);
	/* FUNCTION */
	*(--the->stack) = *mxArgv(0);
	/* RESULT */
	mxZeroSlot(--the->stack);
	fxRunID(the, XS_NO_ID);
}

void fx_Array_every(txMachine* the)
{
	txSlot* anArray;
	txSlot* aFunction;
	txSlot* aProperty;
	txIndex aLength;
	txIndex anIndex;
	txSlot* aValue;

	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc < 1)
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.every: no callbackfn parameter");
	if (mxIsUndefined(mxArgv(0)))
		mxDebug0(the, XS_REFERENCE_ERROR, "Array.prototype.every: callbackfn is undefined");
	aFunction = fxGetInstance(the, mxArgv(0));
	if (!mxIsFunction(aFunction))
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.every: callbackfn is no no function");
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
		
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anIndex = 0;
	while (anIndex < aLength) {
		aValue = aProperty->value.array.address + anIndex;
		if (aValue->ID) {
			fxCallItem(the, aValue, anIndex);
			mxResult->value.boolean = fxToBoolean(the, the->stack);
			the->stack++;
			if (!mxResult->value.boolean)
				break;
		}
		anIndex++;
	}
}

void fx_Array_filter(txMachine* the)
{
	txSlot* anArray;
	txSlot* aFunction;
	txSlot* aResult;
	txSlot* anItem;
	txSlot* aProperty;
	txIndex aLength;
	txIndex anIndex;
	txIndex aCount;
	txSlot* aValue;

	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc < 1)
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.filter: no callbackfn parameter");
	if (mxIsUndefined(mxArgv(0)))
		mxDebug0(the, XS_REFERENCE_ERROR, "Array.prototype.filter: callbackfn is undefined");
	aFunction = fxGetInstance(the, mxArgv(0));
	if (!mxIsFunction(aFunction))
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.filter: callbackfn is no no function");
	mxPush(mxArrayPrototype);
	aResult = fxNewArrayInstance(the);
	*mxResult = *(the->stack++);
	anItem = aResult->next;

	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anIndex = 0;
	aCount = 0;
	while (anIndex < aLength) {
		aValue = aProperty->value.array.address + anIndex;
		if (aValue->ID) {
			fxCallItem(the, aValue, anIndex);
			if (fxToBoolean(the, the->stack)) {
				aValue = aProperty->value.array.address + anIndex; // GC
				anItem->next = fxNewSlot(the);
				anItem = anItem->next;
				anItem->kind = aValue->kind;
				anItem->value = aValue->value;
				aCount++;
			}
			the->stack++;
		}
		anIndex++;
	}
	
	aResult->next->value.array.length = aCount;
	fxCacheArray(the, aResult);
}

void fx_Array_forEach(txMachine* the)
{
	txSlot* anArray;
	txSlot* aFunction;
	txSlot* aProperty;
	txIndex aLength;
	txIndex anIndex;
	txSlot* aValue;

	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc < 1)
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.forEach: no callbackfn parameter");
	if (mxIsUndefined(mxArgv(0)))
		mxDebug0(the, XS_REFERENCE_ERROR, "Array.prototype.forEach: callbackfn is undefined");
	aFunction = fxGetInstance(the, mxArgv(0));
	if (!mxIsFunction(aFunction))
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.forEach: callbackfn is no no function");
		
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anIndex = 0;
	while (anIndex < aLength) {
		aValue = aProperty->value.array.address + anIndex;
		if (aValue->ID) {
			fxCallItem(the, aValue, anIndex);
			the->stack++;
		}
		anIndex++;
	}
}

void fx_Array_indexOf(txMachine* the)
{
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aLength;
	txSize anIndex;
	txSlot* aValue;

	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.prototype.indexOf: no searchElement parameter");
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = -1;
		
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	if (aLength == 0)
		return;
	if (mxArgc > 1) {
		anIndex = fxToInteger(the, mxArgv(1));
		if (anIndex < 0) {
			anIndex = aLength + anIndex;
			if (anIndex < 0)
				anIndex = 0;
		}
	}
	else
		anIndex = 0;
	while (anIndex < (txSize)aLength) {
		aValue = aProperty->value.array.address + anIndex;
		if (aValue->ID) {
			if (fxIsSameSlot(the, aValue, mxArgv(0))) {
				mxResult->value.integer = anIndex;
				return;
			}
		}
		anIndex++;
	}
}

static void joinChars(txMachine* the, txStringCStream* theStream, char* s);
void joinChars(txMachine* the, txStringCStream* theStream, char* s)
{
	txSize aSize = c_strlen(s);
	if ((theStream->offset + aSize) >= theStream->size) {
		char* aBuffer;
		theStream->size += ((aSize / 1024) + 1) * 1024;
		aBuffer = c_realloc(theStream->buffer, theStream->size);
		if (!aBuffer) {
			c_free(theStream->buffer);
			mxDebug1(the, XS_RANGE_ERROR, "Array.prototype.join: not enough memory %ld", theStream->size);
		}
		theStream->buffer = aBuffer;
	}
	c_memcpy(theStream->buffer + theStream->offset, s, aSize);
	theStream->offset += aSize;
}

void fx_Array_join(txMachine* the)
{
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aLength;
	txIndex anIndex;
	txSlot* anItem;
	txStringCStream cStream;
	
	anArray = fxGetOwnInstance(the, mxThis);
	mxCheckArray(anArray);
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;

	mxResult->value.string = mxEmptyString.value.string;
	mxResult->kind = mxEmptyString.kind;
	if (aLength) {
		--the->stack;
		if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
			fxToString(the, mxArgv(0));
			*(the->stack) = *mxArgv(0);
		}
		else
			fxCopyStringC(the, the->stack, ",");

		cStream.buffer = NULL;
		cStream.size = 0;
		cStream.offset = 0;

		for (anIndex = 0; anIndex < aLength; anIndex++) {
			if (anIndex > 0)
				joinChars(the, &cStream, the->stack->value.string);
			anItem = aProperty->value.array.address + anIndex;	
			if (anItem->ID) {
				if ((anItem->kind == XS_REFERENCE_KIND)	|| (anItem->kind == XS_ALIAS_KIND)) {
					mxZeroSlot(--the->stack);
					the->stack->kind = XS_INTEGER_KIND;
					mxZeroSlot(--the->stack);
					the->stack->kind = anItem->kind;
					the->stack->value = anItem->value;
					fxCallID(the, the->toStringID);
					fxToString(the, the->stack);
					joinChars(the, &cStream, the->stack->value.string);
					the->stack++;
				}
				else if ((anItem->kind != XS_UNDEFINED_KIND) && (anItem->kind != XS_NULL_KIND)) {
					mxZeroSlot(--the->stack);
					the->stack->kind = anItem->kind;
					the->stack->value = anItem->value;
					fxToString(the, the->stack);
					joinChars(the, &cStream, the->stack->value.string);
					the->stack++;
				}
			}
		}
		the->stack++;
		mxResult->value.string = fxNewChunk(the, cStream.offset + 1);
		c_memmove(mxResult->value.string, cStream.buffer, cStream.offset);
		mxResult->value.string[cStream.offset] = 0;
		c_free(cStream.buffer);
	}
}

void fx_Array_lastIndexOf(txMachine* the)
{
	txSlot* anArray;
	txSlot* aProperty;
	txSize aLength;
	txSize anIndex;
	txSlot* aValue;

	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.prototype.lastIndexOf: no searchElement parameter");
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = -1;
		
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	if (aLength == 0)
		return;
	if (mxArgc > 1) {
		anIndex = fxToInteger(the, mxArgv(1));
		if (anIndex < 0)
			anIndex = aLength + anIndex;
		else if (anIndex > aLength - 1)
			anIndex = aLength - 1;
	}
	else
		anIndex = aLength - 1;
	while (anIndex >= 0) {
		aValue = aProperty->value.array.address + anIndex;
		if (aValue->ID) {
			if (fxIsSameSlot(the, aValue, mxArgv(0))) {
				mxResult->value.integer = anIndex;
				return;
			}
		}
		anIndex--;
	}
}

void fx_Array_map(txMachine* the)
{
	txSlot* anArray;
	txSlot* aFunction;
	txSlot* aResult;
	txSlot* anItem;
	txSlot* aProperty;
	txIndex aLength;
	txIndex anIndex;
	txIndex aCount;
	txSlot* aValue;

	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc < 1)
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.map: no callbackfn parameter");
	if (mxIsUndefined(mxArgv(0)))
		mxDebug0(the, XS_REFERENCE_ERROR, "Array.prototype.map: callbackfn is undefined");
	aFunction = fxGetInstance(the, mxArgv(0));
	if (!mxIsFunction(aFunction))
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.map: callbackfn is no no function");
	mxPush(mxArrayPrototype);
	aResult = fxNewArrayInstance(the);
	*mxResult = *(the->stack++);
	anItem = aResult->next;

	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anIndex = 0;
	aCount = 0;
	while (anIndex < aLength) {
		aValue = aProperty->value.array.address + anIndex;
		if (aValue->ID) {
			fxCallItem(the, aValue, anIndex);
			anItem->next = fxNewSlot(the);
			anItem = anItem->next;
			anItem->kind = the->stack->kind;
			anItem->value = the->stack->value;
			aCount++;
			the->stack++;
		}
		anIndex++;
	}
	
	aResult->next->value.array.length = aCount;
	fxCacheArray(the, aResult);
}

void fx_Array_pop(txMachine* the)
{
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aLength;
	txSlot* anAddress;

	anArray = fxGetOwnInstance(the, mxThis);
	mxCheckArray(anArray);
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anAddress = aProperty->value.array.address;

	if (aLength) {
		anAddress += aLength - 1;
		mxResult->kind = anAddress->kind;
		mxResult->value = anAddress->value;
		aLength--;
		if (aLength) {
			anAddress = (txSlot *)fxNewChunk(the, aLength * sizeof(txSlot));
			c_memcpy(anAddress, aProperty->value.array.address, aLength * sizeof(txSlot));
		}
		else
			anAddress = C_NULL;
	}
	
	aProperty->value.array.length = aLength;
	aProperty->value.array.address = anAddress;
}

void fx_Array_push(txMachine* the)
{
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aLength;
	txSlot* anAddress;
	txIndex aCount;
	txIndex anIndex;
	txSlot* aSlot;

	anArray = fxGetOwnInstance(the, mxThis);
	mxCheckArray(anArray);
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anAddress = aProperty->value.array.address;

	if ((aCount = mxArgc)) {
		fxNumberToIndex(the, (txNumber)aLength + (txNumber)aCount, &anIndex);
		anAddress = (txSlot *)fxNewChunk(the, anIndex * sizeof(txSlot));
		if (aLength)
			c_memcpy(anAddress, aProperty->value.array.address, aLength * sizeof(txSlot));
		aSlot = anAddress + aLength;
		aLength = anIndex;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			aSlot->ID = XS_NO_ID;
			aSlot->flag = XS_NO_FLAG;
			aSlot->kind = mxArgv(anIndex)->kind;
			aSlot->value = mxArgv(anIndex)->value;
			aSlot++;
		}
	}

	aProperty->value.array.length = aLength;
	aProperty->value.array.address = anAddress;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = aLength;
}

void fx_Array_reduce_aux(txMachine* the, txSlot* theItem, txIndex theIndex)
{
	/* ARG0 */
	mxZeroSlot(--the->stack);
	the->stack->value = mxResult->value;
	the->stack->kind = mxResult->kind;;	
	/* ARG1 */
	mxZeroSlot(--the->stack);
	the->stack->value = theItem->value;
	the->stack->kind = theItem->kind;;	
	/* ARG2 */
	mxZeroSlot(--the->stack);
	the->stack->value.integer = theIndex;
	the->stack->kind = XS_INTEGER_KIND;	
	/* ARG3 */
	mxZeroSlot(--the->stack);
	the->stack->value = mxThis->value;
	the->stack->kind = mxThis->kind;
	/* ARGC */
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 4;
	the->stack->kind = XS_INTEGER_KIND;
	/* THIS */
	mxZeroSlot(--the->stack);
	/* FUNCTION */
	*(--the->stack) = *mxArgv(0);
	/* RESULT */
	mxZeroSlot(--the->stack);
	fxRunID(the, XS_NO_ID);
}

void fx_Array_reduce(txMachine* the)
{
	txSlot* anArray;
	txSlot* aFunction;
	txSlot* aProperty;
	txIndex aLength;
	txIndex anIndex;
	txSlot* aValue;

	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc < 1)
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.reduce: no callbackfn parameter");
	if (mxIsUndefined(mxArgv(0)))
		mxDebug0(the, XS_REFERENCE_ERROR, "Array.prototype.reduce: callbackfn is undefined");
	aFunction = fxGetInstance(the, mxArgv(0));
	if (!mxIsFunction(aFunction))
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.reduce: callbackfn is no function");
		
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anIndex = 0;
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else {
		while (anIndex < aLength) {
			aValue = aProperty->value.array.address + anIndex;
			anIndex++;
			if (aValue->ID) {
				mxResult->kind = aValue->kind;
				mxResult->value = aValue->value;
				break;
			}
		}
		if (anIndex == aLength)
			mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.reduce: no initialValue parameter");
	}
	while (anIndex < aLength) {
		aValue = aProperty->value.array.address + anIndex;
		if (aValue->ID) {
			fx_Array_reduce_aux(the, aValue, anIndex);
			mxResult->kind = the->stack->kind;
			mxResult->value = the->stack->value;
			the->stack++;
		}
		anIndex++;
	}
}

void fx_Array_reduceRight(txMachine* the)
{
	txSlot* anArray;
	txSlot* aFunction;
	txSlot* aProperty;
	txIndex aLength;
	txIndex anIndex;
	txSlot* aValue;

	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc < 1)
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.reduceRight: no callbackfn parameter");
	if (mxIsUndefined(mxArgv(0)))
		mxDebug0(the, XS_REFERENCE_ERROR, "Array.prototype.reduceRight: callbackfn is undefined");
	aFunction = fxGetInstance(the, mxArgv(0));
	if (!mxIsFunction(aFunction))
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.reduceRight: callbackfn is no function");
		
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anIndex = aLength;
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else {
		while (anIndex > 0) {
			anIndex--;
			aValue = aProperty->value.array.address + anIndex;
			if (aValue->ID) {
				mxResult->kind = aValue->kind;
				mxResult->value = aValue->value;
				break;
			}
		}
		if (anIndex == 0)
			mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.reduceRight: no initialValue parameter");
	}
	while (anIndex > 0) {
		anIndex--;
		aValue = aProperty->value.array.address + anIndex;
		if (aValue->ID) {
			fx_Array_reduce_aux(the, aValue, anIndex);
			mxResult->kind = the->stack->kind;
			mxResult->value = the->stack->value;
			the->stack++;
		}
	}
}

void fx_Array_reverse(txMachine* the)
{
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aLength;
	txSlot* aFirst;
	txSlot* aLast;
	txSlot aBuffer;
	
	anArray = fxGetOwnInstance(the, mxThis);
	mxCheckArray(anArray);
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	if (aLength) {
		aFirst = aProperty->value.array.address;
		aLast = aFirst + aLength - 1;
		while (aFirst < aLast) {
			aBuffer = *aLast;
			*aLast = *aFirst;
			*aFirst = aBuffer;
			aFirst++;
			aLast--;
		}
	}
	*mxResult = *mxThis;
}

void fx_Array_shift(txMachine* the)
{
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aLength;
	txSlot* anAddress;

	anArray = fxGetOwnInstance(the, mxThis);
	mxCheckArray(anArray);
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anAddress = aProperty->value.array.address;

	if (aLength) {
		mxResult->kind = anAddress->kind;
		mxResult->value = anAddress->value;
		aLength--;
		if (aLength) {
			anAddress = (txSlot *)fxNewChunk(the, aLength * sizeof(txSlot));
			c_memcpy(anAddress, aProperty->value.array.address + 1, aLength * sizeof(txSlot));
		}
		else
			anAddress = C_NULL;
	}
	
	aProperty->value.array.length = aLength;
	aProperty->value.array.address = anAddress;
}

void fx_Array_slice(txMachine* the)
{
	txSlot* thisArray;
	txSlot* resultArray;
	txIndex aLength;
	txNumber aStart;
	txNumber aStop;
	txIndex anOffset;
	txSlot* anAddress;

	thisArray = fxGetInstance(the, mxThis);
	mxCheckArray(thisArray);
	thisArray = thisArray->next;
	
	mxPush(mxArrayPrototype);
	resultArray = fxNewArrayInstance(the);
	*mxResult = *(the->stack++);
	resultArray = resultArray->next;

	aLength = thisArray->value.array.length;
	aStart = 0;
	aStop = aLength;
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
		aStart = c_floor(fxToNumber(the, mxArgv(0)));
		if (aStart < 0) {
			aStart += aLength;
			if (aStart < 0)
				aStart = 0;
		}
		else if (aStart > aLength)
			aStart = aLength;
	}
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		aStop = c_floor(fxToNumber(the, mxArgv(1)));
		if (aStop < 0) {
			aStop += aLength;
			if (aStop < 0)
				aStop = 0;
		}
		else if (aStop > aLength)
			aStop = aLength;
	}
	if (aStart >= aStop)
		return;
	
	anOffset = (txIndex)aStart;
	aLength = (txIndex)aStop - (txIndex)aStart;
	anAddress = (txSlot *)fxNewChunk(the, aLength * sizeof(txSlot));
	c_memcpy(anAddress, thisArray->value.array.address + anOffset, aLength * sizeof(txSlot));
	resultArray->value.array.length = aLength;
	resultArray->value.array.address = anAddress;
}

void fx_Array_some(txMachine* the)
{
	txSlot* anArray;
	txSlot* aFunction;
	txSlot* aProperty;
	txIndex aLength;
	txIndex anIndex;
	txSlot* aValue;

	anArray = fxGetInstance(the, mxThis);
	mxCheckArray(anArray);
	if (mxArgc < 1)
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.reduceRight: no callbackfn parameter");
	if (mxIsUndefined(mxArgv(0)))
		mxDebug0(the, XS_REFERENCE_ERROR, "Array.prototype.reduceRight: callbackfn is undefined");
	aFunction = fxGetInstance(the, mxArgv(0));
	if (!mxIsFunction(aFunction))
		mxDebug0(the, XS_TYPE_ERROR, "Array.prototype.reduceRight: callbackfn is no function");
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
		
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anIndex = 0;
	while (anIndex < aLength) {
		aValue = aProperty->value.array.address + anIndex;
		if (aValue->ID) {
			fxCallItem(the, aValue, anIndex);
			mxResult->value.boolean = fxToBoolean(the, the->stack);
			the->stack++;
			if (mxResult->value.boolean)
				break;
		}
		anIndex++;
	}
}

void fx_Array_sort(txMachine* the)
{
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aLength;
	
	anArray = fxGetOwnInstance(the, mxThis);
	mxCheckArray(anArray);
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	if (aLength)
		fx_Array_sort_aux(the, aProperty, 0, aLength - 1);
	*mxResult = *mxThis;
}

void fx_Array_sort_aux(txMachine* the, txSlot* theProperty, txIndex lb, txIndex ub)
{
    while (lb < ub) {
        txIndex m;

        m = fx_Array_sort_partition(the, theProperty, lb, ub);

        /* eliminate tail recursion and */
        /* sort the smallest partition first */
        /* to minimize stack requirements    */
        if (m - lb <= ub - m) {
            fx_Array_sort_aux(the, theProperty, lb, m);
            lb = m + 1;
        } 
        else {
            fx_Array_sort_aux(the, theProperty, m + 1, ub);
            ub = m;
        }
    }
}

int fx_Array_sort_compare(txMachine* the, txSlot* theProperty, txIndex i, txIndex j)
{
	txSlot* anAddress = theProperty->value.array.address;
	txSlot* a = anAddress + i;
	txSlot* b = mxResult;
	txSlot* aFunction;
	int result;
	
	if (!(a->ID))
		result = (!(b->ID)) ? 0 : 1;
	else if (!(b->ID))
		result = -1;
	else if (a->kind == XS_UNDEFINED_KIND)
		result = (b->kind == XS_UNDEFINED_KIND) ? 0 : 1;
	else if (b->kind == XS_UNDEFINED_KIND)
		result = -1;
	else {
		mxZeroSlot(--the->stack);
		the->stack->kind = a->kind;
		the->stack->value = a->value;
		mxZeroSlot(--the->stack);
		the->stack->kind = b->kind;
		the->stack->value = b->value;
		if ((mxArgc > 0) && mxIsReference(mxArgv(0))) {
			aFunction = fxGetInstance(the, mxArgv(0));
			if (!mxIsFunction(aFunction))
				aFunction = C_NULL;
		}
		else
			aFunction = C_NULL;
		if (aFunction) {
			/* ARGC */
			mxZeroSlot(--the->stack);
			the->stack->kind = XS_INTEGER_KIND;
			the->stack->value.integer = 2;
			/* THIS */
			*(--the->stack) = *mxThis;
			/* FUNCTION */
			*(--the->stack) = *mxArgv(0);
			/* RESULT */
			mxZeroSlot(--the->stack);
			fxRunID(the, XS_NO_ID);
			if (the->stack->kind == XS_INTEGER_KIND)
				result = the->stack->value.integer;
			else {
				txNumber number = fxToNumber(the, the->stack);
				result = (number < 0) ? -1 :  (number > 0) ? 1 : 0;
			}
			the->stack++;
		}
		else {
			fxToString(the, the->stack + 1);
			fxToString(the, the->stack);
			result = c_strcmp((the->stack + 1)->value.string, the->stack->value.string);
			the->stack += 2;
		}
	}
	return result;
}

txIndex fx_Array_sort_partition(txMachine* the, txSlot* theProperty, txIndex lb, txIndex ub)
{
	txSlot* anAddress = theProperty->value.array.address;
	txSlot* b = anAddress + ((lb + ub) / 2);
    txIndex i = lb - 1;
    txIndex j = ub + 1;
	*mxResult = *b;
    
    for (;;) {
        while (fx_Array_sort_compare(the, theProperty, --j, 0) > 0);
        while (fx_Array_sort_compare(the, theProperty, ++i, 0) < 0);
        if (i >= j) break;
		fx_Array_sort_swap(the, theProperty, i, j);
    }
    return j;
}

void fx_Array_sort_swap(txMachine* the, txSlot* theProperty, txIndex i, txIndex j)
{
	txSlot* anAddress = theProperty->value.array.address;
	txSlot a = anAddress[i];
	txSlot b = anAddress[j];
	anAddress[i] = b;
	anAddress[j] = a;
}

void fx_Array_splice(txMachine* the)
{
	txSlot* thisArray;
	txSlot* resultArray;
	txIndex aCount;
	txIndex anIndex;
	txIndex aLength;
	txNumber aStart;
	txNumber aStop;
	txIndex resultLength;
	txIndex thisLength;
	txIndex anOffset;
	txSlot* resultAddress;
	txSlot* thisAddress;
	txSlot* aSlot;
	txSlot* aParam;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.prototype.splice: no start parameter");
	if (mxArgc < 2)
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.prototype.splice: no deleteCount parameter");

	thisArray = fxGetOwnInstance(the, mxThis);
	mxCheckArray(thisArray);
	thisArray = thisArray->next;
	
	mxPush(mxArrayPrototype);
	resultArray = fxNewArrayInstance(the);
	*mxResult = *(the->stack++);
	resultArray = resultArray->next;
		
	aCount = mxArgc - 2;
	aLength = thisArray->value.array.length;
	aStart = c_floor(fxToNumber(the, mxArgv(0)));
	if (aStart < 0) {
		aStart += aLength;
		if (aStart < 0)
			aStart = 0;
	}
	else if (aStart > aLength)
		aStart = aLength;
	
	aStop = aStart + c_floor(fxToNumber(the, mxArgv(1)));
	if (aStop < aStart)
		aStop = aStart;
	else if (aStop > aLength)
		aStop = aLength;
		
	resultLength = (txIndex)aStop - (txIndex)aStart;
	fxNumberToIndex(the, (txNumber)aCount + (txNumber)aLength - (txNumber)resultLength, &thisLength);

	anOffset = (txIndex)aStart;
	if (resultLength) {
		resultAddress = (txSlot *)fxNewChunk(the, resultLength * sizeof(txSlot));
		c_memcpy(resultAddress, thisArray->value.array.address + anOffset, resultLength * sizeof(txSlot));
		resultArray->value.array.length = resultLength;
		resultArray->value.array.address = resultAddress;
	}
	
	aSlot = thisAddress = (txSlot *)fxNewChunk(the, thisLength * sizeof(txSlot));
	if (anOffset > 0) {
		c_memcpy(aSlot, thisArray->value.array.address, anOffset * sizeof(txSlot));
		aSlot += anOffset;
	}
	for (anIndex = 0; anIndex < aCount; anIndex++) {
		aParam = mxArgv(2 + anIndex);
		aSlot->ID = XS_NO_ID;
		aSlot->flag = XS_NO_FLAG;
		aSlot->kind = aParam->kind;
		aSlot->value = aParam->value;
		aSlot++;
	}
	anOffset = (txIndex)aStop;
	if (aLength > anOffset)
		c_memcpy(aSlot, thisArray->value.array.address + anOffset, (aLength - anOffset) * sizeof(txSlot));
		
	thisArray->value.array.length = thisLength;
	thisArray->value.array.address = thisAddress;
}

void fx_Array_toLocaleString(txMachine* the)
{
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aLength;
	txIndex anIndex;
	txSlot* anItem;
	
	anArray = fxGetOwnInstance(the, mxThis);
	mxCheckArray(anArray);
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;

	mxResult->value.string = mxEmptyString.value.string;
	mxResult->kind = mxEmptyString.kind;
	if (aLength) {
		--the->stack;
		if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
			fxToString(the, mxArgv(0));
			*(the->stack) = *mxArgv(0);
		}
		else
			fxCopyStringC(the, the->stack, ",");
		for (anIndex = 0; anIndex < aLength; anIndex++) {
			if (anIndex > 0)
				fxConcatString(the, mxResult, the->stack);
			anItem = aProperty->value.array.address + anIndex;
			if (anItem->ID) {
				if ((anItem->kind == XS_REFERENCE_KIND)	|| (anItem->kind == XS_ALIAS_KIND)) {
					mxZeroSlot(--the->stack);
					the->stack->kind = XS_INTEGER_KIND;
					mxZeroSlot(--the->stack);
					the->stack->kind = anItem->kind;
					the->stack->value = anItem->value;
					fxCallID(the, fxID(the, "toLocaleString"));
					fxToString(the, the->stack);
					fxConcatString(the, mxResult, the->stack);
					the->stack++;
				}
				else if ((anItem->kind != XS_UNDEFINED_KIND) && (anItem->kind != XS_NULL_KIND)) {
					mxZeroSlot(--the->stack);
					the->stack->kind = anItem->kind;
					the->stack->value = anItem->value;
					fxToString(the, the->stack);
					fxConcatString(the, mxResult, the->stack);
					the->stack++;
				}
			}
		}
		the->stack++;
	}
}

void fx_Array_unshift(txMachine* the)
{
	txSlot* anArray;
	txSlot* aProperty;
	txIndex aLength;
	txSlot* anAddress;
	txIndex aCount;
	txIndex anIndex;
	txSlot* aSlot;

	anArray = fxGetOwnInstance(the, mxThis);
	mxCheckArray(anArray);
	aProperty = anArray->next;
	aLength = aProperty->value.array.length;
	anAddress = aProperty->value.array.address;

	if ((aCount = mxArgc)) {
		fxNumberToIndex(the, (txNumber)aLength + (txNumber)aCount, &anIndex);
		anAddress = (txSlot *)fxNewChunk(the, anIndex * sizeof(txSlot));
		if (aLength)
			c_memcpy(anAddress + aCount, aProperty->value.array.address, aLength * sizeof(txSlot));
		aSlot = anAddress;
		aLength = anIndex;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			aSlot->ID = XS_NO_ID;
			aSlot->flag = XS_NO_FLAG;
			aSlot->kind = mxArgv(anIndex)->kind;
			aSlot->value = mxArgv(anIndex)->value;
			aSlot++;
		}
	}
	
	aProperty->value.array.length = aLength;
	aProperty->value.array.address = anAddress;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = aLength;
}

void fx_Array_isArray(txMachine* the)
{
	txSlot* anArray;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Array.isArray: no instance parameter");
	anArray = fxGetInstance(the, mxArgv(0));
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = anArray && mxIsArray(anArray);
}

