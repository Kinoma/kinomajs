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
#include "xs6All.h"

#define XS_MAX_INDEX ((2 << 28) - 2)
#define mxPop() (the->stack++)

static txBoolean fxCallThisItem(txMachine* the, txSlot* function, txIndex index, txSlot* item);
static txSlot* fxCheckArray(txMachine* the, txSlot* slot);
static int fxCompareArrayItem(txMachine* the, txSlot* function, txSlot* array, txInteger i);
static txSlot* fxCreateArray(txMachine* the, txFlag flag, txIndex length);
static txSlot* fxCreateArraySpecies(txMachine* the, txNumber length);
static void fxFindThisItem(txMachine* the, txSlot* function, txIndex index, txSlot* item);
static txNumber fxGetArrayLength(txMachine* the, txSlot* reference);
static void fxMoveThisItem(txMachine* the, txNumber from, txNumber to);
static void fxReduceThisItem(txMachine* the, txSlot* function, txIndex index);

static void fx_Array(txMachine* the);
static void fx_Array_from(txMachine* the);
static void fx_Array_from_aux(txMachine* the, txSlot* function, txIndex index);
static void fx_Array_isArray(txMachine* the);
static void fx_Array_of(txMachine* the);
static void fx_Array_prototype_concat(txMachine* the);
static void fx_Array_prototype_copyWithin(txMachine* the);
static void fx_Array_prototype_entries(txMachine* the);
static void fx_Array_prototype_every(txMachine* the);
static void fx_Array_prototype_fill(txMachine* the);
static void fx_Array_prototype_filter(txMachine* the);
static void fx_Array_prototype_find(txMachine* the);
static void fx_Array_prototype_findIndex(txMachine* the);
static void fx_Array_prototype_forEach(txMachine* the);
static void fx_Array_prototype_includes(txMachine* the);
static void fx_Array_prototype_indexOf(txMachine* the);
static void fx_Array_prototype_join(txMachine* the);
static void fx_Array_prototype_keys(txMachine* the);
static void fx_Array_prototype_lastIndexOf(txMachine* the);
static void fx_Array_prototype_map(txMachine* the);
static void fx_Array_prototype_pop(txMachine* the);
static void fx_Array_prototype_push(txMachine* the);
static void fx_Array_prototype_reduce(txMachine* the);
static void fx_Array_prototype_reduceRight(txMachine* the);
static void fx_Array_prototype_reverse(txMachine* the);
static void fx_Array_prototype_shift(txMachine* the);
static void fx_Array_prototype_slice(txMachine* the);
static void fx_Array_prototype_some(txMachine* the);
static void fx_Array_prototype_sort(txMachine* the);
static void fx_Array_prototype_splice(txMachine* the);
static void fx_Array_prototype_toString(txMachine* the);
static void fx_Array_prototype_unshift(txMachine* the);
static void fx_Array_prototype_values(txMachine* the);

static void fx_ArrayIterator_prototype_next(txMachine* the);

static void fx_Arguments_prototype_callee(txMachine* the);
static void fx_Arguments_prototype_caller(txMachine* the);

void fxBuildArray(txMachine* the)
{
    static const txHostFunctionBuilder gx_Array_prototype_builders[] = {
		{ fx_Array_prototype_concat, 1, _concat },
		{ fx_Array_prototype_copyWithin, 2, _copyWithin },
		{ fx_Array_prototype_entries, 0, _entries },
		{ fx_Array_prototype_every, 1, _every },
		{ fx_Array_prototype_fill, 1, _fill },
		{ fx_Array_prototype_filter, 1, _filter },
		{ fx_Array_prototype_find, 1, _find },
		{ fx_Array_prototype_findIndex, 1, _findIndex },
		{ fx_Array_prototype_forEach, 1, _forEach },
		{ fx_Array_prototype_includes, 1, _includes },
		{ fx_Array_prototype_indexOf, 1, _indexOf },
		{ fx_Array_prototype_join, 1, _join },
		{ fx_Array_prototype_keys, 0, _keys },
		{ fx_Array_prototype_lastIndexOf, 1, _lastIndexOf },
		{ fx_Array_prototype_map, 1, _map },
		{ fx_Array_prototype_pop, 0, _pop },
		{ fx_Array_prototype_push, 1, _push },
		{ fx_Array_prototype_reduce, 1, _reduce },
		{ fx_Array_prototype_reduceRight, 1, _reduceRight },
		{ fx_Array_prototype_reverse, 0, _reverse },
		{ fx_Array_prototype_shift, 0, _shift },
		{ fx_Array_prototype_slice, 2, _slice },
		{ fx_Array_prototype_some, 1, _some },
		{ fx_Array_prototype_sort, 1, _sort },
		{ fx_Array_prototype_splice, 2, _splice },
		{ fx_Array_prototype_toString, 0, _toString },
		{ fx_Array_prototype_toString, 0, _toLocaleString },
		{ fx_Array_prototype_unshift, 1, _unshift },
		{ fx_Array_prototype_values, 0, _values },
 		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_Array_builders[] = {
		{ fx_Array_from, 1, _from },
		{ fx_Array_isArray, 1, _isArray },
		{ fx_Array_of, 0, _of },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	txSlot* property;
	txSlot* unscopable;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewArrayInstance(the));
	for (builder = gx_Array_prototype_builders; builder->callback; builder++) {
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
		if (builder->id == _values)
			property = slot;
	}
	mxPushSlot(property);
	mxPull(mxArrayIteratorFunction);
	slot = fxNextSlotProperty(the, slot, property, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	slot = fxNextBooleanProperty(the, slot, 1, mxID(_Symbol_isConcatSpreadable), XS_DONT_ENUM_FLAG);
	unscopable = fxLastProperty(the, fxNewInstance(the));
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_find), XS_NO_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_findIndex), XS_NO_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_fill), XS_NO_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_copyWithin), XS_NO_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_entries), XS_NO_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_keys), XS_NO_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_values), XS_NO_FLAG);
	slot = fxNextSlotProperty(the, slot, the->stack++, mxID(_Symbol_unscopables), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxArrayPrototype = *the->stack;
	slot = fxNewHostConstructorGlobal(the, fx_Array, 1, mxID(_Array), XS_DONT_ENUM_FLAG);
	mxArrayConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	for (builder = gx_Array_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_species_get, C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_ArrayIterator_prototype_next, 0, mxID(_next), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Array Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxArrayIteratorPrototype);
	
	mxPush(mxObjectPrototype);
	mxArgumentsSloppyPrototype = *the->stack;
	the->stack++;
	
	mxPush(mxObjectPrototype);
	mxArgumentsStrictPrototype = *the->stack;
	the->stack++;
}

txNumber fxArgToIndex(txMachine* the, txInteger argi, txNumber index, txNumber length)
{
	if ((mxArgc > argi) && (mxArgv(argi)->kind != XS_UNDEFINED_KIND)) {
		txNumber i = c_trunc(fxToNumber(the, mxArgv(argi)));
		if (c_isnan(i))
			i = 0;
		if (i < 0) {
			i = length + i;
			if (i < 0)
				i = 0;
		}
		else if (i > length)
			i = length;
		index = i;
	}
	return index;
}

txNumber fxArgToRange(txMachine* the, txInteger argi, txNumber index, txNumber min, txNumber max)
{
	if ((mxArgc > argi) && (mxArgv(argi)->kind != XS_UNDEFINED_KIND)) {
		txNumber i = c_trunc(fxToNumber(the, mxArgv(argi)));
		if (c_isnan(i))
			i = min;
		if (i <= min)
			i = min;
		else if (i > max)
			i = max;
		index = i;
	}
	return index;
}

void fxCacheArray(txMachine* the, txSlot* instance)
{
	txSlot* array = instance->next;
	txIndex length = array->value.array.length;
	if (length) {
		txSlot* address = (txSlot *)fxNewChunk(the, length * sizeof(txSlot));
		txSlot* srcSlot = array->next->next;
		txSlot* dstSlot = address;
		txIndex index = 0;
		while (srcSlot) {
			*((txIndex*)dstSlot) = index;	
			dstSlot->ID = XS_NO_ID;
			dstSlot->flag = XS_NO_FLAG;
			dstSlot->kind = srcSlot->kind;
			dstSlot->value = srcSlot->value;
			srcSlot = srcSlot->next;
			dstSlot++;
			index++;
		}
		array->value.array.address = address;
		array->next->next = C_NULL;
		if ((array->flag & XS_INTERNAL_FLAG) && (array->kind == XS_ARRAY_KIND))
			array->next->value.number = array->value.array.length;
	}
}

