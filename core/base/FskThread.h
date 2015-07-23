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
#ifndef __FSKTHREADS__
#define __FSKTHREADS__

#include "Fsk.h"

#if TARGET_OS_WIN32
//	#define SUPPORT_WAITABLE_TIMER 1		// preferred for desktop
	#define SUPPORT_TIMER_THREAD 1
#endif


#include "FskUtilities.h"
#include "FskEvent.h"
#include "FskList.h"

#if SUPPORT_INSTRUMENTATION
	#define __FSKTHREAD_PRIV__
#endif

#ifdef __FSKTHREAD_PRIV__
	#include "FskAssociativeArray.h"
	#if TARGET_OS_WIN32
		#include <process.h>
	#endif
#endif

#if TARGET_OS_LINUX || TARGET_OS_MAC
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <signal.h>
	#include <sys/time.h>
	#include <errno.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// thread

typedef struct FskThreadDataHandlerRecord FskThreadDataHandlerRecord;
typedef struct FskThreadDataHandlerRecord *FskThreadDataHandler;

typedef struct FskThreadDataSourceRecord FskThreadDataSourceRecord;
typedef struct FskThreadDataSourceRecord *FskThreadDataSource;

typedef void (*FskThreadProc)(void *refcon);

typedef void (*FskThreadDataReadyCallback)(struct FskThreadDataHandlerRecord *handler, struct FskThreadDataSourceRecord *source, void *refCon);
typedef Boolean (*FskThreadEventHandlerRoutine)(FskEvent event, void *refCon);

#if !defined(__FSKTHREAD_PRIV__) && !SUPPORT_INSTRUMENTATION
	FskDeclarePrivateType(FskThread)
	FskDeclarePrivateType(FskThreadEventHandler)
#else
	#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL
		struct FskThreadRecord;

		struct FskThreadDataSourceRecord {
			FskThreadDataSource	next;
			UInt32		dataNode;		// may need to be 16 on Palm
			Boolean		isSocket;
			Boolean		pendingReadable;
			Boolean		pendingWritable;
			Boolean		pendingClose;
#if USE_POLL
			Boolean		pendingException;
#endif
				/* ... depends on data source ... */
			char		more[4];
		};

		struct FskThreadDataHandlerRecord {
			FskThreadDataHandler		next;
			FskThreadDataSource			source;
			FskThreadDataReadyCallback	callback;
			Boolean						wantsReadable;
			Boolean						wantsWritable;
			void						*refCon;
			struct FskThreadRecord		*owner;
			Boolean						callme;

			Boolean						inUse;
			Boolean						pendingDispose;
#if TARGET_OS_ANDROID
			int                         cycle;
#endif
		};


		typedef struct FskThreadEventHandlerRecord FskThreadEventHandlerRecord;
		typedef struct FskThreadEventHandlerRecord *FskThreadEventHandler;
		struct FskThreadEventHandlerRecord {
			FskThreadEventHandler				next;
			UInt32								eventCode;
			FskThreadEventHandlerRoutine		eventHandler;
			void								*refCon;
			struct FskThreadRecord				*self;
		};

		typedef struct FskThreadRecord FskThreadRecord;
		typedef struct FskThreadRecord *FskThread;

		struct FskThreadRecord {
			FskThread					next;
			UInt32						flags;
			FskInstrumentedItemDeclaration

	#if TARGET_OS_WIN32
			HANDLE						handle;
			unsigned int				id;
			HWND						window;

			UInt32						haveMouseMove;
			MSG							mouseMove;
			FskPointAndTicksRecord		points[50];
			UInt32						nextTicks;

	#if SUPPORT_WAITABLE_TIMER
			HANDLE						waitableTimer;
	#elif SUPPORT_TIMER_THREAD
			HANDLE						waitableTimer;
			FskTimeRecord				nextCallbackTime;
			Boolean						timerSignaled;
	#else
			UINT_PTR					timer;
	#endif
	#elif TARGET_OS_LINUX
			pthread_t		 			pthread;
			FskThreadDataSource			eventTrigger;
			FskThreadDataHandler		eventTriggerHandler;
			int							cycle;
			int							fifoPort;
			int							wakePending;

		#if TARGET_OS_ANDROID
			UInt32						jniEnv;
			UInt32						javaMainWakeCallback;
			UInt32						attachedJava;
			UInt32						play2AndroidClass;
			UInt32						mediaCodecCoreClass;
		#endif

			FskListMutex				eventQueue;

	#elif TARGET_OS_MAC
			pthread_t		 			pthread;
			FskListMutex				eventQueue;

			void						*cfRunloop;
			void						*cfRunLoopSource;
			void						*nsAutoreleasePool;
			CFRunLoopTimerRef			cfRunLoopTimer;
			Boolean						wake;
	#elif TARGET_OS_KPL
			void						*kplThread;
    #endif

			FskList						timerCallbacks;
			FskList						suspendedTimerCallbacks;
			FskList						dataHandlers;

			FskListMutex				eventHandlers;
			FskMutex					running;		// for joinable
			Boolean						threadIsRunning;
			Boolean						threadFinished;

			FskThreadProc				userProc;
			void						*userRefcon;

#if TARGET_OS_MAC
			unsigned int				randomSeed;
#else
			UInt32                      randomSeed;
#endif

			FskAssociativeArray			environmentVariables;

			UInt32						extensionsTypeCount;
			void						*extensions;		// FskExtensionType

#if BG3CDP
			FskTimeRecord	lastSleepTime;
			FskTimeRecord	lastLogTime;
#endif

#if SUPPORT_XS_DEBUG
            char                        debugScratch[2048];
#endif

			char						name[1];		// must be last
		};
	#endif
