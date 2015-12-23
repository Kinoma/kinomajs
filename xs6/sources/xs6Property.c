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

static void fxEnumProperty(txMachine* the, txSlot* context, txInteger id, txSlot* property);

static txSlot* fxGetStarProperty(txMachine* the, txSlot* theInstance, txInteger theID);
static txBoolean fxRemoveStarProperty(txMachine* the, txSlot* theInstance, txInteger theID);
static txSlot* fxSetStarProperty(txMachine* the, txSlot* theInstance, txInteger theID);

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
		slot->kind = home->kind;
		slot->value = home->value;
	}
	else {
		if (get) {
			getter = fxNewHostFunction(the, get, 0, XS_NO_ID);
			slot = mxFunctionInstanceHome(getter);
			slot->kind = home->kind;
			slot->value = home->value;
			slot = fxGetProperty(the, getter, mxID(_name));
			fxRenameFunction(the, slot, id, "", "get ");
		}
		if (set) {
			setter = fxNewHostFunction(the, set, 1, XS_NO_ID);
			slot = mxFunctionInstanceHome(setter);
			slot->kind = home->kind;
			slot->value = home->value;
			slot = fxGetProperty(the, setter, mxID(_name));
			fxRenameFunction(the, slot, id, "", "set ");
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
	slot->kind = home->kind;
	slot->value = home->value;
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

txSlot* fxNextSymbolProperty(txMachine* the, txSlot* property, txID symbol, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_SYMBOL_KIND;
	property->value.ID = symbol;
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

void fxDescribeProperty(txMachine* the, txSlot* property)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	if (property->kind == XS_ACCESSOR_KIND) {
		slot = fxNextUndefinedProperty(the, slot, mxID(_get), XS_NO_FLAG);
		if (property->value.accessor.getter) {
			slot->kind = XS_REFERENCE_KIND;
			slot->value.reference = property->value.accessor.getter;
		}
		slot = fxNextUndefinedProperty(the, slot, mxID(_set), XS_NO_FLAG);
		if (property->value.accessor.setter) {
			slot->kind = XS_REFERENCE_KIND;
			slot->value.reference = property->value.accessor.setter;
		}
	}
	else {
		slot = fxNextSlotProperty(the, slot, property, mxID(_value), XS_NO_FLAG);
		slot = fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_SET_FLAG) ? 0 : 1, mxID(_writable), XS_NO_FLAG);
	}
	slot= fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_ENUM_FLAG) ? 0 : 1, mxID(_enumerable), XS_NO_FLAG);
	slot= fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_DELETE_FLAG) ? 0 : 1, mxID(_configurable), XS_NO_FLAG);
}

void fxEnumProperties(txMachine* the, txSlot* instance, txFlag flag)
{
	txSlot* array;
	txSlot context;
	mxPush(mxArrayPrototype);
	array = fxNewArrayInstance(the);
	*mxResult = *(the->stack++);
	context.value.array.length = 0;
	context.value.array.address = array->next;
	fxEachInstanceProperty(the, instance, flag, fxEnumProperty, &context, instance);
	array->next->value.array.length = context.value.array.length;
	fxCacheArray(the, array);
}

void fxEnumProperty(txMachine* the, txSlot* context, txInteger id, txSlot* property)
{
	txSlot* item = context->value.array.address->next = fxNewSlot(the);
	fxIDToSlot(the, id, item);
	context->value.array.address = item;
	context->value.array.length++;
}

