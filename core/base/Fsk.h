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
#ifndef __FSK__
#define __FSK__


/********************************************************************************
 * Determine the operating system and
 * abstract out the gross CPU hardware characteristics.
 *		TARGET_OS_XXX
 *		TARGET_CPU_XXX
 *		TARGET_RT_XXX_ENDIAN
 ********************************************************************************/

#if defined(KPL)
	#define TARGET_OS_KPL 1
	#include "Kpl.h"

#elif defined(_WIN32)
	#define TARGET_OS_WIN32 1
    #define TARGET_RT_LITTLE_ENDIAN 1
#elif (defined(macintosh) && macintosh) || defined(__APPLE__)
	#define TARGET_OS_MAC	1
	#if (defined(iphone) && iphone)
		#define TARGET_OS_IPHONE 1
		#define TARGET_RT_LITTLE_ENDIAN 1
	#else
		#if defined(__i386__) || defined(i386) || defined(__k8__) || defined(intel) || defined(arm) || defined(__arm__) || defined(__arm64__) || (defined(__LITTLE_ENDIAN__) && __LITTLE_ENDIAN__)
			#define TARGET_RT_LITTLE_ENDIAN 1
		#elif defined(__ppc__) || defined(powerpc) || defined(ppc)
			#define TARGET_RT_BIG_ENDIAN 1
		#endif
	#endif
#elif defined(__GNUC__)
	#if defined(__i386__) || defined(i386) || defined(intel) || defined(arm) || defined(__arm__) || defined(__arm64__)
		#define TARGET_OS_LINUX	1
		#define TARGET_RT_LITTLE_ENDIAN 1
	#elif defined(__ppc__) || defined(powerpc) || defined(ppc)
		#define TARGET_RT_BIG_ENDIAN 1
		#if defined(__linux__)
			#define TARGET_OS_LINUX	1
		#else
			#define TARGET_OS_MACOSX 1
		#endif
	#elif defined(mips) || defined(_mips)
		#define TARGET_OS_LINUX	1
		#define TARGET_RT_LITTLE_ENDIAN 1
	#endif
#else
	#error what is your platform??
#endif

#if TARGET_OS_LINUX
	#define _GNU_SOURCE	1
	#include <ctype.h>
	#include <math.h>
	#include <limits.h>
#if !defined(__SSLIBC__) && !defined(ANDROID) && !defined(ANDROID_PLATFORM)
	#include <values.h>
#endif

#if defined(ANDROID)
	#define TARGET_OS_ANDROID 1
#endif
#endif

#include <stdio.h>
#include <stdlib.h>

//@@ N.B. mxDebug is not the optimial trigger for this, I think... maybe just us NDEBUG like ANSI?
#if mxDebug && TARGET_OS_WIN32
    #define FskAssert(it) if (!(it)) DebugBreak()
#elif mxDebug && (TARGET_OS_MAC || TARGET_OS_ANDROID || TARGET_OS_LINUX)
    #include <assert.h>
    #define FskAssert(a)    \
        do {                \
            assert((a));    \
        } while (0)
#else
    #define FskAssert(a)
#endif

/*
	Portable types
*/

#if TARGET_OS_WIN32
	typedef unsigned long UInt32;
	typedef long SInt32;
	typedef unsigned short UInt16;
	typedef short SInt16;
	typedef unsigned char UInt8;
	typedef signed char SInt8;
	typedef unsigned char Boolean;
	typedef __int64 FskInt64;

	#if !defined(__QTML__) && !defined(__cplusplus) && !defined(true)
		enum {
			false = 0,
			true = 1
		};
	#endif
#elif TARGET_OS_MAC
    #include <CoreFoundation/CoreFoundation.h>
    typedef SInt64 FskInt64;
#elif TARGET_OS_LINUX || TARGET_OS_MACOSX
#if defined(KPL)
#else
	typedef unsigned long UInt32;
	typedef long SInt32;
	typedef unsigned short UInt16;
	typedef short SInt16;
	typedef unsigned char UInt8;
	typedef signed char SInt8;
	typedef unsigned char Boolean;
#endif
	typedef long long FskInt64;
	#if !defined(true) && !defined(__cplusplus)
		enum {
			false = 0,
			true = 1
		};
	#endif

	#ifndef NULL
		#define NULL 0L
	#endif
