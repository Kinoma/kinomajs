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
#include "umc_h264_dec_tables.h"

namespace UMC_H264_DECODER
{

// Offset for 8x8 blocks
const Ipp8u xoff8[4] = {0,8,0,8};
const Ipp8u yoff8[4] = {0,0,8,8};

// Offsets to advance from one luma subblock to the next, using 8x8
// block ordering of subblocks (ie, subblocks 0..3 are the subblocks
// in 8x8 block 0. Table is indexed by current subblock, containing a
// pair of values for each. The first value is the x offset to be added,
// the second value is the pitch multiplier to use to add the y offset.
const Ipp8s xyoff[16][2] = {
    {4,0},{-4,4},{4,0},{4,-4},
    {4,0},{-4,4},{4,0},{-12,4},
    {4,0},{-4,4},{4,0},{4,-4},
    {4,0},{-4,4},{4,0},{-12,4}};

const Ipp8s xyoff8[4][2]  =
    {
        {8,0},{-8,8},{8,0},{0,0}
    };
//////////////////////////////////////////////////////////
// scan matrices, for Run Length Decoding

const
Ipp32s mp_scan4x4[2][16] =
{
    {
        0,  1,  4,  8,
        5,  2,  3,  6,
        9,  12, 13, 10,
        7,  11, 14, 15
    },
    {
        0,  4,  1,  8,
        12, 5,  9,  13,
        2,  6,  10, 14,
        3,  7,  11, 15
    }
};

#ifndef DROP_HIGH_PROFILE
const Ipp32s hp_scan4x4[2][4][16] =
{
    {
        {
             0, 1, 8,16,
             9, 2, 3,10,
            17,24,25,18,
            11,19,26,27
        },
        {
             4, 5,12,20,
            13, 6, 7,14,
            21,28,29,22,
            15,23,30,31
        },
        {
            32,33,40,48,
            41,34,35,42,
            49,56,57,50,
            43,51,58,59
        },
        {
            36,37,44,52,
            45,38,39,46,
            53,60,61,54,
            47,55,62,63
        },
    },
    {
        {
             0, 8, 1,16,
            24, 9,17,25,
             2,10,18,26,
             3,11,19,27
        },
        {
             4,12, 5,20,
            28,13,21,29,
             6,14,22,30,
             7,15,23,31
        },
        {
            32,40,33,48,
            56,41,49,57,
            34,42,50,58,
            35,43,51,59
        },
        {
            36,44,37,52,
            60,45,53,61,
            38,46,54,62,
            39,47,55,63
        }
    }
};

const Ipp32s hp_scan8x8[2][64] =
{
    //8x8 zigzag scan
    {
        0, 1, 8,16, 9, 2, 3,10,
        17,24,32,25,18,11, 4, 5,
        12,19,26,33,40,48,41,34,
        27,20,13, 6, 7,14,21,28,
        35,42,49,56,57,50,43,36,
        29,22,15,23,30,37,44,51,
        58,59,52,45,38,31,39,46,
        53,60,61,54,47,55,62,63
    },
//8x8 field scan
    {
        0, 8,16, 1, 9,24,32,17,
        2,25,40,48,56,33,10, 3,
        18,41,49,57,26,11, 4,19,
        34,42,50,58,27,12, 5,20,
        35,43,51,59,28,13, 6,21,
        36,44,52,60,29,14,22,37,
        45,53,61,30, 7,15,38,46,
        54,62,23,31,39,47,55,63
    }
};
const Ipp32s hp_membership[2][64] =
{
    //8x8 zigzag scan
    {
        0,0,0,0,0,0,0,0,
        0,0,2,0,0,0,1,1,
        1,0,0,2,2,2,2,2,
        0,1,1,1,1,1,1,1,
        2,2,2,2,2,2,2,3,
        1,1,1,1,1,3,3,2,
        2,2,3,3,3,1,3,3,
        3,3,3,3,3,3,3,3
    },
//8x8 field scan
    {
        0,0,0,0,0,0,2,0,
        0,0,2,2,2,2,0,0,
        0,2,2,2,0,0,1,0,
        2,2,2,2,0,1,1,1,
        2,2,2,2,1,1,1,1,
        3,3,3,3,1,1,1,3,
        3,3,3,1,1,1,3,3,
        3,3,1,1,3,3,3,3
    }
};
const Ipp32s hp_CtxIdxInc_sig_coeff_flag[2][63] =
{
    //8x8 zigzag scan
    {
         0, 1, 2, 3, 4, 5, 5, 4,
         4, 3, 3, 4, 4, 4, 5, 5,
         4, 4, 4, 4, 3, 3, 6, 7,
         7, 7, 8, 9,10, 9, 8, 7,
         7, 6,11,12,13,11, 6, 7,
         8, 9,14,10, 9, 8, 6,11,
        12,13,11, 6, 9,14,10, 9,
        11,12,13,11,14,10,12
    },
    //8x8 field scan
    {
         0, 1, 1, 2, 2, 3, 3, 4,
         5, 6, 7, 7, 7, 8, 4, 5,
         6, 9,10,10, 8,11,12,11,
         9, 9,10,10, 8,11,12,11,
         9, 9,10,10, 8,11,12,11,
         9, 9,10,10, 8,13,13, 9,
         9,10,10, 8,13,13, 9, 9,
        10,10,14,14,14,14,14
    }
};

const Ipp32s hp_CtxIdxInc_last_sig_coeff_flag[63] =
{
    0,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,
    5,5,5,5,6,6,6,6,
    7,7,7,7,8,8,8
};
#endif

// chroma QP mapping
const Ipp8u QPtoChromaQP[52] =
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
    20,21,22,23,24,25,26,27,28,29,29,30,31,32,32,33,
    34,34,35,35,36,36,37,37,37,38,38,38,39,39,39,39};

