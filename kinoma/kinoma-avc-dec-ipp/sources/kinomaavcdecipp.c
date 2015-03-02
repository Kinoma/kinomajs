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
//#define __FSKMPEGDECODE_PRIV__

#include "kinomaavcdecipp.h"
#include "avcC.h"
#include "FskYUV420Copy.h"
#include "kinoma_utilities.h"
#include "FskEndian.h"
#include "FskBitmap.h"
#include "QTReader.h"

#include "codecDef.h"
#include "codecVC.h"
#include "misc.h"



#if SUPPORT_INSTRUMENTATION
FskInstrumentedTypeRecord gkinomaavcdecippTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "kinomaavcdecipp"};
//FskInstrumentedSimpleType(kinomaavcdecipp, kinomaavcdecipp);
#endif


#define AVCErr( err )					\
{										\
	if( err == AVCDEC_SUCCESS )			\
		err = 0;						\
	else								\
		err -= 256;						\
}


#define KINOMA_IPP_AVC_PARSE

#define SUPPORT_ROTATION

#if SRC_YUV420i
	#define YUV420_FORMAT	kFskBitmapFormatYUV420i
#else
	#define YUV420_FORMAT	kFskBitmapFormatYUV420
#endif

//***bnie
#define		DATA_BUFFER_SIZE		(1024 * 1024) /*1M*/


#define kBitmapCacheSize		50
#define kYUVBufferMax			64

#define SC_LEN					4	//startcode length
#define DEFAULT_NALU_LEN		4
#define kDefaultNALULengthSize	4
#define NAL_REF_IDC_BITS		0x60
#define NAL_UNITTYPE_BITS       0x1f


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



FskMediaPropertyEntryRecord avcDecodeProperties[] = 
{
	{kFskMediaPropertySampleDescription,		kFskMediaPropertyTypeData,		NULL,		avcDecodeSetSampleDescription},
 	{kFskMediaPropertyFormatInfo,				kFskMediaPropertyTypeData,		NULL,		avcDecodeSetSampleDescription},
	{kFskMediaPropertyRotation,					kFskMediaPropertyTypeFloat,		NULL,		avcDecodeSetRotation},
	{kFskMediaPropertyPixelFormat,				kFskMediaPropertyTypeUInt32List,NULL,		avcDecodeSetPreferredPixelFormat},
	{kFskMediaPropertyMaxFramesToQueue,			kFskMediaPropertyTypeInteger,	avcDecodeGetMaxFramesToQueue, NULL},
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
	
	int					bad_state;
	
	FskInt64			current_decode_time;
	FskInt64			decode_time_ary[kYUVBufferMax];
	unsigned char		*yuv_buffer_ary[kYUVBufferMax];

	FskBitmap			bitmaps[kBitmapCacheSize];

	int					width_src;
	int					height_src;
	int					width_clip;
	int					height_clip;
	
	int					frameSize;
	
	FskBitmapFormatEnum				dst_pixel_format;
	float				rotation_float;
	int 				rotation;

	int					debug_input_frame_count;
	int					debug_output_frame_count;
	int					debug_dropped_frame_count;
	
	
	
    //NSCCheckDisable			1: Input data in the unit of NAL\n");
    //							0: Input data in arbitrary size\n");
	//output delay mode			0, decoder internal re-order!\n");			
	//							1, decoder allocate many buffer, external re-order, simulate display latency!\n");			
	//							2, decoder allocate few buffer, external re-order!\n");			
	//skip mode					0, Normal decoding!\n");			
	//							1, Fast deblocking for all frames!\n");			
	//							2, No deblocking for all frames!\n");			
	//							2.5, skip 1/2 non-reference frames!\n");			
	//							3, skip all non-reference frames!\n");			
	//custom frame malloc mode	0, using default frame mallocate function!\n");
	//							1, using customer defined frame mallocate function!\n");
	int							dec_param_NSCCheckDisable;
    int							dec_param_bOutputDelayDisable;
	int							dec_param_nFdbMode;
	IppSkipMode					dec_param_SkipMode;
	int							dec_param_bCustomFrameMalloc;
	int							dec_param_bRawDecDelayPar;
    
	MiscGeneralCallbackTable    dec_param_SrcCBTable;
	IppBitstream                srcBitStream;
	IppH264PicList              *pDstPicList;
	int							bUsed;
	int							output_frame_count;
	
	void                        *avc_dec;
	AVCC						avcC;
	int							sps_set;
	int							pps_set;
	
	
	
	
	
} kinomaAVCDecode;




