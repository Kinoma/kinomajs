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

static void fx_Object(txMachine* the);
static void fx_Object_prototype___proto__get(txMachine* the);
static void fx_Object_prototype___proto__set(txMachine* the);
static void fx_Object_prototype_hasOwnProperty(txMachine* the);
static void fx_Object_prototype_isPrototypeOf(txMachine* the);
static void fx_Object_prototype_propertyIsEnumerable(txMachine* the);
static void fx_Object_prototype_propertyIsScriptable(txMachine* the);
static void fx_Object_prototype_toPrimitive(txMachine* the);
static void fx_Object_prototype_toString(txMachine* the);
static void fx_Object_prototype_valueOf(txMachine* the);

static void fx_Object_assign(txMachine* the);
static void fx_Object_create(txMachine* the);
static void fx_Object_defineProperties(txMachine* the);
static void fx_Object_defineProperty(txMachine* the);
static void fx_Object_freeze(txMachine* the);
static void fx_Object_getOwnPropertyDescriptor(txMachine* the);
static void fx_Object_getOwnPropertyNames(txMachine* the);
static void fx_Object_getOwnPropertySymbols(txMachine* the);
static void fx_Object_getPrototypeOf(txMachine* the);
static void fx_Object_is(txMachine* the);
static void fx_Object_isExtensible(txMachine* the);
static void fx_Object_isFrozen(txMachine* the);
static void fx_Object_isSealed(txMachine* the);
static void fx_Object_keys(txMachine* the);
static void fx_Object_preventExtensions(txMachine* the);
static void fx_Object_seal(txMachine* the);
static void fx_Object_setPrototypeOf(txMachine* the);

static void fxAssignProperty(txMachine* the, txSlot* theResult, txInteger theID, txSlot* theProperty);
static void fxDefineProperty(txMachine* the, txSlot* theInstance, txInteger theID, txSlot* theDescriptor);
static void fxFreezeProperty(txMachine* the, txSlot* theResult, txInteger theID, txSlot* theProperty);
static void fxIsPropertyFrozen(txMachine* the, txSlot* theResult, txInteger theID, txSlot* theProperty);
static void fxIsPropertySealed(txMachine* the, txSlot* theResult, txInteger theID, txSlot* theProperty);
static void fxSealProperty(txMachine* the, txSlot* theResult, txInteger theID, txSlot* theProperty);