txBoolean fxCallThisItem(txMachine* the, txSlot* function, txIndex index, txSlot* item)
{
	mxPushSlot(mxThis);
	if (fxHasIndex(the, index)) {
		/* ARG0 */
		mxPushSlot(mxThis);
		fxGetIndex(the, index);
		if (item) {
			item->kind = the->stack->kind;
			item->value = the->stack->value;
		}
		/* ARG1 */
		mxPushUnsigned(index);
		/* ARG2 */
		mxPushSlot(mxThis);
		/* ARGC */
		mxPushInteger(3);
		/* THIS */
		if (mxArgc > 1)
			mxPushSlot(mxArgv(1));
		else
			mxPushUndefined();
		/* FUNCTION */
		mxPushReference(function);
		fxCall(the);
		return 1;
	}
	return 0;
}

txSlot* fxCheckArray(txMachine* the, txSlot* slot)
{
	txSlot* instance = fxToInstance(the, slot);
	txSlot* array = instance->next;
	if (array && (array->flag & XS_INTERNAL_FLAG) && (array->kind == XS_ARRAY_KIND)) {
		txSlot* address = array->value.array.address;
		txIndex size = (address) ? (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot) : 0;
		if (array->value.array.length == size) {
			return array;
		}
	}
	return C_NULL;
}

txIndex fxCheckArrayLength(txMachine* the, txSlot* slot)
{
again:
	if (slot->kind == XS_INTEGER_KIND) {
		if (slot->value.integer >= 0)
			return (txIndex)slot->value.integer;
	}
	else if (slot->kind == XS_NUMBER_KIND) {
		txIndex length = (txIndex)slot->value.number;
		txNumber check = length;
		if (slot->value.number == check)
			return length;
	}
	else {
		fxToNumber(the, slot);
		goto again;
	}
	mxRangeError("invalid length");
	return 0;
}

