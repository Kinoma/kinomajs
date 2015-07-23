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

static void fx_Enumerator(txMachine* the);
static void fx_Enumerator_next(txMachine* the);

static void fx_Proxy(txMachine* the);
static void fx_Proxy_revocable(txMachine* the);
static void fx_Proxy_revoke(txMachine* the);
static void fx_Proxy_get(txMachine* the);
static void fx_Proxy_set(txMachine* the);

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

static txSlot* fxCheckProxyFunction(txMachine* the, txSlot* proxy, txID index);

static void fxCallObject(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments);
static void fxCallProxy(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments);
static void fxConstructObject(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target);
static void fxConstructProxy(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target);
static void fxDefineObjectProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* descriptor);
static void fxDefineProxyProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* descriptor);
static void fxEnumerateObject(txMachine* the, txSlot* instance);
static void fxEnumerateProperty(txMachine* the, txSlot* theContext, txInteger theID, txSlot* theProperty);
static void fxEnumerateProxy(txMachine* the, txSlot* instance);
static void fxGetObjectOwnKeys(txMachine* the, txSlot* instance, txFlag flag);
static void fxGetProxyOwnKeys(txMachine* the, txSlot* instance, txFlag flag);
static void fxGetObjectOwnPropertyDescriptor(txMachine* the, txSlot* instance, txInteger id);
static void fxGetProxyOwnPropertyDescriptor(txMachine* the, txSlot* instance, txInteger id);
static void fxGetObjectPrototype(txMachine* the, txSlot* instance);
static void fxGetProxyPrototype(txMachine* the, txSlot* instance);
static void fxIsObjectExtensible(txMachine* the, txSlot* instance);
static void fxIsProxyExtensible(txMachine* the, txSlot* instance);
static void fxPreventObjectExtensions(txMachine* the, txSlot* instance);
static void fxPreventProxyExtensions(txMachine* the, txSlot* instance);
static void fxSetObjectPrototype(txMachine* the, txSlot* instance, txSlot* prototype);
static void fxSetProxyPrototype(txMachine* the, txSlot* instance, txSlot* prototype);

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

	fxNewHostFunction(the, fx_Proxy_get, 0, XS_NO_ID);
	mxProxyPropertyGetter = *the->stack;
	the->stack++;
	fxNewHostFunction(the, fx_Proxy_set, 1, XS_NO_ID);
	mxProxyPropertySetter = *the->stack;
	the->stack++;
	
	mxPush(mxObjectPrototype);
	fxNewProxyInstance(the);
	mxProxyPrototype = *the->stack;
	slot = fxNewHostFunctionGlobal(the, fx_Proxy, 1, mxID(_Proxy), XS_GET_ONLY);
	slot = mxFunctionInstancePrototype(slot);
	slot->kind = mxProxyPrototype.kind;
	slot->value = mxProxyPrototype.value;
	slot = fxLastProperty(the, slot);
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
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_Enumerator_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	fxNewHostConstructor(the, fx_Enumerator, 0, XS_NO_ID);
	mxPull(mxEnumeratorFunction);
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

