#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHandler.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprURL.h"
#include "kprProxyService.h"

typedef struct KprProxyServiceInstanceStruct KprProxyServiceInstanceRecord, *KprProxyServiceInstance;
typedef struct KprProxyServiceTargetStruct KprProxyServiceTargetRecord, *KprProxyServiceTarget;

struct KprProxyServiceInstanceStruct {
	KprProxyServiceVM vm;
	KprContext context;
	char* from;
	char* path;
	xsSlot slot;
};

struct KprProxyServiceTargetStruct {
	xsMachine* the;
	xsSlot resolve;
	xsSlot reject;
};

static void KprProxyServiceCancel(KprService self, KprMessage message);
static void KprProxyServiceInvoke(KprService self, KprMessage message);
static void KprProxyServiceStart(KprService service, FskThread thread, xsMachine* machine);
static void KprProxyServiceStop(KprService self);

static FskErr KprProxyServiceBehaviorNew(KprBehavior* it, KprContent content, xsMachine* the, xsSlot* slot);
static void KprProxyServiceBehaviorDispose(void* it);
static void KprProxyServiceBehaviorMark(void* it, xsMarkRoot markRoot);
static void KprProxyServiceBehaviorInvoke(void* it, KprMessage message);

static void KprProxyServiceMachineCancel(KprMessage message, void* target);
static void KprProxyServiceMachineInvoke(xsMachine* machine, xsSlot* slot, KprMessage message);
static void KprProxyServiceMachineReject(xsMachine* the);
static void kprProxyServiceMachineRejectMessage(xsMachine* machine, KprMessage message);
static void KprProxyServiceMachineResolve(xsMachine* the);
static void KprProxyServiceMachineResolveMessage(xsMachine* machine, KprMessage message);

static void KprProxyServiceTargetComplete(KprMessage message, void* it);
static void KprProxyServiceTargetDispose(void* it);
static void KprProxyServiceTargeInvoke(xsMachine* the);

static KprServiceRecord gProxyService = {
	NULL,
	kprServicesThread,
	"xkpr://services/proxy",
	NULL,
	NULL,
	KprServiceAccept,
	KprProxyServiceCancel,
	KprProxyServiceInvoke,
	KprProxyServiceStart,
	KprProxyServiceStop,
	NULL,
	NULL,
	NULL
};

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprProxyServiceBehaviorInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprBehavior", FskInstrumentationOffset(KprBehaviorRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
KprDelegateRecord kprProxyServiceBehaviorDelegateRecord = {
	KprDefaultBehaviorAccept,
	KprDefaultBehaviorActivated,
	KprDefaultBehaviorAdapt,
	KprDefaultBehaviorCancel,
	KprDefaultBehaviorComplete,
	KprDefaultBehaviorDisplayed,
	KprDefaultBehaviorDisplaying,
	KprProxyServiceBehaviorDispose,
	KprDefaultBehaviorFinished,
	KprDefaultBehaviorFocused,
	KprProxyServiceBehaviorInvoke,
	KprDefaultBehaviorKeyDown,
	KprDefaultBehaviorKeyUp,
	KprDefaultBehaviorLaunch,
	KprDefaultBehaviorLoaded,
	KprDefaultBehaviorLoading,
	KprProxyServiceBehaviorMark,
	KprDefaultBehaviorMeasureHorizontally,
	KprDefaultBehaviorMeasureVertically,
	KprDefaultBehaviorMetadataChanged,
	KprDefaultBehaviorQuit,
#if SUPPORT_REMOTE_NOTIFICATION
	KprDefaultBehaviorRemoteNotificationRegistered,
	KprDefaultBehaviorRemoteNotified,
#endif	/* SUPPORT_REMOTE_NOTIFICATION */
	KprDefaultBehaviorScrolled,
	KprDefaultBehaviorSensorBegan,
	KprDefaultBehaviorSensorChanged,
	KprDefaultBehaviorSensorEnded,
	KprDefaultBehaviorStateChanged,
	KprDefaultBehaviorTimeChanged,
	KprDefaultBehaviorTouchBegan,
	KprDefaultBehaviorTouchCancelled,
	KprDefaultBehaviorTouchEnded,
	KprDefaultBehaviorTouchMoved,
	KprDefaultBehaviorTransitionBeginning,
	KprDefaultBehaviorTransitionEnded,
	KprDefaultBehaviorUndisplayed,
	KprDefaultBehaviorUnfocused
};


FskExport(FskErr) kprProxyService_fskLoad(FskLibrary it)
{
	KprServiceRegister(&gProxyService);
	return kFskErrNone;
}

FskExport(FskErr) kprProxyService_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

