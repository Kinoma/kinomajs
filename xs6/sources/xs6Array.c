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

static txBoolean fxCallArrayItem(txMachine* the, txSlot* function, txSlot* array, txIndex index, txSlot* item);
static txSlot* fxCoerceToArray(txMachine* the, txSlot* slot);
static int fxCompareArrayItem(txMachine* the, txSlot* function, txSlot* array, txInteger i);
static txSlot* fxConstructArrayResult(txMachine* the, txSlot* constructor, txUnsigned length);
static void fxReduceArrayItem(txMachine* the, txSlot* function, txSlot* array, txIndex index);

static void fx_Array(txMachine* the);
static void fx_Array_from(txMachine* the);
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
		{ fx_Array_prototype_join, 0, _toString },
		{ fx_Array_prototype_join, 0, _toLocaleString },
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
	slot = fxNextStringProperty(the, slot, "Array", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
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
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_Array, 1, mxID(_Array), XS_GET_ONLY));
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
	slot = fxNextHostAccessorProperty(the, slot, fx_Arguments_prototype_callee, fx_Arguments_prototype_callee, mxID(_callee), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_Arguments_prototype_caller, fx_Arguments_prototype_caller, mxID(_caller), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_Array_prototype_values, 0, mxID(_Symbol_iterator), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Arguments", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxArgumentsStrictPrototype = *the->stack;
	the->stack++;
}

txIndex fxArgToIndex(txMachine* the, txInteger argi, txIndex index, txIndex length)
{
	if (mxArgc > argi) {
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

txSlot* fxCoerceToArray(txMachine* the, txSlot* slot)
{
	txSlot* instance = fxToInstance(the, slot);
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
		mxPushSlot(constructor);
		fxGetID(the, mxID(_Symbol_species));
	}
	else {
		txSlot* instance = mxThis->value.reference;
		if (mxIsArray(instance)) {
			mxPushSlot(mxThis);
			fxGetID(the, mxID(_constructor));
			if (mxIsReference(the->stack) && mxIsFunction(the->stack->value.reference))
				fxGetID(the, mxID(_Symbol_species));
			else
				the->stack->kind = XS_UNDEFINED_KIND;
		}
		else
			mxPushUndefined();
	}
	if (the->stack->kind == XS_NULL_KIND)
		the->stack->kind = XS_UNDEFINED_KIND;
	if (the->stack->kind == XS_UNDEFINED_KIND) {
		*the->stack = mxGlobal;
		fxGetID(the, mxID(_Array));
	}
	fxNew(the);
	mxPullSlot(mxResult);
	return mxResult->value.reference->next;
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
		array = fxCoerceToArray(the, mxThis);
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
	txSlot* list = fxNewInstance(the);
	txSlot* item = list;
	txIndex count = 0;
	txIndex index;
	txSlot* function = C_NULL;
	txSlot* _this = C_NULL;
	txSlot* resultArray;
	txSlot* resultSlot;
	if (mxArgc > 2)
		_this = mxArgv(2);
	if (mxArgc > 1) {
		txSlot* slot = mxArgv(1);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			if ((slot->next->kind == XS_CODE_KIND) || (slot->next->kind == XS_CALLBACK_KIND))
				function = slot;
		}
	}
	if (mxArgc > 0) {
		mxPushSlot(mxArgv(0));
		if (fxHasID(the, mxID(_Symbol_iterator))) {
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
						if (function) {
							/* ARG1 */
							mxPushInteger(count);
							/* ARGC */
							mxPushInteger(2);
							/* THIS */
							if (_this)
								mxPushSlot(_this);
							else
								mxPushUndefined();
							/* FUNCTION */
							mxPushReference(function);
							fxCall(the);
						}
						item = fxNextSlotProperty(the, item, the->stack, XS_NO_ID, XS_NO_FLAG);
						the->stack++;
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
				/* ARG0 */
				mxPushSlot(mxArgv(0));
				fxGetID(the, index);
				if (function) {
					/* ARG1 */
					mxPushInteger(index);
					/* ARGC */
					mxPushInteger(2);
					/* THIS */
					if (_this)
						mxPushSlot(_this);
					else
						mxPushUndefined();
					/* FUNCTION */
					mxPushReference(function);
					fxCall(the);
				}
				item = fxNextSlotProperty(the, item, the->stack, XS_NO_ID, XS_NO_FLAG);
				the->stack++;
				index++;
			}	
		}
	}
	resultArray = fxConstructArrayResult(the, mxThis, count);
	item = list->next;
	index = 0;
	while (item) {
		resultSlot = resultArray->value.array.address + index;
		resultSlot->ID = XS_NO_ID;
		resultSlot->kind = item->kind;
		resultSlot->value = item->value;
		the->stack++;
		item = item->next;
		index++;
	}
	the->stack++;
}

