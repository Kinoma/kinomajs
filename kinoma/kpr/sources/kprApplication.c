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
#include "kprHTTPClient.h"
#include "kprSkin.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHandler.h"
#include "kprMessage.h"
#include "kprHTTPServer.h"
#include "kprURL.h"
#include "kprShell.h"
#include "kprTransition.h"
#include "kprUtilities.h"

#include "kprApplication.h"

/* HOST */

static void KprHostDispose(void* it);
static void KprHostPlaced(void* it);
static void KprHostSetWindow(void* it, KprShell shell, KprStyle style);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprHostInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprHost", FskInstrumentationOffset(KprHostRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprHostDispatchRecord = {
	"host",
	KprContainerActivated,
	KprContainerAdded,
	KprContainerCascade,
	KprHostDispose,
	KprContentDraw,
	KprContainerFitHorizontally,
	KprContainerFitVertically,
	KprContentGetBitmap,
	KprContainerHit,
	KprContentIdle,
	KprContainerInvalidated,
	KprContainerLayered,
	KprContentMark,
	KprContainerMeasureHorizontally,
	KprContainerMeasureVertically,
	KprContainerPlace,
	KprHostPlaced,
	KprContainerPredict,
	KprContainerReflowing,
	KprContainerRemoving,
	KprHostSetWindow,
	KprContainerShowing,
	KprContainerShown,
	KprContainerUpdate
};

FskErr KprHostNew(KprHost* it, KprCoordinates coordinates, char* url, char* id, Boolean breakOnStart, Boolean breakOnExceptions)
{
	FskErr err = kFskErrNone;
	KprHost self = NULL;
	KprApplication application = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprHostRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprHostInstrumentation);
	self->dispatch = &KprHostDispatchRecord;
	self->flags = kprHost | kprContainer | kprClip | kprVisible;
	KprContentInitialize((KprContent)self, coordinates, NULL, NULL);
	bailIfError(KprApplicationNew(&application, url, id, breakOnStart, breakOnExceptions));
	KprContainerAdd((KprContainer)self, (KprContent)application);
bail:
	return err;
}

void KprHostAdapt(KprHost self)
{
	kprDelegateAdapt(self->first);
}

/* HOST DISPATCH */

void KprHostDispose(void* it)
{
	KprHost self = it;
	FskInstrumentedItemSendMessageNormal(self, kprInstrumentedContentDeleteMachine, self->first);
	xsDeleteMachine(self->first->the);
	KprContainerDispose(it);
}

void KprHostPlaced(void* it)
{
	KprHost self = it;
	FskRectangleRecord bounds = *KprBounds(self);
	KprContainer container = self->container;
	KprContainerPlaced(it);
	while (container) {
		container->hole = bounds;
		bounds.x += container->bounds.x;
		bounds.y += container->bounds.y;
		container = container->container;
	}
}

void KprHostSetWindow(void* it, KprShell shell, KprStyle style)
{
	KprHost self = it;
	if (!(self->flags & kprRotating)) {
		KprShell former = self->shell;
		if (former && !shell)
			former->flags |= kprCollectGarbage;
		KprContainerSetWindow(it, shell, style);
	}
}

/* HOST ECMASCRIPT */

void KPR_Host(xsMachine* the)
{
	KprCoordinatesRecord coordinates;
	KprHost self;
	xsStringValue url = xsToString(xsArg(1));
	xsStringValue id = xsToString(xsArg(2));
	xsBooleanValue breakOnStart = false, breakOnException = false;
	if (xsToInteger(xsArgc) > 3) {
		breakOnStart = xsToBoolean(xsArg(3));
        if (xsToInteger(xsArgc) > 4)
            breakOnException = xsToBoolean(xsArg(4));
    }
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	xsThrowIfFskErr(KprHostNew(&self, &coordinates, url, id, breakOnStart, breakOnException));
	kprContentConstructor(KPR_Host);
}

void KPR_host_get_breakOnException(xsMachine* the)
{
#ifdef mxDebug
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	Boolean result = false;
	xsBeginHost(application->the);
	{
		result = xsTest(xsCall0(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("getBreakOnException")));
	}
	xsEndHost(application->the);
	xsResult = xsBoolean(result);
#endif
}

void KPR_host_get_debugging(xsMachine* the)
{
#ifdef mxDebug
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	Boolean result = false;
	xsBeginHost(application->the);
	{
		result = xsTest(xsCall0(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("getConnected")));
	}
	xsEndHost(application->the);
	xsResult = xsBoolean(result);
#endif
}

