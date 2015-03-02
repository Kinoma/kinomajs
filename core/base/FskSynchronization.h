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
#ifndef __FSKSYNCHRONIZATION_H__
#define __FSKSYNCHRONIZATION_H__

#include "Fsk.h"

#if TARGET_OS_LINUX || TARGET_OS_MAC
	#include <stdio.h>
	#include <pthread.h>
	#include <semaphore.h>
#elif TARGET_OS_WIN32
	#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Mutex

#if TARGET_OS_WIN32
	typedef struct {
		CRITICAL_SECTION		cs;

#if SUPPORT_INSTRUMENTATION
		const char *name;
#endif
		FskInstrumentedItemDeclaration
	} FskMutexRecord, *FskMutex;
#elif TARGET_OS_LINUX || TARGET_OS_MAC
	typedef struct {
		pthread_mutex_t	mutex;
#if SUPPORT_INSTRUMENTATION
		const char *name;
#endif

		FskInstrumentedItemDeclaration
	} FskMutexRecord, *FskMutex;
#elif TARGET_OS_KPL
	typedef struct {
		void	*kplMutex;

#if SUPPORT_INSTRUMENTATION
		const char *name;
#endif
		FskInstrumentedItemDeclaration
	} FskMutexRecord, *FskMutex;
#else
	typedef UInt32 *FskMutex;
#endif


// Semaphore

#if TARGET_OS_WIN32
	typedef struct {
		HANDLE hSem;

		FskInstrumentedItemDeclaration
	} FskSemaphoreRecord, *FskSemaphore;
#elif TARGET_OS_LINUX
	typedef struct {
		sem_t	hSem;

		FskInstrumentedItemDeclaration
	} FskSemaphoreRecord, *FskSemaphore;
#elif TARGET_OS_MAC
	typedef struct {
	#if 1 || TARGET_OS_IPHONE
		sem_t			*hSem;
		char			name[80];
	#else
		MPSemaphoreID   hSem;		// sema_init isn't supported as of 10.3, maybe in the future...
	#endif
		// sem_t	hSem;

		FskInstrumentedItemDeclaration
	} FskSemaphoreRecord, *FskSemaphore;
#elif TARGET_OS_KPL
	typedef struct {
		void	*kplSem;

		FskInstrumentedItemDeclaration
	} FskSemaphoreRecord, *FskSemaphore;
#endif


#if !SUPPORT_SYNCHRONIZATION_DEBUG
#define FskMutexNew(mutex, name)	FskMutexNew_(mutex, name)
#define FskMutexDispose(mutex)	FskMutexDispose_(mutex)
#define FskMutexAcquire(mutex)	FskMutexAcquire_(mutex)
#define FskMutexRelease(mutex)	FskMutexRelease_(mutex)
#define FskMutexTrylock(mutex)	FskMutexTrylock_(mutex)

#define FskSemaphoreNew(sem, value)	FskSemaphoreNew_(sem, value)
#define FskSemaphoreDispose(sem)	FskSemaphoreDispose_(sem)
#define FskSemaphoreAcquire(sem)	FskSemaphoreAcquire_(sem)
#define FskSemaphoreRelease(sem)	FskSemaphoreRelease_(sem)

FskAPI(FskErr) FskMutexNew_(FskMutex *mutex, const char *name);
FskAPI(FskErr) FskMutexDispose_(FskMutex mutex);
FskAPI(FskErr) FskMutexAcquire_(FskMutex mutex);
FskAPI(FskErr) FskMutexRelease_(FskMutex mutex);
FskAPI(UInt32) FskMutexTrylock_(FskMutex mutex);

FskAPI(FskErr) FskSemaphoreNew_(FskSemaphore *sem, UInt32 value);
FskAPI(FskErr) FskSemaphoreDispose_(FskSemaphore sem);
FskAPI(FskErr) FskSemaphoreAcquire_(FskSemaphore sem);
FskAPI(FskErr) FskSemaphoreRelease_(FskSemaphore sem);

#else

