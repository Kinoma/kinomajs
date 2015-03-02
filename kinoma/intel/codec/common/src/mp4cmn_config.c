/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "bstream.h"
#include "mp4cmn_config.h"
#include "mp4cmn_pce.h"

int
dec_audio_specific_config(sAudio_specific_config * p_data,sBitsreamBuffer * p_bs)
{
    p_data->audioObjectType = Getbits(p_bs,5);
    p_data->samplingFrequencyIndex = Getbits(p_bs,4);

    if ( 0x0f == p_data->samplingFrequencyIndex )
    {
        p_data->samplingFrequency = Getbits(p_bs,24);
    }
    p_data->channelConfiguration = Getbits(p_bs,4);

    p_data->sbrPresentFlag = -1;

    p_data->extensionAudioObjectType = 0;
    p_data->extensionSamplingFrequency = 0;
    p_data->extensionSamplingFrequencyIndex = 0;

    if (5 == p_data->audioObjectType)
    {
        p_data->extensionAudioObjectType = p_data->audioObjectType;
        p_data->sbrPresentFlag = 1;
        p_data->extensionSamplingFrequencyIndex = Getbits(p_bs,4);
        if ( 0x0f == p_data->extensionSamplingFrequencyIndex )
        {
            p_data->extensionSamplingFrequency = Getbits(p_bs,24);
        }
        p_data->audioObjectType = Getbits(p_bs,5);
    }
    switch (p_data->audioObjectType)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
        dec_ga_specific_config(&p_data->GASpecificConfig,p_bs,p_data->samplingFrequencyIndex,p_data->channelConfiguration,p_data->audioObjectType);
        break;
    default:
        return -1;
    }

    if (p_data->extensionAudioObjectType != 5 && p_bs->nBit_offset >= 16 )
    {
        int syncExtensionType = Getbits(p_bs,11);

        if (0x2b7 == syncExtensionType)
        {
            p_data->extensionAudioObjectType = Getbits(p_bs,5);

            if ( p_data->extensionAudioObjectType == 5 )
            {
                p_data->sbrPresentFlag = Getbits(p_bs,1);
                if (p_data->sbrPresentFlag == 1)
                {
                    p_data->extensionSamplingFrequencyIndex = Getbits(p_bs, 4);

                    if ( p_data->extensionSamplingFrequencyIndex == 0xf )
                        p_data->extensionSamplingFrequency = Getbits(p_bs, 24);
                }
            }
        }
    }

    return 0;
}

int
dec_ga_specific_config(sGA_specific_config * p_data,sBitsreamBuffer * p_bs, int samplingFrequencyIndex, int channelConfiguration, int audioObjectType)
{

    /// GASpecificConfig();

    p_data->frameLengthFlag = Getbits(p_bs,1);
    p_data->dependsOnCoreCoder = Getbits(p_bs,1);
    if (p_data->dependsOnCoreCoder)
    {
        p_data->coreCoderDelay = Getbits(p_bs,14);
    }
    p_data->extensionFlag = Getbits(p_bs,1);


    if (0 == channelConfiguration)
    {
        dec_program_config_element(&p_data->pce,p_bs);
    }

    return 0;
}

/********************************************************************

    Unpack functions (support alternative bitstream representation)

********************************************************************/

int
unpack_audio_specific_config(sAudio_specific_config * p_data,unsigned char **pp_bitstream, int *p_offset)
{
    p_data->audioObjectType = get_bits(pp_bitstream,p_offset,5);
    p_data->samplingFrequencyIndex = get_bits(pp_bitstream,p_offset,4);

    if ( 0x0f == p_data->samplingFrequencyIndex )
    {
        p_data->samplingFrequency = get_bits(pp_bitstream,p_offset,24);
    }
    p_data->channelConfiguration = get_bits(pp_bitstream,p_offset,4);


    switch (p_data->audioObjectType)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
        unpack_ga_specific_config(&p_data->GASpecificConfig,pp_bitstream,p_offset,p_data->samplingFrequencyIndex,p_data->channelConfiguration,p_data->audioObjectType);
        break;
    default:
        return -1;
    }

    return 0;
}

