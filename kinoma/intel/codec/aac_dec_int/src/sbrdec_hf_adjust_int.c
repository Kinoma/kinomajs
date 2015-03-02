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

#include <math.h>
#include "ipps.h"
#include "ippsr.h"
#include "sbrdec_element.h"
#include "sbrdec_tables_int.h"
#include "sbrdec_api_int.h"
#include "sbrdec_own_int.h"
#include <stdio.h>

/********************************************************************/

#ifndef UMC_MIN
#define UMC_MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef UMC_MAX
#define UMC_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif

static const Ipp32f EPS0 = 1e-12f;

/********************************************************************/

/* (1.584893192)^2 * Q28 */
static const int THRESHOLD_GAIN_BOOST = 0x2830AFD3;

/********************************************************************/

static const Ipp32s POW_MINUS_UNIT[] =
  { 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1,
  1, -1, 1, -1, 1, -1, 1, -1, 1, -1,
  1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1,
  1, -1, 1, -1, 1, -1, 1, -1, 1, -1
};

/********************************************************************/

static const Ipp32s SIN_FI_RE[4] = { 1, 0, -1, 0 };

/********************************************************************/

static const Ipp32s SIN_FI_IM[4] = { 0, 1, 0, -1 };

/********************************************************************/

/* Q30 */
static const int TABLE_LIMIT_GAIN_INT_Q30[4] =
  { 0x20138CA7, 0x40000000, 0x7FB27DCE, THRESHOLD_GAIN };

/********************************************************************/

/* Q31 */
static const int ihSmoothFilter[] = {
  0x04130598, 0x0EBDB043, 0x1BECFA68, 0x2697A512, 0x2AAAAAAB
};

/****************************************************
 *
 * SBR_TABLE_INVERT[i] = 2^0 / (i + 1), Q31
 * see <sbrdec_math_int.c> && <sbrdec_own_int.h>
 *
 ****************************************************/

static int sbrCalcGainGroups(Ipp32s* s_mapped, Ipp32s* degPatched, Ipp32s* f_group, int kx, int M )
{
  Ipp32s  grouping = 0, i = 0, n_group = 0, k = 0;

  for (k = kx; k < kx + M - 1; k++) {
    if (degPatched[k + 1] && (s_mapped[k - kx] == 0)) {
      if (grouping == 0) {
        f_group[i] = k;
        grouping = 1;
        i++;
      }
    } else {
      if (grouping == 1) {
        if (s_mapped[k - kx] == 0)
          f_group[i] = k + 1;
        else
          f_group[i] = k;

        grouping = 0;
        i++;
      }
    }
  }

  if (grouping == 1) {
    f_group[i] = M + kx;
    i++;
  }

  n_group = (i >> 1);

  return n_group;
}

/********************************************************************/

int sbrScale_64s32s(Ipp64s InData, int *scaleFactor)
{

  int     OutData;
#if 1
  int     nGB;
  int     shift = 0;

  if (InData >> 32) {
    nGB = CLZ((int)(InData >> 32)) - 1; //remove "sign" bit
    shift = 32 - nGB;
    OutData = (int)(InData >> shift);

  } else if (InData > (Ipp64s)IPP_MAX_32S ) {//(InData >> 31) {
    shift = 1;
    OutData = (int)(InData >> shift);
  } else {
    OutData = (int)InData;
  }

  *scaleFactor = -shift;
#endif

  return OutData;
}

/********************************************************************/

static int sbrPreProcAliasReduction(Ipp32s* pSrc, Ipp32s* pDst, int len, int minGB)
{
  int absMax;
  int nGB;
  int offset;
  int n;

 //---------------------------------------------------------
  ippsMaxAbs_32s_x(pSrc, len, &absMax);	


  if (absMax == 0) {

    ippsCopy_32s_x(pSrc, pDst, len);
    return 0;   // OK
  }

/* step 2: calculate num Guard bit */
  nGB = CLZ(absMax);
  nGB -= 1;       // remove "sign"

/* step 3: check  nGuardBit >= 3 (!!!) */
  offset = 0;
  if (nGB < minGB)
    offset = minGB - nGB;

/* step 4: scale */
  //if (offset) {
    for (n = 0; n < 64; n++) {
      pDst[n] = pSrc[n] >> offset;
    }

    return offset;
  //}
  //------------------------------------------------------------------
}

/********************************************************************/