txSlot* fxGetProxyProperty(txMachine* the, txSlot* instance, txInteger id)
{
	txSlot* proxy = instance->next;
	txSlot* property = proxy->next;
	property->value.integer = id;
	return property->next;
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
		fxIDToSlot(the, id, the->stack);;
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

txBoolean fxRemoveProxyProperty(txMachine* the, txSlot* instance, txInteger id)
{
	txBoolean result;
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _deleteProperty);
	if (function) {
		fxBeginHost(the);
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxIDToSlot(the, id, the->stack);;
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

void fx_Proxy_get(txMachine* the)
{
	txSlot* proxy = mxThis->value.reference->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _get);
	txInteger id = proxy->next->value.integer;
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxIDToSlot(the, id, the->stack);;
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

void fx_Proxy_set(txMachine* the)
{
	txSlot* proxy = mxThis->value.reference->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _set);
	txInteger id = proxy->next->value.integer;
	txSlot* value = mxArgv(0);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxIDToSlot(the, id, the->stack);;
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

void fx_Reflect_apply(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no function");
	if ((mxArgc < 3) || (mxArgv(2)->kind != XS_REFERENCE_KIND))
		mxTypeError("argumentsList is no object");
	fxCallInstance(the, mxArgv(0)->value.reference, mxArgv(1), mxArgv(2));
}

void fx_Reflect_construct(txMachine* the)
{
	txSlot* target;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
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
	fxDefineInstanceProperty(the, mxArgv(0)->value.reference, id, mxArgv(2));
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
	txInteger id;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	fxSlotToID(the, mxArgv(1), &id);
	fxGetInstanceOwnPropertyDescriptor(the, mxArgv(0)->value.reference, id);
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
	fxGetInstanceOwnKeys(the, mxArgv(0)->value.reference, XS_NO_FLAG);
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
	fxIDToSlot(the, slot->next->value.integer, the->stack);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxCallID(the, mxID(_peek));
	mxPullSlot(mxResult);
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

void fxCallInstance(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxCallProxy(the, instance, _this, arguments);
	else
		fxCallObject(the, instance, _this, arguments);
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

void fxConstructInstance(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxConstructProxy(the, instance, arguments, target);
	else
		fxConstructObject(the, instance, arguments, target);
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
		fxCreateInstance(the, instance);
	else
		mxPushUnitialized();
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

void fxDefineInstanceProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* descriptor)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxDefineProxyProperty(the, instance, id, descriptor);
	else
		fxDefineObjectProperty(the, instance, id, descriptor);
}

void fxDefineObjectProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* descriptor)
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

	descriptor = fxGetInstance(the, descriptor);
	if (descriptor) {
		configurable = fxGetProperty(the, descriptor, mxID(_configurable));
		enumerable = fxGetProperty(the, descriptor, mxID(_enumerable));
		get = fxGetProperty(the, descriptor, mxID(_get));
		set = fxGetProperty(the, descriptor, mxID(_set));
		value = fxGetProperty(the, descriptor, mxID(_value));
		writable = fxGetProperty(the, descriptor, mxID(_writable));
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
						if (!fxIsSameSlot(the, property, value))
							goto not_configurable;
					}
				}
			}
		}
	}
	else {
		property = fxSetProperty(the, instance, id, &aFlag);
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
		if (get) {
			property->value.accessor.getter = getFunction;
			if (mxIsFunction(getFunction)) {
				value = mxFunctionInstanceInfo(getFunction);
				if (value->value.info.name == mxID(_get))
					value->value.info.name = (txID)id; // @@ get id
				value = mxFunctionInstanceHome(getFunction);
				if (value->kind == XS_NULL_KIND) {
					value->kind = XS_REFERENCE_KIND;
					value->value.reference = instance;
				}
			}
		}
		if (set) {
			property->value.accessor.setter = setFunction;
			if (mxIsFunction(setFunction)) {
				value = mxFunctionInstanceInfo(setFunction);
				if (value->value.info.name == mxID(_set))
					value->value.info.name = (txID)id; // @@ set id
				value = mxFunctionInstanceHome(setFunction);
				if (value->kind == XS_NULL_KIND) {
					value->kind = XS_REFERENCE_KIND;
					value->value.reference = instance;
				}
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
			property->kind = value->kind;
			property->value = value->value;
			if (value->kind == XS_REFERENCE_KIND) {
				value = value->value.reference;
				if (mxIsFunction(value)) {
					property = mxFunctionInstanceInfo(value);
					if (property->value.info.name == mxID(_value))
						property->value.info.name = (txID)id;
					property = mxFunctionInstanceHome(value);
					if (property->kind == XS_NULL_KIND) {
						property->kind = XS_REFERENCE_KIND;
						property->value.reference = instance;
					}
				}
			}
		}
		else if (property->kind == XS_ACCESSOR_KIND) {
			property->kind = XS_UNDEFINED_KIND;
		}
	}
	return;
not_configurable:
	mxTypeError("property is not configurable");
}

void fxDefineProxyProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* descriptor)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _defineProperty);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxIDToSlot(the, id, the->stack);;
		mxPushSlot(descriptor);
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
		fxDefineInstanceProperty(the, proxy->value.proxy.target, id, descriptor);
}

