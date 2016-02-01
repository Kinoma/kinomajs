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
static void fx_Set_prototype_keys_next(txMachine* the);
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

static void fxClearEntries(txMachine* the, txSlot* table, txSlot* list, txBoolean paired);
static txInteger fxCountEntries(txMachine* the, txSlot* list, txBoolean paired);
static txBoolean fxDeleteEntry(txMachine* the, txSlot* table, txSlot* list, txSlot* slot, txBoolean paired); 
static txSlot* fxGetEntry(txMachine* the, txSlot* table, txSlot* slot);
static txSlot* fxNewEntryIteratorInstance(txMachine* the, txSlot* iterable);
//static void fxPurgeEntries(txMachine* the, txSlot* list);
static void fxSetEntry(txMachine* the, txSlot* table, txSlot* list, txSlot* slot, txSlot* pair); 
static txU4 fxSumEntry(txMachine* the, txSlot* slot); 
static txBoolean fxTestEntry(txMachine* the, txSlot* a, txSlot* b);

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
		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_Set_prototype_builders[] = {
		{ fx_Set_prototype_add, 1, _add },
		{ fx_Set_prototype_clear, 0, _clear },
		{ fx_Set_prototype_delete, 1, _delete },
		{ fx_Set_prototype_entries, 0, _entries },
		{ fx_Set_prototype_forEach, 1, _forEach },
		{ fx_Set_prototype_has, 1, _has },
		{ fx_Set_prototype_values, 0, _values },
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
	txSlot* property;
	
	/* MAP */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewMapInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, fx_Map_prototype_size, C_NULL, mxID(_size), XS_DONT_ENUM_FLAG);
	for (builder = gx_Map_prototype_builders; builder->callback; builder++) {
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
		if (builder->id == _entries)
			property = slot;
	}
	slot = fxNextSlotProperty(the, slot, property, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Map", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxMapPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_Map, 0, mxID(_Map), XS_DONT_ENUM_FLAG));
	slot = fxNextHostAccessorProperty(the, slot, fx_species_get, C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Map_prototype_entries_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Map Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxMapEntriesIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Map_prototype_keys_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Map Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxMapKeysIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Map_prototype_values_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Map Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxMapValuesIteratorPrototype);
	
	/* SET */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewSetInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, fx_Set_prototype_size, C_NULL, mxID(_size), XS_DONT_ENUM_FLAG);
	for (builder = gx_Set_prototype_builders; builder->callback; builder++) {
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
		if (builder->id == _values)
			property = slot;
	}
	slot = fxNextSlotProperty(the, slot, property, mxID(_keys), XS_DONT_ENUM_FLAG);
	slot = fxNextSlotProperty(the, slot, property, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Set", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxSetPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_Set, 0, mxID(_Set), XS_DONT_ENUM_FLAG));
	slot = fxNextHostAccessorProperty(the, slot, fx_species_get, C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Set_prototype_entries_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Set Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxSetEntriesIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Set_prototype_keys_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Set Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxSetKeysIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Set_prototype_keys_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Set Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxSetValuesIteratorPrototype);

	/* WEAK MAP */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewWeakMapInstance(the));
	for (builder = gx_WeakMap_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "WeakMap", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxWeakMapPrototype = *the->stack;
	fxNewHostConstructorGlobal(the, fx_WeakMap, 0, mxID(_WeakMap), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	/* WEAK SET */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewWeakSetInstance(the));
	for (builder = gx_WeakSet_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "WeakSet", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxWeakSetPrototype = *the->stack;
	fxNewHostConstructorGlobal(the, fx_WeakSet, 0, mxID(_WeakSet), XS_DONT_ENUM_FLAG);
	the->stack++;
}

txSlot* fxCheckMapInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_MAP_KIND) && (instance != mxMapPrototype.value.reference))
			return instance;
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
	txSlot* table;
	txSlot* list;
	txSlot** address;
	map = fxNewSlot(the);
	map->kind = XS_INSTANCE_KIND;
	map->value.instance.garbage = C_NULL;
	map->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = map;
	table = map->next = fxNewSlot(the);
	list = table->next = fxNewSlot(the);
	address = (txSlot**)fxNewChunk(the, 127 * sizeof(txSlot*));
	c_memset(address, 0, 127 * sizeof(txSlot*));
	/* TABLE */
	table->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	table->kind = XS_MAP_KIND;
	table->value.table.address = address;
	table->value.table.length = 127;
	/* LIST */
	list->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	list->kind = XS_LIST_KIND;
	list->value.list.first = C_NULL;
	list->value.list.last = C_NULL;
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
	txSlot* list = table->next;
	fxClearEntries(the, table, list, 1);
}

