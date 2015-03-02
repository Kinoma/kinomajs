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


#ifndef _IPPVC_H_
#define _IPPVC_H_

#include "ippdefs.h"

#ifdef __cplusplus
extern "C" {
#endif


/***** Data Types, Data Structures and Constants ********************************/

/* Residual coefficient pair buffer flag */
#define     H264_16BIT_DATA             0x10
#define     H264_END_DATA               0x20

/* Video Components */
typedef enum {
    IPP_VIDEO_LUMINANCE,        /* Luminance component   */
    IPP_VIDEO_CHROMINANCE,      /* Chrominance component */
    IPP_VIDEO_ALPHA             /* Alpha component       */
} IppVideoComponent;

/* Macroblock Types */
typedef enum {
    IPP_VIDEO_INTER         = 0,    /* P picture or P-VOP */
    IPP_VIDEO_INTER_Q       = 1,    /* P picture or P-VOP */
    IPP_VIDEO_INTER4V       = 2,    /* P picture or P-VOP */
    IPP_VIDEO_INTRA         = 3,    /* I and P picture, or I- and P-VOP */
    IPP_VIDEO_INTRA_Q       = 4,    /* I and P picture, or I- and P-VOP */
    IPP_VIDEO_INTER4V_Q     = 5,    /* P picture or P-VOP(H.263)*/
    IPP_VIDEO_DIRECT        = 6,    /* B picture or B-VOP (MPEG-4 only) */
    IPP_VIDEO_INTERPOLATE   = 7,    /* B picture or B-VOP */
    IPP_VIDEO_BACKWARD      = 8,    /* B picture or B-VOP */
    IPP_VIDEO_FORWARD       = 9,     /* B picture or B-VOP */
    IPP_VIDEO_NOTCODED      = 10,   /* B picture or B-VOP */
	IPP_VIDEO_NOTCODED_SVOP = 11    /* not code SVOP (MPEG-4 only) */
} IppMacroblockType;

/* Motion Vector */
typedef struct _IppMotionVector {
    Ipp16s  dx;
    Ipp16s  dy;
} IppMotionVector;

typedef	struct _IppCoordinate
{	
    int	x;
    int	y;
}IppCoordinate;

typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} IppiRegion;


typedef enum _BlockNum {
    Y_BLOCK1    = 0, 
    Y_BLOCK2    = 1, 
    Y_BLOCK3    = 2, 
    Y_BLOCK4    = 3,
    U_BLOCK     = 4,
    V_BLOCK     = 5,
    A_BLOCK1    = 6,
    A_BLOCK2    = 7,
    A_BLOCK3    = 8,
    A_BLOCK4    = 9
} BlockNum;

/* Transparent Status */
enum {
    IPP_VIDEO_TRANSPARENT   = 0,
    IPP_VIDEO_PARTIAL       = 1,
    IPP_VIDEO_OPAQUE        = 2
};

/* Direction */
enum {
    IPP_VIDEO_NONE          = 0,
    IPP_VIDEO_HORIZONTAL    = 1,
    IPP_VIDEO_VERTICAL      = 2,
    IPP_VIDEO_DCONLY        = 3
};

/* bilinear interpolation type */
enum {
    IPP_VIDEO_INTEGER_PIXEL 	= 0,
    IPP_VIDEO_HALF_PIXEL_X      = 1,
    IPP_VIDEO_HALF_PIXEL_Y      = 2,
    IPP_VIDEO_HALF_PIXEL_XY     = 3
};

enum {
    IPP_DCScalerLinear, 
    IPP_DCScalerNonLinear
};

typedef enum _BAB_TYPE {
    MVDZ_NOUPDT     = 0,
    MVDNZ_NOUPDT    = 1,
    ALL_TRANSP      = 2, 
    ALL_OPAQUE      = 3, 
    INTRA_CAE       = 4, 
    INTER_CAE_MVDZ  = 5,
    INTER_CAE_MVDNZ = 6
}IppBABType;

