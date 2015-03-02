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
#define _WIN32_WINNT 0x0400

#define __FSKUTILITIES_PRIV__
#define __FSKNETUTILS_PRIV__
#define __FSKTHREAD_PRIV__
#define __FSKWINDOW_PRIV__
#define __FSKTIME_PRIV__
#define __FSKPORT_PRIV__
#define __FSKEVENT_PRIV__
#define __FSKEXTENSIONS_PRIV__
#include "FskThread.h"
#include "FskExtensions.h"
#include "FskWindow.h"

#if TARGET_OS_LINUX
	#include "poll.h"
	#define THREAD_STACK_SIZE (128 * 1024)
#endif

#include "FskList.h"
#include "FskMemory.h"
#include "FskPlatformImplementation.h"
#include "FskSynchronization.h"
#include "FskNetUtils.h"
#if TARGET_OS_LINUX
#include "FskMain.h"	/* just for gQuitting */
#endif
#if TARGET_OS_WIN32
	#include "Windowsx.h"
#endif
#if TARGET_OS_MAC
	#include "FskCocoaSupport.h"
#endif
#if TARGET_OS_KPL
	#include "KplThread.h"
	#include "KplSynchronization.h"
#endif

#if TARGET_OS_ANDROID
	# define TIMEVAL_TO_TIMESPEC(tv, ts) {                                  \
        (ts)->tv_sec = (tv)->tv_sec;                                    \
        (ts)->tv_nsec = (tv)->tv_usec * 1000;                           \
	}

#endif

#include "FskHardware.h"

/*
	Thread
*/

FskThread mainThread;
FskListMutex gThreads;


#if TARGET_OS_WIN32
	UINT gThreadEventMessage;
	DWORD gFskThreadTLSSlot;

	static unsigned int __stdcall threadProc(void *refcon);
#elif TARGET_OS_LINUX
	static void threadCheckEvents(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);
	static void LinuxThreadWaitForData(SInt32 msec);
	static pthread_key_t gThreadStructKey;

	static void *threadProc(void *refcon);

	static int sFifoPort = 5678;
#elif TARGET_OS_MAC
	static pthread_key_t gThreadStructKey;

	static void *threadProc(void *refcon);
#elif TARGET_OS_KPL
	static void *threadProc(void *refcon);
#endif

#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageFskThread(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gThreadTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"thread",
		FskInstrumentationOffset(FskThreadRecord),
		NULL,
		0,
		NULL,
		doFormatMessageFskThread
	};

	static FskInstrumentedTypeRecord gConditionTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"condition",
		FskInstrumentationOffset(FskConditionRecord),
		NULL,
		0,
		NULL,
		doFormatMessageFskThread
	};

#endif

	FskInstrumentedSimpleType(Runloop, runloop);

#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL

FskErr FskThreadCreate(FskThread *threadOut, FskThreadProc procedure, UInt32 flags, void *refcon, const char *name)
{
	FskErr err;
	FskThread thread = NULL;

	err = FskMemPtrNewClear(sizeof(FskThreadRecord) + FskStrLen(name), &thread);
	BAIL_IF_ERR(err);

	thread->flags = flags;
	thread->userProc = procedure;
	thread->userRefcon = refcon;
	thread->name = thread->nameBuffer;
	FskStrCopy(thread->name, name);

	if (thread->flags & kFskThreadFlagsJoinable) {
		err = FskMutexNew(&thread->running, "threadRunning");
		BAIL_IF_ERR(err);

		flags |= kFskThreadFlagsWaitForInit;
	}

	if (!(thread->flags & kFskThreadFlagsTransientWorker)) {
		err = FskListMutexNew(&thread->eventHandlers, "threadEventHandlers");
		BAIL_IF_ERR(err);
	}

	FskListMutexPrepend(gThreads, thread);

	FskInstrumentedItemNew(thread, thread->name, &gThreadTypeInstrumentation);

    *threadOut = thread;        // set this early so it is available to client at start of its threadProc

#if TARGET_OS_WIN32

#if SUPPORT_TIMER_THREAD
	thread->nextCallbackTime.seconds = kFskUInt32Max;
	thread->waitableTimer = CreateEvent(NULL, false, false, NULL);
#endif

	thread->handle = (HANDLE)_beginthreadex(NULL, 0, threadProc, thread, 0, &thread->id);
    BAIL_IF_ZERO(thread->handle, err, kFskErrOperationFailed);

	if (thread->flags & kFskThreadFlagsHighPriority)
		SetThreadPriority(thread->handle, THREAD_PRIORITY_ABOVE_NORMAL);
	else if (thread->flags & kFskThreadFlagsLowPriority)
		SetThreadPriority(thread->handle, THREAD_PRIORITY_BELOW_NORMAL);

#elif TARGET_OS_ANDROID || TARGET_OS_LINUX || TARGET_OS_MAC
	{
	pthread_attr_t	attr;
	pthread_t pthread;

	if (!(thread->flags & kFskThreadFlagsTransientWorker)) {
		err = FskListMutexNew(&thread->eventQueue, "threadEventQueue");
		BAIL_IF_ERR(err);
	}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#if TARGET_OS_LINUX
	pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE);
#endif

	if (thread->flags & kFskThreadFlagsHighPriority) {
		struct sched_param sched_level;
		int min, max;
//		int policy = SCHED_FIFO;
		int policy = SCHED_OTHER;

		min = sched_get_priority_min(policy);
		max = sched_get_priority_max(policy);
		max = (min + max) / 2;
		pthread_attr_setschedpolicy(&attr, policy);
		sched_level.sched_priority = max;
		pthread_attr_setschedparam(&attr, &sched_level);
	}

	if (0 != pthread_create(&pthread, &attr, threadProc, thread))
		err = kFskErrOperationFailed;

	if (!(flags & kFskThreadFlagsTransientWorker))
		thread->pthread = pthread;	/* thread should be still alive */
        
	pthread_attr_destroy(&attr);

	BAIL_IF_ERR(err);
	}
#elif TARGET_OS_KPL
	err = KplThreadCreate((KplThread*)&thread->kplThread, (KplThreadProc)threadProc, thread, flags, name);
	BAIL_IF_ERR(err);
#endif

	if (flags & kFskThreadFlagsTransientWorker)
		thread = NULL;		// a transient thread may complete before
	else
	if (flags & kFskThreadFlagsWaitForInit) {
		FskInstrumentedItemSendMessage(thread, kFskThreadInstrMsgWaitForInit, thread);

		while (!thread->threadIsRunning && !thread->threadFinished)
			FskThreadYield();
		FskInstrumentedItemSendMessage(thread, kFskThreadInstrMsgDoneInit, thread);
	}

bail:
	if (kFskErrNone != err) {
		if (NULL != thread) {
			FskInstrumentedItemDispose(thread);
			FskListMutexRemove(gThreads, thread);
			FskMutexDispose(thread->running);
			FskListMutexDispose(thread->eventHandlers);
#if TARGET_OS_LINUX || TARGET_OS_MAC
			FskListMutexDispose(thread->eventQueue);
#endif
			FskMemPtrDisposeAt(&thread);
		}
	}

	*threadOut = thread;

	return err;
}

#else

FskErr FskThreadCreate(FskThread *threadOut, FskThreadProc procedure, UInt32 flags, void *refcon, const char *name)
{
	return kFskErrUnimplemented;
}

#endif

void FskThreadYield(void)
{
#if TARGET_OS_WIN32
	Sleep(10);
#elif TARGET_OS_MAC
	sched_yield();
#elif TARGET_OS_ANDROID
//	FskRunloopPrintfDebug("before sched_yield");
	usleep(10);
	sched_yield();
//	FskRunloopPrintfDebug("after sched_yield");
#elif TARGET_OS_LINUX
//	FskRunloopPrintfDebug("before sched_yield");
	sched_yield();
//	usleep(10);
	FskRunloopPrintfDebug("after sched_yield");
#elif TARGET_OS_KPL
	KplThreadYield();
#endif
}

FskErr FskThreadJoin(FskThread thread) {
	if (!thread)
		return kFskErrOperationFailed;

	FskInstrumentedItemSendMessage(thread, kFskThreadInstrMsgJoin, thread); // Wait for the thread to complete
	if (!thread->threadFinished) {
		FskThreadWake(thread);
		FskThreadYield();
	}
	if ((thread->flags & kFskThreadFlagsJoinable)) {
		FskMutexAcquire(thread->running);
		if (!thread->threadFinished) {
			while (thread->threadIsRunning)	// wait for cleanup
				FskThreadYield();
		}
		if (thread->eventHandlers){
			while (thread->eventHandlers->list)
				FskMemPtrDispose(FskListMutexRemoveFirst(thread->eventHandlers));
			FskListMutexDispose(thread->eventHandlers);
		}
		FskAssociativeArrayDispose(thread->environmentVariables);
		FskExtensionsTerminateThread(thread);
		FskMutexDispose(thread->running);
#if TARGET_OS_WIN32
#if SUPPORT_WAITABLE_TIMER || SUPPORT_TIMER_THREAD
		if (thread->waitableTimer)
			CloseHandle(thread->waitableTimer);
#endif
		CloseHandle(thread->handle);
#endif
#if TARGET_OS_KPL
		KplThreadJoin(thread->kplThread);
#endif
		FskInstrumentedItemSendMessage(thread, kFskThreadInstrMsgJoined, thread); // Wait for the thread to complete
		FskInstrumentedItemDispose(thread);
		FskMemPtrDispose(thread);
	}
	else {
		FskInstrumentedItemSendMessage(thread, kFskThreadInstrMsgJoinNonJoinable, thread);
	}
	return kFskErrNone;
}

