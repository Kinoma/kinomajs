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
#include "mc_event.h"
#include "mc_file.h"
#include "mc_env.h"
#include "mc_time.h"
#include "mc_misc.h"
#include "mc_module.h"
#if !mxMC
#include "mc_compat.h"
#endif

#if !XS_ARCHIVE
#include "xm_system.xs.c"
MC_MOD_DECL(system);
#endif

static int g_reboot = 0;

void
xs_system_init(xsMachine *the)
{
	xsSet(xsThis, xsID("_global"), xsGlobal);
}

void
xs_system_fin(xsMachine *the)
{
}

void
xs_system_init_rng(xsMachine *the)
{
	char *mac = xsToString(xsArg(0));

	/* initialize the secure RNG with the mac addresss */
	mc_rng_init(mac, strlen(mac));
}

void
xs_system_get_rng(xsMachine *the)
{
	int sz = xsToInteger(xsArg(0));

	xsResult = xsArrayBuffer(NULL, sz);
	mc_rng_gen((uint8_t *)xsToArrayBuffer(xsResult), sz);
}

void
xs_system_get_osVersion(xsMachine *the)
{
#if mxMC
	char buf[12];
	snprintf(buf, sizeof(buf), "WM/%d", WMSDK_VERSION);
	xsSetString(xsResult, buf);
#else
	struct utsname un;
	char buf[128];
	uname(&un);
	snprintf(buf, sizeof(buf), "%s/%s", un.sysname, un.release);
	xsSetString(xsResult, buf);
#endif
}

void
xs_system_get_device(xsMachine *the)
{
#if !mxMC
	xsSetString(xsResult, "host");
#elif K5
	xsSetString(xsResult, "K5");
#elif CONFIG_CPU_MC200
	xsSetString(xsResult, "MC200");
#elif CONFIG_CPU_MW300
	xsSetString(xsResult, "MW300");
#else
	xsSetString(xsResult, "unknown");
#endif
}

void
xs_system_get_platform(xsMachine *the)
{
	xsSetString(xsResult, "mc");
}

void
xs_system_get_hostname(xsMachine *the)
{
	char hostname[HOST_NAME_MAX];

	mc_gethostname(hostname, sizeof(hostname));
	xsSetString(xsResult, hostname);
}

void
xs_system_set_hostname(xsMachine *the)
{
	char *hostname = xsToString(xsArg(0));
	mc_sethostname(hostname, strlen(hostname) + 1);
}

void
xs_system_get_time(xsMachine *the)
{
	double t;
	struct timeval tv;

	mc_gettimeofday(&tv, NULL);
	t = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	xsSetNumber(xsResult, t);
}

void
xs_system_set_time(xsMachine *the)
{
	double t, t1;
	struct timeval tv;

	t = xsToNumber(xsArg(0));
	t1 = t / 1000;
	tv.tv_sec = (uint32_t)t1;
	tv.tv_usec = (uint32_t)((t - t1 * 1000) * 1000);
	mc_settimeofday(&tv, NULL);
}

void
xs_system_get_timezone(xsMachine *the)
{
	struct timezone tz;

	xsVars(1);
	mc_gettimeofday(NULL, &tz);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsSetInteger(xsVar(0), tz.tz_minuteswest);
	xsSet(xsResult, xsID("timedifference"), xsVar(0));
	xsSetInteger(xsVar(0), tz.tz_dsttime);
	xsSet(xsResult, xsID("dst"), xsVar(0));
}

void
xs_system_set_timezone(xsMachine *the)
{
	struct timezone tz;

	xsVars(1);
	xsGet(xsVar(0), xsArg(0), xsID("timedifference"));
	tz.tz_minuteswest = xsToInteger(xsVar(0));
	xsGet(xsVar(0), xsArg(0), xsID("dst"));
	tz.tz_dsttime = xsToInteger(xsVar(0));
	mc_settimeofday(NULL, &tz);
	if (xsHas(xsArg(0), xsID("timezone"))) {
		xsGet(xsVar(0), xsArg(0), xsID("timezone"));
		mc_env_set_default("TIME_ZONE", xsToString(xsVar(0)));
	}
}

#if mxMC
#include "firmware_structure.h"
#else
struct img_hdr {
	uint32_t magic_str;
	uint32_t magic_sig;
	uint32_t time;
	uint32_t seg_cnt;
	uint32_t entry;
};
#endif

void
xs_system_get_timestamp(xsMachine *the)
{
	MC_FILE *fp;
	struct img_hdr ih;

	xsSetInteger(xsResult, 0);
	if ((fp = mc_fopen("/mcufw", "ra")) != NULL) {
		if (mc_fread(&ih, sizeof(ih), 1, fp) == 1)
			xsSetInteger(xsResult, ih.time);
		mc_fclose(fp);
	}
}

void
xs_system_run(xsMachine *the)
{
	int status;

	status = mc_event_main(the);
	if (g_reboot != 0)
		status = g_reboot;
	else if (status > 0)
		status = -1;	/* shutdown */
	xsSetInteger(xsResult, status);
}

void
xs_system_load(xsMachine *the)
{
	const char *path = xsToString(xsArg(0));

	fxRunModule(the, (xsStringValue)path);
}

void
xs_system_sleep(xsMachine *the)
{
	uint32_t msec = (uint32_t)xsToInteger(xsArg(0));

	mc_usleep(msec * 1000);
}

void
xs_system_gc(xsMachine *the)
{
	xsCollectGarbage();
}

void
xs_system_addPath(xsMachine *the)
{
	int ac = xsToInteger(xsArgc), i;
	xsIndex id_add = xsID("add");

	xsResult = xsModulePaths();
	for (i = 0; i < ac; i++) {
		xsCall_noResult(xsResult, id_add, &xsArg(i), NULL);
	}
}

#if mxMC
#include <mdev_pm.h>
#include <pwrmgr.h>

void
xs_system_pm(xsMachine *the)
{
	int ac = xsToInteger(xsArgc);
	int state = xsToInteger(xsArg(0));
	int duration = ac > 1 ? xsToInteger(xsArg(1)) : 0;
	int reason;

	reason = pm_mcu_state((power_state_t)state, (uint32_t)duration);
	mc_log_debug("PM: wakeup reason = %d\n", reason);
	xsSetInteger(xsResult, reason);
}
#else	/* !mxMC */
void
xs_system_pm(xsMachine *the)
{
}
#endif	/* mxMC */

void
xs_system_reboot(xsMachine *the)
{
	if (xsToInteger(xsArgc) > 0 && xsTest(xsArg(0))) {
#if mxMC
		pm_reboot_soc();	/* force reboot */
#else
		exit(0);	/* if only we can get argv... */
#endif
	}
	else {
		g_reboot = 1;
		mc_event_exit();
	}
}

void
xs_system_shutdown(xsMachine *the)
{
	int status = xsToInteger(xsArgc) > 0 ? xsToInteger(xsArg(0)) : 0;
	if (status == 0)
		mc_event_exit();
	else if (status > 0) {
#if mxMC
		pm_mcu_state(PM4, 0);	/* deep sleep */
#else
		exit(0);
#endif
	}
	else
		mc_exit(status);	/* terminate the process */
}

#if mxMC
extern void mc_stdio_init();
extern void mc_stdio_fin();
#else
void mc_stdio_init() {};
void mc_stdio_fin() {};
#endif

void
xs_system_console(xsMachine *the)
{
	if (xsToInteger(xsArgc) > 0 && xsTest(xsArg(0)))
		mc_stdio_init();
	else
		mc_stdio_fin();
}
