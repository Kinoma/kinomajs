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
#ifndef __FSKAUDIOIN__
#define __FSKAUDIOIN__

#include "Fsk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FskAudioInStruct FskAudioInRecord, *FskAudioIn;

typedef FskErr (*FskAudioInCallback)(FskAudioIn audioIn, void *refCon, void *data, UInt32 dataSize);

FskAPI(FskErr) FskAudioInNew(FskAudioIn *audioIn, UInt32 inputID, FskAudioInCallback proc, void *refCon);
FskAPI(FskErr) FskAudioInDispose(FskAudioIn audioIn);

FskAPI(FskErr) FskAudioInSetFormat(FskAudioIn audioIn, UInt32 format, UInt16 numChannels, double sampleRate,
					unsigned char *formatInfo, UInt32 formatInfoSize);

FskAPI(FskErr) FskAudioInGetFormat(FskAudioIn audioIn, UInt32 *format, UInt16 *numChannels, double *sampleRate,
					unsigned char **formatInfo, UInt32 *formatInfoSize);


FskAPI(FskErr) FskAudioInStart(FskAudioIn audioIn);
FskAPI(FskErr) FskAudioInStop(FskAudioIn audioIn);

#ifdef __cplusplus
}
#endif

#endif