void KprProxyServiceCancel(KprService self UNUSED, KprMessage message UNUSED)
{
	// ??
}

void KprProxyServiceInvoke(KprService service UNUSED, KprMessage message)
{
	if (KprMessageContinue(message)) {
		SInt32 index = FskStrToL(message->parts.name, NULL, 16);
		KprProxyServiceVM vm = KprProxyServiceVMFind(index);
		if (vm) {
			KprProxyServiceInstance instance = KprProxyServiceVMGetRefcon(vm);
			KprProxyServiceMachineInvoke(KprProxyServiceVMGetMachine(vm), &instance->slot, message);
			if (!message->response.callback)
				KprMessageComplete(message);
		}
		else {
			message->error = kFskErrNotFound;
			message->status = 404;
			KprMessageComplete(message);
		}
	}
}

void KprProxyServiceStart(KprService service, FskThread thread, xsMachine* machine UNUSED)
{
	service->thread = thread;
}

void KprProxyServiceStop(KprService service)
{
	service->thread = NULL;
}

FskErr KprProxyServiceBehaviorNew(KprBehavior* it, KprContent content, xsMachine* the, xsSlot* slot)
{
	FskErr err = kFskErrNone;
	KprBehavior self;
	bailIfError(FskMemPtrNewClear(sizeof(KprBehaviorRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprProxyServiceBehaviorInstrumentation);
	FskInstrumentedItemSetOwner(self, content);
	self->delegate = &kprProxyServiceBehaviorDelegateRecord;
	self->the = the;
	self->slot = *slot;
	FskInstrumentedItemSendMessageNormal(content, kprInstrumentedContentPutBehavior, self);
bail:
	return err;
}

void KprProxyServiceBehaviorDispose(void* it)
{
	KprContent content = it;
	KprBehavior self = content->behavior;
	FskInstrumentedItemSendMessageNormal(content, kprInstrumentedContentRemoveBehavior, self);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

void KprProxyServiceBehaviorMark(void* it, xsMarkRoot markRoot)
{
	KprContent content = it;
	KprBehavior self = content->behavior;
	(*markRoot)(self->the, &self->slot);
}

void KprProxyServiceBehaviorInvoke(void* it, KprMessage message)
{
	KprContent content = it;
	KprBehavior self = content->behavior;
	KprProxyServiceMachineInvoke(self->the, &self->slot, message);
}

void KprProxyServiceMachineCancel(KprMessage message, void* target)
{
	// ??
}

void KprProxyServiceMachineInvoke(xsMachine* machine, xsSlot* slot, KprMessage message)
{
	xsBeginHost(machine);
	{
		FskAssociativeArray query = NULL;
		xsVars(7);
		{
			xsTry {
				xsStringValue key;
				xsThrowIfFskErr(KprQueryParse(message->parts.query, &query));
				key = FskAssociativeArrayElementGetString(query, "key");
				xsVar(0) = xsAccess(*slot);
				xsVar(1) = xsGetAt(xsVar(0), xsString(key));
				if (xsTest(xsVar(1))) {
					xsVar(2) = xsDemarshall(message->request.body);
					xsResult = xsCall2(xsVar(1), xsID_apply, xsVar(0), xsVar(2));
					if (xsIsInstanceOf(xsResult, xsPromisePrototype)) {
						KprMessageSuspend(message, KprProxyServiceMachineCancel, NULL, NULL);
						xsVar(3) = xsCall1(xsGet(xsGlobal, xsID_Promise), xsID_resolve, xsResult);
						xsVar(4) = xsNewHostFunction(KprProxyServiceMachineResolve, 1);
						xsVar(5) = xsNewHostFunction(KprProxyServiceMachineReject, 1);
					
						xsVar(6) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_message));
						xsSetHostData(xsVar(6), message);
						FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageConstruct, message);
						message->usage++; // host
						xsSet(xsVar(4), xsID_message, xsVar(6));
						xsSet(xsVar(5), xsID_message, xsVar(6));
					
						xsCall2(xsVar(3), xsID_then, xsVar(4), xsVar(5));
					}
					else {
						KprProxyServiceMachineResolveMessage(the, message);
					}
				}
				else {
					xsReferenceError("method %s not found", key);
				}
			}
			xsCatch {
				xsResult = xsException;
				kprProxyServiceMachineRejectMessage(the, message);
			}
		}
		FskAssociativeArrayDispose(query);
	}
	xsEndHost(machine);
}

void KprProxyServiceMachineReject(xsMachine* the)
{
	KprMessage message;
	xsResult = xsGet(xsFunction, xsID_message);
	message = kprGetHostData(xsResult, this, message);
	xsResult = xsArg(0);
	kprProxyServiceMachineRejectMessage(the, message);
	KprMessageResume(message);
}

