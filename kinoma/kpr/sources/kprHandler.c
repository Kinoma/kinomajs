/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#include "FskHeaders.h"

#include "kprApplication.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHandler.h"
#include "kprHTTPClient.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprSkin.h"
#include "kprURL.h"

typedef struct {
	KprHandler handler;
	KprMessage message;
	FskTimeCallBack callback;
} KprHandlerTargetRecord, *KprHandlerTarget;

typedef struct {
	KprStreamPart;
	char* path;
	FskFile file;
	UInt32 offset;
	UInt32 size;
} KprDownloadStreamRecord, *KprDownloadStream;

typedef struct KprUploadChunkStruct KprUploadChunkRecord, *KprUploadChunk;

struct KprUploadChunkStruct {
	KprUploadChunk next;
	UInt32 offset;
	UInt32 size;
	char* data;
};

typedef struct {
	KprStreamPart;
	char* path;
	FskFile file;
	UInt32 offset;
	UInt32 prefix;
	UInt32 suffix;
	UInt32 size;
	char buffer[8192];
} KprUploadStreamRecord, *KprUploadStream;

typedef struct {
	KprStreamPart;
	KprUploadChunk chunk;
	Boolean chunkDone;
	UInt32 offset;
	UInt32 size;
	char buffer[8192];
} KprUploadChunkStreamRecord, *KprUploadChunkStream;

static void KprHandlerAccept(KprHandler self, KprMessage message);
static void KprHandlerDispose(void* it);
static void KprHandlerInvoke(KprHandler self, KprMessage message);
static FskErr KprHandlerTargetNew(KprHandlerTarget* it, KprHandler handler, KprMessage message, FskTimeCallBack callback);
static void KprHandlerTargetDispose(void* it);
static void KprHandlerTargetRequestCallback(KprMessage message, void* it);
static void KprHandlerTargetResponseCallback(KprMessage message, void* it);

static FskErr KprDownloadStreamNew(KprDownloadStream* it, char* url);
static void KprDownloadStreamDispose(void* it);
static void KprDownloadStreamOnReceive(KprMessage message, char* data, int size);
static void KprDownloadStreamOnProgress(KprMessage message, void* offset, void* size);
static FskErr KprDownloadStreamReceive(void* it, KprMessage message, xsMachine* machine UNUSED, char* data, int size);
static void KprDownloadStreamTransform(void* it, KprMessage message, xsMachine* machine UNUSED);

static FskErr KprUploadStreamNew(KprUploadStream* it, char* url, int at, KprMessage message);
static void KprUploadStreamDispose(void* it);
static void KprUploadStreamOnProgress(KprMessage message, void* offset, void* size);
static FskErr KprUploadStreamSend(void* it, KprMessage message, xsMachine* machine UNUSED, char** data, int* size);
static void KprUploadStreamTransform(void* it, KprMessage message, xsMachine* machine UNUSED);

static FskErr KprUploadChunkStreamNew(KprUploadChunkStream* it, KprMessage message);
static void KprUploadChunkStreamDispose(void* it);
static Boolean KprUploadChunkStreamNotReconnectable(void* it, KprMessage message);
static void KprUploadChunkStreamAddChunk(KprMessage message, void* size, void* data);
static void KprUploadChunkStreamOnProgress(KprMessage message, void* offset, void* size);
static FskErr KprUploadChunkStreamSend(void* it, KprMessage message, xsMachine* machine UNUSED, char** data, int* dataSize);
static void KprUploadChunkStreamTransform(void* it, KprMessage message, xsMachine* machine UNUSED);

void KprContextAccept(void* it, KprMessage message)
{
	KprContext self = it;
	KprHandler handler = KprContextGetHandler(it, &(message->parts));
	if (handler)
		KprHandlerAccept(handler, message);
	else
		kprDelegateAccept(self, message);
}

void KprContextDisposeHandlers(void* it)
{
	KprContext self = it;
	KprHandler handler = self->firstHandler;
	while (handler) {
		KprHandler next = handler->next;
		handler->shell = NULL;
		handler->container = NULL;
		handler->previous = NULL;
		handler->next = NULL;
		FskInstrumentedItemClearOwner(handler);
		if (!handler->the)
			KprHandlerDispose(handler);
		handler = next;
	}
	self->firstHandler = NULL;
	self->lastHandler = NULL;
}

