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
#define __FSKIMAGE_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKTHREAD_PRIV__

#include "FskAndroidVideoDecoder.h"
#include "FskYUV420Copy.h"
#include "kinoma_utilities.h"
#include "FskEndian.h"
#include "FskBitmap.h"
#include "QTReader.h"
#include <jni.h>

#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
FskInstrumentedTypeRecord gAndroidJavaVideoDecoderTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "FskAndroidVideoDecoder"};
#define dlog(...)  do { FskInstrumentedTypePrintfDebug  (&gAndroidJavaVideoDecoderTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define dlog(...)
#endif


#define SUPPORT_ROTATION
#if SRC_YUV420i
	#define YUV420_FORMAT	kFskBitmapFormatYUV420i
#else
	#define YUV420_FORMAT	kFskBitmapFormatYUV420
#endif

#ifndef QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka	//Samung Galaxy S2 Skyrocket, XiaoMi
#define QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka	0x7fa30c03
#endif

#ifndef COLOR_TI_FormatYUV420PackedSemiPlanar	//Galaxy Nexus
#define COLOR_TI_FormatYUV420PackedSemiPlanar	0x7f000100
#endif

//used for QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka
#define NV12T_WIDTH		64
#define	NV12T_HEIGHT	32
#define NV12T_SIZE	(NV12T_WIDTH * NV12T_HEIGHT)
#define NV12T_GROUP_SIZE	(NV12T_SIZE * 4)

#define NV12T_MAX_ROW	60	//max_width: 40x64
#define NV12T_MAX_COL	60	//max_height: 40x32
static int nv12tile_index_matrix[NV12T_MAX_ROW][NV12T_MAX_COL];
static int nv12tile_index_matrix_uv[NV12T_MAX_ROW][NV12T_MAX_COL];

#define kBitmapCacheSize		50

#define SC_LEN					4	//startcode length
#define DEFAULT_NALU_LEN		4
#define kDefaultNALULengthSize	4
#define NAL_REF_IDC_BITS		0x60
#define NAL_UNITTYPE_BITS       0x1f
#define MAX_ENROLLING_FRAMES	35

//#define Performance_Profiling
#ifdef Performance_Profiling
#include <sys/time.h>
#endif

extern char *modelName;
static int g_mime;
enum frame_type {
	MIME_AVC = 0,
	MIME_M4V = 1
};

typedef enum
{
	NAL_UT_RESERVED  = 0x00, // Reserved
	NAL_UT_SLICE     = 0x01, // Coded Slice - slice_layer_no_partioning_rbsp
	NAL_UT_DPA       = 0x02, // Coded Data partition A - dpa_layer_rbsp
	NAL_UT_DPB       = 0x03, // Coded Data partition A - dpa_layer_rbsp
	NAL_UT_DPC       = 0x04, // Coded Data partition A - dpa_layer_rbsp
	NAL_UT_IDR_SLICE = 0x05, // Coded Slice of a IDR Picture - slice_layer_no_partioning_rbsp
	NAL_UT_SEI       = 0x06, // Supplemental Enhancement Information - sei_rbsp
	NAL_UT_SPS       = 0x07, // Sequence Parameter Set - seq_parameter_set_rbsp
	NAL_UT_PPS       = 0x08, // Picture Parameter Set - pic_parameter_set_rbsp
	NAL_UT_PD        = 0x09, // Picture Delimiter - pic_delimiter_rbsp
	NAL_UT_FD        = 0x0a  // Filler Data - filler_data_rbsp
} NAL_Unit_Type;

FskMediaPropertyEntryRecord AndroidJavaVideoDecodeProperties[] =
{
	{kFskMediaPropertySampleDescription,		kFskMediaPropertyTypeData,		NULL,		AndroidJavaVideoDecodeSetSampleDescription},
 	{kFskMediaPropertyFormatInfo,				kFskMediaPropertyTypeData,		NULL,		AndroidJavaVideoDecodeSetSampleDescription},
	{kFskMediaPropertyRotation,					kFskMediaPropertyTypeFloat,		NULL,		AndroidJavaVideoDecodeSetRotation},
	{kFskMediaPropertyPixelFormat,				kFskMediaPropertyTypeUInt32List,NULL,		AndroidJavaVideoDecodeSetPreferredPixelFormat},
	{kFskMediaPropertyMaxFramesToQueue,			kFskMediaPropertyTypeInteger,	AndroidJavaVideoDecodeGetMaxFramesToQueue, NULL},
	{kFskMediaPropertyUndefined,				kFskMediaPropertyTypeUndefined,	NULL,		NULL}
};

typedef struct FuncItemRecord FuncItemRecord;		//output callback
typedef struct FuncItemRecord *FuncItemPtr;
struct FuncItemRecord
{
	FuncItemPtr					next;
	FskImageDecompressComplete	completionFunction;
	void						*completionRefcon;
	int							frame_number;
	int							drop;
	FskInt64					decode_time;
};

typedef struct
{
	UInt32				sampleDescriptionSize;
	unsigned char		*sampleDescription;
	UInt32				sampleDescriptionSeed;

	FskImageDecompress	deco;

	FskListMutex		func_item_list;
	int					nalu_len_size;
	int					nale_pre;
	int					enrolling_numbers;
	int					bad_state;
	FskBitmap			bitmaps[kBitmapCacheSize];

	UInt32				dst_pixel_format;
	float				rotation_float;
	int 				rotation;

	int					debug_input_frame_count;
	int					debug_output_frame_count;

	//env JNI
	JNIEnv				*jniEnv;
	//MediaCodecCore
	jclass				MediaCodecCore;
	jobject				mMediaCodecCore;
	jmethodID			F_VideoCheckInputBufferAvail_ID;
	jmethodID			F_MediaQueueDataToInput_ID;
    jmethodID           F_MediaQueueNullToInput_ID;
	jmethodID			F_MediaQueueInputBufferSize_ID;
	jmethodID			F_VideoDeQueueOutputBuffer_ID;
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
	int					display_width;
	int					display_height;
	jobject				mC_MediaFormat;
	jmethodID			F_getInteger_ID;
	//Main Loop
	int					flags;
	int					strideW;
	int					strideH;
	int					color_format;
	int					opbuf_changed;
	int                 wr_size;

	AVCC				avcC;

	int					qcft_init_flag;
	unsigned char		*y0;
	unsigned char		*u0;
	unsigned char		*v0;

#ifdef Performance_Profiling
	int					total_time;
	double				avege_time;
#endif

} FskAndroidJavaVideoDecode;

static FskErr func_queue_in( FskListMutex func_item_list, FskImageDecompressComplete completionFunction, void *completionRefcon, int frame_number, int drop, FskInt64 decode_time )
{
	FuncItemPtr item = NULL;
	FskErr      err = kFskErrNone;

	err = FskMemPtrNewClear(sizeof(FuncItemRecord), (FskMemPtr *)&item);
	BAIL_IF_ERR( err );

	item->completionFunction = completionFunction;
	item->completionRefcon   = completionRefcon;
	item->frame_number		 = frame_number;
	item->drop				 = drop;
	item->decode_time		 = decode_time;

	FskListMutexAppend(func_item_list, item);

bail:
	return err;
}

static FskErr func_queue_out( FskListMutex func_item_list, FskImageDecompressComplete *completionFunction, void	**completionRefcon, int *frame_number, int *drop, FskInt64 *decode_time )
{
	FuncItemPtr	item = NULL;
	FskErr      err = kFskErrNone;

	item = FskListMutexRemoveFirst(func_item_list);

	if( item != NULL )
	{
		dlog("return cached\n" );
		*completionFunction = item->completionFunction;
		*completionRefcon   = item->completionRefcon;
		*frame_number		= item->frame_number;
		*drop				= item->drop;
		*decode_time		= item->decode_time;
		FskMemPtrDispose(item);
	}
	else
	{
		dlog("return default\n" );
		*completionFunction = NULL;
		*completionRefcon   = 0;
		*frame_number		= 0;
		*drop				= 0;
		*decode_time		= 0;

		err = 1;
	}

//bail:
	return err;
}


static void func_queue_flush( FskImageDecompress deco, FskListMutex func_item_list, FskErr flush_err )
{
	if( func_item_list == NULL )
		return;

	while(1)
	{
		FuncItemPtr	func_item = FskListMutexRemoveFirst(func_item_list);;

		if( func_item == NULL )
		{
			dlog("no more func_item in queue!!!\n" );
			break;
		}

		if( func_item->completionFunction != NULL )
		{
			dlog("calling completionFunction/RefCon with error: %d, completionFunction: %x, completionRefcon: %d\n", (int)flush_err, (int)func_item->completionFunction, (int)func_item->completionRefcon );
			(func_item->completionFunction)(deco, func_item->completionRefcon, flush_err, NULL);
		}
		else
		{
			dlog("func_item->completionFunction == NULL!!!\n" );
		}

		FskMemPtrDispose(func_item);
	}
}

