/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//  Description:    MPEG-4 header.
//
*/


#ifndef _MP4_H_
#define _MP4_H_

#ifdef _OPENMP
#define _OMP_KARABAS
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ippdefs.h"
#include "ippcore.h"
#include "ipps.h"
#include "ippi.h"
#include "ippvc.h"
//#include "ippcc.h"
#include "vm_debug.h"

#if ! TARGET_OS_LINUX
//#pragma warning(disable : 4710) // function not inlined
//#pragma warning(disable : 4514) // unreferenced inline function has been removed CL
//#pragma warning(disable : 4100) // unreferenced formal parameter CL
#endif

#ifdef __cplusplus
extern "C" {
#endif

//#define MP4_USE_INLINE_BITS_FUNC
#define USE_NOTCODED_STATE

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
    #define __INLINE static __inline
#elif defined( __GNUC__ )
    #define __INLINE static __inline__
#else
    #define __INLINE static
#endif


#if defined(__INTEL_COMPILER) && !defined(_WIN32_WCE)
    #define __ALIGN16(type, name, size) \
        __declspec (align(16)) type name[size]
#else
    #if defined(_WIN64) || defined(WIN64) || defined(LINUX64) || defined(__LP64__)
        #define __ALIGN16(type, name, size) \
            Ipp8u _a16_##name[(size)*sizeof(type)+15]; type *name = (type*)(((Ipp64s)(_a16_##name) + 15) & ~15)
    #else
        #undef __ALIGN16
		#if TARGET_OS_ANDROID
			#define __ALIGN16(type, name, size)		type name[size] __attribute__ ((aligned (16)))
		#else
	        #define __ALIGN16(type, name, size) \
   	         Ipp8u _a16_##name[(size)*sizeof(type)+15]; type *name = (type*)(((Ipp32s)(_a16_##name) + 15) & ~15)
		#endif
    #endif
#endif


#define mp4_CLIP(x, min, max) if ((x) < (min)) (x) = (min); else if ((x) > (max)) (x) = (max)
#define mp4_CLIPR(x, max) if ((x) > (max)) (x) = (max)
#define mp4_SWAP(type, x, y) {type t = (x); (x) = (y); (y) = t;}
#define mp4_ABS(a) ((a) >= 0 ? (a) : -(a))


/* Timer Info */
#if !defined(__SYMBIAN32__) && (defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64))

    #include <windows.h>

    typedef struct _mp4_Timer {
        LARGE_INTEGER  count;
        LARGE_INTEGER  start;
        LARGE_INTEGER  stop;
        int            calls;
    } mp4_Timer;

    __INLINE void mp4_TimerStart(mp4_Timer *t)
    {
        QueryPerformanceCounter(&t->start);
    }

    __INLINE void mp4_TimerStop(mp4_Timer *t)
    {
        QueryPerformanceCounter(&t->stop);
        t->count.QuadPart += t->stop.QuadPart - t->start.QuadPart;
        t->calls ++;
    }

    #define TIMER_FREQ_TYPE LARGE_INTEGER

    __INLINE void mp4_GetTimerFreq(TIMER_FREQ_TYPE *f)
    {
        QueryPerformanceFrequency(f);
    }

    __INLINE double mp4_GetTimerSec(mp4_Timer *t, TIMER_FREQ_TYPE f)
    {
        return (double)t->count.QuadPart / (double)f.QuadPart;
    }

#else  // LINUX

    #include <time.h>

    typedef struct _mp4_Timer {
        clock_t  count;
        clock_t  start;
        clock_t  stop;
        int      calls;
    } mp4_Timer;

    __INLINE void mp4_TimerStart(mp4_Timer *t)
    {
        t->start = clock();
    }

    __INLINE void mp4_TimerStop(mp4_Timer *t)
    {
        t->stop = clock();
        t->count += t->stop - t->start;
        t->calls ++;
    }

    #define TIMER_FREQ_TYPE int

    __INLINE void mp4_GetTimerFreq(TIMER_FREQ_TYPE *f)
    {
        *f = CLOCKS_PER_SEC;
    }

    __INLINE double mp4_GetTimerSec(mp4_Timer *t, TIMER_FREQ_TYPE f)
    {
        return (double)t->count / (double)f;
    }

#endif

/* number of exterior MB */
#define MP4_NUM_EXT_MB 1

/* Statistic Info */
typedef struct _mp4_Statistic {
    // VideoObjectLayer Info
    int         nVOP;
    int         nVOP_I;
    int         nVOP_P;
    int         nVOP_B;
    int         nVOP_S;
    int         nMB;
    int         nMB_INTER;
    int         nMB_INTER_Q;
    int         nMB_INTRA;
    int         nMB_INTRA_Q;
    int         nMB_INTER4V;
    int         nMB_DIRECT;
    int         nMB_INTERPOLATE;
    int         nMB_BACKWARD;
    int         nMB_FORWARD;
    int         nMB_NOTCODED;
    int         nB_INTRA_DC;
    int         nB_INTRA_AC;
    int         nB_INTER_C;
    int         nB_INTER_NC;
    // app Timing Info
    mp4_Timer   time_DecodeShow;    // decode + draw + file reading
    mp4_Timer   time_Decode;        // decode + file reading
    mp4_Timer   time_DecodeOnly;    // decode only
} mp4_Statistic;


__INLINE void mp4_StatisticInc(int *s)
{
    *s = (*s) + 1;
}

// when using Full Statistic, FPS is less
#ifdef MP4_FULL_STAT
#define mp4_StatisticInc_(s) mp4_StatisticInc(s)
#define mp4_TimerStart_(t) mp4_TimerStart(t)
#define mp4_TimerStop_(t) mp4_TimerStop(t)
#else
#define mp4_StatisticInc_(s)
#define mp4_TimerStart_(t)
#define mp4_TimerStop_(t)
#endif


/* status codes */
typedef enum {
    MP4_STATUS_OK           =  0,   // no error
    MP4_STATUS_NO_MEM       = -1,   // out of memory
    MP4_STATUS_FILE_ERROR   = -2,   // file error
    MP4_STATUS_NOTSUPPORT   = -3,   // not supported mode
    MP4_STATUS_PARSE_ERROR  = -4,   // fail in parse MPEG-4 stream
    MP4_STATUS_ERROR        = -5    // unknown/unspecified error
} mp4_Status;


/* MPEG-4 start code values */
// ISO/IEC 14496-2: table 6-3
enum {
    MP4_VIDEO_OBJECT_MIN_SC       = 0x00,
    MP4_VIDEO_OBJECT_MAX_SC       = 0x1F,
    MP4_VIDEO_OBJECT_LAYER_MIN_SC = 0x20,
    MP4_VIDEO_OBJECT_LAYER_MAX_SC = 0x2F,
    MP4_FGS_BP_MIN_SC             = 0x40,
    MP4_FGS_BP_MAX_SC             = 0x5F,
    MP4_VISUAL_OBJECT_SEQUENCE_SC = 0xB0,
    MP4_VISUAL_OBJECT_SEQUENCE_EC = 0xB1,
    MP4_USER_DATA_SC              = 0xB2,
    MP4_GROUP_OF_VOP_SC           = 0xB3,
    MP4_VIDEO_SESSION_ERROR_SC    = 0xB4,
    MP4_VISUAL_OBJECT_SC          = 0xB5,
    MP4_VIDEO_OBJECT_PLANE_SC     = 0xB6,
    MP4_SLICE_SC                  = 0xB7,
    MP4_EXTENSION_SC              = 0xB8,
    MP4_FGS_VOP_SC                = 0xB9,
    MP4_FBA_OBJECT_SC             = 0xBA,
    MP4_FBA_OBJECT_PLANE_SC       = 0xBB,
    MP4_MESH_OBJECT_SC            = 0xBC,
    MP4_MESH_OBJECT_PLANE_SC      = 0xBD,
    MP4_STILL_TEXTURE_OBJECT_SC   = 0xBE,
    MP4_TEXTURE_SPATIAL_LAYER_SC  = 0xBF,
    MP4_TEXTURE_SNR_LAYER_SC      = 0xC0,
    MP4_TEXTURE_TILE_SC           = 0xC1,
    MP4_TEXTURE_SHAPE_LAYER_SC    = 0xC2,
    MP4_STUFFING_SC               = 0xC3
};

/* MPEG-4 code values */
// ISO/IEC 14496-2:2004 table 6-6
enum {
    MP4_VISUAL_OBJECT_TYPE_VIDEO     = 1,
    MP4_VISUAL_OBJECT_TYPE_TEXTURE   = 2,
    MP4_VISUAL_OBJECT_TYPE_MESH      = 3,
    MP4_VISUAL_OBJECT_TYPE_FBA       = 4,
    MP4_VISUAL_OBJECT_TYPE_3DMESH    = 5
};
// ISO/IEC 14496-2:2004 table 6-7
enum {
    MP4_VIDEO_FORMAT_COMPONENT      = 0,
    MP4_VIDEO_FORMAT_PAL            = 1,
    MP4_VIDEO_FORMAT_NTSC           = 2,
    MP4_VIDEO_FORMAT_SECAM          = 3,
    MP4_VIDEO_FORMAT_MAC            = 4,
    MP4_VIDEO_FORMAT_UNSPECIFIED    = 5
};
// ISO/IEC 14496-2:2004 table 6-8..10
enum {
    MP4_VIDEO_COLORS_FORBIDDEN         = 0,
    MP4_VIDEO_COLORS_ITU_R_BT_709      = 1,
    MP4_VIDEO_COLORS_UNSPECIFIED       = 2,
    MP4_VIDEO_COLORS_RESERVED          = 3,
    MP4_VIDEO_COLORS_ITU_R_BT_470_2_M  = 4,
    MP4_VIDEO_COLORS_ITU_R_BT_470_2_BG = 5,
    MP4_VIDEO_COLORS_SMPTE_170M        = 6,
    MP4_VIDEO_COLORS_SMPTE_240M        = 7,
    MP4_VIDEO_COLORS_GENERIC_FILM      = 8
};
// ISO/IEC 14496-2:2004 table 6-11
enum {
    MP4_VIDEO_OBJECT_TYPE_SIMPLE                     = 1,
    MP4_VIDEO_OBJECT_TYPE_SIMPLE_SCALABLE            = 2,
    MP4_VIDEO_OBJECT_TYPE_CORE                       = 3,
    MP4_VIDEO_OBJECT_TYPE_MAIN                       = 4,
    MP4_VIDEO_OBJECT_TYPE_NBIT                       = 5,
    MP4_VIDEO_OBJECT_TYPE_2DTEXTURE                  = 6,
    MP4_VIDEO_OBJECT_TYPE_2DMESH                     = 7,
    MP4_VIDEO_OBJECT_TYPE_SIMPLE_FACE                = 8,
    MP4_VIDEO_OBJECT_TYPE_STILL_SCALABLE_TEXTURE     = 9,
    MP4_VIDEO_OBJECT_TYPE_ADVANCED_REAL_TIME_SIMPLE  = 10,
    MP4_VIDEO_OBJECT_TYPE_CORE_SCALABLE              = 11,
    MP4_VIDEO_OBJECT_TYPE_ADVANCED_CODING_EFFICIENCY = 12,
    MP4_VIDEO_OBJECT_TYPE_ADVANCED_SCALABLE_TEXTURE  = 13,
    MP4_VIDEO_OBJECT_TYPE_SIMPLE_FBA                 = 14,
    MP4_VIDEO_OBJECT_TYPE_SIMPLE_STUDIO              = 15,
    MP4_VIDEO_OBJECT_TYPE_CORE_STUDIO                = 16,
    MP4_VIDEO_OBJECT_TYPE_ADVANCED_SIMPLE            = 17,
    MP4_VIDEO_OBJECT_TYPE_FINE_GRANULARITY_SCALABLE  = 18
};
// ISO/IEC 14496-2:2004 table 6.17 (maximum defined video_object_layer_shape_extension)
#define MP4_SHAPE_EXT_NUM 13
// ISO/IEC 14496-2:2004 table 6-14
enum {
    MP4_ASPECT_RATIO_FORBIDDEN  = 0,
    MP4_ASPECT_RATIO_1_1        = 1,
    MP4_ASPECT_RATIO_12_11      = 2,
    MP4_ASPECT_RATIO_10_11      = 3,
    MP4_ASPECT_RATIO_16_11      = 4,
    MP4_ASPECT_RATIO_40_33      = 5,
    MP4_ASPECT_RATIO_EXTPAR     = 15
};
// ISO/IEC 14496-2:2004 table 6-15
#define MP4_CHROMA_FORMAT_420    1
// ISO/IEC 14496-2:2004 table 6-16
enum {
    MP4_SHAPE_TYPE_RECTANGULAR  = 0,
    MP4_SHAPE_TYPE_BINARY       = 1,
    MP4_SHAPE_TYPE_BINARYONLY   = 2,
    MP4_SHAPE_TYPE_GRAYSCALE    = 3
};
// ISO/IEC 14496-2:2004 table 6-19
#define MP4_SPRITE_STATIC   1
#define MP4_SPRITE_GMC      2
// ISO/IEC 14496-2:2004 table 6-24
enum {
    MP4_VOP_TYPE_I  = 0,
    MP4_VOP_TYPE_P  = 1,
    MP4_VOP_TYPE_B  = 2,
    MP4_VOP_TYPE_S  = 3
};
// ISO/IEC 14496-2:2004 table 6-26
enum {
    MP4_SPRITE_TRANSMIT_MODE_STOP   = 0,
    MP4_SPRITE_TRANSMIT_MODE_PIECE  = 1,
    MP4_SPRITE_TRANSMIT_MODE_UPDATE = 2,
    MP4_SPRITE_TRANSMIT_MODE_PAUSE  = 3
};
// ISO/IEC 14496-2:2004 table 7-3
enum {
    MP4_BAB_TYPE_MVDSZ_NOUPDATE  = 0,
    MP4_BAB_TYPE_MVDSNZ_NOUPDATE = 1,
    MP4_BAB_TYPE_TRANSPARENT     = 2,
    MP4_BAB_TYPE_OPAQUE          = 3,
    MP4_BAB_TYPE_INTRACAE        = 4,
    MP4_BAB_TYPE_MVDSZ_INTERCAE  = 5,
    MP4_BAB_TYPE_MVDSNZ_INTERCAE = 6
};

#define MP4_DC_MARKER  0x6B001 // 110 1011 0000 0000 0001
#define MP4_MV_MARKER  0x1F001 //   1 1111 0000 0000 0001


// ISO/IEC 14496-2:2004 table G.1
enum {
    MP4_SIMPLE_PROFILE_LEVEL_1                     = 0x01,
    MP4_SIMPLE_PROFILE_LEVEL_2                     = 0x02,
    MP4_SIMPLE_PROFILE_LEVEL_3                     = 0x03,
    MP4_SIMPLE_PROFILE_LEVEL_0                     = 0x08,
    MP4_CORE_PROFILE_LEVEL_1                       = 0x21,
    MP4_CORE_PROFILE_LEVEL_2                       = 0x22,
    MP4_MAIN_PROFILE_LEVEL_2                       = 0x32,
    MP4_MAIN_PROFILE_LEVEL_3                       = 0x33,
    MP4_MAIN_PROFILE_LEVEL_4                       = 0x34,
    MP4_ADVANCED_REAL_TIME_SIMPLE_PROFILE_LEVEL_1  = 0x91,
    MP4_ADVANCED_REAL_TIME_SIMPLE_PROFILE_LEVEL_2  = 0x92,
    MP4_ADVANCED_REAL_TIME_SIMPLE_PROFILE_LEVEL_3  = 0x93,
    MP4_ADVANCED_REAL_TIME_SIMPLE_PROFILE_LEVEL_4  = 0x94,
    MP4_ADVANCED_CODING_EFFICIENCY_PROFILE_LEVEL_1 = 0xB1,
    MP4_ADVANCED_CODING_EFFICIENCY_PROFILE_LEVEL_2 = 0xB2,
    MP4_ADVANCED_CODING_EFFICIENCY_PROFILE_LEVEL_3 = 0xB3,
    MP4_ADVANCED_CODING_EFFICIENCY_PROFILE_LEVEL_4 = 0xB4,
    MP4_ADVANCED_CORE_PROFILE_LEVEL_1              = 0xC1,
    MP4_ADVANCED_CORE_PROFILE_LEVEL_2              = 0xC2,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_0            = 0xF0,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_1            = 0xF1,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_2            = 0xF2,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_3            = 0xF3,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_4            = 0xF4,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_5            = 0xF5,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_3B           = 0xF7
};

/* Frame Info */
typedef struct _mp4_Frame {
    Ipp8u*      apY;        // allocated with border
    Ipp8u*      apCb;       // allocated with border
    Ipp8u*      apCr;       // allocated with border
    int         stepY;
    int         stepCr;
    int         stepCb;
    Ipp8u*      pY;         // real pointer
    Ipp8u*      pCb;        // real pointer
    Ipp8u*      pCr;        // real pointer
    int         type;
    Ipp64s      time;
    int         mbPerRow;   // info for realloc VOP with Shape
    int         mbPerCol;
    Ipp8u*      apB;        // for binary mask
    Ipp8u*      pB;
    Ipp8u*      apA[3];     // for aux components
    Ipp8u*      pA[3];
	//***
	int			droppable;
} mp4_Frame;


/* Block Info for Intra Prediction */
typedef struct _mp4_IntraPredBlock {
    struct _mp4_IntraPredBlock  *predA;
    struct _mp4_IntraPredBlock  *predB;
    struct _mp4_IntraPredBlock  *predC;
    Ipp16s      dct_acA[8];
    Ipp16s      dct_acC[8];
    Ipp16s      dct_dc;
} mp4_IntraPredBlock;


/* Buffer for Intra Prediction */
typedef struct _mp4_IntraPredBuff {
    Ipp8u               *quant;     // quant buffer;
    mp4_IntraPredBlock  dcB[6];     // blocks for Left-Top DC only
    mp4_IntraPredBlock  *block;
} mp4_IntraPredBuff;


/* MacroBlock Info Data Partitioned mode */
typedef struct _mp4_DataPartMacroBlock {
    Ipp16s          dct_dc[6];
    Ipp8u           type;
    Ipp8u           not_coded;
    Ipp8u           mcsel;
    Ipp8u           ac_pred_flag;
    Ipp8u           pat;
    Ipp8u           quant;
} mp4_DataPartMacroBlock;


/* MacroBlock Info for Motion */
typedef struct _mp4_MacroBlock {
    IppMotionVector mv[4];
    Ipp8u        validPred;     // for MV pred, OBMC
    Ipp8u        type;          // for OBMC, BVOP
    Ipp8u        not_coded;     // for OBMC, BVOP
    Ipp8u        field_info;    // for Interlaced BVOP Direct mode
} mp4_MacroBlock;


/* ShapeInfo Info */
typedef struct _mp4_ShapeInfo {
    Ipp8u           bab_type;
    Ipp8u           opaque;
    IppMotionVector mvs;
    //    int         coda_i;
    //    int         ac_pred_flag_alpha;
    //    int         cbpa;
    //    int         coda_pb;
} mp4_ShapeInfo;


/* Group Of Video Object Plane Info */
typedef struct _mp4_GroupOfVideoObjectPlane {
    Ipp64s      time_code;
    int         closed_gov;
    int         broken_link;
} mp4_GroupOfVideoObjectPlane;


/* Video Object Plane Info */
typedef struct _mp4_VideoObjectPlane {
    int         coding_type;
    int         modulo_time_base;
    int         time_increment;
    int         coded;
    int         id;                             // verid != 1 (newpred)
    int         id_for_prediction_indication;   // verid != 1 (newpred)
    int         id_for_prediction;              // verid != 1 (newpred)
    int         rounding_type;
    int         reduced_resolution;             // verid != 1
    int         vop_width;
    int         vop_height;
    int         vop_horizontal_mc_spatial_ref;
    int         vop_vertical_mc_spatial_ref;
    int         background_composition;
    int         change_conv_ratio_disable;
    int         vop_constant_alpha;
    int         vop_constant_alpha_value;
    int         intra_dc_vlc_thr;
    int         top_field_first;
    int         alternate_vertical_scan_flag;
    int         sprite_transmit_mode;
    int         warping_mv_code_du[4];
    int         warping_mv_code_dv[4];
    int         brightness_change_factor;
    int         quant;
    int         alpha_quant[3];
    int         fcode_forward;
    int         fcode_backward;
    int         shape_coding_type;
    int         load_backward_shape;
    int         ref_select_code;
    int         dx;
    int         dy;
	//***
	int			droppable;
} mp4_VideoObjectPlane;


/* mp4_ComplexityEstimation Info */
typedef struct _mp4_ComplexityEstimation {
    int         estimation_method;
    int         shape_complexity_estimation_disable;
    int         opaque;
    int         transparent;
    int         intra_cae;
    int         inter_cae;
    int         no_update;
    int         upsampling;
    int         texture_complexity_estimation_set_1_disable;
    int         intra_blocks;
    int         inter_blocks;
    int         inter4v_blocks;
    int         not_coded_blocks;
    int         texture_complexity_estimation_set_2_disable;
    int         dct_coefs;
    int         dct_lines;
    int         vlc_symbols;
    int         vlc_bits;
    int         motion_compensation_complexity_disable;
    int         apm;
    int         npm;
    int         interpolate_mc_q;
    int         forw_back_mc_q;
    int         halfpel2;
    int         halfpel4;
    int         version2_complexity_estimation_disable;     // verid != 1
    int         sadct;                                      // verid != 1
    int         quarterpel;                                 // verid != 1
    int         dcecs_opaque;
    int         dcecs_transparent;
    int         dcecs_intra_cae;
    int         dcecs_inter_cae;
    int         dcecs_no_update;
    int         dcecs_upsampling;
    int         dcecs_intra_blocks;
    int         dcecs_inter_blocks;
    int         dcecs_inter4v_blocks;
    int         dcecs_not_coded_blocks;
    int         dcecs_dct_coefs;
    int         dcecs_dct_lines;
    int         dcecs_vlc_symbols;
    int         dcecs_vlc_bits;
    int         dcecs_apm;
    int         dcecs_npm;
    int         dcecs_interpolate_mc_q;
    int         dcecs_forw_back_mc_q;
    int         dcecs_halfpel2;
    int         dcecs_halfpel4;
    int         dcecs_sadct;                                // verid != 1
    int         dcecs_quarterpel;                           // verid != 1
} mp4_ComplexityEstimation;


/* mp4_Scalability Info */
typedef struct _mp4_ScalabilityParameters {
    int         dummy;
} mp4_ScalabilityParameters;


/* VOLControlParameters Info */
typedef struct _mp4_VOLControlParameters {
    int         chroma_format;
    int         low_delay;
    int         vbv_parameters;
    int         bit_rate;
    int         vbv_buffer_size;
    int         vbv_occupancy;
} mp4_VOLControlParameters;


/* Video Object Plane with short header Info */
typedef struct _mp4_VideoObjectPlaneH263 {
    int         temporal_reference;
    int         split_screen_indicator;
    int         document_camera_indicator;
    int         full_picture_freeze_release;
    int         source_format;
    int         picture_coding_type;
    int         vop_quant;
    int         gob_number;
    int         num_gobs_in_vop;
    int         num_macroblocks_in_gob;
    int         gob_header_empty;
    int         gob_frame_id;
    int         quant_scale;
#ifdef _OMP_KARABAS
    int         nmb;
    int         frGOB;
#endif // _OMP_KARABAS
} mp4_VideoObjectPlaneH263;


/* Video Object Info */
typedef struct _mp4_VideoObject {
// iso part
    int                         id;
    int                         short_video_header;
    int                         random_accessible_vol;
    int                         type_indication;
    int                         is_identifier;
    int                         verid;
    int                         priority;
    int                         aspect_ratio_info;
    int                         aspect_ratio_info_par_width;
    int                         aspect_ratio_info_par_height;
    int                         is_vol_control_parameters;
    mp4_VOLControlParameters    VOLControlParameters;
    int                         shape;
    int                         shape_extension;                // verid != 1
    int                         vop_time_increment_resolution;
    int                         vop_time_increment_resolution_bits;
    int                         fixed_vop_rate;
    int                         fixed_vop_time_increment;
    int                         width;
    int                         height;
    int                         interlaced;
    int                         obmc_disable;
    int                         sprite_enable;                  // if verid != 1 (2 bit GMC is added)
    int                         sprite_width;
    int                         sprite_height;
    int                         sprite_left_coordinate;
    int                         sprite_top_coordinate;
    int                         sprite_warping_points;
    int                         sprite_warping_accuracy;
    int                         sprite_brightness_change;
    int                         low_latency_sprite_enable;
    int                         sadct_disable;                  // verid != 1
    int                         not_8_bit;
    int                         quant_precision;
    int                         bits_per_pixel;
    int                         no_gray_quant_update;
    int                         composition_method;
    int                         linear_composition;
    int                         quant_type;
    int                         load_intra_quant_mat;
    Ipp8u                       intra_quant_mat[64];
    int                         load_nonintra_quant_mat;
    Ipp8u                       nonintra_quant_mat[64];
    int                         load_intra_quant_mat_grayscale[3];
    Ipp8u                       intra_quant_mat_grayscale[3][64];
    int                         load_nonintra_quant_mat_grayscale[3];
    Ipp8u                       nonintra_quant_mat_grayscale[3][64];
    int                         quarter_sample;                 // verid != 1
    int                         complexity_estimation_disable;
    mp4_ComplexityEstimation    ComplexityEstimation;
    int                         resync_marker_disable;
    int                         data_partitioned;
    int                         reversible_vlc;
    int                         newpred_enable;                 // verid != 1
    int                         requested_upstream_message_type;// verid != 1
    int                         newpred_segment_type;           // verid != 1
    int                         reduced_resolution_vop_enable;  // verid != 1
    int                         scalability;
    mp4_ScalabilityParameters   ScalabilityParameters;
    mp4_GroupOfVideoObjectPlane GroupOfVideoObjectPlane;
    mp4_VideoObjectPlane        VideoObjectPlane;
    mp4_VideoObjectPlaneH263    VideoObjectPlaneH263;
// app part
    int                         VOPindex;
    int                         MacroBlockPerRow;
    int                         MacroBlockPerCol;
    int                         MacroBlockPerVOP;
    int                         mbns; // num bits for MacroBlockPerVOP
    mp4_MacroBlock*             MBinfo;
    mp4_IntraPredBuff           IntraPredBuff;
    mp4_DataPartMacroBlock*     DataPartBuff;
    IppiQuantInvIntraSpec_MPEG4*  QuantInvIntraSpec;
    IppiQuantInvInterSpec_MPEG4*  QuantInvInterSpec;
    IppiWarpSpec_MPEG4*         WarpSpec;
    // for B-VOP
    int                         prevPlaneIsB;
    // for interlaced B-VOP direct mode
    int                         Tframe;
    IppMotionVector*            FieldMV;
    // for B-VOP direct mode
    int                         TRB, TRD;
    // time increment of past and future VOP for B-VOP
    Ipp64s                      rTime, nTime;
    // VOP global time
    Ipp64s                      vop_sync_time, vop_sync_time_b;
    // Shape Info
    mp4_ShapeInfo              *ShapeInfo;
#ifdef USE_NOTCODED_STATE
    // not_coded MB state
    Ipp8u*                      ncState;
    int                         ncStateCleared;
#endif
//***
	int							h263_flv;
	int							h263_flv_unlimited_UMV;
	int							h263_plus;

} mp4_VideoObject;

/* StillTexture Object Info */
typedef struct _mp4_StillTextureObject {
    int     dummy;
} mp4_StillTextureObject;

/* Mesh Object Info */
typedef struct _mp4_MeshObject {
    int     dummy;
} mp4_MeshObject;

/* Face Object Info */
typedef struct _mp4_FaceObject {
    int     dummy;
} mp4_FaceObject;

/* video_signal_type Info */
typedef struct _mp4_VideoSignalType {
    int     is_video_signal_type;
    int     video_format;
    int     video_range;
    int     is_colour_description;
    int     colour_primaries;
    int     transfer_characteristics;
    int     matrix_coefficients;
} mp4_VideoSignalType;


/* Visual Object Info */
typedef struct _mp4_VisualObject {
    int                     is_identifier;
    int                     verid;
    int                     priority;
    int                     type;
    mp4_VideoSignalType     VideoSignalType;
    mp4_VideoObject         VideoObject;
    mp4_StillTextureObject  StillTextureObject;
    mp4_MeshObject          MeshObject;
    mp4_FaceObject          FaceObject;
    mp4_Frame               sFrame;  // static sprite
    mp4_Frame               cFrame;  // current
    mp4_Frame               rFrame;  // reference in past
    mp4_Frame               nFrame;  // reference in future
    mp4_Frame              *vFrame;  // frame for display
    int                     frameCount;
    int                     frameInterval;
    int                     frameScale;
    mp4_Statistic           Statistic;
} mp4_VisualObject;


#ifdef _OMP_KARABAS
/* MacroBlock Info for MT */
typedef struct _mp4_MacroBlockMT {
    Ipp16s    dctCoeffs[64*6];
    IppMotionVector mvF[4];     // B-VOP
    IppMotionVector mvB[4];     // B-VOP
    int       lnz[6];
    Ipp8u     pat;
    Ipp8u     mb_type;          // B-VOP
    Ipp8u     dct_type;         // interlaced
    Ipp8u     field_info;       // interlaced
    Ipp8u     mcsel;            // S(GMC)-VOP
    Ipp8u     dummy[3];         // align 16
} mp4_MacroBlockMT;
#endif // _OMP_KARABAS

/* Full Info */
typedef struct _mp4_Info {
    int         ftype;          // 0 - raw, 1 - mp4, 2 - avi
    int         ftype_f;        // ftype == 1 (0 - QuickTime(tm)), ftype == 2 (0 - DivX(tm) v. < 5, XVID, 1 - DivX(tm) v. >= 5)
    int         read;           // bytes read
    FILE*       stream;
    Ipp8u*      buffer;         /* buffer header for saving MPEG-4 stream */
    size_t      buflen;         /* total buffer length */
    size_t      len;            /* valid data in buf_head[], counter from head */
    Ipp8u*      bufptr;         /* current frame, point to header or data */
    int         bitoff;         /* mostly point to next frame header or PSC */
    int         profile_and_level_indication;
    mp4_VisualObject    VisualObject;
#ifdef _OMP_KARABAS
    mp4_MacroBlockMT*  pMBinfoMT;  /* OpenMP buffer */
    int         num_threads;       /* OpenMP number of threads */
#endif // _OMP_KARABAS

	//***kinoma optimization
	IppStatus (__STDCALL *m_ippiDCT8x8Inv_16s_C1I)					(Ipp16s* pSrcDst);
	IppStatus (__STDCALL *m_ippiDCT8x8Inv_4x4_16s_C1I)				(Ipp16s* pSrcDst);
	IppStatus (__STDCALL *m_ippiDCT8x8Inv_2x2_16s_C1I)				(Ipp16s* pSrcDst);
	IppStatus (__STDCALL *m_ippiDCT8x8Inv_16s8u_C1R)				(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
	IppStatus (__STDCALL *m_ippiDCT8x8Inv_4x4_16s8u_C1R)			(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);
	IppStatus (__STDCALL *m_ippiDCT8x8Inv_2x2_16s8u_C1R)			(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep);

} mp4_Info;

//***kinoma optimization
extern void mp4_SetDefaultIDCTProcs( mp4_Info *pInfo );
extern void mp4_SetApprox( mp4_Info *pInfo, int level );

/* bitstream functions */
extern mp4_Status mp4_InitStream(mp4_Info* pInfo, char *fname);
extern mp4_Status mp4_CloseStream(mp4_Info* pInfo);
extern mp4_Status mp4_InitBuffer(mp4_Info* pInfo, int bSize);
extern mp4_Status mp4_CloseBuffer(mp4_Info* pInfo);
extern int        mp4_ReadStreamChunk(mp4_Info* pInfo, int size);
extern int        mp4_RemainStream(mp4_Info* pInfo);
extern int        mp4_ReadStream(mp4_Info* pInfo, int size);
extern int        mp4_LoadStream(mp4_Info* pInfo);
extern int        mp4_ReloadStream(mp4_Info* pInfo);
extern Ipp8u*     mp4_FindStartCodePtr(mp4_Info* pInfo);
extern Ipp8u*     mp4_FindStartCodeOrShortPtr(mp4_Info* pInfo);
extern int        mp4_SeekStartCodePtr(mp4_Info* pInfo);
extern int        mp4_SeekStartCodeOrShortPtr(mp4_Info* pInfo);
extern int        mp4_SeekStartCodeValue(mp4_Info* pInfo, Ipp8u code);
extern Ipp8u*     mp4_FindShortVideoStartMarkerPtr(mp4_Info* pInfo);
extern int        mp4_SeekShortVideoStartMarker(mp4_Info* pInfo);
extern int        mp4_CheckResyncMarker(mp4_Info* pInfo, int rml);

/* tables */
typedef struct _mp4_VLC1 {
    Ipp8u  code;
    Ipp8u  len;
} mp4_VLC1;
extern const Ipp8u mp4_DefaultIntraQuantMatrix[];
extern const Ipp8u mp4_DefaultNonIntraQuantMatrix[];
extern const Ipp8u mp4_ClassicalZigzag[];
extern const Ipp8u mp4_DCScalerLuma[];
extern const Ipp8u mp4_DCScalerChroma[];
extern const Ipp8u mp4_cCbCrMvRound16[];
extern const Ipp8u mp4_cCbCrMvRound12[];
extern const Ipp8u mp4_cCbCrMvRound8[];
extern const Ipp8u mp4_cCbCrMvRound4[];
extern const Ipp8s mp4_dquant[];
extern const mp4_VLC1 mp4_cbpy1[];
extern const mp4_VLC1 mp4_cbpy2[];
extern const mp4_VLC1 mp4_cbpy3[];
extern const mp4_VLC1 mp4_cbpy4[];
extern const mp4_VLC1* mp4_cbpy_t[];
extern const Ipp8u mp4_cbpy_b[];
extern const int mp4_DC_vlc_Threshold[];
extern const Ipp8u mp4_PVOPmb_type[];
extern const Ipp8u mp4_PVOPmb_cbpc[];
extern const Ipp8u mp4_PVOPmb_bits[];
extern const mp4_VLC1 mp4_BVOPmb_type[];
extern const mp4_VLC1 mp4_MVD_B12_1[];
extern const mp4_VLC1 mp4_MVD_B12_2[];
extern const int mp4_H263_width[];
extern const int mp4_H263_height[];
extern const int mp4_H263_mbgob[];
extern const int mp4_H263_gobvop[];
extern const Ipp8u mp4_aux_comp_count[];
extern const Ipp8u mp4_aux_comp_is_alpha[];
extern const Ipp8u mp4_BABtypeIntra[][3];
extern const Ipp32s mp4_DivIntraDivisor[];

// project functions
//extern void       mp4_Error(char *str);
#define mp4_Error(str) //***bnie: vm_debug_trace(4, __VM_STRING(str))
extern mp4_Status mp4_InitDecoder(mp4_Info *pInfo, char *mp4FileName);
extern mp4_Status mp4_CloseDecoder(mp4_Info *pInfo);
extern mp4_Status mp4_InitVOL(mp4_Info *pInfo);
extern mp4_Status mp4_FreeVOL(mp4_Info *pInfo);
extern mp4_Status mp4_Decoder(mp4_Info *pInfo);
//extern void       mp4_ShowFrame(mp4_Frame *frame);
#define mp4_ShowFrame(frame)
extern mp4_Status mp4_Parse_VisualObject(mp4_Info* pInfo);
extern mp4_Status mp4_Parse_VideoObject(mp4_Info* pInfo);
extern mp4_Status mp4_Parse_GroupOfVideoObjectPlane(mp4_Info* pInfo);
extern mp4_Status mp4_Parse_VideoObjectPlane(mp4_Info* pInfo);
extern mp4_Status mp4_DecodeVideoObjectPlane(mp4_Info* pInfo);
extern mp4_Status mp4_DecodeVideoPacket(mp4_Info* pInfo, int *quant_scale, int *found);
extern mp4_Status mp4_InitVOPShape(mp4_Info *pInfo);
extern mp4_Status mp4_FreeVOPShape(mp4_Info *pInfo);


#ifdef _OMP_KARABAS
#ifdef _OPENMP
#include <omp.h>
#endif
extern int mp4_GetNumOfThreads(void);
#endif // _OMP_KARABAS


#ifndef USE_INLINE_BITS_FUNC
extern Ipp32u mp4_ShowBits(mp4_Info* pInfo, int n);
extern Ipp32u mp4_ShowBit(mp4_Info* pInfo);
extern Ipp32u mp4_ShowBits9(mp4_Info* pInfo, int n);
extern void   mp4_FlushBits(mp4_Info* pInfo, int n);
extern Ipp32u mp4_GetBits(mp4_Info* pInfo, int n);
//extern Ipp32u mp4_GetBit(mp4_Info* pInfo);
extern Ipp32u mp4_GetBits9(mp4_Info* pInfo, int n);
extern void   mp4_AlignBits(mp4_Info* pInfo);
extern void   mp4_AlignBits7F(mp4_Info* pInfo);
extern Ipp32u mp4_ShowBitsAlign(mp4_Info* pInfo, int n);
extern Ipp32u mp4_ShowBitsAlign7F(mp4_Info* pInfo, int n);
#else
__INLINE Ipp32u mp4_ShowBits(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bufptr;
    Ipp32u tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp <<= pInfo->bitoff;
    tmp >>= 32 - n;
    return tmp;
}

__INLINE Ipp32u mp4_ShowBit(mp4_Info* pInfo)
{
    Ipp32u tmp = pInfo->bufptr[0];
    tmp >>= 7 - pInfo->bitoff;
    return (tmp & 1);
}

__INLINE Ipp32u mp4_ShowBits9(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bufptr;
    Ipp32u tmp = (ptr[0] <<  8) | ptr[1];
    tmp <<= (pInfo->bitoff + 16);
    tmp >>= 32 - n;
    return tmp;
}

__INLINE void mp4_FlushBits(mp4_Info* pInfo, int n)
{
    n = n + pInfo->bitoff;
    pInfo->bufptr += n >> 3;
    pInfo->bitoff = n & 7;
}

__INLINE Ipp32u mp4_GetBits(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bufptr;
    Ipp32u tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp <<= pInfo->bitoff;
    tmp >>= 32 - n;
    n = n + pInfo->bitoff;
    pInfo->bufptr += n >> 3;
    pInfo->bitoff = n & 7;
    return tmp;
}

__INLINE Ipp32u mp4_GetBits9(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bufptr;
    Ipp32u tmp = (ptr[0] <<  8) | ptr[1];
    tmp <<= (pInfo->bitoff + 16);
    tmp >>= 32 - n;
    n = n + pInfo->bitoff;
    pInfo->bufptr += n >> 3;
    pInfo->bitoff = n & 7;
    return tmp;
}

__INLINE void mp4_AlignBits(mp4_Info* pInfo)
{
    if (pInfo->bitoff > 0) {
        pInfo->bitoff = 0;
        (pInfo->bufptr)++;
    }
}

__INLINE void mp4_AlignBits7F(mp4_Info* pInfo)
{
    if (pInfo->bitoff > 0) {
        pInfo->bitoff = 0;
        (pInfo->bufptr)++;
    } else {
        if (*pInfo->bufptr == 0x7F)
            (pInfo->bufptr)++;
    }
}

__INLINE Ipp32u mp4_ShowBitsAlign(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bitoff ? (pInfo->bufptr + 1) : pInfo->bufptr;
    Ipp32u tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp >>= 32 - n;
    return tmp;
}

__INLINE Ipp32u mp4_ShowBitsAlign7F(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bitoff ? (pInfo->bufptr + 1) : pInfo->bufptr;
    Ipp32u tmp;
    if (!pInfo->bitoff) {
        if (*ptr == 0x7F)
            ptr ++;
    }
    tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp >>= 32 - n;
    return tmp;
}

#endif // USE_INLINE_BITS_FUNC


__INLINE Ipp32u mp4_GetBit(mp4_Info* pInfo)
{
    Ipp32u tmp = pInfo->bufptr[0];
    if (pInfo->bitoff != 7) {
        tmp >>= 7 - pInfo->bitoff;
        pInfo->bitoff ++;
    } else {
        pInfo->bitoff = 0;
        pInfo->bufptr ++;
    }
    return (tmp & 1);
}


__INLINE int mp4_GetMarkerBit(mp4_Info* pInfo) {
    if (!mp4_GetBit(pInfo)) {
        mp4_Error("Error in video_header: wrong marker bit");
        return 0;
    }
    return 1;
}


#ifdef __cplusplus
}
#endif


#endif  //_MP4_H_