KprHandler KprContextGetHandler(void* it, KprURLParts parts)
{
	KprContext self = it;
	KprHandler handler = self->firstHandler;
	while (handler) {
		UInt32 length = FskStrLen(handler->path);
		if ((parts->pathLength == length) && (!FskStrCompareWithLength(parts->path, handler->path, length)))
			return handler;
		handler = handler->next;
	}
	return NULL;
}

void KprContextGLContextLost(void* it)
{
	KprContext self = it;
	KprTexture texture = (KprTexture)self->firstTexture;
	while (texture) {
		FskBitmap bitmap = texture->bitmap;
		if (bitmap && FskBitmapIsOpenGLDestinationAccelerated(bitmap))
			KprTexturePurge(texture);
		texture = (KprTexture)texture->next;
	}
}

void KprContextInvoke(void* it, KprMessage message)
{
	KprContext self = it;
	KprHandler handler = KprContextGetHandler(it, &(message->parts));
	if (handler)
		KprHandlerInvoke(handler, message);
	else
		kprDelegateInvoke(self, message);
}

void KprContextMark(void* it, xsMarkRoot markRoot)
{
	KprContext self = it;
	KprHandler handler = self->firstHandler;
	KprTexture texture = (KprTexture)self->firstTexture;
	while (handler) {
		(*handler->dispatch->mark)(handler, markRoot);
		handler = handler->next;
	}
	while (texture) {
		if (texture->content)
			(*texture->content->dispatch->mark)(texture->content, markRoot);
		texture = (KprTexture)texture->next;
	}
	KprContainerMark(it, markRoot);
}

void KprContextPurge(void* it, Boolean flag)
{
	KprContext self = it;
	FskInstrumentedItemSendMessageNormal(self, kprInstrumentedContentBeginPurge, self);
	KprAssetsPurge(&self->firstSkin, flag);
	KprAssetsPurge(&self->firstSound, flag);
	KprAssetsPurge(&self->firstTexture, flag);
	KprAssetsPurge(&self->firstStyle, flag);
	KprAssetsPurge(&self->firstEffect, flag);
	FskInstrumentedItemSendMessageNormal(self, kprInstrumentedContentEndPurge, self);
}

void KprContextPutHandler(void* it, KprHandler handler)
{
	KprContext self = it;
	KprHandler previous = NULL, next = self->firstHandler;
	while (next) {
		if (FskStrCompare(next->path, handler->path) <= 0)
			break;
		previous = next;
		next = next->next;
	}
	handler->shell = self->shell;
	handler->container = it;
	handler->next = next;
	handler->previous = previous;
	if (next)
		next->previous = handler;
	else
		self->lastHandler = handler;
	if (previous)
		previous->next = handler;
	else
		self->firstHandler = handler;
	FskInstrumentedItemSetOwner(handler, self);
}

void KprContextRemoveHandler(void* it, KprHandler handler)
{
	KprContext self = it;
	KprHandler previous = handler->previous;
	KprHandler next = handler->next;
	if (previous)
		previous->next = next;
	else
		self->firstHandler = next;
	if (next)
		next->previous = previous;
	else
		self->lastHandler = previous;
	handler->shell = NULL;
	handler->container = NULL;
	handler->next = NULL;
	handler->previous = NULL;
	FskInstrumentedItemClearOwner(handler);
	if (!handler->the)
		KprHandlerDispose(handler);
}


#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprHandlerInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprHandler", FskInstrumentationOffset(KprHandlerRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprHandlerDispatchRecord = {
	"handler",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	KprContentMark,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
    NULL
};

FskErr KprHandlerNew(KprHandler* it, char* path)
{
	FskErr err = kFskErrNone;
	KprHandler self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprHandlerRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprHandlerInstrumentation);
	self->dispatch = &KprHandlerDispatchRecord;
	if (path) {
		self->path = FskStrDoCopy(path);	
		bailIfNULL(self->path);
	}
bail:
	return err;
}

void KprHandlerAccept(KprHandler self, KprMessage message)
{
	self->message = message;
	kprDelegateAccept(self, message);
	self->message = NULL;
}

