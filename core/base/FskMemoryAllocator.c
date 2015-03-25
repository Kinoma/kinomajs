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
#define __FSKMEMORYALLOCATOR_PRIV__

#include "FskMemoryAllocator.h"

#define FB_ALLOCATOR_DEBUG		0
#define FB_ALLOCATOR_MAX_SIZE	(1600 * 1200 * 4)
#define MINIMUM_BLOCK_SIZE		(32)

FskInstrumentedSimpleType(MemoryAllocator, memoryallocator);


FskErr FskMemoryBlockDispose(FskMemoryHeap heap, FskMemoryBlock block);

UInt32 ROUNDUP(FskMemoryHeap heap, UInt32 x) {
	if (!heap)
		return 0;
	return ((x + (heap->alignmentQuantum - 1)) / heap->alignmentQuantum) * heap->alignmentQuantum;
}


#if FB_ALLOCATOR_DEBUG
void pmb(FskMemoryBlock mb) {
	FskMemoryAllocatorPrintfDebug("mb (0x%08x)  next  0x%08x    prev  0x%08x\n", mb, mb->next, mb->prev);
	FskMemoryAllocatorPrintfDebug("                 mem   0x%08x    size  0x%08x\n", mb->mem, mb->size);
	FskMemoryAllocatorPrintfDebug("                 ref   0x%08x    mover 0x%08x\n", mb->ref, mb->moveFn );
}

void pfbmem(FskMemoryHeap heap) {
	FskMemoryBlock	cur;
	int total = 0;
	cur = heap->blockHead;
	while (cur) {
		total += cur->size;
		pmb(cur);
		cur = cur->next;
	}
	if (heap->size != total)
		FskMemoryAllocatorPrintfDebug("ACK!!! HEAP TOTAL DOESN'T MATCH SIZE\n");
}

void dumpMem(FskMemoryHeap heap) {
	FskMemoryBlock mx;
	int total = 0;
	if (!heap) {
		FskMemoryAllocatorPrintfDebug("noHeap\n");
		return;
	}
	mx = heap->blockHead;
	while (mx) {
		total += mx->size;
		FskMemoryAllocatorPrintfDebug("%p %s %c <%p >%p @%p [%d] %p\n", mx, mx->inUse ? "USED " : "AVAIL", mx->locked ? 'L' : 'U', mx->prev, mx->next, mx->mem, mx->size, mx->size);
		if (mx->prev && mx->prev->next != mx)
			FskMemoryAllocatorPrintfDebug(" -- ACK!! previous doesn't point to this element\n");
		if (mx->next && mx->next->prev != mx)
			FskMemoryAllocatorPrintfDebug(" -- ACK!! next doesn't point back to this element\n");
		if (mx->next && ((mx->mem + mx->size) != mx->next->mem))
			FskMemoryAllocatorPrintfDebug(" -- ACK!! mem and size != next\n");
		mx = mx->next;
	}
	if (heap->size != total)
		FskMemoryAllocatorPrintfDebug("ACK!!! HEAP TOTAL DOESN'T MATCH SIZE\n");
}
#else
	#define pmb(a)	while (0) {}
	#define pfbmem()	while (0) {}
	#define dumpMem(a)	while (0) {}
#endif


static FskListMutex gHeaps = NULL;

FskErr FskMemoryHeapInitialize() {
	FskErr err = kFskErrNone;

	FskMemoryAllocatorPrintfDebug("FskMemoryHeapInitialize\n");
	if (!gHeaps) {
		err = FskListMutexNew(&gHeaps, "gHeaps");
	}
	return err;
}

FskErr FskMemoryHeapTerminate() {

	FskMemoryAllocatorPrintfDebug("FskMemoryHeapTerminate\n");
	if (!FskListMutexIsEmpty(gHeaps))
		return kFskErrIsBusy;
	FskListMutexDispose(gHeaps);
	gHeaps = NULL;
	return kFskErrNone;
}

