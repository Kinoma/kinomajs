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
#include "mc_stdio.h"
#include "mc_module.h"
#include "mc_elf.h"
#include "mc_dl.h"

struct ext_host {
	void *base;
	struct mc_module *ext;
};

static const char *ext_sections[] = {
	".text",
	".rodata",
	".data",
	".bss",
	".got",
};
#define NUM_EXT_SECTIONS	(sizeof(ext_sections) / sizeof(ext_sections[0]))

extern void *const ext_stubs[];
extern int num_ext_stubs;

#if XIP && !XS_ARCHIVE
struct mc_module *mc_lookup_module(const char *path);
#endif

static int
load_elf(const char *path, void **basep, struct mc_module **modp)
{
	struct mc_elf elf;
	unsigned int i;
	uint32_t psize = 0, sz;
	uint8_t *base = NULL;
	uint8_t *paddr = NULL, *adr;
	struct mc_module *mod = NULL;
	int err = -1;

	mc_log_debug("ELF: loading %s...\n", path);
	if (mc_elf_open(path, &elf) != 0) {
		mc_log_error("ELF: cannot open %s\n", path);
		return -1;
	}
	/* calculate the entire program size in VM */
	for (i = 0; i < NUM_EXT_SECTIONS; i++) {
		if (mc_elf_get_section_addr_and_size(&elf, ext_sections[i], &adr, &sz) != 0)
			continue;
		if (adr >= paddr) {
			paddr = adr;
			psize = (uint32_t)paddr + sz;
		}
	}
	mc_log_debug("ELF: allocating %d bytes...\n", (unsigned int)psize);
	if ((base = mc_malloc(psize)) == NULL)
		goto bail;
	/* load each section */
	for (i = 0; i < NUM_EXT_SECTIONS; i++) {
		if (mc_elf_load_section(&elf, ext_sections[i], base) != 0)
			mc_log_error("cannot load %s (%d)\n", ext_sections[i], err);
	}
#if 0
	/* adjust function(?) addresses */
	if (mc_elf_get_section_addr_and_size(&elf, ".got", &adr, &sz) == 0) {
		uint32_t *gadr = (uint32_t *)(adr + (uint32_t)base), *eadr = (uint32_t *)(adr + (uint32_t)base + sz);
		for (; gadr < eadr; gadr++) {
/*
			if (*gadr & 0xf0000000) {
				*gadr = (*gadr & ~0xf0000000) + (uint32_t)base;
				mc_log_debug("adjusted: 0x%lx\n", *gadr);
			}
*/
			*gadr += (uint32_t)base;
		}
	}
#endif
	/* relocate variables */
	if (mc_elf_set_position(&elf, ".rel.dyn") == 0) {
		struct elf_rel rel;
		while (mc_elf_read(&elf, &rel, sizeof(rel)) == 0) {
			switch (ELF32_R_TYPE(rel.info)) {
			case 0x17:	/* R_ARM_RELATIVE */
				*(uint32_t *)(rel.offset + (uint32_t)base) += (uint32_t)base;
				// mc_log_debug("RELOC (rel): %x: %x\n", rel.offset, *(uint32_t *)(rel.offset + (uint32_t)base));
				break;
			case 0x02: {	/* R_ARM_ABS32 */
				struct elf_sym sym;
				if (mc_elf_get(&elf, ".dynsym", &sym, sizeof(sym), ELF32_R_SYM(rel.info)) != 0) {
					mc_log_error("cannot read .dynsym %ld\n", ELF32_R_SYM(rel.info));
					goto bail;
				}
				*(uint32_t *)(rel.offset + (uint32_t)base) = sym.value + (uint32_t)base;
				// mc_log_debug("RELOC (abs): %x: %x\n", rel.offset, *(uint32_t *)(rel.offset + (uint32_t)base));
				break;
			}
			default:
				/* no-op */
				mc_log_error("RELOC unknown type: 0x%x\n", ELF32_R_TYPE(rel.info));
				break;
			}
		}
	}
	paddr = mc_elf_get_entry(&elf);
	// mc_log_debug("ELF: entry = 0x%lx, base = 0x%lx\n", (uint32_t)paddr, (uint32_t)base);
	paddr += (uint32_t)base;
	mod = (struct mc_module *)paddr;
	*mod->stubs = (const void **)ext_stubs;
	*mod->num_stubs = num_ext_stubs;
	*basep = base;
	*modp = mod;
	err = 0;

bail:
	mc_elf_close(&elf);
	if (err != 0 && base != NULL)
		mc_free(base);
	return err;
}

