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
#define _WIN32_WINNT 0x0400

#if TARGET_OS_LINUX
	#include "poll.h"
#endif

#include "Fsk.h"
#include "FskPlatformImplementation.h"
#include "FskMemory.h"
#include "FskString.h"
#include "FskSynchronization.h"

#if TARGET_OS_KPL
	#include "KplSynchronization.h"
#endif

#if SUPPORT_INSTRUMENTATION

static Boolean doFormatMessageSynchronization(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

static FskInstrumentedTypeRecord gFskMutexTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"mutex",
	FskInstrumentationOffset(FskMutexRecord),
	NULL,
	0,
	NULL,
	doFormatMessageSynchronization
};

static FskInstrumentedTypeRecord gFskSemaphoreTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"semaphore",
	FskInstrumentationOffset(FskSemaphoreRecord),
	NULL,
	0,
	NULL,
	doFormatMessageSynchronization
};

#endif

/*
	Mutex
*/

#if TARGET_OS_WIN32

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexNew_(FskMutex *mutex, const char *name)
#else
FskErr FskMutexNew_(FskMutex *mutex, const char *name, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	FskErr err;

	err = FskMemPtrNewClear(sizeof(FskMutexRecord), (FskMemPtr *)mutex);
	BAIL_IF_ERR(err);

	InitializeCriticalSection(&(*mutex)->cs);

#if SUPPORT_INSTRUMENTATION
	(*mutex)->name = FskStrDoCopy_Untracked(name);
	FskInstrumentedItemNew(*mutex, (*mutex)->name, &gFskMutexTypeInstrumentation);

#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(*mutex)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.name = name;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(*mutex, kFskSynchronizationInstrMsgMutexNew, &msg);
	}
#endif
#endif

bail:
	if ((err != kFskErrNone) && (*mutex != NULL)) {
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose_Untracked((*mutex)->name);
#endif
		FskMemPtrDispose(*mutex);
		*mutex = NULL;
	}

	return err;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexDispose_(FskMutex mutex)
#else
FskErr FskMutexDispose_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	if (mutex) {
#if SUPPORT_INSTRUMENTATION && SUPPORT_SYNCHRONIZATION_DEBUG
		if (FskInstrumentedItemHasListeners(mutex)) {
			FskSynchronizationInstrMsgRecord msg;
			msg.name = mutex->name;
			msg.file = file;
			msg.line = line;
			msg.function = function;
			FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexDispose, &msg);
		}
#endif
		DeleteCriticalSection(&mutex->cs);
		FskInstrumentedItemDispose(mutex);
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose_Untracked(mutex->name);
#endif
		FskMemPtrDispose(mutex);
	}

	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexAcquire_(FskMutex mutex)
#else
FskErr FskMutexAcquire_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(mutex)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.name = mutex->name;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexAcquire, &msg);
	}
#else
	FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexAcquire, mutex);
#endif
#endif
	EnterCriticalSection(&mutex->cs);
	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexRelease_(FskMutex mutex)
#else
FskErr FskMutexRelease_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(mutex)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.name = mutex->name;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexRelease, &msg);
	}
#else
	FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexRelease, mutex);
#endif
#endif
	LeaveCriticalSection(&mutex->cs);
	return kFskErrNone;
}

FskErr FskMutexNew_uninstrumented(FskMutex *mutex, const char *name)
{
	FskErr err;

	err = FskMemPtrNewClear(sizeof(FskMutexRecord), (FskMemPtr *)mutex);
	BAIL_IF_ERR(err);

	InitializeCriticalSection(&(*mutex)->cs);

#if SUPPORT_INSTRUMENTATION
	(*mutex)->name = FskStrDoCopy_Untracked(name);
#endif

bail:
	if ((err != kFskErrNone) && (*mutex != NULL)) {
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose((*mutex)->name);
#endif
		FskMemPtrDispose(*mutex);
		*mutex = NULL;
	}
	return err;
}

FskErr FskMutexDispose_uninstrumented(FskMutex mutex)
{
	if (mutex) {
		DeleteCriticalSection(&mutex->cs);
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose(mutex->name);
#endif
		FskMemPtrDispose(mutex);
	}

	return kFskErrNone;
}

FskErr FskMutexAcquire_uninstrumented(FskMutex mutex)
{
	EnterCriticalSection(&mutex->cs);
	return kFskErrNone;
}

