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
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#define __FSKIMAGE_PRIV__
#define __FSKBITMAP_PRIV__
//#define __FSKMPEGDECODE_PRIV__

#include "kinomaqtdec.h"
#include "avcC.h"
#include "FskYUV420Copy.h"
//#include "kinoma_utilities.h"
#include "FskEndian.h"
#include "FskBitmap.h"
#include "QTReader.h"

#if TARGET_OS_MAC && (defined(__LP64__) || TARGET_OS_IPHONE)
#define USE_VIDEO_TOOLBOX	1
#endif
#ifdef USE_VIDEO_TOOLBOX
#include <VideoToolbox/VideoToolbox.h>
#else
#include <QuickTime/QuickTime.h>
#endif


//#define CACHE_OUTPUT_FRAMES
#ifdef  CACHE_OUTPUT_FRAMES
//#define REORDER_CACHED_FRAMES
#endif

#ifdef REORDER_CACHED_FRAMES
#define INTERNAL_YUV_BUFFER_TOTAL	2
#else
#define INTERNAL_YUV_BUFFER_TOTAL	1
#endif

//#define BUF_PRINTF
//#define DUMP_YUV

#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(kinomaqtdec, kinomaqtdec);
#define mlog  FskkinomaqtdecPrintfMinimal
#define nlog  FskkinomaqtdecPrintfNormal
#define vlog  FskkinomaqtdecPrintfVerbose
#define dlog  FskkinomaqtdecPrintfDebug

#define KINOMA_IPP_AVC_PARSE

#define DST_YUV420_FORMAT	kFskBitmapFormatYUV420
//#define DST_YUV420_FORMAT	kFskBitmapFormatUYVY

#define kBitmapCacheSize		50
#define SC_LEN					4	//startcode length
#define DEFAULT_NALU_LEN		4
#define kDefaultNALULengthSize	4
#define NAL_REF_IDC_BITS		0x60
#define NAL_UNITTYPE_BITS       0x1f
#define	kDefaultStratCodeBufferSize	1024*1024


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


static FskErr qtDecSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr qtDecSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property );
static FskErr qtDecodeGetMaxFramesToQueue (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord qtDecProperties[] = 
{
	{kFskMediaPropertySampleDescription,		kFskMediaPropertyTypeData,		NULL,		qtDecSetSampleDescription},
 	{kFskMediaPropertyFormatInfo,				kFskMediaPropertyTypeData,		NULL,		qtDecSetSampleDescription},
	{kFskMediaPropertyPixelFormat,				kFskMediaPropertyTypeUInt32List,NULL,		qtDecSetPreferredPixelFormat},
	{kFskMediaPropertyMaxFramesToQueue,			kFskMediaPropertyTypeInteger,	qtDecodeGetMaxFramesToQueue, NULL},
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
	int					total;
	int					strd;
	int					slht;
	int					width;
	int					height;
	int					frame_size;
	int					composition_time[INTERNAL_YUV_BUFFER_TOTAL];
	unsigned char		*y[INTERNAL_YUV_BUFFER_TOTAL];
	unsigned char		*u[INTERNAL_YUV_BUFFER_TOTAL];
	unsigned char		*v[INTERNAL_YUV_BUFFER_TOTAL];
}YUVBuffer;




typedef struct 
{
	UInt32				sampleDescriptionSize;
	unsigned char		*sampleDescription;
	UInt32				sampleDescriptionSeed;

	FskImageDecompress	deco;
#ifdef USE_VIDEO_TOOLBOX
	VTDecompressionSessionRef decompressionSession;
	VTDecompressionOutputCallbackRecord callbackRecord;
	CMVideoCodecType codecType;
	CMVideoFormatDescriptionRef videoDesc;
#else
	ICMDecompressionSessionRef	decompressionSession; // decompresses and scales captured frames
#endif

	int					sync_mode;
	FskListMutex		func_item_list;
	int					nalu_len_size;
	
	int					bad_state;
	
	UInt32				timeScale;
	FskInt64			decode_time_init;
	
	YUVBuffer			*yuv_buffer;
	
	FskBitmap			bitmaps[kBitmapCacheSize];
	FskBitmapFormatEnum	dst_pixel_format;

	int					display_width;
	int					display_height;
	
	int					dec_info_available;
	
	int					debug_input_frame_count;
	int					debug_output_frame_count;
	int					debug_dropped_frame_count;
} kinomaQTDecode;



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

#ifdef CACHE_OUTPUT_FRAMES
static FskErr func_queue_putback( FskListMutex func_item_list, FskImageDecompressComplete	completionFunction,  void	*completionRefcon, int frame_number, int drop, FskInt64 decode_time )
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
	
	FskListMutexPrepend(func_item_list, item);
	
bail:
	return err;
}
#endif

static FskErr func_queue_out( FskListMutex func_item_list, FskImageDecompressComplete	*completionFunction,  void	**completionRefcon, int *frame_number, int *drop, FskInt64 *decode_time )
{
	FuncItemPtr	item = NULL;
	FskErr      err = kFskErrNone;
	
	item = FskListMutexRemoveFirst(func_item_list);
	
	if( item != NULL )
	{
		//dlog( "return cached\n" );		
		*completionFunction = item->completionFunction;
		*completionRefcon   = item->completionRefcon;
		*frame_number		= item->frame_number;
		*drop				= item->drop;
		*decode_time		= item->decode_time;
		FskMemPtrDispose(item);
	}
	else
	{
		//dlog( "return default\n" );		
		*completionFunction = NULL;
		*completionRefcon   = 0;
		*frame_number		= 0;
		*drop				= 0;
		*decode_time		= 0;
		
		err = 1;
	}
	
	return err;
}


static void func_queue_flush( FskImageDecompress deco, FskListMutex func_item_list, FskErr flush_err )
{
	if( func_item_list == NULL )
		return;
	
	while(1)
	{
		FuncItemPtr	func_item = FskListMutexRemoveFirst(func_item_list);
		
		if( func_item == NULL )
		{
			dlog( "no more func_item in queue!!!\n" );		
			break;
		}
		
		if( func_item->completionFunction != NULL && func_item->completionRefcon != NULL )
		{
			dlog( "calling completionFunction/RefCon with error: %d, completionFunction: %x, completionRefcon: %d\n", (int)flush_err, (int)func_item->completionFunction, (int)func_item->completionRefcon );		
			(func_item->completionFunction)(deco, func_item->completionRefcon, flush_err, NULL);
		}
		else
		{
			dlog( "func_item->completionFunction == NULL || func_item->completionRefcon != NULL !!!\n" );		
		}
		
		FskMemPtrDispose(func_item);
	}
}


static void SetSizeByLen( unsigned char *data, unsigned char len, short size )
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


static int until_next_start_code( unsigned char *d, int size )
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
	dlog( "into check_next_frame_nalu, *size_in_out: %d, data: %d, %d, %d, %d, %d, %d, %d, %d\n", 
			*size_in_out, next_data[0], next_data[1], next_data[2], next_data[3], next_data[4], next_data[5], next_data[6], next_data[7] ); 

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


static void RefitBitmap( FskBitmapFormatEnum dst_pixel_format, int width, int height, FskBitmap *bits )
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

	*bits = b;
}



void YUVBuffer_dispose( YUVBuffer *b )
{
	if( b != NULL )
	{
		int i;
		for( i = 0; i < INTERNAL_YUV_BUFFER_TOTAL; i++ )
		{
			if( b->y[i] != NULL )
				FskMemPtrDispose(b->y[i]);
			
			if( b->u[i] != NULL )
				FskMemPtrDispose(b->u[i]);
			
			if( b->v[i] != NULL )
				FskMemPtrDispose(b->v[i]);
		}
		
		FskMemPtrDispose( b );
	}
}



