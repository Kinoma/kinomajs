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
#ifndef __TIME_H__
#define __TIME_H__

#include "Fsk.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct FskTimeRecord {
	SInt32	seconds;
	SInt32	useconds;
};
typedef struct FskTimeRecord FskTimeRecord;
typedef struct FskTimeRecord *FskTime;

#if USE_POSIX_CLOCK
	extern FskTimeRecord sBasetime;
#endif /* USE_POSIX_CLOCK */
	
struct FskTimeElementsRecord {
    SInt32	tm_sec;         /* seconds : normally in the range 0 to 59, but can be up to 60 to allow for leap seconds.*/
    SInt32	tm_min;         /* minutes : range 0 to 59 */
    SInt32	tm_hour;        /* hours   : range 0 to 23 */
    SInt32	tm_mday;        /* day of the month: in the range 1 to 31*/
    SInt32	tm_mon;         /* month   :  range 0 to 11*/
    SInt32	tm_year;        /* year    : The number of years since 1900*/
    SInt32	tm_wday;        /* day of the week : range 0 to 6.*/
    SInt32	tm_yday;        /* day in the year : range 0 to 365*/
    SInt32	tm_isdst;       /* daylight saving time :  whether daylight saving time is in effect*/
	
	/// The following two are only valid when after call FskTimeLocaltime
	SInt32	tm_gmtoff;		/* Offset of current TZ from GMT in seconds */
	const char *tm_zone;	/* TZ name */
};

typedef struct FskTimeElementsRecord FskTimeElementsRecord;
typedef struct FskTimeElementsRecord *FskTimeElements;

#define kFskTimeUnits (1000)

#define kFskTimeMsecPerSec (1000)
#define kFskTimeUsecPerSec (kFskTimeMsecPerSec * 1000)
#define kFskTimeUsecPerMsec (1000)
#define kFskTimeNsecPerSec (kFskTimeUsecPerSec * 1000)
#define kFskTimeNsecPerMsec (kFskTimeNsecPerSec / kFskTimeMsecPerSec)
#define kFskTimeNsecPerUsec (1000)

FskAPI(void) FskTimeMakeDate(char *dateString, int stringSize);

/*! 
 * \fn FskAPI(void)	FskTimeStrftime(char *s, UInt32 max, const char *format, const FskTimeElements fsktm, UInt32 *wLen)
 * \brief formats the broken-down time fsktm according to the format specification format and places the result 
 *        in the character array s of size max
 * \param[out]	s		Array to store result
 * \param[in]	max		max length of array s
 * \param[in]	format	format for converting broken-dwon time to string
 * \param[in]	fsktm	broken-down time
 * \param[out]	wLen	the number of characters placed in the array s, not including the terminating null byte
 * \return 	void
 */
FskAPI(void)	FskTimeStrftime(char *s, UInt32 max, const char *format, const FskTimeElements fsktm, UInt32 *wLen);

/*! 
 * \fn FskAPI(void)	FskTimeStrptime(const char *s, const char *format,  FskTimeElements fsktm, char **endptr)
 * \brief It is the converse function to FskTimeStrftime and converts the character string pointed to by s 
 *        to values which are stored in the tm structure pointed to by tm, using the format specified by format.
 * \param[in]	s		string for parsing
 * \param[in]	format	format for converting string to broken-dwon time
 * \param[out]	fsktm	broken-down time
 * \param[out]	endptr	point to the first char in string which un-processed
 * \return 	void
 */
FskAPI(void)	FskTimeStrptime(const char *s, const char *format,  FskTimeElements fsktm, char **endptr);

/*! 
 * \fn FskAPI(FskErr)	FskAPI(FskErr)	FskTimeGmtime(const FskTime fsktime, FskTimeElements fsktm)
 * \brief converts the calendar time fsktime to broken-down time representation, expressed in Coordinated Universal Time (UTC).
 * \param[in]	fsktime	calendar time
 * \param[out]	fsktm	broken-down time (UTC)
 * \return 	kFskErrNone for correct, others are error
 */
FskAPI(FskErr)	FskTimeGmtime(const FskTime fsktime, FskTimeElements fsktm);

/*! 
 * \fn FskAPI(FskErr)	FskTimeLocaltime(const FskTime fsktime, FskTimeElements fsktm)
 * \brief converts the calendar time fsktime to broken-down time representation, expressed relative to the user's specified timezone.
 * \param[in]	fsktime	calendar time
 * \param[out]	fsktm	broken-down time (Local)
 * \return 	kFskErrNone for correct, others are error
 */
FskAPI(FskErr)	FskTimeLocaltime(const FskTime fsktime, FskTimeElements fsktm);

