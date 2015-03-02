/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#include "xsAll.h"

static txSlot* fxGetArrayProperty(txMachine* the, txSlot* theInstance, txIndex theIndex);
static txBoolean fxRemoveArrayProperty(txMachine* the, txSlot* theInstance, txIndex theIndex);
static txSlot* fxSetArrayProperty(txMachine* the, txSlot* theInstance, txIndex theIndex);

static txSlot* fxGetGlobalProperty(txMachine* the, txSlot* theInstance, txID theID, txFlag theMask);
static txBoolean fxRemoveGlobalProperty(txMachine* the, txSlot* theInstance, txID theID, txFlag theMask);
static txSlot* fxSetGlobalProperty(txMachine* the, txSlot* theInstance, txID theID, txFlag* theFlag, txFlag theMask);

static txSlot* fxGetHostProperty(txMachine* the, txSlot* theInstance, txID theID);
#if 0
static txBoolean fxRemoveHostProperty(txMachine* the, txSlot* theInstance, txID theID);
static txSlot* fxSetHostProperty(txMachine* the, txSlot* theInstance, txID theID);
#endif

static txSlot* fxGetHostPropertyAt(txMachine* the, txSlot* theInstance, txIndex theIndex);
static txBoolean fxRemoveHostPropertyAt(txMachine* the, txSlot* theInstance, txIndex theIndex);
static txSlot* fxSetHostPropertyAt(txMachine* the, txSlot* theInstance, txIndex theIndex);


void fxQueueID(txMachine* the, txID theID, txFlag theFlag)
{
	txFlag aFlag = theFlag & ~XS_ACCESSOR_FLAG;
	txSlot* aProperty;

	aProperty = fxSetProperty(the, (the->stack + 1)->value.reference, theID, &aFlag);
	aProperty->flag |= aFlag;
	if (theFlag & XS_ACCESSOR_FLAG) {
		if (aProperty->kind != XS_ACCESSOR_KIND) {
			aProperty->kind = XS_ACCESSOR_KIND;
			aProperty->value.accessor.getter = C_NULL;
			aProperty->value.accessor.setter = C_NULL;
		}
		if (theFlag & XS_GETTER_FLAG)
			aProperty->value.accessor.getter = the->stack->value.reference;
		else
			aProperty->value.accessor.setter = the->stack->value.reference;
	}
	else {
		aProperty->kind = the->stack->kind;
		aProperty->value = the->stack->value;
	}
	the->stack++;
}

void fxEachOwnProperty(txMachine* the, txSlot* theInstance, txFlag theFlag, txStep theStep, txSlot* theContext)
{
	txSlot* aProperty;
	txIndex aCount;
	txIndex anIndex;

	aProperty = theInstance->next;
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (aProperty->kind == XS_ARRAY_KIND) {
			aCount = aProperty->value.array.length;
			if (aCount > 0x7FFF)
				mxDebug1(the, XS_RANGE_ERROR, "%ld: invalid index", aCount);
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				if ((aProperty->value.array.address + anIndex)->ID)
					(*theStep)(the, theContext, (txID)anIndex, aProperty);
			}
		}
		aProperty = aProperty->next;
	}
	while (aProperty) {
		if (!(aProperty->flag & theFlag)) {
			if (aProperty->ID != XS_NO_ID)
				(*theStep)(the, theContext, aProperty->ID, aProperty);
		}
		aProperty = aProperty->next;
	}
}

txSlot* fxGetOwnProperty(txMachine* the, txSlot* theInstance, txID theID) 
{
	txFlag aMask;
	txSlot* result;
	
	mxCheck(the, theInstance->kind == XS_INSTANCE_KIND);
	if (theInstance->flag & XS_SANDBOX_FLAG) {
		theInstance = theInstance->value.instance.prototype;
		aMask = XS_DONT_SCRIPT_FLAG;
	}
	else
		aMask = (the->frame->flag & XS_SANDBOX_FLAG) ? XS_DONT_SCRIPT_FLAG : XS_SANDBOX_FLAG;
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theID < 0) {
			if (theInstance->next->kind == XS_GLOBAL_KIND)
				return fxGetGlobalProperty(the, theInstance, theID, aMask);
		}
		else {
			if (theInstance->next->kind == XS_ARRAY_KIND)
				return fxGetArrayProperty(the, theInstance, (txIndex)theID);
			if (theInstance->next->kind == XS_HOST_KIND)
				return fxGetHostPropertyAt(the, theInstance, (txIndex)theID);
		}
	}
	result = theInstance->next;
	while (result) {
		if ((result->ID == theID) && !(result->flag & aMask))
			return result;
		result = result->next;
	}
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theID < 0) {
			result = theInstance->next;
			if ((result->kind == XS_HOST_KIND) && (result->flag & XS_HOST_HOOKS_FLAG))
				return fxGetHostProperty(the, theInstance, theID);
		}
	}
	return C_NULL;
}

