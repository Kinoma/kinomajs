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

static void fx_Proxy(txMachine* the);
static void fx_Proxy_revocable(txMachine* the);
static void fx_Proxy_revoke(txMachine* the);

static txSlot* fxCheckProxyFunction(txMachine* the, txSlot* proxy, txID index);
static void fxGetProxyPropertyAux(txMachine* the);
static void fxSetProxyPropertyAux(txMachine* the);

static void fx_Reflect_apply(txMachine* the);
static void fx_Reflect_construct(txMachine* the);
static void fx_Reflect_defineProperty(txMachine* the);
static void fx_Reflect_deleteProperty(txMachine* the);
static void fx_Reflect_enumerate(txMachine* the);
static void fx_Reflect_get(txMachine* the);
static void fx_Reflect_getOwnPropertyDescriptor(txMachine* the);
static void fx_Reflect_getPrototypeOf(txMachine* the);
static void fx_Reflect_has(txMachine* the);
static void fx_Reflect_isExtensible(txMachine* the);
static void fx_Reflect_ownKeys(txMachine* the);
static void fx_Reflect_preventExtensions(txMachine* the);
static void fx_Reflect_set(txMachine* the);
static void fx_Reflect_setPrototypeOf(txMachine* the);

static void fx_Hook_get(txMachine* the);

