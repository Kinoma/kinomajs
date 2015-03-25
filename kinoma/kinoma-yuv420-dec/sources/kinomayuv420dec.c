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

#include "kinomayuv420dec.h"
#include "FskYUV420Copy.h"
#include "FskEndian.h"
#include "FskBitmap.h"
#include "FskEnvironment.h"
#include "QTReader.h"


#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(kinomayuv420dec, kinomayuv420dec);
#define mlog  Fskkinomayuv420decPrintfMinimal
#define nlog  Fskkinomayuv420decPrintfNormal
#define vlog  Fskkinomayuv420decPrintfVerbose
#define dlog  Fskkinomayuv420decPrintfDebug

//#define FORCE_OUTPUT_AS_SOURCE

#if SRC_YUV420i
#define YUV420_FORMAT	kFskBitmapFormatYUV420i
#else
#define YUV420_FORMAT	kFskBitmapFormatYUV420
#endif

#define kBitmapCacheSize		50

static FskErr yuv420DecodeSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
//static FskErr yuv420DecodeSetRotation(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420DecodeSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property );

FskMediaPropertyEntryRecord yuv420DecodeProperties[] = 
{
	{kFskMediaPropertySampleDescription,		kFskMediaPropertyTypeData,		NULL,		yuv420DecodeSetSampleDescription},
 	{kFskMediaPropertyFormatInfo,				kFskMediaPropertyTypeData,		NULL,		yuv420DecodeSetSampleDescription},
	{kFskMediaPropertyPixelFormat,				kFskMediaPropertyTypeUInt32List,NULL,		yuv420DecodeSetPreferredPixelFormat},
	{kFskMediaPropertyUndefined,				kFskMediaPropertyTypeUndefined,	NULL,		NULL}
};


typedef struct 
{
	UInt32				sampleDescriptionSize;
	unsigned char		*sampleDescription;
	UInt32				sampleDescriptionSeed;

	FskBitmap			bitmaps[kBitmapCacheSize];
	FskBitmapFormatEnum				src_pixel_format;
	FskBitmapFormatEnum				dst_pixel_format;
	
	unsigned char		*y0;
	unsigned char		*u0;
	unsigned char		*v0;
    
    int                 is_bitmap;
    int                 useGL;
    
    
} kinomaYUV420Decode;


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


FskErr yuv420DecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
	dlog( "###########################################################################################\n" ); 
	dlog( "into yuv420DecodeCanHandle: format: %d, mime: %s\n", (int)format, mime ); 
    
	*canHandle =  (
				    ('yuv ' == format)									|| 
					(0 == FskStrCompare(mime, "x-video-codec/yuv420"))	||
				    (0 == FskStrCompare(mime, "x-video-codec/2vuy"))	||
                    (0 == FskStrCompare(mime, "x-video-codec/yuv420sp"))||
                    (0 == FskStrCompare(mime, "x-video-codec/yuv420spuv"))||
                    (0 == FskStrCompare(mime, "x-video-codec/yuv420spvu"))||
                    (0 == FskStrCompare(mime, "x-video-codec/bitmap"))
				   );
		
	return kFskErrNone;
}

FskErr yuv420DecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension)
{	
	kinomaYUV420Decode *state = NULL;
	FskErr err = kFskErrNone;

	err = FskMemPtrNewClear(sizeof(kinomaYUV420Decode), (FskMemPtr *)&state);
	BAIL_IF_ERR( err ); 
	
	state->y0 = NULL;
	state->u0 = NULL;
	state->v0 = NULL;
	state->src_pixel_format = kFskBitmapFormatUnknown;
	state->dst_pixel_format = kFskBitmapFormatUnknown;
    state->is_bitmap = (0 == FskStrCompare(mime, "x-video-codec/bitmap"));
    
    {
        const char *value = FskEnvironmentGet("useGL");
        state->useGL = value && (0 == FskStrCompare("1", value));
    }
	
	dlog( "###########################################################################################\n" );
	dlog( "in yuv420DecodeNew allocated state: %x, useGL :%d, state->is_bitmap: %d\n", (int)state, (int)state->useGL, (int)state->is_bitmap );
	
bail:
	if (kFskErrNone != err)
		yuv420DecodeDispose(state, deco);
	
	deco->state = state;
	
	dlog( "out of yuv420DecodeNew: err: %d\n", (int)err );
	
	return err;
}