void KprHandlerDispose(void* it)
{
	KprHandler self = it;
	FskList messages = gShell->messages;
	KprMessage message = FskListGetNext(messages, NULL);
	while (message) {
        KprMessage next = FskListGetNext(messages, message);
		if (message->request.dispose == KprHandlerTargetDispose) {
			KprHandlerTarget target = message->request.target;
			if (target->handler == self)
				KprMessageCancel(message);
		}
		if (message->response.dispose == KprHandlerTargetDispose) {
			KprHandlerTarget target = message->response.target;
			if (target->handler == self)
				KprMessageDispose(message);
		}
		message = next;
	}	
	FskMemPtrDispose(self->path);
	kprDelegateDispose(self);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

void KprHandlerInvoke(KprHandler self, KprMessage message)
{
	self->message = message;
	kprDelegateInvoke(self, message);
	self->message = NULL;
}

FskErr KprHandlerTargetNew(KprHandlerTarget* it, KprHandler handler, KprMessage message, FskTimeCallBack callback)
{
	FskErr err = kFskErrNone;
	KprHandlerTarget self;
	bailIfError(FskMemPtrNewClear(sizeof(KprHandlerTargetRecord), it));
	self = *it;
	self->handler = handler;
	self->message = message;
	self->callback = callback;
bail:
    return err;
}

void KprHandlerTargetDispose(void* it)
{
	KprHandlerTarget self = it;
	FskTimeCallbackDispose(self->callback);
	FskMemPtrDispose(self);
}

void KprHandlerTargetRequestCallback(KprMessage message, void* it)
{
	KprHandlerTarget target = it;
    KprHandler self = target->handler;
    KprMessage former = target->message;
	KprMessageResuming(former);
	self->message = former;
    kprDelegateComplete(self, message);
	self->message = NULL;
	KprMessageResumed(former);
}

void KprHandlerTargetResponseCallback(KprMessage message, void* it)
{
	KprHandlerTarget target = it;
    KprHandler self = target->handler;
	self->message = message;
    kprDelegateCancel(self, target->message);
	self->message = NULL;
	KprMessageCancel(target->message);
}

FskErr KprHandlerTrigger(KprHandler self, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprHandlerTarget requestTarget = NULL;
	KprHandlerTarget responseTarget = NULL;
	bailIfError(KprHandlerTargetNew(&requestTarget, self, self->message, NULL));
	bailIfError(KprHandlerTargetNew(&responseTarget, self, message, NULL));
	KprMessageSuspend(self->message, KprHandlerTargetResponseCallback, KprHandlerTargetDispose, responseTarget);
	bailIfError(KprMessageInvoke(message, KprHandlerTargetRequestCallback, KprHandlerTargetDispose, requestTarget));
bail:
    return err;
}

/* ECMASCRIPT */

void KPR_handler(void *it)
{
	if (it) {
		KprHandler self = it;
		kprVolatileDestructor(KPR_handler);
		if (!self->container)
			KprHandlerDispose(self);
	}
}

void KPR_Handler(xsMachine* the)
{
	KprHandler self;
	xsStringValue path = xsToString(xsArg(0));
	xsThrowIfFskErr(KprHandlerNew(&self, path));
	kprContentConstructor(KPR_Handler);
}

void KPR_handler_get_message(xsMachine *the)
{
	KprHandler self = xsGetHostData(xsThis);
	KprMessage message = self->message;
	if (message) {
		xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_message));
		xsSetHostData(xsResult, message);
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageConstruct, message);
		message->usage++; // host
	}
}

void KPR_handler_get_path(xsMachine *the)
{
	KprHandler self = xsGetHostData(xsThis);
	if (self->path)
		xsResult = xsString(self->path);
}

void KPR_handler_invokeAux(xsMachine* the, KprHandler self, KprMessage message)
{
	KprHandlerTarget requestTarget = NULL;
	KprHandlerTarget responseTarget = NULL;
	xsThrowIfFskErr(KprHandlerTargetNew(&requestTarget, self, self->message, NULL));
	xsThrowIfFskErr(KprHandlerTargetNew(&responseTarget, self, message, NULL));
	KprMessageSuspend(self->message, KprHandlerTargetResponseCallback, KprHandlerTargetDispose, responseTarget);
	xsThrowIfFskErr(KprMessageInvoke(message, KprHandlerTargetRequestCallback, KprHandlerTargetDispose, requestTarget));
}

