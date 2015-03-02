/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "mp4cmn_const.h"

static int g_sampling_frequency_table[] =
{
    96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0
};

int
get_sampling_frequency_by_index(int index)
{
    return g_sampling_frequency_table[index];
}