IppCodecStatus videoInitBuffer (IppBitstream *pBufInfo)
{
    // Initialize IppBitstream
    // at least big enough to store 2 frame data for less reload 
    int err = IPP_MemMalloc((void**)(&pBufInfo->pBsBuffer), DATA_BUFFER_SIZE, 4);
	
    if (err != IPP_OK || NULL == pBufInfo->pBsBuffer) 
        return IPP_STATUS_NOMEM_ERR;
	
	IPP_Memset(pBufInfo->pBsBuffer, 0, DATA_BUFFER_SIZE);
	
    //no read data at beginning
    //set current pointer to the end of buffer
    pBufInfo->pBsCurByte = pBufInfo->pBsBuffer + DATA_BUFFER_SIZE;
    pBufInfo->bsCurBitOffset = 0;
    pBufInfo->bsByteLen = DATA_BUFFER_SIZE;
	
    return IPP_STATUS_NOERR;
}

IppCodecStatus videoFreeBuffer (IppBitstream *pBufInfo)
{
    if ( pBufInfo->pBsBuffer ) 
	{
        IPP_MemFree((void**)(&pBufInfo->pBsBuffer));
        pBufInfo->pBsBuffer = NULL;
    }
	
    return IPP_STATUS_NOERR;
}


//callbacks
void* h264_rawdecoder_frameMalloc(int size, int alignment, void* pUsrData)
{
	void* ptr;
	if(IPP_MemMalloc(&ptr, size, alignment) != IPP_OK) 
	{
		dlog("frameMalloc is called, ret addr %x, size %d, align %d, userdata %x\n", (int)NULL, (int)size, (int)alignment, (int)pUsrData);
		return NULL;
	}
	else
	{
		dlog("frameMalloc is called, ret addr %x, size %d, align %d, userdata %x\n", (int)ptr, (int)size, (int)alignment, (int)pUsrData);
		return ptr;
	}
}

void h264_rawdecoder_frameFree(void* pointer, void* pUsrData)
{
	dlog("frameFree is called, pointer %x, userdata %x\n", (int)pointer, (int)pUsrData);
	IPP_MemFree(&pointer);
	return;
}


int getReorderDelay(void* avc_dec)
{
	int poctype, dpbsize;
	IppCodecStatus ret;
	
	ret = DecodeSendCmd_H264Video(IPPVC_GET_POCTYPE, NULL, &poctype, avc_dec);
	if(ret != IPP_STATUS_NOERR) 
		poctype = -1;	
	
	dlog("===========Poc type is %d\n", (int)poctype);
	if(poctype == 2) 
		return 0;
	
	ret = DecodeSendCmd_H264Video(IPPVC_GET_DPBSIZE, NULL, &dpbsize, avc_dec);
	if(ret != IPP_STATUS_NOERR)
		dpbsize = 16;	
	
	dlog("===========dpbsize is %d\n", (int)dpbsize);
	return dpbsize;
}


static FskErr func_queue_in( FskListMutex func_item_list, FskImageDecompressComplete	completionFunction,  void	*completionRefcon, int frame_number, int drop, FskInt64 decode_time )
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


