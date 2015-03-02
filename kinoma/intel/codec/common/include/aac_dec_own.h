/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __AAC_DEC_OWN_H
#define __AAC_DEC_OWN_H

#include "bstream.h"
#include "aaccmn_const.h"
#include "sbrdec.h"
#include "aaccmn_chmap.h"
#include "aac_dec.h"

typedef struct {
  int     adjust_num;
  int     alevcode[8];
  int     aloccode[8];
} SSR_GAIN;

typedef struct {

  int     global_gain;

  /* ics_info */
  int     ics_reserved_bit;
  int     window_sequence;
  int     window_shape;

  int     max_sfb;
  int     num_swb_long;
  int     num_swb_short;
  int     scale_factor_grouping[7];

  int     num_window_groups;
  int     len_window_group[8];
  int     num_windows;

  int     predictor_data_present;
  int     predictor_reset;
  int     predictor_reset_group_number;
  int     pred_max_sfb;
  unsigned char prediction_used[41];

  int     pulse_data_present;
  int     tns_data_present;
  int     gain_control_data_present;

  int     sfb_cb[MAX_GROUP_NUMBER][MAX_SFB];
  short   sf[MAX_GROUP_NUMBER][MAX_SFB];

  short   spectrum_data[1024];
  short  *p_spectrum[8];

  int    *sfb_offset_long_window;
  int    *sfb_offset_short_window;

#ifndef KINOMA_FAST_HUFFMAN
  void  **p_huffman_tables;
#endif

  int     number_pulse;
  int     pulse_start_sfb;
  int     pulse_offset[MAX_NUMBER_PULSE];
  int     pulse_amp[MAX_NUMBER_PULSE];

  /* Tns data */
  int     n_filt[MAX_NUM_WINDOWS];
  int     coef_res[MAX_NUM_WINDOWS];
  int     length[MAX_NUM_WINDOWS][MAX_FILT];
  int     order[MAX_NUM_WINDOWS][MAX_FILT];
  int     direction[MAX_NUM_WINDOWS][MAX_FILT];
  int     coef_compress[MAX_NUM_WINDOWS][MAX_FILT];
  int     coef[MAX_NUM_WINDOWS][MAX_FILT][MAX_ORDER];
  int     tns_max_bands_short;
  int     tns_max_bands_long;
  int     tns_max_order_short;
  int     tns_max_order_long;

  /* Gain control data */
  int      max_band;
  SSR_GAIN SSRInfo[4][8];

  /* LTP data */
  int     ltp_data_present;
  int     ltp_lag_update;
  int     ltp_lag;
  int     ltp_coef;
  int     ltp_long_used[MAX_SFB];
  int     ltp_short_used[MAX_NUM_WINDOWS];
  int     ltp_short_lag_present[MAX_NUM_WINDOWS];
  int     ltp_short_lag[MAX_NUM_WINDOWS];

} s_SE_Individual_channel_stream;

typedef struct {
  int     element_instance_tag;

  s_SE_Individual_channel_stream stream;
} sLfe_channel_element;

typedef struct {
  int     element_instance_tag;

  s_SE_Individual_channel_stream stream;
} sSce_channel_element;

typedef struct {
  int     element_instance_tag;
  int     common_window;

  int     ms_mask_present;
  int     ms_used[8][49];

  s_SE_Individual_channel_stream streams[2];
} sCpe_channel_element;

typedef struct {
  int     element_instance_tag;
  char    data_stream_byte[512];

} sData_stream_element;

typedef struct {
  int     element_instance_tag;
  short   cc_fact[18][MAX_GROUP_NUMBER][MAX_SFB];

  s_SE_Individual_channel_stream stream;
} sCoupling_channel_element;

typedef struct {
  int     ind_sw_cce_flag;
  int     num_coupled_elements;
  int     cc_target_id[9];
  int     cc_target_tag[9];
  int     cc_target_ch[9];
  int     cc_lr[18];
  int     cge[18];
  int     max_sfb;
  int     num_window_groups;
  int     len_window_group[8];
  int     cc_domain;
  int     gain_element_sign;
  int     gain_element_scale;
  int     num_gain_element_lists;
  int     *sfb_offset;
} sCoupling_channel_data;

typedef struct {
  unsigned char exclude_mask[128];
  unsigned char additional_excluded_chns[10];
  int     num_excl_chan;

} sExcluded_channels;

