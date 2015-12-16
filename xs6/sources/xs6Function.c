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

static txBoolean fxIsFunction(txMachine* the, txSlot* instance);

static void fx_Function(txMachine* the);
static void fx_Function_prototype_apply(txMachine* the);
static void fx_Function_prototype_arguments(txMachine* the);
static void fx_Function_prototype_bind(txMachine* the);
static void fx_Function_prototype_bound(txMachine* the);
static void fx_Function_prototype_call(txMachine* the);
static void fx_Function_prototype_caller(txMachine* the);
static void fx_Function_prototype_hasInstance(txMachine* the);
static void fx_Function_prototype_toString(txMachine* the);

void fxBuildFunction(txMachine* the)
{
    static const txHostFunctionBuilder gx_Function_prototype_builders[] = {
		{ fx_Function_prototype_apply, 2, _apply },
		{ fx_Function_prototype_bind, 1, _bind },
		{ fx_Function_prototype_call, 1, _call },
		{ fx_Function_prototype_toString, 0, _toString },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	mxPush(mxFunctionPrototype);
	slot = fxLastProperty(the, the->stack->value.reference);
	for (builder = gx_Function_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
    slot = fxNextHostFunctionProperty(the, slot, fx_Function_prototype_hasInstance, 1, mxID(_Symbol_hasInstance), XS_GET_ONLY);
	slot = fxNextHostAccessorProperty(the, slot, fx_Function_prototype_arguments, fx_Function_prototype_arguments, mxID(_arguments), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_Function_prototype_caller, fx_Function_prototype_caller, mxID(_caller), XS_DONT_ENUM_FLAG);
	slot = fxNewHostConstructorGlobal(the, fx_Function, 1, mxID(_Function), XS_DONT_ENUM_FLAG);
	the->stack++;
}

txSlot* fxCheckFunctionInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if (fxIsFunction(the, slot))
			return slot;
	}
	mxTypeError("this is no Function instance");
	return C_NULL;
}

txSlot* fxCreateInstance(txMachine* the, txSlot* slot)
{
	txSlot* prototype;
	mxPushUndefined();
	prototype = the->stack;
again:
	fxBeginHost(the);
	mxPushReference(slot);
	fxGetID(the, mxID(_prototype));
	*prototype = *the->stack;
	fxEndHost(the);
	if (the->stack->kind != XS_REFERENCE_KIND) {
		txSlot* property = fxGetOwnProperty(the, slot, mxID(_boundFunction));
		if (property) {
			slot = property->value.reference;
			goto again;
		}
		mxTypeError("no prototype");
	}
	fxNewInstanceOf(the);
	return the->stack->value.reference;
}

txBoolean fxIsBaseFunctionInstance(txMachine* the, txSlot* slot)
{
	slot = slot->next;
	if (XS_CALLBACK_KIND == slot->kind)
		return 1;
	if (((XS_CODE_KIND == slot->kind) || (XS_CODE_X_KIND == slot->kind)) && (XS_CODE_BEGIN_STRICT_DERIVED != *(slot->value.code)))
		return 1;
	return 0;
}

txBoolean fxIsFunction(txMachine* the, txSlot* instance) 
{
again:
	if (instance) {
		if (instance->flag & XS_VALUE_FLAG) {
			txSlot* exotic = instance->next;
			if (((exotic->kind == XS_CALLBACK_KIND) || (exotic->kind == XS_CODE_KIND) || (exotic->kind == XS_CODE_X_KIND)))
				return 1;
#if mxProxy
			if (exotic->kind == XS_PROXY_KIND) {
				instance = exotic->value.proxy.target;
				goto again;
			}
#endif
		}
	}
	return 0;
}

