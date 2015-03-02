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

#include"sbrdec_element.h"

/********************************************************************/

// it is usefull function is used by sbrdec_parser (not implemented yet)
static Ipp32s  sbr_fill_default_header(sSbrDecCommon* pSbrHeader)
{
  pSbrHeader->bs_freq_scale     = 2;
  pSbrHeader->bs_alter_scale    = 1;
  pSbrHeader->bs_noise_bands    = 2;
  pSbrHeader->bs_limiter_bands  = 2;
  pSbrHeader->bs_limiter_gains  = 2;
  pSbrHeader->bs_interpol_freq  = 1;
  pSbrHeader->bs_smoothing_mode = 1;
  pSbrHeader->bs_start_freq     = 5;
  pSbrHeader->bs_amp_res        = 1;

  pSbrHeader->Reset = 1;

  return 0; //OK
}

/********************************************************************/

Ipp32s sbrdecResetCommon( sSbrDecCommon* pSbr )
{
  IppStatus status;
  int size = 2 * sizeof(Ipp32s);

  /* init FreqIndx */
  pSbr->sbr_freq_indx = -1;

  /* reset header to default param  */
  sbr_fill_default_header( pSbr );

  /* reset param */
  pSbr->transitionBand[0] = 0;
  pSbr->transitionBand[1] = 0;
  pSbr->kx_prev = 32;
  pSbr->kx = 32;

  pSbr->indexNoise[0] = 0;
  pSbr->indexNoise[1] = 0;
  pSbr->indexSine[0] = 0;
  pSbr->indexSine[1] = 0;

  // it is right
  //pSbr->sbrHeaderFlagPresent = 0;

  pSbr->FlagUpdate[0] = pSbr->FlagUpdate[1] = 1;

  pSbr->N_Q_prev = 0;
  pSbr->L_E_prev[0] = pSbr->L_E_prev[1] = 1;
  pSbr->L_Q_prev[0] = pSbr->L_Q_prev[1] = 1;
  pSbr->lA_prev[0] = pSbr->lA_prev[1] = -1;

  /* reset buffer */
  status = ippsZero_8u_x((Ipp8u *)&(pSbr->r_prev[0][0]),   sizeof(Ipp16s) * MAX_NUM_ENV);
  status = ippsZero_8u_x((Ipp8u *)&(pSbr->S_index_mapped_prev[0][0]), size * MAX_NUM_ENV_VAL);
  status = ippsZero_8u_x((Ipp8u *)&(pSbr->bs_invf_mode_prev[0][0]),     size* MAX_NUM_NOISE_VAL);

  return 0;//OK
}

/********************************************************************/
