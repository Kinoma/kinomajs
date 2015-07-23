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
#ifndef __XS__
#define __XS__

#ifndef __XSPLATFORM__

	#if __FSK_LAYER__
	
		#include "Fsk.h"

		#if SUPPORT_XS_DEBUG
			#define mxDebug 1
		#else
			#undef mxDebug
		#endif

		#if TARGET_RT_BIG_ENDIAN
			#define mxBigEndian 1
			#define mxLittleEndian 0
		#else
			#define mxBigEndian 0
			#define mxLittleEndian 1
		#endif

		#define mxLinux 0
		#define mxMacOSX 0
		#define mxiPhone 0
		#define mxWindows 0
		#define mxAndroid 0
		#define mxKpl 0
		#define mxFsk 1x

		#ifdef mxDebug
			#define mxProfile 1
		#endif

		#if TARGET_OS_WIN32
			#undef mxWindows
			#define mxWindows 1
		#elif TARGET_OS_MACOSX || TARGET_OS_MAC
			#if TARGET_OS_IPHONE
				#undef mxiPhone
				#define mxiPhone 1
			#else
				#undef mxMacOSX
				#define mxMacOSX 1
			#endif
		#elif TARGET_OS_ANDROID
			#undef mxAndroid
			#define mxAndroid 1
		#elif TARGET_OS_LINUX
			#undef mxLinux
			#define mxLinux 1
		#elif TARGET_OS_KPL
			#undef mxKpl
			#define mxKpl 1
		#endif

		#define mxExport extern    
		#define mxImport extern
		
		#define XS_FUNCTION_NORETURN FSK_FUNCTION_NORETURN
		#define XS_FUNCTION_ANALYZER_NORETURN FSK_FUNCTION_ANALYZER_NORETURN

		typedef SInt8 txS1;
		typedef UInt8 txU1;
		typedef SInt16 txS2;
		typedef UInt16 txU2;
		typedef SInt32 txS4;
		typedef UInt32 txU4;

	#else /* !__FSK_LAYER__ */

		#define mxBigEndian 0
		#define mxLittleEndian 0

		#define mxFsk 0
		#define mxiOS 0
		#define mxKpl 0
		#define mxLinux 0
		#define mxMacOSX 0
		#define mxWindows 0

		#if defined(_MSC_VER)
			#if defined(_M_IX86) || defined(_M_X64)
				#undef mxLittleEndian
				#define mxLittleEndian 1
				#undef mxWindows
				#define mxWindows 1
				#define mxExport extern __declspec( dllexport )
				#define mxImport extern __declspec( dllimport )
			#else 
				#error unknown Microsoft compiler
			#endif
		#elif defined(__GNUC__) 
			#if defined(__i386__) || defined(i386) || defined(intel) || defined(arm) || defined(__arm__) || defined(__k8__) || defined(__x86_64__)
				#undef mxLittleEndian
				#define mxLittleEndian 1
				#if defined(__linux__)
					#undef mxLinux
					#define mxLinux 1
					#define mxExport extern    
					#define mxImport extern
				#else
					#if defined(__APPLE__)
						#if defined(iphone)
							#undef mxiOS
							#define mxiOS 1
						#else
							#undef mxMacOSX
							#define mxMacOSX 1
						#endif
					#endif
					#define mxExport __attribute__ ((visibility("default")))
					#define mxImport __attribute__ ((visibility("default")))
				#endif
				#define XS_FUNCTION_NORETURN __attribute__((noreturn))
				#define XS_FUNCTION_ANALYZER_NORETURN
				#if defined(__clang__)
					#if __has_feature(attribute_analyzer_noreturn)
						#undef XS_FUNCTION_ANALYZER_NORETURN
						#define XS_FUNCTION_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
					#endif
				#endif
			#else 
				#error unknown GNU compiler
			#endif
		#else 
			#error unknown compiler
		#endif

		#ifndef XS_FUNCTION_NORETURN
			#define XS_FUNCTION_NORETURN
		#endif
		#ifndef XS_FUNCTION_ANALYZER_NORETURN
			#define XS_FUNCTION_ANALYZER_NORETURN
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
		
		#if mxWindows
			#undef _setjmp
			#define _setjmp setjmp
		#else
			#include <errno.h>
		#endif

	#endif /* !__FSK_LAYER__ */

#endif /* !__XSPLATFORM__ */

#include <setjmp.h>

#ifndef NULL
	#define NULL 0
#endif

#if __FSK_LAYER__
	#define fxPop() (the->scratch = *(the->stack++), the->scratch)
	#define fxPush(_SLOT) (the->scratch = (_SLOT), --the->stack, *(the->stack) = the->scratch)
#else
	#define fxPop() (*(the->stack++))
	#define fxPush(_SLOT) (*(--the->stack) = (_SLOT))
#endif

#ifdef mxDebug
#define xsOverflow(_COUNT) \
	(fxOverflow(the,_COUNT,(char *)__FILE__,__LINE__))
#else
#define xsOverflow(_COUNT) \
	(fxOverflow(the,_COUNT,NULL,0))
#endif

#ifndef __XS6ALL__
typedef struct xsCreationRecord xsCreation;
typedef struct xsJumpRecord xsJump;
typedef struct xsMachineRecord xsMachine;
typedef struct xsSlotRecord xsSlot;
typedef struct xsHostBuilderRecord xsHostBuilder;
#else
typedef struct sxCreation xsCreation;
typedef struct sxJump xsJump;
typedef struct sxMachine xsMachine;
typedef struct sxSlot xsSlot;
typedef struct sxHostFunctionBuilder xsHostBuilder;
#endif

/* Slot */

struct xsSlotRecord {
	void* data[4];
};

enum {
	xsUndefinedType,
	xsNullType,
	xsBooleanType,
	xsIntegerType,
	xsNumberType,
	xsStringType,
	xsStringXType,
	xsSymbolType,
	xsReferenceType
};
typedef char xsType;