void fxBuildProxy(txMachine* the)
{
    static const txHostFunctionBuilder gx_Reflect_builders[] = {
		{ fx_Reflect_apply, 3, _apply },
		{ fx_Reflect_construct, 2, _construct },
		{ fx_Reflect_defineProperty, 3, _defineProperty },
		{ fx_Reflect_deleteProperty, 2, _deleteProperty },
		{ fx_Reflect_enumerate, 1, _enumerate },
		{ fx_Reflect_get, 2, _get },
		{ fx_Reflect_getOwnPropertyDescriptor, 2, _getOwnPropertyDescriptor },
		{ fx_Reflect_getPrototypeOf, 1, _getPrototypeOf },
		{ fx_Reflect_has, 2, _has },
		{ fx_Reflect_isExtensible, 1, _isExtensible },
		{ fx_Reflect_ownKeys, 1, _ownKeys },
		{ fx_Reflect_preventExtensions, 1, _preventExtensions },
		{ fx_Reflect_set, 3, _set },
		{ fx_Reflect_setPrototypeOf, 2, _setPrototypeOf },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;

	fxNewHostFunction(the, fxGetProxyPropertyAux, 0, XS_NO_ID);
	mxProxyPropertyGetter = *the->stack;
	the->stack++;
	fxNewHostFunction(the, fxSetProxyPropertyAux, 1, XS_NO_ID);
	mxProxyPropertySetter = *the->stack;
	the->stack++;
	
	mxPush(mxObjectPrototype);
	fxNewProxyInstance(the);
	mxProxyPrototype = *the->stack;
	slot = fxNewHostFunctionGlobal(the, fx_Proxy, 1, mxID(_Proxy), XS_GET_ONLY);
	slot = fxLastProperty(the, slot);
	slot = fxNextSlotProperty(the, slot, &mxProxyPrototype, mxID(_prototype), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_Proxy_revocable, 2, mxID(_revocable), XS_DONT_ENUM_FLAG);
	the->stack++;

	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	for (builder = gx_Reflect_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Reflect", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxSetGlobalProperty(the, mxGlobal.value.reference, mxID(_Reflect), C_NULL);
	slot->flag = XS_GET_ONLY;
	slot->kind = the->stack->kind;
	slot->value = the->stack->value;
	the->stack++;
	
	slot = fxNewInstance(the);
	slot = fxNextIntegerProperty(the, slot, 0, XS_NO_ID, XS_GET_ONLY);
	slot = fxNextHostAccessorProperty(the, slot, fx_Hook_get, C_NULL, XS_NO_ID, XS_GET_ONLY);
	mxPull(mxHookInstance);		
}

txSlot* fxNewProxyInstance(txMachine* the)
{
	txSlot* prototype;
	txSlot* instance;
	txSlot* property;
	
	prototype = the->stack->value.reference;
	
	instance = fxNewSlot(the);
	instance->flag = XS_VALUE_FLAG;
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = instance;

	property = instance->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_PROXY_KIND;
	if (prototype->flag == XS_VALUE_FLAG) {
		txSlot* slot = prototype->next;
		property->value.proxy.handler = slot->value.proxy.handler;
		property->value.proxy.target = slot->value.proxy.target;
	}
	else {
		property->value.proxy.handler = C_NULL;
		property->value.proxy.target = C_NULL;
    }
	
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_INTEGER_KIND;
	
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_ACCESSOR_KIND;
	property->value.accessor.getter = mxProxyPropertyGetter.value.reference;
	property->value.accessor.setter = mxProxyPropertySetter.value.reference;
	return instance;
}

void fx_Proxy(txMachine* the)
{
	txSlot* proxy;
	txSlot* target;
	txSlot* handler;
	if (mxTarget->kind != XS_REFERENCE_KIND)
		mxTypeError("no new.target");
	proxy = mxThis->value.reference->next;
	if (!proxy || (proxy->kind != XS_PROXY_KIND))
		mxTypeError("this is no proxy");
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	target = mxArgv(0)->value.reference;
	if ((target->flag & XS_VALUE_FLAG) && (target->next->kind == XS_PROXY_KIND) && !target->next->value.proxy.handler)
		mxTypeError("target is a revoked proxy");
	if ((mxArgc < 2) || (mxArgv(1)->kind != XS_REFERENCE_KIND))
		mxTypeError("handler is no object");
	handler = mxArgv(1)->value.reference;
	if ((handler->flag & XS_VALUE_FLAG) && (handler->next->kind == XS_PROXY_KIND) && !handler->next->value.proxy.handler)
		mxTypeError("handler is a revoked proxy");
	proxy->value.proxy.target = target;
	proxy->value.proxy.handler = handler;
}

void fx_Proxy_revocable(txMachine* the)
{
	txSlot* target;
	txSlot* handler;
	txSlot* property;
	txSlot* instance;
	txSlot* slot;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	target = mxArgv(0)->value.reference;
	if ((target->flag & XS_VALUE_FLAG) && (target->next->kind == XS_PROXY_KIND) && !target->next->value.proxy.handler)
		mxTypeError("target is a revoked proxy");
	if ((mxArgc < 2) || (mxArgv(1)->kind != XS_REFERENCE_KIND))
		mxTypeError("handler is no object");
	handler = mxArgv(1)->value.reference;
	if ((handler->flag & XS_VALUE_FLAG) && (handler->next->kind == XS_PROXY_KIND) && !handler->next->value.proxy.handler)
		mxTypeError("handler is a revoked proxy");
		
	mxPush(mxObjectPrototype);
	property = fxLastProperty(the, fxNewObjectInstance(the));
	mxPullSlot(mxResult);
	
	mxPush(mxProxyPrototype);
	instance = fxNewProxyInstance(the);
	slot = instance->next;
	slot->value.proxy.target = target;
	slot->value.proxy.handler = handler;
	property = fxNextSlotProperty(the, property, the->stack, mxID(_proxy), XS_GET_ONLY);
	
	slot = fxLastProperty(the, fxNewHostFunction(the, fx_Proxy_revoke, 0, mxID(_revoke)));
	slot = fxNextSlotProperty(the, slot, the->stack + 1, mxID(_proxy), XS_GET_ONLY);
	property = fxNextSlotProperty(the, property, the->stack, mxID(_revoke), XS_GET_ONLY);
	
	the->stack += 2;
}

void fx_Proxy_revoke(txMachine* the)
{
	txSlot* property = fxGetProperty(the, mxFunction->value.reference, mxID(_proxy));
	if (property && (property->kind == XS_REFERENCE_KIND)) {
		txSlot* instance = property->value.reference;
		txSlot* proxy = instance->next;
		if (!proxy || (proxy->kind != XS_PROXY_KIND))
			mxTypeError("no proxy");
		proxy->value.proxy.target = C_NULL;
		proxy->value.proxy.handler = C_NULL;
		property->kind = XS_NULL_KIND;
	}
}

txSlot* fxCheckProxyFunction(txMachine* the, txSlot* proxy, txID index)
{
	txSlot* function;
	if (!proxy->value.proxy.handler)
		mxTypeError("proxy handler is no object");
	if (!proxy->value.proxy.target)
		mxTypeError("proxy target is no object");
	function = fxGetProperty(the, proxy->value.proxy.handler, mxID(index));
	if (function) {
		if (!mxIsReference(function) || !mxIsFunction(function->value.reference))
			mxTypeError("proxy handler.%s is no function", fxName(the, mxID(index)));
	}
	return function;
}

void fxCallProxy(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _apply);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushSlot(_this);
		mxPushSlot(arguments);
		/* ARGC */
		mxPushInteger(3);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		mxPullSlot(mxResult);
	}
	else 
		fxCallInstance(the, proxy->value.proxy.target, _this, arguments);
}

