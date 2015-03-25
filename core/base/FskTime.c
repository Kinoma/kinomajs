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
#if TARGET_OS_LINUX
	#if TARGET_OS_ANDROID
		#define TIME_QUANTA		0		// service all pending callbacks from now to now + TIME_QUANTA
	#else
		#define TIME_QUANTA		2		// service all pending callbacks from now to now + TIME_QUANTA
	#endif
	#ifndef USE_POSIX_CLOCK
		#define USE_POSIX_CLOCK 0
	#endif
#endif

#define _WIN32_WINNT 0x0400
#define __FSKTIME_PRIV__
#define __FSKTHREAD_PRIV__
#include "FskTime.h"

#include "FskUtilities.h"
#include "FskThread.h"
#include "FskHardware.h"

#if TARGET_OS_WIN32
	#include <time.h>
	#include "FskPlatformImplementation.h"

	#define kWinTimerID	1
#elif TARGET_OS_MAC
    #include <mach/mach_time.h>
	#if TARGET_OS_IPHONE
		#include "FskCocoaSupportPhone.h"
	#else /* !TARGET_OS_IPHONE */
		#include "FskCocoaSupport.h"
	#endif /* !TARGET_OS_IPHONE */
	#include <time.h>
	#include "FskWindow.h"

    static uint64_t baseTicks;
    static mach_timebase_info_data_t timeBaseInfo;
#elif TARGET_OS_KPL
	#include <time.h>
	#include "KplTime.h"
#else /* !TARGET_OS_MAC */
	#include <sys/time.h>
#endif /* !TARGET_OS_MAC */
    
#if USE_POSIX_CLOCK
FskTimeRecord sBasetime;
#endif /* USE_POSIX_CLOCK */

#define MSECPERSEC	1000
#define USECPERMSEC	1000
#define USECPERSEC	((MSECPERSEC) * (USECPERMSEC))

#if TARGET_OS_WIN32 && SUPPORT_TIMER_THREAD
	HANDLE gTimerThreadRescheduleEvent;
	HANDLE gTimerThreadExitEvent;

	static void timerThread(void *refcon);
	static void syncUiTimers(void);
#endif /* TARGET_OS_WIN32 && SUPPORT_TIMER_THREAD */

#if !TARGET_OS_KPL
static Boolean gUiTimersActive = true;
#endif

// ---------------------------------------------------------------------
SInt32 FskTimeInSecs(FskTime time)
{
	if (NULL != time)
		return time->seconds;
	return 0;
}

// ---------------------------------------------------------------------
SInt32 FskTimeInMS(FskTime time)
{
	SInt32 result, fraction;

	if (NULL == time)
		return 0;

	if (time->seconds < 0)
		return 0;

	if (time->seconds > (0x7fffffff / MSECPERSEC))
		return 0x7fffffff;		 // return -1 if seconds will overflow MS

	result = time->seconds * MSECPERSEC;
	fraction = (time->useconds + (USECPERMSEC / 2)) / USECPERMSEC;
	if ((result + fraction) < result)
		return 0x7fffffff;			// overflow
	return result + fraction;
}

// ---------------------------------------------------------------------
void FskTimeGetNow(FskTime t)
{
#if TARGET_OS_WIN32
	DWORD now;

	now = timeGetTime();
	t->seconds = now / 1000;
	t->useconds = (now % 1000) * 1000;
#elif TARGET_OS_MAC
    uint64_t current;

    current = (mach_absolute_time() /* - baseTicks */) * timeBaseInfo.numer / timeBaseInfo.denom;
    t->seconds = current / 1000000000;
    current = current % 1000000000;
    t->useconds = current / 1000;
#elif TARGET_OS_KPL
	KplTimeGetNow((KplTime)t);
#else /* !TARGET_OS_MAC */
	#if USE_POSIX_CLOCK
		struct timespec tp;
		clock_gettime(CLOCK_MONOTONIC, &tp);
		t->seconds = tp.tv_sec;
		t->useconds = tp.tv_nsec / kFskTimeNsecPerUsec;
		FskTimeSub(&sBasetime, t);
	#else /* !USE_POSIX_CLOCK */
		struct timeval tv;
		struct timezone tz;

			// - should use MONOTONIC clock if possible
			// - gettimeofday can vary based on NTP adjustments
			//  or clock changes
		gettimeofday(&tv, &tz);
		t->seconds = tv.tv_sec;
		t->useconds = tv.tv_usec;
	#endif /* !USE_POSIX_CLOCK */
#endif /* !TARGET_OS_MAC */
}

// ---------------------------------------------------------------------
void FskTimeMakeDate(char *dateString, int stringSize) {
#if TARGET_OS_KPL
	KplTimeMakeDate(dateString, stringSize);
#else
	time_t tsec;
	time(&tsec);
	strftime(dateString, stringSize, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&tsec));
#endif
}

#if TARGET_OS_WIN32
static char  win32TzName[128];
char *strptime(const char *buf, const char *fmt, struct tm *tm);
#include <sys/types.h>
#include <sys/timeb.h>
#endif

