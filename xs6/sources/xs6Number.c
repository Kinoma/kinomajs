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

static void fx_isFinite(txMachine* the);
static void fx_isNaN(txMachine* the);
static void fx_parseFloat(txMachine* the);
static void fx_parseInt(txMachine* the);
static void fx_Number(txMachine* the);
static void fx_Number_isFinite(txMachine* the);
static void fx_Number_isInteger(txMachine* the);
static void fx_Number_isNaN(txMachine* the);
static void fx_Number_isSafeInteger(txMachine* the);
static void fx_Number_prototype_toExponential(txMachine* the);
static void fx_Number_prototype_toFixed(txMachine* the);
static void fx_Number_prototype_toPrecision(txMachine* the);
static void fx_Number_prototype_toString(txMachine* the);
static void fx_Number_prototype_valueOf(txMachine* the);
static txSlot* fxCheckNumber(txMachine* the, txSlot* it);

void fxBuildNumber(txMachine* the)
{
    static const txHostFunctionBuilder gx_global_builders[] = {
		{ fx_isFinite, 1, _isFinite },
		{ fx_isNaN, 1, _isNaN },
		{ fx_parseFloat, 1, _parseFloat },
		{ fx_parseInt, 2, _parseInt },
		{ C_NULL, 0, 0 },
    };
  	static const txHostFunctionBuilder gx_Number_prototype_builders[] = {
		{ fx_Number_prototype_toExponential, 1, _toExponential },
		{ fx_Number_prototype_toFixed, 1, _toFixed },
		{ fx_Number_prototype_toString, 0, _toLocaleString },
		{ fx_Number_prototype_toPrecision, 1, _toPrecision },
		{ fx_Number_prototype_toString, 1, _toString },
		{ fx_Number_prototype_valueOf, 0, _valueOf },
		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_Number_builders[] = {
		{ fx_Number_isFinite, 1, _isFinite },
		{ fx_Number_isInteger, 1, _isInteger },
		{ fx_Number_isNaN, 1, _isNaN },
		{ fx_Number_isSafeInteger, 1, _isSafeInteger },
		{ fx_parseFloat, 1, _parseFloat },
		{ fx_parseInt, 1, _parseInt },
		{ C_NULL, 0, 0 },
    };
	const txHostFunctionBuilder* builder;
	txSlot* slot;
	
	for (builder = gx_global_builders; builder->callback; builder++) {
		fxNewHostFunctionGlobal(the, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
		the->stack++;
	}
	slot = fxSetGlobalProperty(the, mxGlobal.value.reference, mxID(_Infinity));
	slot->flag = XS_GET_ONLY;
	slot->kind = XS_NUMBER_KIND;
	slot->value.number = (txNumber)C_INFINITY;
	slot = fxSetGlobalProperty(the, mxGlobal.value.reference, mxID(_NaN));
	slot->flag = XS_GET_ONLY;
	slot->kind = XS_NUMBER_KIND;
	slot->value.number = C_NAN;

	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewNumberInstance(the));
	for (builder = gx_Number_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	mxNumberPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_Number, 1, mxID(_Number), XS_DONT_ENUM_FLAG));
	for (builder = gx_Number_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextNumberProperty(the, slot, C_EPSILON, mxID(_EPSILON), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, C_MAX_SAFE_INTEGER, mxID(_MAX_SAFE_INTEGER), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, C_DBL_MAX, mxID(_MAX_VALUE), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, C_MIN_SAFE_INTEGER, mxID(_MIN_SAFE_INTEGER), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, C_DBL_MIN, mxID(_MIN_VALUE), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, C_NAN, mxID(_NaN), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, -((txNumber)C_INFINITY), mxID(_NEGATIVE_INFINITY), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, (txNumber)C_INFINITY, mxID(_POSITIVE_INFINITY), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	the->stack++;
}

txSlot* fxNewNumberInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	property = fxNextNumberProperty(the, instance, 0, XS_NO_ID, XS_INTERNAL_FLAG | XS_GET_ONLY);
	return instance;
}

void fx_isFinite(txMachine* the)
{
	int fpclass;
	txNumber number = (mxArgc < 1) ?  C_NAN : fxToNumber(the, mxArgv(0)); 
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
	fpclass = c_fpclassify(number);
	if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE))
		mxResult->value.boolean = 1;
}

