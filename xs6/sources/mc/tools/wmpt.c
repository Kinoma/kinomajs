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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * copied from parition.h
 */

#define MAX_NAME    8

/** Partition Table */
struct partition_table {
#define PARTITION_TABLE_MAGIC (('W' << 0)|('M' << 8)|('P' << 16)|('T' << 24))
	/** The magic identifying the start of the partition table */
	uint32_t magic;
#define PARTITION_TABLE_VERSION 1
	/** The version number of this partition table */
	uint16_t version;
	/** The number of partition entries that follow this */
	uint16_t partition_entries_no;
	/** Generation level */
	uint32_t gen_level;
	/** The CRC of all the above components */
	uint32_t crc;
};

/** Partition Entry */
struct partition_entry {
	/** The type of the flash component */
	uint8_t type;
	/** The device id, internal flash is always id 0 */
	uint8_t device;
	/** A descriptive component name */
	char name[MAX_NAME];
	/** Start address on the given device */
	uint32_t start;
	/** Size on the given device */
	uint32_t size;
	/** Generation level */
	uint32_t gen_level;
};

/** The various components in a flash layout */
enum flash_comp {
	/** The secondary stage boot loader to assist firmware bootup */
	FC_COMP_BOOT2 = 0,
	/** The firmware image. There can be a maximum of two firmware
	 * components available in a flash layout. These will be used in an
	 * active-passive mode if rfget module is enabled.
	 */
	FC_COMP_FW,
	/** The wlan firmware image. There can be one wlan firmware image in the
	 * system. The contents of this location would be downloaded to the WLAN
	 * chip.
	 */
	FC_COMP_WLAN_FW,
	/** The FTFS image. */
	FC_COMP_FTFS,
	/** The PSM data */
	FC_COMP_PSM,
	/** Application Specific component */
	FC_COMP_USER_APP,
};

/*
 * copied from flash_layout.h
 */
/** Flash base address */
#define MW300_FLASH_BASE	0x0
/** Flash sector size */
#define MW300_FLASH_SECTOR_SIZE 0x1000	/*!< 4KB */

/** Section: Secondary stage boot-loader with bootrom header
 *  Start:   0x0
 *  Length:  0x4000(16KiB)
 *  Device:  Internal Flash
 */
#define FL_BOOT2_START (MW300_FLASH_BASE + 0x0)
#define FL_BOOTROM_H_SIZE (0x400)
/* Note: Maximum size considering future requirements */
#define FL_BOOT2_BLOCK_SIZE (MW300_FLASH_SECTOR_SIZE * 4)
#define FL_BOOT2_BLOCK_END (FL_BOOT2_START + FL_BOOT2_BLOCK_SIZE)
#define FL_BOOT2_DEV	FL_INT

/** Section: Partition table 1
 *  Start:   0x4000
 *  Length:  0x1000(4KiB)
 *  Device:  Internal Flash
 */
#define FL_PART_SIZE (MW300_FLASH_SECTOR_SIZE)
#define FL_PART1_START FL_BOOT2_BLOCK_END
#define FL_PART1_TABLE_END (FL_PART1_START + FL_PART_SIZE)
#define FL_PART_DEV	FL_INT

/** Section: Partition table 2
 *  Start:   0x5000
 *  Length:  0x1000(4KiB)
 *  Device:  Internal Flash
 */
#define FL_PART2_START FL_PART1_TABLE_END
#define FL_PART2_END (FL_PART2_START + FL_PART_SIZE)


/*
 * wififw header
 */
struct wlan_fw_header {
	uint32_t        magic;
	uint32_t        length;
};

#define WLAN_FW_MAGIC          (('W' << 0)|('L' << 8)|('F' << 16)|('W' << 24))


/*
 * copied from crc32.c
 */
#define TABLE_SIZE 256

/*
 * Polynomial used to generate the table:
 * CRC-32-IEEE 802.3, the polynomial is :
 * x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1
 */


