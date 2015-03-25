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

#define mxBigEndian 0
#define mxLittleEndian 0

#define mxKpl 0
#define mxLinux 0
#define mxMacOSX 0
#define mxSolaris 0
#define mxWindows 0

#if defined(KPL)
	#undef mxKpl
	#define mxKpl 1
	#include "xs_kpl.h"
#elif defined(_MSC_VER)
	#if defined(_M_IX86) || defined(_M_X64)
		#undef mxLittleEndian
		#define mxLittleEndian 1
		#undef mxWindows
		#define mxWindows 1
	#else 
		#error unknown Microsoft compiler
	#endif
#elif defined(__GNUC__) 
	#if defined(__i386__) || defined(i386) || defined(intel) || defined(__k8__) || defined(__x86_64__)
		#undef mxLittleEndian
		#define mxLittleEndian 1
		#if defined(__linux__)
			#undef mxLinux
			#define mxLinux 1
		#else
			#undef mxMacOSX
			#define mxMacOSX 1
		#endif
        #define XS_FUNCTION_NORETURN __attribute__((noreturn))
        #define XS_FUNCTION_ANALYZER_NORETURN
        #if defined(__clang__)
            #if __has_feature(attribute_analyzer_noreturn)
                #undef XS_FUNCTION_ANALYZER_NORETURN
                #define XS_FUNCTION_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
            #endif
        #endif
	#elif defined(__ppc__) || defined(powerpc) || defined(ppc)
		#undef mxBigEndian
		#define mxBigEndian 1
		#if defined(__linux__)
			#undef mxLinux
			#define mxLinux 1
		#else  
			#undef mxMacOSX
			#define mxMacOSX 1
		#endif
	#elif defined(sparc)
		#undef mxBigEndian
		#define mxBigEndian 1
		#undef mxSolaris
		#define mxSolaris 1
	#elif defined(arm) || defined(__arm__) || defined(__arm64__)
		#undef mxLittleEndian
		#define mxLittleEndian 1
		#undef mxLinux
		#define mxLinux 1
	#elif defined(mips) || defined(_mips)
		#undef mxLittleEndian
		#define mxLittleEndian 1
		#undef mxLinux
		#define mxLinux 1
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

#endif

#include <setjmp.h>

#ifndef NULL
	#define NULL 0
#endif

#ifndef mxExport
	#if mxWindows
		#if defined(mxXSC) || defined (__FSK_LAYER__)
			#define mxExport extern
		#else
			#define mxExport extern __declspec( dllexport )
		#endif
	#elif defined (__FSK_LAYER__)
		#if SUPPORT_DLLEXPORT && !defined (_MSC_VER) && !defined(FSK_EXTENSION_EMBED)
			#define mxExport __attribute__ ((visibility("default")))
		#else
			#define mxExport extern
		#endif
	#else
		#define mxExport extern
	#endif
#endif

#ifndef mxImport
	#if mxWindows
		#if defined(mxXSC) || defined (__FSK_LAYER__)
			#define mxImport extern
		#else
			#define mxImport extern __declspec( dllimport )
		#endif
	#elif defined (__FSK_LAYER__)
		#if SUPPORT_DLLEXPORT && !defined (_MSC_VER)
			#define mxImport __attribute__ ((visibility("default")))
		#else
			#define mxImport extern
		#endif
	#else
		#define mxImport extern
	#endif
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

#ifndef __XSALL__
typedef struct xsGrammarRecord xsGrammar;
typedef struct xsJumpRecord xsJump;
typedef struct xsMachineRecord xsMachine;
typedef struct xsSlotRecord xsSlot;
#else
typedef struct sxGrammar xsGrammar;
typedef struct sxJump xsJump;
typedef struct sxMachine xsMachine;
typedef struct sxSlot xsSlot;
#endif

#if __FSK_LAYER__
	#include "Fsk.h"
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
	xsReferenceType,
	xsRegExpType,
	xsStringType
};
typedef char xsType;

#define xsTypeOf(_SLOT) \
	(the->scratch = (_SLOT), \
	fxTypeOf(the, &(the->scratch)))

/* Primitives */

typedef char xsBooleanValue;
typedef int xsIntegerValue;
typedef double xsNumberValue;
typedef char* xsStringValue;

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