/* For H264 */
typedef enum
{
  IPP_4x4_VERT     = 0,  
  IPP_4x4_HOR      = 1,
  IPP_4x4_DC       = 2,
  IPP_4x4_DIAG_DL  = 3,
  IPP_4x4_DIAG_DR  = 4,
  IPP_4x4_VR       = 5,
  IPP_4x4_HD       = 6,
  IPP_4x4_VL       = 7,
  IPP_4x4_HU       = 8,
  IPP_4x4_UNAVAIL  = -1
} IppIntra4x4PredMode_H264;


typedef enum
{
  IPP_16X16_VERT   = 0,
  IPP_16X16_HOR    = 1,
  IPP_16X16_DC     = 2,
  IPP_16X16_PLANE  = 3,
  IPP_16X16_UNAVAIL  = -1
} IppIntra16x16PredMode_H264;


typedef enum
{
  IPP_CHROMA_DC    = 0,
  IPP_CHROMA_HOR   = 1,
  IPP_CHROMA_VERT  = 2,
  IPP_CHROMA_PLANE = 3,
  IPP_CHROMA_UNAVAIL = -1
} IppIntraChromaPredMode_H264;

// Direction for H263 INTRA PREDICTION
enum {
    IPP_VIDEO_DCONLY_H263		= 0,
    IPP_VIDEO_VERTICAL_H263		= 1,
    IPP_VIDEO_HORIZONTAL_H263   = 2,
	IPP_VIDEO_NONE_H263			= 3
};

// DQUANT Update Mode for H263 AnnexT
enum {
	IPP_VIDEO_FORBIDDEN_MQ_H263		= 0,
	IPP_VIDEO_ARBITRARY_MQ_H263		= 1,
	IPP_VIDEO_SMALLSTEP0_MQ_H263	= 2,
	IPP_VIDEO_SMALLSTEP1_MQ_H263	= 3,
};

/********************* Video Coding Functions *********************/


/***** General Video Functions *****/

/* 8 by 8 Inverse Discrete Cosine Transform */
IPPAPI(IppStatus, ippiDCT8x8Inv_Video_16s_C1, (const Ipp16s * pSrc, Ipp16s * pDst))

IPPAPI(IppStatus, ippiDCT8x8Inv_Video_16s_C1I, (Ipp16s * pSrcDst))

IPPAPI(IppStatus, ippiDCT8x8Inv_Video_16s8u_C1R, (const Ipp16s * pSrc, Ipp8u * pDst, int dstStep))

/* Inverse Zigzag Scanning */

IPPAPI(IppStatus, ippiZigzagInvClassical_Compact_16s,
	(const Ipp16s * pSrc, int len, Ipp16s * pDst))

IPPAPI(IppStatus, ippiZigzagInvHorizontal_Compact_16s,
	(const Ipp16s * pSrc, int len, Ipp16s * pDst))

IPPAPI(IppStatus, ippiZigzagInvVertical_Compact_16s,
	(const Ipp16s * pSrc, int len, Ipp16s * pDst))

/* Prediction with Overlapped Block Motion Compensation (OBMC) */
IPPAPI(IppStatus, ippiPredictBlock_OBMC_8u,
	(const Ipp8u * pSrcRef, Ipp8u * pDst, int step,
	 IppMotionVector * pMVCur, IppMotionVector * pMVLeft, IppMotionVector * pMVRight,
	 IppMotionVector * pMVAbove, IppMotionVector * pMVBelow))

/* General Motion compensation */
IPPAPI(IppStatus, ippiMCBlock_RoundOff_8u,
	(const Ipp8u * pSrc, int srcStep, Ipp8u * pDst, int dstStep,int predictType))
IPPAPI(IppStatus, ippiMCBlock_RoundOn_8u,
	(const Ipp8u * pSrc, int srcStep, Ipp8u * pDst, int dstStep,int predictType))