FskErr FskMemoryHeapNew(UInt32 size, char *baseAddr, UInt32 alignmentQuantum, FskMemoryHeap *heapOut, char *name)
{
	FskMemoryBlock block = NULL;
	FskMemoryHeap heap = NULL, whatever;
	FskErr err;

	FskMemoryAllocatorPrintfDebug("FskMemoryHeapNew\n");
	if (alignmentQuantum < 4) {
		BAIL(kFskErrParameterError);
	}
	FskMemoryHeapInitialize();

	if ((NULL != name) && (kFskErrNone == FskMemoryHeapFindByName(name, &whatever))) {
		BAIL(kFskErrDuplicateElement);
	}

	err = FskMemPtrNewClear(sizeof(FskMemoryHeapRecord), (FskMemPtr*)(void*)&heap);
	BAIL_IF_ERR(err);
	heap->baseAddr = baseAddr;
	heap->size = size;
	heap->amtFree = size;
	heap->alignmentQuantum = alignmentQuantum;
	heap->name = FskStrDoCopy(name);
	err = FskMemPtrNewClear(sizeof(FskMemoryBlockRecord), (FskMemPtr*)(void*)&block);
	BAIL_IF_ERR(err);
	block->mem = baseAddr;
	block->size = size;
	err = FskMutexNew(&block->lock, "memory block lock");
	BAIL_IF_ERR(err);
	heap->blockHead = block;
	heap->blockNextFree = block;
	err = FskMutexNew(&heap->lock, "memory heap lock");
	BAIL_IF_ERR(err);
	FskListMutexAppend(gHeaps, heap);

bail:
	if (err && heap) {
		if (block) {
			FskMutexDispose(block->lock);
			FskMemPtrDispose(block);
		}
		FskMutexDispose(heap->lock);
		FskMemPtrDisposeAt(&heap);
	}
	*heapOut = heap;
	FskMemoryAllocatorPrintfDebug("FskMemoryHeapNew returns %x\n", heap);
	return err;
}

FskErr FskMemoryHeapDispose(FskMemoryHeap heap)
{
	FskMemoryAllocatorPrintfDebug("FskMemoryHeapDispose %x\n", heap);
	FskMutexAcquire(heap->lock);
// dispose of memBlocks
	FskListMutexRemove(gHeaps, heap);
	FskMutexDispose(heap->lock);
	FskMemPtrDispose(heap->name);
	FskMemPtrDispose(heap);
	FskMemoryHeapTerminate();
	return kFskErrNone;
}

FskErr FskMemoryHeapFindByName(char *name, FskMemoryHeap *heapOut)
{
	FskErr err = kFskErrNotFound;
	FskMemoryHeap heap = NULL;

	*heapOut = NULL;
	FskMutexAcquire(gHeaps->mutex);
	heap = gHeaps->list;
	while (heap) {
		if (0 == FskStrCompare(name, heap->name)) {
			*heapOut = heap;
			err = kFskErrNone;
			break;
		}
		heap = heap->next;
	}
	FskMutexRelease(gHeaps->mutex);
	return err;
}

FskErr FskMemoryHeapFindAndLock(char *ptr, FskMemoryHeap *heapOut)
{
	FskMemoryHeap heap = NULL;
	FskErr err = kFskErrNotFound;

	FskMemoryAllocatorPrintfDebug("FskMemoryHeapFindAndLock(%x)\n", ptr);
	if (!gHeaps) {
		goto bail;
	}
	FskMutexAcquire(gHeaps->mutex);
	heap = gHeaps->list;
	while (heap) {
		FskMutexAcquire(heap->lock);
		if ((ptr >= heap->baseAddr) && (ptr < heap->baseAddr + heap->size))
			break;
		FskMutexRelease(heap->lock);
		heap = heap->next;
	}
	FskMutexRelease(gHeaps->mutex);
	if (heap)
 		err = kFskErrNone;

bail:
	*heapOut = heap;
	FskMemoryAllocatorPrintfDebug("FskMemoryHeapFindAndLock(%x) returns %x (err:%d)\n", ptr, *heapOut, err);
	return err;
}

FskErr FskMemoryHeapFind(char *ptr, FskMemoryHeap *heapOut)
{
	FskErr err;
	err = FskMemoryHeapFindAndLock(ptr, heapOut);
	if (err)
		return err;
	FskMutexRelease((*heapOut)->lock);
	return kFskErrNone;
}