void fxBuildObject(txMachine* the)
{
    static const txHostFunctionBuilder gx_Object_prototype_builders[] = {
		{ fx_Object_prototype_hasOwnProperty, 1, _hasOwnProperty },
		{ fx_Object_prototype_isPrototypeOf, 1, _isPrototypeOf },
		{ fx_Object_prototype_propertyIsEnumerable, 1, _propertyIsEnumerable },
		{ fx_Object_prototype_propertyIsScriptable, 1, _propertyIsScriptable },
		{ fx_Object_prototype_toPrimitive, 1, _Symbol_toPrimitive },
		{ fx_Object_prototype_toString, 0, _toLocaleString },
		{ fx_Object_prototype_toString, 0, _toString },
		{ fx_Object_prototype_valueOf, 0, _valueOf },
		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_Object_builders[] = {
		{ fx_Object_assign, 2, _assign },
		{ fx_Object_create, 2, _create },
		{ fx_Object_defineProperties, 2, _defineProperties },
		{ fx_Object_defineProperty, 3, _defineProperty },
		{ fx_Object_freeze, 1, _freeze },
		{ fx_Object_getOwnPropertyDescriptor, 2, _getOwnPropertyDescriptor },
		{ fx_Object_getOwnPropertyNames, 1, _getOwnPropertyNames },
		{ fx_Object_getOwnPropertySymbols, 1, _getOwnPropertySymbols },
		{ fx_Object_getPrototypeOf, 1, _getPrototypeOf },
		{ fx_Object_is, 1, _is },
		{ fx_Object_isExtensible, 1, _isExtensible },
		{ fx_Object_isFrozen, 1, _isFrozen },
		{ fx_Object_isSealed, 1, _isSealed },
		{ fx_Object_keys, 1, _keys },
		{ fx_Object_preventExtensions, 1, _preventExtensions },
		{ fx_Object_seal, 1, _seal },
		{ fx_Object_setPrototypeOf, 1, _setPrototypeOf },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, the->stack->value.reference);
	slot = fxNextHostAccessorProperty(the, slot, fx_Object_prototype___proto__get, fx_Object_prototype___proto__set, mxID(___proto__), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	for (builder = gx_Object_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_Object, 1, mxID(_Object), XS_GET_ONLY));
	for (builder = gx_Object_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	the->stack++;
}

txSlot* fxNewObjectInstance(txMachine* the)
{
	txSlot* instance;
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = the->stack->value.reference;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;
	return instance;
}

void fx_Object(txMachine* the)
{
	if ((mxArgc == 0) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND)) {
		if (mxTarget->kind == XS_UNDEFINED_KIND) {
			mxPush(mxObjectPrototype);
			fxNewObjectInstance(the);
			mxPullSlot(mxResult);
		}
	}
	else {
		*mxResult = *mxArgv(0);
		fxToInstance(the, mxResult);
	}
}

void fx_Object_prototype___proto__get(txMachine* the)
{
	txSlot* instance;
	if ((mxThis->kind == XS_UNDEFINED_KIND) || (mxThis->kind == XS_NULL_KIND))
		mxTypeError("invalid this");
	instance = fxToInstance(the, mxThis);
	fxGetInstancePrototype(the, instance);
}

void fx_Object_prototype___proto__set(txMachine* the)
{
	txSlot* instance;
	if ((mxThis->kind == XS_UNDEFINED_KIND) || (mxThis->kind == XS_NULL_KIND))
		mxTypeError("invalid this");
	if ((mxArgc < 1) || ((mxArgv(0)->kind != XS_NULL_KIND) && (mxArgv(0)->kind != XS_REFERENCE_KIND)))
		mxTypeError("invalid prototype");
	if (mxThis->kind == XS_REFERENCE_KIND) {
		instance = mxThis->value.reference;
		fxSetInstancePrototype(the, instance, mxArgv(0));
	}
}

void fx_Object_prototype_hasOwnProperty(txMachine* the)
{
	txInteger id;
	txSlot* instance;
	txSlot* property;

	if ((mxThis->kind == XS_UNDEFINED_KIND) || (mxThis->kind == XS_NULL_KIND))
		mxTypeError("invalid this");
	if (mxArgc < 1)
		mxTypeError("invalid key");
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	fxSlotToID(the, mxArgv(0), &id);
	instance = fxToInstance(the, mxThis);
	property = fxGetOwnProperty(the, instance, id);
	if (property)
		mxResult->value.boolean = 1;
	else {
		if ((instance->flag & XS_VALUE_FLAG) && (id < 0)) {
			property = instance->next;
			switch (property->kind) {
			case XS_ARRAY_KIND:
				if (id == mxID(_length))
					mxResult->value.boolean = 1;
				break;
			case XS_CALLBACK_KIND:
			case XS_CODE_KIND:
				if (id == mxID(_length))
					mxResult->value.boolean = 1;
				else if (id == mxID(_name))
					mxResult->value.boolean = 1;
				else if (id == mxID(_prototype)) {
					property = mxFunctionInstancePrototype(instance);
					mxResult->value.boolean = ((property->kind == XS_NULL_KIND) && (property->flag & XS_DONT_SET_FLAG)) ? 0 : 1;
				}
				break;
			}
		}
	}
}

void fx_Object_prototype_isPrototypeOf(txMachine* the)
{
	if ((mxThis->kind == XS_UNDEFINED_KIND) || (mxThis->kind == XS_NULL_KIND))
		mxTypeError("invalid this");
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			txSlot* prototype = fxToInstance(the, mxThis);
			txSlot* instance = fxGetParent(the, slot->value.reference);
			while (instance) {
				if (prototype == instance) {
					mxResult->value.boolean = 1;
					break;
				}
				instance = fxGetParent(the, instance);
			}
		}
	}
}

