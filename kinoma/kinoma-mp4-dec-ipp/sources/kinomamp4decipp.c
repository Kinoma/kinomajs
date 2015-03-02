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

#include "kinomamp4decipp.h"
#include "FskYUV420Copy.h"
#include "kinoma_utilities.h"
#include "FskEndian.h"
#include "FskBitmap.h"
#include "QTReader.h"

#include "codecDef.h"
#include "codecVC.h"
#include "misc.h"


#if SUPPORT_INSTRUMENTATION
FskInstrumentedTypeRecord gkinomamp4decippTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "kinomamp4decipp"};
//FskInstrumentedSimpleType(kinomamp4decipp, kinomamp4decipp);
#endif

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


FskMediaPropertyEntryRecord mp4DecodeProperties[] = 
{
	{kFskMediaPropertySampleDescription,		kFskMediaPropertyTypeData,		NULL,		mp4DecodeSetSampleDescription},
 	{kFskMediaPropertyFormatInfo,				kFskMediaPropertyTypeData,		NULL,		mp4DecodeSetSampleDescription},
	{kFskMediaPropertyRotation,					kFskMediaPropertyTypeFloat,		NULL,		mp4DecodeSetRotation},
	{kFskMediaPropertyPixelFormat,				kFskMediaPropertyTypeUInt32List,NULL,		mp4DecodeSetPreferredPixelFormat},
	{kFskMediaPropertyMaxFramesToQueue,			kFskMediaPropertyTypeInteger,	mp4DecodeGetMaxFramesToQueue, NULL},
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
	
	UInt32				dst_pixel_format;
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
    //int							dec_param_bOutputDelayDisable;
	//int							dec_param_nFdbMode;
	IppSkipMode					dec_param_SkipMode;
	int							dec_param_bCustomFrameMalloc;
	int							dec_param_bRawDecDelayPar;
    
	MiscGeneralCallbackTable    dec_param_SrcCBTable;
	IppBitstream                srcBitStream;
	int							bUsed;
	int							output_frame_count;
	
	int							is_shorthead;
	void                        *mp4_dec;
	
	
} kinomaMP4Decode;




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


static FskErr send_out_one_pic( kinomaMP4Decode *state, IppPicture *pSrcPicture )
{
	FskImageDecompress		deco	  = state->deco;
	FskBitmapFormatEnum					pixelFormat;
	unsigned char			*dstPtr		= NULL;
	long					dst_y_rb;
	long					rot_width, rot_height;
	long					src_width	= pSrcPicture->picWidth;	//***src_width, src_height really should be named as clip_width, clip_height
	long					src_height	= pSrcPicture->picHeight;
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

	dlog( "into send_out_frame\n" );
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
		
		dlog("retrieved completion function: %x, refcon: %x, frame_number: %d, drop_flag: %d\n", (int)completionFunction, (int)completionRefcon, (int)frame_number, drop_flag );				
		
		
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
	
	
	//dlog( "into send_out_frame\n" );

	//src_y += src_y_rb *  pSrcPicture->picROI.y		 +  pSrcPicture->picROI.x;
	//src_u += src_u_rb * (pSrcPicture->picROI.y >> 1) + (pSrcPicture->picROI.x >> 1);
	//src_v += src_v_rb * (pSrcPicture->picROI.y >> 1) + (pSrcPicture->picROI.x >> 1);
	
	
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



FskErr mp4DecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into mp4DecodeCanHandle: format: %d, mime: %s\n", (int)format, mime ); 
    
	*canHandle = ('mp4v' == format) || (0 == FskStrCompare(mime, "x-video-codec/mp4"));
	
	return kFskErrNone;
}

FskErr mp4DecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension)
{
	kinomaMP4Decode *state;
	FskErr err;
	
	err = FskMemPtrNewClear(sizeof(kinomaMP4Decode), (FskMemPtr *)&state);
	BAIL_IF_ERR( err ); 
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "in mp4DecodeNew allocated state: %x\n", (int)state ); 
	
	err = FskListMutexNew(&state->func_item_list, "FuncItemList");
	BAIL_IF_ERR( err ); 
	
	state->bad_state		= 0;
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
		mp4DecodeDispose(state, deco);
	
	deco->state = state;
	
	dlog( "out of mp4DecodeNew: err: %d\n", (int)err );
	
	return err;
}

