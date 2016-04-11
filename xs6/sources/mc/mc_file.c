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
#if !USE_NATIVE_STDIO
#if mxMC
#include <wmstdio.h>
#include <wm_os.h>
#include <flash.h>
#include <partition.h>
#include <ftfs.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

#define MANAGE_OPEN_COUNT	1
#include "mc_file.h"

#define FFS_MAGIC	0xab	/* anything but 0 or ff */
#define FFS_SECTOR_SIZE	(4*1024)
#define FFS_OCREAT	0x1
#define FFS_OTRUNC	0x2
#define FFS_ORDWR	0x4
#define FFS_OPASSIVE	0x8
#define FFS_OACTIVE	0x10

#define FTFS_PATH	"/ftfs/"

typedef struct {
	uint8_t magic;
	int8_t nextblk;
	uint16_t size;
	char name[FT_MAX_FILENAME];
} ffs_header_t;

typedef struct ffs {
	mdev_t *mdev;
	flash_desc_t fd;
	uint8_t flags;
	int dirty;
	int blk, startblk;
	int bp;
#if MC_LONG_PATH
	int startbp;
#endif
	size_t (*read_f)(void *, size_t, size_t, struct ffs *);
	size_t (*write_f)(const void *, size_t, size_t, struct ffs *);
	long (*position_f)(struct ffs *, long position, int setp);
	enum {FFS_FILE, FFS_VOLUME} kind;
	struct partition_entry *pe;
	union {
		ffs_header_t h;
		uint8_t data[FFS_SECTOR_SIZE];
	} u;
} ffs_t;


struct mc_dir {
	mdev_t *mdev;
	int block;
	flash_desc_t fd;
};

#define FFS_HEADER_SIZE		sizeof(ffs_header_t)
#define FFS_CONTENT_SIZE	(FFS_SECTOR_SIZE - FFS_HEADER_SIZE)
#if MC_LONG_PATH
#define FFS_PATH_MAX		(FFS_SECTOR_SIZE - FFS_HEADER_SIZE + FT_MAX_FILENAME)
#endif

static struct fs *default_ftfs;
#if WMSDK_VERSION < 2040000
static mdev_t *fl_dev = NULL;
#endif

#if !mxMC
#define SIMULATE_ERASE_AND_WRITE	1

#define N_PARTITIONS	12
static struct partition_entry sPartitions[N_PARTITIONS] = {
	{.type = FC_COMP_BOOT2, .device = 0, .name = "boot2", .start = 0x0, .size = 0x6000, .gen_level = 0},
	{.type = FC_COMP_PSM, .device = 0, .name = "psm", .start = 0x6000, .size = 0x4000, .gen_level = 0},
	{.type = FC_COMP_FW, .device = 0, .name = "mcufw", .start = 0xa000, .size = 0x120000, .gen_level = 0},
	{.type = FC_COMP_FW, .device = 0, .name = "mcufw", .start = 0x12a000, .size = 0x120000, .gen_level = 0},
	{.type = FC_COMP_FTFS, .device = 0, .name = "ftfs", .start = 0x24a000, .size = 0x50000, .gen_level = 0},
	{.type = FC_COMP_FTFS, .device = 0, .name = "ftfs", .start = 0x29a000, .size = 0x50000, .gen_level = 0},
	{.type = FC_COMP_WLAN_FW, .device = 0, .name = "wififw", .start = 0x2ea000, .size = 0x40000, .gen_level = 0},
	{.type = FC_COMP_WLAN_FW, .device = 0, .name = "wififw", .start = 0x32a000, .size = 0x40000, .gen_level = 0},
	{.type = FC_COMP_USER_APP, .device = 0, .name = "k0", .start = 0x36a000, .size = 0x20000, .gen_level = 0},
	{.type = FC_COMP_USER_APP, .device = 0, .name = "k1", .start = 0x38a000, .size = 0x20000, .gen_level = 0},
	{.type = FC_COMP_USER_APP, .device = 0, .name = "k2", .start = 0x3aa000, .size = 0x20000, .gen_level = 0},
	{.type = FC_COMP_USER_APP, .device = 0, .name = "k3", .start = 0x8ca000, .size = 0x20000, .gen_level = 0},
};
static mdev_t sDevs[N_PARTITIONS];

static void part_to_flash_desc(struct partition_entry *p, flash_desc_t *f)
{
	for (int i = 0; i < N_PARTITIONS; i++) {
		if (strcmp(p->name, sPartitions[i].name) == 0) {
			f->fl_dev = i;
			f->fl_start = sPartitions[i].start;
			f->fl_size = sPartitions[i].size;
			break;
		}
	}
}

static struct partition_entry *part_get_layout_by_id(enum flash_comp comp, short *start_index)
{
	struct partition_entry *ent = NULL;
	int i = start_index ? *start_index : 0;

	for (; i < N_PARTITIONS; i++) {
		if (comp == sPartitions[i].type) {	// does not work with FC_COMP_USER_APP
			if (start_index)
				*start_index = i + 1;
			ent = &sPartitions[i];
			break;
		}
	}

	return ent;
}

static struct partition_entry *part_get_layout_by_name(const char *name, short *start_index)
{
	struct partition_entry *ent = NULL;
	int i = start_index ? *start_index : 0;

	for (; i < N_PARTITIONS; i++) {
		if (strncmp(name, sPartitions[i].name, MAX_NAME) == 0) {
			if (start_index)
				*start_index = i + 1;
			ent = &sPartitions[i];
			break;
		}
	}

	return ent;
}

struct partition_entry *part_get_active_partition(struct partition_entry *p1,
												  struct partition_entry *p2)
{
	if (p1 && p2)
		return p1->gen_level >= p2->gen_level ? p1 : p2;
	else
		return NULL;
}

static struct partition_entry *part_get_passive_partition(struct partition_entry *p1,
														  struct partition_entry *p2)
{
	if (p1 && p2)
		return p1->gen_level < p2->gen_level ? p1 : p2;
	else
		return NULL;
}

static int part_set_active_partition(struct partition_entry *p)
{
	p->gen_level++;
	/* update the partition table */
	return 0;
}

static struct partition_entry *part_get_active_partition_by_name(const char *name)
{
	short index = 0;
	struct partition_entry *p1 = part_get_layout_by_name(name, &index);
	struct partition_entry *p2 = part_get_layout_by_name(name, &index);

	return part_get_active_partition(p1, p2);
}

static struct partition_entry *part_get_passive_partition_by_name(const char *name)
{
	short index = 0;
	struct partition_entry *p1 = part_get_layout_by_name(name, &index);
	struct partition_entry *p2 = part_get_layout_by_name(name, &index);

	return part_get_passive_partition(p1, p2);
}

static void flash_drv_init()
{
	int i;

	for (i = 0; i < N_PARTITIONS; i++)
		sDevs[i].index = i;
}

static const char *flash_path(const char *name)
{
	static char path[PATH_MAX];
	const char *dir;

	dir = mc_resolve_path(name, 1);
	sprintf(path, "%s.dat", dir);
	return path;
}

static mdev_t *flash_drv_open(int fl_dev)
{
	mdev_t *dev = NULL;

	if ((fl_dev < 0) || (fl_dev >= N_PARTITIONS)) {
		return NULL;
	}

	dev = &sDevs[fl_dev];

	if (fl_dev > 0) {
#if !MANAGE_OPEN_COUNT
		if (dev->openCount == 0) {
#endif
			FILE *fp;
			const char *path;

#if SIMULATE_ERASE_AND_WRITE
			dev->data = mc_malloc(sPartitions[dev->index].size);
			if (dev->data == NULL) {
				return NULL;
			}
			memset(dev->data, 0xff, sPartitions[dev->index].size);
#else
			dev->data = mc_calloc(1, sPartitions[dev->index].size);
			if (dev->data == NULL) {
				return NULL;
			}
#endif
			path = flash_path(sPartitions[dev->index].name);
			if ((fp = fopen(path, "r")) != NULL) {
				fread(dev->data, sPartitions[dev->index].size, 1, fp);
				fclose(fp);
			}
#if !MANAGE_OPEN_COUNT
		}
#endif
	}

#if !MANAGE_OPEN_COUNT
	dev->openCount++;
#endif

	return dev;
}

