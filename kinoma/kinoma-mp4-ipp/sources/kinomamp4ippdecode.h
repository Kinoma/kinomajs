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
#ifndef __MP4DECODE__
#define __MP4DECODE__

#ifdef __cplusplus
extern "C" {
#endif

#include "Fsk.h"
#include "FskBitmap.h"
#include "FskUtilities.h"
#include "FskImage.h"

#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
	extern FskInstrumentedTypeRecord gkinomamp4ippTypeInstrumentation;
	//FskInstrumentedSimpleType(kinomamp4ipp, kinomamp4ipp);
#define dlog(...)  do { FskInstrumentedTypePrintfDebug  (&gkinomamp4ippTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define dlog(...)
#endif
    
FskErr mp4DecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle);
FskErr mp4DecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension);
FskErr mp4DecodeDispose(void *stateIn, FskImageDecompress deco);
FskErr mp4DecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType);
FskErr mp4DecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);

FskErr mp4DecodeGetDimension(const void *esds, UInt32 esds_size, UInt32 *width, UInt32 *height );

extern FskMediaPropertyEntryRecord mp4DecodeProperties[];

#ifdef __cplusplus
}
#endif

#endif

