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
#include "mc_misc.h"
#include "mc_env.h"
#include "mc_event.h"
#include "mc_connection.h"
#include "mc_module.h"
#include "mc_ipc.h"
#include "mc_file.h"

#if !XS_ARCHIVE
#include "xm_wifi.xs.c"
MC_MOD_DECL(wifi);
#endif

static mc_connection_t mc_wm_connection = {0};

#define MC_WM_ENV_PATH	"wifi"

#if mxMC
#include <wm_os.h>
#include <wm_net.h>
#if WMSDK_VERSION < 3000000
#include <arch/sys.h>
#else
#include <wm_wlan.h>
#endif
#include <wlan.h>
#if WMSDK_VERSION < 2012000
#include <wifi_drv.h>
#endif
#include <wmstats.h>


#define MC_WM_PROVISIONING_ADDR	"192.168.0.1"
#define MC_WM_PROVISIONING_MASK	"255.255.255.0"

#define MC_WM_MAX_SSIDS	10

#define MC_WM_TIME_INTERVAL	100	/* in msec */
#define MC_WM_TIMEOUT		10000	/* 10 sec */
#define MC_WM_CONNECTION_RETRY	10

#if WMSDK_VERSION < 2012000
#define is_uap_mode(network)	(network->mode == WLAN_MODE_UAP)
#else
#define is_uap_mode(network)	(network->role == WLAN_BSS_ROLE_UAP)
#endif

#ifndef howmany
#define howmany(x, y)	((x + (y) - 1) / (y))
#endif

static int
mc_wm_init()
{
	int err;

	/* wm_wlan_init is supposed to initialize all necessary drivers */
	if ((err = wm_wlan_init()) != WM_SUCCESS) {
		if (err == -WIFI_ERROR_FW_NOT_DETECTED) {
			mc_log_error("wlan: wifi fw not detected! trying to use passive partition\n");
			mc_set_active_volume("wififw");
			err = wm_wlan_init();
		}
		if (err != WM_SUCCESS) {
			mc_log_error("wlan init failed: %d\n", err);
			return -1;
		}
	}
	mc_wm_connection.state = MC_CONNECTION_UNINITIALIZED;
	return 0;
}

static void
mc_wm_fin()
{
	wlan_stop();
}

static void mc_wm_event_callback(xsMachine *the, void *data);

static int
mc_wm_event_handler(enum wlan_event_reason reason, void *data)
{
	mc_connection_state_t state;

	switch (reason) {
	case WLAN_REASON_INITIALIZED:
		state = MC_CONNECTION_INITIALIZED;
		break;
	case WLAN_REASON_INITIALIZATION_FAILED:
		state = MC_CONNECTION_FATAL;
		break;
	case WLAN_REASON_SUCCESS:
		state = MC_CONNECTION_CONNECTED;
		break;
	case WLAN_REASON_NETWORK_NOT_FOUND:
		state = MC_CONNECTION_ERROR;
		break;
	case WLAN_REASON_NETWORK_AUTH_FAILED:
		state = MC_CONNECTION_ERROR;
		break;
	case WLAN_REASON_ADDRESS_SUCCESS:
		state = MC_CONNECTION_CONNECTED;
		break;
	case WLAN_REASON_ADDRESS_FAILED:
		state = MC_CONNECTION_ERROR;
		break;
	case WLAN_REASON_USER_DISCONNECT:
		state = MC_CONNECTION_DISCONNECTED;
		break;
	case WLAN_REASON_LINK_LOST:
		state = MC_CONNECTION_DISCONNECTED;
		break;
	case WLAN_REASON_UAP_SUCCESS:
		state = MC_CONNECTION_CONNECTED;
		break;
	case WLAN_REASON_UAP_STOPPED:
		state = MC_CONNECTION_DISCONNECTED;
		break;
#if WMSDK_VERSION >= 2014037
	case WLAN_REASON_UAP_CLIENT_ASSOC:
	case WLAN_REASON_UAP_CLIENT_DISSOC:
		state = mc_wm_connection.state;	/* nothing to do */
		break;
	case WLAN_REASON_UAP_START_FAILED:
		state = MC_CONNECTION_FATAL;
		break;
	case WLAN_REASON_UAP_STOP_FAILED:
		/* safe to just ignore and pretend it's succeeded? */
		state = MC_CONNECTION_DISCONNECTED;
		break;
#endif
	default:
		state = mc_wm_connection.state;	/* nothing to do */
		break;
	}
	if (mc_wm_connection.state == MC_CONNECTION_CONNECTED) {
		/* the process is likely to be running */
		mc_event_thread_call(mc_wm_event_callback, (void *)state, 0);
	}
	mc_wm_connection.state = state;
	return 0;
}

static int
mc_wm_start()
{
	int err;

	if ((err = wlan_start(mc_wm_event_handler)) != WLAN_ERROR_NONE) {
		mc_log_error("wlan_start failed: %d\n", err);
		return -1;
	}
	mc_wm_connection.config.state = MC_CONFIG_STATE_NONE;
	mc_wm_connection.state = MC_CONNECTION_PENDING;
	return 0;
}

