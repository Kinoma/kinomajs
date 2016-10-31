#include "kprMessage.h"
#include "kprShell.h"
#include "kprURL.h"

//static void ServicesStart(KprService self, FskThread thread, xsMachine* the);
static void ServicesStop(KprService self);
static void ServicesCancel(KprService self, KprMessage message);
static void ServicesInvoke(KprService self, KprMessage message);

FskExport(FskErr) services_fskLoad(FskLibrary it)
{
	return kFskErrNone;
}


FskExport(FskErr) services_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

void service_destructor(void* data) 
{
	KprService service = data;
	if (service) {
		KprServiceUnregister(service);
		xsBeginHost(service->machine);
		{
			xsVars(2);
			{
				xsTry {
					xsVar(0) = xsGet(xsGlobal, xsID("require"));
					xsVar(0) = xsGet(xsVar(0), xsID("__exports__"));
					xsEnterSandbox();
					xsResult = xsGet(xsVar(0), xsID("onStop"));
					xsLeaveSandbox();
					if (xsTest(xsResult))
						xsCallFunction0(xsResult, xsGlobal);
				}
				xsCatch {
				}
			}
		}
		xsEndHost(service->machine);
		if (service->machine)
			xsDeleteMachine(service->machine);
		if (service->id)
			FskMemPtrDispose(service->id);
		FskMemPtrDispose(service);
	}
}

void Service_constructor(xsMachine* the) 
{
	xsAllocation allocation = {
		32 * 1024, /* initialChunkSize */
		16 * 1024, /* incrementalChunkSize */
		2048,      /* initialHeapCount */
		1024,      /* incrementalHeapCount */
		1024,      /* stackCount */
		4096,      /* symbolCount */
		1993       /* symbolModulo */
	};

	xsStringValue scheme = xsToString(xsArg(0));
	xsStringValue path = NULL; 
	KprService service = NULL;
	xsTry {
		xsThrowIfFskErr(FskMemPtrNewClear(sizeof(KprServiceRecord), &service));
		xsSetHostData(xsThis, service);
		service->flags = kprServicesThread;
		service->id = FskStrDoCopy(scheme);
		xsThrowIfNULL(service->id);
		service->machine = xsAliasMachine(&allocation, gShell->root, service->id, service);
		xsThrowIfNULL(service->machine);
		service->accept = KprServiceAccept;
		service->cancel = ServicesCancel;
		service->invoke = ServicesInvoke;
		service->start = NULL;
		service->stop = ServicesStop;
		path = FskStrDoCopy(xsToString(xsArg(1)));
		xsThrowIfNULL(path);
		
		xsBeginHost(service->machine);
		{
			xsVars(2);
			xsVar(0) = xsNewHostFunction(KPR_require, 1);
			xsSet(xsVar(0), xsID("uri"), xsString(gShell->url));
			xsNewHostProperty(xsGlobal, xsID("require"), xsVar(0), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
			xsVar(1) = xsCallFunction1(xsVar(0), xsGlobal, xsString(path));
			xsSet(xsVar(0), xsID("__exports__"), xsVar(1));
			xsEnterSandbox();
			xsResult = xsGet(xsVar(1), xsID("onStart"));
			xsLeaveSandbox();
			if (xsTest(xsResult)) 
				xsCallFunction0(xsResult, xsGlobal);
			
		}
		xsEndHost(service->machine);
		
		FskMemPtrDispose(path);
		path = NULL;
		KprServiceRegister(service);
	}
	xsCatch {
		if (path)
			FskMemPtrDispose(path);
		if (service) {
			if (service->machine)
				xsDeleteMachine(service->machine);
			if (service->id)
				FskMemPtrDispose(service->id);
			FskMemPtrDispose(service);
		}
		xsThrow(xsException);
	}
}


void ServicesCancel(KprService self UNUSED, KprMessage message UNUSED)
{
	// ??
}

void ServicesInvoke(KprService self, KprMessage message)
{
	FskErr err = kFskErrNone;
	if (KprMessageContinue(message)) {
		xsBeginHost(self->machine);
		{
			xsVars(2);
			{
				xsTry {
					KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
					xsVar(0) = xsGet(xsGlobal, xsID("require"));
					xsVar(0) = xsGet(xsVar(0), xsID("__exports__"));
                    xsEnterSandbox();
					xsVar(0) = xsGet(xsVar(0), xsID("onInvoke"));
                    xsLeaveSandbox();
					xsVar(1) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("message")));
					xsSetHostData(xsVar(1), message);
					message->usage++; // host
					xsResult = xsCallFunction1(xsVar(0), xsGlobal, xsVar(1));
					if (target) 
						target->result = xsMarshall(xsResult);
				}
				xsCatch {
					err = FskStrToNum(xsToString(xsGet(xsException, xsID("message"))));
					if (!err)
						err = kFskErrUnknown;
				}
			}
		}
		xsEndHost(self->machine);
	}
	message->error = err;
	switch (err) {
	case kFskErrNone: message->status = 200; break;
	case kFskErrNotFound: message->status = 404; break;
	case kFskErrIsBusy: message->status = 409; break;
	default: message->status = 500; break;
	}
	KprMessageComplete(message);
}

void ServicesStop(KprService self UNUSED)
{
	// ??
}
