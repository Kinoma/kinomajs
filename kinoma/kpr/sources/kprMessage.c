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
#define __FSKTHREAD_PRIV__

#include "kprApplication.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHandler.h"
#include "kprHTTPClient.h"
#include "kprMessage.h"
#include "kprSkin.h"
#include "kprShell.h"
#include "kprTransition.h"

static void KprXKPRServiceCancel(KprService self, KprMessage message);
static void KprXKPRServiceInvoke(KprService self, KprMessage message);

static void KprMessageNotifyCallback(KprShell self, KprMessage message);

static FskErr KprMessageScriptTargetNew(KprMessageScriptTarget* it, char* name);
static void KprMessageScriptTargetDispose(void* it);
static void KprMessageScriptTargetTransform(void* it, KprMessage message, xsMachine* machine);

static KprService gServices = NULL;

Boolean KprServiceAccept(KprService self, KprMessage message)
{
	return !FskStrCompareWithLength(message->url, self->id, FskStrLen(self->id));
}

void KprServiceCancel(KprService self UNUSED, KprMessage message UNUSED)
{
}

void KprServiceInvoke(KprService self UNUSED, KprMessage message)
{
	message->error = kFskErrUnimplemented;
	KprMessageComplete(message);
}

void KprServiceStart(KprService self UNUSED, FskThread thread UNUSED, xsMachine* the UNUSED)
{
}

void KprServiceStop(KprService self UNUSED)
{
}

void KprServiceRegister(KprService service)
{
	KprService *address = &gServices, current;
	if (!FskStrCompareWithLength(service->id, "xkpr", 4)) {    
		while ((current = *address)) {
			if (!FskStrCompareWithLength(current->id, "xkpr", 4)) {      
				if (FskStrCompare(current->id, service->id) <= 0)
					break;
			}
			address = &(current->next);
		}
	}
	else {
		while ((current = *address)) {
			address = &(current->next);
		}
	}
	service->next = current;
	*address = service;
	if (!service->accept)
		service->accept = KprServiceAccept;
	if (!service->cancel)
		service->cancel = KprServiceCancel;
	if (!service->invoke)
		service->invoke = KprServiceInvoke;
	if (!service->start)
		service->start = KprServiceStart;
	if (!service->stop)
		service->stop = KprServiceStop;
}


void KprServiceUnregister(KprService service)
{
	KprService *address = &gServices, current;
	while ((current = *address)) {
		if (current == service) {
			*address = service->next;
			break;
		}
		address = &(current->next);
	}
}

void KprXKPRServiceCancel(KprService self UNUSED, KprMessage message)
{
	message->usage--; // event queue
	if (gShell && FskListRemove(&gShell->messages, message)) {
		message->usage--; // message queue
		if (message->response.callback)
			(*message->response.callback)(message, message->response.target);
		if (message->response.dispose)
			(*message->response.dispose)(message->response.target);
		message->response.callback = NULL;
		message->response.dispose = NULL;
		message->response.target = NULL;
		message->usage--; // response
	}
	if (!message->usage)
		KprMessageDispose(message);
}

void KprXKPRServiceInvoke(KprService self UNUSED, KprMessage message)
{
	char* authority = message->parts.authority;
	UInt32 authorityLength = message->parts.authorityLength;
	message->usage--; // event queue
	if (!FskStrCompareWithLength(authority, "shell", authorityLength))
		KprContextInvoke(gShell, message);
	else {
		KprContentLink link = gShell->applicationChain.first;
		while (link) {
			KprApplication application = (KprApplication)link->content;
			if (application->id && (!FskStrCompareWithLength(authority, application->id, authorityLength))) {
				KprContextInvoke(application, message);
				break;
			}
			link = link->next;
		}
	}
	if (!message->response.callback) {
        KprMessageTransform(message, gShell->the);
		KprMessageComplete(message);
	}
}

