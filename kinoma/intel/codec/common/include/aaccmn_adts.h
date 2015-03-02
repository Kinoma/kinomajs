/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __AACCMN_ADTS_H
#define __AACCMN_ADTS_H

#include "bstream.h"

typedef struct
{
  int   ID;
  int   Layer;
  int   protection_absent;
  int   Profile;
  int   sampling_frequency_index;
  int   private_bit;
  int   channel_configuration;
  int   original_copy;
  int   Home;
  int   Emphasis;

} sAdts_fixed_header;

typedef struct
{
  int copyright_identification_bit;
  int copyright_identification_start;
  int aac_frame_length;
  int adts_buffer_fullness;
  int no_raw_data_blocks_in_frame;

} sAdts_variable_header;

#ifdef  __cplusplus
extern "C" {
#endif
/*
int dec_adts_fixed_header(s_adts_fixed_header* pHeader,sBitsreamBuffer* pBS);
int dec_adts_variable_header(s_adts_variable_header* pHeader,sBitsreamBuffer* pBS);
*/
int  dec_adts_fixed_header(sAdts_fixed_header* pHeader,sBitsreamBuffer* BS);
int  dec_adts_variable_header(sAdts_variable_header* pHeader,sBitsreamBuffer* BS);

int unpack_adts_fixed_header(sAdts_fixed_header* p_header,unsigned char **pp_bitstream, int *p_offset);
int unpack_adts_variable_header(sAdts_variable_header* p_header,unsigned char **pp_bitstream, int *p_offset);


int get_audio_object_type_by_adts_header(sAdts_fixed_header* p_header);

#ifdef  __cplusplus
}
#endif

#endif//__AACCMN_ADTS_H
