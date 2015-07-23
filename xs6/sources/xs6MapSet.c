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

static txSlot* fxCheckMapInstance(txMachine* the, txSlot* slot);
static txSlot* fxCheckMapKey(txMachine* the);
static void fx_Map(txMachine* the);
static void fx_Map_prototype_clear(txMachine* the);
static void fx_Map_prototype_delete(txMachine* the);
static void fx_Map_prototype_entries(txMachine* the);
static void fx_Map_prototype_entries_next(txMachine* the);
static void fx_Map_prototype_forEach(txMachine* the);
static void fx_Map_prototype_get(txMachine* the);
static void fx_Map_prototype_has(txMachine* the);
static void fx_Map_prototype_keys(txMachine* the);
static void fx_Map_prototype_keys_next(txMachine* the);
static void fx_Map_prototype_set(txMachine* the);
static void fx_Map_prototype_size(txMachine* the);
static void fx_Map_prototype_values(txMachine* the);
static void fx_Map_prototype_values_next(txMachine* the);

static txSlot* fxCheckSetInstance(txMachine* the, txSlot* slot);
static txSlot* fxCheckSetValue(txMachine* the);
static void fx_Set(txMachine* the);
static void fx_Set_prototype_add(txMachine* the);
static void fx_Set_prototype_clear(txMachine* the);
static void fx_Set_prototype_delete(txMachine* the);
static void fx_Set_prototype_entries(txMachine* the);
static void fx_Set_prototype_entries_next(txMachine* the);
static void fx_Set_prototype_forEach(txMachine* the);
static void fx_Set_prototype_has(txMachine* the);
static void fx_Set_prototype_keys(txMachine* the);
static void fx_Set_prototype_size(txMachine* the);
static void fx_Set_prototype_values(txMachine* the);

static txSlot* fxCheckWeakMapInstance(txMachine* the, txSlot* slot);
static txSlot* fxCheckWeakMapKey(txMachine* the);
static void fx_WeakMap(txMachine* the);
static void fx_WeakMap_prototype_delete(txMachine* the);
static void fx_WeakMap_prototype_get(txMachine* the);
static void fx_WeakMap_prototype_has(txMachine* the);
static void fx_WeakMap_prototype_set(txMachine* the);

static txSlot* fxCheckWeakSetInstance(txMachine* the, txSlot* slot);
static txSlot* fxCheckWeakSetValue(txMachine* the);
static void fx_WeakSet(txMachine* the);
static void fx_WeakSet_prototype_add(txMachine* the);
static void fx_WeakSet_prototype_delete(txMachine* the);
static void fx_WeakSet_prototype_has(txMachine* the);

static txBoolean fxAdjustEntries(txMachine* the, txSlot* array, txInteger length);
static txInteger fxCountEntries(txMachine* the, txSlot* array);
static txInteger fxDeleteEntry(txMachine* the, txSlot* table, txSlot* array, txSlot* slot); 
static txInteger fxGetEntry(txMachine* the, txSlot* table, txSlot* array, txSlot* slot); 
static txInteger fxSetEntry(txMachine* the, txSlot* table, txSlot* array, txSlot* slot); 
static txU4 fxSumEntry(txMachine* the, txSlot* slot); 