int fxCompareArrayItem(txMachine* the, txSlot* function, txSlot* array, txInteger i)
{
	txSlot* address = array->value.array.address;
	txSlot* a = address + i;
	txSlot* b = the->stack;
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
		mxPushSlot(a);
		mxPushSlot(b);
		if (function) {
			/* ARGC */
			mxPushInteger(2);
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushReference(function);
			fxCall(the);
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

void fxConstructArrayEntry(txMachine* the, txSlot* entry)
{
	txSlot* value = the->stack;
	txSlot* key = the->stack + 1;
	txSlot* instance;
	txSlot* array;
	txSlot* item;
	mxPush(mxArrayPrototype);
	instance = fxNewArrayInstance(the);
	array = instance->next;
	fxSetArraySize(the, array, 2);
	item = array->value.array.address;
	*((txIndex*)item) = 0;
	item->ID = XS_NO_ID;
	item->kind = key->kind;
	item->value = key->value;
	item++;
	*((txIndex*)item) = 1;
	item->ID = XS_NO_ID;
	item->kind = value->kind;
	item->value = value->value;
	entry->kind = the->stack->kind;
	entry->value = the->stack->value;
	the->stack += 3;
}

txSlot* fxCreateArray(txMachine* the, txFlag flag, txIndex length)
{
	if (length) {
		mxPushUnsigned(length);
		mxPushInteger(1);
	}
	else
		mxPushInteger(0);
	if (mxIsReference(mxThis) && mxIsFunction(mxThis->value.reference)) {
		mxPushSlot(mxThis);
		if (the->stack->value.reference != mxArrayConstructor.value.reference)
			flag = 0;
	}
	else
		mxPush(mxArrayConstructor);
	fxNew(the);
	mxPullSlot(mxResult);
	if (flag)
		fxSetArraySize(the, mxResult->value.reference->next, length);
	return fxCheckArray(the, mxResult);
}

txSlot* fxCreateArraySpecies(txMachine* the, txNumber length)
{
	txFlag flag = 1;
	txSlot* instance = fxToInstance(the, mxThis);
	mxPushNumber(length);
	mxPushInteger(1);
	if (fxIsArray(the, instance)) {
		mxPushSlot(mxThis);
		fxGetID(the, mxID(_constructor));
		if (mxIsReference(the->stack))
			fxGetID(the, mxID(_Symbol_species));
		else
			the->stack->kind = XS_UNDEFINED_KIND;
	}
	else
		mxPushUndefined();
	if ((the->stack->kind == XS_UNDEFINED_KIND) || (the->stack->kind == XS_NULL_KIND))
		*the->stack = mxArrayConstructor;
	else if (the->stack->value.reference != mxArrayConstructor.value.reference)
		flag = 0;
	fxNew(the);
	mxPullSlot(mxResult);
	if (flag && length)
		fxSetArraySize(the, mxResult->value.reference->next, (txIndex)length);
	return fxCheckArray(the, mxResult);
}

txBoolean fxDeleteArrayProperty(txMachine* the, txSlot* array, txIndex index) 
{
	txSlot* address = array->value.array.address;
	txSlot* chunk = address;
	if (address) {
		txIndex length = array->value.array.length;
		txIndex size = (((txChunk*)(((txByte*)chunk) - sizeof(txChunk)))->size) / sizeof(txSlot);
		txSlot* result = address;
		txSlot* limit = result + size;
		if (length == size)
			result = address + index;
		else {
			txIndex at;
			while (result < limit) {
				at = *((txIndex*)result);
				if (at == index)
					break;
				if (at > index)
					return 1;
				result++;
			}
		}
		if (result < limit) {
			if (result->flag & XS_DONT_DELETE_FLAG)
				return 0;
			size--;
			if (size > 0) {
				chunk = (txSlot*)fxNewChunk(the, size * sizeof(txSlot));
				if (result > address)
					c_memcpy(chunk, address, (result - address) * sizeof(txSlot));
				result++;
				if (result < limit)
					c_memcpy(chunk + (result - address - 1), result, (limit - result) * sizeof(txSlot));
				array->value.array.address = chunk;
			}
			else
				array->value.array.address = C_NULL;
		}
	}
	return 1;
}

void fxFindThisItem(txMachine* the, txSlot* function, txIndex index, txSlot* item)
{
	/* ARG0 */
	mxPushSlot(mxThis);
	fxGetIndex(the, index);
	if (item) {
		item->kind = the->stack->kind;
		item->value = the->stack->value;
	}
	/* ARG1 */
	mxPushUnsigned(index);
	/* ARG2 */
	mxPushSlot(mxThis);
	/* ARGC */
	mxPushInteger(3);
	/* THIS */
	if (mxArgc > 1)
		mxPushSlot(mxArgv(1));
	else
		mxPushUndefined();
	/* FUNCTION */
	mxPushReference(function);
	fxCall(the);
}

txNumber fxGetArrayLength(txMachine* the, txSlot* reference)
{
	txNumber length;
	mxPushReference(fxToInstance(the, reference));
	fxGetID(the, mxID(_length));
	length = fxToLength(the, the->stack);
	mxPop();
	return length;
}

txSlot* fxGetArrayProperty(txMachine* the, txSlot* array, txIndex index) 
{
	txSlot* address = array->value.array.address;
	if (address) {
		txIndex length = array->value.array.length;
		txIndex size = (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot);
		if (length == size) {
			if (index < length)
				return address + index;
		}
		else {
			txSlot* result = address;
			txSlot* limit = result + size;
			txIndex at;
			while (result < limit) {
				at = *((txIndex*)result);
				if (at == index)
					return result;
				result++;
			}
		}
	}
	return C_NULL;
}

txIndex fxGetArraySize(txMachine* the, txSlot* array)
{
	txSlot* address = array->value.array.address;
	if (address)
		return (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot);
	return 0;
}

void fxIndexArray(txMachine* the, txSlot* array) 
{
	txSlot* address = array->value.array.address;
	if (address) {
		txIndex size = (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot);
		txIndex index = 0;
		while (index < size) {
			*((txIndex*)address) = index;	
			address++;
			index++;
		}
	}
}

txBoolean fxIsArray(txMachine* the, txSlot* instance) 
{
again:
	if (instance) {
		txSlot* internal = instance->next;
		if (internal && (internal->flag & XS_INTERNAL_FLAG)) {
			if (internal->kind == XS_ARRAY_KIND)
				return 1;
			if (internal->kind == XS_PROXY_KIND) {
				instance = internal->value.proxy.target;
				goto again;
			}
		}
	}
	return 0;
}

void fxMoveThisItem(txMachine* the, txNumber from, txNumber to)
{
	mxPushSlot(mxThis);
	mxPushNumber(from);
	if (fxHasAt(the)) {
		mxPushSlot(mxThis);
		mxPushNumber(from);
		fxGetAt(the);
		mxPushSlot(mxThis);
		mxPushNumber(to);
		fxSetAt(the);
		mxPop();
	}
	else {
		mxPushSlot(mxThis);
		mxPushNumber(to);
		fxDeleteAt(the);
		mxPop();
	}
}

txSlot* fxNewArrayInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	property->kind = XS_ARRAY_KIND;
	property->value.array.length = 0;
	property->value.array.address = C_NULL;
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	property->ID = mxID(_length);
	property->kind = XS_NUMBER_KIND;
	property->value.number = 0;
	return instance;
}

void fxReduceThisItem(txMachine* the, txSlot* function, txIndex index)
{
	mxPushSlot(mxThis);
	if (fxHasIndex(the, index)) {
		/* ARG0 */
		mxPushSlot(mxResult);
		/* ARG1 */
		mxPushSlot(mxThis);
		fxGetIndex(the, index);
		/* ARG2 */
		mxPushUnsigned(index);
		/* ARG3 */
		mxPushSlot(mxThis);
		/* ARGC */
		mxPushInteger(4);
		/* THIS */
		mxPushUndefined();
		/* FUNCTION */
		mxPushReference(function);
		fxCall(the);
		mxPullSlot(mxResult);
	}
}

txSlot* fxSetArrayLength(txMachine* the, txSlot* array, txIndex length)
{
	txSlot* address = array->value.array.address;
	txSlot* chunk = address;
	txBoolean success = 1;
	if (address) {
		txSize size = (((txChunk*)(((txByte*)chunk) - sizeof(txChunk)))->size) / sizeof(txSlot);
		txSlot* result = address + size;
		txSlot* limit = result;
		txIndex at;
		while (result > address) {
			result--;
			at = *((txIndex*)result);
			if (length > at) {
				result++;
				break;
			}
			else if (result->flag & XS_DONT_DELETE_FLAG) {
				result++;
				length = at + 1;
				success = 0;
				break;
			}
		}
		if (result < limit) {
			if (result > address) {
				size = result - address;
				chunk = (txSlot*)fxNewChunk(the, size * sizeof(txSlot));
				address = array->value.array.address;
				c_memcpy(chunk, address, size * sizeof(txSlot));
			}
			else
				chunk = C_NULL;
		}	
	}
	array->value.array.length = length;
	array->value.array.address = chunk;
	if ((array->flag & XS_INTERNAL_FLAG) && (array->kind == XS_ARRAY_KIND))
		array->next->value.number = array->value.array.length;
	if (success) {
		the->scratch.kind = XS_UNDEFINED_KIND;
		return &the->scratch;
	}
	return C_NULL;
}

txSlot* fxSetArrayProperty(txMachine* the, txSlot* instance, txSlot* array, txIndex index) 
{
	txSlot* property;
	txSlot* address = array->value.array.address;
	txSlot* chunk = address;
	txIndex length;
	txIndex size;
	txSlot* result;
	txSlot* limit;
	txIndex at;
	property = ((array->flag & XS_INTERNAL_FLAG) && (array->kind == XS_ARRAY_KIND)) ? array->next : C_NULL;
	if (address) {
		length = array->value.array.length;
		size = (((txChunk*)(((txByte*)chunk) - sizeof(txChunk)))->size) / sizeof(txSlot);
		if (length == size) {
			if (index < length) 
				return address + index;
			if (instance->flag & XS_DONT_PATCH_FLAG)
				return C_NULL;
			if (property && (property->flag & XS_DONT_SET_FLAG))
				return C_NULL;
			size++;
			chunk = (txSlot*)fxRenewChunk(the, address, size * sizeof(txSlot));
			if (!chunk) {
				chunk = (txSlot*)fxNewChunk(the, size * sizeof(txSlot));
				address = array->value.array.address;
				c_memcpy(chunk, address, length * sizeof(txSlot));
			}
			result = chunk + length;
		}
		else {
			result = address;
			limit = result + size;
			while (result < limit) {
				at = *((txIndex*)result);
				if (at == index)
					return result;
				if (at > index)
					break;
				result++;
			}
			if (instance->flag & XS_DONT_PATCH_FLAG)
				return C_NULL;
			if (property && (property->flag & XS_DONT_SET_FLAG) && (index >= array->value.array.length))
				return C_NULL;
			size++;
			at = result - address;
			chunk = (txSlot*)fxNewChunk(the, size * sizeof(txSlot));
			address = array->value.array.address;
			result = address + at;
			limit = address + size - 1;
			if (result > address)
				c_memcpy(chunk, address, (result - address) * sizeof(txSlot));
			if (result < limit)
				c_memcpy(chunk + (result - address) + 1, result, (limit - result) * sizeof(txSlot));
			result = chunk + (result - address);
		}
	}
	else {
        if (instance->flag & XS_DONT_PATCH_FLAG)
            return C_NULL;
		if (property && (property->flag & XS_DONT_SET_FLAG) && (index >= array->value.array.length))
			return C_NULL;
		size = 1;
		chunk = (txSlot*)fxNewChunk(the, sizeof(txSlot));
		result = chunk;
	}
	result->next = C_NULL;
	result->ID = XS_NO_ID;
	result->flag = XS_NO_FLAG;
	result->kind = XS_UNDEFINED_KIND;
	*((txIndex*)result) = index;	
	array->value.array.address = chunk;
	if (index >= array->value.array.length) {
		array->value.array.length = index + 1;
		if (property)
			property->value.number = array->value.array.length;
	}
	return result;
}

void fxSetArraySize(txMachine* the, txSlot* array, txIndex target)
{
	txSlot* address = array->value.array.address;
	txSlot* chunk = C_NULL;
	txIndex size = (address) ? (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot) : 0;
	if (size != target) {
		if ((array->flag & XS_INTERNAL_FLAG) && (array->next->flag & XS_DONT_SET_FLAG))
			mxTypeError("set length: not writable");
		if (address) {
			if (target) {
				chunk = (txSlot*)fxRenewChunk(the, address, target * sizeof(txSlot));
				if (!chunk) {
					chunk = (txSlot*)fxNewChunk(the, target * sizeof(txSlot));
					address = array->value.array.address;
					if (size < target)
						c_memcpy(chunk, address, size * sizeof(txSlot));
					else
						c_memcpy(chunk, address, target * sizeof(txSlot));
				}
			}
		}
		else {
			chunk = (txSlot*)fxNewChunk(the, target * sizeof(txSlot));
		}
		if (size < target)
			c_memset(chunk + size, 0, (target - size) * sizeof(txSlot));
		array->value.array.length = target;
		array->value.array.address = chunk;
		if (array->flag & XS_INTERNAL_FLAG)
			array->next->value.number = array->value.array.length;
	}
}

txNumber fxToLength(txMachine* the, txSlot* slot)
{
again:
	if (slot->kind == XS_INTEGER_KIND) {
		txInteger length = slot->value.integer;
		if (length < 0)
			length = 0;
		return (txNumber)length;
	}
	if (slot->kind == XS_NUMBER_KIND) {
		txNumber length = slot->value.number;
		if (c_isnan(length))
			length = 0;
		else if (length <= 0)
			length = 0;
		else if (length > C_MAX_SAFE_INTEGER)
			length = C_MAX_SAFE_INTEGER;
		else
			length = c_trunc(length);
		return length;
	}
	fxToNumber(the, slot);
	goto again;
}

void fx_Array(txMachine* the)
{
	txSlot* instance;
	txSlot* array;
	txSlot* argument;
	txIndex count = (txIndex)mxArgc;
	txBoolean flag = 0;
	txIndex index = 0;
	txSlot* slot;
	if (mxTarget->kind == XS_UNDEFINED_KIND) {
		mxPush(mxArrayPrototype);
		instance = fxNewArrayInstance(the);
		mxPullSlot(mxResult);
		array = instance->next;	
	}
	else
		array = fxCheckArray(the, mxThis);
	if (count == 1) {
		argument = mxArgv(0);
		if ((argument->kind == XS_INTEGER_KIND) || (argument->kind == XS_NUMBER_KIND)) {
			count = fxCheckArrayLength(the, mxArgv(0));
			flag = 1;
		}
	}
	if (array) {
		if (flag)
			fxSetArrayLength(the, array, count);
		else {
			fxSetArraySize(the, array, count);
			slot = array->value.array.address;
			while (index < count) {
				argument = mxArgv(index);
				*((txIndex*)slot) = index;
				slot->ID = XS_NO_ID;
				slot->kind = argument->kind;
				slot->value = argument->value;
				slot++;
				index++;
			}
		}
	}
	else {
		mxPushNumber(count);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_length));
		mxPop();
		if (!flag) {
			while (index < count) {
				mxPushSlot(mxArgv(index));
				mxPushSlot(mxThis);
				fxDefineIndex(the, index, 0, XS_GET_ONLY);
				mxPop();
				index++;
			}
		}
	}
}

