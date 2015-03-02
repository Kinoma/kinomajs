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
#include<stdio.h>
#include "ippac.h"

#include "sbrdec_element.h"
#include "sbr_freq_tabs.h"
#include "sbrdec_huftabs.h"

/********************************************************************/

static Ipp32s sbr_extension(Ipp32s bs_extension_id, Ipp32s num_bits_left)
{
  bs_extension_id++;
  num_bits_left++;

  return 0;
}

/********************************************************************/

#ifndef ID_SCE
#define ID_SCE    0x0
#endif

#ifndef ID_CPE
#define ID_CPE    0x1
#endif

/********************************************************************/

static Ipp32s sbr_header(sBitsreamBuffer * BS, sSbrDecCommon* pSbrHeader)
{
  Ipp32s     cnt = 0;

  Ipp32s     bs_header_extra_1;
  Ipp32s     bs_header_extra_2;

  Ipp32s     bs_start_freq_new;
  Ipp32s     bs_stop_freq_new;
  Ipp32s     bs_freq_scale_new;
  Ipp32s     bs_alter_scale_new;
  Ipp32s     bs_xover_band_new;
  Ipp32s     bs_noise_bands_new;
  Ipp32s     bs_reserved;

  GET_BITS(BS, pSbrHeader->bs_amp_res, 1);
  GET_BITS(BS, bs_start_freq_new, 4);
  GET_BITS(BS, bs_stop_freq_new, 4);
  GET_BITS(BS, bs_xover_band_new, 3);

  GET_BITS(BS, bs_reserved, 2);

  GET_BITS(BS, bs_header_extra_1, 1);
  GET_BITS(BS, bs_header_extra_2, 1);

  cnt += 16;

  if (bs_header_extra_1) {
    GET_BITS(BS, bs_freq_scale_new, 2);
    GET_BITS(BS, bs_alter_scale_new, 1);
    GET_BITS(BS, bs_noise_bands_new, 2);

    cnt += 5;
  } else {      // default
    bs_freq_scale_new = 2;
    bs_alter_scale_new = 1;
    bs_noise_bands_new = 2;
  }

  if (bs_header_extra_2) {
    GET_BITS(BS, pSbrHeader->bs_limiter_bands, 2);
    GET_BITS(BS, pSbrHeader->bs_limiter_gains, 2);
    GET_BITS(BS, pSbrHeader->bs_interpol_freq, 1);
    GET_BITS(BS, pSbrHeader->bs_smoothing_mode, 1);

    cnt += 6;
  } else {      // default
    pSbrHeader->bs_limiter_bands = 2;
    pSbrHeader->bs_limiter_gains = 2;
    pSbrHeader->bs_interpol_freq = 1;
    pSbrHeader->bs_smoothing_mode = 1;
  }

  if ((pSbrHeader->bs_start_freq != bs_start_freq_new) ||
      (pSbrHeader->bs_stop_freq != bs_stop_freq_new) ||
      (pSbrHeader->bs_freq_scale != bs_freq_scale_new) ||
      (pSbrHeader->bs_alter_scale != bs_alter_scale_new) ||
      (pSbrHeader->bs_xover_band != bs_xover_band_new) ||
      (pSbrHeader->bs_noise_bands != bs_noise_bands_new)) {
    pSbrHeader->Reset = 1;

    pSbrHeader->bs_start_freq = bs_start_freq_new;
    pSbrHeader->bs_stop_freq = bs_stop_freq_new;
    pSbrHeader->bs_freq_scale = bs_freq_scale_new;
    pSbrHeader->bs_alter_scale = bs_alter_scale_new;
    pSbrHeader->bs_xover_band = bs_xover_band_new;
    pSbrHeader->bs_noise_bands = bs_noise_bands_new;
  } else {
    pSbrHeader->Reset = 0;
  }

  return cnt;
}

/********************************************************************/

static Ipp32s sbr_single_channel_element(sBitsreamBuffer * BS, sSbrDecCommon * pSbr)
{
  Ipp32s     ret_cnt = 0;
  Ipp32s     cnt = 0;
  Ipp32s     num_bits_left = 0;
  Ipp32s     tmp = 0;
  Ipp32s     i;
  Ipp32s     bs_data_extra = 0;

  Ipp32s     bs_reserved = 0;
  Ipp32s     bs_esc_count = 0;
  Ipp32s     bs_amp_res;
  Ipp32s     error = 0;

  GET_BITS(BS, bs_data_extra, 1);
  ret_cnt++;

  if (bs_data_extra) {

  GET_BITS(BS, bs_reserved, 4);
    ret_cnt += 4;
  }
  ret_cnt += sbr_grid(BS, &(pSbr->bs_frame_class[0]), &(pSbr->bs_pointer[0]), pSbr->r[0],
                     pSbr->tE[0], pSbr->tQ[0], &(pSbr->L_E[0]),
                     &(pSbr->L_Q[0]), &error );

  if(error)
    return SBR_ERR_REQUIREMENTS;

    //(0, BS, pSbr);
  ret_cnt += sbr_dtdf(BS, pSbr->bs_df_env[0], pSbr->bs_df_noise[0], pSbr->L_E[0], pSbr->L_Q[0]);
  ret_cnt += sbr_invf(0, BS, pSbr);
/*
 * patch
 */
  if ((pSbr->bs_frame_class[0] == FIXFIX) && (pSbr->L_E[0] == 1))
    bs_amp_res = 0;
  else
    bs_amp_res = pSbr->bs_amp_res;

  ret_cnt += sbr_envelope(0, 0, bs_amp_res, BS, pSbr);

  ret_cnt += sbr_noise(BS, pSbr->vecNoise[0], pSbr->vSizeNoise[0], pSbr->bs_df_noise[0],
    pSbr->sbrHuffTables, 0, 0, pSbr->L_Q[0], pSbr->N_Q);

  GET_BITS(BS, pSbr->bs_add_harmonic_flag[0], 1);
  ret_cnt++;
  if (pSbr->bs_add_harmonic_flag[0]) {
    ret_cnt += sbr_sinusoidal_coding( BS, pSbr->bs_add_harmonic[0], pSbr->N_high );   //(0, BS, pSbr);
  } else {
    for (i = 0; i < MAX_NUM_ENV_VAL; i++) {
      pSbr->bs_add_harmonic[0][i] = 0;
    }
  }

  GET_BITS(BS, pSbr->bs_extended_data, 1);
  ret_cnt++;

  if (pSbr->bs_extended_data) {
    GET_BITS(BS, cnt, 4);
    pSbr->bs_extension_size = cnt;
    ret_cnt += 4;

    if (cnt == 15) {
      GET_BITS(BS, bs_esc_count, 8);
      cnt += bs_esc_count;
      ret_cnt += 8;
    }

    num_bits_left = 8 * cnt;
    while (num_bits_left > 7) {
      GET_BITS(BS, pSbr->bs_extension_id, 2);
      ret_cnt += 2;
      num_bits_left -= 2;

      tmp = sbr_extension(pSbr->bs_extension_id, num_bits_left);
      ret_cnt += tmp;
      num_bits_left -= tmp;
    }
  }

  return ret_cnt;
}

