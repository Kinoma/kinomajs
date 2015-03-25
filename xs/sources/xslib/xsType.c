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

void fxAliasInstance(txMachine* the, txSlot* theSlot)
{
	the->aliasArray[the->aliasIndex] = theSlot->value.reference;
	theSlot->kind = XS_ALIAS_KIND;
	theSlot->value.alias = the->aliasIndex;
	the->aliasIndex++;
}

txSlot* fxGetInstance(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_REFERENCE_KIND)
		return theSlot->value.reference;
	if (theSlot->kind == XS_ALIAS_KIND)
		return the->aliasArray[theSlot->value.alias];
	return C_NULL;
}

txSlot* fxGetParent(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_INSTANCE_KIND) {
		if (theSlot->ID >= 0)
			return the->aliasArray[theSlot->ID];
		return theSlot->value.instance.prototype;
	}
	return C_NULL;
}

txSlot* fxGetOwnInstance(txMachine* the, txSlot* theSlot)
{
	txSlot* anInstance = C_NULL;
	
	if (theSlot->kind == XS_REFERENCE_KIND) {
		anInstance = theSlot->value.reference;
		if (anInstance->flag & XS_SHARED_FLAG)
			mxDebug0(the, XS_TYPE_ERROR, "shared instance");
	}
	else if (theSlot->kind == XS_ALIAS_KIND) {
		txID anID = theSlot->value.alias;
		anInstance = the->aliasArray[anID];
		if (anInstance->flag & XS_SHARED_FLAG) {
			txSlot* aSlot;
			txSlot** aSlotAddress;
			aSlot = anInstance;
			aSlotAddress = &(the->aliasArray[anID]);
			*aSlotAddress = fxDuplicateSlot(the, aSlot);
			(*aSlotAddress)->flag &= ~XS_SHARED_FLAG;
			aSlot = aSlot->next;
			aSlotAddress = &((*aSlotAddress)->next);
			while (aSlot) {
				*aSlotAddress = fxDuplicateSlot(the, aSlot);
				aSlot = aSlot->next;
				aSlotAddress = &((*aSlotAddress)->next);
			}
			anInstance = the->aliasArray[anID];
		}
	}
	return anInstance;
}

void fxLookupInstance(txMachine* the, txSlot* theSlot)
{
	txSlot* aSlot;
	txID aCount, anIndex;
	txSlot** aSlotAddress;

	fxDebugger(the, C_NULL, 0);
	aSlot = theSlot->value.reference;
	aCount = the->aliasIndex;
	for (anIndex = 0, aSlotAddress = the->aliasArray; anIndex < aCount; anIndex++, aSlotAddress++) {
		if (*aSlotAddress == aSlot) {
			theSlot->kind = XS_ALIAS_KIND;
			theSlot->value.alias = anIndex;
			return;
		}
	}
	fxAliasInstance(the, theSlot);
}

void fxNewInstance(txMachine* the)
{
	txSlot* anInstance;

	mxZeroSlot(--the->stack);
	anInstance = fxNewSlot(the);
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = C_NULL;
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;
}

txSlot* fxToInstance(txMachine* the, txSlot* theSlot)
{
	txSlot* anInstance = C_NULL;
	
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
		mxDebug0(the, XS_REFERENCE_ERROR, "Cannot coerce undefined to instance");
		break;
	case XS_NULL_KIND:
		mxDebug0(the, XS_REFERENCE_ERROR, "Cannot coerce null to instance");
		break;
	case XS_BOOLEAN_KIND:
		mxPush(mxBooleanPrototype);
		anInstance = fxNewBooleanInstance(the);
		anInstance->next->value.boolean = theSlot->value.boolean;
		*theSlot = *(the->stack++);
		break;
	case XS_INTEGER_KIND:
		mxPush(mxNumberPrototype);
		anInstance = fxNewNumberInstance(the);
		anInstance->next->value.number = theSlot->value.integer;
		*theSlot = *(the->stack++);
		break;
	case XS_NUMBER_KIND:
		mxPush(mxNumberPrototype);
		anInstance = fxNewNumberInstance(the);
		anInstance->next->value.number = theSlot->value.number;
		*theSlot = *(the->stack++);
		break;
	case XS_STRING_KIND:
		mxPush(mxStringPrototype);
		anInstance = fxNewStringInstance(the);
		anInstance->next->value.string = theSlot->value.string;
		anInstance->next->next->value.integer = fxUnicodeLength(theSlot->value.string);
		*theSlot = *(the->stack++);
		break;
	case XS_REFERENCE_KIND:
		anInstance = theSlot->value.reference;
		break;
	case XS_ALIAS_KIND:
		anInstance = the->aliasArray[theSlot->value.alias];
		break;
	default:
		mxDebug0(the, XS_TYPE_ERROR, "Cannot coerce to instance");
		break;
	}
	return anInstance;
}