FskErr FskMutexRelease_uninstrumented(FskMutex mutex)
{
	LeaveCriticalSection(&mutex->cs);
	return kFskErrNone;
}

#elif TARGET_OS_LINUX || TARGET_OS_MAC

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexNew_(FskMutex *mutex, const char *name)
#else
FskErr FskMutexNew_(FskMutex *mutex, const char *name, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	FskErr err;
	pthread_mutexattr_t   attr;
	err = FskMemPtrNewClear(sizeof(FskMutexRecord), (FskMemPtr *)mutex);
	BAIL_IF_ERR(err);

	if ((pthread_mutexattr_init(&attr) != 0) ||
		(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) ||
		(pthread_mutex_init(&(*mutex)->mutex, &attr) != 0)) {
		BAIL(kFskErrOperationFailed);
	}

#if SUPPORT_INSTRUMENTATION
	(*mutex)->name = FskStrDoCopy_Untracked(name);
	FskInstrumentedItemNew(*mutex, (*mutex)->name, &gFskMutexTypeInstrumentation);

#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(*mutex)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.name = name;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(*mutex, kFskSynchronizationInstrMsgMutexNew, &msg);
	}
#endif
#endif

bail:
	pthread_mutexattr_destroy(&attr);

	if ((err != kFskErrNone) && (*mutex != NULL)) {
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose_Untracked((FskMemPtr)(*mutex)->name);
#endif
		FskMemPtrDispose(*mutex);
		*mutex = NULL;
	}

	return err;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexDispose_(FskMutex mutex)
#else
FskErr FskMutexDispose_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	if (mutex) {
#if SUPPORT_INSTRUMENTATION && SUPPORT_SYNCHRONIZATION_DEBUG
		if (FskInstrumentedItemHasListeners(mutex)) {
			FskSynchronizationInstrMsgRecord msg;
			msg.name = mutex->name;
			msg.file = file;
			msg.line = line;
			msg.function = function;
			FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexDispose, &msg);
		}
#endif
		pthread_mutex_destroy(&mutex->mutex);
		FskInstrumentedItemDispose(mutex);
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose_Untracked((FskMemPtr)mutex->name);
#endif
		FskMemPtrDispose(mutex);
	}

	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexAcquire_(FskMutex mutex)
#else
FskErr FskMutexAcquire_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(mutex)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.name = mutex->name;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexAcquire, &msg);
	}
#else
	FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexAcquire, mutex);
#endif
#endif
	pthread_mutex_lock(&mutex->mutex);
	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
UInt32 FskMutexTrylock_(FskMutex mutex)
#else
UInt32 FskMutexTrylock_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
	int ret;
#if SUPPORT_SYNCHRONIZATION_DEBUG
	FskSynchronizationInstrMsgRecord msg;
	if (FskInstrumentedItemHasListeners(mutex)) {
		msg.name = mutex->name;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylock, &msg);
	}
#else
	FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylock, mutex);
#endif
	ret = pthread_mutex_trylock(&mutex->mutex);
	if (0 == ret) {
#if SUPPORT_SYNCHRONIZATION_DEBUG
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylockSucceeded, &msg);
#else
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylockSucceeded, mutex);
#endif
	}
	else {
#if SUPPORT_SYNCHRONIZATION_DEBUG
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylockFailed, &msg);
#else
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylockFailed, mutex);
#endif
	}
	return ret;
#else
	return pthread_mutex_trylock(&mutex->mutex);
#endif
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexRelease_(FskMutex mutex)
#else
FskErr FskMutexRelease_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(mutex)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.name = mutex->name;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexRelease, &msg);
	}
#else
	FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexRelease, mutex);
#endif
#endif
	pthread_mutex_unlock(&mutex->mutex);
	return kFskErrNone;
}

FskErr FskMutexNew_uninstrumented(FskMutex *mutex, const char *name)
{
	FskErr err;
	pthread_mutexattr_t   attr;
	err = FskMemPtrNewClear_Untracked(sizeof(FskMutexRecord), (FskMemPtr *)mutex);
	BAIL_IF_ERR(err);

	if ((pthread_mutexattr_init(&attr) != 0) ||
		(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) ||
		(pthread_mutex_init(&(*mutex)->mutex, &attr) != 0)) {
		BAIL(kFskErrOperationFailed);
	}

#if SUPPORT_INSTRUMENTATION
	(*mutex)->name = FskStrDoCopy_Untracked(name);
#endif

bail:
	pthread_mutexattr_destroy(&attr);

	if ((err != kFskErrNone) && (*mutex != NULL)) {
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose_Untracked((FskMemPtr)(*mutex)->name);
#endif
		FskMemPtrDispose_Untracked(*mutex);
		*mutex = NULL;
	}

	return err;
}

