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


/*
 * integer object operations
 */

void
xs_integer_destructor(void *data)
{
	XS_FNAME("Arith.integer.destructor");

	if (data) {
		cint_t *ai = data;
		FskInstrumentedItemDispose(ai);
		(*ai->cint_free)(ai);
		FskMemPtrDispose(data);
	}
}

void
xs_integer_free(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	XS_FNAME("Arith.integer.free");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	(*ai->cint_free)(ai);
}

void
xs_integer_setChunk(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	unsigned char *data;
	xsIntegerValue size;
	int signess = xsToInteger(xsArgc) > 1 && xsToBoolean(xsArg(1));
	int neg = 0;
	char *err;
	XS_FNAME("Arith.integer.setChunk");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	getChunkData(the, &xsArg(0), (void **)&data, &size);
	if (size == 0) {
		cint_setNaN(ai);
		return;
	}
	if (signess) {
		neg = data[0];
		data++;
		--size;
	}
	err = (*ai->cint_os2i)(ai, data, size);
	if (err)
		cryptThrow(err);
	if (neg)
		(*ai->cint_neg)(ai);
}

void
xs_integer_toChunk(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	UInt32 minBytes = xsToInteger(xsArgc) > 0 ? xsToInteger(xsArg(0)): 0;
	Boolean signess = xsToInteger(xsArgc) > 1 && xsToBoolean(xsArg(1));
	UInt32 nbytes, chunkSize;
	SInt32 n;
	unsigned char *p;
	char *err;
	XS_FNAME("Arith.integer.toChunk");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	if (cint_isNaN(ai)) {
		xsResult = xsUndefined;
		return;
	}
	nbytes = (*ai->cint_sizeOf)(ai);
	chunkSize = MAX(nbytes, minBytes);
	if (signess)
		chunkSize++;
	xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(chunkSize));
	p = xsGetHostData(xsResult);
	/* all clear in case i2os won't fill out "nbytes" -- this would happen in libtom when the number is zero */
	FskMemSet(p, 0, chunkSize);
	if (signess)
		*p++ = (unsigned char)(*ai->cint_sign)(ai);
	/* skip the prepended filler */
	if ((n = minBytes - nbytes) > 0)
		p += n;
	err = (*ai->cint_i2os)(ai, p, nbytes);
	if (err)
		cryptThrow(err);
}

void
xs_integer_setString(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	char *digits = xsToString(xsArg(0));	/* never use the XS functions while accessing this */
	int neg = 0;
	int radix;
	char *err;
	XS_FNAME("Arith.integer.setString");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	if (*digits == '-') {
		neg = 1;
		digits++;
	}
	else if (*digits == '+')
		digits++;
	if (*digits == '0') {
		if (*(digits+1) == 'x' || *(digits+1) == 'X') {
			radix = 16;
			digits += 2;
		}
		else {
			radix = 8;
			digits++;
		}
	}
	else
		radix = 10;
	err = (*ai->cint_str2i)(ai, digits, radix);
	if (err)
		cryptThrow(err);
	if (neg)
		(*ai->cint_neg)(ai);
}

void
xs_integer_toString(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);
	int radix =  ac > 0 ? xsToInteger(xsArg(0)): 10;
	int prefix = ac > 1 ? xsToInteger(xsArg(1)): 0;
	int usize, n;
	char *bp, *p;
	FskErr fskerr;
	char *err;
	XS_FNAME("Arith.integer.toString");
#define NBITS(n)	(n < 4 ? 1: n < 8 ? 2: n < 16 ? 3: n < 32 ? 4: 5)

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	if (cint_isNaN(ai)) {
		xsResult = xsString("NaN");
		return;
	}
	if (radix < 2 || radix > 32)
		cryptThrow("kCryptRangeError");
	/* estimate the output string size */
	usize = (*ai->cint_sizeOf)(ai);
	n = (usize * 8) / NBITS(radix);	/* quite inaccurate, but better than shortage */
	if (n < prefix) n = prefix;
	n += 2;	/* for "+-" sign + '\0' */
	if ((fskerr = FskMemPtrNew(n, (FskMemPtr *)&bp)) != kFskErrNone)
		cryptThrowFSK(fskerr);
	err = (*ai->cint_i2str)(ai, bp, n, radix);
	if (err)
		cryptThrow(err);
	p = *bp == '-' || *bp == '+' ? bp + 1 : bp;
	if ((n = FskStrLen(p)) < prefix) {
		FskMemMove(p + (prefix - n), p, n + 1);	// move the whole string to the right including '\0'
		FskMemSet(p, '0', prefix - n);
	}
	xsResult = xsString(bp);
	FskMemPtrDispose(bp);
}