FskErr FskThreadTerminate() {
	// kill all threads (but main)
	return kFskErrUnimplemented;
}

FskThread FskThreadGetCurrent(void)
{
	if (NULL == gThreads)
		return mainThread;

{
#if TARGET_OS_WIN32
	return (FskThread)TlsGetValue(gFskThreadTLSSlot);
#elif TARGET_OS_LINUX || TARGET_OS_MAC
	FskThread thr;

	thr = (FskThread)pthread_getspecific(gThreadStructKey);
	if (!thr)
		thr = mainThread;

	return thr;
#elif TARGET_OS_KPL
	FskThread current;
	KplThread kplThread = KplThreadGetCurrent();

	current = (FskThread)KplThreadGetRefcon(kplThread);
	if (NULL == current)
		current = mainThread;

	return current;
#else
	return NULL;
#endif
}
}

/*
	Thread data handlers (ie. net and file descriptor)
*/
FskThreadDataSource FskThreadCreateDataSource(UInt32 fd) {
	FskThreadDataSource source;
	FskErr err;

	err = FskMemPtrNewClear(sizeof(FskThreadDataSourceRecord), &source);
	if (kFskErrNone != err)
		return NULL;
	source->dataNode = fd;
	return source;
}

UInt32 FskThreadGetDataHandlerDataNode(FskThreadDataHandler handler) {
	return handler->source->dataNode;
}

#if TARGET_OS_MAC

static void MacThreadCheckDataHandlerForSocket(int s, Boolean *wantsReadable, Boolean *wantsWritable)
{
	FskThread thread;
	FskThreadDataHandler cur;

	*wantsReadable = false;
	*wantsWritable = false;

	thread = FskThreadGetCurrent();
	cur = FskListGetNext(thread->dataHandlers, NULL);
	while (cur) {
		if (cur->source && (int)cur->source->dataNode == s) {
			if (cur->wantsReadable) {
				*wantsReadable = true;
			}
			if (cur->wantsWritable) {
				*wantsWritable = true;
			}
		}
		cur = FskListGetNext(thread->dataHandlers, cur);
	}
}

#endif

void FskThreadAddDataHandler(FskThreadDataHandler *handlerOut, FskThreadDataSource source, FskThreadDataReadyCallback handler, Boolean wantsReadable, Boolean wantsWritable, void *refCon)
{
	FskThreadDataHandler	handlerRec;
	FskThread	self;
	FskErr		err;

	err = FskMemPtrNewClear(sizeof(FskThreadDataHandlerRecord), &handlerRec);
	if (kFskErrNone == err) {
		handlerRec->source = source;
		handlerRec->callback = handler;
		handlerRec->wantsReadable = wantsReadable;
		handlerRec->wantsWritable = wantsWritable;
		handlerRec->refCon = refCon;

		self = FskThreadGetCurrent();

		handlerRec->owner = self;

		FskListPrepend(&self->dataHandlers, handlerRec);
		if (handlerOut)
			*handlerOut = handlerRec;
#if TARGET_OS_MAC
		MacThreadCheckDataHandlerForSocket(handlerRec->source->dataNode, &wantsReadable, &wantsWritable);
		macSocketCallbackEnable(handlerRec->source->dataNode, wantsReadable, wantsWritable);
#endif
		if ((wantsReadable && source->pendingReadable)
			|| (wantsWritable && source->pendingWritable)) {
#if TARGET_OS_WIN32
			PostMessage(self->window, gAsyncSelectMessage, source->dataNode,
				(source->pendingReadable ? FD_READ : 0) | (source->pendingWritable ? FD_WRITE : 0));
#elif TARGET_OS_MAC
			MacThreadGotSocketData();
#elif TARGET_OS_KPL
			KplThreadNotifyPendingSocketData((void*)source->dataNode, source->pendingReadable, source->pendingWritable);
#endif
		}
	}
}


void FskThreadRemoveDataHandler(FskThreadDataHandler *handler) {
	FskThread self;
#if TARGET_OS_MAC
	Boolean wantsReadable, wantsWritable;
#endif

	if (handler && *handler) {
		self = FskThreadGetCurrent();
		if ((*handler)->owner != self) {
			FskInstrumentedTypePrintfMinimal(&gThreadTypeInstrumentation, "!!!!!!!!! - removing data handler from the wrong thread (%s vs %s)", (*handler)->owner->name, self ? self->name : NULL);
			(*handler)->pendingDispose = true;
			*handler = NULL;		// did we just leak here?
			return;
		}
		FskListRemove(&self->dataHandlers, *handler);
#if TARGET_OS_MAC
		MacThreadCheckDataHandlerForSocket((*handler)->source->dataNode, &wantsReadable, &wantsWritable);
		macSocketCallbackEnable((*handler)->source->dataNode, wantsReadable, wantsWritable);
#endif
		if ((*handler)->inUse) {
			(*handler)->pendingDispose = true;
			*handler = NULL;
		}
		else {
			FskInstrumentedTypePrintfDebug(&gThreadTypeInstrumentation, " - RemoveDataHandler - (%x) disposing", *handler);
			FskMemPtrDisposeAt((void **)handler);
		}
	}
}

FskThreadDataHandler FskThreadFindDataHandlerBySourceNode(UInt32 dataNode)
{
	FskThreadDataHandler handler = NULL;
	FskThread thread;

	thread = FskThreadGetCurrent();
	while (1) {
		handler = (FskThreadDataHandler)FskListGetNext(thread->dataHandlers, handler);
		if (!handler)
			break;
		if (handler->source->dataNode == dataNode)
			return handler;
	}
	return NULL;
}


/*
	Thread event handlers
*/

Boolean FskThreadDefaultQuitHandler(FskEvent event, void *quit) {
	*(Boolean *)quit = true;
	return true;
}

FskThreadEventHandler FskThreadAddEventHandler(UInt32 eventCode, FskThreadEventHandlerRoutine handler, void *refCon)
{
	FskThreadEventHandler handlerRec;
	FskErr		err;

	err = FskMemPtrNewClear(sizeof(FskThreadEventHandlerRecord), &handlerRec);
	if (kFskErrNone == err) {
		handlerRec->eventCode = eventCode;
		handlerRec->eventHandler = handler;
		handlerRec->refCon = refCon;

		handlerRec->self = FskThreadGetCurrent();
		FskInstrumentedItemSendMessage(handlerRec->self, kFskThreadInstrMsgAddEventHandler, handlerRec->self);
		FskListMutexPrepend(handlerRec->self->eventHandlers, handlerRec);
		return handlerRec;
	}

	return NULL;
}

FskErr FskThreadRemoveEventHandler(FskThreadEventHandler handler)
{
	if (handler) {
		FskInstrumentedItemSendMessage(handler->self, kFskThreadInstrMsgRemoveEventHandler, handler->self);
		FskListMutexRemove(handler->self->eventHandlers, handler);
		FskMemPtrDispose(handler);
	}
	return kFskErrNone;
}

FskErr FskThreadBroadcastEvent(FskEvent event) {
	FskThread walker = NULL;

	FskListMutexAcquireMutex(gThreads);

	while (true) {
		FskThreadEventHandler	handler = NULL;
		walker = (FskThread)FskListGetNext(gThreads->list, walker);
		if (!walker)
			break;

		FskListMutexAcquireMutex(walker->eventHandlers);

		while (true) {
			handler = (FskThreadEventHandler)FskListGetNext(walker->eventHandlers->list, handler);
			if (!handler)
				break;
			if (handler->eventCode == event->eventCode) {
				FskEvent eventCopy;
				// event with multiple handlers for an event, we only need to post
				// one event to it's thread
				FskInstrumentedItemSendMessage(handler->self, kFskThreadInstrMsgBroadcastEvent, handler->self);
				FskEventCopy(event, &eventCopy);
				FskThreadPostEvent(walker, eventCopy);
				break;
			}
		}

		FskListMutexReleaseMutex(walker->eventHandlers);
	}

	FskListMutexReleaseMutex(gThreads);
	return kFskErrNone;
}

FskErr FskThreadWake(FskThread thread) {
#if TARGET_OS_LINUX
	FskThread self;
	int amt;
	char msg[5] = "wake";
#endif
#if SUPPORT_INSTRUMENTATION
	FskThread thisThread = FskThreadGetCurrent();
	if (thisThread)
		FskInstrumentedItemSendMessage(thisThread, kFskThreadInstrMsgWake, thread);
#endif
#if TARGET_OS_WIN32
	PostMessage(thread->window, gThreadEventMessage, 0, 0);
#elif TARGET_OS_ANDROID
	self = FskThreadGetCurrent();
	if (!thread->wakePending) {
		thread->wakePending++;
		if (thread->flags & kFskThreadFlagsIsMain)
			gAndroidCallbacks->wakeMainCB();
		else {
			FskNetSocketSendUDP((FskSocket)self->eventTrigger, msg, 4, &amt, 0x7f000001, thread->fifoPort);
		}
	}
#elif TARGET_OS_LINUX
	self = FskThreadGetCurrent();
	FskNetSocketSendUDP((FskSocket)self->eventTrigger, msg, 4, &amt, 0x7f000001, thread->fifoPort);
#elif TARGET_OS_MAC
	FskCocoaThreadWake(thread);
#elif TARGET_OS_KPL
	KplThreadWake(thread->kplThread);
#endif
	return kFskErrNone;
}