KprServiceRecord gXKPRService = {
	NULL,
	0,
	"xkpr:",
	NULL,
	NULL,
	KprServiceAccept,
	KprXKPRServiceCancel,
	KprXKPRServiceInvoke,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

FskThread gServicesThread = NULL;
Boolean gServicesThreadQuitting = false;

static void KprServicesLoop(void* theParameter)
{
	xsAllocation allocation = {
		1100000,		// 700 * 1024,
		32 * 1024,
		70000,			// 62000,
		4096,
		2048,
		16000,
		1993
	};
	FskErr err = kFskErrNone;
	KprService service = gServices;
	FskThread thread = FskThreadGetCurrent();
	xsMachine* the = xsAliasMachine(&allocation, gShell->root, "services", gShell);
	bailIfNULL(the);
	while (service) {
		if (service->flags & kprServicesThread) {
			FskDebugStr("Starting %s", service->id);
			(*service->start)(service, thread, the);
		}
		service = service->next;
	}
	FskThreadInitializationComplete(thread);
	while (!gServicesThreadQuitting)
		FskThreadRunloopCycle(-1);
	service = gServices;
	while (service) {
		if (service->flags & kprServicesThread)
			(*service->stop)(service);
		service = service->next;
	}
bail:
	if (the)
		xsDeleteMachine(the);
	return;
}

void KprServicesDiscover(KprContext context, char* id, char* services)
{
	KprService service = gServices;
	while (service) {
		if (service->discover) {
			if (!service->thread)
				service->thread = KprShellGetThread(gShell);
			if (services) {
				if (FskStrStr(services, service->id))
					FskThreadPostCallback(service->thread, (FskThreadCallback)service->discover, service, FskStrDoCopy(context->id), FskStrDoCopy(id), false);
				else if (service->forget)
					FskThreadPostCallback(service->thread, (FskThreadCallback)service->forget, service, FskStrDoCopy(context->id), FskStrDoCopy(id), NULL);
			}
			else 
				FskThreadPostCallback(service->thread, (FskThreadCallback)service->discover, service, FskStrDoCopy(context->id), FskStrDoCopy(id), true);
		}
		service = service->next;
	}
}

void KprServicesForget(KprContext context, char* id)
{
	KprService service = gServices;
	while (service) {
		if (service->forget) {
			if (!service->thread)
				service->thread = KprShellGetThread(gShell);
			FskThreadPostCallback(service->thread, (FskThreadCallback)service->forget, service, FskStrDoCopy(context->id), FskStrDoCopy(id), NULL);
		}
		service = service->next;
	}
}

void KprServicesShare(KprContext context, Boolean shareIt, char* services)
{
	KprService service = gServices;
	while (service) {
		if (service->share) {
			if (!service->thread)
				service->thread = KprShellGetThread(gShell);
			FskThreadPostCallback(service->thread, (FskThreadCallback)service->share, service, FskStrDoCopy(context->id), services ? (FskStrStr(services, service->id) != NULL) : shareIt, services ? false : true);
		}
		service = service->next;
	}
}

void KprServicesStart(KprShell shell)
{
	UInt32 flags = kFskThreadFlagsWaitForInit | kFskThreadFlagsJoinable;
	KprService service = gServices;
	FskThread thread = KprShellGetThread(gShell);
	xsMachine* the = gShell->the;
	gServicesThreadQuitting = false;
	FskThreadCreate(&gServicesThread, KprServicesLoop, flags, NULL, "services");
	while (service) {
		if (!(service->flags & kprServicesThread)) {
			FskDebugStr("Starting %s", service->id);
			(*service->start)(service, thread, the);
		}
		service = service->next;
	}
}

void KprServicesStop(KprShell shell)
{
	KprService service = gServices;
	gServicesThreadQuitting = true;
	FskThreadJoin(gServicesThread);
	while (service) {
		if (!(service->flags & kprServicesThread))
			(*service->stop)(service);
		service = service->next;
	}
}

FskThread KprHTTPGetThread()
{
	return gServicesThread;
}

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprMessageInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprMessage", FskInstrumentationOffset(KprMessageRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprMessageNew(KprMessage *it, char* url)
{
	FskErr err = kFskErrNone;
	KprMessage self;
	bailIfError(FskMemPtrNewClear(sizeof(KprMessageRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprMessageInstrumentation);
	self->url = FskStrDoCopy(url);
	bailIfNULL(self->url);
	KprURLSplit(self->url, &self->parts);
	self->priority = 256;
bail:
	return err;
}

void KprMessageCancel(KprMessage self)
{
	KprService service = gServices;
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedMessageCancel, self);
	if (self->waiting) {
		self->waiting = false;
		if (self->request.target) {
			if (self->request.dispose)
				(*self->request.dispose)(self->request.target);
			self->request.callback = NULL;
			self->request.dispose = NULL;
			self->request.target = NULL;
			self->usage--; // request
		}
		while (service) {
			if ((*service->accept)(service, self)) {
				if (!service->thread)
					service->thread = KprShellGetThread(gShell);
				if (service == &gXKPRService)
					self->usage++; // event queue
				FskThreadPostCallback(service->thread, (FskThreadCallback)service->cancel, service, self, NULL, NULL);
				break;
			}
			service = service->next;
		}
	}
}

void KprMessageCancelReferrer(char* url)
{
	FskList messages = gShell->messages;
	KprMessage message = FskListGetNext(messages, NULL);
	while (message) {
		KprMessage next = FskListGetNext(messages, message);
		char* referrer = KprMessageGetRequestHeader(message, "referrer");
		if (referrer && !FskStrCompare(referrer, url))
			KprMessageCancel(message);
		message = next;
	}
}

void KprMessageClearRequestHeader(KprMessage self, char* name)
{
	if (self->request.headers)
		FskAssociativeArrayElementDispose(self->request.headers, (const char*)name);
}

void KprMessageClearResponseHeader(KprMessage self, char* name)
{
	if (self->response.headers)
		FskAssociativeArrayElementDispose(self->response.headers, (const char*)name);
}

void KprMessageComplete(KprMessage self)
{
	char* url;
	FskThread thread = KprShellGetThread(gShell);
	if (!thread) return; // @@ happens on exit
	if (thread != FskThreadGetCurrent()) {
		FskThreadPostCallback(thread, (FskThreadCallback)KprMessageComplete, self, NULL, NULL, NULL);
        return;
    }
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedMessageComplete, self);
	if (FskListRemove(&gShell->messages, self)) {
		self->usage--; // message queue
		self->waiting = false;
		url = KprMessageGetResponseHeader(self, "location");
		if (url && (!FskStrCompareWithLength(self->url, "xkpr", 4)) && ((!self->sniffing) || (!FskStrCompareWithLength(url, "xkpr", 4)))) {
			FskMemPtrDispose(self->url);
			self->url = FskStrDoCopy(url);
			//bailIfNULL(self->url);
			KprURLSplit(self->url, &self->parts);
			FskAssociativeArrayDispose(self->response.headers);
			self->response.headers = NULL;
			FskMemPtrDispose(self->response.body);
			self->response.body = NULL;
			self->response.size = 0;
            if (self->request.target)
                self->usage--; // @@ request
			if (kFskErrNone == KprMessageInvoke(self, self->request.callback, self->request.dispose, self->request.target))
				return;
		}
		if (self->request.target) {
			if (self->request.callback)
				(*self->request.callback)(self, self->request.target);
			if (self->request.dispose)
				(*self->request.dispose)(self->request.target);
			self->request.callback = NULL;
			self->request.dispose = NULL;
			self->request.target = NULL;
			self->usage--; // request
		}
        if (!self->usage)
            KprMessageDispose(self);
	}
}

Boolean KprMessageContinue(KprMessage self)
{
	//return __sync_fetch_and_and(&self->waiting, true); 
	return self->waiting;
}

void KprMessageDispose(KprMessage self)
{
	if (self) {
		FskListRemove(&gShell->messages, self);
		if (self->stream && self->stream->dispatch->dispose)
			(*self->stream->dispatch->dispose)(self->stream);
		if (self->response.dispose)
			(*self->response.dispose)(self->response.target);
		FskMemPtrDispose(self->response.body);
		FskAssociativeArrayDispose(self->response.headers);
		if (self->request.dispose)
			(*self->request.dispose)(self->request.target);
		FskMemPtrDispose(self->request.body);
		FskMemPtrDispose(self->request.certificate);
		FskMemPtrDispose(self->request.policies);
		FskAssociativeArrayDispose(self->request.headers);
		FskMemPtrDispose(self->url);
		FskMemPtrDispose(self->method);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprMessageGetError(KprMessage self)
{
	return self->error;
}

char* KprMessageGetMethod(KprMessage self)
{
	return self->method;
}

SInt32 KprMessageGetPriority(KprMessage self)
{
	return self->priority;
}

void KprMessageGetRequestBody(KprMessage self, void** data, UInt32* size)
{
	*data = self->request.body;
	*size = self->request.size;
}

char* KprMessageGetRequestHeader(KprMessage self, char* name)
{
	if (self->request.headers)
		return FskAssociativeArrayElementGetString(self->request.headers, (const char*)name);
	return NULL;
}

void KprMessageGetResponseBody(KprMessage self, void** data, UInt32* size)
{
	*data = self->response.body;
	*size = self->response.size;
}

char* KprMessageGetResponseHeader(KprMessage self, char* name)
{
	if (self->response.headers)
		return FskAssociativeArrayElementGetString(self->response.headers, (const char*)name);
	return NULL;
}

UInt32 KprMessageGetTimeout(KprMessage self)
{
	return self->timeout;
}

FskErr KprMessageInvoke(KprMessage self, KprMessageCallbackProc callback, KprMessageDisposeProc dispose, void* target)
{
	KprService service = gServices;
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedMessageInvoke, self);
	if (self->waiting)
		return kFskErrOutOfSequence;
	while (service) {
		if ((*service->accept)(service, self)) {
			self->waiting = true;
			if (target) {
				self->request.callback = callback;
				self->request.dispose = dispose;
				self->request.target = target;
				self->usage++; // request
			}
			FskListAppend(&gShell->messages, self);
			self->usage++; // message queue
			if (!service->thread)
				service->thread = KprShellGetThread(gShell);
			if (service == &gXKPRService)
				self->usage++; // event queue
			FskThreadPostCallback(service->thread, (FskThreadCallback)service->invoke, service, self, NULL, NULL);
			return kFskErrNone;
		}
		service = service->next;
	}
	return kFskErrUnimplemented;
}

FskErr KprMessageNotify(KprMessage self)
{
	FskErr err = kFskErrNone;
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedMessageNotify, self);
	if (!FskStrCompareWithLength(self->parts.scheme, "xkpr", self->parts.schemeLength)) {
		self->usage++; // event queue
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageNotifyCallback, gShell, self, NULL, NULL);
	}
	else
		err = kFskErrUnimplemented;
	return err;
}

