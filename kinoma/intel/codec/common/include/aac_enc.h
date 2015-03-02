/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __AAC_ENC_H__
#define __AAC_ENC_H__

#include "ipps.h"
#include "aac_status.h"
#include "aaccmn_const.h"

#ifdef __cplusplus
extern "C" {
#endif

  struct _AACEnc;
  typedef struct _AACEnc AACEnc;

  AACStatus aacencInit(AACEnc **state_ptr,
                     int sampling_frequency,
                     int chNum,
                     int bit_rate,
                     enum AudioObjectType audioObjectType);

  AACStatus aacencGetFrame(Ipp16s *inPointer,
                           int *encodedBytes,
                           Ipp8u *outPointer,
                           AACEnc *state);
  AACStatus aacencClose(AACEnc *state);

  AACStatus aacencGetSampleFrequencyIndex(int *freq_index,
                                          AACEnc *state);

  AACStatus aacencGetDuration(float *p_duration,
                              AACEnc *state);

#ifdef __cplusplus
}
#endif

#endif