FskErr FskThreadPostEvent(FskThread thread, FskEvent event)
{
	if (!gThreads)
 		return kFskErrOperationFailed;

	FskInstrumentedItemSendMessage(thread, kFskThreadInstrMsgPostEvent, thread);
	FskListMutexAcquireMutex(gThreads);

	if (false == FskListContains(gThreads->list, thread)) {
		FskListMutexReleaseMutex(gThreads);
		return kFskErrInvalidParameter;			// not valid thread
	}

#if TARGET_OS_WIN32
	PostMessage(thread->window, gThreadEventMessage, 0, (LPARAM)event);
#elif TARGET_OS_LINUX  || TARGET_OS_MAC
	FskListMutexAppend(thread->eventQueue, event);
	FskThreadWake(thread);
#elif TARGET_OS_KPL
	KplThreadPostEvent(thread->kplThread, event);
#endif

	FskListMutexReleaseMutex(gThreads);

	return kFskErrNone;
}

#if SUPPORT_INSTRUMENTATION
FskErr FskThreadPostCallback_(FskThread thread, FskThreadCallback function, void *arg1, void *arg2, void *arg3, void *arg4, const char *functionName)
#else
FskErr FskThreadPostCallback_(FskThread thread, FskThreadCallback function, void *arg1, void *arg2, void *arg3, void *arg4)
#endif
{
	FskErr err;
	FskEvent event;
	void *args[5];

#if SUPPORT_INSTRUMENTATION
	{
	void *data[2];
	data[0] = thread;
	data[1] = (void *)functionName;
	FskInstrumentedItemSendMessage(thread, kFskThreadInstrMsgPostCallback, data);
	}
#endif
	err = FskEventNew(&event, kFskEventThreadCallback, 0, 0);
	BAIL_IF_ERR(err);

	args[0] = function;
	args[1] = arg1;
	args[2] = arg2;
	args[3] = arg3;
	args[4] = arg4;
	err = FskEventParameterAdd(event, kFskEventParameterThreadCallbackArgs, sizeof(args), args);
	BAIL_IF_ERR(err);
	err = FskThreadPostEvent(thread, event);

bail:
	if (kFskErrNone != err)
		FskEventDispose(event);

	return err;
}

#if TARGET_OS_KPL
FskErr FskKplThreadPostCallback(KplThread kplThread, KplThreadCallback function, void *arg1, void *arg2, void *arg3, void *arg4)
{
	FskThread thread = NULL, walker = NULL;

	FskListMutexAcquireMutex(gThreads);
	while (true) {
		walker = (FskThread)FskListGetNext(gThreads->list, walker);
		if (!walker)
			break;
		if (kplThread == walker->kplThread) {
			thread = walker;
			break;
		}
	}
	FskListMutexReleaseMutex(gThreads);

	if (!thread)
		return kFskErrInvalidParameter;

	return (FskErr)FskThreadPostCallback(thread, (FskThreadCallback)function, arg1, arg2, arg3, arg4);
}
#endif

Boolean HandleThreadEvent(FskEvent event) {
	void *args[5];
	Boolean handled = false;
	FskThread thread;
	FskThreadEventHandler handler;

	if (!event) return handled;

	thread = FskThreadGetCurrent();
	if (!thread)
		goto bail;

	if (kFskEventThreadCallback == event->eventCode) {
        if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterThreadCallbackArgs, args)) {
            ((FskThreadCallback)args[0])(args[1], args[2], args[3], args[4]);
            handled = true;
            goto bail;
        }
	}

	FskListMutexAcquireMutex(thread->eventHandlers);
	handler = NULL;
	while (true) {
		handler = (FskThreadEventHandler)FskListGetNext(thread->eventHandlers->list, handler);
		if (!handler)
			break;
		if (handler->eventCode == event->eventCode)
			handled = (*handler->eventHandler)(event, handler->refCon);
		if (handled)
			break;
	}

	FskListMutexReleaseMutex(thread->eventHandlers);

bail:
	FskEventDispose(event);
	return handled;
}

#if TARGET_OS_KPL
Boolean FskHandleThreadEvent(void *event) {
	return HandleThreadEvent((FskEvent)event);
}
#endif

#if TARGET_OS_ANDROID
FskErr androidFskThreadRunloopCycle(SInt32 msec, SInt32 *outmsec) {
	FskThread thread;
	FskEvent event;
	SInt32	timerDeltaMS;
	SInt32	retVal;
	Boolean needsTime = false;
	FskTimeRecord nextEventTime;

	thread = FskThreadGetCurrent();
	if (!thread) return kFskErrNone;
	thread->cycle++;
//printEvents(thread->eventQueue, thread);
	FskWindowGetNextEventTime(thread, &nextEventTime);

	FskTimeCallbackServiceUntil(thread, &nextEventTime);	// trigger prior to first event

	while (NULL != (event = FskListMutexRemoveFirst(thread->eventQueue))) {
		HandleThreadEvent(event);		// system events
	}

	if (FskWindowCheckEvents()) {		// window events
		needsTime = true;
	}

	if (gQuitting)
		needsTime = true;

	// Do select loop with timeout in here to get Network and Timer events
	timerDeltaMS = FskTimeCallbackGetNextTimeDelta();

	if (needsTime)
		retVal = 0;
	else
		retVal = timerDeltaMS;

	LinuxThreadWaitForData(0);

	if (FskWindowCheckEvents()) {		// window events
		needsTime = true;
	}

	retVal = needsTime ? 0 : FskTimeCallbackGetNextTimeDelta();

	if (retVal < 0)
		retVal = 0;
	*outmsec = retVal;

	return kFskErrNone;
}
#endif

#if TARGET_OS_ANDROID
Boolean FskWindowNeedsTime(FskWindow win) {
	if (win && win->port) {
		FskThread thread;
		thread = FskThreadGetCurrent();

		if (thread != win->thread)
			return false;

#if TARGET_OS_ANDROID
		if (!FskRectangleIsEmpty(&win->previousInvalidArea)) {
			FskRunloopPrintfDebug(" -- previousInvalidArea is set, call me soon (ret true)");
			return true;
		}
#endif
		if (win->updateTimer && !win->updateSuspended) {
			FskTimeRecord now;
			FskTimeGetNow(&now);
			if (0 <= FskTimeCompare(&win->updateTimer->trigger, &now)) {
				if (FskListContains(win->thread->timerCallbacks, win->updateTimer)) {
					FskRunloopPrintfDebug("ui timer needs triggering, return true");
					return true;
				}
				FskRunloopPrintfDebug("timer needs triggering, but we're suspended - return false");
				return false;
			}
			FskRunloopPrintfDebug("timer doesn't need triggering - return false");
			return false;
		}

		//When continuous drawing enabled, wait for the next drawing pump callback
		if (win->useFrameBufferUpdate && win->doBeforeUpdate && !win->updateSuspended) {

			if (!FskListMutexIsEmpty(thread->eventQueue)) {
				FskRunloopPrintfDebug("We already have a event in queue, return true");
				return true;
			}

			return false;
		}

		if (!FskRectangleIsEmpty(&win->port->invalidArea)) {
			FskRunloopPrintfDebug("there's an invalid area - return true");
			return true;
		}
	}
	return false;
}
#endif


