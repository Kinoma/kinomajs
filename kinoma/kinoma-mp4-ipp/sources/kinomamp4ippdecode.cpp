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
#define __FSKMPEGDECODE_PRIV__

#include "QTReader.h"
#include "FskEndian.h"

#include "kinoma_ipp_lib.h"
#include "kinoma_performance.h"

#include "kinoma_utilities.h"

#include "kinomamp4ippdecode.h"
#include "umc_video_data.h"
#include "umc_mpeg4_video_decoder.h"	
#include "FskYUV420Copy.h"


#define SUPPORT_ROTATION

#if SRC_YUV420i
	#define YUV420_FORMAT	kFskBitmapFormatYUV420i
#else
	#define YUV420_FORMAT	kFskBitmapFormatYUV420
#endif

using namespace UMC;


#if SUPPORT_INSTRUMENTATION
FskInstrumentedTypeRecord gkinomamp4ippTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "kinomamp4ipp"};
//FskInstrumentedSimpleType(kinomamp4ipp, kinomamp4ipp);
#endif


#define DO_DEBLOCKING
//#define LOSSY_SPEEDUP
//#define DUMP_YUV
//#define DUMP_BOX

#ifdef DUMP_YUV
#define Debug_DumpYUV( a, b, c, d, e, f, g )	dump_yuv( a, b, c, d, e, f, g )
#else
#define Debug_DumpYUV( a, b, c, d, e, f, g )
#endif

#ifdef DUMP_BOX
#define Debug_DumpBox( a, b )	dump_bitstream_box( a, b )
#else
#define Debug_DumpBox( a, b )
#endif

extern unsigned char mp4v_to_h264_qp_lut[52];

const int kBitmapCacheSize = 50;

