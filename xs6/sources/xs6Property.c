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

static void fxEnumProperty(txMachine* the, txSlot* context, txID id, txIndex index, txSlot* property);

txSlot* fxLastProperty(txMachine* the, txSlot* slot)
{
	txSlot* property;
	while ((property = slot->next))
		slot = property;
	return slot;
}

txSlot* fxNextHostAccessorProperty(txMachine* the, txSlot* property, txCallback get, txCallback set, txID id, txFlag flag)
{
	txSlot *getter = NULL, *setter = NULL, *home = the->stack, *slot;
	if (get == set) {
		getter = setter = fxNewHostFunction(the, get, 0, XS_NO_ID);
		slot = mxFunctionInstanceHome(getter);
		slot->value.home.object = home->value.reference;
	}
	else {
		if (get) {
			getter = fxNewHostFunction(the, get, 0, XS_NO_ID);
			slot = mxFunctionInstanceHome(getter);
			slot->value.home.object = home->value.reference;
			fxRenameFunction(the, getter, id, XS_NO_ID, "get ");
		}
		if (set) {
			setter = fxNewHostFunction(the, set, 1, XS_NO_ID);
			slot = mxFunctionInstanceHome(setter);
			slot->value.home.object = home->value.reference;
			fxRenameFunction(the, setter, id, XS_NO_ID, "set ");
		}
	}
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_ACCESSOR_KIND;
	property->value.accessor.getter = getter;
	property->value.accessor.setter = setter;
	if (get == set) {
		the->stack++;
	}
	else {
		if (set)
			the->stack++;
		if (get)
			the->stack++;
	}
	return property;
}

txSlot* fxNextHostFunctionProperty(txMachine* the, txSlot* property, txCallback call, txInteger length, txID id, txFlag flag)
{
	txSlot *function, *home = the->stack, *slot;
	function = fxNewHostFunction(the, call, length, id);
	slot = mxFunctionInstanceHome(function);
	slot->value.home.object = home->value.reference;
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	the->stack++;
	return property;
}

txSlot* fxNextUndefinedProperty(txMachine* the, txSlot* property, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_UNDEFINED_KIND;
	return property;
}

txSlot* fxNextNullProperty(txMachine* the, txSlot* property, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_NULL_KIND;
	return property;
}

txSlot* fxNextBooleanProperty(txMachine* the, txSlot* property, txBoolean boolean, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_BOOLEAN_KIND;
	property->value.boolean = boolean;
	return property;
}

txSlot* fxNextIntegerProperty(txMachine* the, txSlot* property, txInteger integer, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_INTEGER_KIND;
	property->value.integer = integer;
	return property;
}

txSlot* fxNextNumberProperty(txMachine* the, txSlot* property, txNumber number, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_NUMBER_KIND;
	property->value.number = number;
	return property;
}

txSlot* fxNextSlotProperty(txMachine* the, txSlot* property, txSlot* slot, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = slot->kind;
	property->value = slot->value;
	return property;
}

txSlot* fxNextStringProperty(txMachine* the, txSlot* property, txString string, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	fxCopyStringC(the, property, string);
	return property;
}

txSlot* fxNextStringXProperty(txMachine* the, txSlot* property, txString string, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_STRING_X_KIND;
	property->value.string = string;
	return property;
}


txSlot* fxNextSymbolProperty(txMachine* the, txSlot* property, txID symbol, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_SYMBOL_KIND;
	property->value.symbol = symbol;
	return property;
}

txSlot* fxNextTypeDispatchProperty(txMachine* the, txSlot* property, txTypeDispatch* dispatch, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_TYPED_ARRAY_KIND;
	property->value.typedArray = dispatch;
	return property;
}


txBoolean fxDefineProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask) 
{
	txSlot* internal;
	txBoolean result;
	mxCheck(the, instance->kind == XS_INSTANCE_KIND);
	if ((internal = instance->next) && (internal->kind == XS_PROXY_KIND)) {
		fxBeginHost(the);
		result = fxDefineProxyProperty(the, instance, id, index, slot, mask);
		fxEndHost(the);
	}
	else
		result = fxDefineObjectProperty(the, instance, id, index, slot, mask);
	return result;
}