FskErr mp4DecodeDispose(void *stateIn, FskImageDecompress deco)
{
	kinomaMP4Decode *state = (kinomaMP4Decode *)stateIn;
	int i;
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into mp4DecodeDispose\n" ); 
	
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
		
		if( state->mp4_dec != NULL )
			DecoderFree_MPEG4Video(&state->mp4_dec);

		videoFreeBuffer(&state->srcBitStream);
		
		FskMemPtrDispose(state);
	}
	
	return kFskErrNone;
}


void debug_check_stream( char *message, IppBitstream *s )
{
	unsigned char *d = (unsigned char *)s->pBsCurByte;
	
	dlog("\n\n\n");
	dlog("%s\n", message);
	dlog("pBsBuffer:     %x\n", (int)s->pBsBuffer);
	dlog("pBsCurByte:    %x\n", (int)s->pBsCurByte);
	dlog("bsByteLen:     %d\n", (int)s->bsByteLen);
	dlog("bsCurBitOffset:%d\n", (int)s->bsCurBitOffset);
	
	dlog("@@@@@@@@@@@@@from top: %d\n", (int)(s->pBsCurByte - s->pBsBuffer) );
	d += 0;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n", d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	d += 8;    dlog( "%2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",	d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7] );
	dlog("\n\n\n");
}