/********************************************************************/

static Ipp32s sbr_channel_pair_element(sBitsreamBuffer * BS, sSbrDecCommon * pSbr)
{
  Ipp32s     ret_cnt = 0;
  Ipp32s     cnt = 0;
  Ipp32s     num_bits_left = 0;
  Ipp32s     tmp = 0;
  Ipp32s     bs_data_extra = 1;

  Ipp32s bs_reserved = 0;
  Ipp32s     bs_esc_count = 0;
  Ipp32s     ampRes;
  Ipp32s     error = 0;

  GET_BITS(BS, bs_data_extra, 1);
  ret_cnt++;

  if (bs_data_extra) {
    GET_BITS(BS, bs_reserved, 8);
    ret_cnt += 8;
  }

  GET_BITS(BS, pSbr->bs_coupling, 1);
  ret_cnt++;

  if (pSbr->bs_coupling) {
    ret_cnt += sbr_grid(BS, &(pSbr->bs_frame_class[0]), &(pSbr->bs_pointer[0]), pSbr->r[0],
      pSbr->tE[0], pSbr->tQ[0], &(pSbr->L_E[0]), &(pSbr->L_Q[0]), &error );

    if(error)
      return error;

    sbr_grid_coupling(pSbr);

    ret_cnt += sbr_dtdf(BS, pSbr->bs_df_env[0], pSbr->bs_df_noise[0], pSbr->L_E[0],
               pSbr->L_Q[0]);
    ret_cnt += sbr_dtdf(BS, pSbr->bs_df_env[1], pSbr->bs_df_noise[1], pSbr->L_E[1],
               pSbr->L_Q[1]);
    ret_cnt += sbr_invf(0, BS, pSbr);

    if ((pSbr->bs_frame_class[0] == FIXFIX) && (pSbr->L_E[0] == 1))
      ampRes = 0;
    else
      ampRes = pSbr->bs_amp_res;

    ret_cnt += sbr_envelope(0, 1, ampRes, BS, pSbr);
    ret_cnt += sbr_noise(BS, pSbr->vecNoise[0], pSbr->vSizeNoise[0], pSbr->bs_df_noise[0],
    pSbr->sbrHuffTables, 0, 1, pSbr->L_Q[0], pSbr->N_Q);

    ret_cnt += sbr_envelope(1, 1, ampRes, BS, pSbr);
    ret_cnt += sbr_noise(BS, pSbr->vecNoise[1], pSbr->vSizeNoise[1], pSbr->bs_df_noise[1],
    pSbr->sbrHuffTables, 1, 1, pSbr->L_Q[1], pSbr->N_Q);
  } else {
    ret_cnt += sbr_grid(BS, &(pSbr->bs_frame_class[0]), &(pSbr->bs_pointer[0]), pSbr->r[0],
               pSbr->tE[0], pSbr->tQ[0], &(pSbr->L_E[0]),
               &(pSbr->L_Q[0]), &error );

      if(error)
        return error;

      ret_cnt += sbr_grid(BS, &(pSbr->bs_frame_class[1]), &(pSbr->bs_pointer[1]), pSbr->r[1],
      pSbr->tE[1], pSbr->tQ[1], &(pSbr->L_E[1]), &(pSbr->L_Q[1]), &error );

       if(error)
        return error;

    ret_cnt += sbr_dtdf(BS, pSbr->bs_df_env[0], pSbr->bs_df_noise[0], pSbr->L_E[0],
      pSbr->L_Q[0]);
    ret_cnt += sbr_dtdf(BS, pSbr->bs_df_env[1], pSbr->bs_df_noise[1], pSbr->L_E[1],
      pSbr->L_Q[1]);
    ret_cnt += sbr_invf(0, BS, pSbr);
    ret_cnt += sbr_invf(1, BS, pSbr);

    if ((pSbr->bs_frame_class[0] == FIXFIX) && (pSbr->L_E[0] == 1))
      ampRes = 0;
    else
      ampRes = pSbr->bs_amp_res;
    ret_cnt += sbr_envelope(0, 0, ampRes, BS, pSbr);
    if ((pSbr->bs_frame_class[1] == FIXFIX) && (pSbr->L_E[1] == 1))
      ampRes = 0;
    else
      ampRes = pSbr->bs_amp_res;
    ret_cnt += sbr_envelope(1, 0, ampRes, BS, pSbr);

    ret_cnt += sbr_noise(BS, pSbr->vecNoise[0], pSbr->vSizeNoise[0], pSbr->bs_df_noise[0],
    pSbr->sbrHuffTables, 0, 0, pSbr->L_Q[0], pSbr->N_Q);

    ret_cnt += sbr_noise(BS, pSbr->vecNoise[1], pSbr->vSizeNoise[1], pSbr->bs_df_noise[1],
    pSbr->sbrHuffTables, 1, 0, pSbr->L_Q[1], pSbr->N_Q);
  }

  GET_BITS(BS, pSbr->bs_add_harmonic_flag[0], 1);
  ret_cnt++;
  if (pSbr->bs_add_harmonic_flag[0]) {
    ret_cnt += sbr_sinusoidal_coding(BS, pSbr->bs_add_harmonic[0], pSbr->N_high);
  } else {
    Ipp32s     i;

    for (i = 0; i < MAX_NUM_ENV_VAL; i++) {
      pSbr->bs_add_harmonic[0][i] = 0;
    }
  }
  GET_BITS(BS, pSbr->bs_add_harmonic_flag[1], 1);
  ret_cnt++;
  if (pSbr->bs_add_harmonic_flag[1]) {
    ret_cnt += sbr_sinusoidal_coding(BS, pSbr->bs_add_harmonic[1], pSbr->N_high);
  } else {
    Ipp32s     i;

    for (i = 0; i < MAX_NUM_ENV_VAL; i++) {
      pSbr->bs_add_harmonic[1][i] = 0;
    }
  }

  GET_BITS(BS, pSbr->bs_extended_data, 1);
  ret_cnt++;
  if (pSbr->bs_extended_data) {
    GET_BITS(BS, cnt, 4);
    pSbr->bs_extension_size = cnt;
    ret_cnt += 4;
    if (cnt == 15) {
      GET_BITS(BS, bs_esc_count, 8);
      ret_cnt += 8;
      cnt += bs_esc_count;
    }

    num_bits_left = 8 * cnt;
    while (num_bits_left > 7) {
      GET_BITS(BS, pSbr->bs_extension_id, 2);
      ret_cnt += 2;
      num_bits_left -= 2;

      tmp = sbr_extension(pSbr->bs_extension_id, num_bits_left);
      num_bits_left -= tmp;
      ret_cnt += tmp;
    }
  }

  return ret_cnt;
}