static FskErr mp4DecodeSetSampleNumber(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4DecodeSetSampleDescription(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4DecodeSetQuality(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4DecodeSetPlayMode(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4DecodeGetPerformanceInfo (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4DecodeSetRotation(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4DecodeSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property );

FskMediaPropertyEntryRecord mp4DecodeProperties[] = {
	{ kFskMediaPropertySampleNumber,		kFskMediaPropertyTypeInteger,	NULL, mp4DecodeSetSampleNumber},
	{ kFskMediaPropertySampleDescription,	kFskMediaPropertyTypeData,		NULL, mp4DecodeSetSampleDescription},
	{ kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,		NULL, mp4DecodeSetSampleDescription},
	{ kFskMediaPropertyQuality,				kFskMediaPropertyTypeFloat,		NULL, mp4DecodeSetQuality},
	{ kFskMediaPropertyPlayMode,			kFskMediaPropertyTypeInteger,	NULL, mp4DecodeSetPlayMode},
	{ kFskMediaPropertyPerformanceInfo,		kFskMediaPropertyTypeFloat,		mp4DecodeGetPerformanceInfo,	NULL},
	{ kFskMediaPropertyRotation,			kFskMediaPropertyTypeFloat,		NULL, mp4DecodeSetRotation},
	{ kFskMediaPropertyPixelFormat,			kFskMediaPropertyTypeUInt32List,NULL, mp4DecodeSetPreferredPixelFormat},
	{ kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined, NULL, NULL}
};


typedef struct 
{
	MPEG4VideoDecoder	*dec;
	UInt32				sampleNumber;
	void				*desc;
	UInt32				descSeed;

	FskBitmap			bitmaps[kBitmapCacheSize];

	float				quality;
	int					approx_level;
	int					play_mode;
	int					do_postprocessing;
	
	unsigned char		*tmp_y;
	unsigned char		*tmp_u;
	unsigned char		*tmp_v;

	FskBitmapFormatEnum	dst_pixel_format;
	float				rotation_float;
	int 				rotation;

	PerformanceInfo		perf;

} FskMPEGVideoDecoderRecord, *FskMPEGVideoDecoder;

FskErr mp4DecodeSetSampleNumber(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskErr mp4DecodeSetSampleDescription(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskErr mp4DecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType);

#if FSK_APPLICATION_FREEPLAY
#ifndef SUPPORT_H263_ONLY	
#define SUPPORT_H263_ONLY
#endif
#endif

FskErr mp4DecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into mp4DecodeCanHandle, format: %d, mime: %s\n", (int)format, mime);
	
	*canHandle = ('h263' == format || 's263' == format)						||
				 (0 == FskStrCompare(mime, "x-video-codec/263")	)			||
				 (0 == FskStrCompare(mime, "x-video-codec/h263-flash") )
#ifndef SUPPORT_H263_ONLY
																			|| 
				 ('mp4v' == format)											||	  
				 (0 == FskStrCompare(mime, "x-video-codec/mp4"))
#endif
				 ;

	return kFskErrNone;
}

FskErr mp4DecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension)
{
	FskErr err;
	FskMPEGVideoDecoder state;

	err = FskMemPtrNewClear(sizeof(FskMPEGVideoDecoderRecord), (FskMemPtr*)&state);
	if (err) goto bail;

	state->quality	= 1.0;		//best quality by default
	state->approx_level = 0;	//best quality by default
	state->play_mode = 0;	//best quality by default
	state->do_postprocessing = 0;
	state->tmp_y = 0;
	state->tmp_u = 0;
	state->tmp_v = 0;
	state->rotation_float	= 0;
	state->rotation			= kRotationNone;

	state->dst_pixel_format = YUV420_FORMAT;
	
	deco->state = state;
	
bail:
	return err;
}

FskErr mp4DecodeDispose(void *stateIn, FskImageDecompress deco)
{
	FskMPEGVideoDecoder state = (FskMPEGVideoDecoder)stateIn;
	int i;

	if (state) 
	{
		if (state->dec != NULL )
			delete state->dec;

		for (i = 0; i < kBitmapCacheSize; i++)
			FskBitmapDispose(state->bitmaps[i]);

		FskMemPtrDispose(state->desc);

		if (state->tmp_y != NULL )
			FskMemPtrDispose(state->tmp_y);

		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}


static int q_a_ary[11] = 
{	
	MP4V_LOSSY_SETTING_2, 
	MP4V_LOSSY_SETTING_2, 
	MP4V_LOSSY_SETTING_2, 
	MP4V_LOSSY_SETTING_2, 
	MP4V_LOSSY_SETTING_2,
	MP4V_LOSSY_SETTING_1, 
	MP4V_LOSSY_SETTING_1, 
	MP4V_LOSSY_SETTING_1, 
	MP4V_LOSSY_SETTING_1, 
	MP4V_LOSSY_SETTING_1,  
	MP4V_LOSSY_SETTING_0
};

static int quility_to_approx( float quality )
{
	int	q = (int)(quality*10);
	int	a;
	if( q > 10 ) q = 10;
	if( q < 0  ) q = 0;
	a = q_a_ary[q];
	
	return a;
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


FskErr mp4DecodeGetDimension(const void *esds, UInt32 esds_size, UInt32 *width, UInt32 *height )
{
	VideoDecoderParams_V51	decParam;
	MediaData_V51			pData;
	FskErr					err	= kFskErrNone;
	MPEG4VideoDecoder		*dec  = new MPEG4VideoDecoder();

	kinoma_ipp_lib_mp4v_init(FSK_ARCH_AUTO);


	pData.SetBufferPointer( (vm_byte *)esds, (size_t)esds_size);
	pData.SetDataSize( (size_t)esds_size);

	decParam.m_pData0				= &pData;
	decParam.info.framerate			= 1;
	decParam.info.duration			= 1;
	//decParam.info.codecPriv			= NULL;
	decParam.info.stream_subtype	= MPEG4_VIDEO_QTIME;
#ifndef DROP_COLOR_CONVERSION
	decParam.lpConverter			= NULL;
	decParam.lpConvertInit			= NULL;
	decParam.lpConvertInitPreview	= NULL;
#endif
	//***
	decParam.uiLimitThreads			= 1;
//	decParam.lFlags					= FLAG_VDEC_REORDER;
	decParam.lFlags					= 0;

	dec->Init(&decParam);
	dec->GetDimensions( (int *)width, (int *)height );

	delete dec;

	return err;
}


FskErr mp4DecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	FskMPEGVideoDecoder state	= (FskMPEGVideoDecoder)stateIn;
	QTImageDescription	desc	= (QTImageDescription)state->desc;
	FskErr				err		= kFskErrNone;
	YUVPlannar			yuvPlannar;

	dlog( "\n###########################################################################################\n" ); 
	dlog( "into mp4DecodeDecompressFrame, data: %ld, dataSize: %d\n", (long)data, (int)dataSize);
	
	if ( state->dec == NULL ) 
	{
		unsigned char		*esds	 = NULL;
		long				esdsSize = 0;

		if( desc != NULL )
			esds = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)desc, 'esds');
		
		if( esds == NULL )
		{//in case of short video header, aka h263   --bnie 11/10/06
			if( data != NULL )
			{
				esdsSize = dataSize;
				esds = (unsigned char *)data;
			}
			else
			{
				BAIL( kFskErrBadData );
			}
				
		}
		else
		{
			int i = 0;
			esdsSize = FskMisaligned32_GetN(esds) - 8;
			esds    += 8;

			//find first start code
			for( i = 0; i < esdsSize; i++ )
			{
				if( esds[i] == 0 && esds[i+1] == 0 && esds[i+2] == 1 )
					break;
			}

			esds	 += i;
			esdsSize -= i;
		}

		Debug_DumpBox( esds, esdsSize );

		kinoma_ipp_lib_mp4v_init(FSK_ARCH_AUTO);
#ifdef DO_DEBLOCKING
		kinoma_ipp_lib_mp4v_deblocking_init(FSK_ARCH_AUTO);
#endif
		state->dec  = new MPEG4VideoDecoder();
		if( state->dec == NULL )
			return kFskErrMemFull;


		{
			VideoDecoderParams_V51	decParam;
			MediaData_V51			pData;

			pData.SetBufferPointer( (vm_byte *)esds, (size_t)esdsSize);
			pData.SetDataSize( (size_t)esdsSize);

			decParam.m_pData0				= &pData;
			decParam.info.framerate			= 1;
			decParam.info.duration			= 1;
			decParam.info.stream_subtype	= MPEG4_VIDEO_QTIME;
#ifndef DROP_COLOR_CONVERSION
			decParam.lpConverter			= NULL;
			decParam.lpConvertInit			= NULL;
			decParam.lpConvertInitPreview	= NULL;
#endif
			//***
			decParam.uiLimitThreads			= 1;
			decParam.lFlags					= 0;

			state->dec->Init(&decParam);
			
			if( NULL == desc ) 
			{
				int width, height;

				state->dec->GetDimensions( &width, &height );
				err = FskMemPtrNewClear(sizeof(QTImageDescriptionRecord), (FskMemPtr *)&desc);
                if( err ) goto bail;
				desc->idSize = sizeof(QTImageDescriptionRecord);				
				desc->width  = width;
				desc->height = height;
				state->desc  = desc;
			}
		}
		performance_reset( &state->perf );
	}
	performance_begin( &state->perf );
	
	Debug_DumpBox( (unsigned char *)data, dataSize );
	
	if( data != NULL )
	{
		MediaData_V51 src;
		UMC::VideoData_V51 dst;
		
		src.SetBufferPointer( (vm_byte *)data, (size_t)dataSize);
		src.SetDataSize( (size_t)dataSize);

		dst.SetVideoParameters( desc->width, desc->height, YV12 );
		{
			int approx_level;

			if( ((frameType&kFskImageTypeMask)==0) )
				approx_level = MP4V_LOSSY_SETTING_0;
			else
				approx_level = state->approx_level;
			
			state->dec->SetApprox( approx_level );
		}
		state->dec->GetFrame(&src, &dst);
		state->dec->GetYUV(&yuvPlannar);
	}
	else
	{
		Status	umcRes = UMC_OK;
		UMC::VideoData_V51 dst;
		
		dst.SetVideoParameters( desc->width, desc->height, YV12 );
		umcRes = state->dec->GetFrame(NULL, &dst);
		if( umcRes == UMC_END_OF_STREAM )
			yuvPlannar.isValid = 0;
		else 
			state->dec->GetYUV(&yuvPlannar);
	}

	// put image into bits...
	if(yuvPlannar.isValid && !(kFskImageFrameDrop & frameType))
	{
		FskBitmapFormatEnum	pixelFormat;
		unsigned char	*dstPtr;
		int			dstRowBytesY;
		int			src_width  = (int)(yuvPlannar.right - yuvPlannar.left);
		int			src_height = (int)(yuvPlannar.bottom - yuvPlannar.top);
		int			rot_width, rot_height;

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
		if (state->dst_pixel_format != pixelFormat)
		{
			FskBitmapWriteEnd(deco->bits);
			err = kFskErrBadData;
			goto bail;
		}

		dlog( "write out image = (%d, %d), format = %d, srcRowBytes = %d, dstRowBytes = %d\n", rot_width, rot_height, (int)pixelFormat, (int)yuvPlannar.rowBytesY, (int)dstRowBytesY);
		{
			if(state->dst_pixel_format == kFskBitmapFormatYUV420 )
			{
				unsigned char *u = dstPtr + (dstRowBytesY * (rot_height + (rot_height & 0)));
				unsigned char *v = u + (dstRowBytesY >> 1) * ((rot_height + 1) >> 1);
				
				dlog( "state->dst_pixel_format == kFskBitmapFormatYUV420 case\n");
				FskYUV420Copy(	rot_width, rot_height,
								yuvPlannar.y, yuvPlannar.cb, yuvPlannar.cr,
								(int)yuvPlannar.rowBytesY, (int)yuvPlannar.rowBytesCb,
								dstPtr, u, v,
								dstRowBytesY, dstRowBytesY >> 1);
#ifdef DO_DEBLOCKING
				if( state->do_postprocessing )
				{
					int mp4v_qp, h264_qp;

					state->dec->GetQp(&mp4v_qp);

					h264_qp = mp4v_to_h264_qp_lut[mp4v_qp];
					deblock_frame(h264_qp, dstPtr, rot_width, rot_height, dstRowBytesY );
				}
#endif
				Debug_DumpYUV( rot_width, rot_height, dstRowBytesY, dstRowBytesY >> 1, dstPtr, u, v ); 
			}
			else
			{
				dlog( "state->dst_pixel_format != kFskBitmapFormatYUV420 case\n");
#ifdef DO_DEBLOCKING
				if( state->do_postprocessing )
				{
					int mp4v_qp, h264_qp;
					int tmp_width		= ((src_width+1)>>1)<<1;
					int tmp_y_rowbytes  = ((src_width+7)>>3)<<3;
					int tmp_height		= ((src_height+1)>>1)<<1;
					int tmp_frame_size	= tmp_y_rowbytes*tmp_height;
				
					state->dec->GetQp(&mp4v_qp);
					h264_qp = mp4v_to_h264_qp_lut[mp4v_qp];

					if( state->tmp_y == NULL )
					{
						err = FskMemPtrNew(tmp_frame_size*3/2, (FskMemPtr*)&state->tmp_y);
						if (err) goto bail;

						state->tmp_u = state->tmp_y + tmp_frame_size;
						state->tmp_v = state->tmp_u + tmp_frame_size/4;
					}

					FskYUV420Copy(	((src_width+1)>>1)<<1, ((src_height+1)>>1)<<1,
									yuvPlannar.y, yuvPlannar.cb, yuvPlannar.cr,
									(int)yuvPlannar.rowBytesY, (int)yuvPlannar.rowBytesCb,
									state->tmp_y, state->tmp_u, state->tmp_v,
									tmp_y_rowbytes, tmp_y_rowbytes>>1);

					deblock_frame(h264_qp, state->tmp_y, tmp_width, tmp_height, tmp_y_rowbytes);

					FskYUV420Interleave_Generic //always copy even width and height
					(
						state->tmp_y, state->tmp_u, state->tmp_v, 
						dstPtr, tmp_height, tmp_width, 
						tmp_y_rowbytes, tmp_y_rowbytes>>1,
						dstRowBytesY, 
						state->rotation
					);
				}
				else	
#endif
				//always copy even width and height
				FskYUV420Interleave_Generic 
				(
					yuvPlannar.y, yuvPlannar.cb, yuvPlannar.cr, 
					dstPtr, ((src_height+1)>>1)<<1, ((src_width+1)>>1)<<1, 
					(int)yuvPlannar.rowBytesY, (int)yuvPlannar.rowBytesCb,
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
				buf_printf(interleave, dstPtr, dstRowBytesY, rot_height, x0+20, 40, "     movie_fps=%6.2f",  state->perf.movie_fps);
				if( state->do_postprocessing )
					buf_printf(interleave, dstPtr, dstRowBytesY, rot_height, x0+20, 60, "     play_mode= 0+p");
				else
					buf_printf(interleave, dstPtr, dstRowBytesY, rot_height, x0+20, 60, "     play_mode=%2d",     state->play_mode);
			}
#endif
		}

		FskBitmapWriteEnd(deco->bits);	
	}

bail:
	if( decodeTime != NULL )
		performance_end( &state->perf, *decodeTime );
	//performance_process( &state->perf );

	return err;
}


unsigned char * mp4_FindShortVideoStartMarkerPtr(unsigned char *ptr, int len)
{
    int  i;

	for (i = 0; i < len - 3; i++) 
	{
		if (
			(ptr[i] == 0 && ptr[i + 1] == 0 && (ptr[i + 2] & (~3)) == 0x80) ||
			(ptr[i] == 0 && ptr[i + 1] == 0 && (ptr[i + 2] & 0xF0) == 0x80) 
			)
		{
            return ptr + i + 2;
        }
    }
    return NULL;
}

unsigned char * mp4_FindVOPStartCodePtr(unsigned char *ptr, int len)
{
    int     i;
    for (i = 0; i < len - 3; i++) {
        if (ptr[i] == 0 && ptr[i + 1] == 0 && ptr[i + 2] == 1) 
		{
            if( ptr[i + 3] == 0xb6 )
				return ptr + i + 4;
        }
    }
    return NULL;
}


FskErr mp4DecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	FskErr err = kFskErrNone;
	UInt32	frame_type = 0;
	unsigned char *ptr;

	if (kFskImageDecompressMetaDataFrameType != metadata)
		return kFskErrUnimplemented;

	ptr = mp4_FindVOPStartCodePtr( (unsigned char *)deco->data, deco->dataSize);
	if( ptr != NULL )
	{
		unsigned char vop_coding_type = (ptr[0] & 0xc0 )>>6;
		
		if( vop_coding_type == 0 )
			frame_type = kFskImageFrameTypeSync;
		else if( vop_coding_type == 1 )
			frame_type = kFskImageFrameTypeDifference;
		else if( vop_coding_type == 2 )
			frame_type = kFskImageFrameTypeDroppable | kFskImageFrameTypeDifference;
		else
			err = kFskErrUnimplemented;

		goto bail;
	}

	ptr = mp4_FindShortVideoStartMarkerPtr( (unsigned char *)deco->data, deco->dataSize);
	if( ptr != NULL )
	{
		//short vide start marker:		22: 8+8+ 6
		//temprial reference:			8
		//marker bit:					1
		//zero bit:						1
		//split screen indicator:		1   ==>3rd byte we want to check
		//documenter camera indicator:	1
		//full picture freeze release:	1
		//source formar:				1
		//pixture coding type:			1
		//...
		unsigned char picture_coding_type = (ptr[2] & 0x08)>>3;

		if( picture_coding_type == 0 )
			frame_type = kFskImageFrameTypeSync;
		else
			frame_type = kFskImageFrameTypeDifference;

		goto bail;
	}

	err = kFskErrBadData;

bail:
	if( err == kFskErrNone )
	{
		if( value != NULL )
		{
			value->type = kFskMediaPropertyTypeInteger;
			value->value.integer = frame_type;
		}
	}

	return kFskErrNone;
}


FskErr mp4DecodeSetSampleNumber(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskMPEGVideoDecoder state = (FskMPEGVideoDecoder)stateIn;
	state->sampleNumber = property->value.integer;
	return kFskErrNone;
}

FskErr mp4DecodeSetSampleDescription(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskMPEGVideoDecoder state = (FskMPEGVideoDecoder)stateIn;
	FskMemPtrDisposeAt((void **)&state->desc);
	FskMemPtrNewFromData(property->value.data.dataSize, property->value.data.data, (FskMemPtr *)&state->desc);
	state->descSeed += 1;
	return kFskErrNone;
}

FskErr mp4DecodeSetQuality(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskMPEGVideoDecoder state = (FskMPEGVideoDecoder)stateIn;

	state->quality = (float)property->value.number;
	if( state->quality > 1.0 )
	{
		state->do_postprocessing = 1;
		state->quality = 1.0;
	}
	else
		state->do_postprocessing = 0;


	state->approx_level = quility_to_approx( state->quality );//;// = dropFrame ? 7 : 0;
			
	return kFskErrNone;
}

FskErr mp4DecodeSetPlayMode(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskMPEGVideoDecoder state = (FskMPEGVideoDecoder)stateIn;

	state->play_mode = property->value.integer;
			
	return kFskErrNone;
}


FskErr mp4DecodeSetRotation(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskMPEGVideoDecoder state = (FskMPEGVideoDecoder)stateIn;

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


FskErr mp4DecodeGetPerformanceInfo (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskMPEGVideoDecoder	state	= (FskMPEGVideoDecoder)stateIn;
	FskErr			err		= kFskErrNone;

	if (NULL == state->dec)
		return kFskErrBadState;

	performance_process( &state->perf );
	
	property->value.number	= state->perf.play_fps;
	property->type			= kFskMediaPropertyTypeFloat;

	return err;
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
	FskMPEGVideoDecoder state = (FskMPEGVideoDecoder)stateIn;
	FskBitmapFormatEnum prefered_yuvFormat = kFskBitmapFormatUnknown;
	UInt32 count = property->value.integers.count;
	
	dlog( "\n###########################################################################################\n" ); 
	dlog( "into mp4DecodeSetPreferredPixelFormat, propertyID: %d, propertyType: %d, count: %d\n", (int)propertyID, (int)property->type, (int)count);
	dlog( "prefered_yuvFormat: %d/%d/%d/%d/%d\n", (int)property->value.integers.integer[0],(int)property->value.integers.integer[1],(int)property->value.integers.integer[2],(int)property->value.integers.integer[3],(int)property->value.integers.integer[4]);
	
	dlog( "looking for kFskBitmapFormatYUV420i\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420i)
	
	dlog( "looking for kFskBitmapFormatYUV420\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420)
	
	if( prefered_yuvFormat != kFskBitmapFormatUnknown )
	{
		dlog( "got matched system preferred: %d\n", (int)prefered_yuvFormat);
		state->dst_pixel_format = prefered_yuvFormat;
	}
	
	dlog( "state->dst_pixel_format: %d\n", (int)state->dst_pixel_format);
	
	return kFskErrNone;
}
