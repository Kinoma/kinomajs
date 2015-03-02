/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//     Intel Integrated Performance Primitives AAC Decode Sample for Windows*
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

#ifndef __SFB_TABLES_H
#define __SFB_TABLES_H

typedef struct {
  int     samp_rate;
  int     num_sfb_long_window;
  int    *sfb_offset_long_window;
  int     num_sfb_short_window;
  int    *sfb_offset_short_window;

} sSfbTableElement;

#ifdef  __cplusplus
extern  "C" {
#endif

  extern sSfbTableElement sfb_tables[];

#ifdef  __cplusplus
}
#endif
#endif             // __SFB_TABLES_H
