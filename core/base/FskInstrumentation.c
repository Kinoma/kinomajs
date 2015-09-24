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

	//
	// to do:
	//		consider mutex for each type rather than global, to minimize unnecessary waits
	//		consider requiring caller to provide storage for listener records (can reduce run-time allocations on appropriately written client)
	//

#include "FskInstrumentation.h"
#include "FskEnvironment.h"
#include "FskString.h"

#if SUPPORT_INSTRUMENTATION

#include "FskFiles.h"
#include "FskList.h"
#include "FskNetUtils.h"
#include "FskThread.h"

#if TARGET_OS_ANDROID
	#include <android/log.h>
	#include "FskHardware.h"
#endif

static FskListMutex gInstrumentedItems;
static FskList gInstrumentTypes;
static FskList gSystemListeners;
static UInt32 gInstrumentedTypeID;

static void doSendSystem(UInt32 msg, const void *data, UInt32 level);
static void doSendType(FskInstrumentedType dispatch, UInt32 msg, void *data, UInt32 level);
static void doSendItem(FskInstrumentedItem item, UInt32 msg, void *data, UInt32 level);

static FskInstrumentedTypeRecord gFskErrorInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "error"};

#define c_va_list va_list
#define c_va_start va_start
#define c_va_end va_end
#define c_va_arg va_arg
#define c_va_copy va_copy

typedef struct {
    const char *msg;
    c_va_list *arguments;
} FskInstrumentationPrintfRecord, *FskInstrumentationPrintf;

FskErr FskInstrumentedItemPrintfForLevel(FskInstrumentedItem item, UInt32 level, const char *msg, ...)
{
	FskInstrumentedListener listener;

	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);

	for (listener = item->listeners; NULL != listener; listener = listener->next) {
		if (level & listener->level) {
			c_va_list arguments;
            FskInstrumentationPrintfRecord pr = {msg, &arguments};

			c_va_start(arguments, msg);
			((FskInstrumentationListenerItemProc)listener->proc)(listener->refcon, kFskInstrumentedItemPrintf, (void *)&pr, level, item);
			c_va_end(arguments);
		}
	}

	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return kFskErrNone;
}

FskErr FskInstrumentedTypePrintfForLevel(FskInstrumentedType dispatch, UInt32 level, const char *msg, ...)
{
	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);

	if (0 == dispatch->typeID)
		FskInstrumentationAddType(dispatch);

	if (dispatch->listenerLevel & level) {
		FskInstrumentedListener listener;

		for (listener = dispatch->listeners; NULL != listener; listener = listener->next) {
			if (level & listener->level) {
				c_va_list arguments;
                FskInstrumentationPrintfRecord pr = {msg, &arguments};

				c_va_start(arguments, msg);
				((FskInstrumentationListenerTypeProc)listener->proc)(listener->refcon, kFskInstrumentedItemPrintf, (void *)&pr, level, dispatch);
				c_va_end(arguments);
			}
		}
	}

	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return kFskErrNone;
}

FskErr FskInstrumentedTypePrintfForLevel_(FskInstrumentedType dispatch, UInt32 level, const char *msg, void *argumentsIn)
{
	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);

	if (0 == dispatch->typeID)
		FskInstrumentationAddType(dispatch);

	if (dispatch->listenerLevel & level) {
		FskInstrumentedListener listener;

		for (listener = dispatch->listeners; NULL != listener; listener = listener->next) {
			if (level & listener->level) {
                FskInstrumentationPrintfRecord pr = {msg, (c_va_list *)argumentsIn};
				((FskInstrumentationListenerTypeProc)listener->proc)(listener->refcon, kFskInstrumentedItemPrintf, (void *)&pr, level, dispatch);
			}
		}
	}

	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return kFskErrNone;
}

FskErr FskInstrumentedSystemPrintfForLevel(UInt32 level, const char *msg, ...)			// level currently unused
{
	FskInstrumentedListener listener;

	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);

	for (listener = (FskInstrumentedListener)gSystemListeners; NULL != listener; listener = listener->next) {
		c_va_list arguments;
        FskInstrumentationPrintfRecord pr = {msg, &arguments};

		c_va_start(arguments, msg);
		((FskInstrumentationListenerSystemProc)listener->proc)(listener->refcon, kFskInstrumentedItemPrintf, (void *)&pr, level);
		c_va_end(arguments);
	}

	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return kFskErrNone;
}

FskErr FskInstrumentedItemNew_(FskInstrumentedItem item, void *id, const char *name, FskInstrumentedType dispatch)
{
	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);


	if (0 == dispatch->typeID)
		FskInstrumentationAddType(dispatch);

	item->ownerNext = NULL;
	item->typeNext = NULL;
	item->dispatch = dispatch;
	item->owner = NULL;
	item->children = NULL;
	item->listeners = NULL;
	item->name = name;

	FskListPrepend(&gInstrumentedItems->list, &item->ownerNext);
	FskListPrepend((FskList*)(void*)&dispatch->items, &item->typeNext);

	if (dispatch->listenerLevel & kFskInstrumentationLevelMinimal)
		doSendType(dispatch, kFskInstrumentedItemNew, item, kFskInstrumentationLevelMinimal);

	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);
	return kFskErrNone;
}

