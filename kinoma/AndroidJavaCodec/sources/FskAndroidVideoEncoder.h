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
#ifndef __ANDROIDVIDEOENCODE__
#define __ANDROIDVIDEOENCODE__

#include "Fsk.h"
#include "FskBitmap.h"
#include "FskUtilities.h"
#include "FskImage.h"
#include "avcC.h"


#ifdef __cplusplus
extern "C" {
#endif

FskErr AndroidJavaVideoEncodeCanHandle(UInt32 format, const char *mime, Boolean *canHandle);
FskErr AndroidJavaVideoEncodeNew(FskImageCompress deco);
FskErr AndroidJavaVideoEncodeDispose(void *stateIn, FskImageCompress comp);
FskErr AndroidJavaVideoEncodeCompressFrame(void *state, FskImageCompress comp, FskBitmap bits, const void **data, UInt32 *dataSize, UInt32 *frameDuration, UInt32 *compositionTimeOffset, UInt32 *frameTypeOut);

FskErr AndroidJavaVideoEncodeGetFormat(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskErr AndroidJavaVideoEncodeGetSampleDescription(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskErr AndroidJavaVideoEncodeSetBitrate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskErr AndroidJavaVideoEncodeGetBitrate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskErr AndroidJavaVideoEncodeSetScale(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskErr AndroidJavaVideoEncodeGetScale(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskErr AndroidJavaVideoEncodeSetKeyFrameRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskErr AndroidJavaVideoEncodeGetKeyFrameRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskErr AndroidJavaVideoEncodeSetCompressionSettings(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskErr AndroidJavaVideoEncodeGetCompressionSettings(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

extern char *modelName;
extern FskMediaPropertyEntryRecord AndroidJavaVideoEncodeProperties[];

#ifdef __cplusplus
}
#endif

#endif

