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

#ifndef _DEFINES_H_
#define _DEFINES_H_

// WWD: Please note: If we have large cache, we should define this to utilize it fully. Otherwise, we should not define this.!!!!!!!!!!!!!!
//#define		LARGE_CACHE


//#define PERFORMANCE_TEST

#ifdef __KINOMA_IPP__

#define VLD_LARGE		16
#define VLD_ERROR		127

#define ZEROLEFT_1		2
#define ZEROLEFT_2		6


#if  defined( WIN32)
#define _DECLSPEC  __declspec(align(16))
#else
#define _DECLSPEC
#endif

static inline int clip_uint8( int a )
{
    if (a&(~255))
        return (-a)>>31;
    else
        return a;
}

#define IClip(Min, Max, Val) (((Val) < (Min)) ? (Min) : (((Val) > (Max)) ? (Max) : (Val)))
#define absm(A) ((A)<(0) ? (-(A)):(A))

#define TOTRUN_NUM 15
#define RUNBEFORE_NUM 7

#ifndef MAX_NUM_PIC_PARAM_SETS
#define MAX_NUM_PIC_PARAM_SETS 256
#endif

#endif

#endif
