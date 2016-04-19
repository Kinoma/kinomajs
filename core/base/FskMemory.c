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
#define _WIN32_WINNT 0x0500
#define __FSKTHREAD_PRIV__

#include "FskMemory.h"
#include "FskList.h"
#include "FskThread.h"

#if TARGET_OS_WIN32
	#include "shlobj.h"

	#define COBJMACROS
	#include <initguid.h>
	#include "ntverp.h"

#elif TARGET_OS_LINUX
	#include <stdio.h>
	#include <string.h>
#if LOCKMEM
	#include <sys/mman.h>

	typedef struct memhead {
		char	*xdata;
		UInt32	size;
		UInt32	alignment[2];
		char	data[0];
	} memhead, *mh;

	#define	OFFSET		sizeof(memhead)
	#define THRESHOLD	(0)
#endif
#elif TARGET_OS_KPL
	#include <string.h>
	#define __FSKMEMORY_KPL_PRIV__
	#include "KplMemory.h"
#endif

#if SUPPORT_MEMORY_DEBUG

#ifndef SUPPORT_MEMORY_THRESHOLD
	#define SUPPORT_MEMORY_THRESHOLD 0
#endif

#define kMemoryDebugThreshold (30000 * 1024)
//#define kMemoryDebugThreshold (12250 * 1024)
//#define kMemoryDebugThreshold (11460 * 1024)
#define kMemoryDebugModulo (20000000)

#include "FskSynchronization.h"

FskMutex gMemoryDebugMutex = NULL;

FskMemoryDebug gMemoryDebugList = NULL;

UInt32 gMemoryDebugSeed = 0;

UInt32 gMemoryDebugInUse = 0;

static void rememberMemory(FskMemoryDebug debug, UInt32 size, FSK_MEMORY_DEBUG_ARGS);
static void updateMemory(FskMemoryDebug debug, UInt32 size, FSK_MEMORY_DEBUG_ARGS);
static void linkMemory(FskMemoryDebug debug);
static void unlinkMemory(FskMemoryDebug debug);
static void forgetMemory(FskMemoryDebug debug);

enum {
	kFskDebugUpdateInUseMask = 0x0f,
	kFskMemoryDebugTrailerSize = 1,
	kFskMemoryTrailerByte = 0x99
};

#define FSK_DEBUG_STOP(s) {FskDebugStr(s); *(int *)0 = 0;}

#else
	#define SUPPORT_MEMORY_THRESHOLD 0
#endif

#if SUPPORT_INSTRUMENTATION
	#include "FskThread.h"

	static Boolean doFormatMessageMemory(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gMemoryTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"memory",
		0,
		NULL,
		0,
		NULL,
		doFormatMessageMemory
	};
#endif

/*
 *	Memory
 */

