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
#include "mc_wmsdk.h"

#if mxMC
#include <wmstdio.h>
#include <wm_os.h>
#include <flash.h>
#include <partition.h>

/*
 * partition table
 */
void
mc_partition_init()
{
	part_init();
}

int
mc_get_partition_by_name(const char *volname, mc_partition_entry_t *pe, int active_flag)
{
	short index = 0;
	struct partition_entry *p1 = part_get_layout_by_name(volname, &index);
	struct partition_entry *p2 = part_get_layout_by_name(volname, &index);
	struct partition_entry *p;

	if (active_flag >= 0 && p1 != NULL && p2 != NULL) {
		struct partition_entry *active = part_get_active_partition(p1, p2);
		p = active_flag ? active : (active == p1 ? p2 : p1);
	}
	else
		p = p1 != NULL ? p1 : p2;
	if (p == NULL)
		return -1;
	pe->pe_dev = p->device;
	pe->pe_start = p->start;
	pe->pe_size = p->size;
	switch (p->type) {
	case FC_COMP_BOOT2: pe->pe_type = MC_PTYPE_BOOT2; break;
	case FC_COMP_FW: pe->pe_type = MC_PTYPE_FW; break;
	case FC_COMP_WLAN_FW: pe->pe_type = MC_PTYPE_WLAN_FW; break;
	case FC_COMP_FTFS: pe->pe_type = MC_PTYPE_FTFS; break;
	case FC_COMP_PSM: pe->pe_type = MC_PTYPE_PSM; break;
	case FC_COMP_USER_APP: pe->pe_type = MC_PTYPE_USER_APP; break;
	default: pe->pe_type = MC_PTYPE_UNKNOWN; break;
	}
	pe->pe_gen = p->gen_level;
	pe->pe_name = NULL;	/* not set */
	return 0;
}

int
mc_set_active_partition(const char *volname)
{
	struct partition_entry *pe1, *pe2, *active;
	short idx = 0;

	pe1 = part_get_layout_by_name(volname, &idx);
	pe2 = part_get_layout_by_name(volname, &idx);
	if (pe1 && pe2) {
		active = part_get_active_partition(pe1, pe2);
		part_set_active_partition(active == pe1 ? pe2 : pe1);
		return 0;
	}
	return -1;
}


/*
 * flash IO
 */
int
mc_flash_drv_init()
{
	static int inited = 0;

	if (!inited) {
		flash_drv_init();
		inited++;
	}
	return 0;
}

void *
mc_flash_drv_open(int pe_dev)
{
	return flash_drv_open(pe_dev);
}

void
mc_flash_drv_close(void *mdev)
{
	flash_drv_close(mdev);
}

int
mc_flash_drv_read(void *mdev, void *buf, size_t sz, long offset)
{
	return flash_drv_read(mdev, buf, sz, offset) == WM_SUCCESS ? 0 : -1;
}

int
mc_flash_drv_write(void *mdev, const void *buf, size_t sz, long offset)
{
	return flash_drv_write(mdev, buf, sz, offset) == WM_SUCCESS ? 0 : -1;
}

int
mc_flash_drv_erase(void *mdev, long offset, size_t sz)
{
	return flash_drv_erase(mdev, offset, sz) == WM_SUCCESS ? 0 : -1;
}


/*
 * dhcpd
 */
#include <wm_net.h>
#include <wmstats.h>
#include <dhcp-server.h>

/* used in WMSDK */
#undef errno
int errno;
struct wm_stats g_wm_stats;

int
mc_dhcpd_start()
{
	void *handler = net_get_uap_handle();

	return dhcp_server_start(handler) == 0 ? 0 : -1;
}

int
mc_dhcpd_stop()
{
	dhcp_server_stop();
	return 0;
}

/*
 * power management
 */
#include <mdev_pm.h>
#include <pwrmgr.h>
#include <lowlevel_drivers.h>

void
mc_pm_init()
{
	pm_init();
}

void
mc_pm_reboot()
{
	pm_reboot_soc();	/* force reboot */
}

void
mc_pm_shutoff()
{
	CLK_RC32M_SfllRefClk();	/* switch to the internal clock so we can now shut wifi off */
	PMU_PowerDownWLAN();
	pm_mcu_state(PM4, 0);	/* deep sleep */
}

/*
 * watchdog timer
 */
#include <mdev_wdt.h>

void
mc_wdt_init(int index, int shutdown)
{
	WDT_Config_Type cfg;

	cfg.timeoutVal = index;
	cfg.mode = shutdown ? WDT_MODE_INT : WDT_MODE_RESET;
	cfg.resetPulseLen = WDT_RESET_PULSE_LEN_2;
	WDT_Init(&cfg);
}

void
mc_wdt_enable()
{
	WDT_Enable();
}

void
mc_wdt_disable()
{
	CLK_ModuleClkDisable(CLK_WDT);
	WDT_Disable();
}

void
mc_wdt_close()
{
	WDT_IntClr();
	CLK_ModuleClkDisable(CLK_WDT);
	WDT_Disable();
}

void
mc_wdt_restart_counter()
{
	WDT_RestartCounter();
}

/*
 * RTC
 */
#include <rtc.h>

void
mc_rtc_time_set(long t)
{
	rtc_time_set(t);
}

/*
 * soft CRC
 */
#include <soft_crc.h>
int
mc_soft_crc32_init(void)
{
	return 0;
}

uint32_t
mc_soft_crc32(const void *data, int data_size, uint32_t crc)
{
	return soft_crc32(data, data_size, crc);
}

void
mc_soft_crc32_fin()
{
}

/*
 * USB
 */