#endif

enum {
	kFskThreadFlagsDefault			= 0x00000000,

	kFskThreadFlagsTransientWorker	= 0x00000001,
	kFskThreadFlagsJoinable			= 0x00000002,
	kFskThreadFlagsIsMain			= 0x00000004,
	kFskThreadFlagsWaitForInit		= 0x00000008,
	kFskThreadFlagsHighPriority		= 0x10000000,
	kFskThreadFlagsLowPriority		= 0x20000000
};

#ifdef __FSKTHREAD_PRIV__
	FskErr FskThreadCreateMain(FskThread *thread);
	FskErr FskThreadTerminateMain(void);
	FskThread FskThreadGetMain(void);
#endif

FskAPI(FskErr) FskThreadCreate(FskThread *thread, FskThreadProc procedure, UInt32 flags, void *refcon, const char *name);
FskAPI(FskErr) FskThreadJoin(FskThread thread);
FskAPI(FskThread) FskThreadGetCurrent(void);
FskAPI(FskErr) FskThreadPostEvent(FskThread thread, FskEvent event);
FskAPI(const char*) FskThreadName(FskThread thread);

FskAPI(void) FskThreadYield(void);
FskAPI(FskErr) FskThreadWake(FskThread thread);

typedef void (*FskThreadCallback)(void *, void *, void *, void *);


#if SUPPORT_INSTRUMENTATION
	#define FskThreadPostCallback(thread, function, arg1, arg2, arg3, arg4) FskThreadPostCallback_(thread, function, arg1, arg2, arg3, arg4, __FUNCTION__)
	FskAPI(FskErr) FskThreadPostCallback_(FskThread thread, FskThreadCallback function, void *arg1, void *arg2, void *arg3, void *arg4, const char *functionName);
#else
	#define FskThreadPostCallback(thread, function, arg1, arg2, arg3, arg4) FskThreadPostCallback_(thread, function, arg1, arg2, arg3, arg4)
	FskAPI(FskErr) FskThreadPostCallback_(FskThread thread, FskThreadCallback function, void *arg1, void *arg2, void *arg3, void *arg4);
#endif

#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL
	FskAPI(FskThreadDataSource) FskThreadCreateDataSource(UInt32 fd);

	FskAPI(void) FskThreadAddDataHandler(FskThreadDataHandler *handlerOut, FskThreadDataSource source, FskThreadDataReadyCallback callback, Boolean wantsReadable, Boolean wantsWritable, void *refCon);
#if USE_POLL
	FskAPI(void) FskThreadDataHandlerSetWantsException(FskThreadDataHandler handler, Boolean wantsException);
