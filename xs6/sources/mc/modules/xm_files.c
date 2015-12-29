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
#include "mc_module.h"

#if !XS_ARCHIVE
#include "xm_files.xs.c"
MC_MOD_DECL(files);
#endif

void
xs_files_constructor(xsMachine *the)
{
	const char *path = xsToString(xsArg(0));
	uint32_t permissions =  xsToInteger(xsArgc) > 1 ? xsToInteger(xsArg(1)) : 0;
	MC_FILE *fp;

	if ((fp = mc_fopen(path, permissions ? "w+" : "r")) == NULL) {
		if (errno == ENOENT && permissions) {
			mc_creat(path, 0666);
			fp = mc_fopen(path, "r+");
		}
	}
	if (fp == NULL)
		mc_xs_throw(the, "Files.constructor");
	xsSetHostData(xsThis, fp);
}

void
xs_files_destructor(void *data)
{
	MC_FILE *fp = data;

	if (fp != NULL)
		mc_fclose(fp);
}

void
xs_files_close(xsMachine *the)
{
	MC_FILE *fp = xsGetHostData(xsThis);

	if (fp != NULL)
		mc_fclose(fp);
	xsSetHostData(xsThis, NULL);
}

void
xs_files_read(xsMachine *the)
{
	MC_FILE *fp = xsGetHostData(xsThis);
	int n = xsToInteger(xsArg(0));
	size_t nread;
	void *buf;

	if (xsToInteger(xsArgc) > 1 && (xsTypeOf(xsArg(1)) == xsReferenceType)) {
		int len = xsGetArrayBufferLength(xsArg(1));
		if (len < n)
			n = len;
		xsResult = xsArg(1);
	}
	else
		xsResult = xsArrayBuffer(NULL, n);
	buf = xsToArrayBuffer(xsResult);
	if ((nread = mc_fread(buf, 1, (size_t)n, fp)) == 0) {
		if (errno != 0)
			mc_xs_throw(the, "files.read");
		/* return EOF */
		xsSetUndefined(xsResult);
	}
	else if (nread != (size_t)n)
		xsSetArrayBufferLength(xsResult, nread);
}

void
xs_files_readChar(xsMachine *the)
{
	MC_FILE *fp = xsGetHostData(xsThis);
	int c;

	if ((c = mc_fgetc(fp)) != EOF)
		xsSetInteger(xsResult, c);
}

int
mc_files_write_element(xsMachine *the, MC_FILE *fp, xsSlot *slot)
{
	size_t datasize;
	void *data;
	uint8_t intdata;
	int n;

	xsVars(1);
	switch (xsTypeOf(*slot)) {
	case xsIntegerType:
		datasize = 1;
		intdata = (uint8_t)xsToInteger(*slot);
		data = &intdata;
		break;
	case xsStringType:
		data = xsToString(*slot);
		datasize = strlen(data);
		break;
	case xsReferenceType:
		if (xsIsInstanceOf(*slot, xsArrayPrototype)) {
			xsGet(xsVar(0), *slot, xsID("length"));
			int len = xsToInteger(xsVar(0)), j;
			for (j = 0, n = 0; j < len; j++) {
				xsGet(xsVar(0), *slot, j);
				if ((n = mc_files_write_element(the, fp, &xsVar(0))) < 0)
					return -1;
			}
			return n;
		}
		else {	/* assume it's an ArrayBuffer */
			datasize = xsGetArrayBufferLength(*slot);
			data = xsToArrayBuffer(*slot);
		}
		break;
	default:
		mc_xs_throw(the, "bad arg");
		return -1;
	}
	if (mc_fwrite(data, 1, datasize, fp) == 0)
		return -1;
	return 0;
}

void
xs_files_write(xsMachine *the)
{
	MC_FILE *fp = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc), i;

	for (i = 0; i < ac; i++) {
		if (mc_files_write_element(the, fp, &xsArg(i)) < 0)
			break;
	}
	xsSetInteger(xsResult, i);
}

void
xs_files_getPosition(xsMachine *the)
{
	MC_FILE *fp = xsGetHostData(xsThis);

	xsSetInteger(xsResult, mc_ftell(fp));
}

void
xs_files_setPosition(xsMachine *the)
{
	MC_FILE *fp = xsGetHostData(xsThis);
	long pos = xsToInteger(xsArg(0));

	if (mc_fseek(fp, pos, SEEK_SET) != 0)
		mc_xs_throw(the, "fseek failed");
}

void
xs_files_getLength(xsMachine *the)
{
	MC_FILE *fp = xsGetHostData(xsThis);

	xsSetInteger(xsResult, mc_fsize(fp));
}

void
xs_files_setLength(xsMachine *the)
{
	MC_FILE *fp = xsGetHostData(xsThis);
	long length = xsToInteger(xsArg(0));

	if (mc_ftruncate(fp, length) != 0)
		mc_xs_throw(the, "ftruncate failed");
}

void
xs_files_delete(xsMachine *the)
{
	int result = mc_unlink(xsToString(xsArg(0)));
	xsSetBoolean(xsResult, result == 0);
}

void
xs_files_deleteDirectory(xsMachine *the)
{
	int result = mc_erase_volume(xsToString(xsArg(0)));
	xsSetBoolean(xsResult, result == 0);
}

void
xs_files_checkDirectory(xsMachine *the)
{
	xsSetInteger(xsResult, mc_check_volume(xsToString(xsArg(0))));
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
