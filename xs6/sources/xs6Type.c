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

static void fxCallObject(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments);
static void fxConstructObject(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target);
static void fxEachObjectProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txSlot* instance);
static void fxEnumerateObject(txMachine* the, txSlot* instance);
static void fxEnumerateProperty(txMachine* the, txSlot* theContext, txID id, txIndex index, txSlot* theProperty);
static txBoolean fxGetObjectOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot);
static txBoolean fxGetObjectProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* value);
static txBoolean fxGetObjectPrototype(txMachine* the, txSlot* instance, txSlot* result);
static txBoolean fxHasObjectProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txBoolean fxIsObjectExtensible(txMachine* the, txSlot* instance);
static txBoolean fxPreventObjectExtensions(txMachine* the, txSlot* instance);
static txBoolean fxSetObjectProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver);
static txBoolean fxSetObjectPrototype(txMachine* the, txSlot* instance, txSlot* prototype);
static void fxStepObjectProperty(txMachine* the, txSlot* instance, txFlag flag, txStep step, txSlot* context, txID id, txIndex index, txSlot* property);

txSlot* fxGetInstance(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_REFERENCE_KIND)
		return theSlot->value.reference;
	return C_NULL;
}

txSlot* fxGetParent(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_INSTANCE_KIND)
		return theSlot->value.instance.prototype;
	return C_NULL;
}

txSlot* fxNewInstance(txMachine* the)
{
	txSlot* instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = C_NULL;
	mxPushReference(instance);
	return instance;
}

txSlot* fxToInstance(txMachine* the, txSlot* theSlot)
{
	txSlot* anInstance = C_NULL;
	
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
		mxTypeError("cannot coerce undefined to object");
		break;
	case XS_NULL_KIND:
		mxTypeError("cannot coerce null to object");
		break;
	case XS_BOOLEAN_KIND:
		mxPush(mxBooleanPrototype);
		anInstance = fxNewBooleanInstance(the);
		anInstance->next->value.boolean = theSlot->value.boolean;
		if (the->frame->flag & XS_STRICT_FLAG)
			anInstance->flag |= XS_DONT_PATCH_FLAG;
		mxPullSlot(theSlot);
		break;
	case XS_INTEGER_KIND:
		mxPush(mxNumberPrototype);
		anInstance = fxNewNumberInstance(the);
		anInstance->next->value.number = theSlot->value.integer;
		if (the->frame->flag & XS_STRICT_FLAG)
			anInstance->flag |= XS_DONT_PATCH_FLAG;
		mxPullSlot(theSlot);
		break;
	case XS_NUMBER_KIND:
		mxPush(mxNumberPrototype);
		anInstance = fxNewNumberInstance(the);
		anInstance->next->value.number = theSlot->value.number;
		if (the->frame->flag & XS_STRICT_FLAG)
			anInstance->flag |= XS_DONT_PATCH_FLAG;
		mxPullSlot(theSlot);
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		mxPush(mxStringPrototype);
		anInstance = fxNewStringInstance(the);
		anInstance->next->kind = theSlot->kind;
		anInstance->next->value.string = theSlot->value.string;
		anInstance->next->next->next->value.integer = fxUnicodeLength(theSlot->value.string);
		if (the->frame->flag & XS_STRICT_FLAG)
			anInstance->flag |= XS_DONT_PATCH_FLAG;
		mxPullSlot(theSlot);
		break;
	case XS_SYMBOL_KIND:
		mxPush(mxSymbolPrototype);
		anInstance = fxNewSymbolInstance(the);
		anInstance->next->value.symbol = theSlot->value.symbol;
		if (the->frame->flag & XS_STRICT_FLAG)
			anInstance->flag |= XS_DONT_PATCH_FLAG;
		mxPullSlot(theSlot);
		break;
	case XS_REFERENCE_KIND:
		anInstance = theSlot->value.reference;
		break;
	default:
		mxTypeError("cannot coerce to instance");
		break;
	}
	return anInstance;
}