void SetSizeByLen( unsigned char *data, unsigned char len, short size )
{
	switch( len )
	{
	case 1:
		data[0] = (unsigned char)size;
		break;
	case 2:
		data[0] = (size>>8)&0x00ff;
		data[1] = size&0x00ff;
		break;
	case 4:
		data[0]	 = 0;
		data[1]	 = 0;
		data[2]	 = (size>>8)&0x00ff;
		data[3]	 = size&0x00ff;
		break;
	default:
		break;
	}
}


FskErr DecAVCC( unsigned char *data, AVCC *avcC )
{
	unsigned char reserved6BitsAndLengthSizeMinusOne = 0xFF;
	unsigned char reserved3BitsAndNumberofSequenceParameterSets = 0xE1;
	long		  idx = 8;//skip size and type

	dlog( "\n");
	dlog( "into DecAVCC\n" );

	avcC->configurationVersion	= data[idx]; idx++;
	avcC->AVCProfileIndication  = data[idx]; idx++;
	avcC->profile_compatibility = data[idx]; idx++;
	avcC->ACVLevelIndication	= data[idx]; idx++;

	reserved6BitsAndLengthSizeMinusOne= data[idx]; idx++;
	avcC->naluLengthSize = (reserved6BitsAndLengthSizeMinusOne&0x03) + 1;
	if( avcC->naluLengthSize != 4 && avcC->naluLengthSize != 2 && avcC->naluLengthSize != 1 )
	{
		dlog( "bad naluLengthSize: %d\n", avcC->naluLengthSize );
		return kFskErrBadData;
	}

	reserved3BitsAndNumberofSequenceParameterSets = data[idx]; idx++;
	avcC->numberofSequenceParameterSets = reserved3BitsAndNumberofSequenceParameterSets&0x1F;
	if(avcC->numberofSequenceParameterSets!=1)
	{
		dlog( "none 1 avcC->numberofSequenceParameterSets: %d\n", avcC->numberofSequenceParameterSets );
		return kFskErrUnimplemented;
	}
	avcC->spsSize = (data[idx]<<8)|data[idx+1]; idx += 2;
	if(avcC->spsSize > 256)
	{
		dlog( "avcC->spsSize: %d, > 256\n", avcC->spsSize );
		return kFskErrBadData;
	}

	avcC->sps		 = avcC->spspps;
	SetSizeByLen( avcC->sps, avcC->naluLengthSize, avcC->spsSize );
	avcC->sps		 += avcC->naluLengthSize;
	avcC->spsppsSize = avcC->naluLengthSize + avcC->spsSize;

	FskMemCopy((void *)avcC->sps, (const void *)&data[idx], avcC->spsSize);
	idx += avcC->spsSize;

	avcC->numberofPictureParameterSets = data[idx]; idx++;;
	if(avcC->numberofPictureParameterSets!=1)
	{
		dlog( "none 1 avcC->numberofPictureParameterSets: %d\n", avcC->numberofPictureParameterSets );
		return kFskErrUnimplemented;
	}

	avcC->ppsSize = (data[idx]<<8)|data[idx+1]; idx += 2;
	if(avcC->ppsSize > 256)
	{
		dlog( "avcC->ppsSize: %d, > 256\n", avcC->ppsSize );
		return kFskErrBadData;
	}

	avcC->pps		 = avcC->spspps + avcC->naluLengthSize + avcC->spsSize;
	SetSizeByLen( avcC->pps, avcC->naluLengthSize, avcC->ppsSize );
	avcC->pps		 += avcC->naluLengthSize;
	avcC->spsppsSize += avcC->naluLengthSize + avcC->ppsSize;

	FskMemCopy((void *)avcC->pps, (const void *)&data[idx], avcC->ppsSize);

	dlog( "out of DecAVCC\n" );

	return 0;
}


int until_next_start_code( unsigned char *d, int size )
{
	unsigned char *first = d;
	unsigned char *last = d + size - SC_LEN;

	if( size < SC_LEN )
		return size;

	while( d <= last )
	{
		if( d[0] == 0x00 && d[1] == 0x00 && d[2] == 0x00 && d[3] == 0x01  )
			break;

		d++;
	}

	if( d >= last+1 )
		return size;

	return d - first;
}


FskErr FakeAVCC( unsigned char *data, int size, AVCC *avcC )
{
	unsigned char	*next_data	= data;
	int				block_size	= 0;
	int				rest_size	= size;

	FskMemSet( (void *)avcC, 0, sizeof(AVCC) );

	//skip every thing before the first starcode
	block_size = until_next_start_code( data, rest_size );
	if( block_size >= rest_size )//every thing skipped???
		return -1;

	next_data += block_size + SC_LEN;
	rest_size -= block_size + SC_LEN;

	avcC->naluLengthSize = DEFAULT_NALU_LEN;
	avcC->sps = avcC->spspps;

	while( 1 )
	{
		unsigned char nalu_type;

		nalu_type = next_data[0] & NAL_UNITTYPE_BITS;
		block_size = until_next_start_code( next_data, rest_size );

		if( NAL_UT_SPS == nalu_type && avcC->numberofSequenceParameterSets == 0 )
		{
			avcC->spsSize	  = block_size;
			SetSizeByLen( avcC->sps, DEFAULT_NALU_LEN, block_size );
			avcC->sps		 += DEFAULT_NALU_LEN;
			avcC->spsppsSize  = DEFAULT_NALU_LEN + block_size;
			FskMemCopy((void *)avcC->sps, (const void *)next_data, block_size);
			avcC->pps		  = avcC->spspps + avcC->spsppsSize;
			avcC->numberofSequenceParameterSets = 1;
		}
		else if( NAL_UT_PPS == nalu_type && avcC->numberofPictureParameterSets == 0 && avcC->numberofSequenceParameterSets == 1)
		{
			avcC->ppsSize	  = block_size;
			SetSizeByLen( avcC->pps, DEFAULT_NALU_LEN, block_size );
			avcC->pps		 += DEFAULT_NALU_LEN;
			avcC->spsppsSize += DEFAULT_NALU_LEN + block_size;
			FskMemCopy((void *)avcC->pps, (const void *)next_data, block_size);
			avcC->numberofPictureParameterSets = 1;
		}

		next_data += block_size + SC_LEN;
		rest_size -= block_size + SC_LEN;

		if( rest_size <= 0 )
			break;
	}

	if( avcC->numberofSequenceParameterSets == 0 || avcC->numberofPictureParameterSets == 0 )
		return -1;
	else
		return 0;
}

int get_nalu_size( unsigned char *data, int nalu_len_size )
{
	unsigned char *s = (unsigned char *)data;
	int			  src_size;

	if( nalu_len_size == 4 )
		src_size = (s[0]<<24)|(s[1]<<16)|(s[2]<<8)|(s[3]);
	else if(  nalu_len_size == 2 )
		src_size = (s[0]<<8)|(s[1]<<0);
	else
		src_size = s[0];

	return src_size;
}

//check next frame nalu and part at the first byte, adjust size
static int check_next_frame_nalu( int is_startcode, int nalu_len_size, unsigned char **data_in_out, int *size_in_out, int *nalu_type_out, int *ref_idc_out, int *block_size_out )
{
	unsigned char	*next_data	= *data_in_out;
	int				block_size	= 0;
	int				rest_size	= *size_in_out;
	int				qt_current_box_size = 0;

	dlog( "\n");
	dlog( "into check_next_frame_nalu\n" );
#if 0
	{ unsigned char *d = &next_data[0]; dlog( "*size_in_out:%d => %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",  *size_in_out, d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7], d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]  ); }
#endif
	*nalu_type_out = 0;
	*ref_idc_out   = 0;

	if( rest_size <= 0 )
		return -1;

	if( is_startcode )
		block_size = until_next_start_code( next_data, rest_size );
	else
	{
		block_size = 0;
		qt_current_box_size = get_nalu_size( next_data, nalu_len_size );
		dlog( "qt_current_box_size: %d\n", qt_current_box_size );
	}

	if( block_size >= rest_size )
		return -1;

	next_data += block_size + nalu_len_size;
	rest_size -= block_size + nalu_len_size;

	while( 1 )
	{
		unsigned char nalu_type;
		unsigned char ref_idc;

		nalu_type  = next_data[0] & NAL_UNITTYPE_BITS;
		ref_idc    = next_data[0] & NAL_REF_IDC_BITS;

		if( is_startcode )
			block_size = until_next_start_code( next_data, rest_size );
		else
		{
			unsigned char *n_d = next_data + qt_current_box_size;
			block_size = qt_current_box_size;
			qt_current_box_size = get_nalu_size( n_d, nalu_len_size );
		}

		if( (NAL_UT_SLICE == nalu_type )  || (NAL_UT_IDR_SLICE == nalu_type ) )
		{
			*nalu_type_out  = nalu_type;
			*ref_idc_out    = ref_idc;
			*block_size_out	= block_size;
			*data_in_out	= next_data;
			*size_in_out	= rest_size;

			dlog( "out of check_next_frame_nalu, nalu_type: %d, ref_idc: %d, block_size: %d, rest_size: %d\n", nalu_type, ref_idc, block_size, *size_in_out );
			return 0;
		}

		next_data += block_size + nalu_len_size;
		rest_size -= block_size + nalu_len_size;

		if( rest_size <= 0 )
			break;
	}

	return -1;
}

