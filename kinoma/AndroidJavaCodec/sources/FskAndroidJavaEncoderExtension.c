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
#define __FSKANDROIDENCODER_PRIV__
#include "FskAndroidVideoEncoder.h"
#include "FskHardware.h"
#include "FskArch.h"

#define dlog(...)
FskMediaPropertyEntryRecord AndroidJavaVideoEncodeProperties[] = {
    {kFskMediaPropertyFormat,               kFskMediaPropertyTypeString,        AndroidJavaVideoEncodeGetFormat,                NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			AndroidJavaVideoEncodeGetSampleDescription,     NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		AndroidJavaVideoEncodeGetBitrate,               AndroidJavaVideoEncodeSetBitrate},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		AndroidJavaVideoEncodeGetScale,                 AndroidJavaVideoEncodeSetScale},
	{kFskMediaPropertyKeyFrameRate,			kFskMediaPropertyTypeInteger,		AndroidJavaVideoEncodeGetKeyFrameRate,          AndroidJavaVideoEncodeSetKeyFrameRate},
	{kFskMediaPropertyCompressionSettings,	kFskMediaPropertyTypeString,		AndroidJavaVideoEncodeGetCompressionSettings,   AndroidJavaVideoEncodeSetCompressionSettings},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,                                           NULL}
};

static FskImageCompressorRecord AndroidJavaVideoEncodeCompressor[] =
{
	{AndroidJavaVideoEncodeCanHandle, AndroidJavaVideoEncodeNew, AndroidJavaVideoEncodeDispose, AndroidJavaVideoEncodeCompressFrame, AndroidJavaVideoEncodeProperties},
	{NULL, NULL, NULL, NULL, NULL}
};

char *modelName = NULL;
FskExport(FskErr) FskAndroidJavaEncoder_fskLoad(FskLibrary library)
{
    FskImageCompressor walkerID;
    FskErr err = kFskErrNone;
	int android_version = 0;
	char *osVersion  = NULL;

	ANDROID_GET_VERSION(modelName, osVersion, android_version, err);
    BAIL_IF_ERR(err);

    if (android_version < ANDROID_JELLY) {
		BAIL(kFskErrUnimplemented);
	}

	for (walkerID = AndroidJavaVideoEncodeCompressor; NULL != walkerID->doCanHandle; walkerID++)
		FskImageCompressorInstall(walkerID);

bail:
	return kFskErrNone;
}

FskExport(FskErr) FskAndroidJavaEncoder_fskUnload(FskLibrary library)
{
	 FskImageCompressor walkerID;

	for (walkerID = AndroidJavaVideoEncodeCompressor; NULL != walkerID->doCanHandle; walkerID++)
		FskImageCompressorUninstall(walkerID);

	return kFskErrNone;
}