int
unpack_ga_specific_config(sGA_specific_config * p_data,unsigned char **pp_bitstream, int *p_offset, int samplingFrequencyIndex, int channelConfiguration, int audioObjectType)
{

    /// GASpecificConfig();

    p_data->frameLengthFlag = get_bits(pp_bitstream,p_offset,1);
    p_data->dependsOnCoreCoder = get_bits(pp_bitstream,p_offset,1);
    if (p_data->dependsOnCoreCoder)
    {
        p_data->coreCoderDelay = get_bits(pp_bitstream,p_offset,14);
    }
    p_data->extensionFlag = get_bits(pp_bitstream,p_offset,1);


    if (0 == channelConfiguration)
    {
        unpack_program_config_element(&p_data->pce,pp_bitstream,p_offset);
    }

    return 0;
}


int
get_channels_number(sAudio_specific_config * p_data)
{
    int ch;
    int i;
    sProgram_config_element* p_pce;

    switch(p_data->channelConfiguration)
    {
    case 0:
        {
            p_pce = &p_data->GASpecificConfig.pce;
            ch = 0;
            for ( i = 0; i < p_pce->num_front_channel_elements; i ++)
            {
                ch ++;
                if (0 != p_pce->front_element_is_cpe[i])
                {
                    ch ++;
                }
            }
            for ( i = 0; i < p_pce->num_back_channel_elements; i ++)
            {
                ch ++;
                if (0 != p_pce->back_element_is_cpe[i])
                {
                    ch ++;
                }
            }
            for ( i = 0; i < p_pce->num_side_channel_elements; i ++)
            {
                ch ++;
                if (0 != p_pce->side_element_is_cpe[i])
                {
                    ch ++;
                }
            }
            for ( i = 0; i < p_pce->num_lfe_channel_elements; i ++)
            {
                ch ++;
            }
        }
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        ch = p_data->channelConfiguration;
        break;
    case 7:
        ch = 8;
        break;
    default:
        ch = 0;
    }

    return ch;
}

static int g_sampling_frequency_table[] =
{
    96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0
};

static int g_sampling_frequency_mapping_table[] =
{

  // minimum for each sampling frequency
  92017, // 96000
  75132, // 88200
  55426, // 64000
  46009, // 48000
  37566, // 44100
  27713, // 32000
  23004, // 24000
  18783, // 22050
  13856, // 16000
  11502, // 12000
  9391,  // 11025
  0,     // 8000
  0
};

int
get_sampling_frequency_index(sAudio_specific_config * p_data)
{
    int sampling_frequency_index;
    int sampling_frequency;

    if (0x0f == p_data->samplingFrequencyIndex)
    {
        if ( 0 != p_data->channelConfiguration)
        {
            sampling_frequency = get_sampling_frequency(p_data, 0);
            sampling_frequency_index = 0;
            while (g_sampling_frequency_mapping_table[sampling_frequency_index] > sampling_frequency) sampling_frequency_index ++;
        }
        else
        {
            sampling_frequency_index = p_data->GASpecificConfig.pce.sampling_frequency_index;
        }
    }
    else
    {
        sampling_frequency_index = p_data->samplingFrequencyIndex;
    }

   return sampling_frequency_index;
}


int
get_sampling_frequency(sAudio_specific_config * p_data, int bHEAAC)
{
    int sampling_frequency;

    int dataSamplingFrequency;
    int dataSamplingFrequencyIndex;

    if (bHEAAC)
    {
        dataSamplingFrequency      = p_data->extensionSamplingFrequency;
        dataSamplingFrequencyIndex = p_data->extensionSamplingFrequencyIndex;
    }
    else
    {
        dataSamplingFrequency      = p_data->samplingFrequency;
        dataSamplingFrequencyIndex = p_data->samplingFrequencyIndex;
    }
    if ( 0x0f == dataSamplingFrequencyIndex)
    {
        sampling_frequency = dataSamplingFrequency;
    }
    else
    {
        sampling_frequency = g_sampling_frequency_table[dataSamplingFrequencyIndex];
    }

    return sampling_frequency;
}