FskErr yuv420DecodeDispose(void *stateIn, FskImageDecompress deco)
{	
	kinomaYUV420Decode *state = (kinomaYUV420Decode *)stateIn;
	int i;
	
	dlog( "###########################################################################################\n" ); 
	dlog( "into yuv420DecodeDispose\n" ); 
	
	if (NULL != state) 
	{
		if( state->sampleDescription != NULL ) 
			FskMemPtrDispose(state->sampleDescription);

		for (i = 0; i < kBitmapCacheSize; i++) 
			FskBitmapDispose(state->bitmaps[i]);
				
		if( state->y0 != NULL )
			FskMemPtrDispose(state->y0 );
		
		if( state->u0 != NULL )
			FskMemPtrDispose(state->u0 );
		
		if( state->v0 != NULL )
			FskMemPtrDispose(state->v0 );
		
		FskMemPtrDispose(state);
	}
		
	return kFskErrNone;
}



static int refit_yuv_16_interleave(int yuv_width, int yuv_height, unsigned char *uyvy_data, unsigned char *b_y, unsigned char *b_u, unsigned char *b_v  )
{
	//int y_size    = yuv_width*yuv_height;
	//int uv_width  = yuv_width/2;
	//int uv_height = yuv_height/2;
	//int uv_size   = uv_width*uv_height;
	int	uyvy_stride = yuv_width<<1;
	int s_stride  = 2*uyvy_stride - (2*yuv_width);
	int	err = 0;
	
	unsigned char *s0;
	unsigned char *s1;
	unsigned char *dy0;
	unsigned char *dy1;
	unsigned char *u0;
	unsigned char *v0; 
	int i,j;
	
	dlog(  "in refit_yuv_16_interleave uyvy_stride: %d, yuv_width: %d, yuv_height: %d\n", uyvy_stride, yuv_width, yuv_height );
	
	s0  = uyvy_data;
	s1  = s0 + uyvy_stride;
	
	dy0 = b_y;
	dy1 = dy0 + yuv_width;
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
	
	for( i = 0; i < yuv_height/2; i++ )
	{
		for( j = 0; j < yuv_width/2; j++ )
		{
			//dlog(  "i/j: %d/%d\n", i, j );
			PACK_CbYCrY
		}
		
		dy0 += yuv_width;
		dy1  = dy0 + yuv_width;
		s0  += s_stride;
		s1   = s0 + uyvy_stride;
	}	
	
//bail:
	dlog(  "out of refit_yuv_16_interleave, err: %d\n", err );
	
	return err;
}


FskErr yuv420DecodeDecompressFrame_direct(void *stateIn, FskImageDecompress deco, const void *data_in, UInt32 dataSize_in, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
    FskBitmap bits = NULL;
	FskErr	err	= kFskErrNone;
    dlog( "###########################################################################################\n" );
    dlog( "into yuv420DecodeDecompressFrame_direct\n" );

    dlog( "into directly set bitmap, data_in: %x\n", (int)data_in );
  
    bits = (FskBitmap)data_in;
    //FskBitmapUse( bits );
    deco->bits = bits;
    dlog( "bits->bounds.x/y/width/height: %d/%d/%d/%d\n", bits->bounds.x, bits->bounds.y, bits->bounds.width, bits->bounds.height );
    dlog( "bits->depth/pixelFormat/rowBytes: %d/%d/%d\n", bits->depth, bits->pixelFormat, bits->rowBytes );
    dlog( "bits->bits/bitsToDispose: %x/%x\n", bits->bits, bits->bitsToDispose );

//bail:
	dlog( "out of yuv420DecodeDecompressFrame_direct\n" );
	return err;
}


