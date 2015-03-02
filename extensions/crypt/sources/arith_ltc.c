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
#include "cryptTypes.h"
#include "common.h"
#include "arith.h"

#include "tomcrypt.h"
#include "tommath.h"	/* use directly the libtommath functions (only) for bitwise operations */

ltc_math_descriptor ltc_mp;

void
xs_Arith_init(xsMachine *the)
{
	ltc_mp = ltm_desc;
}

static char *
ltc_to_crypt_error(int ltc_error)
{
	switch (ltc_error) {
	case CRYPT_OK:               /* Result OK */
		return "kCryptNoError";
	case CRYPT_ERROR:            /* Generic Error */
		return "kCryptErrUnknwon";
	case CRYPT_NOP:              /* Not a failure but no operation was performed */
		return "kCryptNoError";

	case CRYPT_FAIL_TESTVECTOR:  /* Algorithm failed test vectors */
		return "kCryptUnknownError";

	case CRYPT_INVALID_KEYSIZE:  /* Invalid key size given */
	case CRYPT_INVALID_ROUNDS:   /* Invalid number of rounds */
		return "kCryptParameterError";

	case CRYPT_BUFFER_OVERFLOW:  /* Not enough space for output */
	case CRYPT_INVALID_PACKET:   /* Invalid input packet given */
		return "kCryptMalformedInput";

	case CRYPT_INVALID_PRNGSIZE: /* Invalid number of bits for a PRNG */
	case CRYPT_ERROR_READPRNG:   /* Could not read enough from PRNG */
		return "kCryptRangeError";

	case CRYPT_INVALID_CIPHER:   /* Invalid cipher specified */
	case CRYPT_INVALID_HASH:     /* Invalid hash specified */
	case CRYPT_INVALID_PRNG:     /* Invalid PRNG specified */
		return "kCryptInvalidAlgorithm";

	case CRYPT_MEM:              /* Out of memory */
		return "kCryptMemoryFull";

	case CRYPT_PK_TYPE_MISMATCH: /* Not equivalent types of PK keys */
	case CRYPT_PK_NOT_PRIVATE:   /* Requires a private PK key */

	case CRYPT_INVALID_ARG:      /* Generic invalid argument */
		return "kCryptParameterError";

	case CRYPT_FILE_NOTFOUND:    /* File Not Found */
	case CRYPT_PK_NOT_FOUND:     /* Key not found in keyring */
		return "kCryptKeyNotFound";

	case CRYPT_PK_INVALID_TYPE:  /* Invalid type of PK key */
	case CRYPT_PK_INVALID_SYSTEM:/* Invalid PK system specified */
	case CRYPT_PK_DUP:           /* Duplicate key already in key ring */
	case CRYPT_PK_INVALID_SIZE:  /* Invalid size input for PK parameters */
	case CRYPT_INVALID_PRIME_SIZE:/* Invalid size of prime requested */
		return "kCryptParameterError";

	default:
		return "kCryptUnknownError";
	}
}

#define cryptThrowLTC(ltcError)	cryptThrow(ltc_to_crypt_error(ltcError))

/*
 * implementation of the integer object
 */

static Boolean
ltc_int_sign(cint_t *ai)
{
	return(ltc_mp.compare_d(ai->cint_data, 0) < 0);
}

static Boolean
ltc_int_isZero(cint_t *ai)
{
	return(ltc_mp.compare_d(ai->cint_data, 0) == LTC_MP_EQ);
}

static void
ltc_int_neg(cint_t *ai)
{
	ltc_mp.neg(ai->cint_data, ai->cint_data);
}

static int
ltc_int_comp(cint_t *ai, cint_t *o)
{
	return(ltc_mp.compare(ai->cint_data, o->cint_data));
}

static int
ltc_int_sizeOf(cint_t *ai)
{
	int n;
	return((n = ltc_mp.unsigned_size(ai->cint_data)) == 0 ? 1: n);	/* libtom returns 0 when the bignum is zero */
}