static int flash_drv_close(mdev_t *dev)
{
	if (dev == NULL) {
		return WM_FAIL;
	}
#if !MANAGE_OPEN_COUNT
	if (dev->openCount == 0) {
		return WM_FAIL;
	}

	dev->openCount--;
#endif

	if ((dev->index > 0)
#if !MANAGE_OPEN_COUNT
		&& (dev->openCount == 0)
#endif
		) {
		if (dev->dirty) {
			FILE *fp;
			const char *path;

			path = flash_path(sPartitions[dev->index].name);
			if ((fp = fopen(path, "w")) != NULL) {
				fwrite(dev->data, sPartitions[dev->index].size, 1, fp);
				fclose(fp);
			}
			dev->dirty = false;
		}
		mc_free(dev->data);
		dev->data = NULL;
	}

	return WM_SUCCESS;
}

static int flash_drv_write(mdev_t *dev, uint8_t *buf, uint32_t len, uint32_t addr)
{
	struct partition_entry *partition = &sPartitions[dev->index];
#if SIMULATE_ERASE_AND_WRITE
	int i;
#endif

	addr -= partition->start;

	if ((addr + len) > partition->size) {
		return WM_FAIL;
	}

#if SIMULATE_ERASE_AND_WRITE
	// all bits should be set 1.
	for (i = 0; i < len; i++) {
		dev->data[addr + i] &= buf[i];
	}
#else
	memcpy(dev->data + addr, buf, len);
#endif

	dev->dirty = true;

	return WM_SUCCESS;
}

static int flash_drv_read(mdev_t *dev, uint8_t *buf, uint32_t len, uint32_t addr)
{
	struct partition_entry *partition = &sPartitions[dev->index];

	addr -= partition->start;

	if ((addr + len) > partition->size) {
		return WM_FAIL;
	}

	memcpy(buf, dev->data + addr, len);

	return WM_SUCCESS;
}

static int flash_drv_erase(mdev_t *dev, unsigned long start, unsigned long size)
{
	struct partition_entry *partition = &sPartitions[dev->index];

	start -= partition->start;

	if ((start + size) > partition->size) {
		return WM_FAIL;
	}

#if SIMULATE_ERASE_AND_WRITE
	// all bits are set 1.
	memset(dev->data + start, 0xff, size);
#else
	bzero(dev->data + start, size);
#endif

	dev->dirty = true;

	return WM_SUCCESS;
}

struct fs *ftfs_init(struct ftfs_super *sb, flash_desc_t *fd)
{
	memset(sb, 0, sizeof(struct ftfs_super));

	return &sb->fs;
}

file *ft_fopen(struct fs *fs, const char *path, const char *mode)
{
	path = mc_resolve_path(path, 0);
	errno = 0;
	return fopen(path, mode);
}
int ft_fclose(file *f)
{
	fclose(f);
	return WM_SUCCESS;
}
size_t ft_fread(void *ptr, size_t size, size_t nmemb, file *f)
{
	return fread(ptr, size, nmemb, f);
}
long ft_ftell(file *f)
{
	return ftell(f);
}
int ft_fseek(file *f, long offset, int whence)
{
	return fseek(f, offset, whence);
}
int ft_ftruncate(file *f, size_t length)
{
	return ftruncate(fileno(f), length);
}
int
ft_creat(const char *path, mode_t mode)
{
	return creat(mc_resolve_path(path, 1), mode);
}
int
ft_unlink(const char *path)
{
	return unlink(mc_resolve_path(path, 1));
}

static bool sync_with_native_path(const char *path)
{
	int i;

	for (i = 0; i < N_PARTITIONS; i++) {
		if (sPartitions[i].type == FC_COMP_USER_APP) {
			if (strncmp(path + 1, sPartitions[i].name, strlen(sPartitions[i].name)) == 0) {
				return true;
			}
		}
	}

	return false;
}
#endif	/* !mxMC */

static struct partition_entry *ffs_get_active_partition_by_name(const char *name)
{
	struct partition_entry *pe;

	pe = part_get_active_partition_by_name(name);
	if (pe == NULL) {
		// part_get_active_partition_by_name returns NULL,
		// if only one partition exists...
		pe = part_get_layout_by_name(name, NULL);
	}

	return pe;
}

#if MANAGE_OPEN_COUNT
#define MAX_FLASH_DEV	64

struct flash_list {
	uint8_t fl_dev;
	mdev_t *mdev;
	uint32_t openCount;
};

static struct flash_list flashList[MAX_FLASH_DEV];
static uint32_t flashListCount = 0;
#endif

static mdev_t *
ffs_flash_open(struct partition_entry *pe, flash_desc_t	*fd)
{
#if MANAGE_OPEN_COUNT
	struct flash_list *flash;
	uint32_t i;

	part_to_flash_desc(pe, fd);

	flash = NULL;
	for (i = 0; i < flashListCount; i++) {
		if (fd->fl_dev == flashList[i].fl_dev) {
			flash = &flashList[i];
			break;
		}
	}
	if (flash == NULL) {
		if (flashListCount == MAX_FLASH_DEV) {
			mc_log_error("ffs_flash_open: exceed MAX_FLASH_DEV\n");
			return NULL;
		}

		flash = &flashList[flashListCount++];
		flash->fl_dev = fd->fl_dev;
		flash->mdev = NULL;
		flash->openCount = 0;
	}
	if (flash->openCount++ == 0) {
		if ((flash->mdev = flash_drv_open(flash->fl_dev)) == NULL) {
			mc_log_error("ffs_flash_open: flash_drv_open failed\n");
			return NULL;
		}
	}

	return flash->mdev;
#else
	part_to_flash_desc(pe, fd);
	return flash_drv_open(fd->fl_dev);
#endif
}

static void
ffs_flash_close(mdev_t *mdev) {
#if MANAGE_OPEN_COUNT
	struct flash_list *flash;
	uint32_t i = 0;

	flash = NULL;
	for (i = 0; i < flashListCount; i++) {
		if (mdev == flashList[i].mdev) {
			flash = &flashList[i];
			break;
		}
	}
	if ((flash != NULL) && flash->openCount > 0) {
		if (--flash->openCount == 0) {
			flash_drv_close(flash->mdev);
			flash->mdev = NULL;
		}
	}
#else
	flash_drv_close(mdev);
#endif
}

#define CHECKED_BLK_ALLOC_SIZE	4
#define CHECKED_BLK_MAGIC		0xffffffff
static size_t *checked_free_blk = NULL;
static size_t checked_free_blk_size = 0;

static int
ffs_free_block_is_checked(size_t blk)
{
	size_t i;

	for (i = 0; i < checked_free_blk_size; i++)
		if (checked_free_blk[i] == blk)
			return 1;

	return 0;
}

static int
ffs_check_free_block(size_t blk)
{
	size_t alloc_size;
	size_t i;

	for (i = 0; i < checked_free_blk_size; i++) {
		if (checked_free_blk[i] == CHECKED_BLK_MAGIC) {
			checked_free_blk[i] = blk;
			goto bail;
		}
	}

	alloc_size = sizeof(size_t) * (checked_free_blk_size + CHECKED_BLK_ALLOC_SIZE);
	if (checked_free_blk == NULL) {
		if ((checked_free_blk = mc_malloc(alloc_size)) == NULL) {
			mc_log_error("ffs_check_free_block: mc_malloc failed\n");
			return -1;
		}
	} else {
		size_t *new_checked_free_blk;

		if ((new_checked_free_blk = mc_realloc(checked_free_blk, alloc_size)) == NULL) {
			mc_log_error("ffs_check_free_block: mc_realloc failed\n");
			return -1;
		}
		checked_free_blk = new_checked_free_blk;
	}

	checked_free_blk[checked_free_blk_size] = blk;
	for (i = 1; i < CHECKED_BLK_ALLOC_SIZE; i++) {
		checked_free_blk[checked_free_blk_size + i] = CHECKED_BLK_MAGIC;
	}
	checked_free_blk_size += CHECKED_BLK_ALLOC_SIZE;

bail:
//	mc_log_debug("%d is checked\n", (int)blk);

	return 0;
}

