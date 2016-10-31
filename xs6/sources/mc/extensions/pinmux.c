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

#if !XS_ARCHIVE
#include "inetd_tftpd.xs.c"
MC_MOD_DECL(tftpd);
#endif

#if mxMC
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
	int i, n;
	int pin, func;

	xsVars(4);
	xsGet(xsVar(0), xsArg(0), xsID("length"));
	n = xsToInteger(xsVar(0));
	for (i = 0; i < n; i++) {
		xsGet(xsVar(0), xsArg(0), i);
		xsGet(xsVar(1), xsVar(0), 0);
		xsGet(xsVar(2), xsVar(0), 1);
		xsGet(xsVar(3), xsVar(0), 2);
		pin = xsToInteger(xsVar(1));
		func = xsToInteger(xsVar(2));
		GPIO_PinMuxFun(pin, func);
		if (xsTypeOf(xsVar(3)) == xsIntegerType)
			GPIO_SetPinDir(pin, xsToInteger(xsVar(3)));
		mc_usleep(20*1000);
	}
}

void
xs_pin_write(xsMachine *the)
{
	if (xsToInteger(xsArgc) < 1)
		return;
	if (xsIsInstanceOf(xsArg(0), xsArrayPrototype)) {
		xsVars(3);
		xsGet(xsVar(0), xsArg(0), xsID("length"));
		int len = xsToInteger(xsVar(0)), i;
		for (i = 0; i < len; i++) {
			xsGet(xsVar(0), xsArg(0), i);
			xsGet(xsVar(1), xsVar(0), 0);
			xsGet(xsVar(2), xsVar(0), 1);
			GPIO_WritePinOutput(xsToInteger(xsVar(1)), xsToInteger(xsVar(2)));
		}
	}
	else
		GPIO_WritePinOutput(xsToInteger(xsArg(0)), xsToInteger(xsArg(1)));
}

void
xs_pin_read(xsMachine *the)
{
	int pin = xsToInteger(xsArg(0));
	int val;

	val = GPIO_ReadPinLevel(pin);
	xsSetInteger(xsResult, val);
}

void
xs_pin_init(xsMachine *the)
{
	unsigned int i;
	static int once = 0;

	if (!once) {
		gpio_drv_init();
		once++;
	}
	xsVars(1);
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
	xsSetInteger(xsResult, 0);
}

void
xs_pin_init(xsMachine *the)
{
}
#endif
