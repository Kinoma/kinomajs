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

#include<math.h>
#include<string.h>
#include "ipps.h"
#include "ippac.h"
#include "sbrdec_element.h"
#include "sbrdec_own_int.h"
#include "sbrdec_setting_int.h"

/********************************************************************/

#ifndef MUL32_SBR_32S
#define MUL32_SBR_32S(x, y) \
  (Ipp32s)(((Ipp64s)((Ipp64s)(x) * (Ipp64s)(y)))>>32)
#endif

/********************************************************************/

#ifndef UMC_MIN
#define UMC_MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef UMC_MAX
#define UMC_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif

/*******************************************************************/

int sbrNormalizationCovElements(Ipp64sc * vIn, Ipp32sc *vOut, int len)
{
  int     n, nDB, nGB, shift;
  int     absMax;
  Ipp64s mask;
  Ipp64sc tmp;
  Ipp32u absMaxU = 0;

  absMax = 0;

  for (n = 0; n < len; n++) {

    mask = vIn[n].re >> 63;
    tmp.re = (vIn[n].re ^ mask) - mask;

    mask = vIn[n].im >> 63;
    tmp.im = (vIn[n].im ^ mask) - mask;

    absMax |= (int)(tmp.re >> 32) | (int)(tmp.im >> 32);

    absMaxU |= (Ipp32u)tmp.re | (Ipp32u)tmp.im;
  }

  if (absMax == 0) { //up-part is zerro

    /* patch */
    if( absMaxU > (Ipp32u)IPP_MAX_32S )
      nGB = 32;
    else
      nGB = 32 + CLZ( (Ipp32s)absMaxU );

  } else {

    nGB = CLZ(absMax);
  }

  nDB = 64 - nGB;
  if (nDB <= 30) {

    shift = (30 - nDB);
    for (n = 0; n < len; n++) {
      vOut[n].re = (int)vIn[n].re << shift;
      vOut[n].im = (int)vIn[n].im << shift;
    }
    return (2 * SCALE_FACTOR_QMFA_OUT + shift);

  } else {      /* if (nDB < 32 + 30) */

    shift = (nDB - 30);

    for (n = 0; n < len; n++) {
      vOut[n].re = (int)(vIn[n].re >> shift);
      vOut[n].im = (int)(vIn[n].im >> shift);
    }
    return (2 * SCALE_FACTOR_QMFA_OUT - shift);

  }

//  return 0;
}

/*******************************************************************/

int sbrNormalizationCovElementsLP(Ipp64s * vIn, Ipp32s *vOut, int len)
{
  int     n, nDB, nGB, shift;
  int     absMax = 0;
  Ipp64s tmp;
  Ipp64s mask;
  Ipp32u absMaxU = 0;

  for (n = 0; n < len; n++) {

    mask = vIn[n] >> 63;
    tmp = (vIn[n] ^ mask) - mask;
    absMax |= (int)(tmp >> 32);

    absMaxU |= (Ipp32u)tmp;
  }

  if (absMax == 0) { //up-part is zerro

    /* patch */
    if( absMaxU > (Ipp32u)IPP_MAX_32S )
      nGB = 32;
    else
      nGB = 32 + CLZ( (Ipp32s)absMaxU );

  } else {

    nGB = CLZ(absMax);
  }

  nDB = 64 - nGB;
  if (nDB <= 30) {

    shift = (30 - nDB);
    for (n = 0; n < len; n++) {
      vOut[n] = (int)vIn[n] << shift;
    }
    return (2 * SCALE_FACTOR_QMFA_OUT + shift);

  } else {      /* if (nDB < 32 + 30) */

    shift = (nDB - 30);

    for (n = 0; n < len; n++) {
      vOut[n] = (int)(vIn[n] >> shift);
    }
    return (2 * SCALE_FACTOR_QMFA_OUT - shift);

  }

//  return 0;
}

/*******************************************************************/

