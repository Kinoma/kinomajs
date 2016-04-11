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
#define MC_MEM_VERBOSE	2
//#define MC_MEM_FILL
#if MC_MEM_VERBOSE
#undef MC_LOG_VERBOSE
#define MC_LOG_VERBOSE	3
#endif
#define MC_MEM_NOGROW	0
//#define DEBUG_HEAP_EXTRA	/* defined in heap_4.c */

#include "mc_stdio.h"
#include "mc_ipc.h"
#include "xs.h"
#include "mc_memory.h"
#include <FreeRTOS.h>
#include <wm_os.h>
#include "xs_patch.h"

struct overhead {
	struct overhead *free;
	struct overhead *prev;
	size_t size;
#if portBYTE_ALIGNMENT >= 8
	unsigned short seqnum;
	unsigned short extra;
#endif
};
#define HEAP_ALIGNMENT	sizeof(struct overhead)
#define MC_MEM_SIZE(h)	((h->size & ~0x01) - sizeof(struct overhead))

static unsigned int stack_base;

static int total_memory = 0;
static int seqnum = 0;

extern unsigned _heap_start, _heap_end, _heap2_start, _heap2_end, _heap3_start, _heap3_end;	/* defined in .ld */
static uint8_t *heap_ptr = NULL, *heap_pend = NULL;
static uint8_t *heap2_ptr = NULL, *heap2_pend = NULL;

#define MIN_GROW_SIZE	(4*1024)

#ifndef roundup
#define roundup(x, y)	((((x) + (y) - 1) / (y)) * (y))
#endif

static mc_semaphore_t *mc_memory_sem = NULL;

extern void *get_current_task_stack();

void
mc_minit(xsCreation *allocation)
{
	size_t n;
	void *p;
	static mc_semaphore_t sem;

	stack_base = (unsigned int)get_current_task_stack();

	p = pvPortMalloc(1);	/* initialize the heap just in case no one has called ever malloc so far */
	vPortFree(p);
	heap_ptr = (uint8_t *)&_heap2_start;
	heap_pend = (uint8_t *)&_heap2_end;
	heap2_ptr = (uint8_t *)&_heap3_start;
	heap2_pend = (uint8_t *)&_heap3_end;

	/* assign all the 3rd heap to slots */
	n = heap2_pend - heap2_ptr;
	allocation->initialHeapCount = n / sizeof(xsSlot);

	if (mc_semaphore_create(&sem) == 0) {
		mc_memory_sem = &sem;
		mc_semaphore_post(mc_memory_sem);
	}

#if MC_MEM_VERBOSE > 0
	mc_log_debug("xsAllocation: initial: chunk = %d, heap = %d\n", allocation->initialChunkSize, allocation->initialHeapCount);
#endif
}

void
mc_mfin()
{
	if (mc_memory_sem != NULL) {
		mc_semaphore_delete(mc_memory_sem);
		mc_memory_sem = NULL;
	}
}

void
mc_mstats(int verbose)
{
	const heapAllocatorInfo_t *hip = getheapAllocInfo();
	heapAllocatorInfo_t hi = *hip;
	int selftest = vHeapSelfTest(0);

	if (selftest != 0)
		fprintf(stdout, "!!! system heap broken !!!\n");
	if (verbose) {
		fprintf(stdout, "=== heap info ===\n");
		fprintf(stdout, "malloc: %d free, %d allocations, %d biggest block, %d by Fsk\n", hi.freeSize, hi.totalAllocations, hi.biggestFreeBlockAvailable, total_memory);
		fprintf(stdout, "heap2: 0x%x, 0x%x, %d remains\n", heap_ptr, heap_pend, heap_pend - heap_ptr);
		fprintf(stdout, "heap3: 0x%x, %d remains\n", heap2_ptr, heap2_pend - heap2_ptr);
		fprintf(stdout, "===\n");
	}
	else
		fprintf(stdout, "=== %d free, %d biggest block, %d heap ===\n", hi.freeSize, hi.biggestFreeBlockAvailable, heap_pend - heap_ptr);
}

/*
 * !!! tricky tricky tricky !!!
 */
