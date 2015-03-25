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

#ifndef __XSALL__
#define __XSALL__

//#define mxFrequency 1
#define mxRegExp 1
#define mxOptimize 1
#define mxStringLength 0		/* write string length to code byte list */
#define mxStringAccelerator 1	/* enable/disable XS_STING process in xsAccelerator.c */
#define mxUseApHash 1			/* enable/disable Ap_hash function */
#define mxCmpStr 1				/* compare symbol name or not */

#if mxUseApHash
#define symbolEncodeLength 4
#else
#define symbolEncodeLength 0
#undef mxCmpStr
#define mxCmpStr 1
#endif

#if mxStringLength
#define stringCommandOffset 2
#else
#define	stringCommandOffset 0
#endif


#include "xsPlatform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef txS1 txByte;
typedef txU1 txFlag;
typedef txS2 txID;
typedef txU4 txIndex;
typedef txU1 txKind;
typedef txS4 txSize;
typedef txS4 txError;

typedef txS4 txBoolean;
typedef txS4 txInteger;
typedef double txNumber;
typedef char* txString;
typedef txU4 txUnsigned;

typedef struct sxBlock txBlock;
typedef struct sxChunk txChunk;
typedef struct sxSlot txSlot;

typedef struct sxPatternPart txPatternPart;
typedef struct sxPatternData txPatternData;
typedef struct sxRuleData txRuleData;

typedef struct sxJump txJump;

typedef struct sxProfileRecord txProfileRecord;
typedef struct sxMachine txMachine;
typedef struct sxAllocation txAllocation;

typedef struct sxMarkupCallbacks txMarkupCallbacks;
typedef struct sxGrammar txGrammar;
typedef struct sxModule txModule;

typedef void (*txCallback)(txMachine*);
typedef void (*txDestructor)(void*);
typedef int (*txGetter)(void*);
typedef void (*txMarker)(void*, void (*)(txMachine*, txSlot*));
typedef int (*txPutter)(txString, void*);
typedef txByte* (*txRemapIDsHook)(txMachine*, txByte*, txID*);
typedef txByte* (*txRunLoopHook)(txMachine*, txByte*, txSlot**, txJump*);
typedef void (*txStep)(txMachine*, txSlot*, txID, txSlot*);

typedef txS2 txToken;
typedef txS2 txTokenFlag;
typedef struct sxScriptKeyword txScriptKeyword;
typedef struct sxScriptParser txScriptParser;
typedef void(*RunLoop)(txMachine* the, txSlot** theRoute, txJump* theJump);
typedef txID(*Accelerator)(txMachine* the, txSlot** theRoute, txJump* theJump,txSlot **resProperty);

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

struct sxPatternPart {
	txID namespaceID;
	txID nameID;
};

typedef struct  {
	txDestructor destructor;
	txMarker marker;
} txHostHooks;

