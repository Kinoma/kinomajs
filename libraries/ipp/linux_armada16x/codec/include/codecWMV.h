/***************************************************************************************** 
Copyright (c) 2009, Marvell International Ltd. 
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Marvell nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY MARVELL ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL MARVELL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************************/


#include "codecDef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Picture Types */
typedef enum {
	IPP_VIDEO_IVOP = 0,
	IPP_VIDEO_PVOP = 1,
	IPP_VIDEO_BVOP = 2,
	IPP_VIDEO_SPRITE = 3,
	IPP_VIDEO_SKIP = 4
}IppPictureType;

/* WMV Decoder */

IPPCODECAPI( IppCodecStatus, DecoderInitAlloc_WMVVideo, (
	int iWidth, int iHeight,
	Ipp32u iCodecVersion,Ipp32u iPostProcess,			//The codec version that used to compress this bitstream
	Ipp8u * pSequenceHeader, Ipp32u iSeqHeaderLen,		//The sequence header of wmv9 stream, store in rcv file, assum asf file will provide such kind of info
	MiscGeneralCallbackTable *pCallbackTable,
	void **ppDecoderState
))

IPPCODECAPI( IppCodecStatus, DecoderFree_WMVVideo, (
	void *ppDecoderState
))

IPPCODECAPI( IppCodecStatus, Decode_WMVVideo, (
    IppBitstream *pBitStream,
    IppPicture *pPicture,
    void *pDecoderState,
    int  flag
))

/*
 * Suppose only support "IPPVC_GET_PICTURETYPE" command
 * No start code exist in WMV9 SP stream
 */
IPPCODECAPI (IppCodecStatus, DecodeSendCmd_WMVVideo, (
    	int	cmd,
		void *pInParam,
    	void *pOutParam,
    	void *pDecoderState
))

#ifdef _cplusplus
}
#endif

