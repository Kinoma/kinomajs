/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_AAC_DECODER_PARAMS_H__
#define __UMC_AAC_DECODER_PARAMS_H__

#include "umc_audio_codec.h"
#include "aaccmn_const.h"
#include "sbrdec.h"

namespace UMC
{

  class   AACDecoderParams:public AudioCodecParams_V51 {
  public:
    AACDecoderParams(void) {
      ModeDecodeHEAACprofile = HEAAC_HQ_MODE;
      ModeDwnmxHEAACprofile = HEAAC_DWNSMPL_ON;
    };
    int     ModeDecodeHEAACprofile;
    int     ModeDwnmxHEAACprofile;
  };

}// namespace UMC

#endif /* __UMC_AAC_DECODER_PARAMS_H__ */
