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
#include "mc_memory.h"
#include "mc_time.h"
#include "mc_file.h"
#include "mc_env.h"
#include "mc_misc.h"
#include "mc_xs.h"

#include <wmstdio.h>
#include <wm_os.h>
#include "mc_wmsdk.h"

#ifdef mxStress
extern int gxStress;
#endif
#ifdef mxTrace
extern short gxDoTrace;
#endif

static xsCreation creation = {
	25*1024,	/* initial chunk size */
	2048,		/* incremental chunk size */
	0,		/* initial heap count	-- will be calculated later */
	128,		/* incremental heap count	-- wasting 16 bytes / allocation */
	650,		/* stack count */
	2048+1024,	/* key count */
	97,		/* name modulo */
	127,		/* symbol modulo */
};

static jmp_buf _mc_jmpbuf;

static int
mc_main()
{
	xsMachine *machine;
	int status = 0;
	void *archive = NULL;

#ifdef mxTrace
	gxDoTrace = 0;
#endif
#ifdef mxStress
	gxStress = 0;
#endif
	mc_minit(&creation);	/* initialize the secondary heap */
	mc_stdio_init();
#if MC_LOG_MEMSTATS
	mc_mstats(true);
#endif
	mc_file_init();
	mc_env_init();
#if XS_ARCHIVE
	archive = fxMapArchive("", NULL);
#endif
	if ((machine = xsCreateMachine(&creation, archive, "mc", NULL)) == NULL) {
		mc_fatal("Fatal! xsCreateMachine failed\n");
		return -1;
	}
#if MC_LOG_MEMSTATS
	mc_mstats(true);
	xsReportMemoryUse(machine, NULL, NULL);
#endif
	xsBeginHost(machine);
	xsTry {
		xsVars(2);

		/* add the default path */
		xsVar(0) = xsModulePaths();
		xsSetString(xsVar(1), "");
		xsCall_noResult(xsVar(0), xsID("add"), &xsVar(0), NULL);
		fxRunModule(the, "application");
		status = 0;	/* can't get the result... */
	} xsCatch {
		status = -1;
	}
	xsEndHost(machine);
	xsDeleteMachine(machine);
#if XS_ARCHIVE
	fxUnmapArchive(archive);
#endif
	mc_env_fin();
	mc_file_fin();
	mc_stdio_fin();
	mc_mfin();
	return status;
}

static os_thread_t mc_app_thread;
static os_thread_stack_define(mc_app_stack, MC_STACK_SIZE);

#if K5
#include <mdev_pinmux.h>
#include <mdev_gpio.h>

static void
header_gpio_init()
{
	static uint8_t pins[] = {0, 1, 2, 3, 18, 19, 20, 21, 42, 43, 44, 45, 46, 47, 48, 49};
	unsigned int i;

	for (i = 0; i < sizeof(pins); i++) {
		GPIO_PinMuxFun(pins[i], PINMUX_FUNCTION_0);
		GPIO_SetPinDir(pins[i], GPIO_OUTPUT);
	}
}

static void
powerground_init()
{
	int i, j;
	uint32_t digital14 = 0xf, digital58 = 0xf, analog14 = 0xf, analog58 = 0xf;
	static uint8_t pins[][5] = {
		{11, 12, 13, 14, 15},	/* analog */
		{17, 16, 4, 34, 23},	/* digital */
	};
	enum {PIN_OE = 0, PIN_STR, PIN_D1, PIN_CP, PIN_D2};

#if K5_MINI
	/* check to see if the board has power / ground pinmux capability */
	GPIO_SetPinDir(pins[0][PIN_OE], GPIO_INPUT);
	GPIO_SetPinDir(pins[1][PIN_OE], GPIO_INPUT);
	mc_conf.power_ground_pinmux = (GPIO_ReadPinLevel(pins[0][PIN_OE]) == GPIO_IO_LOW || GPIO_ReadPinLevel(pins[1][PIN_OE]) == GPIO_IO_LOW);
	if (!mc_conf.power_ground_pinmux)
		return;
#else
	mc_conf.power_ground_pinmux = !0;	/* always enable for K5 */
#endif

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 5; j++) {
			GPIO_PinMuxFun(pins[i][j], pins[i][j] == 23 ? PINMUX_FUNCTION_1 : PINMUX_FUNCTION_0);
			GPIO_SetPinDir(pins[i][j], GPIO_OUTPUT);
		}
	}
	GPIO_WritePinOutput(pins[0][PIN_OE], GPIO_IO_LOW);
	GPIO_WritePinOutput(pins[1][PIN_OE], GPIO_IO_LOW);
	for (i = 0; i < 8; i++) {
		GPIO_WritePinOutput(pins[0][PIN_CP], GPIO_IO_LOW);
		GPIO_WritePinOutput(pins[1][PIN_CP], GPIO_IO_LOW);
		GPIO_WritePinOutput(pins[1][PIN_D1], digital14 & 0x80 ? GPIO_IO_HIGH : GPIO_IO_LOW);
		GPIO_WritePinOutput(pins[1][PIN_D2], digital58 & 0x80 ? GPIO_IO_HIGH : GPIO_IO_LOW);
		GPIO_WritePinOutput(pins[0][PIN_D1], analog14 & 0x80 ? GPIO_IO_HIGH : GPIO_IO_LOW);
		GPIO_WritePinOutput(pins[0][PIN_D2], analog58 & 0x80 ? GPIO_IO_HIGH : GPIO_IO_LOW);
		mc_usleep(1000);
		GPIO_WritePinOutput(pins[0][PIN_CP], GPIO_IO_HIGH);
		GPIO_WritePinOutput(pins[1][PIN_CP], GPIO_IO_HIGH);
		digital14 <<= 1;
		digital58 <<= 1;
		analog14 <<= 1;
		analog58 <<= 1;
	}
	GPIO_WritePinOutput(pins[0][PIN_STR], GPIO_IO_HIGH);
	GPIO_WritePinOutput(pins[1][PIN_STR], GPIO_IO_HIGH);
	GPIO_WritePinOutput(pins[0][PIN_STR], GPIO_IO_LOW);
	GPIO_WritePinOutput(pins[1][PIN_STR], GPIO_IO_LOW);
	GPIO_WritePinOutput(pins[0][PIN_OE], GPIO_IO_HIGH);
	GPIO_WritePinOutput(pins[1][PIN_OE], GPIO_IO_HIGH);
}
#endif