static FskErr func_queue_out( FskListMutex func_item_list, FskImageDecompressComplete	*completionFunction,  void	**completionRefcon, int *frame_number, int *drop, FskInt64 *decode_time )
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

	memcpy((void *)avcC->sps, (void *)&data[idx], avcC->spsSize);
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

	memcpy((void *)avcC->pps, (void *)&data[idx], avcC->ppsSize);

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

	memset( avcC, 0, sizeof(AVCC) );
	

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
			memcpy((void *)avcC->sps, (void *)next_data, block_size);
			avcC->pps		  = avcC->spspps + avcC->spsppsSize;
			avcC->numberofSequenceParameterSets = 1;
		}
		else if( NAL_UT_PPS == nalu_type && avcC->numberofPictureParameterSets == 0 && avcC->numberofSequenceParameterSets == 1)
		{
			avcC->ppsSize	  = block_size;
			SetSizeByLen( avcC->pps, DEFAULT_NALU_LEN, block_size );
			avcC->pps		 += DEFAULT_NALU_LEN;
			avcC->spsppsSize += DEFAULT_NALU_LEN + block_size;
			memcpy((void *)avcC->pps, (void *)next_data, block_size);
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


static FskErr send_out_one_pic( kinomaAVCDecode *state, IppPicture *pSrcPicture )
{
	FskImageDecompress		deco	  = state->deco;
	FskBitmapFormatEnum					pixelFormat;
	unsigned char			*dstPtr		= NULL;
	long					dst_y_rb;
	long					rot_width, rot_height;
	long					src_width	= pSrcPicture->picROI.width;	//***src_width, src_height really should be named as clip_width, clip_height
	long					src_height	= pSrcPicture->picROI.height;
	int						src_y_rb    = pSrcPicture->picPlaneStep[0];
	int						src_u_rb    = pSrcPicture->picPlaneStep[1];
	int						src_v_rb    = pSrcPicture->picPlaneStep[2];
	unsigned char			*src_y      = pSrcPicture->ppPicPlane[0];
	unsigned char			*src_u      = pSrcPicture->ppPicPlane[1];	
	unsigned char			*src_v      = pSrcPicture->ppPicPlane[2];	
	FskImageDecompressComplete	completionFunction = NULL;
	void					*completionRefcon  = NULL;
	
	int						frame_number = 0;
	int						drop_flag	 = 0;
	FskBitmap				bits		 = NULL;
	FskErr					err			 = kFskErrNone;

	
	//int32_t frame_index;
	//int32_t Release;
	while(1)
	{
		FskInt64	input_decode_time  = 0;		
		//FskInt64	output_decode_time = 0;		
		
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
		
		dlog("retrieved completion function: %x, refcon: %x, frame_number: %d, drop_flag: %d, input_decode_time: %d\n", 
				(int)completionFunction, (int)completionRefcon, (int)frame_number, drop_flag, (int)input_decode_time );				
		
		
		break;
		
		//output_decode_time = state->decode_time_ary[frame_index];
		//dlog("input_decode_time: %d, output_decode_time: %d\n", (int)input_decode_time, (int)output_decode_time );		
		
		//if( input_decode_time < output_decode_time )
		//{
		//	dlog("\nXXXXXX: output frame comes in late, we will need to give up this completionFunction/RefCon\n" );					
		//	if( completionFunction != NULL )
		//		(completionFunction)(deco, completionRefcon, kFskErrNone, NULL);	
		//	state->debug_dropped_frame_count++;
		//}
		//else
		//	break;
	}
	
	
	dlog( "into send_out_frame\n" );

	src_y += src_y_rb *  pSrcPicture->picROI.y		 +  pSrcPicture->picROI.x;
	src_u += src_u_rb * (pSrcPicture->picROI.y >> 1) + (pSrcPicture->picROI.x >> 1);
	src_v += src_v_rb * (pSrcPicture->picROI.y >> 1) + (pSrcPicture->picROI.x >> 1);
	
	
	//dlog( "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ frame_index: %d, release? %d\n", frame_index, Release ); 
	dlog( "width: %d, height: %d\n",  (int)src_width, (int)src_height );
	
	
	if( drop_flag )	//player asked to drop, or flusing
	{
		dlog("this frame is dropped per player's request\n" );		
		if( completionFunction != NULL )
			(completionFunction)(deco, completionRefcon, kFskErrNone, NULL);	
		goto bail;
	}	
	
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
					//FskBitmapUse(state->bitmaps[i]);
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
			BAIL_IF_ERR( err );
			bits->rotation = (SInt32)state->rotation_float;
		}
		
		if( index != -1 ) 
		{
			state->bitmaps[index] = bits;
			FskBitmapUse( bits );
		}
	}
	
	FskBitmapWriteBegin( bits, (void**)(&dstPtr), (SInt32 *)&dst_y_rb, &pixelFormat );
	if (state->dst_pixel_format != pixelFormat)
	{
		FskBitmapWriteEnd( bits );
		 BAIL( kFskErrBadData );
	}
	
	{
		if( state->dst_pixel_format == kFskBitmapFormatYUV420 )
		{
			unsigned char *dst_y = dstPtr;
			unsigned char *dst_u = dst_y + (dst_y_rb * (rot_height +(rot_height & 0)));
			unsigned char *dst_v = dst_u + (dst_y_rb >> 1) * ((rot_height + 1) >> 1);
			
			FskYUV420Copy(	rot_width, rot_height,
						  src_y, src_u, src_v, src_y_rb, src_u_rb,
						  dst_y, dst_u, dst_v,
						  dst_y_rb, dst_y_rb >> 1);
		}
		else
		{
			//always copy even width and height
			dlog( "calling FskYUV420Interleave_Generic\n");
			FskYUV420Interleave_Generic
			(
			 src_y, src_u, src_v, 
			 dstPtr, ((src_height+1)>>1)<<1, ((src_width+1)>>1)<<1, 
			 src_y_rb, src_u_rb,
			 dst_y_rb,
			 state->rotation
			 );
		}
	}
	
	dlog("returning a bits: %x\n", (int)bits);		
	FskBitmapWriteEnd( bits );
	if( completionFunction != NULL )	
		(completionFunction)(deco, completionRefcon, kFskErrNone, bits);
	
bail:
	return err;
}



FskErr avcDecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into avcDecodeCanHandle: format: %d, mime: %s\n", (int)format, mime ); 
    
	*canHandle = ('avc1' == format) || (0 == FskStrCompare(mime, "x-video-codec/avc"));
	
	return kFskErrNone;
}

