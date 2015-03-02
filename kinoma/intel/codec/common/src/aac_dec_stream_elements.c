/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives AAC Decode Sample for Windows*
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel(R) Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel(R) IPP
//  product installation for more information.
//
//  MPEG-4 and AAC are international standards promoted by ISO, IEC, ITU, ETSI
//  and other organizations. Implementations of these standards, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
*/

/********************************************************************/
//***
#include "kinoma_ipp_lib.h"

#include "aac_dec_huff_tables.h"
#include "aac_dec_own.h"

/********************************************************************/

int     dec_ics_info(s_SE_Individual_channel_stream *pData,
                     sBitsreamBuffer *pBS,
                     enum AudioObjectType audioObjectType);

int     ics_info_copy(s_SE_Individual_channel_stream *pDataSrc,
                      s_SE_Individual_channel_stream *pDataDst,
                      enum AudioObjectType audioObjectType);

int     dec_section_data(s_SE_Individual_channel_stream *pData,
                         sBitsreamBuffer *pBS);
int     dec_scale_factor_data(s_SE_Individual_channel_stream *pData,
                              short scalef[8][51],
                              int scale_factor,
                              int noise_nrg,
                              sBitsreamBuffer *pBS);
int     dec_spectral_data(s_SE_Individual_channel_stream *pData,
                          sBitsreamBuffer *BS);
int     dec_pulse_data(s_SE_Individual_channel_stream *pData,
                       sBitsreamBuffer *pBS);
int     dec_tns_data(s_SE_Individual_channel_stream *pData,
                     sBitsreamBuffer *pBS);
int     dec_gain_control_data(s_SE_Individual_channel_stream *pData,
                              sBitsreamBuffer *pBS);
int     dec_ltp_data(s_SE_Individual_channel_stream *pData,
                     sBitsreamBuffer *pBS,
                     enum AudioObjectType audioObjectType);

/********************************************************************/

int dec_cpe_channel_element(sCpe_channel_element *pElement,
                            sBitsreamBuffer *pBS,
                            enum AudioObjectType audioObjectType)
{
  int     g;
  int     sfb;
  int     max_sfb;
  int     num_window_groups;

  GET_BITS(pBS, pElement->element_instance_tag, 4)

  GET_BITS(pBS, pElement->common_window, 1)

  if (pElement->common_window) {
    if (dec_ics_info(&pElement->streams[0], pBS, audioObjectType) < 0)
      return -1;

    ics_info_copy(&pElement->streams[0], &pElement->streams[1],
                  audioObjectType);

    if (audioObjectType == AOT_AAC_LTP &&
        EIGHT_SHORT_SEQUENCE != pElement->streams[1].window_sequence) {
      pElement->streams[1].ltp_data_present = 0;
      if (pElement->streams[1].predictor_data_present) {
        GET_BITS(pBS, pElement->streams[1].ltp_data_present, 1)
        if (pElement->streams[1].ltp_data_present) {
          dec_ltp_data(&pElement->streams[1], pBS, audioObjectType);
        }
      }
    }

    GET_BITS(pBS, pElement->ms_mask_present, 2)
    if (pElement->ms_mask_present == 1) {
      max_sfb = pElement->streams[0].max_sfb;
      num_window_groups = pElement->streams[0].num_window_groups;
      for (g = 0; g < num_window_groups; g++) {
        for (sfb = 0; sfb < max_sfb; sfb++) {
          GET_BITS(pBS, pElement->ms_used[g][sfb], 1)
        }
      }
    }
  } else {
    pElement->ms_mask_present = 0;
  }
  if (dec_individual_channel_stream
      (&pElement->streams[0], pBS, pElement->common_window, 0,
       audioObjectType) < 0)
    return -1;
  if (dec_individual_channel_stream
      (&pElement->streams[1], pBS, pElement->common_window, 0,
       audioObjectType) < 0)
    return -1;

  return 0;
}

/********************************************************************/

int dec_sce_channel_element(sSce_channel_element *pElement,
                            sBitsreamBuffer *pBS,
                            enum AudioObjectType audioObjectType)
{
  GET_BITS(pBS, pElement->element_instance_tag, 4)

  if (dec_individual_channel_stream
      (&pElement->stream, pBS, 0, 0, audioObjectType) < 0)
    return -1;

  return 0;
}

/********************************************************************/

int dec_lfe_channel_element(sLfe_channel_element *pElement,
                            sBitsreamBuffer *pBS,
                            enum AudioObjectType audioObjectType)
{
  GET_BITS(pBS, pElement->element_instance_tag, 4)

  if (dec_individual_channel_stream
      (&pElement->stream, pBS, 0, 0, audioObjectType) < 0)
    return -1;

  return 0;
}

/********************************************************************/