FskErr YUVBuffer_new( YUVBuffer **b_out )
{
	int i;
	YUVBuffer *b  = NULL;
	FskErr    err = kFskErrNone;
	
	err = FskMemPtrNewClear(sizeof(YUVBuffer), (FskMemPtr *)&b );
	BAIL_IF_ERR(err);
	
	for( i = 0; i < INTERNAL_YUV_BUFFER_TOTAL; i++ )
	{
		b->composition_time[i] = -1;
		b->y[i]	= NULL;
		b->u[i]	= NULL;
		b->v[i]	= NULL;
	}
	
	b->total = 0;
	b->strd  = 0;
	b->slht  = 0;
	b->width = 0;
	b->height= 0;
	b->frame_size  = 0;
	
bail:
	*b_out = b;
	return err;
}


static int refit_yuv_16_interleave(int yuvu_width, int uyvy_height, int uyvy_rb, unsigned char *uyvy_data, int y_rb, unsigned char *b_y, unsigned char *b_u, unsigned char *b_v  )
{
	int s_stride  = 2*uyvy_rb - (2*yuvu_width);
	int y_stride  = 2*y_rb - yuvu_width;
	int uv_rb     = y_rb>>1;
	int uv_stride = uv_rb - (yuvu_width>>1);
	int	err = 0;
	
	unsigned char *s0;
	unsigned char *s1;
	unsigned char *dy0;
	unsigned char *dy1;
	unsigned char *u0;
	unsigned char *v0; 
	int i,j;
	
	dlog(  "in refit_yuv_16_interleave uyvy_rb: %d, yuvu_width: %d, uyvy_height: %d\n", uyvy_rb, yuvu_width, uyvy_height );
	
	s0  = uyvy_data;
	s1  = s0 + uyvy_rb;
	
	dy0 = b_y;
	dy1 = dy0 + y_rb;
	u0  = b_u;
	v0  = b_v;
	
	#define PACK_YCbYCr	\
	*(dy0++) =  s0[0];	\
	*(dy0++) =  s0[2];	\
	*(dy1++) =  s1[0];	\
	*(dy1++) =  s1[2];	\
	*(u0++)  =  s0[1];	\
	*(v0++)  =  s0[3];	\
	s0 += 4;			\
	s1 += 4;			
		
	#define PACK_YCrYCb	\
	*(dy0++) =  s0[0];	\
	*(dy0++) =  s0[2];	\
	*(dy1++) =  s1[0];	\
	*(dy1++) =  s1[2];	\
	*(u0++)  =  s0[3];	\
	*(v0++)  =  s0[1];	\
	s0 += 4;			\
	s1 += 4;			
		
	#define PACK_CbYCrY	\
	*(dy0++) =  s0[1];	\
	*(dy0++) =  s0[3];	\
	*(dy1++) =  s1[1];	\
	*(dy1++) =  s1[3];	\
	*(u0++)  =  s0[0];	\
	*(v0++)  =  s0[2];	\
	s0 += 4;			\
	s1 += 4;			
		
	#define PACK_CrYCbY	\
	*(dy0++) =  s0[1];	\
	*(dy0++) =  s0[3];	\
	*(dy1++) =  s1[1];	\
	*(dy1++) =  s1[3];	\
	*(u0++)  =  s0[2];	\
	*(v0++)  =  s0[0];	\
	s0 += 4;			\
	s1 += 4;			
	
	for( i = 0; i < uyvy_height/2; i++ )
	{
		for( j = 0; j < yuvu_width/2; j++ )
		{
			//dlog(  "i/j: %d/%d\n", i, j );
			PACK_CbYCrY
		}
		
		dy0 += y_stride;
		dy1  = dy0 + y_rb;
		u0  += uv_stride;
		v0  += uv_stride; 
		s0  += s_stride;
		s1   = s0 + uyvy_rb;
	}	
	
//bail:
	dlog(  "out of refit_yuv_16_interleave, err: %d\n", err );
	
	return err;
}


void YUVBuffer_reset( YUVBuffer *b )
{
	int i;
	
	for( i = 0; i < INTERNAL_YUV_BUFFER_TOTAL; i++ )
		b->composition_time[i] = -1;
	
	b->total = 0;
	b->strd  = 0;
	b->slht  = 0;
	b->width = 0;
	b->height= 0;
	b->frame_size  = 0;
}


int  YUVBuffer_next_index( YUVBuffer *b )
{
	int  i;
	int  min = -1;
	int	 min_index = 0;
	
	//find index pointing to the earliest frame
	for( i = 0; i < INTERNAL_YUV_BUFFER_TOTAL; i++ )
	{
		if( b->composition_time[i] < 0 )
		{
			dlog(  "YUVBuffer_next_index, returning idx: %d, composition_time: %d\n", i, (int)b->composition_time[i] );
			return i;
		}
		
		if( b->composition_time[i] < min || min < 0 )
		{
			dlog(  "YUVBuffer_next_index, getting idx: %d, composition_time: %d\n", i, (int)b->composition_time[i] );
			min		  = b->composition_time[i];
			min_index = i;
		}
	}
	
	return min_index;
}


int  YUVBuffer_push( FskBitmapFormatEnum dst_pixel_format, YUVBuffer *b,  CVPixelBufferRef pixelBuffer, FskInt64 composition_time  )
{
	int	   idx				= YUVBuffer_next_index( b );
	//size_t  plane_count	= CVPixelBufferGetPlaneCount(pixelBuffer);
	//Boolean is_planar		= CVPixelBufferIsPlanar(pixelBuffer);
	OSType src_pix_format	= CVPixelBufferGetPixelFormatType(pixelBuffer);
	unsigned char *qt_yuv_data = (unsigned char *)CVPixelBufferGetBaseAddress(pixelBuffer);
	int		qt_width		= (int)CVPixelBufferGetWidth(pixelBuffer);
	int		qt_height		= (int)CVPixelBufferGetHeight(pixelBuffer);
	//int		qt_slht			= b->height;
	int		qt_strd			= (int)CVPixelBufferGetBytesPerRow(pixelBuffer);
	//int		qt_frame_size	= CVPixelBufferGetDataSize(pixelBuffer);

	int err = 0;
	
	dlog(  "YUVBuffer_push, idx: %d, composition_time: %d\n", idx, (int)b->composition_time[idx] );
	
#ifdef USE_VIDEO_TOOLBOX
	if( src_pix_format != kCVPixelFormatType_422YpCbCr8 )
#else
	if( src_pix_format != k2vuyPixelFormat )
#endif
	{
		BAIL( kFskErrBadState );
	}
	
	//dlog( "yuv_width: %d\n",  (int)yuv_width  );
	//dlog( "yuv_height: %d\n", (int)yuv_height );
	//dlog( "src_pix_format: %x\n", (int)src_pix_format );
	//dlog( "qt_yuv_data: %x\n", (int)qt_yuv_data );
	//dlog( "strd: %d\n", (int)strd );
	//dlog( "yuv_size: %d\n", (int)yuv_size );
	//dlog( "is_planar: %d\n", (int)is_planar );
	//dlog( "plane_count: %d\n", (int)plane_count );
	//dlog( "src_pix_format == k2vuyPixelFormat\n" );
	
	b->width	= qt_width;
	b->height	= qt_height;
	
	if( dst_pixel_format == kFskBitmapFormatUYVY )
	{
		b->strd			= qt_strd;
		b->slht			= b->height;
		b->frame_size	= b->strd*b->height;
		
		if( b->y[idx] == NULL )
		{
			err = FskMemPtrNew(b->frame_size, (FskMemPtr *)&b->y[idx]);
			BAIL_IF_ERR(err);
		}
		memcpy( (void *)b->y[idx],  qt_yuv_data, b->frame_size );
	}
	else
	{
		b->strd			= qt_strd>>1;		//'2vuy' to planar raw bytes
		b->slht			= b->height;
		b->frame_size	= b->strd*b->slht;	//'2vuy' to planar raw bytes
		
		if( b->y[idx] == NULL )
		{
			err = FskMemPtrNew(b->frame_size, (FskMemPtr *)&b->y[idx]);
			BAIL_IF_ERR(err);
		}
		
		if( b->u[idx] == NULL )
		{
			err = FskMemPtrNew(b->frame_size>>2, (FskMemPtr *)&b->u[idx]);
			BAIL_IF_ERR(err);
		}
		
		if( b->v[idx] == NULL )
		{
			err = FskMemPtrNew(b->frame_size>>2, (FskMemPtr *)&b->v[idx]);
			BAIL_IF_ERR(err);
		}	
		
		err = refit_yuv_16_interleave(b->strd, b->slht, qt_strd, qt_yuv_data, b->strd, b->y[idx], b->u[idx], b->v[idx] );
		BAIL_IF_ERR( err );
	}
	
	b->composition_time[idx] = (int)composition_time;
	b->total++;
	if(b->total > INTERNAL_YUV_BUFFER_TOTAL )
		b->total = INTERNAL_YUV_BUFFER_TOTAL;
	
bail:
	return err;
}