void fxConstructProxy(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _construct);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushSlot(arguments);
		mxPushSlot(mxTarget);
		/* ARGC */
		mxPushInteger(3);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		mxPullSlot(mxResult);
	}
	else 
		fxConstructInstance(the, proxy->value.proxy.target, arguments, target);
}

txBoolean fxDefineProxyProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* descriptor)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _defineProperty);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxIDToSlot(the, id, the->stack);
		mxPushSlot(descriptor);
		/* ARGC */
		mxPushInteger(3);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		return fxToBoolean(the, the->stack++);
	}
	return fxDefineInstanceProperty(the, proxy->value.proxy.target, id, descriptor);
}

void fxEachProxyProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txSlot* instance) 
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _ownKeys);
	if (function) {
		txSlot* list = fxNewInstance(the);
		txInteger length;
		txSlot* reference;
		txSlot* item;
		txInteger index;
		txSlot* key;
		txInteger id;
		mxPushReference(proxy->value.proxy.target);
		/* ARGC */
		mxPushInteger(1);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		reference = the->stack;
		mxPushSlot(reference);
		fxGetID(the, mxID(_length));
		length = fxToInteger(the, the->stack++);
		item = list;
		index = 0;
		while (index < length) {
			mxPushSlot(reference);
			fxGetID(the, index);
			key = the->stack;
			if ((key->kind == XS_STRING_KIND) || (key->kind == XS_STRING_X_KIND)) {
				if (flag & XS_EACH_STRING_FLAG) {
					fxSlotToID(the, key, &id);
					fxStepInstanceProperty(the, target, flag, step, context, id, C_NULL);
				}
			}
			else if (key->kind == XS_SYMBOL_KIND) {
				if (flag & XS_EACH_SYMBOL_FLAG) {
					fxSlotToID(the, key, &id);
					fxStepInstanceProperty(the, target, flag, step, context, id, C_NULL);
				}
			}
			else {
				mxTypeError("key is neither string nor symbol");
			}
			mxPop();
			index++;
		}
		mxPop();
	}
	else {
		fxEachInstanceProperty(the, target, flag, step, context, proxy->value.proxy.target);
	}
}

void fxEnumerateProxy(txMachine* the, txSlot* instance)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _enumerate);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		/* ARGC */
		mxPushInteger(1);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		mxPullSlot(mxResult);
	}
	else
		fxEnumerateInstance(the, proxy->value.proxy.target);
}

