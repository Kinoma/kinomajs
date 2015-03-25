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
/*
 * a simple implementation of EC(p) crypto
 *
 * @@ too slow because not optimized at all
 */

#include "cryptTypes.h"
#include "common.h"
#include "arith.h"

typedef struct {
	cint_t *x, *y;
	Boolean identity;
} gen_ecp_t;

static char *
gen_ec_int_new(cint_t *a, cint_t **r)
{
	return((*a->cint_newFrom)(a, r));
}

static void
gen_ec_int_dispose(cint_t *a)
{
	if (a != NULL) {
		(*a->cint_free)(a);
		FskInstrumentedItemDispose(a);
		FskMemPtrDispose(a);
	}
}

static Boolean
gen_ecpoint_identity(ecp_t *p)
{
	gen_ecp_t *gp = p->ecp_data;

	return(gp->identity);
}

static cint_t *
gen_ecpoint_getX(ecp_t *p)
{
	gen_ecp_t *gp = p->ecp_data;

	return(gp->x);
}

static cint_t *
gen_ecpoint_getY(ecp_t *p)
{
	gen_ecp_t *gp = p->ecp_data;

	return(gp->y);
}

static void
gen_ecpoint_setX(ecp_t *p, cint_t *x)
{
	gen_ecp_t *gp = p->ecp_data;

	(*gp->x->cint_copy)(gp->x, x);
	gp->identity = gp->x && !cint_isNaN(gp->x) && gp->y && !cint_isNaN(gp->y) && (*gp->x->cint_comp)(gp->x, gp->y) == 0;
}

static void
gen_ecpoint_setY(ecp_t *p, cint_t *y)
{
	gen_ecp_t *gp = p->ecp_data;

	(*gp->y->cint_copy)(gp->y, y);
	gp->identity = gp->x && !cint_isNaN(gp->x) && gp->y && !cint_isNaN(gp->y) && (*gp->x->cint_comp)(gp->x, gp->y) == 0;
}

static void
gen_ecpoint_free(ecp_t *p)
{
	gen_ecp_t *gp = p->ecp_data;

	if (gp != NULL) {
		gen_ec_int_dispose(gp->x);
		gen_ec_int_dispose(gp->y);
		FskMemPtrDispose(p->ecp_data);
		p->ecp_data = NULL;
	}
}

static ecp_func_t ecpFuncs = {
	gen_ecpoint_identity,
	gen_ecpoint_getX,
	gen_ecpoint_getY,
	gen_ecpoint_setX,
	gen_ecpoint_setY,
	gen_ecpoint_free,
};

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord gCryptEcpointTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"arith.ecpoint",
	FskInstrumentationOffset(ecp_t),
	NULL,
	0,
	NULL,
	cryptInstrumentFormat,
};
#endif

static char *
gen_ecpoint_new(ecp_t **r)
{
	ecp_t *p;
	gen_ecp_t *gp;
	FskErr ferr;

	if ((ferr = FskMemPtrNew(sizeof(ecp_t), (FskMemPtr *)r)) != kFskErrNone)
		return("kCryptMemoryFull");
	p = *r;
	if ((ferr = FskMemPtrNew(sizeof(gen_ecp_t), (FskMemPtr *)&p->ecp_data)) != kFskErrNone) {
		FskMemPtrDispose(p);
		return("kCryptMemoryFull");
	}
	p->_ecp_code = &ecpFuncs;
	gp = p->ecp_data;
	gp->x = gp->y = NULL;
	gp->identity = false;
	FskInstrumentedItemNew(p, "ecpoint", &gCryptEcpointTypeInstrumentation);
	return(NULL);
}

static char *
gen_ecpoint_copy(ecp_t *src, ecp_t **dst)
{
	gen_ecp_t *ga = src->ecp_data, *gb;
	char *err;

	err = gen_ecpoint_new(dst);
	if (err)
		return(err);
	gb = (*dst)->ecp_data;
	if (ga->identity) {
		gb->identity = true;
		return(NULL);
	}
	(void)((err = gen_ec_int_new(ga->x, &gb->x)) || (err = gen_ec_int_new(ga->y, &gb->y)));
	return(err);
}