#if !SUPPORT_MEMORY_DEBUG
FskErr FskMemPtrNew_(UInt32 size, FskMemPtr *newMemory)
#else
FskErr FskMemPtrNew_(UInt32 size, FskMemPtr *newMemory, FSK_MEMORY_DEBUG_ARGS)
#endif
{
#if SUPPORT_MEMORY_THRESHOLD
	if ((gMemoryDebugInUse > kMemoryDebugThreshold) || ((kMemoryDebugModulo >> 1) == (gMemoryDebugSeed % kMemoryDebugModulo))) {
		gMemoryDebugSeed++;
		*newMemory = NULL;
		return kFskErrMemFull;
	}
#endif
#if !LOCKMEM

#if !SUPPORT_MEMORY_DEBUG
#if TARGET_OS_KPL
	if (kFskErrNone != KplMemPtrNew(size, newMemory))
		return kFskErrMemFull;
#else
	*newMemory = (FskMemPtr)malloc(size);
#endif
#else
	{
	FskMemoryDebug debug;

#if TARGET_OS_KPL
	if (kFskErrNone != KplMemPtrNew(size + sizeof(FskMemoryDebugRecord) + kFskMemoryDebugTrailerSize, (FskMemoryDebug*)&debug))
		return kFskErrMemFull;
#else
	debug = (FskMemoryDebug)malloc(size + sizeof(FskMemoryDebugRecord) + kFskMemoryDebugTrailerSize);
#endif
	if (debug) {
		rememberMemory(debug, size, FSK_MEMORY_DEBUG_PARAMS_CONTINUE);
		*newMemory = (FskMemPtr)(debug + 1);
	}
	else
		*newMemory = NULL;
	}
#endif

#if SUPPORT_INSTRUMENTATION
	if (NULL == *newMemory) {
		if (FskInstrumentedTypeHasListenersForLevel(&gMemoryTypeInstrumentation, kFskInstrumentationLevelMinimal)) {
			FskFileInstrMsgMemoryRecord msg;
			msg.size = size;
			msg.clear = false;
			FskInstrumentedTypeSendMessageMinimal(&gMemoryTypeInstrumentation, kFskFileInstrMsgAllocateFailed, &msg);
		}
	}
	else
	if (FskInstrumentedTypeHasListenersForLevel(&gMemoryTypeInstrumentation, kFskInstrumentationLevelDebug)) {
		FskFileInstrMsgMemoryRecord msg;
		msg.data = *newMemory;
		msg.size = size;
		msg.clear = false;
		FskInstrumentedTypeSendMessageDebug(&gMemoryTypeInstrumentation, kFskFileInstrMsgAllocate, &msg);
	}
#endif

	return *newMemory ? kFskErrNone : kFskErrMemFull;
#else
#if TARGET_OS_KPL
	#error - LOCKMEM
#endif
	int err;
	mh	h;
	h = (mh)malloc(size + OFFSET);
	if (h) {
		if (size > THRESHOLD) {
			err = mlock(h, size + OFFSET);
			if (-1 == err) {
				FskDebugStr("mlock fails for alloc sized %d", size);
				h->size = 0;
				h->xdata = 0;
				free(h);
				*newMemory = NULL;
				return kFskErrMemFull;
			}
			h->size = size;
		}
		else
			h->size = 0;		// don't unlock
		h->xdata = (char*)h + OFFSET;
		*newMemory = h->xdata;
		return kFskErrNone;
	}
	*newMemory = NULL;
	return kFskErrMemFull;
#endif
}