#ifndef INLINE
#if TARGET_OS_WIN32 || (TARGET_OS_KPL && defined(_MSC_VER))
#define INLINE _inline
#else
#define INLINE inline
#endif
#endif
// Use this function to avoid members order of struct tm be modified in different toolchains and platforms
static INLINE void CopyToFskTimeElementsCommon(struct tm *src, FskTimeElements dst){
	dst->tm_sec		= (SInt32)src->tm_sec;  
	dst->tm_min		= (SInt32)src->tm_min;
	dst->tm_hour	= (SInt32)src->tm_hour;
	dst->tm_mday	= (SInt32)src->tm_mday;
	dst->tm_mon		= (SInt32)src->tm_mon;
	dst->tm_year	= (SInt32)src->tm_year;
	dst->tm_wday	= (SInt32)src->tm_wday;
	dst->tm_yday	= (SInt32)src->tm_yday;
	dst->tm_isdst	= (SInt32)src->tm_isdst;
}
static INLINE void CopyFromFskTimeElementsCommon(FskTimeElements src, struct tm *dst){
	dst->tm_sec		= (int)src->tm_sec;  
	dst->tm_min		= (int)src->tm_min;
	dst->tm_hour	= (int)src->tm_hour;
	dst->tm_mday	= (int)src->tm_mday;
	dst->tm_mon		= (int)src->tm_mon;
	dst->tm_year	= (int)src->tm_year;
	dst->tm_wday	= (int)src->tm_wday;
	dst->tm_yday	= (int)src->tm_yday;
	dst->tm_isdst	= (int)src->tm_isdst;
}
void FskTimeStrftime(char *s, UInt32 max, const char *format, const FskTimeElements fsktm, UInt32 *wLen){
	UInt32			len = 0;
#if TARGET_OS_KPL
	len = KplTimeStrftime(s, max, format, (KplTimeElements)fsktm);
#else
	struct tm 		itm;

	CopyFromFskTimeElementsCommon(fsktm, &itm);

	#if !TARGET_OS_WIN32
	{
		itm.tm_gmtoff= 0;
		itm.tm_zone	= NULL;
	}
	#endif
	len = (UInt32)strftime(s, max, format, (const struct tm *) &itm);
#endif
	if(wLen)
		*wLen = len;
	return;
}

void FskTimeStrptime(const char *s, const char *format,  FskTimeElements fsktm, char **endptr){
	char *p;
#if TARGET_OS_KPL
	p = KplTimeStrptime(s, format, (KplTimeElements)fsktm);
#else
	struct tm 	itm;
	// In principle, this function does not initialize tm but stores only the values specified.
	FskMemSet(&itm, 0, sizeof(struct tm));
	p = strptime(s, format, &itm);
	CopyToFskTimeElementsCommon(&itm, fsktm);

	// Just for safe: strptime do not process %Z
	#if !TARGET_OS_WIN32
	fsktm->tm_gmtoff= itm.tm_gmtoff;
	fsktm->tm_zone	= itm.tm_zone;
	#else
	fsktm->tm_gmtoff= 0;
	fsktm->tm_zone  = NULL; 
	#endif
#endif
	if(endptr)
		*endptr = p;
	return;
}

// Covert from FskTime to FskTimeElements (GMT) --- convert from calendar time to broken-down time
FskErr FskTimeGmtime(const FskTime fsktime, FskTimeElements fsktm) {
	FskErr	err = kFskErrNone;
#if TARGET_OS_KPL
	err = KplTimeGmtime((KplTime)fsktime, (KplTimeElements)fsktm);
#else
	struct tm		itm;
	time_t			tsec;

	tsec = (time_t)fsktime->seconds;

	#if TARGET_OS_WIN32
		err = gmtime_s(&itm, &tsec);
		if(err)
			err = kFskErrUnknown;
	#else
	{
		void *p = gmtime_r(&tsec, &itm);
		if(NULL == p)
			err = kFskErrUnknown;
	}
	#endif
	CopyToFskTimeElementsCommon(&itm, fsktm);
	fsktm->tm_gmtoff= 0;
	fsktm->tm_zone	= NULL;
#endif
	return err;
}

// Covert from FskTime to FskTimeElements (local) --- convert from calendar time to broken-down time
FskErr FskTimeLocaltime(const FskTime fsktime, FskTimeElements fsktm){
	FskErr	err = kFskErrNone;
#if TARGET_OS_KPL
	err = KplTimeLocaltime((KplTime)fsktime, (KplTimeElements)fsktm);
#else
	struct tm		itm;
	time_t			tsec;

	tsec = (time_t)fsktime->seconds;

	#if TARGET_OS_WIN32
		err = localtime_s(&itm, &tsec);
		if(err)
			err = kFskErrUnknown;
	#else
		void *p = localtime_r(&tsec, &itm);
		if(NULL == p)
			err = kFskErrUnknown;
	#endif
	// DO it even error
	CopyToFskTimeElementsCommon(&itm, fsktm);
	#if !TARGET_OS_WIN32
		fsktm->tm_gmtoff= itm.tm_gmtoff;
		fsktm->tm_zone	= itm.tm_zone;
	#else
	{
		int tzoff;
		int tzLen;
		_get_timezone(&tzoff);
		fsktm->tm_gmtoff= -tzoff;
		_get_tzname(&tzLen, win32TzName, 128, 0);
		fsktm->tm_zone	= win32TzName;
	}
	#endif
#endif	
	return err;
}

