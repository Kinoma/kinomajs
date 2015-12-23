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

static void fxCallObject(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments);
static void fxConstructObject(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target);
static txBoolean fxDefineObjectProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* descriptor);
static void fxEachObjectProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txSlot* instance);
static void fxEnumerateObject(txMachine* the, txSlot* instance);
static void fxEnumerateProperty(txMachine* the, txSlot* theContext, txInteger theID, txSlot* theProperty);
static txSlot* fxGetObjectOwnProperty(txMachine* the, txSlot* instance, txInteger id);
static void fxGetObjectPrototype(txMachine* the, txSlot* instance);
static void fxIsObjectExtensible(txMachine* the, txSlot* instance);
static void fxPreventObjectExtensions(txMachine* the, txSlot* instance);
static void fxSetObjectPrototype(txMachine* the, txSlot* instance, txSlot* prototype);
static void fxStepObjectProperty(txMachine* the, txSlot* instance, txFlag flag, txStep step, txSlot* context, txInteger id, txSlot* property);

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

void fxCallInstance(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxCallProxy(the, instance, _this, arguments);
	else
		fxCallObject(the, instance, _this, arguments);
}

void fxConstructInstance(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxConstructProxy(the, instance, arguments, target);
	else
		fxConstructObject(the, instance, arguments, target);
}

txBoolean fxDefineInstanceProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* descriptor)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		return fxDefineProxyProperty(the, instance, id, descriptor);
	return fxDefineObjectProperty(the, instance, id, descriptor);
}

void fxEachInstanceProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txSlot* instance)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxEachProxyProperty(the, target, flag, step, context, instance);
	else
		fxEachObjectProperty(the, target, flag, step, context, instance);
}

void fxEnumerateInstance(txMachine* the, txSlot* instance)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxEnumerateProxy(the, instance);
	else
		fxEnumerateObject(the, instance);
}

txSlot* fxGetInstanceOwnProperty(txMachine* the, txSlot* instance, txInteger id)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		return fxGetProxyOwnProperty(the, instance, id);
	return fxGetObjectOwnProperty(the, instance, id);
}

void fxGetInstancePrototype(txMachine* the, txSlot* instance)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxGetProxyPrototype(the, instance);
	else
		fxGetObjectPrototype(the, instance);
}

void fxIsInstanceExtensible(txMachine* the, txSlot* instance)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxIsProxyExtensible(the, instance);
	else
		fxIsObjectExtensible(the, instance);
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

void fxPreventInstanceExtensions(txMachine* the, txSlot* instance)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxPreventProxyExtensions(the, instance);
	else
		fxPreventObjectExtensions(the, instance);
}

void fxSetInstancePrototype(txMachine* the, txSlot* instance, txSlot* prototype)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxSetProxyPrototype(the, instance, prototype);
	else
		fxSetObjectPrototype(the, instance, prototype);
}

void fxStepInstanceProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txInteger id, txSlot* property)
{
	if ((target->flag & XS_VALUE_FLAG) && (target->next->kind == XS_PROXY_KIND))
		fxStepProxyProperty(the, target, flag, step, context, id, property);
	else
		fxStepObjectProperty(the, target, flag, step, context, id, property);
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
		anInstance->next->value.string = theSlot->value.string;
		anInstance->next->next->next->value.integer = fxUnicodeLength(theSlot->value.string);
		if (the->frame->flag & XS_STRICT_FLAG)
			anInstance->flag |= XS_DONT_PATCH_FLAG;
		mxPullSlot(theSlot);
		break;
	case XS_SYMBOL_KIND:
		mxPush(mxSymbolPrototype);
		anInstance = fxNewSymbolInstance(the);
		anInstance->next->value.ID = theSlot->value.ID;
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
		the->stack++;
		fxEndHost(the);
	}
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
	if (fxIsBaseFunctionInstance(the, instance))
		fxCreateInstance(the, target->value.reference);
	else
		mxPushUninitialized();
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