#endif
	FskAPI(UInt32) FskThreadGetDataHandlerDataNode(FskThreadDataHandler handler);
	FskAPI(void) FskThreadRemoveDataHandler(FskThreadDataHandler *handler);

	FskAPI(Boolean) FskThreadDefaultQuitHandler(FskEvent event, void *quit);
	FskAPI(FskThreadEventHandler) FskThreadAddEventHandler(UInt32 eventCode, FskThreadEventHandlerRoutine handler, void *refCon);
	FskAPI(FskErr) FskThreadRemoveEventHandler(FskThreadEventHandler handler);
	FskAPI(FskErr) FskThreadBroadcastEvent(FskEvent event);

	FskAPI(void)	FskThreadInitializationComplete(FskThread thread);
	FskAPI(FskErr) FskThreadRunloopCycle(SInt32 msec);

    FskAPI(char *) FskThreadFormatDiagnostic(char *msg, ...);

#endif

#if TARGET_OS_KPL
	void FskThreadNotifySocketData(int platSkt);	// @@ do we need to pass the platform socket, or can we assume the call is made in the current thread
	Boolean FskHandleThreadEvent(void *event);
#endif

#if TARGET_OS_MAC
	void MacThreadGotSocketData();
#elif TARGET_OS_ANDROID || TARGET_OS_LINUX
	Boolean LinuxHandleThreadEvents(void);
#endif

// condition

#ifndef __FSKTHREAD_PRIV__
	FskDeclarePrivateType(FskCondition)
#else
	#if TARGET_OS_MAC || TARGET_OS_LINUX
		typedef struct {
			pthread_cond_t		cond;

			FskInstrumentedItemDeclaration
		} FskConditionRecord, *FskCondition;
	#elif TARGET_OS_WIN32
		typedef struct {
			HANDLE				semaphore;
			DWORD				waitersCount;
			HANDLE				waitersEvent;
			WORD				waitersFlag;
			CRITICAL_SECTION	waitersMutex;

			FskInstrumentedItemDeclaration
		} FskConditionRecord, *FskCondition;
	#elif TARGET_OS_KPL
		typedef struct {
			void		*kplCond;

			FskInstrumentedItemDeclaration
		} FskConditionRecord, *FskCondition;
	#endif
#endif

FskAPI(FskErr) FskConditionNew(FskCondition *condition);
FskAPI(FskErr) FskConditionDispose(FskCondition condition);

FskAPI(FskErr) FskConditionSignal(FskCondition condition);
FskAPI(FskErr) FskConditionWait(FskCondition condition, FskMutex mutex);
FskAPI(FskErr) FskConditionTimedWait(FskCondition condition, FskMutex mutex, FskTime timeout);
FskAPI(FskErr) FskConditionBroadcast(FskCondition condition);

#define threadTag(t) (t ? (t)->name : "pre-main")

#ifdef __cplusplus
}
#endif

#if SUPPORT_INSTRUMENTATION
enum {
	    kFskThreadInstrMsgWake = kFskInstrumentedItemFirstCustomMessage,
	    kFskThreadInstrMsgWaitForInit,
	    kFskThreadInstrMsgDoneInit,
	    kFskThreadInstrMsgJoin,
	    kFskThreadInstrMsgJoined,
	    kFskThreadInstrMsgJoinNonJoinable,
	    kFskThreadInstrMsgAddEventHandler,
	    kFskThreadInstrMsgRemoveEventHandler,
	    kFskThreadInstrMsgBroadcastEvent,
	    kFskThreadInstrMsgPostEvent,
	    kFskThreadInstrMsgPostCallback,
	    kFskThreadInstrMsgStart,
	    kFskThreadInstrMsgFinished,
	    kFskThreadInstrMsgGotSocketData,
	    kFskThreadInstrMsgFinishedMain,

	    kFskConditionInstrMsgSignal = kFskInstrumentedItemFirstCustomMessage + 1024,
	    kFskConditionInstrMsgWait,
	    kFskConditionInstrMsgTimedWait,
	    kFskConditionInstrMsgTimedWaitSignaled,
	    kFskConditionInstrMsgTimedWaitTimedOut,
	    kFskConditionInstrMsgBroadcast,
};
#endif


#endif