void
xs_integer_setNumber(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	char *err;
	XS_FNAME("Arith.intger.setNumber");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	switch (xsTypeOf(xsArg(0))) {
#if !TARGET_OS_WM
	case xsNumberType: {
		double d = xsToNumber(xsArg(0));
		FskUInt64 ll;
		if (d < 0)
			ll = -d;
		else
			ll = d;
#if TARGET_RT_LITTLE_ENDIAN
		ll = ((ll << 56) | ((ll << 40) & 0xff000000000000ULL) | ((ll << 24) & 0xff0000000000ULL) | ((ll << 8) & 0xff00000000ULL) |
		      (ll >> 56) | ((ll >> 40) & 0xff00ULL) | ((ll >> 24) & 0xff0000ULL) | ((ll >> 8) & 0xff000000ULL));
#endif
		err = (*ai->cint_os2i)(ai, (unsigned char *)&ll, sizeof(ll));
		if (err)
			cryptThrow(err);
		break;
	}
#endif /* TARGET_OS_WM */
	case xsIntegerType: {
		xsIntegerValue v = xsToInteger(xsArg(0));
		err = (*ai->cint_num2i)(ai, v);
		if (err)
			cryptThrow(err);
		break;
	}
	default:
		cryptThrow("kCryptTypeError");
		break;
	}
}

void
xs_integer_toNumber(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	long v;
	char *err;
	XS_FNAME("Arith.integer.toNumber");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	if (cint_isNaN(ai)) {
		xsResult = xsUndefined;	/* how can NaN be returned? */
		return;
	}
	err = (*ai->cint_i2num)(ai, &v);
	if (err)
		cryptThrow(err);
	xsResult = xsInteger((xsIntegerValue)v);
}

void
xs_integer_setInteger(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	char *err;
	XS_FNAME("Arith.integer.setInteger");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	if (xsToInteger(xsArgc) >= 1 || xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("integer")))) {
		cint_t *src = xsGetHostData(xsArg(0));
		if (cint_isNaN(src)) {
			cint_setNaN(ai);
			return;
		}
		err = (*ai->cint_copy)(ai, src);
		if (err)
			cryptThrow(err);
	}
	else
		cryptThrow("kCryptParameterError");
}

void
xs_integer_setNaN(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	XS_FNAME("Arith.integer.setNaN");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	cint_setNaN(ai);
}

void
xs_integer_negate(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	XS_FNAME("Arith.integer.negate");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	if (cint_isNaN(ai))
		nanError();
	(*ai->cint_neg)(ai);
}

void
xs_integer_isZero(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	XS_FNAME("Arith.integer.isZero");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	if (cint_isNaN(ai))
		nanError();
	xsResult = xsBoolean((*ai->cint_isZero)(ai));
}

void
xs_integer_isNaN(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	XS_FNAME("Arith.integer.isNaN");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	xsResult = xsBoolean(cint_isNaN(ai));
}

void
xs_integer_comp(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	cint_t *o;
	XS_FNAME("Arith.integer.comp");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	if (xsToInteger(xsArgc) < 1 || !xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("integer"))))
		cryptThrow("kCryptParameterError");
	o = xsGetHostData(xsArg(0));
	if (cint_isNaN(ai) || cint_isNaN(o))
		nanError();
	xsResult = xsInteger((*ai->cint_comp)(ai, o));
}

void
xs_integer_sign(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	XS_FNAME("Arith.integer.sign");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	if (cint_isNaN(ai))
		nanError();
	xsResult = xsBoolean((*ai->cint_sign)(ai));
}

void
xs_integer_sizeof(xsMachine *the)
{
	cint_t *ai = xsGetHostData(xsThis);
	XS_FNAME("Arith.integer.sizeof");

	FskInstrumentedItemSendMessageNormal(ai, kCryptInstrMsg, NULL);
	if (cint_isNaN(ai))
		nanError();
	xsResult = xsInteger((*ai->cint_sizeOf)(ai));
}




/*
 * arithmetics on Z
 */

#define getIntegerData(x)	(xsIsInstanceOf(x, z->z_protoInteger) ? xsGetHostData(x): NULL)

