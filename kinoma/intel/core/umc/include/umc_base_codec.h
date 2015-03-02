/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __UMC_BASE_CODEC_H
#define __UMC_BASE_CODEC_H

#include "umc_media_data.h"

namespace UMC
{

class BaseCodecParams_V51
{
    DYNAMIC_CAST_DECL_BASE(BaseCodecParams_V51)

public:
    // Default constructor
    BaseCodecParams_V51(void){m_SuggestedInputSize = 0;}
    // Destructor
    virtual ~BaseCodecParams_V51(void){}

    vm_var32 m_SuggestedInputSize;
};

class BaseCodec_V51
{
    DYNAMIC_CAST_DECL_BASE(BaseCodec_V51)

public:
    // Destructor
    virtual ~BaseCodec_V51(void){};

    // Initialize codec with specified parameter(s)
    virtual Status Init(BaseCodecParams_V51 *init) = 0;

    // Compress (decompress) next frame
    virtual Status GetFrame(MediaData_V51 *in, MediaData_V51 *out) = 0;

    // Get codec working (initialization) parameter(s)
    virtual Status GetInfo(BaseCodecParams_V51 *info) = 0;

    // Close all codec resources
    virtual Status Close(void) = 0;

    // Set codec to initial state
    virtual Status Reset(void) = 0;

    // Set new working parameter(s)
    virtual Status SetParams(BaseCodecParams_V51 *params)
    {
        if (NULL == params)
            return UMC_NULL_PTR;

        return UMC_NOT_IMPLEMENTED;
    }

};

} // end namespace UMC

#endif /* __UMC_BASE_CODEC_H */
