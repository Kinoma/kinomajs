/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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

#include "arith_common.h"
#include "kcl_arith.h"

#define arith_get_integer(z, slot)	__arith_get_integer(the, &slot, &z->proto_int)
#define arith_set_integer(z, slot, x)	__arith_set_integer(the, &slot, x, &z->proto_int)

typedef kcl_err_t (*kcl_z_f1)(kcl_z_t *, kcl_int_t *a, kcl_int_t **r);
typedef kcl_err_t (*kcl_z_f2)(kcl_z_t *, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r);
typedef kcl_err_t (*kcl_z_f2d)(kcl_z_t *, kcl_int_t *a, int b, kcl_int_t **r);
typedef kcl_err_t (*kcl_z_f3)(kcl_z_t *, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r, kcl_int_t **m);

static void
z_call1(xsMachine *the, z_t *z, kcl_z_f1 f)
{
	kcl_int_t *a = arith_get_integer(z, xsArg(0));
	kcl_int_t *r = NULL;
	kcl_err_t err;

	err = (*f)(z->ctx, a, &r);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	arith_set_integer(z, xsResult, r);
}

static void
z_call2(xsMachine *the, z_t *z, kcl_z_f2 f)
{
	kcl_int_t *a = arith_get_integer(z, xsArg(0));
	kcl_int_t *b = arith_get_integer(z, xsArg(1));
	kcl_int_t *r = NULL;
	kcl_err_t err;

	err = (*f)(z->ctx, a, b, &r);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	arith_set_integer(z, xsResult, r);
}

static void
z_call2d(xsMachine *the, z_t *z, kcl_z_f2d f)
{
	kcl_int_t *a = arith_get_integer(z, xsArg(0));
	xsIntegerValue b = xsToInteger(xsArg(1));
	kcl_int_t *r = NULL;
	kcl_err_t err;

	err = (*f)(z->ctx, a, (int)b, &r);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	arith_set_integer(z, xsResult, r);
}

static void
z_call3(xsMachine *the, z_t *z, kcl_z_f3 f, xsSlot *rem)
{
	kcl_int_t *a = arith_get_integer(z, xsArg(0));
	kcl_int_t *b = arith_get_integer(z, xsArg(1));
	kcl_int_t *r = NULL, *m = NULL;
	kcl_err_t err;

	err = (*f)(z->ctx, a, b, &r, rem != NULL ? &m : NULL);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	arith_set_integer(z, xsResult, r);
	if (rem != NULL) {
		arith_set_integer(z, *rem, m);
	}
}

static void
kcl_z_error_callback(kcl_err_t err, void *closure)
{
	xsMachine *the = closure;
	kcl_throw_error(the, err);
}

void
xs_z_init(xsMachine *the)
{
	z_t *z;
	kcl_err_t err;
	static kcl_error_callback_t cb;

	if ((z = crypt_malloc(sizeof(z_t))) == NULL)
		kcl_throw_error(the, KCL_ERR_NOMEM);
	if ((err = kcl_z_alloc(&z->ctx)) != KCL_ERR_NONE) {
		crypt_free(z);
		kcl_throw_error(the, err);
	}
	z->proto_int = xsGet(xsThis, xsID("_proto_int"));
	cb.f = kcl_z_error_callback;
	cb.closure = the;
	if ((err = kcl_z_init(z->ctx, &cb)) != KCL_ERR_NONE) {
		kcl_z_dispose(z->ctx);
		crypt_free(z);
		kcl_throw_error(the, err);
	}
	xsSetHostData(xsThis, z);
}

void
xs_z_destructor(void *data)
{
	if (data) {
		z_t *z = data;
		if (z->ctx != NULL)
			kcl_z_dispose(z->ctx);
		crypt_free(z);
	}
}

void
xs_z_add(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	z_call2(the, z, kcl_z_add);
}

void
xs_z_sub(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	z_call2(the, z, kcl_z_sub);
}

void
xs_z_mul(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	z_call2(the, z, kcl_z_mul);
}

void
xs_z_div2(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	xsVars(2);
	z_call3(the, z, kcl_z_div, &xsVar(1));
	xsVar(0) = xsResult;
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsSet(xsResult, xsID("q"), xsVar(0));
	xsSet(xsResult, xsID("r"), xsVar(1));
}

void
xs_z_div(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	z_call3(the, z, kcl_z_div, NULL);
}

void
xs_z_mod(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	xsVars(1);
	z_call3(the, z, kcl_z_div, &xsVar(0));
	xsResult = xsVar(0);
}

void
xs_z_square(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	z_call1(the, z, kcl_z_square);
}

void
xs_z_xor(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	z_call2(the, z, kcl_z_xor);
}

void
xs_z_or(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	z_call2(the, z, kcl_z_or);
}

void
xs_z_and(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	z_call2(the, z, kcl_z_and);
}

void
xs_z_lsl(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	z_call2d(the, z, kcl_z_lsl);
}

void
xs_z_lsr(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);

	z_call2d(the, z, kcl_z_lsr);
}

void
xs_z_toString(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	kcl_int_t *ai = arith_get_integer(z, xsArg(0));
	unsigned int radix = xsToInteger(xsArg(1));
	size_t usize, n;
	char *str;
	kcl_err_t err;
#define NBITS(n)	(n < 4 ? 1: n < 8 ? 2: n < 16 ? 3: n < 32 ? 4: 5)

	usize = kcl_int_sizeof(ai);
	n = (usize * 8) / NBITS(radix);	/* quite inaccurate, but better than shortage */
	n += 2;	/* for "+-" sign + '\0' */
	if ((str = crypt_malloc(n)) == NULL)
		kcl_throw_error(the, KCL_ERR_NOMEM);
	if ((err = kcl_z_i2str(z->ctx, ai, str, n, radix)) != KCL_ERR_NONE)
		goto bail;
	xsResult = xsString(str);
bail:
	crypt_free(str);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
}

void
xs_z_toInteger(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	char *digits = xsToString(xsArg(0));
	unsigned int radix = xsToInteger(xsArg(1));
	kcl_int_t *ai = NULL;
	kcl_err_t err;

	if ((err = kcl_z_str2i(z->ctx, &ai, digits, radix)) != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	arith_set_integer(z, xsResult, ai);
}