/********************************************************************/

static Ipp32s sbr_channel_pair_base_element(sBitsreamBuffer * BS, sSbrDecCommon * pSbr)
{
  Ipp32s     ret_cnt = 0;
  Ipp32s     cnt = 0;
  Ipp32s     num_bits_left = 0;
  Ipp32s     tmp = 0;
  Ipp32s     bs_data_extra;
  Ipp32s     bs_reserved;
  Ipp32s     bs_esc_count;
  Ipp32s     error = 0;

  GET_BITS(BS, bs_data_extra, 1);
  ret_cnt++;
  if (bs_data_extra) {
    GET_BITS(BS, bs_reserved, 8);
    ret_cnt += 8;
  }

  GET_BITS(BS, pSbr->bs_coupling, 1);
  ret_cnt++;

  ret_cnt += sbr_grid(BS, &(pSbr->bs_frame_class[0]), &(pSbr->bs_pointer[0]), pSbr->r[0],
    pSbr->tE[0], pSbr->tQ[0], &(pSbr->L_E[0]), &(pSbr->L_Q[0]), &error );

  if(error)
    return error;

  ret_cnt += sbr_dtdf(BS, pSbr->bs_df_env[0], pSbr->bs_df_noise[0], pSbr->L_E[0], pSbr->L_Q[0]);
  ret_cnt += sbr_invf(0, BS, pSbr);

  ret_cnt += sbr_envelope(0, 1, pSbr->bs_amp_res, BS, pSbr);
  ret_cnt += sbr_noise(BS, pSbr->vecNoise[0], pSbr->vSizeNoise[0], pSbr->bs_df_noise[0],
    pSbr->sbrHuffTables, 0, 1, pSbr->L_Q[0], pSbr->N_Q);

  GET_BITS(BS, pSbr->bs_add_harmonic_flag[0], 1);
  ret_cnt++;
  if (pSbr->bs_add_harmonic_flag[0]) {
    ret_cnt += sbr_sinusoidal_coding(BS, pSbr->bs_add_harmonic[0], pSbr->N_high);
  } else {
    Ipp32s     i;

    for (i = 0; i < MAX_NUM_ENV_VAL; i++) {
      pSbr->bs_add_harmonic[0][i] = 0;
    }
  }

  GET_BITS(BS, pSbr->bs_extended_data, 1);
  ret_cnt++;
  if (pSbr->bs_extended_data) {
    GET_BITS(BS, cnt, 4);
    pSbr->bs_extension_size = cnt;
    ret_cnt += 4;
    if (cnt == 15) {

   GET_BITS(BS, bs_esc_count, 8);
      ret_cnt += 8;
    }

    num_bits_left = 8 * cnt;
    while (num_bits_left > 7) {
      GET_BITS(BS, pSbr->bs_extension_id, 2);
      ret_cnt += 2;
      num_bits_left -= 2;
      tmp = sbr_extension(pSbr->bs_extension_id, num_bits_left);
      num_bits_left -= tmp;
      ret_cnt += tmp;
    }
  }

  return ret_cnt;
}

/********************************************************************/