/* Instances and Prototypes */

#define xsObjectPrototype (the->stackTop[-8])
#define xsFunctionPrototype (the->stackTop[-9])
#define xsArrayPrototype (the->stackTop[-10])
#define xsStringPrototype (the->stackTop[-11])
#define xsBooleanPrototype (the->stackTop[-12])
#define xsNumberPrototype (the->stackTop[-13])
#define xsDatePrototype (the->stackTop[-14])
#define xsRegExpPrototype (the->stackTop[-15])
#define xsHostPrototype (the->stackTop[-16])
#define xsErrorPrototype (the->stackTop[-17])
#define xsEvalErrorPrototype (the->stackTop[-18])
#define xsRangeErrorPrototype (the->stackTop[-19])
#define xsReferenceErrorPrototype (the->stackTop[-20])
#define xsSyntaxErrorPrototype (the->stackTop[-21])
#define xsTypeErrorPrototype (the->stackTop[-22])
#define xsURIErrorPrototype (the->stackTop[-23])
#define xsChunkPrototype (the->stackTop[-24])

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

typedef short xsIndex;

#define xsID(_NAME) \
	fxID(the, _NAME)
#define xsFindID(_NAME) \
	fxFindID(the, _NAME)
#define xsIsID(_NAME) \
	fxIsID(the, _NAME)
#define xsName(_ID) \
	fxName(the, _ID)