void kprProxyServiceMachineRejectMessage(xsMachine* the, KprMessage message)
{
	message->response.body = xsMarshall(xsResult);
	message->response.size = 0xFFFFFFFF;
	message->error = kFskErrUnknown;
	message->status = 500;
}

void KprProxyServiceMachineResolve(xsMachine* the)
{
	KprMessage message;
	xsResult = xsGet(xsFunction, xsID_message);
	message = kprGetHostData(xsResult, this, message);
	xsResult = xsArg(0);
	KprProxyServiceMachineResolveMessage(the, message);
	KprMessageResume(message);
}

void KprProxyServiceMachineResolveMessage(xsMachine* the, KprMessage message)
{
	message->response.body = xsMarshall(xsResult);
	message->response.size = 0xFFFFFFFF;
	message->error = kFskErrNone;
	message->status = 200;
}

void KprProxyServiceTargetComplete(KprMessage message, void* it)
{
	KprProxyServiceTarget self = it;
	xsBeginHost(self->the);
	xsVars(2);
	if (message->error) {
		xsVar(0) = xsAccess(self->reject);
	}
	else {
		xsVar(0) = xsAccess(self->resolve);
	}
	if (message->response.size == 0xFFFFFFFF)
		xsVar(1) = xsDemarshall(message->response.body);
	(void)xsCallFunction1(xsVar(0), xsUndefined, xsVar(1));
	xsEndHost(self->the);
}

void KprProxyServiceTargetDispose(void* it)
{
	KprProxyServiceTarget self = it;
	fxForget(self->the, &self->reject);
	fxForget(self->the, &self->resolve);
	FskMemPtrDispose(self);
}

void KprProxyServiceTargeInvoke(xsMachine* the) 
{
	KprMessage message;
	KprProxyServiceTarget self;
	xsThrowIfFskErr(KprMessageNew(&message, xsToString(xsArg(0))));
	message->request.body = xsMarshall(xsArg(1));
	message->request.size = 0xFFFFFFFF;
	xsThrowIfFskErr(FskMemPtrNewClear(sizeof(KprProxyServiceTargetRecord), &self));
	self->the = the;
	self->resolve = xsArg(2);
	self->reject = xsArg(3);
	fxRemember(self->the, &self->resolve);
	fxRemember(self->the, &self->reject);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageInvoke, message, KprProxyServiceTargetComplete, KprProxyServiceTargetDispose, self);
}

// --------------------------------------------------------------
// KprProxyServiceVMClient
// --------------------------------------------------------------

struct KprProxyServiceVMStruct {
	KprProxyServiceVM next;
	void *context;
	SInt32 index;
	xsMachine* machine;
	void *refcon;
};

static SInt32 gProxyServiceVMIndex = 0;
static FskList gProxyServiceVMClients = NULL;

static void KprProxyServiceVMStop(KprProxyServiceVM self, xsMachine* machine);

FskErr KprProxyServiceVMNew(void *context, void *refcon, KprProxyServiceVM *it)
{
	FskErr err = kFskErrNone;
	KprProxyServiceVM self = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(KprProxyServiceVMRecord), &self));
	self->context = context;
	self->refcon = refcon;
	self->index = gProxyServiceVMIndex++;

	*it = self;

bail:
	return err;
}

KprProxyServiceVM KprProxyServiceVMFind(SInt32 index)
{
	KprProxyServiceVM vm = FskListGetNext(gProxyServiceVMClients, NULL);
	while (vm) {
		if (vm->index == index)
			break;
		vm = FskListGetNext(gProxyServiceVMClients, vm);
	}
	return vm;
}

FskErr KprProxyServiceVMDispose(KprProxyServiceVM self)
{
	if (self) {
		if (FskThreadGetCurrent() == gProxyService.thread) {
			KprProxyServiceVMStop(self, self->machine);
		} else {
			FskThreadPostCallback(gProxyService.thread, (FskThreadCallback)KprProxyServiceVMStop, self, self->machine, NULL, NULL);
		}
	}
	return kFskErrNone;
}

