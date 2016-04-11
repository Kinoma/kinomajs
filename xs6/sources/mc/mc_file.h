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
#ifndef __MC_FILE_H__
#define __MC_FILE_H__

#include "mc_stdio.h"

struct mc_volume_info {
	size_t size;
	size_t used;	/* not supported yet */
};

#if !mxMC
typedef FILE file;
#endif

struct ffs;
struct mc_file {
	file *ftfs;
	struct ffs *ffs;
	/* for stream read */
#define MC_STREAM_BUFSIZ	128
	uint8_t buf[MC_STREAM_BUFSIZ];
	uint8_t *bp, *bufend;
	size_t length;
	long pos;
#if !mxMC
	file *fd;	// for native file
#endif
};

struct mc_dirent {
	char d_name[PATH_MAX];
};

#if mxMC
typedef uint32_t mode_t;
#else
typedef struct mdev {
	uint8_t index;
	void *fd;
#if !MANAGE_OPEN_COUNT
	uint8_t openCount;
#endif
	uint8_t *data;
	bool dirty;
} mdev_t;

typedef struct flash_desc {
	uint8_t fl_dev;
	uint32_t fl_start;
	uint32_t fl_size;
} flash_desc_t;

enum {
	WM_SUCCESS = 0,
	WM_FAIL = -1,
};

struct fs {
};

struct ftfs_super {
	struct fs fs;
//	FT_FILE fds[FTFS_MAX_FILE_DESCRIPTORS];
//	uint32_t fds_mask;
//	uint32_t active_addr;
//	mdev_t *dev;
//	unsigned fs_crc32;
};

enum flash_comp {
	FC_COMP_BOOT2 = 0,
	FC_COMP_FW,
	FC_COMP_WLAN_FW,
	FC_COMP_FTFS,
	FC_COMP_PSM,
	FC_COMP_USER_APP,
};

struct partition_entry {
	uint8_t type;
	uint8_t device;
	char name[MAX_NAME];
	uint32_t start;
	uint32_t size;
	uint32_t gen_level;
};

extern const char *mc_resolve_path(const char *path, int create);
#endif

extern int mc_file_init();
extern void mc_file_fin();
extern MC_FILE *mc_fopen(const char *path, const char *mode);
extern int mc_fclose(MC_FILE *fp);
extern size_t mc_fread(void *ptr, size_t size, size_t nmemb, MC_FILE *fp);
extern size_t mc_fwrite(const void *ptr, size_t size, size_t nmemb, MC_FILE *fp);
extern int mc_fseek(MC_FILE *fp, long offset, int whence);
extern long mc_ftell(MC_FILE *fp);
#define mc_fsize(fp)	(fp->length)
extern int mc_ftruncate(MC_FILE *fp, long length);
extern int mc_fillbuf(MC_FILE *fp);
#define mc_getc(fp)	(fp->bp < fp->bufend ? *fp->bp++ : mc_fillbuf(fp))
extern int mc_fgetc(MC_FILE *fp);
extern char *mc_fgets(char *str, int size, MC_FILE *fp);
extern int mc_fputs(const char *str, MC_FILE *fp);
extern int mc_fflush(MC_FILE *fp);
extern int mc_creat(const char *path, mode_t mode);
extern int mc_unlink(const char *path);
extern int mc_rename(const char *from, const char *to);
extern int mc_get_volume_info(const char *volname, struct mc_volume_info *);
extern int mc_erase_volume(const char *volname);
extern int mc_check_volume(const char *volname, int recovery);
/* directory */
extern MC_DIR *mc_opendir(const char *name);
extern struct mc_dirent *mc_readdir(MC_DIR *dir);
extern int mc_closedir(MC_DIR *dir);

extern const char *mc_get_special_dir(const char *name);
extern const char *mc_get_volume(unsigned int i);
extern void mc_set_active_volume(const char *path);

#if MC_LONG_PATH
extern int mc_update_path_name();
#endif

#if USE_NATIVE_STDIO

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

extern const char *mc_resolve_path(const char *path, int create);
#define MC_FILE	FILE
#define	MC_DIR	DIR
#define mc_file_init()	(0)
#define mc_file_fin()
#define mc_fopen(p, m)	fopen(p, m)
#define mc_fclose(f)	fclose(f)
#define mc_fread(p, s, n, f)	fread(p, s, n, f)
#define mc_fwrite(p, s, n, f)	fwrite(p, s, n, f)
#define mc_fseek(f, o, w)	fseek(f, o, w)
#undef mc_ftell
#define mc_ftell(f)	ftell(f)
#define mc_creat(p, m)	creat(p, m)
#define mc_unlink(p)	unlink(p)
#define mc_rename(f, t)	rename(f, t)
#define mc_opendir(p)	opendir(p)
#define mc_readdir(d)	readdir(d)
#define mc_closedir(d)	closedir(d)
#define mc_fgetc(f)	fgetc(f)
#define mc_ftruncate(f, len)	ftruncate(fileno(f), len)
#define mc_fflush(f)	fflush(f)
#define mc_dirent	dirent
#undef mc_fsize
static inline size_t mc_fsize(MC_FILE *fp) {
	struct stat stbuf;
	return fstat(fileno(fp), &stbuf) == 0 ? stbuf.st_size : 0;
}
#define mc_get_volume_info(vol, info)	(-1)
#define mc_erase_volume(vol)	(-1)
#define mc_check_volume(vol)	(1)
#define mc_get_special_dir(name)	(NULL)
#define mc_get_volume(i)	(NULL)
#define mc_set_active_volume(path)

#endif	/* USE_NATIVE_STDIO */

#endif /* __MC_FILE_H__ */
