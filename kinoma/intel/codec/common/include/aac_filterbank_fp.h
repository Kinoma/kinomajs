/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __SAACDEC_FILTERBANK_H
#define __SAACDEC_FILTERBANK_H

#include "ipps.h"
#include "ippac.h"
#include "aaccmn_const.h"
#include "aac_status.h"

typedef struct {
  IppsMDCTInvSpec_32f *p_mdct_inv_long;
  IppsMDCTInvSpec_32f *p_mdct_inv_short;
  Ipp8u  *p_buffer_inv;

  IppsMDCTFwdSpec_32f *p_mdct_fwd_long;
  IppsMDCTFwdSpec_32f *p_mdct_fwd_short;
  Ipp8u  *p_buffer_fwd;

  float   KBD_long_wnd_table[N_LONG];
  float   KBD_short_wnd_table[N_SHORT];
  float   sin_long_wnd_table[N_LONG];
  float   sin_short_wnd_table[N_SHORT];
} sFilterbank;

#ifdef  __cplusplus
extern  "C" {
#endif

  AACStatus InitFilterbank(sFilterbank* pBlock, int mode,
                           enum AudioObjectType audioObjectType);
  void    FreeFilterbank(sFilterbank * p_data);
  void    FilterbankDec(sFilterbank * p_data, float *p_in_spectrum,
                        float *p_in_prev_samples, int window_sequence,
                        int window_shape, int prev_window_shape,
                        float *p_out_samples_1st, float *p_out_samples_2nd);
  void    FilterbankDecSSR(sFilterbank* p_data, float* p_in_spectrum,
                           int window_sequence, int window_shape,
                           int prev_window_shape, float* p_out_samples);
  void    FilterbankEnc(sFilterbank * p_data, float *p_in_samples_1st_part,
                        float *p_in_samples_2nd_part, int window_sequence,
                        int window_shape, int prev_window_shape,
                        float *p_out_spectrum, int ltp);

#ifdef  __cplusplus
}
#endif
#ifndef PI
#define PI 3.14159265359f
#endif
#endif             // __SAACDEC_FILTERBANK_H
