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
#include <flash.h>
#include <partition.h>
#include <ftfs.h>

void *
mc_ft_fopen(const char *path, const char *mode)
{
	static struct fs *default_ftfs = NULL;

	if (default_ftfs == NULL) {
		struct partition_entry *pe;
		flash_desc_t fd;
		short idx = 0;
		struct partition_entry *pe1 = part_get_layout_by_id(FC_COMP_FTFS, &idx);
		struct partition_entry *pe2 = part_get_layout_by_id(FC_COMP_FTFS, &idx);
		static struct ftfs_super sb;

		if (pe1 && pe2)
			pe = part_get_active_partition(pe1, pe2);
		else if (pe1)
			pe = pe1;
		else if (pe2)
			pe = pe2;
		else {
			mc_log_error("ftfs not found\n");
			errno = ENXIO;
			return NULL;
		}
		part_to_flash_desc(pe, &fd);
		default_ftfs = ftfs_init(&sb, &fd);	/* flash_drv_open is done in it */
	}
	return ft_fopen(default_ftfs, path, mode);
}

void
mc_ft_fclose(void *ftp)
{
	(void)ft_fclose(ftp);
}

size_t
mc_ft_fread(void *data, size_t size, size_t nmemb, void *ftp)
{
	int n = ft_fread(data, size, nmemb, ftp);
	if (n < 0)
		n = 0;
	return (size_t)n;
}

long
mc_ft_fseek(void *ftp, long pos, int whence)
{
	if (ft_fseek(ftp, pos, whence) != WM_SUCCESS)
		return -1;
	return ft_ftell(ftp);
}
