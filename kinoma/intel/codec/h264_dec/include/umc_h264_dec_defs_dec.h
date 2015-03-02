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
#ifndef __UMC_H264_DEC_DEFS_DEC_H__
#define __UMC_H264_DEC_DEFS_DEC_H__

#include <string.h>
#include "ippdefs.h"
#include "ippvc.h"
#include "ipps.h"

#pragma pack(16)

namespace UMC
{

#if defined (_ARM_) || defined (ARM) || defined (_WIN32_WCE)
#define SMALL_MEMORY_USE
#endif

//#define USE_SEI since it's not always correct better turn this off
//
// Define some useful macros
//

#define MAX_NUM_SLICE_GROUPS 8
#define MAX_SLICE_GROUP_MAP_TYPE 6

#undef  MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))

#undef  MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))

#undef  ABS
#define ABS(A) ((A)<(0) ? (-(A)):(A))

#define    _ALIGN(p, n)    ((Ipp8u*)(p) + ((((Ipp8u*)(p) - (Ipp8u*)(0)) & ((n)-1)) ? \
                            ((n) - (((Ipp8u*)(p) - (Ipp8u*)(0)) & ((n)-1))) : 0))

#define    _IS_ALIGNED(p, n)    (!(((Ipp8u*)(p) - (Ipp8u*)(0)) & ((n)-1)))

//#define REDUCE_DPB_TO_MAX_REF_FRAMES //NonStandart REDUCTION!!!
//#define STORE_OUTPUT_YUV
//#define STORE_DFS
//#define STORE_CABAC_BITS
//#define CABAC_DECORER_COMP
//#define CABAC_CONTEXTS_COMP
//#define STORE_VLC
//#define STORE_MVS
//#define STORE_PREDICTORS
//#define STORE_DMVS
//#define STORE_PICLIST
//#define STORE_REFIDXS
//#define STORE_INPUTDATA
//#define STORE_NUTS
//#define STORE_DISPLAY_PICS
//#define STORE_DECODING_PICS

#define __DFS_FILE__ "d:\\dfs.ipp"
#define __VLC_FILE__ "d:\\vlc.ipp"
#define __CABAC_FILE__ "d:\\cabac.tst"
#define __DMVS_FILE__ "d:\\dmv.ipp"
#define __PREDICTORS_FILE__ "d:\\mv_pred.ipp"
#define __MVS_FILE__ "d:\\mv.ipp"
#define __PICLIST_FILE__ "d:\\piclist.ipp"
#define __REFIDXS_FILE__ "d:\\refidxs.ipp"
#define __NUTS_FILE__ "d:\\nuts.ipp"
#define __FINPUT_FILE__ "d:\\finput.ipp"
#define __FLINPUT_FILE__ "d:\\flinput.ipp"
#define __DISPLAYSPICS_FILE__ "d:\\pics_disp.ipp"
#define __DECODINGPICS_FILE__ "d:\\pics_dec.ipp"
#define __YUV_FILE__ "d:\\output.yuv"

enum
{
    ALIGN_VALUE                 = 16
};

// Although the standard allows for a minimum width or height of 4, this
// implementation restricts the minimum value to 32.
#define FLD_STRUCTURE 0
#define TOP_FLD_STRUCTURE 0
#define BOTTOM_FLD_STRUCTURE 1
#define FRM_STRUCTURE 2
#define AFRM_STRUCTURE 3
#ifndef SHARED_ENCDEC_STRUCTURES_DEFS
#define SHARED_ENCDEC_STRUCTURES_DEFS

#define NORMAL_FRAME_PROCESSING 0
#define COMBINE_FIELDS 1
#define REPEAT_LAST_FRAME  2
// Valid QP range
const Ipp32s QP_MAX = 51;
const Ipp32s QP_MIN = 0;

//  NAL Unit Types
#ifndef SHARED_ENCDECBS_STRUCTURES_DEFS
#define SHARED_ENCDECBS_STRUCTURES_DEFS

typedef enum {
        NAL_UT_RESERVED  = 0x00, // Reserved
        NAL_UT_SLICE     = 0x01, // Coded Slice - slice_layer_no_partioning_rbsp
        NAL_UT_DPA       = 0x02, // Coded Data partition A - dpa_layer_rbsp
        NAL_UT_DPB       = 0x03, // Coded Data partition A - dpa_layer_rbsp
        NAL_UT_DPC       = 0x04, // Coded Data partition A - dpa_layer_rbsp
        NAL_UT_IDR_SLICE = 0x05, // Coded Slice of a IDR Picture - slice_layer_no_partioning_rbsp
        NAL_UT_SEI       = 0x06, // Supplemental Enhancement Information - sei_rbsp
        NAL_UT_SPS       = 0x07, // Sequence Parameter Set - seq_parameter_set_rbsp
        NAL_UT_PPS       = 0x08, // Picture Parameter Set - pic_parameter_set_rbsp
        NAL_UT_PD        = 0x09, // Picture Delimiter - pic_delimiter_rbsp
        NAL_UT_FD        = 0x0a  // Filler Data - filler_data_rbsp
} NAL_Unit_Type;
#endif/* SHARED_ENCDECBS_STRUCTURES_DEFS */

// Note!  The Picture Code Type values below are no longer used in the
// core encoder.   It only knows about slice types, and whether or not
// the frame is IDR, Reference or Disposable.  See enum above.

enum EnumSliceCodType        // Permitted MB Prediction Types
{                        // ------------------------------------
    PREDSLICE      = 0,    // I (Intra), P (Pred)
    BPREDSLICE     = 1, // I, P, B (BiPred)
    INTRASLICE       = 2,    // I
    S_PREDSLICE    = 3,    // SP (SPred), I
    S_INTRASLICE   = 4    // SI (SIntra), I
};


