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
#ifndef __XSPLATFORM__
#define __XSPLATFORM__

#include "Fsk.h"
#include "FskUtilities.h"

#if SUPPORT_XS_DEBUG
	#define mxDebug 1
#else
	#undef mxDebug
#endif

#if SUPPORT_XS_PROFILE
	#define mxProfile 1
#else
	#undef mxProfile
#endif

#if TARGET_RT_BIG_ENDIAN
	#define mxBigEndian 1
	#define mxLittleEndian 0
#else
	#define mxBigEndian 0
	#define mxLittleEndian 1
#endif

#define mxLinux 0
#define mxMacOSX 0
#define mxiPhone 0
#define mxWindows 0
#define mxAndroid 0
#define mxKpl 0
#define mxFsk 1

#if TARGET_OS_WIN32
	#undef mxWindows
	#define mxWindows 1
	#define mxExport extern __declspec( dllexport )
	#define mxImport extern __declspec( dllimport )
#elif TARGET_OS_MACOSX || TARGET_OS_MAC
	#if TARGET_OS_IPHONE
		#undef mxiPhone
		#define mxiPhone 1
	#else
		#undef mxMacOSX
		#define mxMacOSX 1
	#endif
	#define mxExport __attribute__ ((visibility("default")))
	#define mxImport __attribute__ ((visibility("default")))
#elif TARGET_OS_ANDROID
	#undef mxAndroid
	#define mxAndroid 1
	#define mxExport extern    
	#define mxImport extern
#elif TARGET_OS_LINUX
	#undef mxLinux
	#define mxLinux 1
	#define mxExport extern    
	#define mxImport extern
#elif TARGET_OS_KPL
	#undef mxKpl
	#define mxKpl 1
	#define mxExport extern    
	#define mxImport extern
#endif

#define XS_FUNCTION_NORETURN FSK_FUNCTION_NORETURN
#define XS_FUNCTION_ANALYZER_NORETURN FSK_FUNCTION_ANALYZER_NORETURN

#define mxCleanPath(path) FskInstrumentationCleanPath(path)
#if SUPPORT_INSTRUMENTATION
enum {
	kFskXSInstrAllocateChunks = kFskInstrumentedItemFirstCustomMessage,
	kFskXSInstrAllocateSlots,
	kFskXSInstrSkipCollect,
	kFskXSInstrBeginCollectSlots,
	kFskXSInstrBeginCollectSlotsAndChunks,
	kFskXSInstrEndCollectChunks,
	kFskXSInstrEndCollectSlots,
	kFskXSInstrNewChunkWork,
	kFskXSInstrTrace,
	kFskXSInstrReportError,
	kFskXSInstrReportWarning,
	kFskXSInstrException
};
#endif

typedef SInt8 txS1;
typedef UInt8 txU1;
typedef SInt16 txS2;
typedef UInt16 txU2;
typedef SInt32 txS4;
typedef UInt32 txU4;

typedef void* txSocket;
#define mxNoSocket NULL

#if mxWindows
	#define mxVolatile(type, name, value) type name = value; type *name ## Address = &name
#else
	#define mxVolatile(type, name, value) type name = value; type *name ## Address __attribute__((unused)) = &name
#endif

/* C */

#include <ctype.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#define C_EOF EOF
#define C_NULL NULL

#define c_tolower tolower
#define c_toupper toupper

typedef jmp_buf c_jmp_buf;
#if defined(_MSC_VER)
	#define c_longjmp longjmp
#else
	#define c_longjmp _longjmp
#endif
#define c_setjmp _setjmp

typedef va_list c_va_list;
#define c_va_arg va_arg
#define c_va_end va_end
#define c_va_start va_start
	
#define c_calloc(a,b) ((void *)FskMemPtrCalloc(a*b))
#define c_exit FskExit
#define c_free FskMemPtrDispose
#define c_malloc(a) ((void *)FskMemPtrAlloc(a))
#define c_qsort(a,b,c,d) FskQSort(a,b,c,(void *)d)
#define c_realloc(a,b) ((void *)FskMemPtrReallocC(a,b))
#define c_strtod FskStrToD
#define c_strtol FskStrToL
#define c_strtoul FskStrToUL
	
/* DATE */
	
#if mxWindows
	#include <sys/types.h>
	#include <sys/timeb.h>
	struct timezone {
		int tz_minuteswest;
		int tz_dsttime;
	};
	extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
