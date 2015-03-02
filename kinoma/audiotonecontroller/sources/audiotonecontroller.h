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
#ifndef __TONECONTROLLER_H__
#define __TONECONTROLLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define __FSKAUDIOFILTER_PRIV__

#include "Fsk.h"
#include "FskAudioFilter.h"
#include "FskAudio.h"
#include "FskExtensions.h"
#include "FskUtilities.h"

Boolean	tonecontrollerCanHandle(const char *filterType);
FskErr tonecontrollerNew(FskAudioFilter filter, void **filterState);
FskErr tonecontrollerDispose(FskAudioFilter filter, void *stateIn);
FskErr tonecontrollerGetMaximumBufferSize(FskAudioFilter filter, void *stateIn, UInt32 sampleCount, UInt32 *bufferSize);
FskErr tonecontrollerStart(FskAudioFilter filter, void *stateIn);
FskErr tonecontrollerStop(FskAudioFilter filter, void *stateIn);
FskErr tonecontrollerProcessSamples(FskAudioFilter filter, void *stateIn, void *input, UInt32 inputSampleCount, void *output, UInt32 *outputSampleCount, UInt32 *outputSize);
FskErr tonecontrollerSetSettings(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

#ifdef __cplusplus
}
#endif

#endif // __TONECONTROLLER_H__
