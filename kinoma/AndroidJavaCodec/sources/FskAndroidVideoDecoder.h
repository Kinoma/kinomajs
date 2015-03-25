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
#ifndef __ANDROIDVIDEODECODE__
#define __ANDROIDVIDEODECODE__

#include "Fsk.h"
#include "FskBitmap.h"
#include "FskUtilities.h"
#include "FskImage.h"
#include "avcC.h"


#ifdef __cplusplus
extern "C" {
#endif
	
FskErr AndroidJavaVideoDecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle);
FskErr AndroidJavaVideoDecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension);
FskErr AndroidJavaVideoDecodeDispose(void *stateIn, FskImageDecompress deco);
FskErr AndroidJavaVideoDecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType);
FskErr AndroidJavaVideoDecodeFlush(void *state, FskImageDecompress deco);
FskErr AndroidJavaVideoDecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);

FskErr AndroidJavaVideoDecodeSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
FskErr AndroidJavaVideoDecodeSetRotation(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
FskErr AndroidJavaVideoDecodeSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property );
FskErr AndroidJavaVideoDecodeGetMaxFramesToQueue (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
	
extern FskMediaPropertyEntryRecord AndroidJavaVideoDecodeProperties[];

#ifdef __cplusplus
}
#endif

#endif