void fx_Enumerator(txMachine* the)
{
	txSlot* iterator;
	if (mxThis->kind == XS_REFERENCE_KIND)
		fxEnumerateInstance(the, mxThis->value.reference);
	else {
		mxPushSlot(mxFunctionInstancePrototype(mxEnumeratorFunction.value.reference));
		iterator = fxNewIteratorInstance(the, mxThis);
		mxPullSlot(mxResult);
		iterator->next->next->next = C_NULL;
	}
}

void fx_Enumerator_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	if (index) {
		value->kind = index->kind;
		value->value = index->value;
		iterable->next = index->next;
	}
	else {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

void fxEnumerateInstance(txMachine* the, txSlot* instance)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxEnumerateProxy(the, instance);
	else
		fxEnumerateObject(the, instance);
}

void fxEnumerateObject(txMachine* the, txSlot* instance)
{
	txSlot* iterator;
	txSlot* result;
	txSlot aContext;
	txSlot* slot;

	mxPushSlot(mxFunctionInstancePrototype(mxEnumeratorFunction.value.reference));
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
		fxEachOwnProperty(the, instance, XS_DONT_ENUM_FLAG, fxEnumerateProperty, &aContext);
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

void fxGetInstanceOwnKeys(txMachine* the, txSlot* instance, txFlag flag)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxGetProxyOwnKeys(the, instance, flag);
	else
		fxGetObjectOwnKeys(the, instance, flag);
}

void fxGetObjectOwnKeys(txMachine* the, txSlot* instance, txFlag flag)
{
	txSlot* array;
	txSlot context;
	mxPush(mxArrayPrototype);
	array = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	context.value.array.length = 0;
	context.value.array.address = array->next;
	fxEachOwnProperty(the, instance, flag, fxEnumPropertyKeys, &context);
	array->next->value.array.length = context.value.array.length;
	fxCacheArray(the, array);
}

void fxGetProxyOwnKeys(txMachine* the, txSlot* instance, txFlag flag)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _ownKeys);
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
		fxGetInstanceOwnKeys(the, proxy->value.proxy.target, flag);
}

void fxGetInstanceOwnPropertyDescriptor(txMachine* the, txSlot* instance, txInteger id)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxGetProxyOwnPropertyDescriptor(the, instance, id);
	else
		fxGetObjectOwnPropertyDescriptor(the, instance, id);
}

void fxGetObjectOwnPropertyDescriptor(txMachine* the, txSlot* instance, txInteger id)
{
	txSlot* property;
	txSlot* slot;
	property = fxGetOwnProperty(the, instance, id);
	if (property) {
		mxPushUndefined();
	}
	else if ((instance->flag & XS_VALUE_FLAG) && (id < 0)) {
		switch (instance->next->kind) {
		case XS_ARRAY_KIND:
			if (id == mxID(_length)) {
				mxPush(*mxArgv(0));
				fxGetID(the, id);
				the->stack->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
				property = the->stack;
			}
			break;
		case XS_CALLBACK_KIND:
		case XS_CODE_KIND:
			if (id == mxID(_length)) {
				mxPush(*mxArgv(0));
				fxGetID(the, id);
				the->stack->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
				property = the->stack;
			}
			else if (id == mxID(_name)) {
				mxPush(*mxArgv(0));
				fxGetID(the, id);
				the->stack->flag = XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
				property = the->stack;
			}
			else if (id == mxID(_prototype)) {
				mxPushUndefined();
				property = instance->next->next->next;
			}
			break;
		case XS_STRING_KIND:
			if (id == mxID(_length)) {
				mxPush(*mxArgv(0));
				fxGetID(the, id);
				the->stack->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
				property = the->stack;
			}
			break;
		}
	}
	if (property) {
		mxPush(mxObjectPrototype);
		fxNewObjectInstance(the);
		slot = fxLastProperty(the, fxNewObjectInstance(the));
		slot= fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_DELETE_FLAG) ? 0 : 1, mxID(_configurable), XS_NO_FLAG);
		slot= fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_ENUM_FLAG) ? 0 : 1, mxID(_enumerable), XS_NO_FLAG);
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
			slot = fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_SET_FLAG) ? 0 : 1, mxID(_writable), XS_NO_FLAG);
			slot = fxNextSlotProperty(the, slot, property, mxID(_value), XS_NO_FLAG);
		}
		mxPullSlot(mxResult);
		the->stack++;
	}
}

