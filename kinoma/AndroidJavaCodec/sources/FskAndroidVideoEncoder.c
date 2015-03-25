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
#define __FSKBITMAP_PRIV__
#define __FSKTHREAD_PRIV__

#include "FskAndroidVideoEncoder.h"
#include "kinoma_utilities.h"
#include "FskEndian.h"
#include "FskBitmap.h"
#include "QTReader.h"
#include <jni.h>

#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
FskInstrumentedTypeRecord gAndroidJavaVideoEncoderTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "FskAndroidJavaVideoEncoder"};
#define dlog(...)  do { FskInstrumentedTypePrintfDebug  (&gAndroidJavaVideoEncoderTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define dlog(...)
#endif

typedef struct
{
	unsigned char	sizeofNALULength;
	long			width;
	long			height;

	unsigned char	profile_id;
	unsigned char	profile_iop;
	unsigned char	constraint_set0_flag;
	unsigned char	constraint_set1_flag;
	unsigned char	constraint_set2_flag;
	unsigned char	level_idc;

	long			spsCount;
	unsigned char	*sps[32];
	short			spsSizes[32];

	long			ppsCount;
	unsigned char	*pps[32];
	short			ppsSizes[32];
} Overview;

typedef struct
{
	long				fps;
	long				key_fps;
	long				bps;
	long				timeScale;
    unsigned char		*desc;
	long				descSize;

	//env JNI
	JNIEnv				*jniEnv;
	//MediaCodecCore
	jclass				MediaCodecCore;
	jobject				mMediaCodecCore;
	jmethodID			F_EncodeVideoFrame_ID;
	jmethodID			F_close_ID;
	jmethodID			F_CodecFormatAvailable_ID;
	jfieldID			V_CODEC_STATUS_ID;
	jfieldID			V_CROP_LT_ID;
	//MediaCodec Android
	jclass				C_MediaCodec;
	jobject				mC_MediaCodec;
	jmethodID			F_createEncoderByType_ID;
	jmethodID			F_getOutputFormat_ID;
	jmethodID			F_configure_ID;
	jmethodID			F_start_ID;
	//MediaFormat Android
	jstring				mimeType;
	int					width;
	int					height;
	jobject				mC_MediaFormat;
	jmethodID			F_setInteger_ID;
	//Main Loop
	int					flags;
	int					color_format;
	int					opbuf_changed;
    jint                key_color_format;
    int                 frame_encoded;

	AVCC				avcC;
    jbyteArray          InputBuf;
	int					qcft_init_flag;
    double              total_time;
	unsigned char		*y0;
	unsigned char		*u0;
	unsigned char		*v0;

} FskAndroidJavaVideoEncode;

/* const values */
static const jint COLOR_FormatYUV420Planar 				= 19; 			//Nexus 7, Coolpad7295
static const jint COLOR_FormatYUV420PackedPlanar        = 20;           //
static const jint COLOR_FormatYUV420SemiPlanar 			= 21;			//XiaoMi, Galaxy NoteII(VU), Galaxy S2
static const jint COLOR_TI_FormatYUV420PackedSemiPlanar = 0x7f000100;	//Galaxy Nexus
static const jint COLOR_QCOM_FormatYUV420SemiPlanar     = 0x7fa30c00;   //QCOM

static const jint CONFIGURE_FLAG_ENCODE                 = 1;            //for encoder

enum DECODE_FRAME_TYPE{
	/* frame type */
	BUFFER_FLAG_SYNC_FRAME    = 1, //sync frame
	BUFFER_FLAG_CODEC_CONFIG  = 2, //configure data
	BUFFER_FLAG_END_OF_STREAM = 4  //end of frame
} ;

#define WriteLongB( v, p )  { p[0] = ((v)&0xff000000)>>24; p[1] = ((v)&0x00ff0000)>>16 ; p[2] = ((v)&0x0000ff00)>>8; p[3] = ((v)&0x000000ff); }
#define WriteShortB( v, p ) { p[0] = ((v)&0xff00)>>8; p[1] = ((v)&0x00ff); }
#define MAX_SPSPPS_SIZE 64 //??
#define DEFAULT_STREAM_BUF_SIZE 1024*500 //??

enum
{
	k_nalu_type_slice_layer_no_partition = 1,
	k_nalu_type_slice_data_partition_a_layer,
	k_nalu_type_slice_data_partition_b_layer,
	k_nalu_type_slice_data_partition_c_layer,
	k_nalu_type_slice_layer_no_partition_IDR,
	k_nalu_type_supplement_enhancement_info,
	k_nalu_type_seq_parameter_set,
	k_nalu_type_pic_parameter_set,
	k_nalu_type_access_unit_delimiter
};

static void YUV420PlanarToYUV420SemiPlanar(int is_uv, unsigned char* buf, int width, int height) {
	int frame_s = width * height;
	unsigned char *uv_start = (unsigned char *)malloc(frame_s/2);
    unsigned char *uv = uv_start;
    unsigned char *u = buf + frame_s;
    unsigned char *v = u + frame_s/4;
	int i;

	dlog("into YUV420PlanarToYUV420SemiPlanar ...");

    if (is_uv) { //UV
        for (i=0; i<frame_s/4; i++) {
            *uv ++ = *u ++;
            *uv ++ = *v ++;

        }
    }
    else { //VU
        for (i=0; i<frame_s/4; i++) {
            *uv ++ = *v ++;
            *uv ++ = *u ++;
        }
    }

    memcpy(buf+frame_s, uv_start, frame_s/2);
	free(uv_start);
}