FskErr FskThreadRunloopCycle(SInt32 msec) {
#if TARGET_OS_WIN32
	MSG msg;
	FskWindow win;
	DWORD anchorTicks, ticks;

#if SUPPORT_WAITABLE_TIMER || SUPPORT_TIMER_THREAD
	FskThread thread = FskThreadGetCurrent();
	const int kFskMWMOFlags = MWMO_ALERTABLE | MWMO_INPUTAVAILABLE;
	Boolean flushMouse = false, timedOut = false;

	if (!thread)
		return kFskErrBadState;

	// we need to wait in an alertable state so APCs can call immediately (e.g. ReadDirectoryChangesW callback)
	while (true) {

#if SUPPORT_WAITABLE_TIMER || SUPPORT_TIMER_THREAD
		if (thread->waitableTimer) {
			DWORD result;
			DWORD waitMS;
			if (0 != thread->nextTicks) {
				DWORD now = GetTickCount();
				if (now >= thread->nextTicks)
					waitMS = 0;
				else
					waitMS = thread->nextTicks - now;
			}
			else
				waitMS = INFINITE;
			result = MsgWaitForMultipleObjectsEx(1, &thread->waitableTimer, waitMS, QS_ALLINPUT, kFskMWMOFlags);
			if ((WAIT_OBJECT_0 == result) || (WAIT_IO_COMPLETION == result))
				FskTimeCallbackService(thread);
			else {
				timedOut = WAIT_TIMEOUT == result;
				break;
			}
		}
		else
#endif
		{
			HANDLE h[1];
			DWORD result = MsgWaitForMultipleObjectsEx(0, h, INFINITE, QS_ALLINPUT, kFskMWMOFlags);

			timedOut = WAIT_TIMEOUT == result;
			if ((WAIT_OBJECT_0 == result) || timedOut)
				break;
		}
	}
#endif

	anchorTicks = GetTickCount();
	flushMouse = timedOut && (0 != thread->nextTicks);
	thread->nextTicks = 0;
	while (true) {
		Boolean haveMessage = PeekMessage(&msg, NULL, 0, 0, true);
		Boolean adjustMouseMessages;

		if (thread->haveMouseMove) {
			if (!haveMessage && !flushMouse) {
				DWORD now = GetTickCount();
				DWORD delta = now - thread->points[1].ticks;
				UInt32 kThreshold;
				win = FskWindowGetActive();
				kThreshold = win ? win->updateInterval : 30;
				if (0 != thread->nextTicks)
					break;
				if (delta < kThreshold) {
					thread->nextTicks = ((now + kThreshold) / kThreshold) * kThreshold;
					break;
				}
			}
			if ((WM_LBUTTONDOWN == msg.message) || (WM_LBUTTONUP == msg.message) || (WM_LBUTTONDBLCLK == msg.message) || (thread->haveMouseMove >= 48) || !haveMessage || flushMouse) {
				thread->points[0].index = thread->haveMouseMove;
				if (kFskErrNone == FskMemPtrNewFromData((sizeof(FskPointAndTicksRecord) * thread->haveMouseMove) + sizeof(UInt32), &thread->points[0].index, &thread->mouseMove.lParam)) {
					thread->mouseMove.message = FSK_WM_MOUSEMOVE;

					TranslateMessage(&thread->mouseMove);
					DispatchMessage(&thread->mouseMove);
				}
				thread->haveMouseMove = 0;
			}
		}

		if (!haveMessage)
			break;

		flushMouse = false;

		win = FskWindowGetActive();
		adjustMouseMessages = (NULL != win);

		if (adjustMouseMessages) {
			if ((WM_LBUTTONDOWN == msg.message) || (WM_LBUTTONDBLCLK == msg.message) || (WM_LBUTTONUP == msg.message) ||
				(WM_RBUTTONDOWN == msg.message) || (WM_RBUTTONUP == msg.message) || (WM_RBUTTONDBLCLK == msg.message) || (WM_MOUSELEAVE == msg.message)) {
				void *tMsg;

				if (kFskErrNone == FskMemPtrNewFromData(sizeof(msg), &msg, &tMsg)) {
					msg.message = FSK_WM_MOUSE_WITH_TIME;
					msg.lParam = (long)tMsg;
					TranslateMessage(&msg);
					DispatchMessage(&msg);
					continue;
				}
			}

			if (WM_MOUSEMOVE == msg.message) {
				thread->haveMouseMove += 1;
				thread->points[thread->haveMouseMove].pt.x = GET_X_LPARAM(msg.lParam);
				thread->points[thread->haveMouseMove].pt.y = GET_Y_LPARAM(msg.lParam);
				thread->points[thread->haveMouseMove].ticks = msg.time;
				thread->points[thread->haveMouseMove].index = 0;

				thread->mouseMove = msg;

				ticks = GetTickCount();
				if ((anchorTicks + 80) < ticks) {
					// every 80 ms allow an event through, so mouse moved don't starve callbacks (which can include drawing)
					FskTimeCallbackService(thread);
					anchorTicks = ticks;
				}

				continue;
			}
		}

		if (win && win->haccel)
			if (TranslateAccelerator(win->hwnd, win->haccel, &msg))
				continue;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#elif TARGET_OS_MAC
    FskThread thread;
    FskEvent event;
    Boolean needsTime;
    thread = FskThreadGetCurrent();
    FskTimeCallbackService(thread);			// timer triggers

	do {
		needsTime = false;
		while (NULL != (event = FskListMutexRemoveFirst(thread->eventQueue))) {
			if (HandleThreadEvent(event) && !needsTime)		// system events
				needsTime = true;
		}
		if (!(thread->flags & kFskThreadFlagsIsMain)) break;
		if (FskWindowCheckEvents())			// window events
			if (!FskListMutexIsEmpty(thread->eventQueue))
				needsTime = true;
	} while (needsTime);
	
    // runloop
    if (needsTime)
        msec = 0;

    if (thread->flags & kFskThreadFlagsIsMain)
        FskCocoaApplicationRun();
    else
    FskCocoaThreadRun(thread, msec);

#elif TARGET_OS_ANDROID

	FskThread thread;
	FskEvent event;
	SInt32	timerDeltaMS;
	SInt32	waitTimeMS;
	Boolean needsTime = false;
	FskTimeRecord	nextEventTime;

	thread = FskThreadGetCurrent();

	thread->cycle++;
	FskWindowGetNextEventTime(thread, &nextEventTime);

	FskTimeCallbackServiceUntil(thread, &nextEventTime);	// trigger prior to first event

	FskRunloopPrintfDebug("Handling thread event START");

	// De-queue and handle system events here.
	// We only handle the events in the queue at this point, since the client may post more events from within the call to FskHandleThreadEvent().
	FskMutexAcquire(thread->eventQueue->mutex);
	event = thread->eventQueue->list;
	thread->eventQueue->list = NULL;
	FskMutexRelease(thread->eventQueue->mutex);

#if SUPPORT_INSTRUMENTATION
	if (event)
		FskRunloopPrintfDebug("There are %ld events in the queue", FskListCount(event));
#endif

	while (NULL != event) {
		FskEvent next = event->next;
		FskRunloopPrintfDebug("Handling thread event");
		HandleThreadEvent(event);	// system events
		event = next;
	}

	FskRunloopPrintfDebug("Handling thread event DONE");

	if (FskWindowCheckEvents()) {		// window events
		FskRunloopPrintfDebug(" - runloop cycle check window events - NEEDS TIME");
		needsTime = true;
	}

	if (gQuitting)
		needsTime = true;

	timerDeltaMS = FskTimeCallbackGetNextTimeDelta();

	if (needsTime)
		waitTimeMS = 0;
	else
		waitTimeMS = timerDeltaMS;

	if (FskWindowNeedsTime(FskWindowGetActive()))
		waitTimeMS = 0;

	FskRunloopPrintfDebug("next timer in %d ms, passed in %d, taking %d", timerDeltaMS, msec, waitTimeMS);

	// Do select loop with timeout in here to get Network and Timer events
	LinuxThreadWaitForData(thread->wakePending ? 0 : waitTimeMS);

#elif TARGET_OS_LINUX
	FskThread thread;
	FskEvent event;
	SInt32	timerDeltaMS;
	FskTimeRecord nextEventTime;
	Boolean needsTime = false;

	thread = FskThreadGetCurrent();

	FskWindowGetNextEventTime(thread, &nextEventTime);
	FskTimeCallbackServiceUntil(thread, &nextEventTime);	// timer triggers prior to first event

	//FskTimeCallbackService(thread);			// timer triggers

	while (NULL != (event = FskListMutexRemoveFirst(thread->eventQueue)))
		HandleThreadEvent(event);		// system events

	if (FskWindowCheckEvents()) {		// window events
		needsTime = true;
	}

	if (gQuitting)
		needsTime = true;

	// Do select loop with timeout in here to get Network and Timer events
	timerDeltaMS = FskTimeCallbackGetNextTimeDelta();
	if ((msec >= 0) && (msec < timerDeltaMS))
		timerDeltaMS = msec;

	if (needsTime)
		timerDeltaMS = 0;

	FskThreadYield();
	LinuxThreadWaitForData(timerDeltaMS);
#elif TARGET_OS_KPL
	KplThreadRunloopCycle(msec);
#endif
	return kFskErrNone;
}


#if TARGET_OS_WIN32

#include "FskPlatformImplementation.h"

#if _DEBUG
	typedef struct tagTHREADNAME_INFO
	{
	DWORD dwType; // must be 0x1000
	LPCSTR szName; // pointer to name (in user addr space)
	DWORD dwThreadID; // thread ID (-1=caller thread)
	DWORD dwFlags; // reserved for future use, must be zero
	} THREADNAME_INFO;
#endif

static void makeThreadWindow(FskThread thread);
static long FAR PASCAL threadWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam);

unsigned int __stdcall threadProc(void *refcon)
{
	FskThread thread = (FskThread)refcon;

	CoInitialize(NULL);

	timeBeginPeriod(5);

#if _DEBUG
	if (IsDebuggerPresent() && (NULL != thread->name)) {
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = thread->name;
		info.dwThreadID = -1;
		info.dwFlags = 0;

		RaiseException(0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info);		// yes, this really comes from the microsoft documentation
	}
#endif

	makeThreadWindow(thread);

	FskRandomInit();		// random needs to be seeded per-thread

	if ((thread->flags & kFskThreadFlagsJoinable)) {
		FskMutexAcquire(thread->running);
		thread->threadIsRunning = false;
	}

	// get it running
	(thread->userProc)(thread->userRefcon);

	thread->threadFinished = true;
	// shut it down
	if (gThreads)
		FskListMutexRemove(gThreads, thread);
	DestroyWindow(thread->window);
	TlsSetValue(gFskThreadTLSSlot, 0);

	if ((thread->flags & kFskThreadFlagsJoinable)) {
		thread->threadIsRunning = false;
		FskMutexRelease(thread->running);
	}
	else {
		if (thread->eventHandlers){
			while (thread->eventHandlers->list)
				FskMemPtrDispose(FskListMutexRemoveFirst(thread->eventHandlers));
			FskListMutexDispose(thread->eventHandlers);
		}

		FskAssociativeArrayDispose(thread->environmentVariables);
		FskExtensionsTerminateThread(thread);
#if SUPPORT_WAITABLE_TIMER || SUPPORT_TIMER_THREAD
		if (thread->waitableTimer)
			CloseHandle(thread->waitableTimer);
#endif
		CloseHandle(thread->handle);		// necessary because we are creating the thread with _beginthreadex
		FskInstrumentedItemDispose(thread);
		FskMemPtrDispose(thread);
	}

	CoUninitialize();

	return 0;
}

void makeThreadWindow(FskThread thread)
{
	WNDCLASS wc;

	// we need a window to be able to receive messages
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = threadWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "kinoma-thread";
	RegisterClass(&wc);
	thread->window = CreateWindow("kinoma-thread", NULL,
		WS_DISABLED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);

	TlsSetValue(gFskThreadTLSSlot, thread);
}


