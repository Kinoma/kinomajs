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
#include "Fsk.h"
#include "kinoma_ipp_lib.h"

#include "ippi.h"
#include "ippac.h"
#include "kinoma_ipp_common.h"

//function pointers
//common
//ipps
Ipp8u*    (__STDCALL  *ippsMalloc_8u_universal)			(int len)=NULL;
void	  (__STDCALL *ippsFree_universal)				(void* ptr)=NULL;
IppStatus (__STDCALL *ippsZero_8u_universal)			(Ipp8u* pDst, int len)=NULL;
IppStatus (__STDCALL  *ippsZero_16s_universal)			(Ipp16s* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsZero_32s_universal)			(Ipp32s* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsZero_32sc_universal)			(Ipp32sc* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsCopy_8u_universal)			(const Ipp8u* pSrc, Ipp8u* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsSet_8u_universal)				(Ipp8u val, Ipp8u* pDst, int len)=NULL;
IppStatus (__STDCALL *ippiCopy_8u_C1R_universal) 		(const Ipp8u *pSrc, int srcStep, Ipp8u *pDst, int dstStep, IppiSize roiSize)=NULL;

#if defined(KINOMA_MP3) || defined(KINOMA_AAC) || defined(KINOMA_MP4V_ENC)
IppStatus (__STDCALL *ippsSortAscend_32s_I_universal)	(Ipp32s* pSrcDst, int len)=NULL;
Ipp32s*   (__STDCALL  *ippsMalloc_32s_universal)		(int len)=NULL;
#endif

#if defined(KINOMA_MP3) || defined(KINOMA_AAC) ||  defined(KINOMA_AAC_ENC)
IppStatus (__STDCALL *ippsCopy_32s_universal)			(const Ipp32s* pSrc, Ipp32s* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsAdd_32s_Sfs_universal)		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsAdd_32s_ISfs_universal)		(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_32s_ISfs_universal)		(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_32s_Sfs_universal)		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst,int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsSub_32s_Sfs_universal)		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMinMax_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMin, Ipp32s* pMax)=NULL;
IppStatus (__STDCALL *ippsMax_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMax)=NULL;
IppStatus (__STDCALL *ippsMax_16s_universal)			(const Ipp16s* pSrc, int len, Ipp16s* pMax)=NULL;
IppStatus (__STDCALL *ippsMin_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMin)=NULL;
IppStatus (__STDCALL *ippsMaxAbs_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMaxAbs)=NULL;
IppStatus (__STDCALL *ippsMove_32s_universal)			(const Ipp32s* pSrc, Ipp32s* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsDiv_32s_ISfs_universal)		(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int ScaleFactor)=NULL;
IppStatus (__STDCALL *ippsSqrt_64s_ISfs_universal)		(Ipp64s* pSrcDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsConvert_64s32s_Sfs_universal)	(const Ipp64s* pSrc, Ipp32s* pDst, int len, IppRoundMode rndMode, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsConvert_32s16s_Sfs_universal)	(const Ipp32s* pSrc, Ipp16s* pDst, int len, int scaleFactor)=NULL;
IppStatus  (__STDCALL *ippsLShiftC_32s_I_universal)		(int val, Ipp32s* pSrcDst, int len)=NULL;
IppStatus (__STDCALL *ippsRShiftC_32s_I_universal)		(int val, Ipp32s* pSrcDst, int len)=NULL;

/* following functions only used in mp3*/
IppStatus (__STDCALL *ippsAddC_32s_ISfs_universal)		(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMulC_32s_ISfs_universal)		(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor)=NULL;

#endif

#if defined(KINOMA_MP3) || defined(KINOMA_AAC)
//ippdc
IppStatus (__STDCALL *ippsVLCDecodeBlock_1u16s_universal)		(Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s* pDst, int dstLen, const IppsVLCDecodeSpec_32s* pVLCSpec)=NULL;
void	  (__STDCALL *ippsVLCDecodeFree_32s_universal)			(IppsVLCDecodeSpec_32s* pVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCDecodeInitAlloc_32s_universal )	(const IppsVLCTable_32s* pInputTable, int inputTableSize, Ipp32s* pSubTablesSizes, int numSubTables, IppsVLCDecodeSpec_32s** ppVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCDecodeOne_1u16s_universal)			(Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s* pDst, const IppsVLCDecodeSpec_32s* pVLCSpec)=NULL;
#endif

#if defined(KINOMA_AAC) || defined(KINOMA_AAC_ENC)
IppStatus (__STDCALL *ippsVLCDecodeEscBlock_AAC_1u16s_universal) (Ipp8u **ppBitStream, int* pBitOffset, Ipp16s *pData, int len, const IppsVLCDecodeSpec_32s *pVLCSpec) = NULL;

/*following functions are for 5.1  aac*/
IppStatus (__STDCALL *ippsMul_32sc_Sfs_universal)		(const Ipp32sc* pSrc1, const Ipp32sc* pSrc2, Ipp32sc* pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_32s32sc_Sfs_universal)	(const Ipp32s* pSrc1, const Ipp32sc* pSrc2, Ipp32sc* pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMulC_32s_Sfs_universal)		(const Ipp32s*  pSrc, Ipp32s  val,Ipp32s*  pDst, int len, int scaleFactor)=NULL;

/* the following 4 functions used in MDCT*/
IppStatus (__STDCALL *ippsFFTInitAlloc_C_32sc_universal)(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint)=NULL;
IppStatus (__STDCALL *ippsFFTGetBufSize_C_32sc_universal)(const IppsFFTSpec_C_32sc* pFFTSpec, int* pSize)=NULL;
IppStatus (__STDCALL *ippsFFTFree_C_32sc_universal)		(IppsFFTSpec_C_32sc* pFFTSpec)=NULL;
IppStatus (__STDCALL *ippsFFTFwd_CToC_32sc_Sfs_universal)(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)=NULL;

#endif


#if defined(KINOMA_MP3)
//ippac

IppStatus (__STDCALL *ippsSynthPQMF_MP3_32s16s_universal)		  (Ipp32s* pSrcY, Ipp16s* pDstAudioOut, Ipp32s* pVBuffer, int* pVPosition, int mode) = NULL;
IppStatus (__STDCALL *ippsVLCDecodeEscBlock_MP3_1u16s_universal)  (Ipp8u **ppBitStream, int *pBitOffset, int linbits, Ipp16s *pData, int len, const IppsVLCDecodeSpec_32s *pVLCSpec) = NULL;
#endif


#if defined(KINOMA_AAC)

IppStatus (__STDCALL *ippsFFTInv_CToC_32s_Sfs_universal)(const Ipp32s* pSrcRe, const Ipp32s* pSrcIm, Ipp32s* pDstRe, Ipp32s* pDstIm, const IppsFFTSpec_C_32s* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)=NULL;
IppStatus (__STDCALL *ippsFFTInit_C_32s_universal)		(IppsFFTSpec_C_32s** ppFFTSpec, int order, int flag,IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit)=NULL;

/* the following 6 functions exist in sbr code */
IppStatus (__STDCALL *ippsFFTGetSize_C_32sc_universal)	(int order, int flag, IppHintAlgorithm hint, int* pSizeSpec, int* pSizeInit, int* pSizeBuf)=NULL;
IppStatus (__STDCALL *ippsFFTInit_C_32sc_universal)		(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit)=NULL;
IppStatus (__STDCALL *ippsFFTInv_CToC_32sc_Sfs_universal)(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)=NULL;
IppStatus (__STDCALL *ippsFFTGetSize_C_32s_universal)	(int order, int flag, IppHintAlgorithm hint, int* pSizeSpec, int* pSizeInit, int* pSizeBuf)=NULL;

#endif


#if defined(KINOMA_AAC_ENC) 
IppStatus (__STDCALL *ippsCopy_16s_universal)				(const Ipp16s* pSrc, Ipp16s* pDst, int len )=NULL;
IppStatus (__STDCALL *ippsSet_16s_universal)					( Ipp16s val, Ipp16s* pDst, int len )=NULL;
IppStatus (__STDCALL *ippsAbs_16s_I_universal)				(Ipp16s* pSrcDst,int len)=NULL;
IppStatus (__STDCALL *ippsAbs_16s_universal)					(const Ipp16s* pSrc, Ipp16s* pDst,int len)=NULL;
IppStatus (__STDCALL *ippsMinEvery_32s_I_universal)			(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len)=NULL;
IppStatus (__STDCALL *ippsMaxEvery_32s_I_universal)			(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len)=NULL;
IppStatus (__STDCALL *ippsMinMax_16s_universal)				(const Ipp16s* pSrc, int len, Ipp16s* pMin, Ipp16s* pMax)=NULL;
IppStatus (__STDCALL *ippsSum_16s32s_Sfs_universal)			(const Ipp16s*  pSrc, int len,Ipp32s*  pSum, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsSum_32s_Sfs_universal)				(const Ipp32s*  pSrc, int len, Ipp32s*  pSum, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsAdd_16s_Sfs_universal)				(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor)=NULL;

IppStatus (__STDCALL *ippsAdd_16s_ISfs_universal)			(const Ipp16s*  pSrc, Ipp16s*  pSrcDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsAddC_16s_Sfs_universal)			(const Ipp16s*  pSrc, Ipp16s  val,Ipp16s*  pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsSub_16s_universal)					(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp16s*  pDst, int len)=NULL;
IppStatus (__STDCALL *ippsSub_16s_ISfs_universal)			(const Ipp16s*  pSrc, Ipp16s*  pSrcDst,int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsSub_16s_Sfs_universal)				(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor)=NULL;
																	
IppStatus (__STDCALL *ippsMul_16s_Sfs_universal)				(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_16s_ISfs_universal)			(const Ipp16s*  pSrc, Ipp16s*  pSrcDst,int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_16s32s_Sfs_universal)			(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp32s*  pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMulC_16s_Sfs_universal)			(const Ipp16s*  pSrc, Ipp16s  val,Ipp16s*  pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_16s_universal)					(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp16s*  pDst, int len)=NULL;

IppStatus (__STDCALL *ippsDiv_16s_Sfs_universal)				(const Ipp16s* pSrc1, const Ipp16s* pSrc2,Ipp16s* pDst, int len, int ScaleFactor)=NULL;
IppStatus (__STDCALL *ippsDiv_16s_ISfs_universal)			(const Ipp16s* pSrc, Ipp16s* pSrcDst,int len, int ScaleFactor)=NULL;
IppStatus (__STDCALL *ippsDivC_16s_ISfs_universal)			(Ipp16s val, Ipp16s* pSrcDst, int len, int ScaleFactor)=NULL;
IppStatus (__STDCALL *ippsSpread_16s_Sfs_universal)			(Ipp16s src1, Ipp16s src2, int inScaleFactor, Ipp16s* pDst)=NULL;
IppStatus (__STDCALL *ippsPow34_16s_Sfs_universal)			(const Ipp16s* pSrc, int inScaleFactor, Ipp16s* pDst, int ScaleFactor, int len)=NULL;
IppStatus (__STDCALL *ippsMagnitude_16sc_Sfs_universal)		(const Ipp16sc* pSrc,Ipp16s* pDst, int len,int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMagnitude_16s_Sfs_universal)		(const Ipp16s* pSrcRe,const Ipp16s* pSrcIm,Ipp16s* pDst,int len,int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsRShiftC_32s_universal)				(const Ipp32s* pSrc, int val, Ipp32s* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsRShiftC_16s_universal)				(const Ipp16s* pSrc, int val, Ipp16s* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsLShiftC_16s_I_universal)			(int val, Ipp16s* pSrcDst, int len)=NULL;

IppStatus (__STDCALL *ippsDotProd_16s32s32s_Sfs_universal)	( const Ipp16s* pSrc1, const Ipp32s* pSrc2,int len, Ipp32s* pDp, int scaleFactor )=NULL;
IppStatus (__STDCALL *ippsDotProd_16s32s_Sfs_universal)		( const Ipp16s* pSrc1, const Ipp16s* pSrc2, int len, Ipp32s* pDp, int scaleFactor )=NULL;
IppStatus (__STDCALL *ippsLn_32s16s_Sfs_universal)			( const Ipp32s* pSrc, Ipp16s* pDst, int Len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsDeinterleave_16s_universal)		(const Ipp16s* pSrc, int ch_num,int len, Ipp16s** pDst)=NULL;

// AAC encoder VLC
IppStatus (__STDCALL *ippsVLCEncodeBlock_16s1u_universal)		(const Ipp16s* pSrc, int srcLen, Ipp8u** ppDst, int* pDstBitsOffset, const IppsVLCEncodeSpec_32s* pVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCEncodeEscBlock_AAC_16s1u_universal)	(const Ipp16s *pSrc,int srcLen,  Ipp8u **ppDst,  int *pDstBitsOffset, const IppsVLCEncodeSpec_32s *pVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCEncodeInitAlloc_32s_universal)		(const IppsVLCTable_32s* pInputTable, int inputTableSize, IppsVLCEncodeSpec_32s** ppVLCSpec)=NULL;
void (__STDCALL *ippsVLCEncodeFree_32s_universal)				(IppsVLCEncodeSpec_32s* pVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCCountBits_16s32s_universal)			(const Ipp16s* pSrc, int srcLen, Ipp32s* pCountBits, const IppsVLCEncodeSpec_32s* pVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCCountEscBits_AAC_16s32s_universal)	(const Ipp16s *pSrc, int srcLen,  Ipp32s *pCountBits,    const IppsVLCEncodeSpec_32s *pVLCSpec)=NULL;

