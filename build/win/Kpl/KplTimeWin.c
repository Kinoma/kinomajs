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
#include <windows.h>
#include <time.h>

#define kWinTimerID	1

#include "FskMemory.h"
#include "FskList.h"
#include "FskTime.h"
#include "FskErrors.h"

#include "KplTimeWinPriv.h"
#include "KplThreadWinPriv.h"

#if SUPPORT_TIMER_THREAD
	HANDLE gTimerThreadRescheduleEvent;
	HANDLE gTimerThreadExitEvent;

	static void timerThread(void *refcon);
	static void sInsertInTime(KplTimeCallback el);
	static void rescheduleTimer(KplThread thread);
#endif

FskErr KplTimeInitialize(void)
{
#if SUPPORT_TIMER_THREAD
	HANDLE thread;

	gTimerThreadRescheduleEvent = CreateEvent(NULL, false, false, NULL);
	gTimerThreadExitEvent = CreateEvent(NULL, false, false, NULL);

	thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)timerThread, NULL, 0, NULL);
	CloseHandle(thread);
#endif
	return kFskErrNone;
}

void KplTimeTerminate(void)
{
#if SUPPORT_TIMER_THREAD
	SetEvent(gTimerThreadExitEvent);
	while (INVALID_HANDLE_VALUE != gTimerThreadExitEvent)
		Sleep(10);
#endif
}

static char  kplwin32TzName[128];
extern char *strptime(const char *buf, const char *fmt, struct tm *tm);
#include <sys/types.h>
#include <sys/timeb.h>

// Use this function to avoid members order of struct tm be modified in different toolchains and platforms
static _inline void CopyToKplTimeElementsCommon(struct tm *src, KplTimeElements dst){
	dst->tm_sec		= src->tm_sec;  
	dst->tm_min		= src->tm_min;
	dst->tm_hour	= src->tm_hour;
	dst->tm_mday	= src->tm_mday;
	dst->tm_mon		= src->tm_mon;
	dst->tm_year	= src->tm_year;
	dst->tm_wday	= src->tm_wday;
	dst->tm_yday	= src->tm_yday;
	dst->tm_isdst	= src->tm_isdst;
}
static _inline void CopyFromKplTimeElementsCommon(KplTimeElements src, struct tm *dst){
	dst->tm_sec		= src->tm_sec;  
	dst->tm_min		= src->tm_min;
	dst->tm_hour	= src->tm_hour;
	dst->tm_mday	= src->tm_mday;
	dst->tm_mon		= src->tm_mon;
	dst->tm_year	= src->tm_year;
	dst->tm_wday	= src->tm_wday;
	dst->tm_yday	= src->tm_yday;
	dst->tm_isdst	= src->tm_isdst;
}

UInt32 KplTimeStrftime(char *s, UInt32 max, const char *format, const KplTimeElements kpltm)
{
	UInt32		len = 0;
	struct tm 	itm;

	CopyFromKplTimeElementsCommon(kpltm, &itm);
	len = (UInt32)strftime(s, max, format, (const struct tm *) &itm);

	return len;
}

char *KplTimeStrptime(const char *s, const char *format,  KplTimeElements kpltm)
{
	char 		*p = NULL;
	struct tm 	itm;

	p = strptime(s, format, &itm);
	CopyToKplTimeElementsCommon(&itm, kpltm);

	// Just for safe: strptime do not process %Z
	kpltm->tm_gmtoff= 0;
	kpltm->tm_zone	= NULL;

	return p;
}

FskErr KplTimeGmtime(const KplTime kpltime, KplTimeElements kpltm)
{
	FskErr		err = kFskErrNone;
	struct tm	itm;
	time_t		tsec;

	tsec = (time_t)kpltime->seconds;

	err = gmtime_s(&itm, &tsec);
	if(err)
		err = kFskErrUnknown;

	CopyToKplTimeElementsCommon(&itm, kpltm);
	kpltm->tm_gmtoff= 0;
	kpltm->tm_zone	= NULL;

	return err;
}

