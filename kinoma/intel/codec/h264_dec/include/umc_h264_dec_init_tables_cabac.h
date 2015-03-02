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
#ifndef __UMC_H264_DEC_INIT_TABLES_CABAC_H__
#define __UMC_H264_DEC_INIT_TABLES_CABAC_H__

#include "umc_h264_dec_defs_dec.h"

namespace UMC
{

// Range table
extern const
Ipp8u rangeTabLPS[64][8];

extern const
Ipp32u rangeTabLPS20[64][8];

extern const
Ipp8u transIdxMPS[64];

extern const
Ipp8u transIdxLPS[64];

extern const
Ipp8u NumBitsToGetTbl[512];

} // end namespace UMC

#endif //__UMC_H264_DEC_INIT_TABLES_CABAC_H__
