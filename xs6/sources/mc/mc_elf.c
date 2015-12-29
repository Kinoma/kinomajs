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
#include "mc_file.h"
#include "mc_elf.h"

/* section header type */
enum {
	SHT_NULL = 0,
	SHT_PROGBITS,
	SHT_SYMTAB,
	SHT_STRTAB,
	SHT_RELA,
	SHT_HASH,
	SHT_DYNAMIC,
	SHT_NOTE,
	SHT_NOBITS,
	SHT_REL,
	SHT_SHLIB,
	SHT_DYNSYM,
	SHT_LOPROC = 0x70000000,
	SHT_HIPROC = 0x7fffffff,
	SHT_LOUSER = 0x80000000,
	SHT_HIUSER = 0xffffffff,
};

int
mc_elf_open(const char *fname, struct mc_elf *elf)
{
	long offset;

	memset(elf, 0, sizeof(elf));
	if ((elf->f = mc_fopen(fname, "r")) == NULL)
		return -1;
	if (mc_fread(&elf->eh, sizeof(elf->eh), 1, elf->f) != 1)
		goto bail;
	if (memcmp(elf->eh.ident, "\177ELF", 4) != 0)
		goto bail;
	if (elf->eh.shentsize != sizeof(struct elf_sheader))
		goto bail;

	offset = elf->eh.shoffset;
	mc_fseek(elf->f, offset, SEEK_SET);
	if ((elf->sh = mc_malloc(elf->eh.shentsize * elf->eh.shnum)) == NULL)
		goto bail;
	if (mc_fread(elf->sh, elf->eh.shentsize * elf->eh.shnum, 1, elf->f) != 1)
		goto bail;

	/* load the string table */
	if (elf->eh.shstrndx >= elf->eh.shnum)
		goto bail;
	elf->strtabsz = elf->sh[elf->eh.shstrndx].size;
	if ((elf->strtab = mc_malloc(elf->strtabsz)) == NULL)
		goto bail;
	offset = elf->sh[elf->eh.shstrndx].offset;
	if (mc_fseek(elf->f, offset, SEEK_SET) == -1 ||
	    mc_fread(elf->strtab, elf->strtabsz, 1, elf->f) != 1)
		goto bail;
	return 0;

bail:
	mc_elf_close(elf);
	return -1;
}

void
mc_elf_close(struct mc_elf *elf)
{
	if (elf->f != NULL) {
		mc_fclose(elf->f);
		elf->f = NULL;
	}
	if (elf->sh != NULL) {
		mc_free(elf->sh);
		elf->sh = NULL;
	}
	if (elf->strtab != NULL) {
		mc_free(elf->strtab);
		elf->strtab = NULL;
	}
}

int
mc_elf_get_section_addr_and_size(struct mc_elf *elf, const char *name, uint8_t **addr, uint32_t *size)
{
	int i;

	for (i = 0; i < elf->eh.shnum; i++) {
		struct elf_sheader *sh = &elf->sh[i];
		if (sh->name < elf->strtabsz && strcmp(name, (char *)&elf->strtab[sh->name]) == 0) {
			*addr = (uint8_t *)sh->addr;
			*size = sh->size;
			return 0;
		}
	}
	return -1;
}

int
mc_elf_set_position(struct mc_elf *elf, const char *name)
{
	int i;
	long offset;
	int err = 0;
	
	for (i = 0; i < elf->eh.shnum; i++) {
		struct elf_sheader *sh = &elf->sh[i];
		if (sh->name < elf->strtabsz && strcmp(name, (char *)&elf->strtab[sh->name]) == 0) {
			offset = sh->offset;
			if (mc_fseek(elf->f, offset, SEEK_SET) == -1) {
				err = -1;
				goto bail;
			}
			elf->current = sh;
			elf->pos = 0;
			break;
		}
	}
bail:
	return err;
}

int
mc_elf_read(struct mc_elf *elf, void *buf, int nbytes)
{
	long offset;
	size_t nread;

	if (elf->pos + nbytes > elf->current->size)
		return -1;
	offset = elf->pos + elf->current->offset;
	if (mc_fseek(elf->f, offset, SEEK_SET) == 0 &&
	    (nread = mc_fread(buf, 1, (size_t)nbytes, elf->f)) != 0) {
		elf->pos += nread;
		return 0;
	}
	else
		return -1;
}

int
mc_elf_get(struct mc_elf *elf, const char *name, void *buf, int size, int ith)
{
	int i;
	long offset;
	
	for (i = 0; i < elf->eh.shnum; i++) {
		struct elf_sheader *sh = &elf->sh[i];
		if (sh->name < elf->strtabsz && strcmp(name, (char *)&elf->strtab[sh->name]) == 0) {
			offset = sh->offset + size * ith;
			if (mc_fseek(elf->f, offset, SEEK_SET) == 0 &&
			    mc_fread(buf, (size_t)size, 1, buf) == 1)
				return 0;
			else
				return -1;
		}
	}
	return -1;
}

int
mc_elf_load_section(struct mc_elf *elf, const char *name, uint8_t *base)
{
	if (mc_elf_set_position(elf, name) == -1)
		return -1;
	if (strcmp(name, ".bss") == 0) {
		memset(base + elf->current->addr, 0, elf->current->size);
		return 0;
	}
	else
		return mc_elf_read(elf, base + elf->current->addr, elf->current->size);
}

uint8_t *
mc_elf_get_entry(struct mc_elf *elf)
{
	return (uint8_t *)elf->eh.entry;
}
