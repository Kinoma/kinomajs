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
//#define __FSKMPEGDECODE_PRIV__

#include "kinoma_ipp_lib.h"
#include "kinoma_performance.h"

#include "kinomaavc.h"

#include "Fsk.h"
#include "FskBitmap.h"
#include "FskUtilities.h"
#include "FskImage.h"
#include "FskEndian.h"

#include "QTReader.h"

#include "umc_h264_dec.h"
#include "umc_media_data_ex.h"

#include "avcC.h"
#include "FskYUV420Copy.h"

#include "kinoma_utilities.h"


#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(kinomaavc, kinomaavc);
#define mlog  FskkinomaavcPrintfMinimal
#define nlog  FskkinomaavcPrintfNormal
#define vlog  FskkinomaavcPrintfVerbose
#define dlog  FskkinomaavcPrintfDebug


#define SUPPORT_ROTATION

//***for debug only

//#define	DUMP_INPUT
//#define DUMP_AVI
//#define DUMP_RAW
//#define DUMP_YUV
//#define DUMP_BOX

#if SRC_YUV420i
	#define YUV420_FORMAT	kFskBitmapFormatYUV420i
#else
	#define YUV420_FORMAT	kFskBitmapFormatYUV420
#endif

using namespace UMC;

#ifdef DUMP_YUV
#define Debug_DumpYUV( a, b, c, d, e, f, g )	dump_yuv( a, b, c, d, e, f, g )
#else
#define Debug_DumpYUV( a, b, c, d, e, f, g )
#endif


#if	defined(DUMP_INPUT)
					#define Debug_Dump_Input( a, b )			dump_bitstream_box( a, b )
					#define Debug_Dump_Input_Header(a,b)	
					#define Debug_Dump_Input_Close()			close_input_dump_file()
#elif defined(DUMP_AVI)
					#define Debug_Dump_Input( a, b )			dump_input( a, b )
					#define Debug_Dump_Input_Header(a,b)		dump_bitstream_raw( a, b )
					#define Debug_Dump_Input_Close()			close_input_dump_file()
#elif  defined(DUMP_RAW)
					#define Debug_Dump_Input( a, b )			dump_bitstream_raw( a, b )
					#define Debug_Dump_Input_Header(a,b)		dump_bitstream_raw( a, b )
					#define Debug_Dump_Input_Close()			close_input_dump_file()
#elif defined(DUMP_BOX)
					#define Debug_Dump_Input_Header(a,b)		dump_bitstream_box( a, b )
					#define Debug_Dump_Input( a, b )			dump_bitstream_box( a, b )
					#define Debug_Dump_Input_Close()			close_input_dump_file()
#else
					#define Debug_Dump_Input( a, b )
					#define Debug_Dump_Input_Header(a,b)	
					#define Debug_Dump_Input_Close()
#endif

#define kDefaultNALULengthSize	4

const int kBitmapCacheSize = 50;

