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
#include "mc_xs.h"
#include "mc_stdio.h"
#include "mc_module.h"

#if !XS_ARCHIVE
#include "ext_dhcpd.xs.c"
MC_MOD_DECL(dhcpd);
#endif

#if mxMC
#include <wm_net.h>
#include <wmstats.h>
#include <dhcp-server.h>

/* used in WMSDK */
#undef errno
int errno;
struct wm_stats g_wm_stats;

static int dhcpd_running = 0;

void
xs_dhcpd_start(xsMachine *the)
{
	void *handler = net_get_uap_handle();
	int err = dhcp_server_start(handler);
	if (err != 0)
		mc_log_error("dhcp_server_start returned %d\n", err);
	dhcpd_running = err == 0;
}

void
xs_dhcpd_stop(xsMachine *the)
{
	if (dhcpd_running) {
		dhcp_server_stop();
		dhcpd_running = 0;
	}
}

void
xs_dhcpd_constructor(xsMachine *the)
{
	dhcpd_running = 0;
	xs_dhcpd_start(the);
}

void
xs_dhcpd_destructor(void *data)
{
	if (dhcpd_running) {
		dhcp_server_stop();
		dhcpd_running = 0;
	}
}
#else
void
xs_dhcpd_constructor(xsMachine *the)
{
}

void
xs_dhcpd_destructor(void *data)
{
}

void
xs_dhcpd_start(xsMachine *the)
{
}

void
xs_dhcpd_stop(xsMachine *the)
{
}
#endif