void fxToPrimitive(txMachine* the, txSlot* theSlot, txInteger theHint)
{
	if (theSlot->kind == XS_REFERENCE_KIND) {
		fxBeginHost(the);
		if (theHint == XS_NO_HINT)
			mxPushString(mxDefaultString.value.string);
		else if (theHint == XS_NUMBER_HINT)
			mxPushString(mxNumberString.value.string);
		else
			mxPushString(mxStringString.value.string);
		mxPushInteger(1);
		mxPushSlot(theSlot);
		fxCallID(the, mxID(_Symbol_toPrimitive));
		theSlot->kind = the->stack->kind;
		theSlot->value = the->stack->value;
		if (theSlot->kind == XS_REFERENCE_KIND) {
			mxTypeError("cannot coerce to primitive");
		}
		the->stack++;
		fxEndHost(the);
	}
}


void fxCallInstance(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		fxCallProxy(the, instance, _this, arguments);
	else
		fxCallObject(the, instance, _this, arguments);
}

void fxConstructInstance(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		fxConstructProxy(the, instance, arguments, target);
	else
		fxConstructObject(the, instance, arguments, target);
}

void fxCreateInstance(txMachine* the, txSlot* function, txSlot* target)
{
	if ((function->next) && (function->next->kind == XS_PROXY_KIND))
		mxPushUndefined();
	else if (fxIsBaseFunctionInstance(the, function)) {
		txSlot* prototype;
		mxPushUndefined();
		prototype = the->stack;
	again:
		fxBeginHost(the);
		mxPushSlot(target);
		fxGetID(the, mxID(_prototype));
		*prototype = *the->stack;
		fxEndHost(the);
		if (the->stack->kind != XS_REFERENCE_KIND) {
			target = fxGetProperty(the, target->value.reference, mxID(_boundFunction), XS_NO_ID, XS_OWN);
			if (target)
				goto again;
			if (function->next->ID == mxID(_Proxy)) {
				fxNewProxyInstance(the);
				return;
			}	
			if (function->next->ID == mxID(_ArrayBuffer)) {
				*the->stack = mxArrayBufferPrototype;
			}	
			else
				mxTypeError("no prototype");
			//*the->stack = mxObjectPrototype;
		}
		fxNewInstanceOf(the);
	}
	else {
		txSlot* slot = fxNewSlot(the);
		slot->kind = XS_UNINITIALIZED_KIND;
		mxPushClosure(slot);
	}
}

txBoolean fxDefineInstanceProperty(txMachine* the, txSlot* instance, txInteger id, txIndex index, txSlot* slot, txFlag mask)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		return fxDefineProxyProperty(the, instance, id, index, slot, mask);
	return fxDefineObjectProperty(the, instance, id, index, slot, mask);
}

txBoolean fxDeleteInstanceProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		return fxDeleteProxyProperty(the, instance, id, index);
	return fxDeleteObjectProperty(the, instance, id, index);
}

void fxEachInstanceProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txSlot* instance)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		fxEachProxyProperty(the, target, flag, step, context, instance);
	else
		fxEachObjectProperty(the, target, flag, step, context, instance);
}

void fxEnumerateInstance(txMachine* the, txSlot* instance)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		fxEnumerateProxy(the, instance);
	else
		fxEnumerateObject(the, instance);
}

txBoolean fxGetInstanceOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		return fxGetProxyOwnProperty(the, instance, id, index, slot);
	return fxGetObjectOwnProperty(the, instance, id, index, slot);
}

txBoolean fxGetInstanceProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* value)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		return fxGetProxyProperty(the, instance, id, index, receiver, value);
	return fxGetObjectProperty(the, instance, id, index, receiver, value);
}

txBoolean fxGetInstancePrototype(txMachine* the, txSlot* instance, txSlot* result)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		return fxGetProxyPrototype(the, instance, result);
	return fxGetObjectPrototype(the, instance, result);
}

txBoolean fxHasInstanceProperty(txMachine* the, txSlot* instance, txID id, txIndex index) 
{
    if ((instance->next) && (instance->next->flag & XS_INTERNAL_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		return fxHasProxyProperty(the, instance, id, index);
	return fxHasObjectProperty(the, instance, id, index);
}

txBoolean fxIsInstanceExtensible(txMachine* the, txSlot* instance)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		return fxIsProxyExtensible(the, instance);
	return fxIsObjectExtensible(the, instance);
}

txBoolean fxPreventInstanceExtensions(txMachine* the, txSlot* instance)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		return fxPreventProxyExtensions(the, instance);
	return fxPreventObjectExtensions(the, instance);
}

