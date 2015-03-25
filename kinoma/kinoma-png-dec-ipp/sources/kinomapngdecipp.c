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

#include "FskBitmap.h"
#include "FskEndian.h"
#include "FskFiles.h"
#include "FskImage.h"
#include "FskPixelOps.h"
#include "kinomapngdecipp.h"
#include "FskUtilities.h"

#include "zlib.h"

#include "misc.h"
#include "codecPNG.h"
#include "dib.h"

//#include "ippLV.h"

#define PNG_INP_STREAM_BUF_LEN 128*1024 //28480
#define PNG_OUT_STREAM_BUF_LEN 28480

#if SUPPORT_INSTRUMENTATION
FskInstrumentedTypeRecord gkinomapngdecippTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "kinomapngdecipp"};
#endif

typedef struct FskPNGDecodeStateRecord FskPNGDecodeStateRecord;
typedef struct FskPNGDecodeStateRecord *FskPNGDecodeState;

struct FskPNGDecodeStateRecord {
	FskMediaSpooler					spooler;
	UInt32							spoolerPosition;
	Boolean							spoolerOpen;
	MiscGeneralCallbackTable		*dec_param_SrcCBTable;
	IppPNGDecoderParam				DecoderPar;
    IppPicture						DstPicture;
	IppBitstream					srcBitStream;
	void							*pDecoderState;
	IppPNGAncillaryInfo				ancillaryInfo;
};
static void _initAncillary(IppPNGAncillaryInfo *pAncillary);
static FskErr pngDecodeSetSpooler(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr pngDecodeGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord pngDecodeProperties[] = {
	{kFskMediaPropertySpooler,			kFskMediaPropertyTypeSpooler,		NULL,						pngDecodeSetSpooler},
	{kFskMediaPropertyDLNASinks,		kFskMediaPropertyTypeStringList,	pngDecodeGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,		kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

static FskErr FskPngDecodeFromMemory(FskPNGDecodeState state, unsigned char *pngBits, SInt32 pngBitsLen, FskBitmap *bits, FskDimension dimensions, UInt32 *depth);
static FskErr spoolerEnsure(FskPNGDecodeState state, unsigned char **pngBits, UInt32 bytesNeeded);

FskErr pngDecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle)
{
	dlog( "into pngDecodeCanHandle: format: %d, mime: %s\n\n", (int)format, mime ); 
	*canHandle =	(mime && (0 == FskStrCompare(mime, "image/png"))) ||
	(extension && (0 == FskStrCompare(extension, "png")));
	
	return kFskErrNone;
}

FskErr pngDecodeSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	if (dataSize > 8) {
		if ((0x89 == data[0]) && (0x50 == data[1]) && (0x4e == data[2]) && (0x47 == data[3]) &&
			(0x0d == data[4]) && (0x0a == data[5]) && (0x1a == data[6]) && (0x0a == data[7])) {
			*mime = FskStrDoCopy("image/png");
			return kFskErrNone;
		}
	}
	dlog( "into pngDecodeSniff: mimetype: %s\n\n", *mime ); 
	
	return kFskErrUnknownElement;
}

FskErr pngDecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension)
{
	dlog( "into pngDecodeNew: format: %d, mime: %s\n", (int)format, mime ); 
	return FskMemPtrNewClear(sizeof(FskPNGDecodeStateRecord), (FskMemPtr *)&deco->state);
}

static IppCodecStatus videoInitBuffer (IppBitstream *pBufInfo)
{
    // Initialize IppBitstream
    // at least big enough to store 2 frame data for less reload 
    int err = IPP_MemMalloc((void**)(&pBufInfo->pBsBuffer), PNG_INP_STREAM_BUF_LEN, 4);
	
    if (err != IPP_OK || NULL == pBufInfo->pBsBuffer) 
        return IPP_STATUS_NOMEM_ERR;
	
	IPP_Memset(pBufInfo->pBsBuffer, 0, PNG_INP_STREAM_BUF_LEN);
	
    //no read data at beginning
    //set current pointer to the end of buffer
    pBufInfo->pBsCurByte = pBufInfo->pBsBuffer + PNG_INP_STREAM_BUF_LEN;
    pBufInfo->bsCurBitOffset = 0;
    pBufInfo->bsByteLen = PNG_INP_STREAM_BUF_LEN;
	
    return IPP_STATUS_NOERR;
}