static Ipp32s sbr_channel_pair_enhance_element(sBitsreamBuffer * BS,
                                            sSbrDecCommon * pSbr)
{
  Ipp32s     ret_cnt = 0;

  ret_cnt += sbr_dtdf(BS, pSbr->bs_df_env[1], pSbr->bs_df_noise[1], pSbr->L_E[1], pSbr->L_Q[1]);

  ret_cnt += sbr_envelope(1, 1, pSbr->bs_amp_res, BS, pSbr);
  ret_cnt += sbr_noise(BS, pSbr->vecNoise[1], pSbr->vSizeNoise[1], pSbr->bs_df_noise[1],
    pSbr->sbrHuffTables, 1, 1, pSbr->L_Q[1], pSbr->N_Q);

  GET_BITS(BS, pSbr->bs_add_harmonic_flag[1], 1);
  ret_cnt += pSbr->bs_add_harmonic_flag[1];
  ret_cnt++;
  if (pSbr->bs_add_harmonic_flag[1]) {
    ret_cnt += sbr_sinusoidal_coding(BS, pSbr->bs_add_harmonic[1], pSbr->N_high);
  } else {
    Ipp32s     i;

    for (i = 0; i < MAX_NUM_ENV_VAL; i++) {
      pSbr->bs_add_harmonic[1][i] = 0;
    }
  }

  return ret_cnt;
}

/********************************************************************/

static Ipp32s sbr_data(sBitsreamBuffer * BS, Ipp32s id_aac, sSbrDecCommon * pSbr)
{
  Ipp32s     cnt = 0;
  Ipp32s     error = 0;

  switch (pSbr->SbrLayer) {
  case SBR_NOT_SCALABLE:
    switch (id_aac) {
    case ID_SCE:

/* parser */
       //cnt = sbr_single_channel_element(BS, pSbr);
      error = sbr_single_channel_element(BS, pSbr);

      if(SBR_ERR_REQUIREMENTS == error)
        return error;
      else
        cnt = error;

/* algorithm decode */
      error = sbrEnvNoiseDec(pSbr, 0);
      if (error)
        return error;

      break;

    case ID_CPE:

/* parser */
      error = sbr_channel_pair_element(BS, pSbr);

      if(SBR_ERR_REQUIREMENTS == error)
        return error;
      else
        cnt = error;

/* algorithm decode */
      error = sbrEnvNoiseDec(pSbr, 0);
      if (error)
        return error;

      error = sbrEnvNoiseDec(pSbr, 1);
      if (error)
        return error;

      break;
    }
    break;

  case SBR_MONO_BASE:
    error = sbr_channel_pair_base_element(BS, pSbr);

    if(SBR_ERR_REQUIREMENTS == error)
        return error;
      else
        cnt = error;

    break;

  case SBR_STEREO_ENHANCE:
    error = sbr_channel_pair_enhance_element(BS, pSbr);

    if(SBR_ERR_REQUIREMENTS == error)
        return error;
      else
        cnt = error;

    break;

  case SBR_STEREO_BASE:
    error = sbr_channel_pair_element(BS, pSbr);

    if(SBR_ERR_REQUIREMENTS == error)
        return error;
      else
        cnt = error;

    break;
  }

  if (error) {
/*
 * printf("\nthere is error %i\n", error);
 */
  }
  return cnt;
}

/********************************************************************/

static Ipp32s GetValSbrLayer(void)
{
  return SBR_NOT_SCALABLE;
}

/********************************************************************/

Ipp32s sbr_extension_data(Ipp32s id_aac, Ipp32s crc_flag, sSbrDecCommon * pSbr,
                       sBitsreamBuffer * BS, Ipp32s cnt)
{
  Ipp32s     num_sbr_bits = 0;
  Ipp32s     num_align_bits;
  Ipp32s     sbr_layer = 0;
  Ipp32s     ret_val;
  Ipp32s     i;
  Ipp32s     bs_header_flag;
  Ipp32s     error = 0;
  Ipp32s     k2 = 0;
  Ipp32s     tmp;

  if (crc_flag) {
    GET_BITS(BS, pSbr->bs_sbr_crc_bits, 10);
    num_sbr_bits += 10;
  }

  sbr_layer = GetValSbrLayer();
  pSbr->SbrLayer = sbr_layer;

  if (sbr_layer != SBR_STEREO_ENHANCE) {
    num_sbr_bits += 1;
    GET_BITS(BS, bs_header_flag, 1);

    if (bs_header_flag) {
      pSbr->sbrHeaderFlagPresent++;
      num_sbr_bits += sbr_header(BS, pSbr );
    }

    if ((pSbr->sbrHeaderFlagPresent != 0) && (pSbr->Reset == 1)) {

      /*
      error = sbrCalcMasterFreqBoundary(pSbr->bs_start_freq, pSbr->bs_stop_freq,
                                        pSbr->FreqSbr, pSbr->sbr_freq_indx,
                                        &(pSbr->k0), &k2 );
     */
      error = sbrCalcMasterFreqBoundary(pSbr->bs_start_freq, pSbr->bs_stop_freq,
                                        pSbr->sbr_freq_indx, &(pSbr->k0), &k2 );


      if (error)
        return error;

      error = sbrCalcMasterFreqBandTable(       // in data
                                          pSbr->k0, k2,
                                          pSbr->bs_freq_scale,
                                          pSbr->bs_alter_scale,
                                  // out data
                                          pSbr->f_master, &(pSbr->N_master));

      if (error)
        return error;

      error = sbrCalcDerivedFreqTables( pSbr->bs_xover_band, pSbr->bs_noise_bands,
                                        k2, pSbr);

      if (error)
        return error;

      error = sbrPatchConstruction(pSbr, pSbr->sbr_freq_indx);


      if (error)
        return error;

      error = sbrCalcLimiterFreqBandTable(      // in data
                                   pSbr->bs_limiter_bands, pSbr->f_TableLow,
                                   pSbr->N_low, pSbr->numPatches,
                                   pSbr->patchNumSubbands,
                           // out data
                                   pSbr->f_TableLim, &(pSbr->N_L));
      if (error)
        return error;

    } else {
    }
  }

  if (pSbr->sbrHeaderFlagPresent != 0) {
    //num_sbr_bits += sbr_data(BS, id_aac, pSbr);
      error = sbr_data(BS, id_aac, pSbr);
      if(SBR_ERR_REQUIREMENTS == error)
        return error;
      else
        num_sbr_bits += error;

  }

  pSbr->cnt_bit -= num_sbr_bits;
  num_align_bits = 8 * cnt - 4 - num_sbr_bits;

  for (i = 0; i < (num_align_bits >> 5); i++) {
    GET_BITS(BS, tmp, 32);
  }

  if ((num_align_bits - (num_align_bits & (~0x1F))) != 0) {
    GET_BITS(BS, tmp, (num_align_bits - (num_align_bits & (~0x1F))));
  }

  ret_val = (num_sbr_bits + num_align_bits + 4) / 8;

  return ret_val;

}

