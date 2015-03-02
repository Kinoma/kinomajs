/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __AUDIO_CODEC_PARAMS_H__
#define __AUDIO_CODEC_PARAMS_H__

typedef enum
{
    UNDEF_AUD             = 0x00000000,
    PCM_AUD               = 0x00000001,
    LPCM_AUD              = 0x00000002,
    AC3_AUD               = 0x00000004,
    TWINVQ_AUD            = 0x00000008,

    MPEG1_AUD             = 0x00000100,
    MPEG2_AUD             = 0x00000200,
    MPEG_AUD_LAYER1       = 0x00000010,
    MPEG_AUD_LAYER2       = 0x00000020,
    MPEG_AUD_LAYER3       = 0x00000040,

    MP1L1_AUD             = MPEG1_AUD|MPEG_AUD_LAYER1,
    MP1L2_AUD             = MPEG1_AUD|MPEG_AUD_LAYER2,
    MP1L3_AUD             = MPEG1_AUD|MPEG_AUD_LAYER3,
    MP2L1_AUD             = MPEG2_AUD|MPEG_AUD_LAYER1,
    MP2L2_AUD             = MPEG2_AUD|MPEG_AUD_LAYER2,
    MP2L3_AUD             = MPEG2_AUD|MPEG_AUD_LAYER3,

    VORBIS_AUD            = 0x00000400,
    AAC_AUD               = 0x00000800
} cAudioStreamType;

typedef enum
{
    UNDEF_AUD_SUBTYPE    = 0x00000000,
    AAC_LC_PROF          = 0x00000001,
    AAC_LTP_PROF         = 0x00000002,
    AAC_MAIN_PROF        = 0x00000004,
    AAC_SSR_PROF         = 0x00000008,
    AAC_HE_PROF          = 0x00000010
} cAudioStreamSubType;


typedef struct
{
    int channels;                                           // (int) number of audio channels
    int sample_frequency;                                   // (int) sample rate in Hz
    int bitrate;                                       // (vm_var32) bitstream in bps
    int bitPerSample;                                  // (vm_var32) 0 if compressed

    double duration;                                        // (double) duration of the stream

    cAudioStreamType stream_type;                            // (AudioStreamType) general type of stream
    cAudioStreamSubType stream_subtype;                      // (AudioStreamSubType) minor type of stream

    int channel_mask;                                  // (vm_var32) channel mask
} cAudioStreamInfo;

typedef struct
{
    int is_valid;
    int m_SuggestedInputSize;
    cAudioStreamInfo m_info_in;                                  // (AudioStreamInfo) original audio stream info
    cAudioStreamInfo m_info_out;                                 // (AudioStreamInfo) output audio stream info

    int m_frame_num;                                       // (vm_var32) keeps number of processed frames
} cAudioCodecParams;

#endif
