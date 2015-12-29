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
#define USB_CONSOLE

#include "mc_stdio.h"
#include "mc_memory.h"
#include "mc_time.h"
#include "mc_file.h"
#include "mc_env.h"
#include "mc_xs.h"

#ifdef USB_CONSOLE
void mc_stdio_init();
void mc_stdio_fin();
#endif

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
	548,		/* stack count */
	2048+512,	/* key count */
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
#if MC_LOG_MEMSTATS
	mc_mstats(true);
#endif
	mc_file_init();
	mc_env_init();
	mc_log_init();
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
		mc_log_error("an exception from application. try to revert the active patition");
		mc_set_active_volume("/mcufw");
		status = -1;
	}
	xsEndHost(machine);
	xsDeleteMachine(machine);
#if XS_ARCHIVE
	fxUnmapArchive(archive);
#endif
	mc_env_fin();
	mc_file_fin();
	return status;
}

#include <wmstdio.h>
#include <wm_os.h>
#include <pwrmgr.h>
#include <partition.h>
#include <rtc.h>

static os_thread_t mc_app_thread;
static os_thread_stack_define(mc_app_stack, 8*1024);

#if K5
#include <mdev_pinmux.h>
#include <mdev_gpio.h>

static void
header_gpio_init()
{
	static uint8_t pins[] = {0, 1, 2, 3, 18, 19, 20, 21, 42, 43, 44, 45, 46, 47, 48, 49};
	int i;

	for (i = 0; i < sizeof(pins); i++){
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

static void
mc_thread_main(os_thread_arg_t data)
{
	int status;

	wmstdio_init(UART0_ID, 0);
#ifdef USB_CONSOLE
	mc_stdio_init();
#endif
#ifdef CONFIG_LCD_DRIVER
	mc_lcd_open();
#endif
	wmtime_init();
	pm_init();		/* (hw)rtc shoud be initialized here */
	rtc_time_set(0);	/* another trick to run RTC normally */
	part_init();
#if K5
	powerground_init();
	header_gpio_init();
#endif
	if (_setjmp(_mc_jmpbuf) != 0)
		goto bail;
	do {
		status = mc_main();
		break;	/* @@ */
	} while (status == 0);
bail:;
#ifdef USB_CONSOLE
	mc_stdio_fin();
#endif
#ifdef CONFIG_LCD_DRIVER
	mc_lcd_close();
#endif
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

#include <critical_error.h>
#include <mdev_pm.h>
#include <pwrmgr.h>

void
critical_error(critical_errno_t errno, void *data)
{
	fprintf(stderr, "[crit] Critical error number: 0x%x\n", errno);
	fprintf(stderr, "[crit] rebooting...\n");
	pm_reboot_soc();
}

#ifdef USB_CONSOLE
#include <usbsysinit.h>
#include <usb_device_api.h>
#include "mc_event.h"
#include "mc_ipc.h"

extern int USBActive(void);
extern int check_write_TO(uint32_t EPNum);

static stdio_funcs_t *saved_stdio_funcs = NULL;
#define USB_BUF_SIZE	(MAX_MSG_LEN * 4)
static uint8_t usb_buf[USB_BUF_SIZE];
static size_t usb_buf_ptr = 0;
static unsigned long usb_last_tick;
static int usb_exit = 0;
#define USB_TIMEOUT	600
#define USB_TIMEOUT_TICKS	os_msec_to_ticks(USB_TIMEOUT)
#define USB_READ_TIMEOUT	0x7fffffff
#define USB_ENDPOINT	2

static void
usb_log(const char *fmt, ...)
{
	va_list args;
	char buf[MAX_MSG_LEN];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	mc_log_write(buf, strlen(buf));
	if (saved_stdio_funcs && saved_stdio_funcs->sf_printf)
		(*saved_stdio_funcs->sf_printf)(buf);
}

static int
usb_drv_write(void *buf, size_t sz)
{
	int n;

#if 0
	if (check_write_TO(USB_ENDPOINT))
		usb_log("usb: timeout.\r\n");
#endif
	if ((n = usb2Write(buf, sz, USB_TIMEOUT, USB_ENDPOINT)) != (int)sz)
		usb_log("usb: usb2Write failed: %d\r\n", n);
	return n;
}

static void
usb_callback(xsMachine *the, void *closure)
{
	char *buf = closure;

	xsBeginHost(the);
	xsVars(2);
	xsGet(xsVar(0), xsGlobal, xsID("require"));
	xsSetString(xsVar(1), "CLI");
	xsCall(xsResult, xsVar(0), xsID("weak"), &xsVar(1), NULL);
	xsSetString(xsVar(1), buf);
	xsCall_noResult(xsResult, xsID("evaluate"), &xsVar(1), NULL);
	xsEndHost(the);
}

static void
usb_thread_main(void *args)
{
	uint8_t c;
	uint8_t buf[MAX_MSG_LEN], *bp = buf, *bend = buf + sizeof(buf);
	uint8_t crnl[2] = {'\r', '\n'};		/* usb can't transfer data in the .ro section?? */
	uint8_t bs[3] = {'\b', ' ', '\b'};
#define CTRL(c)	(c - 'A' + 1)
#define ISPRINT(c)	((c) >= 0x20 && (c) < 0x7f)

	while (!usb_exit) {
		int ret = usb2Read(&c, 1, USB_READ_TIMEOUT, USB_ENDPOINT);
		if (ret <= 0) {
			if (ret != CONIO_TIMEOUT)
				os_thread_sleep(os_msec_to_ticks(1000));
			continue;
		}
		if (c == '\n' || c == '\r') {
			if (bp < bend)
				*bp = '\0';
			else
				*(bp - 1) = '\0';
			usb_drv_write(crnl, 2);
			mc_event_thread_call(usb_callback, buf, 0);
			bp = buf;
		}
		else if (c == '\b' || c == '\x7f') {
			if (bp > buf) {
				--bp;
				usb_drv_write(bs, 3);
			}
		}
		else if (c == CTRL('U')) {
			while (bp > buf) {
				--bp;
				usb_drv_write(bs, 3);
			}
		}
		else {
			if (bp < bend) {
				*bp = c;
				usb_drv_write(bp, 1);
				bp++;
			}
		}
	}
}

static int
usb_write(void *buf, size_t n)
{
	unsigned long t, d;
	uint8_t *p;

	mc_log_write(buf, n);
	if (n > USB_BUF_SIZE)
		return -1;
	if (!USBActive())
		return -1;
	/*
	 * logic can't help
	 */
	t = os_ticks_get();
	d = t < usb_last_tick ? ~0UL - usb_last_tick + t : t - usb_last_tick;
	if (n > USB_BUF_SIZE - usb_buf_ptr) {
		usb_last_tick = t;
		usb_buf_ptr = 0;
		p = &usb_buf[usb_buf_ptr];
		if (d < USB_TIMEOUT_TICKS) {
			os_thread_sleep(USB_TIMEOUT_TICKS * 2);
			if (n >= USB_BUF_SIZE)
				--n;	/* room for '*' */
			*p = '*';
			memcpy(p + 1, buf, n);
			n++;
		}
		else
			memcpy(p, buf, n);
	}
	else {
		if (d < USB_TIMEOUT_TICKS && n > ((USB_BUF_SIZE * 3) / 4) - usb_buf_ptr)
			os_thread_sleep(USB_TIMEOUT_TICKS);
		p = &usb_buf[usb_buf_ptr];
		memcpy(p, buf, n);
	}
	if (usb_drv_write(p, n) != (int)n)
		return -1;
	usb_buf_ptr += n;
	return 0;
}

static int
mc_stdio_printf(char *str)
{
	int n = strlen(str);

	if (usb_write(str, n) == 0)
		return n;
	if (saved_stdio_funcs && saved_stdio_funcs->sf_printf)
		(*saved_stdio_funcs->sf_printf)(str);
	return n;
}

static int
mc_stdio_flush()
{
	if (saved_stdio_funcs && saved_stdio_funcs->sf_flush)
		(*saved_stdio_funcs->sf_flush)();
	return 0;
}

static int
mc_stdio_getchar(uint8_t *cp)
{
	int ret;

	if (saved_stdio_funcs && saved_stdio_funcs->sf_getchar) {
		if ((*saved_stdio_funcs->sf_getchar)(cp) == 1)
			return 1;
	}
	ret = usb2Read(cp, 1, USB_READ_TIMEOUT, USB_ENDPOINT);
	return ret < 0 ? 0 : ret;
}

static int
mc_stdio_putchar(char *cp)
{
	if (usb_write(cp, 1) == 0)
		return 1;
	if (saved_stdio_funcs && saved_stdio_funcs->sf_putchar)
		(*saved_stdio_funcs->sf_putchar)(cp);
	return 1;
}

static stdio_funcs_t mc_stdio_funcs = {
	mc_stdio_printf,
	mc_stdio_flush,
	mc_stdio_getchar,
	mc_stdio_putchar,
};

void
mc_stdio_init()
{
	if (saved_stdio_funcs != NULL)
		return;
	if (usb_device_system_init(USB_CDC) != WM_SUCCESS) {
		wmprintf("usb_stdio_init failed\r\n");
		return;
	}
	os_thread_sleep(os_msec_to_ticks(USB_TIMEOUT));
	saved_stdio_funcs = c_stdio_funcs;	/* set by wmstdio_init and it should be "console" */
	c_stdio_funcs = &mc_stdio_funcs;
	mc_thread_create(usb_thread_main, NULL);	/* create a thread for USB input */
}

void
mc_stdio_fin()
{
	usb_exit++;
	os_thread_sleep(os_msec_to_ticks(USB_READ_TIMEOUT));	/* wait for the thread to termiante */
	if (saved_stdio_funcs != NULL) {
		c_stdio_funcs = saved_stdio_funcs;
		saved_stdio_funcs = NULL;
	}
	/* no API to deactivate USB! at least the thread has to be stopped... */
}
#endif	/* USB_CONSOLE */