void fxBuildMapSet(txMachine* the)
{
    static const txHostFunctionBuilder gx_Map_prototype_builders[] = {
		{ fx_Map_prototype_clear, 0, _clear },
		{ fx_Map_prototype_delete, 1, _delete },
		{ fx_Map_prototype_entries, 0, _entries },
		{ fx_Map_prototype_forEach, 1, _forEach },
		{ fx_Map_prototype_get, 1, _get },
		{ fx_Map_prototype_has, 1, _has },
		{ fx_Map_prototype_keys, 0, _keys },
		{ fx_Map_prototype_set, 2, _set },
		{ fx_Map_prototype_values, 0, _values },
		{ fx_Map_prototype_entries, 0, _Symbol_iterator },
		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_Set_prototype_builders[] = {
		{ fx_Set_prototype_add, 1, _add },
		{ fx_Set_prototype_clear, 0, _clear },
		{ fx_Set_prototype_delete, 1, _delete },
		{ fx_Set_prototype_entries, 0, _entries },
		{ fx_Set_prototype_forEach, 1, _forEach },
		{ fx_Set_prototype_has, 1, _has },
		{ fx_Set_prototype_keys, 0, _keys },
		{ fx_Set_prototype_values, 0, _values },
		{ fx_Set_prototype_values, 0, _Symbol_iterator },
		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_WeakMap_prototype_builders[] = {
		{ fx_WeakMap_prototype_delete, 1, _delete },
		{ fx_WeakMap_prototype_get, 1, _get },
		{ fx_WeakMap_prototype_has, 1, _has },
		{ fx_WeakMap_prototype_set, 2, _set },
		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_WeakSet_prototype_builders[] = {
		{ fx_WeakSet_prototype_add, 1, _add },
		{ fx_WeakSet_prototype_delete, 1, _delete },
		{ fx_WeakSet_prototype_has, 1, _has },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	
	/* MAP */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewMapInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, fx_Map_prototype_size, C_NULL, mxID(_size), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	for (builder = gx_Map_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Map", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxMapPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_Map, 1, mxID(_Map), XS_GET_ONLY));
	slot = fxNextHostAccessorProperty(the, slot, fx_species_get, C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Map_prototype_entries_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Map Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxMapEntriesIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Map_prototype_keys_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Map Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxMapKeysIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Map_prototype_values_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Map Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxMapValuesIteratorPrototype);
	
	/* SET */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewSetInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, fx_Set_prototype_size, C_NULL, mxID(_size), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	for (builder = gx_Set_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Set", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxSetPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_Set, 1, mxID(_Set), XS_GET_ONLY));
	slot = fxNextHostAccessorProperty(the, slot, fx_species_get, C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Set_prototype_entries_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Set Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxSetEntriesIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Map_prototype_values_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Set Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxSetKeysIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Map_prototype_values_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Set Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxSetValuesIteratorPrototype);

	/* WEAK MAP */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewWeakMapInstance(the));
	for (builder = gx_WeakMap_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "WeakMap", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxWeakMapPrototype = *the->stack;
	fxNewHostConstructorGlobal(the, fx_WeakMap, 1, mxID(_WeakMap), XS_GET_ONLY);
	the->stack++;
	
	/* WEAK SET */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewWeakSetInstance(the));
	for (builder = gx_WeakSet_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "WeakSet", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxWeakSetPrototype = *the->stack;
	fxNewHostConstructorGlobal(the, fx_WeakSet, 1, mxID(_WeakSet), XS_GET_ONLY);
	the->stack++;
}

txSlot* fxCheckMapInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if (slot->next->kind == XS_MAP_KIND)
			return slot;
	}
	mxTypeError("this is no Map instance");
	return C_NULL;
}

txSlot* fxCheckMapKey(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		return slot;
	}
	mxTypeError("no key");
	return C_NULL;
}

txSlot* fxNewMapInstance(txMachine* the)
{
	txSlot* map;
	txSlot* slot;
	map = fxNewSlot(the);
	map->flag = XS_VALUE_FLAG;
	map->kind = XS_INSTANCE_KIND;
	map->value.instance.garbage = C_NULL;
	map->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = map;
	/* ENTRIES */
	slot = map->next = fxNewSlot(the);
	slot->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	slot->value.table.address = (txSlot**)fxNewChunk(the, 127 * sizeof(txSlot*));
	slot->value.table.length = 127;
	slot->kind = XS_MAP_KIND;
	c_memset(slot->value.table.address, 0, slot->value.table.length * sizeof(txSlot*));
	/* VALUES */
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	slot->value.array.address = C_NULL;
	slot->value.array.length = 0;
	slot->kind = XS_STACK_KIND;
	/* KEYS */
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	slot->value.array.address = C_NULL;
	slot->value.array.length = 0;
	slot->kind = XS_STACK_KIND;
 	return map;
}

void fx_Map(txMachine* the)
{
	txSlot *iterable, *iterator, *result, *value;
	fxCheckMapInstance(the, mxThis);
	if (mxArgc < 1)
		return;
	iterable = mxArgv(0);
	if ((iterable->kind == XS_UNDEFINED_KIND) || (iterable->kind == XS_NULL_KIND))
		return;
	mxCallID(iterable, mxID(_Symbol_iterator), 0);
	iterator = the->stack;
	{
		mxTry(the) {
			for (;;) {
				mxCallID(iterator, mxID(_next), 0);
				result = the->stack;
				mxGetID(result, mxID(_done));
				if (fxToBoolean(the, the->stack))
					break;
				the->stack++;
				mxGetID(result, mxID(_value));
				value = the->stack;
				if (value->kind != XS_REFERENCE_KIND)
					mxTypeError("item is no object");
				mxGetID(value, 0);
				mxGetID(value, 1);
				mxCallID(mxThis, mxID(_set), 2);
				the->stack++;
				the->stack++;
			}
		}
		mxCatch(the) {
			mxCallID(iterator, mxID(_return), 0);
			fxJump(the);
		}
	}
}

void fx_Map_prototype_clear(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* keys = values->next;
	txSlot* value;
	txSlot* key;
	txInteger index;
	c_memset(table->value.table.address, 0, table->value.table.length * sizeof(txSlot*));
	value = values->value.array.address;
	key = keys->value.array.address;
	index = values->value.array.length;
	while (index) {
		value->flag = XS_DONT_ENUM_FLAG;
		value->kind = XS_UNDEFINED_KIND;
		key->flag = XS_DONT_ENUM_FLAG;
		key->kind = XS_UNDEFINED_KIND;
		value++;
		key++;
		index--;
	}
}

void fx_Map_prototype_delete(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* keys = values->next;
	txSlot* key = fxCheckMapKey(the);
	txInteger index;
	txSlot* slot;
	index = fxDeleteEntry(the, table, keys, key);
	if (index >= 0) {
		slot = values->value.array.address + index;
		slot->flag = XS_DONT_ENUM_FLAG;
		slot->kind = XS_UNDEFINED_KIND;
		slot = keys->value.array.address + index;
		slot->flag = XS_DONT_ENUM_FLAG;
		slot->kind = XS_UNDEFINED_KIND;
		mxResult->value.boolean = 1;
	}
	else
		mxResult->value.boolean = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Map_prototype_entries(txMachine* the)
{
	fxCheckMapInstance(the, mxThis);
	mxPush(mxMapEntriesIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Map_prototype_entries_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* values = iterable->value.reference->next->next;
	txSlot* keys = values->next;
	txSlot *value, *key;
	txInteger c = values->value.array.length;
	txInteger i = index->value.integer;
	if (i < c) {
		value = values->value.array.address + i;
		while ((i < c) && (value->flag & XS_DONT_ENUM_FLAG)) {
			i++;
			value++;
		}
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (i < c) {
		key = keys->value.array.address + i;
		index->value.integer = i + 1;
		mxPushSlot(key);
		mxPushSlot(value);
		fxConstructArrayEntry(the, result);
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
	}
}

void fx_Map_prototype_forEach(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* keys = values->next;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	while (index < values->value.array.length) {
		txSlot* value = values->value.array.address + index;
		if (!(value->flag & XS_DONT_ENUM_FLAG)) {
			txSlot *key = keys->value.array.address + index;
			/* ARG0 */
			mxPushSlot(value);
			/* ARG1 */
			mxPushSlot(key);
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
			the->stack++;
		}
		index++;
	}
}

void fx_Map_prototype_get(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* keys = values->next;
	txSlot* key = fxCheckMapKey(the);
	txInteger index;
	txSlot* value;
	index = fxGetEntry(the, table, keys, key);
	if (index >= 0) {
		value = values->value.array.address + index;
		mxResult->kind = value->kind;
		mxResult->value = value->value;
	}
}

void fx_Map_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* keys = values->next;
	txSlot* key = fxCheckMapKey(the);
	txInteger index;
	index = fxGetEntry(the, table, keys, key);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (index >= 0) ? 1 : 0;
}

void fx_Map_prototype_keys(txMachine* the)
{
	fxCheckMapInstance(the, mxThis);
	mxPush(mxMapKeysIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Map_prototype_keys_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* keys = iterable->value.reference->next->next->next;
	txSlot* key;
	txInteger c = keys->value.array.length;
	txInteger i = index->value.integer;
	if (i < c) {
		key = keys->value.array.address + i;
		while ((i < c) && (key->flag & XS_DONT_ENUM_FLAG)) {
			i++;
			key++;
		}
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (i < c) {
		index->value.integer = i + 1;
		result->kind = key->kind;
		result->value = key->value;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
	}
}

void fx_Map_prototype_set(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* keys = values->next;
	txSlot* key = fxCheckMapKey(the);
	txInteger index;
	txSlot* slot;
	index = fxSetEntry(the, table, keys, key);
	if (fxAdjustEntries(the, keys, index + 1)) {
		slot = keys->value.array.address + index;
		slot->kind = key->kind;
		slot->value = key->value;
	}
	fxAdjustEntries(the, values, index + 1);
	slot = values->value.array.address + index;
	if (mxArgc > 1) {
		slot->kind = mxArgv(1)->kind;
		slot->value = mxArgv(1)->value;
	}
	else
		slot->kind = XS_UNDEFINED_KIND;
	*mxResult = *mxThis;
}

void fx_Map_prototype_size(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = fxCountEntries(the, values);
}

void fx_Map_prototype_values(txMachine* the)
{
	fxCheckMapInstance(the, mxThis);
	mxPush(mxMapValuesIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Map_prototype_values_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* values = iterable->value.reference->next->next;
	txSlot* value;
	txInteger c = values->value.array.length;
	txInteger i = index->value.integer;
	if (i < c) {
		value = values->value.array.address + i;
		while ((i < c) && (value->flag & XS_DONT_ENUM_FLAG)) {
			i++;
			value++;
		}
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (i < c) {
		index->value.integer = i + 1;
		result->kind = value->kind;
		result->value = value->value;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
	}
}

txSlot* fxCheckSetInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if (slot->next->kind == XS_SET_KIND)
			return slot;
	}
	mxTypeError("this is no Set instance");
	return C_NULL;
}

txSlot* fxCheckSetValue(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		return slot;
	}
	mxTypeError("no value");
	return C_NULL;
}

txSlot* fxNewSetInstance(txMachine* the)
{
	txSlot* set;
	txSlot* slot;
	set = fxNewSlot(the);
	set->flag = XS_VALUE_FLAG;
	set->kind = XS_INSTANCE_KIND;
	set->value.instance.garbage = C_NULL;
	set->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = set;
	/* ENTRIES */
	slot = set->next = fxNewSlot(the);
	slot->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	slot->value.table.address = (txSlot**)fxNewChunk(the, 127 * sizeof(txSlot*));
	slot->value.table.length = 127;
	slot->kind = XS_SET_KIND;
	c_memset(slot->value.table.address, 0, slot->value.table.length * sizeof(txSlot*));
	/* VALUES */
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	slot->value.array.address = C_NULL;
	slot->value.array.length = 0;
	slot->kind = XS_STACK_KIND;
 	return set;
}

void fx_Set(txMachine* the)
{
	txSlot *iterable, *iterator, *result;
	fxCheckSetInstance(the, mxThis);
	if (mxArgc < 1)
		return;
	iterable = mxArgv(0);
	if ((iterable->kind == XS_UNDEFINED_KIND) || (iterable->kind == XS_NULL_KIND))
		return;
	mxCallID(iterable, mxID(_Symbol_iterator), 0);
	iterator = the->stack;
	{
		mxTry(the) {
			for (;;) {
				mxCallID(iterator, mxID(_next), 0);
				result = the->stack;
				mxGetID(result, mxID(_done));
				if (fxToBoolean(the, the->stack))
					break;
				the->stack++;
				mxGetID(result, mxID(_value));
				mxCallID(mxThis, mxID(_add), 1);
				the->stack++;
			}
		}
		mxCatch(the) {
			mxCallID(iterator, mxID(_return), 0);
			fxJump(the);
		}
	}
}

void fx_Set_prototype_add(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* value = fxCheckSetValue(the);
	txInteger index;
	txSlot* slot;
	index = fxSetEntry(the, table, values, value);
	if (fxAdjustEntries(the, values, index + 1)) {
		slot = values->value.array.address + index;
		slot->kind = value->kind;
		slot->value = value->value;
	}
	*mxResult = *mxThis;
}

void fx_Set_prototype_clear(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txInteger index;
	txSlot* value;
	c_memset(table->value.table.address, 0, table->value.table.length * sizeof(txSlot*));
	value = values->value.array.address;
	index = values->value.array.length;
	while (index) {
		value->flag = XS_DONT_ENUM_FLAG;
		value->kind = XS_UNDEFINED_KIND;
		value++;
		index--;
	}
}

void fx_Set_prototype_delete(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* value = fxCheckSetValue(the);
	txInteger index;
	txSlot* slot;
	index = fxDeleteEntry(the, table, values, value);
	if (index >= 0) {
		slot = values->value.array.address + index;
		slot->flag = XS_DONT_ENUM_FLAG;
		slot->kind = XS_UNDEFINED_KIND;
		mxResult->value.boolean = 1;
	}
	else
		mxResult->value.boolean = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Set_prototype_entries(txMachine* the)
{
	fxCheckSetInstance(the, mxThis);
	mxPush(mxSetEntriesIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Set_prototype_entries_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* values = iterable->value.reference->next->next;
	txInteger c = values->value.array.length;
	txInteger i = index->value.integer;
	txSlot *value;
	if (i < c) {
		value = values->value.array.address + i;
		while ((i < c) && (value->flag & XS_DONT_ENUM_FLAG)) {
			i++;
			value++;
		}
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (i < c) {
		index->value.integer = i + 1;
		mxPushSlot(value);
		mxPushSlot(value);
		fxConstructArrayEntry(the, result);
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
	}
}

void fx_Set_prototype_forEach(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	while (index < values->value.array.length) {
		txSlot* value = values->value.array.address + index;
		if (!(value->flag & XS_DONT_ENUM_FLAG)) {
			/* ARG0 */
			mxPushSlot(value);
			/* ARG1 */
			mxPushSlot(value);
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
			the->stack++;
		}
		index++;
	}
}

void fx_Set_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* value = fxCheckSetValue(the);
	txInteger index;
	index = fxGetEntry(the, table, values, value);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (index >= 0) ? 1 : 0;
}

void fx_Set_prototype_keys(txMachine* the)
{
	fxCheckSetInstance(the, mxThis);
	mxPush(mxSetKeysIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Set_prototype_size(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = fxCountEntries(the, values);
}

void fx_Set_prototype_values(txMachine* the)
{
	fxCheckSetInstance(the, mxThis);
	mxPush(mxSetValuesIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

txSlot* fxCheckWeakMapInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if (slot->next->kind == XS_WEAK_MAP_KIND)
			return slot;
	}
	mxTypeError("this is no WeakMap instance");
	return C_NULL;
}

txSlot* fxCheckWeakMapKey(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND)
			return slot;
	}
	mxTypeError("key is no object");
	return C_NULL;
}

txSlot* fxNewWeakMapInstance(txMachine* the)
{
	txSlot* map;
	txSlot* table;
	txSlot* values;
	txSlot* keys;
	txSlot** address;
	map = fxNewSlot(the);
	map->flag = XS_VALUE_FLAG;
	map->kind = XS_INSTANCE_KIND;
	map->value.instance.garbage = C_NULL;
	map->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = map;
	table = map->next = fxNewSlot(the);
	values = table->next = fxNewSlot(the);
	keys = values->next = fxNewSlot(the);
	address = (txSlot**)fxNewChunk(the, 128 * sizeof(txSlot*)); // one more slot for the collector weak table list
	c_memset(address, 0, 128 * sizeof(txSlot*));
	/* ENTRIES */
	table->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	table->kind = XS_WEAK_MAP_KIND;
	table->value.table.address = address;
	table->value.table.length = 127;
	/* VALUES */
	values->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	values->kind = XS_WEAK_ARRAY_KIND;
	values->value.array.address = C_NULL;
	values->value.array.length = 0;
	/* KEYS */
	keys->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	keys->kind = XS_WEAK_ARRAY_KIND;
	keys->value.array.address = C_NULL;
	keys->value.array.length = 0;
 	return map;
}

void fx_WeakMap(txMachine* the)
{
	txSlot *iterable, *iterator, *result, *value;
	fxCheckWeakMapInstance(the, mxThis);
	if (mxArgc < 1)
		return;
	iterable = mxArgv(0);
	if ((iterable->kind == XS_UNDEFINED_KIND) || (iterable->kind == XS_NULL_KIND))
		return;
	mxCallID(iterable, mxID(_Symbol_iterator), 0);
	iterator = the->stack;
	{
		mxTry(the) {
			for (;;) {
				mxCallID(iterator, mxID(_next), 0);
				result = the->stack;
				mxGetID(result, mxID(_done));
				if (fxToBoolean(the, the->stack))
					break;
				the->stack++;
				mxGetID(result, mxID(_value));
				value = the->stack;
				mxGetID(value, 0);
				mxGetID(value, 1);
				mxCallID(mxThis, mxID(_set), 2);
				the->stack++;
				the->stack++;
			}
		}
		mxCatch(the) {
			mxCallID(iterator, mxID(_return), 0);
			fxJump(the);
		}
	}
}

void fx_WeakMap_prototype_delete(txMachine* the)
{
	txSlot* instance = fxCheckWeakMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* keys = values->next;
	txSlot* key = fxCheckWeakMapKey(the);
	txInteger index;
	txSlot* slot;
	index = fxDeleteEntry(the, table, keys, key);
	if (index >= 0) {
		slot = values->value.array.address + index;
		slot->flag = XS_DONT_ENUM_FLAG;
		slot->kind = XS_UNDEFINED_KIND;
		slot = keys->value.array.address + index;
		slot->flag = XS_DONT_ENUM_FLAG;
		slot->kind = XS_UNDEFINED_KIND;
		mxResult->value.boolean = 1;
	}
	else
		mxResult->value.boolean = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_WeakMap_prototype_get(txMachine* the)
{
	txSlot* instance = fxCheckWeakMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* keys = values->next;
	txSlot* key = fxCheckWeakMapKey(the);
	txInteger index;
	txSlot* value;
	index = fxGetEntry(the, table, keys, key);
	if (index >= 0) {
		value = values->value.array.address + index;
		mxResult->kind = value->kind;
		mxResult->value = value->value;
	}
}

void fx_WeakMap_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckWeakMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* keys = values->next;
	txSlot* key = fxCheckWeakMapKey(the);
	txInteger index;
	index = fxGetEntry(the, table, keys, key);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (index >= 0) ? 1 : 0;
}

void fx_WeakMap_prototype_set(txMachine* the)
{
	txSlot* instance = fxCheckWeakMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* keys = values->next;
	txSlot* key = fxCheckWeakMapKey(the);
	txInteger index;
	txSlot* slot;
	index = fxSetEntry(the, table, keys, key);
	if (fxAdjustEntries(the, keys, index + 1)) {
		slot = keys->value.array.address + index;
		slot->kind = key->kind;
		slot->value = key->value;
	}
	fxAdjustEntries(the, values, index + 1);
	slot = values->value.array.address + index;
	if (mxArgc > 1) {
		slot->kind = mxArgv(1)->kind;
		slot->value = mxArgv(1)->value;
	}
	else
		slot->kind = XS_UNDEFINED_KIND;
	*mxResult = *mxThis;
}

txSlot* fxCheckWeakSetInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if (slot->next->kind == XS_WEAK_SET_KIND)
			return slot;
	}
	mxTypeError("this is no WeakSet instance");
	return C_NULL;
}

txSlot* fxCheckWeakSetValue(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND)
			return slot;
	}
	mxTypeError("value is no object");
	return C_NULL;
}

txSlot* fxNewWeakSetInstance(txMachine* the)
{
	txSlot* set;
	txSlot* table;
	txSlot* values;
	txSlot** address;
	set = fxNewSlot(the);
	set->flag = XS_VALUE_FLAG;
	set->kind = XS_INSTANCE_KIND;
	set->value.instance.garbage = C_NULL;
	set->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = set;
	table = set->next = fxNewSlot(the);
	values = table->next = fxNewSlot(the);
	address = (txSlot**)fxNewChunk(the, 128 * sizeof(txSlot*)); // one more slot for the collector weak table list
	c_memset(address, 0, 128 * sizeof(txSlot*));
	/* ENTRIES */
	table->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	table->kind = XS_WEAK_SET_KIND;
	table->value.table.address = address;
	table->value.table.length = 127;
	/* VALUES */
	values->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	values->kind = XS_WEAK_ARRAY_KIND;
	values->value.array.address = C_NULL;
	values->value.array.length = 0;
 	return set;
}

void fx_WeakSet(txMachine* the)
{
	txSlot *iterable, *iterator, *result;
	fxCheckWeakSetInstance(the, mxThis);
	if (mxArgc < 1)
		return;
	iterable = mxArgv(0);
	if ((iterable->kind == XS_UNDEFINED_KIND) || (iterable->kind == XS_NULL_KIND))
		return;
	mxCallID(iterable, mxID(_Symbol_iterator), 0);
	iterator = the->stack;
	{
		mxTry(the) {
			for (;;) {
				mxCallID(iterator, mxID(_next), 0);
				result = the->stack;
				mxGetID(result, mxID(_done));
				if (fxToBoolean(the, the->stack))
					break;
				the->stack++;
				mxGetID(result, mxID(_value));
				mxCallID(mxThis, mxID(_add), 1);
				the->stack++;
			}
		}
		mxCatch(the) {
			mxCallID(iterator, mxID(_return), 0);
			fxJump(the);
		}
	}
}

void fx_WeakSet_prototype_add(txMachine* the)
{
	txSlot* instance = fxCheckWeakSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* value = fxCheckWeakSetValue(the);
	txInteger index;
	txSlot* slot;
	index = fxSetEntry(the, table, values, value);
	if (fxAdjustEntries(the, values, index + 1)) {
		slot = values->value.array.address + index;
		slot->kind = value->kind;
		slot->value = value->value;
	}
	*mxResult = *mxThis;
}

void fx_WeakSet_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckWeakSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* value = fxCheckWeakSetValue(the);
	txInteger index;
	index = fxGetEntry(the, table, values, value);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (index >= 0) ? 1 : 0;
}

void fx_WeakSet_prototype_delete(txMachine* the)
{
	txSlot* instance = fxCheckWeakSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* values = table->next;
	txSlot* value = fxCheckWeakSetValue(the);
	txInteger index;
	txSlot* slot;
	index = fxDeleteEntry(the, table, values, value);
	if (index >= 0) {
		slot = values->value.array.address + index;
		slot->flag = XS_DONT_ENUM_FLAG;
		slot->kind = XS_UNDEFINED_KIND;
		mxResult->value.boolean = 1;
	}
	else
		mxResult->value.boolean = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

txBoolean fxAdjustEntries(txMachine* the, txSlot* array, txInteger length) 
{
	txInteger offset = array->value.array.length;
	if (offset < length) {
		txSlot* address = array->value.array.address;
		if (address)
			address = (txSlot*)fxRenewChunk(the, address, length * sizeof(txSlot));
		if (!address) {
			address = (txSlot*)fxNewChunk(the, length * sizeof(txSlot));
			if (offset)
				c_memcpy(address, array->value.array.address, offset * sizeof(txSlot));
		}
		c_memset(address + offset, 0, (length - offset) * sizeof(txSlot));
		array->value.array.length = length;
		array->value.array.address = address;
		return 1;
	}
	return 0;
}

txInteger fxCountEntries(txMachine* the, txSlot* array) 
{
	txSlot* slot = array->value.array.address;
	txInteger index = array->value.array.length;
	txInteger count = 0;
	while (index) {
		if (!(slot->flag & XS_DONT_ENUM_FLAG))
			count++;
		slot++;
		index--;
	}
	return count;
}

txInteger fxDeleteEntry(txMachine* the, txSlot* table, txSlot* array, txSlot* slot) 
{
	txU4 sum = fxSumEntry(the, slot);
	txU4 modulo = sum % table->value.table.length;
	txSlot** address = &(table->value.table.address[modulo]);
	txSlot* entry;
	while ((entry = *address)) {
		if (entry->value.entry.sum == sum) {
			txInteger index = entry->value.entry.index;
			if (fxIsSameSlot(the, array->value.array.address + index, slot)) {
				*address = entry->next;
				entry->next = C_NULL;
				return index;
			}
		}
		address = &entry->next;
	}
	return -1;
}

txInteger fxGetEntry(txMachine* the, txSlot* table, txSlot* array, txSlot* slot) 
{
	txU4 sum = fxSumEntry(the, slot);
	txU4 modulo = sum % table->value.table.length;
	txSlot* entry = table->value.table.address[modulo];
	while (entry) {
		if (entry->value.entry.sum == sum) {
			txInteger index = entry->value.entry.index;
			if (fxIsSameSlot(the, array->value.array.address + index, slot))
				return index;
		}
		entry = entry->next;
	}
	return -1;
}

txInteger fxSetEntry(txMachine* the, txSlot* table, txSlot* array, txSlot* slot) 
{
	txU4 sum = fxSumEntry(the, slot);
	txU4 modulo = sum % table->value.table.length;
	txSlot** address = &(table->value.table.address[modulo]);
	txSlot* entry;
	while ((entry = *address)) {
		if (entry->value.entry.sum == sum) {
			txInteger index = entry->value.entry.index;
			if (fxIsSameSlot(the, array->value.array.address + index, slot))
				return index;
		}
		address = &entry->next;
	}
	entry = fxNewSlot(the);
	entry->kind = XS_ENTRY_KIND;
	entry->value.entry.index = array->value.array.length;
	entry->value.entry.sum = sum;
	address = &(table->value.table.address[modulo]);
	while ((slot = *address))
		address = &slot->next;
	*address = entry;
	//fprintf(stderr, "sum %ld modulo %ld\n", sum, modulo);
	return entry->value.entry.index;
}

txU4 fxSumEntry(txMachine* the, txSlot* slot) 
{
	txU1 kind, *address;
	txU4 sum, size;
	
	kind = slot->kind;
	sum = 0;
	if ((XS_STRING_KIND == kind) || (XS_STRING_X_KIND == kind)) {
		address = (txU1*)slot->value.string;
		while ((kind = *address++))
			sum = (sum << 1) + kind;
		sum = (sum << 1) + XS_STRING_KIND;
	}
	else {
		if (XS_BOOLEAN_KIND == kind) {
			address = (txU1*)&slot->value.boolean;
			size = sizeof(txBoolean);
		}
		else if (XS_INTEGER_KIND == kind) {
			fxToNumber(the, slot);
			kind = slot->kind;
			address = (txU1*)&slot->value.number;
			size = sizeof(txNumber);
		}
		else if (XS_NUMBER_KIND == kind) {
			if (slot->value.number == 0)
				slot->value.number = 0;
			address = (txU1*)&slot->value.number;
			size = sizeof(txNumber);
		}
		else if (XS_SYMBOL_KIND == kind) {
			address = (txU1*)&slot->value.ID;
			size = sizeof(txID);
		}
		else if (XS_REFERENCE_KIND == kind) {
			address = (txU1*)&slot->value.reference;
			size = sizeof(txSlot*);
		}
		else {
			address = C_NULL;
			size = 0;
		}
		while (size) {
			sum = (sum << 1) + *address++;
			size--;
		}
		sum = (sum << 1) + kind;
	}
	sum &= 0x7FFFFFFF;
	return sum;
}