int dec_individual_channel_stream(s_SE_Individual_channel_stream *pData,
                                  sBitsreamBuffer *pBS,
                                  int common_window,
                                  int scal_flag,
                                  enum AudioObjectType audioObjectType)
{
  GET_BITS(pBS, pData->global_gain, 8)

  if (!common_window && !scal_flag) {
    if (dec_ics_info(pData, pBS, audioObjectType) < 0)
      return -1;
  }

  if (dec_section_data(pData, pBS) < 0)
    return -1;

  dec_scale_factor_data(pData, pData->sf, pData->global_gain,
                        pData->global_gain - 90, pBS);

  if (!scal_flag) {

    GET_BITS(pBS, pData->pulse_data_present, 1)
    if (pData->pulse_data_present) {
      if (dec_pulse_data(pData, pBS) < 0)
        return -1;
    }

    GET_BITS(pBS, pData->tns_data_present, 1)
    if (pData->tns_data_present) {
      dec_tns_data(pData, pBS);
    }
    GET_BITS(pBS, pData->gain_control_data_present, 1)
    if (pData->gain_control_data_present) {
      if (AOT_AAC_SSR == audioObjectType) {
        dec_gain_control_data(pData, pBS);
      } else {
        return -1;
      }
    }
  }

  dec_spectral_data(pData, pBS);

  return 0;
}

/********************************************************************/

int dec_ics_info(s_SE_Individual_channel_stream *pData,
                 sBitsreamBuffer *pBS,
                 enum AudioObjectType audioObjectType)
{
  int     i;
  int     pred_min_sfb;

  GET_BITS(pBS, pData->ics_reserved_bit, 1)
  GET_BITS(pBS, pData->window_sequence, 2)
  GET_BITS(pBS, pData->window_shape, 1)

  pData->num_window_groups = 1;
  pData->len_window_group[0] = 1;
  pData->predictor_reset = 0;
  pData->ltp_data_present = 0;

  if (pData->window_sequence == EIGHT_SHORT_SEQUENCE) {
    pData->num_windows = 8;
    GET_BITS(pBS, pData->max_sfb, 4)
    if (pData->num_swb_short < pData->max_sfb)
      return -1;
    for (i = 0; i < 7; i++) {
      GET_BITS(pBS, pData->scale_factor_grouping[i], 1)

      if (pData->scale_factor_grouping[i] == 0) {
        pData->len_window_group[pData->num_window_groups] = 1;
        pData->num_window_groups++;
      } else {
        pData->len_window_group[pData->num_window_groups - 1]++;

      }
    }

  } else {
    pData->num_windows = 1;
    GET_BITS(pBS, pData->max_sfb, 6)
    if (pData->num_swb_long < pData->max_sfb)
      return -1;
    GET_BITS(pBS, pData->predictor_data_present, 1)
    if (pData->predictor_data_present) {
      if (audioObjectType == AOT_AAC_MAIN) {
        GET_BITS(pBS, pData->predictor_reset, 1)
        if (pData->predictor_reset) {
          GET_BITS(pBS, pData->predictor_reset_group_number, 5)
        }
        pred_min_sfb =
          pData->max_sfb <
          pData->pred_max_sfb ? pData->max_sfb : pData->pred_max_sfb;
        for (i = 0; i < pred_min_sfb; i++) {
          GET_BITS(pBS, pData->prediction_used[i], 1)
        }
        for (i = pred_min_sfb; i < pData->pred_max_sfb; i++) {
          pData->prediction_used[i] = 0;
        }
      } else if (audioObjectType == AOT_AAC_LTP) {
        GET_BITS(pBS, pData->ltp_data_present, 1)
        if (pData->ltp_data_present) {
//                    dbg_trace("ltp_data_present\n");
          dec_ltp_data(pData, pBS, audioObjectType);
        }
      } else {
        return -1;
      }
    } else {
      if (audioObjectType == AOT_AAC_MAIN) {
        for (i = 0; i < pData->pred_max_sfb; i++) {
          pData->prediction_used[i] = 0;
        }
      }
    }
  }

  return 0;
}

/********************************************************************/

int ics_info_copy(s_SE_Individual_channel_stream *pDataSrc,
                  s_SE_Individual_channel_stream *pDataDst,
                  enum AudioObjectType audioObjectType)
{
  int     i;

  pDataDst->ics_reserved_bit = pDataSrc->ics_reserved_bit;
  pDataDst->window_sequence = pDataSrc->window_sequence;
  pDataDst->window_shape = pDataSrc->window_shape;

  pDataDst->num_window_groups = 1;
  pDataDst->len_window_group[0] = 1;