void KPR_host_get_id(xsMachine* the)
{
	KprContainer self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	if (application->id)
		xsResult = xsString(application->id);
}

void KPR_host_get_di(xsMachine* the)
{
	KprApplication self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	if (application->id) {
		char* di;
		xsThrowIfFskErr(KprAuthorityReverse(application->id, &di));
		xsResult = xsString(di);
		FskMemPtrDispose(di);
	}
}

void KPR_host_get_profiling(xsMachine* the)
{
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	Boolean result = false;
	xsBeginHost(application->the);
	{
		result = xsTest(xsCall0(xsGet(xsGlobal, xsID("xs")), xsID("isProfiling")));
	}
	xsEndHost(application->the);
	xsResult = xsBoolean(result);
}

void KPR_host_get_profilingDirectory(xsMachine* the)
{
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	FskErr err = kFskErrNone;
	char* directory = NULL;
	xsBeginHost(application->the);
	{
		xsResult = xsCall0(xsGet(xsGlobal, xsID("xs")), xsID("getProfilingDirectory"));
		if (xsTest(xsResult))
			err = KprPathToURL(xsToString(xsResult), &directory);
	}
	xsEndHost(application->the);
	if (directory) {
		xsResult = xsString(directory);
		FskMemPtrDispose(directory);
	}
	else {
		xsThrowIfFskErr(err);
	}
}

void KPR_host_get_rotating(xsMachine* the)
{
	KprHost self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprRotating) ? xsTrue : xsFalse;
}

void KPR_host_get_url(xsMachine* the)
{
	KprContainer self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	xsResult = xsString(application->url);
}

void KPR_host_set_breakOnException(xsMachine* the)
{
#ifdef mxDebug
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	Boolean flag = xsToBoolean(xsArg(0));
	xsBeginHost(application->the);
	{
		(void)xsCall1(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("setBreakOnException"), xsBoolean(flag));
	}
	xsEndHost(application->the);
#endif
}

void KPR_host_set_debugging(xsMachine* the)
{
#ifdef mxDebug
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	Boolean flag = xsToBoolean(xsArg(0));
	xsBeginHost(application->the);
	{
		(void)xsCall1(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("setConnected"), xsBoolean(flag));
	}
	xsEndHost(application->the);
#endif
}

void KPR_host_set_profilingDirectory(xsMachine* the)
{
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	char* directory = NULL;
	if (xsTest(xsArg(0))) {
		xsThrowIfFskErr(KprURLToPath(xsToString(xsArg(0)), &directory));
	}
	xsBeginHost(application->the);
	{
		if (directory) {
			xsResult = xsString(directory);
			FskMemPtrDispose(directory);
		}
		(void)xsCall1(xsGet(xsGlobal, xsID("xs")), xsID("setProfilingDirectory"), xsResult);
	}
	xsEndHost(application->the);
}

void KPR_host_set_profiling(xsMachine* the)
{
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	Boolean flag = xsToBoolean(xsArg(0));
	xsBeginHost(application->the);
	{
		if (flag)
			(void)xsCall0(xsGet(xsGlobal, xsID("xs")), xsID("startProfiling"));
		else
			(void)xsCall0(xsGet(xsGlobal, xsID("xs")), xsID("stopProfiling"));
	}
	xsEndHost(application->the);
}

void KPR_host_set_rotating(xsMachine *the)
{
	KprHost self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprRotating;
	else
		self->flags &= ~kprRotating;
}

void KPR_host_adapt(xsMachine* the)
{
	KprHost self = xsGetHostData(xsThis);
	KprHostAdapt(self);
	KprShellAdjust(self->shell);
}

void KPR_host_clearBreakpoint(xsMachine* the)
{
#ifdef mxDebug
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	xsStringValue file = xsToString(xsArg(0));
	xsStringValue line = xsToString(xsArg(1));
	xsBeginHost(application->the);
	{
		(void)xsCall2(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("clearBreakpoint"), xsString(file), xsString(line));
	}
	xsEndHost(application->the);
#endif
}

void KPR_host_clearAllBreakpoints(xsMachine* the)
{
#ifdef mxDebug
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	xsBeginHost(application->the);
	{
		(void)xsCall0(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("clearAllBreakpoints"));
	}
	xsEndHost(application->the);
#endif
}

void KPR_host_debugger(xsMachine* the)
{
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	xsBeginHostSandboxCode(application->the, NULL);
	{
		xsDebugger();
	}
	xsEndHostSandboxCode();
}

void KPR_host_launch(xsMachine* the)
{
	KprContainer self = xsGetHostData(xsThis);
	KprContent content = self->first;
    KprShellSetFocus(self->shell, content);
	kprDelegateLaunch(content);
}