FskErr FskInstrumentedItemDispose_(FskInstrumentedItem item)
{
	FskInstrumentedType dispatch;

	if (NULL == item)
		return kFskErrNone;

	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);

	dispatch = item->dispatch;
	if (NULL == dispatch) {
		FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);
		return kFskErrNone;
	}

	if (NULL != item->children) {
		doSendSystem(kFskInstrumentedDisposedBeforeChildren, item, kFskInstrumentationLevelMinimal);

			// detach children, to avoid crash later
		while (NULL != item->children) {
			item->children->owner = NULL;
			FskListRemoveFirst((FskList*)(void*)&item->children);
		}
	}

	if (dispatch->listenerLevel & kFskInstrumentationLevelMinimal)
		doSendType(dispatch, kFskInstrumentedItemDispose, item, kFskInstrumentationLevelMinimal);

	if (NULL == item->owner)
		FskListRemove(&gInstrumentedItems->list, &item->ownerNext);
	else
		FskListRemove((FskList*)(void*)&item->owner->children, &item->ownerNext);

	FskListRemove((FskList*)(void*)&dispatch->items, &item->typeNext);

	while (item->listeners)
		FskInstrumentionRemoveItemListener(item->listeners);

	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return kFskErrNone;
}

FskErr FskInstrumentedItemSetOwner_(FskInstrumentedItem item, FskInstrumentedItem owner)
{
	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);

		// detach
	if (NULL == item->owner)
		FskListRemove(&gInstrumentedItems->list, &item->ownerNext);
	else
		FskListRemove((FskList*)(void*)&item->owner->children, &item->ownerNext);

		// assign
	item->owner = owner;

		// attach
	if (NULL == owner)
		FskListPrepend(&gInstrumentedItems->list, &item->ownerNext);
	else
		FskListPrepend((FskList*)(void*)&owner->children, &item->ownerNext);

		// notify
	if (item->listenerLevel & kFskInstrumentationLevelMinimal)
		doSendItem(item, kFskInstrumentedItemChangeOwner, item, kFskInstrumentationLevelMinimal);

	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return kFskErrNone;
}

FskErr FskInstrumentedItemSendMessage_(FskInstrumentedItem item, UInt32 msg, void *data, UInt32 level)
{
	FskInstrumentedListener listener;

	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);

	for (listener = item->listeners; NULL != listener; listener = listener->next) {
		if (level & listener->level)
			((FskInstrumentationListenerItemProc)listener->proc)(listener->refcon, msg, data, level, item);
	}

	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return kFskErrNone;
}

FskErr FskInstrumentedTypeSendMessage_(FskInstrumentedType type, UInt32 msg, void *data, UInt32 level)
{
	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);
		doSendType(type, msg, data, level);
	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return kFskErrNone;
}

FskErr FskInstrumentedSystemSendMessage(UInt32 msg, const void *msgData, UInt32 level)
{
	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);
		doSendSystem(msg, msgData, level);
	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return kFskErrNone;
}

FskInstrumentedListener FskInstrumentionAddSystemListener(FskInstrumentationListenerSystemProc listenerProc, void *refcon)
{
	FskInstrumentedListener listener;

	if (kFskErrNone != FskMemPtrNew_Untracked(sizeof(FskInstrumentedListenerRecord), &listener))
		return NULL;

	listener->next = NULL;
	listener->proc = listenerProc;
	listener->owner = NULL;
	listener->refcon = refcon;
	listener->level = 0;

	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);
		FskListPrepend(&gSystemListeners, listener);
	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return listener;
}

void FskInstrumentionRemoveSystemListener(FskInstrumentedListener listener)
{
	if (NULL == listener)
		return;

	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);
		FskListRemove(&gSystemListeners, listener);
	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	FskMemPtrDispose_Untracked(listener);
}

FskInstrumentedListener FskInstrumentionAddTypeListener(FskInstrumentedType type, FskInstrumentationListenerTypeProc listenerProc, void *refcon, UInt32 level)
{
	FskInstrumentedListener listener;

	if (kFskErrNone != FskMemPtrNew_Untracked(sizeof(FskInstrumentedListenerRecord), &listener))
		return NULL;

	listener->next = NULL;
	listener->proc = listenerProc;
	listener->owner = type;
	listener->refcon = refcon;
	listener->level = level;

	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);
		type->listenerLevel |= level;
		FskListPrepend((FskList*)(void*)&type->listeners, listener);
	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return listener;
}

void FskInstrumentionRemoveTypeListener(FskInstrumentedListener listener)
{
	FskInstrumentedType type;

	if (NULL == listener)
		return;

	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);
		type = (FskInstrumentedType)listener->owner;
		FskListRemove((FskList*)(void*)&type ->listeners, listener);

		type->listenerLevel = 0;
		if (NULL != type->listeners) {
			FskInstrumentedListener walker;
			for (walker = type->listeners; NULL != walker; walker = walker->next)
				type->listenerLevel |= walker->level;
		}

	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	FskMemPtrDispose_Untracked(listener);
}

