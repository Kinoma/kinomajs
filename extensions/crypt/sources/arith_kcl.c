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
#include "cryptTypes.h"
#include "common.h"
#include "arith.h"
#include "bn.h"

static char *kcl_cint_init(cint_t **rp);
static void kcl_cint_dispose(cint_t *cp);
static char *kcl_z_init(xsMachine *the, z_t **rp);
static void kcl_z_dispose(z_t *z);

void
xs_Arith_init(xsMachine *the)
{
}

static char *
fskErrorToString(FskErr err)
{
	static char errorString[20];

	snprintf(errorString, sizeof(errorString), "Fsk error - %ld", err);
	return errorString;
}

static char *
kcl_int_new(cint_t *ai, bn_size size)
{
	bn_t *bn;
	FskErr ferr;

	if ((ferr = FskMemPtrNew(sizeof(bn_t) + ((size - 1) * sizeof(bn_word)), (FskMemPtr *)&bn)) != kFskErrNone)
		return fskErrorToString(ferr);
	bn->sign = 0;
	bn->size = size;
	ai->cint_data = bn;
	return NULL;
}

static Boolean
kcl_int_sign(cint_t *ai)
{
	return(bn_sign(ai->cint_data));
}

static Boolean
kcl_int_isZero(cint_t *ai)
{
	return(bn_iszero(ai->cint_data));
}

static void
kcl_int_neg(cint_t *ai)
{
	bn_negate(ai->cint_data);
}

static int
kcl_int_comp(cint_t *ai, cint_t *o)
{
	return(bn_comp(ai->cint_data, o->cint_data));
}

static int
kcl_int_sizeOf(cint_t *ai)
{
	int n;
	return (n = bn_bitsize(ai->cint_data)) == 0 ? 1 : howmany(n, 8);	/* bn_bitsize = 0 when the value is zero */
}

static char *
kcl_int_copy(cint_t *ai, cint_t *src)
{
	char *ret = NULL;

	if (ai->cint_data == NULL) {
		if ((ret = kcl_int_new(ai, bn_wsizeof(src->cint_data))) != NULL)
			return(ret);
	}
	bn_copy(ai->cint_data, src->cint_data);
	return(ret);
}

static char *
kcl_int_i2os(cint_t *ai, unsigned char *os, int size)
{
	bn_t *a = ai->cint_data;
	int n;
	bn_word x;
	int blen;

	n = a->size - 1;
	x = a->data[n];
	if (x & 0xff000000)
		blen = 4;
	else if (x & 0x00ff0000)
		blen = 3;
	else if (x & 0x0000ff00)
		blen = 2;
	else
		blen = 1;
	if (size < blen + n * 4)
		return "kCryptRangeError";
	while (--blen >= 0)
		*os++ = (UInt8)(x >> (blen * 8));
	while (--n >= 0) {
		x = a->data[n];
		*os++ = (UInt8)(x >> 24);
		*os++ = (UInt8)(x >> 16);
		*os++ = (UInt8)(x >> 8);
		*os++ = (UInt8)(x);
	}
	return NULL;
}

static char *
kcl_int_os2i(cint_t *ai, unsigned char *os, int size)
{
	bn_t *bn;
	int i, j;

	if (ai->cint_data == NULL) {
		char *ret = kcl_int_new(ai, howmany(size, sizeof(bn_word)));
		if (ret) return ret;
	}
	bn = ai->cint_data;

	for (i = size, j = 0; i > 0; i -= 4, j++) {
#define str_data(n)	((n) < 0 ? 0: (bn_word)os[(n)])
		bn->data[j] = (str_data(i-4) << 24) | (str_data(i-3) << 16) | (str_data(i-2) << 8) | str_data(i-1);
	}
	/* adjust size */
	for (i = bn->size; --i >= 1 && bn->data[i] == 0;)
		;
	bn->size = i + 1;
	return(NULL);
}

