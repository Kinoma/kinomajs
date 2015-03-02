/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_AUDIO_CODEC_H__
#define __UMC_AUDIO_CODEC_H__

#include "umc_structures.h"
#include "umc_base_codec.h"
#include "umc_media_data.h"

namespace UMC
{

class AudioCodecParams_V51 : public BaseCodecParams_V51
{
    DYNAMIC_CAST_DECL(AudioCodecParams_V51, BaseCodecParams_V51)
public:
    AudioCodecParams_V51();

    AudioStreamInfo m_info_in;                                  // (AudioStreamInfo) original audio stream info
    AudioStreamInfo m_info_out;                                 // (AudioStreamInfo) output audio stream info

    vm_var32 m_frame_num;                                       // (vm_var32) keeps number of processed frames
    MediaData_V51 *m_pData;                                         // (MediaData *) initial data potion
};

class AudioCodec_V51 : public BaseCodec_V51
{
    DYNAMIC_CAST_DECL(AudioCodec_V51, BaseCodec_V51)

public:

    // Default constructor
    AudioCodec_V51(void){};
    // Destructor
    virtual ~AudioCodec_V51(void){};

    virtual Status GetDuration(float *p_duration)
    {
        p_duration[0] = (float)-1.0;
        return UMC_NOT_IMPLEMENTED;
    }

protected:

    vm_var32 m_frame_num;                                       // (vm_var32) keeps number of processed frames.
};

class AudioData_V51: public MediaData_V51
{
    DYNAMIC_CAST_DECL(AudioData_V51, MediaData_V51)

public:
    AudioStreamInfo m_info;
    AudioData_V51()
    {memset(&m_info, 0, sizeof(AudioStreamInfo));}

};

} // namespace UMC

#endif /* __UMC_AUDIO_CODEC_H__ */
