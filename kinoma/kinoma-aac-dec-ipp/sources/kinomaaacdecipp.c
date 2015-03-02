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
#define __FSKAUDIO_PRIV__
#define __FSKAACDECODE_PRIV__

#include "kinomaaacdecipp.h"

#include "codecAC.h"
#include "misc.h"
#include "ippdefs.h"
#include "QTReader.h"


#if SUPPORT_INSTRUMENTATION
FskInstrumentedTypeRecord gkinomaaacdecippTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "kinomaaacdecipp"};
//FskInstrumentedSimpleType(kinomaaacdecipp, kinomaaacdecipp);
#endif

//#define DUMP_WAV

#ifdef DUMP_WAV
//***bnie utilities
#include "stdlib.h"
#include "stdio.h"

#define ShortSwapEndian( s )  ( ( s&0x00ff )<<8 | ( ( s&0xff00 )>> 8 ) )
#define LongSwapEndian( s )  (  ( s&0x000000ff )<<24 | ( s&0x0000ff00 )<<8 | ( s&0x00ff0000 )>>8 | ( s&0xff000000 )>>24 )

#ifdef _BIG_ENDIAN_ 
#define ShortN2L(s)  ShortSwapEndian(s)
#define LongN2L(s)   LongSwapEndian(s)
#define LongL2N(s)   LongSwapEndian(s)
#define ShortL2N(s)  ShortSwapEndian(s)
#define LongN2B(s)   (s)
#define LongB2N(a)   (s)
#define ShortB2N(s)  (s)
#else
#define ShortN2L(s)  (s)
#define LongN2L(s)   (s)
#define LongL2N(s)   (s)
#define ShortL2N(s)  (s)
#define LongN2B(s)   LongSwapEndian(s)
#define LongB2N(s)   LongSwapEndian(s)
#define ShortB2N(s)  ShortSwapEndian(s)
#endif	

#define WRITE_LE_LONG( v, d )   {									\
d[0]	= (v>>0)&0x000000ff;	\
d[1]	= (v>>8)&0x000000ff;	\
d[2]	= (v>>16)&0x000000ff;	\
d[3]	= (v>>24)&0x000000ff;	\
}

#define WRITE_LE_SHORT( v, d )  {								\
d[0]	= (v>>0)&0x00ff;	\
d[1]	= (v>>8)&0x00ff;	\
}



int wav_create_for_write( char *filename, long  sampleRate, long  numChannels, FILE **inputFile )
{
	FILE	*soundFile = *inputFile;
	
	if( inputFile == NULL )
		return -1;
	
	soundFile = fopen(filename,"wb");	
	*inputFile = soundFile;
	if( soundFile == NULL )
	{
		printf("cannot open dump file for write (%s)\n", filename );
		return -1;
	}	
	{
		long	fourBytes;
		short	twoBytes;
		
		fourBytes = LongN2B('RIFF');
		fwrite( &fourBytes,  4, 1, soundFile);
		
		fourBytes = 1024*1024*10;							//total size not known yet
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
		
		fourBytes = LongN2B('WAVE');		
		fwrite( &fourBytes, 4, 1, soundFile);
		
		fourBytes = LongN2B('fmt ');		
		fwrite( &fourBytes, 4, 1, soundFile);
		
		fourBytes = 16;		
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
		
		twoBytes = 1;										//compression mode, 1 PCM/uncompressed
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);
		
		twoBytes = numChannels;								//	number of channels
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);
		
		fourBytes = sampleRate;								//sample rate
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
		
		fourBytes = sampleRate * 2 * numChannels;			//average bytes per second
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
		
		twoBytes = numChannels * 2;							//	block align
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);
		
		twoBytes = 16;										//	significant bits per sample
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);
		
		fourBytes = LongN2B('data');
		fwrite( &fourBytes,  4, 1, soundFile);
		
		fourBytes = 1024*1024*50;							//total size not known yet
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
	}
	
	fflush( soundFile );
	
	return 0;
}


