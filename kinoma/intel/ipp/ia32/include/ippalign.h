/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
//          Intel(R) Integrated Performance Primitives
//               Intel(R) IPP PCA compatibility (ippalign)
//
*/

#if !defined( __IPPALIGN_H__ ) || defined( _OWN_BLDPCS )
#define __IPPALIGN_H__

#ifndef __IPPDEFS_H__
  #include "ippdefs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined (_WIN32_WCE) && defined (_M_IX86) && defined (__stdcall)
  #define _IPP_STDCALL_CDECL
  #undef __stdcall
#endif


/* /////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                   Functions declarations
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////// */


/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippalignGetLibVersion
//  Purpose:    getting of the library version
//  Returns:    the structure of information about version
//              of ippALIGN library
//  Parameters:
//
//  Notes:      not necessary to release the returned structure
*/
IPPAPI( const IppLibraryVersion*, ippalignGetLibVersion, (void) )

/* /////////////////////////////////////////////////////////////////////////////
//                       Macros
//
*/
#define ippiJoin422_8u_P3C2R                   ippiYCbCr422_8u_P3C2R
#define ippiSplit422_8u_C2P3R                  ippiYCbCr422_8u_C2P3R
#define ippiYCbCr422ToYCbCr422_8u_C2P3R        ippiYCbCr422_8u_C2P3R

#define ippiJoin420_8u_P2C2R                   ippiYCbCr420ToYCbCr422_8u_P2C2R
#define ippiJoin420_Filter_8u_P2C2R            ippiYCbCr420ToYCbCr422_Filter_8u_P2C2R

#define ippiSplit420_Filter_8u_P2P3R           ippiYCbCr420ToYCrCb420_Filter_8u_P2P3R

#define ippiYUToYU422_8u_C2P2R                 ippiYCbCr422ToYCbCr420_8u_C2P2R
#define ippiUYToYU422_8u_C2P2R                 ippiCbYCr422ToYCbCr420_8u_C2P2R

#define ippiYUToYV422_8u_C2P3R                 ippiYCbCr422ToYCrCb420_8u_C2P3R
#define ippiUYToYV422_8u_C2P3R                 ippiCbYCr422ToYCrCb420_8u_C2P3R

#define ippiYUToUY422_8u_C2R                   ippiYCbCr422ToCbYCr422_8u_C2R
#define ippiUYToYU422_8u_C2R                   ippiCbYCr422ToYCbCr422_8u_C2R

#define ippiYVToUY420_8u_P3C2R                 ippiYCrCb420ToCbYCr422_8u_P3C2R
#define ippiYVToYU420_8u_P3C2R                 ippiYCrCb420ToYCbCr422_8u_P3C2R

#define ippiYVToYU420_8u_P3P2R                 ippiYCrCb420ToYCbCr420_8u_P3P2R
#define ippiYUToUY420_8u_P2C2R                 ippiYCbCr420ToCbYCr422_8u_P2C2R

#define ippiSplit420_8u_P2P3R                  ippiYCbCr420ToYCrCb420_8u_P2P3R
#define ippiYCbCr420ToYCbCr420_8u_P2P3R        ippiYCbCr420_8u_P2P3R

#define ippiYCbCr420ToYCbCr420_8u_P3P2R        ippiYCbCr420_8u_P3P2R
#define ippiYCbCr411ToYCbCr411_8u_P3P2R        ippiYCbCr411_8u_P3P2R
#define ippiYCbCr411ToYCbCr411_8u_P2P3R        ippiYCbCr411_8u_P2P3R

#define ippiYCbCr420ToYCbCr422Filter_8u_P3R    ippiYCbCr420ToYCbCr422_Filter_8u_P3R
#define ippiYCrCb420ToYCbCr422Filter_8u_P3R    ippiYCrCb420ToYCbCr422_Filter_8u_P3R
#define ippiYCbCr420ToYCbCr422Filter_8u_P2P3R  ippiYCbCr420ToYCbCr422_Filter_8u_P2P3R

#define ippsAdd_32s(PTR_SRC1, PTR_SRC2, PTR_DST, LEN) ippsAdd_32s_Sfs(PTR_SRC1, PTR_SRC2, PTR_DST, LEN, 0)
#define ippsAdd_32sc(PTR_SRC1, PTR_SRC2, PTR_DST, LEN) ippsAdd_32sc_Sfs(PTR_SRC1, PTR_SRC2, PTR_DST, LEN, 0)
#define ippsAddC_16s(PTR_SRC, VAL, PTR_DST, LEN) ippsAddC_16s_Sfs(PTR_SRC, VAL, PTR_DST, LEN, 0)

#define ippsSub_32s(PTR_SRC1, PTR_SRC2, PTR_DST, LEN) ippsSub_32s_Sfs(PTR_SRC1, PTR_SRC2, PTR_DST, LEN, 0)
#define ippsSub_32sc(PTR_SRC1, PTR_SRC2, PTR_DST, LEN) ippsSub_32sc_Sfs(PTR_SRC1, PTR_SRC2, PTR_DST, LEN, 0)
#define ippsSubC_16s(PTR_SRC, VAL, PTR_DST, LEN) ippsSubC_16s_Sfs(PTR_SRC, VAL, PTR_DST, LEN, 0)

#define ippsMul_32s(PTR_SRC1, PTR_SRC2, PTR_DST, LEN) ippsMul_32s_Sfs(PTR_SRC1, PTR_SRC2, PTR_DST, LEN, 0)
#define ippsMul_32sc(PTR_SRC1, PTR_SRC2, PTR_DST, LEN) ippsMul_32sc_Sfs(PTR_SRC1, PTR_SRC2, PTR_DST, LEN, 0)
#define ippsMulC_16s(PTR_SRC, VAL, PTR_DST, LEN) ippsMulC_16s_Sfs(PTR_SRC, VAL, PTR_DST, LEN, 0)

#define ippsSqr_16s(PTR_SRC,PTR_DST,LEN)    ippsSqr_16s_Sfs(PTR_SRC,PTR_DST,LEN,0)
#define ippsSqr_16s_I(PTR_SRCDST,LEN)       ippsSqr_16s_ISfs(PTR_SRCDST,LEN,0)
#define ippsSqrt_16s(PTR_SRC,PTR_DST,LEN)   ippsSqrt_16s_Sfs(PTR_SRC,PTR_DST,LEN,0)
#define ippsSqrt_16s_I(PTR_SRCDST,LEN)      ippsSqrt_16s_ISfs(PTR_SRCDST,LEN,0)
#define ippsMean_16s(PTR_SRC,LEN,PTR_DST)   ippsMean_16s_Sfs(PTR_SRC,LEN,PTR_DST,0)
#define ippsStdDev_16s(PTR_SRC,LEN,PTR_DST) ippsStdDev_16s_Sfs(PTR_SRC,LEN,PTR_DST,0)

