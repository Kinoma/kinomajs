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
#ifndef __FSKPLATFORMIMPLEMENTATIONKPL_H__
#define __FSKPLATFORMIMPLEMENTATIONKPL_H__

//---------- ctype.h function declarations ----------
//extern int toupper(int c);
//extern int isdigit(int c);

//---------- strings.h function declarations ----------
//extern int strcasecmp(const char *s1, const char *s2);
//extern int strncasecmp(const char *s1, const char *s2, size_t n);

//---------- FskFixedMath.c configuration ---------- 

#if TARGET_CPU_ARM && !defined(ANDROID_PLATFORM)
	#if TARGET_CPU_ARM64
		#define USE_ASSEMBLER_MULTIPLICATION    0
		#define USE_ASSEMBLER_DIVISION          0
		#define USE_ASSEMBLER_RATIO             0
	#else
		#define USE_ASSEMBLER_MULTIPLICATION    1
		#define USE_ASSEMBLER_DIVISION          1
		#define USE_ASSEMBLER_RATIO             1
	#endif
#else
#define USE_ASSEMBLER_MULTIPLICATION    0
#define USE_ASSEMBLER_DIVISION          0
#define USE_ASSEMBLER_RATIO             0
#endif

// Set to 1 to enable saturation for these operations.  Typically set to 1 for ARM CPU targets.
#define USE_MULTIPLICATION_SATURATION   1
#define USE_DIVISION_SATURATION         1
#define USE_SHIFT_SATURATE              0

//---------- FskRectBlit.c configuration ---------- 

// Set to 1 to enable ARM versions of YUV to RGB blitters.
#if TARGET_CPU_ARM && !TARGET_CPU_ARM64
#define USE_ARM_YUV_TO_RGB				1
#else
#define USE_ARM_YUV_TO_RGB				0
#endif

//---------- FskYUV420Copy.c configuration ---------- 

// Set only one of the YUV420i_RGB macros to use either "C" or ARM implementations of the YUV 420->RGB integer blitters
#if SRC_YUV420i
	#if TARGET_CPU_ARM && !TARGET_CPU_ARM64
		#define YUV420i_RGB_ARM_IMPLEMENTATION
	#else
		#define YUV420i_RGB_C_IMPLEMENTATION
	#endif
#elif SRC_YUV420
	#ifdef SUPPORT_WMMX
		#define SUPPORT_YUV420_WMMX_OPT			1
	#endif
#endif

// Network sockets
#ifndef INADDR_ANY
	#define INADDR_ANY	0
#endif

#endif
