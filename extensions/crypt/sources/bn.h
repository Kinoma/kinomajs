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

typedef FskUInt64 bn_dword;
typedef UInt32 bn_word;
typedef UInt8 bn_bool;
typedef UInt16 bn_size;
typedef UInt8 bn_byte;

typedef struct {
	bn_bool sign;
	bn_size size;		/* data size in word */
	bn_word data[1];
} bn_t;

typedef struct {
	xsMachine *the;
	bn_byte *bufptr;
	bn_byte *bufend;
#define BN_TMPBUFSIZE	(1024*2)
	bn_byte buf[BN_TMPBUFSIZE];
} bn_context_t;

typedef struct {
	bn_context_t *ctx;
	bn_t *m;
	bn_word u;	/* for the Montgomery method only */
	bn_t *(*square_func)();
	bn_t *(*mul_func)();
	bn_t *(*conv_in_func)();
	bn_t *(*conv_out_func)();
	bn_t *(*exp_init)();
	bn_t *(*exp_func)();
	bn_t *(*exp2_func)();
	unsigned int options;
#define OPT_NOMONT	0x01
#define OPT_LR		0x02
#define OPT_PRECOMP	0x04
} bn_mod_t;

extern void bn_init_context(xsMachine *the, bn_context_t *ctx);
extern void bn_freetbuf(bn_context_t *ctx, void *bufptr);

extern int bn_iszero(bn_t *a);
extern int bn_isNaN(bn_t *a);
extern bn_bool bn_sign(bn_t *a);
extern bn_size bn_wsizeof(bn_t *a);
extern void bn_negate(bn_t *a);
extern void bn_copy(bn_t *a, bn_t *b);
extern int bn_bitsize(bn_t *e);
extern int bn_comp(bn_t *a, bn_t *b);

extern bn_t *bn_lsl(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw);
extern bn_t *bn_lsr(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw);
extern bn_t *bn_xor(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);
extern bn_t *bn_or(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);
extern bn_t *bn_and(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);

extern bn_t *bn_add(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb);
extern bn_t *bn_sub(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb);
extern bn_t *bn_mul(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb);
extern bn_t *bn_square(bn_context_t *ctx, bn_t *r, bn_t *a);
extern bn_t *bn_div(bn_context_t *ctx, bn_t *q, bn_t *a, bn_t *b, bn_t **r);
extern bn_t *bn_mod(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);
extern bn_t *bn_inc(bn_context_t *ctx, bn_t *r, bn_t *a, int d);

extern bn_t *bn_mod_add(bn_mod_t *mod, bn_t *r, bn_t *a, bn_t *b);
extern bn_t *bn_mod_sub(bn_mod_t *mod, bn_t *r, bn_t *a, bn_t *b);
extern bn_t *bn_mod_inv(bn_mod_t *mod, bn_t *r, bn_t *a);
extern bn_t *bn_mod_mod(bn_mod_t *mod, bn_t *r, bn_t *a);
extern bn_t *bn_mod_mul(bn_mod_t *mod, bn_t *r, bn_t *a, bn_t *b);
extern bn_t *bn_mod_square(bn_mod_t *mod, bn_t *r, bn_t *a);
extern bn_t *bn_mod_mulinv(bn_mod_t *mod, bn_t *r, bn_t *a);
extern bn_t *bn_mod_exp(bn_mod_t *mod, bn_t *r, bn_t *b, bn_t *e);
extern bn_t *bn_mod_exp2(bn_mod_t *mod, bn_t *r, bn_t *b1, bn_t *e1, bn_t *b2, bn_t *e2);
extern void bn_mod_init(bn_mod_t *mod, bn_context_t *ctx, bn_t *m, unsigned int options);
extern void bn_mod_freetbuf(bn_mod_t *mod, void *bufptr);