static void RefitBitmap( FskBitmapFormatEnum dst_pixel_format, float rotation, int width, int height, FskBitmap *bits )
{
	FskBitmap			b = *bits;
	FskRectangleRecord	bounds;
	int err;

	FskBitmapGetBounds(b, (FskRectangle)&bounds);
	if((width != bounds.width) || (height != bounds.height))
	{
		FskBitmapDispose(b);
		err = FskBitmapNew((SInt32)width, (SInt32)height, dst_pixel_format, &b);
		if (err)
			b = NULL;
	}

	if( b != NULL )
		b->rotation = (SInt32)rotation;

	*bits = b;
}

FskErr AndroidJavaVideoDecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
	dlog( "\n###########################################################################################\n" );
	dlog( "into AndroidJavaVideoDecodeCanHandle: format: %d, mime: %s\n", (int)format, mime );

	*canHandle =	('avc1' == format) || (0 == FskStrCompare(mime, "x-video-codec/avc")) ||
					('mp4v' == format) || (0 == FskStrCompare(mime, "x-video-codec/mp4"));
	dlog( "mime %s canHandle = %d \n", mime, (int)*canHandle );
	if (*canHandle == 1 && (0 == FskStrCompare(mime, "x-video-codec/avc"))) {
		g_mime = MIME_AVC;
	}
	else if (*canHandle == 1 && (0 == FskStrCompare(mime, "x-video-codec/mp4"))) {
		g_mime = MIME_M4V;
	}

	return kFskErrNone;
}

enum DECODE_FRAME_TYPE{
	/* frame type */
	CONFIGURE_FLAG_ENCODE     = 0, //decoder only
	BUFFER_FLAG_SYNC_FRAME    = 1, //sync frame
	BUFFER_FLAG_CODEC_CONFIG  = 2, //configure data
	BUFFER_FLAG_END_OF_STREAM = 4  //end of frame
} ;

static void chartoByteArray(JNIEnv* env, jbyteArray jBuf, unsigned char* buf, int nOutSize)
{
	dlog( "into chartoByteArray... \n" );
	jbyte *by = (jbyte*)buf;
	(*env)->SetByteArrayRegion(env, jBuf, 0, nOutSize, by);
}

static char *ByteArraytochar(JNIEnv* env, jbyteArray jarry, int *size)
{
	dlog( "into ByteArraytochar... \n" );
	jbyte *arrayBody = (*env)->GetByteArrayElements(env, jarry, 0);
	*size = (*env)->GetArrayLength(env, jarry);
	char *ptr = (char *)arrayBody;

	return ptr;
}

