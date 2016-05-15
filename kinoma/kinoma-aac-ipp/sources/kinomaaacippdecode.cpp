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
#define __FSKAUDIO_PRIV__
#define __FSKAACDECODE_PRIV__
#include "kinoma_ipp_lib.h"

#include "kinomaaacippdecode.h"

//#define DEBUG_DELAY_OUTPUT
//#define DUMP_WAVE
//#define DUMP_ADTS
#if defined(DUMP_ADTS) || defined(DUMP_WAVE) 
#include "kinoma_utilities.h"
#endif

#include "umc_audio_codec.h"
#include "umc_aac_decoder_int.h"

#if SUPPORT_INSTRUMENTATION
FskInstrumentedTypeRecord gkinomaaacippdecodeTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "kinomaaacippdecode"};
//FskInstrumentedSimpleType(kinomaaacippdecode, kinomaaacippdecode);
#endif

using namespace UMC;

#if __cplusplus
	extern "C" {
#endif

#ifdef KINOMA_DEBUG
int g_kinoma_debug_on = 0;
int g_kinoma_debug_count = 0;
#endif

static FskErr aacDecodeGetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeGetChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeGetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeSetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeGetCanChangeChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeSetCanChangeChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeGetFormat(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeSetFormat(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord aacDecodeProperties[] = {
	{kFskMediaPropertySampleRate,				kFskMediaPropertyTypeInteger,		aacDecodeGetSampleRate,				NULL},
	{kFskMediaPropertyChannelCount,				kFskMediaPropertyTypeInteger,		aacDecodeGetChannelCount,			NULL},
	{kFskMediaPropertyCanChangeSampleRate,		kFskMediaPropertyTypeBoolean,		aacDecodeGetCanChangeSampleRate,	aacDecodeSetCanChangeSampleRate},
	{kFskMediaPropertyCanChangeChannelCount,	kFskMediaPropertyTypeBoolean,		aacDecodeGetCanChangeChannelCount,	aacDecodeSetCanChangeChannelCount},
	{kFskMediaPropertyFormat,					kFskMediaPropertyTypeString,		aacDecodeGetFormat,					aacDecodeSetFormat},
	{kFskMediaPropertyUndefined,				kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};


typedef struct FskAACDecodeRecord FskAACDecodeRecord;
typedef struct FskAACDecodeRecord *FskAACDecode;

struct FskAACDecodeRecord 
{
	int canChangeSampleRate;
	int canChangeChannelCount;
	int	outputSampleRate;
	int	outputChannelCount;
	//int	outFormat;


#ifdef DEBUG_DELAY_OUTPUT
	int				totalBytes_cache;
	unsigned char   *wav_data_cache;
#endif

	AACDecoderInt *dec;
};

#if 0 // not used till now
// sloppy implementation
static int QTESDSScanAudio
(
	unsigned char 	*esds, 
	long 			count, 
	unsigned char 	*codec, 
	long 			*sampleRate, 
	long 			*channelCount
)
{
	const long sampleRates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000};

	while (count-- && (4 != *esds))
		esds += 1;

	if (4 == *esds++) 
	{
		while (*esds++ & 0x80)
			;
		*codec = *esds++;

		esds += 12;
		if (0x05 == *esds++) 
		{
			while (*esds++ & 0x80)
				;
			*sampleRate = sampleRates[((esds[0] & 0x07) << 1) | ((esds[1] & 0x80) >> 7)];
			*channelCount = (esds[1] & 0x78) >> 3;
			return true;
		}
	}

	return false;
}
#endif


FskErr aacDecodeCanHandle(UInt32 format, const char *formatStr, Boolean *canHandle)
{
	dlog( "into aacDecodeCanHandle, format: %d, formatStr: %s\n", (int)format, formatStr);

	*canHandle = (kFskAudioFormatAAC == format) || (0 == FskStrCompare(formatStr, "format:aac")) || (0 == FskStrCompare(formatStr, "x-audio-codec/aac"));
		
	return kFskErrNone;
}

FskErr aacDecodeNew(FskAudioDecompress deco, UInt32 format, const char *mime)
{
	FskAACDecode state;
	FskErr		 err = kFskErrNone;

	dlog( "into aacDecodeNew, format: %d, mime: %s\n", (int)format, mime);
	
#ifdef KINOMA_DEBUG
	g_kinoma_debug_on = 0;
	g_kinoma_debug_count = 0;
#endif

	if ((NULL == deco->formatInfo) || !deco->formatInfoSize)
		return kFskErrBadData;

	err = FskMemPtrNewClear(sizeof(FskAACDecodeRecord), (FskMemPtr *)&state);
	if (kFskErrNone != err) 
		goto bail;

	deco->state = state;

	state->canChangeSampleRate	= 0;
	state->outputSampleRate		= 0;
	deco->outputFormat			= kFskAudioFormatPCM16BitLittleEndian;
	deco->outputChannelCount	= deco->inputChannelCount;
	state->dec					= NULL;


#ifdef DEBUG_DELAY_OUTPUT
	state->totalBytes_cache = 0;
	state->wav_data_cache	= NULL;
#endif

bail:
	return err;
}

FskErr aacDecodeDispose(void *stateIn, FskAudioDecompress deco)
{
	FskAACDecode state = (FskAACDecode)stateIn;

	dlog( "into aacDecodeDispose\n" );
	
	if (state) 
	{
		if( state->dec )
			delete state->dec;

		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}

#define kMaxWavSize    (1024 * 8)
FskErr aacDecodeDecompressFrames(void *stateIn, FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *samplesSize)
{
	FskAACDecode 	state			= (FskAACDecode)stateIn;
	unsigned char 	*thisFrameData 	= (unsigned char *)data;
	unsigned char	*wav_data		= NULL;
	unsigned char 	*thisWavData	= NULL;
	int 			totalBytes		= 0;
	UInt32			i;
	UMC::MediaData_V51  input;
	UMC::AudioData_V51  output;
	FskErr 			err = kFskErrNone;

	dlog( "into aacDecodeDecompressFrames\n" );
	
#ifdef KINOMA_DEBUG
	g_kinoma_debug_count++;
	if( g_kinoma_debug_count <= 1 )
		g_kinoma_debug_on = 1;
	else
		g_kinoma_debug_on = 0;
#endif

	if( state->dec == NULL )
	{
		unsigned char *esds 	= (unsigned char *)deco->formatInfo;
		long	      esdsSize	= deco->formatInfoSize;

		kinoma_ipp_lib_aac_init(FSK_ARCH_AUTO);	
		state->dec = new AACDecoderInt();
		if( state->dec == NULL )
		{
			err = kFskErrMemFull;
			goto bail;
		}
		
		state->dec->Init2(esds, esdsSize);	
	}

	err = FskMemPtrNew(kMaxWavSize * frameCount, (FskMemPtr *)&wav_data);
	if (err) return err;
	
	thisWavData = wav_data;

	if( !state->canChangeSampleRate )
		state->dec->SetIgnoreSBR();

	for( i = 0; i < frameCount; i++ )
	{
		long thisFrameSize = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[i];
		//long thisFrameSize = frameCount == 1 ? dataSize : frameSizes[i];
		long thisWavSize;
		UMC::Status umcRes = UMC::UMC_OK;

#ifdef DUMP_ADTS
		{
			static FILE *adts_dump_file = 0;
			unsigned char  adts_header[16];
			int adts_header_size = 7;

			if( adts_dump_file == NULL )
			{
				adts_dump_file = fopen("E:\\adts_dump_file.adts","wb");	
			}
				
			if( adts_dump_file != NULL )
			{
				unsigned char adts_header[16];
				
				make_adts_header( deco->inputSampleRate, deco->outputChannelCount, thisFrameSize, adts_header );

				fwrite(adts_header, adts_header_size, 1, adts_dump_file );
				fwrite(thisFrameData,   thisFrameSize,   1, adts_dump_file );
				fflush( adts_dump_file );
			}	
		}
#endif

		input.SetBufferPointer( thisFrameData, thisFrameSize );
        input.SetTime(0);
		
		thisFrameData += thisFrameSize;
		
		output.SetBufferPointer(thisWavData, kMaxWavSize);		
		output.SetDataSize(0);		

		umcRes = state->dec->GetFrame( &input, &output );
		if( umcRes != UMC::UMC_OK )
			break;

#ifdef KINOMA_DEBUG
		if( g_kinoma_debug_on )
		fprintf( stderr, "   kinoma debug: input:%d, output:%2d,%2d,%2d,%2d,%2d,%2d...\n", thisFrameSize, 
			thisWavData[0],thisWavData[1],thisWavData[2],thisWavData[3],thisWavData[4],thisWavData[5] );
#endif		

		if( (deco->frameNumber == 0) && (0 == i) )
		{
			if (state->canChangeSampleRate && output.m_info.sample_frequency)
				state->outputSampleRate		= output.m_info.sample_frequency;

			if (state->canChangeChannelCount && output.m_info.channels)
				state->outputChannelCount	= deco->outputChannelCount = output.m_info.channels;
		}

		thisWavSize  = output.GetDataSize();
		totalBytes  += thisWavSize;
		thisWavData += thisWavSize;

	}

bail:	
	if( totalBytes == 0 && wav_data != NULL )
	{
		FskMemPtrDisposeAt( (void **)&wav_data );
		wav_data = NULL;
	}


#ifdef DEBUG_DELAY_OUTPUT
	{
		if( state->wav_data_cache == NULL )
		{
			*samplesSize = 0;
			*samples 	 = NULL;	

			state->totalBytes_cache = totalBytes;
			state->wav_data_cache   = wav_data;
		}
		else
		{
			*samplesSize = state->totalBytes_cache;
			*samples 	 = (void *)state->wav_data_cache;

			state->totalBytes_cache = totalBytes;
			state->wav_data_cache   = wav_data;
		}
	}
#else
	*samplesSize = totalBytes;
	*samples 	 = (void *)wav_data;
#endif


#ifdef	DUMP_WAVE
	{
		static FILE *f = NULL;
		
		if( f == NULL )
			wav_create_for_write( "E:\\ipp_sound_dump.wav", (int)state->outputSampleRate, (int)state->outputChannelCount, &f );
	
		if( f != NULL )
			wav_write( (short *)*samples, (long)state->outputChannelCount, (long)*samplesSize/state->outputChannelCount/2, f );
	}
#endif


	return err;
}


FskErr aacDecodeDecompressFrames2( unsigned char *esds, int esdsSize, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, int *sample_rate, int *channel_total )
{
	unsigned char 	*thisFrameData 	= (unsigned char *)data;
	unsigned char	*wav_data		= NULL;
	//unsigned char 	*thisWavData	= NULL;
	//int 			totalBytes		= 0;
	UMC::MediaData_V51  input;
	UMC::AudioData_V51  output;
	AACDecoderInt	*dec = NULL;
	FskErr 			err = kFskErrNone;

	dlog( "into aacDecodeDecompressFrames2\n" );
	
	dlog( "calling kinoma_ipp_lib_aac_init\n" );
	kinoma_ipp_lib_aac_init(FSK_ARCH_AUTO);	
	dec = new AACDecoderInt();
	if( dec == NULL )
	{
		dlog( "AACDecoderInt failed\n" );
		err = kFskErrMemFull;
		goto bail;
	}
	
	dlog( "calling dec->Init2\n" );
	dec->Init2(esds, esdsSize);	

	err = FskMemPtrNew(kMaxWavSize, (FskMemPtr *)&wav_data);
	if (err) goto bail;
	
	{
		long thisFrameSize = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[0];
		UMC::Status umcRes = UMC::UMC_OK;

		input.SetBufferPointer( thisFrameData, thisFrameSize );
        input.SetTime(0);
		
		output.SetBufferPointer(wav_data, kMaxWavSize);		
		output.SetDataSize(0);		

		dlog( "calling dec->GetFrame\n" );
		umcRes = dec->GetFrame( &input, &output );
		if( umcRes != UMC::UMC_OK )
			goto bail;

		dlog( "output.m_info.sample_frequency: %d, output.m_info.channels: %d\n", output.m_info.sample_frequency, output.m_info.channels );
		*sample_rate	= output.m_info.sample_frequency;
		*channel_total	= output.m_info.channels;
	}

bail:	
	if( wav_data != NULL )
		FskMemPtrDisposeAt( (void **)&wav_data );

	if( dec != NULL )
		delete dec;

	dlog( "out of aacDecodeDecompressFrames2\n" );
	
	return err;
}


FskErr aacDecodeDiscontinuity(void *stateIn, FskAudioDecompress deco)
{
	FskAACDecode state = (FskAACDecode)stateIn;
	UMC::Status umcRes = UMC::UMC_OK;
	
	if( state && state->dec )
	{
		umcRes = state->dec->Reset();
		//FskDebugStr("aacDecodeDiscontinuity");

#ifdef DEBUG_DELAY_OUTPUT
		state->totalBytes_cache = 0;
		state->wav_data_cache	= NULL;
#endif
	}
	
	return kFskErrNone;
}



static FskErr aacDecodeGetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAACDecode state = (FskAACDecode)stateIn;
	UInt32 sampleRate = 0;
	
	dlog( "into aacDecodeGetSampleRate\n" );
	
	if (state && state->outputSampleRate != 0)
	{
		sampleRate = state->outputSampleRate;
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

static FskErr aacDecodeGetChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAACDecode state = (FskAACDecode)stateIn;
	UInt32 channelCount = 0;

	dlog( "into aacDecodeGetChannelCount\n" );
	
	
	if (state && state->outputChannelCount != 0)
	{
		channelCount = state->outputChannelCount;
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

static FskErr aacDecodeGetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAACDecode state = (FskAACDecode)stateIn;
	Boolean canChangeSampleRate = false;

	dlog( "into aacDecodeGetCanChangeSampleRate\n" );
	
	if (state)
	{
		canChangeSampleRate = state->canChangeSampleRate;
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

static FskErr aacDecodeSetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAACDecode state = (FskAACDecode)stateIn;
	FskAudioDecompress deco = (FskAudioDecompress)obj;
	Boolean canChangeSampleRate = property->value.b;

	dlog( "into aacDecodeSetCanChangeSampleRate\n" );
	
	if (deco->frameNumber == 0 && state)
	{
		state->canChangeSampleRate = canChangeSampleRate;
	}
	else
	{
		err = kFskErrOperationFailed;
		goto bail;
	}

bail:
	return err;
}

static FskErr aacDecodeGetCanChangeChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAACDecode state = (FskAACDecode)stateIn;
	Boolean canChangeChannelCount = false;

	dlog( "into aacDecodeGetCanChangeChannelCount\n" );
	
	if (state)
	{
		canChangeChannelCount = state->canChangeChannelCount;
	}
	else
	{
		err = kFskErrOperationFailed;
		goto bail;
	}

	property->value.b = canChangeChannelCount;
	property->type = kFskMediaPropertyTypeBoolean;

bail:
	return err;
}

static FskErr aacDecodeSetCanChangeChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAACDecode state = (FskAACDecode)stateIn;
	FskAudioDecompress deco = (FskAudioDecompress)obj;
	Boolean canChangeChannelCount = property->value.b;

	dlog( "into aacDecodeSetCanChangeChannelCount\n" );
	
	if (deco->frameNumber == 0 && state)
	{
		state->canChangeChannelCount = canChangeChannelCount;
	}
	else
	{
		err = kFskErrOperationFailed;
		goto bail;
	}

bail:
	return err;
}

static FskErr aacDecodeGetFormat(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	//FskFhGAACDecode state = (FskFhGAACDecode)stateIn;

	dlog( "into aacDecodeGetFormat\n" );
	
	//if (!state->outputFormat)
	{
	//	err = kFskErrOperationFailed;
	//	goto bail;
	}
	//property->value.str = FskStrDoCopy(state->outputFormat);
	//property->type = kFskMediaPropertyTypeString;
//bail:

	return err;
}

static FskErr aacDecodeSetFormat(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	//FskFhGAACDecode state = (FskFhGAACDecode)stateIn;
	//FskAudioDecompress deco = (FskAudioDecompress)obj;

	dlog( "into aacDecodeSetFormat\n" );
	
	//if (0 != deco->frameNumber)
	{
	//	err = kFskErrOutOfSequence;
	//	goto bail;
	}
	//if (FskStrCompare(property->value.str, "format:pcm16le") && FskStrCompare(property->value.str, "format:pcm16be"))
	{
	//	err = kFskErrBadData;
	//	goto bail;
	}
	//FskMemPtrDispose(state->outputFormat);
	//state->outputFormat = FskStrDoCopy(property->value.str);
//bail:

	return err;
}


#ifdef __cplusplus
}
#endif