void fx_Object_prototype_propertyIsEnumerable(txMachine* the)
{
	txInteger id;
	txSlot* instance;
	txSlot* property;
	if (mxArgc < 1)
		mxTypeError("invalid key");
	if ((mxThis->kind == XS_UNDEFINED_KIND) || (mxThis->kind == XS_NULL_KIND))
		mxTypeError("invalid this");
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	fxSlotToID(the, mxArgv(0), &id);
	instance = fxToInstance(the, mxThis);
	property = fxGetOwnProperty(the, instance, id);
	if (property && ((property->flag & XS_DONT_ENUM_FLAG) == 0))
		mxResult->value.boolean = 1;
}

void fx_Object_prototype_propertyIsScriptable(txMachine* the)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
}

void fx_Object_prototype_toPrimitive(txMachine* the)
{
	if (mxThis->kind == XS_REFERENCE_KIND) {
		txInteger hint = ((mxArgc > 0) && (c_strcmp(fxToString(the, mxArgv(0)), "string") == 0)) ? XS_STRING_HINT : XS_NUMBER_HINT;
		if (hint == XS_STRING_HINT) {
			mxPushInteger(0);
			mxPushSlot(mxThis);
			fxCallID(the, mxID(_toString));
			if (mxIsReference(the->stack)) {
        		the->stack++;
				mxPushInteger(0);
				mxPushSlot(mxThis);
				fxCallID(the, mxID(_valueOf));
			}
		}
		else {
			mxPushInteger(0);
			mxPushSlot(mxThis);
			fxCallID(the, mxID(_valueOf));
			if (mxIsReference(the->stack)) {
        		the->stack++;
				mxPushInteger(0);
				mxPushSlot(mxThis);
				fxCallID(the, mxID(_toString));
			}
		}
        if (mxIsReference(the->stack)) {
            if (hint == XS_STRING_HINT)
                mxTypeError("cannot coerce object to string");
            else
                mxTypeError("cannot coerce object to number");
        }
        mxResult->kind = the->stack->kind;
        mxResult->value = the->stack->value;
        the->stack++;
	}
	else {
		mxResult->kind = mxThis->kind;
		mxResult->value = mxThis->value;
	}
}

void fx_Object_prototype_toString(txMachine* the)
{
	txSlot* instance;
	fxCopyStringC(the, mxResult, "[object ");
	switch (mxThis->kind) {
	case XS_UNDEFINED_KIND:
		fxConcatStringC(the, mxResult, "Undefined");
		break;
	case XS_NULL_KIND:
		fxConcatStringC(the, mxResult, "Null");
		break;
	case XS_BOOLEAN_KIND:
		fxConcatStringC(the, mxResult, "Boolean");
		break;
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
		fxConcatStringC(the, mxResult, "Number");
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		fxConcatStringC(the, mxResult, "String");
		break;
	case XS_SYMBOL_KIND:
		fxConcatStringC(the, mxResult, "Symbol");
		break;
	case XS_REFERENCE_KIND:
		mxPushSlot(mxThis);
		fxGetID(the, mxID(_Symbol_toStringTag));
		if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND))
			fxConcatString(the, mxResult, the->stack);
		else {
			instance = mxThis->value.reference;
			if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_GLOBAL_KIND))
				fxConcatStringC(the, mxResult, "global");
			else
				fxConcatStringC(the, mxResult, "Object");
		}
		break;
	default:
		fxConcatStringC(the, mxResult, "Object");
		break;
	}
	fxConcatStringC(the, mxResult, "]");
}

void fx_Object_prototype_valueOf(txMachine* the)
{
	*mxResult = *mxThis;
}

void fx_Object_assign(txMachine* the)
{
	txSlot* target;
	txInteger c, i;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid target");
	target = fxToInstance(the, mxArgv(0));
	c = mxArgc;
	for (i = 1; i < c; i++) {
		txSlot* source = mxArgv(i);
		if ((source->kind == XS_UNDEFINED_KIND) || (source->kind == XS_NULL_KIND))
			continue;
		fxEachOwnProperty(the, fxToInstance(the, source), XS_DONT_ENUM_FLAG, fxAssignProperty, source);
	}
	*mxResult = *mxArgv(0);
}