static void
gen_ecpoint_dispose(ecp_t *p)
{
	if (p != NULL) {
		FskInstrumentedItemDispose(p);
		gen_ecpoint_free(p);
		FskMemPtrDispose(p);
	}
}

void
xs_ecpoint_init(xsMachine *the)
{
	ecp_t *p;
	gen_ecp_t *gp;
	FskErr ferr;

	if ((ferr = FskMemPtrNew(sizeof(ecp_t), (FskMemPtr *)&p)) != kFskErrNone)
		cryptThrowFSK(ferr);
	if ((ferr = FskMemPtrNew(sizeof(gen_ecp_t), (FskMemPtr *)&p->ecp_data)) != kFskErrNone) {
		FskMemPtrDispose(p);
		cryptThrowFSK(ferr);
	}
	p->_ecp_code = &ecpFuncs;
	gp = p->ecp_data;
	xsResult = xsNew1(xsGet(xsGlobal, xsID("Arith")), xsID("Integer"), xsArg(0));
	gen_ec_int_new(xsGetHostData(xsResult), &gp->x);
	xsResult = xsNew1(xsGet(xsGlobal, xsID("Arith")), xsID("Integer"), xsArg(1));
	gen_ec_int_new(xsGetHostData(xsResult), &gp->y);
	gp->identity = !cint_isNaN(gp->x) && !cint_isNaN(gp->y) && (*gp->x->cint_comp)(gp->x, gp->y) == 0;
	FskInstrumentedItemNew(p, "ecpoint", &gCryptEcpointTypeInstrumentation);
	xsSetHostData(xsThis, p);
}


/*
 * EC(p)
 */

static char *
gen_ecpoint_new_identity(ecp_t **r)
{
	char *err;

	err = gen_ecpoint_new(r);
	if (!err) {
		gen_ecp_t *gp = (*r)->ecp_data;
		gp->identity = true;
	}
	return(err);
}

static char *
gen_ec_inv(ec_t *ec, ecp_t *a, ecp_t **r)
{
	gen_ecp_t *ga, *gr;
	char *err;

	err = gen_ecpoint_new(r);
	if (err)
		return(err);
	ga = a->ecp_data;
	gr = (*r)->ecp_data;
	if (ga->identity)
		gr->identity = true;
	else
		(void)((err = (*ec->mod->mod_inv)(ec->mod, ga->y, &gr->y)) || (err = gen_ec_int_new(ga->x, &gr->x)));
	return(err);
}