txSlot* fxGetOwnProperty(txMachine* the, txSlot* theInstance, txInteger theID) 
{
	txSlot* result;
	txID id = (txID)theID;
	
	mxCheck(the, theInstance->kind == XS_INSTANCE_KIND);
	if (theInstance->flag & XS_SHARED_FLAG) {
		txSlot* alias = the->aliasArray[theInstance->ID];
		if (alias) {
			result = alias->next;
			while (result) {
				if (result->ID == id)
					return result;
				result = result->next;
			}
		}
	}
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theID < 0) {
			if (theInstance->next->kind == XS_GLOBAL_KIND)
				return fxGetGlobalProperty(the, theInstance, theID);
			if (theInstance->next->kind == XS_STAR_KIND)
				return fxGetStarProperty(the, theInstance, theID);
		}
		else {
			if (theInstance->next->kind == XS_TYPED_ARRAY_KIND)
				return fxGetTypedArrayProperty(the, theInstance, theID);
			if (theInstance->next->kind == XS_STRING_KIND)
				return fxGetStringProperty(the, theInstance, theID);
			if (theInstance->next->kind == XS_PARAMETERS_KIND)
				return fxGetParametersProperty(the, theInstance, theID);
		}
	}
	if (theID >= 0) {
		result = theInstance->next;
		while (result) {
			if (result->ID == XS_NO_ID) {
				if (result->kind == XS_ARRAY_KIND)
					return fxGetArrayProperty(the, result, theID);
			}
			else
				break;
			result = result->next;
		}
	}
	else {
		result = theInstance->next;
		while (result) {
			if (result->ID == theID)
				return result;
			result = result->next;
		}
	}
	if ((theInstance->flag & XS_DONT_PATCH_FLAG) && (theInstance->flag & XS_VALUE_FLAG)) {
		result = theInstance->next;
		if ((result->kind == XS_HOST_KIND) && (result->flag & XS_HOST_HOOKS_FLAG))
			return fxGetHostProperty(the, theInstance, theID);
	}
	return C_NULL;
}

txSlot* fxGetProperty(txMachine* the, txSlot* theInstance, txInteger theID)
{
	txSlot* aSlot;
	txSlot* result;
	txID id = (txID)theID;
	
	mxCheck(the, theInstance->kind == XS_INSTANCE_KIND);
    aSlot = theInstance;
again:
	if (aSlot->flag & XS_SHARED_FLAG) {
		txSlot* alias = the->aliasArray[aSlot->ID];
		if (alias) {
			result = alias->next;
			while (result) {
				if (result->ID == id)
					return result;
				result = result->next;
			}
		}
	}
	if (aSlot->flag & XS_VALUE_FLAG) {
		if (theID < 0) {
			if (aSlot->next->kind == XS_GLOBAL_KIND) {
				result = fxGetGlobalProperty(the, aSlot, theID);
				if (result)
					return result;
				aSlot = aSlot->value.instance.prototype;
				if (aSlot)
					goto again;
                return aSlot;
			}
			if (aSlot->next->kind == XS_STAR_KIND)
				return fxGetStarProperty(the, aSlot, theID);
			if (aSlot->next->kind == XS_PROXY_KIND)
				return fxGetProxyProperty(the, aSlot, theID);
		}
		else {
			if (aSlot->next->kind == XS_TYPED_ARRAY_KIND)
				return fxGetTypedArrayProperty(the, aSlot, theID);
			if (aSlot->next->kind == XS_STRING_KIND)
				return fxGetStringProperty(the, aSlot, theID);
			if (aSlot->next->kind == XS_PARAMETERS_KIND)
				return fxGetParametersProperty(the, aSlot, theID);
			if (aSlot->next->kind == XS_PROXY_KIND)
				return fxGetProxyProperty(the, aSlot, theID);
		}
	}
	if (theID >= 0) {
		result = aSlot->next;
		while (result) {
			if (result->ID == XS_NO_ID) {
				if (result->kind == XS_ARRAY_KIND)
					return fxGetArrayProperty(the, result, theID);
			}
			else
				break;
			result = result->next;
		}
	}
	else {
		result = aSlot->next;
		while (result) {
			if (result->ID == id)
				return result;
			result = result->next;
		}
	}
	aSlot = aSlot->value.instance.prototype;
	if (aSlot)
		goto again;
	if ((theInstance->flag & XS_DONT_PATCH_FLAG) && (theInstance->flag & XS_VALUE_FLAG)) {
		result = theInstance->next;
		if ((result->kind == XS_HOST_KIND) && (result->flag & XS_HOST_HOOKS_FLAG))
			return fxGetHostProperty(the, theInstance, theID);
	}
	return C_NULL;
}

txBoolean fxHasOwnProperty(txMachine* the, txSlot* theInstance, txInteger theID) 
{
    if ((theInstance->flag & XS_VALUE_FLAG) && (theInstance->next->kind == XS_PROXY_KIND)) {
		txBoolean result = fxGetInstanceOwnProperty(the, theInstance, theID) ? 1 : 0;
		mxPop();
		return result;
	}
	return (fxGetOwnProperty(the, theInstance, theID)) ? 1 : 0;
}