int YUVBuffer_ready( YUVBuffer *b )
{
	return b->total >= INTERNAL_YUV_BUFFER_TOTAL;
}


void  YUVBuffer_pull( YUVBuffer *b,  unsigned char **y, unsigned char **u, unsigned char **v, int *composition_time )
{
	if( !YUVBuffer_ready(b) )
	{
		dlog(  "YUVBuffer_pull: empty\n" );
		*y = NULL;
		*u = NULL;
		*v = NULL;
		*composition_time = 0;
		return;
	}
	else
	{
		int	idx = YUVBuffer_next_index( b );

		*y = b->y[idx];
		*u = b->u[idx];
		*v = b->v[idx];
		*composition_time = b->composition_time[idx];
		dlog(  "YUVBuffer_pull, idx: %d, composition_time: %d\n", idx, (int)b->composition_time[idx] );
		
		//rest it so that no one will use it by mistake
		b->composition_time[idx] = -1;
		
		
#ifdef DUMP_YUV	
		if( state->dst_pixel_format == kFskBitmapFormatUYVY )
		{
			dlog("dump 2vuy(uyvu) frame\n" );
			
			unsigned char *uyvu_data= (unsigned char *)b->y[idx];
			int			  uyvu_size	= b->strd * b->slht;
			static FILE   *f_out	= NULL;
			const char    *output_file_path ="/Users/bnie/Documents/Kinoma Media/qtdec_dump_2vuy.yuv";
			
			if( f_out == NULL )
			{
				int frame_rate = 15;
				int	width  = b->strd>>1;
				int	height = b->slht;
				unsigned char *h = uyvu_data;
				
				dlog("creating yuv file: %s, frame_rate: %d, width: %d, height: %d\n", 
						output_file_path, frame_rate, width, height );
				f_out = fopen(output_file_path,"wb");	
				if( f_out == NULL )
					dlog("output file creation failed\n");
				
				h[0] = '2';
				h[1] = 'v';
				h[2] = 'u';
				h[3] = 'y';
				h[4] = ' ';
				h[5] = ' ';
				h[6] = (frame_rate>>8)&0xff;
				h[7] = (frame_rate>>0)&0xff;
				h[8] = (width>>8)&0xff;
				h[9] = (width>>0)&0xff;
				h[10] = (height>>8)&0xff;
				h[11] = (height>>0)&0xff;
			}
			
			fwrite( (void *)uyvu_data,   uyvu_size, 1, f_out);
			fflush( f_out );
		}
		else		
		{
			dlog("dump yuv420 planar frame\n" );
			
			unsigned char *y0		= (unsigned char *)b->y[idx];
			unsigned char *u0		= (unsigned char *)b->u[idx];
			unsigned char *v0		= (unsigned char *)b->v[idx];
			int			  y_size	= b->frame_size;
			int			  uv_size	= y_size>>2;
			static FILE   *f_out	= NULL;
			const char    *output_file_path ="/Users/bnie/Documents/Kinoma Media/qtdec_dump_planar.yuv";
			
			if( f_out == NULL )
			{
				int frame_rate = 15;
				int	width  = b->width;
				int	height = b->height;
				unsigned char *h = y0;
				
				dlog("creating yuv file: %s, frame_rate: %d, width: %d, height: %d\n", 
						output_file_path, frame_rate, width, height );
				f_out = fopen(output_file_path,"wb");	
				if( f_out == NULL )
					dlog("output file creation failed\n");
				
				h[0] = 'y';
				h[1] = 'u';
				h[2] = 'v';
				h[3] = '4';
				h[4] = '2';
				h[5] = '0';
				h[6] = (frame_rate>>8)&0xff;
				h[7] = (frame_rate>>0)&0xff;
				h[8] = (width>>8)&0xff;
				h[9] = (width>>0)&0xff;
				h[10] = (height>>8)&0xff;
				h[11] = (height>>0)&0xff;
			}
			
			fwrite( (void *)y0,   y_size, 1, f_out);
			fwrite( (void *)u0,  uv_size, 1, f_out);
			fwrite( (void *)v0,  uv_size, 1, f_out);
			fflush( f_out );
		}
#endif	
		
	}
}


static FskErr send_out_frame_sync( kinomaQTDecode *state, FskImageDecompress deco )
{
	int						width;
	int						height;
	FskBitmap				bits = NULL;
	//int						input_frame_count	= 0;
	//FskInt64				input_decode_time	= 0;		
	//FskInt64				output_decode_time	= 0;
	int						output_display_time	= 0;
	FskErr					err	= kFskErrNone;
	
	if( !YUVBuffer_ready( state->yuv_buffer ) )
	{	
		dlog( "not ready yet\n" );		
		goto bail;
	}
	
	dlog( "cache setting width/height\n" );		
	width  = state->yuv_buffer->width;
	height = state->yuv_buffer->height;
	
	dlog( "ready to output\n" );		
	if( deco->bits != NULL ) 
	{
		bits	   = deco->bits;
		deco->bits = NULL;
		RefitBitmap( state->dst_pixel_format, width, height, &deco->bits );
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
					RefitBitmap( state->dst_pixel_format, width, height, &state->bitmaps[i] );
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
			err = FskBitmapNew((SInt32)width, (SInt32)height, state->dst_pixel_format, &bits);
			BAIL_IF_ERR( err );
		}
		
		if( index != -1 ) 
		{
			state->bitmaps[index] = bits;
			FskBitmapUse( bits );
		}
	}
	
	{
		FskBitmapFormatEnum pixelFormat;
		SInt32			dst_y_rb;
		unsigned char	*dstPtr	= NULL;
		unsigned char	*src_y	= NULL;
		unsigned char	*src_u	= NULL;	
		unsigned char	*src_v	= NULL;	
		int				frame_size;
		
		FskBitmapWriteBegin( bits, (void**)(&dstPtr), (SInt32 *)&dst_y_rb, &pixelFormat );
		if (state->dst_pixel_format != pixelFormat)
		{
			FskBitmapWriteEnd( bits );
			BAIL( kFskErrBadData );
		}
		
		YUVBuffer_pull( state->yuv_buffer, &src_y, &src_u, &src_v, &output_display_time  );
		frame_size = state->yuv_buffer->frame_size;
		
		if( state->dst_pixel_format == kFskBitmapFormatUYVY )
		{
			int dst_row = width * 2;
			int stride = state->yuv_buffer->strd;

			if (dst_row == stride) {
				dlog("copy 'WITHOUT' padding data");
				memcpy(	(void *)dstPtr, (void *)src_y, frame_size );
			}
			else {
				int i;
				dlog("copy 'WITH' padding data");
				for (i = 0; i < height; i++) {
					memcpy(	(void *)dstPtr, (void *)src_y, dst_row );
					dstPtr += dst_row;
					src_y  += stride;
				}
			}
		}
		else
		{
			int	dst_slht	= height;
			int src_y_rb	= state->yuv_buffer->strd;
			int src_uv_rb	= src_y_rb>>1;
			//int slht_y		= state->yuv_buffer->slht;
			//int slht_uv     = slht_y>>1;
			int dw = 0;;
			int dh = 0;
			
			src_y += dh * src_y_rb	   + dw;
			src_u += (dh>>1)*src_uv_rb + (dw>>1);
			src_v += (dh>>1)*src_uv_rb + (dw>>1);
			
			unsigned char *dst_y = dstPtr;
			unsigned char *dst_u = dst_y + dst_y_rb * dst_slht;
			unsigned char *dst_v = dst_u + (dst_y_rb >> 1) * ((dst_slht + 1) >> 1);
			
			FskYUV420Copy(	width, height,
						  src_y, src_u, src_v, src_y_rb, src_uv_rb,
						  dst_y, dst_u, dst_v,
						  dst_y_rb, dst_y_rb >> 1);
		}
	}	
	
	dlog( "returning a bits: %x\n", (int)bits);		
	FskBitmapWriteEnd( bits );
	deco->bits = bits;
	
