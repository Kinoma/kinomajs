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

#include "FskJPEGDecode.h"

#include "FskBlit.h"
#include "FskEndian.h"
#include "FskFiles.h"
#include "FskImage.h"
#include "FskPort.h"
#include "FskUtilities.h"
#include "FskArch.h"

#if FSKBITMAP_OPENGL
	#define SUPPORT_SCALING 1
	#include "FskGLBlit.h"
    #include "FskAAScaleBitmap.h"
#else
	#define SUPPORT_SCALING 0
#endif

// We #define JPEG_INTERNALS to gain access to the RGB component index values defined by libjpeg.
// The RGB_RED, RGB_GREEN and RGB_BLUE component values aren't necessarily 0, 1 and 2.
// The Unity (ThreadX) libjpeg we link against defines different values.
#define JPEG_INTERNALS

#if TARGET_OS_MAC
	#include "jinclude.h"
#endif

#include "jpeglib.h"
#include "jerror.h"
#include "stddef.h"
#include "setjmp.h"

#include "FskEXIFScan.h"


#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(jpegdec, jpegdec);
#define mlog  FskjpegdecPrintfMinimal
#define nlog  FskjpegdecPrintfNormal
#define vlog  FskjpegdecPrintfVerbose
#define dlog  FskjpegdecPrintfDebug


FskErr jpegDecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
    *canHandle =	('jpeg' == format) ||
					(mime && ((0 == FskStrCompare(mime, "image/jpeg")) || (0 == FskStrCompare(mime, "image/jpg")))) ||
					(extension && ((0 == FskStrCompare(extension, "jpeg")) || (0 == FskStrCompare(extension, "jpg"))));

	return kFskErrNone;
}

static void fsk_init_source(j_decompress_ptr cinfo);
static boolean fsk_fill_input_buffer(j_decompress_ptr cinfo);
static void fsk_skip_input_data(j_decompress_ptr cinfo, long num_bytes);
static void fsk_term_source(j_decompress_ptr cinfo);

static void fsk_init_source_spooler(j_decompress_ptr cinfo);
static boolean fsk_fill_input_buffer_spooler(j_decompress_ptr cinfo);
static void fsk_skip_input_data_spooler(j_decompress_ptr cinfo, long num_bytes);
static void fsk_term_source_spooler(j_decompress_ptr cinfo);

typedef struct FskJPEGDecodeStateRecord FskJPEGDecodeStateRecord;
typedef struct FskJPEGDecodeStateRecord *FskJPEGDecodeState;

typedef void(*jpegCopy)(FskJPEGDecodeState state, UInt32 width, unsigned char *src, unsigned char *dst);

struct FskJPEGDecodeStateRecord {
	struct jpeg_decompress_struct	cinfo;
	struct jpeg_error_mgr			errmgr;
	struct jpeg_source_mgr			src;
	jmp_buf							setjmp_buffer;
	void							*disposeIfLongJmp;
	UInt32							pixelFormat;
	jpegCopy						colorCopy;
	jpegCopy						grayCopy;
	FskMediaSpooler					spooler;
	FskInt64						spoolerPosition;
	Boolean							spoolerOpen;

	// scanned metadata
	Boolean							haveMetadata;
	unsigned char					*thumbnail;
	UInt32							thumbnailLen;

	UInt32							orientation;
	FskDimensionRecord				dimensions;
	UInt32							depth;

	FskEXIFScan						exif0Metadata;
	FskEXIFScan						exif1Metadata;

	Boolean							decompressing;
};