static FskErr avcDecodeSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr avcDecodeSetQuality(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr avcDecodeSetPlayMode(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr avcDecodeSetRotation(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr avcDecodeGetPerformanceInfo (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr avcDecodeSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property );

FskMediaPropertyEntryRecord avcDecodeProperties[] = {
	{kFskMediaPropertySampleDescription,		kFskMediaPropertyTypeData,		NULL,		avcDecodeSetSampleDescription},
 	{kFskMediaPropertyFormatInfo,				kFskMediaPropertyTypeData,		NULL,		avcDecodeSetSampleDescription},
	{kFskMediaPropertyQuality,					kFskMediaPropertyTypeFloat,		NULL,		avcDecodeSetQuality},
	{kFskMediaPropertyPlayMode,					kFskMediaPropertyTypeInteger,	NULL,		avcDecodeSetPlayMode},
	{kFskMediaPropertyPerformanceInfo,			kFskMediaPropertyTypeFloat,		avcDecodeGetPerformanceInfo,		NULL},
	{kFskMediaPropertyRotation,					kFskMediaPropertyTypeFloat,		NULL,		avcDecodeSetRotation},
	{kFskMediaPropertyPixelFormat,				kFskMediaPropertyTypeUInt32List,NULL,		avcDecodeSetPreferredPixelFormat},
	{kFskMediaPropertyUndefined,				kFskMediaPropertyTypeUndefined,	NULL,		NULL}
};

typedef struct {
	UInt32				sampleDescriptionSize;
	unsigned char		*sampleDescription;
	UInt32				sampleDescriptionSeed;
	MediaData3_V51		*src;
	H264VideoDecoder	*dec;
	int					naluLengthSize;
	
	//***bnie
	unsigned char		*qt_data;
	int					qt_data_total;

	FskBitmap			bitmaps[kBitmapCacheSize];

	float				quality;
	int					approx_level;
	int					play_mode;
	
	FskBitmapFormatEnum	dst_pixel_format;
	
	float				rotation_float;
	int 				rotation;

	int					unsupported;

	PerformanceInfo		perf;

#ifdef DO_TIMING
	MyTimer				*my_timer;
	int					timer_count;
#endif

} kinomaAVCDecodeRecord, *kinomaAVCDecode;


FskErr avcDecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
    *canHandle = ('avc1' == format) || (0 == FskStrCompare(mime, "x-video-codec/avc"));

    dlog( "into avcDecodeCanHandle, mime: %s, *canHandle: %d", mime, *canHandle );
	return kFskErrNone;
}

FskErr avcDecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension)
{
	FskErr err;
	kinomaAVCDecode state;

	err = FskMemPtrNewClear(sizeof(kinomaAVCDecodeRecord), (FskMemPtr *)&state);
	if (err) goto bail; 
    
    dlog( "into avcDecodeNew, format: %d, *mime: %s, state: %lx", format, mime, (long)state );

	state->naluLengthSize	= kDefaultNALULengthSize;
	state->quality			= 1.0;	//best quality by default
	state->approx_level		= 0;	
	state->play_mode		= 0;
	state->rotation_float	= 0;
	state->rotation			= kRotationNone;
	state->unsupported		= 0;

	//***bnie
	state->qt_data = NULL;
	state->qt_data_total = 0;

	state->dst_pixel_format = YUV420_FORMAT;
	
bail:
	if (kFskErrNone != err)
		avcDecodeDispose(state, deco);

	deco->state = state;

	return err;
}

FskErr avcDecodeDispose(void *stateIn, FskImageDecompress deco)
{
	kinomaAVCDecode state = (kinomaAVCDecode)stateIn;
	int i;

    dlog( "into avcDecodeDispose, state: %lx", (long)state );
	Debug_Dump_Input_Close();

	if (NULL != state) 
	{
		if( state->sampleDescription != NULL ) 
			FskMemPtrDispose(state->sampleDescription);

		if( state->src != NULL )
			delete state->src;

#ifdef DO_TIMING
		MyTimerDispose(state->my_timer);

		{
			float dur = MyTimerDur(state->my_timer);

			fprintf( stderr, "\n\n avc: dur: %f, total: %d, frame rate: %f fps\n\n", dur, state->timer_count, 1/(dur/state->timer_count) );	
		}
#endif

		if( state->dec != NULL )
			delete state->dec;

		for (i = 0; i < kBitmapCacheSize; i++) 
			FskBitmapDispose(state->bitmaps[i]);
		

	//***bnie
		if( state->qt_data != NULL )
			free( state->qt_data = NULL );

		FskMemPtrDispose(state);
	}

	return kFskErrNone;
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

	avcC->configurationVersion	= data[idx]; idx++;
	avcC->AVCProfileIndication  = data[idx]; idx++;
	avcC->profile_compatibility = data[idx]; idx++;
	avcC->ACVLevelIndication	= data[idx]; idx++;
	
	reserved6BitsAndLengthSizeMinusOne= data[idx]; idx++;
	avcC->naluLengthSize = (reserved6BitsAndLengthSizeMinusOne&0x03) + 1;
	if( avcC->naluLengthSize != 4 && avcC->naluLengthSize != 2 && avcC->naluLengthSize != 1 )
		return kFskErrBadData;

	reserved3BitsAndNumberofSequenceParameterSets = data[idx]; idx++;
	avcC->numberofSequenceParameterSets = reserved3BitsAndNumberofSequenceParameterSets&0x1F;
	if(avcC->numberofSequenceParameterSets!=1)
		return kFskErrUnimplemented;
		
	avcC->spsSize = (data[idx]<<8)|data[idx+1]; idx += 2;
	if(avcC->spsSize + avcC->naluLengthSize > 256)
		return kFskErrBadData;
	
	avcC->sps		 = avcC->spspps;
	SetSizeByLen( avcC->sps, avcC->naluLengthSize, avcC->spsSize );
	avcC->sps		 += avcC->naluLengthSize;
	avcC->spsppsSize = avcC->naluLengthSize + avcC->spsSize;

	memcpy((void *)avcC->sps, (void *)&data[idx], avcC->spsSize);
	idx += avcC->spsSize;

	avcC->numberofPictureParameterSets = data[idx]; idx++;;
	if(avcC->numberofPictureParameterSets!=1)
		return kFskErrUnimplemented;

	avcC->ppsSize = (data[idx]<<8)|data[idx+1]; idx += 2;
	if(avcC->spsSize + avcC->naluLengthSize + avcC->ppsSize + avcC->naluLengthSize > 256)
		return kFskErrBadData;
	
	avcC->pps		 = avcC->spspps + avcC->naluLengthSize + avcC->spsSize;
	SetSizeByLen( avcC->pps, avcC->naluLengthSize, avcC->ppsSize );
	avcC->pps		 += avcC->naluLengthSize;
	avcC->spsppsSize += avcC->naluLengthSize + avcC->ppsSize;

	memcpy((void *)avcC->pps, (void *)&data[idx], avcC->ppsSize);

    return kFskErrNone;
}



#define SC_LEN		4	//startcode length
#define NALU_LEN		4

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

	return (int)(d - first);
}