extern void Init_JNI_Env();
extern jclass gMediaCodecCoreClass;
static int GetMediaCodecInstance(JNIEnv *jniEnv, void *stateIn)
{
	FskAndroidJavaVideoDecode *state = (FskAndroidJavaVideoDecode *)stateIn;
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
	FskAndroidJavaVideoDecode *state = (FskAndroidJavaVideoDecode *)stateIn;
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
	FskAndroidJavaVideoDecode *state = (FskAndroidJavaVideoDecode *)stateIn;
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

    /* function MediaSetMediaCodecInstance */
	if (state->F_MediaSetMediaCodecInstance_ID == NULL) {
		state->F_MediaSetMediaCodecInstance_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "MediaSetMediaCodecInstance", "(Landroid/media/MediaCodec;)V");
		if (state->F_MediaSetMediaCodecInstance_ID == NULL) {
			dlog( "Mapping function F_MediaSetMediaCodecInstance_ID error!\n" );
			return -5;
		}
		dlog( "Mapping function F_MediaSetMediaCodecInstance_ID OK!\n" );
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

	/* function VideoDeQueueOutputBuffer */
	if (state->F_VideoDeQueueOutputBuffer_ID == NULL) {
		state->F_VideoDeQueueOutputBuffer_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "VideoDeQueueOutputBuffer","()Ljava/nio/ByteBuffer;");
		if (state->F_VideoDeQueueOutputBuffer_ID == NULL) {
			dlog( "Mapping function F_VideoDeQueueOutputBuffer_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_VideoDeQueueOutputBuffer_ID OK!\n" );
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

	/* function VideoCheckInputBufferAvail */
	if (state->F_VideoCheckInputBufferAvail_ID == NULL) {
		state->F_VideoCheckInputBufferAvail_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "VideoCheckInputBufferAvail","(I)I");
		if (state->F_VideoCheckInputBufferAvail_ID == NULL) {
			dlog( "Mapping function F_VideoCheckInputBufferAvail_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function F_VideoCheckInputBufferAvail_ID OK!\n" );
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

//FILE *fp = NULL;
FskErr AndroidJavaVideoDecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension)
{
	FskAndroidJavaVideoDecode *state;
	FskErr err;

	err = FskMemPtrNewClear(sizeof(FskAndroidJavaVideoDecode), (FskMemPtr *)&state);
	BAIL_IF_ERR( err );

	dlog( "\n###########################################################################################\n" );
	dlog( "in AndroidJavaVideoDecodeNew allocated state: %x\n", (int)state );

	err = FskListMutexNew(&state->func_item_list, "FuncItemList");
	BAIL_IF_ERR( err );

	state->bad_state		= 0;
	state->nalu_len_size	= kDefaultNALULengthSize;
	state->rotation_float	= 0;
	state->rotation			= kRotationNone;
	state->opbuf_changed	= 0;
	state->enrolling_numbers= 0;
    state->wr_size          = 0;

	state->deco				= deco;
	state->dst_pixel_format = YUV420_FORMAT;
	state->qcft_init_flag	= 0;
#ifdef Performance_Profiling
	state->total_time		= 0;
	state->avege_time		= 0.0;
#endif

//	fp = fopen("/sdcard/jstream.avc", "wb");

bail:
	if (kFskErrNone != err)
		AndroidJavaVideoDecodeDispose(state, deco);

	deco->state = state;

	dlog( "out of AndroidJavaVideoDecodeNew: err: %d\n", (int)err );

	return err;
}

FskErr AndroidJavaVideoDecodeDispose(void *stateIn, FskImageDecompress deco)
{
	FskAndroidJavaVideoDecode *state = (FskAndroidJavaVideoDecode *)stateIn;
	int i;

	dlog( "\n###########################################################################################\n" );
	dlog( "into AndroidJavaVideoDecodeDispose\n" );

	if (NULL != state)
	{
		FskThread self          = FskThreadGetCurrent();
		JNIEnv *jniEnv			= (JNIEnv *)self->jniEnv;

		if( state->sampleDescription != NULL )
			FskMemPtrDispose(state->sampleDescription);

		if( state->y0 != NULL )
			FskMemPtrDispose(state->y0);

		if( state->u0 != NULL )
			FskMemPtrDispose(state->u0);

		if( state->v0 != NULL )
			FskMemPtrDispose(state->v0);

		for (i = 0; i < kBitmapCacheSize; i++)
			FskBitmapDispose(state->bitmaps[i]);


		if( state->func_item_list != NULL )
		{
			dlog( "calling func_queue_flush\n" );
			func_queue_flush( deco, state->func_item_list, kFskErrShutdown );
			FskListMutexDispose(state->func_item_list);
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

		FskMemPtrDispose(state);
	}

//	fclose(fp);
	return kFskErrNone;
}

static void nv12TileYBlockOutput(unsigned char* dst, unsigned char* src, int bwidth, int bheight, int stride_dst, int stride_src)
{
	int i, j;
	unsigned char* dst_y = NULL;
	unsigned char* src_y = NULL;

	for(j = 0; j < bheight; j++) {
		dst_y = dst + j * stride_dst;
		src_y = src + j * stride_src;
		for(i = 0; i < bwidth; i++) {
			*(dst_y++) = *(src_y++);
		}
	}
}

static void nv12TileUVBlockOutput(unsigned char* dst_u, unsigned char* dst_v, unsigned char* src, int bwidth, int bheight, int stride_dst, int stride_src)
{
	int i, j;
	unsigned char* u = NULL;
	unsigned char* v = NULL;
	unsigned char* src_uv = NULL;

	for(j = 0; j < bheight; j++) {
		u = dst_u + j * stride_dst;
		v = dst_v + j * stride_dst;
		src_uv = src + j * stride_src;
		for(i = 0; i < bwidth; i++) {
			*(u++) = *src_uv;
			src_uv += 2;
		}
		src_uv = src + j * stride_src + 1;
		for(i = 0; i < bwidth; i++) {
			*(v++) = *src_uv;
			src_uv += 2;
		}
	}
}

static void nv12TileCalBlockIdxY(int col_num, int row_num)
{
	int i, j;
	int base, offset;

	for(j = 0; j < row_num; j++) {
		if((j & 1) == 0) {
			base = j * col_num;
			if((row_num & 1) && (j == (row_num - 1))) {
				for(i = 0; i < col_num; i++) {
					offset = i;
					nv12tile_index_matrix[j][i] = base + offset;
				}
			} else {
				for(i = 0; i < col_num; i++) {
					offset = i + ((i + 2) & ~3);
					nv12tile_index_matrix[j][i] = base + offset;
				}
			}
		} else {
			for(i = 0; i < col_num; i++) {
				base = (j & (~1)) * col_num + 2;
				offset = i + (i & (~3));
				nv12tile_index_matrix[j][i] = base + offset;
			}
		}
	}
}

static void nv12TileCalBlockIdxUV(int col_num, int row_num)
{
	int i, j;
	int base, offset;

	for(j = 0; j < row_num; j++) {
		if((j & 1) == 0) {
			base = j * col_num;
			if((row_num & 1) && (j == (row_num - 1))) {
				for(i = 0; i < col_num; i++) {
					offset = i;
					nv12tile_index_matrix_uv[j][i] = base + offset;
				}
			} else {
				for(i = 0; i < col_num; i++) {
					offset = i + ((i + 2) & ~3);
					nv12tile_index_matrix_uv[j][i] = base + offset;
				}
			}
		} else {
			for(i = 0; i < col_num; i++) {
				base = (j & (~1)) * col_num + 2;
				offset = i + (i & (~3));
				nv12tile_index_matrix_uv[j][i] = base + offset;
			}
		}
	}
}

int refit_qcom_64x32_tile(FskAndroidJavaVideoDecode *state, int colf, int strd, int slht, unsigned char *yuv_data, unsigned char **dst_y, unsigned char **dst_u, unsigned char **dst_v)
{
	int yuv_width  = strd;
	int yuv_height = slht;
	int uv_width  = yuv_width/2;
	int uv_height = yuv_height/2;
	int uv_size   = uv_width*uv_height;

	int cnt_x = (yuv_width - 1) / NV12T_WIDTH + 1;
	int cnt_real_x = (cnt_x + 1) & ~1;

	int cnt_y_y = (yuv_height - 1) / NV12T_HEIGHT + 1;
	int size_y = cnt_real_x * cnt_y_y * NV12T_SIZE;

	unsigned char *src_y = yuv_data;
	unsigned char *src_uv;

	int i, j;
	int err = 0;

	dlog( "in refit_qcom_64x32_tile, colf: %x, strd: %d, slht: %d\n", colf, strd, slht );

	if((size_y % NV12T_GROUP_SIZE) != 0) {
		size_y = (((size_y - 1) / NV12T_GROUP_SIZE) + 1) * NV12T_GROUP_SIZE;
	}

	if(state->y0 == NULL)
	{
		dlog("allocating y0, size: %d\n", yuv_width*yuv_height);
		err = FskMemPtrNew(yuv_width*yuv_height, (FskMemPtr *)&state->y0);
		BAIL_IF_ERR(err);
	}

	if( state->u0 == NULL )
	{
		dlog("allocating u0, size: %d\n", uv_size);
		err = FskMemPtrNew(uv_size, (FskMemPtr *)&state->u0);
		BAIL_IF_ERR(err);
	}

	if( state->v0 == NULL )
	{
		dlog("allocating v0, size: %d\n", uv_size);
		err = FskMemPtrNew(uv_size, (FskMemPtr *)&state->v0);
		BAIL_IF_ERR(err);
	}

	src_uv = src_y + size_y;

	int row_left, col_left;

	dlog("yuv_height/cnt_y_y/yuv_width/cnt_x: %d/%d/%d/%d\n", yuv_height, cnt_y_y, yuv_width, cnt_x );

	for(j = 0, row_left = yuv_height; j < cnt_y_y; j++, row_left -= NV12T_HEIGHT)
	{
		for(i = 0, col_left = yuv_width; i < cnt_x; i++, col_left -= NV12T_WIDTH)
		{
			int block_width = (col_left > NV12T_WIDTH ? NV12T_WIDTH : col_left);
			int block_height = (row_left > NV12T_HEIGHT ? NV12T_HEIGHT : row_left);
			unsigned char* block_y = src_y + nv12tile_index_matrix[j][i] * NV12T_SIZE;
			unsigned char* y = state->y0 + j * yuv_width * NV12T_HEIGHT + i * NV12T_WIDTH;

			nv12TileYBlockOutput(y, block_y, block_width, block_height, yuv_width, NV12T_WIDTH);

			unsigned char* block_uv = src_uv + nv12tile_index_matrix_uv[j/2][i] * NV12T_SIZE + ((j & 1) ? NV12T_SIZE/2 : 0);
			unsigned char* u = state->u0 + j * uv_width * (NV12T_HEIGHT/2) + i * (NV12T_WIDTH/2);

			unsigned char* v = state->v0 + j * uv_width * (NV12T_HEIGHT/2) + i * (NV12T_WIDTH/2);

			nv12TileUVBlockOutput(u, v, block_uv, block_width/2, block_height/2, uv_width, NV12T_WIDTH);
		}
	}

	*dst_y = state->y0;
	*dst_u = state->u0;
	*dst_v = state->v0;

bail:
	return err;
}

static FskErr send_out_one_pic( FskAndroidJavaVideoDecode *state, unsigned char *buf, int width, int height, int stride_w, int stride_h, int size, int colorfmt)
{
	FskImageDecompress		deco	  = state->deco;
	JNIEnv                  *jniEnv	  = state->jniEnv;
	FskBitmapFormatEnum						pixelFormat;
	unsigned char			*dstPtr		= NULL;
	int						dst_y_rb;
	int						rot_width, rot_height;
	int						src_width	= width;
	int						src_height	= height;
	int						top			= 0;
	int						left		= 0;
	int						widh_stride	= stride_w;
	int						heig_stride = stride_h;
	unsigned char			*pY, *yuv_data;
	unsigned char			*pU;
	unsigned char			*pV;

	FskImageDecompressComplete	completionFunction = NULL;
	void					*completionRefcon  = NULL;

	int						frame_number = 0;
	int						drop_flag	 = 0;
	FskBitmap				bits		 = NULL;
	FskErr					err			 = kFskErrNone;

	yuv_data = pY = buf + top * widh_stride + left * 2;
	dlog( "into send_out_one_pic, src_W = %d, src_H = %d, stride_w = %d, stride_h = %d\n", src_width, src_height, widh_stride, heig_stride );

	while(1)
	{
		FskInt64	input_decode_time  = 0;

		dlog("retrieving completion function\n" );
		err = func_queue_out( state->func_item_list, 	&completionFunction,  &completionRefcon, &frame_number, &drop_flag, &input_decode_time );
		if( err != 0 )
		{
			dlog("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: no completion function, we should always have a completionFunction to return a bits!!!\n");
			err = 0;
			goto bail;
		}

		if( completionFunction == NULL )
		{
			dlog("got a NULL completion function, must be EOS!!!\n" );
		}

		dlog("retrieved completion function: %x, refcon: %x, frame_number: %d, drop_flag: %d\n", (int)completionFunction, (int)completionRefcon, (int)frame_number, drop_flag );
		break;
	}

	/*if( drop_flag )	//player asked to drop, or flusing
	{
		dlog("this frame is dropped per player's request\n" );
		if( completionFunction != NULL )
		{
			dlog( "drop frame, completionFunction = %x, completionRefcon = %x, frame number = %d", (int)completionFunction, (int)completionRefcon, (int)frame_number );
			(completionFunction)(deco, completionRefcon, kFskErrNone, NULL);
		}
		goto bail;
	}*/

	state->debug_output_frame_count++;

	if( state->rotation == kRotationCW90 || state->rotation == kRotationCW270 )
	{
		rot_width  = src_height;
		rot_height = src_width;
	}
	else
	{
		rot_width  = src_width;
		rot_height = src_height;
	}

	/* can we do this ? */
	if (colorfmt == 19) { //COLOR_FormatYUV420Planar
		state->dst_pixel_format = kFskBitmapFormatYUV420;
	}
	else if (colorfmt == 21) { //COLOR_FormatYUV420SemiPlanar
		state->dst_pixel_format = kFskBitmapFormatYUV420spuv;
	}
	else if (colorfmt == 0x7fa30c03) { //QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka
		state->dst_pixel_format = kFskBitmapFormatYUV420;
		if (state->qcft_init_flag == 0) {
			int count_x = ((widh_stride - 1) / NV12T_WIDTH + 2) & ~1;
			int count_y = (heig_stride - 1) / NV12T_HEIGHT + 1;
			int count_uv_y = (heig_stride/2 - 1) / NV12T_HEIGHT + 1;

			nv12TileCalBlockIdxY(count_x, count_y);
			nv12TileCalBlockIdxUV(count_x, count_uv_y);
			state->qcft_init_flag = 1;
		}
	}
	else if (colorfmt == 0x7f000100) { //COLOR_TI_FormatYUV420PackedSemiPlanar
		state->dst_pixel_format = kFskBitmapFormatYUV420spuv;
	}
	dlog( "reschedule dst_pixel_format = %d\n", state->dst_pixel_format );

	if( state->dst_pixel_format == kFskBitmapFormatYUV420i )
	{
		rot_width  = ((rot_width +1)>>1)<<1;	//force even width and height
		rot_height = ((rot_height+1)>>1)<<1;
	}

	if( deco->bits != NULL )
	{
		bits	   = deco->bits;
		deco->bits = NULL;
		RefitBitmap( state->dst_pixel_format, state->rotation_float, rot_width, rot_height, &deco->bits );
	}
	else
	{
		int i, index = -1;

		for (i = 0; i < kBitmapCacheSize; i++)
		{
			if (state->bitmaps[i])
			{
				if (0 == state->bitmaps[i]->useCount)
				{
					index = i;
					RefitBitmap( state->dst_pixel_format, state->rotation_float, rot_width, rot_height, &state->bitmaps[i] );
					bits = state->bitmaps[i];
					break;
				}
			}
			else
				index = i;
		}

		if( bits == NULL )
		{
			err = FskBitmapNew((SInt32)rot_width, (SInt32)rot_height, state->dst_pixel_format, &bits);
			if (err) goto bail;
			bits->rotation = (SInt32)state->rotation_float;
		}

		if( index != -1 )
		{
			state->bitmaps[index] = bits;
			FskBitmapUse( bits );
			dlog("try to find out the max index = %d\n", index);
		}
	}

	FskBitmapWriteBegin( bits, (void**)(&dstPtr), (SInt32 *)&dst_y_rb, &pixelFormat );
	dlog("rot_width = %d, rot_height = %d, dst_y_rb = %d, pixelFormat = %d\n", rot_width, rot_height, (int)dst_y_rb, (int)pixelFormat);

	if( state->dst_pixel_format == kFskBitmapFormatYUV420spuv)
	{
		int i;
		dlog( "state->dst_pixel_format == kFskBitmapFormatYUV420spuv case\n" );
		if (colorfmt == 21) {//COLOR_FormatYUV420SemiPlanar
			unsigned char *dst_y = dstPtr;
			unsigned char *dst_u = dst_y + (dst_y_rb * (rot_height +(rot_height & 0)));
			pU = pY + heig_stride * widh_stride;

			for (i=0; i<rot_height; i++) {
				FskMemCopy(dst_y+i*dst_y_rb, pY+i*widh_stride, rot_width);
			}
			for (i=0; i<rot_height/2; i++) {
				FskMemCopy(dst_u+i*dst_y_rb, pU+i*widh_stride, rot_width);
			}
		}
		else if (colorfmt == 0x7f000100) {//COLOR_TI_FormatYUV420PackedSemiPlanar
			unsigned char *dst_y = dstPtr;
			unsigned char *dst_u = dst_y + (dst_y_rb * (rot_height +(rot_height & 0)));
			int data_offset	= (*jniEnv)->GetIntField(jniEnv, state->mMediaCodecCore, state->V_CROP_LT_ID);
			int crop_top	= data_offset/widh_stride;
			int crop_left	= data_offset - crop_top*widh_stride;
			dlog( "crop_left = %d, crop_top = %d, stride = %d, slice_height = %d\n", crop_left, crop_top, widh_stride, heig_stride );
			pU = pY + heig_stride * widh_stride;
			pY += crop_top*widh_stride + crop_left;
			pU += (crop_top>>1)*widh_stride + crop_left;

			for (i=0; i<rot_height; i++) {
				FskMemCopy(dst_y+i*dst_y_rb, pY+i*widh_stride, rot_width);
			}
			for (i=0; i<rot_height/2; i++) {
				FskMemCopy(dst_u+i*dst_y_rb, pU+i*widh_stride, rot_width);
			}
		}
	}
	else if( state->dst_pixel_format == kFskBitmapFormatYUV420 )
	{
		dlog( "state->dst_pixel_format == kFskBitmapFormatYUV420 case\n" );
		if (colorfmt == 19) { //COLOR_FormatYUV420Planar
			unsigned char *dst_y = dstPtr;
			unsigned char *dst_u = dst_y + (dst_y_rb * (rot_height +(rot_height & 0)));
			unsigned char *dst_v = dst_u + (dst_y_rb >> 1) * ((rot_height + 1) >> 1);

			dlog( "YUV420 case, source format == COLOR_FormatYUV420Planar\n" );
			pU = pY + heig_stride * widh_stride;
			pV = pU + heig_stride * widh_stride/4;

			FskYUV420Copy(	rot_width, rot_height,
						  pY, pU, pV, widh_stride, widh_stride>>1,
						  dst_y, dst_u, dst_v,
						  dst_y_rb, dst_y_rb >> 1);
		}
		else if (colorfmt == 0x7fa30c03) { //QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka
			unsigned char *dst_y = dstPtr;
			unsigned char *dst_u = dst_y + (dst_y_rb * (rot_height +(rot_height & 0)));
			unsigned char *dst_v = dst_u + (dst_y_rb >> 1) * ((rot_height + 1) >> 1);

			dlog( "YUV420 case, source format == QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka\n" );
			err = refit_qcom_64x32_tile(state, colorfmt, widh_stride, heig_stride, yuv_data, &pY, &pU, &pV );
			if (err) goto bail;

			FskYUV420Copy(	rot_width, rot_height,
						  pY, pU, pV, widh_stride, widh_stride>>1,
						  dst_y, dst_u, dst_v,
						  dst_y_rb, dst_y_rb >> 1);
		}
	}

	dlog("returning a bits: %x\n", (int)bits);
	FskBitmapWriteEnd( bits );

	if( completionFunction != NULL )
			(completionFunction)(deco, completionRefcon, kFskErrNone, bits);

bail:
	dlog( "Write to output file, size = %d", size );

	return err;
}

static int size_to_start_code(unsigned char *data, int data_size, int start_code_type, int nalu_len_size)
{
	unsigned char *p = data;
	int size = 0, used_bytes = 0;

	if (start_code_type == 4) {
		while (1) {
			size = get_nalu_size(p, nalu_len_size);
			p[0] = 0x0;
			p[1] = 0x0;
			p[2] = 0x0;
			p[3] = 0x1;
			p += 4 + size;
			used_bytes += 4 + size;
			if (used_bytes >= data_size) {
				break;
			}
		}
	}
	else {
		;//not implemented!
	}

	return used_bytes;
}

FskErr AndroidJavaVideoDecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data_in, UInt32 dataSize_in, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	FskAndroidJavaVideoDecode		*state			= (FskAndroidJavaVideoDecode *)stateIn;
	JNIEnv              *jniEnv			= state->jniEnv;
	QTImageDescription	desc			= (QTImageDescription)state->sampleDescription;
	unsigned char		*data			= (unsigned char *)data_in;
	int					data_size		= (int)dataSize_in;
	int					is_startcode	= desc == NULL;
	int					is_eos			= data == NULL;
	FskInt64			decode_time		= (decodeTime==NULL||is_eos) ? 0 : *decodeTime;
	int					drop_frame		= (0 != (frameType & kFskImageFrameDrop));
	int					immediate_frame	= (0 != (frameType & kFskImageFrameImmediate));
	int					sync_frame		= ( (frameType&0xff) == kFskImageFrameTypeSync )||(immediate_frame);
	FskErr				err				= 0;
	jbyteArray			InputBuf		= NULL;
	jbyteArray			InputCng		= NULL;
	unsigned char		*fdata			= NULL;
	unsigned char		*esds			= NULL;
	int					esds_size		= 0;

	dlog( "\n###########################################################################################\n" );
	dlog( "into AndroidJavaVideoDecodeDecompressFrame, dataSize: %d, is_startcode: %d, drop_frame: %d, immediate_frame: %d, sync_frame: %d, is_eos: %d\n",
			(int)data_size, (int)is_startcode, (int)drop_frame, (int)immediate_frame, (int)sync_frame, (int)is_eos );
	dlog( "requestedWidth: %d, requestedHeight %d\n", (int)deco->requestedWidth, (int)deco->requestedHeight );

	if( state->bad_state )
	{
		if( deco->completionFunction != NULL && deco->completionRefcon != NULL )
		{
			dlog("decoder is not properly initialized, returning data and returning kFskErrBadState!\n");
			deco->completionFunction(deco, deco->completionRefcon, kFskErrShutdown, NULL);
			deco->completionFunction = NULL;
			deco->completionRefcon = NULL;
		}
		return kFskErrBadState;
	}

	if( is_eos||immediate_frame ) //can not support scan process now! createDecoderByType does not return!!!
		goto bail; //bnie
	else {
		dlog( "calling func_queue_in, completionFunction: %x, completionRefcon: %x, frame number = %d\n", (int)deco->completionFunction,  (int)deco->completionRefcon, (int)state->debug_input_frame_count );
		err = func_queue_in(  state->func_item_list, deco->completionFunction, deco->completionRefcon, state->debug_input_frame_count, drop_frame, decode_time );
		BAIL_IF_ERR( err );
		state->enrolling_numbers ++;

		deco->completionFunction = NULL;
		deco->completionRefcon = NULL;
	}

	state->debug_input_frame_count++;

	if (sync_frame) //I
		state->flags = BUFFER_FLAG_SYNC_FRAME; //decoder configure data
	else
		state->flags = 0;

	if (state->mC_MediaFormat == NULL) {
		if (g_mime == MIME_AVC) { //for H.264
			if( !is_startcode )	//normal case
			{
				unsigned char	*avcc_data = NULL;

				dlog( "trying to get avcC from desc\n" );
				avcc_data = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)desc, 'avcC');
				if( avcc_data != NULL )
				{
					dlog( "got avcC from desc and decoding it\n" );
					dlog( "avcc_data: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
							avcc_data[0],avcc_data[1],avcc_data[2],avcc_data[3],avcc_data[4],avcc_data[5],avcc_data[6],avcc_data[7],
							avcc_data[8],avcc_data[9],avcc_data[10],avcc_data[11],avcc_data[12],avcc_data[13],avcc_data[14],avcc_data[15]  );
					err = DecAVCC( avcc_data, &state->avcC );
					BAIL_IF_ERR( err );
				}
				else
				{
					dlog( "no avcC, trying to get it from privately Kinoma defined spsp(runtime only)\n" );
					//***bnie: for now!!!??? :
					avcc_data = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)desc, 'spsp');
					if( avcc_data == NULL )
					{
						BAIL( kFskErrBadData );
					}

					dlog( "avcc_data: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
							avcc_data[0],avcc_data[1],avcc_data[2],avcc_data[3],avcc_data[4],avcc_data[5],avcc_data[6],avcc_data[7],
							avcc_data[8],avcc_data[9],avcc_data[10],avcc_data[11],avcc_data[12],avcc_data[13],avcc_data[14],avcc_data[15]  );
					state->avcC.naluLengthSize = 4;
					state->avcC.spsppsSize = (unsigned short)FskMisaligned32_GetN(avcc_data) - 8;
					dlog( "state->avcC.spsppsSize: %d\n", state->avcC.spsppsSize );
					avcc_data += 8;
					FskMemCopy( (void *)state->avcC.spspps, (const void *)avcc_data, state->avcC.spsppsSize );

					{
						unsigned char *d = state->avcC.spspps;
						state->avcC.spsSize = FskMisaligned32_GetBtoN( d );
						d += 4;
						state->avcC.sps = d;
						d += state->avcC.spsSize;
						state->avcC.ppsSize = FskMisaligned32_GetBtoN( d );
						d += 4;
						state->avcC.pps = d;
					}
				}
#if SUPPORT_INSTRUMENTATION
				{ unsigned char *d = &state->avcC.spspps[0]; dlog( "spsppsSize:%d => %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",    state->avcC.spsppsSize, d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7], d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]  ); }
				{ unsigned char *d = &state->avcC.sps[0];	 dlog( "spsSize:%d    => %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",    state->avcC.spsSize,    d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7], d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15] );}
				{ unsigned char *d = &state->avcC.pps[0];    dlog( "ppsSize:%d    => %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",    state->avcC.ppsSize,    d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7], d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]  ); }
