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
#define MAX_DELAY_FRAME_IN_NUM  4/*35*/
#define MAX_DELAY_FRAME_OUT_NUM 35
#define AAC_DEC_OK 0

static int MAX_INPUT_SIZE = 8192; //Limit of Input Buffer

#define frame_ary_reset(ff)	{ (ff)->i=0; (ff)->n = 0; }

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

//typedef struct
//{
//	int             n;        //total
//	int             i;        //index
//	unsigned char   *d[64];   //data
//	int             s[64];    //size
//	char            f[64];    //flag
//}FrameAry;

typedef struct DataItemRecord DataItemRecord;
typedef struct DataItemRecord *DataItemPtr;

struct DataItemRecord
{
	DataItemPtr					next;
	unsigned char				*data;
	int							size;
	int							flag;
	int							frame_number;
	int							drop;
	FskInt64					decode_time;
	FskInt64					compos_time;
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
	jmethodID			F_AudioQueueDataToInput_ID;
	jmethodID			F_DecodeAudioFrame_ID;
	jmethodID			F_AudioQueueInputBufferSize_ID;
	jmethodID			F_AudioDeQueueOutputBuffer_ID;
	jmethodID			F_flush_ID;
	jmethodID			F_close_ID;
	jmethodID			F_CodecFormatAvailable_ID;
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
    int                 MimeType;
    int                 bad_state;
	int					opbuf_changed;
	FskCondition		needmoredata_condition;
//	int					need_queue_in_data;
	FskListMutex		data_item_list;
	int					frames_queue_in;
	int					frames_send_in;
	int					frames_send_out;
	FskThread			audio_data_thread;
	int					close_audio_data_thread;
	int					audio_data_thread_is_running;
	int					last_idx;
    FskMutex            sample_mutex;
    FskMutex            funcal_mutex;
    UInt32              wav_size;
    unsigned char *     wav_data;
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
//	dlog( "into chartoByteArray... \n" );
	jbyte *by = (jbyte*)buf;
	(*env)->SetByteArrayRegion(env, jBuf, 0, nOutSize, by);
}

