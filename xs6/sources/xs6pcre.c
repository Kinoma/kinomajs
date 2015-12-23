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
#include "xs6Platform.h"

#define LINK_SIZE 2
#define MATCH_LIMIT 10000000
#define MATCH_LIMIT_RECURSION MATCH_LIMIT
#define MAX_NAME_COUNT 10000
#define MAX_NAME_SIZE 32
#define NEWLINE 10
#define PARENS_NEST_LIMIT 250
#define PCREGREP_BUFSIZE 20480
#define PCRE_STATIC 1
#define POSIX_MALLOC_THRESHOLD 10

#define STDC_HEADERS 1
#define SUPPORT_PCRE8 1
#define SUPPORT_UCP 1
#define SUPPORT_UTF 1

#include "pcre_byte_order.c"
#include "pcre_chartables.c"
#include "pcre_compile.c"
#include "pcre_config.c"
#undef NLBLOCK
#undef PSSTART
#undef PSEND
#include "pcre_dfa_exec.c"
#undef NLBLOCK
#undef PSSTART
#undef PSEND
#include "pcre_exec.c"
#include "pcre_fullinfo.c"
#include "pcre_get.c"
#include "pcre_globals.c"
#include "pcre_jit_compile.c"
#include "pcre_maketables.c"
#include "pcre_newline.c"
#include "pcre_ord2utf8.c"
#include "pcre_refcount.c"
#include "pcre_string_utils.c"
#include "pcre_study.c"
#include "pcre_tables.c"
#include "pcre_ucd.c"
#include "pcre_valid_utf8.c"
#include "pcre_version.c"
#include "pcre_xclass.c"

void *pcre_c_malloc(size_t size)
{
	void* p = c_malloc(size);
	//fprintf(stderr, "pcre_c_malloc %ld %p\n", size, p);
	return p;
}

void pcre_c_free(void *p)
{
	//fprintf(stderr, "pcre_c_free %p\n", p);
	c_free(p);
}

void pcre_setup()
{
	pcre_malloc = pcre_c_malloc;
	pcre_free = pcre_c_free;
}