void fx_Map_prototype_delete(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	txSlot* key = fxCheckMapKey(the);
	mxResult->value.boolean = fxDeleteEntry(the, table, list, key, 1);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Map_prototype_entries(txMachine* the)
{
	fxCheckMapInstance(the, mxThis);
	mxPush(mxMapEntriesIteratorPrototype);
	fxNewEntryIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Map_prototype_entries_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* key = index->value.closure;
	txSlot* value;
	while (key && (key->flag & XS_DONT_ENUM_FLAG)) {
		key = key->next->next;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (key) {
		value = key->next;
		mxPushSlot(key);
		mxPushSlot(value);
		fxConstructArrayEntry(the, result);
		index->value.closure = value->next;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
		index->value.closure = C_NULL;
	}
}

void fx_Map_prototype_forEach(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* key = list->value.list.first;
	while (key) {
		txSlot* value = key->next;
		if (!(key->flag & XS_DONT_ENUM_FLAG)) {
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
		key = value->next;
	}
}

void fx_Map_prototype_get(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* key = fxCheckMapKey(the);
	txSlot* result = fxGetEntry(the, table, key);
	if (result) {
		txSlot* value = result->next;
		mxResult->kind = value->kind;
		mxResult->value = value->value;
	}
}

void fx_Map_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* key = fxCheckMapKey(the);
	txSlot* result = fxGetEntry(the, table, key);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (result) ? 1 : 0;
}

void fx_Map_prototype_keys(txMachine* the)
{
	fxCheckMapInstance(the, mxThis);
	mxPush(mxMapKeysIteratorPrototype);
	fxNewEntryIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Map_prototype_keys_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* key = index->value.closure;
	txSlot* value;
	while (key && (key->flag & XS_DONT_ENUM_FLAG)) {
		key = key->next->next;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (key) {
		value = key->next;
		result->kind = key->kind;
		result->value = key->value;
		index->value.closure = value->next;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
		index->value.closure = C_NULL;
	}
}

void fx_Map_prototype_set(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	txSlot* key = fxCheckMapKey(the);
	fxSetEntry(the, table, list, key, (mxArgc > 1) ? mxArgv(1) : &mxUndefined);
	*mxResult = *mxThis;
}

void fx_Map_prototype_size(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = fxCountEntries(the, list, 1);
}

void fx_Map_prototype_values(txMachine* the)
{
	fxCheckMapInstance(the, mxThis);
	mxPush(mxMapValuesIteratorPrototype);
	fxNewEntryIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Map_prototype_values_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* key = index->value.closure;
	txSlot* value;
	while (key && (key->flag & XS_DONT_ENUM_FLAG)) {
		key = key->next->next;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (key) {
		value = key->next;
		result->kind = value->kind;
		result->value = value->value;
		index->value.closure = value->next;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
		index->value.closure = C_NULL;
	}
}

txSlot* fxCheckSetInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_SET_KIND) && (instance != mxSetPrototype.value.reference))
			return instance;
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
	txSlot* table;
	txSlot* list;
	txSlot** address;
	set = fxNewSlot(the);
	set->kind = XS_INSTANCE_KIND;
	set->value.instance.garbage = C_NULL;
	set->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = set;
	table = set->next = fxNewSlot(the);
	list = table->next = fxNewSlot(the);
	address = (txSlot**)fxNewChunk(the, 127 * sizeof(txSlot*));
	c_memset(address, 0, 127 * sizeof(txSlot*));
	/* TABLE */
	table->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	table->kind = XS_SET_KIND;
	table->value.table.address = address;
	table->value.table.length = 127;
	/* LIST */
	list->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	list->kind = XS_LIST_KIND;
	list->value.list.first = C_NULL;
	list->value.list.last = C_NULL;
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
	txSlot* list = table->next;
	txSlot* value = fxCheckSetValue(the);
	fxSetEntry(the, table, list, value, C_NULL);
	*mxResult = *mxThis;
}

void fx_Set_prototype_clear(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	fxClearEntries(the, table, list, 0);
}

void fx_Set_prototype_delete(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	txSlot* value = fxCheckSetValue(the);
	mxResult->value.boolean = fxDeleteEntry(the, table, list, value, 0);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Set_prototype_entries(txMachine* the)
{
	fxCheckSetInstance(the, mxThis);
	mxPush(mxSetEntriesIteratorPrototype);
	fxNewEntryIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Set_prototype_entries_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = index->value.closure;
	while (value && (value->flag & XS_DONT_ENUM_FLAG))
		value = value->next;
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (value) {
		mxPushSlot(value);
		mxPushSlot(value);
		fxConstructArrayEntry(the, result);
		index->value.closure = value->next;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
		index->value.closure = C_NULL;
	}
}

void fx_Set_prototype_forEach(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* value = list->value.list.first;
	while (value) {
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
		value = value->next;
	}
}

void fx_Set_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* value = fxCheckSetValue(the);
	txSlot* result = fxGetEntry(the, table, value);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (result) ? 1 : 0;
}

void fx_Set_prototype_keys(txMachine* the)
{
	fxCheckSetInstance(the, mxThis);
	mxPush(mxSetKeysIteratorPrototype);
	fxNewEntryIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_Set_prototype_keys_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = index->value.closure;
	while (value && (value->flag & XS_DONT_ENUM_FLAG))
		value = value->next;
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (value) {
		result->kind = value->kind;
		result->value = value->value;
		index->value.closure = value->next;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
		index->value.closure = C_NULL;
	}
}

void fx_Set_prototype_size(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = fxCountEntries(the, list, 0);
}

void fx_Set_prototype_values(txMachine* the)
{
	fxCheckSetInstance(the, mxThis);
	mxPush(mxSetValuesIteratorPrototype);
	fxNewEntryIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

txSlot* fxCheckWeakMapInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_WEAK_MAP_KIND) && (instance != mxWeakMapPrototype.value.reference))
			return instance;
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
	txSlot** address;
	map = fxNewSlot(the);
	map->kind = XS_INSTANCE_KIND;
	map->value.instance.garbage = C_NULL;
	map->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = map;
	table = map->next = fxNewSlot(the);
	address = (txSlot**)fxNewChunk(the, 128 * sizeof(txSlot*)); // one more slot for the collector weak table list
	c_memset(address, 0, 128 * sizeof(txSlot*));
	/* TABLE */
	table->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	table->kind = XS_WEAK_MAP_KIND;
	table->value.table.address = address;
	table->value.table.length = 127;
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
	txSlot* key = fxCheckWeakMapKey(the);
	mxResult->value.boolean = fxDeleteEntry(the, table, C_NULL, key, 1);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_WeakMap_prototype_get(txMachine* the)
{
	txSlot* instance = fxCheckWeakMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* key = fxCheckWeakMapKey(the);
	txSlot* result = fxGetEntry(the, table, key);
	if (result) {
		txSlot* value = result->next;
		mxResult->kind = value->kind;
		mxResult->value = value->value;
	}
}