static char *ByteArraytochar(JNIEnv* env, jbyteArray jarry, int *size)
{
//	dlog( "into ByteArraytochar... \n" );
	jbyte *arrayBody = (*env)->GetByteArrayElements(env, jarry, 0);
	*size = (*env)->GetArrayLength(env, jarry);
	char *ptr = (char *)arrayBody;
	
	return ptr;
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
		state->MediaCodecCore = (*jniEnv)->FindClass(jniEnv, FSK_JAVA_NAMESPACE);
		if(state->MediaCodecCore == NULL){
			dlog( "Mapping class MediaCodecCore error!\n" );
			return -1;
		}
		dlog( "Mapping class MediaCodecCore OK!\n" );
	}
	
	if (state->mMediaCodecCore == NULL) {
		if (GetMediaCodecInstance(jniEnv, stateIn) != 0) {
			dlog( "Mapping class mMediaCodecCore error!\n" );
			return -2;
		}
		dlog( "Initialize object mMediaCodecCore OK!\n" );
	}
	
	/* function DecodeAudioFrame */
	if (state->F_DecodeAudioFrame_ID == NULL) {
		state->F_DecodeAudioFrame_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "DecodeAudioFrame","(Landroid/media/MediaCodec;[BI)[B");
		if (state->F_DecodeAudioFrame_ID == NULL) {
			dlog( "Mapping function F_DecodeAudioFrame_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function DecodeAudioFrame OK!\n" );
	}
	
	/* function AudioQueueInputBuffer */
	if (state->F_AudioQueueInputBufferSize_ID == NULL) {
		state->F_AudioQueueInputBufferSize_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "AudioQueueInputBufferSize","(Landroid/media/MediaCodec;[BI)I");
		if (state->F_DecodeAudioFrame_ID == NULL) {
			dlog( "Mapping function F_AudioQueueInputBufferSize_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_AudioQueueInputBufferSize_ID OK!\n" );
	}
	
	/* function AudioDeQueueOutputBuffer */
	if (state->F_AudioDeQueueOutputBuffer_ID == NULL) {
		state->F_AudioDeQueueOutputBuffer_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "AudioDeQueueOutputBuffer","(Landroid/media/MediaCodec;)[B");
		if (state->F_DecodeAudioFrame_ID == NULL) {
			dlog( "Mapping function F_AudioDeQueueOutputBuffer_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_AudioDeQueueOutputBuffer_ID OK!\n" );
	}
	
	/* function AudioCheckInputBufferAvail */
	if (state->F_AudioCheckInputBufferAvail_ID == NULL) {
		state->F_AudioCheckInputBufferAvail_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "AudioCheckInputBufferAvail","(Landroid/media/MediaCodec;I)I");
		if (state->F_AudioCheckInputBufferAvail_ID == NULL) {
			dlog( "Mapping function F_AudioCheckInputBufferAvail_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_AudioCheckInputBufferAvail_ID OK!\n" );
	}
	
	/* function AudioQueueDataToInput */
	if (state->F_AudioQueueDataToInput_ID == NULL) {
		state->F_AudioQueueDataToInput_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "AudioQueueDataToInput","(Landroid/media/MediaCodec;[BII)I");
		if (state->F_AudioQueueDataToInput_ID == NULL) {
			dlog( "Mapping function F_AudioQueueDataToInput_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_AudioQueueDataToInput_ID OK!\n" );
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

static void data_queue_flush(FskListMutex data_item_list)
{
	dlog( "into data_queue_flush ..\n" );
	if (data_item_list == NULL)
		return;

	while(1)
	{
		DataItemPtr item = (DataItemPtr)FskListMutexRemoveFirst(data_item_list);
		if (item == NULL)
			break;
		FskMemPtrDispose(item);
	}
	dlog( "out of data_queue_flush ..\n" );
}

FskErr AndroidJavaAudioDecodeCanHandle(UInt32 format, const char *formatStr, Boolean *canHandle)
{
	
	dlog( "into AndroidJavaAudioDecodeCanHandle\n\n"); 
	*canHandle = (kFskAudioFormatAAC == format) || (0 == FskStrCompare(formatStr, "format:aac")) || (0 == FskStrCompare(formatStr, "x-audio-codec/aac")) ||
				 (kFskAudioFormatMP3 == format) || (0 == FskStrCompare(formatStr, "format:mp3")) || (0 == FskStrCompare(formatStr, "x-audio-codec/mp3"));
	
//	*canHandle = 0;
	
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

	state->frames_queue_in				= 0;
	state->frames_send_in				= 0;
	state->frames_send_out				= 0;
	state->close_audio_data_thread		= 0;
	state->audio_data_thread_is_running	= 0;
    state->MimeType = g_mime;
    state->last_idx = -1;
	
	err = FskConditionNew(&state->needmoredata_condition);
	BAIL_IF_ERR(err);

    err = FskMutexNew(&state->sample_mutex, "SampleMutex");
    BAIL_IF_ERR(err);

    err = FskMutexNew(&state->funcal_mutex, "FunctionCallMutex");
    BAIL_IF_ERR(err);

	deco->state = (void *)state;
	err = FskListMutexNew(&state->data_item_list, "DatItemList");
	BAIL_IF_ERR(err);
	
	state->canChangeSampleRate	= 1;
	state->canChangeChannelCount= 1;
	state->outputSampleRate		= 0;
	state->outputChannelCount   = 0;

	deco->outputFormat		 = kFskAudioFormatPCM16BitLittleEndian;
	deco->outputChannelCount = deco->inputChannelCount;
    state->bad_state = 0;

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
		
		if (state->audio_data_thread_is_running) {
			state->close_audio_data_thread = 1;
			dlog("set close_audio_data_thread = %d\n", state->close_audio_data_thread );
			
			while (state->audio_data_thread_is_running) {
				dlog("audio_data_thread_is_running, wait for a while\n" );
				FskConditionSignal(state->needmoredata_condition);
				FskThreadYield();
			}
			dlog( "dispose audio_data_thread\n" );
//			FskMemPtrDisposeAt(&state->audio_data_thread); //@@crash for some unknown reason ??
		}
		
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
		
		if (state->needmoredata_condition) {
			FskConditionDispose(state->needmoredata_condition);
		}
		if(state->data_item_list != NULL) {
			data_queue_flush(state->data_item_list);
			FskListMutexDispose(state->data_item_list);
		}
        if (state->sample_mutex) {
            FskMutexDispose(state->sample_mutex);
        }
        if (state->funcal_mutex) {
            FskMutexDispose(state->funcal_mutex);
        }
		
		FskMemPtrDispose(state);
	}

//	fclose(fp);
    dlog( "outof AndroidJavaAudioDecodeDispose\n\n");
	
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

static FskErr data_queue_in(FskListMutex data_item_list, unsigned char *data, int size, int flags, int num, int dct)
{
//	FrameAry	ff;
	FskErr		err		= kFskErrNone;
	DataItemPtr	item	= NULL;
	
	dlog( " into data_queue_in(), size: %d, frame number: %d\n", (int)size, (int)num);
	err = FskMemPtrNewClear(sizeof(DataItemRecord), (FskMemPtr *)&item);
	BAIL_IF_ERR( err );

	item->data              = data;
	item->size              = size;
	item->flag              = flags;
	item->frame_number		= num;
	if (dct == 1) {
		FskListMutexAppend(data_item_list, item);
	} else {
		FskListMutexPrepend(data_item_list, item);
	}

	
bail:
	return err;	
}

static FskErr data_queue_out(FskListMutex data_item_list, unsigned char  **data, int *size, int *flags, int *num)
{
	DataItemPtr	item	= NULL;
	FskErr		err		= kFskErrNone;

	item = (DataItemPtr)FskListMutexRemoveFirst(data_item_list);
	
	if( item != NULL )
	{
		dlog(" ### return cached\n" );
		*data	= item->data;
		*size	= item->size;
		*flags	= item->flag;
		*num	= item->frame_number;
		FskMemPtrDispose(item);
	}
	else
	{
		dlog(" ### not exist, return default\n" );
		*data	= NULL;
		*size	= 0;
		*flags	= 0;
	}

	dlog( " ### outof data_queue_out(), size = %d, frame number = %d\n", *size, *num);
	return err;
}

static void audio_data_thread(void *refcon) {
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)refcon;
	JNIEnv *jniEnv = state->jniEnv;
	jbyteArray InputBuf  = NULL;
    jbyteArray OutputBuf = NULL;
	unsigned char *thisFrameData = NULL;
	unsigned char *thisWavData;
	int thisFrameSize;
	FskErr err = kFskErrNone;

//    FskThreadInitializationComplete(FskThreadGetCurrent());

	dlog( " ### into thread audio_data_thread, state = %x jniEnv = %x", (int)state, (int)jniEnv );
	state->audio_data_thread_is_running = 1;

    /* jniEnv of current Thread */
    Init_JNI_Env();
    FskThread self          = FskThreadGetCurrent();
    jniEnv					= (JNIEnv *)self->jniEnv;
    dlog( " ### thread jniEnv = %x", (int)jniEnv );
	
	while (!state->close_audio_data_thread) {
		int index, thisFrameNum = 0, thisFlag = 0;

        FskMutexAcquire(state->funcal_mutex);
		if (state->last_idx == -1) {
			index = (int)(*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_AudioCheckInputBufferAvail_ID, state->mC_MediaCodec, 0/*state->flags*/);
		} else {
			index = state->last_idx;
		}
		state->last_idx = -1;
		dlog( " ### input buffer index is: %d\n", index );
        FskMutexRelease(state->funcal_mutex);

        /* check input buffer */
        if (index >= 0)
        {
			thisFrameData = NULL; thisFrameSize = 0;
			err = data_queue_out(state->data_item_list, &thisFrameData, &thisFrameSize, &thisFlag, &thisFrameNum);
			BAIL_IF_ERR(err);

			if (thisFrameSize != 0 && thisFrameData != NULL) {
//				state->need_queue_in_data = 0;
				InputBuf = (*jniEnv)->NewByteArray(jniEnv, thisFrameSize);
				if (InputBuf) {
					chartoByteArray(jniEnv, InputBuf, (unsigned char *)thisFrameData, (int)thisFrameSize);
				}
				
				dlog( " ########## Queue Data to InputBuffer ########## \n");
                FskMutexAcquire(state->funcal_mutex);
				err = (FskErr)(*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_AudioQueueDataToInput_ID, state->mC_MediaCodec, InputBuf, index, thisFlag);
                FskMutexRelease(state->funcal_mutex);
				if (err) {
                    state->bad_state = 1;
                    goto bail;
                }

                FskMutexAcquire(state->sample_mutex);
                state->frames_send_in += thisFrameNum;
				dlog( " ### frames alread send in is : %d\n", state->frames_send_in);
                FskMutexRelease(state->sample_mutex);

				jbyte* jpoint = (*jniEnv)->GetByteArrayElements(jniEnv, InputBuf, 0);
				(*jniEnv)->ReleaseByteArrayElements(jniEnv, InputBuf, jpoint, 0);
				(*jniEnv)->DeleteLocalRef(jniEnv, InputBuf);
				InputBuf = NULL;
			} else if (thisFlag == BUFFER_FLAG_END_OF_STREAM) {
                dlog( " ########## Queue NULL to Finish Decoding ########## \n");
                FskMutexAcquire(state->funcal_mutex);
				err = (FskErr)(*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_AudioQueueDataToInput_ID, state->mC_MediaCodec, NULL, index, thisFlag);
                FskMutexRelease(state->funcal_mutex);

				if (err) {
                    state->bad_state = 1;
                    goto bail;
                }

            } else {
                FskMutexAcquire(state->funcal_mutex);
//				state->need_queue_in_data = 1;
				dlog( " ### no more data in Mutex List, should call DecodeFrame next ...index = %d\n", index );
				state->last_idx = index;
				err = FskConditionWait(state->needmoredata_condition, state->funcal_mutex);
                FskMutexRelease(state->funcal_mutex);
				BAIL_IF_ERR(err);
//				continue;
			}
		} else if (index == 0xff) {
            state->bad_state = 1;
            BAIL(kFskErrBadState);
        }

        /* check output buffer */
        do {
            int thisWavSize;
            unsigned char *Outbuf;

            FskMutexAcquire(state->funcal_mutex);
            OutputBuf = (jbyteArray)(*jniEnv)->CallObjectMethod(jniEnv, state->mMediaCodecCore, state->F_AudioDeQueueOutputBuffer_ID, state->mC_MediaCodec);

            int CODEC_STATUS = (*jniEnv)->GetIntField(jniEnv, state->mMediaCodecCore, state->V_CODEC_STATUS_ID);
            dlog( " ### Get CODEC_STATUS value = %d\n", CODEC_STATUS );
            FskMutexRelease(state->funcal_mutex);

            if (CODEC_STATUS == -1) {
                dlog( " ### Decoding meet problems, we have to shut down decoder right now !!!" );
                err = kFskErrBadState;
                state->bad_state = 1;
                goto bail;
            }

            if ((CODEC_STATUS&0x4) == 0x4) {
                FskMutexAcquire(state->funcal_mutex);
                dlog( " ### update media format here ...\n" );
                jobject obj_mf = (*jniEnv)->CallObjectMethod(jniEnv, state->mC_MediaCodec, state->F_getOutputFormat_ID);
                /* Keys */
                jstring KEY_SAMPLE_RATE		= (*jniEnv)->NewStringUTF(jniEnv, "sample-rate");	//string of sample rate
                jstring KEY_CHANNEL_COUNT	= (*jniEnv)->NewStringUTF(jniEnv, "channel-count");	//string of channel count

                /* MediaFormat Key tests */
                state->outputSampleRate		= (*jniEnv)->CallIntMethod(jniEnv, obj_mf, state->F_getInteger_ID, KEY_SAMPLE_RATE);
                state->outputChannelCount	= (*jniEnv)->CallIntMethod(jniEnv, obj_mf, state->F_getInteger_ID, KEY_CHANNEL_COUNT);
                FskMutexRelease(state->funcal_mutex);
                dlog( " ### state->outputSampleRate     = %d \n", state->outputSampleRate );
                dlog( " ### state->outputChannelCount	= %d \n", state->outputChannelCount );
            }
            /* check whether output frames are valid more ... */
            if (OutputBuf != NULL) {

//                dlog( " ### mutex acquire 0 ... \n");
                FskMutexAcquire(state->sample_mutex);
//                dlog( " ### mutex acquire 1 ... \n");
                do {
                    Outbuf = (unsigned char *)ByteArraytochar(jniEnv, OutputBuf, &thisWavSize);
                    state->frames_send_out ++;
                    dlog( " ### frames alread send out is : %d\n", state->frames_send_out);

                    err = FskMemPtrRealloc(state->wav_size + thisWavSize, (FskMemPtr *)&state->wav_data);
                    if (err) {
                        dlog( " ### memory is not available, give up!\n");
//                        dlog( " ### mutex release 0 ... \n");
                        FskMutexRelease(state->sample_mutex);
//                        dlog( " ### mutex release 1 ... \n");
                        state->bad_state = 1;
                        goto bail;
                    }
                    dlog( " ### re-allocate wav buffer, buffer size = %d\n", state->wav_size+thisWavSize );
                    thisWavData = state->wav_data + state->wav_size; //last position of wav_data
                    memcpy(thisWavData, Outbuf, thisWavSize);
                    state->wav_size  += thisWavSize;
                    
                    (*jniEnv)->ReleaseByteArrayElements(jniEnv, OutputBuf, (jbyte *)Outbuf, 0);
                    (*jniEnv)->DeleteLocalRef(jniEnv, OutputBuf);		//delete local reference
                } while(0);
//                dlog( " ### mutex release 0 ... \n");
                FskMutexRelease(state->sample_mutex);
//                dlog( " ### mutex release 1 ... \n");
            }
            else { /* stop when no outputbuffer index available */
                dlog( " ### output buffer index is not available now, go next loop!\n");
                break;
            }
        } while (1);
	}

bail:
	dlog( " ### finish main loop of audio_data_thread... err = %d, state = %d\n", err, state->bad_state );
	
	if (err && InputBuf != NULL) {
		dlog( " ### meet bad states, decoder give up !!! ...\n");
		jbyte* jpoint = (*jniEnv)->GetByteArrayElements(jniEnv, InputBuf, 0);
		(*jniEnv)->ReleaseByteArrayElements(jniEnv, InputBuf, jpoint, 0);
		(*jniEnv)->DeleteLocalRef(jniEnv, InputBuf);
	}

	state->audio_data_thread_is_running = 0;
	dlog( " ### out of thread audio_data_thread\n" );
}