bail:
	dlog( "out of send_out_frame_sync, err %d\n", (int)err);		
	return err;
}


static FskErr send_out_frame_async( kinomaQTDecode	*state, CVPixelBufferRef pixelBuffer, TimeValue64 displayTime )
{
	int						width;
	int						height;
	int						stride;
	FskBitmap				bits = NULL;
	FskImageDecompress		deco = state->deco;
	FskImageDecompressComplete	completionFunction = NULL;
	void					*completionRefcon	= NULL;
	int						input_frame_count	= 0;
	int						drop_flag			= 0;
	FskInt64				input_decode_time	= 0;		
	//FskInt64				output_decode_time	= 0;
	int						output_display_time	= 0;
	FskErr					err	= kFskErrNone;
	
	dlog( "into  completion function\n" );		
	dlog( "retrieving completion function\n" );		
	err = func_queue_out( state->func_item_list, 	&completionFunction,  &completionRefcon, &input_frame_count, &drop_flag, &input_decode_time );
	if( err != 0 )
	{
		dlog( "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: no completion function, we should always have a completionFunction to return a bits!!!\n");
		err = 0;
		goto bail;
	}
	
	if( drop_flag )
	{
		dlog( "XXXXXXXXthis frame is dropped!!!\n");
		if( completionFunction != NULL && completionRefcon != NULL)	
			(completionFunction)(deco, completionRefcon, kFskErrShutdown, NULL);
		goto bail;
	}

#ifdef CACHE_OUTPUT_FRAMES
	if( state->yuv_buffer == NULL )
	{
		err = YUVBuffer_new( &state->yuv_buffer );
		BAIL_IF_ERR( err );
	}
	
	dlog( "calling  YUVBuffer_push\n" );		
	err =  YUVBuffer_push( state->dst_pixel_format, state->yuv_buffer, pixelBuffer, displayTime );
	BAIL_IF_ERR( err );
	
	if( !YUVBuffer_ready( state->yuv_buffer ) )
	{	
		dlog( "not ready yet, put it back\n" );		
		err = func_queue_putback( state->func_item_list, completionFunction,  completionRefcon, input_frame_count, drop_flag, input_decode_time );
		goto bail;
	}
	
	dlog( "cache setting width/height\n" );		
	width  = state->yuv_buffer->width;
	height = state->yuv_buffer->height;
#else
	dlog( "no cache setting width/height\n" );		
	width  = (int)CVPixelBufferGetWidth(pixelBuffer);
	height = (int)CVPixelBufferGetHeight(pixelBuffer);
#endif
	
	dlog( "ready to output\n" );		
	if( deco->bits != NULL ) 
	{
		bits	   = deco->bits;
		deco->bits = NULL;
		RefitBitmap( state->dst_pixel_format, width, height, &deco->bits );
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
					RefitBitmap( state->dst_pixel_format, width, height, &state->bitmaps[i] );
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
			err = FskBitmapNew((SInt32)width, (SInt32)height, state->dst_pixel_format, &bits);
			if (err) goto bail;
		}
		
		if( index != -1 ) 
		{
			state->bitmaps[index] = bits;
			FskBitmapUse( bits );
		}
	}
	
	{
		FskBitmapFormatEnum pixelFormat;
		SInt32			dst_y_rb;
		unsigned char	*dstPtr	= NULL;
		unsigned char	*src_y	= NULL;
#ifdef CACHE_OUTPUT_FRAMES
		unsigned char	*src_u	= NULL;
		unsigned char	*src_v	= NULL;	
#endif
        int				frame_size;
		
		FskBitmapWriteBegin( bits, (void**)(&dstPtr), (SInt32 *)&dst_y_rb, &pixelFormat );
		if (state->dst_pixel_format != pixelFormat)
		{
			FskBitmapWriteEnd( bits );
			err = kFskErrBadData;
			goto bail;
		}

#ifdef CACHE_OUTPUT_FRAMES			
		YUVBuffer_pull( state->yuv_buffer, &src_y, &src_u, &src_v, &output_display_time  );
		frame_size = state->yuv_buffer->frame_size;
#else
		stride		= (int)CVPixelBufferGetBytesPerRow(pixelBuffer);
		frame_size 	= stride * height;
		src_y 		= (unsigned char *)CVPixelBufferGetBaseAddress(pixelBuffer);
		output_display_time = (int)displayTime;
#endif
		
		if( state->dst_pixel_format == kFskBitmapFormatUYVY )
		{
			int dst_row = width * 2;
			if (dst_row == stride) {
				dlog("copy 'WITHOUT' padding data");
				memcpy(	(void *)dstPtr, (void *)src_y, frame_size );
			}
			else {
				int i;
				dlog("copy 'WITH' padding data");
				for (i = 0; i < height; i++) {
					memcpy(	(void *)dstPtr, (void *)src_y, dst_row );
					dstPtr += dst_row;
					src_y  += stride;
				}
			}
		}
		else
#ifndef CACHE_OUTPUT_FRAMES	
		{
			unsigned char *uyvu  = src_y;
			int			  uyvy_rb = (int)CVPixelBufferGetBytesPerRow(pixelBuffer);
			unsigned char *dst_y = dstPtr;
			unsigned char *dst_u = dst_y + dst_y_rb * height;
			unsigned char *dst_v = dst_u + (dst_y_rb >> 1) * ((height + 1) >> 1);
			
			err = refit_yuv_16_interleave( width, height, uyvy_rb, uyvu, dst_y_rb, dst_y, dst_u, dst_v  );
			BAIL_IF_ERR( err );
		}
#else  //CACHE_OUTPUT_FRAMES	
		{
			int	dst_slht	= height;
			int src_y_rb	= state->yuv_buffer->strd;
			int src_uv_rb	= src_y_rb>>1;
			//int slht_y		= state->yuv_buffer->slht;
			//int slht_uv     = slht_y>>1;
			int dw = 0;;
			int dh = 0;
			
			src_y += dh * src_y_rb	   + dw;
			src_u += (dh>>1)*src_uv_rb + (dw>>1);
			src_v += (dh>>1)*src_uv_rb + (dw>>1);
			
			unsigned char *dst_y = dstPtr;
			unsigned char *dst_u = dst_y + dst_y_rb * dst_slht;
			unsigned char *dst_v = dst_u + (dst_y_rb >> 1) * ((dst_slht + 1) >> 1);
			
			FskYUV420Copy(	width, height,
						  src_y, src_u, src_v, src_y_rb, src_uv_rb,
						  dst_y, dst_u, dst_v,
						  dst_y_rb, dst_y_rb >> 1);
#ifdef BUF_PRINTF
			{
				int i = input_frame_count % 10;
				
				static int tmp_displayTime[10];
				static int tmp_input_decode_time[10];
				static int tmp_input_frame_count[10];
				
				tmp_displayTime[i]		 = output_display_time;
				tmp_input_decode_time[i] = input_decode_time;
				tmp_input_frame_count[i] = input_frame_count;
				
				//if( i == 0 )
				for( i = 0; i < 10; i ++ )
				{
					int x0 = 0;
					int y0 = i*15;
					
					buf_printf(0, dst_y, dst_y_rb, height, x0, y0, "#%2d display time: %d, decode time: %d",  tmp_input_frame_count[i], tmp_displayTime[i], tmp_input_decode_time[i]);
				}
			}
#endif  //BUF_PRINTF
		}
#endif  //CACHE_OUTPUT_FRAMES
	}	
	
	dlog( "returning a bits: %x\n", (int)bits);		
	FskBitmapWriteEnd( bits );
	if( completionFunction != NULL && completionRefcon != NULL )	
		(completionFunction)(deco, completionRefcon, kFskErrNone, bits);
	
