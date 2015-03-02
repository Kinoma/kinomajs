/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __SBR_FREQ_TABS_H__
#define __SBR_FREQ_TABS_H__
//#include "sbrdec.h"
//#include "bstream.h"

#ifdef  __cplusplus
extern  "C" {
#endif

  Ipp32s  sbrCalcMasterFreqBoundary(Ipp32s bs_start_freq, Ipp32s bs_stop_freq,
                                    Ipp32s sbrFreqIndx, Ipp32s *k0, Ipp32s *k2);

  Ipp32s  sbrCalcMasterFreqBandTable(Ipp32s k0, Ipp32s k2, Ipp32s bs_freq_scale,
                                     Ipp32s bs_alter_scale, Ipp32s *f_master,
                                     Ipp32s *N_master);

  Ipp32s sbrGetPowerVector( Ipp32s numBands0, Ipp32s k1, Ipp32s k0, Ipp32s* pow_vec);


  Ipp32s  sbrCalcLimiterFreqBandTable(
                               /*
                                * in data
                                */
                                       Ipp32s bs_limiter_bands,
                                       Ipp32s *f_TableLow, Ipp32s N_low,
                                       Ipp32s numPatches,
                                       Ipp32s *patchNumSubbands,
                               /*
                                * out data
                                */
                                       Ipp32s *f_TableLim, Ipp32s *N_L);

#ifdef  __cplusplus
}
#endif
#endif/*__SBR_FREQ_TABS_H__ */
/* EOF */
