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
static void fxEachProxyPropertyStep(txMachine* the, txSlot* context, txID id, txIndex index, txSlot* property);
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
	
	slot = fxNewHostFunctionGlobal(the, fx_Proxy, 2, mxID(_Proxy), XS_DONT_ENUM_FLAG);
	slot = fxLastProperty(the, slot);
	//slot = fxNextSlotProperty(the, slot, &mxProxyPrototype, mxID(_prototype), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_Proxy_revocable, 2, mxID(_revocable), XS_DONT_ENUM_FLAG);
	the->stack++;

	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	for (builder = gx_Reflect_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Reflect", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxSetGlobalProperty(the, mxGlobal.value.reference, mxID(_Reflect));
	slot->flag = XS_DONT_ENUM_FLAG;
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
	txSlot* slot;
	
	prototype = mxIsReference(the->stack) ? the->stack->value.reference : C_NULL;
	
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = C_NULL;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = instance;

	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_PROXY_KIND;
	if (prototype && ((slot = prototype->next)) && (slot->kind = XS_PROXY_KIND)) {
		property->value.proxy.handler = slot->value.proxy.handler;
		property->value.proxy.target = slot->value.proxy.target;
	}
	else {
		property->value.proxy.handler = C_NULL;
		property->value.proxy.target = C_NULL;
    }
	
	property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_INTEGER_KIND;
	
	property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
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
	if ((target->next) && (target->next->kind == XS_PROXY_KIND) && !target->next->value.proxy.handler)
		mxTypeError("target is a revoked proxy");
	if ((mxArgc < 2) || (mxArgv(1)->kind != XS_REFERENCE_KIND))
		mxTypeError("handler is no object");
	handler = mxArgv(1)->value.reference;
	if ((handler->next) && (handler->next->kind == XS_PROXY_KIND) && !handler->next->value.proxy.handler)
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
	if ((target->next) && (target->next->kind == XS_PROXY_KIND) && !target->next->value.proxy.handler)
		mxTypeError("target is a revoked proxy");
	if ((mxArgc < 2) || (mxArgv(1)->kind != XS_REFERENCE_KIND))
		mxTypeError("handler is no object");
	handler = mxArgv(1)->value.reference;
	if ((handler->next) && (handler->next->kind == XS_PROXY_KIND) && !handler->next->value.proxy.handler)
		mxTypeError("handler is a revoked proxy");
		
	mxPush(mxObjectPrototype);
	property = fxLastProperty(the, fxNewObjectInstance(the));
	mxPullSlot(mxResult);
	
	mxPushUndefined();
	instance = fxNewProxyInstance(the);
	slot = instance->next;
	slot->value.proxy.target = target;
	slot->value.proxy.handler = handler;
	property = fxNextSlotProperty(the, property, the->stack, mxID(_proxy), XS_GET_ONLY);
	
	slot = fxLastProperty(the, fxNewHostFunction(the, fx_Proxy_revoke, 0, XS_NO_ID));
	slot = fxNextSlotProperty(the, slot, the->stack + 1, mxID(_proxy), XS_GET_ONLY);
	property = fxNextSlotProperty(the, property, the->stack, mxID(_revoke), XS_GET_ONLY);
	
	the->stack += 2;
}

void fx_Proxy_revoke(txMachine* the)
{
	txSlot* property = fxGetProperty(the, mxFunction->value.reference, mxID(_proxy), XS_NO_ID, XS_ANY);
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
		mxTypeError("(proxy).%s: handler is no object", fxName(the, mxID(index)));
	if (!proxy->value.proxy.target)
		mxTypeError("(proxy).%s: target is no object", fxName(the, mxID(index)));
	function = fxGetProperty(the, proxy->value.proxy.handler, mxID(index), XS_NO_ID, XS_ANY);
	if (function) {
		if (mxIsUndefined(function))
			function = C_NULL;
		else if (!mxIsReference(function) || !mxIsFunction(function->value.reference))
			mxTypeError("(proxy).%s: no function", fxName(the, mxID(index)));
	}
	return function;
}