typedef enum {
    MBTYPE_INTRA,            // 4x4
    MBTYPE_INTRA_16x16,
    MBTYPE_PCM,                // Raw Pixel Coding, qualifies as a INTRA type...
    MBTYPE_INTER,            // 16x16
    MBTYPE_INTER_16x8,
    MBTYPE_INTER_8x16,
    MBTYPE_INTER_8x8,
    MBTYPE_INTER_8x8_REF0,
    MBTYPE_FORWARD,
    MBTYPE_BACKWARD,
    MBTYPE_SKIPPED,
    MBTYPE_DIRECT,
    MBTYPE_BIDIR,
    MBTYPE_FWD_FWD_16x8,
    MBTYPE_FWD_FWD_8x16,
    MBTYPE_BWD_BWD_16x8,
    MBTYPE_BWD_BWD_8x16,
    MBTYPE_FWD_BWD_16x8,
    MBTYPE_FWD_BWD_8x16,
    MBTYPE_BWD_FWD_16x8,
    MBTYPE_BWD_FWD_8x16,
    MBTYPE_BIDIR_FWD_16x8,
    MBTYPE_BIDIR_FWD_8x16,
    MBTYPE_BIDIR_BWD_16x8,
    MBTYPE_BIDIR_BWD_8x16,
    MBTYPE_FWD_BIDIR_16x8,
    MBTYPE_FWD_BIDIR_8x16,
    MBTYPE_BWD_BIDIR_16x8,
    MBTYPE_BWD_BIDIR_8x16,
    MBTYPE_BIDIR_BIDIR_16x8,
    MBTYPE_BIDIR_BIDIR_8x16,
    MBTYPE_B_8x8,
    NUMBER_OF_MBTYPES
} MB_Type;


// 8x8 Macroblock subblock type definitions
typedef enum {
    SBTYPE_8x8 = 0,        // P slice modes
    SBTYPE_8x4 = 1,
    SBTYPE_4x8 = 2,
    SBTYPE_4x4 = 3,
    SBTYPE_DIRECT = 4,            // B Slice modes
    SBTYPE_FORWARD_8x8 = 5,        // Subtract 4 for mode #
    SBTYPE_BACKWARD_8x8 = 6,
    SBTYPE_BIDIR_8x8 = 7,
    SBTYPE_FORWARD_8x4 = 8,
    SBTYPE_FORWARD_4x8 = 9,
    SBTYPE_BACKWARD_8x4 = 10,
    SBTYPE_BACKWARD_4x8 = 11,
    SBTYPE_BIDIR_8x4 = 12,
    SBTYPE_BIDIR_4x8 = 13,
    SBTYPE_FORWARD_4x4 = 14,
    SBTYPE_BACKWARD_4x4 = 15,
    SBTYPE_BIDIR_4x4 = 16
} SB_Type;
#endif /*SHARED_ENCDEC_STRUCTURES_DEFS*/

#define IS_I_SLICE(SliceType) ((SliceType) == INTRASLICE)
#define IS_P_SLICE(SliceType) ((SliceType) == PREDSLICE || (SliceType) == S_PREDSLICE)
#define IS_B_SLICE(SliceType) ((SliceType) == BPREDSLICE)

#define MAX_SKIP_REPEAT 4
#define SKIP_DEBLOCKING_MODE 1
#define SKIP_NONREF_FRAMES_MODE 10

#define SKIP_THRESHOLD4 64
#define SKIP_THRESHOLD3 8
#define SKIP_THRESHOLD2 2
#define SKIP_THRESHOLD1 1

#define SKIP_PER_CYCLE 1
#define NUM_SKIP_PER_CYCLE 1

#define INCREASE_SKIP_REPEAT_COUNTER(value)(m_SkipRepeat+=(value))
#define IS_SKIP_DEBLOCKING_MODE (m_PermanentTurnOffDeblocking>0)
#define IS_SKIP_NONREF_FRAMES_MODE(mode) ((mode)>=SKIP_DEBLOCKING_MODE /*&& (mode)<SKIP_NONREF_FRAMES_MODE*/)
#define IS_DECODE_ONLY_INTRA_REF_SLICES(mode) ((mode)>=100000) /*SKIP_NONREF_FRAMES_MODE)*/
#define QPFromCode(x) ClipQPTable[x+52];
// Macroblock type definitions
// Keep these ordered such that intra types are first, followed by
// inter types.  Otherwise you'll need to change the definitions
// of IS_INTRA_MBTYPE and IS_INTER_MBTYPE.
//
// WARNING:  Because the decoder exposes macroblock types to the application,
// these values cannot be changed without affecting users of the decoder.
// If new macro block types need to be inserted in the middle of the list,
// then perhaps existing types should retain their numeric value, the new
// type should be given a new value, and for coding efficiency we should
// perhaps decouple these values from the ones that are encoded in the
// bitstream.
//
//

//***this siginficantly reduces the memory   --bnie 9/27/07
//#define MAX_NUM_SEQ_PARAM_SETS	8
//#define MAX_NUM_PIC_PARAM_SETS 16
//#define MAX_SLICE_NUM 16
//#define MAX_NUM_REF_FRAMES 8

#ifndef MAX_NUM_SEQ_PARAM_SETS
#define MAX_NUM_SEQ_PARAM_SETS 32
#endif

#ifndef MAX_NUM_PIC_PARAM_SETS
#define MAX_NUM_PIC_PARAM_SETS 256
#endif

// Possible values for disable_deblocking_filter_idc:
#define DEBLOCK_FILTER_ON                   0
#define DEBLOCK_FILTER_OFF                  1
#define DEBLOCK_FILTER_ON_NO_SLICE_EDGES    2

#define MAX_DEC_BATCH_MB 120*68//45*30

#ifndef MAX_SLICE_NUM
#define MAX_SLICE_NUM 64 //INCREASE IF NEEDED OR SET to -1 for adaptive counting (increases memory usage)
#endif

#ifndef MAX_NUM_REF_FRAMES
#define MAX_NUM_REF_FRAMES 32
#endif

// macro - yields TRUE if a given MB type is INTRA
#define IS_INTRA_MBTYPE(mbtype) ((mbtype) < MBTYPE_INTER)

// macro - yields TRUE if a given MB type is INTER
#define IS_INTER_MBTYPE(mbtype) ((mbtype) >= MBTYPE_INTER)