// Covert from FskTimeElements (local, broken-down time) to FskTime (calendar time)
void FskTimeMktime(const FskTimeElements fsktm, FskTime fsktime){
#if TARGET_OS_KPL
	KplTimeMktime((KplTimeElements)fsktm, (KplTime)fsktime);
#else
	struct tm		itm;
	time_t			tsec;

	CopyFromFskTimeElementsCommon(fsktm, &itm);
	#if !TARGET_OS_WIN32
	itm.tm_gmtoff = fsktm->tm_gmtoff;	//We assume user give all correct information
	#endif
	tsec = mktime(&itm);

	fsktime->seconds = (SInt32)tsec;
	fsktime->useconds= 0;
#endif	
	return;
}

FskErr FskTimeGetZone(FskTime fsktime, SInt32 *tzoff, SInt32 *dst, const char **tzName){
	FskErr	err = kFskErrNone;
#if TARGET_OS_KPL
	err = KplTimeGetZone((KplTime)fsktime, tzoff, dst, tzName);
#else
	#if TARGET_OS_WIN32
		int		tzLen;
		int		idst = 0;

		if(NULL == fsktime) {
			// Get daylight saving status of current time
			err = _get_daylight(&idst);
		}
		else {
			// Get daylight saving status of fsktime
			struct tm		itm;
			time_t			tsec;

			tsec = fsktime->seconds;
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
			err += _get_tzname(&tzLen, win32TzName, 128, 0);
			*tzName = win32TzName;
		}
		if(err)
			err = kFskErrUnknown;
	#else
		struct tm		itm;
		time_t			tsec;

		if(NULL == fsktime) {
			struct timeval tv;	
			gettimeofday(&tv, NULL);
			tsec = tv.tv_sec;
		}
		else {
			tsec = fsktime->seconds;
		}
		localtime_r(&tsec, &itm);
		
		if(tzoff)
			*tzoff = (SInt32)itm.tm_gmtoff;;
		if(dst)
			*dst = itm.tm_isdst;
		if(tzName)
			*tzName = itm.tm_zone;
	#endif
#endif	
	return err;	
}

void FskTimeGetOSTime(FskTime time)
{
#if TARGET_OS_KPL
	KplTimeGetOSTime((KplTime)time);
#elif TARGET_OS_WIN32 
	{
    struct _timeb tb;
	_ftime_s(&tb);
	time->seconds = (SInt32)(tb.time);
	time->useconds= tb.millitm * 1000;
	}
#else
	#if USE_POSIX_CLOCK
		struct timespec tp;
		clock_gettime(CLOCK_REALTIME, &tp);
		time->seconds = tp.tv_sec;
		time->useconds = tp.tv_nsec / kFskTimeNsecPerUsec;
	#else /* !USE_POSIX_CLOCK */
		struct timeval tv;
		struct timezone tz;
		gettimeofday(&tv, &tz);
		time->seconds = tv.tv_sec;
		time->useconds = tv.tv_usec;
	#endif /* !USE_POSIX_CLOCK */
#endif /* TARGET_OS_KPL */
}

FskErr FskTimeStime(const FskTime time)
{
#if TARGET_OS_KPL
	return KplTimeStime(time->seconds);
#else
	return kFskErrUnimplemented;
#endif
}

FskErr FskTimeTzset(const char *tzName)
{
#if TARGET_OS_KPL
	return KplTimeTzset(tzName);
#else
	return kFskErrUnimplemented;
#endif
}

// ---------------------------------------------------------------------
void FskTimeClear(FskTime t)
{
	if (NULL != t) {
		t->seconds = 0;
		t->useconds = 0;
	}
}

// ---------------------------------------------------------------------
void FskTimeCopy(FskTime t1, FskTime t2)
{
	if ((NULL == t1) || (NULL == t2))
		return;

	t1->seconds = t2->seconds;
	t1->useconds = t2->useconds;
}

// ---------------------------------------------------------------------
void FskTimeAdd(FskTime time1, FskTime time2)
{
	if ((NULL == time1) || (NULL == time2))
		return;

	time2->seconds += time1->seconds;
	time2->useconds += time1->useconds;
	while (time2->useconds >= USECPERSEC) {
		time2->seconds++;
		time2->useconds -= USECPERSEC;
	}
}

// ---------------------------------------------------------------------
void FskTimeAddMS(FskTime time, UInt32 ms)
{
	long s;

	if (NULL == time)
		return;

	s = ms / MSECPERSEC;
	time->seconds += s;
	ms -= s * MSECPERSEC;
	ms *= USECPERMSEC;
	time->useconds += ms;
	if (time->useconds >= USECPERSEC) {
		time->seconds++;
		time->useconds -= USECPERSEC;
	}
}