///////////////////////////////////////
// Tables for decoding CBP

const Ipp8u dec_cbp_inter[2][48] =
{
    {
      0, 1, 2, 4, 8, 3, 5,10,
     12,15, 7,11,13,14, 6, 9,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0
    },
    {
      0,16, 1, 2, 4, 8,32, 3,
      5,10,12,15,47, 7,11,13,
     14, 6, 9,31,35,37,42,44,
     33,34,36,40,39,43,45,46,
     17,18,20,24,19,21,26,28,
     23,27,29,30,22,25,38,41
    }
};
const Ipp8u dec_cbp_intra[2][48] =
{
    {
     15, 0, 7,11,13,14, 3, 5,
     10,12, 1, 2, 4, 8, 6, 9,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0
    },
    {
     47,31,15, 0,23,27,29,30,
      7,11,13,14,39,43,45,46,
     16, 3, 5,10,12,19,21,26,
     28,35,37,42,44, 1, 2, 4,
      8,17,18,20,24, 6, 9,22,
      25,32,33,34,36,40,38,41
    }
};


// Mapping from 8x8 block number to 4x4 block number
const
Ipp8u block_subblock_mapping[16] =
{
    0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15
};
/*
const
Ipp8u chroma_block_subblock_mapping[4][16] =
{
    {0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15},
    {0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15},
    {0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15},
    {0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15}
};*/

const
Ipp8u subblock_block_mapping[4] =
{
    0,2,8,10
};

// The purpose of the these thresholds is to prevent that single or
// 'expensive' coefficients are coded. With 4x4 transform there is
// larger chance that a single coefficient in a 8x8 or 16x16 block
// may be nonzero. A single small (level=1) coefficient in a 8x8 block
// will cost in bits: 3 or more bits for the coefficient, 4 bits for
// EOBs for the 4x4 blocks, possibly also more bits for CBP.  Hence
// the total 'cost' of that single coefficient will typically be 10-12
// bits which in a RD consideration is too much to justify the Distortion
// improvement.  The action below is to watch such 'single' coefficients
// and set the reconstructed block equal to the prediction according to
// a given criterium.  The action is taken only for inter blocks. Action
// is taken only for luma blocks. Notice that this is a pure encoder issue
// and hence does not have any implication on the standard.