#define SCLFLAT16     0
#define SCLDEFAULT    1
#define SCLREDEFINED  2
struct H264ScalingList4x4
{
   Ipp8u ScalingListCoeffs[16];
};
struct H264ScalingList8x8
{
    Ipp8u ScalingListCoeffs[64];
};
/*struct H264LevelScale4x4
{
    Ipp16u LevelScaleCoeffs[16];
};
struct H264LevelScale8x8
{
    Ipp16u LevelScaleCoeffs[64];
};*/
#if 0
struct H264WholeQPDCLevelScale
{
    Ipp32s LevelScaleCoeffs[52]/*since we do not support 422 and 444*/;
};
#endif
struct H264WholeQPLevelScale4x4
{
    Ipp16s LevelScaleCoeffs[52]/*since we do not support 422 and 444*/[16];
};
struct H264WholeQPLevelScale8x8
{
    Ipp16s LevelScaleCoeffs[52]/*since we do not support 422 and 444*/[64];
};


// Sequence parameter set structure, corresponding to the H.264 bitstream definition.
struct H264SeqParamSet
{
    Ipp8u        profile_idc;                        // baseline, main, etc.
    Ipp8u        level_idc;
    Ipp8u        constrained_set0_flag;
    Ipp8u        constrained_set1_flag;
    Ipp8u        constrained_set2_flag;
    Ipp8u        constrained_set3_flag;
    Ipp8u        chroma_format_idc;
    Ipp8u        residual_colour_transform_flag;
    Ipp8u        bit_depth_luma;
    Ipp8u        bit_depth_chroma;
    Ipp8u        qpprime_y_zero_transform_bypass_flag;
    Ipp8u        type_of_scaling_list_used[8];
    Ipp8u        seq_scaling_matrix_present_flag;
    //Ipp8u        seq_scaling_list_present_flag[8];
    H264ScalingList4x4 ScalingLists4x4[6];
    H264ScalingList8x8 ScalingLists8x8[2];
    Ipp8u        gaps_in_frame_num_value_allowed_flag;
    Ipp8u        frame_cropping_flag;
    Ipp32u       frame_cropping_rect_left_offset;
    Ipp32u       frame_cropping_rect_right_offset;
    Ipp32u       frame_cropping_rect_top_offset;
    Ipp32u       frame_cropping_rect_bottom_offset;
    Ipp8u        more_than_one_slice_group_allowed_flag;
    Ipp8u        arbitrary_slice_order_allowed_flag;    // If zero, slice order in pictures must
                                                    // be in increasing MB address order.
    Ipp8u        redundant_pictures_allowed_flag;
    Ipp8u        seq_parameter_set_id;                // id of this sequence parameter set
    Ipp8u        log2_max_frame_num;                    // Number of bits to hold the frame_num
    Ipp8u        pic_order_cnt_type;                    // Picture order counting method

    Ipp8u        delta_pic_order_always_zero_flag;    // If zero, delta_pic_order_cnt fields are
                                                    // present in slice header.
    Ipp8u        frame_mbs_only_flag_kinoma_always_one;                // Nonzero indicates all pictures in sequence
                                                    // are coded as frames (not fields).
    Ipp8u        required_frame_num_update_behavior_flag;

    Ipp8u        mb_adaptive_frame_field_flag_unimplemented;        // Nonzero indicates frame/field switch
                                                    // at macroblock level
    Ipp8u        direct_8x8_inference_flag;            // Direct motion vector derivation method
    Ipp8u        vui_parameters_present_flag;        // Zero indicates default VUI parameters
    Ipp32u       log2_max_pic_order_cnt_lsb;            // Value of MaxPicOrderCntLsb.
    Ipp32s       offset_for_non_ref_pic;

    Ipp32s       offset_for_top_to_bottom_field;        // Expected pic order count difference from
                                                // top field to bottom field.

    Ipp32u       num_ref_frames_in_pic_order_cnt_cycle;
    Ipp32s       *poffset_for_ref_frame;                // pointer to array of stored frame offsets,
                                                    // length num_stored_frames_in_pic_order_cnt_cycle,
                                                    // for pic order cnt type 1
    Ipp32u       num_ref_frames;                        // total number of pics in decoded pic buffer
    Ipp32u       frame_width_in_mbs;
    Ipp32u       frame_height_in_mbs;

    // These fields are calculated from values above.  They are not written to the bitstream
    Ipp32u       MaxMbAddress;
    Ipp32u       MaxPicOrderCntLsb;
    // vui part
    Ipp8u        aspect_ratio_info_present_flag;
    Ipp8u        aspect_ratio_idc;
    Ipp16u       sar_width;
    Ipp16u       sar_height;
    Ipp8u        overscan_info_present_flag;
    Ipp8u        overscan_appropriate_flag;
    Ipp8u        video_signal_type_present_flag;
    Ipp8u        video_format;
    Ipp8u        video_full_range_flag;
    Ipp8u        colour_description_present_flag;
    Ipp8u        colour_primaries;
    Ipp8u        transfer_characteristics;
    Ipp8u        matrix_coefficients;
    Ipp8u        chroma_loc_info_present_flag;
    Ipp8u        chroma_sample_loc_type_top_field;
    Ipp8u        chroma_sample_loc_type_bottom_field;
    Ipp8u        timing_info_present_flag;
    Ipp32u       num_units_in_tick;
    Ipp32u       time_scale;
    Ipp8u        fixed_frame_rate_flag;
    Ipp8u        nal_hrd_parameters_present_flag;
    Ipp8u        vcl_hrd_parameters_present_flag;
    Ipp8u        low_delay_hrd_flag;
    Ipp8u        pic_struct_present_flag;
    Ipp8u        bitstream_restriction_flag;
    Ipp8u        motion_vectors_over_pic_boundaries_flag;
    Ipp8u        max_bytes_per_pic_denom;
    Ipp8u        max_bits_per_mb_denom;
    Ipp8u        log2_max_mv_length_horizontal;
    Ipp8u        log2_max_mv_length_vertical;
    Ipp8u        num_reorder_frames;
    Ipp8u        max_dec_frame_buffering;
    //hrd_parameters
    Ipp8u        cpb_cnt;
    Ipp8u        bit_rate_scale;
    Ipp8u        cpb_size_scale;
    Ipp32u        bit_rate_value[32];
    Ipp8u        cpb_size_value[32];
    Ipp8u        cbr_flag[32];
    Ipp8u        initial_cpb_removal_delay_length;
    Ipp8u        cpb_removal_delay_length;
    Ipp8u        dpb_output_delay_length;
    Ipp8u        time_offset_length;

};    // H264SeqParamSet