void fx_Array_from(txMachine* the)
{
	txSlot* resultArray;
	txSlot* function = (mxArgc > 1) ? fxArgToCallback(the, 1) : C_NULL;
	txIndex length = 0;
	if (mxArgc > 0) {
		mxPushSlot(mxArgv(0));
		fxGetID(the, mxID(_Symbol_iterator));
		if (mxIsUndefined(the->stack)) {
			txIndex index = 0;
			mxPushSlot(mxArgv(0));
			fxGetID(the, mxID(_length));
			if (mxIsUndefined(the->stack))
				length = 0;
			else
				length = fxCheckArrayLength(the, the->stack);
			mxPop();
			fxCreateArray(the, 1, length);
			while (index < length) {
				mxPushSlot(mxArgv(0));
				fxGetIndex(the, index);
				fx_Array_from_aux(the, function, index);
				mxPushSlot(mxResult);
				fxDefineIndex(the, index, 0, XS_GET_ONLY);
				mxPop();
				index++;
			}
		}
		else {
			txSlot* iterator;
			txSlot* result;
			fxCreateArray(the, 0, 0);
			mxPushInteger(0);
			mxPushSlot(mxArgv(0));
			fxCallID(the, mxID(_Symbol_iterator));
			iterator = the->stack;
			{
				mxTry(the) {
					length = 0;
					for(;;) {
						mxCallID(iterator, mxID(_next), 0);
						result = the->stack;
						mxGetID(result, mxID(_done));
						if (fxToBoolean(the, the->stack))
							break;
						the->stack++;
						mxGetID(result, mxID(_value));
						fx_Array_from_aux(the, function, length);
						mxPushSlot(mxResult);
						fxDefineIndex(the, length, 0, XS_GET_ONLY);
						mxPop();
						length++;
					}
				}
				mxCatch(the) {
					fxCloseIterator(the, iterator);
					fxJump(the);
				}
			}
			the->stack++;
		}
	}
	else {
		resultArray = fxCreateArray(the, 1, length);
	}
	mxPushUnsigned(length);
	mxPushSlot(mxResult);
	fxSetID(the, mxID(_length));
	mxPop();
}

void fx_Array_from_aux(txMachine* the, txSlot* function, txIndex index)
{
	if (function) {
		/* ARG1 */
		mxPushInteger(index);
		/* ARGC */
		mxPushInteger(2);
		/* THIS */
		if (mxArgc > 2)
			mxPushSlot(mxArgv(2));
		else
			mxPushUndefined();
		/* FUNCTION */
		mxPushReference(function);
		fxCall(the);
	}
}

void fx_Array_isArray(txMachine* the)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (mxArgc > 0) ? fxIsArray(the, fxGetInstance(the, mxArgv(0))) : 0;
}

void fx_Array_of(txMachine* the)
{
	txIndex count = (txIndex)mxArgc, index = 0;
	txSlot* array = fxCreateArray(the, 1, count);
	if (array) {
		txSlot* slot;
		fxSetArraySize(the, array, count);
		slot = array->value.array.address;
		while (index < count) {
			txSlot* argument = mxArgv(index);
			*((txIndex*)slot) = index;
			slot->ID = XS_NO_ID;
			slot->kind = argument->kind;
			slot->value = argument->value;
			slot++;
			index++;
		}
	}
	else {
		while (index < count) {
			mxPushSlot(mxArgv(index));
			mxPushSlot(mxResult);
			fxDefineIndex(the, index, 0, XS_GET_ONLY);
			mxPop();
			index++;
		}
		mxPushInteger(count);
		mxPushSlot(mxResult);
		fxSetID(the, mxID(_length));
		mxPop();
	}
}

void fx_Array_prototype_concat(txMachine* the)
{
	txSlot* resultArray = fxCreateArraySpecies(the, 0);
	txIndex resultLength = 0;
	txInteger c = mxArgc;
	txInteger i = -1;
	if (resultArray) {
		txSlot* list = fxNewInstance(the);
		txSlot* slot = list;
		txSlot* resultSlot;
		txIndex holeCount = 0;
		mxPushSlot(mxThis);
		for (;;) {
			txSlot* argument = the->stack;
			txBoolean flag = 0;
			if (mxIsReference(argument)) {
				mxPushSlot(argument);
				fxGetID(the, mxID(_Symbol_isConcatSpreadable));
				if (mxIsUndefined(the->stack))
					flag = fxIsArray(the, argument->value.reference);
				else
					flag = fxToBoolean(the, the->stack);
				mxPop();
			}
			if (flag) {	
				txIndex length, index;
				mxPushSlot(argument);
				fxGetID(the, mxID(_length));
				length = (txIndex)fxToLength(the, the->stack);
				mxPop();
				index = 0;
				while (index < length) {
					mxPushSlot(argument);
					if (fxHasIndex(the, index)) {
						mxPushSlot(argument);
						fxGetIndex(the, index);
						slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
						mxPop();
					}
					else {
						slot = slot->next = fxNewSlot(the);
						slot->kind = XS_UNINITIALIZED_KIND;
						holeCount++;
					}
					index++;
					resultLength++;
				}
			}
			else {
				slot = fxNextSlotProperty(the, slot, argument, XS_NO_ID, XS_NO_FLAG);
				resultLength++;
			}
			mxPop();
			i++;
			if (i == c)
				break;
			mxPushSlot(mxArgv(i));
		}
		fxSetArraySize(the, resultArray, resultLength - holeCount);
		resultArray->value.array.length = resultLength;
		resultArray->next->value.number = resultLength;
		resultSlot = resultArray->value.array.address;
		slot = list->next;
		resultLength = 0;
		while (slot) {
			if (slot->kind != XS_UNINITIALIZED_KIND) {
				*((txIndex*)resultSlot) = resultLength;
				resultSlot->ID = XS_NO_ID;
				resultSlot->kind = slot->kind;
				resultSlot->value = slot->value;
				resultSlot++;
			}
			slot = slot->next;
			resultLength++;
		}
		mxPop();
	}
	else {
		mxPushSlot(mxThis);
		for (;;) {
			txSlot* argument = the->stack;
			txBoolean flag = 0;
			if (mxIsReference(argument)) {
				mxPushSlot(argument);
				fxGetID(the, mxID(_Symbol_isConcatSpreadable));
				if (mxIsUndefined(the->stack))
					flag = fxIsArray(the, argument->value.reference);
				else
					flag = fxToBoolean(the, the->stack);
				mxPop();
			}
			if (flag) {	
				txIndex length, index;
				mxPushSlot(argument);
				fxGetID(the, mxID(_length));
				length = (txIndex)fxToLength(the, the->stack);
				mxPop();
				index = 0;
				while (index < length) {
					mxPushSlot(argument);
					if (fxHasIndex(the, index)) {
						mxPushSlot(argument);
						fxGetIndex(the, index);
						mxPushSlot(mxResult);
						fxDefineIndex(the, resultLength, 0, XS_GET_ONLY);
						mxPop();
					}
					index++;
					resultLength++;
				}
			}
			else {
				mxPushSlot(argument);
				mxPushSlot(mxResult);
				fxDefineIndex(the, resultLength, 0, XS_GET_ONLY);
				mxPop();
				resultLength++;
			}
			mxPop();
			i++;
			if (i == c)
				break;
			mxPushSlot(mxArgv(i));
		}
		mxPushInteger(resultLength);
		mxPushSlot(mxResult);
		fxSetID(the, mxID(_length));
		mxPop();
	}
}

