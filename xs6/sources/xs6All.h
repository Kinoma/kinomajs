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
#ifndef __XS6ALL__
#define __XS6ALL__

#include "xs6Common.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define mxFrequency 1
#ifndef mxProxy
	#define mxProxy 1
#endif
#ifndef mxRegExp
	#define mxRegExp 1
#endif

typedef struct sxMachine txMachine;
typedef struct sxSlot txSlot;
typedef struct sxBlock txBlock;
typedef struct sxChunk txChunk;
typedef struct sxJump txJump;
typedef struct sxProfileRecord txProfileRecord;
typedef struct sxCreation txCreation;
typedef struct sxHostFunctionBuilder txHostFunctionBuilder;

typedef void (*txCallback)(txMachine*);
typedef txCallback (*txCallbackAt)(txID index);
typedef void (*txDestructor)(void*);
typedef void (*txMarker)(void*, void (*)(txMachine*, txSlot*));
typedef void (*txStep)(txMachine*, txSlot*, txInteger, txSlot*);
typedef void (*txTypeCallback)(txMachine*, txSlot*, txInteger, txSlot*, int);
typedef int (*txTypeCompare)(const void*, const void*);

typedef struct {
    txInteger lo;
    txInteger hi;
} txSortPartition;
#define mxSortThreshold 4
#define mxSortStackSize 8 * sizeof(txUnsigned)

typedef struct {
	txInteger size;
	txTypeCallback getter;
	txTypeCallback setter;
	txTypeCompare compare;
	txCallback get;
	txCallback set;
	txID getID;
	txID setID;
	txID constructorID;
} txTypeDispatch;

typedef struct  {
	txDestructor destructor;
	txMarker marker;
} txHostHooks;

typedef union {
	txBoolean boolean;
	txInteger integer;
	txNumber number;
	txString string;
	txID ID;

	txSlot* reference;

	txSlot* closure;
	struct { txByte* code; txSlot* scope; } frame;

	struct { txSlot* garbage; txSlot* prototype; } instance;
	
	struct { txSlot* address; txIndex length; } array;
	struct { txByte* address; txIndex length; } arrayBuffer;
	struct { txCallback address; txID* IDs; } callback;
	txByte* code;
	struct { txInteger offset; txInteger size; } dataView;
	struct { void* data; union { txDestructor destructor; txHostHooks* hooks; } variant; } host;
	struct { txSlot* handler; txSlot* target; } proxy;
	struct { void* code; txInteger* offsets; } regexp;
	struct { txSlot** address; txSize length;} table;
	txTypeDispatch* typedArray;
	
	struct { txSlot* getter; txSlot* setter; } accessor;
	struct { txSlot* slot; txU4 sum; } entry;
	struct { void* data; txInteger profileID; } info;
	struct { txString string; txU4 sum; } key;
	struct { txSlot* first; txSlot* last; } list;
} txValue;

struct sxBlock {
	txBlock* nextBlock;
	txByte* current;
	txByte* limit;
	txByte* temporary;
};

struct sxChunk {
	txSize size;
	txByte* temporary;
};

struct sxJump {
	c_jmp_buf buffer; /* xs.h */
	txJump* nextJump; /* xs.h */
	txSlot* stack; /* xs.h */
	txSlot* scope; /* xs.h */
	txSlot* frame; /* xs.h */
	txSlot* environment; /* xs.h */
	txByte* code; /* xs.h */
	txBoolean flag; /* xs.h */
};

struct sxSlot {
	txSlot* next;
	txID ID;
	txFlag flag;
	txKind kind;
#if (defined(__GNUC__) && defined(__LP64__)) || (defined(_MSC_VER) && defined(_M_X64))
	// Made it alinged and consistent on all platforms
	txInteger dummy;
#endif
	txValue value;
};

struct sxProfileRecord {
	txInteger delta;
	txInteger profileID;
};

struct sxMachine {
	txSlot* stack; /* xs.h */
	txSlot* scope; /* xs.h */
	txSlot* frame; /* xs.h */
	txByte* code; /* xs.h */
	txSlot* stackBottom; /* xs.h */
	txSlot* stackTop; /* xs.h */
	txJump* firstJump; /* xs.h */
	void* context; /* xs.h */
	txSlot scratch; /* xs.h */
	txFlag status;

	txSlot* cRoot;

	txBlock* firstBlock;

	txSlot* freeHeap;
	txSlot* firstHeap;

	txSize nameModulo;
	txSlot** nameTable;
	txSize symbolModulo;
	txSlot** symbolTable;

	txSlot** keyArray;
	txID keyCount;
	txID keyIndex;
	int	keyOffset;

	txID aliasCount;
	txID aliasIndex;
	txSlot** aliasArray;
	
	txSlot* firstWeakMapTable;
	txSlot* firstWeakSetTable;

	txSize currentChunksSize;
	txSize peakChunksSize;
	txSize maximumChunksSize;
	txSize minimumChunksSize;

	txSize currentHeapCount;
	txSize peakHeapCount;
	txSize maximumHeapCount;
	txSize minimumHeapCount;

	txBoolean shared;
	txMachine* sharedMachine;

	txBoolean collectFlag;
	txFlag requireFlag;
	void* dtoa;
	
	void* archive;
	void* host;

	char nameBuffer[256];
#ifdef mxDebug
	txString echoBuffer;
	txInteger echoOffset;
	txInteger echoSize;
	txSocket connection;
	txString name;
	txSlot** sorter;
	txBoolean breakOnExceptionFlag;
	void* reader;
#endif
#ifdef mxFrequency
	txNumber exits[XS_CODE_COUNT];
	txNumber frequencies[XS_CODE_COUNT];
#endif
#ifdef mxProfile
	txString profileDirectory;
	void* profileFile;
	txInteger profileID;
	#if mxWindows
		LARGE_INTEGER profileFrequency;
		LARGE_INTEGER profileCounter;
	#else
		c_timeval profileTV;
	#endif
	txProfileRecord* profileBottom;
	txProfileRecord* profileCurrent;
	txProfileRecord* profileTop;
#endif
#if __FSK_LAYER__
	FskInstrumentedItemDeclaration	/* xs.h */
#endif
};

struct sxCreation {
	txSize initialChunkSize; /* xs.h */
	txSize incrementalChunkSize; /* xs.h */
	txSize initialHeapCount; /* xs.h */
	txSize incrementalHeapCount; /* xs.h */
	txSize stackCount; /* xs.h */
	txSize keyCount; /* xs.h */
	txSize nameModulo; /* xs.h */
	txSize symbolModulo; /* xs.h */
};

struct sxHostFunctionBuilder {
	txCallback callback;
	txInteger length;
	txID id; 
};

typedef struct {
	txSlot* slot;
	txSize offset;
	txSize size;
} txStringStream;

typedef struct {
	txString buffer;
	txSize offset;
	txSize size;
} txStringCStream;

/* xsAPI.c */

mxExport txKind fxTypeOf(txMachine*, txSlot*);

mxExport void fxPushCount(txMachine*, txInteger);
mxExport void fxUndefined(txMachine*, txSlot*);
mxExport void fxNull(txMachine*, txSlot*);
mxExport void fxBoolean(txMachine*, txSlot*, txS1);
mxExport txBoolean fxToBoolean(txMachine*, txSlot*);
mxExport void fxInteger(txMachine*, txSlot*, txInteger);
mxExport txInteger fxToInteger(txMachine*, txSlot*);
mxExport void fxNumber(txMachine*, txSlot*, txNumber);
mxExport txNumber fxToNumber(txMachine*, txSlot*);
mxExport void fxString(txMachine*, txSlot*, txString);
mxExport void fxStringBuffer(txMachine* the, txSlot* theSlot, txString theValue, txSize theSize);
mxExport txString fxToString(txMachine*, txSlot*);
mxExport txString fxToStringBuffer(txMachine*, txSlot*, txString, txSize);
mxExport txString fxToStringCopy(txMachine*, txSlot*);
mxExport void fxUnsigned(txMachine*, txSlot*, txUnsigned);
mxExport txUnsigned fxToUnsigned(txMachine*, txSlot*);

