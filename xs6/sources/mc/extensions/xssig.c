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

#include "mc_xs.h"
#include "mc_misc.h"

extern void *xsReadArchive(xsStringValue base);
extern void xsFreeArchive(void *archive);
extern xsBooleanValue xsGetCode(xsMachine *the, void *archive, char *path, void **addr, xsIntegerValue *size);

void
xs_archive_destructor(void *data)
{
	if (data != NULL)
		xsFreeArchive(data);
}

void
xs_archive_constructor(xsMachine *the)
{
	char *base = xsToInteger(xsArgc) > 0 ? xsToString(xsArg(0)) : NULL;
	void *archive;

	if (base != NULL) {
		if ((archive = xsReadArchive(base)) == NULL)
			mc_xs_throw(the, "archive: failed to read");
	}
	else
		archive = NULL;
	xsSetHostData(xsThis, archive);
}

void
xs_archive_getCode(xsMachine *the)
{
	void *archive = xsGetHostData(xsThis);
	char *path = xsToString(xsArg(0));
	void *addr;
	xsIntegerValue size;

	if (xsGetCode(the, archive, path, &addr, &size))
		xsResult = xsArrayBuffer(addr, size);
}
