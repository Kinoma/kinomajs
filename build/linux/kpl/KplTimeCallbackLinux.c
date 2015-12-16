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
#include "FskList.h"
#include "FskTime.h"
#include "FskErrors.h"

#include "KplTime.h"
#include "KplThreadLinuxPriv.h"
#include "KplTimeCallbackLinuxPriv.h"

static void sInsertInTime(KplTimeCallback el);
static void rescheduleTimer(KplThread thread);

#if USE_POSIX_CLOCK
#else
static FskTimeRecord gDeltaTime;
#endif

Boolean KplTimeCallbackGetNextTime(const KplTime now, KplTime whenNext, void *threadIn);

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

// KplTimeCallbackServiceUntil is called from the KplThreadRunloopCycle() function to call timer callbacks that are due
void KplTimeCallbackServiceUntil(void *param, KplTime until)
{
	KplTimeCallback cb;
	KplThread thread = (KplThread)param;

	cb = (KplTimeCallback)thread->timerCallbacks;
	if (NULL == cb)
		return;

	// mark all those that are due
	while (cb) {
		if (FskTimeCompare((FskTime)until, (FskTime)&cb->trigger) >= 0)
			break;
		cb->marked = true;
		cb = cb->next;
	}

	// Remove and call any timer callbacks that are due
	while (1) {
		cb = (KplTimeCallback)FskListGetNext(thread->timerCallbacks, NULL);
		if (!cb)
			break;
		if (!cb->marked)
			break;
		FskListRemove(&thread->timerCallbacks, cb);

		(*cb->callbackProc)(cb, &cb->trigger, cb->param);
	}

	if (thread->timerCallbacks)
		rescheduleTimer(thread);
}

UInt32 KplTimeCallbackGetNextTimeDelta()
{
	KplTimeRecord now, nextTimeBasedCallback;
	SInt32 msec;

	KplTimeGetNow(&now);
	KplTimeCallbackGetNextTime(&now, &nextTimeBasedCallback, NULL);
	FskTimeSub((FskTime)&now, (FskTime)&nextTimeBasedCallback);
	msec = FskTimeInMS((FskTime)&nextTimeBasedCallback);

	// Clamp
	if (msec > 1000000)
		msec = 1000000;
	else if (msec < 1)
		msec = 1;

	return msec;
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

// Insert this timer callback into the owning thread's "timerCallbacks" queue.
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
	// Nothing necessary here
}

void KplTimeTerminate()
{
	// Nothing necessary here
}

FskErr KplTimeInitialize()
{
#if USE_POSIX_CLOCK
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	sBasetime.seconds = tp.tv_sec;
	sBasetime.useconds = tp.tv_nsec / kFskTimeNsecPerUsec;
#else
	gDeltaTime.seconds = 0;
	gDeltaTime.useconds= 0;
#endif
	return kFskErrNone;
}

void KplTimeGetNow(KplTime t)
{
#if USE_POSIX_CLOCK
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	t->seconds = tp.tv_sec;
	t->useconds = tp.tv_nsec / kFskTimeNsecPerUsec;
	FskTimeSub(&sBasetime, (FskTime)t);
#else
	struct timeval tv;	
	struct timezone tz;
	gettimeofday(&tv, &tz);
	t->seconds = tv.tv_sec;
	t->useconds = tv.tv_usec;
	// correction for system time adjusted with KplTimeStime
	FskTimeSub(&gDeltaTime, (FskTime)t);
#endif
}

void KplTimeMakeDate(char *dateString, int dateStringSize)
{
	time_t tsec;
	time(&tsec);
	strftime(dateString, dateStringSize, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&tsec));
}