#if !SUPPORT_MEMORY_DEBUG
FskErr FskMemPtrNewClear_(UInt32 size, FskMemPtr *newMemory)
#else
FskErr FskMemPtrNewClear_(UInt32 size, FskMemPtr *newMemory, FSK_MEMORY_DEBUG_ARGS)
#endif
{
#if SUPPORT_MEMORY_THRESHOLD
	if ((gMemoryDebugInUse > kMemoryDebugThreshold) || ((kMemoryDebugModulo >> 1) == (gMemoryDebugSeed % kMemoryDebugModulo))) {
		gMemoryDebugSeed++;
		*newMemory = NULL;
		return kFskErrMemFull;
	}
#endif

#if !LOCKMEM

#if !SUPPORT_MEMORY_DEBUG
#if TARGET_OS_KPL
	if (kFskErrNone != KplMemPtrNewClear(size, newMemory))
		return kFskErrMemFull;
#else
	*newMemory = (FskMemPtr)calloc(1, size);
#endif
#else
	{
	FskMemoryDebug debug;

#if TARGET_OS_KPL
	if (kFskErrNone != KplMemPtrNewClear(size + sizeof(FskMemoryDebugRecord) + kFskMemoryDebugTrailerSize, (FskMemoryDebug*)&debug))
		return kFskErrMemFull;
#else
	debug = (FskMemoryDebug)calloc(1, size + sizeof(FskMemoryDebugRecord) + kFskMemoryDebugTrailerSize);
#endif

	if (debug) {
		rememberMemory(debug, size, FSK_MEMORY_DEBUG_PARAMS_CONTINUE);
		*newMemory = (FskMemPtr)(debug + 1);
	}
	else
		*newMemory = NULL;
	}
#endif

#if SUPPORT_INSTRUMENTATION
	if (NULL == *newMemory) {
		if (FskInstrumentedTypeHasListenersForLevel(&gMemoryTypeInstrumentation, kFskInstrumentationLevelMinimal)) {
			FskFileInstrMsgMemoryRecord msg;
			msg.size = size;
			msg.clear = true;
			FskInstrumentedTypeSendMessageMinimal(&gMemoryTypeInstrumentation, kFskFileInstrMsgAllocateFailed, &msg);
		}
	}
	else
	if (FskInstrumentedTypeHasListenersForLevel(&gMemoryTypeInstrumentation, kFskInstrumentationLevelDebug)) {
		FskFileInstrMsgMemoryRecord msg;
		msg.data = *newMemory;
		msg.size = size;
		msg.clear = true;
		FskInstrumentedTypeSendMessageDebug(&gMemoryTypeInstrumentation, kFskFileInstrMsgAllocate, &msg);
	}
#endif

	return *newMemory ? kFskErrNone : kFskErrMemFull;
#else
#if TARGET_OS_KPL
	#error - LOCKMEM
#endif
	int err;
	mh	h;
	h = (mh)calloc(1, size + OFFSET);
	if (h) {
		if (size > THRESHOLD) {
			err = mlock(h, size + OFFSET);
			if (-1 == err) {
				FskDebugStr("mlock fails for calloc sized  %d", size);
				h->size = 0;
				h->xdata = 0;
				free(h);
				*newMemory = NULL;
				return kFskErrMemFull;
			}
			h->size = size;
		}
		else
			h->size = 0;		// don't unlock
		h->xdata = (char*)h + OFFSET;
		*newMemory = h->xdata;
		return kFskErrNone;
	}
	*newMemory = NULL;
	return kFskErrMemFull;
#endif
}

// ---------------------------------------------------------------------
#if !SUPPORT_MEMORY_DEBUG
FskMemPtr FskMemPtrAlloc(UInt32 size)
#else
FskMemPtr FskMemPtrAlloc_(UInt32 size, FSK_MEMORY_DEBUG_ARGS)
#endif
{
	FskMemPtr m;
	FskErr err;
#if !SUPPORT_MEMORY_DEBUG
	err = FskMemPtrNew(size, &m);
#else
	err = FskMemPtrNew_(size, &m, FSK_MEMORY_DEBUG_PARAMS_CONTINUE);
#endif
	if (kFskErrNone != err)
		return NULL;

	return m;
}

// ---------------------------------------------------------------------
#if !SUPPORT_MEMORY_DEBUG
FskMemPtr FskMemPtrCalloc(UInt32 size)
#else
FskMemPtr FskMemPtrCalloc_(UInt32 size, FSK_MEMORY_DEBUG_ARGS)
#endif
{
	FskMemPtr m;
#if !SUPPORT_MEMORY_DEBUG
	FskMemPtrNewClear(size, &m);
#else
	FskMemPtrNewClear_(size, &m, FSK_MEMORY_DEBUG_PARAMS_CONTINUE);
#endif
	return m;
}