txSlot* fxAccessProxyProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	txSlot* proxy = instance->next;
	txSlot* property = proxy->next;
	property->value.at.id = id;
	property->value.at.index = index;
	return property->next;
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
		if (!mxIsReference(mxResult))
			mxTypeError("(proxy).construct: no object");
	}
	else 
		fxConstructInstance(the, proxy->value.proxy.target, arguments, target);
}

txBoolean fxDefineProxyProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask)
{
	txBoolean result;
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _defineProperty);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		fxDescribeProperty(the, slot, mask);
		/* ARGC */
		mxPushInteger(3);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (result) {
			mxPushUndefined();
			if (fxGetInstanceOwnProperty(the, proxy->value.proxy.target, id, index, the->stack)) {
				if (fxIsPropertyCompatible(the, the->stack, slot, mask)) {
					if ((mask & XS_DONT_DELETE_FLAG) && (slot->flag & XS_DONT_DELETE_FLAG)) {
						if (!(the->stack->flag & XS_DONT_DELETE_FLAG))
							mxTypeError("(proxy).defineProperty: true with non-configurable descriptor for configurable property");
					}
				}
				else
					mxTypeError("(proxy).defineProperty: true with incompatible descriptor for existent property");
			}
			else if (fxIsInstanceExtensible(the, proxy->value.proxy.target)) {
				if ((mask & XS_DONT_DELETE_FLAG) && (slot->flag & XS_DONT_DELETE_FLAG))
					mxTypeError("(proxy).defineProperty: true with non-configurable descriptor for non-existent property");
			}
			else
				mxTypeError("(proxy).defineProperty: true with descriptor for non-existent property of non-extensible object");
			mxPop();
		}
	}
	else
		result = fxDefineInstanceProperty(the, proxy->value.proxy.target, id, index, slot, mask);
	return result;
}

txBoolean fxDeleteProxyProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	txBoolean result;
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _deleteProperty);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		/* ARGC */
		mxPushInteger(2);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (result) {
			mxPushUndefined();
			if (fxGetInstanceOwnProperty(the, proxy->value.proxy.target, id, index, the->stack)) {
				if (the->stack->flag & XS_DONT_DELETE_FLAG)
					mxTypeError("(proxy).deleteProperty: true for non-configurable property");
			}
			mxPop();
		}
	}
	else
		result = fxDeleteInstanceProperty(the, proxy->value.proxy.target, id, index);
	return result;
}

void fxEachProxyProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txSlot* instance) 
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _ownKeys);
	if (function) {
		txSlot* list = fxNewInstance(the);
		txIndex length;
		txSlot* reference;
		txSlot* item;
		txIndex index;
		txSlot* at;
		txSlot result;
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
			fxGetIndex(the, index);
			at = the->stack;
			if ((at->kind == XS_STRING_KIND) || (at->kind == XS_STRING_X_KIND)) {
				if (flag & XS_EACH_STRING_FLAG) {
					fxAt(the, at);
					item = item->next = fxNewSlot(the);
					mxPullSlot(item);
				}
			}
			else if (at->kind == XS_SYMBOL_KIND) {
				if (flag & XS_EACH_SYMBOL_FLAG) {
					fxAt(the, at);
					item = item->next = fxNewSlot(the);
					mxPullSlot(item);
				}
			}
			else
				mxTypeError("(proxy).ownKeys: key is neither string nor symbol");
			index++;
		}
		mxPop();
		result.flag = fxIsInstanceExtensible(the, proxy->value.proxy.target) ? 0 : XS_DONT_PATCH_FLAG;
		result.value.array.length = length;
		result.value.array.address = list;
		fxEachInstanceProperty(the, proxy->value.proxy.target, (flag & (XS_EACH_STRING_FLAG | XS_EACH_SYMBOL_FLAG)) | XS_STEP_GET_OWN_FLAG, fxEachProxyPropertyStep, &result, proxy->value.proxy.target);
		if ((result.flag & XS_DONT_PATCH_FLAG) && result.value.array.length)
			mxTypeError("(proxy).ownKeys: key for non-existent property of non-extensible object");
			
		at = list->next;
		while (at) {
			fxStepInstanceProperty(the, target, flag, step, context, at->value.at.id, at->value.at.index, C_NULL);
			at = at->next;
		}
		mxPop();
	}
	else {
		fxEachInstanceProperty(the, target, flag, step, context, proxy->value.proxy.target);
	}
}