#define xsTypeOf(_SLOT) \
	(the->scratch = (_SLOT), \
	fxTypeOf(the, &(the->scratch)))

/* Primitives */

typedef txS4 xsBooleanValue;
typedef txS4 xsIntegerValue;
typedef double xsNumberValue;
typedef char* xsStringValue;
typedef txU4 xsUnsignedValue;

#define xsUndefined \
	(fxUndefined(the, &the->scratch), \
	the->scratch)
#define xsNull \
	(fxNull(the, &the->scratch), \
	the->scratch)
#define xsFalse \
	(fxBoolean(the, &the->scratch, 0), \
	the->scratch)
#define xsTrue \
	(fxBoolean(the, &the->scratch, 1), \
	the->scratch)
	
#define xsBoolean(_VALUE) \
	(fxBoolean(the, &the->scratch, _VALUE), \
	the->scratch)
#define xsToBoolean(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToBoolean(the, &(the->scratch)))

#define xsInteger(_VALUE) \
	(fxInteger(the, &the->scratch, _VALUE), \
	the->scratch)
#define xsToInteger(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToInteger(the, &(the->scratch)))

#define xsNumber(_VALUE) \
	(fxNumber(the, &the->scratch, _VALUE), \
	the->scratch)
#define xsToNumber(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToNumber(the, &(the->scratch)))

#define xsString(_VALUE) \
	(fxString(the, &the->scratch, _VALUE), \
	the->scratch)
#define xsStringBuffer(_BUFFER,_SIZE) \
	(fxStringBuffer(the, &the->scratch, _BUFFER ,_SIZE), \
	the->scratch)
#define xsToString(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToString(the, &(the->scratch)))
#define xsToStringBuffer(_SLOT,_BUFFER,_SIZE) \
	(the->scratch = (_SLOT), \
	fxToStringBuffer(the, &(the->scratch), _BUFFER ,_SIZE))

#define xsArrayBuffer(_BUFFER,_SIZE) \
	(fxArrayBuffer(the, &the->scratch, _BUFFER, _SIZE), \
	the->scratch)
#define xsGetArrayBufferData(_SLOT,_OFFSET,_BUFFER,_SIZE) \
	(the->scratch = (_SLOT), \
	fxGetArrayBufferData(the, &(the->scratch), _OFFSET, _BUFFER, _SIZE))
#define xsGetArrayBufferLength(_SLOT) \
	(the->scratch = (_SLOT), \
	fxGetArrayBufferLength(the, &(the->scratch)))
#define xsSetArrayBufferData(_SLOT,_OFFSET,_BUFFER,_SIZE) \
	(the->scratch = (_SLOT), \
	fxSetArrayBufferData(the, &(the->scratch), _OFFSET, _BUFFER, _SIZE))
#define xsSetArrayBufferLength(_SLOT,_LENGTH) \
	(the->scratch = (_SLOT), \
	fxSetArrayBufferLength(the, &(the->scratch), _LENGTH))
#define xsToArrayBuffer(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToArrayBuffer(the, &(the->scratch)))

/* Instances and Prototypes */

#define prototypesStackIndex -2
#define xsObjectPrototype (the->stackTop[prototypesStackIndex - 1])
#define xsFunctionPrototype (the->stackTop[prototypesStackIndex - 2])
#define xsArrayPrototype (the->stackTop[prototypesStackIndex - 3])
#define xsStringPrototype (the->stackTop[prototypesStackIndex - 4])
#define xsBooleanPrototype (the->stackTop[prototypesStackIndex - 5])
#define xsNumberPrototype (the->stackTop[prototypesStackIndex - 6])
#define xsDatePrototype (the->stackTop[prototypesStackIndex - 7])
#define xsRegExpPrototype (the->stackTop[prototypesStackIndex - 8])
#define xsHostPrototype (the->stackTop[prototypesStackIndex - 9])
#define xsErrorPrototype (the->stackTop[prototypesStackIndex - 10])
#define xsEvalErrorPrototype (the->stackTop[prototypesStackIndex - 11])
#define xsRangeErrorPrototype (the->stackTop[prototypesStackIndex - 12])
#define xsReferenceErrorPrototype (the->stackTop[prototypesStackIndex - 13])
#define xsSyntaxErrorPrototype (the->stackTop[prototypesStackIndex - 14])
#define xsTypeErrorPrototype (the->stackTop[prototypesStackIndex - 15])
#define xsURIErrorPrototype (the->stackTop[prototypesStackIndex - 16])

#define xsNewInstanceOf(_PROTOTYPE) \
	(xsOverflow(-1), \
	fxPush(_PROTOTYPE), \
	fxNewInstanceOf(the), \
	fxPop())
#define xsIsInstanceOf(_SLOT,_PROTOTYPE) \
	(xsOverflow(-2), \
	fxPush(_PROTOTYPE), \
	fxPush(_SLOT), \
	fxIsInstanceOf(the))

/* Identifiers */

typedef unsigned char xsFlag;
typedef short xsIndex;
#define XS_NO_ID -1

#define xsID(_NAME) \
	fxID(the, _NAME)
#define xsFindID(_NAME) \
	fxFindID(the, _NAME)
#define xsIsID(_NAME) \
	fxIsID(the, _NAME)
#define xsName(_ID) \
	fxName(the, _ID)

/* Properties */

#define xsEnumerate(_THIS) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxEnumerate(the), \
	fxPop())
	
#define xsHas(_THIS,_ID) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxHasID(the, _ID))

#define xsHasOwn(_THIS,_ID) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxHasOwnID(the, _ID))

#define xsGet(_THIS,_ID) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxGetID(the, _ID), \
	fxPop())

#define xsGetClosure(_ID) \
	(xsOverflow(-1), \
	fxPush(xsFunction), \
	fxGetClosure(the, _ID), \
	fxPop())

