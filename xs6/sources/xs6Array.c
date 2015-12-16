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

#define XS_MAX_INDEX ((2 << 28) - 2)
#define mxPop() (the->stack++)

static txBoolean fxCallArrayItem(txMachine* the, txSlot* function, txSlot* array, txIndex index, txSlot* item);
static txSlot* fxCoerceToArray(txMachine* the, txSlot* slot, txIndex* length);
static int fxCompareArrayItem(txMachine* the, txSlot* function, txSlot* array, txInteger i);
static txSlot* fxConstructArrayResult(txMachine* the, txSlot* constructor, txUnsigned length);
static txBoolean fxIsArray(txMachine* the, txSlot* instance);
static void fxReduceArrayItem(txMachine* the, txSlot* function, txSlot* array, txIndex index);
#if mxProxy
static txBoolean fxCallProxyItem(txMachine* the, txSlot* function, txSlot* instance, txIndex index, txSlot* item);
static void fxMoveProxyItem(txMachine* the, txSlot* instance, txIndex from, txIndex to);
static void fxReduceProxyItem(txMachine* the, txSlot* function, txSlot* instance, txIndex index);
#endif

static void fx_Array(txMachine* the);
static void fx_Array_from(txMachine* the);
static void fx_Array_from_aux(txMachine* the, txSlot* function, txIndex index);
static void fx_Array_isArray(txMachine* the);
static void fx_Array_of(txMachine* the);
static void fx_Array_prototype_concat(txMachine* the);
static void fx_Array_prototype_copyWithin(txMachine* the);
static void fx_Array_prototype_entries(txMachine* the);
static void fx_Array_prototype_entries_next(txMachine* the);
static void fx_Array_prototype_every(txMachine* the);
static void fx_Array_prototype_fill(txMachine* the);
static void fx_Array_prototype_filter(txMachine* the);
static void fx_Array_prototype_find(txMachine* the);
static void fx_Array_prototype_findIndex(txMachine* the);
static void fx_Array_prototype_forEach(txMachine* the);
static void fx_Array_prototype_indexOf(txMachine* the);
static void fx_Array_prototype_join(txMachine* the);
static void fx_Array_prototype_keys(txMachine* the);
static void fx_Array_prototype_keys_next(txMachine* the);
static void fx_Array_prototype_lastIndexOf(txMachine* the);
static void fx_Array_prototype_length_get(txMachine* the);
static void fx_Array_prototype_length_set(txMachine* the);
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
static void fx_Array_prototype_values_next(txMachine* the);

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
		{ fx_Array_prototype_values, 0, _Symbol_iterator },
		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_Array_builders[] = {
		{ fx_Array_from, 1, _from },
		{ fx_Array_isArray, 1, _isArray },
		{ fx_Array_of, 1, _of },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	txSlot* unscopable;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewArrayInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, fx_Array_prototype_length_get, fx_Array_prototype_length_set, mxID(_length), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	for (builder = gx_Array_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextBooleanProperty(the, slot, 1, mxID(_Symbol_isConcatSpreadable), XS_DONT_ENUM_FLAG);
	mxPush(mxObjectPrototype);
	unscopable = fxLastProperty(the, fxNewObjectInstance(the));
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_find), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_findIndex), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_fill), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_copyWithin), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_entries), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_keys), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	unscopable = fxNextBooleanProperty(the, unscopable, 1, mxID(_values), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextSlotProperty(the, slot, the->stack++, mxID(_Symbol_unscopables), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxArrayPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_Array, 1, mxID(_Array), XS_DONT_ENUM_FLAG));
	for (builder = gx_Array_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_species_get, C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Array_prototype_entries_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Array Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxArrayEntriesIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Array_prototype_keys_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Array Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxArrayKeysIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Array_prototype_values_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Array Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxArrayValuesIteratorPrototype);
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringProperty(the, slot, "Arguments", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxParametersPrototype = *the->stack;
	the->stack++;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringProperty(the, slot, "Arguments", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxArgumentsStrictPrototype = *the->stack;
	the->stack++;
}

txIndex fxArgToIndex(txMachine* the, txInteger argi, txIndex index, txIndex length)
{
	if ((mxArgc > argi) && (mxArgv(argi)->kind != XS_UNDEFINED_KIND)) {
		txNumber c = length;
		txNumber i = c_trunc(fxToNumber(the, mxArgv(argi)));
		if (c_isnan(i))
			i = 0;
		if (i < 0) {
			i = c + i;
			if (i < 0)
				i = 0;
		}
		else if (i > c)
			i = c;
		index = (txIndex)i;
	}
	return index;
}

txIndex fxArgToArrayLimit(txMachine* the, txInteger argi)
{
	if (mxArgc > argi) {
		txNumber i = c_trunc(fxToNumber(the, mxArgv(argi)));
		if (c_isnan(i))
			i = 0;
		else if (i < 0)
			i = 0;
		else if (i > XS_MAX_INDEX)
			i = XS_MAX_INDEX;
		return (txIndex)i;
	}
	return XS_MAX_INDEX;
}

void fxCacheArray(txMachine* the, txSlot* instance)
{
	txSlot* array = instance->next;
	txIndex length = array->value.array.length;
	if (length) {
		txSlot* address = (txSlot *)fxNewChunk(the, length * sizeof(txSlot));
		txSlot* srcSlot = instance->next->next;
		txSlot* dstSlot = address;
		while (srcSlot) {
			dstSlot->ID = XS_NO_ID;
			dstSlot->flag = XS_NO_FLAG;
			dstSlot->kind = srcSlot->kind;
			dstSlot->value = srcSlot->value;
			srcSlot = srcSlot->next;
			dstSlot++;
		}
		instance->next->value.array.address = address;
		instance->next->next = C_NULL;
	}
}

txBoolean fxCallArrayItem(txMachine* the, txSlot* function, txSlot* array, txIndex index, txSlot* item)
{
	txSlot* slot = array->value.array.address + index;
	if (slot->ID) {
		/* ARG0 */
		mxPushSlot(slot);
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

txSlot* fxCoerceToArray(txMachine* the, txSlot* slot, txIndex* length)
{
	txSlot* instance = fxToInstance(the, slot);
#if mxProxy
	if (mxIsProxy(instance)) {
		if (length) {
			mxPushSlot(slot);
			fxGetID(the, mxID(_length));
			*length = fxToInteger(the, the->stack++);
		}
		return C_NULL;
	}
#endif
	txSlot** address = &(instance->next);
	txSlot* array = C_NULL;
	txSlot* property;
	while ((property = *address)) {
		if (property->ID == XS_NO_ID) {
			if ((property->kind == XS_ARRAY_KIND) || (property->kind == XS_PARAMETERS_KIND)) {
				array = property;
				break;
			}
		}
		else
			break;
		address = &(property->next);
	}
	if (!array) {
		array = *address = fxNewSlot(the);
		array->next = property;
		array->kind = XS_ARRAY_KIND;
		array->value.array.address = C_NULL;
		array->value.array.length = 0;
	}
	if (length)
		*length = array->value.array.length;
	return array;
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
			mxPushSlot(mxThis);
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
	fxSetArrayLength(the, array, 2);
	item = array->value.array.address;
	item->ID = XS_NO_ID;
	item->kind = key->kind;
	item->value = key->value;
	item++;
	item->ID = XS_NO_ID;
	item->kind = value->kind;
	item->value = value->value;
	entry->kind = the->stack->kind;
	entry->value = the->stack->value;
	the->stack += 3;
}

txSlot* fxConstructArrayResult(txMachine* the, txSlot* constructor, txUnsigned length)
{
	mxPushUnsigned(length);
	mxPushInteger(1);
	if (constructor) {
		if (mxIsReference(constructor) && mxIsFunction(constructor->value.reference))
			mxPushSlot(constructor);
		else
			mxPushUndefined();
	}
	else {
		if (fxIsArray(the, mxThis->value.reference)) {
			mxPushSlot(mxThis);
			fxGetID(the, mxID(_constructor));
			if (mxIsReference(the->stack))
				fxGetID(the, mxID(_Symbol_species));
			else
				the->stack->kind = XS_UNDEFINED_KIND;
		}
		else
			mxPushUndefined();
	}
	if ((the->stack->kind == XS_UNDEFINED_KIND) || (the->stack->kind == XS_NULL_KIND)) {
		*the->stack = mxGlobal;
		fxGetID(the, mxID(_Array));
	}
	fxNew(the);
	mxPullSlot(mxResult);
	return fxCoerceToArray(the, mxResult, C_NULL);
}

txBoolean fxIsArray(txMachine* the, txSlot* instance) 
{
again:
	if (instance) {
		if (instance->flag & XS_VALUE_FLAG) {
			txSlot* exotic = instance->next;
			if (exotic->kind == XS_ARRAY_KIND)
				return 1;
#if mxProxy
			if (exotic->kind == XS_PROXY_KIND) {
				instance = exotic->value.proxy.target;
				goto again;
			}
#endif
		}
	}
	return 0;
}

void fxReduceArrayItem(txMachine* the, txSlot* function, txSlot* array, txIndex index)
{
	txSlot* slot = array->value.array.address + index;
	if (slot->ID) {
		/* ARG0 */
		mxPushSlot(mxResult);
		/* ARG1 */
		mxPushSlot(slot);
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

#if mxProxy
txBoolean fxCallProxyItem(txMachine* the, txSlot* function, txSlot* instance, txIndex index, txSlot* item)
{
	if (fxHasProperty(the, instance, index)) {
		/* ARG0 */
		mxPushReference(instance);
		fxGetID(the, index);
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

void fxMoveProxyItem(txMachine* the, txSlot* instance, txIndex from, txIndex to)
{
	if (fxHasProperty(the, instance, from)) {
		mxPushSlot(mxThis);
		fxGetID(the, from);
		mxPushSlot(mxThis);
		fxSetID(the, to);
		mxPop();
	}
	else {
		mxPushSlot(mxThis);
		fxDeleteID(the, to);
		mxPop();
	}
}

void fxReduceProxyItem(txMachine* the, txSlot* function, txSlot* instance, txIndex index)
{
	if (fxHasProperty(the, instance, index)) {
		/* ARG0 */
		mxPushSlot(mxResult);
		/* ARG1 */
		mxPushReference(instance);
		fxGetID(the, index);
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
#endif

void fxSetArrayLength(txMachine* the, txSlot* array, txIndex target)
{
	txIndex length = array->value.array.length;
	txSlot* address = array->value.array.address;
	if (length != target) {
		if (address)
			address = (txSlot*)fxRenewChunk(the, address, target * sizeof(txSlot));
		if (address) {
			if (length < target)
				c_memset(address + length, 0, (target - length) * sizeof(txSlot));
		}
		else {
			address = (txSlot*)fxNewChunk(the, target * sizeof(txSlot));
			if (length < target) {
				c_memcpy(address, array->value.array.address, length * sizeof(txSlot));
				c_memset(address + length, 0, (target - length) * sizeof(txSlot));
			}
			else
				c_memcpy(address, array->value.array.address, target * sizeof(txSlot));
		}
		array->value.array.length = target;
		array->value.array.address = address;
	}
}

txSlot* fxNewArrayInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_VALUE_FLAG;
	property = instance->next = fxNewSlot(the);
	property->kind = XS_ARRAY_KIND;
	property->value.array.length = 0;
	property->value.array.address = C_NULL;
	return instance;
}

txSlot* fxGetArrayProperty(txMachine* the, txSlot* array, txInteger index) 
{
	txSlot* result;
	if (index < array->value.array.length) {
		result = array->value.array.address + index;
		if (result->ID)
			return result;
	}
	return C_NULL;
}

txBoolean fxRemoveArrayProperty(txMachine* the, txSlot* array, txInteger index) 
{
	txSlot* result;
	if (index < array->value.array.length) {
		result = array->value.array.address + index;
		if (result->flag & XS_DONT_DELETE_FLAG)
			return 0;
		result->ID = 0;
		result->kind = XS_UNDEFINED_KIND;
	}
	return 1;
}

txSlot* fxSetArrayProperty(txMachine* the, txSlot* array, txInteger index) 
{
	txSlot* result;
	if (index >= XS_MAX_INDEX)		
		mxRangeError("invalid index");
	if (array->value.array.length <= index)
		fxSetArrayLength(the, array, index + 1);
	result = array->value.array.address + index;
	result->ID = XS_NO_ID;
	return result;
}

void fx_Array(txMachine* the)
{
	txIndex count = (txIndex)mxArgc;
	txBoolean flag = 0;
	txSlot* instance;
	txSlot* array;
	txSlot* argument;
	txSlot* slot;
	if (mxTarget->kind == XS_UNDEFINED_KIND) {
		mxPush(mxArrayPrototype);
		instance = fxNewArrayInstance(the);
		mxPullSlot(mxResult);
		array = instance->next;	
	}
	else
		array = fxCoerceToArray(the, mxThis, C_NULL);
	flag = 0;
	if (count == 1) {
		argument = mxArgv(0);
		if (argument->kind == XS_INTEGER_KIND) {
			flag = 1;
			if ((0 <= argument->value.integer) && (argument->value.integer < XS_MAX_INDEX))
				count = (txIndex)argument->value.integer;
			else
				mxRangeError("invalid length");
		}
		else if (mxArgv(0)->kind == XS_NUMBER_KIND) {
			flag = 1;
			if ((0 <= argument->value.number) && (argument->value.number < XS_MAX_INDEX))
				count = (txIndex)argument->value.number;
			else
				mxRangeError("invalid length");
		}
	}
	array->value.array.address = (txSlot *)fxNewChunk(the, count * sizeof(txSlot));
	array->value.array.length = count;
	slot = array->value.array.address;
	if (flag) {
		c_memset(slot, 0, count * sizeof(txSlot));
	}
	else {
		txIndex index = 0;
		while (index < count) {
			txSlot* argument = mxArgv(index);
			slot->ID = XS_NO_ID;
			slot->kind = argument->kind;
			slot->value = argument->value;
			slot++;
			index++;
		}
	}
}

void fx_Array_from(txMachine* the)
{
	txSlot* resultArray = fxConstructArrayResult(the, mxThis, 0);
	txSlot* function = (mxArgc > 1) ? fxArgToCallback(the, 1) : C_NULL;
	if (mxArgc > 0) {
		mxPushSlot(mxArgv(0));
		fxGetID(the, mxID(_Symbol_iterator));
		if (resultArray) {
			txSlot* item = resultArray;
			txIndex count = 0;
			txIndex index;
			if (the->stack->kind != XS_UNDEFINED_KIND) {
				txSlot* iterator;
				txSlot* result;
				mxPushInteger(0);
				mxPushSlot(mxArgv(0));
				fxCallID(the, mxID(_Symbol_iterator));
				iterator = the->stack;
				{
					mxTry(the) {
						count = 0;
						for(;;) {
							mxCallID(iterator, mxID(_next), 0);
							result = the->stack;
							mxGetID(result, mxID(_done));
							if (fxToBoolean(the, the->stack))
								break;
							the->stack++;
							mxGetID(result, mxID(_value));
							fx_Array_from_aux(the, function, count);
							item = item->next = fxNewSlot(the);
							mxPullSlot(item);
							resultArray->value.array.length++;
							count++;
						}
					}
					mxCatch(the) {
						mxCallID(iterator, mxID(_return), 0);
						fxJump(the);
					}
				}
				the->stack++;
			}
			else {
				mxPushSlot(mxArgv(0));
				fxGetID(the, mxID(_length));
				count = fxToInteger(the, the->stack++);
				index = 0;
				while (index < count) {
					mxPushSlot(mxArgv(0));
					fxGetID(the, index);
					fx_Array_from_aux(the, function, index);
					item = item->next = fxNewSlot(the);
					mxPullSlot(item);
					resultArray->value.array.length++;
					index++;
				}	
			}
			fxCacheArray(the, mxResult->value.reference);
		}
		else {
#if mxProxy
			txIndex length;
			if (the->stack->kind != XS_UNDEFINED_KIND) {
				txSlot* iterator;
				txSlot* result;
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
							fxDefineDataProperty(the, mxResult->value.reference, length, the->stack);
							the->stack++;
							length++;
						}
					}
					mxCatch(the) {
						mxCallID(iterator, mxID(_return), 0);
						fxJump(the);
					}
				}
				the->stack++;
			}
			else {
				txIndex index = 0;
				mxPushSlot(mxArgv(0));
				fxGetID(the, mxID(_length));
				length = fxToInteger(the, the->stack++);
				while (index < length) {
					mxPushSlot(mxArgv(0));
					fxGetID(the, index);
					fx_Array_from_aux(the, function, index);
					fxDefineDataProperty(the, mxResult->value.reference, index, the->stack);
					the->stack++;
					index++;
				}
			}
			mxPushInteger(length);
			mxPushSlot(mxResult);
			fxSetID(the, mxID(_length));
			mxPop();
#endif
		}
		the->stack++;
	}
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
	txSlot* array = fxConstructArrayResult(the, mxThis, count);
	if (array) {
		txSlot* slot;
		fxSetArrayLength(the, array, count);
		slot = array->value.array.address;
		while (index < count) {
			txSlot* argument = mxArgv(index);
			slot->ID = XS_NO_ID;
			slot->kind = argument->kind;
			slot->value = argument->value;
			slot++;
			index++;
		}
	}
	else {
#if mxProxy
		while (index < count) {
			fxDefineDataProperty(the, mxResult->value.reference, index, mxArgv(index));
			index++;
		}
		mxPushInteger(count);
		mxPushSlot(mxResult);
		fxSetID(the, mxID(_length));
		mxPop();
#endif
	}
}

void fx_Array_prototype_concat(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis, C_NULL);
	txSlot* resultArray = fxConstructArrayResult(the, C_NULL, 0);
	if (array && resultArray) {
		txSlot* slot;
		txIndex length = array->value.array.length;
		txIndex count, index;
		txInteger c = mxArgc, i,  ii, ic;
		txSlot* argument;
		txSlot* resultSlot;
	
		count = 0;
		index = 0;
		while (index < length) {
			slot = array->value.array.address + index;
			if (slot->ID)
				count++;
			index++;
		}
		i = 0;
		while (i < c) {
			argument = mxArgv(i);
			i++;
			if (argument->kind == XS_REFERENCE_KIND) {
				mxPushSlot(argument);
				fxGetID(the, mxID(_Symbol_isConcatSpreadable));
				if (fxToBoolean(the, the->stack++)) {
					mxPushSlot(argument);
					fxGetID(the, mxID(_length));
					ic = fxToInteger(the, the->stack++);
					ii = 0;
					while (ii < ic) {
						if (fxGetProperty(the, argument->value.reference, ii))
							count++;
						ii++;
					}
					continue;
				}
			}
			count++;
		}
		fxSetArrayLength(the, resultArray, count);
		count = 0;
		index = 0;
		while (index < length) {
			slot = array->value.array.address + index;
			if (slot->ID) {
				resultSlot = resultArray->value.array.address + count;
				resultSlot->ID = XS_NO_ID;
				resultSlot->kind = slot->kind;
				resultSlot->value = slot->value;
				count++;
			}
			index++;
		}
		i = 0;
		while (i < c) {
			argument = mxArgv(i);
			i++;
			if (argument->kind == XS_REFERENCE_KIND) {
				mxPushSlot(argument);
				fxGetID(the, mxID(_Symbol_isConcatSpreadable));
				if (fxToBoolean(the, the->stack++)) {
					mxPushSlot(argument);
					fxGetID(the, mxID(_length));
					ic = fxToInteger(the, the->stack++);
					ii = 0;
					while (ii < ic) {
						if ((slot = fxGetProperty(the, argument->value.reference, ii))) {
						
							mxPushSlot(argument);
							fxGetID(the, ii);

							resultSlot = resultArray->value.array.address + count;
							resultSlot->ID = XS_NO_ID;
							mxPullSlot(resultSlot);
							
							count++;
						}
						ii++;
					}
					continue;
				}
			}
			resultSlot = resultArray->value.array.address + count;
			resultSlot->ID = XS_NO_ID;
			resultSlot->kind = argument->kind;
			resultSlot->value = argument->value;
			count++;
		}
	}
	else {
#if mxProxy
		txInteger c = mxArgc;
		txInteger i = -1;
		txInteger total = 0;
		mxPushSlot(mxThis);
		for (;;) {
			txSlot* argument = the->stack;
			mxPushSlot(argument);
			fxGetID(the, mxID(_Symbol_isConcatSpreadable));
			if (fxToBoolean(the, the->stack++)) {
				txIndex length, index;
				mxPushSlot(argument);
				fxGetID(the, mxID(_length));
				length = fxToInteger(the, the->stack++);
				index = 0;
				while (index < length) {
					if (fxHasProperty(the, argument->value.reference, index)) {
						mxPushSlot(argument);
						fxGetID(the, index);
						fxDefineDataProperty(the, mxResult->value.reference, total, the->stack);
						total++;
						the->stack++;
					}
					index++;
				}
			}
			else {
				fxDefineDataProperty(the, mxResult->value.reference, total, argument);
				total++;
			}
			the->stack++;
			i++;
			if (i == c)
				break;
			mxPushSlot(mxArgv(i));
		}
#endif
	}
}

void fx_Array_prototype_copyWithin(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txIndex to = fxArgToIndex(the, 0, 0, length);
	txIndex from = fxArgToIndex(the, 1, 0, length);
	txIndex final = fxArgToIndex(the, 2, length, length);
	txIndex count = final - from;
	if (count > length - to)
		count = length - to;
	if (array) {
		if (count > 0)
			c_memmove(array->value.array.address + to, array->value.array.address + from, count * sizeof(txSlot));
		mxResult->kind = mxThis->kind;
		mxResult->value = mxThis->value;
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		txInteger direction;
		if ((from < to) && (to < from + count)) {
			direction = -1;
			from += count - 1;
			to += count - 1;
		}
		else
			direction = 1;
		while (count > 0) {
			fxMoveProxyItem(the, instance, from, to);
			from += direction;
			to += direction;
			count--;
		}	
#endif
	}	
}

void fx_Array_prototype_entries(txMachine* the)
{
	mxPush(mxArrayEntriesIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Array_prototype_entries_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	txIndex length;
	txSlot* array = fxCoerceToArray(the, iterable, &length);
	txIndex i = index->value.integer;
	if (i < length) {
		txSlot* item = array->value.array.address + i;
		mxPushInteger(i);
		if (item->ID)
			mxPushSlot(item);
		else
			mxPushUndefined();
		fxConstructArrayEntry(the, value);
		index->value.integer = i + 1;
	}
	else {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
		index->value.integer = i;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

void fx_Array_prototype_every(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	if (array) {
		while (index < length) {
			if (fxCallArrayItem(the, function, array, index, C_NULL)) {
				mxResult->value.boolean = fxToBoolean(the, the->stack++);
				if (!mxResult->value.boolean)
					break;
			}
			index++;
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		while (index < length) {
			if (fxCallProxyItem(the, function, instance, index, C_NULL)) {
				mxResult->value.boolean = fxToBoolean(the, the->stack++);
				if (!mxResult->value.boolean)
					break;
			}
			index++;
		}
#endif
	}
}

void fx_Array_prototype_fill(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txIndex start = fxArgToIndex(the, 1, 0, length);
	txIndex end = fxArgToIndex(the, 2, length, length);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	if (array) {
		while (start < end) {
			txSlot* slot = array->value.array.address + start;
			slot->ID = XS_NO_ID;
			slot->kind = the->stack->kind;
			slot->value = the->stack->value;
			start++;
		}
	}
	else {
#if mxProxy
		txSlot* value = the->stack;
		while (start < end) {
			mxPushSlot(value);
			mxPushSlot(mxThis);
			fxSetID(the, start);
			mxPop();
			start++;
		}
#endif
	}
	the->stack++;
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_Array_prototype_filter(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txSlot* resultArray = fxConstructArrayResult(the, C_NULL, 0);
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	txIndex count = 0;
	txSlot* item;
	mxPushUndefined();
	item = the->stack;
	if (array && resultArray) {
		txSlot* list = fxNewInstance(the);
		txSlot* slot = list;
		txSlot* resultSlot;
		while (index < length) {
			if (fxCallArrayItem(the, function, array, index, item)) {
				if (fxToBoolean(the, the->stack++)) {
					count++;
					slot = fxNextSlotProperty(the, slot, item, XS_NO_ID, XS_NO_FLAG);
				}
			}
			index++;
		}
		the->stack++;
		fxSetArrayLength(the, resultArray, count);
		resultSlot = resultArray->value.array.address;
		slot = list->next;
		while (slot) {
			resultSlot->ID = XS_NO_ID;
			resultSlot->kind = slot->kind;
			resultSlot->value = slot->value;
			resultSlot++;
			slot = slot->next;
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		while (index < length) {
			if (fxCallProxyItem(the, function, instance, index, item)) {
				if (fxToBoolean(the, the->stack++)) {
					fxDefineDataProperty(the, mxResult->value.reference, count, item);
					count++;
				}
			}
			index++;
		}
#endif
	}
	the->stack++;
}

void fx_Array_prototype_find(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	txSlot* item;
	mxPushUndefined();
	item = the->stack;
	if (array) {
		while (index < length) {
			if (fxCallArrayItem(the, function, array, index, item)) {
				if (fxToBoolean(the, the->stack++)) {
					mxResult->kind = item->kind;
					mxResult->value = item->value;
					break;
				}
			}
			index++;
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		while (index < length) {
			if (fxCallProxyItem(the, function, instance, index, item)) {
				if (fxToBoolean(the, the->stack++)) {
					mxResult->kind = item->kind;
					mxResult->value = item->value;
					break;
				}
			}
			index++;
		}
#endif
	}
}

void fx_Array_prototype_findIndex(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	fxInteger(the, mxResult, -1);
	if (array) {
		while (index < length) {
			if (fxCallArrayItem(the, function, array, index, C_NULL)) {
				if (fxToBoolean(the, the->stack++)) {
					fxUnsigned(the, mxResult, index);
					break;
				}
			}
			index++;
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		while (index < length) {
			if (fxCallProxyItem(the, function, instance, index, C_NULL)) {
				if (fxToBoolean(the, the->stack++)) {
					fxUnsigned(the, mxResult, index);
					break;
				}
			}
			index++;
		}
#endif
	}
}

void fx_Array_prototype_forEach(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	if (array) {
		while (index < length) {
			fxCallArrayItem(the, function, array, index, C_NULL);
			index++;
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		while (index < length) {
			fxCallProxyItem(the, function, instance, index, C_NULL);
			index++;
		}
#endif
	}
}

void fx_Array_prototype_indexOf(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txIndex index = fxArgToIndex(the, 1, 0, length);
	txSlot* argument;
	fxInteger(the, mxResult, -1);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	argument = the->stack;
	if (array) {
		while (index < length) {
			txSlot* slot = array->value.array.address + index;
			if (slot->ID) {
				if (fxIsSameSlot(the, slot, argument)) {
					fxUnsigned(the, mxResult, index);
					break;
				}
			}
			index++;
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		while (index < length) {
			if (fxHasProperty(the, instance, index)) {
				mxPushSlot(mxThis);
				fxGetID(the, index);
				if (fxIsSameSlot(the, the->stack++, argument)) {
					fxUnsigned(the, mxResult, index);
					break;
				}
			}
			index++;
		}
#endif
	}
	the->stack++;
}

void fx_Array_prototype_join(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txIndex index = 0;
	if (array) {
		txString string;
		txSlot* list = fxNewInstance(the);
		txSlot* slot = list;
		txSlot* item;
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
			item = array->value.array.address + index;
			if (item->ID) {
				slot = fxNextSlotProperty(the, slot, item, XS_NO_ID, XS_NO_FLAG);
				string = fxToString(the, slot);
				slot->kind += XS_KEY_KIND - XS_STRING_KIND;
				slot->value.key.sum = c_strlen(string);
				size += slot->value.key.sum;
			}
			index++;
		}
		the->stack++;
		string = mxResult->value.string = fxNewChunk(the, size + 1);
		slot = list->next;
		while (slot) {
			c_memcpy(string, slot->value.key.string, slot->value.key.sum);
			string += slot->value.key.sum;
			slot = slot->next;
		}
		*string = 0;
		the->stack++;
		mxResult->kind = XS_STRING_KIND;
	}
	else {
#if mxProxy
		txSlot* separator;
		if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
			fxToString(the, mxArgv(0));
			mxPushSlot(mxArgv(0));
		}
		else
			mxPushStringC(",");
		separator = the->stack;
		mxPushSlot(mxThis);
		fxGetID(the, index);
		index++;
		if ((the->stack->kind != XS_UNDEFINED_KIND) && (the->stack->kind != XS_NULL_KIND))
			fxToString(the, the->stack);
		else {
			mxPop();
			mxPush(mxEmptyString);
		}
		mxPullSlot(mxResult);
		while (index < length) {
			fxConcatString(the, mxResult, separator);
			mxPushSlot(mxThis);
			fxGetID(the, index);
			index++;
			if ((the->stack->kind != XS_UNDEFINED_KIND) && (the->stack->kind != XS_NULL_KIND)) {
				fxToString(the, the->stack);
				fxConcatString(the, mxResult, the->stack);
			}
			mxPop();
#endif
		}
		mxPop();
	}
}

void fx_Array_prototype_keys(txMachine* the)
{
	mxPush(mxArrayKeysIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Array_prototype_keys_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	txIndex length;
	txIndex i = index->value.integer;
	(void)fxCoerceToArray(the, iterable, &length);
	if (i < length) {
		value->kind = XS_INTEGER_KIND;
		value->value.integer = i;
		index->value.integer = i + 1;
	}
	else {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
		index->value.integer = i;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

void fx_Array_prototype_lastIndexOf(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txIndex index = fxArgToIndex(the, 1, length, length);
	txSlot* argument;
	fxInteger(the, mxResult, -1);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	argument = the->stack;
	if (index == length)
		index--;
	if (array) {
		while (index >= 0) {
			txSlot* slot = array->value.array.address + index;
			if (slot->ID) {
				if (fxIsSameSlot(the, slot, argument)) {
					fxUnsigned(the, mxResult, index);
					break;
				}
			}
			index--;
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		while (index >= 0) {
			if (fxHasProperty(the, instance, index)) {
				mxPushSlot(mxThis);
				fxGetID(the, index);
				if (fxIsSameSlot(the, the->stack++, argument)) {
					fxUnsigned(the, mxResult, index);
					break;
				}
			}
			index--;
		}
#endif
	}
	the->stack++;
}

void fx_Array_prototype_length_get(txMachine* the)
{
	txIndex length;
	(void)fxCoerceToArray(the, mxThis, &length);
	if (length <= 0x7FFFFFFF) {
		mxResult->kind = XS_INTEGER_KIND;
		mxResult->value.integer = (txInteger)length;
	}
	else {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = (txNumber)length;
	}
}

void fx_Array_prototype_length_set(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txIndex it = length;
	txBoolean flag;
	txSlot* address;
	if (mxArgv(0)->kind == XS_INTEGER_KIND)
		flag = fxIntegerToIndex(the->dtoa, mxArgv(0)->value.integer, (txInteger*)&it);
	else
		flag = fxNumberToIndex(the->dtoa, fxToNumber(the, mxArgv(0)), (txInteger*)&it);
	if (flag) {
		if (length != it) {
			if (it) {
				address = (txSlot *)fxNewChunk(the, it * sizeof(txSlot));
				if (length < it) {
					if (length)
						c_memcpy(address, array->value.array.address, length * sizeof(txSlot));
					c_memset(address + length, 0, (it - length) * sizeof(txSlot));
				}
				else
					c_memcpy(address, array->value.array.address, it * sizeof(txSlot));
			}
			else
				address = C_NULL;
			array->value.array.length = it;
			array->value.array.address = address;
		}
	}
	else
		mxRangeError("invalid length");
	*mxResult = *mxArgv(0);
}

void fx_Array_prototype_map(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txSlot* resultArray = fxConstructArrayResult(the, C_NULL, length);
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	if (array && resultArray) {
		while (index < length) {
			if (fxCallArrayItem(the, function, array, index, C_NULL)) {
				txSlot* slot = resultArray->value.array.address + index;
				slot->ID = XS_NO_ID;
				slot->kind = the->stack->kind;
				slot->value = the->stack->value;
				the->stack++;
			}
			index++;
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		while (index < length) {
			if (fxCallProxyItem(the, function, instance, index, C_NULL)) {
				fxDefineDataProperty(the, mxResult->value.reference, index, the->stack);
				the->stack++;
			}
			index++;
		}
#endif
	}
}

void fx_Array_prototype_pop(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	if (array) {
		txSlot* address;
		if (length > 0) {
			length--;
			address = array->value.array.address + length;
			mxResult->kind = address->kind;
			mxResult->value = address->value;
			fxSetArrayLength(the, array, length);
		}
	}
	else {
#if mxProxy
		if (length > 0) {
			length--;
			mxPushSlot(mxThis);
			fxGetID(the, length);
			mxPullSlot(mxResult);
			mxPushSlot(mxThis);
			fxDeleteID(the, length);
			mxPop();
		}
		mxPushInteger(length);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_length));
		mxPop();
#endif
	}
}

void fx_Array_prototype_push(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txIndex c = mxArgc, i = 0;
	if ((txIndex)(XS_MAX_INDEX - c) < length)
		mxRangeError("array overflow");
	if (array) {
		txSlot* address;
		fxSetArrayLength(the, array, length + (txIndex)c);
		address = array->value.array.address + length;
		while (i < c) {
			txSlot* argument = mxArgv(i);
			address->ID = XS_NO_ID;
			address->kind = argument->kind;
			address->value = argument->value;
			address++;
			i++;
		}
	}
	else {
#if mxProxy
		while (i < c) {
			mxPushSlot(mxArgv(i));
			mxPushSlot(mxThis);
			fxSetID(the, length + i);
			mxPop();
			i++;
		}
		mxPushInteger(length + c);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_length));
#endif
	}
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = length + c;
}

void fx_Array_prototype_reduce(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	if (array) {
		if (mxArgc > 1)
			*mxResult = *mxArgv(1);
		else {
			txBoolean flag = 0;
			while (!flag && (index < length)) {
				txSlot* slot = array->value.array.address + index;
				if (slot->ID) {
					mxResult->kind = slot->kind;
					mxResult->value = slot->value;
					flag = 1;
				}
				index++;
			}
			if (!flag)
				mxTypeError("no initial value");
		}
		while (index < length) {
			fxReduceArrayItem(the, function, array, index);
			index++;
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		if (mxArgc > 1)
			*mxResult = *mxArgv(1);
		else {
			txBoolean flag = 0;
			while (!flag && (index < length)) {
				if (fxHasProperty(the, instance, index)) {
					mxPushSlot(mxThis);
					fxGetID(the, index);
					mxPullSlot(mxResult);
					flag = 1;
				}
				index++;
			}
			if (!flag)
				mxTypeError("no initial value");
		}
		while (index < length) {
			fxReduceProxyItem(the, function, instance, index);
			index++;
		}
#endif
	}
}

void fx_Array_prototype_reduceRight(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = length - 1;
	if (array) {
		if (mxArgc > 1)
			*mxResult = *mxArgv(1);
		else {
			txBoolean flag = 0;
			while (!flag && (index >= 0)) {
				txSlot* slot = array->value.array.address + index;
				if (slot->ID) {
					mxResult->kind = slot->kind;
					mxResult->value = slot->value;
					flag = 1;
				}
				index--;
			}
			if (!flag)
				mxTypeError("no initial value");
		}
		while (index >= 0) {
			fxReduceArrayItem(the, function, array, index);
			index--;
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		if (mxArgc > 1)
			*mxResult = *mxArgv(1);
		else {
			txBoolean flag = 0;
			while (!flag && (index >= 0)) {
				if (fxHasProperty(the, instance, index)) {
					mxPushSlot(mxThis);
					fxGetID(the, index);
					mxPullSlot(mxResult);
					flag = 1;
				}
				index--;
			}
			if (!flag)
				mxTypeError("no initial value");
		}
		while (index >= 0) {
			fxReduceProxyItem(the, function, instance, index);
			index--;
		}
#endif
	}
}

void fx_Array_prototype_reverse(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	if (array) {
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
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		txSlot* lowerSlot;
		txSlot* upperSlot;
		txIndex middle = length / 2;
		txIndex lower = 0;
		while (lower != middle) {
			txIndex upper = length - lower - 1;
			if (fxHasProperty(the, instance, lower)) {
				mxPushSlot(mxThis);
				fxGetID(the, lower);
				lowerSlot = the->stack;
			}
			else
				lowerSlot = C_NULL;
			if (fxHasProperty(the, instance, upper)) {
				mxPushSlot(mxThis);
				fxGetID(the, upper);
				upperSlot = the->stack;
			}
			else
				upperSlot = C_NULL;
			if (upperSlot && lowerSlot) {
				mxPushSlot(upperSlot);
				mxPushSlot(mxThis);
				fxSetID(the, lower);
				mxPop();
				mxPushSlot(lowerSlot);
				mxPushSlot(mxThis);
				fxSetID(the, upper);
				mxPop();
				mxPop();
				mxPop();
			}
			else if (upperSlot) {
				mxPushSlot(upperSlot);
				mxPushSlot(mxThis);
				fxSetID(the, lower);
				mxPop();
				mxPushSlot(mxThis);
				fxDeleteID(the, upper);
				mxPop();
				mxPop();
			}
			else if (lowerSlot) {
				mxPushSlot(mxThis);
				fxDeleteID(the, lower);
				mxPop();
				mxPushSlot(lowerSlot);
				mxPushSlot(mxThis);
				fxSetID(the, upper);
				mxPop();
				mxPop();
			}
			lower++;
		}
#endif
	}
	*mxResult = *mxThis;
}

void fx_Array_prototype_shift(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	if (array) {
		if (length > 0) {
			txSlot* address = array->value.array.address;
			length--;
			mxResult->kind = address->kind;
			mxResult->value = address->value;
			c_memmove(address, address + 1, length * sizeof(txSlot));
			fxSetArrayLength(the, array, length);
		}
	}
	else {
#if mxProxy
		if (length > 0) {
			txSlot* instance = mxThis->value.reference;
			txIndex index = 1;
			mxPushSlot(mxThis);
			fxGetID(the, 0);
			mxPullSlot(mxResult);
			while (index < length) {
				//mxPushSlot(mxThis);
				if (fxHasProperty(the, instance, index)) {
					mxPushSlot(mxThis);
					fxGetID(the, index);
					mxPushSlot(mxThis);
					fxSetID(the, index - 1);
					mxPop();
				}
				else {
					mxPushSlot(mxThis);
					fxDeleteID(the, index - 1);
					mxPop();
				}
				index++;
			}
		}
		mxPushSlot(mxThis);
		fxDeleteID(the, length - 1);
		mxPop();
		mxPushInteger(length - 1);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_length));
		mxPop();
#endif
	}
}

void fx_Array_prototype_slice(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txIndex start = fxArgToIndex(the, 0, 0, length);
	txIndex end = fxArgToIndex(the, 1, length, length);
	txIndex count = (end > start) ? end - start : 0;
	txSlot* resultArray = fxConstructArrayResult(the, C_NULL, count);
	if (array && resultArray) {
		c_memcpy(resultArray->value.array.address, array->value.array.address + start, count * sizeof(txSlot));
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		txIndex index = 0;
		while (start < end) {
			if (fxHasProperty(the, instance, start)) {
				mxPushSlot(mxThis);
				fxGetID(the, start);
				fxDefineDataProperty(the, mxResult->value.reference, index, the->stack);
			}
			index++;
			start++;
		}
		mxPushInteger(count);
		mxPushSlot(mxResult);
		fxSetID(the, mxID(_length));
#endif
	}
}

void fx_Array_prototype_some(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (array) {
		while (index < length) {
			if (fxCallArrayItem(the, function, array, index, C_NULL)) {
				mxResult->value.boolean = fxToBoolean(the, the->stack++);
				if (mxResult->value.boolean)
					break;
			}
			index++;
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		while (index < length) {
			if (fxCallProxyItem(the, function, instance, index, C_NULL)) {
				mxResult->value.boolean = fxToBoolean(the, the->stack++);
				if (mxResult->value.boolean)
					break;
			}
			index++;
		}
#endif
	}
}

void fx_Array_prototype_sort(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txSlot* function = C_NULL;
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
	if (array) {
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
	}
	else {
		mxUnknownError("TBD");
	}
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}


void fx_Array_prototype_splice(txMachine* the)
{
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txIndex start = fxArgToIndex(the, 0, 0, length);
	txInteger c = mxArgc, i, ic, dc;
	txSlot* resultArray;
	if (c == 0) {
		ic = 0;
		dc = 0;
	}
	else if (c == 1) {
		ic = 0;
		dc = length - start;
	}
	else {
		ic = c - 2;
		dc = fxToInteger(the, mxArgv(1));
		if (dc < 0)
			dc = 0;
		else if (dc > (length - start))
			dc = length - start;
	}
	if ((txIndex)(XS_MAX_INDEX - ic + dc) < length)
		mxRangeError("array overflow");
	resultArray = fxConstructArrayResult(the, C_NULL, dc);
	if (array && resultArray) {
		txSlot* address;
		c_memcpy(resultArray->value.array.address, array->value.array.address + start, dc * sizeof(txSlot));
		if (ic < dc) {
			c_memmove(array->value.array.address + start + ic, array->value.array.address + start + dc, (length - (start + dc)) * sizeof(txSlot));
			fxSetArrayLength(the, array, length - (dc - ic));
		}
		else if (ic > dc) {
			fxSetArrayLength(the, array, length + (ic - dc));
			c_memmove(array->value.array.address + start + ic, array->value.array.address + start + dc, (length - (start + dc)) * sizeof(txSlot));
		}
		address = array->value.array.address + start;
		i = 2;
		while (i < c) {
			txSlot* argument = mxArgv(i);
			address->ID = XS_NO_ID;
			address->kind = argument->kind;
			address->value = argument->value;
			address++;
			i++;
		}
	}
	else {	
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		i = 0;
		while (i < dc) {
			txIndex from = start + i;
			if (fxHasProperty(the, instance, from)) {
				mxPushSlot(mxThis);
				fxGetID(the, from);
				mxPushSlot(mxResult);
				fxSetID(the, i);
				mxPop();
			}
			i++;
		}
		mxPushInteger(dc);
		mxPushSlot(mxResult);
		fxSetID(the, mxID(_length));
		mxPop();
		if (ic < dc) {
			i = start;
			while (i < (length - dc)) {
				fxMoveProxyItem(the, instance, i + dc, i + ic);
				i++;
			}
			i = length;
			while (i > (length - dc + ic)) {
				mxPushSlot(mxThis);
				fxDeleteID(the, i - 1);
				mxPop();
				i--;
			}
		}
		else if (ic > dc) {
			i = length - dc;
			while (i > start) {
				fxMoveProxyItem(the, instance, i + dc - 1, i + ic - 1);
				i--;
			}
		}
		i = 0;
		while (i < ic) {
			mxPushSlot(mxArgv(2 + i));
			mxPushSlot(mxThis);
			fxSetID(the, start + i);
			mxPop();
			i++;
		}
		mxPushInteger(length - dc + ic);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_length));
		mxPop();
#endif
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
	txIndex length;
	txSlot* array = fxCoerceToArray(the, mxThis, &length);
	txIndex c = mxArgc;
	if ((txIndex)(XS_MAX_INDEX - c) < length)
		mxRangeError("array overflow");
	if (array) {
		if (c > 0) {
			txSlot* address;
            txIndex i = 0;
			fxSetArrayLength(the, array, length + (txIndex)c);
			address = array->value.array.address;
			c_memmove(address + c, address, length * sizeof(txSlot));
			while (i < c) {
				txSlot* argument = mxArgv(i);
				address->ID = XS_NO_ID;
				address->kind = argument->kind;
				address->value = argument->value;
				address++;
				i++;
			}
		}
	}
	else {
#if mxProxy
		txSlot* instance = mxThis->value.reference;
		txIndex i = length;
		while (i > 0) {
			fxMoveProxyItem(the, instance, i - 1, i + c - 1);
			i--;
		}
		i = 0;
		while (i < c) {
			mxPushSlot(mxArgv(i));
			mxPushSlot(mxThis);
			fxSetID(the, i);
			mxPop();
			i++;
		}
		mxPushInteger(length + c);
		mxPushSlot(mxThis);
		fxSetID(the, mxID(_length));
		mxPop();
#endif
	}
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = length + (txIndex)c;
}

void fx_Array_prototype_values(txMachine* the)
{
	mxPush(mxArrayValuesIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Array_prototype_values_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	txIndex length;
	txSlot* array = fxCoerceToArray(the, iterable, &length);
	txInteger i = index->value.integer;
	if (i < length) {
		txSlot* item = array->value.array.address + i;
		if (item->ID) {
			value->kind = item->kind;
			value->value = item->value;
		}
		else
			value->kind = XS_UNDEFINED_KIND;
		index->value.integer = i + 1;
	}
	else {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
		index->value.integer = i;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}


txSlot* fxNewParametersInstance(txMachine* the, txInteger count)
{
	txSlot* instance;
	txSlot* array;
	txSlot* property;
	txIndex index;
	txSlot* address;
	txInteger length = mxArgc;
	txByte* code = the->code;
	the->code = C_NULL;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_VALUE_FLAG;
	array = instance->next = fxNewSlot(the);
	array->kind = XS_PARAMETERS_KIND;
	array->value.array.length = 0;
	array->value.array.address = C_NULL;
	property = fxNextIntegerProperty(the, array, length, mxID(_length), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, mxFunction, mxID(_callee), XS_DONT_ENUM_FLAG);
	property = fxNextNullProperty(the, property, mxID(_caller), XS_DONT_ENUM_FLAG);
	property = fxNextHostFunctionProperty(the, property, fx_Array_prototype_values, 0, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	the->code = code;
	fxSetArrayLength(the, array, length);
	index = 0;
	address = array->value.array.address;
	property = the->scope + count;
	while ((index < length) && (index < count)) {
		address->ID = XS_NO_ID;
		address->kind = property->kind;
		address->value = property->value;
		index++;
		address++;
		property--;
	}
	while (index < length) {
		property = mxArgv(index);
		address->ID = XS_NO_ID;
		address->kind = property->kind;
		address->value = property->value;
		index++;
		address++;
	}
	return instance;
}

txSlot* fxGetParametersProperty(txMachine* the, txSlot* instance, txInteger index) 
{
	txSlot* array = instance->next;
	txSlot* result;
	if (index < array->value.array.length) {
		result = array->value.array.address + index;
		if (result->ID) {
			if (result->kind == XS_CLOSURE_KIND)
				return result->value.closure;
			return result;
		}
	}
	return C_NULL;
}

txBoolean fxRemoveParametersProperty(txMachine* the, txSlot* instance, txInteger index) 
{
	txSlot* array = instance->next;
	txSlot* result;
	if (index < array->value.array.length) {
		result = array->value.array.address + index;
		if (result->ID) {
			result->ID = 0;
			result->kind = XS_UNDEFINED_KIND;
			return 1;
		}
	}
	return 0;
}

txSlot* fxSetParametersProperty(txMachine* the, txSlot* instance, txInteger index) 
{
	txSlot* array = instance->next;
	txSlot* result;
	if (index < array->value.array.length) {
		result = array->value.array.address + index;
		if (result->ID) {
			if (result->kind == XS_CLOSURE_KIND)
				return result->value.closure;
			return result;
		}
	}
	return C_NULL;
}

txSlot* fxNewArgumentsStrictInstance(txMachine* the, txInteger count)
{
	txSlot* instance;
	txSlot* array;
	txSlot* property;
	txIndex index;
	txSlot* address;
	txInteger length = mxArgc;
	txByte* code = the->code;
	the->code = C_NULL;
	instance = fxNewObjectInstance(the);
	array = instance->next = fxNewSlot(the);
	array->kind = XS_ARRAY_KIND;
	array->value.array.length = 0;
	array->value.array.address = C_NULL;
	property = fxNextIntegerProperty(the, array, length, mxID(_length), XS_GET_ONLY);
	property = fxNextHostAccessorProperty(the, property, fx_Arguments_prototype_callee, fx_Arguments_prototype_callee, mxID(_callee), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	property = fxNextHostAccessorProperty(the, property, fx_Arguments_prototype_caller, fx_Arguments_prototype_caller, mxID(_caller), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	property = fxNextHostFunctionProperty(the, property, fx_Array_prototype_values, 0, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	the->code = code;
	count = mxArgc;
	fxSetArrayLength(the, array, length);
	index = 0;
	address = array->value.array.address;
	while (index < length) {
		property = mxArgv(index);
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