#elif TARGET_OS_KPL
	typedef KplInt64 FskInt64;

	#if !defined(true) && !defined(__cplusplus)
		enum {
			false = 0,
			true = 1
		};
	#endif

	#ifndef NULL
		#define NULL 0L
	#endif
#else
	#pragma error define your core types
#endif

typedef FskInt64 FskFileOffset;

#define kFskUInt16Max ((UInt16)0xFFFFu)
#define kFskUInt32Max ((UInt32)0xFFFFFFFFu)
#define kFskSInt16Min ((SInt16)0x8000)
#define kFskSInt16Max ((SInt16)0x7FFF)
#define kFskSInt32Min ((SInt32)0x80000000)
#define kFskSInt32Max ((SInt32)0x7FFFFFFF)

/*
	Determine the CPU
*/

#if !defined(TARGET_CPU_X86) && !defined(TARGET_CPU_PPC) && !defined(TARGET_CPU_ARM)
	#if defined(__i386__) || defined(i386) || defined(_M_IX86) || defined(_M_X64) || defined(__k8__) || defined(intel) || defined(__INTEL__) /* @@@ Intel is probably a bad switch @@@ */
		#define TARGET_CPU_X86	1
	#elif defined(powerpc) || defined(ppc) || defined(__ppc__) || defined(__POWERPC__) || defined(__ppc64__)
		#define TARGET_CPU_PPC	1
	#elif (defined(_ARM_) && _ARM_) || defined(arm) || defined(__arm__) || defined(__ARM__) || defined(__arm64__)
		#define TARGET_CPU_ARM	1
	#elif defined(mips) || defined(_mips)
		#define TARGET_CPU_MIPS	1
	#else /* TARGET_CPU_UNKNOWN */
		#error What is your CPU?
	#endif
#endif /* TARGET_CPU */
#ifndef TARGET_CPU_X86
	#define TARGET_CPU_X86	0
#endif /* TARGET_CPU_X86 */
#ifndef TARGET_CPU_PPC
	#define TARGET_CPU_PPC	0
#endif /* TARGET_CPU_PPC */
#ifndef TARGET_CPU_ARM
	#define TARGET_CPU_ARM	0
#endif /* TARGET_CPU_ARM */
#ifndef TARGET_CPU_MIPS
	#define TARGET_CPU_MIPS	0
#endif /* TARGET_CPU_MIPS */

/* special treatment for 64bit architectures */
#if TARGET_CPU_ARM64
#undef TARGET_CPU_ARM
#define TARGET_CPU_ARM	1
#endif
#if TARGET_CPU_X86_64
#undef TARGET_CPU_X86
#define TARGET_CPU_X86	1
#endif

/* Determine whether the CPU can copy misaligned multi-byte data correctly. */
#if !defined(FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE)
	#if TARGET_CPU_X86 || TARGET_CPU_PPC
		#define FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE	1
	#else /* TARGET_CPU_ARM, ... */
		#define FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE	0
	#endif /* TARGET_CPU */
#endif /* FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE */


/*
	Be sure we know the endian
*/

#if !defined(TARGET_RT_LITTLE_ENDIAN) && !defined(TARGET_RT_BIG_ENDIAN)
    #error what is your platforms endianness??
#endif /* TARGET_RT____ENDIAN */


#include "FskErrors.h"

/*
	import / export declarations
*/

#if TARGET_OS_WIN32 || (TARGET_OS_KPL && defined(_WIN32))
	#if defined (FSK_EXTENSION_EMBED)
		#define FskExport(return_type) return_type
	#else
		#define FskExport(return_type) extern __declspec( dllexport ) return_type
	#endif
	#define FskImport(return_type) extern __declspec( dllimport ) return_type
#elif TARGET_OS_MAC
	#define FskExport(return_type) __attribute__ ((visibility("default"))) return_type
	#define FskImport(return_type) __attribute__ ((visibility("default"))) return_type
#endif /* TARGET_OS */

#ifndef FskExport
	#define FskExport(return_type) return_type
#endif /* FskExport*/

#ifndef FskImport
	#define FskImport(return_type) extern return_type
#endif /* FskImport */

#if SUPPORT_DLLEXPORT
	#define FskAPI(return_type) FskExport(return_type)
#elif SUPPORT_DLLIMPORT
	#define FskAPI(return_type) FskImport(return_type)
#else
	#define FskAPI(return_type) return_type
#endif


/*
	Make it easy to declare anonymous types
*/

