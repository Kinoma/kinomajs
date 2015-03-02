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
#ifndef __KINOMAMP4DECODE__
#define __KINOMAMP4DECODE__

#include "Fsk.h"
#include "FskBitmap.h"
#include "FskUtilities.h"
#include "FskImage.h"


#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
extern FskInstrumentedTypeRecord gkinomamp4decippTypeInstrumentation;
//FskInstrumentedSimpleType(kinomamp4decipp, kinomamp4decipp);
#define dlog(...)  do { FskInstrumentedTypePrintfDebug  (&gkinomamp4decippTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define dlog(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif
	
FskErr mp4DecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle);
FskErr mp4DecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension);
FskErr mp4DecodeDispose(void *stateIn, FskImageDecompress deco);
FskErr mp4DecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType);
FskErr mp4DecodeFlush(void *state, FskImageDecompress deco);
FskErr mp4DecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);

FskErr mp4DecodeSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
FskErr mp4DecodeSetRotation(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
FskErr mp4DecodeSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property );
FskErr mp4DecodeGetMaxFramesToQueue (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
	
extern FskMediaPropertyEntryRecord mp4DecodeProperties[];

#ifdef __cplusplus
}
#endif

#endif