// ---------------------------------------------------------------------
#if !SUPPORT_MEMORY_DEBUG
FskErr FskMemPtrRealloc_(UInt32 size, FskMemPtr *newMemory)
#else
FskErr FskMemPtrRealloc_(UInt32 size, FskMemPtr *newMemory, FSK_MEMORY_DEBUG_ARGS)
#endif
{
	if (NULL == *newMemory) {
#if !SUPPORT_MEMORY_DEBUG
		return FskMemPtrNew(size, newMemory);
#else
		return FskMemPtrNew_(size, newMemory, FSK_MEMORY_DEBUG_PARAMS_CONTINUE);
#endif
	}
	else
#if !LOCKMEM
	{
		FskMemPtr result;
#if !SUPPORT_MEMORY_DEBUG
#if TARGET_OS_KPL
		KplMemPtrRealloc(size, newMemory);
		result = *newMemory;
#else
		result = (FskMemPtr)realloc(*newMemory, size);
#endif
#else
		{
		FskMemoryDebug debug = (FskMemoryDebug)(((char *)*newMemory) - sizeof(FskMemoryDebugRecord));

		unlinkMemory(debug);
#if TARGET_OS_KPL
		if (kFskErrNone != KplMemPtrRealloc(size + sizeof(FskMemoryDebugRecord) + kFskMemoryDebugTrailerSize, &debug))
			return kFskErrMemFull;
#else
		debug = (FskMemoryDebug)realloc(debug, size + sizeof(FskMemoryDebugRecord) + kFskMemoryDebugTrailerSize);
#endif
		if (debug) {
			updateMemory(debug, size, FSK_MEMORY_DEBUG_PARAMS_CONTINUE);
			result = (FskMemPtr)(debug + 1);
		}
		else {
			linkMemory(debug);
			result = NULL;
		}
		}
#endif

		if (NULL == result && 0 != size) {
#if SUPPORT_INSTRUMENTATION
			if (FskInstrumentedTypeHasListenersForLevel(&gMemoryTypeInstrumentation, kFskInstrumentationLevelMinimal)) {
				FskFileInstrMsgMemoryRecord msg;
				msg.size = size;
				msg.originalData = *newMemory;
				FskInstrumentedTypeSendMessageMinimal(&gMemoryTypeInstrumentation, kFskFileInstrMsgReallocateFailed, &msg);
			}
#endif
			return kFskErrMemFull;
		}
		else {
#if SUPPORT_INSTRUMENTATION
			if (FskInstrumentedTypeHasListenersForLevel(&gMemoryTypeInstrumentation, kFskInstrumentationLevelDebug)) {
				FskFileInstrMsgMemoryRecord msg;
				msg.size = size;
				msg.originalData = *newMemory;
				msg.data = result;
				FskInstrumentedTypeSendMessageDebug(&gMemoryTypeInstrumentation, kFskFileInstrMsgReallocate, &msg);
			}
#endif
		}

		*newMemory = result;
		return kFskErrNone;
	}
#else
	{
#if TARGET_OS_KPL
	#error - LOCKMEM
#endif
	int err;
	mh	h;

	h = (char*)(*newMemory) - OFFSET;
	if (h->xdata != *newMemory)
		FskDebugStr("bad memory reference in realloc");
	else {
		if (h->size) {
			err = munlock(h, h->size + OFFSET);
			if (-1 == err)
				FskDebugStr("munlock fails for realloc: old size  %d", h->size);
		}
	}
	h = (mh)realloc(h, size + OFFSET);
	if (h) {
		if (size > THRESHOLD) {
			err = mlock(h, size + OFFSET);
			if (-1 == err) {
				FskDebugStr("mlock fails for realloc sized  %d", size);
				h->size = 0;
				h->xdata = 0;
				free(h);
				*newMemory = NULL;
				return kFskErrMemFull;
			}
			h->size = size;
		}
		else
			h->size = 0;		// don't unlock
		h->xdata = (char*)h + OFFSET;
		*newMemory = h->xdata;
		return kFskErrNone;
	}
	*newMemory = NULL;
	return kFskErrMemFull;
	}
#endif

	return (*newMemory || (0 == size)) ? kFskErrNone : kFskErrMemFull;
}

#if !SUPPORT_MEMORY_DEBUG
FskMemPtr FskMemPtrReallocC(void *mem, UInt32 size)
#else
FskMemPtr FskMemPtrReallocC_(void *mem, UInt32 size, FSK_MEMORY_DEBUG_ARGS)
#endif
{
#if !SUPPORT_MEMORY_DEBUG
	if (kFskErrNone == FskMemPtrRealloc(size, (FskMemPtr*)(void*)&mem))
#else
	if (kFskErrNone == FskMemPtrRealloc_(size, (FskMemPtr*)(void*)&mem, FSK_MEMORY_DEBUG_PARAMS_CONTINUE))
#endif
		return (FskMemPtr)mem;

	return NULL;
}