/********************************************************************/

static Ipp32s middleBorder(Ipp32s bs_frame_class, Ipp32s bs_pointer, Ipp32s L_E)
{
  Ipp32s     indx1, indx2, ret_val;
  Ipp32s     TableMidBorder[3][3];

  TableMidBorder[2][0] = TableMidBorder[1][0] = TableMidBorder[0][0] = L_E / 2;
  TableMidBorder[0][1] = 1;
  TableMidBorder[1][1] = TableMidBorder[1][2] = TableMidBorder[0][2] = L_E - 1;
  TableMidBorder[2][1] = bs_pointer - 1;
  TableMidBorder[2][2] = L_E + 1 - bs_pointer;

  if ((bs_pointer > 1) || (bs_pointer < 0))
    indx1 = 2;
  else
    indx1 = bs_pointer;

  if (bs_frame_class == FIXFIX)
    indx2 = 0;
  else if (bs_frame_class == VARFIX)
    indx2 = 1;
  else
    indx2 = 2;

  ret_val = TableMidBorder[indx1][indx2];

  return ret_val;
}

/********************************************************************/

Ipp32s sbr_grid(sBitsreamBuffer * BS, Ipp32s* bs_frame_class, Ipp32s* bs_pointer, Ipp16s* r, Ipp32s* tE,
             Ipp32s* tQ, Ipp32s* LE, Ipp32s* LQ, Ipp32s* status)
{
  Ipp32s     ret_cnt = 0;
  Ipp32s     env, rel;
  Ipp32s     ptr_bits = 0;
  Ipp32s     tmp;
  Ipp32s     absBordLead = 0;
  Ipp32s     absBordTrail = 0;
  Ipp32s     bs_var_bord_0 = 0;
  Ipp32s     bs_var_bord_1 = 0;
  Ipp32s     bs_num_rel_1 = 0;
  Ipp32s     bs_num_rel_0 = 0;
  Ipp32s     n_RelLead = 0;
  Ipp32s     n_RelTrail = 0;
  Ipp32s     bs_rel_bord_0[32];
  Ipp32s     bs_rel_bord_1[32];
  Ipp32s     relBordLead[32];
  Ipp32s     relBordTrail[32];
  Ipp32s     l, i;
  Ipp32s     tmp_32s;
  const Ipp32s TABLE_PTR_BITS[6] = { 0, 1, 2, 2, 3, 3 };

  /* patch */
  *status = 0;

  GET_BITS(BS, *bs_frame_class, 2);
  ret_cnt += 2;

  switch ( *bs_frame_class ) {
  case FIXFIX:
    GET_BITS(BS, tmp, 2);
    ret_cnt += 2;
    *LE = 1 << tmp;

    /* patch */
    if( *LE > 5 ){
      *status = SBR_ERR_REQUIREMENTS;
      return SBR_ERR_REQUIREMENTS;
    }

    GET_BITS(BS, r[0], 1);
    ret_cnt++;

    for (env = 1; env < (*LE); env++) {
      r[env] = r[0];
    }

// TimeFrequency Grid
    absBordLead = 0;
    absBordTrail = NUM_TIME_SLOTS;
    n_RelLead = *LE - 1;
    n_RelTrail = 0;

     tmp_32s = NUM_TIME_SLOTS / (*LE);
    for (l = 0; l < n_RelLead; l++) {
      relBordLead[l] = tmp_32s;
    }
// relBordTrail - N/A

    break;

  case FIXVAR:
    GET_BITS(BS, bs_var_bord_1, 2);
    ret_cnt += 2;
    GET_BITS(BS, bs_num_rel_1, 2);
    ret_cnt += 2;
    *LE = bs_num_rel_1 + 1;

    /* patch */
    if( *LE > 5 ){
      *status = SBR_ERR_REQUIREMENTS;
      return SBR_ERR_REQUIREMENTS;
    }

    for (rel = 0; rel < (*LE) - 1; rel++) {
      GET_BITS(BS, tmp, 2);
      ret_cnt += 2;
      bs_rel_bord_1[rel] = 2 * tmp + 2;
    }

    ptr_bits = TABLE_PTR_BITS[ *LE ];
    GET_BITS(BS, *bs_pointer, ptr_bits);
    ret_cnt += ptr_bits;
    for (env = 0; env < (*LE); env++) {
      GET_BITS(BS, r[ *LE - 1 - env], 1);
      ret_cnt++;
    }

// TimeFrequency Grid
    absBordLead = 0;
    absBordTrail = bs_var_bord_1 + NUM_TIME_SLOTS;
    n_RelLead = 0;
    n_RelTrail = bs_num_rel_1;
// relBordLead[l] - N/A
    for (l = 0; l < n_RelTrail; l++) {
      relBordTrail[l] = bs_rel_bord_1[l];
    }

    break;

  case VARFIX:
    GET_BITS(BS, bs_var_bord_0, 2);
    ret_cnt += 2;
    GET_BITS(BS, bs_num_rel_0, 2);
    ret_cnt += 2;
    *LE = bs_num_rel_0 + 1;

    /* patch */
    if( *LE > 5 ){
      *status = SBR_ERR_REQUIREMENTS;
      return SBR_ERR_REQUIREMENTS;
    }

    for (rel = 0; rel < (*LE) - 1; rel++) {
      GET_BITS(BS, tmp, 2);
      ret_cnt += 2;
      bs_rel_bord_0[rel] = 2 * tmp + 2;
    }
    ptr_bits = TABLE_PTR_BITS[ *LE ];
    GET_BITS(BS, *bs_pointer, ptr_bits);
    ret_cnt += ptr_bits;
    for (env = 0; env < (*LE); env++) {
      GET_BITS(BS, r[env], 1);
      ret_cnt++;
    }

// TimeFrequency Grid
    absBordLead = bs_var_bord_0;
    absBordTrail = NUM_TIME_SLOTS;
    n_RelLead = bs_num_rel_0;
    n_RelTrail = 0;
    for (l = 0; l < n_RelLead; l++) {
      relBordLead[l] = bs_rel_bord_0[l];
    }
// relBordTrail - N/A

    break;

  case VARVAR:
    GET_BITS(BS, bs_var_bord_0, 2);
    ret_cnt += 2;
    GET_BITS(BS, bs_var_bord_1, 2);
    ret_cnt += 2;
    GET_BITS(BS, bs_num_rel_0, 2);
    ret_cnt += 2;
    GET_BITS(BS, bs_num_rel_1, 2);
    ret_cnt += 2;

    *LE = bs_num_rel_0 + bs_num_rel_1 + 1;

    /* patch */
    if( *LE > 5 ){
      *status = SBR_ERR_REQUIREMENTS;
      return SBR_ERR_REQUIREMENTS;
    }

    for (rel = 0; rel < bs_num_rel_0; rel++) {
      GET_BITS(BS, tmp, 2);
      ret_cnt += 2;
      bs_rel_bord_0[rel] = 2 * tmp + 2;
    }
    for (rel = 0; rel < bs_num_rel_1; rel++) {
      GET_BITS(BS, tmp, 2);
      ret_cnt += 2;
      bs_rel_bord_1[rel] = 2 * tmp + 2;
    }
// ptr_bits = (Ipp32s)ceil( log(pSbr->L_E[ch]+1.f)/log(2.) );
    ptr_bits = TABLE_PTR_BITS[ *LE ];
    GET_BITS(BS, *bs_pointer, ptr_bits);
    ret_cnt += ptr_bits;
    for (env = 0; env < (*LE); env++) {
      GET_BITS(BS, r[env], 1);
      ret_cnt++;
    }

// TimeFrequency Grid
    absBordLead = bs_var_bord_0;
    absBordTrail = bs_var_bord_1 + NUM_TIME_SLOTS;
    n_RelLead = bs_num_rel_0;
    n_RelTrail = bs_num_rel_1;
    for (l = 0; l < n_RelLead; l++) {
      relBordLead[l] = bs_rel_bord_0[l];
    }
    for (l = 0; l < n_RelTrail; l++) {
      relBordTrail[l] = bs_rel_bord_1[l];
    }

    break;
  }     // end of switch

  *LQ = ((*LE) > 1) ? 2 : 1;

// TimeFrequency Grid
  tE[0] = absBordLead;
  tE[ *LE ] = absBordTrail;

  for (l = 1; l <= n_RelLead; l++) {
    tmp_32s = 0;
    for (i = 0; i < l; i++) {
      tmp_32s += relBordLead[i];
    }
    tE[l] = absBordLead + tmp_32s;
  }
  for (l = n_RelLead + 1; l < (*LE); l++) {
    tmp_32s = 0;
    for (i = 0; i < (*LE) - l; i++) {
      tmp_32s += relBordTrail[i];
    }
    tE[l] = absBordTrail - tmp_32s;
  }

  if ( (*LE) == 1) {
    tQ[0] = tE[0];
    tQ[1] = tE[1];
  } else {
    tQ[0] = tE[0];
    tmp_32s =
      middleBorder( *bs_frame_class, *bs_pointer, *LE);
    tQ[1] = tE[tmp_32s];
    tQ[2] = tE[ *LE ];
  }

  return ret_cnt;
}