#define ipps10Log10_32s_I( pSrcDst, len)   ipps10Log10_32s_ISfs(pSrcDst, len, 0)
#define ippsExp_16s_I( pSrcDst, len )      ippsExp_16s_ISfs(pSrcDst, len, 0)
#define ippsLn_16s_I( pSrcDst, len )       ippsLn_16s_ISfs(pSrcDst, len, 0)
#define ippsLn_32s_I( pSrcDst, len )       ippsLn_32s_ISfs(pSrcDst, len, 0)
#define ipps10Log10_32s( pSrc, pDst, len ) ipps10Log10_32s_Sfs(pSrc, pDst, len, 0)
#define ippsExp_16s( pSrc, pDst, len )     ippsExp_16s_Sfs(pSrc, pDst, len, 0)
#define ippsLn_16s( pSrc, pDst, len )      ippsLn_16s_Sfs(pSrc, pDst, len, 0)
#define ippsLn_32s( pSrc, pDst, len )      ippsLn_32s_Sfs(pSrc, pDst, len, 0)
#define ippsNormalize_16s( pSrc, pDst, len, offset, normFactor) ippsNormalize_16s_Sfs( pSrc, pDst, len, offset, normFactor, 0)
#define ippsAbs_32sc32s(pSrc, pDst, len) ippsMagnitude_32sc_Sfs(pSrc, pDst, len, 0)
#define ippsAbs_32sc32s_Sfs ippsMagnitude_32sc_Sfs
#define ippsSum_32s(pSrc,len,pSum) ippsSum_32s_Sfs(pSrc,len,pSum,0)
#define ippsDotProd_16s(pSrc1,pSrc2,len,pDp)\
        ippsDotProd_16s_Sfs(pSrc1,pSrc2,len,pDp,0)

#define ippiDCTQuantFwdTableInit_JPEG_8u16u ippiQuantFwdTableInit_JPEG_8u16u
#define ippiDCTQuantInvTableInit_JPEG_8u16u ippiQuantInvTableInit_JPEG_8u16u
#define ippiDCTQuantFwd_JPEG_16s ippiDCTQuantFwd8x8_JPEG_16s_C1
#define ippiDCTQuantFwd_JPEG_16s_I ippiDCTQuantFwd8x8_JPEG_16s_C1I
#define ippiDCTQuantInv_JPEG_16s ippiDCTQuantInv8x8_JPEG_16s_C1
#define ippiDCTQuantInv_JPEG_16s_I ippiDCTQuantInv8x8_JPEG_16s_C1I

#define ippsFIR_Direct_16s( pSrc, pDst, sampLen, pTapsQ15, tapsLen,\
                                                 pDelayLine, pDelayLineIndex )\
        ippsFIR_Direct_16s_Sfs( pSrc, pDst, sampLen, pTapsQ15, tapsLen,\
                                               pDelayLine, pDelayLineIndex, 0 )
#define ippsFIR_Direct_16s_I( pSrcDst, sampLen, pTapsQ15, tapsLen, pDelayLine,\
                                                             pDelayLineIndex )\
        ippsFIR_Direct_16s_ISfs( pSrcDst, sampLen, pTapsQ15, tapsLen,\
                                               pDelayLine, pDelayLineIndex, 0 )
#define ippsFIROne_Direct_16s( val, pResult, pTapsQ15, tapsLen, pDelayLine,\
                                                             pDelayLineIndex )\
        ippsFIROne_Direct_16s_Sfs( val, pResult, pTapsQ15, tapsLen,\
                                               pDelayLine, pDelayLineIndex, 0 )
#define ippsFIROne_Direct_16s_I( pValResult, pTapsQ15, tapsLen, pDelayLine,\
                                                             pDelayLineIndex )\
        ippsFIROne_Direct_16s_ISfs( pValResult, pTapsQ15, tapsLen, pDelayLine,\
                                                           pDelayLineIndex, 0 )


#define ippsMagSquared_32sc32s( pSrc,  pDst,  len)\
        ippsMagSquared_32sc32s_Sfs( pSrc,  pDst,  len, 0)

#define ippsSubCRev_16s_I(VAL, PTR_SRCDST, LEN) ippsSubCRev_16s_ISfs(VAL, PTR_SRCDST, LEN, 0)

#define ippsDecodeChanPairElt_MPEG4_AAC ippsDecodeChanPairElt_MP4_AAC

#define ippiResizeCscRotate_8u_C2R( pSrc,srcStep, pDst, dstStep, roiSize, scaleFactor,\
        interpolation, colorConversion, rotation ) \
        ippiResizeCCRotate_8u_C2R( pSrc, srcStep, roiSize, pDst, dstStep, scaleFactor,\
        interpolation, colorConversion, rotation )
#define ippiYCbCr422ToYCbCr420Rotate_8u_C2P3R( pSrc, srcStep, pDst, dstStep, roiSize, rotation )\
        ippiCbYCr422ToYCbCr420_Rotate_8u_C2P3R( pSrc, srcStep, roiSize, pDst, dstStep, rotation )
#define ippiYCbCr422ToYCbCr420Rotate_8u_P3R( pSrc, srcStep, pDst, dstStep, roiSize, rotation )\
        ippiCbYCr422ToYCbCr420_Rotate_8u_P3R( pSrc, srcStep, roiSize, pDst, dstStep, rotation )
#define ippsDivQ15_32s( pSrc1, pSrc2, pDst,len) ippsDiv_32s_Sfs( pSrc1, pSrc2, pDst, len, -15 )
#define ippsDivQ15_32s_I( pSrc, pSrcDst, len ) ippsDiv_32s_ISfs( pSrc, pSrcDst, len, -15 )
#define IppMP3EncPsychoAcousticModel2State IppMP3PsychoacousticModelTwoState

/* /////////////////////////////////////////////////////////////////////////////
//                       Wrappres
//
*/
IPPAPI( IppStatus, ippsUpSampleSize,
                   ( int srcLen, int factor, int phase,  int* pDstLen ))
IPPAPI( IppStatus, ippsUpSample_16s, (const Ipp16s* pSrc, int srcLen,
                                            int* pPhase, Ipp16s* pDst,
                                            int factor))
IPPAPI( IppStatus, ippsDownSampleSize,
                   ( int srcLen, int factor, int phase,  int* pDstLen ))
IPPAPI( IppStatus, ippsDownSample_16s, (const Ipp16s* pSrc, int srcLen,
                                              int* pPhase, Ipp16s* pDst,
                                              int factor))


IPPAPI(IppStatus, ippsNorm_Inf_16s32s, (const Ipp16s* pSrc, int len, Ipp32s* pNorm))

IPPAPI(IppStatus, ippsNorm_L1_16s32s, (const Ipp16s* pSrc, int len, Ipp32s* pNorm))

IPPAPI( IppStatus, ippsNorm_L2_16s32s, (const Ipp16s* pSrc, int len, Ipp32s* pNorm))

IPPAPI( IppStatus, ippsNormDiff_Inf_16s32s, (const Ipp16s* pSrc1, const Ipp16s* pSrc2,
                                            int len, Ipp32s* pNorm))

IPPAPI( IppStatus, ippsNormDiff_L1_16s32s, (const Ipp16s* pSrc1, const Ipp16s* pSrc2,
                                           int len, Ipp32s* pNorm))

IPPAPI( IppStatus, ippsNormDiff_L2_16s32s, (const Ipp16s* pSrc1, const Ipp16s* pSrc2,
                                           int len, Ipp32s* pNorm))


IPPAPI( IppStatus, ippiNorm_Inf_8u_C1RSfs, (const Ipp8u* pSrc, int srcStep,
                                           IppiSize roiSize, Ipp32s* pNorm, int sFactor))

IPPAPI( IppStatus, ippiNorm_Inf_8u_C3RSfs, (const Ipp8u* pSrc, int srcStep,
                                           IppiSize roiSize, Ipp32s pNorm[3], int sFactor))

IPPAPI( IppStatus, ippiNorm_L1_8u_C1RSfs, (const Ipp8u* pSrc, int srcStep,
                                          IppiSize roiSize, Ipp32s* pNorm, int sFactor))

IPPAPI( IppStatus, ippiNorm_L1_8u_C3RSfs, (const Ipp8u* pSrc, int srcStep,
                                          IppiSize roiSize, Ipp32s pNorm[3], int sFactor))

