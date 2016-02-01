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

static void fx_Math_abs(txMachine* the);
static void fx_Math_acos(txMachine* the);
static void fx_Math_acosh(txMachine* the);
static void fx_Math_asin(txMachine* the);
static void fx_Math_asinh(txMachine* the);
static void fx_Math_atan(txMachine* the);
static void fx_Math_atanh(txMachine* the);
static void fx_Math_atan2(txMachine* the);
static void fx_Math_cbrt(txMachine* the);
static void fx_Math_ceil(txMachine* the);
static void fx_Math_clz32(txMachine* the);
static void fx_Math_cos(txMachine* the);
static void fx_Math_cosh(txMachine* the);
static void fx_Math_exp(txMachine* the);
static void fx_Math_expm1(txMachine* the);
static void fx_Math_floor(txMachine* the);
static void fx_Math_fround(txMachine* the);
static void fx_Math_hypot(txMachine* the);
static void fx_Math_imul(txMachine* the);
static void fx_Math_log(txMachine* the);
static void fx_Math_log1p(txMachine* the);
static void fx_Math_log10(txMachine* the);
static void fx_Math_log2(txMachine* the);
static void fx_Math_max(txMachine* the);
static void fx_Math_min(txMachine* the);
static void fx_Math_pow(txMachine* the);
static void fx_Math_random(txMachine* the);
static void fx_Math_round(txMachine* the);
static void fx_Math_sign(txMachine* the);
static void fx_Math_sin(txMachine* the);
static void fx_Math_sinh(txMachine* the);
static void fx_Math_sqrt(txMachine* the);
static void fx_Math_tan(txMachine* the);
static void fx_Math_tanh(txMachine* the);
static void fx_Math_trunc(txMachine* the);
static void fx_Math_toInteger(txMachine* the);

void fxBuildMath(txMachine* the)
{
    static const txHostFunctionBuilder gx_Math_builders[] = {
		{ fx_Math_abs, 1, _abs },
		{ fx_Math_acos, 1, _acos },
		{ fx_Math_acosh, 1, _acosh },
		{ fx_Math_asin, 1, _asin },
		{ fx_Math_asinh, 1, _asinh },
		{ fx_Math_atan, 1, _atan },
		{ fx_Math_atanh, 1, _atanh },
		{ fx_Math_atan2, 2, _atan2 },
		{ fx_Math_cbrt, 1, _cbrt },
		{ fx_Math_ceil, 1, _ceil },
		{ fx_Math_clz32, 1, _clz32 },
		{ fx_Math_cos, 1, _cos },
		{ fx_Math_cosh, 1, _cosh },
		{ fx_Math_exp, 1, _exp },
		{ fx_Math_expm1, 1, _expm1 },
		{ fx_Math_floor, 1, _floor },
		{ fx_Math_fround, 1, _fround },
		{ fx_Math_hypot, 2, _hypot_ },
		{ fx_Math_imul, 1, _imul },
		{ fx_Math_log, 1, _log },
		{ fx_Math_log1p, 1, _log1p },
		{ fx_Math_log10, 1, _log10 },
		{ fx_Math_log2, 1, _log2 },
		{ fx_Math_max, 2, _max },
		{ fx_Math_min, 2, _min },
		{ fx_Math_pow, 2, _pow },
		{ fx_Math_random, 0, _random },
		{ fx_Math_round, 1, _round },
		{ fx_Math_sign, 1, _sign },
		{ fx_Math_sin, 1, _sin },
		{ fx_Math_sinh, 1, _sinh },
		{ fx_Math_sqrt, 1, _sqrt },
		{ fx_Math_tan, 1, _tan },
		{ fx_Math_tanh, 1, _tanh },
		{ fx_Math_trunc, 1, _trunc },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	for (builder = gx_Math_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextNumberProperty(the, slot, C_M_E, mxID(_E), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_LN10, mxID(_LN10), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_LN2, mxID(_LN2), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_LOG10E, mxID(_LOG10E), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_LOG2E, mxID(_LOG2E), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_PI, mxID(_PI), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_SQRT1_2, mxID(_SQRT1_2), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_SQRT2, mxID(_SQRT2), XS_GET_ONLY);
	slot = fxNextStringXProperty(the, slot, "Math", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxSetGlobalProperty(the, mxGlobal.value.reference, mxID(_Math));
	slot->flag = XS_GET_ONLY;
	slot->kind = the->stack->kind;
	slot->value = the->stack->value;
	the->stack++;
	c_srand((unsigned)c_time(0));
}

#define mxNanResultIfNoArg \
	if (mxArgc < 1) {  \
		mxResult->kind = XS_NUMBER_KIND; \
		mxResult->value.number = C_NAN; \
		return; \
	}

#define mxNanResultIfNoArg2 \
	if (mxArgc < 2) {  \
		mxResult->kind = XS_NUMBER_KIND; \
		mxResult->value.number = C_NAN; \
		return; \
	}

void fx_Math_abs(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_fabs(mxArgv(0)->value.number);
}

void fx_Math_acos(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_acos(mxArgv(0)->value.number);
}

void fx_Math_acosh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_acosh(mxArgv(0)->value.number);
}

void fx_Math_asin(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_asin(mxArgv(0)->value.number);
}

void fx_Math_asinh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_asinh(mxArgv(0)->value.number);
}

void fx_Math_atan(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_atan(mxArgv(0)->value.number);
}

void fx_Math_atanh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_atanh(mxArgv(0)->value.number);
}

