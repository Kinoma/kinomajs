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
#include "kprPins.h"

#include "FskExtensions.h"
#include "kprMessage.h"
#include "kprShell.h"

#if TARGET_OS_WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

extern void KPR_require(xsMachine* the);

static FskErr KprPinsNew(KprPins *it);
static void KprPinsCancel(KprService service, KprMessage message);
static void KprPinsDispose(KprPins self);
static KprPinsPoller KprPinsFindPoller(KprPins self, KprMessage message);
static void KprPinsInvoke(KprService service, KprMessage message);
#ifdef DEVICE
static void KprPinsLoop(void* theParameter);
#endif
static void KprPinsStart(KprService service, FskThread thread, xsMachine* the);
static void KprPinsStop(KprService service);

static FskErr KprPinsListenerNew(KprPinsListener *it, KprPins pins, KprMessage message);
static void KprPinsListenerDispose(KprPinsListener self, KprPins pins);
static void KprPinsListenerFind(KprPinsListener *it, KprPins pins, KprMessage message);

static FskErr KprPinsPollerNew(KprPinsPoller *it, KprPinsListener listener, KprMessage message, FskAssociativeArray query);
static void KprPinsPollerDispose(KprPinsPoller self, KprPinsListener listener);
static void KprPinsPollerFind(KprPinsPoller *it, KprPinsListener listener, KprMessage message);
static void KprPinsPollerStep(FskTimeCallBack callback, const FskTime time, void *param);

static FskErr exceptionToFskErr(xsMachine *the);

static KprPins gPins = NULL;
static KprServiceRecord gPinsService = {
	NULL,
#ifdef DEVICE
	kprServicesThread,
#else
	0,
#endif
	"pins:",
	NULL,
	NULL,
	KprServiceAccept,
	KprPinsCancel,
	KprPinsInvoke,
	KprPinsStart,
	KprPinsStop,
	NULL,
	NULL,
	NULL
};
#ifdef DEVICE
static FskThread gPinsThread = NULL;
static Boolean gPinsThreadQuitting = false;
#endif

static Boolean gBreakOnException = true;
static Boolean gBreakOnLaunch = false;

FskExport(FskErr) kprPins_fskLoad(FskLibrary it)
{
#if K4GEN2
	FskHardwarePinsInit();
#endif
	KprServiceRegister(&gPinsService);
	return kFskErrNone;
}

FskExport(FskErr) kprPins_fskUnload(FskLibrary it)
{
	return kFskErrNone;
}


