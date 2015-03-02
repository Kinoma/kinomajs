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
#define __FSKAUDIOCODEC_PRIV__

#include "FskArch.h"
#include "FskEndian.h"
#include "FskAudio.h"
#include "kinomaspeexenc.h"

#include "speex.h"
#include "speex_header.h"

//#if TARGET_OS_ANDROID
void kinoma_speex_init(int mode);
//#endif

#define kWBSamplesPerFrame			320
#define kNBSamplesPerFrame			160
#define kSpeexBitsBuferSize			2000
#define kSPeexFramesPerPacket		4
#define kSpeexDefaultQuailty			8
#define kSpeexDefaultComplexity		3


typedef struct 
{
	SpeexBits	bits;
	int			speex_mode;						//0:nb, 1:wb, 2:uwb
	int			speex_quality;
	int			speex_complexity;
	int			speex_vbr;
	int			speex_samples_per_frame;
	int			speex_frames_per_packet;
	int			speex_frame_count_in_packet;

	int			bps;
	int			hz;

	void		*enc;

	UInt16		*leftoverSrcSamples;
	UInt32		leftoverSrcSamplesSize;
} KinomaSpeexEncoderRecord, *KinomaSpeexEncoder;

static FskErr GetHeader(KinomaSpeexEncoder state, UInt32 *headerSize, UInt8 **headerOut);

FskErr speexEncodeCanHandle(const char *format, Boolean *canHandle)
{
	*canHandle =	(0 == FskStrCompare(format, "format:speex"))			|| 
					(0 == FskStrCompare(format, "format:speexwb"))			||		
					(0 == FskStrCompare(format, "format:speexnb"))			||
					(0 == FskStrCompare(format, "x-audio-codec/speex"))		||
					(0 == FskStrCompare(format, "x-audio-codec/speex-wb"))	||
					(0 == FskStrCompare(format, "x-audio-codec/speex-nb"));

	return kFskErrNone;
}


FskErr speexEncodeNew(FskAudioCompress comp)
{
	KinomaSpeexEncoder state =NULL;
	FskErr	err;

	err = FskMemPtrNewClear(sizeof(KinomaSpeexEncoderRecord), (FskMemPtr *)&state);
	if (0 != err) goto bail;

	comp->state = (KinomaSpeexEncoder)state;

	state->speex_vbr = 0;
	state->speex_mode				= SPEEX_MODEID_NB;
	state->speex_quality			= kSpeexDefaultQuailty;
	state->speex_complexity			= kSpeexDefaultComplexity;
	state->speex_samples_per_frame	= kNBSamplesPerFrame;
	state->speex_frames_per_packet	= kSPeexFramesPerPacket;
	state->speex_frame_count_in_packet=0;
	
	state->bps = 0;
	state->hz  = comp->inputSampleRate;

	comp->outputSamplesPerFrame = state->speex_samples_per_frame*state->speex_frames_per_packet;

bail:
	return err;
}


FskErr speexEncodeDispose(void *stateIn, FskAudioCompress comp)
{
	KinomaSpeexEncoder state = (KinomaSpeexEncoder)stateIn;

	if (NULL != state) 
	{
		if (NULL != state->enc) 
		{
			speex_bits_destroy(&state->bits);
			speex_encoder_destroy(state->enc);
		}
		FskMemPtrDispose(state->leftoverSrcSamples);
		FskMemPtrDisposeAt(&comp->state);
	}

	return kFskErrNone;
}