void KprMessageNotifyCallback(KprShell self, KprMessage message)
{
	KprContentLink link;
	message->usage--; // event queue
	KprContextInvoke(self, message);
	link = self->applicationChain.first;
	while (link) {
		KprContextInvoke(link->content, message);
		link = link->next;
	}
	if (!message->usage)
		KprMessageDispose(message);
}

void KprMessageRedirect(KprMessage self, char* url, char* mime)
{
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedMessageRedirect, self);
	KprMessageSetResponseHeader(self, "location", url);
	if (mime)
		KprMessageSetResponseHeader(self, "content-type", mime);
}

void KprMessageResume(KprMessage self)
{
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedMessageResume, self);
	if (self->response.dispose)
		(*self->response.dispose)(self->response.target);
	self->response.callback = NULL;
	self->response.dispose = NULL;
	self->response.target = NULL;
	self->usage--; // response
	KprMessageTransform(self, gShell->the);
	KprMessageComplete(self);
}

FskErr KprMessageSetCredentials(KprMessage self, char* user, char* password)
{
	FskErr err = kFskErrNone;
	FskMemPtrDisposeAt(&self->user);
	FskMemPtrDisposeAt(&self->password);
	if (user && password) {
		self->user = FskStrDoCopy(user);
		bailIfNULL(self->user);
		self->password = FskStrDoCopy(password);
		bailIfNULL(self->password);
	}
bail:
	return err;
}