txSlot* fxGetProperty(txMachine* the, txSlot* theInstance, txID theID)
{
	txFlag aMask;
	txSlot* aSlot;
	txSlot* result;
	
	mxCheck(the, theInstance->kind == XS_INSTANCE_KIND);
	if (theInstance->flag & XS_SANDBOX_FLAG) {
		theInstance = theInstance->value.instance.prototype;
		aMask = XS_DONT_SCRIPT_FLAG;
	}
	else
		aMask = (the->frame->flag & XS_SANDBOX_FLAG) ? XS_DONT_SCRIPT_FLAG : XS_SANDBOX_FLAG;
    aSlot = theInstance;
again:		
	if (aSlot->flag & XS_VALUE_FLAG) {
		if (theID < 0) {
			if (aSlot->next->kind == XS_GLOBAL_KIND)
				return fxGetGlobalProperty(the, aSlot, theID, aMask);
		}
		else {
			if (aSlot->next->kind == XS_ARRAY_KIND)
				return fxGetArrayProperty(the, aSlot, (txIndex)theID);
			if (aSlot->next->kind == XS_HOST_KIND)
				return fxGetHostPropertyAt(the, aSlot, (txIndex)theID);
		}
	}
	result = aSlot->next;
	while (result) {
		if ((result->ID == theID) && !(result->flag & aMask))
			return result;
		result = result->next;
	}
	if (aSlot->ID >= 0)
		aSlot = the->aliasArray[aSlot->ID];
	else
		aSlot = aSlot->value.instance.prototype;
	if (aSlot)
		goto again;
	/*if (theInstance == the->scope->value.reference) {
		aSlot = the->scope->next;
		while (aSlot) {
			if ((result = fxGetProperty(the, aSlot->value.reference, theID)))
				return result;
			aSlot = aSlot->next;
		}
	}*/
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theID < 0) {
			result = theInstance->next;
			if ((result->kind == XS_HOST_KIND) && (result->flag & XS_HOST_HOOKS_FLAG))
				return fxGetHostProperty(the, theInstance, theID);
		}
	}
	return C_NULL;
}

txBoolean fxRemoveProperty(txMachine* the, txSlot* theInstance, txID theID) 
{
	txFlag aMask;
	txSlot** aSlotAddress;
	txSlot* result;

	mxCheck(the, theInstance->kind == XS_INSTANCE_KIND);
	if (theInstance->flag & XS_SANDBOX_FLAG) {
		theInstance = theInstance->value.instance.prototype;
		aMask = XS_DONT_SCRIPT_FLAG;
	}
	else
		aMask = (the->frame->flag & XS_SANDBOX_FLAG) ? XS_DONT_SCRIPT_FLAG : XS_SANDBOX_FLAG;
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theID < 0) {
			if (theInstance->next->kind == XS_GLOBAL_KIND)
				return fxRemoveGlobalProperty(the, theInstance, theID, aMask);
			if (theID == the->lengthID) {
				if (theInstance->next->kind == XS_STRING_KIND)
					return 0;
				if (theInstance->next->kind == XS_ARRAY_KIND)
					return 0;
				if (theInstance->next->kind == XS_HOST_KIND)
					return 0;
			}
		}
		else {
			if (theInstance->next->kind == XS_ARRAY_KIND)
				return fxRemoveArrayProperty(the, theInstance, (txIndex)theID);
			if (theInstance->next->kind == XS_HOST_KIND)
				return fxRemoveHostPropertyAt(the, theInstance, (txIndex)theID);
		}
	}
	aSlotAddress = &(theInstance->next);
	while ((result = *aSlotAddress)) {
		if ((result->ID == theID) && !(result->flag & aMask)) {
			if (result->flag & XS_DONT_DELETE_FLAG)
				return 0;
			*aSlotAddress = result->next;
			result->next = C_NULL;
			return 1;
		}
		aSlotAddress = &(result->next);
	}
	return 1;
}