#else
	#include <sys/time.h>
#endif
#include <time.h>

typedef time_t c_time_t;
typedef struct timeval c_timeval;
typedef struct tm c_tm;

#define c_gettimeofday gettimeofday
#define c_gmtime gmtime
#define c_localtime localtime
#define c_mktime mktime
#define c_strftime strftime
#define c_time time
	
/* ERROR */
	
#define C_ENOMEM kFskErrMemFull
#define C_EINVAL kFskErrInvalidParameter
	
/* MATH */

#if mxWindows
	#include <math.h>
	#include <float.h>
	#define M_E        2.71828182845904523536
	#define M_LN2      0.693147180559945309417
	#define M_LN10     2.30258509299404568402
	#define M_LOG2E    1.44269504088896340736
	#define M_LOG10E   0.434294481903251827651
	#define M_PI       3.14159265358979323846
	#define M_SQRT1_2  0.707106781186547524401
	#define M_SQRT2    1.41421356237309504880
	#if _MSC_VER < 1800
		enum {
			FP_NAN          = 1,
			FP_INFINITE     = 2,
			FP_ZERO         = 3,
			FP_NORMAL       = 4,
			FP_SUBNORMAL    = 5
		};
		#define INFINITY *(double*)infinity
		extern unsigned long infinity[];
		#define NAN *(double*)nan
		extern unsigned long nan[];
		extern int fpclassify(double x);
		#define isfinite _isfinite
		#define isnan _isnan
		#define isnormal _isnormal
	#endif
#elif mxLinux
	#if !defined(__USE_ISOC99)
		#define __USE_ISOC99 1
		#include <math.h>
		#include <float.h>
		#undef __USE_ISOC99
	#else
		#include <math.h>
		#include <float.h>
	#endif
#else
	#include <math.h>
	#include <float.h>
#endif

#define C_DBL_MAX DBL_MAX
#define C_DBL_MIN DBL_MIN
#define C_EPSILON (double)2.2204460492503130808472633361816e-16
#define C_FP_INFINITE FP_INFINITE
#define C_FP_NAN FP_NAN
#define C_FP_NORMAL FP_NORMAL
#define C_FP_SUBNORMAL FP_SUBNORMAL
#define C_FP_ZERO FP_ZERO
#define C_INFINITY INFINITY
#define C_M_E M_E
#define C_M_LN10 M_LN10
#define C_M_LN2 M_LN2
#define C_M_LOG10E M_LOG10E
#define C_M_LOG2E M_LOG2E
#define C_M_PI M_PI
#define C_M_SQRT1_2 M_SQRT1_2
#define C_M_SQRT2 M_SQRT2
#define C_MAX_SAFE_INTEGER (double)9007199254740991
#define C_MIN_SAFE_INTEGER (double)-9007199254740991
#define C_NAN NAN
#define C_RAND_MAX RAND_MAX

#define c_acos acos
#define c_acosh acosh
#define c_asin asin
#define c_asinh asinh
#define c_atan atan
#define c_atanh atanh
#define c_atan2 atan2
#define c_cbrt cbrt
#define c_ceil ceil
#define c_clz __builtin_clz
#define c_cos cos
#define c_cosh cosh
#define c_exp exp
#define c_expm1 expm1
#define c_fabs fabs
#define c_floor floor
#define c_fmod fmod
#define c_fpclassify fpclassify
#define c_hypot hypot
#define c_isfinite isfinite
#define c_isnormal isnormal
#define c_isnan isnan
#define c_llround llround
#define c_log log
#define c_log1p log1p
#define c_log10 log10
#define c_log2 log2
#define c_pow pow
#define c_rand rand
#define c_round round
#define c_sin sin
#define c_sinh sinh
#define c_sqrt sqrt
#define c_srand srand
#define c_tan tan
#define c_tanh tanh
#define c_trunc trunc

/* STRING */

#define c_memcpy FskMemCopy
#define c_memmove FskMemMove
#define c_memset FskMemSet
#define c_strcat FskStrCat
#define c_strchr FskStrChr
#define c_strcmp FskStrCompare
#define c_strcpy FskStrCopy
#define c_strlen FskStrLen
#define c_strncat FskStrNCat
#define c_strncmp FskStrCompareWithLength
#define c_strncpy FskStrNCopy
#define c_strstr FskStrStr
#define c_strrchr FskStrRChr

#endif /* __XSPLATFORM__ */
