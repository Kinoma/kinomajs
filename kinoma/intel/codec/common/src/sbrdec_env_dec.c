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

#include<ipps.h>
#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include"sbrdec_element.h"

/********************************************************************
 * this code may be integrated to Huffman decode
 ********************************************************************/

static Ipp32s LookUpI_parity(Ipp32s *f_TableHigh, Ipp32s *f_TableLow, Ipp32s k, Ipp32s N_High)
{
  Ipp32s     i = 0;

  while ((f_TableHigh[i] != f_TableLow[k]) && (i <= N_High))
    i++;

  return i;

}

/********************************************************************/

static Ipp32s LookUpI_disparity(Ipp32s *f_TableHigh, Ipp32s *f_TableLow, Ipp32s k,
                             Ipp32s N_Low)
{
  Ipp32s     i, ret_val = 0;

  for (i = 0; i <= N_Low - 1; i++) {
    if ( (f_TableLow[i] <= f_TableHigh[k]) &&
        (f_TableLow[i + 1] > f_TableHigh[k]) )
    {
      ret_val = i;
      break;
    }
  }

  return ret_val;
}

/********************************************************************/

Ipp32s sbrEnvNoiseDec(sSbrDecCommon * pSbr, Ipp32s ch)    // optimization is needed!!!
{
  Ipp16s     delta;
  Ipp32s     i, l, k, n_r;
  Ipp16s     g_E, g;
  Ipp32s     flag_switch;
  Ipp32s     resolution[2];

  Ipp32s  L_Q    = pSbr->L_Q[ch];
  Ipp32s  N_Low  = pSbr->N_low;
  Ipp32s  N_High = pSbr->N_high;
  Ipp32s  N_Q    = pSbr->N_Q;

  Ipp32s* pos     = pSbr->vSizeEnv[ch];
  Ipp32s* posN    = pSbr->vSizeNoise[ch];
  Ipp16s* vEnv    = pSbr->vecEnv[ch];
  Ipp16s* vNoise  = pSbr->vecNoise[ch];
  Ipp16s* r       = pSbr->r[ch];
  Ipp32s* pTable1 = pSbr->f_TableHigh;
  Ipp32s* pTable0 = pSbr->f_TableLow;

  /* check */
  if( pSbr->L_E[ch] > MAX_NUM_ENV)
    return SBR_ERR_REQUIREMENTS;

  resolution[0] = N_Low;
  resolution[1] = N_High;

  delta = ((ch == 1) && (pSbr->bs_coupling == 1)) ? 2 : 1;

/*
 * calculate for l == 0
 */
  n_r = resolution[r[0]];
  g   = pSbr->r_prev[ch][pSbr->L_E_prev[ch] - 1];
  flag_switch = pSbr->bs_df_env[ch][0] * (r[0] - g + 2);

  switch (flag_switch) {

  case 0: // bs_df_env[0] = 0
    vEnv[0] = delta*vEnv[0];

    for (k = 1; k < n_r; k++) {
     vEnv[k] =vEnv[k-1] +vEnv[k]*delta;
    }
    break;

  case 2: // bs_df_env[0] = 1 and r(l)=g(l)
    for (k = 0; k < n_r; k++) {
      g_E    = pSbr->vecEnvPrev[ch][k];
     vEnv[k] = g_E + delta * (vEnv[k]);
    }
    break;

  case 1: // bs_df_env[0] = 1 and r(l)=0 and g(l)=1
    for (k = 0; k < n_r; k++) {
      i      = LookUpI_parity(pTable1, pTable0, k, N_High);
      g_E    = pSbr->vecEnvPrev[ch][i];
     vEnv[k] = g_E + delta * (vEnv[k]);
    }
    break;

  case 3: // bs_df_env[0] = 1 and r(l)=1 and g(l)=0
    for (k = 0; k < n_r; k++) {
      i      = LookUpI_disparity(pTable1, pTable0, k, N_Low);
      g_E    = pSbr->vecEnvPrev[ch][i];
     vEnv[k] = g_E + delta * (vEnv[k]);
    }
  }
/*
 * END!!! l==0 END!!!
 */

/*
 * calcilate for l=1:L_E
 */
  for (l = 1; l < pSbr->L_E[ch]; l++) {
    n_r = resolution[r[l]];
    g   = r[l - 1];
    flag_switch = pSbr->bs_df_env[ch][l] * (r[l] - g + 2);


    switch (flag_switch) {

    case 0: // bs_df_env[l] = 0

     vEnv[pos[l]] = delta*vEnv[pos[l]];
      for (k = 1; k < n_r; k++) {
       vEnv[pos[l]+k] =vEnv[pos[l]+k-1] +vEnv[pos[l]+k] * delta;
      }
      break;

    case 2: // bs_df_env[l] = 1 and r(l)=g(l)
      for (k = 0; k < n_r; k++) {
        g_E =vEnv[pos[l-1]+k];
       vEnv[pos[l]+k] = g_E + delta * (vEnv[pos[l]+k]);
      }
      break;

    case 1: // bs_df_env[l] = 1 and r(l)=0 and g(l)=1
      for (k = 0; k < n_r; k++) {
        i   = LookUpI_parity(pTable1, pTable0, k, N_High);
        g_E = vEnv[pos[l-1]+i];
       vEnv[pos[l]+k] = g_E + delta * (vEnv[pos[l]+k]);
      }
      break;

    case 3: // bs_df_env[l] = 1 and r(l)=1 and g(l)=0
      for (k = 0; k < n_r; k++) {
        i   = LookUpI_disparity(pTable1, pTable0, k, N_Low);
        g_E = vEnv[pos[l-1]+i];
       vEnv[pos[l]+k] = g_E + delta * (vEnv[pos[l]+k]);
      }
    }   // end switch
  }     // end for
/* step(2): noise_dec */

  if (pSbr->bs_df_noise[ch][0] == 1)    // and l==0
  {
    for (k = 0; k < N_Q; k++) {
      vNoise[posN[0]+k] = pSbr->vecNoisePrev[ch][k] + delta * (vNoise[pos[0]+k]);
    }
  } else { // if(pSbr->SbrBSE.bs_df_noise[ch][0] == 0) and l==0

    vNoise[posN[0]+0] = delta*vNoise[posN[0]+0];
    for (k = 1; k < N_Q; k++) {
      vNoise[posN[0]+k] = vNoise[posN[0]+k-1] + delta * vNoise[posN[0]+k];
    }
  }

// noise
  for (l = 1; l < L_Q; l++) {
    if (pSbr->bs_df_noise[ch][l] == 0) {

      vNoise[posN[l]] = delta * vNoise[posN[l]];
      for (k = 1; k < N_Q; k++) {
        vNoise[posN[l]+k] = delta * vNoise[posN[l]+k] + vNoise[posN[l]+k-1];
      }
    } else {
      for (k = 0; k < N_Q; k++) {
        vNoise[posN[l]+k] = vNoise[posN[l-1]+k] + delta * (vNoise[posN[l]+k]);
      }
    }
  }

/* --------------------------------  update ---------------------------- */
  {
    int size = sizeof(Ipp16s);

    ippsZero_8u_x((Ipp8u *)pSbr->vecEnvPrev[ch],   size * MAX_NUM_ENV_VAL);
    ippsZero_8u_x((Ipp8u *)pSbr->vecNoisePrev[ch], size * MAX_NUM_ENV_VAL);
    ippsZero_8u_x((Ipp8u *)pSbr->r_prev[ch],       size * MAX_NUM_ENV);

    ippsCopy_8u_x((const Ipp8u*)r, (Ipp8u*)pSbr->r_prev[ch], size * pSbr->L_E[ch]);
  }

  l   = pSbr->L_E[ch] - 1;
  n_r = resolution[r[l]];
  for (k = 0; k < n_r; k++) {
    pSbr->vecEnvPrev[ch][k] =vEnv[pos[l]+k];
  }

  l = L_Q - 1;
  for (k = 0; k < N_Q; k++)
    pSbr->vecNoisePrev[ch][k] = vNoise[posN[l]+k];

  return 0;     // OK
}

/********************************************************************/
/* EOF */