txSlot* fxSetProperty(txMachine* the, txSlot* theInstance, txID theID, txFlag* theFlag) 
{
	txFlag aMask;
	txSlot** aSlotAddress;
	txSlot* result;
	txSlot* aProperty;
	txBoolean dontPatch = 0;
	
	mxCheck(the, theInstance->kind == XS_INSTANCE_KIND);
	if (theInstance->flag & XS_SANDBOX_FLAG) {
		theInstance = theInstance->value.instance.prototype;
		aMask = XS_DONT_SCRIPT_FLAG;
	}
	else
		aMask = (the->frame->flag & XS_SANDBOX_FLAG) ? XS_DONT_SCRIPT_FLAG : XS_SANDBOX_FLAG;
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theID < 0) {
			if (theInstance->next->kind == XS_GLOBAL_KIND)
				return fxSetGlobalProperty(the, theInstance, theID, theFlag, aMask);
		}
		else {
			if (theInstance->next->kind == XS_ARRAY_KIND)
				return fxSetArrayProperty(the, theInstance, (txIndex)theID);
			if (theInstance->next->kind == XS_HOST_KIND)
				return fxSetHostPropertyAt(the, theInstance, (txIndex)theID);
		}
	}
	aSlotAddress = &(theInstance->next);
	while ((result = *aSlotAddress)) {
		if ((result->ID == theID) && !(result->flag & aMask))
			return result;
		aSlotAddress = &(result->next);
	}
	/*if (theInstance == the->scope->value.reference) {
		aProperty = the->scope->next;
		while (aProperty) {
			if ((result = fxGetProperty(the, aProperty->value.reference, theID)))
				return result;
			aProperty = aProperty->next;
		}
		//return C_NULL;
	}*/
	if (theInstance->flag & XS_DONT_PATCH_FLAG)
		dontPatch = 1;
	aProperty = C_NULL;
	for (;;) {	
		if (theInstance->ID >= 0)
			theInstance = the->aliasArray[theInstance->ID];
		else
			theInstance = theInstance->value.instance.prototype;
		if (!theInstance)
			break;
		result = theInstance->next;
		while (result) {
			if ((result->ID == theID) && !(result->flag & aMask)) {
				aProperty = result;
				break;
			}
			result = result->next;
		}
		if (aProperty)
			break;		
	}
	if (!theFlag && aProperty) {
		if (aProperty->kind == XS_ACCESSOR_KIND)
			return aProperty;
		if (aProperty->flag & XS_DONT_SET_FLAG)
			return aProperty;
	}
	if (dontPatch)
		return C_NULL;
	*aSlotAddress = result = fxNewSlot(the);
	result->ID = theID;
	if (aProperty) {
		result->flag = aProperty->flag;
		result->kind = aProperty->kind;
		result->value = aProperty->value;
	}
	else {
		if (theFlag)
			result->flag = *theFlag;
		else if (theID < 0)
			result->flag |= (aMask & XS_SANDBOX_FLAG) ? XS_DONT_SCRIPT_FLAG : XS_SANDBOX_FLAG;
	}
	return result;
}

/* [] */

txSlot* fxGetOwnPropertyAt(txMachine* the, txSlot* theInstance, txSlot* theSlot, txID* theID)
{
	txSlot* result = C_NULL;
	txIndex anIndex;
	txString aString;
	txSlot* aSymbol;

	if (theSlot->kind == XS_INTEGER_KIND)
		fxIntegerToIndex(the, theSlot->value.integer, &anIndex);
	else if (theSlot->kind == XS_NUMBER_KIND)
		fxNumberToIndex(the, theSlot->value.number, &anIndex);
	else {
		aString = fxToString(the, theSlot);
		if (!fxStringToIndex(the, aString, &anIndex)) {
			if ((aSymbol = fxFindSymbol(the, aString))) {
				*theID = aSymbol->ID;
				result = fxGetOwnProperty(the, theInstance, aSymbol->ID);
			}
			return result;
		}
	}
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theInstance->next->kind == XS_ARRAY_KIND)
			return fxGetArrayProperty(the, theInstance, anIndex);
		if (theInstance->next->kind == XS_HOST_KIND)
			return fxGetHostPropertyAt(the, theInstance, anIndex);
	}
	if (anIndex <= 0x7FFF) {
		*theID = (txID)anIndex;
		result = fxGetOwnProperty(the, theInstance, *theID);
		return result;
	}
	mxDebug1(the, XS_RANGE_ERROR, "%ld: invalid index", anIndex);
	return C_NULL;
}

