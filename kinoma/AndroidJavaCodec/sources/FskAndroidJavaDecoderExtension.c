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
#define __FSKIMAGE_PRIV__
#include "FskAndroidVideoDecoder.h"
#include "FskAndroidAudioDecoder.h"
#include "FskHardware.h"
#include "FskArch.h"

#define dlog(...)
static FskImageDecompressorRecord AndroidJavaVideoDecodeDecompressor =
{
	AndroidJavaVideoDecodeCanHandle, AndroidJavaVideoDecodeNew, AndroidJavaVideoDecodeDispose, AndroidJavaVideoDecodeDecompressFrame, NULL, AndroidJavaVideoDecodeGetMetaData, AndroidJavaVideoDecodeProperties, NULL, AndroidJavaVideoDecodeFlush
};

static FskAudioDecompressorRecord AndroidJavaAudioDecodeDecompressor =
{
	AndroidJavaAudioDecodeCanHandle, AndroidJavaAudioDecodeNew, AndroidJavaAudioDecodeDispose, AndroidJavaAudioDecodeDecompressFrames,	AndroidJavaAudioDecodeDiscontinuity, AndroidJavaAudioDecodeProperties
};

#include "android_device_whitelist.c"

enum CODEC_TYPE {
    VIDEO_DECODER = 0,
    AUDIO_DECODER = 1,
    VIDEO_ENCODER = 2,
    AUDIO_ENCODER = 3,
};

int use_hardware_on_this_device( char *model_number, int android_version, int av_flags )
{
	int use_hardware = 0;
	int i;

	for( i = 0; i < (int)(sizeof(my_devices)/sizeof(DeviceInfo)); i++ )
	{
		DeviceInfo *d = &(my_devices[i]);
		
		if( !strcmp(d->model_number, model_number ) && d->android_version == android_version )
		{
            if (av_flags == VIDEO_DECODER) {
                return use_hardware = d->java_video_hw_dec;
            } else if (av_flags == AUDIO_DECODER) {
                return use_hardware = d->java_audio_hw_dec;
            }
		}
	}
	
	return use_hardware;
}

char *modelName  = NULL;
FskExport(FskErr) FskAndroidJavaDecoder_fskLoad(FskLibrary library)
{
	FskErr err = kFskErrNone;
	int android_version = 0;
	char *osVersion  = NULL;
    struct stat st;

	ANDROID_GET_VERSION(modelName, osVersion, android_version, err);
    BAIL_IF_ERR(err);
    
    if (android_version < ANDROID_JELLY) {
		BAIL(kFskErrUnimplemented);
	}

    if( use_hardware_on_this_device( modelName, android_version, VIDEO_DECODER ) || stat("/sdcard/jmcV", &st) == 0 )
    {
        FskImageDecompressorInstall(&AndroidJavaVideoDecodeDecompressor);
    }

    if( 0 /*use_hardware_on_this_device( modelName, android_version, AUDIO_DECODER ) || stat("/sdcard/jmcA", &st) == 0*/ )
    {
        FskAudioDecompressorInstall(&AndroidJavaAudioDecodeDecompressor);
    }

bail:
	return err;
}


FskExport(FskErr) FskAndroidJavaDecoder_fskUnload(FskLibrary library)
{
	FskImageDecompressorUninstall(&AndroidJavaVideoDecodeDecompressor);
	FskAudioDecompressorUninstall(&AndroidJavaAudioDecodeDecompressor);

	return kFskErrNone;
}
