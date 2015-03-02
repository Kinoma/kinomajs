/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "ippac.h"
#include "ipps.h"
#include "mp3dec.h"

#ifndef __UMC_MP3DEC_INT_H__
#define __UMC_MP3DEC_INT_H__

#include "umc_audio_codec.h"
#include "vm_types.h"

/* /////////////////////////////////////////////////////////////////////////////
//  Class:       MP3Decoder
//
*/


namespace UMC {

  class   MP3DecoderInt:public AudioCodec_V51 {
  public:
    MP3DecoderInt();
    ~MP3DecoderInt();

    Status  Init(BaseCodecParams_V51 * init);

    Status  GetFrame(MediaData_V51 * in, MediaData_V51 * out);

    Status  Close();
    Status  Reset();

    Status  GetInfo(BaseCodecParams_V51 * info);

    Status  SetParams(BaseCodecParams_V51 * params) {
      return UMC_NOT_IMPLEMENTED;
    };

    Status  GetDuration(float *p_duration);

  protected:
    MP3Dec *state;
    cAudioCodecParams params;
    double  m_pts_prev;

    Status StatusMP3_2_UMC(MP3Status st);
  };

}      // namespace UMC

#endif