// ---------------------------------------------------------------------
void FskTimeAddSecs(FskTime time, UInt32 secs)
{
	if (NULL == time)
		return;

	time->seconds += secs;
}

// ---------------------------------------------------------------------
void FskTimeSub(FskTime time1, FskTime time2)
{
	if ((NULL == time1) || (NULL == time2))
		return;

	time2->seconds -= time1->seconds;
	time2->useconds -= time1->useconds;
	while (time2->useconds < 0) {
		time2->seconds -= 1;
		time2->useconds += USECPERSEC;
	}
}

// ---------------------------------------------------------------------
void FskTimeSubMS(FskTime time, UInt32 ms)
{
	long s;

	if (NULL == time)
		return;

	s = ms / MSECPERSEC;
	time->seconds -= s;
	ms -= s * MSECPERSEC;
	ms *= USECPERMSEC;
	time->useconds -= ms;
	if (time->useconds < 0) {
		time->seconds -= 1;
		time->useconds += USECPERSEC;
	}
}

// ---------------------------------------------------------------------
void FskTimeSubSecs(FskTime time, UInt32 secs)
{
	if (NULL == time)
		return;

	time->seconds -= secs;
}

// ---------------------------------------------------------------------
// returns -1 if time1 is later than time2
// returns 1 if time2 is later than time1
// returns 0 if time1 == time2
// undefined if either is NULL
SInt32 FskTimeCompare(const FskTime time1, const FskTime time2)
{
	if ((NULL == time1) || (NULL == time2))
		return 0;

	if (time1->seconds > time2->seconds)
		return -1;
	if (time1->seconds < time2->seconds)
		return 1;
	if (time1->useconds > time2->useconds)
		return -1;
	if (time1->useconds < time2->useconds)
		return 1;
	return 0;
}

// ---------------------------------------------------------------------

#if TARGET_OS_WIN32 || TARGET_OS_MAC
	static void rescheduleTimer(FskThread thread);
#else /* !TARGET_OS_WIN32 && !TARGET_OS_MAC */
	#define rescheduleTimer(a)
#endif /* !TARGET_OS_WIN32 && !TARGET_OS_MAC */

#if !TARGET_OS_KPL
static void sInsertInTime(FskTimeCallBack el)
{
	FskTimeCallBack cur, last = NULL;
	Boolean reschedule = false;
	FskTimeRecord now;
	FskThread thread;

	thread = (FskThread)el->owner;

	FskTimeGetNow(&now);
	if (1 == FskTimeCompare(&el->trigger, &now))
		el->trigger = now;
	cur = (FskTimeCallBack)FskListGetNext(thread->timerCallbacks, NULL);
	el->next = NULL;
	el->marked = false;

	if (cur == NULL) {
		FskInstrumentedItemSendMessage(el, kFskTimeInstrMsgScheduleFirst, el);
		FskListPrepend(&thread->timerCallbacks, el);
		reschedule = true;
		goto done;
	}
	while (cur) {
		if (FskTimeCompare(&el->trigger, &cur->trigger) > 0) {
			if (last == NULL) {
				FskInstrumentedItemSendMessage(el, kFskTimeInstrMsgScheduleFirst, el);
				reschedule = true;
			}
			else {
				FskInstrumentedItemSendMessage(el, kFskTimeInstrMsgScheduleMid, el);
			}
			FskListInsertAfter(&thread->timerCallbacks, el, last);
			goto done;
		}

		last = cur;
		cur = cur->next;
	}
	if (!cur && last) {
		FskInstrumentedItemSendMessage(el, kFskTimeInstrMsgScheduleEnd, el);
		FskListAppend(&thread->timerCallbacks, el);
	}

done:
	if (reschedule)
		rescheduleTimer(thread);
}
#endif

// ---------------------------------------------------------------------
void FskTimeCallbackRemove(FskTimeCallBack ref)
{
#if !TARGET_OS_KPL
	Boolean reschedule;
	FskThread thread;

	if (NULL == ref)
		return;

	thread = ref->owner;
 	if ((NULL == thread) || ((NULL == thread->timerCallbacks) && (NULL == thread->suspendedTimerCallbacks)))
		return;

	reschedule = (ref == thread->timerCallbacks);

	if (!FskListRemove((FskList *)&thread->timerCallbacks, ref))
		FskListRemove((FskList *)&thread->suspendedTimerCallbacks, ref);

	if (reschedule)
		rescheduleTimer(thread);
#else
	if (NULL == ref || NULL == ref->kplTimeCallback)
		return;
	KplTimeCallbackCancel(ref->kplTimeCallback);
#endif
}