FskErr FakeAVCC( unsigned char *data, int size, AVCC *avcC )
{
	unsigned char	*next_data	= data;
	int				block_size	= 0;
	int				rest_size	= size;

	memset( avcC, 0, sizeof(AVCC) );
	

	//skip every thing before the first starcode
	block_size = until_next_start_code( data, rest_size );
	if( block_size >= rest_size )//every thing skipped???
		return kFskErrBadData;

	next_data += block_size + SC_LEN;
	rest_size -= block_size + SC_LEN;
	
	avcC->naluLengthSize = NALU_LEN;
	avcC->sps = avcC->spspps;

	while( 1 )
	{
		unsigned char nalu_type;

		nalu_type = next_data[0] & NAL_UNITTYPE_BITS;
		block_size = until_next_start_code( next_data, rest_size );

		if( UMC::NAL_UT_SPS == nalu_type && avcC->numberofSequenceParameterSets == 0 )
		{
			avcC->spsSize	  = block_size;
			SetSizeByLen( avcC->sps, NALU_LEN, block_size );
			avcC->sps		 += NALU_LEN;
			avcC->spsppsSize  = NALU_LEN + block_size;
			memcpy((void*)avcC->sps, (void*)next_data, block_size);
			avcC->pps		  = avcC->spspps + avcC->spsppsSize;
			avcC->numberofSequenceParameterSets = 1;
		}
		else if( UMC::NAL_UT_PPS == nalu_type && avcC->numberofPictureParameterSets == 0 && avcC->numberofSequenceParameterSets == 1)
		{
			avcC->ppsSize	  = block_size;
			SetSizeByLen( avcC->pps, NALU_LEN, block_size );
			avcC->pps		 += NALU_LEN;
			avcC->spsppsSize += NALU_LEN + block_size;
			memcpy((void *)avcC->pps, (void *)next_data, block_size);
			avcC->numberofPictureParameterSets = 1;
		}

		next_data += block_size + SC_LEN;
		rest_size -= block_size + SC_LEN;

		if( rest_size <= 0 )
			break;
	}

	if( avcC->numberofSequenceParameterSets == 0 || avcC->numberofPictureParameterSets == 0 )
		return kFskErrBadData;
	else
		return kFskErrNone;
}

