#include "mc_connection.h"

extern mc_connection_t mc_wm_connection;

extern int mc_wm_init();
extern void mc_wm_fin();
extern int mc_wm_start();
extern void mc_wm_set_dhcp_name(const char *name);
extern int mc_wm_connect();
extern int mc_wm_disconnect();
extern int mc_wm_scan(int rescan);
extern struct mc_connection_config *mc_wm_ap_config(int i);
extern int mc_wm_get_ipconfig();
extern int mc_wm_getRSSI();
extern int mc_wm_get_bssid(unsigned char *bssid);
extern void mc_wm_set_mac_addr(const uint8_t *mac);
extern void mc_wm_get_mac_addr(uint8_t *mac);

extern void mc_wifi_add_mcast_filter(struct in_addr *addr);

extern int mc_wlan_set_mgmt_ie_221(uint8_t *ie, size_t sz);
extern void mc_wlan_clear_mgmt_ie(int index);