void fxEachProxyPropertyStep(txMachine* the, txSlot* context, txID id, txIndex index, txSlot* property)
{
	txSlot* at = context->value.array.address->next;
	while (at) {
		if ((at->value.at.id == id) && (at->value.at.index == index)) {
			context->value.array.length--;
			return;
		}
		at = at->next;
	}
	if (property->flag & XS_DONT_DELETE_FLAG)
		mxTypeError("(proxy).ownKeys: no key for non-configurable property");
	if (context->flag & XS_DONT_PATCH_FLAG)
		mxTypeError("(proxy).ownKeys: no key for property of non-extensible object");
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

txBoolean fxGetProxyOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot)
{
	txBoolean result;
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _getOwnPropertyDescriptor);
	if (function) {
		txFlag mask;
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		/* ARGC */
		mxPushInteger(2);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		mxPullSlot(slot);
		mxPushUndefined();
		if (slot->kind == XS_UNDEFINED_KIND) {
			if (fxGetInstanceOwnProperty(the, proxy->value.proxy.target, id, index, the->stack)) {
				if (the->stack->flag & XS_DONT_DELETE_FLAG)
					mxTypeError("(proxy).getOwnPropertyDescriptor: no descriptor for non-configurable property");
				if (!fxIsInstanceExtensible(the, proxy->value.proxy.target)) 
					mxTypeError("(proxy).getOwnPropertyDescriptor: no descriptor for existent property of non-extensible object");
			}
			result = 0;
		}
		else {
			mask = fxDescriptorToSlot(the, slot);
			if (!(mask & XS_DONT_DELETE_FLAG)) {
				mask |= XS_DONT_DELETE_FLAG;
				slot->flag |= XS_DONT_DELETE_FLAG;
			}
			if (!(mask & XS_DONT_ENUM_FLAG)) {
				mask |= XS_DONT_ENUM_FLAG;
				slot->flag |= XS_DONT_ENUM_FLAG;
			}
			if (!(mask & (XS_GETTER_FLAG | XS_SETTER_FLAG))) {
				if (!(mask & XS_DONT_SET_FLAG)) {
					mask |= XS_DONT_SET_FLAG;
					slot->flag |= XS_DONT_SET_FLAG;
				}
				if (slot->kind == XS_UNINITIALIZED_KIND)
					slot->kind = XS_UNDEFINED_KIND;
			}
			if (fxGetInstanceOwnProperty(the, proxy->value.proxy.target, id, index, the->stack)) {
				if (fxIsPropertyCompatible(the, the->stack, slot, mask)) {
					if ((mask & XS_DONT_DELETE_FLAG) && (slot->flag & XS_DONT_DELETE_FLAG)) {
						if (!(the->stack->flag & XS_DONT_DELETE_FLAG))
							mxTypeError("(proxy).getOwnPropertyDescriptor: non-configurable descriptor for configurable property");
					}
				}
				else
					mxTypeError("(proxy).getOwnPropertyDescriptor: incompatible descriptor for existent property");
			}
			else if (fxIsInstanceExtensible(the, proxy->value.proxy.target)) {
				if ((mask & XS_DONT_DELETE_FLAG) && (slot->flag & XS_DONT_DELETE_FLAG))
					mxTypeError("(proxy).getOwnPropertyDescriptor: non-configurable descriptor for non-existent property");
			}
			else
				mxTypeError("(proxy).getOwnPropertyDescriptor: descriptor for non-existent property of non-extensible object");
			result = 1;
		}
		mxPop();
	}
	else
		result = fxGetInstanceOwnProperty(the, proxy->value.proxy.target, id, index, slot);
	return result;
}

