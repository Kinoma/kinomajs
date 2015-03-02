/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//        Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//
*/

#ifndef __UMC_H264_DEC_SLICE_DECODER_MT_H
#define __UMC_H264_DEC_SLICE_DECODER_MT_H

#include "umc_h264_dec_slice_decoder.h"

namespace UMC
{

class H264SliceDecoderMultiThreaded : public H264SliceDecoder
{
public:
    // Default constructor
    H264SliceDecoderMultiThreaded(H264SliceStore_ &Store);
    // Destructor
    virtual
    ~H264SliceDecoderMultiThreaded(void);

    // Initialize slice decoder
    virtual
    Status Init(Ipp32s iNumber);
};

} // namespace UMC

#endif // __UMC_H264_DEC_SLICE_DECODER_H
