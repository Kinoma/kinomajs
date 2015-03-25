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

#define MOD_OPT_NOMONT	0x01
#define MOD_OPT_LR	0x02
#define MOD_OPT_PRECOMP	0x04

#define nanError()	cryptThrow("kCryptNaNError")


/*
 * Arith.integer host object
 */

struct cint;
typedef struct cint_func {
	Boolean (*sign)(struct cint *);
	Boolean (*isZero)(struct cint *);
	void (*neg)(struct cint *);
	int (*comp)(struct cint *, struct cint *);
	int (*sizeOf)(struct cint *);
	char *(*copy)(struct cint *, struct cint *src);
	char *(*i2os)(struct cint *, unsigned char *os, int size);
	char *(*os2i)(struct cint *, unsigned char *os, int size);
	char *(*i2str)(struct cint *, char *s, int size, int radix);
	char *(*str2i)(struct cint *, char *s, int radix);
	char *(*i2num)(struct cint *, long *n);
	char *(*num2i)(struct cint *, long n);
	char *(*newFrom)(struct cint *src, struct cint **dst);
	void (*free)(struct cint *);
} cint_func_t;

typedef struct cint {
	void *cint_data;
	cint_func_t *_cint_code;
#define cint_sign	_cint_code->sign
#define cint_isZero	_cint_code->isZero
#define cint_neg	_cint_code->neg
#define cint_comp	_cint_code->comp
#define cint_sizeOf	_cint_code->sizeOf
#define cint_copy	_cint_code->copy
#define cint_i2os	_cint_code->i2os
#define cint_os2i	_cint_code->os2i
#define cint_i2str	_cint_code->i2str
#define cint_str2i	_cint_code->str2i
#define cint_i2num	_cint_code->i2num
#define cint_num2i	_cint_code->num2i
#define cint_newFrom	_cint_code->newFrom
#define cint_free	_cint_code->free
	FskInstrumentedItemDeclaration
} cint_t;

#define cint_setNaN(ci)	(ci)->cint_free(ci)
#define cint_isNaN(ci)	((ci)->cint_data == NULL)


/*
 * Arith.ecpoint host object
 */

struct ecp;
typedef struct ecp_func {
	Boolean (*identity)(struct ecp *);
	cint_t *(*getX)(struct ecp *);
	cint_t *(*getY)(struct ecp *);
	void (*setX)(struct ecp *, cint_t *);
	void (*setY)(struct ecp *, cint_t *);
	void (*free)(struct ecp *);
} ecp_func_t;

typedef struct ecp {
	void *ecp_data;
	ecp_func_t *_ecp_code;
#define ecp_identity	_ecp_code->identity
#define ecp_getX	_ecp_code->getX
#define ecp_getY	_ecp_code->getY
#define ecp_setX	_ecp_code->setX
#define ecp_setY	_ecp_code->setY
#define ecp_free	_ecp_code->free
	FskInstrumentedItemDeclaration
} ecp_t;


/*
 * Arith.z host object
 */

struct z;
typedef char *(*z_f1)(struct z *z, cint_t *a, cint_t **r);
typedef char *(*z_f2)(struct z *z, cint_t *a, cint_t *b, cint_t **r);
typedef char *(*z_f2d)(struct z *z, cint_t *a, int b, cint_t **r);
typedef char *(*z_f3)(struct z *z, cint_t *a, cint_t *b, cint_t **r, cint_t **m);

typedef struct z_func {
	z_f2 add;
	z_f2 sub;
	z_f2 mul;
	z_f3 div;
	z_f1 square;
	z_f2 xor;
	z_f2 or;
	z_f2 and;
	z_f2d lsl;
	z_f2d lsr;
	void (*free)(struct z *z);
} z_func_t;

typedef struct z {
	void *z_data;
	z_func_t *_z_code;
#define z_add	_z_code->add
#define z_sub	_z_code->sub
#define z_mul	_z_code->mul
#define z_div	_z_code->div
#define z_square	_z_code->square
#define z_xor	_z_code->xor
#define z_or	_z_code->or
#define z_and	_z_code->and
#define z_lsl	_z_code->lsl
#define z_lsr	_z_code->lsr
#define z_free	_z_code->free
	xsSlot z_protoInteger;
	FskInstrumentedItemDeclaration
} z_t;


/*
 * Arith.module host object
 */

struct mod;
typedef char *(*mod_f1)(struct mod *mod, cint_t *a, cint_t **r);
typedef char *(*mod_f2)(struct mod *mod, cint_t *a, cint_t *b, cint_t **r);
typedef char *(*mod_f4)(struct mod *mod, cint_t *a1, cint_t *e1, cint_t *a2, cint_t *e2, cint_t **r);

typedef struct mod_func {
	mod_f2 add;
	mod_f1 inv;
	mod_f2 sub;
	mod_f2 mul;
	mod_f1 square;
	mod_f1 mulinv;
	mod_f2 exp;
	mod_f4 exp2;
	mod_f1 mod;
	void (*free)(struct mod *mod);
} mod_func_t;

typedef struct mod {
	z_t *z;
	cint_t *m;
	void *mod_data;
	mod_func_t *_mod_code;
#define mod_add	_mod_code->add
#define mod_inv	_mod_code->inv
#define mod_sub	_mod_code->sub
#define mod_mul	_mod_code->mul
#define mod_square	_mod_code->square
#define mod_mulinv	_mod_code->mulinv
#define mod_exp	_mod_code->exp
#define mod_exp2	_mod_code->exp2
#define mod_mod	_mod_code->mod
#define mod_free	_mod_code->free
	FskInstrumentedItemDeclaration
} mod_t;


/*
 * Arith.ec host object
 */

struct ec;
typedef char *(*ec_f1)(struct ec *ec, ecp_t *a, ecp_t **r);
typedef char *(*ec_f2)(struct ec *ec, ecp_t *a, ecp_t *b, ecp_t **r);
typedef char *(*ec_f2d)(struct ec *ec, ecp_t *a, cint_t *k, ecp_t **r);
typedef char *(*ec_f4d)(struct ec *ec, ecp_t *a1, cint_t *k1, ecp_t *a2, cint_t *k2, ecp_t **r);

typedef struct ec_func {
	ec_f1 inv;
	ec_f2 add;
	ec_f2d mul;
	ec_f4d mul2;
	void (*free)(struct ec *ecp);
} ec_func_t;

typedef struct ec {
	mod_t *mod;
	cint_t *a, *b;
	ec_func_t *_ec_code;
#define ec_inv	_ec_code->inv
#define ec_add	_ec_code->add
#define ec_mul	_ec_code->mul
#define ec_mul2	_ec_code->mul2
#define ec_free	_ec_code->free
	xsSlot ec_protoECPoint;
	FskInstrumentedItemDeclaration
} ec_t;
