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

static void fx_Function(txMachine* the);
static void fx_Function_prototype_get_length(txMachine* the);
static void fx_Function_prototype_set_length(txMachine* the);
static void fx_Function_prototype_get_name(txMachine* the);
static void fx_Function_prototype_set_name(txMachine* the);
static void fx_Function_prototype_get_prototype(txMachine* the);
static void fx_Function_prototype_set_prototype(txMachine* the);
static void fx_Function_prototype_apply(txMachine* the);
static void fx_Function_prototype_bind(txMachine* the);
static void fx_Function_prototype_bound(txMachine* the);
static void fx_Function_prototype_call(txMachine* the);

void fxBuildFunction(txMachine* the)
{
    static const txHostFunctionBuilder gx_Function_prototype_builders[] = {
		{ fx_Function_prototype_apply, 2, _apply },
		{ fx_Function_prototype_bind, 1, _bind },
		{ fx_Function_prototype_call, 1, _call },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	mxPush(mxFunctionPrototype);
	slot = fxLastProperty(the, the->stack->value.reference);
	slot = fxNextHostAccessorProperty(the, slot, fx_Function_prototype_get_length, fx_Function_prototype_set_length, mxID(_length), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_Function_prototype_get_name, fx_Function_prototype_set_name, mxID(_name), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_Function_prototype_get_prototype, fx_Function_prototype_set_prototype, mxID(_prototype), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	for (builder = gx_Function_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Function", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostConstructorGlobal(the, fx_Function, 1, mxID(_Function), XS_GET_ONLY);
	the->stack++;
}

txSlot* fxCheckFunctionInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if ((slot->next->kind == XS_CALLBACK_KIND) || (slot->next->kind == XS_CODE_KIND) || (slot->next->kind == XS_CODE_X_KIND))
			return slot;
	}
	mxTypeError("this is no Function instance");
	return C_NULL;
}

txSlot* fxCreateInstance(txMachine* the, txSlot* slot)
{
	txSlot* prototype = mxFunctionInstancePrototype(slot);
	if (prototype->kind == XS_NULL_KIND) {
		if (prototype->flag & XS_DONT_SET_FLAG)
			mxTypeError("new.target: no constructor");
		if (slot->flag & XS_SHARED_FLAG)
			mxTypeError("new.target: shared");
		fxDefaultFunctionPrototype(the, slot, prototype);
	}
	mxPushSlot(prototype);
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
	
	/* PROTOTYPE */
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	property->kind = XS_NULL_KIND;

	/* HOME */
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_NULL_KIND;

	/* INFO */
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_INFO_KIND;
	property->value.info.length = -1;
	property->value.info.name = name;
#ifdef mxProfile
	property->value.info.profileID = the->profileID;
	the->profileID++;
#endif

	/* MODULE */
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	if (the->frame && (mxFunction->kind == XS_REFERENCE_KIND)) {
		txSlot* slot = mxFunctionInstanceModule(mxFunction->value.reference);
		property->kind = slot->kind;
		property->value = slot->value;
	}
	else
		property->kind = XS_NULL_KIND;

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
		txSlot* from = fxGetInstance(the, the->stack++);
		txSlot* to = fxGetInstance(the, mxThis);
        to->next->value.code = from->next->value.code;
        to = mxFunctionInstanceInfo(to);
        from = mxFunctionInstanceInfo(from);
        to->value = from->value;
	}
}

