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
#ifndef __ANDROIDAUDIODECODE__
#define __ANDROIDAUDIODECODE__

#include "Fsk.h"
#include "FskUtilities.h"
#include "FskAudio.h"

#ifndef FSK_JAVA_NAMESPACE
#define FSK_JAVA_NAMESPACE "com/kinoma/" APP_NAME
#endif


#ifdef __cplusplus
extern "C" {
#endif
	
FskErr AndroidJavaAudioDecodeCanHandle(UInt32 format, const char *mime, Boolean *canHandle);
FskErr AndroidJavaAudioDecodeNew(FskAudioDecompress deco, UInt32 format, const char *mime);
FskErr AndroidJavaAudioDecodeDispose(void *state, FskAudioDecompress deco);
FskErr AndroidJavaAudioDecodeDecompressFrames(void *state, FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *samplesSize);
FskErr AndroidJavaAudioDecodeDiscontinuity(void *state, FskAudioDecompress deco);
//FskErr AndroidJavaAudioDecodeSetSidebandData(void *state, UInt32 what, void *data, UInt32 dataSize);

extern FskMediaPropertyEntryRecord AndroidJavaAudioDecodeProperties[];

#ifdef __cplusplus
}
#endif

#endif
