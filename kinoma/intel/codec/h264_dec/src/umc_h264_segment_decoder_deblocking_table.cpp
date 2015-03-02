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
//***
#include "kinoma_ipp_lib.h"

#include "umc_h264_dec.h"
#include "umc_h264_dec_deblocking.h"
#include "vm_types.h"
#include "vm_debug.h"

namespace UMC
{

// alpha table
Ipp8u ALPHA_TABLE[52] =
{
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    4,  4,  5,  6,  7,  8,  9,  10,
    12, 13, 15, 17, 20, 22, 25, 28,
    32, 36, 40, 45, 50, 56, 63, 71,
    80, 90, 101,113,127,144,162,182,
    203,226,255,255
};

// beta table
Ipp8u BETA_TABLE[52] =
{
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    2,  2,  2,  3,  3,  3,  3,  4,
    4,  4,  6,  6,  7,  7,  8,  8,
    9,  9,  10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18
};

// clipping table
Ipp8u CLIP_TAB[52][5] =
{
    { 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 1, 1, 1, 1},
    { 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 2, 3, 3},
    { 0, 1, 2, 3, 3},{ 0, 2, 2, 3, 3},{ 0, 2, 2, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 3, 3, 5, 5},{ 0, 3, 4, 6, 6},{ 0, 3, 4, 6, 6},
    { 0, 4, 5, 7, 7},{ 0, 4, 5, 8, 8},{ 0, 4, 6, 9, 9},{ 0, 5, 7,10,10},{ 0, 6, 8,11,11},{ 0, 6, 8,13,13},{ 0, 7,10,14,14},{ 0, 8,11,16,16},
    { 0, 9,12,18,18},{ 0,10,13,20,20},{ 0,11,15,23,23},{ 0,13,17,25,25}
};

// chroma scaling QP table
Ipp8u QP_SCALE_CR[52] =
{
    0,  1,  2,  3,  4,  5,  6,  7,
    8,  9,  10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 29, 30,
    31, 32, 32, 33, 34, 34, 35, 35,
    36, 36, 37, 37, 37, 38, 38, 38,
    39, 39, 39, 39
};

// masks for external blocks pair "coded bits"
Ipp32u EXTERNAL_BLOCK_MASK[NUMBER_OF_DIRECTION][2][4] =
{
    // block mask for vertical deblocking
    {
        {2 << 0, 2 << 2, 2 << 8, 2 << 10},
        {2 << 5, 2 << 7, 2 << 13,2 << 15}
    },

    // block mask for horizontal deblocking
    {
        {2 << 0, 2 << 1, 2 << 4, 2 << 5},
        {2 << 10,2 << 11,2 << 14,2 << 15}
    }
};

#define DECL(prev, cur) (2 << (prev) | 2 << (cur))
// masks for internal blocks pair "coded bits"
Ipp32u INTERNAL_BLOCKS_MASK[NUMBER_OF_DIRECTION][12] =
{
    // blocks pair-mask for vertical deblocking
    {
        DECL(0, 1),  DECL(2, 3),  DECL(8, 9),  DECL(10, 11),
        DECL(1, 4),  DECL(3, 6),  DECL(9, 12), DECL(11, 14),
        DECL(4, 5),  DECL(6, 7),  DECL(12, 13),DECL(14, 15)
    },

    // blocks pair-mask for horizontal deblocking
    {
        DECL(0, 2),  DECL(1, 3),  DECL(4, 6),  DECL(5, 7),
        DECL(2, 8),  DECL(3, 9),  DECL(6, 12), DECL(7, 13),
        DECL(8, 10), DECL(9, 11), DECL(12, 14),DECL(13, 15)
    }
};
#undef DECL

// implement array of IPP optimized luma deblocking functions
#if defined(_WIN32_WCE) && defined(_M_IX86) && defined(__stdcall)
#define _IPP_STDCALL_CDECL
#undef __stdcall
#endif // defined(_WIN32_WCE) && defined(_M_IX86) && defined(__stdcall)

IppStatus (__STDCALL *(IppLumaDeblocking[])) (Ipp8u *, Ipp32s, const Ipp8u *, const Ipp8u *, const Ipp8u *, const Ipp8u *) =
{
    &(ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_x),
    &(ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_x)
};

// implement array of IPP optimized chroma deblocking functions
IppStatus (__STDCALL *(IppChromaDeblocking[])) (Ipp8u *, Ipp32s, const Ipp8u *, const Ipp8u *, const Ipp8u *, const Ipp8u *) =
{
    &(ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_x),
    &(ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_x)
};

#if defined(_IPP_STDCALL_CDECL)
#undef _IPP_STDCALL_CDECL
#define __stdcall __cdecl
#endif // defined(_IPP_STDCALL_CDECL)

} // end namespace UMC