txSlot* fxGetProxyOwnProperty(txMachine* the, txSlot* instance, txInteger id)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _getOwnPropertyDescriptor);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxIDToSlot(the, id, the->stack);
		/* ARGC */
		mxPushInteger(2);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		if (the->stack->kind == XS_UNDEFINED_KIND)
			return C_NULL;
		if (the->stack->kind == XS_REFERENCE_KIND) {
			txSlot* slot = the->stack->value.reference;
			txSlot* configurable = fxGetProperty(the, slot, mxID(_configurable));
			txSlot* enumerable = fxGetProperty(the, slot, mxID(_enumerable));
			txSlot* get = fxGetProperty(the, slot, mxID(_get));
			txSlot* set = fxGetProperty(the, slot, mxID(_set));
			txSlot* value = fxGetProperty(the, slot, mxID(_value));
			txSlot* writable = fxGetProperty(the, slot, mxID(_writable));
			the->stack->flag = XS_NO_FLAG;
			the->stack->kind = XS_UNDEFINED_KIND;
			if (get || set) {
				if (get) {
					the->stack->value.accessor.getter = fxGetInstance(the, get);
					if (!the->stack->value.accessor.getter || !mxIsFunction(the->stack->value.accessor.getter))
						mxTypeError("get is no function");
				}
				if (set) {
					the->stack->value.accessor.setter = fxGetInstance(the, set);
					if (!the->stack->value.accessor.setter || !mxIsFunction(the->stack->value.accessor.setter))
						mxTypeError("set is no function");
				}
			}
			else {
				if (value) {
					the->stack->kind = value->kind;
					the->stack->value = value->value;
				}
				if (!writable || !fxToBoolean(the, writable))
					the->stack->flag |= XS_DONT_SET_FLAG;
			}
			if (!configurable || !fxToBoolean(the, configurable))
				the->stack->flag |= XS_DONT_DELETE_FLAG;
			if (!enumerable || !fxToBoolean(the, enumerable))
				the->stack->flag |= XS_DONT_ENUM_FLAG;
			return the->stack;
		}
		mxTypeError("proxy handler.getOwnPropertyDescriptor: invalid descriptor");
	}
	return fxGetInstanceOwnProperty(the, proxy->value.proxy.target, id);
}

txSlot* fxGetProxyProperty(txMachine* the, txSlot* instance, txInteger id)
{
	txSlot* proxy = instance->next;
	txSlot* property = proxy->next;
	property->value.integer = id;
	return property->next;
}

void fxGetProxyPropertyAux(txMachine* the)
{
	txSlot* proxy = mxThis->value.reference->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _get);
	txInteger id = proxy->next->value.integer;
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxIDToSlot(the, id, the->stack);
		mxPushSlot(mxThis);
		/* ARGC */
		mxPushInteger(3);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		mxPullSlot(mxResult);
	}
	else {
		mxPushReference(proxy->value.proxy.target);
		fxGetID(the, id);
		mxPullSlot(mxResult);
	}
}

void fxGetProxyPrototype(txMachine* the, txSlot* instance)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _getPrototypeOf);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		/* ARGC */
		mxPushInteger(1);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		mxPullSlot(mxResult);
	}
	else
		fxGetInstancePrototype(the, proxy->value.proxy.target);
}

txBoolean fxHasProxyProperty(txMachine* the, txSlot* instance, txInteger id)
{
	txBoolean result;
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _has);
	if (function) {
		fxBeginHost(the);
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxIDToSlot(the, id, the->stack);
		/* ARGC */
		mxPushInteger(2);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		result = fxToBoolean(the, the->stack++);
		fxEndHost(the);
	}
	else
		result = fxHasProperty(the, proxy->value.proxy.target, id);
	return result;
}

void fxIsProxyExtensible(txMachine* the, txSlot* instance)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _isExtensible);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		/* ARGC */
		mxPushInteger(1);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		mxPullSlot(mxResult);
	}
	else
		fxIsInstanceExtensible(the, proxy->value.proxy.target);
}