void fx_WeakMap_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckWeakMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* key = fxCheckWeakMapKey(the);
	txSlot* result = fxGetEntry(the, table, key);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (result) ? 1 : 0;
}

void fx_WeakMap_prototype_set(txMachine* the)
{
	txSlot* instance = fxCheckWeakMapInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* key = fxCheckWeakMapKey(the);
	fxSetEntry(the, table, C_NULL, key, (mxArgc > 1) ? mxArgv(1) : &mxUndefined);
	*mxResult = *mxThis;
}

txSlot* fxCheckWeakSetInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_WEAK_SET_KIND) && (instance != mxWeakSetPrototype.value.reference))
			return instance;
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
	txSlot** address;
	set = fxNewSlot(the);
	set->kind = XS_INSTANCE_KIND;
	set->value.instance.garbage = C_NULL;
	set->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = set;
	table = set->next = fxNewSlot(the);
	address = (txSlot**)fxNewChunk(the, 128 * sizeof(txSlot*)); // one more slot for the collector weak table list
	c_memset(address, 0, 128 * sizeof(txSlot*));
	/* TABLE */
	table->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	table->kind = XS_WEAK_SET_KIND;
	table->value.table.address = address;
	table->value.table.length = 127;
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
	txSlot* value = fxCheckWeakSetValue(the);
	fxSetEntry(the, table, C_NULL, value, C_NULL);
	*mxResult = *mxThis;
}

