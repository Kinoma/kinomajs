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

txSlot* fxNewInstance(txMachine* the)
{
	txSlot* instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = C_NULL;
	mxPushReference(instance);
	return instance;
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
		mxPullSlot(theSlot);
		break;
	case XS_INTEGER_KIND:
		mxPush(mxNumberPrototype);
		anInstance = fxNewNumberInstance(the);
		anInstance->next->value.number = theSlot->value.integer;
		mxPullSlot(theSlot);
		break;
	case XS_NUMBER_KIND:
		mxPush(mxNumberPrototype);
		anInstance = fxNewNumberInstance(the);
		anInstance->next->value.number = theSlot->value.number;
		mxPullSlot(theSlot);
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		mxPush(mxStringPrototype);
		anInstance = fxNewStringInstance(the);
		anInstance->next->value.string = theSlot->value.string;
		anInstance->next->next->next->value.integer = fxUnicodeLength(theSlot->value.string);
		mxPullSlot(theSlot);
		break;
	case XS_SYMBOL_KIND:
		mxPush(mxSymbolPrototype);
		anInstance = fxNewSymbolInstance(the);
		anInstance->next->value.ID = theSlot->value.ID;
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

void fx_species_get(txMachine* the)
{
	*mxResult = *mxThis;
}




















