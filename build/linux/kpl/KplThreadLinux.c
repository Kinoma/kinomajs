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
#define __FSKUTILITIES_PRIV__
#define __FSKNETUTILS_PRIV__
#define __FSKTHREAD_PRIV__
#define __FSKWINDOW_PRIV__
#define __FSKTIME_PRIV__
#define __FSKPORT_PRIV__
#define __FSKEVENT_PRIV__
#define __FSKEXTENSIONS_PRIV__
#include "KplThread.h"
#include "KplSynchronization.h"

#include "KplTime.h"
#include "KplThreadLinuxPriv.h"
#include "KplTimeCallbackLinuxPriv.h"

#include "FskNetUtils.h"
#include "FskThread.h"

#include "FskList.h"

#include "poll.h"
#include "signal.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>

#include "FskWindow.h"
#include "FskMain.h"	// To access gQuitting global

#if BG3CDP || MINITV
#include "sys/prctl.h"
#endif

#if SUPPORT_INSTRUMENTATION
FskInstrumentedTypeRecord gKplThreadTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "KplThread"};
#endif

#define THREAD_STACK_SIZE (128 * 1024)
static void *kplThreadProc(void *refcon);
static void LinuxThreadWaitForData(KplThread self, SInt32 msec);

FskListMutex gKplThreads = NULL;

static pthread_key_t gThreadStructKey;

static KplThread mainThread = NULL;

static void threadCheckForEvents(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon) {
    KplThread thread = (KplThread)refCon;
    int count = FskListMutexCount(thread->eventQueue);
    uint64_t ignore;

    if (thread->eventfd >= 0)
        read(thread->eventfd, &ignore, sizeof(ignore));
    else {
        while (true) {
            char junk[1024];
            int amt = read(thread->pipe[0], junk, sizeof(junk));
            if (amt <= 0)
                break;
        }
    }

    // service all events signaled at this point; wait to next time to handle any that arrive after this point
    while (count--) {
        FskEvent event = FskListMutexRemoveFirst(thread->eventQueue);
        if (!event) break;
        FskHandleThreadEvent(event);
    }
}

FskErr KplThreadCreate(KplThread *threadOut, KplThreadProc procedure, void *refcon, UInt32 flags, const char *name)
{
	FskErr			err;
	KplThread	thread = NULL;
	
	err = FskMemPtrNewClear(sizeof(KplThreadRecord) + FskStrLen(name) + 1, (FskMemPtr*)&thread);
	if (err) goto bail;
	

	thread->flags = flags;
	thread->clientProc = procedure;
	thread->clientRefCon = refcon;
	thread->name = thread->nameBuffer;
	FskStrCopy(thread->name, name);

	FskListMutexPrepend(gKplThreads, thread);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
    if (thread->flags & kFskThreadFlagsJoinable ) {
        // create as joinable so we can use the pthread_join() call.  -tlee
	    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    } else { 
	    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }
	pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE);

	if (thread->flags & kFskThreadFlagsHighPriority) {
		struct sched_param sched_level;
		int min, max;
		int policy = SCHED_OTHER;

		min = sched_get_priority_min(policy);
		max = sched_get_priority_max(policy);
		max = (min + max) / 2;
		pthread_attr_setschedpolicy(&attr, policy);
		sched_level.sched_priority = max;
		pthread_attr_setschedparam(&attr, &sched_level);
	}
//	else if (thread->flags & kFskThreadFlagsLowPriority) {
//	}

    if (!(thread->flags & kFskThreadFlagsTransientWorker)) {
		err = FskListMutexNew(&thread->eventQueue, "threadEventQueue");
		if (err) goto bail;
    }
	*threadOut = (KplThread)thread;
	
	if (0 != pthread_create(&thread->pthread, &attr, kplThreadProc, thread))
		err = kFskErrOperationFailed;

	pthread_attr_destroy(&attr);
	if (err) goto bail;

bail:
	if (err) {
		KplThreadJoin(thread);
		*threadOut = NULL;
	}

	return err;
}

FskErr KplThreadJoin(KplThread kplThread)
{
	KplThread thread = (KplThread)kplThread;
	
	if (!thread) return kFskErrNone;

    // Call pthread_join to wait for the KPL thread to terminate.
	pthread_join(thread->pthread, NULL);
	
	if (NULL != gKplThreads)
		FskListMutexRemove(gKplThreads, thread);
	
	FskMemPtrDispose((FskMemPtr)thread);
	
	return kFskErrNone;
}

KplThread KplThreadGetCurrent(void)
{
	KplThread thr;

	thr = (KplThread)pthread_getspecific(gThreadStructKey);
	if (!thr)
		thr = mainThread;

	return thr;
}

void KplThreadYield(void)
{
	usleep(10);
	sched_yield();
}

