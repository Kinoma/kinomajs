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
#include "mc_stdio.h"
#include "mc_xs.h"

void
mc_xs_throw(xsMachine *the, const char *message)
{
	xsVars(1);
	mc_log_error("### throw: %s\n", message);
	xsSetString(xsVar(0), (char *)message);
	xsNew(xsResult, xsGlobal, xsID("Error"), &xsVar(0), NULL);
	xsThrow(xsResult);
}

char *
mc_xs_exception_message(xsMachine *the)
{
	xsIndex id = xsID("message");

	if (xsTest(xsException) && xsHas(xsException, id)) {
		xsGet(the->scratch, xsException, id);
		return xsToString(the->scratch);
	}
	else
		return "";
}

void
mc_xs_build(xsMachine *the, xsSlot *this, const xs_host_t *host, int n)
{
	xsBeginHost(the);
	xsVars(1);
	while (--n >= 0) {
		xsVar(0) = xsNewHostFunction(host->callback, host->length);
		xsNewHostProperty(*this, xsID((char *)host->id), xsVar(0), host->attr, xsDefault);
	}
	xsEndHost(the);
}

int
mc_xs_enableGC(xsMachine *the, int flag)
{
	int current_flag = xsGetCollectFlag(the);
	xsEnableGarbageCollection(flag);
	return current_flag;
}
