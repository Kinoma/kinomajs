/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//     Intel Integrated Performance Primitives AAC Encode Sample for Windows*
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel IPP
//  product installation for more information.
//
//  MPEG-4 and AAC are international standards promoted by ISO, IEC, ITU, ETSI
//  and other organizations. Implementations of these standards, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
*/

#ifndef __SEARCH_H
#define __SEARCH_H

#include "ipps.h"
#include "aac_enc_own.h"

#define  MAX_NON_ESC_VALUE  12

#ifdef  __cplusplus
extern "C" {
#endif

int best_codebooks_search(sEnc_individual_channel_stream* pStream,
                          short *px_quant_unsigned,
                          short *px_quant_signed,
                          short *px_quant_unsigned_pred,
                          short *px_quant_signed_pred);

void tree_build(int sfb_bit_len[MAX_SFB][12],
                int cb_trace[MAX_SFB][12],
                int max_sfb,
                int len_esc_value);

void bit_count(sEnc_individual_channel_stream* pStream,
               short *px_quant_unsigned,
               short *px_quant_signed,
               int   *sfb_offset,
               int   sfb_bit_len[MAX_SFB][12]);

#ifdef  __cplusplus
}
#endif



#endif//__SEARCH_H