mxExport void fxNewInstanceOf(txMachine*);
mxExport txBoolean fxIsInstanceOf(txMachine*);
mxExport void fxArrayCacheBegin(txMachine*, txSlot*);
mxExport void fxArrayCacheEnd(txMachine*, txSlot*);
mxExport void fxArrayCacheItem(txMachine*, txSlot*, txSlot*);

mxExport void fxBuildHosts(txMachine*, txInteger, txHostFunctionBuilder*);
mxExport txSlot* fxNewHostConstructor(txMachine*, txCallback, txInteger, txInteger);
mxExport txSlot* fxNewHostFunction(txMachine*, txCallback, txInteger, txInteger);
mxExport txSlot* fxNewHostInstance(txMachine* the);
mxExport txSlot* fxNewHostObject(txMachine*, txDestructor);
mxExport void* fxGetHostData(txMachine*, txSlot*);
mxExport txDestructor fxGetHostDestructor(txMachine*, txSlot*);
mxExport txHostHooks* fxGetHostHooks(txMachine*, txSlot*);
mxExport void fxSetHostData(txMachine*, txSlot*, void*);
mxExport void fxSetHostDestructor(txMachine*, txSlot*, txDestructor);
mxExport void fxSetHostHooks(txMachine*, txSlot*, txHostHooks*);

mxExport void fxIDs(txMachine* the, txInteger count, txString* names);
mxExport txID fxID(txMachine*, txString);
mxExport txID fxFindID(txMachine* the, txString theName);
mxExport txS1 fxIsID(txMachine*, txString);
mxExport txString fxName(txMachine*, txID);

mxExport void fxEnumerate(txMachine* the);
mxExport txBoolean fxHasID(txMachine*, txInteger);
mxExport txBoolean fxHasOwnID(txMachine*, txInteger);
mxExport void fxGetAt(txMachine*);
mxExport void fxGetClosure(txMachine* the, txInteger theID);
mxExport void fxGetID(txMachine*, txInteger);
mxExport void fxSetAt(txMachine*);
mxExport void fxSetClosure(txMachine* the, txInteger theID);
mxExport void fxSetID(txMachine*, txInteger);
mxExport void fxDeleteAt(txMachine*);
mxExport void fxDeleteID(txMachine*, txInteger);
mxExport void fxCall(txMachine*);
mxExport void fxCallID(txMachine*, txInteger);
mxExport void fxNew(txMachine*);
mxExport void fxNewID(txMachine*, txInteger);
mxExport txBoolean fxRunTest(txMachine* the);

mxExport void fxVars(txMachine*, txInteger);

mxExport txInteger fxCheckArg(txMachine*, txInteger);
mxExport txInteger fxCheckVar(txMachine*, txInteger);
mxExport void fxOverflow(txMachine*, txInteger, txString thePath, txInteger theLine);
mxExport void fxThrow(txMachine* the, txString thePath, txInteger theLine) XS_FUNCTION_NORETURN;
mxExport void fxThrowMessage(txMachine* the, txString thePath, txInteger theLine, txError theError, txString theMessage, ...) XS_FUNCTION_NORETURN;
mxExport void fxError(txMachine* the, txString thePath, txInteger theLine, txInteger theCode) XS_FUNCTION_NORETURN;
mxExport void fxDebugger(txMachine* the, txString thePath, txInteger theLine);

mxExport txMachine* fxCreateMachine(txCreation* theCreation, void* theArchive, txString theName, void* theContext);
mxExport void fxDeleteMachine(txMachine*);
mxExport txMachine* fxCloneMachine(txCreation* theCreation, txMachine* theMachine, txString theName, void* theContext);
mxExport void fxShareMachine(txMachine* the);

mxExport txMachine* fxBeginHost(txMachine*);
mxExport void fxEndHost(txMachine*);
mxExport void fxPutAt(txMachine*, txFlag, txFlag);
mxExport void fxPutID(txMachine*, txInteger, txFlag, txFlag);

mxExport void fxCollectGarbage(txMachine*);
mxExport void fxEnableGarbageCollection(txMachine* the, txBoolean enableIt);
mxExport void fxRemember(txMachine*, txSlot*);
mxExport void fxForget(txMachine*, txSlot*);
mxExport void fxAccess(txMachine*, txSlot*);

mxExport void fxCopyObject(txMachine* the);
mxExport void fxDemarshall(txMachine* the, void* theData, txBoolean alien);
mxExport void* fxMarshall(txMachine* the, txBoolean alien);
mxExport void fxModulePaths(txMachine* the);

/* xs*Host.c */
extern void* fxAllocateChunks(txMachine* the, txSize theSize);
extern txSlot* fxAllocateSlots(txMachine* the, txSize theCount);
extern void fxBuildHost(txMachine* the);
extern void fxBuildKeys(txMachine* the);
extern txID fxFindModule(txMachine* the, txID moduleID, txSlot* name);
extern void fxFreeChunks(txMachine* the, void* theChunks);
extern void fxFreeSlots(txMachine* the, void* theSlots);
extern void fxLoadModule(txMachine* the, txID moduleID);
extern void fxMarkHost(txMachine* the);
extern txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags);
extern void fxQueuePromiseJobs(txMachine* the);
extern void fxSweepHost(txMachine* the);

/* xs*Platform.c */
extern void fxErrorMessage(txMachine* the, txInteger theCode, txString theBuffer, txSize theSize);
#ifdef mxDebug
extern void fxAddReadableCallback(txMachine* the);
extern void fxConnect(txMachine* the);
extern void fxDisconnect(txMachine* the);
extern char *fxGetAddress(txMachine* the);
extern txBoolean fxGetAutomatic(txMachine* the);
mxExport txBoolean fxIsConnected(txMachine* the);
extern txBoolean fxIsReadable(txMachine* the);
extern void fxReadBreakpoints(txMachine* the);
extern void fxReceive(txMachine* the);
extern void fxRemoveReadableCallback(txMachine* the);
extern void fxSend(txMachine* the);
extern void fxSetAddress(txMachine* the, char* theAddress);
extern void fxSetAutomatic(txMachine* the, txBoolean theAutomatic);
extern void fxWriteBreakpoints(txMachine* the);

#endif
#ifdef mxProfile
extern void fxCloseProfileFile(txMachine* the);
extern void fxOpenProfileFile(txMachine* the, char* theName);
extern void fxWriteProfileFile(txMachine* the, void* theBuffer, txInteger theSize);
#endif

/* xsAll.c */
extern txString fxAdornStringC(txMachine* the, txString prefix, txSlot* string, txString suffix);
extern txInteger fxArgToInteger(txMachine* the, txInteger i, txInteger value);
extern txSlot* fxArgToCallback(txMachine* the, txInteger argi);
extern void fxBufferFrameName(txMachine* the, txString buffer, txSize size, txSlot* frame, txString suffix);
extern void fxBufferFunctionName(txMachine* the, txString buffer, txSize size, txSlot* function, txString suffix);
extern void fxBufferObjectName(txMachine* the, txString buffer, txSize size, txSlot* object, txString suffix);
extern txString fxConcatString(txMachine* the, txSlot* a, txSlot* b);
extern txString fxConcatStringC(txMachine* the, txSlot* a, txString b);
extern txString fxCopyString(txMachine* the, txSlot* a, txSlot* b);
extern txString fxCopyStringC(txMachine* the, txSlot* a, txString b);
extern txString fxResizeString(txMachine* the, txSlot* a, txSize theSize);

extern int fxStringGetter(void*);
extern int fxStringCGetter(void*);
extern void fxJump(txMachine*) XS_FUNCTION_NORETURN;
mxExport txInteger fxUnicodeCharacter(txString theString);
mxExport txInteger fxUnicodeLength(txString theString);
mxExport txInteger fxUnicodeToUTF8Offset(txString theString, txInteger theOffset);
mxExport txInteger fxUTF8ToUnicodeOffset(txString theString, txInteger theOffset);

/* xsRun.c */
extern void fxRunID(txMachine* the, txSlot* generator, txID theID);
extern void fxRunScript(txMachine* the, txScript* script, txSlot* _this, txSlot* environment, txSlot* home, txSlot* module);
extern txBoolean fxIsSameSlot(txMachine* the, txSlot* a, txSlot* b);
extern txBoolean fxIsSameValue(txMachine* the, txSlot* a, txSlot* b);

