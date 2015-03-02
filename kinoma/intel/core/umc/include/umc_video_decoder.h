/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_VIDEO_DECODER_H__
#define __UMC_VIDEO_DECODER_H__

#include "umc_structures.h"
#include "umc_base_color_space_converter.h"
#include "umc_media_data.h"
#include "umc_media_data_ex.h"
#include "umc_base_codec.h"

namespace UMC
{

class VideoDecoderParams_V51 : public BaseCodecParams_V51
{
    DYNAMIC_CAST_DECL(VideoDecoderParams_V51, BaseCodecParams_V51)

public:
    // Default constructor
    VideoDecoderParams_V51();
    // Destructor
    virtual ~VideoDecoderParams_V51();

    MediaData3_V51          *m_pData3;
    MediaData_V51           *m_pData0;
    VideoStreamInfo         info;                           // (VideoStreamInfo) compressed video info
    ColorFormat             cformat;                        // (ColorFormat) uncompressed frame format
    vm_var32                lFlags;                         // (vm_var32) decoding flag(s)
    vm_var32                lTrickModesFlag;                // (vm_var32) trick modes
    vm_var32                uiLimitThreads;                 // (vm_var32) maximum number of decoding thread

    //***double                  dPlaybackRate;
};


class VideoDecoder_V51 : public BaseCodec_V51
{
    DYNAMIC_CAST_DECL(VideoDecoder_V51, BaseCodec_V51)

public:
    VideoDecoder_V51(void)
        :m_pConverter(NULL)
    {m_ClipInfo.framerate=0.;}

    // Destructor
    virtual ~VideoDecoder_V51(void){}

    // Initialize codec with specified parameter(s)
    virtual Status Init(BaseCodecParams_V51 *init) = 0;
    // Compress (decompress) next frame
    virtual Status GetFrame(MediaData_V51 *in, MediaData_V51 *out) = 0;
    // Get codec working (initialization) parameter(s)
    virtual Status GetInfo(BaseCodecParams_V51 *info);

    // Close all codec resources
    virtual Status Close() = 0;
    // Set codec to initial state
    virtual Status Reset() = 0;

    // Reset skip frame counter
    virtual Status ResetSkipCount() = 0;
    // Increment skip frame counter
    virtual Status  SkipVideoFrame(int) = 0;
    // Get skip frame counter statistic
    virtual vm_var32 GetSkipedFrame() = 0;

    virtual Status  SetParams(BaseCodecParams_V51* params);

protected:

    VideoStreamInfo         m_ClipInfo;                         // (VideoStreamInfo) clip info
    ColorConversionParams   m_ConversionParam;                  // (ColorConversionParams) conversion parameters
    ColorConversionParams   m_ConversionParamPreview;           // (ColorConversionParams) conversion parameters
    BaseColorSpaceConverter* m_pConverter;                      // (BaseColorSpaceConverter *) pointer to color space converter
};

} // end namespace UMC

#endif // __UMC_VIDEO_DECODER_H__