void fx_Object_create(txMachine* the)
{
	txSlot* properties;
	txSlot* instance;

	if ((mxArgc < 1) || ((mxArgv(0)->kind != XS_NULL_KIND) && (mxArgv(0)->kind != XS_REFERENCE_KIND)))
		mxTypeError("invalid prototype");
	if (mxArgv(0)->kind == XS_NULL_KIND) {
		fxNewInstance(the);
	}
	else {
		mxPushSlot(mxArgv(0));
		fxNewInstanceOf(the);
	}
	mxPullSlot(mxResult);
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		properties = fxToInstance(the, mxArgv(1));
		if (properties) {
			instance = fxGetInstance(the, mxResult);
			fxEachOwnProperty(the, properties, XS_DONT_ENUM_FLAG, fxDefineInstanceProperty, instance);
		}
	}
}

void fx_Object_defineProperties(txMachine* the)
{
	txSlot* instance;
	txSlot* properties;

	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("invalid object");
	if ((mxArgc < 2) || (mxArgv(1)->kind == XS_UNDEFINED_KIND) || (mxArgv(1)->kind == XS_NULL_KIND))
		mxTypeError("invalid properties");
	instance = fxGetInstance(the, mxArgv(0));
	*mxResult = *mxArgv(0);
	properties = fxToInstance(the, mxArgv(1));
	fxEachOwnProperty(the, properties, XS_DONT_ENUM_FLAG, fxDefineInstanceProperty, instance);
}

void fx_Object_defineProperty(txMachine* the)
{
	txSlot* instance;
    txInteger id;

	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("invalid object");
	if (mxArgc < 2)
		mxTypeError("invalid key");
	if ((mxArgc < 3) || (mxArgv(2)->kind != XS_REFERENCE_KIND))
		mxTypeError("invalid descriptor");
	instance = fxGetInstance(the, mxArgv(0));
	*mxResult = *mxArgv(0);
    fxSlotToID(the, mxArgv(1), &id);
	fxDefineInstanceProperty(the, instance, id, mxArgv(2));
}

void fx_Object_freeze(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			fxPreventInstanceExtensions(the, slot);
			fxEachOwnProperty(the, slot, XS_NO_FLAG, fxFreezeProperty, mxResult);
		}
		*mxResult = *mxArgv(0);
	}
}

void fx_Object_getOwnPropertyDescriptor(txMachine* the)
{
	txSlot* instance;
	txInteger id;
	
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	if (mxArgc < 2)
		mxTypeError("invalid key");
	instance = fxToInstance(the, mxArgv(0));
	fxSlotToID(the, mxArgv(1), &id);
	fxGetInstanceOwnPropertyDescriptor(the, instance, id);
}

void fx_Object_getOwnPropertyNames(txMachine* the)
{
	txSlot* instance;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	instance = fxToInstance(the, mxArgv(0));
	fxGetInstanceOwnKeys(the, instance, XS_DONT_ENUM_SYMBOL_FLAG);
}

void fx_Object_getOwnPropertySymbols(txMachine* the)
{
	txSlot* instance;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	instance = fxToInstance(the, mxArgv(0));
	fxGetInstanceOwnKeys(the, instance, XS_DONT_ENUM_STRING_FLAG);
}

void fx_Object_getPrototypeOf(txMachine* the)
{
	txSlot* instance;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	instance = fxToInstance(the, mxArgv(0));
	fxGetInstancePrototype(the, instance);
}