static int
ffs_uncheck_free_block(size_t blk)
{
	size_t i;
	for (i = 0; i < checked_free_blk_size; i++) {
		if (checked_free_blk[i] == blk) {
			checked_free_blk[i] = CHECKED_BLK_MAGIC;
//			mc_log_debug("%d is unchecked\n", (int)blk);
			return 0;
		}
	}
	return -1;
}

static int
ffs_find_free_block(ffs_t *ffs)
{
	size_t blk, offset;
	ffs_header_t h;

	for (blk = 0; (offset = blk * FFS_SECTOR_SIZE) + FFS_SECTOR_SIZE <= ffs->fd.fl_size; blk++) {
		if (ffs_free_block_is_checked(blk)) continue;

		if (flash_drv_read(ffs->mdev, (uint8_t *)&h, sizeof(h), ffs->fd.fl_start + offset) != WM_SUCCESS) {
			mc_log_error("ffs_find_free_block: flash_drv_read failed\n");
			return -1;
		}
		if (h.magic != FFS_MAGIC) {
			ffs_check_free_block(blk);
			return blk;
		}
	}
	mc_log_error("ffs_find_free_block: no more block\n");
	return -1;
}

#define ZERO_CLEAR_ERACE	0

static int
ffs_clear_block(ffs_t *ffs, size_t offset)
{
#if ZERO_CLEAR_ERACE
	ffs_header_t h;
#endif

	offset += ffs->fd.fl_start;
	if (flash_drv_erase(ffs->mdev, offset, FFS_SECTOR_SIZE) != WM_SUCCESS) {
		mc_log_error("ffs_clear_block: flash_drv_erase failed\n");
		return -1;
	}

#if ZERO_CLEAR_ERACE	// flash_drv_erase will turn all bits of sector into 1
	bzero(&h, sizeof(h));
	if (flash_drv_write(ffs->mdev, (uint8_t *)&h, sizeof(h), offset) != WM_SUCCESS) {
		mc_log_error("ffs_clear_block: flash_drv_write failed\n");
		return -1;
	}
#endif

	return 0;
}

static int
ffs_write_block(ffs_t *ffs, size_t offset)
{
	offset += ffs->fd.fl_start;

	ffs_uncheck_free_block(ffs->blk);

#if 0 && !ZERO_CLEAR_ERACE
	if (!ffs_free_block_is_checked(ffs->blk))	// block must be erased if it was free block.
#endif
	(void)flash_drv_erase(ffs->mdev, offset, FFS_SECTOR_SIZE);

	if (flash_drv_write(ffs->mdev, ffs->u.data, FFS_SECTOR_SIZE, offset) != WM_SUCCESS) {
		mc_log_error("ffs_write_block: flash_drv_write failed\n");
		errno = EIO;
		return -1;
	}
	return 0;
}

static int
ffs_flush(ffs_t *ffs)
{
	if ((ffs->flags & (FFS_ORDWR | FFS_OCREAT)) && ffs->dirty) {
		int offset = ffs->blk * FFS_SECTOR_SIZE;
		if (ffs_write_block(ffs, offset) != 0) {
			mc_log_error("ffs_close: failed to flush\n");
			return -1;
		}
		ffs->dirty = 0;
	}
	return 0;
}

static int
ffs_truncate(ffs_t *ffs)
{
	int blk = ffs->u.h.nextblk;
	ffs_header_t h;

	while (blk >= 0) {
		int offset = blk * FFS_SECTOR_SIZE;
		if (flash_drv_read(ffs->mdev, (uint8_t *)&h, sizeof(h), ffs->fd.fl_start + offset) != WM_SUCCESS) {
			mc_log_error("fs_truncate: failed to read at %d\n", blk);
			errno = EIO;
			return -1;
		}
		if (ffs_clear_block(ffs, offset)) {
			mc_log_error("ffs_clear_block: failed to erase at %d\n", blk);
			errno = EIO;
			return -1;
		}
		blk = h.nextblk;
	}
	ffs->u.h.size = ffs->bp;
	ffs->u.h.nextblk = -1;
	ffs->dirty = true;
	return 0;
}

static long
ffs_position_file(ffs_t *ffs, long position, int setp)
{
	int blk = ffs->startblk, lastblk = blk;
	long pos = 0;
	int bp, hsize;
	ffs_header_t *hp, h;
	size_t sz;

	if (setp) {
		ffs_flush(ffs);
		hp = &ffs->u.h;
		sz = FFS_SECTOR_SIZE;
	}
	else {
		hp = &h;
		sz = sizeof(h);
	}
	while (blk >= 0) {
		if (flash_drv_read(ffs->mdev, (uint8_t *)hp, sz, ffs->fd.fl_start + blk * FFS_SECTOR_SIZE) != WM_SUCCESS) {
			mc_log_error("ffs_position: failed to read at %d\n", blk);
			errno = EIO;
			return -1L;
		}
		bp = ffs->bp;
		hsize = hp->size;
#if MC_LONG_PATH
		if (blk == ffs->startblk) {
			bp -= ffs->startbp;
			hsize -= ffs->startbp;
		}
#endif
		if (position < 0 && blk == ffs->blk) {
			return pos + bp;
		}
		else if (position >= pos && position < pos + hsize) {
			if (setp) {
				ffs->blk = blk;
				ffs->bp = position - pos;
#if MC_LONG_PATH
				if (blk == ffs->startblk) {
					ffs->bp += ffs->startbp;
				}
#endif
			}
			return position;
		}
		if (hp->magic != FFS_MAGIC)
			break;
		pos += hsize;
		lastblk = blk;
		blk = hp->nextblk;
	}
	if (setp) {
		ffs->blk = lastblk;
		ffs->bp = hp->size;
	}
	return pos;
}

static long
ffs_position_volume(ffs_t *ffs, long position, int setp)
{
	if (position < 0)
		return ffs->blk * FFS_SECTOR_SIZE + ffs->bp;	/* current position */
	else {
		if (position > (long)ffs->fd.fl_size)
			position = ffs->fd.fl_size;
		if (setp) {
			ffs_flush(ffs);
			ffs->blk = position / FFS_SECTOR_SIZE;
			ffs->bp = position % FFS_SECTOR_SIZE;
			/* read in the new current sector */
			if (flash_drv_read(ffs->mdev, ffs->u.data, FFS_SECTOR_SIZE, ffs->fd.fl_start + ffs->blk * FFS_SECTOR_SIZE) != WM_SUCCESS) {
				mc_log_error("ffs_position_volume: flash_drv_read failed\n");
				errno = EIO;
				return -1L;
			}
		}
		return position;
	}
}

static size_t
ffs_read_file(void *buffer, size_t size, size_t nelm, ffs_t *ffs)
{
	uint8_t *bp = buffer;
	long nbytes = (long)(size * nelm);
	long n;

	errno = 0;
	while (nbytes > 0) {
		if ((n = ffs->u.h.size - ffs->bp) > nbytes)
			n = nbytes;
		if (n <= 0) {
			if (ffs->u.h.nextblk < 0)
				break;
			if (flash_drv_read(ffs->mdev, ffs->u.data, FFS_SECTOR_SIZE, ffs->fd.fl_start + ffs->u.h.nextblk * FFS_SECTOR_SIZE) != WM_SUCCESS) {
				mc_log_error("ffs_read_file: flash_drv_read failed\n");
				errno = EIO;
				return 0;
			}
			/* sanity check */
			if (ffs->u.h.magic != FFS_MAGIC) {
				mc_log_error("ffs_read_file: wrong magic number!\n");
				errno = EFAULT;
				return 0;
			}
			ffs->blk = ffs->u.h.nextblk;
			ffs->bp = 0;
			if ((n = ffs->u.h.size - ffs->bp) > nbytes)
				n = nbytes;
		}
		memcpy(bp, &ffs->u.data[FFS_HEADER_SIZE + ffs->bp], n);
		nbytes -= n;
		bp += n;
		ffs->bp += n;
	}
	nbytes = bp - (uint8_t *)buffer;
	return (size_t)(nbytes / size);
}

