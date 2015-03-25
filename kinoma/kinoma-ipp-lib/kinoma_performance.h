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
#ifndef KINOMA_PERFORMANCE
#define KINOMA_PERFORMANCE

#include "FskTime.h"

#define kTotalBufLen 30

#ifdef __cplusplus
extern "C" {
#endif


typedef struct 
{
	FskTimeRecord		begin_time;
	int					idx;
	int					is_full;
	int					play_dur[kTotalBufLen];
	FskInt64			dec_time[kTotalBufLen];
	float				play_fps;
	float				movie_fps;
} PerformanceInfo;

void performance_reset(   PerformanceInfo *p );
void performance_begin(   PerformanceInfo *p );
void performance_end(     PerformanceInfo *p, FskInt64 dec_time );
void performance_process( PerformanceInfo *p );

#ifdef __cplusplus
}
#endif

#endif		//KINOMA_PERFORMANCE
