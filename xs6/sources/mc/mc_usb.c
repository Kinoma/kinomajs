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
#include "mc_wmsdk.h"
/* K5 Driver Library */
#include <usb/cdc.h>

static int usb_exit = 0;
#define CDC_POLLING_MS	50

#define CRLF	((uint8_t *) "\r\n")
#define BS		((uint8_t *) "\b \b")

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
#define CTRL(c)	(c - 'A' + 1)
#define ISPRINT(c)	((c) >= 0x20 && (c) < 0x7f)

	while (!usb_exit) {
		if (mc_usb_read(&c, 1) == 0) {
			os_thread_sleep(os_msec_to_ticks(CDC_POLLING_MS));
			continue;
		}
		if (c == '\n' || c == '\r') {
			if (bp < bend)
				*bp = '\0';
			else
				*(bp - 1) = '\0';
			mc_usb_write(CRLF, 2);
			mc_event_thread_call(usb_callback, buf, 0);
			bp = buf;
		}
		else if (c == '\b' || c == '\x7f') {
			if (bp > buf) {
				--bp;
				mc_usb_write(BS, 3);
			}
		}
		else if (c == CTRL('U')) {
			while (bp > buf) {
				--bp;
				mc_usb_write(BS, 3);
			}
		}
		else {
			if (bp < bend) {
				*bp = c;
				mc_usb_write(bp, 1);
				bp++;
			}
		}
	}
}

int
mc_usb_init()
{
	mc_usb_hw_init();
	CdcInit();
	mc_thread_create(usb_thread_main, NULL, -1);	/* create a thread for USB input */
	return 0;
}

void
mc_usb_fin()
{
	usb_exit++;
	os_thread_sleep(os_msec_to_ticks(CDC_POLLING_MS * 2));	/* wait for the thread to termiante */
	/* no API to deactivate USB! at least the thread has to be stopped... */
}