FskErr FskMemoryBlockFindAndLock(char *ptr, FskMemoryBlock *blockOut, FskMemoryHeap *heapOut)
{
	FskMemoryHeap heap;
	FskMemoryBlock block = NULL;
	FskErr err = kFskErrNotFound;

	FskMemoryAllocatorPrintfDebug("FskMemoryBlockFindAndLock(%x)\n", ptr);

	if (blockOut)
		*blockOut = NULL;

	if (kFskErrNone != FskMemoryHeapFindAndLock(ptr, &heap)) {
		if (heapOut)
			*heapOut = NULL;
		goto bail;
	}
	block = heap->blockHead;
	while (block) {
		FskMutexAcquire(block->lock);
		if ((ptr >= block->mem) && (ptr < block->mem + block->size))
			break;
		FskMutexRelease(block->lock);
		block = block->next;
	}
	if (heapOut)
		*heapOut = heap;
	else
		FskMutexRelease(heap->lock);
	if (blockOut) {
		*blockOut = block;
		err = kFskErrNone;
	}

bail:
	FskMemoryAllocatorPrintfDebug("FskMemoryBlockFindAndLock(%x) returns %x (%x)\n", ptr, blockOut ? *blockOut : NULL, heap);
	return err;
}

void FskMemoryHeapSetRef(FskMemoryHeap heap, void *ref)
{
	heap->ref = ref;
}

void *FskMemoryHeapGetRef(FskMemoryHeap heap)
{
	return heap->ref;
}

UInt32 FskMemoryHeapGetFree(FskMemoryHeap heap)
{
	return heap->amtFree;
}

UInt32 FskMemoryHeapGetContiguous(FskMemoryHeap heap)
{
	FskMemoryBlock	block;
	UInt32		biggest = 0;
	FskMutexAcquire(heap->lock);
	block = heap->blockHead;
	while (block) {
		if (!block->inUse)
			if (block->size > biggest)
				biggest = block->size;
		block = block->next;
	}
	FskMutexRelease(heap->lock);
	return biggest;
}

FskErr FskMemoryHeapCompact(FskMemoryHeap heap, UInt32 spaceNeeded)
{
	FskMemoryBlock	cur, nx, toss;
	Boolean			coalesce = false;
	UInt32			largest = 0, freesize;
	FskErr			err;

	FskMemoryAllocatorPrintfDebug("COMPACT MEM - need %d bytes\n", spaceNeeded);
	FskMemoryAllocatorPrintfDebug("----- PRE ---\n");
	dumpMem(heap);
	FskMemoryAllocatorPrintfDebug("------------------------\n");

	FskMutexAcquire(heap->lock);
	heap->blockNextFree = NULL;
	cur = heap->blockHead;
	while (cur) {
		if (!cur->inUse) {
			coalesce = false;
			// if this block is free, either move the next block down or coalesce it.
			nx = cur->next;
			if (nx) {
				if (nx->locked)
					goto blockLocked;

				// if nx is used, let the client move the data (and update refs)
				if (nx->inUse) {
					FskMutex	freeLock;
					err = nx->moveFn(nx->mem, cur->mem, nx->size, nx->ref);
					if (err) goto blockLocked;		// can't move it
					// shuffle metadata into previous block
					freeLock = cur->lock;
					cur->lock = nx->lock;
					nx->lock = freeLock;
					freesize = cur->size;
					cur->size = nx->size;
					cur->ref = nx->ref;
					cur->moveFn = nx->moveFn;
					cur->inUse = nx->inUse;
					nx->inUse = false;
					cur->locked = 0;
					// update other metadata block
					nx->mem = cur->mem + cur->size;
					nx->size = freesize;
					if (nx->next && !nx->next->inUse) {
						coalesce = true;
					}
				}
				else {
					nx = cur;
					// coalesce with next block (if it's free)
					if (nx->next && !nx->next->inUse) {
						coalesce = true;
					}
				}
				if (coalesce) {
					toss = nx->next;
					nx->size += toss->size;
					nx->next = toss->next;
					if (nx->next)
						nx->next->prev = nx;
					if (toss->locked)
						FskMemoryAllocatorPrintfDebug("oops\n");
					FskMutexDispose(toss->lock);
					FskMemPtrDispose(toss);
				}
				FskMemoryAllocatorPrintfDebug("----- POST ---\n");
				dumpMem(heap);
				FskMemoryAllocatorPrintfDebug("------------------------\n");
			}

blockLocked:
			if (cur->size > largest)
				largest = cur->size;
		}
		cur = cur->next;
	}

	FskMutexRelease(heap->lock);
	FskMemoryAllocatorPrintfDebug("largest unused block: %d\n", largest);
	if (largest >= spaceNeeded)
		return kFskErrNone;
	return kFskErrMemFull;
}