txSlot* fxNewFunctionInstance(txMachine* the, txID name)
{
	txSlot* instance;
	txSlot* property;

	instance = fxNewObjectInstance(the);
	instance->flag |= XS_VALUE_FLAG;
	
	/* CODE */
	property = instance->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = mxEmptyCode.kind;
	property->value.code = mxEmptyCode.value.code;

	/* CLOSURE */
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_NULL_KIND;

	/* HOME */
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_NULL_KIND;

	/* MODULE */
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	if (the->frame && (mxFunction->kind == XS_REFERENCE_KIND) && (mxIsFunction(mxFunction->value.reference))) {
		txSlot* slot = mxFunctionInstanceModule(mxFunction->value.reference);
		property->kind = slot->kind;
		property->value = slot->value;
	}
	else
		property->kind = XS_NULL_KIND;

#ifdef mxProfile
	/* PROFILE */
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_INTEGER_KIND;
	property->value.integer = the->profileID;
	the->profileID++;
#endif
		
	/* LENGTH */
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->ID = mxID(_length);
	property->kind = XS_INTEGER_KIND;
	property->value.integer = 0;
		
	/* NAME */
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->ID = mxID(_name);
	property->kind = mxEmptyString.kind;
	property->value = mxEmptyString.value;
	if (name != XS_NO_ID)
		fxRenameFunction(the, property, name, "", C_NULL);

	return instance;
}

void fxDefaultFunctionPrototype(txMachine* the, txSlot* function, txSlot* prototype)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = mxObjectPrototype.value.reference;
    prototype->kind = XS_REFERENCE_KIND;
    prototype->value.reference = instance;
	property = instance->next = fxNewSlot(the);
	property->flag = XS_DONT_ENUM_FLAG;
	property->ID = mxID(_constructor);
	property->kind = XS_REFERENCE_KIND;
	property->value.reference = function;
}

void fxRenameFunction(txMachine* the, txSlot* slot, txInteger id, txString former, txString prefix)
{
	txSlot* key;
	if (slot && ((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND))) {
		if (!former || c_strcmp(slot->value.string, former))
			return;
	}
	key = fxGetKey(the, (txID)id);
	if (key) {
		if (key->kind == XS_KEY_KIND) {
			slot->kind = XS_STRING_KIND;
			slot->value.string = key->value.key.string;
		}
		else if (key->kind == XS_KEY_X_KIND) {
			slot->kind = XS_STRING_X_KIND;
			slot->value.string = key->value.key.string;
		}
		else if ((key->kind == XS_STRING_KIND) || (key->kind == XS_STRING_X_KIND)) {
			slot->kind = key->kind;
			slot->value.string = key->value.string;
			fxAdornStringC(the, "[", slot, "]");
		}
		else {
			slot->kind = mxEmptyString.kind;
			slot->value = mxEmptyString.value;
		}
	}
	else {
		slot->kind = mxEmptyString.kind;
		slot->value = mxEmptyString.value;
	}
	if (prefix) 
		fxAdornStringC(the, prefix, slot, C_NULL);
}

void fx_Function(txMachine* the)
{	
	txInteger c, i;
	txStringStream stream;
	
	c = mxArgc;
	i = 0;
	mxPushStringC("(function anonymous(");
	while (c > 1) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
		if (c > 2)
			fxConcatStringC(the, the->stack, ", ");
		c--;
		i++;
	}
	fxConcatStringC(the, the->stack, "){");
	if (c > 0) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
	}
	fxConcatStringC(the, the->stack, "})");
	stream.slot = the->stack;
	stream.offset = 0;
	stream.size = c_strlen(the->stack->value.string);
	fxRunScript(the, fxParseScript(the, &stream, fxStringGetter, mxProgramFlag), C_NULL, C_NULL, C_NULL, C_NULL);
	if (mxTarget->kind == XS_UNDEFINED_KIND)
		mxPullSlot(mxResult);
	else {
		txSlot* from = fxGetInstance(the, the->stack);
		txSlot* to = fxGetInstance(the, mxThis);
		txSlot* slot = to->next;
		to->next = from->next;
		from->next = slot;
		the->stack++;
	}
}