// Use this function to avoid members order of struct tm be modified in different toolchains and platforms
static inline void CopyToKplTimeElementsCommon(struct tm *src, KplTimeElements dst){
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
static inline void CopyFromKplTimeElementsCommon(KplTimeElements src, struct tm *dst){
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

UInt32 KplTimeStrftime(char *s, UInt32 max, const char *format, const KplTimeElements kpltm){
	UInt32			len = 0;
	struct tm 		itm;

	CopyFromKplTimeElementsCommon(kpltm, &itm);
	itm.tm_gmtoff= 0;
	itm.tm_zone	= NULL;
	len = (UInt32)strftime(s, max, format, (const struct tm *) &itm);

	return len;
}

char *KplTimeStrptime(const char *s, const char *format,  KplTimeElements kpltm){
	char *p;
	struct tm 	itm;

	p = strptime(s, format, &itm);
	CopyToKplTimeElementsCommon(&itm, kpltm);
	kpltm->tm_gmtoff= itm.tm_gmtoff;
	kpltm->tm_zone	= itm.tm_zone;

	return p;
}

FskErr KplTimeGmtime(const KplTime kpltime, KplTimeElements kpltm) {
	FskErr			err = kFskErrNone;
	struct tm		itm;
	time_t			tsec;
	void			*p;

	tsec = (time_t)kpltime->seconds;

	p = gmtime_r(&tsec, &itm);
	if(NULL == p)
		err = kFskErrUnknown;
	CopyToKplTimeElementsCommon(&itm, kpltm);
	kpltm->tm_gmtoff= 0;
	kpltm->tm_zone	= NULL;

	return err;
}

FskErr KplTimeLocaltime(const KplTime kpltime, KplTimeElements kpltm){
	FskErr			err = kFskErrNone;
	struct tm		itm;
	time_t			tsec;
	void			*p;

	tsec = (time_t)kpltime->seconds;

	p = localtime_r(&tsec, &itm);
	if(NULL == p)
		err = kFskErrUnknown;
	CopyToKplTimeElementsCommon(&itm, kpltm);
	kpltm->tm_gmtoff= itm.tm_gmtoff;
	kpltm->tm_zone	= itm.tm_zone;	//static

	return err;
}

void KplTimeMktime(const KplTimeElements kpltm, KplTime kpltime){
	struct tm		itm;
	time_t			tsec;

	CopyFromKplTimeElementsCommon(kpltm, &itm);
	itm.tm_gmtoff = kpltm->tm_gmtoff;	//We assume user give all correct information
	tsec = mktime(&itm);

	kpltime->seconds = tsec;
	kpltime->useconds= 0;

	return;
}

FskErr KplTimeStime(SInt32 secs)
{
#ifdef ANDROID_PLATFORM
	FskErr err = kFskErrUnimplemented;
	return err;
#else 
	int result;
	FskErr err = kFskErrNone;
	time_t tsecs = secs;
#if USE_POSIX_CLOCK
	result = stime(&tsecs);
	if (0 != result)
		err = kFskErrUnknown;
#else
	struct timeval tv;
	FskTimeRecord b, a;
	
	gettimeofday(&tv, NULL);
	b.seconds = tv.tv_sec;
	b.useconds= tv.tv_usec;
	
	result = stime(&tsecs);
	
	gettimeofday(&tv, NULL);
	a.seconds = tv.tv_sec;
	a.useconds= tv.tv_usec;
	FskTimeSub(&b, &a);
	
	if (0 != result)
		err = kFskErrUnknown;
	else
		gDeltaTime.seconds += a.seconds; // stime only adjusts seconds
#endif
	
	return err;
#endif 
}

extern double gxDeltaTime;

FskErr KplTimeTzset(const char *tzName)
{
	struct tm		itm;
	
	setenv("TZ", tzName, 1);
	tzset();

	memset(&itm, 0, sizeof(struct tm));
    itm.tm_mday = 2;
    itm.tm_year = 70;
	itm.tm_isdst = -1;
	gxDeltaTime = 1000.0 * ((long)mktime(&(itm)) - (24L * 3600L));
	return kFskErrNone;
}

FskErr KplTimeGetZone(KplTime kpltime, SInt32 *tzoff, SInt32 *dst, const char **tzName) {
	struct tm		itm;
	time_t			tsec;

	if(NULL == kpltime) {
		struct timeval tv;	
		gettimeofday(&tv, NULL);
		tsec = tv.tv_sec;
	}
	else {
		tsec = kpltime->seconds;
	}
	localtime_r(&tsec, &itm);

	if(tzoff)
		*tzoff = (SInt32)itm.tm_gmtoff;;
	if(dst)
		*dst = itm.tm_isdst;
	if(tzName)
		*tzName = itm.tm_zone;

	return kFskErrNone;
}

void KplTimeGetOSTime(KplTime t)
{
#if USE_POSIX_CLOCK
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	t->seconds = tp.tv_sec;
	t->useconds = tp.tv_nsec / kFskTimeNsecPerUsec;
#else
	struct timeval tv;	
	struct timezone tz;
	gettimeofday(&tv, &tz);
	t->seconds = tv.tv_sec;
	t->useconds = tv.tv_usec;
#endif
}