txBoolean fxSetInstanceProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		return fxSetProxyProperty(the, instance, id, index, value, receiver);
	return fxSetObjectProperty(the, instance, id, index, value, receiver);
}

txBoolean fxSetInstancePrototype(txMachine* the, txSlot* instance, txSlot* prototype)
{
	if ((instance->next) && (instance->next->kind == XS_PROXY_KIND))
		return fxSetProxyPrototype(the, instance, prototype);
	return fxSetObjectPrototype(the, instance, prototype);
}

void fxStepInstanceProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txID id, txIndex index, txSlot* property)
{
	if ((target->next) && (target->next->kind == XS_PROXY_KIND))
		fxStepProxyProperty(the, target, flag, step, context, id, index, property);
	else
		fxStepObjectProperty(the, target, flag, step, context, id, index, property);
}

void fxCallObject(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments)
{
	txInteger c, i;
	mxPushSlot(arguments);
	fxGetID(the, mxID(_length));
	c = fxToInteger(the, the->stack);
	the->stack++;
	for (i = 0; i < c; i++) {
		mxPushSlot(arguments);
		fxGetID(the, (txID)i);
	}
	/* ARGC */
	mxPushInteger(c);
	/* THIS */
	mxPushSlot(_this);
	/* FUNCTION */
	mxPushReference(instance);
	fxCall(the);
	mxPullSlot(mxResult);
}

void fxConstructObject(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target)
{
	txInteger c, i;
	mxPushSlot(arguments);
	fxGetID(the, mxID(_length));
	c = fxToInteger(the, the->stack);
	the->stack++;
	for (i = 0; i < c; i++) {
		mxPushSlot(arguments);
		fxGetID(the, (txID)i);
	}
	/* ARGC */
	mxPushInteger(c);
	/* THIS */
	fxCreateInstance(the, instance, target);
	/* FUNCTION */
	mxPushReference(instance);
	/* TARGET */
	mxPushSlot(target);
	/* RESULT */
	--(the->stack);
	*(the->stack) = *(the->stack + 3);
	fxRunID(the, C_NULL, XS_NO_ID);
	mxPullSlot(mxResult);
}