IppStatus (__STDCALL *ippsThreshold_LT_16s_I_universal)			( Ipp16s* pSrcDst, int len,   Ipp16s level )=NULL;
IppStatus (__STDCALL *ippsConjPack_16sc_universal)				( const Ipp16s* pSrc, Ipp16sc* pDst, int lenDst )=NULL;
IppStatus (__STDCALL *ippsConjCcs_16sc_universal)				( const Ipp16s* pSrc, Ipp16sc* pDst, int lenDst )=NULL;

// FFT
IppStatus (__STDCALL *ippsFFTInitAlloc_R_32s_universal)				(IppsFFTSpec_R_32s** ppFFTSpec,int order, int flag, IppHintAlgorithm hint )=NULL;
IppStatus (__STDCALL *ippsFFTGetBufSize_R_32s_universal) 			(const IppsFFTSpec_R_32s*  pFFTSpec, int* pSize )=NULL;
IppStatus (__STDCALL *ippsFFTGetBufSize_R_16s_universal) 			(const IppsFFTSpec_R_16s*  pFFTSpec, int* pSize )=NULL;
IppStatus (__STDCALL *ippsFFTGetBufSize_C_16sc_universal) 			(const IppsFFTSpec_C_16sc*  pFFTSpec, int* pSize )=NULL;
IppStatus (__STDCALL *ippsMDCTFwdGetBufSize_16s_universal)			(const IppsMDCTFwdSpec_16s *pMDCTSpec, int *pSize)=NULL;
IppStatus (__STDCALL *ippsMDCTFwdFree_16s_universal)					(IppsMDCTFwdSpec_16s* pMDCTSpec)=NULL;
IppStatus (__STDCALL *ippsMDCTFwdInitAlloc_16s_universal)			(IppsMDCTFwdSpec_16s ** ppMDCTSpec, int len)=NULL;
IppStatus (__STDCALL *ippsMDCTFwd_16s_Sfs_universal)					(const Ipp16s *pSrc, Ipp16s *pDst,const IppsMDCTFwdSpec_16s* pMDCTSpec,int scaleFactor, Ipp8u* pBuffer)=NULL;
IppStatus (__STDCALL *ippsFFTInitAlloc_R_16s_universal)				( IppsFFTSpec_R_16s** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint )=NULL;
IppStatus (__STDCALL *ippsFFTInitAlloc_C_16s_universal)				( IppsFFTSpec_C_16s** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint )=NULL;
IppStatus (__STDCALL *ippsFFTInitAlloc_C_16sc_universal)				(IppsFFTSpec_C_16sc** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint )=NULL;
IppStatus (__STDCALL *ippsFFTFree_R_16s_universal) 					( IppsFFTSpec_R_16s*  pFFTSpec )=NULL;
IppStatus (__STDCALL *ippsFFTFree_C_16s_universal)					( IppsFFTSpec_C_16s*  pFFTSpec )=NULL;
IppStatus (__STDCALL *ippsFFTFree_C_16sc_universal)					( IppsFFTSpec_C_16sc* pFFTSpec )=NULL;
//IppStatus (__STDCALL *ippsFFTFwd_CToC_16sc_Sfs_universal)			(const Ipp16sc* pSrc, Ipp16sc* pDst, const IppsFFTSpec_C_16sc* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer )=NULL;
IppStatus (__STDCALL *ippsFFTFwd_RToCCS_16s_Sfs_universal)			(const Ipp16s* pSrc,  Ipp16s* pDst,  const IppsFFTSpec_R_16s* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer )=NULL;
IppStatus (__STDCALL *ippsFFTFwd_RToPack_32s_Sfs_universal)			(const Ipp32s* pSrc, Ipp32s* pDst,   const IppsFFTSpec_R_32s* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer )=NULL;
IppStatus (__STDCALL *ippsFFTInv_PackToR_32s_Sfs_universal)			(const Ipp32s* pSrc, Ipp32s* pDst, const IppsFFTSpec_R_32s* pFFTSpec,   int scaleFactor, Ipp8u* pBuffer )=NULL;

#endif

//ippi
//MPEG4 Video

#if defined(KINOMA_MP4V) || defined(KINOMA_MP4V_ENC) 
IppStatus (__STDCALL *ippiDCT8x8Inv_16s_C1I_universal)			(Ipp16s* pSrcDst)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Inv_16s8u_C1R_universal)		(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)=NULL;

IppStatus (__STDCALL *ippiWarpGetSize_MPEG4_universal)			( int*  pSpecSize)=NULL;
IppStatus (__STDCALL *ippiQuantInvInterInit_MPEG4_universal)	( const Ipp8u* pQuantMatrix, IppiQuantInvInterSpec_MPEG4* pSpec, int bitsPerPixel)=NULL;
IppStatus (__STDCALL *ippiQuantInvInterGetSize_MPEG4_universal) ( int* pSpecSize)=NULL;
IppStatus (__STDCALL *ippiQuantInvIntraInit_MPEG4_universal)	(const Ipp8u* pQuantMatrix, IppiQuantInvIntraSpec_MPEG4* pSpec, int bitsPerPixel)=NULL;
IppStatus (__STDCALL *ippiQuantInvIntraGetSize_MPEG4_universal) (int* pSpecSize)=NULL;

IppStatus (__STDCALL *ippiQuantInvIntra_MPEG4_16s_C1I_universal)			(Ipp16s* pCoeffs,int indxLastNonZero,const IppiQuantInvIntraSpec_MPEG4* pSpec, int QP,int blockType)=NULL;
IppStatus (__STDCALL *ippiAdd8x8_16s8u_C1IRS_universal)					(const Ipp16s* pSrc, int srcStep,Ipp8u* pSrcDst,int srcDstStep)=NULL;
IppStatus (__STDCALL *ippiOBMC8x8HP_MPEG4_8u_C1R_universal)				(const Ipp8u* pSrc, int srcStep, Ipp8u* pDst,int dstStep,const IppMotionVector* pMVCur,const IppMotionVector* pMVLeft, const IppMotionVector* pMVRight,const IppMotionVector* pMVAbove, const IppMotionVector* pMVBelow, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy8x8QP_MPEG4_8u_C1R_universal)				(const Ipp8u* pSrc,int srcStep,Ipp8u* pDst,int dstStep,int acc,int rounding)=NULL;

IppStatus (__STDCALL *ippiOBMC8x8QP_MPEG4_8u_C1R_universal)				(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep, const IppMotionVector* pMVCur, const IppMotionVector* pMVLeft, const IppMotionVector* pMVRight,const IppMotionVector* pMVAbove, const IppMotionVector* pMVBelow,int rounding)=NULL;
IppStatus (__STDCALL *ippiAverage8x8_8u_C1IR_universal)					(const Ipp8u*  pSrc,int srcStep,Ipp8u* pSrcDst,int srcDstStep)=NULL;
IppStatus (__STDCALL *ippiAverage16x16_8u_C1IR_universal)				(const Ipp8u*  pSrc,int srcStep,Ipp8u* pSrcDst,int srcDstStep)=NULL;
IppStatus (__STDCALL *ippiCopy8x4HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy8x8HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy16x8HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy16x8QP_MPEG4_8u_C1R_universal)			(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy16x16HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy16x16QP_MPEG4_8u_C1R_universal)			(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy8x8_8u_C1R_universal)						(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep)=NULL;
IppStatus (__STDCALL *ippiCopy16x16_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep)=NULL;

IppStatus (__STDCALL *ippiCalcGlobalMV_MPEG4_universal)						(int xOffset, int yOffset, IppMotionVector* pGMV,const IppiWarpSpec_MPEG4*  pSpec)=NULL;
IppStatus (__STDCALL *ippiWarpChroma_MPEG4_8u_P2R_universal)				(const Ipp8u* pSrcCb, int srcStepCb,const Ipp8u* pSrcCr,int srcStepCr,Ipp8u* pDstCb, int dstStepCb,Ipp8u* pDstCr,int dstStepCr, const IppiRect* dstRect, const IppiWarpSpec_MPEG4* pSpec)=NULL;    
IppStatus (__STDCALL *ippiWarpLuma_MPEG4_8u_C1R_universal)					(const Ipp8u* pSrcY,int srcStepY, Ipp8u* pDstY, int dstStepY, const IppiRect* dstRect,const IppiWarpSpec_MPEG4* pSpec)=NULL;
IppStatus (__STDCALL *ippiWarpInit_MPEG4_universal)							(IppiWarpSpec_MPEG4* pSpec, const int* pDU, const int* pDV, int numWarpingPoints, int spriteType, int warpingAccuracy, int roundingType, int quarterSample, int fcode, const IppiRect* spriteRect, const IppiRect* vopRect)=NULL;
#endif

#if defined(KINOMA_MP4V)
IppStatus (__STDCALL *ippiDCT8x8Inv_4x4_16s_C1I_universal)		(Ipp16s* pSrcDst)=NULL;
IppStatus (__STDCALL *ippiChangeSpriteBrightness_MPEG4_8u_C1IR_universal) 	(Ipp8u*  pSrcDst, int srcDstStep, int width, int height, int brightnessChangeFactor)=NULL;
IppStatus (__STDCALL *ippiAdd8x8HP_16s8u_C1RS_universal)				(const Ipp16s* pSrc1,int src1Step,Ipp8u* pSrc2,int src2Step,Ipp8u* pDst,int dstStep, int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiReconstructCoeffsInter_MPEG4_1u16s_universal)	(Ipp8u**ppBitStream, int*pBitOffset,Ipp16s* pCoeffs,int* pIndxLastNonZero,int rvlcFlag,int scan,const IppiQuantInvInterSpec_MPEG4* pQuantInvInterSpec,int QP)=NULL;
IppStatus (__STDCALL *ippiDecodeDCIntra_MPEG4_1u16s_universal)			(Ipp8u **ppBitStream, int *pBitOffset,Ipp16s *pDC, int blockType)=NULL;
IppStatus (__STDCALL *ippiDecodeCoeffsIntra_MPEG4_1u16s_universal)		(Ipp8u**  ppBitStream,int* pBitOffset,Ipp16s*  pCoeffs, int* pIndxLastNonZero, int rvlcFlag, int noDCFlag,int scan)=NULL;
//IppStatus (__STDCALL *ippiReconstructCoeffsIntra_H263_1u16s_universal) ( Ipp8u** ppBitStream, int* pBitOffset, Ipp16s* pCoef, int* pIndxLastNonZero, int cbp, int QP, int advIntraFlag, int scan, int modQuantFlag)=NULL;
//IppStatus (__STDCALL *ippiReconstructCoeffsInter_H263_1u16s_universal) 	(Ipp8u** ppBitStream,int* pBitOffset,Ipp16s* pCoef,int* pIndxLastNonZero, int QP, int modQuantFlag)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Inv_2x2_16s_C1I_universal)		(Ipp16s* pSrcDst)=NULL;
#endif

