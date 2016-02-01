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
#include <sys/types.h>
#include <sys/timeb.h>
#include <string.h>

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

struct timeval {
	long tv_sec;
	long tv_usec;
};

int strcasecmp(const char *s1, const char *s2)
{
	return _stricmp(s1, s2);
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
	return _strnicmp(s1, s2, n);
}

int gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	struct _timeb tb;

	_ftime(&tb);
	if (tp != 0) {
		tp->tv_sec = (long)(tb.time);
		tp->tv_usec = tb.millitm * 1000;
	}
	if (tzp != 0) {
		tzp->tz_minuteswest = tb.timezone;
		tzp->tz_dsttime = tb.dstflag;
	}
	
	return (0);
}
