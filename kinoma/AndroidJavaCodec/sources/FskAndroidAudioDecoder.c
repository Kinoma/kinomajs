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
#define __FSKTHREAD_PRIV__
#define __FSKAndroidJavaAudioDecode_PRIV__

#include "FskAndroidAudioDecoder.h"
#include <jni.h>

#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
FskInstrumentedTypeRecord gAndroidJavaAudioDecoderTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "FskAndroidAudioDecoder"};
#define dlog(...)  do { FskInstrumentedTypePrintfDebug  (&gAndroidJavaAudioDecoderTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define dlog(...)
#endif

#define kMaxWavSize    (1024 * 8)
#define FILEREAD_MAX_LAYERS 2
#define AAC_DEC_OK 0

static int MAX_INPUT_SIZE = 8192; //Limit of Input Buffer

//#define Performance_Profiling
#ifdef Performance_Profiling
#include <sys/time.h>
#endif

static FskErr AndroidJavaAudioDecodeGetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr AndroidJavaAudioDecodeGetChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr AndroidJavaAudioDecodeGetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr AndroidJavaAudioDecodeSetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr AndroidJavaAudioDecodeGetCanChangeChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr AndroidJavaAudioDecodeSetCanChangeChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord AndroidJavaAudioDecodeProperties[] = 
{
	{kFskMediaPropertySampleRate,				kFskMediaPropertyTypeInteger,		AndroidJavaAudioDecodeGetSampleRate,			NULL},
	{kFskMediaPropertyChannelCount,				kFskMediaPropertyTypeInteger,		AndroidJavaAudioDecodeGetChannelCount,			NULL},
	{kFskMediaPropertyCanChangeSampleRate,		kFskMediaPropertyTypeBoolean,		AndroidJavaAudioDecodeGetCanChangeSampleRate,	AndroidJavaAudioDecodeSetCanChangeSampleRate},
	{kFskMediaPropertyCanChangeChannelCount,	kFskMediaPropertyTypeBoolean,		AndroidJavaAudioDecodeGetCanChangeChannelCount,	AndroidJavaAudioDecodeSetCanChangeChannelCount},
	{kFskMediaPropertyFormat,					kFskMediaPropertyTypeString,		NULL,								NULL },
	{kFskMediaPropertyUndefined,				kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};

typedef struct 
{
	int canChangeSampleRate;
	int canChangeChannelCount;
	int	outputSampleRate;
	int	outputChannelCount;
	
	//env JNI
	JNIEnv				*jniEnv;
	//MediaCodecCore
	jclass				MediaCodecCore;
	jobject				mMediaCodecCore;
	jmethodID			F_AudioCheckInputBufferAvail_ID;
	jmethodID			F_MediaQueueDataToInput_ID;
    jmethodID			F_MediaQueueNullToInput_ID;
	jmethodID			F_MediaQueueInputBufferSize_ID;
	jmethodID			F_AudioDeQueueOutputBuffer_ID;
	jmethodID			F_MediaReleaseOutputBuffer_ID;
	jmethodID			F_MediaSetMediaCodecInstance_ID;
	jmethodID			F_CodecFormatAvailable_ID;
	jmethodID			F_flush_ID;
	jmethodID			F_close_ID;
	jfieldID			V_CODEC_STATUS_ID;
	jfieldID			V_CROP_LT_ID;
	//MediaCodec Android
	jclass				C_MediaCodec;
	jobject				mC_MediaCodec;
	jmethodID			F_createDecoderByType_ID;
	jmethodID			F_getOutputFormat_ID;
	jmethodID			F_configure_ID;
	jmethodID			F_start_ID;
	//MediaFormat Android
	jstring				mimeType;
	jobject				mC_MediaFormat;
	jmethodID			F_getInteger_ID;
	//Main Loop
	int					flags;
#if SUPPORT_INSTRUMENTATION
    int                 frames_in;
    int                 frames_out;
#endif
    int                 WavSize;

#ifdef Performance_Profiling
	int					total_time;
	double				avege_time;
#endif
} FskAndroidJavaAudioDecodeRecord;

/* Functions for JNI */

static int g_mime;
enum frame_type {
	MIME_AAC = 0,
	MIME_MP3 = 1,
	MIME_AMR = 2
};

enum DECODE_FRAME_TYPE{
	/* frame type */
	CONFIGURE_FLAG_ENCODE     = 0, //decoder only
	BUFFER_FLAG_SYNC_FRAME    = 1, //sync frame
	BUFFER_FLAG_CODEC_CONFIG  = 2, //configure data
	BUFFER_FLAG_END_OF_STREAM = 4  //end of frame
};

static void chartoByteArray(JNIEnv* env, jbyteArray jBuf, unsigned char* buf, int nOutSize)
{
	(*env)->SetByteArrayRegion(env, jBuf, 0, nOutSize, (jbyte*)buf);
}

static void GetByteArrayLength(JNIEnv* env, jbyteArray jarry, int *size)
{
    *size = (*env)->GetArrayLength(env, jarry);
}

static char *ByteArraytochar(JNIEnv* env, jbyteArray jarry)
{
	jbyte *arrayBody = (*env)->GetByteArrayElements(env, jarry, 0);
	return (char *)arrayBody;
}

extern void Init_JNI_Env();
extern jclass gMediaCodecCoreClass;
static int GetMediaCodecInstance(JNIEnv *jniEnv, void *stateIn)
{
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	jclass obj_class = state->MediaCodecCore;
	
	if(obj_class == NULL) {
		return -1;
	}
	
	jmethodID construction_id = (*jniEnv)->GetMethodID(jniEnv, obj_class, "<init>", "()V");
	if (construction_id == 0) {
		return -1;
	}
	
	jobject obj = (*jniEnv)->NewObject(jniEnv, obj_class, construction_id);
	state->mMediaCodecCore = (*jniEnv)->NewGlobalRef(jniEnv, obj);
	(*jniEnv)->DeleteLocalRef(jniEnv, obj);
	if (state->mMediaCodecCore == NULL) {
		return -2;
	}
	
	return 0;
}

static int InitJNIMediaCodecSysm(void *stateIn)
{
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	JNIEnv *jniEnv = state->jniEnv;
	
	dlog( "into InitJNIMediaCodecSysm!\n" );
	
	/* MediaCodec Initialization */
	if(state->C_MediaCodec == NULL) {
		jclass cls = (*jniEnv)->FindClass(jniEnv, "android/media/MediaCodec");
		state->C_MediaCodec = (*jniEnv)->NewGlobalRef(jniEnv, cls);
		(*jniEnv)->DeleteLocalRef(jniEnv, cls);
		if(state->C_MediaCodec == NULL){
			dlog( "Mapping function C_MediaCodec error!\n" );
			return -1;
		}
		dlog( "Mapping class MediaCodec OK!\n" );
	}
	/* function createDecoderByType */
	if (state->F_createDecoderByType_ID == NULL) {
		state->F_createDecoderByType_ID = (*jniEnv)->GetStaticMethodID(jniEnv, state->C_MediaCodec, "createDecoderByType",
																	   "(Ljava/lang/String;)Landroid/media/MediaCodec;");
		if (state->F_createDecoderByType_ID == NULL) {
			dlog( "Mapping function F_createDecoderByType_ID error!\n" );
			return -2;
		}
		dlog( "Mapping function createDecoderByType OK!\n" );
	}
	/* function configure */
	if (state->F_getOutputFormat_ID == NULL) {
		state->F_getOutputFormat_ID = (*jniEnv)->GetMethodID(jniEnv, state->C_MediaCodec, "getOutputFormat",
															 "()Landroid/media/MediaFormat;");
		if (state->F_getOutputFormat_ID == NULL) {
			return -3;
		}
		dlog( "Mapping function getOutputFormat OK!\n" );
	}
	/* function configure */
	if (state->F_configure_ID == NULL) {
		state->F_configure_ID = (*jniEnv)->GetMethodID(jniEnv, state->C_MediaCodec, "configure",
													   "(Landroid/media/MediaFormat;Landroid/view/Surface;Landroid/media/MediaCrypto;I)V");
		if (state->F_configure_ID == NULL) {
			dlog( "Mapping function F_configure_ID error!\n" );
			return -4;
		}
		dlog( "Mapping function configure OK!\n" );
	}
	/* function start */
	if (state->F_start_ID == NULL) {
		state->F_start_ID = (*jniEnv)->GetMethodID(jniEnv, state->C_MediaCodec, "start", "()V");
		if (state->F_start_ID == NULL) {
			dlog( "Mapping class F_start_ID error!\n" );
			return -5;
		}
		dlog( "Mapping function start OK!\n" );
	}
	
	return 0;
}

static int InitJNIMediaCodecCore(void *stateIn)
{
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	JNIEnv *jniEnv = state->jniEnv;
	
	dlog( "into InitJNIMediaCodecCore!\n" );
	
	state->MediaCodecCore = gMediaCodecCoreClass;
	
	if(state->MediaCodecCore == NULL) {
        dlog( "Mapping class MediaCodecCore error!\n" );
		return -1;
	}
	
	if (state->mMediaCodecCore == NULL) {
		if (GetMediaCodecInstance(jniEnv, stateIn) != 0) {
			dlog( "Mapping class mMediaCodecCore error!\n" );
			return -2;
		}
		dlog( "Initialize object mMediaCodecCore OK!\n" );
	}

    /* function MediaQueueInputBufferSize */
	if (state->F_MediaQueueInputBufferSize_ID == NULL) {
		state->F_MediaQueueInputBufferSize_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "MediaQueueInputBufferSize","([BI)I");
		if (state->F_MediaQueueInputBufferSize_ID == NULL) {
			dlog( "Mapping function F_MediaQueueInputBufferSize_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_MediaQueueInputBufferSize_ID OK!\n" );
	}

	/* function AudioDeQueueOutputBuffer */
	if (state->F_AudioDeQueueOutputBuffer_ID == NULL) {
		state->F_AudioDeQueueOutputBuffer_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "AudioDeQueueOutputBuffer","()Ljava/nio/ByteBuffer;");
		if (state->F_AudioDeQueueOutputBuffer_ID == NULL) {
			dlog( "Mapping function F_AudioDeQueueOutputBuffer_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_AudioDeQueueOutputBuffer_ID OK!\n" );
	}

    /* function MediaReleaseOutputBuffer */
	if (state->F_MediaReleaseOutputBuffer_ID == NULL) {
		state->F_MediaReleaseOutputBuffer_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "MediaReleaseOutputBuffer","()V");
		if (state->F_MediaReleaseOutputBuffer_ID == NULL) {
			dlog( "Mapping function F_MediaReleaseOutputBuffer_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_MediaReleaseOutputBuffer_ID OK!\n" );
	}

	/* function AudioCheckInputBufferAvail */
	if (state->F_AudioCheckInputBufferAvail_ID == NULL) {
		state->F_AudioCheckInputBufferAvail_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "AudioCheckInputBufferAvail","(I)I");
		if (state->F_AudioCheckInputBufferAvail_ID == NULL) {
			dlog( "Mapping function F_AudioCheckInputBufferAvail_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_AudioCheckInputBufferAvail_ID OK!\n" );
	}

	/* function MediaQueueDataToInput */
	if (state->F_MediaQueueDataToInput_ID == NULL) {
		state->F_MediaQueueDataToInput_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "MediaQueueDataToInput","([BII)I");
		if (state->F_MediaQueueDataToInput_ID == NULL) {
			dlog( "Mapping function F_MediaQueueDataToInput_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_MediaQueueDataToInput_ID OK!\n" );
	}

    /* function MediaQueueNullToInput */
	if (state->F_MediaQueueNullToInput_ID == NULL) {
		state->F_MediaQueueNullToInput_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "MediaQueueNullToInput","([BII)I");
		if (state->F_MediaQueueNullToInput_ID == NULL) {
			dlog( "Mapping function F_MediaQueueNullToInput_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_MediaQueueNullToInput_ID OK!\n" );
	}
	
	/* function close */
	if (state->F_close_ID == NULL) {
		state->F_close_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "close", "(Landroid/media/MediaCodec;)V");
		if (state->F_close_ID == NULL) {
			dlog( "Mapping function F_close_ID error!\n" );
			return -4;
		}
		dlog( "Mapping function close OK!\n" );
	}

    /* function MediaSetMediaCodecInstance */
	if (state->F_MediaSetMediaCodecInstance_ID == NULL) {
		state->F_MediaSetMediaCodecInstance_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "MediaSetMediaCodecInstance", "(Landroid/media/MediaCodec;)V");
		if (state->F_MediaSetMediaCodecInstance_ID == NULL) {
			dlog( "Mapping function F_MediaSetMediaCodecInstance_ID error!\n" );
			return -5;
		}
		dlog( "Mapping function F_MediaSetMediaCodecInstance_ID OK!\n" );
	}
	
	/* function flush */
	if (state->F_flush_ID == NULL) {
		state->F_flush_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "flush", "(Landroid/media/MediaCodec;)V");
		if (state->F_flush_ID == NULL) {
			dlog( "Mapping function F_flush_ID error!\n" );
			return -5;
		}
		dlog( "Mapping function flush OK!\n" );
	}
	
	/* function CodecFormatAvailable */
	if (state->F_CodecFormatAvailable_ID == NULL) {
		state->F_CodecFormatAvailable_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "CodecFormatAvailable","(Ljava/lang/String;I)I");
		if (state->F_CodecFormatAvailable_ID == NULL) {
			dlog( "Mapping function F_CodecFormatAvailable_ID error!\n" );
			return -6;
		}
		dlog( "Mapping function CodecFormatAvailable OK!\n" );
	}
	
	if (state->V_CODEC_STATUS_ID == NULL) {
		state->V_CODEC_STATUS_ID = (*jniEnv)->GetFieldID(jniEnv, state->MediaCodecCore, "codec_status", "I");
		if (state->V_CODEC_STATUS_ID == NULL) {
			dlog( "Mapping field V_CODEC_STATUS_ID error!\n" );
			return -7;
		}
		dlog( "Mapping codec_status OK!\n" );
	}
	
	if (state->V_CROP_LT_ID == NULL) {
		state->V_CROP_LT_ID = (*jniEnv)->GetFieldID(jniEnv, state->MediaCodecCore, "crop_lt", "I");
		if (state->V_CROP_LT_ID == NULL) {
			dlog( "Mapping field V_CROP_LT_ID error!\n" );
			return -8;
		}
		dlog( "Mapping crop_lt OK!\n" );
	}
	
	dlog( "Successful initialize MediaCodecCore class and functions\n" );
	
	return 0;
}