#define xsGetAt(_THIS,_AT) \
	(xsOverflow(-2), \
	fxPush(_THIS), \
	fxPush(_AT), \
	fxGetAt(the), \
	fxPop())

#define xsSet(_THIS,_ID,_SLOT) \
	(xsOverflow(-2), \
	fxPush(_SLOT), \
	fxPush(_THIS), \
	fxSetID(the, _ID), \
	the->stack++)

#define xsSetClosure(_ID,_SLOT) \
	(xsOverflow(-2), \
	fxPush(_SLOT), \
	fxPush(xsFunction), \
	fxSetClosure(the, _ID), \
	the->stack++)

#define xsSetAt(_THIS,_AT,_SLOT) \
	(xsOverflow(-3), \
	fxPush(_SLOT), \
	fxPush(_THIS), \
	fxPush(_AT), \
	fxSetAt(the), \
	the->stack++)

#define xsDelete(_THIS,_ID) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxDeleteID(the, _ID), \
	the->stack++)

#define xsDeleteAt(_THIS,_AT) \
	(xsOverflow(-2), \
	fxPush(_THIS), \
	fxPush(_AT), \
	fxDeleteAt(the), \
	the->stack++)

#define xsCall0(_THIS,_ID) \
	(xsOverflow(-2), \
	fxPushCount(the, 0), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall0_noResult(_THIS,_ID) \
	(xsOverflow(-2), \
	fxPushCount(the, 0), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall1(_THIS,_ID,_SLOT0) \
	(xsOverflow(-3), \
	fxPush(_SLOT0), \
	fxPushCount(the, 1), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall1_noResult(_THIS,_ID,_SLOT0) \
	(xsOverflow(-3), \
	fxPush(_SLOT0), \
	fxPushCount(the, 1), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall2(_THIS,_ID,_SLOT0,_SLOT1) \
	(xsOverflow(-4), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPushCount(the, 2), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall2_noResult(_THIS,_ID,_SLOT0,_SLOT1) \
	(xsOverflow(-4), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPushCount(the, 2), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall3(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-5), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPushCount(the, 3), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall3_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-5), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPushCount(the, 3), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall4(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-6), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPushCount(the, 4), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall4_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-6), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPushCount(the, 4), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall5(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-7), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPushCount(the, 5), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall5_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-7), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPushCount(the, 5), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall6(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5) \
	(xsOverflow(-8), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPushCount(the, 6), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall6_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5) \
	(xsOverflow(-8), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPushCount(the, 6), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall7(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6) \
	(xsOverflow(-9), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPushCount(the, 7), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall7_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6) \
	(xsOverflow(-9), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPushCount(the, 7), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall8(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6,_SLOT7) \
	(xsOverflow(-10), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPush(_SLOT7), \
	fxPushCount(the, 8), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall8_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6,_SLOT7) \
	(xsOverflow(-10), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPush(_SLOT7), \
	fxPushCount(the, 8), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsNew0(_THIS,_ID) \
	(xsOverflow(-2), \
	fxPushCount(the, 0), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew1(_THIS,_ID,_SLOT0) \
	(xsOverflow(-3), \
	fxPush(_SLOT0), \
	fxPushCount(the, 1), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew2(_THIS,_ID,_SLOT0,_SLOT1) \
	(xsOverflow(-4), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPushCount(the, 2), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew3(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-5), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPushCount(the, 3), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew4(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-6), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPushCount(the, 4), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew5(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-7), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPushCount(the, 5), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew6(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5) \
	(xsOverflow(-8), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPushCount(the, 6), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew7(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6) \
	(xsOverflow(-9), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPushCount(the, 7), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew8(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6,_SLOT7) \
	(xsOverflow(-10), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPush(_SLOT7), \
	fxPushCount(the, 8), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())
	
#define xsTest(_SLOT) \
	(xsOverflow(-1), \
	fxPush(_SLOT), \
	fxRunTest(the))
	
/* Globals */

#define xsGlobal (the->stackTop[-1])

/* Host Constructors, Functions and Objects */

typedef void (*xsCallback)(xsMachine*);

struct xsHostBuilderRecord {
	xsCallback callback;
	xsIntegerValue length;
	xsIndex id; 
};
	
#define xsNewHostConstructor(_CALLBACK,_LENGTH,_PROTOTYPE) \
	(xsOverflow(-1), \
	fxPush(_PROTOTYPE), \
	fxNewHostConstructor(the, _CALLBACK, _LENGTH, xsNoID), \
	fxPop())
	
#define xsNewHostConstructorObject(_CALLBACK,_LENGTH,_PROTOTYPE, _NAME) \
	(xsOverflow(-1), \
	fxPush(_PROTOTYPE), \
	fxNewHostConstructor(the, _CALLBACK, _LENGTH, _NAME), \
	fxPop())
	
#define xsNewHostFunction(_CALLBACK,_LENGTH) \
	(fxNewHostFunction(the, _CALLBACK, _LENGTH, xsNoID), \
	fxPop())
	
#define xsNewHostFunctionObject(_CALLBACK,_LENGTH, _NAME) \
	(fxNewHostFunction(the, _CALLBACK, _LENGTH, _NAME), \
	fxPop())

typedef void (*xsDestructor)(void*);

#define xsNewHostObject(_DESTRUCTOR) \
	(fxNewHostObject(the, _DESTRUCTOR), \
	fxPop())

#define xsGetHostData(_SLOT) \
	(the->scratch = (_SLOT), \
	fxGetHostData(the, &(the->scratch)))
#define xsSetHostData(_SLOT,_DATA) \
	(the->scratch = (_SLOT), \
	fxSetHostData(the, &(the->scratch), _DATA))
	
#define xsGetHostDestructor(_SLOT) \
	(the->scratch = (_SLOT), \
	fxGetHostDestructor(the, &(the->scratch)))
#define xsSetHostDestructor(_SLOT,_DESTRUCTOR) \
	(the->scratch = (_SLOT), \
	fxSetHostDestructor(the, &(the->scratch), _DESTRUCTOR))
	
typedef void (*xsMarkRoot)(xsMachine*, xsSlot*);
typedef void (*xsMarker)(void*, xsMarkRoot);
typedef struct {
	xsDestructor destructor;
	xsMarker marker;
} xsHostHooks;
	
#define xsGetHostHooks(_SLOT) \
	(the->scratch = (_SLOT), \
	fxGetHostHooks(the, &(the->scratch)))
#define xsSetHostHooks(_SLOT,_HOOKS) \
	(the->scratch = (_SLOT), \
	fxSetHostHooks(the, &(the->scratch), _HOOKS))

#define xsBuildHosts(_COUNT, _BUILDERS) \
	(fxBuildHosts(the, _COUNT, _BUILDERS), \
	fxPop())

/* Arguments and Variables */

#define xsVars(_COUNT) fxVars(the, _COUNT)

#define xsArg(_INDEX) (the->frame[6 + fxCheckArg(the, _INDEX)])
#define xsArgc (the->frame[5])
#define xsThis (the->frame[4])
#define xsFunction (the->frame[3])
#define xsTarget (the->frame[2])
#define xsResult (the->frame[1])
#define xsVarc (the->frame[-1])
#define xsVar(_INDEX) (the->frame[-2 - fxCheckVar(the, _INDEX)])
	
/* Garbage Collector */

#define xsCollectGarbage() \
	fxCollectGarbage(the)
#define xsEnableGarbageCollection(_ENABLE) \
	fxEnableGarbageCollection(the, _ENABLE)
#define xsRemember(_SLOT) \
	fxRemember(the, &(_SLOT))
#define xsForget(_SLOT) \
	fxForget(the, &(_SLOT))
#define xsAccess(_SLOT) \
	(fxAccess(the, &(_SLOT)), the->scratch)

/* Exceptions */

struct xsJumpRecord {
	jmp_buf buffer;
	xsJump* nextJump;
	xsSlot* stack;
	xsSlot* scope;
	xsSlot* frame;
	xsSlot* environment;
	xsIndex* code;
	xsBooleanValue flag;
};

#define xsException (the->stackTop[-2])

#ifdef mxDebug
#define xsThrow(_SLOT) \
	(the->stackTop[-2] = (_SLOT), \
	fxThrow(the,(char *)__FILE__,__LINE__))
#else
#define xsThrow(_SLOT) \
	(the->stackTop[-2] = (_SLOT), \
	fxThrow(the,NULL,0))
#endif

#define xsTry \
	xsJump __JUMP__; \
	__JUMP__.nextJump = the->firstJump; \
	__JUMP__.stack = the->stack; \
	__JUMP__.scope = the->scope; \
	__JUMP__.frame = the->frame; \
	__JUMP__.environment = NULL; \
	__JUMP__.code = the->code; \
	__JUMP__.flag = 0; \
	the->firstJump = &__JUMP__; \
	if (_setjmp(__JUMP__.buffer) == 0) {

#define xsCatch \
		the->firstJump = __JUMP__.nextJump; \
	} \
	else for ( \
		the->stack = __JUMP__.stack, \
		the->scope = __JUMP__.scope, \
		the->frame = __JUMP__.frame, \
		the->code = __JUMP__.code, \
		the->firstJump = __JUMP__.nextJump; \
		(__JUMP__.stack); \
		__JUMP__.stack = NULL)

/* Errors */

#ifndef __XS6ALL__
	enum {
		XS_NO_ERROR = 0,
		XS_UNKNOWN_ERROR,
		XS_EVAL_ERROR,
		XS_RANGE_ERROR,
		XS_REFERENCE_ERROR,
		XS_SYNTAX_ERROR,
		XS_TYPE_ERROR,
		XS_URI_ERROR,
		XS_ERROR_COUNT
	};
#endif

#ifdef mxDebug
	#define xsUnknownError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_UNKNOWN_ERROR, __VA_ARGS__)
	#define xsEvalError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_EVAL_ERROR, __VA_ARGS__)
	#define xsRangeError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_RANGE_ERROR, __VA_ARGS__)
	#define xsReferenceError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_REFERENCE_ERROR, __VA_ARGS__)
	#define xsSyntaxError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_SYNTAX_ERROR, __VA_ARGS__)
	#define xsTypeError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_TYPE_ERROR, __VA_ARGS__)
	#define xsURIError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_URI_ERROR, __VA_ARGS__)
#else
	#define xsUnknownError(...) fxThrowMessage(the, NULL, 0, XS_UNKNOWN_ERROR, __VA_ARGS__)
	#define xsEvalError(...) fxThrowMessage(the, NULL, 0, XS_EVAL_ERROR, __VA_ARGS__)
	#define xsRangeError(...) fxThrowMessage(the, NULL, 0, XS_RANGE_ERROR, __VA_ARGS__)
	#define xsReferenceError(...) fxThrowMessage(the, NULL, 0, XS_REFERENCE_ERROR, __VA_ARGS__)
	#define xsSyntaxError(...) fxThrowMessage(the, NULL, 0, XS_SYNTAX_ERROR, __VA_ARGS__)
	#define xsTypeError(...) fxThrowMessage(the, NULL, 0, XS_TYPE_ERROR, __VA_ARGS__)
	#define xsURIError(...) fxThrowMessage(the, NULL, 0, XS_URI_ERROR, __VA_ARGS__)
#endif

/* Platform */

#ifdef mxDebug
	#define xsAssert(it)\
		if (!(it)) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_UNKNOWN_ERROR, "%s", #it)
	#define xsError(_CODE) \
		fxError(the, (char *)__FILE__, __LINE__,_CODE)
	#define xsErrorPrintf(_MESSAGE) \
		fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_UNKNOWN_ERROR, "%s", _MESSAGE)
#else
	#define xsAssert(it)\
		if (!(it)) fxThrowMessage(the, NULL, 0, XS_UNKNOWN_ERROR, "%s", #it)
	#define xsError(_CODE) \
		fxError(the, NULL, 0, _CODE)
	#define xsErrorPrintf(_MESSAGE) \
		fxThrowMessage(the, NULL, 0, XS_UNKNOWN_ERROR, "%s", _MESSAGE)
#endif

#if !__FSK_LAYER__
	#ifdef mxDebug
		#define xsIfError(_ERROR) \
			((void)((!(the->scratch.data[0] = (void*)(_ERROR))) || (fxError(the,(char *)__FILE__,__LINE__,(xsIntegerValue)(the->scratch.data[0])), 1)))
	#else
		#define xsIfError(_ERROR) \
			((void)((!(the->scratch.data[0] = (void*)(_ERROR))) || (fxError(the,NULL,0,(xsIntegerValue)(the->scratch.data[0])), 1)))
	#endif
	#if mxWindows
		#ifdef mxDebug
			#define xsElseError(_ASSERTION) \
				((void)((_ASSERTION) || (fxError(the,(char *)__FILE__,__LINE__,GetLastError()), 1)))
		#else
			#define xsElseError(_ASSERTION) \
				((void)((_ASSERTION) || (fxError(the,NULL,0,GetLastError()), 1)))
		#endif
	#else
		#ifdef mxDebug
			#define xsElseError(_ASSERTION) \
				((void)((_ASSERTION) || (fxError(the,(char *)__FILE__,__LINE__,errno), 1)))
		#else
			#define xsElseError(_ASSERTION) \
				((void)((_ASSERTION) || (fxError(the,NULL,0,errno), 1)))
		#endif
	#endif
	#define xsThrowIfNULL(_ASSERTION) xsElseError(_ASSERTION)
#endif /* !__FSK_LAYER__ */

/* Debugger */
	
#ifdef mxDebug
	#define xsDebugger() \
		fxDebugger(the,(char *)__FILE__,__LINE__)
#else
	#define xsDebugger() \
		fxDebugger(the,NULL,0)
#endif

#define xsTrace(_STRING) \
	fxReport(the, "%s", _STRING)
#define xsLog(...) \
	fxReport(the, __VA_ARGS__)

typedef xsCallback (*xsCallbackAt)(xsIndex);

/* Machine */

struct xsMachineRecord {
	xsSlot* stack;
	xsSlot* scope;
	xsSlot* frame;
	xsIndex* code;
	xsSlot* stackBottom;
	xsSlot* stackTop;
	xsJump* firstJump;
	void* context;
	xsSlot scratch;
#if __FSK_LAYER__
	FskInstrumentedItemDeclaration	/* xs.h */
#endif
};

struct xsCreationRecord {
	xsIntegerValue initialChunkSize;
	xsIntegerValue incrementalChunkSize;
	xsIntegerValue initialHeapCount;
	xsIntegerValue incrementalHeapCount;
	xsIntegerValue stackCount;
	xsIntegerValue keyCount;
	xsIntegerValue nameModulo;
	xsIntegerValue symbolModulo;
};

#define xsCreateMachine(_CREATION,_ARCHIVE,_NAME,_CONTEXT) \
	fxCreateMachine(_CREATION, _ARCHIVE, _NAME, _CONTEXT)
	
#define xsDeleteMachine(_THE) \
	fxDeleteMachine(_THE)
	
#define xsCloneMachine(_ALLOCATION,_MACHINE,_NAME,_CONTEXT) \
	fxCloneMachine(_ALLOCATION, _MACHINE, _NAME, _CONTEXT)
	
#define xsShareMachine(_THE) \
	fxShareMachine(_THE)

/* Context */	
	
#define xsGetContext(_THE) \
	((_THE)->context)
	
#define xsSetContext(_THE,_CONTEXT) \
	((_THE)->context = (_CONTEXT))

/* Host */

#define xsBeginHost(_THE) \
	do { \
		xsMachine* __HOST_THE__ = _THE; \
		xsJump __HOST_JUMP__; \
		__HOST_JUMP__.nextJump = (__HOST_THE__)->firstJump; \
		__HOST_JUMP__.stack = (__HOST_THE__)->stack; \
		__HOST_JUMP__.scope = (__HOST_THE__)->scope; \
		__HOST_JUMP__.frame = (__HOST_THE__)->frame; \
		__HOST_JUMP__.environment = NULL; \
		__HOST_JUMP__.code = (__HOST_THE__)->code; \
		__HOST_JUMP__.flag = 0; \
		(__HOST_THE__)->firstJump = &__HOST_JUMP__; \
		if (_setjmp(__HOST_JUMP__.buffer) == 0) { \
			xsMachine* the = fxBeginHost(__HOST_THE__)
			
#define xsEndHost(_THE) \
			fxEndHost(__HOST_THE__); \
			the = NULL; \
		} \
		(__HOST_THE__)->stack = __HOST_JUMP__.stack, \
		(__HOST_THE__)->scope = __HOST_JUMP__.scope, \
		(__HOST_THE__)->frame = __HOST_JUMP__.frame, \
		(__HOST_THE__)->code = __HOST_JUMP__.code, \
		(__HOST_THE__)->firstJump = __HOST_JUMP__.nextJump; \
		break; \
	} while(1)

enum {	
	xsNoID = -1,
	xsDefault = 0,
	xsDontDelete = 2,
	xsDontEnum = 4,
	xsDontSet = 8,
	xsStatic = 16,
	xsIsGetter = 32,
	xsIsSetter = 64,
	xsChangeAll = 30
};
typedef unsigned char xsAttribute;

#define xsNewHostProperty(_THIS,_ID,_SLOT,_ATTRIBUTES,_MASK) \
	(xsOverflow(-2), \
	fxPush(_SLOT), \
	fxPush(_THIS), \
	fxPutID(the, _ID, _ATTRIBUTES, _MASK), \
	the->stack++)
	
#define xsArrayCacheBegin(_ARRAY) \
	(fxPush(_ARRAY), \
	fxArrayCacheBegin(the, the->stack), \
	the->stack++)
#define xsArrayCacheEnd(_ARRAY) \
	(fxPush(_ARRAY), \
	fxArrayCacheEnd(the, the->stack), \
	the->stack++)
#define xsArrayCacheItem(_ARRAY,_ITEM) \
	(fxPush(_ARRAY), \
	fxPush(_ITEM), \
	fxArrayCacheItem(the, the->stack + 1, the->stack), \
	the->stack += 2)
#define xsIDs(_COUNT,_NAMES) fxIDs(the, _COUNT, _NAMES)

#define xsModulePaths() \
	(fxModulePaths(the), \
	fxPop())

#define xsDemarshall(_DATA) \
	(fxDemarshall(the, (_DATA)), \
	fxPop())
#define xsMarshall(_SLOT) \
	(xsOverflow(-1), \
	fxPush(_SLOT), \
	fxMarshall(the))

#define xsIsProfiling() \
	fxIsProfiling(the)
#define xsStartProfiling() \
	fxStartProfiling(the)
#define xsStopProfiling() \
	fxStopProfiling(the)

#ifndef __XS6ALL__

#ifdef __cplusplus
extern "C" {
#endif

mxImport xsType fxTypeOf(xsMachine*, xsSlot*);

mxImport void fxPushCount(xsMachine*, xsIntegerValue);
mxImport void fxUndefined(xsMachine*, xsSlot*);
mxImport void fxNull(xsMachine*, xsSlot*);
mxImport void fxBoolean(xsMachine*, xsSlot*, xsBooleanValue);
mxImport xsBooleanValue fxToBoolean(xsMachine*, xsSlot*);
mxImport void fxInteger(xsMachine*, xsSlot*, xsIntegerValue);
mxImport xsIntegerValue fxToInteger(xsMachine*, xsSlot*);
mxImport void fxNumber(xsMachine*, xsSlot*, xsNumberValue);
mxImport xsNumberValue fxToNumber(xsMachine*, xsSlot*);
mxImport void fxString(xsMachine*, xsSlot*, xsStringValue);
mxImport void fxStringBuffer(xsMachine*, xsSlot*, xsStringValue, xsIntegerValue);
mxImport xsStringValue fxToString(xsMachine*, xsSlot*);
mxImport xsStringValue fxToStringBuffer(xsMachine*, xsSlot*, xsStringValue, xsIntegerValue);
mxImport void fxArrayBuffer(xsMachine*, xsSlot*, void*, xsIntegerValue);
mxImport void fxGetArrayBufferData(xsMachine*, xsSlot*, xsIntegerValue, void*, xsIntegerValue);
mxImport xsIntegerValue fxGetArrayBufferLength(xsMachine*, xsSlot*);
mxImport void fxSetArrayBufferData(xsMachine*, xsSlot*, xsIntegerValue, void*, xsIntegerValue);
mxImport void fxSetArrayBufferLength(xsMachine*, xsSlot*, xsIntegerValue);
mxImport void* fxToArrayBuffer(xsMachine*, xsSlot*);

mxImport void fxNewInstanceOf(xsMachine*);
mxImport xsBooleanValue fxIsInstanceOf(xsMachine*);
mxImport void fxArrayCacheBegin(xsMachine*, xsSlot*);
mxImport void fxArrayCacheEnd(xsMachine*, xsSlot*);
mxImport void fxArrayCacheItem(xsMachine*, xsSlot*, xsSlot*);

mxImport void fxBuildHosts(xsMachine*, xsIntegerValue, xsHostBuilder*);
mxImport void fxNewHostConstructor(xsMachine*, xsCallback, xsIntegerValue, xsIntegerValue);
mxImport void fxNewHostFunction(xsMachine*, xsCallback, xsIntegerValue, xsIntegerValue);
mxImport void fxNewHostObject(xsMachine*, xsDestructor);
mxImport void* fxGetHostData(xsMachine*, xsSlot*);
mxImport void fxSetHostData(xsMachine*, xsSlot*, void*);
mxImport xsDestructor fxGetHostDestructor(xsMachine*, xsSlot*);
mxImport void fxSetHostDestructor(xsMachine*, xsSlot*, xsDestructor);
mxImport xsHostHooks* fxGetHostHooks(xsMachine*, xsSlot*);
mxImport void fxSetHostHooks(xsMachine*, xsSlot*, xsHostHooks*);

mxImport void fxIDs(xsMachine*, xsIntegerValue, char**);
mxImport xsIndex fxID(xsMachine*, char*);
mxImport xsIndex fxFindID(xsMachine*, char*);
mxImport xsBooleanValue fxIsID(xsMachine*, char*);
mxImport char* fxName(xsMachine*, xsIndex);

mxImport void fxEnumerate(xsMachine* the);
mxImport xsBooleanValue fxHasID(xsMachine*, xsIntegerValue);
mxImport xsBooleanValue fxHasOwnID(xsMachine*, xsIntegerValue);
mxImport void fxGet(xsMachine*, xsSlot*, xsIntegerValue);
mxImport void fxGetAt(xsMachine*);
mxImport void fxGetClosure(xsMachine*, xsIntegerValue);
mxImport void fxGetID(xsMachine*, xsIntegerValue);
mxImport void fxSet(xsMachine*, xsSlot*, xsIntegerValue);
mxImport void fxSetAt(xsMachine*);
mxImport void fxSetClosure(xsMachine*, xsIntegerValue);
mxImport void fxSetID(xsMachine*, xsIntegerValue);
mxImport void fxDeleteAt(xsMachine*);
mxImport void fxDeleteID(xsMachine*, xsIntegerValue);
mxImport void fxCall(xsMachine*);
mxImport void fxCallID(xsMachine*, xsIntegerValue);
mxImport void fxNew(xsMachine*);
mxImport void fxNewID(xsMachine*, xsIntegerValue);
mxImport xsBooleanValue fxRunTest(xsMachine* the);

mxImport void fxVars(xsMachine*, xsIntegerValue);
mxImport xsIntegerValue fxCheckArg(xsMachine*, xsIntegerValue);
mxImport xsIntegerValue fxCheckVar(xsMachine*, xsIntegerValue);
mxImport void fxOverflow(xsMachine*, xsIntegerValue, xsStringValue, xsIntegerValue);

mxImport void fxThrow(xsMachine*, xsStringValue, xsIntegerValue) XS_FUNCTION_ANALYZER_NORETURN;
mxImport void fxThrowMessage(xsMachine* the, xsStringValue thePath, xsIntegerValue theLine, xsIntegerValue theError, xsStringValue theMessage, ...) XS_FUNCTION_ANALYZER_NORETURN;

mxImport void fxError(xsMachine*, xsStringValue, xsIntegerValue, xsIntegerValue) XS_FUNCTION_ANALYZER_NORETURN;

mxImport void fxDebugger(xsMachine*, xsStringValue, xsIntegerValue);
mxImport void fxReport(xsMachine*, xsStringValue, ...);

mxImport xsMachine* fxCreateMachine(xsCreation*, void*, xsStringValue, void*);
mxImport void fxDeleteMachine(xsMachine*);
mxImport xsMachine* fxCloneMachine(xsCreation*, xsMachine*, xsStringValue, void*);
mxImport void fxShareMachine(xsMachine*);

mxImport xsBooleanValue fxBuildHost(xsMachine*, xsCallback);
mxImport xsMachine* fxBeginHost(xsMachine*);
mxImport void fxEndHost(xsMachine*);
mxImport void fxPutID(xsMachine*, xsIntegerValue, xsAttribute, xsAttribute);

mxImport void fxCollectGarbage(xsMachine*);
mxImport void fxEnableGarbageCollection(xsMachine* the, xsBooleanValue enableIt);
mxImport void fxRemember(xsMachine*, xsSlot*);
mxImport void fxForget(xsMachine*, xsSlot*);
mxImport void fxAccess(xsMachine*, xsSlot*);

mxImport void fxDecodeURI(xsMachine*, xsStringValue);
mxImport void fxEncodeURI(xsMachine*, xsStringValue);
mxImport xsIntegerValue fxUnicodeLength(xsStringValue theString);
mxImport xsIntegerValue fxUnicodeToUTF8Offset(xsStringValue theString, xsIntegerValue theOffset);
mxImport xsIntegerValue fxUTF8ToUnicodeOffset(xsStringValue theString, xsIntegerValue theOffset);

mxImport void fxCopyObject(xsMachine*);
mxImport void fxDemarshall(xsMachine*, void*);
mxImport void* fxMarshall(xsMachine*);
mxImport void fxModulePaths(xsMachine*);

mxImport xsBooleanValue fxIsProfiling(xsMachine*);
mxImport void fxStartProfiling(xsMachine*);
mxImport void fxStopProfiling(xsMachine*);
	
mxImport void* fxMapArchive(xsStringValue, xsCallbackAt);
mxImport void fxUnmapArchive(void*);
mxImport void fxRunProgram(xsMachine*, xsStringValue);

#ifdef __cplusplus
}
#endif

#endif /* !__XS6ALL__ */

#if __FSK_LAYER__

typedef int (*xsGetter)(void*);
typedef int (*xsPutter)(xsStringValue, void*);

typedef struct {
	xsIntegerValue initialChunkSize;
	xsIntegerValue incrementalChunkSize;
	xsIntegerValue initialHeapCount;
	xsIntegerValue incrementalHeapCount;
	xsIntegerValue stackCount;
	xsIntegerValue symbolCount;
	xsIntegerValue symbolModulo;
} xsAllocation;

typedef struct {
	xsCallback callback;
	xsStringValue symbols;
	xsIntegerValue symbolsSize;
	xsStringValue code;
	xsIntegerValue codeSize;
	xsStringValue name;
} xsGrammar;

enum {
	xsSourceFlag = 1,
	xsNoErrorFlag = 2,
	xsNoWarningFlag = 4,
	xsSniffFlag = 8,
	xsSandboxFlag = 32,
	xsNoMixtureFlag = 64,
	xsDebugFlag = 128
};

#define xsAliasMachine(_ALLOCATION,_MACHINE,_NAME,_CONTEXT) \
	fxAliasMachine(_ALLOCATION, _MACHINE, _NAME, _CONTEXT)

#define xsChunkPrototype \
	(--the->stack, \
	*(the->stack) = xsGlobal, \
	fxGetID(the, xsID("Chunk")), \
	fxGetID(the, xsID("prototype")), \
	*(the->stack++))

#define xsExecute(_THE,_STREAM,_GETTER,_PATH,_LINE) \
	(fxExecute(_THE, _STREAM, _GETTER, _PATH, _LINE))

#define xsLink(_GRAMMAR) \
	(fxLink(the, _GRAMMAR), \
	fxPop())

#define xsModuleURL \
	(fxModuleURL(the), \
	fxPop())

#define xsNewFunction(_ARGUMENTS,_ARGUMENTS_SIZE,_BODY,_BODY_SIZE,_FLAG,_PATH,_LINE) \
	(fxNewFunction(the,_ARGUMENTS,_ARGUMENTS_SIZE,_BODY,_BODY_SIZE,_FLAG,_PATH,_LINE), fxPop())
	
#define xsNewMachine(_ALLOCATION,_GRAMMAR,_CONTEXT) \
	(fxNewMachine(_ALLOCATION, _GRAMMAR, _CONTEXT))
	

#define xsBeginHostSandboxCode(_THE,_CODE) \
	do { \
		xsMachine* __HOST_THE__ = _THE; \
		xsJump __HOST_JUMP__; \
		__HOST_JUMP__.nextJump = (__HOST_THE__)->firstJump; \
		__HOST_JUMP__.stack = (__HOST_THE__)->stack; \
		__HOST_JUMP__.scope = (__HOST_THE__)->scope; \
		__HOST_JUMP__.frame = (__HOST_THE__)->frame; \
		__HOST_JUMP__.environment = NULL; \
		__HOST_JUMP__.code = (__HOST_THE__)->code; \
		__HOST_JUMP__.flag = 0; \
		(__HOST_THE__)->firstJump = &__HOST_JUMP__; \
		if (_setjmp(__HOST_JUMP__.buffer) == 0) { \
			xsMachine* the = fxBeginHost(__HOST_THE__); \
			the->code = _CODE
			
#define xsDontScript 0
			
#define xsEndHostSandboxCode() \
			fxEndHost(__HOST_THE__); \
			the = NULL; \
		} \
		(__HOST_THE__)->stack = __HOST_JUMP__.stack, \
		(__HOST_THE__)->scope = __HOST_JUMP__.scope, \
		(__HOST_THE__)->frame = __HOST_JUMP__.frame, \
		(__HOST_THE__)->code = __HOST_JUMP__.code, \
		(__HOST_THE__)->firstJump = __HOST_JUMP__.nextJump; \
		break; \
	} while(1)

#define xsEnterSandbox() \
	fxEnterSandbox(the)

#define xsLeaveSandbox() \
	fxLeaveSandbox(the)

#define xsSandbox(_THIS) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxSandbox(the), \
	fxPop())

#define xsScript() \
	fxScript(the)

#define xsParse(_STREAM, _GETTER, _PATH, _LINE, _FLAG) \
	(xsOverflow(-4), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 3), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse0(_STREAM, _GETTER, _PATH, _LINE, _FLAG) \
	(xsOverflow(-4), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 3), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse1(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0) \
	(xsOverflow(-5), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPush(_SLOT0), \
	fxPushCount(the, 4), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse2(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1) \
	(xsOverflow(-6), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPushCount(the, 5), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse3(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-7), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPushCount(the, 6), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse4(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-8), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPushCount(the, 7), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse5(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-9), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPushCount(the, 8), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse6(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5) \
	(xsOverflow(-10), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPushCount(the, 9), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse7(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6) \
	(xsOverflow(-11), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPushCount(the, 10), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse8(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6,_SLOT7) \
	(xsOverflow(-12), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPushCount(the, 0), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPush(_SLOT7), \
	fxPushCount(the, 11), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsSerialize(_THIS,_STREAM,_PUTTER) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxSerialize(the, _STREAM, _PUTTER), \
	fxPop())

#ifndef __XS6ALL__

#ifdef __cplusplus
extern "C" {
#endif

mxImport xsMachine* fxAliasMachine(xsAllocation*, xsMachine*, xsStringValue, void*);
mxImport xsBooleanValue fxExecute(xsMachine*, void*, xsGetter, xsStringValue, xsIntegerValue);
mxImport void fxLink(xsMachine*, xsGrammar*);
mxImport void fxNewFunction(xsMachine*, xsStringValue, xsIntegerValue, xsStringValue, xsIntegerValue, xsFlag, xsStringValue, xsIntegerValue);
mxImport xsMachine* fxNewMachine(xsAllocation*, xsGrammar*, void*);

mxImport void fxEnterSandbox(xsMachine*);
mxImport void fxLeaveSandbox(xsMachine*);
mxImport void fxSandbox(xsMachine*);
mxImport xsIntegerValue fxScript(xsMachine*);

mxImport void fxParse(xsMachine*, void*, xsGetter, xsStringValue, xsIntegerValue, xsFlag);
mxImport void fxSerialize(xsMachine*, void*, xsPutter);

mxImport void fxRunForIn(xsMachine* the);

mxImport void fxModuleURL(xsMachine*);

#ifdef __cplusplus
}
#endif

#endif /* !__XS6ALL__ */

/* js2c
mxImport void fxHostScope(xsMachine*, xsIndex);
mxImport void fxNewHostClosure(xsMachine*, xsBooleanValue);
mxImport void fxRunAdd(xsMachine*);
mxImport void fxRunArguments(xsMachine*);
mxImport xsBooleanValue fxRunCompare(xsMachine*, xsBooleanValue, xsBooleanValue, xsBooleanValue, xsBooleanValue);
mxImport void fxRunDelta(xsMachine* the, xsIntegerValue theDelta);
mxImport xsBooleanValue fxRunEqual(xsMachine*, xsBooleanValue, xsBooleanValue, xsBooleanValue);
mxImport xsBooleanValue fxRunIn(xsMachine* the, xsBooleanValue testing);
mxImport xsBooleanValue fxRunInstanceof(xsMachine* the, xsBooleanValue testing);
mxImport void fxRunSign(xsMachine* the, xsIntegerValue theSign);
mxImport xsBooleanValue fxRunStrictEqual(xsMachine* the, xsBooleanValue testing, xsBooleanValue yes, xsBooleanValue no);
mxImport void fxRunSubtract(xsMachine* the);
mxImport void fxRunTypeof(xsMachine* the);
*/

#else /* !__FSK_LAYER__ */

#ifndef __XS6ALL__

mxImport void fxRunLoop(xsMachine*);
mxImport void fxRunModule(xsMachine*, xsStringValue);

#endif /* !__XS6ALL__ */

#endif /* !__FSK_LAYER__ */

#endif /* __XS__ */