FskErr mp4DecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data_in, UInt32 dataSize_in, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	kinomaMP4Decode		*state			= (kinomaMP4Decode *)stateIn;
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

	IppPosition			NextSyncCode;
	int					data_copied		= 0;
	int					bLastOutputFrame = is_eos;

	FskErr				err				= 0;
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into mp4DecodeDecompressFrame, dataSize: %d, decode_time: %d, is_startcode: %d, drop_frame: %d, immediate_frame: %d, sync_frame: %d, is_eos: %d\n", 
			(int)data_size, (int)decode_time, (int)is_startcode, (int)drop_frame, (int)immediate_frame, (int)sync_frame, (int)is_eos ); 
	
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
	
	if( state->mp4_dec == NULL ) 
	{	
		unsigned char	*esds		 = NULL;
		int				esds_size	 = 0;
		
		state->is_shorthead = 0;
		state->width_clip	= 0;
		state->height_clip	= 0;
		
		if( desc != NULL )
		{
			dlog( "trying to get esds from desc, width/height: %d, %d\n", desc->width, desc->height ); 
			esds = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)desc, 'esds');
			if( esds != NULL )
			{
				state->width_clip  = desc->width;
				state->height_clip = desc->height;
			}
		}
		
		state->dec_param_NSCCheckDisable	    = 1;
		//state->dec_param_bOutputDelayDisable    = 0;					//default value
		//state->dec_param_nFdbMode			    = 0;					//***bnie: not used???
		state->dec_param_SkipMode			    = IPPVC_SKIPMODE_0;
		state->dec_param_bCustomFrameMalloc     = 1;					//default value
		state->dec_param_bRawDecDelayPar	    = 0;
		
		state->dec_param_SrcCBTable.fMemCalloc	= IPP_MemCalloc;
		state->dec_param_SrcCBTable.fMemMalloc	= IPP_MemMalloc;
		state->dec_param_SrcCBTable.fMemFree	= IPP_MemFree;
		state->output_frame_count				= 0;
		state->bUsed							= 1;
		
		err = videoInitBuffer(&state->srcBitStream);
		if (IPP_STATUS_NOERR != err ) 
		{
			dlog( "error: no memory!\n");
			BAIL( IPP_FAIL );
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
			
			state->is_shorthead = 0;
			state->srcBitStream.bsByteLen      = esds_size + dataSize_in + 4;
			state->srcBitStream.bsCurBitOffset = 0;
			state->srcBitStream.pBsCurByte     = state->srcBitStream.pBsBuffer;
			IPP_Memcpy( (void *)state->srcBitStream.pBsBuffer, (void *)esds, esds_size );
			IPP_Memcpy( (void *)(state->srcBitStream.pBsBuffer+esds_size), (void *)data_in, dataSize_in );
			//fill SC to trick SC seek
			state->srcBitStream.pBsBuffer[state->srcBitStream.bsByteLen - 4 ] = 0x00;
			state->srcBitStream.pBsBuffer[state->srcBitStream.bsByteLen - 3 ] = 0x00;
			state->srcBitStream.pBsBuffer[state->srcBitStream.bsByteLen - 2 ] = 0x01;
			state->srcBitStream.pBsBuffer[state->srcBitStream.bsByteLen - 1 ] = 0xb6;
			data_copied = 1;
			
			debug_check_stream( "before DecoderInitAlloc_MPEG4Video", &state->srcBitStream );
			
			dlog("calling DecoderInitAlloc_MPEG4Video()\n");
			err = DecoderInitAlloc_MPEG4Video(&state->srcBitStream, &state->width_clip, &state->height_clip, &state->dec_param_SrcCBTable, &state->mp4_dec);
			dlog( "returned: err: %d, width_clip/height_clip: %d, %d\n", (int)err, state->width_clip, state->height_clip ); 

			debug_check_stream( "after DecoderInitAlloc_MPEG4Video", &state->srcBitStream );
			
			if( err != IPP_STATUS_NOERR && err != IPP_STATUS_MP4_SHORTHEAD )
			{
				dlog( "error: decoder init fail, error code %d!\n", (int)err);
                BAIL( IPP_FAIL );
			}
			
			dlog("calling DecodeSendCmd_MPEG4Video:IPPVC_SET_NSCCHECKDISABLE\n");
			err = DecodeSendCmd_MPEG4Video (IPPVC_SET_NSCCHECKDISABLE, &state->dec_param_NSCCheckDisable, NULL, state->mp4_dec);
			dlog("returned err: %d\n", (int)err);
			if ( err != IPP_STATUS_NOERR ) 
			{
				dlog("Error: DecodeSendCmd_MPEG4Video \n");
                BAIL( IPP_FAIL );
			}
			
			//FrameSkip Mode: [IPPVC_SET_SKIPMODE]
			//Drop B: [0-4] 0 == [0%]  1 == [25%]  2 == [50%]  3 == [75%]  4 == [100%]
			dlog("calling DecodeSendCmd_MPEG4Video:IPPVC_SET_SKIPMODE\n");
			err = DecodeSendCmd_MPEG4Video (IPPVC_SET_SKIPMODE, (&state->dec_param_SkipMode), NULL, state->mp4_dec);
			dlog("returned err: %d\n", (int)err);
			if ( err != IPP_STATUS_NOERR ) 
			{
				dlog("Error: DecodeSendCmd_MPEG4Video \n");
				err = IPP_FAIL;
				goto  bail;
			}
		}
		else
		{	//in case of short video header, aka h263 
			state->width_clip  = 352;
			state->height_clip = 288;			
			//state->mode	  = H263_MODE;
			dlog( "no desc or esda, must be h.263, set default width/height: %d, %d\n", state->width_clip, state->height_clip ); 
			
			state->is_shorthead = 1;
			state->srcBitStream.bsByteLen      = dataSize_in;
			state->srcBitStream.bsCurBitOffset = 0;
			state->srcBitStream.pBsCurByte     = state->srcBitStream.pBsBuffer;
			{
				unsigned char *d = (unsigned char *)data_in;
				dlog( "copy vol dataSize_in: %d => %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n",  (int)dataSize_in, d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],d[8],d[9] );
			}
			
			IPP_Memcpy( (void *)state->srcBitStream.pBsBuffer, (void *)data_in, dataSize_in );
			data_copied = 1;
			
			err = DecoderInitAlloc_H263Video(&state->srcBitStream, &state->dec_param_SrcCBTable, &state->mp4_dec/*,u8Flv*/);
			if ( err != IPP_STATUS_NOERR) 
			{
				//IPP_Free the allocated memory 
				dlog("Error: DecoderInitAlloc_H263Video \n");
                BAIL( IPP_FAIL );
			}
			
			err = DecodeSendCmd_H263Video (IPPVC_SET_NSCCHECKDISABLE, &state->dec_param_NSCCheckDisable, NULL, state->mp4_dec);
			if ( err != IPP_STATUS_NOERR ) 
			{
				dlog("Error: DecodeSendCmd_H263Video \n");
				err = IPP_FAIL;
				goto  bail;
			}
		}
	}

	state->current_decode_time = decode_time;
	dlog( "calling func_queue_in, completionFunction: %x, completionRefcon: %x\n", (int)deco->completionFunction,  (int)deco->completionRefcon );
	err = func_queue_in(  state->func_item_list, deco->completionFunction,  deco->completionRefcon, state->debug_input_frame_count,  drop_frame, decode_time );
	BAIL_IF_ERR( err );

	deco->completionFunction = NULL;
	deco->completionRefcon = NULL;
	
	dlog( "start decoding!\n");
	NextSyncCode.ptr    = NULL;
	NextSyncCode.bitoff = 0;
	
	while(1) 
	{
		IppPicture	Pic;	
		
		if( !data_copied ) 
		{
			state->srcBitStream.bsByteLen      = dataSize_in+4;
			state->srcBitStream.bsCurBitOffset = 0;
			state->srcBitStream.pBsCurByte     = state->srcBitStream.pBsBuffer;
			dlog( "copy data, size: %d => %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n",  (int)dataSize_in, data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9] );
			IPP_Memcpy( (void *)state->srcBitStream.pBsBuffer, (void *)data, dataSize_in );
			//fill SC to trick SC seek
			state->srcBitStream.pBsBuffer[state->srcBitStream.bsByteLen - 4 ] = 0x00;
			state->srcBitStream.pBsBuffer[state->srcBitStream.bsByteLen - 3 ] = 0x00;
			state->srcBitStream.pBsBuffer[state->srcBitStream.bsByteLen - 2 ] = 0x01;
			state->srcBitStream.pBsBuffer[state->srcBitStream.bsByteLen - 1 ] = 0xb6;
			data_copied = 1;
		}

		debug_check_stream( "before seek next sync code", &state->srcBitStream );
		
		dlog("NextSyncCode.ptr:   %x\n", (int)NextSyncCode.ptr);
		dlog("NextSyncCode.bitoff:%d\n", (int)NextSyncCode.bitoff);
		
		if( state->is_shorthead )
		{
			dlog("calling DecodeSendCmd_H263Video:IPPVC_SEEK_NEXTSYNCCODE\n");
			err = DecodeSendCmd_H263Video (IPPVC_SEEK_NEXTSYNCCODE, &state->srcBitStream, &NextSyncCode, state->mp4_dec);
		}
		else
		{
			dlog("calling DecodeSendCmd_MPEG4Video:IPPVC_SEEK_NEXTSYNCCODE\n");
			err = DecodeSendCmd_MPEG4Video(IPPVC_SEEK_NEXTSYNCCODE, &state->srcBitStream, &NextSyncCode, state->mp4_dec);
		}

		debug_check_stream( "after seek next sync code", &state->srcBitStream );
		
		dlog("NextSyncCode.ptr:   %x\n", (int)NextSyncCode.ptr);
		dlog("NextSyncCode.bitoff:%d\n", (int)NextSyncCode.bitoff);
		
		dlog("returned err: %d\n", (int)err);
		if((err != IPP_STATUS_NOERR) && (err != IPP_STATUS_SYNCNOTFOUND_ERR)) 
		{
			dlog("Error: DecodeSendCmd_H263Video \n");
			err = IPP_FAIL;
			BAIL_IF_ERR(err);
		}
		
		if (IPP_STATUS_SYNCNOTFOUND_ERR == err )
		{
			dlog("IPP_STATUS_SYNCNOTFOUND_ERR, continue...\n");
			break;
		}
		
		if( state->is_shorthead ) 
		{
			dlog("calling Decode_H263Video()\n");
			err = Decode_H263Video(&state->srcBitStream, &Pic, state->mp4_dec);
		}
		else
		{
			dlog("calling Decode_MPEG4Video()\n");
			err = Decode_MPEG4Video(&state->srcBitStream, &Pic, state->mp4_dec, bLastOutputFrame);
		}
		
		dlog("returned err: %d\n", (int)err);
		if( err == IPP_STATUS_NOERR) 
		{
			dlog("calling send_out_one_pic()\n");
			err = send_out_one_pic( state, &Pic );
			break;
		} 
	} 
		