static size_t
ffs_read_volume(void *buffer, size_t size, size_t nelm, ffs_t *ffs)
{
	uint8_t *bp = buffer;
	long nbytes = (long)(size * nelm);
	long n;

	errno = 0;
	while (nbytes > 0) {
		if (ffs->bp == 0) {
			if ((size_t)ffs->blk * FFS_SECTOR_SIZE >= ffs->fd.fl_size)
				break;
			if (flash_drv_read(ffs->mdev, ffs->u.data, FFS_SECTOR_SIZE, ffs->fd.fl_start + ffs->blk * FFS_SECTOR_SIZE) != WM_SUCCESS) {
				mc_log_error("ffs_read_volume: flash_drv_read failed\n");
				errno = EIO;
				return 0;
			}
		}
		if ((n = FFS_SECTOR_SIZE - ffs->bp) > nbytes)
			n = nbytes;
		memcpy(bp, &ffs->u.data[ffs->bp], n);
		nbytes -= n;
		bp += n;
		ffs->bp += n;
		if (ffs->bp >= FFS_SECTOR_SIZE) {
			ffs->bp = 0;
			ffs->blk++;
		}
	}
	nbytes = bp - (uint8_t *)buffer;
	return (size_t)(nbytes / size);
}

static size_t
ffs_write_file(const void *buffer, size_t size, size_t nelm, ffs_t *ffs)
{
	const uint8_t *bp = buffer;
	long nbytes = (long)(size * nelm);
	long n;

	while (nbytes > 0) {
		if ((n = (FFS_CONTENT_SIZE - ffs->bp)) > nbytes)
			n = nbytes;
		if (n <= 0) {
			int offset = ffs->blk * FFS_SECTOR_SIZE;
			int freeblk;
			if ((freeblk = ffs_find_free_block(ffs)) < 0) {
				mc_log_error("ffs_write_file: no free blocks\n");
				errno = ENOSPC;
				return 0;
			}
			ffs->u.h.nextblk = freeblk;
			if (ffs_write_block(ffs, offset) != 0) {
				mc_log_error("ffs_write_file: ffs_write_block failed\n");
				errno = EIO;
				return 0;
			}
			ffs->u.h.magic = FFS_MAGIC;
			ffs->u.h.nextblk = -1;
			ffs->u.h.size = 0;
			ffs->bp = 0;
			ffs->blk = freeblk;
			bzero(ffs->u.h.name, sizeof(ffs->u.h.name));	/* erase the filename from the header */
			if ((n = FFS_CONTENT_SIZE) > nbytes)
				n = nbytes;
		}
		memcpy(&ffs->u.data[FFS_HEADER_SIZE + ffs->bp], bp, n);
		nbytes -= n;
		bp += n;
		ffs->bp += n;
		if (ffs->bp > ffs->u.h.size)
			ffs->u.h.size = ffs->bp;
		ffs->dirty = true;
	}
	nbytes = bp - (uint8_t *)buffer;
	return (size_t)(nbytes / size);
}

static size_t
ffs_write_volume(const void *buffer, size_t size, size_t nelm, ffs_t *ffs)
{
	const uint8_t *bp = buffer;
	long nbytes = (long)(size * nelm);
	long n;

	while (nbytes > 0) {
		if ((n = FFS_SECTOR_SIZE - ffs->bp) > nbytes)
			n = nbytes;
		if (n <= 0) {
			int offset = ffs->blk * FFS_SECTOR_SIZE;
			if (ffs_write_block(ffs, offset) != 0) {
				mc_log_error("ffs_write_volume: ffs_write_block failed\n");
				errno = EIO;
				return 0;
			}
			if ((unsigned)(ffs->blk + 1) * FFS_SECTOR_SIZE + FFS_SECTOR_SIZE > ffs->fd.fl_size) {
				mc_log_error("ffs_write_volume: disk full! %d > %d\n", (ffs->blk + 1) * FFS_SECTOR_SIZE, ffs->fd.fl_size);
				errno = ENOSPC;
				return 0;
			}
			ffs->blk++;
			ffs->bp = 0;
			if ((n = FFS_SECTOR_SIZE) > nbytes)
				n = nbytes;
		}
		memcpy(&ffs->u.data[ffs->bp], bp, n);
		nbytes -= n;
		bp += n;
		ffs->bp += n;
		ffs->dirty = true;
	}
	nbytes = bp - (uint8_t *)buffer;
	return (size_t)(nbytes / size);
}

static int
ffs_open_file(ffs_t *ffs, const char *fname)
{
	int blk;
	uint32_t offset;
	int targetblk = -1;
	int first_freeblk = -1;

	/* seek to the file position */
	for (blk = 0; (offset = blk * FFS_SECTOR_SIZE) + FFS_SECTOR_SIZE <= ffs->fd.fl_size; blk++) {
		if (ffs_free_block_is_checked(blk)) continue;

		if (flash_drv_read(ffs->mdev, ffs->u.data, FFS_SECTOR_SIZE, ffs->fd.fl_start + offset) != WM_SUCCESS) {
			mc_log_error("FFS: flash_drv_read failed!\n");
			return EIO;
		}
		if (ffs->u.h.magic != FFS_MAGIC) {
			if (first_freeblk < 0)
				first_freeblk = blk;
		}
#if MC_LONG_PATH
		else if (strncmp(fname, ffs->u.h.name, FFS_PATH_MAX) == 0) {
			targetblk = blk;
			break;
		}
#else
		else if (strncmp(fname, ffs->u.h.name, FT_MAX_FILENAME) == 0) {
			targetblk = blk;
			break;
		}
#endif
	}
#if MC_LONG_PATH
	size_t nameLen = strlen(fname);
	if (nameLen >= FFS_PATH_MAX) {
		mc_log_error("FFS: too long path\n");
		return ENOSPC;
	}
#endif
	if (targetblk >= 0) {
		ffs->blk = ffs->startblk = targetblk;
#if MC_LONG_PATH
		if (nameLen >= FT_MAX_FILENAME) {
			ffs->startbp = ffs->bp = nameLen - FT_MAX_FILENAME + 1;
		}
#endif
		if (ffs->flags & FFS_OTRUNC)
			ffs_truncate(ffs);
	}
	else {
		if (ffs->flags & FFS_OCREAT) {
			if (first_freeblk < 0) {
				mc_log_error("FFS: no space\n");
				return ENOSPC;
			}
			ffs->blk = ffs->startblk = first_freeblk;
			ffs->u.h.magic = FFS_MAGIC;
			ffs->u.h.nextblk = -1;
			ffs->u.h.size = 0;
#if MC_LONG_PATH
			if (nameLen >= FT_MAX_FILENAME) {
				ffs->startbp = ffs->bp = nameLen - FT_MAX_FILENAME + 1;
				memcpy(ffs->u.h.name, fname, nameLen);
				ffs->u.h.name[nameLen] = '\0';
			} else {
				strncpy(ffs->u.h.name, fname, FT_MAX_FILENAME);
			}
#else
			strncpy(ffs->u.h.name, fname, FT_MAX_FILENAME);
#endif
			ffs->dirty = true;
			ffs_check_free_block(first_freeblk);
		}
		else
			return ENOENT;
	}
	ffs->read_f = ffs_read_file;
	ffs->write_f = ffs_write_file;
	ffs->position_f = ffs_position_file;
	return 0;
}

static int
ffs_open_volume(ffs_t *ffs, const char *partition)
{
	/* do not need to erase here...
	if (ffs->flags & FFS_OTRUNC)
		(void)flash_drv_erase(ffs->mdev, ffs->fd.fl_start, ffs->fd.fl_size);
	*/
	ffs->read_f = ffs_read_volume;
	ffs->write_f = ffs_write_volume;
	ffs->position_f = ffs_position_volume;
	return 0;
}