#if SUPPORT_INSTRUMENTATION
	static Boolean doFormatMessageTimer(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize) {
		FskThread thread;
		FskTimeCallBack timer;
		FskTimeRecord now, later, next;
		char tmp[64];

		if (msg < kFskInstrumentedItemFirstCustomMessage)
			return false;			// we don't format system mesages, and the timer->owner below can crash if we try

		timer = (FskTimeCallBack)msgData;
		thread = (FskThread)timer->owner;
		switch (msg) {
			case kFskTimeInstrMsgScheduleFirst:
			case kFskTimeInstrMsgScheduleMid:
			case kFskTimeInstrMsgScheduleEnd:
				FskTimeGetNow(&now);
				later = now;
				if (kFskTimeInstrMsgScheduleFirst == msg) {
					later = timer->trigger;
					FskTimeSub(&now, &later);
					if (FskTimeInMS(&later) <= 0)
						snprintf(buffer, bufferSize, "[%s, %s/%ld] Schedule timer to fire now %lds:%ldms", thread->name, timer->schedulingFunctionName, timer->schedulingFunctionLine, now.seconds, now.useconds/1000);
					else
						snprintf(buffer, bufferSize, "[%s, %s/%ld] Schedule timer at head, fire in %lds:%ldms", thread->name, timer->schedulingFunctionName, timer->schedulingFunctionLine, later.seconds, later.useconds/1000);
				}
				else {
					FskTimeSub(&((FskTimeCallBack)thread->timerCallbacks)->trigger, &now);
					FskTimeSub(&timer->trigger, &later);
					snprintf(buffer, bufferSize, "[%s, %s/%ld] Schedule timer %s, first in %lds:%ldms - this one in %lds:%ldms",
						thread->name,
						timer->schedulingFunctionName, timer->schedulingFunctionLine,
						msg == kFskTimeInstrMsgScheduleEnd ? "at end" : "in middle",
						-now.seconds, now.useconds/1000,
						-later.seconds, later.useconds/1000);
				}
				return true;
			case kFskTimeInstrMsgTimerFire: {
				SInt32 laterMS;
				if (timer->next) {
					FskTimeGetNow(&next);
					FskTimeSub(&timer->next->trigger, &next);
					snprintf(tmp, 63, "next in %ld:s%ldms", -next.seconds, next.useconds/1000);
				}
				else
					tmp[0] = '\0';
				later = timer->fired;
				FskTimeSub(&timer->trigger, &later);
				laterMS = FskTimeInMS(&later);
				if (laterMS > 0)
					snprintf(buffer, bufferSize, "[%s, %s/%ld] timer fired %lds:%ldms late - %s", thread->name, timer->schedulingFunctionName, timer->schedulingFunctionLine, later.seconds, later.useconds/1000, tmp);
				else if (0 == laterMS)
					snprintf(buffer, bufferSize, "[%s, %s/%ld] timer fired on time - %s", thread->name, timer->schedulingFunctionName, timer->schedulingFunctionLine, tmp);
				else
					snprintf(buffer, bufferSize, "[%s, %s/%ld] timer fired %lds:%ldms early - %s", thread->name, timer->schedulingFunctionName, timer->schedulingFunctionLine, -later.seconds, later.useconds/1000, tmp);
				}
				return true;
			case kFskTimeInstrMsgDeleteFromWrongThread:
				snprintf(buffer, bufferSize, "thread %s trying to delete a timer that is owned by [%s]", FskThreadGetCurrent()->name, thread->name);
				return true;
			default:
				break;
		}

		return false;
	}

	#include <stddef.h>

	static FskInstrumentedValueRecord gInstrumentationTimeCallbackValues[] = {
		{ "schedulingFunction",	offsetof(FskTimeCallBackRecord, schedulingFunctionName),			kFskInstrumentationKindString},
		{ "schedulingLine",		offsetof(FskTimeCallBackRecord, schedulingFunctionLine),			kFskInstrumentationKindInteger},
		{ NULL,					0,																	kFskInstrumentationKindUndefined}
	};

	static FskInstrumentedTypeRecord gTimeCallbackTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"timecallback",
		FskInstrumentationOffset(FskTimeCallBackRecord),
		NULL,
		0,
		NULL,
		doFormatMessageTimer,
		gInstrumentationTimeCallbackValues
	};
#endif /* SUPPORT_INSTRUMENTATION */

// ---------------------------------------------------------------------
#if !SUPPORT_INSTRUMENTATION
	FskTimeCallBack FskTimeCallbackAddNew(const FskTime when, FskTimeCallback callback, void *param)
#else /* SUPPORT_INSTRUMENTATION */
	FskTimeCallBack FskTimeCallbackAddNew_(const FskTime when, FskTimeCallback callback, void *param, FSK_TIME_DEBUG_ARGS)
#endif /* SUPPORT_INSTRUMENTATION */
{
	FskTimeCallBack ref;

	FskTimeCallbackNew(&ref);
#if !SUPPORT_INSTRUMENTATION
	FskTimeCallbackSet(ref, when, callback, param);
#else /* SUPPORT_INSTRUMENTATION */
	FskTimeCallbackSet_(ref, when, callback, param, FSK_TIME_DEBUG_PARAMS_CONTINUE);
#endif /* SUPPORT_INSTRUMENTATION */
	return ref;
}

