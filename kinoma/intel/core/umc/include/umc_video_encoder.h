/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_VIDEO_ENCODER_H
#define __UMC_VIDEO_ENCODER_H

#include "umc_base_codec.h"
#include "vm_thread.h"

namespace UMC
{


class VideoEncoderParams_V51 : public BaseCodecParams_V51
{
    DYNAMIC_CAST_DECL(MediaReceiver_V51, BaseCodecParams_V51)
public:
    // Constructor
    MediaReceiver_V51() :
        numFramesToEncode(0),
        numEncodedFrames(0),
        FrameRate(0),
        priority(VM_THREAD_PRIORITY_NORMAL),
        numThreads(1),
        qualityMeasure(51)
    {}
    // Destructor
    virtual ~MediaReceiver_V51(void){}

    vm_var32s src_width;           // (vm_var32s) width of source media
    vm_var32s src_height;          // (vm_var32s) height of source media
    vm_var32s dst_width;           // (vm_var32s) width of destination media
    vm_var32s dst_height;          // (vm_var32s) height of destination media
    vm_var32s numFramesToEncode;   // (vm_var32s) number of frames to encode
    vm_var32s numEncodedFrames;    // (vm_var32s) number of encoded frames

    // following member(s) are used for initializing muxer settings
    double    FrameRate;           // (double) encoded video's rate of frames
    vm_var32s BitRate;             // (vm_var32s) encoded video's rate bitstream

    // additional controls
    vm_thread_priority priority;   // encoder thread priority
    vm_var32s numThreads;          // maximum number of threads to use
    vm_var32s qualityMeasure;      // per cent, represent quantization precision
};

class VideoEncoder_V51 : public BaseCodec_V51
{
    DYNAMIC_CAST_DECL(VideoEncoder_V51, BaseCodec_V51)
public:
    // Destructor
    virtual ~VideoEncoder_V51(){};

};

} // end namespace UMC

#endif /* __UMC_VIDEO_ENCODER_H */