txBoolean fxHasProperty(txMachine* the, txSlot* theInstance, txInteger theID) 
{
	if ((theInstance->flag & XS_VALUE_FLAG) && (theInstance->next->kind == XS_PROXY_KIND))
		return fxHasProxyProperty(the, theInstance, theID);
	return (fxGetProperty(the, theInstance, theID)) ? 1 : 0;
}

txBoolean fxRemoveProperty(txMachine* the, txSlot* theInstance, txInteger theID) 
{
	txSlot** aSlotAddress;
	txSlot* result;

	mxCheck(the, theInstance->kind == XS_INSTANCE_KIND);
	if (theInstance->flag & XS_SHARED_FLAG)
		return 0;
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theID < 0) {
			if (theInstance->next->kind == XS_GLOBAL_KIND)
				return fxRemoveGlobalProperty(the, theInstance, theID);
			if (theInstance->next->kind == XS_STAR_KIND)
				return fxRemoveStarProperty(the, theInstance, theID);
			if (theInstance->next->kind == XS_PROXY_KIND)
				return fxRemoveProxyProperty(the, theInstance, theID);
			if (theID == mxID(_length)) {
                if (theInstance->next->kind == XS_ARRAY_KIND)
                    return 0;
				if (theInstance->next->kind == XS_HOST_KIND)
					return 0;
                if (theInstance->next->kind == XS_STRING_KIND)
                    return 0;
			}
			else if (theID == mxID(_prototype)) {
                if (theInstance->next->kind == XS_CALLBACK_KIND)
                    return 0;
                if (theInstance->next->kind == XS_CODE_KIND)
                    return 0;
                if (theInstance->next->kind == XS_CODE_X_KIND)
                    return 0;
			}
		}
		else {
			if (theInstance->next->kind == XS_STRING_KIND)
				return fxRemoveStringProperty(the, theInstance, theID);
			if (theInstance->next->kind == XS_PARAMETERS_KIND)
				return fxRemoveParametersProperty(the, theInstance, theID);
			if (theInstance->next->kind == XS_PROXY_KIND)
				return fxRemoveProxyProperty(the, theInstance, theID);
		}
	}
	if (theID >= 0) {
		result = theInstance->next;
		while (result) {
			if (result->ID == XS_NO_ID) {
				if (result->kind == XS_ARRAY_KIND)
					return fxRemoveArrayProperty(the, result, theID);
			}
			else
				break;
			result = result->next;
		}
		return 1;
	}
	aSlotAddress = &(theInstance->next);
	while ((result = *aSlotAddress)) {
		if (result->ID == theID) {
			if (result->flag & XS_DONT_DELETE_FLAG)
				return 0;
			*aSlotAddress = result->next;
			result->next = C_NULL;
			return 1;
		}
		aSlotAddress = &(result->next);
	}
	return 1;
}