#include <usb_hw.h>

void
mc_usb_hw_init()
{
	USB_HwInit();
}

#else	/* !mxMC */

#define N_PARTITIONS	12
static mc_partition_entry_t mc_partition_table[N_PARTITIONS] = {
	{.pe_type = MC_PTYPE_BOOT2, .pe_dev = 0, .pe_name = "boot2", .pe_start = 0x0, .pe_size = 0x6000, .pe_gen = 0},
	{.pe_type = MC_PTYPE_PSM, .pe_dev = 0, .pe_name = "psm", .pe_start = 0x6000, .pe_size = 0x4000, .pe_gen = 0},
	{.pe_type = MC_PTYPE_FW, .pe_dev = 0, .pe_name = "mcufw", .pe_start = 0xa000, .pe_size = 0x120000, .pe_gen = 0},
	{.pe_type = MC_PTYPE_FW, .pe_dev = 0, .pe_name = "mcufw", .pe_start = 0x12a000, .pe_size = 0x120000, .pe_gen = 0},
	{.pe_type = MC_PTYPE_FTFS, .pe_dev = 0, .pe_name = "ftfs", .pe_start = 0x24a000, .pe_size = 0x50000, .pe_gen = 0},
	{.pe_type = MC_PTYPE_FTFS, .pe_dev = 0, .pe_name = "ftfs", .pe_start = 0x29a000, .pe_size = 0x50000, .pe_gen = 0},
	{.pe_type = MC_PTYPE_WLAN_FW, .pe_dev = 0, .pe_name = "wififw", .pe_start = 0x2ea000, .pe_size = 0x40000, .pe_gen = 0},
	{.pe_type = MC_PTYPE_WLAN_FW, .pe_dev = 0, .pe_name = "wififw", .pe_start = 0x32a000, .pe_size = 0x40000, .pe_gen = 0},
	{.pe_type = MC_PTYPE_USER_APP, .pe_dev = 0, .pe_name = "k0", .pe_start = 0x36a000, .pe_size = 0x20000, .pe_gen = 0},
	{.pe_type = MC_PTYPE_USER_APP, .pe_dev = 0, .pe_name = "k1", .pe_start = 0x38a000, .pe_size = 0x20000, .pe_gen = 0},
	{.pe_type = MC_PTYPE_USER_APP, .pe_dev = 0, .pe_name = "k2", .pe_start = 0x3aa000, .pe_size = 0x20000, .pe_gen = 0},
	{.pe_type = MC_PTYPE_USER_APP, .pe_dev = 0, .pe_name = "k3", .pe_start = 0x3ca000, .pe_size = 0x20000, .pe_gen = 0},
};

#define FLASH_SIZE	(4*1024*1024)
static uint8_t mc_flash[FLASH_SIZE];
#define FLASH_RANGE_CHECK(p, sz)	(p >= mc_flash && p + sz <= mc_flash + FLASH_SIZE)

void
mc_partition_init()
{
}

int
mc_get_partition_by_name(const char *volname, mc_partition_entry_t *pe, int active_flag)
{
	int i;

	for (i = 0; i < N_PARTITIONS; i++) {
		if (strcmp(mc_partition_table[i].pe_name, volname) == 0) {
			*pe = mc_partition_table[i];
			return 0;
		}
	}
	return -1;
}

int
mc_set_active_partition(const char *volname)
{
	int i;

	for (i = 0; i < N_PARTITIONS; i++) {
		if (strcmp(mc_partition_table[i].pe_name, volname) == 0) {
			mc_partition_table[i].pe_gen++;
			return 0;
		}
	}
	return -1;
}

int
mc_flash_drv_init()
{
	return 0;
}

void *
mc_flash_drv_open(int pe_dev)
{
	return (void *)mc_flash;
}

void
mc_flash_drv_close(void *mdev)
{
	/* write it out? */
}

int
mc_flash_drv_read(void *mdev, void *buf, size_t sz, long offset)
{
	uint8_t *p = (uint8_t *)mdev + offset;

	if (!FLASH_RANGE_CHECK(p, sz))
		return -1;
	memcpy(buf, p, sz);
	return 0;
}

int
mc_flash_drv_write(void *mdev, const void *buf, size_t sz, long offset)
{
	uint8_t *p = (uint8_t *)mdev + offset;

	if (!FLASH_RANGE_CHECK(p, sz))
		return -1;
	memcpy(p, buf, sz);
	return 0;
}

int
mc_flash_drv_erase(void *mdev, long offset, size_t sz)
{
	uint8_t *p = (uint8_t *)mdev + offset;

	if (!FLASH_RANGE_CHECK(p, sz))
		return -1;
	memset(p, 0, sz);
	return 0;
}

int
mc_dhcpd_start()
{
	return 0;
}

int
mc_dhcpd_stop()
{
	return 0;
}

void
mc_pm_init()
{
}

void
mc_pm_reboot()
{
	exit(0);
}

void
mc_pm_shutoff()
{
	exit(0);
}

void
mc_wdt_init(int index, int shutdown)
{
}

void
mc_wdt_enable()
{
}

void
mc_wdt_disable()
{
}

void
mc_wdt_close()
{
}

void
mc_wdt_restart_counter()
{
}

void
mc_rtc_time_set(long t)
{
}

int
mc_soft_crc32_init(void)
{
	return 0;
}

uint32_t
mc_soft_crc32(const void *data, int data_size, uint32_t crc)
{
	return 0;	/* @@ should be implemented */
}

void mc_soft_crc32_fin()
{
}

void
mc_usb_hw_init()
{
}

#endif	/* !mxMC */
