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

#include "xsAll.h"

#define mxCheckBoolean(THE_SLOT) \
	if (!mxIsBoolean(THE_SLOT)) \
		mxDebug0(the, XS_TYPE_ERROR, "this is no Boolean")

static void fx_Boolean(txMachine* the);
static void fx_Boolean_toString(txMachine* the);
static void fx_Boolean_valueOf(txMachine* the);

void fxBuildBoolean(txMachine* the)
{
	mxPush(mxGlobal);
	
	mxPush(mxObjectPrototype);
	fxNewBooleanInstance(the);
	fxNewHostFunction(the, fx_Boolean_toString, 0);
	fxQueueID(the, fxID(the, "toLocaleString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Boolean_toString, 0);
	fxQueueID(the, fxID(the, "toString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Boolean_valueOf, 0);
	fxQueueID(the, fxID(the, "valueOf"), XS_DONT_ENUM_FLAG);

	fxAliasInstance(the, the->stack);
	mxBooleanPrototype = *the->stack;
	fxNewHostConstructor(the, fx_Boolean, 1);
	//fxAliasInstance(the, the->stack);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxBooleanPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "Boolean"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	the->stack++;
}

txSlot* fxNewBooleanInstance(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;

	anInstance = fxNewSlot(the);
	anInstance->next = C_NULL;
	anInstance->flag = XS_VALUE_FLAG;
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = C_NULL;
	if (the->stack->kind == XS_ALIAS_KIND)
		anInstance->ID = the->stack->value.alias;
	else
		anInstance->value.instance.prototype = the->stack->value.reference;
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;

	aProperty = anInstance->next = fxNewSlot(the);
	aProperty->next = C_NULL;
	aProperty->ID = XS_NO_ID;
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_BOOLEAN_KIND;
	aProperty->value.boolean = 0;
	
	return anInstance;
}

void fx_Boolean(txMachine* the)
{
	if (mxResult->kind == XS_UNDEFINED_KIND) {
		if (mxArgc > 0)
			*mxResult = *mxArgv(0);
		fxToBoolean(the, mxResult);
	}
	else {
		txSlot* aBoolean = fxGetOwnInstance(the, mxResult);
		mxCheckBoolean(aBoolean);
		if (mxArgc > 0) {
			fxToBoolean(the, mxArgv(0));
			aBoolean->next->value.boolean = mxArgv(0)->value.boolean;
		}	
	}
}

void fx_Boolean_toString(txMachine* the)
{
	txSlot* aBoolean;
	txSlot* aProperty;
	
	aBoolean = fxGetInstance(the, mxThis);
	mxCheckBoolean(aBoolean);
	aProperty = aBoolean->next;
	mxResult->kind = aProperty->kind;
	mxResult->value = aProperty->value;
	fxToString(the, mxResult);
}

void fx_Boolean_valueOf(txMachine* the)
{
	txSlot* aBoolean;
	txSlot* aProperty;
	
	aBoolean = fxGetInstance(the, mxThis);
	mxCheckBoolean(aBoolean);
	aProperty = aBoolean->next;
	mxResult->kind = aProperty->kind;
	mxResult->value = aProperty->value;
}