txBoolean fxDefineObjectProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask) 
{
	txSlot* property = instance->next;
	txBoolean isMappedArgument = (property) && (property->kind == XS_ARGUMENTS_SLOPPY_KIND) && (!id);
	txBoolean result = 1;
	property = fxGetProperty(the, instance, id, index, XS_OWN);
	if (property) {
		if (property->flag & XS_DONT_DELETE_FLAG) {
			if ((mask & XS_DONT_DELETE_FLAG) && !(slot->flag & XS_DONT_DELETE_FLAG))
				return 0;
			if ((mask & XS_DONT_ENUM_FLAG) && ((property->flag & XS_DONT_ENUM_FLAG) != (slot->flag & XS_DONT_ENUM_FLAG)))
				return 0;
			if (mask & XS_ACCESSOR_FLAG) {
				if (property->kind != XS_ACCESSOR_KIND)
					return 0;
				if (mask & XS_GETTER_FLAG) {
					if (property->value.accessor.getter != slot->value.accessor.getter)
						return 0;
				}
				if (mask & XS_SETTER_FLAG) {
					if (property->value.accessor.setter != slot->value.accessor.setter)
						return 0;
				}
			}
			else if ((mask & XS_DONT_SET_FLAG) || (slot->kind != XS_UNINITIALIZED_KIND)) {
				if (property->kind == XS_ACCESSOR_KIND)
					return 0;
				if (property->flag & XS_DONT_SET_FLAG) {
					if ((mask & XS_DONT_SET_FLAG) && !(slot->flag & XS_DONT_SET_FLAG))
						return 0;
					if ((slot->kind != XS_UNINITIALIZED_KIND) && !fxIsSameValue(the, property, slot))
						return 0;
				}
			}
		}
	}
	else {
		property = fxSetProperty(the, instance, id, index, XS_OWN);
		if (!property)
			return 0;
		property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	}
	if (mask & XS_DONT_DELETE_FLAG) {
		if (slot->flag & XS_DONT_DELETE_FLAG)
			property->flag |= XS_DONT_DELETE_FLAG;
		else
			property->flag &= ~XS_DONT_DELETE_FLAG;
	}
	if (mask & XS_DONT_ENUM_FLAG) {
		if (slot->flag & XS_DONT_ENUM_FLAG)
			property->flag |= XS_DONT_ENUM_FLAG;
		else
			property->flag &= ~XS_DONT_ENUM_FLAG;
	}
	if (mask & XS_ACCESSOR_FLAG) {
		if (isMappedArgument)
			property = fxUnmapArgumentsSloppyProperty(the, instance, index);
		if (property->kind != XS_ACCESSOR_KIND) {
			property->kind = XS_ACCESSOR_KIND;
			property->value.accessor.getter = C_NULL;
			property->value.accessor.setter = C_NULL;
		}
		if (mask & XS_GETTER_FLAG) {
			property->value.accessor.getter = slot->value.accessor.getter;
			if (mxIsFunction(slot->value.accessor.getter)) {
				txSlot* home = mxFunctionInstanceHome(slot->value.accessor.getter);
				home->value.home.object = instance;
				if (id)
					fxRenameFunction(the, slot->value.accessor.getter, id, mxID(_get), "get ");
			}
		}
		if (mask & XS_SETTER_FLAG) {
			property->value.accessor.setter = slot->value.accessor.setter;
			if (mxIsFunction(slot->value.accessor.setter)) {
				txSlot* home = mxFunctionInstanceHome(slot->value.accessor.setter);
				home->value.home.object = instance;
				if (id)
					fxRenameFunction(the, slot->value.accessor.setter, id, mxID(_set), "set ");
			}
		}
	}
	else if ((mask & XS_DONT_SET_FLAG) || (slot->kind != XS_UNINITIALIZED_KIND)) {
		if (slot->kind != XS_UNINITIALIZED_KIND) {
			if ((instance->next) && (instance->next->flag & XS_INTERNAL_FLAG) && (instance->next->kind == XS_ARRAY_KIND) && (id == mxID(_length))) {
				result = fxSetArrayLength(the, instance->next, fxCheckArrayLength(the, slot)) ? 1 : 0;
			}
			else {
				property->kind = slot->kind;
				property->value = slot->value;
				if (slot->kind == XS_REFERENCE_KIND) {
					txSlot* function = slot->value.reference;
					if (mxIsFunction(function)) {
						if (mask & XS_METHOD_FLAG) {
							txSlot* home = mxFunctionInstanceHome(function);
							home->value.home.object = instance;
						}
						if (id)
							fxRenameFunction(the, function, id, mxID(_value), C_NULL);
					}
				}
			}
		}
		if (mask & XS_DONT_SET_FLAG) {
			if (slot->flag & XS_DONT_SET_FLAG) {
				if (isMappedArgument)
					property = fxUnmapArgumentsSloppyProperty(the, instance, index);
				property->flag |= XS_DONT_SET_FLAG;
			}
			else
				property->flag &= ~XS_DONT_SET_FLAG;
		}
	}
	return result;
}

txBoolean fxDeleteObjectProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if (instance->ID >= 0)
		return 0;
	if (id) {
		txSlot** address = &(instance->next);
		txSlot* property = instance->next;
		if ((property = *address) && (property->flag & XS_INTERNAL_FLAG)) {
			if (property->kind == XS_GLOBAL_KIND)
				return fxRemoveGlobalProperty(the, instance, id);
			if (property->kind == XS_STAR_KIND)
				return fxRemoveStarProperty(the, instance, id);
			address = &(property->next);
			while ((property = *address) && (property->flag & XS_INTERNAL_FLAG))
				address = &(property->next);
		}
		while ((property = *address)) {
			if (property->ID == id) {
				if (property->flag & XS_DONT_DELETE_FLAG)
					return 0;
				*address = property->next;
				property->next = C_NULL;
				return 1;
			}
			address = &(property->next);
		}
		return 1;
	}
	else {
		txSlot* property = instance->next;
		if (property && (property->flag & XS_INTERNAL_FLAG)) {
			if (property->kind == XS_ARRAY_KIND)
				return fxDeleteArrayProperty(the, property, index);
			if (property->kind == XS_ARGUMENTS_STRICT_KIND)
				return fxDeleteArrayProperty(the, property, index);
			if (property->kind == XS_ARGUMENTS_SLOPPY_KIND)
				return fxRemoveArgumentsSloppyProperty(the, instance, index);
			if (property->kind == XS_STRING_KIND)
				return fxRemoveStringProperty(the, instance, index);
			property = property->next;
			while (property && (property->flag & XS_INTERNAL_FLAG))
				property = property->next;
		}
		if (property && (property->ID == XS_NO_ID) && (property->kind == XS_ARRAY_KIND)) {
			return fxDeleteArrayProperty(the, property, index);
		}
		return 1;
	}
}

void fxEachObjectProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txSlot* instance)
{
	txSlot* property;
	txFlag propertyFlag = ((flag & XS_EACH_ENUMERABLE_FLAG) ? XS_DONT_ENUM_FLAG : 0) | XS_INTERNAL_FLAG;
	if (flag & XS_EACH_STRING_FLAG) {
		property = instance->next;
		if (property && (property->flag & XS_INTERNAL_FLAG) && ((property->kind == XS_STRING_KIND) || (property->kind == XS_STRING_X_KIND))) {
			txIndex length = fxUnicodeLength(property->value.string), index;
			for (index = 0; index < length; index++) {
				fxStepInstanceProperty(the, target, flag, step, context, 0, index, &mxEmptyString);
			}
			property = property->next;
		}
		while (property) {
			if ((property->ID == XS_NO_ID) && ((property->kind == XS_ARRAY_KIND) || (property->kind == XS_ARGUMENTS_STRICT_KIND) || (property->kind == XS_ARGUMENTS_SLOPPY_KIND))) {
				txSlot* slot = property->value.array.address;
				if (slot) {
					txSlot* limit = slot + ((((txChunk*)(((txByte*)slot) - sizeof(txChunk)))->size) / sizeof(txSlot));
					while (slot < limit) {
						if (!(slot->flag & propertyFlag)) {
							txIndex index = *((txIndex*)slot);
							fxStepInstanceProperty(the, target, flag, step, context, 0, index, (slot->kind == XS_CLOSURE_KIND) ? slot->value.closure : slot);
						}
						slot++;
					}
				}
				break;
			}
			property = property->next;
		}
		property = instance->next;
		while (property) {
			if (!(property->flag & propertyFlag)) {
				if (property->ID < XS_NO_ID) {
					txSlot* key = fxGetKey(the, property->ID);
					if (key && (key->flag & XS_DONT_ENUM_FLAG))
						fxStepInstanceProperty(the, target, flag, step, context, property->ID, XS_NO_ID, (property->kind == XS_CLOSURE_KIND) ? property->value.closure : property);
				}
			}
			property = property->next;
		}
	}
	if (flag & XS_EACH_SYMBOL_FLAG) {
		property = instance->next;
		while (property) {
			if (!(property->flag & propertyFlag)) {
				if (property->ID < XS_NO_ID) {
					txSlot* key = fxGetKey(the, property->ID);
					if (!key || !(key->flag & XS_DONT_ENUM_FLAG)) {
						fxStepInstanceProperty(the, target, flag, step, context, property->ID, XS_NO_ID, (property->kind == XS_CLOSURE_KIND) ? property->value.closure : property);
					}
				}
			}
			property = property->next;
		}
	}
}