#endif
			}
			else //is_startcode
			{
				dlog( "no desc, fake avcC from input frame data\n" );
				err = FakeAVCC( data, data_size, &state->avcC );
				BAIL_IF_ERR( err );
			}

			state->nalu_len_size = state->avcC.naluLengthSize;
			state->nale_pre		 = is_startcode ? 4 : state->nalu_len_size;
		}
		else if (g_mime == MIME_M4V) { //for MP4V
			if( desc != NULL )
			{
				dlog( "trying to get esds from desc, width/height: %d, %d\n", desc->width, desc->height );
				esds = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)desc, 'esds');
			}
			if( esds != NULL )
			{
				int i = 0;

				esds_size = FskMisaligned32_GetN(esds) - 8;
				esds    += 8;

				dlog("esds_size: %d:: %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n",
						esds_size,
						esds[0],esds[1],esds[2],esds[3],esds[4],esds[5],esds[6],esds[7],esds[8], esds[9],
						esds[10],esds[11],esds[12],esds[13],esds[14],esds[15],esds[16],esds[17],esds[18],esds[19],
						esds[20],esds[21],esds[22],esds[23],esds[24],esds[25],esds[26],esds[27],esds[28],esds[29],
						esds[30],esds[31],esds[32],esds[33],esds[34],esds[35],esds[36],esds[37],esds[38],esds[39],
						esds[40],esds[41],esds[42],esds[43],esds[44],esds[45],esds[46],esds[47],esds[48],esds[49] );

				//find first start code
				for( i = 0; i < esds_size; i++ )
					if( esds[i] == 0 && esds[i+1] == 0 && esds[i+2] == 1 )
						break;

				esds	 += i;
				esds_size -= i;

				dlog("esds_size: %d:: %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n",
						esds_size, esds[0],esds[1],esds[2],esds[3],esds[4],esds[5],esds[6],esds[7],esds[8],
						esds[9],esds[10],esds[11],esds[12],esds[13],esds[14],esds[15] );
			}
			else
			{//not support H263 decoding.
                BAIL( kFskErrUnimplemented );
			}
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
			case MIME_AVC:
				state->mimeType = (*jniEnv)->NewStringUTF(jniEnv, "video/avc");
				break;
			case MIME_M4V:
				state->mimeType = (*jniEnv)->NewStringUTF(jniEnv, "video/mp4v-es");
				break;
			default:
				dlog( "Can not support this CODEC!!!\n" );
				break;
		}