static char *
ltc_int_copy(cint_t *ai, cint_t *src)
{
	int err;

	if (ai->cint_data == NULL) {
		if ((err = ltc_mp.init(&ai->cint_data)) != CRYPT_OK)
			return(ltc_to_crypt_error(err));
	}
	err = ltc_mp.copy(src->cint_data, ai->cint_data);
	return(err == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_int_i2os(cint_t *ai, unsigned char *os, int size)
{
	int err;

	err = ltc_mp.unsigned_write(ai->cint_data, os);	/* should write exact size */
	return(err == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_int_os2i(cint_t *ai, unsigned char *os, int size)
{
	int err;

	if (ai->cint_data == NULL) {
		if ((err = ltc_mp.init(&ai->cint_data)) != CRYPT_OK)
			return(ltc_to_crypt_error(err));
	}
	err = ltc_mp.unsigned_read(ai->cint_data, os, size);
	return(err == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_int_i2str(cint_t *ai, char *s, int size, int radix)
{
	int err;

	/* libtom doesn't check the size of the string! */
	err = ltc_mp.write_radix(ai->cint_data, s, radix);
	return(err == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_int_str2i(cint_t *ai, char *s, int radix)
{
	int err;

	if (ai->cint_data == NULL) {
		if ((err = ltc_mp.init(&ai->cint_data)) != CRYPT_OK)
			return(ltc_to_crypt_error(err));
	}
	err = ltc_mp.read_radix(ai->cint_data, s, radix);
	return(err == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_int_i2num(cint_t *ai, long *n)
{
	*n = ltc_mp.get_int(ai->cint_data);
	return(NULL);
}

static char *
ltc_int_num2i(cint_t *ai, long n)
{
	int err;

	if (ai->cint_data == NULL) {
		if ((err = ltc_mp.init(&ai->cint_data)) != CRYPT_OK)
			return(ltc_to_crypt_error(err));
	}
	if (n < 0)
		(void)((err = ltc_mp.set_int(ai->cint_data, -n)) != CRYPT_OK || (err = ltc_mp.neg(ai->cint_data, ai->cint_data)));
	else
		err = ltc_mp.set_int(ai->cint_data, n);
	return(err == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static int ltc_int_new(cint_t **r);

static char *
ltc_int_newFrom(cint_t *src, cint_t **dst)
{
	int err;

	return((err = ltc_int_new(dst)) != CRYPT_OK ? ltc_to_crypt_error(err) : ltc_int_copy(*dst, src));
}

static void
ltc_int_free(cint_t *ai)
{
	if (ai->cint_data != NULL) {
		ltc_mp.deinit(ai->cint_data);
		ai->cint_data = NULL;
	}
}

static cint_func_t intFuncs = {
	ltc_int_sign,
	ltc_int_isZero,
	ltc_int_neg,
	ltc_int_comp,
	ltc_int_sizeOf,
	ltc_int_copy,
	ltc_int_i2os,
	ltc_int_os2i,
	ltc_int_i2str,
	ltc_int_str2i,
	ltc_int_i2num,
	ltc_int_num2i,
	ltc_int_newFrom,
	ltc_int_free,
};

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord gCryptIntegerTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"arith.integer",
	FskInstrumentationOffset(cint_t),
	NULL,
	0,
	NULL,
	cryptInstrumentFormat,
};
#endif

static int
ltc_int_new(cint_t **r)
{
	cint_t *ci;
	int err;
	FskErr ferr;

	if ((ferr = FskMemPtrNew(sizeof(cint_t), (FskMemPtr *)&ci)) == kFskErrNone) {
		ci->_cint_code = &intFuncs;
		err = ltc_mp.init(&ci->cint_data);
		if (err != CRYPT_OK) {
			FskMemPtrDispose(ci);
			*r = NULL;
		}
		else {
			*r = ci;
			FskInstrumentedItemNew(*r, "integer", &gCryptIntegerTypeInstrumentation);
		}
	}
	else {
		*r = NULL;
		err = CRYPT_MEM;
	}
	return(err);
}

void
xs_integer_init(xsMachine *the)
{
	cint_t *ci;
	FskErr ferr;
	XS_FNAME("Arith.integer._init");

	if ((ferr = FskMemPtrNew(sizeof(cint_t), (FskMemPtr *)&ci)) != kFskErrNone)
		cryptThrowFSK(ferr);
	ci->_cint_code = &intFuncs;
	ci->cint_data = NULL;	/* this = NaN */
	FskInstrumentedItemNew(ci, "integer", &gCryptIntegerTypeInstrumentation);
	xsSetHostData(xsThis, ci);
}


/*
 * arithmetics on Z
 */

static char *
ltc_z_add(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = ltc_mp.add(a->cint_data, b->cint_data, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_z_sub(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = ltc_mp.sub(a->cint_data, b->cint_data, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_z_mul(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = ltc_mp.mul(a->cint_data, b->cint_data, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_z_div(z_t *z, cint_t *a, cint_t *b, cint_t **r, cint_t **m)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = ltc_int_new(m)) == CRYPT_OK && (err = ltc_mp.mpdiv(a->cint_data, b->cint_data, (*r)->cint_data, (*m)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_z_square(z_t *z, cint_t *a, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = ltc_mp.mul(a->cint_data, a->cint_data, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

/* the following bitwise operations depend on libtommath (i.e. functions whose name is mp_xxx) */

static char *
ltc_z_xor(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = mp_xor(a->cint_data, b->cint_data, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_z_or(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = mp_or(a->cint_data, b->cint_data, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_z_and(z_t *z, cint_t *a, cint_t *b, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = mp_and(a->cint_data, b->cint_data, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_z_lsl(z_t *z, cint_t *a, int b, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = mp_mul_2d(a->cint_data, b, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_z_lsr(z_t *z, cint_t *a, int b, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = mp_div_2d(a->cint_data, b, (*r)->cint_data, NULL)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static void
ltc_z_free(z_t *z)
{
	/* nothing to do */
}

static z_func_t zFuncs = {
	ltc_z_add,
	ltc_z_sub,
	ltc_z_mul,
	ltc_z_div,
	ltc_z_square,
	ltc_z_xor,
	ltc_z_or,
	ltc_z_and,
	ltc_z_lsl,
	ltc_z_lsr,
	ltc_z_free,
};

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord gCryptZTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"arith.z",
	FskInstrumentationOffset(z_t),
	NULL,
	0,
	NULL,
	cryptInstrumentFormat,
};
#endif

void
xs_z_init(xsMachine *the)
{
	z_t *z;
	FskErr ferr;
	XS_FNAME("Arith.z._init");

	if ((ferr = FskMemPtrNew(sizeof(z_t), (FskMemPtr *)&z)) != kFskErrNone)
		cryptThrowFSK(ferr);
	z->_z_code = &zFuncs;
	/* LTC does not use "_data" */
	z->z_protoInteger = xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("integer"));
	FskInstrumentedItemNew(z, "z", &gCryptZTypeInstrumentation);
	xsSetHostData(xsThis, z);
}


/*
 * Modular arithmetics
 */

static char *
ltc_mod_add(mod_t *mod, cint_t *a, cint_t *b, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = ltc_mp.add(a->cint_data, b->cint_data, (*r)->cint_data)) == CRYPT_OK && (err = ltc_mp.mpdiv((*r)->cint_data, mod->m->cint_data, NULL, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_mod_sub(mod_t *mod, cint_t *a, cint_t *b, cint_t **r)
{
	int err;

	if ((err = ltc_int_new(r)) != CRYPT_OK || (err = ltc_mp.sub(a->cint_data, b->cint_data, (*r)->cint_data)) != CRYPT_OK)
		return(ltc_to_crypt_error(err));
	if (ltc_mp.compare_d((*r)->cint_data, 0) < 0) {
		while (ltc_mp.compare_d((*r)->cint_data, 0) < 0) {
			if ((err = ltc_mp.add((*r)->cint_data, mod->m->cint_data, (*r)->cint_data)) != CRYPT_OK)
				return(ltc_to_crypt_error(err));
		}
	}
	else {
		while (ltc_mp.compare((*r)->cint_data, mod->m->cint_data) >= 0) {
			if ((err = ltc_mp.sub((*r)->cint_data, mod->m->cint_data, (*r)->cint_data)) != CRYPT_OK)
				return(ltc_to_crypt_error(err));
		}
	}
	return(NULL);
}

static char *
ltc_mod_inv(mod_t *mod, cint_t *a, cint_t **r)
{
	return(ltc_mod_sub(mod, mod->m, a, r));
}

static char *
ltc_mod_mul(mod_t *mod, cint_t *a, cint_t *b, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = ltc_mp.mulmod(a->cint_data, b->cint_data, mod->m->cint_data, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_mod_square(mod_t *mod, cint_t *a, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = ltc_mp.sqrmod(a->cint_data, mod->m->cint_data, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_mod_mulinv(mod_t *mod, cint_t *a, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = ltc_mp.invmod(a->cint_data, mod->m->cint_data, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_mod_exp(mod_t *mod, cint_t *a, cint_t *e, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = ltc_mp.exptmod(a->cint_data, e->cint_data, mod->m->cint_data, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_mod_exp2(mod_t *mod, cint_t *a1, cint_t *e1, cint_t *a2, cint_t *e2, cint_t **r)
{
	int err;
	void *r1 = NULL, *r2 = NULL;

	(void)((err = ltc_int_new(r)) == CRYPT_OK &&
	       (err = ltc_mp.init(&r1)) == CRYPT_OK && (err = ltc_mp.exptmod(a1->cint_data, e1->cint_data, mod->m->cint_data, r1)) == CRYPT_OK &&
	       (err = ltc_mp.init(&r2)) == CRYPT_OK && (err = ltc_mp.exptmod(a2->cint_data, e2->cint_data, mod->m->cint_data, r2)) == CRYPT_OK &&
	       (err = ltc_mp.mulmod(r1, r2, mod->m->cint_data, (*r)->cint_data)) == CRYPT_OK);
	if (r1 != NULL) ltc_mp.deinit(r1);
	if (r2 != NULL) ltc_mp.deinit(r2);
	return(err == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static char *
ltc_mod_mod(mod_t *mod, cint_t *a, cint_t **r)
{
	int err;

	return((err = ltc_int_new(r)) == CRYPT_OK && (err = ltc_mp.mpdiv(a->cint_data, mod->m->cint_data, NULL, (*r)->cint_data)) == CRYPT_OK ? NULL: ltc_to_crypt_error(err));
}

static void
ltc_mod_free(mod_t *mod)
{
	/* nothing to do */
}

static mod_func_t modFuncs = {
	ltc_mod_add,
	ltc_mod_inv,
	ltc_mod_sub,
	ltc_mod_mul,
	ltc_mod_square,
	ltc_mod_mulinv,
	ltc_mod_exp,
	ltc_mod_exp2,
	ltc_mod_mod,
	ltc_mod_free,
};

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord gCryptModuleTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"arith.module",
	FskInstrumentationOffset(mod_t),
	NULL,
	0,
	NULL,
	cryptInstrumentFormat,
};
#endif

void
xs_mod_init(xsMachine *the)
{
	mod_t *mod;
//	UInt32 options = xsToInteger(xsArgc) > 0 && xsTypeOf(xsArg(0)) == xsIntegerType ? xsToInteger(xsArg(0)): 0;	/* LTC does not suport options */
	FskErr ferr;
	XS_FNAME("Arith.module._init");

	if ((ferr = FskMemPtrNew(sizeof(mod_t), (FskMemPtr *)&mod)) != kFskErrNone)
		cryptThrowFSK(ferr);

	mod->_mod_code = &modFuncs;

	xsResult = xsGet(xsThis, xsID("z"));	/* must exist */
	if (!xsIsInstanceOf(xsResult, xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("z"))) || (mod->z = xsGetHostData(xsResult)) == NULL)
		cryptThrow("kCryptTypeError");

	xsResult = xsGet(xsThis, xsID("m"));	/* must exist */
	if (!xsIsInstanceOf(xsResult, mod->z->z_protoInteger) || (mod->m = xsGetHostData(xsResult)) == NULL)
		nanError();

	FskInstrumentedItemNew(mod, "module", &gCryptModuleTypeInstrumentation);
	xsSetHostData(xsThis, mod);
}