void KPR_host_purge(xsMachine* the)
{
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	xsBeginHostSandboxCode(application->the, NULL);
	{
		FskInstrumentedItemSendMessageNormal(self, kprInstrumentedContentBeginCollect, self);
		xsCollectGarbage();
		FskInstrumentedItemSendMessageNormal(self, kprInstrumentedContentEndCollect, self);
	}
	xsEndHostSandboxCode();
	KprContextPurge(application, false);
}

void KPR_host_quit(xsMachine* the)
{
	KprContainer self = xsGetHostData(xsThis);
	KprContent content = self->first;
	kprDelegateQuit(content);
}

void KPR_host_quitting(xsMachine* the)
{
	KprContainer container = xsGetHostData(xsThis);
	KprContent content = container->first;
	KprScriptBehavior self = (KprScriptBehavior)content->behavior;
#if SUPPORT_INSTRUMENTATION
	void* params[2] = { self, "xsID_onQuitting" };
	FskInstrumentedItemSendMessageDebug(content, kprInstrumentedContentCallBehavior, params);
#endif
    if (self) {
        xsBeginHostSandboxCode(self->the, self->code);
        {
 			xsVars(2);
			xsVar(0) = xsAccess(self->slot);
			if (xsFindResult(xsVar(0), xsID_onQuitting)) {
				xsVar(1) = kprContentGetter(content);
				(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
			}
        }
        xsEndHostSandboxCode();
    }
}

void KPR_host_setBreakpoint(xsMachine* the)
{
#ifdef mxDebug
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	xsStringValue file = xsToString(xsArg(0));
	xsStringValue line = xsToString(xsArg(1));
	xsBeginHost(application->the);
	{
		(void)xsCall2(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("setBreakpoint"), xsString(file), xsString(line));
	}
	xsEndHost(application->the);
#endif
}


void KPR_host_trace(xsMachine* the)
{
	KprHost self = xsGetHostData(xsThis);
	KprApplication application = (KprApplication)self->first;
	xsStringValue text = xsToString(xsArg(0));
	xsBeginHost(application->the);
	{
		xsTrace(text);
	}
	xsEndHost(application->the);
}

/* APPLICATION */

static void KprApplicationDispose(void* it);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprApplicationInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprApplication", FskInstrumentationOffset(KprApplicationRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprApplicationDispatchRecord = {
	"application",
	KprContainerActivated,
	KprContainerAdded,
	KprContainerCascade,
	KprApplicationDispose,
	KprContentDraw,
	KprContainerFitHorizontally,
	KprContainerFitVertically,
	KprContentGetBitmap,
	KprContainerHit,
	KprContentIdle,
	KprContainerInvalidated,
	KprContainerLayered,
	KprContextMark,
	KprContainerMeasureHorizontally,
	KprContainerMeasureVertically,
	KprContainerPlace,
	KprContainerPlaced,
	KprContainerPredict,
	KprContainerReflowing,
	KprContainerRemoving,
	KprContainerSetWindow,
	KprContainerShowing,
	KprContainerShown,
	KprContainerUpdate
};

FskErr KprApplicationNew(KprApplication* it, char* url, char* id, Boolean breakOnStart, Boolean breakOnExceptions)
{
	KprCoordinatesRecord coordinates = { kprLeftRight, kprTopBottom, 0, 0, 0, 0, 0, 0 };
	xsAllocation allocation = {
		2 * 1024 * 1024,
		1024 * 1024,
		64 * 1024,
		8 * 1024,
		2048,
		16000,
		1993
	};
	FskErr err = kFskErrNone;
	KprApplication self;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprApplicationRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprApplicationInstrumentation);
	self->dispatch = &KprApplicationDispatchRecord;
	self->flags = kprContainer | kprClip | kprVisible;
	KprContentInitialize((KprContent)self, &coordinates, NULL, NULL);
	bailIfError(KprURLMerge(gShell->url, url, &self->url));	
	if (id) {
		self->id = FskStrDoCopy(id);	
		bailIfNULL(self->id);
	}
	self->the = xsAliasMachine(&allocation, gShell->root, self->url, self);
	if (!self->the) 
		BAIL(kFskErrMemFull);
	FskInstrumentedItemSendMessageNormal(self, kprInstrumentedContentCreateMachine, self);
	xsBeginHost(self->the);
	xsResult = xsNewHostFunction(KPR_include, 1);
	xsSet(xsResult, xsID("uri"), xsString(self->url));
	xsNewHostProperty(xsGlobal, xsID("include"), xsResult, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsResult = xsNewHostFunction(KPR_require, 1);
	xsSet(xsResult, xsID("uri"), xsString(self->url));
	xsNewHostProperty(xsGlobal, xsID("require"), xsResult, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("application")));
	self->slot = xsResult;
	xsSetHostData(xsResult, self);
	(void)xsCall1(xsGet(xsGlobal, xsID("Object")), xsID("seal"), xsResult);
	xsNewHostProperty(xsGlobal, xsID("application"), xsResult, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsGlobal, xsID("shell"), xsNull, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	if (breakOnStart)
		xsDebugger();
    if (breakOnExceptions)
		(void)xsCall1(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("setBreakOnException"), xsBoolean(breakOnExceptions));
	(void)xsCall1(xsGlobal, xsID("include"), xsString(self->url));
	xsEndHost(self->the);
	KprContentChainPrepend(&gShell->applicationChain, self, 0, NULL);
bail:
	return err;
}