static IppCodecStatus videoFreeBuffer (IppBitstream *pBufInfo)
{
    if ( pBufInfo->pBsBuffer ) 
	{
        IPP_MemFree((void**)(&pBufInfo->pBsBuffer));
        pBufInfo->pBsBuffer = NULL;
    }
	
    return IPP_STATUS_NOERR;
}

FskErr pngDecodeDispose(void *stateIn, FskImageDecompress deco)
{
	FskPNGDecodeState state = stateIn;
	
	dlog( "into pngDecodeDispose\n\n" ); 
	
	FskMemPtrDisposeAt((void **)&state->spooler);
	if (state->pDecoderState)
		DecoderFree_PNG(&state->pDecoderState);
	
	videoFreeBuffer(&state->srcBitStream);
	
	if( state->dec_param_SrcCBTable != NULL ) 
		IPP_MemFree ((void **)&state->dec_param_SrcCBTable);
	
	FskMemPtrDisposeAt(&deco->state);
	
	
	dlog( "finish pngDecodeDispose with no err!!\n" ); 
	
	return kFskErrNone;
}

FskErr pngDecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	FskErr err = kFskErrNone;
	FskBitmap bits = deco->bits;
	
	dlog( "into pngDecodeDecompressFrame: datasize: %d, frameType: %d\n\n", (int)dataSize, (int)frameType );
	
	err = FskPngDecodeFromMemory(stateIn, (void *)data, dataSize, &bits, NULL, NULL);
	if (err) goto bail;
	
	deco->bits = bits;
#if 0 //def BNIE_LOG
	dlog( "finish pngDecodeDecompressFrame, err = %d, bits = %x \n\n", err, (int)bits );
	{
		unsigned int *p = (unsigned int *)bits;
		unsigned int *q;
		
		p++; p++;
		dlog( "bound.width = %d\n", (int)(*p++) );
		dlog( "bound.height = %d\n", (int)(*p++) );
		dlog( "depth = %d\n", (int)(*p++) );
		dlog( "pixelFormat = %d\n", (int)(*p++) );
		dlog( "rowBytes = %d\n", (int)(*p++) );
		q = (unsigned int *)(*p);
		
		FILE *fp = fopen("/home/tlee/work/kinoma/bmp.bin", "wb");
		fwrite((unsigned char*)q, 1, 416*200, fp);
		fclose(fp);
		
		dlog( "data = %x %x %x %x \n", (int)(*q++), (int)(*q++), (int)(*q++), (int)(*q++) );
	}
#endif
	
bail:
	return err;
}

FskErr pngDecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 property, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	FskErr err;
	FskDimensionRecord dimensions;
	UInt32 depth;
	
	dlog( "into pngDecodeGetMetaData \n\n" );
	
	if ((property != kFskImageDecompressMetaDataDimensions) && (property != kFskImageDecompressMetaDataBitDepth))
		return kFskErrUnimplemented;
	
	err = FskPngDecodeFromMemory(stateIn, deco->data, deco->dataSize, NULL, &dimensions, &depth);
	if (err) return err;
	
	if (kFskImageDecompressMetaDataDimensions == property) {
		value->type = kFskMediaPropertyTypeDimension;
		value->value.dimension = dimensions;
	}
	else {
		value->type = kFskMediaPropertyTypeInteger;
		value->value.integer = depth;
	}
	
	return kFskErrNone;
}

typedef void (*pngCopyProc)(unsigned char *src, unsigned char *dst, UInt32 width, UInt32 xStep, unsigned char *palette);