static char *
gen_ec_double(ec_t *ec, ecp_t *a, ecp_t **r)
{
	gen_ecp_t *ga = a->ecp_data, *gr;
	cint_t *t1 = NULL, *t2 = NULL, *t3 = NULL, *t4 = NULL, *t5 = NULL, *t6 = NULL, *lambda = NULL, *rx = NULL, *ry = NULL;
	char *err;

	if (ga->identity || (*ga->x->cint_isZero)(ga->y))
		return(gen_ecpoint_new_identity(r));

	/* lambda = (3*x^2 + a) / 2*y */
	(void)((err = (*ec->mod->mod_square)(ec->mod, ga->x, &t1)) ||		/* t1 = x^2 */
	       (err = (*ec->mod->mod_add)(ec->mod, t1, t1, &t2)) ||	/* t2 = t1 + t1 = t1 * 2*/
	       (err = (*ec->mod->mod_add)(ec->mod, t2, t1, &t3)) ||	/* t3 = t1 + t2 = t1 * 3 */
	       (err = (*ec->mod->mod_add)(ec->mod, t3, ec->a, &t4)) ||
	       (err = (*ec->mod->mod_add)(ec->mod, ga->y, ga->y, &t5)) ||
	       (err = (*ec->mod->mod_mulinv)(ec->mod, t5, &t6)) ||
	       (err = (*ec->mod->mod_mul)(ec->mod, t4, t6, &lambda)));
	gen_ec_int_dispose(t1); t1 = NULL;
	gen_ec_int_dispose(t2); t2 = NULL;
	gen_ec_int_dispose(t3); t3 = NULL;
	gen_ec_int_dispose(t4); t4 = NULL;
	gen_ec_int_dispose(t5); t5 = NULL;
	gen_ec_int_dispose(t6); t6 = NULL;
	if (err) goto bail;

	/* rx = lambda^2 - 2*x */
	(void)((err = (*ec->mod->mod_square)(ec->mod, lambda, &t1)) ||		/* t1 = lambda^2 */
	       (err = (*ec->mod->mod_sub)(ec->mod, t1, ga->x, &t2)) ||	/* t2 = t1 - x */
	       (err = (*ec->mod->mod_sub)(ec->mod, t2, ga->x, &rx)));	/* rx = t2 - x = t1 - 2*x */
	gen_ec_int_dispose(t1); t1 = NULL;
	gen_ec_int_dispose(t2); t2 = NULL;
	if (err) goto bail;

	/* ry = lambda * (x - rx) - y */
	(void)((err = (*ec->mod->mod_sub)(ec->mod, ga->x, rx, &t1)) ||
	       (err = (*ec->mod->mod_mul)(ec->mod, lambda, t1, &t2)) ||
	       (err = (*ec->mod->mod_sub)(ec->mod, t2, ga->y, &ry)));
	gen_ec_int_dispose(t1); t1 = NULL;
	gen_ec_int_dispose(t2); t2 = NULL;
	if (err) goto bail;

	err = gen_ecpoint_new(r);
	if (err)
		goto bail;
	gr = (*r)->ecp_data;
	gr->x = rx;
	gr->y = ry;

bail:
	gen_ec_int_dispose(lambda);
	if (err) {
		gen_ec_int_dispose(rx);
		gen_ec_int_dispose(ry);
	}
	return(err);
}

static char *
gen_ec_add(ec_t *ec, ecp_t *a, ecp_t *b, ecp_t **r)
{
	gen_ecp_t *ga = a->ecp_data, *gb = b->ecp_data, *gr;
	cint_t *t1 = NULL, *t2 = NULL, *t3 = NULL, *lambda = NULL, *rx = NULL, *ry = NULL;
	char *err;

	/* add zero? */
	if (ga->identity)
		return(gen_ecpoint_copy(b, r));
	if (gb->identity)
		return(gen_ecpoint_copy(a, r));

	if ((*ga->x->cint_comp)(ga->x, gb->x) == 0) {
		/* a == b ? */
		if ((*ga->y->cint_comp)(ga->y, gb->y) == 0)
			return(gen_ec_double(ec, a, r));
		else
			return(gen_ecpoint_new_identity(r));
	}

	/* lambda = (y2 - y1) / (x2 - x1) */
	(void)((err = (*ec->mod->mod_sub)(ec->mod, gb->y, ga->y, &t1)) ||
	       (err = (*ec->mod->mod_sub)(ec->mod, gb->x, ga->x, &t2)) ||
	       (err = (*ec->mod->mod_mulinv)(ec->mod, t2, &t3)) ||
	       (err = (*ec->mod->mod_mul)(ec->mod, t1, t3, &lambda)));
	gen_ec_int_dispose(t1); t1 = NULL;
	gen_ec_int_dispose(t2); t2 = NULL;
	gen_ec_int_dispose(t3); t3 = NULL;
	if (err) goto bail;

	/* rx = lambda^2 - x1 - x2 */
	(void)((err = (*ec->mod->mod_square)(ec->mod, lambda, &t1)) ||
	       (err = (*ec->mod->mod_sub)(ec->mod, t1, ga->x, &t2)) ||
	       (err = (*ec->mod->mod_sub)(ec->mod, t2, gb->x, &rx)));
	gen_ec_int_dispose(t1); t1 = NULL;
	gen_ec_int_dispose(t2); t2 = NULL;
	if (err) goto bail;

	/* ry = lambda * (x1 - rx) - y1 */
	(void)((err = (*ec->mod->mod_sub)(ec->mod, ga->x, rx, &t1)) ||
	       (err = (*ec->mod->mod_mul)(ec->mod, lambda, t1, &t2)) ||
	       (err = (*ec->mod->mod_sub)(ec->mod, t2, ga->y, &ry)));
	gen_ec_int_dispose(t1); t1 = NULL;
	gen_ec_int_dispose(t2); t2 = NULL;
	if (err) goto bail;

	err = gen_ecpoint_new(r);
	if (err)
		goto bail;
	gr = (*r)->ecp_data;
	gr->x = rx;
	gr->y = ry;

bail:
	gen_ec_int_dispose(lambda);
	if (err) {
		gen_ec_int_dispose(rx);
		gen_ec_int_dispose(ry);
	}
	return(err);
}