FskErr FskMutexDispose_uninstrumented(FskMutex mutex)
{
	if (mutex) {
		pthread_mutex_destroy(&mutex->mutex);
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose_Untracked((FskMemPtr)mutex->name);
#endif
		FskMemPtrDispose_Untracked(mutex);
	}

	return kFskErrNone;
}

FskErr FskMutexAcquire_uninstrumented(FskMutex mutex)
{
	pthread_mutex_lock(&mutex->mutex);
	return kFskErrNone;
}

FskErr FskMutexRelease_uninstrumented(FskMutex mutex)
{
	pthread_mutex_unlock(&mutex->mutex);
	return kFskErrNone;
}

#elif TARGET_OS_KPL

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexNew_(FskMutex *mutex, const char *name)
#else
FskErr FskMutexNew_(FskMutex *mutex, const char *name, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	FskMutex mtx = NULL;
	FskErr err;

	err = FskMemPtrNewClear(sizeof(FskMutexRecord), (FskMemPtr *)&mtx);
	BAIL_IF_ERR(err);

	err = KplMutexNew((KplMutex*)&mtx->kplMutex);
	BAIL_IF_ERR(err);

#if SUPPORT_INSTRUMENTATION
	mtx->name = FskStrDoCopy_Untracked(name);
	FskInstrumentedItemNew(mtx, mtx->name, &gFskMutexTypeInstrumentation);

#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(mtx)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.name = name;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(mtx, kFskSynchronizationInstrMsgMutexNew, &msg);
	}
#endif
#endif

bail:
	if ((err != kFskErrNone) && (mtx != NULL)) {
		KplMutexDispose(mtx->kplMutex);

#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose_Untracked((void*)mtx->name);
#endif
		FskMemPtrDispose(mtx);
		mtx = NULL;
	}

	*mutex = mtx;

	return err;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexDispose_(FskMutex mutex)
#else
FskErr FskMutexDispose_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	if (mutex) {
#if SUPPORT_INSTRUMENTATION && SUPPORT_SYNCHRONIZATION_DEBUG
		if (FskInstrumentedItemHasListeners(mutex)) {
			FskSynchronizationInstrMsgRecord msg;
			msg.name = mutex->name;
			msg.file = file;
			msg.line = line;
			msg.function = function;
			FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexDispose, &msg);
		}
#endif
		KplMutexDispose(mutex->kplMutex);
		FskInstrumentedItemDispose(mutex);
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose_Untracked((void*)mutex->name);
#endif
		FskMemPtrDispose(mutex);
	}

	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexAcquire_(FskMutex mutex)
#else
FskErr FskMutexAcquire_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(mutex)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.name = mutex->name;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexAcquire, &msg);
	}
#else
	FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexAcquire, mutex);
#endif
#endif
	KplMutexAcquire(mutex->kplMutex);
	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
UInt32 FskMutexTrylock_(FskMutex mutex)
#else
UInt32 FskMutexTrylock_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
	int ret;
#if SUPPORT_SYNCHRONIZATION_DEBUG
	FskSynchronizationInstrMsgRecord msg;
	if (FskInstrumentedItemHasListeners(mutex)) {
		msg.name = mutex->name;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylock, &msg);
	}
#else
	FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylock, mutex);
#endif
	ret = KplMutexTrylock(mutex->kplMutex);
	if (0 == ret) {
#if SUPPORT_SYNCHRONIZATION_DEBUG
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylockSucceeded, &msg);
#else
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylockSucceeded, mutex);
#endif
	}
	else {
#if SUPPORT_SYNCHRONIZATION_DEBUG
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylockFailed, &msg);
#else
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexTrylockFailed, mutex);
#endif
	}
	return ret;

#else

	return KplMutexTrylock(mutex->kplMutex);
#endif
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexRelease_(FskMutex mutex)
#else
FskErr FskMutexRelease_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(mutex)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.name = mutex->name;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexRelease, &msg);
	}
#else
	FskInstrumentedItemSendMessage(mutex, kFskSynchronizationInstrMsgMutexRelease, mutex);