IPPAPI(IppStatus, ippiMCReconBlock_RoundOn,
	(const Ipp8u *pSrc, int srcStep, Ipp16s * pSrcResidue, Ipp8u * pDst,int dstStep, int predictType))
IPPAPI(IppStatus, ippiMCReconBlock_RoundOff,
	(const Ipp8u *pSrc, int srcStep, Ipp16s * pSrcResidue, Ipp8u * pDst, int dstStep, int predictType))	 

/***** H.263+ Functions *****/

IPPAPI(IppStatus, ippiDecodeMV_H263,
	(Ipp8u ** ppBitStream, int * pBitOffset, IppMotionVector * pSrcDstMV))

IPPAPI(IppStatus, ippiDecodeMV_TopBorder_H263,
	(Ipp8u ** ppBitStream, int * pBitOffset, IppMotionVector * pSrcDstMV))

/* Copy */
IPPAPI(IppStatus, ippiCopyMB_H263_8u,
	(const Ipp8u * pSrc, Ipp8u * pDst, int step))

IPPAPI(IppStatus, ippiCopyBlock_H263_8u,
	(const Ipp8u * pSrc, Ipp8u * pDst, int step))

/* Quantization */
IPPAPI(IppStatus, ippiQuantInvIntra_Compact_H263_16s_I,
	(Ipp16s * pSrcDst, int len, int QP))

IPPAPI(IppStatus, ippiQuantInvInter_Compact_H263_16s_I,
	(Ipp16s * pSrcDst, int len, int QP))

/* Reconstruction */
IPPAPI(IppStatus, ippiReconMB_H263,
	(const Ipp8u * pSrc, const Ipp16s * pSrcResidual, Ipp8u * pDst, int step))

IPPAPI(IppStatus, ippiReconMB_H263_I,
	(Ipp8u * pSrcDst, const Ipp16s * pSrcResidual, int step))

IPPAPI(IppStatus, ippiReconBlock_H263,
	(const Ipp8u * pSrc, const Ipp16s * pSrcResidual, Ipp8u * pDst, int step))

IPPAPI(IppStatus, ippiReconBlock_H263_I,
	(Ipp8u * pSrcDst, const Ipp16s * pSrcResidual, int step))

/* Expand Frame */
IPPAPI(IppStatus, ippiExpandFrame_H263_8u,
	(Ipp8u * pSrcDstPlane, int frameWidth, int frameHeight, int expandPels, int step))

/* Deblock Filtering */
IPPAPI(IppStatus, ippiFilterDeblocking_HorEdge_H263_8u_I,
	(Ipp8u * pSrcDst, int step, int QP))

IPPAPI(IppStatus, ippiFilterDeblocking_VerEdge_H263_8u_I,
	(Ipp8u * pSrcDst, int step, int QP))


/* Middle Level */
IPPAPI(IppStatus, ippiDecodeBlockCoef_Intra_H263_1u8u,
	(Ipp8u ** ppBitStream, int * pBitOffset, Ipp8u * pDst, int step, int QP))

IPPAPI(IppStatus, ippiDecodeBlockCoef_Inter_H263_1u16s,
	(Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pDst, int QP))


/***** MPEG-4 Functions *****/

/* motion vector decoding (+ padding) */
IPPAPI(IppStatus, ippiDecodePadMV_PVOP_MPEG4,
	(Ipp8u ** ppBitStream, int * pBitOffset,
	 IppMotionVector * pSrcMVLeftMB, IppMotionVector * pSrcMVUpperMB, 
	 IppMotionVector * pSrcMVUpperRightMB, IppMotionVector * pDstMVCurMB,
	 Ipp8u * pTranspLeftMB, Ipp8u * pTranspUpperMB, Ipp8u * pTranspUpperRightMB, 
	 Ipp8u * pTranspCurMB, int fcodeForward, IppMacroblockType MBType))