FskErr AndroidJavaAudioDecodeCanHandle(UInt32 format, const char *formatStr, Boolean *canHandle)
{
	
	dlog( "into AndroidJavaAudioDecodeCanHandle\n\n"); 
	*canHandle = (kFskAudioFormatAAC == format) || (0 == FskStrCompare(formatStr, "format:aac")) || (0 == FskStrCompare(formatStr, "x-audio-codec/aac"))/* ||
				 (kFskAudioFormatMP3 == format) || (0 == FskStrCompare(formatStr, "format:mp3")) || (0 == FskStrCompare(formatStr, "x-audio-codec/mp3"))*/;
	
	dlog( "mime %s canHandle = %d \n", formatStr, (int)*canHandle );
	if (*canHandle == 1 && (0 == FskStrCompare(formatStr, "x-audio-codec/aac"))) {
		g_mime = MIME_AAC;
	} else if(*canHandle == 1 && (0 == FskStrCompare(formatStr, "x-audio-codec/mp3"))) {
		g_mime = MIME_MP3;
	}
	
	return kFskErrNone;
}

//FILE *fp = NULL;
FskErr AndroidJavaAudioDecodeNew(FskAudioDecompress deco, UInt32 format, const char *mime)
{
	FskErr err = kFskErrNone;
	FskAndroidJavaAudioDecodeRecord *state;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog( "into AndroidJavaAudioDecodeNew\n\n");
	
	if (g_mime == MIME_AAC && ((NULL == deco->formatInfo) || !deco->formatInfoSize))
		return kFskErrBadData;
	
	err = FskMemPtrNewClear(sizeof(FskAndroidJavaAudioDecodeRecord), (FskMemPtr *)&state);
	if (kFskErrNone != err) 
		goto bail;
	
	/* get thread env */

	deco->state = state;
#if SUPPORT_INSTRUMENTATION
    state->frames_in = state->frames_out = 0;
#endif
    state->WavSize = 0;
	state->canChangeSampleRate	= 1;
	state->canChangeChannelCount= 1;
	state->outputSampleRate		= 0;
	state->outputChannelCount   = 0;

    dlog( "deco->inputSampleRate	= %d \n", deco->inputSampleRate);
    dlog( "deco->inputChannelCount	= %d \n", deco->inputChannelCount );
    dlog( "deco->outputChannelCount	= %d \n", deco->outputChannelCount );
    dlog( "deco->outputFormat	    = %d \n", deco->outputFormat );

	deco->outputFormat		 = kFskAudioFormatPCM16BitLittleEndian;
//	deco->outputChannelCount = deco->inputChannelCount;

#ifdef Performance_Profiling
	state->total_time		= 0;
	state->avege_time		= 0.0;
#endif

//	fp = (FILE *)fopen("/sdcard/jstream.aac", "wb");
	
bail:
	return err;
}

