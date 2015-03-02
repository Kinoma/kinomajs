/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MP4CMN_CONFIG_H__
#define __MP4CMN_CONFIG_H__

#include "mp4cmn_pce.h"

typedef struct
{
    int frameLengthFlag;
    int dependsOnCoreCoder;
    int coreCoderDelay;

    int extensionFlag;

    sProgram_config_element pce;

} sGA_specific_config;

typedef struct
{
    /// AudioSpecificConfig;
    int sbrPresentFlag;
    int audioObjectType;
    int samplingFrequencyIndex;
    int samplingFrequency;
    int channelConfiguration;
    int extensionAudioObjectType;
    int extensionSamplingFrequencyIndex;
    int extensionSamplingFrequency;

    sGA_specific_config GASpecificConfig;

} sAudio_specific_config;

#ifdef  __cplusplus
extern "C" {
#endif

int get_channels_number(sAudio_specific_config * p_data);
int get_sampling_frequency(sAudio_specific_config * p_data, int bHEAAC);
int get_sampling_frequency_index(sAudio_specific_config * p_data);

int dec_audio_specific_config(sAudio_specific_config * p_data,sBitsreamBuffer * p_bs);
int dec_ga_specific_config(sGA_specific_config * p_data,sBitsreamBuffer * p_bs, int samplingFrequencyIndex, int channelConfiguration, int audioObjectType);

/***********************************************************************

    Unpack function(s) (support(s) alternative bitstream representation)

***********************************************************************/

int unpack_audio_specific_config(sAudio_specific_config * p_data,unsigned char **pp_bitstream, int *p_offset);
int unpack_ga_specific_config(sGA_specific_config * p_data,unsigned char **pp_bitstream, int *p_offset, int samplingFrequencyIndex, int channelConfiguration, int audioObjectType);


#ifdef  __cplusplus
}
#endif

#endif//__MP4CMN_CONFIG_H__