IPPAPI(IppStatus, ippiDecodeMV_BVOP_Forward_MPEG4,
	(Ipp8u ** ppBitStream, int * pBitOffset,
	 IppMotionVector * pSrcDstMVF,int fcodeForward))

IPPAPI(IppStatus, ippiDecodeMV_BVOP_Backward_MPEG4,
	(Ipp8u ** ppBitStream, int * pBitOffset,
	 IppMotionVector * pSrcDstMVB,int fcodeBackward))

IPPAPI(IppStatus, ippiDecodeMV_BVOP_Interpolate_MPEG4,
	(Ipp8u ** ppBitStream, int * pBitOffset,
	 IppMotionVector * pSrcDstMVF, IppMotionVector * pSrcDstMVB,
	 int fcodeForward, int fcodeBackward))

IPPAPI(IppStatus, ippiDecodeMV_BVOP_Direct_MPEG4,
	(Ipp8u ** ppBitStream, int * pBitOffset,
	 const IppMotionVector * pSrcMV, IppMotionVector * pDstMVF, IppMotionVector * pDstMVB,
	 Ipp8u *pTranspCurMB, int TRB, int TRD))

IPPAPI(IppStatus, ippiDecodeMV_BVOP_DirectSkip_MPEG4,
	(const IppMotionVector * pSrcMV, IppMotionVector * pDstMVF, IppMotionVector * pDstMVB,
	 Ipp8u *pTranspCurMB, int TRB, int TRD))

IPPAPI(IppStatus,ippiLimitMVToRect_MPEG4,
	 (const IppMotionVector * pSrcMV,IppMotionVector *pDstMV,IppiRect *pRectVOPRef, int Xcoord,
	  int Ycoord,int size))


/* coefficient prediction + reconstruction */
IPPAPI(IppStatus, ippiPredictReconCoefIntra_MPEG4_16s,
	(Ipp16s * pSrcDst, Ipp16s * pPredBufRow, Ipp16s * pPredBufCol,
	 int curQP, int predQP, int predDir, int ACPredFlag, IppVideoComponent videoComp))

/* motion padding */
IPPAPI(IppStatus, ippiPadMBHorizontal_MPEG4_8u,
	(const Ipp8u * pSrcY, const Ipp8u * pSrcCb, 
	 const Ipp8u * pSrcCr, const Ipp8u * pSrcA, 
	 Ipp8u * pDstY, Ipp8u * pDstCb, Ipp8u * pDstCr, Ipp8u * pDstA, int stepYA, int stepCbCr))

IPPAPI(IppStatus, ippiPadMBVertical_MPEG4_8u, 
	   (const Ipp8u * pSrcY, const Ipp8u * pSrcCb, 
	    const Ipp8u * pSrcCr, const Ipp8u * pSrcA,
		Ipp8u * pDstY, Ipp8u * pDstCb, Ipp8u * pDstCr, Ipp8u * pDstA, int stepYA, int stepCbCr))

IPPAPI(IppStatus, ippiPadMBGray_MPEG4_8u, (Ipp8u grayVal, Ipp8u * pDstY, 
	   Ipp8u * pDstCb, Ipp8u * pDstCr, Ipp8u * pDstA, int stepYA, int stepCbCr))

IPPAPI(IppStatus, ippiPadCurrent_16x16_MPEG4_8u_I,
	(const Ipp8u * pSrcBAB, int stepBinary, Ipp8u * pSrcDst, int stepTexture))

IPPAPI(IppStatus, ippiPadCurrent_8x8_MPEG4_8u_I,
	(const Ipp8u * pSrcBAB, int stepTexture, Ipp8u * pSrcDst))


/* vector padding */
IPPAPI(IppStatus, ippiPadMV_MPEG4,
	(IppMotionVector * pSrcDstMV, Ipp8u * pTransp))

/* inverse quantization */

IPPAPI(IppStatus, ippiQuantInvIntra_MPEG4_16s_I,
	(Ipp16s * pSrcDst, int QP, const Ipp8u * pQMatrix, IppVideoComponent videoComp))