static void
mc_add_mblock(void *mem, size_t sz)
{
	/* sz must be > overhead * 3 && < the primary heap size */
	unsigned char *p = mem, *endp;
	struct overhead *h;

	/* align to the sizeof overhead */
	endp = (unsigned char *)((((unsigned)p + sz) / HEAP_ALIGNMENT) * HEAP_ALIGNMENT);
	p = (unsigned char *)((((unsigned)p + (HEAP_ALIGNMENT - 1)) / HEAP_ALIGNMENT) * HEAP_ALIGNMENT);
	/* previous block */
	h = (struct overhead *)p;
	h->free = h->prev = NULL;
	h->size = 0x1;	/* ALLOCATED to stop merging */
	/* self */
	h++;
	h->prev = h - 1;
	h->free = NULL;
	h->size = (endp - p) - (sizeof(struct overhead) * 2);	/* including the self header */
	h->size |= 0x1;	/* ALLOCATED */
	p = (unsigned char *)(h + 1);
	/* next block */
	h = (struct overhead *)(endp - sizeof(struct overhead));
	h->prev = h->free = NULL;
	h->size = 0x1;	/* ALLOCATED to stop merging */
	vPortFree(p);
#if MC_MEM_VERBOSE > 0
	mc_log_debug("=== %d reclaimed ===\n", sz);
	mc_mstats(false);
#endif
}

static int
mc_mgrow(size_t size)
{
#if !MC_MEM_NOGROW
	int ret = 0;
	uint8_t *brk = NULL;

	if (mc_memory_sem)
		mc_semaphore_wait(mc_memory_sem);
	size += sizeof(struct overhead) * 3;
	if (size < MIN_GROW_SIZE)
		size = MIN_GROW_SIZE;
	else
		size = roundup(size, HEAP_ALIGNMENT);
#if MC_MEM_VERBOSE > 0
	mc_log_debug("=== grow: %d ===\n", size);
#endif
	if (heap2_ptr + size <= heap2_pend) {
		brk = heap2_ptr;
		heap2_ptr += size;
	}
	else if (heap_pend - size >= heap_ptr) {
		brk = heap_pend - size;
		heap_pend -= size;
	}
	if (brk != NULL) {
		mc_add_mblock(brk, size);
		ret = 1;
	}
	if (mc_memory_sem)
		mc_semaphore_post(mc_memory_sem);
	return ret;
#else
	return 0;
#endif
}

static void
mc_mfail(size_t size MC_MEMORY_DEBUG_ARGS)
{
	errno = ENOMEM;
	mc_log_error("!!! malloc failed: %d !!!\n", size);
#if MC_MEM_VERBOSE > 1 && MC_MEMORY_DEBUG
	mc_log_error("@error at %s, line = %d\n", function, line);
#endif
	mc_mstats(true);
}

static void *
__mc_malloc_(size_t size, int claim MC_MEMORY_DEBUG_ARGS)
{
	void *p;
	struct overhead *h;

	if (size == 0)
		mc_log_error("!!! alloc 0 !!!\n");

	if ((p = pvPortMalloc(size)) == NULL) {
		if (claim) {
#if MC_MEM_VERBOSE > 0
			mc_log_debug("=== malloc failed: %d. claiming morecore ===\n", size);
			mc_mstats(false);
#endif
			if (mc_mgrow(size))
				p = pvPortMalloc(size);
		}
	}
	if (p == NULL)
		return NULL;

	h = (struct overhead *)((char *)p - sizeof(struct overhead));
	total_memory += MC_MEM_SIZE(h);
#ifndef DEBUG_HEAP_EXTRA
	h->seqnum = seqnum++;
	h->extra = MC_MEM_SIZE(h) - size;
#endif
#if MC_MEM_VERBOSE > 1 && MC_MEMORY_DEBUG
	mc_log_debug("@alloc[%d]: %d (%d) ttl=%d; %s, line = %d\n", h->seqnum, MC_MEM_SIZE(h), size, total_memory, function, line);
#endif
	return p;
}

void *
mc_malloc_(size_t size MC_MEMORY_DEBUG_ARGS)
{
	void *p = __mc_malloc_(size, true MC_MEMORY_DEBUG_PARAMS_CONTINUE);
	if (p == NULL)
		mc_mfail(size MC_MEMORY_DEBUG_PARAMS_CONTINUE);
	return p;
}

void
mc_free_(void *ptr MC_MEMORY_DEBUG_ARGS)
{
	struct overhead *h;

	if (ptr == NULL)
		return;

	h = (struct overhead *)((char *)ptr - sizeof(struct overhead));
	total_memory -= MC_MEM_SIZE(h);
#if MC_MEM_VERBOSE > 1 && MC_MEMORY_DEBUG
	mc_log_debug("@free[%d]: %d, ttl=%d; %s, line = %d\n", h->seqnum, MC_MEM_SIZE(h), total_memory, function, line);
#endif
#ifdef MC_MEM_FILL
	memset(ptr, 0xcc, MC_MEM_SIZE(h));
#endif
	vPortFree(ptr);
}

