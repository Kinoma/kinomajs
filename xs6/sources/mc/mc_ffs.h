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
#ifndef __MC_FFS_H__
#define __MC_FFS_H__

#include "mc_stdio.h"
#include "mc_wmsdk.h"
#include "mc_file.h"

#define FFS_OCREAT	0x1
#define FFS_OTRUNC	0x2
#define FFS_ORDWR	0x4
#define FFS_OPASSIVE	0x8
#define FFS_OACTIVE	0x10

struct mc_ffs;
typedef struct mc_ffs mc_ffs_t;

extern int mc_ffs_init();
extern void mc_ffs_fin();
extern int mc_ffs_flush(mc_ffs_t *ffs);
extern int mc_ffs_truncate(mc_ffs_t *ffs);
extern long mc_ffs_position(mc_ffs_t *ffs, long position, int setp);
extern mc_ffs_t *mc_ffs_open(const char *path, uint32_t flags);
extern size_t mc_ffs_read(void *buffer, size_t size, size_t nelm, mc_ffs_t *ffs);
extern size_t mc_ffs_write(const void *buffer, size_t size, size_t nelm, mc_ffs_t *ffs);
extern int mc_ffs_close(mc_ffs_t *ffs);
extern int mc_ffs_delete(const char *path);
extern int mc_ffs_rename(const char *from, const char *to);

extern MC_DIR *mc_ffs_opendir(const char *name);
extern struct mc_dirent *mc_ffs_readdir(MC_DIR *dir);
extern int mc_ffs_closedir(MC_DIR *dir);

extern int mc_ffs_check(mc_partition_entry_t *pe, int recovery, const char *volname);

#endif /* __MC_FFS_H__ */
