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

static mc_connection_t mc_wm_connection;

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
mc_wlan_match(struct wlan_network *network, int hidden)
{
	struct wlan_scan_result *res = mc_wm_connection.aps;
	int i;

	for (i = 0; i < mc_wm_connection.num_aps; i++, res++) {
		if (strcmp(res->ssid, network->ssid) == 0 || (*res->ssid == '\0' && hidden)) {
			switch (network->security.type) {
			case WLAN_SECURITY_NONE:
				if (!res->wep && !res->wpa && !res->wpa2)
					return 1;
				break;
			case WLAN_SECURITY_WEP_OPEN:
			case WLAN_SECURITY_WEP_SHARED:
				if (res->wep)
					return 1;
				break;
			case WLAN_SECURITY_WPA:
				if (res->wpa)
					return 1;
				break;
			case WLAN_SECURITY_WPA2:
				if (res->wpa2 || (hidden && res->wep))	/* seems like wlan_get_scan_result returns WEP instead of WPA2 if the AP is hidden  */
					return 1;
				break;
			default:
				break;
			}
		}
	}
	return 0;
}

static int
mc_wm_wait_status()
{
	int i;
	mc_connection_state_t current_status = mc_wm_connection.state;

	for (i = 0; mc_wm_connection.state == current_status && i <= MC_WM_TIMEOUT / MC_WM_TIME_INTERVAL; i++)
		mc_usleep(MC_WM_TIME_INTERVAL * 1000);
	return mc_wm_connection.state;
}

static int
mc_wm_wait_for_connection(struct wlan_network *network)
{
	int n;
	int err;
	struct wlan_ip_config ipconfig;

	mc_log_debug("connecting %s...\n", network->ssid);

	if (mc_wm_connection.state == MC_CONNECTION_CONNECTED) {
		wlan_remove_network(mc_wm_connection.network_name);
		mc_wm_wait_status();
	}
	mc_wm_connection.state = MC_CONNECTION_DISCONNECTED;
	if ((err = wlan_add_network(network)) != WLAN_ERROR_NONE) {
		mc_log_error("wlan_add_network failed: %d\n", err);
		return -1;
	}
	/* will disconnect automatically if the network has already been connected */
	if (is_uap_mode(network))
		err = wlan_start_network(network->name);
	else
		err = wlan_connect(network->name);
	if (err != WLAN_ERROR_NONE) {
		mc_log_error("wlan_connect failed: %d\n", err);
		wlan_remove_network(network->name);
		return -1;
	}
	for (n = MC_WM_CONNECTION_RETRY; --n >= 0;) {
		mc_wm_wait_status();
		if (mc_wm_connection.state == MC_CONNECTION_ERROR) {
			mc_log_error("connection failed\n");
			wlan_remove_network(network->name);
			return -1;
		}
		else if (mc_wm_connection.state == MC_CONNECTION_CONNECTED)
			break;
	}
	strncpy(mc_wm_connection.network_name, network->name, sizeof(mc_wm_connection.network_name));

	if (is_uap_mode(network)) {
		err = wlan_get_uap_address(&ipconfig);
		mc_wm_connection.mode = MC_CONNECTION_MODE_UAP;
	}
	else {
		err = wlan_get_address(&ipconfig);
		mc_wm_connection.mode = MC_CONNECTION_MODE_STA;
	}
	if (err == WLAN_ERROR_NONE) {
#if WMSDK_VERSION <= 2013082
		mc_wm_connection.ipaddr.s_addr = ipconfig.ip;
#else
		mc_wm_connection.ipaddr.s_addr = ipconfig.ipv4.address;
#endif
	}
	else {
		mc_wm_connection.ipaddr.s_addr = 0;
		mc_log_error("connected: unknown address\n");
	}
	return 0;
}

static int
mc_wm_scan_cb(unsigned int count)
{
	unsigned int i;
	struct wlan_scan_result *aps = NULL;
	int num_aps = 0;

	mc_log_notice("scan: %d networks found\n", count);
	if (count != 0) {
		if ((aps = mc_malloc(count * sizeof(struct wlan_scan_result))) != NULL) {
			for (i = 0; i < count; i++) {
				if (wlan_get_scan_result(i, &aps[num_aps]) == WLAN_ERROR_NONE) {
					struct wlan_scan_result *res = &aps[num_aps];
					mc_log_notice(" [%d] \"%s\" %s\n", num_aps, *res->ssid ? res->ssid : "(hidden)", res->wpa2 ? "wpa2" : res->wpa ? "wpa" : res->wep ? "wep" : "none");
					num_aps++;
				}
			}
		}
	}
	mc_wm_connection.aps = aps;
	mc_wm_connection.num_aps = num_aps;
	mc_wm_connection.state = MC_CONNECTION_SCANNED;
	return 0;
}