txSlot* fxGetPropertyAt(txMachine* the, txSlot* theInstance, txSlot* theSlot, txID* theID) 
{
	txSlot* result = C_NULL;
	txIndex anIndex;
	txString aString;
	txSlot* aSymbol;

	if (theSlot->kind == XS_INTEGER_KIND)
		fxIntegerToIndex(the, theSlot->value.integer, &anIndex);
	else if (theSlot->kind == XS_NUMBER_KIND)
		fxNumberToIndex(the, theSlot->value.number, &anIndex);
	else {
		aString = fxToString(the, theSlot);
		if (!fxStringToIndex(the, aString, &anIndex)) {
			if ((aSymbol = fxFindSymbol(the, aString))) {
				*theID = aSymbol->ID;
				result = fxGetProperty(the, theInstance, *theID);
			}
			return result;
		}
	}
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theInstance->next->kind == XS_ARRAY_KIND)
			return fxGetArrayProperty(the, theInstance, anIndex);
		if (theInstance->next->kind == XS_HOST_KIND)
			return fxGetHostPropertyAt(the, theInstance, anIndex);
	}
	if (anIndex <= 0x7FFF) {
		*theID = (txID)anIndex;
		result = fxGetProperty(the, theInstance, *theID);
		return result;
	}
	mxDebug1(the, XS_RANGE_ERROR, "%ld: invalid index", anIndex);
	return C_NULL;
}

txBoolean fxRemovePropertyAt(txMachine* the, txSlot* theInstance, txSlot* theSlot, txID* theID) 
{
	txBoolean result = 0;
	txIndex anIndex;
	txString aString;
	txSlot* aSymbol;

	if (theSlot->kind == XS_INTEGER_KIND)
		fxIntegerToIndex(the, theSlot->value.integer, &anIndex);
	else if (theSlot->kind == XS_NUMBER_KIND)
		fxNumberToIndex(the, theSlot->value.number, &anIndex);
	else {
		aString = fxToString(the, theSlot);
		if (!fxStringToIndex(the, aString, &anIndex)) {
			if ((aSymbol = fxFindSymbol(the, aString))) {
				*theID = aSymbol->ID;
				result = fxRemoveProperty(the, theInstance, *theID);
			}
			return result;
		}
	}
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theInstance->next->kind == XS_ARRAY_KIND)
			return fxRemoveArrayProperty(the, theInstance, anIndex);
		if (theInstance->next->kind == XS_HOST_KIND)
			return fxRemoveHostPropertyAt(the, theInstance, anIndex);
	}
	if (anIndex <= 0x7FFF) {
		*theID = (txID)anIndex;
		result = fxRemoveProperty(the, theInstance, *theID);
		return result;
	}
	mxDebug1(the, XS_RANGE_ERROR, "delete %ld: invalid index", anIndex);
	return result;
}

txSlot* fxSetPropertyAt(txMachine* the, txSlot* theInstance, txSlot* theSlot, txID* theID, txFlag* theFlag) 
{
	txSlot* result = C_NULL;
	txIndex anIndex;
	txString aString;
	txSlot* aSymbol;
	
	if (theSlot->kind == XS_INTEGER_KIND)
		fxIntegerToIndex(the, theSlot->value.integer, &anIndex);
	else if (theSlot->kind == XS_NUMBER_KIND)
		fxNumberToIndex(the, theSlot->value.number, &anIndex);
	else {
		aString = fxToString(the, theSlot);
		if (!fxStringToIndex(the, aString, &anIndex)) {
			if ((aSymbol = fxNewSymbol(the, theSlot))) {
				*theID = aSymbol->ID;
				result = fxSetProperty(the, theInstance, *theID, theFlag);
			}
			return result;
		}
	}
	if (theInstance->flag & XS_VALUE_FLAG) {
		if (theInstance->next->kind == XS_ARRAY_KIND)
			return fxSetArrayProperty(the, theInstance, anIndex);
		if (theInstance->next->kind == XS_HOST_KIND)
			return fxSetHostPropertyAt(the, theInstance, anIndex);
	}
	if (anIndex <= 0x7FFF) {
		*theID = (txID)anIndex;
		result = fxSetProperty(the, theInstance, *theID, theFlag);
		return result;
	}
	mxDebug1(the, XS_RANGE_ERROR, "%ld: invalid index", anIndex);
	return C_NULL;
}

txFlag fxIntegerToIndex(txMachine* the, txInteger theInteger, txIndex* theIndex)
{
	if ((0 <= theInteger) && (theInteger < XS_MAX_INDEX)) {
		*theIndex = (txIndex)theInteger;
		return 1;
	}
	mxDebug1(the, XS_RANGE_ERROR, "%ld: invalid index", theInteger);
	return 0;
}