IppStatus ownPredictOneCoef_SBR_C_32s_D2L(Ipp32sc** pSrc,
                                          Ipp32sc *pAlpha0, Ipp32sc *pAlpha1,
                                          int k, int lenCorr)
{
  const int TIME_HF_ADJ = 2;
  int     i, j, n;
  int     offset;
  int     maxCommon;
  int     nGB;
  int     absMax;

  Ipp64sc vIn[5];
  Ipp32sc vOut[5];
  Ipp32s  det, invDet;
  Ipp32sc coef;

  Ipp32sc *iA0 = pAlpha0;
  Ipp32sc *iA1 = pAlpha1;

  Ipp32sc bufX[40];

  int     scaleFactorOut;

  const Ipp64sc zerro_64sc = { 0, 0 };

  Ipp64sc fi01, fi02, fi11, fi12, fi22;
  Ipp32sc iFi01, iFi02, iFi11, iFi12, iFi22;

  Ipp32s coefA0Re = 0, coefA0Im = 0, coefA1Re = 0, coefA1Im = 0;

/* step 1: estimate MaxVal */

  for (n = 0; n < lenCorr + TIME_HF_ADJ; n++) {
    bufX[n] = pSrc[n][k];
  }

  ippsMaxAbs_32s_x( (Ipp32s*)bufX, 2*(lenCorr + TIME_HF_ADJ), &absMax);
  maxCommon = absMax;

  if (maxCommon == 0) {

    pAlpha0->re = pAlpha0->im = pAlpha1->re = pAlpha1->im = 0;

    return ippStsNoErr;   // OK
  }

/* step 2: calculate num Guard bit */
  nGB = CLZ(maxCommon);
  nGB -= 1;       // remove "sign"

/* step 3: check  nGB >= 3 (!!!) */
  offset = 0;
  if (nGB < 3)
    offset = 3 - nGB;

/* step 4: scale */
  if (offset) {
    for (n = 0; n < lenCorr+TIME_HF_ADJ; n++) {
      bufX[n].re = pSrc[n][k].re >> offset;
      bufX[n].im = pSrc[n][k].im >> offset;
    }
  }

/* step 5: main loop */
  //for (k = 0; k < k0; k++) {
    fi01 = fi02 = fi11 = fi12 = fi22 = zerro_64sc;

/* ---------------------------- correlation ---------------------------- */
/*
 * step 5.1: correlation
 */
    for (n = 0+TIME_HF_ADJ; n < lenCorr+TIME_HF_ADJ; n++) {
      i = 0;
      j = 1;
      fi01.re += MUL64_SBR_64S( bufX[n - i].re, bufX[n - j].re ) +
                 MUL64_SBR_64S( bufX[n - i].im, bufX[n - j].im );

      fi01.im += MUL64_SBR_64S( bufX[n - i].im, bufX[n - j].re ) -
                 MUL64_SBR_64S( bufX[n - i].re, bufX[n - j].im );

      i = 0;
      j = 2;
      fi02.re += MUL64_SBR_64S( bufX[n - i].re, bufX[n - j].re ) +
                 MUL64_SBR_64S( bufX[n - i].im, bufX[n - j].im );

      fi02.im += MUL64_SBR_64S( bufX[n - i].im, bufX[n - j].re ) -
                 MUL64_SBR_64S( bufX[n - i].re, bufX[n - j].im );

      i = 1;
      j = 1;
      fi11.re += MUL64_SBR_64S( bufX[n - i].re, bufX[n - j].re ) +
                 MUL64_SBR_64S( bufX[n - i].im, bufX[n - j].im );

      i = 1;
      j = 2;
      fi12.re += MUL64_SBR_64S( bufX[n - i].re, bufX[n - j].re ) +
                 MUL64_SBR_64S( bufX[n - i].im, bufX[n - j].im );

      fi12.im += MUL64_SBR_64S( bufX[n - i].im, bufX[n - j].re ) -
                 MUL64_SBR_64S( bufX[n - i].re, bufX[n - j].im );

      i = 2;
      j = 2;
      fi22.re += MUL64_SBR_64S( bufX[n - i].re, bufX[n - j].re ) +
                 MUL64_SBR_64S(bufX[n - i].im, bufX[n - j].im );
    }

/* ---------------------------- determinant ---------------------------- */
/*
 * step 5.2: "exactly" normalization
 */
/*
 * step 5.2.1: fill vector In
 */
    vIn[0] = fi01;
    vIn[1] = fi02;
    vIn[2] = fi11;
    vIn[3] = fi12;
    vIn[4] = fi22;

/*
 * step 5.2.2: Normalization
 */
    scaleFactorOut = sbrNormalizationCovElements(vIn, vOut, 5);

/*
 * step 5.2.3: fill
 */
    iFi01 = vOut[0];
    iFi02 = vOut[1];
    iFi11 = vOut[2];
    iFi12 = vOut[3];
    iFi22 = vOut[4];

    scaleFactorOut = scaleFactorOut - 2 * offset;

/*
 * step 5.3: calc determinant
 */
    det = MUL32_SBR_32S(iFi12.re, iFi12.re) + MUL32_SBR_32S(iFi12.im, iFi12.im);
    det = MUL32_SBR_32S(iFi11.re, iFi22.re) - det;

/* ---------------------------- coefs Alpha1 ---------------------------- */

    if (det) {
      int outSf;

      invDet = sbrInvWrap_32s_Sf(det, &outSf);

      coef.re = MUL32_SBR_32S(iFi01.re, iFi12.re) - MUL32_SBR_32S(iFi01.im, iFi12.im);
      coef.re = coef.re - MUL32_SBR_32S(iFi02.re, iFi11.re);
      coef.re = MUL32_SBR_32S( coef.re, invDet);

      coef.im = MUL32_SBR_32S(iFi01.re, iFi12.im) + MUL32_SBR_32S(iFi01.im, iFi12.re);
      coef.im = coef.im - MUL32_SBR_32S(iFi02.im, iFi11.re);
      coef.im = MUL32_SBR_32S( coef.im, invDet);

      coefA1Re = coef.re << (1 + outSf);
      coefA1Im = coef.im << (1 + outSf);
    }

/* ---------------------------- coefs Alpha0 ---------------------------- */

    if (iFi11.re) {
      int outSf;

      invDet = sbrInvWrap_32s_Sf(iFi11.re, &outSf);

      // Re part
      coef.re = (iFi01.re >> 3) + MUL32_SBR_32S(iFi12.re, coefA1Re) +
                MUL32_SBR_32S(iFi12.im, coefA1Im);

      coef.re = -MUL32_SBR_32S(coef.re, invDet) << (SCALE_FACTOR_LP_COEFS - 25 + outSf);

      // Im part
      coef.im = (iFi01.im >> 3) - MUL32_SBR_32S(iFi12.im, coefA1Re) +
                MUL32_SBR_32S(iFi12.re, coefA1Im);

      coef.im = -MUL32_SBR_32S(coef.im, invDet) << (SCALE_FACTOR_LP_COEFS - 25 + outSf);

      coefA0Re = coef.re;
      coefA0Im = coef.im;
    }

    { // CHECK THRESHOLD
      int sum1, sum2;

      sum1 = MUL32_SBR_32S(coefA0Re, coefA0Re) + MUL32_SBR_32S(coefA0Im, coefA0Im);
      sum2 = MUL32_SBR_32S(coefA1Re, coefA1Re) + MUL32_SBR_32S(coefA1Im, coefA1Im);

      if (sum1 >= THRESHOLD_LP_COEFS || sum2 >= THRESHOLD_LP_COEFS) {

        iA0->re = iA0->im = iA1->re = iA1->im = 0;
      }else{
        iA0->re = coefA0Re;
        iA0->im = coefA0Im;

        iA1->re = coefA1Re;
        iA1->im = coefA1Im;
      }
    }
// -----------------------
  //}     /* for(k=0; k<k0; k++) */

  return ippStsNoErr;
}