static void YUV420SemiPlanarToYUV420Planar(int is_uv, unsigned char* buf, int width, int height) {
	int frame_s = width * height;
	unsigned char *uv_tmp = (unsigned char *)malloc(frame_s/2);
	unsigned char *u = uv_tmp;
	unsigned char *v = u + frame_s/4;
    unsigned char *uv_in = buf + frame_s;
	int i;
    
	dlog("into YUV420SemiPlanarToYUV420Planar ...");
    
    if( is_uv )
        for (i=0; i<frame_s/4; i++)
        {
            *u++ = *uv_in++;
            *v++ = *uv_in++;
        }
    else
        for (i=0; i<frame_s/4; i++)
        {
            *v++ = *uv_in++;
            *u++ = *uv_in++;
        }
    
    memcpy( buf + frame_s, uv_tmp, frame_s/2);
    
	free(uv_tmp);
}

static void YUV420SemiPlanarExchangeUV(unsigned char* buf, int width, int height) {
	int frame_s = width * height;
    unsigned char *uv = buf + frame_s;
    unsigned char tmp;
	int i;

	dlog("into YUV420SemiPlanarExchangeUV ...");
    for (i=0; i<frame_s/4; i++)
    {
        tmp = *uv;
        *uv = *(uv+1);
        *(uv+1) = tmp;
        uv += 2;
    }
}

/*
 srcColor: color format of source input, from YUV file or Camera.
 dstColor: color fomrat encoder supported.
 */
static int colorTransfer(unsigned char* buf, int srcColor, int dstColor, int width, int height) {
    int w = width;
    int h = height;
	
    if( srcColor == kFskBitmapFormatYUV420 )
    {
        switch (dstColor) {
            case 21:
            case 0x7f000100:
                dlog("transfer from YUV420Planar to YUV420SemiPlanar_%x", dstColor);
                YUV420PlanarToYUV420SemiPlanar(1, buf, w, h);
                break;
                
            default:
                break;
        }
    }
    else if( srcColor == kFskBitmapFormatYUV420spuv )
    {
        switch (dstColor) {
            case 19:
                dlog("transfer from YUV420SemiPlanarUV to YUV420Planar");
                YUV420SemiPlanarToYUV420Planar(1, buf, w, h);
                break;
                
            default:
                break;
        }
    }
    else if( srcColor == kFskBitmapFormatYUV420spvu )
    {
        switch (dstColor) {
            case 19:
                dlog("transfer from YUV420SemiPlanarVU to YUV420Planar");
                YUV420SemiPlanarToYUV420Planar(0, buf, w, h);
                break;

            case 21:
            case 0x7f000100:
                dlog("transfer from YUV420SemiPlanarVU to YUV420SemiPlanarUV");
                YUV420SemiPlanarExchangeUV(buf, w, h);
                break;
                
            default:
                break;
        }
    }
    else
    {
        dlog("Are you sure input format is xxx ???");
            return -1; //may invalid format of input
    }
        
	return 0;
}

static short GetSize( unsigned char *p, unsigned char *x )
{
	short size = 0;

	while(1)
	{
		int hit3 = p[0] == 0 && p[1]== 0 && p[2] == 1;
		int hit4 = p[0] == 0 && p[1]== 0 && p[2] == 0 && p[3] == 1;

		if( hit3 || hit4 )
			break;

		size++;
		p++;
		if( p >= x )
			break;

		continue;
	}

	return size;
}

static int analysis_stream_basic( unsigned char *p, long size, unsigned char *naluType, unsigned char *naluOffset )
{
	long i = 0;
	long lastPos = -1;
    short offset = 0;;
    unsigned char *begin = p;
    unsigned char *end = p+size;
    int hit3 = begin[i] == 0 && begin[i+1]== 0 && begin[i+2] == 1;
    int hit4 = begin[i] == 0 && begin[i+1]== 0 && begin[i+2] == 0 && begin[i+3] == 1;

	*naluType = 0;

    //In case, stream does not start as start code.
    while( !hit3 && !hit4 )
    {
        i++;
        if( i >= size - 3 )
        {
            i = size;
            return -2; //no start code at all.
        }
        hit3 = begin[i] == 0 && begin[i+1]== 0 && begin[i+2] == 1;
        hit4 = begin[i] == 0 && begin[i+1]== 0 && begin[i+2] == 0 && begin[i+3] == 1;
    }

    if( hit3 )//we always expect start code to be 0x 00 00 00 01
        return -1;

    if (!hit4) {
        return -2; //we always do not expect this happen.
    }

    begin += i;
    while( *naluType == 0 )
    {
        unsigned char naluByte      = begin[4];
        unsigned char forbidden_bit = (naluByte>>7)&1;
        unsigned char nal_unit_type = (naluByte)&0x1f;
        dlog( "nal_unit_type = %d, when analysising .", nal_unit_type );

        if(	forbidden_bit == 0 && (
            nal_unit_type == k_nalu_type_slice_layer_no_partition	  ||
            nal_unit_type == k_nalu_type_slice_data_partition_a_layer ||
            nal_unit_type == k_nalu_type_slice_data_partition_b_layer ||
            nal_unit_type == k_nalu_type_slice_data_partition_c_layer ||
            nal_unit_type == k_nalu_type_slice_layer_no_partition_IDR) )
        {
            *naluType = nal_unit_type;
            break;
        } else {
            offset = GetSize(begin+4, end);
            begin += offset+4;
            dlog( "offset = %d, begin[0]=%d, begin[1]=%d, begin[2]=%d, begin[3]=%d, begin[4]=%d", offset+4, begin[0], begin[1], begin[2], begin[3], begin[4] );
            continue;
        }
    }

    *naluOffset = begin - p;
	return 0;
}