FskInstrumentedListener FskInstrumentionAddItemListener(FskInstrumentedItem item, FskInstrumentationListenerItemProc listenerProc, void *refcon, UInt32 level)
{
	FskInstrumentedListener listener;

	if (kFskErrNone != FskMemPtrNew_Untracked(sizeof(FskInstrumentedListenerRecord), &listener))
		return NULL;

	listener->next = NULL;
	listener->proc = listenerProc;
	listener->owner = item;
	listener->refcon = refcon;
	listener->level = level;

	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);
		item->listenerLevel |= level;
		FskListPrepend((FskList*)(void*)&item->listeners, listener);
	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	return listener;
}

void FskInstrumentionRemoveItemListener(FskInstrumentedListener listener)
{
	FskInstrumentedItem item;

	if (NULL == listener)
		return;

	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);
		item = (FskInstrumentedItem)listener->owner;
		FskListRemove((FskList*)(void*)&item->listeners, listener);

		item->listenerLevel = 0;
		if (NULL != item->listeners) {
			FskInstrumentedListener walker;
			for (walker = item->listeners; NULL != walker; walker = walker->next)
				item->listenerLevel |= walker->level;
		}
	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);

	FskMemPtrDispose_Untracked(listener);
}

void FskInstrumentationAddType(FskInstrumentedType dispatch)
{
	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);

	dispatch->typeID = ++gInstrumentedTypeID;
	FskListPrepend(&gInstrumentTypes, &dispatch->next);

	if (gSystemListeners)
		doSendSystem(kFskInstrumentedTypeNew, dispatch, kFskInstrumentationLevelMinimal);

	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);
}

FskInstrumentedType FskInstrumentionGetType(const char *typeName)
{
	FskInstrumentedType dispatch;

	for (dispatch = (FskInstrumentedType)gInstrumentTypes; NULL != dispatch; dispatch = dispatch->next) {
		if (0 == FskStrCompare(typeName, dispatch->typeName))
			return dispatch;
	}

	return NULL;
}

FskErr FskInstrumentedTypeFormatMessage(FskInstrumentedType dispatch, UInt32 msg, void *msgData, void *buffer, UInt32 bufferSize)
{
	Boolean result = false;

	if (dispatch->doFormat)
		result = (dispatch->doFormat)(dispatch, msg, msgData, buffer, bufferSize);

	if (!result && (kFskInstrumentedItemPrintf == msg)) {
        FskInstrumentationPrintf pr = (FskInstrumentationPrintf)msgData;
		c_va_list arguments;
		c_va_copy(arguments, *(pr->arguments));
		vsnprintf(buffer, bufferSize, pr->msg, arguments);
		result = true;
	}

	return result;
}

FskInstrumentedType FskInstrumentationGetErrorInstrumentation(void)
{
    return &gFskErrorInstrumentation;
}

void FskInstrumentationInitialize(void)
{
	if (NULL == gInstrumentedItems)
		FskListMutexNew_uninstrumented(&gInstrumentedItems, "gInstrumentedItems");
}

void FskInstrumentationTerminate(void)
{
	FskInstrumentationSimpleClientDump("\n** leaked instrumented objects **\n");
	FskInstrumentationSimpleClientDumpMemory();
}

void doSendSystem(UInt32 msg, const void *data, UInt32 level)
{
	FskInstrumentedListener listener;

	for (listener = (FskInstrumentedListener)gSystemListeners; NULL != listener; listener = listener->next)
		((FskInstrumentationListenerSystemProc)listener->proc)(listener->refcon, msg, data, level);
}

void doSendType(FskInstrumentedType dispatch, UInt32 msg, void *data, UInt32 level)
{
	FskInstrumentedListener listener;
	for (listener = dispatch->listeners; NULL != listener; listener = listener->next) {
		if (level & listener->level)
			((FskInstrumentationListenerTypeProc)listener->proc)(listener->refcon, msg, data, level, dispatch);
	}
}

void doSendItem(FskInstrumentedItem item, UInt32 msg, void *data, UInt32 level)
{
	FskInstrumentedListener listener;
	for (listener = item->listeners; NULL != listener; listener = listener->next) {
		if (level & listener->level)
			((FskInstrumentationListenerItemProc)listener->proc)(listener->refcon, msg, data, level, item);
	}
}

	/*
	simple instrumentation client
*/

FskInstrumentationSimpleClient gFskInstrumentationSimpleClients;

static void iscSystemListener(void *refcon, UInt32 msg, const void *data, UInt32 level);
static void iscTypeListener(void *refcon, UInt32 msg, const void *data, UInt32 level, FskInstrumentedType type);
static void iscItemListener(void *refcon, UInt32 msg, const void *data, UInt32 level, FskInstrumentedItem item);
static void iscWriteLogLine(const char *line, UInt32 level);

static FskFile gLogFREF;
static Boolean gTrace, gThreads, gTimes, gAndroidlog;
static int gSysLog = 0;
static FskInstrumentedListener gSystemListener;