  if (pDataSrc->window_sequence == EIGHT_SHORT_SEQUENCE) {
    pDataDst->num_windows = 8;
    pDataDst->max_sfb = pDataSrc->max_sfb;

    for (i = 0; i < 7; i++) {
      pDataDst->scale_factor_grouping[i] = pDataSrc->scale_factor_grouping[i];

      if (pDataDst->scale_factor_grouping[i] == 0) {
        pDataDst->len_window_group[pDataDst->num_window_groups] = 1;
        pDataDst->num_window_groups++;
      } else {
        pDataDst->len_window_group[pDataDst->num_window_groups - 1]++;

      }
    }

  } else {
    pDataDst->num_windows = 1;
    pDataDst->max_sfb = pDataSrc->max_sfb;
    pDataDst->predictor_data_present = pDataSrc->predictor_data_present;

    if (audioObjectType == AOT_AAC_MAIN) {
      pDataDst->predictor_reset = pDataSrc->predictor_reset;
      pDataDst->predictor_reset_group_number =
        pDataSrc->predictor_reset_group_number;
      for (i = 0; i < pDataDst->pred_max_sfb; i++) {
        pDataDst->prediction_used[i] = pDataSrc->prediction_used[i];
      }
    }
  }

  return 0;
}

/********************************************************************/

int dec_ltp_data(s_SE_Individual_channel_stream *pData,
                 sBitsreamBuffer *pBS,
                 enum AudioObjectType audioObjectType)
{
  int     i;
  int     w;
  int     pred_max_sfb;

  pred_max_sfb =
    MAX_LTP_SFB_LONG < pData->max_sfb ? MAX_LTP_SFB_LONG : pData->max_sfb;
  if (audioObjectType == AOT_ER_AAC_LD) {
    GET_BITS(pBS, pData->ltp_lag_update, 1)
    if (pData->ltp_lag_update) {
      GET_BITS(pBS, pData->ltp_lag, 10)
    } else {
    }
    GET_BITS(pBS, pData->ltp_coef, 3)
    for (i = 0; i < pred_max_sfb; i++) {
      GET_BITS(pBS, pData->ltp_long_used[i], 1)
    }
  } else {
    GET_BITS(pBS, pData->ltp_lag, 11)
    GET_BITS(pBS, pData->ltp_coef, 3)

    if (pData->window_sequence == EIGHT_SHORT_SEQUENCE) {
      for (w = 0; w < pData->num_windows; w++) {
        GET_BITS(pBS, pData->ltp_short_used[w], 1)
        if (pData->ltp_short_used[w]) {
          GET_BITS(pBS, pData->ltp_short_lag_present[w], 1)
          if (pData->ltp_short_lag_present[w]) {
            GET_BITS(pBS, pData->ltp_short_lag[w], 4)
          } else {
            pData->ltp_short_lag[w] = 0;
          }

        }
      }

    } else {
      for (i = 0; i < pred_max_sfb; i++) {
        GET_BITS(pBS, pData->ltp_long_used[i], 1)
      }

      for (i = pred_max_sfb; i < pData->max_sfb; i++) {
        pData->ltp_long_used[i] = 0;
      }
    }
  }
  return 0;
}

/********************************************************************/

int dec_section_data(s_SE_Individual_channel_stream *pData,
                     sBitsreamBuffer *pBS)
{
  int     sfb;
  int     k;
  int     g;
  int     sect_esc_val;
  int     sect_len_incr;
  int     esc_code_len;
  int     sect_cb;
  int     sect_len;

  if (pData->window_sequence == EIGHT_SHORT_SEQUENCE) {
    sect_esc_val = (1 << 3) - 1;
    esc_code_len = 3;
  } else {
    sect_esc_val = (1 << 5) - 1;
    esc_code_len = 5;
  }
  for (g = 0; g < pData->num_window_groups; g++) {
    k = 0;
    while (k < pData->max_sfb) {
      GET_BITS(pBS, sect_cb, 4)
      sect_len = 0;
      GET_BITS(pBS, sect_len_incr, esc_code_len)
      while (sect_len_incr == sect_esc_val) {
        sect_len += sect_esc_val;
        GET_BITS(pBS, sect_len_incr, esc_code_len)
      }
      sect_len += sect_len_incr;

      for (sfb = k; sfb < k + sect_len; sfb++) {
        pData->sfb_cb[g][sfb] = sect_cb;
      }
      k += sect_len;

      if (k > pData->max_sfb) {
        return -1;
      }
    }
    for (; k < 51; k++) {
      pData->sfb_cb[g][k] = 0;
    }
  }

  return 0;
}

/********************************************************************/

