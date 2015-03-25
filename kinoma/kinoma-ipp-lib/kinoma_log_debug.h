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
#ifndef __KINOMA_LOG_H__
#define __KINOMA_LOG_H__

#include <stdio.h>
#include <string.h>
//#include <math.h>


#ifdef BNIE_LOG	

#ifndef BNIE_LOG_FILE_PATH
#define BNIE_LOG_FILE_PATH	"/sdcard/e"
#endif

extern FILE *fErr;
#define dlog(...)							\
do{												\
	if( fErr == NULL )							\
		fErr = fopen(BNIE_LOG_FILE_PATH, "w"); 	\
												\
	if( fErr != NULL )							\
	{											\
		fprintf(fErr, "%5d:   ",__LINE__);		\
		fprintf(fErr,__VA_ARGS__);				\
		fflush( fErr );							\
	}											\
}while(0)


#define dlog_0(...)							\
do{												\
	if( fErr == NULL )							\
		fErr = fopen(BNIE_LOG_FILE_PATH, "w"); 	\
												\
	if( fErr != NULL )							\
	{											\
		fprintf(fErr,__VA_ARGS__);				\
		fflush( fErr );							\
	}											\
}while(0)


#define viewerr_exit(err)	 { dlog("XXXXXXXXXXXXXXXXXXXXXXXXXXXX err = %x/%d, exiting!!!\n", (int)err,(int)err); exit(-1); }
#define viewerr_safe(err)	 { dlog("XXXXXXXXXXXXXXXXXXXXXXXXXXXX err = %x/%d\n", (int)err,(int)err);}

#elif defined(BNIE_INSTRUMENT)
//need to put the following code before this file
//#include "FskPlatformImplementation.h"
//FskInstrumentedSimpleType(MY_TYPE, my_name);
//#define dlog(...)  do { FskInstrumentedTypePrintfDebug  (&g##MY_TYPE##TypeInstrumentation, __VA_ARGS__); } while(0)
#define viewerr_exit(err)	 { dlog("XXXXXXXXXXXXXXXXXXXXXXXXXXXX err = %x/%d, exiting!!!\n", (int)err,(int)err); exit(-1); }
#define viewerr_safe(err)	 { dlog("XXXXXXXXXXXXXXXXXXXXXXXXXXXX err = %x/%d\n", (int)err,(int)err);}

#elif defined(BNIE_LOGCAT)

//#include <cutils/uio.h>
//#include <cutils/logd.h>
//#include "../../build/android/OSS/system/core/include/log.h"

#define dlog LOGD

#define viewerr_exit(err)	 { dlog("XXXXXXXXXXXXXXXXXXXXXXXXXXXX err = %x/%d, exiting!!!\n", (int)err,(int)err); exit(-1); }
#define viewerr_safe(err)	 { dlog("XXXXXXXXXXXXXXXXXXXXXXXXXXXX err = %x/%d\n", (int)err,(int)err);}


#else
#define dlog(...)

#define viewerr_exit(err)
#define viewerr_safe(err)

#endif

#define BailErr( err )     { if( err ) { viewerr_safe( err ); goto bail; } }
#define BailErr0( err )    { if( err ) { viewerr_safe( err ); goto bail; } }


#endif	//__KINOMA_LOG_H__