typedef struct {
	unsigned char		*pngBits;
	UInt32				pngBitsLen;
	
	UInt32				width;
	UInt32				height;
	unsigned char		bitDepth;
	unsigned char		bitsPerPixel;
	unsigned char		filterBytesPerPixel;
	unsigned char		colorType;
	unsigned char		compressionMethod;
	unsigned char		filterMethod;
	unsigned char		interlaceMethod;
	unsigned char		channelCount;
	
	FskBitmap			bitmap;
	SInt32				rowBytes;
	UInt32				depth;
	UInt32				pixelFormat;
	UInt32				pixelFormatSizeInBytes;
	
	unsigned char		*scanLine;			// over allocated by kScanLineSlop bytes at the start
	unsigned char		*prevScanLine;		// over allocated by kScanLineSlop bytes at the start
	unsigned char		*scanBuffers;
	UInt32				scanLineByteCount;
	UInt32				bytesInScanLine;
	unsigned char		*scanOut;
	unsigned char		*initialBaseAddr;
	unsigned char		*palette;
	Boolean				paletteHasAlpha;
	unsigned char		*crushBuffer;
	
	UInt32				pass;				// when interlaced, steps from 0 to 6 (7 total passes)
	UInt32				y;
	
	pngCopyProc			copyProc;
	
	z_stream			zlib;
	Boolean				initializedZlib;
	
	Boolean				isApple;			// Normal PNG is RGBA; Apple PNG is BGRA.
} FskPngDecodeRecord, *FskPngDecode;

static void print_init_(IppPicture DstPicture, unsigned char *pDecoderState, int cnt)
{
	unsigned char *p = (unsigned char *)DstPicture.ppPicPlane[0];
	unsigned char *q = (unsigned char *)pDecoderState;
	dlog("picWidth = %d\n", (int)DstPicture.picWidth);
	dlog("picHeight = %d\n", (int)DstPicture.picHeight);
	if (p != NULL) {
		dlog("ppPicPlane[0] = %x, data = %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", (int)p, (int)p[0], (int)p[1], (int)p[2], (int)p[3], (int)p[4], (int)p[5], (int)p[6], (int)p[7],
				(int)p[8], (int)p[9], (int)p[10], (int)p[11], (int)p[12], (int)p[13], (int)p[14], (int)p[15]);
	}
	else {
		dlog("ppPicPlane[0] is Null\n");
	}
	dlog("picPlaneStep[0] = %d\n", (int)DstPicture.picPlaneStep[0]);
	dlog("picPlaneNum = %d\n", (int)DstPicture.picPlaneNum);
	dlog("picChannelNum = %d\n", (int)DstPicture.picChannelNum);
	dlog("picFormat = %d\n", (int)DstPicture.picFormat);
	
	if (cnt == 0){
		dlog("\n");
		dlog("pDecoderState = %x, data = %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n\n\n", (int)q, (int)q[0], (int)q[1], (int)q[2], (int)q[3], (int)q[4], (int)q[5], (int)q[6], (int)q[7],
				(int)q[8], (int)q[9], (int)q[10], (int)q[11], (int)q[12], (int)q[13], (int)q[14], (int)q[15],
				(int)q[16], (int)q[17], (int)q[18], (int)q[19], (int)q[20], (int)q[21], (int)q[22], (int)q[23]);
	}
	else {
		dlog("\n\n\n");
	}
}

static void print_dec_(IppPNGDecoderParam DecoderPar, IppPNGAncillaryInfo ancillaryInfo)
{
	dlog("nDesiredClrMd = %d\n", (int)DecoderPar.nDesiredClrMd);
	dlog("nDesiredImageSize = %d, %d\n", (int)DecoderPar.nDesiredImageSize.width, (int)DecoderPar.nDesiredImageSize.height);
	dlog("nAlphaValue = %d\n", (int)DecoderPar.nAlphaValue);
	dlog("\n");
	dlog("gama = %d\n", (int)ancillaryInfo.gama);	
	dlog("sRGB = %d\n", (int)ancillaryInfo.sRGB);
	dlog("AncillaryInfoFlag = %d\n\n\n", (int)ancillaryInfo.AncillaryInfoFlag);
}


