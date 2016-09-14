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
#include "mc_ffs.h"

#define FFS_MAGIC	0xab	/* anything but 0 or ff */
#define FFS_SECTOR_SIZE	(4*1024)

#define FFS_MAX_FILENAME	24

typedef struct {
	uint8_t magic;
	int8_t nextblk;
	uint16_t size;
	char name[FFS_MAX_FILENAME];
} ffs_header_t;

typedef struct mc_ffs {
	void *mdev;
	mc_partition_entry_t pe;
	uint8_t flags;
	int dirty;
	int blk, startblk;
	int bp;
#if MC_LONG_PATH
	int startbp;
#endif
	size_t (*read_f)(void *, size_t, size_t, struct mc_ffs *);
	size_t (*write_f)(const void *, size_t, size_t, struct mc_ffs *);
	long (*position_f)(struct mc_ffs *, long position, int setp);
	enum {FFS_FILE, FFS_VOLUME} kind;
	union {
		ffs_header_t h;
		uint8_t data[FFS_SECTOR_SIZE];
	} u;
} mc_ffs_t;

#define FFS_HEADER_SIZE		sizeof(ffs_header_t)
#define FFS_CONTENT_SIZE	(FFS_SECTOR_SIZE - FFS_HEADER_SIZE)
#if MC_LONG_PATH
#define FFS_PATH_MAX		(FFS_SECTOR_SIZE - FFS_HEADER_SIZE + FFS_MAX_FILENAME)
#endif