static void sbrAliasReduction_32s(Ipp32s *degPatched,
                                  Ipp32s *inBufE, int sfBufE,
                                  Ipp32s *inBufG, int sfBufG,
                                  Ipp32s *inBufG2, int sfBufG2,
                                  Ipp32s *s_mapped, Ipp32s kx, Ipp32s M)
{
  Ipp64s  denum_64s, energ_total_new_64s, energ_total_64s;

  const Ipp32s oneQ29 = 0x20000000;

  Ipp32s  new_gain, energ_total_new, alpha;
  Ipp32s  i, n_group, k, m, iStart, iEnd;
  Ipp32s  f_group[64];
  Ipp32s  bufE[64];
  Ipp32s  bufG[64];
  Ipp32s  bufG2[64];

  Ipp32s sfTmp, sfE, sfG, sfG2, sfNewGain;

  int sfETN;

  /* E */
  sfTmp = sbrPreProcAliasReduction(inBufE, bufE, 64, 3);
  sfE   = sfBufE - sfTmp;

  /* G */
  sfTmp = sbrPreProcAliasReduction(inBufG, bufG, 64, 3);
  sfG   = sfBufG - sfTmp;

  /* G2 */
  sfTmp = sbrPreProcAliasReduction(inBufG2, bufG2, 64, 3);
  sfG2  = sfBufG2 - sfTmp;

  n_group = sbrCalcGainGroups( s_mapped, degPatched, f_group, kx, M );

  for (k = 0; k < n_group; k++) {

    iStart = f_group[2 * k] - kx;
    iEnd = f_group[2 * k + 1] - kx;

    denum_64s       = 0;
    energ_total_64s = 0;

    for (i = iStart; i < iEnd; i++) {
      energ_total_64s += MUL64_SBR_64S(bufG2[i], bufE[i]);
      denum_64s += (Ipp64s)bufE[i];
    }

    new_gain = 0;
    sfNewGain = 0;
    if( denum_64s )
    {
      int num_32s, denum_32s;
      int outSf;
      int sfNum, sfDenum;

      /*{
        int tmpSf;

        Ipp64s new_gain_64s = energ_total_64s / denum_64s;

        new_gain = sbrScale_64s32s(new_gain_64s, &tmpSf);

        sfNewGain = sfG2 - tmpSf;
        printf("\n64s: %15.10f\n", (float)(new_gain*pow(2, - sfNewGain)) );
      }*/
      //------------------------
      num_32s   = sbrScale_64s32s(energ_total_64s, &sfNum);
      denum_32s = sbrScale_64s32s(denum_64s, &sfDenum);

      denum_32s = sbrInvWrap_32s_Sf(denum_32s, &outSf);

      new_gain = MUL32_SBR_32S(num_32s, denum_32s);

      sfNewGain = 60 - (sfE + sfDenum + outSf) + (sfG2+sfE + sfNum) - 32;

      new_gain = sbrChangeScaleFactor(new_gain, sfNewGain, sfG2);
      sfNewGain = sfG2;
      //------------------------
    }

    for (m = iStart + kx; m < iEnd + kx; m++) {
      if (m < M + kx - 1) {
        alpha = UMC_MAX(degPatched[m], degPatched[m + 1]);
      } else {
        alpha = degPatched[m];
      }
      //bufG2[m - kx] = alpha * new_gain + (1.0f - alpha) * bufG2[m - kx];
      bufG2[m - kx] = (MUL32_SBR_32S( alpha, new_gain ) << 3) +
                      (MUL32_SBR_32S((oneQ29 - alpha), bufG2[m - kx])<<3);
    }

    energ_total_new_64s = 0L;
    for (i = iStart; i < iEnd; i++)
      energ_total_new_64s += MUL64_SBR_64S(bufG2[i], bufE[i]);


    sfETN = 0;
    energ_total_new = 0;
    if( energ_total_new_64s ) {

      int energ_total_new_32s, energ_total_32s;
      int outSf;
      int sfTmpNew, sfTmpOld;

      energ_total_new_32s = sbrScale_64s32s(energ_total_new_64s, &sfTmpNew);
      energ_total_32s = sbrScale_64s32s(energ_total_64s, &sfTmpOld);

      energ_total_new_32s = sbrInvWrap_32s_Sf(energ_total_new_32s, &outSf);

      energ_total_new = MUL32_SBR_32S(energ_total_32s, energ_total_new_32s);

      sfETN = 60 - (sfG2+sfE + sfTmpNew + outSf) + (sfG2+sfE + sfTmpOld) - 32;
    }

    /* float-point oeration */
    for (i = iStart; i < iEnd; i++) {
#if 0
      float etn_32f = energ_total_new * pow(2, -sfETN );
      float buf_g2_i_32f = bufG2[i] * pow(2, -sfG2 );
      float bug_g_32f = (Ipp32f)(sqrt(etn_32f * buf_g2_i_32f));

      bufG[i] = (int)(bug_g_32f * pow(2,sfG) );
#else
      Ipp32s inSf, outSf;// = sfETN + sfG2;
      Ipp64s varG_64s = MUL64_SBR_64S(energ_total_new, bufG2[i]);
      Ipp32s varG_32s = sbrScale_64s32s(varG_64s, &outSf);

      inSf = sfETN + sfG2 + outSf;

      varG_32s = sbrSqrtWrap_64s_Sfs(varG_32s, inSf, &outSf);
      //bufG[i] = sbrChangeScaleFactor(varG_32s, outSf, sfG);
      inBufG[i] = sbrChangeScaleFactor(varG_32s, outSf, sfBufG);
#endif

    }
  }

#if 0
  /* convert */
  for(i=0; i<64; i++){
    inBufG[i] = sbrChangeScaleFactor(bufG[i], sfG, sfBufG);
  }
#endif

  return;
}

/********************************************************************/

/********************************************************************/

