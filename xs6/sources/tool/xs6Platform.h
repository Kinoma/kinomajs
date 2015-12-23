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

#define mxBigEndian 0
#define mxLittleEndian 0

#define mxFsk 0
#define mxiOS 0
#define mxKpl 0
#define mxLinux 0
#define mxMacOSX 0
#define mxWindows 0

#if defined(_MSC_VER)
	#if defined(_M_IX86) || defined(_M_X64)
		#undef mxLittleEndian
		#define mxLittleEndian 1
		#undef mxWindows
		#define mxWindows 1
		#define mxExport extern __declspec( dllexport )
		#define mxImport extern __declspec( dllimport )
	#else 
		#error unknown Microsoft compiler
	#endif
#elif defined(__GNUC__) 
	#if defined(__i386__) || defined(i386) || defined(intel) || defined(arm) || defined(__arm__) || defined(__k8__) || defined(__x86_64__)
		#undef mxLittleEndian
		#define mxLittleEndian 1
		#if defined(android) || defined(ANDROID)
		#elif defined(__linux__) || defined(linux)
			#undef mxLinux
			#define mxLinux 1
			#define mxExport extern    
			#define mxImport extern
		#else
			#if defined(iphone)
				#undef mxiOS
				#define mxiOS 1
			#else
				#undef mxMacOSX
				#define mxMacOSX 1
			#endif
			#define mxExport __attribute__ ((visibility("default")))
			#define mxImport __attribute__ ((visibility("default")))
		#endif
        #define XS_FUNCTION_NORETURN __attribute__((noreturn))
        #define XS_FUNCTION_ANALYZER_NORETURN
        #if defined(__clang__)
            #if __has_feature(attribute_analyzer_noreturn)
                #undef XS_FUNCTION_ANALYZER_NORETURN
                #define XS_FUNCTION_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
            #endif
        #endif
	#else 
		#error unknown GNU compiler
	#endif
#else 
	#error unknown compiler
#endif

#ifndef XS_FUNCTION_NORETURN
    #define XS_FUNCTION_NORETURN
#endif
#ifndef XS_FUNCTION_ANALYZER_NORETURN
    #define XS_FUNCTION_ANALYZER_NORETURN
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

#if mxWindows
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <winsock2.h>
	typedef SOCKET txSocket;
	#define mxNoSocket INVALID_SOCKET
#else
	#include <arpa/inet.h>
	typedef int txSocket;
	#define mxNoSocket -1
#endif

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
#include <stdlib.h>

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
	
#define c_calloc calloc
#define c_exit exit
#define c_free free
#define c_malloc malloc
#define c_qsort qsort
#define c_realloc realloc
#define c_strtod strtod
#define c_strtol strtol
#define c_strtoul strtoul
#if mxWindows
#define PATH_MAX 1024
extern char *realpath(const char *path, char *resolved_path);
#define snprintf _snprintf
#endif
	
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
	
#if mxWindows
	#define C_ENOMEM ERROR_NOT_ENOUGH_MEMORY
	#define C_EINVAL ERROR_INVALID_DATA
#else
	#include <errno.h>
	#define C_ENOMEM ENOMEM
	#define C_EINVAL EINVAL
#endif
	
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
#define C_INFINITY (double)INFINITY
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
#define c_signbit signbit
#define c_sin sin
#define c_sinh sinh
#define c_sqrt sqrt
#define c_srand srand
#define c_tan tan
#define c_tanh tanh
#define c_trunc trunc

/* STRING */

#include <string.h>
#define c_memcpy memcpy
#define c_memmove memmove
#define c_memset memset
#define c_strcat strcat
#define c_strchr strchr
#define c_strcmp strcmp
#define c_strcpy strcpy
#define c_strlen strlen
#define c_strncat strncat
#define c_strncmp strncmp
#define c_strncpy strncpy
#define c_strstr strstr
#define c_strrchr strrchr

#endif /* __XSPLATFORM__ */