#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprPinsInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprPins", FskInstrumentationOffset(KprPinsRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprPinsNew(KprPins *it)
{
	FskErr err = kFskErrNone;
	KprPins self;
	bailIfError(FskMemPtrNewClear(sizeof(KprPinsRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprPinsInstrumentation);
bail:
	return err;
}

void KprPinsCancel(KprService service UNUSED, KprMessage message UNUSED)
{
}

void KprPinsDispose(KprPins self)
{
	if (self) {
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprPinsInvoke(KprService service, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprPins self = gPins;
	if (KprMessageContinue(message)) {
		KprPinsListener listener = NULL;
		KprPinsListenerFind(&listener, self, message);
		if (!FskStrCompareWithLength(message->parts.path, "close", message->parts.pathLength)) {
			if (listener) {
				xsBeginHostSandboxCode(listener->the, NULL);
				{
                    KprPinsPoller poller = FskListGetNext(listener->pollers, NULL);
                    while (poller) {
                        KprPinsPoller next = FskListGetNext(listener->pollers, poller);
                        KprPinsPollerDispose(poller, listener);
                        poller = next;
                    }
                    {
						xsTry {
							(void)xsCall0(listener->pins, xsID("close"));
						}
						xsCatch {
							err = exceptionToFskErr(the);
						}
					}
				}
				xsEndHostSandboxCode();
				KprPinsListenerDispose(listener, self);
			}
			else {
				err = kFskErrNotFound;
			}
		}
		else if (!FskStrCompareWithLength(message->parts.path, "configure", message->parts.pathLength)) {
			if (listener)
				err = kFskErrIsBusy;
			else {
				err = KprPinsListenerNew(&listener, self, message);
            }
			if (!err) {
				xsBeginHostSandboxCode(listener->the, NULL);
				{
					xsTry {
						if (message->request.body && (message->request.size == 0xFFFFFFFF))
							xsResult = xsDemarshall(message->request.body);
						else
							xsResult = xsNewInstanceOf(xsObjectPrototype);
						(void)xsCall1(listener->pins, xsID("configure"), xsResult);
					}
					xsCatch {
						err = exceptionToFskErr(the);
					}
				}
				xsEndHostSandboxCode();
			}
		}
		else if (!FskStrCompareWithLength(message->parts.path, "clearAllBreakpoints", message->parts.pathLength)) {
			if (listener) {
				xsBeginHost(listener->the);
				{
					{
						xsTry {
							(void)xsCall0(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("clearAllBreakpoints"));
						}
						xsCatch {
							err = exceptionToFskErr(the);
						}
					}
				}
				xsEndHost(listener->the);
			}
			else {
				err = kFskErrNotFound;
			}
		}
		else if (!FskStrCompareWithLength(message->parts.path, "clearBreakpoint", message->parts.pathLength)) {
			if (listener) {
				xsBeginHost(listener->the);
				{
					FskAssociativeArray query = NULL;
					char* file;
					char* line;
					{
						xsTry {
							xsThrowIfFskErr(KprQueryParse(message->parts.query, &query));
							file = FskAssociativeArrayElementGetString(query, "file");
							line = FskAssociativeArrayElementGetString(query, "line");
							(void)xsCall2(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("clearBreakpoint"), xsString(file), xsString(line));
						}
						xsCatch {
							err = exceptionToFskErr(the);
						}
					}
					FskAssociativeArrayDispose(query);
				}
				xsEndHost(listener->the);
			}
			else {
				err = kFskErrNotFound;
			}
		}
		else if (!FskStrCompareWithLength(message->parts.path, "setBreakpoint", message->parts.pathLength)) {
			if (listener) {
				xsBeginHost(listener->the);
				{
					FskAssociativeArray query = NULL;
					char* file;
					char* line;
					{
						xsTry {
							xsThrowIfFskErr(KprQueryParse(message->parts.query, &query));
							file = FskAssociativeArrayElementGetString(query, "file");
							line = FskAssociativeArrayElementGetString(query, "line");
							(void)xsCall2(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("setBreakpoint"), xsString(file), xsString(line));
						}
						xsCatch {
							err = exceptionToFskErr(the);
						}
					}
					FskAssociativeArrayDispose(query);
				}
				xsEndHost(listener->the);
			}
			else {
				err = kFskErrNotFound;
			}
		}
		else if (!FskStrCompareWithLength(message->parts.path, "breakOnException", message->parts.pathLength)) {
            FskAssociativeArray query;

            if (kFskErrNone == KprQueryParse(message->parts.query, &query)) {
                const char *value = FskAssociativeArrayElementGetString(query, "break");
                if (value) {
                    Boolean breakOnException = 0 == FskStrCompare("true", value);
                    if (listener) {
                        xsBeginHost(listener->the);
                        xsCall1_noResult(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("setBreakOnException"), xsBoolean(breakOnException));
                        xsEndHost(listener->the);
                    }
                    else
                        gBreakOnException = breakOnException;
                }
                FskAssociativeArrayDispose(query);
            }
        }
		else if (!FskStrCompareWithLength(message->parts.path, "breakOnLaunch", message->parts.pathLength)) {
            FskAssociativeArray query;

            if (kFskErrNone == KprQueryParse(message->parts.query, &query)) {
                const char *value = FskAssociativeArrayElementGetString(query, "break");
                if (value)
                    gBreakOnLaunch = 0 == FskStrCompare("true", value);
                FskAssociativeArrayDispose(query);
            }
        }
		else {
			if (listener) {
				xsBeginHostSandboxCode(listener->the, NULL);
				{
					KprPinsPoller poller = NULL;
					FskAssociativeArray query = NULL;
					KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
					char* value;
					xsVars(2);
					{
						xsTry {
							if (message->parts.query)
								xsThrowIfFskErr(KprQueryParse(message->parts.query, &query));
							xsResult = xsGet(listener->pins, xsID("behaviors"));
							xsResult = xsGetAt(xsResult, xsStringBuffer(message->parts.path + 1, message->parts.pathLength - message->parts.nameLength - 2));
							if (!xsTest(xsResult))
								xsThrowDiagnosticIfFskErr(kFskErrNotFound, "BLL %s not found when invoking function %s", xsToString(xsStringBuffer(message->parts.path + 1, message->parts.pathLength - message->parts.nameLength - 2)), xsToString(xsStringBuffer(message->parts.name, message->parts.nameLength)));
							xsVar(0) = xsGetAt(xsResult, xsStringBuffer(message->parts.name, message->parts.nameLength));
							if (!xsTest(xsVar(0)))
								xsThrowDiagnosticIfFskErr(kFskErrNotFound, "BLL function %s not found", xsToString(xsStringBuffer(message->parts.name, message->parts.nameLength)));
							if (message->request.body && (message->request.size == 0xFFFFFFFF))
								xsVar(1) = xsDemarshall(message->request.body);
						
							value = FskAssociativeArrayElementGetString(query, "repeat");
							if (!FskStrCompare(value, "on")) {
								KprPinsPollerFind(&poller, listener, message);
								if (poller)
									xsError(kFskErrIsBusy);
								xsThrowIfFskErr(KprPinsPollerNew(&poller, listener, message, query));
							}
							else if (!FskStrCompare(value, "off")) {
								KprPinsPollerFind(&poller, listener, message);
								if (!poller)
									xsError(kFskErrNotFound);
								KprPinsPollerDispose(poller, listener);
							}
							else {
								xsResult = xsCallFunction1(xsVar(0), xsResult, xsVar(1));
								if (target) 
									target->result = xsMarshall(xsResult);
							}
						}
						xsCatch {
							err = exceptionToFskErr(the);
						}
					}
					FskAssociativeArrayDispose(query);
				}
				xsEndHostSandboxCode();
			}
			else {
				err = kFskErrNotFound;
			}
		}
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

#ifdef DEVICE
void KprPinsLoop(void* theParameter UNUSED)
{
	FskErr err = kFskErrNone;
	FskThreadInitializationComplete(FskThreadGetCurrent());
	bailIfError(KprPinsNew(&gPins));
	gPinsService.machine = NULL;
	gPinsService.thread = gPinsThread;
	while (!gPinsThreadQuitting) {
		FskThreadRunloopCycle(-1);
    }
	KprPinsDispose(gPins);
bail:
	return;
}
#endif

void KprPinsStart(KprService service, FskThread thread, xsMachine* the)
{
	FskErr err = kFskErrNone;
#ifdef DEVICE
	UInt32 flags = kFskThreadFlagsWaitForInit | kFskThreadFlagsJoinable;
	gPinsThreadQuitting = false;
	bailIfError(FskThreadCreate(&gPinsThread, KprPinsLoop, flags, NULL, "pins"));
#else
	gPinsService.thread = thread;
	gPinsService.machine = the;
	bailIfError(KprPinsNew(&gPins));
#endif
bail:
	return;
}

void KprPinsStop(KprService service)
{
#ifdef DEVICE
	gPinsThreadQuitting = true;
	FskThreadJoin(gPinsThread);
#else
	KprPinsDispose(gPins);
	gPinsService.machine = NULL;
	gPinsService.thread = NULL;
#endif
}

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprPinsListenerInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprPinsListener", FskInstrumentationOffset(KprPinsListenerRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprPinsListenerNew(KprPinsListener *it, KprPins pins, KprMessage message)
{
#ifdef DEVICE
	#define globalsCount 40
	static char* globals[globalsCount] = {"Behavior","blendColors","Canvas","Column","Container","Content","controlKey","Effect","Event","Files","FskMediaProperty","FskMediaReader","Handler","Host","HTTP","include","instrument","Label","launchURI","Layer","Layout","Line","localStorage","Media","Message","optionKey","Picture","Port","screenScale","Scroller","shiftKey","Skin","Sound","Style","system","Text","Texture","Thumbnail","touches","Transition"};
	#define kprsCount 37
	static char* kprs[kprsCount] = {"application","behavior","canvas","canvasGradient","canvasGradientStop","canvasLinearGradient","canvasPattern","canvasRadialGradient","canvasRenderingContext2D","column","container","content","dummy","effect","events","handler","host","imageData","label","layer","layout","line","MD5","media","message","picture","port","scroller","shell","skin","sound","style","text","textMetrics","texture","thumbnail","transition"};
	xsAllocation allocation = {
		32 * 1024, /* initialChunkSize */
		16 * 1024, /* incrementalChunkSize */
		2048,      /* initialHeapCount */
		1024,      /* incrementalHeapCount */
		1024,      /* stackCount */
		4096,      /* symbolCount */
		1993       /* symbolModulo */
	};
#endif
	FskErr err = kFskErrNone;
	KprPinsListener self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprPinsListenerRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprPinsListenerInstrumentation);
	FskListAppend(&pins->listeners, self);
	FskInstrumentedItemSetOwner(self, pins);

	self->referrer = FskStrDoCopy(KprMessageGetRequestHeader(message, "referrer"));
	bailIfNULL(self->referrer);
#ifdef DEVICE
	self->name = FskStrDoCat("pins @ ", self->referrer);
	bailIfNULL(self->name);
	self->the = xsAliasMachine(&allocation, gShell->root, self->name, self);
	bailIfNULL(self->the);
	xsBeginHost(self->the);
	{
		int i;
		for (i = 0; i < globalsCount; i++)
			xsDelete(xsGlobal, xsID(globals[i]));
		xsResult = xsGet(xsGlobal, xsID("KPR"));
		for (i = 0; i < kprsCount; i++)
			xsDelete(xsResult, xsID(kprs[i]));
		xsResult = xsNewHostFunction(KPR_require, 1);
		xsSet(xsResult, xsID("uri"), xsString(gShell->url));
		xsNewHostProperty(xsGlobal, xsID("require"), xsResult, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
		self->pins = xsGet(xsGlobal, xsID("PINS"));
        xsCall1_noResult(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("setBreakOnException"), xsBoolean(gBreakOnException));
        if (gBreakOnLaunch)
            xsDebugger();
		xsRemember(self->pins);
	}
	xsEndHost(self->the);
#else
	self->the = gPinsService.machine;
	xsBeginHost(self->the);
	{
		xsResult = xsGet(xsGlobal, xsID("PINS"));
		self->pins = xsNewInstanceOf(xsResult);
		xsRemember(self->pins);
	}
	xsEndHost(self->the);
#endif
bail:
	if (kFskErrNone != err) {
		KprPinsListenerDispose(self, pins);
		*it = NULL;
	}
	return err;
}

void KprPinsListenerDispose(KprPinsListener self, KprPins pins)
{
	if (self) {
		if (self->the)
			fxForget(self->the, &self->pins);
#ifdef DEVICE
		if (self->the)
			xsDeleteMachine(self->the);
		FskMemPtrDispose(self->name);
#else
		if (self->the) {
			xsBeginHost(self->the);
			{
				xsResult = xsNewInstanceOf(xsObjectPrototype);
				xsSet(xsGet(xsGlobal, xsID("KPR")), xsID("modules"), xsResult);
			}
			xsEndHost(self->the);
		}
#endif
		FskMemPtrDispose(self->referrer);
		FskListRemove(&pins->listeners, self);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprPinsListenerFind(KprPinsListener *it, KprPins pins, KprMessage message)
{
	char* referrer = KprMessageGetRequestHeader(message, "referrer");
	*it = NULL;
	if (referrer) {
		KprPinsListener listener = FskListGetNext(pins->listeners, NULL);
		while (listener) {
			if (!FskStrCompare(referrer, listener->referrer))
				break;
			listener = FskListGetNext(pins->listeners, listener);
		}
		*it = listener;
	}
}

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprPinsPollerInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprPinsPoller", FskInstrumentationOffset(KprPinsPollerRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

void KprPinsPoller_destructor(void* the)
{
}

FskErr KprPinsPollerNew(KprPinsPoller *it, KprPinsListener listener, KprMessage message, FskAssociativeArray query)
{
	xsMachine* the = listener->the;
	FskErr err = kFskErrNone;
	KprPinsPoller self = NULL;
	char* value;
	bailIfError(FskMemPtrNewClear(sizeof(KprPinsPollerRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprPinsPollerInstrumentation);
	FskListAppend(&listener->pollers, self);
	self->listener = listener;
	FskInstrumentedItemSetOwner(self, listener);
		
	bailIfError(FskMemPtrNewClear(message->parts.pathLength, &self->path));
	FskStrNCopy(self->path, message->parts.path, message->parts.pathLength);

	self->function = xsVar(0);
	if (xsTest(self->function))
		xsRemember(self->function);
	self->instance = xsResult;
	if (xsTest(self->instance))
		xsRemember(self->instance);
	self->parameters = xsVar(1);
	if (xsTest(self->parameters))
		xsRemember(self->parameters);
	
	value = FskAssociativeArrayElementGetString(query, "callback");
	if (value)
		bailIfError(KprURLMerge(listener->referrer, value, &self->url));
		
	value = FskAssociativeArrayElementGetString(query, "timer");
	if (value) {
		xsVar(0) = xsGet(xsResult, xsID(value));
		if (!xsTest(xsVar(0))) {
			err = kFskErrNotFound;
			goto bail;
		}
		self->timer = xsVar(0);
		xsRemember(self->timer);
		xsVar(1) = xsNewHostObject(KprPinsPoller_destructor);
		xsSetHostData(xsVar(1), self);
		xsResult = xsCall1(xsVar(0), xsID("repeat"), xsVar(1));
		if (!xsTest(xsResult))
			goto bail;
		self->interval = xsToInteger(xsResult);
	}
	
	value = FskAssociativeArrayElementGetString(query, "seconds");
	if (value)
		self->interval += 1000 * FskStrToNum(value);
	value = FskAssociativeArrayElementGetString(query, "mseconds");
	if (value)
		self->interval += FskStrToNum(value);
	value = FskAssociativeArrayElementGetString(query, "interval");
	if (value)
		self->interval += FskStrToNum(value);
	if (self->interval == 0)
		self->interval = 100;
	FskTimeCallbackNew(&self->timeCallback);
	
	value = FskAssociativeArrayElementGetString(query, "sendParamsOnce");
	self->parametersAlways = (value && !FskStrCompare(value, "true")) ? 1 : 2;
	value = FskAssociativeArrayElementGetString(query, "skipFirst");
	if (!value || FskStrCompare(value, "true"))
		KprPinsPollerRun(self);
	FskTimeCallbackScheduleFuture(self->timeCallback, self->interval / 1000, self->interval % 1000, KprPinsPollerStep, self);
bail:
	if (kFskErrNone != err) {
		KprPinsPollerDispose(self, listener);
		*it = NULL;
	}
	return err;
}

void KprPinsPollerDispose(KprPinsPoller self, KprPinsListener listener)
{
	xsMachine* the = listener->the;
	if (self) {
		FskMemPtrDispose(self->url);
		FskTimeCallbackDispose(self->timeCallback);
		if (xsTest(self->timer)) {
			(void)xsCall1(self->timer, xsID("repeat"), xsNull);
			xsForget(self->timer);
		}
		if (xsTest(self->parameters))
			xsForget(self->parameters);
		if (xsTest(self->instance))
			xsForget(self->instance);
		if (xsTest(self->function))
			xsForget(self->function);
		FskListRemove(&listener->pollers, self);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprPinsPollerFind(KprPinsPoller *it, KprPinsListener listener, KprMessage message)
{
	KprPinsPoller poller = FskListGetNext(listener->pollers, NULL);
	*it = NULL;
	while (poller) {
		if (!FskStrCompareWithLength(message->parts.path, poller->path, message->parts.pathLength))
			break;
		poller = FskListGetNext(listener->pollers, poller);
	}
	*it = poller;
}


void KprPinsPollerRun(KprPinsPoller self)
{
	KprMessage message = NULL;
//	UInt32 size;
	xsBeginHostSandboxCode(self->listener->the, NULL);
	{
		if (self->parametersAlways > 0) {
			if (self->parametersAlways == 1)
				self->parametersAlways = 0;
			xsResult = xsCallFunction1(self->function, self->instance, self->parameters);
		}
		else
			xsResult = xsCallFunction0(self->function, self->instance);

		if (self->url && (xsTypeOf(xsResult) != xsUndefinedType)) {
			KprMessageNew(&message, self->url);
			message->request.body = xsMarshall(xsResult);
			message->request.size = 0xFFFFFFFF;
			/*
			if (xsTypeOf(xsResult) != xsUndefinedType) {
				if (xsIsInstanceOf(xsResult, xsChunkPrototype)) {
					size = xsToInteger(xsGet(xsResult, xsID("length")));
					KprMessageSetRequestBody(message, xsGetHostData(xsResult), size);
				}
				else {
					xsResult = xsCall1(xsGet(xsGlobal, xsID("JSON")), xsID("stringify"), xsResult);
					size = xsToInteger(xsGet(xsResult, xsID("length")));
					KprMessageSetRequestBody(message, xsToString(xsResult), size);
				}
			}
			*/
		}
	}
	xsEndHostSandboxCode();
	if (message)
		KprMessageInvoke(message, NULL, NULL, NULL);
}

void KprPinsPollerStep(FskTimeCallBack callback, const FskTime time, void *param)
{
	KprPinsPoller self = (KprPinsPoller)param;
	KprPinsPollerRun(self);
	FskTimeCallbackScheduleFuture(callback, self->interval / 1000, self->interval % 1000, KprPinsPollerStep, self);
}

FskErr exceptionToFskErr(xsMachine *the)
{
    FskErr err = FskStrToNum(xsToString(xsGet(xsException, xsID("message"))));
    return err ? err : kFskErrUnknown;
}

void KPR_message_get_requestObject(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	if (self->request.body && (self->request.size == 0xFFFFFFFF))
		xsResult = xsDemarshall(self->request.body);
}

void KPR_message_set_requestObject(xsMachine *the)
{
	KprMessage self = xsGetHostData(xsThis);
	self->request.body = xsMarshall(xsArg(0));
	self->request.size = 0xFFFFFFFF;
}

static FskErr FskhardwarepinsDelay(int delay) {
#if TARGET_OS_WIN32
	Sleep(delay*1000);
#else
	sleep(delay);
#endif
  return kFskErrNone;
}

static FskErr FskhardwarepinsMDelay(int delay) {
#if TARGET_OS_WIN32
	Sleep(delay);
#else
	usleep(delay*1000);
#endif
  return kFskErrNone;
}

static FskErr FskhardwarepinsUDelay(int delay) {
#if TARGET_OS_WIN32
	return kFskErrUnimplemented;
#else
	usleep(delay);
	return kFskErrNone;
#endif
}

void xs_hardwarepins_delay(xsMachine *the){
  int delay = xsToInteger(xsArg(0));
  FskhardwarepinsDelay(delay);
}

void xs_hardwarepins_mdelay(xsMachine *the){
  int delay = xsToInteger(xsArg(0));
  FskhardwarepinsMDelay(delay);
}

void xs_hardwarepins_udelay(xsMachine *the){
  int delay = xsToInteger(xsArg(0));
  FskhardwarepinsUDelay(delay);
}
