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

int parse_yuv_header(const void *data_in, UInt32 dataSize_in, int *width, int *height, int *fps, int *pixel_format, char **mime  )
{
	unsigned char *s = (unsigned char *)data_in;

	if( dataSize_in < 12 )
		return kFskErrUnknownElement;
		
	if
	(
	   s[0] == 'y' &&
	   s[1] == 'u' &&
	   s[2] == 'v' &&
	   s[3] == '4' &&
	   s[4] == '2' &&
	   s[5] == '0'
	)
	{
		if( mime != NULL )
			*mime = FskStrDoCopy("image/yuv");
		
		if( pixel_format != NULL )
			*pixel_format = kFskBitmapFormatYUV420;
	}
	else if
		(
		 s[0] == '2' &&
		 s[1] == 'v' &&
		 s[2] == 'u' &&
		 s[3] == 'y' &&
		 s[4] == ' ' &&
		 s[5] == ' '
		 )
	{
		if( mime != NULL )
			*mime = FskStrDoCopy("image/yuv");
		
		if( pixel_format != NULL )
			*pixel_format = kFskBitmapFormatUYVY;
	}
	else if
		(
		 s[0] == 's' &&
		 s[1] == 'p' &&
		 s[2] == 'u' &&
		 s[3] == 'v' &&
		 s[4] == ' ' &&
		 s[5] == ' '
		 )
	{
		if( mime != NULL )
			*mime = FskStrDoCopy("image/yuv");
		
		if( pixel_format != NULL )
			*pixel_format = kFskBitmapFormatYUV420spuv;
	}
	else if
		(
		 s[0] == 's' &&
		 s[1] == 'p' &&
		 s[2] == 'v' &&
		 s[3] == 'u' &&
		 s[4] == ' ' &&
		 s[5] == ' '
		 )
	{
		if( mime != NULL )
			*mime = FskStrDoCopy("image/yuv");
		
		if( pixel_format != NULL )
			*pixel_format = kFskBitmapFormatYUV420spvu;
	}
	else
		return kFskErrUnknownElement;

	s += 6;
	if( fps != NULL ) *fps = (s[0]<<8)|(s[1]<<0); 
	s+=2;
	if( width != NULL ) *width = (s[0]<<8)|(s[1]<<0); 
	s+=2;
	if( height != NULL ) *height = (s[0]<<8)|(s[1]<<0);
	
	return kFskErrNone;
}


FskErr yuv420DecodeSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	int err = kFskErrUnknownElement;
	//int offset = 0;

	mlog("###########################################################################################");
	mlog("into yuv420DecodeSniff()");

	err = parse_yuv_header( data, dataSize, NULL, NULL, NULL, NULL, mime  );
	if( err == kFskErrNone )
	{
		mlog(" yuv420ReaderSniff() returning *mime: %s", *mime);
	}

	return err;
}



FskErr yuv420DecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
	dlog( "###########################################################################################" );
	dlog( "into yuv420DecodeCanHandle: format: %d, mime: %s", (int)format, mime );
    
	*canHandle =  (
				    ('yuv ' == format)									|| 
					(0 == FskStrCompare(mime, "x-video-codec/yuv420"))	||
				    (0 == FskStrCompare(mime, "x-video-codec/2vuy"))	||
                    (0 == FskStrCompare(mime, "x-video-codec/yuv420sp"))||
                    (0 == FskStrCompare(mime, "x-video-codec/yuv420spuv"))||
                    (0 == FskStrCompare(mime, "x-video-codec/yuv420spvu"))||
                    (0 == FskStrCompare(mime, "x-video-codec/bitmap"))  ||
                    (0 == FskStrCompare(mime, "image/yuv"))
				   );
		
	dlog( "yuv420DecodeCanHandle format: %d, mime: %s, returns *canHandle: %d", (int)format, mime , *canHandle);
		
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
	
	dlog( "###########################################################################################" );
	dlog( "in yuv420DecodeNew allocated state: %x, useGL :%d, state->is_bitmap: %d", (int)state, (int)state->useGL, (int)state->is_bitmap );
	
bail:
	if (kFskErrNone != err)
		yuv420DecodeDispose(state, deco);
	
	deco->state = state;
	
	dlog( "out of yuv420DecodeNew: err: %d", (int)err );
	
	return err;
}

