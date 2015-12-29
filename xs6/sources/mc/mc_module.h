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
#include "mc_xs.h"

struct mc_module {
	const void ***stubs;
	int *num_stubs;
	void (*build)(xsMachine *);
	void (*delete)(xsMachine *);
	struct mc_xref {
		void *fp;
		int fnum;
	} *xrefs;
	int num_xrefs;
};

enum {
	/* put xref function numbers here */
	MC_XREF_DUMMY,
	MC_NUM_XREFS,
};

struct mc_global_record {
	void *xref_stubs;
	int *_errno;
};

#ifdef MC_MODULE
extern const void *ext_stubs[];
extern int num_ext_stubs;
extern void *xref_stubs[];
#define mc_global	((struct mc_global_record *)(ext_stubs[0]))
#undef errno
#define errno		(*(mc_global->_errno))

#define MC_MOD_DECL(name)	\
	static void xsHostModule(xsMachine *the);	\
	struct mc_module _mc_ ## name ## _module = { \
		(const void ***)&ext_stubs, \
		&num_ext_stubs, \
		xsHostModule, \
		NULL, \
		NULL, \
		0, \
	}

#else	/* !MC_MODULE */

#define MC_MOD_DECL(name)

#endif	/* MC_MODULE */
