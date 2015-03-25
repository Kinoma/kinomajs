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
#define __FSKEVENT_PRIV__
#include "FskEvent.h"

#include "FskEvent.h"
#include "FskMain.h"
#include "FskList.h"
#include "FskMain.h"
#include "FskUtilities.h"

static FskEventParameter findEventParameter(FskEvent event, FskEventParameterEnum parameterCode);

#if SUPPORT_INSTRUMENTATION
	static FskInstrumentedTypeRecord gEventTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"event",
		FskInstrumentationOffset(FskEventRecord),
		NULL
	};

	char *getEventName(UInt32 e);
#endif

FskErr FskEventNew(FskEvent *eventOut, FskEventCodeEnum eventCode, FskTime atTime, FskEventModifierEnum modifiers)
{
	FskErr err;
	FskEvent event;

	err = FskMemPtrNewClear(sizeof(FskEventRecord), &event);
	if (err) {
		FskMainDoQuit(err);
		goto bail;
	}

	event->eventCode = eventCode;

	if (NULL != atTime)
		event->atTime = *atTime;
	else
		FskTimeGetNow(&event->atTime);

	if (kFskEventModifierNotSet != modifiers)
		event->modifiers = modifiers;
	else {
		// @@ fill this out from current hardware state
		event->modifiers = 0;
	}

	FskInstrumentedItemNew(event, getEventName(eventCode), &gEventTypeInstrumentation);

bail:
	if (kFskErrNone != err) {
		FskEventDispose(event);
		event = NULL;
	}

	*eventOut = event;

	return err;
}

FskErr FskEventDispose(FskEvent event)
{
	if (NULL != event) {
		FskInstrumentedItemDispose(event);
		while (NULL != event->parameters) {
			FskEventParameter toRemove = (FskEventParameter)FskListRemoveFirst((FskList*)(void*)&event->parameters);
			FskMemPtrDispose(toRemove);
		}
		FskMemPtrDispose(event);
	}

	return kFskErrNone;
}

FskErr FskEventCopy(FskEvent event, FskEvent *eventOut)
{
	FskErr err;
	FskEvent newEvent = NULL;
	FskEventParameter param;

	err = FskEventNew(&newEvent, event->eventCode, &event->atTime, event->modifiers);
	BAIL_IF_ERR(err);

	for (param = event->parameters; NULL != param; param = param->next) {
		err = FskEventParameterAdd(newEvent, param->parameterCode, param->parameterSize, param->parameterData);
		BAIL_IF_ERR(err);
	}

bail:
	if (err) {
		FskEventDispose(newEvent);
		newEvent = NULL;
	}
	*eventOut = newEvent;

	return err;
}

FskErr FskEventParameterAdd(FskEvent event, FskEventParameterEnum parameterCode, UInt32 parameterSize, const void *parameterData)
{
	FskErr err;
	FskEventParameter param;

	// make sure it isn't already there
	if (NULL != findEventParameter(event, parameterCode))
		return kFskErrDuplicateElement;

	// create the parameter
	err = FskMemPtrNewClear(sizeof(FskEventParameterRecord) + parameterSize, &param);
	BAIL_IF_ERR(err);

	param->parameterCode = parameterCode;
	param->parameterSize = parameterSize;
	FskMemMove(&param->parameterData, parameterData, parameterSize);

	FskListPrepend((FskList*)(void*)&event->parameters, param);

bail:
	return err;
}

FskErr FskEventParameterRemove(FskEvent event, FskEventParameterEnum parameterCode)
{
	FskEventParameter param;

	param = findEventParameter(event, parameterCode);
	if (NULL == param)
		return kFskErrUnknownElement;

	FskListRemove((FskList*)(void*)&event->parameters, param);

	// toss it
	FskMemPtrDispose(param);

	return kFskErrNone;
}

FskErr FskEventParameterGet(FskEvent event, FskEventParameterEnum parameterCode, void *parameterData)
{
	FskEventParameter param;

	switch (parameterCode) {
		case kFskEventParameterTime:
			*(FskTime)parameterData = event->atTime;
			break;

		case kFskEventParameterModifiers:
			*(UInt32 *)parameterData = event->modifiers;
			break;

		default:
			param = findEventParameter(event, parameterCode);
			if (NULL == param)
				return kFskErrUnknownElement;
			FskMemMove(parameterData, param->parameterData, param->parameterSize);
			break;
	}

	return kFskErrNone;
}

FskErr FskEventParameterGetSize(FskEvent event, FskEventParameterEnum parameterCode, UInt32 *parameterSize)
{
	FskEventParameter param;

	*parameterSize = 0;
	param = findEventParameter(event, parameterCode);
	if (NULL == param)
		return kFskErrUnknownElement;
	*parameterSize = param->parameterSize;

	return kFskErrNone;
}

FskEventParameter findEventParameter(FskEvent event, FskEventParameterEnum parameterCode)
{
	FskEventParameter walker = event->parameters;
	while (walker) {
		if (walker->parameterCode == parameterCode)
			return walker;
		walker = walker->next;
	}

	return NULL;
}

#if SUPPORT_INSTRUMENTATION

typedef struct {
	UInt32		startEvent;
	UInt32		endEvent;
	char		**name;
} FskEventGroupNameRecord;

char *gEventMouseNames[] = {
	"mouse down",
	"mouse double click",
	"mouse up",
	"mouse moved",
	"mouse still down",
	"mouse right mouse down",
	"mouse right mouse up",
	"mouse right still down",
	"mouse leave",
	"mouse wheel"
};

char *gEventKeyNames[] = {
	"key down",
	"key up",
	"button"
};

char *gEventMediaPlayerNames[] = {
	"media player idle",
	"media player state change",
	"media player bounds change",
	"media player edit change",
	"media player initialize",
	"media player terminate",
	"media player end",
	"media player resource lost"
};

char *gEventWindowNames[] = {
	"window initialize",
	"window close",
	"window update",
	"window resize",
	"window screen updated",
	"window drag enter",
	"window drag over",
	"window drag drop",
	"window drag leave",
	"window activated",
	"window deactivated",
	"window open files",
	"window clipboard changed",
	"window before update",
	"window transition"
};

char *gEventMenuNames[] = {
	"menu command",
	"menu enable items",
	"menu status"
};

char *gEventThreadNames[] = {
	"thread callback"
};

char *gEventSystemNames[] = {
	"system quit"
};

// must be ordered by event id
FskEventGroupNameRecord gEventNames[] = {
	{kFskEventMouseDown, kFskEventMouseWheel, gEventMouseNames},
	{kFskEventKeyDown, kFskEventButton, gEventKeyNames},
	{kFskEventWindowInitialize, kFskEventWindowTransition, gEventWindowNames},
	{kFskEventMenuCommand, kFskEventMenuStatus, gEventMenuNames},
	{kFskEventThreadCallback, kFskEventThreadCallback, gEventThreadNames},
	{kFskEventMediaPlayerIdle, kFskEventMediaPlayerResourceLost, gEventMediaPlayerNames},
	{kFskEventSystemQuitRequest, kFskEventSystemQuitRequest, gEventSystemNames},
	{kFskUInt32Max, kFskUInt32Max, NULL}
};

char *getEventName(UInt32 e)
{
	FskEventGroupNameRecord *n = gEventNames;
	for (n = gEventNames; e >= n->startEvent; n++) {
		if ((n->startEvent <= e) && (e <= n->endEvent))
			return n->name[e - n->startEvent];
	}
	return NULL;
}

#endif