static void SetupDefaultKeyValues(void *stateIn)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;

    if (state->fps == -1) {
        state->fps = 30;
    }
    if (state->bps == -1) { //assume compression ratio is 1/32.
        state->bps = ((state->width * state->height * 3 / 2) * 8 * state->fps ) / 32;
    }
    if (state->key_fps == -1) {
        state->key_fps = 3;
    }

    return;
}

static void GetOverview( long width, long height, unsigned char *p, long size, Overview *ov )
{
	int i = 0;

	if( size < 4 )
		return;

	ov->spsCount	= 0;
	ov->ppsCount	= 0;
	ov->sps[0]		= NULL;
	ov->pps[0]		= NULL;
	ov->spsSizes[0] = 0;
	ov->ppsSizes[0] = 0;
	ov->width		= width;
	ov->height		= height;
	ov->sizeofNALULength = 4;

	while(1)
	{
		int hit3 = p[i] == 0 && p[i+1]== 0 && p[i+2] ==1;
		int hit4 = p[i] == 0 && p[i+1]== 0 && p[i+2] == 0 && p[i+3] == 1;

		if( !hit3 && !hit4 )
		{
			i++;
			if( i >= size - 1 )
				break;
			continue;
		}

		//until we hit start code
		i += hit3 ? 3 : 4;
		if( i >= size - 1 )
			break;

		unsigned char forbidden_bit		= (p[i]>>7) & 1;
		unsigned char nal_reference_idc = (p[i]>>5) & 3;
		unsigned char nal_unit_type		= (p[i]) & 0x1f;

		if( nal_unit_type == k_nalu_type_seq_parameter_set && ov->spsCount == 0 )
		{
			ov->profile_id  = p[i+1];
			ov->profile_iop = p[i+2];
			ov->level_idc   = p[i+3];
			ov->sps[0]      = &p[i];
			ov->spsSizes[0] = GetSize( ov->sps[0], p + size );
			ov->spsCount++;

		}
		else if(  nal_unit_type == k_nalu_type_pic_parameter_set && ov->ppsCount == 0 )
		{
			ov->pps[0]      = &p[i];
			ov->ppsSizes[0] = GetSize( ov->pps[0], p + size );
			ov->ppsCount++;
		}

		if( ov->spsCount == 1 && ov->ppsCount == 1 )
			break;
	}
}

#define FskEndianU32_NtoX(a) (a)  //FskEndianU32_NtoB
#define FskEndianU16_NtoX(a) (a)  //FskEndianU16_NtoB
#define WriteLongX( v, p )   FskMisaligned32_PutN( &v, p ) //WriteLongB( v, p )

static long OverviewCreateImageDesc(  Overview *ov, unsigned char **imageDesc, long *imageDescSize )
{
	long			i, size, avcCSize;
	unsigned char	*p = NULL;
	QTImageDescriptionRecord *idp = NULL;
	unsigned char	*avcC;
	long			err = 0;

	avcCSize = 7;	//minimum
	for( i = 0; i < ov->spsCount; i++ )
		avcCSize += 2 + ov->spsSizes[i];

	for( i = 0; i < ov->ppsCount; i++ )
		avcCSize += 2 + ov->ppsSizes[i];

	size = sizeof(QTImageDescriptionRecord) + 8 + avcCSize;
	err = FskMemPtrNewClear( size, (FskMemPtr*)&p);
	if (0 != err)
		goto bail;

	idp  = (QTImageDescriptionRecord *)p;
	avcC = (unsigned char *)( idp + 1 );

	idp->idSize			= FskEndianU32_NtoX(size);
	idp->cType			= FskEndianU32_NtoX('avc1');
	idp->resvd1			= FskEndianU32_NtoX(0);							// must be zero
	idp->resvd2			= FskEndianU16_NtoX(0);							// must be zero
	idp->dataRefIndex	= FskEndianU16_NtoX(0);						// must be zero
	idp->version		= FskEndianU16_NtoX(1);							// version of codec data format
	idp->revisionLevel	= FskEndianU16_NtoX(1);						// revision of codec data format
	idp->vendor			= FskEndianU32_NtoX('kino');			// Apple
	idp->temporalQuality= FskEndianU32_NtoX(0);					// no temporal compression
	idp->spatialQuality = FskEndianU32_NtoX(512);	// we could be clever, but nobody would care
	idp->width			= FskEndianU16_NtoX(ov->width);
	idp->height			= FskEndianU16_NtoX(ov->height);
	idp->hRes			= FskEndianU32_NtoX(72<<16);							// dots-per-inch
	idp->vRes			= FskEndianU32_NtoX(72<<16);							// dots-per-inch
	idp->dataSize		= FskEndianU32_NtoX(0);							// zero if unknown, which it is
	idp->frameCount		= FskEndianU16_NtoX(1);						// one frame at a time
	idp->name[0]		= 5;
	idp->name[1]		= 'H';
	idp->name[2]		= '.';
	idp->name[3]		= '2';
	idp->name[4]		= '6';
	idp->name[5]		= '4';
	idp->depth			= FskEndianU16_NtoX(24);							// color.
	idp->clutID			= FskEndianU16_NtoX(-1);							// not using a clut

	avcCSize += 8;
	WriteLongX( avcCSize, avcC );
	//*(long *)(&avcC[0]) = FskEndianU32_NtoB(avcCSize + 8);	//size
	avcC += 4;

	{
		long fourCC = 'avcC';
		WriteLongX( fourCC, avcC );
		//*(long *)(&avcC[0]) = FskEndianU32_NtoB('avcC');  //4CC
	}
	avcC += 4;

	avcC[0] = 1;   // version
	avcC++;

	avcC[0] = ov->profile_id;
	avcC++;

	avcC[0] = ov->profile_iop;
	avcC++;

	avcC[0] = ov->level_idc;
	avcC++;

	avcC[0] = 0xFC | ( ov->sizeofNALULength - 1 );
	avcC++;

	avcC[0] = 0xE0 | (UInt8)ov->spsCount;
	avcC++;

	for( i = 0; i < ov->spsCount; i++)
	{
		short spsSize = ov->spsSizes[i];

		WriteShortB( spsSize, avcC );
		avcC += 2;

		FskMemCopy(avcC, ov->sps[i], spsSize);
		avcC += spsSize;
	}

	avcC[0] = (UInt8)ov->ppsCount;
	avcC++;

	for( i = 0; i < ov->spsCount; i++)
	{
		short ppsSize = ov->ppsSizes[i];

		WriteShortB( ppsSize, avcC );
		avcC += 2;

		FskMemCopy(avcC, ov->pps[i], ppsSize);
		avcC += ppsSize;
	}
    
bail:
	if( err != 0 && idp != 0 )
	{
		FskMemPtrDispose( idp );
		*imageDesc		= NULL;
		*imageDescSize	= 0; 
	}
	else
	{
		*imageDesc		= (unsigned char*)idp;
		*imageDescSize	= size; 
	}
    
	return err;
}