static void
z_call1(xsMachine *the, z_t *z, z_f1 f)
{
	cint_t *a = getIntegerData(xsArg(0));
	cint_t *r = NULL;
	char *err;

	if (a == NULL)
		nanError();
	err = (*f)(z, a, &r);
	if (err)
		cryptThrow(err);
	xsResult = xsNewInstanceOf(z->z_protoInteger);
	xsSetHostData(xsResult, r);
}

static void
z_call2(xsMachine *the, z_t *z, z_f2 f)
{
	cint_t *a = getIntegerData(xsArg(0));
	cint_t *b = getIntegerData(xsArg(1));
	cint_t *r = NULL;
	char *err;

	if (a == NULL || b == NULL)
		nanError();
	err = (*f)(z, a, b, &r);
	if (err)
		cryptThrow(err);
	xsResult = xsNewInstanceOf(z->z_protoInteger);
	xsSetHostData(xsResult, r);
}

static void
z_call2d(xsMachine *the, z_t *z, z_f2d f)
{
	cint_t *a = getIntegerData(xsArg(0));
	xsIntegerValue b = xsToInteger(xsArg(1));
	cint_t *r = NULL;
	char *err;

	if (a == NULL)
		nanError();
	err = (*f)(z, a, (int)b, &r);
	if (err)
		cryptThrow(err);
	xsResult = xsNewInstanceOf(z->z_protoInteger);
	xsSetHostData(xsResult, r);
}

static void
z_call3(xsMachine *the, z_t *z, z_f3 f)
{
	cint_t *a = getIntegerData(xsArg(0));
	cint_t *b = getIntegerData(xsArg(1));
	cint_t *r = NULL, *m = NULL;
	char *err;

	xsVars(2);
	if (a == NULL || b == NULL)
		nanError();
	err = (*f)(z, a, b, &r, &m);
	if (err)
		cryptThrow(err);
	xsVar(0) = xsNewInstanceOf(z->z_protoInteger);
	xsVar(1) = xsNewInstanceOf(z->z_protoInteger);
	xsSetHostData(xsVar(0), r);
	xsSetHostData(xsVar(1), m);
	xsResult = xsNew2(xsGet(xsGlobal, xsID("Arith")), xsID("Pair"), xsVar(0), xsVar(1));
}

void
xs_z_destructor(void *data)
{
	XS_FNAME("Arith.z.destructor");

	if (data) {
		z_t *z = data;
		FskInstrumentedItemDispose(z);
		(*z->z_free)(z);
		FskMemPtrDispose(data);
	}
}

void
xs_z_add(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.add");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call2(the, z, z->z_add);
}

void
xs_z_sub(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.sub");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call2(the, z, z->z_sub);
}

void
xs_z_mul(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.mul");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call2(the, z, z->z_mul);
}

void
xs_z_div2(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.div2");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call3(the, z, z->z_div);
}

void
xs_z_div(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.div");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call3(the, z, z->z_div);
	xsResult = xsGet(xsResult, xsID("q"));
}

void
xs_z_mod(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.mod");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call3(the, z, z->z_div);
	xsResult = xsGet(xsResult, xsID("r"));
}

void
xs_z_square(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.square");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call1(the, z, z->z_square);
}

void
xs_z_xor(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.xor");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call2(the, z, z->z_xor);
}

void
xs_z_or(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.or");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call2(the, z, z->z_or);
}

void
xs_z_and(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.and");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call2(the, z, z->z_and);
}

void
xs_z_lsl(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.lsl");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call2d(the, z, z->z_lsl);
}

void
xs_z_lsr(xsMachine *the)
{
	z_t *z = xsGetHostData(xsThis);
	XS_FNAME("Arith.z.lsr");

	FskInstrumentedItemSendMessageNormal(z, kCryptInstrMsg, NULL);
	z_call2d(the, z, z->z_lsr);
}


/*
 * Modular arithmetics
 */

#undef getIntegerData
#define getIntegerData(x)	(xsIsInstanceOf(x, mod->z->z_protoInteger) ? xsGetHostData(x): NULL)

static void
mod_call1(xsMachine *the, mod_t *mod, mod_f1 f)
{
	cint_t *a = getIntegerData(xsArg(0));
	cint_t *r = NULL;
	char *err;

	if (a == NULL)
		nanError();
	err = (*f)(mod, a, &r);
	if (err)
		cryptThrow(err);
	xsResult = xsNewInstanceOf(mod->z->z_protoInteger);
	xsSetHostData(xsResult, r);
}

