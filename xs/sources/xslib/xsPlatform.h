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

#ifndef __XSPLATFORM__
#define __XSPLATFORM__

#if __FSK_LAYER__
	#include "xs_fsk.h"
#else	

#define mxBigEndian 0
#define mxLittleEndian 0
#define mxBastardDouble 0

#define mxKpl 0
#define mxLinux 0
#define mxMacOSX 0
#define mxiPhone 0
#define mxSolaris 0
#define mxWindows 0

#if defined(KPL)
	#undef mxKpl
	#define mxKpl 1
	#include "Kpl.h"
#elif defined(_MSC_VER)
	#if defined(_M_IX86) || defined(_M_X64)
		#undef mxLittleEndian
		#define mxLittleEndian 1
		#undef mxWindows
		#define mxWindows 1
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
			#if defined(arm) || defined(__arm__)
				#undef mxArmLinux
				#define mxArmLinux 1
			#elif defined(mips) || defined(_mips)
				#undef mxMipsLinux
				#define mxMipsLinux 1
			#endif
		#else
			#if defined(iphone)
				#undef mxiPhone
				#define mxiPhone 1
			#else
				#undef mxMacOSX
				#define mxMacOSX 1
			#endif
		#endif
	#elif defined(__ppc__) || defined(powerpc) || defined(ppc)
		#undef mxBigEndian
		#define mxBigEndian 1
		#if defined(__linux__)
			#undef mxLinux
			#define mxLinux 1
		#else  
			#undef mxMacOSX
			#define mxMacOSX 1
		#endif
	#elif defined(sparc)
		#undef mxBigEndian
		#define mxBigEndian 1
		#undef mxSolaris
		#define mxSolaris 1
	#else 
		#error unknown GNU compiler
	#endif
#else 
	#error unknown compiler
#endif

#include <ctype.h>

#include <errno.h>

#include <limits.h>

#if mxLinux
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
#endif

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#if mxWindows
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
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
	#define isfinite _isfinite
	#define isnan _isnan
	#define isnormal _isnormal
	extern int fpclassify(double x);
	
	struct timezone {
		int tz_minuteswest;
		int tz_dsttime;
	};
	extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
	
	#ifdef mxXSC
		#define mxExport extern
		#define mxImport extern
	#else
		#define mxExport extern __declspec( dllexport )
		#define mxImport extern __declspec( dllimport )
	#endif
	#define mxVolatile(type, name, value) type name = value; type *name ## Address = &name

	typedef SOCKET txSocket;

#elif mxiPhone
	#include <fcntl.h>
	#include <float.h>
	#include <netdb.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <unistd.h>
	#include <pwd.h>

	#define mxExport extern
	#define mxImport extern
	#define mxVolatile(type, name, value) type name = value; type *name ## Address __attribute__((unused)) = &name

	typedef int txSocket;
	
#elif mxMacOSX

	#include <CoreServices/CoreServices.h>
	#include <sys/time.h>
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pwd.h>
	
	#define mxExport extern
	#define mxImport extern
	#define mxVolatile(type, name, value) type name = value; type *name ## Address __attribute__((unused)) = &name

	typedef int txSocket;

#elif mxSolaris

	#include <sys/time.h>
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pwd.h>
	#include <signal.h>

	enum {
		FP_NAN          = 1,
		FP_INFINITE     = 2,
		FP_ZERO         = 3,
		FP_NORMAL       = 4,
		FP_SUBNORMAL    = 5
	};
	#define INFINITY *(double*)infinity
	extern unsigned long infinity[];
	#define NAN *(double*)__nan
	extern unsigned long __nan[];
	extern int fpclassify(double x);

	#define mxExport extern
	#define mxImport extern
	#define mxVolatile(type, name, value) type name = value; type *name ## Address __attribute__((unused)) = &name

	typedef int txSocket;

#else

	#include <sys/time.h>
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pwd.h>

	#define mxExport extern    
	#define mxImport extern
	#define mxVolatile(type, name, value) type name = value; type *name ## Address __attribute__((unused)) = &name

	typedef int txSocket;
	
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
#define c_bsearch bsearch
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
#if defined(_MSC_VER)
	#define c_longjmp longjmp
#else
	#define c_longjmp _longjmp
#endif
#define c_malloc malloc
#define c_memcmp memcmp
#define c_memcpy memcpy
#define c_memmove memmove
#define c_memset memset
#define c_mktime mktime
#define c_pow pow
#define c_qsort qsort
#define c_realloc realloc
#define c_rand rand
#if mxWindows
	#define c_setjmp setjmp
#else
	#define c_setjmp _setjmp
#endif
#define c_sin sin
#define c_sqrt sqrt
#define c_srand srand
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
#define c_va_arg va_arg
#define c_va_end va_end
#define c_va_start va_start
	
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
	
#endif

#ifndef XS_FUNCTION_NORETURN
    #if mxMacOSX || mxiPhone || mxLinux
        #define XS_FUNCTION_NORETURN __attribute__((noreturn))
    #else
        #define XS_FUNCTION_NORETURN
    #endif
#endif
#ifndef XS_FUNCTION_ANALYZER_NORETURN
    #define XS_FUNCTION_ANALYZER_NORETURN
    #if (mxMacOSX || mxiPhone || mxLinux) && defined(__clang__)
        #if __has_feature(attribute_analyzer_noreturn)
            #undef XS_FUNCTION_ANALYZER_NORETURN
            #define XS_FUNCTION_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
        #endif
    #endif
#endif


#define mxEndian16_Swap(a)         \
	((((txU1)a) << 8)      |   \
	(((txU2)a) >> 8))

#if mxLittleEndian
	#define mxMisaligned16_GetN(a)         \
		(((txU2)((txU1*)(a))[1] << 8) |  \
		((txU2)((txU1*)(a))[0] << 0))
	#define mxEndianU16_LtoN(a) (a)
#else
	#define mxMisaligned16_GetN(a)         \
		(((txU2)((txU1*)(a))[0] << 8) |  \
		((txU2)((txU1*)(a))[1] << 0))
	#define mxEndianU16_LtoN(a) ((txU2)mxEndian16_Swap(a))
#endif
#endif /* __XSPLATFORM__ */