static int
mc_wm_connect_preferred_network(struct wlan_network *network)
{
	int i;
	char *p, *pass;
	int hidden;
	int security;
	char name[8];
	char *value;
	int n;
	int connected = 0;
	mc_env_t env;

	if (mc_wm_connection.aps == NULL) {
		/* scan access points */
		int save_status = mc_wm_connection.state;
		int err;
		if ((err = wlan_scan(mc_wm_scan_cb)) != WLAN_ERROR_NONE) {
			mc_log_error("wlan: scan request failed: %d\n", err);
			return -1;
		}
		mc_wm_wait_status();
		if (mc_wm_connection.state == MC_CONNECTION_SCANNED)
			mc_wm_connection.state = save_status;
		else {
			mc_log_error("wlan: scan failed\n");
			return -1;
		}
	}
	mc_env_new(&env, MC_WM_ENV_PATH, 1);
	for (i = 0; i < MC_WM_MAX_SSIDS && !connected; i++) {
		sprintf(name, "SSID%d", i);
		if ((value = (char *)mc_env_get(&env, name)) == NULL)
			break;
		if (*value == '\0')
			continue;
		if ((value = mc_strdup(value)) == NULL)
			break;
		memset(network->ssid, '\0', sizeof(network->ssid));	/* f!@*&#$^(*#^(*#$)$#(ck!!! */
		if ((p = strtok(value, " ")) != NULL) {
			n = mc_decode64((unsigned char *)network->ssid, p, IEEEtypes_SSID_SIZE);
			network->ssid[n] = '\0';
		}
		hidden = (p = strtok(NULL, " ")) != NULL ? atoi(p) : 0;
		security = (p = strtok(NULL, " ")) != NULL ? atoi(p) : MC_CONFIG_SECURITY_NONE;
		pass = strtok(NULL, " ");
		switch (security) {
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
		mc_log_debug("matching %s, %d...\n", network->ssid, network->security.type);
		if (mc_wlan_match(network, hidden)) {
			strncpy(network->name, network->ssid, sizeof(network->name));
			if (pass != NULL) {
				network->security.psk_len = mc_decode64((uint8_t *)network->security.psk, pass, sizeof(network->security.psk));
			}
			else
				network->security.psk_len = 0;
			mc_log_debug("matched: %s\n", network->ssid);
			connected = mc_wm_wait_for_connection(network) == 0;
		}
		mc_free(value);
	}
	mc_env_free(&env);
	return connected ? 0 : -1;
}

static int
mc_wm_event_handler(enum wlan_event_reason reason, void *data)
{
	switch (reason) {
	case WLAN_REASON_INITIALIZED:
		mc_log_debug("wlan: initialized\n");
		mc_wm_connection.state = MC_CONNECTION_INITIALIZED;
		break;
	case WLAN_REASON_INITIALIZATION_FAILED:
		mc_log_debug("wlan: initialize failed\n");
		break;
	case WLAN_REASON_SUCCESS:
		mc_log_debug("wlan: success\n");
		mc_wm_connection.state = MC_CONNECTION_CONNECTED;
		break;
	case WLAN_REASON_NETWORK_NOT_FOUND:
		mc_log_debug("wlan: network not found\n");
		mc_wm_connection.state = MC_CONNECTION_ERROR;
		break;
	case WLAN_REASON_NETWORK_AUTH_FAILED:
		mc_log_debug("wlan: network auth failed\n");
		mc_wm_connection.state = MC_CONNECTION_ERROR;
		break;
	case WLAN_REASON_ADDRESS_SUCCESS:
		mc_log_debug("wlan: address success\n");
		mc_wm_connection.state = MC_CONNECTION_CONNECTED;
		break;
	case WLAN_REASON_ADDRESS_FAILED:
		mc_log_debug("wlan: address failed\n");
		// mc_wm_connection.state = MC_CONNECTION_ERROR;
		break;
	case WLAN_REASON_USER_DISCONNECT:
		mc_log_debug("wlan: disconnected\n");
		mc_wm_connection.state = MC_CONNECTION_DISCONNECTED;
		break;
	case WLAN_REASON_LINK_LOST:
		mc_log_debug("wlan: link lost\n");
		if (mc_wm_connection.state == MC_CONNECTION_CONNECTED) {
			mc_log_debug("wlan: trying to reconnect\n");
			wlan_connect(mc_wm_connection.network_name);
		}
		break;
	case WLAN_REASON_UAP_SUCCESS:
		mc_log_debug("wlan: UAP success\n");
		mc_wm_connection.state = MC_CONNECTION_CONNECTED;
		break;
	case WLAN_REASON_UAP_STOPPED:
		mc_log_debug("wlan: UAP stopped\n");
		mc_wm_connection.state = MC_CONNECTION_DISCONNECTED;
		break;
#if WMSDK_VERSION >= 2014037
	case WLAN_REASON_UAP_CLIENT_ASSOC:
	case WLAN_REASON_UAP_CLIENT_DISSOC:
		/* nothing to do */
		break;
	case WLAN_REASON_UAP_START_FAILED:
		mc_log_debug("wlan: UAP start failed\n");
		break;
	case WLAN_REASON_UAP_STOP_FAILED:
		mc_log_debug("wlan: UAP stop failed\n");
		break;
#endif
	default:
		mc_log_error("wlan: unknown event: %d\n", reason);
		break;
	}
	return 0;
}

static int
mc_wm_init()
{
	int err;
	uint8_t mcast_mac[MLAN_MAC_ADDR_LENGTH];
	char hostname[24];
	uint8_t *mac;

	/* wm_wlan_init is supposed to initialize all necessary drivers */
	if ((err = wm_wlan_init()) != WM_SUCCESS) {
		if (err == -WIFI_ERROR_FW_NOT_DETECTED) {
			mc_log_debug("wlan: wifi fw not detected! trying to use passive partition\n");
			mc_set_active_volume("wififw");
			err = wm_wlan_init();
		}
		if (err != WM_SUCCESS) {
			mc_log_error("wlan init failed: %d\n", err);
			return -1;
		}
	}
	mc_wm_connection.state = MC_CONNECTION_UNINITIALIZED;
	if ((err = wlan_start(mc_wm_event_handler)) != WLAN_ERROR_NONE) {
		mc_log_error("wlan_start failed: %d\n", err);
		return -1;
	}
	mc_wm_wait_status();
	if (mc_wm_connection.state != MC_CONNECTION_INITIALIZED) {
		mc_log_error("wlan initialization failed: status = %d\n", mc_wm_connection.state);
		return -1;
	}

	/* initialized. add the SSDP address to the MCAST filter */
	wifi_get_ipv4_multicast_mac(ntohl(inet_addr("239.255.255.250")), mcast_mac);
	wifi_add_mcast_filter(mcast_mac);

	/* set up the dhcp hostname with the mac address */
	wlan_get_mac_address(mc_wm_connection.mac);
	mac = mc_wm_connection.mac;
	if (mc_gethostname(hostname, sizeof(hostname)) != 0 || strncmp(hostname, "k5-", 3) == 0) {
		snprintf(hostname, sizeof(hostname), "k5-%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		mc_sethostname(hostname, sizeof(hostname));
	}
	net_dhcp_hostname_set(hostname);

	mc_wm_connection.config.state = MC_CONFIG_STATE_NONE;
	return 0;
}

static void
mc_wm_fin()
{
	wlan_stop();
}

static int
mc_wm_connect()
{
	struct mc_connection_config *conf = &mc_wm_connection.config;
	struct wlan_network *network;
	char hostname[HOST_NAME_MAX];
	int status = 0;

	if ((network = mc_calloc(1, sizeof(struct wlan_network))) == NULL) {
		errno = ENOMEM;
		return -1;
	}
	memset(network->bssid, 0, 6);
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
		strcpy(network->ssid, hostname);	/* use the hostname as the SSID */
		strcpy(network->name, network->ssid);
#if 0
		network->security.type = WLAN_SECURITY_WPA2;
		strcpy(network->security.psk, "marvellwm");
		network->security.psk_len = strlen(network->security.psk);
#else
		network->security.type = WLAN_SECURITY_NONE;
#endif
		status = mc_wm_wait_for_connection(network);
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
		if (conf->state == MC_CONFIG_STATE_STA) {
			/* connect a configured network */
			strncpy(network->ssid, conf->ssid, sizeof(network->ssid));
			strncpy(network->name, network->ssid, sizeof(network->name));
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
			status = mc_wm_wait_for_connection(network);
		}
		else {
			/* connect a preferred network */
			status = mc_wm_connect_preferred_network(network);
		}
		break;
	default:
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
		return -1;
	if (mc_wm_connection.mode & MC_CONNECTION_MODE_UAP)
		err = wlan_stop_network(mc_wm_connection.network_name);
	else if (mc_wm_connection.mode & MC_CONNECTION_MODE_STA)
		err = wlan_disconnect();
	else
		return -1;
	if (err != WLAN_ERROR_NONE)
		mc_log_error("wm_wlan_disconnect: err = %d\n", err);	/* ignore error! */
	mc_wm_wait_status();
	if (mc_wm_connection.state != MC_CONNECTION_DISCONNECTED)
		mc_log_error("wlan: disconnection failed: %d\n", mc_wm_connection.state);
	wlan_remove_network(mc_wm_connection.network_name);
	mc_wm_wait_status();
	return 0;
}

static int
mc_wm_save()
{
	int namelen, passlen, enclen, len;
	char *value, *p;
	char envname[sizeof("SSIDxx")];
	int i;
	struct mc_connection_config *conf = &mc_wm_connection.config;
	mc_env_t env;

	if (conf->state != MC_CONFIG_STATE_STA || !(conf->flags & MC_CONFIG_FLAG_SAVE))
		return 0;
	/* estimate the environment value length: SSID + hidden + sec_type + password */
	namelen = strlen(conf->ssid);
	passlen = conf->password ? strlen(conf->password) : 0;
	len = howmany(namelen, 3) * 4 + 1 + 1 + 1 + 1 + 1 + howmany(passlen, 3) * 4;
	if ((value = mc_malloc(len + 1)) == NULL) {
		errno = ENOMEM;
		return -1;
	}
	/* encode64 ssid */
	p = value;
	enclen = mc_encode64(p, (unsigned char *)conf->ssid, namelen);
	p += enclen;
	*p++ = ' ';
	sprintf(p, "%d %d", (conf->flags & MC_CONFIG_FLAG_HIDDEN) != 0, conf->security);
	p += strlen(p);
	if (conf->password != NULL) {
		int n;
		*p++ = ' ';
		n = mc_encode64(p, (const uint8_t *)conf->password, passlen);
		p += n;
	}
	*p = '\0';

	mc_env_new(&env, MC_WM_ENV_PATH, 1);	/* encrypt = true */
	/* find an empty slot */
	for (i = 0; i < MC_WM_MAX_SSIDS; i++) {
		const char *v;
		int match;
		sprintf(envname, "SSID%d", i);
		if ((v = mc_env_get(&env, envname)) == NULL)
			break;
		match = strncmp(v, value, enclen) == 0 && v[enclen] == ' ' && value[enclen] == ' ';
		if (match)
			break;
	}
	if (i >= MC_WM_MAX_SSIDS) {
		/* rotate */
		char num[3];
		const char *v;
		if ((v = mc_env_get(&env, "lastSSID")) == NULL)
			i = 0;
		i = atoi(v);
		i = (i + 1) % MC_WM_MAX_SSIDS;
		sprintf(num, "%d", i);
		mc_env_set(&env, "lastSSID", num);
		sprintf(envname, "SSID%d", i);
	}
	mc_env_set(&env, envname, value);
	mc_free(value);
	mc_env_store(&env);
	mc_env_free(&env);
	return 0;
}

static int
mc_wm_scan()
{
	int err, save_status;

	if (mc_wm_connection.aps != NULL) {
		mc_free(mc_wm_connection.aps);
		mc_wm_connection.aps = NULL;
		mc_wm_connection.num_aps = 0;
	}
	if ((err = wlan_scan(mc_wm_scan_cb)) != WLAN_ERROR_NONE) {
		mc_log_error("wlan_scan failed: %d\n", err);
		return -1;
	}
	save_status = mc_wm_connection.state;	/* should be CONNECTED */
	mc_wm_wait_status();
	err = mc_wm_connection.state != MC_CONNECTION_SCANNED;
	if (err)
		mc_log_error("wlan: scan failed: status = \n", mc_wm_connection.state);
	mc_wm_connection.state = save_status;
	return err ? -1 : 0;
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
	mc_wm_connection.state = MC_CONNECTION_INITIALIZED;
	mc_wm_connection.aps = NULL;
	return 0;
}

static void
mc_wm_fin()
{
	mc_wm_connection.state = MC_CONNECTION_UNINITIALIZED;
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
mc_wm_save()
{
	return 0;
}

static int
mc_wm_scan()
{
	mc_wm_connection.aps = NULL;
	return 0;
}

static struct mc_connection_config *
mc_wm_ap_config(int i)
{
	return NULL;
}

static int
mc_wm_getRSSI()
{
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
xs_wm_init(xsMachine *the)
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
	xsSetInteger(xsVar(0), MC_CONNECTION_MODE_WAC);
	xsNewHostProperty(xsThis, xsID("WAC"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	/* status */
	xsSetInteger(xsVar(0), MC_CONNECTION_CONNECTED);
	xsNewHostProperty(xsThis, xsID("CONNECTED"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_DISCONNECTED);
	xsNewHostProperty(xsThis, xsID("DISCONNECTED"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_ERROR);
	xsNewHostProperty(xsThis, xsID("ERROR"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
	xsSetInteger(xsVar(0), MC_CONNECTION_INITIALIZED);
	xsNewHostProperty(xsThis, xsID("INITIALIZED"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);

	if (mc_wm_init() != 0)
		mc_xs_throw(the, "wifi: init failed");
}

void
xs_wm_fin(xsMachine *the)
{
	mc_wm_fin();
}

static void
mc_setup_config(xsMachine *the, mc_connection_config_t *conf)
{
	xsIndex id_ssid = xsID("ssid");
	xsIndex id_security = xsID("security");
	xsIndex id_password = xsID("password");
	xsIndex id_hidden = xsID("hidden");
	xsIndex id_save = xsID("save");
	xsIndex id_wac = xsID("wac");

	xsVars(1);
	/* SSID and PSK can contain any octet including '\0' but assume they are a string here. */
	if (!xsHas(xsArg(0), id_ssid))
		goto bail;
	xsGet(xsVar(0), xsArg(0), id_ssid);
	if ((conf->ssid = mc_strdup(xsToString(xsVar(0)))) == NULL)
		goto bail;
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
	if (xsHas(xsArg(0), id_wac)) {
		xsGet(xsVar(0), xsArg(0), id_wac);
		if (xsTest(xsVar(0)))
			conf->flags |= MC_CONFIG_FLAG_WAC;
	}
	return;
bail:
	mc_xs_throw(the, "Wifi.connect: bad template arg");
}

static int mc_wm_running = 0;

static void
mc_wm_call(void *data)
{
	int (*f)(void) = data;

	if ((*f)() != 0)
		mc_wm_connection.state = MC_CONNECTION_ERROR;
	mc_wm_running = 0;
}

static void
mc_wm_run(int (*f)(void))
{
	if (mc_wm_running)
		return;
	mc_wm_running++;
	mc_thread_create(mc_wm_call, f);
}

void
xs_wm_connect(xsMachine *the)
{
	switch (xsTypeOf(xsArg(0))) {
	case xsIntegerType:
	case xsNumberType:
		switch (xsToInteger(xsArg(0))) {
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
		break;
	case xsReferenceType:
		mc_setup_config(the, &mc_wm_connection.config);
		mc_wm_connection.config.state = MC_CONFIG_STATE_STA;
		break;
	default:
		mc_xs_throw(the, "Wifi.connect: bad args");
		break;
	}
	if (mc_wm_connection.state == MC_CONNECTION_CONNECTED)
		mc_event_shutdown();	/* shutdown all running servers so the event loop will terminate */
	else
		mc_wm_run(mc_wm_connect);
}

void
xs_wm_disconnect(xsMachine *the)
{
	if (mc_wm_connection.state == MC_CONNECTION_CONNECTED)
		mc_wm_run(mc_wm_disconnect);
}

void
xs_wm_getIP(xsMachine *the)
{
	xsSetString(xsResult, inet_ntoa(mc_wm_connection.ipaddr));
}

void
xs_wm_getMAC(xsMachine *the)
{
	uint8_t *mac = mc_wm_connection.mac;
	char str[2*6+1];

	snprintf(str, sizeof(str), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	xsSetString(xsResult, str);
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
	xsSetInteger(xsResult, mc_wm_getRSSI());
}

void
xs_wm_scan(xsMachine *the)
{
	int ac = xsToInteger(xsArgc);
	int i;
	struct mc_connection_config *conf;
	char *ss;

	if (ac > 0 && xsTest(xsArg(0))) {
		/* re-scan the access points */
		if (ac > 1 && xsTest(xsArg(1))) {
			if (mc_wm_scan())
				mc_xs_throw(the, "wifi: scan failed");
		}
		else
			mc_wm_run(mc_wm_scan);
	}
	xsVars(2);
	xsSetNewInstanceOf(xsResult, xsArrayPrototype);
	for (i = 0; (conf = mc_wm_ap_config(i)) != NULL; i++) {
		xsSetNewInstanceOf(xsVar(0), xsObjectPrototype);
		xsSetString(xsVar(1), conf->ssid ? conf->ssid : "");
		xsSet(xsVar(0), xsID("ssid"), xsVar(1));
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
xs_wm_getInterfaces(xsMachine *the)
{
#if mxMC
	xsVars(2);
	xsSetNewInstanceOf(xsVar(0), xsObjectPrototype);
	xsSetTrue(xsVar(1));
	xsSet(xsVar(0), xsID("UP"), xsVar(1));
	xsSet(xsVar(0), xsID("MULTICAST"), xsVar(1));
	xs_wm_getIP(the);
	xsSet(xsVar(0), xsID("addr"), xsResult);
	xs_wm_getMAC(the);
	xsSet(xsVar(0), xsID("mac"), xsResult);
	xsSetNewInstanceOf(xsResult, xsArrayPrototype);
	xsSet(xsResult, xsID("wm0"), xsVar(0));
#elif mxMacOSX
	struct ifaddrs *iflist, *ifa;
	xsIndex id_name;

	if (getifaddrs(&iflist) != 0)
		return;
	xsVars(2);
	xsSetNewInstanceOf(xsResult, xsArrayPrototype);
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
			snprintf(str, sizeof(str), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			xsSetString(xsVar(1), str);
			xsSet(xsVar(0), xsID("mac"), xsVar(1));
		}
		else if (ifa->ifa_addr->sa_family == AF_INET) {	/* interface (IP) address */
			struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
			xsSetString(xsVar(1), inet_ntoa(sin->sin_addr));
			xsSet(xsVar(0), xsID("addr"), xsVar(1));
		}
		xsSetBoolean(xsVar(1), (ifa->ifa_flags & IFF_UP) != 0);
		xsSet(xsVar(0), xsID("UP"), xsVar(1));
		xsSetBoolean(xsVar(1), (ifa->ifa_flags & IFF_MULTICAST) != 0);
		xsSet(xsVar(0), xsID("MULTICAST"), xsVar(1));
	}
	freeifaddrs(iflist);
#else
	int s;
	struct ifconf ifc;
	struct ifreq *ifr;
	struct sockaddr_in *sin;
#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif
#define NEXT_IFR(ifr)	((caddr_t)ifr + MAX(ifr->ifr_addr.sa_len + sizeof(ifr->ifr_name), sizeof(*ifr)))

	xsVars(2);
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0)
		return;
	ifc.ifc_len = 0;	/* to get the actual size */
	ifc.ifc_req = NULL;
	if (ioctl(s, SIOCGIFCONF, &ifc) != 0 || (ifc.ifc_buf = mc_malloc(ifc.ifc_len)) == NULL)
		close(s);
		return;
	}
	ioctl(s, SIOGIFCONF, &ifc);
	xsSetNewInstanceOf(xsResult, xsArrayPrototype);
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
		xsSet(xsVar(0), xsID("UP"), xsBoolean((ifr->ifr_flags & IFF_UP) != 0));
		xsSet(xsVar(0), xsID("MULTICAST"), xsBoolean((ifr->ifr_flags & IFF_MULTICAST) != 0));
		if (ioctl(s, SIOCGIFADDR, ifr) != 0)
			continue;
		sin = (struct sockaddr_in *)&ifr->ifr_addr;
		xsSet(xsVar(0), xsID("addr"), xsString(inet_ntoa(sin->sin_addr)));
	}
	mc_free(ifc.ifc_buf);
	close(s);
#endif
}

void
xs_wm_save(xsMachine *the)
{
	mc_wm_save();
}

void
xs_wm_get_status(xsMachine *the)
{
	xsSetInteger(xsResult, mc_wm_running ? -1 : mc_wm_connection.state);
}

void
xs_wm_get_mode(xsMachine *the)
{
	xsSetInteger(xsResult, mc_wm_connection.mode);
}

void
xs_wm_get_config(xsMachine *the)
{
	struct mc_connection_config *conf = &mc_wm_connection.config;
	int state;

	xsVars(1);
	xsSetNewInstanceOf(xsResult, xsObjectPrototype);
	xsSetBoolean(xsVar(0), conf->flags & MC_CONFIG_FLAG_SAVE);
	xsSet(xsResult, xsID("save"), xsVar(0));
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
	xsSet(xsResult, xsID("state"), xsVar(0));
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