FskErr yuv420DecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data_in, UInt32 dataSize_in, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{	
	kinomaYUV420Decode		*state			= (kinomaYUV420Decode *)stateIn;
	QTImageDescription		desc			= (QTImageDescription)state->sampleDescription;
	FskBitmapFormatEnum					pixelFormat;
	unsigned char			*dstPtr		= NULL;
	long					dst_y_rb;
	long					width	= desc->width;
	long					height	= desc->height;
	long   					src_y_rb	= width;
	long	  				src_uv_rb	= src_y_rb>>1;
	FskImageDecompressComplete	completionFunction = deco->completionFunction;
	void					*completionRefcon  = deco->completionRefcon;
	FskBitmap				bits		 = NULL;
	FskErr					err			 = kFskErrNone;
    
	dlog( "###########################################################################################\n" ); 
	dlog( "into yuv420DecodeDecompressFrame, state->is_bitmap: %d\n", state->is_bitmap ); 
    
	if( data_in == NULL )//eos
		goto bail;
	
    if( state->is_bitmap )
        return yuv420DecodeDecompressFrame_direct(stateIn, deco, data_in, dataSize_in, decodeTime, compositionTimeOffset, compositionTime, frameType);
    
	completionFunction = NULL;
	completionRefcon   = NULL;
	
	//runtime only
	state->src_pixel_format = desc->cType;
	if( state->src_pixel_format != kFskBitmapFormatYUV420  && 
	    state->src_pixel_format != kFskBitmapFormatUYVY    && 
        state->src_pixel_format != kFskBitmapFormatYUV420spuv &&
	    state->src_pixel_format != kFskBitmapFormatYUV420spvu )
	{
		err = kFskErrUnimplemented;
		goto bail;
	}
	   
	if( state->dst_pixel_format == kFskBitmapFormatUnknown )
	{
		dlog( "dst pixel format is unknown, following src pixel format: %d\n", (int)state->src_pixel_format);
		state->dst_pixel_format = kFskBitmapFormatYUV420;//state->src_pixel_format;
	}
		 
    if( !state->useGL && (state->dst_pixel_format == kFskBitmapFormatYUV420spuv || state->dst_pixel_format == kFskBitmapFormatYUV420spvu ) )
    {
        dlog( "useGL is off, cannot handle dst pix format as kFskBitmapFormatYUV420spuv or kFskBitmapFormatYUV420spvu, set back to kFskBitmapFormatYUV420!!!\n");
        state->dst_pixel_format = kFskBitmapFormatYUV420;
    }
    
	dlog( "completionFunction:      %d\n", (int)completionFunction);
	dlog( "completionRefcon:		   %d\n", (int)completionRefcon );
	dlog( "state->src_pixel_format: %d\n", (int)state->src_pixel_format );
	dlog( "state->dst_pixel_format: %d\n", (int)state->dst_pixel_format );
	dlog( "width/height/size: %d/%d/%d\n", (int)width, (int)height, (int)dataSize_in );
	dlog( "state->dst_pixel_format: %d\n", (int)state->dst_pixel_format );
	
	
#ifdef FORCE_OUTPUT_AS_SOURCE
	dlog( "FORCE_OUTPUT_AS_SOURCE==> dst_pixel_format/src_pixel_format: %d/%d\n", (int)state->dst_pixel_format, (int)state->src_pixel_format );
	state->dst_pixel_format = state->src_pixel_format;
#endif
	
	if( deco->bits != NULL ) 
	{
		bits	   = deco->bits;
		deco->bits = NULL;
		RefitBitmap( state->dst_pixel_format, width, height, &bits );
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
	
	FskBitmapWriteBegin( bits, (void**)(&dstPtr), (SInt32 *)&dst_y_rb, &pixelFormat );
	if (state->dst_pixel_format != pixelFormat)
	{
		FskBitmapWriteEnd( bits );
		err = kFskErrBadData;
		goto bail;
	}
	
	dlog( "after FskBitmapWriteBegin(), dst_y_rb: %d, pixelFormat: %d\n", (int)dst_y_rb, (int)pixelFormat );
	
	if( state->dst_pixel_format == kFskBitmapFormatYUV420 )
	{
		unsigned char *dst_y; 
		unsigned char *dst_u;
		unsigned char *dst_v;
		int			  dst_frame_size = dst_y_rb*height;
		
		unsigned char	*src_y = NULL;
		unsigned char	*src_u = NULL;
		unsigned char	*src_v = NULL;	

		dlog( "state->dst_pixel_format == kFskBitmapFormatYUV420 case\n" );
		
		dst_y = dstPtr;
		dst_u = dst_y + dst_frame_size;
		dst_v = dst_u + (dst_frame_size>>2);
		
		if( state->src_pixel_format == kFskBitmapFormatYUV420 )
		{
			dlog( "this is to kFskBitmapFormatYUV420 straight out\n");
			src_y = (unsigned char *)data_in;
			src_u = src_y + (src_y_rb*height);	
			src_v = src_u + (src_uv_rb*(height>>1)) ;	
		}
		else if( state->src_pixel_format == kFskBitmapFormatUYVY )	
		{	
			dlog( "this is to convert 2vuy to yuv420planar mostly for verification purpose now that uyvy output pixel format is supported\n");
			if( state->y0 == NULL )
			{
				err = FskMemPtrNew(dst_frame_size, (FskMemPtr *)&state->y0);
				BAIL_IF_ERR(err);
			}

			if( state->u0 == NULL )
			{
				err = FskMemPtrNew(dst_frame_size>>2, (FskMemPtr *)&state->u0);
				BAIL_IF_ERR(err);
			}
			
			if( state->v0 == NULL )
			{
				err = FskMemPtrNew(dst_frame_size>>2, (FskMemPtr *)&state->v0);
				BAIL_IF_ERR(err);
			}	
			
			err = refit_yuv_16_interleave( width, height, (unsigned char *)data_in, state->y0, state->u0, state->v0 );
			BAIL_IF_ERR( err );
			
			src_y = state->y0;
			src_u = state->u0;
			src_v = state->v0;
		}
		else if( state->src_pixel_format == kFskBitmapFormatYUV420spuv || state->src_pixel_format == kFskBitmapFormatYUV420spvu )
		{	
			int				i;
			int				uv_width	= width>>1;
			int				uv_height	= height>>1;
			unsigned char	*uv_data	= (unsigned char *)data_in + (width*height);
			unsigned char	*b_u		= NULL;
			unsigned char	*b_v		= NULL;
			
			dlog( "this is to convert yuv420sp to yuv420planar\n");
			
			if( state->u0 == NULL )
			{
				err = FskMemPtrNew(dst_frame_size>>2, (FskMemPtr *)&state->u0);
				BAIL_IF_ERR(err);
			}
			
			if( state->v0 == NULL )
			{
				err = FskMemPtrNew(dst_frame_size>>2, (FskMemPtr *)&state->v0);
				BAIL_IF_ERR(err);
			}	
			
			b_u = state->u0;
			b_v = state->v0;
			
            if( state->src_pixel_format == kFskBitmapFormatYUV420spuv )
                for( i = 0; i < uv_width*uv_height; i++ )
                {
                    *b_u++ = *uv_data++;
                    *b_v++ = *uv_data++;
                }
            else
                for( i = 0; i < uv_width*uv_height; i++ )
                {
                    *b_v++ = *uv_data++;
                    *b_u++ = *uv_data++;
                }
            
			
			src_y = (unsigned char *)data_in;
			src_u = state->u0;
			src_v = state->v0;
		}
		else
		{
			dlog( "a source pixel format we can't handle!!!\n" );	
		}
		
		FskYUV420Copy(	  width, height,
						  src_y, src_u, src_v, src_y_rb, src_uv_rb,
						  dst_y, dst_u, dst_v,
						  dst_y_rb, dst_y_rb >> 1
					  );
			
	}
	else if( state->dst_pixel_format == kFskBitmapFormatYUV420i )
	{
		unsigned char	*src_y = NULL;
		unsigned char	*src_u = NULL;
		unsigned char	*src_v = NULL;	
		int				dst_frame_size = dst_y_rb*height;
		
		dlog( "state->dst_pixel_format == kFskBitmapFormatYUV420i case\n" );
		
		if( state->src_pixel_format == kFskBitmapFormatYUV420 )
		{
			src_y = (unsigned char *)data_in;
			src_u = src_y + (src_y_rb*height);	
			src_v = src_u + (src_uv_rb*(height>>1)) ;	
		}
		else if( state->src_pixel_format == kFskBitmapFormatUYVY )
		{	
			dlog( "this is to convert 2vuy to yuv420planar mostly for verification purpose now that uyvy output pixel format is supported\n");
			if( state->y0 == NULL )
			{
				err = FskMemPtrNew(dst_frame_size, (FskMemPtr *)&state->y0);
				BAIL_IF_ERR(err);
			}
			
			if( state->u0 == NULL )
			{
				err = FskMemPtrNew(dst_frame_size>>2, (FskMemPtr *)&state->u0);
				BAIL_IF_ERR(err);
			}
			
			if( state->v0 == NULL )
			{
				err = FskMemPtrNew(dst_frame_size>>2, (FskMemPtr *)&state->v0);
				BAIL_IF_ERR(err);
			}	
			
			err = refit_yuv_16_interleave( width, height, (unsigned char *)data_in, state->y0, state->u0, state->v0 );
			BAIL_IF_ERR( err );
			
			src_y = state->y0;
			src_u = state->u0;
			src_v = state->v0;
		}
		else if( state->src_pixel_format == kFskBitmapFormatYUV420spuv || state->src_pixel_format == kFskBitmapFormatYUV420spvu)
		{	
			int				i;
			int				uv_width	= width>>1;
			int				uv_height	= height>>1;
			unsigned char	*uv_data	= (unsigned char *)data_in + (width*height);
			unsigned char	*b_u		= NULL;
			unsigned char	*b_v		= NULL;
			
			dlog( "this is to convert yuv420sp to yuv420planar\n");
			
			if( state->u0 == NULL )
			{
				err = FskMemPtrNew(dst_frame_size>>2, (FskMemPtr *)&state->u0);
				BAIL_IF_ERR(err);
			}

			if( state->v0 == NULL )
			{
				err = FskMemPtrNew(dst_frame_size>>2, (FskMemPtr *)&state->v0);
				BAIL_IF_ERR(err);
			}	
			
			b_u = state->u0;
			b_v = state->v0;
			
            if( state->src_pixel_format == kFskBitmapFormatYUV420spuv )
                for( i = 0; i < uv_width*uv_height; i++ )
                {
                    *b_u++ = *uv_data++;
                    *b_v++ = *uv_data++;
                }
            else
                for( i = 0; i < uv_width*uv_height; i++ )
                {
                    *b_v++ = *uv_data++;
                    *b_u++ = *uv_data++;
                }
			
			src_y = (unsigned char *)data_in;
			src_u = state->u0;
			src_v = state->v0;
		}
		else
		{
			dlog( "a source pixel format we can't handle!!!\n" );
		}
		
		//always copy even width and height
		FskYUV420Interleave_Generic
		(
		 src_y, src_u, src_v, 
		 dstPtr, ((height+1)>>1)<<1, ((width+1)>>1)<<1, 
		 src_y_rb, src_uv_rb,
		 dst_y_rb,
		 0
		 );
	}
	else if( state->src_pixel_format == kFskBitmapFormatUYVY && state->dst_pixel_format == kFskBitmapFormatUYVY )
	{
		dlog( "state->src_pixel_format == kFskBitmapFormatUYVY && state->dst_pixel_format == kFskBitmapFormatUYVY case\n" );
		//int frame_size = dst_y_rb*height;
		memcpy( dstPtr, data_in, dst_y_rb*height );
	}
	else if( state->src_pixel_format == kFskBitmapFormatYUV420spuv && state->dst_pixel_format == kFskBitmapFormatYUV420spuv )
	{
		dlog( "state->src_pixel_format == kFskBitmapFormatYUV420spuv && state->dst_pixel_format == kFskBitmapFormatYUV420spuv case\n" );
		//int frame_size = dst_y_rb*height;
		if (src_y_rb == dst_y_rb) {
			memcpy( dstPtr, data_in, dst_y_rb*height*3/2 );
		} 
		else {
			const unsigned char *srcPtr = data_in;
			int i;
			for (i=0; i<height*3/2; i++) {
				memcpy( dstPtr, srcPtr, src_y_rb );
				dstPtr += dst_y_rb;
				srcPtr += src_y_rb;
			}
		}
	}
	else if( state->src_pixel_format == kFskBitmapFormatYUV420spvu && state->dst_pixel_format == kFskBitmapFormatYUV420spvu )
	{
		dlog( "state->src_pixel_format == kFskBitmapFormatYUV420spvu && state->dst_pixel_format == kFskBitmapFormatYUV420spvu case\n" );
		//int frame_size = dst_y_rb*height;
		if (src_y_rb == dst_y_rb) {
			memcpy( dstPtr, data_in, dst_y_rb*height*3/2 );
		} 
		else {
			const unsigned char *srcPtr = data_in;
			int i;
			for (i=0; i<height*3/2; i++) {
				memcpy( dstPtr, srcPtr, src_y_rb );
				dstPtr += dst_y_rb;
				srcPtr += src_y_rb;
			}
		}
	}
	
	FskBitmapWriteEnd( bits );
	
	dlog("returning a bits: %x\n", (int)bits);		
	if( completionFunction != NULL )	
	{
		dlog("Async API: returning a bits: %x\n", (int)bits);		
		(completionFunction)(deco, completionRefcon, kFskErrNone, bits);
		
		dlog( "resetting deco->completionFunction and deco->completionRefcon\n" );
		deco->completionFunction = NULL;
		deco->completionRefcon = NULL;
	}
	else
	{
		dlog("Sync API: returning a bits: %x\n", (int)bits);		
		deco->bits = bits;
	}
	
bail:
	dlog( "out of yuv420DecodeDecompressFrame\n" );
	return err;
}


FskErr yuv420DecodeFlush(void *stateIn, FskImageDecompress deco )
{
	//kinomaYUV420Decode		*state	= (kinomaYUV420Decode *)stateIn;
	FskErr				err		= kFskErrNone;
	
	dlog( "###########################################################################################\n" ); 
	dlog( "into yuv420DecodeFlush\n");

//bail:
	dlog( "out of yuv420DecodeFlush: err: %d\n", (int)err );
	return err;
}


FskErr yuv420DecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	//kinomaYUV420Decode *state  = (kinomaYUV420Decode *)stateIn;
	FskErr		  err = kFskErrNone;

	dlog( "###########################################################################################\n" ); 
	dlog( "into yuv420DecodeGetMetaData\n");

	if (kFskImageDecompressMetaDataFrameType != metadata)
	{
		err = kFskErrUnimplemented;
		goto bail;
	}

	value->type = kFskMediaPropertyTypeInteger;
	value->value.integer = kFskImageFrameTypeSync;

bail:	
	dlog( "out of yuv420DecodeGetMetaData: err: %d\n", (int)err );
	return err;
}