int dec_scale_factor_data(s_SE_Individual_channel_stream *pData,
                          short scalef[8][51],
                          int scale_factor,
                          int noise_nrg,
                          sBitsreamBuffer *pBS)
{
  int     g;
  int     sfb;
  short   t;
  int     is_pos;
  int     noise_pcm_flag;
  unsigned char *pSrc;
  int     bitoffset;

#ifndef KINOMA_FAST_HUFFMAN
  IppsVLCDecodeSpec_32s *pVLCDecSpec =
    (IppsVLCDecodeSpec_32s *) pData->p_huffman_tables[0];
#endif

  is_pos = 0;
  noise_pcm_flag = 1;

  pSrc = (Ipp8u *)pBS->pCurrent_dword + ((32 - pBS->nBit_offset) >> 3);
  bitoffset = (32 - pBS->nBit_offset) & 0x7;

  for (g = 0; g < pData->num_window_groups; g++) {
    for (sfb = 0; sfb < pData->max_sfb; sfb++) {
      switch (pData->sfb_cb[g][sfb]) {
      case ZERO_HCB:
        scalef[g][sfb] = 0;
        break;
      case INTENSITY_HCB:
      case INTENSITY_HCB2:
#ifndef KINOMA_FAST_HUFFMAN
        ippsVLCDecodeOne_1u16s_x(&pSrc, &bitoffset, &t, pVLCDecSpec);
#else
        kinoma_aac_dec_huff_one_sf(&pSrc, &bitoffset, &t);
#endif
        is_pos += t - SF_MID;
        scalef[g][sfb] = (short)is_pos;
        break;
      case NOISE_HCB:
        if (noise_pcm_flag) {
          pBS->pCurrent_dword = (Ipp32u *)(pSrc - ((Ipp32s)(pSrc) & 3));
          pBS->dword = BSWAP(pBS->pCurrent_dword[0]);
          pBS->nBit_offset =
            32 - ((pSrc - (Ipp8u *)pBS->pCurrent_dword) << 3) - bitoffset;
          noise_pcm_flag = 0;
          GET_BITS(pBS, t, 9)
          pSrc = (Ipp8u *)pBS->pCurrent_dword + ((32 - pBS->nBit_offset) >> 3);
          bitoffset = (32 - pBS->nBit_offset) & 0x7;
          t -= 256;
        } else {
#ifndef KINOMA_FAST_HUFFMAN
          ippsVLCDecodeOne_1u16s_x(&pSrc, &bitoffset, &t, pVLCDecSpec);
#else
          kinoma_aac_dec_huff_one_sf(&pSrc, &bitoffset, &t);
#endif
          t -= SF_MID;
        }
        noise_nrg += t;
        scalef[g][sfb] = (short)noise_nrg;
        break;
      default:
#ifndef KINOMA_FAST_HUFFMAN
        ippsVLCDecodeOne_1u16s_x(&pSrc, &bitoffset, &t, pVLCDecSpec);
#else
        kinoma_aac_dec_huff_one_sf(&pSrc, &bitoffset, &t);
#endif
        scale_factor += t - SF_MID;
        scalef[g][sfb] = (short)scale_factor;
        break;
      }
    }
  }
  pBS->pCurrent_dword = (Ipp32u *)(pSrc - ((Ipp32s)(pSrc) & 3));
  pBS->dword = BSWAP(pBS->pCurrent_dword[0]);
  pBS->nBit_offset =
    32 - ((pSrc - (Ipp8u *)pBS->pCurrent_dword) << 3) - bitoffset;
  return 0;
}

/********************************************************************/