typedef struct {
  int     drc_num_bands;

  int     pce_tag_present;
  int     pce_innstance_tag;
  int     drc_tag_reserved_bits;

  int     excluded_chns_present;

  sExcluded_channels ec_data;
  int     drc_bands_present;
  int     drc_band_incr;
  int     drc_bands_reserved_bits;
  int     drc_band_top[17];
  int     prog_ref_level_present;
  int     prog_ref_level;
  int     prog_ref_level_reserved_bits;

  int     dyn_rng_sgn[17];
  int     dyn_rng_ctl[17];

} sDynamic_range_info;

typedef struct {
  int                       m_is_chmap_valid;
  int                       m_is_pce_valid;
  int                       m_sampling_frequency;
  int                       m_up_sample;
  int                       m_sampling_frequency_index;
  int                       m_element_number;
  int                       m_channel_number;
  int                       m_channel_number_save;
  int                       m_channel_config;
  int                       m_frame_number;
  int                       m_frame_size;
  enum AudioObjectType      m_audio_object_type;
  sCh_map_item              m_chmap[EL_ID_MAX][EL_TAG_MAX];
  sEl_map_item              m_elmap[EL_TAG_MAX];

  int                       m_index_1st;
  int                       m_index_2nd;
  int                       m_index_3rd;

  int                       m_curr_win_shape[CH_MAX + COUPL_CH_MAX];
  int                       m_prev_win_shape[CH_MAX + COUPL_CH_MAX];
  int                       m_curr_win_sequence[CH_MAX + COUPL_CH_MAX];

  int                       m_order[CH_MAX + COUPL_CH_MAX];
  int                       noiseState;
  int                       adts_channel_configuration;

  SSR_GAIN                  prevSSRInfo[CH_MAX + COUPL_CH_MAX][4];
  SSR_GAIN                  SSRInfo[CH_MAX + COUPL_CH_MAX][4][8];

  sProgram_config_element   m_pce;
  sCpe_channel_element      m_cpe;
  sSce_channel_element      m_sce;
  sLfe_channel_element      m_lfe;
  sCoupling_channel_element m_cce;
  sCoupling_channel_data    m_cdata[COUPL_CH_MAX];

  int                       SbrFlagPresent;
  int                       ModeDecodeHEAACprofile;
  int                       ModeDwnmxHEAACprofile;

  IppsVLCDecodeSpec_32s     *sbrHuffTables[10];

#ifndef KINOMA_FAST_HUFFMAN
  void                      *huffman_tables[16];
#endif

  int                       FirstID3Search;
  int                       id3_size;
} AACDec_com;

#ifdef  __cplusplus
extern  "C" {
#endif

  int     ics_apply_pulse_I(s_SE_Individual_channel_stream *p_data);
  int     dec_individual_channel_stream(s_SE_Individual_channel_stream *pData,
                                        sBitsreamBuffer *pBS,
                                        int common_window,
                                        int scal_flag,
                                        enum AudioObjectType audioObjectType);
  int     dec_cpe_channel_element(sCpe_channel_element *pElement,
                                  sBitsreamBuffer *pBS,
                                  enum AudioObjectType audioObjectType);
  int     dec_sce_channel_element(sSce_channel_element *pElement,
                                  sBitsreamBuffer *pBS,
                                  enum AudioObjectType audioObjectType);
  int     dec_lfe_channel_element(sLfe_channel_element *pElement,
                                  sBitsreamBuffer *pBS,
                                  enum AudioObjectType audioObjectType);
  int     dec_coupling_channel_element(sCoupling_channel_element *pElement,
                                       sCoupling_channel_data *pData,
                                       sBitsreamBuffer *pBS,
                                       enum AudioObjectType audioObjectType);
  int     dec_data_stream_element(sData_stream_element *pData,
                                  sBitsreamBuffer *pBS);

  int     dec_fill_element(sSbrDecCommon * pSbr,
                           int *cnt_fill_sbr_element,
                           sDynamic_range_info *pInfo,
                           sBitsreamBuffer *pBS);
  int     sbr_extension_data(int id_aac,
                             int crc_flag,
                             sSbrDecCommon * pSbr,
                             sBitsreamBuffer *BS,
                             int cnt);

  int     dec_dynamic_range_info(sDynamic_range_info *pInfo,
                                 sBitsreamBuffer *pBS);

  AACStatus aacdecSetSamplingFrequencyCom(int sampling_frequency_index,
                                          AACDec_com *state_com);

  void save_gain_control_data(int ch,
                            s_SE_Individual_channel_stream *pData,
                            AACDec_com *state_com);
#ifdef  __cplusplus
}
#endif

#define EXT_FILL          0x00
#define EXT_FILL_DATA     0x01
#define EXT_DYNAMIC_RANGE 0x0B
#define EXT_SBR_DATA      0x0D
#define EXT_SBR_DATA_CRC  0x0E
#endif
