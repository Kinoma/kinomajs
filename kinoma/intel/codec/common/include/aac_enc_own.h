/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __AAC_ENC_OWN_H
#define __AAC_ENC_OWN_H

#include "bstream.h"
#include "aaccmn_const.h"

typedef struct {
  enum AudioObjectType audioObjectType;
  int global_gain;
  int pulse_data_present;
  int tns_data_present;
  int gain_control_data_present;

  /* ics_info */
  int ics_reserved_bit;
  int windows_sequence;
  int window_shape;
  int max_sfb;
  int num_sfb;
  int scale_factor_grouping[7];
  int num_window_groups;
  int len_window_group[8];
  int predictor_data_present;

  /* section_data */
  int sect_cb[MAX_SECTION_NUMBER];
  int sect_len[MAX_SECTION_NUMBER];
  int sect_num[8];
  short sfb_cb[MAX_SECTION_NUMBER];

  /* scale_factor_data */
  short scale_factors[MAX_SECTION_NUMBER];

  /* spectral_data */
  signed short x_quant[1024];
  int*         sfb_offset;
  void**       pHuffTables;

  /* LTP data */
  int     ltp_data_present;
  int     ltp_lag_update;
  int     ltp_lag;
  int     ltp_coef;
  int     ltp_long_used[MAX_SFB];
  int     ltp_short_used[MAX_NUM_WINDOWS];
  int     ltp_short_lag_present[MAX_NUM_WINDOWS];
  int     ltp_short_lag[MAX_NUM_WINDOWS];
} sEnc_individual_channel_stream;

typedef struct
{
  int*    common_scalefactor_update;
  int*    last_frame_common_scalefactor;

    /// Special variables...
  int     start_common_scalefac;
  int     finish_common_scalefac;
  int     available_bits;
  int     used_bits;
} sQuantizationBlock;


typedef struct {
  sEnc_individual_channel_stream*  p_individual_channel_stream;
} sEnc_single_channel_element;

typedef struct {
  int common_window;
  int ms_mask_present;

  sEnc_individual_channel_stream*  p_individual_channel_stream_0;
  sEnc_individual_channel_stream*  p_individual_channel_stream_1;
} sEnc_channel_pair_element;

typedef struct {
  int   element_id;
  int   element_instance_tag;
  int   prev_window_shape;
  int   bits_in_buf;
  int   max_bits_in_buf;
  int   mean_bits;
  int   common_scalefactor_update;
  int   last_frame_common_scalefactor;
} sOneChannelInfo;

typedef struct {
  int   m_channel_number;
  int   m_sampling_frequency;
  int   m_bitrate;
  int   m_frame_number;

  int   m_buff_prev_index;
  int   m_buff_curr_index;
  int   m_buff_next_index;

  int   sampling_frequency_index;
  enum  AudioObjectType audioObjectType;
  int*  sfb_offset[4];
  int   real_num_sfb[4];
  int   real_max_sfb[4];
  int   real_max_sfb_lfe[4];
  int   sfb_offset_for_short_window[MAX_SECTION_NUMBER + 1];

  IppsVLCEncodeSpec_32s*  huffman_tables[12];
  Ipp16s** buff;
  Ipp8u*   real_state;

  sOneChannelInfo* chInfo;
} AACEnc_com;

#ifdef  __cplusplus
extern "C" {
#endif

int enc_single_channel_element(sEnc_single_channel_element* pElement,
                               int element_instance_tag,
                               sBitsreamBuffer* pBS,
                               int writing);

int enc_channel_pair_element(sEnc_channel_pair_element* pElement,
                             int element_instance_tag,
                             sBitsreamBuffer* pBS,
                             int writing);

#ifdef  __cplusplus
}
#endif
#endif