void KPR_handler_download(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprHandler self = kprGetHostData(xsThis, this, handler);
	KprMessage message = kprGetHostData(xsArg(0), message, message);
	xsStringValue url = NULL;
    KprDownloadStream stream = NULL;
	if ((c > 1) && xsTest(xsArg(1)))
		url = xsToString(xsArg(1));
	xsThrowIfFskErr(KprDownloadStreamNew(&stream, url));
	KprMessageSetStream(message, (KprStream)stream);
	KPR_handler_invokeAux(the, self, message);
}


void KPR_handler_invoke(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprHandler self = kprGetHostData(xsThis, this, handler);
	KprMessage message = kprGetHostData(xsArg(0), message, message);
	if ((c > 1) && xsTest(xsArg(1))) {
		KprHandlerTarget requestTarget = NULL;
		KprHandlerTarget responseTarget = NULL;
		xsThrowIfFskErr(KprHandlerTargetNew(&requestTarget, self, self->message, NULL));
		xsThrowIfFskErr(KprHandlerTargetNew(&responseTarget, self, message, NULL));
		KprMessageScriptTargetSet(message, the, &xsArg(1));
		KprMessageSuspend(self->message, KprHandlerTargetResponseCallback, KprHandlerTargetDispose, responseTarget);
		xsThrowIfFskErr(KprMessageInvoke(message, KprHandlerTargetRequestCallback, KprHandlerTargetDispose, requestTarget));
	}
	else
		xsThrowIfFskErr(KprMessageInvoke(message, NULL, NULL, NULL));
}

void KPR_handler_redirect(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprHandler self = kprGetHostData(xsThis, this, handler);
	KprMessage message = self->message;
	xsStringValue url = NULL;
	xsStringValue mime = NULL;
	if (message) {
		xsThrowIfFskErr(KprMessageURL(xsGetContext(the), xsToString(xsArg(0)), &url));
		if ((c > 1) && xsTest(xsArg(1)))
			mime = xsToString(xsArg(1));
		KprMessageRedirect(message, url, mime);
		FskMemPtrDispose(url);
	}
}

void KPR_handler_upload(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprHandler self = kprGetHostData(xsThis, this, handler);
	KprMessage message = kprGetHostData(xsArg(0), message, message);
	xsStringValue url = xsToString(xsArg(1));
	xsIntegerValue at = 0;
    KprUploadStream stream = NULL;
	if ((c > 2) && xsTest(xsArg(2)))
		at = xsToInteger(xsArg(2));
	xsThrowIfFskErr(KprUploadStreamNew(&stream, url, at, message));
	KprMessageSetStream(message, (KprStream)stream);
	KPR_handler_invokeAux(the, self, message);
}

void KPR_handler_uploadChunk(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprHandler self = kprGetHostData(xsThis, this, handler);
	KprMessage message = kprGetHostData(xsArg(0), message, message);
    KprUploadChunkStream stream = NULL;
	UInt32 size = 0;
	UInt32 chunkSize = 0;
	char* chunkData = NULL;
	char prefix[32];
	if (!message->stream) {
		xsThrowIfFskErr(KprUploadChunkStreamNew(&stream, message));
		KprMessageSetStream(message, (KprStream)stream);
		KPR_handler_invokeAux(the, self, message);
	}
	if ((c > 1) && xsTest(xsArg(1))) {
		const char* crlf = "\r\n";
		UInt32 offset;
		char* data = NULL;
		xsType aType = xsTypeOf(xsArg(1));
		if ((xsReferenceType == aType) && xsIsInstanceOf(xsArg(1), xsArrayBufferPrototype)) {
			data = xsToArrayBuffer(xsArg(1));
			size = xsGetArrayBufferLength(xsArg(1));
		}
		else {
			size = xsToInteger(xsGet(xsArg(1), xsID_length));
			if (aType == xsStringType)
				data = xsToString(xsArg(1));
			else {
				data = xsGetHostData(xsArg(1));
				xsTrace("Calling Handler.uploadChunk called with Chunk is deprecated. Pass ArrayBuffer.");
			}
		}
		snprintf(prefix, 32, "%X", (unsigned int)size);		//@@
		offset = FskStrLen(prefix);
		chunkSize = size + offset + 4;
		xsThrowIfFskErr(FskMemPtrNewClear(chunkSize, &chunkData));
		FskMemCopy(chunkData, prefix, offset);
		FskMemCopy(chunkData + offset, crlf, 2);
		FskMemCopy(chunkData + offset + 2, data, size);
		FskMemCopy(chunkData + chunkSize - 2, crlf, 2);
	}
	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprUploadChunkStreamAddChunk, message, (void*)chunkSize, (void*)chunkData, NULL);
}