txBoolean fxDefineObjectProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* descriptor)
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
	txBoolean isArrayLength = (instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_ARRAY_KIND) && (id == mxID(_length));
	txSlot* property;
	txFlag aFlag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;

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
	}
	if (isArrayLength)
		property = instance->next;
	else
		property = fxGetOwnProperty(the, instance, id);
	if (property) {
		if (property->flag & XS_DONT_DELETE_FLAG) {
			if (configurable && fxToBoolean(the, configurable)) {
				goto not_configurable;
			}
			if (enumerable && fxToBoolean(the, enumerable)) {
				if (property->flag & XS_DONT_ENUM_FLAG)
					goto not_configurable;
			}
			else {
				if (!(property->flag & XS_DONT_ENUM_FLAG))
					goto not_configurable;
			}
			if (get || set) {
				if (property->kind != XS_ACCESSOR_KIND)
					goto not_configurable;
				if (get) {
					if (property->value.accessor.getter != getFunction) 
						goto not_configurable;
				}
				if (set) {
					if (property->value.accessor.setter != setFunction)
						goto not_configurable;
				}	
			}
			else if (value || writable) {
				if (property->kind == XS_ACCESSOR_KIND)
					goto not_configurable;
				if (property->flag & XS_DONT_SET_FLAG) {
					if (writable && fxToBoolean(the, writable))
						goto not_configurable; 
					if (value) {
						if (isArrayLength)
							mxPushInteger(property->value.array.length);
						else
							mxPushSlot(property);
						if (!fxIsSameValue(the, the->stack, value))
							goto not_configurable;
					}
				}
			}
		}
	}
	else {
		property = fxSetProperty(the, instance, id, &aFlag);
		if (!property)
			goto not_configurable;
		property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	}
	if (configurable) {
		if (fxToBoolean(the, configurable))
			property->flag &= ~XS_DONT_DELETE_FLAG;
		else
			property->flag |= XS_DONT_DELETE_FLAG;
	}
	if (enumerable) {
		if (fxToBoolean(the, enumerable))
			property->flag &= ~XS_DONT_ENUM_FLAG;
		else
			property->flag |= XS_DONT_ENUM_FLAG;
	}
	if (get || set) {
		if (property->kind != XS_ACCESSOR_KIND) {
			property->kind = XS_ACCESSOR_KIND;
			property->value.accessor.getter = C_NULL;
			property->value.accessor.setter = C_NULL;
		}
		if (get) {
			property->value.accessor.getter = getFunction;
			if (mxIsFunction(getFunction)) {
				value = mxFunctionInstanceHome(getFunction);
				if (value->kind == XS_NULL_KIND) {
					value->kind = XS_REFERENCE_KIND;
					value->value.reference = instance;
				}
				value = fxGetProperty(the, getFunction, mxID(_name));
				fxRenameFunction(the, value, id, "get", "get ");
			}
		}
		if (set) {
			property->value.accessor.setter = setFunction;
			if (mxIsFunction(setFunction)) {
				value = mxFunctionInstanceHome(setFunction);
				if (value->kind == XS_NULL_KIND) {
					value->kind = XS_REFERENCE_KIND;
					value->value.reference = instance;
				}
				value = fxGetProperty(the, setFunction, mxID(_name));
				fxRenameFunction(the, value, id, "set", "set ");
			}
		}
	}
	else if (value || writable) {
		if (writable) {
			if (fxToBoolean(the, writable))
				property->flag &= ~XS_DONT_SET_FLAG;
			else
				property->flag |= XS_DONT_SET_FLAG;
		}
		if (value) {
			if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_ARRAY_KIND) && (id == mxID(_length))) {
				txBoolean flag;
				txIndex length;
				if (value->kind == XS_INTEGER_KIND)
					flag = fxIntegerToIndex(the->dtoa, value->value.integer, (txInteger*)&length);
				else
					flag = fxNumberToIndex(the->dtoa, fxToNumber(the, value), (txInteger*)&length);
				if (!flag)
					mxRangeError("invalid length");
				if (!fxSetArrayLength(the, property, length))
					mxTypeError("invalid length");
			}
			else {
				property->kind = value->kind;
				property->value = value->value;
				if (value->kind == XS_REFERENCE_KIND) {
					value = value->value.reference;
					if (mxIsFunction(value)) {
						property = mxFunctionInstanceHome(value);
						if (property->kind == XS_NULL_KIND) {
							property->kind = XS_REFERENCE_KIND;
							property->value.reference = instance;
						}
						property = fxGetProperty(the, value, mxID(_name));
						fxRenameFunction(the, property, id, "value", C_NULL);
					}
				}
			}
		}
		else if (property->kind == XS_ACCESSOR_KIND) {
			property->kind = XS_UNDEFINED_KIND;
		}
	}
	the->stack = stack;
	return 1;