#endif
#endif
	KplMutexRelease(mutex->kplMutex);
	return kFskErrNone;
}

FskErr FskMutexNew_uninstrumented(FskMutex *mutex, const char *name)
{
	FskMutex mtx = NULL;
	FskErr err;

	err = FskMemPtrNewClear(sizeof(FskMutexRecord), (FskMemPtr *)&mtx);
	BAIL_IF_ERR(err);

	err = KplMutexNew((KplMutex*)&mtx->kplMutex);
	BAIL_IF_ERR(err);

#if SUPPORT_INSTRUMENTATION
	mtx->name = FskStrDoCopy_Untracked(name);
#endif

bail:
	if ((err != kFskErrNone) && (mtx != NULL)) {
		KplMutexDispose(mtx->kplMutex);
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose((void*)mtx->name);
#endif
		FskMemPtrDispose(mtx);
		mtx = NULL;
	}
	*mutex = mtx;

	return err;
}

FskErr FskMutexDispose_uninstrumented(FskMutex mutex)
{
	if (mutex) {
		KplMutexDispose(mutex->kplMutex);
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose((void*)mutex->name);
#endif
		FskMemPtrDispose(mutex);
	}

	return kFskErrNone;
}

FskErr FskMutexAcquire_uninstrumented(FskMutex mutex)
{
	KplMutexAcquire(mutex->kplMutex);
	return kFskErrNone;
}

FskErr FskMutexRelease_uninstrumented(FskMutex mutex)
{
	KplMutexRelease(mutex->kplMutex);
	return kFskErrNone;
}

#else

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexNew_(FskMutex *mutex, const char *name)
#else
FskErr FskMutexNew_(FskMutex *mutex, const char *name, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	return FskMemPtrNewClear(sizeof(UInt32), (FskMemPtr *)mutex);
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexDispose_(FskMutex mutex)
#else
FskErr FskMutexDispose_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	FskMemPtrDispose(mutex);
	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexAcquire_(FskMutex mutex)
#else
FskErr FskMutexAcquire_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	UInt32 *value = (UInt32 *)mutex;
	if (0 == *value) {
		*value += 1;
		return kFskErrNone;
	}
	else
		return -1;		// can't block on a mutex in single threaded environment!
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskMutexRelease_(FskMutex mutex)
#else
FskErr FskMutexRelease_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	UInt32 *value = (UInt32 *)mutex;
	if (1 == *value) {
		*value -= 1;
		return kFskErrNone;
	}
	else
		return -1;		// huh??
}

#endif

/*
	Semaphore
*/

#if TARGET_OS_WIN32

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreNew_(FskSemaphore *sem, UInt32 value)
#else
FskErr FskSemaphoreNew_(FskSemaphore *sem, UInt32 value, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	FskErr err;

	err = FskMemPtrNewClear(sizeof(FskSemaphoreRecord), (FskMemPtr *)sem);
	BAIL_IF_ERR(err);

	if (((*sem)->hSem = CreateSemaphore(NULL, value, 0x7fffffff, NULL)) == NULL) {
		BAIL(kFskErrOperationFailed);
	}

	FskInstrumentedItemNew(*sem, NULL, &gFskSemaphoreTypeInstrumentation);
#if SUPPORT_INSTRUMENTATION && SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(*sem)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(*sem, kFskSynchronizationInstrMsgSemaphoreNew, &msg);
	}
#endif

bail:
	if ((err != kFskErrNone) && (*sem != NULL)) {
		FskMemPtrDispose(*sem);
		*sem = NULL;
	}

	return err;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreDispose_(FskSemaphore sem)
#else
FskErr FskSemaphoreDispose_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	if (sem) {
#if SUPPORT_INSTRUMENTATION && SUPPORT_SYNCHRONIZATION_DEBUG
		if (FskInstrumentedItemHasListeners(sem)) {
			FskSynchronizationInstrMsgRecord msg;
			msg.file = file;
			msg.line = line;
			msg.function = function;
			FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreDispose, &msg);
		}
#endif
		CloseHandle(sem->hSem);
		FskInstrumentedItemDispose(sem);
		FskMemPtrDispose(sem);
	}
	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreAcquire_(FskSemaphore sem)
#else
FskErr FskSemaphoreAcquire_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(sem)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreAcquire, &msg);
	}
#else
	FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreAcquire, sem);
#endif
#endif
	WaitForSingleObject(sem->hSem, INFINITE);
	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreRelease_(FskSemaphore sem)