#if defined(KINOMA_MP4V_ENC) 
IppStatus (__STDCALL *ippiSAD16x16_8u32s_universal)					(const Ipp8u*  pSrc,  Ipp32s  srcStep, const Ipp8u*  pRef,	Ipp32s  refStep,  Ipp32s* pSAD, Ipp32s  mcType)=NULL;
IppStatus (__STDCALL *ippiSAD16x8_8u32s_C1R_universal) 				(const Ipp8u  *pSrcCur,  int  srcCurStep,  const Ipp8u  *pSrcRef,int srcRefStep, Ipp32s *pDst, Ipp32s mcType)=NULL;
IppStatus (__STDCALL *ippiSAD8x8_8u32s_C1R_universal) 				(const Ipp8u*  pSrcCur, int srcCurStep, const Ipp8u*  pSrcRef, int     srcRefStep, Ipp32s* pDst,Ipp32s  mcType)=NULL;
IppStatus (__STDCALL *ippiMeanAbsDev16x16_8u32s_C1R_universal)		(const Ipp8u*  pSrc,  int srcStep, Ipp32s* pDst)=NULL;
IppStatus (__STDCALL *ippiQuantIntraInit_MPEG4_universal)			(const Ipp8u*  pQuantMatrix, IppiQuantIntraSpec_MPEG4* pSpec, int bitsPerPixel)=NULL;
IppStatus (__STDCALL *ippiQuantInterInit_MPEG4_universal) 			(const Ipp8u*  pQuantMatrix, IppiQuantInterSpec_MPEG4* pSpec, int bitsPerPixel)=NULL;
Ipp32u*	  (__STDCALL *ippsMalloc_32u_universal)			 			(int len)=NULL;
IppStatus (__STDCALL *ippiQuantInterGetSize_MPEG4_universal) 		(int* pSpecSize)=NULL;
IppStatus (__STDCALL *ippiQuantIntraGetSize_MPEG4_universal) 		(int* pSpecSize)=NULL;
IppStatus (__STDCALL *ippiQuantIntra_H263_16s_C1I_universal) 		(Ipp16s* pSrcDst,   int  QP,  int*  pCountNonZero,	int   advIntraFlag,  int   modQuantFlag)=NULL;
IppStatus (__STDCALL *ippiQuantIntra_MPEG4_16s_C1I_universal) 		(Ipp16s*  pCoeffs, const IppiQuantIntraSpec_MPEG4* pSpec, int  QP, int* pCountNonZero, int blockType)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Fwd_8u16s_C1R_universal) 			( const Ipp8u* pSrc, int srcStep, Ipp16s* pDst)=NULL;
IppStatus (__STDCALL *ippiQuantInter_H263_16s_C1I_universal) 		(Ipp16s* pSrcDst, int QP, int* pCountNonZero, int modQuantFlag)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Fwd_16s_C1I_universal) 				(Ipp16s* pSrcDst)=NULL;
IppStatus (__STDCALL *ippiSubSAD8x8_8u16s_C1R_universal) 			(const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, int src2Step, Ipp16s* pDst, int dstStep, Ipp32s* pSAD)=NULL;
IppStatus (__STDCALL *ippiFrameFieldSAD16x16_8u32s_C1R_universal) 	(const Ipp8u* pSrc, int  srcStep,	Ipp32s*  pFrameSAD,  Ipp32s*  pFieldSAD)=NULL;
IppStatus (__STDCALL *ippiSub8x8_8u16s_C1R_universal) 				(const Ipp8u*  pSrc1,  int src1Step, const Ipp8u*  pSrc2, int   src2Step, Ipp16s*  pDst,  int  dstStep)=NULL;
IppStatus (__STDCALL *ippiQuantInter_MPEG4_16s_C1I_universal) 		(Ipp16s* pCoeffs, const IppiQuantInterSpec_MPEG4* pSpec, int  QP,  int* pCountNonZero)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Fwd_16s_C1R_universal) 				(const Ipp16s* pSrc, int srcStep, Ipp16s* pDst )=NULL;
IppStatus (__STDCALL *ippsCopy_1u_universal) 						(const Ipp8u *pSrc, int srcBitOffset, Ipp8u *pDst, int dstBitOffset, int len)=NULL;
//IppStatus (__STDCALL *ippiCopy_8u_C1R_universal) 					(const Ipp8u *pSrc, int srcStep, Ipp8u *pDst, int dstStep, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiFrameFieldSAD16x16_16s32s_C1R_universal) 	(const Ipp16s* pSrc,  int  srcStep, Ipp32s* pFrameSAD, Ipp32s* pFieldSAD)=NULL;
IppStatus (__STDCALL *ippiSub16x16_8u16s_C1R_universal) 				(const Ipp8u*  pSrc1, int src1Step, const Ipp8u*  pSrc2, int   src2Step, Ipp16s*  pDst, int  dstStep)=NULL;
IppStatus (__STDCALL *ippiQuantInvIntra_H263_16s_C1I_universal) 		(Ipp16s* pSrcDst, int indxLastNonZero, int QP, int     advIntraFlag,int modQuantFlag)=NULL;
IppStatus (__STDCALL *ippiSqrDiff16x16_8u32s_universal) 				(const Ipp8u*  pSrc,Ipp32s  srcStep,const Ipp8u*  pRef, Ipp32s  refStep,Ipp32s  mcType,Ipp32s* pSqrDiff)=NULL;
IppStatus (__STDCALL *ippiSSD8x8_8u32s_C1R_universal) 				(const Ipp8u  *pSrcCur,  int srcCurStep, const Ipp8u  *pSrcRef, int srcRefStep, Ipp32s *pDst, Ipp32s mcType)=NULL;
IppStatus (__STDCALL *ippiQuantInvInter_H263_16s_C1I_universal) 		(Ipp16s* pSrcDst, int indxLastNonZero, int QP, int modQuantFlag)=NULL;
IppStatus (__STDCALL *ippiQuantInvInter_MPEG4_16s_C1I_universal) 	(Ipp16s*   pCoeffs, int  indxLastNonZero, const IppiQuantInvInterSpec_MPEG4* pSpec, int QP)=NULL;
IppStatus (__STDCALL *ippiEncodeCoeffsIntra_H263_16s1u_universal) 	(Ipp16s* pQCoef, Ipp8u** ppBitStream, int* pBitOffset, int  countNonZero, int  advIntraFlag, int modQuantFlag, int scan)=NULL;
IppStatus (__STDCALL *ippiEncodeDCIntra_H263_16s1u_universal) 		(Ipp16s  qDC, Ipp8u** ppBitStream, int* pBitOffset)=NULL;
IppStatus (__STDCALL *ippiCountZeros8x8_16s_C1_universal) 			(Ipp16s* pSrc, Ipp32u* pCount)=NULL;
IppStatus (__STDCALL *ippiEncodeCoeffsIntra_MPEG4_16s1u_universal) 	(const Ipp16s*  pCoeffs,  Ipp8u** ppBitStream, int* pBitOffset, int  countNonZero, int  rvlcFlag,int noDCFlag,int scan)=NULL;
IppStatus (__STDCALL *ippiEncodeDCIntra_MPEG4_16s1u_universal) 		(Ipp16s dcCoeff, Ipp8u**  ppBitStream,int*  pBitOffset, int blockType)=NULL;
IppStatus (__STDCALL *ippiEncodeCoeffsInter_H263_16s1u_universal) 	(Ipp16s* pQCoef, Ipp8u** ppBitStream, int*  pBitOffset, int  countNonZero, int modQuantFlag, int  scan)=NULL;
IppStatus (__STDCALL *ippiEncodeCoeffsInter_MPEG4_16s1u_universal) 	(const Ipp16s*  pCoeffs,  Ipp8u**  ppBitStream, int* pBitOffset,int  countNonZero,int rvlcFlag,int scan)=NULL;
#endif

#if defined(KINOMA_AVC) || defined(KINOMA_AVC_ENC)

IppStatus (__STDCALL *ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal)			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, Ipp8u*    pAlpha,  Ipp8u*    pBeta, Ipp8u*    pThresholds, Ipp8u*    pBs)=NULL;
IppStatus (__STDCALL *ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal)			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, Ipp8u*    pAlpha,  Ipp8u*    pBeta, Ipp8u*    pThresholds, Ipp8u*    pBs)=NULL;
IppStatus (__STDCALL *ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal)		(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, Ipp8u*    pAlpha,  Ipp8u*    pBeta, Ipp8u*    pThresholds, Ipp8u*    pBs)=NULL;
IppStatus (__STDCALL *ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal)		(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, Ipp8u*    pAlpha,  Ipp8u*    pBeta, Ipp8u*    pThresholds, Ipp8u*    pBs)=NULL;
IppStatus (__STDCALL *ippiInterpolateBlock_H264_8u_P2P1R_universal)						(Ipp8u *pSrc1,  Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u uWidth,Ipp32u uHeight,Ipp32s pitch)=NULL;
IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateChroma_H264_8u_C1R_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize)=NULL;
#endif

#if defined(KINOMA_AVC)
#include "umc_h264_dec_deblocking.h"

IppStatus (__STDCALL *ippiSet_8u_C1R_universal) 										(Ipp8u value, Ipp8u* pDst, int dstStep, IppiSize roiSize )=NULL;

IppStatus (__STDCALL *ippiDecodeExpGolombOne_H264_1u16s_universal)						(Ipp32u **ppBitStream, Ipp32s *pBitOffset,Ipp16s *pDst,Ipp8u isSigned)=NULL;//***
Ipp32s	  (__STDCALL *ippiDecodeExpGolombOne_H264_1u16s_signed_universal)				(Ipp32u **ppBitStream, Ipp32s *pBitOffset)=NULL;//***
Ipp32s    (__STDCALL *ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal)				(Ipp32u **ppBitStream, Ipp32s *pBitOffset)=NULL;//***
IppStatus (__STDCALL *ippiDecodeCAVLCCoeffs_H264_1u16s_universal)						(Ipp32u **ppBitStream,  Ipp32s *pOffset,   Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs,Ipp32u uVLCSelect,Ipp16s uMaxNumCoeff, const Ipp32s **ppTblCoeffToken, const Ipp32s **ppTblTotalZeros,const Ipp32s **ppTblRunBefore,  const Ipp32s *pScanMatrix)=NULL;//***
IppStatus (__STDCALL *ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_universal)				(Ipp32u **ppBitStream, Ipp32s *pOffset,Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs, const Ipp32s *pTblCoeffToken, const Ipp32s **ppTblTotalZerosCR,   const Ipp32s **ppTblRunBefore)=NULL;//***

IppStatus (__STDCALL *ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_universal)	(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, Ipp32u    nAlpha,  Ipp32u    nBeta, Ipp8u*    pThresholds, Ipp8u*    pBs)=NULL;
IppStatus (__STDCALL *ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_universal)	(Ipp8u*  pSrcDst, Ipp32s    srcdstStep, Ipp32u    nAlpha,  Ipp32u    nBeta, Ipp8u*    pThresholds, Ipp8u*    pBs)=NULL;

//dummy
IppStatus (__STDCALL *ippiHuffmanRunLevelTableInitAlloc_32s_universal)					(const Ipp32s*    pSrcTable,     IppVCHuffmanSpec_32s** ppDstSpec)=NULL;
IppStatus (__STDCALL *ippiHuffmanTableFree_32s_universal)								(IppVCHuffmanSpec_32s *pDecodeTable)=NULL;
IppStatus (__STDCALL *ippiHuffmanTableInitAlloc_32s_universal)							(const Ipp32s*    pSrcTable,       IppVCHuffmanSpec_32s** ppDstSpec)=NULL;

IppStatus (__STDCALL *ippiInterpolateLumaTop_H264_8u_C1R_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateLumaBottom_H264_8u_C1R_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateChromaTop_H264_8u_C1R_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateChromaBottom_H264_8u_C1R_universal)				(const Ipp8u* pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateBlock_H264_8u_P3P1R_universal)						(Ipp8u *pSrc1,  Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u uWidth,Ipp32u uHeight,Ipp32s iPitchSrc1,Ipp32s iPitchSrc2,Ipp32s iPitchDst)=NULL;
IppStatus (__STDCALL *ippiUniDirWeightBlock_H264_8u_C1R_universal)						(Ipp8u *pSrcDst,Ipp32u pitch, Ipp32u ulog2wd, Ipp32s iWeight,Ipp32s iOffset,IppiSize roi)=NULL;
IppStatus (__STDCALL *ippiBiDirWeightBlock_H264_8u_P2P1R_universal)						(Ipp8u *pSrc1,  Ipp8u *pSrc2, Ipp8u *pDst,     Ipp32u pitch,  Ipp32u dstStep, Ipp32u ulog2wd, Ipp32s iWeight1, Ipp32s iOffset1, Ipp32s iWeight2,Ipp32s iOffset2,   IppiSize roi)=NULL;
IppStatus (__STDCALL *ippiBiDirWeightBlock_H264_8u_P3P1R_universal)						(const Ipp8u *pSrc1,const Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u nSrcPitch1, Ipp32u nSrcPitch2,Ipp32u nDstPitch,Ipp32u ulog2wd,Ipp32s iWeight1,Ipp32s iOffset1,Ipp32s iWeight2,Ipp32s iOffset2,IppiSize roi)=NULL;
IppStatus (__STDCALL *ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_universal)				(const Ipp8u *pSrc1,const Ipp8u *pSrc2,Ipp8u *pDst,Ipp32u nSrcPitch1, Ipp32u nSrcPitch2,  Ipp32u nDstPitch,  Ipp32s iWeight1,Ipp32s iWeight2,  IppiSize roi)=NULL;
IppStatus (__STDCALL *ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_universal)				(Ipp8u *pSrc1,  Ipp8u *pSrc2,   Ipp8u *pDst,    Ipp32u pitch,  Ipp32u dstpitch,   Ipp32s iWeight1,  Ipp32s iWeight2,  IppiSize roi)=NULL;

IppStatus (__STDCALL *ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane,Ipp32u srcdstYStep,const IppIntra16x16PredMode_H264 intra_luma_mode,const Ipp32u cbp4x4, const Ipp32u QP,const Ipp8u edge_type)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaIntraMB_H264_16s8u_P2R_universal)			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, const Ipp32u srcdstUVStep, const IppIntraChromaPredMode_H264 intra_chroma_mode, const Ipp32u cbp4x4, const Ipp32u ChromaQP, const Ipp8u edge_type)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_universal)		(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, Ipp32u srcdstUVStep, IppIntraChromaPredMode_H264 intra_chroma_mode, Ipp32u cbp4x4, Ipp32u ChromaQP, Ipp8u edge_type_top, Ipp8u edge_type_bottom)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntraMB_H264_16s8u_C1R_universal)				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, const IppIntra4x4PredMode_H264 *pMBIntraTypes, const Ipp32u cbp4x4, const Ipp32u QP, const Ipp8u edge_type)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x2, Ipp32u QP, Ipp8u edge_type)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaInterMB_H264_16s8u_C1R_universal)				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp4x4, Ipp32s QP)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaInterMB_H264_16s8u_P2R_universal)			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, const Ipp32u srcdstUVStep, const Ipp32u cbp4x4, const Ipp32u ChromaQP)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal)		(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, const IppIntra16x16PredMode_H264 intra_luma_mode, const Ipp32u cbp4x4, const Ipp32u QP, const Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag /*Resevr ONLY : do not support at present */)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra8x8PredMode_H264 *pMBIntraTypes, Ipp32u cbp8x8, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_universal)		(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra8x8PredMode_H264 *pMBIntraTypes, Ipp32u cbp8x2, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp8x8, Ipp32s QP, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x4, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_universal)		(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x2, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp4x4, Ipp32s QP, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_universal)			(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep, IppIntraChromaPredMode_H264 intra_chroma_mode, Ipp32u cbp4x4, Ipp32u ChromaQPU, Ipp32u ChromaQPV, Ipp8u edge_type, Ipp16s *pQuantTableU, Ipp16s *pQuantTableV, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_universal)	(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep,IppIntraChromaPredMode_H264 intra_chroma_mode,Ipp32u cbp4x4,Ipp32u ChromaQPU,Ipp32u ChromaQPV,Ipp8u edge_type_top,Ipp8u edge_type_bottom,Ipp16s *pQuantTableU,Ipp16s *pQuantTableV,Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_universal)			(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep,Ipp32u cbp4x4,Ipp32u ChromaQPU,Ipp32u ChromaQPV,Ipp16s *pQuantTableU,Ipp16s *pQuantTableV,Ipp8u bypass_flag)=NULL;
#endif