IPPAPI(IppStatus, ippiQuantInvInter_MPEG4_16s_I,
	(Ipp16s * pSrcDst, int QP, const Ipp8u * pQMatrix))

/* vlc decoding + zigzag */
IPPAPI(IppStatus, ippiDecodeVLCZigzag_IntraDCVLC_MPEG4_1u16s,
	(Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pDst, int predDir, IppVideoComponent videoComp))

IPPAPI(IppStatus, ippiDecodeVLCZigzag_IntraACVLC_MPEG4_1u16s,
	(Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pDst, int predDir))

IPPAPI(IppStatus, ippiDecodeVLCZigzag_Inter_MPEG4_1u16s,
	(Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pDst))

/* block decoding */
IPPAPI(IppStatus, ippiDecodeBlockCoef_Intra_MPEG4_1u8u,
	(Ipp8u ** ppBitStream, int * pBitOffset, Ipp8u * pDst,
	 int step, Ipp16s * pCoefBufRow, Ipp16s * pCoefBufCol,
	 Ipp8u curQP, Ipp8u * pQPBuf, const Ipp8u * pQMatrix,
	 int blockIndex, int intraDCVLC, int ACPredFlag,
	 int bAlternateVerticalScanFlag, int bInterlaced, int bDCTType))

IPPAPI(IppStatus, ippiDecodeBlockCoef_Inter_MPEG4_1u16s,
	(Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pDst,
	 int QP, const Ipp8u * pQMatrix))

/* MVS decode*/
IPPAPI (IppStatus,ippiDecodeMVS_MPEG4, 
	(Ipp8u **ppBitStream, int *pBitOffset,	IppMotionVector	* pSrcDstMVS,
	const Ipp8u * pSrcBABMode, int stepBABMode, const IppMotionVector	* pSrcMVLeftMB,
	const IppMotionVector * pSrcMVUpperMB,const IppMotionVector	* pSrcMVUpperRightMB,
	const Ipp8u * pTranspLeftMB,const Ipp8u	* pTranspUpperMB,
	const Ipp8u	* pTranspUpperRightMB, int predFlag) ) 


/* IntraCAE decoding */
IPPAPI(IppStatus, ippiDecodeCAEIntraH_MPEG4_1u8u, (Ipp8u ** ppBitStream, 
		int * pBitOffset, Ipp8u * pBinarySrcDst, int step, int blocksize))
	
IPPAPI(IppStatus, ippiDecodeCAEIntraV_MPEG4_1u8u, (Ipp8u ** ppBitStream, 
		int * pBitOffset, Ipp8u * pBinarySrcDst, int step, int blocksize))

/* InterCAE decoding */
IPPAPI(IppStatus, ippiDecodeCAEInterH_MPEG4_1u8u,	(Ipp8u ** ppBitStream, 
		int * pBitOffset, const Ipp8u * pBinarySrcPred, int offsetPred, Ipp8u * pBinarySrcDst, int step, int blocksize))

IPPAPI(IppStatus, ippiDecodeCAEInterV_MPEG4_1u8u, (Ipp8u ** ppBitStream, 
		int * pBitOffset, const Ipp8u * pBinarySrcPred, int offsetPred, Ipp8u * pBinarySrcDst, int step, int blocksize))


/* Middle level padding, respectively for Opaque, Transparent and Partial */
IPPAPI(IppStatus, ippiPadMBPartial_MPEG4_8u_P4R, (const Ipp8u * pSrcBAB, 
 const Ipp32u * pSrcTrasptMBLeft, Ipp8u * pSrcDstCurrY, Ipp8u * pSrcDstCurrCb,
 Ipp8u * pSrcDstCurrCr, Ipp8u * pSrcDstCurrA, Ipp8u * pSrcDstPadded,
 int iMBX, int iMBY, int stepYA, int stepCbCr, int stepBinary))