// Picture parameter set structure, corresponding to the H.264 bitstream definition.
struct H264PicParamSet
{
// Flexible macroblock order structure, defining the FMO map for a picture
// paramter set.
    struct SliceGroupInfoStruct
    {
        Ipp8u        slice_group_map_type;                // 0..6

        // The additional slice group data depends upon map type
        union
        {
            // type 0
            Ipp32u    run_length[MAX_NUM_SLICE_GROUPS];

            // type 2
            struct
            {
                Ipp32u top_left[MAX_NUM_SLICE_GROUPS-1];
                Ipp32u    bottom_right[MAX_NUM_SLICE_GROUPS-1];
            }t1;

            // types 3-5
            struct
            {
                Ipp8u    slice_group_change_direction_flag;
                Ipp32u    slice_group_change_rate;
            }t2;

            // type 6
            struct
            {
                Ipp32u pic_size_in_map_units;        // number of macroblocks if no field coding
                Ipp8u *pSliceGroupIDMap;            // Id for each slice group map unit
            }t3;
        };
    };    // SliceGroupInfoStruct

    struct FMOMapStruct
    {
        Ipp8u        mb_allocation_map_type;
    };    // FMOMapStruct

    Ipp16u       pic_parameter_set_id;            // of this picture parameter set
    Ipp8u        seq_parameter_set_id;            // of seq param set used for this pic param set
    Ipp8u        entropy_coding_mode;            // zero: CAVLC, else CABAC

    Ipp8u        pic_order_present_flag;            // Zero indicates only delta_pic_order_cnt[0] is
                                                // present in slice header; nonzero indicates
                                                // delta_pic_order_cnt[1] is also present.

    Ipp8u        weighted_pred_flag;                // Nonzero indicates weighted prediction applied to
                                                // P and SP slices
    Ipp8u        weighted_bipred_idc;            // 0: no weighted prediction in B slices
                                                // 1: explicit weighted prediction
                                                // 2: implicit weighted prediction
    Ipp8u        pic_init_qp;                    // default QP for I,P,B slices
    Ipp8u        pic_init_qs;                    // default QP for SP, SI slices

    Ipp8s        chroma_qp_index_offset[2];            // offset to add to QP for chroma

    Ipp8u        deblocking_filter_variables_present_flag;    // If nonzero, deblock filter params are
                                                // present in the slice header.
    Ipp8u        constrained_intra_pred_flag;    // Nonzero indicates constrained intra mode

    Ipp8u        redundant_pic_cnt_present_flag;    // Nonzero indicates presence of redundant_pic_cnt
                                                // in slice header
    Ipp32u        num_slice_groups;                // One: no FMO
    FMOMapStruct*pFMOMap;                        // pointer to FMO map for this pic param set
    Ipp32u        num_ref_idx_l0_active;            // num of ref pics in list 0 used to decode the picture
    Ipp32u        num_ref_idx_l1_active;            // num of ref pics in list 1 used to decode the picture
    Ipp8u         transform_8x8_mode_flag;
    Ipp8u         type_of_scaling_list_used[8];
    //Ipp8u        pic_scaling_matrix_present_flag;
    //Ipp8u        pic_scaling_list_present_flag[8];
    H264ScalingList4x4 ScalingLists4x4[6];
    H264ScalingList8x8 ScalingLists8x8[2];
    //H264LevelScale4x4 LevelScale4x4[6];
    //H264LevelScale8x8 LevelScale8x8[2];
/*
    Ipp8s         second_chroma_qp_index_offset;*/

#ifndef DROP_SLICE_GROUP
                                            // in slice header
    SliceGroupInfoStruct SliceGroupInfo;    // Used only when num_slice_groups > 1
#endif

    // Level Scale addition
    //H264WholeQPDCLevelScale         m_DCLevelScale[6];
    H264WholeQPLevelScale4x4        m_LevelScale4x4[6];
    H264WholeQPLevelScale8x8        m_LevelScale8x8[2];

};    // H264PicParamSet

struct RefPicListReorderInfo
{
    Ipp32u        num_entries;                // number of currently valid idc,value pairs
    Ipp8u         reordering_of_pic_nums_idc[MAX_NUM_REF_FRAMES];
    Ipp32u        reorder_value[MAX_NUM_REF_FRAMES];    // abs_diff_pic_num or long_term_pic_num
};

struct AdaptiveMarkingInfo
{
    Ipp32u        num_entries;                // number of currently valid mmco,value pairs
    Ipp8u         mmco[MAX_NUM_REF_FRAMES];    // memory management control operation id
    Ipp32u        value[MAX_NUM_REF_FRAMES*2];// operation-dependent data, max 2 per operation
};

struct PredWeightTable
{
    Ipp8u        luma_weight_flag;            // nonzero: luma weight and offset in bitstream
    Ipp8u        chroma_weight_flag;            // nonzero: chroma weight and offset in bitstream
    Ipp8s        luma_weight;                // luma weighting factor
    Ipp8s        luma_offset;                // luma weighting offset
    Ipp8s        chroma_weight[2];            // chroma weighting factor (Cb,Cr)
    Ipp8s        chroma_offset[2];            // chroma weighting offset (Cb,Cr)
};    // PredWeightTable

typedef Ipp32s H264DecoderMBAddr;