#ifdef KINOMA_AVC_ENC
IppStatus  (__STDCALL *ippiEncodeCoeffsCAVLC_H264_16s_universal)						(Ipp16s* pSrc, Ipp8u   AC, Ipp32s  *pScanMatrix, Ipp8u Count,  Ipp8u   *Trailing_Ones,Ipp8u   *Trailing_One_Signs, Ipp8u   *NumOutCoeffs, Ipp8u   *TotalZeros,  Ipp16s  *Levels, Ipp8u   *Runs )=NULL;
IppStatus  (__STDCALL *ippiEncodeChromaDcCoeffsCAVLC_H264_16s_universal)				(Ipp16s* pSrc,  Ipp8u   *Trailing_Ones, Ipp8u   *Trailing_One_Signs, Ipp8u   *NumOutCoeffs,  Ipp8u   *TotalZeros,  Ipp16s  *Levels,  Ipp8u   *Runs)=NULL;


IppStatus  (__STDCALL *ippiTransformDequantLumaDC_H264_16s_C1I_universal)  			(Ipp16s* pSrcDst, Ipp32s  QP)=NULL;
IppStatus  (__STDCALL *ippiTransformDequantChromaDC_H264_16s_C1I_universal) 			(Ipp16s* pSrcDst, Ipp32s  QP)=NULL;
IppStatus  (__STDCALL *ippiDequantTransformResidualAndAdd_H264_16s_C1I_universal)	(const Ipp8u*  pPred,Ipp16s* pSrcDst,Ipp16s* pDC,Ipp8u*  pDst,Ipp32s  PredStep,Ipp32s  DstStep, Ipp32s  QP,Ipp32s  AC)=NULL;
IppStatus  (__STDCALL *ippiTransformQuantChromaDC_H264_16s_C1I_universal)			(Ipp16s* pSrcDst,Ipp16s* pTBlock, Ipp32s  QPCroma, Ipp8s*  NumLevels, Ipp8u   Intra, Ipp8u   NeedTransform)=NULL;
IppStatus  (__STDCALL *ippiTransformQuantResidual_H264_16s_C1I_universal) 			(Ipp16s* pSrcDst,Ipp32s  QP,Ipp8s*  NumLevels,Ipp8u   Intra,  Ipp16s* pScanMatrix, Ipp8u*  LastCoeff)=NULL;
IppStatus  (__STDCALL *ippiTransformQuantLumaDC_H264_16s_C1I_universal) 				(Ipp16s* pSrcDst,Ipp16s* pTBlock, Ipp32s  QP,   Ipp8s*  NumLevels,  Ipp8u   NeedTransform,   Ipp16s* pScanMatrix,  Ipp8u*  LastCoeff)=NULL;


/// COMM :The following functions are some commam function in IPP:we need deleted it carefully
IppStatus  (__STDCALL *ippiResize_8u_C1R_universal)									(const Ipp8u* pSrc, IppiSize srcSize, int srcStep, IppiRect srcRoi,Ipp8u* pDst, int dstStep, IppiSize dstRoiSize, double xFactor, double yFactor, int interpolation)=NULL;
IppStatus  (__STDCALL *ippiSAD16x16_8u32s_universal)									(const Ipp8u*  pSrc, Ipp32s srcStep, const Ipp8u*  pRef, Ipp32s  refStep, Ipp32s* pSAD,  Ipp32s  mcType)=NULL;
IppStatus  (__STDCALL *ippiSAD8x8_8u32s_C1R_universal)								(const Ipp8u*  pSrcCur, int     srcCurStep,const Ipp8u*  pSrcRef, int srcRefStep, Ipp32s* pDst, Ipp32s  mcType)=NULL;
IppStatus  (__STDCALL *ippiSAD4x4_8u32s_universal)									(const Ipp8u*  pSrc,Ipp32s srcStep,  const Ipp8u*  pRef, Ipp32s  refStep, Ipp32s* pSAD,Ipp32s  mcType )=NULL;
IppStatus  (__STDCALL *ippiSAD16x16Blocks8x8_8u16u_universal)						(const   Ipp8u*  pSrc,Ipp32s  srcStep, const   Ipp8u*  pRef, Ipp32s  refStep, Ipp16u*  pDstSAD, Ipp32s   mcType )=NULL;
IppStatus  (__STDCALL *ippiSAD16x16Blocks4x4_8u16u_universal) 						(const   Ipp8u*  pSrc,Ipp32s  srcStep, const   Ipp8u*  pRef, Ipp32s  refStep,Ipp16u*  pDstSAD, Ipp32s   mcType)=NULL;
IppStatus  (__STDCALL *ippiSumsDiff16x16Blocks4x4_8u16s_C1_universal)				( Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* pPred, Ipp32s predStep, Ipp16s* pSums, Ipp16s* pDiff)=NULL;
IppStatus  (__STDCALL *ippiSumsDiff8x8Blocks4x4_8u16s_C1_universal)					( Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* pPred, Ipp32s predStep, Ipp16s* pSums,Ipp16s* pDiff)=NULL;
IppStatus  (__STDCALL *ippiGetDiff4x4_8u16s_C1_universal)							( const Ipp8u*  pSrcCur, Ipp32s  srcCurStep, const Ipp8u*  pSrcRef, Ipp32s  srcRefStep, Ipp16s* pDstDiff, Ipp32s  dstDiffStep, Ipp16s* pDstPredictor, Ipp32s  dstPredictorStep, Ipp32s  mcType, Ipp32s  roundControl)=NULL;
IppStatus  (__STDCALL *ippiEdgesDetect16x16_8u_C1R_universal)						(const Ipp8u *pSrc,Ipp32u srcStep,Ipp8u EdgePelDifference,Ipp8u EdgePelCount, Ipp8u   *pRes)=NULL;

#endif

#ifdef _WIN32_WCE

#include "windef.h"
#include "winioctl.h"
#include "winbase.h"

#ifdef __cplusplus 
	extern "C" {
#endif
	BOOL KernelIoControl(
		DWORD dwIoControlCode, 
		LPVOID lpInBuf, DWORD nInBufSize, 
		LPVOID lpOutBuf, 
		DWORD nOutBufSize, 
		LPDWORD lpBytesReturned);
#ifdef __cplusplus 
}
#endif

#define IOCTL_PROCESSOR_INFORMATION CTL_CODE(FILE_DEVICE_HAL, 25, METHOD_BUFFERED, FILE_ANY_ACCESS)
// Used by IOCTL_PROCESSOR_INFORMATION
typedef struct _PROCESSOR_INFO 
{
	WORD 	wVersion;
	WCHAR	szProcessCore[40];
	WORD	wCoreRevision;
	WCHAR	szProcessorName[40];
	WORD	wProcessorRevision;
	WCHAR	szCatalogNumber[100];
	WCHAR	szVendor[100];
	DWORD	dwInstructionSet;
	DWORD	dwClockSpeed;
} PROCESSOR_INFO;

typedef PROCESSOR_INFO *PPROCESSOR_INFO;


static int check_wince_cup_type()
{
	PROCESSOR_INFO pi;
	DWORD dwBytesReturned;
	DWORD dwSize;
	BOOL bResult;
	SYSTEM_INFO si;

	memset(&pi, 0, sizeof(PROCESSOR_INFO));
	dwSize = sizeof(PROCESSOR_INFO);

	bResult = KernelIoControl(
								IOCTL_PROCESSOR_INFORMATION, 
								NULL, 
								0, 
								&pi, 
								sizeof(PROCESSOR_INFO), 
								&dwBytesReturned
								);


	memset(&si,0,sizeof(SYSTEM_INFO));
	GetSystemInfo(&si);

	BOOL arm_v4_present = IsProcessorFeaturePresent( PF_ARM_V4 );
	BOOL arm_v5_present = IsProcessorFeaturePresent( PF_ARM_V5 );
//	BOOL arm_v6_present = IsProcessorFeaturePresent( PF_ARM_V6 );
//	BOOL arm_v7_present = IsProcessorFeaturePresent( PF_ARM_V7 );
//	BOOL arm_thumb_present = IsProcessorFeaturePresent( PF_ARM_THUMB );
//	BOOL arm_jazelle_present = IsProcessorFeaturePresent( PF_ARM_JAZELLE );
//	BOOL arm_dsp_present = IsProcessorFeaturePresent( PF_ARM_DSP );

	//QueryInstructionSet( PROCESSOR_ARM_V4_INSTRUCTION
	{
		//wchar_t pxa27x[100]={'P','X','A','2','7','x',0 };
	
		if( 0 == wcscmp(pi.szProcessorName, L"PXA27x") )
			return FSK_ARCH_XSCALE;
	}

	if (arm_v5_present)
		return FSK_ARCH_ARM_V5;		// must check after XScale, since XScale is also ARM5

	return FSK_ARCH_C;
}
#endif

int kinoma_ipp_lib_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = check_wince_cup_type();
#else	
		implementation = FSK_ARCH_XSCALE;