FskErr yuv420DecodeDispose(void *stateIn, FskImageDecompress deco)
{	
	kinomaYUV420Decode *state = (kinomaYUV420Decode *)stateIn;
	int i;
	
	dlog( "###########################################################################################" );
	dlog( "into yuv420DecodeDispose" );
	
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
	
	dlog(  "in refit_yuv_16_interleave uyvy_stride: %d, yuv_width: %d, yuv_height: %d", uyvy_stride, yuv_width, yuv_height );
	
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
			//dlog(  "i/j: %d/%d", i, j );
			PACK_CbYCrY
		}
		
		dy0 += yuv_width;
		dy1  = dy0 + yuv_width;
		s0  += s_stride;
		s1   = s0 + uyvy_stride;
	}	
	
//bail:
	dlog(  "out of refit_yuv_16_interleave, err: %d", err );
	
	return err;
}


FskErr yuv420DecodeDecompressFrame_direct(void *stateIn, FskImageDecompress deco, const void *data_in, UInt32 dataSize_in, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
    FskBitmap bits = NULL;
	FskErr	err	= kFskErrNone;
    dlog( "###########################################################################################" );
    dlog( "into yuv420DecodeDecompressFrame_direct" );

    mlog( "into directly set bitmap, data_in: %x", (int)data_in );
  
    bits = (FskBitmap)data_in;
    //FskBitmapUse( bits );
    deco->bits = bits;
    dlog( "bits->bounds.x/y/width/height: %d/%d/%d/%d", bits->bounds.x, bits->bounds.y, bits->bounds.width, bits->bounds.height );
    dlog( "bits->depth/pixelFormat/rowBytes: %d/%d/%d", bits->depth, bits->pixelFormat, bits->rowBytes );
    dlog( "bits->bits/bitsToDispose: %x/%x", bits->bits, bits->bitsToDispose );

//bail:
	dlog( "out of yuv420DecodeDecompressFrame_direct" );
	return err;
}

#if FSKBITMAP_OPENGL
#include "FskGLBlit.h"

static int preferred_yuv_format = kFskBitmapFormatUnknown;
void get_preferred_yuv_formats(FskBitmapFormatEnum *yuvFormat_out)
{
	FskBitmapFormatEnum *fmtp = yuvFormat_out;

	UInt32 stp[4];													/* Source types with increasing cost */
	FskGLSourceTypes(stp);											/* 0: hardware; 1: shader; 2: in-place lossless conversion; 3: conversion at a high price */
	if (stp[0] & (1 << kFskBitmapFormatYUV420))						/* If there is hardware support for YUV 4:2:0 planar, ... */
		*fmtp++ = kFskBitmapFormatYUV420;							/* ... it is the next choice */
	if (stp[0] & (1 << kFskBitmapFormatYUV420spuv))					/* If there is hardware support for YUV 4:2:0 semi-planar, ... */
		*fmtp++ = kFskBitmapFormatYUV420spuv;						/* ... it is the next choice */
	if (stp[0] & (1 << kFskBitmapFormatYUV420spvu))					/* If there is hardware support for YUV 4:2:0 semi-planar, ... */
		*fmtp++ = kFskBitmapFormatYUV420spvu;						/* ... it is the next choice */
	if (stp[0] & (1 << kFskBitmapFormatUYVY))						/* If there is hardware support for YUV 4:2:2 UYVY chunky, ... */
		*fmtp++ = kFskBitmapFormatUYVY;								/* ... it is the next choice */
	#if GLES_VERSION == 2
		if ((stp[0] ^ stp[1]) & (1 << kFskBitmapFormatYUV420spuv))	/* If YUV 4:2:0sp was not supported in hardware, but is in a shader, ... */
			*fmtp++ = kFskBitmapFormatYUV420spuv;					/* ... it is the next choice */
		if ((stp[0] ^ stp[1]) & (1 << kFskBitmapFormatYUV420spvu))	/* If YUV 4:2:0sp was not supported in hardware, but is in a shader, ... */
			*fmtp++ = kFskBitmapFormatYUV420spvu;					/* ... it is the next choice */
		if ((stp[0] ^ stp[1]) & (1 << kFskBitmapFormatYUV420))		/* If YUV 4:2:0 was not supported in hardware, but is in a shader, ... */
			*fmtp++ = kFskBitmapFormatYUV420;						/* ... it is the next choice */
		if ((stp[0] ^ stp[1]) & (1 << kFskBitmapFormatUYVY))		/* If YUV 4:2:2 UYVY was not supported in hardware, but is in a shader, ... */
			*fmtp++ = kFskBitmapFormatUYVY;							/* ... it is the next choice */
	#endif
	/* We skip analysis of stp[2] because that is convert-in-place, which we can't do with YUV */
	if ((stp[1] ^ stp[3]) & (1 << kFskBitmapFormatYUV420))			/* If YUV 4:2:0 was not supported in HW or shader, but can be converted at any expense ... */
		*fmtp++ = kFskBitmapFormatYUV420;							/* ... it is the next choice */
	if ((stp[1] ^ stp[3]) & (1 << kFskBitmapFormatYUV420spuv))		/* If YUV 4:2:0sp was not supported in HW or shader, but can be converted at any expense ... */
		*fmtp++ = kFskBitmapFormatYUV420spuv;						/* ... it is the next choice */
	if ((stp[1] ^ stp[3]) & (1 << kFskBitmapFormatYUV420spvu))		/* If YUV 4:2:0sp was not supported in HW or shader, but can be converted at any expense ... */
		*fmtp++ = kFskBitmapFormatYUV420spvu;						/* ... it is the next choice */
	if ((stp[1] ^ stp[3]) & (1 << kFskBitmapFormatUYVY))			/* If YUV 4:2:2 UYVY was not supported in HW or shader, but can be converted at any expense ... */
		*fmtp++ = kFskBitmapFormatUYVY;								/* ... it is the next choice */
}
#endif