// ---------------------------------------------------------------------
#if !SUPPORT_MEMORY_DEBUG
FskErr FskMemPtrDispose(void *ptr)
#else
FskErr FskMemPtrDispose_(void *ptr, FSK_MEMORY_DEBUG_ARGS)
#endif
{
	if (NULL != ptr) {
#if !LOCKMEM
#if SUPPORT_MEMORY_DEBUG && SUPPORT_INSTRUMENTATION
		FskMemoryDebug debug = (FskMemoryDebug)(((char *)ptr) - sizeof(FskMemoryDebugRecord));

		if (kFskMemoryDebugMagic != debug->magic)
			FskInstrumentedTypeSendMessageMinimal(&gMemoryTypeInstrumentation, kFskFileInstrMsgFreeInvalid, ptr);

		if (kFskMemoryTrailerByte != *(((unsigned char *)(debug + 1)) + debug->size))
			FskInstrumentedTypeSendMessageMinimal(&gMemoryTypeInstrumentation, kFskFileInstrMsgBlockInvalid, ptr);

		debug->file = file;
		debug->line = line;
		debug->function = function;
#endif
		FskInstrumentedTypeSendMessageDebug(&gMemoryTypeInstrumentation, kFskFileInstrMsgFree, ptr);

#if !SUPPORT_MEMORY_DEBUG
#if TARGET_OS_KPL
		KplMemPtrDispose(ptr);
#else
		free(ptr);
#endif
#else
		forgetMemory((FskMemoryDebug)(((char *)ptr) - sizeof(FskMemoryDebugRecord)));
#if TARGET_OS_KPL
		KplMemPtrDispose((FskMemoryDebug)(((char *)ptr) - sizeof(FskMemoryDebugRecord)));
#else
		free((FskMemoryDebug)(((char *)ptr) - sizeof(FskMemoryDebugRecord)));
#endif
#endif
#else
#if TARGET_OS_KPL
	#error - LOCKMEM
#endif
		int err;
		mh	h;
		h = (char*)ptr - OFFSET;
		if (h->xdata != ptr) {
			FskDebugStr("bad memory reference in dispose");
			free(ptr);
		}
		else {
			if (h->size) {
				err = munlock(h, h->size + OFFSET);
				if (-1 == err)
					FskDebugStr("munlock fails for dispose sized %d", h->size);
				h->size = 0;
				h->xdata = 0;
			}
			free(h);
		}
#endif
	}

	return kFskErrNone;
}

// ---------------------------------------------------------------------
// for convenience, takes pointer to pointer and sets result to NULL
#if !SUPPORT_MEMORY_DEBUG
FskErr FskMemPtrDisposeAt_(void **ptr)
#else
FskErr FskMemPtrDisposeAt_(void **ptr, FSK_MEMORY_DEBUG_ARGS)
#endif
{
	if (NULL != *ptr) {
#if !SUPPORT_MEMORY_DEBUG
		FskMemPtrDispose(*ptr);
#else
		FskMemPtrDispose_(*ptr, FSK_MEMORY_DEBUG_PARAMS_CONTINUE);
#endif
		*ptr = NULL;
	}

	return kFskErrNone;
}

// ---------------------------------------------------------------------
#if !SUPPORT_MEMORY_DEBUG
FskErr FskMemPtrNewFromData_(UInt32 size, const void *data, FskMemPtr *newMemory)
#else
FskErr FskMemPtrNewFromData_(UInt32 size, const void *data, FskMemPtr *newMemory, FSK_MEMORY_DEBUG_ARGS)
#endif
{
	FskErr err;

	if ((NULL == data) || (0 == size)) {
		*newMemory = NULL;
		err = kFskErrNone;
	}
	else {
#if !SUPPORT_MEMORY_DEBUG
		err = FskMemPtrNew(size, newMemory);
#else
		err = FskMemPtrNew_(size, newMemory, FSK_MEMORY_DEBUG_PARAMS_CONTINUE);
#endif
		if (kFskErrNone == err)
			FskMemCopy(*newMemory, data, size);
	}

	return err;
}