#endif

	//implementation = FSK_ARCH_C;

	switch(implementation)
	{
#ifdef __INTEL_IPP__
		case FSK_ARCH_XSCALE:

			//common
			ippsMalloc_8u_universal =						ippsMalloc_8u;			
			ippsFree_universal =							ippsFree;				
			ippsZero_8u_universal =							ippsZero_8u;			
			ippsZero_16s_universal =						ippsZero_16s;		
			ippsZero_32s_universal =						ippsZero_32s;		
			ippsZero_32sc_universal =						ippsZero_32sc;		
			ippsCopy_8u_universal =							ippsCopy_8u;			
			ippsSet_8u_universal =							ippsSet_8u;				
			ippiCopy_8u_C1R_universal =  					ippiCopy_8u_C1R;  				

#if defined(KINOMA_MP3) || defined(KINOMA_AAC) || defined(KINOMA_MP4V_ENC)
			ippsSortAscend_32s_I_universal =					ippsSortAscend_32s_I;
			ippsMalloc_32s_universal =							ippsMalloc_32s;		
#endif


#if defined(KINOMA_MP3)|| defined(KINOMA_AAC) || defined(KINOMA_AAC_ENC)
			ippsCopy_32s_universal =						ippsCopy_32s;		
			ippsAdd_32s_Sfs_universal =						ippsAdd_32s_Sfs;		
			ippsAdd_32s_ISfs_universal =					ippsAdd_32s_ISfs;	
			ippsMul_32s_ISfs_universal =					ippsMul_32s_ISfs;	
			ippsMul_32s_Sfs_universal =						ippsMul_32s_Sfs;		
			ippsSub_32s_Sfs_universal =						ippsSub_32s_Sfs;		
			ippsMinMax_32s_universal =						ippsMinMax_32s;		
			ippsMax_32s_universal =							ippsMax_32s;			
			ippsMax_16s_universal =							ippsMax_16s;			
			ippsMin_32s_universal =							ippsMin_32s;			
			ippsMaxAbs_32s_universal =						ippsMaxAbs_32s;		
			ippsMove_32s_universal =						ippsMove_32s;		
			ippsDiv_32s_ISfs_universal =					ippsDiv_32s_ISfs;	
			ippsSqrt_64s_ISfs_universal =					ippsSqrt_64s_ISfs;	
			ippsConvert_64s32s_Sfs_universal =				ippsConvert_64s32s_Sfs;	
			ippsConvert_32s16s_Sfs_universal =				ippsConvert_32s16s_Sfs;	
			ippsLShiftC_32s_I_universal =					ippsLShiftC_32s_I;		
			ippsRShiftC_32s_I_universal =					ippsRShiftC_32s_I;		

			/* following functions only used in mp3*/			
			ippsAddC_32s_ISfs_universal =					ippsAddC_32s_ISfs;		
			ippsMulC_32s_ISfs_universal =					ippsMulC_32s_ISfs;
#endif

#if defined(KINOMA_AAC) ||  defined(KINOMA_AAC_ENC)
			ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s;

			/*following functions are for 5.1  aac*/			
			ippsMul_32sc_Sfs_universal =					ippsMul_32sc_Sfs;		
			ippsMul_32s32sc_Sfs_universal =					ippsMul_32s32sc_Sfs;
			ippsMulC_32s_Sfs_universal =					ippsMulC_32s_Sfs;		

			/* the following 4 functions used in MDCT*/			
			ippsFFTInitAlloc_C_32sc_universal =				ippsFFTInitAlloc_C_32sc;
			ippsFFTGetBufSize_C_32sc_universal =			ippsFFTGetBufSize_C_32sc;
			ippsFFTFree_C_32sc_universal =					ippsFFTFree_C_32sc;		
			ippsFFTFwd_CToC_32sc_Sfs_universal =			ippsFFTFwd_CToC_32sc_Sfs;
#endif

#if defined(KINOMA_MP3)|| defined(KINOMA_AAC) 
			ippsVLCDecodeBlock_1u16s_universal			= ippsVLCDecodeBlock_1u16s;
			ippsVLCDecodeFree_32s_universal				= ippsVLCDecodeFree_32s;
			ippsVLCDecodeInitAlloc_32s_universal		= ippsVLCDecodeInitAlloc_32s;
			ippsVLCDecodeOne_1u16s_universal			= ippsVLCDecodeOne_1u16s;
#endif

#if defined(KINOMA_MP3)
			
  			ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s;
			ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s;
#endif

#if defined(KINOMA_AAC)
			ippsFFTInv_CToC_32s_Sfs_universal			= ippsFFTInv_CToC_32s_Sfs;
			ippsFFTInit_C_32s_universal					= ippsFFTInit_C_32s;		

			/* the following 6 functions exist in sbr code */  
			ippsFFTGetSize_C_32sc_universal =				ippsFFTGetSize_C_32sc;	
			ippsFFTInit_C_32sc_universal =					ippsFFTInit_C_32sc;		
			ippsFFTInv_CToC_32sc_Sfs_universal =			ippsFFTInv_CToC_32sc_Sfs;
			ippsFFTGetSize_C_32s_universal =				ippsFFTGetSize_C_32s;	
#endif

#if defined(KINOMA_AAC_ENC)
			ippsCopy_16s_universal = 						ippsCopy_16s; 			
			ippsSet_16s_universal = 						ippsSet_16s; 			
			ippsAbs_16s_I_universal = 					ippsAbs_16s_I; 		
			ippsAbs_16s_universal = 						ippsAbs_16s; 			
			ippsMinEvery_32s_I_universal = 				ippsMinEvery_32s_I; 	
			ippsMaxEvery_32s_I_universal = 				ippsMaxEvery_32s_I; 	
			ippsMinMax_16s_universal = 					ippsMinMax_16s; 		
			ippsSum_16s32s_Sfs_universal = 				ippsSum_16s32s_Sfs; 	
			ippsSum_32s_Sfs_universal = 					ippsSum_32s_Sfs; 		
			ippsAdd_16s_Sfs_universal = 					ippsAdd_16s_Sfs; 		
			
			ippsAdd_16s_ISfs_universal = 					ippsAdd_16s_ISfs; 		
			ippsAddC_16s_Sfs_universal = 					ippsAddC_16s_Sfs; 		
			ippsSub_16s_universal = 						ippsSub_16s; 			
			ippsSub_16s_ISfs_universal = 					ippsSub_16s_ISfs; 		
			ippsSub_16s_Sfs_universal = 					ippsSub_16s_Sfs; 		
																		
			ippsMul_16s_Sfs_universal = 					ippsMul_16s_Sfs; 		
			ippsMul_16s_ISfs_universal = 					ippsMul_16s_ISfs; 		
			ippsMul_16s32s_Sfs_universal = 				ippsMul_16s32s_Sfs; 	
			ippsMulC_16s_Sfs_universal = 					ippsMulC_16s_Sfs; 		
			ippsMul_16s_universal = 						ippsMul_16s; 			

			ippsDiv_16s_Sfs_universal = 					ippsDiv_16s_Sfs; 		
			ippsDiv_16s_ISfs_universal = 					ippsDiv_16s_ISfs; 		
			ippsDivC_16s_ISfs_universal = 				ippsDivC_16s_ISfs; 	
			ippsSpread_16s_Sfs_universal = 			    ippsSpread_16s_Sfs; 			
			ippsPow34_16s_Sfs_universal = 			    ippsPow34_16s_Sfs; 			
			ippsMagnitude_16sc_Sfs_universal = 		    ippsMagnitude_16sc_Sfs; 		
			ippsMagnitude_16s_Sfs_universal = 		    ippsMagnitude_16s_Sfs; 		
			ippsRShiftC_32s_universal = 				    ippsRShiftC_32s; 				
			ippsRShiftC_16s_universal = 				    ippsRShiftC_16s; 				
			ippsLShiftC_16s_I_universal = 			    ippsLShiftC_16s_I; 			

			ippsDotProd_16s32s32s_Sfs_universal = 	    ippsDotProd_16s32s32s_Sfs; 	
			ippsDotProd_16s32s_Sfs_universal = 		    ippsDotProd_16s32s_Sfs; 		
			ippsLn_32s16s_Sfs_universal = 			    ippsLn_32s16s_Sfs; 			
			ippsDeinterleave_16s_universal = 			    ippsDeinterleave_16s; 			

			ippsVLCEncodeBlock_16s1u_universal = 			ippsVLCEncodeBlock_16s1u; 			
			ippsVLCEncodeEscBlock_AAC_16s1u_universal = 	ippsVLCEncodeEscBlock_AAC_16s1u; 
			ippsVLCEncodeInitAlloc_32s_universal = 		ippsVLCEncodeInitAlloc_32s;

			ippsVLCEncodeFree_32s_universal = 			ippsVLCEncodeFree_32s; 			
			ippsVLCCountBits_16s32s_universal = 			ippsVLCCountBits_16s32s; 			
			ippsVLCCountEscBits_AAC_16s32s_universal = 	ippsVLCCountEscBits_AAC_16s32s; 	

			ippsThreshold_LT_16s_I_universal = 			ippsThreshold_LT_16s_I; 			
			ippsConjPack_16sc_universal = 				ippsConjPack_16sc; 				
			ippsConjCcs_16sc_universal = 					ippsConjCcs_16sc; 					

			ippsFFTInitAlloc_R_32s_universal = 			ippsFFTInitAlloc_R_32s; 			
			ippsFFTGetBufSize_R_32s_universal =  			ippsFFTGetBufSize_R_32s;  			
			ippsFFTGetBufSize_R_16s_universal =  			ippsFFTGetBufSize_R_16s;  			
			ippsFFTGetBufSize_C_16sc_universal =  		ippsFFTGetBufSize_C_16sc;  		
			ippsMDCTFwdGetBufSize_16s_universal = 		ippsMDCTFwdGetBufSize_16s; 		
			ippsMDCTFwdFree_16s_universal = 				ippsMDCTFwdFree_16s; 				
			ippsMDCTFwdInitAlloc_16s_universal = 			ippsMDCTFwdInitAlloc_16s; 			
			ippsMDCTFwd_16s_Sfs_universal = 				ippsMDCTFwd_16s_Sfs; 				
			ippsFFTInitAlloc_R_16s_universal = 			ippsFFTInitAlloc_R_16s; 			
			ippsFFTInitAlloc_C_16s_universal = 			ippsFFTInitAlloc_C_16s; 			
			ippsFFTInitAlloc_C_16sc_universal = 			ippsFFTInitAlloc_C_16sc; 			
			ippsFFTFree_R_16s_universal =  				ippsFFTFree_R_16s;  				
			ippsFFTFree_C_16s_universal = 				ippsFFTFree_C_16s; 				
			ippsFFTFree_C_16sc_universal = 				ippsFFTFree_C_16sc; 				
			//ippsFFTFwd_CToC_16sc_Sfs_universal = 			ippsFFTFwd_CToC_16sc_Sfs; 			
			ippsFFTFwd_RToCCS_16s_Sfs_universal = 		ippsFFTFwd_RToCCS_16s_Sfs; 		
			ippsFFTFwd_RToPack_32s_Sfs_universal = 		ippsFFTFwd_RToPack_32s_Sfs; 		
			ippsFFTInv_PackToR_32s_Sfs_universal = 		ippsFFTInv_PackToR_32s_Sfs; 		
#endif


#if defined(KINOMA_MP4V) || defined(KINOMA_MP4V_ENC) 
			//ippi
			//MPEG4 Video
			ippiDCT8x8Inv_16s_C1I_universal						= ippiDCT8x8Inv_16s_C1I;						
			ippiDCT8x8Inv_16s8u_C1R_universal					= ippiDCT8x8Inv_16s8u_C1R;					

			ippiWarpGetSize_MPEG4_universal						= ippiWarpGetSize_MPEG4;						
			ippiQuantInvInterInit_MPEG4_universal				= ippiQuantInvInterInit_MPEG4;				
			ippiQuantInvInterGetSize_MPEG4_universal			= ippiQuantInvInterGetSize_MPEG4;			
			ippiQuantInvIntraInit_MPEG4_universal				= ippiQuantInvIntraInit_MPEG4;				
			ippiQuantInvIntraGetSize_MPEG4_universal			= ippiQuantInvIntraGetSize_MPEG4;			

			ippiQuantInvIntra_MPEG4_16s_C1I_universal 			= ippiQuantInvIntra_MPEG4_16s_C1I; 			
			ippiAdd8x8_16s8u_C1IRS_universal 					= ippiAdd8x8_16s8u_C1IRS;					
			ippiOBMC8x8HP_MPEG4_8u_C1R_universal				= ippiOBMC8x8HP_MPEG4_8u_C1R;				
			ippiCopy8x8QP_MPEG4_8u_C1R_universal 				= ippiCopy8x8QP_MPEG4_8u_C1R; 				

			ippiOBMC8x8QP_MPEG4_8u_C1R_universal				= ippiOBMC8x8QP_MPEG4_8u_C1R;				
			ippiAverage8x8_8u_C1IR_universal					= ippiAverage8x8_8u_C1IR;					
			ippiAverage16x16_8u_C1IR_universal					= ippiAverage16x16_8u_C1IR;					

			ippiCalcGlobalMV_MPEG4_universal					= ippiCalcGlobalMV_MPEG4;					
			ippiWarpChroma_MPEG4_8u_P2R_universal				= ippiWarpChroma_MPEG4_8u_P2R;				        
			ippiWarpLuma_MPEG4_8u_C1R_universal					= ippiWarpLuma_MPEG4_8u_C1R;					
			ippiWarpInit_MPEG4_universal						= ippiWarpInit_MPEG4;						

			ippiCopy8x8_8u_C1R_universal						= ippiCopy8x8_8u_C1R;						
			ippiCopy16x16_8u_C1R_universal						= ippiCopy16x16_8u_C1R;						
			ippiCopy8x8HP_8u_C1R_universal						= ippiCopy8x8HP_8u_C1R;						
			ippiCopy8x4HP_8u_C1R_universal						= ippiCopy8x4HP_8u_C1R;						
			ippiCopy16x8HP_8u_C1R_universal						= ippiCopy16x8HP_8u_C1R;						
			ippiCopy16x16HP_8u_C1R_universal					= ippiCopy16x16HP_8u_C1R;					
			ippiCopy16x8QP_MPEG4_8u_C1R_universal				= ippiCopy16x8QP_MPEG4_8u_C1R;				
			ippiCopy16x16QP_MPEG4_8u_C1R_universal				= ippiCopy16x16QP_MPEG4_8u_C1R;				
#endif

#if defined(KINOMA_MP4V)
			ippiDCT8x8Inv_4x4_16s_C1I_universal					= ippiDCT8x8Inv_4x4_16s_C1I;					
			ippiChangeSpriteBrightness_MPEG4_8u_C1IR_universal	= ippiChangeSpriteBrightness_MPEG4_8u_C1IR;			
			ippiAdd8x8HP_16s8u_C1RS_universal					= ippiAdd8x8HP_16s8u_C1RS;					
			ippiReconstructCoeffsInter_MPEG4_1u16s_universal	= ippiReconstructCoeffsInter_MPEG4_1u16s;	
			ippiDecodeDCIntra_MPEG4_1u16s_universal				= ippiDecodeDCIntra_MPEG4_1u16s;				
			ippiDecodeCoeffsIntra_MPEG4_1u16s_universal 		= ippiDecodeCoeffsIntra_MPEG4_1u16s; 		
			//ippiReconstructCoeffsIntra_H263_1u16s_universal 	= ippiReconstructCoeffsIntra_H263_1u16s; 	
			//ippiReconstructCoeffsInter_H263_1u16s_universal 	= ippiReconstructCoeffsInter_H263_1u16s; 	
			ippiDCT8x8Inv_2x2_16s_C1I_universal					= ippiDCT8x8Inv_2x2_16s_C1I;					
#endif

#if defined(KINOMA_MP4V_ENC) 
			ippiSAD16x16_8u32s_universal = 					ippiSAD16x16_8u32s; 				
			ippiSAD16x8_8u32s_C1R_universal =  				ippiSAD16x8_8u32s_C1R;  			
			ippiSAD8x8_8u32s_C1R_universal =  				ippiSAD8x8_8u32s_C1R;  			
			ippiMeanAbsDev16x16_8u32s_C1R_universal = 		ippiMeanAbsDev16x16_8u32s_C1R; 	
			ippiQuantIntraInit_MPEG4_universal = 			ippiQuantIntraInit_MPEG4; 		
			ippiQuantInterInit_MPEG4_universal =  			ippiQuantInterInit_MPEG4;  		
			ippsMalloc_32u_universal = 			 			ippsMalloc_32u; 			 		
			ippiQuantInterGetSize_MPEG4_universal =  		ippiQuantInterGetSize_MPEG4;  	
			ippiQuantIntraGetSize_MPEG4_universal =  		ippiQuantIntraGetSize_MPEG4;  	
			ippiQuantIntra_H263_16s_C1I_universal =  		ippiQuantIntra_H263_16s_C1I;  	
			ippiQuantIntra_MPEG4_16s_C1I_universal =  		ippiQuantIntra_MPEG4_16s_C1I;  	
			ippiDCT8x8Fwd_8u16s_C1R_universal =  			ippiDCT8x8Fwd_8u16s_C1R;  		
			ippiQuantInter_H263_16s_C1I_universal =  		ippiQuantInter_H263_16s_C1I;  	
			ippiDCT8x8Fwd_16s_C1I_universal =  				ippiDCT8x8Fwd_16s_C1I;  			
			ippiSubSAD8x8_8u16s_C1R_universal =  			ippiSubSAD8x8_8u16s_C1R;  		
			ippiFrameFieldSAD16x16_8u32s_C1R_universal =  	ippiFrameFieldSAD16x16_8u32s_C1R; 
			ippiSub8x8_8u16s_C1R_universal =  				ippiSub8x8_8u16s_C1R;  			
			ippiQuantInter_MPEG4_16s_C1I_universal =  		ippiQuantInter_MPEG4_16s_C1I;  	
			ippiDCT8x8Fwd_16s_C1R_universal =  				ippiDCT8x8Fwd_16s_C1R;  			
			ippsCopy_1u_universal =  						ippsCopy_1u;  					
			ippiCopy_8u_C1R_universal =  					ippiCopy_8u_C1R;  				
			ippiFrameFieldSAD16x16_16s32s_C1R_universal =  	ippiFrameFieldSAD16x16_16s32s_C1R;
			ippiSub16x16_8u16s_C1R_universal =  			ippiSub16x16_8u16s_C1R;  			
			ippiQuantInvIntra_H263_16s_C1I_universal =  	ippiQuantInvIntra_H263_16s_C1I;  	
			ippiSqrDiff16x16_8u32s_universal =  			ippiSqrDiff16x16_8u32s;  			
			ippiSSD8x8_8u32s_C1R_universal =  				ippiSSD8x8_8u32s_C1R;  			
			ippiQuantInvInter_H263_16s_C1I_universal =  	ippiQuantInvInter_H263_16s_C1I;  	
			ippiQuantInvInter_MPEG4_16s_C1I_universal =  	ippiQuantInvInter_MPEG4_16s_C1I;  
			ippiEncodeCoeffsIntra_H263_16s1u_universal =  	ippiEncodeCoeffsIntra_H263_16s1u; 
			ippiEncodeDCIntra_H263_16s1u_universal =  		ippiEncodeDCIntra_H263_16s1u;  	
			ippiCountZeros8x8_16s_C1_universal =  			ippiCountZeros8x8_16s_C1;  		
			ippiEncodeCoeffsIntra_MPEG4_16s1u_universal =  	ippiEncodeCoeffsIntra_MPEG4_16s1u;
			ippiEncodeDCIntra_MPEG4_16s1u_universal =  		ippiEncodeDCIntra_MPEG4_16s1u;  	
			ippiEncodeCoeffsInter_H263_16s1u_universal =  	ippiEncodeCoeffsInter_H263_16s1u; 
			ippiEncodeCoeffsInter_MPEG4_16s1u_universal =  	ippiEncodeCoeffsInter_MPEG4_16s1u;

#endif

#if defined(KINOMA_AVC) || defined(KINOMA_AVC_ENC)
			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR;
			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR;
			ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR;
			ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR;

			ippiInterpolateBlock_H264_8u_P2P1R_universal = 						ippiInterpolateBlock_H264_8u_P2P1R;
			ippiInterpolateLuma_H264_8u_C1R_universal = 						ippiInterpolateLuma_H264_8u_C1R;
			ippiInterpolateChroma_H264_8u_C1R_universal = 						ippiInterpolateChroma_H264_8u_C1R;
#endif

#if defined(KINOMA_AVC)
			ippiSet_8u_C1R_universal = 											ippiSet_8u_C1R;

			//ippiDecodeExpGolombOne_H264_1u16s_universal = 						ippiDecodeExpGolombOne_H264_1u16s;
			ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal = 				ippiDecodeExpGolombOne_H264_1u16s_unsigned_ipp;
			ippiDecodeExpGolombOne_H264_1u16s_signed_universal = 				ippiDecodeExpGolombOne_H264_1u16s_signed_ipp;
			ippiDecodeCAVLCCoeffs_H264_1u16s_universal = 						ippiDecodeCAVLCCoeffs_H264_1u16s;
			ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_universal = 				ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s;
			ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_universal =     ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR;
			ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_universal =   ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR;


			ippiHuffmanRunLevelTableInitAlloc_32s_universal = 					ippiHuffmanRunLevelTableInitAlloc_32s;
			ippiHuffmanTableFree_32s_universal = 								ippiHuffmanTableFree_32s;
			ippiHuffmanTableInitAlloc_32s_universal = 							ippiHuffmanTableInitAlloc_32s;

			ippiInterpolateLumaTop_H264_8u_C1R_universal = 						ippiInterpolateLumaTop_H264_8u_C1R;
			ippiInterpolateLumaBottom_H264_8u_C1R_universal = 					ippiInterpolateLumaBottom_H264_8u_C1R;
			ippiInterpolateChromaTop_H264_8u_C1R_universal = 					ippiInterpolateChromaTop_H264_8u_C1R;							
			ippiInterpolateChromaBottom_H264_8u_C1R_universal = 				ippiInterpolateChromaBottom_H264_8u_C1R;
			ippiInterpolateBlock_H264_8u_P3P1R_universal = 						ippiInterpolateBlock_H264_8u_P3P1R;
			ippiUniDirWeightBlock_H264_8u_C1R_universal = 						ippiUniDirWeightBlock_H264_8u_C1R;
			ippiBiDirWeightBlock_H264_8u_P2P1R_universal = 						ippiBiDirWeightBlock_H264_8u_P2P1R;
			ippiBiDirWeightBlock_H264_8u_P3P1R_universal = 						ippiBiDirWeightBlock_H264_8u_P3P1R;
			ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_universal = 				ippiBiDirWeightBlockImplicit_H264_8u_P3P1R;
			ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_universal = 				ippiBiDirWeightBlockImplicit_H264_8u_P2P1R;

			ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R;
			ippiReconstructChromaIntraMB_H264_16s8u_P2R_universal = 			ippiReconstructChromaIntraMB_H264_16s8u_P2R;
			ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_universal = 		ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R;
			ippiReconstructLumaIntraMB_H264_16s8u_C1R_universal = 				ippiReconstructLumaIntraMB_H264_16s8u_C1R;
			ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R;
			ippiReconstructLumaInterMB_H264_16s8u_C1R_universal = 				ippiReconstructLumaInterMB_H264_16s8u_C1R;
			ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R;
			ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R;
			ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R;
			ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R;
			ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaInter8x8MB_H264_16s8u_C1R;
			ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R;
			ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R;
			ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaInter4x4MB_H264_16s8u_C1R;
			ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_universal =     ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R;
			ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_universal = 			ippiReconstructChromaInter4x4MB_H264_16s8u_P2R;
			ippiReconstructChromaInterMB_H264_16s8u_P2R_universal = 			ippiReconstructChromaInterMB_H264_16s8u_P2R;
			ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_universal = 			ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R;

			UMC::IppLumaDeblocking[0]	= ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal;
			UMC::IppLumaDeblocking[1]	= ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal;
			UMC::IppChromaDeblocking[0]	= ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal;
			UMC::IppChromaDeblocking[1]	= ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal;

#endif

#ifdef KINOMA_AVC_ENC
			ippiEncodeCoeffsCAVLC_H264_16s_universal = 						ippiEncodeCoeffsCAVLC_H264_16s;					
			ippiEncodeChromaDcCoeffsCAVLC_H264_16s_universal = 				ippiEncodeChromaDcCoeffsCAVLC_H264_16s; 			


			ippiTransformDequantLumaDC_H264_16s_C1I_universal =   		    ippiTransformDequantLumaDC_H264_16s_C1I;   		
			ippiTransformDequantChromaDC_H264_16s_C1I_universal =  			ippiTransformDequantChromaDC_H264_16s_C1I;  		
			ippiDequantTransformResidualAndAdd_H264_16s_C1I_universal = 	ippiDequantTransformResidualAndAdd_H264_16s_C1I; 	
			ippiTransformQuantChromaDC_H264_16s_C1I_universal = 			ippiTransformQuantChromaDC_H264_16s_C1I; 			
			ippiTransformQuantResidual_H264_16s_C1I_universal =  			ippiTransformQuantResidual_H264_16s_C1I;  			
			ippiTransformQuantLumaDC_H264_16s_C1I_universal =  				ippiTransformQuantLumaDC_H264_16s_C1I;  			


			ippiResize_8u_C1R_universal = 								    ippiResize_8u_C1R; 								
			ippiSAD16x16_8u32s_universal = 									ippiSAD16x16_8u32s; 								
			ippiSAD8x8_8u32s_C1R_universal = 								ippiSAD8x8_8u32s_C1R; 								
			ippiSAD4x4_8u32s_universal = 									ippiSAD4x4_8u32s; 									
			ippiSAD16x16Blocks8x8_8u16u_universal = 						ippiSAD16x16Blocks8x8_8u16u; 						
			ippiSAD16x16Blocks4x4_8u16u_universal =  						ippiSAD16x16Blocks4x4_8u16u;  						
			ippiSumsDiff16x16Blocks4x4_8u16s_C1_universal = 				ippiSumsDiff16x16Blocks4x4_8u16s_C1; 				
			ippiSumsDiff8x8Blocks4x4_8u16s_C1_universal = 				    ippiSumsDiff8x8Blocks4x4_8u16s_C1; 				
			ippiGetDiff4x4_8u16s_C1_universal = 							ippiGetDiff4x4_8u16s_C1; 							
			ippiEdgesDetect16x16_8u_C1R_universal = 						ippiEdgesDetect16x16_8u_C1R; 						
#endif

			result_implementation = FSK_ARCH_XSCALE;
			break;
#endif


#ifdef __KINOMA_IPP__
		case FSK_ARCH_C:
		default: 
			//common
			ippsMalloc_8u_universal =						ippsMalloc_8u_c;			
			ippsFree_universal =							ippsFree_c;				
			ippsZero_8u_universal =							ippsZero_8u_c;			
			ippsZero_16s_universal =						ippsZero_16s_c;		
			ippsZero_32s_universal =						ippsZero_32s_c;		
			ippsZero_32sc_universal =						ippsZero_32sc_c;		
			ippsCopy_8u_universal =							ippsCopy_8u_c;			
			ippsSet_8u_universal =							ippsSet_8u_c;				
			ippiCopy_8u_C1R_universal =  					ippiCopy_8u_C1R_c;  				

#if defined(KINOMA_MP3) || defined(KINOMA_AAC) || defined(KINOMA_MP4V_ENC)
			ippsSortAscend_32s_I_universal =				ippsSortAscend_32s_I_c;
			ippsMalloc_32s_universal =						ippsMalloc_32s_c;		
#endif


#if defined(KINOMA_MP3)|| defined(KINOMA_AAC) || defined(KINOMA_AAC_ENC)
			ippsCopy_32s_universal =						ippsCopy_32s_c;		
			ippsAdd_32s_Sfs_universal =						ippsAdd_32s_Sfs_c;		
			ippsAdd_32s_ISfs_universal =					ippsAdd_32s_ISfs_c;	
			ippsMul_32s_ISfs_universal =					ippsMul_32s_ISfs_c;	
			ippsMul_32s_Sfs_universal =						ippsMul_32s_Sfs_c;		
			ippsSub_32s_Sfs_universal =						ippsSub_32s_Sfs_c;		
			ippsMinMax_32s_universal =						ippsMinMax_32s_c;		
			ippsMax_32s_universal =							ippsMax_32s_c;			
			ippsMax_16s_universal =							ippsMax_16s_c;			
			ippsMin_32s_universal =							ippsMin_32s_c;			
			ippsMaxAbs_32s_universal =						ippsMaxAbs_32s_c;		
			ippsMove_32s_universal =						ippsMove_32s_c;		
			ippsDiv_32s_ISfs_universal =					ippsDiv_32s_ISfs_c;	
			ippsSqrt_64s_ISfs_universal =					ippsSqrt_64s_ISfs_c;	
			ippsConvert_64s32s_Sfs_universal =				ippsConvert_64s32s_Sfs_c;	
			ippsConvert_32s16s_Sfs_universal =				ippsConvert_32s16s_Sfs_c;	
			ippsLShiftC_32s_I_universal =					ippsLShiftC_32s_I_c;		
			ippsRShiftC_32s_I_universal =					ippsRShiftC_32s_I_c;		

			/* following functions only used in mp3*/			
			ippsAddC_32s_ISfs_universal =					ippsAddC_32s_ISfs_c;		
			ippsMulC_32s_ISfs_universal =					ippsMulC_32s_ISfs_c;
#endif

#if defined(KINOMA_MP3) || defined(KINOMA_AAC)  // Add by WWD in 20070714
			
			ippsVLCDecodeBlock_1u16s_universal			= ippsVLCDecodeBlock_1u16s_c;
			ippsVLCDecodeFree_32s_universal				= ippsVLCDecodeFree_32s_c;
			ippsVLCDecodeInitAlloc_32s_universal		= ippsVLCDecodeInitAlloc_32s_c;
			ippsVLCDecodeOne_1u16s_universal			= ippsVLCDecodeOne_1u16s_c;
#endif
#if defined(KINOMA_MP3)
  			ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s_c;
			ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s_c;
#endif

#if defined(KINOMA_AAC) ||  defined(KINOMA_AAC_ENC)
			ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s_c;

			/*following functions are for 5.1  aac*/			
			ippsMul_32sc_Sfs_universal =					ippsMul_32sc_Sfs_c;		
			ippsMul_32s32sc_Sfs_universal =					ippsMul_32s32sc_Sfs_c;
			ippsMulC_32s_Sfs_universal =					ippsMulC_32s_Sfs_c;		

			/* the following 4 functions used in MDCT*/			
			ippsFFTInitAlloc_C_32sc_universal =				ippsFFTInitAlloc_C_32sc_c;
			ippsFFTGetBufSize_C_32sc_universal =			ippsFFTGetBufSize_C_32sc_c;
			ippsFFTFree_C_32sc_universal =					ippsFFTFree_C_32sc_c;		
			ippsFFTFwd_CToC_32sc_Sfs_universal =			ippsFFTFwd_CToC_32sc_Sfs_c;
#endif

#if defined(KINOMA_AAC)
			ippsFFTInv_CToC_32s_Sfs_universal			= ippsFFTInv_CToC_32s_Sfs_c;
			ippsFFTInit_C_32s_universal					= ippsFFTInit_C_32s_c;		

			/* the following 6 functions exist in sbr code */  
			ippsFFTGetSize_C_32sc_universal =				ippsFFTGetSize_C_32sc_c;	
			ippsFFTInit_C_32sc_universal =					ippsFFTInit_C_32sc_c;		
			ippsFFTInv_CToC_32sc_Sfs_universal =			ippsFFTInv_CToC_32sc_Sfs_c;
			ippsFFTGetSize_C_32s_universal =				ippsFFTGetSize_C_32s_c;	
#endif


#if defined(KINOMA_AAC_ENC)
			ippsCopy_16s_universal = 						ippsCopy_16s_c; 			
			ippsSet_16s_universal = 						ippsSet_16s_c; 			
			ippsAbs_16s_I_universal = 					ippsAbs_16s_I_c; 		
			ippsAbs_16s_universal = 						ippsAbs_16s_c; 			
			ippsMinEvery_32s_I_universal = 				ippsMinEvery_32s_I_c; 	
			ippsMaxEvery_32s_I_universal = 				ippsMaxEvery_32s_I_c; 	
			ippsMinMax_16s_universal = 					ippsMinMax_16s_c; 		
			ippsSum_16s32s_Sfs_universal = 				ippsSum_16s32s_Sfs_c; 	
			ippsSum_32s_Sfs_universal = 					ippsSum_32s_Sfs_c; 		
			ippsAdd_16s_Sfs_universal = 					ippsAdd_16s_Sfs_c; 		

			ippsAdd_16s_ISfs_universal = 					ippsAdd_16s_ISfs_c; 		
			ippsAddC_16s_Sfs_universal = 					ippsAddC_16s_Sfs_c; 		
			ippsSub_16s_universal = 						ippsSub_16s_c; 			
			ippsSub_16s_ISfs_universal = 					ippsSub_16s_ISfs_c; 		
			ippsSub_16s_Sfs_universal = 					ippsSub_16s_Sfs_c; 		
																		
			ippsMul_16s_Sfs_universal = 					ippsMul_16s_Sfs_c; 		
			ippsMul_16s_ISfs_universal = 					ippsMul_16s_ISfs_c; 		
			ippsMul_16s32s_Sfs_universal = 				ippsMul_16s32s_Sfs_c; 	
			ippsMulC_16s_Sfs_universal = 					ippsMulC_16s_Sfs_c; 		
			ippsMul_16s_universal = 						ippsMul_16s_c; 			

			ippsDiv_16s_Sfs_universal = 					ippsDiv_16s_Sfs_c; 		
			ippsDiv_16s_ISfs_universal = 					ippsDiv_16s_ISfs_c; 		
			ippsDivC_16s_ISfs_universal = 				ippsDivC_16s_ISfs_c; 	
			ippsSpread_16s_Sfs_universal = 			    ippsSpread_16s_Sfs_c; 			
			ippsPow34_16s_Sfs_universal = 			    ippsPow34_16s_Sfs_c; 			
			ippsMagnitude_16sc_Sfs_universal = 		    ippsMagnitude_16sc_Sfs_c; 		
			ippsMagnitude_16s_Sfs_universal = 		    ippsMagnitude_16s_Sfs_c; 		
			ippsRShiftC_32s_universal = 				    ippsRShiftC_32s_c; 				
			ippsRShiftC_16s_universal = 				    ippsRShiftC_16s_c; 				
			ippsLShiftC_16s_I_universal = 			    ippsLShiftC_16s_I_c; 			

			ippsDotProd_16s32s32s_Sfs_universal = 	    ippsDotProd_16s32s32s_Sfs_c; 	
			ippsDotProd_16s32s_Sfs_universal = 		    ippsDotProd_16s32s_Sfs_c; 		
			ippsLn_32s16s_Sfs_universal = 			    ippsLn_32s16s_Sfs_c; 			
			ippsDeinterleave_16s_universal = 			    ippsDeinterleave_16s_c; 			


			ippsVLCEncodeBlock_16s1u_universal = 			ippsVLCEncodeBlock_16s1u_c; 			
			ippsVLCEncodeEscBlock_AAC_16s1u_universal = 	ippsVLCEncodeEscBlock_AAC_16s1u_c; 	
			ippsVLCEncodeInitAlloc_32s_universal = 		ippsVLCEncodeInitAlloc_32s_c; 		
			ippsVLCEncodeFree_32s_universal = 			ippsVLCEncodeFree_32s_c; 			
			ippsVLCCountBits_16s32s_universal = 			ippsVLCCountBits_16s32s_c; 			
			ippsVLCCountEscBits_AAC_16s32s_universal = 	ippsVLCCountEscBits_AAC_16s32s_c; 	

			ippsThreshold_LT_16s_I_universal = 			ippsThreshold_LT_16s_I_c; 			
			ippsConjPack_16sc_universal = 				ippsConjPack_16sc_c; 				
			ippsConjCcs_16sc_universal = 					ippsConjCcs_16sc_c; 					


			ippsFFTInitAlloc_R_32s_universal = 			ippsFFTInitAlloc_R_32s_c; 			
			ippsFFTGetBufSize_R_32s_universal =  			ippsFFTGetBufSize_R_32s_c;  			
			ippsFFTGetBufSize_R_16s_universal =  			ippsFFTGetBufSize_R_16s_c;  			
			ippsFFTGetBufSize_C_16sc_universal =  		ippsFFTGetBufSize_C_16sc_c;  		
			ippsMDCTFwdGetBufSize_16s_universal = 		ippsMDCTFwdGetBufSize_16s_c; 		
			ippsMDCTFwdFree_16s_universal = 				ippsMDCTFwdFree_16s_c; 				
			ippsMDCTFwdInitAlloc_16s_universal = 			ippsMDCTFwdInitAlloc_16s_c; 			
			ippsMDCTFwd_16s_Sfs_universal = 				ippsMDCTFwd_16s_Sfs_c; 				
			ippsFFTInitAlloc_R_16s_universal = 			ippsFFTInitAlloc_R_16s_c; 			
			ippsFFTInitAlloc_C_16s_universal = 			ippsFFTInitAlloc_C_16s_c; 			
			ippsFFTInitAlloc_C_16sc_universal = 			ippsFFTInitAlloc_C_16sc_c; 			
			ippsFFTFree_R_16s_universal =  				ippsFFTFree_R_16s_c;  				
			ippsFFTFree_C_16s_universal = 				ippsFFTFree_C_16s_c; 				
			ippsFFTFree_C_16sc_universal = 				ippsFFTFree_C_16sc_c; 				
			//ippsFFTFwd_CToC_16sc_Sfs_universal = 			ippsFFTFwd_CToC_16sc_Sfs_c; 			
			ippsFFTFwd_RToCCS_16s_Sfs_universal = 		ippsFFTFwd_RToCCS_16s_Sfs_c; 		
			ippsFFTFwd_RToPack_32s_Sfs_universal = 		ippsFFTFwd_RToPack_32s_Sfs_c; 		
			ippsFFTInv_PackToR_32s_Sfs_universal = 		ippsFFTInv_PackToR_32s_Sfs_c; 		
#endif



#if defined(KINOMA_MP4V) || defined(KINOMA_MP4V_ENC) 
			//mp4v
			ippiDCT8x8Inv_16s_C1I_universal						=ippiDCT8x8Inv_16s_C1I_c;
			ippiDCT8x8Inv_16s8u_C1R_universal					=ippiDCT8x8Inv_16s8u_C1R_c;

			ippiWarpGetSize_MPEG4_universal						= ippiWarpGetSize_MPEG4_c;						
			ippiQuantInvInterInit_MPEG4_universal				= ippiQuantInvInterInit_MPEG4_c;				
			ippiQuantInvInterGetSize_MPEG4_universal			= ippiQuantInvInterGetSize_MPEG4_c;			
			ippiQuantInvIntraInit_MPEG4_universal				= ippiQuantInvIntraInit_MPEG4_c;				
			ippiQuantInvIntraGetSize_MPEG4_universal			= ippiQuantInvIntraGetSize_MPEG4_c;			

			ippiQuantInvIntra_MPEG4_16s_C1I_universal 			= ippiQuantInvIntra_MPEG4_16s_C1I_c; 			
			ippiAdd8x8_16s8u_C1IRS_universal 					= ippiAdd8x8_16s8u_C1IRS_c;					
			ippiOBMC8x8HP_MPEG4_8u_C1R_universal				= ippiOBMC8x8HP_MPEG4_8u_C1R_c;				
			ippiCopy8x8QP_MPEG4_8u_C1R_universal 				= ippiCopy8x8QP_MPEG4_8u_C1R_c; 				

			ippiOBMC8x8QP_MPEG4_8u_C1R_universal				= ippiOBMC8x8QP_MPEG4_8u_C1R_c;				
			ippiAverage8x8_8u_C1IR_universal					= ippiAverage8x8_8u_C1IR_c;					
			ippiAverage16x16_8u_C1IR_universal					= ippiAverage16x16_8u_C1IR_c;					

			ippiCalcGlobalMV_MPEG4_universal					= ippiCalcGlobalMV_MPEG4_c;				
			ippiWarpChroma_MPEG4_8u_P2R_universal				= ippiWarpChroma_MPEG4_8u_P2R_c;				        
			ippiWarpLuma_MPEG4_8u_C1R_universal					= ippiWarpLuma_MPEG4_8u_C1R_c;					
			ippiWarpInit_MPEG4_universal						= ippiWarpInit_MPEG4_c;						

			ippiCopy8x8_8u_C1R_universal						= ippiCopy8x8_8u_C1R_c;						
			ippiCopy16x16_8u_C1R_universal						= ippiCopy16x16_8u_C1R_c;						
			ippiCopy8x8HP_8u_C1R_universal						= ippiCopy8x8HP_8u_C1R_c;
			ippiCopy16x8HP_8u_C1R_universal						= ippiCopy16x8HP_8u_C1R_c;						
			ippiCopy8x4HP_8u_C1R_universal						= ippiCopy8x4HP_8u_C1R_c;						
			ippiCopy16x16HP_8u_C1R_universal					= ippiCopy16x16HP_8u_C1R_c;					
			ippiCopy16x8QP_MPEG4_8u_C1R_universal				= ippiCopy16x8QP_MPEG4_8u_C1R_c;				
			ippiCopy16x16QP_MPEG4_8u_C1R_universal				= ippiCopy16x16QP_MPEG4_8u_C1R_c;				
#endif

#if defined(KINOMA_MP4V)
			ippiDCT8x8Inv_4x4_16s_C1I_universal					=ippiDCT8x8Inv_4x4_16s_C1I_c;
			ippiChangeSpriteBrightness_MPEG4_8u_C1IR_universal	= ippiChangeSpriteBrightness_MPEG4_8u_C1IR_c;			
			ippiAdd8x8HP_16s8u_C1RS_universal					= ippiAdd8x8HP_16s8u_C1RS_c;					
			ippiReconstructCoeffsInter_MPEG4_1u16s_universal	= ippiReconstructCoeffsInter_MPEG4_1u16s_c;	
			ippiDecodeDCIntra_MPEG4_1u16s_universal				= ippiDecodeDCIntra_MPEG4_1u16s_c;				
			ippiDecodeCoeffsIntra_MPEG4_1u16s_universal 		= ippiDecodeCoeffsIntra_MPEG4_1u16s_c; 		
			//ippiReconstructCoeffsIntra_H263_1u16s_universal 	= ippiReconstructCoeffsIntra_H263_1u16s_c; 	
			//ippiReconstructCoeffsInter_H263_1u16s_universal 	= ippiReconstructCoeffsInter_H263_1u16s_c; 	
			ippiDCT8x8Inv_2x2_16s_C1I_universal					=ippiDCT8x8Inv_2x2_16s_C1I_c;
#endif

#if defined(KINOMA_MP4V_ENC) 
			ippiSAD16x16_8u32s_universal = 					ippiSAD16x16_8u32s_c; 				
			ippiSAD16x8_8u32s_C1R_universal =  				ippiSAD16x8_8u32s_C1R_c;  			
			ippiSAD8x8_8u32s_C1R_universal =  				ippiSAD8x8_8u32s_C1R_c;  			
			ippiMeanAbsDev16x16_8u32s_C1R_universal = 		ippiMeanAbsDev16x16_8u32s_C1R_c; 	
			ippiQuantIntraInit_MPEG4_universal = 			ippiQuantIntraInit_MPEG4_c; 		
			ippiQuantInterInit_MPEG4_universal =  			ippiQuantInterInit_MPEG4_c;  		
			ippsMalloc_32u_universal = 			 			ippsMalloc_32u_c; 			 		
			ippiQuantInterGetSize_MPEG4_universal =  		ippiQuantInterGetSize_MPEG4_c;  	
			ippiQuantIntraGetSize_MPEG4_universal =  		ippiQuantIntraGetSize_MPEG4_c;  	
			ippiQuantIntra_H263_16s_C1I_universal =  		ippiQuantIntra_H263_16s_C1I_c;  	
			ippiQuantIntra_MPEG4_16s_C1I_universal =  		ippiQuantIntra_MPEG4_16s_C1I_c;  	
			ippiDCT8x8Fwd_8u16s_C1R_universal =  			ippiDCT8x8Fwd_8u16s_C1R_c;  		
			ippiQuantInter_H263_16s_C1I_universal =  		ippiQuantInter_H263_16s_C1I_c;  	
			ippiDCT8x8Fwd_16s_C1I_universal =  				ippiDCT8x8Fwd_16s_C1I_c;  			
			ippiSubSAD8x8_8u16s_C1R_universal =  			ippiSubSAD8x8_8u16s_C1R_c;  		
			ippiFrameFieldSAD16x16_8u32s_C1R_universal =  	ippiFrameFieldSAD16x16_8u32s_C1R_c; 
			ippiSub8x8_8u16s_C1R_universal =  				ippiSub8x8_8u16s_C1R_c;  			
			ippiQuantInter_MPEG4_16s_C1I_universal =  		ippiQuantInter_MPEG4_16s_C1I_c;  	
			ippiDCT8x8Fwd_16s_C1R_universal =  				ippiDCT8x8Fwd_16s_C1R_c;  			
			ippsCopy_1u_universal =  						ippsCopy_1u_c;  					
			ippiCopy_8u_C1R_universal =  					ippiCopy_8u_C1R_c;  				
			ippiFrameFieldSAD16x16_16s32s_C1R_universal =  	ippiFrameFieldSAD16x16_16s32s_C1R_c;
			ippiSub16x16_8u16s_C1R_universal =  			ippiSub16x16_8u16s_C1R_c;  			
			ippiQuantInvIntra_H263_16s_C1I_universal =  	ippiQuantInvIntra_H263_16s_C1I_c;  	
			ippiSqrDiff16x16_8u32s_universal =  			ippiSqrDiff16x16_8u32s_c;  			
			ippiSSD8x8_8u32s_C1R_universal =  				ippiSSD8x8_8u32s_C1R_c;  			
			ippiQuantInvInter_H263_16s_C1I_universal =  	ippiQuantInvInter_H263_16s_C1I_c;  	
			ippiQuantInvInter_MPEG4_16s_C1I_universal =  	ippiQuantInvInter_MPEG4_16s_C1I_c;  
			ippiEncodeCoeffsIntra_H263_16s1u_universal =  	ippiEncodeCoeffsIntra_H263_16s1u_c; 
			ippiEncodeDCIntra_H263_16s1u_universal =  		ippiEncodeDCIntra_H263_16s1u_c;  	
			ippiCountZeros8x8_16s_C1_universal =  			ippiCountZeros8x8_16s_C1_c;  		
			ippiEncodeCoeffsIntra_MPEG4_16s1u_universal =  	ippiEncodeCoeffsIntra_MPEG4_16s1u_c;
			ippiEncodeDCIntra_MPEG4_16s1u_universal =  		ippiEncodeDCIntra_MPEG4_16s1u_c;  	
			ippiEncodeCoeffsInter_H263_16s1u_universal =  	ippiEncodeCoeffsInter_H263_16s1u_c; 
			ippiEncodeCoeffsInter_MPEG4_16s1u_universal =  	ippiEncodeCoeffsInter_MPEG4_16s1u_c;
#endif

#if defined(KINOMA_AVC) || defined(KINOMA_AVC_ENC)
			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c;
			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c;
			ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c;
			ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c;

			ippiInterpolateBlock_H264_8u_P2P1R_universal = 						ippiInterpolateBlock_H264_8u_P2P1R_c;
			ippiInterpolateLuma_H264_8u_C1R_universal = 						ippiInterpolateLuma_H264_8u_C1R_c;
			ippiInterpolateChroma_H264_8u_C1R_universal = 						ippiInterpolateChroma_H264_8u_C1R_c;
#endif

#if defined(KINOMA_AVC)
			Init_AVC_CAVLC();
			Init_AVC_Reconstruction(FSK_ARCH_C);

			ippiSet_8u_C1R_universal = 											ippiSet_8u_C1R_c;

			ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal = 				ippiDecodeExpGolombOne_H264_1u16s_unsigned_c;
			ippiDecodeExpGolombOne_H264_1u16s_signed_universal = 				ippiDecodeExpGolombOne_H264_1u16s_signed_c;
			
			ippiDecodeCAVLCCoeffs_H264_1u16s_universal = 						ippiDecodeCAVLCCoeffs_H264_1u16s_c;
			ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_universal = 				ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c;
			ippiReconstructLumaIntraMB_H264_16s8u_C1R_universal = 				ippiReconstructLumaIntraMB_H264_16s8u_C1R_c;
			ippiReconstructLumaInterMB_H264_16s8u_C1R_universal = 				ippiReconstructLumaInterMB_H264_16s8u_C1R_c;
			ippiReconstructChromaIntraMB_H264_16s8u_P2R_universal = 			ippiReconstructChromaIntraMB_H264_16s8u_P2R_c;
			ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c;
			
			//***tmp
			//ippiDecodeCAVLCCoeffs_H264_1u16s_universal = 						ippiDecodeCAVLCCoeffs_H264_1u16s_cc;
			//ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_universal = 				ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_cc;
			//ippiReconstructLumaIntraMB_H264_16s8u_C1R_universal = 				ippiReconstructLumaIntraMB_H264_16s8u_C1R_cc;
			//ippiReconstructLumaInterMB_H264_16s8u_C1R_universal = 				ippiReconstructLumaInterMB_H264_16s8u_C1R_cc;
			//ippiReconstructChromaIntraMB_H264_16s8u_P2R_universal = 			ippiReconstructChromaIntraMB_H264_16s8u_P2R_cc;
			//ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_cc;
			
			ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_universal =     ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_c;
			ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_universal =   ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_c;

			ippiHuffmanRunLevelTableInitAlloc_32s_universal = 					ippiHuffmanRunLevelTableInitAlloc_32s_c;
			ippiHuffmanTableFree_32s_universal = 								ippiHuffmanTableFree_32s_c;
			ippiHuffmanTableInitAlloc_32s_universal = 							ippiHuffmanTableInitAlloc_32s_c;

			ippiInterpolateLumaTop_H264_8u_C1R_universal = 						ippiInterpolateLumaTop_H264_8u_C1R_c;
			ippiInterpolateLumaBottom_H264_8u_C1R_universal = 					ippiInterpolateLumaBottom_H264_8u_C1R_c;
			ippiInterpolateChromaTop_H264_8u_C1R_universal = 					ippiInterpolateChromaTop_H264_8u_C1R_c;							
			ippiInterpolateChromaBottom_H264_8u_C1R_universal = 				ippiInterpolateChromaBottom_H264_8u_C1R_c;

			ippiInterpolateBlock_H264_8u_P3P1R_universal = 						ippiInterpolateBlock_H264_8u_P3P1R_c;	//***
			ippiUniDirWeightBlock_H264_8u_C1R_universal = 						ippiUniDirWeightBlock_H264_8u_C1R_c;
			ippiBiDirWeightBlock_H264_8u_P2P1R_universal = 						ippiBiDirWeightBlock_H264_8u_P2P1R_c;
			ippiBiDirWeightBlock_H264_8u_P3P1R_universal = 						ippiBiDirWeightBlock_H264_8u_P3P1R_c;
			ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_universal = 				ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_c;
			ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_universal = 				ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_c;

			//ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c;
			//***ippiReconstructChromaIntraMB_H264_16s8u_P2R_universal = 			ippiReconstructChromaIntraMB_H264_16s8u_P2R_c;
			ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_universal = 		ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_c;
			//***ippiReconstructLumaIntraMB_H264_16s8u_C1R_universal = 				ippiReconstructLumaIntraMB_H264_16s8u_C1R_c;
			ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_c;
			//***ippiReconstructLumaInterMB_H264_16s8u_C1R_universal = 				ippiReconstructLumaInterMB_H264_16s8u_C1R_c;
			ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_c;
			ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_c;
			ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_c;
			ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_c;
			ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_c;
			ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_c;
			ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_c;
			ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_c;
			ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_universal =     ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_c;
			ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_universal = 			ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_c;
			ippiReconstructChromaInterMB_H264_16s8u_P2R_universal = 			ippiReconstructChromaInterMB_H264_16s8u_P2R_c;
			ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_universal = 			ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_c;

			UMC::IppLumaDeblocking[0]	= ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal;
			UMC::IppLumaDeblocking[1]	= ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal;
			UMC::IppChromaDeblocking[0]	= ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal;
			UMC::IppChromaDeblocking[1]	= ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal;
#endif

#ifdef KINOMA_AVC_ENC
			ippiEncodeCoeffsCAVLC_H264_16s_universal = 						ippiEncodeCoeffsCAVLC_H264_16s_c;					
			ippiEncodeChromaDcCoeffsCAVLC_H264_16s_universal = 				ippiEncodeChromaDcCoeffsCAVLC_H264_16s_c; 			


			ippiTransformDequantLumaDC_H264_16s_C1I_universal =   		    ippiTransformDequantLumaDC_H264_16s_C1I_c;   		
			ippiTransformDequantChromaDC_H264_16s_C1I_universal =  			ippiTransformDequantChromaDC_H264_16s_C1I_c;  		
			ippiDequantTransformResidualAndAdd_H264_16s_C1I_universal = 	ippiDequantTransformResidualAndAdd_H264_16s_C1I_c; 	
			ippiTransformQuantChromaDC_H264_16s_C1I_universal = 			ippiTransformQuantChromaDC_H264_16s_C1I_c; 			
			ippiTransformQuantResidual_H264_16s_C1I_universal =  			ippiTransformQuantResidual_H264_16s_C1I_c;  			
			ippiTransformQuantLumaDC_H264_16s_C1I_universal =  				ippiTransformQuantLumaDC_H264_16s_C1I_c;  			


			ippiResize_8u_C1R_universal = 								    ippiResize_8u_C1R_c; 								
			ippiSAD16x16_8u32s_universal = 									ippiSAD16x16_8u32s_c; 								
			ippiSAD8x8_8u32s_C1R_universal = 								ippiSAD8x8_8u32s_C1R_c; 								
			ippiSAD4x4_8u32s_universal = 									ippiSAD4x4_8u32s_c; 									
			ippiSAD16x16Blocks8x8_8u16u_universal = 						ippiSAD16x16Blocks8x8_8u16u_c; 						
			ippiSAD16x16Blocks4x4_8u16u_universal =  						ippiSAD16x16Blocks4x4_8u16u_c;  						
			ippiSumsDiff16x16Blocks4x4_8u16s_C1_universal = 				ippiSumsDiff16x16Blocks4x4_8u16s_C1_c; 				
			ippiSumsDiff8x8Blocks4x4_8u16s_C1_universal = 				    ippiSumsDiff8x8Blocks4x4_8u16s_C1_c; 				
			ippiGetDiff4x4_8u16s_C1_universal = 							ippiGetDiff4x4_8u16s_C1_c; 							
			ippiEdgesDetect16x16_8u_C1R_universal = 						ippiEdgesDetect16x16_8u_C1R_c; 						

#endif
			result_implementation = FSK_ARCH_C;
			break;
#endif
	}

