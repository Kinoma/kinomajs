/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __AACCMN_ICS_H
#define __AACCMN_ICS_H


enum eICS
{
  ICS_MAX_SFB           = 51,
  ICS_MAX_NUM_WINDOWS   = 8,
  ICS_MAX_GROUP_NUMBER  = 8,
  ICS_MAX_NUMBER_PULSE  = ((1<<2)+1),
  ICS_MAX_FILT          = 4,
  ICS_TNS_MAX_ORDER     = 32,
};

typedef struct
{
  int global_gain;

  /// ics_info
  int ics_reserved_bit;
  int window_sequence;
  int window_shape;

  int max_sfb;
  int num_swb_long;
  int num_swb_short;
  int scale_factor_grouping[7];

  int num_window_groups;
  int len_window_group[8];
  int num_windows;

  int predictor_data_present;  ///
  int predictor_reset;
  int predictor_reset_group_number;
  int pred_max_sfb;
  int prediction_used[41];

  int pulse_data_present;
  int tns_data_present;
  int gain_control_data_present;

  int    sfb_cb[ICS_MAX_GROUP_NUMBER][ICS_MAX_SFB];
  short  sf[ICS_MAX_GROUP_NUMBER][ICS_MAX_SFB];

  short  spectrum_data[1024];
  short* p_spectrum[8];

  int*   sfb_offset_long_window;
  int*   sfb_offset_short_window;

  void** p_huffman_tables;

  int    number_pulse;
  int    pulse_start_sfb;
  int    pulse_offset[ICS_MAX_NUMBER_PULSE];
  int    pulse_amp[ICS_MAX_NUMBER_PULSE];


    ///    Tns data
  int    n_filt[ICS_MAX_NUM_WINDOWS];
  int    coef_res[ICS_MAX_NUM_WINDOWS];
  int    length[ICS_MAX_NUM_WINDOWS][ICS_MAX_FILT];
  int    order[ICS_MAX_NUM_WINDOWS][ICS_MAX_FILT];
  int    direction[ICS_MAX_NUM_WINDOWS][ICS_MAX_FILT];
  int    coef_compress[ICS_MAX_NUM_WINDOWS][ICS_MAX_FILT];
  int    coef[ICS_MAX_NUM_WINDOWS][ICS_MAX_FILT][ICS_TNS_MAX_ORDER];
  int    tns_max_bands_short;
  int    tns_max_bands_long;


  /// Gain control data
  int    max_band;
  int    adjust_num[4][8];
  int    alevcode[4][8][8];
  int    aloccode[4][8][8];

  /// LTP data
  int    ltp_data_present; ///1 bit
  int    ltp_lag_update; /// 1 bit
  int    ltp_lag;  /// 10-11 bits
  int    ltp_coef; /// 3 bits
  int    ltp_long_used[ICS_MAX_SFB];
  int    ltp_short_used[ICS_MAX_NUM_WINDOWS];
  int    ltp_short_lag_present[ICS_MAX_NUM_WINDOWS];
  int    ltp_short_lag[ICS_MAX_NUM_WINDOWS];

}sIndividual_channel_stream;



#endif//__AACCMN_ICS_H
