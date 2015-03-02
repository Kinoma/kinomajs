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

#ifndef __UMC_H264_DEC_DEBLOCKING_H
#define __UMC_H264_DEC_DEBLOCKING_H

namespace UMC
{

#if defined(_MSC_VER) && !defined(_WIN32_WCE)
#define __align(value) __declspec(align(value))
#else // !defined(_MSC_VER)
#define __align(value)
#endif // defined(_MSC_VER)

#define IClip(Min, Max, Val) (((Val) < (Min)) ? (Min) : (((Val) > (Max)) ? (Max) : (Val)))
#define SetEdgeStrength(edge, strength) \
    *((Ipp32u *) (edge)) = (((((strength) * 256) + strength) * 256 + strength) * 256 + strength)
#define CopyEdgeStrength(dst_edge, src_edge) \
    *((Ipp32u *) (dst_edge)) = (*((Ipp32u *) (src_edge)))
#define CompareEdgeStrength(strength, edge) \
    ((((((strength) * 256) + strength) * 256 + strength) * 256 + strength) == *((Ipp32u *) (edge)))

// declare used types and constants

enum
{
    VERTICAL_DEBLOCKING     = 0,
    HORIZONTAL_DEBLOCKING   = 1,
    NUMBER_OF_DIRECTION     = 2
};

enum
{
    CURRENT_BLOCK           = 0,
    NEIGHBOUR_BLOCK         = 1
};

// alpha table
extern
Ipp8u ALPHA_TABLE[52];

// beta table
extern
Ipp8u BETA_TABLE[52];

// clipping table
extern
Ipp8u CLIP_TAB[52][5];

// chroma scaling QP table
extern
Ipp8u QP_SCALE_CR[52];

// masks for external blocks pair "coded bits"
extern
Ipp32u EXTERNAL_BLOCK_MASK[NUMBER_OF_DIRECTION][2][4];

// masks for internal blocks pair "coded bits"
extern
Ipp32u INTERNAL_BLOCKS_MASK[NUMBER_OF_DIRECTION][12];

#pragma pack(16)

typedef struct DeblockingParameters
{
    Ipp8u Strength[NUMBER_OF_DIRECTION][16];                    // (Ipp8u [][]) arrays of deblocking sthrengths
    Ipp32u DeblockingFlag[NUMBER_OF_DIRECTION];                 // (Ipp32u []) flags to do deblocking
    Ipp32u ExternalEdgeFlag[NUMBER_OF_DIRECTION];               // (Ipp32u []) flags to do deblocking on external edges
    Ipp32u nMBAddr;                                             // (Ipp32u) macroblock number
    Ipp32u nMaxMVector;                                         // (Ipp32u) maximum vertical motion vector
    Ipp32u nNeighbour[NUMBER_OF_DIRECTION];                     // (Ipp32u) neighbour macroblock addres
    //***bnie: Ipp32u MBFieldCoded;                                        // (Ipp32u) flag means macroblock is field coded (picture may not)
    Ipp32s nAlphaC0Offset;                                      // (Ipp32s) alpha c0 offset
    Ipp32s nBetaOffset;                                         // (Ipp32s) beta offset
    Ipp8u *pLuma;                                               // (Ipp8u *) pointer to luminance data
    Ipp8u *pChroma[2];                                          // (Ipp8u *) pointer to chrominance data
    Ipp32s pitch;                                               // (Ipp32s) working pitch

#ifdef _KINOMA_LOSSY_OPT_
    Ipp32s nCurrMB_X, nCurrMB_Y;								//<!Must used signed int! MB position of current MB
	Ipp8u	*pY, *pU, *pV;
	//Ipp32s MBYAdjust;
#endif

} DeblockingParameters;

typedef struct DeblockingParametersMBAFF : public DeblockingParameters
{
    Ipp8u StrengthComplex[16];                                  // (Ipp8u) arrays of deblocking sthrengths
    Ipp8u StrengthExtra[16];                                    // (Ipp8u) arrays of deblocking sthrengths
    Ipp32u UseComplexVerticalDeblocking;                        // (Ipp32u) flag to do complex deblocking on external vertical edge
    Ipp32u ExtraHorizontalEdge;                                 // (Ipp32u) flag to do deblocking on extra horizontal edge
    Ipp32u nLeft[2];                                            // (Ipp32u []) left couple macroblock numbers

} DeblockingParametersMBAFF;

#pragma pack()

#if defined(_WIN32_WCE) && defined(_M_IX86) && defined(__stdcall)
#define _IPP_STDCALL_CDECL
#undef __stdcall
#endif // defined(_WIN32_WCE) && defined(_M_IX86) && defined(__stdcall)

// implement array of IPP optimized luma deblocking functions
extern
IppStatus (__STDCALL *(IppLumaDeblocking[])) (Ipp8u *, Ipp32s, const Ipp8u *, const Ipp8u *, const Ipp8u *, const Ipp8u *);

// implement array of IPP optimized chroma deblocking functions
extern
IppStatus (__STDCALL *(IppChromaDeblocking[])) (Ipp8u *, Ipp32s, const Ipp8u *, const Ipp8u *, const Ipp8u *, const Ipp8u *);

#if defined(_IPP_STDCALL_CDECL)
#undef _IPP_STDCALL_CDECL
#define __stdcall __cdecl
#endif // defined(_IPP_STDCALL_CDECL)

} // namespace UMC

#endif // __UMC_H264_DEC_DEBLOCKING_H