/* xsMemory.c */
extern void fxCheckStack(txMachine* the, txSlot* slot);
extern void fxAllocate(txMachine* the, txCreation* theCreation);
extern void fxCollect(txMachine* the, txBoolean theFlag);
extern txSlot* fxDuplicateSlot(txMachine* the, txSlot* theSlot);
extern void fxFree(txMachine* the);
mxExport void* fxNewChunk(txMachine* the, txSize theSize);
extern txSlot* fxNewSlot(txMachine* the);
extern void* fxRenewChunk(txMachine* the, void* theData, txSize theSize);
extern void fxShare(txMachine* the);

/* xsDebug.c */
#ifdef mxDebug
mxExport void fxCheck(txMachine* the, txString thePath, txInteger theLine);
extern void fxClearAllBreakpoints(txMachine* the);
extern void fxClearBreakpoint(txMachine* the, txString thePath, txString theLine);
extern void fxDebugCommand(txMachine* the);
extern void fxDebugFile(txMachine* the, txSlot* theSymbol);
extern void fxDebugLine(txMachine* the);
extern void fxDebugLoop(txMachine* the, txString thePath, txInteger theLine, txString message);
extern void fxDebugThrow(txMachine* the, txString thePath, txInteger theLine, txString message);
extern txSlot* fxGetNextBreakpoint(txMachine* the, txSlot* theBreakpoint, txString thePath, txString theLine);
mxExport void fxLogin(txMachine* the);
mxExport void fxLogout(txMachine* the);
mxExport void fxSetBreakpoint(txMachine* the, txString thePath, txString theLine);
#endif
mxExport void fxReport(txMachine* the, txString theFormat, ...);
mxExport void fxReportError(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...);
mxExport void fxReportWarning(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...);

/* xsType.c */
extern txSlot* fxGetInstance(txMachine* the, txSlot* theSlot);
extern txSlot* fxGetParent(txMachine* the, txSlot* theSlot);
extern void fxCallInstance(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments);
extern void fxConstructInstance(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target);
extern txBoolean fxDefineInstanceProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* descriptor);
extern void fxEachInstanceProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txSlot* instance);
extern void fxEnumerateInstance(txMachine* the, txSlot* instance);
extern txSlot* fxGetInstanceOwnProperty(txMachine* the, txSlot* instance, txInteger id);
extern void fxGetInstancePrototype(txMachine* the, txSlot* instance);
extern void fxIsInstanceExtensible(txMachine* the, txSlot* instance);
extern txSlot* fxNewInstance(txMachine* the);
extern void fxPreventInstanceExtensions(txMachine* the, txSlot* instance);
extern void fxSetInstancePrototype(txMachine* the, txSlot* instance, txSlot* prototype);
extern void fxStepInstanceProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txInteger id, txSlot* property);
extern txSlot* fxToInstance(txMachine* the, txSlot* theSlot);
extern void fxToPrimitive(txMachine* the, txSlot* theSlot, txBoolean theHint);
extern void fx_species_get(txMachine* the);

/* xsProperty.c */
extern txSlot* fxLastProperty(txMachine* the, txSlot* slot);
extern txSlot* fxNextHostAccessorProperty(txMachine* the, txSlot* property, txCallback get, txCallback set, txID id, txFlag flag);
extern txSlot* fxNextHostFunctionProperty(txMachine* the, txSlot* property, txCallback call, txInteger length, txID id, txFlag flag);
extern txSlot* fxNextUndefinedProperty(txMachine* the, txSlot* property, txID id, txFlag flag);
extern txSlot* fxNextNullProperty(txMachine* the, txSlot* property, txID id, txFlag flag);
extern txSlot* fxNextBooleanProperty(txMachine* the, txSlot* property, txBoolean boolean, txID id, txFlag flag);
extern txSlot* fxNextIntegerProperty(txMachine* the, txSlot* property, txInteger integer, txID id, txFlag flag);
extern txSlot* fxNextNumberProperty(txMachine* the, txSlot* property, txNumber number, txID id, txFlag flag);
extern txSlot* fxNextPopProperty(txMachine* the, txSlot* property, txID id, txFlag flag);
extern txSlot* fxNextSlotProperty(txMachine* the, txSlot* property, txSlot* slot, txID id, txFlag flag);
extern txSlot* fxNextStringProperty(txMachine* the, txSlot* property, txString string, txID id, txFlag flag);
extern txSlot* fxNextSymbolProperty(txMachine* the, txSlot* property, txID symbol, txID id, txFlag flag);
extern txSlot* fxNextTypeDispatchProperty(txMachine* the, txSlot* property, txTypeDispatch* dispatch, txID id, txFlag flag);

extern void fxDescribeProperty(txMachine* the, txSlot* property);
extern void fxEnumProperties(txMachine* the, txSlot* instance, txFlag flag);
extern txSlot* fxGetOwnProperty(txMachine* the, txSlot* theInstance, txInteger theID);
extern txSlot* fxGetProperty(txMachine* the, txSlot* theInstance, txInteger theID);
extern txBoolean fxHasOwnProperty(txMachine* the, txSlot* theInstance, txInteger theID);
extern txBoolean fxHasProperty(txMachine* the, txSlot* theInstance, txInteger theID);
extern txBoolean fxRemoveProperty(txMachine* the, txSlot* theInstance, txInteger theID);
extern txBoolean fxNewProperty(txMachine* the, txSlot* instance, txInteger ID, txFlag flag, txSlot* slot);
extern txSlot* fxSetProperty(txMachine* the, txSlot* theInstance, txInteger theID, txFlag* theFlag);

extern txSlot* fxGetGlobalProperty(txMachine* the, txSlot* theInstance, txInteger theID);
extern txBoolean fxRemoveGlobalProperty(txMachine* the, txSlot* theInstance, txInteger theID);
extern txSlot* fxSetGlobalProperty(txMachine* the, txSlot* theInstance, txInteger theID, txFlag* theFlag);

/* xsModule.c */
#define mxTransferLocal(TRANSFER) (TRANSFER)->value.reference->next
#define mxTransferFrom(TRANSFER) (TRANSFER)->value.reference->next->next
#define mxTransferImport(TRANSFER) (TRANSFER)->value.reference->next->next->next
#define mxTransferAliases(TRANSFER) (TRANSFER)->value.reference->next->next->next->next
#define mxTransferClosure(TRANSFER) (TRANSFER)->value.reference->next->next->next->next->next

extern void fxBuildModule(txMachine* the);
extern txID fxCurrentModuleID(txMachine* the);
extern txSlot* fxRequireModule(txMachine* the, txID moduleID, txSlot* name);
extern void fxResolveModule(txMachine* the, txID moduleID, txScript* script, void* data, txDestructor destructor);

/* xsGlobal.c */
extern void fxBuildGlobal(txMachine* the);
extern txSlot* fxCheckIteratorInstance(txMachine* the, txSlot* slot);
extern txSlot* fxNewIteratorInstance(txMachine* the, txSlot* iterable);
extern txSlot* fxNewHostAccessorGlobal(txMachine* the, txCallback get, txCallback set, txID id, txFlag flag);
extern txSlot* fxNewHostFunctionGlobal(txMachine* the, txCallback call, txInteger length, txID id, txFlag flag);
extern txSlot* fxNewHostConstructorGlobal(txMachine* the, txCallback call, txInteger length, txID id, txFlag flag);
mxExport void fxDecodeURI(txMachine* the, txString theSet);
mxExport void fxEncodeURI(txMachine* the, txString theSet);

/* xsObject.c */
extern void fxBuildObject(txMachine* the);
extern txSlot* fxNewObjectInstance(txMachine* the);
extern void fxDefineDataProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* value);