static ffs_t *
ffs_open(const char *path, uint32_t flags)
{
#if WMSDK_VERSION < 2040000
	/* can't get the flash layout by id */
	errno = ENXIO;
	return NULL;
#else
	ffs_t *ffs;
	char *fullpath = NULL, *p, *partition, *fname;

	if ((ffs = mc_malloc(sizeof(ffs_t))) == NULL) {
		mc_log_error("ffs_open: mc_malloc failed\n");
		errno = ENOMEM;
		goto bail;
	}
	ffs->mdev = NULL;
	ffs->flags = flags;
	ffs->dirty = false;
	ffs->blk = ffs->startblk = 0;
	ffs->bp = 0;
#if MC_LONG_PATH
	ffs->startbp = 0;
#endif
	if ((fullpath = mc_strdup(path)) == NULL) {
		mc_log_error("ffs_open: mc_strdup failed\n");
		errno = ENOMEM;
		goto bail;
	}
	if (fullpath[0] != '/') {
		mc_log_error("ffs_open: invalid path %s\n", fullpath);
		errno = EINVAL;
		goto bail;
	}
#if MC_LONG_PATH
	if ((p = strchr(&fullpath[1], '/')) != NULL) {
		*p = '\0';
		partition = &fullpath[1];
		fname = p + 1;
	}
#else
	if ((p = strrchr(&fullpath[1], '/')) != NULL) {
		*p = '\0';
		partition = &fullpath[1];
		fname = p + 1;
	}
#endif
	else {
		partition = &fullpath[1];
		fname = NULL;
	}
	if (flags & FFS_OACTIVE)
		ffs->pe = part_get_active_partition_by_name(partition);
	else
		ffs->pe = part_get_passive_partition_by_name(partition);
	if (ffs->pe == NULL) {
		if ((ffs->pe = part_get_layout_by_name(partition, NULL)) == NULL) {
			mc_log_error("ffs_open: partition not found: %s\n", partition);
			errno = ENXIO;
			goto bail;
		}
	}
	if ((ffs->mdev = ffs_flash_open(ffs->pe, &ffs->fd)) == NULL) {
		mc_log_error("ffs_open: ffs_flash_open failed\n");
		errno = ENXIO;
		goto bail;
	}
	if (ffs->pe->type == FC_COMP_USER_APP && fname != NULL) {
		errno = ffs_open_file(ffs, fname);
		ffs->kind = FFS_FILE;
	}
	else {
		errno = ffs_open_volume(ffs, partition);
		ffs->kind = FFS_VOLUME;
	}
bail:
	if (fullpath != NULL)
		mc_free(fullpath);
	if (errno != 0) {
		if (ffs != NULL) {
			if (ffs->mdev != NULL) {
				ffs_flash_close(ffs->mdev);
				ffs->mdev = NULL;
			}
			mc_free(ffs);
			ffs = NULL;
		}
	}
	return ffs;
#endif
}

static int
ffs_close(ffs_t *ffs)
{
	int err;

	err = ffs_flush(ffs);
	ffs_flash_close(ffs->mdev);
	mc_free(ffs);
	return err;
}

static int
ffs_delete(const char *path)
{
	ffs_t *ffs;
	size_t offset;
	int err = 0;

	if ((ffs = ffs_open(path, FFS_OTRUNC)) == NULL) {
		if (errno == ENOENT)
			return 0;
		mc_log_error("ffs_delete: ffs_open failed\n");
		return -1;
	}
	offset = ffs->startblk * FFS_SECTOR_SIZE;
	if (ffs_clear_block(ffs, offset)) {
		mc_log_error("ffs_delete: ffs_clear_block failed\n");
		errno = EIO;
		err = -1;
	}
	ffs->dirty = false;
	(void)ffs_close(ffs);
	return err;
}

static int
ffs_rename(const char *from, const char *to)
{
	ffs_t *ffs;
	int err;
#if MC_LONG_PATH
	size_t fromLen;
	size_t toLen;
	int blk;
	uint32_t offset;
	uint8_t data[FFS_SECTOR_SIZE];
	ffs_header_t *h = (ffs_header_t *)data;

	mc_check_stack();
#else
	char *p;
#endif

	if ((ffs = ffs_open(from, 0)) == NULL)
		return -1;	/* probably NOENT? */
#if MC_LONG_PATH
	// @@ currentry, renaming from/to long path (except length is same) is not supported.
	fromLen = strlen(ffs->u.h.name);
	toLen = strlen(to);
	if (fromLen != toLen) {
		if ((fromLen >= FT_MAX_FILENAME) || (toLen >= FT_MAX_FILENAME)) {
			return -1;
		}
	}
	if (toLen >= FT_MAX_FILENAME) {
		memcpy(ffs->u.h.name, to, toLen);
		ffs->u.h.name[toLen] = '\0';
	} else {
		strncpy(ffs->u.h.name, to, FT_MAX_FILENAME);
	}

	for (blk = 0; (offset = blk * FFS_SECTOR_SIZE) + FFS_SECTOR_SIZE <= ffs->fd.fl_size; blk++) {
		if (flash_drv_read(ffs->mdev, data, FFS_SECTOR_SIZE, ffs->fd.fl_start + offset) != WM_SUCCESS) {
			mc_log_error("FFS: flash_drv_read failed!\n");
			return EIO;
		}
		if (h->magic != FFS_MAGIC) {
			continue;
		}
		if (strncmp(h->name, ffs->u.h.name, FFS_PATH_MAX) == 0) {
			return -1;
		}
	}
#else
	if ((p = strrchr(to, '/')) != NULL)
	to = p + 1;
	strncpy(ffs->u.h.name, to, FT_MAX_FILENAME);
#endif
	err = ffs_write_block(ffs, ffs->blk * FFS_SECTOR_SIZE);
	ffs->dirty = 0;
	(void)ffs_close(ffs);
	return err;
}

/*
 * file
 */

int
mc_file_init()
{
	static struct ftfs_super sb;
	static int flash_drv_inited = 0;

	if (!flash_drv_inited) {
		flash_drv_init();
		flash_drv_inited++;
	}
	if (default_ftfs != NULL)
		return 0;
#if WMSDK_VERSION < 2040000
	struct flash_layout *fl_layout;
	short history = 0;
	if ((fl_layout = flash_get_layout(FC_COMP_FTFS, &history)) == NULL) {
		errno = ENXIO;
		return -1;
	}
	if ((fl_dev = flash_drv_open(fl_layout->fl_flag)) == NULL) {
		errno = ENXIO;
		return -1;
	}
	default_ftfs = ftfs_init(&sb, fl_dev, fl_layout->fl_start);
#else
	struct partition_entry *pe;
	flash_desc_t fd;
	short idx = 0;
	struct partition_entry *pe1 = part_get_layout_by_id(FC_COMP_FTFS, &idx);
	struct partition_entry *pe2 = part_get_layout_by_id(FC_COMP_FTFS, &idx);

	if (pe1 && pe2)
		pe = part_get_active_partition(pe1, pe2);
	else if (pe1)
		pe = pe1;
	else if (pe2)
		pe = pe2;
	else {
		mc_log_error("ftfs not found\n");
		errno = ENXIO;
		return -1;
	}
	part_to_flash_desc(pe, &fd);
	default_ftfs = ftfs_init(&sb, &fd);	/* flash_drv_open is done in it */
	if (default_ftfs == NULL && pe1 && pe2) {
		/* try the passive one */
		mc_log_notice("ftfs corrupteded? switching to the passive partition\n");
		pe = pe == pe1 ? pe2 : pe1;
		part_to_flash_desc(pe, &fd);
		default_ftfs = ftfs_init(&sb, &fd);
		if (default_ftfs != NULL)
			part_set_active_partition(pe);
	}
#endif
	if (default_ftfs == NULL) {
		mc_log_error("ftfs initialization failed\n");
		errno = ENXIO;
		return -1;
	}
	return 0;
}

void
mc_file_fin()
{
#if WMSDK_VERSION < 2040000
	if (fl_dev != NULL) {
		(void)flash_drv_close(fl_dev);
		fl_dev = NULL;
	}
#endif
	default_ftfs = NULL;
	if (checked_free_blk != NULL) {
		mc_free(checked_free_blk);
		checked_free_blk = NULL;
	}
}