txBoolean fxDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index) 
{
	txBoolean result;
	mxCheck(the, instance->kind == XS_INSTANCE_KIND);
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND)) {
		fxBeginHost(the);
		result = fxDeleteProxyProperty(the, instance, id, index);
		fxEndHost(the);
	}
	else
		result = fxDeleteObjectProperty(the, instance, id, index);
	return result;
}

void fxEnumProperties(txMachine* the, txSlot* instance, txFlag flag)
{
	txSlot* array;
	txSlot context;
	mxPush(mxArrayPrototype);
	array = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	context.value.array.length = 0;
	context.value.array.address = fxLastProperty(the, array);
	fxEachInstanceProperty(the, instance, flag, fxEnumProperty, &context, instance);
	array->next->value.array.length = context.value.array.length;
	fxCacheArray(the, array);
}

void fxEnumProperty(txMachine* the, txSlot* context, txID id, txIndex index, txSlot* property)
{
	txSlot* item = context->value.array.address->next = fxNewSlot(the);
	fxKeyAt(the, id, index, item);
	context->value.array.address = item;
	context->value.array.length++;
}

txSlot* fxGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
    txSlot* target = instance;
    txSlot* result;
	mxCheck(the, instance->kind == XS_INSTANCE_KIND);
again:
	if (id) {
		if (instance->ID >= 0) {
			txSlot* alias = the->aliasArray[instance->ID];
			if (alias) {
				result = alias->next;
				while (result) {
					if (result->ID == id)
						return result;
					result = result->next;
				}
			}
		}
		result = instance->next;
		if (result && (result->flag & XS_INTERNAL_FLAG)) {
			if (result->kind == XS_GLOBAL_KIND) {
				result = fxGetGlobalProperty(the, instance, id);
				if (result)
					return result;
				instance = instance->value.instance.prototype;
				if (instance)
					goto again;
				return C_NULL;
			}
			if (result->kind == XS_STAR_KIND)
				return fxGetStarProperty(the, instance, id);
			if (result->kind == XS_PROXY_KIND)
				return fxAccessProxyProperty(the, instance, id, index);
			result = instance->next;
			while (result && (result->flag & XS_INTERNAL_FLAG)) {
				result = result->next;
			}
		}
		while (result) {
			if (result->ID == id)
				return result;
			result = result->next;
		}
	}
	else {
		txSlot* internal = instance->next;
		if (internal && (internal->flag & XS_INTERNAL_FLAG)) {
			if (internal->kind == XS_ARRAY_KIND)
				result = fxGetArrayProperty(the, internal, index);
			else if (internal->kind == XS_TYPED_ARRAY_KIND)
				result = fxGetTypedArrayProperty(the, instance, index);
			else if (internal->kind == XS_ARGUMENTS_STRICT_KIND)
				result = fxGetArrayProperty(the, internal, index);
			else if (internal->kind == XS_ARGUMENTS_SLOPPY_KIND)
				result = fxGetArgumentsSloppyProperty(the, instance, index);
			else if (internal->kind == XS_STRING_KIND)
				result = fxGetStringProperty(the, instance, index);
			else if (internal->kind == XS_PROXY_KIND)
				result = fxAccessProxyProperty(the, instance, id, index);
			else
				result = C_NULL;
			if (result)
				return result;
			internal = internal->next;
			while (internal && (internal->flag & XS_INTERNAL_FLAG)) {
				internal = internal->next;
			}
		}
		if (internal && (internal->ID == XS_NO_ID) && (internal->kind == XS_ARRAY_KIND)) {
			result = fxGetArrayProperty(the, internal, index);
			if (result)
				return result;
		}
	}
	if (flag) {
		instance = instance->value.instance.prototype;
		if (instance)
			goto again;
	}
	if ((target->flag & XS_DONT_PATCH_FLAG) && (target->next)) {
		result = target->next;
		if ((result->kind == XS_HOST_KIND) && (result->flag & XS_HOST_HOOKS_FLAG) && (result->flag & XS_INTERNAL_FLAG))
			return fxGetHostProperty(the, target, id, index);
	}
	return C_NULL;
}

