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
#include "kpr.h"
#include "kprShell.h"
#include "kprUtilities.h"

FskCondition gElementHostCondition = NULL;
FskMutex gElementHostMutex = NULL;
void* gElementHostResult = NULL;

FskExport(FskErr) kprElementHost_fskLoad(FskLibrary library)
{
	FskConditionNew(&gElementHostCondition);
	FskMutexNew(&gElementHostMutex, "kprElementHost");
	return kFskErrNone;
}

FskExport(FskErr) kprElementHost_fskUnload(FskLibrary library)
{
	FskMutexDispose(gElementHostMutex);
	FskConditionDispose(gElementHostCondition);
	return kFskErrNone;
}

typedef struct KprElementHostStruct KprElementHostRecord, *KprElementHost;
struct KprElementHostStruct {
	void* archive;
	xsMachine* machine;
	FskThread thread;
	Boolean quitting;
};

static void PINS_close(xsMachine* the);
static void PINS_configure(xsMachine* the);
static void PINS_invoke(xsMachine* the);
static void PINS_repeat(xsMachine* the);
static void console_log(xsMachine* the);

static void KPR_elementLoop(void* params)
{
	KprElementHost self = params;
	FskThreadInitializationComplete(FskThreadGetCurrent());
	xsBeginHost(self->machine);
	{
		xsVars(2);
		xsTry {
			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
			xsVar(1) = xsNewHostFunction(console_log, 0);
			xsSet(xsVar(0), xsID("log"), xsVar(1));
			xsSet(xsGlobal, xsID("console"), xsVar(0));
			
			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
			xsVar(1) = xsNewHostFunction(PINS_close, 0);
			xsSet(xsVar(0), xsID("close"), xsVar(1));
			xsVar(1) = xsNewHostFunction(PINS_configure, 1);
			xsSet(xsVar(0), xsID("configure"), xsVar(1));
			xsVar(1) = xsNewHostFunction(PINS_invoke, 3);
			xsSet(xsVar(0), xsID("invoke"), xsVar(1));
			xsVar(1) = xsNewHostFunction(PINS_repeat, 3);
			xsSet(xsVar(0), xsID("repeat"), xsVar(1));
			xsSet(xsGlobal, xsID("PINS"), xsVar(0));

			xsVar(0) = xsGet(xsGlobal, xsID("require"));
			xsCall1(xsVar(0), xsID("weak"), xsString("application"));
		}
		xsCatch {
			xsStringValue message = xsToString(xsException);
			fprintf(stderr, "### %s\n", message);
		}
	}
	xsEndHost(self->machine);
	while (!self->quitting)
		FskThreadRunloopCycle(-1);
	xsDeleteMachine(self->machine);
	fxUnmapArchive(self->archive);
	FskMemPtrDispose(self);

	return;
}

void KPR_elementHost(void* data)
{
	KprElementHost self = data;
	if (self) {
		self->quitting = true;
		// FskThreadJoin(self->thread);
	}
}

void KPR_ElementHost(xsMachine* the)
{
	xsCreation creation = {
		25*1024,	/* initial chunk size */
		2048,		/* incremental chunk size */
		50*1024/16,	/* initial heap count	-- will be calculated later */
		128,		/* incremental heap count	-- wasting 16 bytes / allocation */
		600,		/* stack count */
		2048+1024,	/* key count */
		97,		/* name modulo */
		127,		/* symbol modulo */
	};
	KprElementHost self;
	xsThrowIfFskErr(FskMemPtrNewClear(sizeof(KprElementHostRecord), &self));
	xsSetHostData(xsThis, self);
	self->archive = fxMapArchive(xsToString(xsArg(0)), NULL);
	xsThrowIfNULL(self->archive);
	self->machine = xsCreateMachine(&creation, self->archive, "element", NULL);
	xsThrowIfNULL(self->machine);
	xsThrowIfFskErr(FskThreadCreate(&(self->thread), KPR_elementLoop, kFskThreadFlagsWaitForInit | kFskThreadFlagsJoinable, self, "element"));
}

void KPR_elementHost_debugger(xsMachine* the)
{

}

void KPR_elementHost_launch(xsMachine* the)
{
}

void KPR_elementHost_purge(xsMachine* the)
{

}

void KPR_elementHost_quit(xsMachine* the)
{

}

void KPR_elementHost_wake(xsMachine* the)
{
	KprElementHost self = xsGetHostData(xsThis);
	FskThreadWake(self->thread);
}

typedef struct KprElementCallbackStruct KprElementCallbackRecord, *KprElementCallback;
struct KprElementCallbackStruct {
	void* path;
	void* object;
	void* result;
};


void PINS_close_callback()
{
	xsBeginHost(gShell->the);
	(void)xsCall0(xsGet(xsGlobal, xsID("PINS")), xsID("close"));
	xsEndHost(gShell->the);
}

void PINS_close(xsMachine* the)
{
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)PINS_close_callback, NULL, NULL, NULL, NULL);
}

void PINS_configure_callback(void* configuration)
{
	xsBeginHost(gShell->the);
	xsVars(1);
	xsVar(0) = xsDemarshallAlien(configuration);
	xsResult = xsCall1(xsGet(xsGlobal, xsID("PINS")), xsID("configure"), xsVar(0));
	FskMutexAcquire(gElementHostMutex);
	gElementHostResult = xsMarshallAlien(xsResult);
	FskConditionSignal(gElementHostCondition);
	FskMutexRelease(gElementHostMutex);
	xsEndHost(gShell->the);
}

