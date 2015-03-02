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
#ifndef __FSKAUDIOFILTER__
#define __FSKAUDIOFILTER__

#include "FskMedia.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FskAudioFilterRecord FskAudioFilterRecord;
typedef FskAudioFilterRecord *FskAudioFilter;

typedef struct {
	Boolean		(*doCanHandle)(const char *filterType);

	FskErr		(*doNew)(FskAudioFilter filter, void **filterState);
	FskErr		(*doDispose)(FskAudioFilter filter, void *filterState);

	FskErr		(*doGetMaximumBufferSize)(FskAudioFilter filter, void *filterState, UInt32 sampleCount, UInt32 *bufferSize);

	FskErr		(*doStart)(FskAudioFilter filter, void *filterState);
	FskErr		(*doStop)(FskAudioFilter filter, void *filterState);
	FskErr		(*doProcessSamples)(FskAudioFilter filter, void *filterState, void *input, UInt32 inputSampleCount, void *output, UInt32 *outputSampleCount, UInt32 *outputSize);

	FskMediaPropertyEntry	properties;
} FskAudioFilterDispatchRecord, *FskAudioFilterDispatch;

#if defined(__FSKAUDIOFILTER_PRIV__) || SUPPORT_INSTRUMENTATION
	struct FskAudioFilterRecord {
		char					*filterType;

		UInt32					inputFormat;
		UInt32					inputSampleRate;
		UInt32					inputChannelCount;

		FskAudioFilterDispatch	dispatch;
		void					*state;

		FskInstrumentedItemDeclaration
	};
#endif

// instance
FskAPI(FskErr) FskAudioFilterNew(FskAudioFilter *filter, const char *filterType);
FskAPI(FskErr) FskAudioFilterDispose(FskAudioFilter filter);

// conversion
FskAPI(FskErr) FskAudioFilterSetInputFormat(FskAudioFilter filter, UInt32 format, UInt32 sampleRate, UInt32 channelCount);

FskAPI(FskErr) FskAudioFilterGetMaximumBufferSize(FskAudioFilter filter, UInt32 sampleCount, UInt32 *bufferSize);

FskAPI(FskErr) FskAudioFilterStart(FskAudioFilter filter);
FskAPI(FskErr) FskAudioFilterStop(FskAudioFilter filter);

FskAPI(FskErr) FskAudioFilterProcessSamples(FskAudioFilter filter, void *input, UInt32 inputSampleCount,
											void *output, UInt32 *outputSampleCount, UInt32 *outputSize);

// properties
FskAPI(FskErr) FskAudioFilterHasProperty(FskAudioFilter filter, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskAudioFilterSetProperty(FskAudioFilter filter, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskAudioFilterGetProperty(FskAudioFilter filter, UInt32 propertyID, FskMediaPropertyValue property);

#if SUPPORT_INSTRUMENTATION

enum {
	kFskAudioFilterInstrStart = kFskInstrumentedItemFirstCustomMessage,
	kFskAudioFilterInstrStop,
	kFskAudioFilterInstrProcess
};

#endif

#ifdef __cplusplus
}
#endif

#endif