FskErr AndroidJavaAudioDecodeDispose(void *stateIn, FskAudioDecompress deco)
{
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog( "into AndroidJavaAudioDecodeDispose\n\n");
	
	if( state != NULL ) 
	{
		JNIEnv *jniEnv = state->jniEnv;
		
		/* call MediaCodec close */
		if (state->mC_MediaFormat && state->mMediaCodecCore && state->F_close_ID && state->mC_MediaCodec) {
			dlog( "Call MediaCodec close/release ...");
			(*jniEnv)->CallVoidMethod(jniEnv, state->mMediaCodecCore, state->F_close_ID, state->mC_MediaCodec);

			(*jniEnv)->DeleteGlobalRef(jniEnv, state->mMediaCodecCore);
			(*jniEnv)->DeleteGlobalRef(jniEnv, state->C_MediaCodec);
			(*jniEnv)->DeleteGlobalRef(jniEnv, state->mC_MediaCodec);
			(*jniEnv)->DeleteGlobalRef(jniEnv, state->mC_MediaFormat);
			state->MediaCodecCore	= NULL;
			state->mMediaCodecCore	= NULL;
			state->C_MediaCodec		= NULL;
			state->mC_MediaCodec	= NULL;
			state->mC_MediaFormat	= NULL;
		}
		
		FskMemPtrDispose(state);
	}
	
//	fclose(fp);
	
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
					
					*audio_decoder_config_offset = (int)( esds - esds0 );
					
					//while (!(*esds++ & 0x80))			
					//	count++;
					//*audio_decoder_config_size = count;
					
					*audio_decoder_config_size = esds_size;
					
					return 0;
				}
			}
		}
	}
	
	return -1;
}