void fx_Array_isArray(txMachine* the)
{
	txSlot* slot;

	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc > 0) {
		slot = fxGetInstance(the, mxArgv(0));
	again:
		if (slot->flag & XS_VALUE_FLAG) {
			slot = slot->next;
			if (slot->kind == XS_ARRAY_KIND)
				mxResult->value.boolean = 1;
			else if (slot->kind == XS_PROXY_KIND) {
				slot = slot->value.proxy.target;
				goto again;
			}
		}
		
	}
}

void fx_Array_of(txMachine* the)
{
	txIndex count = (txIndex)mxArgc;
	txSlot* resultArray = fxConstructArrayResult(the, mxThis, count);
	txSlot* resultSlot = resultArray->value.array.address;
	txIndex index = 0;
	while (index < count) {
		txSlot* argument = mxArgv(index);
		resultSlot->ID = XS_NO_ID;
		resultSlot->kind = argument->kind;
		resultSlot->value = argument->value;
		resultSlot++;
		index++;
	}
}

void fx_Array_prototype_concat(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txSlot* slot;
	txIndex length = array->value.array.length;
	txIndex count, index;
	txInteger c = mxArgc, i,  ii, ic;
	txSlot* argument;
	txSlot* resultArray;
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
	resultArray = fxConstructArrayResult(the, C_NULL, count);
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
						resultSlot = resultArray->value.array.address + count;
						resultSlot->ID = XS_NO_ID;
						resultSlot->kind = slot->kind;
						resultSlot->value = slot->value;
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

void fx_Array_prototype_copyWithin(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txIndex target = fxArgToIndex(the, 0, 0, length);
	txIndex start = fxArgToIndex(the, 1, 0, length);
	txIndex end = fxArgToIndex(the, 2, length, length);
	txIndex count = end - start;
	if (count > length - target)
		count = length - target;
	c_memmove(array->value.array.address + target, array->value.array.address + start, count * sizeof(txSlot));
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_Array_prototype_entries(txMachine* the)
{
	mxPush(mxArrayEntriesIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Array_prototype_entries_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	txSlot* array = fxCoerceToArray(the, iterable);
	txIndex length = array->value.array.length;
	txIndex i = index->value.integer;
	while (i < length) {
		txSlot* item = array->value.array.address + i;
		if (item->ID) {
			mxPushInteger(i);
			mxPushSlot(item);
			fxConstructArrayEntry(the, value);
			index->value.integer = i + 1;
			break;
		}
		i++;
	}
	if (i == length) {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

void fx_Array_prototype_every(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	while (index < length) {
		if (fxCallArrayItem(the, function, array, index, C_NULL)) {
			mxResult->value.boolean = fxToBoolean(the, the->stack++);
			if (!mxResult->value.boolean)
				break;
		}
		index++;
	}
}

void fx_Array_prototype_fill(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txIndex start = fxArgToIndex(the, 1, 0, length);
	txIndex end = fxArgToIndex(the, 2, length, length);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	while (start < end) {
		txSlot* slot = array->value.array.address + start;
		slot->ID = XS_NO_ID;
		slot->kind = the->stack->kind;
		slot->value = the->stack->value;
		start++;
	}
	the->stack++;
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_Array_prototype_filter(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txIndex index = 0;
	txIndex count = 0;
	txSlot* resultArray;
	txSlot* resultSlot;
	mxPushUndefined();
	while (index < length) {
		if (fxCallArrayItem(the, function, array, index, the->stack)) {
			if (fxToBoolean(the, the->stack++)) {
				count++;
				slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			}
		}
		index++;
	}
	the->stack++;
	resultArray = fxConstructArrayResult(the, C_NULL, count);
	resultSlot = resultArray->value.array.address;
	slot = list->next;
	while (slot) {
		resultSlot->ID = XS_NO_ID;
		resultSlot->kind = slot->kind;
		resultSlot->value = slot->value;
		resultSlot++;
		slot = slot->next;
	}
	the->stack++;
}

void fx_Array_prototype_find(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	mxPushUndefined();
	while (index < length) {
		if (fxCallArrayItem(the, function, array, index, the->stack)) {
			if (fxToBoolean(the, the->stack++)) {
				mxResult->kind = the->stack->kind;
				mxResult->value = the->stack->value;
				break;
			}
		}
		index++;
	}
	the->stack++;
}

void fx_Array_prototype_findIndex(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	fxInteger(the, mxResult, -1);
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

void fx_Array_prototype_forEach(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	while (index < length) {
		fxCallArrayItem(the, function, array, index, C_NULL);
		index++;
	}
}

void fx_Array_prototype_indexOf(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txIndex index = fxArgToIndex(the, 1, 0, length);
	fxInteger(the, mxResult, -1);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	while (index < length) {
		txSlot* slot = array->value.array.address + index;
		if (slot->ID) {
			if (fxIsSameSlot(the, slot, the->stack)) {
				fxUnsigned(the, mxResult, index);
				break;
			}
		}
		index++;
	}
	the->stack++;
}

void fx_Array_prototype_join(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txString string;
	txIndex index = 0;
	txBoolean comma = 0;
	txInteger size = 0;
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txSlot* item;
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

void fx_Array_prototype_keys(txMachine* the)
{
	mxPush(mxArrayKeysIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Array_prototype_keys_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	txSlot* array = fxCoerceToArray(the, iterable);
	txIndex length = array->value.array.length;
	txIndex i = index->value.integer;
	while (i < length) {
		txSlot* item = array->value.array.address + i;
		if (item->ID) {
			value->kind = XS_INTEGER_KIND;
			value->value.integer = i;
			index->value.integer = i + 1;
			break;
		}
		i++;
	}
	if (i == length) {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

void fx_Array_prototype_lastIndexOf(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txIndex index = fxArgToIndex(the, 1, length, length);
	txSlot* slot;
	fxInteger(the, mxResult, -1);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
    if (index == length)
        index--;
	while (index >= 0) {
		slot = array->value.array.address + index;
		if (slot->ID) {
			if (fxIsSameSlot(the, slot, the->stack)) {
				fxUnsigned(the, mxResult, index);
				break;
			}
		}
        index--;
	}
	the->stack++;
}

void fx_Array_prototype_length_get(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
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
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
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
}

void fx_Array_prototype_map(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* resultArray = fxConstructArrayResult(the, C_NULL, length);
	txIndex index = 0;
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

void fx_Array_prototype_pop(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* address;
	if (length > 0) {
		length--;
		address = array->value.array.address + length;
		mxResult->kind = address->kind;
		mxResult->value = address->value;
		fxSetArrayLength(the, array, length);
	}
}

void fx_Array_prototype_push(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* address;
	txSlot* argument;
	txInteger c = mxArgc, i = 0;
	if (c > 0) {
		if ((txIndex)(XS_MAX_INDEX - c) < length)
			mxRangeError("array overflow");
		fxSetArrayLength(the, array, length + (txIndex)c);
		address = array->value.array.address + length;
		while (i < c) {
			argument = mxArgv(i);
			address->ID = XS_NO_ID;
			address->kind = argument->kind;
			address->value = argument->value;
			address++;
			i++;
		}
	}
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = length + (txIndex)c;
}

void fx_Array_prototype_reduce(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else if (index < length) {
		txSlot* slot;
		while (index < length) {
			slot = array->value.array.address + index;
			index++;
			if (slot->ID) {
				mxResult->kind = slot->kind;
				mxResult->value = slot->value;
				break;
			}
		}
	}
	else
		mxTypeError("no initial value");
	while (index < length) {
		fxReduceArrayItem(the, function, array, index);
		index++;
	}
}

void fx_Array_prototype_reduceRight(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = length;
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else if (index > 0) {
		txSlot* slot;
		while (index > 0) {
			index--;
			slot = array->value.array.address + index;
			if (slot->ID) {
				mxResult->kind = slot->kind;
				mxResult->value = slot->value;
				break;
			}
		}
	}
	else
		mxTypeError("no initial value");
	while (index > 0) {
		index--;
		fxReduceArrayItem(the, function, array, index);
	}
}

void fx_Array_prototype_reverse(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
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
	}
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_Array_prototype_shift(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* address;
	if (length > 0) {
		length--;
		address = array->value.array.address;
		mxResult->kind = address->kind;
		mxResult->value = address->value;
		c_memmove(address, address + 1, length * sizeof(txSlot));
		fxSetArrayLength(the, array, length);
	}
}

void fx_Array_prototype_slice(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txIndex start = fxArgToIndex(the, 0, 0, length);
	txIndex end = fxArgToIndex(the, 1, length, length);
	txIndex count = (end > start) ? end - start : 0;
	txSlot* resultArray = fxConstructArrayResult(the, C_NULL, count);
	c_memcpy(resultArray->value.array.address, array->value.array.address + start, count * sizeof(txSlot));
}

void fx_Array_prototype_some(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* function = fxArgToCallback(the, 0);
	txIndex index = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	while (index < length) {
		if (fxCallArrayItem(the, function, array, index, C_NULL)) {
			mxResult->value.boolean = fxToBoolean(the, the->stack++);
			if (mxResult->value.boolean)
				break;
		}
		index++;
	}
}

void fx_Array_prototype_sort(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
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
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_Array_prototype_splice(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txIndex start = fxArgToIndex(the, 0, 0, length);
	txInteger c = mxArgc, i, ic, dc;
	txSlot* resultArray;
	txSlot* argument;
	txSlot* address;
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
		argument = mxArgv(i);
		address->ID = XS_NO_ID;
		address->kind = argument->kind;
		address->value = argument->value;
		address++;
		i++;
	}
}

void fx_Array_prototype_unshift(txMachine* the)
{
	txSlot* array = fxCoerceToArray(the, mxThis);
	txIndex length = array->value.array.length;
	txSlot* address;
	txSlot* argument;
	txInteger c = mxArgc, i = 0;
	if (c > 0) {
		if ((txIndex)(XS_MAX_INDEX - c) < length)
			mxRangeError("array overflow");
		fxSetArrayLength(the, array, length + (txIndex)c);
		address = array->value.array.address;
		c_memmove(address + c, address, length * sizeof(txSlot));
		while (i < c) {
			argument = mxArgv(i);
			address->ID = XS_NO_ID;
			address->kind = argument->kind;
			address->value = argument->value;
			address++;
			i++;
		}
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
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	txSlot* array = fxCoerceToArray(the, iterable);
	txIndex length = array->value.array.length;
	txInteger i = index->value.integer;
	while (i < length) {
		txSlot* item = array->value.array.address + i;
		if (item->ID) {
			value->kind = item->kind;
			value->value = item->value;
			index->value.integer = i + 1;
			break;
		}
		i++;
	}
	if (i == length) {
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
