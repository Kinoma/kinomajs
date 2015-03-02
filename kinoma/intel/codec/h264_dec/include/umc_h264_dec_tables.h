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
#ifndef __UMC_H264_DEC_TABLES_H__
#define __UMC_H264_DEC_TABLES_H__

#include "ippdefs.h"

namespace UMC_H264_DECODER
{

extern const Ipp8s xyoff[16][2];
extern const Ipp8s xyoff8[4][2];

// Offset for 8x8 blocks
extern const Ipp8u xoff8[4];
extern const Ipp8u yoff8[4];

//////////////////////////////////////////////////////////
// scan matrices
extern const Ipp32s mp_scan4x4[2][16];
extern const Ipp32s hp_scan4x4[2][4][16];
extern const Ipp32s hp_scan8x8[2][64];
extern const Ipp32s hp_membership[2][64];

extern const Ipp32s hp_CtxIdxInc_sig_coeff_flag[2][63];
extern const Ipp32s hp_CtxIdxInc_last_sig_coeff_flag[63];

//////////////////////////////////////////////////////////
// Mapping from luma QP to chroma QP
extern const Ipp8u QPtoChromaQP[52];

///////////////////////////////////////
// Tables for decoding CBP
extern const Ipp8u dec_cbp_intra[2][48];
extern const Ipp8u dec_cbp_inter[2][48];


// Mapping from 8x8 block number to 4x4 block number
extern const Ipp8u block_subblock_mapping[16];
extern const Ipp8u subblock_block_mapping[4];


// Tables used for finding if a block is on the edge
// of a macroblock
extern const Ipp8u left_edge_tab4[4];
extern const Ipp8u top_edge_tab4[4];
extern const Ipp8u right_edge_tab4[4];
extern const Ipp8u left_edge_tab16[16];
extern const Ipp8u top_edge_tab16[16];
extern const Ipp8u right_edge_tab16[16];
extern const Ipp8u left_edge_tab16_8x4[16];
extern const Ipp8u top_edge_tab16_8x4[16];
extern const Ipp8u right_edge_tab16_8x4[16];
extern const Ipp8u left_edge_tab16_4x8[16];
extern const Ipp8u top_edge_tab16_4x8[16];
extern const Ipp8u right_edge_tab16_4x8[16];

extern const Ipp8u above_right_avail_8x4[16];
extern const Ipp8u above_right_avail_4x8[16];
extern const Ipp8u above_right_avail_4x4[16];
extern const Ipp8u above_right_avail_4x4_lin[16];
extern const Ipp8u subblock_block_membership[16];

extern const Ipp8u default_intra_scaling_list4x4[16];
extern const Ipp8u default_inter_scaling_list4x4[16];
extern const Ipp8u default_intra_scaling_list8x8[64];
extern const Ipp8u default_inter_scaling_list8x8[64];

extern const Ipp32s pre_norm_adjust_index4x4[16];
extern const Ipp32s pre_norm_adjust4x4[6][3];
extern const Ipp32s pre_norm_adjust8x8[6][6];
extern const Ipp32s pre_norm_adjust_index8x8[64];

extern const Ipp8s ClipQPTable[52*3];

extern const Ipp32u num_blocks[];
extern const Ipp32s dec_values[];
extern const Ipp32u mb_c_width[];
extern const Ipp32u mb_c_height[];
extern const Ipp32u x_pos_value[4][16];
extern const Ipp32u y_pos_value[4][16];
extern const Ipp32u block_y_values[4][4];
extern const Ipp32u first_v_ac_block[];
extern const Ipp32u last_v_ac_block[];

} // end namespace UMC

#endif //__UMC_H264_DEC_TABLES_H__