FskErr yuv420DecodeSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaYUV420Decode *state = (kinomaYUV420Decode *)stateIn;
	QTImageDescription	desc = NULL;
	int err = kFskErrNone;
	
	dlog( "###########################################################################################\n" ); 
	dlog( "into yuv420DecodeSetSampleDescription\n");

	state->sampleDescriptionSeed++;
	if( state->sampleDescription != NULL )
	{
		dlog( "disposing existing state->sampleDescription\n");
		FskMemPtrDisposeAt((void **)&state->sampleDescription);
	}
	state->sampleDescriptionSize = property->value.data.dataSize;
    
	dlog( "state->sampleDescriptionSize: %d\n", (int)state->sampleDescriptionSize);
	err = FskMemPtrNewFromData(state->sampleDescriptionSize, property->value.data.data, (FskMemPtr *)&state->sampleDescription);
	
	desc = (QTImageDescription)state->sampleDescription;
	state->src_pixel_format = desc->cType;
	dlog( "state->src_pixel_format: %d\n", (int)state->src_pixel_format);
   
    {
        unsigned char	*bitmap_flag = NULL;
        bitmap_flag = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)desc, 'btmp');
        if( bitmap_flag != NULL )
        {
            state->is_bitmap = *(UInt32 *)(bitmap_flag+8);
            dlog( "###### got state->is_bitmap: %d\n", state->is_bitmap );
        }
    }