not_configurable:
	the->stack = stack;
	return 0;
}

void fxEachObjectProperty(txMachine* the, txSlot* target, txFlag theFlag, txStep theStep, txSlot* theContext, txSlot* theInstance)
{
	txSlot* aProperty;
	txIndex aCount;
	txIndex anIndex;

	txFlag propertyFlag = (theFlag & XS_EACH_ENUMERABLE_FLAG) ? XS_DONT_ENUM_FLAG : 0;
	if (theFlag & XS_EACH_STRING_FLAG) {
		aProperty = theInstance->next;
		if (theInstance->flag & XS_VALUE_FLAG) {
			if (aProperty->kind == XS_STAR_KIND) {
				aProperty = aProperty->next;
				while (aProperty) {
					if (!(aProperty->flag & propertyFlag)) {
						if (aProperty->ID != XS_NO_ID)
							fxStepInstanceProperty(the, target, theFlag, theStep, theContext, aProperty->ID, aProperty->value.closure);
					}
					aProperty = aProperty->next;
				}
				return;
			}
			else if ((aProperty->kind == XS_STRING_KIND) || (aProperty->kind == XS_STRING_X_KIND)) {
				aCount = fxUnicodeLength(aProperty->value.string);
				for (anIndex = 0; anIndex < aCount; anIndex++) {
					fxStepInstanceProperty(the, target, theFlag, theStep, theContext, anIndex, &mxEmptyString);
				}
			}
		}
		while (aProperty) {
			if (aProperty->ID == XS_NO_ID) {
				if (aProperty->kind == XS_ARRAY_KIND) {
					aCount = aProperty->value.array.length;
					for (anIndex = 0; anIndex < aCount; anIndex++) {
						txSlot* aSlot = aProperty->value.array.address + anIndex;
						if (aSlot->ID && !(aSlot->flag & propertyFlag))
							fxStepInstanceProperty(the, target, theFlag, theStep, theContext, anIndex, aSlot);
					}
					break;
				}
				if (aProperty->kind == XS_PARAMETERS_KIND) {
					aCount = aProperty->value.array.length;
					for (anIndex = 0; anIndex < aCount; anIndex++) {
						txSlot* aSlot = aProperty->value.array.address + anIndex;
						if (aSlot->ID) {
							if (aSlot->kind == XS_CLOSURE_KIND)
								aSlot = aSlot->value.closure;
							if (!(aSlot->flag & propertyFlag))
								fxStepInstanceProperty(the, target, theFlag, theStep, theContext, anIndex, aSlot);
						}
					}
					break;
				}
			}
			aProperty = aProperty->next;
		}
		aProperty = theInstance->next;
		while (aProperty) {
			if (!(aProperty->flag & propertyFlag)) {
				if (aProperty->ID < XS_NO_ID) {
					txSlot* key = fxGetKey(the, aProperty->ID);
					if (key && (key->flag & XS_DONT_ENUM_FLAG)) {
						fxStepInstanceProperty(the, target, theFlag, theStep, theContext, aProperty->ID, aProperty);
					}
				}
			}
			aProperty = aProperty->next;
		}
	}
	if (theFlag & XS_EACH_SYMBOL_FLAG) {
		aProperty = theInstance->next;
		while (aProperty) {
			if (!(aProperty->flag & propertyFlag)) {
				if (aProperty->ID < XS_NO_ID) {
					txSlot* key = fxGetKey(the, aProperty->ID);
					if (!key || !(key->flag & XS_DONT_ENUM_FLAG)) {
						fxStepInstanceProperty(the, target, theFlag, theStep, theContext, aProperty->ID, aProperty);
					}
				}
			}
			aProperty = aProperty->next;
		}
	}
}