txFlag fxNumberToIndex(txMachine* the, txNumber theNumber, txIndex* theIndex)
{
	txIndex anIndex = (txIndex)theNumber;
	txNumber aCheck = anIndex;
	if ((theNumber == aCheck) && (anIndex < XS_MAX_INDEX)) {
		*theIndex = anIndex;
		return 1;
	}
	mxDebug1(the, XS_RANGE_ERROR, "%g: invalid index", theNumber);
	return 0;
}

txFlag fxStringToIndex(txMachine* the, txString theString, txIndex* theIndex)
{
	char aBuffer[256], c;
	txIndex anIndex;

	c = theString[0];

	if (('+' != c) && ('-' != c) && ('.' != c) && !(('0' <= c) && ('9' >= c)))
		return 0;

	anIndex = (txIndex)fxStringToNumber(the, theString, 1);
	fxNumberToString(the, anIndex, aBuffer, sizeof(aBuffer), 0, 0);
	if (!c_strcmp(theString, aBuffer)) {
		*theIndex = anIndex;
		return 1;
	}
	return 0;
}

txSlot* fxGetArrayProperty(txMachine* the, txSlot* theInstance, txIndex theIndex) 
{
	txSlot* result;

	result = theInstance->next;
	if (theIndex < result->value.array.length) {
		result = result->value.array.address + theIndex;
		if (result->ID)
			return result;
	}
	return C_NULL;
}

txBoolean fxRemoveArrayProperty(txMachine* the, txSlot* theInstance, txIndex theIndex) 
{
	txSlot* result;

	result = theInstance->next;
	if (theIndex < result->value.array.length) {
		result = result->value.array.address + theIndex;
		c_memset(result, 0, sizeof(txSlot));
	}
	return 1;
}

txSlot* fxSetArrayProperty(txMachine* the, txSlot* theInstance, txIndex theIndex) 
{
	txSlot* result;
	txSlot* aProperty;
	txInteger aLength;
	txSlot* anAddress;
	txInteger anOffset;
	
	aProperty = theInstance->next;
	aLength = aProperty->value.array.length;
	anAddress = aProperty->value.array.address;
	if (aLength <= (txInteger)theIndex) {
		anOffset = aLength;
		aLength = theIndex + 1;
		if (anAddress)
			anAddress = (txSlot *)fxRenewChunk(the, anAddress, aLength * sizeof(txSlot));
		if (!anAddress) {
			anAddress = (txSlot *)fxNewChunk(the, aLength * sizeof(txSlot));
			if (anOffset)
				c_memcpy(anAddress, aProperty->value.array.address, anOffset * sizeof(txSlot));
		}	
		c_memset(anAddress + anOffset, 0, (aLength - anOffset) * sizeof(txSlot));
		aProperty->value.array.length = aLength;
		aProperty->value.array.address = anAddress;
	}
	result = anAddress + theIndex;
	result->ID = XS_NO_ID;
	return result;
}

txSlot* fxGetGlobalProperty(txMachine* the, txSlot* theInstance, txID theID, txFlag theMask) 
{
	txSlot* globals = theInstance->next;
	txID anID;
	txSlot** aCache;

	mxCheck(the, theID < 0);
	anID = theID & 0x7FFF;
	mxCheck(the, anID < the->symbolCount);
	aCache = (theMask & XS_SANDBOX_FLAG) ? globals->value.global.cache : globals->value.global.sandboxCache;
	return aCache[anID];
}

txBoolean fxRemoveGlobalProperty(txMachine* the, txSlot* theInstance, txID theID, txFlag theMask) 
{
	txSlot* globals = theInstance->next;
	txID anID;
	txSlot** aCache;
	txSlot* result;
	txSlot** aSlotAddress;
	
	mxCheck(the, theID < 0);
	anID = theID & 0x7FFF;
	mxCheck(the, anID < the->symbolCount);
	aCache = (theMask & XS_SANDBOX_FLAG) ? globals->value.global.cache : globals->value.global.sandboxCache;
	result = aCache[anID];
	if (result) {
		aSlotAddress = &(globals->next);
		while ((result = *aSlotAddress)) {
			if (result->ID == theID) {
				if (result->flag & theMask)
					continue;
				if (result->flag & XS_DONT_DELETE_FLAG)
					return 0;
				aCache[anID] = C_NULL;
				if (theMask & XS_SANDBOX_FLAG) {
					if (!(result->flag & XS_DONT_SCRIPT_FLAG))
						globals->value.global.sandboxCache[anID] = C_NULL;
				}
				else {
					if (!(result->flag & XS_SANDBOX_FLAG))
						globals->value.global.cache[anID] = C_NULL;
				}
				*aSlotAddress = result->next;
				result->next = C_NULL;
				return 1;
			}
			aSlotAddress = &(result->next);
		}
	}
	return 1;
}