void FskInstrumentationSimpleClientConfigure(Boolean trace, Boolean threads, Boolean times, const char *path, const char *syslogAddr, Boolean androidlog)
{
	FskInstrumentedType dispatch;

	if (path) {
		FskFileDelete(path);
		FskFileCreate(path);
		FskFileOpen(path, kFskFilePermissionReadWrite, &gLogFREF);
	}
	else
	if (gLogFREF) {
		FskFileClose(gLogFREF);
		gLogFREF = NULL;
	}

	gTrace = trace;
	gThreads = threads;
	gTimes = times;
	gAndroidlog = androidlog;

	if (syslogAddr && *syslogAddr) {
		if (0 == FskStrCompare(syslogAddr, "localhost"))
			gSysLog = kFskNetLocalhost;
		else
			FskNetStringToIPandPort(syslogAddr, &gSysLog, NULL);
	}

	if (NULL == gSystemListener) {
		gSystemListener = FskInstrumentionAddSystemListener(iscSystemListener, NULL);

			// dump list of installed instrumented types
		for (dispatch = (FskInstrumentedType)gInstrumentTypes; NULL != dispatch; dispatch = dispatch->next)
			iscSystemListener(NULL, kFskInstrumentedTypeNew, dispatch, kFskInstrumentationLevelMinimal);
	}
}

void FskInstrumentationSimpleClientAddType(const char *typeName, UInt32 level)
{
	FskInstrumentationSimpleClient isc;
	FskInstrumentedType dispatch;

	for (isc = gFskInstrumentationSimpleClients; NULL != isc; isc = isc->next) {
		if (0 == FskStrCompare(typeName, isc->type))
			break;
	}

	if (NULL == isc) {
		if (kFskErrNone != FskMemPtrNewClear_Untracked(sizeof(FskInstrumentationSimpleClientRecord) + FskStrLen(typeName), &isc))
			return;

		FskStrCopy(isc->type, typeName);

		FskListPrepend((FskList *)(void *)&gFskInstrumentationSimpleClients, isc);
	}

	isc->level = level;

	if (isc->listener) {
		FskInstrumentionRemoveTypeListener(isc->listener);
		isc->listener = NULL;
	}

	if (kFskInstrumentationLevelNone != level) {
		dispatch = FskInstrumentionGetType(typeName);
		if (NULL != dispatch) {
			FskInstrumentedItem item;

			isc->listener = FskInstrumentionAddTypeListener(dispatch, iscTypeListener, isc, isc->level);

			for (item = dispatch->items; NULL != item; item = item->typeNext) {
				item = FskInstrumentedItemFromTypeNext(item);
				FskInstrumentionAddItemListener(item, iscItemListener, isc, isc->level);
			}
		}
	}
}

FskInstrumentationSimpleClient FskInstrumentationSimpleClientGetTypeList(void)
{
	return gFskInstrumentationSimpleClients;
}

void iscSystemListener(void *refcon, UInt32 msg, const void *data, UInt32 level)
{
	FskInstrumentationSimpleClient isc;
	char buffer[1024];

	if (kFskInstrumentedTypeNew == msg) {
		FskInstrumentedType dispatch = (FskInstrumentedType)data;

		snprintf(buffer, sizeof(buffer), "%s instrumentation type added\n", dispatch->typeName);
		iscWriteLogLine(buffer, level);

		for (isc = gFskInstrumentationSimpleClients; NULL != isc; isc = isc->next) {
			if (0 == FskStrCompare(dispatch->typeName, isc->type)) {
				FskInstrumentionAddTypeListener(dispatch, iscTypeListener, isc, isc->level);
				break;
			}
		}

		if (NULL == isc)
			FskInstrumentationSimpleClientAddType(dispatch->typeName, kFskInstrumentationLevelNone);
	}
	else
	if (kFskInstrumentedDisposedBeforeChildren == msg) {
		FskInstrumentedItem item = (FskInstrumentedItem)data;

		snprintf(buffer, sizeof(buffer), "Warning: Disposing object with active children. %s, id=%p, name=%s\n", item->dispatch->typeName, FskInstrumentationItemToObject(item), item->name);
		iscWriteLogLine(buffer, level);

		for (item = item->children; NULL != item; item = item->ownerNext) {
			snprintf(buffer, sizeof(buffer), "         %s, id=%p, name=%s\n", item->dispatch->typeName, FskInstrumentationItemToObject(item), item->name);
			iscWriteLogLine(buffer, level);
		}
	}
	else
	if (kFskInstrumentedItemPrintf == msg) {
        FskInstrumentationPrintf pr = (FskInstrumentationPrintf)data;
		c_va_list arguments;
		c_va_copy(arguments, *(pr->arguments));
		vsnprintf(buffer, sizeof(buffer) - 1, pr->msg, arguments);
		FskStrCat(buffer, "\n");
		iscWriteLogLine(buffer, level);
	}
}