void wav_write( short *soundDataPtr, long numChannels, long sampleCount, FILE *soundFile )
{
	int i;
	
	if( soundFile == NULL )
		return;
	
	for( i = 0; i < sampleCount; i++ )
	{
		short twoBytes;
		
		twoBytes = *soundDataPtr;
		twoBytes = ShortN2L( twoBytes );
		fwrite( &twoBytes, 2, 1, soundFile);
		soundDataPtr++;
		
		if( numChannels == 2 )
		{
			twoBytes = *soundDataPtr;
			twoBytes = ShortN2L( twoBytes );
			fwrite( &twoBytes, 2, 1, soundFile);
			soundDataPtr++;
		}
	}
	
	fflush( soundFile );
}



#endif


static FskErr aacDecodeGetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeGetChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeGetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeSetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeGetCanChangeChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aacDecodeSetCanChangeChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
//static FskErr aacDecodeGetFormat(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
//static FskErr aacDecodeSetFormat(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord aacDecodeProperties[] = 
{
	{kFskMediaPropertySampleRate,				kFskMediaPropertyTypeInteger,		aacDecodeGetSampleRate,				NULL},
	{kFskMediaPropertyChannelCount,				kFskMediaPropertyTypeInteger,		aacDecodeGetChannelCount,			NULL},
	{kFskMediaPropertyCanChangeSampleRate,		kFskMediaPropertyTypeBoolean,		aacDecodeGetCanChangeSampleRate,	aacDecodeSetCanChangeSampleRate},
	{kFskMediaPropertyCanChangeChannelCount,	kFskMediaPropertyTypeBoolean,		aacDecodeGetCanChangeChannelCount,	aacDecodeSetCanChangeChannelCount},
	{kFskMediaPropertyFormat,					kFskMediaPropertyTypeString,		NULL,								NULL }, //aacDecodeGetFormat,					aacDecodeSetFormat},
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
	
	
	int							delay;
	int							frameNum;		
	
	IppBitstream				bitStream;				//AAC decoder input structure
	IppSound					sound;					//AAC decoder output structure
	MiscGeneralCallbackTable   *pCallBackTable;			//Misc callback table
	
	int							input_format;
	char						*pInputBuf;		
	char						*pOutBuf;			
	
	void						*acc_dec;
	
};

FskErr aacDecodeCanHandle(UInt32 format, const char *formatStr, Boolean *canHandle)
{

	dlog( "into aacDecodeCanHandle\n"); 
	*canHandle = (kFskAudioFormatAAC == format) || (0 == FskStrCompare(formatStr, "format:aac")) || (0 == FskStrCompare(formatStr, "x-audio-codec/aac"));

	return kFskErrNone;
}

FskErr aacDecodeNew(FskAudioDecompress deco, UInt32 format, const char *mime)
{
	
	FskErr err = kFskErrNone;
	FskAACDecode state;

	dlog( "/**********************************************************/\n"); 
	dlog( "into aacDecodeNew()\n"); 
	err = FskMemPtrNewClear(sizeof(FskAACDecodeRecord), (FskMemPtr *)&state);
	if (kFskErrNone != err) 
		goto bail;
	
	deco->state = state;

	state->canChangeSampleRate	= 0;
	state->canChangeChannelCount= 0;
	state->outputSampleRate		= 0;
	state->outputChannelCount   = 0;

	deco->outputFormat			= kFskAudioFormatPCM16BitLittleEndian;

	// The software decoder doesn't properly support mono output on
	// AACplus files. Always output stereo.
	deco->outputChannelCount	= deco->inputChannelCount;

bail:
	return err;

}