void fx_isNaN(txMachine* the)
{
	int fpclass;
	txNumber number = (mxArgc < 1) ?  C_NAN : fxToNumber(the, mxArgv(0)); 
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
	fpclass = c_fpclassify(number);
	if (fpclass == FP_NAN)
		mxResult->value.boolean = 1;
}

void fx_parseFloat(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->value.number = C_NAN;
		mxResult->kind = XS_NUMBER_KIND;
        return;
	}
	fxToString(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = fxStringToNumber(the->dtoa, mxArgv(0)->value.string, 0);
}

void fx_parseInt(txMachine* the)
{
	txInteger aRadix, aDigit;
	txNumber aSign, aResult;
	txString s, r;
	char c;
	
	if (mxArgc < 1) {
		mxResult->value.number = C_NAN;
		mxResult->kind = XS_NUMBER_KIND;
        return;
	}
	fxToString(the, mxArgv(0));
	if (mxArgc > 1) {
		aRadix = fxToInteger(the, mxArgv(1));
		if (aRadix) {
			if ((aRadix < 2) || (36 < aRadix)) {
				mxResult->kind = XS_NUMBER_KIND;
				mxResult->value.number = C_NAN;
				return;
			}
		}
	}
	else
		aRadix = 0;
	s = fxSkipSpaces(mxArgv(0)->value.string);
	c = *s;
	aSign = 1;
	if (c == '+')
		s++;
	else if (c == '-') {
		s++;
		aSign = -1;
	}
	if ((*s == '0') && ((*(s + 1) == 'x') || (*(s + 1) == 'X'))) {
		if ((aRadix == 0) || (aRadix == 16)) {
			aRadix = 16;
			s += 2;
		}
	}
	/*if (*s == '0') {
		if ((aRadix == 0) || (aRadix == 8)) {
			aRadix = 8;
		}
	}*/
	if (aRadix == 0)
		aRadix = 10;
	aResult = 0;
	r = s;
	while ((c = *s)) {
		if (('0' <= c) && (c <= '9'))
			aDigit = c - '0';
		else if (('a' <= c) && (c <= 'z'))
			aDigit = 10 + c - 'a';
		else if (('A' <= c) && (c <= 'Z'))
			aDigit = 10 + c - 'A';
		else
			break;
		if (aDigit >= aRadix)
			break;
		aResult = (aResult * aRadix) + aDigit;
		s++;
	}
	if (r == s) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
	}
	else {
		aResult *= aSign;
		aRadix = (txInteger)aResult;
		aSign = aRadix;
		if (aSign == aResult) {
			mxResult->value.integer = aRadix;
			mxResult->kind = XS_INTEGER_KIND;
		}
		else {
			mxResult->value.number = aResult;
			mxResult->kind = XS_NUMBER_KIND;
		}
	}
}

void fx_Number(txMachine* the)
{
	txSlot* slot = fxCheckNumber(the, mxThis);
	txNumber value = 0;
	if (mxArgc > 0)
		value = fxToNumber(the, mxArgv(0));
	if (slot)
		slot->value.number = value;
	else {
		txInteger integer = (txInteger)value;
		txNumber number = integer;
		if (number == value) {
			mxResult->kind = XS_INTEGER_KIND;
			mxResult->value.integer = integer;
		}
		else {
			mxResult->kind = XS_NUMBER_KIND;
			mxResult->value.number = value;
		}
	}
}

void fx_Number_isFinite(txMachine* the)
{
	int fpclass;
	txSlot* slot = (mxArgc < 1) ?  C_NULL : fxCheckNumber(the, mxArgv(0));
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
    if (slot) {
		fpclass = c_fpclassify(slot->value.number);
		if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE))
			mxResult->value.boolean = 1;
	}
}

void fx_Number_isInteger(txMachine* the)
{
	int fpclass;
	txSlot* slot = (mxArgc < 1) ?  C_NULL : fxCheckNumber(the, mxArgv(0));
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
    if (slot) {
		fpclass = c_fpclassify(slot->value.number);
		if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE)) {
			txNumber check = c_trunc(slot->value.number);
			if (slot->value.number == check)
				mxResult->value.boolean = 1;
		}
	}
}