txBoolean fxGetProxyProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* slot)
{
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _get);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		mxPushSlot(receiver);
		/* ARGC */
		mxPushInteger(3);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		mxPullSlot(slot);
		mxPushUndefined();
		if (fxGetInstanceOwnProperty(the, proxy->value.proxy.target, id, index, the->stack)) {
			txSlot* property = the->stack;
			if (property->flag & XS_DONT_DELETE_FLAG) {
				if (property->kind == XS_ACCESSOR_KIND) {
					if ((property->value.accessor.getter == C_NULL) && (slot->kind != XS_UNDEFINED_KIND))
						mxTypeError("(proxy).get: different getter for non-configurable property");
				}
				else {
					if ((property->flag & XS_DONT_SET_FLAG) && (!fxIsSameValue(the, property, slot)))
						mxTypeError("(proxy).get: different value for non-configurable, non-writable property");
				}
			}
		}
		mxPop();
		return 1;
	}
	return fxGetInstanceProperty(the, proxy->value.proxy.target, id, index, receiver, slot);
}

void fxGetProxyPropertyAux(txMachine* the)
{
	txSlot* proxy = mxThis->value.reference->next;
	txID id = proxy->next->value.at.id;
	txIndex index = proxy->next->value.at.index;
	fxGetProxyProperty(the, mxThis->value.reference, id, index, mxThis, mxResult);
}

txBoolean fxGetProxyPrototype(txMachine* the, txSlot* instance, txSlot* slot)
{
	txBoolean result;
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
		mxPullSlot(slot);
		if ((slot->kind == XS_NULL_KIND) ||  (slot->kind == XS_REFERENCE_KIND)) {
			if (!fxIsInstanceExtensible(the, proxy->value.proxy.target)) {
				mxPushUndefined();
				fxGetInstancePrototype(the, proxy->value.proxy.target, the->stack);
				if (!fxIsSameValue(the, slot, the->stack))
					mxTypeError("(proxy).getPrototypeOf: different prototype for non-extensible object");
				mxPop();
			}
		}
		else
			mxTypeError("(proxy).getPrototypeOf: neither object nor null");
		result = (slot->kind == XS_NULL_KIND) ? 0 : 1;
	}
	else
		result = fxGetInstancePrototype(the, proxy->value.proxy.target, slot);
	return result;
}

txBoolean fxHasProxyProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	txBoolean result;
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _has);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		/* ARGC */
		mxPushInteger(2);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (!result) {
			mxPushUndefined();
			if (fxGetInstanceOwnProperty(the, proxy->value.proxy.target, id, index, the->stack)) {
				if (the->stack->flag & XS_DONT_DELETE_FLAG)
					mxTypeError("(proxy).has: false for non-configurable property");
				 if (!fxIsInstanceExtensible(the, proxy->value.proxy.target)) 
					mxTypeError("(proxy).has: false for property of not extensible object");
			}
			mxPop();
		}
	}
	else
		result = fxHasInstanceProperty(the, proxy->value.proxy.target, id, index);
	return result;
}

txBoolean fxIsProxyExtensible(txMachine* the, txSlot* instance)
{
	txBoolean result;
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
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (fxIsInstanceExtensible(the, proxy->value.proxy.target)) {
			if (!result)
				mxTypeError("(proxy).isExtensible: false for extensible object");
		}
		else {
			if (result)
				mxTypeError("(proxy).isExtensible: true for non-extensible object");
		}
	}
	else
		result = fxIsInstanceExtensible(the, proxy->value.proxy.target);
	return result;
}

txBoolean fxPreventProxyExtensions(txMachine* the, txSlot* instance)
{
	txBoolean result;
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
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (result) {
			if (fxIsInstanceExtensible(the, proxy->value.proxy.target))
				mxTypeError("(proxy).preventExtensions: true for extensible object");
		}
	}
	else
		result = fxPreventInstanceExtensions(the, proxy->value.proxy.target);
	return result;
}

