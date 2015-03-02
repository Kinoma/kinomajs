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
#define __FSKMP3DECODE_PRIV__

#include "kinomamp3decipp.h"

#include "codecAC.h"
#include "misc.h"
#include "ippdefs.h"

#if SUPPORT_INSTRUMENTATION
FskInstrumentedTypeRecord gkinomamp3decippTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "kinomamp3decipp"};
//FskInstrumentedSimpleType(kinomamp3decipp, kinomamp3decipp);
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


static FskErr mp3DecodeGetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3DecodeGetChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord mp3DecodeProperties[] = 
{
//	{kFskMediaPropertySampleRate,				kFskMediaPropertyTypeInteger,		mp3DecodeGetSampleRate,				NULL},
//	{kFskMediaPropertyChannelCount,				kFskMediaPropertyTypeInteger,		mp3DecodeGetChannelCount,			NULL},
	{kFskMediaPropertyUndefined,				kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};

typedef struct FskMP3DecodeRecord FskMP3DecodeRecord;
typedef struct FskMP3DecodeRecord *FskMP3Decode;

struct FskMP3DecodeRecord 
{
	int canChangeSampleRate;
	int canChangeChannelCount;
	int	outputSampleRate;
	int	outputChannelCount;
	
	int							frameNum;		
	
	IppBitstream				bitStream;				//MP3 decoder input structure
	IppSound					sound;					//MP3 decoder output structure
	MiscGeneralCallbackTable   *pCallBackTable;			//Misc callback table
	
	int							input_format;
	char						*pInputBuf;		
	char						*pOutBuf;			
	
	void						*acc_dec;
	
};

FskErr mp3DecodeCanHandle(UInt32 format, const char *formatStr, Boolean *canHandle)
{

	dlog( "into mp3DecodeCanHandle, format: %d,  formatStr: %s\n", (int)format, formatStr); 
	*canHandle = (kFskAudioFormatMP3 == format) || (0 == FskStrCompare(formatStr, "format:mp3")) || (0 == FskStrCompare(formatStr, "x-audio-codec/mp3"));

	return kFskErrNone;
}

FskErr mp3DecodeNew(FskAudioDecompress deco, UInt32 format, const char *mime)
{
	
	FskErr err = kFskErrNone;
	FskMP3Decode state;

	err = FskMemPtrNewClear(sizeof(FskMP3DecodeRecord), (FskMemPtr *)&state);
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

FskErr mp3DecodeDispose(void *stateIn, FskAudioDecompress deco)
{
	FskMP3Decode state = (FskMP3Decode)stateIn;

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
			DecoderFree_MP3(&state->acc_dec);
		
		dlog( "freeing state->pCallBackTable: %x\n", (int)state->pCallBackTable); 
		if( state->pCallBackTable != NULL ) 
			IPP_MemFree ((void **)&state->pCallBackTable);

		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}



// sloppy implementation
const int    omx_MPEGAFequency[2][4] =
{{22050, 24000, 16000},{44100, 48000, 32000, 0}};

const int omx_MPEGABitrate[2][3][15] = {
	/* MPEG 2 */
	{
		/* Layer 1  */
		{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176,  192, 224, 256},
		/* Layer 2  */
		{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
		/* Layer 3  */
		{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
	}
	,
	/* MPEG 1 */
	{
		/* Layer 1  */
		{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
		/* Layer 2  */
		{0,32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
		/* Layer 3  */
		{0,32, 40, 48, 56, 64, 80, 96,  112, 128, 160, 192, 224, 256, 320}
	}
};

const int omx_MPEGAChannels[]={2,2,2,1};

#define CheckUInt32(data, i)   (long)((( header[i]<<24)& 0xff000000) | ((header[i+1]<<16) & 0x00ff0000) | ( (header[i+2]<<8) & 0x0000ff00) | (header[i+3]&0x000000ff) );


int scan_mp3_header(unsigned char *header, int size, int *sampleRate, int *channelCount)
{
	unsigned char   code1;
	unsigned long   code2;
	
	//long stream_type1;
	//long stream_type2;
	int m_MPEGASyncWord;
	int sample_frequency;
	int channels;
	int bitPerSample;
	int bitrate;
	
	code1 = header[0];
	code2 = CheckUInt32( header, 0 );
	
	if( !( ((code2 & 0xfff00000) == 0xfff00000) ||
		  ((code2 & 0xffffff00) == 0x49443300) ||
		  ((code2 == 0)) ) )
		;//return -1;
	
	{
		unsigned char aux_byte1;
		unsigned char aux_byte2;
		unsigned char aux_byte3;
		int id, la, br, fr, md;
		
		aux_byte1 = header[1];
		aux_byte2 = header[2];
		aux_byte3 = header[3];
		
		id  = (aux_byte1&0x08)>>3;
		la  = ((aux_byte1&0x06)>>1);
		br  = (aux_byte2&0xf0) >> 4;
		fr  = (aux_byte2&0x0c)>>2;
		md  = (aux_byte3&0xc0)>>6;
		
		if((la == 0) && (id == 1))
			return -1;
		
		//AudioStreamType mpeg_type[] = {MPEG2_AUDIO, MPEG1_AUDIO};
		
		//stream_type1		= MPEG1_PURE_AUDIO_STREAM;
		//stream_type2		=  (AudioStreamType) (mpeg_type[id] | (1<<((4-la)+3)));
		m_MPEGASyncWord     = 0x0000fffa;
		
		sample_frequency   = omx_MPEGAFequency[id][fr];
		channels           = omx_MPEGAChannels[md];
		bitPerSample       = 16;
		bitrate            = omx_MPEGABitrate[id][la][br];
	}
	
	*sampleRate   = sample_frequency;
	*channelCount = channels;
	
	return 0;
}


FskErr mp3DecodeDecompressFrames(void *stateIn, FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *samplesSize)
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
	state->bitStream.pBsCurByte = state->bitStream.pBsBuffer;

	FskMP3Decode 	state = (FskMP3Decode)stateIn;
	int				frame_idx      = 0;
	int				thisFrameSize  = 0;
	unsigned char 	*thisFrameData = NULL;
	unsigned char	*wav_data	   = NULL;
	int				wav_size	   = 0;
	int				this_wav_size  = 0;
	int				done = 0;
	FskErr 			err  = 0;

	dlog( "/**********************************************************/\n"); 
	dlog( "into mp3DecodeDecompressFrames: frameCount: %d\n", (int)frameCount); 
	
	if( frameCount == 0 || data == NULL || dataSize == 0 )
	{
		//***bnie: flush out possible cached samples
		goto bail;
	}
	
	//park at next current frame data
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
		//unsigned char	*esds				= (unsigned char *)deco->formatInfo;
		//int				esds_size			= deco->formatInfoSize;
		//int				samplerate_index	= 0;
		int				channal_total		= 0;
		int				samplerate			= 0;

		err = scan_mp3_header(thisFrameData, thisFrameSize,  &samplerate, &channal_total);
		BAIL_IF_ERR( err );
		
		dlog( "called parse_mp3_header, samplerate: %d, channal_total: %d\n", (int)samplerate, (int)channal_total); 
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
		IPP_MemMalloc((void **)&state->pInputBuf, INPUT_BUF_SIZE_MP3+1, 2);	
		state->bitStream.pBsBuffer		= (Ipp8u *)state->pInputBuf;
		state->bitStream.pBsCurByte		= state->bitStream.pBsBuffer;
		state->bitStream.bsCurBitOffset =0;
	
		dlog("copying this frame, thisFrameSize: %d\n", thisFrameSize);
		COPY_A_FRAME_DATA
		dlog( "####preparing next frame data, frame_idx : %d\n", (int)frame_idx); 
		PREPARE_A_FRAME_DATA		
		
		IPP_MemMalloc((void **)&state->pOutBuf, OUTPUT_BUF_SIZE_MP3 + 4, 4);
		dlog("read input file for initialization\n");
		
		state->sound.pSndFrame = (Ipp16s*)state->pOutBuf;
		state->sound.sndLen =0;		
		
		dlog("calling DecoderInitAlloc_AAC\n");
		err = DecoderInitAlloc_MP3( &state->bitStream, state->pCallBackTable, &state->sound, &state->acc_dec );
		if( err != IPP_STATUS_INIT_OK ) 
		{
			dlog("calling DecoderInitAlloc_MP3 failed, err: %d\n", (int)err);
			err = IPP_FAIL;
			goto bail;
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
		
		dlog("calling Decode_MP3\n");
		err = Decode_MP3(&state->bitStream, &state->sound, state->acc_dec);   
		dlog("called Decode_MP3, err: %d\n", err);
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
			
				this_wav_size = state->sound.sndLen;
				err = FskMemPtrRealloc(wav_size + this_wav_size, &wav_data);
				BAIL_IF_ERR( err );		
				
				FskMemCopy(wav_data + wav_size, state->sound.pSndFrame, this_wav_size);
				wav_size += this_wav_size;
				dlog("added up wav_size: %d, this_wav_size: %d\n", wav_size, this_wav_size);
				break;
				
			case IPP_STATUS_FRAME_UNDERRUN:	
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
				dlog(" The bitstream is not supported, programe terminate\n");
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
				dlog("default, done!\n");
				done = 1;
				break;
		}
	}
	
bail:
	dlog("got err: %d, but reset it so that we keep going\n", (int)err ); 
	err = kFskErrNone;
	
	if( wav_size == 0 && wav_data != NULL )
	{
		dlog("xxxdisposing wav_data\n" ); 
		FskMemPtrDisposeAt( (void **)&wav_data );
		wav_data = NULL;
	}
	
	*samplesSize = wav_size;
	*samples 	 = (void *)wav_data;
	dlog("============>*samplesSize:%d, *samples: %x\n", (int)*samplesSize, (int)(*samples) ); 
	
#ifdef DUMP_WAV	
	{
		static FILE *fff = NULL;
		if( fff == NULL )
		{
			dlog( "creating dump wav file, smplerate: %d, channel_total: %d\n", state->outputSampleRate, state->outputChannelCount); 
			err = wav_create_for_write( "/sdcard/dump.wav", state->outputSampleRate, state->outputChannelCount, &fff );
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

FskErr mp3DecodeDiscontinuity(void *stateIn, FskAudioDecompress deco)
{
	FskMP3Decode state = (FskMP3Decode)stateIn;

	dlog( "/**********************************************************/\n"); 
	dlog( "into mp3DecodeDiscontinuity()\n"); 

#if 1
	if( state != NULL ) 
	{
		dlog( "freeing state->pInputBuf: %x\n", (int)state->pInputBuf); 
		if( state->pInputBuf != NULL )
		{
			IPP_MemFree((void **)&state->pInputBuf);
			state->pInputBuf = NULL;
		}
		
		dlog( "freeing state->pOutBuf: %x\n", (int)state->pOutBuf); 
		if( state->pOutBuf != NULL )
		{
			IPP_MemFree((void **)&state->pOutBuf);
			state->pOutBuf = NULL;
		}
		
		dlog( "freeing state->acc_dec: %x\n", (int)state->acc_dec); 
		if( state->acc_dec != NULL )
		{
			DecoderFree_MP3(&state->acc_dec);
			state->acc_dec = NULL;
		}
		
		dlog( "freeing state->pCallBackTable: %x\n", (int)state->pCallBackTable); 
		if( state->pCallBackTable != NULL ) 
		{
			IPP_MemFree ((void **)&state->pCallBackTable);
			state->pCallBackTable = NULL;
		}
	}
#endif
	return kFskErrNone;
}

#if 0
static FskErr mp3DecodeGetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskMP3Decode state = (FskMP3Decode)stateIn;
	UInt32 sampleRate = 0;

	
	dlog( "/**********************************************************/\n"); 
	dlog("into mp3DecodeGetSampleRate, outputSampleRate: %d\n", state->outputSampleRate ); 
	
	if (state && state->outputSampleRate != 0)
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


static FskErr mp3DecodeGetChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskMP3Decode state = (FskMP3Decode)stateIn;
	UInt32 channelCount = 0;

	dlog( "/**********************************************************/\n"); 
	dlog("into mp3DecodeGetChannelCount, outputChannelCount: %d\n", state->outputChannelCount ); 
	
	
	if (state && state->outputChannelCount != 0)
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
#endif