IPPAPI(IppStatus, ippiPadMBTransparent_MPEG4_8u_P4R, (const Ipp32u * pSrcTrasptMBLeft, 
 Ipp8u * pSrcDstCurrY, Ipp8u * pSrcDstCurrCb, Ipp8u * pSrcDstCurrCr, Ipp8u * pSrcDstCurrA,
 Ipp8u * pSrcDstPadded, Ipp8u grayVal, int iMBX, int iMBY, int iMBXLimit, int iMBYLimit, 
 int stepYA, int stepCbCr))

IPPAPI(IppStatus, ippiPadMBOpaque_MPEG4_8u_P4R, ( const Ipp32u * pSrcTrasptMBLeft,
 Ipp8u * pSrcDstCurrY, Ipp8u * pSrcDstCurrCb, Ipp8u * pSrcDstCurrCr, Ipp8u * pSrcDstCurrA, Ipp8u * pSrcDstPadded, 
 int iMBX, int iMBY, int stepYA, int stepCbCr))


/*********** H.264 Decoder functions ************/
/* Interpolation */
IPPAPI(IppStatus, ippiInterpolateLuma_H264_8u_C1R, (const Ipp8u* pSrc,
 			 Ipp32s srcStep, Ipp8u* pDst, Ipp32s dstStep, int var, int nWidth, int nHeight))
IPPAPI(IppStatus, ippiInterpolateChroma_H264_8u_C1R, (const Ipp8u* pSrc, Ipp8u* pDst, 
       Ipp32s srcStep, Ipp32s dstStep, int width, int height, Ipp32s dx, Ipp32s dy))


/* Intra Prediction */
IPPAPI(IppStatus, ippiPredictIntra_4x4_H264_8u_C1R, (Ipp8u* pSrcLeft, 
 Ipp8u *pSrcAbove, Ipp8u *pSrcAboveLeft, Ipp8u* pDst,  int leftStep, 
 int dstStep, IppIntra4x4PredMode_H264 predMode, Ipp32u availability))

IPPAPI(IppStatus, ippiPredictIntra_16x16_H264_8u_C1R, (Ipp8u* pSrcLeft, 
 Ipp8u *pSrcAbove, Ipp8u *pSrcAboveLeft, Ipp8u* pDst, int leftStep, 
 int dstStep, IppIntra16x16PredMode_H264 predMode, Ipp32u availability))

IPPAPI(IppStatus, ippiPredictIntraChroma8x8_H264_8u_C1R, (Ipp8u* pSrcLeft, 
 Ipp8u *pSrcAbove, Ipp8u *pSrcAboveLeft, Ipp8u* pDst, int leftStep, 
 int dstStep, IppIntraChromaPredMode_H264 predMode, Ipp32u availability))

/* Dequant/Transform */
IPPAPI(IppStatus, ippiTransformDequantLumaDCFromPair_H264_8u16s_C1,
 (Ipp8u** ppSrc, Ipp16s* pDst, int QP))

IPPAPI(IppStatus, ippiTransformDequantChromaDCFromPair_H264_8u16s_C1,
 (Ipp8u** ppSrc, Ipp16s* pDst, int QP))

IPPAPI(IppStatus, ippiDequantTransformResidualFromPairAndAdd_H264_8u_C1,
 (Ipp8u** ppSrc, const Ipp8u* pPred, Ipp16s* pDC, Ipp8u* pDst, 
 int predStep, int dstStep, int QP, int AC))

/* Deblock filter */
IPPAPI(IppStatus, ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR, (Ipp8u* pSrcDst,
 Ipp32s srcdstStep, Ipp8u* pAlpha, Ipp8u* pBeta, Ipp8u* pThresholds, Ipp8u *pBS))

IPPAPI(IppStatus, ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR, (Ipp8u* pSrcDst,
 Ipp32s srcdstStep, Ipp8u* pAlpha, Ipp8u* pBeta, Ipp8u* pThresholds, Ipp8u *pBS))