void fxEnumerateObject(txMachine* the, txSlot* instance)
{
	txSlot* iterator;
	txSlot* result;
	txSlot aContext;
	txSlot** address;
	txSlot* slot;

	mxPush(mxEnumeratorFunction);
	fxGetID(the, mxID(_prototype));
	iterator = fxNewObjectInstance(the);
	mxPush(mxObjectPrototype);
	result = fxNewObjectInstance(the);
	slot = fxNextUndefinedProperty(the, result, mxID(_value), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextBooleanProperty(the, slot, 0, mxID(_done), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextSlotProperty(the, iterator, the->stack, mxID(_result), XS_GET_ONLY);
    the->stack++;
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_GET_ONLY;
	slot->ID = mxID(_iterable);
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = instance;
	mxPullSlot(mxResult);
	aContext.value.array.length = 0;
	aContext.value.array.address = slot;
	aContext.value.array.address->next = C_NULL;
	while (instance) {
		fxEachInstanceProperty(the, instance, XS_EACH_STRING_FLAG, fxEnumerateProperty, &aContext, instance);
		instance = fxGetParent(the, instance);
	}
	address = &(iterator->next->next->next);
	while ((slot = *address)) {
		if (slot->flag & XS_DONT_ENUM_FLAG) {
			*address = slot->next;
		}
		else {
			txID id = slot->value.at.id;
			txIndex index = slot->value.at.index;
			fxKeyAt(the, id, index, slot);
			address = &(slot->next);
		}
	}
}

void fxEnumerateProperty(txMachine* the, txSlot* context, txID id, txIndex index, txSlot* theProperty) 
{
	txSlot* item = context->value.array.address;
	if (id < 0) {
		txSlot* key = fxGetKey(the, id);
		if (!key || !(key->flag & XS_DONT_ENUM_FLAG))
			return;
	}
	while (item) {
		if ((item->value.at.id == id) && (item->value.at.index == index))
			return;
		item = item->next;
	}
	item = fxNewSlot(the);
	item->flag = theProperty->flag;
	item->kind = XS_AT_KIND;
	item->value.at.id = id;
	item->value.at.index = index;
	context->value.array.address->next = item;
	context->value.array.address = item;
	context->value.array.length++;
}

txBoolean fxGetObjectOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot)
{
	txSlot* property = fxGetProperty(the, instance, id, index, XS_OWN);
	if (property) {
		if ((property->kind == XS_ACCESSOR_KIND) && (property->value.accessor.getter == mxStringAccessor.value.accessor.getter)) {
			mxPushInteger(0);
			/* THIS */
			mxPushReference(instance);
			/* FUNCTION */
			mxPushReference(property->value.accessor.getter);
			fxCall(the);
			mxPullSlot(slot);
			slot->flag = XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
		}
		else {
			slot->flag = property->flag;
			slot->kind = property->kind;
			slot->value = property->value;
		}
		return 1;
	}
	slot->flag = XS_NO_FLAG;
	slot->kind = XS_UNDEFINED_KIND;
	return 0;
}

txBoolean fxGetObjectProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* value)
{
	txBoolean result = 0;
	txSlot* property = fxGetProperty(the, instance, id, index, XS_OWN);
	mxPushNull();
	if (!property) {
		if (fxGetInstancePrototype(the, instance, the->stack))
			result = fxGetInstanceProperty(the, the->stack->value.reference, id, index, receiver, value);
		goto bail;
	}
	if (property->kind == XS_ACCESSOR_KIND) {
		txSlot* function = property->value.accessor.getter;
		if (!mxIsFunction(function))
			goto bail;
		mxPushInteger(0);
		/* THIS */
		mxPushSlot(receiver);
		/* FUNCTION */
		mxPushReference(function);
		fxCall(the);
		mxPullSlot(value);
	}
	else {
		value->kind = property->kind;
		value->value = property->value;
	}
	result = 1;
bail:
	mxPop();
	return result;
}

txBoolean fxGetObjectPrototype(txMachine* the, txSlot* instance, txSlot* result)
{
	txSlot* prototype = instance->value.instance.prototype;
	if (prototype) {
		result->kind = XS_REFERENCE_KIND;
		result->value.reference = prototype;
		return 1;
	}
	result->kind = XS_NULL_KIND;
	return 0;
}

txBoolean fxHasObjectProperty(txMachine* the, txSlot* instance, txID id, txIndex index) 
{
	txSlot* property = fxGetProperty(the, instance, id, index, XS_ANY);
	if (property) {
		the->scratch.kind = property->kind;
		the->scratch.value = property->value;
		return 1;
	}
	the->scratch.kind = XS_UNDEFINED_KIND;
	return 0;
}

txBoolean fxIsObjectExtensible(txMachine* the, txSlot* instance)
{
	return (instance->flag & XS_DONT_PATCH_FLAG) ? 0 : 1;
}

txBoolean fxPreventObjectExtensions(txMachine* the, txSlot* instance)
{
	instance->flag |= XS_DONT_PATCH_FLAG;
	return 1;
}