// ---------------------------------------------------------------------
void FskMemMove(void *dst, const void *src, UInt32 count)
{
#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_ANDROID || TARGET_OS_KPL
	memmove(dst, src, count);
#else
	if (dst <= src) {
		char *d = (char *)dst;
		const char *s = (const char *)src;

		while (count--)
			*d++ = *s++;
	}
	else {
		char *d = (char *)dst + count;
		const char *s = (const char *)src + count;
		while (count--)
			*--d = *--s;
	}
#endif
}

// ---------------------------------------------------------------------
void FskMemSet(void *dst, unsigned char fill, UInt32 count)
{
#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_ANDROID || TARGET_OS_KPL
 	memset(dst, fill, count);
#else
	char *d = (char *)dst;

	while (count--)
		*d++ = fill;
#endif
}

// ---------------------------------------------------------------------
void FskMemCopy(void *dst, const void *src, UInt32 count)
{
#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_ANDROID || TARGET_OS_KPL
	if (count)
		memcpy(dst, src, count);
#else
	char *d = (char*)dst;
	const char *s = (const char *)src;

	if (count)
		while (count--)
			*d++ = *s++;
#endif
}

// ---------------------------------------------------------------------
SInt32 FskMemCompare(const void *s1, const void *s2, UInt32 count)
{
	SInt32	ret = 0;
	char	*a, *b;
	a = (char*)s1;
	b = (char*)s2;
	while (count) {
		ret = *b++ - *a++;
		if (ret != 0)
			return ret > 0 ? 1 : -1;
		count--;
	}
	return ret;
}

#if SUPPORT_MEMORY_DEBUG

void rememberMemory(FskMemoryDebug debug, UInt32 size, FSK_MEMORY_DEBUG_ARGS)
{
	if (gMemoryDebugMutex)
		FskMutexAcquire_uninstrumented(gMemoryDebugMutex);

	debug->magic = kFskMemoryDebugMagic;
	debug->prev = NULL;
	debug->next = gMemoryDebugList;
	if (gMemoryDebugList)
		gMemoryDebugList->prev = debug;
	gMemoryDebugList = debug;
	debug->size = size;
	debug->file = file;
	debug->line = line;
	debug->function = function;
	debug->seed = gMemoryDebugSeed++;

	*(((unsigned char *)(debug + 1)) + size) = kFskMemoryTrailerByte;

	gMemoryDebugInUse += size;

#if SUPPORT_INSTRUMENTATION
	if (0 == (gMemoryDebugSeed & kFskDebugUpdateInUseMask))
		FskInstrumentedTypeSendMessageVerbose(&gMemoryTypeInstrumentation, kFskFileInstrMsgInUse, (void *)gMemoryDebugInUse);
#endif

	if (gMemoryDebugMutex)
		FskMutexRelease_uninstrumented(gMemoryDebugMutex);
}

void updateMemory(FskMemoryDebug debug, UInt32 size, FSK_MEMORY_DEBUG_ARGS)
{
	if (gMemoryDebugMutex)
		FskMutexAcquire_uninstrumented(gMemoryDebugMutex);

	gMemoryDebugSeed++;

	debug->prev = NULL;
	debug->next = gMemoryDebugList;
	if (gMemoryDebugList)
		gMemoryDebugList->prev = debug;
	gMemoryDebugList = debug;

	*(((unsigned char *)(debug + 1)) + size) = kFskMemoryTrailerByte;

	gMemoryDebugInUse += size - debug->size;

	debug->file = file;
	debug->line = line;
	debug->function = function;

	debug->size = size;

#if SUPPORT_INSTRUMENTATION
	if (0 == (gMemoryDebugSeed & kFskDebugUpdateInUseMask))
		FskInstrumentedTypeSendMessageVerbose(&gMemoryTypeInstrumentation, kFskFileInstrMsgInUse, (void *)gMemoryDebugInUse);
#endif

	if (gMemoryDebugMutex)
		FskMutexRelease_uninstrumented(gMemoryDebugMutex);
}