/*! 
 * \fn FskAPI(void)	FskTimeMktime(const FskTimeElements fsktm, FskTime fsktime)
 * \brief converts a broken-down time structure fsktm, expressed as local time, to calendar time representation fsktime.
 * \param[in]	fsktm	broken-down time
 * \param[out]	fsktime	calendar time
 * \return 	void
 */
FskAPI(void)	FskTimeMktime(const FskTimeElements fsktm, FskTime fsktime);

/*! 
 * \fn FskAPI(FskErr)	FskTimeStime(const FskTime fsktime)
 * \brief Set the system clock time and date.
 * \param[in]	fsktime	seconds since Epoch
 * \returns 	kFskErrNone on success, otherwise a non-zero FskErr code
 */
FskAPI(FskErr)	FskTimeStime(const FskTime fsktime);

/*! 
 * \fn FskAPI(FskErr)	FskTimeTzset(const char *tzName)
 * \brief Set the system time conversion information.
 * \param[in]	tzName	time zone name, e.g. "America/Los_Angeles"
 * \returns 	kFskErrNone on success, otherwise a non-zero FskErr code
 */
FskAPI(FskErr)	FskTimeTzset(const char *tzName);
	
/*! 
 * \fn FskAPI(FskErr)	FskTimeGetZone(FskTime fsktime, SInt32 *tzoff, SInt32 *dst, const char **tzName)
 * \brief Get Tzone offset (in seconds), day-light saving and tZone name information according to calendar time supplied fsktime
 *        fsktime can be NULL. If it is NULL, FskTimeGetZone will use current time.
 * \param[in]	fsktime	calendar time (It is always NULL on windows)
 * \param[out]	tzoff	Time-zone offset (in seconds). For example CST(China standard time, +8) will return 28800
 * \param[out]	dst		day-light saving on or off at that time.
 * \param[out]	tzName	point to a static string for time-zone name
 * \return 	kFskErrNone for correct, others are error
 */
FskAPI(FskErr)	FskTimeGetZone(FskTime fsktime, SInt32 *tzoff, SInt32 *dst, const char **tzName);

/*! 
 * \fn FskAPI(FskErr)	FskTimeGetDisplayZone(char *tzName)
 * \param[out]	tzName		Get display timezone name for default setting (For example, Asia/Shanghai, America/Los_Angeles etc).
 * \return 	kFskErrNone for correct, others are error
 */
FskAPI(FskErr)	FskTimeGetDisplayZone(char **tzName);


/*! 
 * \fn FskAPI(void)	FskTimeGetOSTime(FskTime time)
 * \brief Get absolute time of OS (from Epoch)
 * \param[out]	t	point to FskTime
 * \return 	void
 */
FskAPI(void)	FskTimeGetOSTime(FskTime time);

#define kFskTime1224Default -1
FskAPI(SInt8) FskTimeGetOS1224();

FskAPI(void) FskTimeGetNow(FskTime time);
FskAPI(void) FskTimeClear(FskTime time);
FskAPI(void) FskTimeCopy(FskTime t1, FskTime t2);
FskAPI(void) FskTimeAdd(FskTime time1, FskTime time2);
FskAPI(void) FskTimeAddMS(FskTime time, UInt32 ms);
FskAPI(void) FskTimeAddSecs(FskTime time, UInt32 secs);
FskAPI(void) FskTimeSub(FskTime time1, FskTime time2);  // time2 = time2 - time1
FskAPI(void) FskTimeSubMS(FskTime time, UInt32 ms);
FskAPI(void) FskTimeSubSecs(FskTime time, UInt32 secs);
FskAPI(SInt32) FskTimeCompare(const FskTime time1, const FskTime time2);
FskAPI(SInt32) FskTimeInSecs(FskTime time);
/*
	Note: FskTimeInMS is intended only to be used with relatively small time differences.
		It is not intended to be used for absolute times.
		It will overflow when the argument is the current time. 
*/
FskAPI(SInt32) FskTimeInMS(FskTime time);

typedef struct FskTimeCallBackRecord FskTimeCallBackRecord;
typedef struct FskTimeCallBackRecord *FskTimeCallBack;

typedef void (*FskTimeCallback)(FskTimeCallBack callback, const FskTime time, void *param);

#if SUPPORT_INSTRUMENTATION
enum {
	kFskTimeInstrMsgScheduleFirst = kFskInstrumentedItemFirstCustomMessage,
	kFskTimeInstrMsgScheduleMid,
	kFskTimeInstrMsgScheduleEnd,
	kFskTimeInstrMsgTimerFire,
	kFskTimeInstrMsgDeleteFromWrongThread
};
#endif /* SUPPORT_INSTRUMENTATION */

#if TARGET_OS_WIN32 || TARGET_OS_ANDROID || TARGET_OS_MAC
	void FskTimersSetUiActive(Boolean active);
#else /* !TARGET_OS_WIN32 */
	#define FskTimersSetUiActive(active)
