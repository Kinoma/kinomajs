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
#include "mc_wifi.h"

#if !XS_ARCHIVE
#include "xm_wifi.xs.c"
MC_MOD_DECL(wifi);
#endif

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
	xsSetInteger(xsVar(0), MC_CONNECTION_FATAL);
	xsNewHostProperty(xsThis, xsID("FATAL"), xsVar(0), xsDontDelete | xsDontEnum | xsDontSet, xsDefault);
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
	(void)mc_wm_get_ipconfig();
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

	mc_wm_get_mac_addr(mc_wm_connection.mac);
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
	mc_wm_get_ipconfig();
	if (mc_wm_connection.dns.s_addr != IPADDR_ANY) {
		xsSetString(xsVar(1), inet_ntoa(mc_wm_connection.dns));
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
	mc_wm_set_mac_addr(mc_wm_connection.mac);
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


/*
 * callbacks from mc_wmsdk lib
 */

void
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

void
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