FskErr KprMessageSetMethod(KprMessage self, char* method)
{
	FskErr err = kFskErrNone;
	FskMemPtrDisposeAt(&self->method);
	if (method) {
		self->method = FskStrDoCopy(method);
		bailIfNULL(self->method);
	}
bail:
	return err;
}

void KprMessageSetPriority(KprMessage self, SInt32 priority)
{
	self->priority = priority;
}

FskErr KprMessageSetRequestBody(KprMessage self, void* data, UInt32 size)
{
	FskErr err = kFskErrNone;
	FskMemPtrDisposeAt(&self->request.body);
	self->request.size = 0;
	if (data && size) {
		bailIfError(FskMemPtrNew(size, &self->request.body));
		FskMemCopy(self->request.body, data, size);
		self->request.size = size;
	}
bail:
	return err;
}

FskErr KprMessageSetRequestCertificate(KprMessage self, void* certificate, UInt32 size, char *policies)
{
	FskErr err = kFskErrNone;
	FskMemPtrDisposeAt(&self->request.certificate);
	FskMemPtrDisposeAt(&self->request.policies);
	self->request.certificateSize = 0;
	if (certificate && size) {
		bailIfError(FskMemPtrNew(size, &self->request.certificate));
		FskMemCopy(self->request.certificate, certificate, size);
		self->request.certificateSize = size;
	}
	self->request.policies = FskStrDoCopy(policies);
bail:
	return err;
}