void fxEnumerateObject(txMachine* the, txSlot* instance)
{
	txSlot* iterator;
	txSlot* result;
	txSlot aContext;
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
		fxEachInstanceProperty(the, instance, XS_EACH_ENUMERABLE_FLAG | XS_EACH_STRING_FLAG, fxEnumerateProperty, &aContext, instance);
		instance = fxGetParent(the, instance);
	}
	slot = iterator->next->next->next;
	while (slot) {
		txInteger id = slot->value.integer;
		if (id < 0) {
			txSlot* key = fxGetKey(the, (txID)id);
			if (key->kind == XS_KEY_KIND) {
				slot->kind = XS_STRING_KIND;
				slot->value.string = key->value.key.string;
			}
			else {
				slot->kind = XS_STRING_X_KIND;
				slot->value.string = key->value.key.string;
			}
		}
		else {
			char buffer[16];
			fxCopyStringC(the, slot, fxIntegerToString(the->dtoa, id, buffer, sizeof(buffer)));
		}
		slot = slot->next;
	}
}

void fxEnumerateProperty(txMachine* the, txSlot* theContext, txInteger theID, txSlot* theProperty) 
{
	txSlot* item = theContext->value.array.address;
	if (theID < 0) {
		txSlot* key = fxGetKey(the, (txID)theID);
		if (!key || !(key->flag & XS_DONT_ENUM_FLAG))
			return;
	}
	while (item) {
		if (item->value.integer == theID)
			return;
		item = item->next;
	}
	item = fxNewSlot(the);
	item->kind = XS_INTEGER_KIND;
	item->value.integer = theID;
	theContext->value.array.address->next = item;
	theContext->value.array.address = item;
	theContext->value.array.length++;
}

txSlot* fxGetObjectOwnProperty(txMachine* the, txSlot* instance, txInteger id)
{
	txSlot* property = fxGetOwnProperty(the, instance, id);
	if (property) {
		mxPushSlot(property);
		the->stack->flag = property->flag;
		return the->stack;
	}
	if ((instance->flag & XS_VALUE_FLAG) && (id < 0)) {
		switch (instance->next->kind) {
		case XS_ARRAY_KIND:
			if (id == mxID(_length)) {
				mxPushInteger(instance->next->value.array.length);
				the->stack->flag = instance->next->flag;
				return the->stack;
			}
			break;
		case XS_CALLBACK_KIND:
		case XS_CODE_KIND:
		case XS_CODE_X_KIND:
			if (id == mxID(_prototype)) {
				mxPushReference(instance);
				fxGetID(the, id);
				if (the->stack->kind == XS_UNDEFINED_KIND)
					return C_NULL;
				the->stack->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
				return the->stack;
			}
			break;
		}
	}
	mxPushUndefined();
	return C_NULL;
}

void fxGetObjectPrototype(txMachine* the, txSlot* instance)
{
	txSlot* prototype = instance->value.instance.prototype;
	if (prototype) {
		mxResult->value.reference = prototype;
		mxResult->kind = XS_REFERENCE_KIND;
	}
	else
		mxResult->kind = XS_NULL_KIND;
}

void fxIsObjectExtensible(txMachine* the, txSlot* instance)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (instance->flag & XS_DONT_PATCH_FLAG) ? 0 : 1;
}

void fxPreventObjectExtensions(txMachine* the, txSlot* instance)
{
	instance->flag |= XS_DONT_PATCH_FLAG;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
}

void fxSetObjectPrototype(txMachine* the, txSlot* instance, txSlot* slot)
{
	txSlot* prototype = (slot->kind == XS_NULL_KIND) ? C_NULL : slot->value.reference;
	if (instance->value.instance.prototype != slot) {
		if (instance->flag & XS_DONT_PATCH_FLAG)
			mxTypeError("not extensible");
		slot = prototype;
		while (slot) {
			if (instance == slot) 
				mxTypeError("no cycle");
			slot = slot->value.instance.prototype;
		}
		instance->value.instance.prototype = prototype;
	}
}

void fxStepObjectProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txInteger id, txSlot* property)
{
	if (flag & XS_STEP_GET_FLAG) {
		mxPushReference(target);
		fxGetID(the, id);
		property = the->stack;
	}
	(*step)(the, context, id, property);
	if (flag & XS_STEP_GET_FLAG)
		mxPop();
}

void fx_species_get(txMachine* the)
{
	*mxResult = *mxThis;
}




