#if SUPPORT_INSTRUMENTATION
		{
			char *cstr = (char *)(*jniEnv)->GetStringUTFChars(jniEnv, state->mimeType, 0);
			dlog( "Create %s Decoder by Type Begin here ...\n", cstr );
		}
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
		jmethodID	F_createVideoFormat_ID = NULL;

		if (C_MediaFormat == NULL) {
			C_MediaFormat = (*jniEnv)->FindClass(jniEnv, "android/media/MediaFormat");
			if(C_MediaFormat == NULL){
				dlog( "Can not find MediaFormat class, error!\n" );
				return kFskErrNotFound;
			}
			dlog( "Mapping class MediaFormat OK!\n" );
		}

		if (F_createVideoFormat_ID == NULL) {
			F_createVideoFormat_ID = (*jniEnv)->GetStaticMethodID(jniEnv, C_MediaFormat, "createVideoFormat",
																  "(Ljava/lang/String;II)Landroid/media/MediaFormat;");
			if (F_createVideoFormat_ID == NULL) {
				(*jniEnv)->DeleteLocalRef(jniEnv, C_MediaFormat);
				return kFskErrNotFound;
			}
			dlog( "Mapping function createVideoFormat OK!\n" );
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
		dlog( "Call createVideoFormat here ...!\n" );
		if (desc != NULL ) {
			dlog("%x desc->width: %d, desc->height: %d\n", (int)state, desc->width, desc->height);
			state->display_width  = desc->width;
			state->display_height = desc->height;
		}
		if( state->display_width == 0 )
			state->display_width  = 320;
		if( state->display_height == 0 )
			state->display_height = 240;
		dlog("%x state->display_width: %d, desc->height: %d\n", (int)state, state->display_width, state->display_height);

		jobject objf = (*jniEnv)->CallStaticObjectMethod(jniEnv, C_MediaFormat, F_createVideoFormat_ID, state->mimeType, state->display_width, state->display_height);
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
		state->flags |= BUFFER_FLAG_CODEC_CONFIG;
	} /* Initialize End */

#ifdef Performance_Profiling
	struct timeval tv_begin;
	gettimeofday(&tv_begin, NULL);