void fx_Math_atan2(txMachine* the)
{
	mxNanResultIfNoArg2;
	fxToNumber(the, mxArgv(0));
	fxToNumber(the, mxArgv(1));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_atan2(mxArgv(0)->value.number, mxArgv(1)->value.number);
}

void fx_Math_cbrt(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_cbrt(mxArgv(0)->value.number);
}

void fx_Math_ceil(txMachine* the)
{
	mxNanResultIfNoArg;
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

void fx_Math_clz32(txMachine* the)
{
	txInteger x = (mxArgc > 0) ? fxToInteger(the, mxArgv(0)) : 0;
	txInteger r;
#if mxWindows
   _BitScanForward(&r, x);
#else
	r = __builtin_clz(x);
#endif	
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = r;
}

void fx_Math_cos(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_cos(mxArgv(0)->value.number);
}

void fx_Math_cosh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_cosh(mxArgv(0)->value.number);
}

void fx_Math_exp(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_exp(mxArgv(0)->value.number);
}

void fx_Math_expm1(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_expm1(mxArgv(0)->value.number);
}

void fx_Math_floor(txMachine* the)
{
	mxNanResultIfNoArg;
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

void fx_Math_fround(txMachine* the)
{
	mxNanResultIfNoArg;
	if (XS_INTEGER_KIND == mxArgv(0)->kind) {
		mxResult->kind = XS_INTEGER_KIND;
		mxResult->value.integer = mxArgv(0)->value.integer;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = (txNumber)c_llround(mxArgv(0)->value.number);
}

void fx_Math_hypot(txMachine* the)
{
	if (mxArgc == 2) {
		fxToNumber(the, mxArgv(0));
		fxToNumber(the, mxArgv(1));
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = c_hypot(mxArgv(0)->value.number, mxArgv(1)->value.number);
	}
	else {
		txInteger c = mxArgc, i;
		txNumber result = 0;
		for (i = 0; i < c; i++) {
			txNumber argument = fxToNumber(the, mxArgv(i));
			result += argument * argument;
		}
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = c_sqrt(result);
	}
}

void fx_Math_imul(txMachine* the)
{
	txInteger x = (mxArgc > 0) ? fxToInteger(the, mxArgv(0)) : 0;
	txInteger y = (mxArgc > 1) ? fxToInteger(the, mxArgv(1)) : 0;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = x * y;
}

void fx_Math_log(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_log(mxArgv(0)->value.number);
}

void fx_Math_log1p(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_log1p(mxArgv(0)->value.number);
}

void fx_Math_log10(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_log10(mxArgv(0)->value.number);
}

void fx_Math_log2(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
#if mxAndroid
	mxResult->value.number = c_log(mxArgv(0)->value.number) / c_log(2);
#else
	mxResult->value.number = c_log2(mxArgv(0)->value.number);
#endif
}

void fx_Math_max(txMachine* the)
{
	txInteger c = mxArgc, i;
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = -((txNumber)C_INFINITY);
	for (i = 0; i < c; i++) {
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
	txInteger c = mxArgc, i;
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = (txNumber)C_INFINITY;
	for (i = 0; i < c; i++) {
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
	mxNanResultIfNoArg2;
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
	mxNanResultIfNoArg;
	if (XS_INTEGER_KIND == mxArgv(0)->kind) {
		mxResult->kind = XS_INTEGER_KIND;
		mxResult->value.integer = mxArgv(0)->value.integer;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_round(mxArgv(0)->value.number);
	fx_Math_toInteger(the);
}

void fx_Math_sqrt(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_sqrt(mxArgv(0)->value.number);
}

void fx_Math_sign(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	if (c_isnan(mxArgv(0)->value.number)) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
	}
	else {
		mxResult->kind = XS_INTEGER_KIND;
		if (mxArgv(0)->value.number < 0)
			mxResult->value.integer = -1;
		else if (mxArgv(0)->value.number > 0)
			mxResult->value.integer = 1;
		else
			mxResult->value.integer = 0;
	}
}

void fx_Math_sin(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_sin(mxArgv(0)->value.number);
}

void fx_Math_sinh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_sinh(mxArgv(0)->value.number);
}

void fx_Math_tan(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_tan(mxArgv(0)->value.number);
}

void fx_Math_tanh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_tanh(mxArgv(0)->value.number);
}

void fx_Math_trunc(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_trunc(mxArgv(0)->value.number);
	fx_Math_toInteger(the);
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