void KplThreadWake(KplThread kplThread)
{
    if (kplThread->eventfd >= 0) {
        uint64_t one = 1;
        write(kplThread->eventfd, &one, sizeof(one));
    }
    else {
        static int count = 1;
        char msg[30];
        
        FskStrNumToStr(count++, msg, sizeof(msg));
        FskStrCat(msg, "-wake");
        write(kplThread->pipe[1], msg, strlen(msg));
    }
}

void KplThreadPostEvent(KplThread kplThread, void *event)
{
    FskListMutexAppend(kplThread->eventQueue, event);
    KplThreadWake(kplThread);
}

FskErr KplThreadCreateMain(KplThread *kplThread)
{
	KplThread thread = NULL;
	FskErr err;
	sigset_t set;

	err = FskMemPtrNewClear(sizeof(KplThreadRecord), (FskMemPtr*)&thread);
	if (err) goto bail;
	
	err = FskListMutexNew(&gKplThreads, "gKplThreads");
	if (err) goto bail;

	mainThread = thread;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGIO);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	pthread_key_create(&gThreadStructKey, NULL);
	pthread_setspecific(gThreadStructKey, thread);

#if BG3CDP || MINITV
	prctl(PR_SET_NAME, thread->name, 0, 0, 0, 0);
#endif

	FskListMutexPrepend(gKplThreads, thread);
	FskListMutexNew(&thread->eventQueue, "threadEventQueue");

    thread->eventfd = eventfd(0, EFD_NONBLOCK);
    FskInstrumentedTypePrintfDebug(&gKplThreadTypeInstrumentation, "eventfd %d allocated in KPL main thread %p\n", thread->eventfd, thread);
    if (thread->eventfd < 0)
        pipe2(thread->pipe, O_NONBLOCK);
	
bail:
	if (err)
		FskMemPtrDisposeAt(&thread);
	
	*kplThread = (KplThread)thread;
	
	return err;
}

void *kplThreadProc(void *refcon)
{
    KplThread thread = refcon;
    sigset_t    set;
	FskEvent 	event;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR1+SIGRTMIN);
    sigaddset(&set, SIGRTMIN);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGIO);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    pthread_setspecific(gThreadStructKey, thread);

#if BG3CDP
	prctl(PR_SET_NAME, thread->name, 0, 0, 0, 0);
#endif

    thread->eventfd = eventfd(0, EFD_NONBLOCK);
    FskInstrumentedTypePrintfDebug(&gKplThreadTypeInstrumentation, "eventfd %d allocated in KPL thread %p\n", thread->eventfd, thread);
    if (thread->eventfd < 0)
        pipe2(thread->pipe, O_NONBLOCK);

    // get it running
    (thread->clientProc)(thread->clientRefCon);

    // shut it down
    if (thread->eventfd >= 0)
        close(thread->eventfd);
    else {
        close(thread->pipe[0]);
        close(thread->pipe[1]);
    }
    FskListMutexRemove(gKplThreads, thread);
	
	if (thread->eventQueue) {
		while (NULL != (event = FskListMutexRemoveFirst(thread->eventQueue)))
			FskEventDispose(event);
		FskListMutexDispose(thread->eventQueue);
	}
	
    if (thread->flags & kFskThreadFlagsTransientWorker) {
		FskMemPtrDispose((FskMemPtr)thread);
    }

    pthread_exit(NULL);
}

void KplThreadNotifyClientComplete(KplThread kplThread)
{
}

FskErr KplThreadTerminateMain(void)
{
	FskEvent	event;
	KplThread	thread;
	
	FskMutexAcquire(gKplThreads->mutex);

    if (mainThread->eventfd >= 0)
        close(mainThread->eventfd);
    else {
        close(mainThread->pipe[0]);
        close(mainThread->pipe[1]);
    }

	while (NULL != (event = FskListMutexRemoveFirst(mainThread->eventQueue)))
		FskEventDispose(event);
	FskListMutexDispose(mainThread->eventQueue);

	FskMutexRelease(gKplThreads->mutex);
	while (NULL != (thread = FskListMutexRemoveFirst(gKplThreads)))
		FskMemPtrDispose(thread);
	
	mainThread = NULL;
	
	FskListMutexDispose(gKplThreads);
	gKplThreads = NULL;

	return kFskErrNone;
}

void *KplThreadGetRefcon(KplThread kplThread)
{
	return (NULL != kplThread) ? kplThread->clientRefCon : NULL;
}

void KplThreadNotifyPendingSocketData(void *socket, Boolean pendingReadable, Boolean pendingWritable)
{
	// No implementation required for Linux build
}