static void
mc_wm_set_dhcp_name(const char *name)
{
	static char hostname[HOST_NAME_MAX + 1];	/* looks like net_dhcp_hostname_set only keeps the reference */

	strncpy(hostname, name, HOST_NAME_MAX);
	hostname[HOST_NAME_MAX] = '\0';
	net_dhcp_hostname_set(hostname);
}

static int
mc_wm_do_connection(struct wlan_network *network)
{
	int err;

	if (mc_wm_connection.network_name[0] != '\0') {
		if ((err = wlan_remove_network(mc_wm_connection.network_name)) != WLAN_ERROR_NONE)
			mc_log_error("wlan_remove_network failed: %d\n", err);
			/* try to continue... */
	}
	if ((err = wlan_add_network(network)) != WLAN_ERROR_NONE) {
		mc_log_error("wlan_add_network failed: %d\n", err);
		return -1;
	}
	/* will disconnect automatically if the network has already been connected */
	if (is_uap_mode(network)) {
		if ((err = wlan_start_network(network->name)) != WLAN_ERROR_NONE) {
			mc_log_error("wlan_start_network failed: %d\n", err);
			wlan_remove_network(network->name);
			return -1;
		}
		mc_wm_connection.mode = MC_CONNECTION_MODE_UAP;
	}
	else {
		if ((err = wlan_connect(network->name)) != WLAN_ERROR_NONE) {
			mc_log_error("wlan_connect failed: %d\n", err);
			wlan_remove_network(network->name);
			return -1;
		}
		mc_wm_connection.mode = MC_CONNECTION_MODE_STA;
	}
	strncpy(mc_wm_connection.network_name, network->name, sizeof(mc_wm_connection.network_name));
	mc_wm_connection.state = MC_CONNECTION_PENDING;
	return 0;
}

static int
mc_wm_connect()
{
	struct mc_connection_config *conf = &mc_wm_connection.config;
	struct wlan_network *network;
	char hostname[HOST_NAME_MAX + 1];
	int status = 0;

	if ((network = mc_calloc(1, sizeof(struct wlan_network))) == NULL) {
		errno = ENOMEM;
		return -1;
	}
	network->ssid_specific = 1;
	network->channel = 0;
	network->security.pmk_valid = 0;
	switch (conf->state) {
	case MC_CONFIG_STATE_UAP:
#if WMSDK_VERSION < 2012000
		network->mode = WLAN_MODE_UAP;
#else
		network->type = WLAN_BSS_TYPE_UAP;
		network->role = WLAN_BSS_ROLE_UAP;
#endif
#if WMSDK_VERSION <= 2013082
		network->address.addr_type = ADDR_TYPE_STATIC;
		network->address.ip = inet_addr(MC_WM_PROVISIONING_ADDR);
		network->address.gw = inet_addr(MC_WM_PROVISIONING_ADDR);
		network->address.netmask = inet_addr(MC_WM_PROVISIONING_MASK);
#else
		network->ip.ipv4.addr_type = ADDR_TYPE_STATIC;
		network->ip.ipv4.address = inet_addr(MC_WM_PROVISIONING_ADDR);
		network->ip.ipv4.gw = inet_addr(MC_WM_PROVISIONING_ADDR);
		network->ip.ipv4.netmask = inet_addr(MC_WM_PROVISIONING_MASK);
#endif
		mc_gethostname(hostname, sizeof(hostname));
		strncpy(network->ssid, hostname, sizeof(network->ssid));	/* use the hostname as the SSID */
		strncpy(network->name, network->ssid, sizeof(network->name));
		memset(network->bssid, 0, 6);
#if 0
		network->security.type = WLAN_SECURITY_WPA2;
		strcpy(network->security.psk, "marvellwm");
		network->security.psk_len = strlen(network->security.psk);
#else
		network->security.type = WLAN_SECURITY_NONE;
#endif
		status = mc_wm_do_connection(network);
		break;
	case MC_CONFIG_STATE_PREFERRED_NETWORK:
	case MC_CONFIG_STATE_STA:
#if WMSDK_VERSION < 2012000
		network->mode = WLAN_MODE_INFRASTRUCTURE;
#else
		network->type = WLAN_BSS_TYPE_STA;
		network->role = WLAN_BSS_ROLE_STA;
#endif
#if WMSDK_VERSION <= 2013082
		network->address.addr_type = ADDR_TYPE_DHCP;
#else
		network->ip.ipv4.addr_type = ADDR_TYPE_DHCP;
#endif
		/* connect a configured network */
		strncpy(network->name, conf->ssid, sizeof(network->name));
		strncpy(network->ssid, conf->ssid, sizeof(network->ssid));
		memcpy(network->bssid, conf->bssid, 6);
		switch (conf->security) {
		default:
		case MC_CONFIG_SECURITY_NONE:
			network->security.type = WLAN_SECURITY_NONE;
			break;
		case MC_CONFIG_SECURITY_WPA:
			network->security.type = WLAN_SECURITY_WPA;
			break;
		case MC_CONFIG_SECURITY_WPA2:
			network->security.type = WLAN_SECURITY_WPA2;
			break;
		}
		if (conf->password != NULL) {
			strncpy(network->security.psk, conf->password, sizeof(network->security.psk));
			network->security.psk_len = strlen(conf->password);
		}
		else
			network->security.psk_len = 0;
		status = mc_wm_do_connection(network);
		break;
	default:
		status = -1;
		break;
	}
	mc_free(network);
	return status;
}