typedef union {
	txSlot* reference;
	txString string;
	txBoolean boolean;
	txNumber number;
	txInteger integer;

	txID alias;
	txByte* code;

	struct {
		void* code;
		txInteger* offsets;
	} regexp;

	struct {
		txCallback address;
		txInteger length;
	} callback;

	struct {
		txSlot* getter;
		txSlot* setter;
	} accessor;

	struct {
		txIndex length;
		txSlot* address;
	} array;

	struct {
		txSlot** cache;
		txSlot** sandboxCache;
	} global;

	struct {
		void* data;
		union {
			txDestructor destructor;
			txHostHooks* hooks;
		} variant;
	} host;

	struct {
		txSlot* garbage;
		txSlot* prototype;
	} instance;

	struct {
		txSlot* link;
		txSlot* symbol;
	} label;

	struct {
		txSlot* first;
		txSlot* last;
	} list;

	struct {
		txInteger index;
#if (defined(__GNUC__) && defined(__LP64__)) || (defined(_MSC_VER) && defined(_M_X64))
		// Just because this member can be echanged with 'list', must be same size with it
		txInteger dummy;
#endif
		txSlot* symbol;
	} local;

	struct {
		txString string;
		txSize sum;
	} symbol;

	struct {
		txByte* code;
		txSlot* scope;
	} frame;

	struct {
		txByte* code;
		txSlot* stack;
	} route;

	txPatternData* pattern;

	struct {
		txPatternPart part;
		txSlot* link;
	} node;

	struct {
		txPatternPart part;
		txString string;
	} prefix;

	struct {
		txPatternPart part;
		txRuleData* data;
	} rule;
	
	struct {
		txSlot* reference;
		txByte* symbolMap;
	} closure;
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
	txByte* code; /* xs.h */
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

struct sxPatternData {
#ifdef mxDebug
	txID pathID;
	txID line;
#endif
	txID alias;
	txID count;
	txPatternPart parts[1];
};

struct sxRuleData {
#ifdef mxDebug
	txID pathID;
	txID line;
#endif
	txSlot* link;
	txID alias;
	txID count;
	txID IDs[1];
};

struct sxScriptKeyword {
	char text[30];
	txToken token;
};

struct sxScriptParser {
	txMachine* the;
	void* stream;
	txGetter getter;
	txSlot* path;

	int character;
#ifdef mxColor
	int offset;
	int size;
	void* stream2;
	txPutter putter;
#endif

	int line;
    int lineFirstError;
#ifdef mxColor
	int startOffset;
	int stopOffset;
#endif
	int crlf;
	int escaped;
	txSlot* flags;
	txInteger integer;
	txNumber number;
	txSlot* string;
	txSlot* symbol;
	txToken token;

	int line2;
#ifdef mxColor
	int startOffset2;
	int stopOffset2;
#endif
	int crlf2;
	int escaped2;
	txSlot* flags2;
	txInteger integer2;
	txNumber number2;
	txSlot* string2;
	txSlot* symbol2;
	txToken token2;

	int line0;
	int debug;
	int errorCount;
	int warningCount;
	int level;
	int sandbox;

	txSlot* breakLabel;
	txSlot* continueLabel;
	txSlot* continueSymbol;
	txSlot* returnLabel;
	txSlot* throwLabel;

	txSlot* arguments;
	txSlot* variables;
	txSlot* functions;

	txBoolean strict;
	txBoolean parameters;

	char buffer[32 * 1024];			// must be last
};

struct sxProfileRecord {
	txInteger delta;
	txInteger profileID;
};

struct sxMachine {
	txSlot* stack; /* xs.h */
	txSlot* stackBottom; /* xs.h */
	txSlot* stackTop; /* xs.h */
	txSlot* scope; /* xs.h */
	txSlot* frame; /* xs.h */
	txJump* firstJump; /* xs.h */
	void* context; /* xs.h */
	txByte* code; /* xs.h */
	txSlot scratch; /* xs.h */
#if __FSK_LAYER__
	FskInstrumentedItemDeclaration	/* xs.h */
#endif

	txBlock* firstBlock;

	txSlot* freeHeap;
	txSlot* firstHeap;

	txSize symbolModulo;
	txSlot** symbolTable;

	txID symbolCount;
	txID symbolIndex;
	txSlot** symbolArray;
#if mxOptimize
	int	symbolOffset;
#endif

	txID aliasCount;
	txID aliasIndex;
	txSlot** aliasArray;
	txSlot** linkArray;

	txFlag status;

	txID argumentsID;
	txID bodyID;
	txID calleeID;
	txID callerID;
	txID chunkID;
	txID configurableID;
	txID constructorID;
	txID enumerableID;
	txID evalID;
	txID execID;
	txID getID;
	txID globalID;
	txID grammarID;
	txID grammarsID;
	txID indexID;
	txID inputID;
	txID instanceID;
	txID instancesID;
	txID lengthID;
	txID lineID;
	txID nameID;
	txID namespaceID;
	txID parentID;
	txID parseID;
	txID pathID;
	txID patternsID;
	txID peekID;
	txID pokeID;
	txID prefixID;
	txID prefixesID;
	txID prototypeID;
	txID rootsID;
	txID sandboxID;
	txID serializeID;
	txID setID;
	txID toJSONID;
	txID toStringID;
	txID valueID;
	txID valueOfID;
	txID writableID;

	txID attributeID;
	txID cdataID;
	txID childrenID;
	txID commentID;
	txID compareAttributesID;
	txID elementID;
	txID piID;
	txID pushID;
	txID sortID;
	txID xmlnsAttributesID;
	txID xmlnsNamespaceID;
	txID xmlnsPrefixID;
	txID _attributesID;
	txID __xs__infosetID;

	txSlot* cRoot;

	txSize currentChunksSize;
	txSize peakChunksSize;
	txSize maximumChunksSize;
	txSize minimumChunksSize;

	txSize currentHeapCount;
	txSize peakHeapCount;
	txSize maximumHeapCount;
	txSize minimumHeapCount;

	txFlag parseFlag;
	txBoolean hacked;
	txBoolean shared;
	txMachine* sharedMachine;

	txBoolean collectFlag;
	void* dtoa;

	txScriptParser* parser;
#ifdef mxDebug
	txString echoBuffer;
	txInteger echoOffset;
	txInteger echoSize;
	txSocket connection;
	txString name;
	txSlot** sorter;
	txBoolean breakOnExceptionFlag;
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

#ifdef mxFrequency
	long fastFrequencies[256];
	long fullFrequencies[256];
#endif
	Accelerator accelerator;
	RunLoop		runloop;
};

struct sxAllocation {
	txSize initialChunkSize; /* xs.h */
	txSize incrementalChunkSize; /* xs.h */
	txSize initialHeapCount; /* xs.h */
	txSize incrementalHeapCount; /* xs.h */
	txSize stackCount; /* xs.h */
	txSize symbolCount; /* xs.h */
	txSize symbolModulo; /* xs.h */
};

struct sxMarkupCallbacks {
	void (*processStartTag)(txMachine*); /* xs.h */
	void (*processStopTag)(txMachine*); /* xs.h */
	void (*processText)(txMachine*); /* xs.h */
	void (*lookupEntity)(txMachine*); /* xs.h */
	void (*processComment)(txMachine*); /* xs.h */
	void (*processProcessingInstruction)(txMachine*); /* xs.h */
	void (*processDoctype)(txMachine*); /* xs.h */
};

struct sxGrammar {
	txCallback callback; /* xs.h */
	txByte* symbols; /* xs.h */
	txInteger symbolsSize; /* xs.h */
	txByte* code; /* xs.h */
	txInteger codeSize; /* xs.h */
	txString name; /* xs.h */
};

struct sxModule {
	txModule* next;
	txString name;
	txCallback callback;
};

typedef struct {
	txSlot* stack;
	txPutter putter;
	void* stream;
	txString buffer;
	txSize size;
	txSize offset;
	txSlot* root;
} txSerializer;

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

typedef struct {
	txS4 size;
	txU4 cmask;
	txU4 cval;
	txS4 shift;
	txU4 lmask;
	txU4 lval;
} txUTF8Sequence;


#ifndef mxCleanPath
    #define mxCleanPath(path) path
#endif

/* xsAPI.c */

mxExport txKind fxTypeOf(txMachine*, txSlot*);

mxExport void fxUndefined(txMachine*, txSlot*);
mxExport void fxNull(txMachine*, txSlot*);
mxExport void fxBoolean(txMachine*, txSlot*, txS1);
mxExport txS1 fxToBoolean(txMachine*, txSlot*);
mxExport void fxInteger(txMachine*, txSlot*, txInteger);
mxExport txInteger fxToInteger(txMachine*, txSlot*);
mxExport void fxNumber(txMachine*, txSlot*, txNumber);
mxExport txNumber fxToNumber(txMachine*, txSlot*);
mxExport void fxString(txMachine*, txSlot*, txString);
mxExport void fxStringBuffer(txMachine* the, txSlot* theSlot, txString theValue, txSize theSize);
mxExport txString fxToString(txMachine*, txSlot*);
mxExport txString fxToStringBuffer(txMachine*, txSlot*, txString, txSize);
mxExport void fxUnsigned(txMachine*, txSlot*, txUnsigned);
mxExport txUnsigned fxToUnsigned(txMachine*, txSlot*);

mxExport void fxNewInstanceOf(txMachine*);
mxExport txBoolean fxIsInstanceOf(txMachine*);

mxExport void fxNewHostConstructor(txMachine*, txCallback, txInteger);
mxExport void fxNewHostFunction(txMachine*, txCallback, txInteger);
mxExport void fxNewHostInstance(txMachine* the);
mxExport void fxNewHostObject(txMachine*, txDestructor);
mxExport void* fxGetHostData(txMachine*, txSlot*);
mxExport txDestructor fxGetHostDestructor(txMachine*, txSlot*);
mxExport txHostHooks* fxGetHostHooks(txMachine*, txSlot*);
mxExport void fxSetHostData(txMachine*, txSlot*, void*);
mxExport void fxSetHostDestructor(txMachine*, txSlot*, txDestructor);
mxExport void fxSetHostHooks(txMachine*, txSlot*, txHostHooks*);

mxExport txID fxID(txMachine*, txString);
mxExport txID fxFindID(txMachine* the, txString theName);
mxExport txS1 fxIsID(txMachine*, txString);
mxExport txString fxName(txMachine*, txID);

mxExport txS1 fxHasID(txMachine*, txID);
mxExport txS1 fxHasOwnID(txMachine*, txID);
mxExport void fxGet(txMachine* the, txSlot* theProperty, txID theID);
mxExport void fxGetAt(txMachine*);
mxExport void fxGetID(txMachine*, txID);
mxExport void fxSet(txMachine* the, txSlot* theProperty, txID theID);
mxExport void fxSetAt(txMachine*);
mxExport void fxSetID(txMachine*, txID);
mxExport void fxDeleteAt(txMachine*);
mxExport void fxDeleteID(txMachine*, txID);
mxExport void fxCall(txMachine*);
mxExport void fxCallID(txMachine*, txID);
mxExport void fxNew(txMachine*);
mxExport void fxNewID(txMachine*, txID);

mxExport void fxVars(txMachine*, txInteger);

mxExport txInteger fxCheckArg(txMachine*, txInteger);
mxExport txInteger fxCheckVar(txMachine*, txInteger);
mxExport void fxOverflow(txMachine*, txInteger, txString thePath, txInteger theLine);
mxExport void fxThrow(txMachine* the, txString thePath, txInteger theLine) XS_FUNCTION_ANALYZER_NORETURN;
mxExport void fxError(txMachine* the, txString thePath, txInteger theLine, txInteger theCode) XS_FUNCTION_ANALYZER_NORETURN;
mxExport void fxErrorPrintf(txMachine* the, txString thePath, txInteger theLine, txString theMessage) XS_FUNCTION_ANALYZER_NORETURN;
mxExport void fxDebugger(txMachine* the, txString thePath, txInteger theLine);
mxExport void fxTrace(txMachine* the, txString theString);

mxExport txMachine* fxNewMachine(txAllocation*, txGrammar*, void*);
mxExport void fxDeleteMachine(txMachine*);
mxExport txMachine* fxAliasMachine(txAllocation* theAllocation, txMachine* theMachine, txString theName, void* theContext);
mxExport void fxShareMachine(txMachine* the);

mxExport txS1 fxBuildHost(txMachine*, txCallback);
mxExport txMachine* fxBeginHost(txMachine*);
mxExport void fxEndHost(txMachine*);
mxExport void fxPutAt(txMachine*, txFlag, txFlag);
mxExport void fxPutID(txMachine*, txID, txFlag, txFlag);

mxExport void fxCollectGarbage(txMachine*);
mxExport void fxEnableGarbageCollection(txMachine* the, txBoolean enableIt);
mxExport void fxRemember(txMachine*, txSlot*);
mxExport void fxForget(txMachine*, txSlot*);
mxExport void fxAccess(txMachine*, txSlot*);

mxExport void fxNewFunction(txMachine* the, txString arguments, txInteger argumentsSize, txString body, txInteger bodySize, txFlag theFlag, txString thePath, txInteger theLine);
mxExport txBoolean fxExecute(txMachine* the, void* theStream, txGetter theGetter,
		txString thePath, long theLine);

mxExport void fxScan(txMachine* the, void* theStream, txGetter theGetter,
		txString thePath, long theLine, txMarkupCallbacks* theCallbacks);
mxExport void fxScanBuffer(txMachine* the, void* theBuffer, txSize theSize,
		txString thePath, long theLine, txMarkupCallbacks* theCallbacks);

mxExport void fxLink(txMachine*, txGrammar*);
mxExport void fxParse(txMachine* the, void* theStream, txGetter theGetter,
		txString thePath, long theLine, txFlag theFlag);
mxExport void fxParseBuffer(txMachine* the, void* theBuffer, txSize theSize,
		txString thePath, long theLine, txFlag theFlag);
mxExport void fxSerialize(txMachine* the, void* theStream, txPutter thePutter);
mxExport void fxSerializeBuffer(txMachine* the, void* theBuffer, txSize theSize);

mxExport void fxEnterSandbox(txMachine* the);
mxExport void fxLeaveSandbox(txMachine* the);
mxExport void fxSandbox(txMachine* the);
mxExport txInteger fxScript(txMachine* the);
mxExport txString fxCurrentFile(txMachine* the);
mxExport txCallback fxGetHostModule(txString theName);
mxExport void fxSetHostModule(txString theName, txCallback theCallback);

mxExport void fxModuleURL(txMachine* the);
mxExport void fxHostScope(txMachine* the, txIndex theDepth);
mxExport void fxNewHostClosure(txMachine* the, txBoolean theFlag);
mxExport void fxNewHostSymbolMap(txMachine* the, txInteger theCount, txString* theStrings);
mxExport void fxCopyObject(txMachine* the);

/* xsPlatform.c */
extern void fxErrorMessage(txMachine* the, txInteger theCode, txString theBuffer, txSize theSize);
extern txString fxIntegerToString(txInteger theValue, txString theBuffer, txSize theSize);
extern txString fxStringToUpper(txMachine* the, txString theString);
extern txString fxStringToLower(txMachine* the, txString theString);
#ifdef mxDebug
extern void fxConnect(txMachine* the);
extern void fxDisconnect(txMachine* the);
extern char *fxGetAddress(txMachine* the);
extern txBoolean fxGetAutomatic(txMachine* the);
extern txBoolean fxIsConnected(txMachine* the);
extern void fxReadBreakpoints(txMachine* the);
extern void fxReceive(txMachine* the);
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
extern txUTF8Sequence gxUTF8Sequences[];

extern txString fxConcatString(txMachine* the, txSlot* a, txSlot* b);
extern txString fxConcatStringC(txMachine* the, txSlot* a, txString b);
extern txString fxCopyString(txMachine* the, txSlot* a, txSlot* b);
extern txString fxCopyStringC(txMachine* the, txSlot* a, txString b);
extern txString fxResizeString(txMachine* the, txSlot* a, txSize theSize);
extern int fxStringGetter(void*);
extern int fxStringCGetter(void*);
extern void fxJump(txMachine*) XS_FUNCTION_ANALYZER_NORETURN;
mxExport txInteger fxUnicodeCharacter(txString theString);
mxExport txInteger fxUnicodeLength(txString theString);
mxExport txInteger fxUnicodeToUTF8Offset(txString theString, txInteger theOffset);
mxExport txInteger fxUTF8ToUnicodeOffset(txString theString, txInteger theOffset);

/* xsMemory.c */
extern void fxAllocate(txMachine* the, txAllocation* theAllocation);
extern void fxCollect(txMachine* the, txBoolean theFlag);
extern txSlot* fxDuplicateSlot(txMachine* the, txSlot* theSlot);
extern void fxFree(txMachine* the);
extern void* fxNewChunk(txMachine* the, txSize theSize);
extern txSlot* fxNewSlot(txMachine* the);
extern void* fxRenewChunk(txMachine* the, void* theData, txSize theSize);
extern void fxShare(txMachine* the);

/* xsDebug.c */
#ifdef mxDebug
extern void fxCheck(txMachine* the, txString thePath, txInteger theLine);
extern void fxClearAllBreakpoints(txMachine* the);
extern void fxClearBreakpoint(txMachine* the, txString thePath, txString theLine);
extern void fxDebug(txMachine* the, txError theError, txString theFormat, ...) XS_FUNCTION_ANALYZER_NORETURN;       //@@
extern void fxDebugID(txMachine* the, txError theError, txString theFormat, txID theID) XS_FUNCTION_ANALYZER_NORETURN;
extern void fxDebugLine(txMachine* the);
extern void fxDebugLoop(txMachine* the, txString theFormat, ...);
extern void fxDebugThrow(txMachine* the, txString theFormat, ...);
extern txSlot* fxGetNextBreakpoint(txMachine* the, txSlot* theBreakpoint, txString thePath, txString theLine);
extern void fxLogin(txMachine* the);
extern void fxLogout(txMachine* the);
extern void fxSetBreakpoint(txMachine* the, txString thePath, txString theLine);
#endif
extern txSlot* fxNewFile(txMachine* the, txSlot* theSlot);
extern txSlot* fxNewFileC(txMachine* the, txString thePath);
extern void fxReport(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...);
extern void fxReportError(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...);
extern void fxReportWarning(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...);
extern void fxVReport(txMachine* the, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments);
extern void fxVReportError(txMachine* the, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments);
extern void fxVReportWarning(txMachine* the, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments);

/* xsScript.c */
extern txByte* fxParseScript(txMachine* the, void* theStream, txGetter theGetter,
		txSlot* thePath, long theLine, txFlag theFlags, txSize* theSize);
extern void fxParseJSON(txMachine* the, void* theStream, txGetter theGetter);
extern void fxBuildJSON(txMachine* the);
#ifdef mxToC
extern void fxBuildTree(txMachine* the, txString thePath);
extern txByte* fxParseTree(txMachine* the, void* theStream, txGetter theGetter,
		txSlot* thePath, long theLine, txFlag theFlags, txSize* theSize);
#endif

/* xsRun.c */
void fxRunID(txMachine* the, txID theID);
void fxRunLoop(txMachine* the, txSlot** theRoute, txJump* theJump);
void fxRemapIDs(txMachine* the, txByte* theCode, txID* theIDs);
#ifdef mxFrequency
void fxReportFrequency(txMachine* the);
#endif
extern txBoolean fxIsSameSlot(txMachine* the, txSlot* a, txSlot* b);
mxExport txRemapIDsHook fxGetRemapIDsHook();
mxExport txRunLoopHook fxGetRunLoopHook();
mxExport void fxSetRemapIDsHook(txRemapIDsHook);
mxExport void fxSetRunLoopHook(txRunLoopHook);

mxExport void fxRunAdd(txMachine* the);
mxExport void fxRunArguments(txMachine* the);
mxExport txBoolean fxRunCompare(txMachine* the, txBoolean testing, txBoolean theLess, txBoolean theEqual, txBoolean theMore);
mxExport void fxRunDelta(txMachine* the, txInteger theDelta);
mxExport txBoolean fxRunEqual(txMachine* the, txBoolean testing, txBoolean yes, txBoolean no);
mxExport void fxRunForIn(txMachine* the);
mxExport txBoolean fxRunIn(txMachine* the, txBoolean testing);
mxExport txBoolean fxRunInstanceof(txMachine* the, txBoolean testing);
mxExport void fxRunSign(txMachine* the, txInteger theSign);
mxExport txBoolean fxRunStrictEqual(txMachine* the, txBoolean testing, txBoolean yes, txBoolean no);
mxExport void fxRunSubtract(txMachine* the);
mxExport txBoolean fxRunTest(txMachine* the);
mxExport void fxRunTypeof(txMachine* the);

/* xsSymbol.c */
extern txSlot* fxGetSymbol(txMachine* the, txID theID);
extern txSlot* fxFindSymbol(txMachine* the, txString theString);
extern txSlot* fxNewSymbol(txMachine* the, txSlot* theSlot);
extern txSlot* fxNewSymbolC(txMachine* the, txString theString);
extern txSlot* fxNewSymbolLink(txMachine* the, txString theString, txSize aSum);

/* xsType.c */
mxExport void fxAliasInstance(txMachine* the, txSlot* theSlot);
extern txSlot* fxGetInstance(txMachine* the, txSlot* theSlot);
extern txSlot* fxGetParent(txMachine* the, txSlot* theSlot);
extern txSlot* fxGetOwnInstance(txMachine* the, txSlot* theSlot);
extern void fxLookupInstance(txMachine* the, txSlot* theSlot);
extern void fxNewInstance(txMachine* the);
extern txSlot* fxToInstance(txMachine* the, txSlot* theSlot);
extern txSlot* fxToOwnInstance(txMachine* the, txSlot* theSlot);
extern void fxToPrimitive(txMachine* the, txSlot* theSlot, txBoolean theHint);

/* xsProperty.c */
extern void fxQueueID(txMachine*, txID, txFlag);

void fxEachOwnProperty(txMachine*, txSlot*, txFlag, txStep, txSlot*);

extern txSlot* fxGetOwnProperty(txMachine* the, txSlot* theInstance, txID theID);
extern txSlot* fxGetProperty(txMachine* the, txSlot* theInstance, txID theID);
extern txBoolean fxRemoveProperty(txMachine* the, txSlot* theInstance, txID theID);
extern txSlot* fxSetProperty(txMachine* the, txSlot* theInstance, txID theID, txFlag* theFlag);

extern txSlot* fxGetOwnPropertyAt(txMachine* the, txSlot* theInstance, txSlot* theSlot, txID* theID);
extern txSlot* fxGetPropertyAt(txMachine* the, txSlot* theInstance, txSlot* theSlot, txID* theID);
extern txBoolean fxRemovePropertyAt(txMachine* the, txSlot* theInstance, txSlot* theSlot, txID* theID);
extern txSlot* fxSetPropertyAt(txMachine* the, txSlot* theInstance, txSlot* theSlot, txID* theID, txFlag* theFlag);

extern txFlag fxIntegerToIndex(txMachine* the, txInteger theInteger, txIndex* theIndex);
extern txFlag fxNumberToIndex(txMachine* the, txNumber theNumber, txIndex* theIndex);
extern txFlag fxStringToIndex(txMachine* the, txString theString, txIndex* theIndex);

/* xsGlobal.c */
extern void fxBuildGlobal(txMachine* the);
mxExport void fxDecodeURI(txMachine* the, txString theSet);
mxExport void fxEncodeURI(txMachine* the, txString theSet);

/* xsObject.c */
extern void fxBuildObject(txMachine* the);
extern txSlot* fxNewObjectInstance(txMachine* the);

/* xsFunction.c */
extern void fxBuildFunction(txMachine* the);
extern txSlot* fxNewArgumentsInstance(txMachine* the);
extern txSlot* fxNewFunctionInstance(txMachine* the);
extern void fxNewProgramInstance(txMachine* the);

/* xsArray.c */
extern void fxBuildArray(txMachine* the);
extern void fxCacheArray(txMachine* the, txSlot* theArray);
extern txSlot* fxQueueItem(txMachine* the, txSlot* theArray);
extern txSlot* fxNewArrayInstance(txMachine* the);

/* xsString.c */
extern void fxBuildString(txMachine* the);
extern txSlot* fxNewStringInstance(txMachine* the);

/* xsBoolean.c */
extern void fxBuildBoolean(txMachine* the);
extern txSlot* fxNewBooleanInstance(txMachine* the);

/* xsNumber.c */
extern void fxBuildNumber(txMachine* the);
extern txSlot* fxNewNumberInstance(txMachine* the);

/* xsDate.c */
extern void fxBuildDate(txMachine* the);
extern txSlot* fxNewDateInstance(txMachine* the);

/* xsMath.c */
extern void fxBuildMath(txMachine* the);

/* xsRegExp.c */
extern void fxBuildRegExp(txMachine* the);
extern txInteger fxExecuteRegExp(txMachine* the, txSlot* theRegExp, txSlot* theString, txInteger theOffset);
extern txSlot* fxNewRegExpInstance(txMachine* the);

/* xsError.c */
extern void fxBuildError(txMachine* the);
extern void fxThrowError(txMachine* the, txError theError) XS_FUNCTION_ANALYZER_NORETURN;
extern void fxThrowMessage(txMachine* the, txError theError, txString theMessage) XS_FUNCTION_ANALYZER_NORETURN;

/* xsChunk.c */
extern void fxBuildChunk(txMachine* the);
extern txSlot* fxNewChunkInstance(txMachine* the);

/* xsMarkup.c */
extern void fxParseMarkup(txMachine* the, void* theStream, txGetter theGetter,
		txSlot* thePath, long theLine, txMarkupCallbacks* theCallbacks);
extern void fxParseMarkupBuffer(txMachine* the, void* theBuffer, txSize theSize,
		txSlot* thePath, long theLine, txMarkupCallbacks* theCallbacks);

/* xsGrammar.c */
extern void fxBuildGrammar(txMachine* the);
extern txMarkupCallbacks gxGrammarMarkupCallbacks;
extern void fxProcessRoots(txMachine* the);
extern txString fxSearchPrefix(txMachine* the, txSlot* theRoot, txID theNamespaceID);
extern void fxSerializeRoot(txMachine* the, txSerializer* theSerializer);

/* xsInfoSet.c */
extern void fxBuildInfoSet(txMachine* the);
extern txID fxSearchInfoSetNamespace(txMachine* the, char* thePrefix, int theLength);

/* xs_dtoa.c */
extern void* fxNew_dtoa(void);
extern void fxDelete_dtoa(void*);
extern txString fxNumberToString(txMachine* the, txNumber theValue, txString theBuffer, txSize theSize, txByte theMode, txInteger thePrecision);
extern txNumber fxStringToNumber(txMachine* the, txString theString, txFlag whole);
extern txNumber fxStringToNumberCompile(txMachine* the, txString theString, txFlag whole);

/* xsProfile.c */
#ifdef mxProfile
extern void fxBeginFunction(txMachine* the);
extern void fxBeginGC(txMachine* the);
extern void fxDoCallback(txMachine* the, txCallback address);
extern void fxEndFunction(txMachine* the);
extern void fxEndGC(txMachine* the);
#endif
mxExport txS1 fxIsProfiling(txMachine* the);
mxExport void fxStartProfiling(txMachine* the);
mxExport void fxStopProfiling(txMachine* the);

#define XS_NO_ID -1

enum {
	XS_NO_STATUS = 0,
	XS_RETURN_STATUS = 1,
	XS_THROW_STATUS = 2,

	XS_NUMBER_HINT = 0,
	XS_STRING_HINT = 1,

	XS_NO_FLAG = 0,

	/* frame flags */
	XS_MARK_FLAG = 1,
	XS_C_FLAG = 2,
	XS_THROW_FLAG = 4,
	XS_STEP_INTO_FLAG = 8,
	XS_STEP_OVER_FLAG = 16,
	XS_SANDBOX_FLAG = 32,
	XS_STRICT_FLAG = 64,
	XS_DEBUG_FLAG = 128,

	/* scope flags */
	/* XS_THROW_FLAG = 4, */
	XS_STACK_FLAG = 64,

	/* instance flags */
	/* XS_MARK_FLAG = 1, */
	XS_DONT_PATCH_FLAG = 2,
	XS_VALUE_FLAG = 4,
	XS_SHARED_FLAG = 8,
	XS_LEVEL_FLAG = 16,
	/* XS_SANDBOX_FLAG = 32, */
	/* XS_STACK_FLAG = 64, */
	/* XS_DEBUG_FLAG = 128, */

	/* property flags */
	/* XS_MARK_FLAG = 1, */
	XS_DONT_DELETE_FLAG = 2,
	XS_DONT_ENUM_FLAG = 4,
	XS_DONT_SCRIPT_FLAG = 8,
	XS_DONT_SET_FLAG = 16,
	/* XS_SANDBOX_FLAG = 32, */
	/* XS_STACK_FLAG = 64, */
	/* XS_DEBUG_FLAG = 128, */

	/* XS_PUT_MEMBER/fxPut flags */
	/* XS_DONT_DELETE_FLAG = 2, */
	/* XS_DONT_ENUM_FLAG = 4, */
	/* XS_DONT_SCRIPT_FLAG = 8, */
	/* XS_DONT_SET_FLAG = 16, */
	XS_GETTER_FLAG = 32,
	XS_SETTER_FLAG = 64,
	XS_ACCESSOR_FLAG = XS_GETTER_FLAG | XS_SETTER_FLAG,

	/* parse flags */
	XS_EVAL_FLAG = 2,
	XS_PROGRAM_FLAG = 4,
	XS_THIS_FLAG = 8,
	XS_TO_C_FLAG = 16,
	/* XS_SANDBOX_FLAG = 32, */
	/* XS_STRICT_FLAG = 64, */
	/* XS_DEBUG_FLAG = 128, */

	/* scan flags */
	XS_SOURCE_FLAG = 1,
	XS_NO_ERROR_FLAG = 2,
	XS_NO_WARNING_FLAG = 4,
	XS_SNIFF_FLAG = 8,
	XS_INFO_SET_FLAG = 32,
	XS_NO_MIXTURE_FLAG = 64,
	/* XS_DEBUG_FLAG = 128, */

	XS_GET_ONLY = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG,

	/* collect flags */
	XS_COLLECTING_FLAG = 1,
	XS_TRASHING_FLAG = 2,
	XS_SKIPPED_COLLECT_FLAG = 4,
	XS_HOST_HOOKS_FLAG = 64
};

//PSEUDO_BRANCH_ELSE0
/*
	XS_GET_NEGTIVE_ID
	XS_BRANCH_ELSE
*/

//PSEUDO_GET_MEMBER
/*
	XS_THIS
	XS_GET_MEMBER
	XS_BRANCH_ELSE
*/

//PSEUDO_BRANCH_ELSE1
/*
	XS_GET_AT
	XS_BRANCH_ELSE
*/

//PSEUDO_SET_MEMBER
/*
	XS_THIS
	XS_SET_MEMBER
	XS_POP
*/

//PSEUDO_LESS
/*
	XS_GET_NEGATIVE_ID
	XS_GET_NEGATIVE_ID
	XS_LESS
	XS_BRANCH_ELSE
*/

//PSEUDO_LESS_EQUAL
/*
	XS_GET_NEGATIVE_ID
	XS_GET_NEGATIVE_ID
	XS_LESS_EQUAL
	XS_BRANCH_ELSE
*/

//PSEUDO_INCREMENT
/*
	XS_GET_NEGATIVE_ID
	XS_DUB
	XS_INCREMENT
	XS_SET_NEGATIVE_ID
	XS_POP
	XS_POP
*/

//PSEUDO_DECREMENT
/*
	XS_GET_NEGATIVE_ID
	XS_DUB
	XS_DECREMENT
	XS_SET_NEGATIVE_ID
	XS_POP
	XS_POP
*/

//PSEUDO_DUB
/*
	XS_POP
	XS_DUB
*/

//PSEUDO_DOUBLE_GET_NEGATIVE
/*
	XS_GET_NEGATIVE_ID
	XS_GET_NEGATIVE_ID
*/

//PSEUDO_DOUBLE_GET_AT
/*
	XS_GET_AT
	XS_GET_AT
*/

enum {
	XS_UNDEFINED_KIND = 0,
	XS_NULL_KIND,
	XS_BOOLEAN_KIND,
	XS_INTEGER_KIND,
	XS_NUMBER_KIND,
	XS_REFERENCE_KIND, /* 5 */
	XS_REGEXP_KIND,
	XS_STRING_KIND,
	XS_DATE_KIND,

	XS_ARRAY_KIND,
	XS_CALLBACK_KIND, /* 10 */
	XS_CODE_KIND,
	XS_HOST_KIND,

	XS_SYMBOL_KIND,
	XS_INSTANCE_KIND,

	XS_FRAME_KIND, /* 15 */
	XS_ROUTE_KIND,

	XS_LIST_KIND,
	XS_ACCESSOR_KIND,

	XS_ALIAS_KIND,
	XS_GLOBAL_KIND, /* 20 */

	XS_NODE_KIND,
	XS_PREFIX_KIND,
	XS_ATTRIBUTE_RULE,
	XS_DATA_RULE,
	XS_PI_RULE, /* 25 */
	XS_EMBED_RULE,
	XS_JUMP_RULE,
	XS_REFER_RULE,
	XS_REPEAT_RULE,
	XS_ERROR_RULE, /* 30 */

	XS_LOCAL,
	XS_LOCALS,
	XS_LABEL,

	XS_ADD,
	XS_ALIAS,
	XS_BEGIN,
	XS_BIND,
	XS_BIT_AND,
	XS_BIT_NOT,
	XS_BIT_OR,
	XS_BIT_XOR,
	XS_BRANCH,
	XS_BRANCH_ELSE,
	XS_BRANCH_IF,
	XS_BREAK,
	XS_CALL,
	XS_CATCH,
	XS_DEBUGGER,
	XS_DECREMENT,
	XS_DELETE,
	XS_DELETE_AT,
	XS_DELETE_MEMBER,
	XS_DELETE_MEMBER_AT,
	XS_DIVIDE,
	XS_DUB,
	XS_END,
	XS_ENUM,
	XS_EQUAL,
	XS_FALSE,
	XS_FILE,
	XS_FUNCTION,
	XS_GET,
	XS_GET_AT,
	XS_GET_MEMBER,
	XS_GET_MEMBER_AT,
	XS_GLOBAL,
	XS_IN,
	XS_INCREMENT,
	XS_INSTANCEOF,
	XS_INSTANCIATE,
	XS_INTEGER_8,
	XS_INTEGER_16,
	XS_INTEGER_32,
	XS_JUMP,
	XS_LEFT_SHIFT,
	XS_LESS,
	XS_LESS_EQUAL,
	XS_LINE,
	XS_MINUS,
	XS_MODULO,
	XS_MORE,
	XS_MORE_EQUAL,
	XS_MULTIPLY,
	XS_NEW,
	XS_NOT,
	XS_NOT_EQUAL,
	XS_NULL,
	XS_NUMBER,
	XS_PARAMETERS,
	XS_PLUS,
	XS_POP,
	XS_PUT_MEMBER,
	XS_PUT_MEMBER_AT,
	XS_RESULT,
	XS_RETURN,
	XS_ROUTE,
	XS_SCOPE,
	XS_SET,
	XS_SET_AT,
	XS_SET_MEMBER,
	XS_SET_MEMBER_AT,
	XS_SIGNED_RIGHT_SHIFT,
	XS_STATUS,
	XS_STRICT_EQUAL,
	XS_STRICT_NOT_EQUAL,
	XS_STRING,
	XS_SUBTRACT,
	XS_SWAP,
	XS_THIS,
	XS_THROW,
	XS_TRUE,
	XS_TYPEOF,
	XS_UNCATCH,
	XS_UNDEFINED,
	XS_UNSCOPE,
	XS_UNSIGNED_RIGHT_SHIFT,
	XS_VOID,

	XS_ATTRIBUTE_PATTERN,
	XS_DATA_PATTERN,
	XS_PI_PATTERN,
	XS_EMBED_PATTERN,
	XS_JUMP_PATTERN,
	XS_REFER_PATTERN,
	XS_REPEAT_PATTERN,
	XS_FLAG_INSTANCE,

	XS_BRANCH2,
	XS_BRANCH_ELSE2,
	XS_BRANCH_IF2,
	XS_FUNCTION2,
	XS_ROUTE2,
	XS_GET_NEGATIVE_ID,
	XS_SET_NEGATIVE_ID,
	XS_BRANCH_ELSE_BOOL,
	XS_BRANCH_IF_BOOL,
	XS_BRANCH_ELSE_BOOL2,
	XS_BRANCH_IF_BOOL2,
	XS_STRING_POINTER,
	XS_STRING_CONCAT,
	XS_STRING_CONCATBY,
	PSEUDO_BRANCH_ELSE0,
	PSEUDO_BRANCH_ELSE1,
	PSEUDO_GET_MEMBER,
	PSEUDO_SET_MEMBER,
	PSEUDO_LESS,
	PSEUDO_LESS_EQUAL,
	PSEUDO_INCREMENT,
	PSEUDO_DECREMENT,
	PSEUDO_DUB,
	PSEUDO_DOUBLE_GET_NEGATIVE,
	PSEUDO_DOUBLE_GET_AT,
	PSEUDO_PUT_MEMBER,
	XS_GET_MEMBER_FOR_CALL,
	XS_GET_FOR_NEW,
	XS_NOP,
	XS_CLOSURE_KIND,

	XS_COUNT
};

#ifdef mxDebug
#define mxCheck(THE, THE_ASSERTION) \
	if (!(THE_ASSERTION)) \
		fxCheck(THE, __FILE__,__LINE__)
#else
#define mxCheck(THE, THE_ASSERTION)
#endif

#ifdef mxDebug
#define mxDebug0(THE, THE_ERROR, THE_FORMAT) \
	(void)fxDebug(THE, THE_ERROR, THE_FORMAT)
#define mxDebug1(THE, THE_ERROR, THE_FORMAT, THE_1) \
	(void)fxDebug(THE, THE_ERROR, THE_FORMAT, THE_1)
#define mxDebug2(THE, THE_ERROR, THE_FORMAT, THE_1, THE_2) \
	(void)fxDebug(THE, THE_ERROR, THE_FORMAT, THE_1, THE_2)
#define mxDebug3(THE, THE_ERROR, THE_FORMAT, THE_1, THE_2, THE_3) \
	(void)fxDebug(THE, THE_ERROR, THE_FORMAT, THE_1, THE_2, THE_3)
#define mxDebugID(THE, THE_ERROR, THE_FORMAT, THE_ID) \
	(void)fxDebugID(THE, THE_ERROR, THE_FORMAT, THE_ID)
#else
#define mxDebug0(THE, THE_ERROR, THE_FORMAT) \
	(void)((!THE_ERROR) || (fxThrowError(THE, THE_ERROR), 1))
#define mxDebug1(THE, THE_ERROR, THE_FORMAT, THE_1) \
	(void)((!THE_ERROR) || (fxThrowError(THE, THE_ERROR), 1))
#define mxDebug2(THE, THE_ERROR, THE_FORMAT, THE_1, THE_2) \
	(void)((!THE_ERROR) || (fxThrowError(THE, THE_ERROR), 1))
#define mxDebug3(THE, THE_ERROR, THE_FORMAT, THE_1, THE_2, THE_3) \
	(void)((!THE_ERROR) || (fxThrowError(THE, THE_ERROR), 1))
#define mxDebugID(THE, THE_ERROR, THE_FORMAT, THE_ID) \
	(void)((!THE_ERROR) || (fxThrowError(THE, THE_ERROR), 1))
#endif

#if mxLittleEndian
#define mxDecode2(THE_CODE, THE_VALUE)	{ \
	txByte* src = (THE_CODE); \
	txByte* dst = (txByte*)&(THE_VALUE) + 1; \
	*dst-- = *src++; \
	*dst = *src++; \
	(THE_CODE) = src; \
	}
