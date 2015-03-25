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
#define __FSKAUDIOCODEC_PRIV__

#include "FskArch.h"
#include "FskEndian.h"
#include "FskAudio.h"
#include "kinomaspeexdec.h"

#include "speex.h"
#include "speex_header.h"

//#if TARGET_OS_ANDROID
void kinoma_speex_init(int mode);
//#endif

typedef struct 
{
	int			can_change_samplerate;
	int			native_samplerate;
	int			output_samplerate;
	int			channel_total;
	int			speex_frame_size;
	int			wb_mode;
	SpeexBits	bits;
	void		*dec;
} KinomaSpeexDecoderRecord, *KinomaSpeexDecoder;

FskErr speexDecodeCanHandle(UInt32 format, const char *formatStr, Boolean *canHandle)
{
	*canHandle = (kFskAudioFormatSPEEXNB == format)							||
				 //(kFskAudioFormatSPEEXWB == format)						||
				 (0 == FskStrCompare(formatStr, "format:speex"))			|| 
				 (0 == FskStrCompare(formatStr, "x-audio-codec/speex"))		||
				 (0 == FskStrCompare(formatStr, "x-audio-codec/speex-nb"))	||
				 (0 == FskStrCompare(formatStr, "x-audio-codec/speex-wb"))
				 ;
	return kFskErrNone;
}

#define kDefaultSpeexFrameSampleTotal 160
FskErr speexDecodeNew(FskAudioDecompress deco, UInt32 format, const char *mime)
{
	KinomaSpeexDecoder state;
	FskErr		 err = kFskErrNone;

	err = FskMemPtrNewClear(sizeof(KinomaSpeexDecoderRecord), (FskMemPtr *)&state);
	if (kFskErrNone != err) 
		goto bail;

	deco->state = state;

	state->can_change_samplerate=0;
	state->native_samplerate	= 16000;
	state->output_samplerate	= 16000;
	state->channel_total		= 1;
	state->speex_frame_size		= kDefaultSpeexFrameSampleTotal*state->channel_total*2;
	state->wb_mode				= (0 != FskStrCompare(mime, "x-audio-codec/speex-nb"));
	state->dec					= NULL;

	deco->outputFormat			= kFskAudioFormatPCM16BitLittleEndian;
	deco->outputChannelCount	= 1;//deco->inputChannelCount;

bail:
	return err;
}


FskErr speexDecodeDispose(void *stateIn, FskAudioDecompress deco)
{
	KinomaSpeexDecoder state = (KinomaSpeexDecoder)stateIn;

	if (NULL != state) 
	{
		if (NULL != state->dec) 
		{
			speex_bits_destroy(&state->bits);
			speex_decoder_destroy(state->dec);
		}
		FskMemPtrDisposeAt(&deco->state);
	}

	return kFskErrNone;
}

#define kMaxWavSize    (640 * 8)
FskErr speexDecodeDecompressFrames(void *stateIn, FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *samplesSize)
{
	KinomaSpeexDecoder	state = (KinomaSpeexDecoder)stateIn;
	unsigned char 	*thisFrameData 	= (unsigned char *)data;
	unsigned char	*wav_data		= NULL;
	unsigned char 	*thisWavData	= NULL;
	int 			totalBytes		= 0;
	UInt32			i;
	FskErr 			err = kFskErrNone;

	if (NULL == state->dec) 
	{
		int speex_frame_sample_total;

		kinoma_speex_init(FSK_ARCH_AUTO);

		speex_bits_init(&state->bits);
		if( state->wb_mode )
			state->dec = speex_decoder_init(&speex_wb_mode);
		else
			state->dec = speex_decoder_init(&speex_nb_mode);

		speex_decoder_ctl(state->dec, SPEEX_GET_FRAME_SIZE, &speex_frame_sample_total);
		state->speex_frame_size = speex_frame_sample_total * state->channel_total * 2;
	}

	err = FskMemPtrNew(kMaxWavSize * frameCount, (FskMemPtr *)&wav_data);
	if (err) return err;
	
	thisWavData = wav_data;
	
	for( i = 0; i < frameCount; i++ )
	{
		long thisFrameSize = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[i];
		
		speex_bits_read_from(&state->bits, (char *)thisFrameData, thisFrameSize);
		thisFrameData += thisFrameSize;

		while(1)
		{
			int ret = 0;

			ret = speex_decode_int(state->dec, &state->bits, (spx_int16_t*)thisWavData);
			if( ret != 0 )
				break;

			totalBytes  += state->speex_frame_size;
			thisWavData += state->speex_frame_size;
		}
	}

	if( totalBytes == 0 && wav_data != NULL )
	{
		FskMemPtrDisposeAt( (void **)&wav_data );
		wav_data = NULL;
	}

	*samplesSize = totalBytes;
	*samples 	 = (void *)wav_data;

	return err;
}


FskErr speexDecodeDiscontinuity(void *stateIn, FskAudioDecompress deco)
{
	KinomaSpeexDecoder state = (KinomaSpeexDecoder)stateIn;

	if( state && state->dec )
	{
		//umcRes = state->dec->Reset();
	}
	
	return kFskErrNone;
}

#if 0 // Not used at all?
static FskErr speexDecodeGetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	KinomaSpeexDecoder state = (KinomaSpeexDecoder)stateIn;
	UInt32 sampleRate = 0;
	
	if (state && state->output_samplerate != 0)
	{
		sampleRate = state->output_samplerate;
	}
	else
	{
		err = kFskErrOperationFailed;
		goto bail;
	}

	property->value.integer = sampleRate;
	property->type = kFskMediaPropertyTypeInteger;

bail:
	return err;
}

static FskErr speexDecodeGetChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	KinomaSpeexDecoder state = (KinomaSpeexDecoder)stateIn;
	UInt32 channelCount = 0;

	if (state && state->channel_total != 0)
	{
		channelCount = state->channel_total;
	}
	else
	{
		err = kFskErrOperationFailed;
		goto bail;
	}

	property->value.integer = channelCount;
	property->type = kFskMediaPropertyTypeInteger;

bail:
	return err;
}

static FskErr speexDecodeGetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	KinomaSpeexDecoder state = (KinomaSpeexDecoder)stateIn;
	Boolean canChangeSampleRate = false;

	if (state)
	{
		canChangeSampleRate = state->can_change_samplerate;
	}
	else
	{
		err = kFskErrOperationFailed;
		goto bail;
	}

	property->value.b = canChangeSampleRate;
	property->type = kFskMediaPropertyTypeBoolean;

bail:
	return err;
}

static FskErr speexDecodeSetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	KinomaSpeexDecoder state = (KinomaSpeexDecoder)stateIn;
	FskAudioDecompress deco = (FskAudioDecompress)obj;
	Boolean canChangeSampleRate = property->value.b;

	if (deco->frameNumber == 0 && state)
	{
		state->can_change_samplerate = canChangeSampleRate;
	}
	else
	{
		err = kFskErrOperationFailed;
		goto bail;
	}

bail:
	return err;
}
#endif