static Ipp32s sbrSinAdd_LP_32s(Ipp32s* YRe, int indxSine, int tE0, int m, int kx, int i,
                               Ipp32s* bufSM, int M, int* numSin )


{
  Ipp32s  fIndexSineMinus1 = 0;
  Ipp32s  fIndexSinePlus1 = 0;
  int     num_sinusoids = *numSin;
  Ipp32s  ksi_middle = 0;
  Ipp32s  xScale = 0xFDE9E1B1;//(Ipp32s)(-0.00815f * pow(2, 32));
  Ipp64s  sumY_64s = 0L;

  fIndexSineMinus1 = (indxSine + (i - 1) - RATE * tE0) & 3;
  fIndexSinePlus1  = (indxSine + (i + 1) - RATE * tE0) & 3;

  if (m == 0) {
    //YRe[m + kx - 1] = -0.00815f * POW_MINUS_UNIT[kx - 1] * bufSM[0] * SIN_FI_RE[fIndexSinePlus1];
    YRe[m + kx - 1] = MUL32_SBR_32S(xScale, bufSM[0]) * POW_MINUS_UNIT[kx - 1] * SIN_FI_RE[fIndexSinePlus1];

    if (m < M - 1) {
      //YRe[m + kx] += -0.00815f * POW_MINUS_UNIT[kx] * bufSM[1] * SIN_FI_RE[fIndexSinePlus1];
      sumY_64s = YRe[m + kx] + MUL32_SBR_32S(xScale, bufSM[1]) * POW_MINUS_UNIT[kx] * SIN_FI_RE[fIndexSinePlus1];

      if (sumY_64s >= (Ipp64s)IPP_MAX_32S )
        YRe[m + kx] = IPP_MAX_32S;
      else if( sumY_64s <= (Ipp64s)IPP_MIN_32S )
        YRe[m + kx] = IPP_MIN_32S;
      else
        YRe[m + kx] = (Ipp32s)(sumY_64s);

    }
  } else if ((0 < m) && (m < M - 1) && (num_sinusoids < 16)) {
    ksi_middle = (bufSM[m - 1] * SIN_FI_RE[fIndexSineMinus1] + bufSM[m + 1] * SIN_FI_RE[fIndexSinePlus1]);
    //ksi_middle *= -0.00815f * POW_MINUS_UNIT[m + kx];
    ksi_middle = MUL32_SBR_32S(xScale, ksi_middle) * POW_MINUS_UNIT[m + kx];
    sumY_64s = YRe[m + kx] + ksi_middle;

    if (sumY_64s >= (Ipp64s)IPP_MAX_32S )
        YRe[m + kx] = IPP_MAX_32S;
      else if( sumY_64s <= (Ipp64s)IPP_MIN_32S )
        YRe[m + kx] = IPP_MIN_32S;
      else
        YRe[m + kx] = (Ipp32s)(sumY_64s);

    //YRe[m + kx] += ksi_middle;

    } else if ((m == M - 1) && (num_sinusoids < 16)) {
      if (m > 0) {
        //YRe[m + kx] += -0.00815f * POW_MINUS_UNIT[m + kx] * bufSM[m - 1] * SIN_FI_RE[fIndexSineMinus1];
        sumY_64s = MUL32_SBR_32S(xScale, bufSM[m - 1]) * POW_MINUS_UNIT[m + kx] * SIN_FI_RE[fIndexSineMinus1];
        sumY_64s = sumY_64s + YRe[m + kx];

        if (sumY_64s >= (Ipp64s)IPP_MAX_32S )
          YRe[m + kx] = IPP_MAX_32S;
        else if( sumY_64s <= (Ipp64s)IPP_MIN_32S )
          YRe[m + kx] = IPP_MIN_32S;
        else
          YRe[m + kx] = (Ipp32s)(sumY_64s);

      }
      if (M + kx < 64) {
        //YRe[m + kx + 1] = -0.00815f * POW_MINUS_UNIT[m + kx + 1] * bufSM[m] * SIN_FI_RE[fIndexSineMinus1];
        YRe[m + kx + 1] = MUL32_SBR_32S(xScale, bufSM[m]) * POW_MINUS_UNIT[m + kx + 1] * SIN_FI_RE[fIndexSineMinus1];
      }
    }

    if (bufSM[m])
      num_sinusoids++;

    *numSin = num_sinusoids;

    return 0;//OK
}

/********************************************************************/

static Ipp32s sbrUpdate_lA(Ipp32s bs_pointer, Ipp32s bs_frame_class, Ipp32s L_E)
{

#ifndef __SYMBIAN32__
  Ipp32s  table_lA[3][3] = { {-1, -1, -1},
  {-1, L_E + 1 - bs_pointer, -1},
  {-1, L_E + 1 - bs_pointer, bs_pointer - 1}
  };
#endif

  Ipp32s  indx1, indx2;
  Ipp32s  lA;

#ifdef __SYMBIAN32__
  Ipp32s  table_lA[3][3];
  table_lA[0][0] = -1; table_lA[0][1] = -1; table_lA[0][2] = -1;
  table_lA[1][0] = -1; table_lA[1][1] = L_E + 1 - bs_pointer; table_lA[1][2] = -1;
  table_lA[2][0] = -1; table_lA[2][1] = L_E + 1 - bs_pointer; table_lA[2][2] = bs_pointer - 1;
#endif

  if ((bs_pointer > 1) || (bs_pointer < 0))
    indx1 = 2;
  else
    indx1 = bs_pointer;

  if (bs_frame_class == VARVAR)
    indx2 = 1;
  else
    indx2 = bs_frame_class;

  lA = table_lA[indx1][indx2];

  return lA;
}

