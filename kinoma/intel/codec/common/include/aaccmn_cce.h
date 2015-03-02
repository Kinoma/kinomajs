/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __AACCMN_CCE_H
#define __AACCMN_CCE_H

typedef struct
{
  int  element_instance_tag;

  int ind_sw_cce_flag;         /// 1 bit
  int num_coupled_elements;    /// 3 bits
  int cc_target_is_cpe[9];     /// 1 bit
  int cc_target_tag_select[9]; /// 4 bits
  int cc_l[9];                 /// 1 bit   if (cc_target_is_cpe[9];)
  int cc_r[9];                 /// 1 bit   if (cc_target_is_cpe[9];)
  int cc_domain;               /// 1 bit
  int gain_element_sign;       /// 1 bit
  int gain_element_scale;      /// 2 bit

  int common_gain_element_present[10];///
  int common_gain_element[10];


//  s_SE_Individual_channel_stream  stream;

  int num_gain_element_lists; ///


} sCoupling_channel_element;

#ifdef  __cplusplus
extern "C" {
#endif

int unpack_coupling_channel_element(sCoupling_channel_element * p_data,unsigned char** pp_bs,int* p_offset,int audio_object_type, int sampling_frequency_index);

#ifdef  __cplusplus
}
#endif


#endif//__AACCMN_CCE_H