void fx_Array_prototype_copyWithin(txMachine* the)
{
	txSlot* array = fxCheckArray(the, mxThis);
	if (array) {
		txIndex length = array->value.array.length;
		txIndex to = (txIndex)fxArgToIndex(the, 0, 0, length);
		txIndex from = (txIndex)fxArgToIndex(the, 1, 0, length);
		txIndex final = (txIndex)fxArgToIndex(the, 2, length, length);
		txIndex count = (final > from) ? final - from : 0;
		if (count > length - to)
			count = length - to;
		if (count > 0) {
			c_memmove(array->value.array.address + to, array->value.array.address + from, count * sizeof(txSlot));
			fxIndexArray(the, array);
		}
	}
	else {
		txNumber length = fxGetArrayLength(the, mxThis);
		txNumber to = fxArgToIndex(the, 0, 0, length);
		txNumber from = fxArgToIndex(the, 1, 0, length);
		txNumber final = fxArgToIndex(the, 2, length, length);
		txNumber count = (final > from) ? final - from : 0;
		txNumber direction;
		if (count > length - to)
			count = length - to;
		if ((from < to) && (to < from + count)) {
			direction = -1;
			from += count - 1;
			to += count - 1;
		}
		else
			direction = 1;
		while (count > 0) {
			fxMoveThisItem(the, from, to);
			from += direction;
			to += direction;
			count--;
		}	
	}	
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_Array_prototype_entries(txMachine* the)
{
	txSlot* property;
	fxToInstance(the, mxThis);
	mxPush(mxArrayIteratorPrototype);
	property = fxLastProperty(the, fxNewIteratorInstance(the, mxThis));
	property = fxNextIntegerProperty(the, property, 2, XS_NO_ID, XS_INTERNAL_FLAG | XS_GET_ONLY);
	mxPullSlot(mxResult);
}

void fx_Array_prototype_every(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = 0;
	txSlot* function = fxArgToCallback(the, 0);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	while (index < length) {
		if (fxCallThisItem(the, function, index, C_NULL)) {
			mxResult->value.boolean = fxToBoolean(the, the->stack);
			mxPop();
			if (!mxResult->value.boolean)
				break;
		}
		index++;
	}
}

void fx_Array_prototype_fill(txMachine* the)
{
	txSlot* instance = fxToInstance(the, mxThis);
	txSlot* array = instance->next;
	txSlot* value;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	value = the->stack;
	if (array && (array->flag & XS_INTERNAL_FLAG) && (array->kind == XS_ARRAY_KIND)) {
		txIndex length = array->value.array.length;
		txIndex start = (txIndex)fxArgToIndex(the, 1, 0, length);
		txIndex end = (txIndex)fxArgToIndex(the, 2, length, length);
		txSlot* address;
		txIndex size;
		if ((start == 0) && (end == length)) {
			fxSetArraySize(the, array, length);
			fxIndexArray(the, array);
		}
		address = array->value.array.address;
		size = (address) ? (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot) : 0;
		if (length == size) {
			while (start < end) {
				txSlot* slot = array->value.array.address + start;
				slot->ID = XS_NO_ID;
				slot->kind = value->kind;
				slot->value = value->value;
				start++;
			}
		}
		else {
			while (start < end) {
				mxPushSlot(value);
				mxPushSlot(mxThis);
				fxSetIndex(the, start);
				mxPop();
				start++;
			}
		}
	}
	else {
		txNumber length = fxGetArrayLength(the, mxThis);
		txNumber start = fxArgToIndex(the, 1, 0, length);
		txNumber end = fxArgToIndex(the, 2, length, length);
		txSlot* value = the->stack;
		while (start < end) {
			mxPushSlot(value);
			mxPushSlot(mxThis);
			mxPushNumber(start);
			fxSetAt(the);
			mxPop();
			start++;
		}
	}
	mxPop();
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_Array_prototype_filter(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = 0;
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* resultArray = fxCreateArraySpecies(the, 0);
	txIndex resultLength = 0;
	txSlot* item;
	mxPushUndefined();
	item = the->stack;
	if (resultArray) {
		txSlot* list = fxNewInstance(the);
		txSlot* slot = list;
		txSlot* resultSlot;
		while (index < length) {
			if (fxCallThisItem(the, function, index, item)) {
				if (fxToBoolean(the, the->stack)) {
					slot = fxNextSlotProperty(the, slot, item, XS_NO_ID, XS_NO_FLAG);
					resultLength++;
				}
				mxPop();
			}
			index++;
		}
		fxSetArraySize(the, resultArray, resultLength);
		resultSlot = resultArray->value.array.address;
		slot = list->next;
		index = 0;
		while (slot) {
			*((txIndex*)resultSlot) = index;
			resultSlot->ID = XS_NO_ID;
			resultSlot->kind = slot->kind;
			resultSlot->value = slot->value;
			resultSlot++;
			slot = slot->next;
			index++;
		}
		mxPop();
	}
	else {
		while (index < length) {
			if (fxCallThisItem(the, function, index, item)) {
				if (fxToBoolean(the, the->stack)) {
					mxPushSlot(item);
					mxPushSlot(mxResult);
					fxDefineIndex(the, resultLength, 0, XS_GET_ONLY);
					mxPop();
					resultLength++;
				}
				mxPop();
			}
			index++;
		}
	}
	mxPop();
}

void fx_Array_prototype_find(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = 0;
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* item;
	mxPushUndefined();
	item = the->stack;
	while (index < length) {
		fxFindThisItem(the, function, index, item);
		if (fxToBoolean(the, the->stack++)) {
			mxResult->kind = item->kind;
			mxResult->value = item->value;
			break;
		}
		index++;
	}
	mxPop();
}

void fx_Array_prototype_findIndex(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = 0;
	txSlot* function = fxArgToCallback(the, 0);
	fxInteger(the, mxResult, -1);
	while (index < length) {
		fxFindThisItem(the, function, index, C_NULL);
		if (fxToBoolean(the, the->stack++)) {
			fxUnsigned(the, mxResult, index);
			break;
		}
		index++;
	}
}

void fx_Array_prototype_forEach(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = 0;
	txSlot* function = fxArgToCallback(the, 0);
	while (index < length) {
		if (fxCallThisItem(the, function, index, C_NULL))
			mxPop();
		index++;
	}
}

void fx_Array_prototype_includes(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = (txIndex)fxArgToIndex(the, 1, 0, length);
	txSlot* argument;
	fxBoolean(the, mxResult, 0);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	argument = the->stack;
	while (index < length) {
		mxPushSlot(mxThis);
		fxGetIndex(the, index);
		if (fxIsSameValue(the, the->stack++, argument, 1)) {
			mxResult->value.boolean = 1;
			break;
		}
		index++;
	}
	mxPop();
}

void fx_Array_prototype_indexOf(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = (txIndex)fxArgToIndex(the, 1, 0, length);
	txSlot* argument;
	fxInteger(the, mxResult, -1);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	argument = the->stack;
	while (index < length) {
		mxPushSlot(mxThis);
		if (fxHasIndex(the, index)) {
			mxPushSlot(mxThis);
			fxGetIndex(the, index);
			if (fxIsSameSlot(the, the->stack++, argument)) {
				fxUnsigned(the, mxResult, index);
				break;
			}
		}
		index++;
	}
	mxPop();
}

void fx_Array_prototype_join(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = 0;
	txString string;
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txBoolean comma = 0;
	txInteger size = 0;
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
		mxPushSlot(mxArgv(0));
		string = fxToString(the, the->stack);
		the->stack->kind += XS_KEY_KIND - XS_STRING_KIND;
		the->stack->value.key.sum = c_strlen(string);
	}
	else {
		mxPushStringC(",");
		the->stack->kind += XS_KEY_KIND - XS_STRING_KIND;
		the->stack->value.key.sum = 1;
	}
	while (index < length) {
		if (comma) {
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			size += slot->value.key.sum;
		}
		else
			comma = 1;
		mxPushSlot(mxThis);
		fxGetIndex(the, index);
		if ((the->stack->kind != XS_UNDEFINED_KIND) && (the->stack->kind != XS_NULL_KIND)) {
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			string = fxToString(the, slot);
			slot->kind += XS_KEY_KIND - XS_STRING_KIND;
			slot->value.key.sum = c_strlen(string);
			size += slot->value.key.sum;
		}
		mxPop();
		index++;
	}
	string = mxResult->value.string = fxNewChunk(the, size + 1);
	slot = list->next;
	while (slot) {
		c_memcpy(string, slot->value.key.string, slot->value.key.sum);
		string += slot->value.key.sum;
		slot = slot->next;
	}
	*string = 0;
	mxResult->kind = XS_STRING_KIND;
	mxPop();
}

void fx_Array_prototype_keys(txMachine* the)
{
	txSlot* property;
	fxToInstance(the, mxThis);
	mxPush(mxArrayIteratorPrototype);
	property = fxLastProperty(the, fxNewIteratorInstance(the, mxThis));
	property = fxNextIntegerProperty(the, property, 1, XS_NO_ID, XS_INTERNAL_FLAG | XS_GET_ONLY);
	mxPullSlot(mxResult);
}

void fx_Array_prototype_lastIndexOf(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = (txIndex)fxArgToIndex(the, 1, length, length);
	txSlot* argument;
	fxInteger(the, mxResult, -1);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	argument = the->stack;
	if (index < length)
		index++;
	while (index > 0) {
		index--;
		mxPushSlot(mxThis);
		if (fxHasIndex(the, index)) {
			mxPushSlot(mxThis);
			fxGetIndex(the, index);
			if (fxIsSameSlot(the, the->stack++, argument)) {
				fxUnsigned(the, mxResult, index);
				break;
			}
		}
	}
	mxPop();
}

void fx_Array_prototype_map(txMachine* the)
{
	txNumber LENGTH = fxGetArrayLength(the, mxThis);
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* resultArray = fxCreateArraySpecies(the, LENGTH);
	txIndex length = (txIndex)LENGTH;
	txIndex index = 0;
	if (resultArray) {
		txIndex resultLength = 0;
		while (index < length) {
			if (fxCallThisItem(the, function, index, C_NULL)) {
				txSlot* slot = resultArray->value.array.address + resultLength;
				*((txIndex*)slot) = index;
				slot->ID = XS_NO_ID;
				slot->kind = the->stack->kind;
				slot->value = the->stack->value;
				resultLength++;
				mxPop();
			}
			index++;
		}
		if (resultLength < length) {
			fxSetArraySize(the, resultArray, resultLength);
			resultArray->value.array.length = length;
			resultArray->next->value.number = length;
		}
	}
	else {
		while (index < length) {
			if (fxCallThisItem(the, function, index, C_NULL)) {
				mxPushSlot(mxResult);
				fxDefineIndex(the, index, 0, XS_GET_ONLY);
				mxPop();
			}
			index++;
		}
	}
}

void fx_Array_prototype_pop(txMachine* the)
{
	txSlot* array = fxCheckArray(the, mxThis);
	if (array) {
		txIndex length = array->value.array.length;
		txSlot* address;
		if (length > 0) {
			length--;
			address = array->value.array.address + length;
			mxResult->kind = address->kind;
			mxResult->value = address->value;
			fxSetArraySize(the, array, length);
		}
	}
	else {
		txNumber length = fxGetArrayLength(the, mxThis);
		if (length > 0) {
			length--;
			mxPushSlot(mxThis);
			mxPushNumber(length);
			fxGetAt(the);
			mxPullSlot(mxResult);
			mxPushSlot(mxThis);
			mxPushNumber(length);
			fxDeleteAt(the);
			mxPop();
		}
		mxPushNumber(length);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_length));
		mxPop();
	}
}