static char *
kcl_int_i2str(cint_t *ai, char *s, int size, int radix)
{
	z_t *z = NULL;
	cint_t *a = NULL, *d = NULL, *q = NULL, *r = NULL;
	char *sp;
	int rem;
	char *err = NULL;

	if ((err = kcl_z_init(NULL, &z)) != NULL)
		goto bail;
	if ((err = kcl_cint_init(&d)) != NULL)
		goto bail;
	d->cint_num2i(d, radix);
	if ((err = kcl_cint_init(&a)) != NULL)
		goto bail;
	if ((err = (*a->cint_copy)(a, ai)) != NULL)
		goto bail;
	sp = s + size;
	*--sp = '\0';
	do {
		if ((err = (*z->z_div)(z, a, d, &q, &r)) != NULL)
			goto bail;
		rem = ((bn_t *)r->cint_data)->data[0];
		*--sp = (char)(rem >= 10 ? 'a' + rem - 10: '0' + rem);
		kcl_cint_dispose(a);
		kcl_cint_dispose(r);
		a = q;
	} while (!(*a->cint_isZero)(a));
	if (ai->cint_sign(ai)) *--sp = '-';
	if (s != sp)
		FskMemMove(s, sp, FskStrLen(sp) + 1);
bail:
	if (a) kcl_cint_dispose(a);
	if (d) kcl_cint_dispose(d);
	if (z) kcl_z_dispose(z);
	return(err);
}

static char *
kcl_int_str2i(cint_t *ai, char *s, int radix)
{
	z_t *z = NULL;
	cint_t *t = NULL, *r = NULL, *rr = NULL, *cradix = NULL;
	char c;
	char *err = NULL;

	if ((err = kcl_z_init(NULL, &z)) != NULL)
		goto bail;
	if ((err = kcl_cint_init(&cradix)) != NULL)
		goto bail;
	(*cradix->cint_num2i)(cradix, radix);
	if ((err = kcl_cint_init(&t)) != NULL)
		goto bail;
	(*t->cint_num2i)(t, 0);	/* just to make a room for 1 digit */
	if ((err = kcl_cint_init(&r)) != NULL)
		goto bail;
	(*r->cint_num2i)(r, 0);
	while ((c = *s++) != '\0') {
		int digit;
		if (c >= '0' && c <= '9')
			digit = c - '0';
		else if (c >= 'A' && c <= 'F')
			digit = c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			digit = c - 'a' + 10;
		else
			continue;
		if (digit < radix) {
			((bn_t *)t->cint_data)->data[0] = digit;
			if ((err = (*z->z_mul)(z, r, cradix, &rr)) != NULL)
				goto bail;
			kcl_cint_dispose(r);
			r = rr;
			if ((err = (*z->z_add)(z, r, t, &rr)) != NULL)
				goto bail;
			kcl_cint_dispose(r);
			r = rr;
		}
	}
	if (ai->cint_data == NULL) {
		ai->cint_data = r->cint_data;
		r->cint_data = NULL;
	}
	else
		kcl_int_copy(ai, r);
bail:
	if (r) kcl_cint_dispose(r);
	if (t) kcl_cint_dispose(t);
	if (cradix) kcl_cint_dispose(cradix);
	if (z) kcl_z_dispose(z);
	return(err);
}

static char *
kcl_int_i2num(cint_t *ai, long *n)
{
	bn_t *bn = (bn_t *)ai->cint_data;

	*n = bn->data[0];
	if (bn->sign)
		*n = -*n;
	return(NULL);
}

static char *
kcl_int_num2i(cint_t *ai, long n)
{
	bn_t *bn;

	if (ai->cint_data == NULL) {
		char *err = kcl_int_new(ai, 1);
		if (err) return(err);
	}
	bn = ai->cint_data;
	if (n < 0) {
		bn->data[0] = -n;
		bn->sign = 1;
	}
	else {
		bn->data[0] = n;
		bn->sign = 0;
	}
	bn->size = 1;
	return(NULL);
}

static char *
kcl_int_newFrom(cint_t *src, cint_t **dst)
{
	char *err;

	if ((err = kcl_cint_init(dst)) != NULL)
		return err;
	return kcl_int_copy(*dst, src);
}

static void
kcl_int_free(cint_t *ai)
{
	if (ai->cint_data != NULL) {
		FskMemPtrDispose(ai->cint_data);
		ai->cint_data = NULL;
	}
}

static cint_func_t cintFuncs = {
	kcl_int_sign,
	kcl_int_isZero,
	kcl_int_neg,
	kcl_int_comp,
	kcl_int_sizeOf,
	kcl_int_copy,
	kcl_int_i2os,
	kcl_int_os2i,
	kcl_int_i2str,
	kcl_int_str2i,
	kcl_int_i2num,
	kcl_int_num2i,
	kcl_int_newFrom,
	kcl_int_free,
};

static char *
kcl_cint_init(cint_t **rp)
{
	FskErr ferr;

	if ((ferr = FskMemPtrNew(sizeof(cint_t), (FskMemPtr *)rp)) != kFskErrNone)
		return fskErrorToString(ferr);
	(*rp)->cint_data = NULL;	/* this = NaN */
	(*rp)->_cint_code = &cintFuncs;
	return(NULL);
}