FskErr avcDecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension)
{
	kinomaAVCDecode *state;
	FskErr err;
	
	err = FskMemPtrNewClear(sizeof(kinomaAVCDecode), (FskMemPtr *)&state);
	BAIL_IF_ERR( err );
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "in avcDecodeNew allocated state: %x\n", (int)state ); 
	
	err = FskListMutexNew(&state->func_item_list, "FuncItemList");
	BAIL_IF_ERR( err ); 
	
	state->bad_state		= 0;
	state->nalu_len_size	= kDefaultNALULengthSize;
	state->rotation_float	= 0;
	state->rotation			= kRotationNone;

	state->width_src	= 0;
	state->height_src	= 0;
	state->width_clip	= 0;
	state->height_clip	= 0;
	
	state->frameSize	= 0;
	
	state->deco				= deco;

	state->dst_pixel_format = YUV420_FORMAT;
	
bail:
	if (kFskErrNone != err)
		avcDecodeDispose(state, deco);
	
	deco->state = state;
	
	dlog( "out of avcDecodeNew: err: %d\n", (int)err );
	
	return err;
}

FskErr avcDecodeDispose(void *stateIn, FskImageDecompress deco)
{
	kinomaAVCDecode *state = (kinomaAVCDecode *)stateIn;
	int i;
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into avcDecodeDispose\n" ); 
	
	if (NULL != state) 
	{
		if( state->sampleDescription != NULL ) 
			FskMemPtrDispose(state->sampleDescription);

		for (i = 0; i < kBitmapCacheSize; i++) 
			FskBitmapDispose(state->bitmaps[i]);
		
		
		if( state->func_item_list != NULL )
		{	
			dlog( "calling func_queue_flush\n" ); 
			func_queue_flush( deco, state->func_item_list, kFskErrShutdown );
			FskListMutexDispose(state->func_item_list);		
		}
		
		if( state->avc_dec != NULL )
			DecoderFree_H264Video(&state->avc_dec);

		videoFreeBuffer(&state->srcBitStream);
		
		FskMemPtrDispose(state);
	}
	
	
	return kFskErrNone;
}




void output_all_pictures( kinomaAVCDecode *state,  IppH264PicList* pDstPicList )
{
	//process the last several pictures of the video stream
	while (pDstPicList) 
	{
		dlog("Outputting picture, POC=%d\n", pDstPicList->pPic->picOrderCnt);
		send_out_one_pic(state, pDstPicList->pPic);
		pDstPicList->pPic->picStatus |= 1;
		pDstPicList = pDstPicList->pNextPic;
	}
}


void output_several_pictures(kinomaAVCDecode	*state, int avail_total, IppH264PicList* pDstPicList )
{
	int i;
	
	for( i = 0; i < avail_total; i++ )
	{
		dlog("Outputting picture ... ... POC=%d\n", pDstPicList->pPic->picOrderCnt);
		send_out_one_pic(state, pDstPicList->pPic );
		pDstPicList->pPic->picStatus |= 1;
		pDstPicList = pDstPicList->pNextPic;
		if( pDstPicList == NULL )
			break;
	}
}



