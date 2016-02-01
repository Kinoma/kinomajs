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
#include "Fsk.h"
#include "FskMemory.h"
#include "KplSynchronization.h"

#include <windows.h>

struct KplMutexRecord {
	CRITICAL_SECTION	cs;
};

struct KplSemaphoreRecord {
	HANDLE				hSem;
};

struct KplConditionRecord {
	HANDLE				semaphore;
	DWORD				waitersCount; 
	HANDLE				waitersEvent;
	WORD				waitersFlag;
	CRITICAL_SECTION	waitersMutex;
};

FskErr KplMutexNew(KplMutex *mutex)
{	
	FskErr	err;
	KplMutex kplMutex = NULL;

	err = FskMemPtrNew(sizeof(KplMutexRecord), (FskMemPtr*)&kplMutex);
	if (err) goto bail;
	
	InitializeCriticalSection(&kplMutex->cs);
	
bail:
	if (err) {
		FskMemPtrDispose(kplMutex);
		kplMutex = NULL;
	}
	
	*mutex = kplMutex;
	
	return err;
}

FskErr KplMutexDispose(KplMutex mutex)
{
	if (mutex) {
		DeleteCriticalSection(&mutex->cs);
		FskMemPtrDispose(mutex);
	}
	return kFskErrNone;
}

FskErr KplMutexAcquire(KplMutex mutex)
{
	EnterCriticalSection(&mutex->cs);
	
	return kFskErrNone;
}

FskErr KplMutexRelease(KplMutex mutex)
{
	LeaveCriticalSection(&mutex->cs);
	
	return kFskErrNone;
}

FskErr KplMutexTrylock(KplMutex mutex)
{
	return kFskErrUnimplemented;	// @@ TBD
}

FskErr KplSemaphoreNew(KplSemaphore *sem, UInt32 value)
{
	FskErr	err;
	KplSemaphore kplSemaphore = NULL;

	err = FskMemPtrNew(sizeof(KplSemaphoreRecord), (FskMemPtr*)&kplSemaphore);
	if (err) goto bail;

	if ((kplSemaphore->hSem = CreateSemaphore(NULL, value, 0x7fffffff, NULL)) == NULL) {
		err = kFskErrOperationFailed;
		goto bail;
	}
	
bail:
	if (err) {
		FskMemPtrDispose(kplSemaphore);
		kplSemaphore = NULL;
	}
	
	*sem = kplSemaphore;

	return err;
}

FskErr KplSemaphoreDispose(KplSemaphore sem)
{
	if (sem) {
		CloseHandle(sem->hSem);
		FskMemPtrDispose(sem);
	}
	return kFskErrNone;
}

FskErr KplSemaphoreAcquire(KplSemaphore sem)
{
	WaitForSingleObject(sem->hSem, INFINITE);
	return kFskErrNone;
}

FskErr KplSemaphoreRelease(KplSemaphore sem)
{
	ReleaseSemaphore(sem->hSem, 1, NULL);
	return kFskErrNone;
}

FskErr KplConditionNew(KplCondition *condition)
{
	FskErr	err;
	KplCondition kplCondition = NULL;

	err = FskMemPtrNewClear(sizeof(KplConditionRecord), (FskMemPtr*)&kplCondition);
	if (err) goto bail;

	kplCondition->semaphore = CreateSemaphore(0, 1, 0x7FFFFFF, NULL);
	if ((HANDLE)ERROR_INVALID_HANDLE == kplCondition->semaphore) {
		kplCondition->semaphore = NULL;
		err = -1;
		goto bail;
	}

	kplCondition->waitersEvent = CreateEvent(0, false, false, NULL);
	if ((HANDLE)ERROR_INVALID_HANDLE == kplCondition->waitersEvent) {
		kplCondition->waitersEvent = NULL;
		err = -1;
		goto bail;
	}

	InitializeCriticalSection(&kplCondition->waitersMutex);
	
bail:
	if (err) {
		KplConditionDispose(kplCondition);
		kplCondition = NULL;
	}
	
	*condition = kplCondition;
	
	return err;
}

FskErr KplConditionDispose(KplCondition condition)
{
	if (condition) {
		DeleteCriticalSection(&condition->waitersMutex);
		if (NULL != condition->waitersEvent) 
			CloseHandle(condition->waitersEvent);
		if (NULL != condition->semaphore) 
			CloseHandle(condition->semaphore);
		FskMemPtrDispose(condition);
	}
	return kFskErrNone;
}

FskErr KplConditionWait(KplCondition condition, KplMutex mutex)
{
	WORD aWaitersFlag = 0;
	
	EnterCriticalSection(&condition->waitersMutex);
	condition->waitersCount++;
	LeaveCriticalSection(&condition->waitersMutex);
	
	KplMutexRelease(mutex);
	WaitForSingleObject(condition->semaphore, INFINITE);
	
	EnterCriticalSection(&condition->waitersMutex);
	condition->waitersCount--;
	if (condition->waitersFlag && (0 == condition->waitersCount)) {
		condition->waitersFlag = 0;
		aWaitersFlag = 1;
	}
	LeaveCriticalSection(&condition->waitersMutex);
	if (aWaitersFlag)
		SetEvent(condition->waitersEvent);

	KplMutexAcquire(mutex);
	
	return kFskErrNone;
}

FskErr KplConditionSignal(KplCondition condition)
{
	EnterCriticalSection(&condition->waitersMutex);
	if (condition->waitersCount > 0)
		ReleaseSemaphore(condition->semaphore, 1, 0);
	LeaveCriticalSection(&condition->waitersMutex);
	
	return kFskErrNone;
}

