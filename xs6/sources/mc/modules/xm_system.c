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
#include "mc_stdio.h"
#include "mc_event.h"
#include "mc_file.h"
#include "mc_env.h"
#include "mc_time.h"
#include "mc_misc.h"
#include "mc_ipc.h"
#include "mc_wmsdk.h"
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
	int i, j;

	xsSet(xsThis, xsID("_global"), xsGlobal);
	mc_rng_init(NULL, 0);
	/*
	 * set config
	 */
	if (mc_conf.deviceID == NULL)
		return;		/* nothing to set */
	xsVars(4);
	xsGet(xsVar(0), xsThis, xsID("_config"));
	/* LEDs */
	xsSetNewInstanceOf(xsVar(1), xsArrayPrototype);
	for (i = 0, j = 0; i < MC_MAX_LED_PINS; i++) {
		if (mc_conf.led_pins[i] >= 0) {
			xsSetInteger(xsVar(2), j); j++;
			xsSetInteger(xsVar(3), mc_conf.led_pins[i]);
			xsSetAt(xsVar(1), xsVar(2), xsVar(3));
		}
	}
	if (j > 0)
		xsSet(xsVar(0), xsID("ledPins"), xsVar(1));
	/* wakeup buttons */
	xsSetNewInstanceOf(xsVar(1), xsArrayPrototype);
	for (i = 0, j = 0; i < MC_MAX_WAKEUP_BUTTONS; i++) {
		if (mc_conf.wakeup_buttons[i] >= 0) {
			xsSetInteger(xsVar(2), j); j++;
			xsSetInteger(xsVar(3), mc_conf.wakeup_buttons[i]);
			xsSetAt(xsVar(1), xsVar(2), xsVar(3));
		}
	}
	if (j > 0)
		xsSet(xsVar(0), xsID("wakeupButtons"), xsVar(1));
	/* power/ground enable */
	xsSetBoolean(xsVar(1), mc_conf.power_ground_pinmux);
	xsSet(xsVar(0), xsID("powerGroundPinmux"), xsVar(1));
	xsSetBoolean(xsVar(1), mc_conf.usb_console);
	xsSet(xsVar(0), xsID("usbConsole"), xsVar(1));
}

void
xs_system_fin(xsMachine *the)
{
}

void
xs_system_init_rng(xsMachine *the)
{
	size_t sz;
	void *data;

	switch (xsTypeOf(xsArg(0))) {
	case xsIntegerType:
	case xsNumberType: {
		unsigned long n = xsToNumber(xsArg(0));
		data = &n;
		sz = sizeof(long);
		break;
	}
	case xsStringType: {
		char *s = xsToString(xsArg(0));
		data = s;
		sz = strlen(s);
		break;
	}
	default:
		if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
			data = xsToArrayBuffer(xsArg(0));
			sz = xsGetArrayBufferLength(xsArg(0));
		}
		else {
			data = NULL;
			sz = 0;
		}
		break;
	}
	mc_rng_init(data, sz);
}

void
xs_system_get_rng(xsMachine *the)
{
	int sz = xsToInteger(xsArg(0));

	xsResult = xsArrayBuffer(NULL, sz);
	mc_rng_gen((uint8_t *)xsToArrayBuffer(xsResult), sz);
}

void
xs_system_init_key(xsMachine *the)
{
	void *data = xsToArrayBuffer(xsArg(0));
	size_t sz = xsGetArrayBufferLength(xsArg(0));

	mc_srng_init(data, sz);
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
	xsSetString(xsResult, mc_conf.deviceID == NULL ? "host" : mc_conf.deviceID);
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

	if (mc_gethostname(hostname, sizeof(hostname)) == 0)
		xsSetString(xsResult, hostname);
	/* else return undefined */
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

struct img_hdr {
	uint32_t magic_str;
	uint32_t magic_sig;
	uint32_t time;
	uint32_t seg_cnt;
	uint32_t entry;
};

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

typedef struct {
	xsMachine *the;
	xsSlot this;
} mc_system_callback_data_t;

static void
mc_system_shutdown_callback(int status, void *closure)
{
	mc_system_callback_data_t *cb = closure;

	xsBeginHost(cb->the);
	xsCall_noResult(cb->this, xsID("onShutdown"), NULL);
	xsEndHost(cb->the);
}

void
xs_system_run(xsMachine *the)
{
	int status;
	static mc_system_callback_data_t data;
	static mc_event_shutdown_callback_t cb;

	cb.f = mc_system_shutdown_callback;
	cb.closure = &data;
	data.the = the;
	data.this = xsThis;
	status = mc_event_main(the, &cb);
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

void
xs_system_reboot(xsMachine *the)
{
	if (xsToInteger(xsArgc) > 0 && xsTest(xsArg(0)))
		mc_pm_reboot();
	else {
		g_reboot = 1;
		mc_event_exit(1);
	}
}

#if mxMC
#include <mdev_gpio.h>

static void
blink_led(void *data)
{
	unsigned int i, toggle = 0;

	while (1) {
		int val = toggle++ & 1 ? GPIO_IO_HIGH : GPIO_IO_LOW;
		for (i = 0; i < MC_MAX_LED_PINS; i++) {
			if (mc_conf.led_pins[i] >= 0)
				GPIO_WritePinOutput(mc_conf.led_pins[i], val);
		}
		mc_usleep(100*1000);
	}
}
#endif

void
xs_system_shutdown(xsMachine *the)
{
	int status = xsToInteger(xsArgc) > 0 ? xsToInteger(xsArg(0)) : 0;
	if (status == 0) {
#if mxMC
		mc_thread_create(blink_led, NULL, 0);
		mc_usleep(10);
#endif
		mc_event_exit(1);
	}
	else if (status > 0) {
#if mxMC
		mc_shutoff();
#else
		exit(0);
#endif
	}
	else
		mc_exit(status);	/* terminate the process */
}
