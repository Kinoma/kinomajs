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
#ifndef __XS_FSK_KPL_H__
#define __XS_FSK_KPL_H__

#include "Kpl.h"

typedef SInt8 txS1;
typedef UInt8 txU1;
typedef SInt16 txS2;
typedef UInt16 txU2;
typedef SInt32 txS4;
typedef UInt32 txU4;

struct timeval {
  long tv_sec;
  long tv_usec;
};

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};

extern int tolower(int c);
extern int gettimeofday(struct timeval *tp, struct timezone *tzp);

#if defined(_MSC_VER)	// @@ KPL building on Visual Studio
	#include <stdarg.h>
	#include <float.h>
	#define NAN *(double*)nan
	#define isnan _isnan
	extern unsigned long nan[];
	#define INFINITY *(double*)infinity
	extern unsigned long infinity[];
	extern int fpclassify(double x);
#endif

#define M_E        2.71828182845904523536
#define M_LN2      0.693147180559945309417
#define M_LN10     2.30258509299404568402
#define M_LOG2E    1.44269504088896340736
#define M_LOG10E   0.434294481903251827651
#define M_PI       3.14159265358979323846
#define M_SQRT1_2  0.707106781186547524401
#define M_SQRT2    1.41421356237309504880

enum {
	FP_NAN          = 1,
	FP_INFINITE     = 2,
	FP_ZERO         = 3,
	FP_NORMAL       = 4,
	FP_SUBNORMAL    = 5
};

#define mxExport extern
#define mxImport extern
#define mxVolatile(type, name, value) type name = value; type *name ## Address __attribute__((unused)) = &name

#endif