FskErr KprMessageSetRequestHeader(KprMessage self, char* name, char* value)
{
	FskErr err = kFskErrNone;
	if (!self->request.headers)
		self->request.headers = FskAssociativeArrayNew();
	if (self->request.headers)
		FskAssociativeArrayElementSet(self->request.headers, (const char*)name, (const char*)value, 0, kFskStringType);
	return err;
}

FskErr KprMessageSetResponseBody(KprMessage self, void* data, UInt32 size)
{
	FskErr err = kFskErrNone;
	FskMemPtrDisposeAt(&self->response.body);
	self->response.size = 0;
	if (data && size) {
		bailIfError(FskMemPtrNew(size, &self->response.body));
		FskMemCopy(self->response.body, data, size);
		self->response.size = size;
	}
bail:
	return err;
}

FskErr KprMessageSetResponseHeader(KprMessage self, char* name, char* value)
{
	FskErr err = kFskErrNone;
	if (!self->response.headers)
		self->response.headers = FskAssociativeArrayNew();
	if (self->response.headers)
		FskAssociativeArrayElementSet(self->response.headers, (const char*)name, (const char*)value, 0, kFskStringType);
	return err;
}

void KprMessageSetStream(KprMessage self, KprStream stream)
{
	self->stream = stream;
	FskInstrumentedItemSetOwner(stream, self);
}

void KprMessageSetTimeout(KprMessage self, UInt32 timeout)
{
	self->timeout = timeout;
}

void KprMessageSuspend(KprMessage self, KprMessageCallbackProc callback, KprMessageDisposeProc dispose, void* target)
{
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedMessageSuspend, self);
	self->response.callback = callback;
	self->response.dispose = dispose;
	self->response.target = target;
	self->usage++; // response
}

void KprMessageTransform(KprMessage self, xsMachine* machine)
{
	KprStream stream = self->stream;
	if (machine && stream && stream->dispatch->transform) {
		if ((!FskStrCompareWithLength(self->url, "xkpr", 4) && KprMessageGetResponseHeader(self, "location")))
			return;
		(*stream->dispatch->transform)(stream, self, machine);
	}
}

FskErr KprMessageURL(KprContext context, char* url, char** result)
{
	FskErr err = kFskErrNone;
	KprURLPartsRecord parts;
	KprURLSplit(url, &parts);
	if (!parts.scheme) {
		parts.scheme = "xkpr";
		parts.schemeLength = 4;
		if (!parts.authority) {
			parts.authority = context->id;
			parts.authorityLength = FskStrLen(parts.authority);
		}
		bailIfError(KprURLJoin(&parts, result));
	}
	else {
		*result = FskStrDoCopy(url);
		bailIfNULL(*result);
	}
bail:
	return err;
}

/* ECMASCRIPT */

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprMessageScriptTargetInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprMessageScriptTarget", FskInstrumentationOffset(KprMessageScriptTargetRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
KprStreamDispatchRecord kprScriptTargetDispatchRecord = {
	KprMessageScriptTargetDispose,
	NULL,
	NULL,
	NULL,
	KprMessageScriptTargetTransform
};

