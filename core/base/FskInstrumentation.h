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
#ifndef __FSKINSTRUMENTATION__
#define __FSKINSTRUMENTATION__

#ifndef __FSK__
    #include "Fsk.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FskInstrumentedItemRecord FskInstrumentedItemRecord;
typedef struct FskInstrumentedItemRecord *FskInstrumentedItem;

typedef struct FskInstrumentedTypeRecord FskInstrumentedTypeRecord;
typedef struct FskInstrumentedTypeRecord *FskInstrumentedType;

typedef struct FskInstrumentedListenerRecord FskInstrumentedListenerRecord;
typedef struct FskInstrumentedListenerRecord *FskInstrumentedListener;

typedef struct FskInstrumentedValueRecord FskInstrumentedValueRecord;
typedef struct FskInstrumentedValueRecord *FskInstrumentedValue;

#if !TARGET_OS_WIN32
    #define FSK_INSTRUMENTATION_INLINE static inline
#else
    #define FSK_INSTRUMENTATION_INLINE static __inline
#endif

#if SUPPORT_INSTRUMENTATION

#include <stdarg.h>

typedef Boolean (*FskInstrumentationTypeFormatMessage)(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

struct FskInstrumentedTypeRecord {
	FskInstrumentedType				next;							// link to next FskInstrumentedTypeRecord
	UInt32							recordSize;						// size of this FskInstrumentedTypeRecord

	const char						*typeName;						// human readable name

	UInt32							instrumentationOffset;			// offset to _instrumentation field in instances of this object

	FskInstrumentedItem				items;							// list of all instances of this type

	UInt32							typeID;							// unique integer id for this type (assigned by system)

	FskInstrumentedListener			listeners;						// list of listeners on this type

	FskInstrumentationTypeFormatMessage
									doFormat;						// function to convert messages from this item to string

	FskInstrumentedValue			values;							// array of FskInstrumentedValueRecord

	UInt32							listenerLevel;					// max of levels of all listeners
};

struct FskInstrumentedItemRecord {
	FskInstrumentedItem				ownerNext;						// link to next sibling
	FskInstrumentedItem				typeNext;						// link to next item in type list (e.g. FskInstrumentedTypeRecord.items list)

	FskInstrumentedType				dispatch;						// link to type record

	FskInstrumentedItem				owner;							// owning instrumented object
	FskInstrumentedItem				children;						// list of children

	FskInstrumentedListener			listeners;						// list of listeners on this instance
	UInt32							listenerLevel;					// max of levels of all listeners

	const char						*name;							// human readable name of this object
};

struct FskInstrumentedValueRecord {
	const char						*name;
	UInt32							offset;
	UInt32							kind;
	const char						*typeName;
	UInt32							offset2;
};

enum {
	kFskInstrumentedTypeNew = 1,
	kFskInstrumentedItemNew,
	kFskInstrumentedItemDispose,
	kFskInstrumentedItemChangeOwner,
	kFskInstrumentedDisposedBeforeChildren,
	kFskInstrumentedItemPrintf,

	kFskInstrumentedItemFirstCustomMessage = 0x4000
};

enum {
	// typically used by instrumented items when sending
	kFskInstrumentationLevelNone = 0,
	kFskInstrumentationLevelMinimal = 0x01,
	kFskInstrumentationLevelNormal = 0x02,
	kFskInstrumentationLevelDetailed = 0x04,		//@@ unused
	kFskInstrumentationLevelVerbose = 0x08,
	kFskInstrumentationLevelDebug = 0x10,

	// typically used by clients when adding listener
	kFskInstrumentationLevelUpToMinimal= 0x01,
	kFskInstrumentationLevelUpToNormal = 0x03,
	kFskInstrumentationLevelUpToDetailed = 0x07,	//@@ unused
	kFskInstrumentationLevelUpToVerbose = 0x0f,
	kFskInstrumentationLevelUpToDebug = 0x1f
};

enum {
	kFskInstrumentationKindUndefined = 0,
	kFskInstrumentationKindInteger,
	kFskInstrumentationKindBoolean,
	kFskInstrumentationKindString,
	kFskInstrumentationKindRectangle,
	kFskInstrumentationKindNamed,
	kFskInstrumentationKindFloat,
	kFskInstrumentationKindDouble,
	kFskInstrumentationKindFixed,

	kFskInstrumentationKindMask = 0x00FFFFFF,
	kFskInstrumentationKindHasOffset2 = 0x80000000
};

/*
	object interface
*/
FskExport(FskErr) FskInstrumentedItemNew_(FskInstrumentedItem item, void *id, const char *name, FskInstrumentedType dispatch);
FskExport(FskErr) FskInstrumentedItemDispose_(FskInstrumentedItem item);

FskExport(FskErr) FskInstrumentedItemSetOwner_(FskInstrumentedItem item, FskInstrumentedItem owner);		// item becomes child of owner

FskExport(FskErr) FskInstrumentedItemSendMessage_(FskInstrumentedItem item, UInt32 msg, void *msgData, UInt32 level);

FskExport(FskErr) FskInstrumentedTypeSendMessage_(FskInstrumentedType type, UInt32 msg, void *msgData, UInt32 level);

FskExport(FskErr) FskInstrumentedSystemSendMessage(UInt32 msg, const void *msgData, UInt32 level);

FskExport(FskErr) FskInstrumentedItemPrintfForLevel(FskInstrumentedItem item, UInt32 level, const char *msg, ...);
FskExport(FskErr) FskInstrumentedTypePrintfForLevel(FskInstrumentedType dispatch, UInt32 level, const char *msg, ...);
FskExport(FskErr) FskInstrumentedTypePrintfForLevel_(FskInstrumentedType dispatch, UInt32 level, const char *msg, void *arguments);
FskExport(FskErr) FskInstrumentedSystemPrintfForLevel(UInt32 level, const char *msg, ...);

#define FskInstrumentedItemDeclaration FskInstrumentedItemRecord _instrumented;
#define FskInstrumentationOffset(a) (UInt32)(UInt32)&(((a *)0)->_instrumented)		// offsetof
#define FskInstrumentedItemNew(obj, name, dispatch) if (NULL != obj) FskInstrumentedItemNew_(&(obj)->_instrumented, obj, name, dispatch)
#define FskInstrumentedItemDispose(obj) FskInstrumentedItemDispose_(&(obj)->_instrumented)
#define FskInstrumentedItemSetOwner(obj, owner) if (obj) FskInstrumentedItemSetOwner_(&(obj)->_instrumented, &(owner)->_instrumented)
#define FskInstrumentedItemSetName(obj, _name) (obj)->_instrumented.name = _name
#define FskInstrumentedItemGetName(obj) ((obj)->_instrumented.name)
#define FskInstrumentationItemToObject(obj) (((char *)obj) - (obj)->dispatch->instrumentationOffset)
#define FskInstrumentedItemHasListenersForLevel(obj, level) (0 != ((obj)->_instrumented.listenerLevel & level))
#define FskInstrumentedItemSendMessageForLevel(obj, msg, msgData, level) FskInstrumentedItemSendMessage_(&(obj)->_instrumented, msg, msgData, level)
#define FskInstrumentedSystemPrintf(...) FskInstrumentedSystemPrintfForLevel(kFskInstrumentationLevelDebug, __VA_ARGS__);

#define FskInstrumentedTypeHasListenersForLevel(type, level) (0 != ((type)->listenerLevel & level))
#define FskInstrumentedTypeSendMessageForLevel(type, msg, msgData, level) FskInstrumentedItemSendMessage_(&(type)-> msg, msgData, level)

#define FskInstrumentedItemSendMessageMinimal(obj, msg, msgData) {if (FskInstrumentedItemHasListenersForLevel(obj, kFskInstrumentationLevelMinimal)) FskInstrumentedItemSendMessageForLevel(obj, msg, msgData, kFskInstrumentationLevelMinimal);}
#define FskInstrumentedItemSendMessageNormal(obj, msg, msgData) {if (FskInstrumentedItemHasListenersForLevel(obj, kFskInstrumentationLevelNormal)) FskInstrumentedItemSendMessageForLevel(obj, msg, msgData, kFskInstrumentationLevelNormal);}
#define FskInstrumentedItemSendMessageVerbose(obj, msg, msgData) {if (FskInstrumentedItemHasListenersForLevel(obj, kFskInstrumentationLevelVerbose)) FskInstrumentedItemSendMessageForLevel(obj, msg, msgData, kFskInstrumentationLevelVerbose);}
#define FskInstrumentedItemSendMessageDebug(obj, msg, msgData) {if (FskInstrumentedItemHasListenersForLevel(obj, kFskInstrumentationLevelDebug)) FskInstrumentedItemSendMessageForLevel(obj, msg, msgData, kFskInstrumentationLevelDebug);}

#define FskInstrumentedItemPrintfMinimal(obj, ...) {if (FskInstrumentedItemHasListenersForLevel(obj, kFskInstrumentationLevelMinimal)) FskInstrumentedItemPrintfForLevel(&(obj)->_instrumented, kFskInstrumentationLevelMinimal, __VA_ARGS__);}
#define FskInstrumentedItemPrintfNormal(obj, ...) {if (FskInstrumentedItemHasListenersForLevel(obj, kFskInstrumentationLevelNormal)) FskInstrumentedItemPrintfForLevel(&(obj)->_instrumented, kFskInstrumentationLevelNormal, __VA_ARGS__);}
#define FskInstrumentedItemPrintfVerbose(obj, ...) {if (FskInstrumentedItemHasListenersForLevel(obj, kFskInstrumentationLevelVerbose)) FskInstrumentedItemPrintfForLevel(&(obj)->_instrumented, kFskInstrumentationLevelVerbose, __VA_ARGS__);}
#define FskInstrumentedItemPrintfDebug(obj, ...) {if (FskInstrumentedItemHasListenersForLevel(obj, kFskInstrumentationLevelDebug)) FskInstrumentedItemPrintfForLevel(&(obj)->_instrumented, kFskInstrumentationLevelDebug, __VA_ARGS__);}

#define FskInstrumentedTypeSendMessageMinimal(type, msg, msgData) {if ((type)->listenerLevel & kFskInstrumentationLevelMinimal) FskInstrumentedTypeSendMessage_(type, msg, msgData, kFskInstrumentationLevelMinimal);}
#define FskInstrumentedTypeSendMessageNormal(type, msg, msgData) {if ((type)->listenerLevel & kFskInstrumentationLevelNormal) FskInstrumentedTypeSendMessage_(type, msg, msgData, kFskInstrumentationLevelNormal);}
#define FskInstrumentedTypeSendMessageVerbose(type, msg, msgData) {if ((type)->listenerLevel & kFskInstrumentationLevelVerbose) FskInstrumentedTypeSendMessage_(type, msg, msgData, kFskInstrumentationLevelVerbose);}
#define FskInstrumentedTypeSendMessageDebug(type, msg, msgData) {if ((type)->listenerLevel & kFskInstrumentationLevelDebug) FskInstrumentedTypeSendMessage_(type, msg, msgData, kFskInstrumentationLevelDebug);}

#define FskInstrumentedTypePrintfMinimal(type, ...) FskInstrumentedTypePrintfForLevel(type, kFskInstrumentationLevelMinimal, __VA_ARGS__);
#define FskInstrumentedTypePrintfNormal(type, ...) FskInstrumentedTypePrintfForLevel(type, kFskInstrumentationLevelNormal, __VA_ARGS__);
#define FskInstrumentedTypePrintfVerbose(type, ...) FskInstrumentedTypePrintfForLevel(type, kFskInstrumentationLevelVerbose, __VA_ARGS__);
#define FskInstrumentedTypePrintfDebug(type, ...) FskInstrumentedTypePrintfForLevel(type, kFskInstrumentationLevelDebug, __VA_ARGS__);

#define FskInstrumentedItemFormatMessage(item, msg, msgData, buffer, bufferSize) FskInstrumentedTypeFormatMessage(((item)->dispatch), msg, msgData, buffer, bufferSize)

#define FskInstrumentedItemFromTypeNext(item) ((FskInstrumentedItem)(-((char *)&item->typeNext - (char *)item) + (char *)item))

#define FskInstrumentedTypePrintfsDefine(type, record) \
            FSK_INSTRUMENTATION_INLINE void Fsk##type##PrintfMinimal(const char *msg, ...) { va_list arguments; va_start(arguments, msg); FskInstrumentedTypePrintfForLevel_(&(record), kFskInstrumentationLevelMinimal, msg, (void *)&arguments); va_end(arguments); } \
            FSK_INSTRUMENTATION_INLINE void Fsk##type##PrintfNormal(const char *msg, ...) { va_list arguments; va_start(arguments, msg); FskInstrumentedTypePrintfForLevel_(&(record), kFskInstrumentationLevelNormal, msg, (void *)&arguments); va_end(arguments); } \
            FSK_INSTRUMENTATION_INLINE void Fsk##type##PrintfVerbose(const char *msg, ...) { va_list arguments; va_start(arguments, msg); FskInstrumentedTypePrintfForLevel_(&(record), kFskInstrumentationLevelVerbose, msg, (void *)&arguments); va_end(arguments); } \
            FSK_INSTRUMENTATION_INLINE void Fsk##type##PrintfDebug(const char *msg, ...) { va_list arguments; va_start(arguments, msg); FskInstrumentedTypePrintfForLevel_(&(record), kFskInstrumentationLevelDebug, msg, (void *)&arguments); va_end(arguments); }

#define FskInstrumentedSimpleType(type, name) FskInstrumentedTypeRecord g##type##TypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), #name}; \
            FskInstrumentedTypePrintfsDefine(type, g##type##TypeInstrumentation)


// these two are for compatibility with the original macros
#define FskInstrumentedItemHasListeners(obj) FskInstrumentedItemHasListenersForLevel(obj, kFskInstrumentationLevelNormal)
#define FskInstrumentedItemSendMessage(obj, msg, msgData) FskInstrumentedItemSendMessageNormal(obj, msg, msgData)

/*
	client interface
*/

typedef void (*FskInstrumentationListenerSystemProc)(void *refcon, UInt32 msg, const void *msgData, UInt32 level);
typedef void (*FskInstrumentationListenerTypeProc)(void *refcon, UInt32 msg, const void *msgData, UInt32 level, FskInstrumentedType type);
typedef void (*FskInstrumentationListenerItemProc)(void *refcon, UInt32 msg, const void *msgData, UInt32 level, FskInstrumentedItem item);

struct FskInstrumentedListenerRecord {
	FskInstrumentedListener				next;
	void								*owner;
	void								*proc;
	void								*refcon;
	UInt32								level;
};

FskExport(FskInstrumentedListener) FskInstrumentionAddSystemListener(FskInstrumentationListenerSystemProc listenerProc, void *refcon);
FskExport(void) FskInstrumentionRemoveSystemListener(FskInstrumentedListener listener);

FskExport(FskInstrumentedListener) FskInstrumentionAddTypeListener(FskInstrumentedType type, FskInstrumentationListenerTypeProc listenerProc, void *refcon, UInt32 level);
FskExport(void) FskInstrumentionRemoveTypeListener(FskInstrumentedListener listener);

FskExport(FskInstrumentedListener) FskInstrumentionAddItemListener(FskInstrumentedItem item, FskInstrumentationListenerItemProc listenerProc, void *refcon, UInt32 level);
FskExport(void) FskInstrumentionRemoveItemListener(FskInstrumentedListener listener);

FskExport(void) FskInstrumentationAddType(FskInstrumentedType dispatch);
FskExport(FskInstrumentedType) FskInstrumentionGetType(const char *typeName);

FskExport(FskErr) FskInstrumentedTypeFormatMessage(FskInstrumentedType dispatch, UInt32 msg, void *msgData, void *buffer, UInt32 bufferSize);

FskExport(void) FskInstrumentationInitialize(void);
FskExport(void) FskInstrumentationTerminate(void);

FskExport(const char *) FskInstrumentationGetErrorString(FskErr err);
FskExport(const char *) FskInstrumentationCleanPath(const char *fullPath);

FskExport(FskInstrumentedType) FskInstrumentationGetErrorInstrumentation();

/*
	simple built-in client
*/

typedef struct FskInstrumentationSimpleClientRecord FskInstrumentationSimpleClientRecord;
typedef struct FskInstrumentationSimpleClientRecord *FskInstrumentationSimpleClient;

struct FskInstrumentationSimpleClientRecord {
	FskInstrumentationSimpleClient			next;

	UInt32									level;
	FskInstrumentedListener					listener;

	char									type[1];
};

FskExport(void) FskInstrumentationSimpleClientConfigure(Boolean trace, Boolean threads, Boolean times, const char *path, const char *syslogAddr, Boolean androidlog);
FskAPI(void) FskInstrumentationSimpleClientAddType(const char *typeName, UInt32 level);
void FskInstrumentationSimpleClientDump(const char *header);
void FskInstrumentationSimpleClientDumpMemory(void);

FskAPI(FskInstrumentationSimpleClient) FskInstrumentationSimpleClientGetTypeList(void);


#else

#define FskInstrumentedItemDeclaration

#define FskInstrumentedItemNew(obj, name, dispatch)
#define FskInstrumentedItemDispose(obj)
#define FskInstrumentedItemSetOwner(obj, id)
#define FskInstrumentedItemSetName(obj, name)
#define FskInstrumentationItemToObject(obj) (NULL)
#define FskInstrumentedItemHasListeners(obj) (false)
#define FskInstrumentedItemSendMessage(obj, msg, msgData)
#define FskInstrumentedItemHasListenersForLevel(obj, level) (false)
#define FskInstrumentedItemSendMessageForLevel(obj, msg, msgData, level)

#define FskInstrumentedSystemPrintf(...)

#define FskInstrumentedItemSendMessageMinimal(obj, msg, msgData)
#define FskInstrumentedItemSendMessageNormal(obj, msg, msgData)
#define FskInstrumentedItemSendMessageVerbose(obj, msg, msgData)
#define FskInstrumentedItemSendMessageDebug(obj, msg, msgData)

#define FskInstrumentedItemPrintfMinimal(obj, ...)
#define FskInstrumentedItemPrintfNormal(obj, ...)
#define FskInstrumentedItemPrintfVerbose(obj, ...)
#define FskInstrumentedItemPrintfDebug(obj, ...)

#define FskInstrumentedTypeSendMessageMinimal(type, msg, msgData)
#define FskInstrumentedTypeSendMessageNormal(type, msg, msgData)
#define FskInstrumentedTypeSendMessageVerbose(type, msg, msgData)
#define FskInstrumentedTypeSendMessageDebug(type, msg, msgData)

#define FskInstrumentedTypePrintfMinimal(type, ...)
#define FskInstrumentedTypePrintfNormal(type, ...)
#define FskInstrumentedTypePrintfVerbose(type, ...)
#define FskInstrumentedTypePrintfDebug(type, ...)

#define FskInstrumentionAddSystemListener(a, b)
#define FskInstrumentionRemoveSystemListener(a)
#define FskInstrumentionAddTypeListener(a, b, c, d)
#define FskInstrumentionRemoveTypeListener(a)
#define FskInstrumentionAddItemListener(a, b, c, d)
#define FskInstrumentionRemoveItemListener(a)

#define FskInstrumentationInitialize()
#define FskInstrumentationTerminate()

#if SUPPORT_XS_DEBUG
    FskExport(const char*) FskInstrumentationGetErrorString(FskErr err);
    FskExport(const char *) FskInstrumentationCleanPath(const char *fullPath);
#else
    #define FskInstrumentationGetErrorString(a) (NULL)
    #define FskInstrumentationCleanPath(fullPath) ("")
#endif

#define FskInstrumentedTypePrintfsDefine(type, record) \
    FSK_INSTRUMENTATION_INLINE void Fsk##type##PrintfMinimal(const char *msg, ...) {} \
    FSK_INSTRUMENTATION_INLINE void Fsk##type##PrintfNormal(const char *msg, ...)  {} \
    FSK_INSTRUMENTATION_INLINE void Fsk##type##PrintfVerbose(const char *msg, ...) {} \
    FSK_INSTRUMENTATION_INLINE void Fsk##type##PrintfDebug(const char *msg, ...) {}

#define FskInstrumentedSimpleType(type, name) FskInstrumentedTypePrintfsDefine(type, NULL)

#endif

#ifdef __cplusplus
}
#endif

#endif
