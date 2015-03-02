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
#include "sbrdec_setting_int.h"
#include "sbrdec_own_int.h"

/********************************************************************/

#ifndef UMC_MIN
#define UMC_MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef UMC_MAX
#define UMC_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif

/********************************************************************/

/* Q(31) */
static Ipp32s SBR_TABLE_NEW_BW_INT_Q31[4][4] = {
  {0x00000000, 0x4ccccccd, 0x73333333, 0x7d70a3d7},
  {0x4ccccccd, 0x60000000, 0x73333333, 0x7d70a3d7},
  {0x00000000, 0x60000000, 0x73333333, 0x7d70a3d7},
  {0x00000000, 0x60000000, 0x73333333, 0x7d70a3d7},
};

/********************************************************************
 *
 * OutPut data: iBwArray, Q(14)
 ********************************************************************/
static Ipp32s sbrCalcChirpFactors(Ipp32s N_Q, Ipp32s *bs_invf_mode_prev,
                                  Ipp32s *bs_invf_mode, Ipp32s *iBwArray )
{
  Ipp32s  i;
  Ipp32s  iNewBw, iTmpBw;

  //printf("\n");
  for (i = 0; i < N_Q; i++) {

    iNewBw = SBR_TABLE_NEW_BW_INT_Q31[bs_invf_mode_prev[i]][bs_invf_mode[i]];

    if (iNewBw < iBwArray[i])
      iTmpBw =
        MUL32_SBR_32S(0x60000000, iNewBw) +
        MUL32_SBR_32S(0x20000000, iBwArray[i]);
    else
      iTmpBw =
        MUL32_SBR_32S(0x74000000, iNewBw) +
        MUL32_SBR_32S(0x0C000000, iBwArray[i]);

    iTmpBw = iTmpBw << 1;
    iBwArray[i] = iTmpBw;

    if (iTmpBw < 0x02000000)
      iBwArray[i] = 0;

    bs_invf_mode_prev[i] = bs_invf_mode[i];

    //printf(" bwArr = %i ", iBwArray[i]);
  }

  //printf("\n");

  return 0;
}

/********************************************************************/

static Ipp32s sbrCalcAliasDegree(Ipp32s *ref, Ipp32s *deg, Ipp32s k0)
{

  Ipp32s  sign = 0;
  Ipp32s  k;

  ippsZero_32s_x(deg, k0);
  ref[0] = 0;
  deg[1] = 0;

  for (k = 2; k < k0; k++) {
    if ((k % 2 == 0) && (ref[k] < 0)) {
      sign = 1;
    } else if ((k % 2 == 1) && (ref[k] > 0)) {
      sign = -1;
    } else {
      sign = 0;
      continue;
    }

    if (sign * ref[k - 1] < 0) {
      deg[k] = 1 << 29;
      if (sign * ref[k - 2] > 0) {
        deg[k - 1] = (1 << 29) - (MUL32_SBR_32S( ref[k - 1] << 1, ref[k - 1] << 1) <<1 );
      }
    } else {
      if (sign * ref[k - 2] > 0) {
        //deg[k] = 1 - ref[k - 1] * ref[k - 1];
        deg[k] = (1 << 29) - (MUL32_SBR_32S( ref[k - 1] << 1, ref[k - 1] << 1) <<1 );
      }
    }
  }

  return 0;     // OK
}

/********************************************************************/