/* Properties */

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
	fxInteger(the, --the->stack, 0), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall0_noResult(_THIS,_ID) \
	(xsOverflow(-2), \
	fxInteger(the, --the->stack, 0), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall1(_THIS,_ID,_SLOT0) \
	(xsOverflow(-3), \
	fxPush(_SLOT0), \
	fxInteger(the, --the->stack, 1), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall1_noResult(_THIS,_ID,_SLOT0) \
	(xsOverflow(-3), \
	fxPush(_SLOT0), \
	fxInteger(the, --the->stack, 1), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall2(_THIS,_ID,_SLOT0,_SLOT1) \
	(xsOverflow(-4), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxInteger(the, --the->stack, 2), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall2_noResult(_THIS,_ID,_SLOT0,_SLOT1) \
	(xsOverflow(-4), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxInteger(the, --the->stack, 2), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall3(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-5), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxInteger(the, --the->stack, 3), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall3_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-5), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxInteger(the, --the->stack, 3), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall4(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-6), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxInteger(the, --the->stack, 4), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall4_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-6), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxInteger(the, --the->stack, 4), \
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
	fxInteger(the, --the->stack, 5), \
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
	fxInteger(the, --the->stack, 5), \
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
	fxInteger(the, --the->stack, 6), \
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
	fxInteger(the, --the->stack, 6), \
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
	fxInteger(the, --the->stack, 7), \
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
	fxInteger(the, --the->stack, 7), \
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
	fxInteger(the, --the->stack, 8), \
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
	fxInteger(the, --the->stack, 8), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsNew0(_THIS,_ID) \
	(xsOverflow(-2), \
	fxInteger(the, --the->stack, 0), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew1(_THIS,_ID,_SLOT0) \
	(xsOverflow(-3), \
	fxPush(_SLOT0), \
	fxInteger(the, --the->stack, 1), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew2(_THIS,_ID,_SLOT0,_SLOT1) \
	(xsOverflow(-4), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxInteger(the, --the->stack, 2), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew3(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-5), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxInteger(the, --the->stack, 3), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew4(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-6), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxInteger(the, --the->stack, 4), \
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
	fxInteger(the, --the->stack, 5), \
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
	fxInteger(the, --the->stack, 6), \
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
	fxInteger(the, --the->stack, 7), \
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
	fxInteger(the, --the->stack, 8), \
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

#define xsNewHostConstructor(_CALLBACK,_LENGTH,_PROTOTYPE) \
	(xsOverflow(-1), \
	fxPush(_PROTOTYPE), \
	fxNewHostConstructor(the, _CALLBACK, _LENGTH), \
	fxPop())
	
#define xsNewHostFunction(_CALLBACK,_LENGTH) \
	(fxNewHostFunction(the, _CALLBACK, _LENGTH), \
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

/* Arguments and Variables */

#define xsVars(_COUNT) fxVars(the, _COUNT)

#define xsArg(_INDEX) (the->frame[5 + fxCheckArg(the, _INDEX)])
#define xsArgc (the->frame[4])
#define xsThis (the->frame[3])
#define xsFunction (the->frame[2])
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

#if mxWindows && !__FSK_LAYER__
	#undef _setjmp
	#define _setjmp setjmp
#endif

#define xsTry \
	xsJump __JUMP__; \
	__JUMP__.nextJump = the->firstJump; \
	__JUMP__.stack = the->stack; \
	__JUMP__.scope = the->scope; \
	__JUMP__.frame = the->frame; \
	__JUMP__.code = the->code; \
	the->firstJump = &__JUMP__; \
	if (_setjmp(__JUMP__.buffer) == 0) {

#define xsCatch \
		the->firstJump = __JUMP__.nextJump; \
	} \
	else for (; (__JUMP__.stack); __JUMP__.stack = NULL)

/* Errors */

#ifdef mxDebug
#define xsError(_CODE) \
	fxError(the,(char *)__FILE__,__LINE__,_CODE)
#else
#define xsError(_CODE) \
	fxError(the,NULL,0,_CODE)
#endif

#define xsErrorPrintf(_MESSAGE) \
    fxErrorPrintf(the, __FILE__, __LINE__, _MESSAGE);

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
	fxTrace(the,_STRING)

/* Machine */

struct xsMachineRecord {
	xsSlot* stack;
	xsSlot* stackBottom;
	xsSlot* stackTop;
	xsSlot* scope;
	xsSlot* frame;
	xsJump* firstJump;
	void* context;
	xsIndex* code;
	xsSlot scratch;
#if __FSK_LAYER__
	FskInstrumentedItemDeclaration	/* xs.h */
#endif
};

typedef struct {
	xsIntegerValue initialChunkSize;
	xsIntegerValue incrementalChunkSize;
	xsIntegerValue initialHeapCount;
	xsIntegerValue incrementalHeapCount;
	xsIntegerValue stackCount;
	xsIntegerValue symbolCount;
	xsIntegerValue symbolModulo;
} xsAllocation;

#define xsNewMachine(_ALLOCATION,_GRAMMAR,_CONTEXT) \
	fxNewMachine(_ALLOCATION, _GRAMMAR, _CONTEXT)
	
#define xsDeleteMachine(_THE) \
	fxDeleteMachine(_THE)
	
#define xsAliasMachine(_ALLOCATION,_MACHINE,_NAME,_CONTEXT) \
	fxAliasMachine(_ALLOCATION, _MACHINE, _NAME, _CONTEXT)
	
#define xsShareMachine(_THE) \
	fxShareMachine(_THE)

/* Context */	
	
#define xsGetContext(_THE) \
	((_THE)->context)
	
#define xsSetContext(_THE,_CONTEXT) \
	((_THE)->context = (_CONTEXT))
	
#define xsModuleURL \
	(fxModuleURL(the), \
	fxPop())

/* Host */

#define xsBuildHost(_THE,_CALLBACK) \
	fxBuildHost(_THE, _CALLBACK)

#define xsBeginHost(_THE) \
	do { \
		xsMachine* __HOST_THE__ = _THE; \
		xsJump __HOST_JUMP__; \
		__HOST_JUMP__.nextJump = (__HOST_THE__)->firstJump; \
		__HOST_JUMP__.stack = (__HOST_THE__)->stack; \
		__HOST_JUMP__.scope = (__HOST_THE__)->scope; \
		__HOST_JUMP__.frame = (__HOST_THE__)->frame; \
		__HOST_JUMP__.code = (__HOST_THE__)->code; \
		(__HOST_THE__)->firstJump = &__HOST_JUMP__; \
		if (_setjmp(__HOST_JUMP__.buffer) == 0) { \
			xsMachine* the = fxBeginHost(__HOST_THE__)
			
#define xsEndHost(_THE) \
			fxEndHost(__HOST_THE__); \
			the = NULL; \
			(__HOST_THE__)->firstJump = __HOST_JUMP__.nextJump; \
		} \
		break; \
	} while(1)

enum {	
	xsDefault = 0,
	xsDontDelete = 2,
	xsDontEnum = 4,
	xsDontScript = 8,
	xsDontSet = 16,
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
	
typedef int (*xsGetter)(void*);
typedef int (*xsPutter)(xsStringValue, void*);

/* Scripts */
	
#define xsNewFunction(_ARGUMENTS,_ARGUMENTS_SIZE,_BODY,_BODY_SIZE,_FLAG,_PATH,_LINE) \
	(fxNewFunction(the,_ARGUMENTS,_ARGUMENTS_SIZE,_BODY,_BODY_SIZE,_FLAG,_PATH,_LINE), fxPop())
#define xsExecute(_THE,_STREAM,_GETTER,_PATH,_LINE) \
	fxExecute(_THE, _STREAM, _GETTER, _PATH, _LINE)
	
/* XML */

typedef struct {
	void (*doStartTag)(xsMachine*);
	void (*doStopTag)(xsMachine*);
	void (*doText)(xsMachine*);
	void (*lookupEntity)(xsMachine*);
	void (*doComment)(xsMachine*);
	void (*doProcessingInstruction)(xsMachine*);
	void (*doDoctype)(xsMachine*);
} xsScanCallbacks;

#define xsScan(_THE,_STREAM,_GETTER,_PATH,_LINE,_CALLBACKS) \
	(xsOverflow(-4), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxInteger(the, --the->stack, 3), \
	fxScan(_THE, _STREAM, _GETTER, _PATH, _LINE, _CALLBACKS), \
	fxPop())
#define xsScanBuffer(_THE,_BUFFER,_SIZE,_PATH,_LINE,_CALLBACKS) \
	(xsOverflow(-4), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxInteger(the, --the->stack, 3), \
	fxScanBuffer(_THE, _BUFFER, _SIZE, _PATH, _LINE, _CALLBACKS), \
	fxPop())
	
/* Grammars */

struct xsGrammarRecord {
	xsCallback callback;
	xsStringValue symbols;
	xsIntegerValue symbolsSize;
	xsStringValue code;
	xsIntegerValue codeSize;
	xsStringValue name;
};
		
#define xsLink(_GRAMMAR) \
	(fxLink(the, _GRAMMAR), \
	fxPop())

enum {
	xsSourceFlag = 1,
	xsNoErrorFlag = 2,
	xsNoWarningFlag = 4,
	xsSniffFlag = 8,
	xsSandboxFlag = 32,
	xsNoMixtureFlag = 64,
	xsDebugFlag = 128
};
typedef unsigned char xsFlag;

#define xsParse(_STREAM, _GETTER, _PATH, _LINE, _FLAG) \
	(xsOverflow(-4), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxInteger(the, --the->stack, 3), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse0(_STREAM, _GETTER, _PATH, _LINE, _FLAG) \
	(xsOverflow(-4), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxInteger(the, --the->stack, 3), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse1(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0) \
	(xsOverflow(-5), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxInteger(the, --the->stack, 4), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse2(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1) \
	(xsOverflow(-6), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxInteger(the, --the->stack, 5), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse3(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-7), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxInteger(the, --the->stack, 6), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse4(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-8), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxInteger(the, --the->stack, 7), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse5(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-9), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxInteger(the, --the->stack, 8), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse6(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5) \
	(xsOverflow(-10), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxInteger(the, --the->stack, 9), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse7(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6) \
	(xsOverflow(-11), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxInteger(the, --the->stack, 10), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParse8(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6,_SLOT7) \
	(xsOverflow(-12), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPush(_SLOT7), \
	fxInteger(the, --the->stack, 11), \
	fxParse(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParseBuffer(_STREAM, _GETTER, _PATH, _LINE, _FLAG) \
	(xsOverflow(-4), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxInteger(the, --the->stack, 3), \
	fxParseBuffer(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParseBuffer0(_STREAM, _GETTER, _PATH, _LINE, _FLAG) \
	(xsOverflow(-4), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxInteger(the, --the->stack, 3), \
	fxParseBuffer(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParseBuffer1(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0) \
	(xsOverflow(-5), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxInteger(the, --the->stack, 4), \
	fxParseBuffer(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParseBuffer2(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1) \
	(xsOverflow(-6), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxInteger(the, --the->stack, 5), \
	fxParseBuffer(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParseBuffer3(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-7), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxInteger(the, --the->stack, 6), \
	fxParseBuffer(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParseBuffer4(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-8), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxInteger(the, --the->stack, 7), \
	fxParseBuffer(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParseBuffer5(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-9), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxInteger(the, --the->stack, 8), \
	fxParseBuffer(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParseBuffer6(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5) \
	(xsOverflow(-10), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxInteger(the, --the->stack, 9), \
	fxParseBuffer(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParseBuffer7(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6) \
	(xsOverflow(-11), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxInteger(the, --the->stack, 10), \
	fxParseBuffer(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsParseBuffer8(_STREAM, _GETTER, _PATH, _LINE, _FLAG,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6,_SLOT7) \
	(xsOverflow(-12), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxUndefined(the, --the->stack), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPush(_SLOT7), \
	fxInteger(the, --the->stack, 11), \
	fxParseBuffer(the, _STREAM, _GETTER, _PATH, _LINE, _FLAG), \
	fxPop())

#define xsSerialize(_THIS,_STREAM,_PUTTER) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxSerialize(the, _STREAM, _PUTTER), \
	fxPop())
	
#define xsSerializeBuffer(_THIS,_BUFFER,_SIZE) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxSerializeBuffer(the, _BUFFER, _SIZE), \
	fxPop())

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

typedef unsigned char xsByteCode;
typedef xsByteCode* (*xsRemapIDsHook)(xsMachine*, xsByteCode*, xsIndex*);
typedef xsByteCode* (*xsRunLoopHook)(xsMachine*, xsByteCode*, xsSlot**, xsJump*);
#define xsGetRemapIDsHook() \
	fxGetRemapIDsHook()
#define xsGetRunLoopHook() \
	fxGetRunLoopHook()
#define xsSetRemapIDsHook(_HOOK) \
	fxSetRemapIDsHook(_HOOK)
#define xsSetRunLoopHook(_HOOK) \
	fxSetRunLoopHook(_HOOK)

#define xsIsProfiling() \
	fxIsProfiling(the)
#define xsStartProfiling() \
	fxStartProfiling(the)
#define xsStopProfiling() \
	fxStopProfiling(the)

#define xsDemarshall(_DATA) \
	(fxDemarshall(the, (_DATA)), \
	fxPop())
#define xsDemarshallCall(_DATA) \
	(fxDemarshallCall(the, (_DATA)), \
	fxPop())
#define xsMarshallCall() \
	(fxMarshallCall(the))
#define xsMarshallApply() \
	(fxMarshallApply(the))
#define xsMarshall(_SLOT) \
	(xsOverflow(-1), \
	fxPush(_SLOT), \
	fxMarshall(the))

#define xsGetHostModule(_NAME) fxGetHostModule(_NAME)
#define xsSetHostModule(_NAME,_CALLBACK) fxSetHostModule(_NAME, _CALLBACK)

#ifndef __XSALL__

#ifdef __cplusplus
extern "C" {
#endif

mxImport xsType fxTypeOf(xsMachine*, xsSlot*);

mxImport void fxUndefined(xsMachine*, xsSlot*);
mxImport void fxNull(xsMachine*, xsSlot*);
mxImport void fxBoolean(xsMachine*, xsSlot*, xsBooleanValue);
mxImport xsBooleanValue fxToBoolean(xsMachine*, xsSlot*);
mxImport void fxInteger(xsMachine*, xsSlot*, xsIntegerValue);
mxImport xsIntegerValue fxToInteger(xsMachine*, xsSlot*);
mxImport void fxNumber(xsMachine*, xsSlot*, xsNumberValue);
mxImport xsNumberValue fxToNumber(xsMachine*, xsSlot*);
mxImport xsSlot* fxString(xsMachine*, xsSlot*, xsStringValue);
mxImport xsSlot* fxStringBuffer(xsMachine*, xsSlot*, xsStringValue, xsIntegerValue);
mxImport xsStringValue fxToString(xsMachine*, xsSlot*);
mxImport xsStringValue fxToStringBuffer(xsMachine*, xsSlot*, xsStringValue, xsIntegerValue);

mxImport void fxNewInstanceOf(xsMachine*);
mxImport xsBooleanValue fxIsInstanceOf(xsMachine*);

mxImport void fxNewHostConstructor(xsMachine*, xsCallback, xsIntegerValue);
mxImport void fxNewHostFunction(xsMachine*, xsCallback, xsIntegerValue);
mxImport void fxNewHostObject(xsMachine*, xsDestructor);
mxImport void* fxGetHostData(xsMachine*, xsSlot*);
mxImport void fxSetHostData(xsMachine*, xsSlot*, void*);
mxImport xsDestructor fxGetHostDestructor(xsMachine*, xsSlot*);
mxImport void fxSetHostDestructor(xsMachine*, xsSlot*, xsDestructor);
mxImport xsHostHooks* fxGetHostHooks(xsMachine*, xsSlot*);
mxImport void fxSetHostHooks(xsMachine*, xsSlot*, xsHostHooks*);

mxImport xsIndex fxID(xsMachine*, char*);
mxImport xsIndex fxFindID(xsMachine*, char*);
mxImport xsBooleanValue fxIsID(xsMachine*, char*);
mxImport char* fxName(xsMachine*, xsIndex);

mxImport xsBooleanValue fxHasID(xsMachine*, xsIndex);
mxImport xsBooleanValue fxHasOwnID(xsMachine*, xsIndex);
mxImport void fxGet(xsMachine*, xsSlot*, xsIndex);
mxImport void fxGetAt(xsMachine*);
mxImport void fxGetID(xsMachine*, xsIndex);
mxImport void fxSet(xsMachine*, xsSlot*, xsIndex);
mxImport void fxSetAt(xsMachine*);
mxImport void fxSetID(xsMachine*, xsIndex);
mxImport void fxDeleteAt(xsMachine*);
mxImport void fxDeleteID(xsMachine*, xsIndex);
mxImport void fxCall(xsMachine*);
mxImport void fxCallID(xsMachine*, xsIndex);
mxImport void fxNew(xsMachine*);
mxImport void fxNewID(xsMachine*, xsIndex);

mxImport void fxRunAdd(xsMachine*);
mxImport void fxRunArguments(xsMachine*);
mxImport xsBooleanValue fxRunCompare(xsMachine*, xsBooleanValue, xsBooleanValue, xsBooleanValue, xsBooleanValue);
mxImport void fxRunDelta(xsMachine* the, xsIntegerValue theDelta);
mxImport xsBooleanValue fxRunEqual(xsMachine*, xsBooleanValue, xsBooleanValue, xsBooleanValue);
mxImport void fxRunForIn(xsMachine* the);
mxImport xsBooleanValue fxRunIn(xsMachine* the, xsBooleanValue testing);
mxImport xsBooleanValue fxRunInstanceof(xsMachine* the, xsBooleanValue testing);
mxImport void fxRunSign(xsMachine* the, xsIntegerValue theSign);
mxImport xsBooleanValue fxRunStrictEqual(xsMachine* the, xsBooleanValue testing, xsBooleanValue yes, xsBooleanValue no);
mxImport void fxRunSubtract(xsMachine* the);
mxImport xsBooleanValue fxRunTest(xsMachine* the);
mxImport void fxRunTypeof(xsMachine* the);

mxImport void fxVars(xsMachine*, xsIntegerValue);
mxImport xsIntegerValue fxCheckArg(xsMachine*, xsIntegerValue);
mxImport xsIntegerValue fxCheckVar(xsMachine*, xsIntegerValue);
mxImport void fxOverflow(xsMachine*, xsIntegerValue, xsStringValue, xsIntegerValue);

mxImport void fxThrow(xsMachine*, xsStringValue, xsIntegerValue) XS_FUNCTION_NORETURN;

mxImport void fxError(xsMachine*, xsStringValue, xsIntegerValue, xsIntegerValue) XS_FUNCTION_ANALYZER_NORETURN;
mxImport void fxErrorPrintf(xsMachine*, xsStringValue, xsIntegerValue, xsStringValue) XS_FUNCTION_ANALYZER_NORETURN;

mxImport void fxDebugger(xsMachine*, xsStringValue, xsIntegerValue);
mxImport void fxTrace(xsMachine*, xsStringValue);

mxImport xsMachine* fxNewMachine(xsAllocation*, xsGrammar*, void*);
mxImport void fxDeleteMachine(xsMachine*);
mxImport xsMachine* fxAliasMachine(xsAllocation*, xsMachine*, xsStringValue, void*);
mxImport void fxShareMachine(xsMachine*);

mxImport xsBooleanValue fxBuildHost(xsMachine*, xsCallback);
mxImport xsMachine* fxBeginHost(xsMachine*);
mxImport void fxEndHost(xsMachine*);
mxImport void fxPutID(xsMachine*, xsIndex, xsAttribute, xsAttribute);

mxImport void fxCollectGarbage(xsMachine*);
mxImport void fxEnableGarbageCollection(xsMachine* the, xsBooleanValue enableIt);
mxImport void fxRemember(xsMachine*, xsSlot*);
mxImport void fxForget(xsMachine*, xsSlot*);
mxImport void fxAccess(xsMachine*, xsSlot*);

mxImport void fxNewFunction(xsMachine*, xsStringValue, xsIntegerValue, xsStringValue, xsIntegerValue, xsFlag, xsStringValue, xsIntegerValue);
mxImport xsBooleanValue fxExecute(xsMachine*, void*, xsGetter, xsStringValue, xsIntegerValue);
mxImport void fxScan(xsMachine*, void*, xsGetter, xsStringValue, xsIntegerValue, xsScanCallbacks*);
mxImport void fxScanBuffer(xsMachine*, void*, xsIntegerValue, xsStringValue, xsIntegerValue, xsScanCallbacks*);

mxImport void fxLink(xsMachine*, xsGrammar*);
mxImport void fxParse(xsMachine*, void*, xsGetter, xsStringValue, xsIntegerValue, xsFlag);
mxImport void fxParseBuffer(xsMachine*, void*, xsIntegerValue, xsStringValue, xsIntegerValue, xsFlag);
mxImport void fxSerialize(xsMachine*, void*, xsPutter);
mxImport void fxSerializeBuffer(xsMachine*, void*, xsIntegerValue);

mxImport void fxEnterSandbox(xsMachine*);
mxImport void fxLeaveSandbox(xsMachine*);
mxImport void fxSandbox(xsMachine*);
mxImport xsIntegerValue fxScript(xsMachine*);

mxImport xsRemapIDsHook fxGetRemapIDsHook();
mxImport xsRunLoopHook fxGetRunLoopHook();
mxImport void fxSetRemapIDsHook(xsRemapIDsHook);
mxImport void fxSetRunLoopHook(xsRunLoopHook);

mxImport xsBooleanValue fxIsProfiling(xsMachine*);
mxImport void fxStartProfiling(xsMachine*);
mxImport void fxStopProfiling(xsMachine*);
	
mxImport void fxDemarshall(xsMachine*, void*);
mxImport void fxDemarshallCall(xsMachine*, void*);
mxImport void* fxMarshall(xsMachine*);
mxImport void* fxMarshallCall(xsMachine*);
mxImport void* fxMarshallApply(xsMachine*);

mxImport void fxDecodeURI(xsMachine*, xsStringValue);
mxImport void fxEncodeURI(xsMachine*, xsStringValue);
mxImport xsIntegerValue fxUnicodeLength(xsStringValue theString);
mxImport xsIntegerValue fxUnicodeToUTF8Offset(xsStringValue theString, xsIntegerValue theOffset);
mxImport xsIntegerValue fxUTF8ToUnicodeOffset(xsStringValue theString, xsIntegerValue theOffset);

mxImport xsCallback fxGetHostModule(xsStringValue theName);
mxImport void fxSetHostModule(xsStringValue theName, xsCallback theCallback);

mxImport void fxModuleURL(xsMachine*);
mxImport void fxHostScope(xsMachine*, xsIndex);
mxImport void fxNewHostClosure(xsMachine*, xsBooleanValue);
mxImport void fxNewHostSymbolMap(xsMachine*, xsIntegerValue, xsStringValue*);
mxImport void fxCopyObject(xsMachine*);

#ifdef __cplusplus
}
#endif

#endif /* __XSALL__ */

#endif /* __XS__ */

