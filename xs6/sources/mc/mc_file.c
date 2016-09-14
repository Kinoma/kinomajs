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
#include "mc_file.h"
#include "mc_mapped_files.h"
#if FTFS
#include "mc_ftfs.h"
#endif
#if !mxMC
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#define NUM_MAPPED_FILES	(sizeof(mc_mapped_files) / sizeof(mc_mapped_files[0]))

#if !mxMC
static int
sync_with_native_path(const char *path)
{
	mc_partition_entry_t pe;
	char name[MC_PARTITION_NAME_SIZE + 1], *p;

	strncpy(name, path + 1, MC_PARTITION_NAME_SIZE);
	name[MC_PARTITION_NAME_SIZE] = '\0';
	if ((p = strchr(name, '/')) != NULL)
		*p = '\0';
	return mc_get_partition_by_name(name, &pe, -1) == 0 && pe.pe_type == MC_PTYPE_USER_APP;
}
#endif

int
mc_file_init()
{
	(void)mc_flash_drv_init();
	return mc_ffs_init();
}

void
mc_file_fin()
{
	mc_ffs_fin();
}

const void *
mc_mmap(const char *path, size_t *sizep)
{
	unsigned int i;

	for (i = 0; i < NUM_MAPPED_FILES; i++) {
		if (strcmp(mc_mapped_files[i].name, path) == 0) {
			*sizep = mc_mapped_files[i].size;
			return mc_mapped_files[i].data;
		}
	}
	return NULL;
}

MC_FILE *
mc_fopen(const char *fullpath, const char *mode)
{
	MC_FILE *fp;

	errno = 0;
	if ((fp = mc_malloc(sizeof(MC_FILE))) == NULL) {
		errno = ENOMEM;
		mc_log_error("mc_fopen: mc_malloc failed\n");
		return NULL;
	}
	fp->ffs = NULL;
	fp->mmapped = NULL;
	fp->buf = NULL;
	fp->bp = fp->bufend = fp->buf;
	fp->length = 0;
	fp->pos = 0;
#if FTFS || !mxMC
	fp->aux = NULL;
#endif
	if ((mode[0] == 'r' && mode[1] != '+') && fullpath[0] != '/') {
		size_t sz;
		const void *p = mc_mmap(fullpath, &sz);
		if (p != NULL) {
			fp->mmapped = p;
			fp->length = sz;
			fp->bp = fp->buf = (uint8_t *)p;
			fp->bufend = fp->buf + sz;
			fp->pos = (long)sz;
		}
		else {
#if FTFS
			if ((fp->aux = mc_ft_fopen(fullpath, mode)) != NULL) {
				fp->length = mc_ft_fseek(fp->aux, 0L, SEEK_END);
				mc_ft_fseek(fp->aux, 0L, SEEK_SET);
			}
			else
				errno = ENOENT;
#else
			errno = ENOENT;
#endif
		}
	}
	else {
		uint32_t flags = 0;
#if !mxMC
		if (sync_with_native_path(fullpath)) {
			const char *path = mc_resolve_path(fullpath, *mode == 'r' ? 0 : 1);
			fp->aux = fopen(path, mode);
			errno = 0;
		}
#endif
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
		if ((fp->ffs = mc_ffs_open(fullpath, flags)) != NULL)
			fp->length = mc_ffs_position(fp->ffs, 1L << 30, mode[0] == 'a');
		else {
			/* errno must've been set */
			if (errno != ENOENT)
				mc_log_error("mc_fopen: ffs_open failed: %d\n", errno);
		}
	}
	if (errno == 0) {
		if (fp->mmapped == NULL) {
			if ((fp->buf = mc_malloc(MC_STREAM_BUFSIZ)) != NULL)
				fp->bufend = fp->bp = fp->buf;
			else {
				mc_log_error("mc_fopen: mc_malloc failed\n");
				errno = ENOMEM;
			}
		}
	}
	if (errno != 0) {
		if (fp != NULL)
			mc_fclose(fp);
		fp = NULL;
	}
	return fp;
}

