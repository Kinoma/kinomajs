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
#include "mc_misc.h"
#include "mc_event.h"
#include "mc_module.h"

#if !XS_ARCHIVE
#include "inetd_tftpd.xs.c"
MC_MOD_DECL(tftpd);
#endif

#if mxMC
#include <mdev_pinmux.h>
#include <mdev_gpio.h>

static int mc_pinmux_functions[] = {
	PINMUX_FUNCTION_0,
	PINMUX_FUNCTION_1,
	PINMUX_FUNCTION_2,
	PINMUX_FUNCTION_3,
	PINMUX_FUNCTION_4,
	PINMUX_FUNCTION_5,
	PINMUX_FUNCTION_6,
	PINMUX_FUNCTION_7,
};

void
xs_pinmux(xsMachine *the)
{
	mdev_t *pinmux = NULL, *gpio = NULL;
	int i, n;
	int pin, func;
	static int initialized = 0;

	xsVars(4);
	if (!initialized) {
		pinmux_drv_init();
		initialized++;
	}
	if ((pinmux = pinmux_drv_open("MDEV_PINMUX")) == NULL)
		goto bail;
	if ((gpio = gpio_drv_open("MDEV_GPIO")) == NULL)
		goto bail;
	xsGet(xsVar(0), xsArg(0), xsID("length"));
	n = xsToInteger(xsVar(0));
	for (i = 0; i < n; i++) {
		xsGet(xsVar(0), xsArg(0), i);
		xsGet(xsVar(1), xsVar(0), 0);
		xsGet(xsVar(2), xsVar(0), 1);
		xsGet(xsVar(3), xsVar(0), 2);
		pin = xsToInteger(xsVar(1));
		func = xsToInteger(xsVar(2));
		pinmux_drv_setfunc(pinmux, pin, func);
		if (xsTypeOf(xsVar(3)) == xsIntegerType)
			gpio_drv_setdir(gpio, pin, xsToInteger(xsVar(3)));
		mc_usleep(20*1000);
	}
bail:
	if (gpio != NULL)
		gpio_drv_close(gpio);
	if (pinmux != NULL)
		pinmux_drv_close(pinmux);
}

void
xs_pin_write(xsMachine *the)
{
	mdev_t *gpio;
	int pin = xsToInteger(xsArg(0));
	int val = xsToInteger(xsArg(1));

	if ((gpio = gpio_drv_open("MDEV_GPIO")) == NULL)
		return;
	gpio_drv_write(gpio, pin, val);
	gpio_drv_close(gpio);
}

void
xs_pin_read(xsMachine *the)
{
	mdev_t *gpio;
	int pin = xsToInteger(xsArg(0));
	int val;

	if ((gpio = gpio_drv_open("MDEV_GPIO")) == NULL)
		return;
	gpio_drv_read(gpio, pin, &val);
	gpio_drv_close(gpio);
	xsSetInteger(xsResult, val);
}

typedef struct {
	xsMachine *the;
	xsSlot this;
	int pin;
	int gpiotype;
	int val;
	enum {PIN_IDLE, PIN_INPROGRESS, PIN_CLOSED, PIN_DESTROYED} status;
} mc_pin_int_callback_t;

static void
xs_pin_int_destructor(void *data)
{
	mc_log_debug("xs_pin_int_destructor: 0x%x\n", data);
	if (data != NULL) {
		mc_pin_int_callback_t *cb = data;
		if (cb->status == PIN_INPROGRESS)
			cb->status = PIN_DESTROYED;
		else
			mc_free(data);
	}
}

static void mc_pin_callback(xsMachine *, void *closure);

static void
mc_gpio_int_callback(int pin, void *data)
{
	mc_pin_int_callback_t *cb = data;
	mdev_t *gpio;

	gpio_drv_set_cb(NULL, cb->pin, GPIO_INT_DISABLE, NULL, NULL);	/* disable the interrupt until the callback is called */
	/* read the value of the pin ... it'll be too late to read the value after the callback is called */
	if ((gpio = gpio_drv_open("MDEV_GPIO")) == NULL)
		return;
	gpio_drv_read(gpio, cb->pin, &cb->val);
	gpio_drv_close(gpio);
	mc_event_thread_call(mc_pin_callback, data, MC_CALL_CRITICAL);
	cb->status = PIN_INPROGRESS;
}