/* xsFunction.c */
extern void fxBuildFunction(txMachine* the);
extern txSlot* fxCreateInstance(txMachine* the, txSlot* slot);
extern void fxDefaultFunctionPrototype(txMachine* the, txSlot* function, txSlot* prototype);
extern txBoolean fxIsBaseFunctionInstance(txMachine* the, txSlot* slot);
extern txSlot* fxNewFunctionInstance(txMachine* the, txID name);
extern void fxRenameFunction(txMachine* the, txSlot* function, txInteger id, txString former, txString prefix);

/* xsSymbol.c */
void fxBuildSymbol(txMachine* the);
extern txSlot* fxNewSymbolInstance(txMachine* the);
extern void fxSymbolToString(txMachine* the, txSlot* slot);
extern txSlot* fxGetKey(txMachine* the, txID theID);
extern txSlot* fxFindName(txMachine* the, txString theString);
extern txSlot* fxNewName(txMachine* the, txSlot* theSlot);
extern txSlot* fxNewNameC(txMachine* the, txString theString);
extern txSlot* fxNewNameX(txMachine* the, txString theString);
extern void fxIDToSlot(txMachine* the, txInteger id, txSlot* slot);
extern void fxIDToString(txMachine* the, txInteger id, txString theBuffer, txSize theSize);
extern void fxSlotToID(txMachine* the, txSlot* slot, txInteger* id);

/* xsBoolean.c */
extern void fxBuildBoolean(txMachine* the);
extern txSlot* fxNewBooleanInstance(txMachine* the);

/* xsError.c */
extern void fxBuildError(txMachine* the);

/* xsNumber.c */
extern void fxBuildNumber(txMachine* the);
extern txSlot* fxNewNumberInstance(txMachine* the);

/* xsMath.c */
extern void fxBuildMath(txMachine* the);

/* xsDate.c */
extern void fxBuildDate(txMachine* the);
extern txSlot* fxNewDateInstance(txMachine* the);

/* xsString.c */
extern void fxBuildString(txMachine* the);
extern txSlot* fxNewStringInstance(txMachine* the);
extern txSlot* fxGetStringProperty(txMachine* the, txSlot* instance, txInteger index);
extern txBoolean fxRemoveStringProperty(txMachine* the, txSlot* instance, txInteger index);
extern txSlot* fxSetStringProperty(txMachine* the, txSlot* instance, txInteger index);
extern txSlot* fx_String_prototype_split_aux(txMachine* the, txSlot* theString, txSlot* theArray, txSlot* theItem, txInteger theStart, txInteger theStop);
extern txInteger fx_String_prototype_replace_aux(txMachine* the, txSlot* theString, txInteger theDelta, txInteger theLength, txInteger theCount, txSlot* theSlot);
extern void fxPushSubstitutionString(txMachine* the, txSlot* string, txInteger size, txInteger offset, txSlot* match, txInteger length, txInteger count, txSlot* captures, txSlot* replace);

/* xsRegExp.c */
extern void fxBuildRegExp(txMachine* the);
extern txBoolean fxIsRegExp(txMachine* the, txSlot* slot);
extern txSlot* fxNewRegExpInstance(txMachine* the);

/* xsArray.c */
extern txIndex fxArgToIndex(txMachine* the, txInteger argi, txIndex index, txIndex length);
extern txIndex fxArgToArrayLimit(txMachine* the, txInteger argi);
extern void fxBuildArray(txMachine* the);
extern void fxCacheArray(txMachine* the, txSlot* theArray);
extern void fxConstructArrayEntry(txMachine* the, txSlot* entry);
extern txSlot* fxGetArrayProperty(txMachine* the, txSlot* array, txInteger index);
extern txBoolean fxIsArray(txMachine* the, txSlot* instance);
extern txSlot* fxNewArrayInstance(txMachine* the);
extern txSlot* fxNewArrayFromList(txMachine* the, txSlot* reference);
extern txSlot* fxNewListFromArray(txMachine* the, txSlot* reference);
extern txBoolean fxRemoveArrayProperty(txMachine* the, txSlot* array, txInteger index);
extern txBoolean fxSetArrayLength(txMachine* the, txSlot* array, txIndex target);
extern txSlot* fxSetArrayProperty(txMachine* the, txSlot* array, txInteger index);

extern txSlot* fxNewParametersInstance(txMachine* the, txInteger count);
extern txSlot* fxGetParametersProperty(txMachine* the, txSlot* instance, txInteger index);
extern txBoolean fxRemoveParametersProperty(txMachine* the, txSlot* instance, txInteger index);
extern txSlot* fxSetParametersProperty(txMachine* the, txSlot* instance, txInteger index);
extern txSlot* fxNewArgumentsStrictInstance(txMachine* the, txInteger count);

/* xsDataView.c */
mxExport void fxArrayBuffer(txMachine* the, txSlot* slot, void* data, txInteger byteLength);
mxExport void fxGetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength);
mxExport txInteger fxGetArrayBufferLength(txMachine* the, txSlot* slot);
mxExport void fxSetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength);
mxExport void fxSetArrayBufferLength(txMachine* the, txSlot* slot, txInteger byteLength);
mxExport void* fxToArrayBuffer(txMachine* the, txSlot* slot);
extern void fxBuildDataView(txMachine* the);
extern txSlot* fxNewArrayBufferInstance(txMachine* the);
extern txSlot* fxNewDataViewInstance(txMachine* the);
extern txSlot* fxNewTypedArrayInstance(txMachine* the, txTypeDispatch* dispatch);
extern txSlot* fxGetTypedArrayProperty(txMachine* the, txSlot* anInstance, txInteger anIndex);
extern txSlot* fxSetTypedArrayProperty(txMachine* the, txSlot* anInstance, txInteger anIndex);

/* xs6MapSet.c */
extern void fxBuildMapSet(txMachine* the);
extern txSlot* fxNewMapInstance(txMachine* the);
extern txSlot* fxNewSetInstance(txMachine* the);
extern txSlot* fxNewWeakMapInstance(txMachine* the);
extern txSlot* fxNewWeakSetInstance(txMachine* the);

/* xsJSON.c */
extern void fxBuildJSON(txMachine* the);

/* xs6Generator.c */
extern void fxBuildGenerator(txMachine* the);
extern txSlot* fxNewGeneratorInstance(txMachine* the);
extern txSlot* fxNewGeneratorFunctionInstance(txMachine* the, txID name);

/* xs6Promise.c */
extern void fxBuildPromise(txMachine* the);
extern txSlot* fxNewPromiseInstance(txMachine* the);
extern void fxRunPromiseJobs(txMachine* the);

/* xs6Proxy.c */
extern void fxBuildProxy(txMachine* the);
extern txSlot* fxNewProxyInstance(txMachine* the);

extern void fxCallProxy(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments);
extern void fxConstructProxy(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target);
extern txBoolean fxDefineProxyProperty(txMachine* the, txSlot* instance, txInteger id, txSlot* descriptor);
extern void fxEachProxyProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txSlot* instance);
extern void fxEnumerateProxy(txMachine* the, txSlot* instance);
extern txSlot* fxGetProxyOwnProperty(txMachine* the, txSlot* instance, txInteger id);
extern txSlot* fxGetProxyProperty(txMachine* the, txSlot* instance, txInteger id);
extern void fxGetProxyPrototype(txMachine* the, txSlot* instance);
extern txBoolean fxHasProxyProperty(txMachine* the, txSlot* instance, txInteger id);
extern void fxIsProxyExtensible(txMachine* the, txSlot* instance);
extern void fxPreventProxyExtensions(txMachine* the, txSlot* instance);
extern txBoolean fxRemoveProxyProperty(txMachine* the, txSlot* instance, txInteger id);
extern txSlot* fxSetProxyProperty(txMachine* the, txSlot* instance, txInteger id);
extern void fxSetProxyPrototype(txMachine* the, txSlot* instance, txSlot* prototype);
extern void fxStepProxyProperty(txMachine* the, txSlot* target, txFlag flag, txStep step, txSlot* context, txInteger id, txSlot* property);



extern txSlot* fxGetHostProperty(txMachine* the, txSlot* instance, txInteger id);

/* xsChunk.c */
extern void fxBuildChunk(txMachine* the);
extern txSlot* fxNewChunkInstance(txMachine* the);