FskErr KprMessageScriptTargetNew(KprMessageScriptTarget* it, char* name)
{
	KprMessageScriptTarget self;
	FskErr err = kFskErrNone;
	bailIfError(FskMemPtrNewClear(sizeof(KprMessageScriptTargetRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprMessageScriptTargetInstrumentation);
	self->dispatch = &kprScriptTargetDispatchRecord;
	self->name = FskStrDoCopy(name);
bail:
	return err;
}

void KprMessageScriptTargetDispose(void* it)
{
	KprMessageScriptTarget self = it;
	FskMemPtrDispose(self->result);
	FskMemPtrDispose(self->name);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

void KprMessageScriptTargetTransform(void* it, KprMessage message, xsMachine* machine)
{
	KprMessageScriptTarget self = it;
	if (message->error) return;
	xsBeginHostSandboxCode(machine, NULL);
	{
		xsVars(2);
		{
			xsTry {
				void* data;
				UInt32 size; 
				KprMessageGetResponseBody(message, &data, &size);
				if (data && size) {
					xsIndex id = xsID(self->name);
					if (xsHas(xsGlobal, id)) {
						xsVar(0) = xsGet(xsGlobal, id);
						xsVar(1) = xsNewInstanceOf(xsChunkPrototype);
						xsSetHostData(xsVar(1), data);
						xsSetHostDestructor(xsVar(1) , NULL);
						xsSet(xsVar(1), xsID("length"), xsInteger(size));
						xsResult = xsCall1(xsVar(0), xsID("parse"), xsVar(1));	
						self->result = xsMarshall(xsResult);
					}
				}
			}
			xsCatch {
			}
		}
	}
	xsEndHostSandboxCode();
}

void KprMessageScriptTargetGet(KprMessage self, xsMachine* the, xsSlot* slot)
{
	if (self->stream->dispatch->dispose == KprMessageScriptTargetDispose) {
		KprMessageScriptTarget target = (KprMessageScriptTarget)self->stream;
		if (target->result)
			*slot = xsDemarshall(target->result);
		else {
			void* data;
			UInt32 size; 
			KprMessageGetResponseBody(self, &data, &size);
			if (data && size) {
				if (!FskStrCompare(target->name, "TEXT"))
					*slot = xsStringBuffer(data, size);
				else {
					*slot = xsNewInstanceOf(xsChunkPrototype);
					xsSetHostData(*slot, data);
					xsSetHostDestructor(*slot , NULL);
					xsSet(*slot, xsID("length"), xsInteger(size));
				}
			}
		}
	}
}

void KprMessageScriptTargetSet(KprMessage self, xsMachine* the, xsSlot* slot)
{
	KprMessageScriptTarget stream = NULL;
	if (xsTypeOf(*slot) == xsStringType) {
		xsThrowIfFskErr(KprMessageScriptTargetNew(&stream, xsToString(*slot)));
	}
	else {
		stream = xsGetHostData(*slot);
    }
	KprMessageSetStream(self, (KprStream)stream);
}

void KPR_message(void *it)
{
	if (it) {
		KprMessage self = it;
		FskInstrumentedItemSendMessageDebug(self, kprInstrumentedMessageDestruct, self);
		self->usage--; // host
		if (!self->usage)
			KprMessageDispose(self);
	}
}

void KPR_Message(xsMachine* the)
{
	xsStringValue url = NULL;
	KprMessage self = NULL;
	xsTry {
		xsThrowIfFskErr(KprMessageURL(xsGetContext(the), xsToString(xsArg(0)), &url));
		xsThrowIfFskErr(KprMessageNew(&self, url));
		xsSetHostData(xsThis, self);
		FskInstrumentedItemSendMessageDebug(self, kprInstrumentedMessageConstruct, self);
		self->usage++; // host
		FskMemPtrDispose(url);
	}
	xsCatch {
		FskMemPtrDispose(url);
		xsThrow(xsException);
	}
}

#define KPR_message_get_part(PART,PART_LENGTH) \
	if (PART) { \
		char *p = PART + PART_LENGTH; \
		char c = *p; \
		*p = 0; \
		xsResult = xsString(PART); \
		*p = c; \
	}

void KPR_message_get_authority(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	KPR_message_get_part(self->parts.authority, self->parts.authorityLength);
}

void KPR_message_get_error(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->error);
}

void KPR_message_get_fragment(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	KPR_message_get_part(self->parts.fragment, self->parts.fragmentLength);
}

void KPR_message_get_method(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	xsStringValue method = KprMessageGetMethod(self);
	if (method)
		xsResult = xsString(method);
}

void KPR_message_get_name(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	KPR_message_get_part(self->parts.name, self->parts.nameLength);
}

void KPR_message_get_password(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	KPR_message_get_part(self->parts.password, self->parts.passwordLength);
}

void KPR_message_get_path(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	KPR_message_get_part(self->parts.path, self->parts.pathLength);
}

void KPR_message_get_priority(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	xsResult = xsNumber((double)KprMessageGetPriority(self) / 4096);
}

void KPR_message_get_query(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	KPR_message_get_part(self->parts.query, self->parts.queryLength);
}

void KPR_message_get_requestChunk(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	void* data;
	UInt32 size; 
	KprMessageGetRequestBody(self, &data, &size);
	if (data && size) {
		xsResult = xsNew1(xsGlobal, xsID_Chunk, xsInteger(size));
		FskMemCopy(xsGetHostData(xsResult), data, size);
	}
}

void KPR_message_get_requestText(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	void* data;
	UInt32 size; 
	KprMessageGetRequestBody(self, &data, &size);
	if (data && size)
		xsResult = xsStringBuffer(data, size);
}

void KPR_message_get_responseChunk(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	void* data;
	UInt32 size; 
	KprMessageGetResponseBody(self, &data, &size);
	if (data && size) {
		xsResult = xsNew1(xsGlobal, xsID_Chunk, xsInteger(size));
		FskMemCopy(xsGetHostData(xsResult), data, size);
	}
}

void KPR_message_get_responseText(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	void* data;
	UInt32 size; 
	KprMessageGetResponseBody(self, &data, &size);
	if (data && size)
		xsResult = xsStringBuffer(data, size);
}

void KPR_message_get_scheme(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	KPR_message_get_part(self->parts.scheme, self->parts.schemeLength);
}

void KPR_message_get_status(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->status);
}

void KPR_message_get_url(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	xsResult = xsString(self->url);
}

void KPR_message_get_user(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	KPR_message_get_part(self->parts.user, self->parts.userLength);
}

void KPR_message_set_error(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	self->error = xsToInteger(xsArg(0));
}

void KPR_message_set_method(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		xsThrowIfFskErr(KprMessageSetMethod(self, xsToString(xsArg(0))));
	else
		xsThrowIfFskErr(KprMessageSetMethod(self, NULL));
}

void KPR_message_set_priority(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	KprMessageSetPriority(self, (SInt32)(4096 * xsToNumber(xsArg(0))));
}

void KPR_message_set_requestChunk(xsMachine* the)
{
	KprMessage self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0))) {
		void* data = xsGetHostData(xsArg(0));
		xsIntegerValue size = xsToInteger(xsGet(xsArg(0), xsID_length));
		xsThrowIfFskErr(KprMessageSetRequestBody(self, data, size));
	}
	else
		xsThrowIfFskErr(KprMessageSetRequestBody(self, NULL, 0));
}