IPPAPI( IppStatus, ippiNorm_L2_8u_C1RSfs, (const Ipp8u* pSrc, int srcStep,
                                          IppiSize roiSize, Ipp32s* pNorm, int sFactor))

IPPAPI( IppStatus, ippiNorm_L2_8u_C3RSfs, (const Ipp8u* pSrc, int srcStep,
                                          IppiSize roiSize, Ipp32s pNorm[3], int sFactor))

IPPAPI( IppStatus, ippiNormDiff_Inf_8u_C1RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s* pNorm, int sFactor))

IPPAPI( IppStatus, ippiNormDiff_Inf_8u_C3RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s pNorm[3], int sFactor))

IPPAPI( IppStatus, ippiNormDiff_L1_8u_C1RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s* pNorm, int sFactor))

IPPAPI( IppStatus, ippiNormDiff_L1_8u_C3RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s pNorm[3], int sFactor))

IPPAPI( IppStatus, ippiNormDiff_L2_8u_C1RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s* pNorm, int sFactor))

IPPAPI( IppStatus, ippiNormDiff_L2_8u_C3RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s pNorm[3], int sFactor))

IPPAPI( IppStatus, ippiNormRel_Inf_8u_C1RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s* pNorm, int sFactor))

IPPAPI( IppStatus, ippiNormRel_Inf_8u_C3RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s pNorm[3], int sFactor))

IPPAPI( IppStatus, ippiNormRel_L1_8u_C1RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s* pNorm, int sFactor))

IPPAPI( IppStatus, ippiNormRel_L1_8u_C3RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s pNorm[3], int sFactor))

IPPAPI( IppStatus, ippiNormRel_L2_8u_C1RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s* pNorm, int sFactor))

IPPAPI( IppStatus, ippiNormRel_L2_8u_C3RSfs, (const Ipp8u* pSrc1, int src1Step,
                                               const Ipp8u* pSrc2, int src2Step,
                                               IppiSize roiSize, Ipp32s pNorm[3], int sFactor))


IPPAPI(IppStatus, ippsAutoCorr_16s,( const Ipp16s* pSrc, int srcLen,
                                                    Ipp16s* pDst, int dstLen ))
IPPAPI(IppStatus, ippsAutoCorr_NormA_16s,( const Ipp16s* pSrc, int srcLen,
                                                    Ipp16s* pDst, int dstLen ))
IPPAPI(IppStatus, ippsAutoCorr_NormB_16s,( const Ipp16s* pSrc, int srcLen,
                                                    Ipp16s* pDst, int dstLen ))

IPPAPI( IppStatus, ippsConv_16s,( const Ipp16s* pSrc1, int lenSrc1,
                              const Ipp16s* pSrc2, int lenSrc2, Ipp16s* pDst ))

IPPAPI( IppStatus, ippsCrossCorr_16s,( const Ipp16s* pSrc1, int srcLen1,
      const Ipp16s* pSrc2, int srcLen2, Ipp16s* pDst, int dstLen, int lowLag ))

IPPAPI( IppStatus, ippsFIRLMSOne_Direct_16s,( Ipp16s src, Ipp16s ref,
                        Ipp16s* pDstVal, Ipp32s* pTaps, int tapsLen, int muQ15,
                                            Ipp16s* pDlyLine, int* pDlyIndex ))

IPPAPI( IppStatus, ippsFIRLMSOne_Direct_16s_I,( Ipp16s* pSrcDst, Ipp16s ref,
     Ipp32s* pTaps, int tapsLen, int muQ15, Ipp16s* pDlyLine, int* pDlyIndex ))

IPPAPI( IppStatus, ippsAddWeightedQ31_32s_I,( const Ipp32s *pSrcOld,
                        Ipp32s *pSrcpDst, int len, Ipp32s weightQ31 ))

IPPAPI( IppStatus, ippsAddWeightedQ31_32s,( const Ipp32s *pSrcOld,
                        const Ipp32s *pSrcNew, Ipp32s *pDst,
                        int len, Ipp32s weightQ31 ))


/*--------------------- Video Decode/Encode -----------------------------*/

/* ippvc.h used only for IppMotionVector type */
#include "ippvc.h"


/* Video Components */
typedef enum
{
  IPP_VIDEO_LUMINANCE,        /* Luminance component   */
  IPP_VIDEO_CHROMINANCE,      /* Chrominance component */
  IPP_VIDEO_ALPHA             /* Alpha component       */

} IppVideoComponent;


/* Macroblock Types */
typedef enum
{
  IPP_VIDEO_INTER        = 0,    /* P picture or P-VOP */
  IPP_VIDEO_INTER_Q      = 1,    /* P picture or P-VOP */
  IPP_VIDEO_INTER4V      = 2,    /* P picture or P-VOP */
  IPP_VIDEO_INTRA        = 3,    /* I and P picture, or I- and P-VOP */
  IPP_VIDEO_INTRA_Q      = 4,    /* I and P picture, or I- and P-VOP */
  IPP_VIDEO_INTER4V_Q    = 5,    /* P picture or P-VOP(H.263)*/
  IPP_VIDEO_DIRECT       = 6,    /* B picture or B-VOP (MPEG-4 only) */
  IPP_VIDEO_INTERPOLATE  = 7,    /* B picture or B-VOP */
  IPP_VIDEO_BACKWARD     = 8,    /* B picture or B-VOP */
  IPP_VIDEO_FORWARD      = 9     /* B picture or B-VOP */

} IppMacroblockType;


/* Transparent Status */
enum
{
  IPP_VIDEO_TRANSPARENT  = 0,
  IPP_VIDEO_PARTIAL      = 1,
  IPP_VIDEO_OPAQUE       = 2
};


/* Direction */
enum
{
  IPP_VIDEO_NONE         = 0,
  IPP_VIDEO_HORIZONTAL   = 1,
  IPP_VIDEO_VERTICAL     = 2,
  IPP_VIDEO_DCONLY       = 3
};


enum
{
  IPP_DCScalerLinear     = 0,
  IPP_DCScalerNonLinear  = 1
};


typedef struct _IppCoordinate
{
     Ipp32s  x;
     Ipp32s  y;
} IppCoordinate;


typedef struct
{
     Ipp32s left;
     Ipp32s top;
     Ipp32s right;
     Ipp32s bottom;
} IppiRegion;