/* xsProfile.c */
#ifdef mxProfile
extern void fxBeginFunction(txMachine* the, txSlot* function);
extern void fxBeginGC(txMachine* the);
extern void fxEndFunction(txMachine* the, txSlot* function);
extern void fxEndGC(txMachine* the);
extern void fxJumpFrames(txMachine* the, txSlot* from, txSlot* to);
#endif
mxExport txS1 fxIsProfiling(txMachine* the);
mxExport void fxStartProfiling(txMachine* the);
mxExport void fxStopProfiling(txMachine* the);

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

enum {
	XS_NO_STATUS = 0,
	XS_RETURN_STATUS = 1,
	XS_THROW_STATUS = 2,

	XS_NO_HINT = 0,
	XS_NUMBER_HINT = 1,
	XS_STRING_HINT = 2,

	XS_NO_FLAG = 0,
	XS_REQUIRE_FLAG = 1,
	XS_REQUIRE_WEAK_FLAG = 2,

	/* frame flags */
	XS_MARK_FLAG = 1,
	XS_C_FLAG = 2,
	/* ? =  = 4, */
	XS_STEP_INTO_FLAG = 8,
	XS_STEP_OVER_FLAG = 16,
	/* ? = 32, */
	XS_STRICT_FLAG = 64,
	XS_DEBUG_FLAG = 128,

	/* instance flags */
	/* XS_MARK_FLAG = 1, */
	XS_DONT_PATCH_FLAG = 2,
	XS_VALUE_FLAG = 4,
	XS_SHARED_FLAG = 8,
	XS_LEVEL_FLAG = 16,
	/* ? = 32, */
	/* ? = 64, */
	/* XS_DEBUG_FLAG = 128, */

	/* property flags */
	/* XS_MARK_FLAG = 1, */
	/* XS_DONT_DELETE_FLAG = 2, */
	/* XS_DONT_ENUM_FLAG = 4, */
	/* XS_DONT_SET_FLAG = 8 ,  */
	/* ? = 16, */
	/* ? = 32, */
	/* ? = 64, */
	/* XS_DEBUG_FLAG = 128, */

	/* fxEachOwnProperty flags */
	XS_EACH_ENUMERABLE_FLAG = 1,
	XS_EACH_STRING_FLAG = 2,
	XS_EACH_SYMBOL_FLAG = 4,
	XS_STEP_GET_OWN_FLAG = 8,
	XS_STEP_GET_FLAG = 16,
	XS_STEP_DESCRIBE_FLAG = 32,
	XS_STEP_DEFINE_FLAG = 64,

	/* XS_PUT_MEMBER/fxPut flags */
	/* XS_DONT_DELETE_FLAG = 2, */
	/* XS_DONT_ENUM_FLAG = 4, */
	/* XS_DONT_SET_FLAG = 8, */
	/* XS_STATIC_FLAG = 16, */
	/* XS_GETTER_FLAG = 32, */
	/* XS_SETTER_FLAG = 64, */
	XS_ACCESSOR_FLAG = XS_GETTER_FLAG | XS_SETTER_FLAG,

	/* parse flags */
	XS_EVAL_FLAG = 2,
	XS_PROGRAM_FLAG = 4,
	XS_THIS_FLAG = 8,
	XS_TO_C_FLAG = 16,
	/* ? = 32, */
	/* XS_STRICT_FLAG = 64, */
	/* XS_DEBUG_FLAG = 128, */

	XS_GET_ONLY = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG,

	/* collect flags */
	XS_COLLECTING_FLAG = 1,
	XS_TRASHING_FLAG = 2,
	XS_SKIPPED_COLLECT_FLAG = 4,
	XS_HOST_HOOKS_FLAG = 64
};

enum {
	XS_UNINITIALIZED_KIND = -1,
	XS_UNDEFINED_KIND = 0,
	XS_NULL_KIND,
	XS_BOOLEAN_KIND,
	XS_INTEGER_KIND,
	XS_NUMBER_KIND,
	XS_STRING_KIND, // 5
	XS_STRING_X_KIND,
	XS_SYMBOL_KIND,
	
	XS_REFERENCE_KIND,
	
	XS_CLOSURE_KIND, 
	XS_FRAME_KIND, //10

	XS_INSTANCE_KIND,
	
	XS_ARRAY_KIND,
	XS_ARRAY_BUFFER_KIND,
	XS_CALLBACK_KIND,
	XS_CODE_KIND, // 15
	XS_CODE_X_KIND,
	XS_DATE_KIND,
	XS_DATA_VIEW_KIND,
	XS_GLOBAL_KIND,
	XS_HOST_KIND, // 20
	XS_MAP_KIND,
	XS_MODULE_KIND,
	XS_PARAMETERS_KIND,
	XS_PROMISE_KIND,
	XS_PROXY_KIND, // 25
	XS_REGEXP_KIND,
	XS_SET_KIND,
	XS_STAR_KIND,
	XS_TYPED_ARRAY_KIND,
	XS_WEAK_MAP_KIND,  // 30
	XS_WEAK_SET_KIND,
	XS_WITH_KIND,

	XS_ACCESSOR_KIND,
	XS_ENTRY_KIND,
	XS_KEY_KIND,
	XS_KEY_X_KIND,
	XS_LIST_KIND,
	XS_STACK_KIND,
	XS_VAR_KIND,
};

#define mxTry(THE_MACHINE) \
	txJump __JUMP__; \
	__JUMP__.nextJump = (THE_MACHINE)->firstJump; \
	__JUMP__.stack = the->stack; \
	__JUMP__.scope = the->scope; \
	__JUMP__.frame = the->frame; \
	__JUMP__.code = the->code; \
	__JUMP__.flag = 0; \
	(THE_MACHINE)->firstJump = &__JUMP__; \
	if (c_setjmp(__JUMP__.buffer) == 0) {

#define mxCatch(THE_MACHINE) \
		(THE_MACHINE)->firstJump = __JUMP__.nextJump; \
	} \
	else for ( \
		the->stack = __JUMP__.stack, \
		the->scope = __JUMP__.scope, \
		the->frame = __JUMP__.frame, \
		the->code = __JUMP__.code, \
		(THE_MACHINE)->firstJump = __JUMP__.nextJump; \
		(__JUMP__.stack); \
		__JUMP__.stack = NULL)


#ifdef mxDebug
#define mxCheck(THE, THE_ASSERTION) \
	if (!(THE_ASSERTION)) \
		fxCheck(THE, __FILE__,__LINE__)
#else
#define mxCheck(THE, THE_ASSERTION)
#endif

#define mxThrowMessage(_CODE,...) fxThrowMessage(the, C_NULL, 0, _CODE, __VA_ARGS__)

#ifdef mxDebug
#define mxUnknownError(...) fxThrowMessage(the, NULL, 0, XS_UNKNOWN_ERROR, __VA_ARGS__)
#define mxEvalError(...) fxThrowMessage(the, NULL, 0, XS_EVAL_ERROR, __VA_ARGS__)
#define mxRangeError(...) fxThrowMessage(the, NULL, 0, XS_RANGE_ERROR, __VA_ARGS__)
#define mxReferenceError(...) fxThrowMessage(the, NULL, 0, XS_REFERENCE_ERROR, __VA_ARGS__)
#define mxSyntaxError(...) fxThrowMessage(the, NULL, 0, XS_SYNTAX_ERROR, __VA_ARGS__)
#define mxTypeError(...) fxThrowMessage(the, NULL, 0, XS_TYPE_ERROR, __VA_ARGS__)
#define mxURIError(...) fxThrowMessage(the, NULL, 0, XS_URI_ERROR, __VA_ARGS__)
#define mxDebugID(THE_ERROR, THE_FORMAT, THE_ID) ( \
	fxIDToString(the, THE_ID, the->nameBuffer, sizeof(the->nameBuffer)), \
	fxThrowMessage(the, NULL, 0, THE_ERROR, THE_FORMAT, the->nameBuffer) \
)
#else
#define mxUnknownError(...) fxThrowMessage(the, NULL, 0, XS_UNKNOWN_ERROR, __VA_ARGS__)
#define mxEvalError(...) fxThrowMessage(the, NULL, 0, XS_EVAL_ERROR, __VA_ARGS__)
#define mxRangeError(...) fxThrowMessage(the, NULL, 0, XS_RANGE_ERROR, __VA_ARGS__)
#define mxReferenceError(...) fxThrowMessage(the, NULL, 0, XS_REFERENCE_ERROR, __VA_ARGS__)
#define mxSyntaxError(...) fxThrowMessage(the, NULL, 0, XS_SYNTAX_ERROR, __VA_ARGS__)
#define mxTypeError(...) fxThrowMessage(the, NULL, 0, XS_TYPE_ERROR, __VA_ARGS__)
#define mxURIError(...) fxThrowMessage(the, NULL, 0, XS_URI_ERROR, __VA_ARGS__)
#define mxDebugID(THE_ERROR, THE_FORMAT, THE_ID) ( \
	fxIDToString(the, THE_ID, the->nameBuffer, sizeof(the->nameBuffer)), \
	fxThrowMessage(the, NULL, 0, THE_ERROR, THE_FORMAT, the->nameBuffer) \
)
#endif


