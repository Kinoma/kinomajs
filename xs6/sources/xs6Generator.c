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

static txSlot* fxCheckGeneratorInstance(txMachine* the, txSlot* slot);

static void fx_Generator(txMachine* the);
static void fx_Generator_prototype_aux(txMachine* the, txFlag status);
static void fx_Generator_prototype_next(txMachine* the);
static void fx_Generator_prototype_return(txMachine* the);
static void fx_Generator_prototype_throw(txMachine* the);
static void fx_GeneratorFunction(txMachine* the);

void fxBuildGenerator(txMachine* the)
{
    static const txHostFunctionBuilder gx_Generator_prototype_builders[] = {
		{ fx_Generator_prototype_next, 1, _next },
		{ fx_Generator_prototype_return, 1, _return },
		{ fx_Generator_prototype_throw, 1, _throw },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	for (builder = gx_Generator_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Generator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxGeneratorPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructor(the, fx_Generator, 1, mxID(_Generator)));
	slot = fxNextStringProperty(the, slot, "GeneratorFunction", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxGeneratorFunctionPrototype = *the->stack;
	slot = fxNewHostConstructor(the, fx_GeneratorFunction, 1, mxID(_GeneratorFunction));
	the->stack++;
	
	slot = fxGetProperty(the, mxGeneratorPrototype.value.reference, mxID(_constructor));
	slot->flag |= XS_DONT_SET_FLAG;
}

txSlot* fxCheckGeneratorInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if ((slot->flag & XS_VALUE_FLAG) && (slot->next->kind == XS_STACK_KIND))
			return slot;
	}
	mxTypeError("this is no Generator instance");
	return C_NULL;
}

txSlot* fxNewGeneratorInstance(txMachine* the)
{
	txSlot* prototype;
	txSlot* instance;
	txSlot* property;
	txSlot* result;
	txSlot* slot;

	prototype = the->stack->value.reference;

	instance = fxNewSlot(the);
	instance->flag = XS_VALUE_FLAG;
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = prototype;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;

	property = instance->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_STACK_KIND;
	property->ID = XS_NO_ID;
	if (prototype->flag == XS_VALUE_FLAG) {
		slot = prototype->next;
		property->value.array.length = slot->value.array.length;
		property->value.array.address = slot->value.array.address;
	}
	else {
		property->value.array.length = 0;
		property->value.array.address = C_NULL;
    }
	result = fxNewInstance(the);
	result->value.instance.prototype = mxObjectPrototype.value.reference;
	slot = result->next = fxNewSlot(the);
	slot->flag = XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
	slot->kind = XS_UNDEFINED_KIND;
	slot->ID = mxID(_value);
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
	slot->kind = XS_BOOLEAN_KIND;
	slot->ID = mxID(_done);
	slot->value.boolean = 0;
    
    property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_REFERENCE_KIND;
	property->value.reference = the->stack->value.reference;
	
    property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_INTEGER_KIND;
	property->value.integer = -1;
	the->stack++;
	
	return instance;
}

void fx_Generator(txMachine* the)
{
	if (mxTarget->kind != XS_UNDEFINED_KIND)
		mxTypeError("new Generator");
}

void fx_Generator_prototype_aux(txMachine* the, txFlag status)
{
	txSlot* generator = fxCheckGeneratorInstance(the, mxThis);
	txSlot* slot = generator->next->next;
	txSlot* result = slot->value.reference;
	txSlot* state = slot->next;
	mxResult->kind = XS_REFERENCE_KIND;
	mxResult->value.reference = result;
	if ((status != XS_NO_STATUS) && (state->value.integer < 0)) {
		result->next->next->value.boolean = 1;
		state->value.integer = 0;
	}
	if (state->value.integer > 0)
		mxTypeError("generator is running");
	if (result->next->next->value.boolean) {
		if (status == XS_NO_STATUS)
			result->next->kind = XS_UNDEFINED_KIND;
		else if (status == XS_RETURN_STATUS) {
			result->next->kind = mxArgv(0)->kind;
			result->next->value = mxArgv(0)->value;
		}
		else {
			mxException = *mxArgv(0);
			fxJump(the);
		}
		return;
	}
	{
		mxTry(the) {
			if (mxArgc > 0) {
				the->scratch.kind = mxArgv(0)->kind;
				the->scratch.value = mxArgv(0)->value;
			}
			else
				the->scratch.kind = XS_UNDEFINED_KIND;
			the->status = status;
			state->value.integer = 1;
			fxRunID(the, generator, XS_NO_ID);
			state->value.integer = 0;
		}
		mxCatch(the) {
			state->value.integer = 0;
			fxJump(the);
		}
	}
	result->next->kind = the->stack->kind;
	result->next->value = the->stack->value;
	the->stack++;
}

void fx_Generator_prototype_next(txMachine* the)
{
	fx_Generator_prototype_aux(the, XS_NO_STATUS);
}

void fx_Generator_prototype_return(txMachine* the)
{
	fx_Generator_prototype_aux(the, XS_RETURN_STATUS);
}

void fx_Generator_prototype_throw(txMachine* the)
{
	fx_Generator_prototype_aux(the, XS_THROW_STATUS);
}

txSlot* fxNewGeneratorFunctionInstance(txMachine* the, txID name)
{
	txSlot* instance;
	txSlot* property;

	instance = fxNewFunctionInstance(the, name);
	property = fxLastProperty(the, instance);
	mxPush(mxGeneratorPrototype);
	fxNewInstanceOf(the);
	fxNextSlotProperty(the, property, the->stack, mxID(_prototype), XS_GET_ONLY);
	the->stack++;
	
	return instance;
}

void fx_GeneratorFunction(txMachine* the)
{	
	txInteger c, i;
	txStringStream stream;
	
	c = mxArgc;
	i = 0;
	mxPushStringC("(function* anonymous(");
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