MC_FILE *
mc_fopen(const char *fullPath, const char *mode)
{
	MC_FILE *fp;

	errno = 0;
	if (default_ftfs == NULL) {
		mc_log_error("mc_fopen: default_ftfs not initialized\n");
		errno = ENXIO;
		return NULL;
	}
	if ((fp = mc_malloc(sizeof(MC_FILE))) == NULL) {
		errno = ENOMEM;
		mc_log_error("mc_fopen: mc_malloc failed\n");
		return NULL;
	}
	fp->ftfs = NULL;
	fp->ffs = NULL;
	fp->bp = fp->bufend = fp->buf;
	fp->length = 0;
	fp->pos = 0;
#if !mxMC
	fp->fd = NULL;
#endif
	if ((mode[0] == 'r' && mode[1] != '+') && (fullPath[0] != '/' || strncmp(fullPath, FTFS_PATH, sizeof(FTFS_PATH)-1) == 0)) {
		const char *p = fullPath[0] == '/' ? fullPath + sizeof(FTFS_PATH) - 1 : fullPath;
		if ((fp->ftfs = ft_fopen(default_ftfs, p, mode)) != NULL) {
			/* get the size of the file */
			if (ft_fseek(fp->ftfs, 0L, SEEK_END) == WM_SUCCESS) {
				fp->length = ft_ftell(fp->ftfs);
				ft_fseek(fp->ftfs, 0L, SEEK_SET);
			}
		}
		else
			errno = ENOENT;
	}
	else {
		uint32_t flags = 0;
		if (strlen(mode) > 1) {
			switch (mode[1]) {
			case '+':
				flags |= FFS_ORDWR;
				break;
			case 'p':
				flags |= FFS_OPASSIVE;
				break;
			case 'a':
				flags |= FFS_OACTIVE;
				break;
			}
		}
		if (mode[0] == 'w')
			flags |= FFS_OTRUNC | FFS_OCREAT;
		if ((fp->ffs = ffs_open(fullPath, flags)) != NULL)
			fp->length = (*fp->ffs->position_f)(fp->ffs, 1L << 30, mode[0] == 'a');
		else {
			/* errno must've been set */
			if (errno != ENOENT)
				mc_log_error("mc_fopen: ffs_open failed: %d\n", errno);
		}
#if !mxMC
		if ((errno == 0) && (fp->ffs != NULL) && (flags & (FFS_OCREAT | FFS_ORDWR)) && sync_with_native_path(fullPath)) {
			// If the files in the flash drive will changed,
			// changes will also be written on the native file,
			// so that xsr can load .xsb from the native file.
			fp->fd = ft_fopen(NULL, fullPath, mode);
			
			// @@ if fullPath has sub-directory, ft_open will fail. Just ignore this.
			errno = 0;
		}
#endif
	}
	if (errno != 0) {
		if (errno != ENOENT)
			mc_log_error("mc_fopen: errno:%d\n", errno);
		mc_free(fp);
		fp = NULL;
	}
	return fp;
}

int
mc_fclose(MC_FILE *fp)
{
	if (fp->ftfs != NULL)
		(void)ft_fclose(fp->ftfs);
	else if (fp->ffs != NULL)
		(void)ffs_close(fp->ffs);
#if !mxMC
	if (fp->fd != NULL)
		(void)ft_fclose(fp->fd);
#endif
	mc_free(fp);
	return 0;
}

size_t
mc_fread(void *ptr, size_t size, size_t nmemb, MC_FILE *fp)
{
	size_t n = size * nmemb;
	uint8_t *p = ptr;

	if (fp->bp < fp->bufend) {
		size_t blen = fp->bufend - fp->bp;
		if (blen > n)
			blen = n;
		memcpy(p, fp->bp, blen);
		p += blen;
		n -= blen;
		fp->bp += blen;
	}
	if (n != 0) {
		if (fp->ftfs != NULL) {
			n = ft_fread(p, 1, n, fp->ftfs);	/* get around the exteremly inefficient implementation... */
			if (n == (size_t)-1)
				errno = EIO;
			else
				p += n;
		}
		else if (fp->ffs != NULL) {
			n = (*fp->ffs->read_f)(p, 1, n, fp->ffs);
			if (n == 0)
				return 0;
			p += n;
		}
		fp->pos += n;
	}
	n = p - (uint8_t *)ptr;
	return n / size;
}

size_t
mc_fwrite(const void *ptr, size_t size, size_t nmemb, MC_FILE *fp)
{
	if (fp->ffs == NULL) {
		mc_log_error("mc_fwrite: fp->ffs is NULL\n");
		errno = EPERM;
		return 0;
	}

	size_t n = (*fp->ffs->write_f)(ptr, size, nmemb, fp->ffs);
	if (n == 0)
		return 0;
	fp->pos += n * size;
	if (fp->pos > (long)fp->length)
		fp->length = fp->pos;
#if !mxMC
	if (fp->fd != NULL)
		fwrite(ptr, size, nmemb, fp->fd);
#endif
	return n;
}

int
mc_fseek(MC_FILE *fp, long offset, int whence)
{
	if (fp->ftfs != NULL) {
		if (ft_fseek(fp->ftfs, offset, whence) != WM_SUCCESS) {
			mc_log_error("mc_fseek: ft_fseek failed\n");
			errno = EINVAL;
			return -1;
		}
		fp->pos = ft_ftell(fp->ftfs);
		fp->bp = fp->bufend = fp->buf;
		return 0;
	}
	else if (fp->ffs != NULL) {
		long pos;
		switch (whence) {
		case SEEK_SET:
			break;
		case SEEK_CUR:
			if ((pos = (*fp->ffs->position_f)(fp->ffs, -1L, 0)) < 0L)
				return -1;
			offset += pos;
			break;
		case SEEK_END:
			if ((pos = (*fp->ffs->position_f)(fp->ffs, 1L << 30, 0)) < 0L)
				return -1;
			offset += pos;
			break;
		}
		offset = (*fp->ffs->position_f)(fp->ffs, offset, 1);
		if (offset >= 0L) {
			fp->pos = offset;
			fp->bp = fp->bufend = fp->buf;
		}
#if !mxMC
		if (fp->fd != NULL)
			ft_fseek(fp->fd, offset, whence);
#endif
		return offset < 0L ? -1 : 0;
	}
	else {
		mc_log_error("mc_fseek: no file description\n");
		errno = ENXIO;
		return -1;
	}
}

int
mc_ftruncate(MC_FILE *fp, long length)
{
	if (fp->ffs == NULL) {
		mc_log_error("mc_ftruncate: fp->ffs is NULL\n");
		errno = ENXIO;
		return -1;
	}
#if !mxMC
	if (fp->fd != NULL)
		ft_ftruncate(fp->fd, length);
#endif
	if ((*fp->ffs->position_f)(fp->ffs, length, 1) >= 0 && ffs_truncate(fp->ffs) == 0) {
		fp->pos = length;
		fp->length = length;
		fp->bp = fp->bufend = fp->buf;
		return 0;
	}
	else
		return -1;
}

int
mc_fillbuf(MC_FILE *fp)
{
	size_t nread;

	if ((nread = mc_fread(fp->buf, 1, MC_STREAM_BUFSIZ, fp)) != 0) {
		fp->bp = fp->buf;
		fp->bufend = fp->bp + nread;
		return *fp->bp++;
	}
	else
		return EOF;
}

char *
mc_fgets(char *str, int size, MC_FILE *fp)
{
	int c;
	char *p = str;

	while (--size >= 0) {
		if ((c = mc_getc(fp)) == EOF)
			break;
		*p++ = c;
		if (c == '\n')
			break;
	}
	if (p == str)
		return NULL;
	if (size > 0)
		*p = '\0';
	else
		*--p = '\0';
	return str;
}

int
mc_fgetc(MC_FILE *fp)
{
	return mc_getc(fp);
}

int
mc_fputs(const char *str, MC_FILE *fp)
{
	int len = strlen(str), n;

	n = (int)mc_fwrite(str, 1, (size_t)len, fp);
	return n == 0 ? EOF : n;
}