static void KPR_handler_waitCancel(KprMessage message, void* it)
{
	KprHandlerTarget target = it;
    KprHandler self = target->handler;
	self->message = message;
    kprDelegateCancel(self, NULL);
	self->message = NULL;
}

static void KPR_handler_waitComplete(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *it)
{
	KprHandlerTarget target = it;
    KprHandler self = target->handler;
    KprMessage former = target->message;
	KprMessageResuming(former);
	self->message = former;
    kprDelegateComplete(self, NULL);
	self->message = NULL;
	KprMessageResumed(former);
}

void KPR_handler_wait(xsMachine *the)
{
	KprHandler self = kprGetHostData(xsThis, this, handler);
	FskTimeCallBack callback;
	KprHandlerTarget waitTarget;
	FskTimeRecord when;
	FskTimeCallbackNew(&callback);
	xsThrowIfFskErr(KprHandlerTargetNew(&waitTarget, self, self->message, callback));
	KprMessageSuspend(self->message, KPR_handler_waitCancel, KprHandlerTargetDispose, waitTarget);
	FskTimeGetNow(&when);
	FskTimeAddMS(&when, xsToInteger(xsArg(0)));
	FskTimeCallbackSet(callback, &when, KPR_handler_waitComplete, waitTarget);
}

void KPR_Handler_get(xsMachine* the)
{
	KprContext self = xsGetContext(the);
	xsStringValue url = xsToString(xsArg(0));
	KprURLPartsRecord parts;
	KprHandler handler;
	KprURLSplit(url, &parts);
	handler = KprContextGetHandler(self, &parts);
	if (handler)
		xsResult = kprContentGetter(handler);
}

void KPR_Handler_put(xsMachine* the)
{
	KprContext self = xsGetContext(the);
	KprHandler handler = kprGetHostData(xsArg(0), handler, handler);
	xsAssert(handler->container == NULL);
	xsAssert(handler->previous == NULL);
	xsAssert(handler->next == NULL);
	KprContextPutHandler(self, handler);
}

void KPR_Handler_remove(xsMachine* the)
{
	KprContext self = xsGetContext(the);
	KprHandler handler = kprGetHostData(xsArg(0), handler, handler);
	xsAssert(handler->container == (KprContainer)self);
	KprContextRemoveHandler(self, handler);
}

void KPR_Handler_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("Handler"));
	xsNewHostProperty(xsResult, xsID("get"), xsNewHostFunction(KPR_Handler_get, 1), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("put"), xsNewHostFunction(KPR_Handler_put, 1), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("remove"), xsNewHostFunction(KPR_Handler_remove, 1), xsDefault, xsDontScript);
}

/* DOWNLOAD */

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprDownloadStreamInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprDownloadStream", FskInstrumentationOffset(KprDownloadStreamRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
KprStreamDispatchRecord kprDownloadStreamDispatchRecord = {
	KprDownloadStreamDispose,
	NULL,
	KprDownloadStreamReceive,
	NULL,
	KprDownloadStreamTransform
};