#else
#define mxDecode2(THE_CODE, THE_VALUE)	{ \
	txByte* src = (THE_CODE); \
	txByte* dst = (txByte*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst = *src++; \
	(THE_CODE) = src; \
	}
#endif

#if mxLittleEndian
#define mxDecode4(THE_CODE, THE_VALUE)	{ \
	txByte* src = (THE_CODE); \
	txByte* dst = (txByte*)&(THE_VALUE) + 3; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst = *src++; \
	(THE_CODE) = src; \
	}
#else
#define mxDecode4(THE_CODE, THE_VALUE)	{ \
	txByte* src = (THE_CODE); \
	txByte* dst = (txByte*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst = *src++; \
	(THE_CODE) = src; \
	}
#endif

#if mxLittleEndian
#if mxBastardDouble
#define mxDecode8(THE_CODE, THE_VALUE)	{ \
	txByte* src = (THE_CODE); \
	txByte* dst = (txByte*)&(THE_VALUE); \
	dst[3] = *src++; \
	dst[2] = *src++; \
	dst[1] = *src++; \
	dst[0] = *src++; \
	dst[7] = *src++; \
	dst[6] = *src++; \
	dst[5] = *src++; \
	dst[4] = *src++; \
	(THE_CODE) = src; \
	}
#else
#define mxDecode8(THE_CODE, THE_VALUE)	{ \
	txByte* src = (THE_CODE); \
	txByte* dst = (txByte*)&(THE_VALUE) + 7; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst = *src++; \
	(THE_CODE) = src; \
}
#endif
#else
#define mxDecode8(THE_CODE, THE_VALUE)	{ \
	txByte* src = (THE_CODE); \
	txByte* dst = (txByte*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst = *src++; \
	(THE_CODE) = src; \
	}