static void chartoByteArray(JNIEnv* env, jbyteArray jBuf, unsigned char* buf, int nOutSize)
{
	dlog( "into chartoByteArray... \n" );
	jbyte *by = (jbyte*)buf;
	(*env)->SetByteArrayRegion(env, jBuf, 0, nOutSize, by);
}

static char *ByteArraytochar(JNIEnv* env, jbyteArray jarry, long *size)
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
	FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
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
	FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
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
	/* function createEncoderByType */
	if (state->F_createEncoderByType_ID == NULL) {
		state->F_createEncoderByType_ID = (*jniEnv)->GetStaticMethodID(jniEnv, state->C_MediaCodec, "createEncoderByType",
                                                                       "(Ljava/lang/String;)Landroid/media/MediaCodec;");
		if (state->F_createEncoderByType_ID == NULL) {
			dlog( "Mapping function F_createEncoderByType_ID error!\n" );
			return -2;
		}
		dlog( "Mapping function createEncoderByType OK!\n" );
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
	FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
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

	/* function encOneFrame */
	if (state->F_EncodeVideoFrame_ID == NULL) {
		state->F_EncodeVideoFrame_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "EncodeVideoFrame","(Landroid/media/MediaCodec;[BI)[B");
		if (state->F_EncodeVideoFrame_ID == NULL) {
			dlog( "Mapping function F_EncodeVideoFrame_ID error!\n" );
			return -3;
		}
		dlog( "Mapping function EncodeVideoFrame OK!\n" );
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
/*
    if (state->F_flush_ID == NULL) {
		state->F_flush_ID = (*jniEnv)->GetMethodID(jniEnv, state->MediaCodecCore, "flush", "(Landroid/media/MediaCodec;)V");
		if (state->F_flush_ID == NULL) {
			dlog( "Mapping function F_flush_ID error!\n" );
			return -5;
		}
		dlog( "Mapping function flush OK!\n" );
	}
*/

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

FskErr AndroidJavaVideoEncodeCanHandle(UInt32 format, const char *mime, Boolean *canHandle)
{
    dlog( "## into AndroidJavaVideoEncodeCanHandle ##\n" );
    
	*canHandle =	('avc1' == format) ||
					(mime && (0 == FskStrCompare(mime, "video/avc"))); //AVC

//    *canHandle =	('mp4v' == format) ||
//    (mime && (0 == FskStrCompare(mime, "video/mp4v-es"))); //MPEG4

	return kFskErrNone;
}
//#define DUMP_STREAM
#ifdef DUMP_STREAM
static FILE *fp = NULL;
#endif
FskErr AndroidJavaVideoEncodeNew(FskImageCompress comp)
{
    FskAndroidJavaVideoEncode *state;
	FskErr err = kFskErrNone;

    dlog( "## into AndroidJavaVideoEncodeNew ##\n" );

	err = FskMemPtrNewClear(sizeof(FskAndroidJavaVideoEncode), (FskMemPtr *)&comp->state);
	BAIL_IF_ERR(err);

	state = (FskAndroidJavaVideoEncode *)comp->state;
	//some default value
	state->width		= ((comp->width+15)>>4)<<4;
	state->height		= ((comp->height+15)>>4)<<4;

    //??How to set timeScale??
    dlog( "quick look at default values from ImageCompress");
    dlog( "comp->timScale = %d", comp->timeScale);
	state->timeScale	= comp->timeScale;
	state->bps			= -1;
	state->fps			= -1;
	state->key_fps		= -1;
    state->frame_encoded= 0;
    state->total_time   = 0.0;
#ifdef DUMP_STREAM
    if (NULL == (fp = fopen("/sdcard/tmp/stream.264", "wb")))
        dlog( "can not open stream.264 file ..." );
#endif

bail:
	return err;
}

FskErr AndroidJavaVideoEncodeDispose(void *stateIn, FskImageCompress comp)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;

    dlog( "## into AndroidJavaVideoEncodeDispose ##\n" );

	if(state)
	{
        JNIEnv *jniEnv = state->jniEnv;

        if( state->desc != NULL )
			FskMemPtrDisposeAt((void **)&state->desc);

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
        
		err = FskMemPtrDisposeAt(&comp->state);
        BAIL_IF_ERR(err);
	}

bail:
	return kFskErrNone;
}