FskErr KprDownloadStreamNew(KprDownloadStream* it, char* url)
{
	FskErr err = kFskErrNone;
	KprDownloadStream self;
	bailIfError(FskMemPtrNewClear(sizeof(KprDownloadStreamRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprDownloadStreamInstrumentation);
	self->dispatch = &kprDownloadStreamDispatchRecord;
	if (url) {
		bailIfError(KprURLToPath(url, &self->path));
	}
bail:
	return err;
}

void KprDownloadStreamDispose(void* it)
{
	if (it) {
		KprDownloadStream self = it;
		FskMemPtrDispose(self->path);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprDownloadStreamOnProgress(KprMessage message, void* offset, void* size)
{
	KprHandlerTarget target = message->request.target;
	if (target) {
		KprHandler handler = target->handler;
		KprScriptBehavior self = (KprScriptBehavior)handler->behavior;
		xsBeginHostSandboxCode(self->the, self->code);
		xsVars(3);
		xsVar(0) = xsAccess(self->slot);
		if (xsFindResult(xsVar(0), xsID("onProgress"))) {
			handler->message = target->message;
			xsVar(1) = kprContentGetter(handler);
			if (message) {
				xsVar(2) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_message));
				xsSetHostData(xsVar(2), message);
				FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageConstruct, message);
				message->usage++; // host
			}
			(void)xsCallFunction4(xsResult, xsVar(0), xsVar(1), xsVar(2), xsInteger((SInt32)offset), xsInteger((SInt32)size));
			handler->message = NULL;
		}
		xsEndHostSandboxCode();
	}
}

void KprDownloadStreamOnReceive(KprMessage message, char* data, int size)
{
	KprHandlerTarget target = message->request.target;
	if (target) {
		KprHandler handler = target->handler;
		KprScriptBehavior self = (KprScriptBehavior)handler->behavior;
		xsDestructor destructor;
		xsBeginHostSandboxCode(self->the, self->code);
		xsVars(4);
		xsVar(0) = xsAccess(self->slot);
		xsVar(3) = xsNewInstanceOf(xsChunkPrototype);
		xsSetHostData(xsVar(3), data);
		destructor = xsGetHostDestructor(xsVar(3));
		xsSetHostDestructor(xsVar(3), NULL);
		xsSet(xsVar(3), xsID("length"), xsInteger(size));
		xsSetHostDestructor(xsVar(3), destructor);
		if (xsFindResult(xsVar(0), xsID("onReceive"))) {
			handler->message = target->message;
			xsVar(1) = kprContentGetter(handler);
			xsVar(2) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_message));
			xsSetHostData(xsVar(2), message);
			FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageConstruct, message);
			message->usage++; // host
			(void)xsCallFunction3(xsResult, xsVar(0), xsVar(1), xsVar(2), xsVar(3));
			handler->message = NULL;
		}
		xsEndHostSandboxCode();
	}
}

FskErr KprDownloadStreamReceive(void* it, KprMessage message, xsMachine* machine UNUSED, char* data, int size)
{
	FskErr err = kFskErrNone;
	KprDownloadStream self = it;
	if (self->path) {
		FskFile file = self->file;
		if (!file) {
			char* value = KprMessageGetResponseHeader(message, kFskStrContentLength);
			if (value)
				self->size = atoi(value);
			err = FskFileOpen(self->path, kFskFilePermissionReadWrite, &file);
			if (kFskErrFileNotFound == err) {
				bailIfError(FskFileCreate(self->path));
				err = FskFileOpen(self->path, kFskFilePermissionReadWrite, &file);
			}
			bailIfError(err);
			self->file = file;
		}
		bailIfError(FskFileWrite(file, size, data, NULL));
		self->offset += size;
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprDownloadStreamOnProgress, message, (void*)self->offset, (void*)self->size, NULL);
	}
	else {
		UInt8* buffer = NULL;
		bailIfError(FskMemPtrNew(size, &buffer));
		FskMemCopy(buffer, data, size);
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprDownloadStreamOnReceive, message, buffer, (void*)size, NULL);
	}
bail:
	return err;
}

void KprDownloadStreamTransform(void* it, KprMessage message UNUSED, xsMachine* machine UNUSED)
{
	KprDownloadStream self = it;
	FskFile file = self->file;
	if (file) {
		FskFileClose(file);
		self->file = NULL;
	}
}

/* UPLOAD */

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprUploadStreamInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUploadStream", FskInstrumentationOffset(KprUploadStreamRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
KprStreamDispatchRecord kprUploadStreamDispatchRecord = {
	KprUploadStreamDispose,
	NULL,
	NULL,
	KprUploadStreamSend,
	KprUploadStreamTransform
};