txBoolean fxSetObjectProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver)
{
	txBoolean result = 0;
	txSlot* property;
	txSlot* prototype;
	mxPushUndefined();
	property = the->stack;
	mxPushNull();
	prototype = the->stack;
	if (!fxGetInstanceOwnProperty(the, instance, id, index, property)) {
		if (fxGetInstancePrototype(the, instance, prototype)) {
			result = fxSetInstanceProperty(the, prototype->value.reference, id, index, value, receiver);
			goto bail;
		}
	}
	if (property->kind == XS_ACCESSOR_KIND) {
		txSlot* function = property->value.accessor.setter;
		if (!mxIsFunction(function))
			goto bail;
		mxPushSlot(value);
		mxPushInteger(1);
		/* THIS */
		mxPushSlot(receiver);
		/* FUNCTION */
		mxPushReference(function);
		fxCall(the);
		mxPop();
		result = 1;
		goto bail;
	}
	if (property->flag & XS_DONT_SET_FLAG)
		goto bail;
	if (receiver->kind != XS_REFERENCE_KIND)
		goto bail;
	if (fxGetInstanceOwnProperty(the, receiver->value.reference, id, index, property)) {
		if (property->kind == XS_ACCESSOR_KIND)
			goto bail;
		if (property->flag & XS_DONT_SET_FLAG)
			goto bail;
	}
	value->flag = XS_NO_FLAG;
	result = fxDefineInstanceProperty(the, receiver->value.reference, id, index, value, XS_GET_ONLY);
bail:
	mxPop();
	mxPop();
	return result;
}

txBoolean fxSetObjectPrototype(txMachine* the, txSlot* instance, txSlot* slot)
{
	txSlot* prototype = (slot->kind == XS_NULL_KIND) ? C_NULL : slot->value.reference;
	if (instance->value.instance.prototype != prototype) {
		if (instance->flag & XS_DONT_PATCH_FLAG)
			return 0;
		slot = prototype;
		while (slot) {
			if (instance == slot) 
				return 0;
			slot = slot->value.instance.prototype;
		}
		instance->value.instance.prototype = prototype;
	}
	return 1;
}

void fxStepObjectProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txID id, txIndex index, txSlot* property)
{
	if (flag & XS_STEP_GET_FLAG) {
		mxPushReference(target);
		fxGetAll(the, id, index);
		property = the->stack;
	}
	(*step)(the, context, id, index, property);
	if (flag & XS_STEP_GET_FLAG)
		mxPop();
}

void fx_species_get(txMachine* the)
{
	*mxResult = *mxThis;
}

