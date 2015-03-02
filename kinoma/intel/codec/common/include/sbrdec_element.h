/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __SBRDEC_ELEMENT_H__
#define __SBRDEC_ELEMENT_H__
#include "sbrdec.h"
#include "bstream.h"

#ifdef  __cplusplus
extern  "C" {
#endif

  Ipp32s sbr_grid(sBitsreamBuffer * BS, Ipp32s* bs_frame_class, Ipp32s* bs_pointer, Ipp16s* r,
                  Ipp32s* tE, Ipp32s* tQ, Ipp32s* LE, Ipp32s* LQ, Ipp32s* status);

  void    sbr_grid_coupling(sSbrDecCommon * pSbr);

  Ipp32s  sbr_dtdf(sBitsreamBuffer * BS, Ipp32s* bs_df_env, Ipp32s* bs_df_noise, Ipp32s LE, Ipp32s LQ);
  Ipp32s  sbr_invf(Ipp32s ch, sBitsreamBuffer * BS, sSbrDecCommon * pSbr);

  Ipp32s  sbr_envelope(Ipp32s ch, Ipp32s bs_coupling, Ipp32s bs_amp_res,
                       sBitsreamBuffer * BS, sSbrDecCommon * pDst);

  Ipp32s sbr_noise(sBitsreamBuffer* BS, Ipp16s* vNoise, Ipp32s* vSize, Ipp32s* bs_df_noise,
              void* sbrHuffTables[10], Ipp32s ch, Ipp32s bs_coupling, Ipp32s LQ, Ipp32s NQ);

  Ipp32s  sbr_sinusoidal_coding(sBitsreamBuffer * BS, Ipp32s* pDst, Ipp32s len);

  Ipp32s  sbrCalcDerivedFreqTables(Ipp32s bs_xover_band, Ipp32s bs_noise_bands, Ipp32s k2,
                                   sSbrDecCommon* pSbr);

  Ipp32s  sbrPatchConstruction(sSbrDecCommon* pSbr, Ipp32s sbrFreqIndx);

  Ipp32s  sbrEnvNoiseDec(sSbrDecCommon * pSbr, Ipp32s ch);

#ifdef  __cplusplus
}
#endif
#endif/*__SBRDEC_ELEMENT_H__ */
/* EOF */
