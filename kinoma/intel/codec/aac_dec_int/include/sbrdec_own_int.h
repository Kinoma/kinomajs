/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __SBRDEC_OWN_INT_H__
#define __SBRDEC_OWN_INT_H__

#include "ippac.h"
#include "ipps.h"

int sbrAnalysisFilter_SBR_RToC_32s_D2L_Sfs(Ipp32s* pSrc,
                                            Ipp32sc* pDst[],
                                            int nSubBand, int kx,
                                            IppsFFTSpec_C_32sc* pFFTSpec,
                                            int* pDelayCoefs,
                                            int scaleFactor,
                                            Ipp8u* pWorkBuf);

/****************************************************************************************/
int sbrAnalysisFilter_SBR_RToR_32s_D2L_Sfs(Ipp32s* pSrc,
                                    Ipp32s* pDst[],
                                    int nSubBand, int kx,
                                    IppsFFTSpec_C_32sc* pFFTSpec,
                                    Ipp32s* pDelayCoefs,
                                    int scaleFactor,
                                    Ipp8u* pWorkBuf);


/****************************************************************************************/


IppStatus ownPredictOneCoef_SBR_C_32s_D2L( Ipp32sc** pSrc,
                                            Ipp32sc* pAlpha0, Ipp32sc* pAlpha1,
                                            int k, int len);


IppStatus ownPredictOneCoef_SBR_R_32s_D2L(Ipp32s** pSrcRe, Ipp32s* pAlpha0Re, Ipp32s* pAlpha1Re,
                                       Ipp32s* pRefCoef, Ipp32s k, Ipp32s len);


/****************************************************************************************/

/* -------------------------------- MATH PART OF SBR DEC ---------------------------- */

#define MUL64_SBR_64S(x, y)  \
  ( ( (Ipp64s)((Ipp64s)(x) * (Ipp64s)(y)) ) )

#define MUL32_SBR_32S(x, y) \
  (Ipp32s)(((Ipp64s)((Ipp64s)(x) * (Ipp64s)(y)))>>32)

int CLZ( int maxVal );

int sbrSqrtWrap_64s_Sfs(int inData, int inScaleFactor, int *outScaleFactor);

int sbrInvWrap_32s_Sf(int x, int* outScaleFactor);

int sbrChangeScaleFactor(int inData, int inScaleFactor, int outScaleFactor);

#endif //__SBRDEC_OWN_INT_H__