static Ipp32s sbrHFGeneratorHQ(
                      /*
                       * in data
                       */
                              Ipp32sc** iXBuf,
                              Ipp32s *vbwArray, sSbrDecCommon * pSbr,
                              int l_start, int l_end,
                      /*
                       * out data
                       */
                              Ipp32sc** iYBuf)
{
  /* values */
  Ipp32s  i, x, q, k_0, k, p, g, l;

  int     iBwAr, iBwAr2;
  Ipp32sc sumY;//sumYRe, sumYIm;
  int     icA0Re, icA0Im, icA1Re, icA1Im;

  Ipp32sc** pX = iXBuf + SBR_TIME_HFADJ;

  Ipp32sc** pY = iYBuf + SBR_TIME_HFADJ;

  Ipp8u   isNonCalcPredictCoef[64];
  Ipp32sc a0Coefs[64], a1Coefs[64];

  /* reset values */
  ippsSet_8u_x(1, isNonCalcPredictCoef, 64);
//  IppsZero_32sc_x(a0Coefs, 64);
//  IppsZero_32sc_x(a1Coefs, 64);

  //printf("\npredict, k0 = %i\n", pSbr->k0);

  /* code */
  for (i = 0; i < pSbr->numPatches; i++) {
    k_0 = 0;
    for (q = 0; q < i; q++) {
      k_0 += pSbr->patchNumSubbands[q];
    }

    k_0 += pSbr->kx;
    for (x = 0; x < pSbr->patchNumSubbands[i]; x++) {
      k = k_0 + x;
      p = pSbr->patchStartSubband[i] + x;

      for (g = 0; g < pSbr->N_Q; g++) {
        if ((k >= pSbr->f_TableNoise[g]) && (k < pSbr->f_TableNoise[g + 1]))
          break;
      }

      iBwAr = vbwArray[g];// Q(31)

      //printf("\np = %i\n", p);
      //printf("\nbwArr = %i\n", iBwAr);

      if ( iBwAr > 0) {

        /* Predict */
        if ( isNonCalcPredictCoef[p] ) {

          ownPredictOneCoef_SBR_C_32s_D2L(iXBuf, &a0Coefs[p], &a1Coefs[p], p, 38);
          isNonCalcPredictCoef[p] = 0;

        }

        // Q(31) * Q(31) * Q(-32) = Q(30)
        iBwAr2 = MUL32_SBR_32S(vbwArray[g], vbwArray[g]);
        iBwAr2 <<= 1;       // Q(31)

      // BwArray
        // Q(31) * Q(29) * Q(-32) = Q(28)
        icA0Re = MUL32_SBR_32S(iBwAr, a0Coefs[p].re);

        // Q(31) * Q(29) * Q(-32) = Q(28)
        icA0Im = MUL32_SBR_32S(iBwAr, a0Coefs[p].im);

        // Q(31) * Q(29) * Q(-32) = Q(28)
        icA1Re = MUL32_SBR_32S(iBwAr2, a1Coefs[p].re);

        // Q(31) * Q(29) * Q(-32) = Q(28)
        icA1Im = MUL32_SBR_32S(iBwAr2, a1Coefs[p].im);

        for (l = l_start; l < l_end; l++) {

  /*
  * bw * Alpha0 * XLow[l-1] = Q(28) * Q(5) * Q(-32) = Q(1)
  */
          sumY.re =
            MUL32_SBR_32S(pX[l - 1][p].re,icA0Re) -
            MUL32_SBR_32S(pX[l - 1][p].im, icA0Im);

          sumY.im =
            MUL32_SBR_32S(pX[l - 1][p].re,icA0Im) +
            MUL32_SBR_32S(pX[l - 1][p].im, icA0Re);

  /*
  * bw^2 * Alpha1 * XLow[l-2] = Q(28) * Q(5) * Q(-32) = Q(1)
  */
          sumY.re +=
            MUL32_SBR_32S(pX[l - 2][p].re,icA1Re) -
            MUL32_SBR_32S(pX[l - 2][p].im, icA1Im);

          sumY.im +=
            MUL32_SBR_32S(pX[l - 2][p].re, icA1Im) +
            MUL32_SBR_32S(pX[l - 2][p].im, icA1Re);

  /*
  * this check may be improved in the future
  */
          if (abs(sumY.re) < (((1 << 30) - 1) >> 4) &&
              abs(sumY.im) < (((1 << 30) - 1) >> 4)) {
                sumY.re <<= 4;
                sumY.im <<= 4;

          } else {

            sumY.re = (sumY.re > 0) ? (1 << 30) - 1 : -(1 << 30);
            sumY.im = (sumY.im > 0) ? (1 << 30) - 1 : -(1 << 30);
          }

          pY[l][k].re = pX[l - 0][p].re + sumY.re;
          pY[l][k].im = pX[l - 0][p].im + sumY.im;

        }// for (l = l_start

      } else { //if (iBwAr == 0)

        for (l = l_start; l < l_end; l++) {
          pY[l][k].re = pX[l - 0][p].re;
          pY[l][k].im = pX[l - 0][p].im;
        }
      }// endif(bwArr > 0) ?

    }//for (x = 0

  }// for (i = 0;

  return 0;     // OK
}

/********************************************************************/