typedef enum _BlockNum
{
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


/* bilinear interpolation type */
enum
{
  IPP_VIDEO_INTEGER_PIXEL   = 0,
  IPP_VIDEO_HALF_PIXEL_X    = 1,
  IPP_VIDEO_HALF_PIXEL_Y    = 2,
  IPP_VIDEO_HALF_PIXEL_XY   = 3
};


typedef enum _BAB_TYPE
{
  MVDZ_NOUPDT     = 0,
  MVDNZ_NOUPDT    = 1,
  ALL_TRANSP      = 2,
  ALL_OPAQUE      = 3,
  INTRA_CAE       = 4,
  INTER_CAE_MVDZ  = 5,
  INTER_CAE_MVDNZ = 6
} IppBABType;



/* rate control */
typedef struct
{
  Ipp32u  exp;
  Ipp32u  frac;
} Ipp32uc;


/*  MPEG4 Rate Control Method */
#define RC_MPEG4              1
#define RC_TM5                3

#define RC_START_RATE_CONTROL 1
#define RC_MAX_SLIDING_WINDOW 20
#define RC_MAX_QUANT          31
#define RC_MIN_QUANT          1


typedef struct
{
  /*
  // X1 and X2 are the first and second order coefficients of the rate
  // distortion function
  */
  Ipp32uc x1;
  Ipp32uc x2;
  Ipp32u  rs; /* bit rate for sequence. e.g. 24000 bits/sec */
  Ipp32u  rf; /* bits used for the first frame, e.g. 10000 bits */
  Ipp32u  rc; /* bits used for the current frame after encoding */
  Ipp32u  rp; /* bits to be removed from the buffer per picture */
  int   ts;   /* number of seconds for the sequence, e.g. 10 sec */

  Ipp32uc ec; /* mean absolute difference for the current frame after motion compensation */
  Ipp32uc ep; /* mean absolute difference for the previous frame after motion compensation */

  Ipp32u  qc; /* quantization level used for the current frame */
  Ipp32u  qp; /* quantization level used for the previous frame */
  Ipp32u  nr; /* number of P frames remaining for encoding */
  Ipp32u  nc; /* number of P frames coded */
  Ipp32u  ns; /* distance between encoded frames */
  int     rr; /* number of bits remaining for encoding this sequence */
  Ipp32u  t;  /* target bit to be used for the current frame */
  Ipp32u  s;  /* number of bits used for encoding the previous frame */
  Ipp32u  hc; /* header and motion vector bits used in the current frame */
  Ipp32u  hp; /* header and motion vector bits used in the previous frame */
  Ipp32u  bs; /* buffer size */
  int     bl; /* current buffer level */
  int     skipNextFrame;                 /* TRUE if buffer is full */
  int     lastFrame;                     /* only used for RC */
  Ipp32u  wQp[RC_MAX_SLIDING_WINDOW];    /* quantization levels for the past frames */
  Ipp32uc wRp[RC_MAX_SLIDING_WINDOW];    /* scaled encoding complexity used for the past frames */
  int     wRejected[RC_MAX_SLIDING_WINDOW]; /* outliers */
  int     vopSize;

} IppMP4RCStatus;



/* ---------------------------------------------------------------------*/

#define ippiDCT8x8Inv_Video_16s_C1(pSrc,pDst) ippiDCT8x8Inv_16s_C1(pSrc,pDst)
#define ippiDCT8x8Inv_Video_16s_C1I(pSrcDst) ippiDCT8x8Inv_16s_C1I(pSrcDst)
#define ippiDCT8x8Inv_Video_16s8u_C1R(pSrc,pDst,dstStep) ippiDCT8x8Inv_16s8u_C1R(pSrc,pDst,dstStep)

#define ippiDCT8x8Fwd_Video_16s_C1(pSrc,pDst) ippiDCT8x8Fwd_16s_C1(pSrc,pDst)
#define ippiDCT8x8Fwd_Video_16s_C1I(pSrcDst) ippiDCT8x8Fwd_16s_C1I(pSrcDst)
#define ippiDCT8x8Fwd_Video_8u16s_C1R(pSrc,srcStep,pDst) ippiDCT8x8Fwd_8u16s_C1R(pSrc,srcStep,pDst)

#define ippiPredictBlock_OBMC_8u(pSrc, pDst, step, pMVCur, pMVLeft, pMVRight, pMVAbove, pMVBelow) \
    ippiOBMC8x8HP_MPEG4_8u_C1R(pSrc, step, pDst, step, pMVCur, pMVLeft, pMVRight, pMVAbove, pMVBelow, 0)

#define ippiCopyBlock_8x8_8u(pSrc, srcStep, pDst, dstStep) \
    ippiCopy8x8_8u_C1R(pSrc, srcStep, pDst, dstStep)

#define ippiCopyBlock_16x16_8u(pSrc, srcStep, pDst, dstStep) \
    ippiCopy16x16_8u_C1R(pSrc, srcStep, pDst, dstStep)

#define ippiReconBlock_8x8(pSrc, srcStep, pResidual, pDst, dstStep) \
    ippiAdd8x8HP_16s8u_C1RS(pResidual, 16, pSrc, srcSep, pDst, dstStep, 0, 0)

#define ippiCopyMB_H263_8u(pSrc, pDst, step) \
    ippiCopy16x16_8u_C1R(pSrc, step, pDst, step)

#define ippiCopyBlock_H263_8u(pSrc, pDst, step) \
    ippiCopy8x8_8u_C1R(pSrc, step, pDst, step)

#define ippiReconMB_H263(pSrc, pResidual, pDst, step) \
    ippiMC16x16_8u_C1(pSrc, step, pResidual, 16, pDst, step, IPPVC_MC_APX_FF, 0)

#define ippiReconMB_H263_I(pSrcDst, pResidual, step) \
    ippiMC16x16_8u_C1(pSrcDst, step, pResidual, 16, pSrcDst, step, IPPVC_MC_APX_FF, 0)

#define ippiReconBlock_H263(pSrc, pResidual, pDst, step) \
    ippiAdd8x8HP_16s8u_C1RS(pResidual, 16, pSrc, step, pDst, step, 0, 0)

#define ippiReconBlock_H263_I(pSrcDst, pResidual, step) \
    ippiAdd8x8_16s8u_C1IRS(pResidual, 16, pSrcDst, step)

#define ippiMCBlock_RoundOff_8u(pSrc, srcStep, pDst, dstStep, predictType) \
    ippiCopy8x8HP_8u_C1R(pSrc, srcStep, pDst, dstStep, predictType, 0)

#define  ippiMCBlock_RoundOn_8u(pSrc, srcStep, pDst, dstStep, predictType) \
    ippiCopy8x8HP_8u_C1R(pSrc, srcStep, pDst, dstStep, predictType, 1)

#define ippiMCReconBlock_RoundOn(pSrc, srcStep, pResidue, pDst, dstStep, predictType) \
    ippiAdd8x8HP_16s8u_C1RS(pResidue, 16, pSrc, srcStep, pDst, dstStep, predictType, 0)

#define ippiMCReconBlock_RoundOff(pSrc, srcStep, pResidue, pDst, dstStep, predictType) \
    ippiAdd8x8HP_16s8u_C1RS(pResidue, 16, pSrc, srcStep, pDst, dstStep, predictType, 1)

#define ippiFilterDeblocking_HorEdge_H263_8u_I(pSrcDst, step, QP) \
    ippiFilterDeblocking8x8HorEdge_H263_8u_C1IR(pSrcDst, step, QP)

#define ippiFilterDeblocking_VerEdge_H263_8u_I(pSrcDst, step, QP) \
    ippiFilterDeblocking8x8VerEdge_H263_8u_C1IR(pSrcDst, step, QP)

#define ippiZigzagInvClassical_Compact_16s(pSrc, len, pDst) \
    ippiScanInv_16s_C1(pSrc, pDst, (len) - 1, IPPVC_SCAN_ZIGZAG)

#define ippiZigzagInvHorizontal_Compact_16s(pSrc, len, pDst) \
    ippiScanInv_16s_C1(pSrc, pDst, (len) - 1, IPPVC_SCAN_HORIZONTAL)

#define ippiZigzagInvVertical_Compact_16s(pSrc, len, pDst) \
    ippiScanInv_16s_C1(pSrc, pDst, (len) - 1, IPPVC_SCAN_VERTICAL)

#define ippiQuantInvIntra_Compact_H263_16s_I(pSrcDst, len, QP) \
    ippiQuantInvIntra_H263_16s_C1I(pSrcDst, (len) - 1, QP, 0, 0)

#define ippiQuantInvInter_Compact_H263_16s_I(pSrcDst, len, QP) \
    ippiQuantInvInter_H263_16s_C1I(pSrcDst, (len) - 1, QP, 0)

#define ippiComputeTextureErrorBlock_SAD_8u16s(pSrc, srcStep, pSrcRef, pDst, pDstSAD) \
    ippiSubSAD8x8_8u16s_C1R(pSrc, srcStep, pSrcRef, 8, pDst, 16, pDstSAD)

#define ippiComputeTextureErrorBlock_8u16s(pSrc, srcStep, pSrcRef, pDst) \
    ippiSub8x8_8u16s_C1R(pSrc, srcStep, pSrcRef, 8, pDst, 16)


/* ---------------------------------------------------------------------*/

#define ippiCopyApproxHMB_H263_8u(pSrc, pDst, step) \
    ippiCopy16x16HP_8u_C1R(pSrc, step, pDst, step, 1, 0)

#define ippiCopyApproxHBlock_H263_8u(pSrc, pDst, step) \
    ippiCopy8x8HP_8u_C1R(pSrc, step, pDst, step, 1, 0)

#define ippiCopyApproxVMB_H263_8u(pSrc, pDst, step) \
    ippiCopy16x16HP_8u_C1R(pSrc, step, pDst, step, 2, 0)

#define ippiCopyApproxVBlock_H263_8u(pSrc, pDst, step) \
    ippiCopy8x8HP_8u_C1R(pSrc, step, pDst, step, 2, 0)

#define ippiCopyApproxHVMB_H263_8u(pSrc, pDst, step) \
    ippiCopy16x16HP_8u_C1R(pSrc, step, pDst, step, 3, 0)

#define ippiCopyApproxHVBlock_H263_8u(pSrc, pDst, step) \
    ippiCopy8x8HP_8u_C1R(pSrc, step, pDst, step, 3, 0)

#define ippiQuantInvIntra_H263_C1I(pSrcDst, QP) \
    ippiQuantInvIntra_H263_16s_C1I(pSrcDst, 63, QP, 0, 0)

#define ippiQuantInv_H263_C1I(pSrcDst, QP) \
    ippiQuantInvInter_H263_16s_C1I(pSrcDst, 63, QP, 0)

#define ippiDCTInv_8x8_16s8u(pSrc, pDst, dstStep) \
    ippiDCT8x8Inv_16s8u_C1R(pSrc, pDst, dstStep)

#define ippiZigzagInv_Horizontal_16s(pSrc, pDst) \
    ippiScanInv_16s_C1(pSrc, pDst, 63, IPPVC_SCAN_HORIZONTAL)

#define ippiZigzagInv_Vertical_16s(pSrc, pDst) \
    ippiScanInv_16s_C1(pSrc, pDst, 63, IPPVC_SCAN_VERTICAL)

#define ippiCopyBlockHalfpel_MPEG4_8u(pSrc, srcStep, pDst, dstStep, pMV, roundControl) \
    ippiCopy8x8HP_8u_C1R(pSrc+((pMV)->dy>>1)*(srcStep)+((pMV)->dx>>1), srcStep, pDst, dstStep, ((pMV)->dy&1)*2+((pMV)->dx&1), roundControl)

#define ippiCopyMBHalfpel_MPEG4_8u(pSrc, srcStep, pDst, dstStep, pMV, roundControl) \
    ippiCopy16x16HP_8u_C1R(pSrc+((pMV)->dy>>1)*(srcStep)+((pMV)->dx>>1), srcStep, pDst, dstStep, ((pMV)->dy&1)*2+((pMV)->dx&1), roundControl)

#define ippiReconBlockHalfpel_MPEG4_8u(pSrc, srcStep, pResidue, pDst, dstStep, pMV, roundControl) \
    ippiAdd8x8HP_16s8u_C1RS(pResidue, 16, pSrc+((pMV)->dy>>1)*(srcStep)+((pMV)->dx>>1), srcStep, pDst, dstStep, ((pMV)->dy&1)*2+((pMV)->dx&1), roundControl)

#define ippiOBMCHalfpel_MPEG4_8u(pSrc, srcStep, pDst, dstStep, pMVCur, pMVLeft, pMVRight, pMVAbove, pMVBelow, roundControl) \
    ippiOBMC8x8HP_MPEG4_8u_C1R(pSrc, srcStep, pDst, dstStep, pMVCur, pMVLeft, pMVRight, pMVAbove, pMVBelow, roundControl)

#define ippiFilterDeblocking_HorEdge_MPEG4_8u_I(pSrcDst, step, QP, THR1, THR2) \
    ippiFilterDeblocking8x8HorEdge_MPEG4_8u_C1IR(pSrcDst, step, QP, THR1, THR2)

#define ippiFilterDeblocking_VerEdge_MPEG4_8u_I(pSrcDst, step, QP, THR1, THR2) \
    ippiFilterDeblocking8x8VerEdge_MPEG4_8u_C1IR(pSrcDst, step, QP, THR1, THR2)

#define ippiFilterDeringingThresholdMB_MPEG4_8u(pSrcY, stepY, pSrcCb, stepCb, pSrcCr, stepCr, threshold) \
    ippiFilterDeringingThreshold_MPEG4_8u_P3R(pSrcY, stepY, pSrcCb, stepCb, pSrcCr, stepCr, threshold)

#define ippiFilterDeringingSmoothBlock_MPEG4_8u(pSrc, srcStep, pDst, dstStep, QP, threshold) \
    ippiFilterDeringingSmooth8x8_MPEG4_8u_C1R(pSrc, srcStep, pDst, dstStep, QP, threshold)

#define ippiDecodeVLC_IntraDCVLC_MPEG4_1u16s ippiDecodeDCIntra_MPEG4_1u16s

#define ippiEncode_IntraDCVLC_MPEG4_16s1u ippiEncodeDCIntra_MPEG4_16s1u

#define ippiEncode_IntraDCVLC_H263_16s1u ippiEncodeDCIntra_H263_16s1u



/* ---------------------------------------------------------------------*/


IPPAPI(IppStatus, ippiDecodeMV_H263,
    (Ipp8u ** ppBitStream, int * pBitOffset, IppMotionVector * pSrcDstMV))

IPPAPI(IppStatus, ippiDecodeMV_TopBorder_H263,
    (Ipp8u ** ppBitStream, int * pBitOffset, IppMotionVector * pSrcDstMV))

IPPAPI(IppStatus, ippiExpandFrame_H263_8u,
    (Ipp8u * pSrcDstPlane, int frameWidth, int frameHeight, int expandPels, int step))

IPPAPI(IppStatus, ippiDecodeBlockCoef_Intra_H263_1u8u,
    (Ipp8u ** ppBitStream, int * pBitOffset, Ipp8u * pDst, int step, int QP))

IPPAPI(IppStatus, ippiDecodeBlockCoef_Inter_H263_1u16s,
    (Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pDst, int QP))

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

IPPAPI(IppStatus, ippiPredictReconCoefIntra_MPEG4_16s,
    (Ipp16s * pSrcDst, Ipp16s * pPredBufRow, Ipp16s * pPredBufCol,
     int curQP, int predQP, int predDir, int ACPredFlag, IppVideoComponent videoComp))

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

IPPAPI(IppStatus, ippiPadCurrent_16x16_MPEG4_8u_I, (
        Ipp8u* pSrcDst,
        int    step,
  const Ipp8u* pBAB))

IPPAPI(IppStatus, ippiPadCurrent_8x8_MPEG4_8u_I, (
        Ipp8u* pSrcDst,
        int    step,
  const Ipp8u* pBAB))
IPPAPI(IppStatus, ippiPadMV_MPEG4,
    (IppMotionVector * pSrcDstMV, Ipp8u * pTransp))

IPPAPI(IppStatus, ippiQuantInvIntra_MPEG4_16s_I,
    (Ipp16s * pSrcDst, int QP, const Ipp8u * pQMatrix, IppVideoComponent videoComp))

IPPAPI(IppStatus, ippiQuantInvInter_MPEG4_16s_I,
    (Ipp16s * pSrcDst, int QP, const Ipp8u * pQMatrix))

IPPAPI(IppStatus, ippiDecodeVLCZigzag_IntraDCVLC_MPEG4_1u16s,
    (Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pDst, int predDir, IppVideoComponent videoComp))

IPPAPI(IppStatus, ippiDecodeVLCZigzag_IntraACVLC_MPEG4_1u16s,
    (Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pDst, int predDir))

IPPAPI(IppStatus, ippiDecodeVLCZigzag_Inter_MPEG4_1u16s,
    (Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pDst))

IPPAPI(IppStatus, ippiDecodeBlockCoef_Intra_MPEG4_1u8u,
    (Ipp8u ** ppBitStream, int * pBitOffset, Ipp8u * pDst,
     int step, Ipp16s * pCoefBufRow, Ipp16s * pCoefBufCol,
     Ipp8u curQP, Ipp8u * pQPBuf, const Ipp8u * pQMatrix,
     int blockIndex, int intraDCVLC, int ACPredFlag))

IPPAPI(IppStatus, ippiDecodeBlockCoef_Inter_MPEG4_1u16s,
    (Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pDst,
     int QP, const Ipp8u * pQMatrix))

IPPAPI (IppStatus,ippiDecodeMVS_MPEG4,
    (Ipp8u **ppBitStream, int *pBitOffset,  IppMotionVector * pSrcDstMVS,
    const Ipp8u * pSrcBABMode, int stepBABMode, const IppMotionVector   * pSrcMVLeftMB,
    const IppMotionVector * pSrcMVUpperMB,const IppMotionVector * pSrcMVUpperRightMB,
    const Ipp8u * pTranspLeftMB,const Ipp8u * pTranspUpperMB,
    const Ipp8u * pTranspUpperRightMB, int predFlag) )

IPPAPI(IppStatus, ippiDecodeCAEIntraH_MPEG4_1u8u, (Ipp8u ** ppBitStream,
        int * pBitOffset, Ipp8u * pBinarySrcDst, int step, int blocksize))

IPPAPI(IppStatus, ippiDecodeCAEIntraV_MPEG4_1u8u, (Ipp8u ** ppBitStream,
        int * pBitOffset, Ipp8u * pBinarySrcDst, int step, int blocksize))

IPPAPI(IppStatus, ippiDecodeCAEInterH_MPEG4_1u8u,   (Ipp8u ** ppBitStream,
        int * pBitOffset, const Ipp8u * pBinarySrcPred, int offsetPred, Ipp8u * pBinarySrcDst, int step, int blocksize))

IPPAPI(IppStatus, ippiDecodeCAEInterV_MPEG4_1u8u, (Ipp8u ** ppBitStream,
        int * pBitOffset, const Ipp8u * pBinarySrcPred, int offsetPred, Ipp8u * pBinarySrcDst, int step, int blocksize))

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

IPPAPI(IppStatus, ippiQuantInter_MPEG4_16s_I,
        (Ipp16s * pSrcDst, Ipp8u QP, const int * pQMatrix))

IPPAPI(IppStatus, ippiQuantIntra_MPEG4_16s_I,
       (Ipp16s * pSrcDst, Ipp8u QP, int blockIndex, const int * pQMatrix))

IPPAPI(IppStatus, ippiEncodeVLCZigzag_IntraDCVLC_MPEG4_16s1u, (Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s *pQDctBlkCoef, Ipp8u predDir,Ipp8u pattern, IppVideoComponent videoComp))
IPPAPI(IppStatus, ippiEncodeVLCZigzag_IntraACVLC_MPEG4_16s1u, (Ipp8u ** ppBitStream, int * pBitOffset, Ipp16s * pQDctBlkCoef, Ipp8u predDir,Ipp8u pattern))
IPPAPI(IppStatus, ippiEncodeVLCZigzag_Inter_MPEG4_16s1u,(Ipp8u **ppBitStream,int * pBitOffset, Ipp16s *pQDctBlkCoef,Ipp8u pattern))

IPPAPI(IppStatus, ippiTransRecBlockCoef_inter_MPEG4 , (Ipp16s *pSrc,
        Ipp16s * pDst,Ipp16s * pRec, Ipp8u QP, const int * pQMatrix))

IPPAPI(IppStatus, ippiTransRecBlockCoef_intra_MPEG4 , (Ipp8u *pSrc,Ipp16s * pDst,
        Ipp8u * pRec,Ipp16s *pPredBufRow,Ipp16s *pPredBufCol,Ipp16s * pPreACPredict,
        int *pSumErr,int blockIndex, Ipp8u QP, Ipp8u *pQpBuf, int srcStep, int dstStep,
        const int * pQMatrix))

IPPAPI(IppStatus, ippiFindMVpred_MPEG4,(IppMotionVector* pSrcMVCurMB,
    IppMotionVector* pSrcCandMV1,IppMotionVector* pSrcCandMV2,IppMotionVector* pSrcCandMV3,
    Ipp8u* pSrcCandTransp1,Ipp8u* pSrcCandTransp2,Ipp8u* pSrcCandTransp3,
    Ipp8u* pSrcTranspCurr, IppMotionVector* pDstMVPred, IppMotionVector* pDstMVPredME,
    int iBlk))

IPPAPI(IppStatus, ippiEncodeMV_MPEG4_8u16s, (Ipp8u **ppBitStream, int *pBitOffset,
    IppMotionVector* pMVCurMB,  IppMotionVector* pSrcMVLeftMB,
    IppMotionVector* pSrcMVUpperMB, IppMotionVector* pSrcMVUpperRightMB,
    Ipp8u* pTranspCurMB, Ipp8u* pTranspLeftMB, Ipp8u* pTranspUpperMB,
    Ipp8u* pTranspUpperRightMB, int fcodeForward, IppMacroblockType MBType))


/* ---------------------------------------------------------------------*/

IPPAPI(IppStatus, ippiAverageBlock_MPEG4_8u, (
  const Ipp8u*  pSrc1,
        int     src1Step,
  const Ipp8u*  pSrc2,
        int     src2Step,
        Ipp8u*  pDst,
        int     dstStep))

IPPAPI(IppStatus, ippiAverageMB_MPEG4_8u, (
  const Ipp8u*  pSrc1,
        int     src1Step,
  const Ipp8u*  pSrc2,
        int     src2Step,
        Ipp8u*  pDst,
        int     dstStep))

/* /////////////////////////////////////////////////////////////////////////////
//                  Definitions for Audio Toolkit functions
///////////////////////////////////////////////////////////////////////////// */
struct AECCtrlState;
struct AECCtrlState32f;

typedef struct AECCtrlState IppAECCtrlState;
typedef struct AECCtrlState32f IppAECCtrlState32f;

typedef struct IppAECScaled32s_Def  {
    Ipp32s  val;
    Ipp32s  sf;
} IppAECScaled32s;


typedef struct {
    Ipp16s *pMicrophone;     /* pointer to mic samples */
    Ipp16s *pLoudspeaker;    /* pointer to speaker samples */
    Ipp16s *pError;          /* pointer to error samples */
    Ipp32s *pAFInputPSD;     /* pointer to filter input PSD */
    Ipp32sc **ppAFCoefs;     /* pointer to filter segment array */
    Ipp32s muQ31;            /* fixed step size (Q31 value in (0,1)) */
    Ipp32s AECOutGainQ30;    /* AEC output gain (Q30 value in [0,1]) */
    Ipp32s speakerGainQ30;   /* loudspeaker gain (Q30 value in [0,1]) */
    int numSegments;         /* number of segments of filter tail */
    int numFFTBins;          /* number of FFT bins (FFTSize / 2 + 1) */
    int numSamples;          /* mic, error, and loudspeaker frame size */
    int sampleRate;          /* sample rate */
} IppAECNLMSParam;

typedef struct {
    Ipp32f *pMicrophone;     /* pointer to mic samples */
    Ipp32f *pLoudspeaker;    /* pointer to speaker samples */
    Ipp32f *pError;          /* pointer to error samples */
    Ipp32f *pAFInputPSD32f;  /* pointer to filter input PSD32f */
    Ipp32fc **ppAFCoefs32f;  /* pointer to filter segment array */
    Ipp32f mu;               /* fixed step size (value in (0,1)) */
    Ipp32f AECOutGain;       /* AEC output gain (Q30 value in [0,1]) */
    Ipp32f speakerGain;      /* loudspeaker gain (Q30 value in [0,1]) */
    int numSegments;         /* number of segments of filter tail */
    int numFFTBins;          /* number of FFT bins (FFTSize / 2 + 1) */
    int numSamples;          /* mic, error, and loudspeaker frame size */
    int sampleRate;          /* sample rate */
} IppAECNLMSParam32f;

/* /////////////////////////////////////////////////////////////////////////////
//                  Definitions of Audio Toolkit functions
///////////////////////////////////////////////////////////////////////////// */

/********************************************************************************
// Name:            IppStatus ippsControllerGetSizeAEC_32s(int *pDstSize)
//
// Description:  This function returns the controller state size in bytes.
//
// Input Arguments:  none
//
// Output Arguments: pDstSize - pointer to int that will hold size
//
// Returns:
//                    IppStsNoErr            - No Error
//                    IppStsBadArgErr        - Bad Arguments
//
// Notes:
********************************************************************************/
IPPAPI(    IppStatus, ippsControllerGetSizeAEC_32s,(int *pDstSize) )

/********************************************************************************
// Name:            IppStatus ippsControllerInitAEC_32s(const IppAECNLMSParam *pSrcParams,
//                                                IppAECCtrlState *pDstState)
//
// Description:  This function initializes the AEC controller state.  Energies
//               are initialized to zero.  Counters are initialized to zero.
//               The default recursive energy smoothing parameters and tracking
//               window size have been designed for a sample rate (Fs) of 8000
//               and a frame update period (N) of 64 samples or 8 milliseconds.
//               These are adjusted as described below for other sample rates
//               and update periods.
//
// Input Arguments:    pSrcParams - pointer to AEC parameters structure
//
// Output Arguments: pDstState - pointer to state structure
//
// Returns:
//                    IppStsNoErr            - No Error
//                    IppStsBadArgErr        - Bad Arguments
//
// Notes:
********************************************************************************/
IPPAPI(    IppStatus, ippsControllerInitAEC_32s,(const IppAECNLMSParam *pSrcParams,
                                             IppAECCtrlState *pDstState) )
/********************************************************************************
// Name:            IppStatus ippsControllerUpdateAEC_32s(
//                                const IppAECNLMSParam *pSrcParams,
//                                Ipp32s *pDstMuQ31, Ipp32s *pDstAECOutGainQ31,
//                                Ipp32s *pDstSpeakerGainQ31)
//
// Description:        This function implements a rudimentary full-band energy-based
//                AEC controller.  The AEC controller adjusts the adaptive filter
//                step size (mu) such that the filter converges rapidly when the
//                loudspeaker signal is present at the microphone but does not
//                diverge quickly during double-talk or insufficient excitation
//                of the microphone by the loudspeaker.  The controller also
//                manages playback and output gain to block echo when the
//                adaptive filter has diverged.
//
//                The controller algorithm is described in detail in the Audio
//                Toolkit Reference Manual IPPAudioToolkitAPI.doc, section 2.1.2.
//                Briefly, the algorithm steps are as follows:
//
//                1. Compute microphone, loudspeaker, and error energies.
//                2. Apply recursive smoothing to calculate average energies.
//                3. Compute echo return loss enhancement (ERLE)
//                4. Update minimum microphone and loudspeaker energies within
//                   tracking window.
//                5. Update thresholds for ERLE, receive activity, mic activity,
//                   and no-double-talk.
//                6. Compare to thresholds and set composite condition index.
//                7. Perform state transitions based on state tables and
//                   condition index.
//                8. Update threshold for low ERLE.
//                9. Set AEC output and loudspeaker gains.
//                10. Update adaptive filter step size.
//
//                The composite event index pSrcDstState->iEvent is created by
//                combining the elementary events bConverged, bSpkActive,
//                bMicActive, and bNoDouble which are defined in the reference
//                manual.  The elementary event bERLElow (also defined in the
//                reference manual) is only used in the IDLE and DOUBLE states
//                to handle force exceptional state transitions (marked with a
//                "*" in the state transition table given in the manual).
//
//                State transitions as a function of the composite event index
//                are stored in tables, nextState<state>, where <state> is one
//                of {STARTUP, TRACK, IDLE, DOUBLE}.  The mu, alpha, and beta
//                settings for each state are defined in tables:
//                nextMuAdjust<state>, nextOutGainTarget<state>,
//                nextSpkGainTarget<state>.  The adaptive filter step size mu
//                is calculated by multiplying the fixed step size
//                pSrcDstState->stepSizeQ31 by the adjustment factor read
//                from the table.
//
//                Note that the AEC output and loudspeaker gains are not
//                applied instantaneously.  The instantaneous gains are
//                modified gradually to avoid audible flutter.  When one of
//                the gain targets changes, the instantaneous gain is
//                increased or decreased linearly to meet the target in
//                exactly 100 milliseconds.
//
// Input Arguments:    pSrcParams - pointer to AEC parameters structure
//
// Output Arguments: pDstMuQ31 - pointer to real-valued Q31 scalar with
//                               values between 0 and 1.
//                   pAECOutGainQ31 - pointer to real-valued Q31 scalar with
//                                    values between 0 and 1.
//                   pDstSpeakerGainQ31 - pointer to real-valued Q31 scalar with
//                                        values between 0 and 1.
//
// Returns:
//                    IppStsNoErr            - No Error
//                    IppStsBadArgErr        - Bad Arguments
//
// Notes:
********************************************************************************/
IPPAPI(IppStatus, ippsControllerUpdateAEC_32s,(const IppAECNLMSParam *pSrcParams,
  IppAECCtrlState *pSrcDstState, Ipp32s *pDstMuQ31, Ipp32s *pDstAECOutGainQ30,
  Ipp32s *pDstSpeakerGainQ30))
/********************************************************************************
// Name:            IppStatus ippsFilterAECNLMS_32sc_Sfs(Ipp32sc **ppSrcSignalIn,
//                                    Ipp32sc **ppSrcCoefs, Ipp32sc *pDstSignalOut,
//                                    int numSegments, int len, int scaleFactor)
//
// Description:        This function computes the filter output in AEC NLMS algorithm. Multiply-Accumulate have been
//            realized by MACRO MULT_CHECK_ACCUM_32SC_64SC, scaling & saturation have been realized by
//            MACRO SCALET_POS_64SC_32SC & SCALET_NEG_64SC_32SC. Those MACROs are defined in FixPoint.h.
//
// Input Arguments: ppSrcSignalIn    - pointer to an array of pointers to the most
//                                    recent input blocks (e.g., Xn, Xn-1,..., Xn-L+1).
//                                    These are the complex-valued vectors that
//                                    contain the FFT of the input signal stored in
//                                    Intel IPP CCS format.
//                  ppSrcCoefs        - pointer to an array of pointers to the
//                                    filter coefficients vectors. These are the
//                                    complex-valued vectors containing the filter
//                                    coefficients stored in Intel IPP CCS format.
//                  numSegments        - the number of filter segments (L).
//                  len                - number of elements contained in each input
//                                    vector and filter segment.
//                  scaleFactor        - saturation fixed scale factor
//
// Output Arguments: pDstSignalOut    - pointer to the complex-valued filter output
//                                    vector stored in Intel IPP CCS format.
//
// Returns:      ippStsNoErr      - No Error.
//               ippStsNullPtrErr - ppSrcSignalIn, ppSrcCoefs or pDstSignalOut is null.
//               ippStsLengthErr  - len is out of range.
//               ippStsRangeErr   - numSegments or scaleFactor is out of range.
//
// Notes:
********************************************************************************/
IPPAPI(IppStatus, ippsFilterAECNLMS_32sc_Sfs,(Ipp32sc **ppSrcSignalIn,Ipp32sc **ppSrcCoefs,
  Ipp32sc *pDstSignalOut,int numSegments, int len, int scaleFactor))

/********************************************************************************
// Name:            IppStatus ippsStepSizeUpdateAECNLMS_32s(Ipp32s *pSrcInputPSD,
//                                                          Ipp32s muQ31,
//                                                            Ipp32s maxStepSize,
//                                                            Ipp32s minInputPSD,
//                                                            IppAECScaled32s *pDstStepSizeQ31,
//                                                            int    len,
//                                                             int       scaleFactorPSD)
//
// Description:         This function calculates the step size of the adaptive filter
//                     used in the AEC algorithm using the classical normalized
//                   LMS approach.
//                     The adaptive step size computation is expressed in the following
//                   equation:
//                        For, 0<mu<<1 and Pxmin > 0
//                        M(k) = mu*2^(m-1) / Pxx(k) * 2^(n-1) :    for Pxx(k) >  Pxxmin
//                             = maxM                          :    for Pxx(k) <= Pxxmin
//                        where m,n is the number of leading zeros in the binary representation
//                            of the positive interger.
//                        where k      is the frequency bin index (0<=k<=N/2) into a N-point DFT
//                           Pxx(k) is the input power spectrum estimate for bin k for the most
//                    recently acquired N/2 input samples
//                     For High precision implement, the IppAECScaled32s struct has been used.
//                     Two components in the struct, val & sf.
//                        val = mu*2^(m-1) / Pxx(k) * 2^(n-1)
//                        sf = 61 + scaleFactorPSD - (n-m)
//
// Input Arguments:  pSrcInputPSD    - pointer to real-valued vector containing the input power
//                                      spectrum estimate.
//                   muQ31           - real-valued Q31 scalar with values between 0 and 1.
//                   maxStepSize     - maximum step size allowed (usually = mu / minInputPSD).
//                   minInputPSD     - minimum value of input PSD for which step size update should occur.
//                   len             - number of elements contained in input and output vectors.
//                   scaleFactorPSD  - scalefactor to record external shift of this function
//
// Output Arguments: pDstStepSizeQ31 - pointer to the struct IppAECScaled32s output vector, two component in
//                                        this struct, the component val keep the value of M(k) with Q30 format,
//                                        the conponent sf keep the value of (m-n).
//
// Returns:      ippStsNoErr      - No Error.
//               ippStsNullPtrErr - pSrcInputPSD or pDstStepSize is null.
//               ippStsLengthErr  - len is out of range.
//               ippStsRangeErr   - pSrcInputPSD[i], muQ31, maxStepSize.val or minInputPSD is out of range.
********************************************************************************/
IPPAPI(IppStatus, ippsStepSizeUpdateAECNLMS_32s, (Ipp32s *pSrcInputPSD,
       Ipp32s muQ31, IppAECScaled32s maxStepSize, Ipp32s minInputPSD,
       IppAECScaled32s *pDstStepSize, int len, int scaleFactorPSD ) )

/********************************************************************************
// Name:            IppStatus ippsCoefUpdateAECNLMS_32sc_I(const IppAECScaled32s
//                        *pSrcStepSize, const Ipp32sc **ppSrcFilterInput,
//                        const Ipp32sc *pSrcError, Ipp32sc **ppSrcDstCoefsQ15,
//                        int numSegments, int len);
//
// Description:        This function updates the coefficients of the AEC adaptive
//                    filter designed based on NLMS algorithm.
//                    In the filter update equation
//                        H(i) = H(i) + Mn * X(n-i) * En for i = 0, .. , L-1
//                    the kth component of the ith "delta" term "+Mn*X(n-i)*En"
//                    may be written
//                        Delta(k) = M'(k) * 2^(-sf(k)) * E(k) * X(n-i)(k)
//                    where
//                        M'(k) = M(k) * 2^(sf(k))
//                    is the new representation of the normalized step size
//                    The result of the first product may be stored in a 32-bit
//                    signed integer without need for saturation.  The first
//                    product need only be computed once for all i.  The result
//                    of the second product might overflow for large step sizes
//                    and large instantaneous inputs.  Although this case is
//                    atypical, it is necessary to check for this condition and
//                    perform the coefficient update addition operation in 64-bit
//                    precision with saturation when it occurs.
//                    So, three step used for updating:
//                    1) compute the Delta filter coefficient - Delta(k)
//                    2) update real part of filter coefficient
//                    3) update imaginary part of filter coefficient
// Input Arguments: pSrcStepSize    - pointer to the step size vector.
//                  ppSrcFilterInput- pointer to an array of pointers to the most
//                                    recent input blocks (e.g., Xn, Xn-1, ..., Xn-L+1).
//                                    These are the complex-valued vectors that
//                                    contain the FFT of the input signal stored in
//                                    Intel IPP CCS format.
//                  ppSrcDstCoefsQ15- pointer to an array of pointers to the
//                                    filter coefficient vectors. These are the
//                                    complex-valued vectors containing the filter
//                                    coefficients stored in Intel IPP CCS format.
//                  pSrcError       - pointer to the complex-valued vector
//                                    containing the filter error stored in Intel
//                                    IPP CCS format.
//                  numSegments     - the number of filter segments (L).
//                  len             - number of elements contained in each input
//                                    vector.
//
// Output Arguments: ppSrcDstCoefsQ15    - pointer to an array of pointers to the
//                                    filter coefficient vectors. It is In-Place
//                                    variant.
//
// Returns:      ippStsNoErr      - No Error.
//               ippStsNullPtrErr - pSrcStepSize, ppSrcFilterInput, pSrcError or ppSrcDstCoefsQ15 is null.
//               ippStsLengthErr  - len is out of range.
//               ippStsRangeErr   - numSegments or scaleFactorCoef is out of range.
//               ippStsSizeErr    - pSrcStepSize[indexVec].val is out of range.
//
// Notes:
********************************************************************************/
IPPAPI(    IppStatus, ippsCoefUpdateAECNLMS_32sc_I, (const IppAECScaled32s *pSrcStepSize,
        const Ipp32sc **ppSrcFilterInput, const Ipp32sc *pSrcError,
        Ipp32sc    **ppSrcDstCoefsQ15, int    numSegments, int len, int scaleFactorCoef))
/* /////////////////////////////////////////////////////////////////////////////
//                  Definitions of Audio Toolkit functions end
///////////////////////////////////////////////////////////////////////////// */


#if defined (_IPP_STDCALL_CDECL)
  #undef  _IPP_STDCALL_CDECL
  #define __stdcall __cdecl
#endif

#ifdef __cplusplus
}
#endif

#endif /* __IPPALIGN_H__ */
/* ////////////////////////////// End of file /////////////////////////////// */