void fx_WeakSet_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckWeakSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* value = fxCheckWeakSetValue(the);
	txSlot* result = fxGetEntry(the, table, value);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (result) ? 1 : 0;
}

void fx_WeakSet_prototype_delete(txMachine* the)
{
	txSlot* instance = fxCheckWeakSetInstance(the, mxThis);
	txSlot* table = instance->next;
	txSlot* value = fxCheckWeakSetValue(the);
	mxResult->value.boolean = fxDeleteEntry(the, table, C_NULL, value, 0);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fxClearEntries(txMachine* the, txSlot* table, txSlot* list, txBoolean paired)
{
	txSlot* slot = list->value.list.first;
	while (slot) {
		slot->flag = XS_DONT_ENUM_FLAG;
		slot->kind = XS_UNDEFINED_KIND;
		slot = slot->next;
	}
	c_memset(table->value.table.address, 0, table->value.table.length * sizeof(txSlot*));
}

txInteger fxCountEntries(txMachine* the, txSlot* list, txBoolean paired) 
{
	txInteger count = 0;
	txSlot* slot = list->value.list.first;
	while (slot) {
		if (!(slot->flag & XS_DONT_ENUM_FLAG))
			count++;
		slot = slot->next;
	}
	if (paired)
		count >>= 1;
	return count;
}

txBoolean fxDeleteEntry(txMachine* the, txSlot* table, txSlot* list, txSlot* slot, txBoolean paired) 
{
	txU4 sum = fxSumEntry(the, slot);
	txU4 modulo = sum % table->value.table.length;
	txSlot** address = &(table->value.table.address[modulo]);
	txSlot* entry;
	txSlot* result;
	while ((entry = *address)) {
		if (entry->value.entry.sum == sum) {
			result = entry->value.entry.slot;
			if (fxTestEntry(the, result, slot)) {
				if (list) {
					result->flag = XS_DONT_ENUM_FLAG;
					result->kind = XS_UNDEFINED_KIND;
					if (paired) {
						slot = result->next;
						slot->flag = XS_DONT_ENUM_FLAG;
						slot->kind = XS_UNDEFINED_KIND;
					}
				}
				*address = entry->next;
				entry->next = C_NULL;
				return 1;
			}
		}
		address = &entry->next;
	}
	return 0;
}

txSlot* fxGetEntry(txMachine* the, txSlot* table, txSlot* slot) 
{
	txU4 sum = fxSumEntry(the, slot);
	txU4 modulo = sum % table->value.table.length;
	txSlot* entry = table->value.table.address[modulo];
	txSlot* result;
	while (entry) {
		if (entry->value.entry.sum == sum) {
			result = entry->value.entry.slot;
			if (fxTestEntry(the, result, slot))
				return result;
		}
		entry = entry->next;
	}
	return C_NULL;
}

txSlot* fxNewEntryIteratorInstance(txMachine* the, txSlot* iterable) 
{
	txSlot* instance;
	txSlot* result;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	mxPush(mxObjectPrototype);
	result = fxNewObjectInstance(the);
	property = fxNextUndefinedProperty(the, result, mxID(_value), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	property = fxNextBooleanProperty(the, property, 0, mxID(_done), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	property = fxNextSlotProperty(the, instance, the->stack, mxID(_result), XS_GET_ONLY);
	property = fxNextSlotProperty(the, property, iterable, mxID(_iterable), XS_GET_ONLY);
	property = fxNextNullProperty(the, property, mxID(_index), XS_GET_ONLY);
	property->kind = XS_CLOSURE_KIND;
	property->value.closure = iterable->value.reference->next->next->value.list.first;
    the->stack++;
	return instance;
}

#if 0
void fxPurgeEntries(txMachine* the, txSlot* list) 
{
	txSlot* former = C_NULL;
	txSlot** address = &(list->value.list.first);
	txSlot* slot;
	while ((slot = *address)) {
		if (slot->flag & XS_DONT_ENUM_FLAG) {
			*address = slot->next;
		}
		else {
			former = slot;
			address = &slot->next;
		}
	}
	list->value.list.last = former;
}
#endif

void fxSetEntry(txMachine* the, txSlot* table, txSlot* list, txSlot* slot, txSlot* pair) 
{
	txU4 sum = fxSumEntry(the, slot);
	txU4 modulo = sum % table->value.table.length;
	txSlot** address = &(table->value.table.address[modulo]);
	txSlot* entry;
	txSlot* result;
	while ((entry = *address)) {
		if (entry->value.entry.sum == sum) {
			result = entry->value.entry.slot;
			if (fxTestEntry(the, result, slot)) {
				if (pair) {
					slot = result->next;
					slot->kind = pair->kind;
					slot->value = pair->value;
				}
				return;
			}
		}
		address = &entry->next;
	}
	result = fxNewSlot(the);
	result->kind = slot->kind;
	result->value = slot->value;
	mxPushClosure(result);
	if (pair) {
		result->next = slot = fxNewSlot(the);
		slot->kind = pair->kind;
		slot->value = pair->value;
		mxPushClosure(slot);
	}
	*address = entry = fxNewSlot(the);
	entry->kind = XS_ENTRY_KIND;
	entry->value.entry.slot = result;
	entry->value.entry.sum = sum;
	if (list) {
		if (list->value.list.last)
			list->value.list.last->next = result;
		else
			list->value.list.first = result;
		if (pair)
			list->value.list.last = slot;
		else
			list->value.list.last = result;
	}
	if (pair)
		mxPop();
	mxPop();
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
			address = (txU1*)&slot->value.symbol;
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

txBoolean fxTestEntry(txMachine* the, txSlot* a, txSlot* b)
{	
	txBoolean result = 0;
	if (a->kind == b->kind) {
		if ((XS_UNDEFINED_KIND == a->kind) || (XS_NULL_KIND == a->kind))
			result = 1;
		else if (XS_BOOLEAN_KIND == a->kind)
			result = a->value.boolean == b->value.boolean;
		else if (XS_INTEGER_KIND == a->kind)
			result = a->value.integer == b->value.integer;
        else if (XS_NUMBER_KIND == a->kind)
			result = ((c_isnan(a->value.number) && c_isnan(b->value.number)) || (a->value.number == b->value.number));
		else if ((XS_STRING_KIND == a->kind) || (XS_STRING_X_KIND == a->kind))
			result = c_strcmp(a->value.string, b->value.string) == 0;
		else if (XS_SYMBOL_KIND == a->kind)
			result = a->value.symbol == b->value.symbol;
		else if (XS_REFERENCE_KIND == a->kind)
			result = a->value.reference == b->value.reference;
	}
	else if ((XS_INTEGER_KIND == a->kind) && (XS_NUMBER_KIND == b->kind))
		result = (!c_isnan(b->value.number)) && ((txNumber)(a->value.integer) == b->value.number);
	else if ((XS_NUMBER_KIND == a->kind) && (XS_INTEGER_KIND == b->kind))
		result = (!c_isnan(a->value.number)) && (a->value.number == (txNumber)(b->value.integer));
	else if ((XS_STRING_KIND == a->kind) && (XS_STRING_X_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	else if ((XS_STRING_X_KIND == a->kind) && (XS_STRING_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	return result;
}










