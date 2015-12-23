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
#ifndef __KPRMESSAGE__
#define __KPRMESSAGE__

#include "kpr.h"
#include "kprURL.h"
#include "FskNetUtils.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum {
	kprServicesThread = 1 << 0,
};

typedef struct KprServiceStruct KprServiceRecord, *KprService;

typedef Boolean (*KprServiceAcceptProc)(KprService self, KprMessage message);
typedef void (*KprServiceCancelProc)(KprService self, KprMessage message);
typedef void (*KprServiceInvokeProc)(KprService self, KprMessage message);
typedef void (*KprServiceStartProc)(KprService self, FskThread thread, xsMachine* the);
typedef void (*KprServiceStopProc)(KprService self);
typedef void (*KprServiceDiscoverProc)(KprService self, char* authority, char* id, Boolean useEnvironment);
typedef void (*KprServiceForgetProc)(KprService self, char* authority, char* id);
typedef void (*KprServiceShareProc)(KprService self, char* authority, Boolean shareIt, Boolean useEnvironment);

struct KprServiceStruct {
	KprService next;
	UInt32 flags;
	char* id;
	xsMachine* machine;
	FskThread thread;
	KprServiceAcceptProc accept;
	KprServiceCancelProc cancel;
	KprServiceInvokeProc invoke;
	KprServiceStartProc start;
	KprServiceStopProc stop;
	KprServiceDiscoverProc discover;
	KprServiceForgetProc forget;
	KprServiceShareProc share;
};

FskAPI(void) KprServicesSetup();
FskAPI(void) KprServicesStart(KprShell shell);
FskAPI(void) KprServicesStop(KprShell shell);
FskAPI(void) KprServicesDiscover(KprContext context, char* id, char* services);
FskAPI(void) KprServicesForget(KprContext context, char* id);
FskAPI(void) KprServicesShare(KprContext context, Boolean shareIt, char* services);

FskAPI(Boolean) KprServiceAccept(KprService self, KprMessage message);
FskAPI(void) KprServiceCancel(KprService self, KprMessage message);
FskAPI(void) KprServiceInvoke(KprService self, KprMessage message);
FskAPI(void) KprServiceRegister(KprService service);
FskAPI(void) KprServiceUnregister(KprService service);

typedef void (*KprStreamDisposeProc)(void* it);
typedef Boolean (*KprStreamReconnectableProc)(void* it, KprMessage message);
typedef FskErr (*KprStreamReceiveProc)(void* it, KprMessage message, xsMachine* machine, char* data, int size);
typedef FskErr (*KprStreamSendProc)(void* it, KprMessage message, xsMachine* machine, char** data, int* size);
typedef void (*KprStreamTransformProc)(void* it, KprMessage message, xsMachine* machine);

struct KprStreamDispatchStruct {
	KprStreamDisposeProc dispose;
	KprStreamReconnectableProc reconnectable;
	KprStreamReceiveProc receive;
	KprStreamSendProc send;
	KprStreamTransformProc transform;
};

struct KprStreamStruct {
	KprStreamPart;
};

typedef void (*KprMessageCallbackProc)(KprMessage message, void* it);
typedef void (*KprMessageDisposeProc)(void* it);

typedef struct {
	void* headers;
	void* body;
	UInt32 size;
	FskSocketCertificateRecord* certs;
	void* target;
	KprMessageCallbackProc callback;
	KprMessageDisposeProc dispose;
} KprMessageRequestRecord, *KprMessageRequest;

typedef struct {
	void* headers;
	void* body;
	UInt32 size;
	void* target;
	KprMessageCallbackProc callback;
	KprMessageDisposeProc dispose;
} KprMessageResponseRecord, *KprMessageResponse;

typedef struct {
	void* target;
	KprMessageCallbackProc callback;
	KprMessageDisposeProc dispose;
} KprMessageStreamRecord, *KprMessageStream;

typedef struct {
	KprStreamPart;
	char* name;
	void* result;
} KprMessageScriptTargetRecord, *KprMessageScriptTarget;

struct KprMessageStruct {
	KprMessage next;
	char* url;
	KprURLPartsRecord parts;
	KprMessageRequestRecord request;
	KprMessageResponseRecord response;
	KprStream stream;
	char* method;
	SInt32 priority;
	char* user;
	char* password;
	FskErr error;
	UInt32 status;
	UInt32 usage;
	Boolean waiting;
	Boolean sniffing;
	UInt32 timeout;
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprMessageNew(KprMessage *it, char* url);
FskAPI(void) KprMessageCancel(KprMessage self);
FskAPI(void) KprMessageClearRequestHeader(KprMessage self, char* name);
FskAPI(void) KprMessageClearResponseHeader(KprMessage self, char* name);
FskAPI(void) KprMessageComplete(KprMessage self);
FskAPI(Boolean) KprMessageContinue(KprMessage self);
FskAPI(void) KprMessageDispose(KprMessage self);
FskAPI(FskErr) KprMessageGetError(KprMessage self);
FskAPI(char*) KprMessageGetMethod(KprMessage self);
FskAPI(void) KprMessageGetRequestBody(KprMessage self, void** data, UInt32* size);
FskAPI(char*) KprMessageGetRequestHeader(KprMessage self, char* name);
FskAPI(void) KprMessageGetResponseBody(KprMessage self, void** data, UInt32* size);
FskAPI(char*) KprMessageGetResponseHeader(KprMessage self, char* name);
FskAPI(UInt32) KprMessageGetTimeout(KprMessage self);
FskAPI(FskErr) KprMessageInvoke(KprMessage self, KprMessageCallbackProc callback, KprMessageDisposeProc dispose, void* target);
FskAPI(FskErr) KprMessageNotify(KprMessage self);
FskAPI(void) KprMessageRedirect(KprMessage self, char* url, char* mime);
FskAPI(void) KprMessageResume(KprMessage self);
FskAPI(FskErr) KprMessageSetCredentials(KprMessage self, char* username, char* password);
FskAPI(FskErr) KprMessageSetMethod(KprMessage self, char* method);
void KprMessageSetPriority(KprMessage self, SInt32 priority);
FskAPI(FskErr) KprMessageSetRequestBody(KprMessage self, void* data, UInt32 size);
FskAPI(FskErr) KprMessageSetRequestCertificate(KprMessage self, FskSocketCertificateRecord* certs);
FskAPI(FskErr) KprMessageSetRequestHeader(KprMessage self, char* name, char* value);
FskAPI(FskErr) KprMessageSetResponseBody(KprMessage self, void* data, UInt32 size);
FskAPI(FskErr) KprMessageSetResponseHeader(KprMessage self, char* name, char* value);
FskAPI(void) KprMessageSetStream(KprMessage self, KprStream stream);
FskAPI(void) KprMessageSetTimeout(KprMessage self, UInt32 timeout);
FskAPI(void) KprMessageSuspend(KprMessage self, KprMessageCallbackProc callback, KprMessageDisposeProc dispose, void* target);
FskAPI(void) KprMessageTransform(KprMessage self, xsMachine* machine);
FskAPI(FskErr) KprMessageURL(KprContext context, char* url, char** result);

FskAPI(void) KprMessageScriptTargetGet(KprMessage self, xsMachine* the, xsSlot* slot);
FskAPI(void) KprMessageScriptTargetSet(KprMessage self, xsMachine* the, xsSlot* slot);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
