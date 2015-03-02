/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __AAC_DEC_OWN_INT_H__
#define __AAC_DEC_OWN_INT_H__

#include "ipps.h"

#include "aac_filterbank_int.h"
#include "aac_dec_ltp_int.h"
#include "sbrdec_api_int.h"

#include "aaccmn_chmap.h"
#include "audio_codec_params.h"
#include "aac_dec_own.h"

struct _AACDec {
  AACDec_com           com;
  //***
  int			ignoreFIL;

  Ipp32s         m_prev_samples[CH_MAX + COUPL_CH_MAX][1024 * 2];
  Ipp32s         m_curr_samples[CH_MAX + COUPL_CH_MAX][1024 * 2];
  Ipp16s         m_ltp_buf[CH_MAX + COUPL_CH_MAX][3][1024 * 2];
  Ipp32s         m_spectrum_data[CH_MAX + COUPL_CH_MAX][1024];
  Ipp32s*        m_ordered_samples[CH_MAX + COUPL_CH_MAX];
  Ipp32s         cc_gain[COUPL_CH_MAX][18][MAX_GROUP_NUMBER][MAX_SFB];
  Ipp32s         cc_gain_factor[COUPL_CH_MAX][18][MAX_GROUP_NUMBER][MAX_SFB];
  s_tns_data     tns_data[CH_MAX];

  sSbrBlock*     sbrBlock[CH_MAX];
  sSbrDecFilter* sbr_filter;
  Ipp8u*         pWorkBuffer;

  sFilterbankInt    m_filterbank;
  sLtp           m_ltp;
};

#endif