int startcode2len( unsigned char *data, int size, unsigned char *qt_data, int *out_size )
{
	unsigned char	*next_data	= data;
	int				block_size	= 0;
	int				rest_size	= size;

	 *out_size = 0;
	block_size = until_next_start_code( data, rest_size );
	if( block_size >= rest_size )
		return -1;

	next_data += block_size + SC_LEN;
	rest_size -= block_size + SC_LEN;
	
	while( 1 )
	{
		unsigned char nalu_type;

		nalu_type  = next_data[0] & NAL_UNITTYPE_BITS;
		block_size = until_next_start_code( next_data, rest_size );

		if( (UMC::NAL_UT_SLICE == nalu_type )  || (UMC::NAL_UT_IDR_SLICE == nalu_type ) )
		{
			SetSizeByLen( qt_data, NALU_LEN, block_size );
			qt_data	  += NALU_LEN;
			memcpy((void *)qt_data, (void *)next_data, block_size);
			qt_data   += block_size;
			*out_size += NALU_LEN + block_size;
		}
			
		next_data += block_size + SC_LEN;
		rest_size -= block_size + SC_LEN;

		if( rest_size <= 0 )
			break;
	}

	return 0;
}


int check_nalu( int is_startcode, unsigned char *data, int size, long *nalu_type_out, long *ref_idc_out )
{
	unsigned char	*next_data	= data;
	int				block_size	= 0;
	int				rest_size	= size;
	int				current_box_size = 0;

	*nalu_type_out = 0;
	*ref_idc_out   = 0;

	if( is_startcode )
		block_size = until_next_start_code( data, rest_size );
	else
	{
		block_size = 0;
		current_box_size = (data[0]<<24)|(data[1]<<16)|(data[2]<<8)|(data[3]<<0); 
	}

	if( block_size >= rest_size )
		return -1;

	next_data += block_size + SC_LEN;
	rest_size -= block_size + SC_LEN;
	
	while( 1 )
	{
		unsigned char nalu_type;
		unsigned char ref_idc;
#define NAL_REF_IDC_BITS 0x60
		nalu_type  = next_data[0] & NAL_UNITTYPE_BITS;
		ref_idc    = next_data[0] & NAL_REF_IDC_BITS;

		if( is_startcode )
			block_size = until_next_start_code( next_data, rest_size );
		else
		{
			unsigned char *n_d = next_data + current_box_size;
			block_size = current_box_size;
			current_box_size = (n_d[0]<<24)|(n_d[1]<<16)|(n_d[2]<<8)|(n_d[3]<<0); 
		}

		if( (UMC::NAL_UT_SLICE == nalu_type )  || (UMC::NAL_UT_IDR_SLICE == nalu_type ) )
		{
			*nalu_type_out = nalu_type;
			*ref_idc_out   = ref_idc;
			return 0;
		}
			
		next_data += block_size + SC_LEN;
		rest_size -= block_size + SC_LEN;

		if( rest_size <= 0 )
			break;
	}

	return -1;
}

static int q_a_ary[11] = 
{	
	AVC_LOSSY_SETTING_10, 
	AVC_LOSSY_SETTING_9, 
	AVC_LOSSY_SETTING_8, 
	AVC_LOSSY_SETTING_7, 
	AVC_LOSSY_SETTING_6,
	AVC_LOSSY_SETTING_5, 
	AVC_LOSSY_SETTING_4, 
	AVC_LOSSY_SETTING_3, 
	AVC_LOSSY_SETTING_2, 
	AVC_LOSSY_SETTING_1,  
	AVC_LOSSY_SETTING_0
};

