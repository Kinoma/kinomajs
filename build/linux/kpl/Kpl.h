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
#ifndef __KPL_H__
#define __KPL_H__

#include "KplCommon.h"
#include <stdint.h>

/*
	Portable types
*/
typedef unsigned int UInt32;
typedef int SInt32;
typedef unsigned short UInt16;
typedef short SInt16;
typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned char Boolean;
typedef long long KplInt64;

/********************************************************************************
 * Determine the operating system and
 * abstract out the gross CPU hardware characteristics.
 *		TARGET_OS_XXX
 *		TARGET_CPU_XXX
 *		TARGET_RT_XXX_ENDIAN
 ********************************************************************************/
#ifndef TARGET_OS_KPL
	#define TARGET_OS_KPL 1
#endif

#define TARGET_RT_LITTLE_ENDIAN 1
//#define TARGET_CPU_ARM	1

/*
    Alignment techniques
*/
#define TARGET_ALIGN_PACK 1
#define TARGET_ALIGN_PACKPUSH 0

// long long support
#define TARGET_HAS_LONG_LONG 1

#include <ctype.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#ifndef ANDROID_PLATFORM
    // this is not found in the ANDROID PLATFORM SOURCE TREE
#include <values.h>
#endif


#ifndef FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE
	#if	defined(__i386__)	|| defined(i386)	|| defined(_M_IX86)	|| defined(__k8__)		|| \
		defined(powerpc)	|| defined(ppc)		|| defined(__ppc__)	|| defined(__POWERPC__)	|| defined(__ppc64__)
			#define FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE 1
	#else
			#define FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE 0
	#endif /* CPU switch */
#endif /* MARVELL_SOC_PXA168 */


#endif // __KPL_H__