void iscTypeListener(void *refcon, UInt32 msg, const void *data, UInt32 level, FskInstrumentedType type)
{
	FskInstrumentationSimpleClient isc = (FskInstrumentationSimpleClient)refcon;
	char buffer[1024];
	char buffer2[1024];
	FskInstrumentedItem item = (FskInstrumentedItem)data;
	void *object;

	switch (msg) {
		case kFskInstrumentedItemNew:
			object = FskInstrumentationItemToObject(item);
			if (gLogFREF == object)
				return;

			if (item->name)
				snprintf(buffer, sizeof(buffer), "new %s %p, name=%s\n", type->typeName, object, item->name);
			else
				snprintf(buffer, sizeof(buffer), "new %s %p\n", type->typeName, object);
			iscWriteLogLine(buffer, level);

			if (isc->level)
				FskInstrumentionAddItemListener(item, iscItemListener, refcon, isc->level);
			break;

		case kFskInstrumentedItemDispose:
			object = FskInstrumentationItemToObject(item);
			if (gLogFREF == object)
				return;

			if (item->name)
				snprintf(buffer, sizeof(buffer), "dispose %s %p, name=%s\n", type->typeName, object, item->name);
			else
				snprintf(buffer, sizeof(buffer), "dispose %s %p\n", type->typeName, object);
			iscWriteLogLine(buffer, level);
			break;

		default:
			if (FskInstrumentedTypeFormatMessage(type, msg, (void *)data, buffer, sizeof(buffer))) {
				snprintf(buffer2, sizeof(buffer2), "%s: %s\n", type->typeName, buffer);
				iscWriteLogLine(buffer2, level);
			}
			break;
	}
}

void iscItemListener(void *refcon, UInt32 msg, const void *data, UInt32 level, FskInstrumentedItem item)
{
	char buffer[1024];
	char buffer2[1024];

	if (FskInstrumentedItemFormatMessage(item, msg, (void *)data, buffer, sizeof(buffer))) {
		snprintf(buffer2, sizeof(buffer2), "%s %p: %s\n", item->dispatch->typeName, FskInstrumentationItemToObject(item), buffer);
		iscWriteLogLine(buffer2, level);
	}
}

#define kFskSysLogSocket (514)

void iscWriteLogLine(const char *line, UInt32 level)
{
	FskThread thread = (gThreads || gSysLog) ? FskThreadGetCurrent() : NULL;
	if (!line)
		return;

	if (gSysLog) {
		static FskSocket syslog = NULL;
		char buffer[1000];

		if (!syslog) {
			FskNetSocketNewUDP(&syslog, "instrumentation syslog");
			if (syslog) {
//				FskNetSocketBind(syslog, -1, kFskSysLogSocket);				// disabled - because if we bind to this socket, then the syslog daemon running on this machine can't bind to it
				FskNetSocketMakeNonblocking(syslog);
			}
		}
		if (syslog) {
			int sentLength, bufferLen;

			if (thread && thread->name[0])
				bufferLen = snprintf(buffer, sizeof(buffer), "[%s] %s", thread->name, line);
			else
				bufferLen = snprintf(buffer, sizeof(buffer), "%s", line);

			FskNetSocketSendUDP(syslog, (void *)buffer, bufferLen, &sentLength, gSysLog, kFskSysLogSocket);
		}
	}

	if (gThreads || gTimes) {
		FskTimeRecord time;
		char buffer[128];
        int bufferLen;

		if (gThreads && gTimes) {
			FskTimeGetNow(&time);
			if (thread && thread->name[0])
				bufferLen = snprintf(buffer, sizeof(buffer), "%ld.%06ld [%s] ", time.seconds, time.useconds, thread->name);
			else
				bufferLen = snprintf(buffer, sizeof(buffer), "%ld.%06ld [%p] ", time.seconds, time.useconds, thread);
		}
		else
		if (gThreads) {
			if (thread && thread->name[0])
				bufferLen = snprintf(buffer, sizeof(buffer), "[%s] ", thread->name);
			else
				bufferLen = snprintf(buffer, sizeof(buffer), "[%p] ", thread);
		}
		else {
			FskTimeGetNow(&time);
			bufferLen = snprintf(buffer, sizeof(buffer), "%ld.%06ld ", time.seconds, time.useconds);
		}

		if (gTrace)
			fprintf(stderr, "%s", buffer);
		if (gLogFREF)
			FskFileWrite(gLogFREF, bufferLen, buffer, NULL);
	}

	if (gTrace)
		fprintf(stderr, "%s", line);
	if (gLogFREF)
		FskFileWrite(gLogFREF, FskStrLen(line), line, NULL);

#if TARGET_OS_ANDROID
	if (gAndroidlog) {
		UInt32 androidLevel;
		if (level & kFskInstrumentationLevelUpToMinimal)
			androidLevel = ANDROID_LOG_ERROR;
		else if (level & kFskInstrumentationLevelUpToVerbose)
			androidLevel = ANDROID_LOG_INFO;
		else		// kFskInstrumentationLevelUpToDebug
			androidLevel = ANDROID_LOG_DEBUG;
		(gAndroidCallbacks->logPrintCB)(androidLevel, "kinoma", line, 0);			// last parameter unused
	}
#endif
}

static void dumpItem(FskInstrumentedItem, UInt32 level, char *buffer, SInt32 bufferSize);

