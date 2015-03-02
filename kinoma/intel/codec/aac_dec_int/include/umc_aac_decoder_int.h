/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __AAC_DECODER_INT_H__
#define __AAC_DECODER_INT_H__

#include "umc_audio_codec.h"
#include "vm_types.h"
#include "umc_dynamic_cast.h"
#include "ippac.h"
#include "aac_dec.h"
#include "aaccmn_adif.h"
#include "aaccmn_adts.h"
#include "audio_codec_params.h"
#include "umc_aac_decoder_params.h"
#include "sbrdec.h"

namespace UMC {

  class   AACDecoderInt:public AudioCodec_V51 {
    DYNAMIC_CAST_DECL(AACDecoderInt, AudioCodec_V51);
  public:

    enum DecodeMode {
      DM_UNDEF_STREAM = 0,
      DM_RAW_STREAM,
      DM_ADTS_STREAM
    };

    AACDecoderInt();
    ~AACDecoderInt(void);

    Status  Init(BaseCodecParams_V51 * init);
    Status  Init2(unsigned char * esda, long size);	//***
	void SetIgnoreSBR(void);//***
    Status  GetFrame(MediaData_V51 * in, MediaData_V51 * out);
    Status  Close();
    Status  Reset();

    Status  GetInfo(BaseCodecParams_V51 * info);
    Status  SetParams(BaseCodecParams_V51 * params);
    Status  GetDuration(float *p_duration);

  protected:

    Status StatusAAC_2_UMC(AACStatus st);

    AACDec *state;
    cAudioCodecParams params;

    sAdif_header m_adif_header;
    sAdts_fixed_header m_adts_fixed_header;
    sAdts_variable_header m_adts_variable_header;

    double       m_pts_prev;
    int          initSubtype;
    unsigned int adts_sw;

    struct {
      AudioStreamType    m_init_stream_type;
      size_t             m_init_config_data_size;
      DecodeMode         m_decode_mode;
      int                m_sampling_frequency_index;

      unsigned long      m_channel_config;
      int                m_frame_number;
      AudioObjectType    m_audio_object_type;
      AudioStreamSubType m_stream_subtype;
    } m_info;
  };
};      // namespace UMC

#endif