#endif

#if mxLittleEndian
#define mxEncode2(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE) + 1; \
	*dst++ = *src--; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#else
#define mxEncode2(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#endif

#if mxLittleEndian
#define mxEncode4(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE) + 3; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#else
#define mxEncode4(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#endif

#if mxLittleEndian
#if mxBastardDouble
#define mxEncode8(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE); \
	*dst++ = src[3]; \
	*dst++ = src[2]; \
	*dst++ = src[1]; \
	*dst++ = src[0]; \
	*dst++ = src[7]; \
	*dst++ = src[6]; \
	*dst++ = src[5]; \
	*dst++ = src[4]; \
	(THE_CODE) = dst; \
	}
#else
#define mxEncode8(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE) + 7; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#endif
#else
#define mxEncode8(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#endif

#if 0
#define mxZeroSlot(THE_SLOT) { \
	txSize* dst = (txSize*)(THE_SLOT); \
	*dst++ = 0; \
	*dst++ = 0; \
	*dst++ = 0; \
	*dst = 0; \
}
#else
#if (defined(__GNUC__) && defined(__OPTIMIZE__)) || (defined(__GNUC__) && defined(__LP64__)) || (defined(_MSC_VER) && defined(_M_X64))
#define mxZeroSlot(THE_SLOT) { \
	txSlot* dest = THE_SLOT;	\
	dest->next = NULL;	\
	dest->kind = 0;	\
	dest->ID = 0;	\
	dest->flag = 0;	\
	dest->value.label.link = NULL;	\
	dest->value.label.symbol = NULL;	\
}