static int
gen_ec_numbits(unsigned char *os, int nbytes)
{
	int i;
	unsigned char m = os[0];

	/* find out the first non-zero bit in the MSB */
	for (i = 0; i < 8 && !(m & (1 << (7 - i))); i++)
		;
	return(nbytes * 8 - i);
}

static int
gen_ec_isset(unsigned char *os, int i, int nbytes)
{
	unsigned char b = os[nbytes - 1 - (i / 8)];
	return((b & (1 << (i % 8))) != 0);
}

#if 0
static void
gen_ec_int_print(cint_t *a)
{
	int i;
	unsigned char *os;
	UInt32 nbytes;
	char *err;

	if ((*a->cint_sign)(a))
		fprintf(stderr, "-");
	nbytes = (*a->cint_sizeOf)(a);
	if (FskMemPtrNew(nbytes, &os) != kFskErrNone) {
		fprintf(stderr, "memory error!?");
		return;
	}
	if (err = (*a->cint_i2os)(a, os, nbytes)) {
		fprintf(stderr, "i2os: %s", err);
		return;
	}
	for (i = 0; i < nbytes; i++)
		fprintf(stderr, "%02x", os[i]);
	FskMemPtrDispose(os);
}

static void
gen_ec_print(ecp_t *p, int i, int b)
{
	fprintf(stderr, "%d (%d): ", i, b);
	if ((*p->ecp_identity)(p))
		fprintf(stderr, "zero");
	else {
		gen_ecp_t *gp = p->ecp_data;
		fprintf(stderr, "(");
		gen_ec_int_print(gp->x);
		fprintf(stderr, ", ");
		gen_ec_int_print(gp->y);
		fprintf(stderr, ")");
	}
	fprintf(stderr, "\n");
}
#endif

static char *
gen_ec_mul(ec_t *ec, ecp_t *a, cint_t *k, ecp_t **r)
{
	int i;
	UInt32 nbytes;
	unsigned char *os;
	ecp_t *t = NULL, *t1 = NULL, *t2 = NULL;
	char *err;

	nbytes = (*k->cint_sizeOf)(k);
	if (FskMemPtrNew(nbytes, &os) != kFskErrNone)
		return("kCryptMemoryFull");
	err = (*k->cint_i2os)(k, os, nbytes);
	if (err) return(err);

	err = gen_ecpoint_new_identity(&t);
	if (err) goto bail;
	for (i = gen_ec_numbits(os, nbytes); --i >= 0;) {
		err = gen_ec_double(ec, t, &t1);
		if (err) goto bail;
		if (gen_ec_isset(os, i, nbytes)) {
			err = gen_ec_add(ec, t1, a, &t2);
			if (err) goto bail;
			gen_ecpoint_dispose(t1);
			t1 = t2;
			t2 = NULL;
		}
		gen_ecpoint_dispose(t);
		t = t1;
		t1 = NULL;
	}
	*r = t;
	t = NULL;
bail:
	if (t2 != NULL) gen_ecpoint_dispose(t2);
	if (t1 != NULL) gen_ecpoint_dispose(t1);
	if (t != NULL) gen_ecpoint_dispose(t);
	FskMemPtrDispose(os);
	return(err);
}