FskErr aacDecodeDispose(void *stateIn, FskAudioDecompress deco)
{
	FskAACDecode state = (FskAACDecode)stateIn;

	dlog( "/**********************************************************/\n"); 
	dlog( "into aacDecodeDispose()\n"); 
	if( state != NULL ) 
	{
		dlog( "freeing state->pInputBuf: %x\n", (int)state->pInputBuf); 
		if( state->pInputBuf != NULL )
			IPP_MemFree((void **)&state->pInputBuf);
		
		dlog( "freeing state->pOutBuf: %x\n", (int)state->pOutBuf); 
		if( state->pOutBuf != NULL )
			IPP_MemFree((void **)&state->pOutBuf);
		
		dlog( "freeing state->acc_dec: %x\n", (int)state->acc_dec); 
		if( state->acc_dec != NULL )
			DecoderFree_AAC(&state->acc_dec);
		
		dlog( "freeing state->pCallBackTable: %x\n", (int)state->pCallBackTable); 
		if( state->pCallBackTable != NULL ) 
			IPP_MemFree ((void **)&state->pCallBackTable);

		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}

enum
{
	kAACObjectTypeMain = 1,
	kAACObjectTypeLow,
	kAACObjectTypeSSR,
	kAACObjectTypeLTP
};
const UInt32 kFskOMXAudioDecoderAACSampleRates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 0};
int scan_aac_esds
(
	unsigned char	*esds,
	int 	esds_size,
	char	*codec_out,
	int 	*audio_type_out,
	int 	*sample_rate_index_out,
	int 	*sampleRate_out,
	int 	*channelCount_out,
	int 	*audio_decoder_config_offset,
	int 	*audio_decoder_config_size
)
{
	unsigned char *esds0		= esds;
	char		codec;
	int			audioType;
	int 		sampleRate;
	int 		channelCount;
	int			samplerateIndex;
	
	while( esds_size-- && (4 != *esds) )
		esds++;
	
	if( esds_size-- && (4 == *esds++) )
	{
		while(  esds_size-- && (*esds++ & 0x80) )
			;
		
		if( esds_size > 13 )
		{
			codec     = *esds++;
			esds	  += 12;
			esds_size -= 13;
			
			if( codec_out != NULL )
				*codec_out = codec;
			
			if( esds_size-- && (0x05 == *esds++) )
			{
				while (esds_size-- && (*esds++ & 0x80) )
					;
				
				if( esds_size == 1 )
					esds_size = 2; //***bnie: if we've got this far, we'll give it a try				
				
				if( esds_size >= 2 )
				{
					//int	count = 0;
					
					audioType  = (esds[0]>>3)&0x1f;
					samplerateIndex = ((esds[0] & 0x07) << 1) | ((esds[1] & 0x80) >> 7);
					sampleRate = kFskOMXAudioDecoderAACSampleRates[samplerateIndex];
					channelCount = (esds[1] & 0x78) >> 3;
					
					if( audio_type_out != NULL )
						*audio_type_out = audioType;
					
					if( sample_rate_index_out != NULL )
						*sample_rate_index_out = samplerateIndex;
					
					if( sampleRate_out != NULL )
						*sampleRate_out = sampleRate;
					
					if( channelCount_out != NULL )
						*channelCount_out = channelCount;
					
					if( audio_decoder_config_offset != NULL )
						*audio_decoder_config_offset = (int)( esds - esds0 );
					
					//while (!(*esds++ & 0x80))			
					//	count++;
					//*audio_decoder_config_size = count;
					
					if( audio_decoder_config_size != NULL )
						*audio_decoder_config_size = esds_size;
					
					return 0;
				}
			}
		}
	}
	
	return -1;
}


