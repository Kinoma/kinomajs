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

#include "FskJPEGEncode.h"

#include "FskBlit.h"
#include "FskEndian.h"
#include "FskFiles.h"
#include "FskImage.h"
#include "FskUtilities.h"
#include "FskArch.h"

#if TARGET_OS_MAC
#include "jinclude.h"
#endif

#include "jpeglib.h"
#include "jerror.h"
#include "stddef.h"
#include "setjmp.h"

#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
FskInstrumentedTypeRecord gFskJPEGEncodeTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "FskJPEGEncode"};
#define klog(...)  do { FskInstrumentedTypePrintfDebug  (&gFskJPEGEncodeTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define klog(...)
#endif

#ifndef SUPPORT_YUV422_ENCODE
	#define SUPPORT_YUV422_ENCODE 0
#endif

static void fsk_init_destination(j_compress_ptr cinfo);
static boolean fsk_empty_output_buffer(j_compress_ptr cinfo);
static void fsk_term_destination(j_compress_ptr cinfo);
void boomC(j_common_ptr cinfo);

typedef struct {
	struct jpeg_destination_mgr		dst;				// must be first!
	struct jpeg_compress_struct		cinfo;
	struct jpeg_error_mgr			errmgr;
	jmp_buf							setjmp_buffer;

	unsigned char					*buffer;
	FskFile							fref;
	SInt32							jpegOutputSize;
	unsigned char					*jpegOutput;

	FskBitmap						scratch;

	double							quality;
} FskJPEGEncodeStateRecord, *FskJPEGEncodeState;

static FskErr jpegEncodeCommon(FskBitmap bits, FskJPEGEncodeState state);

