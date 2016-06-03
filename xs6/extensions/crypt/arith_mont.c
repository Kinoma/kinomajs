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

void
xs_mont_init(xsMachine *the)
{
	int ac = xsToInteger(xsArgc);
	mod_t *mod = xsGetHostData(xsThis);
	z_t *z;
	kcl_int_t *m;
	kcl_mod_method_t method = KCL_MOD_METHOD_LR;
	int options = 0;

	xsVars(2);
	xsVar(0) = xsGet(xsThis, xsID("z"));
	xsVar(1) = xsGet(xsThis, xsID("m"));
	z = xsGetHostData(xsVar(0));
	m = xsGetHostData(xsVar(1));
	if (ac > 0 && xsTypeOf(xsArg(0)) == xsIntegerType) {
		method = xsToInteger(xsArg(0));
		if (ac > 1 && xsTypeOf(xsArg(1)) == xsIntegerType)
			options = xsToInteger(xsArg(1));
	}
	kcl_mont_init(mod->ctx, z->ctx, m, method, options);
}
