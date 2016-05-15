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
#ifndef __KINOMA_IPPS_H__
#define __KINOMA_IPPS_H__

#include "ippac.h"
#include "ippdc.h"
#include "ipps.h"
#include "kinoma_ipp_common.h"

#ifdef __cplusplus
extern "C" {
#endif


//ippac
IppStatus __STDCALL ippsSynthPQMF_MP3_32s16s_c(Ipp32s* pSrcY, Ipp16s* pDstAudioOut, Ipp32s* pVBuffer, int* pVPosition, int mode);
IppStatus __STDCALL ippsVLCDecodeEscBlock_AAC_1u16s_c(Ipp8u **ppBitStream, int* pBitOffset, Ipp16s *pData, int len, const IppsVLCDecodeSpec_32s *pVLCSpec);
IppStatus __STDCALL ippsVLCDecodeEscBlock_MP3_1u16s_c(Ipp8u **ppBitStream, int *pBitOffset, int linbits, Ipp16s *pData, int len, const IppsVLCDecodeSpec_32s *pVLCSpec);

//ippdc
IppStatus __STDCALL ippsVLCDecodeBlock_1u16s_c (Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s* pDst, int dstLen, const IppsVLCDecodeSpec_32s* pVLCSpec);
void	  __STDCALL ippsVLCDecodeFree_32s_c (IppsVLCDecodeSpec_32s* pVLCSpec);
IppStatus __STDCALL ippsVLCDecodeInitAlloc_32s_c (const IppsVLCTable_32s* pInputTable, int inputTableSize, Ipp32s* pSubTablesSizes, int numSubTables, IppsVLCDecodeSpec_32s** ppVLCSpec);
IppStatus __STDCALL ippsVLCDecodeOne_1u16s_c (Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s* pDst, const IppsVLCDecodeSpec_32s* pVLCSpec);


//ipps
Ipp8u*    __STDCALL  ippsMalloc_8u_c		(int len);
Ipp32s*   __STDCALL  ippsMalloc_32s_c		(int len);
void	  __STDCALL ippsFree_c				(void* ptr);
IppStatus __STDCALL ippsCopy_8u_c			(const Ipp8u* pSrc, Ipp8u* pDst, int len);
IppStatus __STDCALL ippsCopy_32s_c			(const Ipp32s* pSrc, Ipp32s* pDst, int len);
IppStatus __STDCALL ippsZero_8u_c			(Ipp8u* pDst, int len);
IppStatus __STDCALL  ippsZero_16s_c			(Ipp16s* pDst, int len);
IppStatus __STDCALL ippsZero_32s_c			(Ipp32s* pDst, int len);
IppStatus __STDCALL ippsZero_32sc_c			(Ipp32sc* pDst, int len);
IppStatus __STDCALL ippsAdd_32s_Sfs_c		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsAdd_32s_ISfs_c		(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor);
IppStatus __STDCALL ippsMul_32s_ISfs_c		(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor);
IppStatus __STDCALL ippsMul_32s_Sfs_c		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst,int len, int scaleFactor);
IppStatus __STDCALL ippsSub_32s_Sfs_c		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsSortAscend_32s_I_c	(Ipp32s* pSrcDst, int len);
IppStatus __STDCALL ippsMinMax_32s_c		(const Ipp32s* pSrc, int len, Ipp32s* pMin, Ipp32s* pMax);
IppStatus __STDCALL ippsMax_32s_c			(const Ipp32s* pSrc, int len, Ipp32s* pMax);
IppStatus __STDCALL ippsMax_16s_c			(const Ipp16s* pSrc, int len, Ipp16s* pMax);
IppStatus __STDCALL ippsMin_32s_c			(const Ipp32s* pSrc, int len, Ipp32s* pMin);
IppStatus __STDCALL ippsMaxAbs_32s_c		(const Ipp32s* pSrc, int len, Ipp32s* pMaxAbs);
IppStatus __STDCALL ippsMove_32s_c			(const Ipp32s* pSrc, Ipp32s* pDst, int len);
IppStatus __STDCALL ippsDiv_32s_ISfs_c		(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int ScaleFactor);
IppStatus __STDCALL ippsSqrt_64s_ISfs_c		(Ipp64s* pSrcDst, int len, int scaleFactor);
IppStatus __STDCALL ippsConvert_64s32s_Sfs_c(const Ipp64s* pSrc, Ipp32s* pDst, int len, IppRoundMode rndMode, int scaleFactor);
IppStatus __STDCALL ippsConvert_32s16s_Sfs_c(const Ipp32s* pSrc, Ipp16s* pDst, int len, int scaleFactor);
IppStatus  __STDCALL ippsLShiftC_32s_I_c	(int val, Ipp32s* pSrcDst, int len);
IppStatus __STDCALL ippsRShiftC_32s_I_c		(int val, Ipp32s* pSrcDst, int len);
/* the following 4 functions used in MDCT*/
IppStatus __STDCALL ippsFFTInitAlloc_C_32sc_c(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint);
IppStatus __STDCALL ippsFFTGetBufSize_C_32sc_c(const IppsFFTSpec_C_32sc* pFFTSpec, int* pSize);
IppStatus __STDCALL ippsFFTFree_C_32sc_c	(IppsFFTSpec_C_32sc* pFFTSpec);
IppStatus __STDCALL ippsFFTFwd_CToC_32sc_Sfs_c(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer);
/* the following 6 functions exist in sbr code */
IppStatus __STDCALL ippsFFTGetSize_C_32sc_c	(int order, int flag, IppHintAlgorithm hint, int* pSizeSpec, int* pSizeInit, int* pSizeBuf);
IppStatus __STDCALL ippsFFTInit_C_32sc_c	(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit);
IppStatus __STDCALL ippsFFTInv_CToC_32sc_Sfs_c(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer);
IppStatus __STDCALL ippsFFTGetSize_C_32s_c	(int order, int flag, IppHintAlgorithm hint, int* pSizeSpec, int* pSizeInit, int* pSizeBuf);
IppStatus __STDCALL ippsFFTInit_C_32s_c		(IppsFFTSpec_C_32s** ppFFTSpec, int order, int flag,IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit);
IppStatus __STDCALL ippsFFTInv_CToC_32s_Sfs_c(const Ipp32s* pSrcRe, const Ipp32s* pSrcIm, Ipp32s* pDstRe, Ipp32s* pDstIm, const IppsFFTSpec_C_32s* pFFTSpec, int scaleFactor, Ipp8u* pBuffer);
/*following functions are for 5.1  aac*/
IppStatus __STDCALL ippsMul_32sc_Sfs_c		(const Ipp32sc* pSrc1, const Ipp32sc* pSrc2, Ipp32sc* pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsMul_32s32sc_Sfs_c	(const Ipp32s* pSrc1, const Ipp32sc* pSrc2, Ipp32sc* pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsMulC_32s_Sfs_c		(const Ipp32s*  pSrc, Ipp32s  val,Ipp32s*  pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsSet_8u_c			(Ipp8u val, Ipp8u* pDst, int len);
/* following functions only used in mp3*/
IppStatus __STDCALL ippsAddC_32s_ISfs_c		(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor);
IppStatus __STDCALL ippsMulC_32s_ISfs_c		(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor);


#ifdef __cplusplus
}
#endif


#endif