static Ipp32s sbrHFGeneratorLP(
                      /* in data  */
                              Ipp32s** piXBufRe,
                      /* out data */
                              Ipp32s** piYBufRe,

                              int*    iBwArray,
                              sSbrDecCommon* pSbr,
                              int l_start, int l_end,
                              Ipp32s *deg, Ipp32s *degPatched)
{
  int     i, x, q, k_0, k, p, g, l;
  int     iBwAr, iBwAr2;
  int     sumYRe;
  int     icA0Re, icA1Re;
  int     coefA0Re, coefA1Re, iRefCoef;

  int** pXRe = piXBufRe + SBR_TIME_HFADJ;
  int** pYRe = piYBufRe + SBR_TIME_HFADJ;


 // ippsZero_32f_x(degPatched, MAX_NUM_ENV_VAL);

  for (i = 0; i < pSbr->numPatches; i++) {
    k_0 = 0;
    for (q = 0; q < i; q++) {
      k_0 += pSbr->patchNumSubbands[q];
    }

    k_0 += pSbr->kx;
    for (x = 0; x < pSbr->patchNumSubbands[i]; x++) {
      k = k_0 + x;
      p = pSbr->patchStartSubband[i] + x;

      for (g = 0; g < pSbr->N_Q; g++) {
        if ((k >= pSbr->f_TableNoise[g]) && (k < pSbr->f_TableNoise[g + 1]))
          break;
      }

      iBwAr = iBwArray[g];// Q(31)

      if ( iBwAr > 0 ) {

        // if( ! isPredict[p] )
        {
           ownPredictOneCoef_SBR_R_32s_D2L(piXBufRe, &coefA0Re, &coefA1Re, &iRefCoef, p, 38);
        }

        // Q(31) * Q(31) * Q(-32) = Q(30)
        iBwAr2 = MUL32_SBR_32S(iBwAr, iBwAr);
        iBwAr2 <<= 1;       // Q(31)

      // BwArray
        // Q(31) * Q(29) * Q(-32) = Q(28)
        icA0Re = MUL32_SBR_32S(iBwAr, coefA0Re);

        // Q(31) * Q(29) * Q(-32) = Q(28)
        icA1Re = MUL32_SBR_32S(iBwAr2, coefA1Re);


        for (l = l_start; l < l_end; l++) {

  /*
  * bw * Alpha0 * XLow[l-1] = Q(28) * Q(5) * Q(-32) = Q(1)
  */
          sumYRe = MUL32_SBR_32S(pXRe[l - 1][p],icA0Re);

  /*
  * bw^2 * Alpha1 * XLow[l-2] = Q(28) * Q(5) * Q(-32) = Q(1)
  */
          sumYRe += MUL32_SBR_32S(pXRe[l - 2][p],icA1Re);

  /*
  * this check may be improved in the future
  */
          if ( abs(sumYRe) < (((1 << 30) - 1) >> 4) ) {
            sumYRe <<= 4;

          } else {

            sumYRe = (sumYRe > 0) ? (1 << 30) - 1 : -(1 << 30);
          }

          pYRe[l][k] = pXRe[l - 0][p] + sumYRe;

          //pYBufRe[l+2][k] = pYRe[l][k] / 32.f ;

        }// for (l = l_start


      }else{ //if( bwArr == 0)

        for (l = l_start; l < l_end; l++) {
          pYRe[l][k] = pXRe[l - 0][p];
          //pYBufRe[l+2][k] = pYRe[l][k] / 32.f ;
        }

      }

/*-------------------------------------------------------------*/
      //if (mode == HEAAC_LP_MODE) {
        if (x == 0)
          degPatched[k] = 0;
        else
          degPatched[k] = deg[p];
    //  }
/*-------------------------------------------------------------*/
    }
  }

  //if (mode == HEAAC_LP_MODE) {
    k_0 = 0;
    for (q = 0; q < pSbr->numPatches; q++)
      k_0 += pSbr->patchNumSubbands[q];

    for (k = pSbr->kx + k_0; k < 64; k++)
      degPatched[k] = 0;
  //}

  return 0;     // OK
}

/********************************************************************/

Ipp32s sbrGenerationHF(Ipp32s** iXBuf,
                       Ipp32s** iYBuf,

                       sSbrDecCommon * sbr_com, Ipp32s *bwArray,
                       Ipp32s *degPatchedCoefs, Ipp32s ch, int decode_mode)
{
  int refCoefs[64];
  int a0Coefs[64];
  int a1Coefs[64];

  int degCoefs[64];

  Ipp32s  l_start = RATE * sbr_com->tE[ch][0];
  Ipp32s  l_end   = RATE * sbr_com->tE[ch][sbr_com->L_E[ch]];


  sbrCalcChirpFactors(sbr_com->N_Q, &(sbr_com->bs_invf_mode_prev[ch][0]),
                      &(sbr_com->bs_invf_mode[ch][0]), bwArray);

  if (HEAAC_HQ_MODE == decode_mode)
    sbrHFGeneratorHQ( (Ipp32sc**)iXBuf, bwArray, sbr_com, l_start, l_end, (Ipp32sc**)iYBuf);

  else { //if (HEAAC_LP_MODE == decode_mode)

    ippsZero_32s_x(degPatchedCoefs, 64);

   {
     int k;
     for(k=0; k<sbr_com->k0; k++)
     {
      ownPredictOneCoef_SBR_R_32s_D2L(iXBuf, &a0Coefs[k], &a1Coefs[k], &refCoefs[k], k, 38);
     }
   }

   sbrCalcAliasDegree(refCoefs, degCoefs, sbr_com->k0);

   sbrHFGeneratorLP(iXBuf, iYBuf, bwArray, sbr_com, l_start, l_end,
                    degCoefs, degPatchedCoefs);

  }

  return 0;     // OK
}

/********************************************************************/

/* EOF */