void fx_Array_prototype_push(txMachine* the)
{
	txIndex c = mxArgc, i = 0;
	txSlot* array = fxCheckArray(the, mxThis);
	if (array) {
		txIndex length = array->value.array.length;
		txSlot* address;
		if (length + c < length)
			mxRangeError("array overflow");
		fxSetArraySize(the, array, length + c);
		address = array->value.array.address + length;
		while (i < c) {
			txSlot* argument = mxArgv(i);
			*((txIndex*)address) = length + i;
			address->ID = XS_NO_ID;
			address->kind = argument->kind;
			address->value = argument->value;
			address++;
			i++;
		}
		mxPushUnsigned(length + c);
	}
	else {
		txNumber length = fxGetArrayLength(the, mxThis);
		if (length + c > C_MAX_SAFE_INTEGER)
			mxTypeError("unsafe integer");
		while (i < c) {
			mxPushSlot(mxArgv(i));
			mxPushSlot(mxThis);
			mxPushNumber(length + i);
			fxSetAt(the);
			mxPop();
			i++;
		}
		mxPushNumber(length + c);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_length));
	}
	mxPullSlot(mxResult);
}

void fx_Array_prototype_reduce(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = 0;
	txSlot* function = fxArgToCallback(the, 0);
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else {
		txBoolean flag = 0;
		while (!flag && (index < length)) {
			mxPushSlot(mxThis);
			if (fxHasIndex(the, index)) {
				mxPushSlot(mxThis);
				fxGetIndex(the, index);
				mxPullSlot(mxResult);
				flag = 1;
			}
			index++;
		}
		if (!flag)
			mxTypeError("no initial value");
	}
	while (index < length) {
		fxReduceThisItem(the, function, index);
		index++;
	}
}

void fx_Array_prototype_reduceRight(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = length;
	txSlot* function = fxArgToCallback(the, 0);
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else {
		txBoolean flag = 0;
		while (!flag && (index > 0)) {
			index--;
			mxPushSlot(mxThis);
			if (fxHasIndex(the, index)) {
				mxPushSlot(mxThis);
				fxGetIndex(the, index);
				mxPullSlot(mxResult);
				flag = 1;
			}
		}
		if (!flag)
			mxTypeError("no initial value");
	}
	while (index > 0) {
		index--;
		fxReduceThisItem(the, function, index);
	}
}

void fx_Array_prototype_reverse(txMachine* the)
{
	txSlot* array = fxCheckArray(the, mxThis);
	if (array) {
		txIndex length = array->value.array.length;
		if (length > 0) {
			txSlot* first = array->value.array.address;
			txSlot* last = first + length - 1;
			while (first < last) {
				txSlot buffer = *last;
				*last = *first;
				*first = buffer;
				first++;
				last--;
			}
			fxIndexArray(the, array);
		}
	}
	else {
		txSlot* lowerSlot;
		txSlot* upperSlot;
		txNumber length = fxGetArrayLength(the, mxThis);
		txNumber middle = c_trunc(length / 2);
		txNumber lower = 0;
		while (lower != middle) {
			txNumber upper = length - lower - 1;
			mxPushSlot(mxThis);
			mxPushNumber(lower);
			if (fxHasAt(the)) {
				mxPushSlot(mxThis);
				mxPushNumber(lower);
				fxGetAt(the);
				lowerSlot = the->stack;
			}
			else
				lowerSlot = C_NULL;
			mxPushSlot(mxThis);
			mxPushNumber(upper);
			if (fxHasAt(the)) {
				mxPushSlot(mxThis);
				mxPushNumber(upper);
				fxGetAt(the);
				upperSlot = the->stack;
			}
			else
				upperSlot = C_NULL;
			if (upperSlot && lowerSlot) {
				mxPushSlot(upperSlot);
				mxPushSlot(mxThis);
				mxPushNumber(lower);
				fxSetAt(the);
				mxPop();
				mxPushSlot(lowerSlot);
				mxPushSlot(mxThis);
				mxPushNumber(upper);
				fxSetAt(the);
				mxPop();
				mxPop();
				mxPop();
			}
			else if (upperSlot) {
				mxPushSlot(upperSlot);
				mxPushSlot(mxThis);
				mxPushNumber(lower);
				fxSetAt(the);
				mxPop();
				mxPushSlot(mxThis);
				mxPushNumber(upper);
				fxDeleteAt(the);
				mxPop();
				mxPop();
			}
			else if (lowerSlot) {
				mxPushSlot(mxThis);
				mxPushNumber(lower);
				fxDeleteAt(the);
				mxPop();
				mxPushSlot(lowerSlot);
				mxPushSlot(mxThis);
				mxPushNumber(upper);
				fxSetAt(the);
				mxPop();
				mxPop();
			}
			lower++;
		}
	}
	*mxResult = *mxThis;
}

void fx_Array_prototype_shift(txMachine* the)
{
	txSlot* array = fxCheckArray(the, mxThis);
	if (array) {
		txIndex length = array->value.array.length;
		txSlot* address;
		if (length > 0) {
			address = array->value.array.address;
			length--;
			mxResult->kind = address->kind;
			mxResult->value = address->value;
			c_memmove(address, address + 1, length * sizeof(txSlot));
			fxSetArraySize(the, array, length);
			fxIndexArray(the, array);
		}
	}
	else {
		txNumber length = fxGetArrayLength(the, mxThis);
		if (length > 0) {
			txNumber index = 1;
			mxPushSlot(mxThis);
			fxGetIndex(the, 0);
			mxPullSlot(mxResult);
			while (index < length) {
				mxPushSlot(mxThis);
				mxPushNumber(index);
				if (fxHasAt(the)) {
					mxPushSlot(mxThis);
					mxPushNumber(index);
					fxGetAt(the);
					mxPushSlot(mxThis);
					mxPushNumber(index - 1);
					fxSetAt(the);
					mxPop();
				}
				else {
					mxPushSlot(mxThis);
					mxPushNumber(index - 1);
					fxDeleteAt(the);
					mxPop();
				}
				index++;
			}
			length--;
			mxPushSlot(mxThis);
			fxDeleteIndex(the, length);
			mxPop();
		}
		mxPushNumber(length);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_length));
		mxPop();
	}
}