txSlot* fxSetGlobalProperty(txMachine* the, txSlot* theInstance, txID theID, txFlag* theFlag, txFlag theMask) 
{
	txSlot* globals = theInstance->next;
	txID anID;
	txSlot** aCache;
	txSlot* result;
	
	mxCheck(the, theID < 0);
	anID = theID & 0x7FFF;
	mxCheck(the, anID < the->symbolCount);
	aCache = (theMask & XS_SANDBOX_FLAG) ? globals->value.global.cache : globals->value.global.sandboxCache;
	result = aCache[anID];
	if (!result) {
		result = fxNewSlot(the);
		result->ID = theID;
		result->next = globals->next;
		globals->next = result;
		aCache[anID] = result;
		if (theFlag) {
			result->flag = *theFlag;
			if (theMask & XS_SANDBOX_FLAG) {
				if (!(result->flag & XS_DONT_SCRIPT_FLAG))
					globals->value.global.sandboxCache[anID] = result;
			}
			else {
				if (!(result->flag & XS_SANDBOX_FLAG))
					globals->value.global.cache[anID] = result;
			}
		}
		else
			result->flag |= (theMask & XS_SANDBOX_FLAG) ? XS_DONT_SCRIPT_FLAG : XS_SANDBOX_FLAG;
	}
	return result;
}

txSlot* fxGetHostProperty(txMachine* the, txSlot* theInstance, txID theID) 
{
	txSlot* aSymbol = fxGetSymbol(the, theID);
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_STRING_KIND;
	the->stack->value.string = aSymbol->value.symbol.string;
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = 1;
	mxZeroSlot(--the->stack);
	the->stack->value.reference = theInstance;
	the->stack->kind = XS_REFERENCE_KIND;
	fxCallID(the, the->peekID);
	the->scratch = *the->stack++;
    if (the->scratch.kind == XS_UNDEFINED_KIND)
        return NULL;
	the->scratch.flag = XS_GET_ONLY;
	return &the->scratch;
}

#if 0
txBoolean fxRemoveHostProperty(txMachine* the, txSlot* theInstance, txID theID) 
{
	return 1;
}

txSlot* fxSetHostProperty(txMachine* the, txSlot* theInstance, txID theID) 
{
	txSlot* aSlot = the->stack + 2;
	txSlot* aSymbol = fxGetSymbol(the, theID);
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_STRING_KIND;
	the->stack->value.string = aSymbol->value.symbol.string;
	mxZeroSlot(--the->stack);
	the->stack->kind = aSlot->kind;
	the->stack->value = aSlot->value;
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = 2;
	mxZeroSlot(--the->stack);
	the->stack->value.reference = theInstance;
	the->stack->kind = XS_REFERENCE_KIND;
	fxCallID(the, the->pokeID);
	the->stack++;
	the->scratch = *aSlot;
	return &the->scratch;
}
#endif

txSlot* fxGetHostPropertyAt(txMachine* the, txSlot* theInstance, txIndex theIndex) 
{
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = theIndex;
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = 1;
	mxZeroSlot(--the->stack);
	the->stack->value.reference = theInstance;
	the->stack->kind = XS_REFERENCE_KIND;
	fxCallID(the, the->peekID);
	the->scratch = *the->stack++;
	return &the->scratch;
}

txBoolean fxRemoveHostPropertyAt(txMachine* the, txSlot* theInstance, txIndex theIndex) 
{
	return 1;
}

txSlot* fxSetHostPropertyAt(txMachine* the, txSlot* theInstance, txIndex theIndex) 
{
	txSlot* aSlot = the->stack + 2;
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = theIndex;
	mxZeroSlot(--the->stack);
	the->stack->kind = aSlot->kind;
	the->stack->value = aSlot->value;
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = 2;
	mxZeroSlot(--the->stack);
	the->stack->value.reference = theInstance;
	the->stack->kind = XS_REFERENCE_KIND;
	fxCallID(the, the->pokeID);
	the->stack++;
	the->scratch = *aSlot;
	return &the->scratch;
}