//bail:
	dlog( "out of yuv420DecodeSetSampleDescription: err: %d\n", (int)err );
	return err;
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

FskErr yuv420DecodeSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property )
{
	kinomaYUV420Decode *state = (kinomaYUV420Decode *)stateIn;
	FskBitmapFormatEnum prefered_yuvFormat = kFskBitmapFormatUnknown;
	UInt32 i,count = property->value.integers.count;
	UInt32 propertyType = property->type;

	dlog( "###########################################################################################\n" ); 
	dlog( "into yuv420DecodeSetPreferredPixelFormat, propertyID: %d, propertyType: %d, count: %d\n", (int)propertyID, (int)propertyType, (int)count);
	dlog( "prefered_yuvFormat: %d/%d/%d/%d/%d\n", (int)property->value.integers.integer[0],(int)property->value.integers.integer[1],(int)property->value.integers.integer[2],(int)property->value.integers.integer[3],(int)property->value.integers.integer[4]);
	
	dlog( "looking for state->src_pixel_format: %d\n", (int)state->src_pixel_format);
	SET_PREFERRED_PIXEL_FORMAT(state->src_pixel_format)
	
	dlog( "looking for kFskBitmapFormatYUV420\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420)

	dlog( "looking for kFskBitmapFormatUYVY\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatUYVY)
	
	dlog( "looking for kFskBitmapFormatYUV420spvu\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420spvu)
	
	dlog( "looking for kFskBitmapFormatYUV420spuv\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420spuv)
	
	dlog( "looking for kFskBitmapFormatYUV420i\n");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420i)
	
	if( prefered_yuvFormat != kFskBitmapFormatUnknown )
	{
		dlog( "got matched system preferred: %d\n", (int)prefered_yuvFormat);
		state->dst_pixel_format = prefered_yuvFormat;
	}
	else
	{
		dlog( "no matched system preferred, use default kFskBitmapFormatYUV420\n");
		state->dst_pixel_format = kFskBitmapFormatYUV420;
	}
		
	dlog( "state->dst_pixel_format: %d\n", (int)state->dst_pixel_format);
	    
	return kFskErrNone;
}