txSlot* fxSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag) 
{
	txSlot* result;
	txSlot** address;
	txSlot* prototype;
	txSlot* property;
	
	mxCheck(the, instance->kind == XS_INSTANCE_KIND);
	if (id) {
		if (instance->ID >= 0) {
			txSlot* alias = the->aliasArray[instance->ID];
			txSlot** aliasAddress = C_NULL;
			if (alias) {
				aliasAddress = &(alias->next);
				while ((result = *aliasAddress)) {
					if (result->ID == id)
						return result;
					aliasAddress = &(result->next);
				}
			}
			if (flag) {
				property = fxGetProperty(the, instance, id, XS_NO_ID, XS_ANY);
				if (property) {
					if (property->kind == XS_ACCESSOR_KIND)
						return property;
					if (property->flag & XS_DONT_SET_FLAG)
						return property;
				}
			}
			if (instance->flag & XS_DONT_PATCH_FLAG)
				return C_NULL;
			if (!alias) {
				alias = fxNewSlot(the);
				alias->kind = XS_INSTANCE_KIND;
				alias->value.instance.garbage = C_NULL;
				alias->value.instance.prototype = C_NULL;
				the->aliasArray[instance->ID] = alias;
				aliasAddress = &(alias->next);
			}
			*aliasAddress = result = fxNewSlot(the);
			result->ID = id;
			/*if (property) {
				result->flag = property->flag;
				result->kind = property->kind;
				result->value = property->value;
			}*/
			return result;
		}
		address = &(instance->next);
		result = instance->next;
		if ((result = *address) && (result->flag & XS_INTERNAL_FLAG)) {
			if ((id == mxID(_length)) && (result->kind == XS_ARRAY_KIND)) {
				if (result->next->flag & XS_DONT_SET_FLAG)
					return result->next;
				return fxSetArrayLength(the, instance->next, fxCheckArrayLength(the, the->stack));
			}
			if (result->kind == XS_GLOBAL_KIND)
				return fxSetGlobalProperty(the, instance, id);
			if (result->kind == XS_STAR_KIND)
				return fxSetStarProperty(the, instance, id);
			if (result->kind == XS_WITH_KIND)
				return fxSetWithProperty(the, instance, id);
			if (result->kind == XS_PROXY_KIND)
				return fxAccessProxyProperty(the, instance, id, index);
			address = &(result->next);
			while ((result = *address) && (result->flag & XS_INTERNAL_FLAG)) {
				address = &(result->next);
			}
		}
		while ((result = *address)) {
			if (result->ID == id)
				return result;
			address = &(result->next);
		}
		if (flag) {
			prototype = instance->value.instance.prototype;
			if (prototype)
				property = fxGetProperty(the, prototype, id, index, XS_ANY);
			else
				property = C_NULL;
			if (property) {
				if (property->kind == XS_ACCESSOR_KIND)
					return property;
				if (property->flag & XS_DONT_SET_FLAG)
					return property;
			}
		}
		if (instance->flag & XS_DONT_PATCH_FLAG)
			return C_NULL;
		*address = result = fxNewSlot(the);
		result->ID = id;
		/*if (property) {
			result->flag = property->flag;
			result->kind = property->kind;
			result->value = property->value;
		}*/
	}
	else {
		address = &(instance->next);
		if ((property = *address) && (property->flag & XS_INTERNAL_FLAG)) {
			if (property->kind == XS_ARRAY_KIND)
				return fxSetArrayProperty(the, instance, property, index);
			if (property->kind == XS_TYPED_ARRAY_KIND)
				return fxSetTypedArrayProperty(the, instance, index);
			if (property->kind == XS_ARGUMENTS_STRICT_KIND)
				return fxSetArrayProperty(the, instance, property, index);
			if (property->kind == XS_ARGUMENTS_SLOPPY_KIND)
				return fxSetArgumentsSloppyProperty(the, instance, index);
			if (property->kind == XS_STRING_KIND) {
				result = fxSetStringProperty(the, instance, index);
				if (result)
					return result;
			}
			if (property->kind == XS_PROXY_KIND)
				return fxAccessProxyProperty(the, instance, id, index);
			address = &(property->next);
			while ((property = *address) && (property->flag & XS_INTERNAL_FLAG)) {
				address = &(property->next);
			}
		}
		if (property && (property->ID == XS_NO_ID) && (property->kind == XS_ARRAY_KIND)) {
			result = fxSetArrayProperty(the, instance, property, index);
		}
		else {
			property = fxNewSlot(the);
			property->next = *address;
			property->kind = XS_ARRAY_KIND;
			property->value.array.address = C_NULL;
			property->value.array.length = 0;
			*address = property;
			result = fxSetArrayProperty(the, instance, property, index);
		}
	}	
	return result;
}

