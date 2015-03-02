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


#ifndef _CODECGIF_H_
#define _CODECGIF_H_

#include "codecDef.h"	/* General Codec external header file*/

#ifdef __cplusplus
extern "C" {
#endif

/*********** Data Types, Data Structures ************************/
typedef struct _IppColorTableRGB888_GIF{
	Ipp8u red;
	Ipp8u green;
	Ipp8u blue;
}IppClrTbl;

/* decoder param returned to user. */
typedef struct _IppGIFDecoderParam {
	IppColorFormat nDesiredClrMd;
	IppiSize          nDesiredImageSize;
	Ipp8u             nAlphaValue;
	/* return to user. or input the colorTable in case that the data stream do not contain the color table */
	int nClrTblSz; /* -1: no ColorTable; 1~256, the size of color table. */
	int nBkgndClrIdx;
	IppClrTbl GlbClrTbl[256];
}IppGIFDecoderParam;

typedef struct IppGIFPictureInfo{	
	int nCurFrame;						/* current frame*/
	/* following field is got from the graphic control extention. */ 
	int nDisposalMethod;			    /* Disposal method. */
	int bUserInput;						/* if set, the image should be display after met user input.*/
	int bTrnsClrFlg;			/* 1: nTrnsClrIdx is enabled. 0: disabled.*/
	int nTrnsClrIdx;			/* the transparent color index */
	int nPicTimeStamp;
}IppGIFPictureInfo;

typedef enum _IppGIFCommand{
	/* Command for GIF decoder */
	IPPGIF_SET_ALPHAVALUE = 0,

	IPPGIFCOMMAND_IPPENUM_FORCE32BIT_RESERVE = 0x7fffffff
} IppGIFCommand;
/***** GIF Codec Functions *****/
IPPCODECAPI( IppCodecStatus, DecoderInitAlloc_GIF, (
    IppGIFDecoderParam        *pDecoderPar,
    IppBitstream              *pSrcDstStream,
    IppPicture                *pDstPicture,
    MiscGeneralCallbackTable  *pSrcCallbackTable,
    void                      **ppDstDecoderState
	))

IPPCODECAPI( IppCodecStatus, Decode_GIF, (
	IppBitstream        *pSrcBitstream,
	IppPicture          *pDstPicture,
	IppGIFPictureInfo   *pPictureInfo,
	void                *pDecoderState	
	))

IPPCODECAPI(IppCodecStatus, DecoderFree_GIF, (
   void **ppDecoderState
   ))

IPPCODECAPI(IppCodecStatus,DecodeSendCmd_GIF, (
   int cmd, void *pInParam, void *pOutParam, void *pDecoderState))


#ifdef __cplusplus
}
#endif

#endif    /* #ifndef _CODECGIF_H_ */

/* EOF */