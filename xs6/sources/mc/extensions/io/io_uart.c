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
#include "mc_xs.h"
#include "mc_stdio.h"
#include "mc_module.h"

#if !XS_ARCHIVE
#include "ext_uart.xs.c"
MC_MOD_DECL(Serial);
#endif

#if mxMC
#include <wm_os.h>
#include <mdev_uart.h>

typedef struct {
	mdev_t *mdev;
} mc_uart_t;

#define MC_UART_MAX_ID	3
static int mc_uart_ids[MC_UART_MAX_ID] = {0};

void
xs_uart_constructor(xsMachine *the)
{
	int port = xsToInteger(xsArg(0));
	int baud = xsToInteger(xsArg(1));
	mc_uart_t *uart;
	if (!mc_uart_ids[port]) {
		uart_drv_init(port, UART_8BIT);
		mc_uart_ids[port]++;
	}
	if ((uart = mc_malloc(sizeof(mc_uart_t))) == NULL)
		mc_xs_throw(the, "uart: no mem");
	if ((uart->mdev = uart_drv_open(port, baud)) == NULL) {
		mc_free(uart);
		mc_xs_throw(the, "uart: uart_drv_open failed");
	}
	xsSetHostData(xsThis, uart);
}

void
xs_uart_destructor(void *data)
{
	if (data != NULL)
		mc_free(data);
}

void
xs_uart_close(xsMachine *the)
{
	mc_uart_t *uart = xsGetHostData(xsThis);

	if (uart->mdev != NULL) {
		uart_drv_close(uart->mdev);
		uart->mdev = NULL;
	}
}

void
xs_uart_read(xsMachine *the)
{
	mc_uart_t *uart = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);
	int nbytes = ac > 0 ? xsToInteger(xsArg(0)) : 128*50;
	uint8_t *p;
	int n, ttl = 0;
	uint8_t buf[128];

	/* read up all data */
	while ((n = uart_drv_read(uart->mdev, buf, sizeof(buf))) > 0) {
		if (ttl == 0)
			xsResult = xsArrayBuffer(NULL, n);
		else
			xsSetArrayBufferLength(xsResult, ttl + n);
		p = xsToArrayBuffer(xsResult);
		memcpy(p + ttl, buf, n);
		ttl += n;
		if (ttl >= nbytes)	/* safe guard */
			break;
		os_thread_sleep(os_msec_to_ticks(2));
	}
}

void
xs_uart_write(xsMachine *the)
{
	mc_uart_t *uart = xsGetHostData(xsThis);
	int datasize = xsGetArrayBufferLength(xsArg(0));
	void *data = xsToArrayBuffer(xsArg(0));
	int n = uart_drv_write(uart->mdev, data, datasize);
	xsSetInteger(xsResult, n);
}

#else

void
xs_uart_constructor(xsMachine *the)
{
}

void
xs_uart_destructor(void *data)
{
}

void
xs_uart_close(xsMachine *the)
{
}

void
xs_uart_read(xsMachine *the)
{
}

void
xs_uart_write(xsMachine *the)
{
}

#endif