/********************************************************************/

IppStatus ownPredictOneCoef_SBR_R_32s_D2L(Ipp32s** pSrcRe, Ipp32s* pAlpha0Re,
                                       Ipp32s* pAlpha1Re, Ipp32s* pRefCoef,
                                       Ipp32s k, Ipp32s len)
{

 const int TIME_HF_ADJ = 2;
  int     i, j, n;
  int     offset;
  int     maxCommon;
  int     nGB;
  int    absMax;

  Ipp64s vIn[5];
  Ipp32s vOut[5];
  Ipp32s  det, invDet;
  Ipp32s coef = {0};
  Ipp32s *iA0Re = pAlpha0Re;
  Ipp32s *iA1Re = pAlpha1Re;

  int bufXRe[40];

  int     scaleFactorOut;

  Ipp64s fi01, fi02, fi11, fi12, fi22;
  Ipp32s iFi01, iFi02, iFi11, iFi12, iFi22;

  Ipp32s coefA0Re = 0, coefA1Re = 0;

/* step 1: estimate MaxVal */

  *pAlpha0Re = 0;
  *pAlpha1Re = 0;
  *pRefCoef  = 0;

  for (n = 0; n < len + TIME_HF_ADJ; n++) {
    bufXRe[n] = pSrcRe[n][k];
  }

  ippsMaxAbs_32s_x(bufXRe, len + TIME_HF_ADJ, &absMax);
  maxCommon = absMax;

  if (maxCommon == 0) {

    return ippStsNoErr;   // OK
  }

/* step 2: calculate num Guard bit */
  nGB = CLZ(maxCommon);
  nGB -= 1;       // remove "sign"

/* step 3: check  nGB >= 2 (!!!) */
  offset = 0;
  if (nGB < 2)
    offset = 2 - nGB;

/* step 4: scale */
  if (offset) {
    for (n = 0; n < 40; n++) {
      bufXRe[n] >>= offset;
    }
  }

/* step 5: main loop */
  //for (k = 0; k < k0; k++) {
    fi01 = fi02 = fi11 = fi12 = fi22 = 0L;

/* ---------------------------- correlation ---------------------------- */
/*
 * step 5.1: correlation
 */
    for (n = 0+TIME_HF_ADJ; n < len+TIME_HF_ADJ; n++) {
      i = 0;
      j = 1;
      fi01 += MUL64_SBR_64S(bufXRe[n - i], bufXRe[n - j]);


      i = 0;
      j = 2;
      fi02 += MUL64_SBR_64S(bufXRe[n - i], bufXRe[n - j]);

      i = 1;
      j = 1;
      fi11 += MUL64_SBR_64S(bufXRe[n - i], bufXRe[n - j]);

      i = 1;
      j = 2;
      fi12 += MUL64_SBR_64S(bufXRe[n - i], bufXRe[n - j]);

      i = 2;
      j = 2;
      fi22 += MUL64_SBR_64S(bufXRe[n - i], bufXRe[n - j]);
    }
/* ---------------------------- determinant ---------------------------- */
/*
 * step 5.2: "exactly" normalization
 */
/*
 * step 5.2.1: fill vector In
 */
    vIn[0] = fi01;
    vIn[1] = fi02;
    vIn[2] = fi11;
    vIn[3] = fi12;
    vIn[4] = fi22;

/*
 * step 5.2.2: Normalization
 */
    scaleFactorOut = sbrNormalizationCovElementsLP(vIn, vOut, 5);

/*
 * step 5.2.3: fill
 */
    iFi01 = vOut[0];
    iFi02 = vOut[1];
    iFi11 = vOut[2];
    iFi12 = vOut[3];
    iFi22 = vOut[4];

    scaleFactorOut = scaleFactorOut - 2 * offset;

/*
 * step 5.3: calc determinant
 */
    det = MUL32_SBR_32S(iFi12, iFi12);
    det = MUL32_SBR_32S(iFi11, iFi22) - det;

/* ---------------------------- coefs Alpha1 ---------------------------- */

    if (det) {
      int outSf;

      invDet = sbrInvWrap_32s_Sf(det, &outSf);

      coef = MUL32_SBR_32S(iFi01, iFi12);
      coef = coef - MUL32_SBR_32S(iFi02, iFi11);
      coef = MUL32_SBR_32S( coef, invDet);

      coefA1Re = coef << (1 + outSf);
    }

/* ---------------------------- coefs Alpha0 ---------------------------- */

    if (iFi11) {
      int outSf;

      invDet = sbrInvWrap_32s_Sf(iFi11, &outSf);

      // Re part
      coef = (iFi01 >> 3) + MUL32_SBR_32S(iFi12, coefA1Re);
      coef = -MUL32_SBR_32S(coef, invDet);// << (SCALE_FACTOR_LP_COEFS - 25 + outSf);

      coefA0Re = coef  << (SCALE_FACTOR_LP_COEFS - 25 + outSf);

      /* patch: only for LP mode */
      {
        const int maxRefCoef = 1 << 29;

        *pRefCoef =  -MUL32_SBR_32S(invDet, iFi01) << (1+outSf); //Q(28)
        *pRefCoef =   UMC_MIN(UMC_MAX(*pRefCoef, -maxRefCoef ), maxRefCoef);
        //*pRefCoef <<= 3; //Q(30) truble?;

        //pRefCoef[k] =  MUL32_SBR_32S(invDet, iFi01) << 2; //Q(30)
      }
    }

    { // CHECK THRESHOLD
      int sum1, sum2;

      sum1 = MUL32_SBR_32S(coefA0Re, coefA0Re);
      sum2 = MUL32_SBR_32S(coefA1Re, coefA1Re);

      if (sum1 >= THRESHOLD_LP_COEFS || sum2 >= THRESHOLD_LP_COEFS) {

        *iA0Re = *iA1Re = 0;
      }else{
        *iA0Re = coefA0Re;
        *iA1Re = coefA1Re;
      }
    }
// -----------------------
  //}     /* for(k=0; k<k0; k++) */

  return ippStsNoErr;
}

/********************************************************************/

/* EOF */
