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
#ifndef __FSKCAMERAANDROID__
#define __FSKCAMERAANDROID__

#include "FskMediaReader.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	
	Boolean cameraAndroidCanHandle(const char *mimeType);
	FskErr cameraAndroidNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
	FskErr cameraAndroidDispose(FskMediaReader reader, void *readerState);
	FskErr cameraAndroidGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
	FskErr cameraAndroidStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
	FskErr cameraAndroidStop(FskMediaReader reader, void *readerState);
	FskErr cameraAndroidExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
	FskErr cameraAndroidGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
	FskErr cameraAndroidSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);
	FskMediaPropertyEntry	cameraAndroidGetProperties();
	
	extern FskMediaPropertyEntryRecord cameraAndroidProperties[];
	
#ifdef __cplusplus
}
#endif /* __cplusplus */	

#endif