FskErr FskMemoryNew(FskMemoryHeap heap, UInt32 size, char **ptrOut, FskMemoryMoveFn move, void *ref)
{
	FskMemoryBlock	cur, nx = NULL;
	UInt32		t;
	FskErr		err;

	FskMemoryAllocatorPrintfDebug("FskMemoryNew (%x, %d, %x)\n", heap, size, ref);
	if (NULL == move)
		return kFskErrParameterError;

	FskMemoryAllocatorPrintfDebug("----- PRE ---\n");
	dumpMem(heap);
	FskMemoryAllocatorPrintfDebug("------------------------\n");

	size = ROUNDUP(heap, size);

	if (size > FB_ALLOCATOR_MAX_SIZE) {
		FskMemoryAllocatorPrintfDebug("allocation %d too big (limit: %d)\n", size, FB_ALLOCATOR_MAX_SIZE);
		*ptrOut = NULL;
		return kFskErrParameterError;
	}

//	memHeapCheckBusy(heap);

	FskMutexAcquire(heap->lock);

	if (size > heap->amtFree) {
		FskMemoryAllocatorPrintfDebug("**** can't get mem from allocator mem free %d, need %d) ****\n", heap->amtFree, size);//tmp
		*ptrOut = NULL;
		BAIL(kFskErrMemFull);
	}

	cur = heap->blockNextFree ? heap->blockNextFree : heap->blockHead;
	t = 0;

	// search for a free block
tryAgain:
	while (cur) {
		if (!cur->inUse && (cur->size >= size))
			break;
		cur = cur->next;
	}

	// if no free block, then compact memory and try again
	if (!cur) {
		// only do compact/retry once
		err = kFskErrMemFull;
		if (t == 1 || (kFskErrNone != (err = FskMemoryHeapCompact(heap, size)))) {
			FskMemoryAllocatorPrintfDebug("**** can't get %d bytes of FB mem (after compaction) ****\n", size);
			goto bail;
		}
		t = 1;
		cur = heap->blockHead;
		goto tryAgain;
	}

	// perfect size, no mem split necessary, next free can stay same
	// OR chunk is a bit bigger, but not big enough to split, don't
	// bother splitting it
	if ((cur->size >= size)
		&& (cur->size <= size + MINIMUM_BLOCK_SIZE)) {
		heap->amtFree -= cur->size;
		FskMemoryAllocatorPrintfDebug("**** FB allocated (no split) 0x%08x %d bytes -- %d left ****\n", cur, size, heap->amtFree);
		*ptrOut = cur->mem;
		cur->ref = ref;
		cur->moveFn = move;
		cur->inUse = true;
		cur->locked = 0;
		BAIL(kFskErrNone);
	}

	// need to split the memory chunk
	err = FskMemPtrNewClear(sizeof(FskMemoryBlockRecord), (FskMemPtr*)(void*)&nx);
	BAIL_IF_ERR(err);
	err = FskMutexNew(&nx->lock, "memory chunk lock");
	BAIL_IF_ERR(err);
	nx->next = cur->next;
	if (nx->next)
		nx->next->prev = nx;
	nx->prev = cur;
	nx->mem = cur->mem + size;
	nx->size = cur->size - size;
	nx->ref = 0;
	nx->moveFn = NULL;
	nx->inUse = false;
	nx->locked = 0;

	cur->next = nx;
	cur->size = size;
	cur->ref = ref;
	cur->moveFn = move;
	cur->inUse = true;
	cur->locked = 0;

	heap->blockNextFree = nx;
	heap->amtFree -= size;

	FskMemoryAllocatorPrintfDebug("**** FB allocated 0x%08x %d bytes -- %d left ****\n", cur, size, heap->amtFree);

	*ptrOut = cur->mem;
bail:
	if (kFskErrNone != err) {
		if (nx) {
			FskMutexDispose(nx->lock);
			FskMemPtrDispose(nx);
		}
		*ptrOut = NULL;
	}
	FskMutexRelease(heap->lock);
	FskMemoryAllocatorPrintfDebug("----- POST ---\n");
	dumpMem(heap);
	FskMemoryAllocatorPrintfDebug("------------------------\n");
	return err;
}

