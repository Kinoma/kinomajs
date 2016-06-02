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
#include "xs.h"
#include "kcl_arith.h"
#include "arith_common.h"

typedef struct {
	kcl_ecp_t *p;
	xsSlot proto_int;
} ecp_t;

#define arith_get_integer(ecp, slot)	__arith_get_integer(the, &slot, &ecp->proto_int)
#define arith_set_integer(ecp, slot, x)	__arith_set_integer(the, &slot, x, &ecp->proto_int)

void
xs_ecpoint_init(xsMachine *the)
{
	ecp_t *ecp;
	kcl_int_t *x, *y;
	kcl_err_t err;

	if ((ecp = crypt_malloc(sizeof(ecp_t))) == NULL)
		kcl_throw_error(the, KCL_ERR_NOMEM);
	if ((err = kcl_ecp_alloc(&ecp->p)) != KCL_ERR_NONE) {
		crypt_free(ecp);
		kcl_throw_error(the, err);
	}

	x = arith_get_integer(ecp, xsArg(0));
	y = arith_get_integer(ecp, xsArg(1));
	kcl_ecp_init(ecp->p, x, y);

	ecp->proto_int = xsGet(xsThis, xsID("_proto_int"));
	xsSetHostData(xsThis, ecp);
}

void
xs_ecpoint_destructor(void *data)
{
	if (data != NULL) {
		ecp_t *ecp = data;
		kcl_ecp_dispose(ecp->p);
		crypt_free(ecp);
	}
}

void
xs_ecpoint_getIdentity(xsMachine *the)
{
	kcl_ecp_t *p = xsGetHostData(xsThis);

	xsResult = xsBoolean(kcl_ecp_identity(p));
}

void
xs_ecpoint_getX(xsMachine *the)
{
	ecp_t *ecp = xsGetHostData(xsThis);

	arith_set_integer(ecp, xsResult, kcl_ecp_getX(ecp->p));
}

void
xs_ecpoint_getY(xsMachine *the)
{
	ecp_t *ecp = xsGetHostData(xsThis);

	arith_set_integer(ecp, xsResult, kcl_ecp_getY(ecp->p));
}

void
xs_ecpoint_setX(xsMachine *the)
{
	ecp_t *ecp = xsGetHostData(xsThis);
	kcl_int_t *x;
	kcl_err_t err;

	x = arith_get_integer(ecp, xsArg(0));
	if ((err = kcl_ecp_setX(ecp->p, x)) != KCL_ERR_NONE)
		kcl_throw_error(the, err);
}

void
xs_ecpoint_setY(xsMachine *the)
{
	ecp_t *ecp = xsGetHostData(xsThis);
	kcl_int_t *y;
	kcl_err_t err;

	y = arith_get_integer(ecp, xsArg(0));
	if ((err = kcl_ecp_setY(ecp->p, y)) != KCL_ERR_NONE)
		kcl_throw_error(the, err);
}