#endif /* !TARGET_OS_WIN32 */

#if !defined(__FSKTIME_PRIV__) && !SUPPORT_INSTRUMENTATION
#else /* __FSKTIME_PRIV__ */
	struct FskTimeCallBackRecord {
		FskTimeCallBack		next;
		void				*param;
		FskTimeRecord		trigger;	// when the timer was supposed to fire
#if SUPPORT_INSTRUMENTATION
		const char			*allocatingName;
		FskTimeRecord		fired;		// when the timer fired
		const char			*schedulingFunctionName;
		UInt32				schedulingFunctionLine;
#endif /* SUPPORT_INSTRUMENTATION */
		FskTimeCallback		callback;
		Boolean				marked;
		Boolean				ui;
		struct FskThreadRecord *owner;
#if TARGET_OS_KPL
		void				*kplTimeCallback;
#endif

		FskInstrumentedItemDeclaration
	};

	FskErr FskTimeInitialize(void);
	FskErr FskTimeTerminate(void);

	Boolean FskTimeCallbackGetNextTime(const FskTime now, FskTime whenNext, void *thread);
	UInt32 FskTimeCallbackGetNextTimeDelta(void);		// in ms
	void FskTimeCallbackService(void *thread);
#if TARGET_OS_ANDROID
	void FskTimeCallbackServiceUntil(void *thread, FskTime until);
#endif

#endif /* __FSKTIME_PRIV__ */

#if !SUPPORT_INSTRUMENTATION
	FskAPI(void) FskTimeCallbackNew(FskTimeCallBack *callbackRef);
#else
	FskAPI(void) FskTimeCallbackNew_(FskTimeCallBack *callbackRef, const char *name);
	#define FskTimeCallbackNew(callbackRef) FskTimeCallbackNew_(callbackRef, __FUNCTION__)
#endif

FskAPI(void) FskTimeCallbackUINew(FskTimeCallBack *callbackRef);
FskAPI(void) FskTimeCallbackDispose(FskTimeCallBack callbackRef);

#if !SUPPORT_INSTRUMENTATION
	FskAPI(FskTimeCallBack) FskTimeCallbackAddNew(const FskTime when, FskTimeCallback callback, void *param);
	FskAPI(void) FskTimeCallbackSet(FskTimeCallBack callbackRef, const FskTime when, FskTimeCallback callback, void *param);
	FskAPI(void) FskTimeCallbackScheduleNextRun(FskTimeCallBack callbackRef, FskTimeCallback callback, void *param);
	FskAPI(void) FskTimeCallbackScheduleFuture(FskTimeCallBack callbackRef, UInt32 sec, UInt32 msec, FskTimeCallback callback, void *param);
#else /* SUPPORT_INSTRUMENTATION */
	#define FSK_TIME_DEBUG_ARGS UInt32 line, const char *function
	#define FSK_TIME_DEBUG_PARAMS __LINE__, __FUNCTION__
	#define FSK_TIME_DEBUG_PARAMS_CONTINUE line, function

	#define FskTimeCallbackAddNew(when, callback, param) FskTimeCallbackAddNew_(when, callback, param, FSK_TIME_DEBUG_PARAMS) 
	FskAPI(FskTimeCallBack) FskTimeCallbackAddNew_(const FskTime when, FskTimeCallback callback, void *param, FSK_TIME_DEBUG_ARGS);

	#define FskTimeCallbackSet(callbackRef, when, callback, param) FskTimeCallbackSet_(callbackRef, when, callback, param, FSK_TIME_DEBUG_PARAMS) 
	FskAPI(void) FskTimeCallbackSet_(FskTimeCallBack callbackRef, const FskTime when, FskTimeCallback callback, void *param, FSK_TIME_DEBUG_ARGS);

	#define FskTimeCallbackScheduleNextRun(callbackRef, callback, param) FskTimeCallbackScheduleNextRun_(callbackRef, callback, param, FSK_TIME_DEBUG_PARAMS)
	FskAPI(void) FskTimeCallbackScheduleNextRun_(FskTimeCallBack callbackRef, FskTimeCallback callback, void *param, FSK_TIME_DEBUG_ARGS);

	#define FskTimeCallbackScheduleFuture(callbackRef, sec, msec, callback, param) FskTimeCallbackScheduleFuture_(callbackRef, sec, msec, callback, param, FSK_TIME_DEBUG_PARAMS)
	FskAPI(void) FskTimeCallbackScheduleFuture_(FskTimeCallBack callbackRef, UInt32 sec, UInt32 msec, FskTimeCallback callback, void *param, FSK_TIME_DEBUG_ARGS);
#endif /* SUPPORT_INSTRUMENTATION */


FskAPI(void) FskTimeCallbackRemove(FskTimeCallBack callbackRef);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __TIME_H__
