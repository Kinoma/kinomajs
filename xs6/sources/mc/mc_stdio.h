/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#ifndef __MC_STDIO_H__
#define __MC_STDIO_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>	/* int8_t, etc. */
#include <stddef.h>	/* size_t, etc. */
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <setjmp.h>	/* in toolchain */

#include "mc_memory.h"

#define	MC_LONG_PATH	1

#if mxMC

#if WMSDK_VERSION < 212000
#include <wltypes.h>	/* this is really really really annoying... */
#else
#include <wmtime.h>
#endif
#ifndef _setjmp
#define _setjmp	setjmp
#endif
#undef LWIP_COMPAT_SOCKET
#define LWIP_COMPAT_SOCKETS	0	/* to avoid renaming very common function names, like read, write, socket... */
#include <lwip/sockets.h>	/* errno, etc. other than the socket compat functions */
#define gethostbyname(a)	lwip_gethostbyname(a)
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#if WMSDK_VERSION < 3002012
typedef unsigned int wint_t;	/* needed in the GNU stdlib */
#endif
struct mc_file;
typedef struct mc_file MC_FILE;
struct mc_dir;
typedef struct mc_dir MC_DIR;
#define EOF	-1
#define stdin	((MC_FILE *)0)	/* never used */
#define stdout	((MC_FILE *)1)
#define stderr	((MC_FILE *)2)
#define stdlcd	((MC_FILE *)3)
#define stdaux	((MC_FILE *)4)
extern int fprintf(MC_FILE *stream, const char *format, ...);
extern int vfprintf(MC_FILE *stream, const char *format, va_list ap);
#define printf(...)	fprintf(stdout, __VA_ARGS__)
#define fflush(x)	/* nothing to do in mc */
#define ERANGE	34
#define DBL_MIN		2.2250738585072015E-308
#define DBL_MAX		1.7976931348623157E+308
#if MC_LONG_PATH
#define	PATH_MAX	128	/* includes null terminator */
#else
#define PATH_MAX	(24 /* FT_MAX_FILENAME */ + 8 /* MAX_NAME */ + 2)
#endif

#else	/* !mxMC */

#define WMSDK_VERSION 2040000
#define MAX_NAME    8	/* partition name */
struct mc_file;
typedef struct mc_file MC_FILE;
struct mc_dir;
typedef struct mc_dir MC_DIR;

#endif

extern void mc_stdio_init();
extern void mc_stdio_fin();
typedef int (*mc_stdio_puts_t)(const char *str, void *closure);
extern int mc_stdio_register(mc_stdio_puts_t f, void *closure);
extern void mc_stdio_unregister(int d);

/*
 * logs
 */
#define MC_LOG_VERBOSE	3
#define MC_LOG_MEMSTATS	1
#if MC_LOG_VERBOSE > 2
#define mc_log_debug(...)	fprintf(stdout, __VA_ARGS__)
#else
#define mc_log_debug(...)
#endif
#if MC_LOG_VERBOSE > 1
#define mc_log_warning(...)	fprintf(stderr, __VA_ARGS__)
#define mc_log_notice(...)	fprintf(stdout, __VA_ARGS__)
#else
#define mc_log_warning(...)
#define mc_log_notice(...)
#endif
#if MC_LOG_VERBOSE > 0
#define mc_log_error(...)	fprintf(stderr, __VA_ARGS__)
#else
#define mc_log_error(...)
#endif
extern void mc_log_write(void *data, size_t n);
extern void mc_log_set_enable(int enable);
extern int mc_log_get_enable();

extern void mc_fatal(const char *fmt, ...);
extern void mc_exit(int status);
extern void mc_shutoff();

#endif /* __MC_STDIO_H__ */