void fx_Function_prototype_apply(txMachine* the)
{
	txSlot* instance = fxCheckFunctionInstance(the, mxThis);
	txInteger c, i;
	if ((mxArgc < 2) || (mxArgv(1)->kind == XS_UNDEFINED_KIND) || (mxArgv(1)->kind == XS_NULL_KIND))
		c = 0;
	else {
		if (mxArgv(1)->kind != XS_REFERENCE_KIND)
			mxTypeError("argArray is no object");
		fxToInstance(the, mxArgv(1));
		mxPushSlot(mxArgv(1));
		fxGetID(the, mxID(_length));
		c = fxToInteger(the, the->stack);
		the->stack++;
		for (i = 0; i < c; i++) {
			mxPushSlot(mxArgv(1));
			fxGetID(the, (txID)i);
		}
	}
	/* ARGC */
	mxPushInteger(c);
	/* THIS */
	if (mxArgc < 1)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	/* FUNCTION */
	mxPushReference(instance);
	fxCall(the);
	mxPullSlot(mxResult);
}

void fx_Function_prototype_arguments(txMachine* the)
{
	if ((mxThis->value.reference == mxFunctionPrototype.value.reference) || (the->frame->next->flag & XS_STRICT_FLAG))
		mxTypeError("no arguments (strict mode)");
}

void fx_Function_prototype_bind(txMachine* the)
{
	txSlot* instance = fxCheckFunctionInstance(the, mxThis);
	txSize length;
	txSlot* slot;
	txSlot* arguments;
	txSlot* argument;
	txSize c = mxArgc, i;

	if (fxHasOwnProperty(the, instance, mxID(_length))) {
		mxPushSlot(mxThis);
		fxGetID(the, mxID(_length));
		length = fxToInteger(the, the->stack++);
		if (c > 1)
			length -= c - 1;
		if (length < 0)
			length = 0;
		mxPop();
	}
	else
		length = 0;
    mxPushReference(instance->value.instance.prototype);
    instance = fxNewFunctionInstance(the, XS_NO_ID);
    mxPullSlot(mxResult);
	slot = mxFunctionInstanceLength(instance);
	slot->value.integer = length;
		
	slot = slot->next;
	mxPushStringC("bound ");
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_name));
	if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND))
		fxConcatString(the, the->stack + 1, the->stack);
	mxPop();
	mxPullSlot(slot);
	
	slot = mxFunctionInstanceCode(instance);
	slot->kind = XS_CALLBACK_KIND;
	slot->value.callback.address = fx_Function_prototype_bound;
	slot->value.callback.IDs = (txID*)mxIDs.value.code;
	
	slot = fxLastProperty(the, instance);
	slot = fxNextSlotProperty(the, slot, mxThis, mxID(_boundFunction), XS_GET_ONLY);
	if (c > 0)
		slot = fxNextSlotProperty(the, slot, mxArgv(0), mxID(_boundThis), XS_GET_ONLY);
	else
		slot = fxNextUndefinedProperty(the, slot, mxID(_boundThis), XS_GET_ONLY);
	
	if (c > 1) {
		mxPush(mxArrayPrototype);
		arguments = fxNewArrayInstance(the);
		argument = arguments->next;
		for (i = 1; i < c; i++) {
			argument->next = fxNewSlot(the);
			argument = argument->next;
			argument->kind = mxArgv(i)->kind;
			argument->value = mxArgv(i)->value;
		}
		arguments->next->value.array.length = c - 1;
		fxCacheArray(the, arguments);
		slot = fxNextSlotProperty(the, slot, the->stack, mxID(_boundArguments), XS_GET_ONLY);
		mxPop();
	}
	else
		slot = fxNextUndefinedProperty(the, slot, mxID(_boundThis), XS_GET_ONLY);
}