FskErr avcDecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data_in, UInt32 dataSize_in, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	kinomaAVCDecode		*state			= (kinomaAVCDecode *)stateIn;
	QTImageDescription	desc			= (QTImageDescription)state->sampleDescription;
	unsigned char		*data			= (unsigned char *)data_in;
	int					data_size		= (int)dataSize_in;
	int					is_startcode	= desc == NULL;
	int					is_eos			= data == NULL;
	FskInt64			decode_time		= (decodeTime==NULL||is_eos) ? 0 : *decodeTime;
	//FskInt64		composition_time	= compositionTime==NULL ? 0 : *compositionTime;
	int					drop_frame		= (0 != (frameType & kFskImageFrameDrop));
	int					immediate_frame	= (0 != (frameType & kFskImageFrameImmediate));
	int					sync_frame		= ( (frameType&0xff) == kFskImageFrameTypeSync )||(immediate_frame);
	FskErr				err				= 0;
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into avcDecodeDecompressFrame, dataSize: %d, is_startcode: %d, drop_frame: %d, immediate_frame: %d, sync_frame: %d, is_eos: %d\n", 
			(int)data_size, (int)is_startcode, (int)drop_frame, (int)immediate_frame, (int)sync_frame, (int)is_eos ); 
	
	if( decodeTime != NULL )
	{
		dlog( "decodeTime: %d\n", (int)(*decodeTime) );
	}
	if( compositionTimeOffset != NULL )
	{
		dlog( "compositionTimeOffset: %d\n", (int)(*compositionTimeOffset) );
	}
	if( compositionTime != NULL )
	{
		dlog( "compositionTime: %d\n", (int)(*compositionTime) );
	}
	
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
	
	if( is_eos )
		goto bail; //***bnie
	
	state->debug_input_frame_count++;
	
	if( state->avc_dec == NULL ) 
	{	
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
					err = kFskErrBadData; 
					BAIL_IF_ERR( err );
				}

				dlog( "avcc_data: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n", 
						avcc_data[0],avcc_data[1],avcc_data[2],avcc_data[3],avcc_data[4],avcc_data[5],avcc_data[6],avcc_data[7],
						avcc_data[8],avcc_data[9],avcc_data[10],avcc_data[11],avcc_data[12],avcc_data[13],avcc_data[14],avcc_data[15]  ); 
				state->avcC.naluLengthSize = 4;
				state->avcC.spsppsSize = (unsigned short)FskMisaligned32_GetN(avcc_data) - 8;
				dlog( "state->avcC.spsppsSize: %d\n", state->avcC.spsppsSize ); 
				avcc_data += 8;
				memcpy( (void *)state->avcC.spspps, (void *)avcc_data, state->avcC.spsppsSize );
				
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
		#if 0	
			{ unsigned char *d = &state->avcC.spspps[0]; dlog( "spsppsSize:%d => %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",    state->avcC.spsppsSize, d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7], d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]  ); }
			{ unsigned char *d = &state->avcC.sps[0];	 dlog( "spsSize:%d    => %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",    state->avcC.spsSize,    d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7], d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15] );}
			{ unsigned char *d = &state->avcC.pps[0];    dlog( "ppsSize:%d    => %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",    state->avcC.ppsSize,    d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7], d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]  ); }	
		#endif
			state->width_clip	= 0;
			state->height_clip	= 0;
		}
		else //is_startcode
		{
			dlog( "no desc, fake avcC from input frame data\n" ); 
			err = FakeAVCC( data, data_size, &state->avcC );			
			BAIL_IF_ERR( err );
		}

		state->nalu_len_size = state->avcC.naluLengthSize;
		state->nale_pre		 = is_startcode ? 4 : state->nalu_len_size;
		
		state->dec_param_NSCCheckDisable	    = 1;
		state->dec_param_bOutputDelayDisable    = 0;					//default value
		state->dec_param_nFdbMode			    = 0;					//***bnie: not used???
		state->dec_param_SkipMode			    = IPPVC_SKIPMODE_0;
		state->dec_param_bCustomFrameMalloc     = 1;					//default value
		state->dec_param_bRawDecDelayPar	    = 0;
		
		state->dec_param_SrcCBTable.fMemCalloc	= IPP_MemCalloc;
		state->dec_param_SrcCBTable.fMemMalloc	= IPP_MemMalloc;
		state->dec_param_SrcCBTable.fMemFree	= IPP_MemFree;
		state->pDstPicList						= NULL;
		state->output_frame_count				= 0;
		state->bUsed							= 1;
		
		err = videoInitBuffer(&state->srcBitStream);
		if (IPP_STATUS_NOERR != err ) 
		{
			dlog( "error: no memory!\n");
			BAIL( IPP_FAIL );
		}
		
		//decoder initialize
		err = DecoderInitAlloc_H264Video(&state->dec_param_SrcCBTable, &state->avc_dec);
		if (IPP_STATUS_NOERR != err) 
		{
			dlog( "error: decoder init fail, error code %d!\n", (int)err);
			BAIL( IPP_FAIL );
		}
		
		err = DecodeSendCmd_H264Video(IPPVC_SET_NSCCHECKDISABLE, (&state->dec_param_NSCCheckDisable), NULL, state->avc_dec);
		if (IPP_STATUS_NOERR != err) 
			dlog( "error: DecodeSendCmd_H264Video(IPPVC_SET_NSCCHECKDISABLE) %d!\n", (int)err);
		
		err = DecodeSendCmd_H264Video(IPPVC_SET_OUTPUTDELAYDISABLE, (&state->dec_param_bRawDecDelayPar), NULL, state->avc_dec);
		if (IPP_STATUS_NOERR != err)
			dlog( "error: DecodeSendCmd_H264Video(IPPVC_SET_OUTPUTDELAYDISABLE) %d!\n", (int)err);
		
		dlog("sample code buffer management method: %d, raw decoder delay parameter: %d\n", (int)state->dec_param_bOutputDelayDisable, (int)state->dec_param_bRawDecDelayPar);
		
		if( state->dec_param_bCustomFrameMalloc == 1 ) 
		{
			FrameMemOpSet MemOpsObj;
			MemOpsObj.fMallocFrame	= h264_rawdecoder_frameMalloc;
			MemOpsObj.fFreeFrame	= h264_rawdecoder_frameFree;
			MemOpsObj.pUsrObj		= NULL;
			dlog("Set customer defined frame malloc, usrdata %x\n", (int)MemOpsObj.pUsrObj);
			err = DecodeSendCmd_H264Video(IPPVC_SET_FRAMEMEMOP, &MemOpsObj, NULL, state->avc_dec);
			if (IPP_STATUS_NOERR != err)
				dlog( "error: DecodeSendCmd_H264Video(IPPVC_SET_FRAMEMEMOP) %d!\n", (int)err);
		}
		
		err = DecodeSendCmd_H264Video(IPPVC_SET_SKIPMODE, (&state->dec_param_SkipMode), NULL, state->avc_dec);
		if (IPP_STATUS_NOERR != err) 
			return IPP_STATUS_ERR;
		
		
		state->sps_set = 0;
		state->pps_set = 0;
		//dlog( "calling PVAVCDecSeqParamSet_func, avcC.spsSize: %d:: %x, %x, %x, %x, %x, %x, %x, %x\n", 
		//		avcC.spsSize, avcC.sps[0],avcC.sps[1],avcC.sps[2],avcC.sps[3],avcC.sps[4],avcC.sps[5],avcC.sps[6],avcC.sps[7] ); 
		
		//dlog( "calling PVAVCDecPicParamSet_func, avcC.pps: %d:: %x, %x, %x, %x, %x, %x, %x, %x\n", 
	}

	state->current_decode_time = decode_time;
	
	dlog( "calling func_queue_in, completionFunction: %x, completionRefcon: %x\n", (int)deco->completionFunction,  (int)deco->completionRefcon );
	err = func_queue_in(  state->func_item_list, deco->completionFunction,  deco->completionRefcon, state->debug_input_frame_count,  drop_frame, decode_time );
	BAIL_IF_ERR( err );

	deco->completionFunction = NULL;
	deco->completionRefcon = NULL;
	
	if( data != NULL &&  (!is_startcode) )//for robustness
	{
		int src_size = get_nalu_size( data, state->nalu_len_size );
		if( src_size > data_size || src_size < 0 )
		{
			BAIL( kFskErrBadState );
		}
	}

	dlog( "start decoding!\n");
	while(1) 
	{
		int   nAvailFrames = 0;
		
		if( state->bUsed )
		{
			int	ref_idc;
			int	nalu_type;
			int	nalu_size;
			unsigned char *this_nalu = NULL;
			
			if( state->sps_set == 0 )
			{
				dlog( "reading sps data!\n");
				this_nalu = &state->avcC.sps[0];
				nalu_size = state->avcC.spsSize;
				state->sps_set = 1;
			#if 0	
				{ unsigned char *d = &state->avcC.sps[0]; dlog( "state->avcC.spsSize:%d => %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",    state->avcC.spsSize, d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7], d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]  ); }
				{ unsigned char *d = this_nalu;           dlog( "nalu_size:%d           => %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",    nalu_size,           d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7], d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]  ); }
			#endif
			}
			else if( state->pps_set == 0 )
			{
				dlog( "reading pps data!\n");
				this_nalu = &state->avcC.pps[0];
				nalu_size = state->avcC.ppsSize;
				state->pps_set = 1;
			#if 0		
				{ unsigned char *d = &state->avcC.pps[0]; dlog( "state->avcC.ppsSize:%d => %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",    state->avcC.ppsSize, d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7], d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]  ); }
				{ unsigned char *d = this_nalu;           dlog( "nalu_size:%d           => %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",    nalu_size,           d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7], d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]  ); }
			#endif
			}
			else
			{
				dlog( "reading frame nalu data!\n");
				err = check_next_frame_nalu( is_startcode, state->nalu_len_size, &data, &data_size, &nalu_type, &ref_idc, &nalu_size);
				if( err != 0 )
				{//exhaused every nalu
					err = 0;
					break;
				}
				
				this_nalu = data;
			}
		
			if( state->dec_param_NSCCheckDisable ) 
			{
				state->srcBitStream.bsByteLen      = nalu_size;
				state->srcBitStream.bsCurBitOffset = 0;
				state->srcBitStream.pBsCurByte     = state->srcBitStream.pBsBuffer;
				
				dlog( "copy nalu nalu_size: %d => %x,  %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n",  
						nalu_size, this_nalu[0],this_nalu[2],this_nalu[3],this_nalu[4],this_nalu[5],this_nalu[6],this_nalu[7],this_nalu[8],this_nalu[9] );
				IPP_Memcpy( (void *)state->srcBitStream.pBsBuffer, (void *)this_nalu, nalu_size );
				
				if( this_nalu == data )
				{
					data	  += nalu_size;
					data_size -= nalu_size;
				}
			}
		}
		
		//decode a NAL
		{
			Ipp8u *pBsCurByteBackup = state->srcBitStream.pBsCurByte;

			dlog("calling DecodeFrame_H264Video()\n");
			err	= DecodeFrame_H264Video(&state->srcBitStream, &state->pDstPicList, state->avc_dec, &nAvailFrames);
			if ((pBsCurByteBackup == state->srcBitStream.pBsCurByte) && (0 < state->srcBitStream.bsByteLen)) 
			{
				dlog("data is not used!!!\n");
				state->bUsed = 0;
			}
			else 
				state->bUsed = 1;
		}
		
		switch(err)
		{
			case IPP_STATUS_NEW_VIDEO_SEQ:
				{
					IppiSize  picsize;     
					//int	g_ReorderDelay = 0;
					
					dlog("DecodeFrame_H264Video=>IPP_STATUS_NEW_VIDEO_SEQ:\n");
					
					//flush all left pictures for last video sequence
					//output_all_pictures( state->pDstPicList );
					
					dlog("skip mode is %d\n", state->dec_param_SkipMode);
					DecodeSendCmd_H264Video(IPPVC_SET_SKIPMODE, (&state->dec_param_SkipMode), NULL, state->avc_dec);;
					
					//g_ReorderDelay = getReorderDelay(state->avc_dec);
					//dlog("\nThe reorder delay is %d\n", g_ReorderDelay);
					
					err = DecodeSendCmd_H264Video(IPPVC_GET_PICSIZE, NULL, &picsize, state->avc_dec);
					if (IPP_STATUS_NOERR == err)
					{	
						dlog("the pic size of the new sequence is: %d*%d\n", picsize.width, picsize.height);    
					}
					else
					{
						dlog("new sequence is not coming!\n");
					}
				}
				break;
				
				//This return code indicates the internal frame buffer is full.  
				//In this case, user needs to take below actions depending on their desires:
				//1. Output pictures from display list to display buffer or other location (reference implemented below)
				//2. stop decoding. 
				//Note: taking no action will result in endless loop!
			case IPP_STATUS_BUFFER_FULL:
			case IPP_STATUS_FRAME_COMPLETE:
				dlog("DecodeFrame_H264Video=>IPP_STATUS_BUFFER_FULL/IPP_STATUS_FRAME_COMPLETE:err:%d, nAvailFrames: %d\n", (int)err, (int)nAvailFrames);
				output_several_pictures(state, nAvailFrames, state->pDstPicList );
				break;
				
			case IPP_STATUS_NOERR:
				//need more data
				dlog("DecodeFrame_H264Video=>IPP_STATUS_NOERR, need more data:\n");
				break;
				
			case IPP_STATUS_SYNCNOTFOUND_ERR: 
				dlog("DecodeFrame_H264Video=>IPP_STATUS_SYNCNOTFOUND_ERR!!!\n");
				//exit(-1);
				//if (IPP_Feof(fpin)) 
				//{
				//	bLastNALUnit = 1;
				//	InsertSyncCode_H264(&state->srcBitStream);
				//} 
				//else if (videoReloadBuffer(&state->srcBitStream, fpin)) 
				//{
				//	dlog("error: fail to fill one NAL unit in source buffer!\n");
				//	bEndOfStream = 1;
				//}
				break;
				
			case IPP_STATUS_NOTSUPPORTED_ERR: 
			case IPP_STATUS_BITSTREAM_ERR:
			case IPP_STATUS_INPUT_ERR: 
				dlog("DecodeFrame_H264Video=>error: decoding error, error code %d!\n", (int)err);
				state->bUsed = 1;
				break;
				
			default:
				err = IPP_FAIL;
				dlog("DecodeFrame_H264Video=>error: fails to display frame %d, error code %d!\n", (int)state->output_frame_count, (int)err);
				state->bUsed = 1;
				break;
		}
	} 
	
		
