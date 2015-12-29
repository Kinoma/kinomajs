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

struct elf_eheader {
	uint8_t ident[16];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint32_t entry;
	uint32_t phoffset;
	uint32_t shoffset;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
};

struct elf_sheader {
	uint32_t name;
	uint32_t type;
	uint32_t flags;
	uint32_t addr;
	uint32_t offset;
	uint32_t size;
	uint32_t link;
	uint32_t info;
	uint32_t align;
	uint32_t entsize;
};

struct elf_rel {
	uint32_t offset;
	uint32_t info;
};

struct elf_sym {
	uint32_t name;
	uint32_t value;
	uint32_t size;
	uint8_t info;
	uint8_t other;
	uint16_t shndx;
};

struct mc_elf {
	MC_FILE *f;
	struct elf_eheader eh;
	struct elf_sheader *sh;
	uint8_t *strtab;
	uint32_t strtabsz;
	struct elf_sheader *current;
	uint32_t pos;
};

#define ELF32_R_SYM(i)	((i)>>8)
#define ELF32_R_TYPE(i)	((unsigned char)(i))

extern int mc_elf_open(const char *fname, struct mc_elf *elf);
extern void mc_elf_close(struct mc_elf *elf);
extern int mc_elf_get_section_addr_and_size(struct mc_elf *elf, const char *name, uint8_t **addr, uint32_t *size);
extern int mc_elf_set_position(struct mc_elf *elf, const char *name);
extern int mc_elf_read(struct mc_elf *elf, void *buf, int nbytes);
extern int mc_elf_get(struct mc_elf *elf, const char *name, void *buf, int size, int ith);
extern int mc_elf_load_section(struct mc_elf *elf, const char *name, uint8_t *base);
extern uint8_t *mc_elf_get_entry(struct mc_elf *elf);
