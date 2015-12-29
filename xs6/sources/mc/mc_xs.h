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
#ifndef __MC_XS_H__
#define __MC_XS_H__

#include "xs.h"
#include "xs_patch.h"

typedef struct {
	xsIntegerValue length;
	xsCallback callback;
	int attr;
	const char *id;
} xs_host_t;

extern void mc_xs_build(xsMachine *the, xsSlot *slot, const xs_host_t *host, int n);
extern char *mc_xs_exception_message(xsMachine *the);
extern void mc_xs_throw(xsMachine *the, const char *message);
extern int mc_xs_enableGC(xsMachine *the, int flag);

#endif	/* __MC_XS_H__ */
