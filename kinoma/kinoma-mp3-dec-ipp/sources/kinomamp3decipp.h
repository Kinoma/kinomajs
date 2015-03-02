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
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"

#include "pthread.h"

#include "FskAudio.h"
	
#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
extern FskInstrumentedTypeRecord gkinomamp3decippTypeInstrumentation;
//FskInstrumentedSimpleType(kinomamp3decipp, kinomamp3decipp);
#define dlog(...)  do { FskInstrumentedTypePrintfDebug  (&gkinomamp3decippTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define dlog(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

FskErr mp3DecodeCanHandle(UInt32 format, const char *mime, Boolean *canHandle);
FskErr mp3DecodeNew(FskAudioDecompress deco, UInt32 format, const char *mime);
FskErr mp3DecodeDispose(void *state, FskAudioDecompress deco);
FskErr mp3DecodeDecompressFrames(void *state, FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *samplesSize);
FskErr mp3DecodeDiscontinuity(void *state, FskAudioDecompress deco);
//FskErr mp3DecodeSetSidebandData(void *state, UInt32 what, void *data, UInt32 dataSize);

extern FskMediaPropertyEntryRecord mp3DecodeProperties[];
	
#ifdef __cplusplus
}
#endif