static int
mc_wm_disconnect()
{
	int err;

	if (mc_wm_connection.state != MC_CONNECTION_CONNECTED)
		return 0;
	if (mc_wm_connection.mode & MC_CONNECTION_MODE_UAP)
		err = wlan_stop_network(mc_wm_connection.network_name);
	else if (mc_wm_connection.mode & MC_CONNECTION_MODE_STA)
		err = wlan_disconnect();
	else {
		mc_log_error("wifi.disconnect: bad mode: 0x%x\n", mc_wm_connection.mode);
		mc_wm_connection.state = MC_CONNECTION_ERROR;
		return 0;
	}
	if (err != WLAN_ERROR_NONE) {
		mc_log_error("wm_wlan_disconnect: disconnect error = %d\n", err);
		return -1;
	}
	mc_wm_connection.state = MC_CONNECTION_PENDING;
	return 0;
}

static void mc_wm_scan_callback(xsMachine *the, void *data);

static int
mc_wm_scan_cb(unsigned int count)
{
	unsigned int i;
	struct wlan_scan_result *aps = NULL;
	int num_aps = 0;

	if (count != 0) {
		if ((aps = mc_malloc(count * sizeof(struct wlan_scan_result))) != NULL) {
			for (i = 0; i < count; i++) {
				if (wlan_get_scan_result(i, &aps[num_aps]) == WLAN_ERROR_NONE)
					num_aps++;
			}
		}
	}
	mc_wm_connection.aps = aps;
	mc_wm_connection.num_aps = num_aps;
	if (mc_wm_connection.state == MC_CONNECTION_PENDING)
		mc_wm_connection.state = MC_CONNECTION_SCANNED;
	else if (mc_wm_connection.state == MC_CONNECTION_CONNECTED)
		mc_event_thread_call(mc_wm_scan_callback, NULL, 0);
	return 0;
}

static int
mc_wm_scan(int rescan)
{
	int err;

	if (mc_wm_connection.aps != NULL) {
		mc_free(mc_wm_connection.aps);
		mc_wm_connection.aps = NULL;
		mc_wm_connection.num_aps = 0;
	}
	if ((err = wlan_scan(mc_wm_scan_cb)) != WLAN_ERROR_NONE) {
		mc_log_error("wlan_scan failed: %d\n", err);
		return -1;
	}
	if (!rescan)
		mc_wm_connection.state = MC_CONNECTION_PENDING;
	return 0;
}

static int
mc_wm_get_address()
{
	struct wlan_ip_config ipconfig;
	int err;

	if (mc_wm_connection.mode & MC_CONNECTION_MODE_UAP)
		err = wlan_get_uap_address(&ipconfig);
	else
		err = wlan_get_address(&ipconfig);
	if (err != WLAN_ERROR_NONE) {
		mc_wm_connection.ipaddr.s_addr = 0;
		mc_log_error("wlan_get_address failed: %d\n", err);
		return -1;
	}
#if WMSDK_VERSION <= 2013082
	mc_wm_connection.ipaddr.s_addr = ipconfig.ip;
#else
	mc_wm_connection.ipaddr.s_addr = ipconfig.ipv4.address;
#endif
	return 0;
}

static int
mc_wm_get_bssid(uint8_t *bssid)
{
	struct wlan_network network;

	if (wlan_get_current_network(&network) != WM_SUCCESS)
		return -1;
	memcpy(bssid, network.bssid, 6);
	return 0;
}

static struct mc_connection_config *
mc_wm_ap_config(int i)
{
	static struct mc_connection_config ret;
	struct wlan_scan_result *sr;

	if (i >= mc_wm_connection.num_aps)
		return NULL;
	sr = mc_wm_connection.aps;
	sr += i;
	ret.ssid = sr->ssid;
	ret.security = sr->wpa2 ? MC_CONFIG_SECURITY_WPA2 : sr->wpa ? MC_CONFIG_SECURITY_WPA : MC_CONFIG_SECURITY_NONE;
	memcpy(ret.bssid, sr->bssid, 6);
	return &ret;
}

static int
mc_wm_getRSSI()
{
	short rssi;

	wlan_get_current_rssi(&rssi);
	return rssi;
}

/* copied from wifi-sdio.h */
typedef struct __nvram_backup_struct {
	t_u32 ioport;
	t_u32 curr_wr_port;
	t_u32 curr_rd_port;
	t_u32 mp_end_port;
	t_u32 bss_num;
	t_u32 sta_mac_addr1;
	t_u32 sta_mac_addr2;
	t_u32 wifi_state;
} nvram_backup_t;
static nvram_backup_t backup_s __attribute__((section(".nvram")));

