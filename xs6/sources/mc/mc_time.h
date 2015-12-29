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
#ifndef __MC_TIME_H__
#define __MC_TIME_H__

#if mxMC

#if WMSDK_VERSION < 2120000
#include <wltypes.h>	/* this is really really really annoying... */
#else
#include <wmtime.h>
#endif
#include <lwip/sockets.h>	/* for timeval */

struct timezone {
	int tz_minuteswest;	/* minutes west of Greenwich */
	int tz_dsttime;		/* type of DST correction */
};

struct mc_tm {
	struct tm _tm;
#define tm_sec	_tm.tm_sec
#define tm_min	_tm.tm_min
#define tm_hour	_tm.tm_hour
#define tm_mday	_tm.tm_mday
#define tm_mon	_tm.tm_mon
#define tm_year	_tm.tm_year
#define tm_wday	_tm.tm_wday
	int tm_isdst;
};

extern int mc_gettimeofday(struct timeval *tv, struct timezone *tz);
extern int mc_settimeofday(const struct timeval *tv, const struct timezone *tz);
extern time_t mc_time(time_t *);
extern struct mc_tm *mc_gmtime(const time_t *);
extern struct mc_tm *mc_localtime(const time_t *);
extern time_t mc_mktime(struct mc_tm *tm);
extern size_t mc_strftime(char *s, size_t maxsize, const char *format, const struct mc_tm *timeptr);

#else	/* mxMC */
#include <time.h>
#include <sys/time.h>

#define mc_gettimeofday(tv, tz)	gettimeofday(tv, tz)
#define mc_settimeofday(tv, tz)	settimeofday(tv, tz)
#define mc_time(t)	time(t)
#define mc_gmtime(t)	gmtime(t)
#define mc_localtime(t)	localtime(t)
#define mc_mktime(tm)	mktime(tm)
#define mc_strftime(s, sz, format, tm)	strftime(s, sz, format, tm)

#endif	/* mxMC */

#endif /* __MC_TIME_H__ */