#ifdef mxDebug
#define mxError(_CODE) \
	fxError(the,__FILE__,__LINE__,_CODE)
#else
#define mxError(_CODE) \
	fxError(the,NULL,0,_CODE)
#endif

#ifdef mxDebug
#define mxIfError(_ERROR) \
	((!(the->scratch.data[0] = (void*)(_ERROR))) || (fxError(the,__FILE__,__LINE__,(xsIntegerValue)(the->scratch.data[0])), 1))
#else
#define mxIfError(_ERROR) \
	((!(the->scratch.data[0] = (void*)(_ERROR))) || (fxError(the,NULL,0,(xsIntegerValue)(the->scratch.data[0])), 1))
#endif

#if mxWindows

#ifdef mxDebug
#define mxElseError(_ASSERTION) \
	((_ASSERTION) || (fxError(the,__FILE__,__LINE__,GetLastError()), 1))
#else
#define mxElseError(_ASSERTION) \
	((_ASSERTION) || (fxError(the,NULL,0,GetLastError()), 1))
#endif

#else

#ifdef mxDebug
#define mxElseError(_ASSERTION) \
	((_ASSERTION) || (fxError(the,__FILE__,__LINE__,errno), 1))
#else
#define mxElseError(_ASSERTION) \
	((_ASSERTION) || (fxError(the,NULL,0,errno), 1))
#endif

#endif

#define mxIsUndefined(THE_SLOT) \
	((THE_SLOT)->kind == XS_UNDEFINED_KIND)
#define mxIsReference(THE_SLOT) \
	((THE_SLOT)->kind == XS_REFERENCE_KIND)
#define mxIsFunction(THE_SLOT) \
	( (THE_SLOT) &&  ((THE_SLOT)->flag & XS_VALUE_FLAG) && (((THE_SLOT)->next->kind == XS_CALLBACK_KIND) || ((THE_SLOT)->next->kind == XS_CODE_KIND) || ((THE_SLOT)->next->kind == XS_CODE_X_KIND)))
#define mxIsArray(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_ARRAY_KIND))
#define mxIsString(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_STRING_KIND) || ((THE_SLOT)->next->kind == XS_STRING_X_KIND))
#define mxIsBoolean(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_BOOLEAN_KIND))
#define mxIsNumber(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_NUMBER_KIND))
#define mxIsDate(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_DATE_KIND))
#define mxIsRegExp(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_REGEXP_KIND))
#define mxIsSymbol(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_SYMBOL_KIND))
#define mxIsHost(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_HOST_KIND))
#define mxIsProxy(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_PROXY_KIND))

#define mxIsStringPrimitive(THE_SLOT) \
	(((THE_SLOT)->kind == XS_STRING_KIND) || ((THE_SLOT)->kind == XS_STRING_X_KIND))
	
#ifdef mxDebug

#define mxPush(THE_SLOT) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = (THE_SLOT).kind, \
	the->stack->value = (THE_SLOT).value)
#define mxPushSlot(THE_SLOT) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = (THE_SLOT)->kind, \
	the->stack->value = (THE_SLOT)->value)
	
#define mxPushBoolean(THE_BOOLEAN) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_BOOLEAN_KIND, \
	the->stack->value.boolean = (THE_BOOLEAN))
#define mxPushClosure(THE_SLOT) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_CLOSURE_KIND, \
	the->stack->value.closure = (THE_SLOT))
#define mxPushInteger(THE_NUMBER) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_INTEGER_KIND, \
	the->stack->value.integer = (THE_NUMBER))
#define mxPushList() \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_LIST_KIND, \
	the->stack->value.list.first = C_NULL, \
	the->stack->value.list.last = C_NULL)
#define mxPushNull() \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_NULL_KIND)
#define mxPushNumber(THE_NUMBER) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_NUMBER_KIND, \
	the->stack->value.number = (THE_NUMBER))
#define mxPushReference(THE_SLOT) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_REFERENCE_KIND, \
	the->stack->value.reference = (THE_SLOT))
#define mxPushString(THE_STRING) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_STRING_KIND, \
	the->stack->value.string = (THE_STRING))
#define mxPushStringC(THE_STRING) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_NULL_KIND, \
	fxCopyStringC(the, the->stack, THE_STRING))
#define mxPushStringX(THE_STRING) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_STRING_X_KIND, \
	the->stack->value.string = (THE_STRING))
#define mxPushUndefined() \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_UNDEFINED_KIND)
#define mxPushUninitialized() \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_UNINITIALIZED_KIND)
#define mxPushUnsigned(THE_NUMBER) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	(THE_NUMBER < 0x7FFFFFFF) ? \
		(the->stack->kind = XS_INTEGER_KIND, \
		the->stack->value.integer = (txInteger)(THE_NUMBER)) \
	: \
		(the->stack->kind = XS_NUMBER_KIND, \
		the->stack->value.number = (txNumber)(THE_NUMBER)) \
	)
#else

#define mxPush(THE_SLOT) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = (THE_SLOT).kind, \
	the->stack->value = (THE_SLOT).value)
#define mxPushSlot(THE_SLOT) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = (THE_SLOT)->kind, \
	the->stack->value = (THE_SLOT)->value)
	
#define mxPushBoolean(THE_BOOLEAN) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_BOOLEAN_KIND, \
	the->stack->value.boolean = (THE_BOOLEAN))
#define mxPushClosure(THE_SLOT) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_CLOSURE_KIND, \
	the->stack->value.closure = (THE_SLOT))
#define mxPushInteger(THE_NUMBER) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_INTEGER_KIND, \
	the->stack->value.integer = (THE_NUMBER))
#define mxPushList() \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_LIST_KIND, \
	the->stack->value.list.first = C_NULL, \
	the->stack->value.list.last = C_NULL)
#define mxPushNull() \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_NULL_KIND)
#define mxPushNumber(THE_NUMBER) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_NUMBER_KIND, \
	the->stack->value.number = (THE_NUMBER))
#define mxPushReference(THE_SLOT) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_REFERENCE_KIND, \
	the->stack->value.reference = (THE_SLOT))
#define mxPushString(THE_STRING) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_STRING_KIND, \
	the->stack->value.string = (THE_STRING))
#define mxPushStringC(THE_STRING) \
	(fxCopyStringC(the, --the->stack, THE_STRING))
#define mxPushStringX(THE_STRING) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_STRING_X_KIND, \
	the->stack->value.string = (THE_STRING))
#define mxPushUndefined() \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_UNDEFINED_KIND)
#define mxPushUninitialized() \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_UNINITIALIZED_KIND)
#define mxPushUnsigned(THE_NUMBER) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	(THE_NUMBER < 0x7FFFFFFF) ? \
		(the->stack->kind = XS_INTEGER_KIND, \
		the->stack->value.integer = (txInteger)(THE_NUMBER)) \
	: \
		(the->stack->kind = XS_NUMBER_KIND, \
		the->stack->value.number = (txNumber)(THE_NUMBER)) \
	)
	
#endif	

#define mxPull(THE_SLOT) \
	((THE_SLOT).value = the->stack->value, \
	(THE_SLOT).kind = (the->stack++)->kind)