FskErr KprUploadStreamNew(KprUploadStream* it, char* url, int at, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprUploadStream self;
	FskInt64 fileSize;
	char value[16];
	bailIfError(FskMemPtrNewClear(sizeof(KprUploadStreamRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprUploadStreamInstrumentation);
	self->dispatch = &kprUploadStreamDispatchRecord;
	
	if (!message->request.body)
		bailIfError(FskMemPtrNewClear(0, &message->request.body));
	
	bailIfError(KprURLToPath(url, &self->path));
	bailIfError(FskFileOpen(self->path, kFskFilePermissionReadOnly, &self->file));
	bailIfError(FskFileGetSize(self->file, &fileSize));
	if (fileSize > (0x7FFFFFFF - message->request.size))
		BAIL(kFskErrOutOfRange);
	self->prefix = at;
	self->suffix = at + (SInt32)fileSize;
	self->size = message->request.size + (SInt32)fileSize;
	FskStrNumToStr(self->size, value, sizeof(value));
	(void)KprMessageSetRequestHeader(message, kFskStrContentLength, value);
bail:
	return err;
}

void KprUploadStreamDispose(void* it)
{
	if (it) {
		KprUploadStream self = it;
		if (self->file) {
			FskFileClose(self->file);
			self->file = NULL;
		}
		FskMemPtrDispose(self->path);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprUploadStreamOnProgress(KprMessage message, void* offset, void* size)
{
	KprHandlerTarget target = message->request.target;
	if (target) {
		KprHandler handler = target->handler;
		KprScriptBehavior self = (KprScriptBehavior)handler->behavior;
		xsBeginHostSandboxCode(self->the, self->code);
		xsVars(3);
		xsVar(0) = xsAccess(self->slot);
		if (xsFindResult(xsVar(0), xsID("onProgress"))) {
			handler->message = target->message;
			xsVar(1) = kprContentGetter(handler);
			xsVar(2) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_message));
			xsSetHostData(xsVar(2), message);
			FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageConstruct, message);
			message->usage++; // host
			(void)xsCallFunction4(xsResult, xsVar(0), xsVar(1), xsVar(2), xsInteger((SInt32)offset), xsInteger((SInt32)size));
			handler->message = NULL;
		}
		xsEndHostSandboxCode();
	}
}

FskErr KprUploadStreamSend(void* it, KprMessage message, xsMachine* machine UNUSED, char** data, int* dataSize)
{
	FskErr err = kFskErrNone;
	KprUploadStream self = it;
	SInt32 offset = 0;
	SInt32 size = sizeof(self->buffer);
	SInt32 remaining = self->prefix - self->offset;
	if (remaining > 0) {
		if (remaining > size)
			remaining = size;
		FskMemCopy(self->buffer, (UInt8*)message->request.body + self->offset, remaining);
		self->offset += remaining;
		offset += remaining;
		size -= remaining;
	}
	if (size > 0) {
		remaining = self->suffix - self->offset;
		if (remaining > 0) {
			if (remaining > size)
				remaining = size;
			bailIfError(FskFileRead(self->file, remaining, self->buffer + offset, NULL));
			self->offset += remaining;
			offset += remaining;
			size -= remaining;
		}
	}
	if (size > 0) {
		remaining = self->size - self->offset;
		if (remaining > 0) {
			if (remaining > size)
				remaining = size;
			FskMemCopy((UInt8*)self->buffer + offset, (UInt8*)message->request.body + self->prefix + self->offset - self->suffix, remaining);
			self->offset += remaining;
			offset += remaining;
		}
	}
	*data = self->buffer;
	*dataSize = offset;
	if (offset == 0) {
		FskInt64 zero = 0;
		err = kFskErrEndOfFile;
		self->offset = 0;
		FskFileSetPosition(self->file, &zero);
	}
	else
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprUploadStreamOnProgress, message, (void*)self->offset, (void*)self->size, NULL);
bail:
	return err;
}

void KprUploadStreamTransform(void* it, KprMessage message UNUSED, xsMachine* machine UNUSED)
{
	KprUploadStream self = it;
	FskFile file = self->file;
	if (file) {
		FskFileClose(file);
		self->file = NULL;
	}
}

/* UPLOAD CHUNK */

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprUploadChunkStreamInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprUploadChunkStream", FskInstrumentationOffset(KprUploadChunkStreamRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
KprStreamDispatchRecord kprUploadChunkStreamDispatchRecord = {
	KprUploadChunkStreamDispose,
	KprUploadChunkStreamNotReconnectable,
	NULL,
	KprUploadChunkStreamSend,
	KprUploadChunkStreamTransform
};