FskErr KplTimeLocaltime(const KplTime kpltime, KplTimeElements kpltm)
{
	FskErr		err = kFskErrNone;
	struct tm	itm;
	time_t		tsec;

	tsec = (time_t)kpltime->seconds;

	err = localtime_s(&itm, &tsec);
	if(err)
		err = kFskErrUnknown;

	// DO it even error
	CopyToKplTimeElementsCommon(&itm, kpltm);
	{
		int tzoff;
		int tzLen;
		_get_timezone(&tzoff);
		kpltm->tm_gmtoff= -tzoff;
		_get_tzname(&tzLen, kplwin32TzName, 128, 0);
		kpltm->tm_zone	= kplwin32TzName;
	}

	return err;
}

void KplTimeMktime(const KplTimeElements kpltm, KplTime kpltime)
{
	struct tm		itm;
	time_t			tsec;

	CopyFromKplTimeElementsCommon(kpltm, &itm);
	tsec = mktime(&itm);

	kpltime->seconds = (SInt32)tsec;
	kpltime->useconds= 0;
}

FskErr KplTimeStime(SInt32 secs)
{
	return kFskErrUnimplemented;
}

FskErr KplTimeTzset(const char *tzName)
{
	return kFskErrUnimplemented;
}

FskErr KplTimeGetZone(KplTime kpltime, SInt32 *tzoff, SInt32 *dst, const char **tzName)
{
	FskErr	err = kFskErrNone;
	int		tzLen;
	int		idst = 0;

	if(NULL == kpltime) {
		// Get daylight saving status of current time
		err = _get_daylight(&idst);
	}
	else {
		// Get daylight saving status of fsktime
		struct tm		itm;
		time_t			tsec;

		tsec = kpltime->seconds;
		err = localtime_s(&itm, &tsec);
		idst = itm.tm_isdst;
	}
	if(tzoff) {
		err = _get_timezone((int *)tzoff);
		*tzoff = -*tzoff;
		if(idst)
			*tzoff += 3600; // adjust accroding to dst
	}

	if(dst)
		*dst = idst;

	if(tzName){
		err += _get_tzname(&tzLen, kplwin32TzName, 128, 0);
		*tzName = kplwin32TzName;
	}
	if(err)
		err = kFskErrUnknown;
	return err;
}

void KplTimeGetOSTime(KplTime t)
{
	struct _timeb tb;
	_ftime_s(&tb);
	t->seconds = (SInt32)(tb.time);
	t->useconds= tb.millitm * 1000;
}

void KplTimeGetNow(KplTime t)
{
	DWORD now;

	now = timeGetTime();
	t->seconds = now / 1000;
	t->useconds = (now % 1000) * 1000;
}

void KplTimeMakeDate(char *dateString, int dateStringSize)
{
	time_t tsec;
	time(&tsec);
	strftime(dateString, dateStringSize, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&tsec));
}

void KplTimeCallbackNew(KplTimeCallback *callback)
{
	KplTimeCallback kplTimeCallback;
	
	if (kFskErrNone != FskMemPtrNewClear(sizeof(KplTimeCallbackRecord), (FskMemPtr*)&kplTimeCallback)) {
		*callback = NULL;
		return;
	}

	*callback = kplTimeCallback;
	kplTimeCallback->owner = KplThreadGetCurrent();
}

void KplTimeCallbackDispose(KplTimeCallback callback)
{
	if (NULL == callback)
		return;

	KplTimeCallbackCancel(callback);
	FskMemPtrDispose(callback);
}

void KplTimeCallbackSet(KplTimeCallback callback, const KplTime when, KplTimeCallbackProc callbackProc, void *param)
{
	if (!callback) return;
	callback->param = param;
	callback->callbackProc = callbackProc;
	callback->trigger = *when;
	callback->marked = false;
		
	sInsertInTime(callback);
}

void KplTimeCallbackCancel(KplTimeCallback callback)
{
	Boolean reschedule;
	KplThread thread;
	
	if (NULL == callback)
		return;

	thread = callback->owner;
 	if ((NULL == thread) || ((NULL == thread->timerCallbacks) && (NULL == thread->suspendedTimerCallbacks)))
		return;

	reschedule = (callback == thread->timerCallbacks);

	if (!FskListRemove((FskList *)&thread->timerCallbacks, callback))
		FskListRemove((FskList *)&thread->suspendedTimerCallbacks, callback);

	if (reschedule)
		rescheduleTimer(thread);
}

