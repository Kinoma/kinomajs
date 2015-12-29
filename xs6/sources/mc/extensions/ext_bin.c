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
#include "mc_xs.h"
#include "mc_misc.h"

#if !XS_ARCHIVE
#include "ext_bin.xs.c"
MC_MOD_DECL(Bin);
#endif

#ifndef howmany
#define howmany(x, y)	((x + (y) - 1) / (y))
#endif

void
xs_bin_encode(xsMachine *the)
{
	void *src = xsToArrayBuffer(xsArg(0));
	size_t len = xsGetArrayBufferLength(xsArg(0));
	char *dst;
	size_t n;

	if ((dst = mc_malloc(howmany(len, 3) * 4 + 1)) == NULL)
		mc_xs_throw(the, "Bin: no mem");
	n = mc_encode64(dst, src, len);
	dst[n] = '\0';
	xsSetString(xsResult, dst);
	mc_free(dst);
}

void
xs_bin_decode(xsMachine *the)
{
	void *src = xsToString(xsArg(0));
	size_t len = strlen(src);
	size_t n = howmany(len, 4) * 3;

	xsResult = xsArrayBuffer(NULL, n);
	n = mc_decode64(xsToArrayBuffer(xsResult), src, n);
	xsSetArrayBufferLength(xsResult, n);
}
