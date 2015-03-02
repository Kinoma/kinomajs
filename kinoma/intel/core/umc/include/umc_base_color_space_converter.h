/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __UMC_BASE_COLOR_SPACE_CONVERTER_H
#define __UMC_BASE_COLOR_SPACE_CONVERTER_H

#include "umc_structures.h"
#include "umc_dynamic_cast.h"
#include "vm_types.h"

namespace UMC
{

// forward declaration(s)
class BaseColorSpaceConverter;
class ColorConversionParams;

// declaration of internal type(s)
typedef void (BaseColorSpaceConverter::*FuncIndex)(ColorConversionParams &ConvertParam);

#if defined(x86) || defined(_X86_)
#pragma pack(1)
#endif // defined(x86) || defined(_X86_)

struct ColorFormatConsts
{
    vm_var32 uiWidthAlign;      // Alignment as power of 2 for Width value:
                                // Width &= uiWidthAlign
    vm_var32 uiHeightAlign;     // Alignment as power of 2 for Height value:
                                // Height &= uiHeightAlign
    vm_var32 uiBytesPerPixPl0;  // Number of bytes per pixel in Plane0
    vm_var32 uiPixPerBytePl12;  // Number of pixels per byte for Plane1
                                // and Plane2
    vm_var32 uiColPerLinePl12;  // Number of lines in picture per line in
                                // Plane1 or Plane2
    vm_var32 uiPlanNum;         // Number of user planes - Plane0, Plane1, Plane2
};

class ColorConversionInfo
{
public:
    // Default constructor
    ColorConversionInfo(void)
    {   Reset();    }
    // Destructor
    virtual ~ColorConversionInfo(void){}

    ColorFormat FormatSource;           // (ColorFormat) format of source image
    ColorFormat FormatDest;             // (ColorFormat) format of destination image
    ClipInfo SizeSource;                // (ClipInfo) size of source image
    ClipInfo SizeDest;                  // (ClipInfo) size of destination image
    UMC::RECT SrcCropRect;              // (UMC::RECT) crop region
    ColorFormatConsts SrcFmtConsts;     // (ColorFormatConsts) constants for source format layout
    ColorFormatConsts DstFmtConsts;     // (ColorFormatConsts) constants for destination format layout
    vm_var32 lInterpolation;            // (vm_var32) type of interpolation to perform for image resampling (see ippi.h)
    vm_var32 lFlags;                    // (vm_var32) resize flag(s)
    vm_var32 lDeinterlace;              // (vm_var32) deinterlace flag(s)

    FuncIndex iInternal;                // internal use index

    void Reset(void)
    {
        FormatSource = NONE;
        FormatDest = NONE;
        memset(&SizeSource, 0, sizeof(SizeSource));
        memset(&SizeDest, 0, sizeof(SizeDest));
        memset(&SrcCropRect, 0, sizeof(SrcCropRect));
        memset(&SrcFmtConsts, 0, sizeof(SrcFmtConsts));
        memset(&DstFmtConsts, 0, sizeof(DstFmtConsts));
        lInterpolation = 0;
        lFlags = 0;
        lDeinterlace = 0;
		iInternal = NULL;
    }
};

class ColorConversionParams
{
public:
    // Default constructor
    ColorConversionParams(void)
    {   Reset();    }
    // Destructor
    virtual ~ColorConversionParams(void){}

    vm_byte *lpSource0;                 // (vm_byte *) first source pointer (to any composite format or to first planar component Y)
    vm_byte *lpSource1;                 // (vm_byte *) second source pointer (to second planar component U or UV merged)
    vm_byte *lpSource2;                 // (vm_byte *) third source pointer (to third planar component V)
    size_t PitchSource0;                // (size_t) pitch of first source pointer (composite format or first planar component Y)
    size_t PitchSource1;                // (size_t) pitch of second source pointer (second planar component U or UV merged)
    size_t PitchSource2;                // (size_t) pitch of third source pointer (third planar component V)

    vm_byte *lpDest0;                   // (vm_byte *) first destination pointer (to any composite format or to first planar component Y)
    vm_byte *lpDest1;                   // (vm_byte *) second destination pointer (to first planar component U or UV merged)
    vm_byte *lpDest2;                   // (vm_byte *) third destination pointer (to first planar component V)
    size_t PitchDest0;                  // (size_t) pitch of first dest pointer (composite format or first planar component Y)
    size_t PitchDest1;                  // (size_t) pitch of second dest pointer (second planar component U or UV merged)
    size_t PitchDest2;                  // (size_t) pitch of third dest pointer (third planar component V)

    ColorConversionInfo ConversionInit; // (ColorConversionInfo) converter initialization info

    void Reset()
    {
        lpSource0 = NULL;
        lpSource1 = NULL;
        lpSource2 = NULL;

        PitchSource0 = 0;
        PitchSource1 = 0;
        PitchSource2 = 0;

        lpDest0 = NULL;
        lpDest1 = NULL;
        lpDest2 = NULL;

        PitchDest0 = 0;
        PitchDest1 = 0;
        PitchDest2 = 0;

        ConversionInit.Reset();
    }
};

#if defined(x86) || defined(_X86_)
#pragma pack()
#endif // defined(x86) || defined(_X86_)

class BaseColorSpaceConverter
{
    DYNAMIC_CAST_DECL_BASE(BaseColorSpaceConverter)
public:
    // Default constructor
    BaseColorSpaceConverter(void){}
    // Destructor
    virtual ~BaseColorSpaceConverter(void){}

    // Initialize space converter
    virtual Status Init(ColorConversionInfo &InitParam) = 0;

    // Begin color conversion
    virtual Status BeginFrame(ColorConversionParams *lpConvertParam) = 0;

    // Convert image(s)
    virtual Status ConvertFrame(ColorConversionParams *lpConvertParam) = 0;

    // Convert slice of image(s)
    virtual Status ConvertSlice(ColorConversionParams *lpConvertParam, vm_var32 lSliceNum) = 0;

    // End color conversion
    virtual Status CloseFrame(ColorConversionParams *lpConvertParam) = 0;

    // Initialize space converter
    virtual Status Close(void) = 0;

};

} // end namespace UMC

#endif /* __UMC_BASE_COLOR_SPACE_CONVERTER_H */
