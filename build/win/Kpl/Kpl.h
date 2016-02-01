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
typedef __int64 KplInt64;

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
#undef TARGET_ALIGN_PACK
#define TARGET_ALIGN_PACK 0

#undef TARGET_ALIGN_PACKPUSH
#define TARGET_ALIGN_PACKPUSH 0

// long long support
#define TARGET_HAS_LONG_LONG 0

#endif // __KPL_H__
