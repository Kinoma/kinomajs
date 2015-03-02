/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __AACCMN_ADIF_H
#define __AACCMN_ADIF_H

#include "mp4cmn_pce.h"

enum eAACCMN_ADIF_H
{
    ADIF_SIGNATURE   = 0x41444946,
    LEN_COPYRIGHT_ID = ((72/8)),
    MAX_PCE_NUM      = 16
};

typedef struct
{
  unsigned long adif_id;
  int           copyright_id_present;
  char          copyright_id[LEN_COPYRIGHT_ID+1];
  int           original_copy;
  int           home;
  int           bitstream_type;
  unsigned long bitrate;
  int           num_program_config_elements;
  unsigned long adif_buffer_fullness;

  sProgram_config_element pce[MAX_PCE_NUM];
} sAdif_header;


#ifdef  __cplusplus
extern "C" {
#endif

int dec_adif_header(sAdif_header* p_adif_header,sBitsreamBuffer* p_bs);


int unpack_adif_header(sAdif_header * p_data,unsigned char **pp_bitstream, int *p_offset);

#ifdef  __cplusplus
}
#endif



#endif//__AACCMN_ADIF_H