// Slice header structure, corresponding to the H.264 bitstream definition.
struct H264SliceHeader
{
    NAL_Unit_Type nal_unit_type;                                // (NAL_Unit_Type) specifies the type of RBSP data structure contained in the NAL unit as specified in Table 7-1 of h264 standart
    Ipp16u        pic_parameter_set_id;                // of pic param set used for this slice
    Ipp8u         field_pic_flag_kinoma_always_zero;                        // zero: frame picture, else field picture
    Ipp8u         MbaffFrameFlag_kinoma_always_zero;
    Ipp8u         bottom_field_flag_kinoma_always_zero;                    // zero: top field, else bottom field
    Ipp8u         direct_spatial_mv_pred_flag;        // zero: temporal direct, else spatial direct
    Ipp8u         num_ref_idx_active_override_flag;    // nonzero: use ref_idx_active from slice header
                                                // instead of those from pic param set
    Ipp8u         no_output_of_prior_pics_flag;        // nonzero: remove previously decoded pictures
                                                // from decoded picture buffer
    Ipp8u         long_term_reference_flag;            // How to set MaxLongTermFrameIdx
    Ipp8u         cabac_init_idc;                        // CABAC initialization table index (0..2)
    Ipp8u         adaptive_ref_pic_marking_mode_flag;    // Ref pic marking mode of current picture
    Ipp8s         slice_qp_delta;                        // to calculate default slice QP
    Ipp8u         sp_for_switch_flag;                    // SP slice decoding control
    Ipp8s         slice_qs_delta;                        // to calculate default SP,SI slice QS
    Ipp8u         disable_deblocking_filter_idc;        // deblock filter control, 0=filter all edges
    Ipp8s         slice_alpha_c0_offset;                // deblock filter c0, alpha table offset
    Ipp8s         slice_beta_offset;                    // deblock filter beta table offset
    H264DecoderMBAddr first_mb_in_slice;
    Ipp32u        frame_num;
    EnumSliceCodType slice_type;
    Ipp8u         idr_flag;
    Ipp8u         nal_ref_idc;
    Ipp32u        idr_pic_id;                            // ID of an IDR picture
    Ipp32u        pic_order_cnt_lsb;                    // picture order count (mod MaxPicOrderCntLsb)
    Ipp32s        delta_pic_order_cnt_bottom;            // Pic order count difference, top & bottom fields
    Ipp32u        difference_of_pic_nums;                // Ref pic memory mgmt
    Ipp32u        long_term_pic_num;                    // Ref pic memory mgmt
    Ipp32u        long_term_frame_idx;                // Ref pic memory mgmt
    Ipp32u        max_long_term_frame_idx;            // Ref pic memory mgmt
    Ipp32s        delta_pic_order_cnt[2];                // picture order count differences
    Ipp32u        redundant_pic_cnt;                    // for redundant slices
    Ipp32u        num_ref_idx_l0_active;                // num of ref pics in list 0 used to decode the slice,
                                                // see num_ref_idx_active_override_flag
    Ipp32u        num_ref_idx_l1_active;                // num of ref pics in list 1 used to decode the slice
                                                // see num_ref_idx_active_override_flag
    Ipp32u        slice_group_change_cycle;            // for FMO
    Ipp8u         luma_log2_weight_denom;                // luma weighting denominator
    Ipp8u         chroma_log2_weight_denom;            // chroma weighting denominator

}; // H264SliceHeader

struct H264LimitedSliceHeader
{
    EnumSliceCodType slice_type;                                // (EnumSliceCodType) slice type
    Ipp8u disable_deblocking_filter_idc;                        // (Ipp8u) deblock filter control, 0 = filter all edges
    Ipp8s slice_alpha_c0_offset;                                // (Ipp8s) deblock filter c0, alpha table offset
    Ipp8s slice_beta_offset;                                    // (Ipp8s) deblock filter beta table offset

}; // H264LimitedSliceHeader

struct H264SEIPayLoad
{
    Ipp32u payLoadType;
    Ipp32u payLoadSize;
    union SEIMessages
    {
        struct BufferingPeriod
        {
            Ipp32u initial_cbp_removal_delay[2][16];
            Ipp32u initial_cbp_removal_delay_offset[2][16];
        }buffering_period;

        struct PicTiming
        {
            Ipp32u cbp_removal_delay;
            Ipp32u dpb_ouput_delay;
            Ipp8u pic_struct;
            Ipp8u clock_timestamp_flag[16];
            struct ClockTimestamps
            {
                Ipp8u ct_type;
                Ipp8u nunit_field_based_flag;
                Ipp8u counting_type;
                Ipp8u full_timestamp_flag;
                Ipp8u discontinuity_flag;
                Ipp8u cnt_dropped_flag;
                Ipp8u n_frames;
                Ipp8u seconds_value;
                Ipp8u minutes_value;
                Ipp8u hours_value;
                Ipp8u time_offset;
            }clock_timestamps[16];
        }pic_timing;

        struct PanScanRect
        {
            Ipp8u pan_scan_rect_id;
            Ipp8u pan_scan_rect_cancel_flag;
            Ipp8u pan_scan_cnt;
            Ipp32u pan_scan_rect_left_offset[32];
            Ipp32u pan_scan_rect_right_offset[32];
            Ipp32u pan_scan_rect_top_offset[32];
            Ipp32u pan_scan_rect_bottom_offset[32];
            Ipp8u pan_scan_rect_repetition_period;
        }pan_scan_rect;

        struct RecoveryPoint
        {
            Ipp8u recovery_frame_cnt;
            Ipp8u exact_match_flag;
            Ipp8u broken_link_flag;
            Ipp8u changing_slice_group_idc;
        }recovery_point;

        struct DecRefPicMarkingRepetition
        {
            Ipp8u original_idr_flag;
            Ipp8u original_frame_num;
            Ipp8u original_field_pic_flag;
            Ipp8u original_bottom_field_flag;
        }dec_ref_pic_marking_repetition;

        struct SparePic
        {
            Ipp32u target_frame_num;
            Ipp8u  spare_field_flag;
            Ipp8u  target_bottom_field_flag;
            Ipp8u  num_spare_pics;
            Ipp8u  delta_spare_frame_num[16];
            Ipp8u  spare_bottom_field_flag[16];
            Ipp8u  spare_area_idc[16];
            Ipp8u  *spare_unit_flag[16];
            Ipp8u  *zero_run_length[16];
        }spare_pic;