/********************************************************************/

void sbr_grid_coupling(sSbrDecCommon * pSbr)
{
  Ipp32s     env, l;

  pSbr->bs_frame_class[1] = pSbr->bs_frame_class[0];

  switch (pSbr->bs_frame_class[1]) {
  case FIXFIX:
    pSbr->L_E[1] = pSbr->L_E[0];
    pSbr->r[1][0] = pSbr->r[0][0];
    for (env = 0; env < pSbr->L_E[1]; env++) {
      pSbr->r[1][env] = pSbr->r[1][0];
    }

    break;

  case FIXVAR:
    pSbr->L_E[1] = pSbr->L_E[0];

    pSbr->bs_pointer[1] = pSbr->bs_pointer[0];
    for (env = 0; env < pSbr->L_E[1]; env++)
      pSbr->r[1][pSbr->L_E[1] - 1 - env] = pSbr->r[0][pSbr->L_E[1] - 1 - env];

    break;

  case VARFIX:
    pSbr->L_E[1] = pSbr->L_E[0];

    pSbr->bs_pointer[1] = pSbr->bs_pointer[0];
    for (env = 0; env < pSbr->L_E[1]; env++)
      pSbr->r[1][env] = pSbr->r[0][env];

    break;

  case VARVAR:
    pSbr->L_E[1] = pSbr->L_E[0];
    pSbr->bs_pointer[1] = pSbr->bs_pointer[0];
    for (env = 0; env < pSbr->L_E[1]; env++)
      pSbr->r[1][env] = pSbr->r[0][env];

    break;
  }

  pSbr->L_Q[1] = pSbr->L_Q[0];

  for (l = 0; l <= pSbr->L_E[1]; l++) {
    pSbr->tE[1][l] = pSbr->tE[0][l];
  }

  for (l = 0; l <= pSbr->L_Q[1]; l++) {
    pSbr->tQ[1][l] = pSbr->tQ[0][l];
  }
}