long
mc_ftell(MC_FILE *fp)
{
	return fp->pos - (fp->bufend - fp->bp);
}

int
mc_fflush(MC_FILE *fp)
{
	int err = 0;
#if mxMC
	if (fp->ffs != NULL)
		err = ffs_flush(fp->ffs);
#endif
	return err;
}

int
mc_creat(const char *fullPath, mode_t mode)
{
#if !mxMC
	if (sync_with_native_path(fullPath))
		ft_creat(fullPath, mode);
#endif
	ffs_t *ffs = ffs_open(fullPath, FFS_OCREAT);
	if (ffs != NULL) {
		ffs_close(ffs);
		return 0;	/* no fd */
	}
	else
		return -1;
}

int
mc_unlink(const char *path)
{
#if !mxMC
	if (sync_with_native_path(path))
		ft_unlink(path);
#endif
	return ffs_delete(path);
}

int
mc_rename(const char *from, const char *to)
{
	return ffs_rename(from, to);
}

int
mc_get_volume_info(const char *volume, struct mc_volume_info *info)
{
#if WMSDK_VERSION >= 2040000
	struct partition_entry *pe;

	if ((strlen(volume) > 0) && (volume[0] == '/'))
		volume++;

	pe = ffs_get_active_partition_by_name(volume);
	if (pe == NULL)
		return -1;
	info->size = pe->size;
	return 0;
#else
	return -1;
#endif
}

int
mc_erase_volume(const char *volname)
{
#if WMSDK_VERSION >= 2040000
	struct partition_entry *pe;
	flash_desc_t fd;
	mdev_t *mdev;

	if ((strlen(volname) > 0) && (volname[0] == '/'))
		volname++;

	pe = part_get_passive_partition_by_name(volname);
	if (pe == NULL) {
		if ((pe = part_get_layout_by_name(volname, NULL)) == NULL) {
			mc_log_error("mc_erase_volume: no partition! %s\n", volname);
			return -1;
		}
	}
	if ((mdev = ffs_flash_open(pe, &fd)) != NULL) {
		(void)flash_drv_erase(mdev, fd.fl_start, fd.fl_size);
		ffs_flash_close(mdev);
		return 0;
	}
	return -1;
#else
	return -1;
#endif
}

int
mc_check_volume(const char *volname, int recovery)
{
#if WMSDK_VERSION >= 2040000
	struct partition_entry *pe;
	flash_desc_t fd;
	mdev_t *mdev = NULL;
	bool *used = NULL;
	char **filename = NULL;
	int filenameLen = 0;
	size_t nBlock = 0;
	int i;
	int err = 0;

	if ((strlen(volname) > 0) && (volname[0] == '/'))
		volname++;

	pe = ffs_get_active_partition_by_name(volname);
	if (pe == NULL) {
		mc_log_error("mc_check_volume: partition not found: %s\n", volname);
		err = -1;
		goto bail;
	}

	switch (pe->type) {
	case FC_COMP_PSM:
	case FC_COMP_USER_APP:
	{
		size_t blk, offset, nFree;
		ffs_header_t h;

		if ((mdev = ffs_flash_open(pe, &fd)) == NULL) {
			mc_log_error("mc_check_volume: ffs_flash_open failed\n");
			err = -1;
			goto bail;
		}

		if ((fd.fl_size % FFS_SECTOR_SIZE) != 0) {
			mc_log_error("mc_check_volume: fd.fl_size is invalid: %d\n", fd.fl_size);
			err = -1;
			goto bail;
		}

		nBlock = fd.fl_size / FFS_SECTOR_SIZE;

		if ((used = mc_calloc(nBlock, sizeof(bool))) == NULL) {
			mc_log_error("mc_check_volume: mc_calloc failed\n");
			err = -1;
			goto bail;
		}
		if ((filename = mc_calloc(nBlock, sizeof(char *))) == NULL) {
			mc_log_error("mc_check_volume: mc_calloc failed\n");
			err = -1;
			goto bail;
		}

		for (blk = 0; blk < nBlock; blk++) {
			if (used[blk]) continue;

			offset = blk * FFS_SECTOR_SIZE;
			if (flash_drv_read(mdev, (uint8_t *)&h, sizeof(h), fd.fl_start + offset) != WM_SUCCESS) {
				mc_log_error("mc_check_volume: flash_drv_read failed\n");
				err = -1;
				goto bail;
			}

			if (h.magic == FFS_MAGIC && h.name[0] != '\0') {
				int nextBlk = h.nextblk;
				size_t nextOffset;
				ffs_header_t nextH;

				for (i = 0; i < filenameLen; i++) {
					if (strcmp(h.name, filename[i]) == 0) {
						if (recovery) {
							used[blk] = true;
							flash_drv_erase(mdev, fd.fl_start + offset, FFS_SECTOR_SIZE);
							while (nextBlk >= 0) {
								nextOffset = nextBlk * FFS_SECTOR_SIZE;
								if (flash_drv_read(mdev, (uint8_t *)&nextH, sizeof(nextH), fd.fl_start + nextOffset) != WM_SUCCESS) {
									mc_log_error("mc_check_volume: flash_drv_read failed\n");
									err = -1;
									goto bail;
								}
								used[nextBlk] = true;
								nextBlk = nextH.nextblk;
								flash_drv_erase(mdev, fd.fl_start + nextOffset, FFS_SECTOR_SIZE);
							}
						} else {
							err = -1;
							goto bail;
						}
					}
				}

				used[blk] = true;
				filename[filenameLen++] = mc_strdup(h.name);

				while (nextBlk >= 0) {
					nextOffset = nextBlk * FFS_SECTOR_SIZE;

					if (flash_drv_read(mdev, (uint8_t *)&nextH, sizeof(nextH), fd.fl_start + nextOffset) != WM_SUCCESS) {
						mc_log_error("mc_check_volume: flash_drv_read failed\n");
						err = -1;
						goto bail;
					}

					if (nextH.magic == FFS_MAGIC) {
						if (nextH.name[0] != '\0') {
							mc_log_error("%s: invalid name at %d\n", volname, (int)blk);
							if (recovery) {
								bzero(nextH.name, sizeof(nextH.name));
								flash_drv_write(mdev, (uint8_t *)&nextH, sizeof(nextH), fd.fl_start + nextOffset);
								err++;
							}
							else {
								err = -1;
								goto bail;
							}
						}
						used[nextBlk] = true;
					}
					else {
						mc_log_error("%s: invalid magic in sub block at %d\n", volname, (int)blk);
						if (recovery) {
							flash_drv_erase(mdev, fd.fl_start + nextOffset, FFS_SECTOR_SIZE);
							/* h should still be alive */
							h.nextblk = -1;
							flash_drv_write(mdev, (uint8_t *)&h, sizeof(h), fd.fl_start + offset);
							mc_log_error("truncated\n");
							err++;
						}
						else {
							err = -1;
							goto bail;
						}

					}

					nextBlk = nextH.nextblk;
				}
			}
		}

		for (blk = nFree = 0; blk < nBlock; blk++) {
			if (used[blk]) continue;

			offset = blk * FFS_SECTOR_SIZE;
			if (flash_drv_read(mdev, (uint8_t *)&h, sizeof(h), fd.fl_start + offset) != WM_SUCCESS) {
				mc_log_error("mc_check_volume: flash_drv_read failed\n");
				err = -1;
				goto bail;
			}

			if (h.magic == FFS_MAGIC) {
				mc_log_error("%s: invalid magic at %d\n", volname, (int)blk);
				if (recovery) {
					flash_drv_erase(mdev, fd.fl_start + offset, FFS_SECTOR_SIZE);
					mc_log_error("erased the sector\n");
					err++;
				}
				else {
					err = -1;
					goto bail;
				}

			}
			nFree++;
		}

		mc_log_notice("%s: %d free blocks\n", volname, (int)nFree);
	}

	default:
		break;
	}

bail:
	if (mdev != NULL) {
		ffs_flash_close(mdev);
	}
	if (used != NULL) {
		mc_free(used);
	}
	if (filename != NULL) {
		for (i = 0; i < (int)nBlock; i++)
			if (filename[i])
				mc_free(filename[i]);
		mc_free(filename);
	}

	return err;
#else
	return 0;
#endif
}

