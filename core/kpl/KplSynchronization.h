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
#ifndef __KPL_SYNCHRONIZATION_H__
#define __KPL_SYNCHRONIZATION_H__

#include "FskErrors.h"
#include "KplTime.h"

#ifdef __cplusplus
extern "C" {
#endif

KplDeclarePrivateType(KplMutex)
KplDeclarePrivateType(KplSemaphore)
KplDeclarePrivateType(KplCondition)

FskErr KplMutexNew(KplMutex *mutex);
FskErr KplMutexDispose(KplMutex mutex);
FskErr KplMutexAcquire(KplMutex mutex);
FskErr KplMutexRelease(KplMutex mutex);
FskErr KplMutexTrylock(KplMutex mutex);

FskErr KplSemaphoreNew(KplSemaphore *sem, UInt32 value);
FskErr KplSemaphoreDispose(KplSemaphore sem);

FskErr KplSemaphoreAcquire(KplSemaphore sem);
FskErr KplSemaphoreRelease(KplSemaphore sem);

FskErr KplConditionNew(KplCondition *condition);
FskErr KplConditionDispose(KplCondition condition);

FskErr KplConditionWait(KplCondition condition, KplMutex mutex);
FskErr KplConditionTimedWait(KplCondition condition, KplMutex mutex, KplTime timeout);
FskErr KplConditionSignal(KplCondition condition);

#ifdef __cplusplus
}
#endif

#endif // __KPL_SYNCHRONIZATION_H__