void linkMemory(FskMemoryDebug debug)
{
	if (gMemoryDebugMutex)
		FskMutexAcquire_uninstrumented(gMemoryDebugMutex);

	debug->prev = NULL;
	debug->next = gMemoryDebugList;
	if (gMemoryDebugList)
		gMemoryDebugList->prev = debug;
	gMemoryDebugList = debug;

	if (gMemoryDebugMutex)
		FskMutexRelease_uninstrumented(gMemoryDebugMutex);
}

void unlinkMemory(FskMemoryDebug debug)
{
	if (gMemoryDebugMutex)
		FskMutexAcquire_uninstrumented(gMemoryDebugMutex);

	if (debug->prev)
		debug->prev->next = debug->next;
	else
		gMemoryDebugList = debug->next;

	if (debug->next)
		debug->next->prev = debug->prev;

	if (gMemoryDebugMutex)
		FskMutexRelease_uninstrumented(gMemoryDebugMutex);
}

void forgetMemory(FskMemoryDebug debug)
{
	if (gMemoryDebugMutex)
		FskMutexAcquire_uninstrumented(gMemoryDebugMutex);

	if (kFskMemoryDebugMagic != debug->magic)
		FSK_DEBUG_STOP("corrupt block debug header");

	if (kFskMemoryTrailerByte != *(((unsigned char *)(debug + 1)) + debug->size))
		FSK_DEBUG_STOP("corrupt block debug trailer");

	if (debug->prev)
		debug->prev->next = debug->next;
	else
		gMemoryDebugList = debug->next;

	if (debug->next)
		debug->next->prev = debug->prev;

	gMemoryDebugInUse -= debug->size;

	debug->magic = kFskUInt32Max;
	*(((unsigned char *)(debug + 1)) + debug->size) = kFskMemoryTrailerByte + 1;

	gMemoryDebugSeed++;

#if SUPPORT_INSTRUMENTATION
	if (0 == (gMemoryDebugSeed & kFskDebugUpdateInUseMask))
		FskInstrumentedTypeSendMessageVerbose(&gMemoryTypeInstrumentation, kFskFileInstrMsgInUse, (void *)gMemoryDebugInUse);
#endif

	if (gMemoryDebugMutex)
		FskMutexRelease_uninstrumented(gMemoryDebugMutex);
}

FskErr FskMemPtrNew_Untracked_(UInt32 size, FskMemPtr *newMemory)
{
#if TARGET_OS_KPL
	if (kFskErrNone != KplMemPtrNew(size, newMemory))
		return kFskErrMemFull;
#else
	*newMemory = (FskMemPtr)malloc(size);
#endif
	return *newMemory ? kFskErrNone : kFskErrMemFull;
}

FskErr FskMemPtrNewClear_Untracked_(UInt32 size, FskMemPtr *newMemory)
{
#if TARGET_OS_KPL
	if (kFskErrNone != KplMemPtrNewClear(size, newMemory))
		return kFskErrMemFull;
#else
	*newMemory = (FskMemPtr)calloc(size, 1);
#endif
	return *newMemory ? kFskErrNone : kFskErrMemFull;
}

FskErr FskMemPtrDispose_Untracked(void *ptr)
{
#if TARGET_OS_KPL
	if (ptr)
		KplMemPtrDispose(ptr);
#else
	if (ptr)
		free(ptr);
#endif

	return kFskErrNone;
}

FskMemoryDebug FskMemoryDebugGetList(void)
{
	return gMemoryDebugList;
}

#endif

void FskMemoryInitialize(void)
{
#if SUPPORT_INSTRUMENTATION
	FskInstrumentationAddType(&gMemoryTypeInstrumentation);

	#if SUPPORT_MEMORY_DEBUG
		FskMutexNew_uninstrumented(&gMemoryDebugMutex, "gMemoryDebugMutex");
	#endif
#endif

#if TARGET_OS_KPL
	KplMemoryInitialize();
#endif
}