long FAR PASCAL threadWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
#if SUPPORT_WAITABLE_TIMER
#elif SUPPORT_TIMER_THREAD
#else
	if (WM_TIMER == msg) {
		FskTimeCallbackService(FskThreadGetCurrent());
		return 0;
	}
	else
#endif
	if (gThreadEventMessage == msg) {
		FskEvent event = (FskEvent)lParam;
		HandleThreadEvent(event);

		return 1;
	}
	else if (gAsyncSelectMessage == msg) {
		FskThreadDataHandler handler;
		int event;

		event = WSAGETSELECTEVENT(lParam);
		win32SocketEvent(wParam, event);
		handler = FskThreadFindDataHandlerBySourceNode(wParam);
		if (handler) {
			Boolean callit = false;
			if (handler->source->pendingReadable && handler->wantsReadable) {
				callit = true;
				if (! handler->source->isSocket)
					handler->source->pendingReadable = false;
			}
			if (handler->source->pendingWritable && handler->wantsWritable) {
				callit = true;
				if (! handler->source->isSocket)
					handler->source->pendingWritable = false;
			}
			if (handler->source->pendingClose)
				callit = true;
			if (callit)
				(*handler->callback)(handler, handler->source, handler->refCon);
		}
		return 1;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

#elif TARGET_OS_ANDROID || TARGET_OS_LINUX

Boolean LinuxHandleThreadEvents(void) {
	FskThread thread = FskThreadGetCurrent();
	if (thread && thread->eventQueue) {
		FskEvent event = FskListMutexRemoveFirst(thread->eventQueue);
        if (event) {
            HandleThreadEvent(event);
            return true;
        }
	}
    return false;
}

void threadCheckEvents(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon) {
	FskThread thread = (FskThread)refCon;
	char junk[9];
	int amt = 8;

	while (amt) {
		FskNetSocketRecvUDP((FskSocket)thread->eventTrigger, junk, 9, &amt, NULL, NULL);
	}
#if TARGET_OS_ANDROID
	thread->wakePending = 0;
#endif
	LinuxHandleThreadEvents();
}


void *threadProc(void *refcon)
{
	FskThread thread = refcon;
	sigset_t	set;
	Boolean		wrapped = false;
	FskErr		err;

	if ((thread->flags & kFskThreadFlagsJoinable)) {
		FskMutexAcquire(thread->running);
	}
	sigemptyset(&set);
#if !TARGET_OS_ANDROID
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGUSR1+SIGRTMIN);
	sigaddset(&set, SIGRTMIN);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGIO);
#endif
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	pthread_setspecific(gThreadStructKey, thread);

	FskRandomInit();		// random needs to be seeded per-thread

	// have to add the data handler in the context of the thread it
	// needs to run in.
	thread->fifoPort = sFifoPort;
	err = FskNetSocketNewUDP((FskSocket*)(void*)&thread->eventTrigger, "threadWakeHandle");
	if (kFskErrNone != err)
		goto fail;

	while (kFskErrAddressInUse == FskNetSocketBind((FskSocket)thread->eventTrigger, 0x7f000001, thread->fifoPort)) {
		thread->fifoPort++;
		if (thread->fifoPort > 0xffff) {
			if (wrapped)
				goto fail;					// already scanned full range once.
			thread->fifoPort = 1024;		// start scan again above assigned ports.
			wrapped = true;
		}
	}
	FskThreadAddDataHandler(&thread->eventTriggerHandler, (FskThreadDataSource)thread->eventTrigger, threadCheckEvents, true, false, thread);
	sFifoPort = thread->fifoPort + 1;

	// get it running
	(thread->userProc)(thread->userRefcon);

#if TARGET_OS_ANDROID
	gAndroidCallbacks->detachThreadCB();
#endif

fail:
	thread->threadFinished = true;
	// shut it down
#if TARGET_OS_LINUX
	FskThreadRemoveDataHandler(&thread->eventTriggerHandler);
	FskNetSocketClose((FskSocket)thread->eventTrigger);
#endif
	FskListMutexRemove(gThreads, thread);

	FskListMutexDispose(thread->eventQueue);
	if ((thread->flags & kFskThreadFlagsJoinable)) {
		thread->threadIsRunning = false;
		FskMutexRelease(thread->running);
		// can't do any more with thread after this point.
	}
	else {
		FskInstrumentedItemPrintfVerbose(thread, "Non-joinable thread deleting itself on exit: %s", thread->name);

		if (thread->eventHandlers){
			while (thread->eventHandlers->list)
				FskMemPtrDispose(FskListMutexRemoveFirst(thread->eventHandlers));
			FskListMutexDispose(thread->eventHandlers);
		}

		FskAssociativeArrayDispose(thread->environmentVariables);
		FskExtensionsTerminateThread(thread);
		FskInstrumentedItemDispose(thread);
		FskMemPtrDispose(thread);
	}

	pthread_exit(NULL);
	return NULL;	/* Include this just to avoid warnings, even though it will be never erached. */
}

#define kMAX_FDS		128
static void LinuxThreadWaitForData(SInt32 msec)
{
	FskThread thread;
	FskThreadDataHandler cur;
#if USE_POLL
	FskThreadDataHandler dh[kMAX_FDS];
	struct pollfd fds[kMAX_FDS];
#else
	fd_set readSet, writeSet;
	struct timeval tv;
	struct timeval *tvp;
#endif
	int max = 0, num;
	static int maxmax = 0;

	thread = FskThreadGetCurrent();
#if USE_POLL
	cur = NULL;
	while (NULL != (cur = FskListGetNext(thread->dataHandlers, cur)))
		max++;

	FskMemSet(fds, 0, sizeof(struct pollfd) * kMAX_FDS);
	max = 0;
	while (NULL != (cur = FskListGetNext(thread->dataHandlers, cur))) {
		dh[max] = cur;
		cur->inUse = true;
		cur->cycle = thread->cycle;
		fds[max].fd = cur->source->dataNode;
		fds[max].events |= POLLERR;
		fds[max].events |= POLLHUP;
		if (cur->wantsReadable)
			fds[max].events |= POLLIN;
		if (cur->wantsWritable)
			fds[max].events |= POLLOUT;
		max++;
	}
	if (max > maxmax) {
		FskInstrumentedTypePrintfVerbose(&gThreadTypeInstrumentation, " -- maxmax fds increased to %d", max);
		maxmax = max;
	}

//	FskRunloopPrintfDebug("about to poll fds: %x, max: %d, msec: %d", fds, max, msec);
	num = poll(fds, max, msec);
	if (num == -1 && errno != 4) {
//		FskRunloopPrintfDebug("poll returns -1, errno: %d", errno);
	}
//	FskRunloopPrintfDebug("poll returns %d fds", num);
	if (num > 0) {
		for (num=0; num<max; num++) {
			cur = dh[num];
			if (!FskListContains(thread->dataHandlers, cur)) {
				FskRunloopPrintfNormal("ACK!! - dataHandler %x went away during the poll", (unsigned)cur);
				continue;
			}
			if (cur->cycle != thread->cycle) {
				FskRunloopPrintfNormal("ACK!! - dataHandler %x went away, and came back on a different cycle %x vs. %d", (unsigned)cur, (unsigned)cur->cycle, thread->cycle);
				continue;
			}
			if (fds[num].revents & POLLIN) {
				cur->source->pendingReadable = true;
				if (cur->source->isSocket) {
					FskRunloopPrintfDebug(" [%x] pendingReadable on fd %d  -- readable on socket %s", cur, fds[num].fd, ((FskSocket)(cur->source))->debugName);
				}
				else {
					FskRunloopPrintfDebug(" [%x] pendingReadable on fd %d ", cur, fds[num].fd);
				}

			}
			if (fds[num].revents & POLLOUT) {
				cur->source->pendingWritable = true;
				FskRunloopPrintfDebug("[%x] pendingWritable on fd %d", cur, fds[num].fd);
			}
			if ((fds[num].revents & POLLERR) || (fds[num].revents & POLLHUP)) {
				cur->source->pendingException = true;
				FskRunloopPrintfDebug("[%x] pendingException on fd %d", cur, fds[num].fd);
			}
		}

		for (num=0; num<max; num++) {
			Boolean callit = false;
			cur = dh[num];
			if (!FskListContains(thread->dataHandlers, cur)) {
				FskRunloopPrintfMinimal("ACK!!!! TRYING TO CHECK A DATA HANDLER THAT HAS BEEN REMOVED %x", (unsigned)cur);
				continue;
			}
			if (cur->cycle != thread->cycle) {
				FskRunloopPrintfMinimal("ACK!!!! data handler %x now has a different cycle %d vs. %d", cur->cycle, thread->cycle);
				goto endCycle;
			}
			if (cur->pendingDispose) {
				FskRunloopPrintfMinimal("ACK!!!! data handler pendingDispose (%x)", cur);
				goto endCycle;
			}
			if (cur->source->pendingReadable && cur->wantsReadable) {
				callit = true;
				if (! cur->source->isSocket)
					cur->source->pendingReadable = false;
			}
			if (cur->source->pendingWritable && cur->wantsWritable) {
				callit = true;
				if (! cur->source->isSocket)
					cur->source->pendingWritable = false;
			}
			if (cur->source->pendingException) {
				callit = true;
				cur->source->pendingException = false;
			}

			if (callit) {
				FskRunloopPrintfDebug(" calling callback on dataHandler: [%x] fd %d", cur, fds[num].fd);
				if (FskListContains(thread->dataHandlers, cur))
					(*cur->callback)(cur, cur->source, cur->refCon);
			}
endCycle:
			cur->inUse = false;
		}
	}

	for (num=0; num<max; num++) {
		cur = dh[num];
		if (cur->pendingDispose)
			FskThreadRemoveDataHandler(&cur);
	}

#else
	if (msec < 0) {
		tvp = NULL;
	}
	else {
		tvp = &tv;
		if (msec > 1000) {
			tv.tv_sec = msec / 1000;
			msec -= tv.tv_sec * 1000;
		}
		else
			tv.tv_sec = 0;
		tv.tv_usec = msec * 1000;
	}
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	cur = NULL;
	while (NULL != (cur = FskListGetNext(thread->dataHandlers, cur))) {
		if (!cur)
			break;
		if (cur->wantsReadable) FD_SET(cur->source->dataNode, &readSet);
		if (cur->wantsWritable) FD_SET(cur->source->dataNode, &writeSet);
		if (cur->source->dataNode > max) max = cur->source->dataNode;
	}
	num = select(max+1, &readSet, &writeSet, NULL, tvp);
	if (num > 0) {
		cur = NULL;
		while (NULL != (cur = FskListGetNext(thread->dataHandlers, cur))) {
			if (FD_ISSET(cur->source->dataNode, &readSet))
				cur->source->pendingReadable = true;
			if (FD_ISSET(cur->source->dataNode, &writeSet))
				cur->source->pendingWritable = true;
		}
		cur = FskListGetNext(thread->dataHandlers, NULL);
		while (cur) {
			Boolean callit = false;
			next = FskListGetNext(thread->dataHandlers, cur);
			if (cur->source->pendingReadable && cur->wantsReadable) {
				callit = true;
				if (! cur->source->isSocket)
					cur->source->pendingReadable = false;
			}
			if (cur->source->pendingWritable && cur->wantsWritable) {
				callit = true;
				if (! cur->source->isSocket)
					cur->source->pendingWritable = false;
			}
			if (callit)
				(*cur->callback)(cur, cur->source, cur->refCon);
			cur = next;
		}
	}
#endif
}