/********************************************************************/

Ipp32s sbr_dtdf(sBitsreamBuffer * BS, Ipp32s* bs_df_env, Ipp32s* bs_df_noise, Ipp32s LE, Ipp32s LQ)
{
  Ipp32s     ret_cnt = 0;
  Ipp32s     env;
  Ipp32s     noise;

  for (env = 0; env < LE; env++) {
    GET_BITS(BS, bs_df_env[env], 1);
    ret_cnt++;
  }
  for (noise = 0; noise < LQ; noise++) {
    GET_BITS(BS, bs_df_noise[noise], 1);
    ret_cnt++;
  }

  return ret_cnt;
}

/********************************************************************/

Ipp32s sbr_invf(Ipp32s ch, sBitsreamBuffer * BS, sSbrDecCommon * pSbr)
{
  Ipp32s     ret_cnt = 0;
  Ipp32s     n;
  Ipp32s     num_noise_bands = pSbr->N_Q;

  if (pSbr->bs_coupling == 1) {
    for (n = 0; n < num_noise_bands; n++) {
      GET_BITS(BS, pSbr->bs_invf_mode[ch][n], 2);
      pSbr->bs_invf_mode[1][n] = pSbr->bs_invf_mode[ch][n];
      ret_cnt += 2;
    }
  } else {
    for (n = 0; n < num_noise_bands; n++) {
      GET_BITS(BS, pSbr->bs_invf_mode[ch][n], 2);
      ret_cnt += 2;
    }
  }

  return ret_cnt;
}

/********************************************************************/

static Ipp16s sbr_huff_dec(void *t_huff, sBitsreamBuffer * pBS, Ipp32s *cnt, Ipp16s LAV)
{
  Ipp16s   index = 0;
  Ipp32s     tmp = pBS->nBit_offset;
  Ipp32s     num_bit_read = 0;
  Ipp8u  *pSrc;
  Ipp32s     bitoffset;

  pSrc = (Ipp8u *)pBS->pCurrent_dword + ((32 - pBS->nBit_offset) >> 3);
  bitoffset = (32 - pBS->nBit_offset) & 0x7;

  ippsVLCDecodeOne_1u16s_x(&pSrc, &bitoffset, &index,
                         (IppsVLCDecodeSpec_32s *) t_huff);

  pBS->pCurrent_dword = (Ipp32u *)(pSrc - ((Ipp32s)(pSrc) & 3));
  pBS->dword = BSWAP(pBS->pCurrent_dword[0]);
  pBS->nBit_offset =
    32 - ((pSrc - (Ipp8u *)pBS->pCurrent_dword) << 3) - bitoffset;
/*
 * patch
 */
  if (pBS->nBit_offset < tmp)
    num_bit_read = tmp - pBS->nBit_offset;
  else
    num_bit_read = tmp + (32 - pBS->nBit_offset);

  *cnt = (*cnt) + num_bit_read;

  return index - LAV;
}

/********************************************************************/

