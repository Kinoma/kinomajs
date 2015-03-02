/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives AAC Decode Sample for Windows*
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel(R) Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel(R) IPP
//  product installation for more information.
//
//  MPEG-4 and AAC are international standards promoted by ISO, IEC, ITU, ETSI
//  and other organizations. Implementations of these standards, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
*/

/********************************************************************/

#include "aac_sfb_tables.h"

/********************************************************************/

int     sfb_offset_long_window_88_96[] = {
  0,
  4,   8,   12,  16,   20, 24,  28,
  32,  36,  40,  44,   48, 52,  56,
  64,  72,  80,  88,   96, 108, 120,
  132, 144, 156, 172, 188, 212, 240,
  276, 320, 384, 448, 512, 576, 640,
  704, 768, 832, 896, 960, 1024
};

int     sfb_offset_short_window_88_96[] = {
  0,
  4,   8, 12, 16, 20, 24, 32,
  40, 48, 64, 92, 128
};

int     sfb_offset_long_window_64[] = {
  0,    // ////
  4,   8,   12,   16, 20,   24, 28,
  32,  36,  40,   44, 48,   52, 56,
  64,  72,  80,   88, 100, 112, 124,
  140, 156, 172, 192, 216, 240, 268,
  304, 344, 384, 424, 464, 504, 544,
  584, 624, 664, 704, 744, 784, 824,
  864, 904, 944, 984, 1024
};

int     sfb_offset_short_window_64[] = {
  0,    // ///
  4,   8, 12, 16, 20, 24, 32,
  40, 48, 64, 92, 128
};

/*
 Scalefactor bands for a window length of 2048 and 1920 (values for 1920 in brackets) for LONG_WINDOW,
 LONG_START_WINDOW, LONG_STOP_WINDOW at 44.1 and 48 kHz
*/

int     sfb_offset_long_window_41_48[] = {
  0,    // / Using for ippsApplySF_I function
  4,    8,  12,  16,  20,  24,  28,
  32,   36, 40,  48,  56,  64,  72,
  80,   88, 96,  108, 120, 132, 144,
  160, 176, 196, 216, 240, 264, 292,
  320, 352, 384, 416, 448, 480, 512,
  544, 576, 608, 640, 672, 704, 736,
  768, 800, 832, 864, 896, 928, 1024
};

/*
 Scalefactor bands for a window length of 256 and 240 (values for 240 in brackets) for SHORT_WINDOW
 at 32, 44.1 and 48 kHz
*/
int     sfb_offset_short_window_32_41_48[] = {
  0,
  4,   8, 12, 16, 20, 28, 36,
  44, 56, 68, 80, 96, 112, 128
};

int     sfb_offset_long_window_32[] = {
  0,
  4,    8,  12,  16,  20,  24,  28,
  32,  36,  40,  48,  56,  64,  72,
  80,  88,  96,  108, 120, 132, 144,
  160, 176, 196, 216, 240, 264, 292,
  320, 352, 384, 416, 448, 480, 512,
  544, 576, 608, 640, 672, 704, 736,
  768, 800, 832, 864, 896, 928, 960,
  992, 1024
};

int     sfb_offset_long_window_22_24[] = {
  0,
  4,     8, 12, 16, 20, 24, 28,
  32,   36, 40, 44, 52, 60, 68,
  76,   84, 92, 100, 108, 116, 124,
  136, 148, 160, 172, 188, 204, 220,
  240, 260, 284, 308, 336, 364, 396,
  432, 468, 508, 552, 600, 652, 704,
  768, 832, 896, 960, 1024
};

int     sfb_offset_short_window_22_24[] = {
  0,
  4,   8, 12, 16, 20, 24, 28,
  36, 44, 52, 64, 76, 92, 108,
  128
};

int     sfb_offset_long_window_11_12_16[] = {
  0,
  8,   16,  24,  32,  40,   48, 56,
  64,  72,  80,  88,  100, 112, 124,
  136, 148, 160, 172, 184, 196, 212,
  228, 244, 260, 280, 300, 320, 344,
  368, 396, 424, 456, 492, 532, 572,
  616, 664, 716, 772, 832, 896, 960,
  1024
};

int     sfb_offset_short_window_11_12_16[] = {
  0,
  4,  8,  12, 16, 20, 24, 28,
  32, 40, 48, 60, 72, 88, 108,
  128
};

int     sfb_offset_long_window_8[] = {
  0,
  12,   24, 36,  48,  60,  72,  84,
  96,  108, 120, 132, 144, 156, 172,
  188, 204, 220, 236, 252, 268, 288,
  308, 328, 348, 372, 396, 420, 448,
  476, 508, 544, 580, 620, 664, 712,
  764, 820, 880, 944, 1024
};

int     sfb_offset_short_window_8[] = {
  0,
  4,  8,  12, 16, 20, 24, 28,
  36, 44, 52, 60, 72, 88, 108,
  128
};

#define NULL 0

sSfbTableElement sfb_tables[] = {
/*
 * sampling_frequency, #long sfb, long sfb, #short sfb, short sfb
 */
/*
 * samp_rate, nsfb1024, SFbands1024, nsfb128, SFbands128
 */
  {96000, 41, sfb_offset_long_window_88_96,    12, sfb_offset_short_window_88_96},    /* 96000 */
  {88200, 41, sfb_offset_long_window_88_96,    12, sfb_offset_short_window_88_96},    /* 88200 */
  {64000, 47, sfb_offset_long_window_64,       12, sfb_offset_short_window_64},       /* 64000 */
  {48000, 49, sfb_offset_long_window_41_48,    14, sfb_offset_short_window_32_41_48}, /* 48000 */
  {44100, 49, sfb_offset_long_window_41_48,    14, sfb_offset_short_window_32_41_48}, /* 44100 */
  {32000, 51, sfb_offset_long_window_32,       14, sfb_offset_short_window_32_41_48}, /* 32000 */
  {24000, 47, sfb_offset_long_window_22_24,    15, sfb_offset_short_window_22_24},    /* 24000 */
  {22050, 47, sfb_offset_long_window_22_24,    15, sfb_offset_short_window_22_24},    /* 22050 */
  {16000, 43, sfb_offset_long_window_11_12_16, 15, sfb_offset_short_window_11_12_16}, /* 16000 */
  {12000, 43, sfb_offset_long_window_11_12_16, 15, sfb_offset_short_window_11_12_16}, /* 12000 */
  {11025, 43, sfb_offset_long_window_11_12_16, 15, sfb_offset_short_window_11_12_16}, /* 11025 */
  {8000, 40, sfb_offset_long_window_8,         15, sfb_offset_short_window_8},        /* 8000  */
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0}

};
