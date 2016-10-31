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
#define __FSKECMASCRIPT_PRIV__

#include "FskTime.h"
#include "FskECMAScript.h"

typedef struct {
	xsMachine* the;
} FskECMAScriptRecord, *FskECMAScript;

#include "FskMain.h"

/*
	Timer
*/
static void xs_Timer_callback(struct FskTimeCallBackRecord *callback, const FskTime time, void *param);

typedef struct {
	FskTimeCallBack	timer;
	xsSlot			obj;
	FskECMAScript	vm;
	Boolean			repeating;
	UInt32			interval;
	FskTimeRecord	scheduleTime;
} xsNativeTimerRecord, *xsNativeTimer;

void xs_Timer(xsMachine *the)
{
	FskErr err;
	xsNativeTimer nt;

	err = FskMemPtrNew(sizeof(xsNativeTimerRecord), &nt);
	if (err) {
		FskMainDoQuit(err);
		xsResult = xsNew1(xsGet(xsGet(xsGlobal, xsID("Fsk")), xsID("Error")), xsID("Native"), xsInteger(err));
		xsThrow(xsResult);
	}
	xsSetHostData(xsThis, nt);
	nt->obj = xsThis;
	nt->vm = (FskECMAScript)xsGetContext(the);

	if ((0 == xsToInteger(xsArgc)) || !xsTest(xsArg(0)))
		FskTimeCallbackNew(&nt->timer);
	else
		FskTimeCallbackUINew(&nt->timer);
}

void xs_Timer_destuctor(void *hostData)
{
	if (hostData) {
		xsNativeTimer nt = (xsNativeTimer)hostData;
		FskTimeCallbackDispose(nt->timer);
		FskMemPtrDispose(nt);
	}
}

void xs_Timer_close(xsMachine *the)
{
	xsNativeTimer nt = (xsNativeTimer)xsGetHostData(xsThis);
	FskTimeCallbackDispose(nt->timer);
	nt->timer = NULL;
}

void xs_Timer_schedule(xsMachine *the)
{
	xsNativeTimer nt = (xsNativeTimer)xsGetHostData(xsThis);
	FskTimeRecord when;

	if (NULL == nt->timer) return;

	nt->repeating = false;
	FskTimeGetNow(&when);
	nt->scheduleTime = when;
	FskTimeAddMS(&when, xsToInteger(xsArg(0)));

	FskTimeCallbackSet(nt->timer, &when, xs_Timer_callback, nt);
}

void xs_Timer_scheduleRepeating(xsMachine *the)
{
	xsNativeTimer nt = (xsNativeTimer)xsGetHostData(xsThis);
	FskTimeRecord when;

	if (NULL == nt->timer) return;

	nt->repeating = true;
	nt->interval = xsToInteger(xsArg(0));
	FskTimeGetNow(&when);
	nt->scheduleTime = when;
	if (nt->interval) {
		FskTimeAddMS(&when, nt->interval);
		FskTimeCallbackSet(nt->timer, &when, xs_Timer_callback, nt);
	}
	else
		FskTimeCallbackScheduleNextRun(nt->timer, xs_Timer_callback, nt);
}

void xs_Timer_cancel(xsMachine *the)
{
	xsNativeTimer nt = (xsNativeTimer)xsGetHostData(xsThis);
	if (NULL == nt->timer) return;
	FskTimeCallbackRemove(nt->timer);
	nt->repeating = false;
}

void xs_Timer_callback(struct FskTimeCallBackRecord *callback, const FskTime time, void *param)
{
	xsNativeTimer nt = (xsNativeTimer)param;
	xsMachine *the = nt->vm->the;
	FskTimeRecord delta;

	xsBeginHost(the); {
		xsTry {
			FskTimeGetNow(&delta);
			FskTimeSub(&nt->scheduleTime, &delta);
			xsCall1_noResult(nt->obj, xsID("onCallback"), xsInteger(FskTimeInMS(&delta)));
		}
		xsCatch {		// try/catch so that if callback throws an error we will still do the repeat
		}

		if (nt->timer && nt->repeating) {
			if (nt->interval) {
				FskTimeRecord when = *time;		// this will give us a non-drifting clock
				FskTimeAddMS(&when, nt->interval);
				FskTimeCallbackSet(nt->timer, &when, xs_Timer_callback, nt);
			}
			else
				FskTimeCallbackScheduleNextRun(nt->timer, xs_Timer_callback, nt);
		}
	}
	xsEndHost(the);
}