#elif TARGET_OS_MAC

void *threadProc(void *refcon)
{
	FskThread thread = refcon;
	sigset_t	set;

	if ((thread->flags & kFskThreadFlagsJoinable)) {
		FskMutexAcquire(thread->running);
	}
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGIO);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	pthread_setspecific(gThreadStructKey, thread);

    if (thread->name)
        pthread_setname_np(thread->name);

	FskRandomInit();		// random needs to be seeded per-thread

	FskCocoaThreadInitialize(thread);

	FskInstrumentedItemSendMessage(thread, kFskThreadInstrMsgStart, thread);
	// get it running
	(thread->userProc)(thread->userRefcon);

	FskCocoaThreadTerminate(thread);

	thread->threadFinished = true;
	FskInstrumentedItemSendMessage(thread, kFskThreadInstrMsgFinished, thread);
	// shut it down
	if (gThreads)
		FskListMutexRemove(gThreads, thread);

	FskListMutexDispose(thread->eventQueue);
	if ((thread->flags & kFskThreadFlagsJoinable)) {
		thread->threadIsRunning = false;
		FskMutexRelease(thread->running);
		// can't do any more with thread after this point.
	}
	else {
		FskInstrumentedItemPrintfVerbose(thread, "Non-joinable thread deleting itself on exit: %s", thread->name);

		if (thread->eventHandlers){
			while (thread->eventHandlers->list)
				FskMemPtrDispose(FskListMutexRemoveFirst(thread->eventHandlers));
			FskListMutexDispose(thread->eventHandlers);
		}

		FskAssociativeArrayDispose(thread->environmentVariables);
		FskExtensionsTerminateThread(thread);
		FskInstrumentedItemDispose(thread);
		FskMemPtrDispose(thread);
	}

	return NULL;
}

Boolean LinuxHandleThreadEvents(void) {
	FskThread thread = FskThreadGetCurrent();
	if (thread && thread->eventQueue) {
		FskEvent event = FskListMutexRemoveFirst(thread->eventQueue);
        if (event) {
            HandleThreadEvent(event);
            return true;
        }
    }
    return false;
}

void MacThreadGotSocketData() {
	FskThread thread;
	FskThreadDataHandler cur, next;
	
	thread = FskThreadGetCurrent();
	cur = FskListGetNext(thread->dataHandlers, NULL);
	while (cur) {
		next = FskListGetNext(thread->dataHandlers, cur);
		if (cur->source) {
			FskInstrumentedItemSendMessage(thread, kFskThreadInstrMsgGotSocketData, thread);
			Boolean callit = false;
			next = FskListGetNext(thread->dataHandlers, cur);
			if (cur->source->pendingReadable && cur->wantsReadable) {
				callit = true;
				if (! cur->source->isSocket)
					cur->source->pendingReadable = false;
			}
			if (cur->source->pendingWritable && cur->wantsWritable) {
				callit = true;
				if (! cur->source->isSocket)
					cur->source->pendingWritable = false;
			}
			if (callit && cur->callback) {
				(*cur->callback)(cur, cur->source, cur->refCon);
				break;			// found it.
			}
		}
		cur = next;
	}
}

#elif TARGET_OS_KPL
void *threadProc(void *refcon)
{
	FskThread thread = (FskThread)refcon;

	FskRandomInit();		// random needs to be seeded per-thread

	if ((thread->flags & kFskThreadFlagsJoinable)) {
		FskMutexAcquire(thread->running);
		thread->threadIsRunning = false;
	}

	// get it running
	(thread->userProc)(thread->userRefcon);

	// thread->threadFinished = true;

	KplThreadNotifyClientComplete(thread->kplThread);

	// shut it down
	if (gThreads)
		FskListMutexRemove(gThreads, thread);

	if ((thread->flags & kFskThreadFlagsJoinable)) {
		thread->threadIsRunning = false;
		FskMutexRelease(thread->running);
		thread->threadFinished = true;

	}
	else {
		if (thread->eventHandlers){
			while (thread->eventHandlers->list)
				FskMemPtrDispose(FskListMutexRemoveFirst(thread->eventHandlers));
			FskListMutexDispose(thread->eventHandlers);
		}

		FskAssociativeArrayDispose(thread->environmentVariables);
		FskExtensionsTerminateThread(thread);
		FskInstrumentedItemDispose(thread);
		FskMemPtrDispose(thread);
	}

	return 0;
}
#endif

void FskThreadInitializationComplete(FskThread thread)
{
	thread->threadIsRunning = true;
}

#if TARGET_OS_WIN32

FskErr FskThreadCreateMain(FskThread *threadOut)
{
	FskErr err;
	FskThread thread;

	err = FskListMutexNew(&gThreads, "gThreads");
	if (err) return err;

	gFskThreadTLSSlot = TlsAlloc();

	err = FskMemPtrNewClear(sizeof(FskThreadRecord) + 4, (FskMemPtr *)&thread);
	if (err) return err;

	mainThread = thread;
	mainThread->flags = kFskThreadFlagsIsMain;
	thread->name = thread->nameBuffer;
	FskStrCopy(thread->name, "main");

#if SUPPORT_TIMER_THREAD
	thread->nextCallbackTime.seconds = kFskUInt32Max;
	thread->waitableTimer = CreateEvent(NULL, false, false, NULL);
	timeBeginPeriod(5);
#endif

	FskListMutexNew(&thread->eventHandlers, "threadEventHandlers");

	thread->id = GetCurrentThreadId();
	makeThreadWindow(thread);

	FskListMutexPrepend(gThreads, thread);

	thread->threadIsRunning = true;
	if (threadOut)
		*threadOut = thread;

	FskInstrumentedItemNew(thread, thread->name, &gThreadTypeInstrumentation);
	return kFskErrNone;
}

FskErr FskThreadTerminateMain(void)
{
	FskInstrumentedItemSendMessage(mainThread, kFskThreadInstrMsgFinishedMain, mainThread);

	mainThread->threadFinished = true;
	mainThread->threadIsRunning = false;
	DestroyWindow(mainThread->window);
	if (mainThread->eventHandlers){
		while (mainThread->eventHandlers->list)
			FskMemPtrDispose(FskListMutexRemoveFirst(mainThread->eventHandlers));
		FskListMutexDispose(mainThread->eventHandlers);
	}

	FskAssociativeArrayDispose(mainThread->environmentVariables);
	FskExtensionsTerminateThread(mainThread);
	FskInstrumentedItemDispose(mainThread);
	if (mainThread->name != mainThread->nameBuffer)
		FskMemPtrDispose(mainThread->name);
	FskListRemove(&gThreads->list, mainThread);
	FskMemPtrDisposeAt((void **)&mainThread);

	FskMutexAcquire(gThreads->mutex);
	FskListMutexDispose(gThreads);
	gThreads = NULL;
	TlsSetValue(gFskThreadTLSSlot, 0);

	return kFskErrNone;
}

#elif TARGET_OS_LINUX