int
mc_fclose(MC_FILE *fp)
{
	if (fp->mmapped == NULL && fp->buf != NULL)
		mc_free(fp->buf);
	if (fp->ffs != NULL)
		(void)mc_ffs_close(fp->ffs);
#if FTFS
	if (fp->aux != NULL)
		(void)mc_ft_fclose(fp->aux);
#endif
#if !mxMC
	if (fp->aux != NULL)
		fclose(fp->aux);
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
		if (fp->ffs != NULL)
			n = mc_ffs_read(p, 1, n, fp->ffs);
#if FTFS
		else if (fp->aux != NULL)
			n = mc_ft_fread(p, 1, n, fp->aux);
#endif
#if !mxMC
		else if (fp->aux != NULL)
			n = fread(p, 1, n, fp->aux);
#endif
		else
			n = 0;
		if (n == 0)
			return 0;
		p += n;
		fp->pos += n;
	}
	n = p - (uint8_t *)ptr;
	return n / size;
}

size_t
mc_fwrite(const void *ptr, size_t size, size_t nmemb, MC_FILE *fp)
{
	size_t n;

	if (fp->ffs == NULL) {
		mc_log_error("mc_fwrite: fp->ffs is NULL\n");
		errno = EPERM;
		return 0;
	}
#if !mxMC
	if (fp->aux != NULL)
		fwrite(ptr, size, nmemb, fp->aux);
#endif
	n = mc_ffs_write(ptr, size, nmemb, fp->ffs);
	if (n == 0)
		return 0;
	fp->pos += n * size;
	if (fp->pos > (long)fp->length)
		fp->length = fp->pos;
	return n;
}

int
mc_fseek(MC_FILE *fp, long offset, int whence)
{
#if !mxMC
	if (fp->aux != NULL)
		fseek(fp->aux, offset, whence);
#endif
	if (fp->ffs != NULL) {
		long pos;
		switch (whence) {
		case SEEK_SET:
			break;
		case SEEK_CUR:
			if ((pos = mc_ffs_position(fp->ffs, -1L, 0)) < 0L)
				return -1;
			offset += pos;
			break;
		case SEEK_END:
			if ((pos = mc_ffs_position(fp->ffs, 1L << 30, 0)) < 0L)
				return -1;
			offset += pos;
			break;
		}
		offset = mc_ffs_position(fp->ffs, offset, 1);
		if (offset >= 0L) {
			fp->pos = offset;
			fp->bp = fp->bufend = fp->buf;
		}
		return offset < 0L ? -1 : 0;
	}
	else if (fp->mmapped != NULL) {
		uint8_t *p;
		switch (whence) {
		default:
		case SEEK_SET:
			p = fp->buf + offset;
			break;
		case SEEK_CUR:
			p = fp->bp + offset;
			break;
		case SEEK_END:
			p = fp->bufend + offset;
			break;
		}
		if (p < fp->buf || p > fp->bufend) {
			mc_log_error("mc_fseek: range error\n");
			errno = ENXIO;
			return -1;
		}
		fp->bp = p;
		return 0;
	}
#if FTFS
	else if (fp->aux != NULL) {
		return mc_ft_fseek(fp->aux, offset, whence);
	}
#endif
	return 0;
}

