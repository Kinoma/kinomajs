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

#define mxExport __attribute__ ((visibility("default")))
#define mxImport __attribute__ ((visibility("default")))

#define mxBigEndian 0
#define mxLittleEndian 1

#define mxFsk 0
#define mxiOS 0
#define mxKpl 0
#define mxLinux 0
#define mxMacOSX 0
#define mxWindows 0

#define XS_FUNCTION_NORETURN __attribute__((noreturn))
#define XS_FUNCTION_ANALYZER_NORETURN

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
typedef int txSocket;
#define mxNoSocket -1

#include "mc_stdio.h"
#include <lwip/netdb.h>
#include <lwip/inet.h>

#define C_EOF EOF
#define C_NULL NULL

#define c_tolower tolower
#define c_toupper toupper

typedef jmp_buf c_jmp_buf;
#define c_longjmp longjmp
#define c_setjmp setjmp

typedef va_list c_va_list;
#define c_va_arg va_arg
#define c_va_end va_end
#define c_va_start va_start
	
#define c_calloc mc_calloc
#define c_exit mc_exit
#define c_free mc_free
#define c_malloc mc_malloc
#define c_qsort qsort
#define c_realloc mc_realloc
#define c_strtod mc_strtod
#define c_strtol strtol
#define c_strtoul strtoul
	
/* DATE */
#include "mc_time.h"
typedef time_t c_time_t;
typedef struct timeval c_timeval;
typedef struct mc_tm c_tm;

#define c_gettimeofday mc_gettimeofday
#define c_gmtime mc_gmtime
#define c_localtime mc_localtime
#define c_mktime mc_mktime
#define c_strftime mc_strftime
#define c_time mc_time

/* ERROR */
	
#define C_ENOMEM ENOMEM
#define C_EINVAL EINVAL
	
/* MATH */
#include <math.h>

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
#define c_signbit signbit
#define c_sin sin
#define c_sinh sinh
#define c_sqrt sqrt
#define c_srand srand
#define c_tan tan
#define c_tanh tanh
#define c_trunc trunc

/* STRING */

#define c_memcpy memcpy
#define c_memmove memmove
#define c_memset memset
#define c_memcmp memcmp
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

/* stdio compatiblity functions */
#include "mc_file.h"
#define	FILE	MC_FILE
#define	DIR		MC_DIR
#define fopen(path, mode)	mc_fopen(path, mode)
#define fclose(fp)		mc_fclose(fp)
#define fread(ptr, sz, n, fp)	mc_fread(ptr, sz, n, fp)
#define fwrite(ptr, sz, n, fp)	mc_fwrite(ptr, sz, n, fp)
#define fseek(fp, off, wh)	mc_fseek(fp, off, wh)
#define ftell(fp)		mc_ftell(fp)
#define fgets(ptr, sz, fp)	mc_fgets(ptr, sz, fp)
#define fputs(ptr, fp)		mc_fputs(ptr, fp)
#define creat(path)		mc_creat(path)
#define unlink(path)		mc_unlink(path)
static inline int fgetc(FILE *fp) {return mc_getc(fp);}

/* socket */
#if !LWIP_COMPAT_SOCKETS
#define socket(a,b,c)		lwip_socket(a,b,c)
#define fcntl(a,b,c)		lwip_fcntl(a,b,c)
/* #define gethostbyname(a)	lwip_gethostbyname(a) */	/* defined in mc_stdio.h */
#define connect(a,b,c)		lwip_connect(a,b,c)
#define select(a,b,c,d,e)	lwip_select(a,b,c,d,e)
#define getsockopt(a,b,c,d,e)	lwip_getsockopt(a,b,c,d,e)
#define closesocket(a)		lwip_close(a)
#define read(a,b,c)		lwip_read(a,b,c)
#define write(a,b,c)		lwip_write(a,b,c)
#define ioctl(a,b,c)		lwip_ioctl(a,b,c)

#endif /* !LWIP_COMPAT_SOCKETS */

/* dynamic linker */
#include "mc_dl.h"
enum {RTLD_LAZY, RTLD_NOW};
#define dlopen(path, mode)	mc_dlopen(path)
#define dlsym(handle, symbol)	mc_dlsym(handle, symbol)
#define dlclose(handle)		mc_dlclose(handle)
#define dlerror()		mc_dlerror()

/* misc */
#include "mc_misc.h"
#define realpath(path, r)	mc_realpath(path, r)
#define strerror(n)		mc_strerror(n)

/* just for pcre */
static inline void *mc_pcre_malloc(size_t sz) {
	return mc_malloc(sz);
}
static inline void mc_pcre_free(void *p) {
	mc_free(p);
}
#define malloc	mc_pcre_malloc
#define free	mc_pcre_free

#endif /* __XSPLATFORM__ */