void fxPreventProxyExtensions(txMachine* the, txSlot* instance)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _preventExtensions);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		/* ARGC */
		mxPushInteger(1);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		mxPullSlot(mxResult);
	}
	else
		fxPreventInstanceExtensions(the, proxy->value.proxy.target);
}

txBoolean fxRemoveProxyProperty(txMachine* the, txSlot* instance, txInteger id)
{
	txBoolean result;
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _deleteProperty);
	if (function) {
		fxBeginHost(the);
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxIDToSlot(the, id, the->stack);
		/* ARGC */
		mxPushInteger(2);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		result = fxToBoolean(the, the->stack++);
		fxEndHost(the);
	}
	else
		result = fxRemoveProperty(the, proxy->value.proxy.target, id);
	return result;
}

txSlot* fxSetProxyProperty(txMachine* the, txSlot* instance, txInteger id)
{
	txSlot* proxy = instance->next;
	txSlot* property = proxy->next;
	property->value.integer = id;
	return property->next;
}

void fxSetProxyPropertyAux(txMachine* the)
{
	txSlot* proxy = mxThis->value.reference->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _set);
	txInteger id = proxy->next->value.integer;
	txSlot* value = mxArgv(0);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxIDToSlot(the, id, the->stack);
		mxPushSlot(value);
		mxPushSlot(mxThis);
		/* ARGC */
		mxPushInteger(4);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		the->stack++;
	}
	else {
		mxPushSlot(value);
		mxPushReference(proxy->value.proxy.target);
		fxSetID(the, id);
	}
}

void fxSetProxyPrototype(txMachine* the, txSlot* instance, txSlot* prototype)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _setPrototypeOf);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushSlot(prototype);
		/* ARGC */
		mxPushInteger(2);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		mxPullSlot(mxResult);
	}
	else
		fxSetInstancePrototype(the, proxy->value.proxy.target, prototype);
}

void fxStepProxyProperty(txMachine* the, txSlot* instance, txFlag flag, txStep step, txSlot* context, txInteger id, txSlot* property)
{
	txSlot* stack = the->stack;
	if (flag & XS_STEP_GET_OWN_FLAG)
		property = fxGetProxyOwnProperty(the, instance, id);
	if (flag & XS_EACH_ENUMERABLE_FLAG) {
		if (!property || (property->flag & XS_DONT_ENUM_FLAG))
			goto bail;
	}
	if (flag & XS_STEP_GET_FLAG) {
		if (!property)
			goto bail;
		mxPushReference(instance);
		fxGetID(the, id);
		property = the->stack;
	}
	(*step)(the, context, id, property);
	if (flag & XS_STEP_DEFINE_FLAG) {
		fxDescribeProperty(the, property);
		property = the->stack;
		fxDefineProxyProperty(the, instance, id, property);
	}
bail:
	the->stack = stack;
}

void fx_Reflect_apply(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND) || !(mxIsFunction(mxArgv(0)->value.reference) || mxIsProxy(mxArgv(0)->value.reference)))
		mxTypeError("target is no function");
	if ((mxArgc < 3) || (mxArgv(2)->kind != XS_REFERENCE_KIND))
		mxTypeError("argumentsList is no object");
	fxCallInstance(the, mxArgv(0)->value.reference, mxArgv(1), mxArgv(2));
}

void fx_Reflect_construct(txMachine* the)
{
    txSlot* target;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND) || !(mxIsFunction(mxArgv(0)->value.reference) || mxIsProxy(mxArgv(0)->value.reference)))
		mxTypeError("target is no function");
	if ((mxArgc < 2) || (mxArgv(1)->kind != XS_REFERENCE_KIND))
		mxTypeError("argumentsList is no object");
	if (mxArgc < 3)
		target = mxArgv(0);
	else if ((mxArgv(2)->kind != XS_REFERENCE_KIND) || !mxIsFunction(mxArgv(2)->value.reference))
		mxTypeError("newTarget is no function");
	else
		target = mxArgv(2);
	fxConstructInstance(the, mxArgv(0)->value.reference, mxArgv(1), target);
}