bail:
	if( is_eos && (err==0 ))
	{
		dlog("is_eos, calling output_all_pictures()!!!\n");
		output_all_pictures( state, state->pDstPicList );
	}
	
	if( (is_eos || state->bad_state || (err == kFskErrBadState) ) && state->func_item_list != NULL )
	{
		FskErr flush_err = kFskErrShutdown;//dec->error_happened ? kFskErrBadData: kFskErrShutdown;
		dlog("flushing completion fucntions!!!\n");
		dlog("is_eos: %d, state->bad_state: %d, err: %d\n", (int)is_eos, (int)state->bad_state, (int)state->bad_state);

		dlog("calling func_queue_flush()\n");
		func_queue_flush(deco, state->func_item_list, flush_err );
	}
	
	dlog( "out of avcDecodeDecompressFrame: err: %d, in/out: %d/ %d\n", (int)err, state->debug_input_frame_count, state->debug_output_frame_count );
	return err;
}


FskErr avcDecodeFlush(void *stateIn, FskImageDecompress deco )
{
	kinomaAVCDecode		*state	= (kinomaAVCDecode *)stateIn;
	FskErr				err		= kFskErrNone;
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into avcDecodeFlush\n");

	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!\n");
		return kFskErrBadState;
	}

	{
		IppH264PicList  *pDstPicList = state->pDstPicList;
		//process the last several pictures of the video stream
		while (pDstPicList) 
		{
			dlog("flushing picture ... ... POC=%d\n", pDstPicList->pPic->picOrderCnt);
			pDstPicList->pPic->picStatus |= 1;
			pDstPicList = pDstPicList->pNextPic;
		}
	}
	
	if( state->func_item_list != NULL )	
	{
		FskErr flush_err = kFskErrShutdown;//dec->error_happened ? kFskErrBadData: kFskErrShutdown;
		
		dlog("calling func_queue_flush()\n");
		func_queue_flush(deco, state->func_item_list, flush_err );
	}
	
	
	state->current_decode_time = 0;
	
	state->debug_input_frame_count  = 0;
	state->debug_output_frame_count	= 0;
	
