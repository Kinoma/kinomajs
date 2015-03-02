/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#include <string.h>
#include "umc_video_decoder.h"

namespace UMC
{

VideoDecoderParams_V51::VideoDecoderParams_V51(void)
{
    m_pData0 = NULL;
    m_pData3 = NULL;
    memset(&info, 0, sizeof(sVideoStreamInfo));
    cformat = NONE;
    lFlags = 0;
    uiLimitThreads = 1;
    lTrickModesFlag= UMC_TRICK_MODES_NO;

} // VideoDecoderParams_V51::VideoDecoderParams_V51(void)

VideoDecoderParams_V51::~VideoDecoderParams_V51(void)
{

} // VideoDecoderParams_V51::~VideoDecoderParams_V51(void)

Status VideoDecoder_V51::GetInfo(BaseCodecParams_V51 *info)
{
    Status umcRes = UMC_OK;
    VideoDecoderParams_V51 *pParams = DynamicCast<VideoDecoderParams_V51> (info);

    if (NULL == pParams)
        umcRes = UMC_NULL_PTR;

    if (UMC_OK == umcRes)
    {
        pParams->info = m_ClipInfo;
    }
    return umcRes;

} // Status VideoDecoder_V51::GetInfo(BaseCodecParams *info)

Status VideoDecoder_V51::SetParams(BaseCodecParams_V51* params)
{
    Status umcRes = UMC_OK;
    VideoDecoderParams_V51 *pParams = DynamicCast<VideoDecoderParams_V51>(params);

    if (NULL == pParams)
        umcRes = UMC_NULL_PTR;

    if (UMC_OK == umcRes)
    {
        m_ClipInfo = pParams->info;
    }

    return umcRes;

} // Status VideoDecoder_V51::SetParams(BaseCodecParams* params)

} // end namespace UMC
