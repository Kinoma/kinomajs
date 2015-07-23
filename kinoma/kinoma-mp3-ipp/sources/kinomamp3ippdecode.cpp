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
#define __FSKMP3DECODE_PRIV__
#include "kinomamp3ippdecode.h"

#include "kinoma_ipp_lib.h"

#include "umc_audio_codec.h"
#include "umc_mp3_dec.h"

using namespace UMC;

#if __cplusplus
	extern "C" {
#endif

typedef struct FskMP3DecodeRecord FskMP3DecodeRecord;
typedef struct FskMP3DecodeRecord *FskMP3Decode;

struct FskMP3DecodeRecord 
{
	MP3DecoderInt *dec;
};

/*
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
*/


FskErr mp3DecodeCanHandle(UInt32 format, const char *formatStr, Boolean *canHandle)
{
	*canHandle = (kFskAudioFormatMP3 == format) || (0 == FskStrCompare(formatStr, "format:mp3")) || (0 == FskStrCompare(formatStr, "x-audio-codec/mp3"));
	return kFskErrNone;
}

//#define DUMP_INPUT_SOUND
//#define DUMP_OUTPUT_SOUND

#ifdef DUMP_INPUT_SOUND
FILE *sndFile = NULL;
#endif

#ifdef DUMP_OUTPUT_SOUND
FILE *wav_file = NULL;

#define LongB2N(a) ( a[0]<<24|a[1]<<16|a[2]<<8|a[3] )


#define ShortSwapEndian( s )  ( ( s&0x00ff )<<8 | ( ( s&0xff00 )>> 8 ) )
#define LongSwapEndian( s )  (  ( s&0x000000ff )<<24 | ( s&0x0000ff00 )<<8 | ( s&0x00ff0000 )>>8 | ( s&0xff000000 )>>24 )

#ifdef macintosh 
	#define ShortN2L(s)  ShortSwapEndian(s)
	#define LongN2L(s)   LongSwapEndian(s)
	#define LongL2N(s)   LongSwapEndian(s)
	#define ShortL2N(s)  ShortSwapEndian(s)
	#define LongN2B(s)   (s)
#else
	#define ShortN2L(s)  (s)
	#define LongN2L(s)   (s)
	#define LongL2N(s)   (s)
	#define ShortL2N(s)  (s)
	#define LongN2B(s)   LongSwapEndian(s)
#endif	


static void Debug_Dump_Sound( short *soundDataPtr, long numChannels, long sampleCount, FILE *soundFile )
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


static void Debug_CreateSoundFileForWrite( long sampleRate, long numChannels, FILE **inputFile, char* filename )
{
	FILE	*soundFile = *inputFile;
//	char	string[512];
//	char	filename[512];
//	FILE	*stream;
//	char	*ptr;
	
	if( inputFile == NULL )
		return;

//	sprintf(filename, "C:\\MP3Output.wav" );
//	ptr = filename;

	soundFile = fopen(filename,"wb");	
	*inputFile = soundFile;
	if( soundFile == NULL )
	{
		printf("cannot open dump file for write (%s)\n", filename );
		return;
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

		fourBytes = 1024*1024*10;							//total size not known yet
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
	}
	
	fflush( soundFile );
}
#endif

FskErr mp3DecodeNew(FskAudioDecompress deco, UInt32 format, const char *mime)
{
	FskErr err = kFskErrNone;
	FskMP3Decode state;

	//***
#ifdef DUMP_INPUT_SOUND
	{
		char			filename[512];
		sprintf(filename, "C:\\test_mp3\\myMP3.box" );
		sndFile = fopen(filename,"wb");	
	}
#endif

	err = FskMemPtrNewClear(sizeof(FskMP3DecodeRecord), (FskMemPtr *)&state);
	if (kFskErrNone != err) 
		goto bail;
	
	kinoma_ipp_lib_mp3_init(FSK_ARCH_AUTO);

	deco->state = state;
	state->dec = new MP3DecoderInt();
	if( state->dec == NULL )
	{
		err = kFskErrMemFull;
		goto bail;
	}
	
	state->dec->Init(NULL);

	deco->outputFormat		 = kFskAudioFormatPCM16BitLittleEndian;
	deco->outputChannelCount = deco->inputChannelCount;

bail:
	return err;
}

FskErr mp3DecodeDispose(void *stateIn, FskAudioDecompress deco)
{
	FskMP3Decode state = (FskMP3Decode)stateIn;

	if (state) 
	{
		delete state->dec;
		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}

#define kMaxWavSize    (1024 * 8)
FskErr mp3DecodeDecompressFrames(void *stateIn, FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *samplesSize)
{
	FskMP3Decode 	state = (FskMP3Decode)stateIn;
	unsigned char	*wav_data;
	unsigned char 	*thisFrameData 	= (unsigned char *)data;
	unsigned char 	*thisWavData;
	//UMC::MediaData  *input, *output;
	UMC::MediaData_V51 input;
	UMC::MediaData_V51 output;

	long			i;
	int 			totalBytes = 0;
	int				sourceBytes = 0;
	FskErr 			err;

	err = FskMemPtrNew(kMaxWavSize * frameCount, (FskMemPtr *)&wav_data);
	if (err) return err;

	thisWavData 	= wav_data;

	//***
#ifdef DUMP_INPUT_SOUND
	{
		long i;
		unsigned char *thisFrame = (unsigned char*)data;

		for( i = 0; i < frameCount; i++ )
		{
			long thisFrameSize = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[i];

			fwrite( &thisFrameSize,  4, 1, sndFile);

			fwrite( thisFrame,  thisFrameSize, 1, sndFile);
			fflush( sndFile );

			thisFrame += thisFrameSize;
		}
	}
#endif

	//input  = new MediaData(0);
	//output = new MediaData(0);
	//frameCount = 1;

	for( i = 0; i < (int)frameCount; i++ )
	{
		long thisFrameSize = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[i];
		long thisWavSize;
		UMC::Status umcRes = UMC::UMC_OK;

		input.SetBufferPointer( thisFrameData, thisFrameSize );
        input.SetTime(0);
		
		thisFrameData += thisFrameSize;
		sourceBytes   += thisFrameSize;

		output.SetBufferPointer(thisWavData, kMaxWavSize);		
		output.SetDataSize(0);		
		umcRes = state->dec->GetFrame( &input, &output );
		if( umcRes != UMC_NOT_FIND_SYNCWORD && umcRes != UMC::UMC_OK )
			break;
		
		thisWavSize  = output.GetDataSize();

#ifdef DUMP_OUTPUT_SOUND
		{
			int sample_rate = 44100;
			int channel_count = 2;
			
			if( wav_file == NULL )
			{
				char wav_path[512];

				sprintf(wav_path, "\\Kinoma\\mp3_dump_wince.wav" );
				//sprintf(wav_path, "c:\\test_mp3\\mp3_dump_win32.wav" );
				Debug_CreateSoundFileForWrite( sample_rate, channel_count, &wav_file,wav_path);
			}
			
			Debug_Dump_Sound( (short *)thisWavData, channel_count, thisWavSize / 2 / channel_count, wav_file );
		}
#endif


		totalBytes  += thisWavSize;
		thisWavData += thisWavSize;

		if( sourceBytes >= (int)dataSize )
			break;
	}
	
	if( totalBytes == 0 )
	{
		FskMemPtrDisposeAt( (void **)&wav_data );
	}
	
//	fprintf( stderr, "============>totalBytes:%d\n", totalBytes );

	*samplesSize = totalBytes;
	*samples 	 = (void *)wav_data;
//bail:
	return err;
}

FskErr mp3DecodeDiscontinuity(void *stateIn, FskAudioDecompress deco)
{
	FskMP3Decode state = (FskMP3Decode)stateIn;
	UMC::Status umcRes = UMC::UMC_OK;
	
	//FskDebugStr("mp3DecodeDiscontinuity");
	umcRes = state->dec->Reset();
	
	return kFskErrNone;
}

#ifdef __cplusplus
}
#endif

