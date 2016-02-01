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
extern int toupper(int c);
extern int tolower(int c);
extern int isdigit(int c);

//---------- strings.h function declarations ----------
extern int strcasecmp(const char *s1, const char *s2);
extern int strncasecmp(const char *s1, const char *s2, size_t n);

//---------- FskFixedMath.c configuration ---------- 

// Set USE_ASSEMBLER_MULTIPLICATION to 1 to enable assembler version.  Only supported for PPC and X86.
#define USE_ASSEMBLER_MULTIPLICATION    0

// Set USE_ASSEMBLER_DIVISION to 1 to enable assembler version.  Only supported for PPC and X86.
#define USE_ASSEMBLER_DIVISION          0

// Set USE_ASSEMBLER_RATIO to 1 to enable assembler version.  Only supported for PPC and X86.
#define USE_ASSEMBLER_RATIO             0

// Set to 1 to enable saturation for these operations.  Typically set to 1 for ARM CPU targets.
#define USE_MULTIPLICATION_SATURATION   0
#define USE_DIVISION_SATURATION         0
#define USE_SHIFT_SATURATE              0

//---------- FskRectBlit.c configuration ---------- 

// Set to 1 to enable ARM versions of YUV to RGB blitters.
#define USE_ARM_YUV_TO_RGB				0

//---------- FskYUV420Copy.c configuration ---------- 

// Define only one of the YUV420i_RGB macros to use either "C" or ARM implementations of the YUV 420->RGB integer blitters
#define YUV420i_RGB_C_IMPLEMENTATION
//#define YUV420i_RGB_ARM_IMPLEMENTATION

#endif