Ipp32s sbr_envelope(Ipp32s ch, Ipp32s bs_coupling, Ipp32s bs_amp_res, sBitsreamBuffer * BS,
                 sSbrDecCommon * pSbr)
{
  Ipp32s     env;
  Ipp32s     ret_cnt = 0;
  Ipp32s     band;
  Ipp32s     num_env_bands[2];     // = {pSbr->n[0], pSbr->n[1]};
  Ipp16s     bs_env_start_value_balance;
  Ipp16s     bs_env_start_value_level;
  void   *t_huff;
  void   *f_huff;
  Ipp16s     LAV;
  Ipp32s*   sizeEnv = pSbr->vSizeEnv[ch];

  num_env_bands[0] = pSbr->N_low;// n[0];
  num_env_bands[1] = pSbr->N_high;//  n[1];

  /* patch */
  sizeEnv[0] = 0;
  for(env = 1; env <= pSbr->L_E[ch]; env++)
    sizeEnv[env] = sizeEnv[env-1] + num_env_bands[pSbr->r[ch][env-1]];
  // LE - total size

  if (bs_coupling) {
    if (ch) {
      if (bs_amp_res) {
        t_huff = pSbr->sbrHuffTables[6];        // t_huffman_env_bal_3_0dB;//
        f_huff = pSbr->sbrHuffTables[7];        // f_huffman_env_bal_3_0dB;//
        LAV = 12;
      } else {
        t_huff = pSbr->sbrHuffTables[2];        // t_huffman_env_bal_1_5dB;//
        f_huff = pSbr->sbrHuffTables[3];        // f_huffman_env_bal_1_5dB;//
        LAV = 24;
      }
    } else {
      if (bs_amp_res) {
        t_huff = pSbr->sbrHuffTables[4];        // t_huffman_env_3_0dB;//
        f_huff = pSbr->sbrHuffTables[5];        // f_huffman_env_3_0dB;//
        LAV = 31;
      } else {
        t_huff = pSbr->sbrHuffTables[0];        // t_huffman_env_1_5dB;//
        f_huff = pSbr->sbrHuffTables[1];        // f_huffman_env_1_5dB;//
        LAV = 60;
      }
    }
  } else {
    if (bs_amp_res) {
      t_huff = pSbr->sbrHuffTables[4];  // t_huffman_env_3_0dB;//
      f_huff = pSbr->sbrHuffTables[5];  // f_huffman_env_3_0dB;//
      LAV = 31;
    } else {
      t_huff = pSbr->sbrHuffTables[0];  // t_huffman_env_1_5dB;//
      f_huff = pSbr->sbrHuffTables[1];  // f_huffman_env_1_5dB;//
      LAV = 60;
    }

  }

  for (env = 0; env < pSbr->L_E[ch]; env++) {
    if (pSbr->bs_df_env[ch][env] == 0) {
      if (bs_coupling && ch) {
        if (bs_amp_res) {
          GET_BITS(BS, bs_env_start_value_balance, 5);
          pSbr->vecEnv[ch][sizeEnv[env]+0] = bs_env_start_value_balance;
          ret_cnt += 5;
        } else {
          GET_BITS(BS, bs_env_start_value_balance, 6);
          pSbr->vecEnv[ch][sizeEnv[env]+0] = bs_env_start_value_balance;
          ret_cnt += 6;
        }
      } else {
        if (bs_amp_res) {
          GET_BITS(BS, bs_env_start_value_level, 6);
          pSbr->vecEnv[ch][sizeEnv[env]+0] = bs_env_start_value_level;
          ret_cnt += 6;
        } else {
          GET_BITS(BS, bs_env_start_value_level, 7);
          pSbr->vecEnv[ch][sizeEnv[env]+0] = bs_env_start_value_level;
          ret_cnt += 7;
        }
      }
      for (band = 1; band < num_env_bands[pSbr->r[ch][env]]; band++) {
        pSbr->vecEnv[ch][sizeEnv[env]+band] =
          sbr_huff_dec(f_huff, BS, &ret_cnt, LAV);
      }
    } else {
      for (band = 0; band < num_env_bands[pSbr->r[ch][env]]; band++) {
        pSbr->vecEnv[ch][sizeEnv[env]+band] =
          sbr_huff_dec(t_huff, BS, &ret_cnt, LAV);
      }
    }
  }

  return ret_cnt;
}

/********************************************************************/

Ipp32s sbr_noise(sBitsreamBuffer* BS, Ipp16s* vNoise, Ipp32s* vSize, Ipp32s* bs_df_noise,
              void* sbrHuffTables[10], Ipp32s ch, Ipp32s bs_coupling, Ipp32s LQ, Ipp32s NQ)
{
  Ipp32s     ret_cnt = 0;
  Ipp32s     noise;
  Ipp32s     band;
  Ipp16s     bs_noise_start_value_balance;
  Ipp16s     bs_noise_start_value_level;
  void   *t_huff;
  void   *f_huff;
  Ipp16s     LAV;

  vSize[0] = 0;
  vSize[1] = NQ;
  // total size
  vSize[2] = 2*NQ;

  if (bs_coupling) {
    if (ch) {
      t_huff = sbrHuffTables[9];  // t_huffman_noise_bal_3_0dB;//
      f_huff = sbrHuffTables[7];  // f_huffman_env_bal_3_0dB;//
      LAV = 12;
    } else {
      t_huff = sbrHuffTables[8];  // t_huffman_noise_3_0dB;//
      f_huff = sbrHuffTables[5];  // f_huffman_env_3_0dB;//
      LAV = 31;
    }
  } else {
    t_huff = sbrHuffTables[8];    // t_huffman_noise_3_0dB;//
    f_huff = sbrHuffTables[5];    // f_huffman_env_3_0dB;//
    LAV = 31;
  }

  for (noise = 0; noise < LQ; noise++) {
    if (bs_df_noise[noise] == 0) {
      if (bs_coupling && ch) {
        GET_BITS(BS, bs_noise_start_value_balance, 5);
        vNoise[vSize[noise]] = bs_noise_start_value_balance;

      } else {
        GET_BITS(BS, bs_noise_start_value_level, 5);
        vNoise[vSize[noise]] = bs_noise_start_value_level;
      }
      ret_cnt += 5;

      for (band = 1; band < NQ; band++) {
        vNoise[vSize[noise] + band] =
          sbr_huff_dec(f_huff, BS, &ret_cnt, LAV);
      }
    } else {
      for (band = 0; band < NQ; band++) {
        vNoise[vSize[noise]+band] =
          sbr_huff_dec(t_huff, BS, &ret_cnt, LAV);
      }
    }
  }

  return ret_cnt;
}

/********************************************************************/

Ipp32s sbr_sinusoidal_coding(sBitsreamBuffer * BS, Ipp32s* pDst, Ipp32s len)
{
  Ipp32s     n;
  Ipp32s     ret_cnt = 0;

  for (n = 0; n < len; n++) {
    GET_BITS(BS, pDst[n], 1);
  }

  ret_cnt = len;

  return ret_cnt;
}

/********************************************************************/
/* EOF */