#else
#define mxZeroSlot(THE_SLOT) { \
	txSize* dst = (txSize*)(THE_SLOT); \
	dst[0] = 0; \
	dst[1] = 0; \
	dst[2] = 0; \
	dst[3] = 0; \
}
#endif

#endif


#if (defined(__GNUC__) && defined(__OPTIMIZE__)) || (defined(__GNUC__) && defined(__LP64__)) || (defined(_MSC_VER) && defined(_M_X64))
#define mxInitSlot(THE_SLOT, xkind) { \
	txSlot* dest = THE_SLOT;	\
	dest->next = NULL;	\
	dest->kind = xkind;	\
	dest->ID = 0;	\
	dest->flag = 0;	\
	dest->value.label.link = NULL;	\
	dest->value.label.symbol = NULL;	\
}
#else
#if mxLittleEndian
#define mxInitSlot(THE_SLOT, kind) { \
	txSize* dst = (txSize*)(THE_SLOT); \
	dst[0] = 0; \
	dst[1] = kind << 24; \
	dst[2] = 0; \
	dst[3] = 0; \
}
#else
#define mxInitSlot(THE_SLOT, kindValue) { \
	txSize* dst = (txSize*)(THE_SLOT); \
	dst[0] = 0; \
	dst[1] = 0; \
	dst[2] = 0; \
	dst[3] = 0; \
	((txSlot *)dst)->kind = kindValue; \
}
#endif	// mxLittleEndian
#endif

