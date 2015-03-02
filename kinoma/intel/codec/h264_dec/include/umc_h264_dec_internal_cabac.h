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
#ifndef __UMC_H264_DEC_INTERNAL_CABAC_H
#define __UMC_H264_DEC_INTERNAL_CABAC_H

namespace UMC
{

// See table 9-11 of H.264 standard
enum // Syntax element type
{
    MB_SKIP_FLAG_P_SP            = 0,
    MB_SKIP_FLAG_B               = 1,
    MB_FIELD_DECODING_FLAG       = 2,
    MB_TYPE_SI                   = 3,
    MB_TYPE_I                    = 4,
    MB_TYPE_P_SP                 = 5,
    MB_TYPE_B                    = 6,
    CODED_BLOCK_PATTERN_LUMA     = 7,
    CODED_BLOCK_PATTERN_CHROMA   = 8,
    MB_QP_DELTA                  = 9,
    PREV_INTRA4X4_PRED_MODE_FLAG = 10,
    PREV_INTRA8X8_PRED_MODE_FLAG = 10,
    REM_INTRA4X4_PRED_MODE       = 11,
    REM_INTRA8X8_PRED_MODE       = 11,
    INTRA_CHROMA_PRED_MODE       = 12,
    REF_IDX_L0                   = 13,
    REF_IDX_L1                   = 14,
    MVD_L0_0                     = 15,
    MVD_L1_0                     = 16,
    MVD_L0_1                     = 17,
    MVD_L1_1                     = 18,
    SUB_MB_TYPE_P_SP             = 19,
    SUB_MB_TYPE_B                = 20,
    TRANSFORM_SIZE_8X8_FLAG      = 21,

    MAIN_SYNTAX_ELEMENT_NUMBER
};

// See table 9-30 of H.264 standard
enum // Syntax element type
{
    CODED_BLOCK_FLAG             = 0,
    SIGNIFICANT_COEFF_FLAG       = 1,
    LAST_SIGNIFICANT_COEFF_FLAG  = 2,
    COEFF_ABS_LEVEL_MINUS1       = 3,

    SYNTAX_ELEMENT_NUMBER
};

// See table 9-32 of H.264 standard
enum // Context block category
{
    BLOCK_LUMA_DC_LEVELS         = 0,
    BLOCK_LUMA_AC_LEVELS         = 1,
    BLOCK_LUMA_LEVELS            = 2,
    BLOCK_CHROMA_DC_LEVELS       = 2,
    BLOCK_CHROMA_DC420_LEVELS    = 3,
    BLOCK_CHROMA_DC422_LEVELS    = 4,
    BLOCK_CHROMA_DC444_LEVELS    = 5,
    BLOCK_CHROMA_AC_LEVELS       = 6,
    BLOCK_LUMA8X8_LEVELS         = 7,

    BLOCK_CATEGORY_NUMBER
};

// See table 9-11 of H.264 standard
extern const
Ipp32u ctxIdxOffset[MAIN_SYNTAX_ELEMENT_NUMBER];

// See table 9-24 of H.264 standard
extern const
Ipp32u ctxIdxOffset4x4FrameCoded[SYNTAX_ELEMENT_NUMBER];

extern const
Ipp32u ctxIdxOffset8x8FrameCoded[SYNTAX_ELEMENT_NUMBER];

// See table 9-24 of H.264 standard
extern const
Ipp32u ctxIdxOffset4x4FieldCoded[SYNTAX_ELEMENT_NUMBER];

extern const
Ipp32u ctxIdxOffset8x8FieldCoded[SYNTAX_ELEMENT_NUMBER];

// See table 9-30 of H.264 standard
extern const
Ipp32u ctxIdxBlockCatOffset[SYNTAX_ELEMENT_NUMBER][BLOCK_CATEGORY_NUMBER];

} // end namespace UMC

#endif //__UMC_H264_DEC_INTERNAL_CABAC_H