bail:
	dlog( "out of send_out_frame_async, err %d\n", (int)err);		
	return err;
}


FskErr qtDecCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
	dlog( "\n###########################################################################################\n" ); 
    
	//avc
	*canHandle  = format == 'avc1';
	*canHandle |= FskStrCompare(mime, "x-video-codec/avc") == 0;
	
	//263
#ifdef USE_VIDEO_TOOLBOX
	*canHandle |= format == 'h263';
	*canHandle |= format == 's263';
	*canHandle |= FskStrCompare(mime, "x-video-codec/263") == 0;
	//*canHandle |= FskStrCompare(mime, "x-video-codec/h263-flash") == 0;
#else
	//*canHandle |= format == 'h263';
	//*canHandle |= format == 's263';
	//*canHandle |= FskStrCompare(mime, "x-video-codec/263") == 0;
#endif
	
	//mp4v
	*canHandle |= format == 'mp4v';
	*canHandle |= FskStrCompare(mime, "x-video-codec/mp4") == 0;
	
	dlog( "in qtDecCanHandle: format: %d, mime: %s, *canHandle: %d\n", (int)format, mime, (int) *canHandle ); 
	
	return kFskErrNone;
}


FskErr qtDecNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension)
{
	kinomaQTDecode *state;
	//int i;
	FskErr err;
	
#ifdef _WIN32
	err = InitializeQTML(0L);
	if (err != noErr) 
		goto bail;
#endif
	
#ifndef USE_VIDEO_TOOLBOX
	err = EnterMovies();
	if (err != noErr)
		goto bail;
#endif
		
	
	err = FskMemPtrNewClear(sizeof(kinomaQTDecode), (FskMemPtr *)&state);
	BAIL_IF_ERR( err ); 
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "in qtDecNew allocated state: %x\n", (int)state ); 
	
	err = FskListMutexNew(&state->func_item_list, "FuncItemList");
	BAIL_IF_ERR( err ); 
	
	state->bad_state		= 0;
	state->timeScale		= 300;
	
	state->nalu_len_size	= kDefaultNALULengthSize;

	state->decode_time_init = 0;

	state->deco					= deco;
	state->dec_info_available	= 0;
	
	state->dst_pixel_format = kFskBitmapFormatYUV420;
	
	state->sync_mode		= 0;	
#ifdef USE_VIDEO_TOOLBOX
	state->codecType = ((format == 'avc1') || (FskStrCompare(mime, "x-video-codec/avc") == 0)) ? kCMVideoCodecType_H264 :
					   ((format == 'h263') || (format == 's263') || (FskStrCompare(mime, "x-video-codec/263") == 0)) ? kCMVideoCodecType_H263 :
//					   ((FskStrCompare(mime, "x-video-codec/h263-flash") == 0)) ? kCMVideoCodecType_H263 :
						kCMVideoCodecType_MPEG4Video;
#endif

bail:
	if (kFskErrNone != err)
	{
		qtDecDispose(state, deco);
		state = NULL;
	}
	
	deco->state = state;
	
	return err;
}


FskErr qtDecDispose(void *stateIn, FskImageDecompress deco)
{
	kinomaQTDecode *state = (kinomaQTDecode *)stateIn;
	int i;
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into qtDecDispose\n" ); 
	
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
		
		if( state->yuv_buffer )
		{
			YUVBuffer_dispose( state->yuv_buffer );
		}
		
		if( state->decompressionSession != NULL )
		{
#ifdef USE_VIDEO_TOOLBOX
			if (state->decompressionSession != NULL)
			{
				VTDecompressionSessionInvalidate(state->decompressionSession);
				CFRelease(state->decompressionSession);
			}
			if (state->videoDesc != NULL)
			{
				CFRelease(state->videoDesc);
			}
#else
			ICMDecompressionSessionRelease( state->decompressionSession );
#endif
		}
			
		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}


#ifdef USE_VIDEO_TOOLBOX
static void decoder_callback(void *decompressionOutputRefCon, void *sourceFrameRefCon, OSStatus status, VTDecodeInfoFlags infoFlags, CVImageBufferRef imageBuffer, CMTime presentationTimeStamp, CMTime presentationDuration )
{
	kinomaQTDecode *state = (kinomaQTDecode *)decompressionOutputRefCon;
	OSStatus err = noErr;

	if ((imageBuffer == NULL) || (CFGetTypeID(imageBuffer) != CVPixelBufferGetTypeID())) {
		goto bail;
	}

	CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)imageBuffer;
	TimeValue64 displayTime = CMTimeGetSeconds(presentationDuration);

	CVPixelBufferLockBaseAddress(pixelBuffer, 0);

	if (state->sync_mode)
	{
		//if( drop_flag )
		if (state->yuv_buffer == NULL)
		{
			err = YUVBuffer_new(&state->yuv_buffer);
			BAIL_IF_ERR(err);
		}

		dlog("calling  YUVBuffer_push\n");
		err = YUVBuffer_push(state->dst_pixel_format, state->yuv_buffer, pixelBuffer, displayTime);
		BAIL_IF_ERR(err);
	}
	else
	{
		err = send_out_frame_async(state, pixelBuffer, displayTime);
		BAIL_IF_ERR(err);
	}

	CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