txBoolean fxNewProperty(txMachine* the, txSlot* instance, txInteger ID, txFlag flag, txSlot* slot) 
{
	txSlot** address = &(instance->next);
	txSlot* property;
	
	if (ID >= 0) {
		txSlot* array = C_NULL;
		while ((property = *address)) {
			if (property->ID == XS_NO_ID) {
				if (property->kind == XS_ARRAY_KIND) {
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
		property = fxSetArrayProperty(the, array, ID);
	}
	else {
		while ((property = *address)) {
			if (property->ID == ID)
				break;
			address = &(property->next);
		}
	}
	if (property) {
		if (property->flag & XS_DONT_DELETE_FLAG) {
			if (!(flag  & XS_DONT_DELETE_FLAG))
				return 0;
			if ((property->flag & XS_DONT_ENUM_FLAG) != (flag & XS_DONT_ENUM_FLAG))
				return 0;
			if (flag & XS_ACCESSOR_FLAG) {
				if (property->kind != XS_ACCESSOR_KIND)
					return 0;
				if (flag & XS_GETTER_FLAG) {
					if (property->value.accessor.getter != slot->value.reference)
						return 0;
				}
				if (flag & XS_SETTER_FLAG) {
					if (property->value.accessor.setter != slot->value.reference)
						return 0;
				}
			}
			else {
				if (property->kind == XS_ACCESSOR_KIND)
					return 0;
				if (property->flag & XS_DONT_SET_FLAG) {
					if (!(flag  & XS_DONT_DELETE_FLAG))
						return 0;
					if (!fxIsSameSlot(the, property, slot))
						return 0;
				}
			}
		}
	}
	else {
		if (instance->flag & XS_DONT_PATCH_FLAG)
			return 0;
		*address = property = fxNewSlot(the);
		property->ID = (txID)ID;
	}
	if (flag & XS_ACCESSOR_FLAG) {
		if (property->kind != XS_ACCESSOR_KIND) {
			property->kind = XS_ACCESSOR_KIND;
			property->value.accessor.getter = C_NULL;
			property->value.accessor.setter = C_NULL;
		}
		property->flag = flag & ~XS_ACCESSOR_FLAG;
		slot = slot->value.reference;
		if (flag & XS_GETTER_FLAG) {
			property->value.accessor.getter = slot;
			if (mxIsFunction(slot)) {
				property = fxGetProperty(the, slot, mxID(_name));
				fxRenameFunction(the, property, ID, "", "get ");
			}
		}
		else {
			property->value.accessor.setter = slot;
			if (mxIsFunction(slot)) {
				property = fxGetProperty(the, slot, mxID(_name));
				fxRenameFunction(the, property, ID, "", "set ");
			}
		}
	}
	else {
		property->flag = flag;
		property->kind = slot->kind;
		property->value = slot->value;
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			if (mxIsFunction(slot)) {
				property = fxGetProperty(the, slot, mxID(_name));
				fxRenameFunction(the, property, ID, "", C_NULL);
			}
		}
	}
	return 1;
}

txSlot* fxSetProperty(txMachine* the, txSlot* theInstance, txInteger theID, txFlag* theFlag) 
{
	txSlot* result;
	txSlot** instanceAddress;
	txSlot* prototype;
	txSlot* aProperty;
	
	mxCheck(the, theInstance->kind == XS_INSTANCE_KIND);
	if (theInstance->flag & XS_SHARED_FLAG) {
		txSlot* alias = the->aliasArray[theInstance->ID];
		txSlot** aliasAddress = C_NULL;
		if (alias) {
			aliasAddress = &(alias->next);
			while ((result = *aliasAddress)) {
				if (result->ID == theID)
					return result;
				aliasAddress = &(result->next);
			}
		}
		aProperty = fxGetProperty(the, theInstance, theID);
		if (!theFlag && aProperty) {
			if (aProperty->kind == XS_ACCESSOR_KIND)
				return aProperty;
			if (aProperty->flag & XS_DONT_SET_FLAG)
				return aProperty;
		}
		if (theInstance->flag & XS_DONT_PATCH_FLAG)
			return C_NULL;
		if (!aliasAddress) {
			alias = fxNewSlot(the);
			alias->kind = XS_INSTANCE_KIND;
			alias->value.instance.garbage = C_NULL;
			alias->value.instance.prototype = C_NULL;
			the->aliasArray[theInstance->ID] = alias;
			aliasAddress = &(alias->next);
		}
		*aliasAddress = result = fxNewSlot(the);
		result->ID = (txID)theID;
		if (aProperty) {
			result->flag = aProperty->flag;
			result->kind = aProperty->kind;
			result->value = aProperty->value;
		}
		else {
			if (theFlag)
				result->flag = *theFlag;
		}
		return result;
	}
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theID < 0) {
			if (theInstance->next->kind == XS_GLOBAL_KIND)
				return fxSetGlobalProperty(the, theInstance, theID, theFlag);
			if (theInstance->next->kind == XS_STAR_KIND)
				return fxSetStarProperty(the, theInstance, theID);
			if (theInstance->next->kind == XS_PROXY_KIND)
				return fxSetProxyProperty(the, theInstance, theID);
		}
		else {
			if (theInstance->next->kind == XS_TYPED_ARRAY_KIND)
				return fxSetTypedArrayProperty(the, theInstance, theID);
			if (theInstance->next->kind == XS_STRING_KIND)
				return fxSetStringProperty(the, theInstance, theID);
			if (theInstance->next->kind == XS_PARAMETERS_KIND)
				return fxSetParametersProperty(the, theInstance, theID);
			if (theInstance->next->kind == XS_PROXY_KIND)
				return fxSetProxyProperty(the, theInstance, theID);
		}
	}
	if (theID >= 0) {
		instanceAddress = &(theInstance->next);
		while ((aProperty = *instanceAddress)) {
			if (aProperty->ID == XS_NO_ID) {
				if (aProperty->kind == XS_ARRAY_KIND) {
					if ((theInstance->flag & XS_DONT_PATCH_FLAG) && (theID >= aProperty->value.array.length))
						return C_NULL;
					return fxSetArrayProperty(the, aProperty, theID);
				}
			}
			else
				break;
			instanceAddress = &(aProperty->next);
		}
		if (theInstance->flag & XS_DONT_PATCH_FLAG)
			return C_NULL;
		*instanceAddress = result = fxNewSlot(the);
		result->next = aProperty;
		result->kind = XS_ARRAY_KIND;
		result->value.array.address = C_NULL;
		result->value.array.length = 0;
		return fxSetArrayProperty(the, result, theID);
	}
	
	instanceAddress = &(theInstance->next);
	while ((result = *instanceAddress)) {
		if (result->ID == theID)
			return result;
		instanceAddress = &(result->next);
	}
	prototype = theInstance->value.instance.prototype;
	if (prototype)
		aProperty = fxGetProperty(the, prototype, theID);
	else
		aProperty = C_NULL;
	if (!theFlag && aProperty) {
		if (aProperty->kind == XS_ACCESSOR_KIND)
			return aProperty;
		if (aProperty->flag & XS_DONT_SET_FLAG)
			return aProperty;
	}
	if (theInstance->flag & XS_DONT_PATCH_FLAG)
		return C_NULL;
	*instanceAddress = result = fxNewSlot(the);
	result->ID = (txID)theID;
	if (aProperty) {
		result->flag = aProperty->flag;
		result->kind = aProperty->kind;
		result->value = aProperty->value;
	}
	else {
		if (theFlag)
			result->flag = *theFlag;
	}
	return result;
}