void FskInstrumentationSimpleClientDump(const char *header)
{
	FskInstrumentedItem item;
	char buffer[1024];

	iscWriteLogLine(header, kFskInstrumentationLevelMinimal);

	FskListMutexAcquireMutex_uninstrumented(gInstrumentedItems);

	for (item = (FskInstrumentedItem)gInstrumentedItems->list; NULL != item; item = item->ownerNext)
		dumpItem(item, 0, buffer, sizeof(buffer));

	FskListMutexReleaseMutex_uninstrumented(gInstrumentedItems);
}

void dumpItem(FskInstrumentedItem item, UInt32 level, char *buffer, SInt32 bufferSize)
{
	UInt32 i;
	char *p = buffer;
	void *object = FskInstrumentationItemToObject(item);

	if (object == gLogFREF)
		return;

	for (i = 0; i < level; i++) {
		*p++ = ' ';
		*p++ = ' ';
	}

	if (item->name)
		snprintf(p, bufferSize, "%s: %p %s\n", item->dispatch->typeName, object, item->name);
	else
		snprintf(p, bufferSize, "%s: %p\n", item->dispatch->typeName, object);
	if (gTrace)
		fprintf(stderr, "%s", buffer);
	if (gLogFREF)
		FskFileWrite(gLogFREF, FskStrLen(buffer), buffer, NULL);

	for (item = item->children; NULL != item; item = item->ownerNext)
		dumpItem(item, level + 1, buffer, bufferSize);
}

#if SUPPORT_MEMORY_DEBUG

void FskInstrumentationSimpleClientDumpMemory(void)
{
	FskMemoryDebug list = FskMemoryDebugGetList(), debug;
	char buffer[1024];
	UInt32 total = 0;

	iscWriteLogLine("\n** memory in use **\n", kFskInstrumentationLevelMinimal);

	for (debug = list; debug; debug = debug->next) {
#if !FSK_EMBED
		snprintf(buffer, sizeof(buffer), "address %p, size=%lu, seed=%lu\n", (debug + 1), debug->size, debug->seed);
#else
		snprintf(buffer, sizeof(buffer), "address %p, size=%lu, seed=%lu, caller=%s @ line=%lu\n", (debug + 1), debug->size, debug->seed, debug->function, debug->line);
#endif
		total += debug->size;
		iscWriteLogLine(buffer, kFskInstrumentationLevelMinimal);
	}

	snprintf(buffer, sizeof(buffer), "\n** %lu total bytes leaked **\n", total);
	iscWriteLogLine(buffer, kFskInstrumentationLevelMinimal);
}
#else

void FskInstrumentationSimpleClientDumpMemory(void) {}

#endif

#endif		// SUPPORT_INSTRUMENTATION


#if SUPPORT_INSTRUMENTATION || SUPPORT_XS_DEBUG

typedef struct LookupEntry {
	int			code;
	const char	*name;
} LookupEntry;

static const char* LookupNameFromCode(const LookupEntry *table, FskErr code) {
	for (; table->name != NULL; ++table)
		if (table->code == code)
			break;
	return table->name;
}
#define LOOKUP_ENTRY(x)	{ (int)(x), #x }