txFlag fxDescriptorToSlot(txMachine* the, txSlot* descriptor)
{
	txSlot* stack = the->stack;
	txSlot* configurable = C_NULL;
	txSlot* enumerable = C_NULL;
	txSlot* get = C_NULL;
	txSlot* set = C_NULL;
	txSlot* value = C_NULL;
	txSlot* writable = C_NULL;
	txSlot* getFunction = C_NULL;
	txSlot* setFunction = C_NULL;
	txFlag mask = 0;
	if (!mxIsReference(descriptor))
		mxTypeError("descriptor is no object");
	mxPushSlot(descriptor);
	if (fxHasID(the, mxID(_enumerable))) {
		mxPushSlot(descriptor);
		fxGetID(the, mxID(_enumerable));
		enumerable = the->stack;
	}
	mxPushSlot(descriptor);
	if (fxHasID(the, mxID(_configurable))) {
		mxPushSlot(descriptor);
		fxGetID(the, mxID(_configurable));
		configurable = the->stack;
	}
	mxPushSlot(descriptor);
	if (fxHasID(the, mxID(_value))) {
		mxPushSlot(descriptor);
		fxGetID(the, mxID(_value));
		value = the->stack;
	}
	mxPushSlot(descriptor);
	if (fxHasID(the, mxID(_writable))) {
		mxPushSlot(descriptor);
		fxGetID(the, mxID(_writable));
		writable = the->stack;
	}
	mxPushSlot(descriptor);
	if (fxHasID(the, mxID(_get))) {
		mxPushSlot(descriptor);
		fxGetID(the, mxID(_get));
		get = the->stack;
	}
	mxPushSlot(descriptor);
	if (fxHasID(the, mxID(_set))) {
		mxPushSlot(descriptor);
		fxGetID(the, mxID(_set));
		set = the->stack;
	}
	if (get) {
		if (value)
			mxTypeError("get and value");
		if (writable)
			mxTypeError("get and writable");
		if (get->kind != XS_UNDEFINED_KIND) {
			getFunction = fxGetInstance(the, get);
			if (!getFunction || !mxIsFunction(getFunction))
				mxTypeError("get is no function");
		}
		mask |= XS_GETTER_FLAG;
	}
	if (set) {
		if (value)
			mxTypeError("set and value");
		if (writable)
			mxTypeError("set and writable");
		if (set->kind != XS_UNDEFINED_KIND) {
			setFunction = fxGetInstance(the, set);
			if (!setFunction || !mxIsFunction(setFunction))
				mxTypeError("set is no function");
		}
		mask |= XS_SETTER_FLAG;
	}
	descriptor->flag = 0;
	if (configurable) {
		mask |= XS_DONT_DELETE_FLAG;
		if (!fxToBoolean(the, configurable))
			descriptor->flag |= XS_DONT_DELETE_FLAG;
	}
	if (enumerable) {
		mask |= XS_DONT_ENUM_FLAG;
		if (!fxToBoolean(the, enumerable))
			descriptor->flag |= XS_DONT_ENUM_FLAG;
	}
	if (get || set) {
		descriptor->kind = XS_ACCESSOR_KIND;
		descriptor->value.accessor.getter = getFunction;
		descriptor->value.accessor.setter = setFunction;
	}
	else {
		if (writable) {
			mask |= XS_DONT_SET_FLAG;
			if (!fxToBoolean(the, writable))
				descriptor->flag |= XS_DONT_SET_FLAG;
		}
		if (value) {
			descriptor->kind = value->kind;
			descriptor->value = value->value;
		}
		else {
			descriptor->kind = XS_UNINITIALIZED_KIND;
		}
	}
	the->stack = stack;
	return mask;
}

void fxDescribeProperty(txMachine* the, txSlot* property, txFlag mask)
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
		if (property->kind != XS_UNINITIALIZED_KIND)
			slot = fxNextSlotProperty(the, slot, property, mxID(_value), XS_NO_FLAG);
		if (mask & XS_DONT_SET_FLAG)
			slot = fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_SET_FLAG) ? 0 : 1, mxID(_writable), XS_NO_FLAG);
	}
	if (mask & XS_DONT_ENUM_FLAG)
		slot= fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_ENUM_FLAG) ? 0 : 1, mxID(_enumerable), XS_NO_FLAG);
	if (mask & XS_DONT_DELETE_FLAG)
		slot= fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_DELETE_FLAG) ? 0 : 1, mxID(_configurable), XS_NO_FLAG);
}

txBoolean fxIsPropertyCompatible(txMachine* the, txSlot* property, txSlot* slot, txFlag mask)
{
	if (property->flag & XS_DONT_DELETE_FLAG) {
		if ((mask & XS_DONT_DELETE_FLAG) && !(slot->flag & XS_DONT_DELETE_FLAG))
			return 0;
		if ((mask & XS_DONT_ENUM_FLAG) && ((property->flag & XS_DONT_ENUM_FLAG) != (slot->flag & XS_DONT_ENUM_FLAG)))
			return 0;
		if (mask & XS_ACCESSOR_FLAG) {
			if (property->kind != XS_ACCESSOR_KIND)
				return 0;
			if (mask & XS_GETTER_FLAG) {
				if (property->value.accessor.getter != slot->value.accessor.getter)
					return 0;
			}
			if (mask & XS_SETTER_FLAG) {
				if (property->value.accessor.setter != slot->value.accessor.setter)
					return 0;
			}
		}
		else if ((mask & XS_DONT_SET_FLAG) || (slot->kind != XS_UNINITIALIZED_KIND)) {
			if (property->kind == XS_ACCESSOR_KIND)
				return 0;
			if (property->flag & XS_DONT_SET_FLAG) {
				if ((mask & XS_DONT_SET_FLAG) && !(slot->flag & XS_DONT_SET_FLAG))
					return 0;
				if ((slot->kind != XS_UNINITIALIZED_KIND) && !fxIsSameValue(the, property, slot))
					return 0;
			}
		}
	}
	return 1;
}