FskErr FskThreadCreateMain(FskThread *threadOut)
{
	FskErr err;
	FskThread thread;

	err = FskListMutexNew(&gThreads, "gThreads");
	if (err) return err;

	err = FskMemPtrNewClear(sizeof(FskThreadRecord) + 4, (FskMemPtr*)(void*)&thread);
	if (err) return err;

	mainThread = thread;
	thread->name = thread->nameBuffer;
	FskStrCopy(thread->name, "main");
	thread->flags = kFskThreadFlagsIsMain;
	thread->pthread = pthread_self();
	{
		// other threads are interested in USR1 and IO, not main
		sigset_t	set;
		sigemptyset(&set);
#if !TARGET_OS_ANDROID
		sigaddset(&set, SIGUSR1);
		sigaddset(&set, SIGUSR1+SIGRTMIN);
#endif
		sigaddset(&set, SIGIO);
		pthread_sigmask(SIG_BLOCK, &set, NULL);
	}
	pthread_key_create(&gThreadStructKey, NULL);
	pthread_setspecific(gThreadStructKey, thread);
	FskInstrumentedItemPrintfVerbose(thread, "main thread: %p", thread);

	FskListMutexPrepend(gThreads, thread);
	FskListMutexNew(&thread->eventQueue, "threadEventQueue");
	FskListMutexNew(&thread->eventHandlers, "threadEventHandlers");

	thread->fifoPort = sFifoPort;
	FskNetSocketNewUDP((FskSocket*)(void*)&thread->eventTrigger, "threadWakeHandle");

	while (kFskErrAddressInUse == FskNetSocketBind((FskSocket)thread->eventTrigger, 0x7f000001, thread->fifoPort)) {
		// couldn't bind to UDP socket on port try next
		thread->fifoPort++;
	}
	FskThreadAddDataHandler(&thread->eventTriggerHandler, (FskThreadDataSource)thread->eventTrigger, threadCheckEvents, true, false, thread);
	sFifoPort = thread->fifoPort + 1;

	thread->threadIsRunning = true;
	if (threadOut)
		*threadOut = thread;

	FskInstrumentedItemNew(thread, thread->name, &gThreadTypeInstrumentation);
	return kFskErrNone;
}

FskErr FskThreadTerminateMain(void)
{
	FskEvent event;

	FskInstrumentedItemSendMessage(mainThread, kFskThreadInstrMsgFinishedMain, mainThread);
	FskMutexAcquire(gThreads->mutex);

	mainThread->threadIsRunning = false;
	mainThread->threadFinished = true;
#if TARGET_OS_LINUX
	FskThreadRemoveDataHandler(&mainThread->eventTriggerHandler);
	FskNetSocketClose((FskSocket)mainThread->eventTrigger);
#endif
	if (mainThread->eventHandlers){
		while (mainThread->eventHandlers->list)
			FskMemPtrDispose(FskListMutexRemoveFirst(mainThread->eventHandlers));
		FskListMutexDispose(mainThread->eventHandlers);
	}

	while (NULL != (event = FskListMutexRemoveFirst(mainThread->eventQueue)))
		FskEventDispose(event);
	FskListMutexDispose(mainThread->eventQueue);

	FskAssociativeArrayDispose(mainThread->environmentVariables);
	FskExtensionsTerminateThread(mainThread);
	FskInstrumentedItemDispose(mainThread);
	FskMemPtrDisposeAt((void**)(void*)&mainThread);

	FskListMutexDispose(gThreads);
	gThreads = NULL;

	return kFskErrNone;
}

#elif TARGET_OS_MAC

FskErr FskThreadCreateMain(FskThread *threadOut)
{
	FskErr err;
	FskThread thread;

	err = FskListMutexNew(&gThreads, "gThreads");
	if (err) return err;

	err = FskMemPtrNewClear(sizeof(FskThreadRecord) + 4, (FskMemPtr *)&thread);
	if (err) return err;

	mainThread = thread;
	thread->name = thread->nameBuffer;
	FskStrCopy(thread->name, "main");
	thread->flags = kFskThreadFlagsIsMain;
	thread->pthread = pthread_self();
	{
		// other threads are interested in USR1 and IO, not main
		sigset_t	set;
		sigemptyset(&set);
		sigaddset(&set, SIGUSR1);
		sigaddset(&set, SIGIO);
		pthread_sigmask(SIG_BLOCK, &set, NULL);
	}
	pthread_key_create(&gThreadStructKey, NULL);
	pthread_setspecific(gThreadStructKey, thread);
	FskInstrumentedTypePrintfVerbose(&gThreadTypeInstrumentation, "main thread: %p", thread);

	FskListMutexPrepend(gThreads, thread);
	FskListMutexNew(&thread->eventQueue, "threadEventQueue");
	FskListMutexNew(&thread->eventHandlers, "threadEventHandlers");

	FskCocoaThreadInitialize(thread);

	thread->threadIsRunning = true;
	if (threadOut)
		*threadOut = thread;

	FskRandomInit();		// random needs to be seeded per-thread

	FskInstrumentedItemNew(thread, thread->name, &gThreadTypeInstrumentation);
	return kFskErrNone;
}

FskErr FskThreadTerminateMain(void)
{
	FskEvent event;

	FskInstrumentedItemSendMessage(mainThread, kFskThreadInstrMsgFinishedMain, mainThread);

	mainThread->threadIsRunning = false;
	mainThread->threadFinished = true;
	if (mainThread->eventHandlers){
		while (mainThread->eventHandlers->list)
			FskMemPtrDispose(FskListMutexRemoveFirst(mainThread->eventHandlers));
		FskListMutexDispose(mainThread->eventHandlers);
	}

	while (NULL != (event = FskListMutexRemoveFirst(mainThread->eventQueue)))
		FskEventDispose(event);
	FskListMutexDispose(mainThread->eventQueue);

	FskAssociativeArrayDispose(mainThread->environmentVariables);
	FskExtensionsTerminateThread(mainThread);
	FskInstrumentedItemDispose(mainThread);
	FskListRemove(&gThreads->list, mainThread);
	FskMemPtrDisposeAt(&mainThread);

	FskMutexAcquire(gThreads->mutex);
	FskListMutexDispose(gThreads);
	gThreads = NULL;

	return kFskErrNone;
}

#elif TARGET_OS_KPL
FskErr FskThreadCreateMain(FskThread *threadOut)
{
	FskErr err;
	FskThread thread;
	KplThread kplThread = NULL;

	err = FskListMutexNew(&gThreads, "gThreads"); 
	BAIL_IF_ERR(err);

	err = FskMemPtrNewClear(sizeof(FskThreadRecord) + 4, (FskMemPtr *)&thread);
	BAIL_IF_ERR(err);

	mainThread = thread;
	mainThread->flags = kFskThreadFlagsIsMain;
	thread->name = thread->nameBuffer;
	FskStrCopy(thread->name, "main");

	FskListMutexPrepend(gThreads, thread);

	err = KplThreadCreateMain(&kplThread);
	BAIL_IF_ERR(err);

	thread->kplThread = kplThread;

	FskListMutexNew(&thread->eventHandlers, "threadEventHandlers");

	thread->threadIsRunning = true;
	if (threadOut)
		*threadOut = thread;

	FskInstrumentedItemNew(thread, thread->name, &gThreadTypeInstrumentation);

bail:

	if (err != kFskErrNone) {
		if (kplThread)
			KplThreadTerminateMain();
	}

	return err;
}

FskErr FskThreadTerminateMain(void)
{
	FskInstrumentedItemSendMessage(mainThread, kFskThreadInstrMsgFinishedMain, mainThread);

	KplThreadTerminateMain();

	mainThread->threadFinished = true;
	mainThread->threadIsRunning = false;
	if (mainThread->eventHandlers){
		while (mainThread->eventHandlers->list)
			FskMemPtrDispose(FskListMutexRemoveFirst(mainThread->eventHandlers));
		FskListMutexDispose(mainThread->eventHandlers);
	}

	FskAssociativeArrayDispose(mainThread->environmentVariables);
	FskExtensionsTerminateThread(mainThread);
	FskInstrumentedItemDispose(mainThread);
	if (mainThread->name != mainThread->nameBuffer)
		FskMemPtrDispose(mainThread->name);
	FskListRemove(&gThreads->list, mainThread);
	FskMemPtrDisposeAt((void **)&mainThread);

	FskMutexAcquire(gThreads->mutex);
	FskListMutexDispose(gThreads);
	gThreads = NULL;

	return kFskErrNone;
}

void FskThreadNotifySocketData(int platSkt)
{
	FskThreadDataHandler handler = FskThreadFindDataHandlerBySourceNode(platSkt);

	if (handler) {
		Boolean callit = false;
		if (handler->source->pendingReadable && handler->wantsReadable) {
			callit = true;
			if (! handler->source->isSocket)
				handler->source->pendingReadable = false;
		}
		if (handler->source->pendingWritable && handler->wantsWritable) {
			callit = true;
			if (! handler->source->isSocket)
				handler->source->pendingWritable = false;
		}
		if (handler->source->pendingClose)
			callit = true;
		if (callit)
			(*handler->callback)(handler, handler->source, handler->refCon);
	}
}
#endif

FskThread FskThreadGetMain(void)
{
	return mainThread;
}

const char* FskThreadName(FskThread thread)
{
	return thread ? thread->name : NULL;
}

char *FskThreadFormatDiagnostic(char *msg, ...)
{
#if SUPPORT_XS_DEBUG
    va_list arguments;
    FskThread thread = FskThreadGetCurrent();

    va_start(arguments, msg);
    vsnprintf(thread->debugScratch, sizeof(thread->debugScratch), msg, arguments);
    va_end(arguments);

    return thread->debugScratch;
#else
    static char zero = 0;
    return &zero;
#endif
}

/*
	Condition
*/