void FskMemoryTerminate(void)
{
#if TARGET_OS_KPL
	KplMemoryTerminate();
#endif

#if SUPPORT_INSTRUMENTATION && SUPPORT_MEMORY_DEBUG
	FskMutexDispose_uninstrumented(gMemoryDebugMutex);
#endif
}

#if SUPPORT_INSTRUMENTATION

Boolean doFormatMessageMemory(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	FskFileInstrMsgMemoryRecord *mmr = (FskFileInstrMsgMemoryRecord *)msgData;

	switch (msg) {
		case kFskFileInstrMsgAllocate: {
			const char *op = mmr->clear ? "calloc" : "malloc";
#if !SUPPORT_MEMORY_DEBUG
			snprintf(buffer, bufferSize, "%s %p, size=%u", op, mmr->data, (unsigned)mmr->size);
#else
			FskMemoryDebug debug = (FskMemoryDebug)(((char *)mmr->data) - sizeof(FskMemoryDebugRecord));
			if (debug->function)
				snprintf(buffer, bufferSize, "%s %p, size=%lu, caller=%s @ line=%lu", op, mmr->data, mmr->size, debug->function, debug->line);
			else
				snprintf(buffer, bufferSize, "%s %p, size=%lu", op, mmr->data, mmr->size);
#endif
			}
			return true;

		case kFskFileInstrMsgAllocateFailed:
			if (mmr->clear)
				snprintf(buffer, bufferSize, "calloc failed size=%u", (unsigned)mmr->size);
			else
				snprintf(buffer, bufferSize, "alloc failed size=%u", (unsigned)mmr->size);
			return true;

		case kFskFileInstrMsgReallocate: {
#if !SUPPORT_MEMORY_DEBUG
			snprintf(buffer, bufferSize, "realloc %p, size=%u, original=%p", mmr->data, (unsigned)mmr->size, mmr->originalData);
#else
			FskMemoryDebug debug = (FskMemoryDebug)(((char *)mmr->data) - sizeof(FskMemoryDebugRecord));
			if (debug->function)
				snprintf(buffer, bufferSize, "realloc %p, size=%u, original=%p, caller=%s @ line=%lu", mmr->data, (unsigned)mmr->size, mmr->originalData, debug->function, debug->line);
			else
				snprintf(buffer, bufferSize, "realloc %p, size=%lu, original=%p", mmr->data, mmr->size, mmr->originalData);
#endif
			}
			return true;

		case kFskFileInstrMsgReallocateFailed:
			snprintf(buffer, bufferSize, "realloc failed size=%u, original=%p", (unsigned)mmr->size, mmr->originalData);
			return true;

		case kFskFileInstrMsgFree:
#if !SUPPORT_MEMORY_DEBUG
			snprintf(buffer, bufferSize, "free %p", msgData);
#else
			{
			FskMemoryDebug debug = (FskMemoryDebug)(((char *)msgData) - sizeof(FskMemoryDebugRecord));
			if (debug->function)
				snprintf(buffer, bufferSize, "free %p, size=%lu, caller=%s @ line=%lu", msgData, debug->size, debug->function, debug->line);
			else
				snprintf(buffer, bufferSize, "free %p, size=%lu", msgData, debug->size);
			}
#endif
			return true;

#if SUPPORT_MEMORY_DEBUG
		case kFskFileInstrMsgInUse:
			snprintf(buffer, bufferSize, "in use %d, seed=%lu", (int)msgData, gMemoryDebugSeed);
			return true;

		case kFskFileInstrMsgFreeInvalid:
			snprintf(buffer, bufferSize, "dispose unknown block: %p", msgData);
			return true;

		case kFskFileInstrMsgBlockInvalid:
			snprintf(buffer, bufferSize, "block trailer trashed on %p", msgData);
			return true;
#endif
	}

	return false;
}

#endif