FskErr FskMemoryDispose(char *ptr)
{
	FskMemoryHeap	heap;
	FskMemoryBlock block;
	FskErr err = kFskErrNotFound;

	FskMemoryAllocatorPrintfDebug("FskMemoryDispose %x\n", ptr);
	if (kFskErrNone != FskMemoryBlockFindAndLock(ptr, &block, &heap))
		goto bail;
	FskMemoryAllocatorPrintfDebug("----- PRE ---\n");
	dumpMem(heap);
	FskMemoryAllocatorPrintfDebug("------------------------\n");

	if (block->locked) {
		err = kFskErrIsBusy;
		goto unlockNBail;
	}

	err = FskMemoryBlockDispose(heap, block);
	block = NULL;		// block lock was released in dispose (and block may have been coalesced)

unlockNBail:
	if (block)
		FskMutexRelease(block->lock);
	FskMutexRelease(heap->lock);
bail:
	FskMemoryAllocatorPrintfDebug("----- POST --- (block: %x, heap: %x)\n", block, heap);
	dumpMem(heap);
	FskMemoryAllocatorPrintfDebug("------------------------\n");
	return err;
}

FskErr FskMemoryBlockDispose(FskMemoryHeap heap, FskMemoryBlock block)
{
	FskMemoryBlock cur, nx;
	// assumes block is unlocked by user, heap and block mutexes are grabbed
	block->inUse = false;
	heap->amtFree += block->size;

	// coalesce
	// try this:
	cur = block->prev ? block->prev : heap->blockHead;
	while (cur->inUse)		// find first empty block
		cur = cur->next;

	// there should always be one (we just cleared one)
	if (cur->next && cur->next->inUse) {
		FskMemoryAllocatorPrintfDebug("freed a block %x between two that are in use. 'sok\n", cur);
	}
	else  {
		while (cur->next && !cur->next->inUse) {
			nx = cur->next;				// nx is going away
			cur->next = nx->next;
			if (cur->next)
				cur->next->prev = cur;
			cur->size += nx->size;
			FskMemoryAllocatorPrintfDebug("coalesced %x: cur->next is %08x", nx, cur->next);
			if (nx == block) {
				block = NULL;		// we're coalescing the original ptr we passed in, so don't free it.
				FskMutexRelease(nx->lock);
			}
			FskMutexDispose(nx->lock);
			FskMemPtrDispose(nx);
			if (cur->next) {
				FskMemoryAllocatorPrintfDebug(", cur->next->prev is %08x\n", cur->next);
			}
			else
				FskMemoryAllocatorPrintfDebug("\n");
		}
	}
	heap->blockNextFree = cur;

	if (block)
		FskMutexRelease(block->lock);

	return kFskErrNone;
}


FskErr FskMemoryLock(char *ptr)
{
	FskMemoryBlock block;
	FskErr err;

	FskMemoryAllocatorPrintfDebug("FskMemoryLock (%x)\n", ptr);
	if (kFskErrNone != (err = FskMemoryBlockFindAndLock(ptr, &block, NULL)))
		goto bail;
pmb(block);
	if (block->locked) {
		FskMemoryAllocatorPrintfDebug("locking a block that's already locked\n");
	}
	block->locked++;
	FskMutexRelease(block->lock);

bail:
	return err;
}

FskErr FskMemoryUnlock(char *ptr)
{
	FskMemoryBlock block;
	FskErr err;
	if (kFskErrNone != (err = FskMemoryBlockFindAndLock(ptr, &block, NULL)))
		goto bail;
	if (!block->locked) {
		FskMemoryAllocatorPrintfDebug("unlocking a block that's already unlocked\n");
	}
	block->locked--;
	FskMutexRelease(block->lock);

bail:
	return err;
}

Boolean FskMemoryTestLock(char *ptr)
{
	FskMemoryBlock block;
	FskErr err;
	Boolean locked = false;
	if (kFskErrNone != (err = FskMemoryBlockFindAndLock(ptr, &block, NULL)))
		goto bail;
	locked = block->locked > 0;
	FskMutexRelease(block->lock);
bail:
	return locked;
}

FskErr FskMemoryDefaultMoveFn(char *from, char *to, UInt32 size, void *ref)
{
	// we can't move the memory if we can't update any references to it.
	return kFskErrIsBusy;
}

FskErr FskMemorySetRef(char *ptr, void *ref)
{
	FskMemoryBlock block;
	FskErr err;
	if (kFskErrNone != (err = FskMemoryBlockFindAndLock(ptr, &block, NULL)))
		goto bail;
	block->ref = ref;
	FskMutexRelease(block->lock);
bail:
	return err;
}

void *FskMemoryGetRef(char *ptr)
{
	FskMemoryBlock block;
	FskErr err;
	if (kFskErrNone != (err = FskMemoryBlockFindAndLock(ptr, &block, NULL)))
		goto bail;
	FskMutexRelease(block->lock);
	return block->ref;
bail:
	return NULL;
}

