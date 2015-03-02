/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "aaccmn_adts.h"
#include "bstream.h"

#include "aaccmn_const.h"

int
dec_adts_fixed_header(sAdts_fixed_header* pHeader,sBitsreamBuffer* BS)
{
  int syncword;

  syncword = Getbits(BS,12);
  if (syncword != 0x0FFF)
  {
    return 1;
  }

  pHeader->ID =  Getbits(BS,1);
  pHeader->Layer = Getbits(BS,2);
  pHeader->protection_absent = Getbits(BS,1);
  pHeader->Profile = Getbits(BS,2);
  pHeader->sampling_frequency_index = Getbits(BS,4);
  pHeader->private_bit = Getbits(BS,1);
  pHeader->channel_configuration = Getbits(BS,3);
  pHeader->original_copy = Getbits(BS,1);
  pHeader->Home = Getbits(BS,1);
//  pHeader->Emphasis = Getbits(BS,2);
  return 0;
}

int
dec_adts_variable_header(sAdts_variable_header* pHeader,sBitsreamBuffer* BS)
{
  pHeader->copyright_identification_bit = Getbits(BS,1);
  pHeader->copyright_identification_start =  Getbits(BS,1);
  pHeader->aac_frame_length = Getbits(BS,13);
  pHeader->adts_buffer_fullness = Getbits(BS,11);
  pHeader->no_raw_data_blocks_in_frame = Getbits(BS,2);

  return 0;
}

/***********************************************************************

    Unpack function(s) (support(s) alternative bitstream representation)

***********************************************************************/


int
unpack_adts_fixed_header(sAdts_fixed_header* p_header,unsigned char **pp_bitstream, int *p_offset)
{
  int syncword;

  syncword = get_bits(pp_bitstream,p_offset,12);
  if (syncword != 0x0FFF)
  {
    return 1;
  }

  p_header->ID =  get_bits(pp_bitstream,p_offset,1);
  p_header->Layer = get_bits(pp_bitstream,p_offset,2);
  p_header->protection_absent = get_bits(pp_bitstream,p_offset,1);
  p_header->Profile = get_bits(pp_bitstream,p_offset,2);
  p_header->sampling_frequency_index = get_bits(pp_bitstream,p_offset,4);
  p_header->private_bit = get_bits(pp_bitstream,p_offset,1);
  p_header->channel_configuration = get_bits(pp_bitstream,p_offset,3);
  p_header->original_copy = get_bits(pp_bitstream,p_offset,1);
  p_header->Home = get_bits(pp_bitstream,p_offset,1);
//  p_header->Emphasis = get_bits(pp_bitstream,p_offset,2);
  return 0;
}

int
unpack_adts_variable_header(sAdts_variable_header* p_header,unsigned char **pp_bitstream, int *p_offset)
{
  p_header->copyright_identification_bit = get_bits(pp_bitstream,p_offset,1);
  p_header->copyright_identification_start =  get_bits(pp_bitstream,p_offset,1);
  p_header->aac_frame_length = get_bits(pp_bitstream,p_offset,13);
  p_header->adts_buffer_fullness = get_bits(pp_bitstream,p_offset,11);
  p_header->no_raw_data_blocks_in_frame = get_bits(pp_bitstream,p_offset,2);

  return 0;
}


static int g_adts_profile_table[4][2] =
{
    /*MPEG-2  ID == 1*/ /*MPEG-4  ID == 0*/
    {AOT_AAC_MAIN,      AOT_AAC_MAIN},
    {AOT_AAC_LC,        AOT_AAC_LC },
    {AOT_AAC_SSR,       AOT_AAC_SSR },
    {AOT_UNDEF,         AOT_AAC_LTP }
};

int
get_audio_object_type_by_adts_header(sAdts_fixed_header* p_header)
{
    int res;

    if (p_header->ID == 1)
    {
        res = g_adts_profile_table[p_header->Profile][0];
    }
    else
    {
        res = g_adts_profile_table[p_header->Profile][1];
    }

    return res;
}