void PINS_configure(xsMachine* the)
{
	void* configuration = xsMarshallAlien(xsArg(0));
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)PINS_configure_callback, configuration, NULL, NULL, NULL);
	FskMutexAcquire(gElementHostMutex);
	while (NULL == gElementHostResult)
		FskConditionWait(gElementHostCondition, gElementHostMutex);
	xsResult = xsDemarshallAlien(gElementHostResult);
	FskMemPtrDisposeAt(&gElementHostResult);
	FskMutexRelease(gElementHostMutex);
	xsCallFunction1(xsArg(1), xsUndefined, xsResult);
}

void PINS_invoke_callback(void* path, void* object)
{
	xsBeginHost(gShell->the);
	xsVars(2);
	xsVar(0) = xsDemarshallAlien(path);
	xsVar(1) = xsDemarshallAlien(object);
	xsResult = xsCall2(xsGet(xsGlobal, xsID("PINS")), xsID("invoke"), xsVar(0), xsVar(1));
	FskMutexAcquire(gElementHostMutex);
	gElementHostResult = xsMarshallAlien(xsResult);
	FskConditionSignal(gElementHostCondition);
	FskMutexRelease(gElementHostMutex);
	xsEndHost(gShell->the);
}

void PINS_invoke(xsMachine* the)
{
	void* path = xsMarshallAlien(xsArg(0));
	void* object = xsMarshallAlien(xsArg(1));
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)PINS_invoke_callback, path, object, NULL, NULL);
	FskMutexAcquire(gElementHostMutex);
	while (NULL == gElementHostResult)
		FskConditionWait(gElementHostCondition, gElementHostMutex);
	xsResult = xsDemarshallAlien(gElementHostResult);
	FskMemPtrDisposeAt(&gElementHostResult);
	FskMutexRelease(gElementHostMutex);
	xsCallFunction1(xsArg(2), xsUndefined, xsResult);
}

void PINS_repeat_callback(void* path, void* object)
{
	xsBeginHost(gShell->the);
	xsVars(2);
	xsVar(0) = xsDemarshallAlien(path);
	xsVar(1) = xsDemarshallAlien(object);
	xsResult = xsCall2(xsGet(xsGlobal, xsID("PINS")), xsID("repeat"), xsVar(0), xsVar(1));
	FskMutexAcquire(gElementHostMutex);
	gElementHostResult = xsMarshallAlien(xsResult);
	FskConditionSignal(gElementHostCondition);
	FskMutexRelease(gElementHostMutex);
	xsEndHost(gShell->the);
}

void PINS_repeat(xsMachine* the)
{
	void* path = xsMarshallAlien(xsArg(0));
	void* object = xsMarshallAlien(xsArg(1));
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)PINS_repeat_callback, path, object, NULL, NULL);
	FskMutexAcquire(gElementHostMutex);
	while (NULL == gElementHostResult)
		FskConditionWait(gElementHostCondition, gElementHostMutex);
	xsResult = xsDemarshallAlien(gElementHostResult);
	FskMemPtrDisposeAt(&gElementHostResult);
	FskMutexRelease(gElementHostMutex);
	if ((xsTypeOf(xsResult) != xsUndefinedType))
		xsCallFunction1(xsArg(2), xsUndefined, xsResult);
}

static int console_log_depth = 0;
void console_log(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	xsBooleanValue space = 0;
	xsVars(4);
	console_log_depth++;
	for (i = 0; i < c; i++) {
		if (space)
			fprintf(stderr,  " ");
		else
			space = 1;
		switch (xsTypeOf(xsArg(i))) {
		case xsUndefinedType:
		case xsNullType:
		case xsBooleanType:
		case xsIntegerType:
		case xsNumberType:
		case xsSymbolType:
			fprintf(stderr,  "%s", xsToString(xsArg(i)));
			break;
		case xsStringType:
		case xsStringXType:
			if ((console_log_depth == 1) && (i == 0))
				fprintf(stderr,  "%s", xsToString(xsArg(i)));
			else
				fprintf(stderr,  "'%s'", xsToString(xsArg(i)));
			break;
		case xsReferenceType:
			if (console_log_depth < 3) {
				xsBooleanValue comma = 0;
				if (xsHas(xsArg(i), xsID("length"))) {
					xsIntegerValue length = xsToInteger(xsGet(xsArg(i), xsID("length"))), index;
					fprintf(stderr,  "[");
					for (index = 0; index < length; index++) {
						xsVar(1) = xsGet(xsArg(i), (xsIndex)index);
						if (comma)
							fprintf(stderr,  ",");
						else
							comma = 1;
						fprintf(stderr,  " ");
						fxPush(xsVar(1));
						fxPushCount(the, 1);
						fxPush(xsThis);
						fxPush(xsFunction);
						fxCall(the);
					}
					fprintf(stderr,  " ]");
				}
				else {
					fprintf(stderr,  "{");
					xsVar(0) = xsEnumerate(xsArg(i));
					for (;;) {
						xsVar(1) = xsCall0(xsVar(0), xsID("next"));
						if (xsTest(xsGet(xsVar(1), xsID("done"))))
							break;
						xsVar(2) = xsGet(xsVar(1), xsID("value"));
						xsVar(3) = xsGetAt(xsArg(i), xsVar(2));
						if (comma)
							fprintf(stderr,  ",");
						else
							comma = 1;
						fprintf(stderr,  " %s: ", xsToString(xsVar(2)));
						fxPush(xsVar(3));
						fxPushCount(the, 1);
						fxPush(xsThis);
						fxPush(xsFunction);
						fxCall(the);
					}
					fprintf(stderr,  " }");
				}
			}
			else
				fprintf(stderr,  "%s", xsToString(xsArg(i)));
			break;
		}
	}
	console_log_depth--;
	if (!console_log_depth)
		fprintf(stderr,  "\n");
}


