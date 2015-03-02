/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_AAC_ENCODER_PARAMS_H__
#define __UMC_AAC_ENCODER_PARAMS_H__

#include "umc_audio_codec.h"
#include "aaccmn_const.h"

namespace UMC
{

class AACEncoderParams:public AudioCodecParams {
  DYNAMIC_CAST_DECL(AACEncoderParams, AudioCodecParams)
public:
  AACEncoderParams(void) {
    audioObjectType = AOT_AAC_LC;
  };
  enum AudioObjectType audioObjectType;
};

}// namespace UMC

#endif /* __UMC_AAC_ENCODER_PARAMS_H__ */

