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
#ifndef __KINOMA_IPP_LIB_H__
#define __KINOMA_IPP_LIB_H__

//***
#include "kinoma_ipp_env.h"
#include "FskArch.h"
#include "kinoma_ipp_lib_unimplemented.h"

#include "ippac.h"
#include "ippdc.h"
#include "ipps.h"
#include "ippvc.h"
#include "ippi.h"

#include <stdlib.h>


// Some define used for speed up de-blocking filter
// (Use 8bits, so speed up for interpolation need use 8--16bits)
#define AVC_DEBLOCKING_MASK				0xFF

#define AVC_DEBLOCKING_NORMAL			0x00
#define AVC_DEBLOCKING_DROP_CHROMA		0x01
#define AVC_DEBLOCKING_SIMPLYBS			0x02

// It is valid only when AVC_DEBLOCKING_SIMPLYBS is set
// BS caculation will be further simplied when this is set
#define AVC_DEBLOCKING_SIMPLYBS_INTRA	0x04

#define AVC_DEBLOCKING_DROP_INTERNAL	0x08
// Eliminate branch in real deblocking functions, which means We do filter no-filter based on BS only
#define AVC_DEBLOCKING_NOBRANCH			0x10
#define AVC_DEBLOCKING_NEWALGO_UV		0x20
#define AVC_DEBLOCKING_NEWALGO_Y		0x40

#define AVC_DEBLOCKING_DROP_ALL			0x80

// NEW algorithm related

// Approx for Interpolation
#define AVC_INTERPOLATION__MASK				0xFF00
#define AVC_INTERPOLATION_NORMAL			0x0000
#define AVC_INTERPOLATION_SPEED				0x0100


// Approx for Interpolation
#define AVC_RECONSTRUCTION__MASK			0xFF00
#define AVC_RECONSTRUCTION_NORMAL			0x0000
#define AVC_RECONSTRUCTION_SPEED			0x0200

//*** previously used settings, for reference only
//#define	AVC_LOSSY_SETTING_0				AVC_DEBLOCKING_NORMAL
//#define AVC_LOSSY_SETTING_1				AVC_DEBLOCKING_DROP_CHROMA
//#define AVC_LOSSY_SETTING_2				AVC_DEBLOCKING_SIMPLYBS
//#define AVC_LOSSY_SETTING_3				(AVC_DEBLOCKING_SIMPLYBS|AVC_DEBLOCKING_DROP_CHROMA)
//#define AVC_LOSSY_SETTING_4				AVC_DEBLOCKING_DROP_ALL
//#define AVC_LOSSY_SETTING_5				(AVC_DEBLOCKING_DROP_ALL|AVC_INTERPOLATION_SPEED)

#define	AVC_LOSSY_SETTING_0				AVC_DEBLOCKING_NORMAL

#define AVC_LOSSY_SETTING_1				AVC_DEBLOCKING_DROP_CHROMA
#define AVC_LOSSY_SETTING_2				AVC_DEBLOCKING_SIMPLYBS

// Two recommond setting for De-blocking
#define AVC_LOSSY_SETTING_3				(AVC_DEBLOCKING_SIMPLYBS| AVC_DEBLOCKING_SIMPLYBS_INTRA)
#define AVC_LOSSY_SETTING_4				(AVC_DEBLOCKING_SIMPLYBS| AVC_DEBLOCKING_SIMPLYBS_INTRA|AVC_DEBLOCKING_NOBRANCH)
#define AVC_LOSSY_SETTING_5				(AVC_DEBLOCKING_SIMPLYBS| AVC_DEBLOCKING_SIMPLYBS_INTRA|AVC_DEBLOCKING_DROP_CHROMA)

#define AVC_LOSSY_SETTING_6				(AVC_DEBLOCKING_SIMPLYBS| AVC_DEBLOCKING_SIMPLYBS_INTRA|AVC_DEBLOCKING_NOBRANCH|AVC_DEBLOCKING_DROP_CHROMA)
#define	AVC_LOSSY_SETTING_7				(AVC_DEBLOCKING_SIMPLYBS| AVC_DEBLOCKING_SIMPLYBS_INTRA|AVC_DEBLOCKING_NOBRANCH|AVC_DEBLOCKING_DROP_CHROMA|AVC_DEBLOCKING_DROP_INTERNAL)
#define AVC_LOSSY_SETTING_8				AVC_DEBLOCKING_DROP_ALL

#define AVC_LOSSY_SETTING_9				AVC_LOSSY_SETTING_8
#define AVC_LOSSY_SETTING_10			AVC_LOSSY_SETTING_8
//#define AVC_LOSSY_SETTING_9			(AVC_DEBLOCKING_DROP_ALL|AVC_RECONSTRUCTION_SPEED)
//#define AVC_LOSSY_SETTING_10			(AVC_DEBLOCKING_DROP_ALL|AVC_RECONSTRUCTION_SPEED|AVC_INTERPOLATION_SPEED)


//MP4V 
#define	MP4V_LOSSY_SETTING_0			0	
#define MP4V_LOSSY_SETTING_1			(1<<0)
#define MP4V_LOSSY_SETTING_2			(1<<1)
#define MP4V_LOSSY_SETTING_3			(1<<1)
#define MP4V_LOSSY_SETTING_4			(1<<2)
#define MP4V_LOSSY_SETTING_5			(1<<2)
#define MP4V_LOSSY_SETTING_6			(1<<3)
#define	MP4V_LOSSY_SETTING_7			(1<<3)
#define MP4V_LOSSY_SETTING_8			(1<<4)
#define MP4V_LOSSY_SETTING_9			(1<<4)
#define MP4V_LOSSY_SETTING_10			(1<<5)


