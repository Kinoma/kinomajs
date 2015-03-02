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
#ifndef __FSKMEMORY__
#define __FSKMEMORY__

#ifndef __FSK__
# include "Fsk.h"
#endif /* __FSK__ */

/*
 *	Memory
 */

typedef unsigned char *FskMemPtr;

#ifdef __cplusplus
extern "C" {
#endif

#if !SUPPORT_MEMORY_DEBUG

#define FskMemPtrNew(size, newMemory) FskMemPtrNew_(size, (FskMemPtr *)(void *)(newMemory))
#define FskMemPtrNewClear(size, newMemory) FskMemPtrNewClear_(size, (FskMemPtr *)(void *)(newMemory))
#define FskMemPtrRealloc(size, newMemory) FskMemPtrRealloc_(size, (FskMemPtr *)(void *)(newMemory))
#define FskMemPtrNewFromData(size, data, newMemory) FskMemPtrNewFromData_(size, data, (FskMemPtr *)(void *)(newMemory))
#define FskMemPtrDisposeAt(ptr) FskMemPtrDisposeAt_((void **)(void *)(ptr))

#define FskMemPtrNew_Untracked(size, newMemory) FskMemPtrNew(size, newMemory)
#define FskMemPtrNewClear_Untracked(size, newMemory) FskMemPtrNewClear(size, newMemory)
#define FskMemPtrDispose_Untracked(memory) FskMemPtrDispose(memory)

FskAPI(FskErr) FskMemPtrNew_(UInt32 size, FskMemPtr *newMemory);
FskAPI(FskErr) FskMemPtrNewClear_(UInt32 size, FskMemPtr *newMemory);
FskAPI(FskErr) FskMemPtrRealloc_(UInt32 size, FskMemPtr *newMemory);

FskAPI(FskMemPtr) FskMemPtrAlloc(UInt32 size);
FskAPI(FskMemPtr) FskMemPtrCalloc(UInt32 size);
FskAPI(FskMemPtr) FskMemPtrReallocC(void *mem, UInt32 size);

FskAPI(FskErr) FskMemPtrNewFromData_(UInt32 size, const void *data, FskMemPtr *newMemory);

FskAPI(FskErr) FskMemPtrDispose(void *ptr);
FskAPI(FskErr) FskMemPtrDisposeAt_(void **ptr);

#else

#define FSK_MEMORY_DEBUG_ARGS const char *file, UInt32 line, const char *function
#define FSK_MEMORY_DEBUG_PARAMS __FILE__, __LINE__, __FUNCTION__
#define FSK_MEMORY_DEBUG_PARAMS_CONTINUE file, line, function

FskAPI(FskErr) FskMemPtrNew_(UInt32 size, FskMemPtr *newMemory, FSK_MEMORY_DEBUG_ARGS);
FskAPI(FskErr) FskMemPtrNewClear_(UInt32 size, FskMemPtr *newMemory, FSK_MEMORY_DEBUG_ARGS);
FskAPI(FskErr) FskMemPtrRealloc_(UInt32 size, FskMemPtr *newMemory, FSK_MEMORY_DEBUG_ARGS);

FskAPI(FskMemPtr) FskMemPtrAlloc_(UInt32 size, FSK_MEMORY_DEBUG_ARGS);
FskAPI(FskMemPtr) FskMemPtrCalloc_(UInt32 size, FSK_MEMORY_DEBUG_ARGS);
FskAPI(FskMemPtr) FskMemPtrReallocC_(void *mem, UInt32 size, FSK_MEMORY_DEBUG_ARGS);

FskAPI(FskErr) FskMemPtrNewFromData_(UInt32 size, const void *data, FskMemPtr *newMemory, FSK_MEMORY_DEBUG_ARGS);

FskAPI(FskErr) FskMemPtrDispose_(void *ptr, FSK_MEMORY_DEBUG_ARGS);
FskAPI(FskErr) FskMemPtrDisposeAt_(void **ptr, FSK_MEMORY_DEBUG_ARGS);

#define FskMemPtrNew(size, newMemory) FskMemPtrNew_(size, (FskMemPtr *)(void *)(newMemory), FSK_MEMORY_DEBUG_PARAMS)
#define FskMemPtrNewClear(size, newMemory) FskMemPtrNewClear_(size, (FskMemPtr *)(void *)(newMemory), FSK_MEMORY_DEBUG_PARAMS)
#define FskMemPtrRealloc(size, newMemory) FskMemPtrRealloc_(size, (FskMemPtr *)(void *)(newMemory), FSK_MEMORY_DEBUG_PARAMS)

#define FskMemPtrAlloc(size) FskMemPtrAlloc_(size, FSK_MEMORY_DEBUG_PARAMS)
#define FskMemPtrCalloc(size) FskMemPtrCalloc_(size, FSK_MEMORY_DEBUG_PARAMS)
#define FskMemPtrReallocC(mem, size) FskMemPtrReallocC_(mem, size, FSK_MEMORY_DEBUG_PARAMS)

#define FskMemPtrNewFromData(size, data, newMemory) FskMemPtrNewFromData_(size, data, (FskMemPtr *)(void *)(newMemory), FSK_MEMORY_DEBUG_PARAMS)

#define FskMemPtrDispose(ptr) FskMemPtrDispose_(ptr, FSK_MEMORY_DEBUG_PARAMS)
#define FskMemPtrDisposeAt(ptr) FskMemPtrDisposeAt_((void **)(void *)(ptr), FSK_MEMORY_DEBUG_PARAMS)

FskAPI(FskErr) FskMemPtrNew_Untracked_(UInt32 size, FskMemPtr *newMemory);
FskAPI(FskErr) FskMemPtrNewClear_Untracked_(UInt32 size, FskMemPtr *newMemory);
FskAPI(FskErr) FskMemPtrDispose_Untracked(void *ptr);

#define FskMemPtrNew_Untracked(size, newMemory) FskMemPtrNew_Untracked_(size, (FskMemPtr *)(void *)(newMemory))
#define FskMemPtrNewClear_Untracked(size, newMemory) FskMemPtrNewClear_Untracked_(size, (FskMemPtr *)(void *)(newMemory))

typedef struct FskMemoryDebugRecord FskMemoryDebugRecord;
typedef struct FskMemoryDebugRecord *FskMemoryDebug;

struct FskMemoryDebugRecord {
	UInt32				magic;

	FskMemoryDebug		prev;
	FskMemoryDebug		next;

	UInt32				size;
		
	const char			*file;
	UInt32				line;
	const char			*function;
		
	UInt32				seed;
};

enum {
	kFskMemoryDebugMagic = 0x76543210
};

FskMemoryDebug FskMemoryDebugGetList(void);

#endif

FskAPI(void) FskMemMove(void *dst, const void *src, UInt32 count);
FskAPI(void) FskMemCopy(void *dst, const void *src, UInt32 count);
FskAPI(void) FskMemSet(void *dst, unsigned char fill, UInt32 count);
FskAPI(SInt32) FskMemCompare(const void *s1, const void *s2, UInt32 count);

void FskMemoryInitialize(void);
void FskMemoryTerminate(void);

#if SUPPORT_INSTRUMENTATION

enum {
	kFskFileInstrMsgAllocate = kFskInstrumentedItemFirstCustomMessage,
	kFskFileInstrMsgAllocateFailed,
	kFskFileInstrMsgFree,
	kFskFileInstrMsgReallocate,
	kFskFileInstrMsgReallocateFailed,
	kFskFileInstrMsgInUse,
	kFskFileInstrMsgFreeInvalid,
	kFskFileInstrMsgBlockInvalid

};

typedef struct {
	void		*data;
	UInt32		size;
	void		*originalData;
	Boolean		clear;
} FskFileInstrMsgMemoryRecord;

#endif

#ifdef __cplusplus
}
#endif

#endif

