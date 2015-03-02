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
#ifndef __KPL_THREAD_H__
#define __KPL_THREAD_H__

#include "Kpl.h"
#include "FskErrors.h"

#ifdef __cplusplus
extern "C" {
#endif

KplDeclarePrivateType(KplThread)

typedef void (*KplThreadProc)(void *refcon);
typedef void (*KplThreadCallback)(void *, void *, void *, void *);

FskErr KplThreadCreateMain(KplThread *kplThread);
FskErr KplThreadTerminateMain(void);

FskErr KplThreadCreate(KplThread *thread, KplThreadProc procedure, void *refcon, UInt32 flags, char *name);
FskErr KplThreadJoin(KplThread thread);
KplThread KplThreadGetCurrent(void);
void KplThreadYield(void);
void KplThreadWake(KplThread thread);

void KplThreadPostEvent(KplThread kplThread, void *event);

FskErr KplThreadRunloopCycle(SInt32 msTimeout);

FskErr FskKplThreadPostCallback(KplThread kplThread, KplThreadCallback function, void *arg1, void *arg2, void *arg3, void *arg4);

void KplThreadNotifyPendingSocketData(void *socket, Boolean pendingReadable, Boolean pendingWritable);

void KplThreadNotifyClientComplete(KplThread kplThread);

void *KplThreadGetRefcon(KplThread kplThread);

#ifdef __cplusplus
}
#endif

#endif // __KPL_THREAD_H__