FskErr AndroidJavaAudioDecodeDecompressFrames(void *stateIn, FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *samplesSize)
{
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	JNIEnv          *jniEnv			= state->jniEnv;
	unsigned char 	*thisFrameData 	= (unsigned char *)data;
	jbyteArray		InputCng		= NULL;
    int             this_mime       = state->MimeType;
	long			i;
	FskErr 			err;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog( "into AndroidJavaAudioDecodeDecompressFrames: dataSize: %d, frameCount: %d, frameSizes = 0x%x\n\n", (int)dataSize, (int)frameCount, (int)frameSizes);

    if (state->bad_state) {
        BAIL(kFskErrBadState);
    }

    /* queue stream */
    if (frameCount) {
        if (state->outputSampleRate == 0 || state->outputChannelCount == 0) { //send in by frame before we get metadata.
            for ( i=0; i<(int)frameCount; i++ ) {
                int thisFrameSize = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[i];
                err = data_queue_in(state->data_item_list, thisFrameData, thisFrameSize, state->flags, 1, 1); state->frames_queue_in ++;
                BAIL_IF_ERR(err);
                thisFrameData += thisFrameSize;
            }
        } else {
            UInt32 k=0, thisFrameSize, frame_count;
            if (dataSize < (UInt32)MAX_INPUT_SIZE) {
                thisFrameSize = dataSize;
                dlog( "chunk size is : %d\n", thisFrameSize);
                err = data_queue_in(state->data_item_list, thisFrameData, thisFrameSize, state->flags, frameCount, 1);
                BAIL_IF_ERR(err);
            } else {
                do {
                    UInt32 size=0;
                    thisFrameSize = 0;
                    frame_count = 0;
                    do {
                        size = frameSizes == NULL ? (dataSize / frameCount) : frameSizes[k];
                        if (thisFrameSize + size > (UInt32)MAX_INPUT_SIZE) {
                            break;
                        } else {
                            thisFrameSize += size;
                            k++;
                            frame_count++;
                        }
                    } while (k < frameCount);
                    dlog( "current frames size is : %d, frames = %d\n", thisFrameSize, k );
                    err = data_queue_in(state->data_item_list, thisFrameData, thisFrameSize, state->flags, frame_count, 1);
                    BAIL_IF_ERR(err);
                    thisFrameData += thisFrameSize;

                } while (k < frameCount);
            }
            state->frames_queue_in += frameCount;
        }
    } else {
        err = data_queue_in(state->data_item_list, NULL, 0, BUFFER_FLAG_END_OF_STREAM, 0, 1);
        BAIL_IF_ERR(err);
    }

	if (state->mC_MediaFormat == NULL) {
		dlog( "=========> Start decoder initialization here!\n" );
		unsigned char *esds 	  = (unsigned char *)deco->formatInfo;
		int			   esds_size  = deco->formatInfoSize;
		unsigned char *codec_specific_data		= NULL;
		int			   codec_specific_data_size = 0;

		if (this_mime == MIME_AAC) { //aac configure data
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
		
		switch (this_mime) {
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
//		state->flags = BUFFER_FLAG_CODEC_CONFIG;

		/* Configure by JAVA Decoder */
		if (this_mime == MIME_AAC) {
			InputCng = (*jniEnv)->NewByteArray(jniEnv, codec_specific_data_size);
			chartoByteArray(jniEnv, InputCng, (unsigned char *)codec_specific_data, (int)codec_specific_data_size);
			dlog( "aac configure data(ESDS) size: %d\n", codec_specific_data_size);
		} else if (this_mime == MIME_MP3) {
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

		int bSize = (*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_AudioQueueInputBufferSize_ID, state->mC_MediaCodec, InputCng, BUFFER_FLAG_CODEC_CONFIG);
		jbyte* jpoint = (*jniEnv)->GetByteArrayElements(jniEnv, InputCng, 0);
		(*jniEnv)->ReleaseByteArrayElements(jniEnv, InputCng, jpoint, 0);
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
    else {
        FskMutexAcquire(state->funcal_mutex);
        err = FskConditionSignal(state->needmoredata_condition);
        FskMutexRelease(state->funcal_mutex);
        BAIL_IF_ERR(err);
    }

	state->flags = 0;
//	fwrite((unsigned char *)&dataSize, 1, 2, fp);
//	fwrite((unsigned char *)data, 1, dataSize, fp);

	/* create thread to send audio frame in */
	if (!state->audio_data_thread_is_running) {
		dlog( "starting thread audio_data_thread\n" );
		FskThreadCreate(&state->audio_data_thread, audio_data_thread, kFskThreadFlagsDefault, (void *)state, "audio_data_thread");
		while(!state->audio_data_thread_is_running)
		{
			dlog("state->audio_data_thread_is_running is false, yeilding!!!\n");		
			FskThreadYield();
		}
		usleep(1000); //1ms
    }

    FskTimeRecord t0, t1;
    FskTimeGetNow(&t0);
    while(  ((int)(state->frames_queue_in - state->frames_send_in) > (int)MAX_DELAY_FRAME_IN_NUM
          || (int)(state->frames_send_in - state->frames_send_out) > (int)MAX_DELAY_FRAME_OUT_NUM)
          && (!err) && state->audio_data_thread_is_running)
    {
        dlog( "Wait for decoder, queue_in/send_in/send_out = %d/%d/%d\n", state->frames_queue_in, state->frames_send_in, state->frames_send_out);
        FskTimeGetNow(&t1);
        if ((FskTimeInMS(&t1) - FskTimeInMS(&t0)) > 200) { //20ms
            dlog( "\nWait for enough time, try next main calling loop!\n\n" );
            break;
        }
        FskThreadYield();
    }
    dlog("============>state->wav_size : %d\n", state->wav_size );

//    dlog( "mutex acquire 0 ... \n");
    FskMutexAcquire(state->sample_mutex);
//    dlog( "mutex acquire 1 ... \n");
    if( (state->wav_size == 0 && state->wav_data != NULL) || err )
    {
        dlog("error condition, dispose wav_data: err:%x\n", (int)err);
        FskMemPtrDisposeAt( (void **)&state->wav_data );
    }

    if( state->wav_size != 0 ) {
        dlog("send out some wave data ...size = %d\n", state->wav_size);
        *samplesSize = state->wav_size;
        *samples 	 = (void *)state->wav_data;

        state->wav_data = NULL;
		state->wav_size = 0;
    }
//    dlog( "mutex release 0 ... \n");
    FskMutexRelease(state->sample_mutex);
//    dlog( "mutex release 1 ... \n");

bail:

	dlog( "outof AndroidJavaAudioDecodeDecompressFrames, err = %d, queue_in/send_in/send_out = %d/%d/%d\n", err, state->frames_queue_in, state->frames_send_in, state->frames_send_out);
	return err;
}

FskErr AndroidJavaAudioDecodeDiscontinuity(void *stateIn, FskAudioDecompress deco)
{
	FskErr err = kFskErrNone;
	FskAndroidJavaAudioDecodeRecord *state = (FskAndroidJavaAudioDecodeRecord *)stateIn;
	JNIEnv *jniEnv = state->jniEnv;
	
	dlog( "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	dlog( "into AndroidJavaAudioDecodeDiscontinuity\n\n");

    if (state->bad_state) {
        BAIL(kFskErrBadState);
    }
	
	if (state->data_item_list != NULL) {
		dlog("calling data_queue_flush()\n");
		data_queue_flush(state->data_item_list);
	}

    FskMutexAcquire(state->funcal_mutex);
	/* call MediaCodec flush */
	if (state->mC_MediaFormat && state->mMediaCodecCore && state->F_flush_ID && state->mC_MediaCodec) {
		dlog( "Call MediaCodec flush ...");
		(*jniEnv)->CallVoidMethod(jniEnv, state->mMediaCodecCore, state->F_flush_ID, state->mC_MediaCodec);
	}
    state->last_idx = -1;
    FskMutexRelease(state->funcal_mutex);

//    dlog( "mutex acquire 0 ... \n");
    FskMutexAcquire(state->sample_mutex);
//    dlog( "mutex acquire 1 ... \n");
	dlog("cleaning up wave data buffer\n");
	if( state->wav_data != NULL )
	{
		FskMemPtrDisposeAt( (void **)&state->wav_data );
	}
	state->wav_data = NULL;
	state->wav_size = 0;
    state->frames_queue_in = 0;
    state->frames_send_in  = 0;
    state->frames_send_out = 0;
//    dlog( "mutex release 0 ... \n");
    FskMutexRelease(state->sample_mutex);
//    dlog( "mutex release 1 ... \n");

bail:
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