void fx_Number_isNaN(txMachine* the)
{
	int fpclass;
	txSlot* slot = (mxArgc < 1) ?  C_NULL : fxCheckNumber(the, mxArgv(0));
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
    if (slot) {
		fpclass = c_fpclassify(slot->value.number);
		if (fpclass == FP_NAN)
			mxResult->value.boolean = 1;
	}
}

void fx_Number_isSafeInteger(txMachine* the)
{
	int fpclass;
	txSlot* slot = (mxArgc < 1) ?  C_NULL : fxCheckNumber(the, mxArgv(0));
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
    if (slot) {
		fpclass = c_fpclassify(slot->value.number);
		if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE)) {
			txNumber check = c_trunc(slot->value.number);
			if (slot->value.number == check) {
				if ((C_MIN_SAFE_INTEGER <= check) && (check <= C_MAX_SAFE_INTEGER))
					mxResult->value.boolean = 1;
			}
		}
	}
}

void fx_Number_prototype_toExponential(txMachine* the)
{
	char buffer[256];
	int precision;
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this is no number");
	if (mxArgc > 0) 
		precision = fxToInteger(the, mxArgv(0));
	else	
		precision = 0;
	fxNumberToString(the->dtoa, slot->value.number, buffer, sizeof(buffer), 'e', precision);
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Number_prototype_toFixed(txMachine* the)
{
	char buffer[256];
	int precision;
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this is no number");
	if (mxArgc > 0) 
		precision = fxToInteger(the, mxArgv(0));
	else	
		precision = 0;
	fxNumberToString(the->dtoa, slot->value.number, buffer, sizeof(buffer), 'f', precision);
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Number_prototype_toPrecision(txMachine* the)
{
	char buffer[256];
	int precision;
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this is no number");
	if (mxArgc > 0) 
		precision = fxToInteger(the, mxArgv(0));
	else	
		precision = 0;
	fxNumberToString(the->dtoa, slot->value.number, buffer, sizeof(buffer), 'g', precision);
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Number_prototype_toString(txMachine* the)
{
	char buffer[256];
	txInteger radix;
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this is no number");
	if (mxArgc > 0) {
		radix = fxToInteger(the, mxArgv(0));
		if (radix) {
			if ((radix < 2) || (36 < radix))
				mxRangeError("invalid radix");
		}
		else	
			radix = 10;
	}
	else	
		radix = 10;
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
	if (radix == 10)
		fxToString(the, mxResult);
	else {
		static char gxDigits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		txString string = buffer + sizeof(buffer);
		txNumber value;
		txBoolean minus;
		value = mxResult->value.number;
		switch (c_fpclassify(value)) {
		case C_FP_INFINITE:
			if (value < 0)
				fxCopyStringC(the, mxResult, "-Infinity");
			else
				fxCopyStringC(the, mxResult, "Infinity");
			break;
		case C_FP_NAN:
			fxCopyStringC(the, mxResult, "NaN");
			break;
		case C_FP_ZERO:
			fxCopyStringC(the, mxResult, "0");
			break;
		default:
			*(--string) = 0;
			if (value < 0) {
				minus = 1;
				value = -value;
			} 
			else
				minus = 0;
			do {
				*(--string) = gxDigits[(txInteger)c_fmod(value, radix)];
				value = value / radix;
			} while (value >= 1);
			if (minus)
				*(--string) = '-';
			fxCopyStringC(the, mxResult, string);
		}
	}
}

void fx_Number_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this is no number");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

txSlot* fxCheckNumber(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if (it->kind == XS_INTEGER_KIND) {
		fxToNumber(the, it);
		result = it;
	}
	else if (it->kind == XS_NUMBER_KIND)
		result = it;
	else if (it->kind == XS_REFERENCE_KIND) {
		txSlot* instance = it->value.reference;
		it = instance->next;
		if ((it) && (it->flag & XS_INTERNAL_FLAG) && (it->kind == XS_NUMBER_KIND))
			result = it;
	}
	return result;
}