//bail:
	dlog( "out of avcDecodeFlush: err: %d\n", (int)err );
	return err;
}

#include <dlfcn.h>

FskErr avcDecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	kinomaAVCDecode *state  = (kinomaAVCDecode *)stateIn;
	unsigned char *data     = (unsigned char *)deco->data;
	int			  data_size = (int)deco->dataSize;
	int			  nalu_type;
	int			  ref_idc;
	int			  block_size;
	UInt32		  frame_type = 0;
	int			  is_startcode = 1;
	FskErr		  err = kFskErrNone;

	dlog( "\n###########################################################################################\n" ); 
	dlog( "into avcDecodeGetMetaData, indec: %d\n", (int)index );

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

#if 0
	{
		int sync		= frame_type == kFskImageFrameTypeSync;
		int droppable   = (frame_type&kFskImageFrameTypeDroppable)  != 0;
		int difference  = (frame_type&kFskImageFrameTypeDifference) != 0;
	
		dlog( "sync:%d, drop:%d, diff:%d \n", sync, droppable, difference );
	}
#endif

	if( value != NULL )
	{
		value->type = kFskMediaPropertyTypeInteger;
		value->value.integer = frame_type;
	}

bail:	
	dlog( "out of avcDecodeGetMetaData: err: %d\n", (int)err );
	return err;
}


