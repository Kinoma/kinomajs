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
#ifndef __MC_CONNECTION_H__
#define __MC_CONNECTION_H__

#include "mc_stdio.h"
#if mxMC
#include "lwip/netdb.h"
#include "lwip/inet.h"
#else
#include "mc_compat.h"
#endif

typedef struct mc_connection_config {
	char *ssid;
	char *password;
	uint8_t bssid[6];
	enum {MC_CONFIG_STATE_NONE, MC_CONFIG_STATE_PREFERRED_NETWORK, MC_CONFIG_STATE_UAP, MC_CONFIG_STATE_STA} state;
	enum {MC_CONFIG_SECURITY_NONE, MC_CONFIG_SECURITY_WPA, MC_CONFIG_SECURITY_WPA2} security;
	uint32_t flags;
#define MC_CONFIG_FLAG_HIDDEN	0x1
#define MC_CONFIG_FLAG_SAVE	0x2
	uint32_t mode;	/* application specific mode */
} mc_connection_config_t;

/* connection request mode */
enum {
	MC_CONNECTION_CONFIG_NORMAL = 0,
	MC_CONNECTION_CONFIG_STA,
	MC_CONNECTION_CONFIG_FALLBACK,
};

typedef enum {
	MC_CONNECTION_NOTRUNNING = 0,
	MC_CONNECTION_UNINITIALIZED,
	MC_CONNECTION_INITIALIZED,
	MC_CONNECTION_SCANNED,
	MC_CONNECTION_DISCONNECTED,
	MC_CONNECTION_CONNECTED,
	MC_CONNECTION_ERROR,
	MC_CONNECTION_FATAL,
	MC_CONNECTION_PENDING,
} mc_connection_state_t;

enum {
	MC_CONNECTION_MODE_NONE = 0x00,
	MC_CONNECTION_MODE_STA = 0x01,
	MC_CONNECTION_MODE_UAP = 0x02,
	MC_CONNECTION_MODE_BITS = 8,
#define MC_CONNECTION_MODE_MASK	((1UL << MC_CONNECTION_MODE_BITS) - 1)
	/* > 0xff application specific mode */
};

#define MC_CONNECTION_MAC_SIZE	6
#define MC_CONNECTION_SSID_SIZE	32

typedef struct mc_connection {
	int (*init)();
	void (*fin)();
	int (*connect)(int);
	int (*disconnect)();
	int (*save)();
	mc_connection_state_t state;
	uint32_t mode;
	mc_connection_config_t config;
	uint8_t mac[MC_CONNECTION_MAC_SIZE];
	char network_name[MC_CONNECTION_SSID_SIZE];	/* copy of the current "network" name */
	/* only for Wifi */
	struct in_addr ipaddr;
	struct in_addr dns;
#if LWIP_IPV6
	struct in6_addr ipaddr6;
#endif
	void *aps;	/* access points */
	int num_aps;
} mc_connection_t;

#endif /* __MC_CONNECTION_H__ */