static int quility_to_approx( float quality )
{
	int	q = (int)(quality*10);
	if( q > 10 ) q = 10;
	if( q < 0  ) q = 0;
	return q_a_ary[q];
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


FskErr avcDecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	kinomaAVCDecode		state	= (kinomaAVCDecode)stateIn;
	QTImageDescription	desc	= (QTImageDescription)state->sampleDescription;
	FskErr				err		= kFskErrNone;
	YUVPlannar			yuvPlannar;
	int					dropFrame = (0 != (frameType & kFskImageFrameDrop));

    dlog( "into avcDecodeDecompressFrame, data: %x, dataSize: %d", (long)data, dataSize );
	if( state->unsupported )
		return kFskErrUnimplemented;

#ifdef DO_TIMING

	if( state->my_timer == NULL )
	{
		state->my_timer = MyTimerNew();
		state->timer_count = 0;
	}

	MyTimerStart(state->my_timer);
	state->timer_count++;
#endif

	if( state->dec == NULL && data != NULL ) //this is initilized to 0
	{	
		kinoma_ipp_lib_avc_init(FSK_ARCH_AUTO);

		unsigned char		*avcc_data = NULL;
		AVCC				avcC;
		VideoDecoderParams_V51	decParam;

		if( desc != NULL )
		{
			avcc_data = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)desc, 'avcC');
			if( avcc_data != NULL )
			{
				err = DecAVCC( avcc_data, &avcC );
				if( err != 0 )
					return err;
			}
			else
			{
				avcc_data = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)desc, 'spsp');
				if( avcc_data == NULL )
				{
					err = kFskErrBadData; 
					goto bail;
				}

				avcC.naluLengthSize = 4;
				avcC.spsppsSize = (unsigned short)FskMisaligned32_GetN(avcc_data) - 8;
				avcc_data += 8;
				memcpy( avcC.spspps, avcc_data, avcC.spsppsSize );


			}
		}
		else
		{
			err = FakeAVCC( (unsigned char *)data, dataSize, &avcC );			
			if( err != 0 )
				return err;
		}

		state->naluLengthSize = avcC.naluLengthSize;

		Debug_Dump_Input_Header( avcC.spspps, avcC.spsppsSize );

		state->dec  = new H264VideoDecoder();
		if( state->dec == NULL )
		{
			err = kFskErrMemFull;
			goto bail;
		}
		if( state->src == NULL )
			state->src = new MediaData3_V51(20);

		decParam.m_pData3 = state->src;

		SwapInQTMediaSample3( 1, state->naluLengthSize, avcC.spspps, avcC.spsppsSize, decParam.m_pData3 );
		decParam.lFlags = decParam.lFlags | FLAG_VDEC_4BYTE_ACCESS;

		decParam.info.framerate			= 1;
		decParam.info.duration			= 1;
		//***
		decParam.uiLimitThreads			= 1;

		{
            Status umcRes = state->dec->Init(&decParam);
			if( umcRes != UMC_OK ) 
			{
				if( state->dec != NULL )
					delete state->dec;

				state->dec = NULL;
				state->unsupported = 1;

				return kFskErrBadState;
			}
		}

		performance_reset( &state->perf );
	}


	if( state->dec == NULL ) //this is initilized to 0
		return kFskErrBadState;

	if( data != NULL )
	{
		unsigned char *s = (unsigned char *)data;
		int src_size;

		if( state->naluLengthSize == 4 )
			src_size = (s[0]<<24)|(s[1]<<16)|(s[2]<<8)|(s[3]);
		else if(  state->naluLengthSize == 2 )
			src_size = (s[0]<<8)|(s[1]<<0);
		else
			src_size = s[0];

		if( src_size > (int)dataSize )
			return kFskErrBadState;
	}

	performance_begin( &state->perf );

	if( state->dec != NULL && data != NULL )
	{
		Status	umcRes = UMC_OK;
			
		if( desc != NULL )
		{
			SwapInQTMediaSample3( 0, state->naluLengthSize, (unsigned char *)data, dataSize, state->src );
			Debug_Dump_Input( (unsigned char *)data, dataSize );
		}
		else
		{
			if( (UInt32)(state->qt_data_total) < dataSize )
			{
				if( state->qt_data != NULL )
					free( state->qt_data );

				state->qt_data = (unsigned char *)malloc( dataSize );
				state->qt_data_total = dataSize;
			}

			if( state->qt_data != NULL )
			{
				int this_size;
				int result = startcode2len(  (unsigned char *)data, dataSize, state->qt_data, &this_size );
				if( result != 0 || this_size == 0 ) {
                    err = kFskErrOperationFailed;
					goto bail;
                }

				SwapInQTMediaSample3( 0, state->naluLengthSize, (unsigned char *)state->qt_data, this_size, state->src );
				Debug_Dump_Input( (unsigned char *)state->qt_data, this_size );
			}
		}

		state->dec->SetApprox( state->approx_level );
		umcRes = state->dec->GetFrame(state->src);
	}
	else
	{
		Status	umcRes = UMC_OK;
		umcRes = state->dec->GetFrame(NULL);
	}

	// put image into bits...
	yuvPlannar.isValid = 0;