int make_aac_config_data( unsigned char *esds, int esds_size, unsigned char **config_data_out, int *config_data_size_out )
{
	unsigned char	*config_data;
	int				config_data_size;	
	int				config_data_offset;	
	
	int				audio_object_type;
	int				sample_rate_index;
	int				output_sample_rate;
	int				output_channel_total;
	int				err = 0;
	
	err = scan_aac_esds
	(
	 esds,
	 esds_size,
	 NULL,
	 &audio_object_type,
	 &sample_rate_index,
	 &output_sample_rate,
	 &output_channel_total,
	 &config_data_offset,
	 &config_data_size
	);
	
	if( err == 0 )
	{
		dlog( "scanned esds, sample_rate: %d, channel_total: %d\n", output_sample_rate, output_channel_total );
		err = FskMemPtrNewClear(config_data_size, (FskMemPtr *)&config_data);
		BAIL_IF_ERR( err );
		
		memcpy( (void *)config_data, (void *)(esds + config_data_offset), config_data_size );
		dlog( "got decoder_config, size: %d, %x, %x, %x\n", config_data_size, config_data[0], config_data[1], config_data[2] );
	}
	else
	{
		UInt32	aac_sample_rate_index;
		
		
		output_channel_total = 2;
		
		for (
			 aac_sample_rate_index = 0; 
			 
			 (kFskOMXAudioDecoderAACSampleRates[aac_sample_rate_index] != 0) && 
			 (output_sample_rate != (int)kFskOMXAudioDecoderAACSampleRates[aac_sample_rate_index]); 
			 
			 aac_sample_rate_index++
			 )
		{}
		
		if (kFskOMXAudioDecoderAACSampleRates[aac_sample_rate_index] == 0)
		{
            BAIL( kFskErrUnimplemented );
		}
		
		config_data_size = 2;
		err = FskMemPtrNewClear(config_data_size, (FskMemPtr *)&config_data);
		BAIL_IF_ERR( err );
		
		dlog( "creating decoder_config, size: 2\n" );
		config_data[0] = (UInt8)((kAACObjectTypeLow << 3)     | (aac_sample_rate_index >> 1));
		config_data[1] = (UInt8)((aac_sample_rate_index << 7) | (output_channel_total << 3));
	}
	
	*config_data_out	  = config_data;
	*config_data_size_out = config_data_size;	
	
bail:
	return err;
}