static char *
gen_ec_mul2(ec_t *ec, ecp_t *a1, cint_t *k1, ecp_t *a2, cint_t *k2, ecp_t **r)
{
	int i, j;
	ecp_t *G[4];
	int k1size, k2size;
	unsigned char *os1 = NULL, *os2 = NULL;
	UInt32 nbytes1, nbytes2;
	ecp_t *t = NULL, *t1 = NULL, *t2 = NULL;
	char *err;
#define isset_k1(i)	(i < k1size ? gen_ec_isset(os1, i, nbytes1): 0)
#define isset_k2(i)	(i < k2size ? gen_ec_isset(os2, i, nbytes2): 0)

	G[1] = a1;
	G[2] = a2;
	err = gen_ec_add(ec, a1, a2, &G[3]);
	if (err) return(err);

	nbytes1 = (*k1->cint_sizeOf)(k1);
	if (FskMemPtrNew(nbytes1, &os1) != kFskErrNone) {
		err = "kCryptMemoryFull";
		goto bail;
	}
	err = (*k1->cint_i2os)(k1, os1, nbytes1);
	if (err) goto bail;
	k1size = gen_ec_numbits(os1, nbytes1);

	nbytes2 = (*k2->cint_sizeOf)(k2);
	if (FskMemPtrNew(nbytes2, &os2) != kFskErrNone) {
		err = "kCryptMemoryFull";
		goto bail;
	}
	err = (*k2->cint_i2os)(k2, os2, nbytes2);
	if (err) goto bail;
	k2size = gen_ec_numbits(os2, nbytes2);

	err = gen_ecpoint_new_identity(&t);
	if (err) goto bail;
	for (i = MAX(k1size, k2size); --i >= 0;) {
		err = gen_ec_double(ec, t, &t1);
		if (err) goto bail;
		j = gen_ec_isset(os1, i, nbytes1) | (gen_ec_isset(os2, i, nbytes2) << 1);
		if (j != 0) {
			err = gen_ec_add(ec, t1, G[j], &t2);
			if (err) goto bail;
			gen_ecpoint_dispose(t1);
			t1 = t2;
			t2 = NULL;
		}
		gen_ecpoint_dispose(t);
		t = t1;
		t1 = NULL;
	}
	*r = t;
	t = NULL;
bail:
	if (t2 != NULL) gen_ecpoint_dispose(t2);
	if (t1 != NULL) gen_ecpoint_dispose(t1);
	if (t != NULL) gen_ecpoint_dispose(t);
	gen_ecpoint_dispose(G[3]);
	if (os1 != NULL) FskMemPtrDispose(os1);
	if (os2 != NULL) FskMemPtrDispose(os2);
	return(err);
}

static void
gen_ec_free(ec_t *ec)
{
}

static ec_func_t ecFuncs = {
	gen_ec_inv,
	gen_ec_add,
	gen_ec_mul,
	gen_ec_mul2,
	gen_ec_free,
};

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord gCryptECTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"arith.ec",
	FskInstrumentationOffset(ec_t),
	NULL,
	0,
	NULL,
	cryptInstrumentFormat,
};
#endif

void
xs_ec_init(xsMachine *the)
{
	ec_t *ec;
	FskErr ferr;

	if ((ferr = FskMemPtrNew(sizeof(ec_t), (FskMemPtr *)&ec)) != kFskErrNone)
		cryptThrowFSK(ferr);

	xsResult = xsGet(xsThis, xsID("m"));
	if (!xsIsInstanceOf(xsResult, xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("module"))) || (ec->mod = xsGetHostData(xsResult)) == NULL)
		cryptThrow("kCryptTypeError");

	xsResult = xsGet(xsThis, xsID("a"));
	if (!xsIsInstanceOf(xsResult, xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("integer"))) || (ec->a = xsGetHostData(xsResult)) == NULL)
		nanError();

	xsResult = xsGet(xsThis, xsID("b"));
	if (!xsIsInstanceOf(xsResult, xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("integer"))) || (ec->b = xsGetHostData(xsResult)) == NULL)
		nanError();

	ec->_ec_code = &ecFuncs;
	ec->ec_protoECPoint = xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("ecpoint"));

	FskInstrumentedItemNew(ec, "ec", &gCryptECTypeInstrumentation);
	xsSetHostData(xsThis, ec);
}