// i22 is a parameter set in dct4 and accumulated for each 8x8 block.
// If level=1 for a coefficient, i22 is increased by a number depending
// on RUN for that coefficient.  The numbers are (see also dct4):
// 3,2,2,1,1,1,0,0,... when RUN equals 0,1,2,3,4,5,6,  etc.
// If level >1 i22 is increased by 9 (or any number above 3).
// The threshold is set to 3.  This means for example:
//    1: If there is one coefficient with (RUN,level)=(0,1) in a
//       8x8 block this coefficient is discarded.
//    2: If there are two coefficients with (RUN,level)=(1,1) and
//       (4,1) the coefficients are also discarded

// i33 is the accumulation of i22 over a whole macroblock.  If i33 is
// 5 or less for the whole MB, all nonzero coefficients are discarded
// for the MB and the reconstructed block is set equal to the prediction.
//
// Search for i22 and i33 to see how the thresholds are used


// Tables used for finding if a luma block is on the edge
// of a macroblock. JVT CD block order
// tab4, indexed by 8x8 block
const Ipp8u left_edge_tab4[4]        = {1,0,1,0};
const Ipp8u top_edge_tab4[4]        = {1,1,0,0};
const Ipp8u right_edge_tab4[4]        = {0,1,0,1};
// tab16, indexed by 4x4 subblock
const Ipp8u left_edge_tab16[16]     = {1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0};
const Ipp8u top_edge_tab16[16]        = {1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0};
const Ipp8u right_edge_tab16[16]     = {0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1};
// 8x4 and 4x8 tables, indexed by [8x8block*4 + subblock]
const Ipp8u left_edge_tab16_8x4[16]     = {1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0};
const Ipp8u top_edge_tab16_8x4[16]        = {1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0};
const Ipp8u right_edge_tab16_8x4[16]     = {0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0};
const Ipp8u left_edge_tab16_4x8[16]     = {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0};
const Ipp8u top_edge_tab16_4x8[16]        = {1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0};
const Ipp8u right_edge_tab16_4x8[16]     = {0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0};

// Tables for MV prediction to find if upper right predictor is
// available, indexed by [8x8block*4 + subblock]
const Ipp8u above_right_avail_8x4[16] = {0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0};
const Ipp8u above_right_avail_4x8[16] = {0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,0};

// Table for 4x4 intra prediction to find if a subblock can use predictors
// from above right. Also used for motion vector prediction availability.
// JVT CD block order.
const Ipp8u above_right_avail_4x4[16] = {1,1,1,0,1,1,1,0,1,1,1,0,1,0,1,0};
const Ipp8u above_right_avail_4x4_lin[16] = {
    1,1,1,1,
    1,0,1,0,
    1,1,1,0,
    1,0,1,0
};
const Ipp8u subblock_block_membership[16] = {
        0,0,1,1,
        0,0,1,1,
        2,2,3,3,
        2,2,3,3
};

const Ipp8u default_intra_scaling_list4x4[16]=
{
     6, 13, 20, 28, 13, 20, 28, 32, 20, 28, 32, 37, 28, 32, 37, 42
};
const Ipp8u default_inter_scaling_list4x4[16]=
{
    10, 14, 20, 24, 14, 20, 24, 27, 20, 24, 27, 30, 24, 27, 30, 34
};

const Ipp8u default_intra_scaling_list8x8[64]=
{
     6, 10, 13, 16, 18, 23, 25, 27, 10, 11, 16, 18, 23, 25, 27, 29,
    13, 16, 18, 23, 25, 27, 29, 31, 16, 18, 23, 25, 27, 29, 31, 33,
    18, 23, 25, 27, 29, 31, 33, 36, 23, 25, 27, 29, 31, 33, 36, 38,
    25, 27, 29, 31, 33, 36, 38, 40, 27, 29, 31, 33, 36, 38, 40, 42
};
const Ipp8u default_inter_scaling_list8x8[64]=
{
     9, 13, 15, 17, 19, 21, 22, 24, 13, 13, 17, 19, 21, 22, 24, 25,
    15, 17, 19, 21, 22, 24, 25, 27, 17, 19, 21, 22, 24, 25, 27, 28,
    19, 21, 22, 24, 25, 27, 28, 30, 21, 22, 24, 25, 27, 28, 30, 32,
    22, 24, 25, 27, 28, 30, 32, 33, 24, 25, 27, 28, 30, 32, 33, 35
};