struct mc_dir {
	void *mdev;
	int block;
	mc_partition_entry_t pe;
};

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
ffs_find_free_block(mc_ffs_t *ffs)
{
	size_t blk, offset;
	ffs_header_t h;

	for (blk = 0; (offset = blk * FFS_SECTOR_SIZE) + FFS_SECTOR_SIZE <= ffs->pe.pe_size; blk++) {
		if (ffs_free_block_is_checked(blk)) continue;

		if (mc_flash_drv_read(ffs->mdev, (uint8_t *)&h, sizeof(h), ffs->pe.pe_start + offset) != 0) {
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
ffs_clear_block(mc_ffs_t *ffs, size_t offset)
{
#if ZERO_CLEAR_ERACE
	ffs_header_t h;
#endif

	offset += ffs->pe.pe_start;
	if (mc_flash_drv_erase(ffs->mdev, offset, FFS_SECTOR_SIZE) != 0) {
		mc_log_error("ffs_clear_block: flash_drv_erase failed\n");
		return -1;
	}

#if ZERO_CLEAR_ERACE	// flash_drv_erase will turn all bits of sector into 1
	bzero(&h, sizeof(h));
	if (mc_flash_drv_write(ffs->mdev, (uint8_t *)&h, sizeof(h), offset) != 0) {
		mc_log_error("ffs_clear_block: flash_drv_write failed\n");
		return -1;
	}
#endif

	return 0;
}

static int
ffs_write_block(mc_ffs_t *ffs, size_t offset)
{
	offset += ffs->pe.pe_start;

	ffs_uncheck_free_block(ffs->blk);

#if 0 && !ZERO_CLEAR_ERACE
	if (!ffs_free_block_is_checked(ffs->blk))	// block must be erased if it was free block.
#endif
	(void)mc_flash_drv_erase(ffs->mdev, offset, FFS_SECTOR_SIZE);

	if (mc_flash_drv_write(ffs->mdev, ffs->u.data, FFS_SECTOR_SIZE, offset) != 0) {
		mc_log_error("ffs_write_block: flash_drv_write failed\n");
		errno = EIO;
		return -1;
	}
	return 0;
}

int
mc_ffs_flush(mc_ffs_t *ffs)
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

int
mc_ffs_truncate(mc_ffs_t *ffs)
{
	int blk = ffs->u.h.nextblk;
	ffs_header_t h;

	while (blk >= 0) {
		int offset = blk * FFS_SECTOR_SIZE;
		if (mc_flash_drv_read(ffs->mdev, (uint8_t *)&h, sizeof(h), ffs->pe.pe_start + offset) != 0) {
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
ffs_position_file(mc_ffs_t *ffs, long position, int setp)
{
	int blk = ffs->startblk, lastblk = blk;
	long pos = 0;
	int bp, hsize;
	ffs_header_t *hp, h;
	size_t sz;

	if (setp) {
		mc_ffs_flush(ffs);
		hp = &ffs->u.h;
		sz = FFS_SECTOR_SIZE;
	}
	else {
		hp = &h;
		sz = sizeof(h);
	}
	while (blk >= 0) {
		if (mc_flash_drv_read(ffs->mdev, (uint8_t *)hp, sz, ffs->pe.pe_start + blk * FFS_SECTOR_SIZE) != 0) {
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
ffs_position_volume(mc_ffs_t *ffs, long position, int setp)
{
	if (position < 0)
		return ffs->blk * FFS_SECTOR_SIZE + ffs->bp;	/* current position */
	else {
		if (position > (long)ffs->pe.pe_size)
			position = ffs->pe.pe_size;
		if (setp) {
			mc_ffs_flush(ffs);
			ffs->blk = position / FFS_SECTOR_SIZE;
			ffs->bp = position % FFS_SECTOR_SIZE;
			/* read in the new current sector */
			if (mc_flash_drv_read(ffs->mdev, ffs->u.data, FFS_SECTOR_SIZE, ffs->pe.pe_start + ffs->blk * FFS_SECTOR_SIZE) != 0) {
				mc_log_error("ffs_position_volume: flash_drv_read failed\n");
				errno = EIO;
				return -1L;
			}
		}
		return position;
	}
}

static size_t
ffs_read_file(void *buffer, size_t size, size_t nelm, mc_ffs_t *ffs)
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
			if (mc_flash_drv_read(ffs->mdev, ffs->u.data, FFS_SECTOR_SIZE, ffs->pe.pe_start + ffs->u.h.nextblk * FFS_SECTOR_SIZE) != 0) {
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
ffs_read_volume(void *buffer, size_t size, size_t nelm, mc_ffs_t *ffs)
{
	uint8_t *bp = buffer;
	long nbytes = (long)(size * nelm);
	long n;

	errno = 0;
	while (nbytes > 0) {
		if (ffs->bp == 0) {
			if ((size_t)ffs->blk * FFS_SECTOR_SIZE >= ffs->pe.pe_size)
				break;
			if (mc_flash_drv_read(ffs->mdev, ffs->u.data, FFS_SECTOR_SIZE, ffs->pe.pe_start + ffs->blk * FFS_SECTOR_SIZE) != 0) {
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
ffs_write_file(const void *buffer, size_t size, size_t nelm, mc_ffs_t *ffs)
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
ffs_write_volume(const void *buffer, size_t size, size_t nelm, mc_ffs_t *ffs)
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
			if ((unsigned)(ffs->blk + 1) * FFS_SECTOR_SIZE + FFS_SECTOR_SIZE > ffs->pe.pe_size) {
				mc_log_error("ffs_write_volume: disk full! %d > %d\n", (ffs->blk + 1) * FFS_SECTOR_SIZE, ffs->pe.pe_size);
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
ffs_open_file(mc_ffs_t *ffs, const char *fname)
{
	int blk;
	uint32_t offset;
	int targetblk = -1;
	int first_freeblk = -1;

	/* seek to the file position */
	for (blk = 0; (offset = blk * FFS_SECTOR_SIZE) + FFS_SECTOR_SIZE <= ffs->pe.pe_size; blk++) {
		if (ffs_free_block_is_checked(blk)) continue;

		if (mc_flash_drv_read(ffs->mdev, ffs->u.data, FFS_SECTOR_SIZE, ffs->pe.pe_start + offset) != 0) {
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
		else if (strncmp(fname, ffs->u.h.name, FFS_MAX_FILENAME) == 0) {
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
		if (nameLen >= FFS_MAX_FILENAME) {
			ffs->startbp = ffs->bp = nameLen - FFS_MAX_FILENAME + 1;
		}
#endif
		if (ffs->flags & FFS_OTRUNC)
			mc_ffs_truncate(ffs);
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
			if (nameLen >= FFS_MAX_FILENAME) {
				ffs->startbp = ffs->bp = nameLen - FFS_MAX_FILENAME + 1;
				memcpy(ffs->u.h.name, fname, nameLen);
				ffs->u.h.name[nameLen] = '\0';
			} else {
				strncpy(ffs->u.h.name, fname, FFS_MAX_FILENAME);
			}
#else
			strncpy(ffs->u.h.name, fname, FFS_MAX_FILENAME);
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
ffs_open_volume(mc_ffs_t *ffs, const char *partition)
{
	/* do not need to erase here...
	if (ffs->flags & FFS_OTRUNC)
		(void)mc_flash_drv_erase(ffs->mdev, ffs->pe.pe_start, ffs->pe.pe_size);
	*/
	ffs->read_f = ffs_read_volume;
	ffs->write_f = ffs_write_volume;
	ffs->position_f = ffs_position_volume;
	return 0;
}

int
mc_ffs_init()
{
	return 0;
}

void
mc_ffs_fin()
{
	if (checked_free_blk != NULL) {
		mc_free(checked_free_blk);
		checked_free_blk = NULL;
	}
	checked_free_blk_size = 0;
}

mc_ffs_t *
mc_ffs_open(const char *path, uint32_t flags)
{
#if WMSDK_VERSION < 2040000
	/* can't get the flash layout by id */
	errno = ENXIO;
	return NULL;
#else
	mc_ffs_t *ffs;
	char *fullpath = NULL, *p, *partition, *fname;

	if ((ffs = mc_malloc(sizeof(mc_ffs_t))) == NULL) {
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
	if (mc_get_partition_by_name(partition, &ffs->pe, (flags & FFS_OACTIVE) != 0) != 0) {
		if (mc_get_partition_by_name(partition, &ffs->pe, -1) != 0) {
			mc_log_error("ffs_open: partition not found: %s\n", partition);
			errno = ENXIO;
			goto bail;
		}
	}
	if ((ffs->mdev = mc_flash_drv_open(ffs->pe.pe_dev)) == NULL) {
		mc_log_error("ffs_open: mc_flash_drv_open failed\n");
		errno = ENXIO;
		goto bail;
	}
	if (ffs->pe.pe_type == MC_PTYPE_USER_APP && fname != NULL) {
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
				mc_flash_drv_close(ffs->mdev);
				ffs->mdev = NULL;
			}
			mc_free(ffs);
			ffs = NULL;
		}
	}
	return ffs;
#endif
}

int
mc_ffs_close(mc_ffs_t *ffs)
{
	int err;

	err = mc_ffs_flush(ffs);
	mc_flash_drv_close(ffs->mdev);
	mc_free(ffs);
	return err;
}

int
mc_ffs_delete(const char *path)
{
	mc_ffs_t *ffs;
	size_t offset;
	int err = 0;

	if ((ffs = mc_ffs_open(path, FFS_OTRUNC)) == NULL) {
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
	(void)mc_ffs_close(ffs);
	return err;
}

int
mc_ffs_rename(const char *from, const char *to)
{
	mc_ffs_t *ffs;
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

	if ((ffs = mc_ffs_open(from, 0)) == NULL)
		return -1;	/* probably NOENT? */
#if MC_LONG_PATH
	// @@ currentry, renaming from/to long path (except length is same) is not supported.
	fromLen = strlen(ffs->u.h.name);
	toLen = strlen(to);
	if (fromLen != toLen) {
		if ((fromLen >= FFS_MAX_FILENAME) || (toLen >= FFS_MAX_FILENAME)) {
			return -1;
		}
	}
	if (toLen >= FFS_MAX_FILENAME) {
		memcpy(ffs->u.h.name, to, toLen);
		ffs->u.h.name[toLen] = '\0';
	} else {
		strncpy(ffs->u.h.name, to, FFS_MAX_FILENAME);
	}

	for (blk = 0; (offset = blk * FFS_SECTOR_SIZE) + FFS_SECTOR_SIZE <= ffs->pe.pe_size; blk++) {
		if (mc_flash_drv_read(ffs->mdev, data, FFS_SECTOR_SIZE, ffs->pe.pe_start + offset) != 0) {
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
	strncpy(ffs->u.h.name, to, FFS_MAX_FILENAME);
#endif
	err = ffs_write_block(ffs, ffs->blk * FFS_SECTOR_SIZE);
	ffs->dirty = 0;
	(void)mc_ffs_close(ffs);
	return err;
}

long
mc_ffs_position(mc_ffs_t *ffs, long position, int setp)
{
	return (*ffs->position_f)(ffs, position, setp);
}

size_t
mc_ffs_read(void *buffer, size_t size, size_t nelm, mc_ffs_t *ffs)
{
	return (*ffs->read_f)(buffer, size, nelm, ffs);
}

size_t
mc_ffs_write(const void *buffer, size_t size, size_t nelm, mc_ffs_t *ffs)
{
	return (*ffs->write_f)(buffer, size, nelm, ffs);
}

MC_DIR *
mc_ffs_opendir(const char *name)
{
	MC_DIR *dir;

	if ((strlen(name) > 0) && name[0] == '/')
		name++;

	if ((dir = mc_malloc(sizeof(struct mc_dir))) == NULL) {
		mc_log_error("mc_opendir: mc_malloc failed\n");
		errno = ENOMEM;
		return NULL;
	}
	if (mc_get_partition_by_name(name, &dir->pe, 1) != 0) {
		mc_log_error("mc_opendir: %s not found\n", name);
		mc_free(dir);
		errno = ENXIO;
		return NULL;
	}
	if ((dir->mdev = mc_flash_drv_open(dir->pe.pe_dev)) == NULL) {
		mc_log_error("mc_opendir: mc_flash_drv_open failed\n");
		mc_free(dir);
		errno = ENXIO;
		return NULL;
	}
	dir->block = 0;
	return dir;
}

struct mc_dirent *
mc_ffs_readdir(MC_DIR *dir)
{
	int blk;
	uint32_t offset;
	ffs_header_t *h;
	uint8_t data[sizeof(h) - FFS_MAX_FILENAME + PATH_MAX];
	static struct mc_dirent dirent;

	mc_check_stack();

	for (blk = dir->block; (offset = blk * FFS_SECTOR_SIZE) + FFS_SECTOR_SIZE <= dir->pe.pe_size; blk++) {
		if (mc_flash_drv_read(dir->mdev, data, sizeof(data), dir->pe.pe_start + offset) != 0) {
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
mc_ffs_closedir(MC_DIR *dir)
{
	mc_flash_drv_close(dir->mdev);
	mc_free(dir);
	return 0;
}


int
mc_ffs_check(mc_partition_entry_t *pe, int recovery, const char *volname)
{
	void *mdev;
	bool *used = NULL;
	char **filename = NULL;
	int filenameLen = 0;
	size_t nBlock = 0;
	int i;
	int err = 0;
	size_t blk, offset, nFree;
	ffs_header_t h;

	if ((mdev = mc_flash_drv_open(pe->pe_dev)) == NULL) {
		mc_log_error("mc_check_volume: mc_flash_drv_open failed\n");
		err = -1;
		goto bail;
	}

	if ((pe->pe_size % FFS_SECTOR_SIZE) != 0) {
		mc_log_error("mc_check_volume: invalid partition size %d\n", pe->pe_size);
		err = -1;
		goto bail;
	}

	nBlock = pe->pe_size / FFS_SECTOR_SIZE;

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
		if (mc_flash_drv_read(mdev, (uint8_t *)&h, sizeof(h), pe->pe_start + offset) != 0) {
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
						mc_flash_drv_erase(mdev, pe->pe_start + offset, FFS_SECTOR_SIZE);
						while (nextBlk >= 0) {
							nextOffset = nextBlk * FFS_SECTOR_SIZE;
							if (mc_flash_drv_read(mdev, (uint8_t *)&nextH, sizeof(nextH), pe->pe_start + nextOffset) != 0) {
								mc_log_error("mc_check_volume: flash_drv_read failed\n");
								err = -1;
								goto bail;
							}
							used[nextBlk] = true;
							nextBlk = nextH.nextblk;
							mc_flash_drv_erase(mdev, pe->pe_start + nextOffset, FFS_SECTOR_SIZE);
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

				if (mc_flash_drv_read(mdev, (uint8_t *)&nextH, sizeof(nextH), pe->pe_start + nextOffset) != 0) {
					mc_log_error("mc_check_volume: flash_drv_read failed\n");
					err = -1;
					goto bail;
				}

				if (nextH.magic == FFS_MAGIC) {
					if (nextH.name[0] != '\0') {
						mc_log_error("%s: invalid name at %d\n", volname, (int)blk);
						if (recovery) {
							bzero(nextH.name, sizeof(nextH.name));
							mc_flash_drv_write(mdev, (uint8_t *)&nextH, sizeof(nextH), pe->pe_start + nextOffset);
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
						mc_flash_drv_erase(mdev, pe->pe_start + nextOffset, FFS_SECTOR_SIZE);
						/* h should still be alive */
						h.nextblk = -1;
						mc_flash_drv_write(mdev, (uint8_t *)&h, sizeof(h), pe->pe_start + offset);
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
		if (mc_flash_drv_read(mdev, (uint8_t *)&h, sizeof(h), pe->pe_start + offset) != 0) {
			mc_log_error("mc_check_volume: flash_drv_read failed\n");
			err = -1;
			goto bail;
		}

		if (h.magic == FFS_MAGIC) {
			mc_log_error("%s: invalid magic at %d\n", volname, (int)blk);
			if (recovery) {
				mc_flash_drv_erase(mdev, pe->pe_start + offset, FFS_SECTOR_SIZE);
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

bail:
	if (mdev != NULL) {
		mc_flash_drv_close(mdev);
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

		for (blk = dir->block; (offset = blk * FFS_SECTOR_SIZE) + FFS_SECTOR_SIZE <= dir->pe.pe_size; blk++) {
			if (mc_flash_drv_read(dir->mdev, (uint8_t *)&h, sizeof(h), dir->pe.pe_start + offset) != 0) {
				mc_closedir(dir);
				return -1;
			}

			if ((h.magic != FFS_MAGIC) || (strnlen(h.name, FFS_MAX_FILENAME) < FFS_MAX_FILENAME)) continue;

			// just remove this file now.

			mc_flash_drv_erase(dir->mdev, dir->pe.pe_start + offset, FFS_SECTOR_SIZE);

			nextBlk = h.nextblk;

			while (nextBlk >= 0) {
				nextOffset = nextBlk * FFS_SECTOR_SIZE;

				if (mc_flash_drv_read(dir->mdev, (uint8_t *)&nextH, sizeof(nextH), dir->pe.pe_start + nextOffset) != 0) {
					mc_closedir(dir);
					return -1;
				}

				mc_flash_drv_erase(dir->mdev, dir->pe.pe_start + nextOffset, FFS_SECTOR_SIZE);

				nextBlk = nextH.nextblk;
			}
		}

		mc_closedir(dir);
	}

	return 0;
}
#endif