FskErr FskConditionNew(FskCondition *conditionOut)
{
	FskErr err;
	FskCondition condition = NULL;

	err = FskMemPtrNewClear(sizeof(FskConditionRecord), (FskMemPtr*)(void*)&condition);
	BAIL_IF_ERR(err);

	FskInstrumentedItemNew(condition, NULL, &gConditionTypeInstrumentation);

#if TARGET_OS_LINUX || TARGET_OS_MAC
	pthread_cond_init(&condition->cond, NULL);
#elif TARGET_OS_WIN32
	condition->semaphore = CreateSemaphore(0, 1, 0x7FFFFFF, NULL);
	if ((HANDLE)ERROR_INVALID_HANDLE == condition->semaphore) {
		condition->semaphore = NULL;
		BAIL(-1);
	}

	condition->waitersEvent = CreateEvent(0, false, false, NULL);
	if ((HANDLE)ERROR_INVALID_HANDLE == condition->waitersEvent) {
		condition->waitersEvent = NULL;
		BAIL(-1);
	}

	InitializeCriticalSection(&condition->waitersMutex);
#elif TARGET_OS_KPL
	err = KplConditionNew((KplCondition*)&condition->kplCond);
	BAIL_IF_ERR(err);
#endif

bail:
	if (err) {
		FskConditionDispose(condition);
		condition = NULL;
	}
	*conditionOut = condition;

	return err;
}

FskErr FskConditionDispose(FskCondition condition)
{
	if (NULL == condition)
		return kFskErrNone;

#if TARGET_OS_LINUX || TARGET_OS_MAC
	pthread_cond_destroy(&condition->cond);
#elif TARGET_OS_WIN32
	DeleteCriticalSection(&condition->waitersMutex);
	if (NULL != condition->waitersEvent)
		CloseHandle(condition->waitersEvent);
	if (NULL != condition->semaphore)
		CloseHandle(condition->semaphore);
#elif TARGET_OS_KPL
	KplConditionDispose(condition->kplCond);
#endif

	FskInstrumentedItemDispose(condition);
	FskMemPtrDispose(condition);

	return kFskErrNone;
}

FskErr FskConditionSignal(FskCondition condition)
{
	FskInstrumentedItemSendMessage(condition, kFskConditionInstrMsgSignal, condition);
#if TARGET_OS_LINUX || TARGET_OS_MAC
	pthread_cond_signal(&condition->cond);
#elif TARGET_OS_WIN32
	EnterCriticalSection(&condition->waitersMutex);
	if (condition->waitersCount > 0)
		ReleaseSemaphore(condition->semaphore, 1, 0);
	LeaveCriticalSection(&condition->waitersMutex);
#elif TARGET_OS_KPL
	KplConditionSignal(condition->kplCond);
#endif

	return kFskErrNone;
}

FskErr FskConditionWait(FskCondition condition, FskMutex mutex)
{
	FskInstrumentedItemSendMessage(condition, kFskConditionInstrMsgWait, condition);
#if TARGET_OS_LINUX || TARGET_OS_MAC
	pthread_cond_wait(&condition->cond, &mutex->mutex);
#elif TARGET_OS_WIN32
	{
	WORD aWaitersFlag = 0;

	EnterCriticalSection(&condition->waitersMutex);
	condition->waitersCount++;
	LeaveCriticalSection(&condition->waitersMutex);

	FskMutexRelease(mutex);
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

	FskMutexAcquire(mutex);
	}
#elif TARGET_OS_KPL
	KplConditionWait(condition->kplCond, mutex->kplMutex);
#endif

	return kFskErrNone;
}

FskErr FskConditionTimedWait(FskCondition condition, FskMutex mutex, FskTime timeout)
{
#if TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_ANDROID
	struct timespec sleep;
	struct timeval day;
	struct timezone tz;
	gettimeofday(&day, &tz);
	TIMEVAL_TO_TIMESPEC(&day, &sleep);
	sleep.tv_sec += timeout->seconds;
	sleep.tv_nsec += timeout->useconds * kFskTimeNsecPerUsec;
	if (sleep.tv_nsec > kFskTimeNsecPerSec) {
		sleep.tv_sec++;
		sleep.tv_nsec -= kFskTimeNsecPerSec;
	}
	FskInstrumentedItemSendMessage(condition, kFskConditionInstrMsgTimedWait, condition);
	if (ETIMEDOUT == pthread_cond_timedwait(&condition->cond, &mutex->mutex, &sleep)) {
		FskInstrumentedItemSendMessage(condition, kFskConditionInstrMsgTimedWaitTimedOut, condition);
	}
	else {
		FskInstrumentedItemSendMessage(condition, kFskConditionInstrMsgTimedWaitSignaled, condition);
	}

#elif TARGET_OS_WIN32
	WORD aWaitersFlag = 0;

	FskInstrumentedItemSendMessage(condition, kFskConditionInstrMsgWait, condition);
	EnterCriticalSection(&condition->waitersMutex);
	condition->waitersCount++;
	LeaveCriticalSection(&condition->waitersMutex);

	FskMutexRelease(mutex);
	WaitForSingleObject(condition->semaphore, (timeout->seconds * 100) + (timeout->useconds / 1000));

	EnterCriticalSection(&condition->waitersMutex);
	condition->waitersCount--;
	if (condition->waitersFlag && (0 == condition->waitersCount)) {
		condition->waitersFlag = 0;
		aWaitersFlag = 1;
	}
	LeaveCriticalSection(&condition->waitersMutex);
	if (aWaitersFlag)
		SetEvent(condition->waitersEvent);

	FskInstrumentedItemSendMessage(condition, kFskConditionInstrMsgTimedWaitSignaled, condition);

	FskMutexAcquire(mutex);
#elif TARGET_OS_KPL
	KplConditionTimedWait(condition->kplCond, mutex->kplMutex, (KplTime)timeout);
#endif

	return kFskErrNone;
}

FskErr FskConditionBroadcast(FskCondition condition)
{
#if TARGET_OS_LINUX || TARGET_OS_MAC
	FskInstrumentedItemSendMessage(condition, kFskConditionInstrMsgBroadcast, condition);
	pthread_cond_broadcast(&condition->cond);
#elif TARGET_OS_WIN32
	DWORD aWaitersCount;

	FskInstrumentedItemSendMessage(condition, kFskConditionInstrMsgBroadcast, condition);
	EnterCriticalSection(&condition->waitersMutex);
	aWaitersCount = condition->waitersCount;
	if (aWaitersCount > 0) {
		condition->waitersFlag = 1;
		ReleaseSemaphore(condition->semaphore, condition->waitersCount, 0);
	}
	LeaveCriticalSection(&condition->waitersMutex);
	if (aWaitersCount > 0)
		WaitForSingleObject(condition->waitersEvent, INFINITE);
#endif

	return kFskErrNone;
}

#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageFskThread(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	FskThread thread = (FskThread)msgData;
	FskThread cur = FskThreadGetCurrent();


	if (cur && cur->name && cur->name[0]) {
		int sz;
		sz = snprintf(buffer, bufferSize, "[%s] ", cur->name);
		bufferSize -= sz;
		buffer += sz;
	}

	switch (msg) {
		case kFskThreadInstrMsgWake:
            snprintf(buffer, bufferSize, "waking thread %s", thread->name);
            return true;
		case kFskThreadInstrMsgWaitForInit:
            snprintf(buffer, bufferSize, "%s waiting for init completion", thread->name);
            return true;
		case kFskThreadInstrMsgDoneInit:
            snprintf(buffer, bufferSize, "%s finished completion", thread->name);
            return true;
		case kFskThreadInstrMsgJoin:
			{
			FskThread current = FskThreadGetCurrent();
			snprintf(buffer, bufferSize, "%s joining %s", current ? current->name : "(unknown)", thread->name);
			}
            return true;
		case kFskThreadInstrMsgJoined:
            snprintf(buffer, bufferSize, "%s joined", thread->name);
            return true;
		case kFskThreadInstrMsgJoinNonJoinable:
            snprintf(buffer, bufferSize, "trying to join nonjoinable %s", thread->name);
            return true;
		case kFskThreadInstrMsgAddEventHandler:
            snprintf(buffer, bufferSize, "adding event handler to %s", thread->name);
            return true;
		case kFskThreadInstrMsgRemoveEventHandler:
            snprintf(buffer, bufferSize, "remove event handler on %s", thread->name);
            return true;
		case kFskThreadInstrMsgBroadcastEvent:
            snprintf(buffer, bufferSize, "broadcasting event to %s", thread->name);
            return true;
		case kFskThreadInstrMsgPostEvent:
            snprintf(buffer, bufferSize, "post event to %s", thread->name);
            return true;
		case kFskThreadInstrMsgPostCallback:
            snprintf(buffer, bufferSize, "post callback to thread %s from function %s", ((FskThread)((void **)msgData)[0])->name, ((char **)msgData)[1]);
            return true;
		case kFskThreadInstrMsgStart:
            snprintf(buffer, bufferSize, "thread %s started", thread->name);
            return true;
		case kFskThreadInstrMsgFinished:
            snprintf(buffer, bufferSize, "thread %s finished", thread->name);
            return true;
		case kFskThreadInstrMsgGotSocketData:
            snprintf(buffer, bufferSize, "thread %s got socket data", thread->name);
            return true;
		case kFskThreadInstrMsgFinishedMain:
            snprintf(buffer, bufferSize, "main thread finished");
            return true;

		case kFskConditionInstrMsgWait:
            snprintf(buffer, bufferSize, "conditionWait");
            return true;
		case kFskConditionInstrMsgTimedWait:
            snprintf(buffer, bufferSize, "conditionTimedWait");
            return true;
		case kFskConditionInstrMsgTimedWaitSignaled:
            snprintf(buffer, bufferSize, "conditionTimedWait - signaled");
            return true;
		case kFskConditionInstrMsgTimedWaitTimedOut:
            snprintf(buffer, bufferSize, "conditionTimedWait - timed out");
            return true;
		case kFskConditionInstrMsgBroadcast:
            snprintf(buffer, bufferSize, "condition broadcast");
            return true;
	}

	return false;
}
#endif