        struct SceneInfo
        {
            Ipp8u scene_info_present_flag;
            Ipp8u scene_id;
            Ipp8u scene_transition_type;
            Ipp8u second_scene_id;
        }scene_info;

        struct SubSeqInfo
        {
            Ipp8u sub_seq_layer_num;
            Ipp8u sub_seq_id;
            Ipp8u first_ref_pic_flag;
            Ipp8u leading_non_ref_pic_flag;
            Ipp8u last_pic_flag;
            Ipp8u sub_seq_frame_num_flag;
            Ipp8u sub_seq_frame_num;
        }sub_seq_info;

        struct SubSeqLayerCharacteristics
        {
            Ipp8u num_sub_seq_layers;
            Ipp8u accurate_statistics_flag[16];
            Ipp16u average_bit_rate[16];
            Ipp16u average_frame_rate[16];
        }sub_seq_layer_characteristics;

        struct SubSeqCharacteristics
        {
            Ipp8u sub_seq_layer_num;
            Ipp8u sub_seq_id;
            Ipp8u duration_flag;
            Ipp8u sub_seq_duration;
            Ipp8u average_rate_flag;
            Ipp8u accurate_statistics_flag;
            Ipp16u average_bit_rate;
            Ipp16u average_frame_rate;
            Ipp8u num_referenced_subseqs;
            Ipp8u ref_sub_seq_layer_num[16];
            Ipp8u ref_sub_seq_id[16];
            Ipp8u ref_sub_seq_direction[16];
        }sub_seq_characteristics;

        struct FullFrameFreeze
        {
            Ipp32u full_frame_freeze_repetition_period;
        }full_frame_freeze;

        struct FullFrameSnapshot
        {
            Ipp8u snapshot_id;
        }full_frame_snapshot;

        struct ProgressiveRefinementSegmentStart
        {
            Ipp8u progressive_refinement_id;
            Ipp8u num_refinement_steps;
        }progressive_refinement_segment_start;

        struct MotionConstrainedSliceGroupSet
        {
            Ipp8u num_slice_groups_in_set;
            Ipp8u slice_group_id[8];
            Ipp8u exact_sample_value_match_flag;
            Ipp8u pan_scan_rect_flag;
            Ipp8u pan_scan_rect_id;
        }motion_constrained_slice_group_set;

        struct FilmGrainCharacteristics
        {
            Ipp8u film_grain_characteristics_cancel_flag;
            Ipp8u model_id;
            Ipp8u separate_colour_description_present_flag;
            Ipp8u film_grain_bit_depth_luma;
            Ipp8u film_grain_bit_depth_chroma;
            Ipp8u film_grain_full_range_flag;
            Ipp8u film_grain_colour_primaries;
            Ipp8u film_grain_transfer_characteristics;
            Ipp8u film_grain_matrix_coefficients;
            Ipp8u blending_mode_id;
            Ipp8u log2_scale_factor;
            Ipp8u comp_model_present_flag[3];
            Ipp8u num_intensity_intervals[3];
            Ipp8u num_model_values[3];
            Ipp8u intensity_interval_lower_bound[3][256];
            Ipp8u intensity_interval_upper_bound[3][256];
            Ipp8u comp_model_value[3][3][256];
            Ipp8u film_grain_characteristics_repetition_period;
        }film_grain_characteristics;

        struct DeblockingFilterDisplayPreference
        {
            Ipp8u deblocking_display_preference_cancel_flag;
            Ipp8u display_prior_to_deblocking_preferred_flag;
            Ipp8u dec_frame_buffering_constraint_flag;
            Ipp8u deblocking_display_preference_repetition_period;
        }deblocking_filter_display_preference;

        struct StereoVideoInfo
        {
            Ipp8u field_views_flag;
            Ipp8u top_field_is_left_view_flag;
            Ipp8u current_frame_is_left_view_flag;
            Ipp8u next_frame_is_second_view_flag;
            Ipp8u left_view_self_contained_flag;
            Ipp8u right_view_self_contained_flag;
        }stereo_video_info;

    }SEI_messages;
};
// This file defines some data structures and constants used by the decoder,
// that are also needed by other classes, such as post filters and
// error concealment.

//--- block types for CABAC ----
#define LUMA_16DC_CTX       0
#define LUMA_16AC_CTX       1
#define LUMA_8x8_CTX        2
#define LUMA_8x4_CTX        3
#define LUMA_4x8_CTX        4
#define LUMA_4x4_CTX        5
#define CHROMA_DC_CTX       6
#define CHROMA_AC_CTX       7
#define NUM_BLOCK_TYPES     8

#define INTERP_FACTOR 4
#define INTERP_SHIFT 2

// at picture edge, clip motion vectors to only this far beyond the edge,
// in pixel units.
#define D_MV_CLIP_LIMIT 19

// Direct motion vector scaling
#define TR_SHIFT    8
#define TR_RND        (1 << (TR_SHIFT - 1))


// Provide a way to get at the motion vectors for subblock 0 of a
// macroblock, given the macroblock's pel position in the luma plane.



#define            D_DIR_FWD 0
#define            D_DIR_BWD 1
#define            D_DIR_BIDIR 2
#define            D_DIR_DIRECT 3
#define            D_DIR_DIRECT_SPATIAL_FWD 4
#define            D_DIR_DIRECT_SPATIAL_BWD 5
#define            D_DIR_DIRECT_SPATIAL_BIDIR 6

// Warning: If these bit defines change, also need to change same
// defines  and related code in sresidual.s.
#define            D_CBP_LUMA_DC 0x00001
#define            D_CBP_LUMA_AC 0x1fffe

#define            D_CBP_CHROMA_DC 0x00001
#define            D_CBP_CHROMA_AC 0x0001e

#define            D_CBP_1ST_LUMA_AC_BITPOS 1
#define            D_CBP_1ST_CHROMA_DC_BITPOS 17
#define            D_CBP_1ST_CHROMA_AC_BITPOS 19