txSlot* fxGetGlobalProperty(txMachine* the, txSlot* instance, txID theID) 
{
	txSlot* globals = instance->next;
	txInteger anID;

	mxCheck(the, theID < 0);
	anID = theID & 0x00007FFF;
	mxCheck(the, anID < globals->value.table.length);
	return globals->value.table.address[anID];
}

txBoolean fxRemoveGlobalProperty(txMachine* the, txSlot* instance, txID theID) 
{
	txSlot* globals = instance->next;
	txInteger anID;
	txSlot* result;
	txSlot** aSlotAddress;
	
	mxCheck(the, theID < 0);
	anID = theID & 0x00007FFF;
	mxCheck(the, anID < globals->value.table.length);
	result = globals->value.table.address[anID];
	if (result) {
		aSlotAddress = &(globals->next);
		while ((result = *aSlotAddress)) {
			if (result->ID == theID) {
				if (result->flag & XS_DONT_DELETE_FLAG)
					return 0;
				globals->value.table.address[anID] = C_NULL;
				*aSlotAddress = result->next;
				result->next = C_NULL;
				return 1;
			}
			aSlotAddress = &(result->next);
		}
	}
	return 1;
}

txSlot* fxSetGlobalProperty(txMachine* the, txSlot* instance, txID theID) 
{
	txSlot* globals = instance->next;
	txID anID;
	txSlot* result;
	txSlot** address;
	
	mxCheck(the, theID < 0);
	anID = theID & 0x00007FFF;
	mxCheck(the, anID < globals->value.table.length);
	result = globals->value.table.address[anID];
	if (instance->flag & XS_DONT_PATCH_FLAG)
		return C_NULL;
	if (!result) {
		address = &(globals->next);
		while ((result = *address) && (result->ID == XS_NO_ID)) {
			address = &(result->next);
		}
		result = fxNewSlot(the);
		result->ID = (txID)theID;
		result->next = *address;
		*address = result;
		globals->value.table.address[anID] = result;
	}
	return result;
}

txSlot* fxGetStarProperty(txMachine* the, txSlot* instance, txID theID)
{
	txSlot* result = instance->next->next;
	while (result) {
		if (result->ID == theID) {
			return result->value.closure;
		}
		result = result->next;
	}
	return C_NULL;
}

txBoolean fxRemoveStarProperty(txMachine* the, txSlot* instance, txID theID) 
{
	return 0;
}

txSlot* fxSetStarProperty(txMachine* the, txSlot* instance, txID theID)
{
	txSlot* result = instance->next->next;
	while (result) {
		if (result->ID == theID) {
			return result->value.closure;
		}
		result = result->next;
	}
	return C_NULL;
}

txSlot* fxSetWithProperty(txMachine* the, txSlot* instance, txID id)
{
	txSlot* result;
	result = instance->next->next;
	while (result) {
		if (result->ID == id)
			return result->value.closure;
		result = result->next;
	}
	return C_NULL;
}

txBoolean fxIsScopableSlot(txMachine* the, txSlot* instance, txID id)
{	
	txBoolean result;
	fxBeginHost(the);
	mxPushReference(instance);
	result = fxHasID(the, id);
	if (result) {
		mxPushReference(instance);
		fxGetID(the, mxID(_Symbol_unscopables));
		if (mxIsReference(the->stack)) {
			fxGetID(the, id);
			result = fxToBoolean(the, the->stack) ? 0 : 1;
		}
		the->stack++;
	}
	fxEndHost(the);
	return result;
}