static FskErr jpegDecodeSetPixelFormat(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegDecodeGetPixelFormat(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegDecodeSetSpooler(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegDecodeGetBuffer(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegDecodeGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord jpegDecodeProperties[] = {
	{kFskMediaPropertyPixelFormat,		kFskMediaPropertyTypeUInt32List,	jpegDecodeGetPixelFormat,	jpegDecodeSetPixelFormat},
	{kFskMediaPropertySpooler,			kFskMediaPropertyTypeSpooler,		NULL,						jpegDecodeSetSpooler},
	{kFskMediaPropertyBuffer,			kFskMediaPropertyTypeFloat,			jpegDecodeGetBuffer,		NULL},
	{kFskMediaPropertyDLNASinks,		kFskMediaPropertyTypeStringList,	jpegDecodeGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,		kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

// pixel massaging
#define declareJPEGCopy(name) static void name(FskJPEGDecodeState state, UInt32 width, unsigned char *src, unsigned char *dst)
declareJPEGCopy(jpegColorTo24BGR);
declareJPEGCopy(jpegGrayTo24BGR);
declareJPEGCopy(jpegColorTo32ABGR);
declareJPEGCopy(jpegGrayTo32ABGR);
declareJPEGCopy(jpegColorTo32ARGB);
declareJPEGCopy(jpegGrayTo32ARGB);
declareJPEGCopy(jpegColorTo32BGRA);
declareJPEGCopy(jpegGrayTo32BGRA);
declareJPEGCopy(jpegColorTo32RGBA);
declareJPEGCopy(jpegGrayTo32RGBA);
#if TARGET_RT_LITTLE_ENDIAN
	declareJPEGCopy(jpegColorTo16RGB565LE);
	declareJPEGCopy(jpegGrayTo16RGB565LE);
	declareJPEGCopy(jpegColorTo16RGBA4444LE);
	declareJPEGCopy(jpegGrayTo16RGBA4444LE);
#endif
declareJPEGCopy(jpegGrayTo8G);

static boolean exifMarkerHandler(j_decompress_ptr cinfo);

#if SUPPORT_SCALING
	typedef struct {
		unsigned char		*pixels;
		SInt32				rowBytes;
		FskRectangleRecord	fit;
	} getOutputLineRecord;

	static void *getOutputLine(void *userData, Boolean done);
#endif

// ijg error handling patches
static void boom(j_common_ptr cinfo);
static void output_message(j_common_ptr cinfo);

static void setupSource(FskJPEGDecodeState state, FskMediaSpooler spooler, const void *jpegBits, UInt32 jpegBitsLen);

void boom(j_common_ptr cinfo)
{
	FskJPEGDecodeState state = (FskJPEGDecodeState)(((char *)cinfo->err) - offsetof(FskJPEGDecodeStateRecord, errmgr));
	_longjmp(state->setjmp_buffer, 1);
}

void output_message(j_common_ptr cinfo)
{
}


FskErr jpegDecodeSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
    if (dataSize >= 3) {
		if ((0xff == data[0]) && (0xd8 == data[1]) && (0xff == data[2])) {
			*mime = FskStrDoCopy("image/jpeg");
			return kFskErrNone;
		}
	}

	return kFskErrUnknownElement;
}

FskErr jpegDecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension)
{
	FskErr err;
	FskJPEGDecodeState state;
	FskMediaPropertyValueRecord property;
    FskBitmapFormatEnum pixelFormat;
    UInt32 pixelFormatUInt32;
    
	err = FskMemPtrNewClear(sizeof(FskJPEGDecodeStateRecord), (FskMemPtr*)(void*)&deco->state);
	if (err) return err;

	state = deco->state;

	state->cinfo.err = jpeg_std_error(&state->errmgr);
	state->cinfo.err->error_exit = boom;
	state->cinfo.err->output_message = output_message;

	config_ifast_idct(FSK_ARCH_AUTO);

	jpeg_create_decompress(&state->cinfo);

	state->cinfo.client_data = state;

	state->cinfo.src = &state->src;
	state->src.init_source = fsk_init_source;
	state->src.term_source = fsk_term_source;
	state->src.skip_input_data = fsk_skip_input_data;
	state->src.fill_input_buffer= fsk_fill_input_buffer;
	state->src.resync_to_restart = jpeg_resync_to_restart;

	jpeg_set_marker_processor(&state->cinfo, 0xe1, exifMarkerHandler);

	FskPortPreferredRGBFormatsGet(NULL, &pixelFormat, NULL);
    pixelFormatUInt32 = (UInt32)pixelFormat;
	property.type = kFskMediaPropertyTypeUInt32List;
    property.value.integers.count = 1;
    property.value.integers.integer = &pixelFormatUInt32;
	return jpegDecodeSetPixelFormat(state, NULL, kFskMediaPropertyPixelFormat, &property);
}

FskErr jpegDecodeDispose(void *stateIn, FskImageDecompress deco)
{
	FskJPEGDecodeState state = stateIn;
    
	if (state) {
		jpeg_destroy_decompress(&state->cinfo);
		FskMemPtrDispose(state->spooler);
		FskMemPtrDispose(state->thumbnail);
		FskEXIFScanDisposeAt(&state->exif0Metadata);
		FskEXIFScanDisposeAt(&state->exif1Metadata);
		FskMemPtrDispose(deco->state);
	}
	return kFskErrNone;
}


FskErr jpegDecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *jpegBits, UInt32 jpegBitsLen, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	FskJPEGDecodeState state = stateIn;
	FskErr err = kFskErrNone;
	struct jpeg_decompress_struct *cinfo = &state->cinfo;
	JSAMPARRAY scanBuf;
	FskBitmap bits = NULL;
	SInt32 dstRowBytes, srcRowBytes, maxSrcRows;
	unsigned char *dstScan = NULL;
	Boolean gray;
	jpegCopy copy;
	unsigned char *y0 = NULL, *y1 = NULL, *u = NULL, *v = NULL;
	unsigned long yRowBump = 0, uvRowBump = 0;
#if SUPPORT_SCALING
	FskAAScaler scaler = NULL;
	getOutputLineRecord scalerData;
#endif

	state->decompressing = true;

	if (state->spooler)
		state->spooler->flags |= kFskMediaSpoolerForwardOnly;
	setupSource(state, state->spooler, jpegBits, jpegBitsLen);

	if (0 != _setjmp(state->setjmp_buffer)) {
		// welcome to the failure case
		err = kFskErrBadData;

		FskMemPtrDisposeAt((void **)&state->disposeIfLongJmp);

		goto bail;
	}

	jpeg_read_header(cinfo, TRUE);

	cinfo->dct_method = JDCT_IFAST;
	cinfo->do_fancy_upsampling = 0;
	cinfo->do_block_smoothing = 0;

	if (deco->requestedWidth && deco->requestedHeight) {
		while (((cinfo->image_height / (cinfo->scale_denom * 2)) >= deco->requestedHeight) &&
				((cinfo->image_width / (cinfo->scale_denom * 2)) >= deco->requestedWidth)) {
			cinfo->scale_denom *= 2;
			if (8 == cinfo->scale_denom)
				break;
		}
	}

	if ((JCS_CMYK == cinfo->jpeg_color_space) || (JCS_YCCK == cinfo->jpeg_color_space))
		cinfo->out_color_space = JCS_CMYK;		// we'll convert to RGB or Gray below
	else
	if (kFskBitmapFormat8G == state->pixelFormat)
		cinfo->out_color_space = JCS_GRAYSCALE;
	else
	if ((JCS_GRAYSCALE != cinfo->jpeg_color_space) && ((kFskBitmapFormatYUV420 == state->pixelFormat) || (kFskBitmapFormatYUV420i == state->pixelFormat)))
		cinfo->out_color_space = JCS_YCbCr;		// YUV 4:4:4 interleaved
#if TARGET_RT_LITTLE_ENDIAN
	else
	if ((JCS_GRAYSCALE != cinfo->jpeg_color_space) && (kFskBitmapFormat16RGB565LE == state->pixelFormat))
		cinfo->out_color_space = JCS_RGB565;
#endif

	jpeg_start_decompress(cinfo);

#if SUPPORT_SCALING
	{
	SInt32 maxDimension = FskGLMaximumTextureDimension();
	FskBitmapFormatEnum srcPixelFormat = kFskBitmapFormatUnknown;
	if (JCS_RGB == cinfo->out_color_space)
		srcPixelFormat = kFskBitmapFormat24RGB;
	else if (JCS_GRAYSCALE == cinfo->out_color_space)
		srcPixelFormat = kFskBitmapFormat8G;
	else if (JCS_RGB565 == cinfo->out_color_space)
		srcPixelFormat = kFskBitmapFormat16RGB565LE;
	if (maxDimension && (srcPixelFormat != kFskBitmapFormatUnknown) && (((SInt32)cinfo->output_width > maxDimension) || ((SInt32)cinfo->output_height > maxDimension))) {
		FskRectangleRecord containing, containee;
		FskRectangleSet(&containing, 0, 0, maxDimension, maxDimension);
		FskRectangleSet(&containee, 0, 0, cinfo->output_width, cinfo->output_height);
		FskRectangleScaleToFit(&containing, &containee, &scalerData.fit);
		FskAAScalerNew(kAAScaleTentKernelType,
					   cinfo->output_width, cinfo->output_height, srcPixelFormat, NULL,
					   scalerData.fit.width, scalerData.fit.height, state->pixelFormat, getOutputLine,
					   0, 0, 0, 0,
					   &scalerData,
					   &scaler);
	}
	}
#endif

	gray = 1 == cinfo->out_color_components;

	if (deco->bits) {
		FskRectangleRecord bounds;
		FskBitmapGetBounds(deco->bits, &bounds);
		if (((SInt32)cinfo->output_width != bounds.width) || ((SInt32)cinfo->output_height != bounds.height)) {
			err = kFskErrBadData;
			goto bail;
		}
		bits = deco->bits;
	}
	else {
#if SUPPORT_SCALING
		err = FskBitmapNew(scaler ? (SInt32)scalerData.fit.width : (SInt32)cinfo->output_width, scaler ? (SInt32)scalerData.fit.height : (SInt32)cinfo->output_height,
				state->pixelFormat, &bits);
#else
		err = FskBitmapNew(cinfo->output_width, cinfo->output_height,
						   state->pixelFormat, &bits);
#endif
		if (err) goto bail;
	}

	srcRowBytes = cinfo->output_width * cinfo->output_components;
	maxSrcRows = cinfo->rec_outbuf_height;
	if ((maxSrcRows < 2) && (JCS_YCbCr == cinfo->out_color_space))
		maxSrcRows = 2;
	scanBuf = (*cinfo->mem->alloc_sarray)((j_common_ptr) cinfo, JPOOL_IMAGE, srcRowBytes, maxSrcRows);

	FskBitmapWriteBegin(bits, (void**)(void*)&dstScan, &dstRowBytes, NULL);

#if SUPPORT_SCALING
	if (scaler) {
		scalerData.pixels = dstScan;
		scalerData.rowBytes = dstRowBytes;
	}
#endif

	if (JCS_YCbCr == cinfo->out_color_space) {
		y0 = dstScan;
		if (kFskBitmapFormatYUV420 == state->pixelFormat) {
			y1 = y0 + dstRowBytes;
			u = y0 + (dstRowBytes * cinfo->output_height);
			v = u + ((dstRowBytes >> 1) * (cinfo->output_height >> 1));

			yRowBump = (dstRowBytes << 1) - (cinfo->output_width + (cinfo->output_width & 1));
			uvRowBump = (dstRowBytes - cinfo->output_width) >> 1;
		}
		else
			yRowBump = dstRowBytes - (cinfo->output_width * 3);
	}

	if (gray)
		copy = state->grayCopy;
	else
		copy = state->colorCopy;

	while (cinfo->output_scanline < cinfo->output_height) {
		unsigned char *src = scanBuf[0];
		JDIMENSION scanLines;

		if (JCS_YCbCr == cinfo->out_color_space) {
			JDIMENSION i;
			unsigned char *in0 = src, *in1 = src + srcRowBytes;
			jpeg_read_scanlines(cinfo, &in0, 1);
			jpeg_read_scanlines(cinfo, &in1, 1);

			i = cinfo->output_width >> 1;
			if (kFskBitmapFormatYUV420 == state->pixelFormat) {
				while (i--) {
					// sub-sample a 2x2 yuv444 cell to planar YUV420
					int ua, va;
					y0[0] = in0[0];
					ua = in0[1]; va = in0[2];

					y0[1] = in0[3];
					ua += in0[4]; va += in0[5];

					y0 += 2; in0 += 6;

					y1[0] = in1[0];
					ua += in1[1]; va += in1[2];

					y1[1] = in1[3];
					ua += in1[4]; va += in1[5];

					y1 += 2; in1 += 6;

					*u++ = ua >> 2;
					*v++ = va >> 2;
				}

				if (1 & cinfo->output_width) {
					// sub-sample a 2x1 yuv444 cell to planar YUV420
					int ua, va;
					y0[0] = in0[0];
					y0[1] = in0[0];
					ua = in0[1]; va = in0[2];

					y0 += 2;

					y1[0] = in1[0];
					y1[1] = in1[0];
					ua += in1[1]; va += in1[2];

					y1 += 2;

					*u++ = ua >> 1;
					*v++ = va >> 1;
				}

				y0 += yRowBump;
				y1 += yRowBump;
				u += uvRowBump;
				v += uvRowBump;
			}
			else {
				while (i--) {
					// sub-sample a 2x2 yuv444 cell to YUV420i
					int ua, va;

					y0[2] = in0[0];
					ua = in0[1]; va = in0[2];

					y0[3] = in0[3];
					ua += in0[4]; va += in0[5];

					y0[4] = in1[0];
					ua += in1[1]; va += in1[2];

					y0[5] = in1[3];
					ua += in1[4]; va += in1[5];

					in0 += 6; in1 += 6;

					y0[0] = ua >> 2;
					y0[1] = va >> 2;

					y0 += 6;
				}

				y0 += yRowBump;
			}

			continue;
		}

		scanLines = jpeg_read_scanlines(cinfo, scanBuf, maxSrcRows);

		if (JCS_CMYK == cinfo->out_color_space) {
			// color convert in place
			JDIMENSION i = scanLines;
			unsigned char *s = src;
			unsigned char *d = src;

			while (i--) {
				JDIMENSION j = cinfo->output_width;

				if (kFskBitmapFormat8G != state->pixelFormat) {
					while (j--) {
						UInt8 c, m, y, k;

						c = *s++;
						m = *s++;
						y = *s++;
						k = *s++;

#if TARGET_RT_LITTLE_ENDIAN
						*(UInt16 *)d = ((((c * k) / 255) >> 3) << 11) | ((((m * k) / 255) >> 2) << 5) | ((((y * k) / 255) >> 3) << 0);
						d += 2;

#else
						*d++ = (c * k) / 255;
						*d++ = (m * k) / 255;
						*d++ = (y * k) / 255;
#endif
					}
				}
				else {
					while (j--) {
						*d++ = s[3];
						s += 4;
					}
				}

				s += srcRowBytes;
				d += dstRowBytes;
			}
		}

		while (scanLines--) {
#if SUPPORT_SCALING
			if (scaler)
				FskAAScalerNextLine(scaler, src);
			else
#endif
			{
				(copy)((void *)state, cinfo->output_width, src, dstScan);
				dstScan += dstRowBytes;
			}
			src += srcRowBytes;
		}
	}

	jpeg_finish_decompress(cinfo);

bail:
	state->decompressing = false;

	fsk_term_source_spooler(cinfo);

#if SUPPORT_SCALING
	FskAAScalerDispose(scaler);
#endif

	if (NULL != dstScan)
		FskBitmapWriteEnd(bits);

	if (kFskErrNone != err) {
		FskBitmapDispose(bits);
		bits = NULL;
	}

	deco->bits = bits;

	return err;
}

FskErr jpegDecodeSetData(void *stateIn, FskImageDecompress deco, void *data, UInt32 dataSize)
{
	FskJPEGDecodeState state = stateIn;

	state->haveMetadata = false;

	FskEXIFScanDisposeAt(&state->exif0Metadata);
	FskEXIFScanDisposeAt(&state->exif1Metadata);

	return kFskErrNone;
}

FskErr jpegDecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	FskErr err = kFskErrNone;
	FskJPEGDecodeState state = stateIn;
	struct jpeg_decompress_struct *cinfo = &state->cinfo;
    
	if (false == state->haveMetadata) {
		if (state->spooler)
			state->spooler->flags &= ~kFskMediaSpoolerForwardOnly;
		setupSource(state, state->spooler, deco->data, deco->dataSize);

		if (0 != _setjmp(state->setjmp_buffer)) {
			err = kFskErrBadData;

			FskMemPtrDisposeAt((void **)&state->disposeIfLongJmp);

			goto bail;
		}

		jpeg_read_header(cinfo, TRUE);

		state->haveMetadata = true;
		state->dimensions.width = cinfo->image_width;
		state->dimensions.height = cinfo->image_height;
		state->depth = (1 == cinfo->out_color_components) ? 8 : 24;
	}

	switch (metadata) {
		case kFskImageDecompressMetaDataDimensions:
			value->type = kFskMediaPropertyTypeDimension;
			value->value.dimension = state->dimensions;
			break;

		case kFskImageDecompressMetaDataBitDepth:
			value->type = kFskMediaPropertyTypeInteger;
			value->value.integer = state->depth;
			break;

		case kFskImageDecompressMetaDataThumbnail:
			if (state->thumbnail) {
				value->type = kFskMediaPropertyTypeImage;
				value->value.data.dataSize = state->thumbnailLen;
				err = FskMemPtrNewFromData(state->thumbnailLen, state->thumbnail, (FskMemPtr*)(void*)&value->value.data.data);
			}
			else
				err = kFskErrUnknownElement;
			break;

		case kFskImageDecompressMetaDataOrientation:
			if (kFskUInt32Max != state->orientation) {
				value->type = kFskMediaPropertyTypeInteger;
				value->value.integer = state->orientation;
			}
			else
				err = kFskErrUnknownElement;
			break;

		case kFskImageDecompressMetaDataHeadersParsed:
			value->type = kFskMediaPropertyTypeBoolean;
			value->value.b = true;
			break;

		default:
			err = kFskErrUnknownElement;

			if (kFskImageMetdataExifMainImage & metadata) {
				FskEXIFScan ei;
				FskMediaPropertyValue vp;

				if (0x01000000 & metadata)
					ei = state->exif1Metadata;
				else
					ei = state->exif0Metadata;

				if (NULL != ei) {
					err = FskEXIFScanFindTagValue(ei, metadata, &vp);
					if (kFskErrNone == err)
						err = FskMediaPropertyCopy(vp, value);
				}
			}
			break;
		}

bail:
	jpeg_abort_decompress(cinfo);

	fsk_term_source_spooler(cinfo);

	return err;
}

FskErr jpegDecodeSetPixelFormat(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskJPEGDecodeState state = stateIn;
	FskErr err = kFskErrNone;
    
	switch (property->value.integers.integer[0]) {
		case kFskBitmapFormat24BGR:
			state->colorCopy = jpegColorTo24BGR;
			state->grayCopy = jpegGrayTo24BGR;
			break;

		case kFskBitmapFormat32ABGR:
			state->colorCopy = jpegColorTo32ABGR;
			state->grayCopy = jpegGrayTo32ABGR;
			break;

		case kFskBitmapFormat32ARGB:
			state->colorCopy = jpegColorTo32ARGB;
			state->grayCopy = jpegGrayTo32ARGB;
			break;

		case kFskBitmapFormat32BGRA:
			state->colorCopy = jpegColorTo32BGRA;
			state->grayCopy = jpegGrayTo32BGRA;
			break;

#if TARGET_RT_LITTLE_ENDIAN
		case kFskBitmapFormat16RGB565LE:
			state->colorCopy = jpegColorTo16RGB565LE;
			state->grayCopy = jpegGrayTo16RGB565LE;
			break;

		case kFskBitmapFormat16RGBA4444LE:
			state->colorCopy = jpegColorTo16RGBA4444LE;
			state->grayCopy = jpegGrayTo16RGBA4444LE;
			break;
#endif

		case kFskBitmapFormat8G:
			state->colorCopy = NULL;			// this case never happens
			state->grayCopy = jpegGrayTo8G;
			break;

		case kFskBitmapFormat32RGBA:
			state->colorCopy = jpegColorTo32RGBA;
			state->grayCopy = jpegGrayTo32RGBA;
			break;

		case kFskBitmapFormatYUV420:
		case kFskBitmapFormatYUV420i:
			// no copy procs in this case
			break;

		default:
			err = kFskErrUnsupportedPixelType;
			break;
	}

	if (kFskErrNone == err)
		state->pixelFormat = property->value.integers.integer[0];

	return err;
}

FskErr jpegDecodeGetPixelFormat(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskJPEGDecodeState state = stateIn;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.number = state->pixelFormat;

	return kFskErrNone;
}

FskErr jpegDecodeSetSpooler(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskJPEGDecodeState state = stateIn;

	FskMemPtrDisposeAt((void**)(void*)&state->spooler);
	if (kFskMediaSpoolerValid & property->value.spooler.flags)
		return FskMemPtrNewFromData(sizeof(FskMediaSpoolerRecord), &property->value.spooler, (FskMemPtr*)(void*)&state->spooler);

	return kFskErrNone;
}

FskErr jpegDecodeGetBuffer(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskJPEGDecodeState state = stateIn;
	UInt32 output_height = state->cinfo.output_height;

	if (!state->decompressing || !output_height)
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (double)state->cinfo.output_scanline / (double)output_height;

	return kFskErrNone;
}

FskErr jpegDecodeGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "image/jpeg\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

declareJPEGCopy(jpegColorTo24BGR)
{
	while (width--) {
		*dst++ = src[RGB_BLUE];
		*dst++ = src[RGB_GREEN];
		*dst++ = src[RGB_RED];
		src += RGB_PIXELSIZE;
	}
}

declareJPEGCopy(jpegGrayTo24BGR)
{
	while (width--) {
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src++;
	}
}

declareJPEGCopy(jpegColorTo32ABGR)
{
	/* RGB_ values come from libjpeg's jmorecfg.h */
	while (width--) {
		*dst++ = 255;
		*dst++ = src[RGB_BLUE];
		*dst++ = src[RGB_GREEN];
		*dst++ = src[RGB_RED];
		src += RGB_PIXELSIZE;
	}
}

declareJPEGCopy(jpegGrayTo32ABGR)
{
	while (width--) {
		*dst++ = 255;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src++;
	}
}

declareJPEGCopy(jpegColorTo32ARGB)
{
	while (width--) {
		*dst++ = 255;
		*dst++ = src[RGB_RED];
		*dst++ = src[RGB_GREEN];
		*dst++ = src[RGB_BLUE];
		src += RGB_PIXELSIZE;
	}
}

declareJPEGCopy(jpegGrayTo32ARGB)
{
	while (width--) {
		*dst++ = 255;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src++;
	}
}

declareJPEGCopy(jpegColorTo32BGRA)
{
	while (width--) {
		*dst++ = src[RGB_BLUE];
		*dst++ = src[RGB_GREEN];
		*dst++ = src[RGB_RED];
		*dst++ = 255;
		src += RGB_PIXELSIZE;
	}
}

declareJPEGCopy(jpegGrayTo32BGRA)
{
	while (width--) {
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src++;
		*dst++ = 255;
	}
}

#if TARGET_RT_LITTLE_ENDIAN
declareJPEGCopy(jpegColorTo16RGB565LE)
{
#if 1
	FskMemMove(dst, src, width << 1);
#else
	while (width--) {
		*(UInt16 *)dst = ((src[0] >> 3) << 11) | ((src[1] >> 2) << 5) | ((src[2] >> 3) << 0);
		dst += 2;
		src += 3;
	}
#endif
}

declareJPEGCopy(jpegGrayTo16RGB565LE)
{
	while (width--) {
		unsigned char gray = *src++;
		*(UInt16 *)dst = ((gray >> 3) << 11) | ((gray >> 2) << 5) | ((gray >> 3) << 0);
		dst += 2;
	}
}

declareJPEGCopy(jpegColorTo16RGBA4444LE)
{
	while (width--) {
		*(UInt16 *)dst = ((src[0] >> 4) << 12) | ((src[1] >> 4) << 8) | src[2] | 0x0f;
		dst += 2;
		src += 3;
	}
}

declareJPEGCopy(jpegGrayTo16RGBA4444LE)
{
	while (width--) {
		*(UInt16 *)dst = ((src[0] >> 4) << 12) | ((src[0] >> 4) << 8) | src[0] | 0x0f;
		dst += 2;
		src += 1;
	}
}
#endif

declareJPEGCopy(jpegColorTo32RGBA)
{
	while (width--) {
		*dst++ = src[RGB_RED];
		*dst++ = src[RGB_GREEN];
		*dst++ = src[RGB_BLUE];
		*dst++ = 255;
		src += RGB_PIXELSIZE;
	}
}

declareJPEGCopy(jpegGrayTo32RGBA)
{
	while (width--) {
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src++;
		*dst++ = 255;
	}
}

declareJPEGCopy(jpegGrayTo8G)
{
	while (width--)
		*dst++ = *src++;
}

void fsk_init_source(j_decompress_ptr cinfo)
{
}

boolean fsk_fill_input_buffer(j_decompress_ptr cinfo)
{
	ERREXIT(cinfo, JERR_FILE_READ);
	return false;
}

void fsk_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	if ((size_t)num_bytes > cinfo->src->bytes_in_buffer)
		ERREXIT(cinfo, JERR_FILE_READ);

	cinfo->src->next_input_byte += num_bytes;
	cinfo->src->bytes_in_buffer -= num_bytes;
}

void fsk_term_source(j_decompress_ptr cinfo)
{
}

void fsk_init_source_spooler(j_decompress_ptr cinfo)
{
	FskJPEGDecodeState state = (FskJPEGDecodeState)cinfo->client_data;
	if (NULL != state->spooler->doOpen) {
		if (kFskErrNone != (state->spooler->doOpen)(state->spooler, kFskFilePermissionReadOnly))
			ERREXIT(cinfo, JERR_FILE_READ);
		state->spoolerPosition = 0;
		state->spoolerOpen = true;
	}
}

//@@const UInt32 kSpoolerBufferSize = 65536;
const UInt32 kSpoolerBufferSize = 1024;

boolean fsk_fill_input_buffer_spooler(j_decompress_ptr cinfo)
{
	FskJPEGDecodeState state = (FskJPEGDecodeState)cinfo->client_data;
	UInt32 bytesRead;
	void *data;

	if (kFskErrNone != (state->spooler->doRead)(state->spooler, state->spoolerPosition, kSpoolerBufferSize, &data, &bytesRead)) {
		ERREXIT(cinfo, JERR_FILE_READ);
		return false;
	}

	state->spoolerPosition += bytesRead;

	cinfo->src->next_input_byte = data;
	cinfo->src->bytes_in_buffer = bytesRead;

	return true;
}

void fsk_skip_input_data_spooler(j_decompress_ptr cinfo, long num_bytes)
{
	FskJPEGDecodeState state = (FskJPEGDecodeState)cinfo->client_data;
	if ((size_t)num_bytes <= cinfo->src->bytes_in_buffer) {
		cinfo->src->bytes_in_buffer -= num_bytes;
		cinfo->src->next_input_byte += num_bytes;
	}
	else {
		num_bytes -= cinfo->src->bytes_in_buffer;
		cinfo->src->bytes_in_buffer = 0;
		state->spoolerPosition += num_bytes;
	}
}

void fsk_term_source_spooler(j_decompress_ptr cinfo)
{
	FskJPEGDecodeState state = (FskJPEGDecodeState)cinfo->client_data;
	if (state->spoolerOpen && (NULL != state->spooler->doClose))
		(state->spooler->doClose)(state->spooler);
	state->spoolerOpen = false;
}

void setupSource(FskJPEGDecodeState state, FskMediaSpooler spooler, const void *jpegBits, UInt32 jpegBitsLen)
{
	FskMemPtrDisposeAt((void**)(void*)&state->thumbnail);
	state->thumbnailLen = 0;
	state->orientation = kFskUInt32Max;
	state->depth = 0;
	state->disposeIfLongJmp = NULL;

	if (NULL != spooler) {
		state->src.init_source = fsk_init_source_spooler;
		state->src.term_source = fsk_term_source_spooler;
		state->src.skip_input_data = fsk_skip_input_data_spooler;
		state->src.fill_input_buffer= fsk_fill_input_buffer_spooler;

		state->src.next_input_byte = NULL;
		state->src.bytes_in_buffer = 0;
	}
	else {
		state->src.init_source = fsk_init_source;
		state->src.term_source = fsk_term_source;
		state->src.skip_input_data = fsk_skip_input_data;
		state->src.fill_input_buffer= fsk_fill_input_buffer;

		state->src.next_input_byte = jpegBits;
		state->src.bytes_in_buffer = jpegBitsLen;
	}
}

boolean getByte(j_decompress_ptr cinfo, UInt8 *byte)
{
	struct jpeg_source_mgr *src = cinfo->src;

	if (0 == src->bytes_in_buffer) {
		if (!(src->fill_input_buffer)(cinfo))
			boom((j_common_ptr)cinfo);
	}

	*byte = *(src->next_input_byte);
	src->next_input_byte += 1;
	src->bytes_in_buffer -= 1;

	return true;
}

boolean exifMarkerHandler(j_decompress_ptr cinfo)
{
	FskJPEGDecodeState state = (FskJPEGDecodeState)cinfo->client_data;
	struct jpeg_source_mgr *src = cinfo->src;
	UInt32 markerLen;
	UInt8 b;
	unsigned char *marker;

	getByte(cinfo, &b);
	markerLen = b << 8;
	getByte(cinfo, &b);
	markerLen += b;

	markerLen -= 2;

	if (src->bytes_in_buffer >= markerLen) {
		marker = (unsigned char *)src->next_input_byte;
		src->next_input_byte += markerLen;
		src->bytes_in_buffer -= markerLen;
	}
	else {
		UInt32 i;

		if (kFskErrNone != FskMemPtrNew(markerLen, (FskMemPtr *)&marker))
			boom((j_common_ptr)cinfo);

		state->disposeIfLongJmp = marker;

		for (i=0; i<markerLen; i++)
			getByte(cinfo, &marker[i]);
	}

	if ((NULL != state->exif0Metadata) || (NULL != state->exif1Metadata))
		goto bail;

	FskEXIFScanNew(marker, markerLen, 0, NULL, NULL, NULL, &state->exif0Metadata);

	{
	unsigned char *thumbBits;
	SInt32 thumbBitsLen;

	if (FskEXIFScanNew(marker, markerLen, 1, &thumbBits, &thumbBitsLen, &state->orientation, &state->exif1Metadata)) {
		FskEXIFScan ei = state->exif1Metadata;
		unsigned char *thumbOut, *p, *patchUpTagSize, *patchUpDirectoryCount, *dir;
		UInt32 directoryCountOut = 0, i;
		const char *mime = "image/jpeg";

		if (kFskErrNone == FskMemPtrNew(ei->idfSize + thumbBitsLen + 128 + FskStrLen(mime) + 1, &thumbOut)) {
			// make a new EXIF from the thumbnail
			p = thumbOut;

			FskStrCopy((char *)p, mime);
			p += FskStrLen(mime) + 1;

			*p++ = 0xff;
			*p++ = 0xd8;
			*p++ = 0xff;
			*p++ = 0xe1;
			patchUpTagSize = p;
			p += 2;
			FskMemMove(p, ei->exifHeader, 10);
			p += 10;

			i = 4 + 4;
			if (ei->littleEndian)
				i = FskEndianU32_NtoL(i);
			else
				i = FskEndianU32_NtoB(i);
			FskMisaligned32_PutN(&i, p);
			p += 4;

			patchUpDirectoryCount = p;
			p += 2;
			// only copy recognized self-contained tags
			for (i = 0, dir = ei->idf; i < ei->directoryCount; i++, dir += 12) {
				UInt16 tag = ei->littleEndian ? (dir[0] | (dir[1] << 8)) : (dir[1] | (dir[0] << 8));
				UInt16 type = ei->littleEndian ? (dir[2] | (dir[3] << 8)) : (dir[3] | (dir[2] << 8));

				if ((1 == type) || (3 == type) || (4 == type))
					;
				else
					continue;

				if (0x112 == tag) {				// orientation
					FskMemMove(p, dir, 12);
					p += 12;
					directoryCountOut += 1;
				}
			}

			if (ei->littleEndian)
				directoryCountOut = FskEndianU16_NtoL(directoryCountOut);
			else
				directoryCountOut = FskEndianU16_NtoB(directoryCountOut);
			FskMisaligned16_PutN(&directoryCountOut, patchUpDirectoryCount);

			i = 0;
			FskMisaligned32_PutN(&i, p);		// offset to next IFD - none
			p += 4;

			i = p - patchUpTagSize;
			patchUpTagSize[0] = (unsigned char)(i >> 8);
			patchUpTagSize[1] = (unsigned char)(i & 255);

			FskMemMove(p, thumbBits + 2, thumbBitsLen - 2);
			p += (thumbBitsLen - 2);

			state->thumbnailLen = p - thumbOut;
			state->thumbnail = thumbOut;
		}
	}
	}

bail:
	FskMemPtrDisposeAt((void **)&state->disposeIfLongJmp);

	return true;
}

#if SUPPORT_SCALING

void *getOutputLine(void *userData, Boolean done)
{
	getOutputLineRecord *r = userData;
	unsigned char *result = r->pixels;
	r->pixels += r->rowBytes;
	return result;
}

#endif