int dec_spectral_data(s_SE_Individual_channel_stream *pData,
                      sBitsreamBuffer *pBS)
{
  int     g;
  int     sfb;
  int    *sfb_offset;
  short  *qp;
  Ipp8u  *pSrc;
#ifndef KINOMA_FAST_HUFFMAN
  Ipp16s  pDst[512];
#endif
  int     bitoffset, i, num;

  if (pData->window_sequence == EIGHT_SHORT_SEQUENCE) {
    sfb_offset = pData->sfb_offset_short_window;
  } else {
    sfb_offset = pData->sfb_offset_long_window;
  }

  pSrc = (Ipp8u *)pBS->pCurrent_dword + ((32 - pBS->nBit_offset) >> 3);
  bitoffset = (32 - pBS->nBit_offset) & 0x7;

  qp = pData->spectrum_data;

  for (g = 0; g < pData->num_window_groups; g++) {
    for (sfb = 0; sfb < pData->max_sfb; sfb++) {
      int     sfb_cb = pData->sfb_cb[g][sfb];
      int     sfb_begin = sfb_offset[sfb];
      int     sfb_end = sfb_offset[sfb + 1];

#ifndef KINOMA_FAST_HUFFMAN
	  int     shift = vlcShifts[sfb_cb];
      int     offset = vlcOffsets[sfb_cb];
      int     mask = (1 << (shift)) - 1;
#endif

#ifndef KINOMA_FAST_HUFFMAN
	  IppsVLCDecodeSpec_32s *pVLCDecSpec =
        (IppsVLCDecodeSpec_32s *) pData->p_huffman_tables[sfb_cb];
#endif

      switch (vlcTypes[sfb_cb]) {
      case 0:  /* 4 tuples */

        num = ((sfb_end - sfb_begin) >> 2) * pData->len_window_group[g];
#ifndef KINOMA_FAST_HUFFMAN
        ippsVLCDecodeBlock_1u16s_x(&pSrc, &bitoffset, pDst,
                                  num, pVLCDecSpec);

        for (i = 0; i < num; i++) {
          int tmp = pDst[i];

          qp[0] = (short)(tmp >> (3 * shift));
          qp[1] = (short)(((tmp >> (2 * shift)) & mask) - offset);
          qp[2] = (short)(((tmp >> (shift)) & mask) - offset);
          qp[3] = (short)((tmp & mask) - offset);

          qp += 4;
        }
#else
		kinoma_aac_dec_huff_quad_block(&pSrc, &bitoffset, &qp, num, sfb_cb);
#endif

        break;
      case 1:  /* 2 tuples */
        num = ((sfb_end - sfb_begin) >> 1) * pData->len_window_group[g];

#ifndef KINOMA_FAST_HUFFMAN
        ippsVLCDecodeBlock_1u16s_x(&pSrc, &bitoffset, pDst,
                                  num, pVLCDecSpec);

        for (i = 0; i < num; i++) {
          int tmp = pDst[i];

          qp[0] = (short)(tmp >> shift);
          qp[1] = (short)((tmp & mask) - offset);

          qp += 2;
        }
#else
        kinoma_aac_dec_huff_pair_block(&pSrc, &bitoffset, &qp, num, sfb_cb);
#endif
		break;
      case 2:  /* esc */
        num = ((sfb_end - sfb_begin)) * pData->len_window_group[g];

#ifndef KINOMA_FAST_HUFFMAN
        ippsVLCDecodeEscBlock_AAC_1u16s_x(&pSrc, &bitoffset, qp,
                                        num, pVLCDecSpec);
#else
        kinoma_aac_dec_huff_esc_block(&pSrc, &bitoffset, qp, num, sfb_cb);
#endif
        qp += num;
        break;
      default:
        num = ((sfb_end - sfb_begin)) * pData->len_window_group[g];
        for (i = 0; i < num; i++) {
          qp[0] = 0;
          qp++;
        }
        break;
      }
    }
  }

  pBS->pCurrent_dword = (Ipp32u *)(pSrc - ((Ipp32s)(pSrc) & 3));
  pBS->dword = BSWAP(pBS->pCurrent_dword[0]);
  pBS->nBit_offset =
    32 - ((pSrc - (Ipp8u *)pBS->pCurrent_dword) << 3) - bitoffset;
  return 0;
}

/********************************************************************/

int dec_pulse_data(s_SE_Individual_channel_stream *pData,
                   sBitsreamBuffer *pBS)
{
  int     i, k;

  GET_BITS(pBS, pData->number_pulse, 2)
  GET_BITS(pBS, pData->pulse_start_sfb, 6)
  if (pData->pulse_start_sfb > pData->num_swb_long)
    return -1;

  k = pData->sfb_offset_long_window[pData->pulse_start_sfb];
  for (i = 0; i < pData->number_pulse + 1; i++) {
    GET_BITS(pBS, pData->pulse_offset[i], 5)
    GET_BITS(pBS, pData->pulse_amp[i], 4)
    k += pData->pulse_offset[i];
  }
  if (k > 1024)
    return -1;

  return 0;
}

/********************************************************************/

int dec_tns_data(s_SE_Individual_channel_stream *pData,
                 sBitsreamBuffer *pBS)
{
  int     w;
  int     filt;
  int     i;
  int     coef_len;

  for (w = 0; w < pData->num_windows; w++) {
    int nbits;

    nbits = (pData->num_windows == 8) ? 1 : 2;
    GET_BITS(pBS, pData->n_filt[w], nbits)
    if (pData->n_filt[w] == 0)
      continue;
    GET_BITS(pBS, pData->coef_res[w], 1)

    for (filt = 0; filt < pData->n_filt[w]; filt++) {
      nbits = (pData->num_windows == 8) ? 4 : 6;
      GET_BITS(pBS, pData->length[w][filt], nbits)

      nbits = (pData->num_windows == 8) ? 3 : 5;
      GET_BITS(pBS, pData->order[w][filt], nbits)

      if (pData->order[w][filt] == 0)
        continue;

      GET_BITS(pBS, pData->direction[w][filt], 1)
      GET_BITS(pBS, pData->coef_compress[w][filt], 1)
      coef_len = 3 + pData->coef_res[w] - pData->coef_compress[w][filt];

      for (i = 0; i < pData->order[w][filt]; i++) {
        GET_BITS(pBS, pData->coef[w][filt][i], coef_len)
      }
    }
  }
  return 0;
}

/********************************************************************/