txBoolean fxSetProxyProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txSlot* receiver)
{
	txBoolean result;
	txSlot* proxy = instance->next;
	txSlot* function = fxCheckProxyFunction(the, proxy, _set);
	if (function) {
		mxPushReference(proxy->value.proxy.target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		mxPushSlot(slot);
		mxPushSlot(receiver);
		/* ARGC */
		mxPushInteger(4);
		/* THIS */
		mxPushReference(proxy->value.proxy.handler);
		/* FUNCTION */
		mxPushSlot(function);
		fxCall(the);
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (result) {
			mxPushUndefined();
			if (fxGetInstanceOwnProperty(the, proxy->value.proxy.target, id, index, the->stack)) {
				txSlot* property = the->stack;
				if (property->flag & XS_DONT_DELETE_FLAG) {
					if (property->kind == XS_ACCESSOR_KIND) {
						if (property->value.accessor.setter == C_NULL)
							mxTypeError("(proxy).set: true for non-configurable property with different setter");
					}
					else {
						if ((property->flag & XS_DONT_SET_FLAG) && (!fxIsSameValue(the, property, slot)))
							mxTypeError("(proxy).set: true for non-configurable, non-writable property with different value");
					}
				}
			}
			mxPop();
		}
	}
	else
		result = fxSetInstanceProperty(the, proxy->value.proxy.target, id, index, slot, receiver);
	return result;
}

void fxSetProxyPropertyAux(txMachine* the)
{
	txSlot* proxy = mxThis->value.reference->next;
	txID id = proxy->next->value.at.id;
	txIndex index = proxy->next->value.at.index;
	fxSetProxyProperty(the, mxThis->value.reference, id, index, mxArgv(0), mxThis);
}

txBoolean fxSetProxyPrototype(txMachine* the, txSlot* instance, txSlot* prototype)
{
	txBoolean result;
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
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (result) {
			if (!fxIsInstanceExtensible(the, proxy->value.proxy.target)) {
				mxPushUndefined();
				fxGetInstancePrototype(the, proxy->value.proxy.target, the->stack);
				if (!fxIsSameValue(the, prototype, the->stack))
					mxTypeError("(proxy).setPrototypeOf: true for non-extensible object with different prototype");
				mxPop();
			}
		}
	}
	else
		result = fxSetInstancePrototype(the, proxy->value.proxy.target, prototype);
	return result;
}

void fxStepProxyProperty(txMachine* the, txSlot* instance, txFlag flag, txStep step, txSlot* context, txID id, txIndex index, txSlot* property)
{
	txSlot* stack = the->stack;
	mxPushUndefined();
	property = the->stack;
	if (flag & (XS_EACH_ENUMERABLE_FLAG | XS_STEP_GET_OWN_FLAG)) {
		if (!fxGetProxyOwnProperty(the, instance, id, index, property))
			goto bail;
		if ((flag & XS_EACH_ENUMERABLE_FLAG) && (property->flag & XS_DONT_ENUM_FLAG))
			goto bail;
	}
	if (flag & XS_STEP_GET_FLAG) {
		mxPushReference(instance);
		fxGetAll(the, id, index);
		property = the->stack;
	}
	(*step)(the, context, id, index, property);
	if (flag & XS_STEP_DEFINE_FLAG) {
		fxDescribeProperty(the, property, XS_GET_ONLY);
		property = the->stack;
		fxDefineProxyProperty(the, instance, id, index, property, XS_GET_ONLY);
	}
bail:
	the->stack = stack;
}

void fx_Reflect_apply(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND) || !(mxIsCallable(mxArgv(0)->value.reference)))
		mxTypeError("target is no function");
	if ((mxArgc < 3) || (mxArgv(2)->kind != XS_REFERENCE_KIND))
		mxTypeError("argumentsList is no object");
	fxCallInstance(the, mxArgv(0)->value.reference, mxArgv(1), mxArgv(2));
}

