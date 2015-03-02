/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __AACCMN_DEC_CONST_H
#define __AACCMN_DEC_CONST_H

#define SF_OFFSET  100
#define SF_MID     60

#define MAX_ENCODER_CHANNEL_NUMBER 6

#define MAX_SFB           51
#define MAX_GROUP_NUMBER  8
#define MAX_NUMBER_PULSE  5
#define MAX_ORDER         32

#define NUM_WINDOW_SHAPE      2
#define NUM_WINDOW_SEQUENCES  4

enum eSAACDEC_FILTERBANK_H {
  N_SHORT = 256,
  N_LONG = 2048,
  FB_DECODER = 0x1,
  FB_ENCODER = 0x2
};

enum eSAACDEC_TNS_H {
    TNS_MAX_ORDER_SHORT    = 7,
    TNS_MAX_ORDER_LONG_LC  = 12,
    TNS_MAX_ORDER_LONG_LTP = 20,
    TNS_MAX_ORDER          = 20
};

enum DecodeMode
{
    DM_UNDEF_STREAM = 0, /// At the begin of decoding.
    DM_RAW_STREAM,
    DM_ADTS_STREAM
};

enum AudioObjectType
{
    AOT_UNDEF = 0,
    AOT_AAC_MAIN = 1,
    AOT_AAC_LC,
    AOT_AAC_SSR,
    AOT_AAC_LTP,
    AOT_SBR = 5,
    AOT_AAC_scalable,
    AOT_TwinVQ,
    AOT_CELP,
    AOT_HVXC,
    AOT_TTSI,
    AOT_Main_synthetic,
    AOT_Wavetable_synthesis,
    AOT_General_MIDI,
    AOT_Algorithmic_Synthesis_and_Audio_FX,
    AOT_ER_AAC_LC,
    AOT_ER_AAC_LTP,
    AOT_ER_AAC_scalable,
    AOT_ER_Twin_VQ,
    AOT_ER_BSAC,
    AOT_ER_AAC_LD,
    AOT_ER_CELP,
    AOT_ER_HVXC,
    AOT_ER_HILN,
    AOT_ER_Parametric
    //NULL_OBJECT      =  0,  /* NULL Object */
    //AAC_MAIN         =  1,  /* AAC Main Object */
    //AAC_LC           =  2,  /* AAC Low Complexity(LC) Object */
    //AAC_SSR          =  3,  /* AAC Scalable Sampling Rate(SSR) Object */
    //AAC_LTP          =  4,  /* AAC Long Term Predictor(LTP) Object */
    //AAC_SBR          =  5,  /* AAC SBR Object */
    //AAC_SCAL         =  6,  /* AAC Scalable Object */
    //TWIN_VQ          =  7,  /* TwinVQ Object */
    //CELP             =  8,  /* CELP Object */
    //HVXC             =  9,  /* HVXC Object */
    //RSVD_10          = 10,  /* (reserved) */
    //RSVD_11          = 11,  /* (reserved) */
    //TTSI             = 12,  /* TTSI Object(not supported) */
    //MAIN_SYNTH       = 13,  /* Main Synthetic Object(not supported) */
    //WAV_TAB_SYNTH    = 14,  /* Wavetable Synthesis Object(not supported) */
    //GEN_MIDI         = 15,  /* General MIDI Object(not supported) */
    //ALG_SYNTH_AUD_FX = 16,  /* Algorithmic Synthesis and Audio FX Object(not supported) */
    //ER_AAC_LC        = 17,  /* Error Resilient(ER) AAC Low Complexity(LC) Object */
    //RSVD_18          = 18,  /* (reserved)
    //ER_AAC_LTP       = 19,  /* Error Resilient(ER) AAC Long Term Predictor(LTP) Object */
    //ER_AAC_SCAL      = 20,  /* Error Resilient(ER) AAC Scalable Object */
    //ER_TWIN_VQ       = 21,  /* Error Resilient(ER) TwinVQ Object */
    //ER_BSAC          = 22,  /* Error Resilient(ER) BSAC Object */
    //ER_AAC_LD        = 23,  /* Error Resilient(ER) AAC LD Object */
    //ER_CELP          = 24,  /* Error Resilient(ER) CELP Object */
    //ER_HVXC          = 25,  /* Error Resilient(ER) HVXC Object */
    //ER_HILN          = 26,  /* Error Resilient(ER) HILN Object */
    //ER_PARA          = 27,  /* Error Resilient(ER) Parametric Object */
    //RSVD_28          = 28,  /* (reserved) */
    //RSVD_29          = 29,  /* (reserved) */
    //RSVD_30          = 30,  /* (reserved) */
    //RSVD_31          = 31   /* (reserved) */
};


enum eId
{
    ID_SCE = 0x0,
    ID_CPE = 0x1,
    ID_CCE = 0x2,
    ID_LFE = 0x3,
    ID_DSE = 0x4,
    ID_PCE = 0x5,
    ID_FIL = 0x6,
    ID_END = 0x7
};

enum eWindowsSequencess
{
    ONLY_LONG_SEQUENCE   = 0,
    LONG_START_SEQUENCE  = 1,
    EIGHT_SHORT_SEQUENCE = 2,
    LONG_STOP_SEQUENCE   = 3
};

#define ZERO_HCB         0
#define NOISE_HCB       13
#define INTENSITY_HCB2  14
#define INTENSITY_HCB   15

#define MAX_LFE_FREQUENCY  600
#define MAX_SECTION_NUMBER 120

enum eConst
{
    MAX_NUM_WINDOWS   =  8,
    MAX_FILT          =  4,
    MAX_LTP_SFB_LONG  =  40,
    MAX_LTP_SFB_SHORT =  8
};

#endif//AACCMN_DEC_CONST_H