static const uint32_t crc_table[TABLE_SIZE] = {
	0x00000000,	0x77073096,	0xee0e612c,	0x990951ba,
	0x076dc419,	0x706af48f,	0xe963a535,	0x9e6495a3,
	0x0edb8832,	0x79dcb8a4,	0xe0d5e91e,	0x97d2d988,
	0x09b64c2b,	0x7eb17cbd,	0xe7b82d07,	0x90bf1d91,
	0x1db71064,	0x6ab020f2,	0xf3b97148,	0x84be41de,
	0x1adad47d,	0x6ddde4eb,	0xf4d4b551,	0x83d385c7,
	0x136c9856,	0x646ba8c0,	0xfd62f97a,	0x8a65c9ec,
	0x14015c4f,	0x63066cd9,	0xfa0f3d63,	0x8d080df5,
	0x3b6e20c8,	0x4c69105e,	0xd56041e4,	0xa2677172,
	0x3c03e4d1,	0x4b04d447,	0xd20d85fd,	0xa50ab56b,
	0x35b5a8fa,	0x42b2986c,	0xdbbbc9d6,	0xacbcf940,
	0x32d86ce3,	0x45df5c75,	0xdcd60dcf,	0xabd13d59,
	0x26d930ac,	0x51de003a,	0xc8d75180,	0xbfd06116,
	0x21b4f4b5,	0x56b3c423,	0xcfba9599,	0xb8bda50f,
	0x2802b89e,	0x5f058808,	0xc60cd9b2,	0xb10be924,
	0x2f6f7c87,	0x58684c11,	0xc1611dab,	0xb6662d3d,
	0x76dc4190,	0x01db7106,	0x98d220bc,	0xefd5102a,
	0x71b18589,	0x06b6b51f,	0x9fbfe4a5,	0xe8b8d433,
	0x7807c9a2,	0x0f00f934,	0x9609a88e,	0xe10e9818,
	0x7f6a0dbb,	0x086d3d2d,	0x91646c97,	0xe6635c01,
	0x6b6b51f4,	0x1c6c6162,	0x856530d8,	0xf262004e,
	0x6c0695ed,	0x1b01a57b,	0x8208f4c1,	0xf50fc457,
	0x65b0d9c6,	0x12b7e950,	0x8bbeb8ea,	0xfcb9887c,
	0x62dd1ddf,	0x15da2d49,	0x8cd37cf3,	0xfbd44c65,
	0x4db26158,	0x3ab551ce,	0xa3bc0074,	0xd4bb30e2,
	0x4adfa541,	0x3dd895d7,	0xa4d1c46d,	0xd3d6f4fb,
	0x4369e96a,	0x346ed9fc,	0xad678846,	0xda60b8d0,
	0x44042d73,	0x33031de5,	0xaa0a4c5f,	0xdd0d7cc9,
	0x5005713c,	0x270241aa,	0xbe0b1010,	0xc90c2086,
	0x5768b525,	0x206f85b3,	0xb966d409,	0xce61e49f,
	0x5edef90e,	0x29d9c998,	0xb0d09822,	0xc7d7a8b4,
	0x59b33d17,	0x2eb40d81,	0xb7bd5c3b,	0xc0ba6cad,
	0xedb88320,	0x9abfb3b6,	0x03b6e20c,	0x74b1d29a,
	0xead54739,	0x9dd277af,	0x04db2615,	0x73dc1683,
	0xe3630b12,	0x94643b84,	0x0d6d6a3e,	0x7a6a5aa8,
	0xe40ecf0b,	0x9309ff9d,	0x0a00ae27,	0x7d079eb1,
	0xf00f9344,	0x8708a3d2,	0x1e01f268,	0x6906c2fe,
	0xf762575d,	0x806567cb,	0x196c3671,	0x6e6b06e7,
	0xfed41b76,	0x89d32be0,	0x10da7a5a,	0x67dd4acc,
	0xf9b9df6f,	0x8ebeeff9,	0x17b7be43,	0x60b08ed5,
	0xd6d6a3e8,	0xa1d1937e,	0x38d8c2c4,	0x4fdff252,
	0xd1bb67f1,	0xa6bc5767,	0x3fb506dd,	0x48b2364b,
	0xd80d2bda,	0xaf0a1b4c,	0x36034af6,	0x41047a60,
	0xdf60efc3,	0xa867df55,	0x316e8eef,	0x4669be79,
	0xcb61b38c,	0xbc66831a,	0x256fd2a0,	0x5268e236,
	0xcc0c7795,	0xbb0b4703,	0x220216b9,	0x5505262f,
	0xc5ba3bbe,	0xb2bd0b28,	0x2bb45a92,	0x5cb36a04,
	0xc2d7ffa7,	0xb5d0cf31,	0x2cd99e8b,	0x5bdeae1d,
	0x9b64c2b0,	0xec63f226,	0x756aa39c,	0x026d930a,
	0x9c0906a9,	0xeb0e363f,	0x72076785,	0x05005713,
	0x95bf4a82,	0xe2b87a14,	0x7bb12bae,	0x0cb61b38,
	0x92d28e9b,	0xe5d5be0d,	0x7cdcefb7,	0x0bdbdf21,
	0x86d3d2d4,	0xf1d4e242,	0x68ddb3f8,	0x1fda836e,
	0x81be16cd,	0xf6b9265b,	0x6fb077e1,	0x18b74777,
	0x88085ae6,	0xff0f6a70,	0x66063bca,	0x11010b5c,
	0x8f659eff,	0xf862ae69,	0x616bffd3,	0x166ccf45,
	0xa00ae278,	0xd70dd2ee,	0x4e048354,	0x3903b3c2,
	0xa7672661,	0xd06016f7,	0x4969474d,	0x3e6e77db,
	0xaed16a4a,	0xd9d65adc,	0x40df0b66,	0x37d83bf0,
	0xa9bcae53,	0xdebb9ec5,	0x47b2cf7f,	0x30b5ffe9,
	0xbdbdf21c,	0xcabac28a,	0x53b39330,	0x24b4a3a6,
	0xbad03605,	0xcdd70693,	0x54de5729,	0x23d967bf,
	0xb3667a2e,	0xc4614ab8,	0x5d681b02,	0x2a6f2b94,
	0xb40bbe37,	0xc30c8ea1,	0x5a05df1b,	0x2d02ef8d,
};