IPPAPI(IppStatus, ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR, (Ipp8u* pSrcDst,
 Ipp32s srcdstStep, Ipp8u* pAlpha, Ipp8u* pBeta, Ipp8u* pThresholds, Ipp8u *pBS))

IPPAPI(IppStatus, ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR, (Ipp8u* pSrcDst,
 Ipp32s srcdstStep, Ipp8u* pAlpha, Ipp8u* pBeta, Ipp8u* pThresholds, Ipp8u *pBS))

/* VLC decoding */
IPPAPI(IppStatus, ippiDecodeCoeffsToPairCAVLC_H264_1u8u, (Ipp8u** ppBitStream,
 Ipp32s* pOffset, Ipp8u* pNumCoeff, Ipp8u** ppPosCoefbuf, int sVLCSelect,
 int  sMaxNumCoeff))

IPPAPI(IppStatus, ippiDecodeChromaDCCoeffsToPairCAVLC_H264_1u8u, (
 Ipp8u** ppBitStream, Ipp32s* pOffset, Ipp8u* pNumCoeff, Ipp8u** ppPosCoefbuf))

/********************* Video Encoding Functions *********************/
/* motion estimation */
IPPAPI(IppStatus, ippiSumNorm_VOP_MPEG4_8u16u, (Ipp8u * pSrcRef, IppiRect *pSrcRefRect,
	   Ipp16u * pDstSumRef, int flag, int step))

IPPAPI(IppStatus, ippiBlockMatch_Integer_16x16_SEA, (Ipp8u * pSrcRef, Ipp8u * pSrcCurr, 
				Ipp16u *pSrcSumBlk, IppMotionVector *pSrcRefMV, 
				IppCoordinate * pSrcPointPos, IppiRect * pSrcRefRect,
				int * pSrcDstminSAD, IppMotionVector * pDstMV,
				int step, int searchRange, int flag))

IPPAPI(IppStatus, ippiMotionEstimation_16x16_SEA, (Ipp8u * pSrcRef, 
		Ipp8u * pSrcReconRef, Ipp16u *pSrcSumBlk, Ipp8u * pSrcCurr,
		IppiRect * pSrcRefRect, IppCoordinate * pSrcPointPos,
		IppMotionVector *pSrcRefMV, IppMotionVector * pDstMV,
		Ipp8u *pDstPreMbtype, int *pDstSAD,
		int step, int roundingControl, int searchRange, int flag))

IPPAPI(IppStatus, ippiBlockMatch_Integer_16x16_MVFAST, (Ipp8u * pSrcRef, 
	    Ipp8u *pSrcCurr, IppMotionVector *pSrcCanMV, 
		IppMotionVector * pSrcRefMV, IppCoordinate * pSrcPointPos, 
		IppiRect * pSrcRefRect, Ipp8u * pSrcChkedPtMap16,  int *pFlag,
		int * pSrcDstSAD, IppMotionVector * pDstMV,
		int refStep, int searchRange))

IPPAPI(IppStatus, ippiMotionEstimation_16x16_MVFAST, (Ipp8u * pSrcRef, 
	    Ipp8u * pSrcReconRef, Ipp8u * pSrcCurr, 
	    IppMotionVector *pSrcCanMV, IppMotionVector *pSrcRefMV, 
		IppCoordinate * pSrcPointPos, IppiRect * pSrcRefRect, 
		Ipp8u * pSrcChkedPtMap16, Ipp8u * pSrcChkedPtMap8, IppMotionVector * pDstMV, 
		Ipp8u * pDstPreMBtype, int * pDstSAD, 
		int  step,  int roundingControl, 
		int searchRange))

/*Quantization*/
IPPAPI(IppStatus, ippiQuantInter_MPEG4_16s_I,
 		(Ipp16s * pSrcDst, Ipp8u QP, const int * pQMatrix))