bail:
	;
}
#else
static void  decoder_callback(void *refCon, OSStatus result, ICMDecompressionTrackingFlags flag, CVPixelBufferRef pixelBuffer, 
              TimeValue64 displayTime, TimeValue64 displayDur, ICMValidTimeFlags validTimeFlags, void *rsvr, void *srcFrmRefCon )
{
	kinomaQTDecode	*state	= (kinomaQTDecode *)refCon;
	OSStatus		err		= noErr;
	
	dlog( "$$$$$$\n");
	dlog( "$$$$$$ decoder_callback, result: %d, flag: %x\n", (int)result, (int)flag );
	dlog( "$$$$$$ srcFrmRefCon:   %d\n",	(int)srcFrmRefCon );
	dlog( "$$$$$$ displayTime:    %d\n",	(int)displayTime );
	dlog( "$$$$$$ displayDur:     %d\n",	(int)displayDur );
	dlog( "$$$$$$ validTimeFlags: %d\n", (int)validTimeFlags );
	dlog( "$$$$$$\n");

	//if( kICMDecompressionTracking_ReleaseSourceData & decompressionTrackingFlags ) 
	//	dlog( "if we were responsible for managing source data buffers, we should release the source buffer here, using srcFrmRefCon to identify it\n" );
	if( ( kICMDecompressionTracking_EmittingFrame & flag ) && pixelBuffer )
	{
		CVPixelBufferLockBaseAddress( pixelBuffer, 0);
		
		if( state->sync_mode )
		{
			//if( drop_flag )
			if( state->yuv_buffer == NULL )
				err = YUVBuffer_new( &state->yuv_buffer );
			
			if( err == noErr )
			{
				dlog( "calling  YUVBuffer_push\n" );		
				err =  YUVBuffer_push( state->dst_pixel_format, state->yuv_buffer, pixelBuffer, displayTime );
			}
		}
		else
		{
			err = send_out_frame_async( state, pixelBuffer, displayTime );
		}
		
		CVPixelBufferUnlockBaseAddress (pixelBuffer, 0);
		BAIL_IF_ERR(err);
	}
bail:
	;
}
#endif


FskErr qtDecDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data_in, UInt32 dataSize_in, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	kinomaQTDecode		*state			= (kinomaQTDecode *)stateIn;
	QTImageDescription	kinoma_desc		= (QTImageDescription)state->sampleDescription;
	unsigned char		*data			= (unsigned char *)data_in;
	int					data_size		= (int)dataSize_in;
	int					is_startcode	= kinoma_desc == NULL;
	int					eosing			= data == NULL;
	FskInt64			decode_time		= (decodeTime==NULL||eosing) ? 0 : *decodeTime;
	FskInt64		composition_offset	= compositionTimeOffset==NULL ? 0 : (SInt32)*compositionTimeOffset;
	FskInt64		composition_time	= composition_offset + decode_time;
	int					drop_frame		= (0 != (frameType & kFskImageFrameDrop));
	int					immediate_frame	= (0 != (frameType & kFskImageFrameImmediate));
	int					sync_frame		= ( (frameType&0xff) == kFskImageFrameTypeSync )||(immediate_frame);
	FskErr				err				= 0;
	
	if( deco->completionFunction == NULL || deco->completionRefcon == NULL )
		state->sync_mode = 1;

	if( decode_time < 0 && deco->frameNumber == 1 )
		state->decode_time_init = -decode_time;
	
	decode_time		 += state->decode_time_init;
	composition_time += state->decode_time_init;

#ifndef REORDER_CACHED_FRAMES
	if( compositionTime != NULL && decodeTime != NULL && compositionTimeOffset != NULL )
	{
		*compositionTime = *decodeTime + (SInt32)*compositionTimeOffset;
		//*compositionTimeOffset = 0;
	}
#endif	
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into qtDecDecompressFrame\n" );
	dlog( "dataSize_in:        %d\n", (int)dataSize_in );
	dlog( "decode_time:        %d\n", (int)decode_time );
	dlog( "composition_offset: %d\n", (int)composition_offset );
	dlog( "composition_time:   %d\n", (int)composition_time );
	
	if( immediate_frame ) 
		dlog( "immediate_frame: %d\n", immediate_frame );
	if( sync_frame ) 
		dlog( "sync_frame: %d\n", sync_frame );
	if( drop_frame ) 
		dlog( "drop_frame: %d\n", drop_frame );
	if( eosing ) 
		dlog( "drop_frame: %d\n", eosing );
	if( is_startcode ) 
		dlog( "is_startcode:%d\n", is_startcode );
	
	state->debug_input_frame_count++;

	if( !state->sync_mode )
	{
		dlog( "calling func_queue_in, completionFunction: %x, completionRefcon: %x\n", (int)deco->completionFunction,  (int)deco->completionRefcon );
		err = func_queue_in(  state->func_item_list, deco->completionFunction,  deco->completionRefcon, state->debug_input_frame_count,  drop_frame, decode_time );
		BAIL_IF_ERR( err );
		
		deco->completionFunction = NULL;
		deco->completionRefcon = NULL;
	}
	
	if( state->bad_state )
	{
		dlog( "bad_state, goto bail!!!\n");
		goto bail;
	}
    
    if( kinoma_desc == NULL )	//normal case
    {
        dlog( "startcode case not implemented, set bad_state!!!\n");
        state->bad_state = 1;
        err = kFskErrUnimplemented;
        goto bail;
    }
	
	if( state->decompressionSession == NULL ) 
	{	
		//AVCC avcC;
#ifdef USE_VIDEO_TOOLBOX
		CFMutableDictionaryRef extensions = NULL;
		CFMutableDictionaryRef videoDecoderDesc = NULL;
		CFMutableDictionaryRef destImageBufferAttr = NULL;

		if (state->codecType == kCMVideoCodecType_H264)
		{
#else
		Handle									idh =	NULL;
		ImageDescription						*desc;
		ICMDecompressionSessionOptionsRef		sessionOptions = NULL;
		CFMutableDictionaryRef					pixelBufferAttributes = NULL;
		CFNumberRef								number = NULL;
		OSType									pixelFormat = k2vuyPixelFormat;
		ICMDecompressionTrackingCallbackRecord	trackingCallbackRec = { NULL };
		UInt32									descWidth;
#endif
        AVCC avcC;
        unsigned char	*avcc_data = NULL;
		
		state->nalu_len_size = 4;
		
        dlog( "trying to get avcC from desc\n" ); 
        avcc_data = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)kinoma_desc, 'avcC');
        if( avcc_data != NULL )
        {
            dlog( "got avcC from desc and decoding it\n" ); 
            err = DecAVCC( avcc_data, &avcC );
            if( err )
            {
                state->bad_state = 1;
                goto bail;
            }
            
            state->nalu_len_size = avcC.naluLengthSize;
            dlog( "set state->nalu_len_size: %d\n", state->nalu_len_size ); 
        }
		
		//
		state->display_width	= kinoma_desc->width;
		state->display_height	= kinoma_desc->height;