#define OUTPUT_IN_DISPLAY_ORDER  1
	state->dec->GetYUV((kFskImageFrameImmediate & frameType) ? 0 : OUTPUT_IN_DISPLAY_ORDER, &yuvPlannar);
	if( yuvPlannar.isValid && !dropFrame )
	{
		FskBitmapFormatEnum    pixelFormat;
		unsigned char	*dstPtr;
		int 			dstRowBytesY;
		int             src_width  = (int)(yuvPlannar.right - yuvPlannar.left);
		int             src_height = (int)(yuvPlannar.bottom - yuvPlannar.top);
		int             rot_width, rot_height;

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

		if( state->dst_pixel_format == kFskBitmapFormatYUV420i )
		{		
			rot_width  = ((rot_width +1)>>1)<<1;	//force even width and height
			rot_height = ((rot_height+1)>>1)<<1;
		}

		if (deco->bits) 
		{
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
						deco->bits = state->bitmaps[i];
						break;
					}
				}
				else
					index = i;
			}

			if (NULL == deco->bits) 
			{
				err = FskBitmapNew((SInt32)rot_width, (SInt32)rot_height, state->dst_pixel_format, &deco->bits);
				if (err) goto bail;
				deco->bits->rotation = (SInt32)state->rotation_float;
			}

			if (-1 != index) 
			{
				state->bitmaps[index]		= deco->bits;
				FskBitmapUse(deco->bits);
			}
		}

		FskBitmapWriteBegin(deco->bits, (void**)(&dstPtr), (SInt32 *)&dstRowBytesY, &pixelFormat );
        dlog("get Bitmap, dstRowBytesY = %d, pixelFormat = %d", (int)dstRowBytesY, (int)pixelFormat);
		if (state->dst_pixel_format != pixelFormat)
		{
			FskBitmapWriteEnd(deco->bits);
			err = kFskErrBadData;
			goto bail;
		}

		{
			if( state->dst_pixel_format == kFskBitmapFormatYUV420 )
			{
				unsigned char *u = dstPtr + (dstRowBytesY * (rot_height +(rot_height & 0)));
				unsigned char *v = u + (dstRowBytesY >> 1) * ((rot_height + 1) >> 1);
				FskYUV420Copy(	rot_width, rot_height,
								yuvPlannar.y, yuvPlannar.cb, yuvPlannar.cr,
								(int)yuvPlannar.rowBytesY, (int)yuvPlannar.rowBytesCbCr,
								dstPtr, u, v,
								dstRowBytesY, dstRowBytesY >> 1);
				
				Debug_DumpYUV( rot_width, rot_height, dstRowBytesY, dstRowBytesY >> 1, dstPtr, u, v ); 
			}
			else
			{
				//always copy even width and height
				FskYUV420Interleave_Generic
				(
					yuvPlannar.y, yuvPlannar.cb, yuvPlannar.cr, 
					dstPtr, ((src_height+1)>>1)<<1, ((src_width+1)>>1)<<1, 
					(int)yuvPlannar.rowBytesY, (int)yuvPlannar.rowBytesCbCr,
					dstRowBytesY,
					state->rotation
				);
			}

#ifdef BUF_PRINTF
			{
				int x0 = rot_width-160;
				int interleave = pixelFormat==kFskBitmapFormatYUV420i;
				int x = (int)(state->quality *10)*5;//5 is font width
				buf_printf(interleave, dstPtr, dstRowBytesY, rot_height, x0+x, 0, ">");
				buf_printf(interleave, dstPtr, dstRowBytesY, rot_height, x0+55, 0, "+");
				buf_printf(interleave, dstPtr, dstRowBytesY, rot_height, x0+60, 0, "%x", state->approx_level);
				
				buf_printf(interleave, dstPtr, dstRowBytesY, rot_height, x0+20, 20, "#%2d: play_fps =%6.2f",  state->perf.idx, state->perf.play_fps);
				buf_printf(interleave, dstPtr, dstRowBytesY, rot_height, x0+20, 40, "     movie_fps=%6.2f", state->perf.movie_fps);
				buf_printf(interleave, dstPtr, dstRowBytesY, rot_height, x0+20, 60, "     play_mode=%2d",	 state->play_mode);
			}
#endif
		}

		FskBitmapWriteEnd(deco->bits);
	}