#define mxPullSlot(THE_SLOT) \
	((THE_SLOT)->value = the->stack->value, \
	(THE_SLOT)->kind = (the->stack++)->kind)

#define mxPop() \
	(the->stack++)

#define mxArgv(THE_INDEX) (the->frame + 5 + ((the->frame + 5)->value.integer) - (THE_INDEX))
#define mxArgc ((the->frame + 5)->value.integer)
#define mxThis (the->frame + 4)
#define mxFunction (the->frame + 3)
#define mxTarget (the->frame + 2)
#define mxResult (the->frame + 1)
#define mxVarc ((the->frame - 1)->value.integer)
#define mxVarv(THE_INDEX) (the->frame -2 - THE_INDEX)

#define mxFunctionInstanceCode(INSTANCE) 		((INSTANCE)->next)
#define mxFunctionInstanceClosures(INSTANCE) 	((INSTANCE)->next->next)
#define mxFunctionInstanceHome(INSTANCE) 		((INSTANCE)->next->next->next)
#define mxFunctionInstanceModule(INSTANCE) 		((INSTANCE)->next->next->next->next)
#ifdef mxProfile
#define mxFunctionInstanceProfile(INSTANCE) 		((INSTANCE)->next->next->next->next->next)
#define mxFunctionInstanceLength(INSTANCE)		((INSTANCE)->next->next->next->next->next->next)
#else
#define mxFunctionInstanceLength(INSTANCE)		((INSTANCE)->next->next->next->next->next)
#endif

#define mxModuleHost(MODULE) 		((MODULE)->value.reference->next)
#define mxModuleFunction(MODULE) 	((MODULE)->value.reference->next->next)
#define mxModuleTransfers(MODULE) 	((MODULE)->value.reference->next->next->next)
#define mxModuleExports(MODULE) 	((MODULE)->value.reference->next->next->next->next)
#define mxModuleID(MODULE) 			((MODULE)->value.reference->next->next->next->next->next)
#define mxModuleURI(MODULE) 		((MODULE)->value.reference->next->next->next->next->next->next)

#define mxCall(_FUNCTION,_THIS,_COUNT) \
	mxPushInteger(_COUNT); \
	mxPushSlot(_THIS); \
	mxPushSlot(_FUNCTION); \
	fxCall(the)

#define mxCallID(_THIS,_ID,_COUNT) \
	mxPushInteger(_COUNT); \
	mxPushSlot(_THIS); \
	fxCallID(the, _ID)
	
#define mxGetID(_THIS,_ID) \
	mxPushSlot(_THIS); \
	fxGetID(the, _ID)


enum {
	mxGlobalStackIndex,
	mxExceptionStackIndex,
	mxObjectPrototypeStackIndex,
	mxFunctionPrototypeStackIndex,
	mxArrayPrototypeStackIndex,
	mxStringPrototypeStackIndex,
	mxBooleanPrototypeStackIndex,
	mxNumberPrototypeStackIndex,
	mxDatePrototypeStackIndex,
	mxRegExpPrototypeStackIndex,
	mxHostPrototypeStackIndex,
	mxErrorPrototypeStackIndex,
	mxEvalErrorPrototypeStackIndex,
	mxRangeErrorPrototypeStackIndex,
	mxReferenceErrorPrototypeStackIndex,
	mxSyntaxErrorPrototypeStackIndex,
	mxTypeErrorPrototypeStackIndex,
	mxURIErrorPrototypeStackIndex,
	mxSymbolPrototypeStackIndex,
	mxArrayBufferPrototypeStackIndex,
	mxDataViewPrototypeStackIndex,
	mxTypedArrayPrototypeStackIndex,
	mxMapPrototypeStackIndex,
	mxSetPrototypeStackIndex,
	mxWeakMapPrototypeStackIndex,
	mxWeakSetPrototypeStackIndex,
	mxPromisePrototypeStackIndex,
	mxProxyPrototypeStackIndex,

	mxModulePathsStackIndex,
	mxImportingModulesStackIndex,
	mxLoadingModulesStackIndex,
	mxLoadedModulesStackIndex,
	mxResolvingModulesStackIndex,
	mxRunningModulesStackIndex,
	mxRequiredModulesStackIndex,
	mxModulesStackIndex,
	mxPendingJobsStackIndex,
	mxRunningJobsStackIndex,
	mxFilesStackIndex,
	mxBreakpointsStackIndex,
	
	mxHostsStackIndex,
	mxIDsStackIndex,
	mxEmptyCodeStackIndex,
	mxEmptyStringStackIndex,
	mxBooleanStringStackIndex,
	mxDefaultStringStackIndex,
	mxFunctionStringStackIndex,
	mxNumberStringStackIndex,
	mxObjectStringStackIndex,
	mxStringStringStackIndex,
	mxSymbolStringStackIndex,
	mxUndefinedStringStackIndex,
	
	mxGetArgumentFunctionStackIndex,
	mxSetArgumentFunctionStackIndex,
	mxThrowTypeErrorFunctionStackIndex,
	mxEnumeratorFunctionStackIndex,
	mxGeneratorPrototypeStackIndex,
	mxGeneratorConstructorStackIndex,
	mxGeneratorFunctionPrototypeStackIndex,
	mxModulePrototypeStackIndex,
	mxModuleConstructorStackIndex,
	mxTransferPrototypeStackIndex,
	mxTransferConstructorStackIndex,
	mxOnRejectedPromiseFunctionStackIndex,
	mxOnResolvedPromiseFunctionStackIndex,
	mxRejectPromiseFunctionStackIndex,
	mxResolvePromiseFunctionStackIndex,
	mxProxyPropertyGetterStackIndex,
	mxProxyPropertySetterStackIndex,
	mxStringAccessorStackIndex,
	mxUndefinedStackIndex,
	
	mxIteratorPrototypeStackIndex,
	mxArrayEntriesIteratorPrototypeStackIndex,
	mxArrayKeysIteratorPrototypeStackIndex,
	mxArrayValuesIteratorPrototypeStackIndex,
	mxMapEntriesIteratorPrototypeStackIndex,
	mxMapKeysIteratorPrototypeStackIndex,
	mxMapValuesIteratorPrototypeStackIndex,
	mxSetEntriesIteratorPrototypeStackIndex,
	mxSetKeysIteratorPrototypeStackIndex,
	mxSetValuesIteratorPrototypeStackIndex,
	mxStringIteratorPrototypeStackIndex,
	mxTypedArrayEntriesIteratorPrototypeStackIndex,
	mxTypedArrayKeysIteratorPrototypeStackIndex,
	mxTypedArrayValuesIteratorPrototypeStackIndex,
	
	mxParametersPrototypeStackIndex,
	mxArgumentsStrictPrototypeStackIndex,
	
	mxHookInstanceIndex,
	
	mxInitializeRegExpFunctionIndex,
	
	mxStackIndexCount
};

