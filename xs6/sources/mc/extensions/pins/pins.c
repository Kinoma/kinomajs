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
#include "xs.h"
#include "mc_misc.h"

static void
bll_mdelay(xsMachine *the)
{
	long mdelay = xsToInteger(xsArg(0));
	mc_usleep(mdelay * 1000);
}

static void
bll_delay(xsMachine *the)
{
	long delay = xsToInteger(xsArg(0));
	mc_usleep(delay * 1000 * 1000);
}

void
for_bll(xsMachine *the)
{
	xsVars(1);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsVar(0) = xsNewHostFunction(bll_mdelay, 1);
	xsSet(xsResult, xsID("mdelay"), xsVar(0));
	xsVar(0) = xsNewHostFunction(bll_delay, 1);
	xsSet(xsResult, xsID("delay"), xsVar(0));
	xsSet(xsGlobal, xsID("sensorUtils"), xsResult);
}
