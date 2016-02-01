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
#include "Kpl.h"
#include "KplUIEvents.h"
#include "KplThread.h"
#include "FskMemory.h"

static KplUIEventCallback gUIEventCallback = 0L;
static void *gUIEventCallbackRefcon;
static KplThread gUIEventCallbackThread;

FskErr KplUIEventNew(KplUIEvent *event, UInt32 eventID, KplTime eventTime)
{
	FskErr err;
	KplUIEvent eventOut = 0L;
	KplTimeRecord now;
	
	if (!eventTime) {
		KplTimeGetNow(&now);
		eventTime = &now;
	}
	
	err = FskMemPtrNewClear(sizeof(KplUIEventRecord), (FskMemPtr*)&eventOut);
	if (err) goto bail;
	
	eventOut->eventID = eventID;
	eventOut->eventTime = *eventTime;
	
bail:
	if (err) {
		KplUIEventDispose(eventOut);
		eventOut = 0L;
	}
	*event = eventOut;
	
	return err;
}

FskErr KplUIEventDispose(KplUIEvent event)
{
	FskMemPtrDispose(event);
	return kFskErrNone;
}

static void KplUIEventThreadCallback(void *arg1, void *arg2, void *arg3, void *arg4)
{
	KplUIEvent event = (KplUIEvent)arg1;
	void *refcon = arg2;
	FskErr err;
	
	err = gUIEventCallback(event, refcon);
	
	// The callback returns the "kFskErrNeedMoreTime" error code when there are more platform events that need servicing ASAP.
	// We accomodate by sending a NULL event back to the queue, so that the platform gets another chance to service the queue. 
	if (kFskErrNeedMoreTime == err) {
		FskKplThreadPostCallback(gUIEventCallbackThread, KplUIEventThreadCallback, 0L, gUIEventCallbackRefcon, 0L, 0L);
	}
}

FskErr KplUIEventSend(KplUIEvent event)
{
	if (gUIEventCallback) {
		FskKplThreadPostCallback(gUIEventCallbackThread, KplUIEventThreadCallback, event, gUIEventCallbackRefcon, 0L, 0L);
	}
	
	return kFskErrNone;
}

FskErr KplUIEventSetCallback(KplUIEventCallback proc, void *refcon)
{
	gUIEventCallback = proc;
	gUIEventCallbackRefcon = refcon;
	gUIEventCallbackThread = KplThreadGetCurrent();
	
	return kFskErrNone;
}
