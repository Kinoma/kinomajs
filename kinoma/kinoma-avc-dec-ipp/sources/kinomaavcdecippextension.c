/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#include "kinomaavcdecipp.h"
#include <dlfcn.h>

#include "FskHardware.h"
#include "FskArch.h"

static FskImageDecompressorRecord avcDecompress =
	{avcDecodeCanHandle, avcDecodeNew, avcDecodeDispose, avcDecodeDecompressFrame, NULL, avcDecodeGetMetaData, avcDecodeProperties, NULL, avcDecodeFlush };

static void *IPPlib_handle = NULL;
static void *AVClib_handle = NULL;

int (*IPP_MemCalloc_func)(void **ppDstBuf, int size, unsigned char align) = NULL;
int (*IPP_MemMalloc_func)(void **ppDstBuf, int size, unsigned char align) = NULL;
int (*IPP_MemFree_func)(void ** ppSrcBuf) = NULL;
void *(*IPP_Memcpy_func)(void* dst, void* src, int len) = NULL;
void *(*IPP_Memset_func)(void *buffer, int c, int count) = NULL;
IppCodecStatus (*DecodeSendCmd_H264Video_func)(
	int cmd,
	void *pInParam,
	void *pOutParam,
	void *pEncoderState
) = NULL;
IppCodecStatus (*DecoderFree_H264Video_func)(void **ppSrcDecoderState) = NULL;
IppCodecStatus (*DecoderInitAlloc_H264Video_func)(MiscGeneralCallbackTable *pSrcCallbackTable, void **ppDstDecoderState) = NULL;
IppCodecStatus (*DecodeFrame_H264Video_func)(
    IppBitstream *pSrcBitStream,
    IppH264PicList **ppDstPicList,
    void *pSrcDstDecoderState,
    int *pDstNumAvailFrames
) = NULL;

FskErr AVCIPP_load_lib()
{
	FskErr err = kFskErrNone;
	dlog( "into AVCIPP_load_lib\n");
	
	if (IPPlib_handle == NULL) {
		dlog("loading libmiscgen.so ...");
		IPPlib_handle = dlopen("libmiscgen.so", RTLD_NOW);
		AVClib_handle = dlopen("libcodech264dec.so", RTLD_NOW);
		
		if (IPPlib_handle == NULL || AVClib_handle == NULL) {
			dlog( "failed to load libs\n"); 
			err = -666;
		} else {
			dlog( "succeeded\n"); 
		}	
		BAIL_IF_ERR(err );
	}

	ANDROID_LOAD_FUNC(IPPlib_handle, "IPP_MemCalloc", IPP_MemCalloc_func, 1);
	ANDROID_LOAD_FUNC(IPPlib_handle, "IPP_MemMalloc", IPP_MemMalloc_func, 1);
	ANDROID_LOAD_FUNC(IPPlib_handle, "IPP_MemFree", IPP_MemFree_func, 1);
	ANDROID_LOAD_FUNC(IPPlib_handle, "IPP_Memcpy", IPP_Memcpy_func, 1);
	ANDROID_LOAD_FUNC(IPPlib_handle, "IPP_Memset", IPP_Memset_func, 1);
	
	ANDROID_LOAD_FUNC(AVClib_handle, "DecodeSendCmd_H264Video", DecodeSendCmd_H264Video_func, 1);
	ANDROID_LOAD_FUNC(AVClib_handle, "DecoderFree_H264Video", DecoderFree_H264Video_func, 1);
	ANDROID_LOAD_FUNC(AVClib_handle, "DecoderInitAlloc_H264Video", DecoderInitAlloc_H264Video_func, 1);
	ANDROID_LOAD_FUNC(AVClib_handle, "DecodeFrame_H264Video", DecodeFrame_H264Video_func, 1);
	
bail:
	return err;
}

FskExport(FskErr) kinomaavcdecipp_fskLoad(FskLibrary library)
{
	FskErr err = kFskErrNone;
	
	err = AVCIPP_load_lib();
	BAIL_IF_ERR( err );
	
	dlog( "into kinomaavcdecipp_fskLoad\n"); 
	FskImageDecompressorInstall(&avcDecompress);
	dlog( "out of kinomaavcdecipp_fskLoad\n"); 

bail:
	return err;
}


FskExport(FskErr) kinomaavcdecipp_fskUnload(FskLibrary library)
{
	dlog( "into kinomaavcipp_fskUnload\n");
	
	if (IPPlib_handle && AVClib_handle) {
		dlclose(IPPlib_handle);
		dlclose(AVClib_handle);
		
		IPP_MemCalloc_func = NULL;
		IPP_MemMalloc_func = NULL;
		IPP_MemFree_func = NULL;
		IPP_Memcpy_func = NULL;
		IPP_Memset_func = NULL;
		DecodeSendCmd_H264Video_func = NULL;
		DecoderFree_H264Video_func   = NULL;
		DecoderInitAlloc_H264Video_func = NULL;
		DecodeFrame_H264Video_func   = NULL;
	}
	FskImageDecompressorUninstall(&avcDecompress);
	dlog( "out of kinomaavcdecipp_fskUnload\n"); 
	
	return kFskErrNone;
}