void fx_Function_prototype_get_length(txMachine* the)
{	
	txSlot* instance = fxCheckFunctionInstance(the, mxThis);
	txSlot* code = mxFunctionInstanceCode(instance);
	if ((code->kind == XS_CODE_KIND) || (code->kind == XS_CODE_X_KIND))
		mxResult->value.integer = *(code->value.code + 1);
	else
		mxResult->value.integer = mxFunctionInstanceInfo(instance)->value.info.length;
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_Function_prototype_set_length(txMachine* the)
{	
	if (the->frame->next->flag & XS_STRICT_FLAG)
		mxTypeError("set \"length\": const");
}

void fx_Function_prototype_get_name(txMachine* the)
{
	txSlot* instance = fxCheckFunctionInstance(the, mxThis);
	txSlot* info = mxFunctionInstanceInfo(instance);
	txSlot* key = fxGetKey(the, info->value.info.name);
	if (key) {
		if (key->kind == XS_KEY_KIND) {
			mxResult->kind = XS_STRING_KIND;
			mxResult->value.string = key->value.key.string;
		}
		else if (key->kind == XS_KEY_X_KIND) {
			mxResult->kind = XS_STRING_X_KIND;
			mxResult->value.string = key->value.key.string;
		} 
		else {
			fxCopyStringC(the, mxResult, "[");
			fxConcatStringC(the, mxResult, key->value.string);
			fxConcatStringC(the, mxResult, "]");
		}
		return;
	}
	*mxResult = mxEmptyString;
}

void fx_Function_prototype_set_name(txMachine* the)
{	
	if (the->frame->next->flag & XS_STRICT_FLAG)
		mxTypeError("set \"name\": const");
}

void fx_Function_prototype_get_prototype(txMachine* the)
{	
	txSlot* instance = fxCheckFunctionInstance(the, mxThis);
	txSlot* slot = mxFunctionInstancePrototype(instance);
	if (slot->kind == XS_NULL_KIND) {
		if (slot->flag & XS_DONT_SET_FLAG)
			return;
		if (instance->flag & XS_SHARED_FLAG)
			mxTypeError("get \"prototype\": shared");
		fxDefaultFunctionPrototype(the, instance, slot);
	}
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

void fx_Function_prototype_set_prototype(txMachine* the)
{	
	txSlot* instance = fxCheckFunctionInstance(the, mxThis);
	txSlot* slot = mxFunctionInstancePrototype(instance);
	if (slot->flag & XS_DONT_SET_FLAG) {
		if (the->frame->next->flag & XS_STRICT_FLAG)
			mxTypeError("set \"prototype\": const");
		return;
	}
	if (instance->flag & XS_SHARED_FLAG)
		mxTypeError("set \"prototype\": shared");
	if ((mxArgc == 0) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND)) {
		slot->kind = XS_NULL_KIND;
	}
	else {
		fxToInstance(the, mxArgv(0));
		slot->value = mxArgv(0)->value;
		slot->kind = mxArgv(0)->kind;
	}
}

void fx_Function_prototype_apply(txMachine* the)
{
	txInteger c, i;
	if ((mxArgc < 2) || (mxArgv(1)->kind == XS_UNDEFINED_KIND) || (mxArgv(1)->kind == XS_NULL_KIND))
		c = 0;
	else {
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
	mxPushSlot(mxThis);
	fxCall(the);
	mxPullSlot(mxResult);
}

void fx_Function_prototype_bind(txMachine* the)
{
	txSlot* instance = fxCheckFunctionInstance(the, mxThis);
	txSize length;
	txSlot* slot;
	txID id;
	txSlot* arguments;
	txSlot* argument;
	txSize c = mxArgc, i;

	mxPushSlot(mxThis);
	fxGetID(the, mxID(_length));
	length = fxToInteger(the, the->stack++);
	if (c > 1)
		length -= c - 1;
	if (length < 0)
		length = 0;
	
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_name));
	mxPushStringC("bound ");
	fxConcatString(the, the->stack, the->stack + 1);
	slot = fxNewName(the, the->stack);
	id = slot->ID;
		
	mxPushReference(instance->value.instance.prototype);
	instance = fxNewFunctionInstance(the, id);
	mxPullSlot(mxResult);
	
	slot = mxFunctionInstanceCode(instance);
	slot->kind = XS_CALLBACK_KIND;
	slot->value.callback.address = fx_Function_prototype_bound;
	slot->value.callback.IDs = (txID*)mxIDs.value.code;
	
	slot = mxFunctionInstanceInfo(instance);
	slot->value.info.length = (txID)length;
	
	slot = fxLastProperty(the, instance);
	slot = fxNextSlotProperty(the, slot, mxThis, mxID(_boundFunction), XS_GET_ONLY);
	if (c > 0)
		slot = fxNextSlotProperty(the, slot, mxArgv(0), mxID(_boundThis), XS_GET_ONLY);
	else
		slot = fxNextUndefinedProperty(the, slot, mxID(_boundThis), XS_GET_ONLY);
	
	mxPush(mxArrayPrototype);
	arguments = fxNewArrayInstance(the);
	argument = arguments->next;
	for (i = 1; i < c; i++) {
		argument->next = fxNewSlot(the);
		argument = argument->next;
		argument->kind = mxArgv(i)->kind;
		argument->value = mxArgv(i)->value;
	}
	arguments->next->value.array.length = mxArgc - 1;
	fxCacheArray(the, arguments);
	slot = fxNextSlotProperty(the, slot, the->stack, mxID(_boundArguments), XS_GET_ONLY);
	the->stack++;
}

void fx_Function_prototype_bound(txMachine* the)
{
	txSlot* boundArguments;
	txInteger c, i;
	txSlot* argument;
	
	mxPush(*mxFunction);
	fxGetID(the, mxID(_boundArguments)); 
	boundArguments = fxGetInstance(the, the->stack);
	the->stack++;
	c = boundArguments->next->value.array.length;
	argument = boundArguments->next->value.array.address;
	for (i = 0; i < c; i++) {
		mxPushSlot(argument);
		argument++;
	}
	for (i = 0; i < mxArgc; i++) {
		mxPushSlot(mxArgv(i));
	}
	/* ARGC */
	mxPushInteger(c + mxArgc);
	/* THIS */
	mxPushSlot(mxFunction);
	fxGetID(the, mxID(_boundThis));
	/* FUNCTION */
	mxPushSlot(mxFunction);
	fxGetID(the, mxID(_boundFunction));
	fxCall(the);
	mxPullSlot(mxResult);
}

void fx_Function_prototype_call(txMachine* the)
{	
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
	mxPushSlot(mxThis);
	fxCall(the);
	mxPullSlot(mxResult);
}