static void board_led_button_init()
{
	typedef output_gpio_cfg_t (*board_led_t)();
	static board_led_t board_led_f[] = {
		board_led_1, board_led_2, board_led_3,
	};
	unsigned int i, j;
	output_gpio_cfg_t gcfg;

	/* set up the board LEDs */
	for (i = 0; i < MC_MAX_LED_PINS; i++)
		mc_conf.led_pins[i] = -1;
	for (i = 0, j = 0; i < sizeof(board_led_f) / sizeof(board_led_f[0]); i++) {
#if K5
		if (!mc_conf.power_ground_pinmux) {	/* K5 mini */
			if (i != 1)	/* only has 1 LED on GPIO40 */
				continue;
		}
#endif
		gcfg = (*board_led_f[i])();
		if (gcfg.gpio != -1) {
			if (j < MC_MAX_LED_PINS)
				mc_conf.led_pins[j++] = gcfg.gpio;
			GPIO_PinMuxFun(gcfg.gpio, PINMUX_FUNCTION_0);
			GPIO_SetPinDir(gcfg.gpio, GPIO_OUTPUT);
			GPIO_WritePinOutput(gcfg.gpio, GPIO_IO_LOW);	/* turn on */
		}
	}

	/* set the power buttons - the board API doesn't tell us what buttons can be the wakeup buttons */
	for (i = 0; i < MC_MAX_WAKEUP_BUTTONS; i++)
		mc_conf.wakeup_buttons[i] = -1;
#if K5
	mc_conf.wakeup_buttons[0] = mc_conf.power_ground_pinmux ? 22 : -1;
	mc_conf.deviceID = "K5";
	mc_conf.usb_console = mc_conf.power_ground_pinmux;
#elif CONFIG_CPU_MC200
	mc_conf.power_ground_pinmux = 0;
	mc_conf.wakeup_buttons[0] = 22;
	mc_conf.wakeup_buttons[1] = 23;
	mc_conf.deviceID = "MC200";
	mc_conf.usb_console = 1;
#elif CONFIG_CPU_MW300
	mc_conf.power_ground_pinmux = 0;
	mc_conf.wakeup_buttons[0] = 22;
	mc_conf.wakeup_buttons[1] = 23;
	mc_conf.deviceID = "MW300";
	mc_conf.usb_console = 1;
#else
	mc_conf.power_ground_pinmux = 0;
	mc_conf.deviceID = "unknown";
	mc_conf.usb_console = 0;
#endif
}

static void
mc_thread_main(os_thread_arg_t data)
{
	int status;

#if K5
	powerground_init();
	header_gpio_init();
#endif
	board_led_button_init();
	wmtime_init();
	mc_pm_init();		/* (hw)rtc shoud be initialized here */
	mc_rtc_time_set(0);	/* another trick to run RTC normally */
	mc_partition_init();
	if (_setjmp(_mc_jmpbuf) != 0)
		goto bail;
	do {
		status = mc_main();
		break;	/* @@ */
	} while (status == 0);
bail:;
}

int
main()
{
	/* the only reason to create a new thread is to have its own stack separated from the system stack */
	os_thread_create(&mc_app_thread,	/* thread handle */
			 "main thread",		/* thread name */
			 mc_thread_main,	/* entry function */
			 0,	/* argument */
			 &mc_app_stack,	/* stack */
			 OS_PRIO_3);	/* priority - medium low */
	return 0;
}

void
mc_exit(int status)
{
	if (status == 0) {
		fprintf(stderr, "exit 0\n");
		status = 1;
	}
	longjmp(_mc_jmpbuf, status);
}

void
mc_fatal(const char *format, ...)
{
	va_list args;
	volatile char junk __attribute__((unused));

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	mc_exit(-1);
}

void
mc_shutoff()
{
#if K5
	powerground_init();
	header_gpio_init();
#endif
	mc_pm_shutoff();
}

void
critical_error(int errno, void *data)
{
	fprintf(stderr, "[crit] Critical error number: 0x%x\n", errno);
	fprintf(stderr, "[crit] rebooting...\n");
	mc_pm_reboot();
}