int dec_coupling_channel_element(sCoupling_channel_element *pElement,
                                 sCoupling_channel_data *pData,
                                 sBitsreamBuffer *pBS,
                                 enum AudioObjectType audioObjectType)
{
  int     c, cc_l, cc_r;
  short   t;
  unsigned char *pSrc;
  int     bitoffset;

#ifndef KINOMA_FAST_HUFFMAN
  IppsVLCDecodeSpec_32s *pVLCDecSpec =
    (IppsVLCDecodeSpec_32s *) pElement->stream.p_huffman_tables[0];
#endif

  GET_BITS(pBS, pElement->element_instance_tag, 4)

  GET_BITS(pBS, pData->ind_sw_cce_flag, 1)
  GET_BITS(pBS, pData->num_coupled_elements, 3)

  pData->num_gain_element_lists = 0;
  for (c = 0; c < pData->num_coupled_elements + 1; c++) {
    pData->num_gain_element_lists++;
    GET_BITS(pBS, pData->cc_target_id[c], 1)
    GET_BITS(pBS, pData->cc_target_tag[c], 4)
    if (pData->cc_target_id[c]) {
      GET_BITS(pBS, cc_l, 1)
      GET_BITS(pBS, cc_r, 1)
      pData->cc_lr[pData->num_gain_element_lists - 1] = (cc_l << 1) + cc_r;
      if (pData->cc_lr[pData->num_gain_element_lists - 1] == 3) {
        pData->num_gain_element_lists++;
        pData->cc_lr[pData->num_gain_element_lists - 1] = 3;
      }
    }
  }

  GET_BITS(pBS, pData->cc_domain, 1)
  GET_BITS(pBS, pData->gain_element_sign, 1)
  GET_BITS(pBS, pData->gain_element_scale, 2)

  if (dec_individual_channel_stream
      (&pElement->stream, pBS, 0, 0, audioObjectType) < 0)
    return -1;

  pData->max_sfb = pElement->stream.max_sfb;
  pData->num_window_groups = pElement->stream.num_window_groups;
  for (c = 0; c < pData->num_window_groups; c++) {
    pData->len_window_group[c] = pElement->stream.len_window_group[c];
  }

  if (pElement->stream.window_sequence != EIGHT_SHORT_SEQUENCE) {
    pData->sfb_offset = pElement->stream.sfb_offset_long_window;
  } else {
    pData->sfb_offset = pElement->stream.sfb_offset_short_window;
  }

  pData->cge[0] = 1;
  pElement->cc_fact[0][0][0] = 0;

  for (c = 1; c < pData->num_gain_element_lists; c++) {
    if (pData->ind_sw_cce_flag) {
      pData->cge[c] = 1;
    } else {
      GET_BITS(pBS, pData->cge[c], 1)
    }

    if (pData->cge[c]) {
      pSrc = (Ipp8u *)pBS->pCurrent_dword + ((32 - pBS->nBit_offset) >> 3);
      bitoffset = (32 - pBS->nBit_offset) & 0x7;
#ifndef KINOMA_FAST_HUFFMAN
      ippsVLCDecodeOne_1u16s_x(&pSrc, &bitoffset, &t, pVLCDecSpec);
#else
	  kinoma_aac_dec_huff_one_sf(&pSrc, &bitoffset, &t);
#endif
      pElement->cc_fact[c][0][0] = t - SF_MID;
      pBS->pCurrent_dword = (Ipp32u *)(pSrc - ((Ipp32s)(pSrc) & 3));
      pBS->dword = BSWAP(pBS->pCurrent_dword[0]);
      pBS->nBit_offset =
        32 - ((pSrc - (Ipp8u *)pBS->pCurrent_dword) << 3) - bitoffset;
    } else {
      dec_scale_factor_data(&pElement->stream, pElement->cc_fact[c], 0,
                            pElement->stream.global_gain - 90, pBS);
    }
  }

  return 0;
}

/********************************************************************/

int dec_data_stream_element(sData_stream_element *pData,
                            sBitsreamBuffer *pBS)
{
  int     data_byte_align_flag;
  int     count;
  int     cnt;
  int     i;

  GET_BITS(pBS, pData->element_instance_tag, 4)
  GET_BITS(pBS, data_byte_align_flag, 1)
  GET_BITS(pBS, count, 8)
  cnt = count;
  if (cnt == 255) {
    GET_BITS(pBS, count, 8)
    cnt += count;
  }
  if (data_byte_align_flag) {
    Byte_alignment(pBS);
  }
  for (i = 0; i < cnt; i++) {
    GET_BITS(pBS, pData->data_stream_byte[i], 8)
  }
  return 0;
}

/********************************************************************/

