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
#include "mc_ipc.h"
#include "mc_xs.h"
#include "mc_usb.h"

#include <wmstdio.h>
#include <wm_os.h>
#include <usbsysinit.h>
#include <usb_device_api.h>

/* for LEDs */
#include "mc_misc.h"
#include <mdev_pinmux.h>
#include <mdev_gpio.h>

extern int USBActive(void);
extern int check_write_TO(uint32_t EPNum);

#define USB_BUF_SIZE	(MAX_MSG_LEN * 4)
static uint8_t usb_buf[USB_BUF_SIZE];
static size_t usb_buf_ptr = 0;
static unsigned long usb_last_tick;
static int usb_exit = 0;
#define USB_TIMEOUT	300
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

int
mc_usb_write(void *buf, size_t n)
{
	unsigned long t, d;
	uint8_t *p;
	int ret = -1;

	if (n > USB_BUF_SIZE)
		goto bail;
	if (!USBActive())
		goto bail;
	t = os_ticks_get();
	d = t < usb_last_tick ? ~0UL - usb_last_tick + t : t - usb_last_tick;
	if (d >= USB_TIMEOUT_TICKS) {
		usb_buf_ptr = 0;
		usb_last_tick = os_ticks_get();
	}
	if (usb_buf_ptr + n > USB_BUF_SIZE) {
		usb_buf_ptr = 0;
		if (d < USB_TIMEOUT_TICKS)
			os_thread_sleep(USB_TIMEOUT_TICKS);
		usb_last_tick = os_ticks_get();
	}
	p = &usb_buf[usb_buf_ptr];
	memcpy(p, buf, n);
	if (usb_drv_write(p, n) != (int)n)
		goto bail;
	usb_buf_ptr += n;
	ret = 0;
bail:
	return ret;
}

int
mc_usb_read(void *buf, size_t n)
{
	return usb2Read(buf, n, USB_READ_TIMEOUT, USB_ENDPOINT);
}

int
mc_usb_init()
{
	if (usb_device_system_init(USB_CDC) != WM_SUCCESS) {
		int i;
		wmprintf("usb_stdio_init failed\r\n");
		for (i = 0; i < MC_MAX_LED_PINS; i++)
			GPIO_WritePinOutput(mc_conf.led_pins[i], GPIO_IO_HIGH);
		GPIO_WritePinOutput(mc_conf.led_pins[0], GPIO_IO_LOW);	/* red */
		os_thread_sleep(5000);
		return -1;
	}
	os_thread_sleep(1000);
	mc_thread_create(usb_thread_main, NULL, -1);	/* create a thread for USB input */
	return 0;
}

void
mc_usb_fin()
{
	usb_exit++;
	os_thread_sleep(os_msec_to_ticks(USB_READ_TIMEOUT));	/* wait for the thread to termiante */
	/* no API to deactivate USB! at least the thread has to be stopped... */
}