void fxGetProxyOwnPropertyDescriptor(txMachine* the, txSlot* instance, txInteger id)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _getOwnPropertyDescriptor);
	txBoolean targetExtensible = 1;
	txBoolean targetConfigurable = 1;
		
//	fxIsObjectExtensible(the, proxy->value.proxy.target, id);
//	targetExtensible = fxToBoolean(the, mxResult);
		
	fxGetInstanceOwnPropertyDescriptor(the, proxy->value.proxy.target, id);
	if (mxResult->kind == XS_REFERENCE_KIND) {
		targetConfigurable = fxToBoolean(the, fxGetProperty(the, mxResult->value.reference, mxID(_configurable)));
	}
	
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxIDToSlot(the, id, the->stack);;
		/* ARGC */
		mxPushInteger(2);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		if (the->stack->kind == XS_UNDEFINED_KIND) {
			if (!targetExtensible || !targetConfigurable)
				mxTypeError("proxy handler.getOwnPropertyDescriptor: invalid descriptor");
		}
		else if (the->stack->kind == XS_REFERENCE_KIND) {
			txSlot* slot = the->stack->value.reference;
			txSlot* configurable = fxGetProperty(the, slot, mxID(_configurable));
			txSlot* enumerable = fxGetProperty(the, slot, mxID(_enumerable));
			txSlot* get = fxGetProperty(the, slot, mxID(_get));
			txSlot* set = fxGetProperty(the, slot, mxID(_set));
			txSlot* value = fxGetProperty(the, slot, mxID(_value));
			txSlot* writable = fxGetProperty(the, slot, mxID(_writable));
			slot = fxLastProperty(the, slot);
			if (!configurable)
				slot = fxNextBooleanProperty(the, slot, 0, mxID(_configurable), XS_NO_FLAG);
			if (!enumerable)
				slot = fxNextBooleanProperty(the, slot, 0, mxID(_enumerable), XS_NO_FLAG);
			if (!get && !set) {
				if (!writable)
					slot = fxNextBooleanProperty(the, slot, 0, mxID(_writable), XS_NO_FLAG);
				if (!value)
					slot = fxNextUndefinedProperty(the, slot, mxID(_value), XS_NO_FLAG);
			}
		}
		else
			mxTypeError("proxy handler.getOwnPropertyDescriptor: invalid descriptor");
		mxPullSlot(mxResult);
	}
}

void fxGetInstancePrototype(txMachine* the, txSlot* instance)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxGetProxyPrototype(the, instance);
	else
		fxGetObjectPrototype(the, instance);
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

void fxIsInstanceExtensible(txMachine* the, txSlot* instance)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxIsProxyExtensible(the, instance);
	else
		fxIsObjectExtensible(the, instance);
}

void fxIsObjectExtensible(txMachine* the, txSlot* instance)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (instance->flag & XS_DONT_PATCH_FLAG) ? 0 : 1;
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

void fxPreventInstanceExtensions(txMachine* the, txSlot* instance)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxPreventProxyExtensions(the, instance);
	else
		fxPreventObjectExtensions(the, instance);
}

void fxPreventObjectExtensions(txMachine* the, txSlot* instance)
{
	instance->flag |= XS_DONT_PATCH_FLAG;
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

void fxSetInstancePrototype(txMachine* the, txSlot* instance, txSlot* prototype)
{
	if ((instance->flag & XS_VALUE_FLAG) && (instance->next->kind == XS_PROXY_KIND))
		fxSetProxyPrototype(the, instance, prototype);
	else
		fxSetObjectPrototype(the, instance, prototype);
}

void fxSetObjectPrototype(txMachine* the, txSlot* instance, txSlot* prototype)
{
	if (prototype->kind == XS_NULL_KIND)
		instance->value.instance.prototype = C_NULL;
	else
		instance->value.instance.prototype = prototype->value.reference;
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
