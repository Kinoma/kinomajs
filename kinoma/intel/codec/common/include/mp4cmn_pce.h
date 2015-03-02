/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MP4CMN_PCE_H__
#define __MP4CMN_PCE_H__

#include "bstream.h"

enum eMP4CMN_PCE_H
{
    LEN_COMMENT = (1<<8),
    MAX_CHANNELS_ELEMENTS   = 16,
    MAX_ASSOC_DATA_ELEMENTS =  8,
    MAX_VALID_CC_ELEMENTS = 16

};

typedef struct
{
  int element_instance_tag;
  int object_type;
  int sampling_frequency_index;

  int num_front_channel_elements;
  int num_side_channel_elements;
  int num_back_channel_elements;
  int num_lfe_channel_elements;

  int num_assoc_data_elements;
  int num_valid_cc_elements;

  int mono_mixdown_present;
  int mono_miwdown_element_number;

  int stereo_mixdown_present;
  int stereo_miwdown_element_number;

  int matrix_mixdown_idx_present;
  int matrix_mixdown_idx;
  int pseudo_surround_enable;

  int front_element_is_cpe[MAX_CHANNELS_ELEMENTS];
  int front_element_tag_select[MAX_CHANNELS_ELEMENTS];
  int side_element_is_cpe[MAX_CHANNELS_ELEMENTS];
  int side_element_tag_select[MAX_CHANNELS_ELEMENTS];
  int back_element_is_cpe[MAX_CHANNELS_ELEMENTS];
  int back_element_tag_select[MAX_CHANNELS_ELEMENTS];
  int lfe_element_tag_select[MAX_CHANNELS_ELEMENTS];

  int assoc_data_element_tag_select[MAX_ASSOC_DATA_ELEMENTS];
  int cc_element_is_ind_sw[MAX_VALID_CC_ELEMENTS];
  int valid_cc_element_tag_select[MAX_VALID_CC_ELEMENTS];

  int comment_field_bytes;
  char comment_field_data[LEN_COMMENT];

  int num_front_channels;
  int num_side_channels;
  int num_back_channels;
  int num_lfe_channels;

} sProgram_config_element;

#ifdef  __cplusplus
extern "C" {
#endif

int dec_program_config_element(sProgram_config_element * p_data,sBitsreamBuffer * p_bs);

/***********************************************************************

    Unpack function(s) (support(s) alternative bitstream representation)

***********************************************************************/


int unpack_program_config_element(sProgram_config_element * p_data,unsigned char **pp_bitstream, int *p_offset);

#ifdef  __cplusplus
}
#endif

#endif//__MP4CMN_PCE_H__