const char *FskInstrumentationGetErrorString(FskErr err)
{
	// sed -e '/kFskErr/!d' -e/kFskErrNone/d -e 's/[ 	]*\(kFskErr[^ 	]*\)[ 	].*/		LOOKUP_ENTRY(\1),/' "$F_HOME/core/base/FskErrors.h"
	static const LookupEntry tab[] = {
		LOOKUP_ENTRY(kFskErrNone),
		LOOKUP_ENTRY(kFskErrMemFull),
		LOOKUP_ENTRY(kFskErrOutOfSequence),
		LOOKUP_ENTRY(kFskErrBadState),
		LOOKUP_ENTRY(kFskErrOperationFailed),
		LOOKUP_ENTRY(kFskErrIteratorComplete),
		LOOKUP_ENTRY(kFskErrInvalidParameter),
		LOOKUP_ENTRY(kFskErrScript),
		LOOKUP_ENTRY(kFskErrUnimplemented),
		LOOKUP_ENTRY(kFskErrUnsupportedPixelType),
		LOOKUP_ENTRY(kFskErrDuplicateElement),
		LOOKUP_ENTRY(kFskErrUnknownElement),
		LOOKUP_ENTRY(kFskErrBadData),
		LOOKUP_ENTRY(kFskErrShutdown),
		LOOKUP_ENTRY(kFskErrIsBusy),
		LOOKUP_ENTRY(kFskErrNotFound),
		LOOKUP_ENTRY(kFskErrOperationCancelled),
		LOOKUP_ENTRY(kFskErrExtensionNotFound),
		LOOKUP_ENTRY(kFskErrCodecNotFound),
		LOOKUP_ENTRY(kFskErrAudioOutReset),
		LOOKUP_ENTRY(kFskErrCannotDecrypt),
		LOOKUP_ENTRY(kFskErrUnknown),
		LOOKUP_ENTRY(kFskErrTooManyOpenFiles),
		LOOKUP_ENTRY(kFskErrFileNotFound),
		LOOKUP_ENTRY(kFskErrFilePermissions),
		LOOKUP_ENTRY(kFskErrFileNotOpen),
		LOOKUP_ENTRY(kFskErrParameterError),
		LOOKUP_ENTRY(kFskErrIsDirectory),
		LOOKUP_ENTRY(kFskErrNotDirectory),
		LOOKUP_ENTRY(kFskErrReadOnly),
		LOOKUP_ENTRY(kFskErrDiskFull),
		LOOKUP_ENTRY(kFskErrEndOfFile),
		LOOKUP_ENTRY(kFskErrEndOfDirectory),
		LOOKUP_ENTRY(kFskErrOutOfRange),
		LOOKUP_ENTRY(kFskErrFileExists),
		LOOKUP_ENTRY(kFskErrFileNeedsRecovery),
		LOOKUP_ENTRY(kFskErrFileRecoveryImpossible),
		LOOKUP_ENTRY(kFskErrVolumeLocked),
		LOOKUP_ENTRY(kFskErrVolumeUnavailable),
		LOOKUP_ENTRY(kFskErrNameLookupFailed),
		LOOKUP_ENTRY(kFskErrBadSocket),
		LOOKUP_ENTRY(kFskErrSocketNotConnected),
		LOOKUP_ENTRY(kFskErrConnectionRefused),
		LOOKUP_ENTRY(kFskErrNoData),
		LOOKUP_ENTRY(kFskErrAddressInUse),
		LOOKUP_ENTRY(kFskErrNoNetworkInterfaces),
		LOOKUP_ENTRY(kFskErrNetworkInterfaceError),
		LOOKUP_ENTRY(kFskErrNetworkInterfaceNotFound),
		LOOKUP_ENTRY(kFskErrTimedOut),
		LOOKUP_ENTRY(kFskErrNetworkInterfacesChanged),
		LOOKUP_ENTRY(kFskErrSocketFull),
		LOOKUP_ENTRY(kFskErrNoMoreSockets),
		LOOKUP_ENTRY(kFskErrConnectionClosed),
		LOOKUP_ENTRY(kFskErrWaitingForSocket),
		LOOKUP_ENTRY(kFskErrConnectionDropped),
		LOOKUP_ENTRY(kFskErrSSLHandshakeFailed),
		LOOKUP_ENTRY(kFskErrSSLServerAuthFailed),
		LOOKUP_ENTRY(kFskErrAuthFailed),
		LOOKUP_ENTRY(kFskErrAuthPending),
		LOOKUP_ENTRY(kFskErrNetworkInterfaceRemoved),
		LOOKUP_ENTRY(kFskErrNeedConnectionSelection),
		LOOKUP_ENTRY(kFskErrNetworkErr),
		LOOKUP_ENTRY(kFskErrBadURLNoProtocol),
		LOOKUP_ENTRY(kFskErrNeedMoreData),
		LOOKUP_ENTRY(kFskErrNeedMoreTime),
		LOOKUP_ENTRY(kFskErrURLTooLong),
		LOOKUP_ENTRY(kFskErrHostDoesntMatch),
		LOOKUP_ENTRY(kFskErrProtocolDoesntMatch),
		LOOKUP_ENTRY(kFskErrRequestTooLarge),
		LOOKUP_ENTRY(kFskErrRequestAborted),
		LOOKUP_ENTRY(kFskErrTooManyRedirects),
		LOOKUP_ENTRY(kFskErrUnsupportedSchema),
		LOOKUP_ENTRY(kFskErrUnsupportedMIME),
		LOOKUP_ENTRY(kFskErrUnsupportedSeek),
		LOOKUP_ENTRY(kFskErrItemNotFound),
		LOOKUP_ENTRY(kFskErrRTSPBadPacket),
		LOOKUP_ENTRY(kFskErrRTSPBadPacketParser),
		LOOKUP_ENTRY(kFskErrRTSPPacketParserUnsupportedFormat),
		LOOKUP_ENTRY(kFskErrRTSPBadSDPParam),
		LOOKUP_ENTRY(kFskErrRTSPSessionBadState),
		LOOKUP_ENTRY(kFskErrRTSPReceiverReportErr),
		LOOKUP_ENTRY(kFskErrRTSPNoMediaStreams),
		LOOKUP_ENTRY(kFskErrRTSPSocketConfigFailure),
		LOOKUP_ENTRY(kFskErrRTSPSessionRedirect),
		LOOKUP_ENTRY(kFskErrRTSPSessionBadURL),
		LOOKUP_ENTRY(kFskErrRTSPNoUDPPackets),
		LOOKUP_ENTRY(kFskErrEmpty),
		LOOKUP_ENTRY(kFskErrFull),
		LOOKUP_ENTRY(kFskErrNoSubpath),
		LOOKUP_ENTRY(kFskErrBufferOverflow),
		LOOKUP_ENTRY(kFskErrSingular),
		LOOKUP_ENTRY(kFskErrMismatch),
		LOOKUP_ENTRY(kFskErrTooMany),
		LOOKUP_ENTRY(kFskErrNotAccelerated),
		LOOKUP_ENTRY(kFskErrGraphicsContext),
		LOOKUP_ENTRY(kFskErrTextureTooLarge),
		LOOKUP_ENTRY(kFskErrGLInvalidEnum),
		LOOKUP_ENTRY(kFskErrGLInvalidValue),
		LOOKUP_ENTRY(kFskErrGLInvalidOperation),
		LOOKUP_ENTRY(kFskErrGLStackOverflow),
		LOOKUP_ENTRY(kFskErrGLStackUnderflow),
		LOOKUP_ENTRY(kFskErrGLOutOfMemory),
		LOOKUP_ENTRY(kFskErrGLShader),
		LOOKUP_ENTRY(kFskErrGLProgram),
		LOOKUP_ENTRY(kFskErrGLTableTooLarge),
		LOOKUP_ENTRY(kFskErrGLFramebufferUnsupported),
		LOOKUP_ENTRY(kFskErrGLFramebufferUndefined),
		LOOKUP_ENTRY(kFskErrGLInvalidFramebufferOperation),
		LOOKUP_ENTRY(kFskErrGLFramebufferIncompleteAttachment),
		LOOKUP_ENTRY(kFskErrGLFramebufferIncompleteMissingAttachment),
		LOOKUP_ENTRY(kFskErrGLFramebufferIncompleteDimensions),
		LOOKUP_ENTRY(kFskErrGLFramebufferIncompleteFormats),
		LOOKUP_ENTRY(kFskErrGLFramebufferIncompleteDrawBuffer),
		LOOKUP_ENTRY(kFskErrGLFramebufferIncompleteReadBuffer),
		LOOKUP_ENTRY(kFskErrGLFramebufferIncompleteLayerTargets),
		LOOKUP_ENTRY(kFskErrGLFramebufferIncompleteMultisample),
		LOOKUP_ENTRY(kFskErrEGLNotInitialized),
		LOOKUP_ENTRY(kFskErrEGLBadAccess),
		LOOKUP_ENTRY(kFskErrEGLBadAlloc),
		LOOKUP_ENTRY(kFskErrEGLBadAttribute),
		LOOKUP_ENTRY(kFskErrEGLBadConfig),
		LOOKUP_ENTRY(kFskErrEGLBadContext),
		LOOKUP_ENTRY(kFskErrEGLCurrentSurface),
		LOOKUP_ENTRY(kFskErrEGLBadDisplay),
		LOOKUP_ENTRY(kFskErrEGLBadMatch),
		LOOKUP_ENTRY(kFskErrEGLBadNativePixmap),
		LOOKUP_ENTRY(kFskErrEGLBadNativeWindow),
		LOOKUP_ENTRY(kFskErrEGLBadParameter),
		LOOKUP_ENTRY(kFskErrEGLBadSurface),
		LOOKUP_ENTRY(kFskErrEGLContextLost),
		LOOKUP_ENTRY(kFskErrCGLBadAttribute),
		LOOKUP_ENTRY(kFskErrCGLBadProperty),
		LOOKUP_ENTRY(kFskErrCGLBadPixelFormat),
		LOOKUP_ENTRY(kFskErrCGLBadRendererInfo),
		LOOKUP_ENTRY(kFskErrCGLBadContext),
		LOOKUP_ENTRY(kFskErrCGLBadDrawable),
		LOOKUP_ENTRY(kFskErrCGLBadDisplay),
		LOOKUP_ENTRY(kFskErrCGLBadState),
		LOOKUP_ENTRY(kFskErrCGLBadValue),
		LOOKUP_ENTRY(kFskErrCGLBadMatch),
		LOOKUP_ENTRY(kFskErrCGLBadEnumeration),
		LOOKUP_ENTRY(kFskErrCGLBadOffScreen),
		LOOKUP_ENTRY(kFskErrCGLBadFullScreen),
		LOOKUP_ENTRY(kFskErrCGLBadWindow),
		LOOKUP_ENTRY(kFskErrCGLBadAddress),
		LOOKUP_ENTRY(kFskErrCGLBadCodeModule),
		LOOKUP_ENTRY(kFskErrCGLBadAlloc),
		LOOKUP_ENTRY(kFskErrCGLBadConnection),
		LOOKUP_ENTRY(kFskErrEAGLBadContext),
		LOOKUP_ENTRY(kFskErrNothingRendered),
		LOOKUP_ENTRY(kFskErrUnalignedYUV),
		LOOKUP_ENTRY(kFskErrGLFramebufferComplete),
		{	0,	NULL	}
	};
	const char *str = LookupNameFromCode(tab, err);
    return str ? str : "(unknown or new error)";
}

const char *FskInstrumentationCleanPath(const char *fullPath)
{
    const char *relativePath;
    static Boolean initialized = false;
    static const char *gCleanPathStart;

    if (!initialized) {
        initialized = true;
        gCleanPathStart = FskStrDoCopy(FskEnvironmentGet("cleanPathStart"));
    }

    if (NULL == gCleanPathStart)
        return fullPath;

    relativePath = FskStrStr(fullPath, gCleanPathStart);
    return relativePath ? relativePath + 1 : fullPath;
}

#endif