#ifdef USE_VIDEO_TOOLBOX
		if (avcc_data != NULL)
		{
			UInt32 avcc_data_size = FskMisaligned32_GetN(avcc_data) - 8;
			const CFStringRef avcCKey = CFStringCreateWithCString(kCFAllocatorDefault, "avcC", kCFStringEncodingUTF8);
			const CFDataRef avcCValue = CFDataCreate(kCFAllocatorDefault, avcc_data + 8, avcc_data_size);
			CFMutableDictionaryRef atomsDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

			CFDictionaryAddValue(atomsDict, avcCKey, avcCValue);
			CFRelease(avcCKey);
			CFRelease(avcCValue);

			extensions = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			CFDictionaryAddValue(extensions, kCMFormatDescriptionExtension_SampleDescriptionExtensionAtoms, atomsDict);
			CFRelease(atomsDict);
		}
		} else {
			// H.263 / MPEG4
			unsigned char *esds_data = NULL;
			UInt32 esds_data_size = 0;

			dlog( "trying to get esds from desc\n" );
			esds_data = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)kinoma_desc, 'esds');
			if (esds_data == NULL)
			{
				// in case of short video header, aka h263
				if (data == NULL)
				{
					state->bad_state = 1;
					goto bail;
				}

				dlog( "short video header\n" );
				esds_data = data;
				esds_data_size = data_size;

			} else {
				esds_data_size = FskMisaligned32_GetN(esds_data) - 8;
				esds_data += 8;
			}

			//
			state->display_width	= kinoma_desc->width;
			state->display_height	= kinoma_desc->height;
			
			const CFStringRef esdsKey = CFStringCreateWithCString(kCFAllocatorDefault, "esds", kCFStringEncodingUTF8);
			const CFDataRef esdsValue = CFDataCreate(kCFAllocatorDefault, esds_data, esds_data_size);
			CFMutableDictionaryRef atomsDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

			CFDictionaryAddValue(atomsDict, esdsKey, esdsValue);
			CFRelease(esdsKey);
			CFRelease(esdsValue);

			extensions = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			CFDictionaryAddValue(extensions, kCMFormatDescriptionExtension_SampleDescriptionExtensionAtoms, atomsDict);
			CFRelease(atomsDict);
		}

		err = CMVideoFormatDescriptionCreate(kCFAllocatorDefault, state->codecType, kinoma_desc->width, kinoma_desc->height, extensions, &state->videoDesc);
		if (extensions) CFRelease(extensions);
		if (err)
		{
			state->bad_state = 1;
			dlog( "calling CMVideoFormatDescriptionCreate failed, bad_state, set err:kFskErrBadState!!!\n" );
			err = kFskErrBadState;
			goto bail;
		}
		state->callbackRecord.decompressionOutputCallback = decoder_callback;
		state->callbackRecord.decompressionOutputRefCon = state;

		{
			const OSType pixelFormat = kCVPixelFormatType_422YpCbCr8;
			const CFNumberRef pixelFormatValue = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pixelFormat);

			destImageBufferAttr = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			CFDictionaryAddValue(destImageBufferAttr, kCVPixelBufferPixelFormatTypeKey, pixelFormatValue);
			CFRelease(pixelFormatValue);
		}
		err = VTDecompressionSessionCreate(kCFAllocatorDefault, state->videoDesc, videoDecoderDesc, destImageBufferAttr, &state->callbackRecord, &state->decompressionSession);
		if (destImageBufferAttr) CFRelease(destImageBufferAttr);
		if (err)
		{
			state->bad_state = 1;
			dlog( "calling VTDecompressionSessionCreate failed, bad_state, set err:kFskErrBadState!!!\n" );
			err = kFskErrBadState;
			goto bail;
		}
#else
		idh = NewHandleClear( kinoma_desc->idSize );
		if( idh == NULL ) 
			goto bail;
		
		desc = *((ImageDescriptionHandle) (idh));

		dlog( "refit kinoma_desc for idh\n" );
		memcpy( (void *)desc, (void *)kinoma_desc, kinoma_desc->idSize ); 
		{
			int total_bytes = kinoma_desc->idSize;
			unsigned char *d = (unsigned char *)desc;
			int desc_size = sizeof( ImageDescription );
			
			dlog( "total_bytes: %d, desc_size: %d\n",total_bytes, desc_size );
			d += desc_size;
			total_bytes -= desc_size;
			dlog( "after desc, total_bytes: %d\n",total_bytes );
			
			#define SWAP4(d)						\
			{										\
				t0=d[0];t1=d[1];t2=d[2];t3=d[3];	\
				d[0]=t3;d[1]=t2;d[2]=t1;d[3]=t0;	\
			}
			while(total_bytes > 8 )
			{
				int		this_size;
				OSType  this_type;
				int		t0,t1,t2,t3;
				
				this_size    = FskMisaligned32_GetN(d);
				dlog( "got this_size: %d, swap it\n", (int)this_size );
				total_bytes -= this_size;
				if( this_size < 8 || total_bytes < 0 )
				{
					dlog( "this_size is 0, breaking\n" );
					break;
				}
				this_size -= 8;
				
				SWAP4(d)
				d+= 4;
				this_type = FskMisaligned32_GetN(d);
				dlog( "got this_type: %s, , swap it\n", (char *)&this_type );
				SWAP4(d)
				d += 4;
				d += this_size;
			}
		}

		pixelBufferAttributes = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
		descWidth = desc->width;
		number = CFNumberCreate( NULL, kCFNumberIntType, &descWidth );
		CFDictionaryAddValue( pixelBufferAttributes, kCVPixelBufferWidthKey, number );
		CFRelease( number );
		
		number = CFNumberCreate( NULL, kCFNumberIntType, &desc->height );
		CFDictionaryAddValue( pixelBufferAttributes, kCVPixelBufferHeightKey, number );
		CFRelease( number );
		
		number = CFNumberCreate( NULL, kCFNumberSInt32Type, &pixelFormat );
		CFDictionaryAddValue( pixelBufferAttributes, kCVPixelBufferPixelFormatTypeKey, number );
		CFRelease( number );
		
		dlog( "setting decoder_callback\n" );
		trackingCallbackRec.decompressionTrackingCallback =	decoder_callback;
		trackingCallbackRec.decompressionTrackingRefCon =	state;
		
		if(0)
		{
			Boolean yes = true;
			err = ICMDecompressionSessionOptionsCreate(NULL, &sessionOptions);
			BAIL_IF_ERR( err );
			
			err = ICMDecompressionSessionOptionsSetProperty( sessionOptions,
													  kQTPropertyClass_ICMDecompressionSessionOptions,
													  kICMDecompressionSessionOptionsPropertyID_DisplayOrderRequired,
													  sizeof(Boolean),
													  &yes);
			BAIL_IF_ERR( err );
		}
		
		dlog( "calling ICMDecompressionSessionCreate\n" );
		err = ICMDecompressionSessionCreate( NULL, (ImageDescriptionHandle)(idh), sessionOptions, pixelBufferAttributes, &trackingCallbackRec, &state->decompressionSession );
		if( err )
		{
			state->bad_state = 1;
			dlog( "calling ICMDecompressionSessionCreate failed, bad_state, set err:kFskErrBadState!!!\n" );
			err = kFskErrBadState;
			goto bail;
		}
		
		CFRelease( pixelBufferAttributes );
		//ICMDecompressionSessionOptionsRelease( sessionOptions );
#endif
	}

	if( eosing  )
	{
		dlog( "eosing\n" );
		dlog( "calling ICMDecompressionSessionFlush\n");
#ifdef USE_VIDEO_TOOLBOX
		VTDecompressionSessionFinishDelayedFrames(state->decompressionSession);
#else
		err = ICMDecompressionSessionFlush( state->decompressionSession );
#endif
		goto bail;
	}

	if( kinoma_desc && (kinoma_desc->cType == 'avc1') )
	{//for robustness
		int src_size = get_nalu_size( data, state->nalu_len_size );
		if( src_size > data_size )
		{
			dlog( "bad size, src_size: %d, data_size: %d\n", (int)src_size, (int)data_size );
			err = kFskErrBadState;
			goto bail;
		}	
	}

	{
#ifdef USE_VIDEO_TOOLBOX
		CMBlockBufferRef dataBuffer;
		CMSampleBufferRef sampleBuffer;
		CMSampleTimingInfo timingInfo ;
		VTDecodeFrameFlags decodeFlags = 0;

		timingInfo.presentationTimeStamp = CMTimeMake(decode_time, 1000000);
		timingInfo.duration =  CMTimeMake(1, 1000000);
		timingInfo.decodeTimeStamp = kCMTimeInvalid;

		err = CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault, (void *)data_in, dataSize_in, kCFAllocatorNull, NULL, 0, dataSize_in, 0, &dataBuffer);
		BAIL_IF_ERR(err);

		err = CMSampleBufferCreateReady(kCFAllocatorDefault, dataBuffer, state->videoDesc, 1, 1, &timingInfo, 0, NULL, &sampleBuffer);
		CFRelease(dataBuffer);
		BAIL_IF_ERR(err);

		decodeFlags |= kVTDecodeFrame_1xRealTimePlayback;
