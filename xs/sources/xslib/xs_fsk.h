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
#if __FSK_LAYER__
	#include "Fsk.h"


	#if SUPPORT_XS_DEBUG
		#define mxDebug 1
	#else
		#undef mxDebug
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
	#define mxSolaris 0
	#define mxWindows 0
	#define mxAndroid 0
	#define mxKpl 0

	#define mxFsk 1

	#if TARGET_OS_WIN32
		#undef mxWindows
		#define mxWindows 1
		#ifdef mxDebug
			#define mxProfile 1
		#endif
	#elif TARGET_OS_MACOSX || TARGET_OS_MAC
		#if TARGET_OS_IPHONE
			#undef mxiPhone
			#define mxiPhone 1
		#else
			#undef mxMacOSX
			#define mxMacOSX 1
			#ifdef mxDebug
				#define mxProfile 1
			#endif
		#endif
	#elif TARGET_OS_ANDROID
		#undef mxAndroid
		#define mxAndroid 1
	#elif TARGET_OS_LINUX
		#undef mxLinux
		#define mxLinux 1
		#if defined(arm) || defined(__arm__)
			#define mxBastardDouble 0		// was 1 -- for kPod
											// with a buggy ARM
		#endif
	#elif TARGET_OS_KPL
		#undef mxKpl
		#define mxKpl 1
	#endif

    #define mxCleanPath(path) FskInstrumentationCleanPath(path)
#elif defined(_MSC_VER)
	#if defined(_M_IX86) || defined(_M_X64)
			#define mxBigEndian 0
			#define mxLittleEndian 1

			#define mxLinux 0
			#define mxMacOSX 0
			#define mxSolaris 0
			#define mxWindows 1
	#else
			#error unknown Microsoft compiler
	#endif
#elif defined(__GNUC__)
	#if defined(__i386__) || defined(i386) || defined(intel)
			#define mxBigEndian 0
			#define mxLittleEndian 1

			#define mxLinux 1
			#define mxMacOSX 0
			#define mxSolaris 0
			#define mxWindows 0
	#elif defined(__ppc__) || defined(powerpc) || defined(ppc)
			#define mxBigEndian 1
			#define mxLittleEndian 0

			#if defined(__linux__)
					#define mxLinux 1
					#define mxMacOSX 0
					#define mxSolaris 0
					#define mxWindows 0
			#else
					#define mxLinux 0
					#define mxMacOSX 1
					#define mxSolaris 0
					#define mxWindows 0
			#endif
	#elif defined(sparc)
			#define mxBigEndian 1
			#define mxLittleEndian 0

			#define mxLinux 0
			#define mxMacOSX 0
			#define mxSolaris 1
			#define mxWindows 0
	#else
			#error unknown GNU compiler
	#endif
#else
	#error unknown compiler
#endif

#if !mxFsk
	#include <ctype.h>
	#include <errno.h>
	#include <limits.h>
	#include <math.h>
	#include <setjmp.h>
	#include <stdarg.h>
	#include <stddef.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>
#else
	#include <limits.h>
	#include <math.h>
	#include <setjmp.h>
	#include <stdarg.h>
	#include <stddef.h>
	#include <time.h>

	#include "FskUtilities.h"
#endif

#if mxWindows

#include <float.h>
#include <sys/types.h>
#include <sys/timeb.h>


enum {
	FP_NAN          = 1,
	FP_INFINITE     = 2,
	FP_ZERO         = 3,
	FP_NORMAL       = 4,
	FP_SUBNORMAL    = 5
};
#define M_E        2.71828182845904523536
#define M_LN2      0.693147180559945309417
#define M_LN10     2.30258509299404568402
#define M_LOG2E    1.44269504088896340736
#define M_LOG10E   0.434294481903251827651
#define M_PI       3.14159265358979323846
#define M_SQRT1_2  0.707106781186547524401
#define M_SQRT2    1.41421356237309504880
#define INFINITY *(double*)infinity
extern unsigned long infinity[];
#define NAN *(double*)nan
extern unsigned long nan[];

extern int fpclassify(double x);
#define isfinite _isfinite
#define isnan _isnan
#define isnormal _isnormal

#if 0
struct timeval {
  long tv_sec;
  long tv_usec;
};

#endif

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};

extern int gettimeofday(struct timeval *tp, struct timezone *tzp);

typedef signed char txS1;
typedef unsigned char txU1;
typedef short txS2;
typedef unsigned short txU2;
typedef long txS4;
typedef unsigned long txU4;

#define mxExport extern __declspec( dllexport )
#define mxImport extern __declspec( dllimport )
#define mxVolatile(type, name, value) type name = value; type *name ## Address = &name

#elif mxMacOSX || mxiPhone

#if defined(__MWERKS__)
	#include <sys/time.h>
	#include <CoreServices.h>

#else
	#if !mxiPhone
		#include <CoreServices/CoreServices.h>
	#endif
	#include <sys/time.h>
#endif
#include <ctype.h>
#include <utime.h>


#if 0 // #ifndef FP_NAN
	enum {
		FP_NAN          = 1
	};
#endif