/* APPLICATION DISPATCH */

void KprApplicationDispose(void* it)
{
	KprApplication self = it;
	KprContentChainRemove(&gShell->applicationChain, self);
	KprContextDisposeHandlers((KprContext)self);
	FskMemPtrDispose(self->id);
	FskMemPtrDispose(self->url);
	KprContextPurge(self, true);
	KprContainerDispose(it);
}

/* APPLICATION ECMASCRIPT */

void KPR_application_get_container(xsMachine *the UNUSED)
{
}

void KPR_application_get_di(xsMachine* the)
{
	KprApplication self = xsGetHostData(xsThis);
	if (self->id) {
		char* di;
		xsThrowIfFskErr(KprAuthorityReverse(self->id, &di));
		xsResult = xsString(di);
		FskMemPtrDispose(di);
	}
}

void KPR_application_get_id(xsMachine* the)
{
	KprApplication self = xsGetHostData(xsThis);
	if (self->id)
		xsResult = xsString(self->id);
}

void KPR_application_get_size(xsMachine *the)
{
	KprContainer container = xsGetHostData(xsThis);
	Boolean horizontal = true;
	SInt32 left = 0;
	SInt32 width = 0;
	SInt32 right = 0;
	Boolean vertical = true;
	SInt32 top = 0;
	SInt32 height = 0;
	SInt32 bottom = 0;
	while (container) {
		if (horizontal) {
			if ((container->coordinates.horizontal & kprWidth)) {
				width = container->coordinates.width - left - right;
				horizontal = false;
				if (!vertical)
					break;
			}
			else {
				left += container->coordinates.left;
				right += container->coordinates.right;
			}
		}
		if (vertical) {
			if ((container->coordinates.vertical & kprHeight)) {
				height = container->coordinates.height - top - bottom;
				vertical = false;
				if (!horizontal)
					break;
			}
			else {
				top += container->coordinates.top;
				bottom += container->coordinates.bottom;
			}
		}
		container = container->container;
	}
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(width), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(height), xsDefault, xsDontScript);
}

void KPR_application_get_url(xsMachine *the)
{
	KprApplication self = xsGetHostData(xsThis);
	xsResult = xsString(self->url);
}

void KPR_application_get_width(xsMachine *the)
{
	KprContainer container = xsGetHostData(xsThis);
	SInt32 left = 0;
	SInt32 width = 0;
	SInt32 right = 0;
	while (container) {
		if ((container->coordinates.horizontal & kprWidth)) {
			width = container->coordinates.width - left - right;
			break;
		}
		else {
			left += container->coordinates.left;
			right += container->coordinates.right;
		}
		container = container->container;
	}
	xsResult = xsInteger(width);
}

void KPR_application_get_height(xsMachine *the)
{
	KprContainer container = xsGetHostData(xsThis);
	SInt32 top = 0;
	SInt32 height = 0;
	SInt32 bottom = 0;
	while (container) {
		if ((container->coordinates.vertical & kprHeight)) {
			height = container->coordinates.height - top - bottom;
			break;
		}
		else {
			top += container->coordinates.top;
			bottom += container->coordinates.bottom;
		}
		container = container->container;
	}
	xsResult = xsInteger(height);
}

void KPR_application_purge(xsMachine *the)
{
	KprApplication self = kprGetHostData(xsThis, this, application);
	FskInstrumentedItemSendMessageNormal(self, kprInstrumentedContentBeginCollect, self);
	xsCollectGarbage();
	FskInstrumentedItemSendMessageNormal(self, kprInstrumentedContentEndCollect, self);
	KprContextPurge(self, false);
}