// ---------------------------------------------------------------------
void FskTimeCallbackNew(FskTimeCallBack *callbackRef)
{
	FskTimeCallBack el = NULL;

	if (kFskErrNone != FskMemPtrNewClear(sizeof(FskTimeCallBackRecord), &el)) {
		*callbackRef = NULL;
		return;
	}

#if TARGET_OS_KPL
	KplTimeCallbackNew((KplTimeCallback*)&el->kplTimeCallback);
	if (NULL == el->kplTimeCallback){
		*callbackRef = NULL;
		return;
	}
#endif

	*callbackRef = el;
	el->owner = FskThreadGetCurrent();

	FskInstrumentedItemNew(el, NULL, &gTimeCallbackTypeInstrumentation);
	FskInstrumentedItemSetOwner(el, (FskThread)(el->owner));
}

// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
FskAPI(void) FskTimeCallbackUINew(FskTimeCallBack *callbackRef)
{
	FskTimeCallbackNew(callbackRef);
	if (*callbackRef)
		(*callbackRef)->ui = true;
}

#if TARGET_OS_KPL
void kplCallback(FskTimeCallBack callback, const FskTime time, void *param)
{
	FskTimeCallBack el = (FskTimeCallBack)param;
	(*el->callback)(el, time, el->param);
}
#endif

#if !SUPPORT_INSTRUMENTATION
	void FskTimeCallbackSet(FskTimeCallBack el, const FskTime when, FskTimeCallback callback, void *param)
#else /* SUPPORT_INSTRUMENTATION */
	void FskTimeCallbackSet_(FskTimeCallBack el, const FskTime when, FskTimeCallback callback, void *param, FSK_TIME_DEBUG_ARGS)
#endif /* SUPPORT_INSTRUMENTATION */
{
	if (!el) return;
	FskTimeCallbackRemove(el);
	el->param = param;
	el->callback = callback;
	el->trigger = *when;
	el->marked = false;
#if SUPPORT_INSTRUMENTATION
	el->schedulingFunctionName = function;
	el->schedulingFunctionLine = line;
#endif /* !SUPPORT_INSTRUMENTATION */
#if !TARGET_OS_KPL
	if (!el->ui || gUiTimersActive)
		sInsertInTime(el);
	else
		FskListPrepend(&((FskThread)el->owner)->suspendedTimerCallbacks, el);
#else
	KplTimeCallbackSet(el->kplTimeCallback, (const KplTime)when, (KplTimeCallbackProc)kplCallback, el);
#endif
}

// ---------------------------------------------------------------------
void FskTimeCallbackDispose(FskTimeCallBack el)
{
	if (NULL == el)
		return;

#if SUPPORT_INSTRUMENTATION
	if (FskThreadGetCurrent() != el->owner) {
		FskInstrumentedItemSendMessage(el, kFskTimeInstrMsgDeleteFromWrongThread, el);
	}
#endif /* !SUPPORT_INSTRUMENTATION */
	FskTimeCallbackRemove(el);
	FskInstrumentedItemDispose(el);
#if TARGET_OS_KPL
	KplTimeCallbackDispose(el->kplTimeCallback);
#endif
	FskMemPtrDispose(el);
}

// ---------------------------------------------------------------------
UInt32  FskTimeCallbackGetNextTimeDelta()
{
	FskTimeRecord now, nextTimeBasedCallback;
	SInt32 msec;

	FskTimeGetNow(&now);
	FskTimeCallbackGetNextTime(&now, &nextTimeBasedCallback, NULL);
	FskTimeSub(&now, &nextTimeBasedCallback);
	msec = FskTimeInMS(&nextTimeBasedCallback);
#if TARGET_OS_ANDROID
	if (msec < 1)
		msec = 0;
#else
	// do the other platforms need this?
	if (msec > 1000000)
		msec = 1000000;
	else if (msec < 1)
		msec = 1;
#endif

	return (UInt32)msec;
}

// ---------------------------------------------------------------------
Boolean FskTimeCallbackGetNextTime(const FskTime now, FskTime whenNext, void *threadIn)
{
	FskThread thread = threadIn ? threadIn : FskThreadGetCurrent();

	if (thread->timerCallbacks) {
		FskTimeCallBack cb = (FskTimeCallBack)thread->timerCallbacks;
		if (FskTimeCompare(now, &cb->trigger) < 1) {
			FskTimeClear(whenNext);
			return false;
		}

		*whenNext = cb->trigger;
	}
	else {
		*whenNext = *now;
		FskTimeAddSecs(whenNext, 3600 * 12);	// a long time from now
	}

	return true;
}

#if !SUPPORT_INSTRUMENTATION
	void FskTimeCallbackScheduleFuture(FskTimeCallBack callbackRef, UInt32 sec, UInt32 ms, FskTimeCallback callback, void *param)