Boolean KplTimeCallbackGetNextTime(const KplTime now, KplTime whenNext, void *threadIn)
{
	KplThread thread = threadIn ? threadIn : KplThreadGetCurrent();

	if (thread->timerCallbacks) {
		KplTimeCallback cb = (KplTimeCallback)thread->timerCallbacks;
		if (FskTimeCompare((FskTime)now, (FskTime)&cb->trigger) < 1) {
			FskTimeClear((FskTime)whenNext);
			return false;
		}

		*whenNext = cb->trigger;
	}
	else {
		*whenNext = *now;
		FskTimeAddSecs((FskTime)whenNext, 3600 * 12);	// a long time from now
	}

	return true;
}

void KplTimeCallbackService(void *param)
{
	KplTimeRecord	now;
	KplTimeCallback cb;
	KplThread thread = (KplThread)param;

#if SUPPORT_WAITABLE_TIMER
	if (thread->waitableTimer) {
		LARGE_INTEGER timeDue;

		timeDue.QuadPart = ~0;
		SetWaitableTimer(thread->waitableTimer, &timeDue, 0, NULL, NULL, 0);
	}
#elif SUPPORT_TIMER_THREAD
	thread->timerSignaled = false;
#else
	KillTimer(thread->window, kWinTimerID);
	thread->timer = 0;
#endif

	cb = (KplTimeCallback)thread->timerCallbacks;
	if (NULL == cb)
		return;

	// mark all those that are due
	KplTimeGetNow(&now);
#if TIME_QUANTA
	FskTimeAddMS(&now, TIME_QUANTA);
#endif /* TIME_QUANTA */
	while (cb) {
		if (FskTimeCompare((FskTime)&now, (FskTime)&cb->trigger) > 0)
			break;
		cb->marked = true;
		cb = cb->next;
	}

	// remove any that are due
	while (1) {
		cb = (KplTimeCallback)FskListGetNext(thread->timerCallbacks, NULL);
		if (!cb)
			break;
		if (!cb->marked)
			break;
		FskListRemove(&thread->timerCallbacks, cb);

		// Call the callback!
		(*cb->callbackProc)(cb, &cb->trigger, cb->param);
	}

	if (thread->timerCallbacks)
		rescheduleTimer(thread);
}

void sInsertInTime(KplTimeCallback el)
{
	KplTimeCallback cur, last = NULL;
	Boolean reschedule = false;
	KplTimeRecord now;
	KplThread thread;

	thread = (KplThread)el->owner;

	KplTimeGetNow(&now);
	if (1 == FskTimeCompare((FskTime)&el->trigger, (FskTime)&now))
		el->trigger = now;
	cur = (KplTimeCallback)FskListGetNext(thread->timerCallbacks, NULL);
	el->next = NULL;
	el->marked = false;

	if (cur == NULL) {
		FskListPrepend(&thread->timerCallbacks, el);
		reschedule = true;
		goto done;
	}
	while (cur) {
		if (FskTimeCompare((FskTime)&el->trigger, (FskTime)&cur->trigger) > 0) {
			if (last == NULL) {
				reschedule = true;
			}
			FskListInsertAfter(&thread->timerCallbacks, el, last);
			goto done;
		}

		last = cur;
		cur = cur->next;
	}
	if (!cur && last) {
		FskListAppend(&thread->timerCallbacks, el);
	}
	
done:
	if (reschedule)
		rescheduleTimer(thread);
}