inline
Ipp32u CreateIPPCBPMask420(Ipp32u cbpU, Ipp32u cbpV)
{
    Ipp32u cbp4x4 = (((cbpU & D_CBP_CHROMA_DC) | ((cbpV & D_CBP_CHROMA_DC) << 1)) << D_CBP_1ST_CHROMA_DC_BITPOS) |
             ((cbpU & D_CBP_CHROMA_AC) << (D_CBP_1ST_CHROMA_AC_BITPOS - 1)) |
             ((cbpV & D_CBP_CHROMA_AC) << (D_CBP_1ST_CHROMA_AC_BITPOS + 4 - 1));

    return cbp4x4;
} // Ipp32u CreateIPPCBPMask420(Ipp32u nUCBP, Ipp32u nVCBP)

inline
Ipp64u CreateIPPCBPMask422(Ipp32u cbpU, Ipp32u cbpV)
{
    Ipp64u cbp4x4 = (((cbpU & D_CBP_CHROMA_DC) | ((cbpV & D_CBP_CHROMA_DC) << 1)) << D_CBP_1ST_CHROMA_DC_BITPOS) |
             (((Ipp64u)cbpU & D_CBP_CHROMA_AC) << (D_CBP_1ST_CHROMA_AC_BITPOS - 1)) |
             (((Ipp64u)cbpV & D_CBP_CHROMA_AC) << (D_CBP_1ST_CHROMA_AC_BITPOS + 8 - 1));

    return cbp4x4;
} // Ipp32u CreateIPPCBPMask422(Ipp32u nUCBP, Ipp32u nVCBP)

inline
Ipp64u CreateIPPCBPMask444(Ipp32u cbpU, Ipp32u cbpV)
{
    Ipp64u cbp4x4 = (((cbpU & D_CBP_CHROMA_DC) | ((cbpV & D_CBP_CHROMA_DC) << 1)) << D_CBP_1ST_CHROMA_DC_BITPOS) |
             (((Ipp64u)cbpU & D_CBP_CHROMA_AC) << (D_CBP_1ST_CHROMA_AC_BITPOS - 1)) |
             (((Ipp64u)cbpV & D_CBP_CHROMA_AC) << (D_CBP_1ST_CHROMA_AC_BITPOS + 16 - 1));

    return cbp4x4;
} // Ipp32u CreateIPPCBPMask444(Ipp32u nUCBP, Ipp32u nVCBP)

/*
#define            D_CBP_GET_LUMA_AC_BITS(cbp) (((cbp) & 0x1fffe)>>D_CBP_1ST_LUMA_AC_BITPOS)
//#define            D_CBP_CHROMA_DC ((Ipp64u) 0x0000000000180000)
//#define            D_CBP_CHROMA_AC ((Ipp64u) 0x0007fffffff80000)

#define            D_CBP_CHROMA_DC ((Ipp64u)(0x00000003)<<D_CBP_1ST_CHROMA_DC_BITPOS)
#define            D_CBP_CHROMA_AC ((Ipp64u)(0xffffffff)<<D_CBP_1ST_CHROMA_AC_BITPOS)

#define            D_CBP_1ST_CHROMA_DC_BITPOS_1 1
#define            D_CBP_1ST_CHROMA_AC_BITPOS_1 1

#define            D_CBP_CHROMA_DC_1 1
#define            D_CBP_CHROMA_AC_1 (0xffff<<D_CBP_1ST_LUMA_AC_BITPOS)
*/

#define BLOCK_IS_ON_LEFT_EDGE(x) (!((x)&3))
#define BLOCK_IS_ON_TOP_EDGE(x) ((x)<4)

#define CHROMA_BLOCK_IS_ON_LEFT_EDGE(x,c) (x_pos_value[c][x]==0)
#define CHROMA_BLOCK_IS_ON_TOP_EDGE(y,c) (y_pos_value[c][y]==0)

#define FIRST_DC_LUMA 0
#define FIRST_AC_LUMA 1
#define FIRST_DC_CHROMA 17
#define FIRST_AC_CHROMA 19
#define GetMBFieldDecodingFlag(x) ((x.mb_aux_fields)&1)
#define GetMBBottomFlag(x) ((x.mb_aux_fields&2)>>1)
#define GetMB8x8TSFlag(x) ((x.mb_aux_fields&4)>>2)

#define pGetMBFieldDecodingFlag(x) (((x)->mb_aux_fields)&1)
#define pGetMBBottomFlag(x) (((x)->mb_aux_fields&2)>>1)
#define pGetMB8x8TSFlag(x) (((x)->mb_aux_fields&4)>>2)

#define pSetMBFieldDecodingFlag(x,y)    \
    ((x->mb_aux_fields) &= 6);          \
    ((x->mb_aux_fields) |= (y))

#define SetMBFieldDecodingFlag(x,y)     \
    ((x.mb_aux_fields) &= 6);           \
    ((x.mb_aux_fields) |= (y))

#define pSetMB8x8TSFlag(x,y)            \
    ((x->mb_aux_fields) &= 3);          \
    ((x->mb_aux_fields) |= (y<<2))

#define SetMB8x8TSFlag(x,y)             \
    ((x.mb_aux_fields) &= 2);           \
    ((x.mb_aux_fields) |= (y<<2))

#define pSetPairMBFieldDecodingFlag(x1,x2,y)    \
    ((x1->mb_aux_fields) &= 6);                 \
    ((x2->mb_aux_fields) &= 6);                 \
    ((x1->mb_aux_fields) |= (y));               \
    ((x2->mb_aux_fields) |= (y))


#define SetPairMBFieldDecodingFlag(x1,x2,y)     \
    ((x1.mb_aux_fields) &= 6);                  \
    ((x2.mb_aux_fields) &= 6);                  \
    ((x1.mb_aux_fields) |= (y));                \
    ((x2.mb_aux_fields) |= (y))

///////////////// New structures
struct H264DecoderMotionVector
{
    Ipp16s  mvx;
    Ipp16s  mvy;
};//4bytes

struct H264DecoderMacroblockRefIdxs
{
    Ipp8s RefIdxs[16];                              // 16 bytes
};//16bytes