void fx_Reflect_defineProperty(txMachine* the)
{
	txInteger id;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	if ((mxArgc < 3) || (mxArgv(2)->kind != XS_REFERENCE_KIND))
		mxTypeError("invalid descriptor");
    fxSlotToID(the, mxArgv(1), &id);
	mxResult->value.boolean = fxDefineInstanceProperty(the, mxArgv(0)->value.reference, id, mxArgv(2));
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Reflect_deleteProperty(txMachine* the)
{
	txInteger id;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	fxSlotToID(the, mxArgv(1), &id);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = fxRemoveProperty(the, mxArgv(0)->value.reference, id);
}

void fx_Reflect_enumerate(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	fxEnumerateInstance(the, mxArgv(0)->value.reference);
}

void fx_Reflect_get(txMachine* the)
{
	txInteger id;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	fxSlotToID(the, mxArgv(1), &id);
	mxPushSlot(mxArgv(0));
	fxGetID(the, id);
	mxPullSlot(mxResult);
}

void fx_Reflect_getOwnPropertyDescriptor(txMachine* the)
{
	txSlot* instance;
	txInteger id;
	txSlot* property;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	fxSlotToID(the, mxArgv(1), &id);
	instance = fxToInstance(the, mxArgv(0));
	fxSlotToID(the, mxArgv(1), &id);
	property = fxGetInstanceOwnProperty(the, instance, id);
	if (property) {
		fxDescribeProperty(the, property);
		mxPullSlot(mxResult);
	}
	mxPop();
}

void fx_Reflect_getPrototypeOf(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	fxGetInstancePrototype(the, mxArgv(0)->value.reference);
}

void fx_Reflect_has(txMachine* the)
{
	txInteger id;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	fxSlotToID(the, mxArgv(1), &id);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = fxHasProperty(the, mxArgv(0)->value.reference, id);
}

void fx_Reflect_isExtensible(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	fxIsInstanceExtensible(the, mxArgv(0)->value.reference);
}

void fx_Reflect_ownKeys(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	fxEnumProperties(the, mxArgv(0)->value.reference, XS_EACH_STRING_FLAG | XS_EACH_SYMBOL_FLAG);
}

void fx_Reflect_preventExtensions(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	fxPreventInstanceExtensions(the, mxArgv(0)->value.reference);
}

void fx_Reflect_set(txMachine* the)
{
	txInteger id;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	if (mxArgc < 3)
		mxTypeError("no value");
	fxSlotToID(the, mxArgv(1), &id);
	mxPushSlot(mxArgv(2));
	mxPushSlot(mxArgv(0));
	fxSetID(the, id);
}

void fx_Reflect_setPrototypeOf(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if ((mxArgc < 2) || ((mxArgv(1)->kind != XS_NULL_KIND) && (mxArgv(1)->kind != XS_REFERENCE_KIND)))
		mxTypeError("invalid prototype");
	fxSetInstancePrototype(the, mxArgv(0)->value.reference, mxArgv(1));
}

txSlot* fxGetHostProperty(txMachine* the, txSlot* instance, txInteger id)
{
	txSlot* slot = mxHookInstance.value.reference;
	slot->next->value.integer = id;
	return slot->next->next;
}

void fx_Hook_get(txMachine* the)
{
	txSlot* slot = mxHookInstance.value.reference;
	mxPushUndefined();
	if (slot->next->value.integer >= 0)
		mxPushInteger(slot->next->value.integer);
	else
		fxIDToSlot(the, slot->next->value.integer, the->stack);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxCallID(the, mxID(_peek));
	mxPullSlot(mxResult);
}
