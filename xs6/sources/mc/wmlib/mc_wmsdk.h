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
#ifndef __MC_WMSDK_H__
#define __MC_WMSDK_H__

#define MC_PARTITION_NAME_SIZE	8

typedef struct mc_partition_entry {
	int pe_dev;
	enum {MC_PTYPE_BOOT2, MC_PTYPE_FW, MC_PTYPE_WLAN_FW, MC_PTYPE_FTFS, MC_PTYPE_PSM, MC_PTYPE_USER_APP, MC_PTYPE_UNKNOWN} pe_type;
	size_t pe_size;
	long pe_start;
	int pe_gen;
	const char *pe_name;
} mc_partition_entry_t;

extern void mc_partition_init();
extern int mc_get_partition_by_name(const char *volname, mc_partition_entry_t *pe, int active_flag);
extern int mc_set_active_partition(const char *volname);
extern int mc_flash_drv_init();
extern void *mc_flash_drv_open(int pe_dev);
extern void mc_flash_drv_close(void *mdev);
extern int mc_flash_drv_read(void *mdev, void *buf, size_t sz, long offset);
extern int mc_flash_drv_write(void *mdev, const void *buf, size_t sz, long offset);
extern int mc_flash_drv_erase(void *mdev, long offset, size_t sz);

extern int mc_dhcpd_start();
extern int mc_dhcpd_stop();

extern void mc_pm_init();
extern void mc_pm_reboot();
extern void mc_pm_shutoff();

extern void mc_wdt_init(int index, int shutdown);
extern void mc_wdt_enable();
extern void mc_wdt_disable();
extern void mc_wdt_close();
extern void mc_wdt_restart_counter();

extern void mc_rtc_time_set(long t);

extern int mc_soft_crc32_init();
extern uint32_t mc_soft_crc32(const void *data, int data_size, uint32_t crc);
extern void mc_soft_crc32_fin();

extern void mc_usb_hw_init();

#endif /* __MC_WMSDK_H__ */