static void
mc_wm_set_mac_addr()
{
	unsigned char *mac = mc_wm_connection.mac;

	wlan_set_mac_addr(mc_wm_connection.mac);
	backup_s.sta_mac_addr1 = ((uint32_t)mac[0] << 24) | ((uint32_t)mac[1] << 16) | ((uint32_t)mac[2] << 8) | (uint32_t)mac[3];
	backup_s.sta_mac_addr2 = ((uint32_t)mac[4] << 24) | ((uint32_t)mac[5] << 16);
}

#else	/* !mxMC */

static int
mc_wm_init()
{
	mc_wm_connection.state = MC_CONNECTION_UNINITIALIZED;
	mc_wm_connection.aps = NULL;
	return 0;
}

static void
mc_wm_fin()
{
	mc_wm_connection.state = MC_CONNECTION_UNINITIALIZED;
}

static int
mc_wm_start()
{
	mc_wm_connection.state = MC_CONNECTION_INITIALIZED;
	return 0;
}

static void
mc_wm_set_dhcp_name(const char *name)
{
}

static int
mc_wm_connect()
{
	mc_wm_connection.state = MC_CONNECTION_CONNECTED;
	mc_wm_connection.mode = MC_CONNECTION_MODE_STA;
	return 0;
}

static int
mc_wm_disconnect()
{
	mc_wm_connection.state = MC_CONNECTION_DISCONNECTED;
	return 0;
}

static int
mc_wm_scan(int rescan)
{
	mc_wm_connection.state = MC_CONNECTION_SCANNED;
	mc_wm_connection.aps = NULL;
	return 0;
}

static struct mc_connection_config *
mc_wm_ap_config(int i)
{
	static struct mc_connection_config ret;

	if (i > 0)
		return NULL;
	ret.ssid = "element test ap";
	ret.security = MC_CONFIG_SECURITY_WPA2;
	memcpy(ret.bssid, "012345", 6);
	return &ret;
}

static int
mc_wm_getRSSI()
{
	return 0;
}

static int
mc_wm_get_bssid(uint8_t *bssid)
{
	memset(bssid, 0, 6);
	return 0;
}

static void
mc_wm_set_mac_addr()
{
}

#endif	/* mxMC */

/*
 * xs functions
 */

