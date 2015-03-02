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
#ifndef __KPL_TIME_H__
#define __KPL_TIME_H__

#include "Kpl.h"
#include "FskErrors.h"

typedef struct {
	SInt32	seconds;
	SInt32	useconds;
} KplTimeRecord, *KplTime;

typedef struct {
    SInt32	tm_sec;         /* seconds : normally in the range 0 to 59, but can be up to 60 to allow for leap seconds.*/
    SInt32	tm_min;         /* minutes : range 0 to 59 */
    SInt32	tm_hour;        /* hours   : range 0 to 23 */
    SInt32	tm_mday;        /* day of the month: in the range 1 to 31*/
    SInt32	tm_mon;         /* month   :  range 0 to 11*/
    SInt32	tm_year;        /* year    : The number of years since 1900*/
    SInt32	tm_wday;        /* day of the week : range 0 to 6.*/
    SInt32	tm_yday;        /* day in the year : range 0 to 365*/
    SInt32	tm_isdst;       /* daylight saving time :  whether daylight saving time is in effect*/

	SInt32	tm_gmtoff;  	/* Offset of current TZ from GMT in seconds */
	const char *tm_zone;	/* TZ name */

	// Above fileds should not be changed (include order)!!!
	// Below is other fileds
}KplTimeElementsRecord, *KplTimeElements;


KplDeclarePrivateType(KplTimeCallback)

typedef void (*KplTimeCallbackProc)(KplTimeCallback callback, const KplTime time, void *param);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

FskErr KplTimeInitialize(void);
void KplTimeTerminate(void);

void KplTimeCallbackNew(KplTimeCallback *callback);
void KplTimeCallbackDispose(KplTimeCallback callback);
void KplTimeCallbackSet(KplTimeCallback callback, const KplTime when, KplTimeCallbackProc callbackProc, void *param);
void KplTimeCallbackCancel(KplTimeCallback callback);

void KplTimeGetNow(KplTime t);
void KplTimeMakeDate(char *dateString, int dateStringSize);

UInt32 KplTimeStrftime(char *s, UInt32 max, const char *format, const KplTimeElements fsktm);
char * KplTimeStrptime(const char *s, const char *format,  KplTimeElements fsktm);
FskErr KplTimeGmtime(const KplTime fsktime, KplTimeElements fsktm);
FskErr KplTimeLocaltime(const KplTime fsktime, KplTimeElements fsktm);
void KplTimeMktime(const KplTimeElements fsktm, KplTime fsktime);
FskErr KplTimeGetZone(KplTime fsktime, SInt32 *tzoff, SInt32 *dst, const char **tzName);
void KplTimeGetOSTime(KplTime t);
FskErr KplTimeStime(SInt32 secs);
FskErr KplTimeTzset(const char *tzName);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __KPL_TIME_H__
