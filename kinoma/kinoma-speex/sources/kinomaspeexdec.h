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
#include "FskAudio.h"

#ifdef __cplusplus
extern "C" {
#endif

	FskErr speexDecodeCanHandle(UInt32 format, const char *mime, Boolean *canHandle);
FskErr speexDecodeNew(FskAudioDecompress deco, UInt32 format, const char *mime);
FskErr speexDecodeDispose(void *state, FskAudioDecompress deco);
FskErr speexDecodeDecompressFrames(void *state, FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *samplesSize);
FskErr speexDecodeDiscontinuity(void *state, FskAudioDecompress deco);

extern FskMediaPropertyEntryRecord speexDecodeProperties[];

#ifdef __cplusplus
}
#endif

