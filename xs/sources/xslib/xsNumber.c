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

#define mxCheckNumber(THE_SLOT) \
	if (!mxIsNumber(THE_SLOT)) \
		mxDebug0(the, XS_TYPE_ERROR, "this is no Number")

static void fx_Number(txMachine* the);
static void fx_Number_toExponential(txMachine* the);
static void fx_Number_toFixed(txMachine* the);
static void fx_Number_toPrecision(txMachine* the);
static void fx_Number_toString(txMachine* the);
static void fx_Number_valueOf(txMachine* the);

void fxBuildNumber(txMachine* the)
{
	mxPush(mxGlobal);

	mxPush(mxObjectPrototype);
	fxNewNumberInstance(the);
	fxNewHostFunction(the, fx_Number_toExponential, 1);
	fxQueueID(the, fxID(the, "toExponential"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Number_toFixed, 1);
	fxQueueID(the, fxID(the, "toFixed"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Number_toString, 0);
	fxQueueID(the, fxID(the, "toLocaleString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Number_toPrecision, 1);
	fxQueueID(the, fxID(the, "toPrecision"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Number_toString, 1);
	fxQueueID(the, fxID(the, "toString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Number_valueOf, 0);
	fxQueueID(the, fxID(the, "valueOf"), XS_DONT_ENUM_FLAG);
	
	fxAliasInstance(the, the->stack);
	mxNumberPrototype = *the->stack;
	fxNewHostConstructor(the, fx_Number, 1);
	mxPushNumber(C_DBL_MAX);
	fxQueueID(the, fxID(the, "MAX_VALUE"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(C_DBL_MIN);
	fxQueueID(the, fxID(the, "MIN_VALUE"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(C_NAN);
	fxQueueID(the, fxID(the, "NaN"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(-C_INFINITY);
	fxQueueID(the, fxID(the, "NEGATIVE_INFINITY"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(C_INFINITY);
	fxQueueID(the, fxID(the, "POSITIVE_INFINITY"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	//fxAliasInstance(the, the->stack);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxNumberPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "Number"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
		
	the->stack++;
}

txSlot* fxNewNumberInstance(txMachine* the)
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
	aProperty->flag = XS_NO_FLAG;
	aProperty->kind = XS_NUMBER_KIND;
	aProperty->value.number = 0;
	
	return anInstance;
}

void fx_Number(txMachine* the)
{
	if (mxResult->kind == XS_UNDEFINED_KIND) {
		if (mxArgc > 0) {
			txInteger anInteger;
			txNumber aNumber;

			fxToNumber(the, mxArgv(0));

			anInteger = (txInteger)mxArgv(0)->value.number;
			aNumber = anInteger;
			if (mxArgv(0)->value.number == aNumber) {
				mxResult->value.integer = anInteger;
				mxResult->kind = XS_INTEGER_KIND;
			}
			else
				*mxResult = *mxArgv(0);
		} 
		else {
			mxResult->kind = XS_NUMBER_KIND;
			mxResult->value.number = 0;
		}
	}
	else {
		txSlot* aNumber = fxGetInstance(the, mxResult);
		mxCheckNumber(aNumber);
		if (mxArgc > 0) {
			fxToNumber(the, mxArgv(0));
			aNumber->next->value.number = mxArgv(0)->value.number;
		}	
	}
}

void fx_Number_toExponential(txMachine* the)
{
	txSlot* aNumber;
	txSlot* aProperty;
	int aPrecision;
	char aBuffer[256];
	
	aNumber = fxGetInstance(the, mxThis);
	mxCheckNumber(aNumber);
	aProperty = aNumber->next;
	if (mxArgc > 0) 
		aPrecision = fxToInteger(the, mxArgv(0));
	else	
		aPrecision = 0;
	fxNumberToString(the, aProperty->value.number, aBuffer, sizeof(aBuffer), 'e', aPrecision);
	fxCopyStringC(the, mxResult, aBuffer);
}

void fx_Number_toFixed(txMachine* the)
{
	txSlot* aNumber;
	int aPrecision;
	txSlot* aProperty;
	char aBuffer[256];
	
	aNumber = fxGetInstance(the, mxThis);
	mxCheckNumber(aNumber);
	aProperty = aNumber->next;
	if (mxArgc > 0) 
		aPrecision = fxToInteger(the, mxArgv(0));
	else	
		aPrecision = 0;
	fxNumberToString(the, aProperty->value.number, aBuffer, sizeof(aBuffer), 'f', aPrecision);
	fxCopyStringC(the, mxResult, aBuffer);
}

void fx_Number_toPrecision(txMachine* the)
{
	txSlot* aNumber;
	txSlot* aProperty;
	int aPrecision;
	char aBuffer[256];
	
	aNumber = fxGetInstance(the, mxThis);
	mxCheckNumber(aNumber);
	aProperty = aNumber->next;
	if (mxArgc > 0) 
		aPrecision = fxToInteger(the, mxArgv(0));
	else	
		aPrecision = 0;
	fxNumberToString(the, aProperty->value.number, aBuffer, sizeof(aBuffer), 'g', aPrecision);
	fxCopyStringC(the, mxResult, aBuffer);
}

void fx_Number_toString(txMachine* the)
{
	txSlot* aNumber;
	txSlot* aProperty;
	txInteger aRadix;
	
	aNumber = fxGetInstance(the, mxThis);
	mxCheckNumber(aNumber);
	aProperty = aNumber->next;
	if (mxArgc > 0) {
		aRadix = fxToInteger(the, mxArgv(0));
		if (aRadix) {
			if ((aRadix < 2) || (36 < aRadix))
				mxDebug0(the, XS_RANGE_ERROR, "invalid radix");
		}
		else	
			aRadix = 10;
	}
	else	
		aRadix = 10;
	mxResult->kind = aProperty->kind;
	mxResult->value = aProperty->value;
	if (aRadix == 10)
		fxToString(the, mxResult);
	else {
		static char gxDigits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		char aBuffer[256];
		txString aString;
		txNumber aValue;
		txBoolean aMinus;
		
		fxToNumber(the, mxResult);
		aString = aBuffer + sizeof(aBuffer);
		*(--aString) = 0;
		aValue = mxResult->value.number;
		if (aValue < 0) {
			aMinus = 1;
			aValue = -aValue;
		} 
		else
			aMinus = 0;
		do {
			*(--aString) = gxDigits[(txInteger)c_fmod(aValue, aRadix)];
			aValue = aValue / aRadix;
		} while (aValue >= 1);
		if (aMinus)
			*(--aString) = '-';
		fxCopyStringC(the, mxResult, aString);
	}
}

void fx_Number_valueOf(txMachine* the)
{
	txSlot* aNumber;
	txSlot* aProperty;
	
	aNumber = fxGetInstance(the, mxThis);
	mxCheckNumber(aNumber);
	aProperty = aNumber->next;
	mxResult->kind = aProperty->kind;
	mxResult->value = aProperty->value;
}