static Boolean
ecp_isNaN(ecp_t *p)
{
	return(p->ecp_data == NULL);
}

void
xs_ecpoint_destructor(void *data)
{
	XS_FNAME("Arith.ecpoint.destructor");

	if (data != NULL) {
		ecp_t *p = data;
		FskInstrumentedItemDispose(p);
		(*p->ecp_free)(p);
		FskMemPtrDispose(data);
	}
}

void
xs_ecpoint_free(xsMachine *the)
{
	ecp_t *p = xsGetHostData(xsThis);
	XS_FNAME("Airth.ecpoint.free");

	FskInstrumentedItemSendMessageNormal(p, kCryptInstrMsg, NULL);
	(*p->ecp_free)(p);
}

void
xs_ecpoint_getIdentity(xsMachine *the)
{
	ecp_t *p = xsGetHostData(xsThis);
	XS_FNAME("Airth.ecpoint.getIdentity");

	FskInstrumentedItemSendMessageNormal(p, kCryptInstrMsg, NULL);
	if (ecp_isNaN(p))
		nanError();
	xsResult = xsBoolean((*p->ecp_identity)(p));
}

void
xs_ecpoint_getX(xsMachine *the)
{
	ecp_t *p = xsGetHostData(xsThis);
	cint_t *r;
	char *err;
	XS_FNAME("Arith.ecpoint.getX");

	FskInstrumentedItemSendMessageNormal(p, kCryptInstrMsg, NULL);
	if (ecp_isNaN(p))
		nanError();
	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("integer")));
	if ((err = gen_ec_int_new((*p->ecp_getX)(p), &r)) != NULL)
		cryptThrow(err);
	xsSetHostData(xsResult, r);
}

void
xs_ecpoint_getY(xsMachine *the)
{
	ecp_t *p = xsGetHostData(xsThis);
	cint_t *r;
	char *err;
	XS_FNAME("Arith.ecpoint.getY");

	FskInstrumentedItemSendMessageNormal(p, kCryptInstrMsg, NULL);
	if (ecp_isNaN(p))
		nanError();
	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("integer")));
	if ((err = gen_ec_int_new((*p->ecp_getY)(p), &r)) != NULL)
		cryptThrow(err);
	xsSetHostData(xsResult, r);
}

void
xs_ecpoint_setX(xsMachine *the)
{
	ecp_t *p = xsGetHostData(xsThis);
	cint_t *x;
	XS_FNAME("Arith.ecpoint.setX");

	FskInstrumentedItemSendMessageNormal(p, kCryptInstrMsg, NULL);
	if (ecp_isNaN(p))
		nanError();
	if (xsToInteger(xsArgc) < 1 || !xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("integer"))))
		cryptThrow("kCryptParameterError");
	x = xsGetHostData(xsArg(0));
	if (cint_isNaN(x))
		nanError();
	(*p->ecp_setX)(p, x);
}

void
xs_ecpoint_setY(xsMachine *the)
{
	ecp_t *p = xsGetHostData(xsThis);
	cint_t *y;
	XS_FNAME("Arith.ecpoint.setY");

	FskInstrumentedItemSendMessageNormal(p, kCryptInstrMsg, NULL);
	if (ecp_isNaN(p))
		nanError();
	if (xsToInteger(xsArgc) < 1 || !xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID("Arith")), xsID("integer"))))
		cryptThrow("kCryptParameterError");
	y = xsGetHostData(xsArg(0));
	if (cint_isNaN(y))
		nanError();
	(*p->ecp_setY)(p, y);
}


/*
 * arithmetics on EC(p)
 */