#define FskDeclarePrivateType(pt)			\
	typedef struct pt##Record pt##Record;	\
	typedef pt##Record *pt;

/*
	Alignment techniques
*/

	#if TARGET_OS_MAC || TARGET_OS_LINUX
		#define TARGET_ALIGN_PACK 1
		#define TARGET_ALIGN_PACKPUSH 0
	#elif TARGET_OS_WIN32
		#define TARGET_ALIGN_PACK 0
		#define TARGET_ALIGN_PACKPUSH 1
	#elif TARGET_OS_KPL
	#else /* TARGET_OS_UNKNOWN */
		#pragma error
	#endif

	#if TARGET_OS_LINUX
		#define FSK_PACK_STRUCT __attribute__((packed))
	#else /* !TARGET_OS_LINUX */
		#define FSK_PACK_STRUCT
	#endif


/*
	Cocoa for Mac OS X (@@jph can FSK_COCOA  be eliminated since TARGET_OS_MAC is always FSK_COCOA)
*/

#if TARGET_OS_MAC
	#define FSK_COCOA 			1
#endif


/*
	Some build defaults
*/

#ifndef SUPPORT_XS_DEBUG
	// if not specified, debugging is on
	#define SUPPORT_XS_DEBUG 1
#endif /* SUPPORT_XS_DEBUG */

#include "FskPlatform.h"
#include "FskInstrumentation.h"

#ifndef kFskBitmapFormatSourceDefaultRGBA
	#define kFskBitmapFormatSourceDefaultRGBA kFskBitmapFormatDefaultRGBA
#endif

#ifndef kFskDecompressorSlop
	// number of bytes needed beyond the end of compressed audio & video data to accomodate platform codec implementations
	#define kFskDecompressorSlop (4)
#endif

#if TARGET_OS_WIN32
	#define WINVER 0x0500	/* Windows 2000. This is needed for Developer Studio 9. */
#endif

/* BAIL macros */
#if SUPPORT_INSTRUMENTATION
    #define BAIL(code)							do { err = (code);		  FskInstrumentedTypePrintfForLevel(FskInstrumentationGetErrorInstrumentation(), kFskInstrumentationLevelNormal, "BAIL %s (%d) in function %s in file %s at line %d", FskInstrumentationGetErrorString((code)), (code), __FUNCTION__, __FILE__, __LINE__); goto bail; } while(0)
    #define BAIL_IF_ERR(err)					do { FskErr scratchErr = (err);  if ((scratchErr) != 0)	{ FskInstrumentedTypePrintfForLevel(FskInstrumentationGetErrorInstrumentation(), kFskInstrumentationLevelNormal, "BAIL_IF_ERROR %s (%d) in function %s in file %s at line %d", FskInstrumentationGetErrorString((scratchErr)), (scratchErr), __FUNCTION__, __FILE__, __LINE__); goto bail; } } while(0)
    #define BAIL_IF_NULL(q, err, code)			do { if ((q) == NULL)	{ FskInstrumentedTypePrintfForLevel(FskInstrumentationGetErrorInstrumentation(), kFskInstrumentationLevelDebug, "BAIL_IF_NULL %s (%d) in function %s in file %s at line %d", FskInstrumentationGetErrorString((code)), (code), __FUNCTION__, __FILE__, __LINE__); (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_ZERO(q, err, code)			do { if ((q) == 0)		{ FskInstrumentedTypePrintfForLevel(FskInstrumentationGetErrorInstrumentation(), kFskInstrumentationLevelDebug, "BAIL_IF_ZERO %s (%d) in function %s in file %s at line %d", FskInstrumentationGetErrorString((code)), (code), __FUNCTION__, __FILE__, __LINE__); (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_NONZERO(q, err, code)		do { if ((q) != 0)		{ FskInstrumentedTypePrintfForLevel(FskInstrumentationGetErrorInstrumentation(), kFskInstrumentationLevelDebug, "BAIL_IF_NONZERO %s (%d) in function %s in file %s at line %d", FskInstrumentationGetErrorString((code)), (code), __FUNCTION__, __FILE__, __LINE__); (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_NEGATIVE(q, err, code)		do { if ((q) <  0)		{ FskInstrumentedTypePrintfForLevel(FskInstrumentationGetErrorInstrumentation(), kFskInstrumentationLevelDebug, "BAIL_IF_NEGATIVE %s (%d) in function %s in file %s at line %d", FskInstrumentationGetErrorString((code)), (code), __FUNCTION__, __FILE__, __LINE__); (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_NONPOSITIVE(q, err, code)	do { if ((q) <= 0)		{ FskInstrumentedTypePrintfForLevel(FskInstrumentationGetErrorInstrumentation(), kFskInstrumentationLevelDebug, "BAIL_IF_NONPOSITIVE %s (%d) in function %s in file %s at line %d", FskInstrumentationGetErrorString((code)), (code), __FUNCTION__, __FILE__, __LINE__); (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_TRUE(q, err, code)			do { if (q)				{ FskInstrumentedTypePrintfForLevel(FskInstrumentationGetErrorInstrumentation(), kFskInstrumentationLevelDebug, "BAIL_IF_TRUE %s (%d) in function %s in file %s at line %d", FskInstrumentationGetErrorString((code)), (code), __FUNCTION__, __FILE__, __LINE__); (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_FALSE(q, err, code)			do { if (!(q))			{ FskInstrumentedTypePrintfForLevel(FskInstrumentationGetErrorInstrumentation(), kFskInstrumentationLevelDebug, "BAIL_IF_FALSE %s (%d) in function %s in file %s at line %d", FskInstrumentationGetErrorString((code)), (code), __FUNCTION__, __FILE__, __LINE__); (err) = (code); goto bail; } } while(0)
