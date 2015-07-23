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
#include "FskMemory.h"
#include "Kpl.h"

#include <pthread.h>
#include <semaphore.h>

#include "KplSynchronization.h"

struct KplMutexRecord {
	pthread_mutex_t mutex;
};

struct KplSemaphoreRecord {
	sem_t hSem;
};

struct KplConditionRecord {
	pthread_cond_t	cond;
};


FskErr KplMutexNew(KplMutex *mutex)
{
	FskErr err;
	pthread_mutexattr_t   attr;
	err = FskMemPtrNewClear(sizeof(KplMutexRecord), (FskMemPtr *)mutex);
	if (err) goto bail;

	if ((pthread_mutexattr_init(&attr) != 0) ||
		(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) ||
		(pthread_mutex_init(&(*mutex)->mutex, &attr) != 0)) {
		err = kFskErrOperationFailed;
		goto bail;
	}


bail:
	pthread_mutexattr_destroy(&attr);

	if ((err != kFskErrNone) && (*mutex != NULL)) {
		FskMemPtrDispose(*mutex);
		*mutex = NULL;
	}

	return err;
}

FskErr KplMutexDispose(KplMutex mutex)
{
	if (mutex) {
		pthread_mutex_destroy(&mutex->mutex);
		FskMemPtrDispose(mutex);
	}

	return kFskErrNone;
}

FskErr KplMutexAcquire(KplMutex mutex)
{
	pthread_mutex_lock(&mutex->mutex);
	return kFskErrNone;
}

FskErr KplMutexTrylock(KplMutex mutex)
{
	if(0 == pthread_mutex_trylock(&mutex->mutex))
		return kFskErrNone;
	else
		return kFskErrOperationFailed;
}

FskErr KplMutexRelease(KplMutex mutex)
{
	pthread_mutex_unlock(&mutex->mutex);
	return kFskErrNone;
}


/*
	Semaphore
*/


FskErr KplSemaphoreNew(KplSemaphore *sem, UInt32 value)
{
	FskErr err;

	err = FskMemPtrNewClear(sizeof(KplSemaphoreRecord), (FskMemPtr *)sem);
	if (err) goto bail;

	if (sem_init(&(*sem)->hSem, 0, value) != 0) {
		err = kFskErrOperationFailed;
		goto bail;
	}

bail:
	if ((err != kFskErrNone) && (*sem != NULL)) {
		FskMemPtrDispose(*sem);
		*sem = NULL;
	}

	return err;
}

FskErr KplSemaphoreDispose(KplSemaphore sem)
{
	if (sem) {
		sem_destroy(&sem->hSem);
		FskMemPtrDispose(sem);
	}
	return kFskErrNone;
}

FskErr KplSemaphoreAcquire(KplSemaphore sem)
{
	sem_wait(&sem->hSem);
	return kFskErrNone;
}

FskErr KplSemaphoreRelease(KplSemaphore sem)
{
	sem_post(&sem->hSem);
	return kFskErrNone;
}

FskErr KplConditionNew(KplCondition *conditionOut)
{
    FskErr err;
    KplCondition condition = NULL;

    err = FskMemPtrNewClear(sizeof(KplConditionRecord), (FskMemPtr *)&condition);
    if (err) goto bail;
	pthread_cond_init(&condition->cond, NULL);

bail:
    if (err) {
        KplConditionDispose(condition);
        condition = NULL;
    }
    *conditionOut = condition;

    return err;
}

FskErr KplConditionDispose(KplCondition condition)
{
    if (NULL == condition)
        return kFskErrNone;

    pthread_cond_destroy(&condition->cond);
    FskMemPtrDispose(condition);

    return kFskErrNone;
}

FskErr KplConditionWait(KplCondition condition, KplMutex mutex)
{
    pthread_cond_wait(&condition->cond, &mutex->mutex);
    return kFskErrNone;
}

# define TIMEVAL_TO_TIMESPEC(tv, ts) {                                   \
    (ts)->tv_sec = (tv)->tv_sec;                                    \
    (ts)->tv_nsec = (tv)->tv_usec * 1000;                           \
}
# define TIMESPEC_TO_TIMEVAL(tv, ts) {                                   \
    (tv)->tv_sec = (ts)->tv_sec;                                    \
    (tv)->tv_usec = (ts)->tv_nsec / 1000;                           \
}
#define kNsecPerUsec 1000
#define kNsecPerSec (1000 * 1000 * 1000)

FskErr KplConditionTimedWait(KplCondition condition, KplMutex mutex, KplTime timeout)
{
	struct timespec sleep;
	struct timeval day;
	gettimeofday(&day, NULL);
	TIMEVAL_TO_TIMESPEC(&day, &sleep);
	sleep.tv_nsec += timeout->useconds * kNsecPerUsec;
	sleep.tv_sec += timeout->seconds + sleep.tv_nsec / kNsecPerSec;
	sleep.tv_nsec %= kNsecPerSec;

	if (ETIMEDOUT == pthread_cond_timedwait(&condition->cond, &mutex->mutex, &sleep)) {
//		fprintf(stderr, "condition awoken - timed out\n");
	}
}


FskErr KplConditionSignal(KplCondition condition)
{
    pthread_cond_signal(&condition->cond);
	return kFskErrNone;
}



