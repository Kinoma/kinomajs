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
#include "kinoma_performance.h"

void performance_reset( PerformanceInfo *p )
{
	int i;

	p->idx		   = 0;
	p->is_full     = 0;
	p->play_fps	   = 0;
	p->movie_fps   = 0;

	for( i = 0; i < kTotalBufLen; i++ )
	{
		p->play_dur[i] = 0;
		p->dec_time[i] = 0;
	}
}

void performance_begin( PerformanceInfo *p )
{
	FskTimeGetNow(&p->begin_time);
}


void performance_end( PerformanceInfo *p, FskInt64 dec_time )
{
	FskTimeRecord end_time;
	int dur;

	FskTimeGetNow(&end_time);
	
	FskTimeSub( &p->begin_time, &end_time );
	dur = (end_time.seconds*1000*1000) + end_time.useconds;
	//dur = end_time.useconds - p->begin_time.useconds;
	//if( end_time.seconds != p->begin_time.seconds )
	//	dur += (end_time.seconds - p->begin_time.seconds)*1000*1000;

	p->play_dur[p->idx] = dur;
	p->dec_time[p->idx] = dec_time;
	p->idx++;
	if( p->idx >= kTotalBufLen )
	{
		p->is_full = 1;
		p->idx     = 0;
	}
}

#define CHECK_RANGE(a) { if( a< 0)				 a += kTotalBufLen;		\
						 if( a >= kTotalBufLen ) a -= kTotalBufLen; }
void performance_process( PerformanceInfo *p )
{
	int total_play_time   = 0;
	int total_movie_time  = 0;
	int total_count = p->is_full? kTotalBufLen  : p->idx;
	int first_idx   = p->is_full? p->idx		: 0;
	int last_idx    = p->idx - 1;
	int i;

	CHECK_RANGE( first_idx )
	CHECK_RANGE( last_idx )

	for( i = 0; i < kTotalBufLen; i++ )
		 total_play_time += p->play_dur[i];

	total_movie_time = (int)(p->dec_time[last_idx]-p->dec_time[first_idx]);	/* TODO: should be use a 64-bit time? */

	p->play_fps  = total_play_time == 0 ? 9999 : (float)total_count     / (float)total_play_time  * 1000 * 1000;
	p->movie_fps = total_movie_time== 0 ? 9999 : (float)(total_count-1) / (float)total_movie_time * 3000;
	
	//if( p->play_fps <= 0 )
	//{
	//	int a = 1;
	//}
	//fprintf( stderr, "#%d: play_fps=%f, movie_fps=%f, \n", p->idx, p->play_fps, p->movie_fps);

}