static void
mc_pin_callback(xsMachine *the, void *closure)
{
	mc_pin_int_callback_t *cb = closure;

	if (cb->status == PIN_DESTROYED) {
		mc_free(cb);
		return;
	}
	else if (cb->status == PIN_CLOSED)
		return;
	xsBeginHost(cb->the);
	xsVars(1);
	xsSetInteger(xsVar(0), cb->val);
	xsCall_noResult(cb->this, xsID("_callback"), &xsVar(0), NULL);
	xsEndHost(cb->the);
	gpio_drv_set_cb(NULL, cb->pin, cb->gpiotype, cb, mc_gpio_int_callback);	/* re-enable the interrupt */
}

static void
xs_pin_close(xsMachine *the)
{
	mc_pin_int_callback_t *cb = xsGetHostData(xsThis);

	gpio_drv_set_cb(NULL, cb->pin, GPIO_INT_DISABLE, NULL, NULL);
	cb->status = PIN_CLOSED;
}

static void
xs_pin_sendEvent(xsMachine *the)
{
	mc_pin_int_callback_t *cb = xsGetHostData(xsThis);

	mc_event_thread_call(mc_pin_callback, cb, MC_CALL_CRITICAL);
}

void
xs_pin_newEvent(xsMachine *the)
{
	int pin = xsToInteger(xsArg(0));
	int type = xsToInteger(xsArg(1));
	GPIO_Int_Type gpiotype;
	mc_pin_int_callback_t *cb;

	xsVars(1);
	if ((cb = mc_malloc(sizeof(mc_pin_int_callback_t))) == NULL)
		return;
	xsResult = xsNewHostObject(xs_pin_int_destructor);
	xsSet(xsResult, xsID("_callback"), xsArg(2));
	xsVar(0) = xsNewHostFunction(xs_pin_close, 0);
	xsSet(xsResult, xsID("close"), xsVar(0));
	/* just for testing the local event */
	xsVar(0) = xsNewHostFunction(xs_pin_sendEvent, 0);
	xsSet(xsResult, xsID("sendEvent"), xsVar(0));

	if ((type & 0x03) == 0x03)
		gpiotype = GPIO_INT_BOTH_EDGES;
	else if (type & 0x1)
		gpiotype = GPIO_INT_RISING_EDGE;
	else if (type & 0x2)
		gpiotype = GPIO_INT_FALLING_EDGE;
	else
		gpiotype = GPIO_INT_DISABLE;
	cb->the = the;
	cb->this = xsResult;
	cb->pin = pin;
	cb->gpiotype = gpiotype;
	cb->status = PIN_IDLE;
	gpio_drv_set_cb(NULL, pin, gpiotype, cb, mc_gpio_int_callback);
	xsSetHostData(xsResult, cb);
}

void
xs_pin_init(xsMachine *the)
{
	unsigned int i;

	xsVars(1);
	pinmux_drv_init();
	gpio_drv_init();
	for (i = 0; i < sizeof(mc_pinmux_functions) / sizeof(mc_pinmux_functions[0]); i++) {
		char name[sizeof("PINMUX_FUNCTION_XXX")];
		snprintf(name, sizeof(name), "PINMUX_FUNCTION_%d", i);
		xsSetInteger(xsVar(0), mc_pinmux_functions[i]);
		xsSet(xsThis, xsID(name), xsVar(0));
	}
	xsSetInteger(xsVar(0), GPIO_INPUT);
	xsSet(xsThis, xsID("GPIO_INPUT"), xsVar(0));
	xsSetInteger(xsVar(0), GPIO_OUTPUT);
	xsSet(xsThis, xsID("GPIO_OUTPUT"), xsVar(0));
}
#else

void
xs_pinmux(xsMachine *the)
{
}

void
xs_pin_write(xsMachine *the)
{
}

void
xs_pin_read(xsMachine *the)
{
}

void
xs_pin_newEvent(xsMachine *the)
{
}

void
xs_pin_init(xsMachine *the)
{
}
#endif