FskErr AndroidJavaAudioDecodeDecompressFrames(void *stateIn, FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *samplesSize)
{
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	JNIEnv              *jniEnv			= state->jniEnv;
	unsigned char	*wav_data;
	unsigned char 	*thisFrameData 	= (unsigned char *)data;
	unsigned char 	*thisWavData;
	jbyteArray			InputBuf		= NULL;
	jbyteArray			InputCng		= NULL;
	long			i;
	int 			totalBytes = 0;
	int				sourceBytes = 0;
	int				max_wav_size = kMaxWavSize * (frameCount ? frameCount : 1);
	int				thisFrameSize = 0;
	FskErr 			err;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog( "into AndroidJavaAudioDecodeDecompressFrames: dataSize: %d, frameCount: %d, frameSizes = 0x%x\n\n", (int)dataSize, (int)frameCount, (int)frameSizes); 

	if (state->mC_MediaFormat == NULL) {
		dlog( "=========> Start decoder initialization here!\n" );
		unsigned char *esds 	  = (unsigned char *)deco->formatInfo;
		int			   esds_size  = deco->formatInfoSize;
		unsigned char *codec_specific_data		= NULL;
		int			   codec_specific_data_size = 0;

		if (g_mime == MIME_AAC) { //aac configure data
			err = make_aac_config_data( esds, esds_size, &codec_specific_data, &codec_specific_data_size );
			BAIL_IF_ERR( err );
			
//			fwrite((unsigned char *)&codec_specific_data_size, 1, 2, fp);
//			fwrite((unsigned char *)codec_specific_data, 1, codec_specific_data_size, fp);
		}
		
		if( state->canChangeSampleRate )
		{
			;//TODO: check SBR?
		}

		Init_JNI_Env();
		FskThread self          = FskThreadGetCurrent();
		jniEnv					= (JNIEnv *)self->jniEnv;
		state->jniEnv			= jniEnv;
		dlog( "In CompressFrame, jniEnv = 0x%x\n", (int)jniEnv );
		
		//init JNI class, objects and functions
		dlog( "Init JNI MediaCodecCore and  MediaCodecSysm ...\n" );
		err = InitJNIMediaCodecSysm(state);
		if (err) {
			BAIL_IF_ERR(err);
		}
		err = InitJNIMediaCodecCore(state);
		if (err) {
			BAIL_IF_ERR(err);
		}
		
		switch (g_mime) {
			case MIME_AAC:
				state->mimeType = (*jniEnv)->NewStringUTF(jniEnv, "audio/mp4a-latm");
				break;
			case MIME_MP3:
				state->mimeType = (*jniEnv)->NewStringUTF(jniEnv, "audio/mpeg");
				break;
			default:
				dlog( "Can not support this CODEC!!!\n" );
				break;
		}
#if SUPPORT_INSTRUMENTATION
		char *cstr = (char *)(*jniEnv)->GetStringUTFChars(jniEnv, state->mimeType, 0);
		dlog( "Create %s Decoder by Type Begin here ...\n", cstr );
#endif
		/* New MediaCodec by Type */
		jobject obj = (*jniEnv)->CallStaticObjectMethod(jniEnv, state->C_MediaCodec, state->F_createDecoderByType_ID, state->mimeType);
		state->mC_MediaCodec = (*jniEnv)->NewGlobalRef(jniEnv, obj);
		(*jniEnv)->DeleteLocalRef(jniEnv, obj);
		if (state->mC_MediaCodec == NULL) {
			dlog( "mC_MediaCodec not initialized ...\n" );
		}
		else {
			dlog( "mC_MediaCodec successful initialized ...\n" );
		}

        /* Set MediaCodec instance */
        (*jniEnv)->CallVoidMethod(jniEnv, state->mMediaCodecCore, state->F_MediaSetMediaCodecInstance_ID, state->mC_MediaCodec);
		
		/* New MediaFormat */
		jclass		C_MediaFormat = NULL;
		jmethodID	F_createAudioFormat_ID = NULL;
		
		if (C_MediaFormat == NULL) {
			C_MediaFormat = (*jniEnv)->FindClass(jniEnv, "android/media/MediaFormat");
			if(C_MediaFormat == NULL){
				dlog( "Can not find MediaFormat class, error!\n" );
				return kFskErrNotFound;
			}
			dlog( "Mapping class MediaFormat OK!\n" );
		}
		
		if (F_createAudioFormat_ID == NULL) {
			F_createAudioFormat_ID = (*jniEnv)->GetStaticMethodID(jniEnv, C_MediaFormat, "createAudioFormat",
																  "(Ljava/lang/String;II)Landroid/media/MediaFormat;");
			if (F_createAudioFormat_ID == NULL) {
				(*jniEnv)->DeleteLocalRef(jniEnv, C_MediaFormat);
				return kFskErrNotFound;
			}
			dlog( "Mapping function createAudioFormat OK!\n" );
		}
		if (state->F_getInteger_ID == NULL) {
			state->F_getInteger_ID = (*jniEnv)->GetMethodID(jniEnv, C_MediaFormat, "getInteger", "(Ljava/lang/String;)I");
			if (state->F_getInteger_ID == NULL) {
				(*jniEnv)->DeleteLocalRef(jniEnv, C_MediaFormat);
				return -2;
			}
			dlog( "Mapping function getInteger OK!\n" );
		}
		
		/* call MediaFormat new */
		dlog( "Call createAudioFormat here ...!\n" );
		jobject objf = (*jniEnv)->CallStaticObjectMethod(jniEnv, C_MediaFormat, F_createAudioFormat_ID, state->mimeType, 441000/*sample_rate*/, 2/*channel_count*/);
		state->mC_MediaFormat = (*jniEnv)->NewGlobalRef(jniEnv, objf);
		(*jniEnv)->DeleteLocalRef(jniEnv, objf);
		
		/* call MediaCodec configure */
		dlog( "Call MediaCodec configure ...\n");
		(*jniEnv)->CallVoidMethod(jniEnv, state->mC_MediaCodec, state->F_configure_ID, state->mC_MediaFormat, NULL, NULL, 0);
																						  /* parameters 0      1     2    3 */
		/* call start */
		dlog( "Call MediaCodec start ...\n");
		(*jniEnv)->CallVoidMethod(jniEnv, state->mC_MediaCodec, state->F_start_ID);
		
		(*jniEnv)->DeleteLocalRef(jniEnv, C_MediaFormat);
		state->flags = BUFFER_FLAG_CODEC_CONFIG;
		
		/* Configure by JAVA Decoder */
		if (g_mime == MIME_AAC) {
			InputCng = (*jniEnv)->NewByteArray(jniEnv, codec_specific_data_size);
			chartoByteArray(jniEnv, InputCng, (unsigned char *)codec_specific_data, (int)codec_specific_data_size);
			dlog( "aac configure data(ESDS) size: %d\n", codec_specific_data_size);
		} else if (g_mime == MIME_MP3) {
			if (esds == NULL || esds_size == 0) {
				dlog( "no configure data!!! use fisrt frame instead !!!\n" );
				esds = (unsigned char *)data;
				esds_size = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[0];
			}
			dlog( "MP3 configure data size = %d, data[0] = %x, data[1] = %x, data[2] = %x, data[3] = %x, data[4] = %x, data[5] = %x\n",
					esds_size, esds[0], esds[1], esds[2], esds[3], esds[4], esds[5]);
			InputCng = (*jniEnv)->NewByteArray(jniEnv, esds_size);
			chartoByteArray(jniEnv, InputCng, (unsigned char *)esds, (int)esds_size);
			dlog( "mp3 configure data size: %d\n", esds_size);
		} else {
			;//@@AMR
		}

        int bSize = (*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_MediaQueueInputBufferSize_ID, InputCng, BUFFER_FLAG_CODEC_CONFIG);
		(*jniEnv)->DeleteLocalRef(jniEnv, InputCng);

        if (bSize != -1) {
            MAX_INPUT_SIZE = bSize;
            dlog( "Get max input buffer size from Java APIs, bSize = %d\n", bSize );
        } else {
            MAX_INPUT_SIZE = 8192;
            dlog( "Something error, can not get bSize ...\n" );
        }

		if( codec_specific_data != NULL )
			FskMemPtrDisposeAt( (void **)&codec_specific_data );
		
		dlog( "=========> decoder initializaton successful!!!\n" );
	} /* Initialize End */

#ifdef Performance_Profiling
	struct timeval tv_begin;
	gettimeofday(&tv_begin, NULL);
#endif

	state->flags = 0;
//	fwrite((unsigned char *)&dataSize, 1, 2, fp);
//	fwrite((unsigned char *)data, 1, dataSize, fp);
	
	err = FskMemPtrNew(max_wav_size, (FskMemPtr *)&wav_data);
	if (err) return err;
	
	dlog( "init wav buffer, max_wav_size = %d\n", max_wav_size );
	thisWavData = wav_data;

	UInt32 K=0;
	if (dataSize != 0 /*&& (state->outputSampleRate == 0 || state->outputChannelCount == 0 || state->outputSampleRate != (int)deco->inputSampleRate)*/) {
		thisFrameSize = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[K];
		K++;
	}
	else {
		if (dataSize < (UInt32)MAX_INPUT_SIZE) {
			thisFrameSize = dataSize;
            K  = frameCount;
		} else {
			UInt32 size=0;
			while (K < frameCount)
            {
				size = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[K];
				if (thisFrameSize + size > (UInt32)MAX_INPUT_SIZE) {
					break;
				} else {
					thisFrameSize += size;
					K++;
				}
			}
		}
	}

	for( i = 0; ; i++ )
	{
		int thisWavSize, index;
		unsigned char *Outbuf;
        jobject obj_bbf;

        index = (int)(*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_AudioCheckInputBufferAvail_ID, state->flags);
        dlog( " ### input buffer index is: %d\n", index );

        if (index >= 0) {
            dlog( "thisFrameSize = %d\n", thisFrameSize );
            if (thisFrameSize != 0) {
                InputBuf = (*jniEnv)->NewByteArray(jniEnv, thisFrameSize);
                chartoByteArray(jniEnv, InputBuf, (unsigned char *)thisFrameData, (int)thisFrameSize);
                
                dlog( "&&&&&&&& Call ProcessOneFrame &&&&&&& \n");
                err = (FskErr)(*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_MediaQueueDataToInput_ID, InputBuf, index, state->flags);
                BAIL_IF_ERR(err);
            } else {
                dlog( "&&&&&&&& Tell ProcessOneFrame to Stop &&&&&&& \n");
                err = (FskErr)(*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_MediaQueueNullToInput_ID, NULL, index, BUFFER_FLAG_END_OF_STREAM);
                BAIL_IF_ERR(err);
            }

            thisFrameData += thisFrameSize;
            sourceBytes   += thisFrameSize;

            /* try to release byteArray here... */
            if (InputBuf) {
                dlog( "release InputBuf here ...\n");
                (*jniEnv)->DeleteLocalRef(jniEnv, InputBuf);
                InputBuf = NULL;
            }

            if (dataSize != 0 /*&& (state->outputSampleRate == 0 || state->outputChannelCount == 0 || state->outputSampleRate != (int)deco->inputSampleRate)*/) {
                dlog( "media format has not changed, keep sending frames one by one ...\n" );
                thisFrameSize = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[K];
                K++;
            }
            else {
                dlog( "still have undecoded frames here, send them ultimate ability ...\n" );
                UInt32 size=0; thisFrameSize = 0;
                while (K < frameCount) {
                    size = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[K];
                    dlog( "K = %d, size = %d!!!\n", K, size);
                    if (thisFrameSize + size > (UInt32)MAX_INPUT_SIZE) {
                        break;
                    } else {
                        K++;
                        thisFrameSize += size;
                    }
                    dlog( "thisFrameSize = %d!!!\n", thisFrameSize);
                }
            }
        } else if (index == 0xff) {
            BAIL(kFskErrBadState);
        }

        /* check output buffer */
        do {
            obj_bbf = (jbyteArray)(*jniEnv)->CallObjectMethod(jniEnv, state->mMediaCodecCore, state->F_AudioDeQueueOutputBuffer_ID);

            if (state->outputSampleRate == 0 || state->outputChannelCount == 0) {
                int CODEC_STATUS = (*jniEnv)->GetIntField(jniEnv, state->mMediaCodecCore, state->V_CODEC_STATUS_ID);
                dlog( "1. Get codec_status value = %d", CODEC_STATUS );

                if ((CODEC_STATUS&0x4) == 0x4) {
                    dlog( "update media format here ...\n" );
                    jobject obj_mf = (*jniEnv)->CallObjectMethod(jniEnv, state->mC_MediaCodec, state->F_getOutputFormat_ID);
                    /* Keys */
                    jstring KEY_SAMPLE_RATE		= (*jniEnv)->NewStringUTF(jniEnv, "sample-rate");	//string of sample rate
                    jstring KEY_CHANNEL_COUNT	= (*jniEnv)->NewStringUTF(jniEnv, "channel-count");	//string of channel count

                    /* MediaFormat Key tests */
                    if (state->canChangeSampleRate)
                        state->outputSampleRate	 = (*jniEnv)->CallIntMethod(jniEnv, obj_mf, state->F_getInteger_ID, KEY_SAMPLE_RATE);
                    if (state->canChangeChannelCount)
                        deco->outputChannelCount = state->outputChannelCount = (*jniEnv)->CallIntMethod(jniEnv, obj_mf, state->F_getInteger_ID, KEY_CHANNEL_COUNT);
                    dlog( "state->outputSampleRate      = %d \n", state->outputSampleRate );
                    dlog( "state->outputChannelCount	= %d \n", state->outputChannelCount );
                } else if (CODEC_STATUS == -1) {
                    dlog( "Decoding meet problems, we have to shut down decoder right now !!!" );
                    BAIL(kFskErrBadState);
                }
            }

            if (obj_bbf != NULL) {
                if (state->WavSize == 0) {
                    state->WavSize = (*jniEnv)->GetIntField(jniEnv, state->mMediaCodecCore, state->V_CROP_LT_ID); //(*jniEnv)->GetDirectBufferCapacity(jniEnv, obj_bbf);
                    dlog( "WavSize: %d!!!\n", (int)state->WavSize);
                }

                thisWavSize = state->WavSize;
                Outbuf = (unsigned char *)(*jniEnv)->GetDirectBufferAddress(jniEnv, obj_bbf);
#if SUPPORT_INSTRUMENTATION
                state->frames_out += 1;
#endif
                totalBytes  += thisWavSize;
                if (totalBytes >= max_wav_size) {
                    max_wav_size += kMaxWavSize;
                    err = FskMemPtrRealloc(max_wav_size, (FskMemPtr *)&wav_data);
                    if (err) return err;
                    dlog( "re-allocate wav buffer, max_wav_size = %d\n", max_wav_size );
                    thisWavData = wav_data + (totalBytes-thisWavSize); //current buffer pointer ...
                }
                memcpy(thisWavData, Outbuf, thisWavSize);
                thisWavData += thisWavSize;

                (*jniEnv)->DeleteLocalRef(jniEnv, obj_bbf);		//delete local reference
                (*jniEnv)->CallVoidMethod(jniEnv, state->mMediaCodecCore, state->F_MediaReleaseOutputBuffer_ID);
            }
        } while(obj_bbf != NULL);

        dlog( "check if there still frames to be decoded, byteused: %d, framecoded: %d...\n", sourceBytes, K);
        if( sourceBytes >= (int)dataSize || i >= (int)frameCount) {
            dlog( "all frames are done !!! ...\n");
            break;
        }
	}

	if( totalBytes == 0 )
	{
		dlog( "no frame decoded, therefor no wave data !!! ...\n");
		FskMemPtrDisposeAt( (void **)&wav_data );
	}
	
	dlog("============>totalBytes:%d\n", totalBytes ); 
	*samplesSize = totalBytes;
	*samples 	 = (void *)wav_data;
#if SUPPORT_INSTRUMENTATION
    state->frames_in += frameCount;
#endif

bail:
	if (InputBuf) {
		(*jniEnv)->DeleteLocalRef(jniEnv, InputBuf);
	}

#ifdef Performance_Profiling
	struct timeval tv_end;
	int cur_time;
	gettimeofday(&tv_end, NULL);
	cur_time = (int)(1000000 * (tv_end.tv_sec - tv_begin.tv_sec) + tv_end.tv_usec - tv_begin.tv_usec);
	state->total_time += cur_time;
	state->avege_time = (double)(state->total_time / state->frames_in);
	dlog("Performance_Profiling: total time = %dms, total input = %d, current time = %dms, mean time = %2.2fms\n", (int)(state->total_time/1000), state->frames_in, (int)(cur_time/1000), state->avege_time/1000);
#endif

	dlog( "outof AndroidJavaAudioDecodeDecompressFrames, err = %d, frames_in/frames_out = %d/%d\n", err, state->frames_in, state->frames_out);
	return err;
}

