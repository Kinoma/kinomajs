/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __SAACDEC_STREAM_ELEMENTS_H
#define __SAACDEC_STREAM_ELEMENTS_H

#include "bstream.h"
#include "aaccmn_const.h"
#include "sbrdec.h"
#include "aac_dec_own.h"

#ifdef  __cplusplus
extern  "C" {
#endif

  int     ics_apply_scale_factors(s_SE_Individual_channel_stream * pData,
                                  Ipp32s *p_spectrum);
  int     cpe_apply_ms(sCpe_channel_element * pElement, Ipp32s *l_spec,
                       Ipp32s *r_spec);
  int     cpe_apply_intensity(sCpe_channel_element * pElement, Ipp32s *l_spec,
                              Ipp32s *r_spec);

  int     apply_pns(s_SE_Individual_channel_stream *pDataL,
                    s_SE_Individual_channel_stream *pDataR,
                    Ipp32s *p_spectrumL,
                    Ipp32s *p_spectrumR,
                    int   numCh,
                    int   ms_mask_present,
                    int   ms_used[8][49],
                    int   *noiseState);

  int     deinterlieve(s_SE_Individual_channel_stream *pData,
                       int *p_spectrum);

  void    coupling_gain_calculation(sCoupling_channel_element *pElement,
                                    sCoupling_channel_data *pData,
                                    int cc_gain[18][MAX_GROUP_NUMBER][MAX_SFB],
                                    int cc_gain_factor[18][MAX_GROUP_NUMBER][MAX_SFB]);

  void    coupling_spectrum(AACDec *state,
                            sCoupling_channel_data *pData,
                            int *c_spectrum,
                            int curr_win_sequence,
                            int cc_gain[18][MAX_GROUP_NUMBER][MAX_SFB],
                            int cc_gain_factor[18][MAX_GROUP_NUMBER][MAX_SFB]);

  void    coupling_samples(AACDec *state,
                           sCoupling_channel_data *pData,
                           int *c_samlpes,
                           int cc_gain[18][MAX_GROUP_NUMBER][MAX_SFB],
                           int cc_gain_factor[18][MAX_GROUP_NUMBER][MAX_SFB]);

#ifdef  __cplusplus
}
#endif
#endif
