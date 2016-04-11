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
#include "mc_xs.h"

#if mxMC
#include <mdev_wdt.h>

static int wdt_enable = 0;

static void
wdt_interrupt()
{
	WDT_IntClr();
	CLK_ModuleClkDisable(CLK_WDT);
	WDT_Disable();
	mc_shutoff();
}

static void
wdt_auto_strobe(int s, unsigned int flags, void *closure)
{
	if (wdt_enable)
		WDT_RestartCounter();
}

void
xs_wdt_start(xsMachine *the)
{
	int ac = xsToInteger(xsArgc);
	int index = xsToInteger(xsArg(0));
	int autostrobe =  ac > 1 && xsTest(xsArg(1));
	int shutdown =  ac > 2 && xsTest(xsArg(2));
	WDT_Config_Type cfg;

	if (shutdown) {
		NVIC_SetPriority(WDT_IRQn, 0xf);
		NVIC_EnableIRQ(WDT_IRQn);
		install_int_callback(INT_WDT, 0, wdt_interrupt);
	}

	cfg.timeoutVal = index;
	cfg.mode = shutdown ? WDT_MODE_INT : WDT_MODE_RESET;
	cfg.resetPulseLen = WDT_RESET_PULSE_LEN_2;
	WDT_Init(&cfg);

	CLK_ModuleClkEnable(CLK_WDT);
#if defined(CONFIG_CPU_MW300)
	/* For 88MW300, APB1 bus runs at 50MHz whereas for 88MC200 it runs at
	 * 25MHz, hence following clk divider is added to keep timeout same.
	 */
	CLK_ModuleClkDivider(CLK_WDT, 1);
#endif
	WDT_Enable();
	if (!shutdown) {
		wdt_enable = 1;
		if (autostrobe)
			mc_event_register(-1, MC_SOCK_ANY, wdt_auto_strobe, NULL);
	}
}

void
xs_wdt_stop(xsMachine *the)
{
	if (wdt_enable) {
		CLK_ModuleClkDisable(CLK_WDT);
		WDT_Disable();
		wdt_enable = 0;
		mc_event_unregister(-1);
	}
}

void
xs_wdt_strobe(xsMachine *the)
{
	wdt_auto_strobe(-1, 0, NULL);
}

#else
void
xs_wdt_destructor(void *data)
{
}

void
xs_wdt_start(xsMachine *the)
{
}

void
xs_wdt_stop(xsMachine *the)
{
}

void
xs_wdt_strobe(xsMachine *the)
{
}
#endif