IPPAPI(IppStatus, ippiQuantIntra_MPEG4_16s_I,
	   (Ipp16s * pSrcDst, Ipp8u QP, int blockIndex, const int * pQMatrix))

/*DCT*/
IPPAPI(IppStatus, ippiDCT8x8Fwd_Video_16s_C1I, (Ipp16s * pSrcDst))

IPPAPI(IppStatus, ippiDCT8x8Fwd_Video_16s_C1,
	   (const Ipp16s * pSrc, Ipp16s * pDst))

IPPAPI(IppStatus, ippiDCT8x8Fwd_Video_8u16s_C1R,
	   (const Ipp8u * pSrc, int srcStep, Ipp16s * pDst))

/*VLC encode*/
IPPAPI(IppStatus, ippiEncodeVLCZigzag_IntraDCVLC_MPEG4_16s1u, (Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s *pQDctBlkCoef, Ipp8u predDir,Ipp8u pattern, IppVideoComponent videoComp, int iNonZeroNum))
IPPAPI(IppStatus, ippiEncodeVLCZigzag_IntraACVLC_MPEG4_16s1u, (Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pQDctBlkCoef, Ipp8u predDir,Ipp8u pattern, int iNonZeroNum))
IPPAPI(IppStatus, ippiEncodeVLCZigzag_Inter_MPEG4_16s1u,(Ipp8u **ppBitStream,int * pBitOffset, Ipp16s *pQDctBlkCoef,Ipp8u pattern, int iNonZeroNum))

/* block encode */
IPPAPI(IppStatus, ippiTransRecBlockCoef_inter_MPEG4 , (Ipp16s *pSrc, 
		Ipp16s * pDst,Ipp16s * pRec, Ipp8u QP, const int * pQMatrix))

IPPAPI(IppStatus, ippiTransRecBlockCoef_intra_MPEG4 , (Ipp8u *pSrc,Ipp16s * pDst,
		Ipp8u * pRec,Ipp16s *pPredBufRow,Ipp16s *pPredBufCol,Ipp16s * pPreACPredict,
		int *pSumErr,int blockIndex, Ipp8u QP, Ipp8u *pQpBuf, int srcStep, int dstStep,
		const int * pQMatrix))

/* MV encode */
IPPAPI(IppStatus, ippiFindMVpred_MPEG4,(IppMotionVector* pSrcMVCurMB, 
	IppMotionVector* pSrcCandMV1,IppMotionVector* pSrcCandMV2,IppMotionVector* pSrcCandMV3,
	Ipp8u* pSrcCandTransp1,Ipp8u* pSrcCandTransp2,Ipp8u* pSrcCandTransp3, 
	Ipp8u* pSrcTranspCurr, IppMotionVector* pDstMVPred, IppMotionVector* pDstMVPredME, 
	int iBlk))

IPPAPI(IppStatus, ippiEncodeMV_MPEG4_8u16s, (Ipp8u **ppBitStream, int *pBitOffset,
	IppMotionVector* pMVCurMB, 	IppMotionVector* pSrcMVLeftMB,			
	IppMotionVector* pSrcMVUpperMB,	IppMotionVector* pSrcMVUpperRightMB,	
	Ipp8u* pTranspCurMB, Ipp8u* pTranspLeftMB, Ipp8u* pTranspUpperMB,
	Ipp8u* pTranspUpperRightMB, int fcodeForward, IppMacroblockType MBType))

/* Compute Texture Error */
IPPAPI(IppStatus, ippiComputeTextureErrorBlock_SAD_8u16s,
(const Ipp8u *pSrc, int srcStep, const Ipp8u *pSrcRef, Ipp16s * pDst, int *pDstSAD))

IPPAPI(IppStatus, ippiComputeTextureErrorBlock_8u16s,
(const Ipp8u *pSrc, int srcStep, const Ipp8u *pSrcRef, Ipp16s * pDst))




#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _IPPVC_H_ */

/* EOF */