bail:

#ifdef DO_TIMING
		MyTimerStop(state->my_timer);
#endif

	if( decodeTime != NULL )
 		performance_end( &state->perf, *decodeTime );
	//performance_process( &state->perf );

	return err;
}


FskErr parse_header( unsigned char *data, int size, int *left, int *right, int *top, int *bottom )
{
	MediaData3_V51 *md = new MediaData3_V51(10);
	H264SeqParamSet sps;
	int width16, height16;

    if( md == NULL )
        return kFskErrMemFull;
    
	kinoma_ipp_lib_avc_init(FSK_ARCH_AUTO);

	SwapInQTMediaSample3( 1, 4, data, size, (void*)md );
	get_sps_pps(md, &sps, NULL );

	width16  = 16 * ( sps.frame_width_in_mbs ); 
	height16 = 16 * ( sps.frame_height_in_mbs );
	
	if( left && right && top && bottom )
	{
		if( sps.frame_cropping_flag )
		{
			*left   = 2*sps.frame_cropping_rect_left_offset;
			*right  = width16 - 2*sps.frame_cropping_rect_right_offset; 
			*top   = 2*sps.frame_cropping_rect_top_offset;
			*bottom = height16 - 2*sps.frame_cropping_rect_bottom_offset;
		}
		else
		{
			*left   = 0;
			*right  = width16;
			*top   = 0;
			*bottom = height16;
		}
	}

	if( md != NULL )
		delete md;

	return kFskErrNone;
}