bail:
	//if( is_eos && (err==0 ))
	//	output_all_pictures( state, state->pDstPicList );

	if( (is_eos || state->bad_state || (err == kFskErrBadState) ) && state->func_item_list != NULL )
	{
		FskErr flush_err = kFskErrShutdown;//dec->error_happened ? kFskErrBadData: kFskErrShutdown;
		dlog("bad_state, flushing completion fucntions!!!\n");
		
		dlog("calling func_queue_flush()\n");
		func_queue_flush(deco, state->func_item_list, flush_err );
	}
	
	dlog( "out of mp4DecodeDecompressFrame: err: %d, in/out: %d/ %d\n", (int)err, state->debug_input_frame_count, state->debug_output_frame_count );
	return err;
}


FskErr mp4DecodeFlush(void *stateIn, FskImageDecompress deco )
{
	kinomaMP4Decode		*state	= (kinomaMP4Decode *)stateIn;
	FskErr				err		= kFskErrNone;
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into mp4DecodeFlush\n");

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
	
	
	state->current_decode_time = 0;
	
	state->debug_input_frame_count  = 0;
	state->debug_output_frame_count	= 0;
	
//bail:
	dlog( "out of mp4DecodeFlush: err: %d\n", (int)err );
	return err;
}

#include <dlfcn.h>