void fx_Array_prototype_slice(txMachine* the)
{
	txNumber LENGTH = fxGetArrayLength(the, mxThis);
	txNumber START = fxArgToIndex(the, 0, 0, LENGTH);
	txNumber END = fxArgToIndex(the, 1, LENGTH, LENGTH);
	txNumber COUNT = (END > START) ? END - START : 0;
	txSlot* array = fxCheckArray(the, mxThis);
	txSlot* resultArray = fxCreateArraySpecies(the, COUNT);
	if (array && resultArray) {
		txIndex start = START;
		txIndex count = COUNT;
		if (count) {
			c_memcpy(resultArray->value.array.address, array->value.array.address + start, count * sizeof(txSlot));
			fxIndexArray(the, resultArray);
		}
	}
	else {
		txNumber INDEX = 0;
		while (START < END) {
			mxPushSlot(mxThis);
			mxPushNumber(INDEX);
			if (fxHasAt(the)) {
				mxPushSlot(mxThis);
				mxPushNumber(START);
				fxGetAt(the);
				mxPushSlot(mxResult);
				mxPushNumber(INDEX);
				fxDefineAt(the, 0, XS_GET_ONLY);
				mxPop();
			}
			INDEX++;
			START++;
		}
		mxPushNumber(COUNT);
		mxPushSlot(mxResult);
		fxSetID(the, mxID(_length));
	}
}

void fx_Array_prototype_some(txMachine* the)
{
	txIndex length = (txIndex)fxGetArrayLength(the, mxThis);
	txIndex index = 0;
	txSlot* function = fxArgToCallback(the, 0);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	while (index < length) {
		if (fxCallThisItem(the, function, index, C_NULL)) {
			mxResult->value.boolean = fxToBoolean(the, the->stack);
			mxPop();
			if (mxResult->value.boolean)
				break;
		}
		index++;
	}
}

void fx_Array_prototype_sort(txMachine* the)
{
	txSlot* array = fxCheckArray(the, mxThis);
	txSlot* function = C_NULL;
 	txNumber LENGTH;
 	txIndex index, length;
	txSlot* instance = C_NULL;
    txSlot* item;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind != XS_UNDEFINED_KIND) {
			slot = fxToInstance(the, slot);
			if ((slot->next->kind == XS_CODE_KIND) || (slot->next->kind == XS_CODE_X_KIND) || (slot->next->kind == XS_CALLBACK_KIND))
				function = slot;
			else
				mxTypeError("compare is no function");
		}
	}
	if (!array) {
		LENGTH = fxGetArrayLength(the, mxThis);
		if (LENGTH > 0xFFFFFFFF)
			mxRangeError("array overflow");
		mxPush(mxArrayPrototype);
		instance = fxNewArrayInstance(the);
		array = instance->next;
		item = array->next;
        index = 0;
		while (index < LENGTH) {
			mxPushSlot(mxThis);
			if (fxHasIndex(the, index)) {
				item->next = fxNewSlot(the);
				item = item->next;
				array->value.array.length++;
				mxPushSlot(mxThis);
				fxGetIndex(the, index);
				mxPullSlot(item);
			}
			index++;
		}
		fxCacheArray(the, instance);
	}
	length = array->value.array.length;
	/* like GCC qsort */
	#define COMPARE(INDEX) \
		fxCompareArrayItem(the, function, array, INDEX)
	#define COPY \
		to->ID = from->ID; \
		to->kind = from->kind; \
		to->value = from->value
	#define MOVE(FROM,TO) \
		from = array->value.array.address + (FROM); \
		to = array->value.array.address + (TO); \
		COPY
	#define PUSH(INDEX) \
		from = array->value.array.address + (INDEX); \
		mxPushUndefined(); \
		to = the->stack; \
		COPY
	#define PULL(INDEX) \
		from = the->stack++; \
		to = array->value.array.address + (INDEX); \
		COPY
	if (length > 0) {
		txIndex i, j;
		txSlot* from;
		txSlot* to;
		if (length > mxSortThreshold) {
			txIndex lo = 0, hi = length - 1;
			txSortPartition stack[mxSortStackSize];
			txSortPartition *top = stack + 1;
			while (stack < top) {
				txIndex mid = lo + ((hi - lo) >> 1);
				PUSH(mid);
				if (COMPARE(lo) > 0) {
					MOVE(lo, mid);
					PULL(lo);
					PUSH(mid);
				}
				if (COMPARE(hi) < 0) {
					MOVE(hi, mid);
					PULL(hi);
					PUSH(mid);
					if (COMPARE(lo) > 0) {
						MOVE(lo, mid);
						PULL(lo);
						PUSH(mid);
					}
				}
				i = lo + 1;
				j = hi - 1;
				do {
					while (COMPARE(i) < 0) i++;
					while (COMPARE(j) > 0) j--;
					if (i < j) {
						PUSH(i);
						MOVE(j, i);
						PULL(j);
						i++;
						j--;
					}
					else if (i == j) {
						i++;
						j--;
						break;
					}
				} while (i <= j);
				if ((j - lo) <= mxSortThreshold) {
					if ((hi - i) <= mxSortThreshold) {
						top--;
						lo = top->lo; 
						hi = top->hi;
					}
					else {
						lo = i;
					}
				}
				else if ((hi - i) <= mxSortThreshold) {
					hi = j;
				}
				else if ((j - lo) > (hi - i)) {
					top->lo = lo;
					top->hi = j; 
					top++;
					lo = i;
				}
				else {
					top->lo = i;
					top->hi = hi; 
					top++;
					hi = j;
				}
			}
		}
		for (i = 1; i < length; i++) {
			PUSH(i);
			for (j = i; (j > 0) && (COMPARE(j - 1) > 0); j--) {
				MOVE(j - 1, j);
			}	
			PULL(j);
		}
	}
	if (instance) {
		item = array->value.array.address;
		index = 0;
		while (index < length) {
			mxPushSlot(item);
			mxPushSlot(mxThis);
			fxSetIndex(the, index);
			mxPop();
			item++;
			index++;
		}
		while (index < LENGTH) {
			mxPushSlot(mxThis);
			fxDeleteIndex(the, index);
			index++;
		}
		mxPop();
	}
	else 
		fxIndexArray(the, array);
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_Array_prototype_splice(txMachine* the)
{
	txIndex c = (txIndex)mxArgc;
	txNumber LENGTH = fxGetArrayLength(the, mxThis);
	txNumber START = fxArgToIndex(the, 0, 0, LENGTH);
	txNumber INSERTIONS, DELETIONS;
	txSlot* array = fxCheckArray(the, mxThis);
	txSlot* resultArray;
	if (c == 0) {
		INSERTIONS = 0;
		DELETIONS = 0;
	}
	else if (c == 1) {
		INSERTIONS = 0;
		DELETIONS = LENGTH - START;
	}
	else {
		INSERTIONS = c - 2;
		DELETIONS = fxArgToRange(the, 1, 0, 0, LENGTH - START);
	}
	if (LENGTH + INSERTIONS - DELETIONS > C_MAX_SAFE_INTEGER)
		mxTypeError("unsafe integer");
	resultArray = fxCreateArraySpecies(the, DELETIONS);
	if (array && resultArray) {
		txSlot* address;
		txIndex length = (txIndex)LENGTH;
		txIndex start = (txIndex)START;
		txIndex insertions = (txIndex)INSERTIONS;
		txIndex deletions = (txIndex)DELETIONS;
		txIndex index;
		if (LENGTH + INSERTIONS - DELETIONS > 0xFFFFFFFF)
			mxTypeError("array overflow");
		c_memcpy(resultArray->value.array.address, array->value.array.address + start, deletions * sizeof(txSlot));
		fxIndexArray(the, resultArray);
		if (insertions < deletions) {
			c_memmove(array->value.array.address + start + insertions, array->value.array.address + start + deletions, (length - (start + deletions)) * sizeof(txSlot));
			fxSetArraySize(the, array, length - (deletions - insertions));
		}
		else if (insertions > deletions) {
			fxSetArraySize(the, array, length + (insertions - deletions));
			c_memmove(array->value.array.address + start + insertions, array->value.array.address + start + deletions, (length - (start + deletions)) * sizeof(txSlot));
		}
		address = array->value.array.address + start;
		index = 2;
		while (index < c) {
			txSlot* argument = mxArgv(index);
			address->ID = XS_NO_ID;
			address->kind = argument->kind;
			address->value = argument->value;
			address++;
			index++;
		}
		fxIndexArray(the, array);
	}
	else {	
		txNumber INDEX = 0;
		while (INDEX < DELETIONS) {
			txNumber FROM = START + INDEX;
			mxPushSlot(mxThis);
			mxPushNumber(FROM);
			if (fxHasAt(the)) {
				mxPushSlot(mxThis);
				mxPushNumber(FROM);
				fxGetAt(the);
				mxPushSlot(mxResult);
				mxPushNumber(INDEX);
				fxSetAt(the);
				mxPop();
			}
			INDEX++;
		}
		mxPushNumber(DELETIONS);
		mxPushSlot(mxResult);
		fxSetID(the, mxID(_length));
		mxPop();
		if (INSERTIONS < DELETIONS) {
			INDEX = START;
			while (INDEX < (LENGTH - DELETIONS)) {
				fxMoveThisItem(the, INDEX + DELETIONS, INDEX + INSERTIONS);
				INDEX++;
			}
			INDEX = LENGTH;
			while (INDEX > (LENGTH - DELETIONS + INSERTIONS)) {
				mxPushSlot(mxThis);
				mxPushNumber(INDEX - 1);
				fxDeleteAt(the);
				mxPop();
				INDEX--;
			}
		}
		else if (INSERTIONS > DELETIONS) {
			INDEX = LENGTH - DELETIONS;
			while (INDEX > START) {
				fxMoveThisItem(the, INDEX + DELETIONS - 1, INDEX + INSERTIONS - 1);
				INDEX--;
			}
		}
		INDEX = 0;
		while (INDEX < INSERTIONS) {
			mxPushSlot(mxArgv(2 + (txInteger)INDEX));
			mxPushSlot(mxThis);
			mxPushNumber(START + INDEX);
			fxSetAt(the);
			mxPop();
			INDEX++;
		}
		mxPushNumber(LENGTH - DELETIONS + INSERTIONS);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_length));
		mxPop();
	}
}