/* [] */
txSlot* fxGetGlobalProperty(txMachine* the, txSlot* theInstance, txInteger theID) 
{
	txSlot* globals = theInstance->next;
	txInteger anID;

	mxCheck(the, theID < 0);
	anID = theID & 0x00007FFF;
	mxCheck(the, anID < globals->value.table.length);
	return globals->value.table.address[anID];
}

txBoolean fxRemoveGlobalProperty(txMachine* the, txSlot* theInstance, txInteger theID) 
{
	txSlot* globals = theInstance->next;
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

txSlot* fxSetGlobalProperty(txMachine* the, txSlot* theInstance, txInteger theID, txFlag* theFlag) 
{
	txSlot* globals = theInstance->next;
	txID anID;
	txSlot* result;
	
	mxCheck(the, theID < 0);
	anID = theID & 0x00007FFF;
	mxCheck(the, anID < globals->value.table.length);
	result = globals->value.table.address[anID];
	if (!result) {
		result = fxNewSlot(the);
		result->ID = (txID)theID;
		result->next = globals->next;
		globals->next = result;
		globals->value.table.address[anID] = result;
		if (theFlag)
			result->flag = *theFlag;
	}
	return result;
}


txSlot* fxGetStarProperty(txMachine* the, txSlot* theInstance, txInteger theID)
{
	txSlot* result = theInstance->next->next;
	while (result) {
		if (result->ID == theID) {
			return result->value.closure;
		}
		result = result->next;
	}
	return C_NULL;
}

txBoolean fxRemoveStarProperty(txMachine* the, txSlot* theInstance, txInteger theID) 
{
	return 0;
}

txSlot* fxSetStarProperty(txMachine* the, txSlot* theInstance, txInteger theID)
{
	txSlot* result = theInstance->next->next;
	while (result) {
		if (result->ID == theID) {
			return result->value.closure;
		}
		result = result->next;
	}
	return C_NULL;
}