void fx_Object_is(txMachine* the)
{
	txBoolean result = 0;
	if (mxArgc > 1) {
		txSlot* a = mxArgv(0);
		txSlot* b = mxArgv(1);
		if (a->kind == b->kind) {
			if ((XS_UNDEFINED_KIND == a->kind) || (XS_NULL_KIND == a->kind))
				result = 1;
			else if (XS_BOOLEAN_KIND == a->kind)
				result = a->value.boolean == b->value.boolean;
			else if (XS_INTEGER_KIND == a->kind)
				result = a->value.integer == b->value.integer;
			else if (XS_NUMBER_KIND == a->kind)
				result = ((a->value.number == b->value.number) && c_isnormal(a->value.number) && c_isnormal(a->value.number)) || (c_isnan(a->value.number) && c_isnan(b->value.number));
			else if ((XS_STRING_KIND == a->kind) || (XS_STRING_X_KIND == a->kind))
				result = c_strcmp(a->value.string, b->value.string) == 0;
			else if (XS_SYMBOL_KIND == a->kind)
				result = a->value.ID == b->value.ID;
			else if (XS_REFERENCE_KIND == a->kind)
				result = a->value.reference == b->value.reference;
		}
		else if ((XS_INTEGER_KIND == a->kind) && (XS_NUMBER_KIND == b->kind))
			result = (((txNumber)(a->value.integer) == b->value.number)) && c_isnormal(b->value.number);
		else if ((XS_NUMBER_KIND == a->kind) && (XS_INTEGER_KIND == b->kind))
			result = (a->value.number == (txNumber)(b->value.integer)) && c_isnormal(a->value.number);
		else if ((XS_STRING_KIND == a->kind) && (XS_STRING_X_KIND == b->kind))
			result = c_strcmp(a->value.string, b->value.string) == 0;
		else if ((XS_STRING_X_KIND == a->kind) && (XS_STRING_KIND == b->kind))
			result = c_strcmp(a->value.string, b->value.string) == 0;
	}
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = result;
}

void fx_Object_isExtensible(txMachine* the)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			fxIsInstanceExtensible(the, slot);
		}
	}
}

void fx_Object_isFrozen(txMachine* the)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			fxIsInstanceExtensible(the, slot);
			if (mxResult->value.boolean)
				fxEachOwnProperty(the, slot, XS_NO_FLAG, fxIsPropertyFrozen, mxResult);
		}
	}
}

void fx_Object_isSealed(txMachine* the)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			fxIsInstanceExtensible(the, slot);
			if (mxResult->value.boolean)
				fxEachOwnProperty(the, slot, XS_NO_FLAG, fxIsPropertySealed, mxResult);
		}
	}
}

void fx_Object_keys(txMachine* the)
{
	txSlot* instance;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	instance = fxToInstance(the, mxArgv(0));
	fxGetInstanceOwnKeys(the, instance, XS_DONT_ENUM_FLAG | XS_DONT_ENUM_SYMBOL_FLAG);
}

void fx_Object_preventExtensions(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			fxPreventInstanceExtensions(the, slot);
		}
		*mxResult = *mxArgv(0);
	}
}

void fx_Object_seal(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			fxPreventInstanceExtensions(the, slot);
			fxEachOwnProperty(the, slot, XS_NO_FLAG, fxSealProperty, mxResult);
		}
		*mxResult = *mxArgv(0);
	}
}

void fx_Object_setPrototypeOf(txMachine* the)
{
	txSlot* instance;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	if ((mxArgc < 2) || ((mxArgv(1)->kind != XS_NULL_KIND) && (mxArgv(1)->kind != XS_REFERENCE_KIND)))
		mxTypeError("invalid prototype");
	if (mxArgv(0)->kind == XS_REFERENCE_KIND) {
		instance = mxArgv(0)->value.reference;
		fxSetInstancePrototype(the, instance, mxArgv(1));
	}
	*mxResult = *mxArgv(0);
}

void fxAssignProperty(txMachine* the, txSlot* theSource, txInteger theID, txSlot* theProperty)
{
	mxPushSlot(theSource);
	fxGetID(the, theID);
	mxPushSlot(mxArgv(0));
	fxSetID(the, theID);
	the->stack++;
}