FskErr KplThreadRunloopCycle(SInt32 msec)
{
	KplThread		thread;
	FskEvent		event;
	SInt32			waitTimeMS;
	FskTimeRecord	nextEventTime;

	FskInstrumentedTypePrintfDebug(&gKplThreadTypeInstrumentation, "KplThreadRunloopCycle START");

	thread = KplThreadGetCurrent();

	FskInstrumentedTypePrintfDebug(&gKplThreadTypeInstrumentation, "Handling time callbacks");

	// Handle time callbacks that expired before the first Window event
	// We need to properly sequence the events.
	FskWindowGetNextEventTime(FskThreadGetCurrent(), &nextEventTime);
	KplTimeCallbackServiceUntil(thread, (KplTime)&nextEventTime);

	FskInstrumentedTypePrintfDebug(&gKplThreadTypeInstrumentation, "Handling thread event START");

	// De-queue and handle system events here.
	// We only handle the events in the queue at this point, since the client may post more events from within the call to FskHandleThreadEvent().
	FskMutexAcquire(thread->eventQueue->mutex);
	event = thread->eventQueue->list;
	thread->eventQueue->list = NULL;
	FskMutexRelease(thread->eventQueue->mutex);

#if SUPPORT_INSTRUMENTATION
	if (event)
		FskInstrumentedTypePrintfDebug(&gKplThreadTypeInstrumentation, "There are %u events in the queue", (unsigned)FskListCount(event));
#endif

	while (NULL != event) {
		FskEvent next = event->next;
		FskInstrumentedTypePrintfDebug(&gKplThreadTypeInstrumentation, "Handling thread event");
		FskHandleThreadEvent(event);	// system events
		event = next;
	}

	FskInstrumentedTypePrintfDebug(&gKplThreadTypeInstrumentation, "Handling thread event DONE");

	// Don't wait if there are window events to process or if we are trying to shutdown
	if (FskWindowCheckEvents() || gQuitting) {
		waitTimeMS = 0;
	}
	else {
		// Calculate maximum wait for socket and native events
		waitTimeMS = KplTimeCallbackGetNextTimeDelta();
	}
    
    if ((msec >= 0) && (waitTimeMS > msec))
        waitTimeMS = msec;
	
	LinuxThreadWaitForData(thread, waitTimeMS);

	FskInstrumentedTypePrintfDebug(&gKplThreadTypeInstrumentation, "KplThreadRunloopCycle DONE");
		
	return kFskErrNone;
}

#define kMAX_FDS	128

static void LinuxThreadWaitForData(KplThread self, SInt32 msec)
{
	FskThread thread;
	FskThreadDataHandler cur;
	FskThreadDataHandler dh[kMAX_FDS];
	struct pollfd fds[kMAX_FDS];
	int max = 0, num;

	thread = FskThreadGetCurrent();

	// For this thread, build up POLL fds list from the sockets
	// and other data sources that we're interested in
    dh[0] = NULL;
    if (self->eventfd >= 0)
        fds[0].fd = self->eventfd;
    else
        fds[0].fd = self->pipe[0];
    fds[0].events = POLLIN | POLLERR | POLLHUP;
    max++;
    
	cur = NULL;
	while (NULL != (cur = FskListGetNext(thread->dataHandlers, cur))) {
		dh[max] = cur;
		cur->inUse = true;
		fds[max].fd = cur->source->dataNode;
		fds[max].events = 0;
		fds[max].events |= POLLERR;
		fds[max].events |= POLLHUP;
		if (cur->wantsReadable)
			fds[max].events |= POLLIN;
		if (cur->wantsWritable)
			fds[max].events |= POLLOUT;
		max++;
		if (max >= kMAX_FDS) break; // avoid memory corruption
	}

	// wait for data or timeout
	num = poll(fds, max, msec);
	if (num > 0) {
		// Got some data. See which data handler, and note that it needs to
		// get called.
		for (num=1; num<max; num++) {
			cur = dh[num];
			if (!FskListContains(thread->dataHandlers, cur)) {
				// this dataHandler got disposed of while we were waiting.
				continue;
			}
			if (fds[num].revents & POLLIN)
				cur->source->pendingReadable = true;
			if (fds[num].revents & POLLOUT)
				cur->source->pendingWritable = true;
			if ((fds[num].revents & POLLOUT) || (fds[num].revents & POLLHUP))
				cur->source->pendingException = true;
		}

		// Make the callout to the data handler
        if (fds[0].revents & POLLIN)
            threadCheckForEvents(NULL, NULL, self);
		for (num=1; num<max; num++) {
			Boolean callit = false;
			cur = dh[num];
			if (!FskListContains(thread->dataHandlers, cur)) {
				// this dataHandler got disposed of while we were waiting.
				continue;
			}
			if (cur->pendingDispose) {
				// this dataHandler is slated to be disposed.
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
				(*cur->callback)(cur, cur->source, cur->refCon);
            }
endCycle:
			cur->inUse = false;
		}
	}

    for (num=0; num<max; num++) {
        cur = dh[num];
		if (!FskListContains(thread->dataHandlers, cur)) {
			// this dataHandler got disposed of while we were waiting.
			continue;
		}
        if (cur->pendingDispose)
            FskThreadRemoveDataHandler(&cur);
    }
}
