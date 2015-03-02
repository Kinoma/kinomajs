/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "umc_audio_codec.h"
//***bnie: #include "umc_base_codec.h"
//***bnie: #include "umc_base_color_space_converter.h"
//***bnie: #include "umc_media_buffer.h"
//***bnie: #include "umc_data_reader.h"
//***bnie: #include "umc_data_writer.h"
//***bnie: #include "umc_media_receiver.h"
//***bnie: #include "umc_video_encoder.h"
//***bnie: #include "umc_muxer.h"
//***bnie: #include "umc_module_context.h"


namespace UMC
{

AudioCodecParams_V51::AudioCodecParams_V51()
{
    m_frame_num = 0;
    m_pData = NULL;

} // AudioCodecParams::AudioCodecParams()

} // namespace UMC