FskErr aacDecodeDecompressFrames(void *stateIn, FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *samplesSize)
{
#define PREPARE_A_FRAME_DATA			\
	if( frame_idx >= 0 && frame_idx < frameCount )			\
	{									\
		thisFrameData += thisFrameSize;	\
		thisFrameSize  = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[frame_idx]; \
		frame_idx++;					\
		if( frame_idx >= frameCount )	\
			frame_idx = -1;				\
	}									\
	else								\
	{									\
		thisFrameData = NULL;			\
		thisFrameSize = 0;				\
		frame_idx     = -1;				\
	}	

#define COPY_A_FRAME_DATA				\
	IPP_Memcpy( state->bitStream.pBsBuffer, (void *)thisFrameData, thisFrameSize );	\
	state->bitStream.bsByteLen  = thisFrameSize;									\
	state->bitStream.pBsCurByte = state->bitStream.pBsBuffer;						\
	
	
	FskAACDecode 	state = (FskAACDecode)stateIn;
	int				frame_idx      = 0;
	int				thisFrameSize  = 0;
	unsigned char 	*thisFrameData = NULL;
	unsigned char	*wav_data	   = NULL;
	int				wav_size	   = 0;
	int				this_wav_size  = 0;
	int				done = 0;
	FskErr 			err  = 0;

	dlog( "/**********************************************************/\n"); 
	dlog( "into aacDecodeDecompressFrames(), frameCount: %d\n", (int)frameCount); 
	
	if( frameCount == 0 || data == NULL || dataSize == 0 )
	{
		//***bnie: flush out possible cached samples
		goto bail;
	}
	
	thisFrameData  = (unsigned char *)data;
	dlog( "####preparing next frame data, frame_idx : %d\n", (int)frame_idx); 
	PREPARE_A_FRAME_DATA;
	
	if(state->acc_dec != NULL )
	{
		dlog("copying this frame, thisFrameSize: %d\n", thisFrameSize);
		COPY_A_FRAME_DATA
		dlog( "####preparing next frame data, frame_idx : %d\n", (int)frame_idx); 
		PREPARE_A_FRAME_DATA		
	}
	else
	{
		unsigned char	*esds				= (unsigned char *)deco->formatInfo;
		int				esds_size			= deco->formatInfoSize;
        int             audio_type          = 0;
		int				channal_total		= 0;
		int				samplerate_index	= 0;
		int				samplerate			= 0;
		IppAACDecoderConfig	 decoder_config;	 //AAC decoding config structure

		err = scan_aac_esds( esds, esds_size, NULL, &audio_type, &samplerate_index, &samplerate, &channal_total, NULL, NULL );
		BAIL_IF_ERR( err );
        dlog( "called scan_aac_esds, samplerate: %d, channal_total: %d\n", (int)samplerate, (int)channal_total);

		{//extra scan for aac plus
		    int samplerate_ext = 0, sbr_flag = 0, profile = audio_type, level = 0;
		    get_extended_aac_profile_level((unsigned char*)(void*)data, (int)samplerate, (int)channal_total, &samplerate_ext, &sbr_flag, (int*)(void*)&profile, (int*)(void*)&level);
            samplerate    = samplerate_ext;
            dlog( "extended check result, samplerate/channal_total: %d/%d\n", (int)samplerate, (int)channal_total);
		}
		
		state->input_format			= AAC_SF_RAW;//AAC_SF_MP4ADTS
		state->delay				= 0;
		state->pCallBackTable		= NULL;	// Misc callback table
		state->frameNum				= 0;		
		state->outputChannelCount	= channal_total;
		state->outputSampleRate		= samplerate;
		
		err = IPP_MemMalloc((void**)&state->pCallBackTable,sizeof(MiscGeneralCallbackTable),4);
		if (IPP_FAIL == err || NULL == state->pCallBackTable) 
		{
			IPP_Log(NULL,"w","Fails to allocate callback tables!\n");
			err = IPP_FAIL; 
			goto bail;
		}
		
		state->pCallBackTable->fMemMalloc		= (MiscMallocCallback)IPP_MemMalloc;
		state->pCallBackTable->fMemCalloc		= (MiscCallocCallback)IPP_MemCalloc;
		state->pCallBackTable->fMemFree			= (MiscFreeCallback)IPP_MemFree;
		state->pCallBackTable->fStreamRealloc	= (MiscStreamReallocCallback)IPP_MemRealloc;
		state->pCallBackTable->fStreamFlush		= NULL;//(MiscStreamFlushCallback)miscgStreamFlush;
		state->pCallBackTable->fFileSeek		= NULL;//(MiscFileSeekCallback)IPP_Fseek;
		state->pCallBackTable->fFileRead		= NULL;//(MiscFileReadCallback)IPP_Fread;
		state->pCallBackTable->fFileWrite		= NULL;//(MiscWriteFileCallBack)IPP_Fwrite;
		
		//input setting
		dlog("Init input structure - input buffer must be 2 byte aligned\n");
		IPP_MemMalloc((void **)&state->pInputBuf, INPUT_BUF_SIZE_AAC+1, 2);	
		state->bitStream.pBsBuffer		= (Ipp8u *)state->pInputBuf;
		state->bitStream.pBsCurByte		= state->bitStream.pBsBuffer;
		state->bitStream.bsCurBitOffset =0;
	
		dlog("copying this frame, thisFrameSize: %d\n", thisFrameSize);
		COPY_A_FRAME_DATA
		dlog( "####preparing next frame data, frame_idx : %d\n", (int)frame_idx); 
		PREPARE_A_FRAME_DATA		
		
		IPP_Memset(&decoder_config, 0, sizeof(IppAACDecoderConfig));
		decoder_config.profileType = AAC_AOT_HE;//AAC_AOT_LC;//AAC_AOT_HE;
		decoder_config.pcmFmt	   = IPP_PCM_16B_INTLVD;
		decoder_config.streamFmt   = state->input_format;
		if ( AAC_SF_RAW == decoder_config.streamFmt ) 
		{
			decoder_config.samplingFrequencyIndex = samplerate_index;
			decoder_config.channelConfiguration   = channal_total;
		}
		
		//output settinf
		dlog("Init output structure - output buffer must be 4 byte aligned\n");
		if((AAC_AOT_HE == decoder_config.profileType) || (AAC_AOT_HE_PS == decoder_config.profileType)) 
			IPP_MemMalloc((void **)&state->pOutBuf, OUTPUT_BUF_SIZE_AAC * 4 * 6 + 3, 4);
		else 
			IPP_MemMalloc((void **)&state->pOutBuf, OUTPUT_BUF_SIZE_AAC * 2 * 6 + 3, 4);
		
		dlog("read input file for initialization\n");
		
		state->sound.pSndFrame = (Ipp16s*)state->pOutBuf;
		state->sound.sndLen =0;		
		
		dlog("calling DecoderInitAlloc_AAC\n");
		err = DecoderInitAlloc_AAC( &state->bitStream, &decoder_config, state->pCallBackTable, &state->acc_dec );
		if( err != IPP_STATUS_INIT_OK ) 
		{
			dlog("calling DecoderInitAlloc_AAC failed, err: %d\n", (int)err);
			err = IPP_FAIL;
			goto bail;
		}
		
		{
			int dmMode	= AAC_NO_DOWNMIX;
			dlog("calling IPPAC_AAC_SET_DOWNMIX_MODE\n");
			DecodeSendCmd_AACAudio(IPPAC_AAC_SET_DOWNMIX_MODE, &dmMode, NULL, state->acc_dec);
		}
	}
	
	/**************************************************************************
	 Invoke frame decoder until input file empty
	 The following frame decoder outcomes are possible:
	 
	 1. IPP_STATUS_FRAME_COMPLETE
	 - Sufficient data was present in the buffer 
	 to decode 1 frame, no frame abnormalities 
	 were detected.  Upon return, PCM samples 
	 are present in the PCM output buffer.
	 
	 2. IPP_STATUS_BUFFER_UNDERRUN
	 3. IPP_STATUS_SYNCNOTFOUND_ERR
	 - Insufficient data present in the buffer to 
	 decode 1 frame; application loads additional 
	 stream bytes into the ring buffer and 
	 invokes the decoder once again. 
	 
	 4. IPP_STATUS_NOTSUPPORTED	- The bitstream is not supported in this 
	 decoder
	 
	 5. IPP_STATUS_FRAME_ERR		- The bitstream has error, and the frame
	 cannot be decoded correctly.
	 6. IPP_STATUS_BS_END		- The bitstream goes to the end.
	 
	 **************************************************************************/
	while(!done)
	{
		int consumed = 0;
		//dlog("pBsBuffer:     %x\n", (int)state->bitStream.pBsBuffer);
		//dlog("pBsCurByte:    %x\n", (int)state->bitStream.pBsCurByte);
		//dlog("bsByteLen:     %d\n", (int)state->bitStream.bsByteLen);
		//dlog("bsCurBitOffset:%d\n", (int)state->bitStream.bsCurBitOffset);
		dlog("calling Decode_AAC()\n");
		err = Decode_AAC(&state->bitStream, &state->sound, state->acc_dec);   
		consumed = (int)(state->bitStream.pBsCurByte - state->bitStream.pBsBuffer);
		//dlog("called Decode_AAC()\n");
		//dlog("pBsBuffer:     %x\n", (int)state->bitStream.pBsBuffer);
		//dlog("pBsCurByte:    %x\n", (int)state->bitStream.pBsCurByte);
		//dlog("bsByteLen:     %d\n", (int)state->bitStream.bsByteLen);
		//dlog("bsCurBitOffset:%d\n", (int)state->bitStream.bsCurBitOffset);
		//dlog("consumed:      %d\n", (int)consumed);
		state->bitStream.bsByteLen -= consumed;
		if( state->bitStream.bsByteLen <= 0 )
		{
			state->bitStream.bsByteLen  = 0;
			state->bitStream.pBsCurByte = state->bitStream.pBsBuffer;
		}
		
		switch( err )
		{
			case IPP_STATUS_FRAME_COMPLETE:	
				dlog("IPP_STATUS_FRAME_COMPLETE\n");
				state->frameNum++;
			
 				dlog("state->sound.sndSampleRate: %d, state->sound.sndChannelNum: %d\n", state->sound.sndSampleRate, state->sound.sndChannelNum);
                if( state->outputSampleRate != state->sound.sndSampleRate )
                    state->outputSampleRate = state->sound.sndSampleRate;
                
				this_wav_size = state->sound.sndLen;
				err = FskMemPtrRealloc(wav_size + this_wav_size, &wav_data);
				BAIL_IF_ERR( err );		
				
				FskMemCopy(wav_data + wav_size, state->sound.pSndFrame, this_wav_size);
				wav_size += this_wav_size;
				dlog("added up wav_size: %d, this_wav_size: %d\n", wav_size, this_wav_size);
				break;
				
			case IPP_STATUS_BUFFER_UNDERRUN:
			case IPP_STATUS_SYNCNOTFOUND_ERR:
			case IPP_STATUS_FRAME_ERR:
				dlog("IPP_STATUS_BUFFER_UNDERRUN/IPP_STATUS_SYNCNOTFOUND_ERR, thisFrameSize: %d, next frame_idx: %d\n", thisFrameSize, frame_idx);
				if( thisFrameSize == 0 ) 
				{
					dlog("frame used up, done!\n");
					done = 1;
					break;
				}
				
				dlog("copying this frame, thisFrameSize: %d\n", thisFrameSize);
				COPY_A_FRAME_DATA
				dlog( "####preparing next frame data, frame_idx : %d\n", (int)frame_idx); 
				PREPARE_A_FRAME_DATA
				break;
				
			//case IPP_STATUS_FRAME_ERR:
			//	dlog("IPP_STATUS_FRAME_ERR\n");
			//	dlog("Skip this frame or other process here\n");
			//	break;
			case IPP_STATUS_NOTSUPPORTED:
				dlog("IPP_STATUS_NOTSUPPORTED\n");
				dlog(" The bitstream is not supported...\n");
				done = 1;
				break;
			case IPP_STATUS_NOMEM_ERR:
				dlog("IPP_STATUS_NOMEM_ERR\n");
				dlog(" Memory allocation failed\n");			    
				done = 1;
				break;				
			case IPP_STATUS_BS_END:
				dlog("IPP_STATUS_BS_END\n");
				dlog(" This frame comes to end for the first ID.\n");
				done = 1;
				break;
			default:
				done = 1;
				break;
		}
	}
	
bail:
	dlog("got err: %d, but reset it so that we keep going", (int)err ); 
	err = kFskErrNone;
	
	if( wav_size == 0 && wav_data != NULL )
	{
		dlog("xxxdisposing wav_data\n" ); 
		FskMemPtrDisposeAt( (void **)&wav_data );
		wav_data = NULL;
	}
	
    /*
    {
        int samplerate	= 0;
        dlog("calling IPPAC_AAC_GET_SAMPLERATE\n");
        DecodeSendCmd_AACAudio(IPPAC_AAC_GET_SAMPLERATE, &samplerate, NULL, state->acc_dec);
        if( samplerate != 0 )
        {
            dlog("got output samplerate: %d\n", samplerate );
            state->outputSampleRate = samplerate;
        }
    }
    */
    
	dlog("============>wav_size:%d\n", wav_size ); 
	*samplesSize = wav_size;
	*samples 	 = (void *)wav_data;

#ifdef DUMP_WAV	
	{
		static FILE *fff = NULL;
		if( fff == NULL )
		{
			dlog( "creating dump wav file, smplerate: %d, channel_total: %d\n", state->outputSampleRate, state->outputChannelCount); 
			err = wav_create_for_write( "/home/bnie/dump.wav", state->outputSampleRate, state->outputChannelCount, &fff );
			if( fff == NULL )
			{
				dlog( "file creating failed!!!\n"); 
			}
		}
		
		if( fff != NULL && wav_size != 0 )
		{
			int sample_total = wav_size/(2*state->outputChannelCount);
			dlog( "dumping wav samples: sample_total: %d\n", sample_total); 
			wav_write( (short *)wav_data, state->outputChannelCount, sample_total, fff );
		}
	}
#endif
	
	return err;
}