void StartcodeToStartlen( unsigned char *avc_bytes, UInt32 *size )
{
    int i = 0, thisOffset = 0;
    int thisSize = size[i++];
    dlog( "thisSize = %d", thisSize );
	while (thisSize) {
        avc_bytes[thisOffset+0] = (unsigned char)(thisSize>>24)&0x000000ff;
		avc_bytes[thisOffset+1] = (unsigned char)(thisSize>>16)&0x000000ff;
		avc_bytes[thisOffset+2] = (unsigned char)(thisSize>>8 )&0x000000ff;
		avc_bytes[thisOffset+3] = (unsigned char)(thisSize>>0 )&0x000000ff;
        thisOffset += thisSize+4;
        thisSize = size[i++];
        dlog( "thisSize = %d", thisSize );
	}
}

FskErr AndroidJavaVideoEncodeCompressFrame(void *stateIn, FskImageCompress comp, FskBitmap bits, const void **data, UInt32 *dataSize, UInt32 *frameDuration, UInt32 *compositionTimeOffset, UInt32 *frameTypeOut)
{
	FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;
    JNIEnv *jniEnv = state->jniEnv;
    FskBitmap		bitmap = bits;
    SInt32			rowBytes;
    UInt32          frameSize;
    FskBitmapFormatEnum	pixelFormat;
    unsigned char	*dstScan = NULL;
    unsigned char	*buffer = NULL;
    unsigned char	*stream = NULL;
    unsigned char   naluType = 0, naluOffset = 0;
    FskRectangleRecord bounds;
    UInt32          outputFrameType = 0;
    jint            srcFormat = 0;
    long            stream_size = 0, wr_size = 0;
    UInt32          naluSize[4] = {0, 0, 0, 0}; //one frame in one calling loop.

    dlog( "## into AndroidJavaVideoEncodeCompressFrame ##  bits: %x, data: %x, dataSize: %x\n", (int)bits, (int)data, (int)dataSize  );

    struct timeval tv_begin, tv_end;
    double dec_time = 0, avg_time = 0;


    if (bitmap) {
        FskBitmapReadBegin(bitmap, (const void**)(&dstScan), &rowBytes, &pixelFormat);
        FskBitmapGetBounds(bitmap, &bounds);
    }

    gettimeofday(&tv_begin, NULL);
    if (state->mC_MediaFormat == NULL) {

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

//		switch (g_mime) {
//			case MIME_AVC:
				state->mimeType = (*jniEnv)->NewStringUTF(jniEnv, "video/avc");
//				break;
//			case MIME_M4V:
//				state->mimeType = (*jniEnv)->NewStringUTF(jniEnv, "video/mp4v-es");
//				break;
//			default:
//				dlog( "Can not support this CODEC!!!\n" );
//				break;
//		}
#if SUPPORT_INSTRUMENTATION
		{
			char *cstr = (char *)(*jniEnv)->GetStringUTFChars(jniEnv, state->mimeType, 0);
			dlog( "Create %s Encoder by Type Begin here ...\n", cstr );
		}
#endif
		/* New MediaCodec by Type */
		jobject obj = (*jniEnv)->CallStaticObjectMethod(jniEnv, state->C_MediaCodec, state->F_createEncoderByType_ID, state->mimeType);
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
		if (state->F_setInteger_ID == NULL) {
			state->F_setInteger_ID = (*jniEnv)->GetMethodID(jniEnv, C_MediaFormat, "setInteger", "(Ljava/lang/String;I)V");
			if (state->F_setInteger_ID == NULL) {
				(*jniEnv)->DeleteLocalRef(jniEnv, C_MediaFormat);
				return -2;
			}
			dlog( "Mapping function setInteger OK!\n" );
		}

		/* call MediaFormat new */
		dlog( "Call createVideoFormat here ...!\n" );
		dlog("%x width: %d, height: %d\n", (int)state, state->width, state->height);

		jobject objf = (*jniEnv)->CallStaticObjectMethod(jniEnv, C_MediaFormat, F_createVideoFormat_ID, state->mimeType, state->width, state->height);
		state->mC_MediaFormat = (*jniEnv)->NewGlobalRef(jniEnv, objf);
		(*jniEnv)->DeleteLocalRef(jniEnv, objf);

        jstring KEY_BIT_RATE 		= (*jniEnv)->NewStringUTF(jniEnv, "bitrate");			//string of bitrate
        jstring KEY_FRAME_RATE 		= (*jniEnv)->NewStringUTF(jniEnv, "frame-rate");		//string of frame-rate
        jstring KEY_COLOR_FORMAT	= (*jniEnv)->NewStringUTF(jniEnv, "color-format");		//string of color-format
        jstring KEY_I_FRAME_INTERVAL= (*jniEnv)->NewStringUTF(jniEnv, "i-frame-interval");	//string of i-frame-interval

        //find a suitable color format encoder supported!
        if (kFskBitmapFormatYUV420spuv == pixelFormat || kFskBitmapFormatYUV420spvu == pixelFormat) {
            srcFormat = COLOR_FormatYUV420SemiPlanar;
        } else if (kFskBitmapFormatYUV420 == pixelFormat) {
            srcFormat = COLOR_FormatYUV420Planar;
        }
        jint key_cf = (jint)(*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_CodecFormatAvailable_ID, state->mimeType, srcFormat);
        if (key_cf == -1) {
            dlog( "could not find any suitable format!" );
            BAIL(kFskErrBadState);
        } else {
            jint color_format_buf[5] = { COLOR_FormatYUV420Planar, COLOR_FormatYUV420PackedPlanar, COLOR_FormatYUV420SemiPlanar, COLOR_TI_FormatYUV420PackedSemiPlanar, COLOR_QCOM_FormatYUV420SemiPlanar };
            int i = 0;
            int check_twice = 0;
            dlog( "check if we got the color format we supported, key_cf = 0x%x", key_cf );
            do {
                for (i=0; i<5; i++) {
                    if (key_cf == color_format_buf[i])
                        break;
                }
                if (i < 5) {
                    dlog( "we got color format we supported! key_cf = 0x%x", key_cf );
                    state->key_color_format = key_cf;
                    break;
                } else if (srcFormat == COLOR_FormatYUV420SemiPlanar) {
                    key_cf = (jint)(*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_CodecFormatAvailable_ID, state->mimeType, COLOR_FormatYUV420Planar);
                    check_twice ++;
                } else if (srcFormat == COLOR_FormatYUV420Planar) {
                    key_cf = (jint)(*jniEnv)->CallIntMethod(jniEnv, state->mMediaCodecCore, state->F_CodecFormatAvailable_ID, state->mimeType, COLOR_FormatYUV420SemiPlanar);
                    check_twice ++;
                } else {
                    dlog( "we don't know how to match pixelFormat = 0x%x, give up!!!", pixelFormat );
                    BAIL(kFskErrBadState);
                }
            } while (check_twice < 2);
        }

        /* set up default key values here */
        SetupDefaultKeyValues((void *)state);
        dlog( "before setting key values into MediaFormat, fps = %d, bps = %d, key_fps = %d", state->fps, state->bps, state->key_fps );
        /* set Key value into MediaFormat */
        (*jniEnv)->CallVoidMethod(jniEnv, state->mC_MediaFormat, state->F_setInteger_ID, KEY_BIT_RATE, state->bps);                 //KEY_BIT_RATE
        (*jniEnv)->CallVoidMethod(jniEnv, state->mC_MediaFormat, state->F_setInteger_ID, KEY_FRAME_RATE, state->fps);               //KEY_FRAME_RATE
        (*jniEnv)->CallVoidMethod(jniEnv, state->mC_MediaFormat, state->F_setInteger_ID, KEY_I_FRAME_INTERVAL, state->key_fps);     //KEY_I_FRAME_INTERVAL
        (*jniEnv)->CallVoidMethod(jniEnv, state->mC_MediaFormat, state->F_setInteger_ID, KEY_COLOR_FORMAT, state->key_color_format);//KEY_COLOR_FORMAT

		/* call MediaCodec configure */
		dlog( "Call MediaCodec configure ...\n");
		(*jniEnv)->CallVoidMethod(jniEnv, state->mC_MediaCodec, state->F_configure_ID, state->mC_MediaFormat, NULL, NULL, CONFIGURE_FLAG_ENCODE);
        /* parameters 0      1     2    3 */
		/* call start */
		dlog( "Call MediaCodec start ...\n");
		(*jniEnv)->CallVoidMethod(jniEnv, state->mC_MediaCodec, state->F_start_ID);
		
		(*jniEnv)->DeleteLocalRef(jniEnv, C_MediaFormat);
		state->flags = 0;
	} /* Initialize End */


    //Check input bitmap
    if (bitmap) {
        if (state->width != bounds.width || state->height != bounds.height) {
           //@@hack, if width or height are not satisfied by encoder's requirement.
            dlog( "@@hack, if width or height are not satisfied by encoder's requirement!" );
            dlog( "bounds->width: %d, bounds->height: %d, state->width: %d, state->height: %d", bounds.width, bounds.height, state->width, state->height );
            UInt32 dstW = state->width;     UInt32 dstH = state->height;
            UInt32 srcW = bounds.width;    UInt32 srcH = bounds.height;
            err = FskMemPtrNewClear(dstW*dstH*3/2, (FskMemPtr*)&buffer);
            BAIL_IF_ERR(err);

            if (dstW > srcW) {
                ;//not implemented yet!
            } else if (dstH > srcH) {
                FskMemCopy(buffer, dstScan, srcH*srcW);
                FskMemCopy(buffer+dstW*dstH, dstScan+srcW*srcH, srcH*srcW/2);
            }
            dstScan = buffer;
        }

        frameSize = state->width * state->height * 3/2; //always support 420 format.
        if (kFskBitmapFormatYUV420 == pixelFormat || kFskBitmapFormatYUV420spuv == pixelFormat || kFskBitmapFormatYUV420spvu == pixelFormat)
        {
            if (strcmp(modelName, "HUAWEI P6-U06") && strcmp(modelName, "GT-I9300")) {//@@corner case: YUV420sp support kFskBitmapFormatYUV420spvu.
                err = colorTransfer((unsigned char *)dstScan, pixelFormat, state->key_color_format, state->width, state->height);
                BAIL_IF_ERR(err);
            }
        }
        else
            BAIL(kFskErrUnsupportedPixelType);

        dlog( "Transfer input Cdata to Jdata ...frameSize = %d\n", frameSize );
        if (state->InputBuf == NULL) {
            dlog( "New Input Buffer to transfer from C to Java. " );
            jbyteArray jba = (*jniEnv)->NewByteArray(jniEnv, frameSize);
            state->InputBuf = (*jniEnv)->NewGlobalRef(jniEnv, jba);
        }
        chartoByteArray(jniEnv, state->InputBuf, (unsigned char *)dstScan, (int)frameSize);
    } else {
        if (state->InputBuf != NULL) {
            /* try to release byteArray here... */
            jbyte* jpoint = (*jniEnv)->GetByteArrayElements(jniEnv, state->InputBuf, 0);
            (*jniEnv)->ReleaseByteArrayElements(jniEnv, state->InputBuf, jpoint, 0);
            (*jniEnv)->DeleteGlobalRef(jniEnv, state->InputBuf);
        }
        state->InputBuf = NULL;
        state->flags = 4;
    }

    /* Main loop */
	do {
		unsigned char *Outbuf;
        UInt32 i = 0;

		dlog( "&&&&&&&& Call EncodeOneFrame &&&&&&& \n");
		jbyteArray OutputBuf = (jbyteArray)(*jniEnv)->CallObjectMethod(jniEnv, state->mMediaCodecCore, state->F_EncodeVideoFrame_ID, state->mC_MediaCodec, state->InputBuf, state->flags);

		int CODEC_STATUS = (*jniEnv)->GetIntField(jniEnv, state->mMediaCodecCore, state->V_CODEC_STATUS_ID);
        dlog( "Get codec_status value = %d", CODEC_STATUS );

        int outDataSize = (*jniEnv)->GetIntField(jniEnv, state->mMediaCodecCore, state->V_CROP_LT_ID);
        dlog( "Get outDataSize value = %d", outDataSize );

		if (CODEC_STATUS == -1) {
			dlog( "Decoding meet problems, we have to shut down decoder right now !!!" );
			BAIL(kFskErrBadState);
		}

		/* get char* from jbyteArray */
		if (OutputBuf != NULL) {

            dlog( "Allocate stream buffer here ..." );
            if (stream == NULL)
                FskMemPtrNewClear(DEFAULT_STREAM_BUF_SIZE, (FskMemPtr *)&stream);

			do {
				Outbuf = (unsigned char *)ByteArraytochar(jniEnv, OutputBuf, &wr_size);
                dlog( "copy data to stream...stream_size = %d, wr_size = %d", stream_size, wr_size );
                wr_size = outDataSize;

                if (stream_size + wr_size > DEFAULT_STREAM_BUF_SIZE) {
                    dlog( "Reallocate stream buffer, buffer size is : %d", stream_size+wr_size );
                    FskMemPtrRealloc(stream_size + wr_size, (FskMemPtr *)&stream);
                }
                FskMemCopy((void *)(stream+stream_size), (const void *)Outbuf, wr_size);
                naluSize[i++] = wr_size-4;
                stream_size += wr_size;
                state->frame_encoded ++;

//                (*jniEnv)->ReleaseByteArrayElements(jniEnv, OutputBuf, (jbyte *)Outbuf, 0);
				(*jniEnv)->DeleteLocalRef(jniEnv, OutputBuf);		//delete local reference

                
                if (state->frame_encoded == 1 && wr_size < MAX_SPSPPS_SIZE)
                {
                    OutputBuf = (jbyteArray)(*jniEnv)->CallObjectMethod(jniEnv, state->mMediaCodecCore, state->F_EncodeVideoFrame_ID, state->mC_MediaCodec, NULL, state->flags);
                    outDataSize = (*jniEnv)->GetIntField(jniEnv, state->mMediaCodecCore, state->V_CROP_LT_ID);
                    dlog( "Get outDataSize value = %d", outDataSize );
                }
                else //send out one frame once
                    OutputBuf = NULL;
			} while(OutputBuf != NULL);

            if (stream_size!=0 && stream) {
                if( state->desc == NULL )
				{
					Overview ov;
                    dlog( "get sample descprition here ..." );
					GetOverview( state->width, state->height, stream, stream_size, &ov );
					OverviewCreateImageDesc( &ov, &state->desc, &state->descSize );
				}
                if (state->frame_encoded == 1 && stream_size < MAX_SPSPPS_SIZE) {
                    dlog( "only SPS&PPS got, do not send out any bytes." );
                    *data     = NULL;
                    *dataSize = 0;
#ifdef DUMP_STREAM
                    fwrite(stream, 1, stream_size, fp);
                    fflush(fp);
#endif
                    err = FskMemPtrDisposeAt(&stream);
                    BAIL_IF_ERR(err);
                } else {
                    dlog( "send out stream encoded here." );
                    /* Work frame_type out here ... */
                    UInt32 outputFrameType = 0;
                    err = analysis_stream_basic(stream, stream_size, &naluType, &naluOffset);
                    dlog(" naluType = %d, naluOffset = %d", naluType, naluOffset);
                    BAIL_IF_ERR(err);

                    if (state->frame_encoded == 2 && naluOffset != 0) {//HACK: give a chance to correct nalu size of SPS&PPS
                        dlog( "some stupid devices declare stream size in a wrong way..." );
#ifdef DUMP_STREAM
                        fwrite(stream, 1, naluOffset, fp);
                        fflush(fp);
#endif
                        //stream_size -= naluOffset;
                        //FskMemMove(stream, stream+naluOffset, stream_size);
                        naluSize[0] = naluOffset - 4; //naluSize[1];
                        naluSize[1] = stream_size - naluOffset - 4;
                    }

                    if( naluType == k_nalu_type_slice_layer_no_partition_IDR)
                    {//IDR
                        dlog( "hit SPS or PPS or I slice, we guess this is I frame." );
                        outputFrameType = 0;
                    }else {//P
                        dlog( "otherwise, this should be a P frame, we don't support B frame here." );
                        outputFrameType = kFskImageFrameTypeDifference;
                    }
                    *frameTypeOut = outputFrameType;
                    *data     = stream;
                    *dataSize = stream_size;
                }
            }
			break;
		}
		else {
			dlog( "No output frame ... need more stream\n");
			break;
		}

	} while(1);

bail:
    if (buffer) {
        err = FskMemPtrDisposeAt(&buffer);
        BAIL_IF_ERR(err);
    }

	if (state->mC_MediaFormat == NULL) {
		err = kFskErrBadState;
	}

    if( *data != NULL && *dataSize != 0 && err == 0)
    {
        unsigned char *avc_bytes = *data;

        dlog( "startcode ==> nalu, naluSize[0] = %d, naluSize[1] = %d", naluSize[0], naluSize[1] );
        StartcodeToStartlen( avc_bytes, naluSize );
#ifdef DUMP_STREAM
        fwrite(*data, 1, *dataSize, fp);
        fflush(fp);
#endif
    }

    gettimeofday(&tv_end, NULL);
    dec_time = 0;
    if (state->frame_encoded > 2)
        dec_time = (int)(1000000 * (tv_end.tv_sec - tv_begin.tv_sec) + tv_end.tv_usec - tv_begin.tv_usec);
	state->total_time += dec_time;
    if (state->frame_encoded)
        avg_time = (state->total_time / state->frame_encoded) / 1000;
    dlog( "## out AndroidJavaVideoEncodeCompressFrame ##, err = %d, dataSize = %d, frame_encoded = %d, total_time = %dms, dec_time = %dms, avg_time = %dms\n", err, *dataSize, state->frame_encoded, (int)(state->total_time/1000), (int)(dec_time/1000), (int)avg_time );

	return err;
}