void KprProxyServiceVMStart(KprProxyServiceVM self, KprProxyServiceVMCallback onStart, void *refcon)
{
	static xsCreation creation = {
		256 * 1024,
		1 * 1024,
		8 * 1024,
		1 * 1024,
		2048,
		16000,
		1993,
		127
	};

	if (FskThreadGetCurrent() != gProxyService.thread) {
		FskThreadPostCallback(gProxyService.thread, (FskThreadCallback)KprProxyServiceVMStart, self, onStart, refcon, NULL);
		return;
	}

	if (!FskListContains(gProxyServiceVMClients, self)) {
		KprProxyServiceVM vm = FskListGetNext(gProxyServiceVMClients, NULL);
		while (vm) {
			if (self->context == vm->context)
				break;
			vm = FskListGetNext(gProxyServiceVMClients, vm);
		}
		self->machine = (vm) ? vm->machine : xsCloneMachine(&creation, gShell->root, "proxy", self->context);
		FskListAppend(&gProxyServiceVMClients, self);

		if (onStart) KprProxyServiceVMExec(self, onStart, refcon);
	}
}

static void KprProxyServiceVMStop(KprProxyServiceVM self, xsMachine* machine)
{
	FskListRemove(&gProxyServiceVMClients, self);
	FskMemPtrDispose(self);

	if (machine) {
		self = FskListGetNext(gProxyServiceVMClients, NULL);
		while (self) {
			if (self->machine == machine)
				return;
			self = FskListGetNext(gProxyServiceVMClients, self);
		}

		fxCollectGarbage(machine);
		xsDeleteMachine(machine);
	}
}

void KprProxyServiceVMExec(KprProxyServiceVM self, KprProxyServiceVMCallback callback, void *refcon)
{
	if (FskThreadGetCurrent() == gProxyService.thread) {
		xsBeginHost(self->machine);
		{
			callback(the, refcon);
		}
		xsEndHost();
	} else {
		FskThreadPostCallback(gProxyService.thread, (FskThreadCallback)KprProxyServiceVMExec, self, callback, refcon, NULL);
	}
}

SInt32 KprProxyServiceVMGetIndex(KprProxyServiceVM self)
{
	return (self ? self->index : -1);
}

xsMachine *KprProxyServiceVMGetMachine(KprProxyServiceVM self)
{
	return (self ? self->machine : NULL);
}

void *KprProxyServiceVMGetRefcon(KprProxyServiceVM self)
{
	return (self ? self->refcon : NULL);
}

// --------------------------------------------------------------
// KprProxyServiceInstance
// --------------------------------------------------------------

static void KprProxyServiceInstanceInitialize(xsMachine *the, void *refcon);
static void KprProxyServiceInstanceDispose(xsMachine *the, void *refcon);

void KprProxyServiceInstanceInitialize(xsMachine *the, void *refcon)
{
	KprProxyServiceInstance self = refcon;
	char buffer[1024];
	xsVars(3);
	xsVar(0) = xsCall1(xsGet(xsGlobal, xsID_require), xsID_weak, xsString(self->from));
	if (self->path) {
		xsVar(1) = xsNewHostFunction(KprProxyServiceTargeInvoke, 4);
		xsVar(2) = xsGet(xsGet(xsGlobal, xsID_ProxyService), xsID_prototype);
		xsVar(2) = xsNewInstanceOf(xsVar(2));
		snprintf(buffer, sizeof(buffer), "xkpr://%s/proxy/%8.8lX?key=", self->context->id, KprProxyServiceVMGetIndex(self->vm));
		xsSet(xsVar(2), xsID_url, xsString(buffer));
		xsVar(2) = xsNew2(xsGlobal, xsID_Proxy, xsVar(1), xsVar(2));
		fxPush(xsVar(2));
		fxPushCount(the, 1);
	}
	else
		fxPushCount(the, 0);
	fxPush(xsVar(0));
	fxNew(the);
	self->slot = fxPop();
	xsRemember(self->slot);
}

void KprProxyServiceInstanceDispose(xsMachine *the, void *refcon)
{
	KprProxyServiceInstance self = refcon;
	xsForget(self->slot);

	FskMemPtrDispose(self->path);
	FskMemPtrDispose(self->from);
	FskMemPtrDispose(self);
}

void ProxyService_destructor(void* data)
{
	KprProxyServiceInstance self = data;
	if (self) {
		{
			KprHandler handler = self->context->firstHandler;
			FskList messages = gShell->messages;
			KprMessage message = FskListGetNext(messages, NULL);
			while (handler) {
				if (!FskStrCompare(self->path, handler->path))
					break;
				handler = handler->next;
			}
			if (handler)
				KprContextRemoveHandler(self->context, handler);
			message = FskListGetNext(messages, NULL);
			while (message) {
				if (!FskStrCompareWithLength(self->path, message->parts.path, message->parts.pathLength))
					KprMessageCancel(message);
				message = FskListGetNext(messages, message);
			}
		}

		{
			KprProxyServiceVM vm = self->vm;

			// This ensures the execution of xsForget in the vm thread,
			// and dispose of the vm will occur in this order.
			KprProxyServiceVMExec(vm, KprProxyServiceInstanceDispose, self);
			KprProxyServiceVMDispose(vm);
		}
	}
}