void KPR_message_set_requestText(xsMachine* the)
{
	KprMessage self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0))) {
		xsStringValue body = xsToString(xsArg(0));
		xsThrowIfFskErr(KprMessageSetRequestBody(self, body, FskStrLen(body)));
	}
	else
		xsThrowIfFskErr(KprMessageSetRequestBody(self, NULL, 0));
}

void KPR_message_set_responseChunk(xsMachine* the)
{
	KprMessage self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0))) {
		void* data = xsGetHostData(xsArg(0));
		xsIntegerValue size = xsToInteger(xsGet(xsArg(0), xsID_length));
		xsThrowIfFskErr(KprMessageSetResponseBody(self, data, size));
	}
	else
		xsThrowIfFskErr(KprMessageSetResponseBody(self, NULL, 0));
}

void KPR_message_set_responseText(xsMachine* the)
{
	KprMessage self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0))) {
		xsStringValue body = xsToString(xsArg(0));
		xsThrowIfFskErr(KprMessageSetResponseBody(self, body, FskStrLen(body)));
	}
	else
		xsThrowIfFskErr(KprMessageSetResponseBody(self, NULL, 0));
}

void KPR_message_set_status(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	self->status = xsToNumber(xsArg(0));
}

void KPR_message_clearRequestHeader(xsMachine* the)
{
	KprMessage self = kprGetHostData(xsThis, this, message);
	KprMessageClearRequestHeader(self, xsToString(xsArg(0)));
}

