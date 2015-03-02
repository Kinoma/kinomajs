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

#ifndef __UMC_H264_DEC_CONVERSION_H
#define __UMC_H264_DEC_CONVERSION_H

namespace UMC
{

// declare conversion stages
enum
{
    PREPARE_DECODING            = 0,
    BEFORE_DECODING             = 1,
    AFTER_DECODING              = 2,
    ABSENT_DECODING             = 3,
    ERROR_DECODING              = 4
};

} // namespace UMC

#endif // __UMC_H264_DEC_CONVERSION_H