static FskErr jpegEncodeGetQuality(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegEncodeSetQuality(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord jpegEncodeProperties[] = {
	{kFskMediaPropertyQuality,			kFskMediaPropertyTypeFloat,		jpegEncodeGetQuality,			jpegEncodeSetQuality},
	{kFskMediaPropertyUndefined,		kFskMediaPropertyTypeUndefined,	NULL,							NULL}
};

FskErr jpegEncodeCanHandle(UInt32 format, const char *mime, Boolean *canHandle)
{
    klog("into jpegEncodeCanHandle, format: %d, mime: %s", (int)format, mime );

	*canHandle =	('jpeg' == format) ||
	(mime && (0 == FskStrCompare(mime, "image/jpeg")));

	return kFskErrNone;
}

FskErr jpegEncodeNew(FskImageCompress comp)
{
	FskErr err;
	FskJPEGEncodeState state;
    
    klog("into jpegEncodeNew");

	err = FskMemPtrNewClear(sizeof(FskJPEGEncodeStateRecord), (FskMemPtr*)(void*)(&comp->state));
	if (err) return err;

	state = comp->state;

	state->cinfo.err = jpeg_std_error(&state->errmgr);
	state->cinfo.err->error_exit = boomC;

	config_ifast_dct(FSK_ARCH_AUTO);

	jpeg_create_compress(&state->cinfo);

	state->cinfo.dest = &state->dst;
	state->dst.init_destination = fsk_init_destination;
	state->dst.empty_output_buffer = fsk_empty_output_buffer;
	state->dst.term_destination = fsk_term_destination;

	state->quality = 0.5;

	return err;
}

FskErr jpegEncodeDispose(void *stateIn, FskImageCompress comp)
{
	FskJPEGEncodeState state = stateIn;

    klog("into jpegEncodeDispose");

	if (state) {
		jpeg_destroy_compress(&state->cinfo);
		FskBitmapDispose(state->scratch);
		FskMemPtrDispose(comp->state);
	}
	return kFskErrNone;
}

FskErr jpegEncodeCompressFrame(void *stateIn, FskImageCompress comp, FskBitmap bits, const void **data, UInt32 *dataSize, UInt32 *frameDuration, UInt32 *compositionTimeOffset, UInt32 *frameType)
{
	FskJPEGEncodeState state = stateIn;
	FskErr err = kFskErrNone;
	Boolean needScratch;
	FskBitmapFormatEnum pixelFormat;
	FskRectangleRecord bounds;
    
    klog("into jpegEncodeCompressFrame");

	if (bits == NULL)
		goto bail;

	FskBitmapGetPixelFormat(bits, &pixelFormat);
	FskBitmapGetBounds(bits, &bounds);
    
    klog("pixelFormat: %d, bounds.width: %d, bounds.height: %d", (int)pixelFormat, (int)bounds.width, (int)bounds.height);
    
    switch (pixelFormat) {
        case kFskBitmapFormat24BGR:
        case kFskBitmapFormat16RGB565LE:
        case kFskBitmapFormat8G:
#if SUPPORT_YUV422_ENCODE
        case kFskBitmapFormatYUV422:
#endif
        case kFskBitmapFormat32ARGB:
        case kFskBitmapFormat32BGRA:
        case kFskBitmapFormat32ABGR:
        case kFskBitmapFormat32RGBA:
            needScratch = false;
            break;
        default:
            needScratch = true;
            break;
    }

	if (needScratch || (((SInt32)comp->width != bounds.width) || ((SInt32)comp->height != bounds.height))) {
		if (NULL == state->scratch) {
			err = FskBitmapNew(comp->width, comp->height, kFskBitmapFormatDefault, &state->scratch);
			if (err) goto bail;
		}
	}
	else {
		if (NULL != state->scratch) {
			FskBitmapDispose(state->scratch);
			state->scratch = NULL;
		}
	}

	if (NULL != state->scratch) {
		FskRectangleRecord scratchBounds;

		FskBitmapGetBounds(state->scratch, &scratchBounds);
		err = FskBitmapDraw(bits, &bounds, state->scratch, &scratchBounds, NULL, NULL, kFskGraphicsModeCopy, NULL);
		if (err) goto bail;

		bits = state->scratch;
	}

	err = jpegEncodeCommon(bits, state);
	if (err) goto bail;

	*data = state->jpegOutput;
	*dataSize = state->jpegOutputSize;
	state->jpegOutput = NULL;
    
    klog("dataSize: %d", *dataSize);

bail:
	FskMemPtrDispose(state->jpegOutput);

	return err;
}

FskErr jpegEncodeGetQuality(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskJPEGEncodeState state = stateIn;

    klog("into jpegEncodeGetQuality, state->quality: %d", (int)state->quality);
    
	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = state->quality;

	return kFskErrNone;
}

FskErr jpegEncodeSetQuality(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskJPEGEncodeState state = stateIn;
	double quality;

    klog("into jpegEncodeSetQuality, property->value.number: %d", (int)property->value.number);

	quality = property->value.number;
	if (quality < 0.0)
		quality  = 0;
	else
		if (quality > 1.0)
			quality = 1.0;
	state->quality = quality;

	return kFskErrNone;
}

#define kMaxLines (8)

FskErr jpegEncodeCommon(FskBitmap bits, FskJPEGEncodeState state)
{
	FskErr err = kFskErrNone;
	struct jpeg_compress_struct *cinfo = &state->cinfo;
	JSAMPROW row_pointer[kMaxLines];
	SInt32 rowBytes, i;
	unsigned char *srcScan = NULL, *dstScan = NULL;
	FskRectangleRecord bounds;
	FskBitmapFormatEnum pixelFormat;

    klog("into jpegEncodeCommon");
    
	if (0 != setjmp(state->setjmp_buffer)) {
		// welcome to the failure case
		err = kFskErrMemFull;
		goto bail;
	}

	state->buffer = NULL;
	state->jpegOutputSize = 0;
	state->jpegOutput = NULL;

	FskBitmapGetBounds(bits, &bounds);
	FskBitmapReadBegin(bits, (const void**)(const void*)(&srcScan), &rowBytes, &pixelFormat);

	cinfo->image_width = bounds.width;
	cinfo->image_height = bounds.height;
	cinfo->input_components = 3;

	cinfo->in_color_space = (kFskBitmapFormat32RGBA == pixelFormat) ? JCS_RGBA : JCS_RGB;
#if SUPPORT_YUV422_ENCODE
    if (kFskBitmapFormatYUV422 == pixelFormat)
		cinfo->in_color_space = JCS_YCbCr;
#endif

	jpeg_set_defaults(cinfo);

	//	cinfo->comp_info[0].h_samp_factor = 1;		// 2 for 4:2:2, 1 for 4:4:4
	//	cinfo->comp_info[0].v_samp_factor = 1;
	//	cinfo->comp_info[1].h_samp_factor = 1;
	//	cinfo->comp_info[1].v_samp_factor = 1;
	//	cinfo->comp_info[2].h_samp_factor = 1;
	//	cinfo->comp_info[2].v_samp_factor = 1;
	cinfo->dct_method = JDCT_IFAST;

	jpeg_set_quality(cinfo, (int)(state->quality * 100.0), false);

	err = FskMemPtrNew(cinfo->image_width * 3 * kMaxLines, (FskMemPtr *)&dstScan);
	if (err) goto bail;

    for (i = 0; i < kMaxLines; i++)
        row_pointer[i] = dstScan + cinfo->image_width * 3 * i;

	jpeg_start_compress(cinfo, TRUE);

	while (cinfo->next_scanline < cinfo->image_height) {
		SInt32 width;
		unsigned char *dst = dstScan;
        JDIMENSION lines = kMaxLines, i;
        if (lines > (cinfo->image_height - cinfo->next_scanline))
            lines = cinfo->image_height - cinfo->next_scanline;
        
        for (i = 0; i < lines; i++) {
            unsigned char *src = srcScan;

            switch (pixelFormat) {
                case kFskBitmapFormat24BGR:
                    for (width = cinfo->image_width; width--; dst += 3) {
                        dst[2] = *src++;
                        dst[1] = *src++;
                        dst[0] = *src++;
                    }
                    break;

                case kFskBitmapFormat16RGB565LE:
                    for (width = cinfo->image_width; width--; src += 2, dst += 3) {
                        UInt16 pixel = *(UInt16 *)src;
                        UInt8 comp;
                        comp = pixel >> 11; dst[0] = (comp << 3) | (comp >> 2);
                        comp = pixel >> 5; dst[1] = (comp << 2) | (comp >> 4);
                        comp = pixel & 0x1f; dst[2] = (comp << 3) | (comp >> 2);
                    }
                    break;

                case kFskBitmapFormat8G:
                    for (width = cinfo->image_width; width--; dst += 3) {
                        unsigned char c = *src++;
                        dst[0] = c;
                        dst[1] = c;
                        dst[2] = c;
                    }
                    break;

    #if SUPPORT_YUV422_ENCODE
                case kFskBitmapFormatYUV422:
                    for (width = cinfo->image_width / 2; width--; dst += 6) {
                        dst[1] = *src++;
                        dst[0] = *src++;
                        dst[2] = *src++;
                        dst[3] = *src++;
                        dst[4] = dst[1];
                        dst[5] = dst[2];
                    }
                    break;
    #endif
                case kFskBitmapFormat32ARGB:
                    for (width = cinfo->image_width; width--; dst += 3) {
                        src++;
                        dst[0] = *src++;
                        dst[1] = *src++;
                        dst[2] = *src++;
                    }
                    break;

                case kFskBitmapFormat32BGRA:
                    for (width = cinfo->image_width; width--; dst += 3) {
                        dst[2] = *src++;
                        dst[1] = *src++;
                        dst[0] = *src++;
                        src++;
                    }
                    break;

                case kFskBitmapFormat32ABGR:
                    for (width = cinfo->image_width; width--; dst += 3) {
                        src++;
                        dst[2] = *src++;
                        dst[1] = *src++;
                        dst[0] = *src++;
                    }
                    break;

                case kFskBitmapFormat32RGBA:
                    row_pointer[i] = src;       // pass scan lines through directly
                    break;

                default:
                    BAIL(kFskErrUnsupportedPixelType);
                    break;
            }
            srcScan += rowBytes;
        }

	    jpeg_write_scanlines(cinfo, row_pointer, lines);
	}

	jpeg_finish_compress(cinfo);

bail:
	FskMemPtrDispose(dstScan);

	if (NULL != srcScan)
		FskBitmapReadEnd(bits);

	return err;
}

void boomC(j_common_ptr cinfo)
{
	FskJPEGEncodeState state = (FskJPEGEncodeState)(((char *)cinfo->err) - offsetof(FskJPEGEncodeStateRecord, errmgr));
	_longjmp(state->setjmp_buffer, 1);
}

#define kBufferSize (4096)

void fsk_init_destination(j_compress_ptr cinfo)
{
	FskJPEGEncodeState state = (FskJPEGEncodeState)cinfo->dest;

	if (kFskErrNone != FskMemPtrNew(kBufferSize, (FskMemPtr *)&state->buffer))
		boomC((j_common_ptr)cinfo);

	state->dst.next_output_byte = state->buffer;
	state->dst.free_in_buffer = kBufferSize;
}

boolean fsk_empty_output_buffer(j_compress_ptr cinfo)
{
	FskJPEGEncodeState state = (FskJPEGEncodeState)cinfo->dest;

	if (NULL != state->fref)
		FskFileWrite(state->fref, kBufferSize, state->buffer, NULL);
	else {
		if (kFskErrNone != FskMemPtrRealloc(state->jpegOutputSize + kBufferSize, &state->jpegOutput))
			boomC((j_common_ptr)cinfo);
		FskMemMove(state->jpegOutputSize  + state->jpegOutput, state->buffer, kBufferSize);
		state->jpegOutputSize  += kBufferSize;
	}

	state->dst.next_output_byte = state->buffer;
	state->dst.free_in_buffer = kBufferSize;

	return true;
}

void fsk_term_destination(j_compress_ptr cinfo)
{
	FskJPEGEncodeState state = (FskJPEGEncodeState)cinfo->dest;
	SInt32 size = kBufferSize - state->dst.free_in_buffer;

	if (NULL != state->fref)
		FskFileWrite(state->fref, size, state->buffer, NULL);
	else {
		if (kFskErrNone != FskMemPtrRealloc(state->jpegOutputSize + size, &state->jpegOutput))
			boomC((j_common_ptr)cinfo);
		FskMemMove(state->jpegOutputSize  + state->jpegOutput, state->buffer, size);
		state->jpegOutputSize  += size;
	}

	FskMemPtrDisposeAt((void**)(void*)(&state->buffer));

}