FskErr AndroidJavaAudioDecodeDiscontinuity(void *stateIn, FskAudioDecompress deco)
{
	FskErr err = kFskErrNone;
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	JNIEnv *jniEnv = state->jniEnv;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog( "into AndroidJavaAudioDecodeDiscontinuity\n\n");
#if SUPPORT_INSTRUMENTATION
    state->frames_in = state->frames_out = 0;
#endif
	/* call MediaCodec close */
	if (state->mC_MediaFormat && state->mMediaCodecCore && state->F_flush_ID && state->mC_MediaCodec) {
		dlog( "Call MediaCodec flush ...");
		(*jniEnv)->CallVoidMethod(jniEnv, state->mMediaCodecCore, state->F_flush_ID, state->mC_MediaCodec);
	}

	return err;
}

static FskErr AndroidJavaAudioDecodeGetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	UInt32 sampleRate = 0;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog("into AndroidJavaAudioDecodeGetSampleRate, outputSampleRate: %d\n\n", state->outputSampleRate ); 
	
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

static FskErr AndroidJavaAudioDecodeGetChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	UInt32 channelCount = 0;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog("into AndroidJavaAudioDecodeGetChannelCount, outputChannelCount: %d\n\n", state->outputChannelCount ); 
	
	
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

static FskErr AndroidJavaAudioDecodeGetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	Boolean canChangeSampleRate = false;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog("into AndroidJavaAudioDecodeGetCanChangeSampleRate, canChangeSampleRate: %d\n\n", state->canChangeSampleRate ); 
	
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

static FskErr AndroidJavaAudioDecodeSetCanChangeSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	FskAudioDecompress deco = (FskAudioDecompress)obj;
	Boolean canChangeSampleRate = property->value.b;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog("into AndroidJavaAudioDecodeSetCanChangeSampleRate, canChangeSampleRate: %d\n\n", canChangeSampleRate ); 
	
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

static FskErr AndroidJavaAudioDecodeGetCanChangeChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	Boolean canChangeChannelCount = false;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog("into AndroidJavaAudioDecodeGetCanChangeChannelCount, canChangeChannelCount: %d\n\n", state->canChangeChannelCount ); 
	
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

static FskErr AndroidJavaAudioDecodeSetCanChangeChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	FskAudioDecompress deco = (FskAudioDecompress)obj;
	Boolean canChangeChannelCount = property->value.b;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog("into AndroidJavaAudioDecodeSetCanChangeChannelCount, canChangeChannelCount: %d\n\n", canChangeChannelCount ); 
	
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