#endif

	/* prepare input bytebuffer */
	switch (g_mime) {
		case MIME_AVC:
			if ( (state->flags&0x2) == BUFFER_FLAG_CODEC_CONFIG) {
				dlog( "rerange input data as codec configure data ...\n" );
				int d_size = state->avcC.spsSize + state->avcC.ppsSize + 8;
				fdata = malloc(d_size);
				unsigned char *p = fdata;

				*p ++ = 0x0;
				*p ++ = 0x0;
				*p ++ = 0x0;
				*p ++ = 0x1;
				FskMemCopy((void *)p, (const void *)state->avcC.sps, state->avcC.spsSize);

				p += state->avcC.spsSize;
				*p ++ = 0x0;
				*p ++ = 0x0;
				*p ++ = 0x0;
				*p ++ = 0x1;
				FskMemCopy((void *)p, (const void *)state->avcC.pps, state->avcC.ppsSize);

				InputCng = (*jniEnv)->NewByteArray(jniEnv, d_size);
				chartoByteArray(jniEnv, InputCng, (unsigned char *)fdata, (int)d_size);
				dlog( "SPS&PPS size is %d\n", d_size);
//				fwrite(fdata, 1, d_size, fp); //@@@ SPS PPS
                (*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_MediaQueueInputBufferSize_ID, InputCng, BUFFER_FLAG_CODEC_CONFIG);

				jbyte* jpoint = (*jniEnv)->GetByteArrayElements(jniEnv, InputCng, 0);
				(*jniEnv)->ReleaseByteArrayElements(jniEnv, InputCng, jpoint, 0);
				(*jniEnv)->DeleteLocalRef(jniEnv, InputCng);

				size_to_start_code(data, data_size, 4, state->nalu_len_size);
//				fwrite(data, 1, data_size, fp); //@@@ I First

				InputBuf = (*jniEnv)->NewByteArray(jniEnv, data_size);
				chartoByteArray(jniEnv, InputBuf, (unsigned char *)data, (int)data_size);
				state->flags = BUFFER_FLAG_SYNC_FRAME;
			}
			else {
				dlog( "rerange input data as normal cases ...\n" );
				size_to_start_code(data, data_size, 4, state->nalu_len_size);
//				fwrite(data, 1, data_size, fp); //@@@ I P B

				InputBuf = (*jniEnv)->NewByteArray(jniEnv, data_size);
				chartoByteArray(jniEnv, InputBuf, (unsigned char *)data, (int)data_size);
			}
			break;

		case MIME_M4V:
			if ( (state->flags&0x2) == BUFFER_FLAG_CODEC_CONFIG) {
				dlog( "rerange input data as codec configure data ...\n" );
				int d_size = esds_size;
				fdata = malloc(d_size);

				FskMemCopy((void *)(fdata), (const void *)esds, esds_size);

				InputCng = (*jniEnv)->NewByteArray(jniEnv, d_size);
				chartoByteArray(jniEnv, InputCng, (unsigned char *)fdata, (int)d_size);
				dlog( "vol size is %d\n", d_size);
                (*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_MediaQueueInputBufferSize_ID, InputCng, BUFFER_FLAG_CODEC_CONFIG);

                jbyte* jpoint = (*jniEnv)->GetByteArrayElements(jniEnv, InputCng, 0);
				(*jniEnv)->ReleaseByteArrayElements(jniEnv, InputCng, jpoint, 0);
				(*jniEnv)->DeleteLocalRef(jniEnv, InputCng);

				InputBuf = (*jniEnv)->NewByteArray(jniEnv, data_size);
				chartoByteArray(jniEnv, InputBuf, (unsigned char *)data, (int)data_size);
				state->flags = BUFFER_FLAG_SYNC_FRAME;
			}
			else {
				dlog( "rerange input data as normal cases ...\n" );
				InputBuf = (*jniEnv)->NewByteArray(jniEnv, data_size);
				chartoByteArray(jniEnv, InputBuf, (unsigned char *)data, (int)data_size);
			}
			break;

		default:
			break;
	}

	/* Main loop */
    int done = 0;
	do
    {
        int index;
		unsigned char *Outbuf;
        jobject obj_bbf = NULL;

        index = (int)(*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_VideoCheckInputBufferAvail_ID, state->flags);
        dlog( " ### input buffer index is: %d\n", index );

        if (index >= 0) {
            err = (FskErr)(*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_MediaQueueDataToInput_ID, InputBuf, index, state->flags);
            BAIL_IF_ERR(err);

            /* try to release byteArray here... */
            if (InputBuf) {
                dlog( "release InputBuf here ...\n");
                (*jniEnv)->DeleteLocalRef(jniEnv, InputBuf);
                InputBuf = NULL;
            }
            done = 1;
        } else if (index == 0xff) {
            BAIL(kFskErrBadState);
        }

        /* check output buffer */
        do {
            obj_bbf = (jbyteArray)(*jniEnv)->CallObjectMethod(jniEnv, state->mMediaCodecCore, state->F_VideoDeQueueOutputBuffer_ID);
            dlog( " ### output buffer available : 0x%x\n", (int)obj_bbf );

            if (state->strideW == 0 || state->strideH == 0) {
                int CODEC_STATUS = (*jniEnv)->GetIntField(jniEnv, state->mMediaCodecCore, state->V_CODEC_STATUS_ID);
                dlog( "1. Get codec_status value = %d", CODEC_STATUS );

                if ((CODEC_STATUS&0x4) == 0x4) {
                    int stride, slice_height;
                    dlog( "update media format here ...\n" );
                    jobject obj_mf = (*jniEnv)->CallObjectMethod(jniEnv, state->mC_MediaCodec, state->F_getOutputFormat_ID);
                    /* Keys */
                    jstring KEY_WIDTH			= (*jniEnv)->NewStringUTF(jniEnv, "width");				//string of width
                    jstring KEY_HEIGHT			= (*jniEnv)->NewStringUTF(jniEnv, "height");			//string of height
                    jstring KEY_STRIDE			= (*jniEnv)->NewStringUTF(jniEnv, "stride");			//string of stride
                    jstring KEY_SLICE_HEIGHT	= (*jniEnv)->NewStringUTF(jniEnv, "slice-height");		//string of slice-height
                    jstring KEY_COLOR_FORMAT	= (*jniEnv)->NewStringUTF(jniEnv, "color-format");		//string of color-format
                    /* MediaFormat Key tests */
                    state->strideW	= (*jniEnv)->CallIntMethod(jniEnv, obj_mf, state->F_getInteger_ID, KEY_WIDTH);
                    state->strideH	= (*jniEnv)->CallIntMethod(jniEnv, obj_mf, state->F_getInteger_ID, KEY_HEIGHT);
                    stride			= (*jniEnv)->CallIntMethod(jniEnv, obj_mf, state->F_getInteger_ID, KEY_STRIDE);
                    slice_height	= (*jniEnv)->CallIntMethod(jniEnv, obj_mf, state->F_getInteger_ID, KEY_SLICE_HEIGHT);
                    state->color_format	= (*jniEnv)->CallIntMethod(jniEnv, obj_mf, state->F_getInteger_ID, KEY_COLOR_FORMAT);
                    if (!strcmp(modelName, "HUAWEI P6-U06")) { //@@hack, decoder lies.
                        state->color_format = COLOR_TI_FormatYUV420PackedSemiPlanar;
                    }
                    dlog( "Get Key values of '%s', width = %d, height = %d, stride = %d, slice-height = %d, color_format = 0x%x\n", modelName, state->strideW, state->strideH, stride, slice_height, state->color_format);

                    (*jniEnv)->DeleteLocalRef(jniEnv, obj_mf);

                    if (state->strideW == 0 || state->strideH == 0) {
                        err = kFskErrBadState;
                        state->bad_state = 1;
                        goto bail;
                    }
                    else {
                        if (stride > state->strideW ) {
                            state->strideW = stride;
                            dlog("adjust stride by %d\n", state->strideW);
                        }
                        if (slice_height > state->strideH ) {
                            state->strideH = slice_height;
                            dlog("adjust height by %d\n", state->strideH);
                        }
                    }
                }
                else if ((CODEC_STATUS&0x2) == 0x2) {
                    state->opbuf_changed = 1;
                    dlog( "output buffer changed ...\n" );
                }
            }

            if (obj_bbf != NULL && state->strideW && state->strideH) {
                dlog( "got one frame decoded.. \n");
                if (state->wr_size == 0) {
                    state->wr_size = (int)(*jniEnv)->GetDirectBufferCapacity(jniEnv, obj_bbf); //(*jniEnv)->GetIntField(jniEnv, state->mMediaCodecCore, state->V_CROP_LT_ID);
                    dlog( "picture size: %d!!!\n", (int)state->wr_size);
                }
				Outbuf = (unsigned char *)(*jniEnv)->GetDirectBufferAddress(jniEnv, obj_bbf);
				if ( state->opbuf_changed == 0 && ((!strcmp(modelName, "GT-I9300")) || (!strcmp(modelName, "GT-I9100")) || (!strcmp(modelName, "HUAWEI P6-U06"))) ) { //dimension not changed *
					dlog( "output buffer does not changed, keep using ole dimension \n");
					state->strideW = state->display_width;
					state->strideH = state->display_height;
				} else if ( !strcmp(modelName, "Nexus 5") ) { //@hack, decoder lies about width when it's not integer times of 16 *
					state->strideW = (state->strideW % 16) ? (state->strideW / 16 + 1) * 16 : state->strideW;
// 					state->strideH = (state->strideH % 16) ? (state->strideH / 16 + 1) * 16 : state->strideH;
					dlog( "decoder lies about width when it's not integer times of 16, (%d, %d) \n", state->strideW, state->strideH);
				}
				send_out_one_pic(state, Outbuf, state->display_width, state->display_height, state->strideW, state->strideH, state->wr_size, state->color_format);
				(*jniEnv)->DeleteLocalRef(jniEnv, obj_bbf);
                (*jniEnv)->CallVoidMethod(jniEnv, state->mMediaCodecCore, state->F_MediaReleaseOutputBuffer_ID);

				state->enrolling_numbers --;
            }
        } while(obj_bbf != NULL);

    } while (!done);