//***overwrite with arm optimized functions
#ifdef __KINOMA_IPP_ARM_V5__
		if( implementation == FSK_ARCH_ARM_V5 )
		{
			//common
			ippsZero_8u_universal	= ippsZero_8u_arm;	
			ippsSet_8u_universal	= ippsSet_8u_arm;	
			ippsZero_16s_universal	= ippsZero_16u_arm;				
			ippsZero_32s_universal	= ippsZero_32u_arm;					
			//ippsZero_32sc_universal = ippsZero_32sc_arm;					

#if defined(KINOMA_MP4V) || defined(KINOMA_MP4V_ENC) 
			ippiCopy8x8HP_8u_C1R_universal			= ippiCopy8x8HP_8u_C1R_arm;

			////ippi
			////MPEG4 Video
			ippiDCT8x8Inv_16s_C1I_universal		=ippiDCT8x8Inv_16s_C1I_arm;
			ippiDCT8x8Inv_16s8u_C1R_universal	=ippiDCT8x8Inv_16s8u_C1R_arm;
			ippiDCT8x8Inv_4x4_16s_C1I_universal	=ippiDCT8x8Inv_4x4_16s_C1I_arm;
			ippiDCT8x8Inv_2x2_16s_C1I_universal	=ippiDCT8x8Inv_2x2_16s_C1I_arm;
#endif


#if defined(KINOMA_AVC)
			ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal	= 	ippiDecodeExpGolombOne_H264_1u16s_unsigned_arm;
			ippiDecodeExpGolombOne_H264_1u16s_signed_universal		= 	ippiDecodeExpGolombOne_H264_1u16s_signed_arm;
			Init_AVC_CAVLC();
			Init_AVC_Reconstruction(FSK_ARCH_ARM_V5);
#endif

			//ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s_arm;
			//ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s_arm;
			//ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s_arm;

			//ippsVLCDecodeBlock_1u16s_universal		= ippsVLCDecodeBlock_1u16s_arm;
			//ippsVLCDecodeFree_32s_universal			= ippsVLCDecodeFree_32s_arm;
			//ippsVLCDecodeInitAlloc_32s_universal	= ippsVLCDecodeInitAlloc_32s_arm;
			//ippsVLCDecodeOne_1u16s_universal		= ippsVLCDecodeOne_1u16s_arm;
 

			//ippsMalloc_8u_universal =  ippsMalloc_8u_c;		//ippsMalloc_8u_arm;		
			//ippsFree_universal =  ippsFree_c;				//ippsFree_arm;			
			
			//ippiSet_8u_C1R_universal = ippiSet_8u_C1R_c;	//ippiSet_8u_C1R_arm;		
			//ippiInterpolateChroma_H264_8u_C1R_universal = 						ippiInterpolateChroma_H264_8u_C1R_arm;
			//ippiInterpolateBlock_H264_8u_P3P1R_universal = 						ippiInterpolateBlock_H264_8u_P3P1R_arm;
			//ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_arm;


			result_implementation = FSK_ARCH_ARM_V5;

		}
#endif
	
	return result_implementation;
}