void *
mc_dlopen(const char *path)
{
	struct ext_host *host;

	if ((host = mc_calloc(1, sizeof(struct ext_host))) == NULL) {
		errno = ENOMEM;
		return NULL;
	}
#if XIP && !XS_ARCHIVE
	/* first look up the linked list */
	if ((host->ext = mc_lookup_module(path)) != NULL)
		return host;
#endif
	if (load_elf(path, &host->base, &host->ext) != 0) {
		mc_free(host);
		return NULL;
	}
#if MC_NUM_XREF > 0
	if (host->ext->xrefs != NULL) {
		int i;
		struct mc_extension *ext = host->ext;
		for (i = 0; i < ext->num_xrefs; i++) {
			struct mc_xref *xref = &ext->xrefs[i];
			xref_stubs[xref->fnum] = xref->fp;
		}
	}
#endif
	return host;
}

void *
mc_dlsym(void *handle, const char *symbol)
{
	struct ext_host *host = handle;

	return host->ext->build;
}

int
mc_dlclose(void *handle)
{
	struct ext_host *host = handle;

	if (host->base != NULL)
		mc_free(host->base);
	mc_free(host);
	return 0;
}

int
mc_dlerror()
{
	return errno;
}


/*
 * dynamic loadable functions
 */

#include "mc_event.h"
#include "mc_file.h"
#include "mc_misc.h"
#include "mc_time.h"
#include "mc_env.h"
#include "mc_xs.h"
#include "mc_memory.h"
#include "crypt.h"

#include <board.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <wmstdio.h>
#include <wm_net.h>
#include <wm_wlan.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/inet.h>
#ifndef CONFIG_IPV6
void net_get_if_ipv6_pref_addr() {}
#endif

void *xref_stubs[MC_NUM_XREFS] = {0};

static const struct mc_global_record _mc_global = {
	(void *)&xref_stubs,
	(void *)&errno,
};

#include "ext_stubs.sym.h"
int num_ext_stubs = sizeof(ext_stubs) / sizeof(ext_stubs[0]);

#if XIP && !XS_ARCHIVE
/* extensions */
extern struct mc_module
	_mc_socket_module,
	_mc_env_module,
	_mc_files_module,
	_mc_system_module,
	_mc_timeinterval_module,
	_mc_wifi_module,
	_mc_telnetd_module,
	_mc_tftpd_module,
	_mc_dhcpd_module,
	_mc_stringview_module,
	_mc_debug_module;

static struct {
	const char *path;
	struct mc_module *mod;
} mc_modules[] = {
	{"socket.so", &_mc_socket_module},
	{"env.so", &_mc_env_module},
	{"files.so", &_mc_files_module},
	{"system.so", &_mc_system_module},
	{"timeinterval.so", &_mc_timeinterval_module},
	{"wifi.so", &_mc_wifi_module},
	{"telnetd.so", &_mc_telnetd_module},
	{"tftpd.so", &_mc_tftpd_module},
	{"dhcpd.so", &_mc_dhcpd_module},
	{"stringview.so", &_mc_stringview_module},
	{"debug.so", &_mc_debug_module},
};
static int mc_num_modules = sizeof(mc_modules) / sizeof(mc_modules[0]);

struct mc_module *
mc_lookup_module(const char *path)
{
	int i;

	for (i = 0; i < mc_num_modules; i++) {
		if (strcmp(mc_modules[i].path, path) == 0)
			return mc_modules[i].mod;
	}
	return NULL;
}

#endif /* XIP */