FskErr parse_header2( unsigned char *data, int size, int *weighted_pred_flag )
{
#define INVALID_SPS_PPS_ID  255
	MediaData3_V51 *md = new MediaData3_V51(10);
	H264SeqParamSet sps;
	H264PicParamSet pps;
	FskErr err = kFskErrNone;
	
    if( md == NULL )
        return kFskErrMemFull;
    
	kinoma_ipp_lib_avc_init(FSK_ARCH_AUTO);
	
	pps.pic_parameter_set_id = INVALID_SPS_PPS_ID;
	sps.seq_parameter_set_id = INVALID_SPS_PPS_ID;

	SwapInQTMediaSample3( 1, 4, data, size, (void*)md );
	if( UMC_OK != get_sps_pps(md, &sps, &pps ) ||
		sps.seq_parameter_set_id == INVALID_SPS_PPS_ID	||
		pps.pic_parameter_set_id == INVALID_SPS_PPS_ID ) {
        err = kFskErrOperationFailed;
		goto bail;
    }
	
	if( weighted_pred_flag )
		*weighted_pred_flag = pps.weighted_pred_flag;

bail:
	if( md != NULL )
		delete md;
    
	return err;
}


FskErr avcDecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	long nalu_type;
	long ref_idc;
	UInt32	frame_type = 0;
	int		is_startcode = 1;


	if( index == 9 )
	{
		int left, right, top, bottom;
		int width, height;
		
		parse_header( (unsigned char *)deco->data, deco->dataSize, &left, &right, &top, &bottom );

		width  = right - left;
		height = bottom - top;

		if( value != NULL )
		{
			value->type = kFskMediaPropertyTypeInteger;
			value->value.integer = (width<<16)|height;
		}

		return kFskErrNone;
	}

	if (kFskImageDecompressMetaDataFrameType != metadata)
		return kFskErrUnimplemented;

	if( index == 0 )
		is_startcode = 1;
	else if( index == 1 )
		is_startcode = 0;
	
	if( 0 != check_nalu( is_startcode, (unsigned char *)deco->data, deco->dataSize, &nalu_type, &ref_idc ) )
		return kFskErrBadData;

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

	return kFskErrNone;
}


FskErr avcDecodeSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaAVCDecode state = (kinomaAVCDecode)stateIn;

    dlog( "into avcDecodeSetSampleDescription" );
    
	state->sampleDescriptionSeed++;
	if( state->sampleDescription != NULL )
		FskMemPtrDisposeAt((void **)&state->sampleDescription);

	state->sampleDescriptionSize = property->value.data.dataSize;
	return FskMemPtrNewFromData(state->sampleDescriptionSize, property->value.data.data, (FskMemPtr *)&state->sampleDescription);
}


FskErr avcDecodeSetQuality(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaAVCDecode state = (kinomaAVCDecode)stateIn;

	state->quality = (float)property->value.number;
	if( state->quality > 1.0 )//allowing postproccessing5
		state->quality = 1.0;

	state->approx_level = quility_to_approx( state->quality );//;// = dropFrame ? 7 : 0;
			
	return kFskErrNone;
}


FskErr avcDecodeSetPlayMode(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaAVCDecode state = (kinomaAVCDecode)stateIn;

	state->play_mode = property->value.integer;
			
	return kFskErrNone;
}

FskErr avcDecodeSetRotation(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaAVCDecode state = (kinomaAVCDecode)stateIn;

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


FskErr avcDecodeSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property )
{
	kinomaAVCDecode state = (kinomaAVCDecode)stateIn;
	FskBitmapFormatEnum prefered_yuvFormat = (FskBitmapFormatEnum)property->value.integers.integer[0];//***get the first for backword compatability;

	if( prefered_yuvFormat == kFskBitmapFormatYUV420i || prefered_yuvFormat == kFskBitmapFormatYUV420 )
		state->dst_pixel_format = prefered_yuvFormat;
		
	return kFskErrNone;
}


FskErr avcDecodeGetPerformanceInfo (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaAVCDecode	state	= (kinomaAVCDecode)stateIn;
	FskErr			err		= kFskErrNone;

	if (NULL == state->dec)
		return kFskErrBadState;
	
	performance_process( &state->perf );

	property->value.number	= state->perf.play_fps;
	property->type			= kFskMediaPropertyTypeFloat;
	
//bail:	
	return err;
}