#undef getIntegerData
#define getIntegerData(x)	(xsIsInstanceOf(x, ec->mod->z->z_protoInteger) ? xsGetHostData(x): NULL)
#define getECPoint(x)	(xsIsInstanceOf(x, ec->ec_protoECPoint) ? xsGetHostData(x): NULL)

static void
ec_call1(xsMachine *the, ec_t *ec, ec_f1 f)
{
	ecp_t *a = getECPoint(xsArg(0));
	ecp_t *r = NULL;
	char *err;

	if (a == NULL)
		nanError();
	err = (*f)(ec, a, &r);
	if (err)
		cryptThrow(err);
	xsResult = xsNewInstanceOf(ec->ec_protoECPoint);
	xsSetHostData(xsResult, r);
}

static void
ec_call2(xsMachine *the, ec_t *ec, ec_f2 f)
{
	ecp_t *a = getECPoint(xsArg(0));
	ecp_t *b = getECPoint(xsArg(1));
	ecp_t *r = NULL;
	char *err;

	if (a == NULL || b == NULL)
		nanError();
	err = (*f)(ec, a, b, &r);
	if (err)
		cryptThrow(err);
	xsResult = xsNewInstanceOf(ec->ec_protoECPoint);
	xsSetHostData(xsResult, r);
}

static void
ec_call2d(xsMachine *the, ec_t *ec, ec_f2d f)
{
	ecp_t *a = getECPoint(xsArg(0));
	cint_t *k = getIntegerData(xsArg(1));
	ecp_t *r = NULL;
	char *err;

	if (a == NULL || k == NULL)
		nanError();
	err = (*f)(ec, a, k, &r);
	if (err)
		cryptThrow(err);
	xsResult = xsNewInstanceOf(ec->ec_protoECPoint);
	xsSetHostData(xsResult, r);
}

static void
ec_call4d(xsMachine *the, ec_t *ec, ec_f4d f)
{
	ecp_t *a1 = getECPoint(xsArg(0));
	cint_t *k1 = getIntegerData(xsArg(1));
	ecp_t *a2 = getECPoint(xsArg(2));
	cint_t *k2 = getIntegerData(xsArg(3));
	ecp_t *r = NULL;
	char *err;

	if (a1 == NULL || k1 == NULL || a2 == NULL || k2 == NULL)
		nanError();
	err = (*f)(ec, a1, k1, a2, k2, &r);
	if (err)
		cryptThrow(err);
	xsResult = xsNewInstanceOf(ec->ec_protoECPoint);
	xsSetHostData(xsResult, r);
}

void
xs_ec_destructor(void *data)
{
	XS_FNAME("Crypt.ec.destructor");

	if (data != NULL) {
		ec_t *ec = data;
		FskInstrumentedItemDispose(ec);
		(*ec->ec_free)(ec);
		FskMemPtrDispose(data);
	}
}

void
xs_ec_inv(xsMachine *the)
{
	ec_t *ec = xsGetHostData(xsThis);
	XS_FNAME("Crypt.ec.inv");

	FskInstrumentedItemSendMessageNormal(ec, kCryptInstrMsg, NULL);
	ec_call1(the, ec, ec->ec_inv);
}

void
xs_ec_add(xsMachine *the)
{
	ec_t *ec = xsGetHostData(xsThis);
	XS_FNAME("Crypt.ec.add");

	FskInstrumentedItemSendMessageNormal(ec, kCryptInstrMsg, NULL);
	ec_call2(the, ec, ec->ec_add);
}

void
xs_ec_mul(xsMachine *the)
{
	ec_t *ec = xsGetHostData(xsThis);
	XS_FNAME("Crypt.ec.mul");

	FskInstrumentedItemSendMessageNormal(ec, kCryptInstrMsg, NULL);
	ec_call2d(the, ec, ec->ec_mul);
}

void
xs_ec_mul2(xsMachine *the)
{
	ec_t *ec = xsGetHostData(xsThis);
	XS_FNAME("Crypt.ec.mul2");

	FskInstrumentedItemSendMessageNormal(ec, kCryptInstrMsg, NULL);
	ec_call4d(the, ec, ec->ec_mul2);
}