static uint32_t soft_crc32(const void *__data, int data_size, uint32_t crc)
{
	const uint8_t *data = __data;
	unsigned int result = crc;
	unsigned char crc_H8;

	while (data_size--) {
		crc_H8 = (unsigned char)(result & 0x000000FF);
		result >>= 8;
		result ^= crc_table[crc_H8 ^ (*data)];
		data++;
	}

	return result;
}


#define DEFAULT_LAYOUT_FILE	"layout.txt"
#define DEFAULT_CONFIG_FILE	"flash_k5.config"
#define DEFAULT_OUTPUT_FILE	"wmpt.flash"

#define MAX_PARTITIONS	((FL_PART_SIZE - sizeof(struct partition_table)) / sizeof(struct partition_entry))

static const uint8_t filler[] = {0xff};
#define FILLER_SIZE	(sizeof(filler) / sizeof(filler[0]))

#define LINE_BUF_MAX	256

static struct partition_table p_table;
static struct partition_entry p_entries[MAX_PARTITIONS];

static const struct {
	const char *component;
	enum flash_comp type;
} component_table[] = {
	{"FC_COMP_BOOT2", FC_COMP_BOOT2},
	{"FC_COMP_FW", FC_COMP_FW},
	{"FC_COMP_WLAN_FW", FC_COMP_WLAN_FW},
	{"FC_COMP_FTFS", FC_COMP_FTFS},
	{"FC_COMP_PSM", FC_COMP_PSM},
	{"FC_COMP_USER_APP", FC_COMP_USER_APP},
};
#define NUM_COMPONENTS	(sizeof(component_table) / sizeof(component_table[0]))

static void
pe_copy(FILE *fout, struct partition_entry *pe, const char *path)
{
	FILE *fp;
	int c;
	unsigned long n = 0;

	if ((fp = fopen(path, "r")) == NULL) {
		perror(path);
		exit(-1);
	}
	fseek(fout, pe->start, SEEK_SET);
	if (pe->type == FC_COMP_WLAN_FW) {
		struct wlan_fw_header fh;
		struct stat stbuf;
		if (fstat(fileno(fp), &stbuf) != 0) {
			perror("stat");
			exit(-1);
		}
		fh.magic = WLAN_FW_MAGIC;
		fh.length = stbuf.st_size;
		fwrite(&fh, sizeof(fh), 1, fout);
		n += sizeof(fh);
	}
	while ((c = getc(fp)) != EOF) {
		if (++n >= pe->size) {
			fprintf(stderr, "%s: too big\n", path);
			break;
		}
		putc(c, fout);
	}
	fclose(fp);
}

static void
pe_process(FILE *fout, const char *pename, const char *path)
{
	struct partition_entry *pe;
	int i;

	for (i = 0; i < p_table.partition_entries_no; i++) {
		pe = &p_entries[i];
		if (strcmp(pename, pe->name) == 0)
			pe_copy(fout, pe, path);
	}
}

