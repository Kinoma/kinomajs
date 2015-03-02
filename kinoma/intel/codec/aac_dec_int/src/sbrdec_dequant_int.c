/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/

/********************************************************************/
//***
#include "kinoma_ipp_lib.h"

#include <ipps.h>
#include <math.h>
#include "sbrdec_api_int.h"
#include "sbrdec_own_int.h"

/********************************************************************/

#ifndef ID_SCE
#define ID_SCE    0x0
#endif

#ifndef ID_CPE
#define ID_CPE    0x1
#endif

/********************************************************************/

#ifndef UMC_MAX
#define UMC_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef UMC_MIN
#define UMC_MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

/********************************************************************/

static int SBR_TABLE_DEQUANT_ENV[2] = { 0x20000000, 0x2D413CCC };

/********************************************************************/

/* SBR_TABLE_DEQUANT_INVERT[i] = 2^1 / (1 + 2^(12 - i)), Q30 */
static const int SBR_TABLE_DEQUANT_INVERT[25] = {
  0x0007ff80, 0x000ffe00, 0x001ff802, 0x003fe010,
  0x007f8080, 0x00fe03f8, 0x01f81f82, 0x03e0f83e,
  0x07878788, 0x0e38e38e, 0x1999999a, 0x2aaaaaab,
  0x40000000, 0x55555555, 0x66666666, 0x71c71c72,
  0x78787878, 0x7c1f07c2, 0x7e07e07e, 0x7f01fc08,
  0x7f807f80, 0x7fc01ff0, 0x7fe007fe, 0x7ff00200,
  0x7ff80080,
};

/********************************************************************/

int sbrDequantInvertCouple(Ipp32s *pSrcDstL, Ipp16s *pSrcR, Ipp32s *pDstR,
                           int len, int shift)
{
  int     expR, expL;
  int     i;

  for (i = 0; i < len; i++) {
    expL = pSrcDstL[i];
    expR = pSrcR[i] >> shift;
/*
 * clip expR [0, 24]
 */
    if (expR < 0)
      expR = 0;
    if (expR > 24)
      expR = 24;

    pDstR[i] =
      MUL32_SBR_32S(expL, SBR_TABLE_DEQUANT_INVERT[24 - expR]) << 2;
    pSrcDstL[i] =
      MUL32_SBR_32S(expL, SBR_TABLE_DEQUANT_INVERT[expR]) << 2;
  }

  return 0;     // OK
}

/********************************************************************/

int sbrDequantNoiseUncouple(Ipp32s *pSrcDstL, Ipp16s *pSrcR, Ipp32s *pDstR,
                            int len)
{

  sbrDequantInvertCouple(pSrcDstL, pSrcR, pDstR, len, 0);

  return 0;     // OK
}

/********************************************************************/

int sbrDequantEnvUncouple(Ipp32s *pSrcDstEnvL, Ipp16s *pSrcEnvR,
                          Ipp32s *pDstEnvR, int numEnv, int *vecSize,
                          int ampRes)
{
  int     len;
  int     env;
  int     shift = 1 - ampRes;
  int    *pBufL = pSrcDstEnvL;
  Ipp16s *pBufInR = pSrcEnvR;
  int    *pBufOutR = pDstEnvR;

  for (env = 0; env < numEnv; env++) {
    len = vecSize[env + 1] - vecSize[env];

    sbrDequantInvertCouple(pBufL, pBufInR, pBufOutR, len, shift);

    pBufL += len;
    pBufInR += len;
    pBufOutR += len;
  }

  return 0;     // OK
}

/********************************************************************/

int sbrDequantNoiseMono(Ipp16s *pSrc, Ipp32s *pDst, int len)
{
  int     i;
  int     currScaleFactor;

  if (len <= 0)
    return 0;

  for (i = 0; i < len; i++) {
    currScaleFactor = SBR_DEQUANT_OFFSET - pSrc[i] + 24;        /* 2^6 *
                                                                 * 2^(-exp) *
                                                                 * Q24 */

    if (currScaleFactor < 0)
      pDst[i] = 0;

    else if (pSrc[i] < 30)
      pDst[i] = 1 << currScaleFactor;

    else
      pDst[i] = 0x3fffffff;
  }

  return 0;
}

/********************************************************************/

int sbrDequantEnvMono_OneBand(Ipp16s *pSrc, Ipp32s *pDst, int len, int ampRes)
{
  Ipp16s  maxScaleFactor, currScaleFactor;
  int     i;
  int     shift = 1 - ampRes;

  if (len <= 0)
    return 0;

  ippsMax_16s_x(pSrc, len, &maxScaleFactor);
  maxScaleFactor >>= shift;

  for (i = 0; i < len; i++) {
// currScaleFactor = maxScaleFactor - (pSrc[i] >> shift) ;
    currScaleFactor = (Ipp16s)UMC_MIN(maxScaleFactor - (pSrc[i] >> shift), 31);
    pDst[i] = SBR_TABLE_DEQUANT_ENV[pSrc[i] & shift & 0x1] >> currScaleFactor;
  }

  return 29 - (6 + maxScaleFactor);
}

/********************************************************************/

static int sbrDequantEnvMono(Ipp16s *pSrc, Ipp32s *pDst, int *vecSize, int LE,
                             int bs_amp_res, int *vecOffset)
{
  int     l, len;
  short  *pBufSrc = pSrc;
  int    *pBufDst = pDst;

  for (l = 0; l < LE; l++) {

    len = vecSize[l + 1] - vecSize[l];

    vecOffset[l] = sbrDequantEnvMono_OneBand(pBufSrc, pBufDst, len, bs_amp_res);

    pBufSrc += len;
    pBufDst += len;
  }

  return 0;     // OK
}

/********************************************************************/

Ipp32s sbrDequantization(sSbrDecCommon * pSbrCom, sSbrDecWorkSpace * pSbrWS,
                         int *sfsEnv, Ipp32s ch, int bs_amp_res)
{
  Ipp32s  error = 0;    // OK

/* ORDINARY */
/*
 * envelope
 */
  sbrDequantEnvMono(pSbrCom->vecEnv[ch], pSbrWS->vEOrig[ch],
                    pSbrCom->vSizeEnv[ch], pSbrCom->L_E[ch], bs_amp_res,
                    sfsEnv);

/*
 * noise
 */
  sbrDequantNoiseMono(pSbrCom->vecNoise[ch], pSbrWS->vNoiseOrig[ch],
                      pSbrCom->vSizeNoise[ch][pSbrCom->L_Q[ch]]);

/* COUPLE MODE */
  if (pSbrCom->bs_coupling == 1) {

/*
 * envelope
 */
    sbrDequantEnvUncouple(pSbrWS->vEOrig[0], pSbrCom->vecEnv[1],
                          pSbrWS->vEOrig[1], pSbrCom->L_E[1],
                          pSbrCom->vSizeEnv[0], bs_amp_res);

/*
 * noise
 */
    sbrDequantNoiseUncouple(pSbrWS->vNoiseOrig[0], pSbrCom->vecNoise[1],
                            pSbrWS->vNoiseOrig[1],
                            pSbrCom->vSizeNoise[0][pSbrCom->L_Q[0]]);
  }

  return error;
}

/* EOF */