int dec_extension_payload(sSbrDecCommon * pSbr,
                          int *pNumFillSbr,
                          sDynamic_range_info *pInfo,
                          sBitsreamBuffer *pBS,
                          int cnt)
{
  int     extension_type;
  int     ret_value;
  int     fill_nibble;
  int     fill_byte;
  int     i;
  int     other_bits;

  ret_value = 0;
  GET_BITS(pBS, extension_type, 4)
  pSbr->cnt_bit -= 4;
  switch (extension_type) {
  case EXT_DYNAMIC_RANGE:
    ret_value = dec_dynamic_range_info(pInfo, pBS);
    break;

  case EXT_SBR_DATA:
    *pNumFillSbr += 1;
    ret_value = sbr_extension_data(pSbr->id_aac, 0, pSbr, pBS, cnt);
    break;

  case EXT_SBR_DATA_CRC:
    *pNumFillSbr += 1;
    ret_value = sbr_extension_data(pSbr->id_aac, 1, pSbr, pBS, cnt);
    break;

  case EXT_FILL_DATA:
    GET_BITS(pBS, fill_nibble, 4)
    for (i = 0; i < cnt - 1; i++) {
      GET_BITS(pBS, fill_byte, 8)
    }
    ret_value = cnt;
    break;
  default:
    for (i = 0; i < 8 * (cnt - 1) + 4; i++) {
      GET_BITS(pBS, other_bits, 1)
    }
    ret_value = cnt;
    break;
  }

  /******************************************************
   * ret_value - positive, number of bit has been read
   * but...
   * if there is error,
   * then ret_value = SBR_ERR_REQUIREMENTS
   ******************************************************/
  return ret_value;
}

/********************************************************************/

int dec_fill_element(sSbrDecCommon *pSbr,
                     int *cnt_fill_sbr_element,
                     sDynamic_range_info *pInfo,
                     sBitsreamBuffer * pBS)
{
  int     cnt;
  int     ret;

  GET_BITS(pBS, cnt, 4)

  if (cnt == 15) {
    int tmp;

    GET_BITS(pBS, tmp, 8)
    cnt += tmp - 1;
  }
  pSbr->cnt_bit = cnt * 8;

  /******************************************************
   * ret - positive value, number of bit has been read
   * but...
   * if there is error,
   * then ret = SBR_ERR_REQUIREMENTS
   ******************************************************/
  while (cnt > 0) {
    ret = dec_extension_payload(pSbr, cnt_fill_sbr_element, pInfo, pBS, cnt);

    if ( SBR_ERR_REQUIREMENTS == ret )
      return SBR_ERR_REQUIREMENTS;
    else
      cnt -= ret;
  }

  return 0;
}

/********************************************************************/

int dec_excluded_channels(sExcluded_channels *pData,
                          sBitsreamBuffer *pBS)
{
  int     n, i;

  n = 0;

  pData->num_excl_chan = 7;
  for (i = 0; i < 7; i++) {
    GET_BITS(pBS, pData->exclude_mask[i], 1)
    n++;
  }

  GET_BITS(pBS, pData->additional_excluded_chns[n - 1], 1)
  while (pData->additional_excluded_chns[n - 1] == 1) {
    for (i = pData->num_excl_chan; i < pData->num_excl_chan + 7; i++) {
      GET_BITS(pBS, pData->exclude_mask[i], 1)
    }
    n++;
    pData->num_excl_chan += 7;
    GET_BITS(pBS, pData->additional_excluded_chns[n - 1], 1)
  }
  return n;
}

/********************************************************************/

int dec_dynamic_range_info(sDynamic_range_info *pInfo,
                           sBitsreamBuffer *pBS)
{
  int     n, i;

  n = 1;

  pInfo->drc_num_bands = 1;
  GET_BITS(pBS, pInfo->pce_tag_present, 1)

  if (pInfo->pce_tag_present == 1) {
    GET_BITS(pBS, pInfo->pce_innstance_tag, 4)
    GET_BITS(pBS, pInfo->drc_tag_reserved_bits, 4)
    n++;
  }

  GET_BITS(pBS, pInfo->excluded_chns_present, 1)

  if (pInfo->excluded_chns_present == 1) {
    n += dec_excluded_channels(&pInfo->ec_data, pBS);
  }

  GET_BITS(pBS, pInfo->drc_bands_present, 1)
  if (pInfo->drc_bands_present == 1) {
    GET_BITS(pBS, pInfo->drc_band_incr, 4)
    GET_BITS(pBS, pInfo->drc_bands_reserved_bits, 4)

    n++;

    pInfo->drc_num_bands += pInfo->drc_band_incr;

    for (i = 0; i < pInfo->drc_num_bands; i++) {
      GET_BITS(pBS, pInfo->drc_band_top[i], 8)
      n++;
    }
  }

  GET_BITS(pBS, pInfo->prog_ref_level_present, 1)

  if (pInfo->prog_ref_level_present == 1) {
    GET_BITS(pBS, pInfo->prog_ref_level, 7)
    GET_BITS(pBS, pInfo->prog_ref_level_reserved_bits, 1)

    n++;
  }

  for (i = 0; i < pInfo->drc_num_bands; i++) {
    GET_BITS(pBS, pInfo->dyn_rng_sgn[i], 1)
    GET_BITS(pBS, pInfo->dyn_rng_ctl[i], 7)

    n++;
  }

  return n;
}