static void
fill(FILE *fout)
{
	struct partition_entry *pe;
	unsigned int i, j;

	for (i = 0; i < p_table.partition_entries_no; i++) {
		pe = &p_entries[i];
		fseek(fout, pe->start, SEEK_SET);
		for (j = 0; j < pe->size; j++)
			putc(filler[j % FILLER_SIZE], fout);
	}
}

static void
pt_write(FILE *fout, uint32_t offset)
{
	unsigned int i;
	uint32_t crc;

	fseek(fout, offset, SEEK_SET);
	fwrite(&p_table, sizeof(p_table), 1, fout);
	for (i = 0; i < p_table.partition_entries_no; i++)
		fwrite(&p_entries[i], sizeof(struct partition_entry), 1, fout);
	crc = soft_crc32(p_entries, sizeof(struct partition_entry) * p_table.partition_entries_no, 0);
	fwrite(&crc, sizeof(crc), 1, fout);
}

static enum flash_comp
component_to_type(const char *component)
{
	int i;

	for (i = 0; i < NUM_COMPONENTS; i++) {
		if (strcmp(component, component_table[i].component) == 0)
			return component_table[i].type;
	}
	fprintf(stderr, "%s: not found\n", component);
	exit(-1);
}

int
main(int ac, char *av[])
{
	const char *outfile = DEFAULT_OUTPUT_FILE;
	const char *bindir = NULL;
	const char *layout = DEFAULT_LAYOUT_FILE;
	const char *conf = DEFAULT_CONFIG_FILE;
	char path[PATH_MAX];
	FILE *fp, *fout;
	char buf[LINE_BUF_MAX], component[LINE_BUF_MAX], name[LINE_BUF_MAX];
	struct partition_entry *pe;

	for (--ac, av++; --ac >= 0; av++) {
		if (**av == '-' && ac > 0) {
			char *opt = *av++;
			--ac;
			switch (opt[1]) {
			case 'o':
				outfile = *av;
				break;
			case 'b':
				bindir = *av;
				break;
			case 'l':
				layout = *av;
				break;
			case 'c':
				conf = *av;
				break;
			default:
				fprintf(stderr, "usage: wmpt [-o outfile] [-b bindir] [-l layout_file] [-c conf_file]\n");
				exit(-1);
			}
		}
	}

	p_table.magic = PARTITION_TABLE_MAGIC;
	p_table.version = PARTITION_TABLE_VERSION;
	p_table.gen_level = 0;

	/* read the layout */
	if (bindir != NULL) {
		snprintf(path, sizeof(path), "%s/%s", bindir, layout);
		layout = path;
	}
	if ((fp = fopen(layout, "r")) == NULL) {
		perror(layout);
		exit(-1);
	}
	pe = p_entries;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		unsigned int addr, size;
		int device;
		if (buf[0] == '#')
			continue;
		if (sscanf(buf, "%s 0x%x 0x%x %d %s", component, &addr, &size, &device, name) == 5) {
			pe->type = component_to_type(component);
			pe->device = device;
			strncpy(pe->name, name, sizeof(pe->name));
			pe->start = addr;
			pe->size = size;
			pe->gen_level = 1;
			pe++;
			if (pe - p_entries >= MAX_PARTITIONS)
				break;
		}
	}
	fclose(fp);
	p_table.partition_entries_no = pe - p_entries;
	p_table.crc = soft_crc32(&p_table, sizeof(p_table) - sizeof(p_table.crc), 0);

	/* fill the output file with a default pattern */
	if ((fout = fopen(outfile, "w+")) == NULL) {
		perror(outfile);
		exit(-1);
	}
	fill(fout);

	/* write the parition table */
	pt_write(fout, FL_PART1_START);
	pt_write(fout, FL_PART2_START);

	/* process the conf file */
	if (bindir != NULL) {
		snprintf(path, sizeof(path), "%s/%s", bindir, conf);
		conf = path;
	}
	if ((fp = fopen(conf, "r")) == NULL) {
		perror(conf);
		exit(-1);
	}
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (buf[0] == '#')
			continue;
		if (sscanf(buf, "%s %s", name, path) == 2)
			pe_process(fout, name, path);
	}
	fclose(fp);
	fclose(fout);
	return 0;
}