/********************************************************************/

int tmpCLIP_32s32s(int InData)
{
  int     OutData;

  /*if (InData > IPP_MAX_32S)
    OutData = IPP_MAX_32S;
  else if (InData < IPP_MIN_32S)
    OutData = IPP_MIN_32S;
  else*/
    OutData = (int)(InData);

  return OutData;

}

/********************************************************************/

int sbrCheckUpDate(int gain_max, int sfGainMax, int gain,
                   int scaleFactorGain)
{
  int     flagUpDate = 0;
  int     shift;

  if (gain_max != THRESHOLD_GAIN) {
    if (scaleFactorGain >= sfGainMax) {
      shift = UMC_MIN(scaleFactorGain - sfGainMax, 31);
      flagUpDate = ((gain >> shift) > gain_max ? 1 : 0);
    } else {
      shift = UMC_MIN(sfGainMax - scaleFactorGain, 31);
      flagUpDate = (gain > (gain_max >> shift) ? 1 : 0);
    }
  }

  return flagUpDate;
}

/********************************************************************/

void sbrAdjustmentHF(Ipp32s** iYBuf, Ipp32s *vEOrig,
                     Ipp32s *vNoiseOrig, Ipp32s *sfsEOrig,

                     int iBufGain[][MAX_NUM_ENV_VAL],
                     int iBufNoise[][MAX_NUM_ENV_VAL], sSbrDecCommon * sbr_com,
                     Ipp32s *degPatched, Ipp8u *WorkBuffer, Ipp32s reset,
                     Ipp32s ch, Ipp32s decode_mode)
{
  /* VALUES */
  Ipp32s  k, m, n, pos, kl, kh, l, p, i, j, delta_step, delta;
  Ipp32s  sumEOrig, sumECurr, sumSM, sumQM, sumECurrGLim;
  Ipp32s  gainMax, gainBoost, gainFilt, noiseFilt;
  Ipp32s  iStart, iEnd;
  Ipp32s  denum;
  Ipp32s  nResolution;

  Ipp64s  sumE, sumEWoInterpol;

  Ipp32s  bufSM[64];
  Ipp32s  bufQM[64];
  Ipp32s  bufEOrig[64];
  Ipp32s  bufG[64];
  Ipp32s  bufG2[64];

  Ipp32s  bufQMapped[64];
  Ipp32s  bufECurr[64];
  Ipp32s  resolution[2];

  Ipp32s  sfsECurr[64];
  Ipp32s  sfsGain[64];

  Ipp32s  sfInterpolation;
  Ipp32s  sfECurrMax;
  Ipp32s  sfGainBoost;
  Ipp32s  sfGainMax;
  Ipp32s  sfTmp;

  Ipp32s  iECurrWoInterpol;
  Ipp32s  mulQ, mulQQ;
  Ipp32s  data;

  Ipp32s  iLimiterGains = TABLE_LIMIT_GAIN_INT_Q30[sbr_com->bs_limiter_gains];

  Ipp32s  interpolation = sbr_com->bs_interpol_freq;
  Ipp32s  fIndexNoise = 0;
  Ipp32s  fIndexSine = 0;

  Ipp32s  hSmoothLen = (sbr_com->bs_smoothing_mode == 0) ? 4 : 0;
  Ipp32s  hSmoothLenConst = hSmoothLen;

  Ipp32s  kx = sbr_com->kx;
  Ipp32s  M = sbr_com->M;
  Ipp32s  LE = sbr_com->L_E[ch];
  Ipp32s  NQ = sbr_com->N_Q;
  Ipp32s  lA = 0;

  Ipp32s  offset_32s = 64 * sizeof(Ipp32s);

  Ipp32s *pFreqTbl;
  Ipp32s *pEOrigCurrAddress;
  Ipp32s *sineIndxMap;
  Ipp32s *sineIndxMapPrev;
  Ipp32s *F[2];

  //Ipp32s** pYRe = iYBuf + SBR_TIME_HFADJ;
  //Ipp32s** pYIm = iYBufIm + SBR_TIME_HFADJ;
  Ipp32sc** pYcmp = (Ipp32sc**)iYBuf + SBR_TIME_HFADJ;
  Ipp32s** pYre =  iYBuf + SBR_TIME_HFADJ;

  /* LP mode */
  Ipp32s  num_sinusoids = 0;
  Ipp32s  s_mapped[64];

  int maxSfGain = 0;
  int minSfECurr;

  /* CODE */

  F[0] = sbr_com->f_TableLow;
  F[1] = sbr_com->f_TableHigh;

  resolution[0] = sbr_com->N_low;
  resolution[1] = sbr_com->N_high;

/* set memory */
  sineIndxMap = (Ipp32s *)(WorkBuffer + 5 * offset_32s);
  sineIndxMapPrev = sbr_com->S_index_mapped_prev[ch];

  if (reset) {
    sbr_com->FlagUpdate[ch] = 1;
    sbr_com->indexNoise[ch] = 0;
  }

  ippsZero_32s_x(sineIndxMap, 64);

  for (i = 0; i < sbr_com->N_high; i++) {
    m = (sbr_com->f_TableHigh[i + 1] + sbr_com->f_TableHigh[i]) >> 1;
    sineIndxMap[m - kx] = sbr_com->bs_add_harmonic[ch][i];
  }

  lA = sbrUpdate_lA(sbr_com->bs_pointer[ch], sbr_com->bs_frame_class[ch],
                 sbr_com->L_E[ch]);

/* main loop */
  for (l = 0; l < LE; l++) {
    for (k = 0; k < sbr_com->L_Q[ch]; k++) {
      if (sbr_com->tE[ch][l] >= sbr_com->tQ[ch][k] &&
          sbr_com->tE[ch][l + 1] <= sbr_com->tQ[ch][k + 1])
        break;
    }

    for (i = 0; i < NQ; i++) {
      for (m = sbr_com->f_TableNoise[i]; m < sbr_com->f_TableNoise[i + 1]; m++) {
        bufQMapped[m - kx] = vNoiseOrig[sbr_com->vSizeNoise[ch][k] + i];
      }
    }

    delta = (l == lA || l == sbr_com->lA_prev[ch]) ? 1 : 0;

    if (decode_mode == HEAAC_HQ_MODE)
      hSmoothLen = (delta ? 0 : hSmoothLenConst);
    else
      hSmoothLen = 0;

/* --------------------------------  Estimation envelope ---------------------------- */
/*
 * new envelope's reset
 */
    pos = 0;
    sfECurrMax = 0;
    nResolution = resolution[sbr_com->r[ch][l]];
    pFreqTbl = F[sbr_com->r[ch][l]];
    iStart = RATE * sbr_com->tE[ch][l];
    iEnd = RATE * sbr_com->tE[ch][1 + l];
    pEOrigCurrAddress = &vEOrig[sbr_com->vSizeEnv[ch][l]];
    sfInterpolation = 0;
    iECurrWoInterpol = 0;

    /* AYA */
    maxSfGain = 0;
    ippsZero_32s_x( sfsGain, 64 );
    ippsZero_32s_x( bufG, 64 );
    ippsZero_32s_x( bufG2, 64 );
    ippsZero_32s_x( bufECurr, 64 );

    for (p = 0; p < nResolution; p++) {
      kl = pFreqTbl[p];
      kh = pFreqTbl[p + 1];
      delta_step = 0;
      sumEWoInterpol = 0L;

      for (j = kl; j < kh; j++) {
        sumE = 0L;
        for (i = iStart; i < iEnd; i++) {

  // tmp PATCH
          {
            int     yRe, yIm;

            if(decode_mode == HEAAC_LP_MODE) {
              yRe = tmpCLIP_32s32s(pYre[i][j] >> 5);
              sumE += MUL64_SBR_64S(yRe, yRe);

            } else { //if (decode_mode == HEAAC_HQ_MODE) {
              yRe = tmpCLIP_32s32s(pYcmp[i][j].re >> 5);
              sumE += MUL64_SBR_64S(yRe, yRe);

              yIm = tmpCLIP_32s32s(pYcmp[i][j].im >> 5);
              sumE += MUL64_SBR_64S(yIm, yIm);
            }

          }
        }
        delta_step = (sineIndxMap[pos] &&
                      (l >= lA || sineIndxMapPrev[pos + kx])) ? 1 : delta_step;

        /*
         * PS: out scaleFactor is negative
         */
        bufECurr[pos] = sbrScale_64s32s(sumE, sfsECurr + pos);

        denum = SBR_TABLE_INVERT[(iEnd - iStart) - 1]; // Q31

        // Q(-sfsECurr[pos]) * Q(31) * Q(-32) = Q(-... - 1)
        bufECurr[pos] = MUL32_SBR_32S(bufECurr[pos], denum);

        sfsECurr[pos] -= 1;

        if (!interpolation) {
          sumEWoInterpol += sumE;
        }

        pos++;
      } // end for (j = kl; j < kh; j++) {

      if (!interpolation) {
        int     scaleFactor;

// may be improved
        denum = SBR_TABLE_INVERT[(iEnd - iStart) - 1];       // Q31
        denum = MUL32_SBR_32S(SBR_TABLE_INVERT[(kh - kl) - 1], denum) << 1;

        iECurrWoInterpol = sbrScale_64s32s(sumEWoInterpol, &scaleFactor);
        iECurrWoInterpol = MUL32_SBR_32S(iECurrWoInterpol, denum);
        sfInterpolation = scaleFactor - 1;
      }

      for (k = pos - (kh - kl); k < pos; k++) {
        bufSM[k] = 0;
        bufEOrig[k] = pEOrigCurrAddress[p];

        if (!interpolation) {
          bufECurr[k] = iECurrWoInterpol;
          sfsECurr[k] = sfInterpolation;
        }

/* --------------------------------  Calculation of Level of Add Component ---------------------------- */

        data = (bufQMapped[k] >> 1) + (1 << (SCALE_FACTOR_NOISE_DEQUANT - 1));
        denum = sbrInvWrap_32s_Sf(data, &sfTmp);
        denum <<= 1;

/*
 * convert to Q31
 */
        mulQ = (denum >> (7 - sfTmp));
        mulQQ = MUL32_SBR_32S(denum, bufQMapped[k]) << (1 + sfTmp);

// ------------------------
        if (decode_mode == HEAAC_LP_MODE) {
          //bufECurr[k] <<= 1;
          sfsECurr[k] -= 1;

          if (delta_step)
            s_mapped[k - pos + kh - kx] = 1;
          else
            s_mapped[k - pos + kh - kx] = 0;
        }
// ------------------------
        if ((delta_step) && (sineIndxMap[k] && (l >= lA || sineIndxMapPrev[k + kx]))) {
          bufSM[k] = MUL32_SBR_32S(bufEOrig[k], mulQ) << 1;
        }

        bufG[k] = bufEOrig[k];
        sfsGain[k] = sfsEOrig[l];

        if (delta_step) {
          bufG[k] = MUL32_SBR_32S(bufG[k], mulQQ) << 1;

        } else if (!delta) {
          bufG[k] = MUL32_SBR_32S(bufG[k], mulQ) << 1;
        }
// --------------------
        if (bufECurr[k]) {

          denum = sbrInvWrap_32s_Sf(bufECurr[k], &sfTmp);
          bufG[k] = MUL32_SBR_32S(bufG[k], denum);
          sfsGain[k] = 28 - sfTmp - sfsECurr[k] + sfsEOrig[l];
        }
// --------------------

        bufQM[k] = MUL32_SBR_32S(bufEOrig[k], mulQQ) << 1;
      } // end for (k = pos - (kh - kl); k < pos; k++)

    }   // end for( p = 0;

/* --------------------------------  Calculation of gain ---------------------------- */

    ippsMin_32s_x(sfsECurr, pos, &sfECurrMax);

    for (k = 0; k < sbr_com->N_L; k++) {
      sumEOrig = 0;
      sumECurr = 0;

      for (i = sbr_com->f_TableLim[k] - kx; i < sbr_com->f_TableLim[k + 1] - kx;
           i++) {

        sumEOrig += (bufEOrig[i] >> SCALE_FACTOR_SUM64);

        sumECurr += (bufECurr[i] >> -(sfECurrMax - sfsECurr[i]));
        if (sumECurr >> 30) {
          sumECurr >>= 1;
          sfECurrMax--;
        }
      } // end for(i=...

      sfGainMax = 30;  // default scaleFactor
      gainMax = iLimiterGains;  // default values

      if ((sumECurr == 0) && (sumEOrig)) {
        gainMax = THRESHOLD_GAIN;
      } else if (sumEOrig == 0) {
        gainMax = 0;
      } else if (gainMax != THRESHOLD_GAIN) {

        data = MUL32_SBR_32S(sumEOrig, gainMax);
/*
 * Q[ sfsEOrig[l] - SCALE_FACTOR_SUM64 - 2 ], gainMax = Q30
 */

        denum = sbrInvWrap_32s_Sf(sumECurr, &sfTmp);
        gainMax = MUL32_SBR_32S(data, denum);

        sfGainMax = 26 - sfTmp - sfECurrMax + (sfsEOrig[l] - SCALE_FACTOR_SUM64);
      }

      sumECurrGLim = 0;
      sumQM = 0;
      sumSM = 0;
      for (i = sbr_com->f_TableLim[k] - kx; i < sbr_com->f_TableLim[k + 1] - kx; i++) {

        if ((gainMax != THRESHOLD_GAIN) && sbrCheckUpDate(gainMax, sfGainMax, bufG[i], sfsGain[i])) {

  // QM[i] = QM[i] * (g_max / vGain[i]);
  // --------------
  // denum = ...Q[60 - (sfsGain[i] + sfTmp)]
          denum = sbrInvWrap_32s_Sf(bufG[i], &sfTmp);

  // data = ...Q[ sfGainMax - (sfsGain[i] + sfTmp) + 28 ]
          data = MUL32_SBR_32S(denum, gainMax);

          sfTmp = sfGainMax + 28 - (sfsGain[i] + sfTmp);
          data = sbrChangeScaleFactor(data, sfTmp, 30);
          bufQM[i] = MUL32_SBR_32S(bufQM[i], data) << 2;
  // --------------

  // G[i] = g_max;
          sfsGain[i] = sfGainMax;
          bufG[i] = gainMax;
        }       // end if

        {
          int     sum, shift;

          shift = sfsEOrig[l] - (sfsGain[i] + sfsECurr[i]);

          if (shift > 0)
            sum = (Ipp32s)( MUL64_SBR_64S(bufG[i], bufECurr[i]) << shift);
          else
            sum = (Ipp32s)( MUL64_SBR_64S(bufG[i], bufECurr[i]) >> -shift);

          sumECurrGLim += (sum >> SCALE_FACTOR_SUM64);
        }

        sumSM += (bufSM[i] >> SCALE_FACTOR_SUM64);

        if (!(bufSM[i] != 0 || delta)) {
          sumQM += (bufQM[i] >> SCALE_FACTOR_SUM64);
        }
//--------------------

      } // for (i = ...; i <

// compensation //
      denum = (sumECurrGLim >> 1) + (sumSM >> 1) + (sumQM >> 1);
// ------------------------------------------//
      sfGainBoost = 0; // default values
      gainBoost = THRESHOLD_GAIN_BOOST;
      sfTmp = 0;

      if ((denum == 0) && (sumEOrig == 0)) {
        gainBoost = 1 << 28;
      } else if (sumEOrig == 0) {
        gainBoost = 0;
      } else {
        denum = sbrInvWrap_32s_Sf(denum, &sfTmp);
        gainBoost = MUL32_SBR_32S(denum, sumEOrig) >> 1;
      }

      /* gainBoost = Q(28 - sfTmp) */
      if (gainBoost > (THRESHOLD_GAIN_BOOST >> sfTmp)) {

        gainBoost = THRESHOLD_GAIN_BOOST;
        sfTmp = 0;
      }
      gainBoost <<= sfTmp;     /* (Q28 - sfTmp) + sfTmp = Q28 */

      //maxSfGain = 0;

      /*
      {
        int len = sbr_com->f_TableLim[k + 1] - sbr_com->f_TableLim[k];
        ippsMax_32s_x(&sfsGain[i], len, &maxSfGain);
      }
      */
// ------------------------------------------//
      for (i = sbr_com->f_TableLim[k] - kx; i < sbr_com->f_TableLim[k + 1] - kx;
           i++) {

// GainLimBoost
        /* Q(sfsGain[i]) + Q28 - Q32 + 2 = Q(sfsGain[i]) - Q2 */
        bufG[i]  = MUL32_SBR_32S(bufG[i], gainBoost) << 2;
        bufG2[i] = bufG[i];

        bufG[i] = sbrSqrtWrap_64s_Sfs(bufG[i], sfsGain[i] - 2, &sfTmp);
        bufG[i] = sbrChangeScaleFactor(bufG[i], sfTmp, SCALE_FACTOR_G_LIM_BOOST);

// QMLimBoost
        bufQM[i] = MUL32_SBR_32S(bufQM[i], gainBoost) << 2;

        bufQM[i] = sbrSqrtWrap_64s_Sfs(bufQM[i], sfsEOrig[l] - 2, &sfTmp);
        bufQM[i] = sbrChangeScaleFactor(bufQM[i], sfTmp, SCALE_FACTOR_QM_LIM_BOOST);

// SMBoost
        bufSM[i] = MUL32_SBR_32S(bufSM[i], gainBoost) << 2;

        bufSM[i] = sbrSqrtWrap_64s_Sfs(bufSM[i], sfsEOrig[l] - 2, &sfTmp);
        bufSM[i] = sbrChangeScaleFactor(bufSM[i], sfTmp, SCALE_FACTOR_SM_BOOST);
      }
    }   // end for(k=0; k<

    if (decode_mode == HEAAC_LP_MODE) {

#if 1
        {
          int b;
          for(b=0; b<64; b++)
          {
            bufG2[b] = (Ipp32s)( MUL64_SBR_64S(bufG[b], bufG[b] ) >> 26 ) ;
          }
        }
#endif

        {
          int t;
          minSfECurr = 0;

          ippsMin_32s_x(sfsECurr, pos, &minSfECurr);

          for(t=0; t<pos; t++)
          {
            bufECurr[t] = bufECurr[t] >> -(minSfECurr - sfsECurr[t]);
          }
        }

         sbrAliasReduction_32s(degPatched,
                                  bufECurr, minSfECurr,
                                  bufG,  SCALE_FACTOR_G_LIM_BOOST,
                                  bufG2, 22,
                                  s_mapped, kx, M);

    }// if(HE_AAC_LP_MODE

/* --------------------------------  Assembling ---------------------------- */
    if (sbr_com->FlagUpdate[ch]) {
      for (n = 0; n < 4; n++) {
        ippsCopy_32s_x(bufG, iBufGain[n], M);
        ippsCopy_32s_x(bufQM, iBufNoise[n], M);
      }
      sbr_com->FlagUpdate[ch] = 0;
    }

    for (i = iStart; i < iEnd; i++) {
      for (m = 0; m < M; m++) {
        iBufGain[4][m] = bufG[m];
        iBufNoise[4][m] = bufQM[m];

        if (decode_mode == HEAAC_LP_MODE) {
          num_sinusoids = 0;
        }

        gainFilt = noiseFilt = 0;
        if (hSmoothLen) {
          for (n = 0; n <= 4; n++) {
            gainFilt  += MUL32_SBR_32S(iBufGain[n][m], ihSmoothFilter[n]);
            noiseFilt += MUL32_SBR_32S(iBufNoise[n][m], ihSmoothFilter[n]);
          }
          gainFilt <<= 1;
          noiseFilt <<= 1;

        } else {
          gainFilt = iBufGain[4][m];
          noiseFilt = iBufNoise[4][m];
        }

        if (bufSM[m] != 0 || delta) {
          noiseFilt = 0;
        }

        fIndexNoise = (sbr_com->indexNoise[ch] + (i - RATE * sbr_com->tE[ch][0]) * M + m + 1) & 511;

        fIndexSine = (sbr_com->indexSine[ch] + i - RATE * sbr_com->tE[ch][0]) & 3;

        {
          int     yRe, yIm;
          int     noiseRe, noiseIm;

  // float f1, f2, f3;
          if( decode_mode == HEAAC_LP_MODE ) {

            yRe = pYre[i][m + kx];

            noiseRe = MUL32_SBR_32S(noiseFilt, SBR_TABLE_V_INT_Q30[0][fIndexNoise]);

            noiseRe >>= (SCALE_FACTOR_QM_LIM_BOOST - 2 - SCALE_FACTOR_QMFA_OUT);

  //--------------------------------------------------
            yRe = MUL32_SBR_32S(yRe, gainFilt);
            yRe <<= (32 - SCALE_FACTOR_G_LIM_BOOST);

            yRe = (yRe >> 1) + (noiseRe >> 1) + (bufSM[m] * SIN_FI_RE[fIndexSine] >> 1);

    // patch

            if (yRe >= (IPP_MAX_32S ) )
              yRe = IPP_MAX_32S;
            else if (yRe <= (IPP_MIN_32S))
              yRe = IPP_MIN_32S;
            else
              yRe <<= 1;
          /* */

            pYre[i][m + kx] = yRe;
            //
          }else {//if (decode_mode == HEAAC_HQ_MODE) {

            yRe = pYcmp[i][m + kx].re;

            noiseRe = MUL32_SBR_32S(noiseFilt, SBR_TABLE_V_INT_Q30[0][fIndexNoise]);

            noiseRe >>= (SCALE_FACTOR_QM_LIM_BOOST - 2 - SCALE_FACTOR_QMFA_OUT);

  //--------------------------------------------------
            yRe = MUL32_SBR_32S(yRe, gainFilt);
            yRe <<= (32 - SCALE_FACTOR_G_LIM_BOOST);

            yRe = (yRe >> 1) + (noiseRe >> 1) + (bufSM[m] * SIN_FI_RE[fIndexSine] >> 1);

    // patch

            if (yRe >= (IPP_MAX_32S ) )
              yRe = IPP_MAX_32S;
            else if (yRe <= (IPP_MIN_32S))
              yRe = IPP_MIN_32S;
            else
              yRe <<= 1;
          /* */

            pYcmp[i][m + kx].re = yRe;

            yIm = pYcmp[i][m + kx].im;

            noiseIm = MUL32_SBR_32S(noiseFilt, SBR_TABLE_V_INT_Q30[1][fIndexNoise]);

            noiseIm >>= (SCALE_FACTOR_QM_LIM_BOOST - 2 - SCALE_FACTOR_QMFA_OUT);

            yIm = MUL32_SBR_32S(yIm, gainFilt);

            yIm <<= (32 - SCALE_FACTOR_G_LIM_BOOST);

            yIm = (yIm >> 1) + (noiseIm >> 1) + (bufSM[m] * POW_MINUS_UNIT[m + kx] * SIN_FI_IM[fIndexSine] >> 1);

            if (yIm >= IPP_MAX_32S)
              yIm = IPP_MAX_32S;
            else if (yIm <= IPP_MIN_32S)
              yIm = IPP_MIN_32S;
            else
              yIm <<= 1;

            pYcmp[i][m + kx].im = yIm;
          }

          if (decode_mode == HEAAC_LP_MODE) {

            sbrSinAdd_LP_32s(pYre[i], sbr_com->indexSine[ch], sbr_com->tE[ch][0], m, kx, i,
              bufSM, M, &num_sinusoids ) ;
          }

//-----------------------------------------------------
        }

      } // end for ( m = 0; m < M; m++ )

      for (n = 0; n < 4; n++) {
        ippsCopy_32s_x(iBufGain[n + 1], iBufGain[n], 64);
        ippsCopy_32s_x(iBufNoise[n + 1], iBufNoise[n], 64);
      }

    }   // end for (i = iStart; i < iEnd; i++)

  }     /* end main loop */

/* --------------------------------  update ---------------------------- */
  ippsCopy_32s_x(sineIndxMap, &(sbr_com->S_index_mapped_prev[ch][kx]), (64 - kx));

  if (lA == LE)
    sbr_com->lA_prev[ch] = 0;
  else
    sbr_com->lA_prev[ch] = -1;

  sbr_com->L_E_prev[ch] = sbr_com->L_E[ch];
  sbr_com->L_Q_prev[ch] = sbr_com->L_Q[ch];

  sbr_com->indexNoise[ch] = fIndexNoise;
  sbr_com->indexSine[ch] = (fIndexSine + 1) & 3;
}

/********************************************************************/
/* EOF */