#else
    #define BAIL(code)                          do { err = (code);		  goto bail;                   } while(0)
    #define BAIL_IF_ERR(err)					do { if ((err) != 0)	{ goto bail;                 } } while(0)
    #define BAIL_IF_NULL(q, err, code)			do { if ((q) == NULL)	{ (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_ZERO(q, err, code)			do { if ((q) == 0)		{ (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_NONZERO(q, err, code)		do { if ((q) != 0)		{ (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_NEGATIVE(q, err, code)		do { if ((q) <  0)		{ (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_NONPOSITIVE(q, err, code)	do { if ((q) <= 0)		{ (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_TRUE(q, err, code)			do { if (q)				{ (err) = (code); goto bail; } } while(0)
    #define BAIL_IF_FALSE(q, err, code)			do { if (!(q))			{ (err) = (code); goto bail; } } while(0)
#endif

#define xsThrowIfFskErr(_ERROR)                             \
    do {                                                    \
        FskErr _err = _ERROR;                               \
        if (_err != kFskErrNone)                            \
            fxError(the, (char *)__FILE__, __LINE__, _err);         \
    } while(0)


#if SUPPORT_XS_DEBUG
    #define xsThrowDiagnosticIfFskErr(_ERROR, _MSG, ...)                       \
        do {                                                                \
            FskErr _err = _ERROR;                                           \
            if (_err != kFskErrNone) {                                      \
                xsErrorPrintf(FskThreadFormatDiagnostic(_MSG, __VA_ARGS__));  \
            }                                                               \
        } while(0)

    #define xsTraceDiagnostic(_MSG, ...) xsTrace(FskThreadFormatDiagnostic(_MSG, __VA_ARGS__))
#else
    #define xsThrowDiagnosticIfFskErr(_ERROR, _MSG, ...) xsThrowIfFskErr(_ERROR)
    #define xsTraceDiagnostic(_MSG, ...)
#endif

#ifdef mxDebug
    #define xsThrowIfNULL(_ASSERTION) \
        ((void)((_ASSERTION) || (fxError(the,(char *)__FILE__,__LINE__,kFskErrMemFull), 1)))
#else
    #define xsThrowIfNULL(_ASSERTION) \
        ((void)((_ASSERTION) || (fxError(the,NULL,0,kFskErrMemFull), 1)))
#endif

#define xsToStringCopy(slot) FskStrDoCopy(xsToString(slot))

/*
    noreturn
*/
#define FSK_FUNCTION_ANALYZER_NORETURN

#if defined(__clang__) || defined(__GNUC__)
    #define FSK_FUNCTION_NORETURN __attribute__((noreturn))
    #if defined(__clang__)
        #if __has_feature(attribute_analyzer_noreturn)
            #undef FSK_FUNCTION_ANALYZER_NORETURN
            #define FSK_FUNCTION_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
        #endif
    #endif
#else
    #define FSK_FUNCTION_NORETURN
#endif

#endif /* __FSK__ */