/********************************************************************/

int dec_gain_control_data(s_SE_Individual_channel_stream *pData,
                          sBitsreamBuffer *pBS)
{
  int     bd, ad, wd;

  GET_BITS(pBS, pData->max_band, 2)

  if (pData->window_sequence == ONLY_LONG_SEQUENCE) {
    for (bd = 1; bd <= pData->max_band; bd++) {
      for (wd = 0; wd < 1; wd++) {
        GET_BITS(pBS, pData->SSRInfo[bd][wd].adjust_num, 3)
        for (ad = 0; ad < pData->SSRInfo[bd][wd].adjust_num; ad++) {
          GET_BITS(pBS, pData->SSRInfo[bd][wd].alevcode[ad], 4)
          GET_BITS(pBS, pData->SSRInfo[bd][wd].aloccode[ad], 5)
        }
      }
    }
  } else if (pData->window_sequence == LONG_START_SEQUENCE) {
    for (bd = 1; bd <= pData->max_band; bd++) {
      for (wd = 0; wd < 2; wd++) {
        GET_BITS(pBS, pData->SSRInfo[bd][wd].adjust_num, 3)
        for (ad = 0; ad < pData->SSRInfo[bd][wd].adjust_num; ad++) {
          GET_BITS(pBS, pData->SSRInfo[bd][wd].alevcode[ad], 4)
          if (wd == 0) {
            GET_BITS(pBS, pData->SSRInfo[bd][wd].aloccode[ad], 4)
          } else {
            GET_BITS(pBS, pData->SSRInfo[bd][wd].aloccode[ad], 2)
          }
        }
      }
    }
  } else if (pData->window_sequence == EIGHT_SHORT_SEQUENCE) {
    for (bd = 1; bd <= pData->max_band; bd++) {
      for (wd = 0; wd < 8; wd++) {
        GET_BITS(pBS, pData->SSRInfo[bd][wd].adjust_num, 3)
        for (ad = 0; ad < pData->SSRInfo[bd][wd].adjust_num; ad++) {
          GET_BITS(pBS, pData->SSRInfo[bd][wd].alevcode[ad], 4)
          GET_BITS(pBS, pData->SSRInfo[bd][wd].aloccode[ad], 2)
        }
      }
    }
  } else if (pData->window_sequence == LONG_STOP_SEQUENCE) {
    for (bd = 1; bd <= pData->max_band; bd++) {
      for (wd = 0; wd < 2; wd++) {
        GET_BITS(pBS, pData->SSRInfo[bd][wd].adjust_num, 3)
        for (ad = 0; ad < pData->SSRInfo[bd][wd].adjust_num; ad++) {
          GET_BITS(pBS, pData->SSRInfo[bd][wd].alevcode[ad], 4)
          if (wd == 0) {
            GET_BITS(pBS, pData->SSRInfo[bd][wd].aloccode[ad], 4)
          } else {
            GET_BITS(pBS, pData->SSRInfo[bd][wd].aloccode[ad], 5)
          }
        }
      }
    }
  }

  return 0;
}

/********************************************************************/

void save_gain_control_data(int ch,
                            s_SE_Individual_channel_stream *pData,
                            AACDec_com *state_com)
{
  int bd, wd, k;

  for (bd = 1; bd < 4; bd++) {
    for (wd = 0; wd < 8; wd++) {
      state_com->SSRInfo[ch][bd][wd].adjust_num = pData->SSRInfo[bd][wd].adjust_num;
      for (k = 0; k < 8; k++) {
        state_com->SSRInfo[ch][bd][wd].alevcode[k] = pData->SSRInfo[bd][wd].alevcode[k];
        state_com->SSRInfo[ch][bd][wd].aloccode[k] = pData->SSRInfo[bd][wd].aloccode[k];
      }
    }
  }
}

/********************************************************************/

int ics_apply_pulse_I(s_SE_Individual_channel_stream *p_data)
{
  int     i, k;

  k = p_data->sfb_offset_long_window[p_data->pulse_start_sfb];

  for (i = 0; i <= p_data->number_pulse; i++) {
    k += p_data->pulse_offset[i];
    if (p_data->spectrum_data[k] > 0) {
      p_data->spectrum_data[k] = p_data->spectrum_data[k] + (short)p_data->pulse_amp[i];
    } else {
      p_data->spectrum_data[k] = p_data->spectrum_data[k] - (short)p_data->pulse_amp[i];
    }

  }
  return 0;
}

/********************************************************************/