int
mc_ftruncate(MC_FILE *fp, long length)
{
#if !mxMC
	if (fp->aux != NULL)
		ftruncate(fileno(fp->aux), length);
#endif
	if (fp->ffs == NULL) {
		mc_log_error("mc_ftruncate: fp->ffs is NULL\n");
		errno = ENXIO;
		return -1;
	}
	if (mc_ffs_position(fp->ffs, length, 1) >= 0 && mc_ffs_truncate(fp->ffs) == 0) {
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
	if (fp->ffs != NULL)
		err = mc_ffs_flush(fp->ffs);
	return err;
}

int
mc_creat(const char *fullpath, mode_t mode)
{
#if !mxMC
	if (sync_with_native_path(fullpath))
		creat(mc_resolve_path(fullpath, 1), mode);
#endif
	mc_ffs_t *ffs = mc_ffs_open(fullpath, FFS_OCREAT);
	if (ffs != NULL) {
		mc_ffs_close(ffs);
		return 0;	/* no fd */
	}
	else
		return -1;
}

int
mc_unlink(const char *path)
{
#if !mxMC
	if (sync_with_native_path(path)) {
		if (unlink(mc_resolve_path(path, 0)) == 0)
			mc_resolve_path(path, -1);
	}
#endif
	return mc_ffs_delete(path);
}

int
mc_rename(const char *from, const char *to)
{
	return mc_ffs_rename(from, to);
}

int
mc_get_volume_info(const char *volume, struct mc_volume_info *info)
{
	mc_partition_entry_t pe;

	if ((strlen(volume) > 0) && (volume[0] == '/'))
		volume++;

	if (mc_get_partition_by_name(volume, &pe, -1) != 0)
		return -1;
	info->size = pe.pe_size;
	return 0;
}

int
mc_erase_volume(const char *volname)
{
	mc_partition_entry_t pe;
	void *mdev;
	int ret;

	if ((strlen(volname) > 0) && (volname[0] == '/'))
		volname++;

	if (mc_get_partition_by_name(volname, &pe, 0) != 0) {
		if (mc_get_partition_by_name(volname, &pe, -1) != 0) {
			mc_log_error("mc_erase_volume: no partition! %s\n", volname);
			return -1;
		}
	}
	if ((mdev = mc_flash_drv_open(pe.pe_dev)) == NULL) {
		mc_log_error("mc_erase_volume: mc_flash_drv_open failed\n");
		return -1;
	}
	ret = mc_flash_drv_erase(mdev, pe.pe_start, pe.pe_size);
	mc_flash_drv_close(mdev);
	return ret;
}

int
mc_check_volume(const char *volname, int recovery)
{
	mc_partition_entry_t pe;
	int err;

	if ((strlen(volname) > 0) && (volname[0] == '/'))
		volname++;
	if (mc_get_partition_by_name(volname, &pe, 1) != 0) {
		mc_log_error("mc_check_volume: partition not found: %s\n", volname);
		return -1;
	}
	switch (pe.pe_type) {
	case MC_PTYPE_USER_APP:
		err = mc_ffs_check(&pe, recovery, volname);
		break;
	default:
		err = 0;
		break;
	}
	return err;
}

/* directory */
MC_DIR *
mc_opendir(const char *name)
{
	return mc_ffs_opendir(name);
}

struct mc_dirent *
mc_readdir(MC_DIR *dir)
{
	return mc_ffs_readdir(dir);
}

int
mc_closedir(MC_DIR *dir)
{
	return mc_ffs_closedir(dir);
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
	if (path[0] == '/')
		path++;
	mc_set_active_partition(path);
}

#if !mxMC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>

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
	if (create > 0) {	// create
		char *prev = (char *)path;
		bool volumeName = true;
		while ((p = strchr(prev, '/')) != NULL) {
			if (p - path >= sizeof(tmp))
				break;
			strncpy(tmp, path, p - path);
			tmp[p - path] = '\0';
			snprintf(resolv, sizeof(resolv), "%s/tmp/mc/%s", home, tmp);
			mkdir(resolv, 0777);
			prev = p + 1;
			volumeName = false;
		}
		snprintf(resolv, sizeof(resolv), "%s/tmp/mc/%s", home, path);
		if (volumeName) {
			mkdir(resolv, 0777);
		}
	}
	else if (create < 0) {	// delete empty sub directories
		char *filePath;
		strncpy(tmp, path, PATH_MAX);
		if ((filePath = strchr(tmp, '/')) != NULL) {	// exclude volume name
			filePath++;
			while ((p = strrchr(filePath, '/')) != NULL) {
				*p = '\0';
				snprintf(resolv, sizeof(resolv), "%s/tmp/mc/%s", home, tmp);
				rmdir(resolv);
			}
		}
	}
	else	// read
		snprintf(resolv, sizeof(resolv), "%s/tmp/mc/%s", home, path);
	return resolv;
}
#endif	/* !mxMC */