void fx_Function_prototype_bound(txMachine* the)
{
	txSlot* boundArguments;
	txInteger c, i;
	txSlot* argument;
	txSlot* slot;
	
	mxPush(*mxFunction);
	fxGetID(the, mxID(_boundArguments));
	if (the->stack->kind == XS_REFERENCE_KIND) {
		boundArguments = fxGetInstance(the, the->stack);
		mxPop();
		c = boundArguments->next->value.array.length;
		argument = boundArguments->next->value.array.address;
		for (i = 0; i < c; i++) {
			mxPushSlot(argument);
			argument++;
		}
	}
	else {
		mxPop();
		c = 0;
	}
	for (i = 0; i < mxArgc; i++)
		mxPushSlot(mxArgv(i));
	/* ARGC */
	mxPushInteger(c + i);
	
	mxPushSlot(mxFunction);
	fxGetID(the, mxID(_boundFunction));
	slot = fxGetInstance(the, the->stack);
	/* THIS */
	if (mxTarget->kind == XS_UNDEFINED_KIND) {
		mxPushSlot(mxFunction);
		fxGetID(the, mxID(_boundThis));
	}
	else
		mxPushSlot(mxThis);
	/* FUNCTION */
	the->scratch = *(the->stack);
	*(the->stack) = *(the->stack + 1);
	*(the->stack + 1) = the->scratch;
	/* TARGET */
	mxPushSlot(mxTarget);
	/* RESULT */
	if (mxTarget->kind == XS_UNDEFINED_KIND)
		mxPushUndefined();
	else
		mxPushSlot(mxThis);
	fxRunID(the, C_NULL, XS_NO_ID);
	mxPullSlot(mxResult);
}

void fx_Function_prototype_call(txMachine* the)
{	
	txSlot* instance = fxCheckFunctionInstance(the, mxThis);
	txInteger c, i;
	c = mxArgc;
	i = 1;
	while (i < c) {
		mxPushSlot(mxArgv(i));
		i++;
	}
	/* ARGC */
	mxPushInteger(i - 1);
	/* THIS */
	if (mxArgc < 1)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	/* FUNCTION */
	mxPushReference(instance);
	fxCall(the);
	mxPullSlot(mxResult);
}

void fx_Function_prototype_caller(txMachine* the)
{
	if ((mxThis->value.reference == mxFunctionPrototype.value.reference) || (the->frame->next->flag & XS_STRICT_FLAG))
		mxTypeError("no caller (strict mode)");
}

void fx_Function_prototype_hasInstance(txMachine* the)
{	
	txSlot* instance;
	txSlot* prototype;
	mxResult->value.boolean = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	if (mxArgc == 0)
		return;
	instance = fxGetInstance(the, mxArgv(0));
	if (!instance)
		return;
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_prototype));
	prototype = fxGetInstance(the, the->stack);
	mxPop();
	if (!prototype) {
		mxPushSlot(mxThis);
		fxGetID(the, mxID(_boundFunction));
		if (mxIsReference(the->stack)) {
			fxGetID(the, mxID(_prototype));
			prototype = fxGetInstance(the, the->stack);
		}
		mxPop();
	}
	if (!prototype)
		mxTypeError("prototype is no object");
	fxGetInstancePrototype(the, instance);
	while (mxResult->kind != XS_NULL_KIND) {
		if (mxResult->value.reference == prototype) {
			mxResult->kind = XS_BOOLEAN_KIND;
			mxResult->value.boolean = 1;
			return;
		}
		fxGetInstancePrototype(the, mxResult->value.reference);
	}
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
}

void fx_Function_prototype_toString(txMachine* the)
{	
	fxCheckFunctionInstance(the, mxThis);
	mxPushStringC("@ \"");
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_name));
	if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND))
		fxConcatString(the, the->stack + 1, the->stack);
	mxPop();
	mxPushStringC("\"");
	fxConcatString(the, the->stack + 1, the->stack);
	mxPop();
	mxPullSlot(mxResult);
}