#else /* SUPPORT_INSTRUMENTATION */
	void FskTimeCallbackScheduleFuture_(FskTimeCallBack callbackRef, UInt32 sec, UInt32 ms, FskTimeCallback callback, void *param, FSK_TIME_DEBUG_ARGS)
#endif /* SUPPORT_INSTRUMENTATION */
{
	FskTimeRecord   now;
	FskTimeRecord   future;
	FskTimeGetNow(&now);
	future.seconds = sec;
	future.useconds = ms * 1000;
	FskTimeAdd(&now, &future);
#if !SUPPORT_INSTRUMENTATION
	FskTimeCallbackSet(callbackRef, &future, callback, param);
#else /* SUPPORT_INSTRUMENTATION */
	FskTimeCallbackSet_(callbackRef, &future, callback, param, FSK_TIME_DEBUG_PARAMS_CONTINUE);
#endif /* SUPPORT_INSTRUMENTATION */
}

// ---------------------------------------------------------------------
#if !SUPPORT_INSTRUMENTATION
	void FskTimeCallbackScheduleNextRun(FskTimeCallBack callbackRef, FskTimeCallback callback, void *param)
#else /* SUPPORT_INSTRUMENTATION */
	void FskTimeCallbackScheduleNextRun_(FskTimeCallBack callbackRef, FskTimeCallback callback, void *param, FSK_TIME_DEBUG_ARGS)
#endif /* SUPPORT_INSTRUMENTATION */
{
	FskTimeRecord	now;
	FskTimeClear(&now);
#if !SUPPORT_INSTRUMENTATION
	FskTimeCallbackSet(callbackRef, &now, callback, param);
#else /* SUPPORT_INSTRUMENTATION */
	FskTimeCallbackSet_(callbackRef, &now, callback, param, FSK_TIME_DEBUG_PARAMS_CONTINUE);
#endif /* SUPPORT_INSTRUMENTATION */
}

// ---------------------------------------------------------------------
void FskTimeCallbackService(void *param)
{
	FskTimeRecord	now;
	FskTimeCallBack cb;
	FskThread thread = (FskThread)param;

#if TARGET_OS_WIN32
	#if SUPPORT_WAITABLE_TIMER
		if (thread->waitableTimer) {
			LARGE_INTEGER timeDue;

			timeDue.QuadPart = kFskUInt32Max;
			SetWaitableTimer(thread->waitableTimer, &timeDue, 0, NULL, NULL, 0);
		}
	#elif SUPPORT_TIMER_THREAD
		thread->timerSignaled = false;
	#else /* !SUPPORT_TIMER_THREAD */
		KillTimer(thread->window, kWinTimerID);
		thread->timer = 0;
	#endif /* !SUPPORT_TIMER_THREAD */
#endif /* TARGET_OS_WIN32 */

	cb = (FskTimeCallBack)thread->timerCallbacks;
	if (NULL == cb)
		return;

	// mark all those that are due
	FskTimeGetNow(&now);
#if TIME_QUANTA
	FskTimeAddMS(&now, TIME_QUANTA);
#endif /* TIME_QUANTA */
	while (cb) {
		if (FskTimeCompare(&now, &cb->trigger) > 0)
			break;
		cb->marked = true;
		cb = cb->next;
	}

	// remove any that are due
	while (1) {
		cb = (FskTimeCallBack)FskListGetNext(thread->timerCallbacks, NULL);
		if (!cb)
			break;
		if (!cb->marked)
			break;
		FskListRemove(&thread->timerCallbacks, cb);

#if SUPPORT_INSTRUMENTATION
		FskTimeGetNow(&cb->fired);
		FskInstrumentedItemSendMessage(cb, kFskTimeInstrMsgTimerFire, cb);
#endif /* SUPPORT_INSTRUMENTATION */

		(*cb->callback)(cb, &cb->trigger, cb->param);
	}

	if (thread->timerCallbacks)
		rescheduleTimer(thread);
}

#if TARGET_OS_ANDROID
// ---------------------------------------------------------------------
void FskTimeCallbackServiceUntil(void *param, FskTime until)
{
	FskTimeCallBack cb;
	FskThread thread = (FskThread)param;

	cb = (FskTimeCallBack)thread->timerCallbacks;
	if (NULL == cb)
		return;

	// mark all those that are due
	while (cb) {
		if (FskTimeCompare(until, &cb->trigger) > 0)
			break;
		cb->marked = true;
		cb = cb->next;
	}

	// remove any that are due
	while (1) {
		cb = (FskTimeCallBack)FskListGetNext(thread->timerCallbacks, NULL);
		if (!cb)
			break;
		if (!cb->marked)
			break;
		FskListRemove(&thread->timerCallbacks, cb);

#if SUPPORT_INSTRUMENTATION
		FskTimeGetNow(&cb->fired);
		FskInstrumentedItemSendMessage(cb, kFskTimeInstrMsgTimerFire, cb);
#endif /* SUPPORT_INSTRUMENTATION */

		(*cb->callback)(cb, &cb->trigger, cb->param);
	}

	if (thread->timerCallbacks)
		rescheduleTimer(thread);
}
#endif