void convert_32BGRA_to_32A16RGB565LE(unsigned char * src, int width, int height, int rowBytes)
{
#ifdef MARVELL_SOC_PXA168
	extern void convert_32BGRA_to_32A16RGB565LE_arm_wmmx(unsigned char * src, int width, int height, int rowBytes);
	convert_32BGRA_to_32A16RGB565LE_arm_wmmx(src, width, height, rowBytes);
#else
	for (; height--; src += rowBytes) {
		unsigned char *s = src;
		UInt32 out = 0;
		int w = width;
		for (; w--; s += 4){
			out = (s[2] >> 3) << 11;
			out |= (s[1] >> 2) << 5;
			out |= s[0] >> 3;
			out |= s[3] << 24;
			*(UInt32 *)s = out;
		}
	}
#endif
}

FskErr FskPngDecodeFromMemory(FskPNGDecodeState state, unsigned char *pngBits, SInt32 pngBitsLen, FskBitmap *bits, FskDimension dimensions, UInt32 *depth)
{
	FskErr err			= kFskErrNone;
	Boolean Done		= false;
	FskPngDecode png	= NULL;
	int data_copied		= 0;
	int bErStream		= 0;
	Boolean	hasAlpha = false;
	int Step;
	UInt32 BufferLen;
	
	dlog( "into FskPngDecodeFromMemory: size = %d, *bits = %d\n\n", (int)pngBitsLen, (int)*bits );
	
	err = FskMemPtrNewClear(sizeof(FskPngDecodeRecord), &png);
	if (err) goto bail;
	
	// validate the header
	if (pngBitsLen < 8) {
		err = kFskErrBadData;
		goto bail;
	}
	
	dlog( "setup spooler!!!\n");
	if (state->spooler) {
		if (state->spooler->doOpen) {
			err = (state->spooler->doOpen)(state->spooler, kFskFilePermissionReadOnly);
			if (err) goto bail;
			state->spoolerPosition = 0;
			state->spoolerOpen = true;
		}
		
		err = spoolerEnsure(state, &pngBits, pngBitsLen);
		if (err) goto bail;
	}
	
	
	if ((0x89 != pngBits[0]) || (0x50 != pngBits[1]) || (0x4e != pngBits[2]) || (0x47 != pngBits[3]) ||
		(0x0d != pngBits[4]) || (0x0a != pngBits[5]) || (0x1a != pngBits[6]) || (0x0a != pngBits[7])) {
		err = kFskErrBadData;
		goto bail;
	}
	dlog( "start code check OK !!!\n");

	//if (state->pDecoderState == NULL)
	{
		err = IPP_MemMalloc((void**)&state->dec_param_SrcCBTable,sizeof(MiscGeneralCallbackTable),4);
		if (IPP_FAIL == err || NULL == state->dec_param_SrcCBTable) 
		{
            dlog( "error happens when MallocCBTable, err is %d\n", (int)err );
			goto bail;
		}
		
		//Set up CallBack table
		dlog( "setup dec_param_SrcCBTable!!!\n");
		state->dec_param_SrcCBTable->fMemMalloc		= (MiscMallocCallback)IPP_MemMalloc;
		state->dec_param_SrcCBTable->fMemCalloc		= (MiscCallocCallback)IPP_MemCalloc;
		state->dec_param_SrcCBTable->fMemFree		= (MiscFreeCallback)IPP_MemFree;
		state->dec_param_SrcCBTable->fStreamRealloc	= (MiscStreamReallocCallback)IPP_MemRealloc;
		state->dec_param_SrcCBTable->fStreamFlush	= NULL;//(MiscStreamFlushCallback)miscgStreamFlush;
		state->dec_param_SrcCBTable->fFileSeek		= NULL;//(MiscFileSeekCallback)IPP_Fseek;
		state->dec_param_SrcCBTable->fFileRead		= NULL;//(MiscFileReadCallback)IPP_Fread;
		state->dec_param_SrcCBTable->fFileWrite		= NULL;//(MiscWriteFileCallBack)IPP_Fwrite;
		
		err = videoInitBuffer(&state->srcBitStream);
		if (err != IPP_STATUS_NOERR) {
			if( state->dec_param_SrcCBTable != NULL ) 
				IPP_MemFree ((void **)&state->dec_param_SrcCBTable);
            dlog( "error happens when InitBuffer, err is %d\n", (int)err );
			goto bail;
		}
		dlog( "init inputBuffer!!!\n");
		
		BufferLen = pngBitsLen;
		state->srcBitStream.pBsCurByte = state->srcBitStream.pBsBuffer;
	    state->srcBitStream.bsByteLen = pngBitsLen > PNG_INP_STREAM_BUF_LEN ? PNG_INP_STREAM_BUF_LEN : pngBitsLen;
	    state->srcBitStream.bsCurBitOffset = 0;
		dlog( "Copy data for the first time!!!, CopyLen = %d\n", state->srcBitStream.bsByteLen );
		IPP_Memcpy( (void *)state->srcBitStream.pBsBuffer, (void *)pngBits, state->srcBitStream.bsByteLen );
		data_copied = 1;
		
		_initAncillary( &state->ancillaryInfo );
		dlog( "init _initAncillary !!!\n" );
		
		/* Parse header of image and get basic info about it */
        err = DecoderInitAlloc_PNG(&state->srcBitStream,
								   &state->DstPicture,
								   &state->pDecoderState,
								   state->dec_param_SrcCBTable);
		
		dlog("After Init_PNG !!!*************************************\n");
		print_init_(state->DstPicture, (unsigned char *)state->pDecoderState, 0);
		
        if ( err != IPP_STATUS_NOERR ) { 
			videoFreeBuffer(&state->srcBitStream);
			if( state->dec_param_SrcCBTable != NULL ) 
				IPP_MemFree ((void **)&state->dec_param_SrcCBTable);
			
			if (state->pDecoderState)
			{
				DecoderFree_PNG(&state->pDecoderState);
				state->pDecoderState = NULL;
			}
            dlog( "error happens when init, err is %d\n", (int)err );
			goto bail;
        }
		
		dlog( "init PNG_DECODER width, height = (%d, %d), picFormat = %d\n", state->DstPicture.picWidth, state->DstPicture.picHeight, state->DstPicture.picFormat );
		state->DecoderPar.nDesiredImageSize.width  = png->width		= state->DstPicture.picWidth;
		state->DecoderPar.nDesiredImageSize.height = png->height	= state->DstPicture.picHeight;
		png->colorType = state->DstPicture.picFormat;
		
		//set Alpha info
		if ( IPP_PNG_GREY_A == state->DstPicture.picFormat || IPP_PNG_TRUECOLOR_A == state->DstPicture.picFormat || state->DstPicture.picFormat == 3)
			hasAlpha = 1;
		
		if (hasAlpha)
			png->pixelFormat = kFskBitmapFormatSourceDefaultRGBA;
		else
			png->pixelFormat = kFskBitmapFormatDefaultRGB;
		
		err = FskBitmapNew(png->width, png->height, png->pixelFormat, &png->bitmap);
		if (err) { 
			videoFreeBuffer(&state->srcBitStream);
			if( state->dec_param_SrcCBTable != NULL ) 
				IPP_MemFree ((void **)&state->dec_param_SrcCBTable);
			
			if (state->pDecoderState)
			{
				DecoderFree_PNG(&state->pDecoderState);
				state->pDecoderState = NULL;
			}
			
			err = IPP_FAIL;
			goto bail;
        }
		
		dlog( "PNG infor!!! Alpha = %d, pixelFormat = %d\n", (int)hasAlpha, (int)png->pixelFormat );

		FskBitmapSetHasAlpha(png->bitmap, hasAlpha);
		
		// sort ouf the bitmap format
		if (6 == png->colorType)
			png->channelCount = 4;
		else if (2 == png->colorType)
			png->channelCount = 3;
		else if (3 == png->colorType)
			png->channelCount = 1;
		else if (0 == png->colorType)
			png->channelCount = 1;
		else if (4 == png->colorType)
			png->channelCount = 2;
		
		png->bitDepth = 8; //set as default
		png->depth = png->channelCount * 8;
		
		// return metadata
		if (dimensions) {
			dimensions->width = png->width;
			dimensions->height = png->height;
		}
		if (depth) {
			*depth = png->depth;
			goto bail; //return for retrieving Metadata
		}
		
		if (kFskBitmapFormat16RGB565LE == png->pixelFormat) //rgb565
		{	
			png->bitsPerPixel = 16;
			state->DecoderPar.nDesiredClrMd			   = IPP_BGR565;
			dlog( "not Alpha, output is RGB565!!!\n" );
		}
		else if (kFskBitmapFormat32A16RGB565LE == png->pixelFormat) //rgb565, Alpha
		{	
		        /*IPP codec does not support this pixel format, we set BGRA8888 here and do a conversion later */
			png->bitsPerPixel = 32;
			state->DecoderPar.nDesiredClrMd			   = IPP_BGRA8888;
			dlog( "has Alpha, output is BGRA8888!!!\n" );
		}
		else if (kFskBitmapFormat32BGRA == png->pixelFormat) //rgba
		{	
			png->bitsPerPixel = 32;
			state->DecoderPar.nDesiredClrMd			   = IPP_BGRA8888;
			dlog( "has Alpha, output is RGBA8888!!!\n" );
		}
		
		Step = IIP_WIDTHBYTES_4B((png->width) * (png->bitsPerPixel));
		Step = (Step+31)&(~31);
		//state->DstPicture.picPlaneStep[0] = -(Step);
		//state->DstPicture.ppPicPlane[0] = (void *)(png->scanOut + (Step*(png->height-1)));
		FskBitmapWriteBegin(png->bitmap, (void **)&png->scanOut, &png->rowBytes, NULL);	
		state->DstPicture.picPlaneStep[0] = Step;
		state->DstPicture.ppPicPlane[0] = (void *)(png->scanOut);
	}
	
	dlog( "PNG infor!!! ChannelNum = %d, ColorType = %d, BitDepth = %d, rowBytes = %d \n", png->channelCount, png->colorType, png->bitDepth, (int)png->rowBytes );
	
	/* Call the core PNG decoder function */
	while(!Done && !bErStream) {
		
		err = Decode_PNG (&state->srcBitStream, &state->DecoderPar, &state->DstPicture, &state->ancillaryInfo, state->pDecoderState);
		dlog("After Decoding Once!!! err = %d ******************************\n",(int)err);
		print_init_(state->DstPicture, (unsigned char *)state->pDecoderState, 1);
		print_dec_(state->DecoderPar, state->ancillaryInfo);
		
		switch(err) {
			case IPP_STATUS_NEED_INPUT:
				BufferLen -= state->srcBitStream.bsByteLen;
				state->srcBitStream.pBsCurByte = state->srcBitStream.pBsBuffer;
				state->srcBitStream.bsByteLen = BufferLen > PNG_INP_STREAM_BUF_LEN ? PNG_INP_STREAM_BUF_LEN : BufferLen;
				state->srcBitStream.bsCurBitOffset = 0;
				dlog( "Decode_PNG need more data !!!, CopyLen = %d\n", state->srcBitStream.bsByteLen );
				IPP_Memcpy( (void *)state->srcBitStream.pBsBuffer, (void *)((unsigned char *)pngBits + (pngBitsLen-BufferLen)), state->srcBitStream.bsByteLen );
				break;
				
			case IPP_STATUS_NOERR:
				dlog( "Decode_PNG loop finish !!!\n" );
				Done = 1;
				break;
				
			case IPP_STATUS_BADARG_ERR:
			case IPP_STATUS_NOMEM_ERR:
			case IPP_STATUS_BITSTREAM_ERR:
			case IPP_STATUS_NOTSUPPORTED_ERR:
			case IPP_STATUS_ERR:
			default:
				bErStream = 1;
                dlog( "Decoding meets %d error\n, Give up!!", (int)err );
				goto bail;
		}
	}
	
bail:
	if (bits)
		*bits = NULL;
	
	dlog( "Decode_PNG Go to The End err = %d !\n", (int)err );
	
	if (NULL != png) {
		if (NULL != png->scanOut) {
		        /* Hack: We do a conversion for 32A16RGB565LE */
			if (kFskBitmapFormat32A16RGB565LE == png->pixelFormat) { 
				convert_32BGRA_to_32A16RGB565LE(png->scanOut, png->width, png->height, png->rowBytes);
			}
			FskBitmapWriteEnd(png->bitmap);
		}
		
		dlog( " going FskBitmapWriteEnd(png->bitmap)! %d \n", (int)png->bitmap);
		
		if (state->spoolerOpen && state->spooler->doClose) {
			(state->spooler->doClose)(state->spooler);
			state->spoolerOpen = false;
		}
		
		if (err == kFskErrNone && bits)
		{	
			*bits = png->bitmap;
			dlog( " Outputing bits = %d!\n", (int)(*bits));
		}
		else
			FskBitmapDispose(png->bitmap);
		
		if (png->initializedZlib)
			inflateEnd(&png->zlib);

		FskMemPtrDispose(png);
	}
	
	//safety's sake, free memory here just in case!
	if (state->pDecoderState)
		DecoderFree_PNG(&state->pDecoderState);
	
	videoFreeBuffer(&state->srcBitStream);
	
	if( state->dec_param_SrcCBTable != NULL ) 
		IPP_MemFree ((void **)&state->dec_param_SrcCBTable);
	
	return err;
}