void KPR_message_clearResponseHeader(xsMachine* the)
{
	KprMessage self = kprGetHostData(xsThis, this, message);
	KprMessageClearResponseHeader(self, xsToString(xsArg(0)));
}

void KPR_message_getRequestHeader(xsMachine* the)
{
	KprMessage self = kprGetHostData(xsThis, this, message);
	xsStringValue result = KprMessageGetRequestHeader(self, xsToString(xsArg(0)));
	if (result)
		xsResult = xsString(result);
}

void KPR_message_getResponseHeader(xsMachine* the)
{
	KprMessage self = kprGetHostData(xsThis, this, message);
	xsStringValue result = KprMessageGetResponseHeader(self, xsToString(xsArg(0)));
	if (result)
		xsResult = xsString(result);
}

void KPR_message_cancel(xsMachine* the)
{
	KprMessage self = kprGetHostData(xsThis, this, message);
	KprMessageCancel(self);
}

void KPR_message_setRequestCertificate(xsMachine* the)
{
	KprMessage self = xsGetHostData(xsThis);
	void *data = NULL;
	xsStringValue policies = NULL;
	xsIntegerValue size = 0;
	if (xsTest(xsArg(0))) {
		data = xsGetHostData(xsArg(0));
		size = xsToInteger(xsGet(xsArg(0), xsID_length));
	}
	if (xsTest(xsArg(1)))
		policies = xsToString(xsArg(1));
	xsThrowIfFskErr(KprMessageSetRequestCertificate(self, data, size, policies));
}

void KPR_message_setRequestHeader(xsMachine* the)
{
	KprMessage self = kprGetHostData(xsThis, this, message);
	xsStringValue name = xsToString(xsArg(0));
	xsStringValue value = xsToString(xsArg(1));
	KprMessageSetRequestHeader(self, name, value);
}

void KPR_message_setResponseHeader(xsMachine* the)
{
	KprMessage self = kprGetHostData(xsThis, this, message);
	xsStringValue name = xsToString(xsArg(0));
	xsStringValue value = xsToString(xsArg(1));
	KprMessageSetResponseHeader(self, name, value);
}

void KPR_message_get_timeout(xsMachine *the)
{
	KprMessage self = kprGetHostData(xsThis, this, message);
	xsResult = xsInteger(self->timeout);
}

void KPR_message_set_timeout(xsMachine *the)
{
	KprMessage self = kprGetHostData(xsThis, this, message);
	UInt32 timeout = xsToInteger(xsArg(0));
	KprMessageSetTimeout(self, timeout);
}

void KPR_Message_URI(xsMachine* the)
{
	char* url = NULL;
	xsThrowIfFskErr(KprMessageURL(xsGetContext(the), xsToString(xsArg(0)), &url));
	xsResult = xsString(url);
	FskMemPtrDispose(url);
}

void KPR_Message_cancelReferrer(xsMachine* the)
{
	char* url = FskStrDoCopy(xsToString(xsArg(0)));
	xsThrowIfNULL(url);
	KprMessageCancelReferrer(url);
	FskMemPtrDispose(url);
}

void KPR_Message_notify(xsMachine* the)
{
	KprMessage self = kprGetHostData(xsArg(0), this, message);
	xsThrowIfFskErr(KprMessageNotify(self));
}

void KPR_Message_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("Message"));
	xsNewHostProperty(xsResult, xsID("CHUNK"), xsString("CHUNK"), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("DOM"), xsString("DOM"), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("JSON"), xsString("JSON"), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("TEXT"), xsString("TEXT"), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("URI"), xsNewHostFunction(KPR_Message_URI, 1), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("cancelReferrer"), xsNewHostFunction(KPR_Message_cancelReferrer, 1), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("notify"), xsNewHostFunction(KPR_Message_notify, 1), xsDefault, xsDontScript);
}