#if TARGET_OS_WIN32

void rescheduleTimer(FskThread thread)
{
	FskTimeRecord nowTime, nextTime;

	if (NULL == thread->timerCallbacks) {
#if SUPPORT_WAITABLE_TIMER
		if (thread->waitableTimer) {
			LARGE_INTEGER timeDue;

			timeDue.QuadPart = kFskUInt32Max;
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
	FskTimeGetNow(&nowTime);

	if (!FskTimeCallbackGetNextTime(&nowTime, &nextTime, thread))
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

#elif TARGET_OS_MAC

void rescheduleTimer(FskThread thread)
{
	FskCocoaTimerReschedule(thread);
}

#endif

#if TARGET_OS_WIN32 && SUPPORT_TIMER_THREAD

void timerThread(void *refcon)
{
	extern FskListMutex gThreads;
	#define kHandleCount (3)
	HANDLE h[kHandleCount];

	h[0] = gTimerThreadRescheduleEvent;
	h[1] = gTimerThreadExitEvent;

	while (!gThreads)
		Sleep(10);

	while (gThreads) {
		DWORD result;
		SInt32 ms = 0x7fffffff;
		FskThread target = NULL, walker;
		FskTimeRecord now;

		FskMutexAcquire(gThreads->mutex);

		FskTimeGetNow(&now);
		for (walker = gThreads->list; NULL != walker; walker = walker->next) {
			FskTimeRecord delta;
			SInt32 deltaMS;

			if (!walker->timerCallbacks || walker->timerSignaled)
				continue;

			delta = walker->nextCallbackTime;
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
		FskMutexRelease(gThreads->mutex);

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

#endif /* TARGET_OS_WIN32 && SUPPORT_TIMER_THREAD */

#if TARGET_OS_WIN32 || TARGET_OS_ANDROID || TARGET_OS_MAC

void syncUiTimers(void);
static Boolean gUiActive = true;

void FskTimersSetUiActive(Boolean active)
{
	if (gUiActive != active) {
		gUiActive = active;
		syncUiTimers();
	}
}

void syncUiTimers(void)
{
	extern FskListMutex gThreads;

#if TARGET_OS_ANDROID
	Boolean screenOff = false;
	if (gFskPhoneHWInfo)
		screenOff = !gFskPhoneHWInfo->backlightOn;
#else
	const Boolean screenOff = false;
#endif

	if (!gThreads)
		return;

	FskMutexAcquire(gThreads->mutex);

	gUiTimersActive = !screenOff && gUiActive;
	if (!gUiTimersActive) {
		FskThread tWalker;

		for (tWalker = gThreads->list; NULL != tWalker; tWalker = tWalker->next) {
			FskTimeCallBack walker;

			for (walker = tWalker->timerCallbacks; NULL != walker; ) {
				FskTimeCallBack next = walker->next;
				if (walker->ui) {
					FskListRemove(&tWalker->timerCallbacks, walker);
					FskListPrepend(&tWalker->suspendedTimerCallbacks, walker);
				}
				walker = next;
			}
		}
	}
	else {
		FskThread tWalker;

		for (tWalker = gThreads->list; NULL != tWalker; tWalker = tWalker->next) {
			while (tWalker->suspendedTimerCallbacks)
				sInsertInTime((FskTimeCallBack)FskListRemoveFirst(&tWalker->suspendedTimerCallbacks));
		}
	}

	FskMutexRelease(gThreads->mutex);
}

#endif	// TARGET_OS_WIN32 || TARGET_OS_ANDROID

FskErr FskTimeInitialize() {
#if USE_POSIX_CLOCK

#if TARGET_OS_ANDROID
	int s, us;
	gAndroidCallbacks->getBasetimeCB(&s, &us);
	sBasetime.seconds = s;
	sBasetime.useconds = us;
#else
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	sBasetime.seconds = tp.tv_sec;
	sBasetime.useconds = tp.tv_nsec / kFskTimeNsecPerUsec;
#endif
#elif TARGET_OS_MAC
    baseTicks = mach_absolute_time();

    mach_timebase_info(&timeBaseInfo);
#elif TARGET_OS_WIN32 && SUPPORT_TIMER_THREAD
	HANDLE thread;

	gTimerThreadRescheduleEvent = CreateEvent(NULL, false, false, NULL);
	gTimerThreadExitEvent = CreateEvent(NULL, false, false, NULL);

	thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)timerThread, NULL, 0, NULL);
	CloseHandle(thread);
#elif TARGET_OS_KPL
	KplTimeInitialize();
#endif

	return kFskErrNone;
}

FskErr FskTimeTerminate() {
#if TARGET_OS_WIN32 && SUPPORT_TIMER_THREAD
	SetEvent(gTimerThreadExitEvent);
	while (INVALID_HANDLE_VALUE != gTimerThreadExitEvent)
		Sleep(10);
#elif TARGET_OS_KPL
	KplTimeTerminate();
#endif

	return kFskErrNone;
}