void fxDefineProperty(txMachine* the, txSlot* theInstance, txInteger theID, txSlot* theDescriptor)
{
	txSlot* configurable = C_NULL;
	txSlot* enumerable = C_NULL;
	txSlot* get = C_NULL;
	txSlot* set = C_NULL;
	txSlot* value = C_NULL;
	txSlot* writable = C_NULL;
	txSlot* getFunction = C_NULL;
	txSlot* setFunction = C_NULL;
	txSlot* property;
	txFlag aFlag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;

	theDescriptor = fxGetInstance(the, theDescriptor);
	if (theDescriptor) {
		configurable = fxGetProperty(the, theDescriptor, mxID(_configurable));
		enumerable = fxGetProperty(the, theDescriptor, mxID(_enumerable));
		get = fxGetProperty(the, theDescriptor, mxID(_get));
		set = fxGetProperty(the, theDescriptor, mxID(_set));
		value = fxGetProperty(the, theDescriptor, mxID(_value));
		writable = fxGetProperty(the, theDescriptor, mxID(_writable));
	}
	else
		mxTypeError("no descriptor");
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
	property = fxGetOwnProperty(the, theInstance, theID);
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
						if (!fxIsSameSlot(the, property, value))
							goto not_configurable;
					}
				}
			}
		}
	}
	else {
		property = fxSetProperty(the, theInstance, theID, &aFlag);
		if (!property)
			mxTypeError("instance is not extensible");
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
		if (get)
			property->value.accessor.getter = getFunction;
		if (set)
			property->value.accessor.setter = setFunction;
	}
	else if (value || writable) {
		if (writable) {
			if (fxToBoolean(the, writable))
				property->flag &= ~XS_DONT_SET_FLAG;
			else
				property->flag |= XS_DONT_SET_FLAG;
		}
		if (value) {
			property->kind = value->kind;
			property->value = value->value;
		}
		else if (property->kind == XS_ACCESSOR_KIND) {
			property->kind = XS_UNDEFINED_KIND;
		}
	}
	return;
not_configurable:
	mxTypeError("property is not configurable");
}

void fxEnumPropertyKeys(txMachine* the, txSlot* theContext, txInteger theID, txSlot* theProperty) 
{
	txSlot* item = fxNewSlot(the);
	if (theID < 0) {
		txSlot* key = fxGetKey(the, (txID)theID);
		if (key && (key->flag & XS_DONT_ENUM_FLAG)) {
			if (key->kind == XS_KEY_KIND) {
				item->kind = XS_STRING_KIND;
				item->value.string = key->value.key.string;
			}
			else {
				item->kind = XS_STRING_X_KIND;
				item->value.string = key->value.key.string;
			}
		}
		else {
			item->kind = XS_SYMBOL_KIND;
			item->value.ID = (txID)theID;
		}
	}
	else {
		char buffer[16];
		fxCopyStringC(the, item, fxIntegerToString(the->dtoa, theID, buffer, sizeof(buffer)));
	}
	theContext->value.array.address->next = item;
	theContext->value.array.address = item;
	theContext->value.array.length++;
}

void fxFreezeProperty(txMachine* the, txSlot* theResult, txInteger theID, txSlot* theProperty)
{
	if (theProperty->kind != XS_ACCESSOR_KIND) 
		theProperty->flag |= XS_DONT_SET_FLAG;
	theProperty->flag |= XS_DONT_DELETE_FLAG;
}

void fxIsPropertyFrozen(txMachine* the, txSlot* theResult, txInteger theID, txSlot* theProperty)
{
	if (theProperty->kind != XS_ACCESSOR_KIND) 
		if (!(theProperty->flag & XS_DONT_SET_FLAG))
			theResult->value.boolean = 0;
	if (!(theProperty->flag & XS_DONT_DELETE_FLAG))
		theResult->value.boolean = 0;
}

void fxIsPropertySealed(txMachine* the, txSlot* theResult, txInteger theID, txSlot* theProperty)
{
	if (!(theProperty->flag & XS_DONT_DELETE_FLAG))
		theResult->value.boolean = 0;
}

void fxSealProperty(txMachine* the, txSlot* theResult, txInteger theID, txSlot* theProperty)
{
	theProperty->flag |= XS_DONT_DELETE_FLAG;
}