static void
kcl_cint_dispose(cint_t *ci)
{
	(*ci->cint_free)(ci);
	FskMemPtrDispose(ci);
}

void
xs_integer_init(xsMachine *the)
{
	cint_t *ai;
	char *err;

	if ((err = kcl_cint_init(&ai)) != NULL)
		cryptThrow(err);
	xsSetHostData(xsThis, ai);
}


/*
 * arithmetics on Z
 */

static char *
kcl_z_set_result(cint_t **r, bn_t *bn)
{
	char *err;

	if ((err = kcl_cint_init(r)) != NULL)
		return(err);
	if ((err = kcl_int_new(*r, bn->size)) != NULL) {
		(*(*r)->cint_free)(*r);
		*r = NULL;
		return(err);
	}
	bn_copy((*r)->cint_data, bn);
	return(NULL);
}

static char *
kcl_z_add(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_add(z->z_data, NULL, a->cint_data, b->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_freetbuf(z->z_data, bn);
	return(err);
}

static char *
kcl_z_sub(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_sub(z->z_data, NULL, a->cint_data, b->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_freetbuf(z->z_data, bn);
	return(err);
}

static char *
kcl_z_mul(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_mul(z->z_data, NULL, a->cint_data, b->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_freetbuf(z->z_data, bn);
	return(err);
}

static char *
kcl_z_div(z_t *z, cint_t *a, cint_t *b, cint_t **r, cint_t **m)
{
	bn_t *bn, *bn_r = NULL;
	char *err;

	bn = bn_div(z->z_data, NULL, a->cint_data, b->cint_data, m ? &bn_r: NULL);
	err = kcl_z_set_result(r, bn);
	if (m && err == NULL)
		err = kcl_z_set_result(m, bn_r);
	bn_freetbuf(z->z_data, bn);	/* free all the results */
	return(err);
}

static char *
kcl_z_square(z_t *z, cint_t *a, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_square(z->z_data, NULL, a->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_freetbuf(z->z_data, bn);
	return(err);
}

static char *
kcl_z_xor(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_xor(z->z_data, NULL, a->cint_data, b->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_freetbuf(z->z_data, bn);
	return(err);
}

static char *
kcl_z_or(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_or(z->z_data, NULL, a->cint_data, b->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_freetbuf(z->z_data, bn);
	return(err);
}

static char *
kcl_z_and(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_and(z->z_data, NULL, a->cint_data, b->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_freetbuf(z->z_data, bn);
	return(err);
}

static char *
kcl_z_lsl(z_t *z, cint_t *a, int b, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_lsl(z->z_data, NULL, a->cint_data, b);
	err = kcl_z_set_result(r, bn);
	bn_freetbuf(z->z_data, bn);
	return(err);
}

static char *
kcl_z_lsr(z_t *z, cint_t *a, int b, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_lsr(z->z_data, NULL, a->cint_data, b);
	err = kcl_z_set_result(r, bn);
	bn_freetbuf(z->z_data, bn);
	return(err);
}

static void
kcl_z_free(z_t *z)
{
	if (z->z_data)
		FskMemPtrDispose(z->z_data);
}

static z_func_t zFuncs = {
	kcl_z_add,
	kcl_z_sub,
	kcl_z_mul,
	kcl_z_div,
	kcl_z_square,
	kcl_z_xor,
	kcl_z_or,
	kcl_z_and,
	kcl_z_lsl,
	kcl_z_lsr,
	kcl_z_free,
};

static char *
kcl_z_init(xsMachine *the, z_t **rp)
{
	bn_context_t *ctx;
	FskErr ferr;

	if ((ferr = FskMemPtrNew(sizeof(z_t), (FskMemPtr *)rp)) != kFskErrNone)
		return fskErrorToString(ferr);
	if ((ferr = FskMemPtrNew(sizeof(bn_context_t), (FskMemPtr *)&ctx)) != kFskErrNone) {
		FskMemPtrDispose(*rp);
		return fskErrorToString(ferr);
	}

	bn_init_context(the, ctx);
	(*rp)->_z_code = &zFuncs;
	(*rp)->z_data = ctx;
	return(NULL);
}

static void
kcl_z_dispose(z_t *z)
{
	(*z->z_free)(z);
	FskMemPtrDispose(z);
}

void
xs_z_init(xsMachine *the)
{
	z_t *z;
	char *err;

	if ((err = kcl_z_init(the, &z)) != NULL)
		cryptThrow(err);
	z->z_protoInteger = xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("integer"));
	xsSetHostData(xsThis, z);
}

/*
 * Modular arithmetics
 */

static char *
kcl_mod_add(mod_t *mod, cint_t *a, cint_t *b, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_mod_add(mod->mod_data, NULL, a->cint_data, b->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_mod_freetbuf(mod->mod_data, bn);
	return(err);
}

static char *
kcl_mod_sub(mod_t *mod, cint_t *a, cint_t *b, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_mod_sub(mod->mod_data, NULL, a->cint_data, b->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_mod_freetbuf(mod->mod_data, bn);
	return(err);
}

static char *
kcl_mod_inv(mod_t *mod, cint_t *a, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_mod_inv(mod->mod_data, NULL, a->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_mod_freetbuf(mod->mod_data, bn);
	return(err);
}

static char *
kcl_mod_mul(mod_t *mod, cint_t *a, cint_t *b, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_mod_mul(mod->mod_data, NULL, a->cint_data, b->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_mod_freetbuf(mod->mod_data, bn);
	return(err);
}

static char *
kcl_mod_square(mod_t *mod, cint_t *a, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_mod_square(mod->mod_data, NULL, a->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_mod_freetbuf(mod->mod_data, bn);
	return(err);
}

static char *
kcl_mod_mulinv(mod_t *mod, cint_t *a, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_mod_mulinv(mod->mod_data, NULL, a->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_mod_freetbuf(mod->mod_data, bn);
	return(err);
}

static char *
kcl_mod_exp(mod_t *mod, cint_t *a, cint_t *e, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_mod_exp(mod->mod_data, NULL, a->cint_data, e->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_mod_freetbuf(mod->mod_data, bn);
	return(err);
}

static char *
kcl_mod_exp2(mod_t *mod, cint_t *a1, cint_t *e1, cint_t *a2, cint_t *e2, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_mod_exp2(mod->mod_data, NULL, a1->cint_data, e1->cint_data, a2->cint_data, e2->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_mod_freetbuf(mod->mod_data, bn);
	return(err);
}

static char *
kcl_mod_mod(mod_t *mod, cint_t *a, cint_t **r)
{
	bn_t *bn;
	char *err;

	bn = bn_mod_mod(mod->mod_data, NULL, a->cint_data);
	err = kcl_z_set_result(r, bn);
	bn_mod_freetbuf(mod->mod_data, bn);
	return(err);
}

static void
kcl_mod_free(mod_t *mod)
{
	if (mod->mod_data != NULL)
		FskMemPtrDispose(mod->mod_data);
}

static mod_func_t modFuncs = {
	kcl_mod_add,
	kcl_mod_inv,
	kcl_mod_sub,
	kcl_mod_mul,
	kcl_mod_square,
	kcl_mod_mulinv,
	kcl_mod_exp,
	kcl_mod_exp2,
	kcl_mod_mod,
	kcl_mod_free,
};

void
xs_mod_init(xsMachine *the)
{
	mod_t *mod;
	UInt32 options = xsToInteger(xsArgc) > 0 && xsTypeOf(xsArg(0)) == xsIntegerType ? xsToInteger(xsArg(0)): 0;
	FskErr ferr;

	if ((ferr = FskMemPtrNew(sizeof(mod_t), (FskMemPtr *)&mod)) != kFskErrNone)
		cryptThrowFSK(ferr);
	if ((ferr = FskMemPtrNew(sizeof(bn_mod_t), (FskMemPtr *)&mod->mod_data)) != kFskErrNone) {
		FskMemPtrDispose(mod);
		cryptThrowFSK(ferr);
	}

	mod->_mod_code = &modFuncs;

	xsResult = xsGet(xsThis, xsID("z"));	/* must exist */
	if (!xsIsInstanceOf(xsResult, xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("z"))) || (mod->z = xsGetHostData(xsResult)) == NULL)
		cryptThrow("kCryptTypeError");

	xsResult = xsGet(xsThis, xsID("m"));	/* must exist */
	if (!xsIsInstanceOf(xsResult, mod->z->z_protoInteger) || (mod->m = xsGetHostData(xsResult)) == NULL)
		nanError();

	bn_mod_init(mod->mod_data, mod->z->z_data, mod->m->cint_data, options);

	xsSetHostData(xsThis, mod);
}
