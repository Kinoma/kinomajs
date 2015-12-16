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
#define __FSKAUDIOFILTER_PRIV__
#include "FskAudioFilter.h"
#include "FskExtensions.h"
#include "FskUtilities.h"

#if SUPPORT_INSTRUMENTATION
	#include <stdio.h>
	static Boolean doFormatMessageAudioFilter(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gAudioFilterTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"audiofilter",
		FskInstrumentationOffset(FskAudioFilterRecord),
		NULL,
		0,
		NULL,
		doFormatMessageAudioFilter
	};
#endif

FskErr FskAudioFilterNew(FskAudioFilter *filterOut, const char *filterType)
{
	FskErr err = kFskErrNone;
	FskAudioFilter filter = NULL;
	UInt32 i = 0;
	FskAudioFilterDispatch dispatch;

	while (true) {
		dispatch = (FskAudioFilterDispatch)FskExtensionGetByIndex(kFskExtensionAudioFilter, i++);
		if (NULL == dispatch)
			return kFskErrUnimplemented;

		if (dispatch->doCanHandle(filterType))
			break;
	}

	if (NULL == filterOut) goto bail;		// "can handle" request

	err = FskMemPtrNewClear(sizeof(FskAudioFilterRecord) + FskStrLen(filterType) + 1, &filter);
	BAIL_IF_ERR(err);

	filter->filterType = (char *)(filter + 1);
	FskStrCopy(filter->filterType, filterType);
	filter->dispatch = dispatch;

	FskInstrumentedItemNew(filter, filter->filterType, &gAudioFilterTypeInstrumentation);

	err = (filter->dispatch->doNew)(filter, &filter->state);
	BAIL_IF_ERR(err);

bail:
	if (kFskErrNone != err) {
		FskAudioFilterDispose(filter);
		filter = NULL;
	}

	if (NULL != filterOut)
		*filterOut = filter;

	return err;
}

FskErr FskAudioFilterDispose(FskAudioFilter filter)
{
	if (NULL == filter)
		goto bail;

	if (filter->dispatch->doDispose)
		(filter->dispatch->doDispose)(filter, filter->state);

	FskInstrumentedItemDispose(filter);

	FskMemPtrDispose(filter);

bail:
	return kFskErrNone;
}

FskErr FskAudioFilterSetInputFormat(FskAudioFilter filter, UInt32 format, UInt32 sampleRate, UInt32 channelCount)
{
	filter->inputFormat = format;
	filter->inputSampleRate = sampleRate;
	filter->inputChannelCount = channelCount;

	return kFskErrNone;
}

FskErr FskAudioFilterGetMaximumBufferSize(FskAudioFilter filter, UInt32 sampleCount, UInt32 *bufferSize)
{
	return (filter->dispatch->doGetMaximumBufferSize)(filter, filter->state, sampleCount, bufferSize);
}

FskErr FskAudioFilterStart(FskAudioFilter filter)
{
	FskInstrumentedItemSendMessageNormal(filter, kFskAudioFilterInstrStart, NULL);
	return filter->dispatch->doStart ? (filter->dispatch->doStart)(filter, filter->state) : kFskErrNone;
}

FskErr FskAudioFilterStop(FskAudioFilter filter)
{
	FskInstrumentedItemSendMessageNormal(filter, kFskAudioFilterInstrStop, NULL);
	return filter->dispatch->doStop ? (filter->dispatch->doStop)(filter, filter->state) : kFskErrNone;
}

FskErr FskAudioFilterProcessSamples(FskAudioFilter filter, void *input, UInt32 inputSampleCount,
											void *output, UInt32 *outputSampleCount, UInt32 *outputSize)
{
	FskErr err;

	err = (filter->dispatch->doProcessSamples)(filter, filter->state, input, inputSampleCount, output, outputSampleCount, outputSize);

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(filter, kFskInstrumentationLevelNormal)) {
		void *msgData[3];
		msgData[0] = (void *)inputSampleCount;
		msgData[1] = (void *)*outputSampleCount;
		msgData[2] = (void *)err;
		FskInstrumentedItemSendMessageForLevel(filter, kFskAudioFilterInstrProcess, msgData, kFskInstrumentationLevelNormal);
	}
#endif

	return err;
}

FskErr FskAudioFilterHasProperty(FskAudioFilter filter, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	return FskMediaHasProperty(filter->dispatch->properties, propertyID, get, set, dataType);
}

FskErr FskAudioFilterSetProperty(FskAudioFilter filter, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaSetProperty(filter->dispatch->properties, filter->state, filter, propertyID, property);
}

FskErr FskAudioFilterGetProperty(FskAudioFilter filter, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(filter->dispatch->properties, filter->state, filter, propertyID, property);
}

#if SUPPORT_INSTRUMENTATION

Boolean doFormatMessageAudioFilter(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kFskAudioFilterInstrStart:
			snprintf(buffer, bufferSize, "start");
			return true;

		case kFskAudioFilterInstrStop:
			snprintf(buffer, bufferSize, "stop");
			return true;

		case kFskAudioFilterInstrProcess: {
			void **md = (void **)msgData;
			if (kFskErrNone != (FskErr)md[2])
				snprintf(buffer, bufferSize, "process failed, err=%ld", (long)md[2]);
			else
				snprintf(buffer, bufferSize, "process samplesIn=%d, samplesOut=%d", (unsigned)md[0], (unsigned)md[1]);
			}
			return true;
	}

	return false;

}

#endif