FskErr mp4DecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	kinomaMP4Decode *state  = (kinomaMP4Decode *)stateIn;
	//unsigned char *data     = (unsigned char *)deco->data;
	//int			  data_size = (int)deco->dataSize;
	//int			  nalu_type;
	//int			  ref_idc;
	//int			  block_size;
	UInt32		  frame_type = kFskImageFrameTypeSync;
	int			  is_startcode = 1;
	FskErr		  err = kFskErrNone;

	dlog( "\n###########################################################################################\n" ); 
	dlog( "into mp4DecodeGetMetaData, indec: %d\n", (int)index );

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
	
	
	//***bnie: implemente these!!!
	//if( nalu_type == 5 )
	//	frame_type = kFskImageFrameTypeSync;
	//else if( ref_idc != 0 )
	//	frame_type = kFskImageFrameTypeDifference;
	//else
	//	frame_type = kFskImageFrameTypeDroppable | kFskImageFrameTypeDifference;

	{
		int sync		= frame_type == kFskImageFrameTypeSync;
		int droppable   = (frame_type&kFskImageFrameTypeDroppable)  != 0;
		int difference  = (frame_type&kFskImageFrameTypeDifference) != 0;
	
		dlog( "sync:%d, drop:%d, diff:%d \n", sync, droppable, difference );
	}

	if( value != NULL )
	{
		value->type = kFskMediaPropertyTypeInteger;
		value->value.integer = frame_type;
	}

bail:	
	dlog( "out of mp4DecodeGetMetaData: err: %d\n", (int)err );
	return err;
}


FskErr mp4DecodeSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaMP4Decode *state = (kinomaMP4Decode *)stateIn;

	if( state->bad_state )
	{
		dlog("decoder is not properly initialized!\n");
		return kFskErrBadState;
	}
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into mp4DecodeSetSampleDescription\n");

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

FskErr mp4DecodeSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property )
{
	kinomaMP4Decode *state = (kinomaMP4Decode *)stateIn;
	FskBitmapFormatEnum prefered_yuvFormat = kFskBitmapFormatUnknown;
	UInt32 count = property->value.integers.count;
	UInt32 propertyType = property->type;

	dlog( "\n###########################################################################################\n" );
	dlog( "into mp4DecodeSetPreferredPixelFormat, propertyID: %d, propertyType: %d, count: %d\n", (int)propertyID, (int)property->type, (int)count);
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


FskErr mp4DecodeGetMaxFramesToQueue (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaMP4Decode *state = (kinomaMP4Decode *)stateIn;
	FskErr				err	 = kFskErrNone;
	
	dlog( "\n###########################################################################################" ); 
	dlog( "into mp4DecodeGetMaxFramesToQueue, propertyID: %d", (int)propertyID );
	
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


FskErr mp4DecodeSetRotation(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaMP4Decode *state = (kinomaMP4Decode *)stateIn;

	dlog( "\n###########################################################################################\n" ); 
	dlog( "into mp4DecodeSetSampleDescription\n");
	
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
