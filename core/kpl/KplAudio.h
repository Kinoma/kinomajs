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
#ifndef __KPL_AUDIO_H__
#define __KPL_AUDIO_H__

#include "Kpl.h"


#include "FskErrors.h"
#include "KplProperty.h"

#ifdef __cplusplus
extern "C" {
#endif

KplDeclarePrivateType(KplAudio)

typedef void (*KplAudioDoneCallback)(KplAudio audio, void *refCon, void *dataRefCon, Boolean played);
typedef FskErr (*KplAudioMoreCallback)(KplAudio audio, void *refCon, SInt32 requestedSamples);

FskErr KplAudioNew(KplAudio *audio);
FskErr KplAudioDispose(KplAudio audio);

FskErr KplAudioSetFormat(KplAudio audio, const char *format, UInt32 channels, double sampleRate, const unsigned char *formatInfo, UInt32 formatInfoSize);
FskErr KplAudioGetFormat(KplAudio audio, const char **format, UInt32 *channels, double *sampleRate, const unsigned char **formatInfo, UInt32 *formatInfoSize);

FskErr KplAudioWrite(KplAudio audio, const char *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, const UInt32 *frameSizes);

FskErr KplAudioStart(KplAudio audio);
FskErr KplAudioStop(KplAudio audio);

FskErr KplAudioGetSamplesQueued(KplAudio audio, UInt32 *samplesQueued, UInt32 *targetQueueLength);

FskErr KplAudioGetProperty(KplAudio audio, UInt32 propertyID, KplProperty value);
FskErr KplAudioSetProperty(KplAudio audio, UInt32 propertyID, KplProperty value);

FskErr KplAudioSetDoneCallback(KplAudio audio, KplAudioDoneCallback cb, void *refCon);
FskErr KplAudioSetMoreCallback(KplAudio audio, KplAudioMoreCallback cb, void *refCon);

#ifdef __cplusplus
}
#endif

#endif // __KPL_AUDIO_H__
