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

static void fx_Boolean(txMachine* the);
static void fx_Boolean_prototype_toString(txMachine* the);
static void fx_Boolean_prototype_valueOf(txMachine* the);
static txSlot* fxCheckBoolean(txMachine* the, txSlot* it);

void fxBuildBoolean(txMachine* the)
{
    static const txHostFunctionBuilder gx_Boolean_prototype_builders[] = {
		{ fx_Boolean_prototype_toString, 0, _toLocaleString },
		{ fx_Boolean_prototype_toString, 0, _toString },
		{ fx_Boolean_prototype_valueOf, 0, _valueOf },
 		{ C_NULL, 0, 0 },
   };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewBooleanInstance(the));
	for (builder = gx_Boolean_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	mxBooleanPrototype = *the->stack;
	fxNewHostConstructorGlobal(the, fx_Boolean, 1, mxID(_Boolean), XS_DONT_ENUM_FLAG);
	the->stack++;
}

txSlot* fxNewBooleanInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_VALUE_FLAG;
	property = fxNextBooleanProperty(the, instance, 0, XS_NO_ID, XS_GET_ONLY);
	return instance;
}

void fx_Boolean(txMachine* the)
{
	txSlot* slot = fxCheckBoolean(the, mxThis);
	txBoolean value = 0;
	if (mxArgc > 0)
		value = fxToBoolean(the, mxArgv(0));
	if (slot)	
		slot->value.boolean = value;
	else {
		mxResult->kind = XS_BOOLEAN_KIND;
		mxResult->value.boolean = value;
	}
}

void fx_Boolean_prototype_toString(txMachine* the)
{
	txSlot* slot = fxCheckBoolean(the, mxThis);
	if (!slot) mxTypeError("this is no boolean");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
	fxToString(the, mxResult);
}

void fx_Boolean_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckBoolean(the, mxThis);
	if (!slot) mxTypeError("this is no boolean");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

txSlot* fxCheckBoolean(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if (it->kind == XS_BOOLEAN_KIND)
		result = it;
	else if (it->kind == XS_REFERENCE_KIND) {
		txSlot* instance = it->value.reference;
		it = instance->next;
		if ((instance->flag & XS_VALUE_FLAG) && (it->kind == XS_BOOLEAN_KIND) && (instance != mxBooleanPrototype.value.reference))
			result = it;
	}
	return result;
}