/******************************************************************************
 // Name:				_initAncillary
 // Description:			Initialize ancillary information, including content  
 //                      about ancillary chunk and its availability
 //
 // Input Arguments:
 //		pAncillary:	    Pointer to ancillary info structure
 //
 // Output Arguments:	
 //		pAncillary:	    Pointer to initialized ancillary info structure
 // Returns:
 //        [NONE]			
 ******************************************************************************/

void _initAncillary( IppPNGAncillaryInfo *pAncillary)
{
	pAncillary->gama                =   0;
    pAncillary->pAlpha              =   NULL;  /* we assume no alpha info */
    pAncillary->greyBkgd            =   0;
    pAncillary->RGBBkgd[0]          =   0;
    pAncillary->RGBBkgd[1]          =   0;
    pAncillary->RGBBkgd[2]          =   0;
    pAncillary->plteBkgd            =   0;
    pAncillary->RGBTrans[0]         =   0;
    pAncillary->RGBTrans[1]         =   0;
    pAncillary->RGBTrans[2]         =   0;
    pAncillary->greyTrans           =   0;
    pAncillary->sRGB                =   0;
    pAncillary->AncillaryInfoFlag   =   0;
}

FskErr pngDecodeSetSpooler(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskPNGDecodeState state = stateIn;
	
	FskMemPtrDisposeAt((void **)&state->spooler);
	if (kFskMediaSpoolerValid & property->value.spooler.flags)
		return FskMemPtrNewFromData(sizeof(FskMediaSpoolerRecord), &property->value.spooler, (FskMemPtr *)&state->spooler);
	
	return kFskErrNone;
}

FskErr spoolerEnsure(FskPNGDecodeState state, unsigned char **pngBits, UInt32 bytesNeeded)
{
	FskErr err;
	UInt32 bytesRead;
	void *data;
	
	if (!state->spooler)
		return kFskErrNone;
	
	err = (state->spooler->doRead)(state->spooler, state->spoolerPosition, bytesNeeded, &data, &bytesRead);
	if (err) return err;
	
	state->spoolerPosition += bytesRead;
	*pngBits = data;
	
	return kFskErrNone;
}

FskErr pngDecodeGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "image/png\000";
	
	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);
	
	return kFskErrNone;
}