#if mxWindows
	#undef c_setjmp
	#define c_setjmp _setjmp
#endif

#define mxTry(THE_MACHINE) \
	txJump __JUMP__; \
	__JUMP__.nextJump = (THE_MACHINE)->firstJump; \
	__JUMP__.stack = the->stack; \
	__JUMP__.scope = the->scope; \
	__JUMP__.frame = the->frame; \
	__JUMP__.code = the->code; \
	(THE_MACHINE)->firstJump = &__JUMP__; \
	if (c_setjmp(__JUMP__.buffer) == 0) {

#define mxCatch(THE_MACHINE) \
		(THE_MACHINE)->firstJump = __JUMP__.nextJump; \
	} \
	else for (; (__JUMP__.stack); __JUMP__.stack = C_NULL)

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
	(((THE_SLOT)->kind == XS_REFERENCE_KIND) || ((THE_SLOT)->kind == XS_ALIAS_KIND))
#define mxIsFunction(THE_SLOT) \
	( (THE_SLOT) &&  ((THE_SLOT)->flag & XS_VALUE_FLAG) && (((THE_SLOT)->next->kind == XS_CALLBACK_KIND) || ((THE_SLOT)->next->kind == XS_CODE_KIND)))
#define mxIsArray(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_ARRAY_KIND))
#define mxIsString(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_STRING_KIND))
#define mxIsBoolean(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_BOOLEAN_KIND))
#define mxIsNumber(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_NUMBER_KIND))
#define mxIsDate(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_DATE_KIND))
#define mxIsRegExp(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_REGEXP_KIND))
#define mxIsHost(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_HOST_KIND))
#define mxIsChunk(THE_SLOT) \
	((THE_SLOT) &&  ((THE_SLOT)->flag & XS_VALUE_FLAG) && ((THE_SLOT)->next->kind == XS_HOST_KIND) \
		&& ((THE_SLOT)->next->next) && ((THE_SLOT)->next->next->ID == the->chunkID))