#define mxGlobal the->stackTop[-1 - mxGlobalStackIndex]
#define mxException the->stackTop[-1 - mxExceptionStackIndex]
#define mxObjectPrototype the->stackTop[-1 - mxObjectPrototypeStackIndex]
#define mxFunctionPrototype the->stackTop[-1 - mxFunctionPrototypeStackIndex]
#define mxArrayPrototype the->stackTop[-1 - mxArrayPrototypeStackIndex]
#define mxStringPrototype the->stackTop[-1 - mxStringPrototypeStackIndex]
#define mxBooleanPrototype the->stackTop[-1 - mxBooleanPrototypeStackIndex]
#define mxNumberPrototype the->stackTop[-1 - mxNumberPrototypeStackIndex]
#define mxDatePrototype the->stackTop[-1 - mxDatePrototypeStackIndex]
#define mxRegExpPrototype the->stackTop[-1 - mxRegExpPrototypeStackIndex]
#define mxHostPrototype the->stackTop[-1 - mxHostPrototypeStackIndex]
#define mxErrorPrototype the->stackTop[-1 - mxErrorPrototypeStackIndex]
#define mxEvalErrorPrototype the->stackTop[-1 - mxEvalErrorPrototypeStackIndex]
#define mxRangeErrorPrototype the->stackTop[-1 - mxRangeErrorPrototypeStackIndex]
#define mxReferenceErrorPrototype the->stackTop[-1 - mxReferenceErrorPrototypeStackIndex]
#define mxSyntaxErrorPrototype the->stackTop[-1 - mxSyntaxErrorPrototypeStackIndex]
#define mxTypeErrorPrototype the->stackTop[-1 - mxTypeErrorPrototypeStackIndex]
#define mxURIErrorPrototype the->stackTop[-1 - mxURIErrorPrototypeStackIndex]
#define mxSymbolPrototype the->stackTop[-1 - mxSymbolPrototypeStackIndex]
#define mxArrayBufferPrototype the->stackTop[-1 - mxArrayBufferPrototypeStackIndex]
#define mxDataViewPrototype the->stackTop[-1 - mxDataViewPrototypeStackIndex]
#define mxTypedArrayPrototype the->stackTop[-1 - mxTypedArrayPrototypeStackIndex]
#define mxMapPrototype the->stackTop[-1 - mxMapPrototypeStackIndex]
#define mxSetPrototype the->stackTop[-1 - mxSetPrototypeStackIndex]
#define mxWeakMapPrototype the->stackTop[-1 - mxWeakMapPrototypeStackIndex]
#define mxWeakSetPrototype the->stackTop[-1 - mxWeakSetPrototypeStackIndex]
#define mxPromisePrototype the->stackTop[-1 - mxPromisePrototypeStackIndex]
#define mxProxyPrototype the->stackTop[-1 - mxProxyPrototypeStackIndex]

#define mxModulePaths the->stackTop[-1 - mxModulePathsStackIndex]
#define mxImportingModules the->stackTop[-1 - mxImportingModulesStackIndex]
#define mxLoadingModules the->stackTop[-1 - mxLoadingModulesStackIndex]
#define mxLoadedModules the->stackTop[-1 - mxLoadedModulesStackIndex]
#define mxResolvingModules the->stackTop[-1 - mxResolvingModulesStackIndex]
#define mxRunningModules the->stackTop[-1 - mxRunningModulesStackIndex]
#define mxRequiredModules the->stackTop[-1 - mxRequiredModulesStackIndex]
#define mxModules the->stackTop[-1 - mxModulesStackIndex]
#define mxPendingJobs the->stackTop[-1 - mxPendingJobsStackIndex]
#define mxRunningJobs the->stackTop[-1 - mxRunningJobsStackIndex]
#define mxFiles the->stackTop[-1 - mxFilesStackIndex]
#define mxBreakpoints the->stackTop[-1 - mxBreakpointsStackIndex]

#define mxHosts the->stackTop[-1 - mxHostsStackIndex]
#define mxIDs the->stackTop[-1 - mxIDsStackIndex]
#define mxEmptyCode the->stackTop[-1 - mxEmptyCodeStackIndex]
#define mxEmptyString the->stackTop[-1 - mxEmptyStringStackIndex]
#define mxBooleanString the->stackTop[-1 - mxBooleanStringStackIndex]
#define mxDefaultString the->stackTop[-1 - mxDefaultStringStackIndex]
#define mxFunctionString the->stackTop[-1 - mxFunctionStringStackIndex]
#define mxNumberString the->stackTop[-1 - mxNumberStringStackIndex]
#define mxObjectString the->stackTop[-1 - mxObjectStringStackIndex]
#define mxStringString the->stackTop[-1 - mxStringStringStackIndex]
#define mxSymbolString the->stackTop[-1 - mxSymbolStringStackIndex]
#define mxUndefinedString the->stackTop[-1 - mxUndefinedStringStackIndex]

#define mxGetArgumentFunction the->stackTop[-1 - mxGetArgumentFunctionStackIndex]
#define mxSetArgumentFunction the->stackTop[-1 - mxSetArgumentFunctionStackIndex]
#define mxThrowTypeErrorFunction the->stackTop[-1 - mxThrowTypeErrorFunctionStackIndex]
#define mxEnumeratorFunction the->stackTop[-1 - mxEnumeratorFunctionStackIndex]
#define mxGeneratorPrototype the->stackTop[-1 - mxGeneratorPrototypeStackIndex]
#define mxGeneratorConstructor the->stackTop[-1 - mxGeneratorConstructorStackIndex]
#define mxGeneratorFunctionPrototype the->stackTop[-1 - mxGeneratorFunctionPrototypeStackIndex]
#define mxModulePrototype the->stackTop[-1 - mxModulePrototypeStackIndex]
#define mxModuleConstructor the->stackTop[-1 - mxModuleConstructorStackIndex]
#define mxTransferPrototype the->stackTop[-1 - mxTransferPrototypeStackIndex]
#define mxTransferConstructor the->stackTop[-1 - mxTransferConstructorStackIndex]
#define mxOnRejectedPromiseFunction the->stackTop[-1 - mxOnRejectedPromiseFunctionStackIndex]
#define mxOnResolvedPromiseFunction the->stackTop[-1 - mxOnResolvedPromiseFunctionStackIndex]
#define mxRejectPromiseFunction the->stackTop[-1 - mxRejectPromiseFunctionStackIndex]
#define mxResolvePromiseFunction the->stackTop[-1 - mxResolvePromiseFunctionStackIndex]
#define mxProxyPropertyGetter the->stackTop[-1 - mxProxyPropertyGetterStackIndex]
#define mxProxyPropertySetter the->stackTop[-1 - mxProxyPropertySetterStackIndex]
#define mxStringAccessor the->stackTop[-1 - mxStringAccessorStackIndex]
#define mxUndefined the->stackTop[-1 - mxUndefinedStackIndex]

#define mxIteratorPrototype the->stackTop[-1 - mxIteratorPrototypeStackIndex]
#define mxArrayEntriesIteratorPrototype the->stackTop[-1 - mxArrayEntriesIteratorPrototypeStackIndex]
#define mxArrayKeysIteratorPrototype the->stackTop[-1 - mxArrayKeysIteratorPrototypeStackIndex]
#define mxArrayValuesIteratorPrototype the->stackTop[-1 - mxArrayValuesIteratorPrototypeStackIndex]
#define mxMapEntriesIteratorPrototype the->stackTop[-1 - mxMapEntriesIteratorPrototypeStackIndex]
#define mxMapKeysIteratorPrototype the->stackTop[-1 - mxMapKeysIteratorPrototypeStackIndex]
#define mxMapValuesIteratorPrototype the->stackTop[-1 - mxMapValuesIteratorPrototypeStackIndex]
#define mxSetEntriesIteratorPrototype the->stackTop[-1 - mxSetEntriesIteratorPrototypeStackIndex]
#define mxSetKeysIteratorPrototype the->stackTop[-1 - mxSetKeysIteratorPrototypeStackIndex]
#define mxSetValuesIteratorPrototype the->stackTop[-1 - mxSetValuesIteratorPrototypeStackIndex]
#define mxStringIteratorPrototype the->stackTop[-1 - mxStringIteratorPrototypeStackIndex]
#define mxTypedArrayEntriesIteratorPrototype the->stackTop[-1 - mxTypedArrayEntriesIteratorPrototypeStackIndex]
#define mxTypedArrayKeysIteratorPrototype the->stackTop[-1 - mxTypedArrayKeysIteratorPrototypeStackIndex]
#define mxTypedArrayValuesIteratorPrototype the->stackTop[-1 - mxTypedArrayValuesIteratorPrototypeStackIndex]

#define mxParametersPrototype the->stackTop[-1 - mxParametersPrototypeStackIndex]
#define mxArgumentsStrictPrototype the->stackTop[-1 - mxArgumentsStrictPrototypeStackIndex]

#define mxHookInstance the->stackTop[-1 - mxHookInstanceIndex]
#define  mxInitializeRegExpFunction the->stackTop[-1 - mxInitializeRegExpFunctionIndex]

#define mxErrorPrototypes(THE_ERROR) (the->stackTop[-mxErrorPrototypeStackIndex-(THE_ERROR)])


#define mxID(_ID) (((txID*)(mxIDs.value.code))[_ID])

#ifdef __cplusplus
}
#endif

#endif /* __XS6ALL__ */