#define FSK_SYNCHRONIZATION_DEBUG_ARGS const char *file, UInt32 line, const char *function
#define FSK_SYNCHRONIZATION_DEBUG_PARAMS __FILE__, __LINE__, __FUNCTION__
#define FSK_SYNCHRONIZATION_DEBUG_PARAMS_CONTINUE file, line, function

FskAPI(FskErr) FskMutexNew_(FskMutex *mutex, const char *name, FSK_SYNCHRONIZATION_DEBUG_ARGS);
FskAPI(FskErr) FskMutexDispose_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS);
FskAPI(FskErr) FskMutexAcquire_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS);
FskAPI(FskErr) FskMutexRelease_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS);
FskAPI(UInt32) FskMutexTrylock_(FskMutex mutex, FSK_SYNCHRONIZATION_DEBUG_ARGS);

FskAPI(FskErr) FskSemaphoreNew_(FskSemaphore *sem, UInt32 value, FSK_SYNCHRONIZATION_DEBUG_ARGS);
FskAPI(FskErr) FskSemaphoreDispose_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS);
FskAPI(FskErr) FskSemaphoreAcquire_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS);
FskAPI(FskErr) FskSemaphoreRelease_(FskSemaphore sem, FSK_SYNCHRONIZATION_DEBUG_ARGS);

#define FskMutexNew(mutex, name)	FskMutexNew_(mutex, name, FSK_SYNCHRONIZATION_DEBUG_PARAMS)
#define FskMutexDispose(mutex)	FskMutexDispose_(mutex, FSK_SYNCHRONIZATION_DEBUG_PARAMS)
#define FskMutexAcquire(mutex)	FskMutexAcquire_(mutex, FSK_SYNCHRONIZATION_DEBUG_PARAMS)
#define FskMutexRelease(mutex)	FskMutexRelease_(mutex, FSK_SYNCHRONIZATION_DEBUG_PARAMS)
#define FskMutexTrylock(mutex)	FskMutexTrylock_(mutex, FSK_SYNCHRONIZATION_DEBUG_PARAMS)

#define FskSemaphoreNew(sem, value)	FskSemaphoreNew_(sem, value, FSK_SYNCHRONIZATION_DEBUG_PARAMS)
#define FskSemaphoreDispose(sem)	FskSemaphoreDispose_(sem, FSK_SYNCHRONIZATION_DEBUG_PARAMS)
#define FskSemaphoreAcquire(sem)	FskSemaphoreAcquire_(sem, FSK_SYNCHRONIZATION_DEBUG_PARAMS)
#define FskSemaphoreRelease(sem)	FskSemaphoreRelease_(sem, FSK_SYNCHRONIZATION_DEBUG_PARAMS)

#endif


#if SUPPORT_INSTRUMENTATION
enum {
	kFskSynchronizationInstrMsgMutexNew = kFskInstrumentedItemFirstCustomMessage,
	kFskSynchronizationInstrMsgMutexDispose,
	kFskSynchronizationInstrMsgMutexAcquire,
	kFskSynchronizationInstrMsgMutexRelease,
	kFskSynchronizationInstrMsgMutexTrylock,
	kFskSynchronizationInstrMsgMutexTrylockSucceeded,
	kFskSynchronizationInstrMsgMutexTrylockFailed,

	kFskSynchronizationInstrMsgSemaphoreNew = kFskInstrumentedItemFirstCustomMessage + 1024,
	kFskSynchronizationInstrMsgSemaphoreDispose,
	kFskSynchronizationInstrMsgSemaphoreAcquire,
	kFskSynchronizationInstrMsgSemaphoreRelease,
};

#if SUPPORT_SYNCHRONIZATION_DEBUG
typedef struct {
	const char *name;
	const char *file;
	UInt32		line;
	const char *function;
} FskSynchronizationInstrMsgRecord;
#endif

#endif

FskErr FskMutexNew_uninstrumented(FskMutex *mutex, const char *name);
FskErr FskMutexDispose_uninstrumented(FskMutex mutex);
FskErr FskMutexAcquire_uninstrumented(FskMutex mutex);
FskErr FskMutexRelease_uninstrumented(FskMutex mutex);


#ifdef __cplusplus
	}
#endif

#endif	// __FSKSYNCHRONIZATION_H__