void rescheduleTimer(KplThread thread)
{
	KplTimeRecord nowTime, nextTime;

	if (NULL == thread->timerCallbacks) {
#if SUPPORT_WAITABLE_TIMER
		if (thread->waitableTimer) {
			LARGE_INTEGER timeDue;

			timeDue.QuadPart = ~0;
			SetWaitableTimer(thread->waitableTimer, &timeDue, 0, NULL, NULL, 0);
		}
#elif SUPPORT_TIMER_THREAD
		thread->timerSignaled = false;
#else /* !SUPPORT_TIMER_THREAD */
		if (thread->timer) {
			KillTimer(thread->window, kWinTimerID);
			thread->timer = 0;
		}
#endif /* !SUPPORT_TIMER_THREAD */
		return;
	}

#if SUPPORT_WAITABLE_TIMER
	if (!thread->waitableTimer)
		thread->waitableTimer = CreateWaitableTimer(NULL, true, NULL);

	{
	LARGE_INTEGER timeDue;

	FskTimeGetNow(&nowTime);
	if (!FskTimeCallbackGetNextTime(&nowTime, &nextTime, thread))
		timeDue.QuadPart = -1;		// now
	else {
		FskTimeSub(&nowTime, &nextTime);
		timeDue.QuadPart = nextTime.seconds * -10000000 + nextTime.useconds * -10;
	}
	SetWaitableTimer(thread->waitableTimer, &timeDue, 0, NULL, NULL, 0);
	}
#elif SUPPORT_TIMER_THREAD
	KplTimeGetNow(&nowTime);

	if (!KplTimeCallbackGetNextTime(&nowTime, &nextTime, thread))
		thread->nextCallbackTime = nowTime;
	else
		thread->nextCallbackTime = nextTime;

	thread->timerSignaled = false;

	SetEvent(gTimerThreadRescheduleEvent);
#else /* !SUPPORT_TIMER_THREAD */
	FskTimeGetNow(&nowTime);
	if (!FskTimeCallbackGetNextTime(&nowTime, &nextTime, thread))
		thread->timer = SetTimer(thread->window, kWinTimerID, 0, NULL);	
	else {
		FskTimeSub(&nowTime, &nextTime);
		thread->timer = SetTimer(thread->window, kWinTimerID, FskTimeInMS(&nextTime), NULL);	
	}
#endif /* !SUPPORT_TIMER_THREAD */
}

#if SUPPORT_TIMER_THREAD

void timerThread(void *refcon)
{
	#define kHandleCount (3)
	HANDLE h[kHandleCount];

	h[0] = gTimerThreadRescheduleEvent;
	h[1] = gTimerThreadExitEvent;

	while (!gKplThreads)
		Sleep(10);

	while (gKplThreads) {
		DWORD result;
		SInt32 ms = 0x7fffffff;
		KplThread target = NULL, walker;
		FskTimeRecord now;

		FskMutexAcquire(gKplThreads->mutex);
 
		FskTimeGetNow(&now);
		for (walker = gKplThreads->list; NULL != walker; walker = walker->next) {
			FskTimeRecord delta;
			SInt32 deltaMS;

			if (!walker->timerCallbacks || walker->timerSignaled)
				continue;

			delta = *(FskTime)&walker->nextCallbackTime;
			FskTimeSub(&now, &delta);
			deltaMS = FskTimeInMS(&delta);
			if (deltaMS <= 0) {
				walker->timerSignaled = true;
				SetEvent(walker->waitableTimer);
			}
			else
			if (deltaMS < ms) {
				ms = deltaMS;
				target = walker;
			}
		}
		FskMutexRelease(gKplThreads->mutex);

		h[kHandleCount - 1] = target ? target->handle : NULL;
		result = WaitForMultipleObjects((target && target->handle) ? kHandleCount : kHandleCount - 1, h, false, ms);
		if (WAIT_OBJECT_0 == result)
			;		// recaclulated next time around
		else
		if ((WAIT_OBJECT_0 + 1) == result)
			break;
		else
		if ((WAIT_OBJECT_0 + (kHandleCount - 1)) == result) {
			target = NULL;			// target thread disposed?
			continue;
		}
		else
		if (target) {
			target->timerSignaled = true;
			SetEvent(target->waitableTimer);
		}
	}

	CloseHandle(gTimerThreadRescheduleEvent);
	CloseHandle(gTimerThreadExitEvent);
	gTimerThreadExitEvent = gTimerThreadRescheduleEvent = INVALID_HANDLE_VALUE;
}
#endif