bail:
	if (InputBuf != NULL) {
		(*jniEnv)->DeleteLocalRef(jniEnv, InputBuf);
	}

	if (state->mC_MediaFormat == NULL) {
		err = kFskErrBadState;
		state->bad_state = 1;
	}

	if( (is_eos || state->bad_state || (err == kFskErrBadState) || state->enrolling_numbers >= (int)MAX_ENROLLING_FRAMES) && state->func_item_list != NULL )
	{
		FskErr flush_err = kFskErrShutdown;//dec->error_happened ? kFskErrBadData: kFskErrShutdown;
		dlog("flushing completion fucntions!!!\n");

		if (!is_eos)
			state->bad_state = 1;
		dlog("is_eos: %d, state->bad_state: %d, err: %d\n", (int)is_eos, (int)state->bad_state, (int)state->bad_state);

		dlog("calling func_queue_flush()\n");
		func_queue_flush(deco, state->func_item_list, flush_err );
	}

	if (fdata)
		free(fdata);

#ifdef Performance_Profiling
	struct timeval tv_end;
	int cur_time;
	gettimeofday(&tv_end, NULL);
	cur_time = (int)(1000000 * (tv_end.tv_sec - tv_begin.tv_sec) + tv_end.tv_usec - tv_begin.tv_usec);
	state->total_time += cur_time;
	state->avege_time = (double)(state->total_time / state->debug_input_frame_count);
	dlog("Performance_Profiling: total time = %dms, total input = %d, current time = %dms, mean time = %2.2fms\n", (int)(state->total_time/1000), state->debug_input_frame_count, (int)(cur_time/1000), state->avege_time/1000);
#endif

	dlog( "out of AndroidJavaVideoDecodeDecompressFrame: err: %d, in/out: %d/ %d\n", (int)err, state->debug_input_frame_count, state->debug_output_frame_count );
	return err;
}


FskErr AndroidJavaVideoDecodeFlush(void *stateIn, FskImageDecompress deco )
{
	FskAndroidJavaVideoDecode		*state	= (FskAndroidJavaVideoDecode *)stateIn;
	FskErr				err		= kFskErrNone;
	FskThread self          = FskThreadGetCurrent();
	JNIEnv *jniEnv			= (JNIEnv *)self->jniEnv;

	dlog( "\n###########################################################################################\n" );
	dlog( "into AndroidJavaVideoDecodeFlush\n");

	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!\n");
		return kFskErrBadState;
	}

	if( state->func_item_list != NULL )
	{
		FskErr flush_err = kFskErrShutdown;//dec->error_happened ? kFskErrBadData: kFskErrShutdown;

		dlog("calling func_queue_flush()\n");
		func_queue_flush(deco, state->func_item_list, flush_err );
	}

	/* call MediaCodec close */
	if (state->mC_MediaFormat && state->mMediaCodecCore && state->F_flush_ID && state->mC_MediaCodec) {
		dlog( "Call MediaCodec flush ...");
		(*jniEnv)->CallVoidMethod(jniEnv, state->mMediaCodecCore, state->F_flush_ID, state->mC_MediaCodec);
	}
	state->debug_input_frame_count  = 0;
	state->debug_output_frame_count	= 0;

//bail:
	dlog( "out of AndroidJavaVideoDecodeFlush: err: %d\n", (int)err );
	return err;
}

#include <dlfcn.h>

FskErr AndroidJavaVideoDecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	FskAndroidJavaVideoDecode *state  = (FskAndroidJavaVideoDecode *)stateIn;
	unsigned char *data     = (unsigned char *)deco->data;
	int			  data_size = (int)deco->dataSize;
	int			  nalu_type;
	int			  ref_idc;
	int			  block_size;
	UInt32		  frame_type = 0;
	int			  is_startcode = 1;
	FskErr		  err = kFskErrNone;

	dlog( "\n###########################################################################################\n" );
	dlog( "into AndroidJavaVideoDecodeGetMetaData, indec: %d\n", (int)index );

	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!\n");
		return kFskErrBadState;
	}

	if (kFskImageDecompressMetaDataFrameType != metadata)
	{
		BAIL( kFskErrUnimplemented );
	}

	if( index == 0 )
		is_startcode = 1;
	else if( index == 1 )
		is_startcode = 0;

	err = check_next_frame_nalu( is_startcode, state->nalu_len_size, &data, &data_size, &nalu_type, &ref_idc, &block_size );
	if( err != 0 )
	{
		BAIL( kFskErrBadData );
	}

	if( nalu_type == 5 )
		frame_type = kFskImageFrameTypeSync;
	else if( ref_idc != 0 )
		frame_type = kFskImageFrameTypeDifference;
	else
		frame_type = kFskImageFrameTypeDroppable | kFskImageFrameTypeDifference;

	if( value != NULL )
	{
		value->type = kFskMediaPropertyTypeInteger;
		value->value.integer = frame_type;
	}

bail:
	dlog( "out of AndroidJavaVideoDecodeGetMetaData: err: %d\n", (int)err );
	return err;
}


FskErr AndroidJavaVideoDecodeSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskAndroidJavaVideoDecode *state = (FskAndroidJavaVideoDecode *)stateIn;

	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!\n");
		return kFskErrBadState;
	}

	dlog( "\n###########################################################################################\n" );
	dlog( "into AndroidJavaVideoDecodeSetSampleDescription\n");

	state->sampleDescriptionSeed++;
	if( state->sampleDescription != NULL )
		FskMemPtrDisposeAt((void **)&state->sampleDescription);

	state->sampleDescriptionSize = property->value.data.dataSize;

	return FskMemPtrNewFromData(state->sampleDescriptionSize, property->value.data.data, (FskMemPtr *)&state->sampleDescription);
}

#define SET_PREFERRED_PIXEL_FORMAT( want_this_format )					\
	if( prefered_yuvFormat == kFskBitmapFormatUnknown )					\
	{																	\
		for( i = 0; i < count; i++ )									\
		{																\
			FskBitmapFormatEnum this_format = (FskBitmapFormatEnum)property->value.integers.integer[i];	\
			if( this_format == want_this_format )						\
			{															\
				prefered_yuvFormat = want_this_format;					\
				break;													\
			}															\
		}																\
	}

FskErr AndroidJavaVideoDecodeSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property )
{
	FskAndroidJavaVideoDecode *state = (FskAndroidJavaVideoDecode *)stateIn;
	FskBitmapFormatEnum prefered_yuvFormat = kFskBitmapFormatUnknown;
	UInt32 i,count = property->value.integers.count;

	dlog( "\n###########################################################################################\n" );
	dlog( "into vmetaDecodeSetPreferredPixelFormat, propertyID: %d, propertyType: %d, count: %d\n", (int)propertyID, (int)property->type, (int)count);
	dlog( "prefered_yuvFormat: %d/%d/%d/%d/%d\n", (int)property->value.integers.integer[0],(int)property->value.integers.integer[1],(int)property->value.integers.integer[2],(int)property->value.integers.integer[3],(int)property->value.integers.integer[4]);

	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!\n");
		return kFskErrBadState;
	}

	dlog( "looking for kFskBitmapFormatYUV420spuv\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420spuv)

	dlog( "looking for kFskBitmapFormatYUV420\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420)

	dlog( "looking for kFskBitmapFormatYUV420i\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420i)

	if( prefered_yuvFormat != kFskBitmapFormatUnknown )
	{
		dlog( "got matched system preferred: %d\n", (int)prefered_yuvFormat);
		state->dst_pixel_format = prefered_yuvFormat;
	}

	dlog( "state->dst_pixel_format: %d\n", (int)state->dst_pixel_format);

	return kFskErrNone;
}

FskErr AndroidJavaVideoDecodeGetMaxFramesToQueue (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskAndroidJavaVideoDecode *state = (FskAndroidJavaVideoDecode *)stateIn;
	FskErr				err	 = kFskErrNone;

	dlog( "\n###########################################################################################" );
	dlog( "into AndroidJavaVideoDecodeGetMaxFramesToQueue, propertyID: %d", (int)propertyID );

	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!");
		return state->bad_state;
	}

	property->value.integer	= (int)MAX_ENROLLING_FRAMES;
	property->type			= kFskMediaPropertyTypeInteger;

	return err;
}


FskErr AndroidJavaVideoDecodeSetRotation(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskAndroidJavaVideoDecode *state = (FskAndroidJavaVideoDecode *)stateIn;

	dlog( "\n###########################################################################################\n" );
	dlog( "into AndroidJavaVideoDecodeSetRotation\n");

	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!\n");
		return kFskErrBadState;
	}

#ifndef SUPPORT_ROTATION
	return kFskErrNone;
#endif

	if( state->dst_pixel_format == kFskBitmapFormatYUV420 )
		return kFskErrNone;

	state->rotation_float = (float)property->value.number;

	if( state->rotation_float >= 45 && state->rotation_float < 135 )
		state->rotation = kRotationCW90;
	else if( state->rotation_float >= 135 && state->rotation_float < 225 )
		state->rotation = kRotationCW180;
	else if( state->rotation_float >= 225 && state->rotation_float < 315 )
		state->rotation = kRotationCW270;
	else
		state->rotation = kRotationNone;

	return kFskErrNone;
}