void fx_Array_prototype_toString(txMachine* the)
{
	mxPushInteger(0);
	mxPushSlot(mxThis);
	fxCallID(the, mxID(_join));
	mxPullSlot(mxResult);
}

void fx_Array_prototype_unshift(txMachine* the)
{
	txIndex c = mxArgc, i;
	txSlot* array = fxCheckArray(the, mxThis);
	if (array) {
		txSlot* address;
		txIndex length = array->value.array.length;
		if (length + c < length)
			mxRangeError("array overflow");
		if (c > 0) {
			fxSetArraySize(the, array, length + c);
			address = array->value.array.address;
			c_memmove(address + c, address, length * sizeof(txSlot));
			i = 0;
			while (i < c) {
				txSlot* argument = mxArgv(i);
				address->ID = XS_NO_ID;
				address->kind = argument->kind;
				address->value = argument->value;
				address++;
				i++;
			}
			fxIndexArray(the, array);
		}
		mxPushUnsigned(length + c);
	}
	else {
		txNumber length = fxGetArrayLength(the, mxThis);
		txNumber index = length;
		if (length + c > C_MAX_SAFE_INTEGER)
			mxTypeError("unsafe integer");
		while (index > 0) {
			fxMoveThisItem(the, index - 1, index + c - 1);
			index--;
		}
		i = 0;
		while (i < c) {
			mxPushSlot(mxArgv(i));
			mxPushSlot(mxThis);
			fxSetIndex(the, i);
			mxPop();
			i++;
		}
		mxPushNumber(length + c);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_length));
	}
	mxPullSlot(mxResult);
}

void fx_Array_prototype_values(txMachine* the)
{
	txSlot* property;
	fxToInstance(the, mxThis);
	mxPush(mxArrayIteratorPrototype);
	property = fxLastProperty(the, fxNewIteratorInstance(the, mxThis));
	property = fxNextIntegerProperty(the, property, 0, XS_NO_ID, XS_INTERNAL_FLAG | XS_GET_ONLY);
	mxPullSlot(mxResult);
}

void fx_ArrayIterator_prototype_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	if (!done->value.boolean) {
		txInteger kind = index->next->value.integer;
		txIndex length = (txIndex)fxGetArrayLength(the, iterable);
		txIndex i = (txIndex)index->value.integer;
		if (i < length) {
			switch(kind) {
			case 0: 
				mxPushSlot(iterable);
				fxGetIndex(the, i);
				mxPullSlot(value);
				break;
			case 1:
				mxPushUnsigned(i);
				mxPullSlot(value);
				break;
			case 2:
				mxPushUnsigned(i);
				mxPushSlot(iterable);
				fxGetIndex(the, i);
				fxConstructArrayEntry(the, value);
				break;
			}
            index->value.integer = i + 1;
		}
		else {
			value->kind = XS_UNDEFINED_KIND;
			done->value.boolean = 1;
		}
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}


txSlot* fxNewArgumentsSloppyInstance(txMachine* the, txIndex count)
{
	txSlot* instance;
	txSlot* array;
	txSlot* property;
	txIndex index;
	txSlot* address;
	txIndex length = (txIndex)mxArgc;
	txByte* code = the->code;
	the->code = C_NULL;
	instance = fxNewObjectInstance(the);
	array = instance->next = fxNewSlot(the);
	array->flag = XS_INTERNAL_FLAG;
	array->kind = XS_ARGUMENTS_SLOPPY_KIND;
	array->value.array.length = 0;
	array->value.array.address = C_NULL;
	property = fxNextNumberProperty(the, array, length, mxID(_length), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, mxFunction, mxID(_callee), XS_DONT_ENUM_FLAG);
	property = fxNextNullProperty(the, property, mxID(_caller), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, &mxArrayIteratorFunction, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	the->code = code;
	fxSetArraySize(the, array, length);
	index = 0;
	address = array->value.array.address;
	property = the->scope + count;
	while ((index < length) && (index < count)) {
		*((txIndex*)address) = index;
		address->ID = XS_NO_ID;
		address->kind = property->kind;
		address->value = property->value;
		index++;
		address++;
		property--;
	}
	while (index < length) {
		property = mxArgv(index);
		*((txIndex*)address) = index;
		address->ID = XS_NO_ID;
		address->kind = property->kind;
		address->value = property->value;
		index++;
		address++;
	}
	return instance;
}

txSlot* fxGetArgumentsSloppyProperty(txMachine* the, txSlot* instance, txIndex index) 
{
	txSlot* result = fxGetArrayProperty(the, instance->next, index);
	if (result && (result->kind == XS_CLOSURE_KIND))
		return result->value.closure;
	return result;
}

txBoolean fxRemoveArgumentsSloppyProperty(txMachine* the, txSlot* instance, txIndex index) 
{
	txSlot* result = fxGetArrayProperty(the, instance->next, index);
	if (result) {
		if (result->kind == XS_CLOSURE_KIND) {
			if (result->value.closure->flag & XS_DONT_DELETE_FLAG)
				return 0;
		}
		return fxDeleteArrayProperty(the, instance->next, index);
	}
	return 1;
}

txSlot* fxSetArgumentsSloppyProperty(txMachine* the, txSlot* instance, txIndex index) 
{
	txSlot* result = fxSetArrayProperty(the, instance, instance->next, index);
	if (result && (result->kind == XS_CLOSURE_KIND))
		return result->value.closure;
	return result;
}

txSlot* fxUnmapArgumentsSloppyProperty(txMachine* the, txSlot* instance, txIndex index)
{
	txSlot* result = fxGetArrayProperty(the, instance->next, index);
	if (result && (result->kind == XS_CLOSURE_KIND)) {
		txSlot* closure = result->value.closure;
		result->flag = closure->flag;
		result->kind = closure->kind;
		result->value = closure->value;
	}
	return result;
}


txSlot* fxNewArgumentsStrictInstance(txMachine* the, txIndex count)
{
	txSlot* instance;
	txSlot* array;
	txSlot* property;
	txIndex index;
	txSlot* address;
	txIndex length = (txIndex)mxArgc;
	txByte* code = the->code;
	the->code = C_NULL;
	instance = fxNewObjectInstance(the);
	array = instance->next = fxNewSlot(the);
	array->flag = XS_INTERNAL_FLAG;
	array->kind = XS_ARGUMENTS_STRICT_KIND;
	array->value.array.length = 0;
	array->value.array.address = C_NULL;
	property = fxNextNumberProperty(the, array, length, mxID(_length), XS_DONT_ENUM_FLAG);
	property = fxNextHostAccessorProperty(the, property, fx_Arguments_prototype_callee, fx_Arguments_prototype_callee, mxID(_callee), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	property = fxNextHostAccessorProperty(the, property, fx_Arguments_prototype_caller, fx_Arguments_prototype_caller, mxID(_caller), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	property = fxNextSlotProperty(the, property, &mxArrayIteratorFunction, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	the->code = code;
	count = mxArgc;
	fxSetArraySize(the, array, length);
	index = 0;
	address = array->value.array.address;
	while (index < length) {
		property = mxArgv(index);
		*((txIndex*)address) = index;
		address->ID = XS_NO_ID;
		address->kind = property->kind;
		address->value = property->value;
		index++;
		address++;
	}
	return instance;
}

void fx_Arguments_prototype_callee(txMachine* the)
{
	mxTypeError("arguments.callee (strict mode)");
}

void fx_Arguments_prototype_caller(txMachine* the)
{
	mxTypeError("arguments.caller (strict mode)");
}