#define mxThis (the->frame + 3)
#define mxFunction (the->frame + 2)
#define mxResult (the->frame + 1)
#define mxRoute (the->frame - 1)
#define mxParameters (the->frame - 4)

#define mxArgc ((the->frame + 4)->value.integer)
#define mxArgv(THE_INDEX) (the->frame + 4 + ((the->frame + 4)->value.integer) - (THE_INDEX))
#define mxVarc ((the->frame - 1)->value.integer)
#define mxVarv(THE_INDEX) (the->frame - 2 - THE_INDEX)

#define mxPush(THE_SLOT) \
	fxOverflow(the, -1, C_NULL, 0); \
	*(--the->stack) = (THE_SLOT)
#define mxPushBoolean(THE_BOOLEAN) \
	fxOverflow(the, -1, C_NULL, 0); \
	mxInitSlot(--the->stack, XS_BOOLEAN_KIND); \
	the->stack->value.boolean = (THE_BOOLEAN)
#define mxPushInteger(THE_NUMBER) \
	fxOverflow(the, -1, C_NULL, 0); \
	mxInitSlot(--the->stack, XS_INTEGER_KIND); \
	the->stack->value.integer = (THE_NUMBER)
#define mxPushNull() \
	fxOverflow(the, -1, C_NULL, 0); \
	mxInitSlot(--the->stack, XS_NULL_KIND)
