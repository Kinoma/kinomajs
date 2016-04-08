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
#ifndef __KINOMAAVCDECODE__
#define __KINOMAAVCDECODE__

#include "Fsk.h"
#include "FskBitmap.h"
#include "FskUtilities.h"
#include "FskImage.h"

#include "codecDef.h"
#include "codecVC.h"
#include "misc.h"

#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
extern FskInstrumentedTypeRecord gkinomaavcdecippTypeInstrumentation;
//FskInstrumentedSimpleType(kinomaavcdecipp, kinomaavcdecipp);
#define dlog(...)  do { FskInstrumentedTypePrintfDebug  (&gkinomaavcdecippTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define dlog(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif
FskErr AVCIPP_load_lib();
extern int (*IPP_MemCalloc_func)(void **ppDstBuf, int size, unsigned char align);
extern int (*IPP_MemMalloc_func)(void **ppDstBuf, int size, unsigned char align);
extern int (*IPP_MemFree_func)(void ** ppSrcBuf);
extern void *(*IPP_Memcpy_func)(void* dst, void* src, int len);
extern void *(*IPP_Memset_func)(void *buffer, int c, int count);
extern IppCodecStatus (*DecodeSendCmd_H264Video_func)(
	int cmd,
	void *pInParam,
	void *pOutParam,
	void *pEncoderState
);
extern IppCodecStatus (*DecoderFree_H264Video_func)(void **ppSrcDecoderState);
extern IppCodecStatus (*DecoderInitAlloc_H264Video_func)(MiscGeneralCallbackTable *pSrcCallbackTable, void **ppDstDecoderState);
extern IppCodecStatus (*DecodeFrame_H264Video_func)(
    IppBitstream *pSrcBitStream,
    IppH264PicList **ppDstPicList,
    void *pSrcDstDecoderState,
    int *pDstNumAvailFrames
);
FskErr avcDecodeCanHandle(UInt32 format, const char *mime, const char *extension, Boolean *canHandle);
FskErr avcDecodeNew(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension);
FskErr avcDecodeDispose(void *stateIn, FskImageDecompress deco);
FskErr avcDecodeDecompressFrame(void *stateIn, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType);
FskErr avcDecodeFlush(void *state, FskImageDecompress deco);
FskErr avcDecodeGetMetaData(void *stateIn, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);

FskErr avcDecodeSetSampleDescription(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
FskErr avcDecodeSetRotation(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
FskErr avcDecodeSetPreferredPixelFormat( void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property );
FskErr avcDecodeGetMaxFramesToQueue (void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
	
extern FskMediaPropertyEntryRecord avcDecodeProperties[];

#ifdef __cplusplus
}
#endif

#endif
