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

static void fx_Math_abs(txMachine* the);
static void fx_Math_acos(txMachine* the);
static void fx_Math_asin(txMachine* the);
static void fx_Math_atan(txMachine* the);
static void fx_Math_atan2(txMachine* the);
static void fx_Math_ceil(txMachine* the);
static void fx_Math_cos(txMachine* the);
static void fx_Math_exp(txMachine* the);
static void fx_Math_floor(txMachine* the);
static void fx_Math_log(txMachine* the);
static void fx_Math_max(txMachine* the);
static void fx_Math_min(txMachine* the);
static void fx_Math_pow(txMachine* the);
static void fx_Math_random(txMachine* the);
static void fx_Math_round(txMachine* the);
static void fx_Math_sin(txMachine* the);
static void fx_Math_sqrt(txMachine* the);
static void fx_Math_tan(txMachine* the);
static void fx_Math_toInteger(txMachine* the);

void fxBuildMath(txMachine* the)
{
	txSlot* aSlot;
	
	mxPush(mxGlobal);
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	
	/* hack Object.prototype.toString */
	aSlot = the->stack->value.reference;
	aSlot->flag = XS_VALUE_FLAG;
	aSlot->next = fxNewSlot(the);
	aSlot = aSlot->next;
	aSlot->kind = XS_UNDEFINED_KIND;

	mxPushNumber(C_M_E);
	fxQueueID(the, fxID(the, "E"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(C_M_LOG2E);
	fxQueueID(the, fxID(the, "LOG2E"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(C_M_LOG10E);
	fxQueueID(the, fxID(the, "LOG10E"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(C_M_LN2);
	fxQueueID(the, fxID(the, "LN2"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(C_M_LN10);
	fxQueueID(the, fxID(the, "LN10"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(C_M_PI);
	fxQueueID(the, fxID(the, "PI"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(C_M_SQRT1_2);
	fxQueueID(the, fxID(the, "SQRT1_2"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(C_M_SQRT2);
	fxQueueID(the, fxID(the, "SQRT2"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_Math_abs, 1);
	fxQueueID(the, fxID(the, "abs"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_acos, 1);
	fxQueueID(the, fxID(the, "acos"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_asin, 1);
	fxQueueID(the, fxID(the, "asin"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_atan, 1);
	fxQueueID(the, fxID(the, "atan"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_atan2, 2);
	fxQueueID(the, fxID(the, "atan2"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_ceil, 1);
	fxQueueID(the, fxID(the, "ceil"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_cos, 1);
	fxQueueID(the, fxID(the, "cos"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_exp, 1);
	fxQueueID(the, fxID(the, "exp"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_floor, 1);
	fxQueueID(the, fxID(the, "floor"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_log, 1);
	fxQueueID(the, fxID(the, "log"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_max, 2);
	fxQueueID(the, fxID(the, "max"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_min, 2);
	fxQueueID(the, fxID(the, "min"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_pow, 2);
	fxQueueID(the, fxID(the, "pow"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_random, 0);
	fxQueueID(the, fxID(the, "random"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_round, 1);
	fxQueueID(the, fxID(the, "round"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_sin, 1);
	fxQueueID(the, fxID(the, "sin"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_sqrt, 1);
	fxQueueID(the, fxID(the, "sqrt"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Math_tan, 1);
	fxQueueID(the, fxID(the, "tan"), XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "Math"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	the->stack++;
	c_srand((unsigned)c_time(0));
}

void fx_Math_abs(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_fabs(mxArgv(0)->value.number);
}

void fx_Math_acos(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_acos(mxArgv(0)->value.number);
}

void fx_Math_asin(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_asin(mxArgv(0)->value.number);
}

void fx_Math_atan(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_atan(mxArgv(0)->value.number);
}

void fx_Math_atan2(txMachine* the)
{
	if (mxArgc < 2) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	fxToNumber(the, mxArgv(0));
	fxToNumber(the, mxArgv(1));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_atan2(mxArgv(0)->value.number, mxArgv(1)->value.number);
}

void fx_Math_ceil(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	if (XS_INTEGER_KIND == mxArgv(0)->kind) {
		mxResult->kind = XS_INTEGER_KIND;
		mxResult->value.integer = mxArgv(0)->value.integer;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_ceil(mxArgv(0)->value.number);
	fx_Math_toInteger(the);
}

void fx_Math_cos(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_cos(mxArgv(0)->value.number);
}

void fx_Math_exp(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_exp(mxArgv(0)->value.number);
}

void fx_Math_floor(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	if (XS_INTEGER_KIND == mxArgv(0)->kind) {
		mxResult->kind = XS_INTEGER_KIND;
		mxResult->value.integer = mxArgv(0)->value.integer;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_floor(mxArgv(0)->value.number);
	fx_Math_toInteger(the);
}

void fx_Math_log(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_log(mxArgv(0)->value.number);
}

void fx_Math_max(txMachine* the)
{
	txInteger i;
	
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = -C_INFINITY;
	for (i = 0; i < mxArgc; i++) {
		txNumber n = fxToNumber(the, mxArgv(i));
		if (c_isnan(n)) {
			mxResult->value.number = C_NAN;
			return;
		}
		if (mxResult->value.number < n)
			mxResult->value.number = n;
	}
}

void fx_Math_min(txMachine* the)
{
	txInteger i;
	
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = C_INFINITY;
	for (i = 0; i < mxArgc; i++) {
		txNumber n = fxToNumber(the, mxArgv(i));
		if (c_isnan(n)) {
			mxResult->value.number = C_NAN;
			return;
		}
		if (mxResult->value.number > n)
			mxResult->value.number = n;
	}
}

void fx_Math_pow(txMachine* the)
{
	txNumber x, y, r;
	
	if (mxArgc < 2) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	x = fxToNumber(the, mxArgv(0));
	y = fxToNumber(the, mxArgv(1));
	mxResult->kind = XS_NUMBER_KIND;
	if (c_isnan(y))
		r = C_NAN;
	else
		r = c_pow(x, y);
	mxResult->value.number = r;
}

void fx_Math_random(txMachine* the)
{
	double result = c_rand();
	while (result == C_RAND_MAX)
		result = c_rand();
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = result / (double)C_RAND_MAX;
}

void fx_Math_round(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}

	if (XS_INTEGER_KIND == mxArgv(0)->kind) {
		mxResult->kind = XS_INTEGER_KIND;
		mxResult->value.integer = mxArgv(0)->value.integer;
		return;
	}

	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_floor(mxArgv(0)->value.number + 0.5);
	fx_Math_toInteger(the);
}

void fx_Math_sqrt(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_sqrt(mxArgv(0)->value.number);
}

void fx_Math_sin(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_sin(mxArgv(0)->value.number);
}

void fx_Math_tan(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_tan(mxArgv(0)->value.number);
}

void fx_Math_toInteger(txMachine* the)
{
	txInteger anInteger;
	txNumber aNumber;
	anInteger = (txInteger)mxResult->value.number;
	aNumber = anInteger;
	if (mxResult->value.number == aNumber) {
		mxResult->value.integer = anInteger;
		mxResult->kind = XS_INTEGER_KIND;
	}
}