FskErr speexEncodeCompressFrames( void *stateIn, FskAudioCompress comp, const void *data, UInt32 dataSize, UInt32 inSampleCount, void **outSamples, UInt32 *outDataSize, UInt32 **outFrameSizes, UInt32 *frameCount)
{
	KinomaSpeexEncoder	state = (KinomaSpeexEncoder)stateIn;
	FskErr err = 0;
	UInt32	outputFrameCount = 0;
	UInt32	outputDataSize = 0;
	UInt32	*outputFrameSizes = 0;
	UInt8	*outputSamples = 0;
	UInt8	*buffer = 0;
	spx_int16_t	*inputSamples = (spx_int16_t*)data;

	if (NULL == state->enc) 
	{
		kinoma_speex_init(FSK_ARCH_AUTO);

		speex_bits_init(&state->bits);	//bits has a 2k bytes internal buffer, translate to 2000/(320*2)
		//speex_bits_reset(&state->bits);

		state->enc = speex_encoder_init(&speex_nb_mode);
		if( state->speex_mode == SPEEX_MODEID_NB )
		{
			state->enc = speex_encoder_init(speex_lib_get_mode(SPEEX_MODEID_NB));
			state->speex_samples_per_frame = kNBSamplesPerFrame;
		}
		else if( state->speex_mode == SPEEX_MODEID_WB )
		{
			state->enc = speex_encoder_init(speex_lib_get_mode(SPEEX_MODEID_WB));
			state->speex_samples_per_frame = kWBSamplesPerFrame;
		}
		else if(  state->speex_mode == SPEEX_MODEID_UWB )
		{
			;//***not supported yet
		}

		speex_encoder_ctl(state->enc, SPEEX_SET_COMPLEXITY,		&state->speex_complexity);
		speex_encoder_ctl(state->enc, SPEEX_SET_SAMPLING_RATE,	&state->hz);
		speex_encoder_ctl(state->enc, SPEEX_SET_VBR,			&state->speex_vbr);
		if( state->bps == 0 )
			speex_encoder_ctl(state->enc, SPEEX_SET_QUALITY,	&state->speex_quality);
		else
		{
			if( state->speex_vbr)
				speex_encoder_ctl(state->enc, SPEEX_SET_ABR,	&state->bps);
			else
				speex_encoder_ctl(state->enc, SPEEX_SET_BITRATE,&state->bps);
		}

		comp->outputSamplesPerFrame = state->speex_samples_per_frame*state->speex_frames_per_packet;
	}

	if (NULL == comp->desc) 
		GetHeader(state, &comp->descSize, (UInt8**)&comp->desc);
	
	if (NULL != state->leftoverSrcSamples) 
	{
		err = FskMemPtrNew(state->leftoverSrcSamplesSize + dataSize, (FskMemPtr*)&buffer);
		if (0 != err) goto bail;

		inSampleCount += state->leftoverSrcSamplesSize / 2;
		FskMemMove(buffer, state->leftoverSrcSamples, state->leftoverSrcSamplesSize);
		FskMemMove(&buffer[state->leftoverSrcSamplesSize], data, dataSize);
		FskMemPtrDisposeAt(&state->leftoverSrcSamples);
		dataSize += state->leftoverSrcSamplesSize;
		state->leftoverSrcSamplesSize = 0;
		inputSamples = (spx_int16_t*)buffer;
	}
	
	// Flush
	if (NULL == data && 0 != inSampleCount && inSampleCount < (UInt32)(state->speex_samples_per_frame)) 
	{
		UInt32 fillSampleCount = comp->outputSamplesPerFrame - inSampleCount;
		err = FskMemPtrRealloc(fillSampleCount * 2 + dataSize, (FskMemPtr*)&buffer);
		if (0 != err) goto bail;
		FskMemSet(&buffer[dataSize], 0, fillSampleCount * 2);
		inputSamples = (spx_int16_t*)buffer;
		dataSize += fillSampleCount * 2;
		inSampleCount = comp->outputSamplesPerFrame;
	}
	
	if (0 == dataSize)
		goto bail;
		
	err = FskMemPtrNew((inSampleCount / comp->outputSamplesPerFrame+1) * sizeof(UInt32), (FskMemPtr*)&outputFrameSizes);
	if (0 != err) goto bail;

	{
		UInt32 not_enough_sample_total   = inSampleCount % state->speex_samples_per_frame;
		int is_flush = (NULL == data);

		while( inSampleCount >  not_enough_sample_total || is_flush ) 
		{
			int bytesEncoded;
			
			if( inSampleCount > 0 )
			{
				//int frame_level;

				speex_encode_int(state->enc, inputSamples, &state->bits);
				//speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_LEVEL, &frame_level);

				inSampleCount -= state->speex_samples_per_frame;
				inputSamples  += state->speex_samples_per_frame;
				state->speex_frame_count_in_packet++;
			}

			if( state->speex_frame_count_in_packet == state->speex_frames_per_packet || is_flush )
			{
				bytesEncoded = speex_bits_nbytes(&state->bits);

				err = FskMemPtrRealloc(outputDataSize + bytesEncoded, (FskMemPtr*)&outputSamples);
				if (0 != err) goto bail;

				speex_bits_write(&state->bits, (char *)&outputSamples[outputDataSize], bytesEncoded);	//output compressed data when reach wanted frame total
				
				outputDataSize += bytesEncoded;
				outputFrameSizes[outputFrameCount++] = bytesEncoded;
				
				speex_bits_reset(&state->bits);
				state->speex_frame_count_in_packet = 0;

				if( is_flush )
					break;
			}
		}
	}
	
	if (0 != inSampleCount) //take care of samples can't be fit into a speex frame
	{
		err = FskMemPtrNewFromData(inSampleCount * 2, inputSamples, (FskMemPtr*)&state->leftoverSrcSamples);
		if (0 != err) goto bail;
		state->leftoverSrcSamplesSize = inSampleCount * 2;
	}
	
bail:
	if (0 != err) 
	{
		FskMemPtrDisposeAt(&outputFrameSizes);
		FskMemPtrDisposeAt(&outputSamples);
		outputFrameCount = 0;
		outputDataSize = 0;
	}

	FskMemPtrDispose(buffer);
	*outSamples = outputSamples;
	*outDataSize = outputDataSize;
	*outFrameSizes = outputFrameSizes;
	*frameCount = outputFrameCount;
	
	return err;
}


FskErr GetHeader(KinomaSpeexEncoder state, UInt32 *headerSize, UInt8 **headerOut)
{
	FskErr err = kFskErrNone;
	SpeexHeader header, *le_header = NULL;
	const SpeexMode *mode;
	int size;

	if (NULL == state->enc)
		return kFskErrBadState;

	mode = speex_lib_get_mode(state->speex_mode);
	speex_init_header(&header, state->hz, 1, mode);
	le_header = (SpeexHeader*)speex_header_to_packet(&header, &size);
	
	*headerSize = size;
	err = FskMemPtrNewFromData(size, le_header, headerOut);
	if (0 != err) goto bail;

bail:
	speex_header_free(le_header);
	return err;
}

