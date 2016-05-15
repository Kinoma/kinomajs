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
#include "mc_file.h"
#include "mc_module.h"

#if !XS_ARCHIVE
#include "xm_files.xs.c"
MC_MOD_DECL(files);
#endif

void
xs_files_delete(xsMachine *the)
{
	int result = mc_unlink(xsToString(xsArg(0)));
	xsSetBoolean(xsResult, result == 0);
}

void
xs_files_deleteVolume(xsMachine *the)
{
	int result = mc_erase_volume(xsToString(xsArg(0)));
	xsSetBoolean(xsResult, result == 0);
}

void
xs_files_rename(xsMachine *the)
{
	char *from;
	int result;

	if ((from = mc_strdup(xsToString(xsArg(0)))) == NULL)
		return;
	result = mc_rename(from, xsToString(xsArg(1)));
	mc_free(from);
	xsSetBoolean(xsResult, result == 0);
}

void
xs_files_fsck(xsMachine *the)
{
	xsSetInteger(xsResult, mc_check_volume(xsToString(xsArg(0)), xsToInteger(xsArgc) > 1 && xsTest(xsArg(1))));
}

void
xs_files_getFileInfo(xsMachine *the)
{
	MC_FILE *fp;

	/* only "size" in the info */
	if ((fp = mc_fopen(xsToString(xsArg(0)), "r")) == NULL)
		return;
	xsSetNewInstanceOf(xsResult, xsObjectPrototype);
	xsVars(1);
	xsSetInteger(xsVar(0), mc_fsize(fp));
	xsSet(xsResult, xsID("size"), xsVar(0));
	mc_fclose(fp);
}

void
xs_files_getVolumeInfo(xsMachine *the)
{
	char *volume = xsToString(xsArg(0));
	struct mc_volume_info info;

	xsVars(1);
	if (mc_get_volume_info(volume, &info) == 0) {
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsSetBoolean(xsVar(0), false);
		xsNewHostProperty(xsResult, xsID("removable"), xsVar(0), xsDefault, xsDefault);
		xsSetInteger(xsVar(0), info.size);
		xsNewHostProperty(xsResult, xsID("size"), xsVar(0), xsDefault, xsDefault);
	}
	/* else return undefined */
}

void
xs_files_iterator_constructor(xsMachine *the)
{
	MC_DIR *dir;

	if ((dir = mc_opendir(xsToString(xsArg(0)))) == NULL)
		mc_xs_throw(the, "opendir");
	xsSetHostData(xsThis, dir);
}

void
xs_files_iterator_getNext(xsMachine *the)
{
	MC_DIR *dir = xsGetHostData(xsThis);
	struct mc_dirent *dirent;

	xsVars(1);
	if ((dirent = mc_readdir(dir)) != NULL) {
		xsSetNewInstanceOf(xsResult, xsObjectPrototype);
		xsSetString(xsVar(0), dirent->d_name);
		xsSet(xsResult, xsID("name"), xsVar(0));
	}
}

void
xs_files_iterator_close(xsMachine *the)
{
	MC_DIR *dir = xsGetHostData(xsThis);

	if (dir != NULL)
		mc_closedir(dir);
	xsSetHostData(xsThis, NULL);
}

void
xs_files_iterator_destructor(void *data)
{
	MC_DIR *dir = data;

	if (dir != NULL)
		mc_closedir(dir);
}

void
xs_volume_iterator_constructor(xsMachine *the)
{
	xsSetHostData(xsThis, (void *)0);
}

void
xs_volume_iterator_getNext(xsMachine *the)
{
	size_t i = (size_t)xsGetHostData(xsThis);
	const char *volume;
	char directory[PATH_MAX];

	mc_check_stack();

	xsVars(1);
	if ((volume = mc_get_volume(i)) != NULL) {
		xsSetNewInstanceOf(xsResult, xsObjectPrototype);
		xsSetString(xsVar(0), (char *)volume);
		xsSet(xsResult, xsID("name"), xsVar(0));
#if mxMC || !USE_NATIVE_STDIO
		directory[0] = '/';
		strcpy(&directory[1], volume);
		xsSetString(xsVar(0), directory);
#else
		xsSetString(xsVar(0), (char *)mc_resolve_path(volume, 1));
#endif
		xsSet(xsResult, xsID("path"), xsVar(0));
		xsSetHostData(xsThis, (void *)(i + 1));
	}
}

void
xs_volume_iterator_destructor(void *data)
{
	/* nop */
}

void
xs_files_getSpecialDirectory(xsMachine *the)
{
	char *name = xsToString(xsArg(0));
	const char *path = mc_get_special_dir(name);

	if (path != NULL)
		xsSetString(xsResult, (char *)path);
}

void
xs_files_init(xsMachine *the)
{
	static int initialized = 0;

	if (!initialized) {
		mc_file_init();
		initialized++;
	}
}

void
xs_files_setActive(xsMachine *the)
{
	char *path = xsToString(xsArg(0));

	mc_set_active_volume(path);
}

void
xs_files_updatePathName(xsMachine *the)
{
#if MC_LONG_PATH
	mc_update_path_name();
#endif
}

void
xs_files_map_destructor(void *data)
{
	/* nothing to do */
}

void
xs_files_map_constructor(xsMachine *the)
{
	size_t sz;
	const void *data;

	if ((data = mc_mmap(xsToString(xsArg(0)), &sz)) == NULL)
		mc_xs_throw(the, "mmap");
	xsSetHostData(xsThis, (void *)data);
	xsSetInteger(xsResult, sz);
	xsSet(xsThis, xsID("byteLength"), xsResult);
}