#else
FskErr FskSemaphoreRelease_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(sem)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreRelease, &msg);
	}
#else
	FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreRelease, sem);
#endif
#endif
	ReleaseSemaphore(sem->hSem, 1, NULL);
	return kFskErrNone;
}

#elif TARGET_OS_LINUX || TARGET_OS_MAC

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreNew_(FskSemaphore *sem, UInt32 value)
#else
FskErr FskSemaphoreNew_(FskSemaphore *sem, UInt32 value, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	FskErr err;

	err = FskMemPtrNewClear(sizeof(FskSemaphoreRecord), (FskMemPtr *)sem);
	BAIL_IF_ERR(err);

#if TARGET_OS_MAC
	{
		FskSemaphore s = *sem;
		snprintf(s->name, sizeof(s->name)-1, "/tmp/sem%lx", (unsigned long)s);
		if ((s->hSem = sem_open(s->name, O_CREAT|O_EXCL, 666, value)) == NULL) {
			BAIL(kFskErrOperationFailed);
		}
	}
#else
	if (sem_init(&(*sem)->hSem, 0, value) != 0) {
		BAIL(kFskErrOperationFailed);
	}
#endif

	FskInstrumentedItemNew(*sem, NULL, &gFskSemaphoreTypeInstrumentation);
#if SUPPORT_INSTRUMENTATION && SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(*sem)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(*sem, kFskSynchronizationInstrMsgSemaphoreNew, &msg);
	}
#endif

bail:
	if ((err != kFskErrNone) && (*sem != NULL)) {
		FskMemPtrDispose(*sem);
		*sem = NULL;
	}

	return err;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreDispose_(FskSemaphore sem)
#else
FskErr FskSemaphoreDispose_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	if (sem) {
#if SUPPORT_INSTRUMENTATION && SUPPORT_SYNCHRONIZATION_DEBUG
		if (FskInstrumentedItemHasListeners(sem)) {
			FskSynchronizationInstrMsgRecord msg;
			msg.file = file;
			msg.line = line;
			msg.function = function;
			FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreDispose, &msg);
		}
#endif
#if TARGET_OS_MAC
		sem_close(sem->hSem);
		sem_unlink(sem->name);
#else
		sem_destroy(&sem->hSem);
#endif
		FskInstrumentedItemDispose(sem);
		FskMemPtrDispose(sem);
	}
	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreAcquire_(FskSemaphore sem)
#else
FskErr FskSemaphoreAcquire_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(sem)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreAcquire, &msg);
	}
#else
	FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreAcquire, sem);
#endif
#endif
#if TARGET_OS_MAC
	sem_wait(sem->hSem);
#else
	sem_wait(&sem->hSem);
#endif
	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreRelease_(FskSemaphore sem)
#else
FskErr FskSemaphoreRelease_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(sem)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreRelease, &msg);
	}
#else
	FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreRelease, sem);
#endif
#endif
#if TARGET_OS_MAC
	sem_post(sem->hSem);
#else
	sem_post(&sem->hSem);
#endif
	return kFskErrNone;
}

#elif TARGET_OS_KPL

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreNew_(FskSemaphore *sem, UInt32 value)
#else
FskErr FskSemaphoreNew_(FskSemaphore *sem, UInt32 value, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	FskErr err;

	err = FskMemPtrNewClear(sizeof(FskSemaphoreRecord), (FskMemPtr *)sem);
	BAIL_IF_ERR(err);

	err = KplSemaphoreNew((KplSemaphore*)&(*sem)->kplSem, value);
	BAIL_IF_ERR(err);

	FskInstrumentedItemNew(*sem, NULL, &gFskSemaphoreTypeInstrumentation);
#if SUPPORT_INSTRUMENTATION && SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(*sem)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(*sem, kFskSynchronizationInstrMsgSemaphoreNew, &msg);
	}
#endif

bail:
	if ((err != kFskErrNone) && (*sem != NULL)) {
		KplSemaphoreDispose((*sem)->kplSem);
		FskMemPtrDispose(*sem);
		*sem = NULL;
	}

	return err;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreDispose_(FskSemaphore sem)
#else
FskErr FskSemaphoreDispose_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
	if (sem) {
#if SUPPORT_INSTRUMENTATION && SUPPORT_SYNCHRONIZATION_DEBUG
		if (FskInstrumentedItemHasListeners(sem)) {
			FskSynchronizationInstrMsgRecord msg;
			msg.file = file;
			msg.line = line;
			msg.function = function;
			FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreDispose, &msg);
		}