FskErr speexEncodeSetBitrate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	KinomaSpeexEncoder	state = (KinomaSpeexEncoder)stateIn;
	FskErr			err = kFskErrNone;

	state->bps = property->value.integer;

	return err;
}

FskErr speexEncodeGetBitrate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	KinomaSpeexEncoder	state = (KinomaSpeexEncoder)stateIn;
	FskErr err = kFskErrNone;

	property->value.integer = state->bps;

	return err;
}

/*
from manual:

Quality (variable)
Speex is a lossy codec, which means that it achives compression at the expense of fidelity of the input speech signal. Unlike
some other speech codecs, it is possible to control the tradeoff made between quality and bit-rate. The Speex encoding process
is controlled most of the time by a quality parameter that ranges from 0 to 10. In constant bit-rate (CBR) operation, the quality
parameter is an integer, while for variable bit-rate (VBR), the parameter is a float.

Complexity (variable)
With Speex, it is possible to vary the complexity allowed for the encoder. This is done by controlling how the search is
performed with an integer ranging from 1 to 10 in a way that¡¯s similar to the -1 to -9 options to gzip and bzip2 compression
utilities. For normal use, the noise level at complexity 1 is between 1 and 2 dB higher than at complexity 10, but the CPU
requirements for complexity 10 is about 5 times higher than for complexity 1. In practice, the best trade-off is between
complexity 2 and 4, though higher settings are often useful when encoding non-speech sounds like DTMF tones.

Variable Bit-Rate (VBR)
Variable bit-rate (VBR) allows a codec to change its bit-rate dynamically to adapt to the ¡°difficulty¡± of the audio being
encoded. In the example of Speex, sounds like vowels and high-energy transients require a higher bit-rate to achieve good
quality, while fricatives (e.g. s,f sounds) can be coded adequately with less bits. For this reason, VBR can achive lower bit-rate
for the same quality, or a better quality for a certain bit-rate. Despite its advantages, VBR has two main drawbacks: first, by
only specifying quality, there¡¯s no guaranty about the final average bit-rate. Second, for some real-time applications like voice
over IP (VoIP), what counts is the maximum bit-rate, which must be low enough for the communication channel.

Average Bit-Rate (ABR)
Average bit-rate solves one of the problems of VBR, as it dynamically adjusts VBR quality in order to meet a specific target
bit-rate. Because the quality/bit-rate is adjusted in real-time (open-loop), the global quality will be slightly lower than that
obtained by encoding in VBR with exactly the right quality setting to meet the target average bit-rate
*/

static void GetNextWord( char *s, long *idx, char *w )
{
	long i = *idx;
	long j = 0;
	
	while( s[i] != ';' && s[i] != ',' && s[i] != 0 )
	{
		w[j] = s[i];
		i++;
		j++;
	}
	
	w[j] = 0;
	if( s[i] == 0 )
		*idx = i;
	else
		*idx = i+1;
}

static void GetKeyAndValue( char *w, char *k, char *v )
{
	long i = 0, j = 0;

	while( w[i] != 0 && w[i] != '=' && w[i] != ':' )
	{
		k[j] = w[i];
		i++;
		j++;
	}
	k[j] = 0;
	
	j = 0;
	while( w[i] != 0  )
	{
		if( w[i] != '=' && w[i] != ':' )
		{
			v[j] = w[i];
			j++;
		}
		
		i++;
	}
	v[j] = 0;
}


FskErr speexEncodeSetCompressionSettings(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	KinomaSpeexEncoder	state	= (KinomaSpeexEncoder)stateIn;
	char				*s		= property->value.str;
	long				i		= 0;
	FskErr				err		= kFskErrNone;

	while(1)
	{
		char w[64];
		char k[32], v[32];

		GetNextWord( s, &i, w );
		if( w[0] == 0 )
			break;

#define SET_PARAM(s, k, v, p)  if( strcmp( k, s ) == 0 ) p = (int)atoi( v );

		GetKeyAndValue( w, k, v );
		
		SET_PARAM( "speex_mode",				k, v, state->speex_mode );				//0:nb, 1:wb, 2:uwb(not supported yet!!!)
		SET_PARAM( "speex_complexity",			k, v, state->speex_complexity );		//1~10
		SET_PARAM( "speex_quality",				k, v, state->speex_quality );			//0~10
		SET_PARAM( "speex_vbr",					k, v, state->speex_vbr );				
		SET_PARAM( "speex_frames_per_packet",	k, v, state->speex_frames_per_packet );	//1~8 are safe for flv, could be more for other file format
	}

	return err;
}


FskErr speexEncodeGetCompressionSettings(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	//KinomaSpeexEncoder	state = (KinomaSpeexEncoder)stateIn;
	FskErr 			err = kFskErrNone;
	
	//property->value.integer = state->key_fps;

	return err;
}
