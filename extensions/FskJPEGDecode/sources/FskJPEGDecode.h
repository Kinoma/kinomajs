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
#ifndef __FSKJPEGDECODE__
#define __FSKJPEGDECODE__

#include "FskImage.h"

FskErr jpegDecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle);
FskErr jpegDecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension);
FskErr jpegDecodeDispose(void *stateIn, FskImageDecompress deco);
FskErr jpegDecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType);
FskErr jpegDecodeSetData(void *stateIn, FskImageDecompress deco, void *data, UInt32 dataSize);
FskErr jpegDecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
FskErr jpegDecodeSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

#endif
