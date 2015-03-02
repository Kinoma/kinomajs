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
#include "aac_dec.h"

struct MDCTFwdSpec_32s;
typedef struct MDCTFwdSpec_32s IppsMDCTFwdSpec_32s;

struct MDCTInvSpec_32s;
typedef struct MDCTInvSpec_32s IppsMDCTInvSpec_32s;

typedef struct {
  IppsMDCTInvSpec_32s *p_mdct_inv_long;
  IppsMDCTInvSpec_32s *p_mdct_inv_short;
  Ipp8u  *p_buffer_inv;

  IppsMDCTFwdSpec_32s *p_mdct_fwd_long;
  IppsMDCTFwdSpec_32s *p_mdct_fwd_short;
  Ipp8u  *p_buffer_fwd;
} sFilterbankInt;

#ifdef  __cplusplus
extern  "C" {
#endif

  AACStatus InitFilterbankInt(sFilterbankInt* pBlock, int mode);
  void    FreeFilterbankInt(sFilterbankInt * p_data);
  void    FilterbankDecInt(sFilterbankInt * p_data, Ipp32s *p_in_spectrum,
                           Ipp32s *p_in_prev_samples, int window_sequence,
                           int window_shape, int prev_window_shape,
                           Ipp32s *p_out_samples_1st, Ipp32s *p_out_samples_2nd);
  void    FilterbankEncInt(sFilterbankInt * p_data, Ipp32s *p_in_samples_1st_part,
                           Ipp32s *p_in_samples_2nd_part, int window_sequence,
                           int window_shape, int prev_window_shape,
                           Ipp32s *p_out_spectrum);

#ifdef  __cplusplus
}
#endif
#ifndef PI
#define PI 3.14159265359f
#endif
#endif             // __SAACDEC_FILTERBANK_H