void
xs_wm_load(xsMachine *the)
{
	xsVars(1);
	/* config */
	xsSetInteger(xsVar(0), MC_CONNECTION_CONFIG_NORMAL);
	xsNewHostProperty(xsThis, xsID("NORMAL"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_CONFIG_FALLBACK);
	xsNewHostProperty(xsThis, xsID("FALLBACK"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	/* mode */
	xsSetInteger(xsVar(0), MC_CONNECTION_MODE_STA);
	xsNewHostProperty(xsThis, xsID("STA"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_MODE_UAP);
	xsNewHostProperty(xsThis, xsID("UAP"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	/* status */
	xsSetInteger(xsVar(0), MC_CONNECTION_CONNECTED);
	xsNewHostProperty(xsThis, xsID("CONNECTED"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_DISCONNECTED);
	xsNewHostProperty(xsThis, xsID("DISCONNECTED"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_ERROR);
	xsNewHostProperty(xsThis, xsID("ERROR"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_INITIALIZED);
	xsNewHostProperty(xsThis, xsID("INITIALIZED"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_UNINITIALIZED);
	xsNewHostProperty(xsThis, xsID("UNINITIALIZED"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_SCANNED);
	xsNewHostProperty(xsThis, xsID("SCANNED"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_NOTRUNNING);
	xsNewHostProperty(xsThis, xsID("NOTRUNNING"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_PENDING);
	xsNewHostProperty(xsThis, xsID("PENDING"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
}

void
xs_wm_init(xsMachine *the)
{
	if (mc_wm_init() != 0)
		mc_xs_throw(the, "wifi: init failed");
}

void
xs_wm_fin(xsMachine *the)
{
	mc_wm_fin();
}

void
xs_wm_start(xsMachine *the)
{
	if (mc_wm_start() != 0)
		mc_xs_throw(the, "wifi: start failed");
}

void
xs_wm_setDHCP(xsMachine *the)
{
	mc_wm_set_dhcp_name(xsToString(xsArg(0)));
}

static int
mc_setup_config(xsMachine *the, mc_connection_config_t *conf)
{
	xsIndex id_ssid = xsID("ssid");
	xsIndex id_bssid = xsID("bssid");
	xsIndex id_security = xsID("security");
	xsIndex id_password = xsID("password");
	xsIndex id_hidden = xsID("hidden");
	xsIndex id_save = xsID("save");
	xsIndex id_mode = xsID("mode");

	xsVars(1);
	/* SSID and PSK can contain any octet including '\0' but assume they are a string here. */
	if (!xsHas(xsArg(0), id_ssid))
		goto bail;
	xsGet(xsVar(0), xsArg(0), id_ssid);
	if (conf->ssid != NULL)
		mc_free(conf->ssid);
	if ((conf->ssid = mc_strdup(xsToString(xsVar(0)))) == NULL)
		goto bail;
	if (xsHas(xsArg(0), id_bssid)) {
		char *p;
		int i;
		xsGet(xsVar(0), xsArg(0), id_bssid);
		p = xsToString(xsVar(0));
		for (i = 0; i < 6; i++) {
			int x;
			sscanf(p, "%02x", &x);
			p += 2;
			conf->bssid[i] = x;
		}
	}
	else
		memset(conf->bssid, 0, 6);
	if (xsHas(xsArg(0), id_security)) {
		xsGet(xsVar(0), xsArg(0), id_security);
		if (xsTypeOf(xsVar(0)) == xsStringType) {
			char *s = xsToString(xsVar(0));
			if (strcasecmp(s, "none") == 0)
				conf->security = MC_CONFIG_SECURITY_NONE;
			else if (strcasecmp(s, "wpa") == 0)
				conf->security = MC_CONFIG_SECURITY_WPA;
			else if (strcasecmp(s, "wpa2") == 0)
				conf->security = MC_CONFIG_SECURITY_WPA2;
			else
				conf->security = MC_CONFIG_SECURITY_NONE;
		}
		else
			conf->security = xsToInteger(xsVar(0));
	}
	else
		conf->security = MC_CONFIG_SECURITY_NONE;
	if (conf->security != MC_CONFIG_SECURITY_NONE) {
		if (!xsHas(xsArg(0), id_password))
			goto bail;
		xsGet(xsVar(0), xsArg(0), id_password);
		if (conf->password != NULL)
			mc_free(conf->password);
		if ((conf->password = mc_strdup(xsToString(xsVar(0)))) == NULL)
			goto bail;
	}
	conf->flags = 0;
	if (xsHas(xsArg(0), id_hidden)) {
		xsGet(xsVar(0), xsArg(0), id_hidden);
		if (xsTest(xsVar(0)))
			conf->flags |= MC_CONFIG_FLAG_HIDDEN;
	}
	if (xsHas(xsArg(0), id_save)) {
		xsGet(xsVar(0), xsArg(0), id_save);
		if (xsTest(xsVar(0)))
			conf->flags |= MC_CONFIG_FLAG_SAVE;
	}
	if (xsHas(xsArg(0), id_mode)) {
		xsGet(xsVar(0), xsArg(0), id_mode);
		conf->mode = xsToInteger(xsVar(0));	/* undefined and null will be converted to 0 */
	}
	else
		conf->mode = 0;
	return 0;
bail:
	if (conf->ssid != NULL) {
		mc_free(conf->ssid);
		conf->ssid = NULL;
	}
	if (conf->password != NULL) {
		mc_free(conf->password);
		conf->password = NULL;
	}
	conf->flags = 0;
	conf->mode = 0;
	return -1;
}

void
xs_wm_connect(xsMachine *the)
{
	switch (xsTypeOf(xsArg(0))) {
	case xsIntegerType:
	case xsNumberType: {
		uint32_t state = xsToInteger(xsArg(0));
		switch (state & MC_CONNECTION_MODE_MASK) {
		case MC_CONNECTION_CONFIG_NORMAL:
			mc_wm_connection.config.state = MC_CONFIG_STATE_PREFERRED_NETWORK;
			break;
		case MC_CONNECTION_CONFIG_FALLBACK:
			mc_wm_connection.config.state = MC_CONFIG_STATE_UAP;
			break;
		case MC_CONNECTION_CONFIG_STA:
			mc_wm_connection.config.state = MC_CONFIG_STATE_STA;
			break;
		}
		if (state >>= MC_CONNECTION_MODE_BITS)
			mc_wm_connection.config.mode = state;
		break;
	}
	case xsReferenceType:
		if (mc_setup_config(the, &mc_wm_connection.config) != 0)
			mc_xs_throw(the, "wifi.connect: conf error");
		mc_wm_connection.config.state = MC_CONFIG_STATE_STA;
		break;
	default:
		mc_xs_throw(the, "wifi.connect: bad args");
		break;
	}
	if (mc_wm_connection.state == MC_CONNECTION_CONNECTED)
		mc_event_shutdown();	/* shutdown all running servers so the event loop will terminate */
	else {
		if (mc_wm_connect() != 0)
			mc_xs_throw(the, "wifi: connection error");
	}
}

void
xs_wm_disconnect(xsMachine *the)
{
	if (mc_wm_disconnect() != 0)
		mc_xs_throw(the, "wifi: disconnection error");
}

void
xs_wm_close(xsMachine *the)
{
	mc_event_exit(0);
}

#if !mxMC
extern void xs_wm_getInterfaces(xsMachine *the);

static void
xs_wm_get_id(xsMachine *the, xsIndex id)
{
	xsIndex id_next = xsID("next");
	xsIndex id_done = xsID("done");
	xsIndex id_value = xsID("value");
	xsIndex id_LOOPBACK = xsID("LOOPBACK");

	xsVars(4);
	xs_wm_getInterfaces(the);
	xsVar(0) = xsEnumerate(xsResult);
	do {
		xsCall(xsVar(1), xsVar(0), id_next, NULL);
		xsGet(xsVar(2), xsVar(1), id_done);
		if (xsTest(xsVar(2)))
			break;
		xsGet(xsVar(2), xsVar(1), id_value);
		xsGetAt(xsVar(3), xsResult, xsVar(2));
		xsGet(xsVar(1), xsVar(3), id_LOOPBACK);
		xsGet(xsVar(2), xsVar(3), id);
		if (!xsTest(xsVar(1)) && xsTest(xsVar(2))) {
			xsResult = xsVar(2);
			return;
		}
	} while (1);
	xsSetUndefined(xsResult);
}
#endif

void
xs_wm_getIP(xsMachine *the)
{
#if mxMC
	(void)mc_wm_get_address();
	xsSetString(xsResult, inet_ntoa(mc_wm_connection.ipaddr));
#else
	xs_wm_get_id(the, xsID("addr"));
#endif
}

void
xs_wm_getMAC(xsMachine *the)
{
#if mxMC
	uint8_t *mac;
	char str[2*6+1];

	wlan_get_mac_address(mc_wm_connection.mac);
	mac = mc_wm_connection.mac;
	snprintf(str, sizeof(str), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	xsSetString(xsResult, str);
#else
	xs_wm_get_id(the, xsID("mac"));
#endif
}

#if !mxMC
static char *
get_resolv_conf()
{
	FILE *fp;
	char buf[128], *p;
	static char res[128];

	if ((fp = fopen("/etc/resolv.conf", "r")) == NULL)
		return NULL;
	while ((p = fgets(buf, sizeof(buf), fp)) != NULL) {
		if (sscanf(p, "nameserver %s", res) == 1)
			break;
	}
	fclose(fp);
	return p ? res : NULL;
}
#endif

void
xs_wm_getInterfaces(xsMachine *the)
{
#if mxMC
	struct wlan_ip_config ifaddr;
	uint32_t dns;

	xsVars(2);
	xsSetNewInstanceOf(xsVar(0), xsObjectPrototype);
	xsSetTrue(xsVar(1));
	xsSet(xsVar(0), xsID("UP"), xsVar(1));
	xsSet(xsVar(0), xsID("MULTICAST"), xsVar(1));
	xs_wm_getIP(the);
	xsSet(xsVar(0), xsID("addr"), xsResult);
	xs_wm_getMAC(the);
	xsSet(xsVar(0), xsID("mac"), xsResult);
	/* set the DHCP servers */
	if (is_uap_started())
		wlan_get_uap_address(&ifaddr);
	else
		wlan_get_address(&ifaddr);
	dns = ifaddr.ipv4.dns1 == IPADDR_ANY ? ifaddr.ipv4.dns2 : ifaddr.ipv4.dns1;
	if (dns != IPADDR_ANY) {
		xsSetString(xsVar(1), inet_ntoa(dns));
		xsSet(xsVar(0), xsID("dns"), xsVar(1));
	}
	/* there exists only one interface */
	xsSetNewInstanceOf(xsResult, xsObjectPrototype);
	xsSet(xsResult, xsID("wm0"), xsVar(0));
#elif mxMacOSX
	struct ifaddrs *iflist, *ifa;
	xsIndex id_name;
	char *dns;

	if (getifaddrs(&iflist) != 0)
		return;
	xsVars(2);
	xsSetNewInstanceOf(xsResult, xsObjectPrototype);
	for (ifa = iflist; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;
		id_name = xsID(ifa->ifa_name);
		if (!xsHas(xsResult, id_name)) {
			xsSetNewInstanceOf(xsVar(0), xsObjectPrototype);
			xsSet(xsResult, id_name, xsVar(0));
		}
		else
			xsGet(xsVar(0), xsResult, id_name);
		if (ifa->ifa_addr->sa_family == AF_LINK) {	/* MAC address */
			char str[2*6+1];
			uint8_t *mac = (uint8_t *)LLADDR((struct sockaddr_dl *)ifa->ifa_addr);
			if (memcmp(mac, "\0\0\0\0\0\0", 6) != 0) {	/* workaround... */
				snprintf(str, sizeof(str), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				xsSetString(xsVar(1), str);
				xsSet(xsVar(0), xsID("mac"), xsVar(1));
			}
		}
		else if (ifa->ifa_addr->sa_family == AF_INET) {	/* interface (IP) address */
			struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
			xsSetString(xsVar(1), inet_ntoa(sin->sin_addr));
			xsSet(xsVar(0), xsID("addr"), xsVar(1));
		}
		xsSetBoolean(xsVar(1), (ifa->ifa_flags & IFF_UP) != 0);
		xsSet(xsVar(0), xsID("UP"), xsVar(1));
		xsSetBoolean(xsVar(1), (ifa->ifa_flags & IFF_LOOPBACK) != 0);
		xsSet(xsVar(0), xsID("LOOPBACK"), xsVar(1));
		xsSetBoolean(xsVar(1), (ifa->ifa_flags & IFF_MULTICAST) != 0);
		xsSet(xsVar(0), xsID("MULTICAST"), xsVar(1));
		/* how can we get DNS servers for each interface? */
		if ((ifa->ifa_flags & (IFF_UP | IFF_LOOPBACK)) == IFF_UP && xsHas(xsVar(0), xsID("addr"))) {
			if ((dns = get_resolv_conf()) != NULL) {
				xsSetString(xsVar(1), dns);
				xsSet(xsVar(0), xsID("dns"), xsVar(1));
			}
		}
	}
	freeifaddrs(iflist);
#else
	int s;
	struct ifconf ifc;
	struct ifreq *ifr;
	struct sockaddr_in *sin;
	char *dns;
#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif
#define NEXT_IFR(ifr)	((caddr_t)ifr + MAX(ifr->ifr_addr.sa_len + sizeof(ifr->ifr_name), sizeof(*ifr)))

	xsVars(2);
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0)
		return;
	ifc.ifc_len = 0;	/* to get the actual size */
	ifc.ifc_req = NULL;
	if (ioctl(s, SIOCGIFCONF, &ifc) != 0 || (ifc.ifc_buf = mc_malloc(ifc.ifc_len)) == NULL) {
		close(s);
		return;
	}
	ioctl(s, SIOGIFCONF, &ifc);
	xsSetNewInstanceOf(xsResult, xsObjectPrototype);
	for (ifr = ifc.ifc_req; (caddr_t)ifr < ifc.ifc_buf + ifc.ifc_len; ifr = NEXT_IFR(ifr)) {
		char str[2*6+1];
		uint8_t *mac;
		if (ifr->ifr_addr.sa_family != AF_LINK)
			continue;
		id_name = xsID(ifr->ifr_name);
		if (!xsHas(xsResult, id_name)) {
			xsSetNewInstanceOf(xsVar(0), xsObjectPrototype);
			xsSet(xsResult, id_name, xsVar(0));
		}
		else
			xsVar(0) = xsGet(xsResult, id_name);
		mac = LLADDR((struct sockaddr_dl *)&ifr->ifr_addr);
		snprintf(str, sizeof(str), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		xsSetString(xsVar(1), str);
		xsSet(xsVar(0), xsID("mac"), xsVar(1));
		if (ioctl(s, SIOCGIFFLAGS, ifr) != 0)
			continue;
		if (ioctl(s, SIOCGIFADDR, ifr) != 0)
			continue;
		sin = (struct sockaddr_in *)&ifr->ifr_addr;
		xsSetString(xsVar(1), inet_ntoa(sin->sin_addr));
		xsSet(xsVar(0), xsID("addr"), xsVar(1));
		xsSetBoolean(xsVar(1), (ifr->ifr_flags & IFF_UP) != 0);
		xsSet(xsVar(0), xsID("UP"), xsVar(1));
		xsSetBoolean(xsVar(1), (fr->ifr_flags & IFF_LOOPBACK) != 0);
		xsSet(xsVar(0), xsID("LOOPBACK"), xsVar(1));
		xsSetBoolean(xsVar(1), (ifr->ifr_flags & IFF_MULTICAST) != 0);
		xsSet(xsVar(0), xsID("MULTICAST"), xsVar(1));
		if ((ifr->ifr_flags & (IFF_UP | IFF_LOOPBACK)) == IFF_UP) {
			if ((dns = get_resolv_conf()) != NULL) {
				xsSetString(xsVar(1), dns);
				xsSet(xsVar(0), xsID("dns"), xsVar(1));
			}
		}
	}
	mc_free(ifc.ifc_buf);
	close(s);
#endif
}

void
xs_wm_setMAC(xsMachine *the)
{
	size_t sz = xsGetArrayBufferLength(xsArg(0));
	void *data = xsToArrayBuffer(xsArg(0));

	if (sz != 6)
		mc_xs_throw(the, "wifi: bad mac");
	memcpy(mc_wm_connection.mac, data, sz);
	mc_wm_set_mac_addr();
}

void
xs_wm_getRSSI(xsMachine *the)
{
	if (mc_wm_connection.state == MC_CONNECTION_CONNECTED)
		xsSetInteger(xsResult, mc_wm_getRSSI());
}

void
xs_wm_scan(xsMachine *the)
{
	int ac = xsToInteger(xsArgc);
	int i;
	struct mc_connection_config *conf;
	char *ss;
	uint8_t *p;
	char bssid[13];
	int rescan = 0;

	if (ac > 0) {
		switch (xsTypeOf(xsArg(0))) {
		case xsIntegerType:
		case xsNumberType:
		case xsBooleanType:
			rescan = xsTest(xsArg(0));
			break;
		case xsReferenceType:
			if (xsIsInstanceOf(xsArg(0), xsFunctionPrototype)) {
				xsSet(xsThis, xsID("scanCallback"), xsArg(0));
				rescan = 1;
			}
			break;
		default:
			break;
		}
	}
	if (rescan) {
		/* re-scan the access points */
		if (mc_wm_scan(mc_wm_connection.state == MC_CONNECTION_CONNECTED) != 0)
			mc_xs_throw(the, "wifi: scan failed");
		return;
	}
	xsVars(2);
	xsSetNewInstanceOf(xsResult, xsArrayPrototype);
	for (i = 0; (conf = mc_wm_ap_config(i)) != NULL; i++) {
		xsSetNewInstanceOf(xsVar(0), xsObjectPrototype);
		xsSetString(xsVar(1), conf->ssid ? conf->ssid : "");
		xsSet(xsVar(0), xsID("ssid"), xsVar(1));
		p = conf->bssid;
		snprintf(bssid, sizeof(bssid), "%02x%02x%02x%02x%02x%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
		xsSetString(xsVar(1), bssid);
		xsSet(xsVar(0), xsID("bssid"), xsVar(1));
		switch (conf->security) {
		case MC_CONFIG_SECURITY_WPA2:
			ss = "wpa2";
			break;
		case MC_CONFIG_SECURITY_WPA:
			ss = "wpa";
			break;
		default:
			ss = "none";
			break;
		}
		xsSetString(xsVar(1), ss);
		xsSet(xsVar(0), xsID("security"), xsVar(1));
		xsSetInteger(xsVar(1), i);
		xsSetAt(xsResult, xsVar(1), xsVar(0));
	}
}

void
xs_wm_get_status(xsMachine *the)
{
	xsSetInteger(xsResult, mc_wm_connection.state);
}

void
xs_wm_set_status(xsMachine *the)
{
	mc_wm_connection.state = xsToInteger(xsArg(0));
}

void
xs_wm_get_mode(xsMachine *the)
{
	xsSetInteger(xsResult, mc_wm_connection.mode | (mc_wm_connection.config.mode << MC_CONNECTION_MODE_BITS));
}

void
xs_wm_get_config(xsMachine *the)
{
	struct mc_connection_config *conf = &mc_wm_connection.config;
	int state;
	xsIndex id_ssid = xsID("ssid");
	xsIndex id_bssid = xsID("bssid");
	xsIndex id_security = xsID("security");
	xsIndex id_password = xsID("password");
	xsIndex id_hidden = xsID("hidden");
	xsIndex id_save = xsID("save");
	xsIndex id_mode = xsID("mode");
	xsIndex id_state = xsID("state");
	uint8_t bssid[6];
	char buf[13];

	xsVars(1);
	xsSetNewInstanceOf(xsResult, xsObjectPrototype);
	xsSetBoolean(xsVar(0), conf->flags & MC_CONFIG_FLAG_SAVE);
	xsSet(xsResult, id_save, xsVar(0));
	switch (conf->state) {
	default:
	case MC_CONFIG_STATE_NONE:
	case MC_CONFIG_STATE_PREFERRED_NETWORK:
		state = MC_CONNECTION_CONFIG_NORMAL;
		break;
	case MC_CONFIG_STATE_UAP:
		state = MC_CONNECTION_CONFIG_FALLBACK;
		break;
	case MC_CONFIG_STATE_STA:
		state = MC_CONNECTION_CONFIG_STA;
		break;
	}
	xsSetInteger(xsVar(0), state);
	xsSet(xsResult, id_state, xsVar(0));
	xsSetInteger(xsVar(0), conf->mode);
	xsSet(xsResult, id_mode, xsVar(0));

	/* AP parameters */
	xsSetString(xsVar(0), conf->ssid != NULL ? conf->ssid : "");	/* SSID is necessary in any case */
	xsSet(xsResult, id_ssid, xsVar(0));

	if (mc_wm_get_bssid(bssid) == 0) {
		uint8_t *p = bssid;
		snprintf(buf, sizeof(buf), "%02x%02x%02x%02x%02x%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
		xsSetString(xsVar(0), buf);
		xsSet(xsResult, id_bssid, xsVar(0));
	}
	if (conf->password != NULL) {
		xsSetString(xsVar(0), conf->password);
		xsSet(xsResult, id_password, xsVar(0));
	}
	xsSetInteger(xsVar(0), conf->security);
	xsSet(xsResult, id_security, xsVar(0));
	xsSetBoolean(xsVar(0), conf->flags & MC_CONFIG_FLAG_HIDDEN);
	xsSet(xsResult, id_hidden, xsVar(0));
}

void
xs_wm_get_ssid(xsMachine *the)
{
	xsSetString(xsResult, mc_wm_connection.network_name);
}

void
xs_wm_stats_sockets(xsMachine *the)
{
#if mxMC
	stats_sock_display();
	wmprintf("\r\n");
#endif
}

static void
mc_wm_event_callback(xsMachine *the, void *data)
{
	mc_connection_state_t state = (mc_connection_state_t)data;

	xsBeginHost(the);
	xsVars(2);
	xsGet(xsVar(0), xsGlobal, xsID("require"));
	xsSetString(xsVar(1), "wifi");
	xsCall(xsResult, xsVar(0), xsID("weak"), &xsVar(1), NULL);
	xsSetInteger(xsVar(1), state);
	xsCall_noResult(xsResult, xsID("notify"), &xsVar(1), NULL);
	xsEndHost(the);
}

static void
mc_wm_scan_callback(xsMachine *the, void *data)
{
	xsBeginHost(the);
	xsVars(2);
	xsGet(xsVar(0), xsGlobal, xsID("require"));
	xsSetString(xsVar(1), "wifi");
	xsCall(xsResult, xsVar(0), xsID("weak"), &xsVar(1), NULL);
	xsCall_noResult(xsResult, xsID("onScanComplete"), NULL);
	xsEndHost(the);
}