/* directory */
MC_DIR *
mc_opendir(const char *name)
{
	struct partition_entry *pe;
	flash_desc_t fd;
	mdev_t *mdev;
	MC_DIR *dir;

	if ((strlen(name) > 0) && name[0] == '/')
		name++;

	if ((pe = ffs_get_active_partition_by_name(name)) == NULL) {
		mc_log_error("mc_opendir: %s not found\n", name);
		errno = ENXIO;
		return NULL;
	}
	if ((mdev = ffs_flash_open(pe, &fd)) == NULL) {
		mc_log_error("mc_opendir: ffs_flash_open failed\n");
		errno = ENXIO;
		return NULL;
	}
	if ((dir = mc_malloc(sizeof(struct mc_dir))) == NULL) {
		mc_log_error("mc_opendir: mc_malloc failed\n");
		ffs_flash_close(mdev);
		errno = ENOMEM;
		return NULL;
	}
	dir->mdev = mdev;
	dir->fd = fd;
	dir->block = 0;
	return dir;
}

struct mc_dirent *
mc_readdir(MC_DIR *dir)
{
	int blk;
	uint32_t offset;
	ffs_header_t *h;
	static struct mc_dirent dirent;
	uint8_t data[FFS_SECTOR_SIZE];

	mc_check_stack();

	for (blk = dir->block; (offset = blk * FFS_SECTOR_SIZE) + FFS_SECTOR_SIZE <= dir->fd.fl_size; blk++) {
		if (flash_drv_read(dir->mdev, data, sizeof(data), dir->fd.fl_start + offset) != WM_SUCCESS) {
			mc_log_error("mc_readdir: flash_drv_read failed\n");
			errno = EIO;
			return NULL;
		}
		h = (ffs_header_t *)data;
		if (h->magic == FFS_MAGIC && h->name[0] != '\0') {
#if MC_LONG_PATH
			strncpy(dirent.d_name, h->name, sizeof(dirent.d_name));
#else
			strncpy(dirent.d_name, h->name, sizeof(h->name));
			dirent.d_name[sizeof(dirent.d_name) - 1] = '\0';
#endif
			dir->block = blk + 1;
			return &dirent;
		}
	}
	dir->block = blk;
	return NULL;
}

int
mc_closedir(MC_DIR *dir)
{
	ffs_flash_close(dir->mdev);
	mc_free(dir);
	return 0;
}

static const char mc_vol_1[] = "k0";
static const char mc_vol_2[] = "k1";
static const char mc_vol_3[] = "k2";
static const char mc_vol_4[] = "k3";
static const char *mc_volumes[] = {
	mc_vol_1, mc_vol_2, mc_vol_3, mc_vol_4,
};
#define MC_NUM_VOLUMES	(sizeof(mc_volumes) / sizeof(mc_volumes[0]))

static const struct {
	const char *property;
	const char *partition;
} mc_special_directories[] = {
	{"applicationDirectory", mc_vol_1},
	{"preferencesDirectory", mc_vol_2},
	{"documentsDirectory", mc_vol_4},
	{"picturesDirectory", mc_vol_4},
	{"temporaryDirectory", mc_vol_4},
	{"variableDirectory", mc_vol_3},
};
#define MC_NUM_SPECIAL_DIRECTORIES	(sizeof(mc_special_directories) / sizeof(mc_special_directories[0]))

const char *
mc_get_special_dir(const char *name)
{
	unsigned int i;

#if !mxMC
	if (strcmp(name, "nativeApplicationDirectory") == 0) {
		return mc_resolve_path(mc_vol_1, 1);
	}
#endif

	for (i = 0; i < MC_NUM_SPECIAL_DIRECTORIES; i++) {
		if (strcmp(mc_special_directories[i].property, name) == 0) {
			const char *partition = mc_special_directories[i].partition;
#if !USE_NATIVE_STDIO
			static char path[PATH_MAX];
			snprintf(path, sizeof(path), "/%s", partition);
			return path;
#else
			return mc_resolve_path(partition, 1);
#endif
		}
	}
	return NULL;
}

const char *
mc_get_volume(unsigned int i)
{
	return i < MC_NUM_VOLUMES ? mc_volumes[i] : NULL;
}

void
mc_set_active_volume(const char *path)
{
	struct partition_entry *pe1, *pe2, *active;
	short idx = 0;

	if (path[0] == '/')
		path++;
	pe1 = part_get_layout_by_name(path, &idx);
	pe2 = part_get_layout_by_name(path, &idx);
	if (pe1 && pe2) {
		active = part_get_active_partition(pe1, pe2);
		part_set_active_partition(active == pe1 ? pe2 : pe1);
	}
}

#if MC_LONG_PATH
int mc_update_path_name()
{
	const char *volume;
	MC_DIR *dir;
	int blk;
	uint32_t offset;
	ffs_header_t h;
	int nextBlk;
	size_t nextOffset;
	ffs_header_t nextH;
	size_t i;

	if (mc_file_init() != 0) {
		return -1;
	}

	for (i = 0; (volume = mc_get_volume(i)) != NULL; i++) {
		if ((dir = mc_opendir(volume)) == NULL) continue;

		for (blk = dir->block; (offset = blk * FFS_SECTOR_SIZE) + FFS_SECTOR_SIZE <= dir->fd.fl_size; blk++) {
			if (flash_drv_read(dir->mdev, (uint8_t *)&h, sizeof(h), dir->fd.fl_start + offset) != WM_SUCCESS) {
				mc_closedir(dir);
				return -1;
			}

			if ((h.magic != FFS_MAGIC) || (strnlen(h.name, FT_MAX_FILENAME) < FT_MAX_FILENAME)) continue;

			// just remove this file now.

			flash_drv_erase(dir->mdev, dir->fd.fl_start + offset, FFS_SECTOR_SIZE);

			nextBlk = h.nextblk;

			while (nextBlk >= 0) {
				nextOffset = nextBlk * FFS_SECTOR_SIZE;

				if (flash_drv_read(dir->mdev, (uint8_t *)&nextH, sizeof(nextH), dir->fd.fl_start + nextOffset) != WM_SUCCESS) {
					mc_closedir(dir);
					return -1;
				}

				flash_drv_erase(dir->mdev, dir->fd.fl_start + nextOffset, FFS_SECTOR_SIZE);

				nextBlk = nextH.nextblk;
			}
		}

		mc_closedir(dir);
	}

	return 0;
}
#endif

#endif /* !USE_NATIVE_STDIO */

#if !mxMC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include "mc_file.h"

const char *
mc_resolve_path(const char *path, int create)
{
	static char resolv[PATH_MAX];
	static char *home = NULL;
	char *p;
	char tmp[PATH_MAX];

	if (home == NULL) {
		if ((home = getenv("HOME")) != NULL) {
			snprintf(tmp, sizeof(tmp), "%s/tmp", home);
			(void)mkdir(tmp, 0777);
			snprintf(resolv, sizeof(resolv), "%s/mc", tmp);
			(void)mkdir(resolv, 0777);
		}
		else
			return path;
	}
	if ((p = realpath(path, resolv)) != NULL)
		return p;
	while (*path == '/')
		path++;
	if (create) {
		if ((p = strchr(path, '/')) != NULL) {
			strncpy(tmp, path, p - path);
			tmp[p - path] = '\0';
			snprintf(resolv, sizeof(resolv), "%s/tmp/mc/%s", home, tmp);
			mkdir(resolv, 0777);
			snprintf(resolv, sizeof(resolv), "%s/tmp/mc/%s", home, path);
		}
		else {
			snprintf(resolv, sizeof(resolv), "%s/tmp/mc/%s", home, path);
			mkdir(resolv, 0777);
		}
	}
	else
		snprintf(resolv, sizeof(resolv), "%s/tmp/mc/%s", home, path);
	return resolv;
}
#endif	/* !mxMC */