static void
mod_call2(xsMachine *the, mod_t *mod, mod_f2 f)
{
	cint_t *a = getIntegerData(xsArg(0));
	cint_t *b = getIntegerData(xsArg(1));
	cint_t *r = NULL;
	char *err;

	if (a == NULL || b == NULL)
		nanError();
	err = (*f)(mod, a, b, &r);
	if (err)
		cryptThrow(err);
	xsResult = xsNewInstanceOf(mod->z->z_protoInteger);
	xsSetHostData(xsResult, r);
}

static void
mod_call4(xsMachine *the, mod_t *mod, mod_f4 f)
{
	cint_t *a1 = getIntegerData(xsArg(0));
	cint_t *e1 = getIntegerData(xsArg(1));
	cint_t *a2 = getIntegerData(xsArg(2));
	cint_t *e2 = getIntegerData(xsArg(3));
	cint_t *r = NULL;
	char *err;

	if (a1 == NULL || e1 == NULL || a2 == NULL || e2 == NULL)
		nanError();
	err = (*f)(mod, a1, e1, a2, e2, &r);
	if (err)
		cryptThrow(err);
	xsResult = xsNewInstanceOf(mod->z->z_protoInteger);
	xsSetHostData(xsResult, r);
}

void
xs_mod_destructor(void *data)
{
	XS_FNAME("Arith.mod.destructor");

	if (data != NULL) {
		mod_t *mod = data;
		FskInstrumentedItemDispose(mod);
		(*mod->mod_free)(mod);
		/* leave m and z as-is */
		FskMemPtrDispose(data);
	}
}

void
xs_mod_add(xsMachine *the)
{
	mod_t *mod = xsGetHostData(xsThis);
	XS_FNAME("Arith.mod.add");

	FskInstrumentedItemSendMessageNormal(mod, kCryptInstrMsg, NULL);
	mod_call2(the, mod, mod->mod_add);
}

void
xs_mod_inv(xsMachine *the)
{
	mod_t *mod = xsGetHostData(xsThis);
	XS_FNAME("Arith.mod.inv");

	FskInstrumentedItemSendMessageNormal(mod, kCryptInstrMsg, NULL);
	mod_call1(the, mod, mod->mod_inv);
}

void
xs_mod_sub(xsMachine *the)
{
	mod_t *mod = xsGetHostData(xsThis);
	XS_FNAME("Arith.mod.sub");

	FskInstrumentedItemSendMessageNormal(mod, kCryptInstrMsg, NULL);
	mod_call2(the, mod, mod->mod_sub);
}

void
xs_mod_mul(xsMachine *the)
{
	mod_t *mod = xsGetHostData(xsThis);
	XS_FNAME("Arith.mod.mul");

	FskInstrumentedItemSendMessageNormal(mod, kCryptInstrMsg, NULL);
	mod_call2(the, mod, mod->mod_mul);
}

void
xs_mod_square(xsMachine *the)
{
	mod_t *mod = xsGetHostData(xsThis);
	XS_FNAME("Arith.mod.square");

	FskInstrumentedItemSendMessageNormal(mod, kCryptInstrMsg, NULL);
	mod_call1(the, mod, mod->mod_square);
}

void
xs_mod_mulinv(xsMachine *the)
{
	mod_t *mod = xsGetHostData(xsThis);
	XS_FNAME("Arith.mod.mulinv");

	FskInstrumentedItemSendMessageNormal(mod, kCryptInstrMsg, NULL);
	mod_call1(the, mod, mod->mod_mulinv);
}

void
xs_mod_exp(xsMachine *the)
{
	mod_t *mod = xsGetHostData(xsThis);
	XS_FNAME("Arith.mod.exp");

	FskInstrumentedItemSendMessageNormal(mod, kCryptInstrMsg, NULL);
	mod_call2(the, mod, mod->mod_exp);
}

void
xs_mod_exp2(xsMachine *the)
{
	mod_t *mod = xsGetHostData(xsThis);
	XS_FNAME("Arith.mod.exp2");

	FskInstrumentedItemSendMessageNormal(mod, kCryptInstrMsg, NULL);
	mod_call4(the, mod, mod->mod_exp2);
}

void
xs_mod_mod(xsMachine *the)
{
	mod_t *mod = xsGetHostData(xsThis);
	XS_FNAME("Arith.mod.mod");

	FskInstrumentedItemSendMessageNormal(mod, kCryptInstrMsg, NULL);
	mod_call1(the, mod, mod->mod_mod);
}