static void *
__mc_realloc_(void *ptr, uint32_t size, int claim MC_MEMORY_DEBUG_ARGS)
{
	struct overhead *h;
	int seq;

	if (size == 0)
		mc_log_error("!!! alloc 0 !!!\n");

	if (ptr != NULL) {
		h = (struct overhead *)((char *)ptr - sizeof(struct overhead));
		total_memory -= MC_MEM_SIZE(h);
#if MC_MEM_VERBOSE > 1 && MC_MEMORY_DEBUG
		mc_log_debug("@realloc[%d]: %d->%d, ttl=%d; %s, line = %d\n", h->seqnum, MC_MEM_SIZE(h), size, total_memory, function, line);
#endif
		seq = h->seqnum;
	}
	else {
		seq = seqnum++;
#if MC_MEM_VERBOSE > 1 && MC_MEMORY_DEBUG
		mc_log_debug("@realloc[%d]: %d, ttl=%d; %s, line = %d\n", seq, size, total_memory, function, line);
#endif
	}

	if ((ptr = pvPortReAlloc(ptr, size)) == NULL) {
		if (claim) {
#if MC_MEM_VERBOSE > 0
			mc_log_debug("=== realloc failed: %d. claiming morecore ===\n", size);
			mc_mstats(false);
#endif
			if (mc_mgrow(size))
				ptr = pvPortReAlloc(ptr, size);
		}
	}
	if (ptr == NULL)
		return NULL;

	h = (struct overhead *)((char *)ptr - sizeof(struct overhead));
	total_memory += MC_MEM_SIZE(h);
#ifndef DEBUG_HEAP_EXTRA
	h->seqnum = seq;
	h->extra = MC_MEM_SIZE(h) - size;
#endif
	return ptr;
}

void *
mc_realloc_(void *ptr, size_t size MC_MEMORY_DEBUG_ARGS)
{
	if ((ptr = __mc_realloc_(ptr, size, true MC_MEMORY_DEBUG_PARAMS_CONTINUE)) == NULL) {
		mc_mfail(size MC_MEMORY_DEBUG_PARAMS_CONTINUE);
		return NULL;
	}
	return ptr;
}

void *
mc_calloc_(size_t count, size_t size MC_MEMORY_DEBUG_ARGS)
{
	void *p;

	size *= count;
	if ((p = __mc_malloc_(size, true MC_MEMORY_DEBUG_PARAMS_CONTINUE)) == NULL) {
		mc_mfail(size MC_MEMORY_DEBUG_PARAMS_CONTINUE);
		return NULL;
	}
	memset(p, 0, size);
	return p;
}

char *
mc_strdup_(const char *str MC_MEMORY_DEBUG_ARGS)
{
	size_t len = strlen(str);
	char *p;

	if ((p = __mc_malloc_(len + 1, true MC_MEMORY_DEBUG_PARAMS_CONTINUE)) == NULL) {
		mc_mfail(len + 1 MC_MEMORY_DEBUG_PARAMS_CONTINUE);
		return NULL;
	}
	strcpy(p, str);
	return p;
}

void *
mc_xs_chunk_allocator(size_t size)
{
	void *ptr = NULL;

	if (heap_ptr + size <= heap_pend) {
		ptr = heap_ptr;
		heap_ptr += size;
#if MC_MEM_VERBOSE > 2
		mc_log_debug("=== allocate xs chunk: %d, heap = %d ===\n", size, heap_pend - heap_ptr);
#endif
	}
	else {
		mc_log_error("!!! xs: failed to allocate chunk %d !!!\n", size);
		mc_mstats(true);
	}
	return ptr;
}

void
mc_xs_chunk_disposer(void *data)
{
	/* @@ too lazy but it should work... */
	if ((uint8_t *)data < heap_ptr)
		heap_ptr = data;
}

void *
mc_xs_slot_allocator(size_t size)
{
	void *ptr = NULL;

	if (heap2_ptr + size <= heap2_pend) {
		ptr = heap2_ptr;
		heap2_ptr += size;
#if MC_MEM_VERBOSE > 2
		mc_log_debug("=== allocate xs slots: %d, heap2 = %d ===\n", size, heap2_pend - heap2_ptr);
#endif
	}
	else if (heap_pend - size >= heap_ptr) {
		ptr = heap_pend - size;
		heap_pend -= size;
#if MC_MEM_VERBOSE > 2
		mc_log_debug("=== allocate xs slots: %d, heap = %d ===\n", size, heap_pend - heap_ptr);
#endif
	}
	else {
		mc_log_error("!!! xs: failed to allocate slots %d !!!\n", size);
		mc_mstats(true);
	}
	return ptr;
}

void
mc_xs_slot_disposer(void *data)
{
	/* nothing to do */
}

void
mc_xs_sbrk(void *p)
{
	heap_ptr = p;
}

void 
mc_check_stack()
{
	void *p;

	if ((unsigned int)&p < stack_base - MC_STACK_SIZE)	/* not accurate */
		mc_fatal("stack overflow\n");
}
