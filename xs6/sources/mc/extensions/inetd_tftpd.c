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
#include "mc_tftpd.h"
#include "mc_module.h"

#if !XS_ARCHIVE
#include "inetd_tftpd.xs.c"
MC_MOD_DECL(tftpd);
#endif

void
xs_tftpd_constructor(xsMachine *the)
{
}

void
xs_tftpd_connect(xsMachine *the)
{
	int s;
	void *instance;

	xsVars(1);
	xsGet(xsVar(0), xsArg(0), xsID("nativeSocket"));
	s = xsToInteger(xsVar(0));
	instance = tftpd_connect(s, the, &xsThis);
	if (instance == NULL) {
		xsSetBoolean(xsResult, 0);
		return;
	}
	xsSetHostData(xsThis, instance);
	xsSetBoolean(xsResult, 1);
}

void
xs_tftpd_destructor(void *data)
{
	if (data != NULL)
		tftpd_close(data);
}

void
xs_tftpd_close(xsMachine *the)
{
	tftpd_close(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}
