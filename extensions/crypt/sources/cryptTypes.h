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
#ifndef __CRYPT_TYPES__
#define __CRYPT_TYPES__

#if WM_FSK
#include "wm_fsk.h"
#else
#include "Fsk.h"
#include "FskUtilities.h"
#endif
#include "xs.h"

#if TARGET_OS_WIN32
typedef unsigned __int64 FskUInt64;
typedef FskUInt64 UInt64;
typedef UInt8 uint8_t;
typedef UInt16 uint16_t;
typedef UInt32 uint32_t;
typedef FskUInt64 uint64_t;
#if !defined(inline) && !defined(__cplusplus)
	#define inline __inline
#endif
#elif TARGET_OS_MAC
typedef UInt64 FskUInt64;
#else
typedef unsigned long long FskUInt64;
typedef FskUInt64 UInt64;
#endif

#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a): (b))
#endif
#ifndef MIN
#define MIN(a, b)	((a) < (b) ? (a): (b))
#endif
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

#endif /* __CRYPT_TYPES__ */