void fx_Reflect_construct(txMachine* the)
{
    txSlot* target;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND) || !(mxIsCallable(mxArgv(0)->value.reference)))
		mxTypeError("target is no function");
	if ((mxArgc < 2) || (mxArgv(1)->kind != XS_REFERENCE_KIND))
		mxTypeError("argumentsList is no object");
	if (mxArgc < 3)
		target = mxArgv(0);
	else if ((mxArgv(2)->kind != XS_REFERENCE_KIND) || !mxIsCallable(mxArgv(2)->value.reference))
		mxTypeError("newTarget is no function");
	else
		target = mxArgv(2);
	fxConstructInstance(the, mxArgv(0)->value.reference, mxArgv(1), target);
}

void fx_Reflect_defineProperty(txMachine* the)
{
	txSlot* at;
	txFlag mask;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	if ((mxArgc < 3) || (mxArgv(2)->kind != XS_REFERENCE_KIND))
		mxTypeError("invalid descriptor");
	mask = fxDescriptorToSlot(the, mxArgv(2));
	mxResult->value.boolean = fxDefineInstanceProperty(the, mxArgv(0)->value.reference, at->value.at.id, at->value.at.index, mxArgv(2), mask);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Reflect_deleteProperty(txMachine* the)
{
	txSlot* at;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	mxResult->value.boolean = fxDeleteInstanceProperty(the, mxArgv(0)->value.reference, at->value.at.id, at->value.at.index);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Reflect_enumerate(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	fxEnumerateInstance(the, mxArgv(0)->value.reference);
}

void fx_Reflect_get(txMachine* the)
{
	txSlot* at;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	fxGetInstanceProperty(the, 	mxArgv(0)->value.reference, at->value.at.id, at->value.at.index, mxArgc < 3 ? mxArgv(0) : mxArgv(2), mxResult);
}

void fx_Reflect_getOwnPropertyDescriptor(txMachine* the)
{
	txSlot* at;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	mxPushUndefined();
	if (fxGetInstanceOwnProperty(the, mxArgv(0)->value.reference, at->value.at.id, at->value.at.index, the->stack)) {
		fxDescribeProperty(the, the->stack, XS_GET_ONLY);
		mxPullSlot(mxResult);
	}
	mxPop();
}

void fx_Reflect_getPrototypeOf(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	fxGetInstancePrototype(the, mxArgv(0)->value.reference, mxResult);
}

void fx_Reflect_has(txMachine* the)
{
	txSlot* at;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	mxResult->value.boolean = fxHasInstanceProperty(the, mxArgv(0)->value.reference, at->value.at.id, at->value.at.index);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Reflect_isExtensible(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	mxResult->value.boolean = fxIsInstanceExtensible(the, mxArgv(0)->value.reference);
	mxResult->kind = XS_BOOLEAN_KIND;
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
	mxResult->value.boolean = fxPreventInstanceExtensions(the, mxArgv(0)->value.reference);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Reflect_set(txMachine* the)
{
	txSlot* at;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	if (mxArgc < 3)
		mxTypeError("no value");
	mxResult->value.boolean = fxSetInstanceProperty(the, mxArgv(0)->value.reference, at->value.at.id, at->value.at.index, mxArgv(2), mxArgc < 4 ? mxArgv(0) : mxArgv(3));
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Reflect_setPrototypeOf(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if ((mxArgc < 2) || ((mxArgv(1)->kind != XS_NULL_KIND) && (mxArgv(1)->kind != XS_REFERENCE_KIND)))
		mxTypeError("invalid prototype");
	mxResult->value.boolean = fxSetInstancePrototype(the, mxArgv(0)->value.reference, mxArgv(1));
	mxResult->kind = XS_BOOLEAN_KIND;
}

txSlot* fxGetHostProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	txSlot* slot = mxHookInstance.value.reference;
	slot->next->value.at.id = id;
	slot->next->value.at.index = index;
	return slot->next->next;
}

void fx_Hook_get(txMachine* the)
{
	txSlot* slot = mxHookInstance.value.reference;
	txID id = slot->next->value.at.id;
	txIndex index = slot->next->value.at.index;
	if (id) {
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
	}
	else
		mxPushUnsigned(index);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxCallID(the, mxID(_peek));
	mxPullSlot(mxResult);
}