txSlot* fxToOwnInstance(txMachine* the, txSlot* theSlot)
{
	txSlot* anInstance = C_NULL;
	
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
		mxDebug0(the, XS_REFERENCE_ERROR, "Cannot coerce undefined to instance");
		break;
	case XS_NULL_KIND:
		mxDebug0(the, XS_REFERENCE_ERROR, "Cannot coerce null to instance");
		break;
	case XS_BOOLEAN_KIND:
		mxPush(mxBooleanPrototype);
		anInstance = fxNewBooleanInstance(the);
		anInstance->next->value.boolean = theSlot->value.boolean;
		*theSlot = *(the->stack++);
		break;
	case XS_INTEGER_KIND:
		mxPush(mxNumberPrototype);
		anInstance = fxNewNumberInstance(the);
		anInstance->next->value.number = theSlot->value.integer;
		*theSlot = *(the->stack++);
		break;
	case XS_NUMBER_KIND:
		mxPush(mxNumberPrototype);
		anInstance = fxNewNumberInstance(the);
		anInstance->next->value.number = theSlot->value.number;
		*theSlot = *(the->stack++);
		break;
	case XS_STRING_KIND:
		mxPush(mxStringPrototype);
		anInstance = fxNewStringInstance(the);
		anInstance->next->value.string = theSlot->value.string;
		anInstance->next->next->value.integer = fxUnicodeLength(theSlot->value.string);
		*theSlot = *(the->stack++);
		break;
	case XS_REFERENCE_KIND:
	case XS_ALIAS_KIND:
		anInstance = fxGetOwnInstance(the, theSlot);
		break;
	default:
		mxDebug0(the, XS_TYPE_ERROR, "Cannot coerce to instance");
		break;
	}
	return anInstance;
}

void fxToPrimitive(txMachine* the, txSlot* theSlot, txBoolean theHint)
{
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
	case XS_BOOLEAN_KIND:
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
	case XS_STRING_KIND:
		break;
	case XS_REFERENCE_KIND:
	case XS_ALIAS_KIND:
		/* ARGC */
		fxInteger(the, --the->stack, 0);
		/* THIS */
		*(--the->stack) = *theSlot;
		/* FUNCTION */
		mxZeroSlot(--the->stack);
		/* RESULT */
		mxZeroSlot(--the->stack);
		/* FRAME */
		mxZeroSlot(--the->stack);
		the->stack->next = the->frame;
		the->stack->ID = XS_NO_ID;
		the->stack->flag = XS_C_FLAG;
		the->stack->kind = XS_FRAME_KIND;
		the->stack->value.frame.code = the->code;
		the->stack->value.frame.scope = the->scope;
		the->frame = the->stack;
		the->code = C_NULL;
		if (theHint) {
			fxInteger(the, --the->stack, 0);
			*(--the->stack) = *theSlot;
			fxCallID(the, the->toStringID);
			*theSlot = *(the->stack++);
			if (mxIsReference(theSlot)) {
				fxInteger(the, --the->stack, 0);
				*(--the->stack) = *theSlot;
				fxCallID(the, the->valueOfID);
				*theSlot = *(the->stack++);
			}
		}
		else {
			fxInteger(the, --the->stack, 0);
			*(--the->stack) = *theSlot;
			fxCallID(the, the->valueOfID);
			*theSlot = *(the->stack++);
			if (mxIsReference(theSlot)) {
				fxInteger(the, --the->stack, 0);
				*(--the->stack) = *theSlot;
				fxCallID(the, the->toStringID);
				*theSlot = *(the->stack++);
			}
		}
		the->stack = the->frame + 5;
		the->scope = the->frame->value.frame.scope;
		the->code = the->frame->value.frame.code;
		the->frame = the->frame->next;
		if (mxIsReference(theSlot))
			mxDebug0(the, XS_TYPE_ERROR, "Cannot coerce to primitive");
		break;
	default:
		mxDebug0(the, XS_TYPE_ERROR, "Cannot coerce to primitive");
		break;
	}
}




