FskErr yuv420DecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data_in, UInt32 dataSize_in, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{	
	kinomaYUV420Decode		*state			= (kinomaYUV420Decode *)stateIn;
	QTImageDescription		desc			= (QTImageDescription)state->sampleDescription;
	FskBitmapFormatEnum		pixelFormat;
	unsigned char			*dstPtr		= NULL;
	int						width, height, dst_y_rb, src_y_rb, src_uv_rb;
	FskImageDecompressComplete	completionFunction = deco->completionFunction;
	void					*completionRefcon  = deco->completionRefcon;
	FskBitmap				bits		 = NULL;
	FskErr					err			 = kFskErrNone;
    
	mlog( "###########################################################################################" );
	mlog( "into yuv420DecodeDecompressFrame, dataSize_in: %d", dataSize_in );
    
	if( data_in == NULL )//eos
		goto bail;
	
    if( state->is_bitmap )
        return yuv420DecodeDecompressFrame_direct(stateIn, deco, data_in, dataSize_in, decodeTime, compositionTimeOffset, compositionTime, frameType);
    
	completionFunction = NULL;
	completionRefcon   = NULL;
	
	if( desc != NULL )
	{
		width	= desc->width;
		height	= desc->height;
	
		//runtime only
		state->src_pixel_format = desc->cType;
	}
	else
	{
		int fps;
		err = parse_yuv_header( data_in, dataSize_in, &width, &height, &fps, &state->src_pixel_format, NULL  );
		if( err )
			goto bail;
	
		mlog( "==>parse_yuv_header");
		mlog( "width: %d", width);
		mlog( "height: %d", height);
		mlog( "fps: %d", fps);
		mlog( "state->src_pixel_format: %d", state->src_pixel_format);
	}
	
	src_y_rb	= width;
	src_uv_rb	= src_y_rb>>1;
	
	if( state->src_pixel_format != kFskBitmapFormatYUV420  && 
	    state->src_pixel_format != kFskBitmapFormatUYVY    && 
        state->src_pixel_format != kFskBitmapFormatYUV420spuv &&
	    state->src_pixel_format != kFskBitmapFormatYUV420spvu )
	{
		err = kFskErrUnimplemented;
		mlog( "src pixel format not supported, bailing!!!");
		goto bail;
	}
	   
#if FSKBITMAP_OPENGL
	if( state->dst_pixel_format == kFskBitmapFormatUnknown )
	{
		if( preferred_yuv_format ==  kFskBitmapFormatUnknown )
		{
			FskMediaPropertyValueRecord pixelFormat;
			FskBitmapFormatEnum preferredYUVFormats[12]={kFskBitmapFormatUnknown};
			get_preferred_yuv_formats(preferredYUVFormats);

			pixelFormat.value.integers.integer = (UInt32 *)&preferredYUVFormats[0];						/* Choose the favorite format */
			for (pixelFormat.value.integers.count = 0; 0 != preferredYUVFormats[pixelFormat.value.integers.count]; pixelFormat.value.integers.count++)
				;
			pixelFormat.type = kFskMediaPropertyTypeUInt32List;
			mlog( "dst pixel format is unknown, asked, yuv420DecodeDecompressFrame=>calling FskImageDecompressSetProperty()");
			FskImageDecompressSetProperty(deco, kFskMediaPropertyPixelFormat, &pixelFormat);
			preferred_yuv_format = state->dst_pixel_format;
		}
		else
			state->dst_pixel_format = preferred_yuv_format;

		mlog( "setting system preferred dst pixel format: %d", (int)state->dst_pixel_format);
	}
#endif

	if( state->dst_pixel_format == kFskBitmapFormatUnknown )
	{
		mlog( "dst pixel format is unknown, following src pixel format: %d", (int)state->src_pixel_format);
		state->dst_pixel_format = state->src_pixel_format;//kFskBitmapFormatYUV420;//;
	}

		 
    if( !state->useGL && (state->dst_pixel_format == kFskBitmapFormatYUV420spuv || state->dst_pixel_format == kFskBitmapFormatYUV420spvu ) )
    {
        dlog( "useGL is off, cannot handle dst pix format as kFskBitmapFormatYUV420spuv or kFskBitmapFormatYUV420spvu, set back to kFskBitmapFormatYUV420!!!");
        state->dst_pixel_format = kFskBitmapFormatYUV420;
    }
    
	dlog( "completionFunction:      %d", (int)completionFunction);
	dlog( "completionRefcon:		   %d", (int)completionRefcon );
	dlog( "state->src_pixel_format: %d", (int)state->src_pixel_format );
	dlog( "state->dst_pixel_format: %d", (int)state->dst_pixel_format );
	dlog( "width/height/size: %d/%d/%d", (int)width, (int)height, (int)dataSize_in );
	dlog( "state->dst_pixel_format: %d", (int)state->dst_pixel_format );
	
	
#ifdef FORCE_OUTPUT_AS_SOURCE
	dlog( "FORCE_OUTPUT_AS_SOURCE==> dst_pixel_format/src_pixel_format: %d/%d", (int)state->dst_pixel_format, (int)state->src_pixel_format );
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
	
	dlog( "after FskBitmapWriteBegin(), dst_y_rb: %d, pixelFormat: %d", (int)dst_y_rb, (int)pixelFormat );
	
	if( state->dst_pixel_format == kFskBitmapFormatYUV420 )
	{
		unsigned char *dst_y; 
		unsigned char *dst_u;
		unsigned char *dst_v;
		int			  dst_frame_size = dst_y_rb*height;
		
		unsigned char	*src_y = NULL;
		unsigned char	*src_u = NULL;
		unsigned char	*src_v = NULL;	

		mlog( "state->dst_pixel_format == kFskBitmapFormatYUV420 case" );
		
		dst_y = dstPtr;
		dst_u = dst_y + dst_frame_size;
		dst_v = dst_u + (dst_frame_size>>2);
		
		if( state->src_pixel_format == kFskBitmapFormatYUV420 )
		{
			mlog( "this is to kFskBitmapFormatYUV420 straight out");
			src_y = (unsigned char *)data_in;
			src_u = src_y + (src_y_rb*height);	
			src_v = src_u + (src_uv_rb*(height>>1)) ;	
		}
		else if( state->src_pixel_format == kFskBitmapFormatUYVY )	
		{	
			mlog( "this is to convert 2vuy to yuv420planar mostly for verification purpose now that uyvy output pixel format is supported");
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
			
			mlog( "this is to convert yuv420sp to yuv420planar");
			
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
			mlog( "a source pixel format we can't handle!!!" );
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
		
		mlog( "state->dst_pixel_format == kFskBitmapFormatYUV420i case" );
		
		if( state->src_pixel_format == kFskBitmapFormatYUV420 )
		{
			src_y = (unsigned char *)data_in;
			src_u = src_y + (src_y_rb*height);	
			src_v = src_u + (src_uv_rb*(height>>1)) ;	
		}
		else if( state->src_pixel_format == kFskBitmapFormatUYVY )
		{	
			mlog( "this is to convert 2vuy to yuv420planar mostly for verification purpose now that uyvy output pixel format is supported");
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
			
			mlog( "this is to convert yuv420sp to yuv420planar");
			
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
			mlog( "a source pixel format we can't handle!!!" );
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
		mlog( "state->src_pixel_format == kFskBitmapFormatUYVY && state->dst_pixel_format == kFskBitmapFormatUYVY case" );
		//int frame_size = dst_y_rb*height;
		memcpy( dstPtr, data_in, dst_y_rb*height );
	}
	else if( state->src_pixel_format == kFskBitmapFormatYUV420spuv && state->dst_pixel_format == kFskBitmapFormatYUV420spuv )
	{
		mlog( "state->src_pixel_format == kFskBitmapFormatYUV420spuv && state->dst_pixel_format == kFskBitmapFormatYUV420spuv case" );
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
		mlog( "state->src_pixel_format == kFskBitmapFormatYUV420spvu && state->dst_pixel_format == kFskBitmapFormatYUV420spvu case" );
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
	else
	{
		mlog( "not supported!!!" );
	}
	
	FskBitmapWriteEnd( bits );
	
	dlog("returning a bits: %x", (int)bits);
	if( completionFunction != NULL )	
	{
		dlog("Async API: returning a bits: %x", (int)bits);
		(completionFunction)(deco, completionRefcon, kFskErrNone, bits);
		
		dlog( "resetting deco->completionFunction and deco->completionRefcon" );
		deco->completionFunction = NULL;
		deco->completionRefcon = NULL;
	}
	else
	{
		dlog("Sync API: returning a bits: %x", (int)bits);
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
	
	dlog( "###########################################################################################" );
	dlog( "into yuv420DecodeFlush");

//bail:
	dlog( "out of yuv420DecodeFlush: err: %d", (int)err );
	return err;
}


FskErr yuv420DecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	//kinomaYUV420Decode *state  = (kinomaYUV420Decode *)stateIn;
	FskErr		  err = kFskErrNone;

	mlog( "###########################################################################################" );
	mlog( "into yuv420DecodeGetMetaData");

	if (kFskImageDecompressMetaDataFrameType != metadata)
	{
		err = kFskErrUnimplemented;
		goto bail;
	}

	value->type = kFskMediaPropertyTypeInteger;
	value->value.integer = kFskImageFrameTypeSync;

bail:	
	mlog( "out of yuv420DecodeGetMetaData: err: %d", (int)err );
	return err;
}


FskErr yuv420DecodeSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	kinomaYUV420Decode *state = (kinomaYUV420Decode *)stateIn;
	QTImageDescription	desc = NULL;
	int err = kFskErrNone;
	
	mlog( "###########################################################################################" );
	mlog( "into yuv420DecodeSetSampleDescription");

	state->sampleDescriptionSeed++;
	if( state->sampleDescription != NULL )
	{
		mlog( "disposing existing state->sampleDescription");
		FskMemPtrDisposeAt((void **)&state->sampleDescription);
	}
	state->sampleDescriptionSize = property->value.data.dataSize;
    
	mlog( "state->sampleDescriptionSize: %d", (int)state->sampleDescriptionSize);
	err = FskMemPtrNewFromData(state->sampleDescriptionSize, property->value.data.data, (FskMemPtr *)&state->sampleDescription);
	
	desc = (QTImageDescription)state->sampleDescription;
	state->src_pixel_format = desc->cType;
	mlog( "state->src_pixel_format: %d", (int)state->src_pixel_format);
   
    {
        unsigned char	*bitmap_flag = NULL;
        bitmap_flag = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)desc, 'btmp');
        if( bitmap_flag != NULL )
        {
            state->is_bitmap = *(UInt32 *)(bitmap_flag+8);
            mlog( "###### got state->is_bitmap: %d", state->is_bitmap );
        }
    }

//bail:
	mlog( "out of yuv420DecodeSetSampleDescription: err: %d", (int)err );
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

	mlog( "###########################################################################################" );
	mlog( "into yuv420DecodeSetPreferredPixelFormat, propertyID: %d, propertyType: %d, count: %d", (int)propertyID, (int)propertyType, (int)count);
	mlog( "prefered_yuvFormat: %d/%d/%d/%d/%d", (int)property->value.integers.integer[0],(int)property->value.integers.integer[1],(int)property->value.integers.integer[2],(int)property->value.integers.integer[3],(int)property->value.integers.integer[4]);
	
	dlog( "looking for state->src_pixel_format: %d", (int)state->src_pixel_format);
	SET_PREFERRED_PIXEL_FORMAT(state->src_pixel_format)
	
	dlog( "looking for kFskBitmapFormatYUV420");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420)

	dlog( "looking for kFskBitmapFormatUYVY");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatUYVY)
	
	dlog( "looking for kFskBitmapFormatYUV420spvu");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420spvu)
	
	dlog( "looking for kFskBitmapFormatYUV420spuv");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420spuv)
	
	dlog( "looking for kFskBitmapFormatYUV420i");
	SET_PREFERRED_PIXEL_FORMAT(kFskBitmapFormatYUV420i)
	
	if( prefered_yuvFormat != kFskBitmapFormatUnknown )
	{
		dlog( "got matched system preferred: %d", (int)prefered_yuvFormat);
		state->dst_pixel_format = prefered_yuvFormat;
	}
	else
	{
		dlog( "no matched system preferred, use default kFskBitmapFormatYUV420");
		state->dst_pixel_format = kFskBitmapFormatYUV420;
	}

	//state->dst_pixel_format = kFskBitmapFormatUYVY;
		
	mlog( "yuv420DecodeSetPreferredPixelFormat, set state->dst_pixel_format: %d", (int)state->dst_pixel_format);

	    
	return kFskErrNone;
}