FskErr AndroidJavaVideoEncodeGetFormat(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;
    dlog( "## into AndroidJavaVideoEncodeGetFormat ##\n" );

    property->value.str = FskStrDoCopy("video/avc"); //only avc support now!
	property->type = kFskMediaPropertyTypeString;
	return err;
}

FskErr AndroidJavaVideoEncodeGetSampleDescription(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;
    dlog( "## into AndroidJavaVideoEncodeGetSampleDescription ##\n" );

    property->value.data.data		= state->desc;
	property->value.data.dataSize	= state->desc ? state->descSize : 0;
	property->type					= kFskMediaPropertyTypeData;
    dlog( "descSize = %d, data is %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x", state->descSize,
         state->desc[0], state->desc[1], state->desc[2], state->desc[3], state->desc[4], state->desc[5], state->desc[6],
         state->desc[7], state->desc[8], state->desc[9], state->desc[10], state->desc[11], state->desc[12], state->desc[13],
         state->desc[14], state->desc[15]);

    //***bnie: need to release the pointer or just make a copy
    state->desc = NULL;
    state->descSize = 0;
    //***bnie
    
	return err;
}

FskErr AndroidJavaVideoEncodeSetBitrate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;
    dlog( "## into AndroidJavaVideoEncodeSetBitrate ##\n" );

    state->bps = property->value.integer;
	return err;
}

