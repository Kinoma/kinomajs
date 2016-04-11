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
#if mxMC

#ifndef __XSPLATFORM__

#include "xs.h"

extern void mc_minit(xsCreation *creation);
extern void mc_mfin();
extern void mc_mstats(int verbose);

#endif	/* __XSPLATFORM__ */

#if MC_MEMORY_DEBUG
#define MC_MEMORY_DEBUG_ARGS , const char *file, uint32_t line, const char *function
#define MC_MEMORY_DEBUG_PARAMS_CONTINUE , file, line, function
#define MC_MEMORY_DEBUG_PARAMS , __FILE__, __LINE__, __FUNCTION__
#else
#define MC_MEMORY_DEBUG_ARGS
#define MC_MEMORY_DEBUG_PARAMS_CONTINUE
#define MC_MEMORY_DEBUG_PARAMS
#endif

extern void *mc_malloc_(size_t size MC_MEMORY_DEBUG_ARGS);
extern void *mc_realloc_(void *ptr, size_t size MC_MEMORY_DEBUG_ARGS);
extern void *mc_calloc_(size_t count, size_t size MC_MEMORY_DEBUG_ARGS);
extern char *mc_strdup_(const char *str MC_MEMORY_DEBUG_ARGS);
extern void mc_free_(void *ptr MC_MEMORY_DEBUG_ARGS);

#define mc_malloc(size)	mc_malloc_(size MC_MEMORY_DEBUG_PARAMS)
#define mc_realloc(ptr, size)	mc_realloc_(ptr, size MC_MEMORY_DEBUG_PARAMS)
#define mc_calloc(count, size)	mc_calloc_(count, size MC_MEMORY_DEBUG_PARAMS)
#define mc_strdup(str)	mc_strdup_(str MC_MEMORY_DEBUG_PARAMS)
#define mc_free(ptr)	mc_free_(ptr  MC_MEMORY_DEBUG_PARAMS)

extern void *mc_xs_chunk_allocator(size_t size);
extern void *mc_xs_slot_allocator(size_t size);
extern void mc_xs_chunk_disposer(void *data);
extern void mc_xs_slot_disposer(void *data);
extern void mc_xs_sbrk(void *p);

#define MC_STACK_SIZE	(8*1024)
extern void mc_check_stack();

#else	/* mxMC */

#define mc_malloc(size)	malloc(size)
#define mc_realloc(ptr, size)	realloc(ptr, size)
#define mc_calloc(count, size)	calloc(count, size)
#define mc_strdup(str)	strdup(str)
#define mc_free(ptr)	free(ptr)
#define mc_check_stack()

#endif	/* mxMC */