FskErr KprUploadChunkStreamNew(KprUploadChunkStream* it, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprUploadChunkStream self;
	bailIfError(FskMemPtrNewClear(sizeof(KprUploadChunkStreamRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprUploadChunkStreamInstrumentation);
	
	if (!message->request.body)
		bailIfError(FskMemPtrNewClear(0, &message->request.body));
	(void)KprMessageSetRequestHeader(message, kFskStrTransferEncoding, kFskStrChunked);
	self->dispatch = &kprUploadChunkStreamDispatchRecord;
bail:
	return err;
}

void KprUploadChunkStreamDispose(void* it)
{
	if (it) {
		KprUploadChunkStream self = it;
		KprUploadChunk chunk = self->chunk;
		while (chunk) {
			KprUploadChunk nextChunk = chunk->next;
			FskMemPtrDispose(chunk->data);
			FskMemPtrDispose(chunk);
			chunk = nextChunk;
		}
		self->chunk = NULL;
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
	return;
}

static Boolean KprUploadChunkStreamNotReconnectable(void* it UNUSED, KprMessage message UNUSED)
{
	return false;
}

static void KprUploadChunkStreamAddChunk(KprMessage message, void* size, void* data)
{
	FskErr err = kFskErrNone;
	KprUploadChunkStream self = (KprUploadChunkStream)message->stream;
	KprUploadChunk chunk = NULL;
	if (self->chunkDone) {
		// stream chunk is done, dispose data
		FskMemPtrDispose(data);
	}
	else {
		bailIfError(FskMemPtrNewClear(sizeof(KprUploadChunkRecord), &chunk));
		if (data) {
			chunk->size = (UInt32)size;
			chunk->data = data;
		}
		else {
			char* prefix = "0\r\n\r\n\r\n";
			chunk->size = FskStrLen(prefix);
			bailIfError(FskMemPtrNewFromData(chunk->size, prefix, &chunk->data));
			self->chunkDone = true;
		}
		FskListAppend(&self->chunk, chunk);
	}
bail:
	return;
}

void KprUploadChunkStreamOnProgress(KprMessage message, void* offset, void* size)
{
	KprHandlerTarget target = message->request.target;
	if (target) {
		KprHandler handler = target->handler;
		KprScriptBehavior self = (KprScriptBehavior)handler->behavior;
		xsBeginHostSandboxCode(self->the, self->code);
		xsVars(3);
		xsVar(0) = xsAccess(self->slot);
		if (xsFindResult(xsVar(0), xsID("onProgress"))) {
			handler->message = target->message;
			xsVar(1) = kprContentGetter(handler);
			xsVar(2) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_message));
			xsSetHostData(xsVar(2), message);
			FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageConstruct, message);
			message->usage++; // host
			(void)xsCallFunction4(xsResult, xsVar(0), xsVar(1), xsVar(2), xsInteger((SInt32)offset), xsInteger((SInt32)size));
			handler->message = NULL;
		}
		xsEndHostSandboxCode();
	}
}

FskErr KprUploadChunkStreamSend(void* it, KprMessage message, xsMachine* machine UNUSED, char** data, int* dataSize)
{
	FskErr err = kFskErrNone;
	KprUploadChunkStream self = it;
	SInt32 offset = 0;
	SInt32 size = sizeof(self->buffer);
	SInt32 remaining;
	while ((size > 0) && self->chunk) {
		KprUploadChunk chunk = self->chunk;
		remaining = chunk->size - chunk->offset;
		if (remaining > size)
			remaining = size;
		FskMemCopy(self->buffer + offset, (UInt8*)chunk->data + chunk->offset, remaining);
		chunk->offset += remaining;
		self->offset += remaining;
		offset += remaining;
		size -= remaining;
		if (chunk->offset == chunk->size) {
			self->chunk = chunk->next;
			FskMemPtrDispose(chunk->data);
			FskMemPtrDispose(chunk);
		}
	}
	*data = self->buffer;
	*dataSize = offset;
	if (offset == 0) {
		if (self->chunkDone) {
			self->offset = 0;
			err = kFskErrEndOfFile;
		}
	}
	else
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprUploadChunkStreamOnProgress, message, (void*)self->offset, (void*)self->size, NULL);
//bail:
	return err;
}

void KprUploadChunkStreamTransform(void* it, KprMessage message UNUSED, xsMachine* machine UNUSED)
{
	KprUploadChunkStream self = it;
	KprUploadChunk chunk = self->chunk;
	while (chunk) {
		KprUploadChunk nextChunk = chunk->next;
		FskMemPtrDispose(chunk->data);
		FskMemPtrDispose(chunk);
		chunk = nextChunk;
	}
	self->chunk = NULL;
	self->chunkDone = true;
}