typedef signed char txS1;
typedef unsigned char txU1;
typedef short txS2;
typedef unsigned short txU2;
#if __LP64__
typedef int txS4;
typedef unsigned int txU4;
#else
typedef long txS4;
typedef unsigned long txU4;
#endif

#if SUPPORT_DLLEXPORT
	#define mxExport __attribute__ ((visibility("default")))
	#define mxImport __attribute__ ((visibility("default")))
#else
	#define mxExport extern
	#define mxImport extern
#endif

#define mxVolatile(type, name, value) type name = value; type *name ## Address __attribute__((unused)) = &name

#if 0
struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};

extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
#endif

#elif mxKpl
	#include "xs_fsk_kpl.h"

#elif mxFsk

#include <sys/time.h>

typedef SInt8 txS1;
typedef UInt8 txU1;
typedef SInt16 txS2;
typedef UInt16 txU2;
typedef SInt32 txS4;
typedef UInt32 txU4;

#define mxExport extern
#define mxImport extern
#define mxVolatile(type, name, value) type name = value; type *name ## Address __attribute__((unused)) = &name

#else

#include <sys/time.h>

typedef signed char txS1;
typedef unsigned char txU1;
typedef short txS2;
typedef unsigned short txU2;
typedef long txS4;
typedef unsigned long txU4;

#define mxExport extern
#define mxImport extern
#define mxVolatile(type, name, value) type name = value; type *name ## Address __attribute__((unused)) = &name

#endif

typedef long txSocket;

/* ANSI C */

#define C_EOF EOF
#define C_NULL NULL
#define C_RAND_MAX RAND_MAX

typedef jmp_buf c_jmp_buf;
typedef time_t c_time_t;
typedef struct timeval c_timeval;
typedef struct tm c_tm;
typedef va_list c_va_list;

#define c_acos acos
#define c_asin asin
#define c_atan atan
#define c_atan2 atan2
#define c_bsearch FskBSearch
#define c_calloc calloc
#define c_ceil ceil
#define c_cos cos
#define c_exit exit
#define c_exp exp
#define c_fabs fabs
#define c_floor floor
#define c_fmod fmod
#define c_free free
#define c_gmtime gmtime
#define c_log log
#define c_localtime localtime
#define c_longjmp _longjmp
#define c_malloc malloc
#define c_memcmp memcmp
#define c_memcpy memcpy
#define c_memmove memmove
#define c_memset memset
#define c_mktime mktime
#define c_pow pow
#define c_qsort qsort
#define c_rand FskRandom
#define c_setjmp _setjmp
#define c_sin sin
#define c_sprintf sprintf
#define c_sqrt sqrt
#define c_srand(a)
#define c_strcat strcat
#define c_strchr strchr
#define c_strcmp strcmp
#define c_strcpy strcpy
#define c_strftime strftime
#define c_strlen strlen
#define c_strncat strncat
#define c_strncmp strncmp
#define c_strncpy strncpy
#define c_strstr strstr
#define c_strtod strtod
#define c_strtol strtol
#define c_strtoul strtoul
#define c_strrchr strrchr
#define c_tan tan
#define c_time time
#define c_tolower tolower
#define c_toupper toupper
#define c_va_end va_end
#define c_va_start va_start
#define c_va_arg va_arg

/* IEEE 754 */

#define C_DBL_MAX DBL_MAX
#define C_DBL_MIN DBL_MIN
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
#define C_NAN NAN

#define c_fpclassify fpclassify
#define c_isfinite isfinite
#define c_isnormal isnormal
#define c_isnan isnan

#define c_gettimeofday gettimeofday

#if mxFsk
	#undef c_strcat
	#define c_strcat FskStrCat
	#undef c_strchr
	#define c_strchr FskStrChr
	#undef c_strcmp
	#define c_strcmp FskStrCompare
	#undef c_strcpy
	#define c_strcpy FskStrCopy
	#undef c_strlen
	#define c_strlen FskStrLen
	#undef c_strncat
	#define c_strncat FskStrNCat
	#undef c_strncmp
	#define c_strncmp FskStrCompareWithLength
	#undef c_strncpy
	#define c_strncpy FskStrNCopy
	#undef c_strstr
	#define c_strstr FskStrStr

	#undef c_strtod
	#define c_strtod FskStrToD
	#undef c_strtol
	#define c_strtol FskStrToL
	#undef c_strtoul
	#define c_strtoul FskStrToUL
	#undef c_strrchr
	#define c_strrchr FskStrRChr

	#undef c_malloc
	#define c_malloc(a) ((void *)FskMemPtrAlloc(a))
	#undef c_calloc
	#define c_calloc(a,b) ((void *)FskMemPtrCalloc(a*b))
	#undef c_realloc
	#define c_realloc(a,b) ((void *)FskMemPtrReallocC(a,b))
	#undef c_free
	#define c_free FskMemPtrDispose
	#undef c_memcpy
	#define c_memcpy FskMemCopy
	#undef c_memmove
	#define c_memmove FskMemMove
	#undef c_memset
	#define c_memset FskMemSet

	#undef c_qsort
	#define c_qsort(a,b,c,d) FskQSort(a,b,c,(void *)d)
	#undef c_exit
	#define c_exit FskExit

	#if mxMacOSX
		#include <float.h>
	#endif

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

#endif
