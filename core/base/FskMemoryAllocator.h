/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#ifndef __FSK_MEMORYALLOCATOR_H__
#define __FSK_MEMORYALLOCATOR_H__

#include "FskThread.h"

typedef FskErr (*FskMemoryMoveFn)(char *from, char *to, UInt32 size, void *ref);

typedef struct FskMemoryHeapRecord FskMemoryHeapRecord;
typedef struct FskMemoryHeapRecord *FskMemoryHeap;

typedef struct FskMemoryBlockRecord FskMemoryBlockRecord;
typedef struct FskMemoryBlockRecord *FskMemoryBlock;

#ifdef __FSKMEMORYALLOCATOR_PRIV__

struct FskMemoryBlockRecord {
	struct FskMemoryBlockRecord	*busyNext;
	struct FskMemoryBlockRecord	*next;
	struct FskMemoryBlockRecord	*prev;
	FskMutex				lock;		// for internal use
	char					*mem;
	UInt32					size;
	void					*ref;		// owner specified
	FskMemoryMoveFn 		moveFn;

	Boolean					inUse;	
	SInt32					locked;		// for user use
};

struct FskMemoryHeapRecord {
	struct FskMemoryHeapRecord	*next;
	char					*baseAddr;
	UInt32					size;
	UInt32					amtFree;
	UInt32					alignmentQuantum;
	FskMemoryBlock			blockHead;
	FskMemoryBlock			blockNextFree;
	FskListMutex			busyList;
	void					*ref;
	FskMutex				lock;
	char					*name;
};

#endif // __FSKMEMORYALLOCATOR_PRIV__


FskAPI(FskErr) FskMemoryHeapNew(UInt32 heapSize, char *baseAddr, UInt32 alignmentQuantum, FskMemoryHeap *heapOut, char *name);
FskAPI(FskErr) FskMemoryHeapDispose(FskMemoryHeap heap);
FskAPI(FskErr) FskMemoryHeapFind(char *ptr, FskMemoryHeap *heapOut);
FskAPI(FskErr) FskMemoryHeapFindByName(char *name, FskMemoryHeap *heapOut);

FskAPI(void)   FskMemoryHeapSetRef(FskMemoryHeap heap, void *ref);
FskAPI(void *) FskMemoryHeapGetRef(FskMemoryHeap heap);
FskAPI(UInt32) FskMemoryHeapGetFree(FskMemoryHeap heap);
FskAPI(UInt32) FskMemoryHeapGetContiguous(FskMemoryHeap heap);

FskAPI(FskErr) FskMemoryNew(FskMemoryHeap heap, UInt32 size, char **ptrOut, FskMemoryMoveFn move, void *ref);
FskAPI(FskErr) FskMemorySetRef(char *ptr, void *ref);
FskAPI(void *) FskMemoryGetRef(char *ptr);
FskAPI(FskErr) FskMemoryDispose(char *ptr);

FskAPI(FskErr) FskMemoryLock(char *ptr);
FskAPI(FskErr) FskMemoryUnlock(char *ptr);
FskAPI(Boolean) FskMemoryTestLock(char *ptr);

FskAPI(FskErr) FskMemoryDefaultMoveFn(char *from, char *to, UInt32 size, void *ref);

#endif // __FSK_MEMORYALLOCATOR_H__

