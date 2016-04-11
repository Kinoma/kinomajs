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
#include "mc_misc.h"
#include "mc_event.h"
#include "mc_module.h"

#if mxMC
#include <mdev_gpio.h>

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

	gpio_drv_set_cb(NULL, cb->pin, GPIO_INT_DISABLE, NULL, NULL);	/* disable the interrupt until the callback is called */
	/* read the value of the pin ... it'll be too late to read the value after the callback is called */
	cb->val = GPIO_ReadPinLevel(cb->pin);
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
#else

void
xs_pin_newEvent(xsMachine *the)
{
}

#endif
