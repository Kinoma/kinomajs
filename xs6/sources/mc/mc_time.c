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
#include <wm_os.h>
#include <mdev_rtc.h>
#include "mc_env.h"
#include "mc_time.h"

#define MC_TIME_ZONE	"TIME_ZONE"
#define MC_TIME_DIFF	"TIME_DIFF"
#define MC_TIME_DST	"DST"

int
mc_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	if (tv != NULL) {
		unsigned long ticks = rtc_drv_get(NULL);

		tv->tv_sec = wmtime_time_get_posix();
		tv->tv_usec = ((ticks & 0x3ff) * 1000 * 1000) / 1024;
	}
	if (tz != NULL) {
		const char *td = mc_env_get_default(MC_TIME_DIFF);
		const char *dst = mc_env_get_default(MC_TIME_DST);
		tz->tz_minuteswest = td ? atoi(td) : 0;
		tz->tz_dsttime = dst ? atoi(dst) : 0;
	}
	return 0;
}

int
mc_settimeofday(const struct timeval *tv, const struct timezone *tz)
{
	if (tv != NULL)
		wmtime_time_set_posix(tv->tv_sec);
	if (tz != NULL) {
		char buf[13];
		snprintf(buf, sizeof(buf), "%d", tz->tz_minuteswest);
		mc_env_set_default(MC_TIME_DIFF, buf);
		snprintf(buf, sizeof(buf), "%d", tz->tz_dsttime);
		mc_env_set_default(MC_TIME_DST, buf);
		mc_env_store(NULL);
	}
	return 0;
}

time_t
mc_time(time_t *tp)
{
	time_t t = wmtime_time_get_posix();

	if (tp != NULL)
		*tp = t;
	return t;
}

struct mc_tm *
mc_gmtime(const time_t *t)
{
	static struct mc_tm result;
	const char *dst;

	gmtime_r(t, &result._tm);
	dst = mc_env_get_default(MC_TIME_DST);
	result.tm_isdst = dst ? atoi(dst) : -1;
	return &result;
}

struct mc_tm *
mc_localtime(const time_t *t)
{
	const char *td = mc_env_get_default(MC_TIME_DIFF);
	time_t tt;

	if (td != NULL) {
		tt = *t + atoi(td) * 60;
		t = &tt;
	}
	return mc_gmtime(t);
}

time_t
mc_mktime(struct mc_tm *tm)
{
	return mktime(&tm->_tm);
}

size_t
mc_strftime(char *s, size_t maxsize, const char *format, const struct mc_tm *timeptr)
{
	uint32_t outSize = 0;
	static char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	static char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	while (true) {
		char temp[50];
		const char *outs = NULL;

		if (0 == *format) {
			*s++ = 0;
			outSize += 1;
			goto bail;
		}

		if ('%' == *format) {
			format += 1;
			switch (*format++) {
				case 0:
					goto bail;

				case 'a':
					outs = days[timeptr->tm_wday];
					break;

				case 'b':
					outs = months[timeptr->tm_mon];
					break;

				case 'd':
					outs = temp;
					snprintf(temp, sizeof(temp), "%d", timeptr->tm_mday);
					break;

				case 'Y':
					outs = temp;
					snprintf(temp, sizeof(temp), "%d", timeptr->tm_year + 1900);
					break;

				case 'H':
					outs = temp;
					temp[0] = (timeptr->tm_hour / 10) + '0';
					temp[1] = (timeptr->tm_hour % 10) + '0';
					temp[2] = 0;
					break;

				case 'M':
					outs = temp;
					temp[0] = (timeptr->tm_min / 10) + '0';
					temp[1] = (timeptr->tm_min % 10) + '0';
					temp[2] = 0;
					break;

				case 'S':
					outs = temp;
					temp[0] = (timeptr->tm_sec / 10) + '0';
					temp[1] = (timeptr->tm_sec % 10) + '0';
					temp[2] = 0;
					break;

				case 'z': {
					const char *td = mc_env_get_default(MC_TIME_DIFF);
					outs = temp;
					snprintf(temp, sizeof(temp), "%d", td ? atoi(td) / 60 : 0);
					break;
				}
				case 'Z': {
					const char *tz = mc_env_get_default(MC_TIME_ZONE);
					outs = tz ? tz : "";
					break;
				}
			}
			if (outs) {
				uint16_t len = strlen(outs);
				strncpy(s, outs, len);
				outSize += len;
				s += len;
			}
		}
		else {
			*s++ = *format++;
			outSize += 1;
		}
	}


bail:
	return outSize;
}
