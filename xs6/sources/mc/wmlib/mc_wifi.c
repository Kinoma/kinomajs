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

#include "mc_connection.h"
#include "mc_event.h"
#include "mc_misc.h"
#include "mc_wmsdk.h"

mc_connection_t mc_wm_connection = {0};

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

int
mc_wm_init()
{
	int err;

	/* wm_wlan_init is supposed to initialize all necessary drivers */
	if ((err = wm_wlan_init()) != WM_SUCCESS) {
		if (err == -WIFI_ERROR_FW_NOT_DETECTED) {
			mc_log_error("wlan: wifi fw not detected! trying to use passive partition\n");
			mc_set_active_partition("wififw");
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

void
mc_wm_fin()
{
	wlan_stop();
}

extern void mc_wm_event_callback(xsMachine *the, void *data);

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

int
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

void
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

int
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

int
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

extern void mc_wm_scan_callback(xsMachine *the, void *data);

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

int
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

int
mc_wm_get_ipconfig()
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
	mc_wm_connection.dns.s_addr = ipconfig.dns1 == IPADDR_ANY ? ipconfig.dns2 : ipconfig.dns1;
#else
	mc_wm_connection.ipaddr.s_addr = ipconfig.ipv4.address;
	mc_wm_connection.dns.s_addr = ipconfig.ipv4.dns1 == IPADDR_ANY ? ipconfig.ipv4.dns2 : ipconfig.ipv4.dns1;
#endif
	return 0;
}

int
mc_wm_get_bssid(uint8_t *bssid)
{
	struct wlan_network network;

	if (wlan_get_current_network(&network) != WM_SUCCESS)
		return -1;
	memcpy(bssid, network.bssid, 6);
	return 0;
}

struct mc_connection_config *
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

int
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

void
mc_wm_set_mac_addr(const uint8_t *mac)
{
	static uint8_t s_mac[6];	/* wmsdk keeps the reference! */

	memcpy(s_mac, mac, sizeof(s_mac));
	wlan_set_mac_addr(s_mac);
	backup_s.sta_mac_addr1 = ((uint32_t)mac[0] << 24) | ((uint32_t)mac[1] << 16) | ((uint32_t)mac[2] << 8) | (uint32_t)mac[3];
	backup_s.sta_mac_addr2 = ((uint32_t)mac[4] << 24) | ((uint32_t)mac[5] << 16);
}

void
mc_wm_get_mac_addr(uint8_t *mac)
{
	wlan_get_mac_address(mac);
}

/*
 * wifi
 */
void
mc_wifi_add_mcast_filter(struct in_addr *addr)
{
	uint8_t mcast_mac[MLAN_MAC_ADDR_LENGTH];

	wifi_get_ipv4_multicast_mac(ntohl(addr->s_addr), mcast_mac);
	wifi_add_mcast_filter(mcast_mac);	/* should be ok to 'add' the address every time... */
}

/*
 * wlan
 */
int
mc_wlan_set_mgmt_ie_221(uint8_t *ie, size_t sz)
{
	return wlan_set_mgmt_ie(mc_wm_connection.mode == MC_CONNECTION_MODE_UAP ? WLAN_BSS_TYPE_UAP : WLAN_BSS_TYPE_STA, MGMT_VENDOR_SPECIFIC_221, ie, sz);
}

void
mc_wlan_clear_mgmt_ie(int index)
{
	wlan_clear_mgmt_ie(mc_wm_connection.mode == MC_CONNECTION_MODE_UAP ? WLAN_BSS_TYPE_UAP : WLAN_BSS_TYPE_STA, index);
}

#else	/* !mxMC */

int
mc_wm_init()
{
	mc_wm_connection.state = MC_CONNECTION_UNINITIALIZED;
	mc_wm_connection.aps = NULL;
	return 0;
}

void
mc_wm_fin()
{
	mc_wm_connection.state = MC_CONNECTION_UNINITIALIZED;
}

int
mc_wm_start()
{
	mc_wm_connection.state = MC_CONNECTION_INITIALIZED;
	return 0;
}

void
mc_wm_set_dhcp_name(const char *name)
{
}

int
mc_wm_connect()
{
	mc_wm_connection.state = MC_CONNECTION_CONNECTED;
	mc_wm_connection.mode = MC_CONNECTION_MODE_STA;
	return 0;
}

int
mc_wm_disconnect()
{
	mc_wm_connection.state = MC_CONNECTION_DISCONNECTED;
	return 0;
}

int
mc_wm_scan(int rescan)
{
	mc_wm_connection.state = MC_CONNECTION_SCANNED;
	mc_wm_connection.aps = NULL;
	return 0;
}

struct mc_connection_config *
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

int
mc_wm_getRSSI()
{
	return 0;
}

int
mc_wm_get_bssid(uint8_t *bssid)
{
	memset(bssid, 0, 6);
	return 0;
}

void
mc_wm_set_mac_addr(const uint8_t *mac)
{
}

#endif	/* mxMC */