struct H264DecoderMacroblockMVFlags
{
    Ipp8s MVFlags[16];                              // 16 bytes
};//16bytes

struct H264DecoderMacroblockMVs
{
    H264DecoderMotionVector MotionVectors[16];                  // (H264DecoderMotionVector []) motion vectors for each block in macroblock

}; // 64 bytes

struct H264DecoderMacroblockCoeffsInfo
{
    Ipp8u numCoeff[48];                                         // (Ipp8u) number of coefficients in each block in macroblock

}; // 48 bytes

struct H264DecoderMacroblockGlobalInfo
{
    Ipp8u sbtype[4];                                            // (Ipp8u []) types of subblocks in macroblock
    Ipp16s slice_id;                                            // (Ipp16s) number of slice
    Ipp8u mbtype;                                               // (Ipp8u) type of macroblock
    Ipp8u mb_aux_fields;

}; // 8 bytes

struct H264DecoderMacroblockLocalInfo
{
    Ipp32u cbp4x4_luma;                                         // (Ipp32u) coded block pattern of luma blocks
    Ipp32u cbp4x4_chroma[2];                                    // (Ipp32u []) coded block patterns of chroma blocks
    Ipp8u sbdir[4];
    Ipp8u cbp;
    Ipp8u mbtypeBS;
    Ipp8u intra_chroma_mode;
    Ipp8s QP;
    Ipp8u spatial_prediction_dir;
    Ipp8s spatial_ref_idx_l0;
    Ipp8s spatial_ref_idx_l1;
    Ipp8u dummy;

}; // 24 bytes


struct H264DecoderBlockLocation
{
    Ipp32s mb_num;                                              // (Ipp32s) number of owning macroblock
    Ipp32s block_num;                                           // (Ipp32s) number of block

}; // 8 bytes

struct H264DecoderMacroblockNeighboursInfo
{
    Ipp32s mb_A;                                                // (Ipp32s) number of left macroblock
    Ipp32s mb_B;                                                // (Ipp32s) number of top macroblock
    Ipp32s mb_C;                                                // (Ipp32s) number of right-top macroblock
    Ipp32s mb_D;                                                // (Ipp32s) number of left-top macroblock

}; // 32 bytes

struct H264DecoderBlockNeighboursInfo
{
    H264DecoderBlockLocation mbs_left[4];
    H264DecoderBlockLocation mb_above;
    H264DecoderBlockLocation mb_above_right;
    H264DecoderBlockLocation mb_above_left;
    H264DecoderBlockLocation mbs_left_chroma[2][4];
    H264DecoderBlockLocation mb_above_chroma[2];
    H264DecoderBlockLocation dummy[2];

}; // 128 bytes

//this structure is present in each decoder frame
struct H264DecoderGlobalMacroblocksDescriptor
{
    H264DecoderMacroblockMVs *MV[2];//MotionVectors L0 L1
    H264DecoderMacroblockRefIdxs *RefIdxs[2];//Reference Indeces L0 l1
    H264DecoderMacroblockGlobalInfo *mbs;//macroblocks
};

//this structure is one(or couple) for all decoder
class H264DecoderLocalMacroblockDescriptor
{
public:
    // Default constructor
    H264DecoderLocalMacroblockDescriptor(void);
    // Destructor
    ~H264DecoderLocalMacroblockDescriptor(void);

    // Allocate decoding data
    bool Allocate(Ipp32s iMBCount);

    H264DecoderMacroblockMVs *(MVDeltas[2]);                    // (H264DecoderMacroblockMVs * ([])) motionVectors Deltas L0 and L1
    H264DecoderMacroblockMVFlags *(MVFlags[2]);                 // (H264DecoderMacroblockMVFlags * ([])) motionVectors Flags L0 and L1
    H264DecoderMacroblockCoeffsInfo *MacroblockCoeffsInfo;      // (H264DecoderMacroblockCoeffsInfo *) info about num_coeffs in each block in the current  picture
    H264DecoderMacroblockLocalInfo *mbs;                        // (H264DecoderMacroblockLocalInfo *) reconstuction info
    H264DecoderMBAddr *active_next_mb_table;                    // (H264DecoderMBAddr *) current "next addres" table

    // Assignment operator
    H264DecoderLocalMacroblockDescriptor &operator = (H264DecoderLocalMacroblockDescriptor &);

protected:
    // Release object
    void Release(void);

    Ipp8u *m_pAllocated;                                        // (Ipp8u *) pointer to allocated memory
    size_t m_nAllocatedSize;                                    // (size_t) size of allocated memory
};

struct H264DecoderCurrentMacroblockDescriptor
{
    H264DecoderMacroblockMVs *MVs[4];//MV L0,L1, MVDeltas 0,1
    H264DecoderMacroblockMVFlags *MVFlags[2];//MVFlags L0, L1
    H264DecoderMacroblockRefIdxs *RefIdxs[2];//RefIdx L0, L1
    H264DecoderMacroblockCoeffsInfo *MacroblockCoeffsInfo;
    H264DecoderMacroblockNeighboursInfo CurrentMacroblockNeighbours;//mb neighboring info
    H264DecoderBlockNeighboursInfo CurrentBlockNeighbours;//block neighboring info (if mbaff turned off remained static)
    H264DecoderMacroblockGlobalInfo *GlobalMacroblockInfo;
    H264DecoderMacroblockGlobalInfo *GlobalMacroblockPairInfo;
    H264DecoderMacroblockLocalInfo *LocalMacroblockInfo;
    H264DecoderMacroblockLocalInfo *LocalMacroblockPairInfo;
};


#if defined(ARM) || defined(_ARM_)
#define DEC_NUM_ALLOC_REF_FRAMES 2
#else
#define DEC_NUM_ALLOC_REF_FRAMES 2
#endif
    // Number of reference frames initially allocated by the decoder.
    // Normally 2, significantly increased for ARM for PVP player. Decrease
    // back to 2 for ARM when building xsavcdec player to free up memory
    // for files for performance and regression testing.

} // end namespace UMC

#pragma pack()

#endif // __UMC_H264_DEC_DEFS_DEC_H__