//		if (!state->sync_mode)
//			decodeFlags |= kVTDecodeFrame_EnableAsynchronousDecompression;

		err = VTDecompressionSessionDecodeFrame(state->decompressionSession, sampleBuffer, decodeFlags, (void *)(uintptr_t)deco->frameNumber, NULL);
		CFRelease(sampleBuffer);
		BAIL_IF_ERR(err);
#else
		ICMFrameTimeRecord	frameTime = {{0}};
		ICMFrameTimeRecord  *frame_time = &frameTime;
		TimeValue time = (TimeValue)composition_time;
		*(TimeValue64 *)&frameTime.value = composition_time;
		frameTime.scale			= state->timeScale;
		frameTime.decodeTime    = decode_time;
		frameTime.rate			= fixed1;
		frameTime.recordSize	= sizeof(ICMFrameTimeRecord);
		frameTime.frameNumber	= deco->frameNumber;
		frameTime.flags			= icmFrameTimeHasDecodeTime;
		
		if( kinoma_desc->cType == 'avc1' )
			frameTime.flags |=  icmFrameTimeIsNonScheduledDisplayTime;
		
		dlog( "calling ICMDecompressionSessionDecodeFrame, frame refCon:\n" );
		dlog( "=====>\n");
		dlog( "=====>flag:           %x\n",  (int)frameTime.flags );
		dlog( "=====>srcFrmRefCon:   %d\n",	(int)deco->frameNumber );
		dlog( "=====>displayTime:    %d\n",	(int)composition_time );
		dlog( "=====>decodeTime:     %d\n",	(int)decode_time );
		dlog( "=====>\n");
		err = ICMDecompressionSessionDecodeFrame( state->decompressionSession, (UInt8 *)data_in, dataSize_in, NULL, frame_time, (void *)deco->frameNumber );
		BAIL_IF_ERR( err );

		dlog( "calling ICMDecompressionSessionSetNonScheduledDisplayTime: Pull decoded frame out\n" );
		ICMDecompressionSessionSetNonScheduledDisplayTime( state->decompressionSession, time, state->timeScale, 0 );
#endif
	}
	
	if( state->sync_mode )
	{
		err = send_out_frame_sync(  state, deco );
		BAIL_IF_ERR( err );
	}
	
bail:	
		
	if( (eosing || state->bad_state ) && state->func_item_list != NULL )
	{
		FskErr flush_err = kFskErrShutdown;//dec->error_happened ? kFskErrBadData: kFskErrShutdown;
		dlog( "eosing or bad_state, flushing completion fucntions!!!\n");
		
		if( !state->sync_mode )
		{
			dlog( "calling func_queue_flush()\n");
			func_queue_flush(deco, state->func_item_list, flush_err );
		}
		
		if( state->yuv_buffer != NULL )
		{
			//***bnie: if eosing, we'd better output existing yuv frames
			YUVBuffer_reset( state->yuv_buffer );
		}
	}
	
	dlog( "out of qtDecDecompressFrame: err: %d, in/out: %d/ %d\n", (int)err, state->debug_input_frame_count, state->debug_output_frame_count );
	return err;
}


FskErr qtDecFlush(void *stateIn, FskImageDecompress deco )
{
	kinomaQTDecode		*state	= (kinomaQTDecode *)stateIn;
	FskErr				err		= kFskErrNone;
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into qtDecFlush\n");

	if( state->bad_state )
	{
		dlog( "decoder is not properly initialized!\n");
		return kFskErrBadState;
	}
	
	dlog( "calling ICMDecompressionSessionFlush\n");
#ifdef USE_VIDEO_TOOLBOX
	VTDecompressionSessionFinishDelayedFrames(state->decompressionSession);
#else
	err = ICMDecompressionSessionFlush( state->decompressionSession );
#endif
	if( state->func_item_list != NULL )
	{
		FskErr flush_err = kFskErrShutdown;//dec->error_happened ? kFskErrBadData: kFskErrShutdown;
		dlog( "flushing completion fucntions!!!\n");
	
		if( !state->sync_mode )
		{		
			dlog( "calling func_queue_flush()\n");
			func_queue_flush(deco, state->func_item_list, flush_err );
		}
		
		if( state->yuv_buffer != NULL )
		{
			YUVBuffer_reset( state->yuv_buffer );
		}
	}
	
	dlog( "out of qtDecFlush: err: %d\n", (int)err );
	
	return err;
}


FskErr qtDecGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	kinomaQTDecode *state  = (kinomaQTDecode *)stateIn;
	unsigned char *data     = (unsigned char *)deco->data;
	int			  data_size = (int)deco->dataSize;
	int			  nalu_type;
	int			  ref_idc;
	int			  block_size;
	UInt32		  frame_type = 0;
	int			  is_startcode = 1;
	FskErr		  err = kFskErrNone;

	dlog( "\n###########################################################################################\n" ); 
	dlog( "into qtDecGetMetaData, indec: %d\n", (int)index );

	if( index == 9 )
		return kFskErrUnimplemented;

	if( state->bad_state )
	{
		dlog( "decoder is not properly initialized, bad_state!\n");
		return kFskErrBadState;
	}
	
	if (kFskImageDecompressMetaDataFrameType != metadata)
	{
		err = kFskErrUnimplemented;
		goto bail;
	}

	if( index == 0 )
		is_startcode = 1;
	else if( index == 1 )
		is_startcode = 0;
	
	err = check_next_frame_nalu( is_startcode, state->nalu_len_size, &data, &data_size, &nalu_type, &ref_idc, &block_size );
	if( err != 0 )
	{
		err = kFskErrBadData;
		goto bail;
	}

	if( nalu_type == 5 )
		frame_type = kFskImageFrameTypeSync;
	else if( ref_idc != 0 )
		frame_type = kFskImageFrameTypeDifference;
	else
		frame_type = kFskImageFrameTypeDroppable | kFskImageFrameTypeDifference;

	{
		//int sync		= frame_type == kFskImageFrameTypeSync;
		//int droppable   = (frame_type&kFskImageFrameTypeDroppable)  != 0;
		//int difference  = (frame_type&kFskImageFrameTypeDifference) != 0;
	
		//dlog( "sync:%d, drop:%d, diff:%d \n", sync, droppable, difference );
	}

	if( value != NULL )
	{
		value->type = kFskMediaPropertyTypeInteger;
		value->value.integer = frame_type;
	}

bail:	
	dlog( "out of qtDecGetMetaData: err: %d\n", (int)err );
	return err;
}


FskErr qtDecSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaQTDecode *state = (kinomaQTDecode *)stateIn;

	if( state->bad_state )
	{
		dlog( "decoder is not properly initialized, bad_state!\n");
		return kFskErrBadState;
	}
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into qtDecSetSampleDescription\n");

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

FskErr qtDecSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property )
{
	kinomaQTDecode *state = (kinomaQTDecode *)stateIn;
	FskBitmapFormatEnum prefered_yuvFormat = kFskBitmapFormatUnknown;
	UInt32 i,count = property->value.integers.count;
	//UInt32 propertyType = property->type;

	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatUYVY)
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420)
	
	if( prefered_yuvFormat != kFskBitmapFormatUnknown )
		state->dst_pixel_format = prefered_yuvFormat;
	else
		state->dst_pixel_format = kFskBitmapFormatYUV420;
	
	return kFskErrNone;
}

FskErr qtDecodeGetMaxFramesToQueue (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaQTDecode *state = (kinomaQTDecode *)stateIn;
	FskErr				err	 = kFskErrNone;
	
	dlog( "\n###########################################################################################" ); 
	dlog( "into qtDecodeGetMaxFramesToQueue, propertyID: %d", (int)propertyID );
	
	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!");
		return state->bad_state;
	}
	
	property->value.integer	= 10;
	property->type			= kFskMediaPropertyTypeInteger;
	
	//bail:	
	return err;
}