FskErr aacDecodeDiscontinuity(void *stateIn, FskAudioDecompress deco)
{
	dlog( "/**********************************************************/\n"); 
	dlog( "into aacDecodeDiscontinuity()\n"); 
#if 1	
	//FskAACDecode state = (FskAACDecode)stateIn;
	//if( PVMP4AudioDecoderResetBuffer_func != NULL )
	//	PVMP4AudioDecoderResetBuffer_func(state->mDecoderBuf);
	
	return kFskErrNone;
#else
	return 0;
#endif
	
}


static FskErr aacDecodeGetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAACDecode state = (FskAACDecode)stateIn;
	UInt32 sampleRate = 0;

	dlog( "/**********************************************************/\n"); 
	dlog("into aacDecodeGetSampleRate(), outputSampleRate: %d\n", state->outputSampleRate ); 
	
	if(state != NULL && state->outputSampleRate != 0 )
	{
		sampleRate = state->outputSampleRate;
	}
	else
	{
		err = kFskErrOperationFailed;
		goto bail;
	}
		
	dlog("setting sampleRate: %d\n", (int)sampleRate ); 
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

	dlog( "/**********************************************************/\n"); 
	dlog("into aacDecodeGetChannelCount(), outputChannelCount: %d\n", state->outputChannelCount ); 
	
	
	if( state != NULL && state->outputChannelCount != 0 )
	{
		channelCount = state->outputChannelCount;
	}
	else
	{
		err = kFskErrOperationFailed;
		goto bail;
	}
	
	dlog("setting channelCount: %d\n", (int)channelCount ); 
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

	dlog( "/**********************************************************/\n"); 
	dlog("into aacDecodeGetCanChangeSampleRate(), canChangeSampleRate: %d\n", state->canChangeSampleRate ); 
	
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

	dlog( "/**********************************************************/\n"); 
	dlog("into aacDecodeSetCanChangeSampleRate(), canChangeSampleRate: %d\n", canChangeSampleRate ); 
	
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

	dlog( "/**********************************************************/\n"); 
	dlog("into aacDecodeGetCanChangeChannelCount(), canChangeChannelCount: %d\n", state->canChangeChannelCount ); 
	
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

	dlog( "/**********************************************************/\n"); 
	dlog("into aacDecodeSetCanChangeChannelCount(), canChangeChannelCount: %d\n", canChangeChannelCount ); 
	
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