#ifdef __cplusplus
extern "C" {
#endif

//Nick test
int kinoma_avc_dec_parse_header_info( int nalu_len_size, unsigned char *data, int size, int *left, int *right, int *top, int *bottom, int *frame_mbs_only_flag, int *profile, int *level );
int kinoma_avc_dec_parse_header_weight( unsigned char* data, int size, int* weighted_pred_flag);

void SwapMemoryAndRemovePreventingBytes_misaligned_c(	unsigned char *pDestination, unsigned char *pSource, int nSrcSize ,int *nDstSize_in_out);
void SwapMemoryAndRemovePreventingBytes_aligned4_c(		unsigned char *pDestination, unsigned char *pSource, int nSrcSize ,int *nDstSize);
void SwapMemoryAndRemovePreventingBytes_aligned4_arm_v6(unsigned char *pDestination, unsigned char *pSource, int nSrcSize ,int *nDstSize);
int  SwapInQTMediaSample3(int spspps_only, int naluLengthSize, unsigned char *src, int size, void *m_void );

//prototypes
//Nick
int kinoma_ipp_lib_avc_parse_init(int implementation);

int kinoma_ipp_lib_avc_init(int implementation);
int	kinoma_ipp_lib_mp4v_deblocking_init(int implementation);
int kinoma_ipp_lib_mp4v_init(int implementation);
int kinoma_ipp_lib_mp3_init(int implementation);
int kinoma_ipp_lib_aac_init(int implementation);
int kinoma_ipp_lib_aac_enc_init(int implementation);
int kinoma_ipp_lib_mp4v_enc_init(int implementation);
int kinoma_ipp_lib_avc_enc_init(int implementation);
int kinoma_ipp_lib_h263_enc_init(int implementation, int is_flv);
void Init_AVC_CAVLC(void);

void Init_AVC_Reconstruction_C();
void Init_AVC_Reconstruction_ARM_V5();
void Init_AVC_Reconstruction_ARM_V6();
void AVC_Reconstruction_SetApprox(int approx_level);

void Init_AVC_Deblocking_C(int for_avc);
void Init_AVC_Deblocking_XScale(int for_avc);
void Init_AVC_Deblocking_ARM_V5(int for_avc);
void Init_AVC_Deblocking_ARM_V6(int for_avc);

void Init_AVC_Interpolation_C(void);
void Init_AVC_Interpolation_XScale(void);
void Init_AVC_Interpolation_ARM_V5(void);
void Init_AVC_Interpolation_ARM_V6(void);

void Init_SAD_Procs();

void deblock_frame(int qp, unsigned char *y, int width, int height, int y_rowbytes );

#define Clip1(limit, val) if (((unsigned int)val) > (unsigned int)limit) val = limit & ~(((signed int) val) >> 31)

int CLZ_c(int value);

#if defined( _WIN32_WCE )

void PLD_arm( void *);
int CLZ_arm( int );
#define PLD(a)		PLD_arm(a)

#if 0
#define CNTLZ(a)	CLZ_arm(a) 
#else
//***need to turn on armv5 to get it work
#include "Cmnintrin.h"
#define CNTLZ		_CountLeadingZeros 

#endif

#elif defined( _WIN32_WINNT )

int CLZ_win32(int value);
#define PLD(a)
#define CNTLZ		CLZ_win32 

#else

int CLZ_c(int value);
#define PLD(a)
#define CNTLZ		CLZ_c 

#endif


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
IppStatus __STDCALL ippsMinIndx_32s_c		(const Ipp32s* pSrc, int len, Ipp32s* pMin, int *indx);
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

//IppStatus __STDCALL ippsFFTInitAlloc_C_32sc_cc(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint);
//IppStatus __STDCALL ippsFFTGetBufSize_C_32sc_cc(const IppsFFTSpec_C_32sc* pFFTSpec, int* pSize);
//IppStatus __STDCALL ippsFFTFree_C_32sc_cc	(IppsFFTSpec_C_32sc* pFFTSpec);
//IppStatus __STDCALL ippsFFTFwd_CToC_32sc_Sfs_cc(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer);


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

//
IppStatus __STDCALL ippsSynthPQMF_MP3_32s16s_c(Ipp32s* pSrcY, Ipp16s* pDstAudioOut, Ipp32s* pVBuffer, int* pVPosition, int mode);
IppStatus __STDCALL ippsVLCDecodeEscBlock_AAC_1u16s_c(Ipp8u **ppBitStream, int* pBitOffset, Ipp16s *pData, int len, const IppsVLCDecodeSpec_32s *pVLCSpec);
IppStatus __STDCALL ippsVLCDecodeEscBlock_MP3_1u16s_c(Ipp8u **ppBitStream, int *pBitOffset, int linbits, Ipp16s *pData, int len, const IppsVLCDecodeSpec_32s *pVLCSpec);

IppStatus __STDCALL ippsVLCDecodeBlock_1u16s_c (Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s* pDst, int dstLen, const IppsVLCDecodeSpec_32s* pVLCSpec);
void	  __STDCALL ippsVLCDecodeFree_32s_c (IppsVLCDecodeSpec_32s* pVLCSpec);
IppStatus __STDCALL ippsVLCDecodeInitAlloc_32s_c (const IppsVLCTable_32s* pInputTable, int inputTableSize, Ipp32s* pSubTablesSizes, int numSubTables, IppsVLCDecodeSpec_32s** ppVLCSpec);
IppStatus __STDCALL ippsVLCDecodeOne_1u16s_c (Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s* pDst, const IppsVLCDecodeSpec_32s* pVLCSpec);


//ippi
//MPEG4 Video
IppStatus __STDCALL ippiDCT8x8Inv_16s_C1I_c(Ipp16s* pSrcDst);
IppStatus __STDCALL ippiDCT8x8Inv_4x4_16s_C1I_c(Ipp16s* pSrcDst);
IppStatus __STDCALL ippiDCT8x8Inv_2x2_16s_C1I_c(Ipp16s* pSrcDst);
IppStatus __STDCALL ippiDCT8x8Inv_DC_16s_C1I_c(Ipp16s* pSrcDst);
IppStatus __STDCALL ippiDCT8x8Inv_16s8u_C1R_c(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
IppStatus __STDCALL ippiDCT8x8Inv_4x4_16s8u_C1R_c(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
IppStatus __STDCALL ippiDCT8x8Inv_2x2_16s8u_C1R_c(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
IppStatus __STDCALL ippiDCT8x8Inv_DC_16s8u_C1R_c(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);

IppStatus __STDCALL ippiDCT8x8Inv_16s_C1I_arm_v5(Ipp16s* pSrcDst);
IppStatus __STDCALL ippiDCT8x8Inv_16s8u_C1R_arm_v5(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
IppStatus __STDCALL ippiDCT8x8Inv_4x4_16s8u_C1R_arm_v5(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
IppStatus __STDCALL ippiDCT8x8Inv_4x4_16s_C1I_arm_v5(Ipp16s* pSrcDst);
IppStatus __STDCALL ippiDCT8x8Inv_2x2_16s_C1I_arm_v5(Ipp16s* pSrcDst);
//IppStatus __STDCALL ippiDCT8x8Inv_DC_16s_C1I_arm(Ipp16s* pSrcDst);
IppStatus __STDCALL ippiDCT8x8Inv_2x2_16s8u_C1R_arm_v5(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
//IppStatus __STDCALL ippiDCT8x8Inv_DC_16s8u_C1R_arm(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
IppStatus __STDCALL ippiDCT8x8Inv_16s_C1I_arm_v6(Ipp16s* pSrcDst);
IppStatus __STDCALL ippiDCT8x8Inv_16s8u_C1R_arm_v6(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
IppStatus __STDCALL ippiDCT8x8Inv_4x4_16s_C1I_arm_v6(Ipp16s* pSrcDst);
IppStatus __STDCALL ippiDCT8x8Inv_4x4_16s8u_C1R_arm_v6(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
IppStatus __STDCALL ippiDCT8x8Inv_2x2_16s_C1I_arm_v6(Ipp16s* pSrcDst);
IppStatus __STDCALL ippiDCT8x8Inv_2x2_16s8u_C1R_arm_v6(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);

IppStatus __STDCALL ippiWarpGetSize_MPEG4_c( int*  pSpecSize);
IppStatus __STDCALL ippiQuantInvInterInit_MPEG4_c( const Ipp8u* pQuantMatrix, IppiQuantInvInterSpec_MPEG4* pSpec, int bitsPerPixel);
IppStatus __STDCALL ippiQuantInvInterGetSize_MPEG4_c( int* pSpecSize);
IppStatus __STDCALL ippiQuantInvIntraInit_MPEG4_c(const Ipp8u* pQuantMatrix, IppiQuantInvIntraSpec_MPEG4* pSpec, int bitsPerPixel);
IppStatus __STDCALL ippiQuantInvIntraGetSize_MPEG4_c(int* pSpecSize);
IppStatus __STDCALL ippiReconstructCoeffsIntra_H263_1u16s_c( Ipp8u** ppBitStream, int* pBitOffset, Ipp16s* pCoef, int* pIndxLastNonZero, int cbp, int QP, int advIntraFlag, int scan, int modQuantFlag);

IppStatus __STDCALL ippiReconstructCoeffsInter_H263_1u16s_c(Ipp8u** ppBitStream,int* pBitOffset,Ipp16s* pCoef,int* pIndxLastNonZero, int QP, int modQuantFlag);
IppStatus __STDCALL ippiQuantInvIntra_MPEG4_16s_C1I_c(Ipp16s* pCoeffs,int indxLastNonZero,const IppiQuantInvIntraSpec_MPEG4* pSpec, int QP,int blockType);
IppStatus __STDCALL ippiDecodeCoeffsIntra_MPEG4_1u16s_c(Ipp8u**  ppBitStream,int* pBitOffset,Ipp16s*  pCoeffs, int* pIndxLastNonZero, int rvlcFlag, int noDCFlag,int scan);
IppStatus __STDCALL ippiDecodeDCIntra_MPEG4_1u16s_c(Ipp8u **ppBitStream, int *pBitOffset,Ipp16s *pDC, int blockType);
IppStatus __STDCALL ippiReconstructCoeffsInter_MPEG4_1u16s_c(Ipp8u**ppBitStream, int*pBitOffset,Ipp16s* pCoeffs,int* pIndxLastNonZero,int rvlcFlag,int scan,const IppiQuantInvInterSpec_MPEG4* pQuantInvInterSpec,int QP);
IppStatus __STDCALL ippiAdd8x8_16s8u_C1IRS_c(const Ipp16s* pSrc, int srcStep,Ipp8u* pSrcDst,int srcDstStep);
IppStatus __STDCALL ippiAdd8x8_16s8u_C1IRS_arm_v6(const Ipp16s* pSrc, int srcStep,Ipp8u* pSrcDst,int srcDstStep);
IppStatus __STDCALL ippiOBMC8x8HP_MPEG4_8u_C1R_c(const Ipp8u* pSrc, int srcStep, Ipp8u* pDst,int dstStep,const IppMotionVector* pMVCur,const IppMotionVector* pMVLeft, const IppMotionVector* pMVRight,const IppMotionVector* pMVAbove, const IppMotionVector* pMVBelow, int rounding);
IppStatus __STDCALL ippiCopy8x8QP_MPEG4_8u_C1R_c(const Ipp8u* pSrc,int srcStep,Ipp8u* pDst,int dstStep,int acc,int rounding);

IppStatus __STDCALL ippiOBMC8x8QP_MPEG4_8u_C1R_c(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep, const IppMotionVector* pMVCur, const IppMotionVector* pMVLeft, const IppMotionVector* pMVRight,const IppMotionVector* pMVAbove, const IppMotionVector* pMVBelow,int rounding);
IppStatus __STDCALL ippiAverage8x8_8u_C1IR_c(const Ipp8u*  pSrc,int srcStep,Ipp8u* pSrcDst,int srcDstStep);
IppStatus __STDCALL ippiAverage16x16_8u_C1IR_c(const Ipp8u*  pSrc,int srcStep,Ipp8u* pSrcDst,int srcDstStep);
IppStatus __STDCALL ippiAdd8x8HP_16s8u_C1RS_c(const Ipp16s* pSrc1,int src1Step,Ipp8u* pSrc2,int src2Step,Ipp8u* pDst,int dstStep, int acc, int rounding);
IppStatus __STDCALL ippiCopy8x4HP_8u_C1R_c(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc,int rounding);
IppStatus __STDCALL ippiCopy8x8HP_8u_C1R_c(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding);

IppStatus __STDCALL ippiCopy8x8HP_8u_C1R_arm(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding);

IppStatus __STDCALL ippiCopy16x8HP_8u_C1R_c(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding);
IppStatus __STDCALL ippiCopy16x8QP_MPEG4_8u_C1R_c(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding);
IppStatus __STDCALL ippiCopy16x16HP_8u_C1R_c(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding);
IppStatus __STDCALL ippiCopy16x16QP_MPEG4_8u_C1R_c(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding);
IppStatus __STDCALL ippiCopy8x8_8u_C1R_c(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep);
IppStatus __STDCALL ippiCopy16x16_8u_C1R_c(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep);

IppStatus __STDCALL ippiCalcGlobalMV_MPEG4_c(int xOffset, int yOffset, IppMotionVector* pGMV,const IppiWarpSpec_MPEG4*  pSpec);
IppStatus __STDCALL ippiWarpChroma_MPEG4_8u_P2R_c(const Ipp8u* pSrcCb, int srcStepCb,const Ipp8u* pSrcCr,int srcStepCr,Ipp8u* pDstCb, int dstStepCb,Ipp8u* pDstCr,int dstStepCr, const IppiRect* dstRect, const IppiWarpSpec_MPEG4* pSpec);    
IppStatus __STDCALL ippiChangeSpriteBrightness_MPEG4_8u_C1IR_c(Ipp8u*  pSrcDst, int srcDstStep, int width, int height, int brightnessChangeFactor);
IppStatus __STDCALL ippiWarpLuma_MPEG4_8u_C1R_c(const Ipp8u* pSrcY,int srcStepY, Ipp8u* pDstY, int dstStepY, const IppiRect* dstRect,const IppiWarpSpec_MPEG4* pSpec);
IppStatus __STDCALL ippiWarpInit_MPEG4_c(IppiWarpSpec_MPEG4* pSpec, const int* pDU, const int* pDV, int numWarpingPoints, int spriteType, int warpingAccuracy, int roundingType, int quarterSample, int fcode, const IppiRect* spriteRect, const IppiRect* vopRect);

IppStatus __STDCALL ippiAdd8x8HP_16s8u_C1RS_arm(const Ipp16s* pSrc1,int src1Step,Ipp8u* pSrc2,int src2Step,Ipp8u* pDst,int dstStep, int acc, int rounding);


extern IppStatus (__STDCALL *ippsSynthPQMF_MP3_32s16s_universal)		(Ipp32s* pSrcY, Ipp16s* pDstAudioOut, Ipp32s* pVBuffer, int* pVPosition, int mode);
extern IppStatus (__STDCALL *ippsVLCDecodeEscBlock_AAC_1u16s_universal) (Ipp8u **ppBitStream, int* pBitOffset, Ipp16s *pData, int len, const IppsVLCDecodeSpec_32s *pVLCSpec);
extern IppStatus (__STDCALL *ippsVLCDecodeEscBlock_MP3_1u16s_universal) (Ipp8u **ppBitStream, int *pBitOffset, int linbits, Ipp16s *pData, int len, const IppsVLCDecodeSpec_32s *pVLCSpec);

extern IppStatus (__STDCALL *ippsVLCDecodeBlock_1u16s_universal)		(Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s* pDst, int dstLen, const IppsVLCDecodeSpec_32s* pVLCSpec);
extern void		 (__STDCALL *ippsVLCDecodeFree_32s_universal)			(IppsVLCDecodeSpec_32s* pVLCSpec);
extern IppStatus (__STDCALL *ippsVLCDecodeInitAlloc_32s_universal )		(const IppsVLCTable_32s* pInputTable, int inputTableSize, Ipp32s* pSubTablesSizes, int numSubTables, IppsVLCDecodeSpec_32s** ppVLCSpec);
extern IppStatus (__STDCALL *ippsVLCDecodeOne_1u16s_universal)			(Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s* pDst, const IppsVLCDecodeSpec_32s* pVLCSpec);

//ippi
//MPEG4 Video
extern IppStatus (__STDCALL *ippiDCT8x8Inv_16s_C1I_universal)			(Ipp16s* pSrcDst);
extern IppStatus (__STDCALL *ippiDCT8x8Inv_4x4_16s_C1I_universal)		(Ipp16s* pSrcDst);
extern IppStatus (__STDCALL *ippiDCT8x8Inv_2x2_16s_C1I_universal)		(Ipp16s* pSrcDst);
extern IppStatus (__STDCALL *ippiDCT8x8Inv_DC_16s_C1I_universal)		(Ipp16s* pSrcDst);
extern IppStatus (__STDCALL *ippiDCT8x8Inv_16s8u_C1R_universal)			(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
extern IppStatus (__STDCALL *ippiDCT8x8Inv_4x4_16s8u_C1R_universal)		(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
extern IppStatus (__STDCALL *ippiDCT8x8Inv_2x2_16s8u_C1R_universal)		(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
extern IppStatus (__STDCALL *ippiDCT8x8Inv_DC_16s8u_C1R_universal)		(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);

extern IppStatus (__STDCALL *ippiWarpGetSize_MPEG4_universal)			( int*  pSpecSize);
extern IppStatus (__STDCALL *ippiQuantInvInterInit_MPEG4_universal)	( const Ipp8u* pQuantMatrix, IppiQuantInvInterSpec_MPEG4* pSpec, int bitsPerPixel);
extern IppStatus (__STDCALL *ippiQuantInvInterGetSize_MPEG4_universal) ( int* pSpecSize);
extern IppStatus (__STDCALL *ippiQuantInvIntraInit_MPEG4_universal)	(const Ipp8u* pQuantMatrix, IppiQuantInvIntraSpec_MPEG4* pSpec, int bitsPerPixel);
extern IppStatus (__STDCALL *ippiQuantInvIntraGetSize_MPEG4_universal) (int* pSpecSize);
extern IppStatus (__STDCALL *ippiReconstructCoeffsIntra_H263_1u16s_universal) ( Ipp8u** ppBitStream, int* pBitOffset, Ipp16s* pCoef, int* pIndxLastNonZero, int cbp, int QP, int advIntraFlag, int scan, int modQuantFlag);

extern IppStatus (__STDCALL *ippiReconstructCoeffsInter_H263_1u16s_universal) 	(Ipp8u** ppBitStream,int* pBitOffset,Ipp16s* pCoef,int* pIndxLastNonZero, int QP, int modQuantFlag);
extern IppStatus (__STDCALL *ippiQuantInvIntra_MPEG4_16s_C1I_universal)			(Ipp16s* pCoeffs,int indxLastNonZero,const IppiQuantInvIntraSpec_MPEG4* pSpec, int QP,int blockType);
extern IppStatus (__STDCALL *ippiDecodeCoeffsIntra_MPEG4_1u16s_universal)		(Ipp8u**  ppBitStream,int* pBitOffset,Ipp16s*  pCoeffs, int* pIndxLastNonZero, int rvlcFlag, int noDCFlag,int scan);
extern IppStatus (__STDCALL *ippiDecodeDCIntra_MPEG4_1u16s_universal)			(Ipp8u **ppBitStream, int *pBitOffset,Ipp16s *pDC, int blockType);
extern IppStatus (__STDCALL *ippiReconstructCoeffsInter_MPEG4_1u16s_universal)	(Ipp8u**ppBitStream, int*pBitOffset,Ipp16s* pCoeffs,int* pIndxLastNonZero,int rvlcFlag,int scan,const IppiQuantInvInterSpec_MPEG4* pQuantInvInterSpec,int QP);
extern IppStatus (__STDCALL *ippiAdd8x8_16s8u_C1IRS_universal)					(const Ipp16s* pSrc, int srcStep,Ipp8u* pSrcDst,int srcDstStep);
extern IppStatus (__STDCALL *ippiOBMC8x8HP_MPEG4_8u_C1R_universal)				(const Ipp8u* pSrc, int srcStep, Ipp8u* pDst,int dstStep,const IppMotionVector* pMVCur,const IppMotionVector* pMVLeft, const IppMotionVector* pMVRight,const IppMotionVector* pMVAbove, const IppMotionVector* pMVBelow, int rounding);
extern IppStatus (__STDCALL *ippiCopy8x8QP_MPEG4_8u_C1R_universal)				(const Ipp8u* pSrc,int srcStep,Ipp8u* pDst,int dstStep,int acc,int rounding);

extern IppStatus (__STDCALL *ippiOBMC8x8QP_MPEG4_8u_C1R_universal)				(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep, const IppMotionVector* pMVCur, const IppMotionVector* pMVLeft, const IppMotionVector* pMVRight,const IppMotionVector* pMVAbove, const IppMotionVector* pMVBelow,int rounding);
extern IppStatus (__STDCALL *ippiAverage8x8_8u_C1IR_universal)					(const Ipp8u*  pSrc,int srcStep,Ipp8u* pSrcDst,int srcDstStep);
extern IppStatus (__STDCALL *ippiAverage16x16_8u_C1IR_universal)				(const Ipp8u*  pSrc,int srcStep,Ipp8u* pSrcDst,int srcDstStep);
extern IppStatus (__STDCALL *ippiAdd8x8HP_16s8u_C1RS_universal)					(const Ipp16s* pSrc1,int src1Step,Ipp8u* pSrc2,int src2Step,Ipp8u* pDst,int dstStep, int acc, int rounding);
extern IppStatus (__STDCALL *ippiCopy8x4HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc,int rounding);
extern IppStatus (__STDCALL *ippiCopy8x8HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc,int rounding);
extern IppStatus (__STDCALL *ippiCopy16x8HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding);
extern IppStatus (__STDCALL *ippiCopy16x8QP_MPEG4_8u_C1R_universal)				(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding);
extern IppStatus (__STDCALL *ippiCopy16x16HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding);
extern IppStatus (__STDCALL *ippiCopy16x16QP_MPEG4_8u_C1R_universal)			(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding);
extern IppStatus (__STDCALL *ippiCopy8x8_8u_C1R_universal)						(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep);
extern IppStatus (__STDCALL *ippiCopy16x16_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep);

extern IppStatus (__STDCALL *ippiCalcGlobalMV_MPEG4_universal)						(int xOffset, int yOffset, IppMotionVector* pGMV,const IppiWarpSpec_MPEG4*  pSpec);
extern IppStatus (__STDCALL *ippiWarpChroma_MPEG4_8u_P2R_universal)					(const Ipp8u* pSrcCb, int srcStepCb,const Ipp8u* pSrcCr,int srcStepCr,Ipp8u* pDstCb, int dstStepCb,Ipp8u* pDstCr,int dstStepCr, const IppiRect* dstRect, const IppiWarpSpec_MPEG4* pSpec);    
extern IppStatus (__STDCALL *ippiChangeSpriteBrightness_MPEG4_8u_C1IR_universal) 	(Ipp8u*  pSrcDst, int srcDstStep, int width, int height, int brightnessChangeFactor);
extern IppStatus (__STDCALL *ippiWarpLuma_MPEG4_8u_C1R_universal)					(const Ipp8u* pSrcY,int srcStepY, Ipp8u* pDstY, int dstStepY, const IppiRect* dstRect,const IppiWarpSpec_MPEG4* pSpec);
extern IppStatus (__STDCALL *ippiWarpInit_MPEG4_universal)							(IppiWarpSpec_MPEG4* pSpec, const int* pDU, const int* pDV, int numWarpingPoints, int spriteType, int warpingAccuracy, int roundingType, int quarterSample, int fcode, const IppiRect* spriteRect, const IppiRect* vopRect);

//
extern Ipp8u*    (__STDCALL  *ippsMalloc_8u_universal)		(int len);
extern Ipp32s*   (__STDCALL  *ippsMalloc_32s_universal)		(int len);
extern void		 (__STDCALL *ippsFree_universal)			(void* ptr);
extern IppStatus (__STDCALL *ippsCopy_8u_universal)			(const Ipp8u* pSrc, Ipp8u* pDst, int len);
extern IppStatus (__STDCALL *ippsCopy_32s_universal)		(const Ipp32s* pSrc, Ipp32s* pDst, int len);
extern IppStatus (__STDCALL *ippsZero_8u_universal)			(Ipp8u* pDst, int len);
extern IppStatus (__STDCALL  *ippsZero_16s_universal)		(Ipp16s* pDst, int len);
extern IppStatus (__STDCALL *ippsZero_32s_universal)		(Ipp32s* pDst, int len);
extern IppStatus (__STDCALL *ippsZero_32sc_universal)		(Ipp32sc* pDst, int len);
extern IppStatus (__STDCALL *ippsAdd_32s_Sfs_universal)		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsAdd_32s_ISfs_universal)	(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMul_32s_ISfs_universal)	(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMul_32s_Sfs_universal)		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst,int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsSub_32s_Sfs_universal)		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsSortAscend_32s_I_universal)(Ipp32s* pSrcDst, int len);
extern IppStatus (__STDCALL *ippsMinMax_32s_universal)		(const Ipp32s* pSrc, int len, Ipp32s* pMin, Ipp32s* pMax);
extern IppStatus (__STDCALL *ippsMax_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMax);
extern IppStatus (__STDCALL *ippsMax_16s_universal)			(const Ipp16s* pSrc, int len, Ipp16s* pMax);
extern IppStatus (__STDCALL *ippsMin_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMin);
extern IppStatus (__STDCALL *ippsMinIndx_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMin, int *indx);
extern IppStatus (__STDCALL *ippsMaxAbs_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMaxAbs);
extern IppStatus (__STDCALL *ippsMove_32s_universal)			(const Ipp32s* pSrc, Ipp32s* pDst, int len);
extern IppStatus (__STDCALL *ippsDiv_32s_ISfs_universal)		(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int ScaleFactor);
extern IppStatus (__STDCALL *ippsSqrt_64s_ISfs_universal)		(Ipp64s* pSrcDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsConvert_64s32s_Sfs_universal)	(const Ipp64s* pSrc, Ipp32s* pDst, int len, IppRoundMode rndMode, int scaleFactor);
extern IppStatus (__STDCALL *ippsConvert_32s16s_Sfs_universal)	(const Ipp32s* pSrc, Ipp16s* pDst, int len, int scaleFactor);
extern IppStatus  (__STDCALL *ippsLShiftC_32s_I_universal)		(int val, Ipp32s* pSrcDst, int len);
extern IppStatus (__STDCALL *ippsRShiftC_32s_I_universal)		(int val, Ipp32s* pSrcDst, int len);
 /* the following 4 functions used in MDCT*/
extern IppStatus (__STDCALL *ippsFFTInitAlloc_C_32sc_universal)  (IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint);
extern IppStatus (__STDCALL *ippsFFTGetBufSize_C_32sc_universal) (const IppsFFTSpec_C_32sc* pFFTSpec, int* pSize);
extern IppStatus (__STDCALL *ippsFFTFree_C_32sc_universal)		 (IppsFFTSpec_C_32sc* pFFTSpec);
extern IppStatus (__STDCALL *ippsFFTFwd_CToC_32sc_Sfs_universal) (const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer);
 /* the following 6 functions exist in sbr code */
extern IppStatus (__STDCALL *ippsFFTGetSize_C_32sc_universal)	(int order, int flag, IppHintAlgorithm hint, int* pSizeSpec, int* pSizeInit, int* pSizeBuf);
extern IppStatus (__STDCALL *ippsFFTInit_C_32sc_universal)		(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit);
extern IppStatus (__STDCALL *ippsFFTInv_CToC_32sc_Sfs_universal)(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer);
extern IppStatus (__STDCALL *ippsFFTGetSize_C_32s_universal)	(int order, int flag, IppHintAlgorithm hint, int* pSizeSpec, int* pSizeInit, int* pSizeBuf);
extern IppStatus (__STDCALL *ippsFFTInit_C_32s_universal)		(IppsFFTSpec_C_32s** ppFFTSpec, int order, int flag,IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit);
extern IppStatus (__STDCALL *ippsFFTInv_CToC_32s_Sfs_universal) (const Ipp32s* pSrcRe, const Ipp32s* pSrcIm, Ipp32s* pDstRe, Ipp32s* pDstIm, const IppsFFTSpec_C_32s* pFFTSpec, int scaleFactor, Ipp8u* pBuffer);
 /*following functions are for 5.1  aac*/
extern IppStatus (__STDCALL *ippsMul_32sc_Sfs_universal)		(const Ipp32sc* pSrc1, const Ipp32sc* pSrc2, Ipp32sc* pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMul_32s32sc_Sfs_universal)		(const Ipp32s* pSrc1, const Ipp32sc* pSrc2, Ipp32sc* pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMulC_32s_Sfs_universal)		(const Ipp32s*  pSrc, Ipp32s  val,Ipp32s*  pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsSet_8u_universal)				(Ipp8u val, Ipp8u* pDst, int len);
 /* following functions only used in mp3*/
extern IppStatus (__STDCALL *ippsAddC_32s_ISfs_universal)		(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMulC_32s_ISfs_universal)		(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor);


//make changes closer to orginal code
#define ippsSynthPQMF_MP3_32s16s_x			(*ippsSynthPQMF_MP3_32s16s_universal)
#define ippsVLCDecodeEscBlock_AAC_1u16s_x  (*ippsVLCDecodeEscBlock_AAC_1u16s_universal)
#define ippsVLCDecodeEscBlock_MP3_1u16s_x  (*ippsVLCDecodeEscBlock_MP3_1u16s_universal)

#define ippsVLCDecodeBlock_1u16s_x			(*ippsVLCDecodeBlock_1u16s_universal)
#define ippsVLCDecodeFree_32s_x				(*ippsVLCDecodeFree_32s_universal)
#define ippsVLCDecodeInitAlloc_32s_x		(*ippsVLCDecodeInitAlloc_32s_universal)
#define ippsVLCDecodeOne_1u16s_x			(*ippsVLCDecodeOne_1u16s_universal)

//ippi
//MPEG4 Video
#define ippiDCT8x8Inv_16s_C1I_x				(*ippiDCT8x8Inv_16s_C1I_universal)
#define ippiDCT8x8Inv_4x4_16s_C1I_x			(*ippiDCT8x8Inv_4x4_16s_C1I_universal)
#define ippiDCT8x8Inv_2x2_16s_C1I_x			(*ippiDCT8x8Inv_2x2_16s_C1I_universal)
#define ippiDCT8x8Inv_DC_16s_C1I_x			(*ippiDCT8x8Inv_DC_16s_C1I_universal)
#define ippiDCT8x8Inv_16s8u_C1R_x			(*ippiDCT8x8Inv_16s8u_C1R_universal)
#define ippiDCT8x8Inv_4x4_16s8u_C1R_x		(*ippiDCT8x8Inv_4x4_16s8u_C1R_universal)
#define ippiDCT8x8Inv_2x2_16s8u_C1R_x		(*ippiDCT8x8Inv_2x2_16s8u_C1R_universal)
#define ippiDCT8x8Inv_DC_16s8u_C1R_x		(*ippiDCT8x8Inv_DC_16s8u_C1R_universal)

#define ippiWarpGetSize_MPEG4_x					(*ippiWarpGetSize_MPEG4_universal)			
#define ippiQuantInvInterInit_MPEG4_x			(*ippiQuantInvInterInit_MPEG4_universal)	
#define ippiQuantInvInterGetSize_MPEG4_x		(*ippiQuantInvInterGetSize_MPEG4_universal)
#define ippiQuantInvIntraInit_MPEG4_x			(*ippiQuantInvIntraInit_MPEG4_universal)
#define ippiQuantInvIntraGetSize_MPEG4_x		(*ippiQuantInvIntraGetSize_MPEG4_universal) 
#define ippiReconstructCoeffsIntra_H263_1u16s_x (*ippiReconstructCoeffsIntra_H263_1u16s_universal)

#define ippiReconstructCoeffsInter_H263_1u16s_x 		(*ippiReconstructCoeffsInter_H263_1u16s_universal) 	
#define ippiQuantInvIntra_MPEG4_16s_C1I_x 				(*ippiQuantInvIntra_MPEG4_16s_C1I_universal)			
#define ippiDecodeCoeffsIntra_MPEG4_1u16s_x 			(*ippiDecodeCoeffsIntra_MPEG4_1u16s_universal)		
#define ippiDecodeDCIntra_MPEG4_1u16s_x					(*ippiDecodeDCIntra_MPEG4_1u16s_universal)			
#define ippiReconstructCoeffsInter_MPEG4_1u16s_x		(*ippiReconstructCoeffsInter_MPEG4_1u16s_universal)	
#define ippiAdd8x8_16s8u_C1IRS_x 						(*ippiAdd8x8_16s8u_C1IRS_universal)					
#define ippiOBMC8x8HP_MPEG4_8u_C1R_x					(*ippiOBMC8x8HP_MPEG4_8u_C1R_universal)				
#define ippiCopy8x8QP_MPEG4_8u_C1R_x 					(*ippiCopy8x8QP_MPEG4_8u_C1R_universal)				

#define ippiOBMC8x8QP_MPEG4_8u_C1R_x					(*ippiOBMC8x8QP_MPEG4_8u_C1R_universal)		
#define ippiAverage8x8_8u_C1IR_x						(*ippiAverage8x8_8u_C1IR_universal)			
#define ippiAverage16x16_8u_C1IR_x						(*ippiAverage16x16_8u_C1IR_universal)		
#define ippiAdd8x8HP_16s8u_C1RS_x						(*ippiAdd8x8HP_16s8u_C1RS_universal)			
#define ippiCopy8x4HP_8u_C1R_x							(*ippiCopy8x4HP_8u_C1R_universal)			
#define ippiCopy8x8HP_8u_C1R_x							(*ippiCopy8x8HP_8u_C1R_universal)			
#define ippiCopy16x8HP_8u_C1R_x							(*ippiCopy16x8HP_8u_C1R_universal)			
#define ippiCopy16x8QP_MPEG4_8u_C1R_x					(*ippiCopy16x8QP_MPEG4_8u_C1R_universal)		
#define ippiCopy16x16HP_8u_C1R_x						(*ippiCopy16x16HP_8u_C1R_universal)			
#define ippiCopy16x16QP_MPEG4_8u_C1R_x					(*ippiCopy16x16QP_MPEG4_8u_C1R_universal)	
#define ippiCopy8x8_8u_C1R_x							(*ippiCopy8x8_8u_C1R_universal)	
#define ippiCopy16x16_8u_C1R_x							(*ippiCopy16x16_8u_C1R_universal)

#define ippiCalcGlobalMV_MPEG4_x						(*ippiCalcGlobalMV_MPEG4_universal)						
#define ippiWarpChroma_MPEG4_8u_P2R_x					(*ippiWarpChroma_MPEG4_8u_P2R_universal)				    
#define ippiChangeSpriteBrightness_MPEG4_8u_C1IR_x		(*ippiChangeSpriteBrightness_MPEG4_8u_C1IR_universal) 	
#define ippiWarpLuma_MPEG4_8u_C1R_x						(*ippiWarpLuma_MPEG4_8u_C1R_universal)					
#define ippiWarpInit_MPEG4_x							(*ippiWarpInit_MPEG4_universal)			



//***
#define ippsMalloc_8u_x								(*ippsMalloc_8u_universal)			
#define ippsMalloc_32s_x							(*ippsMalloc_32s_universal)		
#define ippsFree_x									(*ippsFree_universal)				
#define ippsCopy_8u_x								(*ippsCopy_8u_universal)			
#define ippsCopy_32s_x								(*ippsCopy_32s_universal)		
#define ippsZero_8u_x								(*ippsZero_8u_universal)			
#define ippsZero_16s_x								(*ippsZero_16s_universal)		
#define ippsZero_32s_x								(*ippsZero_32s_universal)		
#define ippsZero_32sc_x								(*ippsZero_32sc_universal)		
#define ippsAdd_32s_Sfs_x							(*ippsAdd_32s_Sfs_universal)		
#define ippsAdd_32s_ISfs_x							(*ippsAdd_32s_ISfs_universal)	
#define ippsMul_32s_ISfs_x							(*ippsMul_32s_ISfs_universal)	
#define ippsMul_32s_Sfs_x							(*ippsMul_32s_Sfs_universal)		
#define ippsSub_32s_Sfs_x							(*ippsSub_32s_Sfs_universal)		
#define ippsSortAscend_32s_I_x						(*ippsSortAscend_32s_I_universal)
#define ippsMinMax_32s_x							(*ippsMinMax_32s_universal)		
#define ippsMax_32s_x								(*ippsMax_32s_universal)			
#define ippsMax_16s_x								(*ippsMax_16s_universal)			
#define ippsMin_32s_x								(*ippsMin_32s_universal)			
#define ippsMinIndx_32s_x							(*ippsMinIndx_32s_universal)			
#define ippsMaxAbs_32s_x							(*ippsMaxAbs_32s_universal)		
#define ippsMove_32s_x								(*ippsMove_32s_universal)		
#define ippsDiv_32s_ISfs_x							(*ippsDiv_32s_ISfs_universal)	
#define ippsSqrt_64s_ISfs_x							(*ippsSqrt_64s_ISfs_universal)	
#define ippsConvert_64s32s_Sfs_x					(*ippsConvert_64s32s_Sfs_universal)	
#define ippsConvert_32s16s_Sfs_x					(*ippsConvert_32s16s_Sfs_universal)	
#define ippsLShiftC_32s_I_x							(*ippsLShiftC_32s_I_universal)		
#define ippsRShiftC_32s_I_x							(*ippsRShiftC_32s_I_universal)		
 /* the following 4 functions used in MDCT*/		
#define ippsFFTInitAlloc_C_32sc_x					(*ippsFFTInitAlloc_C_32sc_universal)
#define ippsFFTGetBufSize_C_32sc_x					(*ippsFFTGetBufSize_C_32sc_universal)
#define ippsFFTFree_C_32sc_x						(*ippsFFTFree_C_32sc_universal)		
#define ippsFFTFwd_CToC_32sc_Sfs_x					(*ippsFFTFwd_CToC_32sc_Sfs_universal)
 /* the following 6 functions exist in sbr code */					
#define ippsFFTGetSize_C_32sc_x						(*ippsFFTGetSize_C_32sc_universal)	
#define ippsFFTInit_C_32sc_x						(*ippsFFTInit_C_32sc_universal)		
#define ippsFFTInv_CToC_32sc_Sfs_x					(*ippsFFTInv_CToC_32sc_Sfs_universal)
#define ippsFFTGetSize_C_32s_x						(*ippsFFTGetSize_C_32s_universal)	
#define ippsFFTInit_C_32s_x							(*ippsFFTInit_C_32s_universal)		
#define ippsFFTInv_CToC_32s_Sfs_x					(*ippsFFTInv_CToC_32s_Sfs_universal)
 /*following functions are for 5.1  aac*/					
#define ippsMul_32sc_Sfs_x							(*ippsMul_32sc_Sfs_universal)		
#define ippsMul_32s32sc_Sfs_x						(*ippsMul_32s32sc_Sfs_universal)
#define ippsMulC_32s_Sfs_x							(*ippsMulC_32s_Sfs_universal)		
#define ippsSet_8u_x								(*ippsSet_8u_universal)				
 /* following functions only used in mp3*/					
#define ippsAddC_32s_ISfs_x							(*ippsAddC_32s_ISfs_universal)		
#define ippsMulC_32s_ISfs_x							(*ippsMulC_32s_ISfs_universal)		

//***
//mp4v enc only
//***
extern IppStatus (__STDCALL *ippiSAD16x16_8u32s_universal)					(const Ipp8u*  pSrc,	Ipp32s  srcStep,	const Ipp8u*  pRef,		Ipp32s  refStep,	Ipp32s* pSAD, Ipp32s mcType);
extern IppStatus (__STDCALL *ippiSAD16x8_8u32s_C1R_universal) 				(const Ipp8u  *pSrcCur, int		srcCurStep, const Ipp8u*  pSrcRef,	int		srcRefStep, Ipp32s *pDst, Ipp32s mcType);
extern IppStatus (__STDCALL *ippiSAD8x8_8u32s_C1R_universal) 				(const Ipp8u*  pSrcCur, int		srcCurStep, const Ipp8u*  pSrcRef,	int     srcRefStep, Ipp32s* pDst,Ipp32s  mcType);
extern IppStatus (__STDCALL *ippiMeanAbsDev16x16_8u32s_C1R_universal)		(const Ipp8u*  pSrc,	int		srcStep, Ipp32s* pDst);
extern IppStatus (__STDCALL *ippiQuantIntraInit_MPEG4_universal)			(const Ipp8u*  pQuantMatrix, IppiQuantIntraSpec_MPEG4* pSpec, int bitsPerPixel);
extern IppStatus (__STDCALL *ippiQuantInterInit_MPEG4_universal) 			(const Ipp8u*  pQuantMatrix, IppiQuantInterSpec_MPEG4* pSpec, int bitsPerPixel);
extern Ipp32u*	 (__STDCALL *ippsMalloc_32u_universal)			 			(int len);
extern IppStatus (__STDCALL *ippiQuantInterGetSize_MPEG4_universal) 			(int* pSpecSize);
extern IppStatus (__STDCALL *ippiQuantIntraGetSize_MPEG4_universal) 			(int* pSpecSize);
extern IppStatus (__STDCALL *ippiQuantIntra_H263_16s_C1I_universal) 			(Ipp16s* pSrcDst,   int  QP,  int*  pCountNonZero,	int   advIntraFlag,  int   modQuantFlag);
extern IppStatus (__STDCALL *ippiQuantIntra_MPEG4_16s_C1I_universal) 		(Ipp16s*  pCoeffs, const IppiQuantIntraSpec_MPEG4* pSpec, int  QP, int* pCountNonZero, int blockType);
extern IppStatus (__STDCALL *ippiDCT8x8Fwd_8u16s_C1R_universal) 				( const Ipp8u* pSrc, int srcStep, Ipp16s* pDst);
extern IppStatus (__STDCALL *ippiQuantInter_H263_16s_C1I_universal) 			(Ipp16s* pSrcDst, int QP, int* pCountNonZero, int modQuantFlag);
extern IppStatus (__STDCALL *ippiDCT8x8Fwd_16s_C1I_universal) 				(Ipp16s* pSrcDst);
extern IppStatus (__STDCALL *ippiSubSAD8x8_8u16s_C1R_universal) 				(const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, int src2Step, Ipp16s* pDst, int dstStep, Ipp32s* pSAD);
extern IppStatus (__STDCALL *ippiSubSAD8x8_8u16s_C1R_16x16_universal) 		(const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, Ipp16s* pDst, Ipp32s* pSAD);
extern IppStatus (__STDCALL *ippiSubSAD8x8_8u16s_C1R_8x16_universal) 		(const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, Ipp16s* pDst, Ipp32s* pSAD);
extern IppStatus (__STDCALL *ippiFrameFieldSAD16x16_8u32s_C1R_universal) 	(const Ipp8u* pSrc, int  srcStep,	Ipp32s*  pFrameSAD,  Ipp32s*  pFieldSAD);
extern IppStatus (__STDCALL *ippiSub8x8_8u16s_C1R_universal) 				(const Ipp8u*  pSrc1,  int src1Step, const Ipp8u*  pSrc2, int   src2Step, Ipp16s*  pDst,  int  dstStep);
extern IppStatus (__STDCALL *ippiQuantInter_MPEG4_16s_C1I_universal) 		(Ipp16s* pCoeffs, const IppiQuantInterSpec_MPEG4* pSpec, int  QP,  int* pCountNonZero);
extern IppStatus (__STDCALL *ippiDCT8x8Fwd_16s_C1R_universal) 				(const Ipp16s* pSrc, int srcStep, Ipp16s* pDst );
extern IppStatus (__STDCALL *ippsCopy_1u_universal) 							(const Ipp8u *pSrc, int srcBitOffset, Ipp8u *pDst, int dstBitOffset, int len);
extern IppStatus (__STDCALL *ippiCopy_8u_C1R_universal) 						(const Ipp8u *pSrc, int srcStep, Ipp8u *pDst, int dstStep, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiFrameFieldSAD16x16_16s32s_C1R_universal) 	(const Ipp16s* pSrc,  int  srcStep, Ipp32s* pFrameSAD, Ipp32s* pFieldSAD);
extern IppStatus (__STDCALL *ippiSub16x16_8u16s_C1R_universal) 				(const Ipp8u*  pSrc1, int src1Step, const Ipp8u*  pSrc2, int   src2Step, Ipp16s*  pDst, int  dstStep);
extern IppStatus (__STDCALL *ippiQuantInvIntra_H263_16s_C1I_universal) 		(Ipp16s* pSrcDst, int indxLastNonZero, int QP, int     advIntraFlag,int modQuantFlag);
extern IppStatus (__STDCALL *ippiSqrDiff16x16_8u32s_universal) 				(const Ipp8u*  pSrc,Ipp32s  srcStep,const Ipp8u*  pRef, Ipp32s  refStep,Ipp32s  mcType,Ipp32s* pSqrDiff);
extern IppStatus (__STDCALL *ippiSSD8x8_8u32s_C1R_universal) 				(const Ipp8u  *pSrcCur,  int srcCurStep, const Ipp8u  *pSrcRef, int srcRefStep, Ipp32s *pDst, Ipp32s mcType);
extern IppStatus (__STDCALL *ippiQuantInvInter_H263_16s_C1I_universal) 		(Ipp16s* pSrcDst, int indxLastNonZero, int QP, int modQuantFlag);
extern IppStatus (__STDCALL *ippiQuantInvInter_MPEG4_16s_C1I_universal) 		(Ipp16s*   pCoeffs, int  indxLastNonZero, const IppiQuantInvInterSpec_MPEG4* pSpec, int QP);
extern IppStatus (__STDCALL *ippiEncodeCoeffsIntra_H263_16s1u_universal) 	(Ipp16s* pQCoef, Ipp8u** ppBitStream, int* pBitOffset, int  countNonZero, int  advIntraFlag, int modQuantFlag, int scan);
extern IppStatus (__STDCALL *ippiEncodeDCIntra_H263_16s1u_universal) 		(Ipp16s  qDC, Ipp8u** ppBitStream, int* pBitOffset);
extern IppStatus (__STDCALL *ippiCountZeros8x8_16s_C1_universal) 			(Ipp16s* pSrc, Ipp32u* pCount);
extern IppStatus (__STDCALL *ippiEncodeCoeffsIntra_MPEG4_16s1u_universal) 	(const Ipp16s*  pCoeffs,  Ipp8u** ppBitStream, int* pBitOffset, int  countNonZero, int  rvlcFlag,int noDCFlag,int scan);
extern IppStatus (__STDCALL *ippiEncodeDCIntra_MPEG4_16s1u_universal) 		(Ipp16s dcCoeff, Ipp8u**  ppBitStream,int*  pBitOffset, int blockType);
extern IppStatus (__STDCALL *ippiEncodeCoeffsInter_H263_16s1u_universal) 	(Ipp16s* pQCoef, Ipp8u** ppBitStream, int*  pBitOffset, int  countNonZero, int modQuantFlag, int  scan);
extern IppStatus (__STDCALL *ippiEncodeCoeffsInter_MPEG4_16s1u_universal) 	(const Ipp16s*  pCoeffs,  Ipp8u**  ppBitStream, int* pBitOffset,int  countNonZero,int rvlcFlag,int scan);

IppStatus __STDCALL ippiSAD16x16_8u32s_c(const Ipp8u*  pSrc,  Ipp32s  srcStep, const Ipp8u*  pRef,	Ipp32s  refStep,  Ipp32s* pSAD, Ipp32s  mcType);
IppStatus __STDCALL ippiSAD16x16_8u32s_generic_arm_v6(const Ipp8u*  pSrc,  Ipp32s  srcStep, const Ipp8u*  pRef,	Ipp32s  refStep,  Ipp32s* pSAD, Ipp32s  mcType);
IppStatus __STDCALL ippiSAD16x8_8u32s_C1R_c(const Ipp8u  *pSrcCur,  int  srcCurStep,  const Ipp8u  *pSrcRef,int srcRefStep, Ipp32s *pDst, Ipp32s mcType);
IppStatus __STDCALL ippiSAD8x8_8u32s_C1R_c(	const Ipp8u*  pSrcCur, int srcCurStep, const Ipp8u*  pSrcRef, int     srcRefStep, Ipp32s* pDst,Ipp32s  mcType);
IppStatus __STDCALL ippiMeanAbsDev16x16_8u32s_C1R_c(const Ipp8u*  pSrc,  int srcStep, Ipp32s* pDst);
IppStatus __STDCALL ippiQuantIntraInit_MPEG4_c(	const Ipp8u*  pQuantMatrix, IppiQuantIntraSpec_MPEG4* pSpec, int bitsPerPixel);
IppStatus __STDCALL ippiQuantInterInit_MPEG4_c(	const Ipp8u*  pQuantMatrix, IppiQuantInterSpec_MPEG4* pSpec, int bitsPerPixel);
Ipp32u*	  __STDCALL  ippsMalloc_32u_c(int len);
IppStatus __STDCALL ippiQuantInterGetSize_MPEG4_c(int* pSpecSize);
IppStatus __STDCALL ippiQuantIntraGetSize_MPEG4_c(int* pSpecSize);
IppStatus __STDCALL ippiQuantIntra_H263_16s_C1I_c(Ipp16s* pSrcDst,   int  QP,  int*  pCountNonZero,	int   advIntraFlag,  int   modQuantFlag);
IppStatus __STDCALL ippiQuantIntra_MPEG4_16s_C1I_c(Ipp16s*  pCoeffs, const IppiQuantIntraSpec_MPEG4* pSpec, int  QP, int* pCountNonZero, int blockType);
IppStatus __STDCALL ippiDCT8x8Fwd_8u16s_C1R_c( const Ipp8u* pSrc, int srcStep, Ipp16s* pDst);
IppStatus __STDCALL ippiQuantInter_H263_16s_C1I_c(Ipp16s* pSrcDst, int QP, int* pCountNonZero, int modQuantFlag);
IppStatus __STDCALL ippiDCT8x8Fwd_16s_C1I_c(Ipp16s* pSrcDst);
IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_c(const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, int src2Step, Ipp16s* pDst, int dstStep, Ipp32s* pSAD);
IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_16x16_c(	const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, Ipp16s* pDst, Ipp32s* pSAD);
IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_8x16_c(		const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, Ipp16s* pDst, Ipp32s* pSAD);
IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_16x16_arm_v6(	const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, Ipp16s* pDst, Ipp32s* pSAD);
IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_8x16_c_arm_v6(		const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, Ipp16s* pDst, Ipp32s* pSAD);
IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_16x16(		const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, Ipp16s* pDst, Ipp32s* pSAD);
IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_8x16(		const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, Ipp16s* pDst, Ipp32s* pSAD);
IppStatus __STDCALL ippiFrameFieldSAD16x16_8u32s_C1R_c(const Ipp8u* pSrc, int  srcStep,	Ipp32s*  pFrameSAD,  Ipp32s*  pFieldSAD);
IppStatus __STDCALL ippiSub8x8_8u16s_C1R_c(	const Ipp8u*  pSrc1,  int src1Step, const Ipp8u*  pSrc2, int   src2Step, Ipp16s*  pDst,  int  dstStep);
IppStatus __STDCALL ippiQuantInter_MPEG4_16s_C1I_c(	Ipp16s* pCoeffs, const IppiQuantInterSpec_MPEG4* pSpec, int  QP,  int* pCountNonZero);
IppStatus __STDCALL ippiDCT8x8Fwd_16s_C1R_c( const Ipp16s* pSrc, int srcStep, Ipp16s* pDst );
IppStatus __STDCALL ippsCopy_1u_c(const Ipp8u *pSrc, int srcBitOffset, Ipp8u *pDst, int dstBitOffset, int len);
IppStatus __STDCALL ippiCopy_8u_C1R_c(const Ipp8u *pSrc, int srcStep, Ipp8u *pDst, int dstStep, IppiSize roiSize);
IppStatus __STDCALL ippiFrameFieldSAD16x16_16s32s_C1R_c(const Ipp16s* pSrc,  int  srcStep, Ipp32s* pFrameSAD, Ipp32s* pFieldSAD);
IppStatus __STDCALL ippiSub16x16_8u16s_C1R_c(const Ipp8u*  pSrc1, int src1Step, const Ipp8u*  pSrc2, int   src2Step, Ipp16s*  pDst, int  dstStep);
IppStatus __STDCALL ippiQuantInvIntra_H263_16s_C1I_c(Ipp16s* pSrcDst, int indxLastNonZero, int QP, int     advIntraFlag,int modQuantFlag);
IppStatus __STDCALL ippiSqrDiff16x16_8u32s_c(const Ipp8u*  pSrc,Ipp32s  srcStep,const Ipp8u*  pRef, Ipp32s  refStep,Ipp32s  mcType,Ipp32s* pSqrDiff);
IppStatus __STDCALL ippiSSD8x8_8u32s_C1R_c(	const Ipp8u  *pSrcCur,  int srcCurStep, const Ipp8u  *pSrcRef, int srcRefStep, Ipp32s *pDst, Ipp32s mcType);
IppStatus __STDCALL ippiQuantInvInter_H263_16s_C1I_c(Ipp16s* pSrcDst, int indxLastNonZero, int QP, int modQuantFlag);
IppStatus __STDCALL ippiQuantInvInter_MPEG4_16s_C1I_c( Ipp16s*   pCoeffs, int  indxLastNonZero, const IppiQuantInvInterSpec_MPEG4* pSpec, int QP);
IppStatus __STDCALL ippiEncodeCoeffsIntra_H263_16s1u_c( Ipp16s* pQCoef, Ipp8u** ppBitStream, int* pBitOffset, int  countNonZero, int  advIntraFlag, int modQuantFlag, int scan);
IppStatus __STDCALL ippiEncodeDCIntra_H263_16s1u_c(Ipp16s  qDC, Ipp8u** ppBitStream, int* pBitOffset);
IppStatus __STDCALL ippiCountZeros8x8_16s_C1_c(Ipp16s* pSrc, Ipp32u* pCount);
IppStatus __STDCALL ippiEncodeCoeffsIntra_MPEG4_16s1u_c(const Ipp16s*  pCoeffs,  Ipp8u** ppBitStream, int* pBitOffset, int  countNonZero, int  rvlcFlag,int noDCFlag,int scan);
IppStatus __STDCALL ippiEncodeDCIntra_MPEG4_16s1u_c(Ipp16s dcCoeff, Ipp8u**  ppBitStream,int*  pBitOffset, int blockType);
IppStatus __STDCALL ippiEncodeCoeffsInter_H263_16s1u_c(Ipp16s* pQCoef, Ipp8u** ppBitStream, int*  pBitOffset, int  countNonZero, int modQuantFlag, int  scan);
IppStatus __STDCALL ippiEncodeCoeffsInter_MPEG4_16s1u_c(const Ipp16s*  pCoeffs,  Ipp8u**  ppBitStream, int* pBitOffset,int  countNonZero,int rvlcFlag,int scan);

IppStatus __STDCALL ippiQuantIntra_H263_16s_C1I_flv_c(Ipp16s* pSrcDst,   int  QP,  int*  pCountNonZero,	int   advIntraFlag,  int   modQuantFlag);
IppStatus __STDCALL ippiEncodeCoeffsIntra_H263_16s1u_flv_c(Ipp16s* pQCoef, Ipp8u** ppBitStream, int* pBitOffset, int  countNonZero, int  advIntraFlag, int modQuantFlag, int scan);
IppStatus __STDCALL ippiEncodeCoeffsInter_H263_16s1u_flv_c(Ipp16s* pQCoef, Ipp8u** ppBitStream, int* pBitOffset, int  countNonZero, int modQuantFlag, int  scan);

#define ippiSAD16x16_8u32s_x							(*ippiSAD16x16_8u32s_universal)	
#define ippiSAD16x8_8u32s_C1R_x						(*ippiSAD16x8_8u32s_C1R_universal)
#define ippiSAD8x8_8u32s_C1R_x						(*ippiSAD8x8_8u32s_C1R_universal)
#define ippiMeanAbsDev16x16_8u32s_C1R_x				(*ippiMeanAbsDev16x16_8u32s_C1R_universal)
#define ippiQuantIntraInit_MPEG4_x					(*ippiQuantIntraInit_MPEG4_universal)
#define ippiQuantInterInit_MPEG4_x					(*ippiQuantInterInit_MPEG4_universal)
#define ippsMalloc_32u_x								(*ippsMalloc_32u_universal)
#define ippiQuantInterGetSize_MPEG4_x				(*ippiQuantInterGetSize_MPEG4_universal)
#define ippiQuantIntraGetSize_MPEG4_x				(*ippiQuantIntraGetSize_MPEG4_universal)
#define ippiQuantIntra_H263_16s_C1I_x				(*ippiQuantIntra_H263_16s_C1I_universal)
#define ippiQuantIntra_MPEG4_16s_C1I_x				(*ippiQuantIntra_MPEG4_16s_C1I_universal)
#define ippiDCT8x8Fwd_8u16s_C1R_x					(*ippiDCT8x8Fwd_8u16s_C1R_universal)
#define ippiQuantInter_H263_16s_C1I_x				(*ippiQuantInter_H263_16s_C1I_universal)
#define ippiDCT8x8Fwd_16s_C1I_x						(*ippiDCT8x8Fwd_16s_C1I_universal)
#define ippiSubSAD8x8_8u16s_C1R_x					(*ippiSubSAD8x8_8u16s_C1R_universal)
#define ippiSubSAD8x8_8u16s_C1R_16x16_x				(*ippiSubSAD8x8_8u16s_C1R_16x16_universal)
#define ippiSubSAD8x8_8u16s_C1R_8x16_x				(*ippiSubSAD8x8_8u16s_C1R_8x16_universal)
#define ippiFrameFieldSAD16x16_8u32s_C1R_x			(*ippiFrameFieldSAD16x16_8u32s_C1R_universal)
#define ippiSub8x8_8u16s_C1R_x						(*ippiSub8x8_8u16s_C1R_universal)
#define ippiQuantInter_MPEG4_16s_C1I_x				(*ippiQuantInter_MPEG4_16s_C1I_universal)
#define ippiDCT8x8Fwd_16s_C1R_x						(*ippiDCT8x8Fwd_16s_C1R_universal)
#define ippsCopy_1u_x								(*ippsCopy_1u_universal)
#define ippiCopy_8u_C1R_x							(*ippiCopy_8u_C1R_universal)
#define ippiFrameFieldSAD16x16_16s32s_C1R_x			(*ippiFrameFieldSAD16x16_16s32s_C1R_universal)
#define ippiSub16x16_8u16s_C1R_x						(*ippiSub16x16_8u16s_C1R_universal)
#define ippiQuantInvIntra_H263_16s_C1I_x				(*ippiQuantInvIntra_H263_16s_C1I_universal)
#define ippiSqrDiff16x16_8u32s_x						(*ippiSqrDiff16x16_8u32s_universal)
#define ippiSSD8x8_8u32s_C1R_x						(*ippiSSD8x8_8u32s_C1R_universal)
#define ippiQuantInvInter_H263_16s_C1I_x				(*ippiQuantInvInter_H263_16s_C1I_universal)
#define ippiQuantInvInter_MPEG4_16s_C1I_x			(*ippiQuantInvInter_MPEG4_16s_C1I_universal)
#define ippiEncodeCoeffsIntra_H263_16s1u_x			(*ippiEncodeCoeffsIntra_H263_16s1u_universal)
#define ippiEncodeDCIntra_H263_16s1u_x				(*ippiEncodeDCIntra_H263_16s1u_universal)
#define ippiCountZeros8x8_16s_C1_x					(*ippiCountZeros8x8_16s_C1_universal)
#define ippiEncodeCoeffsIntra_MPEG4_16s1u_x			(*ippiEncodeCoeffsIntra_MPEG4_16s1u_universal)
#define ippiEncodeDCIntra_MPEG4_16s1u_x				(*ippiEncodeDCIntra_MPEG4_16s1u_universal)
#define ippiEncodeCoeffsInter_H263_16s1u_x			(*ippiEncodeCoeffsInter_H263_16s1u_universal)
#define ippiEncodeCoeffsInter_MPEG4_16s1u_x			(*ippiEncodeCoeffsInter_MPEG4_16s1u_universal)
													


//***
//avc dec
//***
extern IppStatus (__STDCALL *ippiSet_8u_C1R_universal) 										(Ipp8u value, Ipp8u* pDst, int dstStep, IppiSize roiSize );

extern IppStatus (__STDCALL *ippiDecodeExpGolombOne_H264_1u16s_universal)						(Ipp32u **ppBitStream, Ipp32s *pBitOffset,Ipp16s *pDst,Ipp8u isSigned);//***
extern Ipp32s    (__STDCALL *ippiDecodeExpGolombOne_H264_1u16s_signed_universal)				(Ipp32u **ppBitStream, Ipp32s *pBitOffset);//***
extern Ipp32s    (__STDCALL *ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal)				(Ipp32u **ppBitStream, Ipp32s *pBitOffset);//***
extern IppStatus (__STDCALL *ippiDecodeCAVLCCoeffs_H264_1u16s_universal)						(Ipp32u **ppBitStream,  Ipp32s *pOffset,   Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs,Ipp32u uVLCSelect,Ipp16s uMaxNumCoeff, const Ipp32s **ppTblCoeffToken, const Ipp32s **ppTblTotalZeros,const Ipp32s **ppTblRunBefore,  const Ipp32s *pScanMatrix);//***
extern IppStatus (__STDCALL *ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_universal)				(Ipp32u **ppBitStream, Ipp32s *pOffset,Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs, const Ipp32s *pTblCoeffToken, const Ipp32s **ppTblTotalZerosCR,   const Ipp32s **ppTblRunBefore);//***
extern IppStatus (__STDCALL *ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal)			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
extern IppStatus (__STDCALL *ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_universal)		(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp32u    nAlpha,  const Ipp32u    nBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
extern IppStatus (__STDCALL *ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal)			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
extern IppStatus (__STDCALL *ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal)			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
extern IppStatus (__STDCALL *ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal)			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
extern IppStatus (__STDCALL *ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_universal)	(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp32u    nAlpha,  const Ipp32u    nBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);

//dummy
extern IppStatus (__STDCALL *ippiHuffmanRunLevelTableInitAlloc_32s_universal)					(const Ipp32s*    pSrcTable,     IppVCHuffmanSpec_32s** ppDstSpec);
extern IppStatus (__STDCALL *ippiHuffmanTableFree_32s_universal)								(IppVCHuffmanSpec_32s *pDecodeTable);
extern IppStatus (__STDCALL *ippiHuffmanTableInitAlloc_32s_universal)							(const Ipp32s*    pSrcTable,       IppVCHuffmanSpec_32s** ppDstSpec);


extern IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_16x16_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_16x8_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_8x16_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_8x8_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_8x4_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_4x8_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_4x4_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
//extern IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_0_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_universal)							(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateLumaTop_H264_8u_C1R_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateLumaBottom_H264_8u_C1R_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateChroma_H264_8u_C1R_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateChromaTop_H264_8u_C1R_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateChromaBottom_H264_8u_C1R_universal)					(const Ipp8u* pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
extern IppStatus (__STDCALL *ippiInterpolateBlock_H264_8u_P2P1R_universal)						(const Ipp8u *pSrc1,  const Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u uWidth,Ipp32u uHeight,Ipp32s pitch);
extern IppStatus (__STDCALL *ippiInterpolateBlock_H264_8u_P3P1R_universal)						(const Ipp8u *pSrc1,  const Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u uWidth,Ipp32u uHeight,Ipp32s iPitchSrc1,Ipp32s iPitchSrc2,Ipp32s iPitchDst);
extern IppStatus (__STDCALL *ippiUniDirWeightBlock_H264_8u_C1R_universal)						(Ipp8u *pSrcDst,Ipp32u pitch, Ipp32u ulog2wd, Ipp32s iWeight,Ipp32s iOffset,IppiSize roi);
extern IppStatus (__STDCALL *ippiBiDirWeightBlock_H264_8u_P2P1R_universal)						(Ipp8u *pSrc1,  Ipp8u *pSrc2, Ipp8u *pDst,     Ipp32u pitch,  Ipp32u dstStep, Ipp32u ulog2wd, Ipp32s iWeight1, Ipp32s iOffset1, Ipp32s iWeight2,Ipp32s iOffset2,   IppiSize roi);
extern IppStatus (__STDCALL *ippiBiDirWeightBlock_H264_8u_P3P1R_universal)						(const Ipp8u *pSrc1,const Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u nSrcPitch1, Ipp32u nSrcPitch2,Ipp32u nDstPitch,Ipp32u ulog2wd,Ipp32s iWeight1,Ipp32s iOffset1,Ipp32s iWeight2,Ipp32s iOffset2,IppiSize roi);
extern IppStatus (__STDCALL *ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_universal)				(const Ipp8u *pSrc1,const Ipp8u *pSrc2,Ipp8u *pDst,Ipp32u nSrcPitch1, Ipp32u nSrcPitch2,  Ipp32u nDstPitch,  Ipp32s iWeight1,Ipp32s iWeight2,  IppiSize roi);
extern IppStatus (__STDCALL *ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_universal)				(Ipp8u *pSrc1,  Ipp8u *pSrc2,   Ipp8u *pDst,    Ipp32u pitch,  Ipp32u dstpitch,   Ipp32s iWeight1,  Ipp32s iWeight2,  IppiSize roi);

extern IppStatus (__STDCALL *ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane,Ipp32u srcdstYStep,const IppIntra16x16PredMode_H264 intra_luma_mode,const Ipp32u cbp4x4, const Ipp32u QP,const Ipp8u edge_type);
extern IppStatus (__STDCALL *ippiReconstructChromaIntraMB_H264_16s8u_P2R_universal)				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, const Ipp32u srcdstUVStep, const IppIntraChromaPredMode_H264 intra_chroma_mode, const Ipp32u cbp4x4, const Ipp32u ChromaQP, const Ipp8u edge_type);
extern IppStatus (__STDCALL *ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_universal)		(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, Ipp32u srcdstUVStep, IppIntraChromaPredMode_H264 intra_chroma_mode, Ipp32u cbp4x4, Ipp32u ChromaQP, Ipp8u edge_type_top, Ipp8u edge_type_bottom);
extern IppStatus (__STDCALL *ippiReconstructLumaIntraMB_H264_16s8u_C1R_universal)				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, const IppIntra4x4PredMode_H264 *pMBIntraTypes, const Ipp32u cbp4x4, const Ipp32u QP, const Ipp8u edge_type);
extern IppStatus (__STDCALL *ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x2, Ipp32u QP, Ipp8u edge_type);
extern IppStatus (__STDCALL *ippiReconstructLumaInterMB_H264_16s8u_C1R_universal)				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp4x4, Ipp32s QP);
extern IppStatus (__STDCALL *ippiReconstructChromaInterMB_H264_16s8u_P2R_universal)				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, const Ipp32u srcdstUVStep, const Ipp32u cbp4x4, const Ipp32u ChromaQP);
extern IppStatus (__STDCALL *ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, const IppIntra16x16PredMode_H264 intra_luma_mode, const Ipp32u cbp4x4, const Ipp32u QP, const Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag /*Resevr ONLY : do not support at present */);
extern IppStatus (__STDCALL *ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra8x8PredMode_H264 *pMBIntraTypes, Ipp32u cbp8x8, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
extern IppStatus (__STDCALL *ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_universal)		(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra8x8PredMode_H264 *pMBIntraTypes, Ipp32u cbp8x2, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
extern IppStatus (__STDCALL *ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp8x8, Ipp32s QP, Ipp16s *pQuantTable, Ipp8u bypass_flag);
extern IppStatus (__STDCALL *ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x4, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
extern IppStatus (__STDCALL *ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_universal)		(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x2, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
extern IppStatus (__STDCALL *ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp4x4, Ipp32s QP, Ipp16s *pQuantTable, Ipp8u bypass_flag);
extern IppStatus (__STDCALL *ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_universal)			(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep, IppIntraChromaPredMode_H264 intra_chroma_mode, Ipp32u cbp4x4, Ipp32u ChromaQPU, Ipp32u ChromaQPV, Ipp8u edge_type, Ipp16s *pQuantTableU, Ipp16s *pQuantTableV, Ipp8u bypass_flag);
extern IppStatus (__STDCALL *ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_universal)		(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep,IppIntraChromaPredMode_H264 intra_chroma_mode,Ipp32u cbp4x4,Ipp32u ChromaQPU,Ipp32u ChromaQPV,Ipp8u edge_type_top,Ipp8u edge_type_bottom,Ipp16s *pQuantTableU,Ipp16s *pQuantTableV,Ipp8u bypass_flag);
extern IppStatus (__STDCALL *ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_universal)			(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep,Ipp32u cbp4x4,Ipp32u ChromaQPU,Ipp32u ChromaQPV,Ipp16s *pQuantTableU,Ipp16s *pQuantTableV,Ipp8u bypass_flag);

IppStatus __STDCALL ippiSet_8u_C1R_c 										(Ipp8u value, Ipp8u* pDst, int dstStep, IppiSize roiSize );

//IppStatus __STDCALL ippiDecodeExpGolombOne_H264_1u16s_c						(Ipp32u **ppBitStream, Ipp32s *pBitOffset,Ipp16s *pDst,Ipp8u isSigned);//***
Ipp32s    __STDCALL ippiDecodeExpGolombOne_H264_1u16s_signed_c				(Ipp32u **ppBitStream, Ipp32s *pBitOffset);//***
Ipp32s	  __STDCALL ippiDecodeExpGolombOne_H264_1u16s_unsigned_c			(Ipp32u **ppBitStream, Ipp32s *pBitOffset);//***
Ipp32s    __STDCALL ippiDecodeExpGolombOne_H264_1u16s_signed_ipp			(Ipp32u **ppBitStream, Ipp32s *pBitOffset);//***
Ipp32s    __STDCALL ippiDecodeExpGolombOne_H264_1u16s_unsigned_ipp			(Ipp32u **ppBitStream, Ipp32s *pBitOffset);//***
Ipp32s    __STDCALL ippiDecodeExpGolombOne_H264_1u16s_signed_arm			(Ipp32u **ppBitStream, Ipp32s *pBitOffset);//***
Ipp32s    __STDCALL ippiDecodeExpGolombOne_H264_1u16s_unsigned_arm			(Ipp32u **ppBitStream, Ipp32s *pBitOffset);//***

IppStatus __STDCALL ippiDecodeCAVLCCoeffs_H264_1u16s_c_only					(Ipp32u **ppBitStream,  Ipp32s *pOffset,   Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs,Ipp32u uVLCSelect,Ipp16s uMaxNumCoeff, const Ipp32s **ppTblCoeffToken, const Ipp32s **ppTblTotalZeros,const Ipp32s **ppTblRunBefore,  const Ipp32s *pScanMatrix);//***
IppStatus __STDCALL ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_only			(Ipp32u **ppBitStream, Ipp32s *pOffset,Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs, const Ipp32s *pTblCoeffToken, const Ipp32s **ppTblTotalZerosCR,   const Ipp32s **ppTblRunBefore);//***

IppStatus __STDCALL ippiDecodeCAVLCCoeffs_H264_1u16s_c						(Ipp32u **ppBitStream,  Ipp32s *pOffset,   Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs,Ipp32u uVLCSelect,Ipp16s uMaxNumCoeff, const Ipp32s **ppTblCoeffToken, const Ipp32s **ppTblTotalZeros,const Ipp32s **ppTblRunBefore,  const Ipp32s *pScanMatrix);//***
IppStatus __STDCALL ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c				(Ipp32u **ppBitStream, Ipp32s *pOffset,Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs, const Ipp32s *pTblCoeffToken, const Ipp32s **ppTblTotalZerosCR,   const Ipp32s **ppTblRunBefore);//***
IppStatus __STDCALL ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
IppStatus __STDCALL ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_c	(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp32u    nAlpha,  const Ipp32u    nBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
IppStatus __STDCALL ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
IppStatus __STDCALL ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c		(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
IppStatus __STDCALL ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c		(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
IppStatus __STDCALL ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_c	(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp32u    nAlpha,  const Ipp32u    nBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);

//WWD-200711
IppStatus __STDCALL ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_simple		(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
IppStatus __STDCALL ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_simple		(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);


IppStatus __STDCALL ippiHuffmanRunLevelTableInitAlloc_32s_c					(const Ipp32s*    pSrcTable,     IppVCHuffmanSpec_32s** ppDstSpec);
IppStatus __STDCALL ippiHuffmanTableFree_32s_c								(IppVCHuffmanSpec_32s *pDecodeTable);
IppStatus __STDCALL ippiHuffmanTableInitAlloc_32s_c							(const Ipp32s*    pSrcTable,       IppVCHuffmanSpec_32s** ppDstSpec);

IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_16x16_c					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_16x8_c					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_8x16_c					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_8x8_c					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_8x4_c					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_4x8_c					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_4x4_c					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_c						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateLumaTop_H264_8u_C1R_c					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateLumaBottom_H264_8u_C1R_c					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateChroma_H264_8u_C1R_c						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateChromaTop_H264_8u_C1R_c					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateChromaBottom_H264_8u_C1R_c				(const Ipp8u* pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateBlock_H264_8u_P2P1R_c					(const Ipp8u *pSrc1,  const Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u uWidth,Ipp32u uHeight,Ipp32s pitch);
IppStatus __STDCALL ippiInterpolateBlock_H264_8u_P3P1R_c					(const Ipp8u *pSrc1,  const Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u uWidth,Ipp32u uHeight,Ipp32s iPitchSrc1,Ipp32s iPitchSrc2,Ipp32s iPitchDst);
IppStatus __STDCALL ippiUniDirWeightBlock_H264_8u_C1R_MSc						(Ipp8u *pSrcDst,Ipp32u pitch, Ipp32u ulog2wd, Ipp32s iWeight,Ipp32s iOffset,IppiSize roi);
IppStatus __STDCALL ippiBiDirWeightBlock_H264_8u_P2P1R_MSc					(Ipp8u *pSrc1,  Ipp8u *pSrc2, Ipp8u *pDst,     Ipp32u pitch,  Ipp32u dstStep, Ipp32u ulog2wd, Ipp32s iWeight1, Ipp32s iOffset1, Ipp32s iWeight2,Ipp32s iOffset2,   IppiSize roi);
IppStatus __STDCALL ippiBiDirWeightBlock_H264_8u_P3P1R_MSc					(const Ipp8u *pSrc1,const Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u nSrcPitch1, Ipp32u nSrcPitch2,Ipp32u nDstPitch,Ipp32u ulog2wd,Ipp32s iWeight1,Ipp32s iOffset1,Ipp32s iWeight2,Ipp32s iOffset2,IppiSize roi);
IppStatus __STDCALL ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_c			(const Ipp8u *pSrc1,const Ipp8u *pSrc2,Ipp8u *pDst,Ipp32u nSrcPitch1, Ipp32u nSrcPitch2,  Ipp32u nDstPitch,  Ipp32s iWeight1,Ipp32s iWeight2,  IppiSize roi);
IppStatus __STDCALL ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_c			(Ipp8u *pSrc1,  Ipp8u *pSrc2,   Ipp8u *pDst,    Ipp32u pitch,  Ipp32u dstpitch,   Ipp32s iWeight1,  Ipp32s iWeight2,  IppiSize roi);

IppStatus __STDCALL ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c		(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane,Ipp32u srcdstYStep,const IppIntra16x16PredMode_H264 intra_luma_mode,const Ipp32u cbp4x4, const Ipp32u QP,const Ipp8u edge_type);
IppStatus __STDCALL ippiReconstructChromaIntraMB_H264_16s8u_P2R_c			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, const Ipp32u srcdstUVStep, const IppIntraChromaPredMode_H264 intra_chroma_mode, const Ipp32u cbp4x4, const Ipp32u ChromaQP, const Ipp8u edge_type);
IppStatus __STDCALL ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_c		(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, Ipp32u srcdstUVStep, IppIntraChromaPredMode_H264 intra_chroma_mode, Ipp32u cbp4x4, Ipp32u ChromaQP, Ipp8u edge_type_top, Ipp8u edge_type_bottom);
IppStatus __STDCALL ippiReconstructLumaIntraMB_H264_16s8u_C1R_c				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, const IppIntra4x4PredMode_H264 *pMBIntraTypes, const Ipp32u cbp4x4, const Ipp32u QP, const Ipp8u edge_type);
IppStatus __STDCALL ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_c			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x2, Ipp32u QP, Ipp8u edge_type);
IppStatus __STDCALL ippiReconstructLumaInterMB_H264_16s8u_C1R_c				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp4x4, Ipp32s QP);
IppStatus __STDCALL ippiReconstructChromaInterMB_H264_16s8u_P2R_c			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, const Ipp32u srcdstUVStep, const Ipp32u cbp4x4, const Ipp32u ChromaQP);
IppStatus __STDCALL ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_c		(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, const IppIntra16x16PredMode_H264 intra_luma_mode, const Ipp32u cbp4x4, const Ipp32u QP, const Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag /*Resevr ONLY : do not support at present */);
IppStatus __STDCALL ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_c			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra8x8PredMode_H264 *pMBIntraTypes, Ipp32u cbp8x8, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_c		(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra8x8PredMode_H264 *pMBIntraTypes, Ipp32u cbp8x2, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_c			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp8x8, Ipp32s QP, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_c			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x4, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_c		(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x2, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_c			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp4x4, Ipp32s QP, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_c		(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep, IppIntraChromaPredMode_H264 intra_chroma_mode, Ipp32u cbp4x4, Ipp32u ChromaQPU, Ipp32u ChromaQPV, Ipp8u edge_type, Ipp16s *pQuantTableU, Ipp16s *pQuantTableV, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_c	(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep,IppIntraChromaPredMode_H264 intra_chroma_mode,Ipp32u cbp4x4,Ipp32u ChromaQPU,Ipp32u ChromaQPV,Ipp8u edge_type_top,Ipp8u edge_type_bottom,Ipp16s *pQuantTableU,Ipp16s *pQuantTableV,Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_c		(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep,Ipp32u cbp4x4,Ipp32u ChromaQPU,Ipp32u ChromaQPV,Ipp16s *pQuantTableU,Ipp16s *pQuantTableV,Ipp8u bypass_flag);



// + declare arm asm functions for AVC decoder
Ipp8u*    __STDCALL  ippsMalloc_8u_arm		(int len);
void	  __STDCALL ippsFree_arm			(void* ptr);
IppStatus __STDCALL ippsZero_8u_arm			(Ipp8u* pDst, int len);
IppStatus __STDCALL ippsZero_16u_arm		(Ipp16s* pDst, int len);
IppStatus __STDCALL ippsZero_32u_arm		(Ipp32s* pDst, int len);
IppStatus __STDCALL ippiSet_8u_C1R_arm 		(Ipp8u value, Ipp8u* pDst, int dstStep, IppiSize roiSize );
IppStatus __STDCALL ippsSet_8u_arm			(Ipp8u val, Ipp8u* pDst, int len);

IppStatus __STDCALL ippiDecodeCAVLCCoeffs_H264_1u16s_arm						(Ipp32u **ppBitStream,  Ipp32s *pOffset,   Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs,Ipp32u uVLCSelect,Ipp16s uMaxNumCoeff, const Ipp32s **ppTblCoeffToken, const Ipp32s **ppTblTotalZeros,const Ipp32s **ppTblRunBefore,  const Ipp32s *pScanMatrix);
IppStatus __STDCALL ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_arm				(Ipp32u **ppBitStream, Ipp32s *pOffset,Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs, const Ipp32s *pTblCoeffToken, const Ipp32s **ppTblTotalZerosCR,   const Ipp32s **ppTblRunBefore);
IppStatus __STDCALL ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_arm			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
IppStatus __STDCALL ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_arm		(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp32u    nAlpha,  const Ipp32u    nBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
IppStatus __STDCALL ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_arm			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
IppStatus __STDCALL ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_arm			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
IppStatus __STDCALL ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_arm			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);
IppStatus __STDCALL ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_arm	(Ipp8u*	   pSrcDst, Ipp32s    srcdstStep, const Ipp32u    nAlpha,  const Ipp32u    nBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs);


IppStatus __STDCALL ippiHuffmanRunLevelTableInitAlloc_32s_arm					(const Ipp32s*    pSrcTable,     IppVCHuffmanSpec_32s** ppDstSpec);
IppStatus __STDCALL ippiHuffmanTableFree_32s_arm								(IppVCHuffmanSpec_32s *pDecodeTable);
IppStatus __STDCALL ippiHuffmanTableInitAlloc_32s_arm							(const Ipp32s*    pSrcTable,       IppVCHuffmanSpec_32s** ppDstSpec);

IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_arm						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateLumaTop_H264_8u_C1R_arm					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateLumaBottom_H264_8u_C1R_arm					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateChroma_H264_8u_C1R_arm						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateChromaTop_H264_8u_C1R_arm					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateChromaBottom_H264_8u_C1R_arm				(const Ipp8u* pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize);
IppStatus __STDCALL ippiInterpolateBlock_H264_8u_P2P1R_arm					(Ipp8u *pSrc1,  Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u uWidth,Ipp32u uHeight,Ipp32s pitch);
IppStatus __STDCALL ippiInterpolateBlock_H264_8u_P3P1R_arm					(Ipp8u *pSrc1,  Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u uWidth,Ipp32u uHeight,Ipp32s iPitchSrc1,Ipp32s iPitchSrc2,Ipp32s iPitchDst);
IppStatus __STDCALL ippiUniDirWeightBlock_H264_8u_C1R_arm						(Ipp8u *pSrcDst,Ipp32u pitch, Ipp32u ulog2wd, Ipp32s iWeight,Ipp32s iOffset,IppiSize roi);
IppStatus __STDCALL ippiBiDirWeightBlock_H264_8u_P2P1R_arm					(Ipp8u *pSrc1,  Ipp8u *pSrc2, Ipp8u *pDst,     Ipp32u pitch,  Ipp32u dstStep, Ipp32u ulog2wd, Ipp32s iWeight1, Ipp32s iOffset1, Ipp32s iWeight2,Ipp32s iOffset2,   IppiSize roi);
IppStatus __STDCALL ippiBiDirWeightBlock_H264_8u_P3P1R_arm					(const Ipp8u *pSrc1,const Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u nSrcPitch1, Ipp32u nSrcPitch2,Ipp32u nDstPitch,Ipp32u ulog2wd,Ipp32s iWeight1,Ipp32s iOffset1,Ipp32s iWeight2,Ipp32s iOffset2,IppiSize roi);
IppStatus __STDCALL ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_arm			(const Ipp8u *pSrc1,const Ipp8u *pSrc2,Ipp8u *pDst,Ipp32u nSrcPitch1, Ipp32u nSrcPitch2,  Ipp32u nDstPitch,  Ipp32s iWeight1,Ipp32s iWeight2,  IppiSize roi);
IppStatus __STDCALL ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_arm			(Ipp8u *pSrc1,  Ipp8u *pSrc2,   Ipp8u *pDst,    Ipp32u pitch,  Ipp32u dstpitch,   Ipp32s iWeight1,  Ipp32s iWeight2,  IppiSize roi);

IppStatus __STDCALL ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_arm		(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane,Ipp32u srcdstYStep,const IppIntra16x16PredMode_H264 intra_luma_mode,const Ipp32u cbp4x4, const Ipp32u QP,const Ipp8u edge_type);
IppStatus __STDCALL ippiReconstructChromaIntraMB_H264_16s8u_P2R_arm			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, const Ipp32u srcdstUVStep, const IppIntraChromaPredMode_H264 intra_chroma_mode, const Ipp32u cbp4x4, const Ipp32u ChromaQP, const Ipp8u edge_type);
IppStatus __STDCALL ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_arm		(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, Ipp32u srcdstUVStep, IppIntraChromaPredMode_H264 intra_chroma_mode, Ipp32u cbp4x4, Ipp32u ChromaQP, Ipp8u edge_type_top, Ipp8u edge_type_bottom);
IppStatus __STDCALL ippiReconstructLumaIntraMB_H264_16s8u_C1R_arm				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, const IppIntra4x4PredMode_H264 *pMBIntraTypes, const Ipp32u cbp4x4, const Ipp32u QP, const Ipp8u edge_type);
IppStatus __STDCALL ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_arm			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x2, Ipp32u QP, Ipp8u edge_type);
IppStatus __STDCALL ippiReconstructLumaInterMB_H264_16s8u_C1R_arm				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp4x4, Ipp32s QP);
IppStatus __STDCALL ippiReconstructChromaInterMB_H264_16s8u_P2R_arm			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, const Ipp32u srcdstUVStep, const Ipp32u cbp4x4, const Ipp32u ChromaQP);
IppStatus __STDCALL ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_arm		(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, const IppIntra16x16PredMode_H264 intra_luma_mode, const Ipp32u cbp4x4, const Ipp32u QP, const Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag /*Resevr ONLY : do not support at present */);
IppStatus __STDCALL ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_arm			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra8x8PredMode_H264 *pMBIntraTypes, Ipp32u cbp8x8, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_arm		(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra8x8PredMode_H264 *pMBIntraTypes, Ipp32u cbp8x2, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_arm			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp8x8, Ipp32s QP, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_arm			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x4, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_arm		(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x2, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_arm			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp4x4, Ipp32s QP, Ipp16s *pQuantTable, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_arm		(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep, IppIntraChromaPredMode_H264 intra_chroma_mode, Ipp32u cbp4x4, Ipp32u ChromaQPU, Ipp32u ChromaQPV, Ipp8u edge_type, Ipp16s *pQuantTableU, Ipp16s *pQuantTableV, Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_arm	(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep,IppIntraChromaPredMode_H264 intra_chroma_mode,Ipp32u cbp4x4,Ipp32u ChromaQPU,Ipp32u ChromaQPV,Ipp8u edge_type_top,Ipp8u edge_type_bottom,Ipp16s *pQuantTableU,Ipp16s *pQuantTableV,Ipp8u bypass_flag);
IppStatus __STDCALL ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_arm		(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep,Ipp32u cbp4x4,Ipp32u ChromaQPU,Ipp32u ChromaQPV,Ipp16s *pQuantTableU,Ipp16s *pQuantTableV,Ipp8u bypass_flag);



#define	ippiSet_8u_C1R_x											(*ippiSet_8u_C1R_universal)

#define ippiDecodeExpGolombOne_H264_1u16s_x							(*ippiDecodeExpGolombOne_H264_1u16s_universal)
#define ippiDecodeExpGolombOne_H264_1u16s_signed_x					(*ippiDecodeExpGolombOne_H264_1u16s_signed_universal)
#define ippiDecodeExpGolombOne_H264_1u16s_unsigned_x				(*ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal)
#define ippiDecodeCAVLCCoeffs_H264_1u16s_x							(*ippiDecodeCAVLCCoeffs_H264_1u16s_universal)
#define ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_x					(*ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_universal)
#define ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_x				(*ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal)
#define ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_x		(*ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_universal)
#define ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_x				(*ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal)
#define ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_x			(*ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal)
#define ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_x			(*ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal)
#define ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_x		(*ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_universal)

//dummy
#define ippiHuffmanRunLevelTableInitAlloc_32s_x						(*ippiHuffmanRunLevelTableInitAlloc_32s_universal)
#define ippiHuffmanTableFree_32s_x									(*ippiHuffmanTableFree_32s_universal)
#define ippiHuffmanTableInitAlloc_32s_x								(*ippiHuffmanTableInitAlloc_32s_universal)

#define ippiInterpolateLuma_H264_8u_C1R_16x16_x						(*ippiInterpolateLuma_H264_8u_C1R_16x16_universal)
#define ippiInterpolateLuma_H264_8u_C1R_16x8_x						(*ippiInterpolateLuma_H264_8u_C1R_16x8_universal)
#define ippiInterpolateLuma_H264_8u_C1R_8x16_x						(*ippiInterpolateLuma_H264_8u_C1R_8x16_universal)
#define ippiInterpolateLuma_H264_8u_C1R_8x8_x						(*ippiInterpolateLuma_H264_8u_C1R_8x8_universal)
#define ippiInterpolateLuma_H264_8u_C1R_8x4_x						(*ippiInterpolateLuma_H264_8u_C1R_8x4_universal)
#define ippiInterpolateLuma_H264_8u_C1R_4x8_x						(*ippiInterpolateLuma_H264_8u_C1R_4x8_universal)
#define ippiInterpolateLuma_H264_8u_C1R_4x4_x						(*ippiInterpolateLuma_H264_8u_C1R_4x4_universal)
#define ippiInterpolateLuma_H264_8u_C1R_x							(*ippiInterpolateLuma_H264_8u_C1R_universal)
#define ippiInterpolateLumaTop_H264_8u_C1R_x						(*ippiInterpolateLumaTop_H264_8u_C1R_universal)
#define ippiInterpolateLumaBottom_H264_8u_C1R_x						(*ippiInterpolateLumaBottom_H264_8u_C1R_universal)
#define ippiInterpolateChroma_H264_8u_C1R_x							(*ippiInterpolateChroma_H264_8u_C1R_universal)
#define ippiInterpolateChromaTop_H264_8u_C1R_x						(*ippiInterpolateChromaTop_H264_8u_C1R_universal)						
#define ippiInterpolateChromaBottom_H264_8u_C1R_x					(*ippiInterpolateChromaBottom_H264_8u_C1R_universal)
#define ippiInterpolateBlock_H264_8u_P2P1R_x						(*ippiInterpolateBlock_H264_8u_P2P1R_universal)
#define ippiInterpolateBlock_H264_8u_P2P1R_x						(*ippiInterpolateBlock_H264_8u_P2P1R_universal)
#define ippiInterpolateBlock_H264_8u_P3P1R_x						(*ippiInterpolateBlock_H264_8u_P3P1R_universal)
#define ippiUniDirWeightBlock_H264_8u_C1R_x							(*ippiUniDirWeightBlock_H264_8u_C1R_universal)
#define ippiBiDirWeightBlock_H264_8u_P2P1R_x						(*ippiBiDirWeightBlock_H264_8u_P2P1R_universal)
#define ippiBiDirWeightBlock_H264_8u_P3P1R_x						(*ippiBiDirWeightBlock_H264_8u_P3P1R_universal)
#define ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x				(*ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_universal)
#define ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_x				(*ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_universal)

#define ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_x			(*ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_universal)
#define ippiReconstructChromaIntraMB_H264_16s8u_P2R_x				(*ippiReconstructChromaIntraMB_H264_16s8u_P2R_universal)
#define ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_x			(*ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_universal)
#define ippiReconstructLumaIntraMB_H264_16s8u_C1R_x					(*ippiReconstructLumaIntraMB_H264_16s8u_C1R_universal)
#define ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_x				(*ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_universal)
#define ippiReconstructLumaInterMB_H264_16s8u_C1R_x					(*ippiReconstructLumaInterMB_H264_16s8u_C1R_universal)
#define ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_x			(*ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal)
#define ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_x			(*ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal)
#define ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_x				(*ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_universal)
#define ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_x			(*ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_universal)
#define ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_x				(*ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_universal)
#define ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_x				(*ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_universal)
#define ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_x			(*ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_universal)
#define ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_x				(*ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_universal)
#define ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_x		(*ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_universal)
#define ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_x			(*ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_universal)
#define ippiReconstructChromaInterMB_H264_16s8u_P2R_x				(*ippiReconstructChromaInterMB_H264_16s8u_P2R_universal)
#define ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_x			(*ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_universal)

//***
//avc enc
//***

extern IppStatus  (__STDCALL *ippiEncodeCoeffsCAVLC_H264_16s_universal)						(const Ipp16s* pSrc, Ipp8u   AC, const Ipp32s  *pScanMatrix, Ipp8u Count,  Ipp8u   *Trailing_Ones,Ipp8u   *Trailing_One_Signs, Ipp8u   *NumOutCoeffs, Ipp8u   *TotalZeros,  Ipp16s  *Levels, Ipp8u   *Runs );
extern IppStatus  (__STDCALL *ippiEncodeChromaDcCoeffsCAVLC_H264_16s_universal)				(const Ipp16s* pSrc, Ipp8u   *Trailing_Ones, Ipp8u   *Trailing_One_Signs, Ipp8u   *NumOutCoeffs,  Ipp8u   *TotalZeros,  Ipp16s  *Levels,  Ipp8u   *Runs);


extern IppStatus  (__STDCALL *ippiTransformDequantLumaDC_H264_16s_C1I_universal)  			(Ipp16s* pSrcDst, Ipp32s  QP);
extern IppStatus  (__STDCALL *ippiTransformDequantChromaDC_H264_16s_C1I_universal) 			(Ipp16s* pSrcDst, Ipp32s  QP);
extern IppStatus  (__STDCALL *ippiDequantTransformResidualAndAdd_H264_16s_C1I_universal)	(const Ipp8u*  pPred,Ipp16s* pSrcDst, const Ipp16s* pDC,Ipp8u*  pDst,Ipp32s  PredStep,Ipp32s  DstStep, Ipp32s  QP,Ipp32s  AC);
extern IppStatus  (__STDCALL *ippiTransformQuantChromaDC_H264_16s_C1I_universal)			(Ipp16s* pSrcDst,Ipp16s* pTBlock, Ipp32s  QPCroma, Ipp8s*  NumLevels, Ipp8u   Intra, Ipp8u   NeedTransform);
extern IppStatus  (__STDCALL *ippiTransformQuantResidual_H264_16s_C1I_universal) 			(Ipp16s* pSrcDst,Ipp32s  QP,Ipp8s*  NumLevels,Ipp8u   Intra, const Ipp16s* pScanMatrix, Ipp8u*  LastCoeff);
extern IppStatus  (__STDCALL *ippiTransformQuantFwd4x4_H264_16s_C1_universal )				(const Ipp16s *pSrc,Ipp16s *pDst,Ipp32s QP,Ipp32s *NumLevels,Ipp32s Intra,const Ipp16s *pScanMatrix,Ipp32s *LastCoeff,const Ipp16s *pScaleLevels );
extern IppStatus  (__STDCALL *ippiTransformQuantLumaDC_H264_16s_C1I_universal) 				(Ipp16s* pSrcDst,Ipp16s* pTBlock, Ipp32s  QP,   Ipp8s*  NumLevels,  Ipp8u   NeedTransform,   const Ipp16s* pScanMatrix,  Ipp8u*  LastCoeff);


/// COMM :The following functions are some commam function in IPP:we need deleted it carefully
extern IppStatus  (__STDCALL *ippiResize_8u_C1R_universal)									(const Ipp8u*  pSrc,	IppiSize srcSize,	int	  srcStep, IppiRect srcRoi,Ipp8u* pDst, int dstStep, IppiSize dstRoiSize, double xFactor, double yFactor, int interpolation);
extern IppStatus  (__STDCALL *ippiSAD16x16_8u32s_universal)									(const Ipp8u*  pSrc,	Ipp32s	srcStep,	const Ipp8u*  pRef, Ipp32s  refStep, Ipp32s* pSAD,  Ipp32s  mcType);
extern IppStatus  (__STDCALL *ippiSAD8x8_8u32s_C1R_universal)								(const Ipp8u*  pSrcCur, int     srcCurStep,	const Ipp8u*  pSrcRef, int srcRefStep, Ipp32s* pDst, Ipp32s  mcType);
extern IppStatus  (__STDCALL *ippiSAD4x4_8u32s_universal)									(const Ipp8u*  pSrc,	Ipp32s	srcStep,	const Ipp8u*  pRef, Ipp32s  refStep, Ipp32s* pSAD,Ipp32s  mcType );
extern IppStatus  (__STDCALL *ippiSAD16x16Blocks8x8_8u16u_universal)						(const Ipp8u*  pSrc,	Ipp32s  srcStep,	const Ipp8u*  pRef, Ipp32s  refStep, Ipp16u*  pDstSAD, Ipp32s   mcType );
extern IppStatus  (__STDCALL *ippiSAD16x16Blocks4x4_8u16u_universal) 						(const Ipp8u*  pSrc,	Ipp32s  srcStep,	const Ipp8u*  pRef, Ipp32s  refStep,Ipp16u*  pDstSAD, Ipp32s   mcType);
extern IppStatus  (__STDCALL *ippiSumsDiff16x16Blocks4x4_8u16s_C1_universal)				(const Ipp8u*  pSrc,	Ipp32s	srcStep,	const Ipp8u* pPred, Ipp32s predStep, Ipp16s* pSums, Ipp16s* pDiff);
extern IppStatus  (__STDCALL *ippiSumsDiff8x8Blocks4x4_8u16s_C1_universal)					(const Ipp8u*  pSrc,	Ipp32s	srcStep,	const Ipp8u* pPred, Ipp32s predStep, Ipp16s* pSums,Ipp16s* pDiff);
extern IppStatus  (__STDCALL *ippiGetDiff4x4_8u16s_C1_universal)							(const Ipp8u*  pSrcCur, Ipp32s  srcCurStep, const Ipp8u*  pSrcRef, Ipp32s  srcRefStep, Ipp16s* pDstDiff, Ipp32s  dstDiffStep, Ipp16s* pDstPredictor, Ipp32s  dstPredictorStep, Ipp32s  mcType, Ipp32s  roundControl);
extern IppStatus  (__STDCALL *ippiSub4x4_8u16s_C1R_universal)								(const Ipp8u*  pSrcCur, Ipp32s  srcCurStep, const Ipp8u*  pSrcRef, Ipp32s  srcRefStep, Ipp16s* pDstDiff, Ipp32s  dstDiffStep);
extern IppStatus  (__STDCALL *ippiEdgesDetect16x16_8u_C1R_universal)						(const Ipp8u*  pSrc,	Ipp32u	srcStep,	Ipp8u EdgePelDifference,Ipp8u EdgePelCount, Ipp8u   *pRes);

/// The following ths encoder spec functions
IppStatus  __STDCALL   ippiEncodeCoeffsCAVLC_H264_16s_c(		const Ipp16s* pSrc, Ipp8u   AC, const Ipp32s  *pScanMatrix, Ipp8u Count,  Ipp8u   *Trailing_Ones,Ipp8u   *Trailing_One_Signs, Ipp8u   *NumOutCoeffs, Ipp8u   *TotalZeros,  Ipp16s  *Levels, Ipp8u   *Runs );
IppStatus  __STDCALL  ippiEncodeChromaDcCoeffsCAVLC_H264_16s_c(	const Ipp16s* pSrc,  Ipp8u   *Trailing_Ones, Ipp8u   *Trailing_One_Signs, Ipp8u   *NumOutCoeffs,  Ipp8u   *TotalZeros,  Ipp16s  *Levels,  Ipp8u   *Runs);

#define ippiEncodeCoeffsCAVLC_H264_16s_x						(*ippiEncodeCoeffsCAVLC_H264_16s_universal)
#define ippiEncodeChromaDcCoeffsCAVLC_H264_16s_x				(*ippiEncodeChromaDcCoeffsCAVLC_H264_16s_universal)


IppStatus  __STDCALL ippiTransformDequantLumaDC_H264_16s_C1I_c  (Ipp16s* pSrcDst, Ipp32s  QP);
IppStatus  __STDCALL ippiTransformDequantChromaDC_H264_16s_C1I_c (Ipp16s* pSrcDst, Ipp32s  QP);
IppStatus  __STDCALL ippiDequantTransformResidualAndAdd_H264_16s_C1I_c(const Ipp8u*  pPred,Ipp16s* pSrcDst, const Ipp16s* pDC,Ipp8u*  pDst,Ipp32s  PredStep,Ipp32s  DstStep, Ipp32s  QP,Ipp32s  AC);
IppStatus  __STDCALL ippiDequantTransformResidualAndAdd_H264_16s_C1I_NO_AC_c(const Ipp8u*  pPred,Ipp16s* pSrcDst, const Ipp16s* pDC,Ipp8u*  pDst,Ipp32s  PredStep,Ipp32s  DstStep, Ipp32s  QP);
IppStatus  __STDCALL ippiTransformQuantChromaDC_H264_16s_C1I_c(Ipp16s* pSrcDst,Ipp16s* pTBlock, Ipp32s  QPCroma, Ipp8s*  NumLevels, Ipp8u   Intra, Ipp8u   NeedTransform);
IppStatus  __STDCALL ippiTransformQuantResidual_H264_16s_C1I_c (Ipp16s* pSrcDst,Ipp32s  QP,Ipp8s*  NumLevels,Ipp8u   Intra,  const Ipp16s* pScanMatrix, Ipp8u*  LastCoeff);
IppStatus  __STDCALL ippiTransformQuantFwd4x4_H264_16s_C1_c(const Ipp16s *pSrc,Ipp16s *pDst,Ipp32s QP,Ipp32s *NumLevels,Ipp32s Intra,const Ipp16s *pScanMatrix,Ipp32s *LastCoeff,const Ipp16s *pScaleLevels);
IppStatus  __STDCALL ippiTransformQuantLumaDC_H264_16s_C1I_c (Ipp16s* pSrcDst,Ipp16s* pTBlock, Ipp32s  QP,   Ipp8s*  NumLevels,  Ipp8u   NeedTransform,   const Ipp16s* pScanMatrix,  Ipp8u*  LastCoeff);

#define ippiTransformDequantLumaDC_H264_16s_C1I_x				(*ippiTransformDequantLumaDC_H264_16s_C1I_universal)
#define ippiTransformDequantChromaDC_H264_16s_C1I_x				(*ippiTransformDequantChromaDC_H264_16s_C1I_universal)
#define ippiDequantTransformResidualAndAdd_H264_16s_C1I_x		(*ippiDequantTransformResidualAndAdd_H264_16s_C1I_universal)
#define ippiTransformQuantChromaDC_H264_16s_C1I_x				(*ippiTransformQuantChromaDC_H264_16s_C1I_universal)
#define ippiTransformQuantResidual_H264_16s_C1I_x				(*ippiTransformQuantResidual_H264_16s_C1I_universal)
#define ippiTransformQuantFwd4x4_H264_16s_C1_x					(*ippiTransformQuantFwd4x4_H264_16s_C1_universal)
#define ippiTransformQuantLumaDC_H264_16s_C1I_x					(*ippiTransformQuantLumaDC_H264_16s_C1I_universal)



/// COMM :The following functions are some commam function in IPP:we need deleted it carefully
IppStatus  __STDCALL ippiResize_8u_C1R_c(					const Ipp8u* pSrc,		IppiSize srcSize,	int srcStep, IppiRect srcRoi,Ipp8u* pDst, int dstStep, IppiSize dstRoiSize, double xFactor, double yFactor, int interpolation);
IppStatus  __STDCALL  ippiSAD4x4_8u32s_c(					const Ipp8u*  pSrc,		Ipp32s	srcStep,	const Ipp8u*  pRef,    Ipp32s  refStep, Ipp32s* pSAD,Ipp32s  mcType );
IppStatus  __STDCALL  ippiSAD16x16Blocks8x8_8u16u_c(		const Ipp8u*  pSrc,		Ipp32s  srcStep,	const Ipp8u*  pRef,    Ipp32s  refStep, Ipp16u*  pDstSAD, Ipp32s   mcType );
IppStatus  __STDCALL  ippiSAD16x16Blocks4x4_8u16u_c (		const Ipp8u*  pSrc,		Ipp32s  srcStep,	const Ipp8u*  pRef,    Ipp32s  refStep,Ipp16u*  pDstSAD, Ipp32s   mcType);
IppStatus  __STDCALL  ippiSumsDiff16x16Blocks4x4_8u16s_C1_c(const Ipp8u*  pSrc,		Ipp32s	srcStep,	const Ipp8u*  pPred,   Ipp32s predStep, Ipp16s* pSums, Ipp16s* pDiff);
IppStatus  __STDCALL  ippiSumsDiff8x8Blocks4x4_8u16s_C1_c(  const Ipp8u*  pSrc,		Ipp32s	srcStep,	const Ipp8u*  pPred,   Ipp32s predStep, Ipp16s* pSums,Ipp16s* pDiff);
IppStatus  __STDCALL  ippiGetDiff4x4_8u16s_C1_c(			const Ipp8u*  pSrcCur,	Ipp32s  srcCurStep, const Ipp8u*  pSrcRef, Ipp32s  srcRefStep, Ipp16s* pDstDiff, Ipp32s  dstDiffStep, Ipp16s* pDstPredictor, Ipp32s  dstPredictorStep, Ipp32s  mcType, Ipp32s  roundControl);
IppStatus  __STDCALL  ippiSub4x4_8u16s_C1R_c(				const Ipp8u*  pSrcCur,	Ipp32s  srcCurStep, const Ipp8u*  pSrcRef, Ipp32s  srcRefStep, Ipp16s* pDstDiff, Ipp32s  dstDiffStep);
IppStatus  __STDCALL ippiEdgesDetect16x16_8u_C1R_c(			const Ipp8u*  pSrc,		Ipp32u	srcStep,Ipp8u EdgePelDifference,Ipp8u EdgePelCount, Ipp8u   *pRes);

//***for lossy optimization
IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_bilinear_approx_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, Ipp32s dx, Ipp32s dy, IppiSize dummp);

// Defined some thing for AVC encoder
#define ippiResize_8u_C1R_x							(*ippiResize_8u_C1R_universal)
#define ippiSAD16x16_8u32s_x						(*ippiSAD16x16_8u32s_universal)
#define ippiSAD8x8_8u32s_C1R_x						(*ippiSAD8x8_8u32s_C1R_universal)
#define ippiSAD4x4_8u32s_x							(*ippiSAD4x4_8u32s_universal)
#define ippiSAD16x16Blocks8x8_8u16u_x				(*ippiSAD16x16Blocks8x8_8u16u_universal)
#define ippiSAD16x16Blocks4x4_8u16u_x				(*ippiSAD16x16Blocks4x4_8u16u_universal)
#define ippiSumsDiff16x16Blocks4x4_8u16s_C1_x		(*ippiSumsDiff16x16Blocks4x4_8u16s_C1_universal)
#define ippiSumsDiff8x8Blocks4x4_8u16s_C1_x			(*ippiSumsDiff8x8Blocks4x4_8u16s_C1_universal)
#define ippiGetDiff4x4_8u16s_C1_x					(*ippiGetDiff4x4_8u16s_C1_universal)
#define ippiSub4x4_8u16s_C1R_x						(*ippiSub4x4_8u16s_C1R_universal)
#define ippiEdgesDetect16x16_8u_C1R_x				(*ippiEdgesDetect16x16_8u_C1R_universal)

IppStatus  __STDCALL  ippiSAD16x16_8u32s_aligned_4_arm_v6(const Ipp8u*  pSrc, Ipp32s srcStep, const Ipp8u*  pRef, Ipp32s* pSAD );
IppStatus  __STDCALL  ippiSAD16x16_8u32s_misaligned_arm_v6(const Ipp8u*  pSrc, Ipp32s srcStep, const Ipp8u*  pRef, Ipp32s  refStep, Ipp32s* pSAD,  Ipp32s  mcType);
IppStatus  __STDCALL  ippiSAD8x8_8u32s_arm_v6(const Ipp8u*  pSrc, Ipp32s srcStep, const Ipp8u*  pRef, Ipp32s  refStep, Ipp32s* pSAD,  Ipp32s  mcType);
IppStatus  __STDCALL  ippiSAD4x4_8u32s_arm_v6(const Ipp8u*  pSrc, Ipp32s srcStep, const Ipp8u*  pRef, Ipp32s  refStep, Ipp32s* pSAD,  Ipp32s  mcType);
IppStatus __STDCALL	  ippiMeanAbsDev16x16_8u32s_C1R_arm_v6(const Ipp8u*  pSrc,  int srcStep, Ipp32s* pDst);


///*** Add for AAC encoder

extern IppStatus (__STDCALL *ippsCopy_16s_universal)				(const Ipp16s* pSrc, Ipp16s* pDst, int len );
extern IppStatus (__STDCALL *ippsSet_16s_universal)					( Ipp16s val, Ipp16s* pDst, int len );
extern IppStatus (__STDCALL *ippsSet_32s_universal)					( Ipp32s val, Ipp32s* pDst, int len );
extern IppStatus (__STDCALL *ippsAbs_16s_I_universal)				(Ipp16s* pSrcDst,int len);
extern IppStatus (__STDCALL *ippsAbs_16s_universal)					(const Ipp16s* pSrc, Ipp16s* pDst,int len);
extern IppStatus (__STDCALL *ippsMinEvery_32s_I_universal)			(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len);
extern IppStatus (__STDCALL *ippsMaxEvery_32s_I_universal)			(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len);
extern IppStatus (__STDCALL *ippsMinMax_16s_universal)				(const Ipp16s* pSrc, int len, Ipp16s* pMin, Ipp16s* pMax);
extern IppStatus (__STDCALL *ippsSum_16s32s_Sfs_universal)			(const Ipp16s*  pSrc, int len,Ipp32s*  pSum, int scaleFactor);
extern IppStatus (__STDCALL *ippsSum_32s_Sfs_universal)				(const Ipp32s*  pSrc, int len, Ipp32s*  pSum, int scaleFactor);
extern IppStatus (__STDCALL *ippsAdd_16s_Sfs_universal)				(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsAdd_16s_ISfs_universal)			(const Ipp16s*  pSrc, Ipp16s*  pSrcDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsAddC_16s_Sfs_universal)			(const Ipp16s*  pSrc, Ipp16s  val,Ipp16s*  pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsSub_16s_universal)					(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp16s*  pDst, int len);
extern IppStatus (__STDCALL *ippsSub_16s_ISfs_universal)			(const Ipp16s*  pSrc, Ipp16s*  pSrcDst,int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsSub_16s_Sfs_universal)				(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor);
																	
extern IppStatus (__STDCALL *ippsMul_16s_Sfs_universal)				(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMul_16s_ISfs_universal)			(const Ipp16s*  pSrc, Ipp16s*  pSrcDst,int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMulC_32s_Sfs_universal)			(const Ipp32s*  pSrc, Ipp32s  val,Ipp32s*  pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMulC_32s_ISfs_universal)			(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMul_16s32s_Sfs_universal)			(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp32s*  pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMulC_16s_Sfs_universal)			(const Ipp16s*  pSrc, Ipp16s  val,Ipp16s*  pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMul_16s_universal)					(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp16s*  pDst, int len);
extern IppStatus (__STDCALL *ippsMul_32s_ISfs_universal)			(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsMul_32s_Sfs_universal)				(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst,int len, int scaleFactor);

extern IppStatus (__STDCALL *ippsDiv_32s_ISfs_universal)			(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int ScaleFactor);
extern IppStatus (__STDCALL *ippsDiv_16s_Sfs_universal)				(const Ipp16s* pSrc1, const Ipp16s* pSrc2,Ipp16s* pDst, int len, int ScaleFactor);
extern IppStatus (__STDCALL *ippsDiv_16s_ISfs_universal)			(const Ipp16s* pSrc, Ipp16s* pSrcDst,int len, int ScaleFactor);
extern IppStatus (__STDCALL *ippsDivC_16s_ISfs_universal)			(Ipp16s val, Ipp16s* pSrcDst, int len, int ScaleFactor);
extern IppStatus (__STDCALL *ippsSpread_16s_Sfs_universal)			(Ipp16s src1, Ipp16s src2, int inScaleFactor, Ipp16s* pDst);
extern IppStatus (__STDCALL *ippsPow34_16s_Sfs_universal)			(const Ipp16s* pSrc, int inScaleFactor, Ipp16s* pDst, int ScaleFactor, int len);
extern IppStatus (__STDCALL *ippsMagnitude_16sc_Sfs_universal)		(const Ipp16sc* pSrc,Ipp16s* pDst, int len,int scaleFactor);
extern IppStatus (__STDCALL *ippsMagnitude_16s_Sfs_universal)		(const Ipp16s* pSrcRe,const Ipp16s* pSrcIm,Ipp16s* pDst,int len,int scaleFactor);
extern IppStatus (__STDCALL *ippsConvert_64s32s_Sfs_universal)		(const Ipp64s* pSrc, Ipp32s* pDst, int len, IppRoundMode rndMode, int scaleFactor);
extern IppStatus (__STDCALL *ippsConvert_32s16s_Sfs_universal)		(const Ipp32s* pSrc, Ipp16s* pDst, int len, int scaleFactor);
extern IppStatus (__STDCALL *ippsRShiftC_32s_I_universal)			(int val, Ipp32s* pSrcDst, int len);
extern IppStatus (__STDCALL *ippsRShiftC_32s_universal)				(const Ipp32s* pSrc, int val, Ipp32s* pDst, int len);
extern IppStatus (__STDCALL *ippsRShiftC_16s_universal)				(const Ipp16s* pSrc, int val, Ipp16s* pDst, int len);
extern IppStatus (__STDCALL *ippsLShiftC_16s_I_universal)			(int val, Ipp16s* pSrcDst, int len);

extern IppStatus (__STDCALL *ippsDotProd_16s32s32s_Sfs_universal)	( const Ipp16s* pSrc1, const Ipp32s* pSrc2,int len, Ipp32s* pDp, int scaleFactor );
extern IppStatus (__STDCALL *ippsDotProd_16s32s_Sfs_universal)		( const Ipp16s* pSrc1, const Ipp16s* pSrc2, int len, Ipp32s* pDp, int scaleFactor );
extern IppStatus (__STDCALL *ippsLn_32s16s_Sfs_universal)			( const Ipp32s* pSrc, Ipp16s* pDst, int Len, int scaleFactor);
extern IppStatus (__STDCALL *ippsDeinterleave_16s_universal)		(const Ipp16s* pSrc, int ch_num,int len, Ipp16s** pDst);

// AAC encoder VLC
extern IppStatus (__STDCALL *ippsVLCEncodeBlock_16s1u_universal)		(const Ipp16s* pSrc, int srcLen, Ipp8u** ppDst, int* pDstBitsOffset, const IppsVLCEncodeSpec_32s* pVLCSpec);
extern IppStatus (__STDCALL *ippsVLCEncodeEscBlock_AAC_16s1u_universal)	(const Ipp16s *pSrc,int srcLen,  Ipp8u **ppDst,  int *pDstBitsOffset, const IppsVLCEncodeSpec_32s *pVLCSpec);
extern IppStatus (__STDCALL *ippsVLCEncodeInitAlloc_32s_universal)		(const IppsVLCTable_32s* pInputTable, int inputTableSize, IppsVLCEncodeSpec_32s** ppVLCSpec);
extern void (__STDCALL *ippsVLCEncodeFree_32s_universal)				(IppsVLCEncodeSpec_32s* pVLCSpec);
extern IppStatus (__STDCALL *ippsVLCCountBits_16s32s_universal)			(const Ipp16s* pSrc, int srcLen, Ipp32s* pCountBits, const IppsVLCEncodeSpec_32s* pVLCSpec);
extern IppStatus (__STDCALL *ippsVLCCountEscBits_AAC_16s32s_universal)	(const Ipp16s *pSrc, int srcLen,  Ipp32s *pCountBits,    const IppsVLCEncodeSpec_32s *pVLCSpec);

extern IppStatus (__STDCALL *ippsThreshold_LT_16s_I_universal)			( Ipp16s* pSrcDst, int len,   Ipp16s level );
extern IppStatus (__STDCALL *ippsConjPack_16sc_universal)				( const Ipp16s* pSrc, Ipp16sc* pDst, int lenDst );
extern IppStatus (__STDCALL *ippsConjCcs_16sc_universal)				( const Ipp16s* pSrc, Ipp16sc* pDst, int lenDst );

// FFT
extern IppStatus (__STDCALL *ippsFFTInitAlloc_R_32s_universal)				(IppsFFTSpec_R_32s** ppFFTSpec,int order, int flag, IppHintAlgorithm hint );
extern IppStatus (__STDCALL *ippsFFTGetBufSize_C_32sc_universal)			(const IppsFFTSpec_C_32sc* pFFTSpec, int* pSize);
extern IppStatus (__STDCALL *ippsFFTGetBufSize_R_32s_universal) 			(const IppsFFTSpec_R_32s*  pFFTSpec, int* pSize );
extern IppStatus (__STDCALL *ippsFFTGetBufSize_R_16s_universal) 			(const IppsFFTSpec_R_16s*  pFFTSpec, int* pSize );
extern IppStatus (__STDCALL *ippsFFTGetBufSize_C_16sc_universal) 			(const IppsFFTSpec_C_16sc*  pFFTSpec, int* pSize );
extern IppStatus (__STDCALL *ippsMDCTFwdGetBufSize_16s_universal)			(const IppsMDCTFwdSpec_16s *pMDCTSpec, int *pSize);
extern IppStatus (__STDCALL *ippsMDCTFwdFree_16s_universal)					(IppsMDCTFwdSpec_16s* pMDCTSpec);
extern IppStatus (__STDCALL *ippsMDCTFwdInitAlloc_16s_universal)			(IppsMDCTFwdSpec_16s ** ppMDCTSpec, int len);
extern IppStatus (__STDCALL *ippsMDCTFwd_16s_Sfs_universal)					(const Ipp16s *pSrc, Ipp16s *pDst,const IppsMDCTFwdSpec_16s* pMDCTSpec,int scaleFactor, Ipp8u* pBuffer);
extern IppStatus (__STDCALL *ippsFFTInitAlloc_R_16s_universal)				( IppsFFTSpec_R_16s** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint );
extern IppStatus (__STDCALL *ippsFFTInitAlloc_C_16s_universal)				( IppsFFTSpec_C_16s** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint );
extern IppStatus (__STDCALL *ippsFFTInitAlloc_C_16sc_universal)				(IppsFFTSpec_C_16sc** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint );
extern IppStatus (__STDCALL *ippsFFTInitAlloc_C_32sc_universal)				(IppsFFTSpec_C_32sc** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint );
extern IppStatus (__STDCALL *ippsFFTFree_R_16s_universal) 					( IppsFFTSpec_R_16s*  pFFTSpec );
extern IppStatus (__STDCALL *ippsFFTFree_C_16s_universal)					( IppsFFTSpec_C_16s*  pFFTSpec );
extern IppStatus (__STDCALL *ippsFFTFree_C_16sc_universal)					( IppsFFTSpec_C_16sc* pFFTSpec );
extern IppStatus (__STDCALL *ippsFFTFree_C_32sc_universal)					(IppsFFTSpec_C_32sc* pFFTSpec);
//extern IppStatus (__STDCALL *ippsFFTFwd_CToC_16sc_Sfs_universal)			(const Ipp16sc* pSrc, Ipp16sc* pDst, const IppsFFTSpec_C_16sc* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer );
extern IppStatus (__STDCALL *ippsFFTFwd_CToC_32sc_Sfs_universal)			(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer );
extern IppStatus (__STDCALL *ippsFFTFwd_RToCCS_16s_Sfs_universal)			(const Ipp16s* pSrc,  Ipp16s* pDst,  const IppsFFTSpec_R_16s* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer );
extern IppStatus (__STDCALL *ippsFFTFwd_RToPack_32s_Sfs_universal)			(const Ipp32s* pSrc, Ipp32s* pDst,   const IppsFFTSpec_R_32s* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer );
extern IppStatus (__STDCALL *ippsFFTInv_PackToR_32s_Sfs_universal)			(const Ipp32s* pSrc, Ipp32s* pDst, const IppsFFTSpec_R_32s* pFFTSpec,   int scaleFactor, Ipp8u* pBuffer );

IppStatus __STDCALL ippsCopy_16s_c(const Ipp16s* pSrc, Ipp16s* pDst, int len );
IppStatus __STDCALL ippsSet_16s_c( Ipp16s val, Ipp16s* pDst, int len );
IppStatus __STDCALL ippsSet_32s_c( Ipp32s val, Ipp32s* pDst, int len );
IppStatus __STDCALL ippsAbs_16s_I_c(Ipp16s* pSrcDst,int len);
IppStatus __STDCALL ippsMinEvery_32s_I_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len);
IppStatus __STDCALL ippsMaxEvery_32s_I_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len);
IppStatus __STDCALL ippsAbs_16s_c(const Ipp16s* pSrc, Ipp16s* pDst,int len);
IppStatus __STDCALL ippsMinEvery_32s_I_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len);
IppStatus __STDCALL ippsMinMax_16s_c(const Ipp16s* pSrc, int len, Ipp16s* pMin, Ipp16s* pMax);
IppStatus __STDCALL ippsSum_16s32s_Sfs_c(const Ipp16s*  pSrc, int len,Ipp32s*  pSum, int scaleFactor);
IppStatus __STDCALL ippsSum_32s_Sfs_c(const Ipp32s*  pSrc, int len, Ipp32s*  pSum, int scaleFactor);
IppStatus __STDCALL ippsAdd_16s_Sfs_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsAdd_16s_ISfs_c(const Ipp16s*  pSrc, Ipp16s*  pSrcDst, int len, int scaleFactor);
IppStatus __STDCALL ippsAddC_16s_Sfs_c(const Ipp16s*  pSrc, Ipp16s  val,Ipp16s*  pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsSub_16s_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp16s*  pDst, int len);
IppStatus __STDCALL ippsSub_16s_ISfs_c(const Ipp16s*  pSrc, Ipp16s*  pSrcDst,int len, int scaleFactor);
IppStatus __STDCALL ippsSub_16s_Sfs_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor);

IppStatus __STDCALL ippsMul_16s_Sfs_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsMul_16s_ISfs_c(const Ipp16s*  pSrc, Ipp16s*  pSrcDst,int len, int scaleFactor);
IppStatus __STDCALL ippsMulC_32s_Sfs_c(const Ipp32s*  pSrc, Ipp32s  val,Ipp32s*  pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsMulC_32s_ISfs_c(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor);
IppStatus __STDCALL ippsMul_16s32s_Sfs_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp32s*  pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsMulC_16s_Sfs_c(const Ipp16s*  pSrc, Ipp16s  val,Ipp16s*  pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsMul_16s_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp16s*  pDst, int len);
IppStatus __STDCALL ippsMul_32s_ISfs_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor);
IppStatus __STDCALL ippsMul_32s_Sfs_c(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst,int len, int scaleFactor);

IppStatus __STDCALL ippsDiv_32s_ISfs_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int ScaleFactor);
IppStatus __STDCALL ippsDiv_16s_Sfs_c(const Ipp16s* pSrc1, const Ipp16s* pSrc2,Ipp16s* pDst, int len, int ScaleFactor);
IppStatus __STDCALL ippsDiv_16s_ISfs_c(const Ipp16s* pSrc, Ipp16s* pSrcDst,int len, int ScaleFactor);
IppStatus __STDCALL ippsDivC_16s_ISfs_c(Ipp16s val, Ipp16s* pSrcDst, int len, int ScaleFactor);
IppStatus __STDCALL ippsSpread_16s_Sfs_c(Ipp16s src1, Ipp16s src2, int inScaleFactor, Ipp16s* pDst);
IppStatus __STDCALL ippsPow34_16s_Sfs_c(const Ipp16s* pSrc, int inScaleFactor, Ipp16s* pDst, int ScaleFactor, int len);
IppStatus __STDCALL ippsMagnitude_16sc_Sfs_c(const Ipp16sc* pSrc,Ipp16s* pDst, int len,int scaleFactor);
IppStatus __STDCALL ippsMagnitude_16s_Sfs_c(const Ipp16s* pSrcRe,const Ipp16s* pSrcIm,Ipp16s* pDst,int len,int scaleFactor);
IppStatus __STDCALL ippsConvert_64s32s_Sfs_c(const Ipp64s* pSrc, Ipp32s* pDst, int len, IppRoundMode rndMode, int scaleFactor);
IppStatus __STDCALL ippsConvert_32s16s_Sfs_c(const Ipp32s* pSrc, Ipp16s* pDst, int len, int scaleFactor);
IppStatus __STDCALL ippsRShiftC_32s_I_c(int val, Ipp32s* pSrcDst, int len);
IppStatus __STDCALL ippsRShiftC_32s_c(const Ipp32s* pSrc, int val, Ipp32s* pDst, int len);
IppStatus __STDCALL ippsRShiftC_16s_c(const Ipp16s* pSrc, int val, Ipp16s* pDst, int len);
IppStatus __STDCALL ippsLShiftC_16s_I_c(int val, Ipp16s* pSrcDst, int len);

IppStatus __STDCALL ippsDotProd_16s32s32s_Sfs_c( const Ipp16s* pSrc1, const Ipp32s* pSrc2,int len, Ipp32s* pDp, int scaleFactor );
IppStatus __STDCALL ippsDotProd_16s32s_Sfs_c( const Ipp16s* pSrc1, const Ipp16s* pSrc2, int len, Ipp32s* pDp, int scaleFactor );
IppStatus __STDCALL ippsLn_32s16s_Sfs_c( const Ipp32s* pSrc, Ipp16s* pDst, int Len, int scaleFactor);
IppStatus __STDCALL ippsDeinterleave_16s_c(const Ipp16s* pSrc, int ch_num,int len, Ipp16s** pDst);

// AAC encoder VLC
IppStatus __STDCALL ippsVLCEncodeBlock_16s1u_c(const Ipp16s* pSrc, int srcLen, Ipp8u** ppDst, int* pDstBitsOffset, const IppsVLCEncodeSpec_32s* pVLCSpec);
IppStatus __STDCALL ippsVLCEncodeEscBlock_AAC_16s1u_c(const Ipp16s *pSrc,int srcLen,  Ipp8u **ppDst,  int *pDstBitsOffset, const IppsVLCEncodeSpec_32s *pVLCSpec);
IppStatus __STDCALL ippsVLCEncodeInitAlloc_32s_c(const IppsVLCTable_32s* pInputTable, int inputTableSize, IppsVLCEncodeSpec_32s** ppVLCSpec);
void __STDCALL ippsVLCEncodeFree_32s_c(IppsVLCEncodeSpec_32s* pVLCSpec);
IppStatus __STDCALL ippsVLCCountBits_16s32s_c(const Ipp16s* pSrc, int srcLen, Ipp32s* pCountBits, const IppsVLCEncodeSpec_32s* pVLCSpec);
IppStatus __STDCALL ippsVLCCountEscBits_AAC_16s32s_c(const Ipp16s *pSrc, int srcLen,  Ipp32s *pCountBits,    const IppsVLCEncodeSpec_32s *pVLCSpec);

IppStatus __STDCALL ippsThreshold_LT_16s_I_c( Ipp16s* pSrcDst, int len,   Ipp16s level );
IppStatus __STDCALL ippsConjPack_16sc_c( const Ipp16s* pSrc, Ipp16sc* pDst, int lenDst );
IppStatus __STDCALL ippsConjCcs_16sc_c( const Ipp16s* pSrc, Ipp16sc* pDst, int lenDst );

// FFT
IppStatus __STDCALL ippsFFTInitAlloc_R_32s_c(IppsFFTSpec_R_32s** ppFFTSpec,int order, int flag, IppHintAlgorithm hint );
IppStatus __STDCALL ippsFFTGetBufSize_R_32s_c (const IppsFFTSpec_R_32s*  pFFTSpec, int* pSize );
IppStatus __STDCALL ippsFFTGetBufSize_R_16s_c (const IppsFFTSpec_R_16s*  pFFTSpec, int* pSize );
IppStatus __STDCALL ippsFFTGetBufSize_C_16sc_c (const IppsFFTSpec_C_16sc*  pFFTSpec, int* pSize );
IppStatus __STDCALL ippsMDCTFwdGetBufSize_16s_c(const IppsMDCTFwdSpec_16s *pMDCTSpec, int *pSize);
IppStatus __STDCALL ippsMDCTFwdFree_16s_c(IppsMDCTFwdSpec_16s* pMDCTSpec);
IppStatus __STDCALL ippsMDCTFwdInitAlloc_16s_c(IppsMDCTFwdSpec_16s ** ppMDCTSpec, int len);
IppStatus __STDCALL ippsMDCTFwd_16s_Sfs_c(const Ipp16s *pSrc, Ipp16s *pDst,const IppsMDCTFwdSpec_16s* pMDCTSpec,int scaleFactor, Ipp8u* pBuffer);
IppStatus __STDCALL ippsFFTInitAlloc_R_16s_c( IppsFFTSpec_R_16s** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint );
IppStatus __STDCALL ippsFFTInitAlloc_C_16s_c( IppsFFTSpec_C_16s** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint );
IppStatus __STDCALL ippsFFTInitAlloc_C_16sc_c(IppsFFTSpec_C_16sc** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint );
IppStatus __STDCALL ippsFFTFree_R_16s_c ( IppsFFTSpec_R_16s*  pFFTSpec );
IppStatus __STDCALL ippsFFTFree_C_16s_c ( IppsFFTSpec_C_16s*  pFFTSpec );
IppStatus __STDCALL ippsFFTFree_C_16sc_c( IppsFFTSpec_C_16sc* pFFTSpec );
IppStatus __STDCALL ippsFFTFwd_CToC_16sc_Sfs_c (const Ipp16sc* pSrc, Ipp16sc* pDst, const IppsFFTSpec_C_16sc* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer );
IppStatus __STDCALL ippsFFTFwd_RToCCS_16s_Sfs_c(const Ipp16s* pSrc,  Ipp16s* pDst,  const IppsFFTSpec_R_16s* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer );
IppStatus __STDCALL ippsFFTFwd_RToPack_32s_Sfs_c(const Ipp32s* pSrc, Ipp32s* pDst,   const IppsFFTSpec_R_32s* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer );
IppStatus __STDCALL ippsFFTInv_PackToR_32s_Sfs_c(const Ipp32s* pSrc, Ipp32s* pDst, const IppsFFTSpec_R_32s* pFFTSpec,   int scaleFactor, Ipp8u* pBuffer );

#define ippsCopy_16s_x							(*ippsCopy_16s_universal)								
#define ippsSet_16s_x							(*ippsSet_16s_universal)					
#define ippsSet_32s_x							(*ippsSet_32s_universal)					
#define ippsAbs_16s_I_x							(*ippsAbs_16s_I_universal)				
#define ippsAbs_16s_x							(*ippsAbs_16s_universal)					
#define ippsMinEvery_32s_I_x					(*ippsMinEvery_32s_I_universal)			
#define ippsMaxEvery_32s_I_x					(*ippsMaxEvery_32s_I_universal)			
#define ippsMinMax_16s_x						(*ippsMinMax_16s_universal)				
#define ippsSum_16s32s_Sfs_x					(*ippsSum_16s32s_Sfs_universal)			
#define ippsSum_32s_Sfs_x						(*ippsSum_32s_Sfs_universal)				
#define ippsAdd_16s_Sfs_x						(*ippsAdd_16s_Sfs_universal)				
#define ippsAdd_16s_ISfs_x						(*ippsAdd_16s_ISfs_universal)			
#define ippsAddC_16s_Sfs_x						(*ippsAddC_16s_Sfs_universal)			
#define ippsSub_16s_x							(*ippsSub_16s_universal)					
#define ippsSub_16s_ISfs_x						(*ippsSub_16s_ISfs_universal)			
#define ippsSub_16s_Sfs_x						(*ippsSub_16s_Sfs_universal)				
																						
#define ippsMul_16s_Sfs_x						(*ippsMul_16s_Sfs_universal)				
#define ippsMul_16s_ISfs_x						(*ippsMul_16s_ISfs_universal)			
#define ippsMulC_32s_Sfs_x						(*ippsMulC_32s_Sfs_universal)			
#define ippsMulC_32s_ISfs_x						(*ippsMulC_32s_ISfs_universal)			
#define ippsMul_16s32s_Sfs_x					(*ippsMul_16s32s_Sfs_universal)			
#define ippsMulC_16s_Sfs_x						(*ippsMulC_16s_Sfs_universal)			
#define ippsMul_16s_x							(*ippsMul_16s_universal)					
#define ippsMul_32s_ISfs_x						(*ippsMul_32s_ISfs_universal)			
#define ippsMul_32s_Sfs_x						(*ippsMul_32s_Sfs_universal)				

#define ippsDiv_32s_ISfs_x						(*ippsDiv_32s_ISfs_universal)			
#define ippsDiv_16s_Sfs_x						(*ippsDiv_16s_Sfs_universal)				
#define ippsDiv_16s_ISfs_x						(*ippsDiv_16s_ISfs_universal)			
#define ippsDivC_16s_ISfs_x						(*ippsDivC_16s_ISfs_universal)			
#define ippsSpread_16s_Sfs_x					(*ippsSpread_16s_Sfs_universal)			
#define ippsPow34_16s_Sfs_x						(*ippsPow34_16s_Sfs_universal)			
#define ippsMagnitude_16sc_Sfs_x				(*ippsMagnitude_16sc_Sfs_universal)		
#define ippsMagnitude_16s_Sfs_x					(*ippsMagnitude_16s_Sfs_universal)		
#define ippsConvert_64s32s_Sfs_x				(*ippsConvert_64s32s_Sfs_universal)		
#define ippsConvert_32s16s_Sfs_x				(*ippsConvert_32s16s_Sfs_universal)		
#define ippsRShiftC_32s_I_x						(*ippsRShiftC_32s_I_universal)			
#define ippsRShiftC_32s_x						(*ippsRShiftC_32s_universal)				
#define ippsRShiftC_16s_x						(*ippsRShiftC_16s_universal)				
#define ippsLShiftC_16s_I_x						(*ippsLShiftC_16s_I_universal)			

#define ippsDotProd_16s32s32s_Sfs_x				(*ippsDotProd_16s32s32s_Sfs_universal)	
#define ippsDotProd_16s32s_Sfs_x				(*ippsDotProd_16s32s_Sfs_universal)		
#define ippsLn_32s16s_Sfs_x						(*ippsLn_32s16s_Sfs_universal)			
#define ippsDeinterleave_16s_x					(*ippsDeinterleave_16s_universal)		


#define ippsVLCEncodeBlock_16s1u_x				(*ippsVLCEncodeBlock_16s1u_universal)		
#define ippsVLCEncodeEscBlock_AAC_16s1u_x	    (*ippsVLCEncodeEscBlock_AAC_16s1u_universal)	
#define ippsVLCEncodeInitAlloc_32s_x		    (*ippsVLCEncodeInitAlloc_32s_universal)		
#define ippsVLCEncodeFree_32s_x				    (*ippsVLCEncodeFree_32s_universal)				
#define ippsVLCCountBits_16s32s_x			    (*ippsVLCCountBits_16s32s_universal)			
#define ippsVLCCountEscBits_AAC_16s32s_x	    (*ippsVLCCountEscBits_AAC_16s32s_universal)	

#define ippsThreshold_LT_16s_I_x			    (*ippsThreshold_LT_16s_I_universal)			
#define ippsConjPack_16sc_x						(*ippsConjPack_16sc_universal)				
#define ippsConjCcs_16sc_x						(*ippsConjCcs_16sc_universal)				


#define ippsFFTInitAlloc_R_32s_x			    (*ippsFFTInitAlloc_R_32s_universal)			
#define ippsFFTGetBufSize_C_32sc_x				(*ippsFFTGetBufSize_C_32sc_universal)		
#define ippsFFTGetBufSize_R_32s_x 				(*ippsFFTGetBufSize_R_32s_universal) 		
#define ippsFFTGetBufSize_R_16s_x 				(*ippsFFTGetBufSize_R_16s_universal) 		
#define ippsFFTGetBufSize_C_16sc_x 				(*ippsFFTGetBufSize_C_16sc_universal) 		
#define ippsMDCTFwdGetBufSize_16s_x				(*ippsMDCTFwdGetBufSize_16s_universal)		
#define ippsMDCTFwdFree_16s_x				    (*ippsMDCTFwdFree_16s_universal)				
#define ippsMDCTFwdInitAlloc_16s_x				(*ippsMDCTFwdInitAlloc_16s_universal)		
#define ippsMDCTFwd_16s_Sfs_x				    (*ippsMDCTFwd_16s_Sfs_universal)				
#define ippsFFTInitAlloc_R_16s_x			    (*ippsFFTInitAlloc_R_16s_universal)			
#define ippsFFTInitAlloc_C_16s_x			    (*ippsFFTInitAlloc_C_16s_universal)			
#define ippsFFTInitAlloc_C_16sc_x			    (*ippsFFTInitAlloc_C_16sc_universal)			
#define ippsFFTInitAlloc_C_32sc_x			    (*ippsFFTInitAlloc_C_32sc_universal)			
#define ippsFFTFree_R_16s_x 				    (*ippsFFTFree_R_16s_universal) 				
#define ippsFFTFree_C_16s_x						(*ippsFFTFree_C_16s_universal)				
#define ippsFFTFree_C_16sc_x				    (*ippsFFTFree_C_16sc_universal)				
#define ippsFFTFree_C_32sc_x				    (*ippsFFTFree_C_32sc_universal)				
//#define ippsFFTFwd_CToC_16sc_Sfs_x				(*ippsFFTFwd_CToC_16sc_Sfs_universal)		
#define ippsFFTFwd_CToC_32sc_Sfs_x				(*ippsFFTFwd_CToC_32sc_Sfs_universal)		
#define ippsFFTFwd_RToCCS_16s_Sfs_x				(*ippsFFTFwd_RToCCS_16s_Sfs_universal)		
#define ippsFFTFwd_RToPack_32s_Sfs_x		    (*ippsFFTFwd_RToPack_32s_Sfs_universal)		
#define ippsFFTInv_PackToR_32s_Sfs_x		    (*ippsFFTInv_PackToR_32s_Sfs_universal)		

#ifdef __cplusplus
}
#endif


#endif	//__KINOMA_IPP_LIB_H__