const Ipp32s pre_norm_adjust_index4x4[16] =
{// 0 1 2 3
    0,2,0,2,//0
    2,1,2,1,//1
    0,2,0,2,//2
    2,1,2,1 //3
};

const Ipp32s pre_norm_adjust4x4[6][3] =
{
    {10,16,13},
    {11,18,14},
    {13,20,16},
    {14,23,18},
    {16,25,20},
    {18,29,23}
};

const Ipp32s pre_norm_adjust8x8[6][6] =
{
    {20, 18, 32, 19, 25, 24},
    {22, 19, 35, 21, 28, 26},
    {26, 23, 42, 24, 33, 31},
    {28, 25, 45, 26, 35, 33},
    {32, 28, 51, 30, 40, 38},
    {36, 32, 58, 34, 46, 43}
};
const Ipp32s pre_norm_adjust_index8x8[64] =
{// 0 1 2 3 4 5 6 7
    0,3,4,3,0,3,4,3,//0
    3,1,5,1,3,1,5,1,//1
    4,5,2,5,4,5,2,5,//2
    3,1,5,1,3,1,5,1,//3
    0,3,4,3,0,3,4,3,//4
    3,1,5,1,3,1,5,1,//5
    4,5,2,5,4,5,2,5,//6
    3,1,5,1,3,1,5,1 //7
};
const Ipp8s ClipQPTable[52*3]=
{
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        10,11,12,13,14,15,16,17,18,19,
        20,21,22,23,24,25,26,27,28,29,
        30,31,32,33,34,35,36,37,38,39,
        40,41,42,43,44,45,46,47,48,49,
        50,51,
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        10,11,12,13,14,15,16,17,18,19,
        20,21,22,23,24,25,26,27,28,29,
        30,31,32,33,34,35,36,37,38,39,
        40,41,42,43,44,45,46,47,48,49,
        50,51,
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        10,11,12,13,14,15,16,17,18,19,
        20,21,22,23,24,25,26,27,28,29,
        30,31,32,33,34,35,36,37,38,39,
        40,41,42,43,44,45,46,47,48,49,
        50,51
};
const Ipp32u num_blocks[]={4,4,8,16};
const Ipp32u x_pos_value[4][16]=
{
    {//400
        0,1,
        0,1
    },
    {//420
        0,1,
        0,1
    },
    {//422
        0,1,
        0,1,
        0,1,
        0,1
    },
    {//444
        0,1,2,3,
        0,1,2,3,
        0,1,2,3,
        0,1,2,3
    }
};
const Ipp32u y_pos_value[4][16]={
    {//400
        0,0,
        1,1
    },
    {//420
        0,0,
        1,1
    },
    {//422
        0,0,
        1,1,
        2,2,
        3,3
    },
    {//444
        0,0,0,0,
        1,1,1,1,
        2,2,2,2,
        3,3,3,3
    }
};
const Ipp32u block_y_values[4][4]=
{
    {
        0, 2, 0, 2
    },
    {
        0, 2, 0, 2
    },
    {
        0, 2, 4, 6
    },
    {
        0, 4, 8, 12,
    }
};
const Ipp32s dec_values[]={20,20,24,32};
const Ipp32u mb_c_width[]={2,2,2,4};
const Ipp32u mb_c_height[]={2,2,4,4};
const Ipp32u first_v_ac_block[]={23,23,27,35};
const Ipp32u last_v_ac_block[]={26,26,34,50};

} // end namespace UMC
