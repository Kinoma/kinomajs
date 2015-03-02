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

#include "aac_dec_own.h"
#include "aac_sfb_tables.h"

/********************************************************************/

AACStatus aacdecSetSamplingFrequencyCom(int sampling_frequency_index,
                                        AACDec_com *state_com)
{
  static int pred_max_sfb[] = {
    33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34, 0, 0, 0, 0
  };
  static int tns_max_bands[][4] = {
    {31,  9, 28,  7}, /* 96000 */ {31,  9, 28,  7}, /* 88200 */
    {34, 10, 27,  7}, /* 64000 */ {40, 14, 26,  6}, /* 48000 */
    {42, 14, 26,  6}, /* 44100 */ {51, 14, 26,  6}, /* 32000 */
    {46, 14, 29,  7}, /* 24000 */ {46, 14, 29,  7}, /* 22050 */
    {42, 14, 23,  8}, /* 16000 */ {42, 14, 23,  8}, /* 12000 */
    {42, 14, 23,  8}, /* 11025 */ {39, 14, 19,  7}, /* 8000  */
    {39, 14, 19,  7}, /* 7350  */ {0,0,0,0}, {0,0,0,0}, {0,0,0,0}
  };
  static int sampling_frequency_table[] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000,
    22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0
  };
  int     max_order_long, ssr_index;

  if (!state_com)
    return AAC_NULL_PTR;

  if ((sampling_frequency_index < 0) || (sampling_frequency_index > 12))
    return AAC_BAD_PARAMETER;

  state_com->m_sampling_frequency_index = sampling_frequency_index;
  state_com->m_sampling_frequency = sampling_frequency_table[sampling_frequency_index];
  state_com->m_is_chmap_valid = 0;

  state_com->m_cpe.streams[0].num_swb_long =
    sfb_tables[sampling_frequency_index].num_sfb_long_window;
  state_com->m_cpe.streams[0].num_swb_short =
    sfb_tables[sampling_frequency_index].num_sfb_short_window;

  state_com->m_cpe.streams[1].num_swb_long =
    sfb_tables[sampling_frequency_index].num_sfb_long_window;
  state_com->m_cpe.streams[1].num_swb_short =
    sfb_tables[sampling_frequency_index].num_sfb_short_window;

  state_com->m_sce.stream.num_swb_short =
    sfb_tables[sampling_frequency_index].num_sfb_short_window;
  state_com->m_sce.stream.num_swb_long =
    sfb_tables[sampling_frequency_index].num_sfb_long_window;

  state_com->m_lfe.stream.num_swb_short =
    sfb_tables[sampling_frequency_index].num_sfb_short_window;
  state_com->m_lfe.stream.num_swb_long =
    sfb_tables[sampling_frequency_index].num_sfb_long_window;

  state_com->m_cce.stream.num_swb_short =
    sfb_tables[sampling_frequency_index].num_sfb_short_window;
  state_com->m_cce.stream.num_swb_long =
    sfb_tables[sampling_frequency_index].num_sfb_long_window;

  state_com->m_cpe.streams[0].sfb_offset_long_window =
    sfb_tables[sampling_frequency_index].sfb_offset_long_window;
  state_com->m_cpe.streams[1].sfb_offset_long_window =
    sfb_tables[sampling_frequency_index].sfb_offset_long_window;

  state_com->m_sce.stream.sfb_offset_long_window =
    sfb_tables[sampling_frequency_index].sfb_offset_long_window;

  state_com->m_lfe.stream.sfb_offset_long_window =
    sfb_tables[sampling_frequency_index].sfb_offset_long_window;
  state_com->m_cce.stream.sfb_offset_long_window =
    sfb_tables[sampling_frequency_index].sfb_offset_long_window;

  state_com->m_cpe.streams[0].sfb_offset_short_window =
    sfb_tables[sampling_frequency_index].sfb_offset_short_window;
  state_com->m_cpe.streams[1].sfb_offset_short_window =
    sfb_tables[sampling_frequency_index].sfb_offset_short_window;

  state_com->m_sce.stream.sfb_offset_short_window =
    sfb_tables[sampling_frequency_index].sfb_offset_short_window;

  state_com->m_lfe.stream.sfb_offset_short_window =
    sfb_tables[sampling_frequency_index].sfb_offset_short_window;
  state_com->m_cce.stream.sfb_offset_short_window =
    sfb_tables[sampling_frequency_index].sfb_offset_short_window;

  state_com->m_cpe.streams[0].pred_max_sfb = pred_max_sfb[sampling_frequency_index];
  state_com->m_cpe.streams[1].pred_max_sfb = pred_max_sfb[sampling_frequency_index];

  ssr_index = 0;
  if (AOT_AAC_SSR == state_com->m_audio_object_type) {
    ssr_index = 2;
  }

  state_com->m_cpe.streams[0].tns_max_bands_long =
    tns_max_bands[sampling_frequency_index][0 + ssr_index];
  state_com->m_cpe.streams[0].tns_max_bands_short =
    tns_max_bands[sampling_frequency_index][1 + ssr_index];

  state_com->m_cpe.streams[1].tns_max_bands_long =
    tns_max_bands[sampling_frequency_index][0 + ssr_index];
  state_com->m_cpe.streams[1].tns_max_bands_short =
    tns_max_bands[sampling_frequency_index][1 + ssr_index];

  state_com->m_lfe.stream.pred_max_sfb = pred_max_sfb[sampling_frequency_index];
  state_com->m_sce.stream.pred_max_sfb = pred_max_sfb[sampling_frequency_index];
  state_com->m_cce.stream.pred_max_sfb = pred_max_sfb[sampling_frequency_index];

  state_com->m_lfe.stream.tns_max_bands_long =
    tns_max_bands[sampling_frequency_index][0 + ssr_index];
  state_com->m_lfe.stream.tns_max_bands_short =
    tns_max_bands[sampling_frequency_index][1 + ssr_index];

  state_com->m_sce.stream.tns_max_bands_long =
    tns_max_bands[sampling_frequency_index][0 + ssr_index];
  state_com->m_sce.stream.tns_max_bands_short =
    tns_max_bands[sampling_frequency_index][1 + ssr_index];

  state_com->m_cce.stream.tns_max_bands_long =
    tns_max_bands[sampling_frequency_index][0 + ssr_index];
  state_com->m_cce.stream.tns_max_bands_short =
    tns_max_bands[sampling_frequency_index][1 + ssr_index];

  max_order_long = 12;
  if ((AOT_AAC_MAIN == state_com->m_audio_object_type) ||
      (AOT_AAC_LTP == state_com->m_audio_object_type)) {
    max_order_long = 20;
  }

  state_com->m_lfe.stream.tns_max_order_long = max_order_long;
  state_com->m_lfe.stream.tns_max_order_short = TNS_MAX_ORDER_SHORT;

  state_com->m_sce.stream.tns_max_order_long = max_order_long;
  state_com->m_sce.stream.tns_max_order_short = TNS_MAX_ORDER_SHORT;

  state_com->m_cce.stream.tns_max_order_long = max_order_long;
  state_com->m_cce.stream.tns_max_order_short = TNS_MAX_ORDER_SHORT;

  state_com->m_cpe.streams[0].tns_max_order_long = max_order_long;
  state_com->m_cpe.streams[0].tns_max_order_short = TNS_MAX_ORDER_SHORT;

  state_com->m_cpe.streams[1].tns_max_order_long = max_order_long;
  state_com->m_cpe.streams[1].tns_max_order_short = TNS_MAX_ORDER_SHORT;

  return AAC_OK;
}

/********************************************************************/