#endif
		KplSemaphoreDispose(sem->kplSem);
		FskInstrumentedItemDispose(sem);
		FskMemPtrDispose(sem);
	}
	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreAcquire_(FskSemaphore sem)
#else
FskErr FskSemaphoreAcquire_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(sem)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreAcquire, &msg);
	}
#else
	FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreAcquire, sem);
#endif
#endif
	KplSemaphoreAcquire(sem->kplSem);
	return kFskErrNone;
}

#if !SUPPORT_SYNCHRONIZATION_DEBUG
FskErr FskSemaphoreRelease_(FskSemaphore sem)
#else
FskErr FskSemaphoreRelease_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS)
#endif
{
#if SUPPORT_INSTRUMENTATION
#if SUPPORT_SYNCHRONIZATION_DEBUG
	if (FskInstrumentedItemHasListeners(sem)) {
		FskSynchronizationInstrMsgRecord msg;
		msg.file = file;
		msg.line = line;
		msg.function = function;
		FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreRelease, &msg);
	}
#else
	FskInstrumentedItemSendMessage(sem, kFskSynchronizationInstrMsgSemaphoreRelease, sem);
#endif
#endif
	KplSemaphoreRelease(sem->kplSem);
	return kFskErrNone;
}
#endif

#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageSynchronization(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize) {
#if SUPPORT_SYNCHRONIZATION_DEBUG
	FskSynchronizationInstrMsgRecord *msgRec = (FskSynchronizationInstrMsgRecord *)msgData;
	#define MOREFMT		"[%s] %s:%d %s - "
	#define MOREPARAMS	, msgRec->name, msgRec->file, msgRec->line, msgRec->function
	#define SEMMOREFMT		"%s:%d %s - "
	#define SEMMOREPARAMS	, msgRec->file, msgRec->line, msgRec->function
#else
	FskMutex mutex = (FskMutex)msgData;
	#define MOREFMT		"[%s] "
	#define MOREPARAMS	, mutex->name
	#define SEMMOREFMT
	#define SEMMOREPARAMS
#endif

	switch (msg) {
		case kFskSynchronizationInstrMsgMutexNew:
			snprintf(buffer, bufferSize, MOREFMT "New Mutex" MOREPARAMS);
			return true;
		case kFskSynchronizationInstrMsgMutexDispose:
			snprintf(buffer, bufferSize, MOREFMT "Dispose Mutex" MOREPARAMS);
			return true;
		case kFskSynchronizationInstrMsgMutexAcquire:
			snprintf(buffer, bufferSize, MOREFMT "Acquire Mutex" MOREPARAMS);
			return true;
		case kFskSynchronizationInstrMsgMutexRelease:
			snprintf(buffer, bufferSize, MOREFMT "Release Mutex" MOREPARAMS);
			return true;
		case kFskSynchronizationInstrMsgMutexTrylock:
			snprintf(buffer, bufferSize, MOREFMT "Trylock Mutex" MOREPARAMS);
			return true;
		case kFskSynchronizationInstrMsgMutexTrylockSucceeded:
			snprintf(buffer, bufferSize, MOREFMT "Trylock Mutex Succeeded" MOREPARAMS);
			return true;
		case kFskSynchronizationInstrMsgMutexTrylockFailed:
			snprintf(buffer, bufferSize, MOREFMT "Trylock Mutex Failed" MOREPARAMS);
			return true;
		case kFskSynchronizationInstrMsgSemaphoreNew:
			snprintf(buffer, bufferSize, SEMMOREFMT "New Semaphore" SEMMOREPARAMS);
			return true;
		case kFskSynchronizationInstrMsgSemaphoreDispose:
			snprintf(buffer, bufferSize, SEMMOREFMT "Dispose Semaphore" SEMMOREPARAMS);
			return true;
		case kFskSynchronizationInstrMsgSemaphoreAcquire:
			snprintf(buffer, bufferSize, SEMMOREFMT "Acquire Semaphore" SEMMOREPARAMS);
			return true;
		case kFskSynchronizationInstrMsgSemaphoreRelease:
			snprintf(buffer, bufferSize, SEMMOREFMT "Release Semaphore" SEMMOREPARAMS);
			return true;
	}
	return false;
}

#endif