FskErr avcDecodeSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaAVCDecode *state = (kinomaAVCDecode *)stateIn;

	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!\n");
		return kFskErrBadState;
	}
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into avcDecodeSetSampleDescription\n");

	state->sampleDescriptionSeed++;
	if( state->sampleDescription != NULL )
		FskMemPtrDisposeAt((void **)&state->sampleDescription);

	state->sampleDescriptionSize = property->value.data.dataSize;

	return FskMemPtrNewFromData(state->sampleDescriptionSize, property->value.data.data, (FskMemPtr *)&state->sampleDescription);
}

#define SET_PREFERRED_PIXEL_FORMAT( want_this_format )					\
	if( prefered_yuvFormat == kFskBitmapFormatUnknown )					\
	{																	\
		UInt32 i = 0;													\
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

FskErr avcDecodeSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property )
{
	kinomaAVCDecode *state = (kinomaAVCDecode *)stateIn;
	FskBitmapFormatEnum prefered_yuvFormat = kFskBitmapFormatUnknown;
	UInt32 count = property->value.integers.count;
	UInt32 propertyType = property->type;

	dlog( "\n###########################################################################################\n" );
	dlog( "into avcDecodeSetPreferredPixelFormat, propertyID: %d, propertyType: %d, count: %d\n", (int)propertyID, (int)property->type, (int)count);
	dlog( "prefered_yuvFormat: %d/%d/%d/%d/%d\n", (int)property->value.integers.integer[0],(int)property->value.integers.integer[1],(int)property->value.integers.integer[2],(int)property->value.integers.integer[3],(int)property->value.integers.integer[4]);

	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!\n");
		return kFskErrBadState;
	}

	dlog( "looking for kFskBitmapFormatYUV420i\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420i)

	dlog( "looking for kFskBitmapFormatYUV420\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420)

	dlog( "looking for kFskBitmapFormatYUV420spvu\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420spvu)
    
	dlog( "looking for kFskBitmapFormatYUV420spuv\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420spuv)
    
	dlog( "looking for kFskBitmapFormatUYVY\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatUYVY)

	if( prefered_yuvFormat != kFskBitmapFormatUnknown )
	{
		dlog( "got matched system preferred: %d\n", (int)prefered_yuvFormat);
		state->dst_pixel_format = prefered_yuvFormat;
	}

	dlog( "state->dst_pixel_format: %d\n", (int)state->dst_pixel_format);

	return kFskErrNone;
}

FskErr avcDecodeGetMaxFramesToQueue (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaAVCDecode *state = (kinomaAVCDecode *)stateIn;
	FskErr				err	 = kFskErrNone;
	
	dlog( "\n###########################################################################################" ); 
	dlog( "into avcDecodeGetMaxFramesToQueue, propertyID: %d", (int)propertyID );
	
	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!");
		return state->bad_state;
	}
	
	property->value.integer	= 20;
	property->type			= kFskMediaPropertyTypeInteger;
	
	//bail:	
	return err;
}


FskErr avcDecodeSetRotation(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaAVCDecode *state = (kinomaAVCDecode *)stateIn;

	dlog( "\n###########################################################################################\n" ); 
	dlog( "into avcDecodeSetSampleDescription\n");
	
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