FskErr AndroidJavaVideoEncodeGetBitrate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;
    dlog( "## into AndroidJavaVideoEncodeGetBitrate ##\n" );

    property->value.integer = state->bps;
    return err;
}

FskErr AndroidJavaVideoEncodeSetScale(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;
    dlog( "## into AndroidJavaVideoEncodeSetScale ##\n" );

    state->timeScale = property->value.integer;
	return err;
}

FskErr AndroidJavaVideoEncodeGetScale(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;
    dlog( "## into AndroidJavaVideoEncodeGetScale ##\n" );

    property->value.integer = state->timeScale;
	return err;
}

FskErr AndroidJavaVideoEncodeSetKeyFrameRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;
    dlog( "## into AndroidJavaVideoEncodeSetKeyFrameRate ##\n" );

    state->key_fps = property->value.integer;
	return err;
}

FskErr AndroidJavaVideoEncodeGetKeyFrameRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;
    dlog( "## into AndroidJavaVideoEncodeGetKeyFrameRate ##\n" );

    property->value.integer = state->key_fps;
	return err;
}

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

FskErr AndroidJavaVideoEncodeSetCompressionSettings(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;
    char *s = property->value.str;
	long i = 0;
    dlog( "## into AndroidJavaVideoEncodeSetCompressionSettings ##\n" );
    dlog( "string of compresstion setting is : %s", s );

    while(1)
	{
		char w[64];
		char k[32], v[32];

		GetNextWord( s, &i, w );
		if( w[0] == 0 )
			break;

		GetKeyAndValue( w, k, v );

		if (strcmp( k, "fps" ) == 0) {
			state->fps = atoi( v );
            dlog( "accept fps from compression settings, fps = %d", state->fps );
		} else if (strcmp( k, "bps" ) == 0) {
			state->bps = atoi( v );
            dlog( "accept bps from compression settings, bps = %d", state->bps );
		} else if (strcmp( k, "key_fps" ) == 0) {
			state->key_fps = atoi( v );
            dlog( "accept key_fps from compression settings, key_fps = %d", state->key_fps );
		}
    }

	return err;
}

FskErr AndroidJavaVideoEncodeGetCompressionSettings(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    FskAndroidJavaVideoEncode *state = (FskAndroidJavaVideoEncode *)stateIn;
	FskErr err = kFskErrNone;
    dlog( "## into AndroidJavaVideoEncodeGetCompressionSettings ##\n" );

	return err;
}