#define mxPushNumber(THE_NUMBER) \
	fxOverflow(the, -1, C_NULL, 0); \
	mxInitSlot(--the->stack, XS_NUMBER_KIND); \
	the->stack->value.number = (THE_NUMBER)
#define mxPushString(THE_STRING) \
	fxOverflow(the, -1, C_NULL, 0); \
	mxInitSlot(--the->stack, XS_STRING_KIND); \
	the->stack->value.string = (THE_STRING)
#define mxPushStringC(THE_STRING) \
	fxOverflow(the, -1, C_NULL, 0); \
	mxZeroSlot(--the->stack); \
	fxCopyStringC(the, the->stack, THE_STRING)
#define mxPushUndefined() \
	fxOverflow(the, -1, C_NULL, 0); \
	mxInitSlot(--the->stack, XS_UNDEFINED_KIND)

#define mxGlobal (the->stackTop[-1])
#define mxException (the->stackTop[-2])

#define mxFiles (the->stackTop[-4])
#define mxBreakpoints (the->stackTop[-5])
#define mxEmptyCode (the->stackTop[-6])
#define mxEmptyString (the->stackTop[-7])

#define mxObjectPrototype (the->stackTop[-8])
#define mxFunctionPrototype (the->stackTop[-9])
#define mxArrayPrototype (the->stackTop[-10])
#define mxStringPrototype (the->stackTop[-11])
#define mxBooleanPrototype (the->stackTop[-12])
#define mxNumberPrototype (the->stackTop[-13])
#define mxDatePrototype (the->stackTop[-14])
#define mxRegExpPrototype (the->stackTop[-15])
#define mxHostPrototype (the->stackTop[-16])

#define mxErrorPrototypes(THE_ERROR) (the->stackTop[-16-(THE_ERROR)])
#define mxErrorPrototype (the->stackTop[-17])
#define mxEvalErrorPrototype (the->stackTop[-18])
#define mxRangeErrorPrototype (the->stackTop[-19])
#define mxReferenceErrorPrototype (the->stackTop[-20])
#define mxSyntaxErrorPrototype (the->stackTop[-21])
#define mxTypeErrorPrototype (the->stackTop[-22])
#define mxURIErrorPrototype (the->stackTop[-23])

#define mxChunkPrototype (the->stackTop[-24])
#define mxGetArgumentFunction (the->stackTop[-25])
#define mxSetArgumentFunction (the->stackTop[-26])
#define mxThrowTypeErrorFunction (the->stackTop[-27])

#define XS_MAX_INDEX ((2 << 28) - 2)

#ifdef __cplusplus
}
#endif

#endif /* __XSALL__ */
