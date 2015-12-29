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
#include "mc_stdio.h"
#include "mc_module.h"

#if !XS_ARCHIVE
#include "xm_debug.xs.c"
MC_MOD_DECL(debug);
#endif

#ifdef mxStress
extern int gxStress;
#endif
#ifdef mxTrace
extern short gxDoTrace;
#endif

void
xs_dbg_report(xsMachine *the)
{
	if (xsToInteger(xsArgc) > 0 && xsTest(xsArg(0))) {
		/* silence - return the memory use */
		xsMemoryUse slot, chunk;
		xsReportMemoryUse(the, &slot, &chunk);
		xsVars(1);
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsSetInteger(xsVar(0), chunk.current);
		xsSet(xsResult, xsID("chunk"), xsVar(0));
		xsSetInteger(xsVar(0), slot.current);
		xsSet(xsResult, xsID("slot"), xsVar(0));
	}
	else {
#if mxMC
		mc_mstats(true);
#endif
		xsReportMemoryUse(the, NULL, NULL);
	}
}

void
xs_dbg_login(xsMachine *the)
{
	char *host = xsToString(xsArg(0));

	xsSetBoolean(xsResult, xsStartDebug(the, host));
}

void
xs_dbg_logout(xsMachine *the)
{
	xsStopDebug(the);
}

void
xs_dbg_debugger(xsMachine *the)
{
	xsDebugger();
}

void
xs_dbg_gcEnable(xsMachine *the)
{
	xsBooleanValue flag = xsToBoolean(xsArg(0));
	xsEnableGarbageCollection(flag);
}

void
xs_dbg_stress(xsMachine *the)
{
#ifdef mxStress
	gxStress = xsToBoolean(xsArg(0));
#endif
}

void
xs_dbg_xstrace(xsMachine *the)
{
#if mxTrace
	gxDoTrace = xsToBoolean(xsArg(0));
#endif
}

#if mxDebug
extern void fxSetBreakpoint(xsMachine* the, xsStringValue thePath, xsStringValue theLine);
#endif

void
xs_dbg_setBreakpoint(xsMachine *the)
{
#if mxDebug
	char num[13];

	xsToStringBuffer(xsArg(1), num, sizeof(num));
	fxSetBreakpoint(the, xsToString(xsArg(0)), num);
#endif
}