void ProxyService_constructor(xsMachine* the) 
{
	FskErr err = kFskErrNone;
	KprProxyServiceInstance self = NULL;
	char buffer[1024];
	KprHandler handler = NULL;
	KprBehavior behavior = NULL;
	KprContext context = xsGetContext(the);
	SInt32 index;
	xsVars(1);

	bailIfError(FskMemPtrNewClear(sizeof(KprProxyServiceInstanceRecord), &self));
	xsSetHostData(xsThis, self);

	bailIfError(KprProxyServiceVMNew(context, self, &self->vm));
	index = KprProxyServiceVMGetIndex(self->vm);

	self->context = context;
	self->from = FskStrDoCopy(xsToString(xsArg(0)));
	bailIfNULL(self->from);
	snprintf(buffer, sizeof(buffer), "/proxy/%8.8lX", index);
	self->path = FskStrDoCopy(buffer);
	bailIfNULL(self->path);
	
	xsVar(0) = xsNewHostFunction(KprProxyServiceTargeInvoke, 4);
	snprintf(buffer, sizeof(buffer), "xkpr://services/proxy/%8.8lX?key=", index);
	xsSet(xsThis, xsID_url, xsString(buffer));
	xsResult = xsNew2(xsGlobal, xsID_Proxy, xsVar(0), xsThis);
	
	if ((xsToInteger(xsArgc) > 1) && xsTest(xsArg(1))) {
		bailIfError(KprHandlerNew(&handler, self->path));
		KprContextPutHandler(self->context, handler);
		bailIfError(KprProxyServiceBehaviorNew(&behavior, (KprContent)handler, the, &xsArg(1)));
		KprContentSetBehavior((KprContent)handler, behavior);
	}
	KprProxyServiceVMStart(self->vm, KprProxyServiceInstanceInitialize, self);

bail:
	if (err && self) {
		if (behavior)
			KprContentSetBehavior((KprContent)handler, NULL);
		if (handler)
			KprContextRemoveHandler(self->context, handler);
		KprProxyServiceVMDispose(self->vm);
		FskMemPtrDispose(self->path);
		FskMemPtrDispose(self->from);
		FskMemPtrDispose(self);
	}
	xsThrowIfFskErr(err);
}

typedef struct KprTimeoutStruct KprTimeoutRecord, *KprTimeout;

struct KprTimeoutStruct {
	FskTimeCallBack callback;
	xsMachine* machine;
	xsSlot slot;
};

void KPR_clearTimeout(xsMachine* the)
{
	KprTimeout self = xsGetHostData(xsArg(0));
	if (self) {
		xsForget(self->slot); 
		FskTimeCallbackDispose(self->callback);
		xsSetHostData(xsArg(0), NULL);
		FskMemPtrDispose(self);
	}
}

void KPR_setTimeoutCallback(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *it)
{
	KprTimeout self = it;
	xsMachine* machine = self->machine;
	xsBeginHost(machine);
	{
		xsVars(2);
		xsVar(0) = xsAccess(self->slot);
		xsVar(1) = xsGet(xsVar(0), xsID_callback);

		xsForget(self->slot); 
		FskTimeCallbackDispose(self->callback);
		FskMemPtrDispose(self);
		xsSetHostData(xsVar(0), NULL);
		
		(void)xsCallFunction0(xsVar(1), xsUndefined);
	}
	xsEndHost(machine);
}

void KPR_setTimeoutDestructor(void* data)
{
	KprTimeout self = data;
	if (self) {
		// only at xsDeleteMachine
		FskTimeCallbackDispose(self->callback);
		FskMemPtrDispose(self);
	}
}

void KPR_setTimeout(xsMachine* the)
{
	KprTimeout self = NULL;
	FskTimeRecord when;
	
	xsThrowIfFskErr(FskMemPtrNewClear(sizeof(KprTimeoutRecord), &self));
	
	xsResult = xsNewHostObject(KPR_setTimeoutDestructor);
	xsSetHostData(xsResult, self);
	xsSet(xsResult, xsID_callback, xsArg(0));
	self->machine = the;
	self->slot = xsResult;
	xsRemember(self->slot); 
	
	FskTimeCallbackNew(&self->callback);
	FskTimeGetNow(&when);
	FskTimeAddMS(&when, xsToInteger(xsArg(1)));
	FskTimeCallbackSet(self->callback, &when, KPR_setTimeoutCallback, self);
}







